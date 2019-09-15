/*
 * Scan line converter
 *
 * Copyright © 1997-1998 Herbert Duerr
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

#include "ttf.h"
#include <climits>
#include <cstring>

struct Dot {
	int y, x;
};

static Dot *dots[2], *dots0, *dots1;
#define DOTBUFSIZE 32768

void
Rasterizer::openDraw()
{
	dots[0] = (Dot *)allocMem(2 * DOTBUFSIZE * sizeof(Dot));
	dots[1] = &dots[0][DOTBUFSIZE];
	dots[0]->y = dots[1]->y = INT_MIN;	// sort markers
}

void
Rasterizer::closeDraw()
{
	deallocMem(dots[0], 2 * DOTBUFSIZE * sizeof(Dot));
}

static void
dropoutInit()
{
	dots0 = dots[0];
	dots1 = dots[1];
}

static inline Dot *
drawDots(Dot *p, int x1, int y1, int x2, int y2)
{
	if (y2 < y1) {
		int t = y1; y1 = y2; y2 = t;
		t = x1; x1 = x2; x2 = t;
	}

	int ygoal = ((y2 - 32) | 63) - 31;

	// better use interval=64 Bresenham algorithm
	for (x2 -= x1, y2 -= y1; ygoal > y1; ygoal -= 64) {
		++p;
		p->y = ygoal;
		p->x = x1 + MULDIV(ygoal - y1, x2, y2);
	}

	return p;
}

inline void
Rasterizer::drawSegment(int x1, int y1, int x2, int y2)
{
	dots0 = drawDots(dots0, x1, y1, x2, y2);
	dots1 = drawDots(dots1, y1, x1, y2, x2);
}

static void
preSort(Dot *l, Dot *r)
{
	// quicksort algorithm
	// (it's pretty hard to find a good partitioning element here)
	int t;
	Dot *i = l - 1;
manual_tail_recursion:
	Dot *j = r;

	for (;;) {
		t = r->y;

		do
			++i;
		while (i->y < t);
		do
			--j;
		while (j->y > t);

		if (j < i)
			break;

		t = j->y;
		j->y = i->y;
		i->y = t;

		t = j->x;
		j->x = i->x;
		i->x = t;
	}
	/*t = r->y;*/
	r->y = i->y;
	i->y = t;
	
	t = r->x;
	r->x = i->x;
	i->x = t;
	
	if (l < j - 10)
		preSort(l, j);
	l = i + 1;
	if (l < r-8)
		goto manual_tail_recursion;
}

static void
endSort(Dot *l, Dot *r)
{
	for (; l < r; ++l) {
		Dot *j = l;
		int ty = j[1].y;
		int tx = j[1].x;
		for (; ty < j->y || (ty == j->y && tx < j->x); --j)
			j[1] = j[0];
		j[1].y = ty;
		j[1].x = tx;
	}
}

// draw ignoring dropout candidates
static void
drawHorizontal(uint8_t *const bmp, int height, int dX)
{
	for (Dot *p = dots[0] + 1; p < dots0; p += 2) {
		if (p[1].x - p[0].x < 96)
			continue;

		int x1 = (p[0].x + 31) >> SHIFT;	/* 32, 31 */

		int y = height - (p->y >> SHIFT);
		TYPESLP *ptr = (TYPESLP *)&bmp[y * dX];
		ptr += x1 >> LOGSLP;

#if MSB_BIT_FIRST
		TYPESLP hscan = (TYPESLP)~0 >> (x1 & SLPMASK);
#else
		TYPESLP hscan = (TYPESLP)~0 << (x1 & SLPMASK);
#endif

		int x2 = (p[1].x - 32) >> SHIFT;	/* 31, 33 */
		int x = (x2 >> LOGSLP) - (x1 >> LOGSLP);
		if (x) {
			*ptr |= hscan;
			for (hscan = (TYPESLP)~0U; --x > 0;)
				*(++ptr) = hscan;
			++ptr;
		}

#if MSB_BIT_FIRST
		hscan ^= ((TYPESLP)1 << (~x2 & SLPMASK)) - 1;
#else
		hscan ^= (TYPESLP)~1 << (x2 & SLPMASK);
#endif
		*ptr |= hscan;
	}
}

// draw horizontal dropout candidates
static void
drawHDropouts(uint8_t *const bmp, int height, int dX)
{
	for (Dot *p = dots[0] + 1; p < dots0; p += 2) {
		if (p[1].x - p[0].x >= 96 )
			continue;

		int y = height - (p->y >> SHIFT);
		TYPESLP *ptr = (TYPESLP *)&bmp[y * dX];

		int x = (p[1].x + p[0].x) >> (SHIFT + 1);
		ptr += x >> LOGSLP;
#if MSB_BIT_FIRST
		// XXX: if ((*ptr >> ((~x & SLPMASK) - 1)) & 7 == 0)
		*ptr |= /*(TYPESLP)*/1 << (~x & SLPMASK);
#else
		// XXX: if ((*ptr >> ((x & SLPMASK) - 1)) & 7 == 0)
		*ptr |= /*(TYPESLP)*/1 << (x & SLPMASK);
#endif
	}
}

// draw vertical dropout candidates
static void
drawVDropouts(uint8_t *const bmp, int height, int dX)
{
	for (Dot *p = dots[1] + 1; p < dots1; p += 2) {
		if (p[1].x - p[0].x >= 63)
			continue;

		int x = p[0].y >> SHIFT;
#if MSB_BIT_FIRST
		TYPESLP hscan = (TYPESLP)1 << (~x & SLPMASK);
#else
		TYPESLP hscan = (TYPESLP)1 << (x & SLPMASK);
#endif
		int y = height - ((p[1].x + p[0].x) >> (SHIFT+1));
		TYPESLP *ptr = (TYPESLP *)&bmp[y * dX];
		ptr += x >> LOGSLP;
		hscan &= ~*(TYPESLP *)((uint8_t *)ptr - dX);
		hscan &= ~*(TYPESLP *)((uint8_t *)ptr + dX);
		*ptr |= hscan;
	}
}

void
Rasterizer::drawBitmap(uint8_t *const bmp, int height, int dX)
{
	// sort horizontal/vertical dots
	debug("dropoutControl = %d\n", gs.dropout_control);
	if (dots[0] + 1 < dots0) {
		preSort(dots[0] + 1, dots0);
		endSort(dots[0] + 1, dots0);
		drawHorizontal(bmp, height, dX);
		// XXX: if (gs.dropout_control)
			drawHDropouts(bmp, height, dX);
	}
	if (gs.dropout_control && dots[1] + 1 < dots1) {
		preSort(dots[1] + 1, dots1);
		endSort(dots[1] + 1, dots1);
		drawVDropouts(bmp, height, dX);
	}

#if DEBUG
	for (Dot *i1 = dots[0] + 1; i1 <= dots0; ++i1)
		debug("dh[%3zd] = (%5d %5d)\n", i1 - dots[0], i1->y, i1->x);
	debug("\n");
	for (Dot* i2 = dots[1] + 1; i2 <= dots1; ++i2)
		debug("dv[%3zd] = (%5d %5d)\n", i2 - dots[1], i2->x, i2->y);
	debug("\n");
#endif
}

void
Rasterizer::drawGlyph(uint8_t *const bmp, uint8_t *const endbmp)
{
	if (bmp + length >= endbmp) {
		length = 0;
		return;
	}

	memset(bmp, 0, length);
	dropoutInit();

	Point *startPoint = p[1];
	for (int i1 = nPoints[1]; --i1 >= 0; ++startPoint)
		startPoint->flags &= ON_CURVE | END_SUBGLYPH;

	startPoint = p[1];
	for (int i2 = 0; i2 < nEndPoints; ++i2) {
		Point *endPoint = p[1] + endPoints[i2];
		int flag = endPoint->flags & END_SUBGLYPH;
		endPoint->flags &= ON_CURVE;
		drawContour(startPoint, endPoint);
		startPoint = endPoint + 1;
		if (flag) {
			drawBitmap(bmp, height - 1, dX);
			dropoutInit();
		}
	}
}

inline const Point *
Rasterizer::drawPoly(const Point &p0, const Point &p1, const Point &p2)
{
	if (p1.flags) { // x1x
		if (p0.flags) // 11x
			drawSegment(p0.xnow, p0.ynow, p1.xnow, p1.ynow);
		return &p0 + 1;
	}

	if (p0.flags && p2.flags) { // 101
		bezier1(p0.xnow, p0.ynow, p1.xnow, p1.ynow, p2.xnow, p2.ynow);
		return &p0 + 2;
	}

	int px0 = p0.xnow, py0 = p0.ynow;
	if (!p0.flags) {	// 00x
		px0 = (px0 + p1.xnow) >> 1;
		py0 = (py0 + p1.ynow) >> 1;
	}

	if (!p2.flags) {	// x00
		int px2 = (p2.xnow + p1.xnow) >> 1;
		int py2 = (p2.ynow + p1.ynow) >> 1;
		bezier1(px0, py0, p1.xnow, p1.ynow, px2, py2);
		return &p0 + 1;
	}

	bezier1(px0, py0, p1.xnow, p1.ynow, p2.xnow, p2.ynow);
	return &p0 + 2;
}

void
Rasterizer::drawContour(Point *const first, Point *const last)
{
	const Point *p0 = first;
	do {
		const Point *p1 = p0 + 1;
		const Point *p2 = p0 + 2;
		if (p1 == last) {
			p2 = first;
		} else if (p1 > last) {
			p1 = first;
			p2 = p1 + 1;
		}
		p0 = drawPoly(*p0, *p1, *p2);
	} while (p0 <= last);
}

void
Rasterizer::bezier1(int x0, int y0, int x11, int y11, int x2, int y2)
{
	// treat second order bezier spline as line (p2-p0)
	// if control point p1 is almost on this line:
	// C * |p2-p0| * |p1-p0| * sin <= |p2-p0| <= manhattan_distance(p2,p0)
manual_tail_recursion:
	int dx = x2 - x0;
	int dy = y2 - y0;
	int dd = dx * (y11 - y0) + dy * (x11 - x0);
	if (dd < 0) dd = -dd;
	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;

	if ((dd >> 4) <= (dx + dy + 4)) {
		drawSegment(x0, y0, x2, y2);
		return;
	}

	int x10 = x0 + x11;
	x11 += x2;
	int x20 = (x10 + x11) >> 2;
	x10 >>= 1;
	x11 >>= 1;

	int y10 = y0 + y11;
	y11 += y2;
	int y20 = (y10 + y11) >> 2;
	y10 >>= 1;
	y11 >>= 1;

	bezier1(x0, y0, x10, y10, x20, y20);
	x0 = x20;
	y0 = y20;
	goto manual_tail_recursion;
}

// the secrets of "grayscaling technology" ...
void
Rasterizer::antiAliasing2(uint8_t* bmp)
{
	void *buf0 = allocMem(length);
	memcpy(buf0, bmp, length);

	uint8_t *p1 = (uint8_t *)buf0;
	for (int y = height>>1; --y >= 0; p1 += dX) {
		for (int x = dX >> (LOGSLP - 3); --x >= 0;) {
			TYPESLP c1 = *(TYPESLP *)p1;
			TYPESLP c2 = *(TYPESLP *)(p1 + dX);
			p1 += 1U << (LOGSLP - 3);
			for (int i = SCANLINEPAD >> 1; --i >= 0;) {
				uint8_t c3 = (c1 & 1) + (c2 & 1)
					  + (((c1 & 2) + (c2 & 2)) >> 1);
#if MSB_BIT_FIRST
				bmp[i] = c3;
#else
				*(bmp++) = c3;
#endif
				c1 >>= 2;
				c2 >>= 2;
			}
#if MSB_BIT_FIRST
			bmp += SCANLINEPAD >> 1;
#endif
		}
	}

	deallocMem(buf0, length);

	height >>= 1;
	dX *= 4;
	width = dX;
	length = dX * height;
}
