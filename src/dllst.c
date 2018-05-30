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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum { FALSE=0, TRUE } boolean_t;
typedef enum { 	F_SIGNED_CHAR_T=0, F_UNSIGNED_CHAR_T, F_SIGNED_SHORT_T, \
		F_SIGNED_SHORT_PTR_T, F_UNSIGNED_SHORT_T, F_UNSIGNED_SHORT_PTR_T, \
		F_SIGNED_INT_T, F_SIGNED_INT_PTR_T, F_UNSIGNED_INT_T, \
		F_UNSIGNED_INT_PTR_T, F_SIGNED_LONG_T, F_SIGNED_LONG_PTR_T, \
		F_UNSIGNED_LONG_T, F_UNSIGNED_LONG_PTR_T, F_SIGNED_LONG_LONG_T, \
		F_SIGNED_LONG_LONG_PTR_T, F_UNSIGNED_LONG_LONG_T, F_UNSIGNED_LONG_LONG_PTR_T, \
		F_FLOAT_T, F_FLOAT_PTR_T, F_DOUBLE_T, \
		F_DOUBLE_PTR_T, F_VOID_PTR_T, F_STRING_T \
} field_type_t;

typedef union {
	long long a;
	double b;
} p2align_t;

typedef struct dllst_item_struct {
#if DEBUG_DLLST_SORTROUTINE
	unsigned n;
#endif
	void *fields;
	struct dllst_item_struct *prev;
	struct dllst_item_struct *next;
} dllst_item_struct_t;

typedef struct dllst_struct {
	struct f_info_st {
		field_type_t f_type;
		unsigned f_size;
		char *f_sprintf;
	} **f_info;
	unsigned fields_no;
	unsigned long size;
	dllst_item_struct_t *head;
	dllst_item_struct_t *tail;
} dllst_t;

boolean_t dllst_verbose = TRUE;

static void dbginfo_print(char *);
static unsigned long dllst_findmin(dllst_t *l, void *, unsigned, unsigned long, boolean_t);
static unsigned long dllst_findmax(dllst_t *l, void *, unsigned, unsigned long, boolean_t);
dllst_t *dllst_initlst(dllst_t *l, char *fields_info);
dllst_item_struct_t *dllst_newitem(dllst_t *l, void *fields);
dllst_item_struct_t *dllst_delitem(dllst_t *l, unsigned long n);
boolean_t dllst_isinlst(dllst_t *l, void *fields);
void dllst_swapitems(dllst_t *l, unsigned long a, unsigned long b);
void dllst_sortby(dllst_t *l, unsigned field, boolean_t asc);



static void dbginfo_print(char *s)
{
	if (dllst_verbose)
		printf("%s\n", s);
}

/**
 *
 * dllst_initlst() -	Create a new double-linked list
 * @l:			Pointer to the list that will be created (must be NULL).
 * @fields_info:	A per colon-separated (':') string containing appropriated
 *			data type for each field in the list.
 *
 * This function recognizes the following letters as basic C data type indicator:
 *
 * 'c'	char
 * 'C'	unsigned char
 * 's'	short
 * 'S'	unsigned short
 * 'i'	int
 * 'I'	unsigned int
 * 'l'	long
 * 'L'	unsigned long
 * '3'	long long (i.e., it stands for -63-bit ... +63-bit - 1)
 * '4'	unsigned long long (i.e., it stands for 0 ... +64-bit - 1)
 * 'f'	single precision 'float' (32-bit)
 * 'd'	double precision 'double' (64-bit)
 * 'v'	pointer to void (void *)
 * 't'	a string (char *)
 * '*'	pointer to the type defined by the previous letter, except for
 *	'c', 'C', 'v' and 't'.
 *
 * There should not be any mismatch between the types given in @fields_info and
 * their corresponding fields in subsequent calls to 'dllst_newitem'. Otherwise,
 * functions such as 'dllst_isinlst', 'dllst_sortby' and 'dllst_sprintf' might
 * not work as you can expect (i.e., most likely the program being run exits by
 * raising a segmentation fault signal, double free, or so).
 *
 */
dllst_t *dllst_initlst(dllst_t *l, char *fields_info)
{
	int i = 0, j = 0, t = -1;
	char debug[128] = { '\0' };

	if (!fields_info)
		return NULL;

	if (!l) {
		l = (dllst_t *)malloc(sizeof(dllst_t));
		if (l) {
			for (i=0;i<strlen(fields_info);i++)
				if (fields_info[i] == ':')
					j++;

			l->fields_no = j;

			l->f_info = (struct f_info_st **)calloc(l->fields_no, sizeof(struct f_info_st *));
			if (!l->f_info)
				goto oom_return;

			for (i=0;i<l->fields_no;i++) {
				l->f_info[i] = (struct f_info_st *)calloc(1, sizeof(struct f_info_st));
				if (!l->f_info[i])
					goto oom_return;
			}

			i = -1;
next_field:
			t++;
			i++;
#define ADD_CASE_F_INFO(letter, type, str_sprintf) \
case letter: \
	*&(l->f_info[t]->f_type) = type; \
	*&(l->f_info[t]->f_size) = sizeof(p2align_t); \
	l->f_info[t]->f_sprintf  = (char *)calloc(1, strlen(str_sprintf) + 1); \
	if (!l->f_info[t]->f_sprintf) \
		goto oom_return; \
	strcpy(l->f_info[t]->f_sprintf, str_sprintf); \
	break;
			while (i < strlen(fields_info) && fields_info[i] != ':') {
				switch (fields_info[i]) {
				ADD_CASE_F_INFO('c', F_SIGNED_CHAR_T, "%hhd")
				ADD_CASE_F_INFO('C', F_UNSIGNED_CHAR_T, "%hhu")
				ADD_CASE_F_INFO('s', F_SIGNED_SHORT_T, "%hd")
				ADD_CASE_F_INFO('S', F_UNSIGNED_SHORT_T, "%hu")
				ADD_CASE_F_INFO('i', F_SIGNED_INT_T, "%d")
				ADD_CASE_F_INFO('I', F_UNSIGNED_INT_T, "%u")
				ADD_CASE_F_INFO('l', F_SIGNED_LONG_T, "%ld")
				ADD_CASE_F_INFO('L', F_UNSIGNED_LONG_T, "%lu")
				ADD_CASE_F_INFO('3', F_SIGNED_LONG_LONG_T, "%lld")
				ADD_CASE_F_INFO('4', F_UNSIGNED_LONG_LONG_T, "%llu")
				ADD_CASE_F_INFO('f', F_FLOAT_T, "%f")
				ADD_CASE_F_INFO('d', F_DOUBLE_T, "%f")
				ADD_CASE_F_INFO('v', F_VOID_PTR_T, "%p")
				ADD_CASE_F_INFO('t', F_STRING_T, "%s")
				case '*':
					(*&(l->f_info[t]->f_type))++;
					*&(l->f_info[t]->f_size) = sizeof(p2align_t);
					l->f_info[t]->f_sprintf = (char *)calloc(1, strlen("%p") + 1);
					if (!l->f_info[t]->f_sprintf)
						goto oom_return;

					strcpy(l->f_info[t]->f_sprintf, "%p");
					break;
				};

				i++;
			}

			if (i < strlen(fields_info))
				goto next_field;

			dbginfo_print("n\ttype\tsize\tsprintf");
			for (i=0;i<t;i++) {
				sprintf(debug, "%d\t%d\t%u\t\"%s\"", i, \
					l->f_info[i]->f_type, \
					l->f_info[i]->f_size, \
					l->f_info[i]->f_sprintf);
				dbginfo_print(debug);
			}

			l->size = 0;
			l->head = NULL;
			l->tail = NULL;
			dbginfo_print("New list successfully created");
			return l;
		} else {
oom_return:
			dbginfo_print("No memory available");
			return NULL;
		}
	} else {
		dbginfo_print("INFO: List already initialized");
		return l;
	}

#undef ADD_CASE_F_INFO
}

/**
 *
 * dllst_newitem() -	Add a new item to the list
 * @l:			Pointer to the list
 * @fields		Pointer to any 8-bytes padded user-defined structure,
 *			where the fields are copied from.
 *
 * In order to get the right results, you must pass onto @fields a valid 8-byte
 * padded struct, with each unused member in that structure set to NULL. If you
 * don't, then the behavior of the program is unpredictable.
 *
 */
dllst_item_struct_t *dllst_newitem(dllst_t *l, void *fields)
{
	int i, j = 0;
	dllst_item_struct_t *item = NULL, *prev = NULL;

	if (l) {
		prev = l->tail;
		item = prev ? l->tail->next : l->tail;

		item = (dllst_item_struct_t *)calloc(1, sizeof(dllst_item_struct_t));
		if (item) {
			for (i=0;i<l->fields_no;i++)
				j += l->f_info[i]->f_size;

#define ADD_CASE_F_VALUE(type) \
case type: \
	*((p2align_t *)item->fields + i) = *((p2align_t *)fields + i); \
	break;
			item->fields = (void *)calloc(1, j);
			if (item->fields) {
				for (i=0;i<l->fields_no;i++) {
					switch (l->f_info[i]->f_type) {
					ADD_CASE_F_VALUE(F_SIGNED_CHAR_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_CHAR_T)
					ADD_CASE_F_VALUE(F_SIGNED_SHORT_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_SHORT_T)
					ADD_CASE_F_VALUE(F_SIGNED_INT_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_INT_T)
					ADD_CASE_F_VALUE(F_SIGNED_LONG_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_LONG_T)
					ADD_CASE_F_VALUE(F_SIGNED_LONG_LONG_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_LONG_LONG_T)
					ADD_CASE_F_VALUE(F_FLOAT_T)
					ADD_CASE_F_VALUE(F_DOUBLE_T)
					ADD_CASE_F_VALUE(F_STRING_T)
					ADD_CASE_F_VALUE(F_SIGNED_SHORT_PTR_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_SHORT_PTR_T)
					ADD_CASE_F_VALUE(F_SIGNED_INT_PTR_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_INT_PTR_T)
					ADD_CASE_F_VALUE(F_SIGNED_LONG_PTR_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_LONG_PTR_T)
					ADD_CASE_F_VALUE(F_SIGNED_LONG_LONG_PTR_T)
					ADD_CASE_F_VALUE(F_UNSIGNED_LONG_LONG_PTR_T)
					ADD_CASE_F_VALUE(F_FLOAT_PTR_T)
					ADD_CASE_F_VALUE(F_DOUBLE_PTR_T)
					ADD_CASE_F_VALUE(F_VOID_PTR_T)
					};
				}
			} else {
				dbginfo_print("No memory available");
				return NULL;
			}

			item->next = NULL;
			if (!l->size) {
				l->head = item;
				item->prev = NULL;
				dbginfo_print("First item successfully added");
			} else {
				prev->next = item;
				item->prev = prev;
				dbginfo_print("New item successfully added");
			}

			l->tail = item;
#if DEBUG_DLLST_SORTROUTINE
			item->n = l->size;
#endif
			l->size++;
			return item;
		} else {
			dbginfo_print("No memory available");
			return NULL;
		}
	} else {
		dbginfo_print("Nothing done");
		return NULL;
	}

#undef ADD_CASE_F_VALUE
}

dllst_item_struct_t *dllst_getitem(dllst_t *l, unsigned long n)
{
        long i;
        char str[128] = { '\0' };
        dllst_item_struct_t *item = NULL;

        if (l) {
                if (!n)
                        return l->head;

                if (n < l->size) {
                        // Choose the shortest path from <head | tail> until 'n'
                        if ((l->size - 1) - n > n) {
                                sprintf(str, "Iterating over the list from head until %lu (size = %lu)", n, l->size);
                                dbginfo_print(str);
                                item = l->head;
                                for (i=0;i<n;i++)
                                        item = item->next;
                        } else {
                                sprintf(str, "Iterating over the list from tail (%lu) until %lu", l->size - 1, n);
                                dbginfo_print(str);
                                item = l->tail;
                                for (i=(l->size - 1);i>n;i--)
                                        item = item->prev;
                        }
                        return item;
                } else {
                        dbginfo_print("Requested item is out of range");
                        return NULL;
                }
        }

        dbginfo_print("Nothing done");
        return NULL;
}

dllst_item_struct_t *dllst_delitem(dllst_t *l, unsigned long n)
{
	char ch[64] = { '\0' };
	dllst_item_struct_t *item = NULL, *prev = NULL;

	if (l) {
		if (!n) {
			item = l->head;
			if (item) {
				l->size--;
				l->head = item->next;

				// warn if list is empty
				if (l->head)
					l->head->prev = NULL;
				else
					l->tail = NULL;

				free(item->fields);
				item->fields = NULL;
				free(item);
				item = NULL;
				sprintf(ch, "1st item deleted, head is %p", l->head);
				dbginfo_print(ch);
				return l->head;
			}
		} else if (n < l->size) {
			prev = dllst_getitem(l, n - 1);
			item = prev->next;
			if (prev && item) {
				if (n != l->size - 1) {
					prev->next = item->next;
					item->next->prev = item->prev;
				} else {
					prev->next = NULL;
					l->tail = prev;
				}
				free(item->fields);
				item->fields = NULL;
				free(item);
				item = NULL;
				l->size--;
				sprintf(ch, "%lu-th item deleted", n);
				dbginfo_print(ch);
				return prev;
			}
		} else {
			sprintf(ch, "I have only %lu items, not %lu", l->size, n + 1);
			dbginfo_print(ch);
			return NULL;
		}
	}

	dbginfo_print("Nothing done");
	return NULL;
}

/**
 *
 * dllst_isinlst() -	Determine wether or not @fields are on the list
 * @l:			Pointer to the list
 * @fields:		A structure containing the fields that the caller function
 *			query for existence in the list
 *
 * Because of we don't know at compile-time if the program that is linked against
 * this library stores in @fields pointers to pointers, only basic data types are
 * meaningful when comparing such an argument with 'item->fields'.
 *
 */
boolean_t dllst_isinlst(dllst_t *l, void *fields)
{
	int i;
	dllst_item_struct_t *item = NULL;
	char *string0 = NULL, *string1 = NULL;
	float  eps0 = 1.40129846432481707092372958328991613128e-7;
	double eps1 = 4.94065645841246544176568792868221372365e-16;

#define ADD_CASE_FIELDS_COMPARISON(f_type, f_type_p) \
case f_type: \
	if (*((f_type_p *)fields + i * sizeof(p2align_t) / sizeof(f_type_p)) != \
	    *((f_type_p *)item->fields + i * sizeof(p2align_t) / sizeof(f_type_p))) \
		goto next_item; \
	break;

#define ADD_CASE_FP_COMPARISON(f_type, f_type_p, epsilon) \
case f_type: \
	if (!(*((f_type_p *)fields + i * sizeof(p2align_t) / sizeof(f_type_p)) > \
	    *((f_type_p *)item->fields + i * sizeof(p2align_t) / sizeof(f_type_p)) - epsilon && \
	    *((f_type_p *)fields + i * sizeof(p2align_t) / sizeof(f_type_p)) < \
	    *((f_type_p *)item->fields + i * sizeof(p2align_t) / sizeof(f_type_p)) + epsilon)) \
		goto next_item; \
	break;

	item = l->head;
	while (item) {
		for (i=0;i<l->fields_no;i++) {
			switch (l->f_info[i]->f_type) {
			ADD_CASE_FIELDS_COMPARISON(F_SIGNED_CHAR_T, char)
			ADD_CASE_FIELDS_COMPARISON(F_UNSIGNED_CHAR_T, unsigned char)
			ADD_CASE_FIELDS_COMPARISON(F_SIGNED_SHORT_T, short)
			ADD_CASE_FIELDS_COMPARISON(F_UNSIGNED_SHORT_T, unsigned short)
			ADD_CASE_FIELDS_COMPARISON(F_SIGNED_INT_T, int)
			ADD_CASE_FIELDS_COMPARISON(F_UNSIGNED_INT_T, unsigned int)
			ADD_CASE_FIELDS_COMPARISON(F_SIGNED_LONG_T, long)
			ADD_CASE_FIELDS_COMPARISON(F_UNSIGNED_LONG_T, unsigned long)
			ADD_CASE_FIELDS_COMPARISON(F_SIGNED_LONG_LONG_T, long long)
			ADD_CASE_FIELDS_COMPARISON(F_UNSIGNED_LONG_LONG_T, unsigned long long)
			ADD_CASE_FP_COMPARISON(F_FLOAT_T, float, eps0)
			ADD_CASE_FP_COMPARISON(F_DOUBLE_T, double, eps1)
			case F_STRING_T:
				string0 = *((char **)item->fields + i * sizeof(p2align_t) / sizeof(char *));
				string1 = *((char **)fields + i * sizeof(p2align_t) / sizeof(char *));
				if (strcmp(string0, string1))
					goto next_item;

				break;
			};
		}

		return TRUE;

next_item:
		item = item->next;
	}

	return FALSE;

#undef ADD_CASE_FP_COMPARISON
#undef ADD_CASE_FIELDS_COMPARISON
}

void dllst_swapitems(dllst_t *l, unsigned long a, unsigned long b)
{
	unsigned long i;
	dllst_item_struct_t *item, *preitem;
	dllst_item_struct_t *next, *prenext;
	dllst_item_struct_t *temp;


	if (a != b) {
		if (b < a) {
			i = b;
			b = a;
			a = i;
		}

		item    = dllst_getitem(l, a);
		preitem = item->prev;
		next    = dllst_getitem(l, b);
		prenext = next->prev;

		if (l->head == item)
			l->head = next;

		if (preitem)
			preitem->next = next;

		temp = item->next;
		item->next = next->next;
		next->next = (b - a) > 1 ? temp : item;
		temp = item->prev;
		item->prev = next->prev;
		next->prev = (b - a) > 1 ? temp : preitem;

		if (next->next)
			next->next->prev = next;
		if (item->next)
			item->next->prev = item;

		if (prenext && prenext != item)
			prenext->next = item;

		if (b == l->size - 1) {
			item->next = NULL;
			l->tail = item;
		}
	}
}

/*
 *
 * TODO: Add support for list ordering based on multiple comparison criteria
 * (e.g., sort by x (asc), then by y (des), and so on)
 *
 */
void dllst_sortby(dllst_t *l, unsigned field, boolean_t asc)
{
	dllst_item_struct_t *item = NULL, *preitem = NULL;
	dllst_item_struct_t *next = NULL, *prenext = NULL, *temp = NULL;
	unsigned long (*ascdes)(dllst_t *, void *, unsigned, unsigned long, boolean_t) = asc ? dllst_findmin : dllst_findmax;
	unsigned long i = 0, j = 0, m = 0;
#if DEBUG_DLLST_SORTROUTINE
	dllst_item_struct_t *debug = NULL;
	unsigned n;
	char res[256] = { '\0' };
#endif

	if (l && l->size > 1) {
		while (i < l->size) {
			item = dllst_getitem(l, i);
			while (item) {
				m = ascdes(l, item->fields, field, j, j - i ? FALSE : TRUE);
				j++;
				item = item->next;
			}

			if (m != i) {
				item    = dllst_getitem(l, i);
				preitem = item->prev;
				next    = dllst_getitem(l, m);
				prenext = next->prev;

				if (l->head == item)
					l->head = next;

				if (preitem)
					preitem->next = next;

				temp = item->next;
				item->next = next->next;
				next->next = (m - i) > 1 ? temp : item;
				temp = item->prev;
				item->prev = next->prev;
				next->prev = (m - i) > 1 ? temp : preitem;

				if (next->next)
					next->next->prev = next;
				if (item->next)
					item->next->prev = item;

				if (prenext && prenext != item)
					prenext->next = item;

				if (m == l->size - 1) {
					item->next = NULL;
					l->tail = item;
				}

#if DEBUG_DLLST_SORTROUTINE
				debug = l->head;
				printf("\titem |\tprev\tfields\tnext\n");
				printf("\t-----+----------------------\n");
				for (n=0;n<l->size;n++) {
					printf("\t  %u  |\t", debug->n);

					if (!debug->prev)
						printf("NULL");
					else
						printf("%u", debug->prev->n);

					dllst_sprintf(l, debug, res);
					printf("\t%s\t", res);

					if (!debug->next)
						printf("NULL");
					else
						printf("%u", debug->next->n);

					if (debug == preitem)
						printf("\t<- preitem");
					if (debug == item)
						printf("\t<- item");
					if (debug == prenext)
						printf("\t<- prenext");
					if (debug == next)
						printf("\t<- next");

					printf("\n");
					debug = debug->next;
				}

				printf("\n --- \n");
#endif

			}
			i++;
			j = i;
		}
	}
}

static unsigned long dllst_findmin(dllst_t *l, void *a, unsigned field, unsigned long index, boolean_t reset)
{
	static unsigned long min = 0UL;

	static char			 var_schar = 0x7f;
	static unsigned char		 var_uchar = ~0;
	static short			 var_sshort = 0x7fff;
	static unsigned short		 var_ushort = ~0;
	static int			 var_sint = 0x7fffffff;
	static unsigned int		 var_uint = ~0;
	static long			 var_slong = 0x7fffffff;
	static unsigned long		 var_ulong = ~0;
	static long long		 var_slonglong = 0x7fffffffffffffff;
	static unsigned long long	 var_ulonglong = ~0;
	static float			 var_float = +3.4028234663852885981170418348451692544000e+38;
	static double			 var_double = +1.7976931348623157081452742373170435679807e+308;
	static char *var_string = NULL;

	if (reset) {
		var_schar = 0x7f;
		var_uchar = ~0;
		var_sshort = 0x7fff;
		var_ushort = ~0;
		var_sint = 0x7fffffff;
		var_uint = ~0;
		var_slong = 0x7fffffff;
		var_ulong = ~0;
		var_slonglong = 0x7fffffffffffffff;
		var_ulonglong = ~0;
		var_float = +3.4028234663852885981170418348451692544000e+38;
		var_double = +1.7976931348623157081452742373170435679807e+308;
		free(var_string);
		var_string = NULL;
	}

#define ADD_CASE_STATIC_COMPARISON(f_type, f_type_p, static_var, op) \
case f_type: \
	if (*((f_type_p *)a + field * sizeof(p2align_t) / sizeof(f_type_p)) op static_var) { \
		static_var = *((f_type_p *)a + field * sizeof(p2align_t) / sizeof(f_type_p)); \
		min = index; \
	} \
	break;

	switch (l->f_info[field]->f_type) {
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_CHAR_T, char, var_schar, <=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_CHAR_T, unsigned char, var_uchar, <=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_SHORT_T, short, var_sshort, <=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_SHORT_T, unsigned short, var_ushort, <=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_INT_T, int, var_sint, <=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_INT_T, unsigned int, var_uint, <=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_LONG_T, long, var_slong, <=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_LONG_T, unsigned long, var_ulong, <=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_LONG_LONG_T, long long, var_slonglong, <=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_LONG_LONG_T, unsigned long long, var_ulonglong, <=)
	ADD_CASE_STATIC_COMPARISON(F_FLOAT_T, float, var_float, <)
	ADD_CASE_STATIC_COMPARISON(F_DOUBLE_T, double, var_double, <)
	case F_STRING_T:
		if (!var_string) {
new_string:
			var_string = (char *)calloc(1, \
					strlen(*((char **)a + field * sizeof(p2align_t) / sizeof(char *))) + 1);
			strcpy(var_string, *((char **)a + field * sizeof(p2align_t) / sizeof(char *)));
			min = index;
		} else {
			if (strcmp(*((char **)a + field * sizeof(p2align_t) / sizeof(char *)), var_string) <= 0) {
				free(var_string);
				goto new_string;
			}
		}
		break;
	};

	return min;

#undef ADD_CASE_STATIC_COMPARISON
}

static unsigned long dllst_findmax(dllst_t *l, void *a, unsigned field, unsigned long index, boolean_t reset)
{
	static unsigned long max = 0UL;

	static char			 var_schar = 0x80;
	static unsigned char		 var_uchar = 0;
	static short			 var_sshort = 0x8000;
	static unsigned short		 var_ushort = 0;
	static int			 var_sint = 0x80000000;
	static unsigned int		 var_uint = 0;
	static long			 var_slong = 0x80000000;
	static unsigned long		 var_ulong = 0UL;
	static long long		 var_slonglong = 0x8000000000000000;
	static unsigned long long	 var_ulonglong = 0ULL;
	static float			 var_float = -3.4028234663852885981170418348451692544000e+38;
	static double			 var_double = -1.7976931348623157081452742373170435679807e+308;
	static char *var_string = NULL;

	if (reset) {
		var_schar = 0x80;
		var_uchar = 0;
		var_sshort = 0x8000;
		var_ushort = 0;
		var_sint = 0x80000000;
		var_uint = 0;
		var_slong = 0x80000000;
		var_ulong = 0UL;
		var_slonglong = 0x8000000000000000;
		var_ulonglong = 0ULL;
		var_float = -3.4028234663852885981170418348451692544000e+38;
		var_double = -1.7976931348623157081452742373170435679807e+308;
		free(var_string);
		var_string = NULL;
	}

#define ADD_CASE_STATIC_COMPARISON(f_type, f_type_p, static_var, op) \
case f_type: \
	if (*((f_type_p *)a + field * sizeof(p2align_t) / sizeof(f_type_p)) op static_var) { \
		static_var = *((f_type_p *)a + field * sizeof(p2align_t) / sizeof(f_type_p)); \
		max = index; \
	} \
	break;

	switch (l->f_info[field]->f_type) {
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_CHAR_T, char, var_schar, >=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_CHAR_T, unsigned char, var_uchar, >=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_SHORT_T, short, var_sshort, >=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_SHORT_T, unsigned short, var_ushort, >=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_INT_T, int, var_sint, >=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_INT_T, unsigned int, var_uint, >=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_LONG_T, long, var_slong, >=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_LONG_T, unsigned long, var_ulong, >=)
	ADD_CASE_STATIC_COMPARISON(F_SIGNED_LONG_LONG_T, long long, var_slonglong, >=)
	ADD_CASE_STATIC_COMPARISON(F_UNSIGNED_LONG_LONG_T, unsigned long long, var_ulonglong, >=)
	ADD_CASE_STATIC_COMPARISON(F_FLOAT_T, float, var_float, >)
	ADD_CASE_STATIC_COMPARISON(F_DOUBLE_T, double, var_double, >)
	case F_STRING_T:
		if (!var_string) {
new_string:
			var_string = (char *)calloc(1, \
					strlen(*((char **)a + field * sizeof(p2align_t) / sizeof(char *))) + 1);
			strcpy(var_string, *((char **)a + field * sizeof(p2align_t) / sizeof(char *)));
			max = index;
		} else {
			if (strcmp(*((char **)a + field * sizeof(p2align_t) / sizeof(char *)), var_string) >= 0) {
				free(var_string);
				goto new_string;
			}
		}
		break;
	};

	return max;

#undef ADD_CASE_STATIC_COMPARISON
}

