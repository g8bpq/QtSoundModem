

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#endif

#include "ARDOPC.h"

VOID SortSignals2(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin);

int LastBusyOn;
int LastBusyOff;

BOOL blnLastBusy = FALSE;

float dblAvgStoNSlowNarrow;
float dblAvgStoNFastNarrow;
float dblAvgStoNSlowWide;
float dblAvgStoNFastWide;
int intLastStart = 0;
int intLastStop  = 0;
int intBusyOnCnt  = 0;  // used to filter Busy ON detections 
int intBusyOffCnt  = 0; // used to filter Busy OFF detections 
int dttLastBusyTrip = 0;
int dttPriorLastBusyTrip = 0;
int dttLastBusyClear = 0;
int dttLastTrip;
extern float dblAvgPk2BaselineRatio, dblAvgBaselineSlow, dblAvgBaselineFast;
int intHoldMs = 5000;


VOID ClearBusy()
{
	dttLastBusyTrip = Now;
	dttPriorLastBusyTrip = dttLastBusyTrip;
	dttLastBusyClear = dttLastBusyTrip + 610;	// This insures test in ARDOPprotocol ~ line 887 will work 
	dttLastTrip = dttLastBusyTrip -intHoldMs;	// This clears the busy detect immediatly (required for scanning when re enabled by Listen=True
	blnLastBusy = False;
	intBusyOnCnt = 0;
	intBusyOffCnt = 0;
	intLastStart = 0;
	intLastStop = 0;		// This will force the busy detector to ignore old averages and initialze the rolling average filters
}

extern int FFTSize;

BOOL BusyDetect3(float * dblMag, int StartFreq, int EndFreq)
{

	// Based on code from ARDOP, but using diffferent FFT size
	// QtSM is using an FFT size based on waterfall settings.

	// First sort signals and look at highes signals:baseline ratio..
	
	// Start and Stop are in Hz. Convert to bin numbers


	float BinSize = 12000.0 / FFTSize;
	int StartBin = StartFreq / BinSize;
	int EndBin = EndFreq / BinSize;

	float dblAVGSignalPerBinNarrow, dblAVGSignalPerBinWide, dblAVGBaselineNarrow, dblAVGBaselineWide;
	float dblSlowAlpha = 0.2f;
	float dblAvgStoNNarrow = 0, dblAvgStoNWide = 0;
	int intNarrow = 100 / BinSize;  // 8 x 11.72 Hz about 94 z
	int intWide = ((EndBin - StartBin) * 2) / 3; //* 0.66);
	int blnBusy = FALSE;
	int  BusyDet4th = BusyDet * BusyDet * BusyDet * BusyDet;


	// First sort signals and look at highest signals:baseline ratio..
	// First narrow band (~94Hz)

	SortSignals2(dblMag, StartBin, EndBin, intNarrow, &dblAVGSignalPerBinNarrow, &dblAVGBaselineNarrow);

	if (intLastStart == StartBin && intLastStop == EndBin)
		dblAvgStoNNarrow = (1 - dblSlowAlpha) * dblAvgStoNNarrow + dblSlowAlpha * dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
	else
	{
		// This initializes the Narrow average after a bandwidth change

		dblAvgStoNNarrow = dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
 		intLastStart = StartBin;
		intLastStop = EndBin;
	}
	
	// Wide band (66% of current bandwidth)
	
	SortSignals2(dblMag, StartBin, EndBin, intWide, &dblAVGSignalPerBinWide, &dblAVGBaselineWide);

	if (intLastStart == StartBin && intLastStop == EndBin)
		dblAvgStoNWide = (1 - dblSlowAlpha) * dblAvgStoNWide + dblSlowAlpha * dblAVGSignalPerBinWide / dblAVGBaselineWide;
	else
	{
		// This initializes the Wide average after a bandwidth change
		
		dblAvgStoNWide = dblAVGSignalPerBinWide / dblAVGBaselineWide;
		intLastStart = StartBin;
		intLastStop = EndBin;
	}

	// Preliminary calibration...future a function of bandwidth and BusyDet.
   
	switch (ARQBandwidth)
	{
	case XB200:
		blnBusy = (dblAvgStoNNarrow > (3 + 0.008 * BusyDet4th)) || (dblAvgStoNWide > (5 + 0.02 * BusyDet4th));
		break;

	case XB500:
		blnBusy = (dblAvgStoNNarrow > (3 + 0.008 * BusyDet4th) )|| (dblAvgStoNWide > (5 + 0.02 * BusyDet4th));
		break;

	case XB2500:
		blnBusy = (dblAvgStoNNarrow > (3 + 0.008 * BusyDet4th)) || (dblAvgStoNWide > (5 + 0.016 * BusyDet4th));
 	}

	if (BusyDet == 0)
		blnBusy = FALSE;		// 0 Disables check ?? Is this the best place to do this?

//	WriteDebugLog(LOGDEBUG, "Busy %d Wide %f Narrow %f", blnBusy, dblAvgStoNWide, dblAvgStoNNarrow); 

	if (blnBusy)
	{
		// This requires multiple adjacent busy conditions to skip over one nuisance Busy trips. 
		// Busy must be present at least 3 consecutive times ( ~250 ms) to be reported

		intBusyOnCnt += 1;
		intBusyOffCnt = 0;
        if (intBusyOnCnt > 3)
			dttLastTrip = Now;
	}
	else
	{
		intBusyOffCnt += 1;
		intBusyOnCnt = 0;
	}

	if (blnLastBusy == False && intBusyOnCnt >= 3)
	{
		dttPriorLastBusyTrip = dttLastBusyTrip;  // save old dttLastBusyTrip for use in BUSYBLOCKING function
		dttLastBusyTrip = Now;
		blnLastBusy = True;
	}
	else
	{
		if (blnLastBusy && (Now - dttLastTrip) > intHoldMs && intBusyOffCnt >= 3)
		{
			dttLastBusyClear = Now;
			blnLastBusy = False;
		}
	}
	return blnLastBusy;
}

VOID SortSignals(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin)
{
	// puts the top intNumber of bins between intStartBin and intStopBin into dblAVGSignalPerBin, the rest into dblAvgBaselinePerBin
    // for decent accuracy intNumBins should be < 75% of intStopBin-intStartBin)

	float dblAVGSignal[200] = {0};//intNumBins
	float dblAVGBaseline[200] = {0};//intStopBin - intStartBin - intNumBins

	float dblSigSum = 0;
	float dblTotalSum = 0;
	int intSigPtr = 0;
	int intBasePtr = 0;
	int i, j, k;

	for (i = 0; i <  intNumBins; i++)
	{
		for (j = intStartBin; j <= intStopBin; j++)
		{
			if (i == 0)
			{
				dblTotalSum += dblMag[j];
				if (dblMag[j] > dblAVGSignal[i])
					dblAVGSignal[i] = dblMag[j];
			}
			else
			{
				if (dblMag[j] > dblAVGSignal[i] && dblMag[j] < dblAVGSignal[i - 1])
					dblAVGSignal[i] = dblMag[j];
			}
		}
	}
	
	for(k = 0; k < intNumBins; k++)
	{
		dblSigSum += dblAVGSignal[k];
	}
	*dblAVGSignalPerBin = dblSigSum / intNumBins;
	*dblAVGBaselinePerBin = (dblTotalSum - dblSigSum) / (intStopBin - intStartBin - intNumBins + 1);
}

 BOOL compare(const void *p1, const void *p2)
{
    float x = *(const float *)p1;
    float y = *(const float *)p2;

    if (x < y)
        return -1;  // Return -1 if you want ascending, 1 if you want descending order. 
    else if (x > y)
        return 1;   // Return 1 if you want ascending, -1 if you want descending order. 

    return 0;
}

VOID SortSignals2(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin)
{
	// puts the top intNumber of bins between intStartBin and intStopBin into dblAVGSignalPerBin, the rest into dblAvgBaselinePerBin
    // for decent accuracy intNumBins should be < 75% of intStopBin-intStartBin)

	// This version uses a native sort function which is much faster and reduces CPU loading significantly on wide bandwidths. 
  
	float dblSort[202]; 
	float dblSum1 = 0, dblSum2 = 0;
	int numtoSort = (intStopBin - intStartBin) + 1, i;
	
	memcpy(dblSort, &dblMag[intStartBin], numtoSort * sizeof(float));

	qsort((void *)dblSort, numtoSort, sizeof(float), compare);

	for (i = numtoSort -1; i >= 0; i--)
	{
		if (i >= (numtoSort - intNumBins))
			dblSum1 += dblSort[i];
		else
			dblSum2 += dblSort[i];
	}

	*dblAVGSignalPerBin = dblSum1 / intNumBins;
	*dblAVGBaselinePerBin = dblSum2 / (intStopBin - intStartBin - intNumBins - 1);
}

 