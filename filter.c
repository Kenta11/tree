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
#include "filter.h"

// C standard library
#include <stdlib.h>

// tree modules
#include "path.h"
#include "tree.h"
#include "xstdlib.h"

static struct ignorefile *filterstack = NULL;

static char fpattern[PATH_MAX];

void gittrim(char *s) {
  int e = strnlen(s, PATH_MAX) - 1;

  if (s[e] == '\n') {
    e--;
  }

  for (int i = e; i > 0; i--) {
    if (s[i] != ' ') {
      break;
    }
    if (s[i - 1] != '\\') {
      e--;
    }
  }
  s[e + 1] = '\0';

  for (int i = e = 0; s[i] != '\0'; i++) {
    if (s[i] == '\\') {
      i++;
    }
    s[e++] = s[i];
  }
  s[e] = '\0';
}

struct pattern *new_pattern(char *pattern) {
  struct pattern *p = xmalloc(sizeof *p);
  p->pattern = scopy(pattern);
  p->next = NULL;
  return p;
}

struct ignorefile *new_ignorefile(char *path) {
  char buf[PATH_MAX];
  snprintf(buf, PATH_MAX, "%s/.gitignore", path);
  FILE *fp = fopen(buf, "r");
  if (fp == NULL) {
    return NULL;
  }

  struct pattern *remove = NULL, *remend;
  struct pattern *reverse = NULL, *revend;
  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') {
      continue;
    }
    int rev = (buf[0] == '!');
    gittrim(buf);
    if (strlen(buf) == 0) {
      continue;
    }
    struct pattern *p = new_pattern(buf + (rev ? 1 : 0));
    if (rev) {
      if (reverse == NULL) {
        reverse = revend = p;
      } else {
        revend->next = p;
        revend = p;
      }
    } else {
      if (remove == NULL) {
        remove = remend = p;
      } else {
        remend->next = p;
        remend = p;
      }
    }
  }
  fclose(fp);

  struct ignorefile *ig = xmalloc(sizeof *ig);
  ig->remove = remove;
  ig->reverse = reverse;
  ig->path = scopy(path);
  ig->next = NULL;

  return ig;
}

void push_filterstack(struct ignorefile *ig) {
  if (ig != NULL) {
    ig->next = filterstack;
    filterstack = ig;
  }
}

struct ignorefile *pop_filterstack(void) {
  struct ignorefile *ig = filterstack;
  filterstack = filterstack->next;

  for (struct pattern *p = ig->remove; p != NULL; p = p->next) {
    free(p->pattern);
  }
  for (struct pattern *p = ig->reverse; p != NULL; p = p->next) {
    free(p->pattern);
  }
  free(ig->path);
  free(ig);

  return NULL;
}

/**
 * true if remove filter matches and no reverse filter matches.
 */
bool filtercheck(char *path, int isdir) {
  for (struct ignorefile *ig = filterstack; ig != NULL; ig = ig->next) {
    int fpos = sprintf(fpattern, "%s/", ig->path);

    for (struct pattern *p = ig->remove; p != NULL; p = p->next) {
      if (patmatch(path, p->pattern, isdir) == 1) {
        goto second_half_of_filtercheck;
      }
      if (p->pattern[0] == '/') {
        continue;
      }
      sprintf(fpattern + fpos, "%s", p->pattern);
      if (patmatch(path, fpattern, isdir) == 1) {
        goto second_half_of_filtercheck;
      }
    }
  }
  return false;

second_half_of_filtercheck:
  for (struct ignorefile *ig = filterstack; ig != NULL; ig = ig->next) {
    int fpos = sprintf(fpattern, "%s/", ig->path);

    for (struct pattern *p = ig->reverse; p != NULL; p = p->next) {
      if (patmatch(path, p->pattern, isdir) == 1) {
        return false;
      }

      if (p->pattern[0] == '/') {
        continue;
      }
      sprintf(fpattern + fpos, "%s", p->pattern);
      if (patmatch(path, fpattern, isdir) == 1) {
        return false;
      }
    }
  }

  return true;
}
