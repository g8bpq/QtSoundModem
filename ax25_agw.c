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

extern char modes_name[modes_count][21];
extern int RSID_SABM[4];
extern int RSID_UI[4];
extern int RSID_SetModem[4];
extern int needRSID[4];

/*


unit ax25_agw;

interface

uses sysutils,classes;

  void AGW_init;
  void AGW_free;
  void AGW_add_socket(socket integer);
  void AGW_del_socket(socket integer);
  void AGW_send_to_app(socket integer; data string);
  void AGW_AX25_frame_analiz(snd_ch byte; RX boolean; frame string);
  void AGW_frame_analiz(Socket Integer; frame string);
  void AGW_explode_frame(Socket Integer; data string);
  void AGW_AX25_conn(socket integer; snd_ch, mode byte; path string);
  void AGW_AX25_disc(socket integer; snd_ch, mode byte; path string);
  void AGW_AX25_data_in(socket integer; snd_ch,PID byte; path,data string);
  void AGW_Raw_monitor(snd_ch byte; data string);
  void AGW_frame_header(AGWPort,DataKind,PID,CallFrom,CallTo string; Len word) string;
  void AGW_C_Frame(port char; CallFrom,CallTo,Conn_MSG string) string;
  void erase_zero_ssid(call string) string;
  void AGW_get_socket(socket integer) integer;
  void clr_zero(data string) string;

type TAGWUser = record
  socket TStringList;
  AGW_frame_buf TStringList;
  Monitor TStringList;
  Monitor_raw TStringList;
};
*/

void AGW_AX25_frame_analiz(int snd_ch, int RX, string * frame);

void AGW_frame_analiz(AGWUser *  AGW);
void AGW_send_to_app(void * socket, string * data);
string * make_frame(string * data, Byte * path, Byte  pid, Byte nr, Byte ns, Byte f_type, Byte f_id, boolean rpt, boolean pf, boolean cr);
void del_incoming_mycalls_by_sock(void * socket);
void del_incoming_mycalls(char * src_call);
void send_data_buf(TAX25Port * AX25Sess, int  nr);
void get_monitor_path(Byte * path, char * mycall, char * corrcall, char * digi);
void decode_frame(Byte * frame, int len, Byte * path, string * data,
	Byte * pid, Byte * nr, Byte * ns, Byte * f_type, Byte * f_id,
	Byte *  rpt, Byte * pf, Byte * cr);


int AGWVersion[2] = {2019, 'B'};		// QTSM Signature

  //ports_info1='1;Port1 with LoopBack Port;

#define    LSB 29
#define    MSB 30
#define    MON_ON '1'
#define    MON_OFF '0'
#define    MODE_OUR 0
#define    MODE_OTHER 1
#define    MODE_RETRY 2


AGWUser ** AGWUsers = NULL;				// List of currently connected clients
int AGWConCount = 0;


struct AGWHeader
{
	int Port;
	unsigned char DataKind;
	unsigned char filler2;
	unsigned char PID;
	unsigned char filler3;
	char callfrom[10];
	char callto[10];
	int DataLength;
	int reserved;
};

#define AGWHDDRRLEN sizeof(struct AGWHeader)


struct AGWSocketConnectionInfo
{
	int Number;					// Number of record - for AGWConnections display
	void *socket;
//	SOCKADDR_IN sin;
	BOOL SocketActive;
	BOOL RawFlag;
	BOOL MonFlag;
	unsigned char CallSign1[10];
	unsigned char CallSign2[10];
	BOOL GotHeader;
	int MsgDataLength;
	struct AGWHeader AGWRXHeader;
};


AGWUser * AGW_get_socket(void * socket)
{
	int i;
	AGWUser * AGW = NULL;

	if (AGWConCount == 0)
		return NULL;

	for (i = 0; i < AGWConCount; i++)
	{
		if (AGWUsers[i]->socket == socket)
		{
			AGW = AGWUsers[i];
			break;
		}
	}
	return AGW;
}


void AGW_del_socket(void * socket)
{
	AGWUser * AGW = AGW_get_socket(socket);
	TAX25Port * AX25Sess = NULL;

	int i = 0;
	int port = 0;

	// Close any connections

	for (port = 0; port < 4; port++)
	{
		for (i = 0; i < port_num; i++)
		{
			AX25Sess = &AX25Port[port][i];

			if (AX25Sess->status != STAT_NO_LINK && AX25Sess->socket == socket)
			{
				rst_timer(AX25Sess);
				set_unlink(AX25Sess, AX25Sess->Path);
			}
		}
	}

	// Clear registrations

	del_incoming_mycalls_by_sock(socket);

	if (AGW == NULL)
		return;

//	Clear(&AGW->AGW_frame_buf);
	freeString(AGW->data_in);
	AGW->Monitor = 0;
	AGW->Monitor_raw = 0;
	AGW->reportFreqAndModem = 0;
}



void AGW_add_socket(void * socket)
{
	AGWUser * User = (struct AGWUser_t *)malloc(sizeof(struct AGWUser_t));			// One Client
	
	AGWUsers = realloc(AGWUsers, (AGWConCount + 1) * sizeof(void *));
	AGWUsers[AGWConCount++] = User;

	User->data_in = newString();
	User->socket = socket;

	User->Monitor = 0;
	User->Monitor_raw = 0;
	User->reportFreqAndModem = 0;
};




void agw_free()
{
 // AGWUser.AGW_frame_buf.Free;
 // AGWUser.Monitor.Free;
 // AGWUser.Monitor_raw.Free;
 // AGWUser.socket.Free;
};

/*
void erase_zero_ssid(call string) string;
var
  p byte;
{
  p = pos('-0',call);
  if (p>0 ) delete(call,p,2);
  result = call;
};
*/

string * AGW_frame_header(char AGWPort, char DataKind, UCHAR PID, char * CallFrom, char * CallTo, int Len )
{
	string * Msg = newString();

	struct AGWHeader * Hddr = (struct AGWHeader *)Msg->Data;
	memset(Hddr, 0, sizeof(struct AGWHeader));

	Hddr->Port = AGWPort;
	Hddr->DataLength = Len;
	Hddr->DataKind = DataKind;
	Hddr->PID = PID;
	strcpy(Hddr->callfrom, CallFrom);
	strcpy(Hddr->callto, CallTo);

	Msg->Length = sizeof(struct AGWHeader);
	return Msg;
};



// AGW to APP frames

string * AGW_R_Frame()
{
	string * Msg = AGW_frame_header(0, 'R', 0, "", "", 8);
	
	stringAdd(Msg, (UCHAR *)AGWVersion, 8);

	return Msg;
};


string * AGW_X_Frame(char * CallFrom,  UCHAR reg_call)
{
	string * Msg = AGW_frame_header(0, 'X', 0, CallFrom, "", 1);

	stringAdd(Msg, (UCHAR *)&reg_call, 1);

	return Msg;

};

string * AGW_G_Frame()
{
	char Ports[256] = "0;";
	char portMsg[64];

	string * Msg;
	
	for (int i = 0; i < 4; i++)
	{
		if (soundChannel[i])
		{
			Ports[0]++;
			sprintf(portMsg, "Port%c with SoundCard Ch %c;", Ports[0], 'A' + i);
			strcat(Ports, portMsg);
		}
//		else
//			sprintf(portMsg, "Port%c Disabled;", Ports[0]);

;
	}


	Msg = AGW_frame_header(0, 'G', 0, "", "", strlen(Ports) + 1);

	stringAdd(Msg, (UCHAR *)Ports, strlen(Ports) + 1);

	return Msg;
};


string * AGW_Gs_Frame(int port, Byte * port_info, int Len)
{
	string * Msg;

	Msg = AGW_frame_header(port, 'g', 0, "", "", Len);
	stringAdd(Msg, port_info, Len);
	return Msg;
};


/*
void AGW_Ys_Frame(port char; frame_outstanding string) string;
var
  DataLen word;
{
  DataLen = 4;
  result = AGW_frame_header(port,'y','','','',DataLen)+frame_outstanding;
};

/
void AGW_Y_Frame(port char; CallFrom,CallTo,frame_outstanding string) string;
var
  DataLen word;
{
  DataLen = 4;
  result = AGW_frame_header(port,'Y','',CallFrom,CallTo,DataLen)+frame_outstanding;
};

*/


string * AGW_Y_Frame(int port, char * CallFrom, char *CallTo, int frame_outstanding)
{
	string * Msg;
	
	Msg = AGW_frame_header(port, 'Y', 0, CallFrom, CallTo, 4);

	stringAdd(Msg, (UCHAR *)&frame_outstanding, 4);
	return Msg;
}

/*

void AGW_H_Frame(port char; heard string) string;
var
  DataLen word;
{
  DataLen = length(heard);
  result = AGW_frame_header(port,'H','','','',DataLen)+heard;
};
*/


string *  AGW_C_Frame(int port, char * CallFrom, char * CallTo, string * Conn_MSG)
{
	string * Msg;
	int DataLen;

	DataLen = Conn_MSG->Length;

	Msg = AGW_frame_header(port, 'C', 240, CallFrom, CallTo, DataLen);

	stringAdd(Msg, Conn_MSG->Data, Conn_MSG->Length);

	freeString(Conn_MSG);

	return Msg;
}

string * AGW_Ds_Frame(int port, char * CallFrom, char * CallTo, string * Disc_MSG)
{
	string * Msg;
	int DataLen;
	
	DataLen = Disc_MSG->Length;

	Msg = AGW_frame_header(port, 'd', 240, CallFrom, CallTo, DataLen);

	stringAdd(Msg, Disc_MSG->Data, Disc_MSG->Length);

	freeString(Disc_MSG);

	return Msg;
};


string * AGW_D_Frame(int port, int PID, char * CallFrom, char * CallTo, string * Data)
{
	string * Msg;
	int DataLen;

	DataLen = Data->Length;

	Msg = AGW_frame_header(port, 'D', PID, CallFrom, CallTo, DataLen);

	stringAdd(Msg, Data->Data, Data->Length);

	freeString(Data);

	return Msg;
}



string *  AGW_I_Frame(int port, char * CallFrom, char * CallTo, char * Monitor)
{
	string * Msg;
	int DataLen;

	DataLen = strlen(Monitor);
	Msg = AGW_frame_header(port, 'I', 0, CallFrom, CallTo, DataLen);

	stringAdd(Msg, (Byte *)Monitor, DataLen);
	return Msg;
}

string *  AGW_S_Frame(int port, char * CallFrom, char * CallTo, char * Monitor)
{
	string * Msg;
	int DataLen;

	DataLen = strlen(Monitor);
	Msg = AGW_frame_header(port, 'S', 0, CallFrom, CallTo, DataLen);

	stringAdd(Msg, (Byte *)Monitor, DataLen);
	return Msg;
};

string *  AGW_U_Frame(int port, char * CallFrom, char * CallTo, char * Monitor)
{
	string * Msg;
	int DataLen;

	DataLen = strlen(Monitor);
	Msg = AGW_frame_header(port, 'U', 0, CallFrom, CallTo, DataLen);

	stringAdd(Msg, (Byte *)Monitor, DataLen);
	return Msg;
}


string * AGW_T_Frame(int port, char * CallFrom, char * CallTo, char * Data)
{
	string * Msg;
	int DataLen;

	DataLen = strlen(Data);
	Msg = AGW_frame_header(port, 'T', 0, CallFrom, CallTo, DataLen);

	stringAdd(Msg, (Byte *)Data, DataLen);

	return Msg;
}

// Raw Monitor 
string * AGW_K_Frame(int port, int PID, char * CallFrom, char * CallTo, string * Data)
{
	string * Msg;
	int DataLen;

	DataLen = Data->Length;

	Msg = AGW_frame_header(port, 'K', PID, CallFrom, CallTo, DataLen);

	stringAdd(Msg, Data->Data, Data->Length);

	freeString(Data);

	return Msg;
}

// APP to AGW frames

void on_AGW_P_frame(AGWUser * AGW)
{
	UNUSED(AGW);
}

void on_AGW_X_frame(AGWUser * AGW, char * CallFrom)
{
	Byte reg_call;

	if (add_incoming_mycalls(AGW->socket, CallFrom))
		reg_call = 1;
	else
		reg_call = 0;
 
	AGW_send_to_app(AGW->socket, AGW_X_Frame(CallFrom, reg_call));
}


void on_AGW_Xs_frame(char * CallFrom)
{
	del_incoming_mycalls(CallFrom);
};

void on_AGW_G_frame(AGWUser * AGW)
{
  AGW_send_to_app(AGW->socket, AGW_G_Frame());
};


void on_AGW_Ms_frame(AGWUser * AGW)
{
	AGW->Monitor ^= 1;				// Flip State
}


void on_AGW_R_frame(AGWUser * AGW)
{
  AGW_send_to_app(AGW->socket, AGW_R_Frame());
}

int refreshModems = 0;


/*

+ 00 On air baud rate(0 = 1200 / 1 = 2400 / 2 = 4800 / 3 = 9600…)
+ 01 Traffic level(if 0xFF the port is not in autoupdate mode)
+ 02 TX Delay
+ 03 TX Tail
+ 04 Persist
+ 05 SlotTime
+ 06 MaxFrame
+ 07 How Many connections are active on this port
+ 08 LSB Low Word
+ 09 MSB Low Word
+ 10 LSB High Word
+ 11 MSB High Word HowManyBytes(received in the last 2 minutes) as a 32 bits(4 bytes) integer.Updated every two minutes.
*/

void on_AGW_Gs_frame(AGWUser * AGW, struct AGWHeader * Frame, Byte * Data)
{
	// QTSM with a data field is used by QtSM to set/read Modem Params

	Byte info[48] = { 0, 255, 24, 3, 100, 15, 6, 0, 1, 0, 0, 0 }; //QTSM Signature
	int Len = 12;
	int Port = Frame->Port;

	// KD6YAM wants the info to be correct. BPQ used 24, 3, 100 as a signature but could use the Version.
	// For now I'll fill in the rest

	info[2] = txdelay[Port] / 10;
	info[3] = txtail[Port] / 10;
	info[4] = persist[Port];
	info[5] = slottime[Port];
	info[6] = maxframe[Port];
	info[7] = 0;

	int i = 0;
	while (i < port_num)
	{
		if (AX25Port[Port][i].status != STAT_NO_LINK)
			info[7]++;

		i++;
	}

	memcpy(&info[8], &bytes2mins[Port], 4);

	if (Frame->DataLength == 32)
	{
		// BPQ to QTSM private Format. 

		// First 4 Freq, 4 to 24 Modem, rest was spare. Use 27-31 for modem control flags (fx.25 il2p etc)

		int Freq;
		Byte versionBytes[4] = VersionBytes;

		AGW->reportFreqAndModem = 1;			// Can report frequency and Modem

		memcpy(&Freq, Data, 4);

		if (Freq)
		{
			// Set Frequency

			memcpy(&rx_freq[Frame->Port], Data, 2);
			refreshModems = 1;
		}

		if (Data[4])
		{
			// New Modem Name. Need to convert to index unless numeric

			int n;

			if (strlen(&Data[4]) < 3)
			{
				n = atoi(&Data[4]);
				if (n < modes_count)
				{
					speed[Frame->Port] = n;
					refreshModems = 1;
				}
			}
			else
			{
				for (n = 0; n < modes_count; n++)
				{
					if (strcmp(modes_name[n], &Data[4]) == 0)
					{
						// Found it

						speed[Frame->Port] = n;
						refreshModems = 1;
						break;
					}
				}

			}
		}

		if (Data[27] == 2)
		{
			fx25_mode[Frame->Port] = Data[28];
			il2p_mode[Frame->Port] = Data[29];
			il2p_crc[Frame->Port] = Data[30];
		}

		// Return Freq and Modem

		memcpy(&info[12], &rx_freq[Frame->Port], 2);
		memcpy(&info[16], modes_name[speed[Frame->Port]], 20);
		info[37] = speed[Frame->Port];			// Index
		memcpy(&info[38], versionBytes, 4);

		Len = 44;

		if (Data[27])
		{
			// BPQ understands fx25 and il2p fields

			AGW->reportFreqAndModem = 2;			// Can report frequency Modem and flags


			Len = 48;

			info[44] = 1;			// Show includes Modem Flags
			info[45] = fx25_mode[Frame->Port];
			info[46] = il2p_mode[Frame->Port];
			info[47] = il2p_crc[Frame->Port];
		}

		AGW_send_to_app(AGW->socket, AGW_Gs_Frame(Frame->Port, info, Len));
		return;
	}
	AGW_send_to_app(AGW->socket, AGW_Gs_Frame(Frame->Port, info, Len));
};
/*
void on_AGW_H_Frame(socket integer; port char);
{
};

void on_AGW_Ys_Frame(socket integer; port char);
var
  snd_ch,i byte;
  info string;
  frames word;
{
  frames = 0;
  //for i = 0 to port_num-1 do frames = frames+AX25port[i].frame_buf.Count;
  snd_ch = ord(Port)+1;
  for i = 0 to port_num-1 do frames = frames+AX25port[snd_ch][i].in_data_buf.Count+AX25port[snd_ch][i].I_frame_buf.Count;
  info = chr(lo(frames))+chr(hi(frames))+#0#0;
  AGW_send_to_app(socket,AGW_Ys_Frame(port,info));
};

*/

void on_AGW_Y_frame(void * socket, int snd_ch, char * CallFrom, char * CallTo)
{
	int  frames;
	TAX25Port * AX25Sess;

	AX25Sess = get_user_port_by_calls(snd_ch, CallFrom, CallTo);

	if (AX25Sess)
	{
		//frames = AX25port[snd_ch][user_port].in_data_buf.Count;
		frames = AX25Sess->in_data_buf.Count + AX25Sess->I_frame_buf.Count;
;
		AGW_send_to_app(socket, AGW_Y_Frame(snd_ch, CallFrom, CallTo, frames));
	}
}

// UI Transmit

void on_AGW_M_frame(int port, Byte PID, char * CallFrom, char *CallTo, Byte *  Msg, int MsgLen)
{
	Byte path[80];
	char Calls[80];
	string * Data = newString();

	stringAdd(Data, Msg, MsgLen);

	sprintf(Calls, "%s,%s", CallTo, CallFrom);

	get_addr(Calls, path);

	Add(&all_frame_buf[port],
		make_frame(Data, path, PID, 0, 0, U_FRM, U_UI, FALSE, SET_F, SET_C));

}


void on_AGW_C_frame(AGWUser * AGW, struct AGWHeader * Frame)
{
	int snd_ch = Frame->Port;
	char * CallFrom = Frame->callfrom;
	char * CallTo = Frame->callto;

	char path[128];
	Byte axpath[80];

	TAX25Port * AX25Sess;

	// Also used for 'v' - connect via digis

	AX25Sess = get_free_port(snd_ch);

	if (AX25Sess)
	{
		AX25Sess->snd_ch = snd_ch;

		strcpy(AX25Sess->mycall, CallFrom);
		strcpy(AX25Sess->corrcall, CallTo);

		sprintf(path, "%s,%s", CallTo, CallFrom);


		if (Frame->DataLength)
		{
			// Have digis

			char * Digis = (char *)Frame + 36;
			int nDigis = Digis[0];

			Digis++;

			while(nDigis--)
			{
				sprintf(path, "%s,%s", path, Digis);
				Digis += 10;
			}
		}

		AX25Sess->digi[0] = 0;

//		rst_timer(snd_ch, free_port);

		strcpy(AX25Sess->kind, "Outgoing");
		AX25Sess->socket = AGW->socket;

		AX25Sess->pathLen = get_addr(path, axpath);

		if (AX25Sess->pathLen == 0)
			return;						// Invalid Path

		strcpy((char *)AX25Sess->Path, (char *)axpath);
		reverse_addr(axpath, AX25Sess->ReversePath, AX25Sess->pathLen);

		if (RSID_SABM[snd_ch])			// Send RSID before SABM/UA
			needRSID[snd_ch] = 1;

		set_link(AX25Sess, AX25Sess->Path);
	};
};





void on_AGW_D_frame(int snd_ch, char * CallFrom, char * CallTo, Byte * Msg, int Len)
{
	TAX25Port * AX25Sess;

	AX25Sess = get_user_port_by_calls(snd_ch, CallFrom, CallTo);

	if (AX25Sess)
	{
		string * data = newString();

		stringAdd(data, Msg, Len);

		Add(&AX25Sess->in_data_buf, data);

		send_data_buf(AX25Sess, AX25Sess->vs);
	}
}

void on_AGW_Ds_frame(void * socket, int snd_ch, char * CallFrom, char * CallTo)
{
	TAX25Port * AX25Sess;

	AX25Sess = get_user_port_by_calls(snd_ch, CallFrom, CallTo);

	if (AX25Sess)
	{
		rst_timer(AX25Sess);

		set_unlink(AX25Sess, AX25Sess->Path);
	}
	else
	{
		string * Msg = newString();

		Msg->Length = sprintf((char *)Msg->Data, "*** DISCONNECTED From Station %s\r", CallTo);
		Msg->Length++;					// Include the terminating NULL

		//del_outgoing_mycalls(CallTo);

		AGW_send_to_app(socket, AGW_Ds_Frame(snd_ch, CallTo, CallFrom, Msg));
	}
}


/*

void on_AGW_Vs_Frame(socket integer; port char; CallFrom,CallTo,Data string);
var
  snd_ch,num_digi,free_port byte;
  need_free_port boolean;
  digi,call,call1 string;
{
  snd_ch = ord(Port)+1;
  num_digi = 0;
  need_free_port = get_free_port(snd_ch,free_port);
  if (need_free_port )
  {
    digi = '';
    get_call(CallFrom,AX25Port[snd_ch][free_port].mycall);
    get_call(CallTo,AX25Port[snd_ch][free_port].corrcall);
    if (length(data)>0 ) { num_digi = ord(data[1]); delete(data,1,1); };
    if ((num_digi in [1..7]) and (length(data)>=num_digi*10) )
    {
      repeat
        call = clr_zero(copy(data,1,10)); delete(data,1,10);
        if (call<>'' )
        {
          get_call(call,call1);
          if (digi='' ) digi = call1
          else digi = digi+','+call1;
        };
      until data='';
      AX25Port[snd_ch][free_port].digi = reverse_digi(digi);
      rst_timer(snd_ch,free_port);
      AX25Port[snd_ch][free_port].kind = 'Outgoing';
      AX25Port[snd_ch][free_port].socket = socket;
      set_link(snd_ch,free_port,CallTo+','+CallFrom);
    };
  };
};

void on_AGW_V_Frame(socket integer; port char; PID,CallFrom,CallTo,Data string);
var
  call,call1,digi,path string;
  snd_ch byte;
  num_digi byte;
  i byte;
{
  digi = '';
  snd_ch = ord(port)+1;
  num_digi = 0;
  if (length(data)>0 ) { num_digi = ord(data[1]); delete(data,1,1); };
  if ((num_digi in [1..7]) and (length(data)>=num_digi*10) )
  {
    for i = 1 to num_digi do
    {
      call = clr_zero(copy(data,1,10)); delete(data,1,10);
      if (call<>'' )
      {
        get_call(call,call1);
        if (digi='' ) digi = call1
        else digi = digi+','+call1;
      };
    };
  };
  path = CallTo+','+CallFrom+','+digi;
  all_frame_buf[snd_ch].Add(make_frame(Data,path,ord(PID[1]),0,0,U_FRM,U_UI,FALSE,SET_F,SET_C));
};

void on_AGW_Cs_Frame(socket integer; port char; PID,CallFrom,CallTo string);
{
};
*/

void on_AGW_K_frame(struct AGWHeader * Frame)
{
	// KISS frame

	unsigned short CRC;
	UCHAR CRCString[2];
	string * TXMSG;

	UCHAR * Data = (UCHAR * )Frame;
	int Len = Frame->DataLength;

	Data = &Data[AGWHDDRRLEN];

	TXMSG = newString();

	stringAdd(TXMSG, Data, Len);		// include Control

	CRC = get_fcs(&Data[1], Len - 1);

	CRCString[0] = CRC & 0xff;
	CRCString[1] = CRC >> 8;

	stringAdd(TXMSG, CRCString, 2);

	Add(&all_frame_buf[Frame->Port], TXMSG);

	// for now assume only used for sending UI

	if (RSID_UI[Frame->Port])			// Send RSID before UI
		needRSID[Frame->Port] = 1;

}

void on_AGW_Ks_frame(AGWUser * AGW)
{
	AGW->Monitor_raw ^= 1;			// Flip State
}

// Analiz incoming frames

void AGW_explode_frame(void * socket, UCHAR * data, int length)
{
	int AGWHeaderLen = sizeof(struct AGWHeader);
	int i;

	AGWUser *  AGW = NULL;

	if (AGWConCount == 0)
		return;

	for (i = 0; i < AGWConCount; i++)
	{
		if (AGWUsers[i]->socket == socket)
		{
			AGW = AGWUsers[i];
			break;
		}
	}

	if (AGW == NULL)
		return;

	stringAdd(AGW->data_in, data, length);

	while (AGW->data_in->Length >= AGWHeaderLen)		// Make sure have at least header
	{
		struct AGWHeader * Hddr = (struct AGWHeader *)AGW->data_in->Data;

		int AgwLen = Hddr->DataLength + AGWHeaderLen;

		if (AgwLen < AGWHeaderLen)						// Corrupt
		{
			AGW->data_in->Length = 0;
			return;
		}

		if (AGW->data_in->Length >= AgwLen)
		{
			// Have frame as well

			if (AGW->data_in->Data[0] == 0xC0)			// Getting KISS Data on AGW Port
			{
				AGW->data_in->Length = 0;				// Delete data
				return;
			}
		
			AGW_frame_analiz(AGW);
			mydelete(AGW->data_in, 0, AgwLen);
		}
		else
			return;					// Wait for the data
	}




	/*
	idx = AGW_get_socket(socket);
	 if (idx>=0 )
	 {
	   AGWUser.AGW_frame_buf.Strings[idx] = AGWUser.AGW_frame_buf.Strings[idx]+data;
	   str_buf = AGWUser.AGW_frame_buf.Strings[idx];
	   repeat
		 done = FALSE;
		 BufLen = length(str_buf);
		 if (BufLen>=HEADER_SIZE )
		 {
		   DataLen = ord(str_buf[LSB])+ord(str_buf[MSB])*256;
		   FrameLen = HEADER_SIZE+DataLen;
		   if (length(str_buf)>=FrameLen )
		   {
			 frame = copy(str_buf,1,FrameLen);
			 delete(str_buf,1,FrameLen);
			 done = TRUE;
			 AGW_frame_analiz(socket,frame);
		   };
		 };
	   until not done;
	   // Check if (socket is still present and has same index
	   if ((AGWUser.socket.Count>idx) and (AGWUser.socket.Strings[idx]=inttostr(socket)) )
		 AGWUser.AGW_frame_buf.Strings[idx] = str_buf;
	 };
	 */
};

/*
void clr_zero(data string) string;
var
  p,i word;
  s string;
{
  s = '';
  p = pos(#0,data);
  if (p>1 ) data = copy(data,1,p-1);
  if (length(data)>0 ) for i = 1 to length(data) do if (data[i]>#31 ) s = s+data[i];
  result = s;
};

void AGW_parse_frame(frame string; var DataKind,PID,AGWPort char; var Pass,CallFrom,CallTo,DataLen,Data string);
{
  DataKind = frame[5];
  PID = frame[7];
  AGWPort = frame[1];
  Pass = '';
  CallFrom = clr_zero(copy(frame,9,10));
  CallTo = clr_zero(copy(frame,19,10));
  get_call(CallFrom,CallFrom);
  get_call(CallTo,CallTo);
  DataLen = inttostr(ord(frame[LSB])+ord(frame[MSB])*256);
  if (length(frame)>HEADER_SIZE ) Data = copy(frame,37,strtoint(DataLen)) else Data = '';
};

*/

void AGW_send_to_app(void * socket, string * data)
{
	char * Msg = malloc(data->Length);
	memcpy(Msg, data->Data, data->Length);
	// can use KISS proc as it just sends to the supplied socket but need copy of message
	KISSSendtoServer(socket, (Byte *)Msg, data->Length);
	freeString(data);
};


void AGW_AX25_data_in(void  * socket, int snd_ch, int PID, Byte * path, string * data)
{
	int len = 250, sendlen;

	char CallFrom[10];
	char CallTo[10];

	string * pkt;

	CallTo[ConvFromAX25(&path[7], CallTo)] = 0;
	CallFrom[ConvFromAX25(path, CallFrom)] = 0;

	while (data->Length)
	{
		if (data->Length > len)
			sendlen = len;
		else
			sendlen = data->Length;

		pkt = copy(data, 0, sendlen);
		mydelete(data, 0, sendlen);

		AGW_send_to_app(socket, AGW_D_Frame(snd_ch, PID, CallFrom, CallTo, pkt));
	}

}

void AGW_AX25_conn(TAX25Port * AX25Sess, int snd_ch, Byte mode)
{
	string * Msg = newString();

	switch (mode)
	{
	case MODE_OTHER:

		Msg->Length = sprintf((char *)Msg->Data, "*** CONNECTED To Station  %s\r", AX25Sess->corrcall);
		break;

	case MODE_OUR:

		Msg->Length = sprintf((char *)Msg->Data, "*** CONNECTED With Station %s\r", AX25Sess->corrcall);
		break;

	};

	Msg->Length++;					// Include the terminating NULL

	AGW_send_to_app(AX25Sess->socket, AGW_C_Frame(snd_ch, AX25Sess->corrcall, AX25Sess->mycall, Msg));
};



void AGW_AX25_disc(TAX25Port * AX25Sess, Byte mode)
{
	string * Msg = newString();

	switch (mode)
	{

	case MODE_OTHER:
	case MODE_OUR:

		Msg->Length = sprintf((char *)Msg->Data, "*** DISCONNECTED From Station %s\r", AX25Sess->corrcall);
		break;

	case MODE_RETRY:

		Msg->Length = sprintf((char *)Msg->Data, "*** DISCONNECTED RETRYOUT With Station %s\r", AX25Sess->corrcall);
		break;

	};

	Msg->Length++;					// Include the terminating NULL

	//del_outgoing_mycalls(CallTo);

	AGW_send_to_app(AX25Sess->socket, AGW_Ds_Frame(AX25Sess->snd_ch, AX25Sess->corrcall, AX25Sess->mycall, Msg));
};


void AGW_frame_monitor(Byte snd_ch, Byte * path, string * data, Byte pid, Byte nr, Byte ns, Byte f_type, Byte f_id, Byte  rpt, Byte pf, Byte cr, Byte RX)
{
	char mon_frm[512];
	char AGW_path[256];
	string * AGW_data = NULL;

	const char * frm;
	Byte * datap = data->Data;
	Byte _data[512];
	Byte * p_data = _data;
	int _datalen;

	char  agw_port;
	char  CallFrom[10], CallTo[10], Digi[80];

	integer i;
	const char * ctrl;
	int len;

	AGWUser * AGW;

	UNUSED(rpt);

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

	agw_port = snd_ch;

	get_monitor_path(path, CallTo, CallFrom, Digi);

	ctrl = "";
	frm = "";

	switch (f_id)
	{
	case I_I:

		frm = "I";
		if (cr == SET_C && pf == SET_P)
			ctrl = " P";

		break;

	case S_RR:

		frm = "RR";
		if (pf == SET_P)
			ctrl = " P/F";

		break;

	case S_RNR:

		frm = "RNR";
		if (pf == SET_P)
			ctrl = " P/F";

		break;

	case S_REJ:

		frm = "REJ";
		if (pf == SET_P)
			ctrl = " P/F";

		break;


	case S_SREJ:

		frm = "SREJ";
		if (pf == SET_P)
			ctrl = " P/F";

		break;

	case U_SABM:

		frm = "SABM";
		if (cr == SET_C && pf == SET_P)
			ctrl = " P";

		break;

	case U_DISC:

		frm = "DISC";
		if (cr == SET_C && pf == SET_P)
			ctrl = " P";
		break;

	case U_DM:

		frm = "DM";
		if ((cr == SET_R) && (pf == SET_P))
			ctrl = " F ";
		break;

	case U_UA:

		frm = "UA";
		if ((cr == SET_R) && (pf == SET_P))
			ctrl = " F ";

		break;

	case U_FRMR:

		frm = "FRMR";
		if ((cr == SET_R) && (pf == SET_P))
			ctrl = " F ";
		break;

	case U_UI:

		frm = "UI";
		if ((pf == SET_P))
			ctrl = " P/F";
	}
	
	if (Digi[0])
		sprintf(AGW_path, " %d:Fm %s To %s Via %s <%s", snd_ch + 1, CallFrom, CallTo, Digi, frm);
	else
		sprintf(AGW_path, " %d:Fm %s To %s <%s", snd_ch + 1, CallFrom, CallTo, frm);
	

	switch (f_type)
	{
	case I_FRM:

		//mon_frm = AGW_path + ctrl + ' R' + inttostr(nr) + ' S' + inttostr(ns) + ' pid=' + dec2hex(pid) + ' Len=' + inttostr(len) + ' >' + time_now + #13 + _data + #13#13;
		sprintf(mon_frm, "%s%s R%d S%d pid=%X Len=%d >[%s]\r%s\r", AGW_path, ctrl, nr, ns, pid, len, ShortDateTime(), _data);

		break;

	case U_FRM:

		if (f_id == U_UI)
		{
			sprintf(mon_frm, "%s pid=%X Len=%d >[%s]\r%s\r", AGW_path, pid, len, ShortDateTime(), _data); // "= AGW_path + ctrl + '>' + time_now + #13;
		}
		else if (f_id == U_FRMR)
		{
			sprintf(mon_frm, "%s%s>%02x %02x %02x[%s]\r", AGW_path, ctrl, datap[0], datap[1], datap[2], ShortDateTime()); // "= AGW_path + ctrl + '>' + time_now + #13;
		}
		else
			sprintf(mon_frm, "%s%s>[%s]\r", AGW_path, ctrl, ShortDateTime()); // "= AGW_path + ctrl + '>' + time_now + #13;

		break;

	case S_FRM:

		//		mon_frm = AGW_path + ctrl + ' R' + inttostr(nr) + ' >' + time_now + #13;
		sprintf(mon_frm, "%s%s R%d>[%s]\r", AGW_path, ctrl, nr, ShortDateTime()); // "= AGW_path + ctrl + '>' + time_now + #13;

		break;

	}

//	stringAdd(mon_frm, "", 1); //  Add 0 at the end of each frame

	// I think we send to all AGW sockets

	for (i = 0; i < AGWConCount; i++)
	{
		AGW = AGWUsers[i];

		if (AGW->Monitor)
		{
			if (RX)
			{
				switch (f_id)
				{

				case I_I:
					AGW_data = AGW_I_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case S_RR:
				case S_RNR:
				case S_REJ:
				case S_SREJ:

					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_SABM:
					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_DISC:
					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_DM:
					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_UA:
					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_FRMR:
					AGW_data = AGW_S_Frame(agw_port, CallFrom, CallTo, mon_frm);
					break;

				case U_UI:
					AGW_data = AGW_U_Frame(agw_port, CallFrom, CallTo, mon_frm);
				}
				if (AGW_data)
					AGW_send_to_app(AGW->socket, AGW_data);
			}

			else
			{
				AGW_data = AGW_T_Frame(agw_port, CallFrom, CallTo, mon_frm);
				AGW_send_to_app(AGW->socket, AGW_data);
			}
		}
	}
}

void AGW_Report_Modem_Change(int port)
{
	// Send modem change report to all sockets that support it

	int i;
	AGWUser * AGW;

	if (soundChannel[port] == 0)		// Not in use
		return;

	// I think we send to all AGW sockets

	for (i = 0; i < AGWConCount; i++)
	{
		AGW = AGWUsers[i];

		if (AGW->reportFreqAndModem)
		{
			// QTSM 'g' Message with a data field is used by QtSM to set/read Modem Params

			Byte info[48] = { 0, 255, 24, 3, 100, 15, 6, 0, 1, 0, 0, 0 }; //QTSM Signature
			int Len = 44;

			// Return Freq and Modem

			memcpy(&info[12], &rx_freq[port], 2);
			memcpy(&info[16], modes_name[speed[port]], 20);
			info[37] = speed[port];			// Index

			if (AGW->reportFreqAndModem == 2)
			{
				Len = 48;

				info[44] = 1;			// Show includes Modem Flags
				info[45] = fx25_mode[port];
				info[46] = il2p_mode[port];
				info[47] = il2p_crc[port];
			}
			AGW_send_to_app(AGW->socket, AGW_Gs_Frame(port, info, Len));
		}
	}
}


void AGW_Raw_monitor(int snd_ch, string * data)
{
	int i;
	AGWUser * AGW;
	string * pkt;

	// I think we send to all AGW sockets

	for (i = 0; i < AGWConCount; i++)
	{
		AGW = AGWUsers[i];

		if (AGW->Monitor_raw)
		{
			pkt = newString();

			pkt->Data[0] = snd_ch << 4;		// KISS Address
			pkt->Length++;

			stringAdd(pkt, data->Data, data->Length - 2);		// Exclude CRC

			AGW_send_to_app(AGW->socket, AGW_K_Frame(snd_ch, 0, "", "", pkt));
		}
	}
}

void AGW_AX25_frame_analiz(int snd_ch, int RX, string * frame)
{
	//  path,data string;
	Byte pid, nr, ns, f_type, f_id;
	Byte  rpt, cr, pf;
	Byte path[80];
	string * data = newString();

	decode_frame(frame->Data, frame->Length, path, data, &pid, &nr, &ns, &f_type, &f_id, &rpt, &pf, &cr);

	AGW_frame_monitor(snd_ch, path, data, pid, nr, ns, f_type, f_id, rpt, pf, cr, RX);
	
//	if (RX)
//		AGW_Raw_monitor(snd_ch, frame);
};


void AGW_frame_analiz(AGWUser *  AGW)
{
	struct AGWHeader * Frame = (struct AGWHeader *)AGW->data_in->Data;
	Byte * Data = &AGW->data_in->Data[36];

	if (Frame->Port < 0 || Frame->Port > 3)
		return;

	if (soundChannel[Frame->Port] == 0)
		return;

	if (Frame->Port > 3)
		return;

	switch (Frame->DataKind)
	{
	case 'P':
		
		on_AGW_P_frame(AGW);
		return;

	case 'X':

		on_AGW_X_frame(AGW, Frame->callfrom);
		return;
		

	case 'x':
		
		on_AGW_Xs_frame(Frame->callfrom);
		return;

	case 'G':
		
		on_AGW_G_frame(AGW);
		return;
		
	case 'm':
		
		on_AGW_Ms_frame(AGW);
		return;

	case 'R':
		
		on_AGW_R_frame(AGW);
		return;
	
	case 'g':
	
		on_AGW_Gs_frame(AGW, Frame, Data);
		return;
//	'H': on_AGW_H_frame(AGW,Frame->Port);
//	'y': on_AGW_Ys_frame(AGW,Frame->Port);

	case 'Y':
		on_AGW_Y_frame(AGW->socket, Frame->Port, Frame->callfrom, Frame->callto);
		break;

	case 'M':
		
		on_AGW_M_frame(Frame->Port,Frame->PID, Frame->callfrom, Frame->callto, Data, Frame->DataLength);
		break;

	case 'C':
	case 'v':				// Call with digis

		on_AGW_C_frame(AGW, Frame);
		return;

	case 'D':
		
		on_AGW_D_frame(Frame->Port, Frame->callfrom, Frame->callto, Data, Frame->DataLength);
		return;
	
	case 'd':
		on_AGW_Ds_frame(AGW->socket, Frame->Port, Frame->callfrom, Frame->callto);
		return;

//	'V': on_AGW_V_frame(AGW,Frame->Port,PID,CallFrom,CallTo,Data);
//	'c': on_AGW_Cs_frame(sAGWocket,Frame->Port,PID,CallFrom,CallTo);


	case 'K':
		
		on_AGW_K_frame(Frame);
		return;

	case 'k':
		on_AGW_Ks_frame(AGW);
		return;

	default:
		Debugprintf("AGW %c", Frame->DataKind);
	}
}

void doAGW2MinTimer()
{
	for (int n = 0; n < 4; n++)
	{
		if (soundChannel[n] == 0)	// Channel not used
			continue;

		bytes2mins[n] = bytes[n];
		bytes[n] = 0;
	}
}

