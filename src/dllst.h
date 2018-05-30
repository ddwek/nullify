/*
 *
 * Copyright 2018 Daniel Dwek
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
#ifndef _DLLST_H_
#define _DLLST_H_
#ifndef _HAVE_BOOLEAN_T_
#define _HAVE_BOOLEAN_T_
typedef enum { FALSE=0, TRUE } boolean_t;
#endif

#ifndef _HAVE_FIELD_TYPE_T_
#define _HAVE_FIELD_TYPE_T_
typedef enum {  F_SIGNED_CHAR_T=0, F_UNSIGNED_CHAR_T, F_SIGNED_SHORT_T, \
		F_SIGNED_SHORT_PTR_T, F_UNSIGNED_SHORT_T, F_UNSIGNED_SHORT_PTR_T, \
		F_SIGNED_INT_T, F_SIGNED_INT_PTR_T, F_UNSIGNED_INT_T, \
		F_UNSIGNED_INT_PTR_T, F_SIGNED_LONG_T, F_SIGNED_LONG_PTR_T, \
		F_UNSIGNED_LONG_T, F_UNSIGNED_LONG_PTR_T, F_SIGNED_LONG_LONG_T, \
		F_SIGNED_LONG_LONG_PTR_T, F_UNSIGNED_LONG_LONG_T, F_UNSIGNED_LONG_LONG_PTR_T, \
		F_FLOAT_T, F_FLOAT_PTR_T, F_DOUBLE_T, \
		F_DOUBLE_PTR_T, F_VOID_PTR_T, F_STRING_T \
} field_type_t;
#endif

#ifndef _HAVE_F_INFO_T_
#define _HAVE_F_INFO_T_
typedef struct f_info_st {
	field_type_t f_type;
	unsigned f_size;
	char *f_sprintf;
} f_info_t;
#endif

typedef struct dllst_item_struct {
	void *fields;
	struct dllst_item_struct *prev;
	struct dllst_item_struct *next;
} dllst_item_struct_t;

typedef struct dllst_struct {
	f_info_t **f_info;
	unsigned fields_no;
	unsigned long size;
	dllst_item_struct_t *head;
	dllst_item_struct_t *tail;
} dllst_t;

extern boolean_t dllst_verbose;

extern dllst_t *dllst_initlst (dllst_t *l, char *fields_info);
extern dllst_item_struct_t *dllst_newitem (dllst_t *l, void *fields);
extern dllst_item_struct_t *dllst_getitem(dllst_t *l, unsigned long n);
extern dllst_item_struct_t *dllst_delitem (dllst_t *l, long unsigned int n);
extern boolean_t dllst_isinlst (dllst_t *l, void *fields);
extern void dllst_swapitems(dllst_t *l, unsigned long a, unsigned long b);
extern void dllst_sortby (dllst_t *l, unsigned int field, boolean_t asc);
#endif

