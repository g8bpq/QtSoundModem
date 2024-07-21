#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#define SOCKET int
#include <unistd.h>
#define closesocket close
#endif

//#include "Version.h"

#include "ARDOPC.h"
//#include "getopt.h"

void CompressCallsign(char * Callsign, UCHAR * Compressed);
void CompressGridSquare(char * Square, UCHAR * Compressed);
void  ASCIIto6Bit(char * Padded, UCHAR * Compressed);
void GetTwoToneLeaderWithSync(int intSymLen);
void SendID(BOOL blnEnableCWID);
void PollReceivedSamples();
void CheckTimers();
BOOL GetNextARQFrame();
BOOL TCPHostInit();
BOOL SerialHostInit();
BOOL KISSInit();
void SerialHostPoll();
void TCPHostPoll();
BOOL MainPoll();
VOID PacketStartTX();
void PlatformSleep(int mS);
BOOL BusyDetect2(float * dblMag, int intStart, int intStop);
BOOL IsPingToMe(char * strCallsign);
void LookforPacket(float * dblMag, float dblMagAvg, int count, float * real, float * imag);
void PktARDOPStartTX();

// Config parameters

char GridSquare[9] = "No GS ";
char Callsign[10] = "";
BOOL wantCWID = FALSE;
BOOL NeedID = FALSE;		// SENDID Command Flag
BOOL NeedCWID = FALSE;
BOOL NeedConReq = FALSE;	// ARQCALL Command Flag
BOOL NeedPing = FALSE;		// PING Command Flag
BOOL NeedCQ = FALSE;		// PING Command Flag
BOOL NeedTwoToneTest = FALSE;
BOOL UseKISS = TRUE;			// Enable Packet (KISS) interface
int PingCount;
int CQCount;


BOOL blnPINGrepeating = False;
BOOL blnFramePending = False;	//  Cancels last repeat
int intPINGRepeats = 0;

char ConnectToCall[16] = "";


#ifdef TEENSY
int LeaderLength = 500;
#else
int LeaderLength = 300;
#endif
int TrailerLength = 0;
unsigned int ARQTimeout = 120;
int TuningRange = 100;
int TXLevel = 300;				// 300 mV p-p Used on Teensy
//int RXLevel = 0;				// Configured Level - zero means auto tune
int autoRXLevel = 1500;			// calculated level
int ARQConReqRepeats = 5;
BOOL DebugLog = TRUE;
BOOL CommandTrace = TRUE;
int DriveLevel = 100;
char strFECMode[16] = "OFDM.500.55";
int FECRepeats = 0;
BOOL FECId = FALSE;
int Squelch = 5;

enum _ARQBandwidth ARQBandwidth = XB500;
BOOL NegotiateBW = TRUE;
char HostPort[80] = "";
int port = 8515;
int pktport = 0;
BOOL RadioControl = FALSE;
BOOL SlowCPU = FALSE;
BOOL AccumulateStats = TRUE;
BOOL Use600Modes = FALSE;
BOOL EnableOFDM = TRUE;
BOOL UseOFDM = TRUE;
BOOL FSKOnly = FALSE;
BOOL fastStart = TRUE;
BOOL ConsoleLogLevel = LOGDEBUG;
BOOL FileLogLevel = LOGDEBUG;
BOOL EnablePingAck = TRUE;


// Stats

//    Public Structure QualityStats
  
int SessBytesSent;
int SessBytesReceived;
int int4FSKQuality;
int int4FSKQualityCnts;
int int8FSKQuality;
int int8FSKQualityCnts;
int int16FSKQuality;
int int16FSKQualityCnts;
int intFSKSymbolsDecoded;
int intPSKQuality[2];
int intPSKQualityCnts[2];
int intOFDMQuality[8];
int intOFDMQualityCnts[8];
int intPSKSymbolsDecoded; 
int intOFDMSymbolsDecoded; 

int intQAMQuality;
int intQAMQualityCnts;
int intQAMSymbolsDecoded;
int intGoodQAMSummationDecodes;


int intLeaderDetects;
int intLeaderSyncs;
int intAccumLeaderTracking;
float dblFSKTuningSNAvg;
int intGoodFSKFrameTypes;
int intFailedFSKFrameTypes;
int intAccumFSKTracking;
int intFSKSymbolCnt;
int intGoodFSKFrameDataDecodes;
int intFailedFSKFrameDataDecodes;
int intAvgFSKQuality;
int intFrameSyncs;
int intGoodPSKSummationDecodes;
int intGoodFSKSummationDecodes;
int intGoodQAMSummationDecodes;
int intGoodOFDMSummationDecodes;
float dblLeaderSNAvg;
int intAccumPSKLeaderTracking;
float dblAvgPSKRefErr;
int intPSKTrackAttempts;
int intAccumPSKTracking;
int intQAMTrackAttempts;
int intOFDMTrackAttempts;
int intAccumQAMTracking;
int intAccumOFDMTracking;
int intPSKSymbolCnt;
int intQAMSymbolCnt;
int intOFDMSymbolCnt;
int intGoodPSKFrameDataDecodes;
int intFailedPSKFrameDataDecodes;
int intGoodQAMFrameDataDecodes;
int intFailedQAMFrameDataDecodes;
int intAvgPSKQuality;
int intGoodOFDMFrameDataDecodes;
int intFailedOFDMFrameDataDecodes;
int intAvgOFDMQuality;
float dblAvgDecodeDistance;
int intDecodeDistanceCount;
int	intShiftUPs;
int intShiftDNs;
unsigned int dttStartSession;
int intLinkTurnovers;
int intEnvelopeCors;
float dblAvgCorMaxToMaxProduct;
int intConReqSN;
int intConReqQuality;
int intTimeouts;



char stcLastPingstrSender[10];
char stcLastPingstrTarget[10];
int stcLastPingintRcvdSN;
int stcLastPingintQuality;
time_t stcLastPingdttTimeReceived;

BOOL blnInitializing = FALSE;

BOOL blnLastPTT = FALSE;

BOOL PlayComplete = FALSE;

BOOL blnBusyStatus = 0;
BOOL newStatus;

unsigned int tmrSendTimeout;

int intCalcLeader;        // the computed leader to use based on the reported Leader Length
int intRmtLeaderMeasure = 0;

int dttCodecStarted;

enum _ReceiveState State;
enum _ARDOPState ProtocolState;

const char ARDOPStates[8][9] = {"OFFLINE", "DISC", "ISS", "IRS", "IDLE", "IRStoISS", "FECSEND", "FECRCV"};

const char ARDOPModes[3][6] = {"Undef", "FEC", "ARQ"};

struct SEM Semaphore = {0, 0, 0, 0};

int DecodeCompleteTime;

BOOL blnAbort = FALSE;
int intRepeatCount;
BOOL blnARQDisconnect = FALSE;

int dttLastPINGSent;

enum _ProtocolMode ProtocolMode = FEC;

extern int intTimeouts;
extern BOOL blnEnbARQRpt;
extern BOOL blnDISCRepeating;
extern char strRemoteCallsign[10];
extern char strLocalCallsign[10];
extern char strFinalIDCallsign[10];
extern int dttTimeoutTrip;
extern unsigned int dttLastFECIDSent;
extern BOOL blnPending;
extern unsigned int tmrIRSPendingTimeout;
extern unsigned int tmrFinalID;
extern unsigned int tmrPollOBQueue;
VOID EncodeAndSend4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, int LeaderLength);
void SendPING(char * strMycall, char * strTargetCall, int intRpt);
void SendCQ(int intRpt);

int intRepeatCnt;


BOOL blnClosing = FALSE;
BOOL blnCodecStarted = FALSE;

unsigned int dttNextPlay = 0;


const UCHAR bytValidFrameTypesALL[]=
{
	DataNAK,
	DataNAKLoQ,
	ConRejBusy,
	ConRejBW,
	ConAck,
	DISCFRAME,
	BREAK,
	END,
	IDLEFRAME,
	ConReq200,
	ConReq500,
	ConReq2500,
	OConReq500,
	OConReq2500,
	IDFRAME,
	PINGACK,
	PING,	
	CQ_de,
	D4PSK_200_50_E,
	D4PSK_200_50_O,
	D4PSK_200_100_E,
	D4PSK_200_100_O,
	D16QAM_200_100_E,
	D16QAM_200_100_O,
	D4FSK_500_50_E,
	D4FSK_500_50_O,
	D4PSK_500_50_E,
	D4PSK_500_50_O,
	D4PSK_500_100_E,
	D4PSK_500_100_O,
	D16QAMR_500_100_E,
	D16QAMR_500_100_O,
	D16QAM_500_100_E,
	D16QAM_500_100_O,
	DOFDM_200_55_E,
	DOFDM_200_55_O,
	DOFDM_500_55_E,
	DOFDM_500_55_O,
	D4FSK_1000_50_E,
	D4FSK_1000_50_O,
	D4PSKR_2500_50_E,
	D4PSKR_2500_50_O,
	D4PSK_2500_50_E,
	D4PSK_2500_50_O,
	D4PSK_2500_100_E,
	D4PSK_2500_100_O,
	D16QAMR_2500_100_E,
	D16QAMR_2500_100_O,
	D16QAM_2500_100_E,
	D16QAM_2500_100_O,
	DOFDM_2500_55_E,
	DOFDM_2500_55_O,

	PktFrameHeader,	// Variable length frame Header
	PktFrameData,	// Variable length frame Data (Virtual Frsme Type)
	OFDMACK,
	DataACK, 	
	DataACKHiQ};

const UCHAR bytValidFrameTypesISS[]=		// ACKs, NAKs, END, DISC, BREAK
{
	DataNAK,
	DataNAKLoQ,
	ConRejBusy,
	ConRejBW,
	ConAck,
	DISCFRAME,
	END,
	IDFRAME,
	PktFrameHeader,	// Variable length frame Header
	PktFrameData,	// Variable length frame Data (Virtual Frsme Type)
	OFDMACK,
	DataACK, 	
	DataACKHiQ};

const UCHAR * bytValidFrameTypes;

int bytValidFrameTypesLengthISS = sizeof(bytValidFrameTypesISS);
int bytValidFrameTypesLengthALL = sizeof(bytValidFrameTypesALL);
int bytValidFrameTypesLength;


BOOL blnTimeoutTriggered = FALSE;

//	We can't keep the audio samples for retry, but we can keep the
//	encoded data

unsigned char bytEncodedBytes[4500] ="";		// OFDM is big (maybe not 4500)
int EncLen;


extern UCHAR bytSessionID;

int intLastRcvdFrameQuality;

int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 

const char strAllDataModes[18][16] =
		{"4PSK.200.50", "4PSK.200.100",
		"16QAM.200.100", "4FSK.500.50", 
		"4PSK.500.50", "4PSK.500.100",
		"OFDM.200.55", "OFDM.500.55", 
		"16QAMR.500.100", "16QAM.500.100",
		"4FSK.1000.50", 
		"4PSKR.2500.50", "4PSK.2500.50", 
		"4PSK.2500.100", 
		"16QAMR.2500.100", "16QAM.2500.100", "OFDM.2500.55"};

int strAllDataModesLen = 18;

// Frame Speed By Type (from Rick's spreadsheet) Bytes per minute

const short Rate[64] = 
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 00 - 0F
	402,402,826,826,1674,1674,0,0,0,0,402,402,857,857,1674,1674,	// 10 - 1F
	1674,1674,3349,3359,0,0,0,0,857,857,2143,2143,4286,4286,8372,8372,	// 20 - 2F
	8372,8372,16744,16744,0,0,0,0,0,0,0,0,0,0,0,0,	// 30 - 3F
};

const short FrameSize[64] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 00 - 0F
	32,32,64,64,120,120,0,0,0,0,32,32,64,64,128,128,	// 10 - 1F
	120,120,240,240,360,360,720,720,64,64,160,160,320,320,640,640,	// 20 - 2F
	600,600,1200,1200,680,680,1360,1360,0,0,0,0,0,0,0,0,	// 30 - 3F
};


const char strFrameType[64][18] =
{
	//	Short Control Frames 1 Car, 500Hz,4FSK, 50 baud 
        
	"DataNAK",			// 0
	"DataNAKLoQ",
	"ConRejBusy",
	"ConRejBW",
	"ConAck",			// 4
	"DISC",
	"BREAK",
	"END",
	"IDLE",				// 8
	"ConReq200",
	"ConReq500",
	"ConReq2500",
	"IDFrame",			// C
	"PingAck",
	"Ping",				// E
	"CQ_de",			// F
	
	//	200 Hz Bandwidth 
	//	1 Car modes
	
	"4PSK.200.50.E",	// 0x10
	"4PSK.200.50.O",
	"4PSK.200.100.E",
	"4PSK.200.100.O",
	"16QAM.200.100.E",	// 0x14
	"16QAM.200.100.O",
	"", "",// 0x16 to 0x17
	"OConReq500", "OConReq2500", 
	//	500 Hz bandwidth Data 
	//	1 Car 4FSK Data mode 500 Hz, 50 baud tones spaced @ 100 Hz 

	"4FSK.500.50.E",	// 0x1A
	"4FSK.500.50.O",
	//	2 Car PSK Data Modes 
	"4PSK.500.50.E",
	"4PSK.500.50.O",
	"4PSK.500.100.E",	// 0x1E
	"4PSK.500.100.O",
	//	2 Car 16QAM Data Modes 100 baud
	"16QAMR.500.100.E",	// 0x20
	"16QAMR.500.100.O",
	//	2 Car 16QAM Data Modes 100 baud
	"16QAM.500.100.E",	// 0x22
	"16QAM.500.100.O",
	"OFDM.500.55.E",
	"OFDM.500.55.O",
	"OFDM.200.55.E",
	"OFDM.200.55.O",

	//	1 Khz Bandwidth Data Modes 
	//	4 Car 4FSK Data mode 1000 Hz, 50 baud tones spaced @ 100 Hz 
	"4FSK.1000.50.E",	// 0x28
	"4FSK.1000.50.O",

	//	2500 dblOffsetHz bandwidth modes
	//	10 Car PSK Data Modes 50 baud

	"4PSKR.2500.50.E",	// 0x2A
	"4PSKR.2500.50.O",
	"4PSK.2500.50.E",
	"4PSK.2500.50.O",

	//	10 Car PSK Data Modes 100 baud

	"4PSK.2500.100.E",	// 0x2E
	"4PSK.2500.100.O",


	//	10 Car 16QAM Data modes 100 baud  
	"16QAMR.2500.100.E",	// 0x30
	"16QAMR.2500.100.O",
	"16QAM.2500.100.E",	// 0x32
	"16QAM.2500.100.O",
	"OFDM.2500.55.E",
	"OFDM.2500.55.O",
	"",
	"",
	"", "", // 0x38 to 0x39
	"PktFrameHeader",	//3A	
	"PktFrameData",
	"",					// 0x3C 
	"OFDMACK",
	"DataACK",		 // note special coding to have large distance from NAKs
	"DataACKHiQ"	 // note special coding to have large distance from NAKs
};

const char shortFrameType[64][12] =
{
	//	Short Control Frames 1 Car, 500Hz,4FSK, 50 baud 
	//	Used on OLED display
        
	"DataNAK",			// 0
	"DataNAKLoQ",
	"ConRejBusy",
	"ConRejBW",
	"ConAck",			// 4
	"DISC",
	"BREAK",
	"END",
	"IDLE",				// 8
	"ConReq200",
	"ConReq500",
	"ConReq2500",
	"IDFrame",			// C
	"PingAck",
	"Ping",				// E
	"CQ_de",			// F
	
	//	200 Hz Bandwidth 
	//	1 Car modes
	
	"4P.200.50",	// 0x10
	"4P.200.50",
	"4P.200.100",
	"4PS.200.100",
	"16Q.200.100",	// 0x14
	"16Q.200.100",
	"", "",// 0x16 to 0x17
	"OConReq500", "OConReq2500", 

	//	500 Hz bandwidth Data 
	//	1 Car 4FSK Data mode 500 Hz, 50 baud tones spaced @ 100 Hz 

	"4F.500.50",	// 0x1A
	"4F.500.50",
	//	2 Car PSK Data Modes 
	"4P.500.50",
	"4P.500.50",
	"4P.500.100",	// 0x1E
	"4P.500.100",
	//	2 Car 16QAM Data Modes 100 baud
	"16QR.500.100",	// 0x20
	"16QR.500.100",
	//	2 Car 16QAM Data Modes 100 baud
	"16Q.500.100",	// 0x22
	"16Q.500.100",
	"OFDM.500",
	"OFDM.500",
	"OFDM.200",
	"OFDM.200",

	//	1 Khz Bandwidth Data Modes 
	//	4 Car 4FSK Data mode 1000 Hz, 50 baud tones spaced @ 100 Hz 
	"4F.1K.50",	// 0x28
	"4F.1K.50",

	//	2500 dblOffsetHz bandwidth modes
	//	10 Car PSK Data Modes 50 baud

	"4PR.2500.50",	// 0x2A
	"4PR.2500.50",
	"4P.2500.50",
	"4P.2500.50",

	//	10 Car PSK Data Modes 100 baud

	"4P.2500.100",	// 0x2E
	"4P.2500.100",


	//	10 Car 16QAM Data modes 100 baud  
	"QR.2500.100",	// 0x30
	"QR.2500.100",
	"Q.2500.100",	// 0x32
	"Q.2500.100",
	"OFDM.2500",
	"OFDM.2500",
	"",
	"",
	"", "", // 0x38 to 0x39
	"PktHeader",	//3A	
	"PktData",
	"",		// 0x3C
	"OFDMACK",
	"DataACK",		 // note special coding to have large distance from NAKs
	"DataACKHiQ"	 // note special coding to have large distance from NAKs
};



void GetSemaphore()
{
}

void FreeSemaphore()
{
}

BOOL CheckValidCallsignSyntax(char * strCallsign)
{
	// Function for checking valid call sign syntax

	char * Dash = strchr(strCallsign, '-');
	int callLen = strlen(strCallsign);
	char * ptr = strCallsign;
	int SSID;

	if (Dash)
	{
		callLen = Dash - strCallsign;

		SSID = atoi(Dash + 1);
		if (SSID > 15)
			return FALSE;

		if (strlen(Dash + 1) > 2)
			return FALSE;

		if (!isalnum(*(Dash + 1)))
			return FALSE;
	}
		
	if (callLen > 7 || callLen < 3)
			return FALSE;

	while (callLen--)
	{
		if (!isalnum(*(ptr++)))
			return FALSE;
	}
	return TRUE;
}

//	 Function to check for proper syntax of a 4, 6 or 8 character GS

BOOL CheckGSSyntax(char * GS)
{
	int Len = strlen(GS);

	if (!(Len == 4 || Len == 6 || Len == 8))
		return FALSE;

	if (!isalpha(GS[0]) || !isalpha(GS[1]))
		return FALSE;
	
	if (!isdigit(GS[2]) || !isdigit(GS[3]))
		return FALSE;

	if (Len == 4)
		return TRUE;

	if (!isalpha(GS[4]) || !isalpha(GS[5]))
		return FALSE;

	if (Len == 6)
		return TRUE;

	if (!isdigit(GS[6]) || !isdigit(GS[7]))
		return FALSE;

	return TRUE;
}

// Function polled by Main polling loop to see if time to play next wave stream
   
#ifdef WIN32

extern LARGE_INTEGER Frequency;
extern LARGE_INTEGER StartTicks;
extern LARGE_INTEGER NewTicks;

#endif

extern int NErrors;

void testRS()
{
	// feed random data into RS to check robustness

	BOOL blnRSOK, FrameOK;
	char bytRawData[256];
	int DataLen = 128;
	int intRSLen = 64;
	int i;

	for (i = 0; i < DataLen; i++)
	{
		bytRawData[i] = rand() % 256;
	}

	FrameOK = RSDecode(bytRawData, DataLen, intRSLen, &blnRSOK);
}


void SendCWID(char * Callsign, BOOL x)
{
}

// Subroutine to generate 1 symbol of leader

//	 returns pointer to Frame Type Name

const char * Name(UCHAR bytID)
{
	return strFrameType[bytID];
}

//	 returns pointer to Frame Type Name

const char * shortName(UCHAR bytID)
{
	return shortFrameType[bytID];
}
// Function to look up frame info from bytFrameType

BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
			   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType)
{
	// Used to "lookup" all parameters by frame Type. 
	// returns TRUE if all fields updated otherwise FALSE (improper bytFrameType)

	// 1 Carrier 4FSK control frames 

	switch(bytFrameType)
	{
	case DataNAK:
	case DataNAKLoQ:
	case DataACK:
	case DataACKHiQ:
	case ConAck:
	case ConRejBW:
	case ConRejBusy:
	case IDLEFRAME:
	case DISCFRAME:
	case BREAK:
	case END:

		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		break;

	case IDFRAME:
	case PING:
	case CQ_de:

		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 12;
		*intRSLen = 4;			// changed 0.8.0
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		break;

	case PINGACK:
	
		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 3;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		break;


	case ConReq200:
	case ConReq500:
	case ConReq2500:
	case OConReq500:
	case OConReq2500:

		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 6;
		*intRSLen = 2;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		break;
	
	case OFDMACK:
	
		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 6;
		*intRSLen = 4;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		break;

	case PktFrameHeader:

		// Special Variable Length frame

		// This defines the header, 4PSK.500.100. Length is 6 bytes
		// Once we have that we receive the rest of the packet in the 
		// mode defined in the header.
		// Header is 4 bits Type 12 Bits Len 2 bytes CRC 2 bytes RS

		*blnOdd = 0;
		*intNumCar = 1;
		*intDataLen = 2;
		*intRSLen = 2;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
 		break;

	case PktFrameData:

		// Special Variable Length frame

		// This isn't ever transmitted but is used to define the
		// current setting for the data frame. Mode and Length 
		// are variable
		

		*blnOdd = 1;
		*intNumCar = pktCarriers[pktMode];
		*intDataLen = pktDataLen;
		*intRSLen = pktRSLen;
		strcpy(strMod, &pktMod[pktMode][0]);
		strlop(strMod, '/');
		*intBaud = 100;
 		break;

	default:

		// Others are Even/Odd Pairs

		switch(bytFrameType & 0xFE)
		{
		case D4PSK_200_50_E:
	
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 1;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4PSK");
			*intBaud = 50;
			break;

		case D4PSK_200_100_E:
	
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 1;
			*intDataLen = 64;
			*intRSLen = 16;
			strcpy(strMod, "4PSK");
			*intBaud = 100;
			break;

 
		case D16QAM_200_100_E:
	
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 1;
			*intDataLen = 120;
			*intRSLen = 40;
			strcpy(strMod, "16QAM");
			*intBaud = 100;
			break;


		case D4FSK_500_50_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 1;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4FSK");
			*intBaud = 50;
			*bytQualThres = 30;	
			break;

		case D4PSK_500_50_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 2;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4PSK");
			*intBaud = 50;
			break;

		case D4PSK_500_100_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 2;
			*intDataLen = 64;
			*intRSLen = 16;
			strcpy(strMod, "4PSK");
			*intBaud = 100;
			break;

 		case D16QAMR_500_100_E:
	
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 2;
			*intDataLen = 120;
			*intRSLen = 40;
			strcpy(strMod, "16QAMR");
			*intBaud = 100;
			break;

  		case D16QAM_500_100_E:
	
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 2;
			*intDataLen = 120;
			*intRSLen = 40;
			strcpy(strMod, "16QAM");
			*intBaud = 100;
			break;

		case DOFDM_200_55_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 3;
			*intDataLen = 40;
			*intRSLen = 10;
			strcpy(strMod, "OFDM");
			*intBaud = 55;
			break;

		case DOFDM_500_55_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 9;
			*intDataLen = 40;
			*intRSLen = 10;
			strcpy(strMod, "OFDM");
			*intBaud = 55;
			break;

		case D4FSK_1000_50_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 2;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4FSK");
			*intBaud = 50;
			break;

		case D4PSKR_2500_50_E:
 
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 10;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4PSKR");
			*intBaud = 50;	
			break;

		case D4PSK_2500_50_E:
 
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 10;
			*intDataLen = 32;
			*intRSLen = 8;
			strcpy(strMod, "4PSK");
			*intBaud = 50;	
			break;

		case D4PSK_2500_100_E:
			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 10;
			*intDataLen = 64;
			*intRSLen = 16;
			strcpy(strMod, "4PSK");
			*intBaud = 100;	
			break;

		case D16QAMR_2500_100_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 10;
			*intDataLen = 120;
			*intRSLen = 40;
			strcpy(strMod, "16QAMR");
			*intBaud = 100;	
			break;

		case D16QAM_2500_100_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 10;
			*intDataLen = 120;
			*intRSLen = 40;
			strcpy(strMod, "16QAM");
			*intBaud = 100;	
			break;

		case DOFDM_2500_55_E:

			*blnOdd = (1 & bytFrameType) != 0;
			*intNumCar = 43;
			*intDataLen = 40;
			*intRSLen = 10;
			strcpy(strMod, "OFDM");
			*intBaud = 55;
			break;

		default:
			Debugprintf("No data for frame type= %X",  bytFrameType);
			return FALSE;
		}
	}	
	
	strcpy(strType,strFrameType[bytFrameType]);

	return TRUE;
}

int xNPAR = -1;	// Number of Parity Bytes - used in RS Code

int MaxErrors = 0;

int xRSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen)
{
	// This just returns the Parity Bytes. I don't see the point
	// in copying the message about

	unsigned char Padded[256];		// The padded Data

	int Length = DataLen + RSLen;	// Final Length of packet
	int PadLength = 255 - Length;	// Padding bytes needed for shortened RS codes

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	if (NPAR != RSLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = RSLen;
		MaxErrors = NPAR / 2;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen); 

	encode_data(Padded, 255-RSLen, RSBytes);

	return RSLen;
}

//	Main RS decode function

extern int index_of[];
extern int recd[];
int Corrected[256];
extern int tt;		//  number of errors that can be corrected 
extern int kk;		// Info Symbols

BOOL blnErrorsCorrected;

#define NEWRS
	
//  Function to encode ConnectRequest frame 

//	' Function to encode an ACK control frame  (2 bytes total) ...with 5 bit Quality code 


void SendID(BOOL blnEnableCWID)
{

}


void  ASCIIto6Bit(char * Padded, UCHAR * Compressed)
{
	// Input must be 8 bytes which will convert to 6 bytes of packed 6 bit characters and
	// inputs must be the ASCII character set values from 32 to 95....
    
	unsigned long long intSum = 0;

	int i;

	for (i=0; i<4; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[0] = (UCHAR)(intSum >> 16) & 255;
	Compressed[1] = (UCHAR)(intSum >> 8) &  255;
	Compressed[2] = (UCHAR)intSum & 255;

	intSum = 0;

	for (i=4; i<8; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[3] = (UCHAR)(intSum >> 16) & 255;
	Compressed[4] = (UCHAR)(intSum >> 8) &  255;
	Compressed[5] = (UCHAR)intSum & 255;
}

void Bit6ToASCII(UCHAR * Padded, UCHAR * UnCompressed)
{
	// uncompress 6 to 8

	// Input must be 6 bytes which represent packed 6 bit characters that well 
	// result will be 8 ASCII character set values from 32 to 95...

	unsigned long long intSum = 0;

	int i;

	for (i=0; i<3; i++)
	{
		intSum = (intSum << 8) + Padded[i];
	}

	UnCompressed[0] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[1] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[2] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[3] = (UCHAR)(intSum & 63) + 32;

	intSum = 0;

	for (i=3; i<6; i++)
	{
		intSum = (intSum << 8) + Padded[i] ;
	}

	UnCompressed[4] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[5] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[6] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[7] = (UCHAR)(intSum & 63) + 32;
}


// Function to compress callsign (up to 7 characters + optional "-"SSID   (-0 to -15 or -A to -Z) 
    
void CompressCallsign(char * inCallsign, UCHAR * Compressed)
{
	char Callsign[10] = "";
	char Padded[16];
	int SSID;
	char * Dash;

	memcpy(Callsign, inCallsign, 10);
	Dash = strchr(Callsign, '-');
	
	if (Dash == 0)		// if No SSID
	{
		strcpy(Padded, Callsign);
		strcat(Padded, "    ");
		Padded[7] = '0';			//  "0" indicates no SSID
	}
	else
	{
		*(Dash++) = 0;
		SSID = atoi(Dash);

		strcpy(Padded, Callsign);
		strcat(Padded, "    ");

		if (SSID >= 10)		// ' handles special case of -10 to -15 : ; < = > ? '
			Padded[7] = ':' + SSID - 10;
		else
			Padded[7] = *(Dash);
	}

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to compress Gridsquare (up to 8 characters)

void CompressGridSquare(char * Square, UCHAR * Compressed)
{
	char Padded[17];
        
	if (strlen(Square) > 8)
		return;

	strcpy(Padded, Square);
	strcat(Padded, "        ");

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
  
void DeCompressCallsign(char * bytCallsign, char * returned)
{
	char bytTest[10] = "";
	char SSID[8] = "";
    
	Bit6ToASCII(bytCallsign, bytTest);

	memcpy(returned, bytTest, 7);
	returned[7] = 0;
	strlop(returned, ' ');		// remove trailing space

	if (bytTest[7] == '0') // Value of "0" so No SSID
		returned[6] = 0;
	else if (bytTest[7] >= 58 && bytTest[7] <= 63) //' handles special case for -10 to -15
		sprintf(SSID, "-%d", bytTest[7] - 48);
	else
		sprintf(SSID, "-%c", bytTest[7]);
	
	strcat(returned, SSID);
}


// Function to decompress 6 byte Grid square to 4, 6 or 8 characters

void DeCompressGridSquare(char * bytGS, char * returned)
{
	char bytTest[10] = "";
	Bit6ToASCII(bytGS, bytTest);

	strlop(bytTest, ' ');
	strcpy(returned, bytTest);
}

// A function to compute the parity symbol used in the frame type encoding

UCHAR ComputeTypeParity(UCHAR bytFrameType)
{
	UCHAR bytMask = 0x30;		// only using 6 bits
	UCHAR bytParitySum = 3;
	UCHAR bytSym = 0;
	int k;

	for (k = 0; k < 3; k++)
	{
		bytSym = (bytMask & bytFrameType) >> (2 * (2 - k));
		bytParitySum = bytParitySum ^ bytSym;
		bytMask = bytMask >> 2;
	}
    
	return bytParitySum & 0x3;
}

// Function to look up the byte value from the frame string name

UCHAR FrameCode(char * strFrameName)
{
	int i;

    for (i = 0; i < 64; i++)
	{
		if (strcmp(strFrameType[i], strFrameName) == 0)
		{
			return i;
		}
	}
	return 0;
}

unsigned int GenCRC16(unsigned char * Data, unsigned short length)
{
	// For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-0xFFFF

	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j < length; j++)	
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}
 
	return intRegister;
}

BOOL checkcrc16(unsigned char * Data, unsigned short length)
{
	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length - 2); j++)		// ' 2 bytes short of data length
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}

    if (Data[length - 2] == intRegister >> 8)
		if (Data[length - 1] == (intRegister & 0xFF))
			return TRUE;
   
	return FALSE;
}


//	Subroutine to compute a 16 bit CRC value and append it to the Data... With LS byte XORed by bytFrameType
    
void GenCRC16FrameType(char * Data, int Length, UCHAR bytFrameType)
{
	unsigned int CRC = GenCRC16(Data, Length);

	// Put the two CRC bytes after the stop index

	Data[Length++] = (CRC >> 8);		 // MS 8 bits of Register
	Data[Length] = (CRC & 0xFF) ^ bytFrameType;  // LS 8 bits of Register
}

// Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) XORing LS byte with bytFrameType..
 
unsigned short int compute_crc(unsigned char *buf,int len);

BOOL  CheckCRC16FrameType(unsigned char * Data, int Length, UCHAR bytFrameType)
{
	// returns TRUE if CRC matches, else FALSE
    // For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-0xFFFF

	unsigned int CRC = GenCRC16(Data, Length);
	unsigned short CRC2 =  compute_crc(Data, Length);
	CRC2 ^= 0xffff;
  
	// Compare the register with the last two bytes of Data (the CRC) 
    
	if ((CRC >> 8) == Data[Length])
		if (((CRC & 0xFF) ^ bytFrameType) == Data[Length + 1])
			return TRUE;

	return FALSE;
}


void SaveQueueOnBreak()
{
	// Save data we are about to remove from TX buffer
}


extern UCHAR bytEchoData[];		// has to be at least max packet size (?1280)

extern int bytesEchoed;

extern UCHAR DelayedEcho;


// Timer Rotines

void CheckTimers()
{

}

// Main polling Function returns True or FALSE if closing 

int dttLastBusy;
int dttLastClear;
int dttStartRTMeasure;
extern int intLastStart;
extern int intLastStop;
float dblAvgBaselineSlow;
float dblAvgBaselineFast;
float dblAvgPk2BaselineRatio;

//  Functino to extract bandwidth from ARQBandwidth


//	 Function to implement a busy detector based on 1024 point FFT
 /*
BOOL BusyDetect(float * dblMag, int intStart, int intStop)
{
	// this only called while searching for leader ...once leader detected, no longer called.
	// First look at simple peak over the frequency band of  interest.
	// Status:  May 28, 2014.  Some initial encouraging results. But more work needed.
	//       1) Use of Start and Stop ranges good and appear to work well ...may need some tweaking +/_ a few bins.
	//       2) Using a Fast attack and slow decay for the dblAvgPk2BaselineRation number e.g.
	//       dblAvgPk2BaselineRatio = Max(dblPeakPwrAtFreq / dblAvgBaselineX, 0.9 * dblAvgPk2BaselineRatio)
	// Seems to work well for the narrow detector. Tested on real CW, PSK, RTTY. 
	// Still needs work on the wide band detector. (P3, P4 etc)  May be issues with AGC speed. (my initial on-air tests using AGC Fast).
	// Ideally can find a set of parameters that won't require user "tweaking"  (Busy Squelch) but that is an alternative if needed. 
	// use of technique of re initializing some parameters on a change in detection bandwidth looks good and appears to work well with 
	// scanning.  Could be expanded with properties to "read back" these parameters so they could be saved and re initialize upon each new frequency. 

	static int intBusyCountPkToBaseline = 0;
	static int intBusyCountFastToSlow = 0;
	static int intBusyCountSlowToFast = 0;
	static BOOL blnLastBusy = FALSE;

	float dblAvgBaseline = 0;
	float dblPwrAtPeakFreq = 0;
	float dblAvgBaselineX;
	float dblAlphaBaselineSlow = 0.1f; // This factor affects the filtering of baseline slow. smaller = slower filtering
	float dblAlphaBaselineFast = 0.5f; // This factor affects the filtering of baseline fast. smaller = slower filtering
	int intPkIndx = 0;
	float dblFSRatioNum, dblSFRatioNum;
	BOOL  blnFS, blnSF, blnPkBaseline;
	int i;

	// This holds off any processing of data until 100 ms after PTT release to allow receiver recovery.
      
	if (Now - dttStartRTMeasure < 100)
		return blnLastBusy;

	for (i = intStart; i <= intStop; i++)	 // cover a range that matches the bandwidth expanded (+/-) by the tuning range
	{
		if (dblMag[i] > dblPwrAtPeakFreq)
		{
			dblPwrAtPeakFreq = dblMag[i];
			intPkIndx = i;
		}
		dblAvgBaseline += dblMag[i];
	}
     
	if (intPkIndx == 0)
		return 0;
	
	// add in the 2 bins above and below the peak (about 59 Hz total bandwidth)
	// This needs refinement for FSK mods like RTTY which have near equal peaks making the Peak and baseline on strong signals near equal
	// Computer the power within a 59 Hz spectrum around the peak

	dblPwrAtPeakFreq += (dblMag[intPkIndx - 2] + dblMag[intPkIndx - 1]) + (dblMag[intPkIndx + 2] + dblMag[intPkIndx + 1]);
	dblAvgBaselineX = (dblAvgBaseline - dblPwrAtPeakFreq) / (intStop - intStart - 5);  // the avg Pwr per bin ignoring the 59 Hz area centered on the peak
	dblPwrAtPeakFreq = dblPwrAtPeakFreq / 5;  //the the average Power (per bin) in the region of the peak (peak +/- 2bins...about 59 Hz)

	if (intStart == intLastStart && intStop == intLastStop)
	{
		dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX;
		dblAvgBaselineSlow = (1 - dblAlphaBaselineSlow) * dblAvgBaselineSlow + dblAlphaBaselineSlow * dblAvgBaseline;
		dblAvgBaselineFast = (1 - dblAlphaBaselineFast) * dblAvgBaselineFast + dblAlphaBaselineFast * dblAvgBaseline;
	}
	else
	{
		// This initializes the values after a bandwidth change

		dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX;
		dblAvgBaselineSlow = dblAvgBaseline;
		dblAvgBaselineFast = dblAvgBaseline;
		intLastStart = intStart;
		intLastStop = intStop;
	}
	
	if (Now - dttLastBusy < 1000 ||  ProtocolState != DISC)	// Why??
		return blnLastBusy;
	
	if (dblAvgPk2BaselineRatio > 1.118f * powf(Squelch, 1.5f))   // These values appear to work OK but may need optimization April 21, 2015
	{
		blnPkBaseline = TRUE;
		dblAvgBaselineSlow = dblAvgBaseline;
		dblAvgBaselineFast = dblAvgBaseline;
	}
	else
	{
       // 'If intBusyCountPkToBaseline > 0 Then

		blnPkBaseline = FALSE;
	}
	
	// This detects wide band "pulsy" modes like Pactor 3, MFSK etc

	switch(Squelch)		 // this provides a modest adjustment to the ratio limit based on practical squelch values
	{
		//These values yield less sensiivity for F:S which minimizes trigger by static crashes but my need further optimization May 2, 2015

	case 0:
	case 1:
	case 2:
		dblFSRatioNum = 1.9f;
		dblSFRatioNum = 1.2f;
		break;
		
	case 3:
		dblFSRatioNum = 2.1f;
		dblSFRatioNum = 1.4f;
		break;
	case 4:
		dblFSRatioNum = 2.3f;
		dblSFRatioNum = 1.6f;
		break;
	case 5:
		dblFSRatioNum = 2.5f;
		dblSFRatioNum = 1.8f;
		break;
	case 6:
		dblFSRatioNum = 2.7f;
		dblSFRatioNum = 2.0f;
	case 7:
		dblFSRatioNum = 2.9f;
		dblSFRatioNum = 2.2f;
	case 8:
	case 9:
	case 10:
        dblFSRatioNum = 3.1f;
		dblSFRatioNum = 2.4f;
	}
	
	// This covers going from Modulation to no modulation e.g. End of Pactor frame

	if ((dblAvgBaselineSlow / dblAvgBaselineFast) > dblSFRatioNum)
	
		//Debug.WriteLine("     Slow to Fast")
		blnSF = TRUE;
	else
		blnSF = FALSE;
	
	//  This covers going from no modulation to Modulation e.g. Start of Pactor Frame or Static crash
	
	if ((dblAvgBaselineFast / dblAvgBaselineSlow) > dblFSRatioNum)
         //Debug.WriteLine("     Fast to Slow")
		blnFS = TRUE;
	else
		blnFS = FALSE;

	if (blnFS || blnSF || blnPkBaseline)
	{
		//'If blnFS Then Debug.WriteLine("Busy: Fast to Slow")
		//'If blnSF Then Debug.WriteLine("Busy: Slow to Fast")
		//'If blnPkBaseline Then Debug.WriteLine("Busy: Pk to Baseline")
		blnLastBusy = TRUE;
		dttLastBusy = Now;
		return TRUE;
	}
	else
	{
		blnLastBusy = FALSE;
		dttLastClear = Now;
        return FALSE;
	}
	return blnLastBusy;
}
*/
//	Subroutine to update the Busy detector when not displaying Spectrum or Waterfall (graphics disabled)
 		

extern UCHAR CurrentLevel;

#ifdef PLOTSPECTRUM		
float dblMagSpectrum[206];
float dblMaxScale = 0.0f;
extern UCHAR Pixels[4096];
extern UCHAR * pixelPointer;
#endif


/* Old Version pre gui

void UpdateBusyDetector(short * bytNewSamples)
{
	float dblReF[1024];
	float dblImF[1024];

	float dblMag[206];
	
	static BOOL blnLastBusyStatus;
	
	float dblMagAvg = 0;
	int intTuneLineLow, intTuneLineHi, intDelta;
	int i;

	if (ProtocolState != DISC)		// ' Only process busy when in DISC state
		return;

//	if (State != SearchingForLeader)
//		return;						// only when looking for leader

	if (Now - LastBusyCheck < 100)
		return;

	LastBusyCheck = Now;

	FourierTransform(1024, bytNewSamples, &dblReF[0], &dblImF[0], FALSE);

	for (i = 0; i <  206; i++)
	{
		//	starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)
            
		dblMag[i] = powf(dblReF[i + 25], 2) + powf(dblImF[i + 25], 2);	 // first pass 
		dblMagAvg += dblMag[i];
	}

//	LookforPacket(dblMag, dblMagAvg, 206, &dblReF[25], &dblImF[25]);
//	packet_process_samples(bytNewSamples, 1200);

	intDelta = (ExtractARQBandwidth() / 2 + TuningRange) / 11.719f;

	intTuneLineLow = max((103 - intDelta), 3);
	intTuneLineHi = min((103 + intDelta), 203);
    
//	if (ProtocolState == DISC)		// ' Only process busy when in DISC state
	{
		blnBusyStatus = BusyDetect3(dblMag, intTuneLineLow, intTuneLineHi);
		
		if (blnBusyStatus && !blnLastBusyStatus)
		{
			QueueCommandToHost("BUSY TRUE");
         	newStatus = TRUE;				// report to PTC
		}
		//    stcStatus.Text = "True"
            //    queTNCStatus.Enqueue(stcStatus)
            //    'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
			
		else if (blnLastBusyStatus && !blnBusyStatus)
		{
			QueueCommandToHost("BUSY FALSE");
			newStatus = TRUE;				// report to PTC
		} 
		//    stcStatus.Text = "False"
        //    queTNCStatus.Enqueue(stcStatus)
        //    'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		blnLastBusyStatus = blnBusyStatus;
	}
}

*/

// Function to encode data for all PSK frame types

int EncodePSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes)
{
	// Objective is to use this to use this to send all PSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 100, 167
	//    intPSKMode may be 4 (4PSK) or 8 (8PSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, bytDataToSendLengthPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[18];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedBytes[2];

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}

	//	Generate the 2 bytes for the frame type data:

	bytEncodedBytes[0] = bytFrameType;
	bytEncodedBytes[1] = bytFrameType ^ bytSessionID;

	bytDataToSendLengthPtr = 0;
	intEncodedDataPtr = 2;

	// Now compute the RS frame for each carrier in sequence and move it to bytEncodedBytes 

	if (strchr(strMod, 'R'))
	{
		// Robust Frame. We send data twice, so only encode half the carriers

		intNumCar /= 2;
	}

	for (i = 0; i < intNumCar; i++)		//  across all carriers
	{
		intCarDataCnt = Length - bytDataToSendLengthPtr;

		if (intCarDataCnt > intDataLen) // why not > ??
		{
			// Won't all fit 

			bytToRS[0] = intDataLen;
			intStartIndex = intEncodedDataPtr;
			memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen);
			bytDataToSendLengthPtr += intDataLen;
		}
		else
		{
			// Last bit

			memset(&bytToRS[0], 0, intDataLen);

			bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

			if (intCarDataCnt > 0)
			{
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
				bytDataToSendLengthPtr += intCarDataCnt;
			}
		}

		GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

		RSEncode(bytToRS, bytToRS + intDataLen + 3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total

		//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

		intEncodedDataPtr += intDataLen + 3 + intRSLen;

		bytToRS += intDataLen + 3 + intRSLen;
	}

	if (strchr(strMod, 'R'))
	{
		// Robust Frame. We send data twice, so copy data

		memcpy(&bytEncodedBytes[intEncodedDataPtr], &bytEncodedBytes[2], intEncodedDataPtr - 2);
		intEncodedDataPtr += intEncodedDataPtr - 2;
	}


	return intEncodedDataPtr;
}



// Function to encode data for all FSK frame types

int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes)
{
	// Objective is to use this to use this to send all 4FSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 50, 100
	//    strMod is 4FSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, bytDataToSendLengthPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[18];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedBytes[2];

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}

	//	Generate the 2 bytes for the frame type data:

	bytEncodedBytes[0] = bytFrameType;
	bytEncodedBytes[1] = bytFrameType ^ bytSessionID;

	//   Dim bytToRS(intDataLen + 3 - 1) As Byte ' Data + Count + 2 byte CRC

	bytDataToSendLengthPtr = 0;
	intEncodedDataPtr = 2;

	if (intBaud < 600 || intDataLen < 600)
	{
		// Now compute the RS frame for each carrier in sequence and move it to bytEncodedBytes 

		for (i = 0; i < intNumCar; i++)		//  across all carriers
		{
			intCarDataCnt = Length - bytDataToSendLengthPtr;

			if (intCarDataCnt >= intDataLen) // why not > ??
			{
				// Won't all fit 

				bytToRS[0] = intDataLen;
				intStartIndex = intEncodedDataPtr;
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen);
				bytDataToSendLengthPtr += intDataLen;
			}
			else
			{
				// Last bit

				bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

				if (intCarDataCnt > 0)
				{
					memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
					bytDataToSendLengthPtr += intCarDataCnt;
				}
			}

			GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

			RSEncode(bytToRS, bytToRS + intDataLen + 3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total

			//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

			intEncodedDataPtr += intDataLen + 3 + intRSLen;

			bytToRS += intDataLen + 3 + intRSLen;
		}
		return intEncodedDataPtr;
	}

	// special case for 600 baud 4FSK which has 600 byte data field sent as three sequencial (200 byte + 50 byte RS) groups

	for (i = 0; i < 3; i++)		 // for three blocks of RS data
	{
		intCarDataCnt = Length - bytDataToSendLengthPtr;

		if (intCarDataCnt >= intDataLen / 3) // why not > ??
		{
			// Won't all fit 

			bytToRS[0] = intDataLen / 3;
			intStartIndex = intEncodedDataPtr;
			memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen / 3);
			bytDataToSendLengthPtr += intDataLen / 3;
		}
		else
		{
			// Last bit

			bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

			if (intCarDataCnt > 0)
			{
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
				bytDataToSendLengthPtr += intCarDataCnt;
			}
		}
		GenCRC16FrameType(bytToRS, intDataLen / 3 + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

		RSEncode(bytToRS, bytToRS + intDataLen / 3 + 3, intDataLen / 3 + 3, intRSLen / 3);  // Generate the RS encoding ...now 14 bytes total
		intEncodedDataPtr += intDataLen / 3 + 3 + intRSLen / 3;
		bytToRS += intDataLen / 3 + 3 + intRSLen / 3;
	}
	return intEncodedDataPtr;
}


void DrawRXFrame(int State, const char * Frame)
{
}
void DrawTXFrame(const char * Frame)
{
}

int SendtoGUI(char Type, unsigned char * Msg, int Len)
{
	return 0;
}