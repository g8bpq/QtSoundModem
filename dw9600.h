#pragma once

// Includes code from Dire Wolf Copyright (C) 2011, 2012, 2013, 2014, 2015  John Langner, WB2OSZ
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



// Dephi emulation functions

string * Strings(TStringList * Q, int Index);
void Clear(TStringList * Q);
int Count(TStringList * List);

string * newString();
string * copy(string * Source, int StartChar, int Count);
TStringList * newTStringList();

void freeString(string * Msg);

void initString(string * S);
void initTStringList(TStringList* T);

// Two delete() This is confusing!!
// Not really - one acts on String, other TStringList

void Delete(TStringList * Q, int Index);
void mydelete(string * Source, int StartChar, int Count);

void move(unsigned char * SourcePointer, unsigned char * DestinationPointer, int CopyCount);
void fmove(float * SourcePointer, float * DestinationPointer, int CopyCount);

void setlength(string * Msg, int Count);		// Set string length

string * stringAdd(string * Msg, unsigned char * Chars, int Count);		// Extend string 

void Assign(TStringList * to, TStringList * from);	// Duplicate from to to

string * duplicateString(string * in);

// This looks for a string in a stringlist. Returns inhex if found, otherwise -1

int  my_indexof(TStringList * l, string * s);

int Add(TStringList * Q, string * Entry);


#define MAX_FILTER_SIZE 480		/* 401 is needed for profile A, 300 baud & 44100. Revisit someday. */
// Size comes out to 417 for 1200 bps with 48000 sample rate
// v1.7 - Was 404.  Bump up to 480.


#define MAX_TOTAL_CHANS 16
#define MAX_ADEVS 3	
#define AX25_MAX_ADDR_LEN 12

#include <stdint.h>	// for uint64_t



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
		dw_printf("ax25_get_num_control, %02x is I frame, returns %d\n", c, (this_p->modulo == 128) ? 2 : 1);
#endif
		return ((this_p->modulo == 128) ? 2 : 1);
	}

	if ((c & 0x03) == 1) {			/* S   xxxx xx01 */
#if DEBUGX
		dw_printf("ax25_get_num_control, %02x is S frame, returns %d\n", c, (this_p->modulo == 128) ? 2 : 1);
#endif
		return ((this_p->modulo == 128) ? 2 : 1);
	}

#if DEBUGX
	dw_printf("ax25_get_num_control, %02x is U frame, always returns 1.\n", c);
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
		dw_printf("ax25_get_num_pid, %02x is I or UI frame, pid = %02x, returns %d\n", c, pid, (pid == AX25_PID_ESCAPE_CHARACTER) ? 2 : 1);
#endif
		if (pid == AX25_PID_ESCAPE_CHARACTER) {
			return (2);			/* pid 1111 1111 means another follows. */
		}
		return (1);
	}
#if DEBUGX
	dw_printf("ax25_get_num_pid, %02x is neither I nor UI frame, returns 0\n", c);
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
	dw_printf("ax25_get_info_offset, returns %d\n", offset);
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

typedef struct alevel_s {

	int rec;
	int mark;
	int space;
	//float ms_ratio;	// TODO: take out after temporary investigation.
} alevel_t;


#ifndef AXTEST
// TODO: remove this?
#define AX25MEMDEBUG 0
#endif


#if AX25MEMDEBUG	// to investigate a memory leak problem


extern void ax25memdebug_set(void);
extern int ax25memdebug_get(void);
extern int ax25memdebug_seq(packet_t this_p);


extern packet_t ax25_from_text_debug(char *monitor, int strict, char *src_file, int src_line);
#define ax25_from_text(m,s) ax25_from_text_debug(m,s,__FILE__,__LINE__)

extern packet_t ax25_from_frame_debug(unsigned char *data, int len, alevel_t alevel, char *src_file, int src_line);
#define ax25_from_frame(d,l,a) ax25_from_frame_debug(d,l,a,__FILE__,__LINE__);

extern packet_t ax25_dup_debug(packet_t copy_from, char *src_file, int src_line);
#define ax25_dup(p) ax25_dup_debug(p,__FILE__,__LINE__);

extern void ax25_delete_debug(packet_t pp, char *src_file, int src_line);
#define ax25_delete(p) ax25_delete_debug(p,__FILE__,__LINE__);

#else

extern packet_t ax25_from_text(char *monitor, int strict);

extern packet_t ax25_from_frame(unsigned char *data, int len, alevel_t alevel);

extern packet_t ax25_dup(packet_t copy_from);

extern void ax25_delete(packet_t pp);

#endif




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



int demod_init(struct audio_s *pa);
int demod_get_sample(int a);
void demod_process_sample(int chan, int subchan, int sam);
void demod_print_agc(int chan, int subchan);
alevel_t demod_get_audio_level(int chan, int subchan);
void hdlc_rec_bit(int chan, int subchan, int slice, int raw, int is_scrambled, int descram_state);

/* Provided elsewhere to process a complete frame. */

//void process_rec_frame (int chan, unsigned char *fbuf, int flen, int level);


/* Is HLDC decoder is currently gathering bits into a frame? */
/* Similar to, but not exactly the same as, data carrier detect. */
/* We use this to influence the PLL inertia. */

int hdlc_rec_gathering(int chan, int subchan, int slice);

/* Transmit needs to know when someone else is transmitting. */

void dcd_change(int chan, int subchan, int slice, int state);

int hdlc_rec_data_detect_any(int chan);



/* direwolf.h - Common stuff used many places. */

// TODO:   include this file first before anything else in each .c file.


#ifdef NDEBUG
#undef NDEBUG		// Because it would disable assert().
#endif

#define __restrict__

#define dw_printf Debugprintf

#define M_PI       3.1415926f

#define no_init_all deprecated

#ifndef DIREWOLF_H
#define DIREWOLF_H 1

/*
 * Support Windows XP and later.
 *
 * We need this before "#include <ws2tcpip.h>".
 *
 * Don't know what other impact it might have on others.
 */

#ifdef WIN32
#define __WIN32__ 1
#endif

#if __WIN32__

#ifdef _WIN32_WINNT
#error	Include "direwolf.h" before any windows system files.
#endif
#ifdef WINVER
#error	Include "direwolf.h" before any windows system files.
#endif

#define _WIN32_WINNT 0x0501     /* Minimum OS version is XP. */
#define WINVER       0x0501     /* Minimum OS version is XP. */

#include <winsock2.h>
#include <windows.h>

#endif

 /*
  * Maximum number of audio devices.
  * Three is probably adequate for standard version.
  * Larger reasonable numbers should also be fine.
  *
  * For example, if you wanted to use 4 audio devices at once, change this to 4.
  */

#define MAX_ADEVS 3			


  /*
   * Maximum number of radio channels.
   * Note that there could be gaps.
   * Suppose audio device 0 was in mono mode and audio device 1 was stereo.
   * The channels available would be:
   *
   *	ADevice 0:	channel 0
   *	ADevice 1:	left = 2, right = 3
   *
   * TODO1.2:  Look for any places that have
   *		for (ch=0; ch<MAX_CHANS; ch++) ...
   * and make sure they handle undefined channels correctly.
   */

#define MAX_RADIO_CHANS 4

#define MAX_CHANS 4	// TODO: Replace all former  with latter to avoid confusion with following.

#define MAX_TOTAL_CHANS 16		// v1.7 allows additional virtual channels which are connected
   // to something other than radio modems.
   // Total maximum channels is based on the 4 bit KISS field.
   // Someone with very unusual requirements could increase this and
   // use only the AGW network protocol.

/*
 * Maximum number of rigs.
 */

#ifdef USE_HAMLIB
#define MAX_RIGS MAX_CHANS
#endif

 /*
  * Get audio device number for given channel.
  * and first channel for given device.
  */

#define ACHAN2ADEV(n) ((n)>>1)
#define ADEVFIRSTCHAN(n) ((n) * 2)

  /*
   * Maximum number of modems per channel.
   * I called them "subchannels" (in the code) because
   * it is short and unambiguous.
   * Nothing magic about the number.  Could be larger
   * but CPU demands might be overwhelming.
   */

#define MAX_SUBCHANS 9

   /*
	* Each one of these can have multiple slicers, at
	* different levels, to compensate for different
	* amplitudes of the AFSK tones.
	* Initially used same number as subchannels but
	* we could probably trim this down a little
	* without impacting performance.
	*/

#define MAX_SLICERS 9


#if __WIN32__
#define SLEEP_SEC(n) Sleep((n)*1000)
#define SLEEP_MS(n) Sleep(n)
#else
#define SLEEP_SEC(n) sleep(n)
#define SLEEP_MS(n) usleep((n)*1000)
#endif

#if __WIN32__

#define PTW32_STATIC_LIB
	//#include "pthreads/pthread.h"

	// This enables definitions of localtime_r and gmtime_r in system time.h.
	//#define _POSIX_THREAD_SAFE_FUNCTIONS 1
#define _POSIX_C_SOURCE 1

#else
#include <pthread.h>
#endif


#ifdef __APPLE__

	// https://groups.yahoo.com/neo/groups/direwolf_packet/conversations/messages/2072

	// The original suggestion was to add this to only ptt.c.
	// I thought it would make sense to put it here, so it will apply to all files,
	// consistently, rather than only one file ptt.c.

	// The placement of this is critical.  Putting it earlier was a problem.
	// https://github.com/wb2osz/direwolf/issues/113

	// It needs to be after the include pthread.h because
	// pthread.h pulls in <sys/cdefs.h>, which redefines __DARWIN_C_LEVEL back to ansi,
	// which breaks things.
	// Maybe it should just go in ptt.c as originally suggested.

	// #define __DARWIN_C_LEVEL  __DARWIN_C_FULL

	// There is a more involved patch here:
	//  https://groups.yahoo.com/neo/groups/direwolf_packet/conversations/messages/2458

#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif

// Defining _DARWIN_C_SOURCE ensures that the definition for the cfmakeraw function (or similar)
// are pulled in through the include file <sys/termios.h>.

#ifdef __DARWIN_C_LEVEL
#undef __DARWIN_C_LEVEL
#endif

#define __DARWIN_C_LEVEL  __DARWIN_C_FULL

#endif




#define DW_METERS_TO_FEET(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 3.2808399)
#define DW_FEET_TO_METERS(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.3048)
#define DW_KM_TO_MILES(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.621371192)
#define DW_MILES_TO_KM(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 1.609344)

#define DW_KNOTS_TO_MPH(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 1.15077945)
#define DW_KNOTS_TO_METERS_PER_SEC(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.51444444444)
#define DW_MPH_TO_KNOTS(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.868976)
#define DW_MPH_TO_METERS_PER_SEC(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.44704)

#define DW_MBAR_TO_INHG(x) ((x) == G_UNKNOWN ? G_UNKNOWN : (x) * 0.0295333727)




#if __WIN32__

typedef CRITICAL_SECTION dw_mutex_t;

#define dw_mutex_init(x) \
	InitializeCriticalSection (x)

/* This one waits for lock. */

#define dw_mutex_lock(x) \
	EnterCriticalSection (x) 

/* Returns non-zero if lock was obtained. */

#define dw_mutex_try_lock(x) \
	TryEnterCriticalSection (x)

#define dw_mutex_unlock(x) \
	LeaveCriticalSection (x)


#else

typedef pthread_mutex_t dw_mutex_t;

#define dw_mutex_init(x) pthread_mutex_init (x, NULL)

/* this one will wait. */

#define dw_mutex_lock(x) \
	{	\
	  int err; \
	  err = pthread_mutex_lock (x); \
	  if (err != 0) { \
	    text_color_set(DW_COLOR_ERROR); \
	    dw_printf ("INTERNAL ERROR %s %d pthread_mutex_lock returned %d", __FILE__, __LINE__, err); \
	    exit (1); \
	  } \
	}

/* This one returns true if lock successful, false if not. */
/* pthread_mutex_trylock returns 0 for success. */

#define dw_mutex_try_lock(x) \
	({	\
	  int err; \
	  err = pthread_mutex_trylock (x); \
	  if (err != 0 && err != EBUSY) { \
	    text_color_set(DW_COLOR_ERROR); \
	    dw_printf ("INTERNAL ERROR %s %d pthread_mutex_trylock returned %d", __FILE__, __LINE__, err); \
	    exit (1); \
	  } ; \
	  ! err; \
	})

#define dw_mutex_unlock(x) \
	{	\
	  int err; \
	  err = pthread_mutex_unlock (x); \
	  if (err != 0) { \
	    text_color_set(DW_COLOR_ERROR); \
	    dw_printf ("INTERNAL ERROR %s %d pthread_mutex_unlock returned %d", __FILE__, __LINE__, err); \
	    exit (1); \
	  } \
	}

#endif



// Formerly used write/read on Linux, for some forgotten reason,
// but always using send/recv makes more sense.
// Need option to prevent a SIGPIPE signal on Linux.  (added for 1.5 beta 2)

#if __WIN32__ || __APPLE__
#define SOCK_SEND(s,data,size) send(s,data,size,0)
#else
#define SOCK_SEND(s,data,size) send(s,data,size, MSG_NOSIGNAL)
#endif
#define SOCK_RECV(s,data,size) recv(s,data,size,0)


/* Platform differences for string functions. */



#if __WIN32__
char *strsep(char **stringp, const char *delim);
char *strtok_r(char *str, const char *delim, char **saveptr);
#endif

// Don't recall why I added this for everyone rather than only for Windows.
char *strcasestr(const char *S, const char *FIND);

#endif   /* ifndef DIREWOLF_H */



/*------------------------------------------------------------------
 *
 * Module:      audio.h
 *
 * Purpose:   	Interface to audio device commonly called a "sound card"
 *		for historical reasons.
 *
 *---------------------------------------------------------------*/



 /*
  * PTT control.
  */

enum ptt_method_e {
	PTT_METHOD_NONE,	/* VOX or no transmit. */
	PTT_METHOD_SERIAL,	/* Serial port RTS or DTR. */
	PTT_METHOD_GPIO,	/* General purpose I/O, Linux only. */
	PTT_METHOD_LPT,	    	/* Parallel printer port, Linux only. */
	PTT_METHOD_HAMLIB, 	/* HAMLib, Linux only. */
	PTT_METHOD_CM108
};	/* GPIO pin of CM108/CM119/etc.  Linux only. */

typedef enum ptt_method_e ptt_method_t;

enum ptt_line_e { PTT_LINE_NONE = 0, PTT_LINE_RTS = 1, PTT_LINE_DTR = 2 };	  //  Important: 0 for neither.	
typedef enum ptt_line_e ptt_line_t;

enum audio_in_type_e {
	AUDIO_IN_TYPE_SOUNDCARD,
	AUDIO_IN_TYPE_SDR_UDP,
	AUDIO_IN_TYPE_STDIN
};

/* For option to try fixing frames with bad CRC. */

typedef enum retry_e {
	RETRY_NONE = 0,
	RETRY_INVERT_SINGLE = 1,
	RETRY_INVERT_DOUBLE = 2,
	RETRY_INVERT_TRIPLE = 3,
	RETRY_INVERT_TWO_SEP = 4,
	RETRY_MAX = 5
}  retry_t;

// Type of communication medium associated with the channel.

enum medium_e {
	MEDIUM_NONE = 0,	// Channel is not valid for use.
	MEDIUM_RADIO,		// Internal modem for radio.
	MEDIUM_IGATE,		// Access IGate as ordinary channel.
	MEDIUM_NETTNC
};	// Remote network TNC.  (possible future)


typedef enum sanity_e { SANITY_APRS, SANITY_AX25, SANITY_NONE } sanity_t;


struct audio_s {

	/* Previously we could handle only a single audio device. */
	/* In version 1.2, we generalize this to handle multiple devices. */
	/* This means we can now have more than 2 radio channels. */

	struct adev_param_s {

		/* Properties of the sound device. */

		int defined;		/* Was device defined? */
					/* First one defaults to yes. */

		char adevice_in[80];	/* Name of the audio input device (or file?). */
					/* TODO: Can be "-" to read from stdin. */

		char adevice_out[80];	/* Name of the audio output device (or file?). */

		int num_channels;		/* Should be 1 for mono or 2 for stereo. */
		int samples_per_sec;	/* Audio sampling rate.  Typically 11025, 22050, or 44100. */
		int bits_per_sample;	/* 8 (unsigned char) or 16 (signed short). */

	} adev[MAX_ADEVS];


	/* Common to all channels. */

	char tts_script[80];		/* Script for text to speech. */

	int statistics_interval;	/* Number of seconds between the audio */
					/* statistics reports.  This is set by */
					/* the "-a" option.  0 to disable feature. */

	int xmit_error_rate;		/* For testing purposes, we can generate frames with an invalid CRC */
					/* to simulate corruption while going over the air. */
					/* This is the probability, in per cent, of randomly corrupting it. */
					/* Normally this is 0.  25 would mean corrupt it 25% of the time. */

	int recv_error_rate;		/* Similar but the % probability of dropping a received frame. */

	float recv_ber;			/* Receive Bit Error Rate (BER). */
					/* Probability of inverting a bit coming out of the modem. */

	//int fx25_xmit_enable;		/* Enable transmission of FX.25.  */
					/* See fx25_init.c for explanation of values. */
					/* Initially this applies to all channels. */
					/* This should probably be per channel. One step at a time. */
					/* v1.7 - replaced by layer2_xmit==LAYER2_FX25 */

	int fx25_auto_enable;		/* Turn on FX.25 for current connected mode session */
					/* under poor conditions. */
					/* Set to 0 to disable feature. */
					/* I put it here, rather than with the rest of the link layer */
					/* parameters because it is really a part of the HDLC layer */
					/* and is part of the KISS TNC functionality rather than our data link layer. */
					/* Future: not used yet. */


	char timestamp_format[40];	/* -T option */
					/* Precede received & transmitted frames with timestamp. */
					/* Command line option uses "strftime" format string. */



	/* originally a "channel" was always connected to an internal modem. */
	/* In version 1.6, this is generalized so that a channel (as seen by client application) */
	/* can be connected to something else.  Initially, this will allow application */
	/* access to the IGate.  Later we might have network TNCs or other internal functions. */

	// Properties for all channels.

	enum medium_e chan_medium[MAX_TOTAL_CHANS];
	// MEDIUM_NONE for invalid.
	// MEDIUM_RADIO for internal modem.  (only possibility earlier)
	// MEDIUM_IGATE allows application access to IGate.
	// MEDIUM_NETTNC for external TNC via TCP.

	int igate_vchannel;		/* Virtual channel mapped to APRS-IS. */
					/* -1 for none. */
					/* Redundant but it makes things quicker and simpler */
					/* than always searching thru above. */

	/* Properties for each radio channel, common to receive and transmit. */
	/* Can be different for each radio channel. */

	struct achan_param_s {

		// What else should be moved out of structure and enlarged when NETTNC is implemented.  ???
		char mycall[AX25_MAX_ADDR_LEN];      /* Call associated with this radio channel. */
									/* Could all be the same or different. */


		enum modem_t { MODEM_AFSK, MODEM_BASEBAND, MODEM_SCRAMBLE, MODEM_QPSK, MODEM_8PSK, MODEM_OFF, MODEM_16_QAM, MODEM_64_QAM, MODEM_AIS, MODEM_EAS } modem_type;

		/* Usual AFSK. */
		/* Baseband signal. Not used yet. */
		/* Scrambled http://www.amsat.org/amsat/articles/g3ruh/109/fig03.gif */
		/* Might try MFJ-2400 / CCITT v.26 / Bell 201 someday. */
		/* No modem.  Might want this for DTMF only channel. */

		enum layer2_t { LAYER2_AX25 = 0, LAYER2_FX25, LAYER2_IL2P } layer2_xmit;

		// IL2P - New for version 1.7.
		// New layer 2 with FEC.  Much less overhead than FX.25 but no longer backward compatible.
		// Only applies to transmit.
		// Listening for FEC sync word should add negligible overhead so
		// we leave reception enabled all the time as we do with FX.25.
		// TODO:  FX.25 should probably be put here rather than global for all channels.

		int fx25_strength;		// Strength of FX.25 FEC.
					// 16, 23, 64 for specific number of parity symbols.
					// 1 for automatic selection based on frame size.

		int il2p_max_fec;		// 1 for max FEC length, 0 for automatic based on size.

		int il2p_invert_polarity;	// 1 means invert on transmit.  Receive handles either automatically.

		enum v26_e { V26_UNSPECIFIED = 0, V26_A, V26_B } v26_alternative;

		// Original implementation used alternative A for 2400 bbps PSK.
		// Years later, we discover that MFJ-2400 used alternative B.
		// It's likely the others did too.  it also works a little better.
		// Default to MFJ compatible and print warning if user did not
		// pick one explicitly.

#define V26_DEFAULT V26_B

		enum dtmf_decode_t { DTMF_DECODE_OFF, DTMF_DECODE_ON } dtmf_decode;

		/* Originally the DTMF ("Touch Tone") decoder was always */
		/* enabled because it took a negligible amount of CPU. */
		/* There were complaints about the false positives when */
		/* hearing other modulation schemes on HF SSB so now it */
		/* is enabled only when needed. */

		/* "On" will send special "t" packet to attached applications */
		/* and process as APRStt.  Someday we might want to separate */
		/* these but for now, we have a single off/on. */

		int decimate;		/* Reduce AFSK sample rate by this factor to */
					/* decrease computational requirements. */

		int upsample;		/* Upsample by this factor for G3RUH. */

		int mark_freq;		/* Two tones for AFSK modulation, in Hz. */
		int space_freq;		/* Standard tones are 1200 and 2200 for 1200 baud. */

		int baud;			/* Data bits per second. */
					/* Standard rates are 1200 for VHF and 300 for HF. */
					/* This should really be called bits per second. */

	/* Next 3 come from config file or command line. */

		char profiles[16];		/* zero or more of ABC etc, optional + */

		int num_freq;		/* Number of different frequency pairs for decoders. */

		int offset;			/* Spacing between filter frequencies. */

		int num_slicers;		/* Number of different threshold points to decide */
					/* between mark or space. */

	/* This is derived from above by demod_init. */

		int num_subchan;		/* Total number of modems for each channel. */


	/* These are for dealing with imperfect frames. */

		enum retry_e fix_bits;	/* Level of effort to recover from */
					/* a bad FCS on the frame. */
					/* 0 = no effort */
					/* 1 = try fixing a single bit */
					/* 2... = more techniques... */

		enum sanity_e sanity_test;	/* Sanity test to apply when finding a good */
					/* CRC after making a change. */
					/* Must look like APRS, AX.25, or anything. */

		int passall;		/* Allow thru even with bad CRC. */



	/* Additional properties for transmit. */

	/* Originally we had control outputs only for PTT. */
	/* In version 1.2, we generalize this to allow others such as DCD. */
	/* In version 1.4 we add CON for connected to another station. */
	/* Index following structure by one of these: */


#define OCTYPE_PTT 0
#define OCTYPE_DCD 1
#define OCTYPE_CON 2

#define NUM_OCTYPES 3		/* number of values above.   i.e. last value +1. */

		struct {

			ptt_method_t ptt_method; /* none, serial port, GPIO, LPT, HAMLIB, CM108. */

			char ptt_device[128];	/* Serial device name for PTT.  e.g. COM1 or /dev/ttyS0 */
					/* Also used for HAMLIB.  Could be host:port when model is 1. */
					/* For years, 20 characters was plenty then we start getting extreme names like this: */
					/* /dev/serial/by-id/usb-FTDI_Navigator__CAT___2nd_PTT__00000000-if00-port0 */
					/* /dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0 */
					/* Issue 104, changed to 100 bytes in version 1.5. */

					/* This same field is also used for CM108/CM119 GPIO PTT which will */
					/* have a name like /dev/hidraw1 for Linux or */
					/* \\?\hid#vid_0d8c&pid_0008&mi_03#8&39d3555&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030} */
					/* for Windows.  Largest observed was 95 but add some extra to be safe. */

			ptt_line_t ptt_line;	/* Control line when using serial port. PTT_LINE_RTS, PTT_LINE_DTR. */
			ptt_line_t ptt_line2;	/* Optional second one:  PTT_LINE_NONE when not used. */

			int out_gpio_num;	/* GPIO number.  Originally this was only for PTT. */
					/* It is now more general. */
					/* octrl array is indexed by PTT, DCD, or CONnected indicator. */
					/* For CM108/CM119, this should be in range of 1-8. */

#define MAX_GPIO_NAME_LEN 20	// 12 would cover any case I've seen so this should be safe

			char out_gpio_name[MAX_GPIO_NAME_LEN];
			/* originally, gpio number NN was assumed to simply */
			/* have the name gpioNN but this turned out not to be */
			/* the case for CubieBoard where it was longer. */
			/* This is filled in by ptt_init so we don't have to */
			/* recalculate it each time we access it. */

			/* This could probably be collapsed into ptt_device instead of being separate. */

			int ptt_lpt_bit;	/* Bit number for parallel printer port.  */
					/* Bit 0 = pin 2, ..., bit 7 = pin 9. */

			int ptt_invert;		/* Invert the output. */
			int ptt_invert2;	/* Invert the secondary output. */

#ifdef USE_HAMLIB

			int ptt_model;		/* HAMLIB model.  -1 for AUTO.  2 for rigctld.  Others are radio model. */
			int ptt_rate;		/* Serial port speed when using hamlib CAT control for PTT. */
					/* If zero, hamlib will come up with a default for pariticular rig. */
#endif

		} octrl[NUM_OCTYPES];


		/* Each channel can also have associated input lines. */
		/* So far, we just have one for transmit inhibit. */

#define ICTYPE_TXINH 0

#define NUM_ICTYPES 1		/* number of values above. i.e. last value +1. */

		struct {
			ptt_method_t method;	/* none, serial port, GPIO, LPT. */

			int in_gpio_num;	/* GPIO number */

			char in_gpio_name[MAX_GPIO_NAME_LEN];
			/* originally, gpio number NN was assumed to simply */
			/* have the name gpioNN but this turned out not to be */
			/* the case for CubieBoard where it was longer. */
			/* This is filled in by ptt_init so we don't have to */
			/* recalculate it each time we access it. */

			int invert;		/* 1 = active low */
		} ictrl[NUM_ICTYPES];

		/* Transmit timing. */

		int dwait;			/* First wait extra time for receiver squelch. */
					/* Default 0 units of 10 mS each . */

		int slottime;		/* Slot time in 10 mS units for persistence algorithm. */
					/* Typical value is 10 meaning 100 milliseconds. */

		int persist;		/* Sets probability for transmitting after each */
					/* slot time delay.  Transmit if a random number */
					/* in range of 0 - 255 <= persist value.  */
					/* Otherwise wait another slot time and try again. */
					/* Default value is 63 for 25% probability. */

		int txdelay;		/* After turning on the transmitter, */
					/* send "flags" for txdelay * 10 mS. */
					/* Default value is 30 meaning 300 milliseconds. */

		int txtail;			/* Amount of time to keep transmitting after we */
					/* are done sending the data.  This is to avoid */
					/* dropping PTT too soon and chopping off the end */
					/* of the frame.  Again 10 mS units. */
					/* At this point, I'm thinking of 10 (= 100 mS) as the default */
					/* because we're not quite sure when the soundcard audio stops. */

		int fulldup;		/* Full Duplex. */

	} achan[MAX_CHANS];

#ifdef USE_HAMLIB
	int rigs;               /* Total number of configured rigs */
	RIG *rig[MAX_RIGS];     /* HAMLib rig instances */
#endif

};


#if __WIN32__
#define DEFAULT_ADEVICE	""		/* Windows: Empty string = default audio device. */
#elif __APPLE__
#define DEFAULT_ADEVICE	""		/* Mac OSX: Empty string = default audio device. */
#elif USE_ALSA
#define DEFAULT_ADEVICE	"default"	/* Use default device for ALSA. */
#elif USE_SNDIO
#define DEFAULT_ADEVICE	"default"	/* Use default device for sndio. */
#else
#define DEFAULT_ADEVICE	"/dev/dsp"	/* First audio device for OSS.  (FreeBSD) */
#endif					



/*
 * UDP audio receiving port.  Couldn't find any standard or usage precedent.
 * Got the number from this example:   http://gqrx.dk/doc/streaming-audio-over-udp
 * Any better suggestions?
 */

#define DEFAULT_UDP_AUDIO_PORT 7355


 // Maximum size of the UDP buffer (for allowing IP routing, udp packets are often limited to 1472 bytes)

#define SDR_UDP_BUF_MAXLEN 2000



#define DEFAULT_NUM_CHANNELS 	1
#define DEFAULT_SAMPLES_PER_SEC	44100	/* Very early observations.  Might no longer be valid. */
					/* 22050 works a lot better than 11025. */
					/* 44100 works a little better than 22050. */
					/* If you have a reasonable machine, use the highest rate. */
#define MIN_SAMPLES_PER_SEC	8000
//#define MAX_SAMPLES_PER_SEC	48000	/* Originally 44100.  Later increased because */
					/* Software Defined Radio often uses 48000. */

#define MAX_SAMPLES_PER_SEC	192000	/* The cheap USB-audio adapters (e.g. CM108) can handle 44100 and 48000. */
					/* The "soundcard" in my desktop PC can do 96kHz or even 192kHz. */
					/* We will probably need to increase the sample rate to go much above 9600 baud. */

#define DEFAULT_BITS_PER_SAMPLE	16

#define DEFAULT_FIX_BITS RETRY_INVERT_SINGLE

/*
 * Standard for AFSK on VHF FM.
 * Reversing mark and space makes no difference because
 * NRZI encoding only cares about change or lack of change
 * between the two tones.
 *
 * HF SSB uses 300 baud and 200 Hz shift.
 * 1600 & 1800 Hz is a popular tone pair, sometimes
 * called the KAM tones.
 */

#define DEFAULT_MARK_FREQ	1200	
#define DEFAULT_SPACE_FREQ	2200
#define DEFAULT_BAUD		1200

 /* Used for sanity checking in config file and command line options. */
 /* 9600 baud is known to work.  */
 /* TODO: Is 19200 possible with a soundcard at 44100 samples/sec or do we need a higher sample rate? */

#define MIN_BAUD		100
//#define MAX_BAUD		10000
#define MAX_BAUD		40000		// Anyone want to try 38.4 k baud?

/*
 * Typical transmit timings for VHF.
 */

#define DEFAULT_DWAIT		0
#define DEFAULT_SLOTTIME	10
#define DEFAULT_PERSIST		63
#define DEFAULT_TXDELAY		30
#define DEFAULT_TXTAIL		10	
#define DEFAULT_FULLDUP		0

 /*
  * Note that we have two versions of these in audio.c and audio_win.c.
  * Use one or the other depending on the platform.
  */

int audio_open(struct audio_s *pa);

int audio_get(int a);		/* a = audio device, 0 for first */

int audio_put(int a, int c);

int audio_flush(int a);

void audio_wait(int a);

int audio_close(void);



/* end audio.h */



void multi_modem_init(struct audio_s *pmodem);

void multi_modem_process_sample(int c, int audio_sample);

int multi_modem_get_dc_average(int chan);

// Deprecated.  Replace with ...packet


void multi_modem_process_rec_packet(int chan, int subchan, int slice, packet_t pp, alevel_t alevel, retry_t retries, int is_fx25);


void fsk_gen_filter(int samples_per_sec,
	int baud,
	int mark_freq, int space_freq,
	char profile,
	struct demodulator_state_s *D);



/* fsk_demod_state.h */



/*
 * Demodulator state.
 * The name of the file is from we only had FSK.  Now we have other techniques.
 * Different copy is required for each channel & subchannel being processed concurrently.
 */

 // TODO1.2:  change prefix from BP_ to DSP_

typedef enum bp_window_e {
	BP_WINDOW_TRUNCATED,
	BP_WINDOW_COSINE,
	BP_WINDOW_HAMMING,
	BP_WINDOW_BLACKMAN,
	BP_WINDOW_FLATTOP
} bp_window_t;

// Experimental low pass filter to detect DC bias or low frequency changes.
// IIR behaves like an analog R-C filter.
// Intuitively, it seems like FIR would be better because it is based on a finite history.
// However, it would require MANY taps and a LOT of computation for a low frequency.
// We can use a little trick here to keep a running average.
// This would be equivalent to convolving with an array of all 1 values.
// That would eliminate the need to multiply.
// We can also eliminate the need to add them all up each time by keeping a running total.
// Add a sample to the total when putting it in our array of recent samples.
// Subtract it from the total when it gets pushed off the end.
// We can also eliminate the need to shift them all down by using a circular buffer.

#define CIC_LEN_MAX 4000

typedef struct cic_s {
	int len;		// Number of elements used.
				// Might want to dynamically allocate.
	short in[CIC_LEN_MAX];	// Samples coming in.
	int sum;		// Running sum.
	int inext;		// Next position to fill.
} cic_t;


#define MAX_FILTER_SIZE 480		/* 401 is needed for profile A, 300 baud & 44100. Revisit someday. */
// Size comes out to 417 for 1200 bps with 48000 sample rate
// v1.7 - Was 404.  Bump up to 480.

struct demodulator_state_s
{
	/*
	 * These are set once during initialization.
	 */
	enum modem_t modem_type;		// MODEM_AFSK, MODEM_8PSK, etc.

//	enum v26_e v26_alt;			// Which alternative when V.26.

	char profile;			// 'A', 'B', etc.	Upper case.
					// Only needed to see if we are using 'F' to take fast path.

#define TICKS_PER_PLL_CYCLE ( 256.0 * 256.0 * 256.0 * 256.0 )

	int pll_step_per_sample;	// PLL is advanced by this much each audio sample.
					// Data is sampled when it overflows.


/*
 * Window type for the various filters.
 */

	bp_window_t lp_window;

	/*
	 * Alternate Low pass filters.
	 * First is arbitrary number for quick IIR.
	 * Second is frequency as ratio to baud rate for FIR.
	 */
	int lpf_use_fir;		/* 0 for IIR, 1 for FIR. */

	float lpf_iir;			/* Only if using IIR. */

	float lpf_baud;			/* Cutoff frequency as fraction of baud. */
					/* Intuitively we'd expect this to be somewhere */
					/* in the range of 0.5 to 1. */
					/* In practice, it turned out a little larger */
					/* for profiles B, C, D. */

	float lp_filter_width_sym;  	/* Length in number of symbol times. */

#define lp_filter_len_bits lp_filter_width_sym	// FIXME: temp hack

	int lp_filter_taps;		/* Size of Low Pass filter, in audio samples. */

#define lp_filter_size lp_filter_taps		// FIXME: temp hack


/*
 * Automatic gain control.  Fast attack and slow decay factors.
 */
	float agc_fast_attack;
	float agc_slow_decay;

	/*
	 * Use a longer term view for reporting signal levels.
	 */
	float quick_attack;
	float sluggish_decay;

	/*
	 * Hysteresis before final demodulator 0 / 1 decision.
	 */
	float hysteresis;
	int num_slicers;		/* >1 for multiple slicers. */

/*
 * Phase Locked Loop (PLL) inertia.
 * Larger number means less influence by signal transitions.
 * It is more resistant to change when locked on to a signal.
 */
	float pll_locked_inertia;
	float pll_searching_inertia;


	/*
	 * Optional band pass pre-filter before mark/space detector.
	 */
	int use_prefilter;	/* True to enable it. */

	float prefilter_baud;	/* Cutoff frequencies, as fraction of */
				/* baud rate, beyond tones used.  */
				/* Example, if we used 1600/1800 tones at */
				/* 300 baud, and this was 0.5, the cutoff */
				/* frequencies would be: */
				/* lower = min(1600,1800) - 0.5 * 300 = 1450 */
				/* upper = max(1600,1800) + 0.5 * 300 = 1950 */

	float pre_filter_len_sym;  	// Length in number of symbol times.
#define pre_filter_len_bits pre_filter_len_sym 		// temp until all references changed.

	bp_window_t pre_window;		// Window type for filter shaping.

	int pre_filter_taps;		// Calculated number of filter taps.
#define pre_filter_size pre_filter_taps		// temp until all references changed.

	float pre_filter[MAX_FILTER_SIZE];

	float raw_cb[MAX_FILTER_SIZE];	// audio in,  need better name.

/*
 * The rest are continuously updated.
 */

	unsigned int lo_phase;	/* Local oscillator for PSK. */


/*
 * Use half of the AGC code to get a measure of input audio amplitude.
 * These use "quick" attack and "sluggish" decay while the
 * AGC uses "fast" attack and "slow" decay.
 */

	float alevel_rec_peak;
	float alevel_rec_valley;
	float alevel_mark_peak;
	float alevel_space_peak;

	/*
	 * Outputs from the mark and space amplitude detection,
	 * used as inputs to the FIR lowpass filters.
	 * Kernel for the lowpass filters.
	 */

	float lp_filter[MAX_FILTER_SIZE];

	float m_peak, s_peak;
	float m_valley, s_valley;
	float m_amp_prev, s_amp_prev;


	/*
	 * For the PLL and data bit timing.
	 * starting in version 1.2 we can have multiple slicers for one demodulator.
	 * Each slicer has its own PLL and HDLC decoder.
	 */

	 /*
	  * Version 1.3: Clean up subchan vs. slicer.
	  *
	  * Originally some number of CHANNELS (originally 2, later 6)
	  * which can have multiple parallel demodulators called SUB-CHANNELS.
	  * This was originally for staggered frequencies for HF SSB.
	  * It can also be used for multiple demodulators with the same
	  * frequency but other differing parameters.
	  * Each subchannel has its own demodulator and HDLC decoder.
	  *
	  * In version 1.2 we added multiple SLICERS.
	  * The data structure, here, has multiple slicers per
	  * demodulator (subchannel).  Due to fuzzy thinking or
	  * expediency, the multiple slicers got mapped into subchannels.
	  * This means we can't use both multiple decoders and
	  * multiple slicers at the same time.
	  *
	  * Clean this up in 1.3 and keep the concepts separate.
	  * This means adding a third variable many places
	  * we are passing around the origin.
	  *
	  */
	struct {

		signed int data_clock_pll;		// PLL for data clock recovery.
							// It is incremented by pll_step_per_sample
							// for each audio sample.
							// Must be 32 bits!!!
							// So far, this is the case for every compiler used.

		signed int prev_d_c_pll;		// Previous value of above, before
							// incrementing, to detect overflows.

		int pll_symbol_count;			// Number symbols during time nudge_total is accumulated.
		int64_t pll_nudge_total;		// Sum of DPLL nudge amounts.
							// Both of these are cleared at start of frame.
							// At end of frame, we can see if incoming
							// baud rate is a little off.

		int prev_demod_data;			// Previous data bit detected.
							// Used to look for transitions.
		float prev_demod_out_f;

		/* This is used only for "9600" baud data. */

		int lfsr;				// Descrambler shift register.

		// This is for detecting phase lock to incoming signal.

		int good_flag;				// Set if transition is near where expected,
							// i.e. at a good time.
		int bad_flag;				// Set if transition is not where expected,
							// i.e. at a bad time.
		unsigned char good_hist;		// History of good transitions for past octet.
		unsigned char bad_hist;			// History of bad transitions for past octet.
		unsigned int score;			// History of whether good triumphs over bad
							// for past 32 symbols.
		int data_detect;			// True when locked on to signal.

	} slicer[MAX_SLICERS];				// Actual number in use is num_slicers.
							// Should be in range 1 .. MAX_SLICERS,
/*
 * Version 1.6:
 *
 *	This has become quite disorganized and messy with different combinations of
 *	fields used for different demodulator types.  Start to reorganize it into a common
 *	part (with things like the DPLL for clock recovery), and separate sections
 *	for each of the demodulator types.
 *	Still a lot to do here.
 */

	union {

		//////////////////////////////////////////////////////////////////////////////////
		//										//
		//			AFSK only - new method in 1.7				//
		//										//
		//////////////////////////////////////////////////////////////////////////////////


		struct afsk_only_s {

			unsigned int m_osc_phase;		// Phase for Mark local oscillator.
			unsigned int m_osc_delta;		// How much to change for each audio sample.

			unsigned int s_osc_phase;		// Phase for Space local oscillator.
			unsigned int s_osc_delta;		// How much to change for each audio sample.

			unsigned int c_osc_phase;		// Phase for Center frequency local oscillator.
			unsigned int c_osc_delta;		// How much to change for each audio sample.

			// Need two mixers for profile "A".

			float m_I_raw[MAX_FILTER_SIZE];
			float m_Q_raw[MAX_FILTER_SIZE];

			float s_I_raw[MAX_FILTER_SIZE];
			float s_Q_raw[MAX_FILTER_SIZE];

			// Only need one mixer for profile "B".  Reuse the same storage?

	//#define c_I_raw m_I_raw
	//#define c_Q_raw m_Q_raw
			float c_I_raw[MAX_FILTER_SIZE];
			float c_Q_raw[MAX_FILTER_SIZE];

			int use_rrc;		// Use RRC rather than generic low pass.

			float rrc_width_sym;	/* Width of RRC filter in number of symbols.  */

			float rrc_rolloff;		/* Rolloff factor for RRC.  Between 0 and 1. */

			float prev_phase;		// To see phase shift between samples for FM demod.

			float normalize_rpsam;	// Normalize to -1 to +1 for expected tones.

		} afsk;

		//////////////////////////////////////////////////////////////////////////////////
		//										//
		//				Baseband only, AKA G3RUH			//
		//										//
		//////////////////////////////////////////////////////////////////////////////////

		// TODO: Continue experiments with root raised cosine filter.
		// Either switch to that or take out all the related stuff.

		struct bb_only_s {

			float rrc_width_sym;		/* Width of RRC filter in number of symbols. */

			float rrc_rolloff;		/* Rolloff factor for RRC.  Between 0 and 1. */

			int rrc_filter_taps;		// Number of elements used in the next two.

	// FIXME: TODO: reevaluate max size needed.

			float audio_in[MAX_FILTER_SIZE];	// Audio samples in.


			float lp_filter[MAX_FILTER_SIZE];	// Low pass filter.

			// New in 1.7 - Polyphase filter to reduce CPU requirements.

			float lp_polyphase_1[MAX_FILTER_SIZE];
			float lp_polyphase_2[MAX_FILTER_SIZE];
			float lp_polyphase_3[MAX_FILTER_SIZE];
			float lp_polyphase_4[MAX_FILTER_SIZE];

			float lp_1_iir_param;		// very low pass filters to get DC offset.
			float lp_1_out;

			float lp_2_iir_param;
			float lp_2_out;

			float agc_1_fast_attack;	// Signal envelope detection.
			float agc_1_slow_decay;
			float agc_1_peak;
			float agc_1_valley;

			float agc_2_fast_attack;
			float agc_2_slow_decay;
			float agc_2_peak;
			float agc_2_valley;

			float agc_3_fast_attack;
			float agc_3_slow_decay;
			float agc_3_peak;
			float agc_3_valley;

			// CIC low pass filters to detect DC bias or low frequency changes.
			// IIR behaves like an analog R-C filter.
			// Intuitively, it seems like FIR would be better because it is based on a finite history.
			// However, it would require MANY taps and a LOT of computation for a low frequency.
			// We can use a little trick here to keep a running average.
			// This would be equivalent to convolving with an array of all 1 values.
			// That would eliminate the need to multiply.
			// We can also eliminate the need to add them all up each time by keeping a running total.
			// Add a sample to the total when putting it in our array of recent samples.
			// Subtract it from the total when it gets pushed off the end.
			// We can also eliminate the need to shift them all down by using a circular buffer.
			// This only works with integers because float would have cumulated round off errors.

			cic_t cic_center1;
			cic_t cic_above;
			cic_t cic_below;

		} bb;

		//////////////////////////////////////////////////////////////////////////////////
		//										//
		//					PSK only.				//
		//										//
		//////////////////////////////////////////////////////////////////////////////////


		struct psk_only_s {

			enum v26_e v26_alt;		// Which alternative when V.26.

			float sin_table256[256];	// Precomputed sin table for speed.


		// Optional band pass pre-filter before phase detector.

	// TODO? put back into common section?
	// TODO? Why was I thinking that?

			int use_prefilter;	// True to enable it.

			float prefilter_baud;	// Cutoff frequencies, as fraction of baud rate, beyond tones used.
						// In the case of PSK, we use only a single tone of 1800 Hz.
						// If we were using 2400 bps (= 1200 baud), this would be
						// the fraction of 1200 for the cutoff below and above 1800.


			float pre_filter_width_sym;  /* Length in number of symbol times. */

			int pre_filter_taps;	/* Size of pre filter, in audio samples. */

			bp_window_t pre_window;

			float audio_in[MAX_FILTER_SIZE];
			float pre_filter[MAX_FILTER_SIZE];

			// Use local oscillator or correlate with previous sample.

			int psk_use_lo;		/* Use local oscillator rather than self correlation. */

			unsigned int lo_step;	/* How much to advance the local oscillator */
						/* phase for each audio sample. */

			unsigned int lo_phase;	/* Local oscillator phase accumulator for PSK. */

			// After mixing with LO before low pass filter.

			float I_raw[MAX_FILTER_SIZE];	// signal * LO cos.
			float Q_raw[MAX_FILTER_SIZE];	// signal * LO sin.

			// Number of delay line taps into previous symbol.
			// They are one symbol period and + or - 45 degrees of the carrier frequency.

			int boffs;		/* symbol length based on sample rate and baud. */
			int coffs;		/* to get cos component of previous symbol. */
			int soffs;		/* to get sin component of previous symbol. */

			float delay_line_width_sym;
			int delay_line_taps;	// In audio samples.

			float delay_line[MAX_FILTER_SIZE];

			// Low pass filter Second is frequency as ratio to baud rate for FIR.

		// TODO? put back into common section?
		// TODO? What are the tradeoffs?
			float lpf_baud;			/* Cutoff frequency as fraction of baud. */
							/* Intuitively we'd expect this to be somewhere */
							/* in the range of 0.5 to 1. */

			float lp_filter_width_sym;  	/* Length in number of symbol times. */

			int lp_filter_taps;		/* Size of Low Pass filter, in audio samples (i.e. filter taps). */

			bp_window_t lp_window;

			float lp_filter[MAX_FILTER_SIZE];

		} psk;

	} u;	// end of union for different demodulator types.

};


/*-------------------------------------------------------------------
 *
 * Name:        pll_dcd_signal_transition2
 *		dcd_each_symbol2
 *
 * Purpose:     New DCD strategy for 1.6.
 *
 * Inputs:	D		Pointer to demodulator state.
 *
 *		chan		Radio channel: 0 to MAX_CHANS - 1
 *
 *		subchan		Which of multiple demodulators: 0 to MAX_SUBCHANS - 1
 *
 *		slice		Slicer number: 0 to MAX_SLICERS - 1.
 *
 *		dpll_phase	Signed 32 bit counter for DPLL phase.
 *				Wraparound is where data is sampled.
 *				Ideally transitions would occur close to 0.
 *
 * Output:	D->slicer[slice].data_detect - true when PLL is locked to incoming signal.
 *
 * Description:	From the beginning, DCD was based on finding several flag octets
 *		in a row and dropping when eight bits with no transitions.
 *		It was less than ideal but we limped along with it all these years.
 *		This fell apart when FX.25 came along and a couple of the
 *		correlation tags have eight "1" bits in a row.
 *
 * 		Our new strategy is to keep a running score of how well demodulator
 *		output transitions match to where expected.
 *
 *--------------------------------------------------------------------*/



 // These are good for 1200 bps AFSK.
 // Might want to override for other modems.

#ifndef DCD_THRESH_ON
#define DCD_THRESH_ON 30		// Hysteresis: Can miss 2 out of 32 for detecting lock.
					// 31 is best for TNC Test CD.  30 almost as good.
					// 30 better for 1200 regression test.
#endif

#ifndef DCD_THRESH_OFF
#define DCD_THRESH_OFF 6		// Might want a little more fine tuning.
#endif

#ifndef DCD_GOOD_WIDTH
#define DCD_GOOD_WIDTH 512		// No more than 1024!!!
#endif


inline static void pll_dcd_signal_transition2(struct demodulator_state_s *D, int slice, int dpll_phase)
{
	if (dpll_phase > -DCD_GOOD_WIDTH * 1024 * 1024 && dpll_phase < DCD_GOOD_WIDTH * 1024 * 1024) {
		D->slicer[slice].good_flag = 1;
	}
	else {
		D->slicer[slice].bad_flag = 1;
	}
}


inline static void pll_dcd_each_symbol2(struct demodulator_state_s *D, int chan, int subchan, int slice)
{
	D->slicer[slice].good_hist <<= 1;
	D->slicer[slice].good_hist |= D->slicer[slice].good_flag;
	D->slicer[slice].good_flag = 0;

	D->slicer[slice].bad_hist <<= 1;
	D->slicer[slice].bad_hist |= D->slicer[slice].bad_flag;
	D->slicer[slice].bad_flag = 0;

	D->slicer[slice].score <<= 1;
	// 2 is to detect 'flag' patterns with 2 transitions per octet.
	D->slicer[slice].score |= (signed)__builtin_popcount(D->slicer[slice].good_hist)
		- (signed)__builtin_popcount(D->slicer[slice].bad_hist) >= 2;

	int s = __builtin_popcount(D->slicer[slice].score);
	if (s >= DCD_THRESH_ON) {
		if (D->slicer[slice].data_detect == 0) {
			D->slicer[slice].data_detect = 1;
			dcd_change(chan, subchan, slice, D->slicer[slice].data_detect);
		}
	}
	else if (s <= DCD_THRESH_OFF) {
		if (D->slicer[slice].data_detect != 0) {
			D->slicer[slice].data_detect = 0;
			dcd_change(chan, subchan, slice, D->slicer[slice].data_detect);
		}
	}
}



/* Provided elsewhere to process a complete frame. */

//void process_rec_frame (int chan, unsigned char *fbuf, int flen, int level);


/* Is HLDC decoder is currently gathering bits into a frame? */
/* Similar to, but not exactly the same as, data carrier detect. */
/* We use this to influence the PLL inertia. */

int hdlc_rec_gathering(int chan, int subchan, int slice);

/* Transmit needs to know when someone else is transmitting. */

void dcd_change(int chan, int subchan, int slice, int state);

int hdlc_rec_data_detect_any(int chan);


#define FASTER13 1		// Don't pack 8 samples per byte.


//typedef short slice_t;


/*
 * Maximum size (in bytes) of an AX.25 frame including the 2 octet FCS.
 */

#define MAX_FRAME_LEN ((AX25_MAX_PACKET_LEN) + 2)	

 /*
  * Maximum number of bits in AX.25 frame excluding the flags.
  * Adequate for extreme case of bit stuffing after every 5 bits
  * which could never happen.
  */

#define MAX_NUM_BITS (MAX_FRAME_LEN * 8 * 6 / 5)

typedef struct rrbb_s {
	int magic1;
	struct rrbb_s* nextp;	/* Next pointer to maintain a queue. */

	int chan;		/* Radio channel from which it was received. */
	int subchan;		/* Which modem when more than one per channel. */
	int slice;		/* Which slicer. */

	alevel_t alevel;	/* Received audio level at time of frame capture. */
	unsigned int len;	/* Current number of samples in array. */

	int is_scrambled;	/* Is data scrambled G3RUH / K9NG style? */
	int descram_state;	/* Descrambler state before first data bit of frame. */
	int prev_descram;	/* Previous descrambled bit. */

	unsigned char fdata[MAX_NUM_BITS];

	int magic2;
} *rrbb_t;



rrbb_t rrbb_new(int chan, int subchan, int slice, int is_scrambled, int descram_state, int prev_descram);

void rrbb_clear(rrbb_t b, int is_scrambled, int descram_state, int prev_descram);


static inline /*__attribute__((always_inline))*/ void rrbb_append_bit(rrbb_t b, const unsigned char val)
{
	if (b->len >= MAX_NUM_BITS) {
		return;	/* Silently discard if full. */
	}
	b->fdata[b->len] = val;
	b->len++;
}

static inline /*__attribute__((always_inline))*/ unsigned char rrbb_get_bit(const rrbb_t b, const int ind)
{
	return (b->fdata[ind]);
}


void rrbb_chop8(rrbb_t b);

int rrbb_get_len(rrbb_t b);

//void rrbb_flip_bit (rrbb_t b, unsigned int ind);

void rrbb_delete(rrbb_t b);

void rrbb_set_nextp(rrbb_t b, rrbb_t np);
rrbb_t rrbb_get_nextp(rrbb_t b);

int rrbb_get_chan(rrbb_t b);
int rrbb_get_subchan(rrbb_t b);
int rrbb_get_slice(rrbb_t b);

void rrbb_set_audio_level(rrbb_t b, alevel_t alevel);
alevel_t rrbb_get_audio_level(rrbb_t b);

int rrbb_get_is_scrambled(rrbb_t b);
int rrbb_get_descram_state(rrbb_t b);
int rrbb_get_prev_descram(rrbb_t b);




void hdlc_rec2_init(struct audio_s *audio_config_p);

void hdlc_rec2_block(rrbb_t block);

int hdlc_rec2_try_to_fix_later(rrbb_t block, int chan, int subchan, int slice, alevel_t alevel);

/* Provided by the top level application to process a complete frame. */

void app_process_rec_packet(int chan, int subchan, int slice, packet_t pp, alevel_t level, int is_fx25, retry_t retries, char *spectrum);





int gen_tone_init(struct audio_s *pp, int amp, int gen_packets);


//int gen_tone_open (int nchan, int sample_rate, int bit_rate, int f1, int f2, int amp, char *fname);

//int gen_tone_open_fd (int nchan, int sample_rate, int bit_rate, int f1, int f2, int amp, int fd)  ;

//int gen_tone_close (void);

void tone_gen_put_bit(int chan, int dat, int scramble);

void gen_tone_put_sample(int chan, int a, int sam); 

enum dw_color_e {
	DW_COLOR_INFO,		/* black */
	DW_COLOR_ERROR,		/* red */
	DW_COLOR_REC,		/* green */
	DW_COLOR_DECODED,	/* blue */
	DW_COLOR_XMIT,		/* magenta */
	DW_COLOR_DEBUG		/* dark_green */
};

typedef enum dw_color_e dw_color_t;


void text_color_init(int enable_color);
void text_color_set(dw_color_t c);
void text_color_term(void);


/* Degree symbol. */

#if __WIN32__

//#define CH_DEGREE "\xc2\xb0"	/* UTF-8. */

#define CH_DEGREE " "


#else

/* Maybe we could change this based on LANG environment variable. */

//#define CH_DEGREE "\xc2\xb0"	/* UTF-8. */

#define CH_DEGREE " "

#endif

/* demod_9600.h */


void demod_9600_init(enum modem_t modem_type, int original_sample_rate, int upsample, int baud, struct demodulator_state_s *D);

void demod_9600_process_sample(int chan, int sam, int upsample, struct demodulator_state_s *D);


/* Undo data scrambling for 9600 baud. */

static inline int descramble(int in, int *state)
{
	int out;

	out = (in ^ (*state >> 16) ^ (*state >> 11)) & 1;
	*state = (*state << 1) | (in & 1);
	return (out);
}
