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
#include <string.h>
#include <stdlib.h>
#include "errorcodes.h"
#include "../src/dllst.h"

#define GET_FIELD_A(iter)	((char *)*(char **)iter->fields + 0)
#define GET_FIELD_B(iter)	((short)*((short *)iter->fields + 4))
#define GET_FIELD_C(iter)	((double)*((double *)iter->fields + 2))
#define GET_FIELD_D(iter)	((int)*((int *)iter->fields + 6))

char *mystrings[] = {
			"dllst_initlst() initializes a list",
			"dllst_newitem() adds a new item to the list",
			"dllst_getitem() gets the nth item from the list",
			"dllst_isinlst() queries whether an item is on the list",
			"dllst_sortby() sorts a list by the specified field",
			"dllst_delitem() deletes an item from the list"
};
struct {		// offsets:
	char *a;	// 0
	short b;	// 8
	short unused1;
	int unused2;
	double c;	// 16
	int d;		// 24
	int unused3;
} fields = { 0 };


int main(int argc, char **argv)
{
	int j = 0, ret = 0;
	dllst_t *dllst = NULL;
	dllst_item_struct_t *iter, *iter2;


	dllst_verbose = FALSE;
	dllst = dllst_initlst(dllst, "t:s:d:i:");
	for (j=0;j<6;j++) {
		fields.a = mystrings[j];
		fields.b = j;
		fields.c = (double)(j * 2.0);
		fields.d = j * 4;
		dllst_newitem(dllst, &fields);
	}

	printf("Checking whether dllst_delitem() deletes the list head... ");
	iter = dllst_getitem(dllst, 1);
	dllst_delitem(dllst, 0);

	// After deletion, the second item becomes the new list head
	if (dllst->head == iter) {
		printf("yes\n");

		// We can't use the least significant bit to indicate a successful test
                // because it's reserved to ERR_FAIL
		ret |= 1 << 1;
	} else {
		printf("no\n");
		ret = ERR_FAIL;
	}

	printf("Checking whether dllst_delitem() deletes an arbitrary item... ");
	iter = dllst_getitem(dllst, dllst->size / 2 + 1);
	iter2 = dllst_delitem(dllst, dllst->size / 2);

	// dllst_delitem() returns the previous node from being deleted
	if (iter2->next == iter) {
		printf("yes\n");
		ret |= 1 << 2;
	} else {
		printf("no\n");
		ret = ERR_FAIL;
	}

	printf("Checking whether dllst_delitem() deletes an item beyond its list size... ");
	iter = dllst->tail;
	iter2 = dllst_delitem(dllst, dllst->size);

	// it should never occur
	if (iter == iter2) {
		printf("yes\n");
		ret = ERR_FAIL;
	} else {
		printf("no\n");
		ret |= 1 << 3;
	}

	if (ret == 0xe)
		ret = ERR_PASS;

	return ret;
}

