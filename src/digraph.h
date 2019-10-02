/*
 *
 * Copyright 2018-2019 Daniel Dwek
 *
 * This file is part of nullify.
 *
 *  nullify is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nullify is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with nullify.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _DIGRAPH_H_
#define _DIGRAPH_H_
#include "dllst.h"

typedef struct {
	unsigned dim;
	unsigned **mtx;
	char **names;
} digraph_table_t;

int get_suit_selector(char *card);
digraph_table_t *digraph_create_table(dllst_t *playerlst);
void digraph_print_table(digraph_table_t *table);
void digraph_destroy_table(digraph_table_t *table);
dllst_t *digraph_get_paths(digraph_table_t *table, unsigned length, dllst_t *cond);
#endif
