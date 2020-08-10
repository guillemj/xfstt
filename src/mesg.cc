/*
 * Message handling routines
 *
 * Copyright © 2004, 2020 Guillem Jover
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation;
 * version 2 of the License.
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

#include "config.h"

#include <cstdarg>
#include <cstdio>

#include "mesg.h"

void
info(const char *format, ...)
{
	std::va_list args;

	va_start(args, format);
	std::vfprintf(stdout, format, args);
	va_end(args);
}

void
warning(const char *format, ...)
{
	std::va_list args;

	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
}

void
error(const char *format, ...)
{
	std::va_list args;

	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
}
