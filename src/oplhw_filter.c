/*
 * oplhw: ALSA hwdep-based library for OPL2-based soundcards.
 *
 * Copyright (C) 2023 by David Gow <david@davidgow.net>
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

#include <stdlib.h>

#include "oplhw.h"
#include "oplhw_internal.h"

typedef struct oplhw_filter_device
{
	oplhw_device dev;
	oplhw_device *next;
} oplhw_filter_device;

typedef struct oplhw_volume_filter_device
{
	oplhw_filter_device dev;
	int volume;
} oplhw_volume_filter_device;

void oplhw_filter_CloseDevice(oplhw_device *dev)
{
	oplhw_filter_device *filter_dev = (oplhw_filter_device *)dev;
	filter_dev->next->close(filter_dev->next);
	free(filter_dev);
}

void oplhw_volume_filter_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	oplhw_volume_filter_device *vol_dev = (oplhw_volume_filter_device *)dev;

	/* If we've got a volume set command. */
	if ((reg & 0xe0) == 0x40)
	{
		int volume = ~val & 0x3f;

		/* Scale the volume. */
		volume = (volume * vol_dev->volume) >> 8;

		/* Update the value. */
		val = (val & ~0x3f) | (~volume & 0x3f);
	}

	vol_dev->dev.next->write(vol_dev->dev.next, reg, val);
}

oplhw_device *oplhw_CreateVolumeFilter(oplhw_device *backing_dev)
{
	oplhw_volume_filter_device *dev = calloc(sizeof(*dev), 1);
	dev->dev.dev.close = oplhw_filter_CloseDevice;
	dev->dev.dev.write = oplhw_volume_filter_Write;
	dev->dev.next = backing_dev;
	dev->volume = 255;
	return (oplhw_device *)dev;
}

int oplhw_SetVolume(oplhw_device *volume_dev, int volume)
{
	oplhw_volume_filter_device *vol_dev = (oplhw_volume_filter_device *)volume_dev;
	int old_vol = vol_dev->volume;
	vol_dev->volume = volume;
	return old_vol;
}
