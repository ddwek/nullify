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
#ifndef _GUI_H_
#define _GUI_H_			1
#include <X11/Xlib.h>

typedef struct gui_dialog_st {
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

extern gui_dialog_t *gui_newdialog(Display *display, Window window, char *title, int x, int y, int width, int height);
extern void gui_destroydialog(Display *display, Window window, gui_dialog_t *dlg);
extern void gui_addlabel(Display *display, Window window, gui_dialog_t *dlg, char *label, int dx, int dy);
extern gui_button_t *gui_addbutton(Display *display, Window window, gui_dialog_t *dlg, char *caption, int dx, int dy);
#endif

