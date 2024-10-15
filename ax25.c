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

#ifdef WIN32

__declspec(dllimport) unsigned short __stdcall htons(__in unsigned short hostshort);
__declspec(dllimport) unsigned short __stdcall ntohs(__in unsigned short hostshort);

#else

#include <stdint.h>

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

#define strtok_s strtok_r
#include <stddef.h>
#endif 

void set_DM(int snd_ch, Byte * path);
void  rst_values(TAX25Port * AX25Sess);

void decode_frame(Byte * frame, int len, Byte * path, string * data,
	Byte * pid, Byte * nr, Byte * ns, Byte * f_type, Byte * f_id,
	Byte *  rpt, Byte * pf, Byte * cr);

/*

unit ax25;

interface

uses classes,sysutils,windows;

  procedure get_exclude_list(line: string; var list: TStringList);
  procedure get_exclude_frm(line: string; var list: TStringList);
  procedure get_monitor_path(path: string; var mycall,corrcall,digi: string);
  procedure get_call(src_call: string; var call: string);
  procedure get_call_fm_path(path: string; var callto,callfrom: string);
  procedure set_corrcall(snd_ch,port: byte; path: string);
  procedure set_mycall(snd_ch,port: byte; path: string);
  procedure set_digi(snd_ch,port: byte; path: string);
  procedure decode_frame(frame: string; var path,data: string; var pid,nr,ns,f_type,f_id: byte; var rpt,pf,cr: boolean);
  procedure del_incoming_mycalls(src_call: string);
  procedure del_incoming_mycalls_by_sock(socket: integer);
  procedure ax25_info_init(snd_ch,port: byte);
  procedure write_ax25_info(snd_ch,port: byte);
  procedure clr_frm_win(snd_ch,port: byte);
  procedure ax25_init;
  procedure ax25_free;
  function dec2hex(value: byte): string;
  function get_corrcall(path: string): string;
  function get_mycall(path: string): string;
  function get_digi(path: string): string;
  function is_excluded_call(snd_ch: byte; path: string): boolean;
  function is_excluded_frm(snd_ch,f_id: byte; data: string): boolean;
  function is_last_digi(path: string): boolean;
  function is_digi(snd_ch,port: byte; path: string): boolean;
  function is_corrcall(snd_ch,port: byte; path: string): boolean;
  function is_mycall(snd_ch,port: byte; path: string): boolean;
  function is_correct_path(path: string; pid: byte): boolean;
  function direct_addr(path: string): string;
  function reverse_addr(path: string): string;
  function reverse_digi(path: string): string;
  function number_digi(path: string): byte;
  function info_pid(pid: byte): string;
  function get_fcs(var data: string; len: word): word;
  function set_addr(path: string; rpt,cr: boolean): string;
  function set_ctrl(nr,ns,f_type,f_id: byte; pf: boolean): byte;
  function make_frame(data,path: string; pid,nr,ns,f_type,f_id: byte; rpt,pf,cr: boolean): string;
  function get_incoming_socket_by_call(src_call: string): integer;
  function add_incoming_mycalls(socket: integer; src_call: string): boolean;
  function in_list_incoming_mycall(path: string): boolean;
  function get_UTC_time: string;
  function parse_NETROM(data: string; f_id: byte): string;
  function parse_IP(data: string): string;
  function parse_ARP(data: string): string;
  function add_raw_frames(snd_ch: byte; frame: string; var buf: TStringList): boolean;
  function scrambler(in_buf: string): string;
  function my_indexof(var l: TStringList; s: string): integer;


const
  port_num=32;
  PKT_ERR=17; //Minimum packet size, bytes
  I_MAX=7; //Maximum number of packets
  B_IDX_MAX=256;
  ADDR_MAX_LEN=10;
  FRAME_FLAG=126;
  // Status flags
  STAT_NO_LINK=0;
  STAT_LINK=1;
  STAT_CHK_LINK=2;
  STAT_WAIT_ANS=3;
  STAT_TRY_LINK=4;
  STAT_TRY_UNLINK=5;
  // Сmd,Resp,Poll,Final,Digipeater flags
  SET_P=TRUE;
  SET_F=FALSE;
  SET_C=TRUE;
  SET_R=FALSE;
  SET_NO_RPT=FALSE;
  SET_RPT=TRUE;
  // Frame ID flags
  I_FRM=0;
  S_FRM=1;
  U_FRM=2;
  I_I=0;
  S_RR=1;
  S_RNR=5;
  S_REJ=9;
  U_SABM=47;
  U_DISC=67;
  U_DM=15;
  U_UA=99;
  U_FRMR=135;
  U_UI=3;
  // PID flags
  PID_X25=$01;       // 00000001-CCIT X25 PLP
  PID_SEGMENT=$08;   // 00001000-Segmentation fragment
  PID_TEXNET=$C3;    // 11000011-TEXNET Datagram Protocol
  PID_LQ=$C4;        // 11001000-Link Quality Protocol
  PID_APPLETALK=$CA; // 11001010-Appletalk
  PID_APPLEARP=$CB;  // 11001011-Appletalk ARP
  PID_IP=$CC;        // 11001100-ARPA Internet Protocol
  PID_ARP=$CD;       // 11001101-ARPA Address Resolution Protocol
  PID_NET_ROM=$CF;   // 11001111-NET/ROM
*/

#define ADDR_MAX_LEN 10
#define PID_NO_L3 0xF0;     // 11110000-No Level 3 Protocol


unsigned short CRCTable[256] = {
	  0,  4489,  8978, 12955, 17956, 22445, 25910, 29887,
  35912, 40385, 44890, 48851, 51820, 56293, 59774, 63735,
   4225,   264, 13203,  8730, 22181, 18220, 30135, 25662,
  40137, 36160, 49115, 44626, 56045, 52068, 63999, 59510,
   8450, 12427,   528,  5017, 26406, 30383, 17460, 21949,
  44362, 48323, 36440, 40913, 60270, 64231, 51324, 55797,
  12675,  8202,  4753,   792, 30631, 26158, 21685, 17724,
  48587, 44098, 40665, 36688, 64495, 60006, 55549, 51572,
  16900, 21389, 24854, 28831,  1056,  5545, 10034, 14011,
  52812, 57285, 60766, 64727, 34920, 39393, 43898, 47859,
  21125, 17164, 29079, 24606,  5281,  1320, 14259,  9786,
  57037, 53060, 64991, 60502, 39145, 35168, 48123, 43634,
  25350, 29327, 16404, 20893,  9506, 13483,  1584,  6073,
  61262, 65223, 52316, 56789, 43370, 47331, 35448, 39921,
  29575, 25102, 20629, 16668, 13731,  9258,  5809,  1848,
  65487, 60998, 56541, 52564, 47595, 43106, 39673, 35696,
  33800, 38273, 42778, 46739, 49708, 54181, 57662, 61623,
   2112,  6601, 11090, 15067, 20068, 24557, 28022, 31999,
  38025, 34048, 47003, 42514, 53933, 49956, 61887, 57398,
   6337,  2376, 15315, 10842, 24293, 20332, 32247, 27774,
  42250, 46211, 34328, 38801, 58158, 62119, 49212, 53685,
  10562, 14539,  2640,  7129, 28518, 32495, 19572, 24061,
  46475, 41986, 38553, 34576, 62383, 57894, 53437, 49460,
  14787, 10314,  6865,  2904, 32743, 28270, 23797, 19836,
  50700, 55173, 58654, 62615, 32808, 37281, 41786, 45747,
  19012, 23501, 26966, 30943,  3168,  7657, 12146, 16123,
  54925, 50948, 62879, 58390, 37033, 33056, 46011, 41522,
  23237, 19276, 31191, 26718,  7393,  3432, 16371, 11898,
  59150, 63111, 50204, 54677, 41258, 45219, 33336, 37809,
  27462, 31439, 18516, 23005, 11618, 15595,  3696,  8185,
  63375, 58886, 54429, 50452, 45483, 40994, 37561, 33584,
  31687, 27214, 22741, 18780, 15843, 11370,  7921,  3960 };


unsigned short pkt_raw_min_len = 8;
int stat_r_mem = 0;

extern struct TKISSMode_t  KISS;

TAX25Port AX25Port[4][port_num];

TStringList KISS_acked[4];
TStringList KISS_iacked[4];

// I think this is the queue for a soundcard channel, so can only be
// two of them (L and R)

TStringList all_frame_buf[5];

typedef struct registeredCalls_t
{
	UCHAR myCall[7];		// call in ax.25
	void * socket;

} registeredCalls;


TStringList list_incoming_mycalls;	// list strings containing a registered call


boolean busy = 0;
boolean dcd[5] = { 0 ,0 ,0, 0 };

boolean  tx = 0;

int stdtones = 0;
int fullduplex = 0;

UCHAR diddles = 0;
struct TQPSK_t qpsk_set[4];

word MEMRecovery[5] = { 200,200,200,200 };
int NonAX25[5] = { 0 };

boolean dyn_frack[4] = { FALSE,FALSE,FALSE,FALSE };
Byte recovery[4] = { 0,0,0,0 };
Byte users[4] = { 0,0,0,0 };

short txtail[5] = { 50, 50, 50, 50, 50 };
short txdelay[5] = { 400, 400, 400, 400, 400 };

short modem_def[5] = { 1, 1, 1, 1, 1 };

int emph_db[5] = { 0, 0, 0, 0, 0 };
UCHAR emph_all[5] = { 0, 0, 0, 0, 0 };

boolean  KISS_opt[4] = { FALSE, FALSE, FALSE, FALSE };
int resptime[4] = { 0,0,0,0 };
int slottime[4] = { 0,0,0,0 };
int persist[4] = { 100,100,100,100 };
int fracks[4] = { 0,0,0,0 };
int frack_time[4] = { 0,0,0,0 };
int idletime[4] = { 0,0,0,0 };
int redtime[4] = { 0,0,0,0 };
int IPOLL[4] = { 30,30,30,30 };
int maxframe[4] = { 0,0,0,0 };
int TXFrmMode[4] = { 0,0,0,0 };
int max_frame_collector[4] = { 0,0,0,0 };

int bytes[4] = { 0,0,0,0 };
int bytes2mins[4] = { 0,0,0,0 };

char MyDigiCall[4][512] = { "","","","" };
char exclude_callsigns[4][512] = { "","","","" };
char exclude_APRS_frm[4][512] = { "","","","" };

TStringList  list_exclude_callsigns[4];
TStringList list_exclude_APRS_frm[4];
TStringList list_digi_callsigns[4];

Byte xData[256];
Byte xEncoded[256];
Byte xDecoded[256];

int frame_count = 0;
int single_frame_count = 0;

/*
  mydigi: string;
  //// end of user params
  addr: string[70];
  ctrl: byte;
  pid: byte=PID_NO_L3;
  fcs: word;
  data: string;
  frame: string;

implementation

uses ax25_l2,sm_main;

*/

void scrambler(UCHAR * in_buf, int Len)
{
	integer i;
	word  sreg;
	Byte  a = 0, k;

	sreg = 0x1ff;

	for (i = 0; i < Len; i++)
	{
		for (k = 0; k < 8; k++)
		{

			//	a: = (a shr 1) or (sreg and 1 shl 7);

			a = (a >> 1) | ((sreg & 1) << 7);

			//	sreg: = (sreg shl 4 and $200) xor (sreg shl 8 and $200) or (sreg shr 1);


			sreg = (((sreg << 4) & 0x200) ^ ((sreg << 8) & 0x200)) | (sreg >> 1);
		}
		in_buf[i] = in_buf[i] ^ a;
	}
}


/*
function parse_ARP(data: string): string;

function get_callsign(data: string): string;
var
  i: integer;
  s: string;
  a: byte;
begin
  s:='';
  if length(data)=7 then
  begin
	for i:=1 to 6 do
	begin
	  a:=ord(data[i]) shr 1;
	  if (a in [$30..$39,$41..$5A]) then s:=s+chr(a);
	end;
	a:=ord(data[7]) shr 1 and 15;
	if a>0 then s:=s+'-'+inttostr(a);
  end
  else
  begin
	if length(data)>0 then
	begin
	  for i:=1 to length(data) do
		if i=1 then s:=dec2hex(ord(data[i])) else s:=s+':'+dec2hex(ord(data[i]));
	end;
  end;
  if s<>'' then s:=s+' ';
  result:=s;
end;

function get_IP(data: string): string;
var
  i: integer;
  s: string;
begin
  s:='';
  if length(data)>0 then
  begin
	for i:=1 to length(data) do
	  if i=1 then s:=inttostr(ord(data[i])) else s:=s+'.'+inttostr(ord(data[i]));
  end;
  if s<>'' then s:=s+' ';
  result:=s;
end;

const
  opcode: array [0..3] of string = ('ARP Request','ARP Response','RARP Request','RARP Response');
var
  oper: word;
  hlen,plen: byte;
  sha,spa,tha,tpa: string;
  s: string;
  i: word;
begin
  s:=data;
  if length(data)>7 then
  begin
	hlen:=ord(data[5]);
	plen:=ord(data[6]);
	oper:=(ord(data[7]) shl 8 or ord(data[8])) and 2;
	i:=9;      sha:=get_callsign(copy(data,i,hlen));
	i:=i+hlen; spa:=get_ip(copy(data,i,plen));
	i:=i+plen; tha:=get_callsign(copy(data,i,hlen));
	i:=i+hlen; tpa:=get_ip(copy(data,i,plen));
	s:='  [ARP] '+opcode[oper]+' from '+sha+spa+'to '+tha+tpa;
  end;
  result:=s;
end;

function parse_NETROM(data: string; f_id: byte): string;

  function deshift_AX25(data: string): string;
  var
	i: byte;
	call: string[6];
	ssid: string[2];
  begin
	result:='';
	if length(data)<7 then exit;
	for i:=1 to 7 do data[i]:=chr(ord(data[i]) shr 1);
	call:=trim(copy(data,1,6));
	ssid:=trim(inttostr(ord(data[7]) and 15));
	if ssid='0' then result:=call else result:=call+'-'+ssid;
  end;

  function con_req_info(data: string): string;
  var
	s_call: string;
	d_call: string;
	w: byte;
	t_o: byte;
  begin
	result:='';
	if length(data)>14 then
	begin
	  w:=ord(data[1]);
	  s_call:=deshift_AX25(copy(data,2,7));
	  d_call:=deshift_AX25(copy(data,9,7));
	  result:=' w='+inttostr(w)+' '+s_call+' at '+d_call;
	end;
	if length(data)>15 then
	begin
	  t_o:=ord(data[16]);
	  result:=result+' t/o '+inttostr(t_o);
	end;
  end;

  function con_ack_info(data: string): string;
  var
	w: byte;
  begin
	result:='';
	if length(data)>0 then
	begin
	  w:=ord(data[1]);
	  result:=' w='+inttostr(w);
	end;
  end;

const
  opcode_arr: array[0..7] of string = ('PE','CON REQ','CON ACK','DISC REQ','DISQ ACK','INFO','INFO ACK','RST');
var
  s: string;
  netrom_header: string;
  c_idx:  byte;
  c_ID:   byte;
  TX_nr:  byte;
  RX_nr:  byte;
  opcode: byte;
  s_call: string;
  s_node: string;
  d_call: string;
  d_node: string;
  b_call: string;
  r_s_nr: string;
  opc_flags: string;
  quality: byte;
  ttl: byte;
  hops: byte;
  rtt: word;
  inp3_nr_field: byte;
  inp3_field_len: byte;
  inp3_ext_fields: boolean;
begin
  s:=data;
  if length(data)>0 then
  begin
	if data[1]=#$FF then
	begin
	  delete(data,1,1);
	  //Nodes broadcasting
	  if (f_id=U_UI) and (length(data)>5) then
	  begin
		s_node:=copy(data,1,6);
		delete(data,1,6);
		s:='NODES broadcast from '+s_node+#13#10;
		while length(data)>20 do
		begin
		  d_call:=deshift_AX25(copy(data,1,7));
		  d_node:=copy(data,8,6);
		  b_call:=deshift_AX25(copy(data,14,7));
		  quality:=ord(data[21]);
		  delete(data,1,21);
		  s:=s+'  '+d_node+':'+d_call+' via '+b_call+' Q='+inttostr(quality)+#13#10;
		end;
	  end;
	  // INP3 RIF
	  if (f_id=I_I) and (length(data)>10) then
	  begin
		s:='[INP3 RIF]'+#13#10;
		while length(data)>10 do
		begin
		  d_call:=deshift_AX25(copy(data,1,7));
		  hops:=ord(data[8]);
		  rtt:=(ord(data[9]) shl 8) or ord(data[10]);
		  delete(data,1,10);
		  inp3_ext_fields:=TRUE;
		  inp3_nr_field:=0;
		  while (length(data)>0) and inp3_ext_fields do
		  begin
			inp3_field_len:=ord(data[1]);
			if inp3_field_len>0 then
			begin
			  if (inp3_nr_field=0) and (length(data)>1) then
			  begin
				if data[2]=#0 then d_call:=copy(data,3,inp3_field_len-2)+':'+d_call; // Copy alias
			  end;
			  delete(data,1,inp3_field_len);
			  inc(inp3_nr_field);
			end
			else inp3_ext_fields:=FALSE;
		  end;
		  delete(data,1,1);
		  s:=s+d_call+' hops='+inttostr(hops)+' rtt='+inttostr(rtt)+#13#10;
		end;
	  end;
	end
	else
	begin
	  // NETROM frames
	  if length(data)>19 then
	  begin
		s_call:=deshift_AX25(copy(data,1,7));
		d_call:=deshift_AX25(copy(data,8,7));
		ttl:=ord(data[15]);
		netrom_header:=copy(data,16,5);
		delete(data,1,20);
		c_idx:=ord(netrom_header[1]);
		c_ID:=ord(netrom_header[2]);
		TX_nr:=ord(netrom_header[3]);
		RX_nr:=ord(netrom_header[4]);
		opcode:=ord(netrom_header[5]);
		// Opcode flags
		opc_flags:='';
		if opcode and 128 = 128 then opc_flags:=opc_flags+' C';
		if opcode and 64  = 64  then opc_flags:=opc_flags+' N';
		//
		s:='  [NETROM] '+s_call+' to '+d_call+' ttl='+inttostr(ttl)+' cct='+dec2hex(c_idx)+dec2hex(c_ID);
		r_s_nr:=' S'+inttostr(TX_nr)+' R'+inttostr(RX_nr);
		case (opcode and 7) of
		  0 : s:=s+' <'+opcode_arr[opcode and 7]+r_s_nr+'>'+#13#10+data;
		  1 : s:=s+' <'+opcode_arr[opcode and 7]+'>'+con_req_info(data);
		  2 : s:=s+' <'+opcode_arr[opcode and 7]+'>'+con_ack_info(data)+' my cct='+dec2hex(TX_nr)+dec2hex(RX_nr);
		  3 : s:=s+' <'+opcode_arr[opcode and 7]+'>';
		  4 : s:=s+' <'+opcode_arr[opcode and 7]+'>';
		  5 : s:=s+' <'+opcode_arr[opcode and 7]+r_s_nr+'>:'+#13#10+data;
		  6 : s:=s+' <'+opcode_arr[opcode and 7]+' R'+inttostr(RX_nr)+'>'+opc_flags;
		  7 : s:=s+' <'+opcode_arr[opcode and 7]+r_s_nr+'>'+#13#10+data;
		end;
	  end;
	end;
  end;
  result:=s;
end;

function parse_IP(data: string): string;

  function parse_ICMP(var data: string): string;
  var
	ICMP_type: byte;
	ICMP_code: byte;
	s: string;
  begin
	result:='';
	if length(data)>3 then
	begin
	  ICMP_type:=ord(data[1]);
	  ICMP_code:=ord(data[2]);
	  delete(data,1,4);
	  s:='  [ICMP] Type='+inttostr(ICMP_type)+' Code='+inttostr(ICMP_code)+#13#10;
	  result:=s;
	end;
  end;

  function parse_TCP(var data: string): string;
  var
	s: string;
	src_port: string;
	dest_port: string;
	wnd: string;
	ihl: word;
	idl: word;
	flags: byte;
	seq: string;
	ack: string;
	s_flags: string;
	s_idl: string;
  begin
	result:='';
	if length(data)>19 then
	begin
	  src_port:=' src_port:'+inttostr((ord(data[1]) shl 8)+ord(data[2]));
	  dest_port:=' dest_port:'+inttostr((ord(data[3]) shl 8)+ord(data[4]));
	  seq:=' seq='+dec2hex(ord(data[5]))+dec2hex(ord(data[6]))+dec2hex(ord(data[7]))+dec2hex(ord(data[8]));
	  ack:=' ack='+dec2hex(ord(data[9]))+dec2hex(ord(data[10]))+dec2hex(ord(data[11]))+dec2hex(ord(data[12]));
	  ihl:=(ord(data[13]) shr 4)*4;
	  idl:=length(data)-ihl;
	  flags:=ord(data[14]);
	  wnd:=' wnd='+inttostr((ord(data[15]) shl 8)+ord(data[16]));
	  delete(data,1,ihl);
	  //
	  s_flags:=' ';
	  if (flags and 32)=32 then s_flags:=s_flags+'URG ';
	  if (flags and 16)=16 then s_flags:=s_flags+'ACK ';
	  if (flags and 8)=8   then s_flags:=s_flags+'PSH ';
	  if (flags and 4)=4   then s_flags:=s_flags+'RST ';
	  if (flags and 2)=2   then s_flags:=s_flags+'SYN ';
	  if (flags and 1)=1   then s_flags:=s_flags+'FIN ';
	  //
	  if idl>0 then s_idl:=' data='+inttostr(idl) else s_idl:='';
	  if (flags and 16)<>16 then ack:='';
	  //
	  s:='  [TCP]'+src_port+dest_port+seq+ack+wnd+s_idl+s_flags+#13#10;
	  result:=s;
	end;
  end;

  function parse_UDP(var data: string): string;
  var
	s: string;
	src_port: string;
	dest_port: string;
	idl: word;
	len: word;
	s_idl: string;
  begin
	result:='';
	if length(data)>7 then
	begin
	  src_port:=' src_port:'+inttostr((ord(data[1]) shl 8)+ord(data[2]));
	  dest_port:=' dest_port:'+inttostr((ord(data[3]) shl 8)+ord(data[4]));
	  len:=(ord(data[5]) shl 8)+ord(data[6]);
	  idl:=len-8;
	  delete(data,1,8);
	  //
	  if idl>0 then s_idl:=' data='+inttostr(idl) else s_idl:='';
	  //
	  s:='  [UDP]'+src_port+dest_port+' len='+inttostr(len)+s_idl+#13#10;
	  result:=s;
	end;
  end;

const
  prot_idx=#1#6#17;
  prot_name: array [1..3] of string = ('ICMP','TCP','UDP');
var
  s: string;
  src_ip: string;
  dest_ip: string;
  s_prot: string;
  len: string;
  c_prot: char;
  ttl: string;
  offset: string;
  ihl: byte;
  p: byte;
  fragment_offset: word;
begin
  s:=data;
  if length(data)>19 then
  begin
	ihl:=(ord(data[1]) and 15)*4;
	len:=' len='+inttostr((ord(data[3]) shl 8)+ord(data[4]));
	fragment_offset:=((ord(data[7]) shl 8)+ord(data[8])) shl 3;
	ttl:=' ttl='+inttostr(ord(data[9]));
	c_prot:=data[10];
	src_ip:=' Fm '+inttostr(ord(data[13]))+'.'+inttostr(ord(data[14]))+'.'+inttostr(ord(data[15]))+'.'+inttostr(ord(data[16]));
	dest_ip:=' To '+inttostr(ord(data[17]))+'.'+inttostr(ord(data[18]))+'.'+inttostr(ord(data[19]))+'.'+inttostr(ord(data[20]));
	delete(data,1,ihl);
	//
	p:=pos(c_prot,prot_idx);
	if p>0 then s_prot:=' prot='+prot_name[p] else s_prot:=' prot=Type'+inttostr(ord(c_prot));
	if fragment_offset>0 then offset:=' offset='+inttostr(fragment_offset) else offset:='';
	s:='  [IP]'+src_ip+dest_ip+s_prot+ttl+len+offset+#13#10;
	if fragment_offset=0 then
	case p of
	  1 : s:=s+parse_ICMP(data);
	  2 : s:=s+parse_TCP(data);
	  3 : s:=s+parse_UDP(data);
	end;
	s:=s+data;
  end;
  result:=s;
end;

function get_UTC_time: string;
var
  st: TSYSTEMTIME;
  sec,hour,minute: string;
begin
  GetSystemTime(st);
  if st.wSecond<10 then sec:='0'+inttostr(st.wSecond) else sec:=inttostr(st.wSecond);
  if st.wMinute<10 then minute:='0'+inttostr(st.wMinute) else minute:=inttostr(st.wMinute);
  if st.wHour<10 then Hour:='0'+inttostr(st.wHour) else Hour:=inttostr(st.wHour);
  result:=hour+':'+minute+':'+sec;
end;

function dec2hex(value: byte): string;
const
  hex='0123456789ABCDEF';
var
  lo,hi: byte;
begin
  lo:=value and 15;
  hi:=value shr 4;
  result:=hex[hi+1]+hex[lo+1];
end;

*/

unsigned short get_fcs(UCHAR * Data, unsigned short len)
{
	unsigned short i;
	unsigned short result;

	result = 0xFFFF;

	if (len == 0)
		return result;

	for (i = 0; i < len; i++)
		result = (result >> 8) ^ CRCTable[(result ^ Data[i]) & 0xff];


	result ^= 0xffff;

	return result;
}


unsigned short CRCTAB[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

unsigned short int compute_crc(unsigned char *buf, int len)
{
	unsigned short fcs = 0xffff;
	int i;

	for (i = 0; i < len; i++)
		fcs = (fcs >> 8) ^ CRCTAB[(fcs ^ buf[i]) & 0xff];

	fcs ^= 0xffff;

	return fcs;
}

/*

function info_pid(pid: byte): string;
begin
  if ((pid and 48=1) or (pid and 48=2)) then result:='0';
  if pid=204 then result:='1';
  if pid=221 then result:='2';
  if pid=240 then result:='3';
  if pid=255 then result:='4';
end;

procedure explode(var a: array of string; border, s: string; p1: word);
var
  s2: string;
  i: integer;
begin
  i:=0;
  s2:=s+border;
  repeat
	a[i]:=copy(s2,0,pos(border,s2)-1);
	delete(s2,1,length(a[i]+border));
	inc(i);
  until (s2='') or (i=p1);
end;

function set_addr(path: string; rpt,cr: boolean): string;
const
  A_RX=0;
  A_TX=1;
  C_BIT=64;
  H_BIT=64;
  RR_BIT=48;
var
  cnt,i: byte;
  a_call: array [0..1] of string;
  a_path: array [0..ADDR_MAX_LEN-1] of string;
  calls: array [0..ADDR_MAX_LEN-1] of string;
  ssids: array [0..ADDR_MAX_LEN-1] of byte;
  addr: string;
  n: word;
begin
  for i:=0 to ADDR_MAX_LEN-1 do
  begin
	a_path[i]:='';
	calls[i]:='';
	ssids[i]:=0;
  end;
  //Разделяем позывные
  try explode(a_path,',',path,ADDR_MAX_LEN); except end;
  //Разделяем позывные и ssid
  cnt:=0;
  repeat
	a_call[0]:=''; a_call[1]:='';
	try explode(a_call,'-',a_path[cnt],2); except end;
	if a_call[0]<>'' then
	begin
	  calls[cnt]:=copy(a_call[0]+'      ',1,6);
	  if a_call[1]='' then a_call[1]:='0';
	  try ssids[cnt]:=strtoint(a_call[1]); except ssids[cnt]:=0; end;
	  inc(cnt);
	end;
  until (a_call[0]='') or (cnt=ADDR_MAX_LEN);
  //Формируем адресную строку
  addr:='';
  if cnt>1 then
  begin
	for i:=0 to cnt-1 do
	begin
	  if (cr=SET_C) and (i=A_RX) then ssids[i]:=ssids[i]+C_BIT; //Добавляем C в CRRSSID1
	  if (cr=SET_R) and (i=A_TX) then ssids[i]:=ssids[i]+C_BIT; //Добавляем C в CRRSSID1
	  if rpt and (i>A_TX) then
		if calls[i]=mydigi then ssids[i]:=ssids[i]+H_BIT; //Добавляем H в HRRSSID1
	  ssids[i]:=ssids[i]+RR_BIT; //Добавляем RR в xRRSSID1
	  addr:=addr+calls[i]+chr(ssids[i]);
	end;
	for n:=1 to length(addr) do addr[n]:=chr(ord(addr[n]) shl 1); //сдвигаем на 1 бит октеты
	addr[length(addr)]:=chr(ord(addr[length(addr)]) xor 1); //устанавливаем конец адреса
  end;
  result:=addr;
end;
*/

int get_addr(char * Calls, UCHAR * AXCalls)
{
	//	CONVERT CALL + OPTIONAL DIGI STRING TO AX25, RETURN 
	//	CONVERTED STRING IN AXCALLS. Return FALSE if invalied

	Byte * axptr = AXCalls;
	char * ptr, *Context;
	int n = 8;						// Max digis

	memset(AXCalls, 0, 70);

	ptr = strtok_s(Calls, " ,", &Context);

	if (ptr == NULL)
		return FALSE;

	// First field is Call

	if (ConvToAX25(ptr, axptr) == 0)
		return FALSE;

	axptr += 7;

	ptr = strtok_s(NULL, " ,", &Context);

	if (ptr == NULL)
		return FALSE;

	if (ConvToAX25(ptr, axptr) == 0)
		return FALSE;

	axptr += 7;

	ptr = strtok_s(NULL, " ,", &Context);

	while (ptr && n--)
	{
		// NEXT FIELD = COULD BE CALLSIGN, VIA, 

		if (memcmp(ptr, "VIA", (int)strlen(ptr)) == 0)
		{
		}	//skip via
		else
		{
			// Convert next digi

			if (ConvToAX25(ptr, axptr) == 0)
				return FALSE;

			axptr += 7;
		}

		ptr = strtok_s(NULL, " ,", &Context);
	}

	axptr[-1] |= 1;			// Set end of address

	return axptr - AXCalls;
}

Byte set_ctrl(Byte nr, Byte ns, Byte f_type, Byte f_id, boolean pf)
{
	Byte  pf_bit, ctrl;

	ctrl = 0;
	pf_bit = 0;

	if (pf)
		pf_bit = 16;

	switch (f_type)
	{
	case I_FRM:

		ctrl = (nr << 5) + pf_bit + (ns << 1);
		break;

	case S_FRM:

		ctrl = (nr << 5) + pf_bit + f_id;
		break;

	case U_FRM:

		ctrl = f_id + pf_bit;
	}

	return ctrl;
}

string * make_frame(string * data, Byte * axaddr,  Byte pid, Byte nr, Byte ns, Byte f_type, Byte f_id, boolean rpr, boolean pf, boolean cr)
{
	UNUSED(rpr);

	Byte ctrl;

	string * frame = newString();
	int addrlen;
	Byte addr[80];

	unsigned short CRC;
	UCHAR CRCString[2];

	frame->Data[0] = 0;					// Lower software expects a kiss control byte here
	frame->Length = 1;

	ctrl = set_ctrl(nr, ns, f_type, f_id, pf);

	addrlen = strlen((char *)axaddr);

	memcpy(addr, axaddr, addrlen);

	if (cr)
		addr[6] |= 0x80;		// Set Command Bit
	else
		addr[13] |= 0x80;	// Set Response Bit


	switch (f_type)
	{
	case I_FRM:

		stringAdd(frame, addr, addrlen);
		stringAdd(frame, (Byte *)&ctrl, 1);
		stringAdd(frame, (Byte *)&pid, 1);
		stringAdd(frame, data->Data, data->Length);

		break;


	case S_FRM:

		stringAdd(frame, addr, addrlen);
		stringAdd(frame, (Byte *)&ctrl, 1);

		break;

	case U_FRM:

		if (f_id == U_UI)
		{
			stringAdd(frame, addr, addrlen);
			stringAdd(frame, (Byte *)&ctrl, 1);
			stringAdd(frame, (Byte *)&pid, 1);
			stringAdd(frame, data->Data, data->Length);
		}
		else if (f_id == U_FRMR)
		{
			stringAdd(frame, addr, addrlen);
			stringAdd(frame, (Byte *)&ctrl, 1);
			stringAdd(frame, data->Data, data->Length);
		}
		else
		{
			stringAdd(frame, addr, addrlen);
			stringAdd(frame, (Byte *)&ctrl, 1);
		}
	}

	CRC = get_fcs(&frame->Data[1], frame->Length - 1);

	CRCString[0] = CRC & 0xff;
	CRCString[1] = CRC >> 8;
	stringAdd(frame, CRCString, 2);

	return frame;
}


int add_raw_frames(int snd_ch, string * frame, TStringList * buf)
{
	string  *s_data = newString();
	Byte  s_pid, s_nr, s_ns, s_f_type, s_f_id;
	Byte  s_rpt, s_cr, s_pf;
	string  *d_data = newString();
	Byte  d_pid, d_nr, d_ns, d_f_type, d_f_id;
	Byte  d_rpt, d_cr, d_pf;

	Byte d_path[80];
	Byte s_path[80];

	boolean  found_I;
	int  i;

	unsigned char * framecontents;
	int Length;

	boolean result = TRUE;

	// Have to be careful as at this point frames have KISS Header and maybe trailer

	if (buf->Count > 0)
	{
		// Check for duplicate. Ok to just compare as copy will have same header

		if (my_indexof(buf, frame) >= 0)
		{
			Debugprintf("KISSOptimise discarding duplicate frame");
			return FALSE;
		}

		// Need to adjust for KISS bytes

		// Normally one, but ackmode has 3 on front and sizeof(void *) on end

		framecontents = frame->Data;
		Length = frame->Length;

		if ((framecontents[0] & 15) == 12)		// Ackmode
		{
			framecontents += 3;
			Length -= (3 + sizeof(void *));
		}
		else
		{
			framecontents++;
			Length--;
		}

		decode_frame(framecontents, Length, s_path, s_data, &s_pid, &s_nr, &s_ns, &s_f_type, &s_f_id, &s_rpt, &s_pf, &s_cr);

		found_I = FALSE;

		// check for multiple RR (r)

		if (s_f_id == S_FRM && s_cr == SET_R)
		{
			for (i = 0; i < buf->Count; i++)
			{
				framecontents = buf->Items[i]->Data;
				Length = buf->Items[i]->Length;

				if ((framecontents[0] & 15) == 12)		// Ackmode
				{
					framecontents += 3;
					Length -= (3 + sizeof(void *));
				}
				else
				{
					framecontents++;
					Length--;
				}

				decode_frame(framecontents, Length, d_path, d_data, &d_pid, &d_nr, &d_ns, &d_f_type, &d_f_id, &d_rpt, &d_pf, &d_cr);

				if (d_f_id == S_FRM && d_cr == SET_R && strcmp((char *)s_path, (char *)d_path) == 0)
				{
					Delete(buf, i);
					Debugprintf("KISSOptimise discarding unneeded RR(R%d)", d_nr);

					break;
				}
			}
		}

	
		// check for RR after I Frame
	
		if (s_f_id == S_FRM && s_cr == SET_C)
		{
			for (i = 0; i < buf->Count; i++)
			{
				framecontents = buf->Items[i]->Data;
				Length = buf->Items[i]->Length;

				if ((framecontents[0] & 15) == 12)		// Ackmode
				{
					framecontents += 3;
					Length -= (3 + sizeof(void *));
				}
				else
				{
					framecontents++;
					Length--;
				}

				decode_frame(framecontents, Length, d_path, d_data, &d_pid, &d_nr, &d_ns, &d_f_type, &d_f_id, &d_rpt, &d_pf, &d_cr);

				if (d_f_id == I_FRM && strcmp((char *)s_path, (char *)d_path) == 0)
				{
					found_I = TRUE;
					break;
				}
			}

			if (found_I)
			{
				Debugprintf("KISSOptimise discarding unneeded RR(C %d) after I frame", s_nr);
				result = FALSE;
			}
		}
			
		// check on I

		if (s_f_id == I_FRM)
		{
			for (i = 0; i < buf->Count; i++)
			{
				framecontents = buf->Items[i]->Data;
				Length = buf->Items[i]->Length;

				if ((framecontents[0] & 15) == 12)		// Ackmode
				{
					framecontents += 3;
					Length -= (3 + sizeof(void *));
				}
				else
				{
					framecontents++;
					Length--;
				}

				decode_frame(framecontents, Length, d_path, d_data, &d_pid, &d_nr, &d_ns, &d_f_type, &d_f_id, &d_rpt, &d_pf, &d_cr);

				if (strcmp((char *)s_path, (char *)d_path) == 0 && d_f_id == S_FRM && d_cr == SET_C)
				{
					Delete(buf, i);
					Debugprintf("KISSOptimise discarding unneeded RR(C %d)", d_nr);
					i--;				// i was removed
				}
			}
		}
	}

	freeString(d_data);
	freeString(s_data);

	return result;
}

//////////////////////// Register incoming callsign ////////////////////////////

// I think a call should only be registered on one socket (or we won't know where to send
// incoming calls

boolean add_incoming_mycalls(void * socket, char * src_call)
{
	registeredCalls * reg = malloc(sizeof(struct registeredCalls_t));
	int i = 0;

	// Build a string containing Call and Socket

	ConvToAX25(src_call, reg->myCall);
	reg->socket = socket;

	if (list_incoming_mycalls.Count > 0)
	{
		for (i = 0; i < list_incoming_mycalls.Count; i++)
		{
			registeredCalls * check = (registeredCalls *)list_incoming_mycalls.Items[i];

			if (memcmp(check->myCall, reg->myCall, 7) == 0)
			{
				// Update socket

				check->socket = socket;
				return FALSE;
			}
		}
	}

	Add(&list_incoming_mycalls, (string *)reg);
	return TRUE;
}



void del_incoming_mycalls(char * src_call)
{
	int i = 0;
	Byte axcall[7];
	registeredCalls * reg;

	ConvToAX25(src_call, axcall);

	while (i < list_incoming_mycalls.Count)
	{
		reg = (registeredCalls *)list_incoming_mycalls.Items[i];
		{
			if (memcmp(reg->myCall, axcall, 7) == 0)
			{
				// cant use Delete as stringlist doesn't contain strings

				TStringList * Q = &list_incoming_mycalls;
				int Index = i;

				free(Q->Items[Index]);

				Q->Count--;

				while (Index < Q->Count)
				{
					Q->Items[Index] = Q->Items[Index + 1];
					Index++;
				}

				return;
			}
		}
		i++;
	}
}


void del_incoming_mycalls_by_sock(void * socket)
{
	int i = 0, snd_ch, port;
	registeredCalls * reg;

	while (i < list_incoming_mycalls.Count)
	{
		reg = (registeredCalls *)list_incoming_mycalls.Items[i];
		{
			if (reg->socket == socket)
			{
				// cant use Delete as stringlist doesn't contain strings

				TStringList * Q = &list_incoming_mycalls;
				int Index = i; 
		
				free(Q->Items[Index]);

				Q->Count--;

				while (Index < Q->Count)
				{
					Q->Items[Index] = Q->Items[Index + 1];
					Index++;
				}
				
				//Delete(&list_incoming_mycalls, i);
			}
			else
				i++;
		}
	}

	// Should clear all connections on socket

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		for (port = 0; port < port_num; port++)
		{
			TAX25Port * AX25Sess = &AX25Port[snd_ch][port];

			if (AX25Sess->socket == socket)
			{
				if (AX25Sess->status != STAT_NO_LINK)
				{
					// Shouldn't we send DM? -0 try it

					set_DM(snd_ch, AX25Sess->ReversePath);

					rst_timer(AX25Sess);
					rst_values(AX25Sess);

					AX25Sess->status = STAT_NO_LINK;
				}
				AX25Sess->socket = 0;
			}
		}
	}
}


/*
function get_incoming_socket_by_call(src_call: string): integer;
var
  i: integer;
  found: boolean;
  socket: integer;
  call,ssid: string;
  a_call: array[0..1] of string;
begin
  socket:=-1;
  i:=0;
  found:=FALSE;
  try explode(a_call,'-',src_call,2); except end;
  call:=trim(a_call[0]);
  if a_call[1]<>'' then ssid:=a_call[1] else ssid:='0';
  if list_incoming_mycalls.Count>0 then
  repeat
	if (call+'-'+ssid)=list_incoming_mycalls.Strings[i] then
	begin socket:=strtoint(list_incoming_mycalls_sock.Strings[i]); found:=TRUE; end;
	inc(i);
  until found or (i=list_incoming_mycalls.Count);
  result:=socket;
end;
*/



void * in_list_incoming_mycall(Byte * path)
{
	// See if to call is in registered calls list

	int i = 0;
	registeredCalls * check;		// list_incoming_mycalls contains registeredCalls, not Strings
	
	while (i < list_incoming_mycalls.Count)
	{
		check = (registeredCalls *)list_incoming_mycalls.Items[i];

		if (memcmp(check->myCall, path, 7) == 0)
			return check->socket;

		i++;
	}

	return NULL;
}

/*
////////////////////////////////////////////////////////////////////////////////

function is_corrcall(snd_ch,port: byte; path: string): boolean;
var
  call,ssid: string;
begin
  call:=trim(copy(path,8,6));
  ssid:=copy(path,14,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15)) else ssid:='0';
  call:=call+'-'+ssid;
  if call=AX25Sess->corrcall then result:=TRUE else result:=FALSE;
end;

function is_mycall(snd_ch,port: byte; path: string): boolean;
var
  call,ssid: string;
begin
  call:=trim(copy(path,1,6));
  ssid:=copy(path,7,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15)) else ssid:='0';
  call:=call+'-'+ssid;
  if call=AX25Sess->mycall then result:=TRUE else result:=FALSE;
end;

function is_digi(snd_ch,port: byte; path: string): boolean;
var
  digi,call,ssid: string;
begin
  digi:='';
  if length(path)>14 then
  begin
	delete(path,1,14);
	repeat
	  call:=trim(copy(path,1,6));
	  ssid:=copy(path,7,1);
	  delete(path,1,7);
	  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15))
	  else ssid:='0';
	  if path<>'' then digi:=digi+call+'-'+ssid+','
	  else digi:=digi+call+'-'+ssid;
	until path='';
  end;
  if digi=AX25Sess->digi then result:=TRUE else result:=FALSE;
end;
*/

// Check if laast digi used

boolean is_last_digi(Byte *path)
{
	int len = strlen(path);

	if (len == 14)
		return TRUE;

	if ((path[len - 1] & 128) == 128)
		return TRUE;

	return FALSE;
}



/*
function get_corrcall(path: string): string;
var
  call,ssid: string;
begin
  call:=trim(copy(path,8,6));
  ssid:=copy(path,14,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15)) else ssid:='0';
  call:=call+'-'+ssid;
  result:=call;
end;

function get_mycall(path: string): string;
var
  call,ssid: string;
begin
  call:=trim(copy(path,1,6));
  ssid:=copy(path,7,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15)) else ssid:='0';
  call:=call+'-'+ssid;
  result:=call;
end;

function get_digi(path: string): string;
var
  digi,call,ssid: string;
begin
  digi:='';
  if length(path)>14 then
  begin
	delete(path,1,14);
	repeat
	  call:=trim(copy(path,1,6));
	  ssid:=copy(path,7,1);
	  delete(path,1,7);
	  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15))
	  else ssid:='0';
	  if path<>'' then digi:=digi+call+'-'+ssid+','
	  else digi:=digi+call+'-'+ssid;
	until path='';
  end;
  result:=digi;
end;
*/

boolean is_correct_path(Byte * path, Byte pid)
{
	Byte networks[] = { 6, 7, 8, 0xc4, 0xcc, 0xcd, 0xce, 0xcf, 0xf0 , 0 };
	Byte call[10];
	int i;


	if (pid == 0 || strchr(networks, pid))
	{
		// Validate calls

		// I think checking bottom  bit of first 13 bytes is enough

		for (i = 0; i < 13; i++)
		{
			if ((*(path) & 1))
				return FALSE;

			path++;
		}
		return TRUE;
	}
	return FALSE;
}


void get_exclude_list(char * line, TStringList * list)
{
	// Convert comma separated list of calls to ax25 format in list
	// Convert to 6 chars - SSID is ignored

	string axcall;

	char copy[512];

	char * ptr, *Context;

	if (line[0] == 0)
		return;

	strcpy(copy, line);						// copy as strtok messes with it
	strcat(copy, ",");

	axcall.Length = 6;
	axcall.AllocatedLength = 8;
	axcall.Data = malloc(8);

	memset(axcall.Data, 0, 8);

	ptr = strtok_s(copy, " ,", &Context);

	while (ptr)
	{
		if (ConvToAX25(ptr, axcall.Data) == 0)
			return;

		axcall.Data[6] = 0;

		Add(list, duplicateString(&axcall));

		ptr = strtok_s(NULL, " ,", &Context);
	}
}

void get_digi_list(char * line, TStringList * list)
{
	// Convert comma separated list of calls to ax25 format in list

	string axcall;

	char copy[512];

	char * ptr, *Context;

	if (line[0] == 0)
		return;

	strcpy(copy, line);						// copy as strtok messes with it
	strcat(copy, ",");

	axcall.Length = 7;
	axcall.AllocatedLength = 8;
	axcall.Data = malloc(8);

	memset(axcall.Data, 0, 8);

	ptr = strtok_s(copy, " ,", &Context);

	while (ptr)
	{
		if (ConvToAX25(ptr, axcall.Data) == 0)
			return;

		Add(list, duplicateString(&axcall));
		ptr = strtok_s(NULL, " ,", &Context);
	}
}



void get_exclude_frm(char * line, TStringList * list)
{
	/*

  s: string;
  p: integer;
  n: integer;
begin
  list.Clear;
  if line='' then exit;
  repeat
	p:=pos(',',line);
	if p>0 then
	  begin
		s:=trim(copy(line,1,p-1));
		if s<>'' then
		begin
		  try n:=strtoint(s); except n:=-1; end;
		  if n in [0..255] then list.Add(chr(n));
		end;
		delete(line,1,p);
	  end
	else
	  begin
		s:=trim(line);
		if s<>'' then
		begin
		  try n:=strtoint(s); except n:=-1; end;
		  if n in [0..255] then list.Add(chr(n));
		end;
	  end;
  until p=0;
end;
*/
}

/*

function is_excluded_call(snd_ch: byte; path: string): boolean;
var
  excluded: boolean;
  call: string;
  ssid: string;
begin
  excluded:=FALSE;
  if (list_exclude_callsigns[snd_ch].Count>0) and (length(path)>13) then
  begin
	// Copy sender
	call:=trim(copy(path,8,6));
	ssid:=copy(path,14,1);
	if ssid<>'' then call:=call+'-'+inttostr((ord(ssid[1]) and 15));
	if list_exclude_callsigns[snd_ch].IndexOf(call)>-1 then excluded:=TRUE;
  end;
  result:=excluded;
end;

function is_excluded_frm(snd_ch,f_id: byte; data: string): boolean;
var
  excluded: boolean;
begin
  excluded:=FALSE;
  if list_exclude_APRS_frm[snd_ch].Count>0 then
	if f_id=U_UI then
	  if length(data)>0 then
		if list_exclude_APRS_frm[snd_ch].IndexOf(data[1])>=0 then excluded:=TRUE;
  result:=excluded;
end;

procedure set_corrcall(snd_ch,port: byte; path: string);
var
  call,ssid: string;
begin
  call:=trim(copy(path,8,6));
  ssid:=copy(path,14,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15))
  else ssid:='0';
  AX25Sess->corrcall:=call+'-'+ssid;
end;

procedure set_mycall(snd_ch,port: byte; path: string);
var
  call,ssid: string;
begin
  call:=trim(copy(path,1,6));
  ssid:=copy(path,7,1);
  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15))
  else ssid:='0';
  AX25Sess->mycall:=call+'-'+ssid;
end;

procedure set_digi(snd_ch,port: byte; path: string);
var
  digi,call,ssid: string;
begin
  digi:='';
  if length(path)>14 then
  begin
	delete(path,1,14);
	repeat
	  call:=trim(copy(path,1,6));
	  ssid:=copy(path,7,1);
	  delete(path,1,7);
	  if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15))
	  else ssid:='0';
	  if path<>'' then digi:=digi+call+'-'+ssid+','
	  else digi:=digi+call+'-'+ssid;
	until path='';
  end;
  AX25Sess->digi:=digi;
end;

procedure get_call_fm_path(path: string; var callto,callfrom: string);
var
  a_path: array [0..ADDR_MAX_LEN-1] of string;
  i: byte;
begin
  for i:=0 to ADDR_MAX_LEN-1 do a_path[i]:='';
  try explode(a_path,',',path,ADDR_MAX_LEN); except end;
  callto:=a_path[0];
  callfrom:=a_path[1];
end;

procedure get_call(src_call: string; var call: string);
var
  a_call: array[0..1] of string;
  ssid: string;
begin
  try explode(a_call,'-',src_call,2); except end;
  call:=trim(a_call[0]);
  if a_call[1]<>'' then ssid:=trim(a_call[1]) else ssid:='0';
  call:=call+'-'+ssid;
end;
*/



int is_excluded_call(int snd_ch, unsigned char * path)
{
	string * call = newString();
	int Excluded = FALSE;

	stringAdd(call, &path[7], 6);
	
	if (list_exclude_callsigns[snd_ch].Count > 0)
		if (my_indexof(&list_exclude_callsigns[snd_ch], call) > -1)
			Excluded = TRUE;
	
	freeString(call);
	return Excluded;
}


int  is_excluded_frm(int snd_ch, int f_id, string * data)
{
	if (f_id == U_UI)
		if (data->Length > 0)
			if (my_indexof(&list_exclude_APRS_frm[snd_ch], data) >= 0)
				return TRUE;

	return FALSE;
}



int number_digi(char * path)
{
	int n = 0;

	//  a_path: array [0..ADDR_MAX_LEN-3] of string;
	int i;

	// for i:=0 to ADDR_MAX_LEN-3 do a_path[i]:='';
	// try explode(a_path,',',path,ADDR_MAX_LEN-2); except end;
	// for i:=0 to ADDR_MAX_LEN-3 do if a_path[i]<>'' then inc(n);

	return n;
}



get_monitor_path(Byte * path, char * mycall, char * corrcall, char * digi)
{
	Byte * digiptr = digi;

	digi[0] = 0;

	mycall[ConvFromAX25(path, mycall)] = 0;
	path += 7;
	corrcall[ConvFromAX25(path, corrcall)] = 0;

	while ((path[6] & 1) == 0)				// End of call bit
	{
		if (digi != digiptr)
			*(digi++) = ',';

			path += 7;
			digi += ConvFromAX25(path, digi);

			if (((path[6] & 128) == 128))		// Digi'd
				*(digi++) = '*';
	}
	*digi = 0;
}


/*

function reverse_digi(path: string): string;
var
  digi: string;
  a_path: array [0..ADDR_MAX_LEN-3] of string;
  i: word;
begin
  digi:='';
  for i:=0 to ADDR_MAX_LEN-3 do a_path[i]:='';
  try explode(a_path,',',path,ADDR_MAX_LEN-2); except end;
  for i:=0 to ADDR_MAX_LEN-3 do
  if a_path[i]<>'' then
  begin
	if digi='' then digi:=a_path[i]+digi
	else digi:=a_path[i]+','+digi;
  end;
  result:=digi;
end;

function direct_addr(path: string): string;
var
  s,call,ssid: string;
begin
  s:='';
  repeat
	call:=copy(path,1,6);
	delete(path,1,6);
	ssid:=copy(path,1,1);
	delete(path,1,1);
	if ssid<>'' then ssid:=inttostr((ord(ssid[1]) and 15));
	if s='' then s:=call+'-'+ssid else s:=s+','+call+'-'+ssid;
  until path='';
  result:=s;
end;
*/

void reverse_addr(Byte * path, Byte * revpath, int Len)
{
	Byte * ptr = path;
	Byte * copy = revpath;
	int endbit = Len - 1;
	int numdigis = (Len - 14) / 7;
	int i;

	if (Len < 14)
		return;

	Byte digis[57];						// 8 * 7 + null terminator
	memset(digis, 0, 57);
	Byte * digiptr = digis + 49;			// Last Digi

	// remove end of address bit

	path[endbit] &= 0xFE;

	// first reverse dest and origin

	memcpy(copy + 7, ptr, 7);
	memcpy(copy, ptr + 7, 7);

	Len -= 14;
	ptr += 14;

	for (i = 0; i < numdigis; i++)
	{
		memcpy(digiptr, ptr, 7);
		ptr += 7;
		digiptr -= 7;
	}

	// Digiptr now points to new first digi 

	memcpy(&copy[14], &digiptr[7], 7 * numdigis);

	path[endbit] |= 1;			// restore original end bit

	copy[endbit++] |= 1;
	copy[endbit] = 0;			//  Null terminate

	return;
}



void decode_frame(Byte * frame, int len, Byte * path, string * data,
	Byte * pid, Byte * nr, Byte * ns, Byte * f_type, Byte * f_id,
	Byte *  rpt, Byte * pf, Byte * cr)
{
	int i;
	int addr_end;
	Byte ctrl;
	Byte * savepath = path;

	i = 0;
	addr_end = FALSE;

	*cr = SET_R;
	*pf = SET_F;
	data->Length = 0;
	ctrl = 0;
	*pid = 0;
	*nr = 0;
	*ns = 0;
	*f_type = 0;
	*f_id = 0;
	*rpt = FALSE;

	if (len < PKT_ERR)
		return;

	if ((frame[6] & 128) == 128 && (frame[13] & 128) == 0)
		*cr = SET_C;

	while (len > i && i < ADDR_MAX_LEN * 7)
	{
		*path++ = frame[i];
		if ((frame[i] & 1) == 1)
		{
			addr_end = TRUE;
			break;
		}
		i++;
	}

	if (addr_end == 0)
		return;

	// clear the c and r bits from address

	savepath[6] &= 0x7f;		// Mask 
	savepath[13] &= 0x7f;		// Mask 

	*path = 0;		// add null terminate		

	i++;			// Points to ctrl byte

	ctrl = frame[i];

	if ((ctrl & 16) == 16)
		*pf = SET_P;

	if ((ctrl & 1) == 0)		// I frame
	{
		*f_type = I_FRM;
		*f_id = I_I;
		*nr = (ctrl >> 5);
		*ns = (ctrl >> 1) & 7;
	}
	else
	{
		// Not I

		*f_type = U_FRM;

		*f_id = ctrl & 239;

		switch (ctrl & 15)
		{
		case  S_RR:
		case  S_RNR:
		case  S_REJ:
		case  S_SREJ:

			*f_type = S_FRM;
		}

		if (*f_type == S_FRM)
		{
			*f_id = ctrl & 15;
			*nr = ctrl >> 5;
		}
	}


	if (*f_id == I_I || *f_id == U_UI)
	{
		i++;
		*pid = frame[i];
		i++;
		if (len > i)
			stringAdd(data, &frame[i], len - i - 2);		// Exclude FCS
	}
	else if (*f_id == U_FRMR)
	{
		*pid = 0;
		i++;
		if (len > i)
			stringAdd(data, &frame[i], len - i - 2);		// Exclude FCS
	}
}
void ax25_info_init(TAX25Port * AX25Sess)
{
	AX25Sess->info.stat_s_pkt = 0;
	AX25Sess->info.stat_s_byte = 0;
	AX25Sess->info.stat_r_pkt = 0;
	AX25Sess->info.stat_r_byte = 0;
	AX25Sess->info.stat_r_fc = 0;
	AX25Sess->info.stat_fec_count = 0;
	AX25Sess->info.stat_l_r_byte = 0;
	AX25Sess->info.stat_l_s_byte = 0;
	AX25Sess->info.stat_begin_ses = 0;
	AX25Sess->info.stat_end_ses = 0;
}


void  clr_frm_win(TAX25Port * AX25Sess)
{
	int  i;

	for (i = 0; i < 8; i++)
		initString(&AX25Sess->frm_win[i]);
}

void ax25_init()
{
	int snd_ch, port, i;

	for (i = 0; i < 4; i++)
	{
		initTStringList(&all_frame_buf[i]);
		initTStringList(&list_exclude_callsigns[i]);
		initTStringList(&list_exclude_APRS_frm[i]);
		initTStringList(&list_digi_callsigns[i]);
		initTStringList(&KISS_acked[i]);

		get_digi_list(MyDigiCall[i], &list_digi_callsigns[i]);
		get_exclude_list(exclude_callsigns[i], &list_exclude_callsigns[i]);
		get_exclude_frm(exclude_APRS_frm[i], &list_exclude_APRS_frm[i]);

	}

	initTStringList(&list_incoming_mycalls);
//	initTStringList(&list_incoming_mycalls_sock);

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		for (port = 0; port < port_num; port++)
		{
			TAX25Port * AX25Sess = &AX25Port[snd_ch][port];

			AX25Sess->hi_vs = 0;
			AX25Sess->vs = 0;
			AX25Sess->vr = 0;
			AX25Sess->PID = PID_NO_L3;
			initTStringList(&AX25Sess->in_data_buf);
			initString(&AX25Sess->out_data_buf);
			AX25Sess->t1 = 0;
			AX25Sess->t2 = 0;
			AX25Sess->t3 = 0;
			AX25Sess->i_lo = 0;
			AX25Sess->i_hi = 0;
			AX25Sess->n1 = 0;
			AX25Sess->n2 = 0;
			AX25Sess->status = 0;
			AX25Sess->clk_frack = 0;
			initTStringList(&AX25Sess->frame_buf);
			initTStringList(&AX25Sess->I_frame_buf);
			initTStringList(&AX25Sess->frm_collector);
			AX25Sess->corrcall[0] = 0;
			AX25Sess->mycall[0] = 0;
			AX25Sess->digi[0] = 0;
			AX25Sess->Path[0] = 0;
			AX25Sess->kind[0] = 0;
			AX25Sess->socket = NULL;
			ax25_info_init(AX25Sess);
			clr_frm_win(AX25Sess);
		}
	}
}
/*

procedure ax25_free;
var
  snd_ch,port,i: byte;
begin
  for snd_ch:=1 to 4 do
  for port:=0 to port_num-1 do
  begin
	AX25Sess->in_data_buf.Free;
	AX25Sess->frame_buf.Free;
	AX25Sess->I_frame_buf.Free;
	AX25Sess->frm_collector.Free;
  end;
  for i:=1 to 4 do
  begin
	all_frame_buf[i].Free;
	list_exclude_callsigns[i].Free;
	list_exclude_APRS_frm[i].Free;
	list_digi_callsigns[i].Free;
  end;
  list_incoming_mycalls.Free;
  list_incoming_mycalls_sock.Free;
end;
*/
void  write_ax25_info(TAX25Port * AX25Sess)
{}

/*var
  new: boolean;
  t: text;
  s: string;
  time_ses: tdatetime;
  time_ses_sec: extended;
  call,mycall,spkt,sbyte,rpkt,rbyte,rfc,tcps,rcps,acps,startses,timeses: string;
begin
  if stat_log then
  begin
	time_ses:=AX25Sess->info.stat_end_ses-AX25Sess->info.stat_begin_ses;
	time_ses_sec:=time_ses*86400; //время сессии в секундах
	if time_ses_sec<1 then exit;
	mycall:=copy(AX25Sess->mycall+'         ',1,9);
	call:=copy(AX25Sess->corrcall+'         ',1,9);
	spkt:=copy(inttostr(AX25Sess->info.stat_s_pkt)+'         ',1,6);
	sbyte:=copy(inttostr(AX25Sess->info.stat_s_byte)+'         ',1,9);
	rpkt:=copy(inttostr(AX25Sess->info.stat_r_pkt)+'         ',1,6);
	rbyte:=copy(inttostr(AX25Sess->info.stat_r_byte)+'         ',1,9);
	rfc:=copy(inttostr(AX25Sess->info.stat_r_fc)+'         ',1,6);
	tcps:=copy(inttostr(round(AX25Sess->info.stat_s_byte/time_ses_sec))+'         ',1,5);
	rcps:=copy(inttostr(round(AX25Sess->info.stat_r_byte/time_ses_sec))+'         ',1,5);
	acps:=copy(inttostr(round(AX25Sess->info.stat_s_byte/time_ses_sec+AX25Sess->info.stat_r_byte/time_ses_sec))+'         ',1,5);
	timeses:=FormatDateTime('hh:mm:ss',time_ses);
	startses:=FormatDateTime('dd-mm-yy hh:mm:ss',AX25Sess->info.stat_begin_ses);
	s:=mycall+' '+call+' '+spkt+' '+sbyte+' '+rpkt+' '+rbyte+' '+rfc+' '+tcps+' '+rcps+' '+acps+' '+startses+' '+timeses;
	assignfile(t,'log.txt');
	if FileSearch('log.txt','')='' then new:=TRUE else new:=FALSE;
	if new then
	begin
	  rewrite(t);
	  writeln(t,'Mycall    CorrCall  TXPkt  TXByte    RXPkt  RXByte    FCPkt  TXCPS RXCPS T.CPS Begin session     SesTime');
	  writeln(t,'--------  --------- ------ --------- ------ --------- ------ ----- ----- ----- ----------------- --------');
	end
	else append(t);
	if (AX25Sess->info.stat_s_byte>0) or (AX25Sess->info.stat_r_byte>0) then writeln(t,s);
	closefile(t);
  end;
end;

end.
*/


/*
Copyright 2001-2018 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
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

#define	PFBIT 0x10		// POLL/FINAL BIT IN CONTROL BYTE

#define	NETROM_PID 0xCF
#define	IP_PID 0xCC
#define	ARP_PID 0xCD

#define	NODES_SIG	0xFF

/*

DllExport int APIENTRY SetTraceOptions(int mask, int mtxparam, int mcomparam)
{

	//	Sets the tracing options for DecodeFrame. Mask is a bit
	//	mask of ports to monitor (ie 101 binary will monitor ports
	//	1 and 3). MTX enables monitoring on transmitted frames. MCOM
	//	enables monitoring of protocol control frames (eg SABM, UA, RR),
	//	as well as info frames.

	MMASK = mask;
	MTX = mtxparam;
	MCOM = mcomparam;

	return (0);
}

DllExport int APIENTRY SetTraceOptionsEx(int mask, int mtxparam, int mcomparam, int monUIOnly)
{

	//	Sets the tracing options for DecodeFrame. Mask is a bit
	//	mask of ports to monitor (ie 101 binary will monitor ports
	//	1 and 3). MTX enables monitoring on transmitted frames. MCOM
	//	enables monitoring of protocol control frames (eg SABM, UA, RR),
	//	as well as info frames.


	MMASK = mask;
	MTX = mtxparam;
	MCOM = mcomparam;
	MUIONLY = monUIOnly;

	return 0;
}

*/

#define USHORT unsigned short

UCHAR	MCOM = 1;
UCHAR	MTX = 1;
ULONG	MMASK = 0xF;
UCHAR	MUIONLY = 0;

#define	SREJ 0x0D
#define SABME 0x6F
#define XID 0xAF
#define TEST 0xE3

#define L4BUSY	0x80		// BNA - DONT SEND ANY MORE
#define L4NAK	0x40		// NEGATIVE RESPONSE FLAG
#define L4MORE	0x20		// MORE DATA FOLLOWS - FRAGMENTATION FLAG

#define L4CREQ	1		// CONNECT REQUEST
#define L4CACK	2		// CONNECT ACK
#define L4DREQ	3		// DISCONNECT REQUEST
#define L4DACK	4		// DISCONNECT ACK
#define L4INFO	5		// INFORMATION
#define L4IACK	6		// INFORMATION ACK

#pragma pack(1)

struct myin_addr {
	union {
		struct { unsigned char s_b1, s_b2, s_b3, s_b4; } S_un_b;
		struct { unsigned short s_w1, s_w2; } S_un_w;
		unsigned long addr;
	};
};


typedef struct _IPMSG
{
	//       FORMAT OF IP HEADER
	//
	//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	UCHAR	VERLEN;          // 4 BITS VERSION, 4 BITS LENGTH
	UCHAR	TOS;             // TYPE OF SERVICE
	USHORT	IPLENGTH;        // DATAGRAM LENGTH
	USHORT	IPID;            // IDENTIFICATION
	USHORT	FRAGWORD;        // 3 BITS FLAGS, 13 BITS OFFSET
	UCHAR	IPTTL;
	UCHAR	IPPROTOCOL;      // HIGHER LEVEL PROTOCOL
	USHORT	IPCHECKSUM;      // HEADER CHECKSUM
	struct myin_addr IPSOURCE;
	struct myin_addr IPDEST;

	UCHAR	Data;

} IPMSG, *PIPMSG;


typedef struct _TCPMSG
{

	//	FORMAT OF TCP HEADER WITHIN AN IP DATAGRAM

	//	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	USHORT	SOURCEPORT;
	USHORT	DESTPORT;

	ULONG	SEQNUM;
	ULONG	ACKNUM;

	UCHAR	TCPCONTROL;			// 4 BITS DATA OFFSET 4 RESERVED
	UCHAR	TCPFLAGS;			// (2 RESERVED) URG ACK PSH RST SYN FIN

	USHORT	WINDOW;
	USHORT	CHECKSUM;
	USHORT	URGPTR;


} TCPMSG, *PTCPMSG;

typedef struct _UDPMSG
{

	//	FORMAT OF UDP HEADER WITHIN AN IP DATAGRAM

	//	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	USHORT	SOURCEPORT;
	USHORT	DESTPORT;
	USHORT	LENGTH;
	USHORT	CHECKSUM;
	UCHAR	UDPData[0];

} UDPMSG, *PUDPMSG;

//		ICMP MESSAGE STRUCTURE

typedef struct _ICMPMSG
{
	//	FORMAT OF ICMP HEADER WITHIN AN IP DATAGRAM

	//	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	UCHAR	ICMPTYPE;
	UCHAR	ICMPCODE;
	USHORT	ICMPCHECKSUM;

	USHORT	ICMPID;
	USHORT	ICMPSEQUENCE;
	UCHAR	ICMPData[0];

} ICMPMSG, *PICMPMSG;


typedef struct _L3MESSAGE
{
	//
	//	NETROM LEVEL 3 MESSAGE - WITHOUT L2 INFO 
	//
	UCHAR	L3SRCE[7];			// ORIGIN NODE
	UCHAR	L3DEST[7];			// DEST NODE
	UCHAR	L3TTL;				// TX MONITOR FIELD - TO PREVENT MESSAGE GOING								// ROUND THE NETWORK FOR EVER DUE TO ROUTING LOOP
//
//	NETROM LEVEL 4 DATA
//
	UCHAR	L4INDEX;			// TRANSPORT SESSION INDEX
	UCHAR	L4ID;				// TRANSPORT SESSION ID
	UCHAR	L4TXNO;				// TRANSMIT SEQUENCE NUMBER
	UCHAR	L4RXNO;				// RECEIVE (ACK) SEQ NUMBER
	UCHAR	L4FLAGS;			// FRAGMENTATION, ACK/NAK, FLOW CONTROL AND MSG TYPE BITS

	UCHAR	L4DATA[236];		//DATA

} L3MESSAGE, *PL3MESSAGE;


typedef struct _MESSAGE
{
	//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGE * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

	//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;

	union
	{                   /*  array named screen */
		UCHAR L2DATA[256];
		struct _L3MESSAGE L3MSG;
	};


}MESSAGE, *PMESSAGE;

#pragma pack()

char * strlop(char * buf, char delim);
UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, unsigned int msglen);
char * DISPLAY_NETROM(MESSAGE * ADJBUFFER, UCHAR * Output, int MsgLen);
UCHAR * DISPLAYIPDATAGRAM(IPMSG * IP, UCHAR * Output, int MsgLen);
char * DISPLAYARPDATAGRAM(UCHAR * Datagram, UCHAR * Output);

int CountBits(unsigned long in)
{
	int n = 0;
	while (in)
	{
		if (in & 1) n++;
		in >>= 1;
	}
	return n;
}

BOOL ConvToAX25(char * callsign, unsigned char * ax25call)
{
	int i;

	memset(ax25call, 0x40, 6);		// in case short
	ax25call[6] = 0x60;				// default SSID

	for (i = 0; i < 7; i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			i = atoi(&callsign[i + 1]);

			if (i < 16)
			{
				ax25call[6] |= i << 1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ' || callsign[i] == ',')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}

		ax25call[i] = callsign[i] << 1;
	}

	//
	//	Too many chars
	//

	return (FALSE);
}


int ConvFromAX25(unsigned char * incall, char * outcall)
{
	int in, out = 0;
	unsigned char chr;

	memset(outcall, 0x20, 10);

	for (in = 0; in < 6; in++)
	{
		chr = incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++] = chr;
	}

	chr = incall[6];				// ssid

	if (chr == 0x42)
	{
		outcall[out++] = '-';
		outcall[out++] = 'T';
		return out;
	}

	if (chr == 0x44)
	{
		outcall[out++] = '-';
		outcall[out++] = 'R';
		return out;
	}

	chr >>= 1;
	chr &= 15;

	if (chr > 0)
	{
		outcall[out++] = '-';
		if (chr > 9)
		{
			chr -= 10;
			outcall[out++] = '1';
		}
		chr += 48;
		outcall[out++] = chr;
	}
	return (out);
}


int IntDecodeFrame(MESSAGE * msg, char * buffer, time_t Stamp, UINT Mask, BOOL APRS, BOOL MCTL);

int APRSDecodeFrame(MESSAGE * msg, char * buffer, time_t Stamp, UINT Mask)
{
	return IntDecodeFrame(msg, buffer, Stamp, Mask, TRUE, FALSE);
}

int myDecodeFrame(MESSAGE * msg, char * buffer, int Stamp)
{
	return IntDecodeFrame(msg, buffer, Stamp, MMASK, FALSE, FALSE);
}

int IntDecodeFrame(MESSAGE * msg, char * buffer, time_t Stamp, UINT Mask, BOOL APRS, BOOL MINI)
{
	UCHAR * ptr;
	int n;
	MESSAGE * ADJBUFFER;
	ptrdiff_t Work;
	UCHAR CTL;
	BOOL PF = 0;
	char CRCHAR[3] = "  ";
	char PFCHAR[3] = "  ";
	int Port;
	int MSGFLAG = 0;		//CR and V1 flags
	char * Output = buffer;
	int HH, MM, SS;
	char TR = 'R';
	char From[10], To[10];
	BOOL Info = 0;
	BOOL FRMRFLAG = 0;
	BOOL XIDFLAG = 0;
	BOOL TESTFLAG = 0;

	size_t MsgLen = msg->LENGTH;

	// MINI mode is for Node Listen (remote monitor) Mode. Keep info to minimum
/*
KO6IZ*>K7TMG-1:
/ex
KO6IZ*>K7TMG-1:
b
KO6IZ*>K7TMG-1 (UA)
W0TX*>KC6OAR>KB9KC>ID:
W0TX/R DRC/D W0TX-2/G W0TX-1/B W0TX-7/N
KC6OAR*>ID:
*/

//	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED

	n = 8;						// MAX DIGIS
	ptr = &msg->ORIGIN[6];	// End of Address bit

	while ((*ptr & 1) == 0)
	{
		//	MORE TO COME

		ptr += 7;
		n--;

		if (n == 0)
		{
			return 0;						// Corrupt - no end of address bit
		}
	}

	// Reached End of digis

	Work = ptr - &msg->ORIGIN[6];			// Work is length of digis

	MsgLen -= Work;

	ADJBUFFER = (MESSAGE *)((UCHAR *)msg + Work);			// ADJBUFFER points to CTL, etc. allowing for digis

	CTL = ADJBUFFER->CTL;

	if (CTL & PFBIT)
		PF = TRUE;

	CTL &= ~PFBIT;

	if (MUIONLY)
		if (CTL != 3)
			return 0;

	if ((CTL & 1) == 0 || CTL == FRMR || CTL == 3)
	{
	}
	else
	{
		if (((CTL & 2) && MINI) == 0)		// Want Control (but not super unless MCOM
			if (MCOM == 0)
				return 0;						// Dont do control
	}


	Port = msg->PORT;

	if (Port & 0x80)
	{
		if (MTX == 0)
			return 0;							//	TRANSMITTED FRAME - SEE IF MTX ON

		TR = 'T';
	}

	Port &= 0x7F;

	if (((1 << (Port - 1)) & Mask) == 0)		// Check MMASK
		return 0;


	Stamp = Stamp % 86400;		// Secs
	HH = (int)(Stamp / 3600);

	Stamp -= HH * 3600;
	MM = (int)(Stamp / 60);

	SS = (int)(Stamp - MM * 60);

	// Add Port: if MINI mode and monitoring more than one port

	if (MINI == 0)
		Output += sprintf((char *)Output, "%02d:%02d:%02d%c ", HH, MM, SS, TR);
	else
		if (CountBits(Mask) > 1)
			Output += sprintf((char *)Output, "%d:", Port);

	From[ConvFromAX25(msg->ORIGIN, From)] = 0;
	To[ConvFromAX25(msg->DEST, To)] = 0;

	Output += sprintf((char *)Output, "%s>%s", From, To);

	//	Display any Digi-Peaters   

	n = 8;					// Max number of digi-peaters
	ptr = &msg->ORIGIN[6];	// End of Address bit

	while ((*ptr & 1) == 0)
	{
		//	MORE TO COME

		From[ConvFromAX25(ptr + 1, From)] = 0;
		Output += sprintf((char *)Output, ",%s", From);

		ptr += 7;
		n--;

		if (n == 0)
			break;

		// See if digi actioned - put a * on last actioned

		if (*ptr & 0x80)
		{
			if (*ptr & 1)						// if last address, must need *
				*(Output++) = '*';
			else
				if ((ptr[7] & 0x80) == 0)		// Repeased by next?
					*(Output++) = '*';			// No, so need *
		}
	}

	if (MINI == 0)
		Output += sprintf((char *)Output, " Port=%d ", Port);

	// Set up CR and PF

	CRCHAR[0] = 0;
	PFCHAR[0] = 0;

	if (msg->DEST[6] & 0x80)
	{
		if (msg->ORIGIN[6] & 0x80)			//	Both set, assume V1
			MSGFLAG |= VER1;
		else
		{
			MSGFLAG |= CMDBIT;
			CRCHAR[0] = ' ';
			CRCHAR[1] = 'C';
			if (PF)							// If FP set
			{
				PFCHAR[0] = ' ';
				PFCHAR[1] = 'P';
			}
		}
	}
	else
	{
		if (msg->ORIGIN[6] & 0x80)			//	Only Origin Set
		{
			MSGFLAG |= RESP;
			CRCHAR[0] = ' ';
			CRCHAR[1] = 'R';
			if (PF)							// If FP set
			{
				PFCHAR[0] = ' ';
				PFCHAR[1] = 'F';
			}
		}
		else
			MSGFLAG |= VER1;				// Neither, assume V1
	}

	if ((CTL & 1) == 0)						// I frame
	{
		int NS = (CTL >> 1) & 7;			// ISOLATE RECEIVED N(S)
		int NR = (CTL >> 5) & 7;

		Info = 1;

		if (MINI == 0)
			Output += sprintf((char *)Output, "<I%s%s S%d R%d>", CRCHAR, PFCHAR, NS, NR);
	}
	else if (CTL == 3)
	{
		//	Un-numbered Information Frame 

		Output += sprintf((char *)Output, "<UI%s>", CRCHAR);
		Info = 1;
	}
	else if (CTL & 2)
	{
		// UN Numbered

		char SUP[6] = "??";

		switch (CTL)
		{
		case SABM:

			strcpy(SUP, "C");
			break;

		case SABME:

			strcpy(SUP, "SABME");
			break;

		case XID:

			strcpy(SUP, "XID");
			XIDFLAG = 1;
			break;

		case TEST:

			strcpy(SUP, "TEST");
			TESTFLAG = 1;
			break;

		case DISC:

			strcpy(SUP, "D");
			break;

		case DM:

			strcpy(SUP, "DM");
			break;

		case UA:

			strcpy(SUP, "UA");
			break;


		case FRMR:

			strcpy(SUP, "FRMR");
			FRMRFLAG = 1;
			break;
		}

		if (MINI)
			Output += sprintf((char *)Output, "<%s>", SUP);
		else
			Output += sprintf((char *)Output, "<%s%s%s>", SUP, CRCHAR, PFCHAR);
	}
	else
	{
		// Super

		int NR = (CTL >> 5) & 7;
		char SUP[5] = "??";

		switch (CTL & 0x0F)
		{
		case RR:

			strcpy(SUP, "RR");
			break;

		case RNR:

			strcpy(SUP, "RNR");
			break;

		case REJ:

			strcpy(SUP, "REJ");
			break;
		case SREJ:

			strcpy(SUP, "SREJ");
			break;
		}

		Output += sprintf((char *)Output, "<%s%s%s R%d>", SUP, CRCHAR, PFCHAR, NR);

	}

	if (FRMRFLAG)
		Output += sprintf((char *)Output, " %02X %02X %02X", ADJBUFFER->PID, ADJBUFFER->L2DATA[0], ADJBUFFER->L2DATA[1]);

	if (XIDFLAG)
	{
		// Decode and display XID

		UCHAR * ptr = &ADJBUFFER->PID;

		if (*ptr++ == 0x82 && *ptr++ == 0x80)
		{
			int Type;
			int Len;
			unsigned int value;
			int xidlen = *(ptr++) << 8;
			xidlen += *ptr++;

			// XID is set of Type, Len, Value n-tuples

// G8BPQ-2>G8BPQ:(XID cmd, p=1) Half-Duplex SREJ modulo-128 I-Field-Length-Rx=256 Window-Size-Rx=32 Ack-Timer=3000 Retries=10


			while (xidlen > 0)
			{
				Type = *ptr++;
				Len = *ptr++;

				value = 0;
				xidlen -= (Len + 2);

				while (Len--)
				{
					value <<= 8;
					value += *ptr++;
				}
				switch (Type)
				{
				case 2:				//Bin fields
				case 3:

					Output += sprintf((char *)Output, " %d=%x", Type, value);
					break;

				case 6:				//RX Size

					Output += sprintf((char *)Output, " RX Paclen=%d", value / 8);
					break;

				case 8:				//RX Window

					Output += sprintf((char *)Output, " RX Window=%d", value);
					break;
				}
			}
		}
	}
	if (Info)
	{
		// We have an info frame

		switch (ADJBUFFER->PID)
		{
		case 0xF0:		// Normal Data
		{
			char Infofield[257];
			char * ptr1 = Infofield;
			char * ptr2 = ADJBUFFER->L2DATA;
			UCHAR C;
			size_t len;

			MsgLen = MsgLen - (19 + sizeof(void *));

			if (MsgLen < 0 || MsgLen > 257)
				return 0;				// Duff

			while (MsgLen--)
			{
				C = *(ptr2++);

				if (APRS)
					*(ptr1++) = C;		// MIC-E needs all chars
				else
				{
					// Convert to printable

					C &= 0x7F;

					if (C == 13 || C == 10 || C > 31)
						*(ptr1++) = C;
				}
			}

			len = ptr1 - Infofield;

			Output[0] = ':';
			Output[1] = 13;
			memcpy(&Output[2], Infofield, len);
			Output += (len + 2);

			break;
		}
		case NETROM_PID:

			Output = DISPLAY_NETROM(ADJBUFFER, Output, (int)MsgLen);
			break;

		case IP_PID:

			Output += sprintf((char *)Output, " <IP>\r");
			Output = DISPLAYIPDATAGRAM((IPMSG *)&ADJBUFFER->L2DATA[0], Output, (int)MsgLen);
			break;

		case ARP_PID:

			Output = DISPLAYARPDATAGRAM(&ADJBUFFER->L2DATA[0], Output);
			break;

		case 8:					// Fragmented IP

			n = ADJBUFFER->L2DATA[0];	// Frag Count

			Output += sprintf((char *)Output, "<Fragmented IP %02x>\r", n);

			if (ADJBUFFER->L2DATA[0] & 0x80)	// First Frag - Display Header
			{
				Output = DISPLAYIPDATAGRAM((IPMSG *)&ADJBUFFER->L2DATA[2], Output, (int)MsgLen - 1);
			}

			break;
		}
	}

	if (Output[-1] != 13)
		Output += sprintf((char *)Output, "\r");

	return (int)(Output - buffer);

}
//      Display NET/ROM data                                                 

char * DISPLAY_NETROM(MESSAGE * ADJBUFFER, UCHAR * Output, int MsgLen)
{
	char Alias[7] = "";
	char Dest[10];
	char Node[10];
	UCHAR TTL, Index, ID, TXNO, RXNO, OpCode, Flags, Window;
	UCHAR * ptr = &ADJBUFFER->L2DATA[0];

	if (ADJBUFFER->L2DATA[0] == NODES_SIG)
	{
		// Display NODES


		// If an INP3 RIF (type <> UI) decode as such

		if (ADJBUFFER->CTL != 3)		// UI
			return DisplayINP3RIF(&ADJBUFFER->L2DATA[1], Output, MsgLen - 24);

		memcpy(Alias, ++ptr, 6);

		ptr += 6;

		Output += sprintf((char *)Output, " NODES broadcast from %s\r", Alias);

		MsgLen -= 30;					//Header, mnemonic and signature length

		while (MsgLen > 20)				// Entries are 21 bytes
		{
			Dest[ConvFromAX25(ptr, Dest)] = 0;
			ptr += 7;
			memcpy(Alias, ptr, 6);
			ptr += 6;
			strlop(Alias, ' ');
			Node[ConvFromAX25(ptr, Node)] = 0;
			ptr += 7;

			Output += sprintf((char *)Output, "  %s:%s via %s qlty=%d\r", Alias, Dest, Node, ptr[0]);
			ptr++;
			MsgLen -= 21;
		}
		return Output;
	}

	//	Display normal NET/ROM transmissions 

	Output += sprintf((char *)Output, " NET/ROM\r  ");

	Dest[ConvFromAX25(ptr, Dest)] = 0;
	ptr += 7;
	Node[ConvFromAX25(ptr, Node)] = 0;
	ptr += 7;

	TTL = *(ptr++);
	Index = *(ptr++);
	ID = *(ptr++);
	TXNO = *(ptr++);
	RXNO = *(ptr++);
	OpCode = Flags = *(ptr++);

	OpCode &= 15;				// Remove Flags

	Output += sprintf((char *)Output, "%s to %s ttl %d cct=%02X%02X ", Dest, Node, TTL, Index, ID);
	MsgLen -= 20;

	switch (OpCode)
	{
	case L4CREQ:

		Window = *(ptr++);
		Dest[ConvFromAX25(ptr, Dest)] = 0;
		ptr += 7;
		Node[ConvFromAX25(ptr, Node)] = 0;
		ptr += 7;

		Output += sprintf((char *)Output, "<CON REQ> w=%d %s at %s", Window, Dest, Node);

		if (MsgLen > 38)				// BPQ Extended Params
		{
			short Timeout = (short)*ptr;
			Output += sprintf((char *)Output, " t/o %d", Timeout);
		}

		return Output;

	case L4CACK:

		if (Flags & L4BUSY)				// BUSY RETURNED
			return Output + sprintf((char *)Output, " <CON NAK> - BUSY");

		return Output + sprintf((char *)Output, " <CON ACK> w=%d my cct=%02X%02X", ptr[1], TXNO, RXNO);

	case L4DREQ:

		return Output + sprintf((char *)Output, " <DISC REQ>");

	case L4DACK:

		return Output + sprintf((char *)Output, " <DISC ACK>");

	case L4INFO:
	{
		char Infofield[257];
		char * ptr1 = Infofield;
		UCHAR C;
		size_t len;

		Output += sprintf((char *)Output, " <INFO S%d R%d>", TXNO, RXNO);

		if (Flags & L4BUSY)
			*(Output++) = 'B';

		if (Flags & L4NAK)
			*(Output++) = 'N';

		if (Flags & L4MORE)
			*(Output++) = 'M';

		MsgLen = MsgLen - (19 + sizeof(void *));

		if (MsgLen < 0 || MsgLen > 257)
			return Output;				// Duff

		while (MsgLen--)
		{
			C = *(ptr++);

			// Convert to printable

			C &= 0x7F;

			if (C == 13 || C == 10 || C > 31)
				*(ptr1++) = C;
		}

		len = ptr1 - Infofield;

		Output[0] = ':';
		Output[1] = 13;
		memcpy(&Output[2], Infofield, len);
		Output += (len + 2);
	}

	return Output;

	case L4IACK:

		Output += sprintf((char *)Output, " <INFO ACK R%d> ", RXNO);

		if (Flags & L4BUSY)
			*(Output++) = 'B';

		if (Flags & L4NAK)
			*(Output++) = 'N';

		if (Flags & L4MORE)
			*(Output++) = 'M';

		return Output;


	case 0:

		//	OPcode zero is used for several things

		if (Index == 0x0c && ID == 0x0c)	// IP	
		{
			*(Output++) = 13;
			*(Output++) = ' ';
			Output = DISPLAYIPDATAGRAM((IPMSG *)ptr, Output, MsgLen);
			return Output;
		}

		if (Index == 0 && ID == 1)			// NRR	
		{
			Output += sprintf((char *)Output, " <Record Route>\r");

			MsgLen -= 23;

			while (MsgLen > 6)
			{
				Dest[ConvFromAX25(ptr, Dest)] = 0;

				if (ptr[7] & 0x80)
					Output += sprintf((char *)Output, "%s* ", Dest);
				else
					Output += sprintf((char *)Output, "%s ", Dest);

				ptr += 8;
				MsgLen -= 8;
			}

			return Output;
		}
	}

	Output += sprintf((char *)Output, " <???\?>");
	return Output;
}

/*

	PUBLIC	L3IP
L3IP:
;
;	TCP/IP DATAGRAM
;
	mov	EBX,OFFSET IP_MSG
	call	NORMSTR
;
	INC	ESI			; NOW POINTING TO IP HEADER

*/
UCHAR * DISPLAYIPDATAGRAM(IPMSG * IP, UCHAR * Output, int MsgLen)
{
	UCHAR * ptr;
	USHORT FRAGWORD;

	ptr = (UCHAR *)&IP->IPSOURCE;
	Output += sprintf((char *)Output, "%d.%d.%d.%d>", ptr[0], ptr[1], ptr[2], ptr[3]);

	ptr = (UCHAR *)&IP->IPDEST;
	Output += sprintf((char *)Output, "%d.%d.%d.%d LEN:%d ", ptr[0], ptr[1], ptr[2], ptr[3], htons(IP->IPLENGTH));

	FRAGWORD = ntohs(IP->FRAGWORD);

	if (FRAGWORD)
	{
		// If nonzero, check which bits are set 

		//Bit 0: reserved, must be zero
		//Bit 1: (DF) 0 = May Fragment,  1 = Don't Fragment.
		//Bit 2: (MF) 0 = Last Fragment, 1 = More Fragments.
		//Fragment Offset:  13 bits

		if (FRAGWORD & (1 << 14))
			Output += sprintf((char *)Output, "DF ");

		if (FRAGWORD & (1 << 13))
			Output += sprintf((char *)Output, "MF ");

		FRAGWORD &= 0xfff;

		if (FRAGWORD)
		{
			Output += sprintf((char *)Output, "Offset %d ", FRAGWORD * 8);
			return Output;			// Cant display proto fields
		}
	}

	if (IP->IPPROTOCOL == 6)
	{
		PTCPMSG TCP = (PTCPMSG)&IP->Data;

		Output += sprintf((char *)Output, "TCP Src %d Dest %d ", ntohs(TCP->SOURCEPORT), ntohs(TCP->DESTPORT));
		return Output;
	}

	if (IP->IPPROTOCOL == 1)
	{
		PICMPMSG ICMPptr = (PICMPMSG)&IP->Data;

		Output += sprintf((char *)Output, "ICMP ");

		if (ICMPptr->ICMPTYPE == 8)
			Output += sprintf((char *)Output, "Echo Request ");
		else
			if (ICMPptr->ICMPTYPE == 0)
				Output += sprintf((char *)Output, "Echo Reply ");
			else
				Output += sprintf((char *)Output, "Code %d", ICMPptr->ICMPTYPE);

		return Output;
	}

	/*
		MOV	AL,IPPROTOCOL[ESI]
		CMP AL,6
		JNE @F

		MOV EBX, OFFSET TCP
		CALL NORMSTR
		JMP ADD_CR
	@@:

		CMP AL,1
		JNE @F

		MOV EBX, OFFSET ICMP
		CALL NORMSTR
		JMP ADD_CR
	@@:

		CALL	DISPLAY_BYTE_1		; DISPLAY PROTOCOL TYPE

	;	mov	AL,CR
	;	call	PUTCHAR
	;
	;	MOV	ECX,39			; TESTING
	;IPLOOP:
	;	lodsb
	;	CALL	BYTE_TO_HEX
	;
	;	LOOP	IPLOOP

		JMP	ADD_CR


	*/
	return Output;
}

char * DISPLAYARPDATAGRAM(UCHAR * Datagram, UCHAR * Output)
{
	UCHAR * ptr = Datagram;
	char Dest[10];

	if (ptr[7] == 1)		// Request
		return Output + sprintf((char *)Output, " ARP Request who has %d.%d.%d.%d? Tell %d.%d.%d.%d",
			ptr[26], ptr[27], ptr[28], ptr[29], ptr[15], ptr[16], ptr[17], ptr[18]);

	// Response

	Dest[ConvFromAX25(&ptr[8], Dest)] = 0;

	return Output + sprintf((char *)Output, " ARP Reply %d.%d.%d.%d is at %s Tell %d.%d.%d.%d",
		ptr[15], ptr[16], ptr[17], ptr[18], Dest, ptr[26], ptr[27], ptr[28], ptr[29]);

}


UCHAR * DisplayINP3RIF(UCHAR * ptr1, UCHAR * ptr2, unsigned int msglen)
{
	char call[10];
	int calllen;
	int hops;
	unsigned short rtt;
	unsigned int len;
	unsigned int opcode;
	char alias[10] = "";
	UCHAR IP[6];
	int i;

	ptr2 += sprintf(ptr2, " INP3 RIF:\r Alias  Call  Hops  RTT\r");

	while (msglen > 0)
	{
		calllen = ConvFromAX25(ptr1, call);
		call[calllen] = 0;

		// Validate the call

		for (i = 0; i < calllen; i++)
		{
			if (!isupper(call[i]) && !isdigit(call[i]) && call[i] != '-')
			{
				ptr2 += sprintf(ptr2, " Corrupt RIF\r");
				return ptr2;
			}
		}

		ptr1 += 7;

		hops = *ptr1++;
		rtt = (*ptr1++ << 8);
		rtt += *ptr1++;

		IP[0] = 0;
		strcpy(alias, "      ");

		msglen -= 10;

		while (*ptr1 && msglen > 0)
		{
			len = *ptr1;
			opcode = *(ptr1 + 1);

			if (len < 2 || len > msglen)
			{
				ptr2 += sprintf(ptr2, " Corrupt RIF\r");
				return ptr2;
			}
			if (opcode == 0 && len < 9)
			{
				memcpy(&alias[6 - (len - 2)], ptr1 + 2, len - 2);		// Right Justiify
			}
			else
				if (opcode == 1 && len < 8)
				{
					memcpy(IP, ptr1 + 2, len - 2);
				}

			ptr1 += len;
			msglen -= len;
		}

		if (IP[0])
			ptr2 += sprintf(ptr2, " %s:%s %d %4.2d %d.%d.%d.%d\r", alias, call, hops, rtt, IP[0], IP[1], IP[2], IP[3]);
		else
			ptr2 += sprintf(ptr2, " %s:%s %d %4.2d\r", alias, call, hops, rtt);

		ptr1++;
		msglen--;		// EOP
	}

	return ptr2;
}


