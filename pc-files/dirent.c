/* @(#)dirent.c
 *
 * public domain implementation of POSIX directory routines for DOS and OS/2
 * Written by Michael Rendell ({uunet,utai}michael@garfield), August 1897
 *
 * Ported to OS/2 by Kai Uwe Rommel
 * December 1989, February 1990
 * Change for HPFS support, October 1990
 * Updated for better error checking and real rewinddir, February 1992
 * made POSIX conforming, February 1992
 */

#ifdef __EMX__
#include <sys/emx.h>
#define __32BIT__
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#define MAGIC    0x4711

#if !defined(TRUE) && !defined(FALSE)
#define TRUE    1
#define FALSE   0
#endif


#ifdef OS2

#ifndef __32BIT__
#define far     _far
#define near    _near
#define pascal  _pascal
#define cdecl   _cdecl
#endif

#define INCL_NOPM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#ifdef __32BIT__

#ifdef __EMX__

static struct _find find;
#define achName name
#define DosFindFirst(p, pd, a, pf, sf, pc)    __findfirst(p, a, pf)
#define DosFindNext(d, pf, sf, pc)            __findnext(pf)
#define DosFindClose(d)

#else

static HDIR hdir;
static ULONG count;
static FILEFINDBUF3 find;
#define DosFindFirst(p1, p2, p3, p4, p5, p6) \
        DosFindFirst(p1, p2, p3, p4, p5, p6, 1)

#endif

#define DosQCurDisk     DosQueryCurrentDisk
#define DosQFSAttach    DosQueryFSAttach

#define ENOTDIR EINVAL

#ifdef __IBMC__
#define S_IFMT 0xF000
#endif

#else

static HDIR hdir;
static USHORT count;
static FILEFINDBUF find;

#define DosQFSAttach(p1, p2, p3, p4, p5) \
        DosQFSAttach(p1, p2, p3, p4, p5, 0)
#define DosFindFirst(p1, p2, p3, p4, p5, p6) \
        DosFindFirst(p1, p2, p3, p4, p5, p6, 0)

#endif

#else

#include <dos.h>

#ifdef __TURBOC__

#include <dir.h>

static struct ffblk find;
#define achName ff_name
#define DosFindFirst(p, pd, a, pf, sf, pc)    findfirst(p, pf, a)
#define DosFindNext(d, pf, sf, pc)            findnext(pf)
#define DosFindClose(d)

#else

static struct find_t find;
#define achName name
#define DosFindFirst(p, pd, a, pf, sf, pc)    _dos_findfirst(p, a, pf)
#define DosFindNext(d, pf, sf, pc)            _dos_findnext(pf)
#define DosFindClose(d)

#endif

#endif


int attributes = _A_DIR | _A_HIDDEN;

static int getdirent(char *);
static int savedirent(DIR *);
static void free_dircontents(struct d_dircontents *);


DIR *opendir(char *name)
{
  struct stat statb;
  struct d_dircontents *dp;
  DIR *dirp;
  char nbuf[_POSIX_PATH_MAX + 1];
  char c, *s;

  if ( name == NULL || *name == '\0' )
  {
    errno = ENOENT;
    return NULL;
  }

  if ( (dirp = malloc(sizeof(DIR))) == NULL )
  {
    errno = ENOMEM;
    return NULL;
  }

  strcpy(nbuf, name);

  if ( ((c = nbuf[strlen(nbuf) - 1]) == '\\' || c == '/') &&
       (strlen(nbuf) > 1) )
  {
    nbuf[strlen(nbuf) - 1] = '\0';

    if ( nbuf[strlen(nbuf) - 1] == ':' )
      strcat(nbuf, "\\.");
  }
  else
    if ( nbuf[strlen(nbuf) - 1] == ':' )
      strcat(nbuf, ".");

  if ( stat(nbuf, &statb) == 0 )
  {
    if ( (statb.st_mode & S_IFMT) != S_IFDIR )
    {
      free(dirp);
      errno = ENOTDIR;
      return NULL;
    }

    if ( nbuf[strlen(nbuf) - 1] == '.'
	&& (strlen(nbuf) < 2 || nbuf[strlen(nbuf) - 2] != '.') )
      strcpy(nbuf + strlen(nbuf) - 1, "*.*");
    else
      if ( ((c = nbuf[strlen(nbuf) - 1]) == '\\' || c == '/') &&
	  (strlen(nbuf) == 1) )
	strcat(nbuf, "*.*");
      else
	strcat(nbuf, "\\*.*");
    
    _fullpath(dirp -> d_dd_name, nbuf, _POSIX_PATH_MAX);
  }
  else /* pattern, can't stat it, we hope it works ... */
    strcpy(dirp -> d_dd_name, nbuf);

  dirp -> d_dd_id  = MAGIC;
  dirp -> d_dd_loc = 0;
  dirp -> d_dd_contents = NULL;

  if ( getdirent(dirp -> d_dd_name) )
    do
    {
      if ( !savedirent(dirp) )
      {
	free_dircontents(dirp -> d_dd_contents);
	free(dirp);
	errno = ENOMEM;
	return NULL;
      }
    }
    while ( getdirent(NULL) );

  dirp -> d_dd_cp = dirp -> d_dd_contents;

  return dirp;
}


int closedir(DIR *dirp)
{
  if ( dirp == NULL || dirp -> d_dd_id != MAGIC )
  {
    errno = EBADF;
    return -1;
  }

  free_dircontents(dirp -> d_dd_contents);
  free(dirp);

  return 0;
}


struct dirent *readdir(DIR *dirp)
{
  static struct dirent dp;

  if ( dirp == NULL || dirp -> d_dd_cp == NULL || dirp -> d_dd_id != MAGIC )
  {
    errno = EBADF;
    return NULL;
  }

  dp.d_namlen = dp.d_reclen =
    strlen(strcpy(dp.d_name, dirp -> d_dd_cp -> d_dc_entry));

  dp.d_ino = 0;

  dp.d_size = dirp -> d_dd_cp -> d_dc_size;
  dp.d_mode = dirp -> d_dd_cp -> d_dc_mode;
  dp.d_time = dirp -> d_dd_cp -> d_dc_time;
  dp.d_date = dirp -> d_dd_cp -> d_dc_date;

  dirp -> d_dd_cp = dirp -> d_dd_cp -> d_dc_next;
  dirp -> d_dd_loc++;

  return &dp;
}


void seekdir(DIR *dirp, long off)
{
  long i = off;
  struct d_dircontents *dp;

  if ( dirp == NULL || dirp -> d_dd_id != MAGIC )
  {
    errno = EBADF;
    return;
  }

  if ( off >= 0 )
  {
    for ( dp = dirp -> d_dd_contents; (--i >= 0) && dp; dp = dp -> d_dc_next );

    dirp -> d_dd_loc = off - (i + 1);
    dirp -> d_dd_cp = dp;
  }
}


long telldir(DIR *dirp)
{
  if ( dirp == NULL || dirp -> d_dd_id != MAGIC )
  {
    errno = EBADF;
    return -1;
  }

  return dirp -> d_dd_loc;
}


void rewinddir(DIR *dirp)
{
  char *s;
  struct d_dircontents *dp;

  if ( dirp == NULL || dirp -> d_dd_id != MAGIC )
  {
    errno = EBADF;
    return;
  }

  free_dircontents(dirp -> d_dd_contents);
  dirp -> d_dd_loc = 0;
  dirp -> d_dd_contents = NULL;

  if ( getdirent(dirp -> d_dd_name) )
    do
    {
      if ( !savedirent(dirp) )
      {
	free_dircontents(dirp -> d_dd_contents);
	dirp -> d_dd_contents = NULL;
	errno = ENOMEM;
	break;
      }
    }
    while ( getdirent(NULL) );

  dirp -> d_dd_cp = dirp -> d_dd_contents;
}


static void free_dircontents(struct d_dircontents *dp)
{
  struct d_dircontents *odp;

  while (dp)
  {
    if (dp -> d_dc_entry)
      free(dp -> d_dc_entry);

    odp = dp;
    dp = odp -> d_dc_next;
    free(odp);
  }
}


int savedirent(DIR *dirp)
{
  struct d_dircontents *dp;

  if ( (dp = malloc(sizeof(struct d_dircontents))) == NULL ||
       (dp -> d_dc_entry = malloc(strlen(find.achName) + 1)) == NULL )
  {
    if ( dp )
      free(dp);
    return FALSE;
  }

  if ( dirp -> d_dd_contents )
  {
    dirp -> d_dd_cp -> d_dc_next = dp;
    dirp -> d_dd_cp = dirp -> d_dd_cp -> d_dc_next;
  }
  else
    dirp -> d_dd_contents = dirp -> d_dd_cp = dp;

  dp -> d_dc_next = NULL;
  strcpy(dp -> d_dc_entry, find.achName);

#ifdef __EMX__
  dp -> d_dc_size = ((unsigned long) find.size_hi << 16) + find.size_lo;
  dp -> d_dc_mode = find.attr;
  dp -> d_dc_time = *(unsigned *) &(find.time);
  dp -> d_dc_date = *(unsigned *) &(find.date);
#else
#ifdef OS2
  dp -> d_dc_size = find.cbFile;
  dp -> d_dc_mode = find.attrFile;
  dp -> d_dc_time = *(unsigned *) &(find.ftimeLastWrite);
  dp -> d_dc_date = *(unsigned *) &(find.fdateLastWrite);
#else
  dp -> d_dc_size = find.size;
  dp -> d_dc_mode = find.attrib;
  dp -> d_dc_time = find.wr_time;
  dp -> d_dc_date = find.wr_date;
#endif
#endif

  return TRUE;
}


int getdirent(char *dir)
{
  int done;
  static int lower = TRUE;

  if (dir != NULL)
  {				       /* get first entry */
    lower = IsFileSystemFAT(dir);

#ifndef __EMX__
#ifdef OS2
    hdir = HDIR_CREATE;
    count = 1;
#endif
#endif
    done = DosFindFirst(dir, &hdir, attributes, &find, sizeof(find), &count);
  }
  else				       /* get next entry */
    done = DosFindNext(hdir, &find, sizeof(find), &count);

  if (done == 0)
  {
    if ( lower )
      strlwr(find.achName);
    return TRUE;
  }
  else
  {
    DosFindClose(hdir);
    return FALSE;
  }
}


int IsFileSystemFAT(char *dir)
{
#ifdef OS2
  static USHORT nLastDrive = -1, nResult;
  ULONG lMap;
  BYTE bData[64], bName[3];
#ifdef __32BIT__
  ULONG nDrive, cbData;
  FSQBUFFER2 *pData = (FSQBUFFER2 *) bData;
#else
  USHORT nDrive, cbData;
  FSQBUFFER *pData = (FSQBUFFER *) bData;
#endif

  if ( _osmode == DOS_MODE )
    return TRUE;

  /* We separate FAT and HPFS file systems here. */

  if ( isalpha(dir[0]) && (dir[1] == ':') )
    nDrive = toupper(dir[0]) - '@';
  else
    DosQCurDisk(&nDrive, &lMap);

  if ( nDrive == nLastDrive )
    return nResult;

  bName[0] = (char) (nDrive + '@');
  bName[1] = ':';
  bName[2] = 0;

  nLastDrive = nDrive;
  cbData = sizeof(bData);

    if ( !DosQFSAttach(bName, 0, FSAIL_QUERYNAME, (PVOID) pData, &cbData) )
      nResult = !strcmp(pData -> szFSDName + pData -> cbName, "FAT");
    else
      nResult = FALSE;

  return nResult;  
#else
  return TRUE;
#endif
}


int IsFileNameValid(char *name)
{
  HFILE hf;
#ifdef __32BIT__
  ULONG uAction;
#else
  USHORT uAction;
#endif

  if ( _osmode == DOS_MODE )
    return FALSE;

  switch( DosOpen(name, &hf, &uAction, 0L, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0L) )
  {
  case ERROR_INVALID_NAME:
  case ERROR_FILENAME_EXCED_RANGE:
    return FALSE;
  case NO_ERROR:
    DosClose(hf);
  default:
    return TRUE;
  }
}


#ifdef TEST
int main(void)
{
  DIR *dirp = opendir(".");
  struct dirent *dp;

  while ( (dp = readdir(dirp)) != NULL )
    printf(">%s<\n", dp -> d_name);

  return 0;
}
#endif
