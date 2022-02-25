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

#ifndef INFO_H
#define INFO_H
// C standard library
#include <stdbool.h>

// System library
//// POSIX
#include <sys/types.h>

struct _info {
  char *name;
  char *lnk;
  bool isdir;
  bool issok;
  bool isfifo;
  bool isexe;
  bool orphan;
  mode_t mode, lnkmode;
  uid_t uid;
  gid_t gid;
  off_t size;
  time_t atime, ctime, mtime;
  dev_t dev, ldev;
  ino_t inode, linode;
#ifdef __EMX__
  long attr;
#endif
  char *err;
  const char *tag;
  char **comment;
  struct _info **child, *next, *tchild;
};

struct comment {
  struct pattern *pattern;
  char **desc;
  struct comment *next;
};

struct infofile {
  char *path;
  struct comment *comments;
  struct infofile *next;
};

struct infofile *new_infofile(char *path);
void push_infostack(struct infofile *inf);
struct infofile *pop_infostack(void);
struct comment *infocheck(char *path, char *name, int top, int isdir);
void printcomment(int line, int lines, char *s);

#endif // INFO_H
