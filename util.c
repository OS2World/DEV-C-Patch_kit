#include "EXTERN.h"
#include "common.h"
#include "INTERN.h"
#include "util.h"
#include "backupfile.h"

/* Rename a file, copying it if necessary. */

int
move_file(from,to)
char *from, *to;
{
    char bakname[512];
    Reg1 char *s;
    Reg2 int i;
    Reg3 int fromfd;

    /* to stdout? */

    if (strEQ(to, "-")) {
#ifdef DEBUGGING
	if (debug & 4)
	    say2("Moving %s to stdout.\n", from);
#endif
        fromfd = open(from, O_RDONLY|O_BINARY);
	if (fromfd < 0)
	    fatal2("patch: internal error, can't reopen %s\n", from);
	while ((i=read(fromfd, buf, sizeof buf)) > 0)
	    if (write(1, buf, i) != 1)
		fatal1("patch: write failed\n");
	Close(fromfd);
	return 0;
    }

    if (origprae) {
	Strcpy(bakname, origprae);
	Strcat(bakname, to);
    } else {
#ifndef NODIR
	char *backupname = find_backup_file_name(to);
	if (backupname == (char *) 0)
	    fatal1("Can't seem to get enough memory.\n");
	Strcpy(bakname, backupname);
	free(backupname);
#else /* NODIR */
	Strcpy(bakname, to);
    	Strcat(bakname, simple_backup_suffix);
#endif /* NODIR */
#ifdef OS2
        if ( !IsFileNameValid(bakname) )
        {
	  Strcpy(bakname, to);

          if ((s=strrchr(bakname,'.'))!=NULL)
            *s=0;

          strcat(bakname, ".org");
        }
#endif
    }

    if (stat(to, &filestat) >= 0) {	/* output file exists */
	dev_t to_device = filestat.st_dev;
	ino_t to_inode  = filestat.st_ino;
	char *simplename = bakname;

	for (s=bakname; *s; s++) {
#ifdef OS2
            if (*s == '/' || *s == '\\')
#else
	    if (*s == '/')
#endif
		simplename = s+1;
	}
	/* Find a backup name that is not the same file.
	   Change the first lowercase char into uppercase;
	   if that isn't sufficient, chop off the first char and try again.  */
	while (stat(bakname, &filestat) >= 0 &&
		to_device == filestat.st_dev && to_inode == filestat.st_ino) {
	    /* Skip initial non-lowercase chars.  */
	    for (s=simplename; *s && !islower(*s); s++) ;
	    if (*s)
		*s = toupper(*s);
	    else
		Strcpy(simplename, simplename+1);
	}
	while (unlink(bakname) >= 0) ;	/* while() is for benefit of Eunice */
#ifdef DEBUGGING
	if (debug & 4)
	    say3("Moving %s to %s.\n", to, bakname);
#endif

#ifdef OS2
        if (rename(to, bakname) < 0) {
#else
	if (link(to, bakname) < 0) {
#endif
	    say3("patch: can't backup %s, output is in %s\n",
		to, from);
	    return -1;
	}

#ifndef OS2
	while (unlink(to) >= 0) ;
#endif
    }

#ifdef DEBUGGING
    if (debug & 4)
	say3("Moving %s to %s.\n", from, to);
#endif

#ifdef OS2
    if (rename(from, to) < 0) {           /* different file system? */
#else
    if (link(from, to) < 0) {		/* different file system? */
#endif
	Reg4 int tofd;

        tofd = open(to, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IWRITE|S_IREAD);
	if (tofd < 0) {
	    say3("patch: can't create %s, output is in %s.\n",
	      to, from);
	    return -1;
	}

        fromfd = open(from, O_RDONLY|O_BINARY);
	if (fromfd < 0)
	    fatal2("patch: internal error, can't reopen %s\n", from);
	while ((i=read(fromfd, buf, sizeof buf)) > 0)
	    if (write(tofd, buf, i) != i)
		fatal1("patch: write failed\n");
	Close(fromfd);
	Close(tofd);
#ifdef OS2
    Unlink(from);
    }
#else
    }
    Unlink(from);
#endif
    return 0;
}

/* Copy a file. */

void
copy_file(from,to)
char *from, *to;
{
    Reg3 int tofd;
    Reg2 int fromfd;
    Reg1 int i;

    tofd = open(to, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,S_IWRITE|S_IREAD);
    if (tofd < 0)
	fatal2("patch: can't create %s.\n", to);
    fromfd = open(from, O_RDONLY|O_BINARY);
    if (fromfd < 0)
	fatal2("patch: internal error, can't reopen %s\n", from);
    while ((i=read(fromfd, buf, sizeof buf)) > 0)
	if (write(tofd, buf, i) != i)
	    fatal2("patch: write (%s) failed\n", to);
    Close(fromfd);
    Close(tofd);
}

/* Allocate a unique area for a string. */

char *
savestr(s)
Reg1 char *s;
{
    Reg3 char *rv;
    Reg2 char *t;

    if (!s)
	s = "Oops";
    t = s;
    while (*t++);
    rv = malloc((MEM) (t - s));
    if (rv == Nullch) {
	if (using_plan_a)
	    out_of_mem = TRUE;
	else
	    fatal1("patch: out of memory (savestr)\n");
    }
    else {
	t = rv;
	while (*t++ = *s++);
    }
    return rv;
}

#if defined(lint) && defined(CANVARARG)

/*VARARGS ARGSUSED*/
say(pat) char *pat; { ; }
/*VARARGS ARGSUSED*/
fatal(pat) char *pat; { ; }
/*VARARGS ARGSUSED*/
ask(pat) char *pat; { ; }

#else

/* Vanilla terminal output (buffered). */

#ifdef OS2
#include <stdarg.h>

void say(char *pat,...)
{
    va_list argptr;

    va_start(argptr, pat);
    vfprintf(stderr, pat, argptr);
    va_end(argptr);
    Fflush(stderr);
}

void fatal(char *pat,...)
{
    void my_exit();
    va_list argptr;

    va_start(argptr, pat);
    vfprintf(stderr, pat, argptr);
    va_end(argptr);
    Fflush(stderr);
    my_exit(1);
}

#else
void
say(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    fprintf(stderr, pat, arg1, arg2, arg3);
    Fflush(stderr);
}

/* Terminal output, pun intended. */

void				/* very void */
fatal(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    void my_exit();

    say(pat, arg1, arg2, arg3);
    my_exit(1);
}
#endif

/* Get a response from the user, somehow or other. */

#ifdef OS2
void ask(char *pat,...)
{
    int ttyfd;
    int r;
    bool tty2 = isatty(2);
    va_list argptr;

    va_start(argptr, pat);
    vsprintf(buf, pat, argptr);
    va_end(argptr);
#else
void
ask(pat,arg1,arg2,arg3)
char *pat;
long arg1,arg2,arg3;
{
    int ttyfd;
    int r;
    bool tty2 = isatty(2);

    Sprintf(buf, pat, arg1, arg2, arg3);
#endif
    Fflush(stderr);
    write(2, buf, strlen(buf));
    if (tty2) {				/* might be redirected to a file */
	r = read(2, buf, sizeof buf);
    }
    else if (isatty(1)) {		/* this may be new file output */
	Fflush(stdout);
	write(1, buf, strlen(buf));
	r = read(1, buf, sizeof buf);
    }
#ifdef OS2
    else if ((ttyfd = open("con", O_RDWR)) >= 0 && isatty(ttyfd)) {
#else
    else if ((ttyfd = open("/dev/tty", O_RDWR)) >= 0 && isatty(ttyfd)) {
#endif
					/* might be deleted or unwriteable */
	write(ttyfd, buf, strlen(buf));
	r = read(ttyfd, buf, sizeof buf);
	Close(ttyfd);
    }
    else if (isatty(0)) {		/* this is probably patch input */
	Fflush(stdin);
	write(0, buf, strlen(buf));
	r = read(0, buf, sizeof buf);
    }
    else {				/* no terminal at all--default it */
	buf[0] = '\n';
	r = 1;
    }
    if (r <= 0)
	buf[0] = 0;
    else
	buf[r] = '\0';
    if (!tty2)
	say1(buf);
}
#endif /* lint */

/* How to handle certain events when not in a critical region. */

void
set_signals(reset)
int reset;
{
    void my_exit();
#ifndef lint
#ifdef VOIDSIG
    static void (*hupval)(),(*intval)();
#else
    static int (*hupval)(),(*intval)();
#endif

    if (!reset) {
#ifndef OS2
	hupval = signal(SIGHUP, SIG_IGN);
	if (hupval != SIG_IGN)
#ifdef VOIDSIG
	    hupval = my_exit;
#else
	    hupval = (int(*)())my_exit;
#endif
#endif
	intval = signal(SIGINT, SIG_IGN);
	if (intval != SIG_IGN)
#ifdef VOIDSIG
	    intval = my_exit;
#else
	    intval = (int(*)())my_exit;
#endif
    }
#ifndef OS2
    Signal(SIGHUP, hupval);
#endif
    Signal(SIGINT, intval);
#endif
}

/* How to handle certain events when in a critical region. */

void
ignore_signals()
{
#ifndef lint
#ifndef OS2
    Signal(SIGHUP, SIG_IGN);
#endif
    Signal(SIGINT, SIG_IGN);
#endif
}

/* Make sure we'll have the directories to create a file. */

void
makedirs(filename,striplast)
Reg1 char *filename;
bool striplast;
{
    char tmpbuf[256];
    Reg2 char *s = tmpbuf;
    char *dirv[20];
    Reg3 int i;
    Reg4 int dirvp = 0;

    while (*filename) {
#ifdef OS2
        if (*filename == '/' || *filename == '\\') {
#else
	if (*filename == '/') {
#endif
	    filename++;
	    dirv[dirvp++] = s;
	    *s++ = '\0';
	}
	else {
	    *s++ = *filename++;
	}
    }
    *s = '\0';
    dirv[dirvp] = s;
    if (striplast)
	dirvp--;
    if (dirvp < 0)
	return;
    strcpy(buf, "mkdir");
    s = buf;
    for (i=0; i<=dirvp; i++) {
	while (*s) s++;
	*s++ = ' ';
	strcpy(s, tmpbuf);
	*dirv[i] = '/';
    }
    system(buf);
}

/* Make filenames more reasonable. */

char *
fetchname(at,strip_leading,assume_exists)
char *at;
int strip_leading;
int assume_exists;
{
    char *s;
    char *name;
    Reg1 char *t;
    char tmpbuf[200];

    if (!at)
	return Nullch;
    while (isspace(*at)) at++;
#ifdef DEBUGGING
    if (debug & 128)
	say4("fetchname %s %d %d\n",at,strip_leading,assume_exists);
#endif
    if (strnEQ(at, "/dev/null", 9) && (!at[9] || isspace(at[9])))
					/* so files can be created by diffing */
	return Nullch;			/*   against /dev/null. */
    name = s = t = savestr(at);
    for (; *t && !isspace(*t); t++)
#ifdef OS2
        if (*t == '/' || *t == '\\')
#else
	if (*t == '/')
#endif
	    if (--strip_leading >= 0)
		name = t+1;
    *t = '\0';
    name = savestr(name);
    free(s);
    if (stat(name, &filestat) < 0 && !assume_exists) {
	Sprintf(tmpbuf, "RCS/%s%s", name, RCSSUFFIX);
	if (stat(tmpbuf, &filestat) < 0 && stat(tmpbuf+4, &filestat) < 0) {
	    Sprintf(tmpbuf, "SCCS/%s%s", SCCSPREFIX, name);
	    if (stat(tmpbuf, &filestat) < 0 && stat(tmpbuf+5, &filestat) < 0) {
		free(name);
		name = Nullch;
	    }
	}
    }
    return name;
}


#ifdef OS2
#ifndef __EMX__

/* only one pipe can be open at a time */

/* In fact, this is a pipe simulation for the dumb MS-DOS */
/* but it works good enough for the situation where it is */
/* needed in patch, even if OS/2 has real pipes. */

static char pipename[128], command[128];
static int wrpipe;

FILE *popen(char *cmd, char *flags)
{
  wrpipe = (strchr(flags, 'w') != NULL);

  if ( wrpipe )
  {
    strcpy(command, cmd);
    strcpy(pipename, "~WXXXXXX");
    Mktemp(pipename);
    return fopen(pipename, flags);  /* ordinary file */
  }
  else
  {
    strcpy(pipename, "~RXXXXXX");
    Mktemp(pipename);
    strcpy(command, cmd);
    strcat(command, ">");
    strcat(command, pipename);
    system(command);
    return fopen(pipename, flags);  /* ordinary file */
  }
}

int pclose(FILE *pipe)
{
  int rc;

  if ( fclose(pipe) == EOF )
    return EOF;

  if ( wrpipe )
  {
    strcat(command, "<");
    strcat(command, pipename);
    rc = system(command);
    unlink(pipename);
    return rc;
  }
  else
  {
    unlink(pipename);
    return 0;
  }
}

#endif
#endif
