/*
 * Kerning Table
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

KernTable::KernTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length),
	kernOffset(), nPairs(), kernLength(), coverage()
{
	/* version = */ readUShort();
	int16_t nTables = readSShort();

	for(; --nTables >= 0; seekRelative(kernLength - 14)) {
		/* subVersion = */ readUShort();
		kernLength = readUShort();
		coverage = readUShort();

		nPairs = readUShort();
		/* searchRange = */ readUShort();
		/* entrySelector = */ readUShort();
		/* rangeShift = */ readUShort();

		if ((coverage & FORMAT) == 0) {
			kernOffset = tell();
			break;
		}
	}
}

int
KernTable::getKerning(int /* leftChar */, int /* rightChar */)
{
	// TODO
	return 0;
}
