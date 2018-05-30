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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gui.h"


gui_dialog_t *gui_newdialog(Display *display, Window window, char *title, int x, int y, int width, int height)
{
	gui_dialog_t *dialog = NULL;
	GC gc_gui_background, gc_gui_titlebar_bg, gc_gui_titlebar_fg;


	gc_gui_background  = XCreateGC(display, window, 0, NULL);
	gc_gui_titlebar_bg = XCreateGC(display, window, 0, NULL);
	gc_gui_titlebar_fg = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc_gui_background,  0xa0a0a0);
	XSetForeground(display, gc_gui_titlebar_bg, 0x804060);
	XSetForeground(display, gc_gui_titlebar_fg, 0xffffff);
	XSetBackground(display, gc_gui_titlebar_fg, 0x804060);

	dialog = (gui_dialog_t *)calloc(1, sizeof(gui_dialog_t));
	if (dialog) {
		dialog->x0 = x;
		dialog->y0 = y;
		dialog->x1 = x + width;
		dialog->y1 = y + height;
		dialog->rect = XCreatePixmap(display, window, width, height, XDefaultScreenOfDisplay(display)->root_depth);
		XCopyArea(display, window, dialog->rect, gc_gui_background, x, y, width, height, 0, 0);
		XFillRectangle(display, window, gc_gui_background, x, y, width, height);
		XFillRectangle(display, window, gc_gui_titlebar_bg, x + 2, y + 2, width - 4, 16);
		XDrawImageString(display, window, gc_gui_titlebar_fg, x + 10, y + 14, title, strlen(title));
	}

	return dialog;
}

void gui_destroydialog(Display *display, Window window, gui_dialog_t *dlg)
{
	GC gc;


	gc = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc, 0x000000);
	XCopyArea(display, dlg->rect, window, gc,
		  0, 0, dlg->x1 - dlg->x0, dlg->y1 - dlg->y0, dlg->x0, dlg->y0);
	XFreePixmap(display, dlg->rect);
	free(dlg);
	dlg = NULL;
}

void gui_addlabel(Display *display, Window window, gui_dialog_t *dlg, char *label, int dx, int dy)
{
	GC gc_gui_label;


	gc_gui_label = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc_gui_label, 0x202020);
	XSetBackground(display, gc_gui_label, 0xa0a0a0);

	if (dlg)
		XDrawImageString(display, window, gc_gui_label,
				 dlg->x0 + dx + 2, dlg->y0 + dy + 16, label, strlen(label));
}

gui_button_t *gui_addbutton(Display *display, Window window, gui_dialog_t *dlg, char *caption, int dx, int dy)
{
	GC gc_gui_button_bg;
	GC gc_gui_foreground;
	gui_button_t *button = NULL;


	gc_gui_button_bg  = XCreateGC(display, window, 0, NULL);
	gc_gui_foreground = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc_gui_button_bg,  0x202020);
	XSetForeground(display, gc_gui_foreground, 0xffffff);
	XSetBackground(display, gc_gui_foreground, 0x202020);

	if (dlg) {
		button = (gui_button_t *)calloc(1, sizeof(gui_button_t));
		if (button) {
			button->x0 = dlg->x0 + 2 + dx;
			button->y0 = dlg->y0 + 16 + dy;
			button->x1 = button->x0 + (strlen(caption) + 1) * 8;
			button->y1 = button->y0 + 20;
			XFillRectangle(display, window, gc_gui_button_bg,
				       button->x0, button->y0, (strlen(caption) + 1) * 8, 20);
			XDrawImageString(display, window, gc_gui_foreground,
					 dlg->x0 + 2 + dx + 8, dlg->y0 + 16 + dy + 14, caption, strlen(caption));
		}
	}

	return button;
}

