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
	if ((relative_dev_name = get_protocol_path("retrowave:", dev_name)))
	{
		if ((dev = oplhw_retrowave_OpenDevice(relative_dev_name)))
			return dev;
		return NULL;
	}
#endif
#ifdef WITH_OPLHW_MODULE_IOPORT
	else if ((relative_dev_name = get_protocol_path("ioport:", dev_name)))
	{
		if ((dev = oplhw_ioport_OpenDevice(relative_dev_name)))
			return dev;
	}
#endif
#ifdef WITH_OPLHW_MODULE_LPT
	else if ((relative_dev_name = get_protocol_path("opl2lpt:", dev_name)))
	{
		if ((dev = oplhw_lpt_OpenDevice(relative_dev_name, false)))
			return dev;
	}
	else if ((relative_dev_name = get_protocol_path("opl3lpt:", dev_name)))
	{
		if ((dev = oplhw_lpt_OpenDevice(relative_dev_name, true)))
			return dev;
	}
#endif
#ifdef WITH_OPLHW_MODULE_ALSA
	else if ((relative_dev_name = get_protocol_path("alsa:", dev_name)))
	{
		if ((dev = oplhw_alsa_OpenDevice(relative_dev_name)))
			return dev;
	}
	else if ((dev = oplhw_alsa_OpenDevice(dev_name)))
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

struct oplhw_devlist *oplhw_devlist_new()
{
	struct oplhw_devlist *list = calloc(1, sizeof(struct oplhw_devlist));
	return list;
}

void oplhw_devlist_add(struct oplhw_devlist *list, const char *prefix, const char *path, const char *desc)
{
	struct oplhw_devlist *new_entry = list;
	if (list->path)
	{
		new_entry = calloc(1, sizeof(struct oplhw_devlist));
		list->next = new_entry;
	}
	asprintf(&new_entry->path, "%s%s", prefix, path);
	new_entry->description = strdup(desc);
}

void oplhw_devlist_free(struct oplhw_devlist *list)
{
	struct oplhw_devlist *entry;
	for (entry = list; entry;)
	{
		if (entry->path)
			free(entry->path);
		if (entry->description)
			free(entry->description);
		struct oplhw_devlist *old_entry = entry;
		entry = entry->next;
		free(old_entry);
	}
}

OPLHW_API oplhw_devlist *oplhw_Enumerate()
{
	const char *relative_dev_name;
	struct oplhw_devlist *list = oplhw_devlist_new();

	/* Default to the device in $OPLHW_DEVICE if none specified. */
	#ifdef HAVE_SECURE_GETENV
	const char *dev_name = secure_getenv("OPLHW_DEVICE");
	#else
	const char *dev_name = getenv("OPLHW_DEVICE");
	#endif

	if (dev_name)
	{
		oplhw_devlist_add(list, "", dev_name, "User-provided OPLHW_DEVICE");
	}

	#ifdef WITH_OPLHW_MODULE_RETROWAVE
	oplhw_retrowave_Enumerate(list);
	#endif
	#ifdef WITH_OPLHW_MODULE_IOPORT
	oplhw_ioport_Enumerate(list);
	#endif
	#ifdef WITH_OPLHW_MODULE_LPT
	oplhw_lpt_Enumerate(list);
	#endif
	#ifdef WITH_OPLHW_MODULE_ALSA
	oplhw_alsa_Enumerate(list);
	#endif

	return list;
}
