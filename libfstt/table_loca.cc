/*
 * Character Location Table
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

LocaTable::LocaTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length),
	maxGlyph(), isShort()
{
}

int
LocaTable::getGlyphOffset(int glyphNo)
{
	int ofs;
	if (glyphNo < 0 || glyphNo >= maxGlyph)
		ofs = -1;
	else if (!isShort) {
		seekAbsolute(glyphNo << 2);
		ofs = readUInt();
		if ((unsigned)ofs == readUInt())
			ofs = -1;
	} else {
		seekAbsolute(glyphNo << 1);
		uint16_t ofs16 = readUShort();
		if (ofs16 != readUShort())
			ofs = ofs16 << 1;
		else
			ofs = -1;
	}

	return ofs;
}
