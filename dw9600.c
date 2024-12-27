// 4800/9600 RHU Mode code for QtSoundModem

// Based on code from Dire Wolf Copyright (C) 2011, 2012, 2013, 2014, 2015  John Langner, WB2OSZ


#define MODE_RUH 7

typedef struct string_T
{
	unsigned char * Data;
	int Length;
	int AllocatedLength;				// A reasonable sized block is allocated at the start to speed up adding chars

}string;

typedef struct TStringList_T
{
	int Count;
	string ** Items;

} TStringList;

#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include "dw9600.h"

#define stringAdd(s1, s2, c) mystringAdd(s1, s2, c, __FILE__, __LINE__)

string * mystringAdd(string * Msg, unsigned char * Chars, int Count, char * FILE, int  LINE);

extern int fx25_mode[4];
extern int il2p_mode[4];
extern int il2p_crc[4];
extern short rx_baudrate[5];
extern short tx_bitrate[5];

#define FX25_MODE_NONE  0
#define FX25_MODE_RX  1
#define FX25_MODE_TXRX 2
#define FX25_TAG 0
#define FX25_LOAD 1

#define IL2P_MODE_NONE  0
#define IL2P_MODE_RX  1				// RX il2p + HDLC
#define IL2P_MODE_TXRX 2
#define IL2P_MODE_ONLY 3			// RX only il2p, TX il2p


extern unsigned short * DMABuffer;
extern int SampleNo;

string * il2p_send_frame(int chan, packet_t pp, int max_fec, int polarity);
int fx25_send_frame(int chan, unsigned char *fbuf, int flen, int fx_mode);
int multi_modem_process_rec_frame(int chan, int subchan, int slice, unsigned char *fbuf, int flen, int alevel, int retries, int is_fx25);
void ProcessRXFrames(int snd_ch);
unsigned short get_fcs(unsigned char * Data, unsigned short len);
void il2p_rec_bit(int chan, int subchan, int slice, int dbit);
void Debugprintf(const char * format, ...);

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


#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>          // uint64_t
#define RRBB_C 1

/********************************************************************************
 *
 * File:	hdlc_rec.c
 *
 * Purpose:	Extract HDLC frames from a stream of bits.
 *
 *******************************************************************************/


//#define TEST 1				/* Define for unit testing. */

//#define DEBUG3 1				/* monitor the data detect signal. */



/*
 * Minimum & maximum sizes of an AX.25 frame including the 2 octet FCS.
 */

#define MIN_FRAME_LEN ((AX25_MIN_PACKET_LEN) + 2)

#define MAX_FRAME_LEN ((AX25_MAX_PACKET_LEN) + 2)	

 /*
  * This is the current state of the HDLC decoder.
  *
  * It is possible to run multiple decoders concurrently by
  * having a separate set of state variables for each.
  *
  * Should have a reset function instead of initializations here.
  */

struct hdlc_state_s {


	int prev_raw;			/* Keep track of previous bit so */
					/* we can look for transitions. */
					/* Should be only 0 or 1. */

	int lfsr;			/* Descrambler shift register for 9600 baud. */

	int prev_descram;		/* Previous descrambled for 9600 baud. */

	unsigned char pat_det; 		/* 8 bit pattern detector shift register. */
					/* See below for more details. */

	unsigned int flag4_det;		/* Last 32 raw bits to look for 4 */
					/* flag patterns in a row. */

	unsigned char oacc;		/* Accumulator for building up an octet. */

	int olen;			/* Number of bits in oacc. */
					/* When this reaches 8, oacc is copied */
					/* to the frame buffer and olen is zeroed. */
					/* The value of -1 is a special case meaning */
					/* bits should not be accumulated. */

	unsigned char frame_buf[MAX_FRAME_LEN];
	/* One frame is kept here. */

	int frame_len;			/* Number of octets in frame_buf. */
					/* Should be in range of 0 .. MAX_FRAME_LEN. */

	rrbb_t rrbb;			/* Handle for bit array for raw received bits. */

	uint64_t eas_acc;		/* Accumulate most recent 64 bits received for EAS. */

	int eas_gathering;		/* Decoding in progress. */

	int eas_plus_found;		/* "+" seen, indicating end of geographical area list. */

	int eas_fields_after_plus;	/* Number of "-" characters after the "+". */
};

static struct hdlc_state_s hdlc_state[MAX_CHANS];

static int num_subchan[MAX_CHANS];		//TODO1.2 use ptr rather than copy.

static int composite_dcd[MAX_CHANS][MAX_SUBCHANS + 1];


/***********************************************************************************
 *
 * Name:	hdlc_rec_init
 *
 * Purpose:	Call once at the beginning to initialize.
 *
 * Inputs:	None.
 *
 ***********************************************************************************/

int was_init[4] = { 0, 0, 0, 0 };

extern struct audio_s pa[4];

void hdlc_rec_init(struct audio_s *pa)
{
	int ch;
	struct hdlc_state_s *H;

	//text_color_set(DW_COLOR_DEBUG);
	//dw_printf ("hdlc_rec_init (%p) \n", pa);

	assert(pa != NULL);


	memset(composite_dcd, 0, sizeof(composite_dcd));

	for (ch = 0; ch < MAX_CHANS; ch++)
	{
		pa->achan[ch].num_subchan = 1;
		num_subchan[ch] = pa->achan[ch].num_subchan;

		assert(num_subchan[ch] >= 1 && num_subchan[ch] <= MAX_SUBCHANS);

		H = &hdlc_state[ch];

		H->olen = -1;

		H->rrbb = rrbb_new(ch, 0, 0, pa->achan[ch].modem_type == MODEM_SCRAMBLE, H->lfsr, H->prev_descram);
	}
	hdlc_rec2_init(pa);

}

/* Own copy of random number generator so we can get */
/* same predictable results on different operating systems. */
/* TODO: Consolidate multiple copies somewhere. */

#define MY_RAND_MAX 0x7fffffff
static int seed = 1;

static int my_rand(void) {
	// Perform the calculation as unsigned to avoid signed overflow error.
	seed = (int)(((unsigned)seed * 1103515245) + 12345) & MY_RAND_MAX;
	return (seed);
}


/***********************************************************************************
 *
 * Name:	eas_rec_bit
 *
 * Purpose:	Extract EAS trasmissions from a stream of bits.
 *
 * Inputs:	chan	- Channel number.
 *
 *		subchan	- This allows multiple demodulators per channel.
 *
 *		slice	- Allows multiple slicers per demodulator (subchannel).
 *
 *		raw 	- One bit from the demodulator.
 *			  should be 0 or 1.
 *
 *		future_use - Not implemented yet.  PSK already provides it.
 *
 *
 * Description:	This is called once for each received bit.
 *		For each valid transmission, process_rec_frame()
 *		is called for further processing.
 *
 ***********************************************************************************/

#define PREAMBLE      0xababababababababULL
#define PREAMBLE_ZCZC 0x435a435aababababULL
#define PREAMBLE_NNNN 0x4e4e4e4eababababULL
#define EAS_MAX_LEN 268  	// Not including preamble.  Up to 31 geographic areas.


static void eas_rec_bit(int chan, int subchan, int slice, int raw, int future_use)
{
	struct hdlc_state_s *H;

	/*
	 * Different state information for each channel / subchannel / slice.
	 */
	H = &hdlc_state[chan];

	//dw_printf ("slice %d = %d\n", slice, raw);

// Accumulate most recent 64 bits.

	H->eas_acc >>= 1;
	if (raw) {
		H->eas_acc |= 0x8000000000000000ULL;
	}

	int done = 0;

	if (H->eas_acc == PREAMBLE_ZCZC) {
		//dw_printf ("ZCZC\n");
		H->olen = 0;
		H->eas_gathering = 1;
		H->eas_plus_found = 0;
		H->eas_fields_after_plus = 0;
		strcpy((char*)(H->frame_buf), "ZCZC");
		H->frame_len = 4;
	}
	else if (H->eas_acc == PREAMBLE_NNNN) {
		//dw_printf ("NNNN\n");
		H->olen = 0;
		H->eas_gathering = 1;
		strcpy((char*)(H->frame_buf), "NNNN");
		H->frame_len = 4;
		done = 1;
	}
	else if (H->eas_gathering) {
		H->olen++;
		if (H->olen == 8) {
			H->olen = 0;
			char ch = H->eas_acc >> 56;
			H->frame_buf[H->frame_len++] = ch;
			H->frame_buf[H->frame_len] = '\0';
			//dw_printf ("frame_buf = %s\n", H->frame_buf);

			// What characters are acceptable?
			// Only ASCII is allowed.  i.e. the MSB must be 0.
			// The examples show only digits but the geographical area can
			// contain anything in range of '!' to DEL or CR or LF.
			// There are no restrictions listed for the originator and
			// examples contain a slash.
			// It's not clear if a space can occur in other places.

			if (!((ch >= ' ' && ch <= 0x7f) || ch == '\r' || ch == '\n')) {
				//#define DEBUG_E 1
#ifdef DEBUG_E
				dw_printf("reject %d invalid character = %s\n", slice, H->frame_buf);
#endif
				H->eas_gathering = 0;
				return;
			}
			if (H->frame_len > EAS_MAX_LEN) {		// FIXME: look for other places with max length
#ifdef DEBUG_E
				dw_printf("reject %d too long = %s\n", slice, H->frame_buf);
#endif
				H->eas_gathering = 0;
				return;
			}
			if (ch == '+') {
				H->eas_plus_found = 1;
				H->eas_fields_after_plus = 0;
			}
			if (H->eas_plus_found && ch == '-') {
				H->eas_fields_after_plus++;
				if (H->eas_fields_after_plus == 3) {
					done = 1;	// normal case
				}
			}
		}
	}

	if (done) {
#ifdef DEBUG_E
		dw_printf("frame_buf %d = %s\n", slice, H->frame_buf);
#endif
		alevel_t alevel = demod_get_audio_level(chan, subchan);
		multi_modem_process_rec_frame(chan, subchan, slice, H->frame_buf, H->frame_len, 0, 0, 0);
		H->eas_gathering = 0;
	}

} // end eas_rec_bit


/*

EAS has no error detection.
Maybe that doesn't matter because we would normally be dealing with a reasonable
VHF FM or TV signal.
Let's see what happens when we intentionally introduce errors.
When some match and others don't, the multislice voting should give preference
to those matching others.

	$ src/atest -P+ -B EAS -e 3e-3 ../../ref-doc/EAS/same.wav
	Demodulator profile set to "+"
	96000 samples per second.  16 bits per sample.  1 audio channels.
	2079360 audio bytes in file.  Duration = 10.8 seconds.
	Fix Bits level = 0
	Channel 0: 521 baud, AFSK 2083 & 1563 Hz, D+, 96000 sample rate / 3.

case 1:  Slice 6 is different than others (EQS vs. EAS) so we want one of the others that match.
	 Slice 3 has an unexpected character (in 0120u7) so it is a mismatch.
	 At this point we are not doing validity checking other than all printable characters.

	 We are left with 0 & 4 which don't match (012057 vs. 012077).
	 So I guess we don't have any two that match so it is a toss up.

	reject 7 invalid character = ZCZC-EAS-RWT-0120▒
	reject 5 invalid character = ZCZC-ECW-RWT-012057-012081-012101-012103-012115+003
	frame_buf 6 = ZCZC-EQS-RWT-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 4 = ZCZC-EAS-RWT-012077-012081-012101-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 3 = ZCZC-EAS-RWT-0120u7-012281-012101-012103-092115+0038-2780415-VTSP/TV-
	frame_buf 0 = ZCZC-EAS-RWT-012057-412081-012101-012103-012115+0030-2780415-WTSP/TV-

	DECODED[1] 0:01.313 EAS audio level = 194(106/108)     |__||_|__
	[0.0] EAS>APDW16:{DEZCZC-EAS-RWT-012057-412081-012101-012103-012115+0030-2780415-WTSP/TV-

Case 2: We have two that match so pick either one.

	reject 5 invalid character = ZCZC-EAS-RW▒
	reject 7 invalid character = ZCZC-EAS-RWT-0
	reject 3 invalid character = ZCZC-EAS-RWT-012057-012080-012101-012103-01211
	reject 0 invalid character = ZCZC-EAS-RWT-012057-012081-012101-012103-012115+0030-2780415-W▒
	frame_buf 6 = ZCZC-EAS-RWT-012057-012081-012!01-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 1 = ZCZC-EAS-RWT-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-

	DECODED[2] 0:03.617 EAS audio level = 194(106/108)     _|____|__
	[0.1] EAS>APDW16:{DEZCZC-EAS-RWT-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-

Case 3: Slice 6 is a mismatch (EAs vs. EAS).
	Slice 7 has RST rather than RWT.
	2 & 4 don't match either (012141 vs. 012101).
	We have another case where no two match so there is no clear winner.


	reject 5 invalid character = ZCZC-EAS-RWT-012057-012081-012101-012103-012115+▒
	frame_buf 7 = ZCZC-EAS-RST-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 6 = ZCZC-EAs-RWT-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 4 = ZCZC-EAS-RWT-112057-012081-012101-012103-012115+0030-2780415-WTSP/TV-
	frame_buf 2 = ZCZC-EAS-RWT-012057-012081-012141-012103-012115+0030-2780415-WTSP/TV-

	DECODED[3] 0:05.920 EAS audio level = 194(106/108)     __|_|_||_
	[0.2] EAS>APDW16:{DEZCZC-EAS-RWT-012057-012081-012141-012103-012115+0030-2780415-WTSP/TV-

Conclusions:

	(1) The existing algorithm gives a higher preference to those frames matching others.
	We didn't see any cases here where that would be to our advantage.

	(2) A partial solution would be more validity checking.  (i.e. non-digit where
	digit is expected.)  But wait... We might want to keep it for consideration:

	(3) If I got REALLY ambitious, some day, we could compare all of them one column
	at a time and take the most popular (and valid for that column) character and
	use all of the most popular characters. Better yet, at the bit level.

Of course this is probably all overkill because we would normally expect to have pretty
decent signals.  The designers didn't even bother to add any sort of checksum for error checking.

The random errors injected are also not realistic. Actual noise would probably wipe out the
same bit(s) for all of the slices.

The protocol specification suggests comparing all 3 transmissions and taking the best 2 out of 3.
I think that would best be left to an external application and we just concentrate on being
a good modem here and providing a result when it is received.

*/


/***********************************************************************************
 *
 * Name:	hdlc_rec_bit
 *
 * Purpose:	Extract HDLC frames from a stream of bits.
 *
 * Inputs:	chan	- Channel number.
 *
 *		subchan	- This allows multiple demodulators per channel.
 *
 *		slice	- Allows multiple slicers per demodulator (subchannel).
 *
 *		raw 	- One bit from the demodulator.
 *			  should be 0 or 1.
 *
 *		is_scrambled - Is the data scrambled?
 *
 *		descram_state - Current descrambler state.  (not used - remove)
 *				Not so fast - plans to add new parameter.  PSK already provides it.
 *
 *
 * Description:	This is called once for each received bit.
 *		For each valid frame, process_rec_frame()
 *		is called for further processing.
 *
 ***********************************************************************************/

void hdlc_rec_bit(int chan, int subchan, int slice, int raw, int is_scrambled, int not_used_remove)
{

	int dbit;		/* Data bit after undoing NRZI. */
					/* Should be only 0 or 1. */

	struct hdlc_state_s *H;
	int descram;

	/*
	 * Different state information for each channel / subchannel / slice.
	 */
	H = &hdlc_state[chan];


	/*
	 * Using NRZI encoding,
	 *   A '0' bit is represented by an inversion since previous bit.
	 *   A '1' bit is represented by no change.
	 */

	if (is_scrambled)
	{
		descram = descramble(raw, &(H->lfsr));
		dbit = (descram == H->prev_descram);			// This does nrzi
		H->prev_descram = descram;
		H->prev_raw = raw;
	}
	else {

		dbit = (raw == H->prev_raw);
		H->prev_raw = raw;						// This does nrzi
	}

	// dbit should be after nrzi, descram after descramble but before nrzi

	// After BER insertion, NRZI, and any descrambling, feed into FX.25 decoder as well.

//	fx25_rec_bit (chan, subchan, slice, dbit);

	if (il2p_mode[chan])
	{
		il2p_rec_bit(chan, subchan, slice, raw);	// not scrambled, not nrzi	
		if (il2p_mode[chan] == IL2P_MODE_ONLY)		// Dont try HDLC decode
			return;
	}

	/*
	 * Octets are sent LSB first.
	 * Shift the most recent 8 bits thru the pattern detector.
	 */
	H->pat_det >>= 1;
	if (dbit) {
		H->pat_det |= 0x80;
	}

	H->flag4_det >>= 1;
	if (dbit) {
		H->flag4_det |= 0x80000000;
	}

	rrbb_append_bit(H->rrbb, raw);

	if (H->pat_det == 0x7e) {

		rrbb_chop8(H->rrbb);

		/*
		 * The special pattern 01111110 indicates beginning and ending of a frame.
		 * If we have an adequate number of whole octets, it is a candidate for
		 * further processing.
		 *
		 * It might look odd that olen is being tested for 7 instead of 0.
		 * This is because oacc would already have 7 bits from the special
		 * "flag" pattern before it is detected here.
		 */


#if OLD_WAY

#if TEST
		text_color_set(DW_COLOR_DEBUG);
		dw_printf("\nfound flag, olen = %d, frame_len = %d\n", olen, frame_len);
#endif
		if (H->olen == 7 && H->frame_len >= MIN_FRAME_LEN) {

			unsigned short actual_fcs, expected_fcs;

#if TEST
			int j;
			dw_printf("TRADITIONAL: frame len = %d\n", H->frame_len);
			for (j = 0; j < H->frame_len; j++) {
				dw_printf("  %02x", H->frame_buf[j]);
			}
			dw_printf("\n");

#endif
			/* Check FCS, low byte first, and process... */

			/* Alternatively, it is possible to include the two FCS bytes */
			/* in the CRC calculation and look for a magic constant.  */
			/* That would be easier in the case where the CRC is being */
			/* accumulated along the way as the octets are received. */
			/* I think making a second pass over it and comparing is */
			/* easier to understand. */

			actual_fcs = H->frame_buf[H->frame_len - 2] | (H->frame_buf[H->frame_len - 1] << 8);

			expected_fcs = fcs_calc(H->frame_buf, H->frame_len - 2);

			if (actual_fcs == expected_fcs) {
				alevel_t alevel = demod_get_audio_level(chan, subchan);

				multi_modem_process_rec_frame(chan, subchan, slice, H->frame_buf, H->frame_len - 2, alevel, RETRY_NONE, 0);   /* len-2 to remove FCS. */
			}
			else {

#if TEST
				dw_printf("*** actual fcs = %04x, expected fcs = %04x ***\n", actual_fcs, expected_fcs);
#endif

			}

	  }

#else

		 /*
		  * New way - Decode the raw bits in later step.
		  */

#if TEST
		text_color_set(DW_COLOR_DEBUG);
		dw_printf("\nfound flag, channel %d.%d, %d bits in frame\n", chan, subchan, rrbb_get_len(H->rrbb) - 1);
#endif
		if (rrbb_get_len(H->rrbb) >= MIN_FRAME_LEN * 8) {

			alevel_t alevel = demod_get_audio_level(chan, subchan);

			rrbb_set_audio_level(H->rrbb, alevel);
			hdlc_rec2_block(H->rrbb);
			/* Now owned by someone else who will free it. */

			H->rrbb = rrbb_new(chan, 0, 0, is_scrambled, H->lfsr, H->prev_descram); /* Allocate a new one. */
		}
		else {
			rrbb_clear(H->rrbb, is_scrambled, H->lfsr, H->prev_descram);
		}

		H->olen = 0;		/* Allow accumulation of octets. */
		H->frame_len = 0;


		rrbb_append_bit(H->rrbb, H->prev_raw); /* Last bit of flag.  Needed to get first data bit. */
						  /* Now that we are saving other initial state information, */
						  /* it would be sensible to do the same for this instead */
						  /* of lumping it in with the frame data bits. */
#endif

	}

	//#define EXPERIMENT12B 1

#if EXPERIMENT12B

	else if (H->pat_det == 0xff) {

		/*
		 * Valid data will never have seven 1 bits in a row.
		 *
		 *	11111110
		 *
		 * This indicates loss of signal.
		 * But we will let it slip thru because it might diminish
		 * our float bit fixup effort.   Instead give up on frame
		 * only when we see eight 1 bits in a row.
		 *
		 *	11111111
		 *
		 * What is the impact?  No difference.
		 *
		 *  Before:	atest -P E -F 1 ../02_Track_2.wav	= 1003
		 *  After:	atest -P E -F 1 ../02_Track_2.wav	= 1003
		 */

#else
	else if (H->pat_det == 0xfe) {

		/*
		 * Valid data will never have 7 one bits in a row.
		 *
		 *	11111110
		 *
		 * This indicates loss of signal.
		 */

#endif

		H->olen = -1;		/* Stop accumulating octets. */
		H->frame_len = 0;	/* Discard anything in progress. */

		rrbb_clear(H->rrbb, is_scrambled, H->lfsr, H->prev_descram);

	}
	else if ((H->pat_det & 0xfc) == 0x7c) {

		/*
		 * If we have five '1' bits in a row, followed by a '0' bit,
		 *
		 *	0111110xx
		 *
		 * the current '0' bit should be discarded because it was added for
		 * "bit stuffing."
		 */
		;

	}
	else {

		/*
		 * In all other cases, accumulate bits into octets, and complete octets
		 * into the frame buffer.
		 */
		if (H->olen >= 0) {

			H->oacc >>= 1;
			if (dbit) {
				H->oacc |= 0x80;
			}
			H->olen++;

			if (H->olen == 8) {
				H->olen = 0;

				if (H->frame_len < MAX_FRAME_LEN) {
					H->frame_buf[H->frame_len] = H->oacc;
					H->frame_len++;
				}
			}
		}
	}
}

// TODO:  Data Carrier Detect (DCD) is now based on DPLL lock
// rather than data patterns found here.
// It would make sense to move the next 2 functions to demod.c
// because this is done at the modem level, rather than HDLC decoder.

/*-------------------------------------------------------------------
 *
 * Name:        dcd_change
 *
 * Purpose:     Combine DCD states of all subchannels/ into an overall
 *		state for the channel.
 *
 * Inputs:	chan
 *
 *		subchan		0 to MAX_SUBCHANS-1 for HDLC.
 *				SPECIAL CASE --> MAX_SUBCHANS for DTMF decoder.
 *
 *		slice		slicer number, 0 .. MAX_SLICERS - 1.
 *
 *		state		1 for active, 0 for not.
 *
 * Returns:	None.  Use hdlc_rec_data_detect_any to retrieve result.
 *
 * Description:	DCD for the channel is active if ANY of the subchannels/slices
 *		are active.  Update the DCD indicator.
 *
 * version 1.3:	Add DTMF detection into the final result.
 *		This is now called from dtmf.c too.
 *
 *--------------------------------------------------------------------*/

void dcd_change(int chan, int subchan, int slice, int state)
{

}


/*-------------------------------------------------------------------
 *
 * Name:        hdlc_rec_data_detect_any
 *
 * Purpose:     Determine if the radio channel is currently busy
 *		with packet data.
 *		This version doesn't care about voice or other sounds.
 *		This is used by the transmit logic to transmit only
 *		when the channel is clear.
 *
 * Inputs:	chan	- Audio channel.
 *
 * Returns:	True if channel is busy (data detected) or
 *		false if OK to transmit.
 *
 *
 * Description:	We have two different versions here.
 *
 *		hdlc_rec_data_detect_any sees if ANY of the decoders
 *		for this channel are receiving a signal.   This is
 *		used to determine whether the channel is clear and
 *		we can transmit.  This would apply to the 300 baud
 *		HF SSB case where we have multiple decoders running
 *		at the same time.  The channel is busy if ANY of them
 *		thinks the channel is busy.
 *
 * Version 1.3: New option for input signal to inhibit transmit.
 *
 *--------------------------------------------------------------------*/

int hdlc_rec_data_detect_any(int chan)
{

	int sc;
	assert(chan >= 0 && chan < MAX_CHANS);

	for (sc = 0; sc < num_subchan[chan]; sc++) {
		if (composite_dcd[chan][sc] != 0)
			return (1);
	}

	return (0);

} /* end hdlc_rec_data_detect_any */

/* end hdlc_rec.c */



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



/********************************************************************************
 *
 * File:	hdlc_rec2.c
 *
 * Purpose:	Extract HDLC frame from a block of bits after someone
 *		else has done the work of pulling it out from between
 *		the special "flag" sequences.
 *
 *
 * New in version 1.1:
 *
 *		Several enhancements provided by Fabrice FAURE:
 *
 *		- Additional types of attempts to fix a bad CRC.
 *		- Optimized code to reduce execution time.
 *		- Improved detection of duplicate packets from different fixup attempts.
 *		- Set limit on number of packets in fix up later queue.
 *
 *		One of the new recovery attempt cases recovers three additional
 *		packets that were lost before.  The one thing I disagree with is
 *		use of the word "swap" because that sounds like two things
 *		are being exchanged for each other.  I would prefer "flip"
 *		or "invert" to describe changing a bit to the opposite state.
 *		I took "swap" out of the user-visible messages but left the
 *		rest of the source code as provided.
 *
 * Test results:	We intentionally use the worst demodulator so there
 *			is more opportunity to try to fix the frames.
 *
 *		atest -P A -F n 02_Track_2.wav
 *
 *		n   	description	frames	sec
 *		--  	----------- 	------	---
 *		0	no attempt	963	40	error-free frames
 *		1	invert 1	979	41	16 more
 *		2	invert 2	982	42	3 more
 *		3	invert 3	982	42	no change
 *		4	remove 1	982	43	no change
 *		5	remove 2	982	43	no change
 *		6	remove 3	982	43	no change
 *		7	insert 1	982	45	no change
 *		8	insert 2	982	47	no change
 *		9	invert two sep	993	178	11 more, some visually obvious errors.
 *		10	invert many?	993	190	no change
 *		11	remove many	995	190	2 more, need to investigate in detail.
 *		12	remove two sep	995	201	no change
 *
 * Observations:	The "insert" and "remove" techniques had no benefit.  I would not expect them to.
 *			We have a phase locked loop that attempts to track any slight variations in the
 *			timing so we sample near the middle of the bit interval.  Bits can get corrupted
 *			by noise but not disappear or just appear.  That would be a gap in the timing.
 *			These should probably be removed in a future version.
 *
 *
 * Version 1.2:	Now works for 9600 baud.
 *		This was more complicated due to the data scrambling.
 *		It was necessary to retain more initial state information after
 *		the start flag octet.
 *
 * Version 1.3: Took out all of the "insert" and "remove" cases because they
 *		offer no benenfit.
 *
 *		Took out the delayed processing and just do it realtime.
 *		Changed SWAP to INVERT because it is more descriptive.
 *
 *******************************************************************************/


//#define DEBUG 1
//#define DEBUGx 1
//#define DEBUG_LATER 1

/* Audio configuration. */

static struct audio_s          *save_audio_config_p;


/*
 * Minimum & maximum sizes of an AX.25 frame including the 2 octet FCS.
 */

#define MIN_FRAME_LEN ((AX25_MIN_PACKET_LEN) + 2)

#define MAX_FRAME_LEN ((AX25_MAX_PACKET_LEN) + 2)	


 /*
  * This is the current state of the HDLC decoder.
  *
  * It is possible to run multiple decoders concurrently by
  * having a separate set of state variables for each.
  *
  * Should have a reset function instead of initializations here.
  */

  // TODO: Clean up. This is a remnant of splitting hdlc_rec.c into 2 parts.
  // This is not the same as hdlc_state_s in hdlc_rec.c
  // "2" was added to reduce confusion.  Can be trimmed down.

struct hdlc_state2_s {

	int prev_raw;			/* Keep track of previous bit so */
					/* we can look for transitions. */
					/* Should be only 0 or 1. */

	int is_scrambled;		/* Set for 9600 baud. */
	int lfsr;			/* Descrambler shift register for 9600 baud. */
	int prev_descram;		/* Previous unscrambled for 9600 baud. */


	unsigned char pat_det; 		/* 8 bit pattern detector shift register. */
					/* See below for more details. */

	unsigned char oacc;		/* Accumulator for building up an octet. */

	int olen;			/* Number of bits in oacc. */
					/* When this reaches 8, oacc is copied */
					/* to the frame buffer and olen is zeroed. */

	unsigned char frame_buf[MAX_FRAME_LEN];
	/* One frame is kept here. */

	int frame_len;			/* Number of octets in frame_buf. */
					/* Should be in range of 0 .. MAX_FRAME_LEN. */

};




typedef enum retry_mode_e {
	RETRY_MODE_CONTIGUOUS = 0,
	RETRY_MODE_SEPARATED = 1,
}  retry_mode_t;

typedef enum retry_type_e {
	RETRY_TYPE_NONE = 0,
	RETRY_TYPE_SWAP = 1
}  retry_type_t;

typedef struct retry_conf_s {
	retry_t      retry;
	retry_mode_t mode;
	retry_type_t type;
	union {
		struct {
			int bit_idx_a; /*  */
			int bit_idx_b; /*  */
			int bit_idx_c; /*  */
		} sep;       /* RETRY_MODE_SEPARATED */

		struct {
			int bit_idx;
			int nr_bits;
		} contig;  /* RETRY_MODE_CONTIGUOUS */

	} u_bits;
	int insert_value;

} retry_conf_t;




#if defined(DIREWOLF_C) || defined(ATEST_C) || defined(UDPTEST_C)

static const char * retry_text[] = {
		"NONE",
		"float",
		"DOUBLE",
		"TRIPLE",
		"TWO_SEP",
		"PASSALL" };
#endif



static int try_decode(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel, retry_conf_t retry_conf, int passall);

static int try_to_fix_quick_now(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel);

static int sanity_check(unsigned char *buf, int blen, retry_t bits_flipped, enum sanity_e sanity_test);


/***********************************************************************************
 *
 * Name:	hdlc_rec2_init
 *
 * Purpose:	Initialization.
 *
 * Inputs:	p_audio_config	 - Pointer to configuration settings.
 *				   This is what we care about for each channel.
 *
 *	   			enum retry_e fix_bits;
 *					Level of effort to recover from
 *					a bad FCS on the frame.
 *					0 = no effort
 *					1 = try inverting a float bit
 *					2... = more techniques...
 *
 *	    			enum sanity_e sanity_test;
 *					Sanity test to apply when finding a good
 *					CRC after changing one or more bits.
 *					Must look like APRS, AX.25, or anything.
 *
 *	    			int passall;
 *					Allow thru even with bad CRC after exhausting
 *					all fixup attempts.
 *
 * Description:	Save pointer to configuration for later use.
 *
 ***********************************************************************************/

void hdlc_rec2_init(struct audio_s *p_audio_config)
{
	save_audio_config_p = p_audio_config;
}



/***********************************************************************************
 *
 * Name:	hdlc_rec2_block
 *
 * Purpose:	Extract HDLC frame from a stream of bits.
 *
 * Inputs:	block 		- Handle for bit array.
 *
 * Description:	The other (original) hdlc decoder took one bit at a time
 *		right out of the demodulator.
 *
 *		This is different in that it processes a block of bits
 *		previously extracted from between two "flag" patterns.
 *
 *		This allows us to try decoding the same received data more
 *		than once.
 *
 * Version 1.2:	Now works properly for G3RUH type scrambling.
 *
 ***********************************************************************************/


void hdlc_rec2_block(rrbb_t block)
{
	int chan = rrbb_get_chan(block);
	int subchan = rrbb_get_subchan(block);
	int slice = rrbb_get_slice(block);
	alevel_t alevel = rrbb_get_audio_level(block);
	retry_t fix_bits = save_audio_config_p->achan[chan].fix_bits;
	int passall = save_audio_config_p->achan[chan].passall;
	int ok;

#if DEBUGx
	text_color_set(DW_COLOR_DEBUG);
	dw_printf("\n--- try to decode ---\n");
#endif

	/* Create an empty retry configuration */
	retry_conf_t retry_cfg;

	memset(&retry_cfg, 0, sizeof(retry_cfg));

	/*
	 * For our first attempt we don't try to alter any bits.
	 * Still let it thru if passall AND no retries are desired.
	 */

	retry_cfg.type = RETRY_TYPE_NONE;
	retry_cfg.mode = RETRY_MODE_CONTIGUOUS;
	retry_cfg.retry = RETRY_NONE;
	retry_cfg.u_bits.contig.nr_bits = 0;
	retry_cfg.u_bits.contig.bit_idx = 0;

	ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, passall & (fix_bits == RETRY_NONE));
	if (ok) {
#if DEBUG
		text_color_set(DW_COLOR_INFO);
		dw_printf("Got it the first time.\n");
#endif
		rrbb_delete(block);
		return;
	}

	/*
	 * Not successful with frame in original form.
	 * See if we can "fix" it.
	 */
	if (try_to_fix_quick_now(block, chan, subchan, slice, alevel)) {
		rrbb_delete(block);
		return;
	}


	if (passall) {
		/* Exhausted all desired fix up attempts. */
		/* Let thru even with bad CRC.  Of course, it still */
		/* needs to be a minimum number of whole octets. */
		ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, 1);
		rrbb_delete(block);
	}
	else {
		rrbb_delete(block);
	}

} /* end hdlc_rec2_block */


/***********************************************************************************
 *
 * Name:	try_to_fix_quick_now
 *
 * Purpose:	Attempt some quick fixups that don't take very long.
 *
 * Inputs:	block	- Stream of bits that might be a frame.
 *		chan	- Radio channel from which it was received.
 *		subchan	- Which demodulator when more than one per channel.
 *		alevel	- Audio level for later reporting.
 *
 * Global In:	configuration fix_bits - Maximum level of fix up to attempt.
 *
 *				RETRY_NONE (0)	- Don't try any.
 *				RETRY_INVERT_float (1)  - Try inverting float bits.
 *				etc.
 *
 *		configuration passall - Let it thru with bad CRC after exhausting
 *				all fixup attempts.
 *
 *
 * Returns:	1 for success.  "try_decode" has passed the result along to the
 *				processing step.
 *		0 for failure.  Caller might continue with more aggressive attempts.
 *
 * Original:	Some of the attempted fix up techniques are quick.
 *		We will attempt them immediately after receiving the frame.
 *		Others, that take time order N**2, will be done in a later section.
 *
 * Version 1.2:	Now works properly for G3RUH type scrambling.
 *
 * Version 1.3: Removed the extra cases that didn't help.
 *		The separated bit case is now handled immediately instead of
 *		being thrown in a queue for later processing.
 *
 ***********************************************************************************/

static int try_to_fix_quick_now(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel)
{
	int ok;
	int len, i;
	retry_t fix_bits = save_audio_config_p->achan[chan].fix_bits;
	//int passall = save_audio_config_p->achan[chan].passall;


	len = rrbb_get_len(block);
	/* Prepare the retry configuration */
	retry_conf_t retry_cfg;

	memset(&retry_cfg, 0, sizeof(retry_cfg));

	/* Will modify only contiguous bits*/
	retry_cfg.mode = RETRY_MODE_CONTIGUOUS;
	/*
	 * Try inverting one bit.
	 */
	if (fix_bits < RETRY_INVERT_SINGLE) {

		/* Stop before float bit fix up. */

		return 0;	/* failure. */
	}
	/* Try to swap one bit */
	retry_cfg.type = RETRY_TYPE_SWAP;
	retry_cfg.retry = RETRY_INVERT_SINGLE;
	retry_cfg.u_bits.contig.nr_bits = 1;

	for (i = 0; i < len; i++) {
		/* Set the index of the bit to swap */
		retry_cfg.u_bits.contig.bit_idx = i;
		ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, 0);
		if (ok) {
#if DEBUG
			text_color_set(DW_COLOR_ERROR);
			dw_printf("*** Success by flipping float bit %d of %d ***\n", i, len);
#endif
			return 1;
		}
	}

	/*
	 * Try inverting two adjacent bits.
	 */
	if (fix_bits < RETRY_INVERT_DOUBLE) {
		return 0;
	}
	/* Try to swap two contiguous bits */
	retry_cfg.retry = RETRY_INVERT_DOUBLE;
	retry_cfg.u_bits.contig.nr_bits = 2;


	for (i = 0; i < len - 1; i++) {
		retry_cfg.u_bits.contig.bit_idx = i;
		ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, 0);
		if (ok) {
#if DEBUG
			text_color_set(DW_COLOR_ERROR);
			dw_printf("*** Success by flipping DOUBLE bit %d of %d ***\n", i, len);
#endif
			return 1;
		}
	}

	/*
	 * Try inverting adjacent three bits.
	 */
	if (fix_bits < RETRY_INVERT_TRIPLE) {
		return 0;
	}
	/* Try to swap three contiguous bits */
	retry_cfg.retry = RETRY_INVERT_TRIPLE;
	retry_cfg.u_bits.contig.nr_bits = 3;

	for (i = 0; i < len - 2; i++) {
		retry_cfg.u_bits.contig.bit_idx = i;
		ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, 0);
		if (ok) {
#if DEBUG
			text_color_set(DW_COLOR_ERROR);
			dw_printf("*** Success by flipping TRIPLE bit %d of %d ***\n", i, len);
#endif
			return 1;
		}
	}


	/*
	 * Two  non-adjacent ("separated") float bits.
	 * It chews up a lot of CPU time.  Usual test takes 4 times longer to run.
	 *
	 * Processing time is order N squared so time goes up rapidly with larger frames.
	 */
	if (fix_bits < RETRY_INVERT_TWO_SEP) {
		return 0;
	}

	retry_cfg.mode = RETRY_MODE_SEPARATED;
	retry_cfg.type = RETRY_TYPE_SWAP;
	retry_cfg.retry = RETRY_INVERT_TWO_SEP;
	retry_cfg.u_bits.sep.bit_idx_c = -1;

#ifdef DEBUG_LATER
	tstart = dtime_now();
	dw_printf("*** Try flipping TWO SEPARATED BITS %d bits\n", len);
#endif
	len = rrbb_get_len(block);
	for (i = 0; i < len - 2; i++) {
		retry_cfg.u_bits.sep.bit_idx_a = i;
		int j;

		ok = 0;
		for (j = i + 2; j < len; j++) {
			retry_cfg.u_bits.sep.bit_idx_b = j;
			ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, 0);
			if (ok) {
				break;
			}

		}
		if (ok) {
#if DEBUG
			text_color_set(DW_COLOR_ERROR);
			dw_printf("*** Success by flipping TWO SEPARATED bits %d and %d of %d \n", i, j, len);
#endif
			return (1);
		}
	}

	return 0;
}



// TODO:  Remove this.  but first figure out what to do in atest.c



int hdlc_rec2_try_to_fix_later(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel)
{
	int ok;
	//int len;
	//retry_t fix_bits = save_audio_config_p->achan[chan].fix_bits;
	int passall = save_audio_config_p->achan[chan].passall;
#if DEBUG_LATER
	double tstart, tend;
#endif
	retry_conf_t retry_cfg;

	memset(&retry_cfg, 0, sizeof(retry_cfg));

	//len = rrbb_get_len(block);


/*
 * All fix up attempts have failed.
 * Should we pass it along anyhow with a bad CRC?
 * Note that we still need a minimum number of whole octets.
 */
	if (passall) {

		retry_cfg.type = RETRY_TYPE_NONE;
		retry_cfg.mode = RETRY_MODE_CONTIGUOUS;
		retry_cfg.retry = RETRY_NONE;
		retry_cfg.u_bits.contig.nr_bits = 0;
		retry_cfg.u_bits.contig.bit_idx = 0;
		ok = try_decode(block, chan, subchan, slice, alevel, retry_cfg, passall);
		return (ok);
	}

	return (0);

}  /* end hdlc_rec2_try_to_fix_later */



/*
 * Check if the specified index of bit has been modified with the current type of configuration
 * Provide a specific implementation for contiguous mode to optimize number of tests done in the loop
 */

inline static char is_contig_bit_modified(int bit_idx, retry_conf_t retry_conf) {
	int cont_bit_idx = retry_conf.u_bits.contig.bit_idx;
	int cont_nr_bits = retry_conf.u_bits.contig.nr_bits;

	if (bit_idx >= cont_bit_idx && (bit_idx < cont_bit_idx + cont_nr_bits))
		return 1;
	else
		return 0;
}

/*
 * Check  if the specified index of bit has been modified with the current type of configuration in separated bit index mode
 * Provide a specific implementation for separated mode to optimize number of tests done in the loop
 */

inline static char is_sep_bit_modified(int bit_idx, retry_conf_t retry_conf) {
	if (bit_idx == retry_conf.u_bits.sep.bit_idx_a ||
		bit_idx == retry_conf.u_bits.sep.bit_idx_b ||
		bit_idx == retry_conf.u_bits.sep.bit_idx_c)
		return 1;
	else
		return 0;
}



/***********************************************************************************
 *
 * Name:	try_decode
 *
 * Purpose:
 *
 * Inputs:	block		- Bit string that was collected between "flag" patterns.
 *
 *		chan, subchan	- where it came from.
 *
 *		alevel		- audio level for later reporting.
 *
 *		retry_conf	- Controls changes that will be attempted to get a good CRC.
 *
 *	   			retry:
 *					Level of effort to recover from a bad FCS on the frame.
 *				                RETRY_NONE = 0
 *				                RETRY_INVERT_float = 1
 *				                RETRY_INVERT_DOUBLE = 2
 *		                                RETRY_INVERT_TRIPLE = 3
 *		                                RETRY_INVERT_TWO_SEP = 4
 *
 *	    			mode:	RETRY_MODE_CONTIGUOUS - change adjacent bits.
 *						contig.bit_idx - first bit position
 *						contig.nr_bits - number of bits
 *
 *				        RETRY_MODE_SEPARATED  - change bits not next to each other.
 *						sep.bit_idx_a - bit positions
 *						sep.bit_idx_b - bit positions
 *						sep.bit_idx_c - bit positions
 *
 *				type:	RETRY_TYPE_NONE	- Make no changes.
 *					RETRY_TYPE_SWAP - Try inverting.
 *
 *		passall		- All it thru even with bad CRC.
 *				  Valid only when no changes make.  i.e.
 *					retry == RETRY_NONE, type == RETRY_TYPE_NONE
 *
 * Returns:	1 = successfully extracted something.
 *		0 = failure.
 *
 ***********************************************************************************/

static int try_decode(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel, retry_conf_t retry_conf, int passall)
{
	struct hdlc_state2_s H2;
	int blen;			/* Block length in bits. */
	int i;
	int raw;			/* From demodulator.  Should be 0 or 1. */
#if DEBUGx
	int crc_failed = 1;
#endif
	int retry_conf_mode = retry_conf.mode;
	int retry_conf_type = retry_conf.type;
	int retry_conf_retry = retry_conf.retry;


	H2.is_scrambled = rrbb_get_is_scrambled(block);
	H2.prev_descram = rrbb_get_prev_descram(block);
	H2.lfsr = rrbb_get_descram_state(block);
	H2.prev_raw = rrbb_get_bit(block, 0);	  /* Actually last bit of the */
					/* opening flag so we can derive the */
					/* first data bit.  */

	/* Does this make sense? */
	/* This is the last bit of the "flag" pattern. */
	/* If it was corrupted we wouldn't have detected */
	/* the start of frame. */

	if ((retry_conf.mode == RETRY_MODE_CONTIGUOUS && is_contig_bit_modified(0, retry_conf)) ||
		(retry_conf.mode == RETRY_MODE_SEPARATED && is_sep_bit_modified(0, retry_conf))) {
		H2.prev_raw = !H2.prev_raw;
	}

	H2.pat_det = 0;
	H2.oacc = 0;
	H2.olen = 0;
	H2.frame_len = 0;

	blen = rrbb_get_len(block);

#if DEBUGx
	text_color_set(DW_COLOR_DEBUG);
	if (retry_conf.type == RETRY_TYPE_NONE)
		dw_printf("try_decode: blen=%d\n", blen);
#endif
	for (i = 1; i < blen; i++) {
		/* Get the value for the current bit */
		raw = rrbb_get_bit(block, i);
		/* If swap two sep mode , swap the bit if needed */
		if (retry_conf_retry == RETRY_INVERT_TWO_SEP) {
			if (is_sep_bit_modified(i, retry_conf))
				raw = !raw;
		}
		/* Else handle all the others contiguous modes */
		else if (retry_conf_mode == RETRY_MODE_CONTIGUOUS) {

			if (retry_conf_type == RETRY_TYPE_SWAP) {
				/* If this is the bit to swap */
				if (is_contig_bit_modified(i, retry_conf))
					raw = !raw;
			}

		}
		else {
		}
		/*
		 * Octets are sent LSB first.
		 * Shift the most recent 8 bits thru the pattern detector.
		 */
		H2.pat_det >>= 1;

		/*
		 * Using NRZI encoding,
		 *   A '0' bit is represented by an inversion since previous bit.
		 *   A '1' bit is represented by no change.
		 *   Note: this code can be factorized with the raw != H2.prev_raw code at the cost of processing time
		 */

		int dbit;

		if (H2.is_scrambled) {
			int descram;

			descram = descramble(raw, &(H2.lfsr));

			dbit = (descram == H2.prev_descram);
			H2.prev_descram = descram;
			H2.prev_raw = raw;
		}
		else {

			dbit = (raw == H2.prev_raw);
			H2.prev_raw = raw;
		}

		if (dbit) {

			H2.pat_det |= 0x80;
			/* Valid data will never have 7 one bits in a row: exit. */
			if (H2.pat_det == 0xfe) {
#if DEBUGx
				text_color_set(DW_COLOR_DEBUG);
				dw_printf("try_decode: found abort, i=%d\n", i);
#endif
				return 0;
			}
			H2.oacc >>= 1;
			H2.oacc |= 0x80;
		}
		else {

			/* The special pattern 01111110 indicates beginning and ending of a frame: exit. */
			if (H2.pat_det == 0x7e) {
#if DEBUGx
				text_color_set(DW_COLOR_DEBUG);
				dw_printf("try_decode: found flag, i=%d\n", i);
#endif
				return 0;
				/*
				 * If we have five '1' bits in a row, followed by a '0' bit,
				 *
				 *	011111xx
				 *
				 * the current '0' bit should be discarded because it was added for
				 * "bit stuffing."
				 */

			}
			else if ((H2.pat_det >> 2) == 0x1f) {
				continue;
			}
			H2.oacc >>= 1;
		}

		/*
		 * Now accumulate bits into octets, and complete octets
		 * into the frame buffer.
		 */

		H2.olen++;

		if (H2.olen & 8) {
			H2.olen = 0;

			if (H2.frame_len < MAX_FRAME_LEN) {
				H2.frame_buf[H2.frame_len] = H2.oacc;
				H2.frame_len++;

			}
		}
	}	/* end of loop on all bits in block */
/*
 * Do we have a minimum number of complete bytes?
 */

#if DEBUGx
	text_color_set(DW_COLOR_DEBUG);
	dw_printf("try_decode: olen=%d, frame_len=%d\n", H2.olen, H2.frame_len);
#endif

	if (H2.olen == 0 && H2.frame_len >= MIN_FRAME_LEN) {

		unsigned short actual_fcs, expected_fcs;

#if DEBUGx 
		if (retry_conf.type == RETRY_TYPE_NONE) {
			int j;
			text_color_set(DW_COLOR_DEBUG);
			dw_printf("NEW WAY: frame len = %d\n", H2.frame_len);
			for (j = 0; j < H2.frame_len; j++) {
				dw_printf("  %02x", H2.frame_buf[j]);
			}
			dw_printf("\n");

		}
#endif
		/* Check FCS, low byte first, and process... */

		/* Alternatively, it is possible to include the two FCS bytes */
		/* in the CRC calculation and look for a magic constant.  */
		/* That would be easier in the case where the CRC is being */
		/* accumulated along the way as the octets are received. */
		/* I think making a second pass over it and comparing is */
		/* easier to understand. */

		actual_fcs = H2.frame_buf[H2.frame_len - 2] | (H2.frame_buf[H2.frame_len - 1] << 8);

		expected_fcs = get_fcs(H2.frame_buf, H2.frame_len - 2);


		if (actual_fcs == expected_fcs &&
			sanity_check(H2.frame_buf, H2.frame_len - 2, retry_conf.retry, save_audio_config_p->achan[chan].sanity_test)) {

			// TODO: Shouldn't be necessary to pass chan, subchan, alevel into
			// try_decode because we can obtain them from block.
			// Let's make sure that assumption is good...

			assert(rrbb_get_chan(block) == chan);
			assert(rrbb_get_subchan(block) == subchan);
			multi_modem_process_rec_frame(chan, subchan, slice, H2.frame_buf, H2.frame_len - 2, 0, retry_conf.retry, 0);   /* len-2 to remove FCS. */
			return 1;		/* success */

		}
		else if (passall) {
			if (retry_conf_retry == RETRY_NONE && retry_conf_type == RETRY_TYPE_NONE) {

				//text_color_set(DW_COLOR_ERROR);
				//dw_printf ("ATTEMPTING PASSALL PROCESSING\n");

				multi_modem_process_rec_frame(chan, subchan, slice, H2.frame_buf, H2.frame_len - 2, 0, RETRY_MAX, 0);   /* len-2 to remove FCS. */
				return 1;		/* success */
			}
			else {
				text_color_set(DW_COLOR_ERROR);
				dw_printf("try_decode: internal error passall = %d, retry_conf_retry = %d, retry_conf_type = %d\n",
					passall, retry_conf_retry, retry_conf_type);
			}
		}
		else {

			goto failure;
		}
	}
	else {
#if DEBUGx
		crc_failed = 0;
#endif
		goto failure;
	}
failure:
#if DEBUGx
	if (retry_conf.type == RETRY_TYPE_NONE) {
		int j;
		text_color_set(DW_COLOR_ERROR);
		if (crc_failed)
			dw_printf("CRC failed\n");
		if (H2.olen != 0)
			dw_printf("Bad olen: %d \n", H2.olen);
		else if (H2.frame_len < MIN_FRAME_LEN) {
			dw_printf("Frame too small\n");
			goto end;
		}

		dw_printf("FAILURE with frame: frame len = %d\n", H2.frame_len);
		dw_printf("\n");
		for (j = 0; j < H2.frame_len; j++) {
			dw_printf(" %02x", H2.frame_buf[j]);
		}
		dw_printf("\nDEC\n");
		for (j = 0; j < H2.frame_len; j++) {
			dw_printf("%c", H2.frame_buf[j] >> 1);
		}
		dw_printf("\nORIG\n");
		for (j = 0; j < H2.frame_len; j++) {
			dw_printf("%c", H2.frame_buf[j]);
		}
		dw_printf("\n");
	}
end:
#endif
	return 0;	/* failure. */

} /* end try_decode */



/***********************************************************************************
 *
 * Name:	sanity_check
 *
 * Purpose:	Try to weed out bogus packets from initially failed FCS matches.
 *
 * Inputs:	buf
 *
 *		blen
 *
 *		bits_flipped
 *
 *		sanity		How much sanity checking to perform:
 *					SANITY_APRS - Looks like APRS.  See User Guide,
 *						section that discusses bad apples.
 *					SANITY_AX25 - Has valid AX.25 address part.
 *						No checking of the rest.  Useful for
 *						connected mode packet.
 *					SANITY_NONE - No checking.  Would be suitable
 *						only if using frames that don't conform
 *						to AX.25 standard.
 *
 * Returns:	1 if it passes the sanity test.
 *
 * Description:	This is NOT a validity check.
 *		We don't know if modifying the frame fixed the problem or made it worse.
 *		We can only test if it looks reasonable.
 *
 ***********************************************************************************/


static int sanity_check(unsigned char *buf, int blen, retry_t bits_flipped, enum sanity_e sanity_test)
{
	int alen;		/* Length of address part. */
	int j;

	/*
	 * No sanity check if we didn't try fixing the data.
	 * Should we have different levels of checking depending on
	 * how much we try changing the raw data?
	 */
	if (bits_flipped == RETRY_NONE) {
		return 1;
	}


	/*
	 * If using frames that do not conform to AX.25, it might be
	 * desirable to skip the sanity check entirely.
	 */
	if (sanity_test == SANITY_NONE) {
		return (1);
	}

	/*
	 * Address part must be a multiple of 7.
	 */

	alen = 0;
	for (j = 0; j < blen && alen == 0; j++) {
		if (buf[j] & 0x01) {
			alen = j + 1;
		}
	}

	if (alen % 7 != 0) {
#if DEBUGx
		text_color_set(DW_COLOR_ERROR);
		dw_printf("sanity_check: FAILED.  Address part length %d not multiple of 7.\n", alen);
#endif
		return 0;
	}

	/*
	 * Need at least 2 addresses and maximum of 8 digipeaters.
	 */

	if (alen / 7 < 2 || alen / 7 > 10) {
#if DEBUGx
		text_color_set(DW_COLOR_ERROR);
		dw_printf("sanity_check: FAILED.  Too few or many addresses.\n");
#endif
		return 0;
	}

	/*
	 * Addresses can contain only upper case letters, digits, and space.
	 */

	for (j = 0; j < alen; j += 7) {

		char addr[7];

		addr[0] = buf[j + 0] >> 1;
		addr[1] = buf[j + 1] >> 1;
		addr[2] = buf[j + 2] >> 1;
		addr[3] = buf[j + 3] >> 1;
		addr[4] = buf[j + 4] >> 1;
		addr[5] = buf[j + 5] >> 1;
		addr[6] = '\0';


		if ((!isupper(addr[0]) && !isdigit(addr[0])) ||
			(!isupper(addr[1]) && !isdigit(addr[1]) && addr[1] != ' ') ||
			(!isupper(addr[2]) && !isdigit(addr[2]) && addr[2] != ' ') ||
			(!isupper(addr[3]) && !isdigit(addr[3]) && addr[3] != ' ') ||
			(!isupper(addr[4]) && !isdigit(addr[4]) && addr[4] != ' ') ||
			(!isupper(addr[5]) && !isdigit(addr[5]) && addr[5] != ' ')) {
#if DEBUGx	  
			text_color_set(DW_COLOR_ERROR);
			dw_printf("sanity_check: FAILED.  Invalid characters in addresses \"%s\"\n", addr);
#endif
			return 0;
		}
	}


	/*
	 * That's good enough for the AX.25 sanity check.
	 * Continue below for additional APRS checking.
	 */
	if (sanity_test == SANITY_AX25) {
		return (1);
	}

	/*
	 * The next two bytes should be 0x03 and 0xf0 for APRS.
	 */

	if (buf[alen] != 0x03 || buf[alen + 1] != 0xf0) {
		return (0);
	}

	/*
	 * Finally, look for bogus characters in the information part.
	 * In theory, the bytes could have any values.
	 * In practice, we find only printable ASCII characters and:
	 *
	 *	0x0a	line feed
	 *	0x0d	carriage return
	 *	0x1c	MIC-E
	 *	0x1d	MIC-E
	 *	0x1e	MIC-E
	 *	0x1f	MIC-E
	 *	0x7f	MIC-E
	 *	0x80	"{UIV32N}<0x0d><0x9f><0x80>"
	 *	0x9f	"{UIV32N}<0x0d><0x9f><0x80>"
	 *	0xb0	degree symbol, ISO LATIN1
	 *		  (Note: UTF-8 uses two byte sequence 0xc2 0xb0.)
	 *	0xbe	invalid MIC-E encoding.
	 *	0xf8	degree symbol, Microsoft code page 437
	 *
	 * So, if we have something other than these (in English speaking countries!),
	 * chances are that we have bogus data from twiddling the wrong bits.
	 *
	 * Notice that we shouldn't get here for good packets.  This extra level
	 * of checking happens only if we twiddled a couple of bits, possibly
	 * creating bad data.  We want to be very fussy.
	 */

	for (j = alen + 2; j < blen; j++) {
		int ch = buf[j];

		if (!((ch >= 0x1c && ch <= 0x7f)
			|| ch == 0x0a
			|| ch == 0x0d
			|| ch == 0x80
			|| ch == 0x9f
			|| ch == 0xc2
			|| ch == 0xb0
			|| ch == 0xf8)) {
#if DEBUGx
			text_color_set(DW_COLOR_ERROR);
			dw_printf("sanity_check: FAILED.  Probably bogus info char 0x%02x\n", ch);
#endif
			return 0;
		}
	}

	return 1;
}


/* end hdlc_rec2.c */



//
//    This file is part of Dire Wolf, an amateur radio packet TNC.
// 
//    Copyright (C) 2011, 2012, 2013, 2015, 2019, 2021  John Langner, WB2OSZ
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


//#define DEBUG4 1	/* capture 9600 output to log files */


/*------------------------------------------------------------------
 *
 * Module:      demod_9600.c
 *
 * Purpose:   	Demodulator for baseband signal.
 *		This is used for AX.25 (with scrambling) and IL2P without.
 *
 * Input:	Audio samples from either a file or the "sound card."
 *
 * Outputs:	Calls hdlc_rec_bit() for each bit demodulated.
 *
 *---------------------------------------------------------------*/



 // Fine tuning for different demodulator types.
 // Don't remove this section.  It is here for a reason.


void gen_lowpass(float fc, float *lp_filter, int filter_size, bp_window_t wtype);
void hdlc_rec_bit(int chan, int subchan, int slice, int raw, int is_scrambled, int not_used_remove);

static float slice_point[MAX_SUBCHANS];


/* Add sample to buffer and shift the rest down. */


static inline void push_sample(float val, float *buff, int size)
{
	memmove(buff + 1, buff, (size - 1) * sizeof(float));
	buff[0] = val;
}


/* FIR filter kernel. */


static inline float convolve(const float *__restrict__ data, const float *__restrict__ filter, int filter_size)
{
	float sum = 0.0f;
	int j;

	//#pragma GCC ivdep				// ignored until gcc 4.9
	for (j = 0; j < filter_size; j++) {
		sum += filter[j] * data[j];
	}
	return (sum);
}

/* Automatic gain control. */
/* Result should settle down to 1 unit peak to peak.  i.e. -0.5 to +0.5 */


static inline float agc(float in, float fast_attack, float slow_decay, float *ppeak, float *pvalley)
{
	if (in >= *ppeak) {
		*ppeak = in * fast_attack + *ppeak * (1.0f - fast_attack);
	}
	else {
		*ppeak = in * slow_decay + *ppeak * (1.0f - slow_decay);
	}

	if (in <= *pvalley) {
		*pvalley = in * fast_attack + *pvalley * (1.0f - fast_attack);
	}
	else {
		*pvalley = in * slow_decay + *pvalley * (1.0f - slow_decay);
	}

	if (*ppeak > *pvalley) {
		return ((in - 0.5f * (*ppeak + *pvalley)) / (*ppeak - *pvalley));
	}
	return (0.0);
}


/*------------------------------------------------------------------
 *
 * Name:        demod_9600_init
 *
 * Purpose:     Initialize the 9600 (or higher) baud demodulator.
 *
 * Inputs:      modem_type	- Determines whether scrambling is used.
 *
 *		samples_per_sec	- Number of samples per second for audio.
 *
 *		upsample	- Factor to upsample the incoming stream.
 *				  After a lot of experimentation, I discovered that
 *				  it works better if the data is upsampled.
 *				  This reduces the jitter for PLL synchronization.
 *
 *		baud		- Data rate in bits per second.
 *
 *		D		- Address of demodulator state.
 *
 * Returns:     None
 *
 *----------------------------------------------------------------*/

void demod_9600_init(enum modem_t modem_type, int original_sample_rate, int upsample, int baud, struct demodulator_state_s *D)
{
	float fc;
	int j;
	if (upsample < 1) upsample = 1;
	if (upsample > 4) upsample = 4;


	memset(D, 0, sizeof(struct demodulator_state_s));
	D->modem_type = modem_type;
	D->num_slicers = 1;

	// Multiple profiles in future?

	//	switch (profile) {

	//	  case 'J':			// upsample x2 with filtering.
	//	  case 'K':			// upsample x3 with filtering.
	//	  case 'L':			// upsample x4 with filtering.


	D->lp_filter_len_bits = 1.0;	// -U4 = 61 	4.59 samples/symbol

	// Works best with odd number in some tests.  Even is better in others.
	//D->lp_filter_size = ((int) (0.5f * ( D->lp_filter_len_bits * (float)original_sample_rate / (float)baud ))) * 2 + 1;

	// Just round to nearest integer.
	D->lp_filter_size = (int)((D->lp_filter_len_bits * (float)original_sample_rate / baud) + 0.5f);

	D->lp_window = BP_WINDOW_COSINE;

	D->lpf_baud = 1.00;

	D->agc_fast_attack = 0.080;
	D->agc_slow_decay = 0.00012;

	D->pll_locked_inertia = 0.89;
	D->pll_searching_inertia = 0.67;

	//	    break;
	//	}

#if 0
	text_color_set(DW_COLOR_DEBUG);
	dw_printf("----------  %s  (%d, %d)  -----------\n", __func__, samples_per_sec, baud);
	dw_printf("filter_len_bits = %.2f\n", D->lp_filter_len_bits);
	dw_printf("lp_filter_size = %d\n", D->lp_filter_size);
	dw_printf("lp_window = %d\n", D->lp_window);
	dw_printf("lpf_baud = %.2f\n", D->lpf_baud);
	dw_printf("samples per bit = %.1f\n", (double)samples_per_sec / baud);
#endif


	// PLL needs to use the upsampled rate.

	D->pll_step_per_sample =
		(int)round(TICKS_PER_PLL_CYCLE * (double)baud / (double)(original_sample_rate * upsample));


#ifdef TUNE_LP_WINDOW
	D->lp_window = TUNE_LP_WINDOW;
#endif

#if TUNE_LP_FILTER_SIZE
	D->lp_filter_size = TUNE_LP_FILTER_SIZE;
#endif

#ifdef TUNE_LPF_BAUD
	D->lpf_baud = TUNE_LPF_BAUD;
#endif	

#ifdef TUNE_AGC_FAST
	D->agc_fast_attack = TUNE_AGC_FAST;
#endif

#ifdef TUNE_AGC_SLOW
	D->agc_slow_decay = TUNE_AGC_SLOW;
#endif

#if defined(TUNE_PLL_LOCKED)
	D->pll_locked_inertia = TUNE_PLL_LOCKED;
#endif

#if defined(TUNE_PLL_SEARCHING)
	D->pll_searching_inertia = TUNE_PLL_SEARCHING;
#endif

	// Initial filter (before scattering) is based on upsampled rate.

	fc = (float)baud * D->lpf_baud / (float)(original_sample_rate * upsample);

	//dw_printf ("demod_9600_init: call gen_lowpass(fc=%.2f, , size=%d, )\n", fc, D->lp_filter_size);

	gen_lowpass(fc, D->u.bb.lp_filter, D->lp_filter_taps * upsample, D->lp_window);

	// New in 1.7 -
	// Use a polyphase filter to reduce the CPU load.
	// Originally I used zero stuffing to upsample.
	// Here is the general idea.
	//
	// Suppose the input samples are 1 2 3 4 5 6 7 8 9 ...
	// Filter coefficients are a b c d e f g h i ...
	//
	// With original sampling rate, the filtering would involve multiplying and adding:
	//
	// 	1a 2b 3c 4d 5e 6f ...
	//
	// When upsampling by 3, each of these would need to be evaluated
	// for each audio sample:
	//
	//	1a 0b 0c 2d 0e 0f 3g 0h 0i ...
	//	0a 1b 0c 0d 2e 0f 0g 3h 0i ...
	//	0a 0b 1c 0d 0e 2f 0g 0h 3i ...
	//
	// 2/3 of the multiplies are always by a stuffed zero.
	// We can do this more efficiently by removing them.
	//
	//	1a       2d       3g       ...
	//	   1b       2e       3h    ...
	//	      1c       2f       3i ...
	//
	// We scatter the original filter across multiple shorter filters.
	// Each input sample cycles around them to produce the upsampled rate.
	//
	//	a d g ...
	//	b e h ...
	//	c f i ...
	//
	// There are countless sources of information DSP but this one is unique
	// in that it is a college course that mentions APRS.
	// https://www2.eecs.berkeley.edu/Courses/EE123
	//
	// Was the effort worthwhile?  Times on an RPi 3.
	//
	// command:   atest -B9600  ~/walkabout9600[abc]-compressed*.wav
	//
	// These are 3 recordings of a portable system being carried out of
	// range and back in again.  It is a real world test for weak signals.
	//
	//	options		num decoded	seconds		x realtime
	//			1.6	1.7	1.6	1.7	1.6	1.7
	//			---	---	---	---	---	---
	//	-P-		171	172	23.928	17.967	14.9	19.9
	//	-P+		180	180	54.688	48.772	6.5	7.3
	//	-P- -F1		177	178	32.686	26.517	10.9	13.5
	//
	// So, it turns out that -P+ doesn't have a dramatic improvement, only
	// around 4%, for drastically increased CPU requirements.
	// Maybe we should turn that off by default, especially for ARM.
	//

	int k = 0;
	for (int i = 0; i < D->lp_filter_size; i++) {
		D->u.bb.lp_polyphase_1[i] = D->u.bb.lp_filter[k++];
		if (upsample >= 2) {
			D->u.bb.lp_polyphase_2[i] = D->u.bb.lp_filter[k++];
			if (upsample >= 3) {
				D->u.bb.lp_polyphase_3[i] = D->u.bb.lp_filter[k++];
				if (upsample >= 4) {
					D->u.bb.lp_polyphase_4[i] = D->u.bb.lp_filter[k++];
				}
			}
		}
	}


	/* Version 1.2: Experiment with different slicing levels. */
	// Really didn't help that much because we should have a symmetrical signal.

	for (j = 0; j < MAX_SUBCHANS; j++) {
		slice_point[j] = 0.02f * (j - 0.5f * (MAX_SUBCHANS - 1));
		//dw_printf ("slice_point[%d] = %+5.2f\n", j, slice_point[j]);
	}

} /* end fsk_demod_init */



/*-------------------------------------------------------------------
 *
 * Name:        demod_9600_process_sample
 *
 * Purpose:     (1) Filter & slice the signal.
 *		(2) Descramble it.
 *		(2) Recover clock and data.
 *
 * Inputs:	chan	- Audio channel.  0 for left, 1 for right.
 *
 *		sam	- One sample of audio.
 *			  Should be in range of -32768 .. 32767.
 *
 * Returns:	None
 *
 * Descripion:	"9600 baud" packet is FSK for an FM voice transceiver.
 *		By the time it gets here, it's really a baseband signal.
 *		At one extreme, we could have a 4800 Hz square wave.
 *		A the other extreme, we could go a considerable number
 *		of bit times without any transitions.
 *
 *		The trick is to extract the digital data which has
 *		been distorted by going thru voice transceivers not
 *		intended to pass this sort of "audio" signal.
 *
 *		For G3RUH mode, data is "scrambled" to reduce the amount of DC bias.
 *		The data stream must be unscrambled at the receiving end.
 *
 *		We also have a digital phase locked loop (PLL)
 *		to recover the clock and pick out data bits at
 *		the proper rate.
 *
 *		For each recovered data bit, we call:
 *
 *			  hdlc_rec (channel, demodulated_bit);
 *
 *		to decode HDLC frames from the stream of bits.
 *
 * Future:	This could be generalized by passing in the name
 *		of the function to be called for each bit recovered
 *		from the demodulator.  For now, it's simply hard-coded.
 *
 *		After experimentation, I found that this works better if
 *		the original signal is upsampled by 2x or even 4x.
 *
 * References:	9600 Baud Packet Radio Modem Design
 *		http://www.amsat.org/amsat/articles/g3ruh/109.html
 *
 *		The KD2BD 9600 Baud Modem
 *		http://www.amsat.org/amsat/articles/kd2bd/9k6modem/
 *
 *		9600 Baud Packet Handbook
 * 		ftp://ftp.tapr.org/general/9600baud/96man2x0.txt
 *
 *
 *--------------------------------------------------------------------*/

inline static void nudge_pll(int chan, int subchan, int slice, float demod_out, struct demodulator_state_s *D);

static void process_filtered_sample(int chan, float fsam, struct demodulator_state_s *D);



void demod_9600_process_sample(int chan, int sam, int upsample, struct demodulator_state_s *D)
{
	float fsam;

#if DEBUG4
	static FILE *demod_log_fp = NULL;
	static int log_file_seq = 0;		/* Part of log file name */
#endif

	int subchan = 0;

	assert(chan >= 0 && chan < MAX_CHANS);
	assert(subchan >= 0 && subchan < MAX_SUBCHANS);

	/* Scale to nice number for convenience. */
	/* Consistent with the AFSK demodulator, we'd like to use */
	/* only half of the dynamic range to have some headroom. */
	/* i.e.  input range +-16k becomes +-1 here and is */
	/* displayed in the heard line as audio level 100. */

	fsam = (float)sam / 16384.0f;

	// Low pass filter
	push_sample(fsam, D->u.bb.audio_in, D->lp_filter_size);

	fsam = convolve(D->u.bb.audio_in, D->u.bb.lp_polyphase_1, D->lp_filter_size);
	process_filtered_sample(chan, fsam, D);
	if (upsample >= 2) {
		fsam = convolve(D->u.bb.audio_in, D->u.bb.lp_polyphase_2, D->lp_filter_size);
		process_filtered_sample(chan, fsam, D);
		if (upsample >= 3) {
			fsam = convolve(D->u.bb.audio_in, D->u.bb.lp_polyphase_3, D->lp_filter_size);
			process_filtered_sample(chan, fsam, D);
			if (upsample >= 4) {
				fsam = convolve(D->u.bb.audio_in, D->u.bb.lp_polyphase_4, D->lp_filter_size);
				process_filtered_sample(chan, fsam, D);
			}
		}
	}
}


static void process_filtered_sample(int chan, float fsam, struct demodulator_state_s *D)
{

	int subchan = 0;

	/*
	 * Version 1.2: Capture the post-filtering amplitude for display.
	 * This is similar to the AGC without the normalization step.
	 * We want decay to be substantially slower to get a longer
	 * range idea of the received audio.
	 * For AFSK, we keep mark and space amplitudes.
	 * Here we keep + and - peaks because there could be a DC bias.
	 */

	 // TODO:  probably no need for this.  Just use  D->m_peak, D->m_valley

	if (fsam >= D->alevel_mark_peak) {
		D->alevel_mark_peak = fsam * D->quick_attack + D->alevel_mark_peak * (1.0f - D->quick_attack);
	}
	else {
		D->alevel_mark_peak = fsam * D->sluggish_decay + D->alevel_mark_peak * (1.0f - D->sluggish_decay);
	}

	if (fsam <= D->alevel_space_peak) {
		D->alevel_space_peak = fsam * D->quick_attack + D->alevel_space_peak * (1.0f - D->quick_attack);
	}
	else {
		D->alevel_space_peak = fsam * D->sluggish_decay + D->alevel_space_peak * (1.0f - D->sluggish_decay);
	}

	/*
	 * The input level can vary greatly.
	 * More importantly, there could be a DC bias which we need to remove.
	 *
	 * Normalize the signal with automatic gain control (AGC).
	 * This works by looking at the minimum and maximum signal peaks
	 * and scaling the results to be roughly in the -1.0 to +1.0 range.
	 */
	float demod_out;
	int demod_data;				/* Still scrambled. */

	demod_out = agc(fsam, D->agc_fast_attack, D->agc_slow_decay, &(D->m_peak), &(D->m_valley));

	// TODO: There is potential for multiple decoders with one filter.

	//dw_printf ("peak=%.2f valley=%.2f fsam=%.2f norm=%.2f\n", D->m_peak, D->m_valley, fsam, norm);

	if (D->num_slicers <= 1) {

		/* Normal case of one demodulator to one HDLC decoder. */
		/* Demodulator output is difference between response from two filters. */
		/* AGC should generally keep this around -1 to +1 range. */

		demod_data = demod_out > 0;
		nudge_pll(chan, subchan, 0, demod_out, D);
	}
	else {
		int slice;

		/* Multiple slicers each feeding its own HDLC decoder. */

		for (slice = 0; slice < D->num_slicers; slice++) {
			demod_data = demod_out - slice_point[slice] > 0;
			nudge_pll(chan, subchan, slice, demod_out - slice_point[slice], D);
		}
	}

	// demod_data is used only for debug out.
	// suppress compiler warning about it not being used.
	(void)demod_data;

#if DEBUG4

	if (chan == 0) {

		if (1) {
			//if (D->slicer[slice].data_detect) {
			char fname[30];
			int slice = 0;

			if (demod_log_fp == NULL) {
				log_file_seq++;
				snprintf(fname, sizeof(fname), "demod/%04d.csv", log_file_seq);
				//if (log_file_seq == 1) mkdir ("demod", 0777);
				if (log_file_seq == 1) mkdir("demod");

				demod_log_fp = fopen(fname, "w");
				text_color_set(DW_COLOR_DEBUG);
				dw_printf("Starting demodulator log file %s\n", fname);
				fprintf(demod_log_fp, "Audio, Filtered,  Max,  Min, Normalized, Sliced, Clock\n");
			}

			fprintf(demod_log_fp, "%.3f, %.3f, %.3f, %.3f, %.3f, %d, %.2f\n",
				fsam + 6,
				fsam + 4,
				D->m_peak + 4,
				D->m_valley + 4,
				demod_out + 2,
				demod_data + 2,
				(D->slicer[slice].data_clock_pll & 0x80000000) ? .5 : .0);

			fflush(demod_log_fp);
		}
		else {
			if (demod_log_fp != NULL) {
				fclose(demod_log_fp);
				demod_log_fp = NULL;
			}
		}
	}
#endif

} /* end demod_9600_process_sample */


/*-------------------------------------------------------------------
 *
 * Name:        nudge_pll
 *
 * Purpose:	Update the PLL state for each audio sample.
 *
 *		(2) Descramble it.
 *		(2) Recover clock and data.
 *
 * Inputs:	chan	- Audio channel.  0 for left, 1 for right.
 *
 *		subchan	- Which demodulator.  We could have several running in parallel.
 *
 *		slice	- Determines which Slicing level & HDLC decoder to use.
 *
 *		demod_out_f - Demodulator output, possibly shifted by slicing level
 *				It will be compared with 0.0 to bit binary value out.
 *
 *		D	- Demodulator state for this channel / subchannel.
 *
 * Returns:	None
 *
 * Description:	A PLL is used to sample near the centers of the data bits.
 *
 *		D->data_clock_pll is a SIGNED 32 bit variable.
 *		When it overflows from a large positive value to a negative value, we
 *		sample a data bit from the demodulated signal.
 *
 *		Ideally, the the demodulated signal transitions should be near
 *		zero we we sample mid way between the transitions.
 *
 *		Nudge the PLL by removing some small fraction from the value of
 *		data_clock_pll, pushing it closer to zero.
 *
 *		This adjustment will never change the sign so it won't cause
 *		any erratic data bit sampling.
 *
 *		If we adjust it too quickly, the clock will have too much jitter.
 *		If we adjust it too slowly, it will take too long to lock on to a new signal.
 *
 *		I don't think the optimal value will depend on the audio sample rate
 *		because this happens for each transition from the demodulator.
 *
 * Version 1.4:	Previously, we would always pull the PLL phase toward 0 after
 *		after a zero crossing was detetected.  This adds extra jitter,
 *		especially when the ratio of audio sample rate to baud is low.
 *		Now, we interpolate between the two samples to get an estimate
 *		on when the zero crossing happened.  The PLL is pulled toward
 *		this point.
 *
 *		Results???  TBD
 *
 * Version 1.6:	New experiment where filter size to extract clock is not the same
 *		as filter to extract the data bit value.
 *
 *--------------------------------------------------------------------*/

inline static void nudge_pll(int chan, int subchan, int slice, float demod_out_f, struct demodulator_state_s *D)
{
	D->slicer[slice].prev_d_c_pll = D->slicer[slice].data_clock_pll;

	// Perform the add as unsigned to avoid signed overflow error.
	D->slicer[slice].data_clock_pll = (signed)((unsigned)(D->slicer[slice].data_clock_pll) + (unsigned)(D->pll_step_per_sample));

	if (D->slicer[slice].prev_d_c_pll > 1000000000 && D->slicer[slice].data_clock_pll < -1000000000) {

		/* Overflow.  Was large positive, wrapped around, now large negative. */

		hdlc_rec_bit(chan, subchan, slice, demod_out_f > 0, D->modem_type == MODEM_SCRAMBLE, D->slicer[slice].lfsr);
		pll_dcd_each_symbol2(D, chan, subchan, slice);
	}

	/*
	 * Zero crossing?
	 */
	if ((D->slicer[slice].prev_demod_out_f < 0 && demod_out_f > 0) ||
		(D->slicer[slice].prev_demod_out_f > 0 && demod_out_f < 0)) {

		// Note:  Test for this demodulator, not overall for channel.

		pll_dcd_signal_transition2(D, slice, D->slicer[slice].data_clock_pll);

		float target = D->pll_step_per_sample * demod_out_f / (demod_out_f - D->slicer[slice].prev_demod_out_f);

		if (D->slicer[slice].data_detect) {
			D->slicer[slice].data_clock_pll = (int)(D->slicer[slice].data_clock_pll * D->pll_locked_inertia + target * (1.0f - D->pll_locked_inertia));
		}
		else {
			D->slicer[slice].data_clock_pll = (int)(D->slicer[slice].data_clock_pll * D->pll_searching_inertia + target * (1.0f - D->pll_searching_inertia));
		}
	}


#if DEBUG5

	//if (chan == 0) {
	if (D->slicer[slice].data_detect) {

		char fname[30];


		if (demod_log_fp == NULL) {
			seq++;
			snprintf(fname, sizeof(fname), "demod96/%04d.csv", seq);
			if (seq == 1) mkdir("demod96"
#ifndef __WIN32__
				, 0777
#endif
			);

			demod_log_fp = fopen(fname, "w");
			text_color_set(DW_COLOR_DEBUG);
			dw_printf("Starting 9600 decoder log file %s\n", fname);
			fprintf(demod_log_fp, "Audio, Peak, Valley, Demod, SData, Descram, Clock\n");
		}
		fprintf(demod_log_fp, "%.3f, %.3f, %.3f, %.3f, %.2f, %.2f, %.2f\n",
			0.5f * fsam + 3.5,
			0.5f * D->m_peak + 3.5,
			0.5f * D->m_valley + 3.5,
			0.5f * demod_out + 2.0,
			demod_data ? 1.35 : 1.0,
			descram ? .9 : .55,
			(D->data_clock_pll & 0x80000000) ? .1 : .45);
	}
	else {
		if (demod_log_fp != NULL) {
			fclose(demod_log_fp);
			demod_log_fp = NULL;
		}
	}
	//}

#endif


/*
 * Remember demodulator output (pre-descrambling) so we can compare next time
 * for the DPLL sync.
 */
	D->slicer[slice].prev_demod_out_f = demod_out_f;

} /* end nudge_pll */


/* end demod_9600.c */




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


/********************************************************************************
 *
 * File:	rrbb.c
 *
 * Purpose:	Raw Received Bit Buffer.
 *		An array of bits used to hold data out of
 *		the demodulator before feeding it into the HLDC decoding.
 *
 * Version 1.2: Save initial state of 9600 baud descrambler so we can
 *		attempt bit fix up on G3RUH/K9NG scrambled data.
 *
 * Version 1.3:	Store as bytes rather than packing 8 bits per byte.
 *
 *******************************************************************************/





#define MAGIC1 0x12344321
#define MAGIC2 0x56788765


static int new_count = 0;
static int delete_count = 0;


/***********************************************************************************
 *
 * Name:	rrbb_new
 *
 * Purpose:	Allocate space for an array of samples.
 *
 * Inputs:	chan	- Radio channel from whence it came.
 *
 *		subchan	- Which demodulator of the channel.
 *
 *		slice	- multiple thresholds per demodulator.
 *
 *		is_scrambled - Is data scrambled? (true, false)
 *
 *		descram_state - State of data descrambler.
 *
 *		prev_descram - Previous descrambled bit.
 *
 * Returns:	Handle to be used by other functions.
 *
 * Description:
 *
 ***********************************************************************************/

rrbb_t rrbb_new(int chan, int subchan, int slice, int is_scrambled, int descram_state, int prev_descram)
{
	rrbb_t result;

	result = malloc(sizeof(struct rrbb_s));
	if (result == NULL) {
		text_color_set(DW_COLOR_ERROR);
		dw_printf("FATAL ERROR: Out of memory.\n");
		exit(0);
	}
	result->magic1 = MAGIC1;
	result->chan = chan;
	result->subchan = subchan;
	result->slice = slice;
	result->magic2 = MAGIC2;

	new_count++;

	if (new_count > delete_count + 100) {
		text_color_set(DW_COLOR_ERROR);
		dw_printf("MEMORY LEAK, rrbb_new, new_count=%d, delete_count=%d\n", new_count, delete_count);
	}

	rrbb_clear(result, is_scrambled, descram_state, prev_descram);

	return (result);
}

/***********************************************************************************
 *
 * Name:	rrbb_clear
 *
 * Purpose:	Clear by setting length to zero, etc.
 *
 * Inputs:	b 		-Handle for sample array.
 *
 *		is_scrambled 	- Is data scrambled? (true, false)
 *
 *		descram_state 	- State of data descrambler.
 *
 *		prev_descram 	- Previous descrambled bit.
 *
 ***********************************************************************************/

void rrbb_clear(rrbb_t b, int is_scrambled, int descram_state, int prev_descram)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	assert(is_scrambled == 0 || is_scrambled == 1);
	assert(prev_descram == 0 || prev_descram == 1);

	b->nextp = NULL;

	b->alevel.rec = 9999;	// TODO: was there some reason for this instead of 0 or -1?
	b->alevel.mark = 9999;
	b->alevel.space = 9999;

	b->len = 0;

	b->is_scrambled = is_scrambled;
	b->descram_state = descram_state;
	b->prev_descram = prev_descram;
}


/***********************************************************************************
 *
 * Name:	rrbb_append_bit
 *
 * Purpose:	Append another bit to the end.
 *
 * Inputs:	Handle for sample array.
 *		Value for the sample.
 *
 ***********************************************************************************/

 /* Definition in header file so it can be inlined. */


 /***********************************************************************************
  *
  * Name:	rrbb_chop8
  *
  * Purpose:	Remove 8 from the length.
  *
  * Inputs:	Handle for bit array.
  *
  * Description:	Back up after appending the flag sequence.
  *
  ***********************************************************************************/

void rrbb_chop8(rrbb_t b)
{

	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	if (b->len >= 8) {
		b->len -= 8;
	}
}

/***********************************************************************************
 *
 * Name:	rrbb_get_len
 *
 * Purpose:	Get number of bits in the array.
 *
 * Inputs:	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_len(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->len);
}



/***********************************************************************************
 *
 * Name:	rrbb_get_bit
 *
 * Purpose:	Get value of bit in specified position.
 *
 * Inputs:	Handle for sample array.
 *		Index into array.
 *
 ***********************************************************************************/

 /* Definition in header file so it can be inlined. */




 /***********************************************************************************
  *
  * Name:	rrbb_flip_bit
  *
  * Purpose:	Complement the value of bit in specified position.
  *
  * Inputs:	Handle for bit array.
  *		Index into array.
  *
  ***********************************************************************************/

  //void rrbb_flip_bit (rrbb_t b, unsigned int ind)
  //{
  //	unsigned int di, mi;
  //
  //	assert (b != NULL);
  //	assert (b->magic1 == MAGIC1);
  //	assert (b->magic2 == MAGIC2);
  //
  //	assert (ind < b->len);
  //
  //	di = ind / SOI;
  //	mi = ind % SOI;
  //
  //	b->data[di] ^= masks[mi];
  //}

  /***********************************************************************************
   *
   * Name:	rrbb_delete
   *
   * Purpose:	Free the storage associated with the bit array.
   *
   * Inputs:	Handle for bit array.
   *
   ***********************************************************************************/

void rrbb_delete(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	b->magic1 = 0;
	b->magic2 = 0;

	free(b);

	delete_count++;
}


/***********************************************************************************
 *
 * Name:	rrbb_set_netxp
 *
 * Purpose:	Set the nextp field, used to maintain a queue.
 *
 * Inputs:	b	Handle for bit array.
 *		np	New value for nextp.
 *
 ***********************************************************************************/

void rrbb_set_nextp(rrbb_t b, rrbb_t np)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	b->nextp = np;
}


/***********************************************************************************
 *
 * Name:	rrbb_get_netxp
 *
 * Purpose:	Get value of nextp field.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

rrbb_t rrbb_get_nextp(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->nextp);
}

/***********************************************************************************
 *
 * Name:	rrbb_get_chan
 *
 * Purpose:	Get channel from which bit buffer was received.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_chan(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	assert(b->chan >= 0 && b->chan < MAX_CHANS);

	return (b->chan);
}


/***********************************************************************************
 *
 * Name:	rrbb_get_subchan
 *
 * Purpose:	Get subchannel from which bit buffer was received.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_subchan(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	assert(b->subchan >= 0 && b->subchan < MAX_SUBCHANS);

	return (b->subchan);
}


/***********************************************************************************
 *
 * Name:	rrbb_get_slice
 *
 * Purpose:	Get slice number from which bit buffer was received.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_slice(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	assert(b->slice >= 0 && b->slice < MAX_SLICERS);

	return (b->slice);
}


/***********************************************************************************
 *
 * Name:	rrbb_set_audio_level
 *
 * Purpose:	Set audio level at time the frame was received.
 *
 * Inputs:	b	Handle for bit array.
 *		alevel	Audio level.
 *
 ***********************************************************************************/

void rrbb_set_audio_level(rrbb_t b, alevel_t alevel)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	b->alevel = alevel;
}


/***********************************************************************************
 *
 * Name:	rrbb_get_audio_level
 *
 * Purpose:	Get audio level at time the frame was received.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

alevel_t rrbb_get_audio_level(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->alevel);
}



/***********************************************************************************
 *
 * Name:	rrbb_get_is_scrambled
 *
 * Purpose:	Find out if using scrambled data.
 *
 * Inputs:	b	Handle for bit array.
 *
 * Returns:	True (for 9600 baud) or false (for slower AFSK).
 *
 ***********************************************************************************/

int rrbb_get_is_scrambled(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->is_scrambled);
}



/***********************************************************************************
 *
 * Name:	rrbb_get_descram_state
 *
 * Purpose:	Get data descrambler state before first data bit of frame.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_descram_state(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->descram_state);
}


/***********************************************************************************
 *
 * Name:	rrbb_get_prev_descram
 *
 * Purpose:	Get previous descrambled bit before first data bit of frame.
 *
 * Inputs:	b	Handle for bit array.
 *
 ***********************************************************************************/

int rrbb_get_prev_descram(rrbb_t b)
{
	assert(b != NULL);
	assert(b->magic1 == MAGIC1);
	assert(b->magic2 == MAGIC2);

	return (b->prev_descram);
}


/* end rrbb.c */



//
//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2011, 2014, 2015, 2016, 2019  John Langner, WB2OSZ
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


/*------------------------------------------------------------------
 *
 * Module:      gen_tone.c
 *
 * Purpose:     Convert bits to AFSK for writing to .WAV sound file
 *		or a sound device.
 *
 *
 *---------------------------------------------------------------*/

// Properties of the digitized sound stream & modem.

static struct audio_s *save_audio_config_p = NULL;

/*
 * 8 bit samples are unsigned bytes in range of 0 .. 255.
 *
 * 16 bit samples are signed short in range of -32768 .. +32767.
 */


 /* Constants after initialization. */

#define TICKS_PER_CYCLE ( 256.0 * 256.0 * 256.0 * 256.0 )

static int ticks_per_sample[MAX_CHANS];	/* Same for both channels of same soundcard */
					/* because they have same sample rate */
					/* but less confusing to have for each channel. */

static int ticks_per_bit[MAX_CHANS];
static int f1_change_per_sample[MAX_CHANS];
static int f2_change_per_sample[MAX_CHANS];


static short sine_table[256];


/* Accumulators. */

static unsigned int tone_phase[MAX_CHANS]; // Phase accumulator for tone generation.
					   // Upper bits are used as index into sine table.

#define PHASE_SHIFT_180 ( 128u << 24 )
#define PHASE_SHIFT_90  (  64u << 24 )
#define PHASE_SHIFT_45  (  32u << 24 )


static int bit_len_acc[MAX_CHANS];	// To accumulate fractional samples per bit.

static int lfsr[MAX_CHANS];		// Shift register for scrambler.

static int bit_count[MAX_CHANS];	// Counter incremented for each bit transmitted
					// on the channel.   This is only used for QPSK.
					// The LSB determines if we save the bit until
					// next time, or send this one with the previously saved.
					// The LSB+1 position determines if we add an
					// extra 180 degrees to the phase to compensate
					// for having 1.5 carrier cycles per symbol time.

					// For 8PSK, it has a different meaning.  It is the
					// number of bits in 'save_bit' so we can accumulate
					// three for each symbol.
static int save_bit[MAX_CHANS];


static int prev_dat[MAX_CHANS];		// Previous data bit.  Used for G3RUH style.




/*------------------------------------------------------------------
 *
 * Name:        gen_tone_init
 *
 * Purpose:     Initialize for AFSK tone generation which might
 *		be used for RTTY or amateur packet radio.
 *
 * Inputs:      audio_config_p		- Pointer to modem parameter structure, modem_s.
 *
 *				The fields we care about are:
 *
 *					samples_per_sec
 *					baud
 *					mark_freq
 *					space_freq
 *					samples_per_sec
 *
 *		amp		- Signal amplitude on scale of 0 .. 100.
 *
 *				  100% uses the full 16 bit sample range of +-32k.
 *
 *		gen_packets	- True if being called from "gen_packets" utility
 *				  rather than the "direwolf" application.
 *
 * Returns:     0 for success.
 *              -1 for failure.
 *
 * Description:	 Calculate various constants for use by the direct digital synthesis
 * 		audio tone generation.
 *
 *----------------------------------------------------------------*/

static int amp16bit;	/* for 9600 baud */


int gen_tone_init(struct audio_s *pa, int amp, int gen_packets)
{
	int j;
	int chan = 0;

#if DEBUG
	text_color_set(DW_COLOR_DEBUG);
	dw_printf("gen_tone_init ( audio_config_p=%p, amp=%d, gen_packets=%d )\n",
		audio_config_p, amp, gen_packets);
#endif

	/*
	 * Save away modem parameters for later use.
	 */

	// This should be in config somewhere

	pa->adev[0].num_channels = DEFAULT_NUM_CHANNELS;
	pa->adev[0].samples_per_sec = 48000;
	pa->adev[0].bits_per_sample = 16;

	pa->achan[0].baud = rx_baudrate[0];

	pa->adev[1].num_channels = DEFAULT_NUM_CHANNELS;
	pa->adev[1].samples_per_sec = 48000;
	pa->adev[1].bits_per_sample = 16;

	pa->achan[1].baud = rx_baudrate[1];

	pa->chan_medium[0] = MEDIUM_RADIO;
	pa->chan_medium[1] = MEDIUM_RADIO;

	pa->achan[0].modem_type = MODEM_SCRAMBLE;
	pa->achan[1].modem_type = MODEM_SCRAMBLE;

	save_audio_config_p = pa;

	amp16bit = (int)((32767 * amp) / 100);

	for (chan = 0; chan < MAX_CHANS; chan++) {

		if (pa->chan_medium[chan] == MEDIUM_RADIO) {

			int a = ACHAN2ADEV(chan);

#if DEBUG
			text_color_set(DW_COLOR_DEBUG);
			dw_printf("gen_tone_init: chan=%d, modem_type=%d, bps=%d, samples_per_sec=%d\n",
				chan,
				save_pa->achan[chan].modem_type,
				pa->achan[chan].baud,
				pa->adev[a].samples_per_sec);
#endif

			tone_phase[chan] = 0;
			bit_len_acc[chan] = 0;
			lfsr[chan] = 0;

			ticks_per_sample[chan] = (int)((TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);

			// The terminology is all wrong here.  Didn't matter with 1200 and 9600.
			// The config speed should be bits per second rather than baud.
			// ticks_per_bit should be ticks_per_symbol.

			switch (pa->achan[chan].modem_type) {

			case MODEM_QPSK:

				pa->achan[chan].mark_freq = 1800;
				pa->achan[chan].space_freq = pa->achan[chan].mark_freq;	// Not Used.

				// symbol time is 1 / (half of bps)
				ticks_per_bit[chan] = (int)((TICKS_PER_CYCLE / ((double)pa->achan[chan].baud * 0.5)) + 0.5);
				f1_change_per_sample[chan] = (int)(((double)pa->achan[chan].mark_freq * TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);
				f2_change_per_sample[chan] = f1_change_per_sample[chan];	// Not used.

				tone_phase[chan] = PHASE_SHIFT_45;	// Just to mimic first attempt.
				break;

			case MODEM_8PSK:

				pa->achan[chan].mark_freq = 1800;
				pa->achan[chan].space_freq = pa->achan[chan].mark_freq;	// Not Used.

				// symbol time is 1 / (third of bps)
				ticks_per_bit[chan] = (int)((TICKS_PER_CYCLE / ((double)pa->achan[chan].baud / 3.)) + 0.5);
				f1_change_per_sample[chan] = (int)(((double)pa->achan[chan].mark_freq * TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);
				f2_change_per_sample[chan] = f1_change_per_sample[chan];	// Not used.
				break;

			case MODEM_BASEBAND:
			case MODEM_SCRAMBLE:
			case MODEM_AIS:

				// Tone is half baud.
				ticks_per_bit[chan] = (int)((TICKS_PER_CYCLE / (double)pa->achan[chan].baud) + 0.5);
				f1_change_per_sample[chan] = (int)(((double)pa->achan[chan].baud * 0.5 * TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);
				break;

			default:		// AFSK

				ticks_per_bit[chan] = (int)((TICKS_PER_CYCLE / (double)pa->achan[chan].baud) + 0.5);
				f1_change_per_sample[chan] = (int)(((double)pa->achan[chan].mark_freq * TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);
				f2_change_per_sample[chan] = (int)(((double)pa->achan[chan].space_freq * TICKS_PER_CYCLE / (double)pa->adev[a].samples_per_sec) + 0.5);
				break;
			}
		}
	}

	for (j = 0; j < 256; j++) {
		double a;
		int s;

		a = ((double)(j) / 256.0) * (2 * M_PI);
		s = (int)(sin(a) * 32767 * amp / 100.0);

		/* 16 bit sound sample must fit in range of -32768 .. +32767. */

		if (s < -32768) {
			text_color_set(DW_COLOR_ERROR);
			dw_printf("gen_tone_init: Excessive amplitude is being clipped.\n");
			s = -32768;
		}
		else if (s > 32767) {
			text_color_set(DW_COLOR_ERROR);
			dw_printf("gen_tone_init: Excessive amplitude is being clipped.\n");
			s = 32767;
		}
		sine_table[j] = s;
	}

	return (0);

} /* end gen_tone_init */


/*-------------------------------------------------------------------
 *
 * Name:        tone_gen_put_bit
 *
 * Purpose:     Generate tone of proper duration for one data bit.
 *
 * Inputs:      chan	- Audio channel, 0 = first.
 *
 *		dat	- 0 for f1, 1 for f2.
 *
 * 			  	-1 inserts half bit to test data
 *				recovery PLL.
 *
 * Assumption:  fp is open to a file for write.
 *
 * Version 1.4:	Attempt to implement 2400 and 4800 bps PSK modes.
 *
 * Version 1.6: For G3RUH, rather than generating square wave and low
 *		pass filtering, generate the waveform directly.
 *		This avoids overshoot, ringing, and adding more jitter.
 *		Alternating bits come out has sine wave of baud/2 Hz.
 *
 * Version 1.6:	MFJ-2400 compatibility for V.26.
 *
 *--------------------------------------------------------------------*/

static const int gray2phase_v26[4] = { 0, 1, 3, 2 };
static const int gray2phase_v27[8] = { 1, 0, 2, 3, 6, 7, 5, 4 };

// We are only using this for RUH modes

void tone_gen_put_bit(int chan, int dat, int scramble)
{
	int a = 0;

	// scramble

	if (scramble)
	{
		int x;

		x = (dat ^ (lfsr[chan] >> 16) ^ (lfsr[chan] >> 11)) & 1;
		lfsr[chan] = (lfsr[chan] << 1) | (x & 1);
		dat = x;
	}

	do {		/* until enough audio samples for this symbol. */

		int sam;

		if (dat != prev_dat[chan])
		{
			tone_phase[chan] += f1_change_per_sample[chan];
		}
		else
		{
			if (tone_phase[chan] & 0x80000000)
				tone_phase[chan] = 0xc0000000;	// 270 degrees.
			else
				tone_phase[chan] = 0x40000000;	// 90 degrees.
		}

		sam = sine_table[(tone_phase[chan] >> 24) & 0xff];
		gen_tone_put_sample(chan, a, sam);


		/* Enough for the bit time? */

		bit_len_acc[chan] += ticks_per_sample[chan];

	} while (bit_len_acc[chan] < ticks_per_bit[chan]);

	bit_len_acc[chan] -= ticks_per_bit[chan];

	prev_dat[chan] = dat;		// Only needed for G3RUH baseband/scrambled.

}  /* end tone_gen_put_bit */

#define ARDOPBufferSize 12000 * 100					// May need to be bigger for 48K

extern short ARDOPTXBuffer[4][ARDOPBufferSize];	// Enough to hold whole frame of samples

void gen_tone_put_sample(int chan, int a, int sam) 
{
	// This replaces the DW code

	ARDOPTXBuffer[chan][SampleNo++] = sam;
}




/* end gen_tone.c */



//
//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2011, 2013, 2014, 2019, 2021  John Langner, WB2OSZ
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


static void send_control_nrzi(int, int);
static void send_data_nrzi(int, int);
static void send_bit_nrzi(int, int);

static int number_of_bits_sent[MAX_CHANS];	// Count number of bits sent by "hdlc_send_frame" or "hdlc_send_flags"

/*-------------------------------------------------------------
 *
 * Name:	layer2_send_frame
 *
 * Purpose:	Convert frames to a stream of bits.
 *		Originally this was for AX.25 only, hence the file name.
 *		Over time, FX.25 and IL2P were shoehorned in.
 *
 * Inputs:	chan	- Audio channel number, 0 = first.
 *
 *		pp	- Packet object.
 *
 *		bad_fcs	- Append an invalid FCS for testing purposes.
 *			  Applies only to regular AX.25.
 *
 * Outputs:	Bits are shipped out by calling tone_gen_put_bit().
 *
 * Returns:	Number of bits sent including "flags" and the
 *		stuffing bits.
 *		The required time can be calculated by dividing this
 *		number by the transmit rate of bits/sec.
 *
 * Description:	For AX.25, send:
 *			start flag
 *			bit stuffed data
 *			calculated FCS
 *			end flag
 *		NRZI encoding for all but the "flags."
 *
 *
 * Assumptions:	It is assumed that the tone_gen module has been
 *		properly initialized so that bits sent with
 *		tone_gen_put_bit() are processed correctly.
 *
 *--------------------------------------------------------------*/

static int ax25_only_hdlc_send_frame(int chan, unsigned char *fbuf, int flen, int bad_fcs);


static int ax25_only_hdlc_send_frame(int chan, unsigned char *fbuf, int flen, int bad_fcs)
{
	int j, fcs;


	number_of_bits_sent[chan] = 0;

#if DEBUG
	text_color_set(DW_COLOR_DEBUG);
	dw_printf("hdlc_send_frame ( chan = %d, fbuf = %p, flen = %d, bad_fcs = %d)\n", chan, fbuf, flen, bad_fcs);
	fflush(stdout);
#endif

	send_control_nrzi(chan, 0x7e);	/* Start frame */

	for (j = 0; j < flen; j++) {
		send_data_nrzi(chan, fbuf[j]);
	}

	fcs = get_fcs(fbuf, flen);

	if (bad_fcs) {
		/* For testing only - Simulate a frame getting corrupted along the way. */
		send_data_nrzi(chan, (~fcs) & 0xff);
		send_data_nrzi(chan, ((~fcs) >> 8) & 0xff);
	}
	else {
		send_data_nrzi(chan, fcs & 0xff);
		send_data_nrzi(chan, (fcs >> 8) & 0xff);
	}

	send_control_nrzi(chan, 0x7e);	/* End frame */

	return (number_of_bits_sent[chan]);
}


// The following are only for HDLC.
// All bits are sent NRZI.
// Data (non flags) use bit stuffing.


static int stuff[MAX_CHANS];		// Count number of "1" bits to keep track of when we
					// need to break up a long run by "bit stuffing."
					// Needs to be array because we could be transmitting
					// on multiple channels at the same time.

static void send_control_nrzi(int chan, int x)
{
	int i;

	for (i = 0; i < 8; i++) {
		send_bit_nrzi(chan, x & 1);
		x >>= 1;
	}

	stuff[chan] = 0;
}

static void send_data_nrzi(int chan, int x)
{
	int i;

	for (i = 0; i < 8; i++) {
		send_bit_nrzi(chan, x & 1);
		if (x & 1) {
			stuff[chan]++;
			if (stuff[chan] == 5) {
				send_bit_nrzi(chan, 0);
				stuff[chan] = 0;
			}
		}
		else {
			stuff[chan] = 0;
		}
		x >>= 1;
	}
}

/*
 * NRZI encoding.
 * data 1 bit -> no change.
 * data 0 bit -> invert signal.
 */

static void send_bit_nrzi(int chan, int b)
{
	static int output[MAX_CHANS];

	if (b == 0) {
		output[chan] = !output[chan];
	}

	tone_gen_put_bit(chan, output[chan], MODEM_SCRAMBLE);

	number_of_bits_sent[chan]++;
}

/* end hdlc_send.c */



//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2011, 2012, 2013, 2015, 2019  John Langner, WB2OSZ
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


/*------------------------------------------------------------------
 *
 * Name:        dsp.c
 *
 * Purpose:     Generate the filters used by the demodulators.
 *
 *----------------------------------------------------------------*/


#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))


 // Don't remove this.  It serves as a reminder that an experiment is underway.

#if defined(TUNE_MS_FILTER_SIZE) || defined(TUNE_MS2_FILTER_SIZE) || defined(TUNE_AGC_FAST) || defined(TUNE_LPF_BAUD) || defined(TUNE_PLL_LOCKED) || defined(TUNE_PROFILE)
#define DEBUG1 1		// Don't remove this.
#endif


/*------------------------------------------------------------------
 *
 * Name:        window
 *
 * Purpose:     Filter window shape functions.
 *
 * Inputs:   	type	- BP_WINDOW_HAMMING, etc.
 *		size	- Number of filter taps.
 *		j	- Index in range of 0 to size-1.
 *
 * Returns:     Multiplier for the window shape.
 *
 *----------------------------------------------------------------*/

	static float window(bp_window_t type, int size, int j)
{
	float center;
	float w;

	center = 0.5 * (size - 1);

	switch (type) {

	case BP_WINDOW_COSINE:
		w = cos((j - center) / size * M_PI);
		//w = sin(j * M_PI / (size - 1));
		break;

	case BP_WINDOW_HAMMING:
		w = 0.53836 - 0.46164 * cos((j * 2 * M_PI) / (size - 1));
		break;

	case BP_WINDOW_BLACKMAN:
		w = 0.42659 - 0.49656 * cos((j * 2 * M_PI) / (size - 1))
			+ 0.076849 * cos((j * 4 * M_PI) / (size - 1));
		break;

	case BP_WINDOW_FLATTOP:
		w = 1.0 - 1.93  * cos((j * 2 * M_PI) / (size - 1))
			+ 1.29  * cos((j * 4 * M_PI) / (size - 1))
			- 0.388 * cos((j * 6 * M_PI) / (size - 1))
			+ 0.028 * cos((j * 8 * M_PI) / (size - 1));
		break;

	case BP_WINDOW_TRUNCATED:
	default:
		w = 1.0;
		break;
	}
	return (w);
}


/*------------------------------------------------------------------
 *
 * Name:        gen_lowpass
 *
 * Purpose:     Generate low pass filter kernel.
 *
 * Inputs:   	fc		- Cutoff frequency as fraction of sampling frequency.
 *		filter_size	- Number of filter taps.
 *		wtype		- Window type, BP_WINDOW_HAMMING, etc.
 *		lp_delay_fract	- Fudge factor for the delay value.
 *
 * Outputs:     lp_filter
 *
 * Returns:	Signal delay thru the filter in number of audio samples.
 *
 *----------------------------------------------------------------*/


void gen_lowpass(float fc, float *lp_filter, int filter_size, bp_window_t wtype)
{
	int j;
	float G;


#if DEBUG1
	text_color_set(DW_COLOR_DEBUG);

	dw_printf("Lowpass, size=%d, fc=%.2f\n", filter_size, fc);
	dw_printf("   j     shape   sinc   final\n");
#endif

	assert(filter_size >= 3 && filter_size <= MAX_FILTER_SIZE);

	for (j = 0; j < filter_size; j++) {
		float center;
		float sinc;
		float shape;

		center = 0.5 * (filter_size - 1);

		if (j - center == 0) {
			sinc = 2 * fc;
		}
		else {
			sinc = sin(2 * M_PI * fc * (j - center)) / (M_PI*(j - center));
		}

		shape = window(wtype, filter_size, j);
		lp_filter[j] = sinc * shape;

#if DEBUG1
		dw_printf("%6d  %6.2f  %6.3f  %6.3f\n", j, shape, sinc, lp_filter[j]);
#endif
	}

	/*
	 * Normalize lowpass for unity gain at DC.
	 */
	G = 0;
	for (j = 0; j < filter_size; j++) {
		G += lp_filter[j];
	}
	for (j = 0; j < filter_size; j++) {
		lp_filter[j] = lp_filter[j] / G;
	}

	return;

}  /* end gen_lowpass */


#undef DEBUG1



/*------------------------------------------------------------------
 *
 * Name:        gen_bandpass
 *
 * Purpose:     Generate band pass filter kernel for the prefilter.
 *		This is NOT for the mark/space filters.
 *
 * Inputs:   	f1		- Lower cutoff frequency as fraction of sampling frequency.
 *		f2		- Upper cutoff frequency...
 *		filter_size	- Number of filter taps.
 *		wtype		- Window type, BP_WINDOW_HAMMING, etc.
 *
 * Outputs:     bp_filter
 *
 * Reference:	http://www.labbookpages.co.uk/audio/firWindowing.html
 *
 *		Does it need to be an odd length?
 *
 *----------------------------------------------------------------*/


void gen_bandpass(float f1, float f2, float *bp_filter, int filter_size, bp_window_t wtype)
{
	int j;
	float w;
	float G;
	float center = 0.5 * (filter_size - 1);

#if DEBUG1
	text_color_set(DW_COLOR_DEBUG);

	dw_printf("Bandpass, size=%d\n", filter_size);
	dw_printf("   j     shape   sinc   final\n");
#endif

	assert(filter_size >= 3 && filter_size <= MAX_FILTER_SIZE);

	for (j = 0; j < filter_size; j++) {
		float sinc;
		float shape;

		if (j - center == 0) {
			sinc = 2 * (f2 - f1);
		}
		else {
			sinc = sin(2 * M_PI * f2 * (j - center)) / (M_PI*(j - center))
				- sin(2 * M_PI * f1 * (j - center)) / (M_PI*(j - center));
		}

		shape = window(wtype, filter_size, j);
		bp_filter[j] = sinc * shape;

#if DEBUG1
		dw_printf("%6d  %6.2f  %6.3f  %6.3f\n", j, shape, sinc, bp_filter[j]);
#endif
	}


	/*
	 * Normalize bandpass for unity gain in middle of passband.
	 * Can't use same technique as for lowpass.
	 * Instead compute gain in middle of passband.
	 * See http://dsp.stackexchange.com/questions/4693/fir-filter-gain
	 */
	w = 2 * M_PI * (f1 + f2) / 2;
	G = 0;
	for (j = 0; j < filter_size; j++) {
		G += 2 * bp_filter[j] * cos((j - center)*w);  // is this correct?
	}

#if DEBUG1
	dw_printf("Before normalizing, G=%.3f\n", G);
#endif
	for (j = 0; j < filter_size; j++) {
		bp_filter[j] = bp_filter[j] / G;
	}

} /* end gen_bandpass */



/*------------------------------------------------------------------
 *
 * Name:        gen_ms
 *
 * Purpose:     Generate mark and space filters.
 *
 * Inputs:   	fc		- Tone frequency, i.e. mark or space.
 *		sps		- Samples per second.
 *		filter_size	- Number of filter taps.
 *		wtype		- Window type, BP_WINDOW_HAMMING, etc.
 *
 * Outputs:     bp_filter
 *
 * Reference:	http://www.labbookpages.co.uk/audio/firWindowing.html
 *
 *		Does it need to be an odd length?
 *
 *----------------------------------------------------------------*/


void gen_ms(int fc, int sps, float *sin_table, float *cos_table, int filter_size, int wtype)
{
	int j;
	float Gs = 0, Gc = 0;;

	for (j = 0; j < filter_size; j++) {

		float center = 0.5f * (filter_size - 1);
		float am = ((float)(j - center) / (float)sps) * ((float)fc) * (2.0f * (float)M_PI);

		float shape = window(wtype, filter_size, j);

		sin_table[j] = sinf(am) * shape;
		cos_table[j] = cosf(am) * shape;

		Gs += sin_table[j] * sinf(am);
		Gc += cos_table[j] * cosf(am);

#if DEBUG1
		dw_printf("%6d  %6.2f  %6.2f  %6.2f\n", j, shape, sin_table[j], cos_table[j]);
#endif
	}


	/* Normalize for unity gain */

#if DEBUG1
	dw_printf("Before normalizing, Gs = %.2f, Gc = %.2f\n", Gs, Gc);
#endif
	for (j = 0; j < filter_size; j++) {
		sin_table[j] = sin_table[j] / Gs;
		cos_table[j] = cos_table[j] / Gc;
	}

} /* end gen_ms */





/*------------------------------------------------------------------
 *
 * Name:        rrc
 *
 * Purpose:     Root Raised Cosine function.
 *		Why do they call it that?
 *		It's mostly the sinc function with cos windowing to taper off edges faster.
 *
 * Inputs:      t		- Time in units of symbol duration.
 *				  i.e. The centers of two adjacent symbols would differ by 1.
 *
 *		a		- Roll off factor, between 0 and 1.
 *
 * Returns:	Basically the sinc  (sin(x)/x) function with edges decreasing faster.
 *		Should be 1 for t = 0 and 0 at all other integer values of t.
 *
 *----------------------------------------------------------------*/


float rrc(float t, float a)
{
	float sinc, window, result;

	if (t > -0.001 && t < 0.001) {
		sinc = 1;
	}
	else {
		sinc = sinf(M_PI * t) / (M_PI * t);
	}

	if (fabsf(a * t) > 0.499 && fabsf(a * t) < 0.501) {
		window = M_PI / 4;
	}
	else {
		window = cos(M_PI * a * t) / (1 - powf(2 * a * t, 2));
		// This made nicer looking waveforms for generating signal.
		//window = cos(M_PI * a * t);
		// Do we want to let it go negative?
		// I think this would happen when a > 0.5 / (filter width in symbol times)
		if (window < 0) {
			//printf ("'a' is too large for range of 't'.\n");
			//window = 0;
		}
	}

	result = sinc * window;

#if DEBUGRRC
	// t should vary from - to + half of filter size in symbols.
	// Result should be 1 at t=0 and 0 at all other integer values of t.

	printf("%.3f, %.3f, %.3f, %.3f\n", t, sinc, window, result);
#endif
	return (result);
}

// The Root Raised Cosine (RRC) low pass filter is suppposed to minimize Intersymbol Interference (ISI).

void gen_rrc_lowpass(float *pfilter, int filter_taps, float rolloff, float samples_per_symbol)
{
	int k;
	float t;

	for (k = 0; k < filter_taps; k++) {
		t = (k - ((filter_taps - 1.0) / 2.0)) / samples_per_symbol;
		pfilter[k] = rrc(t, rolloff);
	}

	// Scale it for unity gain.

	t = 0;
	for (k = 0; k < filter_taps; k++) {
		t += pfilter[k];
	}
	for (k = 0; k < filter_taps; k++) {
		pfilter[k] = pfilter[k] / t;
	}
}

/* end dsp.c */


int ax25_pack(packet_t this_p, unsigned char * result)
{
	int len = 0;
	//	assert(this_p->magic1 == MAGIC);
	//	assert(this_p->magic2 == MAGIC);

	//	assert(this_p->frame_len >= 0 && this_p->frame_len <= AX25_MAX_PACKET_LEN);

	//	memcpy(result, this_p->frame_data, this_p->frame_len);

	return (len);
}


int audio_flush(int a)
{
	return 0;
}

int fx25_send_frame(int chan, unsigned char *fbuf, int flen, int fx_mode)
{
	return 0;
}


struct demodulator_state_s D[4];
struct audio_s pa[4];

extern int was_init[4];

extern short rx_baudrate[5];
extern unsigned char  modem_mode[5];

void dw9600ProcessSample(int snd_ch, short Sample)
{
	demod_9600_process_sample(snd_ch, Sample, 1, &D[snd_ch]);
}

void init_RUH48(int snd_ch)
{
	modem_mode[snd_ch] = MODE_RUH;
	tx_bitrate[snd_ch] = rx_baudrate[snd_ch] = 4800;

	if (was_init[snd_ch] == 0)
	{
		hdlc_rec_init(&pa[snd_ch]);
		gen_tone_init(&pa[snd_ch], 100, 0);
		was_init[snd_ch] = 1;
	}

	demod_9600_init(2,
		48000,
		1, //upsample
		4800,
		&D[snd_ch]);

}

void init_RUH96(int snd_ch)
{
	modem_mode[snd_ch] = MODE_RUH;
	tx_bitrate[snd_ch] =  rx_baudrate[snd_ch] = 9600;

	if (was_init[snd_ch] == 0)
	{
		hdlc_rec_init(&pa[snd_ch]);
		gen_tone_init(&pa[snd_ch], 100, 0);
		was_init[snd_ch];
	}

	demod_9600_init(2,
		48000,
		1, //upsample
		9600,
		&D[snd_ch]);

}


void text_color_set(dw_color_t c)
{
	return;
}

typedef struct  TMChannel_t
{

	float prev_LPF1I_buf[4096];
	float LPF1Q_buf[4096];
	float prev_dLPFI_buf[4096];
	float prev_dLPFQ_buf[4096];
	float prev_AFCI_buf[4096];
	float prev_AFCQ_buf[4096];
	float AngleCorr;
	float MUX_osc;
	float AFC_IZ1;
	float AFC_IZ2;
	float AFC_QZ1;
	float AFC_QZ2;
	float AFC_bit_buf1I[1024];
	float AFC_bit_buf1Q[1024];
	float AFC_bit_buf2[1024];
	float AFC_IIZ1;
	float AFC_QQZ1;

} TMChannel;


typedef struct TFX25_t
{
	string  data;
	uint8_t  status;
	uint8_t  bit_cnt;
	uint8_t  uint8_t_rx;
	unsigned long long tag;
	uint8_t  size;
	uint8_t  rs_size;
	uint8_t size_cnt;
} TFX25;

typedef struct TDetector_t
{
	struct TFX25_t fx25[4];
	TStringList	mem_ARQ_F_buf[5];
	TStringList mem_ARQ_buf[5];
	float pll_loop[5];
	float last_sample[5];
	uint8_t ones[5];
	uint8_t zeros[5];
	float bit_buf[5][1024];
	float bit_buf1[5][1024];
	uint8_t sample_cnt[5];
	uint8_t last_bit[5];
	float PSK_IZ1[5];
	float PSK_QZ1[5];
	float PkAmpI[5];
	float PkAmpQ[5];
	float PkAmp[5];
	float PkAmpMax[5];
	int newpkpos[5];
	float AverageAmp[5];
	float AngleCorr[5];
	float MinAmp[5];
	float MaxAmp[5];
	float MUX3_osc[5];
	float MUX3_1_osc[5];
	float MUX3_2_osc[5];
	float Preemphasis6[5];
	float Preemphasis12[5];
	float PSK_AGC[5];
	float AGC[5];
	float AGC1[5];
	float AGC2[5];
	float AGC3[5];
	float AGC_max[5];
	float AGC_min[5];
	float AFC_IZ1[5];
	float AFC_IZ2[5];
	float AFC_QZ1[5];
	float AFC_QZ2[5];

	uint8_t last_rx_bit[5];
	uint8_t bit_stream[5];
	uint8_t uint8_t_rx[5];
	uint8_t bit_stuff_cnt[5];
	uint8_t bit_cnt[5];
	float bit_osc[5];
	uint8_t frame_status[5];
	string rx_data[5];
	string FEC_rx_data[5];
	//
	uint8_t FEC_pol[5];
	unsigned short FEC_err[5];
	unsigned long long FEC_header1[5][2];
	unsigned short FEC_blk_int[5];
	unsigned short FEC_len_int[5];
	unsigned short FEC_len[5];

	unsigned short FEC_len_cnt[5];

	uint8_t rx_intv_tbl[5][4];
	uint8_t rx_intv_sym[5];
	uint8_t rx_viterbi[5];
	uint8_t viterbi_cnt[5];
	//	  SurvivorStates [1..4,0..511] of TSurvivor;
		  //
	TMChannel MChannel[5][4];

	float AFC_dF_avg[5];
	float AFC_dF[5];
	float AFC_bit_osc[5];
	float AFC_bit_buf[5][1024];
	unsigned short AFC_cnt[5];

	string raw_bits1[5];
	string raw_bits[5];
	uint8_t last_nrzi_bit[5];

	float BPF_core[5][2048];
	float LPF_core[5][2048];

	float src_INTR_buf[5][8192];
	float src_INTRI_buf[5][8192];
	float src_INTRQ_buf[5][8192];
	float src_LPF1I_buf[5][8192];
	float src_LPF1Q_buf[5][8192];

	float src_BPF_buf[5][2048];
	float src_Loop_buf[5][8192];
	float prev_BPF_buf[5][4096];

	float prev_LPF1I_buf[5][4096];
	float prev_LPF1Q_buf[5][4096];
	float prev_INTR_buf[5][16384];
	float prev_INTRI_buf[5][16384];
	float prev_INTRQ_buf[5][16384];

	uint8_t emph_decoded;
	uint8_t rx_decoded;
	uint8_t errors;

} TDetector;


extern TStringList detect_list[5];
extern TStringList detect_list_c[5];

#define nr_emph 2
extern struct TDetector_t  DET[nr_emph + 1][16];

#define decodedNormal 4 //'-'
#define decodedFEC    3 //'F'
#define decodedMEM	  2 //'#'
#define decodedSingle 1 //'$'


int multi_modem_process_rec_frame(int chan, int subchan, int slice, unsigned char *fbuf, int flen, int alevel, int retries, int is_fx25)
{
	// Convert to QtSM internal format

	struct TDetector_t * pDET = &DET[0][subchan];
	string *  data = newString();
	char Mode[16] = "RUH";
	int i, found;

	stringAdd(data, fbuf, flen + 2);  // QTSM assumes a CRC

	if (detect_list[chan].Count > 0 &&
		my_indexof(&detect_list[chan], data) >= 0)
	{
		// Already have a copy of this frame

		freeString(data);
		Debugprintf("Discarding copy rcvr %d emph %d", subchan, 0);
		return 0;
	}

	string * xx = newString();
	memset(xx->Data, 0, 16);

	Add(&detect_list_c[chan], xx);
	Add(&detect_list[chan], data);

	pDET->rx_decoded = decodedNormal;
	pDET->emph_decoded = decodedNormal;
	pDET->errors = 0;

	//	if (retries)
	//		sprintf(Mode, "IP2P-%d", retries);

	stringAdd(xx, Mode, strlen(Mode));
	return 0;

}

extern unsigned short * DMABuffer;


extern int Number;
extern int SampleNo;

extern short txtail[5];
extern short txdelay[5];
extern short tx_baudrate[5];

extern int ARDOPTXLen[4];			// Length of frame
extern int ARDOPTXPtr[4];			// Tx Pointer


void RUHEncode(unsigned char * Data, int Len, int chan)
{
	//	Generate audio samples from frame data

	int bitcount;
	int txdcount;
	int i, j;
	unsigned short CRC;

	number_of_bits_sent[chan] = 0;

	SampleNo = 0;

	if (il2p_mode[chan] >= IL2P_MODE_TXRX)
	{
		// il2p generates TXDELAY as part of the frame, so just build frame

		string * result;
		packet_t pp = ax25_new();
		int polarity = 0;

		// Call il2p_send_frame to build the bit stream

		pp->frame_len = Len;
		memcpy(pp->frame_data, Data, Len + 2);	// Copy the crc in case we are going to send it

		result = il2p_send_frame(chan, pp, 1, 0);

		for (j = 0; j < result->Length; j++)
		{
			int x = result->Data[j];

			for (i = 0; i < 8; i++) 
			{
				int dbit = (x & 0x80) != 0;
				tone_gen_put_bit(chan, (dbit ^ polarity) & 1, 0);		// No Scramble
				x <<= 1;
				number_of_bits_sent[chan]++;
			}
		}

		freeString(result);
		ax25_delete(pp);

		// sample No should now contain number of (stereo) samples

		ARDOPTXLen[chan] = SampleNo;
		ARDOPTXPtr[chan] = 0;

		return;
	}

	// First do TX delay

	// Set up txd worth of flags

	txdcount = (txdelay[chan] * tx_baudrate[chan]) / 8000;		// 8 for bits, 1000 for mS

	if (txdcount > 1024)
		txdcount = 1024;

	while (txdcount--)
		send_control_nrzi(chan, 0x7e);	// Flag

	for (j = 0; j < Len; j++)
	{
		send_data_nrzi(chan, Data[j]);
	}

	CRC = get_fcs(Data, Len);

	send_data_nrzi(chan, (CRC & 0xff));
	send_data_nrzi(chan, (CRC >> 8));

	// do we need tail here??

	send_control_nrzi(chan, 0x7e);	// Flag

	ARDOPTXLen[chan] = SampleNo;
	ARDOPTXPtr[chan] = 0;

	// sampleNo should now contain number of (stereo) samples

}







