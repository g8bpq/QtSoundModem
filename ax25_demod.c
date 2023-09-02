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

extern int blnBusyStatus;
extern word MEMRecovery[5];

void  make_rx_frame_FX25(int snd_ch, int rcvr_nr, int emph, string * data);
string * memory_ARQ(TStringList * buf, string * data);

float GuessCentreFreq(int i);
void ProcessRXFrames(int snd_ch);

extern struct il2p_context_s *il2p_context[4][16][3];


/*

unit ax25_demod;

interface

uses math,sysutils,Graphics,classes;

  procedure detector_init;
  procedure detector_free;
  procedure Mux3(snd_ch,rcvr_nr,emph: byte; src1,core: array of single; var dest,prevI,prevQ: array of single; tap,buf_size: word);
  procedure Mux3_PSK(snd_ch,rcvr_nr,emph: byte; src1,core: array of single; var destI,destQ,prevI,prevQ: array of single; tap,buf_size: word);
  procedure make_core_intr(snd_ch: byte);
  procedure make_core_LPF(snd_ch: byte; width: single);
  procedure make_core_BPF(snd_ch: byte; freq,width: single);
  procedure make_core_TXBPF(snd_ch: byte; freq,width: single);
  procedure init_BPF(freq1,freq2: single; tap: word; samplerate: single; var buf: array of single);
  procedure FIR_filter(src: array of single; buf_size,tap: word; core: array of single; var dest,prev: array of single);
  procedure Demodulator(snd_ch,rcvr_nr: byte; src_buf: array of single; last: boolean);
  function memory_ARQ(buf: TStringList; data: string): string;

type TSurvivor = record
    BitEstimates: int64;
    Pathdistance: integer;
}

type TMChannel = record
  prev_LPF1I_buf : array [0..4095] of single;
  prev_LPF1Q_buf : array [0..4095] of single;
  prev_dLPFI_buf : array [0..4095] of single;
  prev_dLPFQ_buf : array [0..4095] of single;
  prev_AFCI_buf  : array [0..4095] of single;
  prev_AFCQ_buf  : array [0..4095] of single;
  AngleCorr      : single;
  MUX_osc        : single;
  AFC_IZ1        : single;
  AFC_IZ2        : single;
  AFC_QZ1        : single;
  AFC_QZ2        : single;
  AFC_bit_buf1I  : array [0..1023] of single;
  AFC_bit_buf1Q  : array [0..1023] of single;
  AFC_bit_buf2   : array [0..1023] of single;
  AFC_IIZ1       : single;
  AFC_QQZ1       : single;
}
*/
 
 
#define sbc 175
  
single  ch_offset[4] = { -sbc * 1.5,-sbc * 0.5,sbc*0.5,sbc*1.5 };



float PI125 = 0.125f * M_PI;
float PI375 = 0.375f * M_PI;
float PI625 = 0.625f * M_PI;
float PI875 = 0.875f * M_PI;
float PI5 = 0.5f * M_PI;
float PI25 = 0.25f * M_PI;
float PI75 = 0.75f * M_PI;

unsigned char  modem_mode[5] ={0,0,0,0};

unsigned short bpf[5] = { 500, 500, 500, 500,500 };
unsigned short lpf[5] = { 150, 150, 150, 150, 150 };

float BIT_AFC = 32;
float slottime_tick[5] = { 0 };
float resptime_tick[5] = { 0 };
int dcd_threshold = 128;
int rxOffset = 0;
int chanOffset[4] = { 0,0,0,0 };

float DCD_LastPkPos[5] = { 0 };
float DCD_LastPerc[5] = { 0 };
int dcd_bit_cnt[5] = { 0 };
Byte DCD_status[5] = { 0 };
float DCD_persist[5] = { 0 };
int dcd_bit_sync[5] = { 0 };
Byte dcd_hdr_cnt[5] = { 0 };
longword DCD_header[5] = { 0 };
int dcd_on_hdr[5] = { 0 };

extern int centreFreq[4];

float lastangle[4];			// pevious value for differential modes
 

unsigned short n_INTR[5] = { 1,1,1,1,1 };
unsigned short INTR_tap[5] = { 16, 16,16,16,16 };
unsigned short BPF_tap[5] = { 256, 256,256,256,256 };   // 256 default
unsigned short LPF_tap[5] = { 128, 128,128,128,128 };   // 128
  


short rx_freq[5] = { 1700, 1700,1700,1700,1700 };
short rx_shift[5] = { 200, 200, 200, 200, 200 };  
short rx_baudrate[5] = { 300, 300, 300, 300, 300 };
short rcvr_offset[5] = { 30, 30, 30, 30,30 }; 

// rx_freq is configured freq. We shouldn't change it so need a sparate variable
// for the actual demod freq when using multiple decoders

short active_rx_freq[5] = { 1700, 1700,1700,1700,1700 };
 
int fx25_mode[4] = { 0, 0, 0, 0 };
int il2p_mode[4] = { 0, 0, 0, 0 };

int pnt_change[5] = { 0 };
float src_buf[5][2048];

float INTR_core[5][2048];
float AFC_core[5][2048];
float LPF_core[5][2048];

int new_tx_port[4] = { 0,0,0,0 };
UCHAR RCVR[5] = { 0 };

// We allow two (or more!) ports to be assigned to the same soundcard channel

int soundChannel[5] = { 0 };		// 0 = Unused 1 = Left 2 = Right 3 = Mono
int modemtoSoundLR[4] = { 0 };

struct TDetector_t  DET[nr_emph + 1][16];

// Chan, Decoder, Emph

float Phases[4][16][nr_emph + 1][4096];
float Mags[4][16][nr_emph + 1][4096];
int nPhases[4][16][nr_emph + 1];

TStringList detect_list_l[5];
TStringList detect_list[5];
TStringList detect_list_c[5];

int lastDCDState[4] = { 0,0,0,0 };
/*

implementation

uses sm_main,ax25,ax25_l2,ax25_mod,ax25_agw,rsunit,kiss_mode;
*/

void detector_init()
{
	int i, k, j;

	for (k = 0; k < 16; k++)
	{
		for (i = 1; i <= 4; i++)
		{
			for (j = 0; j <= nr_emph; j++)
			{
				struct TDetector_t * pDET = &DET[j][k];

				pDET->fx25[i].status = FX25_TAG;
				pDET->AngleCorr[i] = 0;
				pDET->last_sample[i] = 0;
				pDET->sample_cnt[i] = 0;
				pDET->last_bit[i] = 0;
				pDET->PkAmp[i] = 0;
				pDET->PkAmpMax[i] = 0;
				pDET->newpkpos[i] = 0;
				pDET->ones[i] = 0;
				pDET->zeros[i] = 0;
				pDET->MinAmp[i] = 0;
				pDET->MaxAmp[i] = 0;
				pDET->MUX3_osc[i] = 0;
				pDET->Preemphasis6[i] = 0;
				pDET->Preemphasis12[i] = 0;
				pDET->PSK_AGC[i] = 0;
				pDET->AGC[i] = 0;
				pDET->AGC1[i] = 0;
				pDET->AGC2[i] = 0;
				pDET->AGC3[i] = 0;
				pDET->AGC_max[i] = 0;
				pDET->AGC_min[i] = 0;
				pDET->AFC_IZ1[i] = 0;
				pDET->AFC_IZ2[i] = 0;
				pDET->AFC_QZ1[i] = 0;
				pDET->AFC_QZ2[i] = 0;
				pDET->AFC_dF[i] = 0;
				pDET->AFC_cnt[i] = 0;
				pDET->PSK_IZ1[i] = 0;
				pDET->PSK_QZ1[i] = 0;
				pDET->PkAmpI[i] = 0;
				pDET->PkAmpQ[i] = 0;
				pDET->last_rx_bit[i] = 0;
				pDET->bit_stream[i] = 0;
				pDET->byte_rx[i] = 0;
				pDET->bit_stuff_cnt[i] = 0;
				pDET->bit_cnt[i] = 0;
				pDET->bit_osc[i] = 0;
				pDET->frame_status[i] = 0;
				initString(&pDET->FEC_rx_data[i]);
				initString(&pDET->rx_data[i]);
				initString(&pDET->raw_bits[i]);
				initTStringList(&pDET->mem_ARQ_buf[i]);
				initTStringList(&pDET->mem_ARQ_F_buf[i]);
				pDET->rx_decoded = 0;
				pDET->emph_decoded = 0;
			}
		}
	}
	
	for (i = 1; i <= 4; i++)
	{
		initTStringList(&detect_list[i]);
		initTStringList(&detect_list_l[i]);
		initTStringList(&detect_list_c[i]);
  }
}


/*
procedure detector_free;
var
  i,k,j: word;
{
  for i = 1 to 4 do
  {
    detect_list[i].Free;
    detect_list_l[i].Free;
    detect_list_c[i].Free;
  }
  for k = 0 to 16 do
    for i = 1 to 4 do
      for j = 0 to nr_emph do
      {
        DET[j,k].mem_ARQ_buf[i].Free;
        DET[j,k].mem_ARQ_F_buf[i].Free;
      }
}
*/

void FIR_filter(float * src, unsigned short buf_size, unsigned short tap, float * core, float * dest, float * prev)
{
	float accum = 0.0f;
	float fp1;

	int eax, ebx;
	float * edi;

	fmove(&prev[buf_size], &prev[0], tap * 4);
	fmove(&src[0], &prev[tap], buf_size * 4);

	eax = 0;

	//  ; shl ecx, 2;
	//  ; shl edx, 2;

cfir_i:
	edi = prev;

	edi += eax;

	ebx = 0;
	accum = 0.0f;

cfir_k:

	// FLD pushes operand onto stack, so old value goes to fp1

	fp1 = accum;
	accum = edi[ebx];
	accum *= core[ebx];
	accum += fp1;

	ebx++;
	if (ebx != tap)
		goto cfir_k;

	dest[eax] = accum;

	eax++;

	if (eax != buf_size)
		goto cfir_i;

}


float get_persist(int snd_ch, int persist)
{
	single x, x1 ;

	x = 256 / persist;

	x1 = round(x*x) * rand() / RAND_MAX;

	return x1 * 0.5 * slottime[snd_ch];
}

void chk_dcd1(int snd_ch, int buf_size)
{
	// This seems to schedule all TX, but is only called when a frame has been processed
	//  ? does this work as Andy passes aborted frames to decoder

	Byte port;
	word  i;
	single  tick;
	word  active;
	boolean  ind_dcd;
	boolean  dcd_sync;
	longint  n;

	TAX25Port * AX25Sess;

	dcd[snd_ch] = 1;

	ind_dcd = 0;

	tick = 1000 / RX_Samplerate;

	if (modem_mode[snd_ch] == MODE_ARDOP)
	{
		dcd_bit_sync[snd_ch] = blnBusyStatus;
	}
	else if (modem_mode[snd_ch] == MODE_RUH)
	{
		dcd_bit_sync[snd_ch] = blnBusyStatus;
	}
	else
	{
		if (dcd_bit_cnt[snd_ch] > 0)
			dcd_bit_sync[snd_ch] = 0;
		else
			dcd_bit_sync[snd_ch] = 1;

		if (dcd_on_hdr[snd_ch])
			dcd_bit_sync[snd_ch] = 1;

		if (modem_mode[snd_ch] == MODE_MPSK && DET[0][0].frame_status[snd_ch] == FRAME_LOAD)
			dcd_bit_sync[snd_ch] = 1;
	}

	if (lastDCDState[snd_ch] != dcd_bit_sync[snd_ch])
	{
		updateDCD(snd_ch, dcd_bit_sync[snd_ch]);
		lastDCDState[snd_ch] = dcd_bit_sync[snd_ch];
	}

	if (resptime_tick[snd_ch] < resptime[snd_ch])
		resptime_tick[snd_ch] = resptime_tick[snd_ch] + tick * buf_size;

	slottime_tick[snd_ch] = slottime_tick[snd_ch] + tick * buf_size;

	if (dcd_bit_sync[snd_ch]) // reset the slottime timer
	{
		slottime_tick[snd_ch] = 0;
		DCD_status[snd_ch] = DCD_WAIT_SLOT;
	}

	switch (DCD_status[snd_ch])
	{
	case DCD_WAIT_SLOT:

		if (slottime_tick[snd_ch] >= slottime[snd_ch])
		{
			DCD_status[snd_ch] = DCD_WAIT_PERSIST;
			DCD_persist[snd_ch] = get_persist(snd_ch, persist[snd_ch]);
		}
		break;

	case DCD_WAIT_PERSIST:

		if (slottime_tick[snd_ch] >= slottime[snd_ch] + DCD_persist[snd_ch])
		{
			dcd[snd_ch] = FALSE;
			slottime_tick[snd_ch] = 0;
			DCD_status[snd_ch] = DCD_WAIT_SLOT;
		}
		break;
	}

	active = 0;

	for (i = 0; i < port_num; i++)
	{
		if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
			active++;

		if (active < 2)
			resptime_tick[snd_ch] = resptime[snd_ch];

		if (TX_rotate)
		{
			for (int i = 0; i < 4; i++)
			{
				if (snd_status[i] == SND_TX)
					dcd[snd_ch] = TRUE;
			}
		}

		if (snd_ch == 1)
			snd_ch = 1;

		if (!dcd[snd_ch] && resptime_tick[snd_ch] >= resptime[snd_ch])
		{
			i = 0;

			port = new_tx_port[snd_ch];
			do
			{
				AX25Sess = &AX25Port[snd_ch][port];

				if (AX25Sess->frame_buf.Count > 0)
					Frame_Optimize(AX25Sess, &AX25Sess->frame_buf);

				if (AX25Sess->frame_buf.Count > 0)
				{
					for (n = 0; n < AX25Sess->frame_buf.Count; n++)
					{
						Add(&all_frame_buf[snd_ch], duplicateString(Strings(&AX25Sess->frame_buf, n)));
					}

					Clear(&AX25Sess->frame_buf);
				}

				port++;

				if (port >= port_num)
					port = 0;

				if (all_frame_buf[snd_ch].Count > 0)
					new_tx_port[snd_ch] = port;

				i++;

			} while (all_frame_buf[snd_ch].Count == 0 && i < port_num);

			// Add KISS frames

			if (KISSServ)
			{
				// KISS monitor outgoing AGW frames

				if (all_frame_buf[snd_ch].Count > 0)
				{
					for (n = 0; n < all_frame_buf[snd_ch].Count; n++)
					{
						KISS_on_data_out(snd_ch, Strings(&all_frame_buf[snd_ch], n), 1);	// Mon TX
					}
				}

				// Add outgoing KISS frames to TX Q

				if (KISS.buffer[snd_ch].Count > 0)
				{
					for (n = 0; n < KISS.buffer[snd_ch].Count; n++)
					{
						if (AGWServ)
							AGW_Raw_monitor(snd_ch, Strings(&KISS.buffer[snd_ch], n));

						// Need to add copy as clear will free original

						Add(&all_frame_buf[snd_ch], duplicateString(Strings(&KISS.buffer[snd_ch], n)));
					}
					Clear(&KISS.buffer[snd_ch]);
				}
			}

			if (all_frame_buf[snd_ch].Count > 0 && snd_status[snd_ch] == SND_IDLE)
			{
				resptime_tick[snd_ch] = 0;
				RX2TX(snd_ch);					// Do TX
				return;
			}
		}
	}
}


string * get_pkt_data(string * stream)
{
	Byte  bitstuff_cnt;
	Byte  bits_cnt;
	word  i;
	string * s = newString();

	Byte  bit;
	Byte  raw_bit;
	Byte  sym;

	bits_cnt = 0;
	bitstuff_cnt = 0;
	sym = 0;

	if (stream->Length > 0)
	{
		for (i = 0; i < stream->Length; i++)
		{
			if (stream->Data[i] == '1')
				bit = RX_BIT1;
			else
				bit = RX_BIT0;

			if (bitstuff_cnt < 5)
			{
				sym = (sym >> 1) | bit;
				bits_cnt++;
			}

			if (bitstuff_cnt == 5 || bit == RX_BIT0)
				bitstuff_cnt = 0;

			if (bit == RX_BIT1)
				bitstuff_cnt++;

			if (bits_cnt == 8)
			{
				stringAdd(s, &sym, 1);
				sym = 0;
				bits_cnt = 0;
			}
		}
	}

	return s;
}

string * get_pkt_data2(string * stream, Byte last_nrzi_bit)
{
	Byte  bitstuff_cnt;
	Byte  bits_cnt;
	word  i;
	string * s = newString();

	Byte pkt[350];

	Byte bit;
	Byte raw_bit;
	Byte sym;
	int  n = 0;

	bits_cnt = 0;
	bitstuff_cnt = 0;
	sym = 0;

	if (stream->Length > 0)
	{
		for (i = 0; i < stream->Length; i++)
		{
			if (stream->Data[i] == '1') raw_bit = RX_BIT1; else raw_bit = RX_BIT0;
			if (raw_bit == last_nrzi_bit) bit = RX_BIT1; else bit = RX_BIT0;

			last_nrzi_bit = raw_bit;

			if (bitstuff_cnt < 5)
			{
				sym = (sym >> 1) | bit;
				bits_cnt++;
			}

			if (bitstuff_cnt == 5 || bit == RX_BIT0)
				bitstuff_cnt = 0;

			if (bit == RX_BIT1)
				bitstuff_cnt++;

			if (bits_cnt == 8)
			{
				if (n < 330) 
					pkt[n++] = sym;
			
				sym = 0;
				bits_cnt = 0;
			}
		}
	}

	stringAdd(s, pkt, n);
	return s;
}

string * get_NRZI_data(string * stream, UCHAR last_nrzi_bit)
{
	longword len;
	word i;
	string * s = NULL;
	Byte raw_bit;

	len = stream->Length;

	if (len > 65535)
		len = 65535;

	if (len > 0)
	{
		s = newString();

		setlength(s, len);

		for (i = 0; i < len; i++)
		{
			if (stream->Data[i] == '1')
				raw_bit = RX_BIT1;
			else
				raw_bit = RX_BIT0;

			if (raw_bit == last_nrzi_bit)
				s->Data[i] = '1';
			else
				s->Data[i] = '0';

			last_nrzi_bit = raw_bit;
		}
	}
	return s;
}
/*

function invert_NRZI_data(stream: string; last_nrzi_bit: byte): string;
var
  len: longword;
  i: word;
  s: string;
{
  s = '';
  len = length(stream);
  if len>65535 then len = 65535;
  if len>0 then
  {
    setlength(s,len);
    for i = 1 to len do
      if last_nrzi_bit=RX_BIT0 then
      {
        if stream[i]='1' then s[i] = '0' else s[i] = '1';
      end
      else s[i] = stream[i];
  }
  result = s;
}
*/

void make_rx_frame(int snd_ch, int rcvr_nr, int emph, Byte last_nrzi_bit, string * raw_data, string * raw_data1)
{
	int swap_i, swap_k;
	string * data;
	string * nrzi_data;
	longword raw_len;
	word len, crc1, crc2;
	int arq_mem = 0;
	string s;
	int i, k, n;
	unsigned char * raw;
	unsigned char * raw1;
	char Mode[16] = "";

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	// Decode RAW-stream

	raw_len = raw_data->Length;

	if (raw_len < 80)
		return;

	mydelete(raw_data, raw_len - 6, 7);  // Does this remove trailing flag
	raw_len = raw_data->Length;

	nrzi_data = get_NRZI_data(raw_data, last_nrzi_bit);

	if (nrzi_data == NULL)
		return;

//	data = newString();
	data = get_pkt_data(nrzi_data);

	len = data->Length;

	if (len < pkt_raw_min_len)
	{
		freeString(nrzi_data);
		freeString(data);
		return;
	}

	crc1 = get_fcs(data->Data, len - 2);
	crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

	// MEM recovery

	arq_mem = FALSE;

	if (raw_len > 2970)
		freeString(nrzi_data);
	else
	{
		Add(&pDET->mem_ARQ_buf[snd_ch], nrzi_data);

		if (pDET->mem_ARQ_buf[snd_ch].Count > MEMRecovery[snd_ch])
			Delete(&pDET->mem_ARQ_buf[snd_ch], 0);

		if (crc1 != crc2)
		{
			freeString(data);
			data = get_pkt_data(memory_ARQ(&pDET->mem_ARQ_buf[snd_ch], nrzi_data));
			crc1 = get_fcs(data->Data, len - 2);
			arq_mem = TRUE;
		}
	}

	if (crc1 == crc2)
	{
		if (arq_mem)
		{
			Debugprintf("Good CRC after Memory ARQ correction %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);
			stat_r_mem++;

			pDET->emph_decoded = 2; //MEM
			pDET->rx_decoded = decodedMEM;
		}
		else
		{
			Debugprintf("Good CRC %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

			pDET->rx_decoded = decodedNormal;
			pDET->emph_decoded = 4; //Normal
		}


		if (detect_list[snd_ch].Count > 0 &&
			my_indexof(&detect_list[snd_ch], data) >= 0)
		{
			// Already have a copy of this frame

			freeString(data);
			Debugprintf("Discarding copy rcvr %d emph %d", rcvr_nr, emph);
			return;
		}

		string * xx = newString();
		memset(xx->Data, 0, 16);

		sprintf(Mode, "AX25 %d", centreFreq[snd_ch]);


		Add(&detect_list_c[snd_ch], xx);
		Add(&detect_list[snd_ch], data);

		if (arq_mem)
			stringAdd(xx, "MEM", 3);
		else
			stringAdd(xx, "", 0);

		sprintf(Mode, "AX25 %d", centreFreq[snd_ch]);

		stringAdd(xx, Mode, strlen(Mode));


		return;

	}

	// Single bit recovery

	freeString(data);			// finished with original

	if (recovery[snd_ch] == 0 || raw_len > 2970)
		return;

	raw = raw_data->Data;
	raw1 = raw_data1->Data;

	for (i = 0; i < raw_len; i++)
	{
		if (raw[i] != raw1[i])
		{
			//change bit
			raw[i] ^= 1;

			// get new data

			data = get_pkt_data2(raw_data, last_nrzi_bit);

			//restore bit

			raw[i] ^= 1;

			len = data->Length;

			if (len > pkt_raw_min_len)
			{
				crc1 = get_fcs(data->Data, len - 2);
				crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

				if (crc1 == crc2)
				{
					Debugprintf("Good CRC after single bit correction %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

					if (detect_list[snd_ch].Count > 0 &&
						my_indexof(&detect_list[snd_ch], data) >=- 0)
					{
						// Already have a copy of this frame

						Debugprintf("Discarding copy rcvr %d, emph %d", rcvr_nr, emph);
						freeString(data);
						return;
					}
					string * xx = newString();
					memset(xx->Data, 0, 16);

					Add(&detect_list_c[snd_ch], xx);
					Add(&detect_list[snd_ch], data);
					stringAdd(xx, "SINGLE", 3);

					pDET->rx_decoded = decodedSingle;
					pDET->emph_decoded = 1; //SINGLE

					return;
				}
			}
			freeString(data);			// finished with original
		}
	}
}



int lastcrc = 0;


void make_rx_frame_PSK(int snd_ch, int rcvr_nr, int emph, string * data)
{
	word len, crc1, crc2;

	len = data->Length;

	if (len < pkt_raw_min_len)
		return;

	crc1 = get_fcs(data->Data, len - 2);
	crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

	if (crc1 == crc2)
	{
		struct TDetector_t * pDET = &DET[emph][rcvr_nr];

		Debugprintf("Good CRC %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

		pDET->rx_decoded = decodedNormal;
		pDET->emph_decoded = 4; //Normal

		if (detect_list[snd_ch].Count > 0 &&
			my_indexof(&detect_list[snd_ch], data) >= 0)
		{
			// Already have a copy of this frame

			Debugprintf("Discarding copy rcvr %d emph %d", rcvr_nr, emph);
			return;
		}

		string * xx = newString();

		memset(xx->Data, 0, 16);
		Add(&detect_list_c[snd_ch], xx);
		xx = duplicateString(data);
		Add(&detect_list[snd_ch], xx);
	}
}


  /*

function memory_ARQ_FEC(buf: TStringList; data: string): string;
var
  len,i,k: integer;
  s,temp: string;
  new_blk,temp_blk: TStringList;
  n,err: byte;
  done: boolean;
{
  s = '';
  if data='' then { result = s; exit; }
  new_blk = TStringList.Create;
  temp_blk = TStringList.Create;
  temp = data;
  len = length(data);
  // Split new data;
  repeat
    n = ord(temp[1]) and $7F;
    err = ord(temp[1]) and $80;
    if err=0 then new_blk.Add(copy(temp,2,n)) else new_blk.Add('');
    delete(temp,1,n+1);
  until temp='';
  // Search blocks
  if (buf.Count>0) and (new_blk.Count>0) then
  {
    i = 0;
    repeat
      // If length is the same
      if length(buf.Strings[i])=len then
      {
        temp = buf.Strings[i];
        // If last 4 bytes is the same
        if copy(temp,len-3,4)=copy(data,len-3,4) then
        {
          temp_blk.Clear;
          repeat
            n = ord(temp[1]) and $7F;
            err = ord(temp[1]) and $80;
            if err=0 then temp_blk.Add(copy(temp,2,n)) else temp_blk.Add('');
            delete(temp,1,n+1);
          until temp='';
          // Add new parts
          if new_blk.Count=temp_blk.Count then
          {
            done = TRUE;
            for k = 0 to new_blk.Count-1 do
            {
              if (new_blk.Strings[k]='') and (temp_blk.Strings[k]<>'') then
                new_blk.Strings[k] = temp_blk.Strings[k];
              // Check if no empty data
              if new_blk.Strings[k]='' then done = FALSE;
            }
          }
        }
      }
      inc(i);
    until (i=buf.Count) or done;
    if done then for k = 0 to new_blk.Count-1 do s = s+new_blk.Strings[k];
  }
  result = s;
  new_blk.Free;
  temp_blk.Free
}

procedure add_to_ARQ_FEC(buf: TStringList; data: string);
{
  if buf.Count=50 then buf.Delete(0);
  buf.Add(data);
}
*/

void make_rx_frame_FEC(int snd_ch, int rcvr_nr, string * data, string * fec_data, word nErr)
{
}

/*var
  len,crc1,crc2: word;
  s: string;
  i,k,n: word;
{
  len = length(data);
  if len<17 then exit;
  crc1 = get_fcs(data,len-2);
  crc2 = (ord(data[len]) shl 8) or ord(data[len-1]);
  if crc1=crc2 then
  {
    if detect_list[snd_ch].Count>0 then
      {
        //if detect_list[snd_ch].IndexOf(data)<0 then
        if my_indexof(detect_list[snd_ch],data)<0 then
        {
          detect_list[snd_ch].Add(data);
          detect_list_c[snd_ch].Add('Err: '+inttostr(nErr));
        }
      end
    else
    {
      detect_list[snd_ch].Add(data);
      detect_list_c[snd_ch].Add('Err: '+inttostr(nErr));
    }
    add_to_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
  }
  if crc1<>crc2 then
  {
    data = memory_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
    add_to_ARQ_FEC(DET[0,rcvr_nr].mem_ARQ_F_buf[snd_ch],fec_data);
    if data<>'' then
    {
      len = length(data);
      crc1 = get_fcs(data,len-2);
      crc2 = (ord(data[len]) shl 8) or ord(data[len-1]);
      if crc1=crc2 then
      {
        if detect_list[snd_ch].Count>0 then
        {
          //if detect_list[snd_ch].IndexOf(data)<0 then
          if my_indexof(detect_list[snd_ch],data)<0 then
          {
            detect_list[snd_ch].Add(data);
            detect_list_c[snd_ch].Add('MEM Err: '+inttostr(nErr));
          }
        end
        else
        {
          detect_list[snd_ch].Add(data);
          detect_list_c[snd_ch].Add('MEM Err: '+inttostr(nErr));
        }
      }
    }
  }
}

*/
////////////////////////////  PLL-Peak-detector  ////////////////////////////

void  Mux3(int snd_ch, int rcvr_nr, int emph, float * src1, float * core, float *dest, float * prevI, float * prevQ, int tap, int buf_size)
{
	float pi2 = 2 * pi;

	int i;
	float x;
	float acc1, acc2, acc3, mag;
	int tap4;
	int tap_cnt;
	unsigned int ii, kk;

	float Preemphasis6, Preemphasis12, MUX3_osc, AGC;
	float AFC_IZ1, AFC_QZ1, AFC_IZ2, AFC_QZ2;

	// looks like this is an LPF

	// Get local copy of this detectors variables

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	Preemphasis6 = pDET->Preemphasis6[snd_ch];
	Preemphasis12 = pDET->Preemphasis12[snd_ch];
	MUX3_osc = pDET->MUX3_osc[snd_ch];
	AGC = pDET->AGC[snd_ch];
	AFC_IZ2 = pDET->AFC_IZ2[snd_ch];
	AFC_QZ2 = pDET->AFC_QZ2[snd_ch];
	AFC_QZ1 = pDET->AFC_QZ1[snd_ch];
	AFC_IZ1 = pDET->AFC_IZ1[snd_ch];
	//
	tap4 = tap * 4;
	x = active_rx_freq[snd_ch] * pi2 / RX_Samplerate;

	fmove(&prevI[buf_size], &prevI[0], tap4);
	fmove(&prevQ[buf_size], &prevQ[0], tap4);
	tap_cnt = tap;

	if (prevI[128] != prevI[128])
		prevI[128] = 0;

	for (i = 0; i < buf_size; i++)
	{
		// Pre-emphasis 6dB
		if (emph > 0)
		{
			acc1 = Preemphasis6 - src1[i];
			Preemphasis6 = src1[i];
			src1[i] = acc1;
		}
		// Pre-emphasis 12dB
		if (emph > 1)
		{
			acc1 = Preemphasis12 - src1[i];
			Preemphasis12 = src1[i];
			src1[i] = acc1;
		}
		//
		MUX3_osc = MUX3_osc + x;

		if (MUX3_osc > pi2)
			MUX3_osc = MUX3_osc - pi2;

		if (src1[i] != src1[i])
			src1[i] = 0;

		if (prevI[128] != prevI[128])
			prevI[128] = 0;


		prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);
		prevQ[tap_cnt] = src1[i] * cosf(MUX3_osc);

		if (prevI[128] != prevI[128])
			prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);

		if (prevI[128] != prevI[128])
			prevI[128] = 0;
		/*

			mag = sqrtf(prevI[tap_cnt] * prevI[tap_cnt] + prevQ[tap_cnt] * prevQ[tap_cnt]);
			DET[emph][rcvr_nr].AGC1[snd_ch] = 0.5*DET[emph][rcvr_nr].AGC1[snd_ch] + 0.5*mag;
			AGC = 0.5*AGC + 0.5*DET[emph][rcvr_nr].AGC1[snd_ch];
			if (AGC > 1)
			begin
				prevI[tap_cnt] = prevI[tap_cnt] / AGC;
				prevQ[tap_cnt] = prevQ[tap_cnt] / AGC;
			end
	*/


	// Fast AGC

		mag = sqrtf(prevI[tap_cnt] * prevI[tap_cnt] + prevQ[tap_cnt] * prevQ[tap_cnt]);

		AGC = 0.5 * AGC + 0.5  *mag;

		if (AGC > 1)
		{
			prevI[tap_cnt] = prevI[tap_cnt] / AGC;
			prevQ[tap_cnt] = prevQ[tap_cnt] / AGC;
		}

		ii = i << 2;
		kk = tap << 2;

		// C version of delphi asm code below
		{
			float accum = 0.0f;
			float fp1;

			int ebx;
			float * edi;

			edi = &prevI[i];
			ebx = 0;
			accum = 0.0f;

		fsk_k1:

			// FLD pushes operand onto stack, so old value goes to fp1

			fp1 = accum;
			accum = edi[ebx];
			if (accum != accum)
				accum = 0;

			accum *= core[ebx];
			if (accum != accum)
				accum = 0;
			accum += fp1;
			if (accum != accum)
				accum = 0;

			ebx++;
			if (ebx != tap)
				goto fsk_k1;

			acc1 = accum;

			if (acc1 != acc1)
				acc1 = 0;

			edi = &prevQ[i];

			ebx = 0;
			accum = 0.0f;

		fsk_k2:

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;
			if (ebx != tap)
				goto fsk_k2;

			acc2 = accum;
		}

		if (acc1 != acc1)
			acc1 = 0;


		tap_cnt++;

		/// PLL-Detector ///


		dest[i] = (acc1 - AFC_IZ2)*AFC_QZ1 - (acc2 - AFC_QZ2)*AFC_IZ1;

		// Check for NAN

		if (dest[i] != dest[i])
			dest[i] = 0.0f;

		AFC_IZ2 = AFC_IZ1;
		AFC_QZ2 = AFC_QZ1;
		AFC_IZ1 = acc1;
		AFC_QZ1 = acc2;
	}

	pDET->Preemphasis6[snd_ch] = Preemphasis6;
	pDET->Preemphasis12[snd_ch] = Preemphasis12;
	pDET->MUX3_osc[snd_ch] = MUX3_osc;
	pDET->AGC[snd_ch] = AGC;
	pDET->AFC_IZ2[snd_ch] = AFC_IZ2;
	pDET->AFC_QZ2[snd_ch] = AFC_QZ2;
	pDET->AFC_QZ1[snd_ch] = AFC_QZ1;
	pDET->AFC_IZ1[snd_ch] = AFC_IZ1;
}



void Mux3_PSK(int snd_ch, int rcvr_nr, int emph, float * src1, float * core, float *destI, float *destQ, float * prevI, float * prevQ, int tap, int buf_size)
{
	float pi2 = 2 * pi;

	int i;
	float x;
	float acc1, acc2, mag;
	int tap4;
	int prev_cnt, tap_cnt;

	float Preemphasis6, Preemphasis12, MUX3_osc;

	// looks like this is an LPF

	// Get local copy of this detectors variables

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	Preemphasis6 = pDET->Preemphasis6[snd_ch];
	Preemphasis12 = pDET->Preemphasis12[snd_ch];
	MUX3_osc = pDET->MUX3_osc[snd_ch];

	tap4 = tap * 4;

	x = active_rx_freq[snd_ch] * pi2 / RX_Samplerate;

	fmove(&prevI[buf_size], &prevI[0], tap4);
	fmove(&prevQ[buf_size], &prevQ[0], tap4);

	tap_cnt = tap;

	if (prevI[128] != prevI[128])
		prevI[128] = 0;

	for (i = 0; i < buf_size; i++)
	{
		// Pre-emphasis 6dB
		if (emph > 0)
		{
			acc1 = Preemphasis6 - src1[i];
			Preemphasis6 = src1[i];
			src1[i] = acc1;
		}
		// Pre-emphasis 12dB
		if (emph > 1)
		{
			acc1 = Preemphasis12 - src1[i];
			Preemphasis12 = src1[i];
			src1[i] = acc1;
		}

		MUX3_osc = MUX3_osc + x;

		if (MUX3_osc > pi2)
			MUX3_osc = MUX3_osc - pi2;

		prevI[tap_cnt] = src1[i] * sinf(MUX3_osc);
		prevQ[tap_cnt] = src1[i] * cosf(MUX3_osc);


		// C version of delphi asm code 
		{
			float accum = 0.0f;
			float fp1;

			int ebx;
			float * edi;

			edi = &prevI[i];
			ebx = 0;
			accum = 0.0f;

		fsk_k1:

			// FLD pushes operand onto stack, so old value goes to fp1

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;

			if (ebx != tap)
				goto fsk_k1;

			acc1 = accum;

			edi = &prevQ[i];

			ebx = 0;
			accum = 0.0f;

		fsk_k2:

			fp1 = accum;
			accum = edi[ebx];
			accum *= core[ebx];
			accum += fp1;

			ebx++;
			if (ebx != tap)
				goto fsk_k2;

			acc2 = accum;
		}

		if (acc1 != acc1)
			acc1 = 0;

		tap_cnt++;

		destI[i] = acc1;
		destQ[i] = acc2;
	}

	pDET->Preemphasis6[snd_ch] = Preemphasis6;
	pDET->Preemphasis12[snd_ch] = Preemphasis12;
	pDET->MUX3_osc[snd_ch] = MUX3_osc;

}

int stats[2] = { 0 };

#define dcd_corr 0.11111f

void decode_stream_MPSK(int snd_ch, int rcvr_nr, float *  src, int buf_size, int  last)
{

#ifndef XXXX

	// Until ASM is converted

	return;
}
#else

	float  pi2 = 2 * pi;

#define NR_FEC_CH 3

	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	word  dcnt, dsize;
	word  i, k, j, j1, j2, j3;
	single  x, x1;
	single  amp, acc1, acc2;
	single  sumI, sumQ, sumIQ, muxI, muxQ;
	word  tap_cnt, tap_cnt1;
	single  afc_lim;
	word  i_tap, tap;
	single  AFC, k1, k2, freq;
	single  maxval, div_bit_afc, baudrate;
	word  max_cnt;
	single  AmpI, AmpQ, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	single  AFC_acc1, AFC_acc2;
	single  BIT_acc1, BIT_acc2;
	integer  AFC_newpkpos;
	//
	single  threshol;
	single  tr;
	Byte fec_ch, bit;
	longword ii, kk;
	single * core, *prevI, *prevQ;
	//
	unsigned long long bit64 = 0;
	boolean  hdr_ok;
	Byte  fec_code;
	string  fec_data_blk;
	string  line1;

	Byte line[512];
	int linelen = 0;

	integer nErr;
	word  crc1, crc2, size;

	Byte  hdr_byte[15] = "";

	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	baudrate = 400;
	div_bit_afc = 1.0f / roundf(BIT_AFC*(RX_Samplerate / 11025));
	x1 = baudrate / RX_Samplerate;
	max_cnt = roundf(RX_Samplerate / baudrate);
	//

	afc_lim = rx_baudrate[snd_ch] * 0.1f;
	dsize = buf_size / n_INTR[snd_ch];
	tap = LPF_tap[snd_ch];
	i_tap = INTR_tap[snd_ch];
	freq = active_rx_freq[snd_ch];


	for (fec_ch = 0; fec_ch <= NR_FEC_CH; fec_ch++)
	{
		struct TMChannel_t * pMChan = &DET[0][rcvr_nr].MChannel[snd_ch][fec_ch];

		fmove(&pMChan->prev_dLPFI_buf[buf_size], &pMChan->prev_dLPFI_buf[0], i_tap * 4);
		fmove(&pMChan->prev_dLPFQ_buf[buf_size], &pMChan->prev_dLPFQ_buf[0], i_tap * 4);
		fmove(&pMChan->prev_LPF1I_buf[dsize], &pMChan->prev_LPF1I_buf[0], tap * 4);
		fmove(&pMChan->prev_LPF1Q_buf[dsize], &pMChan->prev_LPF1Q_buf[0], tap * 4);
		fmove(&pMChan->prev_AFCI_buf[dsize], &pMChan->prev_AFCI_buf[0], tap * 4);
		fmove(&pMChan->prev_AFCQ_buf[dsize], &pMChan->prev_AFCQ_buf[0], tap * 4);
	}

	tap_cnt = i_tap;
	tap_cnt1 = tap;
	dcnt = 0;
	k = 0;

	for (i = 0; i < buf_size; i++)
	{
		for (fec_ch = 0; fec_ch <= NR_FEC_CH; fec_ch++)
		{
			struct TDetector_t * pDET = &DET[0][rcvr_nr];

			x = (freq + pDET->AFC_dF[snd_ch] + ch_offset[fec_ch])*pi2 / RX_Samplerate;

			struct TMChannel_t * pMChan = &pDET->MChannel[snd_ch][fec_ch];
			{
				pMChan->MUX_osc = pMChan->MUX_osc + x;

				if (pMChan->MUX_osc > pi2)
					pMChan->MUX_osc = pMChan->MUX_osc - pi2;

				pMChan->prev_dLPFI_buf[tap_cnt] = src[i] * sinf(pMChan->MUX_osc);
				pMChan->prev_dLPFQ_buf[tap_cnt] = src[i] * cosf(pMChan->MUX_osc);
				prevI = pMChan->prev_dLPFI_buf;
				prevQ = pMChan->prev_dLPFQ_buf;
				core = INTR_core[snd_ch];
				// Decimation filter
				ii = i << 2;
				kk = i_tap << 2;
	
				_asm
				{
					push eax;
					push ebx;
					push edi;
					push esi;
					mov edi, prevI;
					mov esi, core;
					add edi, ii;
					mov eax, kk;
					xor ebx, ebx;
					fldz;
				lk1:
					fld dword ptr[edi + ebx];
					fmul dword ptr[esi + ebx];
					fadd;
					add ebx, 4;
					cmp ebx, eax;
					jne lk1;
					fstp dword ptr acc1;
					wait;
					mov edi, prevQ;
					add edi, ii;
					xor ebx, ebx;
					fldz;
				lk2:
					fld dword ptr[edi + ebx];
					fmul dword ptr[esi + ebx];
					fadd;
					add ebx, 4;
					cmp ebx, eax;
					jne lk2;
					fstp dword ptr acc2;
					wait;
					pop esi;
					pop edi;
					pop ebx;
					pop eax;
				}
			}

			if (fec_ch == NR_FEC_CH)
				tap_cnt++;

			// Decimation

			if (dcnt == 0)
			{
				{
					pMChan->prev_LPF1I_buf[tap_cnt1] = acc1;
					pMChan->prev_LPF1Q_buf[tap_cnt1] = acc2;
					pMChan->prev_AFCI_buf[tap_cnt1] = acc1;
					pMChan->prev_AFCQ_buf[tap_cnt1] = acc2;
					// Bit-filter
					prevI = pMChan->prev_LPF1I_buf;
					prevQ = pMChan->prev_LPF1Q_buf;
					core = LPF_core[snd_ch];
					ii = k << 2;
					kk = tap << 2;
					
					__asm
					{
						push eax;
						push ebx;
						push edi;
						push esi;
						mov edi, prevI;
						mov esi, core;
						add edi, ii;
						mov eax, kk;
						xor ebx, ebx;
						fldz;
					xk1:
						fld dword ptr[edi + ebx];
						fmul dword ptr[esi + ebx];
						fadd;
						add ebx, 4;
						cmp ebx, eax;
						jne xk1;
						fstp dword ptr BIT_acc1;
						wait;
						mov edi, prevQ;
						add edi, ii;
						xor ebx, ebx;
						fldz;
					xk2:
						fld dword ptr[edi + ebx];
						fmul dword ptr[esi + ebx];
						fadd;
						add ebx, 4;
						cmp ebx, eax;
						jne xk2;
						fstp dword ptr BIT_acc2;
						wait;
						pop esi;
						pop edi;
						pop ebx;
						pop eax;
					}
					
					// AFC-filter
					prevI = pMChan->prev_AFCI_buf;
					prevQ = pMChan->prev_AFCQ_buf;
					core = AFC_core[snd_ch];
					ii = k << 2;
					kk = tap << 2;
					_asm
					{
						push eax;
						push ebx;
						push edi;
						push esi;
						mov edi, prevI;
						mov esi, core;
						add edi, ii;
						mov eax, kk;
						xor ebx, ebx;
						fldz;
					xxk1:
						fld dword ptr[edi + ebx];
						fmul dword ptr[esi + ebx];
						fadd;
						add ebx, 4;
						cmp ebx, eax;
						jne xxk1;
						fstp dword ptr AFC_acc1;
						wait;
						mov edi, prevQ;
						add edi, ii;
						xor ebx, ebx;
						fldz;
					xxk2:
						fld dword ptr[edi + ebx];
						fmul dword ptr[esi + ebx];
						fadd;
						add ebx, 4;
						cmp ebx, eax;
						jne xxk2;
						fstp dword ptr AFC_acc2;
						wait;
						pop esi;
						pop edi;
						pop ebx;
						pop eax;
					}
				}

				// AGC

				amp = sqrtf(BIT_acc1*BIT_acc1 + BIT_acc2 * BIT_acc2);
				if (amp > pDET->PSK_AGC[snd_ch])
					pDET->PSK_AGC[snd_ch] = pDET->PSK_AGC[snd_ch] * agc_fast1 + amp*agc_fast;
				else
					pDET->PSK_AGC[snd_ch] = pDET->PSK_AGC[snd_ch] * agc_slow1 + amp*agc_slow;

				if (pDET->PSK_AGC[snd_ch] > 1)
				{
					BIT_acc1 = BIT_acc1 / pDET->PSK_AGC[snd_ch];
					BIT_acc2 = BIT_acc2 / pDET->PSK_AGC[snd_ch];
					AFC_acc1 = AFC_acc1 / pDET->PSK_AGC[snd_ch];
					AFC_acc2 = AFC_acc2 / pDET->PSK_AGC[snd_ch];
					amp = amp / pDET->PSK_AGC[snd_ch];
				}

				// AFC Correction


				sumIQ = (AFC_acc1 - pMChan->AFC_IZ2)*pMChan->AFC_QZ1 - (AFC_acc2 - pMChan->AFC_QZ2)*pMChan->AFC_IZ1;
				pMChan->AFC_IZ2 = pMChan->AFC_IZ1;
				pMChan->AFC_QZ2 = pMChan->AFC_QZ1;
				pMChan->AFC_IZ1 = AFC_acc1;
				pMChan->AFC_QZ1 = AFC_acc2;

				pDET->AFC_dF[snd_ch] = pDET->AFC_dF[snd_ch] - sumIQ * 0.07f; // AFC_LPF=1

				if (pDET->AFC_dF[snd_ch] > afc_lim)
					pDET->AFC_dF[snd_ch] = afc_lim;

				if (pDET->AFC_dF[snd_ch] < -afc_lim)
					pDET->AFC_dF[snd_ch] = -afc_lim;


				pMChan->AFC_bit_buf1I[pDET->AFC_cnt[snd_ch]] = BIT_acc1;
				pMChan->AFC_bit_buf1Q[pDET->AFC_cnt[snd_ch]] = BIT_acc2;
				pMChan->AFC_bit_buf2[pDET->AFC_cnt[snd_ch]] = amp;

				if (fec_ch == NR_FEC_CH)
				{
					pDET->AFC_cnt[snd_ch]++;
					pDET->AFC_bit_osc[snd_ch] = pDET->AFC_bit_osc[snd_ch] + x1;

					if (pDET->AFC_bit_osc[snd_ch] >= 1)
					{
						// Find the maximum in the synchronization buffer

						for (j = 0; j <= NR_FEC_CH; j++)
						{
							struct TMChannel_t * pMChan = &pDET->MChannel[snd_ch][j];

							maxval = 0;

							for (j1 = 0; j1 < pDET->AFC_cnt[snd_ch]; j1++)
							{
								amp = pMChan->AFC_bit_buf2[j1];

								pDET->AFC_bit_buf[snd_ch][j1] = pDET->AFC_bit_buf[snd_ch][j1] * 0.95 + amp*0.05;

								if (pDET->AFC_bit_buf[snd_ch][j1] > maxval)
								{
									{
										AFC_newpkpos = j1;
										maxval = pDET->AFC_bit_buf[snd_ch][j1];
									}
								}

								k1 = 1.0f *AFC_newpkpos / (pDET->AFC_cnt[snd_ch] - 1);
								k2 = pila(k1) - 1;



								//AFC = div_bit_afc*k2;
								AFC = div_bit_afc * k2*0.25; //for 4 carriers
								if (k1 > 0.5)
									pDET->AFC_bit_osc[snd_ch] = pDET->AFC_bit_osc[snd_ch] + AFC;
								else
									pDET->AFC_bit_osc[snd_ch] = pDET->AFC_bit_osc[snd_ch] - AFC;

								//DCD feature

								if (last)
								{
									DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96f + AFC_newpkpos * 0.04f;
									DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96f + abs(AFC_newpkpos - DCD_LastPkPos[snd_ch])*0.04f;
									if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
										dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
									else
										dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
								}

							}
							// Bit-detector

							AmpI = pMChan->AFC_bit_buf1I[AFC_newpkpos];
							AmpQ = pMChan->AFC_bit_buf1Q[AFC_newpkpos];
							muxI1 = AmpI * pMChan->AFC_IIZ1;
							muxI2 = AmpQ * pMChan->AFC_IIZ1;
							muxQ1 = AmpQ * pMChan->AFC_QQZ1;
							muxQ2 = AmpI * pMChan->AFC_QQZ1;
							sumIQ1 = muxI1 + muxQ1;
							sumIQ2 = muxI2 - muxQ2;
							angle = atan2f(sumIQ2, sumIQ1);
							pMChan->AFC_IIZ1 = AmpI;
							pMChan->AFC_QQZ1 = AmpQ;

							// Phase corrector

							if (fabsf(angle) < PI5)
								pMChan->AngleCorr = pMChan->AngleCorr * 0.9f - angle * 0.1f;
							else
							{
								if (angle > 0)
									pMChan->AngleCorr = pMChan->AngleCorr * 0.9f + (pi - angle)*0.1f;
								else
									pMChan->AngleCorr = pMChan->AngleCorr * 0.9f + (-pi - angle)*0.1f;
							}
							angle = angle + pMChan->AngleCorr;


							if (fabsf(angle) < PI5)
								bit = RX_BIT1;
							else
								bit = RX_BIT0;

							// DCD on flag

							if (last)
							{
								if (dcd_hdr_cnt[snd_ch] > 0)
									dcd_hdr_cnt[snd_ch]--;

								DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

								if ((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000 ||
									(DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000 ||
									(DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000)
								{
									dcd_hdr_cnt[snd_ch] = 48;
									dcd_on_hdr[snd_ch] = TRUE;
								}
							}

							// header stream
							bit64 = bit;
							bit64 <<= 56;

							pDET->FEC_header1[snd_ch][1] = (pDET->FEC_header1[snd_ch][1] >> 1) | (pDET->FEC_header1[snd_ch][0] << 63);
							pDET->FEC_header1[snd_ch][0] = (pDET->FEC_header1[snd_ch][0] >> 1) | bit64;
	
							// copy body
							if (pDET->frame_status[snd_ch] == FRAME_LOAD)
							{
								pDET->bit_stream[snd_ch] = (pDET->bit_stream[snd_ch] >> 1) + bit;
								pDET->bit_cnt[snd_ch]++;
								if (pDET->bit_cnt[snd_ch] == 8)
								{
									pDET->bit_cnt[snd_ch] = 0;
									pDET->FEC_len_cnt[snd_ch]++;

									stringAdd(&pDET->FEC_rx_data[snd_ch], &pDET->bit_stream[snd_ch], 1);

									if (pDET->FEC_len_cnt[snd_ch] == pDET->FEC_len[snd_ch])
									{
										// descrambler
										scrambler(pDET->FEC_rx_data[snd_ch].Data, pDET->FEC_rx_data[snd_ch].Length);
										// deinterleave
										pDET->FEC_blk_int[snd_ch] = ((pDET->FEC_len[snd_ch] - 1) / 16) + 1;

										linelen = pDET->FEC_rx_data[snd_ch].Length;

										memcpy(line, pDET->FEC_rx_data[snd_ch].Data, linelen);

										j3 = 1;

										for (j1 = 0; j1 < 16; j1++)
										{
											for (j2 = 0; j2 < pDET->FEC_blk_int[snd_ch]; j2++)
											{
												if ((j2 * 16 + j1) <= pDET->FEC_len[snd_ch] && j3 <= pDET->FEC_len[snd_ch])
												{
													pDET->FEC_rx_data[snd_ch].Data[j2 * 16 + j1] = line[j3];
													j3++;
												}
											}
										}

										// RS-decode

										/*

										line = pDET->FEC_rx_data[snd_ch];
										pDET->FEC_rx_data[snd_ch].Length = 0;
										do
										{
											line1 = copy(line, 1, 16);
											size = length(line1);
											FillChar(xEncoded, SizeOf(xEncoded), 0);
											FillChar(xDecoded, SizeOf(xDecoded), 0);
											move(line1[1], xEncoded[0], size);
											RS.InitBuffers;
											nErr = RS.DecodeRS(xEncoded, xDecoded);
											line1 = '';
											for j1 = MaxErrors * 2 to size - 1 do line1 = line1 + chr(xDecoded[j1]);
											pDET->FEC_rx_data[snd_ch] = FEC_rx_data[snd_ch] + line1;
											if nErr >= 0 then FEC_err[snd_ch] = FEC_err[snd_ch] + nErr;
											// For MEM-ARQ
											fec_code = length(line1) and $7F;
											if nErr < 0 then fec_code = fec_code or $80;
											fec_data_blk = fec_data_blk + chr(fec_code) + line1;
											delete(line, 1, 16);
										} while(line.Count);
										*/


										make_rx_frame_FEC(snd_ch, rcvr_nr, &pDET->FEC_rx_data[snd_ch], &fec_data_blk, pDET->FEC_err[snd_ch]);
										pDET->FEC_rx_data[snd_ch].Length = 0;
										pDET->frame_status[snd_ch] = FRAME_WAIT;
										pDET->FEC_header1[snd_ch][0] = 0;
										pDET->FEC_header1[snd_ch][1] = 0;
									}
								}
							}

							hdr_ok = FALSE;

							// I think FEC_header1[0] and FEC_header1[1] form the 128 bit header
							// We look for a pair of flags, but allow a few bits to be wrong
							// as FEC may fix them

							if (pDET->frame_status[snd_ch] == FRAME_WAIT)
							{
								j1 = (pDET->FEC_header1[snd_ch][1] >> 16) ^ 0x7E7E;
								/*_asm
								{
									push ax;
									push bx;
									push cx;
									mov ax, 15;
									mov bx, j1;
								zloop:
									mov cx, bx;
									and cx, 1;
									cmp cx, 1;
									jne is_zero;
									inc ah; // count damaged bits
								is_zero:
									shr bx, 1;
									dec al;
									jnz zloop;
									cmp ah, 5; // greater than 4 bits
									jnb greater;
									mov hdr_ok, TRUE;
								greater:
									pop cx;
									pop bx;
									pop ax;
								}
								*/
							}
							//if (FEC_header1[snd_ch][1] shr 24 and $FF=$7E) and (frame_status[snd_ch]=FRAME_WAIT) then

							if (hdr_ok)
							{
								// Have up to 4 bits wrong in 7E7E pattern

								// Extract header, check crc then try RS

								hdr_ok = FALSE;

//								if ((pDET->FEC_header1[snd_ch][1] & 0xffff0000) == 0x7E7E0000)
//								{

								hdr_byte[13] = (pDET->FEC_header1[snd_ch][1] >> 24) & 0xFF;
								hdr_byte[14] = (pDET->FEC_header1[snd_ch][1] >> 16) & 0xFF;

								if (hdr_byte[13] == 0x7E && hdr_byte[14] == 0x7E)
								{
									hdr_byte[1] = (pDET->FEC_header1[snd_ch][0] >> 56) & 0xFF;
									hdr_byte[2] = (pDET->FEC_header1[snd_ch][0] >> 48) & 0xFF;
									hdr_byte[3] = (pDET->FEC_header1[snd_ch][0] >> 40) & 0xFF;
									hdr_byte[4] = (pDET->FEC_header1[snd_ch][0] >> 32) & 0xFF;
									hdr_byte[5] = (pDET->FEC_header1[snd_ch][0] >> 24) & 0xFF;
									hdr_byte[6] = (pDET->FEC_header1[snd_ch][0] >> 16) & 0xFF;
									hdr_byte[7] = (pDET->FEC_header1[snd_ch][0] >> 8) & 0xFF;
									hdr_byte[8] = pDET->FEC_header1[snd_ch][0] & 0xFF;
									hdr_byte[9] = (pDET->FEC_header1[snd_ch][1] >> 56) & 0xFF;
									hdr_byte[10] = (pDET->FEC_header1[snd_ch][1] >> 48) & 0xFF;
									hdr_byte[11] = (pDET->FEC_header1[snd_ch][1] >> 40) & 0xFF;
									hdr_byte[12] = (pDET->FEC_header1[snd_ch][1] >> 32) & 0xFF;

									pDET->FEC_len[snd_ch] = hdr_byte[12] << 8 + hdr_byte[11];
									line[0] = 0x7E;
									line[1] = 0x7E;
									line[2] = hdr_byte[12];
									line[3] = hdr_byte[11];

									crc1 = (hdr_byte[10] << 8) + hdr_byte[9];
									crc2 = get_fcs(line, 4);

									if (crc1 == crc2)
										hdr_ok = TRUE;

									Debugprintf("Len %d CRC %x %x", pDET->FEC_len[snd_ch], crc1, crc2);
								}
								/*									if (!hdr_ok)
																	{
																		linelen = 0;
																		for (j1 = 14; j1 > 0; j1-)
																			line[linelen++] = hdr_byte[j1);



																		FillChar(xEncoded, SizeOf(xEncoded), 0);
																		FillChar(xDecoded, SizeOf(xDecoded), 0);
																		line = copy(&line, 7, 8) + copy(&line, 1, 6);
																		move(&line[1], xEncoded[0], 14);
																		RS.InitBuffers;
																		nErr = RS.DecodeRS(xEncoded, xDecoded);
																		if (nErr > -1)
																		{
																			line.Length = 0;

																			for (j1 = 8; j1 < 13; j1++)
																				stringAdd(&line, &xDecoded[j1], 1);

																			if (line[1] == 0x7E && line[2] == 0x7E)
																			{
																				FEC_len[snd_ch] = ord(line[3]) shl 8 + ord(line[4]);
																				crc1 = (line[5] << 8) + line[6]);
																				line = copy(line, 1, 4);
																				crc2 = get_fcs(line, 4);
																				if (crc1 == crc2)
																					hdr_ok = TRUE;
																			}
																		}
																	}
								*/
								if (hdr_ok)
								{
									pDET->FEC_len[snd_ch] = pDET->FEC_len[snd_ch] & 1023; //limit of length
									if (pDET->FEC_len[snd_ch] > 0)
									{
										pDET->frame_status[snd_ch] = FRAME_LOAD;
										pDET->FEC_len_cnt[snd_ch] = 0;
										pDET->bit_cnt[snd_ch] = 0;
										pDET->FEC_err[snd_ch] = 0;
										pDET->FEC_rx_data[snd_ch].Length = 0;
										fec_data_blk.Length = 0;
									}
								}
							}
						}
						// Finalize
						if (pDET->AFC_cnt[snd_ch] <= max_cnt)
							for (j = pDET->AFC_cnt[snd_ch]; j <= max_cnt + 5; j++)
								pDET->AFC_bit_buf[snd_ch][j] = 0.95f*pDET->AFC_bit_buf[snd_ch][j];

						pDET->AFC_cnt[snd_ch] = 0;
						pDET->AFC_bit_osc[snd_ch] = pDET->AFC_bit_osc[snd_ch] - 1;
					}
				}
				if (fec_ch == NR_FEC_CH)
				{
					tap_cnt1++;
					k++;
				}
			}
		}
		dcnt = (dcnt + 1) % n_INTR[snd_ch];
	}
}

#endif

void  make_rx_frame_FX25(int snd_ch, int rcvr_nr, int emph, string * data)
{
	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	word len, crc1, crc2;

	len = data->Length;
	
	if (len < pkt_raw_min_len)
	{
		free(data);
		return;
	}

	crc1 = get_fcs(data->Data, len - 2);
	crc2 = (data->Data[len - 1] << 8) | data->Data[len - 2];

	if (crc1 != crc2)
	{
		freeString(data);
		return;
	}
	Debugprintf("FEC Good CRC %x Len %d chan %d rcvr %d emph %d", crc1, len, snd_ch, rcvr_nr, emph);

	pDET->rx_decoded = decodedFEC;

	if (detect_list[snd_ch].Count > 0)
	{
		//if detect_list[snd_ch].IndexOf(data)<0 then

		if (my_indexof(&detect_list[snd_ch], data) < 0)
		{
			string * xx = newString();
			xx->Length = sprintf(xx->Data, "FX25 %d", centreFreq[snd_ch]);


			Add(&detect_list_c[snd_ch], xx);
			Add(&detect_list[snd_ch], data);

			stringAdd(xx, "", 0);
		}
		else
		{
			// Should check if previous decode was Single or MEM and if so replace

			Debugprintf("Discarding copy rcvr %d", rcvr_nr);
			freeString(data);
		}
	}
	else
	{
		string * xx = newString();
		xx->Length = sprintf(xx->Data, "FX25 %d", centreFreq[snd_ch]);

		Add(&detect_list_c[snd_ch], xx);
		Add(&detect_list[snd_ch], data);

		if (rcvr_nr == 0)
			pDET->emph_decoded = 3; //FX.25
	}

}

	
string * decode_FX25_data(TFX25 fx25)
{
	integer eras_pos = 0, i, j, len, rs_res;
	Byte a, k;
	Byte bit, byte_rx, bit_stuff_cnt, bit_cnt = 0, frame_status, bit_stream;

	string * data = newString();

	int done;
	Byte rs_block[256];
	int RSOK;

	bit_stream = 0;
	len = fx25.size - fx25.rs_size;
	frame_status = FRAME_WAIT;

	done = 0;

	// RS FEC

	memset(rs_block, 0, 255);
	memcpy(rs_block, fx25.data.Data, len);
	memcpy(&rs_block[255 - fx25.rs_size], &fx25.data.Data[len], fx25.rs_size);

	rs_res = fx25_decode_rs(rs_block, &eras_pos, 0, 0, fx25.rs_size);

	if (rs_res == -1)
	{
		Debugprintf("RS Correction Failed");
		return data;
	}

	if (rs_res == 0)
		Debugprintf("RS No Errors");
	else
		Debugprintf("RS %d Errors Corrected", rs_res);

	// need to do ax.25 decode of bit stream

	i = 0;
	
	while (i < len)
	{
		a = rs_block[i];
		i++;
		for (k = 0; k < 8; k++)
		{
			bit = a << 7;
			a = a >> 1;

			bit_stream = (bit_stream >> 1) | bit;

			if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
			{
				frame_status = FRAME_WAIT;

				if (bit_cnt == 6 && data->Length)
					return data;
			}

			if (frame_status == FRAME_LOAD)
			{
				if (bit_stuff_cnt == 5)
					bit_stuff_cnt = 0;
				else
				{
					if (bit == RX_BIT1)
						bit_stuff_cnt++;
					else 
						bit_stuff_cnt = 0;

					byte_rx = (byte_rx >> 1) | bit;
					bit_cnt++;
				}

				if (bit_cnt == 8)
				{
					stringAdd(data, &byte_rx, 1);
					bit_cnt = 0;
				}
			}

			if ((bit_stream && FRAME_FLAG == FRAME_FLAG) && frame_status == FRAME_WAIT)
			{
				frame_status = FRAME_LOAD;
				bit_cnt = 0;
				bit_stuff_cnt = 0;
				data->Length = 0;
			}
		}
	}
	return data;
}

int FX25_corr[4] = {1, 1, 1, 1};

#define tags_nr 11
#define tt 8

unsigned long long tags[tags_nr] =
{
	0xB74DB7DF8A532F3E, 0x26FF60A600CC8FDE, 0xC7DC0508F3D9B09E, 0x8F056EB4369660EE,
	0x6E260B1AC5835FAE, 0xFF94DC634F1CFF4E, 0x1EB7B9CDBC09C00E, 0xDBF869BD2DBB1776,
	0x3ADB0C13DEAE2836, 0xAB69DB6A543188D6, 0x4A4ABEC4A724B796
};

int sizes[tags_nr] = { 255, 144, 80, 48, 255, 160, 96, 64, 255, 192, 128 };
int rs_sizes[tags_nr] = { 16, 16, 16, 16, 32, 32, 32, 32, 64, 64, 64 };

/*
unsigned char get_corr_arm(unsigned long long n)
{
	unsigned char  max_corr;
	unsigned char result = 255;
	int i = 0;

	while (i < tags_nr)
	{
		if (__builtin_popcountll(n ^ tags[i] <= tt))
			return i;
	}

	return 255;
}
*/

char errors;


unsigned char get_corr(unsigned long long val)
{
	unsigned long v;
	unsigned long long n;
	int i = 0;

	while (i < tags_nr)
	{
		n = val ^ tags[i];

		v = n;

		v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
		errors = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

		if (errors > tt)
		{
			i++;
			continue;
		}

		v = n >> 32;

		v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
		errors += ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count

		if (errors <= tt)
			return i;

		i++;
	}
	return 255;
}



void decode_stream_FSK(int last, int snd_ch, int rcvr_nr, int emph, float * src_buf, float * bit_buf, int  buf_size, string * data)
{
	int i, k, j, n;
	UCHAR bit;
	UCHAR raw_bit;
	UCHAR raw_bit1;
	UCHAR raw_bit2;
	float AFC, x, amp, k1, k2;
	float baudrate;
	float div_bit_afc;
	word max_cnt;
	float threshold;
	float tr;
	float Freq;

	Byte sample_cnt;
	float PkAmp, PkAmpMax = 0, MaxAmp, MinAmp, AverageAmp;
	int newpkpos;
	float bit_osc;
	Byte last_rx_bit, bit_stream, frame_status;

	TFX25 fx25;
	unsigned long long tag64;
	boolean rx_fx25_mode;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	last_rx_bit = pDET->last_rx_bit[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PkAmp = pDET->PkAmp[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	MaxAmp = pDET->MaxAmp[snd_ch];
	MinAmp = pDET->MinAmp[snd_ch];
	AverageAmp = pDET->AverageAmp[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	frame_status = pDET->frame_status[snd_ch];

	fx25 = pDET->fx25[snd_ch];

	if (fx25_mode[snd_ch] == FX25_MODE_NONE)
	{
		rx_fx25_mode = FALSE;
		fx25.status = FX25_TAG;
	}
	else
		rx_fx25_mode = TRUE;


	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	// src_buf is input samples processed in some way.
	// not sure what bit_buf is, but guess bits extracted from samples
	// but then why floats ??

	baudrate = 300;

	div_bit_afc = 1.0f / roundf(BIT_AFC*(RX_Samplerate / 11025.0f));

	x = baudrate / RX_Samplerate;

	// I guess max_cnt is samples per bit

	//was - why + 1 then round??
	max_cnt = roundf(RX_Samplerate / baudrate) + 1;

	max_cnt = (RX_Samplerate / baudrate);

	for (i = 0; i < buf_size; i++)
	{
		// Seems to be accumulating squares of all samples in the input for one bit period

		bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt] + 0.05*src_buf[i] * src_buf[i];

		// Check for NAN

		if (bit_buf[sample_cnt] != bit_buf[sample_cnt])
			bit_buf[sample_cnt] = 0.0f;

		// Находим максимум в буфере синхронизации
		// Find the maximum in the synchronization buffer

		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmp = src_buf[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}
		sample_cnt++;

		bit_osc = bit_osc + x;

		// This seems to be how it does samples to bits


		if (bit_osc >= 0.99f)					// Allow for rounding errors
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95f*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;
			AFC = div_bit_afc * k2;

			if (k1 > 0.5f)
				bit_osc = bit_osc + AFC;
			else
				bit_osc = bit_osc - AFC;

			PkAmpMax = 0;
			sample_cnt = 0;

			// Not sure about this, but if bit_buf gets to NaN it stays there

			bit_osc = bit_osc - 1;
			//DCD feature
			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96f + newpkpos * 0.04f;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96f + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04f;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
					dcd_bit_cnt[snd_ch]++;
				else
					dcd_bit_cnt[snd_ch]--;
			}

			amp = PkAmp;

			if (amp > 0)
				raw_bit1 = RX_BIT1;
			else
				raw_bit1 = RX_BIT0;
			//
			if (amp > 0)
				MaxAmp = MaxAmp * 0.9f + amp*0.1f; //0.9  
			else
				MinAmp = MinAmp * 0.9f + amp*0.1f;

			amp = amp - (MaxAmp + MinAmp)*0.5f;

			//  Bit-detector

			AverageAmp = AverageAmp * 0.5f + amp*0.5f;
			threshold = 0.5f * AverageAmp;

			if (amp > threshold)
				raw_bit = RX_BIT1;
			else
				raw_bit = RX_BIT0;

			// 0.75

			if (amp > 0.75*AverageAmp)
				raw_bit2 = RX_BIT1;
			else
				raw_bit2 = RX_BIT0;

			if (raw_bit != raw_bit2)
				raw_bit1 = raw_bit2;

			// look for il2p before nrzi 

			if (il2p_mode[snd_ch])
			{
				il2p_rec_bit(snd_ch, rcvr_nr, emph, raw_bit);
				if (il2p_mode[snd_ch] == IL2P_MODE_ONLY)		// Dont try HDLC decode
					continue;
			}
			//NRZI

			if (raw_bit == last_rx_bit)
				bit = RX_BIT1;
			else
				bit = RX_BIT0;

			last_rx_bit = raw_bit;
			//
			bit_stream = (bit_stream >> 1) | bit;

			// DCD on flag

			if (last)
			{
				if (dcd_hdr_cnt[snd_ch] > 0)
					dcd_hdr_cnt[snd_ch]--;

				DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

				if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
				{
					dcd_hdr_cnt[snd_ch] = 48;
					dcd_on_hdr[snd_ch] = 1;
				}
			}

			// FX25 process

			if (rx_fx25_mode)
			{
				if (fx25.status == FX25_LOAD)
				{
					//if last then DCD_on_hdr[snd_ch]:=true;

					fx25.byte_rx = (fx25.byte_rx >> 1) | bit;
					fx25.bit_cnt++;

					if (fx25.bit_cnt == 8)
					{
						fx25.bit_cnt = 0;
						stringAdd(&fx25.data, &fx25.byte_rx, 1);
						fx25.size_cnt++;
						if (fx25.size == fx25.size_cnt)
						{
							fx25.status = FX25_TAG;
							make_rx_frame_FX25(snd_ch, rcvr_nr, emph, decode_FX25_data(fx25));
							//if last and (DCD_hdr_cnt[snd_ch]=0) then DCD_on_hdr[snd_ch]:=false;
						}
					}
				}
				else
				{
					fx25.size = 0;

					fx25.tag = (fx25.tag >> 1);
					if (bit)
						fx25.tag |= 0x8000000000000000;

					tag64 = fx25.tag & 0XFFFFFFFFFFFFFFFE;

					// FX25 tag correlation

					if (FX25_corr[snd_ch])
					{
						unsigned char res;

						res = get_corr(tag64);

						if (res < tags_nr)
						{
							Debugprintf("Got FEC Tag %d Errors %d", res, errors);
							fx25.size = sizes[res];
							fx25.rs_size = rs_sizes[res];
						}
					}
					else
					{
						if (tag64 == 0xB74DB7DF8A532F3E)
						{
							fx25.size = 255;
							fx25.rs_size = 16;
						}
						if (tag64 == 0x26FF60A600CC8FDE)
						{
							fx25.size = 144;
							fx25.rs_size = 16;
						}
						if (tag64 == 0xC7DC0508F3D9B09E)
						{
							fx25.size = 80;
							fx25.rs_size = 16;
						}
						if (tag64 == 0x8F056EB4369660EE)
						{
							fx25.size = 48;
							fx25.rs_size = 16;
						}
						if (tag64 == 0x6E260B1AC5835FAE)
						{
							fx25.size = 255;
							fx25.rs_size = 32;
						}
						if (tag64 == 0xFF94DC634F1CFF4E)
						{
							fx25.size = 160;
							fx25.rs_size = 32;
						}
						if (tag64 == 0x1EB7B9CDBC09C00E)
						{
							fx25.size = 96;
							fx25.rs_size = 32;
						}
						if (tag64 == 0xDBF869BD2DBB1776)
						{
							fx25.size = 64;
							fx25.rs_size = 32;
						}
						if (tag64 == 0x3ADB0C13DEAE2836)
						{
							fx25.size = 255;
							fx25.rs_size = 64;
						}
						if (tag64 == 0xAB69DB6A543188D6)
						{
							fx25.size = 192;
							fx25.rs_size = 64;
						}
						if (tag64 == 0x4A4ABEC4A724B796)
						{
							fx25.size = 128;
							fx25.rs_size = 64;
						}
					}
					if (fx25.size != 0)
					{
						fx25.status = FX25_LOAD;
						fx25.data.Length = 0;
						fx25.bit_cnt = 0;
						fx25.size_cnt = 0;
						centreFreq[snd_ch] = GuessCentreFreq(snd_ch);
					}
				}
			}
			//


			if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
			{
				// All have 7 or more 1 bits

				if (frame_status == FRAME_LOAD)
				{
					// Have started receiving frame 

//					Debugprintf("Frame Abort len= %d bits", pDET->raw_bits[snd_ch].Length);

					frame_status = FRAME_WAIT;

					// Raw stream init

					pDET->raw_bits[snd_ch].Length = 0;
					pDET->raw_bits1[snd_ch].Length = 0;
					pDET->last_nrzi_bit[snd_ch] = raw_bit;

//					dcd_hdr_cnt[snd_ch] = 48;
//					dcd_on_hdr[snd_ch] = 1;


					if (last)
						chk_dcd1(snd_ch, buf_size);
				}
				continue;
			}

			if (((bit_stream & FRAME_FLAG) == FRAME_FLAG) && (frame_status == FRAME_LOAD))
			{
				frame_status = FRAME_WAIT;

				if (pDET->raw_bits[snd_ch].Length == 7)		// Another flag
				{
					// Raw stream init

					pDET->raw_bits[snd_ch].Length = 0;
					pDET->raw_bits1[snd_ch].Length = 0;
					pDET->last_nrzi_bit[snd_ch] = raw_bit;
				}

				if (pDET->raw_bits[snd_ch].Length > 7)
				{
//b					Debugprintf("Got Frame len = %d AFC %f", pDET->raw_bits[snd_ch].Length, AFC);
					centreFreq[snd_ch] = GuessCentreFreq(snd_ch);
					make_rx_frame(snd_ch, rcvr_nr, emph, pDET->last_nrzi_bit[snd_ch], &pDET->raw_bits[snd_ch], &pDET->raw_bits1[snd_ch]);
				}
			}


			if (frame_status == FRAME_LOAD)
			{
				//Raw stream

				if (pDET->raw_bits[snd_ch].Length < 36873)
				{
					if (raw_bit == RX_BIT1)
						stringAdd(&pDET->raw_bits[snd_ch], "1", 1);
					else
						stringAdd(&pDET->raw_bits[snd_ch], "0", 1);
				}

				if (pDET->raw_bits1[snd_ch].Length < 36873)
				{
					if (raw_bit1 == RX_BIT1)
						stringAdd(&pDET->raw_bits1[snd_ch], "1", 1);
					else
						stringAdd(&pDET->raw_bits1[snd_ch], "0", 1);
				}
				//
			}

			if (((bit_stream & FRAME_FLAG) == FRAME_FLAG) && (frame_status == FRAME_WAIT))
			{
				frame_status = FRAME_LOAD;

				// Raw stream init

				pDET->raw_bits[snd_ch].Length = 0;
				pDET->raw_bits1[snd_ch].Length = 0;
				pDET->last_nrzi_bit[snd_ch] = raw_bit;

				//				Debugprintf("New Frame");
			}

		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PkAmp[snd_ch] = PkAmp;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->MaxAmp[snd_ch] = MaxAmp;
	pDET->MinAmp[snd_ch] = MinAmp;
	pDET->AverageAmp[snd_ch] = AverageAmp;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->last_rx_bit[snd_ch] = last_rx_bit;
	pDET->fx25[snd_ch] = fx25;
}


void decode_stream_BPSK(int last, int snd_ch, int rcvr_nr, int emph, float * srcI, float * srcQ, float * bit_buf, int  buf_size, string * data)
{
	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	int i, k, j, n;
	Byte dibit, bit;
	single afc, x, amp, k1, k2;
	single baudrate;
	single div_bit_afc;
	word max_cnt;
	single threshold;
	single tr;
	single KCorr, AngleCorr, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	Byte newpkpos, sample_cnt;
	single PkAmpI, PkAmpQ, PkAmpMax, PSK_AGC;
	single PSK_IZ1, PSK_QZ1;
	single bit_osc;
	Byte bit_stuff_cnt, last_rx_bit, frame_status, bit_cnt, bit_stream, byte_rx;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	// global -> local

	AngleCorr = pDET->AngleCorr[snd_ch];
	bit_stuff_cnt = pDET->bit_stuff_cnt[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PSK_AGC = pDET->PSK_AGC[snd_ch];
	PkAmpI = pDET->PkAmpI[snd_ch];
	PkAmpQ = pDET->PkAmpQ[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	PSK_IZ1 = pDET->PSK_IZ1[snd_ch];
	PSK_QZ1 = pDET->PSK_QZ1[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	bit_cnt = pDET->bit_cnt[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	byte_rx = pDET->byte_rx[snd_ch];

	//
	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	baudrate = 300;
	div_bit_afc = 1.0f / round(BIT_AFC*(RX_Samplerate / 11025));
	x = baudrate / RX_Samplerate;

//	max_cnt = round(RX_Samplerate / baudrate) + 1;
	max_cnt = round(RX_Samplerate / baudrate) + 1;

	for (i = 0; i < buf_size - 1; i++)
	{
		// AGC
		amp = sqrt(srcI[i] * srcI[i] + srcQ[i] * srcQ[i]);

		if (amp > PSK_AGC)

			PSK_AGC = PSK_AGC * agc_fast1 + amp*agc_fast;
		else
			PSK_AGC = PSK_AGC * agc_slow1 + amp*agc_slow;

		if (PSK_AGC > 1)

		{
			srcI[i] = srcI[i] / PSK_AGC;
			srcQ[i] = srcQ[i] / PSK_AGC;
			amp = amp / PSK_AGC; // Вместо SQRT
		}
		//
		bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt] + 0.05*amp;
		// Находим максимум в буфере синхронизации
		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmpI = srcI[i];
			PkAmpQ = srcQ[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}
	
		sample_cnt++;

		bit_osc = bit_osc + x;

		if (bit_osc >= 1)
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;

			afc = div_bit_afc * k2;

			if (k1 > 0.5f)
				bit_osc = bit_osc + afc;
			else
				bit_osc = bit_osc - afc;

			PkAmpMax = 0;
			sample_cnt = 0;
			bit_osc = bit_osc - 1;

			//DCD feature
			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96f + newpkpos * 0.04f;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96f + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04f;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
				else
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
			}

			// Bit-detector

			muxI1 = PkAmpI * PSK_IZ1;
			muxI2 = PkAmpQ * PSK_IZ1;
			muxQ1 = PkAmpQ * PSK_QZ1;
			muxQ2 = PkAmpI * PSK_QZ1;
			sumIQ1 = muxI1 + muxQ1;
			sumIQ2 = muxI2 - muxQ2;
			angle = atan2f(sumIQ2, sumIQ1);
			PSK_IZ1 = PkAmpI;
			PSK_QZ1 = PkAmpQ;

			float Mag = sqrtf(powf(PSK_IZ1, 2) + powf(PSK_QZ1, 2));

			// Phase corrector

			if (fabsf(angle) < PI5)
				AngleCorr = AngleCorr * 0.95f - angle * 0.05f;
			else
			{
				if (angle > 0)
					AngleCorr = AngleCorr * 0.95f + (pi - angle)*0.05f;
				else
					AngleCorr = AngleCorr * 0.95f - (pi + angle)*0.05f;
			}

			angle = angle + AngleCorr;
			//

			if (fabsf(angle) < PI5)
				bit = RX_BIT1;
			else
				bit = RX_BIT0;
			//

			//	is this the best place to store phase for constellation?
			// only for ilp2 for now

			if (il2p_mode[snd_ch])
			{
				struct il2p_context_s * il2p = il2p_context[snd_ch][rcvr_nr][emph];

				if (il2p && il2p->state > IL2P_SEARCHING)
				{
					Phases[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]] = angle;
					Mags[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]++] = Mag;
					if (nPhases[snd_ch][rcvr_nr][emph] > 4090)
						nPhases[snd_ch][rcvr_nr][emph]--;
				}
				il2p_rec_bit(snd_ch, rcvr_nr, emph, bit);
				if (il2p_mode[snd_ch] == IL2P_MODE_ONLY)		// Dont try HDLC decode
					continue;
			}
			if (bit)
				stats[1]++;
			else
				stats[0]++;

			bit_stream = (bit_stream >> 1) | bit;

			// DCD on flag

			if (last)
			{
				if (dcd_hdr_cnt[snd_ch] > 0)
					dcd_hdr_cnt[snd_ch]--;

				DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

				if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
					((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
				{
					dcd_hdr_cnt[snd_ch] = 48;
					dcd_on_hdr[snd_ch] = 1;
				}
			}


			// I think Andy looks for both flag and abort here. I think it would be
			// clearer to detect abort separately

			// This may not be optimun but should work

			if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
			{
				// All have 7 or more 1 bits

				if (frame_status == FRAME_LOAD)
				{
					// Have started receiving frame

//	  				Debugprintf("Frame Abort len= %d bytes", data->Length);

					frame_status = FRAME_WAIT;

					// Raw stream init

					bit_cnt = 0;
					bit_stuff_cnt = 0;
					data->Length = 0;
				}
				continue;
			}


			if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
			{
				frame_status = FRAME_WAIT;
				//				if (bit_cnt == 6)
				make_rx_frame_PSK(snd_ch, rcvr_nr, emph, data);
			}

			if (frame_status == FRAME_LOAD)
			{
				if (bit_stuff_cnt == 5)
					bit_stuff_cnt = 0;
				else
				{
					if (bit == RX_BIT1)
						bit_stuff_cnt++;
					else
						bit_stuff_cnt = 0;

					byte_rx = (byte_rx >> 1) + bit;
					bit_cnt++;
				}

				if (bit_cnt == 8)
				{
					if (data->Length < 4097)
						stringAdd(data, &byte_rx, 1);
					bit_cnt = 0;
				}
			}
			if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_WAIT)
			{
				frame_status = FRAME_LOAD;
				bit_cnt = 0;
				bit_stuff_cnt = 0;
				data->Length = 0;
			}
		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PSK_AGC[snd_ch] = PSK_AGC;
	pDET->PkAmpI[snd_ch] = PkAmpI;
	pDET->PkAmpQ[snd_ch] = PkAmpQ;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->PSK_IZ1[snd_ch] = PSK_IZ1;
	pDET->PSK_QZ1[snd_ch] = PSK_QZ1;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->bit_cnt[snd_ch] = bit_cnt;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->byte_rx[snd_ch] = byte_rx;
	pDET->bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
	pDET->AngleCorr[snd_ch] = AngleCorr;
}


void decode_stream_QPSK(int last, int snd_ch, int rcvr_nr, int emph, float * srcI, float * srcQ, float * bit_buf, int  buf_size, string * data)
{
	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	int i, k, j, n;
	Byte dibit = 0, bit;
	single afc, x, amp, k1, k2;
	single baudrate;
	single div_bit_afc;
	word max_cnt;
	single threshold;
	single tr;
	single KCorr = 0, AngleCorr, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	Byte newpkpos, sample_cnt;
	single PkAmpI, PkAmpQ, PkAmpMax, PSK_AGC;
	single PSK_IZ1, PSK_QZ1;
	single bit_osc;
	Byte bit_stuff_cnt, last_rx_bit, frame_status, bit_cnt, bit_stream, byte_rx;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	bit_stuff_cnt = pDET->bit_stuff_cnt[snd_ch];
	last_rx_bit = pDET->last_rx_bit[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PSK_AGC = pDET->PSK_AGC[snd_ch];
	PkAmpI = pDET->PkAmpI[snd_ch];
	PkAmpQ = pDET->PkAmpQ[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	PSK_IZ1 = pDET->PSK_IZ1[snd_ch];
	PSK_QZ1 = pDET->PSK_QZ1[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	bit_cnt = pDET->bit_cnt[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	byte_rx = pDET->byte_rx[snd_ch];
	AngleCorr = pDET->AngleCorr[snd_ch];

	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	// I think this works because of upsampling - 1200 = 4x 300 and we upsampled by 4

	baudrate = 300;

	div_bit_afc = 1.0f / roundf(BIT_AFC*(RX_Samplerate / 11025.0f));

	x = baudrate / RX_Samplerate;

	max_cnt = roundf(RX_Samplerate / baudrate) + 1;

	for (i = 0; i < buf_size; i++)
	{
		// AGC
		amp = sqrt(srcI[i] * srcI[i] + srcQ[i] * srcQ[i]);

		if (amp > PSK_AGC)
			PSK_AGC = PSK_AGC * agc_fast1 + amp * agc_fast;
		else
			PSK_AGC = PSK_AGC * agc_slow1 + amp * agc_slow;

		if (PSK_AGC > 1)
		{
			srcI[i] = srcI[i] / PSK_AGC;
			srcQ[i] = srcQ[i] / PSK_AGC;

			amp = amp / PSK_AGC; // Вместо SQRT
		}

		bit_buf[sample_cnt] = 0.95 *  bit_buf[sample_cnt] + 0.05 * amp;

		// Check for NAN

		if (bit_buf[sample_cnt] != bit_buf[sample_cnt])
			bit_buf[sample_cnt] = 0.0f;

		// Find the maximum in the synchronization buffer

		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmpI = srcI[i];
			PkAmpQ = srcQ[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}

		sample_cnt++;

		bit_osc = bit_osc + x;

		// This seems to be how it does samples to bits

		if (bit_osc >= 1)
		{
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;
			afc = div_bit_afc * k2;

			if (k1 > 0.5)
				bit_osc = bit_osc + afc;
			else
				bit_osc = bit_osc - afc;

			PkAmpMax = 0;
			sample_cnt = 0;
			bit_osc = bit_osc - 1;
			//DCD feature

			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96 + newpkpos * 0.04;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96 + fabsf(newpkpos - DCD_LastPkPos[snd_ch])*0.04;

				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001f)
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
				else
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
			}

			// Bit-detector

			muxI1 = PkAmpI * PSK_IZ1;
			muxI2 = PkAmpQ * PSK_IZ1;
			muxQ1 = PkAmpQ * PSK_QZ1;
			muxQ2 = PkAmpI * PSK_QZ1;
			sumIQ1 = muxI1 + muxQ1;
			sumIQ2 = muxI2 - muxQ2;
			angle = atan2f(sumIQ2, sumIQ1);
			PSK_IZ1 = PkAmpI;
			PSK_QZ1 = PkAmpQ;

			float Mag = sqrtf(powf(PSK_IZ1, 2) + powf(PSK_QZ1, 2));

			if (angle > pi || angle < -pi)
				angle = angle;

			if (modem_mode[snd_ch] == MODE_PI4QPSK)
			{
				// Phase corrector

				// I'm pretty sure we send 4 phases starting 45 degrees from 0 so .25, .75 - .25 - .75

				if (angle >= 0 && angle <= PI5)
					KCorr = angle - PI25;

				if (angle > PI5)
					KCorr = angle - PI75;

				if (angle < -PI5)
					KCorr = angle + PI75;

				if (angle < 0 && angle >= -PI5)
					KCorr = angle + PI25;

				AngleCorr = AngleCorr * 0.95f - KCorr * 0.05f;
				angle = angle + AngleCorr;

				if (angle >= 0 && angle <= PI5)
				{
					dibit = qpsk_set[snd_ch].rx[0];		// 00 - 0
					qpsk_set[snd_ch].count[0]++;
				}
				else if (angle > PI5)
				{
					dibit = qpsk_set[snd_ch].rx[1];		// 01 - PI/2
					qpsk_set[snd_ch].count[1]++;
				}
				else if (angle < -PI5)
				{
					dibit = qpsk_set[snd_ch].rx[2];		// 10 - PI
					qpsk_set[snd_ch].count[2]++;
				}
				else if (angle < 0 && angle >= -PI5)
				{
					dibit = qpsk_set[snd_ch].rx[3];		// 11 - -PI/2
					qpsk_set[snd_ch].count[3]++;
				}
			}
			else
			{
				// "Normal" QPSK
	
				// Phase corrector

				// I think this sends 0 90 180 270

				if (fabsf(angle) < PI25)
					KCorr = angle;

				if (angle >= PI25 && angle <= PI75)
					KCorr = angle - PI5;

				if (angle <= -PI25 && angle >= -PI75)
					KCorr = angle + PI5;

				if (fabsf(angle) > PI75)
				{
					if (angle > 0)
						KCorr = -M_PI + angle;
					else
						KCorr = M_PI + angle;
				}

				AngleCorr = AngleCorr * 0.95 - KCorr * 0.05;
				angle = angle + AngleCorr;

				if (fabsf(angle) < PI25)
					dibit = qpsk_set[snd_ch].rx[0];							// 00 - 0
				else if (angle >= PI25 && angle <= PI75)
					dibit = qpsk_set[snd_ch].rx[1];				// 01 - PI/2
				else if (fabsf(angle) > PI75)
					dibit = qpsk_set[snd_ch].rx[2];					// 10 - PI
				else if (angle <= -PI25 && angle >= -PI75)
					dibit = qpsk_set[snd_ch].rx[3];					// 11 - -PI/2
			}

			for (j = 0; j < 2; j++)
			{
				dibit = dibit << 1;

				//	is this the best place to store phase for constellation?
				// only for ilp2 for now

				if (il2p_mode[snd_ch])
				{
					struct il2p_context_s * il2p = il2p_context[snd_ch][rcvr_nr][emph];

					if (il2p && il2p->state > IL2P_SEARCHING)
					{
						Phases[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]] = angle;
						Mags[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]++] = Mag;
						if (nPhases[snd_ch][rcvr_nr][emph] > 4090)
							nPhases[snd_ch][rcvr_nr][emph]--;
					}

					il2p_rec_bit(snd_ch, rcvr_nr, emph, (dibit & RX_BIT1));
					if (il2p_mode[snd_ch] == IL2P_MODE_ONLY)		// Dont try HDLC decode
						continue;
				}

				// NRZI

				if (last_rx_bit == (dibit & RX_BIT1))
					bit = RX_BIT1;
				else
					bit = RX_BIT0;

				last_rx_bit = dibit & RX_BIT1;
			
				bit_stream = (bit_stream >> 1) | bit;

				// DCD on flag

				if (last)
				{
					if (dcd_hdr_cnt[snd_ch] > 0)
						dcd_hdr_cnt[snd_ch]--;

					DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

					if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
					{
						dcd_hdr_cnt[snd_ch] = 48;
						dcd_on_hdr[snd_ch] = 1;
					}
				}


				// I think Andy looks for both flag and abort here. I think it would be
				// clearer to detect abort separately

				// This may not be optimun but should work

				if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
				{
					// All have 7 or more 1 bits

					if (frame_status == FRAME_LOAD)
					{
						// Have started receiving frame

	//					Debugprintf("Frame Abort len= %d bytes", data->Length);

						frame_status = FRAME_WAIT;

						// Raw stream init

						bit_cnt = 0;
						bit_stuff_cnt = 0;
						data->Length = 0;
					}
					continue;
				}

				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
				{
					frame_status = FRAME_WAIT;
					//				if (bit_cnt == 6)
					make_rx_frame_PSK(snd_ch, rcvr_nr, emph, data);
				}

				if (frame_status == FRAME_LOAD)
				{
					if (bit_stuff_cnt == 5)
						bit_stuff_cnt = 0;
					else
					{
						if (bit == RX_BIT1)
							bit_stuff_cnt++;
						else
							bit_stuff_cnt = 0;

						byte_rx = (byte_rx >> 1) + bit;
						bit_cnt++;

					}

					if (bit_cnt == 8)
					{
						if (data->Length < 4097)
							stringAdd(data, &byte_rx, 1);
						bit_cnt = 0;
					}
				}
				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_WAIT)
				{
					frame_status = FRAME_LOAD;
					bit_cnt = 0;
					bit_stuff_cnt = 0;
					data->Length = 0;
				}
			}
		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PSK_AGC[snd_ch] = PSK_AGC;
	pDET->PkAmpI[snd_ch] = PkAmpI;
	pDET->PkAmpQ[snd_ch] = PkAmpQ;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->PSK_IZ1[snd_ch] = PSK_IZ1;
	pDET->PSK_QZ1[snd_ch] = PSK_QZ1;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->bit_cnt[snd_ch] = bit_cnt;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->byte_rx[snd_ch] = byte_rx;
	pDET->last_rx_bit[snd_ch] = last_rx_bit;
	pDET->bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
	pDET->AngleCorr[snd_ch] = AngleCorr;
}

void decode_stream_8PSK(int last, int snd_ch, int rcvr_nr, int emph, float * srcI, float * srcQ, float * bit_buf, int  buf_size, string * data)
{
	float agc_fast = 0.01f;
	float agc_fast1 = 1 - agc_fast;
	float agc_slow = agc_fast / 4;
	float agc_slow1 = 1 - agc_slow;

	int i, k, j, n;
	Byte tribit = 0, bit;
	single afc, x, amp, k1, k2;
	single baudrate;
	single div_bit_afc;
	word max_cnt;
	single threshold;
	single tr;
	single KCorr = 0, AngleCorr, angle, muxI1, muxQ1, muxI2, muxQ2, sumIQ1, sumIQ2;
	Byte newpkpos, sample_cnt;
	single PkAmpI, PkAmpQ, PkAmpMax, PSK_AGC;
	single PSK_IZ1, PSK_QZ1;
	single bit_osc;
	Byte bit_stuff_cnt, last_rx_bit, frame_status, bit_cnt, bit_stream, byte_rx;

	// get saved values to local variables to speed up access

	struct TDetector_t * pDET = &DET[emph][rcvr_nr];

	bit_stuff_cnt = pDET->bit_stuff_cnt[snd_ch];
	last_rx_bit = pDET->last_rx_bit[snd_ch];
	sample_cnt = pDET->sample_cnt[snd_ch];
	PSK_AGC = pDET->PSK_AGC[snd_ch];
	PkAmpI = pDET->PkAmpI[snd_ch];
	PkAmpQ = pDET->PkAmpQ[snd_ch];
	PkAmpMax = pDET->PkAmpMax[snd_ch];
	newpkpos = pDET->newpkpos[snd_ch];
	PSK_IZ1 = pDET->PSK_IZ1[snd_ch];
	PSK_QZ1 = pDET->PSK_QZ1[snd_ch];
	bit_osc = pDET->bit_osc[snd_ch];
	frame_status = pDET->frame_status[snd_ch];
	bit_cnt = pDET->bit_cnt[snd_ch];
	bit_stream = pDET->bit_stream[snd_ch];
	byte_rx = pDET->byte_rx[snd_ch];
	AngleCorr = pDET->AngleCorr[snd_ch];

	tr = dcd_threshold * dcd_corr;

	if (last)
	{
		// Update DCD status

		if (dcd_hdr_cnt[snd_ch] == 0)
			dcd_on_hdr[snd_ch] = 0;

		dcd_bit_cnt[snd_ch] = 0;
	}

	// Not sure how this works

	if (tx_baudrate[snd_ch] == 300)
		baudrate = 300;
	else
		baudrate = 1600 / 6;

	div_bit_afc = 1.0 / round(BIT_AFC*(RX_Samplerate / 11025));
	x = baudrate / RX_Samplerate;
	max_cnt = round(RX_Samplerate / baudrate) + 1;
	for (i = 0; i < buf_size; i++)
	{
		// AGC
		amp = sqrt(srcI[i] * srcI[i] + srcQ[i] * srcQ[i]);
		if (amp > PSK_AGC)
			PSK_AGC = PSK_AGC * agc_fast1 + amp*agc_fast;
		else
			PSK_AGC = PSK_AGC * agc_slow1 + amp*agc_slow;

		if (PSK_AGC > 1)
		{
			srcI[i] = srcI[i] / PSK_AGC;
			srcQ[i] = srcQ[i] / PSK_AGC;
			amp = amp / PSK_AGC; // Вместо SQRT
		}

		bit_buf[sample_cnt] = 0.95*bit_buf[sample_cnt] + 0.05*amp;

		// Находим максимум в буфере синхронизации
		if (bit_buf[sample_cnt] > PkAmpMax)
		{
			PkAmpI = srcI[i];
			PkAmpQ = srcQ[i];
			PkAmpMax = bit_buf[sample_cnt];
			newpkpos = sample_cnt;
		}
		//

		sample_cnt++;

		bit_osc = bit_osc + x;

		if (bit_osc >= 1)
		{			
			if (sample_cnt <= max_cnt)
				for (k = sample_cnt; k <= max_cnt; k++)
					bit_buf[k] = 0.95*bit_buf[k];

			k1 = (1.0f * newpkpos) / (sample_cnt - 1);
			k2 = pila(k1) - 1;

			afc = div_bit_afc * k2;
			if (k1 > 0.5)
				 bit_osc = bit_osc + afc;
			else
				bit_osc = bit_osc - afc;

			PkAmpMax = 0;
			sample_cnt = 0;
			bit_osc = bit_osc - 1;
			//DCD feature
			if (last)
			{
				DCD_LastPkPos[snd_ch] = DCD_LastPkPos[snd_ch] * 0.96 + newpkpos * 0.04;
				DCD_LastPerc[snd_ch] = DCD_LastPerc[snd_ch] * 0.96 + abs(newpkpos - DCD_LastPkPos[snd_ch])*0.04;
				if (DCD_LastPerc[snd_ch] >= tr || DCD_LastPerc[snd_ch] < 0.00001)
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] + 1;
				else
					dcd_bit_cnt[snd_ch] = dcd_bit_cnt[snd_ch] - 1;
			}
			// Bit-detector
			muxI1 = PkAmpI * PSK_IZ1;
			muxI2 = PkAmpQ * PSK_IZ1;
			muxQ1 = PkAmpQ * PSK_QZ1;
			muxQ2 = PkAmpI * PSK_QZ1;
			sumIQ1 = muxI1 + muxQ1;
			sumIQ2 = muxI2 - muxQ2;
			angle = atan2f(sumIQ2, sumIQ1);
			PSK_IZ1 = PkAmpI;
			PSK_QZ1 = PkAmpQ;

			float Mag = sqrtf(powf(PSK_IZ1, 2) + powf(PSK_QZ1, 2));

			// Phase corrector

			if (fabsf(angle) < PI125)
				KCorr = angle;

			if (angle >= PI125 && angle <= PI375)
				KCorr = angle - PI25;
			if (angle >= PI375 && angle < PI625)
				KCorr = angle - PI5;
			if (angle >= PI625 && angle <= PI875)
				KCorr = angle - PI75;
			if (angle <= -PI125 && angle > -PI375)
				KCorr = angle + PI25;
			if (angle <= -PI375 && angle > -PI625)
				KCorr = angle + PI5;
			if (angle <= -PI625 && angle >= -PI875)
				KCorr = angle + PI75;

			if (fabsf(angle) > PI875)
			{
				if (angle > 0)
					KCorr = angle - pi;
				else
					KCorr = angle + pi;
			}

			AngleCorr = AngleCorr * 0.95 - KCorr * 0.05;
			angle = angle + AngleCorr;
			
			if (fabsf(angle) < PI125)
				tribit = 1;
			if (angle >= PI125 && angle < PI375)
				tribit = 0;
			if (angle >= PI375 && angle < PI625)
				tribit = 2;
			if (angle >= PI625 && angle <= PI875)
				tribit = 3;
			if (fabsf(angle) > PI875)
				tribit = 7;
			if (angle <= -PI625 && angle >= -PI875)
				tribit = 6;
			if (angle <= -PI375 && angle > -PI625)
				tribit = 4;
			if (angle <= -PI125 && angle > -PI375)
				tribit = 5;

			tribit = tribit << 4;

			for (j = 0; j < 3; j++)
			{
				tribit = tribit << 1;

				// look for il2p before nrzi 

				//	is this the best place to store phase for constellation?
				// only for ilp2 for now

				if (il2p_mode[snd_ch])
				{
					struct il2p_context_s * il2p = il2p_context[snd_ch][rcvr_nr][emph];
					
					if (il2p && il2p->state > IL2P_SEARCHING)
					{
						Phases[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]] = angle;
						Mags[snd_ch][rcvr_nr][emph][nPhases[snd_ch][rcvr_nr][emph]++] = Mag;
					if (nPhases[snd_ch][rcvr_nr][emph] > 4090)
						nPhases[snd_ch][rcvr_nr][emph]--;
					}

					il2p_rec_bit(snd_ch, rcvr_nr, emph, tribit & RX_BIT1);
					if (il2p_mode[snd_ch] == IL2P_MODE_ONLY)		// Dont try HDLC decode
						continue;
				}

				//NRZI

				if (last_rx_bit == (tribit & RX_BIT1))
					bit = RX_BIT1;
				else
					bit = RX_BIT0;

				last_rx_bit = tribit & RX_BIT1;
				//
				bit_stream = (bit_stream >> 1) | bit;

				// DCD on flag

				if (last)
				{
					if (dcd_hdr_cnt[snd_ch] > 0)
						dcd_hdr_cnt[snd_ch]--;

					DCD_header[snd_ch] = (DCD_header[snd_ch] >> 1) | (bit << 24);

					if (((DCD_header[snd_ch] & 0xFFFF0000) == 0x7E7E0000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x7E000000) ||
						((DCD_header[snd_ch] & 0xFFFFFF00) == 0x00000000))
					{
						dcd_hdr_cnt[snd_ch] = 48;
						dcd_on_hdr[snd_ch] = 1;
					}
				}


				// I think Andy looks for both flag and abort here. I think it would be
				// clearer to detect abort separately

				// This may not be optimun but should work

				if (bit_stream == 0xFF || bit_stream == 0x7F || bit_stream == 0xFE)
				{
					// All have 7 or more 1 bits

					if (frame_status == FRAME_LOAD)
					{
						// Have started receiving frame

	   //					Debugprintf("Frame Abort len= %d bytes", data->Length);

						frame_status = FRAME_WAIT;

						// Raw stream init

						bit_cnt = 0;
						bit_stuff_cnt = 0;
						data->Length = 0;
					}
					continue;
				}


				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_LOAD)
				{
					frame_status = FRAME_WAIT;
					if (bit_cnt == 6)
						make_rx_frame_PSK(snd_ch, rcvr_nr, emph, data);
				}
				if (frame_status == FRAME_LOAD)
				{
					if (bit_stuff_cnt == 5)
						bit_stuff_cnt = 0;
					else
					{
						if (bit == RX_BIT1)
							bit_stuff_cnt++;
						else
							bit_stuff_cnt = 0;

						byte_rx = (byte_rx >> 1) + bit;
						bit_cnt++;
					}
					if (bit_cnt == 8)
					{
						if (data->Length < 4097)
							stringAdd(data, &byte_rx, 1);
						bit_cnt = 0;
					}
				}

				if ((bit_stream & FRAME_FLAG) == FRAME_FLAG && frame_status == FRAME_WAIT)
				{
					frame_status = FRAME_LOAD;
					bit_cnt = 0;
					bit_stuff_cnt = 0;
					data->Length = 0;
				}
			}
		}
	}

	pDET->sample_cnt[snd_ch] = sample_cnt;
	pDET->PSK_AGC[snd_ch] = PSK_AGC;
	pDET->PkAmpI[snd_ch] = PkAmpI;
	pDET->PkAmpQ[snd_ch] = PkAmpQ;
	pDET->PkAmpMax[snd_ch] = PkAmpMax;
	pDET->newpkpos[snd_ch] = newpkpos;
	pDET->PSK_IZ1[snd_ch] = PSK_IZ1;
	pDET->PSK_QZ1[snd_ch] = PSK_QZ1;
	pDET->bit_osc[snd_ch] = bit_osc;
	pDET->frame_status[snd_ch] = frame_status;
	pDET->bit_cnt[snd_ch] = bit_cnt;
	pDET->bit_stream[snd_ch] = bit_stream;
	pDET->byte_rx[snd_ch] = byte_rx;
	pDET->last_rx_bit[snd_ch] = last_rx_bit;
	pDET->bit_stuff_cnt[snd_ch] = bit_stuff_cnt;
	pDET->AngleCorr[snd_ch] = AngleCorr;

}

/*

////////////////////////////////////////////////////////

function blackman(i,tap: word): single;
var
  a0,a1,a2,a: single;
{
  a = 0.16;
  a0 = (1-a)/2;
  a1 = 1/2;
  a2 = a/2;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1));
}

function nuttal(i,tap: word): single;
var
  a0,a1,a2,a3: single;
{
  a0 = 0.355768;
  a1 = 0.487396;
  a2 = 0.144232;
  a3 = 0.012604;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1))-a3*cos(6*pi*i/(tap-1));
}

function flattop(i,tap: word): single;
var
  a0,a1,a2,a3,a4: single;
{
  a0 = 1;
  a1 = 1.93;
  a2 = 1.29;
  a3 = 0.388;
  a4 = 0.032;
  result = a0-a1*cos(2*pi*i/(tap-1))+a2*cos(4*pi*i/(tap-1))-a3*cos(6*pi*i/(tap-1))+a4*cos(8*pi*i/(tap-1));
}
*/


void init_BPF(float freq1, float freq2, unsigned short tap, float samplerate, float * buf)
{
	unsigned short tap1, i;
	float tap12, ham, acc1, acc2;
	float bpf_l[2048];
	float bpf_h[2048];
	float itap12, pi2, x1, x2;

	acc1 = 0;
	acc2 = 0;
	tap1 = tap - 1;
	tap12 = tap1 / 2;
	pi2 = 2 * pi;
	x1 = pi2 * freq1 / samplerate;
	x2 = pi2 * freq2 / samplerate;
	for (i = 0; i <= tap1; i++)
	{
//		float x = (pi2 * i) / tap1;
//		x = cosf(x);
//		ham = 0.5 - 0.5 * x;

		ham = 0.5 - 0.5 * cosf((pi2 * i) / tap1); //old

		if (ham != ham)			// check for NaN
			ham = 0.0f;

		itap12 = i - tap12;

		if (itap12 == 0)
		{
			bpf_l[i] = x1;
			bpf_h[i] = x2;
		}
		else
		{
			bpf_l[i] = sinf(x1*itap12) / itap12;
			bpf_h[i] = sinf(x2*itap12) / itap12;
		}

		bpf_l[i] = bpf_l[i] * ham;
		bpf_h[i] = bpf_h[i] * ham;
		acc1 = acc1 + bpf_l[i];
		acc2 = acc2 + bpf_h[i];
	}

	for (i = 0; i <= tap1; i++)
	{
		bpf_l[i] = bpf_l[i] / acc1;
		bpf_h[i] = -(bpf_h[i] / acc2);
	};

	bpf_h[tap / 2] = bpf_h[tap / 2] + 1;

	for (i = 0; i <= tap; i++)
	{
		buf[i] = -(bpf_l[i] + bpf_h[i]);
	}
	buf[tap / 2] = buf[tap / 2] + 1;
}



void  init_LPF(float width, unsigned short tap, float samplerate, float * buf)
{
	float acc1, ham;
	unsigned short tap1, i;
	float itap12, tap12, x1, pi2;

	acc1 = 0;
	tap1 = tap - 1;
	tap12 = tap1 / 2;
	pi2 = 2 * pi;
	x1 = pi2 * width / samplerate;

	for (i = 0; i <= tap1; i++)
	{
		ham = 0.53836f - 0.46164f * cosf(pi2 * i / tap1); //old

		if (ham != ham)			// check for NaN
			ham = 0.0f;

	   //ham = 0.5-0.5*cos(pi2*i/tap1);
		//ham = 0.5*(1-cos(pi2*i/tap1)); //hann

		//ham = blackman(i,tap); //blackman
		//ham = nuttal(i,tap);

		itap12 = i - tap12;

		if (itap12 == 0)
			buf[i] = x1;
		else
			buf[i] = sinf(x1*itap12) / itap12;

		buf[i] = buf[i] * ham;
		acc1 = acc1 + buf[i];
	}
	for (i = 0; i <= tap1; i++)
		buf[i] = buf[i] / acc1;
}

void make_core_INTR(UCHAR snd_ch)
{
	float width;

	width = roundf(RX_Samplerate / 2);

	n_INTR[snd_ch] = 1;

	switch (speed[snd_ch])
	{
	case SPEED_300:

		width = roundf(RX_Samplerate / 2);
		n_INTR[snd_ch] = 1;
		break;

	case SPEED_P300:

		width = roundf(RX_Samplerate / 2);
		n_INTR[snd_ch] = 1;
		break;


	case SPEED_Q300:
	case SPEED_8PSK300:

		width = roundf(RX_Samplerate / 2);
		n_INTR[snd_ch] = 1;
		break;

	case SPEED_600:

		width = roundf(RX_Samplerate / 4);
		n_INTR[snd_ch] = 2;
		break;

	case SPEED_P600:

		width = roundf(RX_Samplerate / 4);
		n_INTR[snd_ch] = 2;
		break;

	case SPEED_1200:
		width = roundf(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_P1200:
		width = roundf(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

//	case SPEED_Q1200:
//		width = roundf(RX_Samplerate / 8);
//		n_INTR[snd_ch] = 4;
//		break;

	case SPEED_Q2400:
		width = 300;
		n_INTR[snd_ch] = 4;
		break; //8

	case SPEED_DW2400:

		width = 300;
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_2400V26B:

		width = 300;
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_MP400:

		width = round(RX_Samplerate / 8);
		n_INTR[snd_ch] = 4;
		break;

	case SPEED_Q3600:
		width = 300;
		n_INTR[snd_ch] = 6;//12
		break;

	case SPEED_8P4800:

		width = 100;
		n_INTR[snd_ch] = 6;
		break;

	case SPEED_2400:

		width = round(RX_Samplerate / 16);
		n_INTR[snd_ch] = 8;
		break;

	case SPEED_P2400:
		width = round(RX_Samplerate / 16);
		n_INTR[snd_ch] = 8;
		break;

	case SPEED_Q4800:
		width = 300;
		n_INTR[snd_ch] = 8;//16
		break;
	}


	init_LPF(width, INTR_tap[snd_ch], RX_Samplerate, INTR_core[snd_ch]);
}

void  make_core_LPF(UCHAR snd_ch, short width)
{
	if (modem_mode[snd_ch] == MODE_MPSK)
	{
		init_LPF(width, LPF_tap[snd_ch], RX_Samplerate / n_INTR[snd_ch], LPF_core[snd_ch]);
		init_LPF(rx_baudrate[snd_ch], LPF_tap[snd_ch], RX_Samplerate / n_INTR[snd_ch], AFC_core[snd_ch]);
	}
	else
		init_LPF(width, LPF_tap[snd_ch], RX_Samplerate, LPF_core[snd_ch]);
}


void  make_core_BPF(UCHAR snd_ch, short freq, short width)
{
	float old_freq, width2, rx_samplerate2, freq1, freq2;

	UCHAR i;

	freq = freq + rxOffset + chanOffset[snd_ch];

	// I want to run decoders lowest to highest to simplify my display,
	// so filters must be calculated in same order

	int offset = -(RCVR[snd_ch] * rcvr_offset[snd_ch]); // lowest

	rx_samplerate2 = 0.5 * RX_Samplerate;
	width2 = 0.5 * width;
	old_freq = freq;

	for (i = 0; i <= RCVR[snd_ch] << 1; i++)
	{
		freq = old_freq + offset;
	
		freq1 = freq - width2;
		freq2 = freq + width2;
		if (freq1 < 1)
			freq1 = 1;

		if (freq2 < 1)
			freq2 = 1;

		if (freq1 > rx_samplerate2)
			freq1 = rx_samplerate2;

		if (freq2 > rx_samplerate2)
			freq2 = rx_samplerate2;

		init_BPF(freq1, freq2, BPF_tap[snd_ch], RX_Samplerate, &DET[0][i].BPF_core[snd_ch][0]);

		offset += rcvr_offset[snd_ch];
	}
}



void make_core_TXBPF(UCHAR snd_ch, float freq, float width)
{
	float freq1, freq2;

	freq1 = freq - width / 2;
	freq2 = freq + width / 2;

	if (freq1 < 1)
		freq1 = 1;

	if (freq2 < 1)
		freq2 = 1;

	if (freq1 > TX_Samplerate / 2)
		freq1 = TX_Samplerate / 2;

	if (freq2 > TX_Samplerate / 2)
		freq2 = TX_Samplerate / 2;

	init_BPF(freq1, freq2, tx_BPF_tap[snd_ch], TX_Samplerate, tx_BPF_core[snd_ch]);
}




void interpolation(int snd_ch, int rcvr_nr, int emph, float * dest_buf, float * src_buf, int buf_size)
{
	int n_intr1, buf_size1, k, i, j;
	float buf[8192];

	buf_size1 = buf_size - 1;
	n_intr1 = n_INTR[snd_ch] - 1;
	k = 0;

	for (i = 0; i <= buf_size1; i++)
	{
		for (j = 0; j <= n_intr1; j++)
		{
			buf[k] = src_buf[i];
			k++;
	   }
	}
	FIR_filter(buf, buf_size *n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], dest_buf, DET[emph][rcvr_nr].prev_INTR_buf[snd_ch]);
}

void interpolation_PSK(int snd_ch, int rcvr_nr, int emph, float * destI, float * destQ, float * srcI, float * srcQ, int buf_size)
{
	word n_intr1, buf_size1, k, i, j;
	single bufI[8192], bufQ[8192];

	buf_size1 = buf_size - 1;
	n_intr1 = n_INTR[snd_ch] - 1;

	k = 0;

	for (i = 0; i <= buf_size1; i++)
	{
		for (j = 0; j <= n_intr1; j++)
		{
			bufI[k] = srcI[i];
			bufQ[k] = srcQ[i];
			k++;
		}
	}

	FIR_filter(bufI, buf_size*n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], destI, DET[emph][rcvr_nr].prev_INTRI_buf[snd_ch]);
	FIR_filter(bufQ, buf_size*n_INTR[snd_ch], INTR_tap[snd_ch], INTR_core[snd_ch], destQ, DET[emph][rcvr_nr].prev_INTRQ_buf[snd_ch]);
}


void FSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	// filtered samples in src_BPF_buf, output in src_Loop_buf

	Mux3(snd_ch,rcvr_nr,emph, &DET[0][rcvr_nr].src_BPF_buf[snd_ch][0], &LPF_core[snd_ch][0], &DET[emph][rcvr_nr].src_Loop_buf[snd_ch][0],
		&DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch][0], &DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch][0], LPF_tap[snd_ch], rx_bufsize);
  
  if (n_INTR[snd_ch] > 1)
  {
	  interpolation(snd_ch, rcvr_nr, emph, DET[emph][rcvr_nr].src_INTR_buf[snd_ch], DET[emph][rcvr_nr].src_Loop_buf[snd_ch], rx_bufsize);
	  decode_stream_FSK(last, snd_ch, rcvr_nr, emph, &DET[emph][rcvr_nr].src_INTR_buf[snd_ch][0], &DET[emph][rcvr_nr].bit_buf[snd_ch][0], rx_bufsize*n_INTR[snd_ch], &DET[emph][rcvr_nr].rx_data[snd_ch]);
  }
  else 
	  decode_stream_FSK(last,snd_ch,rcvr_nr,emph,DET[emph][rcvr_nr].src_Loop_buf[snd_ch], &DET[emph][rcvr_nr].bit_buf[snd_ch][0], rx_bufsize, &DET[emph][rcvr_nr].rx_data[snd_ch]);
}

void  BPSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	Mux3_PSK(snd_ch, rcvr_nr, emph,
		DET[0][rcvr_nr].src_BPF_buf[snd_ch],
		LPF_core[snd_ch],
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch],
		LPF_tap[snd_ch], rx_bufsize);

	if (n_INTR[snd_ch] > 1)
	{
		interpolation_PSK(snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch], rx_bufsize);

		decode_stream_BPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize*n_INTR[snd_ch],
			&DET[emph][rcvr_nr].rx_data[snd_ch]);

	}
	else
		decode_stream_BPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize,
			&DET[emph][rcvr_nr].rx_data[snd_ch]);
}

void  QPSK_Demodulator(int snd_ch, int rcvr_nr, int emph, int last)
{
	Mux3_PSK(snd_ch, rcvr_nr, emph,
		DET[0][rcvr_nr].src_BPF_buf[snd_ch],
		LPF_core[snd_ch],
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch],
		LPF_tap[snd_ch], rx_bufsize);

	if (n_INTR[snd_ch] > 1)
	{
		interpolation_PSK(snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch], rx_bufsize);

		decode_stream_QPSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize*n_INTR[snd_ch],
			&DET[emph][rcvr_nr].rx_data[snd_ch]);

	}
	else decode_stream_QPSK(last, snd_ch, rcvr_nr, emph, 
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].bit_buf[snd_ch],
		rx_bufsize,
		&DET[emph][rcvr_nr].rx_data[snd_ch]);

}




void PSK8_Demodulator(int snd_ch, int rcvr_nr, int emph, boolean last)
{
	Mux3_PSK(snd_ch, rcvr_nr, emph,
		DET[0][rcvr_nr].src_BPF_buf[snd_ch],
		LPF_core[snd_ch],
		DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1I_buf[snd_ch],
		DET[emph][rcvr_nr].prev_LPF1Q_buf[snd_ch],
		LPF_tap[snd_ch], rx_bufsize);

	if (n_INTR[snd_ch] > 1)
	{
		interpolation_PSK(snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch], rx_bufsize);

		decode_stream_8PSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_INTRI_buf[snd_ch],
			DET[emph][rcvr_nr].src_INTRQ_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize*n_INTR[snd_ch],
			&DET[emph][rcvr_nr].rx_data[snd_ch]);

	}
	else
		decode_stream_8PSK(last, snd_ch, rcvr_nr, emph,
			DET[emph][rcvr_nr].src_LPF1I_buf[snd_ch],
			DET[emph][rcvr_nr].src_LPF1Q_buf[snd_ch],
			DET[emph][rcvr_nr].bit_buf[snd_ch],
			rx_bufsize,
			&DET[emph][rcvr_nr].rx_data[snd_ch]);
}


void Demodulator(int snd_ch, int rcvr_nr, float * src_buf, int last, int xcenter)
{
	// called once per decoder (current one in rcvr_nr)

	int i, k;
	string rec_code;
	UCHAR emph;
	int found;
	string * s_emph;
	struct TDetector_t * pDET = &DET[0][rcvr_nr];

	// looks like this filters to src_BPF_buf

	FIR_filter(src_buf, rx_bufsize, BPF_tap[snd_ch], pDET->BPF_core[snd_ch], pDET->src_BPF_buf[snd_ch], pDET->prev_BPF_buf[snd_ch]);

	// AFSK demodulator

	if (modem_mode[snd_ch] == MODE_FSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				FSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			FSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			FSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);
	}

	// BPSK demodulator
	if (modem_mode[snd_ch] == MODE_BPSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				BPSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			BPSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			BPSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);

	}

	// QPSK demodulator
	if (modem_mode[snd_ch] == MODE_QPSK || modem_mode[snd_ch] == MODE_PI4QPSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				QPSK_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			QPSK_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
		else
			QPSK_Demodulator(snd_ch, rcvr_nr, emph_db[snd_ch], last);
	}

	// 8PSK demodulator

	if (modem_mode[snd_ch]==MODE_8PSK)
	{
		if (emph_all[snd_ch])
		{
			for (emph = 1; emph <= nr_emph; emph++)
				PSK8_Demodulator(snd_ch, rcvr_nr, emph, FALSE);

			PSK8_Demodulator(snd_ch, rcvr_nr, 0, last);
		}
	  else
			PSK8_Demodulator(snd_ch,rcvr_nr,emph_db[snd_ch],last);
	}
	
	// MPSK demodulator

	if (modem_mode[snd_ch] == MODE_MPSK)
	{
		decode_stream_MPSK(snd_ch, rcvr_nr, DET[0][rcvr_nr].src_BPF_buf[snd_ch], rx_bufsize, last);
	}


// I think this handles multiple decoders and passes packet on to next level

// Packet manager
	if (last)
		ProcessRXFrames(snd_ch);
}

void ProcessRXFrames(int snd_ch)
{
	boolean fecflag = 0;
	char indicators[5] = "-$#F+"; // None, Single, MEM, FEC, Normal

	// Work out which decoder and which emph settings worked. 

	if (snd_ch < 0 || snd_ch >3)
		return;

	if (detect_list[snd_ch].Count > 0)		// no point if nothing decoded
	{
		char decoded[32] = "";
		char indicators[5] = "-$#F+"; // None, Single, MEM, FEC, Normal
		char s_emph[4] = "";
		int emph[4] = { 0 };
		char report[32] = "";
		int il2perrors = 255;

		// The is one DET for each Decoder for each Emph setting

		struct TDetector_t * pDET;
		int i = 0, j, found;
		int maxemph = nr_emph;

		for (i = 0; i <= nr_emph; i++)
		{
			for (j = 0; j <= RCVR[snd_ch] * 2; j++)
			{
				pDET = &DET[i][j];

				if (pDET->rx_decoded > decoded[j])		// Better than other one (| is higher than F)
					decoded[j] = pDET->rx_decoded;

				if (pDET->emph_decoded > emph[i])
					emph[i] = pDET->emph_decoded;

				if (il2perrors > pDET->errors)
					il2perrors = pDET->errors;

				pDET->rx_decoded = 0;
				pDET->emph_decoded = 0;					// Ready for next time
				pDET->errors = 255;
			}
			if (emph_all[snd_ch] == 0)
				break;
		}

		decoded[j] = 0;

		for (j--; j >= 0; j--)
			decoded[j] = indicators[decoded[j]];

		if (emph_all[snd_ch])
		{
			for (i = 0; i <= nr_emph; i++)
			{
				s_emph[i] = indicators[emph[i]];
			}
			sprintf(report, "%s][%s", s_emph, decoded);
		}

		else
			strcpy(report, decoded);

		if (detect_list_c[snd_ch].Items[0]->Length)
		{
			if (il2perrors < 255 && il2perrors > 0)
				sprintf(detect_list_c[snd_ch].Items[0]->Data, "%s-%d", detect_list_c[snd_ch].Items[0]->Data, il2perrors);

			strcat(report, "][");
			strcat(report, detect_list_c[snd_ch].Items[0]->Data);
		}

		if (detect_list[snd_ch].Count > 0)
		{
			for (i = 0; i < detect_list[snd_ch].Count; i++)
			{
				found = 0;

				//					if (detect_list_l[snd_ch].Count > 0)
				//						if (my_indexof(&detect_list_l[snd_ch], detect_list[snd_ch].Items[i]) > -1)
				//							found = 1;

				if (found == 0)
				{
					if (modem_mode[snd_ch] == MODE_MPSK)
					{
						//					analiz_frame(snd_ch, detect_list[snd_ch].Items[i]->Data, [snd_ch].Items[i]->Data + ' dF: ' + FloatToStrF(DET[0, 0].AFC_dF[snd_ch], ffFixed, 0, 1));
					}
					else
					{
						analiz_frame(snd_ch, detect_list[snd_ch].Items[i], report, fecflag);
					}
				}
			}

			// Cancel FX25 decode

			if (fx25_mode[snd_ch] != FX25_MODE_NONE)
			{
				int e;

				for (i = 0; i < 16; i++)
					for (e = 0; e <= nr_emph; e++)
						DET[e][i].fx25[snd_ch].status = FX25_TAG;
			}
		}

		//			Assign(&detect_list_l[snd_ch], &detect_list[snd_ch]);	// Duplicate detect_list to detect_list_l

		Clear(&detect_list[snd_ch]);
		Clear(&detect_list_c[snd_ch]);
	}
	chk_dcd1(snd_ch, rx_bufsize);
}

string * memory_ARQ(TStringList * buf, string * data)
{
	unsigned char crc[32];
	string * s;
	string * frame;
	word k, len, i;

	Byte zeros, ones;
	TStringList need_frames;

	s = data;

	CreateStringList(&need_frames);
	len = data->Length;

	memcpy(crc, &data->Data[data->Length - 18], 18);

	if (buf->Count > 0)
	{
		for (i = 0; i < buf->Count; i++)
		{
			if (buf->Items[i]->Length == len)
				if (memcmp(&buf->Items[i]->Data[len - 18], crc, 18) == 0)
					Add(&need_frames, buf->Items[i]);
		}
	}

	if (need_frames.Count > 2)
	{
		for (i = 0; i < len - 18; i++)
		{
			zeros = 0;
			ones = 0;

			for (k = 0; k < need_frames.Count; k++)
			{
				frame = need_frames.Items[k];
				if (frame->Data[i] == '1') ones++;  else zeros++;
			}
			if (ones > zeros) s->Data[i] = '1'; else s->Data[i] = '0';
		}
	}

//	Clear(&need_frames);
	return s;
}

	
