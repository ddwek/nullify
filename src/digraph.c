/*
 * digraph.c: routines to handle oriented graphs (digraphs)
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

/*
 *
 *  My terminology employed can confuse you a little. I use "paths" whereas
 *  in some texts you will find it as "roads" or whatever. Also, matrices
 *  of paths can sometimes be called as "reachability matrices". When in
 *  doubt, please keep a good book of Discrete Mathematics applied to
 *  Computer Sciences close to your hands.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllst.h"
#include "digraph.h"

#define CARD_ACE(x)             (x * 13 + 0)
#define CARD_TWO(x)             (x * 13 + 1)
#define CARD_FOUR(x)            (x * 13 + 3)
#define CARD_SEVEN(x)           (x * 13 + 6)
#define CARD_JACK(x)            (x * 13 + 10)
#define CARD_QUEEN(x)           (x * 13 + 11)
#define CARD_KING(x)            (x * 13 + 12)
#define CARD_SUIT(x)            (*((unsigned int *)x->fields + 0))
#define CARD_NUMBER(x)          (*((unsigned int *)x->fields + 2))
#define SUIT_CLUBS              0
#define SUIT_DIAMONDS           1
#define SUIT_HEARTS             2
#define SUIT_SPADES             3

extern dllst_t *played_list;
extern int lastsuit;

extern int getactiveplayers(void);
extern char *decode_card(int suit, int number, boolean_t addnode);

/*
 *
 * Simple function to print boolean matrices as tables. Useful when
 * debugging with gdb (e.g., you can display the matrix each time a
 * breakpoint is hit using the internal command
 * "display digraph_print_mtx(mtx, dim)")
 *
 */
void digraph_print_table(digraph_table_t *table)
{
	int i, j;


	if (!table)
		return;

	printf("\n    |");
	for (i=0;i<table->dim;i++) {
		printf("%s", table->names[i]);
		if (strlen(table->names[i]) == 3)
			printf("  ");
	}
	printf("\n----+");
	for (i=0;i<table->dim;i++)
		printf("-----");
	printf("\n");

	for (i=0;i<table->dim;i++) {
		printf("%s", table->names[i]);
		if (strlen(table->names[i]) == 3)
			printf(" |");

		for (j=0;j<table->dim;j++)
			printf(" %3u ", table->mtx[i][j]);
		printf("\n");
	}

	printf("\n");
}

/*
 *
 * Get the suit selector of a jack (e.g., if @card is " Jc:d", the
 * new suit to play will be diamonds)
 *
 */
int get_suit_selector(char *card)
{
	int i, ret = 0;


	for (i=0;i<strlen(card);i++) {
		if (card[i] == ':' && card[i + 1] != '\0') {
			switch (card[i + 1]) {
			case 'c':
				ret = SUIT_CLUBS;
				break;
			case 'd':
				ret = SUIT_DIAMONDS;
				break;
			case 'h':
				ret = SUIT_HEARTS;
				break;
			case 's':
				ret = SUIT_SPADES;
				break;
			}
		}
	}

	return ret;
}

/*
 *
 * Create a digraph_table_t structure from the doubly-linked list @playerlst.
 * The returned table will contain the reachability matrix with its corresponding
 * names for each column, which in turn can be used as input parameter to
 * digraph_get_paths().
 *
 * This function is only meaningful for this game. If you are using digraphs in
 * your project, you will have to write your own routine to build a table in a
 * format that can be accepted by digraph_get_paths().
 *
 */
digraph_table_t *digraph_create_table(dllst_t *playerlst)
{
	int i, j, t = 0;
	digraph_table_t *table = NULL;
	dllst_t *lst = NULL, *lst2 = NULL;
	dllst_item_struct_t *iter, *next;
	struct {
		unsigned suit;
		int unused0;
		unsigned number;
		int unused1;
	} fields = { 0 };
	char *dcstr = NULL, buf[6] = { '\0' };
	char suitletter[] = "cdhs";


	if (!playerlst)
		return NULL;

	// determine how many jacks the player has
	for (iter=playerlst->head;iter;iter=iter->next)
		if (CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13)
			t++;

	table = (digraph_table_t *)calloc(1, sizeof(digraph_table_t));
	if (!table)
		return NULL;

	// 'table' will have a squared matrix of size = table->dim
	table->dim = playerlst->size + 1 - t + t * 4;

	table->mtx = (unsigned **)calloc(table->dim, sizeof(unsigned *));
	if (!table->mtx)
		return NULL;

	for (i=0;i<table->dim;i++) {
		table->mtx[i] = (unsigned *)calloc(table->dim, sizeof(unsigned));
		if (!table->mtx[i]) {
			free(table->mtx);
			return NULL;
		}
	}

	table->names = (char **)calloc(table->dim, sizeof(char *));
	if (!table->names)
		return NULL;

	for (i=0;i<table->dim;i++) {
		table->names[i] = (char *)calloc(1, 6);
		if (!table->names[i]) {
			free(table->names);
			return NULL;
		}
	}

	// We can't work with the input list since we need to add the last card played
	// as the first item of this copy list
	lst2 = dllst_initlst(lst2, "I:I:");
	fields.suit = CARD_SUIT(played_list->tail);
	fields.number = CARD_NUMBER(played_list->tail);
	dllst_newitem(lst2, &fields);
	for (iter=playerlst->head;iter;iter=iter->next) {
		fields.suit = CARD_SUIT(iter);
		fields.number = CARD_NUMBER(iter);
		dllst_newitem(lst2, &fields);
	}

	// This will be the definitive list. It contains the last card played, as well as
	// every single card that is not a jack and four entries for each jack in the player list.
	lst = dllst_initlst(lst, "I:I:");
	for (i=0,iter=lst2->head;iter;iter=iter->next,i++) {
		dcstr = decode_card(CARD_SUIT(iter), CARD_NUMBER(iter), FALSE);

		if (CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13 && iter != lst2->head) {
			for (j=0;j<4;j++) {
				sprintf(buf, "%s:%c", dcstr, suitletter[j]);
				strcpy(table->names[i + j], buf);

				fields.suit = CARD_SUIT(iter);
				fields.number = CARD_NUMBER(iter);
				dllst_newitem(lst, &fields);
			}
			i += 3;
		} else {
			strcpy(table->names[i], dcstr);

			fields.suit = CARD_SUIT(iter);
			fields.number = CARD_NUMBER(iter);
			dllst_newitem(lst, &fields);
		}
	}

	for (i=0,iter=lst->head;iter;iter=iter->next,i++) {
		for (j=0,next=lst->head;next;next=next->next,j++) {

			// The first column of the matrix must be completely unset
			// because it corresponds to the last card played, not to the player
			if (!j)
				continue;

			if (i == j) {
				table->mtx[i][j] = 0;
			} else {

				// Matches a 4 as the first card to play and either a 4 or a card
				// of the same suit as the second one
				if ((CARD_NUMBER(iter) == CARD_FOUR(CARD_SUIT(iter)) % 13 &&
				     CARD_NUMBER(next) == CARD_FOUR(CARD_SUIT(next)) % 13 ) ||
				    (CARD_NUMBER(iter) == CARD_FOUR(CARD_SUIT(iter)) % 13 &&
				     CARD_SUIT(iter) == CARD_SUIT(next)))
					table->mtx[i][j] = 1;

				// Matches a 7 as the first card to play and either a 7 or a card
				// of the same suit as the second one
				else if ((CARD_NUMBER(iter) == CARD_SEVEN(CARD_SUIT(iter)) % 13 &&
					  CARD_NUMBER(next) == CARD_SEVEN(CARD_SUIT(next)) % 13 ) ||
					 (CARD_NUMBER(iter) == CARD_SEVEN(CARD_SUIT(iter)) % 13 &&
					  CARD_SUIT(iter) == CARD_SUIT(next)))
						table->mtx[i][j] = 1;

				// Matches a Q as the first card to play and either a Q or a card
				// of the same suit as the second one, as long as the number of
				// active players is 2
				else if ((CARD_NUMBER(iter) == CARD_QUEEN(CARD_SUIT(iter)) % 13 &&
					  CARD_NUMBER(next) == CARD_QUEEN(CARD_SUIT(next)) % 13 &&
					  getactiveplayers() == 2) ||
					 (CARD_NUMBER(iter) == CARD_QUEEN(CARD_SUIT(iter)) % 13 &&
					  CARD_SUIT(iter) == CARD_SUIT(next) &&
					  getactiveplayers() == 2))
						table->mtx[i][j] = 1;

				// Matches a J as the first card to play and either a J or a card
				// of the last suit selected (i.e., the topmost card in the stack is
				// a jack but another suit was chosen). Additionally, it matches
				// against every possible suit to select.
				else if ((CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13 &&
					  CARD_NUMBER(next) == CARD_JACK(CARD_SUIT(next)) % 13) ||
					 (CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13 && !i &&
					  CARD_SUIT(next) == lastsuit) ||
					 (CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13 &&
					  get_suit_selector(table->names[i]) == CARD_SUIT(next)))
						table->mtx[i][j] = 1;

				// Matches this card against the last card played (top of the stack)
				// iff we are processing the first row
				else if (!i && (CARD_SUIT(played_list->tail) == CARD_SUIT(next) ||
					 CARD_NUMBER(played_list->tail) == CARD_NUMBER(next)))
						table->mtx[i][j] = 1;

				// Cards other than 4, 7, J and Q have no special behaviors
				else
					table->mtx[i][j] = 0;
			}

			if (CARD_NUMBER(iter) == CARD_JACK(CARD_SUIT(iter)) % 13 &&
			    CARD_NUMBER(iter) == CARD_NUMBER(next) && CARD_SUIT(iter) == CARD_SUIT(next))
				table->mtx[i][j] = 0;
		}
	}

	while (dllst_delitem(lst2, 0)) {}
	free(lst2);
	while (dllst_delitem(lst, 0)) {}
	free(lst);

	return table;
}

/*
 *
 * Free bytes allocated by digraph_create_table() to hold the table
 *
 */
void digraph_destroy_table(digraph_table_t *table)
{
	int i;


	for (i=0;i<table->dim;i++) {
		free(table->mtx[i]);
		free(table->names[i]);
	}

	free(table->mtx);
	free(table->names);
	free(table);
	table = NULL;
}

/*
 *
 * Get a list of all the possible paths from each starting row listed on @cond
 * up to a valid column (vertex), as long as the path length is less than
 * @length.
 *
 * Loops over a given vertex, multiple connections from vertex A to B and vice versa,
 * weighted roads based on probabilities, Eulerian and Hamiltonian paths are
 * current limitations but they could be added in future releases, if needed.
 *
 */
dllst_t *digraph_get_paths(digraph_table_t *table, unsigned length, dllst_t *cond)
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
	dllst_item_struct_t *nextrow;
	boolean_t samejack = FALSE;


	if (!table || !cond || length > table->dim)
		return NULL;

	// We can handle up to (2^32 - 2) dimensions because we need to use -1
	// as initial value of the starting columns
	if (table->dim > 0xfffffffe)
		return NULL;

	mtx = (unsigned **)calloc(table->dim, sizeof(unsigned *));
	if (!mtx)
		return NULL;

	for (i=0;i<table->dim;i++) {
		mtx[i] = (unsigned *)calloc(table->dim, sizeof(unsigned));
		if (!mtx[i]) {
			free(mtx);
			return NULL;
		}
	}

	for (i=0;i<table->dim;i++) {
		for (j=0;j<table->dim;j++) {
			mtx[i][j] = table->mtx[i][j];

			// loops are not implemented yet
			if (i == j && table->mtx[i][j])
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
	nextcol = (unsigned *)calloc(table->dim, sizeof(unsigned));
	if (!nextcol)
		return NULL;

	// This is the first processed column of a row. Each time that the current
	// column matches against this value, a cycle is completed and level is decreased
	startingcol = (int *)calloc(table->dim, sizeof(int));
	if (!startingcol)
		return NULL;
	for (i=0;i<table->dim;i++)
		startingcol[i] = -1;

	// When a cycle ends, the current row pops from this virtual stack its
	// previous value
	savedrow = (unsigned *)calloc(table->dim, sizeof(unsigned));
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

			// cycle through 0 ... table->dim - 1
			col = (col + 1) % table->dim;
			niterations++;

check_end_of_row:
			// niterations avoids looping forever when none 1 has been encountered
			if (col == startingcol[row] || niterations == table->dim) {
				if (--level == -1) {
					if (nextrow) {
						row = *((unsigned *)nextrow->fields + 0);
						nextrow = nextrow->next;
					} else {
						break;
					}

					if (row == table->dim)
						break;
					savedrow[0] = row;
					col = 0;
					nextcol[0] = 1;
					fields[0].a = row;
					level = 0;
					niterations = 0;

					// reset the whole array to its default values
					for (i=level;i<table->dim;i++)
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
				for (j=0;j<=level;j++) {

					// It prevents the same jack from being added regardless of its
					// suit selector. This is not really needed to deal with the
					// vast majority of the use cases, it's just for this game
					if (!strncmp(table->names[fields[j].a], table->names[col], 3))
						samejack = TRUE;
				}

				// query whether 'col' has been added before
				if (fields[i].a == col || samejack) {
					if (startingcol[row] == -1)
						startingcol[row] = col;
					col = (col + 1) % table->dim;
					samejack = FALSE;
					goto check_end_of_row;
				}
			}

			nextcol[level] = (col + 1) % table->dim;

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
				samejack = FALSE;

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
			samejack = FALSE;

			goto check_end_of_row;
		}
	}

	// clean up variables
	for (i=0;i<table->dim;i++)
		free(mtx[i]);
	free(mtx);
	free(lsttypes);
	free(fields);
	free(savedrow);
	free(nextcol);
	free(startingcol);

	return lst;
}

