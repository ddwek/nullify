/*
 * gui.c: routines to create and display graphical user interfaces controls
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gui.h"

/*
 *
 * Create a new dialog on the X display @display and X window @window, with the
 * specified title, x, y, width and height and then display it.
 * Return the recently created dialog or NULL if it could not make room to
 * hold the corresponding structure.
 *
 */
gui_dialog_t *gui_newdialog(Display *display, Window window, char *title, int x, int y, int width, int height)
{
	gui_dialog_t *dialog = NULL;
	GC gc_gui_background, gc_gui_titlebar_bg, gc_gui_titlebar_fg;


	gc_gui_background  = XCreateGC(display, window, 0, NULL);
	gc_gui_titlebar_bg = XCreateGC(display, window, 0, NULL);
	gc_gui_titlebar_fg = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc_gui_background,  0x003030);
	XSetForeground(display, gc_gui_titlebar_bg, 0x002020);
	XSetForeground(display, gc_gui_titlebar_fg, 0xffffff);
	XSetBackground(display, gc_gui_titlebar_fg, 0x002020);

	dialog = (gui_dialog_t *)calloc(1, sizeof(gui_dialog_t));
	if (dialog) {
		dialog->dy = display;
		dialog->win = window;
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

	XFreeGC(display, gc_gui_background);
	XFreeGC(display, gc_gui_titlebar_bg);
	XFreeGC(display, gc_gui_titlebar_fg);

	return dialog;
}

/*
 *
 * Free allocated bytes from a previously initialized dialog @dlg. Also, the original
 * contents of the lower window are restored and shown.
 *
 */
void gui_destroydialog(gui_dialog_t *dlg)
{
	GC gc;


	if (!dlg)
		return;

	gc = XCreateGC(dlg->dy, dlg->win, 0, NULL);
	XSetForeground(dlg->dy, gc, 0x000000);
	XCopyArea(dlg->dy, dlg->rect, dlg->win, gc,
		  0, 0, dlg->x1 - dlg->x0, dlg->y1 - dlg->y0, dlg->x0, dlg->y0);
	XFreePixmap(dlg->dy, dlg->rect);
	XFreeGC(dlg->dy, gc);
	free(dlg);
	dlg = NULL;
}

/*
 *
 * Add text @label to the dialog @dlg at the specified row.
 * By default, each label starts to be shown at the left margin of the dialog.
 *
 */
void gui_addlabel(gui_dialog_t *dlg, char *label, unsigned row)
{
	GC gc_gui_label;
	XRectangle rects;


	if (!dlg)
		return;

	gc_gui_label = XCreateGC(dlg->dy, dlg->win, 0, NULL);
	XSetForeground(dlg->dy, gc_gui_label, 0xaaaaaa);
	XSetBackground(dlg->dy, gc_gui_label, 0x003030);

	rects.x = dlg->x0;
	rects.y = dlg->y0;
	rects.width = dlg->x1 - dlg->x0;
	rects.height = dlg->y1 - dlg->y0;
	XSetClipRectangles(dlg->dy, gc_gui_label, 0, 0, &rects, 1, YXBanded);

	XDrawImageString(dlg->dy, dlg->win, gc_gui_label,
			 dlg->x0 + 10, dlg->y0 + 30 + row * (FNT_HEIGHT + 1),
			 label, strlen(label));

	XFreeGC(dlg->dy, gc_gui_label);
}

/*
 *
 * Add a button to the dialog @dlg, with the text @caption, at specified
 * x and y offsets from the left and top margins of the dialog, respectively.
 * Return the button recently created or NULL if it could not be allocated.
 *
 */
gui_button_t *gui_addbutton(gui_dialog_t *dlg, char *caption, int dx, int dy)
{
	GC gc_gui_button_bg;
	GC gc_gui_foreground;
	gui_button_t *button = NULL;


	if (!dlg)
		return NULL;

	gc_gui_button_bg  = XCreateGC(dlg->dy, dlg->win, 0, NULL);
	gc_gui_foreground = XCreateGC(dlg->dy, dlg->win, 0, NULL);
	XSetForeground(dlg->dy, gc_gui_button_bg,  0x005050);
	XSetForeground(dlg->dy, gc_gui_foreground, 0xcccccc);
	XSetBackground(dlg->dy, gc_gui_foreground, 0x005050);

	button = (gui_button_t *)calloc(1, sizeof(gui_button_t));
	if (button) {
		button->x0 = dlg->x0 + 2 + dx;
		button->y0 = dlg->y0 + 16 + dy;
		button->x1 = button->x0 + (strlen(caption) + 2) * FNT_WIDTH;
		button->y1 = button->y0 + 20;
		XFillRectangle(dlg->dy, dlg->win, gc_gui_button_bg,
			       button->x0, button->y0, (strlen(caption) + 2) * FNT_WIDTH, 20);
		XDrawImageString(dlg->dy, dlg->win, gc_gui_foreground,
				 dlg->x0 + 2 + dx + FNT_WIDTH,
				 dlg->y0 + 16 + dy + FNT_HEIGHT, caption, strlen(caption));
	}

	XFreeGC(dlg->dy, gc_gui_button_bg);
	XFreeGC(dlg->dy, gc_gui_foreground);

	return button;
}

/*
 *
 * Create a table of @nrows rows per @ncols columns on the dialog @dlg.
 * This function only allocates and initializes the appropriate structure members:
 * to set each cell of the table and display it use gui_table_cell_set() and
 * gui_table_show(), respectively.
 * Return the table recently created or NULL if it could not be allocated.
 *
 */
gui_table_t *gui_table_new(gui_dialog_t *dlg, unsigned nrows, unsigned ncols)
{
	int i;
	gui_table_t *tab = NULL;


	if (!dlg)
		return NULL;

	tab = (gui_table_t *)calloc(1, sizeof(gui_table_t));
	if (tab) {
		tab->dy = dlg->dy;
		tab->win = dlg->win;
		tab->dlg = dlg;
		tab->nrows = nrows;
		tab->row = (gui_table_row_t *)calloc(nrows, sizeof(gui_table_row_t));
		if (tab->row) {
			for (i=0;i<nrows;i++) {
				tab->row[i].ncols = ncols;
				tab->row[i].col = (gui_table_col_t *)calloc(ncols, sizeof(gui_table_col_t));
			}
		} else {
			free(tab->row);
			free(tab);
		}
	} else {
		free(tab);
	}

	return tab;
}

/*
 *
 * Set the cell of the table @tab located at @row:@col to the text @text.
 * Additionally, it adjusts the maximum width of such a column to a large
 * enough number in order to hold the widest string and be correctly rendered
 * by gui_table_show().
 * Return TRUE if @text could be copied to the cell, FALSE otherwise.
 *
 */
boolean_t gui_table_cell_set(gui_table_t *tab, unsigned row, unsigned col, char *text)
{
	boolean_t ret;
	unsigned i, maxwidth = 0;


	if (tab && row < tab->nrows && col < tab->row[row].ncols) {
		free(tab->row[row].col[col].text);
		tab->row[row].col[col].text = NULL;

		tab->row[row].col[col].text = (char *)calloc(1, strlen(text) + 1);
		if (tab->row[row].col[col].text) {
			strcpy(tab->row[row].col[col].text, text);

			for (i=0;i<tab->nrows;i++)
				if (tab->row[i].col[col].width > maxwidth)
					maxwidth = tab->row[i].col[col].width;

			if ((strlen(text) + 2) * FNT_WIDTH > maxwidth) {
				for (i=0;i<tab->nrows;i++)
					tab->row[i].col[col].width = (strlen(text) + 2) * FNT_WIDTH;
			}
			tab->row[row].height = FNT_HEIGHT + 8;

			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else {
		ret = FALSE;
	}

	return ret;
}

/*
 *
 * Render the table @tab and its contents at specified x and y offsets from
 * the left and top margins of the dialog this table was added in. Also, it
 * applies different styles to even and odd rows.
 *
 */
void gui_table_show(gui_table_t *tab, unsigned dx, unsigned dy)
{
	int i, j, t, x, y, w, h;
	unsigned goffset = 0;
	char *label = NULL;
	GC gc, gc_gui_table_row_even, gc_gui_table_row_odd, gc_gui_table_cell;


	if (!tab)
		return;

	gc_gui_table_row_even = XCreateGC(tab->dy, tab->win, 0, NULL);
	gc_gui_table_row_odd = XCreateGC(tab->dy, tab->win, 0, NULL);
	gc_gui_table_cell = XCreateGC(tab->dy, tab->win, 0, NULL);
	XSetForeground(tab->dy, gc_gui_table_row_even, 0x004040);
	XSetForeground(tab->dy, gc_gui_table_row_odd,  0x005050);
	XSetForeground(tab->dy, gc_gui_table_cell, 0xcccccc);

	for (i=0;i<tab->nrows;i++) {
		for (j=0;j<tab->row[i].ncols;j++) {
			goffset = 0;

			for (t=0;t<j;t++)
				goffset += tab->row[i].col[t].width;

			x = tab->dlg->x0 + dx + goffset;
			y = tab->dlg->y0 + 16 + dy + i * tab->row[i].height;
			w = tab->row[i].col[j].width;
			h = tab->row[i].height;
			label = tab->row[i].col[j].text;
			gc = i & 1 ? gc_gui_table_row_odd : gc_gui_table_row_even;
			XSetBackground(tab->dy, gc_gui_table_cell, i & 1 ? 0x005050 : 0x004040);
			XFillRectangle(tab->dy, tab->win, gc, x, y, w, h);
			XDrawImageString(tab->dy, tab->win, gc_gui_table_cell,
					 x + FNT_WIDTH, y + FNT_HEIGHT, label, strlen(label));
		}
	}

	XFreeGC(tab->dy, gc_gui_table_row_even);
	XFreeGC(tab->dy, gc_gui_table_row_odd);
	XFreeGC(tab->dy, gc_gui_table_cell);
}

