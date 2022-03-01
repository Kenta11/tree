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
#include "file.h"

// C standard library
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

// System library
//// POSIX
#include <sys/stat.h>

// tree modules
#include "tree.h"
#include "xstdlib.h"

enum ftok { T_PATHSEP, T_DIR, T_FILE, T_EOP };

static char *nextpc(char **p, int *tok) {
  static char prev = 0;
  char *s = *p;
  if (**p == 0) {
    *tok = T_EOP; // Shouldn't happen.
    return NULL;
  }
  if (prev != 0) {
    prev = 0;
    *tok = T_PATHSEP;
    return NULL;
  }
  if (strchr(file_pathsep, **p) != NULL) {
    (*p)++;
    *tok = T_PATHSEP;
    return NULL;
  }
  while ((**p != 0) && (strchr(file_pathsep, **p) == NULL)) {
    (*p)++;
  }

  if (**p != 0) {
    *tok = T_DIR;
    prev = **p;
    *(*p)++ = '\0';
  } else {
    *tok = T_FILE;
  }
  return s;
}

static struct _info *newent(char *name) {
  struct _info *n = xmalloc(sizeof *n);
  memset(n, 0, sizeof(struct _info));
  n->name = scopy(name);
  n->child = NULL;
  n->tchild = n->next = NULL;
  return n;
}

// Should replace this with a Red-Black tree implementation or the like
static struct _info *search(struct _info **dir, char *name) {
  if (*dir == NULL) {
    *dir = newent(name);
    return *dir;
  }

  struct _info *prev = *dir;
  struct _info *ptr;
  for (ptr = *dir; ptr != NULL; ptr = ptr->next) {
    int cmp = strcmp(ptr->name, name);
    if (cmp == 0) {
      return ptr;
    } else if (cmp > 0) {
      break;
    } else {
      prev = ptr;
    }
  }

  struct _info *n = newent(name);
  n->next = ptr;
  if (prev == ptr) {
    *dir = n;
  } else {
    prev->next = n;
  }
  return n;
}

static void freefiletree(struct _info *entries) {
  for (struct _info *entry = entries; entry != NULL;) {
    if (entry->tchild != NULL) {
      freefiletree(entry->tchild);
    }
    struct _info *tmp = entry;
    entry = entry->next;
    free(tmp);
  }
}

/**
 * Recursively prune (unset show flag) files/directories of matches/ignored
 * patterns:
 */
static struct _info **fprune(struct _info *head, bool matched, bool root) {
  struct _info *new = NULL, *end = NULL;
  int count = 0;

  for (struct _info *entry = head; entry != NULL;) {
    if (entry->tchild) {
      entry->isdir = 1;
    }

    bool show = true;
    if (dflag && !(entry->isdir)) {
      show = false;
    }
    if ((!aflag) && (!root) && (entry->name[0] == '.')) {
      show = false;
    }
    if (show && (!matched)) {
      if (!entry->isdir) {
        if ((pattern != 0) && (patinclude(entry->name, 0) == 0)) {
          show = false;
        }
        if ((ipattern != 0) && (patignore(entry->name, 0) != 0)) {
          show = false;
        }
      }
      if (entry->isdir && show && matchdirs && (pattern != 0)) {
        if (patinclude(entry->name, 1) != 0) {
          matched = true;
        }
      }
    }
    if (pruneflag && (!matched) && entry->isdir && (entry->tchild == NULL)) {
      show = false;
    }
    if (show && (entry->tchild != NULL)) {
      entry->child = fprune(entry->tchild, matched, false);
    }

    struct _info *t = entry->next;
    if (show) {
      if (end != NULL) {
        end = end->next = t;
      } else {
        new = end = t;
      }
      count++;
    } else {
      t->next = NULL;
      freefiletree(t);
    }
  }
  if (end != NULL) {
    end->next = NULL;
  }

  struct _info **dir = xmalloc((sizeof *dir) * (count + 1));
  count = 0;
  for (struct _info *entry = new; entry != NULL; entry = entry->next) {
    dir[count++] = entry;
  }
  dir[count] = NULL;

  if (topsort != NULL) {
    qsort(dir, count, sizeof(struct _info *),
          (int (*)(const void *, const void *))topsort);
  }

  return dir;
}

struct _info **file_getfulltree(char *d, unsigned long lev, dev_t dev,
                                off_t *size, char **err) {
  struct _info *root = NULL;

  (void)lev;
  (void)dev;
  (void)size;
  (void)err;

  FILE *fp = (strcmp(d, ".") ? fopen(d, "r") : stdin);
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s for reading.\n", d);
    return NULL;
  }
  // 64K paths maximum
  int pathsize = 64 * 1024;
  char *path = xmalloc((sizeof *path) * pathsize);

  while (fgets(path, pathsize, fp) != NULL) {
    if ((file_comment != NULL) && (strcmp(path, file_comment) == 0)) {
      continue;
    }

    for (size_t l = strlen(path); (l > 0) && isspace(path[l - 1]); l--) {
      path[l - 1] = '\0';
    }
    if (path[0] == '\0') {
      continue;
    }

    char *spath = path;
    int tok;
    struct _info **cwd = &root;
    do {
      char *s = nextpc(&spath, &tok);

      switch (tok) {
      case T_FILE:
      case T_DIR: {
        // Should probably handle '.' and '..' entries here
        struct _info *entry = search(cwd, s);
        // Might be empty, but should definitely be considered a directory:
        if (tok == T_DIR) {
          entry->isdir = 1;
          entry->mode = S_IFDIR;
        } else {
          entry->mode = S_IFREG;
        }
        cwd = &(entry->tchild);
        break;
      }
      default: {
        break;
      }
      }
    } while ((tok != T_FILE) && (tok != T_EOP));
  }

  if (fp != stdin) {
    fclose(fp);
  }

  // Prune accumulated directory tree:
  return fprune(root, false, true);
}
