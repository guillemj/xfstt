/*
 * Test ttf engine performance
 *
 * Copyright © 1997-1998 Herbert Duerr
 * Copyright © 2008-2009, 2012, 2016, 2018-2019 Guillem Jover
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

#include "config.h"

#define MAXFONTBUFSIZE (2048*2048)

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include <cctype>
#include <cstring>
#include <cstdlib>

#include "ttf.h"
#include "ttfn.h"

#ifndef MAGNIFY
int MAGNIFY = 0;
#endif /* MAGNIFY */

static int numGlyphs = 0;

static int
ttPerfDir(Rasterizer *raster, int pt, FontExtent *fe, const char *ttdir)
{
	int nfonts = 0;
	printf("xfstt: perftest in directory " FONTDIR "/%s\n", ttdir);
	DIR *dirp = opendir(".");

	while (dirent *de = readdir(dirp)) {
		string filename(de->d_name);
		int namelen = filename.size();

		if (namelen - 4 <= 0)
			continue;

		string ext = string(filename, namelen - 4, namelen);

		if (ext != ".ttf")
			continue;

		struct stat statbuf;
		if (stat(de->d_name, &statbuf) < 0)
			continue;
		if (!S_ISREG(statbuf.st_mode))
			continue;

		struct timeval t0, t1;
		gettimeofday(&t0, nullptr);

		static int countFonts = 0;
		printf("opening \"%s\",\tno. %5d\n", de->d_name, countFonts++);
		fflush(stdout);
		if (!strcmp("DAVSDING.TTF", de->d_name))
			continue;
		if (!strcmp("FH0495.TTF", de->d_name))
			continue;
		if (!strcmp("GAELACH.TTF", de->d_name))
			continue;

		TTFont ttFont(de->d_name);
		if (ttFont.badFont())
			continue;

		FontInfo fi;
		ttFont.getFontInfo(&fi);
		if (fi.faceLength > 31)
			fi.faceLength = 31;
		fi.faceName[fi.faceLength] = 0;
		printf("TTF(\"%s\")", fi.faceName);

		raster->useTTFont(&ttFont);
		raster->setPointSize(pt, 0, 0, pt, 96, 96);

		numGlyphs += ttFont.maxpTable->getNumGlyphs();
		raster->getFontExtent(fe);

		++nfonts;

		gettimeofday(&t1, nullptr);
		double dt = (t1.tv_sec - t0.tv_sec) * 1.0e+3;
		dt += (t1.tv_usec - t0.tv_usec) * 1.0e-3;

		string indent(fi.faceLength >> 3, '\t');
		printf("%s\t%7.3f ms\n", indent.c_str(), dt);
	}

	closedir(dirp);
	return nfonts;
}

int
main(int argc, char **argv)
{
	if (chdir(FONTDIR)) {
		fputs("xfstt: " FONTDIR " does not exist!\n", stderr);
		return -1;
	}

	int ptsize = 0;
	if (argc > 1)
		ptsize = atoi(argv[1]);
	if (ptsize <= 0)
		ptsize = 12;

	printf("perftest(ptsize = %d, resolution = 96)\n", ptsize);

	FontExtent fe;
	fe.buflen = MAXFONTBUFSIZE;
	fe.buffer = (uint8_t *)allocMem(fe.buflen);

	Rasterizer raster;

	int nfonts = 0;
	nfonts += ttPerfDir(&raster, ptsize, &fe, ".");
	DIR *dirp = opendir(".");
	while (dirent *de = readdir(dirp)) {
		chdir(FONTDIR);
		if (de->d_name[0] != '.' && !chdir(de->d_name))
			nfonts += ttPerfDir(&raster, ptsize, &fe, de->d_name);
	}
	printf("\nTested %d fonts (%d glyphs)\n", nfonts, numGlyphs);

	deallocMem(fe.buffer, fe.buflen);
	closedir(dirp);
	return 0;
}
