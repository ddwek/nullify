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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>
#undef TRUE
#undef FALSE
#include "gui.h"
#include "dllst.h"

#define NPLAYERS		4
#define CARD_WIDTH		73
#define CARD_HEIGHT		97
#define CARD_ACE(x)		(x * 13 + 0)
#define CARD_TWO(x)		(x * 13 + 1)
#define CARD_FOUR(x)		(x * 13 + 3)
#define CARD_SEVEN(x)		(x * 13 + 6)
#define CARD_JOCKER(x)		(x * 13 + 10)
#define CARD_QUEEN(x)		(x * 13 + 11)
#define CARD_KING(x)		(x * 13 + 12)
#define KIND_CLOVERS		0
#define KIND_DIAMONDS		1
#define KIND_HEARTS		2
#define KIND_PICAS		3
#define KIND_X			((800 - 2 * (CARD_WIDTH - 7)) / 2 - 32)
#define KIND_Y(x)		((600 - CARD_HEIGHT) / 2 + x * 26)
#define ARROW_X			(KIND_X - 100)
#define ARROW_Y			(KIND_Y(KIND_CLOVERS) + 20)
#define CARD_KIND(x)		(*((unsigned int *)x->fields + 0))
#define CARD_NUMBER(x)		(*((unsigned int *)x->fields + 2))
#define DECK_PLAYED_X		((800 - 2 * (CARD_WIDTH - 7)) / 2 + CARD_WIDTH + 7)
#define DECK_PLAYED_Y		((600 - CARD_HEIGHT) / 2)
#define FLAGS_NONE		0
#define FLAGS_QUEEN		1
#define HUMAN			0
#define CPU1			1
#define CPU2			2
#define CPU3			3
#define HUMAN_X			((800 - 5 * (CARD_WIDTH + 7)) / 2 - 60)
#define HUMAN_Y			(600 - 10 - (CARD_HEIGHT + 3) / 2)
#define CPU1_X			10
#define CPU1_Y			128
#define CPU2_X			((800 - 5 * (CARD_WIDTH + 7)) / 2 - 60)
#define CPU2_Y			(10 + (CARD_HEIGHT + 3) / 2)
#define CPU3_X			(800 - 10 - CARD_WIDTH)
#define CPU3_Y			128
#define NRESOURCES		65
#define RES_PLAYING_DISABLED	52
#define RES_PLAYING_ENABLED	53
#define RES_CLOVERS		54
#define RES_DIAMONDS		55
#define RES_HEARTS		56
#define RES_PICAS		57
#define RES_CLOVERS_INV		58
#define RES_DIAMONDS_INV	59
#define RES_HEARTS_INV		60
#define RES_PICAS_INV		61
#define RES_ARROW_CW		62
#define RES_ARROW_CCW		63
#define RES_DECK		64
#define RES_KIND_BASE		54
#define RES_KIND_BASE_INV	58

typedef enum { EXPOSURE_CARD=0, DELETE_CARD, ADD_CARD, GET_CARD } action_t;
typedef enum { EXPOSURE_KIND=0, SELECT_KIND, SELECT_NONE } action_table_t;

Display *display;
Window window;
XEvent event;
GC gc_table, gc_selector, gc_white, gc_black, gc_anim[8];
struct itimerspec its = { { 0 }, { 0 } };
timer_t deck_timer, playing_timer, table_timer;
unsigned turn;
struct resource_st {
	png_bytepp bytes;
	GC *color;
	unsigned ncolors;
	unsigned width;
	unsigned height;
	struct {
		int x0;
		int y0;
		int x1;
		int y1;
	} region;
} *resource = NULL;
struct player_st {
	boolean_t active;		// Whether the player is still active in the round or not
	dllst_t *list;			// List of cards
	float probabilities[17];	// Bots choose the best card to play depending on probabilities of kind/number
	unsigned scores;		// Total scores in the game
} *player;
dllst_t *deck_list = NULL, *played_list = NULL;
int card_row[NPLAYERS] = { 0 };
int playing_x[NPLAYERS] = { 0 }, playing_y[NPLAYERS] = { 0 };
int getatmost, lastkind, rotation;
boolean_t round_finished = FALSE;
gui_dialog_t *dialog = NULL;
gui_button_t *newround_button = NULL, *newgame_button = NULL;
struct {
	unsigned int kind;
	unsigned int unused0;
	unsigned int number;
	unsigned int unused1;
} fields = { 0 };

int load_resource(struct resource_st *res, const char *filename);
void render_resource(struct resource_st *res, int X, int Y);
void init_deck(void);
int init_players(int n);
void init_round(void);
void calc_probabilities(int n);
void add_selector(struct resource_st *card, int x, int dx, int y);
void del_selector(struct resource_st *card, int x, int dx, int y);
void update_cards(int nplayer, action_t act, void *param);
void update_table(action_table_t act);
void update_turn(int flags);
int getcardfromdeck(int nplayer, action_t act);
void playcard(int n, int kind, int number, unsigned long *param);
int selectbestkind(int n);
void bot_play(int n);
int getactiveplayers(void);
void finish_round(void);
void do_exposure(XExposeEvent *ep);
void do_buttondown(XButtonEvent *bp);
void do_timer(union sigval tp);

int main(int argc, char **argv)
{
	int i, j;
	char filename[64] = { '\0' };
	struct sigevent sev = { { 0 } };
	Pixmap stdpixmap = { 0 };
	XSizeHints hints;


	XInitThreads();
	display = XOpenDisplay("");
	window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 0, 0, 800, 600, 1, 0, 0);
	XSelectInput(display, window, ExposureMask|ButtonPressMask);
	hints.max_width = 800;
	hints.max_height = 600;
	hints.flags = PMaxSize;
	XSetStandardProperties(display, window, "nullify", NULL, stdpixmap, argv, argc, &hints);
	XMapRaised(display, window);

	gc_white    = XCreateGC(display, window, 0, NULL);
	gc_black    = XCreateGC(display, window, 0, NULL);
	gc_table    = XCreateGC(display, window, 0, NULL);
	gc_selector = XCreateGC(display, window, 0, NULL);
	for (i=0;i<8;i++) {
		gc_anim[i] = XCreateGC(display, window, 0, NULL);
		XSetArcMode(display, gc_anim[i], ArcPieSlice);
	}
	XSetForeground(display, gc_white,    0xffffff);
	XSetBackground(display, gc_white,    0x006600);
	XSetForeground(display, gc_black,    0x404040);
	XSetBackground(display, gc_black,    0xa0a0a0);
	XSetForeground(display, gc_table,    0x006600);
	XSetForeground(display, gc_selector, 0xffffff);
	XSetForeground(display, gc_anim[0],  0xf6c182);
	XSetForeground(display, gc_anim[1],  0xd3198d);
	XSetForeground(display, gc_anim[2],  0xf9280c);
	XSetForeground(display, gc_anim[3],  0x8536fb);
	XSetForeground(display, gc_anim[4],  0x3697fb);
	XSetForeground(display, gc_anim[5],  0x6aff56);
	XSetForeground(display, gc_anim[6],  0xeaf6a9);
	XSetForeground(display, gc_anim[7],  0xff9d3e);

	player = (struct player_st *)calloc(NPLAYERS, sizeof(struct player_st));
	if (!player)
		return -1;

	dllst_verbose = 0;
	resource = (struct resource_st *)calloc(NRESOURCES, sizeof(struct resource_st));
	if (resource) {
		for (i=0;i<4;i++) {
			strcpy(filename, "/usr/local/share/nullify/cards/");
			switch (i) {
			case 0:
				strcat(filename, "c");
				break;
			case 1:
				strcat(filename, "d");
				break;
			case 2:
				strcat(filename, "h");
				break;
			case 3:
				strcat(filename, "p");
				break;
			};

			for (j=0;j<13;j++) {
				sprintf(&filename[32], "%02d.png", j + 1);
				load_resource(&resource[i * 13 + j], filename);
			}
		}

		load_resource(&resource[RES_PLAYING_DISABLED], "/usr/local/share/nullify/res/playing_disabled.png");
		load_resource(&resource[RES_PLAYING_ENABLED], "/usr/local/share/nullify/res/playing_enabled.png");
		load_resource(&resource[RES_CLOVERS], "/usr/local/share/nullify/res/clovers.png");
		load_resource(&resource[RES_DIAMONDS], "/usr/local/share/nullify/res/diamonds.png");
		load_resource(&resource[RES_HEARTS], "/usr/local/share/nullify/res/hearts.png");
		load_resource(&resource[RES_PICAS], "/usr/local/share/nullify/res/picas.png");
		load_resource(&resource[RES_CLOVERS_INV], "/usr/local/share/nullify/res/clovers-inv.png");
		load_resource(&resource[RES_DIAMONDS_INV], "/usr/local/share/nullify/res/diamonds-inv.png");
		load_resource(&resource[RES_HEARTS_INV], "/usr/local/share/nullify/res/hearts-inv.png");
		load_resource(&resource[RES_PICAS_INV], "/usr/local/share/nullify/res/picas-inv.png");
		load_resource(&resource[RES_ARROW_CW], "/usr/local/share/nullify/res/arrow-cw.png");
		load_resource(&resource[RES_ARROW_CCW], "/usr/local/share/nullify/res/arrow-ccw.png");
		load_resource(&resource[RES_DECK], "/usr/local/share/nullify/res/deck.png");
	}

	card_row[0] = 600 - CARD_HEIGHT - 10;
	card_row[2] = 10;
	playing_x[HUMAN] = (800 - 32) / 2;
	playing_y[HUMAN] = card_row[HUMAN] - 56;
	playing_x[CPU1]  = CARD_WIDTH + 32;
	playing_y[CPU1]  = (600 - 32) / 2;
	playing_x[CPU2]  = (800 - 32) / 2;
	playing_y[CPU2]  = card_row[CPU1] + 125;
	playing_x[CPU3]  = 800 - CARD_WIDTH - 10 - 64;
	playing_y[CPU3]  = (600 - 32) / 2;

	srand(time(NULL));
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = do_timer;
	sev.sigev_value.sival_int = 1;
	timer_create(CLOCK_REALTIME, &sev, &deck_timer);
	sev.sigev_value.sival_int = 2;
	timer_create(CLOCK_REALTIME, &sev, &playing_timer);
	sev.sigev_value.sival_int = 3;
	timer_create(CLOCK_REALTIME, &sev, &table_timer);

	XNextEvent(display, &event);
	if (event.type == Expose) {
		dialog = gui_newdialog(display, window, "nullify", 60, 80, 680, 450);
		gui_addlabel(display, window, dialog, "nullify is a turn-based cards videogame which goal is to get rid of all cards in hand", 0, 16);
		gui_addlabel(display, window, dialog, "(initially 5 cards). There are no winners, but a looser: last player who could not get rid of his/her cards.", 0, 32);
		gui_addlabel(display, window, dialog, "Main rule", 0, 48);
		gui_addlabel(display, window, dialog, "=========", 0, 64);
		gui_addlabel(display, window, dialog, "When in turn, the player must either play a card of the same kind / number as last card played", 0, 80);
		gui_addlabel(display, window, dialog, "or pick up one card from the deck. If is not possible play the new card because it mismatches kind or", 0, 96);
		gui_addlabel(display, window, dialog, "number, you must to do right-click over the deck and the turn is updated to the next player.", 0, 112);
		gui_addlabel(display, window, dialog, "Quick reference of special cards (see README for further explanations)", 0, 128);
		gui_addlabel(display, window, dialog, "======================================================================", 0, 144);
		gui_addlabel(display, window, dialog, "Ace and two", 0, 160);
		gui_addlabel(display, window, dialog, "-----------", 0, 176);
		gui_addlabel(display, window, dialog, "Each time a player dispatches an 'A' or '2', the next player in turn will be", 0, 192);
		gui_addlabel(display, window, dialog, "forced to get up to 4 or 2 cards from the deck, respectively.", 0, 208);
		gui_addlabel(display, window, dialog, "Four and seven", 0, 224);
		gui_addlabel(display, window, dialog, "--------------", 0, 240);
		gui_addlabel(display, window, dialog, "Whether a '4' or '7' is played, another card of the same kind / number must be played.", 0, 256);
		gui_addlabel(display, window, dialog, "Jocker ('J')", 0, 272);
		gui_addlabel(display, window, dialog, "------------", 0, 288);
		gui_addlabel(display, window, dialog, "Likewise, but also the player can select the kind of cards to play.", 0, 304);
		gui_addlabel(display, window, dialog, "Queen", 0, 320);
		gui_addlabel(display, window, dialog, "-----", 0, 336);
		gui_addlabel(display, window, dialog, "Whether a 'Q' is played, the most immediate player in turn is skipped.", 0, 352);
		gui_addlabel(display, window, dialog, "King", 0, 368);
		gui_addlabel(display, window, dialog, "----", 0, 384);
		gui_addlabel(display, window, dialog, "Whether a 'K' is played, rotation mode is toggled clockwise/counter-clockwise", 0, 400);
		gui_addlabel(display, window, dialog, "Have fun ;-)", 0, 416);
		newround_button = gui_addbutton(display, window, dialog, "Start", 620, 400);
		round_finished = TRUE;
	}

	while (1) {
		if (XCheckMaskEvent(display, ExposureMask|ButtonPressMask, &event)) {
			switch(event.type) {
			case Expose:
				if (newround_button || newgame_button)
					init_round();
				if (!event.xexpose.count)
					do_exposure(&event.xexpose);

				round_finished = FALSE;
				free(newround_button);
				newround_button = NULL;
				free(newgame_button);
				newgame_button = NULL;
				break;
			case ButtonPress:
				do_buttondown(&event.xbutton);
				break;
			};
		}
	}

	while (dllst_delitem(deck_list, 0)) {}
	free(deck_list);
	while (dllst_delitem(played_list, 0)) {}
	free(played_list);
	for (i=0;i<NPLAYERS;i++) {
		while (dllst_delitem(player[i].list, 0)) {}
		free(player[i].list);
	}
	free(player);
	for (i=0;i<NRESOURCES;i++) {
		free(resource[i].bytes);
		free(resource[i].color);
	}
	free(resource);
	XFreeGC(display, gc_white);
	XFreeGC(display, gc_black);
	XFreeGC(display, gc_table);
	XFreeGC(display, gc_selector);
	for (i=0;i<8;i++)
		XFreeGC(display, gc_anim[i]);
	XDestroyWindow(display, window);
	XFlush(display);
	XCloseDisplay(display);
	exit(0);
}

/*
 *
 * Load png-image @filename and store it in @res.
 *
 */
int load_resource(struct resource_st *res, const char *filename)
{
	int x, y;
	FILE *fileptr;
	dllst_t *dllst = NULL;
	dllst_item_struct_t *iter;
	struct {
		unsigned int color;
		void *unused0;
	} fields2 = { 0 };
	png_structp png_ptr   = NULL;
	png_infop png_infoptr = NULL;


	fileptr = fopen(filename, "rb");
	if (!fileptr) {
		perror("fopen");
		return -1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infoptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, fileptr);
	png_read_info(png_ptr, png_infoptr);

	res->bytes = (png_bytepp)malloc(png_infoptr->height * sizeof(png_bytep));
	if (!res->bytes) {
		perror("malloc");
		return -1;
	}

	for (y=0;y<png_infoptr->height;y++) {
		res->bytes[y] = (png_bytep)malloc(png_infoptr->width * 3);
		if (!res->bytes[y]) {
			perror("malloc");
			return -1;
		}
	}
	png_read_image(png_ptr, res->bytes);
	png_read_end(png_ptr, png_infoptr);

	dllst = dllst_initlst(dllst, "I:");
	for (y=0;y<png_infoptr->height;y++) {
		for (x=0;x<png_infoptr->width * 3;x+=3) {
			fields2.color = (res->bytes[y][x + 2] <<  0)|
					(res->bytes[y][x + 1] <<  8)|
					(res->bytes[y][x + 0] << 16);
			if (!dllst_isinlst(dllst, &fields2))
				dllst_newitem(dllst, &fields2);
		}
	}

	res->height = png_infoptr->height;
	res->width  = png_infoptr->width;
	res->ncolors = dllst->size;
	res->color = (GC *)malloc(dllst->size * sizeof(GC));
	if (!res->color) {
		perror("malloc");
		return -1;
	}

	for (x=0,iter=dllst->head;x<dllst->size;x++,iter=iter->next) {
		res->color[x] = XCreateGC(display, window, 0, NULL);
		XSetForeground(display, res->color[x], *((unsigned long *)iter->fields));
	}

	while (dllst_delitem(dllst, 0)) {}
	free(dllst);
	dllst = NULL;
	png_destroy_read_struct(&png_ptr, &png_infoptr, NULL);

	return 0;
}

/*
 *
 * Render @res, a previously loaded png-image, at (@X, @Y) coordinates.
 *
 */
void render_resource(struct resource_st *res, int X, int Y)
{
	int i, j, x, t;
	unsigned long color = 0;
	XGCValues values;


	for (i=0;i<res->height;i++) {
		for (x=0,j=0;x<res->width;x++,j+=3) {
			color = (res->bytes[i][j + 2] <<  0)|
				(res->bytes[i][j + 1] <<  8)|
				(res->bytes[i][j + 0] << 16);
			for (t=0;t<res->ncolors;t++) {
				XGetGCValues(display, res->color[t], GCForeground, &values);
				if (color == values.foreground) {
					XDrawPoint(display, window, res->color[t], X + x, Y + i);
					break;
				}
			}
		}
	}
}

/*
 *
 * Initialize the deck with the 52 cards randomly sorted.
 *
 */
void init_deck(void)
{
	int i, j;


	deck_list = dllst_initlst(deck_list, "I:I:");
	for (i=0;i<4;i++) {
		for (j=0;j<13;j++) {
			fields.kind = (unsigned int)i;
			fields.number = (unsigned int)j;
			dllst_newitem(deck_list, &fields);
		}
	}

	if (deck_list->size != 52)
		return;

	for (i=0;i<52;i++)
		dllst_swapitems(deck_list, i, rand() % 52);

	getatmost = 0;
}

/*
 *
 * Initialize the players to 5 cards get from the deck.
 *
 */
int init_players(int n)
{
	int i, j;


	for (i=0;i<n;i++) {
		// Player is active in the current round (i.e., this bot can play cards when in turn)
		player[i].active = TRUE;

		player[i].list = dllst_initlst(player[i].list, "I:I:");
		for (j=0;j<5;j++) {
			fields.kind   = CARD_KIND(deck_list->head);
			fields.number = CARD_NUMBER(deck_list->head);
			dllst_newitem(player[i].list, &fields);

			// Delete the first item (top card) of deck_list
			dllst_delitem(deck_list, 0);
		}
	}

	return 0;
}

void init_round(void)
{
	init_deck();
	init_players(NPLAYERS);
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	timer_settime(deck_timer, TIMER_ABSTIME, &its, NULL);
	timer_settime(playing_timer, TIMER_ABSTIME, &its, NULL);
	timer_settime(table_timer, TIMER_ABSTIME, &its, NULL);

	played_list = dllst_initlst(played_list, "I:I:");
	fields.kind   = CARD_KIND(deck_list->head);
	fields.number = CARD_NUMBER(deck_list->head);
	dllst_newitem(played_list, &fields);
	dllst_delitem(deck_list, 0);

	turn = HUMAN;
	getatmost = 0;
	lastkind = CARD_KIND(played_list->tail);
	rotation = 1;
}

/*
 *
 * Calculate probabilities for each kind / number of card in card list of the bot.
 * Bots select the card to play based on the kind/number of the card with less
 * probability than others.
 *
 */
void calc_probabilities(int n)
{
	int i;
	unsigned counter[17];
	dllst_item_struct_t *iter;


	for (i=0;i<13;i++)
		counter[i] = 4;
	for (i=13;i<17;i++)
		counter[i] = 13;

	for (i=0;i<13;i++) {
		for (iter=player[n].list->head;iter;iter=iter->next)
			if (CARD_NUMBER(iter) == i)
				counter[i]--;
		for (iter=played_list->head;iter;iter=iter->next)
			if (CARD_NUMBER(iter) == i)
				counter[i]--;
	}

	// probabilities for the 13 numbers (ace up to king)
	for (i=0;i<13;i++)
		player[n].probabilities[i] = (float)counter[i] / (float)4.0;

	for (i=0;i<4;i++) {
		for (iter=player[n].list->head;iter;iter=iter->next)
			if (CARD_KIND(iter) == i)
				counter[i + 13]--;
		for (iter=played_list->head;iter;iter=iter->next)
			if (CARD_KIND(iter) == i)
				counter[i + 13]--;
	}

	// probabilities for the 4 kinds
	for (i=13;i<17;i++)
		player[n].probabilities[i] = (float)counter[i] / (float)13.0;
}

/*
 *
 * Draw a white triangle indicating that @card can be selected with the mouse.
 *
 */
void add_selector(struct resource_st *card, int x, int dx, int y)
{
	XPoint selpoints[3];


	card->region.x0 = x - dx;
	card->region.y0 = y;
	card->region.x1 = x + dx;
	card->region.y1 = y + CARD_HEIGHT;

	selpoints[0].x = x - 5;
	selpoints[0].y = y - 5;
	selpoints[1].x = x + 5;
	selpoints[1].y = y - 5;
	selpoints[2].x = x;
	selpoints[2].y = y;
	XFillPolygon(display, window, gc_selector, selpoints, 3, Complex, CoordModeOrigin);
}

/*
 *
 * Inhibit selection of this card by setting its region structure members to negative values.
 *
 */
void del_selector(struct resource_st *card, int x, int dx, int y)
{
	XPoint selpoints[3];


	card->region.x0 = -1;
	card->region.y0 = -1;
	card->region.x1 = -1;
	card->region.y1 = -1;

	selpoints[0].x = x - 5;
	selpoints[0].y = y - 5;
	selpoints[1].x = x + 5;
	selpoints[1].y = y - 5;
	selpoints[2].x = x;
	selpoints[2].y = y;
	XFillPolygon(display, window, gc_table, selpoints, 3, Complex, CoordModeOrigin);
}

/*
 *
 * Update cards of @nplayer according to action @act and optional argument @param.
 * Valid values for @act are ADD_CARD, DELETE_CARD and GET_CARD, for add, delete or
 * get cards from the deck, respectively. Selectors are meaningless for bots.
 *
 */
void update_cards(int nplayer, action_t act, void *param)
{
	int i, xstart;
	float sep;
	dllst_item_struct_t *iter;


	if (nplayer == HUMAN || nplayer == CPU2) {
		if (player[nplayer].list->size > 5) {
			sep = (CARD_WIDTH + 7) * 5 / player[nplayer].list->size;
			xstart = (800 - 400) / 2;
		} else {
			sep = CARD_WIDTH + 7;
			xstart = (800 - player[nplayer].list->size * (CARD_WIDTH + 7)) / 2;
		}

		for (i=0,iter=player[nplayer].list->head;iter;iter=iter->next,i++) {
			XFillRectangle(display, window, gc_table,
				       i * sep + xstart, card_row[nplayer], CARD_WIDTH, CARD_HEIGHT);
			if (nplayer == HUMAN)
				del_selector(&resource[CARD_KIND(iter) * 13 + CARD_NUMBER(iter)],
					     i * sep + xstart + sep / 2,
					     sep / 2,
					     card_row[nplayer]);
		}
	} else {
		if (nplayer == CPU1)
			xstart = 10;
		else
			xstart = 800 - CARD_WIDTH - 10;

		if (player[nplayer].list->size > 5) {
			sep  = (600 - 5 * 60) / player[nplayer].list->size;
		} else {
			sep = (600 - 5 * (CARD_HEIGHT + 3)) / 2;
		}

		for (i=0;i<player[nplayer].list->size;i++)
			XFillRectangle(display, window, gc_table, xstart, i * sep + 150, CARD_WIDTH, CARD_HEIGHT);
	}

	switch (act) {
	case DELETE_CARD:
		dllst_delitem(player[nplayer].list, *((unsigned long *)param));
		break;
	case ADD_CARD:
		dllst_newitem(player[nplayer].list, param);
		break;
	};

	if (nplayer == HUMAN || nplayer == CPU2) {
		if (player[nplayer].list->size > 5) {
			sep = (CARD_WIDTH + 7) * 5 / player[nplayer].list->size;
			xstart = (800 - 400) / 2;
		} else {
			sep = CARD_WIDTH + 7;
			xstart = (800 - player[nplayer].list->size * (CARD_WIDTH + 7)) / 2;
		}

		for (i=0,iter=player[nplayer].list->head;iter;iter=iter->next,i++) {
			if (nplayer == HUMAN)
				render_resource(&resource[CARD_KIND(iter) * 13 + CARD_NUMBER(iter)],
					    i * sep + xstart, card_row[nplayer]);
			else
				render_resource(&resource[RES_DECK], i * sep + xstart, card_row[nplayer]);
			if (nplayer == HUMAN) {
				if (act == GET_CARD) {
					if (CARD_NUMBER(iter) == CARD_TWO(0) ||
					    CARD_NUMBER(iter) == CARD_TWO(1) ||
					    CARD_NUMBER(iter) == CARD_TWO(2) ||
					    CARD_NUMBER(iter) == CARD_TWO(3) ||
					    CARD_NUMBER(iter) == CARD_ACE(0) ||
					    CARD_NUMBER(iter) == CARD_ACE(1) ||
					    CARD_NUMBER(iter) == CARD_ACE(2) ||
					    CARD_NUMBER(iter) == CARD_ACE(3)) {
						add_selector(&resource[CARD_KIND(iter) * 13 + CARD_NUMBER(iter)],
							     i * sep + xstart + sep / 2,
							     sep / 2,
							     card_row[nplayer]);
					}
					continue;
				}

				if (CARD_KIND(iter) == lastkind ||
				    CARD_NUMBER(iter) == CARD_NUMBER(played_list->tail))
					add_selector(&resource[CARD_KIND(iter) * 13 + CARD_NUMBER(iter)],
						     i * sep + xstart + sep / 2,
						     sep / 2,
						     card_row[nplayer]);
			}
		}
	} else {
		if (nplayer == CPU1)
			xstart = 10;
		else
			xstart = 800 - CARD_WIDTH - 10;

		if (player[nplayer].list->size > 5) {
			sep  = (600 - 5 * 60) / player[nplayer].list->size;
		} else {
			sep = (600 - 5 * (CARD_HEIGHT + 3)) / 2;
		}

		for (i=0;i<player[nplayer].list->size;i++)
			render_resource(&resource[RES_DECK], xstart, i * sep + 150);
	}
}

/*
 *
 * Update the status of the kind selectors and prepare their regions to be clicked
 * by the human player as long as last card played is a jocker.
 *
 */
void update_table(action_table_t act)
{
	int i;


	if (act == EXPOSURE_KIND) {
		for (i=0;i<4;i++)
			if (i == lastkind)
				render_resource(&resource[RES_KIND_BASE_INV + i], KIND_X, KIND_Y(i));
			else
				render_resource(&resource[RES_KIND_BASE + i], KIND_X, KIND_Y(i));
	} else if (act == SELECT_KIND) {
		for (i=0;i<4;i++) {
			resource[RES_KIND_BASE + i].region.x0 = KIND_X;
			resource[RES_KIND_BASE + i].region.y0 = KIND_Y(i);
			resource[RES_KIND_BASE + i].region.x1 = KIND_X + resource[RES_KIND_BASE + i].width;
			resource[RES_KIND_BASE + i].region.y1 = KIND_Y(i) + resource[RES_KIND_BASE + i].height;
		}
		its.it_interval.tv_sec = its.it_value.tv_sec = 0;
		its.it_interval.tv_nsec = its.it_value.tv_nsec = 125000000;
		timer_settime(table_timer, TIMER_ABSTIME, &its, NULL);
	} else if (act == SELECT_NONE) {
		for (i=0;i<4;i++) {
			resource[RES_KIND_BASE + i].region.x0 = -1;
			resource[RES_KIND_BASE + i].region.y0 = -1;
			resource[RES_KIND_BASE + i].region.x1 = -1;
			resource[RES_KIND_BASE + i].region.y1 = -1;
		}
		its.it_value.tv_sec = its.it_value.tv_nsec = 0;
		timer_settime(table_timer, TIMER_ABSTIME, &its, NULL);
	}
}

/*
 *
 * Update the turn according to rotation and skipping modes. Rotation mode can be
 * toggled clockwise / counter-clockwise by playing a king. Skipping mode is automatically
 * set if last card played was a queen, so that the turn is incremented or decremented twice
 * (e.g., rotation mode is set to ccw and turn is currently set to HUMAN; he/she plays a queen,
 * then the next players in turn are CPU2, CPU1, HUMAN, CPU3, etc).
 *
 */
void update_turn(int flags)
{
	render_resource(&resource[RES_PLAYING_DISABLED], playing_x[turn], playing_y[turn]);
	if (flags & FLAGS_QUEEN) {
		if (rotation == 1) {

			// FIXME: (turn + 1) & (NPLAYERS - 1) is slightly faster than modulo op
			while (!player[(turn + 1) % NPLAYERS].active) {
				turn++;
				turn %= NPLAYERS;
			}
			turn++;
			turn %= NPLAYERS;
		} else {
			while (!player[(turn - 1) % NPLAYERS].active) {
				turn--;
				turn %= NPLAYERS;
			}
			turn--;
			turn %= NPLAYERS;
		}
	}

	if (rotation == 1) {
		while (!player[(turn + 1) % NPLAYERS].active) {
			turn++;
			turn %= NPLAYERS;
		}
		turn++;
		turn %= NPLAYERS;
	} else {
		while (!player[(turn - 1) % NPLAYERS].active) {
			turn--;
			turn %= NPLAYERS;
		}
		turn--;
		turn %= NPLAYERS;
	}
	render_resource(&resource[RES_PLAYING_ENABLED], playing_x[turn], playing_y[turn]);
}

/*
 *
 * Move one card from the deck to player_list.
 * This function needs to be bugfixed (see below).
 *
 */
int getcardfromdeck(int nplayer, action_t act)
{
	int i;


getcard:
	if (deck_list->size) {
		fields.kind   = CARD_KIND(deck_list->head);
		fields.number = CARD_NUMBER(deck_list->head);
		dllst_delitem(deck_list, 0);
		update_cards(nplayer, act, &fields);
		return fields.kind * 13 + fields.number;
	} else {
		its.it_interval.tv_sec = its.it_value.tv_sec = 0;
		its.it_interval.tv_nsec = its.it_value.tv_nsec = 62500000;
		timer_settime(deck_timer, TIMER_ABSTIME, &its, NULL);
		while (played_list->size > 1) {
			fields.kind   = CARD_KIND(played_list->head);
			fields.number = CARD_NUMBER(played_list->head);
			dllst_delitem(played_list, 0);
			dllst_newitem(deck_list, &fields);
		}
		for (i=0;i<deck_list->size;i++)
			dllst_swapitems(deck_list, i, rand() % deck_list->size);

		// In order to avoid looping forever when one or more players get
		// 'played_list->size - 1' cards from the deck, we need to add the special 'finish round' case:
		// that is, the player with more cards than others looses and his/her cards are put back
		// to the deck.
		goto getcard;
	}
}

/*
 *
 * Move one card from player's list of cards to the stack of played cards.
 *
 */
void playcard(int n, int kind, int number, unsigned long *param)
{
	fields.kind   = kind;
	fields.number = number;
	dllst_newitem(played_list, &fields);
	render_resource(&resource[kind * 13 + number], DECK_PLAYED_X, DECK_PLAYED_Y);
	update_cards(n, DELETE_CARD, param);
	update_table(EXPOSURE_KIND);

	if (rotation == -1)
		render_resource(&resource[RES_ARROW_CCW], ARROW_X, ARROW_Y);
	else
		render_resource(&resource[RES_ARROW_CW], ARROW_X, ARROW_Y);
}

/*
 *
 * Bots select the best kind to switch when they play a jocker depending on how much cards of
 * each kind they have.
 *
 */
int selectbestkind(int n)
{
	int i, ret, count[4] = { 0 };
	dllst_t *dllst = NULL;
	dllst_item_struct_t *iter;


	for (iter=player[n].list->head;iter;iter=iter->next) {
		switch (CARD_KIND(iter)) {
		case 0:
			count[0]++;
			break;
		case 1:
			count[1]++;
			break;
		case 2:
			count[2]++;
			break;
		case 3:
			count[3]++;
			break;
		};
	}

	dllst = dllst_initlst(dllst, "I:I:");
	for (i=0;i<4;i++) {
		fields.kind = i;
		fields.number = count[i];
		dllst_newitem(dllst, &fields);
	}
	dllst_sortby(dllst, 1, FALSE);
	ret = *((int *)dllst->head->fields + 0);

	while (dllst_delitem(dllst, 0)) {}
	free(dllst);
	dllst = NULL;

	return ret;
}

/*
 *
 * Main function of bots game.
 *
 * Rain of gotos. Yes, I know, but label names are descriptive enough so that it does not
 * affect the readability of the code.
 *
 */
void bot_play(int n)
{
	unsigned long i = 0, j, min, moves = 0, xcard;
	dllst_item_struct_t *iter;
	boolean_t cpuplayed = FALSE;
	dllst_t *alternatives = NULL;


	alternatives = dllst_initlst(alternatives, "I:I:");
	for (j=0;j<4;j++) {
		if (CARD_NUMBER(played_list->tail) == CARD_TWO(j) ||
		    CARD_NUMBER(played_list->tail) == CARD_ACE(j)) {

			// fill the list with aces or two's matching last card played
			for (iter=player[n].list->head;iter;iter=iter->next) {
				if (CARD_NUMBER(iter) == CARD_TWO(0) ||
				    CARD_NUMBER(iter) == CARD_TWO(1) ||
				    CARD_NUMBER(iter) == CARD_TWO(2) ||
				    CARD_NUMBER(iter) == CARD_TWO(3) ||
				    CARD_NUMBER(iter) == CARD_ACE(0) ||
				    CARD_NUMBER(iter) == CARD_ACE(1) ||
				    CARD_NUMBER(iter) == CARD_ACE(2) ||
				    CARD_NUMBER(iter) == CARD_ACE(3)) {
					fields.kind   = CARD_KIND(iter);
					fields.number = CARD_NUMBER(iter);
					dllst_newitem(alternatives, &fields);
				}
			}
			goto checkalternatives;
		}
	}

	for (j=0;j<4;j++) {
		if (!alternatives->size &&
		    (CARD_NUMBER(played_list->tail) == CARD_TWO(j) ||
		     CARD_NUMBER(played_list->tail) == CARD_ACE(j)))

			// There are no alternatives, but last card played was either an ace or a two
			goto getmorecards;
	}

	while (i < 2) {
playagain:
		for (iter=player[n].list->head;iter;iter=iter->next) {

			// Main rule of the game: each player must put on table one card matching kind or
			// number as last card played.
			if (CARD_KIND(iter) == lastkind ||
			    CARD_NUMBER(iter) == CARD_NUMBER(played_list->tail)) {
				fields.kind   = CARD_KIND(iter);
				fields.number = CARD_NUMBER(iter);
				dllst_newitem(alternatives, &fields);
			}
		}

checkalternatives:
		if (alternatives->size) {
			calc_probabilities(n);
			min = CARD_KIND(alternatives->head) * 13 + CARD_NUMBER(alternatives->head);
			for (iter=alternatives->head->next;iter;iter=iter->next)
				if (player[n].probabilities[CARD_KIND(iter)] < player[n].probabilities[min / 13] ||
				    player[n].probabilities[CARD_NUMBER(iter)] < player[n].probabilities[min % 13])
					min = CARD_KIND(iter) * 13 + CARD_NUMBER(iter);

			for (j=0,iter=player[n].list->head;iter;iter=iter->next,j++)
				if (CARD_KIND(iter) == min / 13 && CARD_NUMBER(iter) == min % 13)
					break;

			if (min == CARD_KING(min / 13))
				rotation *= (-1);

			lastkind = min / 13;
			playcard(n, min / 13, min % 13, &j);
			moves++;

			if (min == CARD_JOCKER(min / 13)) {
				lastkind = selectbestkind(n);
				update_table(EXPOSURE_KIND);
			}

			if (min == CARD_QUEEN(min / 13))
				update_turn(FLAGS_QUEEN);

			if (min == CARD_TWO(min / 13)) {
				getatmost += 2;
				printf("CPU%d: New static limit is %d cards\n", n, getatmost);
			} else if (min == CARD_ACE(min / 13)) {
				getatmost += 4;
				printf("CPU%d: New static limit is %d cards\n", n, getatmost);
			}

			if (min == CARD_FOUR(min / 13) ||
			    min == CARD_SEVEN(min / 13) ||
			    min == CARD_JOCKER(min / 13) ||
			    (min == CARD_QUEEN(min / 13) &&
			     turn == n)) {
				while (dllst_delitem(alternatives, 0)) {}
				goto playagain;
			} else {
				cpuplayed = TRUE;
			}
		}

fnreturn:
		if (cpuplayed && moves) {
			if (!player[n].list->size) {
				printf("CPU%d finished\n", n);
				player[n].active = FALSE;
				if (getactiveplayers() == 1)
					finish_round();
			}
			update_turn(FLAGS_NONE);
			while (dllst_delitem(alternatives, 0)) {}
			free(alternatives);
			return;
		} else if (!i) {
getmorecards:
			if (getatmost) {
				printf("CPU%d: Getting up to %d cards from the deck\n", n, getatmost);
				for (j=0;j<getatmost;j++) {
					xcard = getcardfromdeck(n, ADD_CARD);
					if (!j &&
					    (xcard == CARD_TWO(xcard / 13) ||
					     xcard == CARD_ACE(xcard / 13))) {
						printf("CPU%d: Skipping to get %d cards from the deck because "
						       "I get an ace/two in the first attempt\n", n, getatmost);

						min = player[n].list->size - 1;
						lastkind = xcard / 13;
						playcard(n, xcard / 13, xcard % 13, &min);
						moves++;

						if (xcard == CARD_TWO(xcard / 13))
							getatmost += 2;
						else if (xcard == CARD_ACE(xcard / 13))
							getatmost += 4;

						cpuplayed = TRUE;
						goto fnreturn;
					}
				}
				getatmost = 0;
				moves++;
			} else {
				getcardfromdeck(n, ADD_CARD);
			}
		}

		i++;
	}

	printf("CPU%d: I have no cards matching kind or number as last played card\n", n);
	update_turn(FLAGS_NONE);
	while (dllst_delitem(alternatives, 0)) {}
	free(alternatives);
	if (getactiveplayers() == 1)
		finish_round();
}

int getactiveplayers(void)
{
	int i, ret = 0;


	for (i=0;i<NPLAYERS;i++)
		if (player[i].active)
			ret++;

	return ret;
}

void finish_round(void)
{
	unsigned j, t, limit;
	char str[32] = { '\0' };
	struct {
		int x0;
		int y0;
		int x1;
		int y1;
	} vtx[4] = { 
			{  0,  0, 16,  0 },
			{ 16,  0, 16, 16 },
			{ 16, 16,  0, 16 },
			{  0, 16,  0,  0 },
		   };


	if (round_finished)
		return;

	for (j=0;j<NPLAYERS;j++)
		if (player[j].active)
			player[j].scores += player[j].list->size;

	del_selector(&resource[RES_DECK],
		     (800 - 2 * (CARD_WIDTH - 7)) / 2 + CARD_WIDTH / 2,
		     CARD_WIDTH / 2,
		     (600 - CARD_HEIGHT) / 2);

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	timer_settime(deck_timer, TIMER_ABSTIME, &its, NULL);
	dialog = gui_newdialog(display, window, "Table of scores",
			       (800 - 300) / 2,
			       (600 - 240) / 2, 300, 240);
	gui_addlabel(display, window, dialog, "Human CPU1   CPU2   CPU3", 10, 24);
	for (t=0;t<NPLAYERS;t++) {
		if (player[t].scores > 16)
			limit = 16;
		else
			limit = player[t].scores;

		for (j=0;j<limit;j++)
			XDrawLine(display, window, gc_black,
				  dialog->x0 + 12 + t * 40 + vtx[j & 3].x0,
				  dialog->y0 + 48 + (j / 4) * 24 + vtx[j & 3].y0,
				  dialog->x0 + 12 + t * 40 + vtx[j & 3].x1,
				  dialog->y0 + 48 + (j / 4) * 24 + vtx[j & 3].y1);
	}

	for (t=0;t<NPLAYERS;t++)
		if (player[t].scores > 16)
			break;

	if (t == NPLAYERS) {
		newround_button = gui_addbutton(display, window, dialog, "New round", 200, 200);
	} else {
		if (!t)
			sprintf(str, "Human ");
		else
			sprintf(str, "CPU%u ", t);
		strcat(str, "looses the game");
		gui_addlabel(display, window, dialog, str, 10, 180);
		newgame_button = gui_addbutton(display, window, dialog, "New game", 200, 200);
	}
	round_finished = TRUE;
}

void do_exposure(XExposeEvent *ep)
{
	int i;


	XFillRectangle(display, window, gc_table, 0, 0, 800, 600);
	render_resource(&resource[RES_DECK], (800 - 2 * (CARD_WIDTH - 7)) / 2, (600 - CARD_HEIGHT) / 2);
	add_selector(&resource[RES_DECK],
		     (800 - 2 * (CARD_WIDTH - 7)) / 2 + CARD_WIDTH / 2,
		     CARD_WIDTH / 2,
		     (600 - CARD_HEIGHT) / 2);

	fields.kind   = CARD_KIND(played_list->tail);
	fields.number = CARD_NUMBER(played_list->tail);
	render_resource(&resource[fields.kind * 13 + fields.number], DECK_PLAYED_X, DECK_PLAYED_Y);

	update_cards(HUMAN, EXPOSURE_CARD, NULL);
	update_cards(CPU1, EXPOSURE_CARD, NULL);
	update_cards(CPU2, EXPOSURE_CARD, NULL);
	update_cards(CPU3, EXPOSURE_CARD, NULL);

	XDrawImageString(display, window, gc_white, HUMAN_X, HUMAN_Y, "Human", strlen("Human"));
	XDrawImageString(display, window, gc_white, CPU1_X, CPU1_Y, "CPU1", strlen("CPU1"));
	XDrawImageString(display, window, gc_white, CPU2_X, CPU2_Y, "CPU2", strlen("CPU2"));
	XDrawImageString(display, window, gc_white, CPU3_X, CPU3_Y, "CPU3", strlen("CPU3"));

	for (i=0;i<NPLAYERS;i++)
		if (turn == i)
			render_resource(&resource[RES_PLAYING_ENABLED], playing_x[i], playing_y[i]);
		else
			render_resource(&resource[RES_PLAYING_DISABLED], playing_x[i], playing_y[i]);

	update_table(EXPOSURE_KIND);
	if (rotation == 1)
		render_resource(&resource[RES_ARROW_CW], ARROW_X, ARROW_Y);
	else
		render_resource(&resource[RES_ARROW_CCW], ARROW_X, ARROW_Y);
}

void do_buttondown(XButtonEvent *bp)
{
	unsigned long i, j, limit, xcard;
	boolean_t humanplayed = FALSE;
	dllst_item_struct_t *iter;
	static int moves = 0;
	static int prevmoves = 0;
	static boolean_t vlock = FALSE;


	if (round_finished) {
		if ((newround_button && (bp->x > newround_button->x0 && bp->x < newround_button->x1 &&
		                         bp->y > newround_button->y0 && bp->y < newround_button->y1)) ||
		    (newgame_button && (bp->x > newgame_button->x0 && bp->x < newgame_button->x1 &&
		                        bp->y > newgame_button->y0 && bp->y < newgame_button->y1))) {
			if (vlock)
				return;

			vlock = TRUE;
			while (dllst_delitem(played_list, 0)) {}
			free(played_list);
			played_list = NULL;
			while (dllst_delitem(deck_list, 0)) {}
			free(deck_list);
			deck_list = NULL;
			for (i=0;i<NPLAYERS;i++) {
				while (dllst_delitem(player[i].list, 0)) {}
				free(player[i].list);
				player[i].list = NULL;
			}

			for (i=0;i<52;i++)
				del_selector(&resource[i], 0, 0, 0);

			if (newgame_button && (bp->x > newgame_button->x0 && bp->x < newgame_button->x1 &&
			                       bp->y > newgame_button->y0 && bp->y < newgame_button->y1))
				for (i=0;i<NPLAYERS;i++)
					player[i].scores = 0;

			round_finished = FALSE;
			free(newround_button);
			newround_button = NULL;
			free(newgame_button);
			newgame_button = NULL;
			gui_destroydialog(display, window, dialog);
			init_round();
			do_exposure(&event.xexpose);
			vlock = FALSE;
		}
	}

	if (turn != HUMAN)
		return;

	switch (bp->button) {
	case Button1:
		if (bp->x > resource[RES_DECK].region.x0 && bp->x < resource[RES_DECK].region.x1 &&
		    bp->y > resource[RES_DECK].region.y0 && bp->y < resource[RES_DECK].region.y1) {
			if (getatmost)
				limit = getatmost;
			else
				limit = 1;

			if (getatmost)
				printf("HUMAN: Getting up to %lu cards from the deck\n", limit);
			for (;moves<limit;moves++) {
				xcard = getcardfromdeck(HUMAN, ADD_CARD);
				if (!moves && limit > 1 &&
				    (xcard == CARD_TWO(xcard / 13) ||
				     xcard == CARD_ACE(xcard / 13))) {
					printf("HUMAN: Skipping to get %lu cards from the deck because "
					       "I get an ace/two in the first attempt\n", limit);
					update_cards(HUMAN, GET_CARD, NULL);
					moves = prevmoves = 1;
					return;
				}
			}

			getatmost = 0;
			break;
		}

		for (i=0;i<4;i++) {
			if (bp->x > resource[RES_KIND_BASE + i].region.x0 &&
			    bp->x < resource[RES_KIND_BASE + i].region.x1 &&
			    bp->y > resource[RES_KIND_BASE + i].region.y0 &&
			    bp->y < resource[RES_KIND_BASE + i].region.y1) {
				update_table(SELECT_NONE);
				lastkind = i;
				update_table(EXPOSURE_KIND);
				update_cards(HUMAN, EXPOSURE_CARD, NULL);
				return;
			}
		}

		for (i=0;i<52;i++) {
			if (bp->x > resource[i].region.x0 && bp->x < resource[i].region.x1 &&
			    bp->y > resource[i].region.y0 && bp->y < resource[i].region.y1) {
				for (iter=player[HUMAN].list->head,j=0;iter;iter=iter->next,j++) {
					if (i == CARD_KIND(iter) * 13 + CARD_NUMBER(iter)) {
						moves = prevmoves = 0;

						if (i != CARD_FOUR(CARD_KIND(iter)) &&
						    i != CARD_SEVEN(CARD_KIND(iter)) &&
						    i != CARD_JOCKER(CARD_KIND(iter))) {
							humanplayed = TRUE;
						} else {
							its.it_interval.tv_sec = its.it_value.tv_sec = 0;
							its.it_interval.tv_nsec = its.it_value.tv_nsec = 125000000;
							timer_settime(playing_timer, TIMER_ABSTIME, &its, NULL);
						}

						if (i == CARD_TWO(CARD_KIND(iter))) {
							getatmost += 2;
							printf("HUMAN: New static limit is %d cards\n", getatmost);
						} else if (i == CARD_ACE(CARD_KIND(iter))) {
							getatmost += 4;
							printf("HUMAN: New static limit is %d cards\n", getatmost);
						} else {
							getatmost = 0;
						}

						if (i == CARD_JOCKER(CARD_KIND(iter)))
							update_table(SELECT_KIND);

						if (i == CARD_KING(CARD_KIND(iter)))
							rotation *= (-1);

						lastkind = CARD_KIND(iter);
						playcard(HUMAN, CARD_KIND(iter), CARD_NUMBER(iter), &j);
						break;
					}
				}

				if (humanplayed) {
					update_table(SELECT_NONE);
					update_table(EXPOSURE_KIND);
					its.it_value.tv_sec = 0;
					its.it_value.tv_nsec = 0;
					timer_settime(playing_timer, TIMER_ABSTIME, &its, NULL);
					timer_settime(table_timer, TIMER_ABSTIME, &its, NULL);
					if (CARD_NUMBER(played_list->tail) == CARD_QUEEN(KIND_CLOVERS) ||
					    CARD_NUMBER(played_list->tail) == CARD_QUEEN(KIND_DIAMONDS) ||
					    CARD_NUMBER(played_list->tail) == CARD_QUEEN(KIND_HEARTS) ||
					    CARD_NUMBER(played_list->tail) == CARD_QUEEN(KIND_PICAS))
						update_turn(FLAGS_QUEEN);
					else
						update_turn(FLAGS_NONE);

					while (turn != HUMAN)
						bot_play(turn);

					if (!getatmost) {
						update_cards(HUMAN, EXPOSURE_CARD, NULL);
						if (!player[HUMAN].list->size) {
							printf("HUMAN finished\n");
							player[HUMAN].active = FALSE;
							update_turn(FLAGS_NONE);
							while (getactiveplayers() > 1)
								bot_play(turn);
							finish_round();
						}
					} else {
						update_cards(HUMAN, GET_CARD, NULL);
					}
				}

				break;
			}
		}
		break;
	case Button3:
		if (bp->x > resource[RES_DECK].region.x0 && bp->x < resource[RES_DECK].region.x1 &&
		    bp->y > resource[RES_DECK].region.y0 && bp->y < resource[RES_DECK].region.y1) {
			if (getatmost)
				limit = getatmost;
			else
				limit = 1;

			if (!humanplayed && moves >= limit) {
				update_table(SELECT_NONE);
				update_table(EXPOSURE_KIND);
				humanplayed = TRUE;
				moves = 0;
				its.it_value.tv_sec = 0;
				its.it_value.tv_nsec = 0;
				timer_settime(playing_timer, TIMER_ABSTIME, &its, NULL);
				timer_settime(table_timer, TIMER_ABSTIME, &its, NULL);
				update_turn(FLAGS_NONE);
				while (turn != HUMAN)
					bot_play(turn);
				if (!getatmost)
					update_cards(HUMAN, EXPOSURE_CARD, NULL);
				else
					update_cards(HUMAN, GET_CARD, NULL);
				break;
			}
		}
		break;
	};
}

void do_timer(union sigval tp)
{
	int j;
	static int i = 0, kind = 0;
	static unsigned long playing_status = 0;


	if (tp.sival_int == 1) {

		// deck_timer
		XFillArc(display, window, gc_anim[i & 7],
			 DECK_PLAYED_X - CARD_WIDTH / 2 - 35, DECK_PLAYED_Y + CARD_HEIGHT + 15, 50, 50,
		 	 i * 45 * 64, 45 * 64);
		if (++i > 8) {
			i = 0;
			its.it_value.tv_sec = 0;
			its.it_value.tv_nsec = 0;
			timer_settime(deck_timer, TIMER_ABSTIME, &its, NULL);
			XFillArc(display, window, gc_table,
				 DECK_PLAYED_X - CARD_WIDTH / 2 - 35, DECK_PLAYED_Y + CARD_HEIGHT + 15,
				 50, 50, 0, 360 * 64);
		}
	} else if (tp.sival_int == 2) {

		// playing_timer
		XLockDisplay(display);
		if (playing_status & 1)
			render_resource(&resource[RES_PLAYING_ENABLED], playing_x[turn], playing_y[turn]);
		else
			render_resource(&resource[RES_PLAYING_DISABLED], playing_x[turn], playing_y[turn]);

		playing_status++;
		XUnlockDisplay(display);
	} else if (tp.sival_int == 3) {

		// table_timer
		XLockDisplay(display);
		for (j=0;j<4;j++)
			if ((kind & 3) == j)
				render_resource(&resource[RES_KIND_BASE_INV + j], KIND_X, KIND_Y(j));
			else
				render_resource(&resource[RES_KIND_BASE + j], KIND_X, KIND_Y(j));
		kind++;
		XUnlockDisplay(display);
	}
}

