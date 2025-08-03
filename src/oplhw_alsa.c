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

#include <alsa/asoundlib.h>
#include <sound/asound_fm.h>
#include <sys/ioctl.h>
#include <unistd.h>


typedef struct oplhw_alsa_device
{
	oplhw_device dev;
	snd_hwdep_t *oplHwDep;
	bool opl3Enabled;
	struct snd_dm_fm_voice oplOperators[35];
	struct snd_dm_fm_note oplChannels[18];
	struct snd_dm_fm_params oplParams;
} oplhw_alsa_device;

const int regToOper[0x20] =
	{0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1,
		12, 13, 14, 15, 16, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1};

void oplhw_alsa_Write(oplhw_device *dev, uint16_t reg, uint8_t val)
{
	oplhw_alsa_device *alsa_dev = (oplhw_alsa_device *)dev;
	bool paramsDirty = false;
	if (reg == 0x08)
	{
		alsa_dev->oplParams.kbd_split = (val >> 6) & 1;
		paramsDirty = true;
	}
	else if (reg == 0xBD)
	{
		/* Perussion / Params */
		alsa_dev->oplParams.hihat = (val)&1;
		alsa_dev->oplParams.cymbal = (val >> 1) & 1;
		alsa_dev->oplParams.tomtom = (val >> 2) & 1;
		alsa_dev->oplParams.snare = (val >> 3) & 1;
		alsa_dev->oplParams.bass = (val >> 4) & 1;
		alsa_dev->oplParams.rhythm = (val >> 5) & 1;
		alsa_dev->oplParams.vib_depth = (val >> 6) & 1;
		alsa_dev->oplParams.am_depth = (val >> 7) & 1;
		paramsDirty = true;
	}
	else if ((reg & 0xf0) == 0xa0)
	{
		/* Channel Freq (low 8 bits) */
		int channel = (reg & 0xf) + ((alsa_dev->opl3Enabled && (reg & 0x100)) ? 8 : 0);
		alsa_dev->oplChannels[channel].fnum = (alsa_dev->oplChannels[channel].fnum & 0x300) | (val & 0xff);
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_PLAY_NOTE, (void *)&alsa_dev->oplChannels[channel]);
	}
	else if ((reg & 0xf0) == 0xb0)
	{
		/* Channel freq (high 3 bits) */
		int channel = (reg & 0xf) + ((alsa_dev->opl3Enabled && (reg & 0x100)) ? 8 : 0);
		alsa_dev->oplChannels[channel].fnum = (alsa_dev->oplChannels[channel].fnum & 0xff) | ((val << 8) & 0x300);
		alsa_dev->oplChannels[channel].octave = (val >> 2) & 7;
		alsa_dev->oplChannels[channel].key_on = (val >> 5) & 1;
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_PLAY_NOTE, (void *)&alsa_dev->oplChannels[channel]);
	}
	else if ((reg & 0xf0) == 0xc0)
	{
		int operTbl[] = {0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20, 24, 25, 26, 30, 31, 32};
		int channel = (reg & 0xf) + ((alsa_dev->opl3Enabled && (reg & 0x100)) ? 8 : 0);
		int oper = operTbl[channel];
		if (oper >= ((alsa_dev->opl3Enabled) ? 35 : 18))
			return;
		alsa_dev->oplOperators[oper].connection = (val)&1;
		alsa_dev->oplOperators[oper + 3].connection = (val)&1;
		alsa_dev->oplOperators[oper].feedback = (val >> 1) & 7;
		alsa_dev->oplOperators[oper + 3].feedback = (val >> 1) & 7;
		if (alsa_dev->opl3Enabled)
		{
			alsa_dev->oplOperators[oper].left = (val & 16) ? 1 : 0;
			alsa_dev->oplOperators[oper].right = (val & 32) ? 1 : 0;
			alsa_dev->oplOperators[oper + 3].left = (val & 16) ? 1 : 0;
			alsa_dev->oplOperators[oper + 3].right = (val & 32) ? 1 : 0;
		}
		else
		{
			alsa_dev->oplOperators[oper].left = 1;
			alsa_dev->oplOperators[oper].right = 1;
			alsa_dev->oplOperators[oper + 3].left = 1;
			alsa_dev->oplOperators[oper + 3].right = 1;
		}
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x20)
	{
		int oper = regToOper[reg & 0x1f];
		if (alsa_dev->opl3Enabled && (reg & 0x100)) oper += 17;
		if (oper == -1)
			return;
		alsa_dev->oplOperators[oper].harmonic = val & 0xf;
		alsa_dev->oplOperators[oper].kbd_scale = (val >> 4) & 1;
		alsa_dev->oplOperators[oper].do_sustain = (val >> 5) & 1;
		alsa_dev->oplOperators[oper].vibrato = (val >> 6) & 1;
		alsa_dev->oplOperators[oper].am = (val >> 7) & 1;
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x40)
	{
		int oper = regToOper[reg & 0x1f];
		if (alsa_dev->opl3Enabled && (reg & 0x100)) oper += 17;
		if (oper == -1)
			return;
		alsa_dev->oplOperators[oper].volume = ~val & 0x3f;
		alsa_dev->oplOperators[oper].scale_level = (val >> 6) & 3;
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x60)
	{
		int oper = regToOper[reg & 0x1f];
		if (alsa_dev->opl3Enabled && (reg & 0x100)) oper += 17;
		if (oper == -1)
			return;
		alsa_dev->oplOperators[oper].decay = val & 0xf;
		alsa_dev->oplOperators[oper].attack = (val >> 4) & 0xf;
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x80)
	{
		int oper = regToOper[reg & 0x1f];
		if (alsa_dev->opl3Enabled && (reg & 0x100)) oper += 17;
		if (oper == -1)
			return;
		alsa_dev->oplOperators[oper].release = val & 0xf;
		alsa_dev->oplOperators[oper].sustain = (val >> 4) & 0xf;
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0xe0)
	{
		int oper = regToOper[reg & 0x1f];
		if (alsa_dev->opl3Enabled && (reg & 0x100)) oper += 17;
		if (oper == -1)
			return;
		alsa_dev->oplOperators[oper].waveform = val & (alsa_dev->opl3Enabled ? 0x7 : 0x3);
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&alsa_dev->oplOperators[oper]);
	}
	else if (reg == 0x104)
	{
		if (alsa_dev->opl3Enabled)
		{
			snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_CONNECTION, (void *)(uintptr_t)val);
		}
		else
		{	
			snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_CONNECTION, (void *)0);
		}
	}
	else if (reg == 0x105)
	{
		printf("Setting opl3 mode to %d", val);
		alsa_dev->opl3Enabled = (val & 1) ? true : false;
		//void *mode = (void *)(uintptr_t)((val & 1) ? SNDRV_DM_FM_MODE_OPL3 : SNDRV_DM_FM_MODE_OPL2);
		//snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_MODE, mode);
	}
	else
	{
		/* Unsupported register write. */
		/* TODO: Report this as an error somehow. */
	}

	if (paramsDirty)
		snd_hwdep_ioctl(alsa_dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_PARAMS, (void *)&alsa_dev->oplParams);
}

/* Find an OPL2 hwdep device to use as the default. */
static const char *findHwDep()
{
	void **hints, **current_hint;
	const char *hwdep_name = NULL;
	int err = snd_device_name_hint(-1, "hwdep", &hints);
	if (err) {
		return NULL;
	}

	for (current_hint = hints; *current_hint; current_hint++)
	{
		const char *name = snd_device_name_get_hint(*current_hint, "NAME");
		const char *desc = snd_device_name_get_hint(*current_hint, "DESC");
		if (strstr(desc, "OPL3") || strstr(desc, "OPL2"))
		{
			hwdep_name = strdup(name);
			break;
		}
	}

	snd_device_name_free_hint(hints);
	return hwdep_name;
}

static void setupStructs(oplhw_alsa_device *dev)
{
	int i;
	
	memset(&dev->oplParams, 0, sizeof(dev->oplParams));
	memset(dev->oplOperators, 0, sizeof(dev->oplOperators));
	memset(dev->oplChannels, 0, sizeof(dev->oplChannels));

	for (i = 0; i < 35; ++i)
	{
		dev->oplOperators[i].op = (i / 3) % 2;
		dev->oplOperators[i].voice = (i / 6) * 3 + i % 3;
		dev->oplOperators[i].left = 1;
		dev->oplOperators[i].right = 1;
	}

	for (i = 0; i < 18; ++i)
	{
		dev->oplChannels[i].voice = i;
	}
}

void oplhw_alsa_CloseDevice(oplhw_device *dev)
{
	oplhw_alsa_device *alsa_dev = (oplhw_alsa_device *)dev;
	snd_hwdep_close(alsa_dev->oplHwDep);
	free(alsa_dev);
}

oplhw_device *oplhw_alsa_OpenDevice(const char *dev_name)
{
	bool should_free_name = false;
	oplhw_alsa_device *dev = calloc(1, sizeof(*dev));

	dev->dev.close = &oplhw_alsa_CloseDevice;
	dev->dev.write = &oplhw_alsa_Write;

	/* If we don't have a dev_name, attempt to find one. */
	if (!dev_name || !dev_name[0])
	{
		dev_name = findHwDep();
		if (dev_name)
			should_free_name = true;
		else
			return NULL;
	}

	if (snd_hwdep_open(&dev->oplHwDep, dev_name, SND_HWDEP_OPEN_WRITE) < 0)
	{
		if (should_free_name)
			free((char*)dev_name);
		/* TODO: Report errors properly. */
		free(dev);
		return NULL;
	}
	
	if (should_free_name)
		free((char*)dev_name);

	snd_hwdep_info_t *info;
	snd_hwdep_info_alloca(&info);

	if (snd_hwdep_info(dev->oplHwDep, info))
	{
		/* TODO: Report errors properly. */
		free(dev);
		return NULL;
	}

	int interface = snd_hwdep_info_get_iface(info);

	if (interface == SND_HWDEP_IFACE_OPL2)
	{
		dev->dev.isOPL3 = false;
		snd_hwdep_ioctl(dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_MODE, (void *)SNDRV_DM_FM_MODE_OPL2);
	}
	else if (interface == SND_HWDEP_IFACE_OPL3)
	{
		dev->dev.isOPL3 = true;
		snd_hwdep_ioctl(dev->oplHwDep, SNDRV_DM_FM_IOCTL_SET_MODE, (void *)SNDRV_DM_FM_MODE_OPL3);
	}

	/* We start off with OPL3 disabled. */
	dev->opl3Enabled = false;

	setupStructs(dev);

	return (oplhw_device *)dev;
}
