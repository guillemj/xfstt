/*
 * Name Table
 *
 * Copyright © 1997-1998 Herbert Duerr
 * Copyright © 2008 Guillem Jover
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

NameTable::NameTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length)
{
	readUShort();			// format selector == 0
	nRecords = readUShort();
	strBase = readUShort();		// start of string offset
}

string
NameTable::getString(int pfId, int strId)
{
	// name records
	seekAbsolute(6);
	// search should be binary
	for (int i = 0; i < nRecords; ++i) {
		// 0: Apple Unicode
		// 1: Macintosh
		// 2: ISO
		// 3: MS encoding
		uint16_t platformId = readUShort();

		// 3.0 undefined charset
		// 3.1 UGL charset with unicode indexing
		/* uint16_t encodingId = */ readUShort();

		/* uint16_t languageId = */ readUShort();

		// 0: copyright notice
		// 1: font family
		// 2: font subfamily
		// 3: unique font id
		// 4: complete font name
		// 5: version
		// 6: postscript name
		// 7: trademark notice
		uint16_t nameId = readUShort();

		uint16_t strLength = readUShort();
		uint16_t strOffset = readUShort();

		if (platformId == pfId && nameId == strId) {
			char *p = (char *)base + strBase + strOffset;

			if (p <= (char *)base)
				return nullptr;
			if (p >= (char *)base + getLength())
				return nullptr;

			return string(p, strLength);
		}
	}

	// hack to convert unicode -> ascii
	if (pfId == 1) {
		const string p = getString(3, strId);

		if (p.empty())
			return nullptr;

		int len = p.size() >> 1;

		string convbuf;
		convbuf.reserve(len);

		for (int i = len; --i >= 0;)
			convbuf[i] = p[2 * i + 1];

		return convbuf;
	}

	return nullptr;
}
