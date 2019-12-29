/*
 * Glyph Table
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

GlyphTable::GlyphTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length),
	xmin(0), codeOffset(0), codeLength(0),
	nEndPoints(0), endPoints(nullptr), nPoints(0), points(nullptr)
{
}

int
GlyphTable::getGlyphData(int glyphNo, LocaTable *locaTable, Rasterizer *raster)
{
	// this unsigned fixes a compile warning but may be the wrong
	// way to fix it
	unsigned int glyphOffset = locaTable->getGlyphOffset(glyphNo);
	if (glyphOffset >= getLength())
		return 0;
	seekAbsolute(glyphOffset);

	nEndPoints = readSShort();
	xmin = readSShort();
#if 1
	seekRelative(6);
#else
	ymin = readSShort();
	xmax = readSShort();
	ymax = readSShort();
#endif

	if (nEndPoints < 0)
		return getCompositeGlyphData(glyphNo, locaTable, raster);

	for (int iEndPoints = 0; iEndPoints < nEndPoints; ++iEndPoints)
		endPoints[iEndPoints] = readUShort();
	nPoints = 1 + endPoints[nEndPoints - 1];

	codeLength = readUShort();
	codeOffset = tell();
	seekRelative(codeLength);

	Point *pp = points;
	for (int iFlags = nPoints; --iFlags >= 0; ++pp) {
		uint8_t flag = readUByte();
		if (flag & F_SAME)
			for (int j = readUByte(); --j >= 0; --iFlags, ++pp)
				pp->flags = flag;
		pp->flags = flag;
	}

	int16_t oldval = 0;
	pp = points;
	for (int iXpos = nPoints; --iXpos >= 0; ++pp) {
		switch (pp->flags & (X_SHORT | X_EXT)) {
		case 0:			// delta vector
			oldval += readSShort();
			break;
		case X_SHORT:		// negative delta
			oldval -= readUByte();
			break;
		case X_SHORT | X_EXT:	// positive delta
			oldval += readUByte();
			break;
		case X_EXT:		// same value
			break;
		}
		pp->xnow = oldval;
	}

	oldval = 0;
	pp = points;
	for (int iYpos = nPoints; --iYpos >= 0; ++pp) {
		switch (pp->flags & (Y_SHORT | Y_EXT)) {
		case 0:			// delta vector
			oldval += readSShort();
			break;
		case Y_SHORT:		// negative delta
			oldval -= readUByte();
			break;
		case Y_SHORT | Y_EXT:	// positive delta
			oldval += readUByte();
			break;
		case Y_EXT:		// same value
			break;
		}
		pp->ynow = oldval;
	}
	(pp - 1)->flags |= END_SUBGLYPH;

	raster->putGlyphData(nEndPoints, nPoints, endPoints, points,
	                     glyphNo, xmin);
	raster->scaleGlyph();
	raster->hintGlyph(this, codeOffset, codeLength);;

	return 1;
}

int
GlyphTable::getCompositeGlyphData(int glyphNo, LocaTable *locaTable,
				  Rasterizer *raster)
{
	uint16_t flag;

	int sumEndpoints = 0;
	int sumPoints = 0;

	int myMetricsLsb = 0, myMetricsAdv = 0;

	// read and process subGlyphs
	do {
		flag = readUShort();
		int glyphNo = readUShort();

		int tmp_position = tell();
		if (!getGlyphData(glyphNo, locaTable, raster))
			return 0;
		seekAbsolute(tmp_position);

		// determine subGlyph position
		int xarg, yarg;
		if (flag & ARGS_ARE_WORDS) {
			xarg = readSShort();
			yarg = readSShort();
		} else {
			xarg = readSByte();
			yarg = readSByte();
		}

		xarg = raster->scaleX(xarg, yarg);
		yarg = raster->scaleY(yarg, xarg);

		if (flag & ARGS_ARE_XY) {
			if (flag & ROUND_XY_TO_GRID) {
				xarg = (xarg + 32) & -64;
				yarg = (yarg + 32) & -64;
			}
		} else {
			// match points
			Point *p1 = &(points - sumPoints)[xarg];
			Point *p2 = &points[yarg];
			xarg = p1->xnow - p2->xnow;
			yarg = p1->ynow - p2->ynow;
		}

		// XXX: scale subGlyph
		int xxscale = 0x2000, yyscale = xxscale;
		int xyscale = 0, yxscale = xyscale;
		if (flag & HAS_A_SCALE) {
			yyscale = xxscale = readSShort();
		} else if (flag & HAS_XY_SCALE) {
			xxscale = readSShort();
			yyscale = readSShort();
		} else if (flag & HAS_2X2_SCALE) {
			xxscale = readSShort();
			xyscale = readSShort();
			yyscale = readSShort();
			yxscale = readSShort();
		}

		if (xxscale != 0x2000)
			for (int i = nPoints; --i >= 0;)
				points[i].xnow = (xxscale * points[i].xnow)
				                 >> 14;
		if (yyscale != 0x2000)
			for (int i = nPoints; --i >= 0;)
				points[i].ynow = (yyscale * points[i].ynow)
				                 >> 14;
		if (xyscale | yxscale)
			for (int i = nPoints; --i >= 0;) {
				int xnow = points[i].xnow;
				int ynow = points[i].ynow;
				points[i].xnow += (xyscale * ynow) >> 14;
				points[i].ynow += (yxscale * xnow) >> 14;
			}

		// move subGlyph
		for (int i = nPoints; --i >= 0;) {
			points[i].xold = points[i].xnow += xarg;
			points[i].yold = points[i].ynow += yarg;
		}

		for (int j = nEndPoints; --j >= 0;)
			endPoints[j] += sumPoints;

		sumEndpoints += nEndPoints;
		sumPoints += nPoints;

		endPoints += nEndPoints;
		points += nPoints;

		if (myMetricsAdv == 0/* XXX */ || flag & USE_MY_METRICS) {
			myMetricsLsb = points[0].xnow;
			myMetricsAdv = points[1].xnow;
		}
	} while (flag & MORE_COMPONENTS);

	if (flag & HAS_CODE) {
		codeLength = readUShort();
		codeOffset = tell();
		seekRelative(codeLength);
		debug("Composite Hints: ofs %05X, len %d\n",
		      codeOffset, codeLength);
	} else {
		codeLength = 0;
	}

	endPoints -= nEndPoints = sumEndpoints;
	points -= nPoints = sumPoints;

	raster->putGlyphData(nEndPoints, nPoints, endPoints, points, glyphNo,
	                     xmin);

	if (myMetricsAdv) {
		debug("Composite Hinting\n");
		points[nPoints].xold = points[nPoints].xnow = myMetricsLsb;
		points[nPoints + 1].xold = points[nPoints + 1].xnow
		                         = myMetricsAdv;
	}

	raster->hintGlyph(this, codeOffset, codeLength);

	return 2;
}
