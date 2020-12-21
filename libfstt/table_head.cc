/*
 * Head Table
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

HeadTable::HeadTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length),
	flags(), emUnits(), xmin(), ymin(), xmax(), ymax(),
	macStyle(), lowestPP(), locaMode()
{
	/* version = */ readUInt();
	/* revision = */ readUInt();
	checksumAdj = readUInt();		// checkSumAdjustment
	headMagic = readUInt();
	if (headMagic != VALIDHEAD_MAGIC)
		return;
	flags = readUShort();
	emUnits = readUShort();
	/* ctime = */ readULong();		// seconds since 1904!!!
	/* mtime = */ readULong();		// seconds since 1904!!!
	xmin = readSShort();
	ymin = readSShort();
	xmax = readSShort();
	ymax = readSShort();
	macStyle = readUShort();		// macStyle 1:bold | 2:italic
	lowestPP = readUShort();		// lowestRecPPEM
	/* direction = */ readUShort();		// directionHint 1: left2right
	locaMode = readUShort();		// 0: shortGlyph, 1: longGlyph
	/* glyfFormat = */ readUShort();	// glyphDataFormat == 0
}
