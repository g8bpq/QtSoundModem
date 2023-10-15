#pragma once

//
//	 My port of UZ7HO's Soundmodem
//

#define VersionString "0.0.0.70"
#define VersionBytes {0, 0, 0, 70}

// Added FX25. 4x100 FEC and V27 not Working and disabled

// 0.8 V27 now OK.

// 0.9 Digipeating added

// 0.10 Fix second channel tones and calibrate

// 0.11 Fix allocation of sessions to correct modem
//		Fix DCD
//		Fix Monitoring of Multiline packets
//		Fix possible saving of wrong center freq
//		Limit TX sample Q in Linux
//

// 0.12	Add AGWPE monitoring of received frames
//		Fix DCD Threshold
//		Fix KISS transparency issue

// 0.13 Fix sending last few bits in FX.25 Mode

// 0.14 Add "Copy on Select" to Trace Window

// 0.15 Limit Trace window to 10000 lines

// 0.16 Fix overwriting monitor window after scrollback

// 0.17	Add GPIO and CAT PTT

// 0.18	Add CM108/119 PTT

// 0.19 Fix scheduling KISS frames

// 0.20 Debug code added to RR processing

// 0.21	Fix AGW monitor of multiple line packets
//		Close ax.25 sessions if AGW Host session closes

// 0.22	Add FEC Count to Session Stats

// 0.23 Retry DISC until UA received or retry count exceeded

// 0.24	More fixes to DISC handling

// 0.26 Add OSS PulseAudio and HAMLIB support

// 0.27 Dynamically load PulseAudio modules

// 0.28 Add ARDOPPacket Mode

// 0.29 Fix saving settings and geometry on close
// 0.30 Retructure code to build with Qt 5.3
//      Fix crash in nogui mode if pulse requested but not available
//		Try to fix memory leaks

// 0.31 Add option to run modems in seprate threads

// 0.32	Fix timing problem with AGW connect at startup
//		Add Memory ARQ
//		Add Single bit "Correction"
//		Fix error in 31 when using multiple decoders

// 0.33 Fix Single bit correction
//		More memory leak fixes

// 0.34 Add API to set Modem and Center Frequency
//		Fix crash in delete_incoming_mycalls

// 0.35 Return Version in AGW Extended g response

// 0.36 Fix timing problem on startup

// 0.37 Add scrollbars to Device and Modem dialogs

// 0.38 Change default CM108 name to /dev/hidraw0 on Linux

// 0.39	Dont try to display Message Boxes in nogui mode.
//		Close Device and Modem dialogs on Accept or Reject
//		Fix using HAMLIB in nogui mode

// 0.40	Fix bug in frame optimize when using 6 char calls

// 0.41	Fix "glitch" on waterfall markers when changing modem freqs 

// 0.42	Add "Minimize to Tray" option

// 0.43 Add Andy's on_SABM fix.
//		Fix Crash if KISS Data sent to AGW port

// 0.44 Add UDP bridge.

// 0.45 Add two more modems.
// 0.46 Fix two more modems.

// 0.47 Fix suprious DM when host connection lost
//		Add CWID

// 0.48 Send FRMR for unrecognised frame types

// 0.49 Add Andy's FEC Tag correlation coode

// 0.50 Fix Waterfall display when only using right channel
//		Allow 1200 baud fsk at other center freqs
//		Add Port numbers to Window title and Try Icon tooltip
//		Fix calculation of filters for multiple decoders
//		Add RX Offset setting (for satellite operation

// 0.51	Fix Multithreading with more that 2 modems

// 0.52	Add Stdin as source on Linux

// 0.53	Use Byte instead of byte as byte is defined in newer versions of gcc

// 0.54 Fix for ALSA problem on new pi OS

// 0.55 Fix for compiler error with newer compiler

// 0.56	Fix errors in Config.cpp			June 22

// 0.57	Add Restart Waterfall action		August 22

// 0.58 Add RSID							Sept 2022

// 0.59 Add config of Digi Calls			Dec 2022

// 0.60 Allow ARDOP Packet on modems 2 to 4 March 2023

// 0.61 Add il2p support					April 2023

// 0.62										April 2023
//	Add option to specify sound devices that aren't in list
//	Add Save button to Modem dialog to save current tab without closing dialog
//	Don't add plug: to Linux device addresses unless addr contains : (allows use of eg ARDOP)

// 0.64 Fix sending ax.25 (broken in .61)

// 0.65	Allow Set Modem command to use modem index as well as modem name

// 0.66 Allow configuration of waterfall span	June 23
//		Add Exclude

// .67 Add extra modes 8PSK 900 RUH 4800 RUH 9600 QPSK 600 QPSK 2400	August 23
//	   Fix loading txtail
//	   Fix digipeating
//	   Add MaxFrame to Modem Dialog
//	   Fix 64 bit compatibility in ackmode
//	   Add option to change CWID tones
//	   Fix minimum centre freq validation

// .68 Monitor XID and TEST
//	   Flag active interface in title bar
//	   Improve header validation in il2p
//	   Add CWID only after traffic option
//	   Add font selection
//	   Separate modem bandwidth indicators
//	   Fix problems with il2p and PSK modes in nogui mode
//	   Add signal level bar to GUI
//	   Fix Waterfall display when using right channel only
//	   Allow PTT device to be added 

// .69	Add basic Dark Theme
//		Fix some timing bugs in Waterfall and RX Level refresh
//		Only display session table if AGW interface is enabled
//		Fix operation with both left and right channels in use

// .70  Restructure Waterfall area to be a single image 



#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED(x) (void)(x)

#ifdef M_PI
#undef M_PI
#endif

#define M_PI       3.1415926f

#define pi M_PI

#ifndef WIN32
#define _strdup strdup
#endif

	//#define NULL ((void *)0)

	//Delphi Types remember case insensitive

#define single float
#define boolean int
#define Byte unsigned char		//                  0 to 255
#define Word unsigned short	//                        0 to 65,535
#define SmallInt short 		//                  -32,768 to 32,767
#define LongWord unsigned int	//                        0 to 4,294,967,295
 //  Int6 : Cardinal; //                        0 to 4,294,967,295
#define LongInt int			//           -2,147,483,648 to 2,147,483,647
#define Integer int  //           -2,147,483,648 to 2,147,483,647
//#define Int64 long long		 // -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

//#define Byte unsigned char		//                  0 to 255
#define word unsigned short	//                        0 to 65,535
#define smallint short 		//                  -32,768 to 32,767
#define longword unsigned int	//                        0 to 4,294,967,295
 //  Int6 : Cardinal; //                        0 to 4,294,967,295
#define longint int			//           -2,147,483,648 to 2,147,483,647
#define integer int  //           -2,147,483,648 to 2,147,483,647

typedef unsigned long ULONG;

#define UCHAR unsigned char
#define UINT unsigned int
#define BOOL int
#define TRUE 1
#define FALSE 0

// Soundcard Channels

#define NONE 0
#define LEFT 1
#define RIGHT 2

#define nr_emph 2

#define decodedNormal 4 //'-'
#define decodedFEC    3 //'F'
#define decodedMEM	  2 //'#'
#define decodedSingle 1 //'$'


// Think about implications of changing this !!
extern int FFTSize;

// Seems to use Delphi TStringList for a lot of queues. This seems to be a list of pointers and a count
// Each pointer is to a Data/Length pair
//Maybe something like

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

// QPSK struct

typedef struct TQPSK_t
{
	UCHAR tx[4];
	int count[4];
	UCHAR rx[4];
	UCHAR mode;
} TPQSK;


typedef struct TKISSMode_t
{
	string * data_in;
	void * Socket;				// Used as a key

	// Not sure what rest are used for. Seems to be one per channel

	TStringList buffer[4];			// Outgoing Frames

} TKISSMode;

typedef struct  TMChannel_t
{

	single prev_LPF1I_buf[4096];
	single prev_LPF1Q_buf[4096];
	single prev_dLPFI_buf[4096];
	single prev_dLPFQ_buf[4096];
	single prev_AFCI_buf[4096];
	single prev_AFCQ_buf[4096];
	single AngleCorr;
	single MUX_osc;
	single AFC_IZ1;
	single AFC_IZ2;
	single AFC_QZ1;
	single AFC_QZ2;
	single AFC_bit_buf1I[1024];
	single AFC_bit_buf1Q[1024];
	single AFC_bit_buf2[1024];
	single AFC_IIZ1;
	single AFC_QQZ1;

} TMChannel;

typedef struct TFX25_t
{
	string  data;
	Byte  status;
	Byte  bit_cnt;
	Byte  byte_rx;
	unsigned long long tag;
	Byte  size;
	Byte  rs_size;
	Byte size_cnt;
} TFX25;



typedef struct TDetector_t
{
	struct TFX25_t fx25[4];
	TStringList	mem_ARQ_F_buf[5];
	TStringList mem_ARQ_buf[5];
	float pll_loop[5];
	float last_sample[5];
	UCHAR ones[5];
	UCHAR zeros[5];
	float bit_buf[5][1024];
	float bit_buf1[5][1024];
	UCHAR sample_cnt[5];
	UCHAR last_bit[5];
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

	UCHAR last_rx_bit[5];
	UCHAR bit_stream[5];
	UCHAR byte_rx[5];
	UCHAR bit_stuff_cnt[5];
	UCHAR bit_cnt[5];
	float bit_osc[5];
	UCHAR frame_status[5];
	string rx_data[5];
	string FEC_rx_data[5];
	//
	UCHAR FEC_pol[5];
	unsigned short FEC_err[5];
	unsigned long long FEC_header1[5][2];
	unsigned short FEC_blk_int[5];
	unsigned short FEC_len_int[5];
	unsigned short FEC_len[5];

	unsigned short FEC_len_cnt[5];
	
	UCHAR rx_intv_tbl[5][4];
	UCHAR rx_intv_sym[5];
	UCHAR rx_viterbi[5];
	UCHAR viterbi_cnt[5];
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
	UCHAR last_nrzi_bit[5];

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

	Byte emph_decoded;	
	Byte rx_decoded;
	Byte errors;

} TDetector;



typedef struct AGWUser_t
{
	void *socket;
	string * data_in;
	TStringList AGW_frame_buf;
	boolean	Monitor;
	boolean	Monitor_raw;
	boolean reportFreqAndModem;			// Can report modem and frequency to host

} AGWUser;

typedef struct  TAX25Info_t
{
	longint	stat_s_pkt;
	longint stat_s_byte;
	longint stat_r_pkt;
	longint stat_r_byte;
	longint stat_r_fc;
	longint stat_fec_count;
	time_t stat_begin_ses;
	time_t stat_end_ses;
	longint stat_l_r_byte;
	longint stat_l_s_byte;

} TAX25Info;

typedef struct TAX25Port_t
{
	Byte hi_vs;
	Byte vs;
	Byte vr;
	Byte PID;
	TStringList in_data_buf;
	TStringList frm_collector;
	string frm_win[8];
	string out_data_buf;
	word t1;
	word t2;
	word t3;
	Byte i_lo;
	Byte i_hi;
	word n1;
	word n2;
	word IPOLL_cnt;
	TStringList frame_buf; //буфер кадров на передачу
	TStringList I_frame_buf;
	Byte status;
	word clk_frack;
	char corrcall[10];
	char mycall[10];
	UCHAR digi[56];
	UCHAR Path[80];				// Path in ax25 format - added to save building it each time
	UCHAR ReversePath[80];
	int snd_ch;					// Simplifies parameter passing
	int port;
	int pathLen;
	void * socket;
	char kind[16];
	TAX25Info info;
} TAX25Port;


#define LOGEMERGENCY 0 
#define LOGALERT 1
#define LOGCRIT 2 
#define LOGERROR 3 
#define LOGWARNING 4
#define LOGNOTICE 5
#define LOGINFO 6
#define LOGDEBUG 7

#define PTTRTS		1
#define PTTDTR		2
#define PTTCAT		4
#define PTTCM108	8
#define PTTHAMLIB	16

// Status flags

#define STAT_NO_LINK  0
#define STAT_LINK 1
#define STAT_CHK_LINK 2
#define STAT_WAIT_ANS 3
#define STAT_TRY_LINK 4
#define STAT_TRY_UNLINK 5


	// Сmd,Resp,Poll,Final,Digipeater flags
#define 	SET_P 1
#define 	SET_F 0
#define 	SET_C 1
#define 	SET_R 0
#define 	SET_NO_RPT 0
#define 	SET_RPT 1
	// Frame ID flags
#define 	I_FRM 0
#define 	S_FRM 1
#define 	U_FRM 2
#define 	I_I 0
#define 	S_RR 1
#define 	S_RNR 5
#define 	S_REJ 9
#define		S_SREJ 0x0D
#define 	U_SABM 47
#define 	U_DISC 67
#define 	U_DM 15
#define 	U_UA 99
#define 	U_FRMR 135
#define 	U_UI 3

#define		U_XID 0xAF
#define		U_TEST 0xE3

	// PID flags
#define 	PID_X25 0x01       // 00000001-CCIT X25 PLP
#define 	PID_SEGMENT 0x08   // 00001000-Segmentation fragment
#define 	PID_TEXNET 0xC3    // 11000011-TEXNET Datagram Protocol
#define 	PID_LQ 0xC4        // 11001000-Link Quality Protocol
#define 	PID_APPLETALK 0xCA // 11001010-Appletalk
#define 	PID_APPLEARP 0xCB  // 11001011-Appletalk ARP
#define 	PID_IP 0xCC        // 11001100-ARPA Internet Protocol
#define 	PID_ARP 0xCD       // 11001101-ARPA Address Resolution Protocol
#define 	PID_NET_ROM 0xCF   // 11001111-NET/ROM


//	Sound interface buffer sizes

extern int ReceiveSize;
extern int SendSize;

#define NumberofinBuffers 4

#define Now getTicks()

// #defines from all modules (?? is this a good idaa ??

#define WIN_MAXIMIZED 0
#define WIN_MINIMIZED 1
#define MODEM_CAPTION 'SoundModem by UZ7HO'
#define MODEM_VERSION '1.06'
#define SND_IDLE 0
#define SND_TX 1
#define BUF_EMPTY 0
#define BUF_FULL 1
#define DISP_MONO FALSE
#define DISP_RGB TRUE
#define MOD_IDLE 0
#define MOD_RX 1
#define MOD_TX 2
#define MOD_WAIT 3
#define TIMER_FREE 0
#define TIMER_BUSY 1
#define TIMER_OFF 2
#define TIMER_EVENT_ON 3
#define TIMER_EVENT_OFF 4
#define DEBUG_TIMER 1
#define DEBUG_WATERFALL 2
#define DEBUG_DECODE 4
#define DEBUG_SOUND 8
#define IS_LAST TRUE
#define IS_NOT_LAST FALSE
#define modes_count 20
#define SPEED_300 0
#define SPEED_1200 1
#define SPEED_600 2
#define SPEED_2400 3
#define SPEED_P1200 4
#define SPEED_P600 5
#define SPEED_P300 6
#define SPEED_P2400 7
#define SPEED_Q4800 8
#define SPEED_Q3600 9
#define SPEED_Q2400 10
#define SPEED_MP400 11
#define SPEED_DW2400 12
#define SPEED_8P4800 13
#define SPEED_2400V26B 14
#define SPEED_ARDOP 15
#define SPEED_Q300 16
#define SPEED_8PSK300 17
#define SPEED_RUH48 18
#define SPEED_RUH96 19

#define MODE_FSK 0
#define MODE_BPSK 1
#define MODE_QPSK 2
#define MODE_MPSK 3
#define MODE_8PSK 4
#define MODE_PI4QPSK 5
#define MODE_ARDOP 6
#define MODE_RUH 7

#define QPSK_SM 0
#define QPSK_V26 1

#define MODEM_8P4800_BPF 3200
#define MODEM_8P4800_TXBPF 3400
#define MODEM_8P4800_LPF 1000
#define MODEM_8P4800_BPF_TAP 64
#define MODEM_8P4800_LPF_TAP 8
 //
#define MODEM_MP400_BPF 775
#define MODEM_MP400_TXBPF 850
#define MODEM_MP400_LPF 70
#define MODEM_MP400_BPF_TAP 256
#define MODEM_MP400_LPF_TAP 128
 //
#define MODEM_DW2400_BPF 2400
#define MODEM_DW2400_TXBPF 2500
#define MODEM_DW2400_LPF 900
#define MODEM_DW2400_BPF_TAP 256 //256
#define MODEM_DW2400_LPF_TAP 32  //128
 //
#define MODEM_Q2400_BPF 2400
#define MODEM_Q2400_TXBPF 2500
#define MODEM_Q2400_LPF 900
#define MODEM_Q2400_BPF_TAP 256 //256
#define MODEM_Q2400_LPF_TAP 128  //128
 //
#define MODEM_Q3600_BPF 3600
#define MODEM_Q3600_TXBPF 3750
#define MODEM_Q3600_LPF 1350
#define MODEM_Q3600_BPF_TAP 256
#define MODEM_Q3600_LPF_TAP 128
 //
#define MODEM_Q4800_BPF 4800
#define MODEM_Q4800_TXBPF 5000
#define MODEM_Q4800_LPF 1800
#define MODEM_Q4800_BPF_TAP 256
#define MODEM_Q4800_LPF_TAP 128
 //
#define MODEM_P2400_BPF 4800
#define MODEM_P2400_TXBPF 5000
#define MODEM_P2400_LPF 1800
#define MODEM_P2400_BPF_TAP 256
#define MODEM_P2400_LPF_TAP 128
 //
#define MODEM_P1200_BPF 2400
#define MODEM_P1200_TXBPF 2500
#define MODEM_P1200_LPF 900
#define MODEM_P1200_BPF_TAP 256
#define MODEM_P1200_LPF_TAP 128
 //
#define MODEM_P600_BPF 1200
#define MODEM_P600_TXBPF 1250
#define MODEM_P600_LPF 400
#define MODEM_P600_BPF_TAP 256
#define MODEM_P600_LPF_TAP 128
 //
#define MODEM_P300_BPF 600
#define MODEM_P300_TXBPF 625
#define MODEM_P300_LPF 200
#define MODEM_P300_BPF_TAP 256
#define MODEM_P300_LPF_TAP 128
 //
#define MODEM_300_BPF 500
#define MODEM_300_TXBPF 500
#define MODEM_300_LPF 155
#define MODEM_300_BPF_TAP 256
#define MODEM_300_LPF_TAP 128
 //
#define MODEM_600_BPF 800
#define MODEM_600_TXBPF 900
#define MODEM_600_LPF 325
#define MODEM_600_BPF_TAP 256
#define MODEM_600_LPF_TAP 128
 //
#define MODEM_1200_BPF 1400
#define MODEM_1200_TXBPF 1600
#define MODEM_1200_LPF 650
#define MODEM_1200_BPF_TAP 256
#define MODEM_1200_LPF_TAP 128
 //
#define MODEM_2400_BPF 3200
#define MODEM_2400_TXBPF 3200
#define MODEM_2400_LPF 1400
#define MODEM_2400_BPF_TAP 256
#define MODEM_2400_LPF_TAP 128

#define TX_SILENCE 0
#define TX_DELAY 1
#define TX_TAIL 2
#define TX_NO_DATA 3
#define TX_FRAME 4
#define TX_WAIT_BPF 5


#define FRAME_WAIT 0
#define FRAME_LOAD 1
#define RX_BIT0 0
#define RX_BIT1 128
#define DCD_WAIT_SLOT 0
#define DCD_WAIT_PERSIST 1

#define FX25_MODE_NONE  0
#define FX25_MODE_RX  1
#define FX25_MODE_TXRX 2
#define FX25_TAG 0
#define FX25_LOAD 1

#define IL2P_MODE_NONE  0
#define IL2P_MODE_RX  1				// RX il2p + HDLC
#define IL2P_MODE_TXRX 2
#define IL2P_MODE_ONLY 3			// RX only il2p, TX il2p


#define    MODE_OUR 0
#define    MODE_OTHER 1
#define    MODE_RETRY 2

#define FRAME_FLAG 126		// 7e

#define port_num 32		// ?? Max AGW sessions
#define PKT_ERR 17		// Minimum packet size, bytes
#define I_MAX 7			// Maximum number of packets


	// externs for all modules

#define ARDOPBufferSize 12000 * 100

extern short ARDOPTXBuffer[4][ARDOPBufferSize];	// Enough to hold whole frame of samples

extern int ARDOPTXLen[4];				// Length of frame
extern int ARDOPTXPtr[4];				// Tx Pointer

extern BOOL KISSServ;
extern int KISSPort;

extern BOOL AGWServ;
extern int AGWPort;

extern TStringList KISS_acked[];
extern TStringList KISS_iacked[];

extern TStringList all_frame_buf[5];

extern unsigned short pkt_raw_min_len;
extern int stat_r_mem;

extern UCHAR diddles;

extern int stdtones;
extern int fullduplex;

extern struct TQPSK_t qpsk_set[4];

extern int NonAX25[5];

extern short txtail[5];
extern short txdelay[5];

extern short modem_def[5];

extern int emph_db[5];
extern UCHAR emph_all[5];

extern UCHAR modem_mode[5];

extern UCHAR RCVR[5];
extern int soundChannel[5];
extern int modemtoSoundLR[4];

extern short rx_freq[5];
extern short active_rx_freq[5];
extern short rx_shift[5];
extern short rx_baudrate[5];
extern short rcvr_offset[5];

extern int tx_hitoneraisedb[5];
extern float tx_hitoneraise[5];


extern UCHAR tx_status[5];
extern float tx_freq[5];
extern float tx_shift[5];
extern unsigned short tx_baudrate[5];
extern unsigned short tx_bitrate[5];

extern unsigned short bpf[5];
extern unsigned short lpf[5];

extern unsigned short txbpf[5];

extern unsigned short  tx_BPF_tap[5];
extern unsigned short  tx_BPF_timer[5];

extern unsigned short  BPF_tap[5];
extern unsigned short  LPF_tap[5];

extern float tx_BPF_core[5][32768];
extern float LPF_core[5][2048];

extern UCHAR xData[256];
extern UCHAR xEncoded[256];
extern UCHAR xDecoded[256];

extern float PI125;
extern float PI375;
extern float PI625;
extern float PI875;
extern 	float PI5;
extern float PI25;
extern float PI75;

extern int max_frame_collector[4];
extern boolean KISS_opt[4];

#define MaxErrors 4

extern BOOL MinOnStart;

//RS TReedSolomon;
//  Form1 TForm1;
//  WaveFormat TWaveFormatEx;

extern int UDPServ;
extern long long udpServerSeqno;

extern int Channels;
extern int BitsPerSample;
extern float TX_Samplerate;
extern float RX_Samplerate;
extern int RX_SR;
extern int TX_SR;
extern int RX_PPM;
extern int TX_PPM;
extern int tx_bufsize;
extern int rx_bufsize;
extern int tx_bufcount;
extern int rx_bufcount;
extern int fft_size;
extern int  mouse_down[2];
//UCHAR * RX_pBuf array[257];
//  RX_header array[1..256] of TWaveHdr;
//  TX_pBuf array[1..4,1..256] of pointer;
//TX_header array[1..4,1..256] of TWaveHdr;
extern UCHAR calib_mode[5];
extern UCHAR snd_status[5];
extern UCHAR buf_status[5];
extern UCHAR tx_buf_num1[5];
extern UCHAR tx_buf_num[5];
extern int speed[5];
extern int panels[6];

extern int FFTSize;
#define fft_size FFTSize

extern float fft_window_arr[2048];
//  fft_s,fft_d array[0..2047] of TComplex;
extern short fft_buf[2][8192];
extern UCHAR fft_disp[2][1024];
//  bm array[1..4] of TBitMap;
//  bm1,bm2,bm3 TBitMap;

//  WaveInHandle hWaveIn;
//  WaveOutHandle array[1..4] of hWaveOut;
extern int RXBufferLength;

// data1 PData16;

extern int grid_time;
extern int fft_mult;
extern int fft_spd;
extern int grid_timer;
extern int stop_wf;
extern int raduga;
extern char snd_rx_device_name[32];
extern char snd_tx_device_name[32];
extern int snd_rx_device;
extern int snd_tx_device;
extern UCHAR mod_icon_status;
extern UCHAR last_mod_icon_status;
extern UCHAR icon_timer;
//  TelIni TIniFile;
extern char cur_dir[];
//  TimerId1 cardinal;
//  TimerId2 cardinal;
extern UCHAR TimerStat1;
extern UCHAR TimerStat2;
extern int stat_log;

extern char PTTPort[80];			// Port for Hardware PTT - may be same as control port.
extern int PTTMode;
extern int PTTBAUD ;

extern char PTTOnString[128];
extern char PTTOffString[128];

extern UCHAR PTTOnCmd[64];
extern UCHAR PTTOnCmdLen;

extern UCHAR PTTOffCmd[64];
extern UCHAR PTTOffCmdLen;

extern int PTT_device;
extern int RX_device;
extern int TX_device;
extern int TX_rotate;
extern int UsingLeft;
extern int UsingRight;
extern int UsingBothChannels;
extern int pttGPIOPin;
extern int pttGPIOPinR;
extern BOOL pttGPIOInvert;
extern BOOL useGPIO;
extern BOOL gotGPIO;
extern int VID;
extern int PID;
extern char CM108Addr[80];
extern int HamLibPort;
extern char HamLibHost[];

extern int SCO;
extern int DualPTT;
extern UCHAR  DebugMode;
extern UCHAR TimerEvent;
extern int nr_monitor_lines;
extern int UTC_Tim;
extern int MainPriority;
//  MainThreadHandle THandle;
extern UCHAR w_state;

extern BOOL Firstwaterfall;
extern BOOL Secondwaterfall;

extern int dcd_threshold;
extern int rxOffset;
extern int chanOffset[4];
extern int Continuation[4];	// Sending 2nd or more packet of burst

extern boolean busy;
extern boolean dcd[5];

extern struct TKISSMode_t  KISS;

extern boolean dyn_frack[4] ;
extern Byte recovery[4];
extern Byte users[4];

extern int resptime[4];
extern int slottime[4];
extern int persist[4];
extern int fracks[4];
extern int frack_time[4];
extern int idletime[4];
extern int redtime[4];
extern int IPOLL[4];
extern int maxframe[4];
extern int TXFrmMode[4];

extern char MyDigiCall[4][512];
extern char exclude_callsigns[4][512];
extern char exclude_APRS_frm[4][512];

extern TStringList  list_exclude_callsigns[4];
extern TStringList list_exclude_APRS_frm[4];
extern TStringList list_digi_callsigns[4];


extern int SoundIsPlaying;
extern int Capturing;

extern struct TDetector_t  DET[nr_emph + 1][16];

extern char CaptureDevice[80];
extern char PlaybackDevice[80];

extern TAX25Port AX25Port[4][port_num];

extern int fx25_mode[4];
extern int il2p_mode[4];

extern int tx_fx25_size[4];
extern int tx_fx25_size_cnt[4];
extern int tx_fx25_mode[4];

extern int SatelliteMode;

extern int using48000;			// Set if using 48K sample rate (ie RUH Modem active)

extern int txmin, txmax;

// Function prototypes

void KISS_send_ack(UCHAR port, string * data);
void AGW_AX25_frame_analiz(int snd_ch, int RX, string * frame);
void FIR_filter(float * src, unsigned short buf_size, unsigned short tap, float * core, float * dest, float * prev);
void make_core_TXBPF(UCHAR snd_ch, float freq, float width);
void OpenPTTPort();
void ClosePTTPort();

void RadioPTT(int snd_ch, BOOL PTTState);
void put_frame(int snd_ch, string * frame, char * code, int  tx_stat, int excluded);
void CloseCOMPort(int fd);
void COMClearRTS(int fd);
void COMClearDTR(int fd);
unsigned int getTicks();
char * ShortDateTime();
void  write_ax25_info(TAX25Port * AX25Sess);
void reverse_addr(Byte * path, Byte * revpath, int Len);
string * get_mycall(string * path);
TAX25Port * get_user_port_by_calls(int snd_ch, char *  CallFrom, char *  CallTo);
TAX25Port * get_free_port(int snd_ch);
void * in_list_incoming_mycall(Byte * path);
boolean add_incoming_mycalls(void * socket, char * src_call);
int get_addr(char * Calls, UCHAR * AXCalls);
void reverse_addr(Byte * path, Byte * revpath, int Len);
void set_link(TAX25Port * AX25Sess, UCHAR * axpath);
void rst_timer(TAX25Port * AX25Sess);
void set_unlink(TAX25Port * AX25Sess, Byte * path);
unsigned short get_fcs(UCHAR * Data, unsigned short len);
void KISSSendtoServer(void * sock, Byte * Msg, int Len);
int ConvFromAX25(unsigned char * incall, char * outcall);
BOOL ConvToAX25(char * callsign, unsigned char * ax25call);
void Debugprintf(const char * format, ...);

double pila(double x);

void AGW_Raw_monitor(int snd_ch, string * data);

// Delphi emulation functions

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

void move(UCHAR * SourcePointer, UCHAR * DestinationPointer, int CopyCount);
void fmove(float * SourcePointer, float * DestinationPointer, int CopyCount);

void setlength(string * Msg, int Count);		// Set string length

string * stringAdd(string * Msg, UCHAR * Chars, int Count);		// Extend string 

void Assign(TStringList * to, TStringList * from);	// Duplicate from to to

string * duplicateString(string * in);

// This looks for a string in a stringlist. Returns inhex if found, otherwise -1

int  my_indexof(TStringList * l, string * s);

boolean compareStrings(string * a, string * b);

int Add(TStringList * Q, string * Entry);


#define IL2P_SYNC_WORD_SIZE 3
#define IL2P_HEADER_SIZE 13	// Does not include 2 parity.
#define IL2P_HEADER_PARITY 2

#define IL2P_MAX_PAYLOAD_SIZE 1023
#define IL2P_MAX_PAYLOAD_BLOCKS 5
#define IL2P_MAX_PARITY_SYMBOLS 16		// For payload only.
#define IL2P_MAX_ENCODED_PAYLOAD_SIZE (IL2P_MAX_PAYLOAD_SIZE + IL2P_MAX_PAYLOAD_BLOCKS * IL2P_MAX_PARITY_SYMBOLS)

struct il2p_context_s {

	enum { IL2P_SEARCHING = 0, IL2P_HEADER, IL2P_PAYLOAD, IL2P_DECODE } state;

	unsigned int acc;	// Accumulate most recent 24 bits for sync word matching.
				// Lower 8 bits are also used for accumulating bytes for
				// the header and payload.

	int bc;			// Bit counter so we know when a complete byte has been accumulated.

	int polarity;		// 1 if opposite of expected polarity.

	unsigned char shdr[IL2P_HEADER_SIZE + IL2P_HEADER_PARITY];
	// Scrambled header as received over the radio.  Includes parity.
	int hc;			// Number if bytes placed in above.

	unsigned char uhdr[IL2P_HEADER_SIZE];  // Header after FEC and unscrambling.

	int eplen;		// Encoded payload length.  This is not the nuumber from
				// from the header but rather the number of encoded bytes to gather.

	unsigned char spayload[IL2P_MAX_ENCODED_PAYLOAD_SIZE];
	// Scrambled and encoded payload as received over the radio.
	int pc;			// Number of bytes placed in above.

	int corrected;		// Number of symbols corrected by RS FEC.
};

extern int NeedWaterfallHeaders;

#ifdef __cplusplus
}
#endif