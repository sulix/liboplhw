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

#include <ieee1284.h>



typedef struct oplhw_lpt_device
{
	oplhw_device dev;
	struct parport *parport;
} oplhw_lpt_device;


void oplhw_lpt_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	oplhw_lpt_device *lpt_dev = (oplhw_lpt_device *)dev;

	ieee1284_write_data(lpt_dev->parport, reg & 0xFF);
	if (reg & 0x100)
	{
		ieee1284_write_control(lpt_dev->parport, (C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(lpt_dev->parport, (C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(lpt_dev->parport, (C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
	}
	else
	{
		ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN | C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
	}
	usleep(4);

	ieee1284_write_data(lpt_dev->parport, val);
	ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN | C1284_NINIT) ^ C1284_INVERTED);
	ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN) ^ C1284_INVERTED);
	ieee1284_write_control(lpt_dev->parport, (C1284_NSELECTIN | C1284_NINIT) ^ C1284_INVERTED);
	usleep(33);
}

void oplhw_lpt_CloseDevice(oplhw_device *dev)
{
	oplhw_lpt_device *lpt_dev = (oplhw_lpt_device *)dev;

	ieee1284_close(lpt_dev->parport);
	free(lpt_dev);
}

oplhw_device *oplhw_lpt_OpenDevice(const char *dev_name, bool isOPL3)
{
	oplhw_lpt_device *dev = calloc(sizeof(*dev), 1);
	struct parport_list all_ports = {};
	int caps = CAP1284_RAW;

	dev->dev.close = &oplhw_lpt_CloseDevice;
	dev->dev.write = &oplhw_lpt_Write;
	dev->dev.isOPL3 = isOPL3;

	if (ieee1284_find_ports(&all_ports, 0) != E1284_OK)
	{
		free(dev);
		return NULL;
	}

	for (int i = 0; i < all_ports.portc; ++i)
	{
		if (!dev_name[0] || !strcmp(dev_name, all_ports.portv[i]->name))
		{
			dev->parport = all_ports.portv[i];
			break;
		}
	}
	//ieee1284_free_ports(&all_ports);

	if (!dev->parport)
	{
		free(dev);
		return NULL;
	}

	if (ieee1284_open(dev->parport, F1284_EXCL, &caps) != E1284_OK)
	{
		free(dev);
		return NULL;
	}

	if (ieee1284_claim(dev->parport) != E1284_OK)
	{
		free(dev);
		return NULL;
	}

	return (oplhw_device *)dev;
}

