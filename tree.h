/* $Copyright: $
 * Copyright (c) 1996 - 2022 by Steve Baker (ice@mama.indstate.edu)
 * All Rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef TREE_H
#define TREE_H

#include "bool.h"
#include "info.h"
#include "filter.h"

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#ifdef __EMX__  /* for OS/2 systems */
#  define INCL_DOSFILEMGR
#  define INCL_DOSNLS
#  include <os2.h>
#  include <sys/nls.h>
#  include <io.h>
  /* On many systems stat() function is idential to lstat() function.
   * But the OS/2 does not support symbolic links and doesn't have lstat() function.
   */
#  define         lstat          stat
#  define         strcasecmp     stricmp
  /* Following two functions, getcwd() and chdir() don't support for drive letters.
   * To implement support them, use _getcwd2() and _chdir2().
   */
#  define getcwd _getcwd2
#  define chdir _chdir2
#endif

#include <locale.h>
#include <langinfo.h>
#include <wchar.h>
#include <wctype.h>

#ifdef __ANDROID
#define mbstowcs(w,m,x) mbsrtowcs(w,(const char**)(& #m),x,NULL)
#endif

// Start using PATH_MAX instead of the magic number 4096 everywhere.
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef INFO_PATH
#define INFO_PATH "/usr/share/finfo/global_info"
#endif

#ifdef __linux__
#include <fcntl.h>
# define ENV_STDDATA_FD  "STDDATA_FD"
# ifndef STDDATA_FILENO
#  define STDDATA_FILENO 3
# endif
#endif

/* Should probably use strdup(), but we like our xmalloc() */
#define scopy(x)	strcpy(xmalloc(strlen(x)+1),(x))
#define MINIT		30	/* number of dir entries to initially allocate */
#define MINC		20	/* allocation increment */

extern char *version;
extern char *hversion;

extern bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
extern bool Dflag, inodeflag, devflag;
extern bool hflag, Rflag;
extern bool Hflag, siflag, cflag, duflag, pruneflag;
extern bool noindent, xdev, force_color, noreport, nocolor, nolinks, flimit;
extern bool matchdirs, metafirst;

extern struct listingcalls lc;

extern int pattern, ipattern;

extern char *host, *title, *sp, *_nl;
extern char *file_comment, *file_pathsep;
extern const char *charset;

extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err);
extern int (*topsort)();

extern FILE *outfile;
extern int Level, *dirs;
extern int errors;

#ifdef __EMX__
extern const u_short ifmt[];
#else
  #ifdef S_IFPORT
  extern const u_int ifmt[];
  extern const char *ftype[];
  #else
  extern const u_int ifmt[];
  extern const char *ftype[];
  #endif
#endif

void setoutput(char *filename);
void usage(int);
void push_files(char *dir, struct ignorefile **ig, struct infofile **inf);
int patignore(char *name, int isdir);
int patinclude(char *name, int isdir);
struct _info **unix_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err);
struct _info **read_dir(char *dir, int *n, int infotop);

int filesfirst(struct _info **, struct _info **);
int dirsfirst(struct _info **, struct _info **);
int alnumsort(struct _info **, struct _info **);
int versort(struct _info **a, struct _info **b);
int reversealnumsort(struct _info **, struct _info **);
int mtimesort(struct _info **, struct _info **);
int ctimesort(struct _info **, struct _info **);
int sizecmp(off_t a, off_t b);
int fsizesort(struct _info **a, struct _info **b);

void *xmalloc(size_t), *xrealloc(void *, size_t);
char *gnu_getcwd();
int patmatch(char *, char *, int);
void indent(int maxlevel);
void free_dir(struct _info **);
#ifdef __EMX__
char *prot(long);
#else
char *prot(mode_t);
#endif
char *do_date(time_t);
void printit(char *);
int psize(char *buf, off_t size);
char Ftype(mode_t mode);
struct _info *stat2info(struct stat *st);
char *fillinfo(char *buf, struct _info *ent);

#endif // TREE_H
