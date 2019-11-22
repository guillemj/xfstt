/*
 * Font scaler
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

Rasterizer::Rasterizer(int _grid_fitting, int _anti_aliasing,
		       int _sizeTwilight, int _sizePoints, int _sizeContours,
		       int _sizeStack, int _sizeCvt, int _sizeStor,
		       int _sizeFDefs):
	stackbase(nullptr), ttFont(nullptr), status(INVALID_FONT),
	sizeContours(_sizeContours), sizeStack(_sizeStack), sizeCvt(_sizeCvt),
	sizeStor(_sizeStor), sizeFDefs(_sizeFDefs), sizeIDefs(0),
	format(LOGSLP - 3),
	grid_fitting(_grid_fitting), anti_aliasing(_anti_aliasing)
{
	sizePoints[0] = _sizeTwilight;
	sizePoints[1] = _sizePoints;
	openDraw();
}

Rasterizer::~Rasterizer()
{
	endInterpreter();
	closeDraw();

	if (sizeContours)
		delete[] endPoints;
	if (sizePoints[1])
		delete[] p[1];
}

// useTTFont must be executed before setP*Size!
void
Rasterizer::useTTFont(TTFont* _ttFont, int _flags)
{
	flags = _flags;

	// set scaling defaults = 16points
	xx = yy = (16 * VGARES + 36) / 72;
	xy = yx = 0;

	if (!_ttFont || _ttFont->badFont()) {
		status = INVALID_FONT;
		return;
	}

	/* XXX */
#if 0
	if (ttFont == _ttFont)
		return;
#endif

	ttFont = _ttFont;
	status = NOT_READY;
	initInterpreter();

#define MEMSLACK 2
	int i = ttFont->maxpTable->maxPoints
		+ ttFont->maxpTable->maxCompPoints + 2;
	if (sizePoints[1] < i) {
		if (sizePoints[1])
			delete[] p[1];
		sizePoints[1] = MEMSLACK * i;
		p[1] = new Point[sizePoints[1]];
	}

	i = ttFont->maxpTable->maxContours + ttFont->maxpTable->maxCompContours;
	if (sizeContours < i) {
		if (sizeContours)
			delete[] endPoints;
		sizeContours = MEMSLACK * i;
		endPoints = new int[sizeContours];
	}

	ttFont->points = p[1];
	ttFont->endPoints = endPoints;
}

// prepare scaling of outlines
// it can only be called when useTTFont has defined a font
void
Rasterizer::setPointSize(int _xx, int _xy, int _yx, int _yy, int xres, int yres)
{
	debug("_xx = %d,\t_xy = %d\n", _xx, _xy);
	debug("_yx = %d,\t_yy = %d\n", _xy, _yy);

	if (!(_xx | _xy) || !(_yx | _yy))
		_xx = _yy = 12;

	if (!xres || !yres)
		xres = yres = VGARES;

	pointSize = (_xx + _yy) / 2;	// XXX: quick hack

	_xx = (_xx * xres + 36) / 72;
	_xy = (_xy * xres + 36) / 72;
	_yx = (_yx * yres + 36) / 72;
	_yy = (_yy * yres + 36) / 72;

	setPixelSize(_xx, _xy, _yx, _yy);
}

void
Rasterizer::setPixelSize(int _xx, int _xy, int _yx, int _yy)
{
	mppemx = xx = _xx ? _xx : 16;
	xy = _xy;
	yx = _yx;
	mppemy = yy = _yy ? _yy : 16;

	// 0177 would make more sense than 177, wouldn't it?
	// but since the reference implementation does it we should do it too
	for (xxexp = 0; xx > 177 || xx < -177 || yy > 177 || yy < -177; ++xxexp) {
		xx >>= 1; xy >>= 1;
		yx >>= 1; yy >>= 1;
	}
	debug("xx = %d, xy = %d\n", xx, xy);
	debug("yx = %d, yy = %d\n", yx, yy);
	debug("exp = %d\n", xxexp);

	if (ttFont)
		applyTransformation();
	else
		status = NOT_READY;
}

// only needed when emUnits != 2048, but since we don't know this until
// we know the TTFont we have to separate it from setP*Size()
void
Rasterizer::applyTransformation()
{
	int emUnits = ttFont->getEmUnits();
	debug("emUnits = %d\n", emUnits);

	for (; emUnits > 2048 && xxexp > 0; --xxexp)
		emUnits >>= 1;
	for (; emUnits < 2048; ++xxexp)
		emUnits <<= 1;
	if (emUnits != 2048) {
		xx = (xx << 11) / emUnits;
		xy = (xy << 11) / emUnits;
		yx = (yx << 11) / emUnits;
		yy = (yy << 11) / emUnits;
	}

	mppem = (mppemx + mppemy) >> 1;

	debug("xx = %d, xy = %d\n", xx, xy);
	debug("yx = %d, yy = %d\n", yx, yy);
	debug("exp = %d\n", xxexp);

	debug("mppem = %d, mppemx = %d, mppemy = %d\n", mppem, mppemx, mppemy);

	if (grid_fitting)
		calcCVT();

	status = TRAFO_APPLIED;
}

/* get font extent
 * XXX: is there a way to avoid going through all glyphs and interpret them?
 *      we need max/min-width, max/min-left, max/min-right, max/min-height, ...
 *      would it have advantages to delay the scan line conversion?
 */
void
Rasterizer::getFontExtent(FontExtent *fe)
{
	if (status == NOT_READY)
		applyTransformation();
	if (status == FONT_DONE)
		return;

	if (ttFont->os2Table) {
		int i = ttFont->os2Table->winAscent;
		fe->yWinAscent = ((yy * i) << xxexp) >> 11;
		i = ttFont->os2Table->winDescent;
		fe->yWinDescent = ((yy * i) << xxexp) >> 11;
	} else {
		int i = ttFont->hheaTable->yAscent;
		fe->yWinAscent = ((yy * i) << xxexp) >> 11;
		i = -ttFont->hheaTable->yDescent;
		fe->yWinDescent = ((yy * i) << xxexp) >> 11;
	}

	fe->xLeftMin = fe->xRightMin =
	fe->xAdvanceMin = fe->yAdvanceMin =
	fe->yAscentMin = fe->yDescentMin =
	fe->xBlackboxMin = fe->yBlackboxMin = INT_MAX;

	fe->xLeftMax = fe->xRightMax =
	fe->xAdvanceMax = fe->yAdvanceMax =
	fe->yAscentMax = fe->yDescentMax =
	fe->xBlackboxMax = fe->yBlackboxMax = INT_MIN;

	if (format < 0) {
		// fixed xBlackbox for all glyphs
		fe->xBlackboxMax = ttFont->getMaxWidth(mppemx);
		dX = (((fe->xBlackboxMax - 1) | ((8 << ~format) - 1)) + 1) >> 3;
	}

	uint32_t buflen = fe->buflen;
	uint8_t *endbmp = fe->buffer + buflen;

	CharInfo *ci = (CharInfo *)fe->buffer;
	fe->bmpFormat = MSB_BIT_FIRST | MSB_BYTE_FIRST;
	fe->numGlyphs = ttFont->maxpTable->numGlyphs;
	fe->bitmaps = (uint8_t *)&ci[fe->numGlyphs];
	uint8_t *buffer = fe->bitmaps;
	if (buffer >= endbmp) {
		// XXX: not even enough room for charinfo!!! what to do???
		return;
	}
	for (int glyphNo = 0; glyphNo < fe->numGlyphs; ++glyphNo, ++ci) {
		GlyphMetrics *gm = &ci->gm;

		ci->length = putGlyphBitmap(glyphNo, buffer, endbmp, gm);
		ci->offset = buffer - fe->bitmaps;
		buffer += ci->length;

#define MINMAX(x, y, z) \
	if (z < fe->x) \
		fe->x = z; \
	if (z > fe->y) \
		fe->y = z;

		MINMAX(xBlackboxMin, xBlackboxMax, gm->xBlackbox);
		MINMAX(yBlackboxMin, yBlackboxMax, gm->yBlackbox);

		MINMAX(xLeftMin, xLeftMax, -gm->xOrigin);
		MINMAX(xRightMin, xRightMax, gm->xBlackbox - gm->xOrigin);

		MINMAX(yAscentMin, yAscentMax, gm->yBlackbox - gm->yOrigin);
		MINMAX(yDescentMin, yDescentMax, gm->yOrigin);

		MINMAX(xAdvanceMin, xAdvanceMax, gm->xAdvance);
		MINMAX(yAdvanceMin, yAdvanceMax, gm->yAdvance);
	}

	fe->bmplen = buffer - fe->bitmaps;
	status = FONT_DONE;
}

int
Rasterizer::putChar8Bitmap(char c, uint8_t *bmp, uint8_t *endbmp,
                           GlyphMetrics *gm)
{
	debug("charNo8 = %d", c);
	int glyphNo = ttFont->getGlyphNo8(c);
	return putGlyphBitmap(glyphNo, bmp, endbmp, gm);
}

int
Rasterizer::putChar16Bitmap(int c, uint8_t *bmp, uint8_t *endbmp,
                            GlyphMetrics *gm)
{
	int glyphNo = ttFont->getGlyphNo16(c);
	debug("charNo16 = %d", c);
	return putGlyphBitmap(glyphNo, bmp, endbmp, gm);
}

int
Rasterizer::putGlyphBitmap(int glyphNo, uint8_t *bmp, uint8_t *endbmp,
                           GlyphMetrics *gm)
{
	debug("\n=============== glyphNo %d ==================\n", glyphNo);

	GlyphTable *g = ttFont->glyphTable;
	g->setupGlyph(ttFont->points, ttFont->endPoints);
	if (bmp >= endbmp || !g->getGlyphData(glyphNo, ttFont->locaTable, this)) {
		gm->xAdvance = ttFont->getGlyphWidth(mppemx, glyphNo);
		gm->yAdvance = 0;
		gm->xOrigin = gm->yOrigin = 0;
		gm->xBlackbox = gm->yBlackbox = 0;
		return (length = 0);
	}

	int xmin = INT_MAX, ymin = xmin;
	int xmax = INT_MIN, ymax = xmax;

	Point *pp = p[1];
	for (int i = nPoints[1]; --i >= 0; ++pp) {
		if (MAGNIFY > 1) {
			pp->xnow *= MAGNIFY;
			pp->ynow *= MAGNIFY;
		}
		if (anti_aliasing > 0) {
			pp->xnow <<= anti_aliasing;
			pp->ynow <<= anti_aliasing;
		}

		int xt = pp->xnow;
		if (xmin > xt)
			xmin = xt;
		if (xmax < xt)
			xmax = xt;

		int yt = pp->ynow;
		if (ymin > yt)
			ymin = yt;
		if (ymax < yt)
			ymax = yt;
	}

	if (anti_aliasing > 0) {
		xmin &= -64 << anti_aliasing;
		ymin &= -64 << anti_aliasing;
		ymax |= ~(-64 << anti_aliasing);
	} else {
		xmin &= -64;
		ymin &= -64;
		xmax += 63;
		ymax |= 63;
	}

#ifdef DEBUG
	printOutline();
#endif

	pp = p[1];
	nPoints[1] += 2;
	for (int j = nPoints[1]; --j >= 0; ++pp) {
		pp->xnow -= xmin;
		pp->ynow -= ymin;
	}

	xmax >>= SHIFT;
	ymax >>= SHIFT;
	xmin >>= SHIFT;
	ymin >>= SHIFT;

	gm->xBlackbox = width = xmax - xmin + 1;
	gm->yBlackbox = height = ymax - ymin + 1;

	pp -= 2;
	gm->xOrigin = pp[0].xnow >> SHIFT;
	gm->yOrigin = pp[0].ynow >> SHIFT;
	gm->xAdvance = (pp[1].xnow - pp[0].xnow) >> SHIFT;
	gm->yAdvance = (pp[1].ynow - pp[0].ynow) >> SHIFT;

	if (format >= 0)
		dX = (((width - 1) | ((8 << format) - 1)) + 1) >> 3;

	length = dX * height;

	if (ttFont->hdmxTable) { // XXX: hack
		int hdmx = ttFont->hdmxTable->getGlyphWidth(mppemx, glyphNo);
#if 0
		if (hdmx && gm->xAdvance != hdmx)
			debug("adv(%d) = %3d <-> %3d\n",
			      glyphNo, gm->xAdvance, hdmx);
#endif
		if (hdmx)
			gm->xAdvance = hdmx;
	}

	debug("width = %d, dX = %d, height = %d\n", width, dX, height);
	debug("gn=%d, length= %d\n", glyphNo, length);

	drawGlyph(bmp, endbmp);

	if (anti_aliasing == 1)
		antiAliasing2(bmp);
	return length;
}

void
Rasterizer::putGlyphData(int ne, int np, int *ep, Point *pp, int glyphNo,
		         int xmin)
{
	nEndPoints = ne;
	nPoints[1] = np;
	endPoints = ep;
	p[1] = pp;

	// prepare phantom point 0
	pp += np;
	int advanceWidth, lsb;
	ttFont->hmtxTable->getHMetrics(glyphNo, &advanceWidth, &lsb);
	int val = xmin - lsb;
	pp->xold = scaleX(val, 0);
	pp->yold = scaleY(0, val);
	debug("xmin = %d, adv = %d, lsb = %d\n", xmin, advanceWidth, lsb);
	pp->xnow = (pp->xold + 32) & -64;
	pp->ynow = (pp->yold + 32) & -64;
#if 0
	// XXX: reference implementation always has (0 0)!
	val = pp->xnow = pp->ynow = 0;
	pp->xold = pp->yold = 0;
#endif
	debug("phantom[0] = %5d -> %5d -> %5d\n", val, pp->xold, pp->xnow);

	// prepare phantom point 1
	val += advanceWidth;
	++pp;
	pp->xold = scaleX(val, 0);
	pp->yold = scaleY(0, val);
	pp->xnow = (pp->xold + 32) & -64;
	pp->ynow = (pp->yold + 32) & -64;
}

void
Rasterizer::scaleGlyph()
{
	// scale outline
	Point *pp = p[1];
	for (int i = nPoints[1]; --i >= 0; ++pp) {
		pp->xold = scaleX(pp->xnow, pp->ynow);
		pp->yold = scaleY(pp->ynow, pp->xnow);
		pp->xnow = pp->xold;
		pp->ynow = pp->yold;
	}

	switch (status) {
	case INVALID_FONT:
		return;
	case FONT_DONE:
		debug("### We shouldn't waste time like this ###\n");
		return;
	case NOT_READY:
		applyTransformation();
		// fall through
	case TRAFO_APPLIED:
		break;
	}
}

void
Rasterizer::printOutline(void)
{
	debug("\n=== grid fitted outline ===\n");
	Point *pp = p[1];
	for (int i = 0, j = 0; i < nPoints[1] + 2; ++i, ++pp) {
		debug("p[%d]\t%6d %6d  ", i, pp->xold, pp->yold);
		debug("-> %6d %6d", pp->xnow, pp->ynow);
		debug("  %d%d", (pp->flags & X_TOUCHED) != 0,
			 (pp->flags & Y_TOUCHED) != 0);

		debug(" %c", (pp->flags & ON_CURVE) ? '*' : ' ');

		debug("\n");
		if (i == endPoints[j]) {
			++j;
			debug("----\n");
		}
	 }
}
