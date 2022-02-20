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

#ifndef HTML_H
#define HTML_H

#include "bool.h"
#include "list.h"

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

extern int htmldirlen;

void html_intro(void);
void html_outtro(void);
int html_printinfo(char *dirname, struct _info *file, int level);
int html_printfile(char *dirname, char *filename, struct _info *file, int descend);
int html_error(char *error);
void html_newline(struct _info *file, int level, int postdir, int needcomma);
void html_close(struct _info *file, int level, int needcomma);
void html_report(struct totals tot);
void html_encode(FILE *fd, char *s);

#endif // HTML_H
