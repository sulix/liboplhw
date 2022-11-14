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

#include "oplhw.h"
#include "oplhw_internal.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



typedef struct oplhw_retrowave_device
{
	oplhw_device dev;
	int fd;
} oplhw_retrowave_device;

/* A weird "insert a 1 bit everywhere" protocol, see:
 * https://github.com/SudoMaker/RetroWave/blob/master/RetroWaveLib/Protocol/README.md
 */
static void retrowave_write_pkt(oplhw_retrowave_device *dev, uint8_t *bytes, size_t len)
{
	uint8_t packed[512];
	int out_offset = 1;
	uint16_t buffer = 0;
	int bits_buffered = 0;
	int bytes_written = 0;

	packed[0] = '\0';
	for(int i = 0; i < len; ++i)
	{
		if (bits_buffered > 7)
		{
			bits_buffered -= 7;
			packed[out_offset++] = ((buffer >> (bits_buffered - 1)) & 0xFE) | 1;
		}
		buffer = buffer << 8 | bytes[i];
		bits_buffered += 8;

	}
	while (bits_buffered)
	{
		if (bits_buffered >= 7)
		{
			bits_buffered -= 7;
			packed[out_offset++] = ((buffer >> (bits_buffered - 1)) & 0xFE) | 1;
		}
		else
		{
			packed[out_offset++] = (buffer << (7-bits_buffered + 1)) | 1;
			bits_buffered = 0;
		}
	}

	packed[out_offset++] = 0x02;

	while (bytes_written < out_offset)
	{
		ssize_t res = write(dev->fd, &packed[bytes_written], out_offset - bytes_written);
		if (res < 0)
			return;
		bytes_written += res;
	}
}

void oplhw_retrowave_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	oplhw_retrowave_device *rw_dev = (oplhw_retrowave_device *)dev;
	bool port = (reg & 0x100); /* Are we outputting to the 2nd port on OPL3? */
	uint8_t pkt[] = {0x42, 0x12, port ? 0xE5 : 0xE1, reg & 0xFF, port ? 0xE7 : 0xE3, val, 0xFB, val};
	retrowave_write_pkt(rw_dev, pkt, sizeof(pkt));

}

void oplhw_retrowave_CloseDevice(oplhw_device *dev)
{
	oplhw_retrowave_device *rw_dev = (oplhw_retrowave_device *)dev;

	close(rw_dev->fd);
	free(rw_dev);
}

oplhw_device *oplhw_retrowave_OpenDevice(const char *dev_name)
{
	oplhw_retrowave_device *dev = calloc(sizeof(*dev), 1);

	dev->dev.close = &oplhw_retrowave_CloseDevice;
	dev->dev.write = &oplhw_retrowave_Write;
	
	/* All RetroWave OPL3s are, indeed, OPL3s */
	dev->dev.isOPL3 = true;

	dev->fd = open(dev_name, O_RDWR);

	if (dev->fd < 0)
	{
		free(dev);
		return NULL;
	}

	return (oplhw_device *)dev;
}

