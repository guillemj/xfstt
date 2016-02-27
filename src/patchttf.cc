/*
 * Quick and dirty hack to patch a ttf file and to fix the checksums
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

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

using std::string;

static void
usage()
{
	printf("Usage: patchttf fontfile -p 0xaddr 0xdata\n");
	// typical: CLEAR PUSHB00 0 IF ... EIF
	// => 22 B0 00 58 ... 59
}

static int
checksum(uint8_t *buf, int len)
{
	len = (len + 3) >> 2;
	int sum = 0;

	for (uint8_t *p = buf; --len >= 0; p += 4) {
		int val = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
		sum += val;
	}
	
	return sum;
}

static int
patchttf(int argc, char **argv)
{
	string inTTname, outTTname;

	if (argc < 2)
		return -1;

	inTTname = argv[1];
	outTTname = FONTDIR + inTTname;

	printf("i = %s\no = %s\n", inTTname.c_str(), outTTname.c_str());

	// read the stuff
	FILE *fp = fopen(inTTname.c_str(), "rb");
	if (!fp) {
		printf("Cannot read \"%s\"\n", inTTname.c_str());
		return -1;
	}

	struct stat st;
	if (fstat(fileno(fp), &st) < 0) {
		printf("Cannot stat \"%s\"\n", inTTname.c_str());
		return -1;
	}

	int flen = st.st_size;
	uint8_t *buf = new uint8_t[flen + 3];
	for (int ibuf = 0; ibuf < 3; ++ibuf)
		buf[flen + ibuf] = 0;
	printf("TTFsize = %d\n", flen);

	if (fread(buf, 1, flen, fp) != flen) {
		printf("Cannot read \"%s\"\n", inTTname.c_str());
		return -1;
	}
	fclose(fp);

	// patch as requested
	for (int i = 2; i < argc; i += 3) {
		char *p = argv[i];
		if (p[0] != '-' || p[1] != 'p')
			return -i;

		int addr = 0;
		p = argv[i+1];
		if (p[0] != '0' || p[1] != 'x')
			return -i;
		for (p += 2; *p; ++p) {
			char c = *p | 0x20;
			addr = (addr << 4)
			       + ((c > '9') ? c - 'a' + 10 : c - '0');
		}
		if (addr >= flen)
			return -i;

		p = argv[i + 2];
		if (p[0] != '0' || p[1] != 'x')
			return -i;
		for (p += 2; *p; ++p, ++addr) {
			int c = *p | 0x20;
			uint8_t nhex = (c > '9') ? c - 'a' + 10 : c - '0';
			c = *++p | 0x20;
			nhex <<= 4;
			nhex |= (c > '9') ? c - 'a' + 10 : c - '0';

			printf("patch %05X = %02X -> %02X\n",
			       addr, buf[addr], nhex);

			buf[addr] = nhex;
		}
	}

	// update the checksums
	int nTables = (buf[4] << 8) + buf[5];
	//printf("nTables = %d\n", nTables);
	uint8_t *headTable = 0;
	for (int iTable = 0; iTable < nTables; ++iTable) {
		uint8_t *b = &buf[12 + iTable * 16];
		int name = (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3];
		int offset = (b[8] << 24) + (b[9] << 16) + (b[10] << 8) + b[11];
		int length = (b[12] << 24) + (b[13] << 16) + (b[14] << 8) + b[15];
		//printf("offset = %08X, length = %08X\n", offset, length);
		int check = checksum(buf + offset, length);
		b[4] = (uint8_t)(check >> 24);
		b[5] = (uint8_t)(check >> 16);
		b[6] = (uint8_t)(check >> 8);
		b[7] = (uint8_t)check;
		//printf("checksum[%d] = %08X\n", iTable, check);
		if (name == 0x68656164) {
			headTable = buf + offset;
			//printf("headOffset = %08X\n", offset);
		}
	}

	int check = checksum(buf, flen) - 0xB1B0AFBA;
	//printf("csAdjust = %08X\n", check);
	headTable[8] = (uint8_t)(check >> 24);
	headTable[9] = (uint8_t)(check >> 16);
	headTable[10] = (uint8_t)(check >> 8);
	headTable[11] = (uint8_t)check;

	// write the patched file
	fp = fopen(outTTname.c_str(), "wb");
	if (!fp) {
		printf("Cannot write \"%s\"\n", outTTname.c_str());
		return -1;
	}
	if (fwrite(buf, 1, flen, fp) != flen) {
		printf("Cannot write \"%s\"\n", outTTname.c_str());
		return -1;
	}

	fclose(fp);

	delete[] buf;

	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 4) {
		usage();
		return 0;
	}

	int err = patchttf(argc, argv);
	if (err)
		printf("Error at arg %d\n", -err);

	return err;
}
