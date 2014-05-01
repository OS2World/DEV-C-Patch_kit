/* @(#) dirent.h
 * public domain implementation of POSIX directory routines for DOS and OS/2
 */


#define _POSIX_NAME_MAX  256
#define _POSIX_PATH_MAX  260


struct dirent
{
  ino_t    d_ino;                         /* a bit of a farce */
  int      d_reclen;                      /* more farce */
  int      d_namlen;                      /* length of d_name */
  char     d_name[_POSIX_NAME_MAX + 1];   /* null terminated */
  /* nonstandard fields */
  long     d_size;                        /* size in bytes */
  unsigned d_mode;                        /* file attributes */
  unsigned d_time;                        /* file time */
  unsigned d_date;                        /* file date */
};

/* The fields d_size, d_mode, d_time and d_date are extensions.
 * The find_first and find_next calls deliver this data without
 * any extra cost. If this data is needed, these fields save a lot
 * of extra calls to stat() (each stat() again does a find_first!).
 */

struct d_dircontents
{
  char *d_dc_entry;
  long  d_dc_size;
  unsigned d_dc_mode;
  unsigned d_dc_time;
  unsigned d_dc_date;
  struct d_dircontents *d_dc_next;
};

struct d_dirdesc
{
  int   d_dd_id;           /* uniquely identify each open directory */
  long  d_dd_loc;          /* where we are in directory entry is this */
  struct d_dircontents *d_dd_contents; /* pointer to contents of dir */
  struct d_dircontents *d_dd_cp;       /* pointer to current position */
  char d_dd_name[_POSIX_PATH_MAX];     /* remember name for rewinddir */
};

typedef struct d_dirdesc DIR;


extern DIR *opendir(char *);
extern struct dirent *readdir(DIR *);
extern void rewinddir(DIR *);
extern int closedir(DIR *);

#ifndef _POSIX_SOURCE

extern void seekdir(DIR *, long);
extern long telldir(DIR *);

#define _A_RONLY    0x01
#define _A_HIDDEN   0x02
#define _A_SYSTEM   0x04
#define _A_LABEL    0x08
#define _A_DIR      0x10
#define _A_ARCHIVE  0x20

extern int attributes;

extern int scandir(char *, struct dirent ***,
                   int (*)(struct dirent *),
                   int (*)(struct dirent *, struct dirent *));

extern int getfmode(char *);
extern int setfmode(char *, unsigned);

extern int IsFileSystemFAT(char *dir);

#endif

/*
NAME
     opendir, readdir, telldir, seekdir, rewinddir, closedir -
     directory operations

SYNTAX
     #include <sys/types.h>
     #include <sys/dir.h>

     DIR *opendir(filename)
     char *filename;

     struct direct *readdir(dirp)
     DIR *dirp;

     long telldir(dirp)
     DIR *dirp;

     seekdir(dirp, loc)
     DIR *dirp;
     long loc;

     rewinddir(dirp)
     DIR *dirp;

     int closedir(dirp)
     DIR *dirp;

DESCRIPTION
     The opendir library routine opens the directory named by
     filename and associates a directory stream with it.  A
     pointer is returned to identify the directory stream in sub-
     sequent operations.  The pointer NULL is returned if the
     specified filename can not be accessed, or if insufficient
     memory is available to open the directory file.

     The readdir routine returns a pointer to the next directory
     entry.  It returns NULL upon reaching the end of the direc-
     tory or on detecting an invalid seekdir operation.  The
     readdir routine uses the getdirentries system call to read
     directories. Since the readdir routine returns NULL upon
     reaching the end of the directory or on detecting an error,
     an application which wishes to detect the difference must
     set errno to 0 prior to calling readdir.

     The telldir routine returns the current location associated
     with the named directory stream. Values returned by telldir
     are good only for the lifetime of the DIR pointer from which
     they are derived.  If the directory is closed and then reo-
     pened, the telldir value may be invalidated due to
     undetected directory compaction.

     The seekdir routine sets the position of the next readdir
     operation on the directory stream. Only values returned by
     telldir should be used with seekdir.

     The rewinddir routine resets the position of the named
     directory stream to the beginning of the directory.

     The closedir routine closes the named directory stream and
     returns a value of 0 if successful. Otherwise, a value of -1
     is returned and errno is set to indicate the error.  All
     resources associated with this directory stream are
     released.

EXAMPLE
     The following sample code searches a directory for the entry
     name.

     len = strlen(name);

     dirp = opendir(".");

     for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))

     if (dp->d_namlen == len && !strcmp(dp->d_name, name)) {

               closedir(dirp);

               return FOUND;

          }

     closedir(dirp);

     return NOT_FOUND;


SEE ALSO
     close(2), getdirentries(2), lseek(2), open(2), read(2),
     dir(5)

-----------------------

NAME
     scandir - scan a directory

SYNTAX
     #include <sys/types.h>
     #include <sys/dir.h>

     scandir(dirname, namelist, select, compar)
     char *dirname;
     struct direct *(*namelist[]);
     int (*select)();
     int (*compar)();

     alphasort(d1, d2)
     struct direct **d1, **d2;

DESCRIPTION
     The scandir subroutine reads the directory dirname and
     builds an array of pointers to directory entries using mal-
     loc(3).  It returns the number of entries in the array and a
     pointer to the array through namelist.

     The select parameter is a pointer to a user supplied subrou-
     tine which is called by scandir to select which entries are
     to be included in the array.  The select routine is passed a
     pointer to a directory entry and should return a non-zero
     value if the directory entry is to be included in the array.
     If select is null, then all the directory entries will be
     included.

     The compar parameter is a pointer to a user supplied subrou-
     tine which is passed to qsort(3) to sort the completed
     array.  If this pointer is null, the array is not sorted.
     The alphasort is a routine which can be used for the compar
     parameter to sort the array alphabetically.

     The memory allocated for the array can be deallocated with
     free by freeing each pointer in the array and the array
     itself.  For further information, see malloc(3).

DIAGNOSTICS
     Returns -1 if the directory cannot be opened for reading or
     if malloc(3) cannot allocate enough memory to hold all the
     data structures.

SEE ALSO
     directory(3), malloc(3), qsort(3), dir(5)
*/
