/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/jfont.h"
#include "jinete/jmanager.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

static JTheme ji_current_theme = NULL;
static JTheme ji_standard_theme = NULL;

static void draw_text(BITMAP *bmp, FONT *f, const char *text, int x, int y,
		      int fg_color, int bg_color, bool fill_bg);

int _ji_theme_init(void)
{
  return 0;
}

void _ji_theme_exit(void)
{
  if (ji_standard_theme) {
    jtheme_free(ji_standard_theme);
    ji_standard_theme = NULL;
  }
}

JTheme jtheme_new(void)
{
  JTheme theme;

  theme = jmalloc(sizeof(struct jtheme));
  if (!theme)
    return NULL;

  theme->name = "Theme";
  theme->default_font = font;	/* default Allegro font */
  theme->desktop_color = 0;
  theme->textbox_fg_color = 0;
  theme->textbox_bg_color = 0;
  theme->check_icon_size = 0;
  theme->radio_icon_size = 0;
  theme->scrollbar_size = 0;
  theme->destroy = NULL;
  theme->regen = NULL;
  theme->set_cursor = NULL;
  theme->init_widget = NULL;
  theme->get_window_mask = NULL;
  theme->map_decorative_widget = NULL;
  theme->nmethods = 0;
  theme->methods = NULL;

  return theme;
}

void jtheme_free(JTheme theme)
{
  if (theme->destroy)
    (*theme->destroy)();

  if (ji_current_theme == theme)
    ji_set_theme(NULL);

  if (theme->methods)
    jfree(theme->methods);

  jfree(theme);
}

void jtheme_set_method(JTheme theme, int widget_type, JDrawFunc draw_widget)
{
  if (widget_type >= theme->nmethods) {
    int c, old_nmethods = theme->nmethods;

    theme->nmethods = widget_type+1;
    theme->methods = jrealloc(theme->methods,
			      sizeof(JDrawFunc) * theme->nmethods);

    for (c=old_nmethods; c<theme->nmethods; c++)
      theme->methods[c] = NULL;
  }

  theme->methods[widget_type] = draw_widget;
}

JDrawFunc jtheme_get_method(JTheme theme, int widget_type)
{
  if (theme->methods && widget_type >= 0 && widget_type < theme->nmethods)
    return theme->methods[widget_type];
  else
    return NULL;
}

/**********************************************************************/

void ji_set_theme(JTheme theme)
{
  JWidget manager = ji_get_default_manager();

  ji_current_theme = theme;

  if (ji_current_theme) {
    ji_regen_theme();

    /* TODO any better idea? */
    if (manager && jwidget_get_theme(manager) == NULL)
      jwidget_set_theme(manager, theme);
  }
}

void ji_set_standard_theme(void)
{
  if (!ji_standard_theme) {
    ji_standard_theme = jtheme_new_standard();
    if (!ji_standard_theme)
      return;
  }

  ji_set_theme(ji_standard_theme);
}

JTheme ji_get_theme(void)
{
  return ji_current_theme;
}

void ji_regen_theme(void)
{
  if (ji_current_theme) {
    /* hide the cursor */
/*     if () { */
/*       show_mouse(NULL); */
/*       set_mouse_sprite(NULL); */
/*     } */
    int type = jmouse_get_cursor();
    jmouse_set_cursor(JI_CURSOR_NULL);

    if (ji_current_theme->regen)
      (*ji_current_theme->regen)();

    /* ok, reset the mouse cursor */
    jmouse_set_cursor(type);
  }
}

int ji_color_foreground(void)
{
  if (ji_current_theme && ji_current_theme->color_foreground)
    return (*ji_current_theme->color_foreground)();
  else
    return makecol(0, 0, 0);
}

int ji_color_disabled(void)
{
  if (ji_current_theme && ji_current_theme->color_disabled)
    return (*ji_current_theme->color_disabled)();
  else
    return makecol(128, 128, 128);
}

int ji_color_face(void)
{
  if (ji_current_theme && ji_current_theme->color_face)
    return (*ji_current_theme->color_face)();
  else
    return makecol(255, 255, 255);
}

int ji_color_facelight(void)
{
  register int c = ji_color_face();
  return makecol(MIN(getr(c)+64, 255),
		 MIN(getg(c)+64, 255),
		 MIN(getb(c)+64, 255));
}

int ji_color_faceshadow(void)
{
  register int c = ji_color_face();
  return makecol(MAX(getr(c)-64, 0),
		 MAX(getg(c)-64, 0),
		 MAX(getb(c)-64, 0));
}

int ji_color_hotface(void)
{
  if (ji_current_theme && ji_current_theme->color_hotface)
    return (*ji_current_theme->color_hotface)();
  else
    return makecol(255, 255, 255);
}

int ji_color_selected(void)
{
  if (ji_current_theme && ji_current_theme->color_selected)
    return (*ji_current_theme->color_selected)();
  else
    return makecol(0, 0, 255);
}

int ji_color_background(void)
{
  if (ji_current_theme && ji_current_theme->color_background)
    return (*ji_current_theme->color_background)();
  else
    return makecol(255, 255, 255);
}

void _ji_theme_draw_sprite_color(BITMAP *bmp, BITMAP *sprite,
				 int x, int y, int color)
{
  int u, v, mask = bitmap_mask_color(sprite);

  for (v=0; v<sprite->h; v++)
    for (u=0; u<sprite->w; u++)
      if (getpixel(sprite, u, v) != mask)
	putpixel(bmp, x+u, y+v, color);
}

void _ji_theme_rectedge(BITMAP *bmp,
			int x1, int y1, int x2, int y2, int c1, int c2)
{
  vline(bmp, x1, y1, y2, c1);
  vline(bmp, x2, y1, y2, c2);

  hline(bmp, x1+1, y1, x2-1, c1);
  hline(bmp, x1+1, y2, x2-1, c2);
}

void _ji_theme_rectfill_exclude(BITMAP *bmp,
				int x1, int y1, int x2, int y2,
				int ex1, int ey1, int ex2, int ey2, int color)
{
  if ((ex1 > x2) || (ex2 < x1) ||
      (ey1 > y2) || (ey2 < y1))
    rectfill(bmp, x1, y1, x2, y2, color);
  else {
    int y, my1, my2;

    my1 = MAX(y1, ey1);
    my2 = MIN(y2, ey2);

    /* top */
    for (y=y1; y<ey1; y++)
      hline(bmp, x1, y, x2, color);

    /* left */
    if (x1 < ex1)
      for (y=my1; y<=my2; y++)
        hline(bmp, x1, y, ex1-1, color);

    /* right */
    if (x2 > ex2)
      for (y=my1; y<=my2; y++)
        hline(bmp, ex2+1, y, x2, color);

    /* bottom */
    for (y=ey2+1; y<=y2; y++)
      hline(bmp, x1, y, x2, color);
  }
}

void _ji_theme_textbox_draw(BITMAP *bmp, JWidget widget,
			    int *w, int *h, int bg, int fg)
{
  JWidget view = jwidget_get_view(widget);
  char *text = widget->text;
  int x1, y1, x2, y2;
  int x, y, chr, len;
  char *beg, *end;
  int scroll_x, scroll_y;
  int viewport_w, viewport_h;
  int textheight = jwidget_get_text_height(widget);
  FONT *font = widget->text_font;
  char *beg_end, *old_end;
  int width;

  if (view) {
    JRect vp = jview_get_viewport_position(view);
    x1 = vp->x1;
    y1 = vp->y1;
    viewport_w = jrect_w(vp);
    viewport_h = jrect_h(vp);
    jview_get_scroll(view, &scroll_x, &scroll_y);
    jrect_free(vp);
  }
  else {
    x1 = widget->rc->x1 + widget->border_width.l;
    y1 = widget->rc->y1 + widget->border_width.t;
    viewport_w = jrect_w(widget->rc) - widget->border_width.l - widget->border_width.r;
    viewport_h = jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b;
    scroll_x = scroll_y = 0;
  }
  x2 = x1+viewport_w-1;
  y2 = y1+viewport_h-1;

  chr = 0;

  /* without word-wrap */
  if (!(widget->align & JI_WORDWRAP)) {
    width = jrect_w(widget->rc);
  }
  /* with word-wrap */
  else {
    if (w) {
      width = *w;
      *w = 0;
    }
    else {
      /* TODO modificable option? I don't think so, this is very internal stuff */
#if 0
      /* shows more information in x-scroll 0 */
      width = viewport_w;
#else
      /* make good use of the complete text-box */
      if (view) {
	int w, h;
	jview_get_max_size(view, &w, &h);
	width = MAX(viewport_w, w);
      }
      else {
	width = viewport_w;
      }
#endif
    }
  }

  /* draw line-by-line */
  y = y1 - scroll_y;
  for (beg=end=text; end; ) {
    x = x1 - scroll_x;

    /* without word-wrap */
    if (!(widget->align & JI_WORDWRAP)) {
      end = ustrchr(beg, '\n');
      if (end) {
	chr = *end;
	*end = 0;
      }
    }
    /* with word-wrap */
    else {
      old_end = NULL;
      for (beg_end=beg;;) {
	end = ustrpbrk(beg_end, " \n");
	if (end) {
	  chr = *end;
	  *end = 0;
	}

	/* to here we can print */
	if ((old_end) && (x+text_length(font, beg) > x1-scroll_x+width)) {
	  if (end)
	    *end = chr;

	  end = old_end;
	  chr = *end;
	  *end = 0;
	  break;
	}
	/* we can print one word more */
	else if (end) {
	  /* force break */
	  if (chr == '\n')
	    break;

	  *end = chr;
	  beg_end = end+1;
	}
	/* we are in the end of text */
	else
	  break;

	old_end = end;
      }
    }

    len = text_length(font, beg);

    /* render the text in the "bmp" */
    if (bmp) {
      int xout;

      if (widget->align & JI_CENTER)
	xout = x + width/2 - len/2;
      else if (widget->align & JI_RIGHT)
	xout = x + width - len;
      else			/* left */
	xout = x;

      draw_text(bmp, font, beg, xout, y, fg, bg, TRUE);

      _ji_theme_rectfill_exclude
	(bmp,
	 x1, y, x2, y+textheight-1,
	 xout, y, xout+len-1, y+textheight-1, bg);
    }

    /* width */
    if (w)
      *w = MAX(*w, len);

    y += textheight;

    if (end) {
      *end = chr;
      beg = end+1;
    }
  }

  /* height */
  if (h)
    *h = (y-y1+scroll_y);

  if (w) *w += widget->border_width.l + widget->border_width.r;
  if (h) *h += widget->border_width.t + widget->border_width.b;

  /* fill bottom area */
  if (bmp) {
    if (y <= y2)
      rectfill(bmp, x1, y, x2, y2, bg);
  }
}

static void draw_text(BITMAP *bmp, FONT *f, const char *text, int x, int y,
		      int fg_color, int bg_color, bool fill_bg)
{
  /* TODO optional anti-aliased textout */
  ji_font_set_aa_mode(f, bg_color);
  text_mode(fill_bg ? bg_color: -1);
  textout(bmp, f, text, x, y, fg_color);
  /* TODO */
}
