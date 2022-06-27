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

#ifndef OPLHW_INTERNAL_H
#define OPLHW_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct oplhw_device
{
	bool isOPL3;
	void (*close)(struct oplhw_device *dev);
	void (*write)(struct oplhw_device *dev, uint16_t reg, uint8_t val);
} oplhw_device;

oplhw_device *oplhw_retrowave_OpenDevice(const char *dev_name);
oplhw_device *oplhw_ioport_OpenDevice(const char *dev_name);
oplhw_device *oplhw_lpt_OpenDevice(const char *dev_name);
oplhw_device *oplhw_alsa_OpenDevice(const char *dev_name);

#endif
