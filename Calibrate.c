
#include "UZ7HOStuff.h"

// if in Satellite Mode look for a Tuning signal

// As a first try, use ardop leader pattern then single tone

static short rawSamples[2400];	// Get Frame Type need 2400 and we may add 1200
static int rawSamplesLength = 0;
static int maxrawSamplesLength;
static float dblOffsetHz = 0;;
static int blnLeaderFound = 0;

enum _ReceiveState		// used for initial receive testing...later put in correct protocol states
{
	SearchingForLeader,
	AcquireSymbolSync,
	AcquireFrameSync,
	AcquireFrameType,
	DecodeFrameType,
	AcquireFrame,
	DecodeFramestate
};

static enum _ReceiveState State;


void LookForCalPattern(short * Samples, int nSamples);

void doTuning(short * Samples, int nSamples)
{
	short ardopbuff[2][1200];
	int i, i1 = 0;

	if (UsingBothChannels)
	{
		for (i = 0; i < rx_bufsize; i++)
		{
			ardopbuff[0][i] = Samples[i1];
			i1++;
			ardopbuff[1][i] = Samples[i1];
			i1++;
		}
	}
	else if (UsingRight)
	{
		// Extract just right

		i1 = 1;

		for (i = 0; i < rx_bufsize; i++)
		{
			ardopbuff[1][i] = Samples[i1];
			i1 += 2;
		}
	}
	else
	{
		// Extract just left

		for (i = 0; i < rx_bufsize; i++)
		{
			ardopbuff[0][i] = Samples[i1];
			i1 += 2;
		}
	}

	if (UsingLeft)
	{
		LookForCalPattern(&ardopbuff[0][0], 0);
	}

	if (UsingRight)
	{
		LookForCalPattern(&ardopbuff[0][0], 1);
	}

}




void LookForCalPattern(short * Samples, int nSamples)
{
	BOOL blnFrameDecodedOK = FALSE;

	//	LookforUZ7HOLeader(Samples, nSamples);

	//	printtick("Start afsk");
	//	DemodAFSK(Samples, nSamples);
	//	printtick("End afsk");

	//	return;

	// Append new data to anything in rawSamples

	memcpy(&rawSamples[rawSamplesLength], Samples, nSamples * 2);
	rawSamplesLength += nSamples;

	if (rawSamplesLength > maxrawSamplesLength)
		maxrawSamplesLength = rawSamplesLength;

	if (rawSamplesLength >= 2400)
		Debugprintf("Corrupt rawSamplesLength %d", rawSamplesLength);


	nSamples = rawSamplesLength;
	Samples = rawSamples;

	rawSamplesLength = 0;

	//	printtick("Start Busy");
	if (nSamples >= 1024)
		UpdateBusyDetector(Samples);
	//	printtick("Done Busy");

		// it seems that searchforleader runs on unmixed and unfilered samples

		// Searching for leader

	if (State == SearchingForLeader)
	{
		// Search for leader as long as 960 samples (8  symbols) available

//		printtick("Start Leader Search");

		while (State == SearchingForLeader && nSamples >= 1200)
		{
			int intSN;

			blnLeaderFound = SearchFor2ToneLeader4(Samples, nSamples, &dblOffsetHz, &intSN);
			//			blnLeaderFound = SearchFor2ToneLeader2(Samples, nSamples, &dblOffsetHz, &intSN);

			if (blnLeaderFound)
			{
				//				Debugprintf("Got Leader");

				nSamples -= 480;
				Samples += 480;		// !!!! needs attention !!!
			}
			else
			{
				nSamples -= 240;
				Samples += 240;		// !!!! needs attention !!!
			}
		}
		if (State == SearchingForLeader)
		{
			// Save unused samples

			memmove(rawSamples, Samples, nSamples * 2);
			rawSamplesLength = nSamples;

			//			printtick("End Leader Search");

			return;
		}
	}
}
