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
#include "hash.h"

// C standard library
#include <stdlib.h>

// System library
//// POSIX
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

// tree modules
#include "tree.h"
#include "xstdlib.h"

#define HASH(x) ((x)&255)
#define inohash(x) ((x)&255)

/* Faster uid/gid -> name lookup with hash(tm)(r)(c) tables! */
static struct xtable *gtable[256] = {NULL};
static struct xtable *utable[256] = {NULL};
static struct inotable *itable[256] = {NULL};

static void free_xtable(size_t size, struct xtable **table);
static void free_inotable(size_t size, struct inotable **table);

static void free_xtable(size_t size, struct xtable **table) {
  for (size_t i = 0; i < size; i++) {
    struct xtable *next;
    for (struct xtable *base = table[i]; base != NULL; base = next) {
      next = base->nxt;
      free(base);
    }
  }
  memset(table, 0, size * (sizeof *table));
}

static void free_inotable(size_t size, struct inotable **table) {
  for (size_t i = 0; i < size; i++) {
    struct inotable *next;
    for (struct inotable *base = table[i]; base != NULL; base = next) {
      next = base->nxt;
      free(base);
    }
  }
  memset(table, 0, size * (sizeof *table));
}

char *uidtoname(uid_t uid) {
  struct xtable *o, *p;
  int uent = HASH(uid);
  o = utable[uent];
  for (p = utable[uent]; p != NULL; p = p->nxt) {
    if (uid == p->xid) {
      return p->name;
    } else if (uid < p->xid) {
      break;
    } else {
      o = p;
    }
  }

  /* Not found, do a real lookup and add to table */
  struct xtable *t = xmalloc(sizeof *t);
  struct passwd *ent = getpwuid(uid);
  if (ent != NULL) {
    t->name = scopy(ent->pw_name);
  } else {
    char ubuf[32];
    snprintf(ubuf, 30, "%d", uid);
    ubuf[31] = 0;
    t->name = scopy(ubuf);
  }
  t->xid = uid;
  t->nxt = p;
  if (p == utable[uent]) {
    utable[uent] = t;
  } else {
    o->nxt = t;
  }

  return t->name;
}

char *gidtoname(gid_t gid) {
  struct xtable *o, *p;
  int gent = HASH(gid);
  o = gtable[gent];
  for (p = gtable[gent]; p != NULL; p = p->nxt) {
    if (gid == p->xid) {
      return p->name;
    } else if (gid < p->xid) {
      break;
    } else {
      o = p;
    }
  }

  /* Not found, do a real lookup and add to table */
  struct xtable *t = xmalloc(sizeof *t);
  struct group *ent = getgrgid(gid);
  if (ent != NULL) {
    t->name = scopy(ent->gr_name);
  } else {
    char gbuf[32];
    snprintf(gbuf, 30, "%d", gid);
    gbuf[31] = 0;
    t->name = scopy(gbuf);
  }
  t->xid = gid;
  t->nxt = p;
  if (p == gtable[gent]) {
    gtable[gent] = t;
  } else {
    o->nxt = t;
  }

  return t->name;
}

bool findino(ino_t inode, dev_t device) {
  struct inotable *it;

  for (it = itable[inohash(inode)]; it != NULL; it = it->nxt) {
    if (it->inode > inode) {
      break;
    }
    if ((it->inode == inode) && (it->device >= device)) {
      break;
    }
  }

  return ((it != NULL) && (it->inode == inode) && (it->device == device));
}

/* Record inode numbers of followed sym-links to avoid refollowing them */
void saveino(ino_t inode, dev_t device) {
  struct inotable *ip, *pp;
  int hp = inohash(inode);

  for (pp = ip = itable[hp]; ip; ip = ip->nxt) {
    if (ip->inode > inode) {
      break;
    }
    if ((ip->inode == inode) && (ip->device >= device)) {
      break;
    }
    pp = ip;
  }

  if ((ip != NULL) && (ip->inode == inode) && (ip->device == device)) {
    return;
  }

  struct inotable *it = xmalloc(sizeof *it);
  it->inode = inode;
  it->device = device;
  it->nxt = ip;
  if (ip == itable[hp]) {
    itable[hp] = it;
  } else {
    pp->nxt = it;
  }
}

void free_tables(void) {
  free_xtable((sizeof gtable) / (sizeof gtable[0]), gtable);
  free_xtable((sizeof utable) / (sizeof utable[0]), utable);
  free_inotable((sizeof itable) / (sizeof itable[0]), itable);
}
