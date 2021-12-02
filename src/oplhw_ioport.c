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
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct oplhw_ioport_device
{
	oplhw_device dev;
	int iobase;
	int devport_fd;
} oplhw_ioport_device;

static void ioport_WritePort(oplhw_ioport_device *io_dev, int port, uint8_t val)
{
#ifndef USE_DEV_PORT
	outb(val, io_dev->iobase + port);
#else
	lseek(io_dev->devport_fd, io_dev->iobase + port, SEEK_SET);
	write(io_dev->devport_fd, &val, 1);
#endif
}

void oplhw_ioport_Write(oplhw_device *dev, uint8_t reg, uint8_t val)
{
	oplhw_ioport_device *io_dev = (oplhw_ioport_device *)dev;
	ioport_WritePort(io_dev, 0, reg);
	usleep(10);
	ioport_WritePort(io_dev, 1, val);
	usleep(30);
}

void oplhw_ioport_CloseDevice(oplhw_device *dev)
{
	oplhw_ioport_device *io_dev = (oplhw_ioport_device *)dev;
	/* Reset the device again, so we don't have hanging notes */
	for (int i = 0; i < 256; ++i)
	{
		ioport_WritePort(io_dev, 0, i);
		ioport_WritePort(io_dev, 1, 0x00);
	}
	ioperm(io_dev->iobase, 4, 0);
}

oplhw_device *oplhw_ioport_OpenDevice(const char *dev_name)
{
	oplhw_ioport_device *dev = calloc(sizeof(*dev), 1);

	dev->dev.close = &oplhw_ioport_CloseDevice;
	dev->dev.write = &oplhw_ioport_Write;

	dev->iobase = strtol(dev_name, NULL, 16);

	/* Default to the legacy AdLib port. */
	if (!dev->iobase)
		dev->iobase = 0x388;

#ifndef USE_DEV_PORT
	if (ioperm(dev->iobase, 4, 1) < 0)
	{
		free(dev);
		return NULL;
	}
#else
	dev->devport_fd = open("/dev/port", O_RDWR);
	if (dev->devport_fd < 0)
	{
		free(dev);
		return NULL;
	}
#endif

	/* Diasble OPL3 mode. */
	ioport_WritePort(dev, 2, 0x05);
	ioport_WritePort(dev, 3, 0x00);

	/* And reset. */
	for (int i = 0; i < 256; ++i)
	{
		ioport_WritePort(dev, 0, i);
		ioport_WritePort(dev, 1, 0x00);
	}

	return (oplhw_device *)dev;
}

