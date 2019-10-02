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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/dllst.h"
#include "errorcodes.h"

typedef struct {
	unsigned dim;
	unsigned **mtx;
	char **names;
} digraph_table_t;

void digraph_print_table(digraph_table_t *table);
dllst_t *digraph_get_paths(unsigned **inmtx, unsigned dim, unsigned length, dllst_t *cond);

int main(int argc, char **argv)
{
	int i, j, t, n, ret = ERR_PASS;
	const int dim = 4;
	unsigned **mtx = NULL;
	dllst_t *lst = NULL, *conds = NULL;
	dllst_item_struct_t *iter;
	struct {
		char mtx[4][4];
		int res[1024];
	} array[4] = {
			{
				{
					// The null matrix
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				},
				{
					-1
				},
			},
			{
				{
					/*
					 *
					 * 0 --------> 1
					 * ^\       _ /|
					 * | `\     /| |
					 * |   `\  /   |
					 * |     \/    |
					 * |     /\    |
					 * | |_/  _\|  |
					 * | /      `\ v
					 * 3           2
					 *
					 */

					// note that in this matrix, vertex 2
					// has two incoming connections but has
					// not outgoing connections
					{ 0, 1, 1, 0 },
					{ 0, 0, 1, 1 },
					{ 0, 0, 0, 0 },
					{ 1, 1, 0, 0 },
				},
				{
					1, 3, 0, 2,
					3, 0, 1, 2,
					0, 1, 2, -1,
					0, 1, 3, -1,
					1, 3, 0, -1,
					3, 0, 1, -1,
					3, 0, 2, -1,
					3, 1, 2, -1,
					0, 1, -1, -1,
					0, 2, -1, -1,
					1, 2, -1, -1,
					1, 3, -1, -1,
					3, 0, -1, -1,
					3, 1, -1, -1,
				},
			},
			{
				{
					// An arbitrary matrix of paths
					{ 0, 1, 1, 0 },
					{ 0, 0, 1, 0 },
					{ 0, 1, 0, 1 },
					{ 1, 0, 0, 0 },
				},
				{
					// Length = 4
					0, 1, 2, 3,
					1, 2, 3, 0,
					2, 3, 0, 1,
					3, 0, 1, 2,
					3, 0, 2, 1,
	
					// Length = 3
					0, 1, 2, -1,
					0, 2, 3, -1,
					0, 2, 1, -1,
					1, 2, 3, -1,
					2, 3, 0, -1,
					3, 0, 1, -1,
					3, 0, 2, -1,
		
					// Length = 2
					0, 1, -1, -1,
					0, 2, -1, -1,
					1, 2, -1, -1,
					2, 1, -1, -1,
					2, 3, -1, -1,
					3, 0, -1, -1,
				}
			},
			{
				{
					// 1's complemented boolean identity matrix.
					// Note that with this kind of matrices you
					// can easily build a decision tree of n!
					// permutations of a word (which can later be mapped
					// to letters, numbers, colors, etc)
					{ 0, 1, 1, 1 },
					{ 1, 0, 1, 1 },
					{ 1, 1, 0, 1 },
					{ 1, 1, 1, 0 },
				},
				{
					// Length = 4
					0,  1,  2,  3,
					0,  1,  3,  2,
					0,  2,  1,  3,
					0,  2,  3,  1,
					0,  3,  1,  2,
					0,  3,  2,  1,
					1,  0,  3,  2,
					1,  0,  2,  3,
					1,  2,  3,  0,
					1,  2,  0,  3,
					1,  3,  2,  0,
					1,  3,  0,  2,
					2,  0,  1,  3,
					2,  0,  3,  1,
					2,  1,  0,  3,
					2,  1,  3,  0,
					2,  3,  0,  1,
					2,  3,  1,  0,
					3,  0,  2,  1,
					3,  0,  1,  2,
					3,  1,  2,  0,
					3,  1,  0,  2,
					3,  2,  1,  0,
					3,  2,  0,  1,
					
					// Length = 3
					0,  1,  2, -1,
					0,  1,  3, -1,
					0,  2,  1, -1,
					0,  2,  3, -1,
					0,  3,  1, -1,
					0,  3,  2, -1,
					1,  0,  3, -1,
					1,  0,  2, -1,
					1,  2,  3, -1,
					1,  2,  0, -1,
					1,  3,  2, -1,
					1,  3,  0, -1,
					2,  0,  1, -1,
					2,  0,  3, -1,
					2,  1,  0, -1,
					2,  1,  3, -1,
					2,  3,  0, -1,
					2,  3,  1, -1,
					3,  0,  2, -1,
					3,  0,  1, -1,
					3,  1,  2, -1,
					3,  1,  0, -1,
					3,  2,  1, -1,
					3,  2,  0, -1,
					
					// Length = 2
					0,  1, -1, -1,
					0,  2, -1, -1,
					0,  3, -1, -1,
					1,  0, -1, -1,
					1,  2, -1, -1,
					1,  3, -1, -1,
					2,  0, -1, -1,
					2,  1, -1, -1,
					2,  3, -1, -1,
					3,  0, -1, -1,
					3,  1, -1, -1,
					3,  2, -1, -1,
				}
			}
	};


	dllst_verbose = FALSE;

	conds = dllst_initlst(conds, "I:");
	for (i=0;i<dim;i++)
		dllst_newitem(conds, &i);

	mtx = (unsigned **)calloc(dim, sizeof(unsigned *));
	if (!mtx)
		return -1;

	for (i=0;i<dim;i++) {
		mtx[i] = (unsigned *)calloc(dim, sizeof(unsigned));
		if (!mtx[i]) {
			free(mtx);
			return -1;
		}
	}

	for (n=0;n<4;n++) {
		for (i=0;i<dim;i++)
			for (j=0;j<dim;j++)
				mtx[i][j] = array[n].mtx[i][j];

		t = 0;
		for (i=dim;i>1;i--) {
			printf("Checking paths of length %d:\n", i);
			lst = digraph_get_paths(mtx, dim, i, conds);
			if (!lst)
				continue;
			for (iter=lst->head;iter;iter=iter->next) {
				for (j=0;j<lst->fields_no;j++) {
check_invalid_value:
					if (array[n].res[t] == -1) {
						t++;
						goto check_invalid_value;
					}
					if (*((unsigned *)iter->fields + j * 2) != array[n].res[t]) {
						ret = ERR_FAIL;
//						printf("(%d != %d)\n", *((unsigned *)iter->fields + j * 2),
//							array[n].res[t]);
					}
					printf("%3u,", *((unsigned *)iter->fields + j * 2));
					t++;
				}
				printf("\n");
			}
			while (dllst_delitem(lst, 0)) {}
			free(lst);
		}

		if (ret != ERR_FAIL)
			printf("...passed\n");

		// We can't use the least significant bit to indicate a successful test
		// because it's reserved to ERR_FAIL
		ret |= 1 << (n + 1);
	}

	for (i=0;i<dim;i++)
		free(mtx[i]);
	free(mtx);

	if (ret == 0x1e)
		ret = ERR_PASS;

	return ret;
}

/*
 *
 * Simple function to print boolean matrices as tables. Useful when
 * debugging with gdb (e.g., you can display the matrix each time a
 * breakpoint is hit using the internal command "display digraph_print_mtx(mtx, dim)")
 *
 */
static void digraph_print_mtx(unsigned **inmtx, unsigned dim)
{
	int i, j;


	if (!inmtx)
		return;

	printf("\n");
	for (i=0;i<dim;i++)
		printf("\t%d", i);
	printf("\n-------+--------------------------\n");

	for (i=0;i<dim;i++) {
		printf("%2d", i);
		for (j=0;j<dim;j++)
			printf("\t%d,", inmtx[i][j]);
		printf("\n");
	}

	printf("\n");
}

dllst_t *digraph_get_paths(unsigned **inmtx, unsigned dim, unsigned length, dllst_t *cond)
{
	int i, j, niterations, row, col;
	unsigned **mtx = NULL;
	char *lsttypes = NULL;
	dllst_t *lst = NULL;
	struct _fields_st {
		unsigned a;
		int unused0;
	} *fields = NULL;
	unsigned *savedrow = NULL;
	unsigned *nextcol = NULL;
	int *startingcol = NULL, level;
	dllst_item_struct_t *nextrow = NULL;


	// We can handle up to (2^32 - 2) dimensions because we need to use -1
	// as initial value of the starting columns
	if (dim > 0xfffffffe)
		return NULL;

	if (!inmtx || !cond || length > dim)
		return NULL;

	mtx = (unsigned **)calloc(dim, sizeof(unsigned *));
	if (!mtx)
		return NULL;

	for (i=0;i<dim;i++) {
		mtx[i] = (unsigned *)calloc(dim, sizeof(unsigned));
		if (!mtx[i]) {
			free(mtx);
			return NULL;
		}
	}

	for (i=0;i<dim;i++) {
		for (j=0;j<dim;j++) {
			mtx[i][j] = inmtx[i][j];

			// loops are not implemented yet
			if (i == j && inmtx[i][j])
				return NULL;
		}
	}

	lsttypes = (char *)calloc(1, length * 2 + 1);
	if (!lsttypes)
		return NULL;
	for (i=0;i<length;i++)
		strcat(lsttypes, "I:");
	lst = dllst_initlst(lst, lsttypes);

	fields = (struct _fields_st *)calloc(length, sizeof(struct _fields_st));
	if (!fields)
		return NULL;

	// Save on this array the next column to be processed for a given row
	nextcol = (unsigned *)calloc(dim, sizeof(unsigned));
	if (!nextcol)
		return NULL;

	// This is the first processed column of a row. Each time that the current
	// column matches against this value, a cycle is completed and level is decreased
	startingcol = (int *)calloc(dim, sizeof(int));
	if (!startingcol)
		return NULL;
	for (i=0;i<dim;i++)
		startingcol[i] = -1;

	// When a cycle ends, the current row pops from this virtual stack its
	// previous value
	savedrow = (unsigned *)calloc(dim, sizeof(unsigned));
	if (!savedrow)
		return NULL;

	// Conditionally process only those rows listed on @cond, which must be
	// a dllst with one integer field per item
	row = *((unsigned *)cond->head->fields + 0);
	nextrow = cond->head->next;
	savedrow[0] = row;
	col = 0;
	nextcol[0] = 1;
	level = 0;
	fields[0].a = row;
	niterations = 0;
	for (;;) {
		if (mtx[row][col] == 0) {

			// cycle through 0 ... dim - 1
			col = (col + 1) % dim;
			niterations++;

check_end_of_row:
                        // niterations avoids looping forever when none 1 has been encountered
                        if (col == startingcol[row] || niterations == dim) {
				if (--level == -1) {
					if (nextrow) {
						row = *((unsigned *)nextrow->fields + 0);
						nextrow = nextrow->next;
					} else {
						break;
					}

					if (row == dim)
						break;
					savedrow[0] = row;
					col = 0;
					nextcol[0] = 1;
					fields[0].a = row;
					level = 0;
					niterations = 0;

					// reset the whole array to its default values
					for (i=level;i<dim;i++)
						startingcol[i] = -1;

					goto check_end_of_row;
				}

				row = savedrow[level];
				col = nextcol[level];
				niterations = 0;

				goto check_end_of_row;
			}

			continue;
		} else {
			for (i=0;i<=level;i++) {

				// query whether 'col' has been added before
				if (fields[i].a == col) {
					if (startingcol[row] == -1)
						startingcol[row] = col;
					col = (col + 1) % dim;
					goto check_end_of_row;
				}
			}

			nextcol[level] = (col + 1) % dim;

			// set startingcol[row] to a valid value
			if (startingcol[row] == -1)
				startingcol[row] = col;
			savedrow[level] = row;
			level++;
			fields[level].a = col;

			if (level != length - 1) {
				row = col;
				col = nextcol[level];
				niterations = 0;

				// We have leveled up and therefore we must to reset the
				// starting column of the new row
				startingcol[row] = -1;
				continue;
			}

			dllst_newitem(lst, fields);
			level--;
			row = savedrow[level];
			col = nextcol[level];
			niterations = 0;

			goto check_end_of_row;
		}
	}

	// clean up variables
	for (i=0;i<dim;i++)
		free(mtx[i]);
	free(mtx);
	free(lsttypes);
	free(fields);
	free(savedrow);
	free(nextcol);
	free(startingcol);

	return lst;
}

