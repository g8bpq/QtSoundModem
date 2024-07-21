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

#include <QSettings>
#include <QDialog>

#include "UZ7HOStuff.h"

extern "C" void get_exclude_list(char * line, TStringList * list);
extern "C" void get_exclude_frm(char * line, TStringList * list);

extern "C" int SoundMode; 
extern "C" int onlyMixSnoop;

//extern "C" int RX_SR;
//extern "C" int TX_SR;
extern "C" int txLatency;

extern "C" int multiCore;
extern "C" char * Wisdom;
extern int WaterfallMin;
extern int WaterfallMax;

extern "C" word MEMRecovery[5];

extern int MintoTray;
extern "C" int UDPClientPort;
extern "C" int UDPServerPort;
extern "C" int TXPort;

extern char UDPHost[64];
extern QDialog * constellationDialog;
extern QRect PSKRect;

extern char CWIDCall[128];
extern "C" char CWIDMark[32];
extern int CWIDInterval;
extern int CWIDLeft;
extern int CWIDRight;
extern int CWIDType;
extern bool afterTraffic;
extern bool darkTheme;

extern "C" int RSID_SABM[4];
extern "C" int RSID_UI[4];
extern "C" int RSID_SetModem[4];

extern "C" int nonGUIMode;



extern QFont Font;


QSettings* settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);

// This makes geting settings for more channels easier

char Prefix[16] = "AX25_A";

void GetPortSettings(int Chan);

QVariant getAX25Param(const char * key, QVariant Default)
{
	char fullKey[64];
	QVariant Q;
	QByteArray x;
	sprintf(fullKey, "%s/%s", Prefix, key);
	Q = settings->value(fullKey, Default);
	x = Q.toString().toUtf8();

	return Q;
}

void getAX25Params(int chan)
{
	Prefix[5] = chan + 'A';
	GetPortSettings(chan);
}


void GetPortSettings(int Chan)
{
	tx_hitoneraisedb[Chan] = getAX25Param("HiToneRaise", 0).toInt();

	maxframe[Chan] = getAX25Param("Maxframe", 3).toInt();
	fracks[Chan] = getAX25Param("Retries", 15).toInt();
	frack_time[Chan] = getAX25Param("FrackTime", 5).toInt();

	idletime[Chan] = getAX25Param("IdleTime", 180).toInt();
	slottime[Chan] = getAX25Param("SlotTime", 100).toInt();
	persist[Chan] = getAX25Param("Persist", 128).toInt();
	resptime[Chan] = getAX25Param("RespTime", 1500).toInt();
	TXFrmMode[Chan] = getAX25Param("TXFrmMode", 1).toInt();
	max_frame_collector[Chan] = getAX25Param("FrameCollector", 6).toInt();
	KISS_opt[Chan] = getAX25Param("KISSOptimization", false).toInt();;
	dyn_frack[Chan] = getAX25Param("DynamicFrack", false).toInt();;
	recovery[Chan] = getAX25Param("BitRecovery", 0).toInt();
	NonAX25[Chan] = getAX25Param("NonAX25Frm", false).toInt();;
	MEMRecovery[Chan]= getAX25Param("MEMRecovery", 200).toInt();
	IPOLL[Chan] = getAX25Param("IPOLL", 80).toInt();

	strcpy(MyDigiCall[Chan], getAX25Param("MyDigiCall", "").toString().toUtf8());
	strcpy(exclude_callsigns[Chan], getAX25Param("ExcludeCallsigns", "").toString().toUtf8());

	fx25_mode[Chan] = getAX25Param("FX25", FX25_MODE_RX).toInt();
	il2p_mode[Chan] = getAX25Param("IL2P", IL2P_MODE_NONE).toInt();
	il2p_crc[Chan] = getAX25Param("IL2PCRC", 0).toInt();
	RSID_UI[Chan] = getAX25Param("RSID_UI", 0).toInt();
	RSID_SABM[Chan] = getAX25Param("RSID_SABM", 0).toInt();
	RSID_SetModem[Chan] = getAX25Param("RSID_SetModem", 0).toInt();
}

void getSettings()
{
	int snd_ch;

	QSettings* settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
	settings->sync();

	PSKRect = settings->value("PSKWindow").toRect();

	SoundMode = settings->value("Init/SoundMode", 0).toInt();
	UDPClientPort = settings->value("Init/UDPClientPort", 8888).toInt();
	UDPServerPort = settings->value("Init/UDPServerPort", 8884).toInt();
	TXPort = settings->value("Init/TXPort", UDPServerPort).toInt();

	strcpy(UDPHost, settings->value("Init/UDPHost", "192.168.1.255").toString().toUtf8());
	UDPServ = settings->value("Init/UDPServer", FALSE).toBool();

//	RX_SR = settings->value("Init/RXSampleRate", 12000).toInt();
//	TX_SR = settings->value("Init/TXSampleRate", 12000).toInt();
	txLatency = settings->value("Init/txLatency", 50).toInt();


	onlyMixSnoop = settings->value("Init/onlyMixSnoop", 0).toInt();

	strcpy(CaptureDevice, settings->value("Init/SndRXDeviceName", "hw:1,0").toString().toUtf8());
	strcpy(PlaybackDevice, settings->value("Init/SndTXDeviceName", "hw:1,0").toString().toUtf8());

	raduga = settings->value("Init/DispMode", DISP_RGB).toInt();

	strcpy(PTTPort, settings->value("Init/PTT", "").toString().toUtf8());
	PTTMode = settings->value("Init/PTTMode", 19200).toInt();
	PTTBAUD = settings->value("Init/PTTBAUD", 19200).toInt();

	strcpy(PTTOnString, settings->value("Init/PTTOnString", "").toString().toUtf8());
	strcpy(PTTOffString, settings->value("Init/PTTOffString", "").toString().toUtf8());

	pttGPIOPin = settings->value("Init/pttGPIOPin", 17).toInt();
	pttGPIOPinR = settings->value("Init/pttGPIOPinR", 17).toInt();

#ifdef WIN32
	strcpy(CM108Addr, settings->value("Init/CM108Addr", "0xD8C:0x08").toString().toUtf8());
#else
	strcpy(CM108Addr, settings->value("Init/CM108Addr", "/dev/hidraw0").toString().toUtf8());
#endif

	HamLibPort = settings->value("Init/HamLibPort", 4532).toInt();
	strcpy(HamLibHost, settings->value("Init/HamLibHost", "127.0.0.1").toString().toUtf8());

	FLRigPort = settings->value("Init/FLRigPort", 12345).toInt();
	strcpy(FLRigHost, settings->value("Init/FLRigHost", "127.0.0.1").toString().toUtf8());


	DualPTT = settings->value("Init/DualPTT", 1).toInt();
	TX_rotate = settings->value("Init/TXRotate", 0).toInt();
	multiCore = settings->value("Init/multiCore", 0).toInt();
	MintoTray = settings->value("Init/MinimizetoTray", 1).toInt();
	Wisdom = strdup(settings->value("Init/Wisdom", "").toString().toUtf8());
	WaterfallMin = settings->value("Init/WaterfallMin", 0).toInt();
	WaterfallMax = settings->value("Init/WaterfallMax", 3300).toInt();


	rx_freq[0] = settings->value("Modem/RXFreq1", 1700).toInt();
	rx_freq[1] = settings->value("Modem/RXFreq2", 1700).toInt();
	rx_freq[2] = settings->value("Modem/RXFreq3", 1700).toInt();
	rx_freq[3] = settings->value("Modem/RXFreq4", 1700).toInt();

	rcvr_offset[0] = settings->value("Modem/RcvrShift1", 30).toInt();
	rcvr_offset[1] = settings->value("Modem/RcvrShift2", 30).toInt();
	rcvr_offset[2] = settings->value("Modem/RcvrShift3", 30).toInt();
	rcvr_offset[3] = settings->value("Modem/RcvrShift4", 30).toInt();
	speed[0] = settings->value("Modem/ModemType1", SPEED_1200).toInt();
	speed[1] = settings->value("Modem/ModemType2", SPEED_1200).toInt();
	speed[2] = settings->value("Modem/ModemType3", SPEED_1200).toInt();
	speed[3] = settings->value("Modem/ModemType4", SPEED_1200).toInt();

	RCVR[0] = settings->value("Modem/NRRcvrPairs1", 0).toInt();;
	RCVR[1] = settings->value("Modem/NRRcvrPairs2", 0).toInt();;
	RCVR[2] = settings->value("Modem/NRRcvrPairs3", 0).toInt();;
	RCVR[3] = settings->value("Modem/NRRcvrPairs4", 0).toInt();;

	soundChannel[0] = settings->value("Modem/soundChannel1", 1).toInt();
	soundChannel[1] = settings->value("Modem/soundChannel2", 0).toInt();
	soundChannel[2] = settings->value("Modem/soundChannel3", 0).toInt();
	soundChannel[3] = settings->value("Modem/soundChannel4", 0).toInt();

	SCO = settings->value("Init/SCO", 0).toInt();

	dcd_threshold = settings->value("Modem/DCDThreshold", 40).toInt();
	rxOffset = settings->value("Modem/rxOffset", 0).toInt();

	AGWServ = settings->value("AGWHost/Server", TRUE).toBool();
	AGWPort = settings->value("AGWHost/Port", 8000).toInt();
	KISSServ = settings->value("KISS/Server", FALSE).toBool();
	KISSPort = settings->value("KISS/Port", 8105).toInt();

//	RX_Samplerate = RX_SR + RX_SR * 0.000001*RX_PPM;
//	TX_Samplerate = TX_SR + TX_SR * 0.000001*TX_PPM;

	emph_all[0] = settings->value("Modem/PreEmphasisAll1", FALSE).toBool();
	emph_all[1] = settings->value("Modem/PreEmphasisAll2", FALSE).toBool();
	emph_all[2] = settings->value("Modem/PreEmphasisAll3", FALSE).toBool();
	emph_all[3] = settings->value("Modem/PreEmphasisAll4", FALSE).toBool();

	emph_db[0] = settings->value("Modem/PreEmphasisDB1", 0).toInt();
	emph_db[1] = settings->value("Modem/PreEmphasisDB2", 0).toInt();
	emph_db[2] = settings->value("Modem/PreEmphasisDB3", 0).toInt();
	emph_db[3] = settings->value("Modem/PreEmphasisDB4", 0).toInt();

	Firstwaterfall = settings->value("Window/Waterfall1", TRUE).toInt();
	Secondwaterfall = settings->value("Window/Waterfall2", TRUE).toInt();

	txdelay[0] = settings->value("Modem/TxDelay1", 250).toInt();
	txdelay[1] = settings->value("Modem/TxDelay2", 250).toInt();
	txdelay[2] = settings->value("Modem/TxDelay3", 250).toInt();
	txdelay[3] = settings->value("Modem/TxDelay4", 250).toInt();

	txtail[0] = settings->value("Modem/TxTail1", 50).toInt();
	txtail[1] = settings->value("Modem/TxTail2", 50).toInt();
	txtail[2] = settings->value("Modem/TxTail3", 50).toInt();
	txtail[3] = settings->value("Modem/TxTail4", 50).toInt();

	strcpy(CWIDCall, settings->value("Modem/CWIDCall", "").toString().toUtf8().toUpper());
	strcpy(CWIDMark, settings->value("Modem/CWIDMark", "").toString().toUtf8().toUpper());
	CWIDInterval = settings->value("Modem/CWIDInterval", 0).toInt();
	CWIDLeft = settings->value("Modem/CWIDLeft", 0).toInt();
	CWIDRight = settings->value("Modem/CWIDRight", 0).toInt();
	CWIDType = settings->value("Modem/CWIDType", 1).toInt();			// on/off
	afterTraffic = settings->value("Modem/afterTraffic", false).toBool();

	getAX25Params(0);
	getAX25Params(1);
	getAX25Params(2);
	getAX25Params(3);

	// Validate and process settings

	UsingLeft = 0;
	UsingRight = 0;
	UsingBothChannels = 0;

	for (int i = 0; i < 4; i++)
	{
		if (soundChannel[i] == LEFT)
		{
			UsingLeft = 1;
			modemtoSoundLR[i] = 0;
		}
		else if (soundChannel[i] == RIGHT)
		{
			UsingRight = 1;
			modemtoSoundLR[i] = 1;
		}
	}

	if (UsingLeft && UsingRight)
		UsingBothChannels = 1;

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		tx_hitoneraise[snd_ch] = powf(10.0f, -abs(tx_hitoneraisedb[snd_ch]) / 20.0f);

		if (IPOLL[snd_ch] < 0)
			IPOLL[snd_ch] = 0;
		else if (IPOLL[snd_ch] > 65535)
			IPOLL[snd_ch] = 65535;

		if (MEMRecovery[snd_ch] < 1)
			MEMRecovery[snd_ch] = 1;

		//			if (MEMRecovery[snd_ch]> 65535)
		//				MEMRecovery[snd_ch]= 65535;

				/*
				if resptime[snd_ch] < 0 then resptime[snd_ch]= 0;
					if resptime[snd_ch] > 65535 then resptime[snd_ch]= 65535;
					if persist[snd_ch] > 255 then persist[snd_ch]= 255;
					if persist[snd_ch] < 32 then persist[snd_ch]= 32;
					if fracks[snd_ch] < 1 then fracks[snd_ch]= 1;
					if frack_time[snd_ch] < 1 then frack_time[snd_ch]= 1;
					if idletime[snd_ch] < frack_time[snd_ch] then idletime[snd_ch]= 180;
				*/

		if (emph_db[snd_ch] < 0 || emph_db[snd_ch] > nr_emph)
			emph_db[snd_ch] = 0;

		if (max_frame_collector[snd_ch] > 6) max_frame_collector[snd_ch] = 6;
		if (maxframe[snd_ch] == 0 || maxframe[snd_ch] > 7) maxframe[snd_ch] = 3;
		if (qpsk_set[snd_ch].mode > 1)  qpsk_set[snd_ch].mode = 0;

	}

	darkTheme = settings->value("Init/darkTheme", false).toBool();

	delete(settings);
}

void SavePortSettings(int Chan);

void saveAX25Param(const char * key, QVariant Value)
{
	char fullKey[64];

	sprintf(fullKey, "%s/%s", Prefix, key);

	settings->setValue(fullKey, Value);
}

void saveAX25Params(int chan)
{
	Prefix[5] = chan + 'A';
	SavePortSettings(chan);
}

void SavePortSettings(int Chan)
{
	saveAX25Param("Retries", fracks[Chan]);
	saveAX25Param("HiToneRaise", tx_hitoneraisedb[Chan]);
	saveAX25Param("Maxframe",maxframe[Chan]);
	saveAX25Param("Retries", fracks[Chan]);
	saveAX25Param("FrackTime", frack_time[Chan]);
	saveAX25Param("IdleTime", idletime[Chan]);
	saveAX25Param("SlotTime", slottime[Chan]);
	saveAX25Param("Persist", persist[Chan]);
	saveAX25Param("RespTime", resptime[Chan]);
	saveAX25Param("TXFrmMode", TXFrmMode[Chan]);
	saveAX25Param("FrameCollector", max_frame_collector[Chan]);
	saveAX25Param("ExcludeCallsigns", exclude_callsigns[Chan]);
	saveAX25Param("ExcludeAPRSFrmType", exclude_APRS_frm[Chan]);
	saveAX25Param("KISSOptimization", KISS_opt[Chan]);
	saveAX25Param("DynamicFrack", dyn_frack[Chan]);
	saveAX25Param("BitRecovery", recovery[Chan]);
	saveAX25Param("NonAX25Frm", NonAX25[Chan]);
	saveAX25Param("MEMRecovery", MEMRecovery[Chan]);
	saveAX25Param("IPOLL", IPOLL[Chan]);
	saveAX25Param("MyDigiCall", MyDigiCall[Chan]);
	saveAX25Param("FX25", fx25_mode[Chan]);
	saveAX25Param("IL2P", il2p_mode[Chan]);
	saveAX25Param("IL2PCRC", il2p_crc[Chan]);
	saveAX25Param("RSID_UI", RSID_UI[Chan]);
	saveAX25Param("RSID_SABM", RSID_SABM[Chan]);
	saveAX25Param("RSID_SetModem", RSID_SetModem[Chan]);
}

void saveSettings()
{
	QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);

	settings->setValue("FontFamily", Font.family());
	settings->setValue("PointSize", Font.pointSize());
	settings->setValue("Weight", Font.weight());

	if (nonGUIMode == 0)
		settings->setValue("PSKWindow", constellationDialog->geometry());

	settings->setValue("Init/SoundMode", SoundMode);
	settings->setValue("Init/UDPClientPort", UDPClientPort);
	settings->setValue("Init/UDPServerPort", UDPServerPort);
	settings->setValue("Init/TXPort", TXPort);

	settings->setValue("Init/UDPServer", UDPServ);
	settings->setValue("Init/UDPHost", UDPHost);


//	settings->setValue("Init/TXSampleRate", TX_SR);
//	settings->setValue("Init/RXSampleRate", RX_SR);
	settings->setValue("Init/txLatency", txLatency);

	settings->setValue("Init/onlyMixSnoop", onlyMixSnoop);
	
	settings->setValue("Init/SndRXDeviceName", CaptureDevice);
	settings->setValue("Init/SndTXDeviceName", PlaybackDevice);

	settings->setValue("Init/SCO", SCO);
	settings->setValue("Init/DualPTT", DualPTT);
	settings->setValue("Init/TXRotate", TX_rotate);

	settings->setValue("Init/DispMode", raduga);

	settings->setValue("Init/PTT", PTTPort);
	settings->setValue("Init/PTTBAUD", PTTBAUD);
	settings->setValue("Init/PTTMode", PTTMode);

	settings->setValue("Init/PTTOffString", PTTOffString);
	settings->setValue("Init/PTTOnString", PTTOnString);

	settings->setValue("Init/pttGPIOPin", pttGPIOPin);
	settings->setValue("Init/pttGPIOPinR", pttGPIOPinR);

	settings->setValue("Init/CM108Addr", CM108Addr);
	settings->setValue("Init/HamLibPort", HamLibPort);
	settings->setValue("Init/HamLibHost", HamLibHost);
	settings->setValue("Init/FLRigPort", FLRigPort);
	settings->setValue("Init/FLRigHost", FLRigHost);

	settings->setValue("Init/MinimizetoTray", MintoTray);
	settings->setValue("Init/multiCore", multiCore);
	settings->setValue("Init/Wisdom", Wisdom);

	settings->setValue("Init/WaterfallMin", WaterfallMin);
	settings->setValue("Init/WaterfallMax", WaterfallMax);

	// Don't save freq on close as it could be offset by multiple decoders

	settings->setValue("Modem/NRRcvrPairs1", RCVR[0]);
	settings->setValue("Modem/NRRcvrPairs2", RCVR[1]);
	settings->setValue("Modem/NRRcvrPairs3", RCVR[2]);
	settings->setValue("Modem/NRRcvrPairs4", RCVR[3]);

	settings->setValue("Modem/RcvrShift1", rcvr_offset[0]);
	settings->setValue("Modem/RcvrShift2", rcvr_offset[1]);
	settings->setValue("Modem/RcvrShift3", rcvr_offset[2]);
	settings->setValue("Modem/RcvrShift4", rcvr_offset[3]);

	settings->setValue("Modem/ModemType1", speed[0]);
	settings->setValue("Modem/ModemType2", speed[1]);
	settings->setValue("Modem/ModemType3", speed[2]);
	settings->setValue("Modem/ModemType4", speed[3]);

	settings->setValue("Modem/soundChannel1", soundChannel[0]);
	settings->setValue("Modem/soundChannel2", soundChannel[1]);
	settings->setValue("Modem/soundChannel3", soundChannel[2]);
	settings->setValue("Modem/soundChannel4", soundChannel[3]);

	settings->setValue("Modem/DCDThreshold", dcd_threshold);
	settings->setValue("Modem/rxOffset", rxOffset);

	settings->setValue("AGWHost/Server", AGWServ);
	settings->setValue("AGWHost/Port", AGWPort);
	settings->setValue("KISS/Server", KISSServ);
	settings->setValue("KISS/Port", KISSPort);

	settings->setValue("Modem/PreEmphasisAll1", emph_all[0]);
	settings->setValue("Modem/PreEmphasisAll2", emph_all[1]);
	settings->setValue("Modem/PreEmphasisAll3", emph_all[2]);
	settings->setValue("Modem/PreEmphasisAll4", emph_all[3]);

	settings->setValue("Modem/PreEmphasisDB1", emph_db[0]);
	settings->setValue("Modem/PreEmphasisDB2", emph_db[1]);
	settings->setValue("Modem/PreEmphasisDB3", emph_db[2]);
	settings->setValue("Modem/PreEmphasisDB4", emph_db[3]);

	settings->setValue("Window/Waterfall1", Firstwaterfall);
	settings->setValue("Window/Waterfall2", Secondwaterfall);

	settings->setValue("Modem/TxDelay1", txdelay[0]);
	settings->setValue("Modem/TxDelay2", txdelay[1]);
	settings->setValue("Modem/TxDelay3", txdelay[2]);
	settings->setValue("Modem/TxDelay4", txdelay[3]);

	settings->setValue("Modem/TxTail1", txtail[0]);
	settings->setValue("Modem/TxTail2", txtail[1]);
	settings->setValue("Modem/TxTail3", txtail[2]);
	settings->setValue("Modem/TxTail4", txtail[3]);

	settings->setValue("Modem/CWIDCall", CWIDCall);
	settings->setValue("Modem/CWIDMark", CWIDMark);
	settings->setValue("Modem/CWIDInterval", CWIDInterval);
	settings->setValue("Modem/CWIDLeft", CWIDLeft);
	settings->setValue("Modem/CWIDRight", CWIDRight);
	settings->setValue("Modem/CWIDType", CWIDType);
	settings->setValue("Modem/afterTraffic", afterTraffic);

	settings->setValue("Init/darkTheme", darkTheme);

	saveAX25Params(0);
	saveAX25Params(1);
	saveAX25Params(2);
	saveAX25Params(3);

	settings->sync();

	delete(settings);
}
