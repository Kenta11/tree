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
#include "info.h"

// C standard library
#include <stdlib.h>

// tree modules
#include "color.h"
#include "filter.h"
#include "path.h"
#include "tree.h"
#include "xstdlib.h"

/**
 * TODO: Make a "filenote" command for info comments.
 * maybe TODO: Support language extensions (i.e. .info.en, .info.gr, etc)
 * # comments
 * pattern
 * pattern
 * 	info messages
 * 	more info
 */

static struct infofile *infostack = NULL;

static struct comment *new_comment(struct pattern *phead, char **line,
                                   int lines) {
  struct comment *com = xmalloc(sizeof *com);
  com->pattern = phead;
  com->desc = xmalloc((sizeof *(com->desc)) * (lines + 1));
  int i;
  for (i = 0; i < lines; i++) {
    com->desc[i] = line[i];
  }
  com->desc[i] = NULL;
  com->next = NULL;
  return com;
}

struct infofile *new_infofile(char *path) {
  char buf[PATH_MAX];
  FILE *fp;

  if (strcmp(path, INFO_PATH) == 0) {
    fp = fopen(path, "r");
  } else {
    snprintf(buf, PATH_MAX, "%s/.info", path);
    fp = fopen(buf, "r");
  }
  if (fp == NULL) {
    return NULL;
  }

  struct comment *chead = NULL, *cend = NULL, *com;
  struct pattern *phead = NULL, *pend = NULL;
  char *line[PATH_MAX];
  int lines = 0;
  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') {
      continue;
    }
    gittrim(buf);
    if (strlen(buf) < 1) {
      continue;
    }

    if (buf[0] == '\t') {
      line[lines++] = scopy(buf + 1);
    } else {
      if (lines != 0) {
        // Save previous pattern/message:
        if (phead != NULL) {
          com = new_comment(phead, line, lines);
          if (!chead) {
            chead = cend = com;
          } else {
            cend = cend->next = com;
          }
        } else {
          // Accumulated info message lines w/ no associated pattern?
          for (int i = 0; i < lines; i++) {
            free(line[i]);
          }
        }
        // Reset for next pattern/message:
        phead = pend = NULL;
        lines = 0;
      }
      struct pattern *p = new_pattern(buf);
      if (phead == NULL) {
        phead = pend = p;
      } else {
        pend = pend->next = p;
      }
    }
  }
  if (phead != NULL) {
    com = new_comment(phead, line, lines);
    if (chead == NULL) {
      chead = cend = com;
    } else {
      cend = cend->next = com;
    }
  } else {
    for (int i = 0; i < lines; i++) {
      free(line[i]);
    }
  }

  fclose(fp);

  struct infofile *inf = xmalloc(sizeof *inf);
  inf->comments = chead;
  inf->path = scopy(path);
  inf->next = NULL;

  return inf;
}

void push_infostack(struct infofile *inf) {
  if (inf != NULL) {
    inf->next = infostack;
    infostack = inf;
  }
}

void pop_infostack(void) {
  if (infostack == NULL) {
    return;
  }

  struct infofile *inf = infostack;
  infostack = infostack->next;

  free(inf->path);
  for (struct comment *comment = inf->comments, *next; comment != NULL;
       comment = next) {
    next = comment->next;
    for (struct pattern *pattern = comment->pattern, *next; pattern != NULL;
         pattern = next) {
      next = pattern->next;
      free(pattern->pattern);
      free(pattern);
    }
    size_t i;
    for (i = 0; comment->desc[i] != NULL; i++) {
      free(comment->desc[i]);
    }
    free(comment->desc[i]);
    free(comment->desc);
    free(comment);
  }

  free(inf);
}

/**
 * Returns an info pointer if a path matches a pattern.
 * top == 1 if called in a directory with a .info file.
 */
struct comment *infocheck(char *path, char *name, int top, int isdir) {
  struct infofile *inf = infostack;
  if (inf == NULL) {
    return NULL;
  }

  for (inf = infostack; inf != NULL; inf = inf->next) {
    for (struct comment *com = inf->comments; com != NULL; com = com->next) {
      for (struct pattern *p = com->pattern; p != NULL; p = p->next) {
        if ((patmatch(path, p->pattern, isdir) == 1) ||
            (top && (patmatch(name, p->pattern, isdir) == 1))) {
          return com;
        }
      }
    }
    top = 0;
  }
  return NULL;
}

void printcomment(int line, int lines, char *s) {
  if (lines == 1) {
    fprintf(outfile, "%s ", linedraw->csingle);
  } else if (line == 0) {
    fprintf(outfile, "%s ", linedraw->ctop);
  } else if (line < 2) {
    fprintf(outfile, "%s ", linedraw->cmid);
  } else {
    fprintf(outfile, "%s ",
            (line == lines - 1) ? linedraw->cbot : linedraw->cext);
  }
  fprintf(outfile, "%s\n", s);
}
