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

#ifndef LIST_H
#define LIST_H

// System library
//// POSIX
#include <sys/types.h>

// tree modules
#include "bool.h"
#include "info.h"

struct totals {
  u_long files, dirs;
  off_t size;
};

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

void null_intro(void);
void null_outtro(void);
void null_close(struct _info *file, int level, int needcomma);
void emit_tree(struct listingcalls *lc, char **dirname, bool needfulltree);

#endif // LIST_H
