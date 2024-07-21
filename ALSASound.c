/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundModem

QtSoundModem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundModem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundModem.  If not, see http://www.gnu.org/licenses

*/

//#define TXSILENCE

// UZ7HO Soundmodem Port by John Wiseman G8BPQ
//
//	Audio interface Routine

//	Passes audio samples to/from the sound interface

//	As this is platform specific it also has the main() routine, which does
//	platform specific initialisation before calling ardopmain()

//	This is ALSASound.c for Linux
//	Windows Version is Waveout.c


#include <alsa/asoundlib.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "UZ7HOStuff.h"

#define VOID void

char * strlop(char * buf, char delim);

int gethints();

struct timespec pttclk;

extern int Closing;

int SoundMode = 0;
int stdinMode = 0;
int onlyMixSnoop = 0;

int txLatency;

//#define SHARECAPTURE		// if defined capture device is opened and closed for each transission

#define HANDLE int

void gpioSetMode(unsigned gpio, unsigned mode);
void gpioWrite(unsigned gpio, unsigned level);
int WriteLog(char * msg, int Log);
int _memicmp(unsigned char *a, unsigned char *b, int n);
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2);
int gpioInitialise(void);
HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits);
int CloseSoundCard();
int PackSamplesAndSend(short * input, int nSamples);
void displayLevel(int max);
BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite);
VOID processargs(int argc, char * argv[]);
void PollReceivedSamples();


HANDLE OpenCOMPort(char * Port, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits);
VOID COMSetDTR(HANDLE fd);
VOID COMClearDTR(HANDLE fd);
VOID COMSetRTS(HANDLE fd);
VOID COMClearRTS(HANDLE fd);

int oss_read(short * samples, int nSamples);
int oss_write(short * ptr, int len);
int oss_flush();
int oss_audio_open(char * adevice_in, char * adevice_out);
void oss_audio_close();

int listpulse();
int pulse_read(short * ptr, int len);
int pulse_write(short * ptr, int len);
int pulse_flush();
int pulse_audio_open(char * adevice_in, char * adevice_out);
void pulse_audio_close();


int initdisplay();

extern BOOL blnDISCRepeating;
extern BOOL UseKISS;			// Enable Packet (KISS) interface

extern short * DMABuffer;

#define MaxReceiveSize 2048		// Enough for 9600
#define MaxSendSize 4096

short buffer[2][MaxSendSize * 2];		// Two Transfer/DMA buffers of 0.1 Sec  (x2 for Stereo)
short inbuffer[MaxReceiveSize * 2];	// Input Transfer/ buffers of 0.1 Sec (x2 for Stereo)


extern short * DMABuffer;
extern int Number;

int ReceiveSize = 512;
int SendSize = 1024;
int using48000 = 0;

int SampleRate = 12000;


BOOL UseLeft = TRUE;
BOOL UseRight = TRUE;
char LogDir[256] = "";

void WriteDebugLog(char * Msg);

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	WriteDebugLog(Mess);

	return;
}


void Sleep(int mS)
{
	usleep(mS * 1000);
	return;
}


// Windows and ALSA work with signed samples +- 32767
// STM32 and Teensy DAC uses unsigned 0 - 4095


BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "plughw:0,0";
char PlaybackDevice[80] = "plughw:0,0";

char * CaptureDevices = CaptureDevice;
char * PlaybackDevices = CaptureDevice;

int CaptureIndex = 0;
int PlayBackIndex = 0;

int Ticks;

int LastNow;

extern int Number;				// Number waiting to be sent

snd_pcm_sframes_t MaxAvail;

#include <stdarg.h>

FILE *logfile[3] = {NULL, NULL, NULL};
char LogName[3][256] = {"ARDOPDebug", "ARDOPException", "ARDOPSession"};

#define DEBUGLOG 0
#define EXCEPTLOG 1
#define SESSIONLOG 2

FILE *statslogfile = NULL;

void printtick(char * msg)
{
	Debugprintf("%s %i", msg, Now - LastNow);
	LastNow = Now;
}

struct timespec time_start;

unsigned int getTicks()
{	
	struct timespec tp;
	
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (tp.tv_sec - time_start.tv_sec) * 1000 + (tp.tv_nsec - time_start.tv_nsec) / 1000000;
}

void PlatformSleep(int mS)
{
	Sleep(mS);
}

// PTT via GPIO code

#ifdef __ARM_ARCH

#define PI_INPUT  0
#define PI_OUTPUT 1
#define PI_ALT0   4
#define PI_ALT1   5
#define PI_ALT2   6
#define PI_ALT3   7
#define PI_ALT4   3
#define PI_ALT5   2

// Set GPIO pin as output and set low

void SetupGPIOPTT()
{
	if (pttGPIOPin == -1)
	{
		Debugprintf("GPIO PTT disabled"); 
		useGPIO = FALSE;
	}
	else
	{
		if (pttGPIOPin < 0) {
			pttGPIOInvert = TRUE;
			pttGPIOPin = -pttGPIOPin;
		}

		gpioSetMode(pttGPIOPin, PI_OUTPUT);
		gpioWrite(pttGPIOPin, pttGPIOInvert ? 1 : 0);
		Debugprintf("Using GPIO pin %d for Left/Mono PTT", pttGPIOPin); 

		if (pttGPIOPinR != -1)
		{
			gpioSetMode(pttGPIOPinR, PI_OUTPUT);
			gpioWrite(pttGPIOPinR, pttGPIOInvert ? 1 : 0);
			Debugprintf("Using GPIO pin %d for Right PTT", pttGPIOPin);
		}

		useGPIO = TRUE;
	}
}
#endif


static void sigterm_handler(int n)
{
	UNUSED(n);

	printf("terminating on SIGTERM\n");
	Closing = TRUE;
}

static void sigint_handler(int n)
{
	UNUSED(n);

	printf("terminating on SIGINT\n");
	Closing = TRUE;
}

char * PortString = NULL;


void platformInit()
{
	struct sigaction act;

//	Sleep(1000);	// Give LinBPQ time to complete init if exec'ed by linbpq

	// Get Time Reference
		
	clock_gettime(CLOCK_MONOTONIC, &time_start);
	LastNow = getTicks();

	// Trap signals

	memset (&act, '\0', sizeof(act));
 
	act.sa_handler = &sigint_handler; 
	if (sigaction(SIGINT, &act, NULL) < 0) 
		perror ("SIGINT");

	act.sa_handler = &sigterm_handler; 
	if (sigaction(SIGTERM, &act, NULL) < 0) 
		perror ("SIGTERM");

	act.sa_handler = SIG_IGN; 

	if (sigaction(SIGHUP, &act, NULL) < 0) 
		perror ("SIGHUP");

	if (sigaction(SIGPIPE, &act, NULL) < 0) 
		perror ("SIGPIPE");
}

void txSleep(int mS)
{
	// called while waiting for next TX buffer. Run background processes

	if (mS < 0)
		return;

	while (mS > 50)
	{
		PollReceivedSamples();			// discard any received samples

		Sleep(50);
		mS -= 50;
	}

	Sleep(mS);

	PollReceivedSamples();			// discard any received samples
}
 
// ALSA Code 

#define true 1
#define false 0

snd_pcm_t *	playhandle = NULL;
snd_pcm_t *	rechandle = NULL;

int m_playchannels = 2;
int m_recchannels = 2;


char SavedCaptureDevice[256];	// Saved so we can reopen
char SavedPlaybackDevice[256];

int Savedplaychannels = 2;

int SavedCaptureRate;
int SavedPlaybackRate;

char CaptureNames[16][256] = { "" };
char PlaybackNames[16][256] = { "" };

int PlaybackCount = 0;
int CaptureCount = 0;

// Routine to check that library is available

int CheckifLoaded()
{
	// Prevent CTRL/C from closing the TNC
	// (This causes problems if the TNC is started by LinBPQ)

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	return TRUE;
}

int GetOutputDeviceCollection()
{
	// Get all the suitable devices and put in a list for GetNext to return

	snd_ctl_t *handle= NULL;
	snd_pcm_t *pcm= NULL;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_pcm_hw_params_t *pars;
	snd_pcm_format_mask_t *fmask;
	char NameString[256];

	Debugprintf("Playback Devices\n");

	CloseSoundCard();

	// free old struct if called again

//	while (PlaybackCount)
//	{
//		PlaybackCount--;
//		free(PlaybackNames[PlaybackCount]);
//	}

//	if (PlaybackNames)
//		free(PlaybackNames);

	PlaybackCount = 0;

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max, ratemin, ratemax;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	
	card = -1;

	if (snd_card_next(&card) < 0)
	{
		Debugprintf("No Devices");
		return 0;
	}

	if (playhandle)
		snd_pcm_close(playhandle);

	playhandle = NULL;

	while (card >= 0)
	{
		sprintf(hwdev, "hw:%d", card);
		err = snd_ctl_open(&handle, hwdev, 0);
		err = snd_ctl_card_info(handle, info);
    
		Debugprintf("Card %d, ID `%s', name `%s'", card, snd_ctl_card_info_get_id(info),
                snd_ctl_card_info_get_name(info));


		dev = -1;

		if(snd_ctl_pcm_next_device(handle, &dev) < 0)
		{
			// Card has no devices

			snd_ctl_close(handle);
			goto nextcard;      
		}

		while (dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
	
			err = snd_ctl_pcm_info(handle, pcminfo);

			
			if (err == -ENOENT)
				goto nextdevice;

			nsubd = snd_pcm_info_get_subdevices_count(pcminfo);
		
			Debugprintf("  Device hw:%d,%d ID `%s', name `%s', %d subdevices (%d available)",
				card, dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);

			if (err)
			{
				Debugprintf("Error %d opening output device", err);
				goto nextdevice;
			}

			//	Get parameters for this device

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
			
			snd_pcm_hw_params_get_rate_min(pars, &ratemin, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &ratemax, NULL);

			if( min == max )
				if( min == 1 )
					Debugprintf("    1 channel,  sampling rate %u..%u Hz", ratemin, ratemax);
				else
					Debugprintf("    %d channels,  sampling rate %u..%u Hz", min, ratemin, ratemax);
			else
				Debugprintf("    %u..%u channels, sampling rate %u..%u Hz", min, max, ratemin, ratemax);

			// Add device to list

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

			strcpy(PlaybackNames[PlaybackCount++], NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:
			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }
		snd_ctl_close(handle);

nextcard:
			
		Debugprintf("");

		if (snd_card_next(&card) < 0)		// No more cards
			break;
	}

	return PlaybackCount;
}


int GetInputDeviceCollection()
{
	// Get all the suitable devices and put in a list for GetNext to return

	snd_ctl_t *handle= NULL;
	snd_pcm_t *pcm= NULL;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_pcm_hw_params_t *pars;
	snd_pcm_format_mask_t *fmask;
	char NameString[256];

	Debugprintf("Capture Devices\n");

	CaptureCount = 0;

	//	Get Device List  from ALSA
	
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_hw_params_alloca(&pars);
	snd_pcm_format_mask_alloca(&fmask);

	char hwdev[80];
	unsigned min, max, ratemin, ratemax;
	int card, err, dev, nsubd;
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	
	card = -1;

	if(snd_card_next(&card) < 0)
	{
		Debugprintf("No Devices");
		return 0;
	}

	if (rechandle)
		snd_pcm_close(rechandle);

	rechandle = NULL;

	while(card >= 0)
	{
		sprintf(hwdev, "hw:%d", card);
		err = snd_ctl_open(&handle, hwdev, 0);
		err = snd_ctl_card_info(handle, info);
    
		Debugprintf("Card %d, ID `%s', name `%s'", card, snd_ctl_card_info_get_id(info),
                snd_ctl_card_info_get_name(info));

		dev = -1;
			
		if (snd_ctl_pcm_next_device(handle, &dev) < 0)		// No Devicdes
		{
			snd_ctl_close(handle);
			goto nextcard;      
		}

		while(dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			err= snd_ctl_pcm_info(handle, pcminfo);
	
			if (err == -ENOENT)
				goto nextdevice;
	
			nsubd= snd_pcm_info_get_subdevices_count(pcminfo);
			Debugprintf("  Device hw:%d,%d ID `%s', name `%s', %d subdevices (%d available)",
				card, dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo),
				nsubd, snd_pcm_info_get_subdevices_avail(pcminfo));

			sprintf(hwdev, "hw:%d,%d", card, dev);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);
	
			if (err)
			{	
				Debugprintf("Error %d opening input device", err);
				goto nextdevice;
			}

			err = snd_pcm_hw_params_any(pcm, pars);
 
			snd_pcm_hw_params_get_channels_min(pars, &min);
			snd_pcm_hw_params_get_channels_max(pars, &max);
			snd_pcm_hw_params_get_rate_min(pars, &ratemin, NULL);
			snd_pcm_hw_params_get_rate_max(pars, &ratemax, NULL);

			if( min == max )
				if( min == 1 )
					Debugprintf("    1 channel,  sampling rate %u..%u Hz", ratemin, ratemax);
				else
					Debugprintf("    %d channels,  sampling rate %u..%u Hz", min, ratemin, ratemax);
			else
				Debugprintf("    %u..%u channels, sampling rate %u..%u Hz", min, max, ratemin, ratemax);

			sprintf(NameString, "hw:%d,%d %s(%s)", card, dev,
				snd_pcm_info_get_name(pcminfo), snd_ctl_card_info_get_name(info));

//			Debugprintf("%s", NameString);

			strcpy(CaptureNames[CaptureCount++], NameString);

			snd_pcm_close(pcm);
			pcm= NULL;

nextdevice:
			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				break;
	    }
		snd_ctl_close(handle);
nextcard:

		Debugprintf("");
		if (snd_card_next(&card) < 0 )
			break;
	}

	strcpy(CaptureNames[CaptureCount++], "stdin");

	return CaptureCount;
}

int OpenSoundPlayback(char * PlaybackDevice, int m_sampleRate, int channels, int Report)
{
	int err = 0;

	char buf1[256];
	char buf2[256];
	char * ptr;

	if (playhandle)
	{
		snd_pcm_close(playhandle);
		playhandle = NULL;
	}

	strcpy(SavedPlaybackDevice, PlaybackDevice);	// Saved so we can reopen in error recovery
	SavedPlaybackRate = m_sampleRate;

	strcpy(buf2, PlaybackDevice);

	ptr = strchr(buf2, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name


	if (strstr(buf2, "plug") == 0 && strchr(buf2, ':'))
		sprintf(buf1, "plug%s", buf2);
	else
		strcpy(buf1, buf2);
	
	if (Report)
		Debugprintf("Real Device %s", buf1);

	snd_pcm_hw_params_t *hw_params;
	
	if ((err = snd_pcm_open(&playhandle, buf1, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
	{
		Debugprintf("cannot open playback audio device %s (%s)",  buf1, snd_strerror(err));
		return false;
	}
		   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
	{
		Debugprintf("cannot allocate hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (playhandle, hw_params)) < 0) {
		Debugprintf("cannot initialize hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (playhandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		Debugprintf("cannot set playback access type (%s)", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (playhandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		Debugprintf("cannot setplayback  sample format (%s)", snd_strerror(err));
		return false;
	}

	if ((err = snd_pcm_hw_params_set_rate (playhandle, hw_params, m_sampleRate, 0)) < 0) {
		Debugprintf("cannot set playback sample rate (%s)", snd_strerror(err));
		return false;
	}

	// Initial call has channels set to 1. Subequent ones set to what worked last time

	channels = 2;

	if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, channels)) < 0)
	{
		Debugprintf("cannot set play channel count to %d (%s)", channels, snd_strerror(err));
		
		if (channels == 2)
			return false;				// Shouldn't happen as should have worked before
		
		channels = 2;

		if ((err = snd_pcm_hw_params_set_channels (playhandle, hw_params, 2)) < 0)
		{
			Debugprintf("cannot play set channel count to 2 (%s)", snd_strerror(err));
			return false;
		}
	}
	
	if (Report)
		Debugprintf("Play using %d channels", channels);

	if ((err = snd_pcm_hw_params (playhandle, hw_params)) < 0) 
	{
		Debugprintf("cannot set parameters (%s)", snd_strerror(err));
		return false;
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (playhandle)) < 0) 
	{
		Debugprintf("cannot prepare audio interface for use (%s)", snd_strerror(err));
		return false;
	}

	Savedplaychannels = m_playchannels = channels;

	MaxAvail = snd_pcm_avail_update(playhandle);

	if (Report)
		Debugprintf("Playback Buffer Size %d", (int)MaxAvail);

	return true;
}

int OpenSoundCapture(char * CaptureDevice, int m_sampleRate, int Report)
{
	int err = 0;

	char buf1[256];
	char buf2[256];

	char * ptr;
	snd_pcm_hw_params_t *hw_params;

	if (strcmp(CaptureDevice, "stdin") == 0)
	{
		stdinMode = 1;

		Debugprintf("Input from stdin");
		return TRUE;
	}

	if (rechandle)
	{
		snd_pcm_close(rechandle);
		rechandle = NULL;
	}

	strcpy(SavedCaptureDevice, CaptureDevice);	// Saved so we can reopen in error recovery
	SavedCaptureRate = m_sampleRate;

	strcpy(buf2, CaptureDevice);

	ptr = strchr(buf2, ' ');
	if (ptr) *ptr = 0;				// Get Device part of name

	if (strstr(buf2, "plug") == 0 && strchr(buf2, ':'))
		sprintf(buf1, "plug%s", buf2);
	else
		strcpy(buf1, buf2);

	if (Report)
		Debugprintf("Real Device %s", buf1);

	
	if ((err = snd_pcm_open (&rechandle, buf1, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		Debugprintf("cannot open capture audio device %s (%s)",  buf1, snd_strerror(err));
		return false;
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		Debugprintf("cannot allocate capture hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
				 
	if ((err = snd_pcm_hw_params_any (rechandle, hw_params)) < 0) {
		Debugprintf("cannot initialize capture hardware parameter structure (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_access (rechandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		Debugprintf("cannot set capture access type (%s)", snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_hw_params_set_format (rechandle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		Debugprintf("cannot set capture sample format (%s)", snd_strerror(err));
		return false;
	}
	
	if ((err = snd_pcm_hw_params_set_rate (rechandle, hw_params, m_sampleRate, 0)) < 0) {
		Debugprintf("cannot set capture sample rate (%s)", snd_strerror(err));
		return false;
	}

	m_recchannels = 2;
	
	if ((err = snd_pcm_hw_params_set_channels(rechandle, hw_params, m_recchannels)) < 0)
	{
		if (Report)
			Debugprintf("cannot set rec channel count to 2 (%s)", snd_strerror(err));

		m_recchannels = 1;

		if ((err = snd_pcm_hw_params_set_channels(rechandle, hw_params, 1)) < 0)
		{
			Debugprintf("cannot set rec channel count to 1 (%s)", snd_strerror(err));
			return false;
		}
		if (Report)
			Debugprintf("Record channel count set to 1");
	}
	else
		if (Report)
			Debugprintf("Record channel count set to 2");

	/*
	{
	unsigned int val = 0;
	unsigned int dir = 0, frames = 0;

	
	snd_pcm_hw_params_get_channels(rechandle, &val);
	printf("channels = %d\n", val);

	snd_pcm_hw_params_get_rate(rechandle, &val, &dir);
	printf("rate = %d bps\n", val);

	snd_pcm_hw_params_get_period_time(rechandle, &val, &dir);
	printf("period time = %d us\n", val);

	snd_pcm_hw_params_get_period_size(rechandle, &frames, &dir);
	printf("period size = %d frames\n", (int)frames);

	snd_pcm_hw_params_get_buffer_time(rechandle, &val, &dir);
	printf("buffer time = %d us\n", val);

	snd_pcm_hw_params_get_buffer_size(rechandle, (snd_pcm_uframes_t *)&val);
	printf("buffer size = %d frames\n", val);

	snd_pcm_hw_params_get_periods(rechandle, &val, &dir);
	printf("periods per buffer = %d frames\n", val);
	}
	*/

	if ((err = snd_pcm_hw_params (rechandle, hw_params)) < 0)
	{
		// Try setting some more params Have to reinit params

		snd_pcm_hw_params_any(rechandle, hw_params);
		snd_pcm_hw_params_set_access(rechandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
		snd_pcm_hw_params_set_format(rechandle, hw_params, SND_PCM_FORMAT_S16_LE);
		snd_pcm_hw_params_set_rate(rechandle, hw_params, m_sampleRate, 0);
		snd_pcm_hw_params_set_channels(rechandle, hw_params, m_recchannels);

	//	err = snd_pcm_hw_params_set_buffer_size(rechandle, hw_params, 65536);

	//	if (err)
	//		Debugprintf("cannot set buffer size (%s)", snd_strerror(err));

		err = snd_pcm_hw_params_set_period_size(rechandle, hw_params, (snd_pcm_uframes_t) { 1024 }, (int) { 0 });

		if (err)
			Debugprintf("cannot set period size (%s)", snd_strerror(err));

		if ((err = snd_pcm_hw_params(rechandle, hw_params)) < 0)
		{
			Debugprintf("cannot set parameters (%s)", snd_strerror(err));
			return false;
		}
	}
	
	snd_pcm_hw_params_free(hw_params);
	
	if ((err = snd_pcm_prepare (rechandle)) < 0) {
		Debugprintf("cannot prepare audio interface for use (%s)", snd_strerror(err));
		return FALSE;
	}

	if (Report)
		Debugprintf("Capture using %d channels", m_recchannels);

	short buf[256];

	if ((err = snd_pcm_readi(rechandle, buf, 12)) != 12)
	{
		Debugprintf("read from audio interface failed (%s)", snd_strerror(err));
	}


//	Debugprintf("Read got %d", err);

 	return TRUE;
}

int OpenSoundCard(char * CaptureDevice, char * PlaybackDevice, int c_sampleRate, int p_sampleRate, int Report)
{
	int Channels = 1;

	if (Report)
		Debugprintf("Opening Playback Device %s Rate %d", PlaybackDevice, p_sampleRate);

//	if (UseLeft == 0 || UseRight == 0)
	Channels = 2;						// L or R implies stereo

	if (OpenSoundPlayback(PlaybackDevice, p_sampleRate, Channels, Report))
	{
#ifdef SHARECAPTURE

		// Close playback device so it can be shared
		
		if (playhandle)
		{
			snd_pcm_close(playhandle);
			playhandle = NULL;
		}
#endif
		if (Report)
			Debugprintf("Opening Capture Device %s Rate %d", CaptureDevice, c_sampleRate);
		return OpenSoundCapture(CaptureDevice, c_sampleRate, Report);
	}
	else
		return false;
}



int CloseSoundCard()
{
	if (rechandle)
	{
		snd_pcm_close(rechandle);
		rechandle = NULL;
	}

	if (playhandle)
	{
		snd_pcm_close(playhandle);
		playhandle = NULL;
	}
	return 0;
}


int SoundCardWrite(short * input, int nSamples)
{
	unsigned int ret;
	snd_pcm_sframes_t avail; // , maxavail;

	if (playhandle == NULL)
		return 0;

	//	Stop Capture

//	if (rechandle)
//	{
//		snd_pcm_close(rechandle);
//		rechandle = NULL;
//	}

	avail = snd_pcm_avail_update(playhandle);
//	Debugprintf("avail before play returned %d", (int)avail);

	if (avail < 0)
	{
		if (avail != -32)
			Debugprintf("Playback Avail Recovering from %d ..", (int)avail);
		snd_pcm_recover(playhandle, avail, 1);

		avail = snd_pcm_avail_update(playhandle);

		if (avail < 0)
			Debugprintf("avail play after recovery returned %d", (int)avail);
	}
	
//	maxavail = avail;

//	Debugprintf("Tosend %d Avail %d", nSamples, (int)avail);

	while (avail < nSamples || (MaxAvail - avail) > SampleRate)				// Limit to 1 sec of audio
	{
		txSleep(10);
		avail = snd_pcm_avail_update(playhandle);
//		Debugprintf("After Sleep Tosend %d Avail %d", nSamples, (int)avail);
	}

	ret = PackSamplesAndSend(input, nSamples);

	return ret;
}

int PackSamplesAndSend(short * input, int nSamples)
{
	unsigned short samples[256000];
	int ret;

	ret = snd_pcm_writei(playhandle, input, nSamples);

	if (ret < 0)
	{
//		Debugprintf("Write Recovering from %d ..", ret);
		snd_pcm_recover(playhandle, ret, 1);
		ret = snd_pcm_writei(playhandle, samples, nSamples);
//		Debugprintf("Write after recovery returned %d", ret);
	}

	snd_pcm_avail_update(playhandle);
	return ret;

}
/*
int xSoundCardClearInput()
{
	short samples[65536];
	int n;
	int ret;
	int avail;

	if (rechandle == NULL)
		return 0;

	// Clear queue 
	
	avail = snd_pcm_avail_update(rechandle);

	if (avail < 0)
	{
		Debugprintf("Discard Recovering from %d ..", avail);
		if (rechandle)
		{
			snd_pcm_close(rechandle);
			rechandle = NULL;
		}
		OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, NULL);
		avail = snd_pcm_avail_update(rechandle);
	}

	while (avail)
	{
		if (avail > 65536)
			avail = 65536;

			ret = snd_pcm_readi(rechandle, samples, avail);
//			Debugprintf("Discarded %d samples from card", ret);
			avail = snd_pcm_avail_update(rechandle);

//			Debugprintf("Discarding %d samples from card", avail);
	}
	return 0;
}
*/

int SoundCardRead(short * input, int nSamples)
{
	short samples[65536];
	int n;
	int ret;
	int avail;

	if (SoundMode == 1)		// OSS
	{
		ret = oss_read(samples, nSamples);
	}
	else if (SoundMode == 2)// Pulse
	{
		ret = pulse_read(samples, nSamples);
	}
	else
	{
		if (rechandle == NULL)
			return 0;

		avail = snd_pcm_avail_update(rechandle);

		if (avail < 0)
		{
			Debugprintf("avail Recovering from %d ..", avail);
			if (rechandle)
			{
				snd_pcm_close(rechandle);
				rechandle = NULL;
			}

			OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, 0);
			//		snd_pcm_recover(rechandle, avail, 0);
			avail = snd_pcm_avail_update(rechandle);
			Debugprintf("After avail recovery %d ..", avail);
		}

		if (avail < nSamples)
			return 0;

		//	Debugprintf("ALSARead available %d", avail);

		ret = snd_pcm_readi(rechandle, samples, nSamples);

		if (ret < 0)
		{
			Debugprintf("RX Error %d", ret);
			//		snd_pcm_recover(rechandle, avail, 0);
			if (rechandle)
			{
				snd_pcm_close(rechandle);
				rechandle = NULL;
			}

			OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, 0);
			//		snd_pcm_recover(rechandle, avail, 0);
			avail = snd_pcm_avail_update(rechandle);
			Debugprintf("After Read recovery Avail %d ..", avail);

			return 0;
		}
	}


	if (ret < nSamples)
		return 0;

	if (m_recchannels == 1)
	{
		for (n = 0; n < ret; n++)
		{
			*(input++) = samples[n];
			*(input++) = samples[n];		// Duplicate
		}
	}
	else
	{
		for (n = 0; n < ret * 2; n++)		// return all
		{
			*(input++) = samples[n];
		}
	}

	return ret;
}




int PriorSize = 0;

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;				// DMA Buffer being used 0 or 1

BOOL DMARunning = FALSE;		// Used to start DMA on first write

void ProcessNewSamples(short * Samples, int nSamples);

short * SendtoCard(short * buf, int n)
{
	if (Loopback)
	{
		// Loop back   to decode for testing

		ProcessNewSamples(buf, 1200);		// signed
	}

	if (SoundMode == 1)			// OSS
		oss_write(buf, n);
	else if (SoundMode == 2)	// Pulse
		pulse_write(buf, n);
	else
	{
		if (playhandle)
			SoundCardWrite(buf, n);

		//	txSleep(10);				// Run buckground while waiting 
	}

	Index = !Index;
	return &buffer[Index][0];
}

short loopbuff[1200];		// Temp for testing - loop sent samples to decoder


//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

short * SoundInit();

void GetSoundDevices()
{
	if (onlyMixSnoop)
	{
		gethints();
	}
	else
	{
		if (SoundMode == 0)
		{
			GetInputDeviceCollection();
			GetOutputDeviceCollection();
		}
		else if (SoundMode == 1)
		{
			PlaybackCount = 3;

			strcpy(&PlaybackNames[0][0], "/dev/dsp0");
			strcpy(&PlaybackNames[1][0], "/dev/dsp1");
			strcpy(&PlaybackNames[2][0], "/dev/dsp2");

			CaptureCount = 3;

			strcpy(&CaptureNames[0][0], "/dev/dsp0");
			strcpy(&CaptureNames[1][0], "/dev/dsp1");
			strcpy(&CaptureNames[2][0], "/dev/dsp2");
		}
		else if (SoundMode == 2)
		{
			// Pulse

			listpulse();
		}
	}
}

int InitSound(BOOL Quiet)
{
	if (using48000)
	{
		ReceiveSize = 2048;
		SendSize = 4096;		// 100 mS for now
		SampleRate = 48000;
	}
	else
	{
		ReceiveSize = 512;
		SendSize = 1024;
		SampleRate = 12000;
	}

	GetSoundDevices();

	switch (SoundMode)
	{
	case 0:				// ALSA

		if (!OpenSoundCard(CaptureDevice, PlaybackDevice, SampleRate, SampleRate, Quiet))
			return FALSE;


		break;

	case 1:				// OSS

		if (!oss_audio_open(CaptureDevice, PlaybackDevice))
			return FALSE;

		break;

	case 2:				// PulseAudio

		if (!pulse_audio_open(CaptureDevice, PlaybackDevice))
			return FALSE;

		break;

	}

	printf("InitSound %s %s\n", CaptureDevice, PlaybackDevice);

	DMABuffer = SoundInit();
	return TRUE;
}

int min = 0, max = 0, lastlevelreport = 0, lastlevelGUI = 0;
UCHAR CurrentLevel = 0;		// Peak from current samples
UCHAR CurrentLevelR = 0;		// Peak from current samples

void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	int bytes;
#ifdef TXSILENCE
	SendSilence();			// send silence (attempt to fix CM delay issue)
#endif

	if (stdinMode)
	{
		// will block if no input. May get less, in which case wait a bit then try to read rest

		// rtl_udp outputs mono samples

		short input[1200];
		short * ptr1, *ptr2;		
		int n = 20; // Max Wait

		bytes = read(STDIN_FILENO, input, ReceiveSize * 2);		// 4 = Stereo 2 bytes per sample

		while (bytes < ReceiveSize * 2 && n--)
		{
			Sleep(50);	//mS
			bytes += read(STDIN_FILENO, &input[bytes / 2], (ReceiveSize * 2) - bytes);
		}
			
		// if still not enough, too bad!

		if (bytes != ReceiveSize * 2)
		{
			// This seems to happen occasionally even when we shouldn't be in stdin mode. Exit

			Debugprintf("Short Read %d", bytes);
			closeTraceLog();

			Closing = TRUE;

			sleep(1);
			exit(1);
		}


		// convert to stereo

		ptr1 = input;
		ptr2 = inbuffer;
		n = ReceiveSize;

		while (n--)
		{
			*ptr2++ = *ptr1;
			*ptr2++ = *ptr1++;
		}
	}
	else
		bytes = SoundCardRead(inbuffer, ReceiveSize);	 // returns ReceiveSize or none

	if (bytes > 0)
	{
		short * ptr = inbuffer;
		int i;

		for (i = 0; i < ReceiveSize; i++)
		{
			if (*(ptr) < min)
				min = *ptr;
			else if (*(ptr) > max)
				max = *ptr;
			ptr++;
		}


		CurrentLevel = ((max - min) * 75) /32768;	// Scale to 150 max

		if ((Now - lastlevelGUI) > 2000)	// 2 Secs
		{
				lastlevelGUI = Now;

			if ((Now - lastlevelreport) > 60000)	// 60 Secs
			{
				char HostCmd[64];
				lastlevelreport = Now;

				sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);

				Debugprintf("Input peaks = %d, %d", min, max);
			}
			min = max = 0;							// Every 2 secs
		}

		ProcessNewSamples(inbuffer, ReceiveSize);
	}
} 

void StopCapture()
{
	Capturing = FALSE;

#ifdef SHARECAPTURE

	// Stopcapture is only called when we are about to transmit, so use it to open plaback device. We don't keep
	// it open all the time to facilitate sharing.

	OpenSoundPlayback(SavedPlaybackDevice, SavedPlaybackRate, Savedplaychannels, NULL);
#endif
}

void StartCapture()
{
	Capturing = TRUE;

//	Debugprintf("Start Capture");
}

void CloseSound()
{
	switch (SoundMode)
	{
	case 0:				// ALSA

		CloseSoundCard();
		return;

	case 1:				// OSS
		
		oss_audio_close();
		return;

	case 2:				// PulseAudio

		pulse_audio_close();
		return;
	}
}

short * SoundInit()
{
	Index = 0;
	return &buffer[0][0];
}
	
//	Called at end of transmission

int useTimedPTT = 1;

extern int SampleNo;
int pttOnTime();

void SoundFlush()
{
	// Append Trailer then send remaining samples

	snd_pcm_status_t *status = NULL;
	int err, res;
	int lastavail = 0;

	if (Loopback)
		ProcessNewSamples(&buffer[Index][0], Number);

	SendtoCard(&buffer[Index][0], Number);

	// Wait for tx to complete

//	Debugprintf("Flush Soundmode = %d", SoundMode);

	if (SoundMode == 0)		// ALSA
	{
		if (useTimedPTT)
		{
			// Calulate PTT Time from Number of samples and samplerate

			// samples sent is is in SampleNo, Time PTT was raised in timeval pttclk
			// txLatency is extra ptt time to compenstate for time soundcard takes to start outputting samples

			struct timespec pttnow;

			clock_gettime(CLOCK_MONOTONIC, &pttnow);

			time_t pttontimemS = (pttclk.tv_sec * 1000) + (pttclk.tv_nsec / 1000000);
			time_t nowtimemS = (pttnow.tv_sec * 1000) + (pttnow.tv_nsec / 1000000);
						
			// We have already added latency to tail, so don't add again

			int txlenMs = (1000 * SampleNo / TX_Samplerate);		// 12000 samples per sec. 

			Debugprintf("Tx Time %d Time till end = %d", txlenMs, (nowtimemS - pttontimemS));

			txSleep(txlenMs - (nowtimemS - pttontimemS));

		}
		else
		{
			usleep(100000);

			while (1 && playhandle)
			{
				snd_pcm_sframes_t avail = snd_pcm_avail_update(playhandle);

				//		Debugprintf("Waiting for complete. Avail %d Max %d", avail, MaxAvail);

				snd_pcm_status_alloca(&status);					// alloca allocates once per function, does not need a free

	//			Debugprintf("Waiting for complete. Avail %d Max %d last %d", avail, MaxAvail, lastavail);

				if ((err = snd_pcm_status(playhandle, status)) != 0)
				{
					Debugprintf("snd_pcm_status() failed: %s", snd_strerror(err));
					break;
				}

				res = snd_pcm_status_get_state(status);

				//		Debugprintf("PCM Status = %d", res);

				if (res != SND_PCM_STATE_RUNNING || lastavail == avail)			// If sound system is not running then it needs data
	//			if (res != SND_PCM_STATE_RUNNING)				// If sound system is not running then it needs data
		//		if (MaxAvail - avail < 100)	
				{
					// Send complete - Restart Capture

					OpenSoundCapture(SavedCaptureDevice, SavedCaptureRate, 0);
					break;
				}
				lastavail = avail;
				usleep(50000);
			}


		}
		// I think we should turn round the link here. I dont see the point in
		// waiting for MainPoll

#ifdef SHARECAPTURE
		if (playhandle)
		{
			snd_pcm_close(playhandle);
			playhandle = NULL;
		}
#endif
	}
	else if (SoundMode == 1)
	{
		oss_flush();
	}
	else if (SoundMode == 2)
	{
		pulse_flush();
	}

	SoundIsPlaying = FALSE;

	Number = 0;
	
	memset(buffer, 0, sizeof(buffer));
	DMABuffer = &buffer[0][0];

#ifdef TXSILENCE
	SendtoCard(&buffer[0][0], 1200);			// Start sending silence (attempt to fix CM delay issue)
#endif

	StartCapture();
	return;
}

#ifdef TXSILENCE
		
// send silence (attempt to fix CM delay issue)


void SendSilence()
{
	short buffer[2400];

	snd_pcm_sframes_t Avail = snd_pcm_avail_update(playhandle);

	if ((MaxAvail - Avail) < 1200)
	{
		// Keep at least 100 ms of audio in buffer

//		printtick("Silence");

		memset(buffer, 0, sizeof(buffer));
		SendtoCard(buffer, 1200);			// Start sending silence (attempt to fix CM delay issue)
	}
}

#endif

// GPIO access stuff for PTT on PI

#ifdef __ARM_ARCH

/*
   tiny_gpio.c
   2016-04-30
   Public Domain
*/
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define GPSET0 7
#define GPSET1 8

#define GPCLR0 10
#define GPCLR1 11

#define GPLEV0 13
#define GPLEV1 14

#define GPPUD     37
#define GPPUDCLK0 38
#define GPPUDCLK1 39

unsigned piModel;
unsigned piRev;

static volatile uint32_t  *gpioReg = MAP_FAILED;

#define PI_BANK (gpio>>5)
#define PI_BIT  (1<<(gpio&0x1F))

/* gpio modes. */

void gpioSetMode(unsigned gpio, unsigned mode)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   gpioReg[reg] = (gpioReg[reg] & ~(7<<shift)) | (mode<<shift);
}

int gpioGetMode(unsigned gpio)
{
   int reg, shift;

   reg   =  gpio/10;
   shift = (gpio%10) * 3;

   return (*(gpioReg + reg) >> shift) & 7;
}

/* Values for pull-ups/downs off, pull-down and pull-up. */

#define PI_PUD_OFF  0
#define PI_PUD_DOWN 1
#define PI_PUD_UP   2

void gpioSetPullUpDown(unsigned gpio, unsigned pud)
{
   *(gpioReg + GPPUD) = pud;

   usleep(20);

   *(gpioReg + GPPUDCLK0 + PI_BANK) = PI_BIT;

   usleep(20);
  
   *(gpioReg + GPPUD) = 0;

   *(gpioReg + GPPUDCLK0 + PI_BANK) = 0;
}

int gpioRead(unsigned gpio)
{
   if ((*(gpioReg + GPLEV0 + PI_BANK) & PI_BIT) != 0) return 1;
   else                                         return 0;
}
void gpioWrite(unsigned gpio, unsigned level)
{
   if (level == 0)
	   *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else
	   *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;
}

void gpioTrigger(unsigned gpio, unsigned pulseLen, unsigned level)
{
   if (level == 0) *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else            *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;

   usleep(pulseLen);

   if (level != 0) *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else            *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;
}

/* Bit (1<<x) will be set if gpio x is high. */

uint32_t gpioReadBank1(void) { return (*(gpioReg + GPLEV0)); }
uint32_t gpioReadBank2(void) { return (*(gpioReg + GPLEV1)); }

/* To clear gpio x bit or in (1<<x). */

void gpioClearBank1(uint32_t bits) { *(gpioReg + GPCLR0) = bits; }
void gpioClearBank2(uint32_t bits) { *(gpioReg + GPCLR1) = bits; }

/* To set gpio x bit or in (1<<x). */

void gpioSetBank1(uint32_t bits) { *(gpioReg + GPSET0) = bits; }
void gpioSetBank2(uint32_t bits) { *(gpioReg + GPSET1) = bits; }

unsigned gpioHardwareRevision(void)
{
   static unsigned rev = 0;

   FILE * filp;
   char buf[512];
   char term;
   int chars=4; /* number of chars in revision string */

   if (rev) return rev;

   piModel = 0;

   filp = fopen ("/proc/cpuinfo", "r");

   if (filp != NULL)
   {
      while (fgets(buf, sizeof(buf), filp) != NULL)
      {
         if (piModel == 0)
         {
            if (!strncasecmp("model name", buf, 10))
            {
               if (strstr (buf, "ARMv6") != NULL)
               {
                  piModel = 1;
                  chars = 4;
               }
               else if (strstr (buf, "ARMv7") != NULL)
               {
                  piModel = 2;
                  chars = 6;
               }
               else if (strstr (buf, "ARMv8") != NULL)
               {
                  piModel = 2;
                  chars = 6;
               }
            }
         }

         if (!strncasecmp("revision", buf, 8))
         {
            if (sscanf(buf+strlen(buf)-(chars+1),
               "%x%c", &rev, &term) == 2)
            {
               if (term != '\n') rev = 0;
            }
         }
      }

      fclose(filp);
   }
   return rev;
}

int gpioInitialise(void)
{
   int fd;

   piRev = gpioHardwareRevision(); /* sets piModel and piRev */

   fd = open("/dev/gpiomem", O_RDWR | O_SYNC) ;

   if (fd < 0)
   {
      fprintf(stderr, "failed to open /dev/gpiomem\n");
      return -1;
   }

   gpioReg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

   close(fd);

   if (gpioReg == MAP_FAILED)
   {
      fprintf(stderr, "Bad, mmap failed\n");
      return -1;
   }
   return 0;
}


	
#endif



int stricmp(const unsigned char * pStr1, const unsigned char *pStr2)
{
    unsigned char c1, c2;
    int  v;

	if (pStr1 == NULL)
	{
		if (pStr2)
			Debugprintf("stricmp called with NULL 1st param - 2nd %s ", pStr2);
		else
			Debugprintf("stricmp called with two NULL params");

		return 1;
	}


    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* The casts are necessary when pStr1 is shorter & char is signed */
        v = tolower(c1) - tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') );

    return v;
}

char Leds[8]= {0};
unsigned int PKTLEDTimer = 0;



static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};


VOID COMSetDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &status);
}

VOID COMClearDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &status);
}

VOID COMSetRTS(HANDLE fd)
{
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1)
		perror("COMSetRTS PTT TIOCMGET");
	status |= TIOCM_RTS;
	if (ioctl(fd, TIOCMSET, &status) == -1)
		perror("COMSetRTS PTT TIOCMSET");
}

VOID COMClearRTS(HANDLE fd)
{
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1)
		perror("COMClearRTS PTT TIOCMGET");
	status &= ~TIOCM_RTS;
	if (ioctl(fd, TIOCMSET, &status) == -1)
		perror("COMClearRTS PTT TIOCMSET");

}



HANDLE OpenCOMPort(char * Port, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char buf[256];

	//	Linux Version.

	int fd;
	u_long param = 1;
	struct termios term;
	struct speed_struct *s;

	char fulldev[80];

	UNUSED(Stopbits);

	sprintf(fulldev, "/dev/%s", Port);

	printf("%s\n", fulldev);

	if ((fd = open(fulldev, O_RDWR | O_NDELAY)) == -1)
	{
		if (Quiet == 0)
		{
			perror("Com Open Failed");
			sprintf(buf, " %s could not be opened", (char *)fulldev);
			Debugprintf(buf);
		}
		return 0;
	}

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)
		if (s->user_speed == speed)
			break;

	if (s->user_speed == -1)
	{
		fprintf(stderr, "tty_speed: invalid speed %d", speed);
		return FALSE;
	}

	if (tcgetattr(fd, &term) == -1)
	{
		perror("tty_speed: tcgetattr");
		return FALSE;
	}

	cfmakeraw(&term);
	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	Debugprintf("Port %s fd %d", fulldev, fd);

	if (SetDTR)
		COMSetDTR(fd);
	else
		COMClearDTR(fd);

	if (SetRTS)
		COMSetRTS(fd);
	else
		COMClearRTS(fd);

	return fd;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	//	Some systems seem to have a very small max write size

	int ToSend = BytesToWrite;
	int Sent = 0, ret;

	while (ToSend)
	{
		ret = write(fd, &Block[Sent], ToSend);

		if (ret >= ToSend)
			return TRUE;

//		perror("WriteCOM");

		if (ret == -1)
		{
			if (errno != 11 && errno != 35)					// Would Block
				return FALSE;

			usleep(10000);
			ret = 0;
		}

		Sent += ret;
		ToSend -= ret;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	close(fd);
}

// "hints" processing for looking for SNOOP/MIX devices

int gethints()
{
	const char *iface = "pcm";
	void **hints;
	char **n;
	int err;
	char hwdev[256];
	snd_pcm_t *pcm = NULL;
	char NameString[256];

	CloseSoundCard();

	Debugprintf("Available Mix/Snoop Devices\n");

	PlaybackCount = 0;
	CaptureCount = 0;

	err = snd_device_name_hint(-1, iface, &hints);

	if (err < 0)
		Debugprintf("snd_device_name_hint error: %s", snd_strerror(err));

	n = (char **)hints;

	while (*n != NULL)
	{
		if (memcmp(*n, "NAMEmix", 7) == 0)	//NAMEmix00|DESCQtSM Mix for hw0:0
		{
			char Hint[256];
			char * ptr;
			snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

			strcpy(Hint, *n);

			ptr = strchr(Hint, '|');

			if (ptr)
			{
				*ptr++ = 0;
			}	
			
			strcpy(hwdev, &Hint[4]);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);

			if (err)
			{
				Debugprintf("Error %d opening output device %s ", err, hwdev);
				goto nextdevice;
			}

			// Add device to list

			if (ptr)
				sprintf(NameString, "%s %s", hwdev, &ptr[4]);
			else
				strcpy(NameString, hwdev);

			Debugprintf(NameString);

			strcpy(PlaybackNames[PlaybackCount++], NameString);
			snd_pcm_close(pcm);
			pcm = NULL;
		}
		else if (memcmp(*n, "NAMEsnoop", 9) == 0)
		{
			char Hint[256];
			char * ptr;
			snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

			strcpy(Hint, *n);

			ptr = strchr(Hint, '|');

			if (ptr)
			{
				*ptr++ = 0;
			}

			strcpy(hwdev, &Hint[4]);

			err = snd_pcm_open(&pcm, hwdev, stream, SND_PCM_NONBLOCK);

			if (err)
			{
				Debugprintf("Error %d opening input device %s ", err, hwdev);
				goto nextdevice;
			}

			// Add device to list

			if (ptr)
				sprintf(NameString, "%s %s", hwdev, &ptr[4]);
			else
				strcpy(NameString, hwdev);

			Debugprintf(NameString);

			strcpy(CaptureNames[CaptureCount++], NameString);
			snd_pcm_close(pcm);
			pcm = NULL;
		}

	nextdevice:
		
		n++;

	}
	snd_device_name_free_hint(hints);
	return 0;
}



