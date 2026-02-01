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

#include <libudev.h>

/* Handle udev-based device enumeration on Linux systems. */

void oplhw_udev_enumerate_retrowave(struct oplhw_devlist *list)
{
	struct udev *udev = udev_new();
	struct udev_enumerate *udev_enum = udev_enumerate_new(udev);
	struct udev_list_entry *entry;

	udev_enumerate_add_match_property(udev_enum, "ID_MODEL", "Retrowave_USB_Adapter");
	udev_enumerate_add_match_subsystem(udev_enum, "tty");
	udev_enumerate_scan_devices(udev_enum);

	entry = udev_enumerate_get_list_entry(udev_enum);

	while (entry)
	{
		const char *sysfs_path = udev_list_entry_get_name(entry);
		struct udev_device *dev = udev_device_new_from_syspath(udev, sysfs_path);
		struct udev_list_entry *prop_list = udev_device_get_properties_list_entry(dev);
		struct udev_list_entry *dev_path = udev_list_entry_get_by_name(prop_list, "DEVNAME");
		struct udev_list_entry *dev_name = udev_list_entry_get_by_name(prop_list, "ID_MODEL");

		oplhw_devlist_add(list, "retrowave:", udev_list_entry_get_value(dev_path), dev_name?udev_list_entry_get_value(dev_name):"NONE");
		entry = udev_list_entry_get_next(entry);
	}

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);
}
