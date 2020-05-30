/*
 * Architecture specifics, little endian 32bit
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

#ifndef LIBFSTT_ARCH_H
#define LIBFSTT_ARCH_H

// Compiler support

#ifdef __GNUC__
#define XFSTT_ATTR_UNUSED	__attribute__((unused))
#else
#define XFSTT_ATTR_UNUSED
#endif

#if defined(__cplusplus) && __cplusplus < 201103L
#define nullptr 0
#endif

// environment specific

#ifdef WORDS_BIGENDIAN
#  define MSB_BYTE_FIRST	1
#  define MSB_BIT_FIRST		2
#  define FSBYTEORDER		'B'
#else
#  define MSB_BYTE_FIRST	0
#  define MSB_BIT_FIRST		0
#  define FSBYTEORDER		'l'
#endif

// default SCANLINEPAD is 32bits = 1<<5
// change it to 3 for some font clients
#  define LOGSLP		5

// architecture specific

#include <netinet/in.h>

// byte swapping (doing it by ntohl/ntohs works only on little endian CPUs)
#define bswapl(x)	ntohl(x)
#define bswaps(x)	ntohs(x)

#ifdef __i386__
// MULDIV is worth putting in assembler,
// undefine it to use the generic method
#  define MULDIV muldiv
inline int muldiv(int a, int b, int c)
{
	int r, d;

	// 32Bit * 32Bit / 32Bit with intermediate 64Bit divisor
	__asm__("imull %2\n"
	        "idivl %4\n"
	        : "=&a"(r), "=&d" (d)
	        : "0"(a), "1"(b), "rm"(c));

	return r;
}
#endif

#ifndef MULDIV
#  define MULDIV(a, b, c) (int)(((int64_t)(a) * (b) + (c >> 1)) / (c))
#endif

//=========== add special cases here ==================

#ifdef __sgi
#  undef LOGSLP
#  define LOGSLP 3
#endif

#endif
