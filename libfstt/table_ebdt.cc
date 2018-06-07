/*
 * Embedded Bitmap Data Table
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

EbdtTable::EbdtTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length)
{
	/* int version = */ readUInt();	// == 0x00020000
}

int
EbdtTable::readBitmap(int format, uint8_t *bitmap,
                      GlyphMetrics *gm XFSTT_ATTR_UNUSED)
{
	int height, width;
/*
	int hBearX, hBearY;
	int vBearX, vBearY;
	int hAdv, vAdv;
*/

	// get glyph metric
	switch (format) {
	case 1:
	case 2:
	case 8:	// small metrics
		height = readUByte();
		width = readUByte();
		/* hBearX = */readSByte();
		/* hBearY = */readSByte();
		/* hAdv = */readUByte();
		break;
	case 3:	// obsolete
	case 4:	// unsupported
	default:
		debug("EBDT table bitmap format = %d\n", format);
		return -1;
	case 5:	// metrics in EBLC instead
		debug("EBDT table bitmap format = 5\n");
		break;
	case 6:
	case 7:
	case 9:	// big metrics
		height = readUByte();
		width = readUByte();
		/* hBearX = */readSByte();
		/* hBearY = */readSByte();
		/* hAdv = */readUByte();
		/* vBearX = */readSByte();
		/* vBearY = */readSByte();
		/* vAdv = */readUByte();
		break;
	}

	// get glyph bitmap
	switch (format) {
	case 1:
	case 6:	// byte aligned bitmap
		//memcpy(bitmap, tellAbs(), height * ((width + 7) >> 3));
		{
			int len = height * ((width + 7) >> 3);
			for (uint8_t *p = bitmap; --len >= 0; ++p)
				*p = readUByte();
		}
		break;
	case 5:	// metrics in EBLC instead, similar to format 2 and 7
		// TODO
		break;
	case 2:
	case 7:	// bit aligned bitmap
		{
		for (int rem = 0, h = height; --h >= 0;) {
			int data = 0, w;
			for (w = width; w >= 8; w -= 8) {
				int i = readUByte();
				data |= i >> rem;
				*(bitmap++) = data;
				data = i << (8 - rem);
			}
			rem = w & 7;
			if (rem)
				*(bitmap++) = data & (~0 << (8 - rem));
		}
		break;
		}
	case 8:
	case 9:	// composite bitmap
		debug("EBDT table bitmap format = %d\n", format);
		break;
	}

	return 0;
}
