/*
 * Header for glyph remapping subsystem
 *
 * $Id: encoding.h,v 1.1 2002/11/14 12:08:08 guillem Exp $
 *
 * Copyright (C) 1998 Herbert Duerr
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

#ifndef ENCODING_H
#define ENCODING_H

class Encoding {
public:
	virtual ~Encoding() {}
	virtual int map2unicode(int code) = 0;
	virtual int hasGlyphs(int /*unicodeRange*/[4]) { return 1; }
	static void getDefault(Encoding** maps, int maxcodes);
	static int parse(char* mapnames, Encoding** maps, int maxcodes);
	static Encoding* find(char* mapname);
	static Encoding* enumerate(Encoding* iterator);

	const char	*strName;
	const int	lenName;

protected:
	Encoding(char* name);

private:
	static Encoding	*first, *last;
	Encoding	*next;
};

#endif

