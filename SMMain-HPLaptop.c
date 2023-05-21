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

#include "UZ7HOStuff.h"
#include "fftw3.h"
#include <time.h>
#include "ecc.h"				// RS Constants
#include "hidapi.h"
#include <fcntl.h>
#include <errno.h>

BOOL KISSServ;
int KISSPort;

BOOL AGWServ;
int AGWPort;

int Number = 0;				// Number waiting to be sent

int SoundIsPlaying = 0;
int UDPSoundIsPlaying = 0;
int Capturing = 0;

extern unsigned short buffer[2][1200];
extern int SoundMode;
extern int needRSID[4];

extern short * DMABuffer;

unsigned short * SendtoCard(unsigned short * buf, int n);
short * SoundInit();
void DoTX(int Chan);
void UDPPollReceivedSamples();


extern int SampleNo;

extern int pnt_change[5];				// Freq Changed Flag

// fftw library interface


fftwf_complex *in, *out;
fftwf_plan p;

#define N 2048

void initfft()
{
	in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

void dofft(short * inp, float * outr, float * outi)
{
	int i;
	
	fftwf_complex * fft = in;

	for (i = 0; i < N; i++)
	{
		fft[0][0] = inp[0] * 1.0f;
		fft[0][1] = 0;
		fft++;
		inp++;
	}

	fftwf_execute(p); 

	fft = out;

	for (i = 0; i < N; i++)
	{
		outr[0] = fft[0][0];
		outi[0] = fft[0][1];
		fft++;
		outi++;
		outr++;
	}
}

void freefft()
{
	fftwf_destroy_plan(p);
	fftwf_free(in);
	fftwf_free(out);
}

int nonGUIMode = 0;

void soundMain()
{
	// non platform specific initialisation

	platformInit();

	// initialise fft library

	RsCreate();				// RS code for MPSK

	detector_init();
	KISS_init();
	ax25_init();
	init_raduga();			// Set up waterfall colour table

	initfft();

	if (nonGUIMode)
	{
		Firstwaterfall = 0;
		Secondwaterfall = 0;
	}

	OpenPTTPort();
}


void SampleSink(int LR, short Sample)
{
	// This version is passed samples one at a time, as we don't have
	//	enough RAM in embedded systems to hold a full audio frame

	// LR - 1 == Right Chan

#ifdef TEENSY	
		int work = Sample;
		DMABuffer[Number++] = (work + 32768) >> 4; // 12 bit left justify
#else
	if (SCO)			// Single Channel Output - same to both L and R
	{
		DMABuffer[2 * Number] = Sample;
		DMABuffer[1 + 2 * Number] = Sample;

	}
	else
	{
		if (LR)				// Right
		{
			DMABuffer[1 + 2 * Number] = Sample;
			DMABuffer[2 * Number] = 0;
		}
		else
		{
			DMABuffer[2 * Number] = Sample;
			DMABuffer[1 + 2 * Number] = 0;
		}
	}
	Number++;
#endif
		if (Number >= SendSize)
		{
			// send this buffer to sound interface

			DMABuffer = SendtoCard(DMABuffer, SendSize);
			Number = 0;
		}
	

//	Last120[Last120Put++] = Sample;

//	if (Last120Put == (intN + 1))
//		Last120Put = 0;

	SampleNo++;
}


void Flush()
{
	SoundFlush(Number);
}

int ipow(int base, int exp)
{
	int result = 1;
	while (exp)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}

int NumberOfBitsNeeded(int PowerOfTwo)
{
	int i;

	for (i = 0; i <= 16; i++)
	{
		if ((PowerOfTwo & ipow(2, i)) != 0)
			return i;

	}
	return 0;
}


int ReverseBits(int Index, int NumBits)
{
	int i, Rev = 0;

	for (i = 0; i < NumBits; i++)
	{
		Rev = (Rev * 2) | (Index & 1);
		Index = Index / 2;
	}

	return Rev;
}


void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform)
{
	float AngleNumerator;
	unsigned char NumBits;

	int i, j, K, n, BlockSize, BlockEnd;
	float DeltaAngle, DeltaAr;
	float Alpha, Beta;
	float TR, TI, AR, AI;

	if (InverseTransform)
		AngleNumerator = -2.0f * M_PI;
	else
		AngleNumerator = 2.0f * M_PI;

	NumBits = NumberOfBitsNeeded(NumSamples);

	for (i = 0; i < NumSamples; i++)
	{
		j = ReverseBits(i, NumBits);
		RealOut[j] = RealIn[i];
		ImagOut[j] = 0.0f; // Not using i in ImageIn[i];
	}

	BlockEnd = 1;
	BlockSize = 2;

	while (BlockSize <= NumSamples)
	{
		DeltaAngle = AngleNumerator / BlockSize;
		Alpha = sinf(0.5f * DeltaAngle);
		Alpha = 2.0f * Alpha * Alpha;
		Beta = sinf(DeltaAngle);

		i = 0;

		while (i < NumSamples)
		{
			AR = 1.0f;
			AI = 0.0f;

			j = i;

			for (n = 0; n < BlockEnd; n++)
			{
				K = j + BlockEnd;
				TR = AR * RealOut[K] - AI * ImagOut[K];
				TI = AI * RealOut[K] + AR * ImagOut[K];
				RealOut[K] = RealOut[j] - TR;
				ImagOut[K] = ImagOut[j] - TI;
				RealOut[j] = RealOut[j] + TR;
				ImagOut[j] = ImagOut[j] + TI;
				DeltaAr = Alpha * AR + Beta * AI;
				AI = AI - (Alpha * AI - Beta * AR);
				AR = AR - DeltaAr;
				j = j + 1;
			}
			i = i + BlockSize;
		}
		BlockEnd = BlockSize;
		BlockSize = BlockSize * 2;
	}

	if (InverseTransform)
	{
		//	Normalize the resulting time samples...

		for (i = 0; i < NumSamples; i++)
		{
			RealOut[i] = RealOut[i] / NumSamples;
			ImagOut[i] = ImagOut[i] / NumSamples;
		}
	}
}



int LastBusyCheck = 0;

extern UCHAR CurrentLevel;

#ifdef PLOTSPECTRUM		
float dblMagSpectrum[206];
float dblMaxScale = 0.0f;
extern UCHAR Pixels[4096];
extern UCHAR * pixelPointer;
#endif

extern int blnBusyStatus;
BusyDet = 0;

#define PLOTWATERFALL

int WaterfallActive = 1;
int SpectrumActive;

/*

void UpdateBusyDetector(short * bytNewSamples)
{
	float dblReF[1024];
	float dblImF[1024];
	float dblMag[206];
#ifdef PLOTSPECTRUM
	float dblMagMax = 0.0000000001f;
	float dblMagMin = 10000000000.0f;
#endif
	UCHAR Waterfall[256];			// Colour index values to send to GUI
	int clrTLC = Lime;				// Default Bandwidth lines on waterfall

	static BOOL blnLastBusyStatus;

	float dblMagAvg = 0;
	int intTuneLineLow, intTuneLineHi, intDelta;
	int i;

	//	if (State != SearchingForLeader)
	//		return;						// only when looking for leader

	if (Now - LastBusyCheck < 100)
		return;

	LastBusyCheck = Now;

	FourierTransform(1024, bytNewSamples, &dblReF[0], &dblImF[0], FALSE);

	for (i = 0; i < 206; i++)
	{
		//	starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)

		dblMag[i] = powf(dblReF[i + 25], 2) + powf(dblImF[i + 25], 2);	 // first pass 
		dblMagAvg += dblMag[i];
#ifdef PLOTSPECTRUM		
		dblMagSpectrum[i] = 0.2f * dblMag[i] + 0.8f * dblMagSpectrum[i];
		dblMagMax = max(dblMagMax, dblMagSpectrum[i]);
		dblMagMin = min(dblMagMin, dblMagSpectrum[i]);
#endif
	}

	//	LookforPacket(dblMag, dblMagAvg, 206, &dblReF[25], &dblImF[25]);
	//	packet_process_samples(bytNewSamples, 1200);

	intDelta = roundf(500 / 2) + 50 / 11.719f;

	intTuneLineLow = max((103 - intDelta), 3);
	intTuneLineHi = min((103 + intDelta), 203);

//	if (ProtocolState == DISC)		// ' Only process busy when in DISC state
	{
	//	blnBusyStatus = BusyDetect3(dblMag, intTuneLineLow, intTuneLineHi);

		if (blnBusyStatus && !blnLastBusyStatus)
		{
//			QueueCommandToHost("BUSY TRUE");
//			newStatus = TRUE;				// report to PTC

			if (!WaterfallActive && !SpectrumActive)
			{
				UCHAR Msg[2];

//				Msg[0] = blnBusyStatus;
//				SendtoGUI('B', Msg, 1);
			}
		}
		//    stcStatus.Text = "TRUE"
			//    queTNCStatus.Enqueue(stcStatus)
			//    'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		else if (blnLastBusyStatus && !blnBusyStatus)
		{
//			QueueCommandToHost("BUSY FALSE");
//			newStatus = TRUE;				// report to PTC

			if (!WaterfallActive && !SpectrumActive)
			{
				UCHAR Msg[2];

				Msg[0] = blnBusyStatus;
//				SendtoGUI('B', Msg, 1);
			}
		}
		//    stcStatus.Text = "FALSE"
		//    queTNCStatus.Enqueue(stcStatus)
		//    'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		blnLastBusyStatus = blnBusyStatus;
	}

	if (BusyDet == 0)
		clrTLC = Goldenrod;
	else if (blnBusyStatus)
		clrTLC = Fuchsia;

	// At the moment we only get here what seaching for leader,
	// but if we want to plot spectrum we should call
	// it always



	if (WaterfallActive)
	{
#ifdef PLOTWATERFALL
		dblMagAvg = log10f(dblMagAvg / 5000.0f);

		for (i = 0; i < 206; i++)
		{
			// The following provides some AGC over the waterfall to compensate for avg input level.

			float y1 = (0.25f + 2.5f / dblMagAvg) * log10f(0.01 + dblMag[i]);
			int objColor;

			// Set the pixel color based on the intensity (log) of the spectral line
			if (y1 > 6.5)
				objColor = Orange; // Strongest spectral line 
			else if (y1 > 6)
				objColor = Khaki;
			else if (y1 > 5.5)
				objColor = Cyan;
			else if (y1 > 5)
				objColor = DeepSkyBlue;
			else if (y1 > 4.5)
				objColor = RoyalBlue;
			else if (y1 > 4)
				objColor = Navy;
			else
				objColor = Black;

			if (i == 102)
				Waterfall[i] = Tomato;  // 1500 Hz line (center)
			else if (i == intTuneLineLow || i == intTuneLineLow - 1 || i == intTuneLineHi || i == intTuneLineHi + 1)
				Waterfall[i] = clrTLC;
			else
				Waterfall[i] = objColor; // ' Else plot the pixel as received
		}

		// Send Signal level and Busy indicator to save extra packets

		Waterfall[206] = CurrentLevel;
		Waterfall[207] = blnBusyStatus;

		doWaterfall(Waterfall);
#endif
	}
	else if (SpectrumActive)
	{
#ifdef PLOTSPECTRUM
		// This performs an auto scaling mechansim with fast attack and slow release
		if (dblMagMin / dblMagMax < 0.0001) // more than 10000:1 difference Max:Min
			dblMaxScale = max(dblMagMax, dblMaxScale * 0.9f);
		else
			dblMaxScale = max(10000 * dblMagMin, dblMagMax);

//		clearDisplay();

		for (i = 0; i < 206; i++)
		{
			// The following provides some AGC over the spectrum to compensate for avg input level.

			float y1 = -0.25f * (SpectrumHeight - 1) *  log10f((max(dblMagSpectrum[i], dblMaxScale / 10000)) / dblMaxScale); // ' range should be 0 to bmpSpectrumHeight -1
			int objColor = Yellow;

			Waterfall[i] = round(y1);
		}

		// Send Signal level and Busy indicator to save extra packets

		Waterfall[206] = CurrentLevel;
		Waterfall[207] = blnBusyStatus;
		Waterfall[208] = intTuneLineLow;
		Waterfall[209] = intTuneLineHi;

//		SendtoGUI('X', Waterfall, 210);
#endif
	}
}

*/

extern short rawSamples[2400];	// Get Frame Type need 2400 and we may add 1200
int rawSamplesLength = 0;
extern int maxrawSamplesLength;

void ProcessNewSamples(short * Samples, int nSamples)
{
	if (SoundIsPlaying == FALSE && UDPSoundIsPlaying == FALSE)
		BufferFull(Samples, nSamples);
};

void doCalib(int Chan, int Act)
{
	if (Chan == 0 && calib_mode[1])
		return;						
	
	if (Chan == 1 && calib_mode[0])
		return;

	calib_mode[Chan] = Act;

	if (Act == 0)
	{
		tx_status[Chan] = TX_SILENCE;		// Stop TX
		Flush();
		RadioPTT(Chan, 0);
		Debugprintf("Stop Calib");
	}
}

int Freq_Change(int Chan, int Freq)
{
	int low, high;

	low = round(rx_shift[1] / 2 + RCVR[Chan] * rcvr_offset[Chan] + 1);
	high = round(RX_Samplerate / 2 - (rx_shift[Chan] / 2 + RCVR[Chan] * rcvr_offset[Chan]));

	if (Freq < low)
		return rx_freq[Chan];				// Dont allow change

	if (Freq > high)
		return rx_freq[Chan];				// Dont allow change

	rx_freq[Chan] = Freq;
	tx_freq[Chan] = Freq;

	pnt_change[Chan] = TRUE;
	wf_pointer(soundChannel[Chan]);

	return Freq;
}

void MainLoop()
{
	// Called by background thread every 10 ms (maybe)

	// Actually we may have two cards
	
	// Original only allowed one channel per card.
	// I think we should be able to run more, ie two or more
	// modems on same soundcard channel
	
	// So All the soundcard stuff will need to be generalised

	if (UDPServ)
		UDPPollReceivedSamples();

	if (SoundMode == 3)
		UDPPollReceivedSamples();
	else
		PollReceivedSamples();


	if (modem_mode[0] == MODE_ARDOP)
	{
		chk_dcd1(0, 512);
	}

	DoTX(0);
	DoTX(1);
	DoTX(2);
	DoTX(3);

}

int ARDOPSendToCard(int Chan, int Len)
{
	// Send Next Block of samples to the soundcard

	short * in = &ARDOPTXBuffer[Chan][ARDOPTXPtr[Chan]];		// Enough to hold whole frame of samples
	short * out = DMABuffer;

	int LR = modemtoSoundLR[Chan];

	int i;

	for (i = 0; i < Len; i++)
	{
		if (SCO)			// Single Channel Output - same to both L and R
		{
			*out++ = *in;
			*out++ = *in++;
		}
		else
		{
			if (LR)				// Right
			{
				*out++ = 0;
				*out++ = *in++;
			}
			else
			{
				*out++ = *in++;
				*out++ = 0;
			}
		}
	}
	DMABuffer = SendtoCard(DMABuffer, Len);

	ARDOPTXPtr[Chan] += Len;

	// See if end of buffer

	if (ARDOPTXPtr[Chan] > ARDOPTXLen[Chan])
		return 1;

	return 0;
}
void DoTX(int Chan)
{
	// This kicks off a send sequence or calibrate

//	printtick("dotx");

	if (calib_mode[Chan])
	{
		// Maybe new calib or continuation

		if (pnt_change[Chan])
		{
			make_core_BPF(Chan, rx_freq[Chan], bpf[Chan]);
			make_core_TXBPF(Chan, tx_freq[Chan], txbpf[Chan]);
			pnt_change[Chan] = FALSE;
		}
		
		// Note this may block in SendtoCard

		modulator(Chan, tx_bufsize);
		return;
	}

	// I think we have to detect NO_DATA here and drop PTT and return to SILENCE

	if (tx_status[Chan] == TX_NO_DATA)
	{
		Flush();
		Debugprintf("TX Complete");
		RadioPTT(0, 0);
		tx_status[Chan] = TX_SILENCE;

		// We should now send any ackmode acks as the channel is now free for dest to reply

		sendAckModeAcks(Chan);
	}

	if (tx_status[Chan] != TX_SILENCE)
	{
		// Continue the send

		if (modem_mode[Chan] == MODE_ARDOP)
		{
//			if (SeeIfCardBusy())
//				return 0;

			if (ARDOPSendToCard(Chan, SendSize) == 1)
			{
				// End of TX

				Number = 0;
				Flush();

				// See if more to send. If so, don't drop PTT

				if (all_frame_buf[Chan].Count)
				{
					SoundIsPlaying = TRUE;
					Number = 0;

					Debugprintf("TX Continuing");

					string * myTemp = Strings(&all_frame_buf[Chan], 0);			// get message
					string * tx_data;

					if ((myTemp->Data[0] & 0x0f) == 12)			// ACKMODE
					{
						// Save copy then copy data up 3 bytes

						Add(&KISS_acked[Chan], duplicateString(myTemp));

						mydelete(myTemp, 0, 3);
						myTemp->Length -= sizeof(void *);
					}
					else
					{
						// Just remove control 

						mydelete(myTemp, 0, 1);
					}

					tx_data = duplicateString(myTemp);		// so can free original below

					Delete(&all_frame_buf[Chan], 0);			// This will invalidate temp

					AGW_AX25_frame_analiz(Chan, FALSE, tx_data);

					put_frame(Chan, tx_data, "", TRUE, FALSE);

					PktARDOPEncode(tx_data->Data, tx_data->Length - 2, Chan);

					freeString(tx_data);

					// Samples are now in DMABuffer = Send first block

					ARDOPSendToCard(Chan, SendSize);
					tx_status[Chan] = TX_FRAME;
					return;
				}

				Debugprintf("TX Complete");
				RadioPTT(0, 0);
				tx_status[Chan] = TX_SILENCE;

				// We should now send any ackmode acks as the channel is now free for dest to reply
			}

			return;
		}

		modulator(Chan, tx_bufsize); 
		return;
	}

	if (SoundIsPlaying || UDPSoundIsPlaying)
		return;

	// Not doing anything so see if we have anything new to send

	// See if frequency has changed

	if (pnt_change[Chan])
	{
		make_core_BPF(Chan, rx_freq[Chan], bpf[Chan]);
		make_core_TXBPF(Chan, tx_freq[Chan], txbpf[Chan]);
		pnt_change[Chan] = FALSE;
	}

	// See if we need an RSID

	if (needRSID[Chan])
	{
		needRSID[Chan] = 0;

		// Note this may block in SampleSink

		Debugprintf("Sending RSID");
		sendRSID(Chan, all_frame_buf[Chan].Count == 0);
		return;
	}

	if (all_frame_buf[Chan].Count == 0)
		return;

	// Start a new send. modulator should handle TXD etc

	Debugprintf("TX Start");
	SampleNo = 0;

	SoundIsPlaying = TRUE;
	RadioPTT(Chan, 1);
	Number = 0;

	if (modem_mode[Chan] == MODE_ARDOP)
	{
		// I think ARDOP will have to generate a whole frame of samples
		// then send them out a bit at a time to avoid stopping here for
		// possibly 10's of seconds

		// Can do this here as unlike normal ardop we don't need to run on Teensy
		// to 12000 sample rate we need either 24K or 48K per second, depending on
		// where we do the stereo mux. 

		// Slowest rate is 50 baud, so a 255 byte packet would take about a minute
		// allowing for RS overhead. Not really realistic put perhaps should be possible.
		// RAM isn't an issue so maybe allocate 2 MB. 

		// ?? Should we allow two ARDOP modems - could make sense if we can run sound
		// card channels independently

		string * myTemp = Strings(&all_frame_buf[Chan], 0);			// get message
		string * tx_data;

		if ((myTemp->Data[0] & 0x0f) == 12)			// ACKMODE
		{
			// Save copy then copy data up 3 bytes

			Add(&KISS_acked[Chan], duplicateString(myTemp));

			mydelete(myTemp, 0, 3);
			myTemp->Length -= sizeof(void *);
		}
		else
		{
			// Just remove control 

			mydelete(myTemp, 0, 1);
		}

		tx_data = duplicateString(myTemp);		// so can free original below

		Delete(&all_frame_buf[Chan], 0);			// This will invalidate temp

		AGW_AX25_frame_analiz(Chan, FALSE, tx_data);

		put_frame(Chan, tx_data, "", TRUE, FALSE);

		PktARDOPEncode(tx_data->Data, tx_data->Length - 2, Chan);

		freeString(tx_data);

		// Samples are now in DMABuffer = Send first block

		ARDOPSendToCard(Chan, SendSize);
		tx_status[Chan] = TX_FRAME;

	}
	else
		modulator(Chan, tx_bufsize);

	return;
}

void stoptx(int snd_ch)
{
	Flush();
	Debugprintf("TX Complete");
	RadioPTT(snd_ch, 0);
	tx_status[snd_ch] = TX_SILENCE;

	snd_status[snd_ch] = SND_IDLE;
}

void RX2TX(int snd_ch)
{
	if (snd_status[snd_ch] == SND_IDLE)
	{
		DoTX(snd_ch);
	}
}

// PTT Stuff

int hPTTDevice = 0;
char PTTPort[80] = "";			// Port for Hardware PTT - may be same as control port.
int PTTBAUD = 19200;
int PTTMode = PTTRTS;			// PTT Control Flags.

char PTTOnString[128] = "";
char PTTOffString[128] = "";

UCHAR PTTOnCmd[64];
UCHAR PTTOnCmdLen = 0;

UCHAR PTTOffCmd[64];
UCHAR PTTOffCmdLen = 0;

int pttGPIOPin = 17;			// Default
int pttGPIOPinR = 17;
BOOL pttGPIOInvert = FALSE;
BOOL useGPIO = FALSE;
BOOL gotGPIO = FALSE;

int HamLibPort = 4532;
char HamLibHost[32] = "192.168.1.14";

char CM108Addr[80] = "";

int VID = 0;
int PID = 0;

// CM108 Code

char * CM108Device = NULL;

void DecodeCM108(char * ptr)
{
	// Called if Device Name or PTT = Param is CM108

#ifdef WIN32

	// Next Param is VID and PID - 0xd8c:0x8 or Full device name
	// On Windows device name is very long and difficult to find, so 
	//	easier to use VID/PID, but allow device in case more than one needed

	char * next;
	long VID = 0, PID = 0;
	char product[256] = "Unknown";

	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;

	if (strlen(ptr) > 16)
		CM108Device = _strdup(ptr);
	else
	{
		VID = strtol(ptr, &next, 0);
		if (next)
			PID = strtol(++next, &next, 0);

		// Look for Device

		devs = hid_enumerate((unsigned short)VID, (unsigned short)PID);
		cur_dev = devs;

		while (cur_dev)
		{
			if (cur_dev->product_string)
				wcstombs(product, cur_dev->product_string, 255);
			
			Debugprintf("HID Device %s VID %X PID %X", product, cur_dev->vendor_id, cur_dev->product_id);
			if (cur_dev->vendor_id == VID && cur_dev->product_id == PID)
			{
				path_to_open = cur_dev->path;
				break;
			}
			cur_dev = cur_dev->next;
		}

		if (path_to_open)
		{
			handle = hid_open_path(path_to_open);

			if (handle)
			{
				hid_close(handle);
				CM108Device = _strdup(path_to_open);
			}
			else
			{
				Debugprintf("Unable to open CM108 device %x %x", VID, PID);
			}
		}
		else
			Debugprintf("Couldn't find CM108 device %x %x", VID, PID);

		hid_free_enumeration(devs);
	}
#else

	// Linux - Next Param HID Device, eg /dev/hidraw0

	CM108Device = _strdup(ptr);
#endif
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}

void OpenPTTPort()
{
	PTTMode &= ~PTTCM108;
	PTTMode &= ~PTTHAMLIB;

	if (PTTPort[0] && strcmp(PTTPort, "None") != 0)
	{
		if (PTTMode == PTTCAT)
		{
			// convert config strings from Hex

			char * ptr1 = PTTOffString;
			UCHAR * ptr2 = PTTOffCmd;
			char c;
			int val;

			while (c = *(ptr1++))
			{
				val = c - 0x30;
				if (val > 15) val -= 7;
				val <<= 4;
				c = *(ptr1++) - 0x30;
				if (c > 15) c -= 7;
				val |= c;
				*(ptr2++) = val;
			}

			PTTOffCmdLen = ptr2 - PTTOffCmd;

			ptr1 = PTTOnString;
			ptr2 = PTTOnCmd;

			while (c = *(ptr1++))
			{
				val = c - 0x30;
				if (val > 15) val -= 7;
				val <<= 4;
				c = *(ptr1++) - 0x30;
				if (c > 15) c -= 7;
				val |= c;
				*(ptr2++) = val;
			}

			PTTOnCmdLen = ptr2 - PTTOnCmd;
		}

		if (stricmp(PTTPort, "GPIO") == 0)
		{
			// Initialise GPIO for PTT if available

#ifdef __ARM_ARCH

			if (gpioInitialise() == 0)
			{
				printf("GPIO interface for PTT available\n");
				gotGPIO = TRUE;

				SetupGPIOPTT();
			}
			else
				printf("Couldn't initialise GPIO interface for PTT\n");

#else
			printf("GPIO interface for PTT not available on this platform\n");
#endif

		}
		else if (stricmp(PTTPort, "CM108") == 0)
		{
			DecodeCM108(CM108Addr);
			PTTMode |= PTTCM108;
		}

		else if (stricmp(PTTPort, "HAMLIB") == 0)
		{
			PTTMode |= PTTHAMLIB;
			HAMLIBSetPTT(0);			// to open port
			return;
		}

		else		//  Not GPIO
		{
			hPTTDevice = OpenCOMPort(PTTPort, PTTBAUD, FALSE, FALSE, FALSE, 0);
		}
	}
}

void ClosePTTPort()
{
	CloseCOMPort(hPTTDevice);
	hPTTDevice = 0;
}
void CM108_set_ptt(int PTTState)
{
	char io[5];
	hid_device *handle;
	int n;

	io[0] = 0;
	io[1] = 0;
	io[2] = 1 << (3 - 1);
	io[3] = PTTState << (3 - 1);
	io[4] = 0;

	if (CM108Device == NULL)
		return;

#ifdef WIN32
	handle = hid_open_path(CM108Device);

	if (!handle) {
		printf("unable to open device\n");
		return;
	}

	n = hid_write(handle, io, 5);
	if (n < 0)
	{
		printf("Unable to write()\n");
		printf("Error: %ls\n", hid_error(handle));
	}

	hid_close(handle);

#else

	int fd;

	fd = open(CM108Device, O_WRONLY);

	if (fd == -1)
	{
		printf("Could not open %s for write, errno=%d\n", CM108Device, errno);
		return;
	}

	io[0] = 0;
	io[1] = 0;
	io[2] = 1 << (3 - 1);
	io[3] = PTTState << (3 - 1);
	io[4] = 0;

	n = write(fd, io, 5);
	if (n != 5)
	{
		printf("Write to %s failed, n=%d, errno=%d\n", CM108Device, n, errno);
	}

	close(fd);
#endif
	return;

}



void RadioPTT(int snd_ch, BOOL PTTState)
{
#ifdef __ARM_ARCH
	if (useGPIO)
	{
		if (DualPTT && modemtoSoundLR[snd_ch] == 1)
			gpioWrite(pttGPIOPinR, (pttGPIOInvert ? (1 - PTTState) : (PTTState)));
		else
			gpioWrite(pttGPIOPin, (pttGPIOInvert ? (1 - PTTState) : (PTTState)));

		return;
	}

#endif

	if ((PTTMode & PTTCM108))
	{
		CM108_set_ptt(PTTState);
		return;
	}
	
	if ((PTTMode & PTTHAMLIB))
	{
		HAMLIBSetPTT(PTTState);
		return;
	}
	if (hPTTDevice == 0)
		return;

	if ((PTTMode & PTTCAT))
	{
		if (PTTState)
			WriteCOMBlock(hPTTDevice, PTTOnCmd, PTTOnCmdLen);
		else
			WriteCOMBlock(hPTTDevice, PTTOffCmd, PTTOffCmdLen);

		return;
	}

	if (DualPTT && modemtoSoundLR[snd_ch] == 1)		// use DTR
	{
		if (PTTState)
			COMSetDTR(hPTTDevice);
		else
			COMClearDTR(hPTTDevice);
	}
	else
	{
		if ((PTTMode & PTTRTS))
		{
			if (PTTState)
				COMSetRTS(hPTTDevice);
			else
				COMClearRTS(hPTTDevice);
		}
	}

}

char ShortDT[] = "HH:MM:SS";

char * ShortDateTime()
{
	struct tm * tm;
	time_t NOW = time(NULL);

	tm = gmtime(&NOW);

	sprintf(ShortDT, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	return ShortDT;
}


// Reed Solomon Stuff


int NPAR = -1;	// Number of Parity Bytes - used in RS Code

int xMaxErrors = 0;

int RSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen)
{
	// This just returns the Parity Bytes. I don't see the point
	// in copying the message about

	unsigned char Padded[256];		// The padded Data

	int Length = DataLen + RSLen;	// Final Length of packet
	int PadLength = 255 - Length;	// Padding bytes needed for shortened RS codes

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	if (NPAR != RSLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = RSLen;
		xMaxErrors = NPAR / 2;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen);

	encode_data(Padded, 255 - RSLen, RSBytes);

	return RSLen;
}

//	Main RS decode function

extern int index_of[];
extern int recd[];
extern int Corrected[256];
extern int tt;		//  number of errors that can be corrected 
extern int kk;		// Info Symbols

extern BOOL blnErrorsCorrected;


BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK)
{


	// Using a modified version of Henry Minsky's code

	//Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009

	// Rick's Implementation processes the byte array in reverse. and also 
	//	has the check bytes in the opposite order. I've modified the encoder
	//	to allow for this, but so far haven't found a way to mske the decoder
	//	work, so I have to reverse the data and checksum to decode G8BPQ Nov 2015

	//	returns TRUE if was ok or correction succeeded, FALSE if correction impossible

	UCHAR intTemp[256];				// WOrk Area to pass to Decoder		
	int i;
	UCHAR * ptr2 = intTemp;
	UCHAR * ptr1 = &bytRcv[Length - CheckLen - 1]; // Last Byte of Data

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;

	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		xMaxErrors = NPAR / 2;

		initialize_ecc();
	}


	//	We reverse the data while zero padding it to speed things up

	//	We Need (Data Reversed) (Zero Padding) (Checkbytes Reversed)

	// Reverse Data

	for (i = 0; i < DataLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	//	Clear padding

	memset(ptr2, 0, PadLength);

	ptr2 += PadLength;

	// Error Bits

	ptr1 = &bytRcv[Length - 1];			// End of check bytes

	for (i = 0; i < CheckLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	decode_data(intTemp, 255);

	// check if syndrome is all zeros 

	if (check_syndrome() == 0)
	{
		// RS ok, so no need to correct

		*blnRSOK = TRUE;
		return TRUE;		// No Need to Correct
	}

	if (correct_errors_erasures(intTemp, 255, 0, 0) == 0) // Dont support erasures at the momnet

		// Uncorrectable

		return FALSE;

	// Data has been corrected, so need to reverse again

	ptr1 = &intTemp[DataLen - 1];
	ptr2 = bytRcv; // Last Byte of Data

	for (i = 0; i < DataLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	// ?? Do we need to return the check bytes ??

	// Yes, so we can redo RS Check on supposedly connected frame

	ptr1 = &intTemp[254];	// End of Check Bytes

	for (i = 0; i < CheckLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	return TRUE;
}

extern TStringList detect_list[5];
extern TStringList detect_list_c[5];

void ProcessPktFrame(int snd_ch, UCHAR * Data, int frameLen)
{
	string * pkt = newString();

	stringAdd(pkt, Data, frameLen + 2);			// 2 for crc (not actually there)

	analiz_frame(snd_ch, pkt, "ARDOP", 1);

}
