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
#include "list.h"

// C standard library
#include <stdlib.h>

// System library
#include <sys/stat.h>

// tree modules
#include "hash.h"
#include "html.h"
#include "tree.h"
#include "xstdlib.h"

static char errbuf[256];

static struct _info *stat2info(struct stat *st);
static struct totals listdir(struct listingcalls *lc, char *dirname,
                             struct _info **dir, int lev, dev_t dev,
                             bool hasfulltree);

static struct _info *stat2info(struct stat *st) {
  static struct _info info;

  info.linode = st->st_ino;
  info.ldev = st->st_dev;
  info.mode = st->st_mode;
  info.uid = st->st_uid;
  info.gid = st->st_gid;
  info.size = st->st_size;
  info.atime = st->st_atime;
  info.ctime = st->st_ctime;
  info.mtime = st->st_mtime;

  info.isdir = ((st->st_mode & S_IFMT) == S_IFDIR);
  info.issok = ((st->st_mode & S_IFMT) == S_IFSOCK);
  info.isfifo = ((st->st_mode & S_IFMT) == S_IFIFO);
  info.isexe = (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 1 : 0;

  return &info;
}

static struct totals listdir(struct listingcalls *lc, char *dirname,
                             struct _info **dir, int lev, dev_t dev,
                             bool hasfulltree) {
  struct totals tot = {0}, subtotal;
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **subdir;
  int descend, htmldescend = 0, n, dirlen = strlen(dirname),
               pathlen = dirlen + 257;
  int needsclosed;
  char *path, *newpath, *filename, *err = NULL;

  int es = (dirname[strlen(dirname) - 1] == '/');

  for (n = 0; dir[n] != NULL; n++)
    ;
  if (topsort) {
    qsort(dir, n, sizeof(struct _info *),
          (int (*)(const void *, const void *))topsort);
  }

  dirs[lev] = *(dir + 1) ? 1 : 2;

  path = xmalloc((sizeof *path) * pathlen);

  for (; *dir != NULL; dir++) {
    lc->printinfo(*dir, lev);

    if (es) {
      sprintf(path, "%s%s", dirname, (*dir)->name);
    } else {
      sprintf(path, "%s/%s", dirname, (*dir)->name);
    }
    if (fflag) {
      filename = path;
    } else {
      filename = (*dir)->name;
    }

    descend = 0;
    err = NULL;

    if ((*dir)->isdir) {
      tot.dirs++;

      bool found = findino((*dir)->inode, (*dir)->dev);
      if (found == 0) {
        saveino((*dir)->inode, (*dir)->dev);
      }

      if (!(xdev && dev != (*dir)->dev) &&
          (((*dir)->lnk == NULL) || (((*dir)->lnk != NULL) && lflag))) {
        descend = 1;
        newpath = path;

        if ((*dir)->lnk != NULL) {
          if (*(*dir)->lnk == '/') {
            newpath = (*dir)->lnk;
          } else {
            if (fflag && (strcmp(dirname, "/") == 0)) {
              sprintf(path, "%s%s", dirname, (*dir)->lnk);
            } else {
              sprintf(path, "%s/%s", dirname, (*dir)->lnk);
            }
          }
          if (found) {
            err = "recursive, not followed";
            descend = 0;
          }
        }

        if ((0 <= Level) && (Level < lev)) {
          if (Rflag) {
            FILE *outsave = outfile;
            char *paths[2] = {newpath, NULL},
                 *output = xmalloc(strlen(newpath) + 13);
            int *dirsave = xmalloc((sizeof *dirsave) * (lev + 2));

            memcpy(dirsave, dirs, sizeof(int) * (lev + 1));
            sprintf(output, "%s/00Tree.html", newpath);
            setoutput(output);
            emit_tree(lc, paths, hasfulltree);

            free(output);
            fclose(outfile);
            outfile = outsave;

            memcpy(dirs, dirsave, sizeof(int) * (lev + 1));
            free(dirsave);
            htmldescend = 10;
          } else {
            htmldescend = 0;
          }
          descend = 0;
        }

        if (descend != 0) {
          if (hasfulltree) {
            subdir = (*dir)->child;
            err = (*dir)->err;
          } else {
            push_files(newpath, &ig, &inf);
            subdir = read_dir(newpath, &n, inf != NULL);
            if ((subdir == NULL) && (n != 0)) {
              err = "error opening dir";
              errors++;
            }
            if ((0 < flimit) && (flimit < n)) {
              err = errbuf;
              sprintf(err, "%d entries exceeds filelimit, not opening dir", n);
              errors++;
              free_dir(subdir);
              subdir = NULL;
            }
          }
          if (subdir == NULL) {
            descend = 0;
          }
        }
      }
    } else {
      tot.files++;
    }

    needsclosed = lc->printfile(dirname, filename, *dir, descend + htmldescend);
    if (err != NULL) {
      lc->error(err);
    }

    if (descend != 0) {
      lc->newline(*dir, lev, 0, 0);

      subtotal = listdir(lc, newpath, subdir, lev + 1, dev, hasfulltree);
      tot.dirs += subtotal.dirs;
      tot.files += subtotal.files;
      tot.size += subtotal.size;
      free_dir(subdir);
    } else if (needsclosed == 0) {
      lc->newline(*dir, lev, 0, *(dir + 1) != NULL);
    }

    if (needsclosed != 0) {
      lc->close(*dir, descend ? lev : -1, *(dir + 1) != NULL);
    }

    if ((*(dir + 1) != NULL) && (*(dir + 2) == NULL)) {
      dirs[lev] = 2;
    }
    tot.size += (*dir)->size;

    if (ig != NULL) {
      pop_filterstack();
      ig = NULL;
    }
    if (inf != NULL) {
      pop_infostack();
      inf = NULL;
    }
  }

  dirs[lev] = 0;
  free(path);
  return tot;
}

/**
 * Maybe TODO: Refactor the listing calls / when they are called.  A more
 * thorough analysis of the different outputs is required.  This all is not as
 * clean as I had hoped it to be.
 */

void null_intro(void) {}

void null_outtro(void) {}

void null_close(struct _info *file, int level, int needcomma) {
  (void)file;
  (void)level;
  (void)needcomma;
}

void emit_tree(struct listingcalls *lc, char **dirname, bool needfulltree) {
  struct totals tot = {0};
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **dir = NULL, *info = NULL;

  lc->intro();

  for (int i = 0; dirname[i] != NULL; i++) {
    if (fflag) {
      int j = strlen(dirname[i]);
      do {
        if ((j > 1) && (dirname[i][j - 1] == '/')) {
          dirname[i][--j] = 0;
        }
      } while ((j > 1) && (dirname[i][j - 1] == '/'));
    }

    struct stat st;
    int n = lstat(dirname[i], &st);
    if (n >= 0) {
      saveino(st.st_ino, st.st_dev);
      info = stat2info(&st);
      info->name = dirname[i];

      if (needfulltree) {
        char *err;
        dir = getfulltree(dirname[i], 0, st.st_dev, &(info->size), &err);
        n = err ? -1 : 0;
      } else {
        push_files(dirname[i], &ig, &inf);
        dir = read_dir(dirname[i], &n, inf != NULL);
      }

      lc->printinfo(info, 0);
    } else {
      info = NULL;
    }

    int needsclosed = lc->printfile(NULL, dirname[i], info, dir != NULL);

    if ((dir == NULL) && (n != 0)) {
      lc->error("error opening dir");
      lc->newline(info, 0, 0, dirname[i + 1] != NULL);
      errors++;
    } else if ((0 < flimit) && (flimit < n)) {
      sprintf(errbuf, "%d entries exceeds filelimit, not opening dir", n);
      lc->error(errbuf);
      lc->newline(info, 0, 0, dirname[i + 1] != NULL);
      errors++;
    } else {
      lc->newline(info, 0, 0, 0);
      if (dir != NULL) {
        tot = listdir(lc, dirname[i], dir, 1, 0, needfulltree);
      } else {
        tot = (struct totals){0};
      }
    }
    if (dir != NULL) {
      free_dir(dir);
      dir = NULL;
    }
    if (needsclosed != 0) {
      lc->close(info, 0, dirname[i + 1] != NULL);
    }
    if (duflag) {
      tot.size = info->size;
    } else {
      tot.size += st.st_size;
    }

    if (ig != NULL) {
      pop_filterstack();
      ig = NULL;
    }
    if (inf != NULL) {
      pop_infostack();
      inf = NULL;
    }
  }

  if (!noreport) {
    lc->report(tot);
  }

  lc->outtro();
}
