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

#ifndef HASH_H
#define HASH_H

#include "hash.h"

#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef __EMX__ /* for OS/2 systems */
#define INCL_DOSFILEMGR
#define INCL_DOSNLS
#include <io.h>
#include <os2.h>
#include <sys/nls.h>
/* On many systems stat() function is idential to lstat() function.
 * But the OS/2 does not support symbolic links and doesn't have lstat()
 * function.
 */
#define lstat stat
#define strcasecmp stricmp
/* Following two functions, getcwd() and chdir() don't support for drive
 * letters. To implement support them, use _getcwd2() and _chdir2().
 */
#define getcwd _getcwd2
#define chdir _chdir2
#endif

#include <langinfo.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#ifdef __ANDROID
#define mbstowcs(w, m, x) mbsrtowcs(w, (const char **)(&#m), x, NULL)
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
#define ENV_STDDATA_FD "STDDATA_FD"
#ifndef STDDATA_FILENO
#define STDDATA_FILENO 3
#endif
#endif

/* Should probably use strdup(), but we like our xmalloc() */
#define scopy(x) strcpy(xmalloc(strlen(x) + 1), (x))
#define MINIT 30 /* number of dir entries to initially allocate */
#define MINC 20  /* allocation increment */

struct xtable {
  unsigned int xid;
  char *name;
  struct xtable *nxt;
};
struct inotable {
  ino_t inode;
  dev_t device;
  struct inotable *nxt;
};

extern struct inotable *itable[256];

extern struct xtable *gtable[256], *utable[256];

char *uidtoname(uid_t uid);
char *gidtoname(gid_t gid);
int findino(ino_t, dev_t);
void saveino(ino_t, dev_t);

#endif // HASH_H
