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

// C standard library
#include <stdio.h>

// System library
//// POSIX
#include <sys/stat.h>
#include <sys/types.h>

// tree modules
#include "bool.h"
#include "filter.h"
#include "info.h"
#include "list.h"

struct listingcalls {
  void (*intro)(void);
  void (*outtro)(void);
  int (*printinfo)(char *dirname, struct _info *file, int level);
  int (*printfile)(char *dirname, char *filename, struct _info *file,
                   int descend);
  int (*error)(char *error);
  void (*newline)(struct _info *file, int level, int postdir, int needcomma);
  void (*close)(struct _info *file, int level, int needcomma);
  void (*report)(struct totals tot);
};

extern char *version, *hversion, *host, *title, *sp, *_nl, *file_comment,
    *file_pathsep;
extern const char *charset;
extern bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag,
    Dflag, inodeflag, devflag, hflag, Rflag, Hflag, siflag, cflag, duflag,
    qflag, Nflag, Qflag, pruneflag, noindent, xdev, force_color, noreport,
    nocolor, nolinks, matchdirs, metafirst, reverse;
extern int flimit, pattern, ipattern, Level, *dirs, errors, mb_cur_max;
extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev,
                                     off_t *size, char **err);
extern struct listingcalls lc;
extern int (*topsort)();
#ifdef __EMX__
extern const u_short ifmt[];
#else
extern const u_int ifmt[];
extern const char *ftype[];
#endif
extern FILE *outfile;

void setoutput(char *filename);
void push_files(char *dir, struct ignorefile **ig, struct infofile **inf);
int patignore(char *name, int isdir);
int patinclude(char *name, int isdir);
struct _info **read_dir(char *dir, int *n, int infotop);
int patmatch(char *, char *, int);
void indent(int maxlevel);
void free_dir(struct _info **);
char *do_date(time_t);
int psize(char *buf, off_t size);
char *fillinfo(char *buf, struct _info *ent);
#ifdef __EMX__
char *prot(long);
#else
char *prot(mode_t);
#endif

#endif // TREE_H
