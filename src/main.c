/*
 * main.c: the main functions of the game.
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
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>
#undef TRUE
#undef FALSE
#include "gui.h"
#include "dllst.h"
#include "digraph.h"
#include "misc.h"
#include "xmlwrappers.h"
#include "replay.h"

// some convenience defines
#define NPLAYERS		4
#define CARD_WIDTH		73
#define CARD_HEIGHT		97
#define CARD_ACE(x)		(x * 13 + 0)
#define CARD_TWO(x)		(x * 13 + 1)
#define CARD_FOUR(x)		(x * 13 + 3)
#define CARD_SEVEN(x)		(x * 13 + 6)
#define CARD_JACK(x)		(x * 13 + 10)
#define CARD_QUEEN(x)		(x * 13 + 11)
#define CARD_KING(x)		(x * 13 + 12)
#define SUIT_CLUBS		0
#define SUIT_DIAMONDS		1
#define SUIT_HEARTS		2
#define SUIT_SPADES		3
#define SUIT_X			((800 - 2 * (CARD_WIDTH - 7)) / 2 - 32)
#define SUIT_Y(x)		((600 - CARD_HEIGHT) / 2 + x * 26)
#define ARROW_X			(SUIT_X - 100)
#define ARROW_Y			(SUIT_Y(SUIT_CLUBS) + 20)
#define CARD_SUIT(x)		(*((unsigned int *)x->fields + 0))
#define CARD_NUMBER(x)		(*((unsigned int *)x->fields + 2))
#define DECK_X			((800 - 2 * (CARD_WIDTH - 7)) / 2)
#define DECK_Y			((600 - CARD_HEIGHT) / 2)
#define STACK_OF_PLAYED_X	(DECK_X + CARD_WIDTH + 7)
#define STACK_OF_PLAYED_Y	DECK_Y
#define HUMAN			0
#define BOT_1			1
#define BOT_2			2
#define BOT_3			3
#define HUMAN_X			((800 - 5 * (CARD_WIDTH + 7)) / 2 - 60)
#define HUMAN_Y			580
#define BOT_1_X			10
#define BOT_1_Y			128
#define BOT_2_X			((800 - 5 * (CARD_WIDTH + 7)) / 2 - 60)
#define BOT_2_Y			20
#define BOT_3_X			(800 - 10 - CARD_WIDTH)
#define BOT_3_Y			128
#define NRESOURCES		66
#define RES_PLAYING_DISABLED	52
#define RES_PLAYING_ENABLED	53
#define RES_CLUBS		54
#define RES_DIAMONDS		55
#define RES_HEARTS		56
#define RES_SPADES		57
#define RES_CLUBS_INV		58
#define RES_DIAMONDS_INV	59
#define RES_HEARTS_INV		60
#define RES_SPADES_INV		61
#define RES_ARROW_CW		62
#define RES_ARROW_CCW		63
#define RES_DECK		64
#define RES_DECK_LOCKED		65
#define RES_SUIT_BASE		54
#define RES_SUIT_BASE_INV	58

// user-defined types and global variables
typedef enum { EXPOSURE_CARD=0, DELETE_CARD, ADD_CARD, GET_CARD } action_t;
typedef enum { EXPOSURE_SUIT=0, SELECT_SUIT, SELECT_NONE } action_table_t;

Display *display;
Window window;
XEvent event;
Pixmap cardpixmap;
GC gc_table, gc_selector, gc_white, gc_black, gc_anim[8];
timer_t deck_timer, playing_timer, table_timer;
unsigned turn, ngame, nhand;
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
} resource[NRESOURCES] = { 0 };
struct player_st {
	char name[20];
	boolean_t active;		// Whether the player is still active in the hand or not
	dllst_t *list;			// List of cards
	float probabilities[17];	// Bots choose the best card to play depending on probabilities of suit/number
	int scores;			// Total scores in the game
	int specialpts;
} player[NPLAYERS] = { 0 };
dllst_t *deck_list = NULL, *played_list = NULL;
int card_row[NPLAYERS] = { 0 };
int playing_x[NPLAYERS] = { 0 }, playing_y[NPLAYERS] = { 0 };
int getatmost, lastsuit, rotation;
char *suitstr[] = { "CLUBS", "DIAMONDS", "HEARTS", "SPADES" };
boolean_t hand_finished = FALSE;
gui_dialog_t *dialog = NULL;
gui_button_t *newhand_button = NULL, *newgame_button = NULL;
int skipframes = 2;
int game_total = 16;
boolean_t show_bot_cards = FALSE;
char *logfilename = NULL;
char *userdir = NULL;
#if defined(HAVE_XML_LOGS)
boolean_t savelog = FALSE;
int replaygame = 0, replayhand = 0;
xmlDocPtr xml_logfile = NULL, xml_inputfile = NULL;
xmlNodePtr session_node = NULL, game_node = NULL;
xmlNodePtr hand_node = NULL, turn_node = NULL, node = NULL;
#endif
struct {
	unsigned int suit;
	unsigned int unused0;
	unsigned int number;
	unsigned int unused1;
} fields = { 0 };

// local function declarations
int load_resource(struct resource_st *res, const char *filename, int xoffset, int stop);
void render_resource(struct resource_st *res, int X, int Y);
void init_deck(void);
void lock_deck(void);
void unlock_deck(void);
int init_players(int n);
void init_hand(void);
void add_selector(struct resource_st *card, int x, int dx, int y);
void del_selector(struct resource_st *card, int x, int dx, int y);
void update_cards(int nplayer, action_t act, void *param);
void update_table(action_table_t act);
void update_turn(void);
char *decode_card(int suit, int number, boolean_t addnode);
void animate_card(int nplayer, boolean_t isplaying, int suit, int number);
int getcardfromdeck(int nplayer, action_t act);
void playcard(int n, int suit, int number, unsigned long *param);
void bot_calc_probabilities(int n);
void bot_play(int n);
int getactiveplayers(void);
void finish_hand(void);
void do_exposure(XExposeEvent *ep);
void do_buttondown(XButtonEvent *bp);
void do_timer(union sigval tp);
void do_exit(void);

int main(int argc, char **argv, char **env)
{
	int i, j, opt;
	char filename[96] = { '\0' };
	char str[64] = { '\0' };
	Pixmap iconpixmap = { 0 };
	unsigned iconwidth, iconheight;
	XSizeHints hints;
	struct option longoptions[] = {
		{ "name",       required_argument, NULL, 'n' },
		{ "debug",      no_argument, NULL, 'd' },
		{ "skipframes", required_argument, NULL, 'f' },
		{ "total",      required_argument, NULL, 't' },
#if defined(HAVE_XML_LOGS)
		{ "savelog",    no_argument, NULL, 's' },
		{ "logfile",    required_argument, NULL, 'l' },
		{ "replay-file",required_argument, NULL, 'R' },
		{ "replay-hand",required_argument, NULL, 'H' },
		{ "list-hands", required_argument, NULL, 'L' },
#endif
		{ "version",    no_argument, NULL, 'v' },
		{ "help",       no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};
#if defined(HAVE_XML_LOGS)
	xmlNodePtr rootnode = NULL;
#endif


	XInitThreads();
	display = XOpenDisplay("");
	window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 0, 0, 800, 600, 1, 0, 0);
	XSelectInput(display, window, ExposureMask|ButtonPressMask);
	hints.min_width = 800;
	hints.min_height = 600;
	hints.max_width = 800;
	hints.max_height = 600;
	hints.flags = PMinSize|PMaxSize;
	XReadBitmapFile(display, window, "/usr/local/share/nullify/res/icon.xbm",
			&iconwidth, &iconheight, &iconpixmap, NULL, NULL);
	XSetStandardProperties(display, window, PACKAGE_STRING, NULL, iconpixmap, argv, argc, &hints);
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

	// Read game settings from the installation data directory
	if (parse_conf_file("/usr/local/share/nullify/res/nullify.conf")) {
		strcpy(player[0].name, "Human");
		strcpy(player[1].name, "Bot_1");
		strcpy(player[2].name, "Bot_2");
		strcpy(player[3].name, "Bot_3");
		printf("Failed to get default values from config file\n");
	}

	// Parse the command line options
	while ((opt = getopt_long(argc, argv, "n:df:sl:t:R:H:L:vh", longoptions, NULL)) != -1) {
		switch (opt) {
		case 'n':
			strncpy(player[0].name, optarg, 19);
			break;
		case 'd':
			show_bot_cards = TRUE;
			break;
		case 'f':
			skipframes = strtol(optarg, NULL, 10);
			if (skipframes < 1)
				skipframes = 1;
			break;
		case 't':
			game_total = strtol(optarg, NULL, 10);
			if (game_total < 1)
				game_total = 16;
			break;
#if defined(HAVE_XML_LOGS)
		case 's':
			savelog = TRUE;
			break;
		case 'l':
			free(logfilename);
			logfilename = NULL;
			logfilename = (char *)calloc(1, strlen(optarg) + 1);
			if (logfilename)
				strcat(logfilename, optarg);
			break;
		case 'R':
			rootnode = parse_xml_session(optarg);
			break;
		case 'H':
			replaygame = get_nth_field(optarg, 0);
			replayhand = get_nth_field(optarg, 1);
			break;
		case 'L':
			printf("Reading games and hands from %s:\n", optarg);
			printf("Games\tHands\n");
			replay_list_hands(parse_xml_session(optarg));
			return 0;
			break;
#endif
		case 'v':
			printf("%s\nPlease report any bug you find to <%s>\n",
				PACKAGE_STRING, PACKAGE_BUGREPORT);
			return 0;
		case 'h':
		default:
			goto usage;
		};
	}

#if defined(HAVE_XML_LOGS)
	// Create the root node of the XML file
	do_LIBXML_TEST_VERSION;
	do_xmlNewDoc(xml_logfile, "1.0");
	do_xmlNewNode(session_node, "session");
	do_xmlDocSetRootElement(xml_logfile, session_node);
	do_xmlCreateIntSubset(xml_logfile, "session", "/usr/local/share/nullify/res/nullify.dtd");

	for (i=0;i<4;i++) {
		do_xmlNewChild(node, session_node, "player", player[i].name);
		sprintf(str, "%d", i);
		do_xmlNewProp(node, "id", str);
	}

	do_xmlNewNode(game_node, "game");
	sprintf(str, "%u", ngame);
	do_xmlNewProp(game_node, "id", str);
	do_xmlAddChild(session_node, game_node);

	if (rootnode) {
		xml_inputfile = replay_select_hand(rootnode, replaygame, replayhand);
		if (!xml_inputfile) {
			printf("Requested game:hand is inexistent.\n"
				"Use \"--list-hands=<file>\" option to list "
				"available hands from <file>\n");
			return 1;
		}
	}
#endif

	dllst_verbose = 0;
	for (i=0;i<4;i++) {
		strcpy(filename, "/usr/local/share/nullify/res/");
		switch (i) {
		case 0:
			strcat(filename, "suit-clubs.png");
			break;
		case 1:
			strcat(filename, "suit-diamonds.png");
			break;
		case 2:
			strcat(filename, "suit-hearts.png");
			break;
		case 3:
			strcat(filename, "suit-spades.png");
			break;
		};

		for (j=0;j<13;j++)
			load_resource(&resource[i * 13 + j], filename, j * CARD_WIDTH, CARD_WIDTH);
		memset(filename, '\0', 96);
	}
	load_resource(&resource[RES_PLAYING_DISABLED], "/usr/local/share/nullify/res/playing_disabled.png", -1, -1);
	load_resource(&resource[RES_PLAYING_ENABLED], "/usr/local/share/nullify/res/playing_enabled.png", -1, -1);
	load_resource(&resource[RES_CLUBS], "/usr/local/share/nullify/res/clubs.png", -1, -1);
	load_resource(&resource[RES_DIAMONDS], "/usr/local/share/nullify/res/diamonds.png", -1, -1);
	load_resource(&resource[RES_HEARTS], "/usr/local/share/nullify/res/hearts.png", -1, -1);
	load_resource(&resource[RES_SPADES], "/usr/local/share/nullify/res/spades.png", -1, -1);
	load_resource(&resource[RES_CLUBS_INV], "/usr/local/share/nullify/res/clubs-inv.png", -1, -1);
	load_resource(&resource[RES_DIAMONDS_INV], "/usr/local/share/nullify/res/diamonds-inv.png", -1, -1);
	load_resource(&resource[RES_HEARTS_INV], "/usr/local/share/nullify/res/hearts-inv.png", -1, -1);
	load_resource(&resource[RES_SPADES_INV], "/usr/local/share/nullify/res/spades-inv.png", -1, -1);
	load_resource(&resource[RES_ARROW_CW], "/usr/local/share/nullify/res/arrow-cw.png", -1, -1);
	load_resource(&resource[RES_ARROW_CCW], "/usr/local/share/nullify/res/arrow-ccw.png", -1, -1);
	load_resource(&resource[RES_DECK], "/usr/local/share/nullify/res/deck.png", -1, -1);
	load_resource(&resource[RES_DECK_LOCKED], "/usr/local/share/nullify/res/deck-locked.png", -1, -1);

	card_row[0] = 600 - CARD_HEIGHT - 10;
	card_row[2] = 10;
	playing_x[HUMAN] = (800 - 32) / 2;
	playing_y[HUMAN] = card_row[HUMAN] - 56;
	playing_x[BOT_1]  = CARD_WIDTH + 32;
	playing_y[BOT_1]  = (600 - 32) / 2;
	playing_x[BOT_2]  = (800 - 32) / 2;
	playing_y[BOT_2]  = card_row[BOT_1] + 125;
	playing_x[BOT_3]  = 800 - CARD_WIDTH - 10 - 64;
	playing_y[BOT_3]  = (600 - 32) / 2;

	do_timer_prepare(&deck_timer, do_timer, 1);
	do_timer_prepare(&playing_timer, do_timer, 2);
	do_timer_prepare(&table_timer, do_timer, 3);

	userdir = expand_tilde(env);
	atexit(do_exit);

	XNextEvent(display, &event);
	if (event.type == Expose) {
		dialog = gui_newdialog(display, window, "Nullify", 60, 80, 680, 450);
		print_rules(dialog, "/usr/local/share/nullify/res/rules.txt");
		newhand_button = gui_addbutton(dialog, "Start", 620, 400);
		hand_finished = TRUE;
		cardpixmap = XCreatePixmap(display, window, CARD_WIDTH, CARD_HEIGHT,
					  XDefaultScreenOfDisplay(display)->root_depth);
	}

	// X main loop
	while (1) {
		if (XCheckMaskEvent(display, ExposureMask|ButtonPressMask, &event)) {
			switch(event.type) {
			case Expose:
				if (newhand_button || newgame_button)
					init_hand();
				if (!event.xexpose.count)
					do_exposure(&event.xexpose);

				hand_finished = FALSE;
				free(newhand_button);
				newhand_button = NULL;
				free(newgame_button);
				newgame_button = NULL;
				break;
			case ButtonPress:
				do_buttondown(&event.xbutton);
				break;
			};
		}
	}

usage:
	printf("%s\n", PACKAGE_STRING);
	printf("Usage: %s [options]\n", PACKAGE);
	printf("Available options are:\n");
	printf("  -n --name=<yourname>     Set your name to something other than \"Human\"\n");
	printf("  -d --debug               It allows show the cards of the bots\n");
	printf("  -f --skipframes=<n>      Set the amount of frames to skip during animations.\n");
	printf("                           <n> must be an integer different than zero\n");
	printf("\n");
	printf("  -t --total=<n>           Specify play each game up to <n> points is to be reached\n");
	printf("                           (default = 16)\n");
#if defined(HAVE_XML_LOGS)
	printf("  -s --savelog             Save the session log when the game exits (it must have\n");
	printf("                           been compiled with the --enable-xml-logs=yes option)\n");
	printf("\n");
	printf("  -l --logfile=<filename>  Save the session log to <filename> rather than\n");
	printf("                           to the default file \"~/.nullify-session.xml\"\n");
	printf("\n");
	printf("  -R --replay-file=<file>  Reproduce a saved game by reading it from XML <file>\n");
	printf("  -H --replay-hand=<g:h>   Reproduce the specified hand <h> of the game <g> only\n");
	printf("  -L --list-hands=<file>   Show available games and hands to replay from XML session\n");
	printf("                           <file> and exit\n");
#endif
	printf("  -v --version             Print the program version and exit\n");
	printf("  -h --help                Display this message\n");
	return -1;
}

/*
 *
 * Load png-image @filename and store it in @res. @xoffset and @stop, if given
 * other than -1, allow load the subimage starting at (@xoffset, 0) and ending at
 * (@xoffset + @stop, image-height) coordinates.
 *
 */
int load_resource(struct resource_st *res, const char *filename, int xoffset, int stop)
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
	png_bytepp subimage = NULL;
	unsigned width, height;


	fileptr = fopen(filename, "rb");
	if (!fileptr) {
		perror("fopen");
		return -1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infoptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, fileptr);
	png_read_info(png_ptr, png_infoptr);

// This should work on the most of the systems. See BUGS for further information.
#if PNG_LIBPNG_VER >= 10500
	// libpng-1.5.0+
	width = png_get_image_width(png_ptr, png_infoptr);
	height = png_get_image_height(png_ptr, png_infoptr);
#else
	width = png_infoptr->width;
	height = png_infoptr->height;
#endif

	res->bytes = (png_bytepp)malloc(height * sizeof(png_bytep));
	if (!res->bytes) {
		perror("malloc");
		return -1;
	}
	
	for (y=0;y<height;y++) {
		res->bytes[y] = (png_bytep)malloc(width * 3);
		if (!res->bytes[y]) {
			perror("malloc");
			return -1;
		}
	}
	png_read_image(png_ptr, res->bytes);
	png_read_end(png_ptr, png_infoptr);

	if (xoffset < width && stop != -1) {
		subimage = (png_bytepp)malloc(height * sizeof(png_bytep));
		if (!subimage) {
			perror("malloc");
			return -1;
		}

		for (y=0;y<height;y++) {
			subimage[y] = (png_bytep)malloc(stop * 3);
			if (!subimage[y]) {
				perror("malloc");
				return -1;
			}
		}

		for (y=0;y<height;y++)
			for (x=0;x<stop * 3;x++)
				subimage[y][x] = res->bytes[y][xoffset * 3 + x];

		for (y=0;y<height;y++) {
			free(res->bytes[y]);
			res->bytes[y] = NULL;
		}
		free(res->bytes);
		res->bytes = NULL;

		res->bytes = (png_bytepp)malloc(height * sizeof(png_bytep));
		if (!res->bytes) {
			perror("malloc");
			return -1;
		}
		
		for (y=0;y<height;y++) {
			res->bytes[y] = (png_bytep)malloc(stop * 3);
			if (!res->bytes[y]) {
				perror("malloc");
				return -1;
			}
		}

		for (y=0;y<height;y++)
			for (x=0;x<stop * 3;x++)
				res->bytes[y][x] = subimage[y][x];

		for (y=0;y<height;y++)
			free(subimage[y]);
		free(subimage);
		width = stop;
	}

	dllst = dllst_initlst(dllst, "I:");
	for (y=0;y<height;y++) {
		for (x=0;x<width * 3;x+=3) {
			fields2.color = (res->bytes[y][x + 2] <<  0)|
					(res->bytes[y][x + 1] <<  8)|
					(res->bytes[y][x + 0] << 16);
			if (!dllst_isinlst(dllst, &fields2))
				dllst_newitem(dllst, &fields2);
		}
	}

	res->height = height;
	res->width  = width;
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
	char card[8] = { '\0' };
	char buf[256] = { '\0' };
	dllst_item_struct_t *iter;


	deck_list = dllst_initlst(deck_list, "I:I:");
	for (i=0;i<4;i++) {
		for (j=0;j<13;j++) {
			fields.suit = (unsigned int)i;
			fields.number = (unsigned int)j;
			dllst_newitem(deck_list, &fields);
		}
	}

	if (deck_list->size != 52)
		return;

	for (i=0;i<52;i++)
		dllst_swapitems(deck_list, i, rand() % 52);

	for (iter=deck_list->head;iter;iter=iter->next) {
		sprintf(card, "%02d ", CARD_SUIT(iter) * 13 + CARD_NUMBER(iter));
		strcat(buf, card);
	}

	do_xmlNewNode(node, "deck");
	do_xmlNewProp(node, "list", buf);
	do_xmlAddChild(hand_node, node);

	getatmost = 0;
}

/*
 *
 * Prevent the deck from being clicked and display a big "X" on it.
 *
 */
void lock_deck(void)
{
	del_selector(&resource[RES_DECK], DECK_X + CARD_WIDTH / 2, CARD_WIDTH / 2, DECK_Y);

	render_resource(&resource[RES_DECK_LOCKED], DECK_X, DECK_Y);
	resource[RES_DECK_LOCKED].region.x0 = DECK_X;
	resource[RES_DECK_LOCKED].region.y0 = DECK_Y;
	resource[RES_DECK_LOCKED].region.x1 = DECK_X + CARD_WIDTH;
	resource[RES_DECK_LOCKED].region.y1 = DECK_Y + CARD_HEIGHT;
}

/*
 *
 * Enable clicks on deck coordinates and display "Tux" on it.
 *
 */
void unlock_deck(void)
{
	del_selector(&resource[RES_DECK_LOCKED], DECK_X + CARD_WIDTH / 2, CARD_WIDTH / 2, DECK_Y);

	render_resource(&resource[RES_DECK], DECK_X, DECK_Y);
	add_selector(&resource[RES_DECK], DECK_X + CARD_WIDTH / 2, CARD_WIDTH / 2, DECK_Y);
}

/*
 *
 * Initialize the players to 5 cards get from the deck.
 *
 */
int init_players(int n)
{
	int i, j;
	char card[4] = { '\0' };
	char buf[32] = { '\0' };
	char str[64] = { '\0' };
	dllst_item_struct_t *iter;


	for (i=0;i<n;i++) {
		// Player is active in the current hand (i.e., this bot can play cards when in turn)
		player[i].active = TRUE;

		// Reset the special points collected in the previous hand
		player[i].specialpts = 0;

		player[i].list = dllst_initlst(player[i].list, "I:I:");
		for (j=0;j<5;j++) {
			fields.suit   = CARD_SUIT(deck_list->head);
			fields.number = CARD_NUMBER(deck_list->head);
			dllst_newitem(player[i].list, &fields);

			// Delete the first item (top card) of deck_list
			dllst_delitem(deck_list, 0);
		}

		sprintf(str, "%d", i);

		for (iter=player[i].list->head;iter;iter=iter->next) {
			sprintf(card, "%02d ", CARD_SUIT(iter) * 13 + CARD_NUMBER(iter));
			strcat(buf, card);
		}

		do_xmlNewNode(node, "playerlist");
		do_xmlNewProp(node, "id", str);
		do_xmlNewProp(node, "list", buf);
		do_xmlAddChild(hand_node, node);

		for (j=0;j<strlen(buf);j++)
			buf[j] = '\0';
	}

	return 0;
}

void init_hand(void)
{
	long now = 0;
	char str[64] = { '\0' }, *rseedstr = NULL;
	xmlNodePtr current_node = NULL;


#if defined(HAVE_XML_LOGS)
	if (!xml_inputfile) {
		now = time(NULL);
	} else {
		// get the random seed attribute of the element 'hand'
		do_xmlDocGetRootElement(current_node, xml_inputfile);
		current_node = replay_get_next_matching_iter(current_node, "hand");
		do_xmlGetProp(rseedstr, current_node, "rseed");
		if (rseedstr)
			now = strtol(rseedstr, NULL, 0x10);
		current_node = replay_get_next_matching_iter(current_node, "turn");
	}
#else
	now = time(NULL);
#endif
	srand(now);

	do_xmlNewNode(hand_node, "hand");
	sprintf(str, "%u", nhand);
	do_xmlNewProp(hand_node, "id", str);
	sprintf(str, "0x%016lX", now);
	do_xmlNewProp(hand_node, "rseed", str);
	do_xmlAddChild(game_node, hand_node);

	init_deck();
	init_players(NPLAYERS);

	do_timer_unset(deck_timer);
	do_timer_unset(playing_timer);
	do_timer_unset(table_timer);

	played_list = dllst_initlst(played_list, "I:I:");
	fields.suit   = CARD_SUIT(deck_list->head);
	fields.number = CARD_NUMBER(deck_list->head);
	dllst_newitem(played_list, &fields);
	dllst_delitem(deck_list, 0);

	turn = HUMAN;
	getatmost = 0;
	lastsuit = CARD_SUIT(played_list->tail);
	rotation = 1;

#if defined(HAVE_XML_LOGS)
	if (xml_inputfile) {
		savelog = FALSE;
		do_exposure(&event.xexpose);
		replay_hand(current_node);
		return;
	}
#endif

	printf("%s:\n", player[turn].name);

	do_xmlNewNode(turn_node, "turn");
	do_xmlNewProp(turn_node, "id", "0");
	do_xmlAddChild(hand_node, turn_node);
}

/*
 *
 * Calculate probabilities for each suit / number of card in card list of the bot.
 * Bots select the card to play based on the suit/number of the card with less
 * probability than others.
 * As of v0.3.1, the denominator is no longer a constant value for number and
 * suit quotients. Instead, it now represents the amount of cards which neither
 * belong to the @n bot nor to the stack of played cards, that is, the total
 * number of cards of other players.
 *
 */
void bot_calc_probabilities(int n)
{
	int i;
	unsigned counter[17];
	dllst_item_struct_t *iter;
	float total;


	for (i=0;i<13;i++)
		counter[i] = 4;
	for (i=13;i<17;i++)
		counter[i] = 13;

	total = 52 - player[n].list->size - played_list->size;

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
		player[n].probabilities[i] = (float)counter[i] / total;

	for (i=0;i<4;i++) {
		for (iter=player[n].list->head;iter;iter=iter->next)
			if (CARD_SUIT(iter) == i)
				counter[i + 13]--;
		for (iter=played_list->head;iter;iter=iter->next)
			if (CARD_SUIT(iter) == i)
				counter[i + 13]--;
	}

	// probabilities for the 4 suits
	for (i=13;i<17;i++)
		player[n].probabilities[i] = (float)counter[i] / total;
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


	if (nplayer == HUMAN || nplayer == BOT_2) {
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
			if (nplayer == HUMAN) {
				if (!iter->next) {
					del_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
						     i * sep + xstart + CARD_WIDTH / 2,
						     CARD_WIDTH / 2,
						     card_row[HUMAN]);
				} else {
					del_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
						     i * sep + xstart + sep / 2,
						     sep / 2,
						     card_row[HUMAN]);
				}
			}
		}
	} else {
		if (nplayer == BOT_1)
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

	if (nplayer == HUMAN || nplayer == BOT_2) {
		if (player[nplayer].list->size > 5) {
			sep = (CARD_WIDTH + 7) * 5 / player[nplayer].list->size;
			xstart = (800 - 400) / 2;
		} else {
			sep = CARD_WIDTH + 7;
			xstart = (800 - player[nplayer].list->size * (CARD_WIDTH + 7)) / 2;
		}

		for (i=0,iter=player[nplayer].list->head;iter;iter=iter->next,i++) {
			if (nplayer == HUMAN) {
				render_resource(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
					    i * sep + xstart, card_row[nplayer]);
			} else {
				if (show_bot_cards)
					render_resource(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
						    i * sep + xstart, card_row[nplayer]);
				else
					render_resource(&resource[RES_DECK], i * sep + xstart, card_row[nplayer]);
			}
			if (nplayer == HUMAN) {
				if (act == GET_CARD) {
					if (CARD_NUMBER(iter) == CARD_TWO(SUIT_CLUBS)    ||
					    CARD_NUMBER(iter) == CARD_TWO(SUIT_DIAMONDS) ||
					    CARD_NUMBER(iter) == CARD_TWO(SUIT_HEARTS)   ||
					    CARD_NUMBER(iter) == CARD_TWO(SUIT_SPADES)   ||
					    CARD_NUMBER(iter) == CARD_ACE(SUIT_CLUBS)    ||
					    CARD_NUMBER(iter) == CARD_ACE(SUIT_DIAMONDS) ||
					    CARD_NUMBER(iter) == CARD_ACE(SUIT_HEARTS)   ||
					    CARD_NUMBER(iter) == CARD_ACE(SUIT_SPADES)) {
						if (!iter->next) {
							add_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
								     i * sep + xstart + CARD_WIDTH / 2,
								     CARD_WIDTH / 2,
								     card_row[HUMAN]);
						} else {
							add_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
								     i * sep + xstart + sep / 2,
								     sep / 2,
								     card_row[HUMAN]);
						}
					}
					continue;
				}

				if (CARD_SUIT(iter) == lastsuit ||
				    CARD_NUMBER(iter) == CARD_NUMBER(played_list->tail)) {
					if (!iter->next) {

						/*
						 * This is the last card of the list. We add a slightly modified
						 * selector to allow to the user select this card from
						 * (x - CARD_WIDTH / 2) <= mouse_x <= (x + CARD_WIDTH / 2)
						 */
						add_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
							     i * sep + xstart + CARD_WIDTH / 2,
							     CARD_WIDTH / 2,
							     card_row[HUMAN]);
					} else {
						add_selector(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
							     i * sep + xstart + sep / 2,
							     sep / 2,
							     card_row[HUMAN]);
					}
				}
			}
		}
	} else {
		if (nplayer == BOT_1)
			xstart = 10;
		else
			xstart = 800 - CARD_WIDTH - 10;

		if (player[nplayer].list->size > 5) {
			sep  = (600 - 5 * 60) / player[nplayer].list->size;
		} else {
			sep = (600 - 5 * (CARD_HEIGHT + 3)) / 2;
		}

		if (!show_bot_cards) {
			for (i=0;i<player[nplayer].list->size;i++)
				render_resource(&resource[RES_DECK], xstart, i * sep + 150);
		} else {
			for (i=0,iter=player[nplayer].list->head;iter;iter=iter->next,i++)
				render_resource(&resource[CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)],
					    xstart, i * sep + 150);
		}
	}
}

/*
 *
 * Update the status of the suit selectors and prepare their regions to be clicked
 * by the human player as long as last card played is a jack.
 *
 */
void update_table(action_table_t act)
{
	int i;


	if (act == EXPOSURE_SUIT) {
		for (i=0;i<4;i++)
			if (i == lastsuit)
				render_resource(&resource[RES_SUIT_BASE_INV + i], SUIT_X, SUIT_Y(i));
			else
				render_resource(&resource[RES_SUIT_BASE + i], SUIT_X, SUIT_Y(i));
	} else if (act == SELECT_SUIT) {
		for (i=0;i<4;i++) {
			resource[RES_SUIT_BASE + i].region.x0 = SUIT_X;
			resource[RES_SUIT_BASE + i].region.y0 = SUIT_Y(i);
			resource[RES_SUIT_BASE + i].region.x1 = SUIT_X + resource[RES_SUIT_BASE + i].width;
			resource[RES_SUIT_BASE + i].region.y1 = SUIT_Y(i) + resource[RES_SUIT_BASE + i].height;
		}
		do_timer_set(table_timer, 0, 125000000);
	} else if (act == SELECT_NONE) {
		for (i=0;i<4;i++) {
			resource[RES_SUIT_BASE + i].region.x0 = -1;
			resource[RES_SUIT_BASE + i].region.y0 = -1;
			resource[RES_SUIT_BASE + i].region.x1 = -1;
			resource[RES_SUIT_BASE + i].region.y1 = -1;
		}
		do_timer_unset(table_timer);
	}
}

/*
 *
 * Update the turn according to rotation and skipping modes. Rotation mode can be
 * toggled clockwise / counter-clockwise by playing a king. Skipping mode is automatically
 * set if last card played was a queen, so that the turn is incremented or decremented twice
 * (e.g., rotation mode is set to ccw and turn is currently set to HUMAN; he/she plays a queen,
 * then the next players in turn are BOT_2, BOT_1, HUMAN, BOT_3, etc).
 *
 */
void update_turn(void)
{
	render_resource(&resource[RES_PLAYING_DISABLED], playing_x[turn], playing_y[turn]);
	if (CARD_NUMBER(played_list->tail) == CARD_QUEEN(SUIT_CLUBS) % 13    ||
	    CARD_NUMBER(played_list->tail) == CARD_QUEEN(SUIT_DIAMONDS) % 13 ||
	    CARD_NUMBER(played_list->tail) == CARD_QUEEN(SUIT_HEARTS) % 13   ||
	    CARD_NUMBER(played_list->tail) == CARD_QUEEN(SUIT_SPADES) % 13) {
		if (rotation == 1) {
			while (!player[(turn + 1) & 3].active) {
				turn++;
				turn &= 3;
			}
			turn++;
			turn &= 3;
		} else {
			while (!player[(turn - 1) & 3].active) {
				turn--;
				turn &= 3;
			}
			turn--;
			turn &= 3;
		}
	}

	if (rotation == 1) {
		while (!player[(turn + 1) & 3].active) {
			turn++;
			turn &= 3;
		}
		turn++;
		turn &= 3;
	} else {
		while (!player[(turn - 1) & 3].active) {
			turn--;
			turn &= 3;
		}
		turn--;
		turn &= 3;
	}
	render_resource(&resource[RES_PLAYING_ENABLED], playing_x[turn], playing_y[turn]);

	printf("%s:\n", player[turn].name);
}

/*
 *
 * Get a human readable string corresponding to the card of suit @suit
 * and number @number. If @addnode is TRUE, a node is added to the XML
 * session file. The converse mode of operation of this function is also
 * supported: see misc.c:decode_card_rev() for more detail.
 *
 */
char *decode_card(int suit, int number, boolean_t addnode)
{
	char card[4] = { '\0' }, *buf = NULL;


	buf = (char *)calloc(1, 4);
	if (!buf)
		return NULL;

	sprintf(card, "%2d", (number & 0xff) + 1);
	switch (number) {
	case 0:
		card[0] = ' ';
		card[1] = 'A';
		break;
	case 10:
		card[0] = ' ';
		card[1] = 'J';
		break;
	case 11:
		card[0] = ' ';
		card[1] = 'Q';
		break;
	case 12:
		card[0] = ' ';
		card[1] = 'K';
		break;
	};

	switch (suit) {
	case SUIT_CLUBS:
		strcat(card, "c");
		break;
	case SUIT_DIAMONDS:
		strcat(card, "d");
		break;
	case SUIT_HEARTS:
		strcat(card, "h");
		break;
	case SUIT_SPADES:
		strcat(card, "s");
		break;
	};

	sprintf(buf, "%s", card);
	if (addnode) {
		printf("\t%s\n", buf);
		do_xmlNewChild(node, turn_node, "card", buf);
	}

	return buf;
}

/*
 *
 * Render animated moves when the player @nplayer either gets a card from
 * the deck or plays the card of suit @suit and number @number. In the latter
 * case, @isplaying must be supplied as TRUE.
 *
 */
void animate_card(int nplayer, boolean_t isplaying, int suit, int number)
{
	int i, t, x, y, incr, obj_x = 0, obj_y = 0, start;
	int dst_x, dst_y, bound;
	boolean_t (*less_or_greater)(int, int);
	float slope, sep;
	dllst_item_struct_t *iter = NULL;


	if (nplayer == HUMAN || nplayer == BOT_2) {
		if (player[nplayer].list->size > 5) {
			sep = (CARD_WIDTH + 7) * 5 / player[nplayer].list->size;
			start = (800 - 400) / 2;
		} else {
			sep = CARD_WIDTH + 7;
			start = (800 - player[nplayer].list->size * (CARD_WIDTH + 7)) / 2;
		}
	} else {
		if (nplayer == BOT_1)
			start = 10;
		else
			start = 800 - CARD_WIDTH - 10;

		if (player[nplayer].list->size > 5)
			sep = (600 - 5 * 60) / player[nplayer].list->size;
		else
			sep = (600 - 5 * (CARD_HEIGHT + 3)) / 2;
	}

	if (!isplaying) {
		if (nplayer == HUMAN || nplayer == BOT_2) {
			obj_x = start + player[nplayer].list->size * sep;
			obj_y = nplayer == HUMAN ? card_row[HUMAN] : card_row[BOT_2];
		} else {
			obj_x = start;
			obj_y = 150 + player[nplayer].list->size * sep;
		}
	} else {
		for (i=0,iter=player[nplayer].list->head;iter;iter=iter->next,i++) {
			if (CARD_SUIT(iter) == suit && CARD_NUMBER(iter) == number) {
				if (nplayer == HUMAN || nplayer == BOT_2) {
					obj_x = start + i * sep;
					obj_y = nplayer == HUMAN ? card_row[HUMAN] : card_row[BOT_2];
				} else {
					obj_x = start;
					obj_y = 150 + i * sep;
				}

				break;
			}
		}
	}

	if (!isplaying) {
		dst_x = DECK_X;
		dst_y = DECK_Y;
	} else {
		dst_x = STACK_OF_PLAYED_X;
		dst_y = STACK_OF_PLAYED_Y;
	}

	if (obj_x != dst_x) {
		slope = (float)(obj_y - dst_y) / (float)(obj_x - dst_x);
		incr = obj_x > dst_x ? 1 : -1;
		if (dst_x == STACK_OF_PLAYED_X)
			incr *= (-1);
		less_or_greater = incr == 1 ? fn_less : fn_greater;
		bound = dst_x == DECK_X ? (obj_x - dst_x) : (dst_x - obj_x);
		if (dst_x != DECK_X) {
			t = dst_x;
			dst_x = obj_x;
			obj_x = t;
			t = dst_y;
			dst_y = obj_y;
			obj_y = t;
		}
		for (x=0;less_or_greater(x, bound);x+=incr * skipframes) {
			XCopyArea(display, window, cardpixmap, gc_white,
				  dst_x + x, dst_y + slope * x, CARD_WIDTH, CARD_HEIGHT, 0, 0);
			if (!isplaying)
				render_resource(&resource[RES_DECK], dst_x + x, dst_y + slope * x);
			else
				render_resource(&resource[suit * 13 + number], dst_x + x, dst_y + slope * x);
			XCopyArea(display, cardpixmap, window, gc_white,
				  0, 0, CARD_WIDTH, CARD_HEIGHT, dst_x + x, dst_y + slope * x);
		}
	} else {
		incr = obj_y > dst_y ? 1 : -1;
		if (dst_y == STACK_OF_PLAYED_Y)
			incr *= (-1);
		less_or_greater = incr == 1 ? fn_less : fn_greater;
		bound = dst_y == DECK_Y ? (obj_y - dst_y) : (dst_y - obj_y);
		if (dst_y != DECK_X) {
			t = dst_y;
			dst_y = obj_y;
			obj_y = t;
			t = dst_x;
			dst_x = obj_x;
			obj_x = t;
		}
		for (y=0;less_or_greater(y, bound);y+=incr * skipframes) {
			XCopyArea(display, window, cardpixmap, gc_white,
				  dst_x, dst_y + y, CARD_WIDTH, CARD_HEIGHT, 0, 0);
			if (!isplaying)
				render_resource(&resource[RES_DECK], dst_x, dst_y + y);
			else
				render_resource(&resource[suit * 13 + number], dst_x, dst_y + y);
			XCopyArea(display, cardpixmap, window, gc_white,
				  0, 0, CARD_WIDTH, CARD_HEIGHT, dst_x, dst_y + y);
		}
	}
}

/*
 *
 * Move one card from the deck to player_list.
 *
 */
int getcardfromdeck(int nplayer, action_t act)
{
	int i;
	long j;
	char card[8] = { '\0' };
	char buf[256] = { '\0' };
	dllst_item_struct_t *iter;


getcard:
	if (deck_list->size) {
		animate_card(nplayer, FALSE, 0, 0);

		fields.suit   = CARD_SUIT(deck_list->head);
		fields.number = CARD_NUMBER(deck_list->head);
		dllst_delitem(deck_list, 0);
		update_cards(nplayer, act, &fields);

		return fields.suit * 13 + fields.number;
	} else {
		do_timer_set(deck_timer, 0, 62500000);
		while (played_list->size > 1) {
			fields.suit   = CARD_SUIT(played_list->head);
			fields.number = CARD_NUMBER(played_list->head);
			dllst_delitem(played_list, 0);
			dllst_newitem(deck_list, &fields);
		}

		// In order to avoid looping forever when one or more players get
		// all the available cards from the deck, we need to add the
		// special 'greedy player' case: that is, the player with more cards
		// than others looses and his/her cards are put back to the deck.
		if (!deck_list->size) {
			for (j=0,i=0;i<NPLAYERS;i++)
				if (player[i].list->size > j)
					j = player[i].list->size;

			for (i=0;i<NPLAYERS;i++)
				if (player[i].list->size == j)
					break;

			player[i].specialpts += player[i].list->size;
			player[i].active = FALSE;

			printf("%s:\n\tThese are the cards returning to the deck:\n", player[i].name);
			for (j=player[i].list->size - 1;j>-1L;j--)
				playcard(i, CARD_SUIT(player[i].list->tail),
					    CARD_NUMBER(player[i].list->tail), &j);

			if (getactiveplayers() == 1) {
				finish_hand();
				return -1;
			} else {
				update_turn();
			}
		}

		for (i=0;i<deck_list->size;i++)
			dllst_swapitems(deck_list, i, rand() % deck_list->size);

		for (iter=deck_list->head;iter;iter=iter->next) {
			sprintf(card, "%02d ", CARD_SUIT(iter) * 13 + CARD_NUMBER(iter));
			strcat(buf, card);
		}

		do_xmlNewNode(node, "deck");
		do_xmlNewProp(node, "list", buf);
		do_xmlAddChild(hand_node, node);

		goto getcard;
	}
}

/*
 *
 * Move one card from player's list of cards to the stack of played cards.
 *
 */
void playcard(int n, int suit, int number, unsigned long *param)
{
	animate_card(n, TRUE, suit, number);

	fields.suit   = suit;
	fields.number = number;
	dllst_newitem(played_list, &fields);
	render_resource(&resource[suit * 13 + number], STACK_OF_PLAYED_X, STACK_OF_PLAYED_Y);
	update_cards(n, DELETE_CARD, param);
	update_table(EXPOSURE_SUIT);

	if (rotation == -1)
		render_resource(&resource[RES_ARROW_CCW], ARROW_X, ARROW_Y);
	else
		render_resource(&resource[RES_ARROW_CW], ARROW_X, ARROW_Y);

	decode_card(suit, number, TRUE);
}

/*
 *
 * Main function of bots game.
 *
 * Rain of gotos. Yes, I know, but label names are descriptive enough so that it does not
 * affect the readability of the code.
 *
 */
#define THIS_CARD(x)	table->names[*((unsigned *)x->fields + t * 2)]
#define LAST_CARD(x)	table->names[*((unsigned *)x->fields + (paths->fields_no - 1) * 2)]
void bot_play(int n)
{
	long i = 0, j, t, id, min, moves = 0, xcard;
	int ret_suit, ret_number;
	unsigned cond_row;
	dllst_item_struct_t *iter, *iter2;
	boolean_t cpuplayed = FALSE, match = FALSE;
	dllst_t *alternatives = NULL, *paths = NULL, *conds = NULL, *prob = NULL;
	char message[128] = { '\0' }, buf[64] = { '\0' };
	char *pathstr = NULL;
	digraph_table_t *table = NULL;
	struct {
		unsigned num;
		int unused0;
		float prb;
		int unused1;
	} prob_fields = { 0 };


	do_xmlNewNode(turn_node, "turn");
	sprintf(buf, "%d", n);
	do_xmlNewProp(turn_node, "id", buf);
	do_xmlAddChild(hand_node, turn_node);

	// Last card played was either an ace or a two. Bots cannot get only
	// one card from the deck or play an arbitrary card because that
	// rule is temporarily locked.
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
					fields.suit   = CARD_SUIT(iter);
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

			// There are no alternatives, but last card played
			// was either an ace or a two
			goto getmorecards;
	}

	/*
	 *
	 * This is the new algorithm of smart dispatching of cards.
	 * Bots build a list of all the possible ways of playing in order to
	 * get rid the greater amount of cards as they can.
	 * To do so, the first card must match either suit or number
	 * of the last card played and permit another card can be played after it,
	 * repeating the process until there is no more special card to play
	 * (i.e., special cards are 4, 7, jacks and queens under some circumstances).
	 *
	 */
	while (i < 2) {
		conds = dllst_initlst(conds, "I:");
		cond_row = 0;
		dllst_newitem(conds, &cond_row);
		table = digraph_create_table(player[n].list);

		for (j=table->dim;j>0;j--) {
			paths = digraph_get_paths(table, j, conds);
			if (!paths->size) {
				while (dllst_delitem(paths, 0)) {}
				free(paths);
				paths = NULL;

				// save an indentation level
				continue;
			}

			printf("\tPossible paths of length %ld:\n", j - 1);
			for (id=0,iter=paths->head;iter;iter=iter->next,id++) {

				pathstr = (char *)calloc(1, paths->fields_no * 7 + 1);
				if (!pathstr) {
					printf("Warning: could not allocate paths string\n");
					break;
				}

				for (t=1;t<paths->fields_no;t++) {
					strcat(pathstr, table->names[*((unsigned *)iter->fields + t * 2)]);
					strcat(pathstr, " ");
				}
				do_xmlNewChild(node, turn_node, "possibility", pathstr);
				sprintf(buf, "%ld", id);
				do_xmlNewProp(node, "id", buf);
				printf("\t\t%s\n", pathstr);
				free(pathstr);
				pathstr = NULL;
			}

			/*
			 *
			 * So far, probabilities were computed for twos and aces only.
			 * As of v0.3.1, they are calculated to choose which path is more
			 * convenient to play according to the least number/suit probabilities
			 * of the last card of each possible path. It is intended to
			 * difficult to play a card of the same number for the next player,
			 * which turns bots more evil (or smarter, depending on your point
			 * of view).
			 *
			 */
			bot_calc_probabilities(n);
			prob = dllst_initlst(prob, "I:f:");
			for (t=0;t<17;t++) {
				prob_fields.num = t;
				prob_fields.prb = player[n].probabilities[t];
				dllst_newitem(prob, &prob_fields);
			}
			dllst_sortby(prob, 1, TRUE);

#define DEBUG_PROBS	0
#if DEBUG_PROBS
			memset(buf, '\0', 64);
			for (iter=prob->head;iter;iter=iter->next) {
				switch (*((unsigned *)iter->fields + 0)) {
				case 0:
					buf[0] = ' ';
					buf[1] = 'A';
					break;
				case 1 ... 9:
					sprintf(buf, "%2u", *((unsigned *)iter->fields + 0) + 1);
					break;
				case 10:
					buf[0] = ' ';
					buf[1] = 'J';
					break;
				case 11:
					buf[0] = ' ';
					buf[1] = 'Q';
					break;
				case 12:
					buf[0] = ' ';
					buf[1] = 'K';
					break;
				case 13:
					buf[0] = ' ';
					buf[1] = 'C';
					break;
				case 14:
					buf[0] = ' ';
					buf[1] = 'D';
					break;
				case 15:
					buf[0] = ' ';
					buf[1] = 'H';
					break;
				case 16:
					buf[0] = ' ';
					buf[1] = 'S';
					break;
				};
				printf("%s: %f\n", buf, *((float *)iter->fields + 2));
				memset(buf, '\0', 64);
			}
#endif
			match = FALSE;
			for (iter=prob->head;iter;iter=iter->next) {
				for (iter2=paths->head;iter2;iter2=iter2->next) {
					decode_card_rev(LAST_CARD(iter2), &ret_suit, &ret_number);
					if (*((unsigned *)iter->fields) == ret_number ||
					    *((unsigned *)iter->fields) == ret_suit + 13) {
						match = TRUE;
						break;
					}
				}
				if (match)
					break;
			}

			while (dllst_delitem(prob, 0)) {}
			free(prob);
			prob = NULL;
			if (!match)
				iter2 = paths->head;

			for (t=1;t<paths->fields_no;t++) {
				decode_card_rev(THIS_CARD(iter2), &ret_suit, &ret_number);
#if DEBUG_PROBS
				if (t == paths->fields_no - 1)
					printf("%s has the lowest probability\n",
						decode_card(ret_suit, ret_number, FALSE));
#endif

				for (j=0,iter=player[n].list->head;iter;iter=iter->next,j++)
					if (CARD_SUIT(iter) == ret_suit && CARD_NUMBER(iter) == ret_number)
						break;

				if (ret_number == CARD_JACK(ret_suit) % 13) {
					lastsuit = get_suit_selector(THIS_CARD(iter2));
					sprintf(message, "%s selected", suitstr[lastsuit]);
					printf("\t%s\n", message);
					do_xmlNewChild(node, turn_node, "msg", message);
				} else {
					lastsuit = ret_suit;
				}

				playcard(n, ret_suit, ret_number, &j);

				if (ret_number == CARD_TWO(ret_suit) % 13) {
					getatmost += 2;
					sprintf(message, "New static limit is %d cards", getatmost);
					printf("\t%s\n", message);
					do_xmlNewChild(node, turn_node, "msg", message);
				} else if (ret_number == CARD_ACE(ret_suit) % 13) {
					getatmost += 4;
					sprintf(message, "New static limit is %d cards", getatmost);
					printf("\t%s\n", message);
					do_xmlNewChild(node, turn_node, "msg", message);
				}

				if (ret_number == CARD_QUEEN(ret_suit) % 13)
					update_turn();

				if (ret_number == CARD_KING(ret_suit) % 13)
					rotation *= (-1);

				cpuplayed = TRUE;
				moves++;
			}

			// The player will subtract one point for each four cards played
			player[n].specialpts -= moves / 4;
			while (dllst_delitem(paths, 0)) {}
			free(paths);
			paths = NULL;
			break;
		}
		digraph_destroy_table(table);
		while (dllst_delitem(conds, 0)) {}
		free(conds);
		conds = NULL;
		goto fnreturn;

checkalternatives:
		if (alternatives->size) {
			bot_calc_probabilities(n);
			min = CARD_SUIT(alternatives->head) * 13 + CARD_NUMBER(alternatives->head);
			for (iter=alternatives->head->next;iter;iter=iter->next)
				if (player[n].probabilities[CARD_SUIT(iter)] < player[n].probabilities[min / 13] ||
				    player[n].probabilities[CARD_NUMBER(iter)] < player[n].probabilities[min % 13])
					min = CARD_SUIT(iter) * 13 + CARD_NUMBER(iter);

			for (j=0,iter=player[n].list->head;iter;iter=iter->next,j++)
				if (CARD_SUIT(iter) == min / 13 && CARD_NUMBER(iter) == min % 13)
					break;

			lastsuit = min / 13;
			playcard(n, min / 13, min % 13, &j);
			moves++;

			if (min == CARD_TWO(min / 13)) {
				getatmost += 2;
				sprintf(message, "New static limit is %d cards", getatmost);
				printf("\t%s\n", message);
				do_xmlNewChild(node, turn_node, "msg", message);
			} else if (min == CARD_ACE(min / 13)) {
				getatmost += 4;
				sprintf(message, "New static limit is %d cards", getatmost);
				printf("\t%s\n", message);
				do_xmlNewChild(node, turn_node, "msg", message);
			}

			while (dllst_delitem(alternatives, 0)) {}
			free(alternatives);
			alternatives = NULL;
			cpuplayed = TRUE;
		}

fnreturn:
		if (cpuplayed && moves) {
			if (!player[n].list->size) {
				sprintf(message, "finished");
				printf("\t%s\n", message);
				do_xmlNewChild(node, turn_node, "msg", message);
				player[n].active = FALSE;
				if (getactiveplayers() == 1)
					finish_hand();
			}
			update_turn();
			while (dllst_delitem(alternatives, 0)) {}
			free(alternatives);
			return;
		} else if (!i) {
getmorecards:
			if (getatmost) {
				sprintf(message, "Getting %d cards from the deck", getatmost);
				printf("\t%s\n", message);
				for (j=0;j<getatmost;j++) {
					xcard = getcardfromdeck(n, ADD_CARD);
					if (xcard == -1)
						return;

					sprintf(message, "Getting 1 card from the deck");
					do_xmlNewChild(node, turn_node, "msg", message);

					if (!j &&
					    (xcard == CARD_TWO(xcard / 13) ||
					     xcard == CARD_ACE(xcard / 13))) {
						sprintf(message, "Not taking %d cards because "
						       "I got an ace/two in the first attempt",
							getatmost);
						printf("\t%s\n", message);
						do_xmlNewChild(node, turn_node, "msg", message);

						min = player[n].list->size - 1;
						lastsuit = xcard / 13;
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

				// The player will add one extra point for each six cards
				// obtained from the deck
				player[n].specialpts += getatmost / 6;
				getatmost = 0;
				moves++;
			} else {
				xcard = getcardfromdeck(n, ADD_CARD);
				if (xcard == -1)
					return;

				sprintf(message, "Getting 1 card from the deck");
				do_xmlNewChild(node, turn_node, "msg", message);
			}
		}

		i++;
	}

	sprintf(message, "I have no cards matching suit or number as last card played");
	printf("\t%s\n", message);
	do_xmlNewChild(node, turn_node, "msg", message);
	update_turn();
	while (dllst_delitem(alternatives, 0)) {}
	free(alternatives);
	if (getactiveplayers() == 1)
		finish_hand();
}
#undef LAST_CARD
#undef THIS_CARD

int getactiveplayers(void)
{
	int i, ret = 0;


	for (i=0;i<NPLAYERS;i++)
		if (player[i].active)
			ret++;

	return ret;
}

void finish_hand(void)
{
	unsigned j, t;
	char str[32] = { '\0' }, buf[16] = { '\0' };
	gui_table_t *table = NULL;


	if (hand_finished)
		return;

	for (j=0;j<NPLAYERS;j++) {
		if (player[j].active)
			player[j].scores += player[j].list->size;
		player[j].scores += player[j].specialpts;
	}

	for (j=0;j<52;j++)
		del_selector(&resource[j], 0, 0, 0);

	del_selector(&resource[RES_DECK],
		     (800 - 2 * (CARD_WIDTH - 7)) / 2 + CARD_WIDTH / 2,
		     CARD_WIDTH / 2,
		     (600 - CARD_HEIGHT) / 2);

	do_timer_unset(deck_timer);
	dialog = gui_newdialog(display, window, "Table of scores",
			       (800 - 350) / 2,
			       (600 - 240) / 2, 350, 240);
	table = gui_table_new(dialog, 5, 4);
	gui_table_cell_set(table, 0, 0, "Player");
	gui_table_cell_set(table, 0, 1, "This hand");
	gui_table_cell_set(table, 0, 2, "Special");
	gui_table_cell_set(table, 0, 3, "Total");
	for (j=0;j<NPLAYERS;j++) {
		gui_table_cell_set(table, j + 1, 0, player[j].name);
		sprintf(buf, "%ld", player[j].list->size);
		gui_table_cell_set(table, j + 1, 1, buf);
		sprintf(buf, "%d", player[j].specialpts);
		gui_table_cell_set(table, j + 1, 2, buf);
		sprintf(buf, "%d", player[j].scores);
		gui_table_cell_set(table, j + 1, 3, buf);
	}
	gui_table_show(table, 10, 10);

	for (j=0;j<NPLAYERS;j++) {
		do_xmlNewNode(node, "scores");
		sprintf(buf, "%d", j);
		do_xmlNewProp(node, "id", buf);
		sprintf(buf, "%d", player[j].scores);
		do_xmlNewProp(node, "total", buf);
		do_xmlAddChild(hand_node, node);
	}

	for (t=0;t<NPLAYERS;t++)
		if (player[t].scores >= game_total)
			break;

	if (t == NPLAYERS) {
		nhand++;
		newhand_button = gui_addbutton(dialog, "New hand", 350 - 10 - 10 * FNT_WIDTH, 200);
	} else {
		nhand = 0;
		ngame++;
		sprintf(str, "%s lost", player[t].name);
		gui_addlabel(dialog, str, 9);
		newgame_button = gui_addbutton(dialog, "New game", 350 - 10 - 10 * FNT_WIDTH, 200);
	}
	hand_finished = TRUE;
}

void do_exposure(XExposeEvent *ep)
{
	int i;


	XFillRectangle(display, window, gc_table, 0, 0, 800, 600);
	unlock_deck();

	fields.suit   = CARD_SUIT(played_list->tail);
	fields.number = CARD_NUMBER(played_list->tail);
	render_resource(&resource[fields.suit * 13 + fields.number], STACK_OF_PLAYED_X, STACK_OF_PLAYED_Y);

	update_cards(HUMAN, EXPOSURE_CARD, NULL);
	update_cards(BOT_1, EXPOSURE_CARD, NULL);
	update_cards(BOT_2, EXPOSURE_CARD, NULL);
	update_cards(BOT_3, EXPOSURE_CARD, NULL);

	XDrawImageString(display, window, gc_white, 200 - strlen(player[0].name) * 6, HUMAN_Y,
			 player[0].name, strlen(player[0].name));
	XDrawImageString(display, window, gc_white, BOT_1_X, BOT_1_Y, player[1].name, strlen(player[1].name));
	XDrawImageString(display, window, gc_white, BOT_2_X, BOT_2_Y, player[2].name, strlen(player[2].name));
	XDrawImageString(display, window, gc_white, BOT_3_X, BOT_3_Y, player[3].name, strlen(player[3].name));

	for (i=0;i<NPLAYERS;i++)
		if (turn == i)
			render_resource(&resource[RES_PLAYING_ENABLED], playing_x[i], playing_y[i]);
		else
			render_resource(&resource[RES_PLAYING_DISABLED], playing_x[i], playing_y[i]);

	update_table(EXPOSURE_SUIT);

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
	char message[128] = { '\0' };
	int dispatched = 0;


	if (hand_finished) {
		if ((newhand_button && (bp->x > newhand_button->x0 && bp->x < newhand_button->x1 &&
		                         bp->y > newhand_button->y0 && bp->y < newhand_button->y1)) ||
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
			                       bp->y > newgame_button->y0 && bp->y < newgame_button->y1)) {
				for (i=0;i<NPLAYERS;i++) {
					player[i].scores = 0;
					player[i].specialpts = 0;
				}
				do_xmlNewNode(game_node, "game");
				sprintf(message, "%u", ngame);
				do_xmlNewProp(game_node, "id", message);
				do_xmlAddChild(session_node, game_node);
			}

			hand_finished = FALSE;
			free(newhand_button);
			newhand_button = NULL;
			free(newgame_button);
			newgame_button = NULL;
			gui_destroydialog(dialog);
			init_hand();
			do_exposure(&event.xexpose);
			vlock = FALSE;
		}
	}

	if (turn != HUMAN)
		return;

	switch (bp->button) {
	// left button
	case Button1:
		if (bp->x > resource[RES_DECK].region.x0 &&
		    bp->x < resource[RES_DECK].region.x1 &&
		    bp->y > resource[RES_DECK].region.y0 &&
		    bp->y < resource[RES_DECK].region.y1) {
			if (getatmost)
				limit = getatmost;
			else
				limit = 1;

			if (getatmost) {
				sprintf(message, "Getting %lu cards from the deck", limit);
				printf("\t%s\n", message);
			}

			for (;moves<limit;moves++) {
				xcard = getcardfromdeck(HUMAN, ADD_CARD);
				if (xcard == -1)
					return;

				sprintf(message, "Getting 1 card from the deck");
				do_xmlNewChild(node, turn_node, "msg", message);

				if (!moves && limit > 1 &&
				    (xcard == CARD_TWO(xcard / 13) ||
				     xcard == CARD_ACE(xcard / 13))) {
					sprintf(message, "Not taking %lu cards because "
					       "I got an ace/two in the first attempt",
						limit);
					printf("\t%s\n", message);
					do_xmlNewChild(node, turn_node, "msg", message);
					update_cards(HUMAN, GET_CARD, NULL);
					moves = prevmoves = 1;
					return;
				}
			}

			// @limit can be at most 24 (i.e., all of the aces and twos
			// have been played)
			if (limit > 1)
				player[HUMAN].specialpts += limit / 6;

			getatmost = 0;
			lock_deck();
			break;
		}

		for (i=0;i<4;i++) {
			if (bp->x > resource[RES_SUIT_BASE + i].region.x0 &&
			    bp->x < resource[RES_SUIT_BASE + i].region.x1 &&
			    bp->y > resource[RES_SUIT_BASE + i].region.y0 &&
			    bp->y < resource[RES_SUIT_BASE + i].region.y1) {
				update_table(SELECT_NONE);
				lastsuit = i;
				update_table(EXPOSURE_SUIT);
				update_cards(HUMAN, EXPOSURE_CARD, NULL);
				sprintf(message, "%s selected", suitstr[lastsuit]);
				printf("\t%s\n", message);
				do_xmlNewChild(node, turn_node, "msg", message);
				return;
			}
		}

		for (i=0;i<52;i++) {
			if (bp->x > resource[i].region.x0 &&
			    bp->x < resource[i].region.x1 &&
			    bp->y > resource[i].region.y0 &&
			    bp->y < resource[i].region.y1) {
				for (iter=player[HUMAN].list->head,j=0;iter;iter=iter->next,j++) {
					if (i == CARD_SUIT(iter) * 13 + CARD_NUMBER(iter)) {
						moves = prevmoves = 0;

						if (i != CARD_FOUR(CARD_SUIT(iter)) &&
						    i != CARD_SEVEN(CARD_SUIT(iter)) &&
						    i != CARD_JACK(CARD_SUIT(iter)) &&
						    !(i == CARD_QUEEN(CARD_SUIT(iter)) &&
						     getactiveplayers() == 2)) {
							humanplayed = TRUE;
						} else {
							dispatched++;
							do_timer_set(playing_timer, 0, 125000000);
						}

						if (i == CARD_TWO(CARD_SUIT(iter)))
							getatmost += 2;
						else if (i == CARD_ACE(CARD_SUIT(iter)))
							getatmost += 4;
						else
							getatmost = 0;

						if (i == CARD_JACK(CARD_SUIT(iter)))
							update_table(SELECT_SUIT);

						if (i == CARD_KING(CARD_SUIT(iter)))
							rotation *= (-1);

						lastsuit = CARD_SUIT(iter);
						playcard(HUMAN, CARD_SUIT(iter), CARD_NUMBER(iter), &j);
						if (getatmost) {
							sprintf(message, "New static limit is %d cards", getatmost);
							printf("\t%s\n", message);
							do_xmlNewChild(node, turn_node, "msg", message);
						}
						unlock_deck();
						break;
					}
				}

				if (humanplayed) {
					update_table(SELECT_NONE);
					update_table(EXPOSURE_SUIT);
					do_timer_unset(playing_timer);
					do_timer_unset(table_timer);

					// @dispatched can be at most 17 (i.e., all of
					// the 4s, 7s, jacks, queens and a arbitrary
					// card have been played)
					player[HUMAN].specialpts -= dispatched / 4;

					update_turn();

					while (turn != HUMAN && getactiveplayers() > 1)
						bot_play(turn);

					if (getactiveplayers() == 1) {
						finish_hand();
						return;
					}

					do_xmlNewNode(turn_node, "turn");
					do_xmlNewProp(turn_node, "id", "0");
					do_xmlAddChild(hand_node, turn_node);

					if (!getatmost) {
						update_cards(HUMAN, EXPOSURE_CARD, NULL);
						if (!player[HUMAN].list->size) {
							sprintf(message, "finished");
							printf("\t%s\n", message);
							do_xmlNewChild(node, turn_node, "msg", message);
							player[HUMAN].active = FALSE;
							update_turn();
							while (getactiveplayers() > 1)
								bot_play(turn);
							finish_hand();
						}
					} else {
						update_cards(HUMAN, GET_CARD, NULL);
					}
				}

				break;
			}
		}
		break;
	// right button
	case Button3:
		if (bp->x > resource[RES_DECK_LOCKED].region.x0 &&
		    bp->x < resource[RES_DECK_LOCKED].region.x1 &&
		    bp->y > resource[RES_DECK_LOCKED].region.y0 &&
		    bp->y < resource[RES_DECK_LOCKED].region.y1) {
			if (getatmost)
				limit = getatmost;
			else
				limit = 1;

			if (!humanplayed && moves >= limit) {
				unlock_deck();
				update_table(SELECT_NONE);
				update_table(EXPOSURE_SUIT);
				humanplayed = TRUE;
				moves = 0;
				do_timer_unset(playing_timer);
				do_timer_unset(table_timer);

				sprintf(message, "I have no cards matching suit or number as last card played");
				printf("\t%s\n", message);
				do_xmlNewChild(node, turn_node, "msg", message);

				update_turn();
				while (turn != HUMAN && getactiveplayers() > 1)
					bot_play(turn);

				if (getactiveplayers() == 1) {
					finish_hand();
					return;
				}

				do_xmlNewNode(turn_node, "turn");
				do_xmlNewProp(turn_node, "id", "0");
				do_xmlAddChild(hand_node, turn_node);

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
	static int i = 0, suit = 0;
	static unsigned long playing_status = 0;


	if (tp.sival_int == 1) {

		// deck_timer
		XLockDisplay(display);
		XFillRectangle(display, window, gc_anim[i & 7], DECK_X - 7, DECK_Y + i * 12, 3, 11);
		if (++i > 7) {
			i = 0;
			do_timer_unset(deck_timer);
			XFillRectangle(display, window, gc_table, DECK_X - 7, DECK_Y, 3, 8 * 12);
		}
		XUnlockDisplay(display);
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
			if ((suit & 3) == j)
				render_resource(&resource[RES_SUIT_BASE_INV + j], SUIT_X, SUIT_Y(j));
			else
				render_resource(&resource[RES_SUIT_BASE + j], SUIT_X, SUIT_Y(j));
		suit++;
		XUnlockDisplay(display);
	}
}

/*
 *
 * Function registered as the last procedure to run on program exit.
 * Please note that it only occurs when the window is closed, in which
 * case the connection to the X server is first broken and therefore all of
 * the subsequent calls to Xlib functions are no-ops (e.g., XFreeGC(),
 * XDestroyWindow(), etc). This should be sanitized in future releases by
 * adding code to be run when the window manager sends messages to this
 * window.
 *
 */
void do_exit(void)
{
	int i;
	char *default_file = NULL;


	printf("Bye!\n");
#if defined(HAVE_XML_LOGS)
	if (savelog) {
		if (logfilename) {
			do_xmlSaveFormatFileEnc(logfilename, xml_logfile);
		} else {
			i = strlen(userdir) + strlen("/.nullify-session.xml");
			default_file = (char *)calloc(1, i + 1);
			if (default_file) {
				strcat(default_file, userdir);
				strcat(default_file, "/.nullify-session.xml");
				do_xmlSaveFormatFileEnc(default_file, xml_logfile);
			}
			free(userdir);
			free(default_file);
		}
	}
#endif
	do_xmlFreeDoc(xml_logfile);
	do_xmlCleanupParser();

	while (dllst_delitem(deck_list, 0)) {}
	free(deck_list);
	while (dllst_delitem(played_list, 0)) {}
	free(played_list);
	for (i=0;i<NPLAYERS;i++) {
		while (dllst_delitem(player[i].list, 0)) {}
		free(player[i].list);
	}
	for (i=0;i<NRESOURCES;i++) {
		free(resource[i].bytes);
		free(resource[i].color);
	}
	XFreePixmap(display, cardpixmap);
	XFreeGC(display, gc_white);
	XFreeGC(display, gc_black);
	XFreeGC(display, gc_table);
	XFreeGC(display, gc_selector);
	for (i=0;i<8;i++)
		XFreeGC(display, gc_anim[i]);
	XDestroyWindow(display, window);
	XFlush(display);
	XCloseDisplay(display);
}

