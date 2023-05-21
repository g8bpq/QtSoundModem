//
// Derived from fldigi rdid.cxx by John G8BPQ September 22
//
// ----------------------------------------------------------------------------
//
//	rsid.cxx
//
// Copyright (C) 2008-2012
//		Dave Freese, W1HKJ
// Copyright (C) 2009-2012
//		Stelios Bounanos, M0GLD
// Copyright (C) 2012
//		John Douyere, VK2ETA
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS for (A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------


#include <malloc.h>

#include "rsid.h"
#include <math.h>
#include "fftw3.h"

#include "UZ7HOStuff.h"

#define M_PI       3.1415926f

#define true 1
#define false 0

#define TRUE 1
#define FALSE 0

#define WORD unsigned int
#define BYTE unsigned char
#define byte unsigned char

void SampleSink(int LR, short Sample);
void Flush();
void extSetOffset(int rxOffset);
void mon_rsid(int snd_ch, char * RSID);

extern int RSID_SABM[4];
extern int RSID_UI[4];
extern int RSID_SetModem[4];

struct RSIDs {
	unsigned short rs;
	trx_mode mode;
	const char* name;
};

extern int SampleNo;
extern int Number;

int len;
int symlen;

#include "rsid_defs.cxx"

#define RSWINDOW 1


const int Squares[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	0, 2, 4, 6, 8,10,12,14, 9,11,13,15, 1, 3, 5, 7,
	0, 3, 6, 5,12,15,10, 9, 1, 2, 7, 4,13,14,11, 8,
	0, 4, 8,12, 9,13, 1, 5,11,15, 3, 7, 2, 6,10,14,
	0, 5,10,15,13, 8, 7, 2, 3, 6, 9,12,14,11, 4, 1,
	0, 6,12,10, 1, 7,13,11, 2, 4,14, 8, 3, 5,15, 9,
	0, 7,14, 9, 5, 2,11,12,10,13, 4, 3,15, 8, 1, 6,
	0, 8, 9, 1,11, 3, 2,10,15, 7, 6,14, 4,12,13, 5,
	0, 9,11, 2,15, 6, 4,13, 7,14,12, 5, 8, 1, 3,10,
	0,10,13, 7, 3, 9,14, 4, 6,12,11, 1, 5,15, 8, 2,
	0,11,15, 4, 7,12, 8, 3,14, 5, 1,10, 9, 2, 6,13,
	0,12, 1,13, 2,14, 3,15, 4, 8, 5, 9, 6,10, 7,11,
	0,13, 3,14, 6,11, 5, 8,12, 1,15, 2,10, 7, 9, 4,
	0,14, 5,11,10, 4,15, 1,13, 3, 8, 6, 7, 9, 2,12,
	0,15, 7, 8,14, 1, 9, 6, 5,10, 2,13,11, 4,12, 3
};

const int indices[] = {
	2, 4, 8, 9, 11, 15, 7, 14, 5, 10, 13, 3
};

int rmode, rmode2;

void Encode(int code, unsigned char *rsid)
{
	rsid[0] = code >> 8;
	rsid[1] = (code >> 4) & 0x0f;
	rsid[2] = code & 0x0f;
	for (int i = 3; i < RSID_NSYMBOLS; i++)
		rsid[i] = 0;
	for (int i = 0; i < 12; i++) {
		for (int j = RSID_NSYMBOLS - 1; j > 0; j--)
			rsid[j] = rsid[j - 1] ^ Squares[(rsid[j] << 4) + indices[i]];
		rsid[0] = Squares[(rsid[0] << 4) + indices[i]];
	}
}

//=============================================================================
// transmit rsid code for current mode
//=============================================================================

float sampd;
short samps;

// Each symbol is transmitted using MFSK modulation.There are 16 possibilities of frequencies separated by
// 11025 / 1024 = 10.766 Hz. Each symbol transmission being done on only one frequency for a duration equal
// to 1024 / 11025 x 1000 = 92.88 ms.The entire RSID sequence of 15 symbols is transmitted in 15 x 1024 / 11025 = 1393 ms.

// The analysis is based on a Fast Fourier transform of 2048 points at 11025 samples / sec, regularly done at each
// semi - step of time(46.44 ms).

// For each semi - step of time(46.44 ms) and for each semi - step of frequency(5.38 Hz),the program attempts to detect 
// an RSID extending for the last 1.393 seconds.So each second, about 8500 possible RSID
// (depending on the selected bandwidth) are tested

// But we are working at 12000 samples/sec so 92.88 mS = 1114.56 samples, so I think we run fft every 557.28 samples (46.44 ms)

// How do we get 5.28 Hz buckets at 12000? Can we run fft of length 2,272.727 (say 2273) length?

// Actually not sure we need to. We can interpolate freq and so long as we can get within a few Hz should be ok


// Spec says tone spacing ia 10.766 (11025 / 1024)

double toneinterval = RSID_SAMPLE_RATE / 1024;

// fftw library interface

static fftwf_complex * in = 0, *out;
static fftwf_plan p;

#define N 4096

short savedSamples[N + 1000];			// At least N + max input length (currently uses 512);
int savedSampLen = 0;

int firstBin = (300 * 2048) / 12000;	// Search Lowest (300 Hz)
int lastBin = (3000 * 2048) / 12000;;	// Seach Highest (3000 Hz)

double avmag;			// Average magnitude over spectrum

int	fft_buckets[RSID_NTIMES][RSID_FFT_SIZE];			// This seems to have last 30 sets of values

float aFFTAmpl[RSID_FFT_SIZE];							// Amplitude samples from fft

// Table of precalculated Reed Solomon symbols
unsigned char pCodes1[256][16];
unsigned char pCodes2[256][16];

int found1;
int found2;

double rsid_secondary_time_out;


int bPrevTimeSliceValid;
int	iPrevDistance;
int	iPrevBin;
int	iPrevSymbol;

int	fft_buckets[RSID_NTIMES][RSID_FFT_SIZE];

int	bPrevTimeSliceValid2;
int	iPrevDistance2;
int	iPrevBin2;
int	iPrevSymbol2;

int hamming_resolution = 2;

int needRSID[4] = { 0,0,0,0 };				// Request TX scheduler to send RSID

int needSetOffset[4] = { 0,0,0,0 };

void CalculateBuckets(const float *pSpectrum, int iBegin, int iEnd);
void Search();

void RSIDinitfft()
{
	unsigned char * c;

	in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);

	// Initialization  of assigned mode/submode IDs.

	for (int i = 0; i < rsid_ids_size1; i++)
	{
		c = &pCodes1[i][0];
		Encode(rsid_ids_1[i].rs, c);
	}
}

void reset()
{
	iPrevDistance = iPrevDistance2 = 99;
	bPrevTimeSliceValid = bPrevTimeSliceValid2 = false;
	found1 = found2 = false;
	rsid_secondary_time_out = 0;
}

// Compute fft of last  557 * 2 values and return bucket number of peak

static int dofft(short * inp, float * mags)
{
	int i;
	float mag;
	float max = 0;
	int maxindex = 0;

	memset(in, 0, sizeof(fftwf_complex) * N);
	avmag = 0;


	for (i = 0; i < 512 * 2; i++)
	{
		//		in[i][0] = inp[i] * 1.0f;

		// Hamming Window

		in[i][0] = inp[i];
		in[i][0] = (float)((0.53836f - (0.46164f * cos(2 * M_PI * i / (float)(557.0 * 2.0 - 1)))) * inp[i]);
		in[i][1] = 0;
	}

	fftwf_execute(p);

	for (i = firstBin; i < lastBin; i++)						// only need buckets up to 3000
	{
		// Convert Real/Imag to amplitude

		mag = powf(out[i][0], 2);
		mag += powf(out[i][1], 2);
		mag = sqrtf(mag);
		mags[i] = mag;
		avmag += mag;

		if (mag > max)
		{
			max = mag;
			maxindex = i;
		}
	}
	avmag /= (lastBin - firstBin);
	return maxindex;
}

void RSIDProcessSamples(short * Samples, int nSamples)
{
	// Add to saved samples, and if we have more than 557 run FFT and save remaining

	// We process last 557 + new 557 + zero padding

	// Trying with 512 @ 12000

	// savedSampLen is number of shorts not bytes

	if (in == 0)			// Not initialised
		return;

	memcpy(&savedSamples[savedSampLen], Samples, nSamples * sizeof(short));
	savedSampLen += nSamples;

	if (savedSampLen >= 512 * 2)				// Old + New
	{
		int peakBucket;

		peakBucket = dofft(savedSamples, aFFTAmpl);

		if (peakBucket > firstBin && peakBucket < lastBin)
		{
//			float freq;
//			freq = peakBucket * 12000.0f / 2048;
//			Debugprintf("%d %f %f %f", peakBucket, freq, aFFTAmpl[peakBucket], avmag);
		}

		savedSampLen -= 512;
		memmove(savedSamples, &savedSamples[512 * sizeof(short)], savedSampLen * sizeof(short));

		Search();
	}
}


int HammingDistance(int iBucket, unsigned char *p2)
{
	int dist = 0;

	for (int i = 0, j = 0; i < RSID_NSYMBOLS; i++, j += 2)
	{
		if (fft_buckets[j][iBucket] != p2[i])
		{
			++dist;
			if (dist > hamming_resolution)
				break;
		}
	}
	return dist;
}

int iDistanceMin = 1000;  // infinity

int search_amp(int *bin_out, int *symbol_out)
{
	int i, j;
	int iDistance = 1000;
	int iBin = -1;
	int iSymbol = -1;

	int tblsize;

	iDistanceMin = 1000;  // infinity

	tblsize = rsid_ids_size1;

	unsigned char *pc = 0;

	for (i = 0; i < tblsize; i++)
	{
		pc = &pCodes1[i][0];

		for (j = firstBin; j < lastBin - RSID_NTIMES; j++)
		{
			iDistance = HammingDistance(j, pc);

			if (iDistance < iDistanceMin)
			{
				iDistanceMin = iDistance;
				iSymbol = i;
				iBin = j;
				if (iDistanceMin == 0) break;
			}
		}
	}

	if (iDistanceMin <= hamming_resolution)
	{
		*symbol_out = iSymbol;
		*bin_out = iBin;
		return true;
	}

	return false;
}

void apply(int iBin, int iSymbol)
{
	// Does something with the found id

	const struct RSIDs *prsid = &rsid_ids_1[iSymbol];
	int Freq = (int)(iBin + 15) * 12000.0f / 2048;
	char Msg[128];
	int Offset = Freq - rx_freq[0];
	int i;
	int nearest = -1, minOffset = 9999, absOffset;

	// If there is more than one decoder with update from rsid set update the nearest

	for (i = 0; i < 4; i++)
	{
		if (RSID_SetModem[i])
		{
			absOffset = abs(Freq - rx_freq[i]);

			if (absOffset < minOffset)
			{
				// Nearer

				nearest = i;
				minOffset = absOffset;
			}	
		}
	}

	// We don't run this unless at least one modem has RSID_SetModem set.

	Offset = Freq - rx_freq[nearest];

	sprintf(Msg, "RSID %s %d %d Nearest Modem %c Offset %d", prsid->name, iDistanceMin, Freq, nearest + 'A', Offset);
	
	mon_rsid(0, Msg);

		// Set Modem RX Offset to match received freq

	chanOffset[nearest] = Offset;
	needSetOffset[nearest] = 1;				// Update GUI

}


void Search()
{
	int symbol = -1;
	int bin = -1;
	
	// We have just calculated a new set of fft amplitude bins in aFFTAmpl 

	// we find peak bin, and store in fft_buckets array. This has 30 sets of 1024 buckets (though we only use first 512, as we limit search to 3 KHz)

	// We move previous 29 entries to start of array and add new values on end so samples corresponding to first bit of rsid msg are at start

	memmove(fft_buckets, &(fft_buckets[1][0]), (RSID_NTIMES - 1) * RSID_FFT_SIZE * sizeof(int));
	memset(&(fft_buckets[RSID_NTIMES - 1][0]), 0, RSID_FFT_SIZE * sizeof(int));

	// We process even then odd bins, using alternate bins to get resolution to 1/2 bin

	CalculateBuckets(aFFTAmpl, firstBin, lastBin - RSID_NTIMES);
	CalculateBuckets(aFFTAmpl, firstBin + 1, lastBin - RSID_NTIMES);

	// Now have 30 sets of 512 bit valies (0-15). We look for a set of 15 that match an ID

	found1 = search_amp(&bin, &symbol);

	if (found1)
	{
		apply(bin, symbol);
		reset();
	}
}



void CalculateBuckets(const float *pSpectrum, int iBegin, int iEnd)
{
	float Amp = 0.0f, AmpMax = 0.0f;

	int iBucketMax = iBegin - 2;
	int j;

	// This searches odd and even pairs of amps, hence the += 2

	for (int i = iBegin; i < iEnd; i += 2)
	{
		if (iBucketMax == i - 2)
		{
			// if max was first in grooup of 15 redo full search

			AmpMax = pSpectrum[i];
			iBucketMax = i;
			for (j = i + 2; j < i + RSID_NTIMES + 2; j += 2)
			{
				Amp = pSpectrum[j];
				if (Amp > AmpMax)
				{
					AmpMax = Amp;
					iBucketMax = j;
				}
			}
		}
		else
		{
			// Max wasn't first, so must be in next 14, so we can just check if new last is > max

			j = i + RSID_NTIMES;
			Amp = pSpectrum[j];
			if (Amp > AmpMax)
			{
				AmpMax = Amp;
				iBucketMax = j;
			}
		}
		fft_buckets[RSID_NTIMES - 1][i] = (iBucketMax - i) >> 1;
	}
}








void sendRSID(int Chan, int dropTX)
{
	unsigned char rsid[RSID_NSYMBOLS];
	float sr;
	int iTone;
	float freq, phaseincr;
	float fr;
	float phase;

	int Mode = speed[Chan];
	int Freq = rx_freq[Chan];

	rmode2 = 687;
	rmode = 35 + Mode;				// Packet 300 or 1200

	Encode(rmode, rsid);

	sr = 12000;
	symlen = (size_t)floor(RSID_SYMLEN * sr);

	SampleNo = 0;

	SoundIsPlaying = TRUE;
	RadioPTT(Chan, 1);
	Number = 0;

	// transmit 5 symbol periods of silence at beginning of rsid

	for (int i = 0; i < 5 * symlen; i++)
		SampleSink(0, 0);

	// transmit sequence of 15 symbols (tones)

	fr = 1.0f * Freq - (RSID_SAMPLE_RATE * 7 / 1024);
	phase = 0.0f;

	for (int i = 0; i < 15; i++)
	{
		iTone = rsid[i];
		freq = fr + iTone * RSID_SAMPLE_RATE / 1024;
		phaseincr = 2.0f * M_PI * freq / sr;

		for (int j = 0; j < symlen; j++)
		{
			phase += phaseincr;
			if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;

			sampd = sinf(phase);
			sampd = sampd * 32767.0f;
			samps = (short)sampd;
			SampleSink(0, samps);
		}
	}

	// 5 symbol periods of silence at end of transmission
	// and between RsID and the data signal
	int nperiods = 5;

	for (int i = 0; i < nperiods * symlen; i++)
		SampleSink(modemtoSoundLR[Chan], 0);

	tx_status[Chan] = TX_SILENCE;		// Stop TX
	Flush();
	if (dropTX)
		RadioPTT(Chan, 0);
}

// Experimental Busy Detect, based on ARDOP code


static int LastBusyCheck = 0;
static int BusyCount = 0;
static int BusyStatus = 0;
static int LastBusyStatus = 0;

static int BusyDet = 5;				// DCD Threshold
int LastStart = 0;
int LastStop = 0;
extern int LastBusyOn;
extern int LastBusyOff;

static int LastBusy = FALSE;

extern float dblAvgStoNSlowNarrow;
extern float dblAvgStoNFastNarrow;
extern float dblAvgStoNSlowWide;
extern float dblAvgStoNFastWide;
int BusyOnCnt = 0;  // used to filter Busy ON detections 
int BusyOffCnt = 0; // used to filter Busy OFF detections 
unsigned int LastBusyTrip = 0;
unsigned int PriorLastBusyTrip = 0;
unsigned int LastBusyClear = 0;
unsigned int LastTrip;

void SortSignals(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin);
void SortSignals2(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin);

static BOOL BusyDetect3(float * dblMag, int intStart, int intStop)        // this only called while searching for leader ...once leader detected, no longer called.
{
	// each bin is about 12000/2048 or 5.86 Hz
	// First sort signals and look at highes signals:baseline ratio..

	float dblAVGSignalPerBinNarrow, dblAVGSignalPerBinWide, dblAVGBaselineNarrow, dblAVGBaselineWide;
	float dblSlowAlpha = 0.2f;
	float dblAvgStoNNarrow = 0, dblAvgStoNWide = 0;
	int intNarrow = 16;  // 16 x  5.86 about 94 z
	int intWide = ((intStop - intStart) * 2) / 3; //* 0.66);
	int blnBusy = FALSE;
	int  BusyDet4th = BusyDet * BusyDet * BusyDet * BusyDet;
	int BusyDet = 5;
	unsigned int HoldMs = 5000;

	// First sort signals and look at highest signals:baseline ratio..
	// First narrow band (~94Hz)

	SortSignals2(dblMag, intStart, intStop, intNarrow, &dblAVGSignalPerBinNarrow, &dblAVGBaselineNarrow);

	if (LastStart == intStart && LastStop == intStop)
		dblAvgStoNNarrow = (1 - dblSlowAlpha) * dblAvgStoNNarrow + dblSlowAlpha * dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
	else
	{
		// This initializes the Narrow average after a bandwidth change

		dblAvgStoNNarrow = dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
		LastStart = intStart;
		LastStop = intStop;
	}

	// Wide band (66% of current bandwidth)

	SortSignals2(dblMag, intStart, intStop, intWide, &dblAVGSignalPerBinWide, &dblAVGBaselineWide);

	if (LastStart == intStart && LastStop == intStop)
		dblAvgStoNWide = (1 - dblSlowAlpha) * dblAvgStoNWide + dblSlowAlpha * dblAVGSignalPerBinWide / dblAVGBaselineWide;
	else
	{
		// This initializes the Wide average after a bandwidth change

		dblAvgStoNWide = dblAVGSignalPerBinWide / dblAVGBaselineWide;
		LastStart = intStart;
		LastStop = intStop;
	}

	// Preliminary calibration...future a function of bandwidth and BusyDet.

	blnBusy = (dblAvgStoNNarrow > (3 + 0.008 * BusyDet4th)) || (dblAvgStoNWide > (5 + 0.016 * BusyDet4th));

	if (BusyDet == 0)
		blnBusy = FALSE;		// 0 Disables check ?? Is this the best place to do this?

//	WriteDebugLog(LOGDEBUG, "Busy %d Wide %f Narrow %f", blnBusy, dblAvgStoNWide, dblAvgStoNNarrow); 

	if (blnBusy)
	{
		// This requires multiple adjacent busy conditions to skip over one nuisance Busy trips. 
		// Busy must be present at least 3 consecutive times ( ~250 ms) to be reported

		BusyOnCnt += 1;
		BusyOffCnt = 0;
		if (BusyOnCnt > 3)
			LastTrip = Now;
	}
	else
	{
		BusyOffCnt += 1;
		BusyOnCnt = 0;
	}

	if (LastBusy == 0 && BusyOnCnt >= 3)
	{
		PriorLastBusyTrip = LastBusyTrip;  // save old dttLastBusyTrip for use in BUSYBLOCKING function
		LastBusyTrip = Now;
		LastBusy = TRUE;
	}
	else
	{
		if (LastBusy && (Now - LastTrip) > HoldMs && BusyOffCnt >= 3)
		{
			LastBusyClear = Now;
			LastBusy = FALSE;
		}
	}
	return LastBusy;
}


static void UpdateBusyDetector()
{
	// Use applitude bins in aFFTAmpl

	float dblMagAvg = 0;
	int intTuneLineLow, intTuneLineHi;
	int i;
	int BusyFlag;

	if (Now - LastBusyCheck < 100)
		return;

	LastBusyCheck = Now;

	for (i = 52; i < 512; i++)
	{
		//	starting at ~300 Hz to ~3000 Hz Which puts the center of the signal in the center of the window (~1500Hz)

		dblMagAvg += aFFTAmpl[i];
	}

	intTuneLineLow = 52;
	intTuneLineHi = 512;

	BusyFlag = BusyDetect3(aFFTAmpl, intTuneLineLow, intTuneLineHi);

	if (BusyFlag == 0)
	{
		if (BusyCount == 0)
			BusyStatus = 0;
		else
			BusyCount--;
	}
	else
	{
		BusyStatus = 1;
		BusyCount = 10;			// Try delaying busy off a bit
	}

	if (BusyStatus && !LastBusyStatus)
	{
		Debugprintf("BUSY TRUE");
	}
	//    stcStatus.Text = "True"
		//    queTNCStatus.Enqueue(stcStatus)
		//    'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

	else if (LastBusyStatus && !BusyStatus)
	{
		Debugprintf("BUSY FALSE");
	}

	LastBusyStatus = BusyStatus;

}

