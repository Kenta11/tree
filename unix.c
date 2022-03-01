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
#include "unix.h"

// C standard library
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

// System library
//// POSIX
#include <sys/stat.h>

// tree modules
#include "color.h"
#include "tree.h"
#include "xstdlib.h"

static char info[512] = {0};

static void printit(char *);
static char Ftype(mode_t mode);

/**
 * Must fix this someday
 */
static void printit(char *s) {
  if (Nflag) {
    if (Qflag) {
      fprintf(outfile, "\"%s\"", s);
    } else {
      fprintf(outfile, "%s", s);
    }
    return;
  }
  if (mb_cur_max > 1) {
    size_t c = strlen(s) + 1;
    wchar_t *ws = xmalloc((sizeof *ws) * c);
    if (mbstowcs(ws, s, c) != (size_t)-1) {
      if (Qflag) {
        putc('"', outfile);
      }
      for (wchar_t *tp = ws; *tp && c > 1; tp++, c--) {
        if (iswprint(*tp)) {
          fprintf(outfile, "%lc", (wint_t)*tp);
        } else if (qflag) {
          putc('?', outfile);
        } else {
          fprintf(outfile, "\\%03o", (unsigned int)*tp);
        }
      }
      if (Qflag) {
        putc('"', outfile);
      }
      free(ws);
      return;
    }
    free(ws);
  }
  if (Qflag) {
    putc('"', outfile);
  }
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if ((7 <= c && c <= 13) || (c == '\\') || (c == '"' && Qflag) ||
        (c == ' ' && !Qflag)) {
      putc('\\', outfile);
      if (c > 13) {
        putc(c, outfile);
      } else {
        putc("abtnvfr"[c - 7], outfile);
      }
    } else if (isprint(c)) {
      putc(c, outfile);
    } else if (qflag) {
      if (mb_cur_max > 1 && c > 127) {
        putc(c, outfile);
      } else {
        putc('?', outfile);
      }
    } else {
      fprintf(outfile, "\\%03o", c);
    }
  }
  if (Qflag) {
    putc('"', outfile);
  }
}

static char Ftype(mode_t mode) {
  int m = mode & S_IFMT;
  if (!dflag && m == S_IFDIR) {
    return '/';
  } else if (m == S_IFSOCK) {
    return '=';
  } else if (m == S_IFIFO) {
    return '|';
  } else if (m == S_IFLNK) {
    return '@'; /* Here, but never actually used though. */
  }
#ifdef S_IFDOOR
  else if (m == S_IFDOOR) {
    return '>';
  }
#endif
  else if ((m == S_IFREG) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
    return '*';
  }
  return 0;
}

int unix_printinfo(struct _info *file, int level) {
  fillinfo(info, file);
  if (metafirst) {
    if (info[0] == '[') {
      fprintf(outfile, "%s  ", info);
    }
    if (!noindent) {
      indent(level);
    }
  } else {
    if (!noindent) {
      indent(level);
    }
    if (info[0] == '[') {
      fprintf(outfile, "%s  ", info);
    }
  }
  return 0;
}

int unix_printfile(char *dirname, char *filename, struct _info *file,
                   int descend) {
  bool colored;

  (void)dirname;
  (void)descend;

  if (file && colorize) {
    if (file->lnk && linktargetcolor) {
      colored = color(file->lnkmode, file->name, file->orphan, false);
    } else {
      colored = color(file->mode, file->name, file->orphan, false);
    }
  } else {
    colored = false;
  }

  printit(filename);

  if (colored) {
    endcolor();
  }

  if (file) {
    if (Fflag && !file->lnk) {
      char c = Ftype(file->mode);
      if (c != 0) {
        fputc(c, outfile);
      }
    }

    if (file->lnk) {
      fprintf(outfile, " -> ");
      if (colorize) {
        colored = color(file->lnkmode, file->lnk, file->orphan, true);
      }
      printit(file->lnk);
      if (colored) {
        endcolor();
      }
      if (Fflag) {
        char c = Ftype(file->lnkmode);
        if (c != 0) {
          fputc(c, outfile);
        }
      }
    }
  }
  return 0;
}

int unix_error(char *error) {
  fprintf(outfile, "  [%s]", error);
  return 0;
}

void unix_newline(struct _info *file, int level, int postdir, int needcomma) {
  (void)needcomma;

  if (postdir <= 0) {
    fprintf(outfile, "\n");
  }
  if ((file != NULL) && (file->comment != NULL)) {
    int infosize = 0, line, lines;
    if (metafirst) {
      infosize = info[0] == '[' ? strlen(info) + 2 : 0;
    }

    for (lines = 0; file->comment[lines]; lines++)
      ;
    dirs[level + 1] = 1;
    for (line = 0; line < lines; line++) {
      if (metafirst) {
        printf("%*s", infosize, "");
      }
      indent(level);
      printcomment(line, lines, file->comment[line]);
    }
    dirs[level + 1] = 0;
  }
}

void unix_report(struct totals tot) {
  fputc('\n', outfile);

  if (duflag) {
    char buf[256];
    psize(buf, tot.size);
    fprintf(outfile, "%s%s used in ", buf, hflag || siflag ? "" : " bytes");
  }

  if (dflag) {
    fprintf(outfile, "%ld director%s\n", tot.dirs,
            (tot.dirs == 1 ? "y" : "ies"));
  } else {
    fprintf(outfile, "%ld director%s, %ld file%s\n", tot.dirs,
            (tot.dirs == 1 ? "y" : "ies"), tot.files,
            (tot.files == 1 ? "" : "s"));
  }
}
