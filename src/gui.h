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
#ifndef _GUI_H_
#define _GUI_H_			1
#ifndef _HAVE_BOOLEAN_T_
#define _HAVE_BOOLEAN_T_
typedef enum { FALSE=0, TRUE } boolean_t;
#endif
#include <X11/Xlib.h>

#define FNT_WIDTH               6
#define FNT_HEIGHT              13

typedef struct gui_dialog_st {
	Display *dy;
	Window win;
	int x0;
	int y0;
	int x1;
	int y1;
	Pixmap rect;
} gui_dialog_t;

typedef struct gui_button_st {
	int x0;
	int y0;
	int x1;
	int y1;
} gui_button_t;
                                                                                            
typedef struct gui_table_col_st {
        unsigned width;
        char *text;
} gui_table_col_t;

typedef struct gui_table_row_st {
        unsigned height;
        unsigned ncols;
        gui_table_col_t *col;
} gui_table_row_t;

typedef struct gui_table_st {
        Display *dy;
        Window win;
        gui_dialog_t *dlg;
        unsigned nrows;
        gui_table_row_t *row;
} gui_table_t;

extern gui_dialog_t *gui_newdialog(Display *dy, Window win, char *title, int x, int y, int w, int h);
extern void gui_destroydialog(gui_dialog_t *dlg);
extern void gui_addlabel(gui_dialog_t *dlg, char *label, unsigned row);
extern gui_button_t *gui_addbutton(gui_dialog_t *dlg, char *caption, int dx, int dy);
extern gui_table_t *gui_table_new(gui_dialog_t *dlg, unsigned nrows, unsigned ncols);
extern boolean_t gui_table_cell_set(gui_table_t *tab, unsigned row, unsigned col, char *text);
extern void gui_table_show(gui_table_t *tab, unsigned dx, unsigned dy);
#endif

