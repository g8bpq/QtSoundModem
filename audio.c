

// 
//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2011, 2012, 2013, 2014, 2015  John Langner, WB2OSZ
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// I've extracted the OSS bits from Direwolf's audio.c for use in QtSoundModem


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <assert.h>

#include <errno.h>
#ifdef __OpenBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif

void Debugprintf(const char * format, ...);

extern int Closing;

int oss_fd = -1;	/* Single device, both directions. */

int insize = 0;
short rxbuffer[2400];		// 1200 stereo samples

int num_channels = 2;		/* Should be 1 for mono or 2 for stereo. */
int samples_per_sec = 12000;	/* Audio sampling rate.  Typically 11025, 22050, or 44100. */
int bits_per_sample = 16;	/* 8 (unsigned char) or 16 (signed short). */

// Originally 40.  Version 1.2, try 10 for lower latency.

#define ONE_BUF_TIME 10

static int set_oss_params(int fd);

#define roundup1k(n) (((n) + 0x3ff) & ~0x3ff)

static int calcbufsize(int rate, int chans, int bits)
{
	int size1 = (rate * chans * bits / 8 * ONE_BUF_TIME) / 1000;
	int size2 = roundup1k(size1);
#if DEBUG
	text_color_set(DW_COLOR_DEBUG);
	printf("audio_open: calcbufsize (rate=%d, chans=%d, bits=%d) calc size=%d, round up to %d\n",
		rate, chans, bits, size1, size2);
#endif
	return (size2);
}


int oss_audio_open(char * adevice_in, char * adevice_out)
{
	char audio_in_name[30];
	char audio_out_name[30];

	strcpy(audio_in_name, adevice_in);
	strcpy(audio_out_name, adevice_out);

	if (strcmp(audio_in_name, audio_out_name) == 0)
	{
		printf("Audio device for both receive and transmit: %s \n", audio_in_name);
	}
	else
	{
		printf("Audio input device for receive: %s\n", audio_in_name);
		printf("Audio out device for transmit: %s\n", audio_out_name);
	}

	oss_fd = open(audio_in_name, O_RDWR);

	if (oss_fd < 0)
	{
		printf("Could not open audio device %s\n", audio_in_name);
		return 0;
	}
	else
		printf("OSS fd = %d\n", oss_fd);

	return set_oss_params(oss_fd);
}


static int set_oss_params(int fd)
{
	int err;
	int devcaps;
	int asked_for;
	int ossbuf_size_in_bytes;
	int frag = (5 << 16) | (11);

	err = ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag);

	if (err == -1)
	{
		perror("Not able to set fragment size");
		//		ossbuf_size_in_bytes = 2048;	/* pick something reasonable */
	}

	err = ioctl(fd, SNDCTL_DSP_CHANNELS, &num_channels);
	if (err == -1)
	{
		perror("Not able to set audio device number of channels");
		return (0);
	}

	asked_for = samples_per_sec;

	err = ioctl(fd, SNDCTL_DSP_SPEED, &samples_per_sec);
	if (err == -1)
	{

		perror("Not able to set audio device sample rate");
		return (0);
	}

	printf("Asked for %d samples/sec but actually using %d.\n", asked_for, samples_per_sec);


	/* This is actually a bit mask but it happens that */
	/* 0x8 is unsigned 8 bit samples and */
	/* 0x10 is signed 16 bit little endian. */

	err = ioctl(fd, SNDCTL_DSP_SETFMT, &bits_per_sample);

	if (err == -1)
	{
		perror("Not able to set audio device sample size");
		return (0);
	}

	/*
	 * Determine capabilities.
	 */
	err = ioctl(fd, SNDCTL_DSP_GETCAPS, &devcaps);
	if (err == -1)
	{
		perror("Not able to get audio device capabilities");
		// Is this fatal? //	return (-1);
	}



	printf("audio_open(): devcaps = %08x\n", devcaps);
	if (devcaps & DSP_CAP_DUPLEX) printf("Full duplex record/playback.\n");
	if (devcaps & DSP_CAP_BATCH) printf("Device has some kind of internal buffers which may cause delays.\n");
	if (devcaps & ~(DSP_CAP_DUPLEX | DSP_CAP_BATCH)) printf("Others...\n");

	if (!(devcaps & DSP_CAP_DUPLEX))
	{
		printf("Audio device does not support full duplex\n");
		// Do we care? //	return (-1);
	}

	err = ioctl(fd, SNDCTL_DSP_SETDUPLEX, NULL);
	if (err == -1)
	{
		perror("Not able to set audio full duplex mode");
		// Unfortunate but not a disaster.
	}

	/*
	 * Get preferred block size.
	 * Presumably this will provide the most efficient transfer.
	 *
	 * In my particular situation, this turned out to be
	 *  	2816 for 11025 Hz 16 bit mono
	 *	5568 for 11025 Hz 16 bit stereo
	 *     11072 for 44100 Hz 16 bit mono
	 *
	 * This was long ago under different conditions.
	 * Should study this again some day.
	 *
	 * Your milage may vary.
	 */


	err = ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &ossbuf_size_in_bytes);
	if (err == -1)
	{
		perror("Not able to get audio block size");
		ossbuf_size_in_bytes = 2048;	/* pick something reasonable */
	}

	printf("audio_open(): suggestd block size is %d\n", ossbuf_size_in_bytes);

	/*
	 * That's 1/8 of a second which seems rather long if we want to
	 * respond quickly.
	 */

	ossbuf_size_in_bytes = calcbufsize(samples_per_sec, num_channels, bits_per_sample);

	printf("audio_open(): using block size of %d\n", ossbuf_size_in_bytes);

	/* Version 1.3 - after a report of this situation for Mac OSX version. */
	if (ossbuf_size_in_bytes < 256 || ossbuf_size_in_bytes > 32768)
	{
		printf("Audio buffer has unexpected extreme size of %d bytes.\n", ossbuf_size_in_bytes);
		printf("Detected at %s, line %d.\n", __FILE__, __LINE__);
		printf("This might be caused by unusual audio device configuration values.\n");
		ossbuf_size_in_bytes = 2048;
		printf("Using %d to attempt recovery.\n", ossbuf_size_in_bytes);
	}
	return (ossbuf_size_in_bytes);
}





int oss_read(short * samples, int nSamples)
{
	int n;
	int nBytes = nSamples * 4;

	if (oss_fd < 0)
		return 0;

	//	printf("audio_get(): read %d\n", nBytes - insize);

	n = read(oss_fd, &rxbuffer[insize], nBytes - insize);

	if (n < 0)
	{
		perror("Can't read from audio device");
		insize = 0;

		return (0);
	}

	insize += n;

	if (n == nSamples * 4)
	{
		memcpy(samples, rxbuffer, insize);
		insize = 0;
		return nSamples;
	}

	return 0;
}


int oss_write(short * ptr, int len)
{
	int k;

//	int delay;
//	ioctl(oss_fd, SNDCTL_DSP_GETODELAY, &delay);
//	Debugprintf("Delay %d", delay);

	k = write(oss_fd, ptr, len * 4);

//
	if (k < 0)
	{
		perror("Can't write to audio device");
		return (-1);
	}
	if (k < len * 4)
	{
		printf("oss_write(): write %d returns %d\n", len * 4, k);
		/* presumably full but didn't block. */
		usleep(10000);
	}
	ptr += k;
	len -= k;

	return 0;
} 

void oss_flush()
{
	int delay;

	if (oss_fd < 0)
	{
		Debugprintf("OSS Flush Called when OSS closed");
			return;
	}

	ioctl(oss_fd, SNDCTL_DSP_GETODELAY, &delay);
	Debugprintf("OSS Flush Delay %d", delay);
	
	while (delay)
	{
		Sleep(10);
		ioctl(oss_fd, SNDCTL_DSP_GETODELAY, &delay);
//		Debugprintf("Flush Delay %d", delay);
	}
}

void oss_audio_close(void)
{
	if (oss_fd > 0)
	{
		close(oss_fd);
		oss_fd = -1;
	}
	return;
}

