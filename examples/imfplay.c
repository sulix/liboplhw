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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "oplhw.h"

#define DEFAULT_IMFRATE 560

typedef struct IMFPacket {
	uint8_t reg;
	uint8_t val;
	uint16_t delay;
} IMFPacket;

typedef struct KMFHeader {
	uint32_t sig;
	uint16_t rate;
	uint16_t len;
} KMFHeader;

#define KMF_SIG 0x1A464D4B
#define KMF_SIG_LOW (KMF_SIG & 0xFFFF) /* Low bits of the KMF id for detection */

int main(int argc, char **argv)
{
	const char *filename = argv[2];
	const char *devname = NULL;
	int rate = DEFAULT_IMFRATE, ticrate;
	int opl3 = 0;
	KMFHeader kmf_header;
	oplhw_device *dev;
	FILE *f;
	int i;

	printf("%s: A simple IMF player using liboplhw.\n", argv[0]);
	printf("(C) 2021 David Gow\n");
	printf("\n");
	
	for (i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-')
			break;

		if (!strcmp(argv[i], "--device"))
		{
			devname = argv[++i];
		}
		else if (!strcmp(argv[i], "--rate"))
		{
			rate = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "--opl3") || !strcmp(argv[i], "--stereo"))
		{
			opl3 = 1;
		}
	}

	if (i >= argc)
	{
		printf("Usage: %s [--device device] [--rate rate] [--opl3] [filename]\n", argv[0]);
		printf("\tdevice: The liboplhw device.\n");
		printf("\t\tThis is usually something like \"opl3lpt:parport0\"\n");
		printf("\trate: The IMF tick rate.\n");
		printf("\t\tThis is 560Hz for Commander Keen, 700Hz for Wolf 3D.\n");
		printf("\t\tModdingWiki has a full list! 560Hz is the default.\n");
		printf("\topl3: Playback in OPL3 mode\n");
		printf("\t\tThis is used to play back files from stereoimf.\n");
		printf("\t\tNote that you'll need OPL3 hardware.\n");
		printf("\tfilename: The IMF (or KMF) file to play.\n");
		return -1;
	}

	filename = argv[i];

	f = fopen(filename, "rb");
	if (!f)
	{
		perror("fopen");
		return -2;
	}

	dev = oplhw_OpenDevice(devname);
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
	{
		/* Type 0 IMF */
		fseek(f, 0, SEEK_SET);
	}
	else if (len == KMF_SIG_LOW)
	{
		/* We've got a KMF file. */
		fseek(f, 0, SEEK_SET);
		fread(&kmf_header.sig, sizeof(uint32_t), 1, f);
		fread(&kmf_header.rate, sizeof(uint16_t), 1, f);
		fread(&kmf_header.len, sizeof(uint16_t), 1, f);
		rate = kmf_header.rate;
		len = kmf_header.len;
	}

	
	ticrate = 1000000/rate;

	oplhw_Reset(dev);

	if (opl3)
	{
		if (!oplhw_IsOPL3(dev))
		{
			fprintf(stderr, "Warning: Requested OPL3 (stereo) mode, but this device doesn't support it!\n");
		}
		/* Enable the OPL3. If the file is StereoIMF, the channels are already encoded. */
		oplhw_Write(dev, 0x105, 1);
		oplhw_Write(dev, 0x104, 0);
	}

	/* Keep reading until end of file. This will break if there are tags.*/
	while (!feof(f))
	{
		if (kmf_header.sig == KMF_SIG)
		{
			// K1n9_Duk3's KMF file format
			uint8_t cmd_count, delay;
			if (fread(&cmd_count, 1, 1, f) != 1)
			{
				fprintf(stderr, "Couldn't read command count from KMF file \"%s\"\n", filename);
				oplhw_CloseDevice(dev);
				return -5;
			}
			if (fread(&delay, 1, 1, f) != 1)
			{
				fprintf(stderr, "Couldn't read command delay from KMF file \"%s\"\n", filename);
				oplhw_CloseDevice(dev);
				return -5;
			}
#ifdef DEBUG
			printf("num_commands = %d, delay = %x (%d ms)\n", cmd_count, delay, delay*1000/rate);
#endif
			len -= 2;
			while (cmd_count--)
			{
				uint8_t reg, val;
				if (fread(&reg, 1, 1, f) != 1)
				{
					fprintf(stderr, "Couldn't read command delay from KMF file \"%s\"\n", filename);
					oplhw_CloseDevice(dev);
					return -5;
				}
				if (fread(&val, 1, 1, f) != 1)
				{
					fprintf(stderr, "Couldn't read command delay from KMF file \"%s\"\n", filename);
					oplhw_CloseDevice(dev);
					return -5;
				}
				len -= 2;
				oplhw_Write(dev, reg, val);
			}
			if (delay)
				usleep((useconds_t)delay * ticrate);
			if (!len)
				break;
		}
		else
		{
			// A normal IMF file
			IMFPacket p;
			if (fread(&p, sizeof(p), 1, f) != 1)
			{
				fprintf(stderr, "Couldn't read a packet from \"%s\"!\n", filename);
				oplhw_CloseDevice(dev);
				fclose(f);
				return -5;
			}

#ifdef DEBUG
			printf("reg = %x, val = %x, delay = %x (%d ms)\n", p.reg, p.val, p.delay, p.delay*1000/rate);
#endif
			oplhw_Write(dev, p.reg, p.val);
			if (p.delay)
				usleep(p.delay*ticrate);

			/* This'll underflow for type-0 files. */
			len -= sizeof(IMFPacket);
			if (!len)
				break;
		}
	}

	oplhw_CloseDevice(dev);

	return 0;
}
