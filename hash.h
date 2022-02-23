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

// System library
//// POSIX
#include <sys/types.h>

struct inotable {
  ino_t inode;
  dev_t device;
  struct inotable *nxt;
};

struct xtable {
  unsigned int xid;
  char *name;
  struct xtable *nxt;
};

extern struct xtable *gtable[256], *utable[256];
extern struct inotable *itable[256];

char *uidtoname(uid_t uid);
char *gidtoname(gid_t gid);
int findino(ino_t inode, dev_t device);
void saveino(ino_t inode, dev_t device);

#endif // HASH_H
