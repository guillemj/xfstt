/*
 * Character Map Table
 *
 * Copyright © 1997-1998 Herbert Duerr
 * mac7 style ttf support by Ethan Fischer
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

CmapTable::CmapTable(RandomAccessFile &f, int offset, int length):
	RandomAccessFile(f, offset, length),
	format(-1), subtableOffset(),
	f4NSegments(), f6FirstCode(), f6EntryCount()
{
	/* version = */ readUShort();
	int16_t nSubTables = readSShort();

	for (int i = nSubTables; --i >= 0;) {
		/* platformID = */ readUShort();
		/* encodingID = */ readUShort();
		subtableOffset = readUInt();
		// XXX
	}

	if (subtableOffset) {
		seekAbsolute(subtableOffset);
		format = readUShort();
	}

	switch (format) {
	case BYTE_ENCODING: // normal Mac format
		// format = readUShort();
		// length = readUShort();
		// version = readUShort();
		break;
	case HIGH_BYTE_MAPPING: // not supported
		debug("CMAP table format = HIGH_BYTE_MAPPING\n");
		break;
	case SEGMENT_MAPPING: // normal Windows format
		// format = readUShort();	// == 4
		/* length = */ readUShort();
		/* version = */ readUShort();
		f4NSegments = readSShort() >> 1;
		// searchRange = readUShort();
		// entrySelector = readUShort();
		// rangeShift = readUShort();
		// short[] endCount;
		// readShort(); // reserved
		// short[] startCount;
		// short[] idDelta;
		// short[] idRangeOffset;
		break;
	case TRIMMED_MAPPING: // newer Mac fonts use this?
		// format = readUShort();
		/* length = */ readUShort();
		/* version = */ readUShort();
		f6FirstCode = readUShort();
		f6EntryCount = readUShort();
		break;
	case -1: // no encoding tables
		break;
	default: // unknown table format
		debug("CMAP table format = %d\n", format);
		break;
	}
}

int
CmapTable::char2glyphNo(char char8)
{
	return unicode2glyphNo(char8);
}

int
CmapTable::unicode2glyphNo(uint16_t unicode)
{
	if (format == -1)
		return 0;
	else if (format == BYTE_ENCODING) {
		if (unicode > 255)
			return 0;
		seekAbsolute(subtableOffset + 6 + unicode);
		int glyphNo = readUByte();
		debug("MAC.cmap[%d] = %d\n", unicode, glyphNo);
		return glyphNo;
	} else if (format == TRIMMED_MAPPING) {
		if ((unicode < f6FirstCode)
		    || (unicode >= f6FirstCode + f6EntryCount))
			return 0;
		seekAbsolute(subtableOffset + 10 + (unicode - f6FirstCode) * 2);
		int glyphNo = readUShort();
		return glyphNo;
	} else if (format != SEGMENT_MAPPING)
		return 0;

	// search for endCount
	int lower = 0;
	int upper = f4NSegments - 1;
	int index = 0;
	while (upper > lower) {
		index = (upper + lower) >> 1;
		// XXX: read header and do more checking
		seekAbsolute(subtableOffset + 14 + (index << 1));
		if (readUShort() >= unicode)
			upper = index;
		else
			lower = index + 1;
	}

	// check corresponding startCount
	int ofs = subtableOffset + 16 + (f4NSegments << 1) + (lower << 1);
	seekAbsolute(ofs);
	uint16_t startCount = readUShort();
	if (unicode < startCount)
		return 0;

	// get idDelta
	seekAbsolute(ofs += (f4NSegments << 1));
	int idDelta = readSShort();

	// get idRangeOffset
	seekAbsolute(ofs += (f4NSegments << 1));
	uint16_t idRangeOffset = readUShort();

	// calculate GlyphIndex
	if (idRangeOffset == 0)
		return (unicode + idDelta);

	seekAbsolute(ofs + idRangeOffset + ((unicode - startCount) << 1));
	return readUShort();
}

uint16_t
CmapTable::nextUnicode(uint16_t unicode)
{
	++unicode;

	if (format == -1)
		return 0;
	else if (format == BYTE_ENCODING) {
		if (unicode > 255)
			return 0;
		return unicode;
	} else if (format == TRIMMED_MAPPING) {
		if (unicode < f6FirstCode)
			return f6FirstCode;
		else if (unicode >= f6FirstCode + f6EntryCount)
			return 0;
		
		return unicode;
	} else if (format != SEGMENT_MAPPING)
		return 0;

	int lower = 0;
	int upper = f4NSegments - 1;
	if (lower > upper)
		return 0;
	while (upper > lower) {
		int index = (upper + lower) >> 1;
		seekAbsolute(subtableOffset + 14 + (index << 1));
		if (readUShort() >= unicode)
			upper = index;
		else
			lower = index + 1;
	}

	// check corresponding startCount
	seekAbsolute(subtableOffset + 16 + (f4NSegments << 1) + (lower << 1));
	int startCount = readUShort();
	if (startCount == 0xffff)
		return 0;
	if (startCount > unicode)
		return startCount;
	return unicode;
}

uint16_t
CmapTable::firstUnicode()
{
	if (format == -1)
		return 0;
	else if (format == BYTE_ENCODING)
		return 0;
	else if (format == TRIMMED_MAPPING)
		return f6FirstCode;
	else if (format != SEGMENT_MAPPING)
		return 0;

	seekAbsolute(subtableOffset + 16 + (f4NSegments << 1));
	uint16_t i = readUShort();
	debug("First Unicode = %d\n", i);
	return i;
}

uint16_t
CmapTable::lastUnicode()
{
	if (format == -1)
		return 0;
	else if (format == BYTE_ENCODING)
		return 255;
	else if (format == TRIMMED_MAPPING)
		return f6FirstCode + f6EntryCount - 1;
	else if (format != SEGMENT_MAPPING)
		return 0;

	seekAbsolute(subtableOffset + 14 + ((f4NSegments - 2) << 1));
	uint16_t i = readUShort();
	debug("Last Unicode = %d\n", i);
	return i;
}
