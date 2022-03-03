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

#ifndef COLOR_H
#define COLOR_H

// C standard library
#include <stdbool.h>

extern bool colorize, linktargetcolor;

void parse_dir_colors(void);
bool color(unsigned short mode, char *name, bool orphan, bool islink);
void endcolor(void);
const struct linedraw *initlinedraw(int flag);
void free_color_code(void);
void free_extensions(void);

#endif // COLOR_H
