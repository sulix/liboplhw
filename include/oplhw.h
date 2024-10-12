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

#ifndef OPLHW_H
#define OPLHW_H

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#define OPLHW_API __declspec(dllexport)
#else
#if __GNUC__ >= 4
#define OPLHW_API __attribute__((visibility("default")))
#else
#define OPLHW_API
#endif
#endif

typedef struct oplhw_device oplhw_device;

#ifdef __cplusplus
extern "C" {
#endif

/* Core API */
OPLHW_API oplhw_device *oplhw_OpenDevice(const char *dev_name);
OPLHW_API void oplhw_CloseDevice(oplhw_device *dev);
OPLHW_API void oplhw_Write(oplhw_device *dev, uint16_t reg, uint8_t val);
OPLHW_API bool oplhw_IsOPL3(oplhw_device *dev);
OPLHW_API void oplhw_Reset(oplhw_device *dev);

/* Filters */

/* Create a volume filter device. */
OPLHW_API oplhw_device *oplhw_CreateVolumeFilter(oplhw_device *backing_dev);
/* Set the volume. The device must be a volume filter device. */
OPLHW_API int oplhw_SetVolume(oplhw_device *volume_dev, int volume);

#ifdef __cplusplus
}
#endif 

#endif
