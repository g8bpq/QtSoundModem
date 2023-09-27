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

// IL2P code. Based on Direwolf code, under the following copyright

//
//    Copyright (C) 2021  John Langner, WB2OSZ
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


// IP2P receive code (il2p_rec_bit) is called from the bit receiving code in ax25_demod.c, so includes parallel decoders




#include "UZ7HOStuff.h"

void Debugprintf(const char * format, ...);
int SMUpdatePhaseConstellation(int chan, float * Phases, float * Mags, int intPSKPhase, int Count);

#define MAX_ADEVS 3			

#define MAX_RADIO_CHANS ((MAX_ADEVS) * 2)

#define MAX_CHANS MAX_RADIO_CHANS	// TODO: Replace all former  with latter to avoid confusion with following.

#define MAX_TOTAL_CHANS 16		// v1.7 allows additional virtual channels which are connected
 // to something other than radio modems.
 // Total maximum channels is based on the 4 bit KISS field.
 // Someone with very unusual requirements could increase this and
 // use only the AGW network protocol.


#define MAX_SUBCHANS 9

#define MAX_SLICERS 9

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

extern int nPhases[4][16][nr_emph + 1];
extern float Phases[4][16][nr_emph + 1][4096];
extern float Mags[4][16][nr_emph + 1][4096];

/* For option to try fixing frames with bad CRC. */

typedef enum retry_e {
	RETRY_NONE = 0,
	RETRY_INVERT_SINGLE = 1,
	RETRY_INVERT_DOUBLE = 2,
	RETRY_INVERT_TRIPLE = 3,
	RETRY_INVERT_TWO_SEP = 4,
	RETRY_MAX = 5
}  retry_t;

typedef struct alevel_s {
	int rec;
	int mark;
	int space;
	//float ms_ratio;	// TODO: take out after temporary investigation.
} alevel_t;


alevel_t demod_get_audio_level(int chan, int subchan);
void tone_gen_put_bit(int chan, int dat);

int ax25memdebug = 1;

// Code to try to determine centre freq

float MagOut[4096];
float MaxMagOut = 0;
int MaxMagIndex = 0;

// FFT Bin Size is 12000 / FFTSize

#ifndef FX25_H
#define FX25_H

#include <stdint.h>	// for uint64_t

extern unsigned int pskStates[4];

/* Reed-Solomon codec control block */
struct rs {
	unsigned int mm;              /* Bits per symbol */
	unsigned int nn;              /* Symbols per block (= (1<<mm)-1) */
	unsigned char *alpha_to;      /* log lookup table */
	unsigned char *index_of;      /* Antilog lookup table */
	unsigned char *genpoly;       /* Generator polynomial */
	unsigned int nroots;     /* Number of generator roots = number of parity symbols */
	unsigned char fcr;        /* First consecutive root, index form */
	unsigned char prim;       /* Primitive element, index form */
	unsigned char iprim;      /* prim-th root of 1, index form */
};

#define MM (rs->mm)
#define NN (rs->nn)
#define ALPHA_TO (rs->alpha_to) 
#define INDEX_OF (rs->index_of)
#define GENPOLY (rs->genpoly)
#define NROOTS (rs->nroots)
#define FCR (rs->fcr)
#define PRIM (rs->prim)
#define IPRIM (rs->iprim)
#define A0 (NN)

int __builtin_popcountll(unsigned long long int i)
{
	return 0;
}

int __builtin_popcount(unsigned int n)
{
	unsigned int count = 0;
	while (n)
	{
		count += n & 1;
		n >>= 1;
	}
	return count;
}

static inline int modnn(struct rs *rs, int x) {
	while (x >= rs->nn) {
		x -= rs->nn;
		x = (x >> rs->mm) + (x & rs->nn);
	}
	return x;
}

#define MODNN(x) modnn(rs,x)


#define ENCODE_RS encode_rs_char
#define DECODE_RS decode_rs_char
#define INIT_RS init_rs_char
#define FREE_RS free_rs_char

#define DTYPE unsigned char

void ENCODE_RS(struct rs *rs, DTYPE *data, DTYPE *bb);

int DECODE_RS(struct rs *rs, DTYPE *data, int *eras_pos, int no_eras);

struct rs *INIT_RS(unsigned int symsize, unsigned int gfpoly,
	unsigned int fcr, unsigned int prim, unsigned int nroots);

void FREE_RS(struct rs *rs);



// These 3 are the external interface.
// Maybe these should be in a different file, separated from the internal stuff.

void fx25_init(int debug_level);
int fx25_send_frame(int chan, unsigned char *fbuf, int flen, int fx_mode);
void fx25_rec_bit(int chan, int subchan, int slice, int dbit);
int fx25_rec_busy(int chan);


// Other functions in fx25_init.c.

struct rs *fx25_get_rs(int ctag_num);
uint64_t fx25_get_ctag_value(int ctag_num);
int fx25_get_k_data_radio(int ctag_num);
int fx25_get_k_data_rs(int ctag_num);
int fx25_get_nroots(int ctag_num);
int fx25_get_debug(void);
int fx25_tag_find_match(uint64_t t);
int fx25_pick_mode(int fx_mode, int dlen);

void fx_hex_dump(unsigned char *x, int len);

/*-------------------------------------------------------------------
 *
 * Name:	ax25_pad.h
 *
 * Purpose:	Header file for using ax25_pad.c
 *
 *------------------------------------------------------------------*/

#ifndef AX25_PAD_H
#define AX25_PAD_H 1


#define AX25_MAX_REPEATERS 8
#define AX25_MIN_ADDRS 2	/* Destination & Source. */
#define AX25_MAX_ADDRS 10	/* Destination, Source, 8 digipeaters. */	

#define AX25_DESTINATION  0	/* Address positions in frame. */
#define AX25_SOURCE       1	
#define AX25_REPEATER_1   2
#define AX25_REPEATER_2   3
#define AX25_REPEATER_3   4
#define AX25_REPEATER_4   5
#define AX25_REPEATER_5   6
#define AX25_REPEATER_6   7
#define AX25_REPEATER_7   8
#define AX25_REPEATER_8   9

#define AX25_MAX_ADDR_LEN 12	/* In theory, you would expect the maximum length */
 /* to be 6 letters, dash, 2 digits, and nul for a */
 /* total of 10.  However, object labels can be 10 */
 /* characters so throw in a couple extra bytes */
 /* to be safe. */

#define AX25_MIN_INFO_LEN 0	/* Previously 1 when considering only APRS. */

#define AX25_MAX_INFO_LEN 2048	/* Maximum size for APRS. */
				/* AX.25 starts out with 256 as the default max */
				/* length but the end stations can negotiate */
				/* something different. */
				/* version 0.8:  Change from 256 to 2028 to */
				/* handle the larger paclen for Linux AX25. */

				/* These don't include the 2 bytes for the */
				/* HDLC frame FCS. */

/*
 * Previously, for APRS only.
 * #define AX25_MIN_PACKET_LEN ( 2 * 7 + 2 + AX25_MIN_INFO_LEN)
 * #define AX25_MAX_PACKET_LEN ( AX25_MAX_ADDRS * 7 + 2 + AX25_MAX_INFO_LEN)
 */

 /* The more general case. */
 /* An AX.25 frame can have a control byte and no protocol. */

#define AX25_MIN_PACKET_LEN ( 2 * 7 + 1 )

#define AX25_MAX_PACKET_LEN ( AX25_MAX_ADDRS * 7 + 2 + 3 + AX25_MAX_INFO_LEN)


/*
 * packet_t is a pointer to a packet object.
 *
 * The actual implementation is not visible outside ax25_pad.c.
 */

#define AX25_UI_FRAME 3		/* Control field value. */

#define AX25_PID_NO_LAYER_3 0xf0		/* protocol ID used for APRS */
#define AX25_PID_SEGMENTATION_FRAGMENT 0x08
#define AX25_PID_ESCAPE_CHARACTER 0xff

struct packet_s {

	int magic1;		/* for error checking. */

	int seq;		/* unique sequence number for debugging. */

	double release_time;	/* Time stamp in format returned by dtime_now(). */
				/* When to release from the SATgate mode delay queue. */

#define MAGIC 0x41583235

	struct packet_s *nextp;	/* Pointer to next in queue. */

	int num_addr;		/* Number of addresses in frame. */
				/* Range of AX25_MIN_ADDRS .. AX25_MAX_ADDRS for AX.25. */
				/* It will be 0 if it doesn't look like AX.25. */
				/* -1 is used temporarily at allocation to mean */
				/* not determined yet. */



				/*
				 * The 7th octet of each address contains:
					 *
				 * Bits:   H  R  R  SSID  0
				 *
				 *   H 		for digipeaters set to 0 initially.
				 *		Changed to 1 when position has been used.
				 *
				 *		for source & destination it is called
				 *		command/response.  Normally both 1 for APRS.
				 *		They should be opposites for connected mode.
				 *
				 *   R	R	Reserved.  Normally set to 1 1.
				 *
				 *   SSID	Substation ID.  Range of 0 - 15.
				 *
				 *   0		Usually 0 but 1 for last address.
				 */


#define SSID_H_MASK	0x80
#define SSID_H_SHIFT	7

#define SSID_RR_MASK	0x60
#define SSID_RR_SHIFT	5

#define SSID_SSID_MASK	0x1e
#define SSID_SSID_SHIFT	1

#define SSID_LAST_MASK	0x01


	int frame_len;		/* Frame length without CRC. */

	int modulo;		/* I & S frames have sequence numbers of either 3 bits (modulo 8) */
				/* or 7 bits (modulo 128).  This is conveyed by either 1 or 2 */
				/* control bytes.  Unfortunately, we can't determine this by looking */
				/* at an isolated frame.  We need to know about the context.  If we */
				/* are part of the conversation, we would know.  But if we are */
				/* just listening to others, this would be more difficult to determine. */

				/* For U frames:   	set to 0 - not applicable */
				/* For I & S frames:	8 or 128 if known.  0 if unknown. */

	unsigned char frame_data[AX25_MAX_PACKET_LEN + 1];
	/* Raw frame contents, without the CRC. */


	int magic2;		/* Will get stomped on if above overflows. */
};



typedef struct packet_s *packet_t;

typedef enum cmdres_e { cr_00 = 2, cr_cmd = 1, cr_res = 0, cr_11 = 3 } cmdres_t;


extern packet_t ax25_new(void);


#ifdef AX25_PAD_C	/* Keep this hidden - implementation could change. */


/*
 * APRS always has one control octet of 0x03 but the more
 * general AX.25 case is one or two control bytes depending on
 * whether "modulo 128 operation" is in effect.
 */

 //#define DEBUGX 1

static inline int ax25_get_control_offset(packet_t this_p)
{
	return (this_p->num_addr * 7);
}

static inline int ax25_get_num_control(packet_t this_p)
{
	int c;

	c = this_p->frame_data[ax25_get_control_offset(this_p)];

	if ((c & 0x01) == 0) {			/* I   xxxx xxx0 */
#if DEBUGX
		Debugprintf("ax25_get_num_control, %02x is I frame, returns %d\n", c, (this_p->modulo == 128) ? 2 : 1);
#endif
		return ((this_p->modulo == 128) ? 2 : 1);
	}

	if ((c & 0x03) == 1) {			/* S   xxxx xx01 */
#if DEBUGX
		Debugprintf("ax25_get_num_control, %02x is S frame, returns %d\n", c, (this_p->modulo == 128) ? 2 : 1);
#endif
		return ((this_p->modulo == 128) ? 2 : 1);
	}

#if DEBUGX
	Debugprintf("ax25_get_num_control, %02x is U frame, always returns 1.\n", c);
#endif

	return (1);					/* U   xxxx xx11 */
}



/*
 * APRS always has one protocol octet of 0xF0 meaning no level 3
 * protocol but the more general case is 0, 1 or 2 protocol ID octets.
 */

static inline int ax25_get_pid_offset(packet_t this_p)
{
	return (ax25_get_control_offset(this_p) + ax25_get_num_control(this_p));
}

static int ax25_get_num_pid(packet_t this_p)
{
	int c;
	int pid;

	c = this_p->frame_data[ax25_get_control_offset(this_p)];

	if ((c & 0x01) == 0 ||				/* I   xxxx xxx0 */
		c == 0x03 || c == 0x13) {			/* UI  000x 0011 */

		pid = this_p->frame_data[ax25_get_pid_offset(this_p)];
#if DEBUGX
		Debugprintf("ax25_get_num_pid, %02x is I or UI frame, pid = %02x, returns %d\n", c, pid, (pid == AX25_PID_ESCAPE_CHARACTER) ? 2 : 1);
#endif
		if (pid == AX25_PID_ESCAPE_CHARACTER) {
			return (2);			/* pid 1111 1111 means another follows. */
		}
		return (1);
	}
#if DEBUGX
	Debugprintf("ax25_get_num_pid, %02x is neither I nor UI frame, returns 0\n", c);
#endif
	return (0);
}


/*
 * AX.25 has info field for 5 frame types depending on the control field.
 *
 *	xxxx xxx0	I
 *	000x 0011	UI		(which includes APRS)
 *	101x 1111	XID
 *	111x 0011	TEST
 *	100x 0111	FRMR
 *
 * APRS always has an Information field with at least one octet for the Data Type Indicator.
 */

static inline int ax25_get_info_offset(packet_t this_p)
{
	int offset = ax25_get_control_offset(this_p) + ax25_get_num_control(this_p) + ax25_get_num_pid(this_p);
#if DEBUGX
	Debugprintf("ax25_get_info_offset, returns %d\n", offset);
#endif
	return (offset);
}

static inline int ax25_get_num_info(packet_t this_p)
{
	int len;

	/* assuming AX.25 frame. */

	len = this_p->frame_len - this_p->num_addr * 7 - ax25_get_num_control(this_p) - ax25_get_num_pid(this_p);
	if (len < 0) {
		len = 0;		/* print error? */
	}

	return (len);
}

#endif


typedef enum ax25_modulo_e { modulo_unknown = 0, modulo_8 = 8, modulo_128 = 128 } ax25_modulo_t;

typedef enum ax25_frame_type_e {

	frame_type_I = 0,	// Information

	frame_type_S_RR,	// Receive Ready - System Ready To Receive
	frame_type_S_RNR,	// Receive Not Ready - TNC Buffer Full
	frame_type_S_REJ,	// Reject Frame - Out of Sequence or Duplicate
	frame_type_S_SREJ,	// Selective Reject - Request single frame repeat

	frame_type_U_SABME,	// Set Async Balanced Mode, Extended
	frame_type_U_SABM,	// Set Async Balanced Mode
	frame_type_U_DISC,	// Disconnect
	frame_type_U_DM,	// Disconnect Mode
	frame_type_U_UA,	// Unnumbered Acknowledge
	frame_type_U_FRMR,	// Frame Reject
	frame_type_U_UI,	// Unnumbered Information
	frame_type_U_XID,	// Exchange Identification
	frame_type_U_TEST,	// Test
	frame_type_U,		// other Unnumbered, not used by AX.25.

	frame_not_AX25		// Could not get control byte from frame.
				// This must be last because value plus 1 is
				// for the size of an array.

} ax25_frame_type_t;


/*
 * Originally this was a single number.
 * Let's try something new in version 1.2.
 * Also collect AGC values from the mark and space filters.
 */

#ifndef AXTEST
// TODO: remove this?
#define AX25MEMDEBUG 1
#endif



extern packet_t ax25_from_text(char *monitor, int strict);

extern packet_t ax25_from_frame(unsigned char *data, int len, alevel_t alevel);

extern packet_t ax25_dup(packet_t copy_from);

extern void ax25_delete(packet_t pp);



extern int ax25_parse_addr(int position, char *in_addr, int strict, char *out_addr, int *out_ssid, int *out_heard);
extern int ax25_check_addresses(packet_t pp);

extern packet_t ax25_unwrap_third_party(packet_t from_pp);

extern void ax25_set_addr(packet_t pp, int, char *);
extern void ax25_insert_addr(packet_t this_p, int n, char *ad);
extern void ax25_remove_addr(packet_t this_p, int n);

extern int ax25_get_num_addr(packet_t pp);
extern int ax25_get_num_repeaters(packet_t this_p);

extern void ax25_get_addr_with_ssid(packet_t pp, int n, char *station);
extern void ax25_get_addr_no_ssid(packet_t pp, int n, char *station);

extern int ax25_get_ssid(packet_t pp, int n);
extern void ax25_set_ssid(packet_t this_p, int n, int ssid);

extern int ax25_get_h(packet_t pp, int n);

extern void ax25_set_h(packet_t pp, int n);

extern int ax25_get_heard(packet_t this_p);

extern int ax25_get_first_not_repeated(packet_t pp);

extern int ax25_get_rr(packet_t this_p, int n);

extern int ax25_get_info(packet_t pp, unsigned char **paddr);
extern void ax25_set_info(packet_t pp, unsigned char *info_ptr, int info_len);
extern int ax25_cut_at_crlf(packet_t this_p);

extern void ax25_set_nextp(packet_t this_p, packet_t next_p);

extern int ax25_get_dti(packet_t this_p);

extern packet_t ax25_get_nextp(packet_t this_p);

extern void ax25_set_release_time(packet_t this_p, double release_time);
extern double ax25_get_release_time(packet_t this_p);

extern void ax25_set_modulo(packet_t this_p, int modulo);
extern int ax25_get_modulo(packet_t this_p);

extern void ax25_format_addrs(packet_t pp, char *);
extern void ax25_format_via_path(packet_t this_p, char *result, size_t result_size);

extern int ax25_pack(packet_t pp, unsigned char result[AX25_MAX_PACKET_LEN]);

extern ax25_frame_type_t ax25_frame_type(packet_t this_p, cmdres_t *cr, char *desc, int *pf, int *nr, int *ns);

extern void ax25_hex_dump(packet_t this_p);

extern int ax25_is_aprs(packet_t pp);
extern int ax25_is_null_frame(packet_t this_p);

extern int ax25_get_control(packet_t this_p);
extern int ax25_get_c2(packet_t this_p);

extern int ax25_get_pid(packet_t this_p);

extern int ax25_get_frame_len(packet_t this_p);
extern unsigned char *ax25_get_frame_data_ptr(packet_t this_p);

extern unsigned short ax25_dedupe_crc(packet_t pp);

extern unsigned short ax25_m_m_crc(packet_t pp);

extern void ax25_safe_print(char *, int, int ascii_only);

#define AX25_ALEVEL_TO_TEXT_SIZE 40	// overkill but safe.
extern int ax25_alevel_to_text(alevel_t alevel, char text[AX25_ALEVEL_TO_TEXT_SIZE]);


#endif /* AX25_PAD_H */

/* end ax25_pad.h */




#define CTAG_MIN 0x01
#define CTAG_MAX 0x0B

// Maximum sizes of "data" and "check" parts.

#define FX25_MAX_DATA 239	// i.e. RS(255,239)
#define FX25_MAX_CHECK 64	// e.g. RS(255, 191)
#define FX25_BLOCK_SIZE 255	// Block size always 255 for 8 bit symbols.

#endif // FX25_H

#ifndef IL2P_H
#define IL2P_H 1


#define IL2P_PREAMBLE 0x55

#define IL2P_SYNC_WORD 0xF15E48

#define IL2P_SYNC_WORD_SIZE 3
#define IL2P_HEADER_SIZE 13	// Does not include 2 parity.
#define IL2P_HEADER_PARITY 2

#define IL2P_MAX_PAYLOAD_SIZE 1023
#define IL2P_MAX_PAYLOAD_BLOCKS 5
#define IL2P_MAX_PARITY_SYMBOLS 16		// For payload only.
#define IL2P_MAX_ENCODED_PAYLOAD_SIZE (IL2P_MAX_PAYLOAD_SIZE + IL2P_MAX_PAYLOAD_BLOCKS * IL2P_MAX_PARITY_SYMBOLS)

#define IL2P_MAX_PACKET_SIZE (IL2P_SYNC_WORD_SIZE + IL2P_HEADER_SIZE + IL2P_HEADER_PARITY + IL2P_MAX_ENCODED_PAYLOAD_SIZE)


float GuessCentreFreq(int i)
{
	float Freq = 0;
	float Start;
	float End;
	int n;
	float Max = 0;
	int Index = 0;
	float BinSize = 12000.0 / FFTSize;

	Start = (rx_freq[i] - RCVR[i] * rcvr_offset[i]) / BinSize;
	End = (rx_freq[i] + RCVR[i] * rcvr_offset[i]) / BinSize;

	Start = (active_rx_freq[i] - RCVR[i] * rcvr_offset[i]) / BinSize;
	End = (active_rx_freq[i] + RCVR[i] * rcvr_offset[i]) / BinSize;


	for (n = Start; n <= End; n++)
	{
		if (MagOut[n] > Max)
		{
			Max = MagOut[n];
			Index = n;
		}
	}

	Freq = Index * BinSize;

	return Freq;
}

/*------------------------------------------------------------------------------
 *
 * Name:	ax25_new
 *
 * Purpose:	Allocate memory for a new packet object.
 *
 * Returns:	Identifier for a new packet object.
 *		In the current implementation this happens to be a pointer.
 *
 *------------------------------------------------------------------------------*/

int last_seq_num = 0;
int new_count = 0;
int delete_count = 0;

packet_t ax25_new(void)
{
	struct packet_s *this_p;


#if DEBUG 
	text_color_set(DW_COLOR_DEBUG);
	Debugprintf("ax25_new(): before alloc, new=%d, delete=%d\n", new_count, delete_count);
#endif

	last_seq_num++;
	new_count++;

	/*
	 * check for memory leak.
	 */

	 // version 1.4 push up the threshold.   We could have considerably more with connected mode.

		 //if (new_count > delete_count + 100) {
	if (new_count > delete_count + 256) {

		Debugprintf("Memory leak for packet objects.  new=%d, delete=%d\n", new_count, delete_count);
#if AX25MEMDEBUG
#endif
	}

	this_p = calloc(sizeof(struct packet_s), (size_t)1);

	if (this_p == NULL) {
		Debugprintf("ERROR - can't allocate memory in ax25_new.\n");
	}

//	assert(this_p != NULL);

	this_p->magic1 = MAGIC;
	this_p->seq = last_seq_num;
	this_p->magic2 = MAGIC;
	this_p->num_addr = (-1);

	return (this_p);
}

/*------------------------------------------------------------------------------
 *
 * Name:	ax25_delete
 *
 * Purpose:	Destroy a packet object, freeing up memory it was using.
 *
 *------------------------------------------------------------------------------*/

void ax25_delete(packet_t this_p)
{
	if (this_p == NULL) {
		Debugprintf("ERROR - NULL pointer passed to ax25_delete.\n");
		return;
	}

	delete_count++;

//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	this_p->magic1 = 0;
	this_p->magic1 = 0;

	free(this_p);
}





/*------------------------------------------------------------------------------
 *
 * Name:	ax25_s_frame
 *
 * Purpose:	Construct an S frame.
 *
 * Input:	addrs		- Array of addresses.
 *
 *		num_addr	- Number of addresses, range 2 .. 10.
 *
 *		cr		- cr_cmd command frame, cr_res for a response frame.
 *
 *		ftype		- One of:
 *				        frame_type_S_RR,        // Receive Ready - System Ready To Receive
 *				        frame_type_S_RNR,       // Receive Not Ready - TNC Buffer Full
 *				        frame_type_S_REJ,       // Reject Frame - Out of Sequence or Duplicate
 *				        frame_type_S_SREJ,      // Selective Reject - Request single frame repeat
 *
 *		modulo		- 8 or 128.  Determines if we have 1 or 2 control bytes.
 *
 *		nr		- N(R) field --- describe.
 *
 *		pf		- Poll/Final flag.
 *
 *		pinfo		- Pointer to data for Info field.  Allowed only for SREJ.
 *
 *		info_len	- Length for Info field.
 *
 *
 * Returns:	Pointer to new packet object.
 *
 *------------------------------------------------------------------------------*/


packet_t ax25_s_frame(char addrs[AX25_MAX_ADDRS][AX25_MAX_ADDR_LEN], int num_addr, cmdres_t cr, ax25_frame_type_t ftype, int modulo, int nr, int pf, unsigned char *pinfo, int info_len)
{
	packet_t this_p;
	unsigned char *p;
	int ctrl = 0;

	this_p = ax25_new();

	if (this_p == NULL) return (NULL);

	if (!set_addrs(this_p, addrs, num_addr, cr)) {
		Debugprintf("Internal error in %s: Could not set addresses for S frame.\n", __func__);
		ax25_delete(this_p);
		return (NULL);
	}

	if (modulo != 8 && modulo != 128) {
		Debugprintf("Internal error in %s: Invalid modulo %d for S frame.\n", __func__, modulo);
		modulo = 8;
	}
	this_p->modulo = modulo;

	if (nr < 0 || nr >= modulo) {
		Debugprintf("Internal error in %s: Invalid N(R) %d for S frame.\n", __func__, nr);
		nr &= (modulo - 1);
	}

	// Erratum: The AX.25 spec is not clear about whether SREJ should be command, response, or both.
	// The underlying X.25 spec clearly says it is response only.  Let's go with that.

	if (ftype == frame_type_S_SREJ && cr != cr_res) {
		Debugprintf("Internal error in %s: SREJ must be response.\n", __func__);
	}

	switch (ftype) {

	case frame_type_S_RR:		ctrl = 0x01;	break;
	case frame_type_S_RNR:	ctrl = 0x05;	break;
	case frame_type_S_REJ:	ctrl = 0x09;	break;
	case frame_type_S_SREJ:	ctrl = 0x0d;	break;

	default:
		Debugprintf("Internal error in %s: Invalid ftype %d for S frame.\n", __func__, ftype);
		ax25_delete(this_p);
		return (NULL);
		break;
	}

	p = this_p->frame_data + this_p->frame_len;

	if (modulo == 8) {
		if (pf) ctrl |= 0x10;
		ctrl |= nr << 5;
		*p++ = ctrl;
		this_p->frame_len++;
	}
	else {
		*p++ = ctrl;
		this_p->frame_len++;

		ctrl = pf & 1;
		ctrl |= nr << 1;
		*p++ = ctrl;
		this_p->frame_len++;
	}

	if (ftype == frame_type_S_SREJ) {
		if (pinfo != NULL && info_len > 0) {
			if (info_len > AX25_MAX_INFO_LEN) {
				Debugprintf("Internal error in %s: SREJ frame, Invalid information field length %d.\n", __func__, info_len);
				info_len = AX25_MAX_INFO_LEN;
			}
			memcpy(p, pinfo, info_len);
			p += info_len;
			this_p->frame_len += info_len;
		}
	}
	else {
		if (pinfo != NULL || info_len != 0) {
			Debugprintf("Internal error in %s: Info part not allowed for RR, RNR, REJ frame.\n", __func__);
		}
	}
	*p = '\0';


	return (this_p);

} /* end ax25_s_frame */





/*------------------------------------------------------------------------------
 *
 * Name:	ax25_i_frame
 *
 * Purpose:	Construct an I frame.
 *
 * Input:	addrs		- Array of addresses.
 *
 *		num_addr	- Number of addresses, range 2 .. 10.
 *
 *		cr		- cr_cmd command frame, cr_res for a response frame.
 *
 *		modulo		- 8 or 128.
 *
 *		nr		- N(R) field --- describe.
 *
 *		ns		- N(S) field --- describe.
 *
 *		pf		- Poll/Final flag.
 *
 *		pid		- Protocol ID.
 *				  Normally 0xf0 meaning no level 3.
 *				  Could be other values for NET/ROM, etc.
 *
 *		pinfo		- Pointer to data for Info field.
 *
 *		info_len	- Length for Info field.
 *
 *
 * Returns:	Pointer to new packet object.
 *
 *------------------------------------------------------------------------------*/

packet_t ax25_i_frame(char addrs[AX25_MAX_ADDRS][AX25_MAX_ADDR_LEN], int num_addr, cmdres_t cr, int modulo, int nr, int ns, int pf, int pid, unsigned char *pinfo, int info_len)
{
	packet_t this_p;
	unsigned char *p;
	int ctrl = 0;

	this_p = ax25_new();

	if (this_p == NULL) return (NULL);

	if (!set_addrs(this_p, addrs, num_addr, cr)) {
		Debugprintf("Internal error in %s: Could not set addresses for I frame.\n", __func__);
		ax25_delete(this_p);
		return (NULL);
	}

	if (modulo != 8 && modulo != 128) {
		Debugprintf("Internal error in %s: Invalid modulo %d for I frame.\n", __func__, modulo);
		modulo = 8;
	}
	this_p->modulo = modulo;

	if (nr < 0 || nr >= modulo) {
		Debugprintf("Internal error in %s: Invalid N(R) %d for I frame.\n", __func__, nr);
		nr &= (modulo - 1);
	}

	if (ns < 0 || ns >= modulo) {
		Debugprintf("Internal error in %s: Invalid N(S) %d for I frame.\n", __func__, ns);
		ns &= (modulo - 1);
	}

	p = this_p->frame_data + this_p->frame_len;

	if (modulo == 8) {
		ctrl = (nr << 5) | (ns << 1);
		if (pf) ctrl |= 0x10;
		*p++ = ctrl;
		this_p->frame_len++;
	}
	else {
		ctrl = ns << 1;
		*p++ = ctrl;
		this_p->frame_len++;

		ctrl = nr << 1;
		if (pf) ctrl |= 0x01;
		*p++ = ctrl;
		this_p->frame_len++;
	}

	// Definitely don't want pid value of 0 (not in valid list)
	// or 0xff (which means more bytes follow).

	if (pid < 0 || pid == 0 || pid == 0xff) {
		Debugprintf("Warning: Client application provided invalid PID value, 0x%02x, for I frame.\n", pid);
		pid = AX25_PID_NO_LAYER_3;
	}
	*p++ = pid;
	this_p->frame_len++;

	if (pinfo != NULL && info_len > 0) {
		if (info_len > AX25_MAX_INFO_LEN) {
			Debugprintf("Internal error in %s: I frame, Invalid information field length %d.\n", __func__, info_len);
			info_len = AX25_MAX_INFO_LEN;
		}
		memcpy(p, pinfo, info_len);
		p += info_len;
		this_p->frame_len += info_len;
	}

	*p = '\0';


	return (this_p);

} /* end ax25_i_frame */





extern TStringList detect_list[5];
extern TStringList detect_list_c[5];

void multi_modem_process_rec_packet(int snd_ch, int subchan, int slice, packet_t pp, alevel_t alevel, retry_t retries, int is_fx25, int emph, int centreFreq)
{
	// Convert to QtSM internal format

	struct TDetector_t * pDET = &DET[emph][subchan];
	string *  data = newString();
	char Mode[32] = "IL2P";
	int Quality = 0;

	sprintf(Mode, "IL2P %d", centreFreq);

	stringAdd(data, pp->frame_data, pp->frame_len + 2);  // QTSM assumes a CRC

	ax25_delete(pp);

	if (retries)
	{
		pDET->rx_decoded = decodedFEC;
		pDET->emph_decoded = decodedFEC;
		pDET->errors = retries;
	}
	else
	{
		pDET->rx_decoded = decodedNormal;
		pDET->emph_decoded = decodedNormal;
		pDET->errors = 0;
	}

	if (detect_list[snd_ch].Count > 0 &&
		my_indexof(&detect_list[snd_ch], data) >= 0)
	{
		// Already have a copy of this frame

		freeString(data);
		Debugprintf("Discarding copy rcvr %d emph %d", subchan, 0);
		return;
	}

	string * xx = newString();
	memset(xx->Data, 0, 16);

	if (pskStates[snd_ch])
	{
		Quality = SMUpdatePhaseConstellation(snd_ch, &Phases[snd_ch][subchan][slice][0], &Mags[snd_ch][subchan][slice][0], pskStates[snd_ch], nPhases[snd_ch][subchan][slice]);
		sprintf(Mode, "%s][Q%d", Mode, Quality);
	}

	Add(&detect_list_c[snd_ch], xx);
	Add(&detect_list[snd_ch], data);

//	if (retries)
//		sprintf(Mode, "IP2P-%d", retries);

	stringAdd(xx, Mode, strlen(Mode));


	return;

}




alevel_t demod_get_audio_level(int chan, int subchan)
{
	alevel_t alevel;
	alevel.rec = 0;
	alevel.mark = 0;
	alevel.space = 0;
	return (alevel);
}

void ax25_hex_dump(packet_t this_p)
{}


/*------------------------------------------------------------------------------
 *
 * Name:	ax25_from_frame
 *
 * Purpose:	Split apart an HDLC frame to components.
 *
 * Inputs:	fbuf	- Pointer to beginning of frame.
 *
 *		flen	- Length excluding the two FCS bytes.
 *
 *		alevel	- Audio level of received signal.
 *			  Maximum range 0 - 100.
 *			  -1 might be used when not applicable.
 *
 * Returns:	Pointer to new packet object or NULL if error.
 *
 * Outputs:	Use the "get" functions to retrieve information in different ways.
 *
 *------------------------------------------------------------------------------*/


packet_t ax25_from_frame(unsigned char *fbuf, int flen, alevel_t alevel)
{
	packet_t this_p;


	/*
	 * First make sure we have an acceptable length:
	 *
	 *	We are not concerned with the FCS (CRC) because someone else checked it.
	 *
	 * Is is possible to have zero length for info?
	 *
	 * In the original version, assuming APRS, the answer was no.
	 * We always had at least 3 octets after the address part:
	 * control, protocol, and first byte of info part for data type.
	 *
	 * In later versions, this restriction was relaxed so other
	 * variations of AX.25 could be used.  Now the minimum length
	 * is 7+7 for addresses plus 1 for control.
	 *
	 */


	if (flen < AX25_MIN_PACKET_LEN || flen > AX25_MAX_PACKET_LEN)
	{
		Debugprintf("Frame length %d not in allowable range of %d to %d.", flen, AX25_MIN_PACKET_LEN, AX25_MAX_PACKET_LEN);
		return (NULL);
	}

	this_p = ax25_new();

	/* Copy the whole thing intact. */

	memcpy(this_p->frame_data, fbuf, flen);
	this_p->frame_data[flen] = 0;
	this_p->frame_len = flen;

	/* Find number of addresses. */

	this_p->num_addr = (-1);
	(void)ax25_get_num_addr(this_p);

	return (this_p);
}



/*------------------------------------------------------------------------------
 *
 * Name:	ax25_get_num_addr
 *
 * Purpose:	Return number of addresses in current packet.
 *
 * Assumption:	ax25_from_text or ax25_from_frame was called first.
 *
 * Returns:	Number of addresses in the current packet.
 *		Should be in the range of 2 .. AX25_MAX_ADDRS.
 *
 * Version 0.9:	Could be zero for a non AX.25 frame in KISS mode.
 *
 *------------------------------------------------------------------------------*/

int ax25_get_num_addr(packet_t this_p)
{
	//unsigned char *pf;
	int a;
	int addr_bytes;


//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	/* Use cached value if already set. */

	if (this_p->num_addr >= 0) {
		return (this_p->num_addr);
	}

	/* Otherwise, determine the number ofaddresses. */

	this_p->num_addr = 0;		/* Number of addresses extracted. */

	addr_bytes = 0;
	for (a = 0; a < this_p->frame_len && addr_bytes == 0; a++) {
		if (this_p->frame_data[a] & SSID_LAST_MASK) {
			addr_bytes = a + 1;
		}
	}

	if (addr_bytes % 7 == 0) {
		int addrs = addr_bytes / 7;
		if (addrs >= AX25_MIN_ADDRS && addrs <= AX25_MAX_ADDRS) {
			this_p->num_addr = addrs;
		}
	}

	return (this_p->num_addr);
}



void ax25_get_addr_with_ssid(packet_t pp, int n, char *station)
{}

/*------------------------------------------------------------------------------
 *
 * Name:	ax25_get_addr_no_ssid
 *
 * Purpose:	Return specified address WITHOUT any SSID.
 *
 * Inputs:	n	- Index of address.   Use the symbols
 *			  AX25_DESTINATION, AX25_SOURCE, AX25_REPEATER1, etc.
 *
 * Outputs:	station - String representation of the station, WITHOUT the SSID.
 *			e.g.  "WB2OSZ"
 *			  Usually variables will be AX25_MAX_ADDR_LEN bytes
 *			  but 7 would be adequate.
 *
 * Bugs:	No bounds checking is performed.  Be careful.
 *
 * Assumption:	ax25_from_text or ax25_from_frame was called first.
 *
 * Returns:	Character string in usual human readable format,
 *
 *
 *------------------------------------------------------------------------------*/

void ax25_get_addr_no_ssid(packet_t this_p, int n, char *station)
{
	int i;

	//assert(this_p->magic1 == MAGIC);
	//assert(this_p->magic2 == MAGIC);


	if (n < 0) {
		Debugprintf("Internal error detected in ax25_get_addr_no_ssid, %s, line %d.\n", __FILE__, __LINE__);
		Debugprintf("Address index, %d, is less than zero.\n", n);
		strcpy(station, "??????");
		return;
	}

	if (n >= this_p->num_addr) {
		Debugprintf("Internal error detected in ax25_get_no_with_ssid, %s, line %d.\n", __FILE__, __LINE__);
		Debugprintf("Address index, %d, is too large for number of addresses, %d.\n", n, this_p->num_addr);
		strcpy(station, "??????");
		return;
	}

	// At one time this would stop at the first space, on the assumption we would have only trailing spaces.
	// Then there was a forum discussion where someone encountered the address " WIDE2" with a leading space.
	// In that case, we would have returned a zero length string here.
	// Now we return exactly what is in the address field and trim trailing spaces.
	// This will provide better information for troubleshooting.

	for (i = 0; i < 6; i++) {
		station[i] = (this_p->frame_data[n * 7 + i] >> 1) & 0x7f;
	}
	station[6] = '\0';

	for (i = 5; i >= 0; i--) {
		if (station[i] == ' ')
			station[i] = '\0';
		else
			break;
	}

	if (strlen(station) == 0) {
		Debugprintf("Station address, in position %d, is empty!  This is not a valid AX.25 frame.\n", n);
	}

} /* end ax25_get_addr_no_ssid */


/*------------------------------------------------------------------------------
 *
 * Name:	ax25_get_ssid
 *
 * Purpose:	Return SSID of specified address in current packet.
 *
 * Inputs:	n	- Index of address.   Use the symbols
 *			  AX25_DESTINATION, AX25_SOURCE, AX25_REPEATER1, etc.
 *
 * Assumption:	ax25_from_text or ax25_from_frame was called first.
 *
 * Returns:	Substation id, as integer 0 .. 15.
 *
 *------------------------------------------------------------------------------*/

int ax25_get_ssid(packet_t this_p, int n)
{

//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	if (n >= 0 && n < this_p->num_addr) {
		return ((this_p->frame_data[n * 7 + 6] & SSID_SSID_MASK) >> SSID_SSID_SHIFT);
	}
	else {
		Debugprintf("Internal error: ax25_get_ssid(%d), num_addr=%d\n", n, this_p->num_addr);
		return (0);
	}
}



static inline int ax25_get_pid_offset(packet_t this_p)
{
	return (ax25_get_control_offset(this_p) + ax25_get_num_control(this_p));
}

static int ax25_get_num_pid(packet_t this_p)
{
	int c;
	int pid;

	c = this_p->frame_data[ax25_get_control_offset(this_p)];

	if ((c & 0x01) == 0 ||				/* I   xxxx xxx0 */
		c == 0x03 || c == 0x13) {			/* UI  000x 0011 */

		pid = this_p->frame_data[ax25_get_pid_offset(this_p)];
		if (pid == AX25_PID_ESCAPE_CHARACTER) {
			return (2);			/* pid 1111 1111 means another follows. */
		}
		return (1);
	}
	return (0);
}


inline int ax25_get_control_offset(packet_t this_p)
{
	return (this_p->num_addr * 7);
}

inline int ax25_get_num_control(packet_t this_p)
{
	int c;

	c = this_p->frame_data[ax25_get_control_offset(this_p)];

	if ((c & 0x01) == 0) {			/* I   xxxx xxx0 */
		return ((this_p->modulo == 128) ? 2 : 1);
	}

	if ((c & 0x03) == 1) {			/* S   xxxx xx01 */
		return ((this_p->modulo == 128) ? 2 : 1);
	}

	return (1);					/* U   xxxx xx11 */
}




int ax25_get_info_offset(packet_t this_p)
{
	int offset = ax25_get_control_offset(this_p) + ax25_get_num_control(this_p) + ax25_get_num_pid(this_p);
	return (offset);
}

int ax25_get_num_info(packet_t this_p)
{
	int len;

	/* assuming AX.25 frame. */

	len = this_p->frame_len - this_p->num_addr * 7 - ax25_get_num_control(this_p) - ax25_get_num_pid(this_p);
	if (len < 0) {
		len = 0;		/* print error? */
	}

	return (len);
}





	/*------------------------------------------------------------------------------
	 *
	 * Name:	ax25_get_info
	 *
	 * Purpose:	Obtain Information part of current packet.
	 *
	 * Inputs:	this_p	- Packet object pointer.
	 *
	 * Outputs:	paddr	- Starting address of information part is returned here.
	 *
	 * Assumption:	ax25_from_text or ax25_from_frame was called first.
	 *
	 * Returns:	Number of octets in the Information part.
	 *		Should be in the range of AX25_MIN_INFO_LEN .. AX25_MAX_INFO_LEN.
	 *
	 *------------------------------------------------------------------------------*/

int ax25_get_info(packet_t this_p, unsigned char **paddr)
{
	unsigned char *info_ptr;
	int info_len;


	//assert(this_p->magic1 == MAGIC);
	//assert(this_p->magic2 == MAGIC);

	if (this_p->num_addr >= 2) {

		/* AX.25 */

		info_ptr = this_p->frame_data + ax25_get_info_offset(this_p);
		info_len = ax25_get_num_info(this_p);
	}
	else {

		/* Not AX.25.  Treat Whole packet as info. */

		info_ptr = this_p->frame_data;
		info_len = this_p->frame_len;
	}

	/* Add nul character in case caller treats as printable string. */

//		assert(info_len >= 0);

	info_ptr[info_len] = '\0';

	*paddr = info_ptr;
	return (info_len);

} /* end ax25_get_info */




void ax25_set_info(packet_t this_p, unsigned char *new_info_ptr, int new_info_len)
{
	unsigned char *old_info_ptr;
	int old_info_len = ax25_get_info(this_p, &old_info_ptr);
	this_p->frame_len -= old_info_len;

	if (new_info_len < 0) new_info_len = 0;
	if (new_info_len > AX25_MAX_INFO_LEN) new_info_len = AX25_MAX_INFO_LEN;
	memcpy(old_info_ptr, new_info_ptr, new_info_len);
	this_p->frame_len += new_info_len;
}

int ax25_get_pid(packet_t this_p)
{
//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	// TODO: handle 2 control byte case.
	// TODO: sanity check: is it I or UI frame?

	if (this_p->frame_len == 0) return(-1);

	if (this_p->num_addr >= 2) {
		return (this_p->frame_data[ax25_get_pid_offset(this_p)]);
	}
	return (-1);
}


int ax25_get_frame_len(packet_t this_p)
{
//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

//	assert(this_p->frame_len >= 0 && this_p->frame_len <= AX25_MAX_PACKET_LEN);

	return (this_p->frame_len);

} /* end ax25_get_frame_len */


unsigned char *ax25_get_frame_data_ptr(packet_t this_p)
{
//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	return (this_p->frame_data);

} /* end ax25_get_frame_data_ptr */


int ax25_get_modulo(packet_t this_p)
{
	return 7;
}


/*------------------------------------------------------------------
 *
 * Function:	ax25_get_control
		ax25_get_c2
 *
 * Purpose:	Get Control field from packet.
 *
 * Inputs:	this_p	- pointer to packet object.
 *
 * Returns:	APRS uses AX25_UI_FRAME.
 *		This could also be used in other situations.
 *
 *------------------------------------------------------------------*/


int ax25_get_control(packet_t this_p)
{
//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);

	if (this_p->frame_len == 0) return(-1);

	if (this_p->num_addr >= 2) {
		return (this_p->frame_data[ax25_get_control_offset(this_p)]);
	}
	return (-1);
}


/*------------------------------------------------------------------
*
* Function:	ax25_frame_type
*
* Purpose : Extract the type of frame.
*		This is derived from the control byte(s) but
*		is an enumerated type for easier handling.
*
* Inputs : this_p - pointer to packet object.
*
* Outputs : desc - Text description such as "I frame" or
*"U frame SABME".
*			  Supply 56 bytes to be safe.
*
*		cr - Command or response ?
*
*		pf - P / F - Poll / Final or -1 if not applicable
*
*		nr - N(R) - receive sequence or -1 if not applicable.
*
*		ns - N(S) - send sequence or -1 if not applicable.
*
* Returns:	Frame type from  enum ax25_frame_type_e.
*
*------------------------------------------------------------------*/

// TODO: need someway to ensure caller allocated enough space.
// Should pass in as parameter.

#define DESC_SIZ 56


ax25_frame_type_t ax25_frame_type(packet_t this_p, cmdres_t *cr, char *desc, int *pf, int *nr, int *ns)
{
	int c;		// U frames are always one control byte.
	int c2 = 0;	// I & S frames can have second Control byte.

//	assert(this_p->magic1 == MAGIC);
//	assert(this_p->magic2 == MAGIC);


	strcpy(desc, "????");
	*cr = cr_11;
	*pf = -1;
	*nr = -1;
	*ns = -1;

	c = ax25_get_control(this_p);
	if (c < 0) {
		strcpy(desc, "Not AX.25");
		return (frame_not_AX25);
	}

	/*
	 * TERRIBLE HACK :-(  for display purposes.
	 *
	 * I and S frames can have 1 or 2 control bytes but there is
	 * no good way to determine this without dipping into the data
	 * link state machine.  Can we guess?
	 *
	 * S frames have no protocol id or information so if there is one
	 * more byte beyond the control field, we could assume there are
	 * two control bytes.
	 *
	 * For I frames, the protocol id will usually be 0xf0.  If we find
	 * that as the first byte of the information field, it is probably
	 * the pid and not part of the information.  Ditto for segments 0x08.
	 * Not fool proof but good enough for troubleshooting text out.
	 *
	 * If we have a link to the peer station, this will be set properly
	 * before it needs to be used for other reasons.
	 *
	 * Setting one of the RR bits (find reference!) is sounding better and better.
	 * It's in common usage so I should lobby to get that in the official protocol spec.
	 */

	// Dont support mod 128
/*
	if (this_p->modulo == 0 && (c & 3) == 1 && ax25_get_c2(this_p) != -1) {
		this_p->modulo = modulo_128;
	}
	else if (this_p->modulo == 0 && (c & 1) == 0 && this_p->frame_data[ax25_get_info_offset(this_p)] == 0xF0) {
		this_p->modulo = modulo_128;
	}
	else if (this_p->modulo == 0 && (c & 1) == 0 && this_p->frame_data[ax25_get_info_offset(this_p)] == 0x08) {	// same for segments
		this_p->modulo = modulo_128;
	}


	if (this_p->modulo == modulo_128) {
		c2 = ax25_get_c2(this_p);
	}
*/

		int dst_c = this_p->frame_data[AX25_DESTINATION * 7 + 6] & SSID_H_MASK;
		int src_c = this_p->frame_data[AX25_SOURCE * 7 + 6] & SSID_H_MASK;

		char cr_text[8];
		char pf_text[8];

		if (dst_c) {
			if (src_c) { *cr = cr_11;  strcpy(cr_text, "cc=11"); strcpy(pf_text, "p/f"); }
			else { *cr = cr_cmd; strcpy(cr_text, "cmd");   strcpy(pf_text, "p"); }
		}
		else {
			if (src_c) { *cr = cr_res; strcpy(cr_text, "res");   strcpy(pf_text, "f"); }
			else { *cr = cr_00;  strcpy(cr_text, "cc=00"); strcpy(pf_text, "p/f"); }
		}

		if ((c & 1) == 0) {

			// Information 			rrr p sss 0		or	sssssss 0  rrrrrrr p

			if (this_p->modulo == modulo_128) {
				*ns = (c >> 1) & 0x7f;
				*pf = c2 & 1;
				*nr = (c2 >> 1) & 0x7f;
			}
			else {
				*ns = (c >> 1) & 7;
				*pf = (c >> 4) & 1;
				*nr = (c >> 5) & 7;
			}

			//snprintf (desc, DESC_SIZ, "I %s, n(s)=%d, n(r)=%d, %s=%d", cr_text, *ns, *nr, pf_text, *pf);
			sprintf(desc, "I %s, n(s)=%d, n(r)=%d, %s=%d, pid=0x%02x", cr_text, *ns, *nr, pf_text, *pf, ax25_get_pid(this_p));
			return (frame_type_I);
		}
		else if ((c & 2) == 0) {

			// Supervisory			rrr p/f ss 0 1		or	0000 ss 0 1  rrrrrrr p/f

			if (this_p->modulo == modulo_128) {
				*pf = c2 & 1;
				*nr = (c2 >> 1) & 0x7f;
			}
			else {
				*pf = (c >> 4) & 1;
				*nr = (c >> 5) & 7;
			}


			switch ((c >> 2) & 3) {
			case 0: sprintf(desc, "RR %s, n(r)=%d, %s=%d", cr_text, *nr, pf_text, *pf);   return (frame_type_S_RR);   break;
			case 1: sprintf(desc, "RNR %s, n(r)=%d, %s=%d", cr_text, *nr, pf_text, *pf);  return (frame_type_S_RNR);  break;
			case 2: sprintf(desc, "REJ %s, n(r)=%d, %s=%d", cr_text, *nr, pf_text, *pf);  return (frame_type_S_REJ);  break;
			case 3: sprintf(desc, "SREJ %s, n(r)=%d, %s=%d", cr_text, *nr, pf_text, *pf); return (frame_type_S_SREJ); break;
			}
		}
		else {

			// Unnumbered			mmm p/f mm 1 1

			*pf = (c >> 4) & 1;

			switch (c & 0xef) {

			case 0x6f: sprintf(desc, "SABME %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_SABME); break;
			case 0x2f: sprintf(desc, "SABM %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_SABM);  break;
			case 0x43: sprintf(desc, "DISC %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_DISC);  break;
			case 0x0f: sprintf(desc, "DM %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_DM);    break;
			case 0x63: sprintf(desc, "UA %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_UA);    break;
			case 0x87: sprintf(desc, "FRMR %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_FRMR);  break;
			case 0x03: sprintf(desc, "UI %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_UI);    break;
			case 0xaf: sprintf(desc, "XID %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_XID);   break;
			case 0xe3: sprintf(desc, "TEST %s, %s=%d", cr_text, pf_text, *pf);  return (frame_type_U_TEST);  break;
			default:   sprintf(desc, "U other???");        				 return (frame_type_U);       break;
			}
		}

		// Should be unreachable but compiler doesn't realize that.
		// Here only to suppress "warning: control reaches end of non-void function"

	return (frame_not_AX25);

} /* end ax25_frame_type */



packet_t ax25_u_frame(char addrs[AX25_MAX_ADDRS][AX25_MAX_ADDR_LEN], int num_addr, cmdres_t cr, ax25_frame_type_t ftype, int pf, int pid, unsigned char *pinfo, int info_len)
{
	packet_t this_p;
	unsigned char *p;
	int ctrl = 0;
	unsigned int t = 999;	// 1 = must be cmd, 0 = must be response, 2 = can be either.
	int i = 0;		// Is Info part allowed?

	this_p = ax25_new();

	if (this_p == NULL) return (NULL);

	this_p->modulo = 0;

	if (!set_addrs(this_p, addrs, num_addr, cr)) {
		Debugprintf("Internal error in %s: Could not set addresses for U frame.\n", __func__);
		ax25_delete(this_p);
		return (NULL);
	}

	switch (ftype) {
		// 1 = cmd only, 0 = res only, 2 = either
	case frame_type_U_SABME:	ctrl = 0x6f;	t = 1;		break;
	case frame_type_U_SABM:	ctrl = 0x2f;	t = 1;		break;
	case frame_type_U_DISC:	ctrl = 0x43;	t = 1;		break;
	case frame_type_U_DM:		ctrl = 0x0f;	t = 0;		break;
	case frame_type_U_UA:		ctrl = 0x63;	t = 0;		break;
	case frame_type_U_FRMR:	ctrl = 0x87;	t = 0;	i = 1;	break;
	case frame_type_U_UI:		ctrl = 0x03;	t = 2;	i = 1;	break;
	case frame_type_U_XID:	ctrl = 0xaf;	t = 2;	i = 1;	break;
	case frame_type_U_TEST:	ctrl = 0xe3;	t = 2;	i = 1;	break;

	default:
		Debugprintf("Internal error in %s: Invalid ftype %d for U frame.\n", __func__, ftype);
		ax25_delete(this_p);
		return (NULL);
		break;
	}
	if (pf) ctrl |= 0x10;

	if (t != 2) {
		if (cr != t) {
			Debugprintf("Internal error in %s: U frame, cr is %d but must be %d. ftype=%d\n", __func__, cr, t, ftype);
		}
	}

	p = this_p->frame_data + this_p->frame_len;
	*p++ = ctrl;
	this_p->frame_len++;

	if (ftype == frame_type_U_UI) {

		// Definitely don't want pid value of 0 (not in valid list)
		// or 0xff (which means more bytes follow).

		if (pid < 0 || pid == 0 || pid == 0xff) {
			Debugprintf("Internal error in %s: U frame, Invalid pid value 0x%02x.\n", __func__, pid);
			pid = AX25_PID_NO_LAYER_3;
		}
		*p++ = pid;
		this_p->frame_len++;
	}

	if (i) {
		if (pinfo != NULL && info_len > 0) {
			if (info_len > AX25_MAX_INFO_LEN) {

				Debugprintf("Internal error in %s: U frame, Invalid information field length %d.\n", __func__, info_len);
				info_len = AX25_MAX_INFO_LEN;
			}
			memcpy(p, pinfo, info_len);
			p += info_len;
			this_p->frame_len += info_len;
		}
	}
	else {
		if (pinfo != NULL && info_len > 0) {
			Debugprintf("Internal error in %s: Info part not allowed for U frame type.\n", __func__);
		}
	}
	*p = '\0';

	//assert(p == this_p->frame_data + this_p->frame_len);
	//assert(this_p->magic1 == MAGIC);
	//assert(this_p->magic2 == MAGIC);

#if PAD2TEST
	ax25_frame_type_t check_ftype;
	cmdres_t check_cr;
	char check_desc[80];
	int check_pf;
	int check_nr;
	int check_ns;

	check_ftype = ax25_frame_type(this_p, &check_cr, check_desc, &check_pf, &check_nr, &check_ns);

	text_color_set(DW_COLOR_DEBUG);
	Debugprintf("check: ftype=%d, desc=\"%s\", pf=%d\n", check_ftype, check_desc, check_pf);

	assert(check_cr == cr);
	assert(check_ftype == ftype);
	assert(check_pf == pf);
	assert(check_nr == -1);
	assert(check_ns == -1);

#endif

	return (this_p);

} /* end ax25_u_frame */






static const char *position_name[1 + AX25_MAX_ADDRS] = {
	"", "Destination ", "Source ",
	"Digi1 ", "Digi2 ", "Digi3 ", "Digi4 ",
	"Digi5 ", "Digi6 ", "Digi7 ", "Digi8 " };

int ax25_parse_addr(int position, char *in_addr, int strict, char *out_addr, int *out_ssid, int *out_heard)
{
	char *p;
	char sstr[8];		/* Should be 1 or 2 digits for SSID. */
	int i, j, k;
	int maxlen;

	*out_addr = '\0';
	*out_ssid = 0;
	*out_heard = 0;

	// Debugprintf ("ax25_parse_addr in: position=%d, '%s', strict=%d\n", position, in_addr, strict);

	if (position < -1) position = -1;
	if (position > AX25_REPEATER_8) position = AX25_REPEATER_8;
	position++;	/* Adjust for position_name above. */

	if (strlen(in_addr) == 0) {
		Debugprintf("%sAddress \"%s\" is empty.\n", position_name[position], in_addr);
		return 0;
	}

	if (strict && strlen(in_addr) >= 2 && strncmp(in_addr, "qA", 2) == 0) {

		Debugprintf("%sAddress \"%s\" is a \"q-construct\" used for communicating with\n", position_name[position], in_addr);
		Debugprintf("APRS Internet Servers.  It should never appear when going over the radio.\n");
	}

	// Debugprintf ("ax25_parse_addr in: %s\n", in_addr);

	maxlen = strict ? 6 : (AX25_MAX_ADDR_LEN - 1);
	p = in_addr;
	i = 0;
	for (p = in_addr; *p != '\0' && *p != '-' && *p != '*'; p++) {
		if (i >= maxlen) {
			Debugprintf("%sAddress is too long. \"%s\" has more than %d characters.\n", position_name[position], in_addr, maxlen);
			return 0;
		}
		if (!isalnum(*p)) {
			Debugprintf("%sAddress, \"%s\" contains character other than letter or digit in character position %d.\n", position_name[position], in_addr, (int)(long)(p - in_addr) + 1);
			return 0;
		}

		out_addr[i++] = *p;
		out_addr[i] = '\0';

#if DECAMAIN	// Hack when running in decode_aprs utility.
		// Exempt the "qA..." case because it was already mentioned.

		if (strict && islower(*p) && strncmp(in_addr, "qA", 2) != 0) {
			text_color_set(DW_COLOR_ERROR);
			Debugprintf("%sAddress has lower case letters. \"%s\" must be all upper case.\n", position_name[position], in_addr);
		}
#else
		if (strict && islower(*p)) {
			Debugprintf("%sAddress has lower case letters. \"%s\" must be all upper case.\n", position_name[position], in_addr);
			return 0;
		}
#endif
	}

	j = 0;
	sstr[j] = '\0';
	if (*p == '-') {
		for (p++; isalnum(*p); p++) {
			if (j >= 2) {
				Debugprintf("%sSSID is too long. SSID part of \"%s\" has more than 2 characters.\n", position_name[position], in_addr);
				return 0;
			}
			sstr[j++] = *p;
			sstr[j] = '\0';
			if (strict && !isdigit(*p)) {
				Debugprintf("%sSSID must be digits. \"%s\" has letters in SSID.\n", position_name[position], in_addr);
				return 0;
			}
		}
		k = atoi(sstr);
		if (k < 0 || k > 15) {
			Debugprintf("%sSSID out of range. SSID of \"%s\" not in range of 0 to 15.\n", position_name[position], in_addr);
			return 0;
		}
		*out_ssid = k;
	}

	if (*p == '*') {
		*out_heard = 1;
		p++;
		if (strict == 2) {
			Debugprintf("\"*\" is not allowed at end of address \"%s\" here.\n", in_addr);
			return 0;
		}
	}

	if (*p != '\0') {
		Debugprintf("Invalid character \"%c\" found in %saddress \"%s\".\n", *p, position_name[position], in_addr);
		return 0;
	}

	// Debugprintf ("ax25_parse_addr out: '%s' %d %d\n", out_addr, *out_ssid, *out_heard);

	return (1);

} /* end ax25_parse_addr */



int set_addrs(packet_t pp, char addrs[AX25_MAX_ADDRS][AX25_MAX_ADDR_LEN], int num_addr, cmdres_t cr)
{
	int n;

	//assert(pp->frame_len == 0);
	//assert(cr == cr_cmd || cr == cr_res);

	if (num_addr < AX25_MIN_ADDRS || num_addr > AX25_MAX_ADDRS) {
		Debugprintf("INTERNAL ERROR: %s %s %d, num_addr = %d\n", __FILE__, __func__, __LINE__, num_addr);
		return (0);
	}

	for (n = 0; n < num_addr; n++) {

		unsigned char *pa = pp->frame_data + n * 7;
		int ok;
		int strict = 1;
		char oaddr[AX25_MAX_ADDR_LEN];
		int ssid;
		int heard;
		int j;

		ok = ax25_parse_addr(n, addrs[n], strict, oaddr, &ssid, &heard);

		if (!ok) return (0);

		// Fill in address.

		memset(pa, ' ' << 1, 6);
		for (j = 0; oaddr[j]; j++) {
			pa[j] = oaddr[j] << 1;
		}
		pa += 6;

		// Fill in SSID.

		*pa = 0x60 | ((ssid & 0xf) << 1);

		// Command / response flag.

		switch (n) {
		case AX25_DESTINATION:
			if (cr == cr_cmd) *pa |= 0x80;
			break;
		case AX25_SOURCE:
			if (cr == cr_res) *pa |= 0x80;
			break;
		default:
			break;
		}

		// Is this the end of address field?

		if (n == num_addr - 1) {
			*pa |= 1;
		}

		pp->frame_len += 7;
	}

	pp->num_addr = num_addr;
	return (1);

} /* end set_addrs */



///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_init.c
//
///////////////////////////////////////////////////////////////////////////////


// Init must be called at start of application.

extern void il2p_init(int debug);

extern struct rs *il2p_find_rs(int nparity);	// Internal later?

extern void il2p_encode_rs(unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out);

extern int il2p_decode_rs(unsigned char *rec_block, int data_size, int num_parity, unsigned char *out);

extern int il2p_get_debug(void);
extern void il2p_set_debug(int debug);


///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_rec.c
//
///////////////////////////////////////////////////////////////////////////////

// Receives a bit stream from demodulator.

extern void il2p_rec_bit(int chan, int subchan, int slice, int dbit);




///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_send.c
//
///////////////////////////////////////////////////////////////////////////////


// Send bit stream to modulator.

string * il2p_send_frame(int chan, packet_t pp, int max_fec, int polarity);



///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_codec.c
//
///////////////////////////////////////////////////////////////////////////////


extern int il2p_encode_frame(packet_t pp, int max_fec, unsigned char *iout);

packet_t il2p_decode_frame(unsigned char *irec);

packet_t il2p_decode_header_payload(unsigned char* uhdr, unsigned char *epayload, int *symbols_corrected);




///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_header.c
//
///////////////////////////////////////////////////////////////////////////////


extern int il2p_type_1_header(packet_t pp, int max_fec, unsigned char *hdr);

extern packet_t il2p_decode_header_type_1(unsigned char *hdr, int num_sym_changed);


extern int il2p_type_0_header(packet_t pp, int max_fec, unsigned char *hdr);

extern int il2p_clarify_header(unsigned char *rec_hdr, unsigned char *corrected_descrambled_hdr);



///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_scramble.c
//
///////////////////////////////////////////////////////////////////////////////

extern void il2p_scramble_block(unsigned char *in, unsigned char *out, int len);

extern void il2p_descramble_block(unsigned char *in, unsigned char *out, int len);


///////////////////////////////////////////////////////////////////////////////
//
// 	il2p_payload.c
//
///////////////////////////////////////////////////////////////////////////////


typedef struct {
	int payload_byte_count;		// Total size, 0 thru 1023
	int payload_block_count;
	int small_block_size;
	int large_block_size;
	int large_block_count;
	int small_block_count;
	int parity_symbols_per_block;	// 2, 4, 6, 8, 16
} il2p_payload_properties_t;

extern int il2p_payload_compute(il2p_payload_properties_t *p, int payload_size, int max_fec);

extern int il2p_encode_payload(unsigned char *payload, int payload_size, int max_fec, unsigned char *enc);

extern int il2p_decode_payload(unsigned char *received, int payload_size, int max_fec, unsigned char *payload_out, int *symbols_corrected);

extern int il2p_get_header_attributes(unsigned char *hdr, int *hdr_type, int *max_fec);

#endif



// Interesting related stuff:
// https://www.kernel.org/doc/html/v4.15/core-api/librs.html
// https://berthub.eu/articles/posts/reed-solomon-for-programmers/ 


#define MAX_NROOTS 16

#define NTAB 5

static struct {
	int symsize;          // Symbol size, bits (1-8).  Always 8 for this application.
	int genpoly;          // Field generator polynomial coefficients.
	int fcs;              // First root of RS code generator polynomial, index form.
			  // FX.25 uses 1 but IL2P uses 0.
	int prim;             // Primitive element to generate polynomial roots.
	int nroots;           // RS code generator polynomial degree (number of roots).
						  // Same as number of check bytes added.
	struct rs *rs;        // Pointer to RS codec control block.  Filled in at init time.
} Tab[NTAB] = {
  {8, 0x11d,   0,   1, 2, NULL },  // 2 parity
  {8, 0x11d,   0,   1, 4, NULL },  // 4 parity
  {8, 0x11d,   0,   1, 6, NULL },  // 6 parity
  {8, 0x11d,   0,   1, 8, NULL },  // 8 parity
  {8, 0x11d,   0,   1, 16, NULL },  // 16 parity
};



static int g_il2p_debug = 0;


/*-------------------------------------------------------------
 *
 * Name:	il2p_init
 *
 * Purpose:	This must be called at application start up time.
 *		It sets up tables for the Reed-Solomon functions.
 *
 * Inputs:	debug	- Enable debug output.
 *
 *--------------------------------------------------------------*/

void il2p_init(int il2p_debug)
{
	g_il2p_debug = il2p_debug;

	for (int i = 0; i < NTAB; i++) {
		//assert(Tab[i].nroots <= MAX_NROOTS);
		Tab[i].rs = INIT_RS(Tab[i].symsize, Tab[i].genpoly, Tab[i].fcs, Tab[i].prim, Tab[i].nroots);
		if (Tab[i].rs == NULL) {
			Debugprintf("IL2P internal error: init_rs_char failed!\n");
			exit(0);
		}
	}

} // end il2p_init


int il2p_get_debug(void)
{
	return (g_il2p_debug);
}
void il2p_set_debug(int debug)
{
	g_il2p_debug = debug;
}


// Find RS codec control block for specified number of parity symbols.

struct rs *il2p_find_rs(int nparity)
{
	for (int n = 0; n < NTAB; n++) {
		if (Tab[n].nroots == nparity) {
			return (Tab[n].rs);
		}
	}
	Debugprintf("IL2P INTERNAL ERROR: il2p_find_rs: control block not found for nparity = %d.\n", nparity);
	return (Tab[0].rs);
}


/*-------------------------------------------------------------
 *
 * Name:	void il2p_encode_rs
 *
 * Purpose:	Add parity symbols to a block of data.
 *
 * Inputs:	tx_data		Header or other data to transmit.
 *		data_size	Number of data bytes in above.
 *		num_parity	Number of parity symbols to add.
 *				Maximum of IL2P_MAX_PARITY_SYMBOLS.
 *
 * Outputs:	parity_out	Specified number of parity symbols
 *
 * Restriction:	data_size + num_parity <= 255 which is the RS block size.
 *		The caller must ensure this.
 *
 *--------------------------------------------------------------*/

void il2p_encode_rs(unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out)
{
	//assert(data_size >= 1);
	//assert(num_parity == 2 || num_parity == 4 || num_parity == 6 || num_parity == 8 || num_parity == 16);
	//assert(data_size + num_parity <= 255);

	unsigned char rs_block[FX25_BLOCK_SIZE];
	memset(rs_block, 0, sizeof(rs_block));
	memcpy(rs_block + sizeof(rs_block) - data_size - num_parity, tx_data, data_size);
	ENCODE_RS(il2p_find_rs(num_parity), rs_block, parity_out);
}

/*-------------------------------------------------------------
 *
 * Name:	void il2p_decode_rs
 *
 * Purpose:	Check and attempt to fix block with FEC.
 *
 * Inputs:	rec_block	Received block composed of data and parity.
 *				Total size is sum of following two parameters.
 *		data_size	Number of data bytes in above.
 *		num_parity	Number of parity symbols (bytes) in above.
 *
 * Outputs:	out		Original with possible corrections applied.
 *				data_size bytes.
 *
 * Returns:	-1 for unrecoverable.
 *		>= 0 for success.  Number of symbols corrected.
 *
 *--------------------------------------------------------------*/

int il2p_decode_rs(unsigned char *rec_block, int data_size, int num_parity, unsigned char *out)
{

	//  Use zero padding in front if data size is too small.

	int n = data_size + num_parity;		// total size in.

	unsigned char rs_block[FX25_BLOCK_SIZE];

	// We could probably do this more efficiently by skipping the
	// processing of the bytes known to be zero.  Good enough for now.

	memset(rs_block, 0, sizeof(rs_block) - n);
	memcpy(rs_block + sizeof(rs_block) - n, rec_block, n);

	if (il2p_get_debug() >= 3) {
		Debugprintf("==============================  il2p_decode_rs  ==============================\n");
		Debugprintf("%d filler zeros, %d data, %d parity\n", (int)(sizeof(rs_block) - n), data_size, num_parity);
		fx_hex_dump(rs_block, sizeof(rs_block));
	}

	int derrlocs[FX25_MAX_CHECK];	// Half would probably be OK.

	int derrors = DECODE_RS(il2p_find_rs(num_parity), rs_block, derrlocs, 0);

	memcpy(out, rs_block + sizeof(rs_block) - n, data_size);

	if (il2p_get_debug() >= 3) {
		if (derrors == 0) {
			Debugprintf("No errors reported for RS block.\n");
		}
		else if (derrors > 0) {
			Debugprintf("%d errors fixed in positions:\n", derrors);
			for (int j = 0; j < derrors; j++) {
				Debugprintf("        %3d  (0x%02x)\n", derrlocs[j], derrlocs[j]);
			}
			fx_hex_dump(rs_block, sizeof(rs_block));
		}
	}

	// It is possible to have a situation where too many errors are
	// present but the algorithm could get a good code block by "fixing"
	// one of the padding bytes that should be 0.

	for (int i = 0; i < derrors; i++) {
		if (derrlocs[i] < sizeof(rs_block) - n) {
			if (il2p_get_debug() >= 3) {
				Debugprintf("RS DECODE ERROR!  Padding position %d should be 0 but it was set to %02x.\n", derrlocs[i], rs_block[derrlocs[i]]);
			}
			derrors = -1;
			break;
		}
	}

	if (il2p_get_debug() >= 3) {
		Debugprintf("==============================  il2p_decode_rs  returns %d  ==============================\n", derrors);
	}
	return (derrors);
}

// end il2p_init.c













void ENCODE_RS(struct rs * rs, DTYPE * data, DTYPE * bb)
{

	int i, j;
	DTYPE feedback;

	memset(bb, 0, NROOTS * sizeof(DTYPE)); // clear out the FEC data area

	for (i = 0; i < NN - NROOTS; i++) {
		feedback = INDEX_OF[data[i] ^ bb[0]];
		if (feedback != A0) {      /* feedback term is non-zero */
			for (j = 1; j < NROOTS; j++)
				bb[j] ^= ALPHA_TO[MODNN(feedback + GENPOLY[NROOTS - j])];
		}
		/* Shift */
		memmove(&bb[0], &bb[1], sizeof(DTYPE)*(NROOTS - 1));
		if (feedback != A0)
			bb[NROOTS - 1] = ALPHA_TO[MODNN(feedback + GENPOLY[0])];
		else
			bb[NROOTS - 1] = 0;
	}
}




int DECODE_RS(struct rs * rs, DTYPE * data, int *eras_pos, int no_eras) {

	int deg_lambda, el, deg_omega;
	int i, j, r, k;
	DTYPE u, q, tmp, num1, num2, den, discr_r;
	//  DTYPE lambda[NROOTS+1], s[NROOTS];	/* Err+Eras Locator poly and syndrome poly */
	//  DTYPE b[NROOTS+1], t[NROOTS+1], omega[NROOTS+1];
	//  DTYPE root[NROOTS], reg[NROOTS+1], loc[NROOTS];
	DTYPE lambda[FX25_MAX_CHECK + 1], s[FX25_MAX_CHECK];	/* Err+Eras Locator poly and syndrome poly */
	DTYPE b[FX25_MAX_CHECK + 1], t[FX25_MAX_CHECK + 1], omega[FX25_MAX_CHECK + 1];
	DTYPE root[FX25_MAX_CHECK], reg[FX25_MAX_CHECK + 1], loc[FX25_MAX_CHECK];
	int syn_error, count;

	/* form the syndromes; i.e., evaluate data(x) at roots of g(x) */
	for (i = 0; i < NROOTS; i++)
		s[i] = data[0];

	for (j = 1; j < NN; j++) {
		for (i = 0; i < NROOTS; i++) {
			if (s[i] == 0) {
				s[i] = data[j];
			}
			else {
				s[i] = data[j] ^ ALPHA_TO[MODNN(INDEX_OF[s[i]] + (FCR + i)*PRIM)];
			}
		}
	}

	/* Convert syndromes to index form, checking for nonzero condition */
	syn_error = 0;
	for (i = 0; i < NROOTS; i++) {
		syn_error |= s[i];
		s[i] = INDEX_OF[s[i]];
	}

	// fprintf(stderr,"syn_error = %4x\n",syn_error);
	if (!syn_error) {
		/* if syndrome is zero, data[] is a codeword and there are no
		 * errors to correct. So return data[] unmodified
		 */
		count = 0;
		goto finish;
	}
	memset(&lambda[1], 0, NROOTS * sizeof(lambda[0]));
	lambda[0] = 1;

	if (no_eras > 0) {
		/* Init lambda to be the erasure locator polynomial */
		lambda[1] = ALPHA_TO[MODNN(PRIM*(NN - 1 - eras_pos[0]))];
		for (i = 1; i < no_eras; i++) {
			u = MODNN(PRIM*(NN - 1 - eras_pos[i]));
			for (j = i + 1; j > 0; j--) {
				tmp = INDEX_OF[lambda[j - 1]];
				if (tmp != A0)
					lambda[j] ^= ALPHA_TO[MODNN(u + tmp)];
			}
		}

#if DEBUG >= 1
		/* Test code that verifies the erasure locator polynomial just constructed
		   Needed only for decoder debugging. */

		   /* find roots of the erasure location polynomial */
		for (i = 1; i <= no_eras; i++)
			reg[i] = INDEX_OF[lambda[i]];

		count = 0;
		for (i = 1, k = IPRIM - 1; i <= NN; i++, k = MODNN(k + IPRIM)) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != A0) {
					reg[j] = MODNN(reg[j] + j);
					q ^= ALPHA_TO[reg[j]];
				}
			if (q != 0)
				continue;
			/* store root and error location number indices */
			root[count] = i;
			loc[count] = k;
			count++;
		}
		if (count != no_eras) {
			fprintf(stderr, "count = %d no_eras = %d\n lambda(x) is WRONG\n", count, no_eras);
			count = -1;
			goto finish;
		}
#if DEBUG >= 2
		fprintf(stderr, "\n Erasure positions as determined by roots of Eras Loc Poly:\n");
		for (i = 0; i < count; i++)
			fprintf(stderr, "%d ", loc[i]);
		fprintf(stderr, "\n");
#endif
#endif
	}
	for (i = 0; i < NROOTS + 1; i++)
		b[i] = INDEX_OF[lambda[i]];

	/*
	 * Begin Berlekamp-Massey algorithm to determine error+erasure
	 * locator polynomial
	 */
	r = no_eras;
	el = no_eras;
	while (++r <= NROOTS) {	/* r is the step number */
	  /* Compute discrepancy at the r-th step in poly-form */
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i - 1] != A0)) {
				discr_r ^= ALPHA_TO[MODNN(INDEX_OF[lambda[i]] + s[r - i - 1])];
			}
		}
		discr_r = INDEX_OF[discr_r];	/* Index form */
		if (discr_r == A0) {
			/* 2 lines below: B(x) <-- x*B(x) */
			memmove(&b[1], b, NROOTS * sizeof(b[0]));
			b[0] = A0;
		}
		else {
			/* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
			t[0] = lambda[0];
			for (i = 0; i < NROOTS; i++) {
				if (b[i] != A0)
					t[i + 1] = lambda[i + 1] ^ ALPHA_TO[MODNN(discr_r + b[i])];
				else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				/*
				 * 2 lines below: B(x) <-- inv(discr_r) *
				 * lambda(x)
				 */
				for (i = 0; i <= NROOTS; i++)
					b[i] = (lambda[i] == 0) ? A0 : MODNN(INDEX_OF[lambda[i]] - discr_r + NN);
			}
			else {
				/* 2 lines below: B(x) <-- x*B(x) */
				memmove(&b[1], b, NROOTS * sizeof(b[0]));
				b[0] = A0;
			}
			memcpy(lambda, t, (NROOTS + 1) * sizeof(t[0]));
		}
	}

	/* Convert lambda to index form and compute deg(lambda(x)) */
	deg_lambda = 0;
	for (i = 0; i < NROOTS + 1; i++) {
		lambda[i] = INDEX_OF[lambda[i]];
		if (lambda[i] != A0)
			deg_lambda = i;
	}
	/* Find roots of the error+erasure locator polynomial by Chien search */
	memcpy(&reg[1], &lambda[1], NROOTS * sizeof(reg[0]));
	count = 0;		/* Number of roots of lambda(x) */
	for (i = 1, k = IPRIM - 1; i <= NN; i++, k = MODNN(k + IPRIM)) {
		q = 1; /* lambda[0] is always 0 */
		for (j = deg_lambda; j > 0; j--) {
			if (reg[j] != A0) {
				reg[j] = MODNN(reg[j] + j);
				q ^= ALPHA_TO[reg[j]];
			}
		}
		if (q != 0)
			continue; /* Not a root */
		  /* store root (index-form) and error location number */
#if DEBUG>=2
		fprintf(stderr, "count %d root %d loc %d\n", count, i, k);
#endif
		root[count] = i;
		loc[count] = k;
		/* If we've already found max possible roots,
		 * abort the search to save time
		 */
		if (++count == deg_lambda)
			break;
	}
	if (deg_lambda != count) {
		/*
		 * deg(lambda) unequal to number of roots => uncorrectable
		 * error detected
		 */
		count = -1;
		goto finish;
	}
	/*
	 * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
	 * x**NROOTS). in index form. Also find deg(omega).
	 */
	deg_omega = 0;
	for (i = 0; i < NROOTS; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i - j] != A0) && (lambda[j] != A0))
				tmp ^= ALPHA_TO[MODNN(s[i - j] + lambda[j])];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = INDEX_OF[tmp];
	}
	omega[NROOTS] = A0;

	/*
	 * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
	 * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
	 */
	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != A0)
				num1 ^= ALPHA_TO[MODNN(omega[i] + i * root[j])];
		}
		num2 = ALPHA_TO[MODNN(root[j] * (FCR - 1) + NN)];
		den = 0;

		/* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
		for (i = min(deg_lambda, NROOTS - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != A0)
				den ^= ALPHA_TO[MODNN(lambda[i + 1] + i * root[j])];
		}
		if (den == 0) {
#if DEBUG >= 1
			fprintf(stderr, "\n ERROR: denominator = 0\n");
#endif
			count = -1;
			goto finish;
		}
		/* Apply error to data */
		if (num1 != 0) {
			data[loc[j]] ^= ALPHA_TO[MODNN(INDEX_OF[num1] + INDEX_OF[num2] + NN - INDEX_OF[den])];
		}
	}
finish:
	if (eras_pos != NULL) {
		for (i = 0; i < count; i++)
			eras_pos[i] = loc[i];
	}
	return count;
}




struct rs *INIT_RS(unsigned int symsize, unsigned int gfpoly, unsigned fcr, unsigned prim,
	unsigned int nroots) {
	struct rs *rs;
	int i, j, sr, root, iprim;

	if (symsize > 8 * sizeof(DTYPE))
		return NULL; /* Need version with ints rather than chars */

	if (fcr >= (1 << symsize))
		return NULL;
	if (prim == 0 || prim >= (1 << symsize))
		return NULL;
	if (nroots >= (1 << symsize))
		return NULL; /* Can't have more roots than symbol values! */

	rs = (struct rs *)calloc(1, sizeof(struct rs));
	if (rs == NULL) {
		Debugprintf("FATAL ERROR: Out of memory.\n");
		exit(0);
	}
	rs->mm = symsize;
	rs->nn = (1 << symsize) - 1;

	rs->alpha_to = (DTYPE *)calloc((rs->nn + 1), sizeof(DTYPE));
	if (rs->alpha_to == NULL) {
		Debugprintf("FATAL ERROR: Out of memory.\n");
		exit(0);
	}
	rs->index_of = (DTYPE *)calloc((rs->nn + 1), sizeof(DTYPE));
	if (rs->index_of == NULL) {
		Debugprintf("FATAL ERROR: Out of memory.\n");
		exit(0);
	}

	/* Generate Galois field lookup tables */
	rs->index_of[0] = A0; /* log(zero) = -inf */
	rs->alpha_to[A0] = 0; /* alpha**-inf = 0 */
	sr = 1;
	for (i = 0; i < rs->nn; i++) {
		rs->index_of[sr] = i;
		rs->alpha_to[i] = sr;
		sr <<= 1;
		if (sr & (1 << symsize))
			sr ^= gfpoly;
		sr &= rs->nn;
	}
	if (sr != 1) {
		/* field generator polynomial is not primitive! */
		free(rs->alpha_to);
		free(rs->index_of);
		free(rs);
		return NULL;
	}

	/* Form RS code generator polynomial from its roots */
	rs->genpoly = (DTYPE *)calloc((nroots + 1), sizeof(DTYPE));
	if (rs->genpoly == NULL) {
		Debugprintf("FATAL ERROR: Out of memory.\n");
		exit(0);
	}
	rs->fcr = fcr;
	rs->prim = prim;
	rs->nroots = nroots;

	/* Find prim-th root of 1, used in decoding */
	for (iprim = 1; (iprim % prim) != 0; iprim += rs->nn)
		;
	rs->iprim = iprim / prim;

	rs->genpoly[0] = 1;
	for (i = 0, root = fcr * prim; i < nroots; i++, root += prim) {
		rs->genpoly[i + 1] = 1;

		/* Multiply rs->genpoly[] by  @**(root + x) */
		for (j = i; j > 0; j--) {
			if (rs->genpoly[j] != 0)
				rs->genpoly[j] = rs->genpoly[j - 1] ^ rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[j]] + root)];
			else
				rs->genpoly[j] = rs->genpoly[j - 1];
		}
		/* rs->genpoly[0] can never be zero */
		rs->genpoly[0] = rs->alpha_to[modnn(rs, rs->index_of[rs->genpoly[0]] + root)];
	}
	/* convert rs->genpoly[] to index form for quicker encoding */
	for (i = 0; i <= nroots; i++) {
		rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
	}

	// diagnostic prints
#if 0
	printf("Alpha To:\n\r");
	for (i = 0; i < sizeof(DTYPE)*(rs->nn + 1); i++)
		printf("0x%2x,", rs->alpha_to[i]);
	printf("\n\r");

	printf("Index Of:\n\r");
	for (i = 0; i < sizeof(DTYPE)*(rs->nn + 1); i++)
		printf("0x%2x,", rs->index_of[i]);
	printf("\n\r");

	printf("GenPoly:\n\r");
	for (i = 0; i <= nroots; i++)
		printf("0x%2x,", rs->genpoly[i]);
	printf("\n\r");
#endif
	return rs;
}


// TEMPORARY!!!
// FIXME: We already have multiple copies of this.
// Consolidate them into one somewhere.

void fx_hex_dump(unsigned char *p, int len)
{
	int n, i, offset;

	offset = 0;
	while (len > 0) {
		n = len < 16 ? len : 16;
		Debugprintf("  %03x: ", offset);
		for (i = 0; i < n; i++) {
			Debugprintf(" %02x", p[i]);
		}
		for (i = n; i < 16; i++) {
			Debugprintf("   ");
		}
		Debugprintf("  ");
		for (i = 0; i < n; i++) {
			Debugprintf("%c", isprint(p[i]) ? p[i] : '.');
		}
		Debugprintf("\n");
		p += 16;
		offset += 16;
		len -= 16;
	}
}


/*-------------------------------------------------------------
 *
 * File:	il2p_codec.c
 *
 * Purpose:	Convert IL2P encoded format from and to direwolf internal packet format.
 *
 *--------------------------------------------------------------*/


 /*-------------------------------------------------------------
  *
  * Name:	il2p_encode_frame
  *
  * Purpose:	Convert AX.25 frame to IL2P encoding.
  *
  * Inputs:	chan	- Audio channel number, 0 = first.
  *
  *		pp	- Packet object pointer.
  *
  *		max_fec	- 1 to send maximum FEC size rather than automatic.
  *
  * Outputs:	iout	- Encoded result, excluding the 3 byte sync word.
  *			  Caller should provide  IL2P_MAX_PACKET_SIZE  bytes.
  *
  * Returns:	Number of bytes for transmission.
  *		-1 is returned for failure.
  *
  * Description:	Encode into IL2P format.
  *
  * Errors:	If something goes wrong, return -1.
  *
  *		Most likely reason is that the frame is too large.
  *		IL2P has a max payload size of 1023 bytes.
  *		For a type 1 header, this is the maximum AX.25 Information part size.
  *		For a type 0 header, this is the entire AX.25 frame.
  *
  *--------------------------------------------------------------*/

int il2p_encode_frame(packet_t pp, int max_fec, unsigned char *iout)
{
	// Can a type 1 header be used?

	unsigned char hdr[IL2P_HEADER_SIZE + IL2P_HEADER_PARITY];
	int e;
	int out_len = 0;

	e = il2p_type_1_header(pp, max_fec, hdr);
	if (e >= 0) {
		il2p_scramble_block(hdr, iout, IL2P_HEADER_SIZE);
		il2p_encode_rs(iout, IL2P_HEADER_SIZE, IL2P_HEADER_PARITY, iout + IL2P_HEADER_SIZE);
		out_len = IL2P_HEADER_SIZE + IL2P_HEADER_PARITY;

		if (e == 0) {
			// Success. No info part.
			return (out_len);
		}

		// Payload is AX.25 info part.

		unsigned char *pinfo;
		int info_len;
		info_len = ax25_get_info(pp, &pinfo);

		int k = il2p_encode_payload(pinfo, info_len, max_fec, iout + out_len);
		if (k > 0) {
			out_len += k;
			// Success. Info part was <= 1023 bytes.
			return (out_len);
		}

		// Something went wrong with the payload encoding.
		return (-1);
	}
	else if (e == -1) {

		// Could not use type 1 header for some reason.
		// e.g. More than 2 addresses, extended (mod 128) sequence numbers, etc.

		e = il2p_type_0_header(pp, max_fec, hdr);
		if (e > 0) {

			il2p_scramble_block(hdr, iout, IL2P_HEADER_SIZE);
			il2p_encode_rs(iout, IL2P_HEADER_SIZE, IL2P_HEADER_PARITY, iout + IL2P_HEADER_SIZE);
			out_len = IL2P_HEADER_SIZE + IL2P_HEADER_PARITY;

			// Payload is entire AX.25 frame.

			unsigned char *frame_data_ptr = ax25_get_frame_data_ptr(pp);
			int frame_len = ax25_get_frame_len(pp);
			int k = il2p_encode_payload(frame_data_ptr, frame_len, max_fec, iout + out_len);
			if (k > 0) {
				out_len += k;
				// Success. Entire AX.25 frame <= 1023 bytes.
				return (out_len);
			}
			// Something went wrong with the payload encoding.
			return (-1);
		}
		else if (e == 0) {
			// Impossible condition.  Type 0 header must have payload.
			return (-1);
		}
		else {
			// AX.25 frame is too large.
			return (-1);
		}
	}

	// AX.25 Information part is too large.
	return (-1);
}



/*-------------------------------------------------------------
 *
 * Name:	il2p_decode_frame
 *
 * Purpose:	Convert IL2P encoding to AX.25 frame.
 *		This is only used during testing, with a whole encoded frame.
 *		During reception, the header would have FEC and descrambling
 *		applied first so we would know how much to collect for the payload.
 *
 * Inputs:	irec	- Received IL2P frame excluding the 3 byte sync word.
 *
 * Future Out:	Number of symbols corrected.
 *
 * Returns:	Packet pointer or NULL for error.
 *
 *--------------------------------------------------------------*/

packet_t il2p_decode_frame(unsigned char *irec)
{
	unsigned char uhdr[IL2P_HEADER_SIZE];		// After FEC and descrambling.
	int e = il2p_clarify_header(irec, uhdr);

	// TODO?: for symmetry we might want to clarify the payload before combining.

	return (il2p_decode_header_payload(uhdr, irec + IL2P_HEADER_SIZE + IL2P_HEADER_PARITY, &e));
}


/*-------------------------------------------------------------
 *
 * Name:	il2p_decode_header_payload
 *
 * Purpose:	Convert IL2P encoding to AX.25 frame
 *
 * Inputs:	uhdr 		- Received header after FEC and descrambling.
 *		epayload	- Encoded payload.
 *
 * In/Out:	symbols_corrected - Symbols (bytes) corrected in the header.
 *				  Should be 0 or 1 because it has 2 parity symbols.
 *				  Here we add number of corrections for the payload.
 *
 * Returns:	Packet pointer or NULL for error.
 *
 *--------------------------------------------------------------*/

packet_t il2p_decode_header_payload(unsigned char* uhdr, unsigned char *epayload, int *symbols_corrected)
{
	int hdr_type;
	int max_fec;
	int payload_len = il2p_get_header_attributes(uhdr, &hdr_type, &max_fec);

	packet_t pp = NULL;

	if (hdr_type == 1) {

		// Header type 1.  Any payload is the AX.25 Information part.

		pp = il2p_decode_header_type_1(uhdr, *symbols_corrected);
		if (pp == NULL) {
			// Failed for some reason.
			return (NULL);
		}

		if (payload_len > 0) {
			// This is the AX.25 Information part.

			unsigned char extracted[IL2P_MAX_PAYLOAD_SIZE];
			int e = il2p_decode_payload(epayload, payload_len, max_fec, extracted, symbols_corrected);

			// It would be possible to have a good header but too many errors in the payload.

			if (e <= 0) {
				ax25_delete(pp);
				pp = NULL;
				return (pp);
			}

			if (e != payload_len) {
				Debugprintf("IL2P Internal Error: %s(): hdr_type=%d, max_fec=%d, payload_len=%d, e=%d.\n", __func__, hdr_type, max_fec, payload_len, e);
			}

			ax25_set_info(pp, extracted, payload_len);
		}
		return (pp);
	}
	else {

		// Header type 0.  The payload is the entire AX.25 frame.

		unsigned char extracted[IL2P_MAX_PAYLOAD_SIZE];
		int e = il2p_decode_payload(epayload, payload_len, max_fec, extracted, symbols_corrected);

		if (e <= 0) {	// Payload was not received correctly.
			return (NULL);
		}

		if (e != payload_len) {
			Debugprintf("IL2P Internal Error: %s(): hdr_type=%d, e=%d, payload_len=%d\n", __func__, hdr_type, e, payload_len);
			return (NULL);
		}

		alevel_t alevel;
		memset(&alevel, 0, sizeof(alevel));
		//alevel = demod_get_audio_level (chan, subchan); 	// What TODO? We don't know channel here.
						// I think alevel gets filled in somewhere later making
						// this redundant.

		pp = ax25_from_frame(extracted, payload_len, alevel);
		return (pp);
	}

} // end il2p_decode_header_payload

// end il2p_codec.c



/*--------------------------------------------------------------------------------
 *
 * File:	il2p_header.c
 *
 * Purpose:	Functions to deal with the IL2P header.
 *
 * Reference:	http://tarpn.net/t/il2p/il2p-specification0-4.pdf
 *
 *--------------------------------------------------------------------------------*/



 // Convert ASCII to/from DEC SIXBIT as defined here:
 // https://en.wikipedia.org/wiki/Six-bit_character_code#DEC_six-bit_code

static inline int ascii_to_sixbit(int a)
{
	if (a >= ' ' && a <= '_') return (a - ' ');
	return (31);	// '?' for any invalid.
}

static inline int sixbit_to_ascii(int s)
{
	return (s + ' ');
}

// Functions for setting the various header fields.
// It is assumed that it was zeroed first so only the '1' bits are set.

static void set_field(unsigned char *hdr, int bit_num, int lsb_index, int width, int value)
{
	while (width > 0 && value != 0) {
		//assert(lsb_index >= 0 && lsb_index <= 11);
		if (value & 1) {
			hdr[lsb_index] |= 1 << bit_num;
		}
		value >>= 1;
		lsb_index--;
		width--;
	}
	//assert(value == 0);
}

#define SET_UI(hdr,val) set_field(hdr, 6, 0, 1, val)

#define SET_PID(hdr,val) set_field(hdr, 6, 4, 4, val)

#define SET_CONTROL(hdr,val) set_field(hdr, 6, 11, 7, val)


#define SET_FEC_LEVEL(hdr,val) set_field(hdr, 7, 0, 1, val)

#define SET_HDR_TYPE(hdr,val) set_field(hdr, 7, 1, 1, val)

#define SET_PAYLOAD_BYTE_COUNT(hdr,val) set_field(hdr, 7, 11, 10, val)


// Extracting the fields.

static int get_field(unsigned char *hdr, int bit_num, int lsb_index, int width)
{
	int result = 0;
	lsb_index -= width - 1;
	while (width > 0) {
		result <<= 1;
		//assert(lsb_index >= 0 && lsb_index <= 11);
		if (hdr[lsb_index] & (1 << bit_num)) {
			result |= 1;
		}
		lsb_index++;
		width--;
	}
	return (result);
}

#define GET_UI(hdr) get_field(hdr, 6, 0, 1)

#define GET_PID(hdr) get_field(hdr, 6, 4, 4)

#define GET_CONTROL(hdr) get_field(hdr, 6, 11, 7)


#define GET_FEC_LEVEL(hdr) get_field(hdr, 7, 0, 1)

#define GET_HDR_TYPE(hdr) get_field(hdr, 7, 1, 1)

#define GET_PAYLOAD_BYTE_COUNT(hdr) get_field(hdr, 7, 11, 10)



// AX.25 'I' and 'UI' frames have a protocol ID which determines how the
// information part should be interpreted.
// Here we squeeze the most common cases down to 4 bits.
// Return -1 if translation is not possible.  Fall back to type 0 header in this case.

static int encode_pid(packet_t pp)
{
	int pid = ax25_get_pid(pp);

	if ((pid & 0x30) == 0x20) return (0x2);		// AX.25 Layer 3
	if ((pid & 0x30) == 0x10) return (0x2);		// AX.25 Layer 3
	if (pid == 0x01) return (0x3);			// ISO 8208 / CCIT X.25 PLP
	if (pid == 0x06) return (0x4);			// Compressed TCP/IP
	if (pid == 0x07) return (0x5);			// Uncompressed TCP/IP
	if (pid == 0x08) return (0x6);			// Segmentation fragmen
	if (pid == 0xcc) return (0xb);			// ARPA Internet Protocol
	if (pid == 0xcd) return (0xc);			// ARPA Address Resolution
	if (pid == 0xce) return (0xd);			// FlexNet
	if (pid == 0xcf) return (0xe);			// TheNET
	if (pid == 0xf0) return (0xf);			// No L3
	return (-1);
}

// Convert IL2P 4 bit PID to AX.25 8 bit PID.


static int decode_pid(int pid)
{
	static const unsigned char axpid[16] = {
	0xf0,			// Should not happen. 0 is for 'S' frames.
	0xf0,			// Should not happen. 1 is for 'U' frames (but not UI).
	0x20,			// AX.25 Layer 3
	0x01,			// ISO 8208 / CCIT X.25 PLP
	0x06,			// Compressed TCP/IP
	0x07,			// Uncompressed TCP/IP
	0x08,			// Segmentation fragment
	0xf0,			// Future
	0xf0,			// Future
	0xf0,			// Future
	0xf0,			// Future
	0xcc,			// ARPA Internet Protocol
	0xcd,			// ARPA Address Resolution
	0xce,			// FlexNet
	0xcf,			// TheNET
	0xf0 };			// No L3

	//assert(pid >= 0 && pid <= 15);
	return (axpid[pid]);
}



/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_type_1_header
 *
 * Purpose:	Attempt to create type 1 header from packet object.
 *
 * Inputs:	pp	- Packet object.
 *
 *		max_fec	- 1 to use maximum FEC symbols , 0 for automatic.
 *
 * Outputs:	hdr	- IL2P header with no scrambling or parity symbols.
 *			  Must be large enough to hold IL2P_HEADER_SIZE unsigned bytes.
 *
 * Returns:	Number of bytes for information part or -1 for failure.
 *		In case of failure, fall back to type 0 transparent encapsulation.
 *
 * Description:	Type 1 Headers do not support AX.25 repeater callsign addressing,
 *		Modulo-128 extended mode window sequence numbers, nor any callsign
 *		characters that cannot translate to DEC SIXBIT.
 *		If these cases are encountered during IL2P packet encoding,
 *		the encoder switches to Type 0 Transparent Encapsulation.
 *		SABME can't be handled by type 1.
 *
 *--------------------------------------------------------------------------------*/

int il2p_type_1_header(packet_t pp, int max_fec, unsigned char *hdr)
{
	memset(hdr, 0, IL2P_HEADER_SIZE);

	if (ax25_get_num_addr(pp) != 2) {
		// Only two addresses are allowed for type 1 header.
		return (-1);
	}

	// Check does not apply for 'U' frames but put in one place rather than two.

	if (ax25_get_modulo(pp) == 128) return(-1);

	// Destination and source addresses go into low bits 0-5 for bytes 0-11.

	char dst_addr[AX25_MAX_ADDR_LEN];
	char src_addr[AX25_MAX_ADDR_LEN];

	ax25_get_addr_no_ssid(pp, AX25_DESTINATION, dst_addr);
	int dst_ssid = ax25_get_ssid(pp, AX25_DESTINATION);

	ax25_get_addr_no_ssid(pp, AX25_SOURCE, src_addr);
	int src_ssid = ax25_get_ssid(pp, AX25_SOURCE);

	unsigned char *a = (unsigned char *)dst_addr;
	for (int i = 0; *a != '\0'; i++, a++) {
		if (*a < ' ' || *a > '_') {
			// Shouldn't happen but follow the rule.
			return (-1);
		}
		hdr[i] = ascii_to_sixbit(*a);
	}

	a = (unsigned char *)src_addr;
	for (int i = 6; *a != '\0'; i++, a++) {
		if (*a < ' ' || *a > '_') {
			// Shouldn't happen but follow the rule.
			return (-1);
		}
		hdr[i] = ascii_to_sixbit(*a);
	}

	// Byte 12 has DEST SSID in upper nybble and SRC SSID in lower nybble and 
	hdr[12] = (dst_ssid << 4) | src_ssid;

	ax25_frame_type_t frame_type;
	cmdres_t cr;			// command or response.
	char description[64];
	int pf;				// Poll/Final.
	int nr, ns;			// Sequence numbers.

	frame_type = ax25_frame_type(pp, &cr, description, &pf, &nr, &ns);

	//Debugprintf ("%s(): %s-%d>%s-%d: %s\n", __func__, src_addr, src_ssid, dst_addr, dst_ssid, description);

	switch (frame_type) {

	case frame_type_S_RR:		// Receive Ready - System Ready To Receive
	case frame_type_S_RNR:		// Receive Not Ready - TNC Buffer Full
	case frame_type_S_REJ:		// Reject Frame - Out of Sequence or Duplicate
	case frame_type_S_SREJ:		// Selective Reject - Request single frame repeat

		// S frames (RR, RNR, REJ, SREJ), mod 8, have control N(R) P/F S S 0 1
		// These are mapped into    P/F N(R) C S S
		// Bit 6 is not mentioned in documentation but it is used for P/F for the other frame types.
		// C is copied from the C bit in the destination addr.
		// C from source is not used here.  Reception assumes it is the opposite.
		// PID is set to 0, meaning none, for S frames.

		SET_UI(hdr, 0);
		SET_PID(hdr, 0);
		SET_CONTROL(hdr, (pf << 6) | (nr << 3) | (((cr == cr_cmd) | (cr == cr_11)) << 2));

		// This gets OR'ed into the above.
		switch (frame_type) {
		case frame_type_S_RR:	SET_CONTROL(hdr, 0);	break;
		case frame_type_S_RNR:	SET_CONTROL(hdr, 1);	break;
		case frame_type_S_REJ:	SET_CONTROL(hdr, 2);	break;
		case frame_type_S_SREJ:	SET_CONTROL(hdr, 3);	break;
		default:	break;
		}

		break;

	case frame_type_U_SABM:		// Set Async Balanced Mode
	case frame_type_U_DISC:		// Disconnect
	case frame_type_U_DM:		// Disconnect Mode
	case frame_type_U_UA:		// Unnumbered Acknowledge
	case frame_type_U_FRMR:		// Frame Reject
	case frame_type_U_UI:		// Unnumbered Information
	case frame_type_U_XID:		// Exchange Identification
	case frame_type_U_TEST:		// Test

		// The encoding allows only 3 bits for frame type and SABME got left out.
		// Control format:  P/F opcode[3] C n/a n/a
		// The grayed out n/a bits are observed as 00 in the example.
		// The header UI field must also be set for UI frames.
		// PID is set to 1 for all U frames other than UI.

		if (frame_type == frame_type_U_UI) {
			SET_UI(hdr, 1);	// I guess this is how we distinguish 'I' and 'UI'
				// on the receiving end.
			int pid = encode_pid(pp);
			if (pid < 0) return (-1);
			SET_PID(hdr, pid);
		}
		else {
			SET_PID(hdr, 1);	// 1 for 'U' other than 'UI'.
		}

		// Each of the destination and source addresses has a "C" bit.
		// They should normally have the opposite setting.
		// IL2P has only a single bit to represent 4 possbilities.
		//
		//	dst	src	il2p	meaning
		//	---	---	----	-------
		//	0	0	0	Not valid (earlier protocol version)
		//	1	0	1	Command (v2)
		//	0	1	0	Response (v2)
		//	1	1	1	Not valid (earlier protocol version)
		//
		// APRS does not mention how to set these bits and all 4 combinations
		// are seen in the wild.  Apparently these are ignored on receive and no
		// one cares.  Here we copy from the C bit in the destination address.
		// It should be noted that the case of both C bits being the same can't
		// be represented so the il2p encode/decode bit not produce exactly the
		// same bits.  We see this in the second example in the protocol spec.
		// The original UI frame has both C bits of 0 so it is received as a response.

		SET_CONTROL(hdr, (pf << 6) | (((cr == cr_cmd) | (cr == cr_11)) << 2));

		// This gets OR'ed into the above.
		switch (frame_type) {
		case frame_type_U_SABM:	SET_CONTROL(hdr, 0 << 3);	break;
		case frame_type_U_DISC:	SET_CONTROL(hdr, 1 << 3);	break;
		case frame_type_U_DM:	SET_CONTROL(hdr, 2 << 3);	break;
		case frame_type_U_UA:	SET_CONTROL(hdr, 3 << 3);	break;
		case frame_type_U_FRMR:	SET_CONTROL(hdr, 4 << 3);	break;
		case frame_type_U_UI:	SET_CONTROL(hdr, 5 << 3);	break;
		case frame_type_U_XID:	SET_CONTROL(hdr, 6 << 3);	break;
		case frame_type_U_TEST:	SET_CONTROL(hdr, 7 << 3);	break;
		default:	break;
		}
		break;

	case frame_type_I:		// Information

		// I frames (mod 8 only)
		// encoded control: P/F N(R) N(S)

		SET_UI(hdr, 0);

		int pid2 = encode_pid(pp);
		if (pid2 < 0) return (-1);
		SET_PID(hdr, pid2);

		SET_CONTROL(hdr, (pf << 6) | (nr << 3) | ns);
		break;

	case frame_type_U_SABME:		// Set Async Balanced Mode, Extended
	case frame_type_U:			// other Unnumbered, not used by AX.25.
	case frame_not_AX25:		// Could not get control byte from frame.
	default:

		// Fall back to the header type 0 for these.
		return (-1);
	}

	// Common for all header type 1.

		// Bit 7 has [FEC Level:1], [HDR Type:1], [Payload byte Count:10]

	SET_FEC_LEVEL(hdr, max_fec);
	SET_HDR_TYPE(hdr, 1);

	unsigned char *pinfo;
	int info_len;

	info_len = ax25_get_info(pp, &pinfo);
	if (info_len < 0 || info_len > IL2P_MAX_PAYLOAD_SIZE) {
		return (-2);
	}

	SET_PAYLOAD_BYTE_COUNT(hdr, info_len);
	return (info_len);
}


// This should create a packet from the IL2P header.
// The information part will not be filled in.

static void trim(char *stuff)
{
	char *p = stuff + strlen(stuff) - 1;
	while (strlen(stuff) > 0 && (*p == ' ')) {
		*p = '\0';
		p--;
	}
}



/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_decode_header_type_1
 *
 * Purpose:	Attempt to convert type 1 header to a packet object.
 *
 * Inputs:	hdr - IL2P header with no scrambling or parity symbols.
 *
 *		num_sym_changed - Number of symbols changed by FEC in the header.
 *				Should be 0 or 1.
 *
 * Returns:	Packet Object or NULL for failure.
 *
 * Description:	A later step will process the payload for the information part.
 *
 *--------------------------------------------------------------------------------*/

packet_t il2p_decode_header_type_1(unsigned char *hdr, int num_sym_changed)
{

	if (GET_HDR_TYPE(hdr) != 1) {
		Debugprintf("IL2P Internal error.  Should not be here: %s, when header type is 0.\n", __func__);
		return (NULL);
	}

	// First get the addresses including SSID.

	char addrs[AX25_MAX_ADDRS][AX25_MAX_ADDR_LEN];
	int num_addr = 2;
	memset(addrs, 0, 2 * AX25_MAX_ADDR_LEN);

	// The IL2P header uses 2 parity symbols which means a single corrupted symbol (byte)
	// can always be corrected.
	// However, I have seen cases, where the error rate is very high, where the RS decoder
	// thinks it found a valid code block by changing one symbol but it was the wrong one.
	// The result is trash.  This shows up as address fields like 'R&G4"A' and 'TEW\ !'.
	// I added a sanity check here to catch characters other than upper case letters and digits.
	// The frame should be rejected in this case.  The question is whether to discard it
	// silently or print a message so the user can see that something strange is happening?
	// My current thinking is that it should be silently ignored if the header has been
	// modified (correctee or more likely, made worse in this cases).
	// If no changes were made, something weird is happening.  We should mention it for
	// troubleshooting rather than sweeping it under the rug.

	// The same thing has been observed with the payload, under very high error conditions,
	// and max_fec==0.  Here I don't see a good solution.  AX.25 information can contain
	// "binary" data so I'm not sure what sort of sanity check could be added.
	// This was not observed with max_fec==1.  If we make that the default, same as Nino TNC,
	// it would be extremely extremely unlikely unless someone explicitly selects weaker FEC.

	// TODO: We could do something similar for header type 0.
	// The address fields should be all binary zero values.
	// Someone overly ambitious might check the addresses found in the first payload block.

	for (int i = 0; i <= 5; i++) {
		addrs[AX25_DESTINATION][i] = sixbit_to_ascii(hdr[i] & 0x3f);
	}
	trim(addrs[AX25_DESTINATION]);
	for (int i = 0; i < strlen(addrs[AX25_DESTINATION]); i++) {
		if (!isupper(addrs[AX25_DESTINATION][i]) && !isdigit(addrs[AX25_DESTINATION][i])) {
			if (num_sym_changed == 0) {
				// This can pop up sporadically when receiving random noise.
				// Would be better to show only when debug is enabled but variable not available here.
				// TODO: For now we will just suppress it.
					//text_color_set(DW_COLOR_ERROR);
					//Debugprintf ("IL2P: Invalid character '%c' in destination address '%s'\n", addrs[AX25_DESTINATION][i], addrs[AX25_DESTINATION]);
			}
			return (NULL);
		}
	}
	sprintf(addrs[AX25_DESTINATION] + strlen(addrs[AX25_DESTINATION]), "-%d", (hdr[12] >> 4) & 0xf);

	for (int i = 0; i <= 5; i++) {
		addrs[AX25_SOURCE][i] = sixbit_to_ascii(hdr[i + 6] & 0x3f);
	}
	trim(addrs[AX25_SOURCE]);
	for (int i = 0; i < strlen(addrs[AX25_SOURCE]); i++) {
		if (!isupper(addrs[AX25_SOURCE][i]) && !isdigit(addrs[AX25_SOURCE][i])) {
			if (num_sym_changed == 0) {
				// This can pop up sporadically when receiving random noise.
				// Would be better to show only when debug is enabled but variable not available here.
				// TODO: For now we will just suppress it.
					//text_color_set(DW_COLOR_ERROR);
					//Debugprintf ("IL2P: Invalid character '%c' in source address '%s'\n", addrs[AX25_SOURCE][i], addrs[AX25_SOURCE]);
			}
			return (NULL);
		}
	}
	sprintf(addrs[AX25_SOURCE] + strlen(addrs[AX25_SOURCE]), "-%d", hdr[12] & 0xf);

	// The PID field gives us the general type.
	// 0 = 'S' frame.
	// 1 = 'U' frame other than UI.
	// others are either 'UI' or 'I' depending on the UI field.

	int pid = GET_PID(hdr);
	int ui = GET_UI(hdr);

	if (pid == 0) {

		// 'S' frame.
		// The control field contains: P/F N(R) C S S

		int control = GET_CONTROL(hdr);
		cmdres_t cr = (control & 0x04) ? cr_cmd : cr_res;
		ax25_frame_type_t ftype;
		switch (control & 0x03) {
		case 0: ftype = frame_type_S_RR; break;
		case 1: ftype = frame_type_S_RNR; break;
		case 2: ftype = frame_type_S_REJ; break;
		default: ftype = frame_type_S_SREJ; break;
		}
		int modulo = 8;
		int nr = (control >> 3) & 0x07;
		int pf = (control >> 6) & 0x01;
		unsigned char *pinfo = NULL;	// Any info for SREJ will be added later.
		int info_len = 0;
		return (ax25_s_frame(addrs, num_addr, cr, ftype, modulo, nr, pf, pinfo, info_len));
	}
	else if (pid == 1) {

		// 'U' frame other than 'UI'.
		// The control field contains: P/F OPCODE{3) C x x

		int control = GET_CONTROL(hdr);
		cmdres_t cr = (control & 0x04) ? cr_cmd : cr_res;
		int axpid = 0;	// unused for U other than UI.
		ax25_frame_type_t ftype;
		switch ((control >> 3) & 0x7) {
		case 0: ftype = frame_type_U_SABM; break;
		case 1: ftype = frame_type_U_DISC; break;
		case 2: ftype = frame_type_U_DM; break;
		case 3: ftype = frame_type_U_UA; break;
		case 4: ftype = frame_type_U_FRMR; break;
		case 5: ftype = frame_type_U_UI; axpid = 0xf0; break;		// Should not happen with IL2P pid == 1.
		case 6: ftype = frame_type_U_XID; break;
		default: ftype = frame_type_U_TEST; break;
		}
		int pf = (control >> 6) & 0x01;
		unsigned char *pinfo = NULL;	// Any info for UI, XID, TEST will be added later.
		int info_len = 0;
		return (ax25_u_frame(addrs, num_addr, cr, ftype, pf, axpid, pinfo, info_len));
	}
	else if (ui) {

		// 'UI' frame.
		// The control field contains: P/F OPCODE{3) C x x

		int control = GET_CONTROL(hdr);
		cmdres_t cr = (control & 0x04) ? cr_cmd : cr_res;
		ax25_frame_type_t ftype = frame_type_U_UI;
		int pf = (control >> 6) & 0x01;
		int axpid = decode_pid(GET_PID(hdr));
		unsigned char *pinfo = NULL;	// Any info for UI, XID, TEST will be added later.
		int info_len = 0;
		return (ax25_u_frame(addrs, num_addr, cr, ftype, pf, axpid, pinfo, info_len));
	}
	else {

		// 'I' frame.
		// The control field contains: P/F N(R) N(S)

		int control = GET_CONTROL(hdr);
		cmdres_t cr = cr_cmd;		// Always command.
		int pf = (control >> 6) & 0x01;
		int nr = (control >> 3) & 0x7;
		int ns = control & 0x7;
		int modulo = 8;
		int axpid = decode_pid(GET_PID(hdr));
		unsigned char *pinfo = NULL;	// Any info for UI, XID, TEST will be added later.
		int info_len = 0;
		return (ax25_i_frame(addrs, num_addr, cr, modulo, nr, ns, pf, axpid, pinfo, info_len));
	}
	return (NULL);	// unreachable but avoid warning.

} // end


/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_type_0_header
 *
 * Purpose:	Attempt to create type 0 header from packet object.
 *
 * Inputs:	pp	- Packet object.
 *
 *		max_fec	- 1 to use maximum FEC symbols, 0 for automatic.
 *
 * Outputs:	hdr	- IL2P header with no scrambling or parity symbols.
 *			  Must be large enough to hold IL2P_HEADER_SIZE unsigned bytes.
 *
 * Returns:	Number of bytes for information part or -1 for failure.
 *		In case of failure, fall back to type 0 transparent encapsulation.
 *
 * Description:	The type 0 header is used when it is not one of the restricted cases
 *		covered by the type 1 header.
 *		The AX.25 frame is put in the payload.
 *		This will cover: more than one address, mod 128 sequences, etc.
 *
 *--------------------------------------------------------------------------------*/

int il2p_type_0_header(packet_t pp, int max_fec, unsigned char *hdr)
{
	memset(hdr, 0, IL2P_HEADER_SIZE);

	// Bit 7 has [FEC Level:1], [HDR Type:1], [Payload byte Count:10]

	SET_FEC_LEVEL(hdr, max_fec);
	SET_HDR_TYPE(hdr, 0);

	int frame_len = ax25_get_frame_len(pp);

	if (frame_len < 14 || frame_len > IL2P_MAX_PAYLOAD_SIZE) {
		return (-2);
	}

	SET_PAYLOAD_BYTE_COUNT(hdr, frame_len);
	return (frame_len);
}


/***********************************************************************************
 *
 * Name:        il2p_get_header_attributes
 *
 * Purpose:     Extract a few attributes from an IL2p header.
 *
 * Inputs:      hdr	- IL2P header structure.
 *
 * Outputs:     hdr_type - 0 or 1.
 *
 *		max_fec	- 0 for automatic or 1 for fixed maximum size.
 *
 * Returns:	Payload byte count.   (actual payload size, not the larger encoded format)
 *
 ***********************************************************************************/


int il2p_get_header_attributes(unsigned char *hdr, int *hdr_type, int *max_fec)
{
	*hdr_type = GET_HDR_TYPE(hdr);
	*max_fec = GET_FEC_LEVEL(hdr);
	return(GET_PAYLOAD_BYTE_COUNT(hdr));
}


/***********************************************************************************
 *
 * Name:        il2p_clarify_header
 *
 * Purpose:     Convert received header to usable form.
 *		This involves RS FEC then descrambling.
 *
 * Inputs:      rec_hdr	- Header as received over the radio.
 *
 * Outputs:     corrected_descrambled_hdr - After RS FEC and unscrambling.
 *
 * Returns:	Number of symbols that were corrected:
 *		 0 = No errors
 *		 1 = Single symbol corrected.
 *		 <0 = Unable to obtain good header.
 *
 ***********************************************************************************/

int il2p_clarify_header(unsigned char *rec_hdr, unsigned char *corrected_descrambled_hdr)
{
	unsigned char corrected[IL2P_HEADER_SIZE + IL2P_HEADER_PARITY];

	int e = il2p_decode_rs(rec_hdr, IL2P_HEADER_SIZE, IL2P_HEADER_PARITY, corrected);

	if (e > 1)		// only have 2 rs bytes so can only detect 1 error
	{
		Debugprintf("Header correction seems ok but errors > 1");
		return -1;
	}

	il2p_descramble_block(corrected, corrected_descrambled_hdr, IL2P_HEADER_SIZE);

	return (e);
}

// end il2p_header.c 


/*--------------------------------------------------------------------------------
 *
 * File:	il2p_payload.c
 *
 * Purpose:	Functions dealing with the payload.
 *
 *--------------------------------------------------------------------------------*/


 /*--------------------------------------------------------------------------------
  *
  * Function:	il2p_payload_compute
  *
  * Purpose:	Compute number and sizes of data blocks based on total size.
  *
  * Inputs:	payload_size	0 to 1023.  (IL2P_MAX_PAYLOAD_SIZE)
  *		max_fec		true for 16 parity symbols, false for automatic.
  *
  * Outputs:	*p		Payload block sizes and counts.
  *				Number of parity symbols per block.
  *
  * Returns:	Number of bytes in the encoded format.
  *		Could be 0 for no payload blocks.
  *		-1 for error (i.e. invalid unencoded size: <0 or >1023)
  *
  *--------------------------------------------------------------------------------*/

int il2p_payload_compute(il2p_payload_properties_t *p, int payload_size, int max_fec)
{
	memset(p, 0, sizeof(il2p_payload_properties_t));

	if (payload_size < 0 || payload_size > IL2P_MAX_PAYLOAD_SIZE) {
		return (-1);
	}
	if (payload_size == 0) {
		return (0);
	}

	if (max_fec) {
		p->payload_byte_count = payload_size;
		p->payload_block_count = (p->payload_byte_count + 238) / 239;
		p->small_block_size = p->payload_byte_count / p->payload_block_count;
		p->large_block_size = p->small_block_size + 1;
		p->large_block_count = p->payload_byte_count - (p->payload_block_count * p->small_block_size);
		p->small_block_count = p->payload_block_count - p->large_block_count;
		p->parity_symbols_per_block = 16;
	}
	else {
		p->payload_byte_count = payload_size;
		p->payload_block_count = (p->payload_byte_count + 246) / 247;
		p->small_block_size = p->payload_byte_count / p->payload_block_count;
		p->large_block_size = p->small_block_size + 1;
		p->large_block_count = p->payload_byte_count - (p->payload_block_count * p->small_block_size);
		p->small_block_count = p->payload_block_count - p->large_block_count;
		//p->parity_symbols_per_block = (p->small_block_size / 32) + 2;  // Looks like error in documentation

		// It would work if the number of parity symbols was based on large block size.

		if (p->small_block_size <= 61) p->parity_symbols_per_block = 2;
		else if (p->small_block_size <= 123) p->parity_symbols_per_block = 4;
		else if (p->small_block_size <= 185) p->parity_symbols_per_block = 6;
		else if (p->small_block_size <= 247) p->parity_symbols_per_block = 8;
		else {
			// Should not happen.  But just in case...
			Debugprintf("IL2P parity symbol per payload block error.  small_block_size = %d\n", p->small_block_size);
			return (-1);
		}
	}

	// Return the total size for the encoded format.

	return (p->small_block_count * (p->small_block_size + p->parity_symbols_per_block) +
		p->large_block_count * (p->large_block_size + p->parity_symbols_per_block));

} // end il2p_payload_compute



/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_encode_payload
 *
 * Purpose:	Split payload into multiple blocks such that each set
 *		of data and parity symbols fit into a 255 byte RS block.
 *
 * Inputs:	*payload	Array of bytes.
 *		payload_size	0 to 1023.  (IL2P_MAX_PAYLOAD_SIZE)
 *		max_fec		true for 16 parity symbols, false for automatic.
 *
 * Outputs:	*enc		Encoded payload for transmission.
 *				Up to IL2P_MAX_ENCODED_SIZE bytes.
 *
 * Returns:	-1 for error (i.e. invalid size)
 *		0 for no blocks.  (i.e. size zero)
 *		Number of bytes generated.  Maximum IL2P_MAX_ENCODED_SIZE.
 *
 * Note:	I interpreted the protocol spec as saying the LFSR state is retained
 *		between data blocks.  During interoperability testing, I found that
 *		was not the case.  It is reset for each data block.
 *
 *--------------------------------------------------------------------------------*/


int il2p_encode_payload(unsigned char *payload, int payload_size, int max_fec, unsigned char *enc)
{
	if (payload_size > IL2P_MAX_PAYLOAD_SIZE) return (-1);
	if (payload_size == 0) return (0);

	// Determine number of blocks and sizes.

	il2p_payload_properties_t ipp;
	int e;
	e = il2p_payload_compute(&ipp, payload_size, max_fec);
	if (e <= 0) {
		return (e);
	}

	unsigned char *pin = payload;
	unsigned char *pout = enc;
	int encoded_length = 0;
	unsigned char scram[256];
	unsigned char parity[IL2P_MAX_PARITY_SYMBOLS];

	// First the large blocks.

	for (int b = 0; b < ipp.large_block_count; b++) {

		il2p_scramble_block(pin, scram, ipp.large_block_size);
		memcpy(pout, scram, ipp.large_block_size);
		pin += ipp.large_block_size;
		pout += ipp.large_block_size;
		encoded_length += ipp.large_block_size;
		il2p_encode_rs(scram, ipp.large_block_size, ipp.parity_symbols_per_block, parity);
		memcpy(pout, parity, ipp.parity_symbols_per_block);
		pout += ipp.parity_symbols_per_block;
		encoded_length += ipp.parity_symbols_per_block;
	}

	// Then the small blocks.

	for (int b = 0; b < ipp.small_block_count; b++) {

		il2p_scramble_block(pin, scram, ipp.small_block_size);
		memcpy(pout, scram, ipp.small_block_size);
		pin += ipp.small_block_size;
		pout += ipp.small_block_size;
		encoded_length += ipp.small_block_size;
		il2p_encode_rs(scram, ipp.small_block_size, ipp.parity_symbols_per_block, parity);
		memcpy(pout, parity, ipp.parity_symbols_per_block);
		pout += ipp.parity_symbols_per_block;
		encoded_length += ipp.parity_symbols_per_block;
	}

	return (encoded_length);

} // end il2p_encode_payload


/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_decode_payload
 *
 * Purpose:	Extract original data from encoded payload.
 *
 * Inputs:	received	Array of bytes.  Size is unknown but in practice it
 *				must not exceed IL2P_MAX_ENCODED_SIZE.
 *		payload_size	0 to 1023.  (IL2P_MAX_PAYLOAD_SIZE)
 *				Expected result size based on header.
 *		max_fec		true for 16 parity symbols, false for automatic.
 *
 * Outputs:	payload_out	Recovered payload.
 *
 * In/Out:	symbols_corrected	Number of symbols corrected.
 *
 *
 * Returns:	Number of bytes extracted.  Should be same as payload_size going in.
 *		-3 for unexpected internal inconsistency.
 *		-2 for unable to recover from signal corruption.
 *		-1 for invalid size.
 *		0 for no blocks.  (i.e. size zero)
 *
 * Description:	Each block is scrambled separately but the LSFR state is carried
 *		from the first payload block to the next.
 *
 *--------------------------------------------------------------------------------*/

int il2p_decode_payload(unsigned char *received, int payload_size, int max_fec, unsigned char *payload_out, int *symbols_corrected)
{
	// Determine number of blocks and sizes.

	il2p_payload_properties_t ipp;
	int e;
	e = il2p_payload_compute(&ipp, payload_size, max_fec);
	if (e <= 0) {
		return (e);
	}

	unsigned char *pin = received;
	unsigned char *pout = payload_out;
	int decoded_length = 0;
	int failed = 0;

	// First the large blocks.

	for (int b = 0; b < ipp.large_block_count; b++) {
		unsigned char corrected_block[255];
		int e = il2p_decode_rs(pin, ipp.large_block_size, ipp.parity_symbols_per_block, corrected_block);

		// Debugprintf ("%s:%d: large block decode_rs returned status = %d\n", __FILE__, __LINE__, e);

		if (e < 0) failed = 1;
		*symbols_corrected += e;

		il2p_descramble_block(corrected_block, pout, ipp.large_block_size);

		if (il2p_get_debug() >= 2) {
	
			Debugprintf("Descrambled large payload block, %d bytes:\n", ipp.large_block_size);
			fx_hex_dump(pout, ipp.large_block_size);
		}

		pin += ipp.large_block_size + ipp.parity_symbols_per_block;
		pout += ipp.large_block_size;
		decoded_length += ipp.large_block_size;
	}

	// Then the small blocks.

	for (int b = 0; b < ipp.small_block_count; b++) {
		unsigned char corrected_block[255];
		int e = il2p_decode_rs(pin, ipp.small_block_size, ipp.parity_symbols_per_block, corrected_block);

		// Debugprintf ("%s:%d: small block decode_rs returned status = %d\n", __FILE__, __LINE__, e);

		if (e < 0) failed = 1;
		*symbols_corrected += e;

		il2p_descramble_block(corrected_block, pout, ipp.small_block_size);

		if (il2p_get_debug() >= 2) {
	
			Debugprintf("Descrambled small payload block, %d bytes:\n", ipp.small_block_size);
			fx_hex_dump(pout, ipp.small_block_size);
		}

		pin += ipp.small_block_size + ipp.parity_symbols_per_block;
		pout += ipp.small_block_size;
		decoded_length += ipp.small_block_size;
	}

	if (failed) {
		//Debugprintf ("%s:%d: failed = %0x\n", __FILE__, __LINE__, failed);
		return (-2);
	}

	if (decoded_length != payload_size) {
		Debugprintf("IL2P Internal error: decoded_length = %d, payload_size = %d\n", decoded_length, payload_size);
		return (-3);
	}

	return (decoded_length);

} // end il2p_decode_payload

// end il2p_payload.c

struct il2p_context_s *il2p_context[4][16][3];


/***********************************************************************************
 *
 * Name:        il2p_rec_bit
 *
 * Purpose:     Extract FX.25 packets from a stream of bits.
 *
 * Inputs:      chan    - Channel number.
 *
 *              subchan - This allows multiple demodulators per channel.
 *
 *              slice   - Allows multiple slicers per demodulator (subchannel).
 *
 *              dbit	- One bit from the received data stream.
 *
 * Description: This is called once for each received bit.
 *              For each valid packet, process_rec_frame() is called for further processing.
 *		It can gather multiple candidates from different parallel demodulators
 *		("subchannels") and slicers, then decide which one is the best.
 *
 ***********************************************************************************/

int centreFreq[4] = { 0, 0, 0, 0 };

void il2p_rec_bit(int chan, int subchan, int slice, int dbit)
{
	// Allocate context blocks only as needed.

	if (dbit)
		dbit = 1;

	struct il2p_context_s *F = il2p_context[chan][subchan][slice];
	if (F == NULL) {
		//assert(chan >= 0 && chan < MAX_CHANS);
		//assert(subchan >= 0 && subchan < MAX_SUBCHANS);
		//assert(slice >= 0 && slice < MAX_SLICERS);
		F = il2p_context[chan][subchan][slice] = (struct il2p_context_s *)malloc(sizeof(struct il2p_context_s));
		//assert(F != NULL);
		memset(F, 0, sizeof(struct il2p_context_s));
	}

	// Accumulate most recent 24 bits received.  Most recent is LSB.

	F->acc = ((F->acc << 1) | (dbit & 1)) & 0x00ffffff;

	// State machine to look for sync word then gather appropriate number of header and payload bytes.

	switch (F->state) {

	case IL2P_SEARCHING:		// Searching for the sync word.

		if (__builtin_popcount(F->acc ^ IL2P_SYNC_WORD) <= 1) {	// allow single bit mismatch
		  //text_color_set (DW_COLOR_INFO);
		  //Debugprintf ("IL2P header has normal polarity\n");
			F->polarity = 0;
			F->state = IL2P_HEADER;
			F->bc = 0;
			F->hc = 0;
			nPhases[chan][subchan][slice] = 0;

			// Determine Centre Freq
			
			centreFreq[chan] = GuessCentreFreq(chan);
		}
		else if (__builtin_popcount((~F->acc & 0x00ffffff) ^ IL2P_SYNC_WORD) <= 1) {
			// FIXME - this pops up occasionally with random noise.  Find better way to convey information.
			// This also happens for each slicer - to noisy.
			//Debugprintf ("IL2P header has reverse polarity\n");
			F->polarity = 1;
			F->state = IL2P_HEADER;
			F->bc = 0;
			F->hc = 0;
			centreFreq[chan] = GuessCentreFreq(chan);
			nPhases[chan][subchan][slice] = 0;
		}
			
		break;

	case IL2P_HEADER:		// Gathering the header.

		F->bc++;
		if (F->bc == 8) {	// full byte has been collected.
			F->bc = 0;
			if (!F->polarity) {
				F->shdr[F->hc++] = F->acc & 0xff;
			}
			else {
				F->shdr[F->hc++] = (~F->acc) & 0xff;
			}
			if (F->hc == IL2P_HEADER_SIZE + IL2P_HEADER_PARITY) {		// Have all of header

				//if (il2p_get_debug() >= 1)
				//{
				//	Debugprintf("IL2P header as received [%d.%d.%d]:\n", chan, subchan, slice);
				//	fx_hex_dump(F->shdr, IL2P_HEADER_SIZE + IL2P_HEADER_PARITY);
				//}

				// Fix any errors and descramble.
				F->corrected = il2p_clarify_header(F->shdr, F->uhdr);

				if (F->corrected >= 0) {	// Good header.
							// How much payload is expected?
					il2p_payload_properties_t plprop;
					int hdr_type, max_fec;
					int len = il2p_get_header_attributes(F->uhdr, &hdr_type, &max_fec);

					F->eplen = il2p_payload_compute(&plprop, len, max_fec);

					if (il2p_get_debug() >= 1)
					{	
						Debugprintf("Header type %d, max fec = %d", hdr_type, max_fec);
						Debugprintf("Need to collect %d encoded bytes for %d byte payload.", F->eplen, len);
						Debugprintf("%d small blocks of %d and %d large blocks of %d.  %d parity symbols per block",
							plprop.small_block_count, plprop.small_block_size,
							plprop.large_block_count, plprop.large_block_size, plprop.parity_symbols_per_block);
					}

					if (len > 340)
					{
						Debugprintf("Packet too big for QtSM");
						F->state = IL2P_SEARCHING;
						return;
					}
					if (F->eplen >= 1) {		// Need to gather payload.
						F->pc = 0;
						F->state = IL2P_PAYLOAD;
					}
					else if (F->eplen == 0) {	// No payload.
						F->pc = 0;
						F->state = IL2P_DECODE;
					}
					else {			// Error.

						if (il2p_get_debug() >= 1) {
							Debugprintf("IL2P header INVALID.\n");
						}

						F->state = IL2P_SEARCHING;
					}
				}  // good header after FEC.
				else {
					F->state = IL2P_SEARCHING;	// Header failed FEC check.
				}
			}  // entire header has been collected.    
		}  // full byte collected.
		break;

	case IL2P_PAYLOAD:		// Gathering the payload, if any.

		F->bc++;
		if (F->bc == 8) {	// full byte has been collected.
			F->bc = 0;
			if (!F->polarity) {
				F->spayload[F->pc++] = F->acc & 0xff;
			}
			else {
				F->spayload[F->pc++] = (~F->acc) & 0xff;
			}
			if (F->pc == F->eplen) {

				// TODO?: for symmetry it seems like we should clarify the payload before combining.

				F->state = IL2P_DECODE;
			}
		}
		break;

	case IL2P_DECODE:
		// We get here after a good header and any payload has been collected.
		// Processing is delayed by one bit but I think it makes the logic cleaner.
		// During unit testing be sure to send an extra bit to flush it out at the end.

		// in uhdr[IL2P_HEADER_SIZE];  // Header after FEC and descrambling.

		// TODO?:  for symmetry, we might decode the payload here and later build the frame.

	{
		packet_t pp = il2p_decode_header_payload(F->uhdr, F->spayload, &(F->corrected));

		if (il2p_get_debug() >= 1)
		{
			if (pp == NULL)
			{
				// Most likely too many FEC errors.
				Debugprintf("FAILED to construct frame in %s.\n", __func__);
			}
		}

		if (pp != NULL) {
			alevel_t alevel = demod_get_audio_level(chan, subchan);
			retry_t retries = F->corrected;
			int is_fx25 = 1;		// FIXME: distinguish fx.25 and IL2P.
					  // Currently this just means that a FEC mode was used.

			// TODO: Could we put last 3 arguments in packet object rather than passing around separately?

			multi_modem_process_rec_packet(chan, subchan, slice, pp, alevel, retries, is_fx25, slice, centreFreq[chan]);
		}
	}   // end block for local variables.

	if (il2p_get_debug() >= 1) {

		Debugprintf("-----");
	}

	F->state = IL2P_SEARCHING;
	break;

	} // end of switch

} // end il2p_rec_bit







// Scramble bits for il2p transmit.

// Note that there is a delay of 5 until the first bit comes out.
// So we need to need to ignore the first 5 out and stick in
// an extra 5 filler bits to flush at the end.

#define INIT_TX_LSFR 0x00f

static inline int scramble_bit(int in, int *state)
{
	int out = ((*state >> 4) ^ *state) & 1;
	*state = ((((in ^ *state) & 1) << 9) | (*state ^ ((*state & 1) << 4))) >> 1;
	return (out);
}


// Undo data scrambling for il2p receive.

#define INIT_RX_LSFR 0x1f0

static inline int descramble_bit(int in, int *state)
{
	int out = (in ^ *state) & 1;
	*state = ((*state >> 1) | ((in & 1) << 8)) ^ ((in & 1) << 3);
	return (out);
}


/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_scramble_block
 *
 * Purpose:	Scramble a block before adding RS parity.
 *
 * Inputs:	in		Array of bytes.
 *		len		Number of bytes both in and out.
 *
 * Outputs:	out		Array of bytes.
 *
 *--------------------------------------------------------------------------------*/

void il2p_scramble_block(unsigned char *in, unsigned char *out, int len)
{
	int tx_lfsr_state = INIT_TX_LSFR;

	memset(out, 0, len);

	int skipping = 1;	// Discard the first 5 out.
	int ob = 0;		// Index to output byte.
	int om = 0x80;		// Output bit mask;
	for (int ib = 0; ib < len; ib++) {
		for (int im = 0x80; im != 0; im >>= 1) {
			int s = scramble_bit((in[ib] & im) != 0, &tx_lfsr_state);
			if (ib == 0 && im == 0x04) skipping = 0;
			if (!skipping) {
				if (s) {
					out[ob] |= om;
				}
				om >>= 1;
				if (om == 0) {
					om = 0x80;
					ob++;
				}
			}
		}
	}
	// Flush it.

	// This is a relic from when I thought the state would need to
	// be passed along for the next block.
	// Preserve the LSFR state from before flushing.
	// This might be needed as the initial state for later payload blocks.
	int x = tx_lfsr_state;
	for (int n = 0; n < 5; n++) {
		int s = scramble_bit(0, &x);
		if (s) {
			out[ob] |= om;
		}
		om >>= 1;
		if (om == 0) {
			om = 0x80;
			ob++;
		}
	}

}  // end il2p_scramble_block



/*--------------------------------------------------------------------------------
 *
 * Function:	il2p_descramble_block
 *
 * Purpose:	Descramble a block after removing RS parity.
 *
 * Inputs:	in		Array of bytes.
 *		len		Number of bytes both in and out.
 *
 * Outputs:	out		Array of bytes.
 *
 *--------------------------------------------------------------------------------*/

void il2p_descramble_block(unsigned char *in, unsigned char *out, int len)
{
	int rx_lfsr_state = INIT_RX_LSFR;

	memset(out, 0, len);

	for (int b = 0; b < len; b++) {
		for (int m = 0x80; m != 0; m >>= 1) {
			int d = descramble_bit((in[b] & m) != 0, &rx_lfsr_state);
			if (d) {
				out[b] |= m;
			}
		}
	}
}

// end il2p_scramble.c




static int number_of_bits_sent[MAX_CHANS];		// Count number of bits sent by "il2p_send_frame"

static void send_bytes(int chan, unsigned char *b, int count, int polarity);
static void send_bit(int chan, int b, int polarity);



/*-------------------------------------------------------------
 *
 * Name:	il2p_send_frame
 *
 * Purpose:	Convert frames to a stream of bits in IL2P format.
 *
 * Inputs:	chan	- Audio channel number, 0 = first.
 *
 *		pp	- Pointer to packet object.
 *
 *		max_fec	- 1 to force 16 parity symbols for each payload block.
 *			  0 for automatic depending on block size.
 *
 *		polarity - 0 for normal.  1 to invert signal.
 *			   2 special case for testing - introduce some errors to test FEC.
 *
 * Outputs:	Bits are shipped out by calling tone_gen_put_bit().
 *
 * Returns:	Number of bits sent including
 *		- Preamble   (01010101...)
 *		- 3 byte Sync Word.
 *		- 15 bytes for Header.
 *		- Optional payload.
 *		The required time can be calculated by dividing this
 *		number by the transmit rate of bits/sec.
 *		-1 is returned for failure.
 *
 * Description:	Generate an IL2P encoded frame.
 *
 * Assumptions:	It is assumed that the tone_gen module has been
 *		properly initialized so that bits sent with
 *		tone_gen_put_bit() are processed correctly.
 *
 * Errors:	Return -1 for error.  Probably frame too large.
 *
 * Note:	Inconsistency here. ax25 version has just a byte array
 *		and length going in.  Here we need the full packet object.
 *
 *--------------------------------------------------------------*/

string * il2p_send_frame(int chan, packet_t pp, int max_fec, int polarity)
{
	unsigned char encoded[IL2P_MAX_PACKET_SIZE];
	string * packet = newString();
	int preamblecount;
	unsigned char preamble[1024];

	encoded[0] = (IL2P_SYNC_WORD >> 16) & 0xff;
	encoded[1] = (IL2P_SYNC_WORD >> 8) & 0xff;
	encoded[2] = (IL2P_SYNC_WORD) & 0xff;

	int elen = il2p_encode_frame(pp, max_fec, encoded + IL2P_SYNC_WORD_SIZE);

	if (elen <= 0) {
		Debugprintf("IL2P: Unable to encode frame into IL2P.\n");
		return (packet);
	}

	elen += IL2P_SYNC_WORD_SIZE;

	number_of_bits_sent[chan] = 0;

	if (il2p_get_debug() >= 1) {
		Debugprintf("IL2P frame, max_fec = %d, %d encoded bytes total", max_fec, elen);
//		fx_hex_dump(encoded, elen);
	}

	// Send bits to modulator.

	// Try using preaamble for txdelay

	// Nino now uses 00 as preamble for QPSK

	// We don't need txdelay between frames in one transmission


	if (Continuation[chan] == 0)
	{
		preamblecount = (txdelay[chan] * tx_bitrate[chan]) / 8000;		// 8 for bits, 1000 for mS

		if (preamblecount > 1024)
			preamblecount = 1024;

		if (pskStates[chan])		// PSK Modes
			memset(preamble, 01, preamblecount);
		else
			memset(preamble, IL2P_PREAMBLE, preamblecount);

		stringAdd(packet, preamble, preamblecount);
		Continuation[chan] = 1;
	}

	stringAdd(packet, encoded, elen);

	tx_fx25_size[chan] = packet->Length * 8;

	return packet;
}



// TX Code. Builds whole packet then sends a bit at a time

#define TX_SILENCE 0
#define TX_DELAY 1
#define TX_TAIL 2
#define TX_NO_DATA 3
#define TX_FRAME 4
#define TX_WAIT_BPF 5


#define TX_BIT0 0
#define TX_BIT1 1
#define FRAME_EMPTY 0
#define FRAME_FULL 1
#define FRAME_NO_FRAME 2
#define FRAME_NEW_FRAME 3
#define BYTE_EMPTY 0
#define BYTE_FULL 1

extern UCHAR tx_frame_status[5];
extern UCHAR tx_byte_status[5];
extern string * tx_data[5];
extern int tx_data_len[5];
extern UCHAR tx_bit_stream[5];
extern UCHAR tx_bit_cnt[5];
extern long tx_tail_cnt[5];
extern BOOL tx_bs_bit[5];

string * fill_il2p_data(int snd_ch, string * data)
{
	string * result;
	packet_t pp = ax25_new();
	
	// Call il2p_send_frame to build the bit stream

	pp->frame_len = data->Length - 2;					// Included CRC
	memcpy(pp->frame_data, data->Data, data->Length);

	result = il2p_send_frame(snd_ch, pp, 1, 0);

	return result;
}



void il2p_get_new_frame(int snd_ch, TStringList * frame_stream)
{
	string * myTemp;

	tx_bs_bit[snd_ch] = 0;
	tx_bit_cnt[snd_ch] = 0;
	tx_fx25_size_cnt[snd_ch] = 0;
	tx_fx25_size[snd_ch] = 1;
	tx_frame_status[snd_ch] = FRAME_NEW_FRAME;
	tx_byte_status[snd_ch] = BYTE_EMPTY;

	if (frame_stream->Count == 0)
		tx_frame_status[snd_ch] = FRAME_NO_FRAME;
	else
	{
		// We now pass control byte and ack bytes on front and pointer to socket on end if ackmode

		myTemp = Strings(frame_stream, 0);			// get message

		if ((myTemp->Data[0] & 0x0f) == 12)			// ACKMODE
		{
			// Save copy then copy data up 3 bytes

			Add(&KISS_acked[snd_ch], duplicateString(myTemp));

			mydelete(myTemp, 0, 3);
			myTemp->Length -= sizeof(void *);
		}
		else
		{
			// Just remove control 

			mydelete(myTemp, 0, 1);
		}

		AGW_AX25_frame_analiz(snd_ch, FALSE, myTemp);
		put_frame(snd_ch, myTemp, "", TRUE, FALSE);

		tx_data[snd_ch] = fill_il2p_data(snd_ch, myTemp);

		Delete(frame_stream, 0);			// This will invalidate temp
	}
}



// Original code

/*
static void send_bytes(int chan, unsigned char *b, int count, int polarity)
{
	for (int j = 0; j < count; j++) {
		unsigned int x = b[j];
		for (int k = 0; k < 8; k++) {
			send_bit(chan, (x & 0x80) != 0, polarity);
			x <<= 1;
		}
	}
}

// NRZI would be applied for AX.25 but IL2P does not use it.
// However we do have an option to invert the signal.
// The direwolf receive implementation will automatically compensate
// for either polarity but other implementations might not.

static void send_bit(int chan, int b, int polarity)
{
	tone_gen_put_bit(chan, (b ^ polarity) & 1);
	number_of_bits_sent[chan]++;
}
*/




int il2p_get_new_bit(int snd_ch, Byte bit)
{
	string *s;

	if (tx_frame_status[snd_ch] == FRAME_EMPTY)
	{
		il2p_get_new_frame(snd_ch, &all_frame_buf[snd_ch]);
		if (tx_frame_status[snd_ch] == FRAME_NEW_FRAME)
			tx_frame_status[snd_ch] = FRAME_FULL;
	}

	if (tx_frame_status[snd_ch] == FRAME_FULL)
	{
		if (tx_byte_status[snd_ch] == BYTE_EMPTY)
		{
			if (tx_data[snd_ch]->Length)
			{
				s = tx_data[snd_ch];

				tx_bit_stream[snd_ch] = s->Data[0];
				tx_frame_status[snd_ch] = FRAME_FULL;
				tx_byte_status[snd_ch] = BYTE_FULL;
				tx_bit_cnt[snd_ch] = 0;
				mydelete(tx_data[snd_ch], 0, 1);
			}
			else
				tx_frame_status[snd_ch] = FRAME_EMPTY;
		}
		if (tx_byte_status[snd_ch] == BYTE_FULL)
		{
			// il2p sends high order bit first

			bit = tx_bit_stream[snd_ch] >> 7;			// top bit to bottom

			tx_bit_stream[snd_ch] = tx_bit_stream[snd_ch] << 1;
			tx_bit_cnt[snd_ch]++;
			tx_fx25_size_cnt[snd_ch]++;
			if (tx_bit_cnt[snd_ch] >= 8)
				tx_byte_status[snd_ch] = BYTE_EMPTY;
			if (tx_fx25_size_cnt[snd_ch] == tx_fx25_size[snd_ch])
				tx_frame_status[snd_ch] = FRAME_EMPTY;
		}
	}

	if (tx_frame_status[snd_ch] == FRAME_EMPTY)
	{
		il2p_get_new_frame(snd_ch, &all_frame_buf[snd_ch]);

		switch (tx_frame_status[snd_ch])
		{
		case FRAME_NEW_FRAME:
			tx_frame_status[snd_ch] = FRAME_FULL;
			break;

		case FRAME_NO_FRAME:
			tx_tail_cnt[snd_ch] = 0;
			tx_frame_status[snd_ch] = FRAME_EMPTY;
			tx_status[snd_ch] = TX_TAIL;
			break;
		}
	}
	return bit;
}



