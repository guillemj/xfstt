/*
 * Show glyphs
 *
 * Copyright © 1997-1998 Herbert Duerr
 * Copyright © 2004,2008,2010,2012 Guillem Jover <guillem@hadrons.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Softaware
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#define STARTGLYF	6
#define DEFAULT_FONT	"times.ttf"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_image.h>

#include "ttf.h"

static Rasterizer *raster;
static uint32_t color[5];
static uint8_t *pixmap;

#define BMPSIZE		1048 * 2048
uint8_t *bitmap = 0;

#define DEFAULT_WIDTH	200
#define DEFAULT_HEIGHT	220

#ifndef MAGNIFY
int MAGNIFY = 1;
#endif /*MAGNIFY*/

struct font_state {
	int size;
	int angle;
	uint16_t width;
	uint16_t height;
	int glyphNo;
};

static void
glyph2image(struct font_state *state, xcb_image_t *img)
{
	int length;
	static int old_size = 0, old_angle = 0;

	if (!bitmap)
		bitmap = (uint8_t *)allocMem(BMPSIZE);
	if (!bitmap)
		return;

	if (state->size != old_size || state->angle != old_angle) {
		old_size = state->size;
		old_angle = state->angle;
		int xcos = (int)(state->size * cos(state->angle * M_PI / 180));
		int xsin = (int)(state->size * sin(state->angle * M_PI / 180));
		raster->setPixelSize(xcos, -xsin, xsin, xcos);
		printf("fontsize = %d, angle = %d\n", state->size, state->angle);
	}

	GlyphMetrics gm;
	length = raster->putGlyphBitmap(state->glyphNo, bitmap, bitmap + BMPSIZE, &gm);
	img->size = length;

	img->width = 0;
	if (length == 0)
		return;

#if 0
	if (raster->anti_aliasing <= 0)
		switch (SCANLINEPAD) {
		case 8:
			break;
		case 16: {
			uint16_t *p = (uint16_t *)bitmap;
			for (int i = length; --i >= 0; ++p)
				*p = bswaps(*p);
			}
			break;
		case 32: {
			uint32_t *p = (uint32_t *)bitmap;
			for (int i = length; --i >= 0; ++p)
				*p = bswapl(*p);
			}
			break;
		case 64: {
			uint32_t *p = (uint32_t *)bitmap;
			for (int i = length; --i >= 0; p += 2) {
					uint32_t tmp = p[0];
					p[0] = bswapl(p[1]);
					p[1] = bswapl(tmp);
				}
			}
			break;
		}
#endif

	if (raster->anti_aliasing > 0)
		for (int i = length; --i >= 0;)
			pixmap[i] = color[bitmap[i]];
#if 0
#ifndef MAGNIFY
	else if (MAGNIFY) {
		int8_t *p1 = (int8_t *)bitmap;
		uint8_t *p2 = pixmap;
		raster->dX *= 8;	// 1 bpp -> 8 bpp
		for (int h = 0; h < raster->height; ++h) {
			int cy = h / MAGNIFY;
			int8_t m;
			for (int w = 0; w < raster->dX; ++w) {
				if ((w & 7) == 0)
					m = *(p1++);
				int c = ((w / MAGNIFY) ^ cy) & 1;
				*(p2++) = color[(m < 0) ? 3 + c : c];
				m <<= 1;
			}
		}
	}
#endif
#endif

	int isPixmap = (MAGNIFY || raster->anti_aliasing > 0);

	img->width = raster->width;
	img->height = raster->height;
	if (isPixmap)
		img->format = XCB_IMAGE_FORMAT_Z_PIXMAP;
	else
		img->format = XCB_IMAGE_FORMAT_XY_BITMAP;
	img->data = (uint8_t *)(isPixmap ? pixmap : bitmap);
	img->byte_order = XCB_IMAGE_ORDER_MSB_FIRST;
	img->unit = 8;
	img->bit_order = XCB_IMAGE_ORDER_MSB_FIRST;
	img->scanline_pad = 0;
	img->depth = isPixmap ? 8 : 1;
	img->stride = raster->dX;
	img->bpp = isPixmap ? 8 : 1;
}

static uint32_t
new_color(xcb_connection_t *c, xcb_colormap_t cmap,
          uint16_t r, uint16_t g, uint16_t b)
{
	xcb_alloc_color_cookie_t cookie;
	xcb_alloc_color_reply_t *reply;
	uint32_t color;

	cookie = xcb_alloc_color(c, cmap, r, g, b);
	reply = xcb_alloc_color_reply(c, cookie, nullptr);
	if (!reply)
		return 0;

	color = reply->pixel;
	free(reply);

	return color;
}

xcb_screen_t *
screen_of_display(xcb_connection_t *c, int screen)
{
	xcb_screen_iterator_t iter;

	iter = xcb_setup_roots_iterator(xcb_get_setup(c));
	for (; iter.rem; --screen, xcb_screen_next(&iter))
		if (screen == 0)
			return iter.data;

	return nullptr;
}

int
main(int argc, char** argv)
{
	xcb_connection_t *c;
	int screen_nr;
	xcb_screen_t *screen;
	xcb_visualid_t root_visual = { 0 };
	xcb_window_t root_window = { 0 };
	xcb_gcontext_t gc = { 0 };
	xcb_colormap_t cmap = { 0 };
	xcb_key_symbols_t *keysyms;
	uint32_t black, white, yellow;

	if (argc != 2) {
		fprintf(stderr, "Usage: showttf fontfile.ttf\n");
		fprintf(stderr, "use the cursor keys to navigate\n");
		return -1;
	}

	const char *ttFileName = (argc == 2) ? argv[1] : DEFAULT_FONT;

	pixmap = new uint8_t[1024 * 1024];

	int done = 0;

	c = xcb_connect(nullptr, &screen_nr);

	screen = screen_of_display(c, screen_nr);
	if (screen == nullptr)
		return 1;

	root_visual = screen->root_visual;
	root_window = screen->root;
	cmap = screen->default_colormap;

	gc = xcb_generate_id(c);

	uint32_t mask = 0;
	uint32_t values[5];

	mask |= XCB_GC_FUNCTION;
	values[0] = XCB_GX_COPY;

	mask |= XCB_GC_FOREGROUND;
	values[1] = screen->black_pixel;

	mask |= XCB_GC_BACKGROUND;
	values[2] = screen->white_pixel;

	mask |= XCB_GC_LINE_WIDTH;
	values[3] = 1;

	mask |= XCB_GC_LINE_STYLE;
	values[4] = XCB_LINE_STYLE_SOLID;

	xcb_create_gc(c, gc, root_window, mask, values);

	keysyms = xcb_key_symbols_alloc(c);

	color[0] = new_color(c, cmap, 0xF000, 0xF000, 0xF000);
	color[1] = new_color(c, cmap, 0xB400, 0xB400, 0xB400);
	color[2] = new_color(c, cmap, 0x7800, 0x7800, 0x7800);
	// XXX: color[3] = new_color(c, cmap, 0x3C00, 0x3C00, 0x3C00);
	color[3] = new_color(c, cmap, 0x5000, 0x5000, 0x5000);
	color[4] = new_color(c, cmap, 0x0000, 0x0000, 0x0000);

	white = new_color(c, cmap, 0xF000, 0xF000, 0xF000);
	black = new_color(c, cmap, 0x0000, 0x0000, 0x0000);
	yellow = new_color(c, cmap, 0xFF00, 0xFF00, 0x0000);

	struct font_state state;

	state.width = DEFAULT_WIDTH;
	state.height = DEFAULT_HEIGHT;

#if 0
	int fid = XLoadFont(display, "TTM20_Bitstream Cyberbit");
	XSetFont(display, textGC, fid);
	XChar2b *tststr = (XChar2b *)"\0a\0b\0c\0\x61";
	//char *tststr = "abcd";
#endif

	uint32_t cw_mask = XCB_CW_EVENT_MASK;
	uint32_t cw_values[1] = {
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	xcb_window_t window = xcb_generate_id(c);
	xcb_create_window(c, 0, window, root_window, 0, 0,
	                  state.width, state.height, 1,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, root_visual,
	                  cw_mask, cw_values);

	const char title[] = "TrueType Viewer";
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
	                    XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
	                    strlen(title), title);

	xcb_map_window(c, window);
	xcb_flush(c);

	raster = new Rasterizer();
	TTFont *ttFont = new TTFont(ttFileName);
	raster->useTTFont(ttFont);

	state.size = 16;
	state.angle = 0;
	state.glyphNo = STARTGLYF;

	xcb_image_t img;
	xcb_generic_event_t *event;

	while ((event = xcb_wait_for_event(c)) && !done) {
		switch (event->response_type & ~0x80) {
		drawglyph:
#ifndef MAGNIFY
			if (MAGNIFY && fontsize) {
				MAGNIFY = state.height / (state.fontsize + 1);
				if (!MAGNIFY) MAGNIFY = 1;
			}
#endif
		{
			//ttFont->getGlyphNo16(state.glyphNo);
			fprintf(stderr, "gno2 %d\n", state.glyphNo);
			fflush(stderr);
			glyph2image(&state, &img);
		}
			/* fall through */
		case XCB_EXPOSE:
		expose:
		{
			uint32_t gc_values[] = { 0 };

			gc_values[0] = yellow;
			xcb_change_gc(c, gc, XCB_GC_FOREGROUND, gc_values);

			xcb_rectangle_t rect = {
				0, 0, state.width, state.height
			};
			xcb_poly_fill_rectangle(c, window, gc, 1, &rect);

			gc_values[0] = black;
			xcb_change_gc(c, gc, XCB_GC_FOREGROUND, gc_values);

			if (img.width)
				xcb_image_put(c, window, gc, &img, 4, 4, 0);

#if 0
			tststr[3].byte2 = glyphNo;
			tststr[3].byte1 = glyphNo >> 8;
			fprintf(stderr, "glyphNo = %d\n", glyphNo);
			XDrawImageString16(display, win, textGC, 4, 120,
					   tststr, 4);
#endif

			xcb_flush(c);
			break;
		}
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t *ev_key;
			xcb_keysym_t ksym;

			ev_key = (xcb_key_press_event_t *)event;

			ksym = xcb_key_press_lookup_keysym(keysyms, ev_key, 0);

			// FIXME: need the keysym name (XLookupString);
//			char keyname[8];

			switch (ksym) {
			case XK_Escape:
				done = 1;
				break;
			case XK_F2:
				raster->grid_fitting = !raster->grid_fitting;
				goto drawglyph;
			case XK_F3:
				raster->anti_aliasing = !raster->anti_aliasing;
				goto drawglyph;
			case XK_F4:
#ifndef MAGNIFY
				MAGNIFY = !MAGNIFY;
#endif /*MAGNIFY*/
				goto drawglyph;
			case XK_Left:
				--state.glyphNo;
				goto drawglyph;
			case XK_Right:
				++state.glyphNo;
				goto drawglyph;
			case XK_Up:
				state.size += 1;
				goto drawglyph;
			case XK_Down:
				state.size -= 1;
				if (state.size < 6)
					state.size = 4;
				goto drawglyph;
			case XK_Page_Up:
				state.size += (state.size >> 2) ? state.size >> 2 : 1;
				goto drawglyph;
			case XK_Page_Down:
				state.size -= (state.size >> 2) ? state.size >> 2 : 1;
				goto drawglyph;
			case XK_Begin:
				state.angle = 0;
				state.size = 8;
#ifndef MAGNIFY
				MAGNIFY = 0;
#endif /*MAGNIFY*/
				goto drawglyph;
			case XK_End:
				state.angle = 0;
				state.size = state.height - 10;
#ifndef MAGNIFY
				MAGNIFY = 0;
#endif /*MAGNIFY*/
				goto drawglyph;
			case XK_Shift_L:
			case XK_Shift_R:
			case XK_Meta_L:
			case XK_Meta_R:
			case XK_Control_L:
			case XK_Control_R:
				goto expose;
			default:
				// XXX: glyphNo = ttFont->getGlyphNo16(0x20AC);

				// FIXME: need keysym name.
				state.glyphNo = ttFont->getGlyphNo16(ksym);
				// printf("key = \"%s\" -> %d\n", keyname, glyphNo);
				printf("key = %c -> %d\n", ksym, state.glyphNo);
				glyph2image(&state, &img);
			}
			goto expose;
		}

		case XCB_CONFIGURE_NOTIFY:
		{
			xcb_configure_notify_event_t *ev_conf;

			ev_conf = (xcb_configure_notify_event_t *)event;

			state.width = ev_conf->width;
			state.height = ev_conf->height;
			goto drawglyph;
			//break;
		}
		case XCB_DESTROY_NOTIFY:
			done = 1;
			break;

		default:
			//printf("XEvent.type = %d\n", event.type);
			break;
		}
	}

	if (bitmap)
		deallocMem(bitmap, BMPSIZE);

	delete raster;
	delete ttFont;
	delete [] pixmap;

	cleanupMem();

	xcb_key_symbols_free(keysyms);
	xcb_unmap_window(c, window);
	xcb_disconnect(c);

	return 0;
}
