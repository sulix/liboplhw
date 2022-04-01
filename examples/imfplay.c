/*
 * oplhw: ALSA hwdep-based library for OPL2-based soundcards.
 *
 * Copyright (C) 2021 by David Gow <david@davidgow.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "oplhw.h"

#define IMFRATE 560

typedef struct IMFPacket {
	uint8_t reg;
	uint8_t val;
	uint16_t delay;
} IMFPacket;

int main(int argc, char **argv)
{
	const char *filename = argv[2];
	const char *devname = argv[1];

	printf("%s: A simple IMF player using liboplhw.\n", argv[0]);
	printf("(C) 2021 David Gow\n");
	printf("\n");

	if (argc < 2)
	{
		printf("Usage: %s [device] [filename]\n", argv[0]);
		printf("\tdevice: The ALSA hwdep OPL2 device.\n");
		printf("\t\tThis is usually something like \"hw:0,0\"\n");
		printf("\t\tTry looking in /proc/asound/hwdep.\n");
		printf("\tfilename: The IMF file to play.\n");
		return -1;
	}

	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		perror("fopen");
		return -2;
	}

	oplhw_device *dev = oplhw_OpenDevice(devname);
	if (!dev)
	{
		fprintf(stderr, "Couldn't open OPL2 device \"%s\"\n", devname);
		fclose(f);
		return -3;
	}

	/* "Type 1" IMF files start with a length. Assume we're a "Type 1" file
	 * unless the length is 0.
	 */
	uint16_t len;
	if (fread(&len, sizeof(len), 1, f) != 1)
	{
		fprintf(stderr, "Couldn't read a length from \"%s\"!\n", filename);
		oplhw_CloseDevice(dev);
		fclose(f);
		return -4;
	}

	if (len == 0)
		fseek(f, 0, SEEK_SET);

	/* Keep reading until end of file. This will break if there are tags.*/
	while (!feof(f))
	{
		IMFPacket p;
		if (fread(&p, sizeof(p), 1, f) != 1)
		{
			fprintf(stderr, "Couldn't read a packet from \"%s\"!\n", filename);
			oplhw_CloseDevice(dev);
			fclose(f);
			return -5;
		}

#ifdef DEBUG
		printf("reg = %x, val = %x, delay = %x (%d ms)\n", p.reg, p.val, p.delay, p.delay*1000/IMFRATE);
#endif
		oplhw_Write(dev, p.reg, p.val);
		if (p.delay)
			usleep(p.delay*1000000/IMFRATE);

		/* This'll underflow for type-0 files. */
		len -= sizeof(IMFPacket);
		if (!len)
			break;
	}

	oplhw_CloseDevice(dev);

}
