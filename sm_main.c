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
newsamp

You should have received a copy of the GNU General Public License
along with QtSoundModem.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

#include "UZ7HOStuff.h"

void make_core_BPF(UCHAR snd_ch, short freq, short width);
void make_core_TXBPF(UCHAR snd_ch, float freq, float width);
void make_core_INTR(UCHAR snd_ch);
void make_core_LPF(UCHAR snd_ch, short width);
void wf_pointer(int snd_ch);
void dw9600ProcessSample(int snd_ch, short Sample);
void init_RUH48(int snd_ch);
void init_RUH96(int snd_ch);

char modes_name[modes_count][21] = 
{
	"AFSK AX.25 300bd","AFSK AX.25 1200bd","AFSK AX.25 600bd","AFSK AX.25 2400bd",
	"BPSK AX.25 1200bd","BPSK AX.25 600bd","BPSK AX.25 300bd","BPSK AX.25 2400bd",
	"QPSK AX.25 4800bd","QPSK AX.25 3600bd","QPSK AX.25 2400bd","BPSK FEC 4x100bd",
	"QPSK V26A 2400bps","8PSK V27 4800bps","QPSK V26B 2400bps", "Not Available",
	"QPSK V26A 600bps", "8PSK 900bps", "RUH 4800(DW)", "RUH 9600(DW)"
};

typedef struct wavehdr_tag {
	unsigned short *  lpData;   /* pointer to locked data buffer */
	int dwBufferLength;         /* length of data buffer */
	int dwBytesRecorded;        /* used for input only */
	int * dwUser;               /* for client's use */
	int dwFlags;                /* assorted flags (see defines) */
	int dwLoops;                /* loop control counter */
	struct wavehdr_tag *lpNext; /* reserved for driver */
	int *   reserved;           /* reserved for driver */
} WAVEHDR, *PWAVEHDR,  * NPWAVEHDR,  * LPWAVEHDR;

extern int pnt_change[5];
int debugmode = 0;
extern float src_buf[5][2048];
extern Byte RCVR[5];

int SatelliteMode = 0;

int UDPServerPort = 8884;
int UDPClientPort = 8888;
int TXPort = 8884;

BOOL Firstwaterfall = 1;
BOOL Secondwaterfall = 1;

int multiCore = FALSE;



BOOL MinOnStart  =  0;
//RS TReedSolomon;
//  Form1 TForm1;
//  WaveFormat TWaveFormatEx;
int Channels = 2;
int BitsPerSample = 16;
float TX_Samplerate = 12000;
float RX_Samplerate = 12000;
int RX_SR = 11025;
int TX_SR = 11025;
int RX_PPM = 0;
int TX_PPM = 0;
int tx_bufsize = 512;
int rx_bufsize = 512;
int tx_bufcount = 16;
int rx_bufcount = 16;
int  mouse_down[2] = {0, 0};
//UCHAR * RX_pBuf array[257];
//  RX_header array[1..256] of TWaveHdr;
//  TX_pBuf array[1..4,1..256] of pointer;
//TX_header array[1..4,1..256] of TWaveHdr;
UCHAR calib_mode[5] = {0,0,0,0};
UCHAR snd_status [5] = {0,0,0,0};
UCHAR buf_status [5]  = {0,0,0,0};
UCHAR tx_buf_num1 [5] = {0,0,0,0};
UCHAR tx_buf_num [5] = {0,0,0,0};

extern short active_rx_freq[5];

unsigned int pskStates[4] = {0, 0, 0, 0};	

int speed[5] = {0,0,0,0};
int panels[6] = {1,1,1,1,1};

short fft_buf[2][8192];
UCHAR fft_disp[2][1024];
int fftCount = 0;			// FTF samples collected

//  bm array[1..4] of TBitMap;
//  bm1,bm2,bm3 TBitMap;

//  WaveInHandle hWaveIn;
//  WaveOutHandle array[1..4] of hWaveOut;
int RXBufferLength;

int grid_time = 0;
int fft_mult = 0;
int fft_spd = 3;
int grid_timer = 0;
int stop_wf  = 0;
int raduga = DISP_RGB;
char snd_rx_device_name[32] = "";
char snd_tx_device_name[32] = "";
int snd_rx_device = 0;
int snd_tx_device = 0;
UCHAR mod_icon_status = MOD_IDLE;
UCHAR last_mod_icon_status = MOD_IDLE;
UCHAR icon_timer = 0;
//  TelIni TIniFile;
char cur_dir[] = "";
//  TimerId1 cardinal;
//  TimerId2 cardinal;
UCHAR TimerStat1 = TIMER_FREE;
UCHAR TimerStat2 = TIMER_FREE;
int stat_log = FALSE;

int PTT_device = FALSE;
int RX_device = FALSE;
int TX_device = FALSE;
int TX_rotate = FALSE;
int UsingBothChannels = FALSE;
int UsingLeft = FALSE;
int UsingRight = FALSE;

int SCO = FALSE;
int DualPTT = TRUE;
UCHAR  DebugMode = 0;
UCHAR TimerEvent = TIMER_EVENT_OFF;
int nr_monitor_lines = 50;
int UTC_Time = FALSE;
int MainPriority = 0;
//  MainThreadHandle THandle;
UCHAR w_state = WIN_MAXIMIZED;
 
 
void get_filter_values(UCHAR snd_ch)
{
	//, unsigned short dbpf,
//unsigned short dtxbpf,
//unsigned short dbpftap,
//unsigned short dlpf, 
//unsigned short dlpftap)
//	speed[snd_ch], bpf[snd_ch], txbpf[snd_ch], bpf_tap[snd_ch], lpf[snd_ch], lpf_tap[snd_ch]);

	switch (speed[snd_ch])
	{
	case SPEED_8P4800:

		lpf[snd_ch] = MODEM_8P4800_LPF;
		bpf[snd_ch] = MODEM_8P4800_BPF;
		txbpf[snd_ch] = MODEM_8P4800_TXBPF;
		BPF_tap[snd_ch] = MODEM_8P4800_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_8P4800_LPF_TAP;
		break;

	case SPEED_MP400:

		lpf[snd_ch] = MODEM_MP400_LPF;
		bpf[snd_ch] = MODEM_MP400_BPF;
		txbpf[snd_ch] = MODEM_MP400_TXBPF;
		BPF_tap[snd_ch] = MODEM_MP400_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_MP400_LPF_TAP;

		break;


	case SPEED_Q4800:

		lpf[snd_ch] = MODEM_Q4800_LPF;
		bpf[snd_ch] = MODEM_Q4800_BPF;
		txbpf[snd_ch] = MODEM_Q4800_TXBPF;
		BPF_tap[snd_ch] = MODEM_Q4800_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_Q4800_LPF_TAP;

		break;

	case SPEED_Q3600:

		lpf[snd_ch] = MODEM_Q3600_LPF;
		bpf[snd_ch] = MODEM_Q3600_BPF;
		txbpf[snd_ch] = MODEM_Q3600_TXBPF;
		BPF_tap[snd_ch] = MODEM_Q3600_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_Q3600_LPF_TAP;
		break;

	case SPEED_Q2400:

		lpf[snd_ch] = MODEM_Q2400_LPF;
		bpf[snd_ch] = MODEM_Q2400_BPF;
		txbpf[snd_ch] = MODEM_Q2400_TXBPF;
		BPF_tap[snd_ch] = MODEM_Q2400_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_Q2400_LPF_TAP;

		break;

	case SPEED_DW2400:
	case SPEED_2400V26B:


		lpf[snd_ch] = MODEM_DW2400_LPF;
		bpf[snd_ch] = MODEM_DW2400_BPF;
		txbpf[snd_ch] = MODEM_DW2400_TXBPF;
		BPF_tap[snd_ch] = MODEM_DW2400_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_DW2400_LPF_TAP;
		break;

	case SPEED_P2400:

		lpf[snd_ch] = MODEM_P2400_LPF;
		bpf[snd_ch] = MODEM_P2400_BPF;
		txbpf[snd_ch] = MODEM_P2400_TXBPF;
		BPF_tap[snd_ch] = MODEM_P2400_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P2400_LPF_TAP;
		break;

	case SPEED_P1200:

		lpf[snd_ch] = MODEM_P1200_LPF;
		bpf[snd_ch] = MODEM_P1200_BPF;
		txbpf[snd_ch] = MODEM_P1200_TXBPF;
		BPF_tap[snd_ch] = MODEM_P1200_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P1200_LPF_TAP;
		break;

	case SPEED_P600:

		lpf[snd_ch] = MODEM_P600_LPF;
		bpf[snd_ch] = MODEM_P600_BPF;
		txbpf[snd_ch] = MODEM_P600_TXBPF;
		BPF_tap[snd_ch] = MODEM_P600_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P600_LPF_TAP;
		break;

	case SPEED_P300:

		lpf[snd_ch] = MODEM_P300_LPF;
		bpf[snd_ch] = MODEM_P300_BPF;
		txbpf[snd_ch] = MODEM_P300_TXBPF;
		BPF_tap[snd_ch] = MODEM_P300_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P300_LPF_TAP;
		break;

	case SPEED_300:

		lpf[snd_ch] = MODEM_300_LPF;
		bpf[snd_ch] = MODEM_300_BPF;
		txbpf[snd_ch] = MODEM_300_TXBPF;
		BPF_tap[snd_ch] = MODEM_300_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_300_LPF_TAP;

		break;

	case SPEED_600:

		lpf[snd_ch] = MODEM_600_LPF;
		bpf[snd_ch] = MODEM_600_BPF;
		txbpf[snd_ch] = MODEM_600_TXBPF;
		BPF_tap[snd_ch] = MODEM_600_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_600_LPF_TAP;

		break;

	case SPEED_1200:

		lpf[snd_ch] = MODEM_1200_LPF;
		bpf[snd_ch] = MODEM_1200_BPF;
		txbpf[snd_ch] = MODEM_1200_TXBPF;
		BPF_tap[snd_ch] = MODEM_1200_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_1200_LPF_TAP;
		break;

	case SPEED_2400:

		lpf[snd_ch] = MODEM_2400_LPF;
		bpf[snd_ch] = MODEM_2400_BPF;
		txbpf[snd_ch] = MODEM_2400_TXBPF;
		BPF_tap[snd_ch] = MODEM_2400_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_2400_LPF_TAP;
		break;


	case SPEED_Q300:
	case SPEED_8PSK300:

		lpf[snd_ch] = MODEM_P300_LPF;
		bpf[snd_ch] = MODEM_P300_BPF;
		txbpf[snd_ch] = MODEM_P300_TXBPF;
		BPF_tap[snd_ch] = MODEM_P300_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P300_LPF_TAP;

		break;

/*

	case SPEED_Q1200:

		lpf[snd_ch] = MODEM_P1200_LPF;
		bpf[snd_ch] = MODEM_P1200_BPF;
		txbpf[snd_ch] = MODEM_P1200_TXBPF;
		BPF_tap[snd_ch] = MODEM_P1200_BPF_TAP;
		LPF_tap[snd_ch] = MODEM_P1200_LPF_TAP;
		break;
*/
	}
}


void init_2400(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 1805;
	rx_baudrate[snd_ch] = 2400;
	tx_bitrate[snd_ch] = 2400;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_1200(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 1000;

	if (stdtones)
		rx_freq[snd_ch] = 1700;

	rx_baudrate[snd_ch] = 1200;
	tx_bitrate[snd_ch] = 1200;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_600(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 450;

	rx_baudrate[snd_ch] = 600;
	tx_bitrate[snd_ch] = 600;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_300(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 200;
	rx_baudrate[snd_ch] = 300;
	tx_bitrate[snd_ch] = 300;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_MP400(int snd_ch)
{
	modem_mode[snd_ch] = MODE_MPSK;
	rx_shift[snd_ch] = 175 /*sbc*/ * 3;
	rx_baudrate[snd_ch] = 100;
	tx_bitrate[snd_ch] = 400;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}


void init_8P4800(int snd_ch)
{
	modem_mode[snd_ch] = MODE_8PSK;
	if (stdtones)
		rx_freq[snd_ch] = 1800;

	rx_shift[snd_ch] = 1600;
	rx_baudrate[snd_ch] = 1600;
	tx_bitrate[snd_ch] = 4800;
	pskStates[snd_ch] = 8;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_V26B2400(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_V26;
	modem_mode[snd_ch] = MODE_PI4QPSK;

	if (stdtones)
		rx_freq[snd_ch] = 1800;

	rx_shift[snd_ch] = 1200;
	rx_baudrate[snd_ch] = 1200;
	tx_bitrate[snd_ch] = 2400;
	pskStates[snd_ch] = 8;			// Pretend 8 so quality calc works

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_DW2400(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_V26;
	modem_mode[snd_ch] = MODE_QPSK;

	if (stdtones)
		rx_freq[snd_ch] = 1800;

	rx_shift[snd_ch] = 1200;
	rx_baudrate[snd_ch] = 1200;
	tx_bitrate[snd_ch] = 2400;
	pskStates[snd_ch] = 4;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q4800(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_SM;
	modem_mode[snd_ch] = MODE_QPSK;
	rx_shift[snd_ch] = 2400;
	rx_baudrate[snd_ch] = 2400;
	tx_bitrate[snd_ch] = 4800;
	pskStates[snd_ch] = 4;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q3600(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_SM;
	modem_mode[snd_ch] = MODE_QPSK;
	rx_shift[snd_ch] = 1800;
	rx_baudrate[snd_ch] = 1800;
	tx_bitrate[snd_ch] = 3600;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q2400(int snd_ch)
{
  qpsk_set[snd_ch].mode = QPSK_SM;
  modem_mode[snd_ch] = MODE_QPSK;
  rx_shift[snd_ch] = 1200;
  rx_baudrate[snd_ch] = 1200;
  tx_bitrate[snd_ch] = 2400;
  pskStates[snd_ch] = 4;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P2400(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 2400;
  rx_baudrate[snd_ch] = 2400;
  tx_bitrate[snd_ch] = 2400;
  pskStates[snd_ch] = 2;
  
  if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P1200(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 1200;
  rx_baudrate[snd_ch] = 1200;
  tx_bitrate[snd_ch] = 1200;
  pskStates[snd_ch] = 2;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P600(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 600;
  rx_baudrate[snd_ch] = 600;
  tx_bitrate[snd_ch] = 600;
  pskStates[snd_ch] = 2;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P300(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 300;
  rx_baudrate[snd_ch] = 300;
  pskStates[snd_ch] = 2;
  tx_bitrate[snd_ch] = 300;

  if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q300(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_V26;
	modem_mode[snd_ch] = MODE_QPSK;

	rx_shift[snd_ch] = 300;
	rx_baudrate[snd_ch] = 300;
	tx_bitrate[snd_ch] = 600;
	pskStates[snd_ch] = 4;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_8PSK300(int snd_ch)
{
	modem_mode[snd_ch] = MODE_8PSK;

	rx_shift[snd_ch] = 300;
	rx_baudrate[snd_ch] = 300;
	tx_bitrate[snd_ch] = 900;
	pskStates[snd_ch] = 8;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_ARDOP(int snd_ch)
{
	modem_mode[snd_ch] = MODE_ARDOP;
	rx_shift[snd_ch] = 500;
	rx_freq[snd_ch] = 1500;
	rx_baudrate[snd_ch] = 500;
	tx_bitrate[snd_ch] = 500;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}


void init_speed(int snd_ch);

void set_speed(int snd_ch, int Modem)
{
	speed[snd_ch] = Modem;

	init_speed(snd_ch);

}

int needPSKRefresh = 0;

void init_speed(int snd_ch)
{
	int low, high;

	pskStates[snd_ch] = 0;		// Not PSK

	switch (speed[snd_ch])
	{
	case SPEED_8P4800:
		init_8P4800(snd_ch);
		break;

	case SPEED_2400V26B:
		init_V26B2400(snd_ch);

		break;

	case SPEED_DW2400:
		init_DW2400(snd_ch);
		break;

	case SPEED_MP400:
		init_MP400(snd_ch);
		break;
	case SPEED_Q4800:
		init_Q4800(snd_ch);
		break;

	case SPEED_Q3600:
		init_Q3600(snd_ch);
		break;

	case SPEED_Q2400:
		init_Q2400(snd_ch);
		break;

	case SPEED_P2400:
		init_P2400(snd_ch);
		break;

	case SPEED_P1200:
		init_P1200(snd_ch);
		break;

//	case SPEED_Q1200:
//		init_Q1200(snd_ch);
//		break;

	case SPEED_P600:
		init_P600(snd_ch);
		break;

	case SPEED_P300:
		init_P300(snd_ch);
		break;

	case SPEED_Q300:
		init_Q300(snd_ch);
		break;

	case SPEED_8PSK300:
		init_8PSK300(snd_ch);
		break;

	case SPEED_300:

		init_300(snd_ch);
		break;

	case SPEED_600:

		init_600(snd_ch);
		break;

	case SPEED_1200:

		init_1200(snd_ch);
		break;

	case SPEED_2400:

		init_2400(snd_ch);
		break;

	case SPEED_ARDOP:

		init_ARDOP(snd_ch);
		break;

	case SPEED_RUH48:

		init_RUH48(snd_ch);
		break;

	case SPEED_RUH96:

		init_RUH96(snd_ch);
		break;

	}

	//QPSK_SM: begin move(#0#1#2#3, tx[0], 4); move(#0#32#64#96, rx[0], 4); end;
	//QPSK_V26: begin move(#2#3#1#0, tx[0], 4); move(#96#64#0#32, rx[0], 4); end;

	if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_PI4QPSK)
	{
		switch (qpsk_set[snd_ch].mode)
		{
		case QPSK_SM:

			memcpy(&qpsk_set[snd_ch].tx[0], "\0\1\2\3", 4);
			memcpy(&qpsk_set[snd_ch].rx[0], "\x0\x20\x40\x60", 4);
			break;

		case QPSK_V26:

			memcpy(&qpsk_set[snd_ch].tx[0], "\2\3\1\0", 4);
			memcpy(&qpsk_set[snd_ch].rx[0], "\x60\x40\0\x20", 4);
			break;
		}
	}

	tx_shift[snd_ch] = rx_shift[snd_ch];
	tx_baudrate[snd_ch] = rx_baudrate[snd_ch];
	low = roundf(rx_shift[snd_ch] / 2 + RCVR[snd_ch] * rcvr_offset[snd_ch] + 1);
	high = roundf(RX_Samplerate / 2 - (rx_shift[snd_ch] / 2 + RCVR[snd_ch] * rcvr_offset[snd_ch]));

	if (rx_freq[snd_ch] - low < 0)  rx_freq[snd_ch] = low;
	if (high - rx_freq[snd_ch] < 0) rx_freq[snd_ch] = high;

	tx_freq[snd_ch] = rx_freq[snd_ch];

	make_core_BPF(snd_ch, rx_freq[snd_ch], bpf[snd_ch]);
	make_core_TXBPF(snd_ch, tx_freq[snd_ch], txbpf[snd_ch]);
	make_core_INTR(snd_ch);
	make_core_LPF(snd_ch, lpf[snd_ch]);

	/*
	  for i = 0 to 16 do
		for j = 0 to nr_emph do with DET[j,i] do
		begin
		  minamp[snd_ch] = 0;
		  maxamp[snd_ch] = 0;
		  ones[snd_ch] = 0;
		  zeros[snd_ch] = 0;
		  sample_cnt[snd_ch] = 0;
		  bit_cnt[snd_ch] = 0;
		  bit_osc[snd_ch] = 0;
		  frame_status[snd_ch] = FRAME_WAIT;
		end;
	  form1.show_modes;
	  form1.show_freq_a;
	  form1.show_freq_b;
	  */
	wf_pointer(snd_ch);

	CheckPSKWindows();
}


void  chk_snd_buf(float * buf, int len)
{
	word i;
	boolean  good;
	single prev_amp;

	if (len < 2)
		return;

	good = FALSE;
	i = 1;
	prev_amp = buf[0];
	do
	{
		if (buf[i++] != prev_amp)
			good = TRUE;

	} while (good == FALSE && i < len);

	// Make noise
	if (!good)
		for (i = 0; i < len; i++)
			buf[i] = rand();
}

#ifdef WIN32

typedef void *HANDLE;
typedef unsigned long DWORD;

#define WINAPI __stdcall
__declspec(dllimport)
DWORD
WINAPI
WaitForSingleObject(
	__in HANDLE hHandle,
	__in DWORD dwMilliseconds
);




#define pthread_t uintptr_t

uintptr_t _beginthread(void(__cdecl *start_address)(void *), unsigned stack_size, void *arglist);
#else

#include <pthread.h>

pthread_t _beginthread(void(*start_address)(void *), unsigned stack_size, void * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*)arglist) != 0)
		perror("New Thread");

	return thread;
}

#endif

void runModemthread(void * param);

void runModems()
{
	int snd_ch, res;
	pthread_t thread[4] = { 0,0,0,0 };

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		if (soundChannel[snd_ch] == 0)				// Unused channed
			continue;	
	
		if (modem_mode[snd_ch] == MODE_ARDOP)
			continue;			// Processed above

		if (modem_mode[snd_ch] == MODE_RUH)
			continue;			// Processed above

		// do we need to do this again ??
		// make_core_BPF(snd_ch, rx_freq[snd_ch], bpf[snd_ch]);

		if (multiCore)			// Run modems in separate threads
			thread[snd_ch] = _beginthread(runModemthread, 0, (void *)(size_t)snd_ch);
		else
			runModemthread((void *)(size_t)snd_ch);
	}

	if (multiCore)
	{
#ifdef WIN32
		if (thread[0]) WaitForSingleObject(&thread[0], 2000);
		if (thread[1]) WaitForSingleObject(&thread[1], 2000);
		if (thread[2]) WaitForSingleObject(&thread[2], 2000);
		if (thread[3]) WaitForSingleObject(&thread[3], 2000);
#else
		if (thread[0]) pthread_join(thread[0], &res);
		if (thread[1]) pthread_join(thread[1], &res);
		if (thread[2]) pthread_join(thread[2], &res);
		if (thread[3]) pthread_join(thread[3], &res);
#endif
	}
}

Byte rcvr_idx;

void runModemthread(void * param)
{
	int snd_ch = (int)(size_t)param;

	// I want to run lowest to highest to simplify my display 

	int offset = -(RCVR[snd_ch] * rcvr_offset[snd_ch]); // lowest
	int lastrx = RCVR[snd_ch] * 2;

	if (soundChannel[snd_ch] == 0)				// Unused channed
		return;

	for (rcvr_idx = 0; rcvr_idx <= lastrx; rcvr_idx++)
	{
		active_rx_freq[snd_ch] = rxOffset + chanOffset[snd_ch] + rx_freq[snd_ch] + offset;
		offset += rcvr_offset[snd_ch];

		Demodulator(snd_ch, rcvr_idx, src_buf[modemtoSoundLR[snd_ch]], rcvr_idx == lastrx, offset == 0);
	}
}

// I think this processes a buffer of samples

void BufferFull(short * Samples, int nSamples)			// These are Stereo Samples
{
	word i, i1, j;
	Byte snd_ch, rcvr_idx;
	int buf_offset;
	int Needed;
	short * data1;
	short * data2 = 0;

	// if UDP server active send as UDP Datagram

	if (UDPServ)	// Extract just left
	{
		short Buff[1024];

		i1 = 0;

		for (i = 0; i < rx_bufsize; i++)
		{
			Buff[i] = Samples[i1];
			i1 += 2;
		}

		sendSamplestoUDP(Buff, 512, TXPort);
	}

	// Do RSID processing (can we also use this for waterfall??

	RSIDProcessSamples(Samples, nSamples);

	// Do FFT on every 4th buffer (2048 samples)

	// if in Satellite Mode look for a Tuning signal

//	if (SatelliteMode)
//	{
//		doTuning(Samples, nSamples);
//	}

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		if (soundChannel[snd_ch] == 0)				// Unused channed
			continue;

		if (pnt_change[snd_ch])
		{
			make_core_BPF(snd_ch, rx_freq[snd_ch], bpf[snd_ch]);
			make_core_TXBPF(snd_ch, tx_freq[snd_ch], txbpf[snd_ch]);
			pnt_change[snd_ch] = FALSE;
		}

	}

	// I don't think we should process RX if either is sending

	if (snd_status[0] != SND_TX && snd_status[1] != SND_TX && snd_status[2] != SND_TX && snd_status[3] != SND_TX)
	{
		for (snd_ch = 0; snd_ch < 4; snd_ch++)
		{
			if (soundChannel[snd_ch] == 0)				// Unused channed
				continue;

			if (modem_mode[snd_ch] == MODE_ARDOP)
			{
				short ardopbuff[1200];
				i1 = 0;

				if (using48000)
				{
					i1 = 0;
					j = 0;

					//	Need to downsample 48K to 12K
					//	Try just skipping 3 samples	

					nSamples /= 4;

					for (i = 0; i < nSamples; i++)
					{
						ardopbuff[i] = Samples[i1];		
						i1 += 8;
					}
				}
				else
				{
					for (i = 0; i < nSamples; i++)
					{
						ardopbuff[i] = Samples[i1];
						i1++;
						i1++;
					}
				}
				ARDOPProcessNewSamples(snd_ch, ardopbuff, nSamples);
			}

			else if (modem_mode[snd_ch] == MODE_RUH)
			{
				i1 = 0;
				if (modemtoSoundLR[snd_ch] == 1)		// Using Right Chan
					i1++;			

				for (i = 0; i < nSamples; i++)
				{
					dw9600ProcessSample(snd_ch, Samples[i1]);
					i1++;
					i1++;
				}
			}
			ProcessRXFrames(snd_ch);
		}

		// extract mono samples from data. 

		data1 = Samples;

		if (using48000)
		{
			i1 = 0;
			j = 0;

			//	Need to downsample 48K to 12K
			//	Try just skipping 3 samples	

			nSamples /= 4;

			for (i = 0; i < nSamples; i++)
			{
				Samples[j++] = Samples[i1];
				Samples[j++] = Samples[i1 + 1];
				i1 += 8;
			}
		}

		i1 = 0;

		// src_buf[0] is left data,. src_buf[1] right

		// We could skip extracting other channel if only one in use - is it worth it??

		if (UsingBothChannels)
		{
			for (i = 0; i < nSamples; i++)
			{
				src_buf[0][i] = data1[i1];
				i1++;
				src_buf[1][i] = data1[i1];
				i1++;
			}
		}
		else if (UsingRight)
		{
			// Extract just right

			i1 = 1;

			for (i = 0; i < nSamples; i++)
			{
				src_buf[1][i] = data1[i1];
				i1 += 2;
			}
		}
		else
		{
			// Extract just left

			for (i = 0; i < nSamples; i++)
			{
				src_buf[0][i] = data1[i1];
				i1 += 2;
			}
		}

		// Run modems before waterfall so fft buffer has values from before sync was detected

		runModems();

		// Do whichever waterfall is needed

		// We need to run the waterfall FFT for the frequency guessing to work

		int FirstWaterfallChan = 0;
		short * ptr1 = &fft_buf[0][fftCount];
		short * ptr2 = &fft_buf[1][fftCount];

		int remainingSamples = rx_bufsize;

		if (UsingLeft == 0)
		{
			FirstWaterfallChan = 1;
			data1++;					// to Right Samples
		}

		if (UsingBothChannels)			// Second is always Right
			data2 = &Samples[1];		// to Right Samples


		// FFT size isn't necessarily a multiple of rx_bufsize, so this is a bit more complicated
		// Save FFTSize samples then process. Put the unused bits back in the fft buffer

		// Collect samples for both channels if needed

		Needed = FFTSize - fftCount;

		if (Needed <= rx_bufsize)
		{
			// add Needed samples to fft_buf and process. Copy rest to start of fft_buf

			for (i = 0; i < Needed; i++)
			{
				*ptr1++ = *data1;
				data1 += 2;
			}

			doWaterfall(FirstWaterfallChan);

			if (data2)
			{
				for (i = 0; i < Needed; i++)
				{
					*ptr2++ = *data2;
					data2 += 2;
				}
				doWaterfall(1);
			}

			remainingSamples = rx_bufsize - Needed;
			fftCount = 0;

			ptr1 = &fft_buf[0][0];
			ptr2 = &fft_buf[1][0];
		}

		for (i = 0; i < remainingSamples; i++)
		{
			*ptr1++ = *data1;
			data1 += 2;
		}

		if (data2)
		{
			for (i = 0; i < remainingSamples; i++)
			{
				*ptr2++ = *data2;
				data2 += 2;
			}
		}
		fftCount += remainingSamples;

	}


	if (TimerEvent == TIMER_EVENT_ON)
	{
		timer_event();
//		timer_event2();
	}
}

		/*

procedure TForm1.BufferFull1(var Msg TMessage);
var
  i,snd_ch byte;
begin
  for snd_ch = 1 to 2 do
    if pnt_change[snd_ch] then
    begin
      make_core_BPF(snd_ch,rx_freq[snd_ch],bpf[snd_ch]);
      make_core_TXBPF(snd_ch,tx_freq[snd_ch],txbpf[snd_ch]);
      pnt_change[snd_ch] = FALSE;
    end;
  snd_ch = 0;
  for i = 1 to 2 do if msg.WParam = WaveOutHandle[i] then snd_ch = i;
  if (snd_ch = 0) then exit;
  if (snd_status[snd_ch]<>SND_TX) then exit;
  inc(tx_buf_num[snd_ch]); if tx_buf_num[snd_ch]>tx_bufcount then tx_buf_num[snd_ch] = 1;
  if (buf_status[snd_ch] = BUF_EMPTY) and (tx_buf_num[snd_ch] = tx_buf_num1[snd_ch]) then TX2RX(snd_ch);
  if buf_status[snd_ch] = BUF_FULL then
  beginf
    make_wave_buf(snd_ch,TX_pbuf[snd_ch][tx_buf_num[snd_ch]]);
    WaveOutWrite(WaveOutHandle[snd_ch],@TX_header[snd_ch][tx_buf_num[snd_ch]],sizeof(TWaveHdr));
    inc(tx_buf_num1[snd_ch]); if tx_buf_num1[snd_ch]>tx_bufcount then tx_buf_num1[snd_ch] = 1;
  end;
end;

procedure TForm1.TX2RX(snd_ch byte);
begin
  if snd_status[snd_ch] = SND_TX then stoptx(snd_ch);
  if snd_status[snd_ch] = SND_IDLE then begin pttoff(snd_ch); end;
end;
*/


// Monitor Code - from moncode.asm


#define	CMDBIT	4		// CURRENT MESSAGE IS A COMMAND
#define	RESP 2		// CURRENT MSG IS RESPONSE
#define	VER1 1 		// CURRENT MSG IS VERSION 1


#define	UI	3
#define	SABM 0x2F
#define	DISC 0x43
#define	DM	0x0F
#define	UA	0x63
#define	FRMR 0x87
#define	RR	1
#define	RNR	5
#define	REJ	9

#define	SREJ 0x0D
#define SABME 0x6F
#define XID 0xAF
#define TEST 0xE3


#define	PFBIT 0x10		// POLL/FINAL BIT IN CONTROL BYTE

#define	NETROM_PID 0xCF
#define	IP_PID 0xCC
#define	ARP_PID 0xCD

#define	NODES_SIG	0xFF

char FrameData[1024] = "";

char * frame_monitor(string * frame, char * code, int tx_stat)
{
	char mon_frm[512];
	char AGW_path[256];
	string * AGW_data;

	const Byte * frm = "???";
	Byte * datap;
	Byte _data[512] = "";
	Byte * p_data = _data;
	int _datalen;

	char  agw_port;
	char  CallFrom[10], CallTo[10], Digi[80];

	char TR = 'R';
	char codestr[64] = "";

	integer i;
	char  time_now[32];
	int len;

	AGWUser * AGW;

	Byte pid, nr, ns, f_type, f_id;
	Byte  rpt, cr, pf;
	Byte path[80];
	char c;
	const char * p;

	string * data = newString();

	if (code[0] && strlen(code) < 60)
		sprintf(codestr, "[%s]", code);

	if (tx_stat)
		TR = 'T';

	decode_frame(frame->Data, frame->Length, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

	datap = data->Data;

	len = data->Length;

	//	if (pid == 0xCF)
	//		data = parse_NETROM(data, f_id);
		// IP parsing
	//	else if (pid == 0xCC)
	//		data = parse_IP(data);
		// ARP parsing
	//	else if (pid == 0xCD)
	//		data = parse_ARP(data);
		//

	if (len > 0)
	{
		for (i = 0; i < len; i++)
		{
			if (datap[i] > 31 || datap[i] == 13 || datap[i] == 9)
				*(p_data++) = datap[i];
		}
	}

	_datalen = p_data - _data;

	if (_datalen)
	{
		Byte * ptr = _data;
		i = 0;

		// remove successive cr or cr on end		while (i < _datalen)

		while (i < _datalen)
		{
			if ((_data[i] == 13) && (_data[i + 1] == 13))
				i++;
			else
				*(ptr++) = _data[i++];
		}

		if (*(ptr - 1) == 13)
			ptr--;

		*ptr = 0;

		_datalen = ptr - _data;
	}

	get_monitor_path(path, CallTo, CallFrom, Digi);

	if (cr)
	{
		c = 'C';
		if (pf)
			p = " P";
		else p = "";
	}
	else
	{
		c = 'R';
		if (pf)
			p = " F";
		else
			p = "";
	}

	switch (f_id)
	{
	case I_I:

		frm = "I";
		break;

	case S_RR:

		frm = "RR";
		break;

	case S_RNR:

		frm = "RNR";
		break;

	case S_REJ:

		frm = "REJ";
		break;

	case S_SREJ:

		frm = "SREJ";
		break;

	case U_SABM:

		frm = "SABM";
		break;

	case SABME:

		frm = "SABME";
		break;

	case U_DISC:

		frm = "DISC";
		break;

	case U_DM:

		frm = "DM";
		break;

	case U_UA:

		frm = "UA";
		break;

	case U_FRMR:

		frm = "FRMR";
		break;

	case U_UI:

		frm = "UI";
		break;

	case U_XID:

		frm = "XID";
		break;

	case U_TEST:

		frm = "TEST";
	}

	if (Digi[0])
		sprintf(AGW_path, "Fm %s To %s Via %s <%s %c%s",CallFrom, CallTo, Digi, frm, c, p);
	else
		sprintf(AGW_path, "Fm %s To %s <%s %c %s", CallFrom, CallTo, frm, c, p);


	switch (f_type)
	{
	case I_FRM:

		//mon_frm = AGW_path + ctrl + ' R' + inttostr(nr) + ' S' + inttostr(ns) + ' pid=' + dec2hex(pid) + ' Len=' + inttostr(len) + ' >' + time_now + #13 + _data + #13#13;
		sprintf(mon_frm, "%s R%d S%d pid=%X Len=%d>[%s%c]%s\r%s\r", AGW_path, nr, ns, pid, len, ShortDateTime(), TR, codestr, _data);

		break;

	case U_FRM:

		if (f_id == U_UI)
		{
			sprintf(mon_frm, "%s pid=%X Len=%d>[%s%c]%s\r%s\r", AGW_path, pid, len, ShortDateTime(), TR, codestr, _data); // "= AGW_path + ctrl + '>' + time_now + #13;
		}
		else if (f_id == U_FRMR)
		{
			sprintf(mon_frm, "%s>%02x %02x %02x[%s]\r", AGW_path, datap[0], datap[1], datap[2], ShortDateTime()); // "= AGW_path + ctrl + '>' + time_now + #13;
		}
		else
			sprintf(mon_frm, "%s>[%s%c]%s\r", AGW_path, ShortDateTime(), TR, codestr); // "= AGW_path + ctrl + '>' + time_now + #13;

		break;

	case S_FRM:

		//		mon_frm = AGW_path + ctrl + ' R' + inttostr(nr) + ' >' + time_now + #13;
		sprintf(mon_frm, "%s R%d>[%s%c]%s\r", AGW_path, nr, ShortDateTime(), TR, codestr); // "= AGW_path + ctrl + '>' + time_now + #13;

		break;

	}
	sprintf(FrameData, "%s", mon_frm);
	return FrameData;
}

