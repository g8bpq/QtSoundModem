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

char modes_name[modes_count][20] = 
{
	"AFSK AX.25 300bd","AFSK AX.25 1200bd","AFSK AX.25 600bd","AFSK AX.25 2400bd",
	"BPSK AX.25 1200bd","BPSK AX.25 600bd","BPSK AX.25 300bd","BPSK AX.25 2400bd",
	"QPSK AX.25 4800bd","QPSK AX.25 3600bd","QPSK AX.25 2400bd","BPSK FEC 4x100bd",
	"DW QPSK V26A 2400bd","DW 8PSK V27 4800bd","DW QPSK V26B 2400bd", "ARDOP Packet"
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

BOOL multiCore = FALSE;

/*
type
  TComboBox  =  class(StdCtrls.TComboBox)
  private
    procedure CMMouseWheel(var msg TCMMouseWheel); message CM_MOUSEWHEEL;
  end;
  TData16  =  array [0..4095] of smallint;
  PData16  =  ^TData16;
  TWaveHeader  =  record
    RIFF        dword;
    ChunkLen    integer;
    WAVE        dword;
    fmt         dword;
    FormatLen   integer;
    Format      word;
    Channels    word;
    Frequency   integer;
    BytesPS     integer;
    BlockAlign  word;
    BitsPS      word;
    data        dword;
    DataLen     integer
  end;
  TForm1  =  class(TForm)
    Panel5 TPanel;
    ServerSocket1 TServerSocket;
    MainMenu1 TMainMenu;
    Settings1 TMenuItem;
    OutputVolume1 TMenuItem;
    InputVolume1 TMenuItem;
    CoolTrayIcon1 TCoolTrayIcon;
    ImageList1 TImageList;
    ABout1 TMenuItem;
    Panel1 TPanel;
    Panel2 TPanel;
    View1 TMenuItem;
    Firstwaterfall1 TMenuItem;
    Secondwaterfall1 TMenuItem;
    Panel3 TPanel;
    StringGrid1 TStringGrid;
    Devices1 TMenuItem;
    Statustable1 TMenuItem;
    Monitor1 TMenuItem;
    Panel4 TPanel;
    PaintBox2 TPaintBox;
    Filters1 TMenuItem;
    Clearmonitor1 TMenuItem;
    RxRichEdit1 TRxRichEdit;
    MemoPopupMenu1 TPopupMenu;
    Copytext1 TMenuItem;
    Label1 TLabel;
    Label5 TLabel;
    ApplicationEvents1 TApplicationEvents;
    PaintBox1 TPaintBox;
    PaintBox3 TPaintBox;
    ServerSocket2 TServerSocket;
    Font1 TMenuItem;
    FontDialog1 TFontDialog;
    N1 TMenuItem;
    Calibration1 TMenuItem;
    Panel9 TPanel;
    Panel6 TPanel;
    Label4 TLabel;
    Shape2 TShape;
    ComboBox2 TComboBox;
    SpinEdit2 TSpinEdit;
    Panel7 TPanel;
    Label3 TLabel;
    Shape1 TShape;
    ComboBox1 TComboBox;
    SpinEdit1 TSpinEdit;
    Panel8 TPanel;
    Label2 TLabel;
    TrackBar1 TTrackBar;
    CheckBox1 TCheckBox;
    OpenDialog1 TOpenDialog;
    procedure FormCreate(Sender TObject);
    procedure TrackBar1Change(Sender TObject);
    procedure PaintBox1MouseMove(Sender TObject; Shift TShiftState; X,
      Y Integer);
    procedure PaintBox1MouseDown(Sender TObject; Button TMouseButton;
      Shift TShiftState; X, Y Integer);
    procedure ServerSocket1ClientRead(Sender TObject;
      Socket TCustomWinSocket);
    procedure ServerSocket1ClientError(Sender TObject;
      Socket TCustomWinSocket; ErrorEvent TErrorEvent;
      var ErrorCode Integer);
    procedure OutputVolume1Click(Sender TObject);
    procedure InputVolume1Click(Sender TObject);
    procedure ServerSocket1ClientConnect(Sender TObject;
      Socket TCustomWinSocket);
    procedure ServerSocket1ClientDisconnect(Sender TObject;
      Socket TCustomWinSocket);
    procedure CoolTrayIcon1Click(Sender TObject);
    procedure CoolTrayIcon1Cycle(Sender TObject; NextIndex Integer);
    procedure ABout1Click(Sender TObject);
    procedure PaintBox3MouseDown(Sender TObject; Button TMouseButton;
      Shift TShiftState; X, Y Integer);
    procedure PaintBox3MouseMove(Sender TObject; Shift TShiftState; X,
      Y Integer);
    procedure Firstwaterfall1Click(Sender TObject);
    procedure Secondwaterfall1Click(Sender TObject);
    procedure Devices1Click(Sender TObject);
    procedure Statustable1Click(Sender TObject);
    procedure Monitor1Click(Sender TObject);
    procedure FormPaint(Sender TObject);
    procedure Filters1Click(Sender TObject);
    procedure SpinEdit1Change(Sender TObject);
    procedure SpinEdit2Change(Sender TObject);
    procedure Clearmonitor1Click(Sender TObject);
    procedure Copytext1Click(Sender TObject);
    procedure PaintBox3MouseUp(Sender TObject; Button TMouseButton;
      Shift TShiftState; X, Y Integer);
    procedure PaintBox1MouseUp(Sender TObject; Button TMouseButton;
      Shift TShiftState; X, Y Integer);
    procedure ApplicationEvents1Minimize(Sender TObject);
    procedure ApplicationEvents1Restore(Sender TObject);
    procedure ServerSocket2ClientConnect(Sender TObject;
      Socket TCustomWinSocket);
    procedure ServerSocket2ClientDisconnect(Sender TObject;
      Socket TCustomWinSocket);
    procedure ServerSocket2ClientError(Sender TObject;
      Socket TCustomWinSocket; ErrorEvent TErrorEvent;
      var ErrorCode Integer);
    procedure ServerSocket2ClientRead(Sender TObject;
      Socket TCustomWinSocket);
    procedure Font1Click(Sender TObject);
    procedure Calibration1Click(Sender TObject);
    procedure ComboBox1Change(Sender TObject);
    procedure ComboBox1KeyDown(Sender TObject; var Key Word;
      Shift TShiftState);
    procedure ComboBox1KeyPress(Sender TObject; var Key Char);
    procedure ComboBox2Change(Sender TObject);
    procedure FormDestroy(Sender TObject);
  private
    { Private declarations }
    procedure BufferFull(var Msg TMessage); Message MM_WIM_DATA;
    procedure BufferFull1(var Msg TMessage); Message MM_WOM_DONE;
    procedure make_wave_buf(snd_ch byte; buf PChar);
    procedure disp2(snd_ch byte);
    procedure create_timer1;
    procedure free_timer1;
    procedure show_panels;
    procedure show_combobox;
    procedure Timer_Event2;
    procedure waterfall_init;
    procedure waterfall_free;
  public
    { Public declarations }
    function get_idx_by_name(name string) word;
    function frame_monitor(s,code string; tx_stat boolean) string;
    procedure ChangePriority;
    procedure put_frame(snd_ch byte; frame,code string; tx_stat,excluded boolean);
    procedure show_grid;
    procedure RX2TX(snd_ch byte);
    procedure TX2RX(snd_ch byte);
    procedure WriteIni;
    procedure ReadIni;
    procedure init_8P4800(snd_ch byte);
    procedure init_DW2400(snd_ch byte);
    procedure init_AE2400(snd_ch byte);
    procedure init_MP400(snd_ch byte);
    procedure init_Q4800(snd_ch byte);
    procedure init_Q3600(snd_ch byte);
    procedure init_Q2400(snd_ch byte);
    procedure init_P2400(snd_ch byte);
    procedure init_P1200(snd_ch byte);
    procedure init_P600(snd_ch byte);
    procedure init_P300(snd_ch byte);
    procedure init_300(snd_ch byte);
    procedure init_600(snd_ch byte);
    procedure init_1200(snd_ch byte);
    procedure init_2400(snd_ch byte);
    procedure init_speed(snd_ch byte);
    procedure get_filter_values(idx byte; var dbpf,dtxbpf,dbpftap,dlpf,dlpftap word);
    procedure show_mode_panels;
    procedure show_modes;
    procedure show_freq_a;
    procedure show_freq_b;
    procedure ChkSndDevName;
    procedure StartRx;
    procedure StartTx(snd_ch byte);
    procedure StopRx;
    procedure StopTx(snd_ch byte);
    procedure StartAGW;
    procedure StartKISS;
    procedure wf_scale;
    procedure wf_pointer(snd_ch byte);
  end;

var
/*/

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
int fft_size = 2048;
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



int speed[5] = {0,0,0,0};
int panels[6] = {1,1,1,1,1};

float fft_window_arr[2048];
float  fft_s[2048], fft_d[2048];

short fft_buf[5][2048];
UCHAR fft_disp[5][2048];
//  bm array[1..4] of TBitMap;
//  bm1,bm2,bm3 TBitMap;

//  WaveInHandle hWaveIn;
//  WaveOutHandle array[1..4] of hWaveOut;
int RXBufferLength;

short * data1;

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
 
  /*
implementation

{$R *.DFM}

uses ax25_mod, ax25_demod, ax25, ax25_l2, ax25_ptt, ax25_agw, ax25_about, rgb_rad,
  AX25_set, ax25_filter, AX25_modem_set, kiss_mode, ax25_calibration;

procedure TComboBox.CMMouseWheel(var msg TCMMouseWheel);
begin
  if SendMessage(GetFocus, CB_GETDROPPEDSTATE, 0, 0)  =  0 then msg.Result  =  1;
end;

procedure TForm1.ChangePriority;
begin
  case MainPriority of
    0  SetThreadPriority(MainThreadHandle,THREAD_PRIORITY_NORMAL);
    1  SetThreadPriority(MainThreadHandle,THREAD_PRIORITY_ABOVE_NORMAL);
    2  SetThreadPriority(MainThreadHandle,THREAD_PRIORITY_HIGHEST);
    3  SetThreadPriority(MainThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
  end;
end;

procedure TForm1.show_modes;
var
  s string;
begin
  s = MODEM_CAPTION+" - Ver "+MODEM_VERSION+" - ["+modes_name[Speed[1]];
  if dualchan then s = s+" - "+modes_name[Speed[2]];
  form1.Caption = s+"]";
end;

procedure TForm1.show_freq_a;
begin
  SpinEdit1.Value = round(rx_freq[1]);
  SpinEdit1.Refresh;
end;

procedure TForm1.show_freq_b;
begin
  SpinEdit2.Value = round(rx_freq[2]);
  SpinEdit2.Refresh;
end;
*/
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
	case SPEED_AE2400:


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
	}

}


void init_2400(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 1805;
	rx_baudrate[snd_ch] = 2400;

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

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_600(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 450;

	rx_baudrate[snd_ch] = 600;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_300(int snd_ch)
{
	modem_mode[snd_ch] = MODE_FSK;
	rx_shift[snd_ch] = 200;
	rx_baudrate[snd_ch] = 300;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_MP400(int snd_ch)
{
	modem_mode[snd_ch] = MODE_MPSK;
	rx_shift[snd_ch] = 175 /*sbc*/ * 3;
	rx_baudrate[snd_ch] = 100;

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

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_AE2400(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_V26;
	modem_mode[snd_ch] = MODE_PI4QPSK;

	if (stdtones)
		rx_freq[snd_ch] = 1800;

	rx_shift[snd_ch] = 1200;
	rx_baudrate[snd_ch] = 1200;

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

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q4800(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_SM;
	modem_mode[snd_ch] = MODE_QPSK;
	rx_shift[snd_ch] = 2400;
	rx_baudrate[snd_ch] = 2400;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q3600(int snd_ch)
{
	qpsk_set[snd_ch].mode = QPSK_SM;
	modem_mode[snd_ch] = MODE_QPSK;
	rx_shift[snd_ch] = 1800;
	rx_baudrate[snd_ch] = 1800;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_Q2400(int snd_ch)
{
  qpsk_set[snd_ch].mode = QPSK_SM;
  modem_mode[snd_ch] = MODE_QPSK;
  rx_shift[snd_ch] = 1200;
  rx_baudrate[snd_ch] = 1200;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P2400(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 2400;
  rx_baudrate[snd_ch] = 2400;
 	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P1200(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 1200;
  rx_baudrate[snd_ch] = 1200;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P600(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 600;
  rx_baudrate[snd_ch] = 600;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_P300(int snd_ch)
{
  modem_mode[snd_ch] = MODE_BPSK;
  rx_shift[snd_ch] = 300;
  rx_baudrate[snd_ch] = 300;
	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}

void init_ARDOP(int snd_ch)
{
	modem_mode[snd_ch] = MODE_ARDOP;
	rx_shift[snd_ch] = 500;
	rx_freq[snd_ch] = 1500;
	rx_baudrate[snd_ch] = 500;

	if (modem_def[snd_ch])
		get_filter_values(snd_ch);
}


void init_speed(int snd_ch);

void set_speed(int snd_ch, int Modem)
{
	speed[snd_ch] = Modem;

	init_speed(snd_ch);

}

void init_speed(int snd_ch)
{
	int low, high;

	/*

  if (BPF[snd_ch]>round(rx_samplerate/2) then BPF[snd_ch] = round(rx_samplerate/2);
  if TXBPF[snd_ch]>round(rx_samplerate/2) then TXBPF[snd_ch] = round(rx_samplerate/2);
  if LPF[snd_ch]>round(rx_samplerate/2) then LPF[snd_ch] = round(rx_samplerate/2);
  if BPF[snd_ch]<1 then BPF[snd_ch] = 1;
  if TXBPF[snd_ch]<1 then TXBPF[snd_ch] = 1;
  if LPF[snd_ch]<1 then LPF[snd_ch] = 1;
  if TXDelay[snd_ch]<1 then TXDelay[snd_ch] = 1;
  if TXTail[snd_ch]<1 then TXTail[snd_ch] = 1;
  if BPF_tap[snd_ch]>1024 then BPF_tap[snd_ch] = 1024;
  if LPF_tap[snd_ch]>512 then LPF_tap[snd_ch] = 512;
  if BPF_tap[snd_ch]<8 then BPF_tap[snd_ch] = 8;
  if LPF_tap[snd_ch]<8 then LPF_tap[snd_ch] = 8;
  if not (RCVR[snd_ch] in [0..8]) then RCVR[snd_ch] = 0;
  if not (rcvr_offset[snd_ch] in [0..100]) then rcvr_offset[snd_ch] = 30;
  if not (speed[snd_ch] in [0..modes_count]) then speed[snd_ch] = SPEED_300;
*/
	switch (speed[snd_ch])
	{
	case SPEED_8P4800:
		init_8P4800(snd_ch);
		break;

	case SPEED_AE2400:
		init_AE2400(snd_ch);
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

	case SPEED_P600:
		init_P600(snd_ch);
		break;

	case SPEED_P300:
		init_P300(snd_ch);
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
	wf_pointer(soundChannel[snd_ch]);


}

/*
procedure TForm1.show_combobox;
var
  i word;
begin
  for i = 0 to length(modes_name)-1 do
  begin
    ComboBox1.Items.Add(modes_name[i]);
    ComboBox2.Items.Add(modes_name[i]);
  end;
  ComboBox1.ItemIndex = ComboBox1.Items.IndexOf(modes_name[Speed[1]]);
  ComboBox2.ItemIndex = ComboBox2.Items.IndexOf(modes_name[Speed[2]]);
end;

function TForm1.get_idx_by_name(name string) word;
var
  i word;
  found boolean;
begin
  i = 0;
  found = FALSE;
  result = 0;
  repeat
    if name = modes_name[i] then
    begin
      found = TRUE;
      result = i;
    end
    else inc(i);
  until found or (i = length(modes_name));
end;

procedure TForm1.ReadIni;
var
  snd_ch byte;
begin
  TelIni = TIniFile.Create(cur_dir+"soundmodem.ini");
  with TelIni do
  begin
    UTC_Time = ReadBool("Init","UTCTime",FALSE);
    MainPriority = ReadInteger("Init","Priority",2);
    nr_monitor_lines = ReadInteger("Init","NRMonitorLines",500);
    ptt = ReadString("Init","PTT","NONE");
    stop_wf = ReadBool("Init","StopWF",FALSE);
    raduga = ReadBool("Init","DispMode",DISP_MONO);
    stat_log = ReadBool("Init","StatLog",FALSE);
    SND_RX_DEVICE = ReadInteger("Init","SndRXDevice",0);
    SND_TX_DEVICE = ReadInteger("Init","SndTXDevice",0);
    snd_rx_device_name = ReadString("Init","SndRXDeviceName","");
    snd_tx_device_name = ReadString("Init","SndTXDeviceName","");
    RX_SR = ReadInteger("Init","RXSampleRate",11025);
    TX_SR = ReadInteger("Init","TXSampleRate",11025);
    RX_PPM = ReadInteger("Init","RX_corr_PPM",0);
    TX_PPM = ReadInteger("Init","TX_corr_PPM",0);
    tx_bufcount = ReadInteger("Init","TXBufNumber",32);
    rx_bufcount = ReadInteger("Init","RXBufNumber",32);
    DebugMode = ReadInteger("Init","DisableUnit",0);
    TX_rotate = ReadBool("Init","TXRotate",FALSE);
    DualChan = ReadBool("Init","DualChan",FALSE);
    DualPTT = ReadBool("Init","DualPTT",TRUE);
    SCO = ReadBool("Init","SCO",FALSE);
    stdtones = ReadBool("Init","UseStandardTones",TRUE);
    // Channel A settings
    maxframe[1] = ReadInteger("AX25_A","Maxframe",3);
    fracks[1] = ReadInteger("AX25_A","Retries",15);
    frack_time[1] = ReadInteger("AX25_A","FrackTime",5);
    idletime[1] = ReadInteger("AX25_A","IdleTime",180);
    slottime[1] = ReadInteger("AX25_A","SlotTime",100);
    persist[1] = ReadInteger("AX25_A","Persist",128);
    resptime[1] = ReadInteger("AX25_A","RespTime",1500);
    TXFrmMode[1] = ReadInteger("AX25_A","TXFrmMode",1);
    max_frame_collector[1] = ReadInteger("AX25_A","FrameCollector",6);
    exclude_callsigns[1] = ReadString("AX25_A","ExcludeCallsigns","");
    exclude_APRS_frm[1] = ReadString("AX25_A","ExcludeAPRSFrmType","");
    KISS_opt[1] = ReadBool("AX25_A","KISSOptimization",FALSE);
    dyn_frack[1] = ReadBool("AX25_A","DynamicFrack",FALSE);
    recovery[1] = ReadInteger("AX25_A","BitRecovery",0);
    NonAX25[1] = ReadBool("AX25_A","NonAX25Frm",FALSE);
    MEMRecovery[1] = ReadInteger("AX25_A","MEMRecovery",200);
    IPOLL[1] = ReadInteger("AX25_A","IPOLL",80);
    MyDigiCall[1] = ReadString("AX25_A","MyDigiCall","");
    tx_hitoneraisedb[1] = ReadInteger("AX25_A","HiToneRaise",0);
    // Channel B settings
    maxframe[2] = ReadInteger("AX25_B","Maxframe",3);
    fracks[2] = ReadInteger("AX25_B","Retries",15);
    frack_time[2] = ReadInteger("AX25_B","FrackTime",5);
    idletime[2] = ReadInteger("AX25_B","IdleTime",180);
    slottime[2] = ReadInteger("AX25_B","SlotTime",100);
    persist[2] = ReadInteger("AX25_B","Persist",128);
    resptime[2] = ReadInteger("AX25_B","RespTime",1500);
    TXFrmMode[2] = ReadInteger("AX25_B","TXFrmMode",1);
    max_frame_collector[2] = ReadInteger("AX25_B","FrameCollector",6);
    exclude_callsigns[2] = ReadString("AX25_B","ExcludeCallsigns","");
    exclude_APRS_frm[2] = ReadString("AX25_B","ExcludeAPRSFrmType","");
    KISS_opt[2] = ReadBool("AX25_B","KISSOptimization",FALSE);
    dyn_frack[2] = ReadBool("AX25_B","DynamicFrack",FALSE);
    recovery[2] = ReadInteger("AX25_B","BitRecovery",0);
    NonAX25[2] = ReadBool("AX25_B","NonAX25Frm",FALSE);
    MEMRecovery[2] = ReadInteger("AX25_B","MEMRecovery",200);
    IPOLL[2] = ReadInteger("AX25_B","IPOLL",80);
    MyDigiCall[2] = ReadString("AX25_B","MyDigiCall","");
    tx_hitoneraisedb[2] = ReadInteger("AX25_B","HiToneRaise",0);
    // Modem settings
    pkt_raw_min_len = ReadInteger("Modem","RawPktMinLen",17);
    swap_ptt = ReadBool("Modem","SwapPTTPins",FALSE);
    inv_ptt = ReadBool("Modem","InvPTTPins",FALSE);
    Emph_all[1] = ReadBool("Modem","PreEmphasisAll1",TRUE);
    Emph_all[2] = ReadBool("Modem","PreEmphasisAll2",TRUE);
    emph_db[1] = ReadInteger("Modem","PreEmphasisDB1",0);
    emph_db[2] = ReadInteger("Modem","PreEmphasisDB2",0);
    txbpf[1] = ReadInteger("Modem","TXBPF1",500);
    txbpf[2] = ReadInteger("Modem","TXBPF2",500);
    bpf[1] = ReadInteger("Modem","BPF1",500);
    bpf[2] = ReadInteger("Modem","BPF2",500);
    lpf[1] = ReadInteger("Modem","LPF1",150);
    lpf[2] = ReadInteger("Modem","LPF2",150);
    BPF_tap[1] = ReadInteger("Modem","BPFTap1",256);
    BPF_tap[2] = ReadInteger("Modem","BPFTap2",256);
    LPF_tap[1] = ReadInteger("Modem","LPFTap1",128);
    LPF_tap[2] = ReadInteger("Modem","LPFTap2",128);
    DCD_threshold = ReadInteger("Modem","DCDThreshold",32);
    rx_freq[1] = ReadFloat("Modem","RXFreq1",1700);
    rx_freq[2] = ReadFloat("Modem","RXFreq2",1700);
    CheckBox1.Checked = ReadBool("Modem","HoldPnt",FALSE);
    BIT_AFC = ReadInteger("Modem","AFC",32);
    txdelay[1] = ReadInteger("Modem","TxDelay1",250);
    txdelay[2] = ReadInteger("Modem","TxDelay2",250);
    txtail[1] = ReadInteger("Modem","TxTail1",50);
    txtail[2] = ReadInteger("Modem","TxTail2",50);
    diddles = ReadInteger("Modem","Diddles",0);
    RCVR[1] = ReadInteger("Modem","NRRcvrPairs1",0);
    RCVR[2] = ReadInteger("Modem","NRRcvrPairs2",0);
    rcvr_offset[1] = ReadInteger("Modem","RcvrShift1",30);
    rcvr_offset[2] = ReadInteger("Modem","RcvrShift2",30);
    speed[1] = ReadInteger("Modem","ModemType1",SPEED_1200);
    speed[2] = ReadInteger("Modem","ModemType2",SPEED_1200);
    modem_def[1] = ReadBool("Modem","Default1",TRUE);
    modem_def[2] = ReadBool("Modem","Default2",TRUE);
    AGWServ = ReadBool("AGWHost","Server",TRUE);
    AGWPort = ReadInteger("AGWHost","Port",8000);
    KISSServ = ReadBool("KISS","Server",FALSE);
    KISSPort = ReadInteger("KISS","Port",8100);
    Form1.Top = ReadInteger("Window","Top",0);
    Form1.Left = ReadInteger("Window","Left",0);
    Form1.Height = ReadInteger("Window","Height",656);
    Form1.Width = ReadInteger("Window","Width",764);
    MinOnStart = ReadBool("Window","MinimizedOnStartup",FALSE);
    Firstwaterfall1.checked = ReadBool("Window","Waterfall1",TRUE);
    Secondwaterfall1.checked = ReadBool("Window","Waterfall2",FALSE);
    Statustable1.checked = ReadBool("Window","StatTable",TRUE);
    Monitor1.checked = ReadBool("Window","Monitor",TRUE);
    RXRichEdit1.Font.Size = ReadInteger("Font","Size",RXRichEdit1.Font.Size);
    RXRichEdit1.Font.Name = ReadString("Font","Name",RXRichEdit1.Font.Name);
  end;
  TelIni.Free;
  newAGWPort = AGWPort;
  newAGWServ = AGWServ;
  newKISSPort = KISSPort;
  newKISSServ = KISSServ;

  RX_SampleRate = RX_SR+RX_SR*0.000001*RX_PPM;
  TX_SampleRate = TX_SR+TX_SR*0.000001*TX_PPM;

  panels[4] = Monitor1.Checked;
  panels[3] = Statustable1.Checked;
  panels[2] = Firstwaterfall1.Checked;
  panels[1] = Secondwaterfall1.Checked;

  if tx_bufcount>255 then tx_bufcount = 255;
  if tx_bufcount<2 then tx_bufcount = 2;
  if rx_bufcount>255 then rx_bufcount = 255;
  if rx_bufcount<2 then rx_bufcount = 2;

  if not (diddles in [0..2]) then diddles = 0;

  if nr_monitor_lines>65535 then nr_monitor_lines = 65535;
  if nr_monitor_lines<10    then nr_monitor_lines = 10;

  if not (MainPriority in [0..3]) then MainPriority = 2;

  for snd_ch = 1 to 2 do
  begin

    tx_hitoneraise[snd_ch] = power(10,-abs(tx_hitoneraisedb[snd_ch])/20);

    if IPOLL[snd_ch]<0 then IPOLL[snd_ch] = 0;
    if IPOLL[snd_ch]>65535 then IPOLL[snd_ch] = 65535;

    if MEMRecovery[snd_ch]<1 then MEMRecovery[snd_ch] = 1;
    if MEMRecovery[snd_ch]>65535 then MEMRecovery[snd_ch] = 65535;

    get_exclude_list(AnsiUpperCase(MyDigiCall[snd_ch]),list_digi_callsigns[snd_ch]);
    get_exclude_list(AnsiUpperCase(exclude_callsigns[snd_ch]),list_exclude_callsigns[snd_ch]);
    get_exclude_frm(exclude_APRS_frm[snd_ch],list_exclude_APRS_frm[snd_ch]);

    if resptime[snd_ch]<0 then resptime[snd_ch] = 0;
    if resptime[snd_ch]>65535 then resptime[snd_ch] = 65535;
    if persist[snd_ch]>255 then persist[snd_ch] = 255;
    if persist[snd_ch]<32 then persist[snd_ch] = 32;
    if fracks[snd_ch]<1 then fracks[snd_ch] = 1;
    if frack_time[snd_ch]<1 then frack_time[snd_ch] = 1;
    if idletime[snd_ch]<frack_time[snd_ch] then idletime[snd_ch] = 180;

    if not (Emph_db[snd_ch] in [0..nr_emph]) then Emph_db[snd_ch] = 0;
    if not (Recovery[snd_ch] in [0..1]) then Recovery[snd_ch] = 0;
    if not (TXFrmMode[snd_ch] in [0..1]) then TXFrmMode[snd_ch] = 0;
    if not (max_frame_collector[snd_ch] in [0..6]) then max_frame_collector[snd_ch] = 6;
    if not (maxframe[snd_ch] in [1..7]) then maxframe[snd_ch] = 3;

    if not (qpsk_set[snd_ch].mode in [0..1]) then qpsk_set[snd_ch].mode = 0;
    init_speed(snd_ch);
  end;
  TrackBar1.Position = DCD_threshold;

  // Check device ID
  ChkSndDevName;
end;

procedure TForm1.WriteIni;
begin
  TelIni = TIniFile.Create(cur_dir+"soundmodem.ini");
  with TelIni do
  begin
    WriteInteger("Init","Priority",MainPriority);
    WriteBool("Init","UTCTime",UTC_Time);
    WriteInteger("Init","NRMonitorLines",nr_monitor_lines);
    WriteString("Init","PTT",ptt);
    WriteBool("Init","DispMode",raduga);
    WriteBool("Init","StopWF",stop_wf);
    WriteBool("Init","StatLog",stat_log);
    WriteInteger("Init","SndRXDevice",SND_RX_DEVICE);
    WriteInteger("Init","SndTXDevice",SND_TX_DEVICE);
    WriteString("Init","SndRXDeviceName",snd_rx_device_name);
    WriteString("Init","SndTXDeviceName",snd_tx_device_name);
    WriteInteger("Init","RXSampleRate",RX_SR);
    WriteInteger("Init","TXSampleRate",TX_SR);
    WriteInteger("Init","RX_corr_PPM",RX_PPM);
    WriteInteger("Init","TX_corr_PPM",TX_PPM);
    WriteInteger("Init","DisableUnit",DebugMode);
    WriteBool("Init","TXRotate",TX_rotate);
    WriteBool("Init","DualChan",DualChan);
    WriteBool("Init","DualPTT",DualPTT);
    WriteBool("Init","SCO",SCO);
    WriteInteger("Init","TXBufNumber",tx_bufcount);
    WriteInteger("Init","RXBufNumber",rx_bufcount);
    WriteBool("Init","UseStandardTones",stdtones);
    // Channel A settings
    WriteInteger("AX25_A","Maxframe",maxframe[1]);
    WriteInteger("AX25_A","Retries",fracks[1]);
    WriteInteger("AX25_A","FrackTime",frack_time[1]);
    WriteInteger("AX25_A","IdleTime",idletime[1]);
    WriteInteger("AX25_A","SlotTime",slottime[1]);
    WriteInteger("AX25_A","Persist",persist[1]);
    WriteInteger("AX25_A","RespTime",resptime[1]);
    WriteInteger("AX25_A","TXFrmMode",TXFrmMode[1]);
    WriteInteger("AX25_A","FrameCollector",max_frame_collector[1]);
    WriteString("AX25_A","ExcludeCallsigns",exclude_callsigns[1]);
    WriteString("AX25_A","ExcludeAPRSFrmType",exclude_APRS_frm[1]);
    WriteBool("AX25_A","KISSOptimization",KISS_opt[1]);
    WriteBool("AX25_A","DynamicFrack",dyn_frack[1]);
    WriteInteger("AX25_A","BitRecovery",recovery[1]);
    WriteBool("AX25_A","NonAX25Frm",NonAX25[1]);
    WriteInteger("AX25_A","MEMRecovery",MEMRecovery[1]);
    WriteInteger("AX25_A","IPOLL",IPOLL[1]);
    WriteString("AX25_A","MyDigiCall",MyDigiCall[1]);
    WriteInteger("AX25_A","HiToneRaise",tx_hitoneraisedb[1]);
    // Channel B settings
    WriteInteger("AX25_B","Maxframe",maxframe[2]);
    WriteInteger("AX25_B","Retries",fracks[2]);
    WriteInteger("AX25_B","FrackTime",frack_time[2]);
    WriteInteger("AX25_B","IdleTime",idletime[2]);
    WriteInteger("AX25_B","SlotTime",slottime[2]);
    WriteInteger("AX25_B","Persist",persist[2]);
    WriteInteger("AX25_B","RespTime",resptime[2]);
    WriteInteger("AX25_B","TXFrmMode",TXFrmMode[2]);
    WriteInteger("AX25_B","FrameCollector",max_frame_collector[2]);
    WriteString("AX25_B","ExcludeCallsigns",exclude_callsigns[2]);
    WriteString("AX25_B","ExcludeAPRSFrmType",exclude_APRS_frm[2]);
    WriteBool("AX25_B","KISSOptimization",KISS_opt[2]);
    WriteBool("AX25_B","DynamicFrack",dyn_frack[2]);
    WriteInteger("AX25_B","BitRecovery",recovery[2]);
    WriteBool("AX25_B","NonAX25Frm",NonAX25[2]);
    WriteInteger("AX25_B","MEMRecovery",MEMRecovery[2]);
    WriteInteger("AX25_B","IPOLL",IPOLL[2]);
    WriteString("AX25_B","MyDigiCall",MyDigiCall[2]);
    WriteInteger("AX25_B","HiToneRaise",tx_hitoneraisedb[2]);
    // Modem settings
    if not modem_def[1] then
    begin
      WriteInteger("Modem","BPF1",bpf[1]);
      WriteInteger("Modem","TXBPF1",txbpf[1]);
      WriteInteger("Modem","LPF1",lpf[1]);
      WriteInteger("Modem","BPFTap1",BPF_tap[1]);
      WriteInteger("Modem","LPFTap1",LPF_tap[1]);
    end;
    if not modem_def[2] then
    begin
      WriteInteger("Modem","BPF2",bpf[2]);
      WriteInteger("Modem","TXBPF2",txbpf[2]);
      WriteInteger("Modem","LPF2",lpf[2]);
      WriteInteger("Modem","BPFTap2",BPF_tap[2]);
      WriteInteger("Modem","LPFTap2",LPF_tap[2]);
    end;
    WriteInteger("Modem","RawPktMinLen",pkt_raw_min_len);
    WriteBool("Modem","SwapPTTPins",swap_ptt);
    WriteBool("Modem","InvPTTPins",inv_ptt);
    WriteInteger("Modem","PreEmphasisDB1",emph_db[1]);
    WriteInteger("Modem","PreEmphasisDB2",emph_db[2]);
    WriteBool("Modem","PreEmphasisAll1",emph_all[1]);
    WriteBool("Modem","PreEmphasisAll2",emph_all[2]);
    WriteBool("Modem","Default1",modem_def[1]);
    WriteBool("Modem","Default2",modem_def[2]);
    WriteInteger("Modem","DCDThreshold",DCD_threshold);
    WriteBool("Modem","HoldPnt",CheckBox1.Checked);
    WriteFloat("Modem","RXFreq1",rx_freq[1]);
    WriteFloat("Modem","RXFreq2",rx_freq[2]);
    WriteFloat("Modem","AFC",BIT_AFC);
    WriteInteger("Modem","TxDelay1",txdelay[1]);
    WriteInteger("Modem","TxDelay2",txdelay[2]);
    WriteInteger("Modem","TxTail1",txtail[1]);
    WriteInteger("Modem","TxTail2",txtail[2]);
    WriteInteger("Modem","Diddles",diddles);
    WriteInteger("Modem","NRRcvrPairs1",RCVR[1]);
    WriteInteger("Modem","NRRcvrPairs2",RCVR[2]);
    WriteInteger("Modem","RcvrShift1",rcvr_offset[1]);
    WriteInteger("Modem","RcvrShift2",rcvr_offset[2]);
    WriteInteger("Modem","ModemType1",speed[1]);
    WriteInteger("Modem","ModemType2",speed[2]);
    WriteBool("AGWHost","Server",newAGWServ);
    WriteInteger("AGWHost","Port",newAGWPort);
    WriteBool("KISS","Server",newKISSServ);
    WriteInteger("KISS","Port",newKISSPort);
    WriteInteger("Window","Top",Form1.Top);
    WriteInteger("Window","Left",Form1.Left);
    WriteInteger("Window","Height",Form1.Height);
    WriteInteger("Window","Width",Form1.Width);
    WriteBool("Window","Waterfall1",Firstwaterfall1.checked);
    WriteBool("Window","Waterfall2",Secondwaterfall1.checked);
    WriteBool("Window","StatTable",Statustable1.checked);
    WriteBool("Window","Monitor",Monitor1.checked);
    WriteBool("Window","MinimizedOnStartup",MinOnStart);
    WriteInteger("Font","Size",RXRichEdit1.Font.Size);
    WriteString("Font","Name",RXRichEdit1.Font.Name);
  end;
  TelIni.Free;
end;

procedure TForm1.ChkSndDevName;
var
  DevInCaps TWaveInCapsA;
  DevOutCaps TWaveOutCapsA;
  i,k,numdevs integer;
  RXDevList,TXDevList TStringList;
begin
  RXDevList = TStringList.Create;
  TXDevList = TStringList.Create;
  numdevs = WaveOutGetNumDevs;
  if numdevs>0 then
    for k = 0 to numdevs-1 do
    begin
      waveOutGetDevCaps(k,@DevOutCaps,sizeof(DevOutCaps));
      TXDevList.Add(DevOutCaps.szpname);
    end;
  numdevs = WaveInGetNumDevs;
  if numdevs>0 then
    for k = 0 to numdevs-1 do
    begin
      waveInGetDevCaps(k,@DevInCaps,sizeof(DevInCaps));
      RXDevList.Add(DevInCaps.szpname);
    end;
  // TX Dev
  if (snd_tx_device<0) or (snd_tx_device> = TXDevList.Count) then snd_tx_device = 0;
  if TXDevList.Count>0 then
    if TXDevList.Strings[snd_tx_device]<>snd_tx_device_name then
    begin
      i = TXDevList.IndexOf(snd_tx_device_name);
      if i> = 0 then snd_tx_device = i else snd_tx_device_name = TXDevList.Strings[snd_tx_device];
    end;
  // RX Dev
  if (snd_rx_device<0) or (snd_rx_device> = RXDevList.Count) then snd_rx_device = 0;
  if RXDevList.Count>0 then
    if RXDevList.Strings[snd_rx_device]<>snd_rx_device_name then
    begin
      i = RXDevList.IndexOf(snd_rx_device_name);
      if i> = 0 then snd_rx_device = i else snd_rx_device_name = RXDevList.Strings[snd_rx_device];
    end;
  RXDevList.Free;
  TXDevList.Free;
end;

procedure TForm1.startrx;
var
  OpenResult MMRESULT;
  Loop       integer;
  ErrorText  string;
Begin
  RX_device = TRUE;
  RXBufferLength  =  rx_bufsize * Channels * (BitsPerSample div 8);
  with WaveFormat do
  begin
    wFormatTag       =  WAVE_FORMAT_PCM;
    nChannels        =  Channels;
    nSamplesPerSec   =  RX_SR;
    nAvgBytesPerSec  =  RX_SR * Channels * (BitsPerSample div 8);
    nBlockAlign      =  Channels * (BitsPerSample div 8);
    wBitsPerSample   =  BitsPerSample;
    cbSize           =  0;
  end;
  OpenResult  =  waveInOpen (@WaveInHandle,SND_RX_DEVICE,@WaveFormat,integer(Self.Handle),0,CALLBACK_WINDOW);
  if OpenResult = MMSYSERR_NOERROR then
  begin
    for Loop  =  1 to rx_bufcount do
    begin
      GetMem(RX_pbuf[Loop], RXBufferLength);
      RX_header[Loop].lpData          =  RX_pbuf[Loop];
      RX_header[Loop].dwBufferLength  =  RXBufferLength;
      RX_header[Loop].dwUser          =  Loop;
      RX_header[Loop].dwFlags         =  0;
      RX_header[Loop].dwLoops         =  0;
      OpenResult  =  WaveInPrepareHeader(WaveInhandle, @RX_header[Loop], sizeof(TWaveHdr));
      if OpenResult = MMSYSERR_NOERROR then WaveInAddBuffer(WaveInHandle, @RX_header[Loop], sizeof(TWaveHdr))
      else
        begin
          case OpenResult of
            MMSYSERR_INVALHANDLE   ErrorText  =  "device handle is invalid";
            MMSYSERR_NODRIVER      ErrorText  =  "no device driver present";
            MMSYSERR_NOMEM         ErrorText  =  "memory allocation error, could be incorrect samplerate";
            else                    ErrorText  =  "unknown error";
          end;
          MessageDlg(format("Error adding buffer %d device (%s)",[Loop, ErrorText]), mtError, [mbOk], 0);
        end;
    end;
    WaveInStart(WaveInHandle);
  end
  else
  begin
    case OpenResult of
      MMSYSERR_ERROR         ErrorText  =  "unspecified error";
      MMSYSERR_BADDEVICEID   ErrorText  =  "device ID out of range";
      MMSYSERR_NOTENABLED    ErrorText  =  "driver failed enable";
      MMSYSERR_ALLOCATED     ErrorText  =  "device already allocated";
      MMSYSERR_INVALHANDLE   ErrorText  =  "device handle is invalid";
      MMSYSERR_NODRIVER      ErrorText  =  "no device driver present";
      MMSYSERR_NOMEM         ErrorText  =  "memory allocation error, could be incorrect samplerate";
      MMSYSERR_NOTSUPPORTED  ErrorText  =  "function isn""t supported";
      MMSYSERR_BADERRNUM     ErrorText  =  "error value out of range";
      MMSYSERR_INVALFLAG     ErrorText  =  "invalid flag passed";
      MMSYSERR_INVALPARAM    ErrorText  =  "invalid parameter passed";
      MMSYSERR_HANDLEBUSY    ErrorText  =  "handle being used simultaneously on another thread (eg callback)";
      MMSYSERR_INVALIDALIAS  ErrorText  =  "specified alias not found";
      MMSYSERR_BADDB         ErrorText  =  "bad registry database";
      MMSYSERR_KEYNOTFOUND   ErrorText  =  "registry key not found";
      MMSYSERR_READERROR     ErrorText  =  "registry read error";
      MMSYSERR_WRITEERROR    ErrorText  =  "registry write error";
      MMSYSERR_DELETEERROR   ErrorText  =  "registry delete error";
      MMSYSERR_VALNOTFOUND   ErrorText  =  "registry value not found";
      MMSYSERR_NODRIVERCB    ErrorText  =  "driver does not call DriverCallback";
      else                    ErrorText  =  "unknown error";
    end;
    MessageDlg(format("Error opening wave input device (%s)",[ErrorText]), mtError, [mbOk], 0);
    RX_device = FALSE;
  end;
end;

procedure TForm1.stoprx;
var
  Loop integer;
begin
  if not RX_device then exit;
  WaveInStop(WaveInHandle);
  WaveInReset(WaveInHandle);
  for Loop  =  1 to rx_bufcount do
    WaveInUnPrepareHeader(WaveInHandle, @RX_header[Loop], sizeof(TWaveHdr));
  WaveInClose(WaveInHandle);
  for Loop  =  1 to rx_bufcount do
  begin
    if RX_pbuf[Loop]<>nil then
    begin
      FreeMem(RX_pbuf[Loop]);
      RX_pbuf[Loop]  =  nil;
    end;
  end;
  RX_device = FALSE;
end;

procedure TForm1.make_wave_buf(snd_ch byte; buf PChar);
const
  amplitude = 22000;
var
  i word;
begin
  modulator(snd_ch,audio_buf[snd_ch],tx_bufsize);
  if tx_status[snd_ch] = TX_NO_DATA then buf_status[snd_ch] = BUF_EMPTY;
  for i = 0 to tx_bufsize-1 do
  begin
    case snd_ch of
    1
      begin
        // left channel
        PSmallInt(buf)^ = round(amplitude*audio_buf[snd_ch][i]);
        Inc(PSmallInt(Buf));
        // right channel
        if SCO then PSmallInt(buf)^ = round(amplitude*audio_buf[snd_ch][i]) else PSmallInt(buf)^ = 0;
        Inc(PSmallInt(Buf));
      end;
    2
      begin
        // left channel
        if SCO then PSmallInt(buf)^ = round(amplitude*audio_buf[snd_ch][i]) else PSmallInt(buf)^ = 0;
        Inc(PSmallInt(Buf));
        // right channel
        PSmallInt(buf)^ = round(amplitude*audio_buf[snd_ch][i]);
        Inc(PSmallInt(Buf));
      end;
    end;
  end;
end;

procedure TForm1.starttx(snd_ch byte);
var
  OpenResult MMRESULT;
  Loop       integer;
  ErrorText  string;
  BufferLength longint;
Begin
  if snd_status[snd_ch]<>SND_IDLE then exit;
  BufferLength  =  tx_bufsize * Channels * (BitsPerSample div 8);
  with WaveFormat do
  begin
    wFormatTag       =  WAVE_FORMAT_PCM;
    nChannels        =  Channels;
    nSamplesPerSec   =  TX_SR;
    nAvgBytesPerSec  =  TX_SR * Channels * (BitsPerSample div 8);
    nBlockAlign      =  Channels * (BitsPerSample div 8);
    wBitsPerSample   =  BitsPerSample;
    cbSize           =  0;
  end;
  OpenResult  =  WaveOutOpen (@WaveOutHandle[snd_ch],SND_TX_DEVICE,@WaveFormat,integer(Self.Handle),0,CALLBACK_WINDOW);
  if OpenResult = MMSYSERR_NOERROR then
  begin
    snd_status[snd_ch] = SND_TX;
    buf_status[snd_ch] = BUF_FULL;
    tx_status[snd_ch] = TX_SILENCE;
    tx_buf_num[snd_ch] = 0;
    tx_buf_num1[snd_ch] = 0;
    for Loop  =  1 to tx_bufcount do
    begin
      GetMem(TX_pbuf[snd_ch][Loop], BufferLength);
      TX_header[snd_ch][Loop].lpData          =  TX_pbuf[snd_ch][Loop];
      TX_header[snd_ch][Loop].dwBufferLength  =  BufferLength;
      TX_header[snd_ch][Loop].dwUser          =  0;
      TX_header[snd_ch][Loop].dwFlags         =  0;
      TX_header[snd_ch][Loop].dwLoops         =  0;
      OpenResult  =  WaveOutPrepareHeader(WaveOuthandle[snd_ch], @TX_header[snd_ch][Loop], sizeof(TWaveHdr));
      if OpenResult = MMSYSERR_NOERROR then
      begin
        // Заполнить буфер на передачу
        if buf_status[snd_ch] = BUF_FULL then
        begin
          make_wave_buf(snd_ch,TX_pbuf[snd_ch][Loop]);
          WaveOutWrite(WaveOutHandle[snd_ch],@TX_header[snd_ch][Loop],sizeof(TWaveHdr));
          inc(tx_buf_num1[snd_ch]);
        end;
      end
      else
      begin
        case OpenResult of
          MMSYSERR_INVALHANDLE   ErrorText  =  "device handle is invalid";
          MMSYSERR_NODRIVER      ErrorText  =  "no device driver present";
          MMSYSERR_NOMEM         ErrorText  =  "memory allocation error, could be incorrect samplerate";
          else                    ErrorText  =  "unknown error";
        end;
        MessageDlg(format("Error adding buffer %d device (%s)",[Loop, ErrorText]), mtError, [mbOk], 0);
      end;
    end;
  end
  else
  begin
    case OpenResult of
      MMSYSERR_ERROR         ErrorText  =  "unspecified error";
      MMSYSERR_BADDEVICEID   ErrorText  =  "device ID out of range";
      MMSYSERR_NOTENABLED    ErrorText  =  "driver failed enable";
      MMSYSERR_ALLOCATED     ErrorText  =  "device already allocated";
      MMSYSERR_INVALHANDLE   ErrorText  =  "device handle is invalid";
      MMSYSERR_NODRIVER      ErrorText  =  "no device driver present";
      MMSYSERR_NOMEM         ErrorText  =  "memory allocation error, could be incorrect samplerate";
      MMSYSERR_NOTSUPPORTED  ErrorText  =  "function isn""t supported";
      MMSYSERR_BADERRNUM     ErrorText  =  "error value out of range";
      MMSYSERR_INVALFLAG     ErrorText  =  "invalid flag passed";
      MMSYSERR_INVALPARAM    ErrorText  =  "invalid parameter passed";
      MMSYSERR_HANDLEBUSY    ErrorText  =  "handle being used simultaneously on another thread (eg callback)";
      MMSYSERR_INVALIDALIAS  ErrorText  =  "specified alias not found";
      MMSYSERR_BADDB         ErrorText  =  "bad registry database";
      MMSYSERR_KEYNOTFOUND   ErrorText  =  "registry key not found";
      MMSYSERR_READERROR     ErrorText  =  "registry read error";
      MMSYSERR_WRITEERROR    ErrorText  =  "registry write error";
      MMSYSERR_DELETEERROR   ErrorText  =  "registry delete error";
      MMSYSERR_VALNOTFOUND   ErrorText  =  "registry value not found";
      MMSYSERR_NODRIVERCB    ErrorText  =  "driver does not call DriverCallback";
      else                    ErrorText  =  "unknown error";
    end;
    MessageDlg(format("Error opening wave output device (%s)",[ErrorText]), mtError, [mbOk], 0);
  end;
end;

procedure TForm1.stoptx(snd_ch byte);
var
  Loop integer;
begin
  if snd_status[snd_ch]<>SND_TX then exit;
  WaveOutReset(WaveOutHandle[snd_ch]);
  for Loop  =  1 to tx_bufcount do
    WaveOutUnPrepareHeader(WaveOutHandle[snd_ch], @TX_header[snd_ch][Loop], sizeof(TWaveHdr));
  WaveOutClose(WaveOutHandle[snd_ch]);
  for Loop  =  1 to tx_bufcount do
  begin
    if TX_pbuf[snd_ch][Loop]<>nil then
    begin
      FreeMem(TX_pbuf[snd_ch][Loop]);
      TX_pbuf[snd_ch][Loop]  =  nil;
    end;
  end;
  WaveOutHandle[snd_ch] = 0;
  snd_status[snd_ch] = SND_IDLE;
end;

procedure show_grid_title;
const
  title array [0..10] of string  =  ("MyCall","DestCall","Status","Sent pkts","Sent bytes","Rcvd pkts","Rcvd bytes","Rcvd FC","CPS TX","CPS RX","Direction");
var
  i byte;
begin
  for i = 0 to 10 do Form1.StringGrid1.Cells[i,0] = title[i];
end;
*/
/*

procedure disp1(src1,src2 array of single);
var
  i,n word;
  k,k1,amp1,amp2,amp3,amp4 single;
  bm TBitMap;
begin
  bm = TBitMap.Create;
  bm.pixelformat = pf32bit;
  //bm.pixelformat = pf24bit;
  bm.Width = Form1.PaintBox2.Width;
  bm.Height = Form1.PaintBox2.Height;
  amp1 = 0;
  amp3 = 0;
  //k = 0.20;
  k = 50000;
  k1 = 0;
  //k = 1000;
  //k = 0.00001;
  bm.Canvas.MoveTo(0,50);
  bm.Canvas.LineTo(512,50);
  n = 0;
  for i = 0 to RX_Bufsize-1 do
  begin
    begin
      amp2 = src1[i];
      amp4 = src2[i];
      bm.Canvas.Pen.Color = clRed;
      bm.Canvas.MoveTo(n,50-round(amp1*k1));
      bm.Canvas.LineTo(n+1,50-round(amp2*k1));
      bm.Canvas.Pen.Color = clBlue;
      bm.Canvas.MoveTo(n,50-round(amp3*k));
      bm.Canvas.LineTo(n+1,50-round(amp4*k));
      bm.Canvas.Pen.Color = clBlack;
      inc(n);
      amp1 = amp2;
      amp3 = amp4;
    end;
  end;
  Form1.PaintBox2.Canvas.Draw(0,0,bm);
  bm.Free;
end;
*/

/*

procedure TForm1.wf_pointer(snd_ch byte);
var
  x single;
  x1,x2,y,k,pos1,pos2,pos3 word;
begin
  k = 24;
  x = fft_size/RX_SampleRate;
  pos1 = round((rx_freq[snd_ch]-0.5*rx_shift[snd_ch])*x)-5;
  pos2 = round((rx_freq[snd_ch]+0.5*rx_shift[snd_ch])*x)-5;
  pos3 = round(rx_freq[snd_ch]*x);
  x1 = pos1+5;
  x2 = pos2+5;
  y = k+5;
  with bm3.Canvas do
  begin
    Draw(0,20,bm[snd_ch]);
    Pen.Color = clWhite;
    Brush.Color = clRed;
    Polygon([Point(x1+3,y),Point(x1,y-7),Point(x1-3,y),Point(x2+3,y),Point(x2,y-7),Point(x2-3,y)]);
    Brush.Color = clBlue;
    Polygon([Point(x1+3,y),Point(x1,y+7),Point(x1-3,y),Point(x2+3,y),Point(x2,y+7),Point(x2-3,y)]);
    Polyline([Point(pos3,k+1),Point(pos3,k+9)]);
    Pen.Color = clBlack;
  end;
  case snd_ch of
    1  PaintBox1.Canvas.Draw(0,0,bm3);
    2  PaintBox3.Canvas.Draw(0,0,bm3);
  end;
end;

procedure TForm1.wf_Scale;
var
  k single;
  max_freq,x,i word;
begin
  max_freq = round(RX_SampleRate*0.005);
  k = 100*fft_size/RX_SampleRate;
  with bm1.Canvas do
  begin
    Brush.Color = clBlack;
    FillRect(ClipRect);
    Pen.Color = clWhite;
    Font.Color = clWhite;
    Font.Size = 8;
    for i = 0 to max_freq do
    begin
      x = round(k*i);
      if x<1025 then
      begin
        if (i mod 5) = 0 then
          PolyLine([Point(x,20),Point(x,13)])
        else
          PolyLine([Point(x,20),Point(x,16)]);
        if (i mod 10) = 0 then TextOut(x-12,1,inttostr(i*100));
      end;
    end;
    Pen.Color = clBlack;
  end;
  bm3.Canvas.Draw(0,0,bm1);
end;
*/

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

	// do we need to do this again ??
//			make_core_BPF(snd_ch, rx_freq[snd_ch], bpf[snd_ch]);

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
	word i, i1;
	Byte snd_ch, rcvr_idx;
	boolean add_fft_line;
	int buf_offset;

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

	add_fft_line = FALSE;
	fft_mult++;

	if (fft_mult == fft_spd)
	{
		add_fft_line = TRUE;
		fft_mult = 0;
	}

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

				for (i = 0; i < rx_bufsize; i++)
				{
					ardopbuff[i] = Samples[i1];
					i1++;
					i1++;
				}

				ARDOPProcessNewSamples(ardopbuff, nSamples);
			}
		}

		// extract mono samples from data. 

		data1 = Samples;

		i1 = 0;

		// src_buf[0] is left data,. src_buf[1] right

		// We could skip extracting other channel if only one in use - is it worth it??

		if (UsingBothChannels)
		{
			for (i = 0; i < rx_bufsize; i++)
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

			for (i = 0; i < rx_bufsize; i++)
			{
				src_buf[1][i] = data1[i1];
				i1 += 2;
			}
		}
		else
		{
			// Extract just left

			for (i = 0; i < rx_bufsize; i++)
			{
				src_buf[0][i] = data1[i1];
				i1 += 2;
			}
		}

		// Run modems before waterfall so fft buffer has values from before sync was detected

		runModems();

		// Do whichever waterfall is needed

		int FirstWaterfallChan = 0;

		// not sure why this is needed

//	if (UsingLeft)
//		chk_snd_buf(src_buf[0], rx_bufsize);
//	if (UsingRight)
//		chk_snd_buf(src_buf[1], rx_bufsize);


		if (Firstwaterfall)
		{
			if (UsingLeft == 0)
			{
				FirstWaterfallChan = 1;
				data1++;					// to Right Samples
			}

			buf_offset = fft_size - rx_bufsize;
			move((UCHAR *)&fft_buf[FirstWaterfallChan][rx_bufsize], (UCHAR *)&fft_buf[FirstWaterfallChan][0], buf_offset * 2);

			for (i = 0; i < rx_bufsize; i++)
			{
				fft_buf[FirstWaterfallChan][i + buf_offset] = *data1;
				data1 += 2;
			}

			if (add_fft_line)
				if (Firstwaterfall)
					doWaterfall(FirstWaterfallChan);
		}

		if (UsingBothChannels && Secondwaterfall)
		{
			// Second is always Right

			data1 = &Samples[1];			// to Right Samples

			buf_offset = fft_size - rx_bufsize;
			move((UCHAR *)&fft_buf[1][rx_bufsize], (UCHAR *)&fft_buf[1][0], buf_offset * 2);

			for (i = 0; i < rx_bufsize; i++)
			{
				fft_buf[1][i + buf_offset] = *data1;
				data1 += 2;
			}

			if (add_fft_line)
				if (Secondwaterfall)
					doWaterfall(1);
		}

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


/*
procedure TForm1.RX2TX(snd_ch byte);
begin
  if snd_status[snd_ch] = SND_IDLE then begin ptton(snd_ch); starttx(snd_ch); end;
end;

function TForm1.frame_monitor(s,code string; tx_stat boolean) string;
var
  len word;
  s_tx_stat string;
  time_now,s1,c,p string;
  callfrom,callto,digi,path,data,frm string;
  frm_body string;
  pid,nr,ns,f_type,f_id byte;
  rpt,cr,pf boolean;
  i word;
begin
  decode_frame(s,path,data,pid,nr,ns,f_type,f_id,rpt,pf,cr);
  len = length(data);
  // NETROM parsing
  if pid = $CF then data = parse_NETROM(data,f_id);
  // IP parsing
  if pid = $CC then data = parse_IP(data);
  // ARP parsing
  if pid = $CD then data = parse_ARP(data);
  //
  get_monitor_path(path,CallTo,CallFrom,Digi);
  if cr then
  begin
    c = "C";
    if pf then p = " P" else p = "";
  end
  else
  begin
    c = "R";
    if pf then p = " F" else p = "";
  end;
  frm = "UNKN";
  case f_id of
    I_I     frm = "I";
    S_RR    frm = "RR";
    S_RNR   frm = "RNR";
    S_REJ   frm = "REJ";
    U_SABM  frm = "SABM";
    U_DISC  frm = "DISC";
    U_DM    frm = "DM";
    U_UA    frm = "UA";
    U_FRMR  frm = "FRMR";
    U_UI    frm = "UI";
  end;
  case tx_stat of
    TRUE    s_tx_stat = "T";
    FALSE   s_tx_stat = "R";
  end;
  s1 = "";

  if code<>"" then code = " ["+code+"]";
  if UTC_Time then time_now = " [UTC"+get_UTC_time+s_tx_stat+"]"
  else time_now = " ["+FormatDateTime("hhmmss",now)+s_tx_stat+"]";

  if digi = "" then frm_body = "Fm "+CallFrom+" To "+CallTo+" <"+frm+" "+c+p
  else frm_body = "Fm "+CallFrom+" To "+CallTo+" Via "+Digi+" <"+frm+" "+c+p;
  case f_type of
    I_FRM   frm_body = frm_body+" R"+inttostr(nr)+" S"+inttostr(ns)+" Pid = "+dec2hex(pid)+" Len = "+inttostr(len)+">"+time_now+code+#13+data;
    U_FRM   if f_id = U_UI then frm_body = frm_body+" Pid = "+dec2hex(pid)+" Len = "+inttostr(len)+">"+time_now+code+#13+data
               else if f_id = U_FRMR then begin data = copy(data+#0#0#0,1,3); frm_body = frm_body+">"+time_now+code+#13+inttohex((byte(data[1]) shl 16) or (byte(data[2]) shl 8) or byte(data[3]),6) end
                 else frm_body = frm_body+">"+time_now+code;
    S_FRM   frm_body = frm_body+" R"+inttostr(nr)+">"+time_now+code;
  end;
  for i = 1 to length(frm_body) do
  begin
    if frm_body[i]>#31 then s1 = s1+frm_body[i];
    if frm_body[i] = #13 then s1 = s1+#13#10;
    if frm_body[i] = #10 then s1 = s1+"";
    if frm_body[i] = #9 then s1 = s1+#9;
  end;
  result = s1;
end;

procedure TForm1.waterfall_init;
begin
  bm[1] = TBitMap.Create;
  bm[2] = TBitMap.Create;
  bm1 = TBitMap.Create;
  bm2 = TBitMap.Create;
  bm3 = TBitMap.Create;
  bm[1].pixelformat = pf32bit;
  bm[2].pixelformat = pf32bit;
  bm1.pixelformat = pf32bit;
  bm2.pixelformat = pf32bit;
  bm3.pixelformat = pf32bit;
  bm[1].Height = PaintBox1.Height-20;
  bm[1].Width = PaintBox1.width;
  bm[2].Height = PaintBox1.Height-20;
  bm[2].Width = PaintBox1.width;
  bm1.Height = 20;
  bm1.Width = PaintBox1.width;
  bm3.Height = PaintBox1.Height;
  bm3.Width = PaintBox1.width;
end;

procedure TForm1.waterfall_free;
begin
  bm[1].Free;
  bm[2].Free;
  bm1.Free;
  bm2.Free;
  bm3.Free;
end;

procedure TForm1.StartAGW;
begin
  try
    ServerSocket1.Port = AGWPort;
    ServerSocket1.Active = AGWServ;
  except
    ServerSocket1.Active = FALSE;
    MessageDlg("AGW host port is busy!", mtWarning,[mbOk],0);
  end;
end;

procedure TForm1.StartKISS;
begin
  try
    ServerSocket2.Port = KISSPort;
    ServerSocket2.Active = KISSServ;
  except
    ServerSocket2.Active = FALSE;
    MessageDlg("KISS port is busy!", mtWarning,[mbOk],0);
  end;
end;

procedure fft_window_init;
var
  mag single;
  i word;
begin
  mag = 2*pi/(fft_size-1);
  for i = 0 to fft_size-1 do fft_window_arr[i] = 0.5-0.5*cos(i*mag); //hann
end;

procedure TForm1.FormCreate(Sender TObject);
begin
  if hPrevInst <> 0 then begin
    MessageDlg("Программа уже запущена!", mtError, [mbOk], 0);
    Application.Terminate;
  end;
  RS = TReedSolomon.Create(Self);
  MainThreadHandle = GetCurrentThread;
  form1.Caption = MODEM_CAPTION+" - Ver "+MODEM_VERSION;
  cur_dir = ExtractFilePath(Application.ExeName);
  fft_window_init;
  detector_init;
  kiss_init;
  agw_init;
  ax25_init;
  init_raduga;
  waterfall_init;
  ReadIni;
  show_combobox;
  show_grid_title;
  show_mode_panels;
  show_panels;
  wf_pointer(1);
  wf_pointer(2);
  wf_Scale;
  ChangePriority;
  Visible = TRUE;
  StartAGW;
  StartKISS;
  PTTOpen;
  startrx;
  TimerEvent = TIMER_EVENT_OFF;
  if (debugmode and DEBUG_TIMER) = 0 then create_timer1;
  if MinOnStart then WindowState = wsMinimized;
end;

procedure TForm1.TrackBar1Change(Sender TObject);
begin
  dcd_threshold = TrackBar1.position;
end;
*/

void Timer_Event2()
{
	if (TimerStat2 == TIMER_BUSY || TimerStat2 == TIMER_OFF)
		return;

	TimerStat2 = TIMER_BUSY;

//	show_grid();

	/*

	if (mod_icon_status = MOD_WAIT) then inc(icon_timer);
	if icon_timer = 10 then mod_icon_status = MOD_IDLE;
	if (mod_icon_status<>MOD_WAIT) and (mod_icon_status<>last_mod_icon_status) then
	begin
	  icon_timer = 0;
	  case mod_icon_status of
		MOD_IDLE form1.CoolTrayIcon1.IconIndex = 0;
		MOD_RX begin form1.CoolTrayIcon1.IconIndex = 1; mod_icon_status = MOD_WAIT; end;
		MOD_TX form1.CoolTrayIcon1.IconIndex = 2;
	  end;
	  last_mod_icon_status = mod_icon_status;
	end;
	//*/

	TimerStat2 = TIMER_FREE;
}

/*

procedure TimeProc1(uTimerId, uMesssage UINT; dwUser, dw1, dw2 DWORD); stdcall;
begin
  TimerEvent = TIMER_EVENT_ON;
end;

procedure TForm1.create_timer1;
var
  TimeEpk cardinal;
begin
  TimeEpk = 100;
  TimerId1 = TimeSetEvent(TimeEpk,0,@TimeProc1,0,TIME_PERIODIC);
end;

procedure TForm1.free_timer1;
begin
  TimerStat1 = TIMER_OFF;
  timeKillEvent(TimerId1);
end;

*/

/*


procedure TForm1.PaintBox1MouseMove(Sender TObject; Shift TShiftState; X,
  Y Integer);
var
  low,high word;
begin
  if CheckBox1.Checked then exit;
  if not mouse_down[1] then Exit;
  rx_freq[1] = round(x*RX_SampleRate/fft_size);
  low = round(rx_shift[1]/2+Rcvr[1]*rcvr_offset[1]+1);
  high = round(rx_samplerate/2-(rx_shift[1]/2+Rcvr[1]*rcvr_offset[1]));
  if (rx_freq[1]-low)<0 then rx_freq[1] = low;
  if (high-rx_freq[1])<0 then rx_freq[1] = high;
  tx_freq[1] = rx_freq[1];
  show_freq_a;
end;

procedure TForm1.PaintBox3MouseMove(Sender TObject; Shift TShiftState; X,
  Y Integer);
var
  low,high word;
begin
  if CheckBox1.Checked then exit;
  if not mouse_down[2] then Exit;
  rx_freq[2] = round(x*RX_SampleRate/fft_size);
  low = round(rx_shift[2]/2+Rcvr[2]*rcvr_offset[2]+1);
  high = round(rx_samplerate/2-(rx_shift[2]/2+Rcvr[2]*rcvr_offset[2]));
  if (rx_freq[2]-low)<0 then rx_freq[2] = low;
  if (high-rx_freq[2])<0 then rx_freq[2] = high;
  tx_freq[2] = rx_freq[2];
  show_freq_b;
end;

procedure TForm1.PaintBox1MouseUp(Sender TObject; Button TMouseButton;
  Shift TShiftState; X, Y Integer);
begin
  mouse_down[1] = FALSE;
end;

procedure TForm1.PaintBox3MouseUp(Sender TObject; Button TMouseButton;
  Shift TShiftState; X, Y Integer);
begin
  mouse_down[2] = FALSE;
end;

procedure TForm1.PaintBox1MouseDown(Sender TObject; Button TMouseButton;
  Shift TShiftState; X, Y Integer);
var
  low,high word;
begin
  if CheckBox1.Checked then exit;
  if not (ssLeft in shift) then Exit;
  mouse_down[1] = TRUE;
  rx_freq[1] = round(x*RX_SampleRate/fft_size);
  low = round(rx_shift[1]/2+Rcvr[1]*rcvr_offset[1]+1);
  high = round(rx_samplerate/2-(rx_shift[1]/2+Rcvr[1]*rcvr_offset[1]));
  if (rx_freq[1]-low)<0 then rx_freq[1] = low;
  if (high-rx_freq[1])<0 then rx_freq[1] = high;
  tx_freq[1] = rx_freq[1];
  show_freq_a;
end;

procedure TForm1.PaintBox3MouseDown(Sender TObject; Button TMouseButton;
  Shift TShiftState; X, Y Integer);
var
  low,high word;
begin
  if CheckBox1.Checked then exit;
  if not (ssLeft in shift) then Exit;
  mouse_down[2] = TRUE;
  rx_freq[2] = round(x*RX_SampleRate/fft_size);
  low = round(rx_shift[2]/2+Rcvr[2]*rcvr_offset[2]+1);
  high = round(rx_samplerate/2-(rx_shift[2]/2+Rcvr[2]*rcvr_offset[2]));
  if (rx_freq[2]-low)<0 then rx_freq[2] = low;
  if (high-rx_freq[2])<0 then rx_freq[2] = high;
  tx_freq[2] = rx_freq[2];
  show_freq_b;
end;

procedure TForm1.ServerSocket1ClientRead(Sender TObject;
  Socket TCustomWinSocket);
var
  s string;
begin
  s = Socket.ReceiveText;
  AGW_explode_frame(Socket.sockethandle,s);
end;

procedure TForm1.ServerSocket1ClientDisconnect(Sender TObject;
  Socket TCustomWinSocket);
begin
  del_incoming_mycalls_by_sock(socket.SocketHandle);
  AGW_del_socket(socket.SocketHandle);
end;

procedure TForm1.ServerSocket1ClientError(Sender TObject;
  Socket TCustomWinSocket; ErrorEvent TErrorEvent;
  var ErrorCode Integer);
begin
  del_incoming_mycalls_by_sock(socket.SocketHandle);
  AGW_del_socket(socket.SocketHandle);
  ErrorCode = 0;
end;

procedure TForm1.ServerSocket1ClientConnect(Sender TObject;
  Socket TCustomWinSocket);
begin
  agw_add_socket(Socket.sockethandle);
end;

procedure TForm1.OutputVolume1Click(Sender TObject);
var
  s string;
begin
  s = "SndVol32.exe -D"+inttostr(SND_TX_DEVICE);
  WinExec(pchar(s),SW_SHOWNORMAL);
end;

procedure TForm1.InputVolume1Click(Sender TObject);
var
  s string;
begin
  s = "SndVol32.exe -R -D"+inttostr(SND_RX_DEVICE);
  WinExec(pchar(s),SW_SHOWNORMAL);
end;

procedure TForm1.CoolTrayIcon1Click(Sender TObject);
begin
  CoolTrayIcon1.ShowMainForm;
end;

procedure TForm1.CoolTrayIcon1Cycle(Sender TObject; NextIndex Integer);
begin
  CoolTrayIcon1.IconIndex = 2;
  CoolTrayIcon1.CycleIcons = FALSE;
end;

procedure TForm1.ABout1Click(Sender TObject);
begin
  Form2.ShowModal;
end;

procedure TForm1.put_frame(snd_ch byte; frame,code string; tx_stat,excluded boolean);
var
  s string;
begin
  if RxRichedit1.Focused then Windows.SetFocus(0);
  if code = "NON-AX25" then
    s = inttostr(snd_ch)+" <NON-AX25 frame Len = "+inttostr(length(frame)-2)+"> ["+FormatDateTime("hhmmss",now)+"R]"
  else
    s = inttostr(snd_ch)+""+frame_monitor(frame,code,tx_stat);
  //RxRichedit1.Lines.BeginUpdate;
  RxRichedit1.SelStart = length(RxRichedit1.text);
  RxRichEdit1.SelLength = length(s);
  case tx_stat of
    TRUE   RxRichEdit1.SelAttributes.Color = clMaroon;
    FALSE  RxRichEdit1.SelAttributes.Color = clBlack;
  end;
  if excluded then RxRichEdit1.SelAttributes.Color = clGreen;
  RxRichedit1.SelText = s+#10;
  if RxRichedit1.Lines.Count>nr_monitor_lines then
  repeat
    RxRichedit1.Lines.Delete(0);
  until RxRichedit1.Lines.Count = nr_monitor_lines;
  RxRichedit1.HideSelection = FALSE;
  RxRichedit1.SelStart = length(RxRichedit1.text);
  RxRichedit1.SelLength = 0;
  RxRichedit1.HideSelection = TRUE;
  //RxRichedit1.Lines.EndUpdate;
end;

procedure TForm1.show_mode_panels;
begin
  panel8.Align = alNone;
  panel6.Align = alNone;
  if dualchan then panel6.Visible = TRUE else panel6.Visible = FALSE;
  panel8.Align = alLeft;
  panel6.Align = alLeft;
end;

procedure TForm1.show_panels;
var
  i byte;
begin
  panel1.Align = alNone;
  panel2.Align = alNone;
  panel3.Align = alNone;
  panel4.Align = alNone;
  panel5.Align = alNone;
  for i = 1 to 5 do
  case i of
    1  panel1.Visible = panels[i];
    2  panel5.Visible = panels[i];
    3  panel3.Visible = panels[i];
    4  panel4.Visible = panels[i];
    5  panel2.Visible = panels[i];
  end;
  panel1.Align = alBottom;
  panel5.Align = alBottom;
  panel3.Align = alBottom;
  panel2.Align = alTop;
  panel4.Align = alClient;
end;

procedure TForm1.Secondwaterfall1Click(Sender TObject);
begin
  case Secondwaterfall1.Checked of
    TRUE   Secondwaterfall1.Checked = FALSE;
    FALSE  Secondwaterfall1.Checked = TRUE;
  end;
  panels[1] = Secondwaterfall1.Checked;
  show_panels;
end;



procedure TForm1.Firstwaterfall1Click(Sender TObject);
begin
  case Firstwaterfall1.Checked of
    TRUE   Firstwaterfall1.Checked = FALSE;
    FALSE  Firstwaterfall1.Checked = TRUE;
  end;
  panels[2] = Firstwaterfall1.Checked;
  show_panels;
end;

procedure TForm1.Statustable1Click(Sender TObject);
begin
  case Statustable1.Checked of
    TRUE   Statustable1.Checked = FALSE;
    FALSE  Statustable1.Checked = TRUE;
  end;
  panels[3] = Statustable1.Checked;
  show_panels;
end;

procedure TForm1.Monitor1Click(Sender TObject);
begin
  case Monitor1.Checked of
    TRUE   Monitor1.Checked = FALSE;
    FALSE  Monitor1.Checked = TRUE;
  end;
  panels[4] = Monitor1.Checked;
  show_panels;
end;

procedure TForm1.Devices1Click(Sender TObject);
begin
  if (ptt = "EXT") or (ptt = "CAT") then Form3.Button3.Enabled = TRUE else Form3.Button3.Enabled = FALSE;
  Form3.GetDeviceInfo;
  form3.ShowModal;
end;

procedure TForm1.FormPaint(Sender TObject);
begin
  RxRichedit1.HideSelection = FALSE;
  RxRichedit1.SelStart = length(RxRichedit1.text);
  RxRichedit1.SelLength = 0;
  RxRichedit1.HideSelection = TRUE;
end;

procedure TForm1.Filters1Click(Sender TObject);
begin
  Form5.Show_modem_settings;
end;

procedure TForm1.Clearmonitor1Click(Sender TObject);
begin
  RxRichEdit1.Clear;
  frame_count = 0;
  single_frame_count = 0;
end;

procedure TForm1.Copytext1Click(Sender TObject);
begin
  RxRichEdit1.CopyToClipboard;
end;

procedure TForm1.ApplicationEvents1Minimize(Sender TObject);
begin
  if stop_wf then w_state = WIN_MINIMIZED;
end;

procedure TForm1.ApplicationEvents1Restore(Sender TObject);
begin
  w_state = WIN_MAXIMIZED;
end;

procedure TForm1.ServerSocket2ClientConnect(Sender TObject;
  Socket TCustomWinSocket);
begin
  KISS_add_stream(socket.sockethandle);
end;

procedure TForm1.ServerSocket2ClientDisconnect(Sender TObject;
  Socket TCustomWinSocket);
begin
  KISS_del_stream(socket.sockethandle);
end;

procedure TForm1.ServerSocket2ClientError(Sender TObject;
  Socket TCustomWinSocket; ErrorEvent TErrorEvent;
  var ErrorCode Integer);
begin
  KISS_del_stream(socket.sockethandle);
  ErrorCode = 0;
end;

procedure TForm1.ServerSocket2ClientRead(Sender TObject;
  Socket TCustomWinSocket);
var
  data string;
begin
  data = socket.ReceiveText;
  KISS_on_data_in(socket.sockethandle,data);
end;

procedure TForm1.Font1Click(Sender TObject);
begin
  FontDialog1.Font = RXRichEdit1.Font;
  if FontDialog1.Execute then
  begin
    RXRichEdit1.SelStart = 0;
    RXRichEdit1.SelLength = Length(RXRichEdit1.Text);
    RXRichEdit1.SelAttributes.Size = FontDialog1.Font.Size;
    RXRichEdit1.SelAttributes.Name = FontDialog1.Font.Name;
    RXRichEdit1.Font.Size = FontDialog1.Font.Size;
    RXRichEdit1.Font.Name = FontDialog1.Font.Name;
    WriteIni;
  end;
end;

procedure TForm1.Calibration1Click(Sender TObject);
begin
  Form6.ShowModal;
end;

procedure TForm1.ComboBox1Change(Sender TObject);
begin
  Speed[1] = get_idx_by_name(ComboBox1.Text);
  init_speed(1);
  windows.setfocus(0);
end;

procedure TForm1.ComboBox1KeyDown(Sender TObject; var Key Word;
  Shift TShiftState);
begin
  key = 0;
  windows.SetFocus(0);
end;

procedure TForm1.ComboBox1KeyPress(Sender TObject; var Key Char);
begin
  key = #0;
  windows.SetFocus(0);
end;

procedure TForm1.ComboBox2Change(Sender TObject);
begin
  Speed[2] = get_idx_by_name(ComboBox2.Text);
  init_speed(2);
  windows.setfocus(0);
end;

procedure TForm1.FormDestroy(Sender TObject);
var
  snd_ch byte;
begin
  stoprx;
  for snd_ch = 1 to 2 do if snd_status[snd_ch] = SND_TX then stoptx(snd_ch);
  if (debugmode and DEBUG_TIMER) = 0 then free_timer1;
  TimerStat2 = TIMER_OFF;
  PTTClose;
  ax25_free;
  agw_free;
  kiss_free;
  detector_free;
  RS.Free;
  waterfall_free;
  WriteIni;
end;

end.
*/