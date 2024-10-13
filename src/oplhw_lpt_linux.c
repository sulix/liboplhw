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

/* For O_PATH */
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "oplhw.h"
#include "oplhw_internal.h"

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/ppdev.h>



typedef struct oplhw_lpt_device
{
	oplhw_device dev;
	int fd;
} oplhw_lpt_device;


void oplhw_lpt_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	oplhw_lpt_device *lpt_dev = (oplhw_lpt_device *)dev;
	uint8_t reg_byte = reg & 0xFF;
	uint8_t reg_ctrl_byte0 = (0x01 | 0x04 | 0x08);
	uint8_t reg_ctrl_byte1 = (0x01 | 0x08);
	uint8_t val_ctrl_byte0 = (0x04 | 0x08);
	uint8_t val_ctrl_byte1 = (0x08);

	ioctl(lpt_dev->fd, PPWDATA, &reg_byte);


	ioctl(lpt_dev->fd, PPWCONTROL, &reg_ctrl_byte0);
	ioctl(lpt_dev->fd, PPWCONTROL, &reg_ctrl_byte1);
	ioctl(lpt_dev->fd, PPWCONTROL, &reg_ctrl_byte0);

	usleep(4);

	ioctl(lpt_dev->fd, PPWDATA, &val);


	ioctl(lpt_dev->fd, PPWCONTROL, &val_ctrl_byte0);
	ioctl(lpt_dev->fd, PPWCONTROL, &val_ctrl_byte1);
	ioctl(lpt_dev->fd, PPWCONTROL, &val_ctrl_byte0);
	usleep(33);
}

void oplhw_lpt_CloseDevice(oplhw_device *dev)
{
	oplhw_lpt_device *lpt_dev = (oplhw_lpt_device *)dev;

	ioctl(lpt_dev->fd, PPRELEASE);
	close(lpt_dev->fd);
	free(lpt_dev);
}

oplhw_device *oplhw_lpt_OpenDevice(const char *dev_name, bool isOPL3)
{
	oplhw_lpt_device *dev = calloc(1, sizeof(*dev));

	dev->dev.close = &oplhw_lpt_CloseDevice;
	dev->dev.write = &oplhw_lpt_Write;
	dev->dev.isOPL3 = isOPL3;

	dev->fd = open(dev_name, O_WRONLY);

	/* For compatibility with libieee1284, look in /dev. */
	if (dev->fd < 0)
	{
		int dir_fd = open("/dev", O_PATH | O_DIRECTORY);
		if (dir_fd < 0)
		{
			free(dev);
			return NULL;
		}
		dev->fd = openat(dir_fd, dev_name, O_WRONLY);
		close(dir_fd);
	}

	if (dev->fd < 0)
	{
		free(dev);
		return NULL;
	}

	if (ioctl(dev->fd, PPCLAIM))
	{
		free(dev);
		return NULL;
	}

	return (oplhw_device *)dev;
}

