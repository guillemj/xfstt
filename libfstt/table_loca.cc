/*
 * Character Location Table
 *
 * $Id: table_loca.cc,v 1.1 2002/11/14 12:08:11 guillem Exp $
 *
 * Copyright (C) 1997-1998 Herbert Duerr
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include "ttf.h"

LocaTable::LocaTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length)
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
		U16 ofs16 = readUShort();
		if (ofs16 != readUShort())
			ofs = ofs16 << 1;
		else
			ofs = -1;
	}

	return ofs;
}

