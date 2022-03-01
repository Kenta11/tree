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

#ifndef JSON_H
#define JSON_H

// tree modules
#include "info.h"
#include "list.h"

void json_intro(void);
void json_outtro(void);
int json_printinfo(struct _info *file, int level);
int json_printfile(char *dirname, char *filename, struct _info *file,
                   int descend);
int json_error(char *error);
void json_newline(struct _info *file, int level, int postdir, int needcomma);
void json_close(struct _info *file, int level, int needcomma);
void json_report(struct totals tot);

#endif // JSON_H
