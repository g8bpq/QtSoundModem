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

extern int RSID_SABM[4];
extern int RSID_UI[4];
extern int RSID_SetModem[4];
extern int needRSID[4];
extern int needRSID[4];


/*

unit ax25_l2;

interface

uses sysutils,classes;

  procedure frame_optimize(snd_ch,port: byte; var buf: TStringList);
  procedure analiz_frame(snd_ch: byte; frame,code: string);
  procedure send_data_buf(snd_ch,port: byte; path: string; nr: byte);
  procedure add_pkt_buf(snd_ch,port: byte; data: string);
  procedure set_link(snd_ch,port: byte; path: string);
  procedure set_unlink(socket: integer; snd_ch,port: byte; path: string);
  procedure set_chk_link(snd_ch,port: byte; path: string);
  procedure UpdateActiveConnects(snd_ch: byte);
  procedure timer_event;
  procedure rst_values(snd_ch,port: byte);
  procedure rst_timer(snd_ch,port: byte);
  function get_free_port(snd_ch: byte; var port: byte): boolean;
  function get_user_port_by_calls(snd_ch: byte; var port: byte; CallFrom,CallTo: string): boolean;

implementation

uses ax25,ax25_agw,sm_main,kiss_mode;
*/

string * make_frame(string * data, Byte * path, Byte  pid, Byte nr, Byte ns, Byte f_type, Byte f_id, boolean rpt, boolean pf, boolean cr);
void rst_t3(TAX25Port * AX25Sess);

TAX25Port * get_user_port(int snd_ch, Byte * path);

void  inc_frack(TAX25Port * AX25Sess)
{
	AX25Sess->clk_frack++;
}


void  rst_frack(TAX25Port * AX25Sess)
{
	AX25Sess->clk_frack = 0;
}

void inc_t1(TAX25Port * AX25Sess)
{
	AX25Sess->t1++;
}

void rst_t1(TAX25Port * AX25Sess)
{
	AX25Sess->t1 = 0;
}

void inc_t3(TAX25Port * AX25Sess)
{
	AX25Sess->t3++;
}

void rst_t3(TAX25Port * AX25Sess)
{
	AX25Sess->t3 = 0;
}


void  rst_values(TAX25Port * AX25Sess)
{
	AX25Sess->IPOLL_cnt = 0;
	AX25Sess->hi_vs = 0;
	AX25Sess->vs = 0;
	AX25Sess->vr = 0;
	Clear(&AX25Sess->I_frame_buf);
	Clear(&AX25Sess->in_data_buf);
	Clear(&AX25Sess->frm_collector);

	ax25_info_init(AX25Sess);
	clr_frm_win(AX25Sess);
}


void rst_timer(TAX25Port * AX25Sess)
{
	rst_frack(AX25Sess);
	rst_t1(AX25Sess);
	rst_t3(AX25Sess);
}

void upd_i_lo(TAX25Port * AX25Sess, int n) //Update the counter of the first frame in the I-frame buffer
{
	AX25Sess->i_lo = n;
}

void upd_i_hi(TAX25Port * AX25Sess, int n) //Update last frame counter in I-frame buffer
{
	AX25Sess->i_hi = n;
}

void upd_vs(TAX25Port * AX25Sess, int n) //Update the counter of the next frame to transmit
{
	AX25Sess->vs = ++n & 7;
}

void upd_vr(TAX25Port * AX25Sess, int n) //Refresh the counter of the next frame at the reception
{
	AX25Sess->vr = ++n & 7;
}


void Frame_Optimize(TAX25Port * AX25Sess, TStringList * buf)
{
	// I think this removes redundant frames from the TX Queue (eg repeated RR frames)

	string * frame;
	Byte path[80];
	string * data = newString();

	Byte  pid, nr, ns, f_type, f_id, rpt, cr, pf;
	boolean  curr_req, optimize;
	int i, k;
	char need_frm[8] = "";
	int index = 0;
	boolean  PollRR;
	boolean PollREJ;

	PollRR = FALSE;
	PollREJ = FALSE;
	curr_req = FALSE;

	// Check Poll RR and REJ frame

	i = 0;

	while (i < buf->Count && !PollREJ)
	{
		frame = Strings(buf, i);
		// TX frame has kiss control on front

		decode_frame(frame->Data + 1, frame->Length - 1, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

		if (cr == SET_R && pf == SET_P)
		{
			if (f_id == S_REJ)
				PollREJ = TRUE;
			else if (f_id == S_RR && nr == AX25Sess->vr)
				PollRR = TRUE;
		}
		i++;
	}

	// Performance of the REJ Cards: Optional Rej Cards

	i = 0;

	while (i < buf->Count)
	{
		optimize = TRUE;
		frame = Strings(buf, i);
		decode_frame(frame->Data + 1, frame->Length - 1, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

		if (f_id == S_REJ && cr == SET_R)
		{
			if ((pf == SET_F && PollREJ) || nr != AX25Sess->vr)
			{
				Debugprintf("Optimizer dropping REJ nr %d vr %d pf %d PollREJ %d", nr, AX25Sess->vr, pf, PollREJ);
				Delete(buf, i);
				optimize = FALSE;
			}
			if (nr == AX25Sess->vr)
				curr_req = TRUE;
		}
		if (optimize)
			i++;
	}

	// Performance Options

	i = 0;

	while (i <buf->Count)
	{
		need_frm[0] = 0;
		index = 0;
		k = AX25Sess->i_lo;

		while (k != AX25Sess->vs)
		{
			need_frm[index++] = k + 'A';
			k = (++k) & 7;
		}

		optimize = TRUE;

		frame = Strings(buf, i);

		decode_frame(frame->Data +1 , frame->Length - 1, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

		if (f_id == S_RR)
		{
			// RR Cards Methods: Optional RR, F Cards
			if (cr == SET_R)
			{
				if (nr != AX25Sess->vr || ((pf == SET_F) && PollRR) || curr_req)
				{
					Debugprintf("Optimizer dropping RR nr %d vr %d pf %d PollRR %d", nr, AX25Sess->vr, pf, PollRR);

					Delete(buf, i);
					optimize = FALSE;
				}
			}


			// RR Cards Methods : Optional RR, P Cards
			if (cr == SET_C)
			{
				if (AX25Sess->status == STAT_LINK || AX25Sess->status == STAT_WAIT_ANS)
				{
					Debugprintf("Optimizer dropping RR nr %d vr %d pf %d PollRR %d", nr, AX25Sess->vr, pf, PollRR);
					Delete(buf, i);
					optimize = FALSE;
				}
			}
		}
		// 	  I - Cards Methods : Options for I - Cards
		else if (f_id == I_I)
		{
			if (strchr(need_frm, ns + 'A') == 0)
			{
				Delete(buf, i);
				optimize = FALSE;
			}
			else
			{
				if (nr != AX25Sess->vr)
					buf->Items[i] = make_frame(data, path, pid, AX25Sess->vr, ns, f_type, f_id, rpt, pf, cr);
			}
		}

		// SABM Applications
		
		if (f_id == U_SABM)
		{
			if (AX25Sess->status != STAT_TRY_LINK)
			{
				Delete(buf, i);
				optimize = FALSE;
			}
		}
				
		if (optimize)
			i++;
	}
}

void  add_pkt_buf(TAX25Port * AX25Sess, string * data)
{
	boolean found = 0;
	string * frm;
	int i = 0;

	while (i < AX25Sess->frame_buf.Count && !found)
	{
		found = compareStrings(Strings(&AX25Sess->frame_buf, i++), data);
	}
	
	if (found)
		freeString(data);
	else
		Add(&AX25Sess->frame_buf, data);
}

void add_I_FRM(TAX25Port * AX25Sess, Byte * path)
{
	string * data;
	int  i;

	upd_i_lo(AX25Sess, AX25Sess->vs);

	while (AX25Sess->in_data_buf.Count > 0 && AX25Sess->I_frame_buf.Count != maxframe[AX25Sess->snd_ch])
	{
		data = duplicateString(Strings(&AX25Sess->in_data_buf, 0));
		Delete(&AX25Sess->in_data_buf, 0);
		Add(&AX25Sess->I_frame_buf, data);
	}
	if (AX25Sess->I_frame_buf.Count > 0)
	{
		for (i = 0; i < AX25Sess->I_frame_buf.Count; i++)
		{
			upd_i_hi(AX25Sess, AX25Sess->vs);
			upd_vs(AX25Sess, AX25Sess->vs);
			AX25Sess->hi_vs = AX25Sess->vs; // Last transmitted frame
		}
	}
}


void  delete_I_FRM(TAX25Port * AX25Sess, int  nr)
{
	int i;

	i = AX25Sess->i_lo;

	while (i != nr)
	{
		if (AX25Sess->I_frame_buf.Count > 0)
		{
			AX25Sess->info.stat_s_pkt++;
			AX25Sess->info.stat_s_byte += Strings(&AX25Sess->I_frame_buf, 0)->Length;
			Delete(&AX25Sess->I_frame_buf, 0);
		}

		i++;
		i &= 7;
	}
	upd_i_lo(AX25Sess, nr);
}

void delete_I_FRM_port(TAX25Port * AX25Sess)
{
	string * frame;
	string path = { 0 }; 
	string data= { 0 };

	Byte pid, nr, ns, f_type, f_id, rpt, cr, pf;
	boolean  optimize;
	int  i = 0;

	while (i < AX25Sess->frame_buf.Count)
	{
		optimize = TRUE;
		frame = Strings(&AX25Sess->frame_buf, i);

		decode_frame(frame->Data, frame->Length, &path, &data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

		if (f_id == I_I)
		{
			Delete(&AX25Sess->frame_buf, i);
			optimize = FALSE;
		}
		if (optimize)
			i++;
	}
}
 
void send_data_buf(TAX25Port * AX25Sess, int  nr)
{
	int i;
	boolean new_frames;
	boolean PF_bit;

	if (AX25Sess->status != STAT_LINK)
		return;

	AX25Sess->IPOLL_cnt = 0;
	AX25Sess->vs = nr;
	delete_I_FRM(AX25Sess, nr);		// ?? free acked frames
//	delete_I_FRM_port(AX25Sess);

	if (TXFrmMode[AX25Sess->snd_ch] == 1)
	{
		new_frames = FALSE;
	
		if (AX25Sess->I_frame_buf.Count < 2)
		{
			add_I_FRM(AX25Sess, AX25Sess->Path);
			AX25Sess->status = STAT_LINK;
			new_frames = TRUE;
		}

		if (AX25Sess->I_frame_buf.Count > 0)
		{
			if (new_frames)
			{
				for (i = 0; i < AX25Sess->I_frame_buf.Count; i++)
				{
					if (i == AX25Sess->I_frame_buf.Count - 1)
						PF_bit = SET_P;
					else
						PF_bit = SET_F;

					add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, i), AX25Sess->Path, AX25Sess->PID, AX25Sess->vr, ((AX25Sess->i_lo + i) & 7), I_FRM, I_I, FALSE, PF_bit, SET_C));
				}
			}
			if (!new_frames)
			{
				add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, 0), AX25Sess->Path, AX25Sess->PID, AX25Sess->vr, AX25Sess->i_lo, I_FRM, I_I, FALSE, SET_P, SET_C)); //SET_P
				upd_vs(AX25Sess, AX25Sess->vs);
			}
			AX25Sess->status = STAT_WAIT_ANS;
			rst_timer(AX25Sess);
		}
	}

	if (TXFrmMode[AX25Sess->snd_ch] == 0)
	{
		add_I_FRM(AX25Sess, AX25Sess->Path);
		AX25Sess->status = STAT_LINK;
		
		if (AX25Sess->I_frame_buf.Count > 0)
		{
			for (i = 0; i < AX25Sess->I_frame_buf.Count; i++)
			{
				if (i == AX25Sess->I_frame_buf.Count - 1)
					PF_bit = SET_P;
				else
					PF_bit = SET_F;
				add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, i), AX25Sess->Path, AX25Sess->PID, AX25Sess->vr, ((AX25Sess->i_lo + i) & 7), I_FRM, I_I, FALSE, PF_bit, SET_C));
			}
			AX25Sess->status = STAT_WAIT_ANS;
			rst_timer(AX25Sess);
		}
	}
}


void send_data_buf_srej(TAX25Port * AX25Sess, int  nr)
{
	// Similar to send_data_buf but only sends the requested I frame with P set

	int i = 0;
	boolean new_frames;
	boolean PF_bit;

	if (AX25Sess->status != STAT_LINK)
		return;

	AX25Sess->IPOLL_cnt = 0;
	AX25Sess->vs = nr;
	delete_I_FRM(AX25Sess, nr);		// ?? free acked frames

	new_frames = FALSE;

	add_I_FRM(AX25Sess, AX25Sess->Path);
	AX25Sess->status = STAT_LINK;
	new_frames = TRUE;

	if (AX25Sess->I_frame_buf.Count > 0)
	{
		if (new_frames)
		{
			PF_bit = SET_P;

			add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, i), AX25Sess->Path, AX25Sess->PID, AX25Sess->vr, ((AX25Sess->i_lo + i) & 7), I_FRM, I_I, FALSE, PF_bit, SET_C));
		}
		else
		{

			add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, 0), AX25Sess->Path, AX25Sess->PID, AX25Sess->vr, AX25Sess->i_lo, I_FRM, I_I, FALSE, SET_P, SET_C)); //SET_P
			upd_vs(AX25Sess, AX25Sess->vs);
		}
	}
	AX25Sess->status = STAT_WAIT_ANS;
	rst_timer(AX25Sess);
}

void  write_frame_collector(TAX25Port * AX25Sess, int ns, string * data)
{
	Byte  i;
	char  frm_ns;
	string  * frm;
	boolean  found ;
	boolean need_frm;

	if (max_frame_collector[AX25Sess->snd_ch] < 1)
		return;

	need_frm = FALSE;
	i = 1;
	do
	{
		if (ns == (AX25Sess->vr + i) & 7)
			need_frm = TRUE;

		i++;

	} while (i <= max_frame_collector[AX25Sess->snd_ch] && !need_frm);

	if (need_frm)
	{
		frm_ns = ns;
		found = FALSE;
		i = 0;

		if (AX25Sess->frm_collector.Count > 0)
		{
			do
			{
				frm = Strings(&AX25Sess->frm_collector, i);

				if (frm_ns == frm->Data[0])
					found = TRUE;

				i++;
			}
			while (!found &&  i != AX25Sess->frm_collector.Count);

		}

		if (!found)
		{
			string * frm = newString();

			stringAdd(frm, (char *)&frm_ns, 1);
			stringAdd(frm, data->Data, data->Length);
			Add(&AX25Sess->frm_collector, frm);
		}
	}
}

string * read_frame_collector(TAX25Port * AX25Sess, boolean fecflag)
{
	// Look for required frame no in saved frames

	string * frm;
	string * data = newString();

	int i = 0;
	boolean found = FALSE;
	Byte frm_ns;

	while (i < AX25Sess->frm_collector.Count)
	{
		frm = duplicateString(Strings(&AX25Sess->frm_collector, i));

		frm_ns = frm->Data[0];

		if (frm_ns == AX25Sess->vr)
		{
			Delete(&AX25Sess->frm_collector, i);

			upd_vr(AX25Sess, AX25Sess->vr);

			mydelete(frm, 0, 1);				// Remove vr from front

			stringAdd(data, frm->Data, frm->Length);

			found = TRUE;

			AX25Sess->info.stat_r_pkt++;
			AX25Sess->info.stat_r_fc++;
		
			if (fecflag)
				AX25Sess->info.stat_fec_count++;

			AX25Sess->info.stat_r_byte += frm->Length;
			AX25Sess->frm_win[frm_ns].Length = frm->Length; //Save the frame to the window buffer
			AX25Sess->frm_win[frm_ns].Data = frm->Data; //Save the frame to the window buffer
		}

		i++;
	}
	return data;
}


/////////////////////////// SET-FRAMES //////////////////////////////////

void set_chk_link(TAX25Port * AX25Sess, Byte * path)
{
	boolean  b_IPOLL;
	int   len;

	AX25Sess->status = STAT_CHK_LINK;

	//  if AX25Sess->digi<>'' then path:=path+','+reverse_digi(AX25Sess->digi);

	b_IPOLL = FALSE;

	if (AX25Sess->I_frame_buf.Count > 0 && AX25Sess->IPOLL_cnt < 2)
	{
		len = Strings(&AX25Sess->I_frame_buf, 0)->Length;

		if (len > 0 && len <= IPOLL[AX25Sess->snd_ch])
		{

			b_IPOLL = TRUE;
			AX25Sess->IPOLL_cnt++;
		}
	}
	if (b_IPOLL)
		add_pkt_buf(AX25Sess, make_frame(Strings(&AX25Sess->I_frame_buf, 0), path, AX25Sess->PID, AX25Sess->vr, AX25Sess->i_lo, I_FRM, I_I, FALSE, SET_P, SET_C));
	else
		add_pkt_buf(AX25Sess, make_frame(NULL, path, 0xF0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, SET_P, SET_C));

	inc_frack(AX25Sess);
}



// This seems to start a connection

void set_link(TAX25Port * AX25Sess, UCHAR * axpath)
{
	if (AX25Sess->status != STAT_LINK)
	{
		string nullstring;
		nullstring.Length = 0;

		rst_values(AX25Sess);

		AX25Sess->status = STAT_TRY_LINK;

		//		if (AX25Sess->digi[0] != 0)
		//			path: = path + ',' + reverse_digi(AX25Sess->digi);

		add_pkt_buf(AX25Sess, make_frame(&nullstring, axpath, 0, 0, 0, U_FRM, U_SABM, SET_NO_RPT, SET_P, SET_C));

		inc_frack(AX25Sess);
	}
}

#define    MODE_OUR 0

void set_try_unlink(TAX25Port * AX25Sess, Byte * path)
{
	string nullstring;
	nullstring.Length = 0;
	
	AX25Sess->status = STAT_TRY_UNLINK;
	add_pkt_buf(AX25Sess, make_frame(&nullstring, path, 0, 0, 0, U_FRM, U_DISC, SET_NO_RPT, SET_P, SET_C));
	inc_frack(AX25Sess);
}


void set_unlink(TAX25Port * AX25Sess, Byte * path)
{
	if (AX25Sess->status == STAT_TRY_UNLINK 
		|| AX25Sess->status == STAT_TRY_LINK
		|| AX25Sess->status == STAT_NO_LINK)
	{
		string nullstring;
		nullstring.Length = 0;

		//		if (AX25Sess->digi[0] != 0)
		//			path: = path + ',' + reverse_digi(AX25Sess->digi);

		AGW_AX25_disc(AX25Sess, MODE_OUR);

		if (AX25Sess->status != STAT_TRY_UNLINK)
			add_pkt_buf(AX25Sess, make_frame(&nullstring, path, 0, 0, 0, U_FRM, U_DISC, SET_NO_RPT, SET_P, SET_C));

		AX25Sess->info.stat_end_ses = time(NULL);

		write_ax25_info(AX25Sess);
		rst_values(AX25Sess);

		memset(AX25Sess->corrcall, 0, 10);
		memset(AX25Sess->mycall, 0, 10);
		AX25Sess->digi[0] = 0;
		AX25Sess->status = STAT_NO_LINK;

	}
	else
		set_try_unlink(AX25Sess, AX25Sess->Path);
}

void set_FRMR(int snd_ch, Byte * path, unsigned char frameType)
{
	//We may not have a session when sending FRMR so reverse path and send

	Byte revpath[80];
	string * Data = newString();

	Data->Data[0] = frameType;
	Data->Data[1] = 0;
	Data->Data[2] = 1;			// Invalid CTL Byte
	Data->Length = 3;

	reverse_addr(path, revpath, strlen(path));

	add_pkt_buf(&AX25Port[snd_ch][0], make_frame(Data, revpath, 0, 0, 0, U_FRM, U_FRMR, FALSE, SET_P, SET_R));

	freeString(Data);
}

void set_DM(int snd_ch, Byte * path)
{
	//We may not have a session when sending DM so reverse path and send
	
	Byte revpath[80];

	reverse_addr(path, revpath, strlen(path));

	add_pkt_buf(&AX25Port[snd_ch][0], make_frame(NULL, revpath, 0, 0, 0, U_FRM,U_DM,FALSE,SET_P,SET_R));
}

/////////////////////////// S-FRAMES ////////////////////////////////////


void on_RR(TAX25Port * AX25Sess, Byte * path, int  nr, int  pf, int cr)
{
	char need_frame[16] = "";
	int index = 0;

	int i;

	if (AX25Sess->status == STAT_TRY_LINK)
		return;
	
	if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_UNLINK)
	{
		if (cr == SET_C)
			set_DM(AX25Sess->snd_ch, path);

		return;
	}

	if (cr == SET_R)
	{
		// Determine which frames could get into the user’s frame buffer.
		i = AX25Sess->vs;

		need_frame[index++] = i + '0';

//		Debugprintf("RR Rxed vs = %d hi_vs = %d", AX25Sess->vs, AX25Sess->hi_vs);
		while (i != AX25Sess->hi_vs)
		{
			i = (i++) & 7;
			need_frame[index++] = i + '0';
			if (index > 10)
			{
				Debugprintf("Index corrupt %d need_frame = %s", index, need_frame);
				break;
			}
		}

		//Clear the received frames from the transmission buffer.

		if (AX25Sess->status == STAT_WAIT_ANS)
			delete_I_FRM(AX25Sess, nr);

		// We restore the link if the number is valid

		if (AX25Sess->status == STAT_CHK_LINK || strchr(need_frame, nr + '0') > 0)
		{
			rst_timer(AX25Sess);
			AX25Sess->status = STAT_LINK;
			send_data_buf(AX25Sess, nr);
		}
	}

	if (cr == SET_C)
		add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, pf, SET_R));

	rst_t3(AX25Sess);
}


void on_RNR(TAX25Port * AX25Sess, Byte * path, int  nr, int  pf, int cr)
{
	if (AX25Sess->status == STAT_TRY_LINK)
		return;

	if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_UNLINK)
	{
		if (cr == SET_C)
			set_DM(AX25Sess->snd_ch, path);

		return;
	}

	if (cr == SET_R)
	{
		rst_timer(AX25Sess);

		if (AX25Sess->status == STAT_WAIT_ANS)
			delete_I_FRM(AX25Sess, nr);

		AX25Sess->status = STAT_CHK_LINK;
	}

	if (cr == SET_C)
		add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, SET_P, SET_R));

	rst_t3(AX25Sess);
}


void on_REJ(TAX25Port * AX25Sess, Byte * path, int  nr, int  pf, int cr)
{
	if (AX25Sess->status == STAT_TRY_LINK)
		return;

	if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_UNLINK)
	{
		if (cr == SET_C)
			set_DM(AX25Sess->snd_ch, path);

		return;
	}

	if (cr == SET_R)
	{
		rst_timer(AX25Sess);
		AX25Sess->status = STAT_LINK;

		send_data_buf(AX25Sess, nr);
	}

	if (cr == SET_C)
		add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, SET_P, SET_R));
}


void on_SREJ(TAX25Port * AX25Sess, Byte * path, int  nr, int  pf, int cr)
{
	if (AX25Sess->status == STAT_TRY_LINK)
		return;

	if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_UNLINK)
	{
		if (cr == SET_C)
			set_DM(AX25Sess->snd_ch, path);

		return;
	}

	if (cr == SET_R)
	{
		rst_timer(AX25Sess);
		AX25Sess->status = STAT_LINK;

		send_data_buf_srej(AX25Sess, nr);
	}

	if (cr == SET_C)
		add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, SET_P, SET_R));
}
/////////////////////////// I-FRAMES ////////////////////////////////////

void  on_I(void * socket, TAX25Port * AX25Sess, int PID, Byte * path, string * data, int nr, int ns, int pf, int cr, boolean fecflag)
{
	string * collector_data;
	int i;
	Byte need_frame[16] = "";
	int index = 0;
	{
		if (AX25Sess->status == STAT_TRY_LINK)
			return;

		if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_UNLINK)
		{
			set_DM(0, path);
			return;
		}

		if (busy)
		{
			add_pkt_buf(AX25Sess, make_frame(NULL, path, PID, AX25Sess->vr, 0, S_FRM, S_RNR, FALSE, pf, SET_R));
			return;
		}

		// Determine which frames could get into the user’s frame buffer.

		i = AX25Sess->vs;

		need_frame[index++] = i + '0';

//		Debugprintf("I Rxed vs = %d hi_vs = %d", AX25Sess->vs, AX25Sess->hi_vs);
	
		while (i != AX25Sess->hi_vs)
		{
			i = (i++) & 7;
			need_frame[index++] = i + '0';
			if (index > 10)
			{
				Debugprintf("Index corrupt %d need_frame = %s", index, need_frame);
				break;
			}
		}

		//Clear received frames from the transmission buffer.

		if (AX25Sess->status == STAT_WAIT_ANS)
			delete_I_FRM(AX25Sess, nr);

		//We restore the link if the number is valid

		if (AX25Sess->status == STAT_CHK_LINK || strchr(need_frame, nr + '0') > 0)
		{
			//We restore the link if the number is valid

			rst_timer(AX25Sess);
			AX25Sess->status = STAT_LINK;
			send_data_buf(AX25Sess, nr);
		}

		if (ns == AX25Sess->vr)
		{
			//If the expected frame, send RR, F

			AX25Sess->info.stat_r_pkt++;
			AX25Sess->info.stat_r_byte += data->Length;

			if (fecflag)
				AX25Sess->info.stat_fec_count++;

			upd_vr(AX25Sess, AX25Sess->vr);

			AX25Sess->frm_win[ns].Length = data->Length; //Save the frame to the window buffer
			AX25Sess->frm_win[ns].Data = data->Data; //Save the frame to the window buffer

			collector_data = read_frame_collector(AX25Sess, fecflag);

			AGW_AX25_data_in(socket, AX25Sess->snd_ch, PID, path, stringAdd(data, collector_data->Data, collector_data->Length));

			add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_RR, FALSE, pf, SET_R));
		}
		else
		{
			// If the frame is not expected, send REJ, F

			if ((AX25Sess->frm_win[ns].Length != data->Length) &&
				memcmp(&AX25Sess->frm_win[ns].Data, data->Data, data->Length) != 0)

				write_frame_collector(AX25Sess, ns, data);

			add_pkt_buf(AX25Sess, make_frame(NULL, path, 0, AX25Sess->vr, 0, S_FRM, S_REJ, FALSE, pf, SET_R));
		}
		rst_t3(AX25Sess);
	}
}
/////////////////////////// U-FRAMES ////////////////////////////////////


void  on_SABM(void * socket, TAX25Port * AX25Sess)
{
	if (AX25Sess->status == STAT_TRY_UNLINK)
	{
		AX25Sess->info.stat_end_ses = time(NULL);

		write_ax25_info(AX25Sess);
		rst_values(AX25Sess);

		memset(AX25Sess->corrcall, 0, 10);
		memset(AX25Sess->mycall, 0, 10);
		AX25Sess->digi[0] = 0;

		AGW_AX25_disc(AX25Sess, MODE_OTHER);
		Clear(&AX25Sess->frame_buf);

		AX25Sess->status = STAT_NO_LINK;
	}

	if (AX25Sess->status == STAT_TRY_LINK)
	{
		AGW_AX25_disc(AX25Sess, MODE_OTHER);

		rst_timer(AX25Sess);
		rst_values(AX25Sess);
		Clear(&AX25Sess->frame_buf);
		AX25Sess->status = STAT_NO_LINK;
	}

	if (AX25Sess->status != STAT_NO_LINK)
	{
		if ((strcmp(AX25Sess->kind, "Outgoing") == 0) ||
			AX25Sess->status == STAT_TRY_UNLINK || AX25Sess->info.stat_s_byte > 0 || 
			AX25Sess->info.stat_r_byte > 0 || AX25Sess->frm_collector.Count > 0)
		{
			AX25Sess->info.stat_end_ses = time(NULL);
			AGW_AX25_disc(AX25Sess, MODE_OTHER);
			write_ax25_info(AX25Sess);
			rst_timer(AX25Sess);
			rst_values(AX25Sess);
			Clear(&AX25Sess->frame_buf);
			AX25Sess->status = STAT_NO_LINK;
		}
	}

	if (AX25Sess->status == STAT_NO_LINK)
	{
		AGW_AX25_conn(AX25Sess, AX25Sess->snd_ch, MODE_OTHER);
		AX25Sess->vr = 0;
		AX25Sess->vs = 0;
		AX25Sess->hi_vs = 0;
		Clear(&AX25Sess->frm_collector);
		clr_frm_win(AX25Sess);
		AX25Sess->status = STAT_LINK;
		AX25Sess->info.stat_begin_ses = time(NULL);
		strcpy(AX25Sess->kind, "Incoming");
		AX25Sess->socket = socket;

		if (RSID_SABM[AX25Sess->snd_ch])			// Send RSID before SABM/UA
			needRSID[AX25Sess->snd_ch] = 1;

	}

	add_pkt_buf(AX25Sess, make_frame(NULL, AX25Sess->Path, 0, 0, 0, U_FRM, U_UA, FALSE, SET_P, SET_R));
}

void on_DISC(void * socket, TAX25Port * AX25Sess)
{
	if (AX25Sess->status != STAT_NO_LINK)
	{
		AX25Sess->info.stat_end_ses = time(NULL);
		AGW_AX25_disc(AX25Sess, MODE_OTHER);
		write_ax25_info(AX25Sess);
	}

	if (AX25Sess->status == STAT_NO_LINK || AX25Sess->status == STAT_TRY_LINK)
		set_DM(AX25Sess->snd_ch, AX25Sess->Path);
	else
	{
		add_pkt_buf(AX25Sess, make_frame(NULL, AX25Sess->Path, 0, 0, 0, U_FRM, U_UA, FALSE, SET_P, SET_R));

		if (RSID_SABM[AX25Sess->snd_ch])			// Send RSID before SABM/UA
			needRSID[AX25Sess->snd_ch] = 1;
	}

	rst_values(AX25Sess);
	memset(AX25Sess->corrcall, 0, 10);
	memset(AX25Sess->mycall, 0, 10);
	AX25Sess->digi[0] = 0;
	AX25Sess->status = STAT_NO_LINK;
}

void on_DM(void * socket, TAX25Port * AX25Sess)
{
	if (AX25Sess->status != STAT_NO_LINK)
	{
		AX25Sess->info.stat_end_ses = time(NULL);
		AGW_AX25_disc(AX25Sess, MODE_OTHER);
		write_ax25_info(AX25Sess);
	}

	rst_timer(AX25Sess);
	rst_values(AX25Sess);
	memset(AX25Sess->corrcall, 0, 10);
	memset(AX25Sess->mycall, 0, 10);
	AX25Sess->digi[0] = 0;
	AX25Sess->status = STAT_NO_LINK;
}


void on_UA(void *socket, TAX25Port * AX25Sess)
{
	switch (AX25Sess->status)
	{
	case STAT_TRY_LINK:

		AX25Sess->info.stat_begin_ses = time(NULL);
		AX25Sess->status = STAT_LINK;
		AGW_AX25_conn(AX25Sess, AX25Sess->snd_ch, MODE_OUR);
		break;

	case STAT_TRY_UNLINK:

		AX25Sess->info.stat_end_ses = time(NULL);
		AGW_AX25_disc(AX25Sess, MODE_OUR);
		write_ax25_info(AX25Sess);

		rst_values(AX25Sess);
		AX25Sess->status = STAT_NO_LINK;
		memset(AX25Sess->corrcall, 0, 10);
		memset(AX25Sess->mycall, 0, 10);
		AX25Sess->digi[0] = 0;
		break;
	}

	rst_timer(AX25Sess);
}

void on_UI(TAX25Port * AX25Sess, int pf, int cr)
{
}

void on_FRMR(void * socket, TAX25Port * AX25Sess, Byte * path)
{
	if (AX25Sess->status != STAT_NO_LINK)
	{
		AX25Sess->info.stat_end_ses = time(NULL);

		AGW_AX25_disc(socket, AX25Sess->snd_ch, MODE_OTHER, path);
		write_ax25_info(AX25Sess);
	}

	set_DM(AX25Sess->snd_ch, path);

	rst_values(AX25Sess);
	memset(AX25Sess->corrcall, 0, 10);
	memset(AX25Sess->mycall, 0, 10);
	AX25Sess->digi[0] = 0;
	AX25Sess->status = STAT_NO_LINK;
}



void UpdateActiveConnects(int snd_ch)
{
	int port;

	users[snd_ch] = 0;

	for (port = 0; port < port_num; port++)
		if (AX25Port[snd_ch][port].status != STAT_NO_LINK)
			users[snd_ch]++;
}


void timer_event()
{
	int  snd_ch, port;
	void * socket;
	single  frack;
	Byte  active;
	TAX25Port * AX25Sess;

	TimerEvent = TIMER_EVENT_OFF;

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		//reset the slottime timer
		if (dyn_frack[snd_ch])
		{
			UpdateActiveConnects(snd_ch);
			if (users[snd_ch] > 0)
				active = users[snd_ch] - 1;
			else
				active = 0;

			frack = frack_time[snd_ch] + frack_time[snd_ch] * active * 0.5;
		}
		else
			frack = frack_time[snd_ch];

		//
		for (port = 0; port < port_num; port++)
		{
			AX25Sess = &AX25Port[snd_ch][port];

			if (AX25Sess->status == STAT_NO_LINK)
				continue;

			if (snd_status[snd_ch] != SND_TX)  //when at the reception
				if (AX25Sess->frame_buf.Count == 0)  //when the transfer buffer is empty
					inc_t1(AX25Sess); // we consider time of the timer of repeated requests

			// Wouldn't it make more sense to keep path in ax.25 struct?

			if (AX25Sess->t1 >= frack * 10 + (number_digi(AX25Sess->digi) * frack * 20))
			{
				if (AX25Sess->clk_frack >= fracks[snd_ch])
				{
					// This disconnects after retries expires

					rst_frack(AX25Sess);

					//socket:=get_incoming_socket_by_call(AX25Sess->mycall);

					set_unlink(AX25Sess, AX25Sess->Path);
				}

				switch (AX25Sess->status)
				{
				case STAT_TRY_LINK:

					set_link(AX25Sess, AX25Sess->Path);
					break;

				case STAT_TRY_UNLINK:
					
					set_try_unlink(AX25Sess, AX25Sess->Path);
					break;
				
				case STAT_WAIT_ANS:

					set_chk_link(AX25Sess, AX25Sess->Path);
					break;

				case STAT_CHK_LINK:

					set_chk_link(AX25Sess, AX25Sess->Path);
					break;
				}

				rst_t1(AX25Sess);
			}


			if (AX25Sess->t3 >= idletime[snd_ch] * 10)
			{
				set_chk_link(AX25Sess, AX25Sess->Path);
				rst_t1(AX25Sess);
				rst_t3(AX25Sess);
			}

			if (AX25Sess->status == STAT_LINK)
				inc_t3(AX25Sess);

		}
	}
	// KISS ACKMODE
	//if (snd_status[snd_ch]<>SND_TX) and KISSServ then KISS_send_ack1(snd_ch);
}

TAX25Port * get_free_port(int snd_ch)
{
	int i;
	int need_free_port;

	i = 0;
	
	need_free_port = FALSE;
	
	while (i < port_num)
	{
		if (AX25Port[snd_ch][i].status == STAT_NO_LINK)
			return &AX25Port[snd_ch][i];

		i++;
	} 
	return FALSE;
}



TAX25Port * get_user_port(int snd_ch, Byte * path)
                                                                                                                                                                                                                                                                                                                                                                       {
	TAX25Port * AX25Sess = NULL;

	int i = 0;
	int port = 0;


	while (i < port_num)
	{
		AX25Sess = &AX25Port[snd_ch][i];

		if (AX25Sess->status != STAT_NO_LINK && memcmp(AX25Sess->ReversePath, path, AX25Sess->pathLen) == 0)
			return AX25Sess;

		i++;
	}

	return FALSE;
}

boolean get_user_dupe(int snd_ch, Byte * path)
{
	int i = 0;
	TAX25Port * AX25Sess;

	while (i < port_num)
	{
		AX25Sess = &AX25Port[snd_ch][i];

		if (AX25Sess->status != STAT_NO_LINK && memcmp(AX25Sess->ReversePath, path, AX25Sess->pathLen) == 0)
			return TRUE;

		i++;
	}

	return FALSE;
}

TAX25Port * get_user_port_by_calls(int snd_ch, char *  CallFrom, char *  CallTo)
{
	int  i = 0;
	TAX25Port * AX25Sess = NULL;

	while (i < port_num)
	{
		AX25Sess = &AX25Port[snd_ch][i];

		if (AX25Sess->status != STAT_NO_LINK &&
			strcmp(CallFrom, AX25Sess->mycall) == 0 && strcmp(CallTo, AX25Sess->corrcall) == 0)

			return AX25Sess;

		i++;
	}

	return NULL;
}

void * get_sock_by_port(TAX25Port * AX25Sess)
{
	void * socket = (void *)-1;

	if (AX25Sess)
		socket = AX25Sess->socket;

	return socket;
}

void Digipeater(int snd_ch, string * frame)
{
	boolean addr_end, flag_replaced, digi_stop;
	word crc;
	char call[16];
	Byte * addr = &frame->Data[7];					// Origon
	int len = frame->Length - 2;					// not FCS
	string * frameCopy;

	int n = 8;										// Max digis

	if (list_digi_callsigns[snd_ch].Count == 0)
		return;

	// Look for first unused digi

	while ((addr[6] & 1) == 0 && n--)					// until end of address
	{
		addr += 7;

		if ((addr[6] & 128) == 0)
		{
			// unused digi - is it addressed to us?

			UCHAR CRCString[2];

			memcpy(call, addr, 7);
			call[6] &= 0x7E;				// Mask end of call 

			// See if in digi list

			int i;

			for (i = 0; i < list_digi_callsigns->Count; i++)
			{
				if (memcmp(list_digi_callsigns->Items[i]->Data, call, 7) == 0)
				{
					// for us

					addr[6] |= 128;				// set digi'ed

					// TX Frames need a KISS control on front

					frameCopy = newString();

					frameCopy->Data[0] = 0;
					frameCopy->Length = 1;

					stringAdd(frameCopy, frame->Data, frame->Length - 2);		// Exclude CRC

					addr[6] &= 0x7f;				// clear digi'ed from original;

					// Need to redo crc

					crc = get_fcs(frameCopy->Data, len);

					CRCString[0] = crc & 0xff;
					CRCString[1] = crc >> 8;

					stringAdd(frameCopy, CRCString, 2);

					Add(&all_frame_buf[snd_ch], frameCopy);

					return;
				}
			}
		}
	}
}

void analiz_frame(int snd_ch, string * frame, char * code, boolean fecflag)
{
	int  port, free_port;
	Byte path[80];
	string  *data = newString();
	Byte  pid, nr, ns, f_type, f_id, rpt, cr, pf;
	boolean	need_free_port;
	void * socket = NULL;
	boolean	incoming_conn = 0;
	Byte revpath[80];
	int pathlen;
	Byte * ptr;

	int excluded = 0;
	int len;

	TAX25Port * AX25Sess;

	//	mod_icon_status = mod_rx;

	len = frame->Length;

	if (len < PKT_ERR)
		return;

	decode_frame(frame->Data, frame->Length, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

	if (is_excluded_call(snd_ch, path))
		excluded =TRUE;

	// if is_excluded_frm(snd_ch,f_id,data) then excluded:=TRUE;
	

	if (excluded)
		return;

	if (NonAX25[snd_ch])
	{
		if (AGWServ)
			AGW_Raw_monitor(snd_ch, frame);
		if (KISSServ)
			KISS_on_data_out(snd_ch, frame, 0);
	}
	
	// CRC Collision Check

	if (!is_correct_path(path, pid))
	{
		// Duff path - if Non-AX25 filter active log and discard

		if (NonAX25[snd_ch])
		{
			put_frame(snd_ch, frame, "NON-AX25", FALSE, excluded);
			return;
		}
	}

	put_frame(snd_ch, frame, code, 0, excluded);				// Monitor

	if (!NonAX25[snd_ch])
	{
		if (AGWServ)
			AGW_Raw_monitor(snd_ch, frame);

		if (KISSServ)
			KISS_on_data_out(snd_ch, frame, 0);
	}

	// Digipeat frame

	Digipeater(snd_ch, frame);

	if (!is_last_digi(path))
		return;							// Don't process if still unused digis

	// Clear reapeated bits from digi path

	ptr = &path[13];

	while ((*ptr & 1) == 0)				// end of address
	{
		ptr += 7;
		*(ptr) &= 0x7f;					// Clear digi'd bit
	}

	// search for port of correspondent

	AX25Sess = get_user_port(snd_ch, path);

	// if not an active session, AX25Sess will be NULL

	if (AX25Sess == NULL)
		socket = in_list_incoming_mycall(path);
	else
		socket = get_sock_by_port(AX25Sess);

	// link analysis

	if (AX25Sess == NULL)
	{
		// No Session. If socket is set (so call is in incoming calls list) and SABM set up session

		if (socket == NULL)
			return;						// not for us

		if (f_id != U_SABM)				// Not SABM
		{
			// 			// send DM if P set

			if (cr == SET_C)
			{
				switch (f_id)
				{
				case U_DISC:
				case S_RR:
				case S_REJ:
				case S_RNR:
				case I_I:

					set_DM(snd_ch, path);
					break;

				case U_UI:
					break;

				default:
					set_FRMR(snd_ch, path, f_id);
				}


			}
			return;
		}

		// Must be SABM. See if it would duplicate an existing session (but could it - wouldn't that be found earlier ??

		if (get_user_dupe(snd_ch, path))		// Not SABM or a duplicate call pair
			return;

		AX25Sess = get_free_port(snd_ch);

		if (AX25Sess == NULL)
		{
			// if there are no free ports for connection - beat off

			Byte Rev[80];

			reverse_addr(path, Rev, strlen(path));
			set_DM(snd_ch, Rev);
			return;
		}

		// initialise new session

		AX25Sess->snd_ch = snd_ch;

		AX25Sess->corrcall[ConvFromAX25(&path[7], AX25Sess->corrcall)] = 0;
		AX25Sess->mycall[ConvFromAX25(path, AX25Sess->mycall)] = 0;
		AX25Sess->digi[0] = 0;

		//		rst_timer(snd_ch, free_port);

		strcpy(AX25Sess->kind, "Incoming");
		AX25Sess->socket = socket;

		Debugprintf("incoming call socket = %x", socket);

		// I think we need to reverse the path

		AX25Sess->pathLen = strlen(path);
		strcpy(AX25Sess->ReversePath, path);
		reverse_addr(path, AX25Sess->Path, strlen(path));
	}

	// we process a packet on the necessary port

	memcpy(path, AX25Sess->Path, AX25Sess->pathLen);

	switch (f_id)
	{
	case I_I:

		on_I(socket, AX25Sess, pid, path, data, nr, ns, pf, cr, fecflag);
		break;

	case S_RR:

		on_RR(AX25Sess, path, nr, pf, cr);
		break;

	case S_RNR:

		on_RNR(AX25Sess, path, nr, pf, cr);
		break;

	case S_REJ:

		on_REJ(AX25Sess, path, nr, pf, cr);
		break;

	case S_SREJ:

		on_SREJ(AX25Sess, path, nr, pf, cr);
		break;

	case U_SABM:

		on_SABM(socket, AX25Sess);
		break;

	case U_DISC:

		on_DISC(socket, AX25Sess);
		break;

	case U_UA:

		on_UA(socket, AX25Sess);
		break;

	case U_DM:

		on_DM(socket, AX25Sess);
		break;

	case U_UI:

		on_UI(AX25Sess, pf, cr);
		break;

	case U_FRMR:

		on_FRMR(socket, AX25Sess, path);
		break;
	}
	
	if (AGWServ)
		AGW_AX25_frame_analiz(snd_ch, TRUE, frame);
}


