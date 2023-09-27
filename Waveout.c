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

// UZ7HO Soundmodem Port by John Wiseman G8BPQ
//
//	Passes audio samples to the sound interface

//	Windows uses WaveOut

//	Nucleo uses DMA

//	Linux will use ALSA

//	This is the Windows Version

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <fcntl.h>
#include <mmsystem.h>

#include "UZ7HOStuff.h"

#pragma comment(lib, "winmm.lib")
void printtick(char * msg);
void PollReceivedSamples();
short * SoundInit();
void StdinPollReceivedSamples();

#include <math.h>

void GetSoundDevices();

// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

// Currently use 1200 samples for TX but 480 for RX to reduce latency

#define MaxReceiveSize 2048		// Enough for 9600
#define MaxSendSize 4096

short buffer[2][MaxSendSize * 2];		// Two Transfer/DMA buffers of 0.1 Sec  (x2 for Stereo)
short inbuffer[5][MaxReceiveSize * 2];	// Input Transfer/ buffers of 0.1 Sec (x2 for Stereo)

extern short * DMABuffer;
extern int Number;

int ReceiveSize = 512;
int SendSize = 1024;
int using48000 = 0;

int SoundMode = 0;
int stdinMode = 0;

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "real"; //"2";
char PlaybackDevice[80] = "real"; //"1";

int CaptureIndex = -1;		// Card number
PlayBackIndex = -1;

BOOL UseLeft = 1;
BOOL UseRight = 1;
char LogDir[256] = "";

FILE *logfile[3] = {NULL, NULL, NULL};
char LogName[3][256] = {"ARDOPDebug", "ARDOPException", "ARDOPSession"};

char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

int CaptureCount = 0;
int PlaybackCount = 0;

char CaptureNames[16][256]= {""};
char PlaybackNames[16][256]= {""};

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 2, 12000, 48000, 4, 16, 0 };

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEHDR header[2] =
{
	{(char *)buffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)buffer[1], 0, 0, 0, 0, 0, 0, 0}
};

WAVEHDR inheader[5] =
{
	{(char *)inbuffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[1], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[2], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[3], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[4], 0, 0, 0, 0, 0, 0, 0}
};

WAVEOUTCAPSA pwoc;
WAVEINCAPSA pwic;

unsigned int RTC = 0;

int InitSound(BOOL Quiet);
void HostPoll();

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite);

int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

extern BOOL blnDISCRepeating;

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

	
VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	WriteDebugLog(Mess);

	return;
}


void platformInit()
{
	TIMECAPS tc;
	unsigned int     wTimerRes;
	DWORD	t, lastt = 0;
	int i = 0;


	_strupr(CaptureDevice);
	_strupr(PlaybackDevice);

	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	GetSoundDevices();

	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
		printf("Failed to set High Priority (%d)\n", GetLastError());
}

unsigned int getTicks()
{
	return timeGetTime();
//		QueryPerformanceCounter(&NewTicks);
//		return (int)(NewTicks.QuadPart - StartTicks.QuadPart) / Frequency.QuadPart;
}

void printtick(char * msg)
{
	QueryPerformanceCounter(&NewTicks);
	Debugprintf("%s %i\r", msg, Now - LastNow);
	LastNow = Now;
}

void txSleep(int mS)
{
	// called while waiting for next TX buffer. Run background processes

	while (mS > 50)
	{
		if (SoundMode == 3)
			UDPPollReceivedSamples();
		else
			PollReceivedSamples();			// discard any received samples

		QSleep(50);
		mS -= 50;
	}

	QSleep(mS);

	if (SoundMode == 3)
		UDPPollReceivedSamples();
	else
		PollReceivedSamples();			// discard any received samples
}

int PriorSize = 0;

int Index = 0;				// DMA TX Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used


FILE * wavfp1;

BOOL DMARunning = FALSE;		// Used to start DMA on first write

BOOL SeeIfCardBusy()
{
	if ((header[!Index].dwFlags & WHDR_DONE))
		return 0;

	return 1;
}

extern void sendSamplestoUDP(short * Samples, int nSamples, int Port);

extern int UDPClientPort;

short * SendtoCard(unsigned short * buf, int n)
{
	if (SoundMode == 3)			// UDP
	{
		sendSamplestoUDP(buf, n, UDPClientPort);
		return buf;
	}

	if (SoundMode == 4)			// STDOUT
	{
		sendSamplestoStdout(buf, n);
		return buf;
	}


	header[Index].dwBufferLength = n * 4;

	waveOutPrepareHeader(hWaveOut, &header[Index], sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header[Index], sizeof(WAVEHDR));

	// wait till previous buffer is complete

	while (!(header[!Index].dwFlags & WHDR_DONE))
	{
		txSleep(5);				// Run buckground while waiting 
	}

	waveOutUnprepareHeader(hWaveOut, &header[!Index], sizeof(WAVEHDR));
	Index = !Index;

	return &buffer[Index][0];
}


//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

void GetSoundDevices()
{
	int i;

	if (SoundMode == 1)
	{
		PlaybackCount = 3;

		strcpy(&PlaybackNames[0][0], "/dev/dsp0");
		strcpy(&PlaybackNames[1][0], "/dev/dsp1");
		strcpy(&PlaybackNames[2][0], "/dev/dsp2");

		CaptureCount = 3;

		strcpy(&CaptureNames[0][0], "/dev/dsp0");
		strcpy(&CaptureNames[1][0], "/dev/dsp1");
		strcpy(&CaptureNames[2][0], "/dev/dsp2");
		return;
	}

	else if (SoundMode == 2)		// Pulse
	{
		PlaybackCount = 1;
		strcpy(&PlaybackNames[0][0], "Pulse");

		CaptureCount = 1;
		strcpy(&CaptureNames[0][0], "Pulse");
		return;
	}
	else if (SoundMode == 3)		// UDP
	{
		PlaybackCount = 1;
		strcpy(&PlaybackNames[0][0], "UDP");

		CaptureCount = 1;
		strcpy(&CaptureNames[0][0], "UDP");
		return;
	}

	Debugprintf("Capture Devices");

	CaptureCount = waveInGetNumDevs();

	CaptureDevices = malloc((MAXPNAMELEN + 2) * (CaptureCount + 2));
	CaptureDevices[0] = 0;
	
	for (i = 0; i < CaptureCount; i++)
	{
		waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveInGetDevCapsA((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPSA));

		if (CaptureDevices)
			strcat(CaptureDevices, ",");
		strcat(CaptureDevices, pwic.szPname);
		Debugprintf("%d %s", i, pwic.szPname);
		memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
		_strupr(&CaptureNames[i][0]);
	}

	CaptureCount++;
	Debugprintf("%d %s", i, "STDIN");
	strcpy(&CaptureNames[i][0], "STDIN"); 

	Debugprintf("Playback Devices");

	PlaybackCount = waveOutGetNumDevs();

	PlaybackDevices = malloc((MAXPNAMELEN + 2) * PlaybackCount);
	PlaybackDevices[0] = 0;

	for (i = 0; i < PlaybackCount; i++)
	{
		waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveOutGetDevCapsA((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPSA));

		if (PlaybackDevices[0])
			strcat(PlaybackDevices, ",");
		strcat(PlaybackDevices, pwoc.szPname);
		Debugprintf("%i %s", i, pwoc.szPname);
		memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
		_strupr(&PlaybackNames[i][0]);
		waveOutClose(hWaveOut);
	}
}


HANDLE hStdin;

int InitSound(BOOL Report)
{
	int i, t, ret;

	if (SoundMode == 4)
	{
		char fn[] = "D:samples.wav";

		// create a separate new console window
//		AllocConsole();

		// attach the new console to this application's process
//		AttachConsole(GetCurrentProcessId());

		hStdin =  CreateFileA(fn,
			GENERIC_READ,
			1,                    // exclusive access
			NULL,                 // no security attrs
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
			
		// reopen the std I/O streams to redirect I/O to the new console

//		freopen("CON", "r", stdin);

//		i = _setmode(_fileno(stdin), _O_BINARY);
//		if (i == -1)
//			i = errno;
//		else
//			printf("'stdin' successfully changed to binary mode\n");

		return TRUE;
	}

	header[0].dwFlags = WHDR_DONE;
	header[1].dwFlags = WHDR_DONE;

	if (strlen(PlaybackDevice) <= 2)
		PlayBackIndex = atoi(PlaybackDevice);
	else
	{
		// Name instead of number. Look for a substring match

		for (i = 0; i < PlaybackCount; i++)
		{
			if (strstr(&PlaybackNames[i][0], PlaybackDevice))
			{
				PlayBackIndex = i;
				break;
			}
		}
	}

	if (using48000)
	{
		wfx.nSamplesPerSec = 48000;
		wfx.nAvgBytesPerSec = 48000 * 4;
		ReceiveSize = 2048;
		SendSize = 4096;		// 100 mS for now
	}
	else
	{
		wfx.nSamplesPerSec = 12000;
		wfx.nAvgBytesPerSec = 12000 * 4;
		ReceiveSize = 512;
		SendSize = 1024;
	}

    ret = waveOutOpen(&hWaveOut, PlayBackIndex, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER

	if (ret)
		Debugprintf("Failed to open WaveOut Device %s Error %d", PlaybackDevice, ret);
	else
	{
		ret = waveOutGetDevCapsA((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPSA));
		if (Report)
			Debugprintf("Opened WaveOut Device %s", pwoc.szPname);
	}

	if (strlen(CaptureDevice) <= 2)
		CaptureIndex = atoi(CaptureDevice);
	else
	{
		// Name instead of number. Look for a substring match

		for (i = 0; i < CaptureCount; i++)
		{
			if (strstr(&CaptureNames[i][0], CaptureDevice))
			{
				CaptureIndex = i;
				break;
			}
		}
	}

	if (strcmp(CaptureNames[CaptureIndex], "STDIN") == 0)
	{
		stdinMode = 1;
		char fn[] = "D:samples.wav";

		hStdin = CreateFileA(fn,
			GENERIC_READ,
			1,                    // exclusive access
			NULL,                 // no security attrs
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		DMABuffer = SoundInit();

		return TRUE;
	}

    ret = waveInOpen(&hWaveIn, CaptureIndex, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	if (ret)
		Debugprintf("Failed to open WaveIn Device %s Error %d", CaptureDevice, ret);
	else
	{
		ret = waveInGetDevCapsA((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPSA));
		if (Report)
			Debugprintf("Opened WaveIn Device %s", pwic.szPname);
	}

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	for (i = 0; i < NumberofinBuffers; i++)
	{
		inheader[i].dwBufferLength = ReceiveSize * 4;

		ret = waveInPrepareHeader(hWaveIn, &inheader[i], sizeof(WAVEHDR));
		ret = waveInAddBuffer(hWaveIn, &inheader[i], sizeof(WAVEHDR));
	}

	ret = waveInStart(hWaveIn);

	DMABuffer = SoundInit();


// This generates a nice musical pattern for sound interface testing

/*
for (i = 0; i < 100; i++)
{
	for (t = 0; t < SendSize ; ++t)
		SampleSink(((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xff) * 255);
}
*/

	return TRUE;
}

static int min = 0, max = 0, lastlevelGUI = 0, lastlevelreport = 0;

UCHAR CurrentLevel = 0;		// Peak from current samples

void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	// For level display we want a fairly rapid level average but only want to report 
	// to log every 10 secs or so

	// with Windows we get mono data

	if (stdinMode)
	{
		StdinPollReceivedSamples();
		return;
	}

	while (inheader[inIndex].dwFlags & WHDR_DONE)
	{
		short * ptr = &inbuffer[inIndex][0];
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
//			if (WaterfallActive == 0 && SpectrumActive == 0)				// Don't need to send as included in Waterfall Line
//				SendtoGUI('L', &CurrentLevel, 1);	// Signal Level
			
			lastlevelGUI = Now;

			if ((Now - lastlevelreport) > 10000)	// 10 Secs
			{
				char HostCmd[64];
				lastlevelreport = Now;

				sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
				Debugprintf("Input peaks = %d, %d", min, max);

			}
			min = max = 0;
		}

//		debugprintf(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
//		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/4);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex++;
		
		if (inIndex == NumberofinBuffers)
			inIndex = 0;
	}
}



/*

// Pre GUI Version
void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	if (inheader[inIndex].dwFlags & WHDR_DONE)
	{
		short * ptr = &inbuffer[inIndex][0];
		int i;

		for (i = 0; i < ReceiveSize; i++)
		{
			if (*(ptr) < min)
				min = *ptr;
			else if (*(ptr) > max)
				max = *ptr;
			ptr++;
		}
		leveltimer++;

		if (leveltimer > 100)
		{
			char HostCmd[64];
			leveltimer = 0;
			sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
			QueueCommandToHost(HostCmd);

			debugprintf(LOGDEBUG, "Input peaks = %d, %d", min, max);
			min = max = 0;
		}

//		debugprintf(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/2);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex++;
		
		if (inIndex == NumberofinBuffers)
			inIndex = 0;
	}
}
*/

void StopCapture()
{
	Capturing = FALSE;

//	waveInStop(hWaveIn);
//	debugprintf(LOGDEBUG, "Stop Capture");
}

void StartCapture()
{
	Capturing = TRUE;
//	debugprintf(LOGDEBUG, "Start Capture");
}
void CloseSound()
{ 
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);
}

#include <stdarg.h>

VOID CloseDebugLog()
{	
	if(logfile[0])
		fclose(logfile[0]);
	logfile[0] = NULL;
}


FILE *statslogfile = NULL;

VOID CloseStatsLog()
{
	fclose(statslogfile);
	statslogfile = NULL;
}

VOID WriteSamples(short * buffer, int len)
{
	fwrite(buffer, 1, len * 2, wavfp1);
}

short * SoundInit()
{
	Index = 0;
	inIndex = 0;
	return &buffer[0][0];


}
	
//	Called at end of transmission

extern int Number;				// Number of samples waiting to be sent

// Subroutine to add trailer before filtering

void SoundFlush()
{
	// Append Trailer then wait for TX to complete

//	AddTrailer();			// add the trailer.

//	if (Loopback)
//		ProcessNewSamples(buffer[Index], Number);

	SendtoCard(buffer[Index], Number);

	//	Wait for all sound output to complete
	
	while (!(header[0].dwFlags & WHDR_DONE))
		txSleep(10);
	while (!(header[1].dwFlags & WHDR_DONE))
		txSleep(10);

	// I think we should turn round the link here. I dont see the point in
	// waiting for MainPoll

	SoundIsPlaying = FALSE;

	// Clear buffers

	Number = 0;
	memset(buffer, 0, sizeof(buffer));
	DMABuffer = &buffer[0][0];

//	StartCapture();

	return;
}

int CheckAllSent()
{
	if ((header[0].dwFlags & WHDR_DONE) && (header[1].dwFlags & WHDR_DONE))
		return 1;

	return 0;
}


void StartCodec(char * strFault)
{
	strFault[0] = 0;
	InitSound(FALSE);

}

void StopCodec(char * strFault)
{
	CloseSound();
	strFault[0] = 0;
}

// Serial Port Stuff

HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal;
	COMMTIMEOUTS  CommTimeOuts;
	int	Err;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	if (_memicmp(pPort, "COM", 3) == 0)
	{
		char * pp = (char *)pPort;
		int p = atoi(&pp[3]);
		sprintf(szPort, "\\\\.\\COM%d", p);
	}
	else	
		strcpy(szPort, pPort);
	
	// open COMM device

	fd = CreateFileA(szPort, GENERIC_READ | GENERIC_WRITE,
		0,                    // exclusive access
		NULL,                 // no security attrs
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (fd == (HANDLE)-1)
	{
		if (Quiet == 0)
		{
			sprintf(buf, " %s could not be opened \r\n ", pPort);
			OutputDebugStringA(buf);
		}
		return (FALSE);
	}

	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096);

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
		PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(fd, &CommTimeOuts);

	dcb.DCBlength = sizeof(DCB);

	GetCommState(fd, &dcb);

	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = 0;
	dcb.StopBits = TWOSTOPBITS;
	dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;

	// setup software flow control

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 100;
	dcb.XoffLim = 100;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;

	fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		if (SetDTR)
			EscapeCommFunction(fd, SETDTR);
		if (SetRTS)
			EscapeCommFunction(fd, SETRTS);
	}
	else
	{
		sprintf(buf, "%s Setup Failed %d ", pPort, GetLastError());

		printf(buf);
		OutputDebugStringA(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;

}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue

	ClearCommError(fd, &dwErrorFlags, &ComStat);

	dwLength = min((DWORD)MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL);

		if (!fReadStat)
		{
			dwLength = 0;
			ClearCommError(fd, &dwErrorFlags, &ComStat);
		}
	}

	return dwLength;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
		&BytesWritten, NULL);

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
	{
		int Err = GetLastError();
		ClearCommError(fd, &ErrorFlags, &ComStat);
		return FALSE;
	}
	return TRUE;
}


VOID CloseCOMPort(int fd)
{
	SetCommMask((HANDLE)fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm((HANDLE)fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle((HANDLE)fd);
}


VOID COMSetDTR(int fd)
{
	EscapeCommFunction((HANDLE)fd, SETDTR);
}

VOID COMClearDTR(int fd)
{
	EscapeCommFunction((HANDLE)fd, CLRDTR);
}

VOID COMSetRTS(int fd)
{
	EscapeCommFunction((HANDLE)fd, SETRTS);
}

VOID COMClearRTS(int fd)
{
	EscapeCommFunction((HANDLE)fd, CLRRTS);
}

/*

void CatWrite(char * Buffer, int Len)
{
	if (hCATDevice)
		WriteCOMBlock(hCATDevice, Buffer, Len);
}

*/

void * initPulse()
{
	return NULL;
}



void StdinPollReceivedSamples()
{
	short buffer[2048];
	DWORD out;
	int res = ReadFile(hStdin, buffer, 2048, &out, NULL);
	if (res <= 0)
	{
		res = GetLastError();
		printf("\nEnd of file on stdin.  Exiting.\n");
	}

	short * ptr = buffer;
	int i;

	for (i = 0; i < ReceiveSize; i++)
	{
		if (*(ptr) < min)
			min = *ptr;
		else if (*(ptr) > max)
			max = *ptr;
		ptr++;
	}

	CurrentLevel = ((max - min) * 75) / 32768;	// Scale to 150 max

	if ((Now - lastlevelGUI) > 2000)	// 2 Secs
	{
//		RefreshLevel(CurrentLevel);	// Signal Level
		lastlevelGUI = Now;

		if ((Now - lastlevelreport) > 10000)	// 10 Secs
		{
			char HostCmd[64];
			lastlevelreport = Now;

			sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
			Debugprintf("Input peaks = %d, %d", min, max);

		}
		min = max = 0;
	}

	//		debugprintf(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
	//		if (Capturing && Loopback == FALSE)
	ProcessNewSamples(buffer, 512);


}


