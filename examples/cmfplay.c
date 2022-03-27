/*
 * oplhw: ALSA hwdep-based library for OPL2-based soundcards.
 *
 * Copyright (C) 2022 by David Gow <david@davidgow.net>
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
#include <stdlib.h>
#include <unistd.h>

#include <math.h>

#include "oplhw.h"

struct oplhw_device *oplDevice;

typedef struct CMFHeader
{
	uint32_t signature;
	uint8_t verMinor;
	uint8_t verMajor;
	uint16_t offInstruments;
	uint16_t offMusic;
	uint16_t ticksPerCrotchet;
	uint16_t ticksPerSecond;
	uint16_t offTitle;
	uint16_t offComposer;
	uint16_t offRemarks;
	uint8_t channelUsed[16];
	uint8_t numInstruments;
	/* This could be 16-bit, but never is? */
} CMFHeader;

CMFHeader cmfHeader;

typedef struct CMFInstrument
{
	uint8_t modCharacteristic;
	uint8_t carrCharacteristic;
	uint8_t modScaleVol;
	uint8_t carrScaleVol;
	uint8_t modAttackDecay;
	uint8_t carrAttackDecay;
	uint8_t modSustainRelease;
	uint8_t carrSustainRelease;
	uint8_t modWaveSelect;
	uint8_t carrWaveSelect;
	uint8_t feedback;
	uint8_t padding[5];
} CMFInstrument;

CMFInstrument *cmfInstruments;

#define OPLOFFSET(channel)   (((channel) / 3) * 8 + ((channel) % 3))

void SetInstrumentChannel(CMFInstrument *inst, int channel)
{
	/* Disable the note before changing instrument. */
	int operTbl[] = {0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20, 24, 25, 26, 30, 31, 32};
	int modReg = OPLOFFSET(channel);
	int modReg2 = operTbl[channel];
	printf ("inst:chan = %d, mod reg = %x (%x)\n" , channel, modReg, modReg2);
	oplhw_Write(oplDevice, 0x20+modReg, inst->modCharacteristic);
	oplhw_Write(oplDevice, 0x23+modReg, inst->carrCharacteristic);
	oplhw_Write(oplDevice, 0x40+modReg, inst->modScaleVol);
	oplhw_Write(oplDevice, 0x43+modReg, inst->carrScaleVol);
	oplhw_Write(oplDevice, 0x60+modReg, inst->modAttackDecay);
	oplhw_Write(oplDevice, 0x63+modReg, inst->carrAttackDecay);
	oplhw_Write(oplDevice, 0x80+modReg, inst->modSustainRelease);
	oplhw_Write(oplDevice, 0x83+modReg, inst->carrSustainRelease);
	oplhw_Write(oplDevice, 0xE0+modReg, inst->modWaveSelect);
	oplhw_Write(oplDevice, 0xE3+modReg, inst->carrWaveSelect);
	oplhw_Write(oplDevice, 0xC0+channel, inst->feedback | 0xF0);
}

float MIDINoteToAdlib(int note, int block)
{
	/* A below middle C (not 69) is a 440 Hz */
	float freq = 440.f * pow(2, (note - 69.f) / 12.f);

	float fnum = freq * pow(2, (20 - block)) / 49716.f;
	printf("freq = %f, f-num = %f\n", freq, fnum);

	return fnum;
}

void ReadHeaders(FILE *f)
{
	fread(&cmfHeader, sizeof(cmfHeader), 1, f);

	/* CTMF */
	if (cmfHeader.signature != 0x464d5443)
	{
		fprintf(stderr, "Not a valid CMF file!");
		exit(1);
	}

	float tempo = 60.f * cmfHeader.ticksPerSecond / cmfHeader.ticksPerCrotchet;

	printf("CMF v%d.%d\n", cmfHeader.verMajor, cmfHeader.verMinor);
	printf("Tempo = %f (%d ticks/beat, %d ticks/sec)\n", tempo, cmfHeader.ticksPerCrotchet, cmfHeader.ticksPerSecond);
	printf("Num instruments: %d\n", cmfHeader.numInstruments);

	// TODO: Read metadata.

	cmfInstruments = malloc(sizeof(CMFInstrument) * cmfHeader.numInstruments);
	fseek(f, cmfHeader.offInstruments, SEEK_SET);
	fread(cmfInstruments, cmfHeader.numInstruments, sizeof(CMFInstrument), f);

	for (int i = 0; i < 9; ++i)
	{
		SetInstrumentChannel(&cmfInstruments[i], i);
	}

	fseek(f, cmfHeader.offMusic, SEEK_SET);	
}

uint32_t ReadMIDILength(FILE *f)
{
	uint32_t val = 0;
	do
	{
		uint8_t c = fgetc(f);
		val |= c & 0x7F;

		if (c & 0x80) val <<= 7;
		else break;
	} while (true);
	return val;
}

bool noinst = false;

bool DoMIDIEvent(FILE *f)
{
	uint32_t waitTicks = ReadMIDILength(f);
	usleep(waitTicks * 1000000.f / cmfHeader.ticksPerSecond);
	static uint8_t eventType = 0;
	uint8_t statusByte = fgetc(f);
	if ((statusByte & 0x80)) eventType = statusByte;
	else fseek(f, -1, SEEK_CUR);
	int channel = (eventType & 0xF);
	printf("Event: delay = %d, type = $%x\n", waitTicks, eventType);
	switch (eventType & 0xF0)
	{
	case 0x80:
	{
		uint8_t noteNum = fgetc(f);
		uint8_t velocity = fgetc(f);
		printf("Note off: channel %d, note %d, vel %d\n", channel, noteNum, velocity);
		int block = (noteNum / 12) - 1;
		float fnum = MIDINoteToAdlib(noteNum, block);
		uint8_t f_lo = (int)fnum & 0xFF;
		uint8_t f_hi = ((int)fnum >> 8) & 0x3;
		f_hi |= (block << 2);
		f_hi &= 0x1F; // Note OFF

		oplhw_Write(oplDevice, 0xA0 | channel, f_lo);
		oplhw_Write(oplDevice, 0xB0 | channel, f_hi);
		
	} break;
	case 0x90:
	{
		uint8_t noteNum = fgetc(f);
		uint8_t velocity = fgetc(f);
		printf("Note on: channel %d, note %d, vel %d\n", channel, noteNum, velocity);
		int block = ((noteNum / 12));
		if (block > 1) block--;
		float fnum = MIDINoteToAdlib(noteNum, block);
		printf("fNum = %f, block = %d\n", fnum, block);
		uint8_t f_lo = (int)fnum & 0xFF;
		uint8_t f_hi = ((int)fnum >> 8) & 0x3;
		f_hi |= (block << 2);
		if (velocity)
			f_hi |= 0x20; // Note ON

		int vel = 0x2F - (velocity * 0x2F / 127);
		oplhw_Write(oplDevice, 0xA0 + channel, f_lo);
		oplhw_Write(oplDevice, 0xB0 + channel, f_hi);
		//oplhw_Write(oplDevice, 0x43 + OPLOFFSET(channel), 0x00);
	} break;
	case 0xB0:
	{
		uint8_t controller = fgetc(f);
		uint8_t value = fgetc(f);

		printf("Controller %x -> %x\n", controller, value);
	} break;
	case 0xC0:
	{
		uint8_t inst = fgetc(f);
		printf("Instrument change: %d -> %d\n", channel, inst);
		SetInstrumentChannel(&cmfInstruments[inst], channel);
	} break;
	case 0xF0:
	{
		if (eventType == 0xFF)
		{
			uint8_t metaEvent = fgetc(f);
			printf("Meta Event %x\n", metaEvent);
			/* Check for end of track first, due to bothersome invalid files */
			if (metaEvent == 0x2F)
				return true;
			uint32_t length = ReadMIDILength(f);
		}
		else
		{
			printf("SysEx message %x\n", eventType);
		}
	} break;
	default:
		printf("Unknown event type %x\n", eventType);
		//oplhw_CloseDevice(oplDevice);
		return true;
	}
	return false;
}

int main(int argc, char **argv)
{
	const char *filename = argv[2];
	const char *devname = argv[1];

	printf("%s: A simple CMF player using liboplhw.\n", argv[0]);
	printf("(C) 2022 David Gow\n");
	printf("\n");

	if (argc < 2)
	{
		printf("Usage: %s [device] [filename]\n", argv[0]);
		printf("\tdevice: The ALSA hwdep OPL2 device.\n");
		printf("\t\tThis is usually something like \"hw:0,0\"\n");
		printf("\t\tTry looking in /proc/asound/hwdep.\n");
		printf("\tfilename: The IMF file to play.\n");
		return -1;
	}

	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		perror("fopen");
		return -2;
	}

	oplDevice = oplhw_OpenDevice(devname);
	if (!oplDevice)
	{
		fprintf(stderr, "Couldn't open OPL2 device \"%s\"\n", devname);
		return -3;
	}

	for (int i = 0; i < 255; ++i)
		oplhw_Write(oplDevice, i, 0);
	oplhw_Write(oplDevice, 0x01, 0x20);
	oplhw_Write(oplDevice, 0x05, 0x00);
	oplhw_Write(oplDevice, 0x08, 0x00);
	oplhw_Write(oplDevice, 0xBD, 0xC0);

	ReadHeaders(f);
	while (!DoMIDIEvent(f));

	/*for (int i = 0; i < 255; ++i)
		oplhw_Write(oplDevice, i, 0);*/

	oplhw_CloseDevice(oplDevice);

}
