/*
 * Name Table
 *
 * $Id: table_name.cc,v 1.1 2002/11/14 12:08:11 guillem Exp $
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
#include <string.h>

NameTable::NameTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length)
{
	readUShort();			// format selector == 0
	nRecords = readUShort();
	strBase = readUShort();		// start of string offset
}

char *
NameTable::getString(int pfId, int strId, int *pLen, char *convbuf)
{
	// name records
	seekAbsolute(6);
	// search should be binary
	for (int i = 0; i < nRecords; ++i) {
		// 0: Apple Unicode
		// 1: Macintosh
		// 2: ISO
		// 3: MS encoding
		U16 platformId = readUShort();

		// 3.0 undefined charset
		// 3.1 UGL charset with unicode indexing
		/* U16 encodingId = */ readUShort();

		/* U16 languageId = */ readUShort();

		// 0: copyright notice
		// 1: font family
		// 2: font subfamily
		// 3: unique font id
		// 4: complete font name
		// 5: version
		// 6: postscript name
		// 7: trademark notice
		U16 nameId = readUShort();

		U16 strLength = readUShort();
		U16 strOffset = readUShort();

		if (platformId == pfId && nameId == strId) {
			char *p = (char *)base + strBase + strOffset;
#if 0
			for (int j = 0; j < strLength; ++j)
				dprintf1("%c", *(p++));
			dprintf0("\n");
#endif
			*pLen = strLength;
			p = (char *)base + strBase + strOffset;

			if (p <= (char *)base)
				return 0;
			if (p >= (char *)base + getLength())
				return 0;
			return p;
		}
	}

	// hack to convert unicode -> ascii
	if (convbuf && pfId == 1) {
		const char *p = getString(3, strId, pLen, 0);
		if (!p)
			return 0;

		*pLen >>= 1;
		for (int i = *pLen; --i >= 0;)
			convbuf[i] = p[2 * i + 1];

		return convbuf;
	}

	return 0;
}
