// ----------------------------------------------------------------------------
//
//	rsid.h
//
// Copyright (C) 2008, 2009
//		Dave Freese, W1HKJ
// Copyright (C) 2009
//		Stelios Bounanos, M0GLD
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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Tone separation: 10.766Hz
// Integer tone separator (x 16): 172
// Error on 16 tones: 0.25Hz

// Tone duration: 0.093 sec
// Tone duration, #samples at 8ksps: 743
// Error on 15 tones: negligible

// 1024 samples -> 512 tones
// 2048 samples, second half zeros

// each 512 samples new FFT
// ----------------------------------------------------------------------------

// This code has been modified to work with QtSOundModem by John Wiseman G8BPQ

// Main change is to run at 12000 samples/sec and only support QtSM Modes. This
// makes it incompatble with MultiPSK and fldigi

// Needed code has been extracted and converted to C

#ifndef RSID_H
#define RSID_H

#include "globals.h"
//#include "gfft.h"

//#define RSID_SAMPLE_RATE 11025.0f
#define RSID_SAMPLE_RATE 12000.0f

#define RSID_FFT_SAMPLES 	512
#define RSID_FFT_SIZE		1024
#define RSID_ARRAY_SIZE	 	(RSID_FFT_SIZE * 2)
#define RSID_BUFFER_SIZE	(RSID_ARRAY_SIZE * 2)

#define RSID_NSYMBOLS    15
#define RSID_NTIMES      (RSID_NSYMBOLS * 2)
#define RSID_PRECISION   2.7 // detected frequency precision in Hz

// each rsid symbol has a duration equal to 1024 samples at 11025 Hz smpl rate
#define RSID_SYMLEN		(1024.0 / RSID_SAMPLE_RATE) // 0.09288 // duration of each rsid symbol

enum {
	RSID_BANDWIDTH_500 = 0,
	RSID_BANDWIDTH_1K,
	RSID_BANDWIDTH_WIDE,
};

typedef double rs_fft_type;


#endif
