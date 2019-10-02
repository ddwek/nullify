/*
 * misc.c: miscellaneous procedures of the game.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "misc.h"

#define SUIT_CLUBS              0
#define SUIT_DIAMONDS           1
#define SUIT_HEARTS             2
#define SUIT_SPADES             3

/*
 *
 * Open the text file @filename and add labels to the dialog @dlg for each
 * line ending in '\n'. Only Unix-style line terminators are supported.
 *
 */
void print_rules(gui_dialog_t *dlg, char *filename)
{
	int i, j, fd, nrow = 0, rowstart = 0;
	struct stat fd_stat;
	unsigned char *fd_buf = NULL;
	char *rowstr = NULL;


	fd = open(filename, O_RDONLY);
	if (!fd) {
		printf("Could not open %s\n", filename);
		return;
	}

	fstat(fd, &fd_stat);
	fd_buf = (unsigned char *)calloc(1, fd_stat.st_size + 1);
	if (!fd_buf) {
		printf("Could not allocate %ld bytes\n", fd_stat.st_size + 1);
		close(fd);
		return;
	}

	read(fd, fd_buf, fd_stat.st_size);
	close(fd);

	for (i=0;i<fd_stat.st_size;i++) {
		if (fd_buf[i] == '\n') {
			rowstr = (char *)calloc(1, i - rowstart + 1);
			if (!rowstr) {
				printf("Could not allocate %d bytes\n", i - rowstart + 1);
				free(fd_buf);
				return;
			}

			for (j=0;j<i - rowstart;j++)
				rowstr[j] = fd_buf[rowstart + j];

			gui_addlabel(dlg, rowstr, nrow);
			nrow++;
			rowstart = i + 1;

			free(rowstr);
			rowstr = NULL;
		}
	}

	free(fd_buf);
}

/*
 *
 * Return TRUE if @var is less than or equal to @bound, FALSE otherwise.
 * fn_less() and fn_greater() are functions called as continuity condition of
 * loops which can iterate both positively and negatively. See main.c:animate_card()
 * for a use case.
 *
 */
boolean_t fn_less(int var, int bound)
{
	if (var <= bound)
		return TRUE;
	return FALSE;
}

/*
 *
 * Return TRUE if @var is greater than or equal to @bound, FALSE otherwise.
 * fn_less() and fn_greater() are functions called as continuity condition of
 * loops which can iterate both positively and negatively. See main.c:animate_card()
 * for a use case.
 *
 */
boolean_t fn_greater(int var, int bound)
{
	if (var >= bound)
		return TRUE;
	return FALSE;
}

/*
 *
 * Create @timerid timer with event notifications based on threads running
 * the notify function @notify_fn, which receives @fn_arg as its only argument.
 * Then, you can write the notify function to process conditionally the
 * argument and execute different branches for each of them. See main.c:do_timer()
 * for an example.
 *
 */
void do_timer_prepare(timer_t *timerid, void (*notify_fn)(union sigval), int fn_arg)
{
	struct sigevent sev = { { 0 } };


	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = notify_fn;
	sev.sigev_value.sival_int = fn_arg;
	timer_create(CLOCK_REALTIME, &sev, timerid);
}

/*
 *
 * Set @timerid interval to be triggered to @secs seconds and @nsecs
 * nanoseconds. @timerid must be first created with do_timer_prepare().
 *
 */
void do_timer_set(timer_t *timerid, time_t secs, signed long nsecs)
{
	struct itimerspec its = { { 0 }, { 0 } };


	its.it_interval.tv_sec = its.it_value.tv_sec = secs;
	its.it_interval.tv_nsec = its.it_value.tv_nsec = nsecs;
	timer_settime(timerid, TIMER_ABSTIME, &its, NULL);
}

/*
 *
 * Unset the specified timer @timerid: that is, set interval of execution to 0.
 *
 */
void do_timer_unset(timer_t *timerid)
{
	struct itimerspec its = { { 0 }, { 0 } };


	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	timer_settime(timerid, TIMER_ABSTIME, &its, NULL);
}

/*
 *
 * Return the suit and number of a given card on @ret_suit and @ret_number,
 * respectively. All of its arguments must be non-null.
 *
 */
void decode_card_rev(char *card, int *ret_suit, int *ret_number)
{
	if (!card || !ret_suit || !ret_number)
		return;

	if (!strncmp(card, " A", 2))
		*ret_number = 0;
	else if (!strncmp(card, " 2", 2))
		*ret_number = 1;
	else if (!strncmp(card, " 3", 2))
		*ret_number = 2;
	else if (!strncmp(card, " 4", 2))
		*ret_number = 3;
	else if (!strncmp(card, " 5", 2))
		*ret_number = 4;
	else if (!strncmp(card, " 6", 2))
		*ret_number = 5;
	else if (!strncmp(card, " 7", 2))
		*ret_number = 6;
	else if (!strncmp(card, " 8", 2))
		*ret_number = 7;
	else if (!strncmp(card, " 9", 2))
		*ret_number = 8;
	else if (!strncmp(card, "10", 2))
		*ret_number = 9;
	else if (!strncmp(card, " J", 2))
		*ret_number = 10;
	else if (!strncmp(card, " Q", 2))
		*ret_number = 11;
	else if (!strncmp(card, " K", 2))
		*ret_number = 12;

	switch (card[2]) {
	case 'c':
		*ret_suit = SUIT_CLUBS;
		break;
	case 'd':
		*ret_suit = SUIT_DIAMONDS;
		break;
	case 'h':
		*ret_suit = SUIT_HEARTS;
		break;
	case 's':
		*ret_suit = SUIT_SPADES;
		break;
	};
}

