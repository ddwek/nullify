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
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "gui.h"

#ifndef _SRC_MISC_H_
#define _SRC_MISC_H_
#ifndef _HAVE_BOOLEAN_T_
#define _HAVE_BOOLEAN_T_
typedef enum { FALSE=0, TRUE } boolean_t;
#endif

void print_rules(gui_dialog_t *dlg, char *filename);
boolean_t fn_less(int var, int bound);
boolean_t fn_greater(int var, int bound);
void do_timer_prepare(timer_t *timerid, void (*notify_fn)(union sigval), int fn_arg);
void do_timer_set(timer_t *timerid, time_t secs, signed long nsecs);
void do_timer_unset(timer_t *timerid);
void decode_card_rev(char *card, int *ret_suit, int *ret_number);
#endif

