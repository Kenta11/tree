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

#include "tree.h"

// C standard library
#include <ctype.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <wctype.h>

// System library
//// POSIX
#include <dirent.h>
#include <langinfo.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __linux__
#include <fcntl.h>
#endif

// tree modules
#include "color.h"
#include "file.h"
#include "filter.h"
#include "hash.h"
#include "html.h"
#include "info.h"
#include "json.h"
#include "list.h"
#include "path.h"
#include "unix.h"
#include "xml.h"
#include "xstdlib.h"
#include "xstring.h"

#define MINIT 30 /* number of dir entries to initially allocate */
#define MINC 20  /* allocation increment */
#ifdef __linux__
#define ENV_STDDATA_FD "STDDATA_FD"
#ifndef STDDATA_FILENO
#define STDDATA_FILENO 3
#endif
#elif defined(__ANDROID)
#define mbstowcs(w, m, x) mbsrtowcs(w, (const char **)(&#m), x, NULL)
#endif

const char *version =
    "$Version: $ tree v2.0.2 (c) 1996 - 2022 by Steve Baker, Thomas Moore, "
    "Francesc Rocher, Florian Sesser, Kyosuke Tokoro $";
const char *hversion =
    "\t\t tree v2.0.2 %s 1996 - 2022 by Steve Baker and Thomas Moore <br>\n"
    "\t\t HTML output hacked and copyleft %s 1998 by Francesc Rocher <br>\n"
    "\t\t JSON output hacked and copyleft %s 2014 by Florian Sesser <br>\n"
    "\t\t Charsets / OS/2 support %s 2001 by Kyosuke Tokoro\n";
const char *host = NULL, *title = "Directory Tree", *sp = " ", *_nl = "\n",
           *file_comment = "#", *file_pathsep = "/";
const char *charset = NULL;
bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag, Dflag,
    inodeflag, devflag, hflag, Rflag, Hflag, siflag, cflag, duflag, qflag,
    Nflag, Qflag, pruneflag, noindent, xdev, force_color, noreport, nocolor,
    nolinks, matchdirs, metafirst, reverse;
int flimit, pattern = 0, ipattern = 0, Level, *dirs, errors, mb_cur_max;
struct _info **(*getfulltree)(char *d, unsigned long lev, dev_t dev,
                              off_t *size, char **err) = NULL;
const struct linedraw *linedraw = NULL;
int (*topsort)(const struct _info **, const struct _info **) = NULL;
#ifdef S_IFPORT
const unsigned int ifmt[] = {S_IFREG,  S_IFDIR, S_IFLNK,  S_IFCHR,  S_IFBLK,
                             S_IFSOCK, S_IFIFO, S_IFDOOR, S_IFPORT, 0};
const char *ftype[] = {"file",  "directory", "link", "char",
                       "block", "socket",    "fifo", "door",
                       "port",  "unknown",   NULL};
#else
const unsigned int ifmt[] = {S_IFREG, S_IFDIR,  S_IFLNK, S_IFCHR,
                             S_IFBLK, S_IFSOCK, S_IFIFO, 0};
const char *ftype[] = {"file",   "directory", "link",    "char", "block",
                       "socket", "fifo",      "unknown", NULL};
#endif
FILE *outfile = NULL;

static char **patterns = NULL, **ipatterns = NULL, *sLevel, *timefmt = NULL,
            *lbuf = NULL, *path = NULL;
static bool Xflag, Jflag, ignorecase, fromfile, showinfo, gitignore, ansilines;
static int maxpattern = 0, maxipattern = 0;
static int (*basesort)(const struct _info **, const struct _info **) = NULL;
static unsigned long maxdirs;

#ifdef S_IFPORT
static const char fmt[] = "-dlcbspDP?";
#else
static const char fmt[] = "-dlcbsp?";
#endif

static struct _info **unix_getfulltree(char *d, unsigned long lev, dev_t dev,
                                       off_t *size, char **err);
static int alnumsort(const struct _info **a, const struct _info **b);
static int ctimesort(const struct _info **a, const struct _info **b);
static int dirsfirst(const struct _info **a, const struct _info **b);
static int filesfirst(const struct _info **a, const struct _info **b);
static int fsizesort(struct _info **a, struct _info **b);
static int mtimesort(const struct _info **a, const struct _info **b);
static int versort(const struct _info **a, const struct _info **b);
static void usage(int n);
static struct _info *getinfo(char *name, char *path);
static int sizecmp(off_t a, off_t b);
static char cond_lower(char c);

/* This is for all the impossible things people wanted the old tree to do.
 * This can and will use a large amount of memory for large directory trees
 * and also take some time.
 */
static struct _info **unix_getfulltree(char *d, unsigned long lev, dev_t dev,
                                       off_t *size, char **err) {
  *err = NULL;
  if ((0 <= Level) && ((unsigned long)Level < lev)) {
    return NULL;
  }

  if (xdev && (lev == 0)) {
    struct stat sb;
    stat(d, &sb);
    dev = sb.st_dev;
  }
  // if the directory name matches, turn off pattern matching for contents
  int tmp_pattern = 0;
  if (matchdirs && (pattern != 0)) {
    unsigned long lev_tmp = lev;
    char *start_rel_path;
    for (start_rel_path = d + strlen(d); start_rel_path != d;
         --start_rel_path) {
      if (*start_rel_path == '/') {
        --lev_tmp;
      }
      if (lev_tmp <= 0) {
        if (*start_rel_path) {
          ++start_rel_path;
        }
        break;
      }
    }
    if (*start_rel_path && patinclude(start_rel_path, 1)) {
      tmp_pattern = pattern;
      pattern = 0;
    }
  }

  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  push_files(d, &ig, &inf);

  int n;
  struct _info **dir = read_dir(d, &n, inf != NULL);
  struct _info **sav = dir;
  if (tmp_pattern != 0) {
    pattern = tmp_pattern;
  }
  if ((dir == NULL) && (n != 0)) {
    *err = scopy("error opening dir");
    errors++;
    free(dir);
    return NULL;
  }
  if (n == 0) {
    if (sav != NULL) {
      free_dir(sav);
    }
    return NULL;
  }
  unsigned long pathsize = PATH_MAX;
  char *path = xmalloc(pathsize);

  if ((0 < flimit) && (flimit < n)) {
    sprintf(path, "%d entries exceeds filelimit, not opening dir", n);
    *err = scopy(path);
    free_dir(sav);
    free(path);
    return NULL;
  }

  if (lev >= (maxdirs - 1)) {
    maxdirs += 1024;
    dirs = xrealloc(dirs, sizeof(int) * maxdirs);
  }

  while (*dir != NULL) {
    if (((*dir)->isdir) && !(xdev && (dev != (*dir)->dev))) {
      if (((*dir)->lnk) != NULL) {
        if (lflag) {
          if (findino((*dir)->inode, (*dir)->dev)) {
            (*dir)->err = scopy("recursive, not followed");
          } else {
            saveino((*dir)->inode, (*dir)->dev);
            if (*((*dir)->lnk) == '/') {
              (*dir)->child = unix_getfulltree((*dir)->lnk, lev + 1, dev,
                                               &((*dir)->size), &((*dir)->err));
            } else {
              if (strlen(d) + strlen((*dir)->lnk) + 2 > pathsize) {
                pathsize = strlen(d) + strlen((*dir)->name) + 1024;
                path = xrealloc(path, pathsize);
              }
              if (fflag && (strcmp(d, "/") == 0)) {
                sprintf(path, "%s%s", d, (*dir)->lnk);
              } else {
                sprintf(path, "%s/%s", d, (*dir)->lnk);
              }
              (*dir)->child = unix_getfulltree(path, lev + 1, dev,
                                               &((*dir)->size), &((*dir)->err));
            }
          }
        }
      } else {
        if (strlen(d) + strlen((*dir)->name) + 2 > pathsize) {
          pathsize = strlen(d) + strlen((*dir)->name) + 1024;
          path = xrealloc(path, pathsize);
        }
        if (fflag && (strcmp(d, "/") == 0)) {
          sprintf(path, "%s%s", d, (*dir)->name);
        } else {
          sprintf(path, "%s/%s", d, (*dir)->name);
        }
        saveino((*dir)->inode, (*dir)->dev);
        (*dir)->child = unix_getfulltree(path, lev + 1, dev, &((*dir)->size),
                                         &((*dir)->err));
      }
      // prune empty folders, unless they match the requested pattern
      if (pruneflag && ((*dir)->child == NULL) &&
          !(matchdirs && (pattern != 0) &&
            (patinclude((*dir)->name, (*dir)->isdir) != 0))) {
        struct _info *sp = *dir;
        for (struct _info **p = dir; *p != NULL; p++) {
          *p = *(p + 1);
        }
        n--;
        free(sp->name);
        if (sp->lnk != NULL) {
          free(sp->lnk);
        }
        free(sp);
        continue;
      }
    }
    if (duflag) {
      *size += (*dir)->size;
    }
    dir++;
  }

  // sorting needs to be deferred for --du:
  if (topsort != NULL) {
    qsort(sav, n, sizeof(struct _info *),
          (int (*)(const void *, const void *))topsort);
  }

  free(path);
  if (n == 0) {
    free_dir(sav);
    sav = NULL;
  } else {
    if (ig != NULL) {
      pop_filterstack();
    }
    if (inf != NULL) {
      pop_infostack();
    }
  }
  return sav;
}

/* Sorting functions */
static int alnumsort(const struct _info **a, const struct _info **b) {
  int v = strcoll((*a)->name, (*b)->name);
  return reverse ? -v : v;
}

static int ctimesort(const struct _info **a, const struct _info **b) {
  time_t diff = (*b)->ctime - (*a)->ctime;

  int v;
  if (diff == 0) {
    v = strcoll((*a)->name, (*b)->name);
  } else {
    v = diff > 0 ? -1 : 1;
  }
  return reverse ? -v : v;
}

static int dirsfirst(const struct _info **a, const struct _info **b) {
  int v;

  if ((*a)->isdir != (*b)->isdir) {
    v = (*a)->isdir ? -1 : 1;
  } else {
    v = basesort(a, b);
  }

  return v;
}

/**
 * filesfirst and dirsfirst are now top-level meta-sorts.
 */
static int filesfirst(const struct _info **a, const struct _info **b) {
  int v;

  if ((*a)->isdir != (*b)->isdir) {
    v = (*a)->isdir ? 1 : -1;
  } else {
    v = basesort(a, b);
  }

  return v;
}

static int fsizesort(struct _info **a, struct _info **b) {
  int v = sizecmp((*a)->size, (*b)->size);
  if (v == 0) {
    v = strcoll((*a)->name, (*b)->name);
  }
  return reverse ? -v : v;
}

static int mtimesort(const struct _info **a, const struct _info **b) {
  int v;

  if ((*a)->mtime == (*b)->mtime) {
    v = strcoll((*a)->name, (*b)->name);
  } else {
    v = (*a)->mtime < (*b)->mtime ? -1 : 1;
  }

  return reverse ? -v : v;
}

static int versort(const struct _info **a, const struct _info **b) {
  int v = xstrverscmp((*a)->name, (*b)->name);
  return reverse ? -v : v;
}

static void usage(int n) {
  /* 123456789!123456789!123456789!123456789!123456789!123456789!123456789!123456789!
   */
  /* \t9!123456789!123456789!123456789!123456789!123456789!123456789!123456789!
   */
  fprintf(
      n < 2 ? stderr : stdout,
      "usage: tree [-acdfghilnpqrstuvxACDFJQNSUX] [-L level [-R]] [-H  "
      "baseHREF]\n"
      "\t[-T title] [-o filename] [-P pattern] [-I pattern] [--gitignore]\n"
      "\t[--matchdirs] [--metafirst] [--ignore-case] [--nolinks] [--inodes]\n"
      "\t[--device] [--sort[=]<name>] [--dirsfirst] [--filesfirst]\n"
      "\t[--filelimit #] [--si] [--du] [--prune] [--charset X]\n"
      "\t[--timefmt[=]format] [--fromfile] [--noreport] [--version] [--help]\n"
      "\t[--] [directory ...]\n");

  if (n < 2) {
    return;
  }
  fprintf(
      stdout,
      "  ------- Listing options -------\n"
      "  -a            All files are listed.\n"
      "  -d            List directories only.\n"
      "  -l            Follow symbolic links like directories.\n"
      "  -f            Print the full path prefix for each file.\n"
      "  -x            Stay on current filesystem only.\n"
      "  -L level      Descend only level directories deep.\n"
      "  -R            Rerun tree when max dir level reached.\n"
      "  -P pattern    List only those files that match the pattern given.\n"
      "  -I pattern    Do not list files that match the given pattern.\n"
      "  --gitignore   Filter by using .gitignore files.\n"
      "  --ignore-case Ignore case when pattern matching.\n"
      "  --matchdirs   Include directory names in -P pattern matching.\n"
      "  --metafirst   Print meta-data at the beginning of each line.\n"
      "  --info        Print information about files found in .info files.\n"
      "  --noreport    Turn off file/directory count at end of tree listing.\n"
      "  --charset X   Use charset X for terminal/HTML and indentation line "
      "output.\n"
      "  --filelimit # Do not descend dirs with more than # files in them.\n"
      "  -o filename   Output to file instead of stdout.\n"
      "  ------- File options -------\n"
      "  -q            Print non-printable characters as '?'.\n"
      "  -N            Print non-printable characters as is.\n"
      "  -Q            Quote filenames with double quotes.\n"
      "  -p            Print the protections for each file.\n"
      "  -u            Displays file owner or UID number.\n"
      "  -g            Displays file group owner or GID number.\n"
      "  -s            Print the size in bytes of each file.\n"
      "  -h            Print the size in a more human readable way.\n"
      "  --si          Like -h, but use in SI units (powers of 1000).\n"
      "  -D            Print the date of last modification or (-c) status "
      "change.\n"
      "  --timefmt <f> Print and format time according to the format <f>.\n"
      "  -F            Appends '/', '=', '*', '@', '|' or '>' as per ls -F.\n"
      "  --inodes      Print inode number of each file.\n"
      "  --device      Print device ID number to which each file belongs.\n"
      "  ------- Sorting options -------\n"
      "  -v            Sort files alphanumerically by version.\n"
      "  -t            Sort files by last modification time.\n"
      "  -c            Sort files by last status change time.\n"
      "  -U            Leave files unsorted.\n"
      "  -r            Reverse the order of the sort.\n"
      "  --dirsfirst   List directories before files (-U disables).\n"
      "  --filesfirst  List files before directories (-U disables).\n"
      "  --sort X      Select sort: name,version,size,mtime,ctime.\n"
      "  ------- Graphics options -------\n"
      "  -i            Don't print indentation lines.\n"
      "  -A            Print ANSI lines graphic indentation lines.\n"
      "  -S            Print with CP437 (console) graphics indentation lines.\n"
      "  -n            Turn colorization off always (-C overrides).\n"
      "  -C            Turn colorization on always.\n"
      "  ------- XML/HTML/JSON options -------\n"
      "  -X            Prints out an XML representation of the tree.\n"
      "  -J            Prints out an JSON representation of the tree.\n"
      "  -H baseHREF   Prints out HTML format with baseHREF as top directory.\n"
      "  -T string     Replace the default HTML title and H1 header with "
      "string.\n"
      "  --nolinks     Turn off hyperlinks in HTML output.\n"
      "  ------- Input options -------\n"
      "  --fromfile    Reads paths from files (.=stdin)\n"
      "  ------- Miscellaneous options -------\n"
      "  --version     Print version and exit.\n"
      "  --help        Print usage and this help message and exit.\n"
      "  --            Options processing terminator.\n");
}

/**
 * Split out stat portion from read_dir as prelude to just using stat structure
 * directly.
 */
static struct _info *getinfo(char *name, char *path) {
  static int lbufsize = 0;

  if (lbuf == NULL) {
    lbufsize = PATH_MAX;
    lbuf = xmalloc(lbufsize);
  }

  struct stat lst;
  if (lstat(path, &lst) < 0) {
    return NULL;
  }

  int rs;
  struct stat st;
  if ((lst.st_mode & S_IFMT) == S_IFLNK) {
    rs = stat(path, &st);
    if (rs < 0) {
      memset(&st, 0, sizeof(st));
    }
  } else {
    rs = 0;
    st.st_mode = lst.st_mode;
    st.st_dev = lst.st_dev;
    st.st_ino = lst.st_ino;
  }

  int isdir = (st.st_mode & S_IFMT) == S_IFDIR;

  if (gitignore && filtercheck(path, isdir)) {
    return NULL;
  }

  if ((lst.st_mode & S_IFMT) != S_IFDIR &&
      !(lflag && ((st.st_mode & S_IFMT) == S_IFDIR))) {
    if ((pattern != 0) && (patinclude(name, isdir) == 0)) {
      return NULL;
    }
  }
  if ((ipattern != 0) && (patignore(name, isdir) != 0)) {
    return NULL;
  }

  if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) {
    return NULL;
  }

  struct _info *ent = (struct _info *)xmalloc(sizeof(struct _info));
  memset(ent, 0, sizeof(struct _info));

  ent->name = scopy(name);
  /* We should just incorporate struct stat into _info, and eliminate this
   * unnecessary copying. Made sense long ago when we had fewer options and
   * didn't need half of stat.
   */
  ent->mode = lst.st_mode;
  ent->uid = lst.st_uid;
  ent->gid = lst.st_gid;
  ent->size = lst.st_size;
  ent->dev = st.st_dev;
  ent->inode = st.st_ino;
  ent->ldev = lst.st_dev;
  ent->linode = lst.st_ino;
  ent->lnk = NULL;
  ent->orphan = false;
  ent->err = NULL;
  ent->child = NULL;

  ent->atime = lst.st_atime;
  ent->ctime = lst.st_ctime;
  ent->mtime = lst.st_mtime;

  /* These should be eliminated, as they're barely used: */
  ent->isdir = isdir;
  ent->issok = ((st.st_mode & S_IFMT) == S_IFSOCK);
  ent->isfifo = ((st.st_mode & S_IFMT) == S_IFIFO);
  ent->isexe = (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 1 : 0;

  if ((lst.st_mode & S_IFMT) == S_IFLNK) {
    if (lst.st_size + 1 > lbufsize) {
      lbufsize = lst.st_size + 8192;
      lbuf = xrealloc(lbuf, lbufsize);
    }
    int len = readlink(path, lbuf, lbufsize - 1);
    if (len < 0) {
      ent->lnk = scopy("[Error reading symbolic link information]");
      ent->isdir = false;
    } else {
      lbuf[len] = 0;
      ent->lnk = scopy(lbuf);
      if (rs < 0) {
        ent->orphan = true;
      }
    }
    ent->lnkmode = st.st_mode;
  }

  ent->comment = NULL;

  return ent;
}

static int sizecmp(off_t a, off_t b) {
  int v;

  if (a == b) {
    v = 0;
  } else if (a < b) {
    v = 1;
  } else {
    v = -1;
  }

  return v;
}

static inline char cond_lower(char c) { return ignorecase ? tolower(c) : c; }

void setoutput(char *filename) {
  if (filename == NULL) {
    if (outfile == NULL) {
      outfile = stdout;
    }
  } else {
    outfile = fopen(filename, "w");
    if (outfile == NULL) {
      fprintf(stderr, "tree: invalid filename '%s'\n", filename);
      exit(1);
    }
  }
}

void push_files(char *dir, struct ignorefile **ig, struct infofile **inf) {
  if (gitignore) {
    *ig = new_ignorefile(dir);
    if (*ig != NULL) {
      push_filterstack(*ig);
    }
  }
  if (showinfo) {
    *inf = new_infofile(dir);
    if (*inf != NULL) {
      push_infostack(*inf);
    }
  }
}

/**
 * True if file matches an -I pattern
 */
int patignore(char *name, int isdir) {
  int v = 0;

  for (int i = 0; i < ipattern; i++) {
    if (patmatch(name, ipatterns[i], isdir) != 0) {
      v = 1;
      break;
    }
  }

  return v;
}

/**
 * True if name matches a -P pattern
 */
int patinclude(char *name, int isdir) {
  int v = 0;

  for (int i = 0; i < pattern; i++) {
    if (patmatch(name, patterns[i], isdir) != 0) {
      v = 1;
      break;
    }
  }

  return v;
}

struct _info **read_dir(char *dir, int *n, int infotop) {
  static unsigned long pathsize;

  if (path == NULL) {
    pathsize = strlen(dir) + PATH_MAX;
    path = xmalloc(pathsize);
  }

  *n = -1;
  DIR *d = opendir(dir);
  if (d == NULL) {
    return NULL;
  }

  int ne = MINIT;
  struct _info **dl = (struct _info **)xmalloc(sizeof(struct _info *) * ne);

  int p = 0;
  const bool es = (dir[strlen(dir) - 1] == '/');
  struct dirent *entry;
  while ((entry = (struct dirent *)readdir(d))) {
    if ((strcmp("..", entry->d_name) == 0) ||
        (strcmp(".", entry->d_name) == 0)) {
      continue;
    }
    if (Hflag && (strcmp(entry->d_name, "00Tree.html") == 0)) {
      continue;
    }
    if (!aflag && (entry->d_name[0] == '.')) {
      continue;
    }

    if (strlen(dir) + strlen(entry->d_name) + 2 > pathsize) {
      pathsize = strlen(dir) + strlen(entry->d_name) + PATH_MAX;
      path = xrealloc(path, pathsize);
    }
    if (es) {
      sprintf(path, "%s%s", dir, entry->d_name);
    } else {
      sprintf(path, "%s/%s", dir, entry->d_name);
    }

    struct _info *info = getinfo(entry->d_name, path);
    if (info) {
      if (showinfo) {
        struct comment *com =
            infocheck(path, entry->d_name, infotop, info->isdir);
        if (com != NULL) {
          int i;
          for (i = 0; com->desc[i] != NULL; i++)
            ;
          info->comment = xmalloc(sizeof(char *) * (i + 1));
          for (i = 0; com->desc[i] != NULL; i++) {
            info->comment[i] = scopy(com->desc[i]);
          }
          info->comment[i] = NULL;
        }
      }
      if (p == (ne - 1)) {
        ne += MINC;
        dl = (struct _info **)xrealloc(dl, sizeof(struct _info *) * ne);
      }
      dl[p++] = info;
    }
  }
  closedir(d);

  *n = p;
  if (*n == 0) {
    free(dl);
    return NULL;
  }

  dl[p] = NULL;
  return dl;
}

/*
 * Patmatch() code courtesy of Thomas Moore (dark@mama.indstate.edu)
 * '|' support added by David MacMahon (davidm@astron.Berkeley.EDU)
 * Case insensitive support added by Jason A. Donenfeld (Jason@zx2c4.com)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
int patmatch(char *buf, char *pat, int isdir) {
  int match = 1;

  /* If a bar is found, call patmatch recursively on the two sub-patterns */
  char *bar = strchr(pat, '|');
  if (bar != NULL) {
    /* If the bar is the first or last character, it's a syntax error */
    if ((bar == pat) || (bar[1] == 0)) {
      return -1;
    }
    /* Break pattern into two sub-patterns */
    *bar = '\0';
    match = patmatch(buf, pat, isdir);
    if (!match) {
      match = patmatch(buf, bar + 1, isdir);
    }
    /* Join sub-patterns back into one pattern */
    *bar = '|';
    return match;
  }

  char pprev = 0;
  int n;
  while (*pat && match) {
    switch (*pat) {
    case '[': {
      pat++;
      if (*pat != '^') {
        n = 1;
        match = 0;
      } else {
        pat++;
        n = 0;
      }
      while (*pat != ']') {
        if (*pat == '\\') {
          pat++;
        }
        if (*pat == 0) {
          return -1;
        }
        if (pat[1] == '-') {
          char m = *pat;
          pat += 2;
          if (*pat == '\\' && *pat) {
            pat++;
          }
          if ((cond_lower(*buf) >= cond_lower(m)) &&
              (cond_lower(*buf) <= cond_lower(*pat))) {
            match = n;
          }
          if (!*pat) {
            pat--;
          }
        } else if (cond_lower(*buf) == cond_lower(*pat)) {
          match = n;
        }
        pat++;
      }
      buf++;
      break;
    }
    case '*': {
      pat++;
      if (*pat == 0) {
        return 1;
      }
      match = 0;
      /* "Support" ** for .gitignore support, mostly the same as *: */
      if (*pat == '*') {
        pat++;
        if (*pat == 0) {
          return 1;
        }

        while (*buf && !(match = patmatch(buf, pat, isdir))) {
          // ../**/.. is allowed to match a null /:
          if (pprev == '/' && *pat == '/' && *(pat + 1) &&
              (match = patmatch(buf, pat + 1, isdir))) {
            return match;
          }
          buf++;
          while (*buf && *buf != '/') {
            buf++;
          }
        }
      } else {
        while (*buf && !(match = patmatch(buf++, pat, isdir)))
          ;
      }
      if ((*buf == 0) && (match == 0)) {
        match = patmatch(buf, pat, isdir);
      }
      return match;
    }
    case '?': {
      if (*buf == 0) {
        return 0;
      }
      buf++;
      break;
    }
    case '/': {
      if ((*(pat + 1) == 0) && (*buf == 0)) {
        return isdir;
      }
      match = (*buf++ == *pat);
      break;
    }
    default: {
      if ((*pat) == '\\') {
        pat++;
      }
      match = (cond_lower(*buf++) == cond_lower(*pat));
      break;
    }
    }
    pprev = *pat++;
    if (match < 1) {
      return match;
    }
  }
  if (*buf == 0) {
    return match;
  }
  return 0;
}

/**
 * They cried out for ANSI-lines (not really), but here they are, as an option
 * for the xterm and console capable among you, as a run-time option.
 */
void indent(int maxlevel) {
  if (ansilines) {
    if (dirs[1]) {
      fprintf(outfile, "\033(0");
    }
    for (int i = 1; (i <= maxlevel) && (dirs[i] != 0); i++) {
      if (dirs[i + 1] != 0) {
        if (dirs[i] == 1) {
          fprintf(outfile, "\170   ");
        } else {
          printf("    ");
        }
      } else {
        if (dirs[i] == 1) {
          fprintf(outfile, "\164\161\161 ");
        } else {
          fprintf(outfile, "\155\161\161 ");
        }
      }
    }
    if (dirs[1] != 0) {
      fprintf(outfile, "\033(B");
    }
  } else {
    if (Hflag) {
      fprintf(outfile, "\t");
    }
    for (int i = 1; (i <= maxlevel) && (dirs[i] != 0); i++) {
      fprintf(outfile, "%s ",
              dirs[i + 1]
                  ? (dirs[i] == 1 ? linedraw->vert
                                  : (Hflag ? "&nbsp;&nbsp;&nbsp;" : "   "))
                  : (dirs[i] == 1 ? linedraw->vert_left : linedraw->corner));
    }
  }
}

void free_dir(struct _info **d) {
  for (int i = 0; d[i] != NULL; i++) {
    free(d[i]->name);
    if (d[i]->lnk) {
      free(d[i]->lnk);
    }
    free(d[i]);
  }
  free(d);
}

char *do_date(time_t t) {
  static char buf[256];
  struct tm *tm = localtime(&t);
  const time_t SIXMONTHS = (6 * 31 * 24 * 60 * 60);
  if (timefmt) {
    strftime(buf, 255, timefmt, tm);
    buf[255] = 0;
  } else {
    time_t c = time(NULL);
    /* Use strftime() so that locale is respected: */
    if ((c < t) || (t + SIXMONTHS) < c) {
      strftime(buf, 255, "%b %e  %Y", tm);
    } else {
      strftime(buf, 255, "%b %e %R", tm);
    }
  }

  return buf;
}

int psize(char *buf, off_t size) {
  static char *iec_unit = "BKMGTPEZY", *si_unit = "dkMGTPEZY";

  if (hflag || siflag) {
    int idx, usize = siflag ? 1000 : 1024;
    char *unit = siflag ? si_unit : iec_unit;
    for (idx = size < usize ? 0 : 1; size >= (usize * usize);
         idx++, size /= usize)
      ;
    if (idx == 0) {
      return sprintf(buf, " %4d", (int)size);
    } else {
      return sprintf(buf, ((size / usize) >= 10) ? " %3.0f%c" : " %3.1f%c",
                     (float)size / (float)usize, unit[idx]);
    }
  } else {
    return sprintf(buf,
                   sizeof(off_t) == sizeof(long long) ? " %11lld" : " %9lld",
                   (long long int)size);
  }
}

char *fillinfo(char *buf, struct _info *ent) {
  int n = 0;
  buf[n] = 0;
  if (inodeflag) {
#ifdef __USE_FILE_OFFSET64
    n += sprintf(buf, " %7lld", (long long)ent->linode);
#else
    n += sprintf(buf, " %7ld", (long int)ent->linode);
#endif
  }
  if (devflag) {
    n += sprintf(buf + n, " %3d", (int)ent->ldev);
  }
  if (pflag) {
    n += sprintf(buf + n, " %s", prot(ent->mode));
  }
  if (uflag) {
    n += sprintf(buf + n, " %-8.32s", uidtoname(ent->uid));
  }
  if (gflag) {
    n += sprintf(buf + n, " %-8.32s", gidtoname(ent->gid));
  }
  if (sflag) {
    n += psize(buf + n, ent->size);
  }
  if (Dflag) {
    n += sprintf(buf + n, " %s", do_date(cflag ? ent->ctime : ent->mtime));
  }

  if (buf[0] == ' ') {
    buf[0] = '[';
    sprintf(buf + n, "]");
  }

  return buf;
}

char *prot(mode_t m) {
  static char buf[11], perms[] = "rwxrwxrwx";

  int i;
  for (i = 0; ifmt[i] && (m & S_IFMT) != ifmt[i]; i++)
    ;
  buf[0] = fmt[i];

  /**
   * Nice, but maybe not so portable, it is should be no less portable than the
   * old code.
   */
  for (int b = S_IRUSR, i = 0; i < 9; b >>= 1, i++) {
    buf[i + 1] = (m & (b)) ? perms[i] : '-';
  }
  if (m & S_ISUID) {
    buf[3] = (buf[3] == '-') ? 'S' : 's';
  }
  if (m & S_ISGID) {
    buf[6] = (buf[6] == '-') ? 'S' : 's';
  }
  if (m & S_ISVTX) {
    buf[9] = (buf[9] == '-') ? 'T' : 't';
  }
  buf[10] = 0;

  return buf;
}

int main(int argc, char **argv) {
  struct sorts {
    char *name;
    int (*cmpfunc)();
  } sorts[] = {{"name", alnumsort},  {"version", versort}, {"size", fsizesort},
               {"mtime", mtimesort}, {"ctime", ctimesort}, {NULL, NULL}};

  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag =
      Dflag = qflag = Nflag = Qflag = Rflag = hflag = Hflag = siflag = cflag =
          noindent = force_color = nocolor = xdev = noreport = nolinks =
              reverse = ignorecase = matchdirs = inodeflag = devflag = Xflag =
                  Jflag = duflag = pruneflag = metafirst = gitignore =
                      ansilines = false;

  flimit = 0;
  maxdirs = PATH_MAX;
  dirs = xmalloc(sizeof(int) * maxdirs);
  memset(dirs, 0, sizeof(int) * maxdirs);
  dirs[0] = 0;
  Level = -1;

  getfulltree = unix_getfulltree;
  basesort = alnumsort;

  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");

  charset = getenv("TREE_CHARSET");
  if ((charset == NULL) && ((strcmp(nl_langinfo(CODESET), "UTF-8") == 0) ||
                            (strcmp(nl_langinfo(CODESET), "utf8") == 0))) {
    charset = "UTF-8";
  }

  struct listingcalls lc = (struct listingcalls){
      null_intro, null_outtro,  unix_printinfo, unix_printfile,
      unix_error, unix_newline, null_close,     unix_report};

/* Still a hack, but assume that if the macro is defined, we can use it: */
#ifdef MB_CUR_MAX
  mb_cur_max = (int)MB_CUR_MAX;
#else
  mb_cur_max = 1;
#endif

  char *stmp, *outfilename = NULL;
  char **dirname = NULL;
  bool optf = true;
  int n = 1, p = 0, q = 0;
  for (int i = 1; i < argc; i = n) {
    n++;
    if (optf && argv[i][0] == '-' && argv[i][1]) {
      for (int j = 1; argv[i][j] != 0; j++) {
        switch (argv[i][j]) {
        case 'N': {
          Nflag = true;
          break;
        }
        case 'q': {
          qflag = true;
          break;
        }
        case 'Q': {
          Qflag = true;
          break;
        }
        case 'd': {
          dflag = true;
          break;
        }
        case 'l': {
          lflag = true;
          break;
        }
        case 's': {
          sflag = true;
          break;
        }
        case 'h': {
          hflag = true;
          sflag = true; /* Assume they also want -s */
          break;
        }
        case 'u': {
          uflag = true;
          break;
        }
        case 'g': {
          gflag = true;
          break;
        }
        case 'f': {
          fflag = true;
          break;
        }
        case 'F': {
          Fflag = true;
          break;
        }
        case 'a': {
          aflag = true;
          break;
        }
        case 'p': {
          pflag = true;
          break;
        }
        case 'i': {
          noindent = true;
          _nl = "";
        } break;
        case 'C': {
          force_color = true;
          break;
        }
        case 'n': {
          nocolor = true;
          break;
        }
        case 'x': {
          xdev = true;
          break;
        }
        case 'P': {
          if (argv[n] == NULL) {
            fprintf(stderr, "tree: missing argument to -P option.\n");
            exit(1);
          }
          if (pattern >= maxpattern - 1) {
            maxpattern += 10;
            patterns = xrealloc(patterns, sizeof(char *) * maxpattern);
          }
          patterns[pattern++] = argv[n++];
          patterns[pattern] = NULL;
          break;
        }
        case 'I': {
          if (argv[n] == NULL) {
            fprintf(stderr, "tree: missing argument to -I option.\n");
            exit(1);
          }
          if (ipattern >= maxipattern - 1) {
            maxipattern += 10;
            ipatterns = xrealloc(ipatterns, sizeof(char *) * maxipattern);
          }
          ipatterns[ipattern++] = argv[n++];
          ipatterns[ipattern] = NULL;
          break;
        }
        case 'A': {
          ansilines = true;
          break;
        }
        case 'S': {
          charset = "IBM437";
          break;
        }
        case 'D': {
          Dflag = true;
          break;
        }
        case 't': {
          basesort = mtimesort;
          break;
        }
        case 'c': {
          basesort = ctimesort;
          cflag = true;
          break;
        }
        case 'r': {
          reverse = true;
          break;
        }
        case 'v': {
          basesort = versort;
          break;
        }
        case 'U': {
          basesort = NULL;
          break;
        }
        case 'X': {
          Xflag = true;
          Hflag = false;
          Jflag = false;
          lc = (struct listingcalls){xml_intro,     xml_outtro, xml_printinfo,
                                     xml_printfile, xml_error,  xml_newline,
                                     xml_close,     xml_report};
          break;
        }
        case 'J': {
          Jflag = true;
          Xflag = false;
          Hflag = false;
          lc = (struct listingcalls){
              json_intro, json_outtro,  json_printinfo, json_printfile,
              json_error, json_newline, json_close,     json_report};
          break;
        }
        case 'H': {
          Hflag = true;
          Xflag = false;
          Jflag = false;
          lc = (struct listingcalls){
              html_intro, html_outtro,  html_printinfo, html_printfile,
              html_error, html_newline, html_close,     html_report};
          if (argv[n] == NULL) {
            fprintf(stderr, "tree: missing argument to -H option.\n");
            exit(1);
          }
          host = argv[n++];
          sp = "&nbsp;";
          break;
        }
        case 'T': {
          if (argv[n] == NULL) {
            fprintf(stderr, "tree: missing argument to -T option.\n");
            exit(1);
          }
          title = argv[n++];
          break;
        }
        case 'R': {
          Rflag = true;
          break;
        }
        case 'L': {
          if ((sLevel = argv[n++]) == NULL) {
            fprintf(stderr, "tree: Missing argument to -L option.\n");
            exit(1);
          }
          Level = strtoul(sLevel, NULL, 0) - 1;
          if (Level < 0) {
            fprintf(stderr, "tree: Invalid level, must be greater than 0.\n");
            exit(1);
          }
          break;
        }
        case 'o': {
          if (argv[n] == NULL) {
            fprintf(stderr, "tree: missing argument to -o option.\n");
            exit(1);
          }
          outfilename = argv[n++];
          break;
        }
        default: {
          if (argv[i][1] == '-') {
            if (strcmp("--", argv[i]) == 0) {
              optf = false;
              break;
            }
            if (strcmp("--help", argv[i]) == 0) {
              usage(2);
              exit(0);
            }
            if (strcmp("--version", argv[i]) == 0) {
              const char *v = version + 12;
              printf("%.*s\n", (int)strlen(v) - 1, v);
              exit(0);
            }
            if (strcmp("--inodes", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              inodeflag = true;
              break;
            }
            if (strcmp("--device", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              devflag = true;
              break;
            }
            if (strcmp("--noreport", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              noreport = true;
              break;
            }
            if (strcmp("--nolinks", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              nolinks = true;
              break;
            }
            if (strcmp("--dirsfirst", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              topsort = dirsfirst;
              break;
            }
            if (strcmp("--filesfirst", argv[i]) == 0) {
              j = strlen(argv[i]) - 1;
              topsort = filesfirst;
              break;
            }
            if (strncmp("--filelimit", argv[i], 11) == 0) {
              if (*(argv[i] + 11) == '=') {
                if (*(argv[i] + 12) != 0) {
                  flimit = atoi(argv[i] + 12);
                  j = strlen(argv[i]) - 1;
                  break;
                } else {
                  fprintf(stderr, "tree: missing argument to --filelimit=\n");
                  exit(1);
                }
              }
              if (argv[n] != NULL) {
                flimit = atoi(argv[n++]);
                j = strlen(argv[i]) - 1;
              } else {
                fprintf(stderr, "tree: missing argument to --filelimit\n");
                exit(1);
              }
              break;
            }
            if (strncmp("--charset", argv[i], 9) == 0) {
              j = 9;
              if (*(argv[i] + j) == '=') {
                if (*(charset = (argv[i] + 10))) {
                  j = strlen(argv[i]) - 1;
                  break;
                } else {
                  fprintf(stderr, "tree: missing argument to --charset=\n");
                  exit(1);
                }
              }
              if (argv[n] != NULL) {
                charset = argv[n++];
                j = strlen(argv[i]) - 1;
              } else {
                linedraw = initlinedraw(1);
                exit(1);
              }
              break;
            }
            if (strncmp("--si", argv[i], 4) == 0) {
              j = strlen(argv[i]) - 1;
              sflag = true;
              hflag = true;
              siflag = true;
              break;
            }
            if (strncmp("--du", argv[i], 4) == 0) {
              j = strlen(argv[i]) - 1;
              sflag = true;
              duflag = true;
              break;
            }
            if (strncmp("--prune", argv[i], 7) == 0) {
              j = strlen(argv[i]) - 1;
              pruneflag = true;
              break;
            }
            if (strncmp("--timefmt", argv[i], 9) == 0) {
              j = 9;
              if (*(argv[i] + j) == '=') {
                if (*(argv[i] + (++j))) {
                  timefmt = scopy(argv[i] + j);
                  j = strlen(argv[i]) - 1;
                  break;
                } else {
                  fprintf(stderr, "tree: missing argument to --timefmt=\n");
                  exit(1);
                }
              } else if (argv[n] != NULL) {
                timefmt = scopy(argv[n]);
                n++;
                j = strlen(argv[i]) - 1;
              } else {
                fprintf(stderr, "tree: missing argument to --timefmt\n");
                exit(1);
              }
              Dflag = true;
              break;
            }
            if (strncmp("--ignore-case", argv[i], 13) == 0) {
              j = strlen(argv[i]) - 1;
              ignorecase = true;
              break;
            }
            if (strncmp("--matchdirs", argv[i], 11) == 0) {
              j = strlen(argv[i]) - 1;
              matchdirs = true;
              break;
            }
            if (strncmp("--sort", argv[i], 6) == 0) {
              j = 6;
              if (*(argv[i] + j) == '=') {
                if (*(argv[i] + (++j))) {
                  stmp = argv[i] + j;
                  j = strlen(argv[i]) - 1;
                } else {
                  fprintf(stderr, "tree: missing argument to --sort=\n");
                  exit(1);
                }
              } else if (argv[n] != NULL) {
                stmp = argv[n++];
                j = strlen(argv[i]) - 1;
              } else {
                fprintf(stderr, "tree: missing argument to --sort\n");
                exit(1);
              }
              basesort = NULL;
              for (int k = 0; sorts[k].name != NULL; k++) {
                if (strcasecmp(sorts[k].name, stmp) == 0) {
                  basesort = sorts[k].cmpfunc;
                  break;
                }
              }
              if (basesort == NULL) {
                fprintf(
                    stderr,
                    "tree: sort type '%s' not valid, should be one of: ", stmp);
                for (int k = 0; sorts[k].name != NULL; k++) {
                  printf("%s%c", sorts[k].name, sorts[k + 1].name ? ',' : '\n');
                }
                exit(1);
              }
              break;
            }
            if (strncmp("--fromfile", argv[i], 10) == 0) {
              j = strlen(argv[i]) - 1;
              fromfile = true;
              getfulltree = file_getfulltree;
              break;
            }
            if (strncmp("--metafirst", argv[i], 11) == 0) {
              j = strlen(argv[i]) - 1;
              metafirst = true;
              break;
            }
            if (strncmp("--gitignore", argv[i], 11) == 0) {
              j = strlen(argv[i]) - 1;
              gitignore = true;
              break;
            }
            if (strncmp("--info", argv[i], 6) == 0) {
              j = strlen(argv[i]) - 1;
              showinfo = true;
              break;
            }
          } else {
            printf("here i = %d, n = %d\n", i, n);
          }
          fprintf(stderr, "tree: Invalid argument `%s'.\n", argv[i]);
          usage(1);
          exit(1);
        }
        }
      }
    } else {
      if (dirname == NULL) {
        q = MINIT;
        dirname = (char **)xmalloc(sizeof(char *) * q);
      } else if (p == (q - 2)) {
        q += MINC;
        dirname = (char **)xrealloc(dirname, sizeof(char *) * q);
      }
      dirname[p++] = scopy(argv[i]);
    }
  }
  if (p) {
    dirname[p] = NULL;
  }

  setoutput(outfilename);

  parse_dir_colors();
  linedraw = initlinedraw(0);

  /* Insure sensible defaults and sanity check options: */
  if (dirname == NULL) {
    dirname = xmalloc(sizeof(char *) * 2);
    dirname[0] = scopy(".");
    dirname[1] = NULL;
  }
  if (topsort == NULL) {
    topsort = basesort;
  }
  if (timefmt) {
    setlocale(LC_TIME, "");
  }
  if (dflag) {
    pruneflag = false; /* You'll just get nothing otherwise. */
  }
  if (Rflag && (Level == -1)) {
    Rflag = false;
  }

  // Not going to implement git configs so no core.excludesFile support.
  if (gitignore) {
    stmp = getenv("GIT_DIR");
    if (stmp != NULL) {
      char *path = xmalloc(PATH_MAX);
      snprintf(path, PATH_MAX, "%s/info/exclude", stmp);
      push_filterstack(new_ignorefile(path));
      free(path);
    }
  }

  if (showinfo) {
    push_infostack(new_infofile(INFO_PATH));
  }

  bool needfulltree = duflag || pruneflag || matchdirs || fromfile;

  emit_tree(&lc, dirname, needfulltree);

  if (outfilename != NULL) {
    fclose(outfile);
  }

  for (int i = 0; dirname[i] != NULL; i++) {
    free(dirname[i]);
  }
  free(dirname);

  free_color_code();

  free_tables();

  free_extensions();

  free(dirs);
  free(lbuf);
  free(path);

  return errors ? 2 : 0;
}
