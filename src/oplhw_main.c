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

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oplhw.h"
#include "oplhw_internal.h"

bool oplhw_IsOPL3(oplhw_device *dev)
{
	return dev->isOPL3;
}

void oplhw_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	dev->write(dev, reg, val);
}

void oplhw_Reset(oplhw_device *dev)
{
	int i;

	/* NOTE: This resets an OPL3 back to OPL2 mode! */
	for (i = 0; i < 256; ++i)
	{
		dev->write(dev, i, 0x00);
	}
	
	if (dev->isOPL3)
	{
		for (i = 0x100; i < 0x1FF; ++i)
		{
			dev->write(dev, i, 0x00);
		}
	}
}

static const char* get_protocol_path(const char *prefix, const char *path)
{
	size_t prefix_len = strlen(prefix);
	if (!path)
		return NULL;
	if (strncmp(path, prefix, prefix_len) == 0)
		return path + prefix_len;
	return NULL;
}

oplhw_device *oplhw_OpenDevice(const char *dev_name)
{
	oplhw_device *dev = NULL;
	const char *relative_dev_name;

	/* Default to the device in $OPLHW_DEVICE if none specified. */
	if (!dev_name || !*dev_name)
	{
#ifdef HAVE_SECURE_GETENV
		dev_name = secure_getenv("OPLHW_DEVICE");
#else
		dev_name = getenv("OPLHW_DEVICE");
#endif
	}

#ifdef WITH_OPLHW_MODULE_RETROWAVE
	if (relative_dev_name = get_protocol_path("retrowave:", dev_name))
	{
		if (dev = oplhw_retrowave_OpenDevice(relative_dev_name))
			return dev;
		return NULL;
	}
#endif
#ifdef WITH_OPLHW_MODULE_IOPORT
	else if (relative_dev_name = get_protocol_path("ioport:", dev_name))
	{
		if (dev = oplhw_ioport_OpenDevice(relative_dev_name))
			return dev;
	}
#endif
#ifdef WITH_OPLHW_MODULE_LPT
	else if (relative_dev_name = get_protocol_path("opl2lpt:", dev_name))
	{
		if (dev = oplhw_lpt_OpenDevice(relative_dev_name, false))
			return dev;
	}
	else if (relative_dev_name = get_protocol_path("opl3lpt:", dev_name))
	{
		if (dev = oplhw_lpt_OpenDevice(relative_dev_name, true))
			return dev;
	}
#endif
#ifdef WITH_OPLHW_MODULE_ALSA
	else if (relative_dev_name = get_protocol_path("alsa:", dev_name))
	{
		if (dev = oplhw_alsa_OpenDevice(relative_dev_name))
			return dev;
	}
	else if (dev = oplhw_alsa_OpenDevice(dev_name))
	{
		/* Fallback to ALSA by default. */
		return dev;
	}
#endif

	return NULL;
}

void oplhw_CloseDevice(oplhw_device *dev)
{
	dev->close(dev);
}
