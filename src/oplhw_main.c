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



void oplhw_Write(oplhw_device *dev, uint8_t reg, uint8_t val)
{
	dev->write(dev, reg, val);
}

static const char* get_protocol_path(const char *prefix, const char *path)
{
	size_t prefix_len = strlen(prefix);
	if (strncmp(path, prefix, prefix_len) == 0)
		return path + prefix_len;
	return NULL;
}

oplhw_device *oplhw_OpenDevice(const char *dev_name)
{
	oplhw_device *dev = NULL;
	const char *relative_dev_name;
	if (relative_dev_name = get_protocol_path("retrowave:", dev_name))
	{
		if (dev = oplhw_retrowave_OpenDevice(relative_dev_name))
			return dev;
		return NULL;
	}
	else if (relative_dev_name = get_protocol_path("ioport:", dev_name))
	{
		if (dev = oplhw_ioport_OpenDevice(relative_dev_name))
			return dev;
	}
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

	return NULL;
}

void oplhw_CloseDevice(oplhw_device *dev)
{
	dev->close(dev);
}
