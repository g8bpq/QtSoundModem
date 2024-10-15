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

// UZ7HO Soundmodem Port

// Not Working 4psk100 FEC 

// Thoughts on Waterfall Display.

// Original used a 2048 sample FFT giving 5.859375 Hz bins. We plotted 1024 points, giving a 0 to 6000 specrum

// If we want say 300 to 3300 we need about half the bin size so twice the fft size. But should we also fit required range to window size?

// Unless we resize the most displayed bit of the screen in around 900 pixels. So each bin should be 3300 / 900 = 3.66667 Hz or a FFT size of around 3273

#include "QtSoundModem.h"
#include <qheaderview.h>
//#include <QDebug>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QSettings>
#include <QPainter>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <qevent.h>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QFontDialog>
#include <QFile>
#include "UZ7HOStuff.h"

#include <time.h>

QImage *Constellation[4];
QImage *Waterfall = 0;
QLabel *DCDLabel[4];
QLineEdit *chanOffsetLabel[4];
QImage *DCDLed[4];

QImage *RXLevel;
QImage *RXLevel2;

QLabel *WaterfallCopy;

QLabel * RXLevelCopy;
QLabel * RXLevel2Copy;

QTextEdit * monWindowCopy;

extern workerThread *t;
extern QtSoundModem * w;
extern QCoreApplication * a;
extern serialThread *serial;

QList<QSerialPortInfo> Ports = QSerialPortInfo::availablePorts();

void saveSettings();
void getSettings();
void DrawModemFreqRange();

extern "C" void CloseSound();
extern "C" void GetSoundDevices();
extern "C" char modes_name[modes_count][21];
extern "C" int speed[5];
extern "C" int KISSPort;
extern "C" short rx_freq[5];

extern "C" int CaptureCount;
extern "C" int PlaybackCount;

extern "C" int CaptureIndex;		// Card number
extern "C" int PlayBackIndex;

extern "C" char CaptureNames[16][256];
extern "C" char PlaybackNames[16][256];

extern "C" int SoundMode;
extern "C" bool onlyMixSnoop;

extern "C" int multiCore;

extern "C" int refreshModems;
int NeedPSKRefresh;

extern "C" int pnt_change[5];
extern "C" int needRSID[4];

extern "C" int needSetOffset[4];

extern "C" float MagOut[4096];
extern "C" float MaxMagOut;
extern "C" int MaxMagIndex;


extern "C" int using48000;			// Set if using 48K sample rate (ie RUH Modem active)
extern "C" int ReceiveSize;
extern "C" int SendSize;		// 100 mS for now

extern "C" int txLatency;

extern "C" int BusyDet;

extern "C"
{ 
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
	void modulator(UCHAR snd_ch, int buf_size);
	void SampleSink(int LR, short Sample);
	void doCalib(int Port, int Act);
	int Freq_Change(int Chan, int Freq);
	void set_speed(int snd_ch, int Modem);
	void init_speed(int snd_ch);
	void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);
	void dofft(short * in, float * outr, float * outi);
	void init_raduga();
	void DrawFreqTicks();
	void AGW_Report_Modem_Change(int port);
	char * strlop(char * buf, char delim);
	void sendRSID(int Chan, int dropTX);
	void RSIDinitfft();
	void il2p_init(int il2p_debug);
	void closeTraceLog();
}

void make_graph_buf(float * buf, short tap, QPainter * bitmap);

int ModemA = 2;
int ModemB = 2;
int ModemC = 2;
int ModemD = 2;
int FreqA = 1500;
int FreqB = 1500;
int FreqC = 1500;
int FreqD = 1500;
int DCD = 50;

char CWIDCall[128] = "";
extern "C" char CWIDMark[32];
int CWIDInterval = 0;
int CWIDLeft = 0;
int CWIDRight = 0;
int CWIDType = 1;			// on/off
bool afterTraffic = 0;
bool cwidtimerisActive = false;

int WaterfallMin = 00;
int WaterfallMax = 6000;

int Configuring = 0;
bool lockWaterfall = false;
bool inWaterfall = false;

int MgmtPort = 0;

extern "C" int NeedWaterfallHeaders;
extern "C" float BinSize;

extern "C" { int RSID_SABM[4]; }
extern "C" { int RSID_UI[4]; }
extern "C" { int RSID_SetModem[4]; }
extern "C" unsigned int pskStates[4];

int Closing = FALSE;				// Set to stop background thread

QRgb white = qRgb(255, 255, 255);
QRgb black = qRgb(0, 0, 0);

QRgb green = qRgb(0, 255, 0);
QRgb red = qRgb(255, 0, 0);
QRgb yellow = qRgb(255, 255, 0);
QRgb cyan = qRgb(0, 255, 255);

QRgb txText = qRgb(192, 0, 0);
QRgb rxText = qRgb(0, 0, 192);

bool darkTheme = true;
bool minimizeonStart = true;
extern "C" bool useKISSControls;

// Indexed colour list from ARDOPC

#define WHITE 0
#define Tomato 1
#define Gold 2
#define Lime 3
#define Yellow 4
#define Orange 5
#define Khaki 6
#define Cyan 7
#define DeepSkyBlue 8
#define RoyalBlue 9
#define Navy 10
#define Black 11
#define Goldenrod 12
#define Fuchsia 13

QRgb vbColours[16] = { qRgb(255, 255, 255), qRgb(255, 99, 71), qRgb(255, 215, 0), qRgb(0, 255, 0),
						qRgb(255, 255, 0), qRgb(255, 165, 0), qRgb(240, 240, 140), qRgb(0, 255, 255),
						qRgb(0, 191, 255), qRgb(65, 105, 225), qRgb(0, 0, 128), qRgb(0, 0, 0),
						qRgb(218, 165, 32), qRgb(255, 0, 255) };

unsigned char  WaterfallLines[2][80][4096] = { 0 };
int NextWaterfallLine[2] = {0, 0};

unsigned int LastLevel = 255;
unsigned int LastBusy = 255;

extern "C" int UDPClientPort;
extern "C" int UDPServerPort;
extern "C" int TXPort;
extern char UDPHost[64];

QTimer *cwidtimer;
QTimer *PTTWatchdog;

QWidget * mythis;

QElapsedTimer pttOnTimer;



QSystemTrayIcon * trayIcon = nullptr;

int MintoTray = 1;

int RSID_WF = 0;				// Set to use RSID FFT for Waterfall. 

char SixPackDevice[256] = "";
int SixPackPort = 0;
int SixPackEnable = 0;

// Stats

uint64_t PTTonTime[4] = { 0 };
uint64_t PTTActivemS[4] = { 0 };			// For Stats
uint64_t BusyonTime[4] = { 0 };
uint64_t BusyActivemS[4] = { 0 };

int AvPTT[4] = { 0 };
int AvBusy[4] = { 0 };


extern "C" void WriteDebugLog(char * Mess)
{
	qDebug() << Mess;
}

void QtSoundModem::doupdateDCD(int Chan, int State)
{
	DCDLabel[Chan]->setVisible(State);

	// This also tries to get a percentage on time over a minute

	uint64_t Time = QDateTime::currentMSecsSinceEpoch();

	if (State)
	{
		BusyonTime[Chan] = Time;
		return;
	}

	if (BusyonTime[Chan])
	{
		BusyActivemS[Chan] += Time - BusyonTime[Chan];
		BusyonTime[Chan] = 0;
	}


}

extern "C" char * frame_monitor(string * frame, char * code, bool tx_stat);
extern "C" char * ShortDateTime();

extern "C" void mon_rsid(int snd_ch, char * RSID)
{
	int Len;
	char * Msg = (char *)malloc(1024);		// Cant pass local variable via signal/slot

	sprintf(Msg, "%d:%s [%s%c]", snd_ch + 1, RSID, ShortDateTime(), 'R');

	Len = strlen(Msg);

	if (Msg[Len - 1] != '\r')
	{
		Msg[Len++] = '\r';
		Msg[Len] = 0;
	}

	emit t->sendtoTrace(Msg, 0);
}

extern "C" void put_frame(int snd_ch, string * frame, char * code, int  tx, int excluded)
{
	UNUSED(excluded);

	int Len;
	char * Msg = (char *)malloc(1024);		// Cant pass local variable via signal/slot

	if (strcmp(code, "NON-AX25") == 0)
		sprintf(Msg, "%d: <NON-AX25 frame Len = %d [%s%c]\r", snd_ch, frame->Length, ShortDateTime(), 'R');
	else
		sprintf(Msg, "%d:%s", snd_ch + 1, frame_monitor(frame, code, tx));

	Len = strlen(Msg);

	if (Msg[Len - 1] != '\r')
	{
		Msg[Len++] = '\r';
		Msg[Len] = 0;
	}

	emit t->sendtoTrace(Msg, tx);
}

extern "C" void updateDCD(int Chan, bool State)
{
	emit t->updateDCD(Chan, State);
}

bool QtSoundModem::eventFilter(QObject* obj, QEvent *evt)
{
	UNUSED(obj);

	if (evt->type() == QEvent::Resize)
	{
		return QWidget::event(evt);
	}

	if (evt->type() == QEvent::WindowStateChange)
	{
		if (windowState().testFlag(Qt::WindowMinimized) == true)
			w_state = WIN_MINIMIZED;
		else
			w_state = WIN_MAXIMIZED;
	}
//	if (evt->type() == QGuiApplication::applicationStateChanged) - this is a sigma;
//	{
//		qDebug() << "App State changed =" << evt->type() << endl;
//	}

	return QWidget::event(evt);
}

void QtSoundModem::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	QRect r = geometry();

	int modemBoxHeight = 34;
	
	int Width = r.width();
	int Height = r.height() - 25;

	int monitorTop;
	int monitorHeight;
	int sessionTop;
	int sessionHeight = 0;
	int waterfallsTop;
	int waterfallsHeight = WaterfallImageHeight;

	ui.modeB->setVisible(soundChannel[1]);
	ui.centerB->setVisible(soundChannel[1]);
	ui.labelB->setVisible(soundChannel[1]);
	DCDLabel[1]->setVisible(soundChannel[1]);
	ui.RXOffsetB->setVisible(soundChannel[1]);

	ui.modeC->setVisible(soundChannel[2]);
	ui.centerC->setVisible(soundChannel[2]);
	ui.labelC->setVisible(soundChannel[2]);
	DCDLabel[2]->setVisible(soundChannel[2]);
	ui.RXOffsetC->setVisible(soundChannel[2]);

	ui.modeD->setVisible(soundChannel[3]);
	ui.centerD->setVisible(soundChannel[3]);
	ui.labelD->setVisible(soundChannel[3]);
	DCDLabel[3]->setVisible(soundChannel[3]);
	ui.RXOffsetD->setVisible(soundChannel[3]);

	if (soundChannel[2] || soundChannel[3])
		modemBoxHeight = 60;

	ui.Waterfall->setVisible(0);

	monitorTop = modemBoxHeight + 1;

	// Now have one waterfall label containing headers and waterfalls

		if (Firstwaterfall || Secondwaterfall)
			ui.Waterfall->setVisible(1);

	if (AGWServ)
	{
		sessionTable->setVisible(true);
		sessionHeight = 150;
	}
	else
	{
		sessionTable->setVisible(false);
	}

	// if only displaying one Waterfall, change height of waterfall area

	if (UsingBothChannels == 0  || (Firstwaterfall == 0) || (Secondwaterfall == 0))
	{
		waterfallsHeight /= 2;
	}

	if ((Firstwaterfall == 0) && (Secondwaterfall == 0))
		waterfallsHeight = 0;

	monitorHeight = Height - sessionHeight - waterfallsHeight - modemBoxHeight;
	waterfallsTop = Height - waterfallsHeight;
	sessionTop = Height - (sessionHeight + waterfallsHeight);

	ui.monWindow->setGeometry(QRect(0, monitorTop, Width, monitorHeight));

	if (AGWServ)
		sessionTable->setGeometry(QRect(0, sessionTop, Width, sessionHeight));

	if (waterfallsHeight)
		ui.Waterfall->setGeometry(QRect(0, waterfallsTop, Width, waterfallsHeight + 2));
}

QAction * setupMenuLine(QMenu * Menu, char * Label, QObject * parent, int State)
{
	QAction * Act = new QAction(Label, parent);
	Menu->addAction(Act);

	Act->setCheckable(true);
	if (State)
		Act->setChecked(true);

	parent->connect(Act, SIGNAL(triggered()), parent, SLOT(menuChecked()));

	return Act;
}

void QtSoundModem::menuChecked()
{
	QAction * Act = static_cast<QAction*>(QObject::sender());

	int state = Act->isChecked();

	if (Act == actWaterfall1)
		Firstwaterfall = state;

	else if (Act == actWaterfall2)
		Secondwaterfall = state;

	initWaterfall(Firstwaterfall | Secondwaterfall);

	saveSettings();
}

void QtSoundModem::initWaterfall(int state)
{
	if (state == 1)
	{
	//	if (ui.Waterfall)
	//	{
	//		delete ui.Waterfall;
	//		ui.Waterfall = new QLabel(ui.centralWidget);
	//	}
		WaterfallCopy = ui.Waterfall;

		Waterfall = new QImage(1024, WaterfallImageHeight + 2, QImage::Format_RGB32);

		NeedWaterfallHeaders = 1;

	}
	else
	{
		delete(Waterfall);
		Waterfall = 0;
	}

	QSize Size(800, 602);						// Not actually used, but Event constructor needs it

	QResizeEvent *event = new QResizeEvent(Size, Size);
	QApplication::sendEvent(this, event);
}

QRect PSKRect = { 100,100,100,100 };

QDialog * constellationDialog;
QLabel * constellationLabel[4];
QLabel * QualLabel[4];

// Local copies

QLabel *RXOffsetLabel;
QSlider *RXOffset;

QFont Font;

extern "C" void CheckPSKWindows()
{
	NeedPSKRefresh = 1;
}
void DoPSKWindows()
{
	// Display Constellation for PSK Window;

	int NextX = 0;
	int i;

	for (i = 0; i < 4; i++)
	{
		if (soundChannel[i] && pskStates[i])
		{
			constellationLabel[i]->setGeometry(QRect(NextX, 19, 121, 121));
			QualLabel[i]->setGeometry(QRect(1 + NextX, 1, 120, 15));
			constellationLabel[i]->setVisible(1);
			QualLabel[i]->setVisible(1);

			NextX += 122;
		}
		else
		{
			constellationLabel[i]->setVisible(0);
			QualLabel[i]->setVisible(0);
		}
	}
	constellationDialog->resize(NextX, 140);
}

QTimer *wftimer;
extern "C" struct timespec pttclk;

QtSoundModem::QtSoundModem(QWidget *parent) : QMainWindow(parent)
{
	QString family;
	int csize;
	QFont::Weight weight;

#ifndef WIN32
	clock_getres(CLOCK_MONOTONIC, &pttclk);
	printf("CLOCK_MONOTONIC %d, %d\n", pttclk.tv_sec, pttclk.tv_nsec);
#endif	


	ui.setupUi(this);

	mythis = this;

	getSettings();

	serial = new serialThread;

	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);

	family = mysettings.value("FontFamily", "Courier New").toString();
	csize = mysettings.value("PointSize", 0).toInt();
	weight = (QFont::Weight)mysettings.value("Weight", 50).toInt();

	Font = QFont(family);
	Font.setPointSize(csize);
	Font.setWeight(weight);

	QApplication::setFont(Font);

	constellationDialog = new QDialog(nullptr, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	constellationDialog->resize(488, 140);
	constellationDialog->setGeometry(PSKRect);

	QFont f("Arial", 8, QFont::Normal);

	for (int i = 0; i < 4; i++)
	{
		char Text[16];
		sprintf(Text, "Chan %c", i + 'A');

		constellationLabel[i] = new QLabel(constellationDialog);
		constellationDialog->setWindowTitle("PSK Constellations");

		QualLabel[i] = new QLabel(constellationDialog);
		QualLabel[i]->setText(Text);
		QualLabel[i]->setFont(f);

		Constellation[i] = new QImage(121, 121, QImage::Format_RGB32);
		Constellation[i]->fill(black);
		constellationLabel[i]->setPixmap(QPixmap::fromImage(*Constellation[i]));
	}
	constellationDialog->show();

	if (MintoTray)
	{
		char popUp[256];
		sprintf(popUp, "QtSoundModem %d %d", AGWPort, KISSPort);
		trayIcon = new QSystemTrayIcon(QIcon(":/QtSoundModem/soundmodem.ico"), this);
		trayIcon->setToolTip(popUp);
		trayIcon->show();
		
		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(TrayActivated(QSystemTrayIcon::ActivationReason)));
	}

	using48000 = 0;			// Set if using 48K sample rate (ie RUH Modem active)
	ReceiveSize = 512;
	SendSize = 1024;		// 100 mS for now

	for (int i = 0; i < 4; i++)
	{
		if (soundChannel[i] && (speed[i] == SPEED_RUH48 || speed[i] == SPEED_RUH96))
		{
			using48000 = 1;			// Set if using 48K sample rate (ie RUH Modem active)
			ReceiveSize = 2048;
			SendSize = 4096;		// 100 mS for now
		}
	}

	float FFTCalc = 12000.0f / ((WaterfallMax - WaterfallMin) / 900.0f);

	FFTSize = FFTCalc + 0.4999;

	if (FFTSize > 8191)
		FFTSize = 8190;

	if (FFTSize & 1)		// odd
		FFTSize--;

	BinSize = 12000.0 / FFTSize;

	restoreGeometry(mysettings.value("geometry").toByteArray());
	restoreState(mysettings.value("windowState").toByteArray());

	sessionTable = new QTableWidget(ui.centralWidget);

	sessionTable->verticalHeader()->setVisible(FALSE);
	sessionTable->verticalHeader()->setDefaultSectionSize(20);
	sessionTable->horizontalHeader()->setDefaultSectionSize(68);
	sessionTable->setRowCount(1);
	sessionTable->setColumnCount(12);
	m_TableHeader << "MyCall" << "DestCall" << "Status" << "Sent pkts" << "Sent Bytes" << "Rcvd pkts" << "Rcvd bytes" << "Rcvd FC" << "FEC corr" << "CPS TX" << "CPS RX" << "Direction";

	mysetstyle();

	sessionTable->setHorizontalHeaderLabels(m_TableHeader);
	sessionTable->setColumnWidth(0, 80);
	sessionTable->setColumnWidth(1, 80);
	sessionTable->setColumnWidth(4, 76);
	sessionTable->setColumnWidth(5, 76);
	sessionTable->setColumnWidth(6, 80);
	sessionTable->setColumnWidth(11, 72);

	for (int i = 0; i < modes_count; i++)
	{
		ui.modeA->addItem(modes_name[i]);
		ui.modeB->addItem(modes_name[i]);
		ui.modeC->addItem(modes_name[i]);
		ui.modeD->addItem(modes_name[i]);
	}

	// Set up Menus

	setupMenu = ui.menuBar->addMenu(tr("Settings"));

	actDevices = new QAction("Setup Devices", this);
	setupMenu->addAction(actDevices);

	connect(actDevices, SIGNAL(triggered()), this, SLOT(clickedSlot()));
	actDevices->setObjectName("actDevices");
	actModems = new QAction("Setup Modems", this);
	actModems->setObjectName("actModems");
	setupMenu->addAction(actModems);

	connect(actModems, SIGNAL(triggered()), this, SLOT(clickedSlot()));

	actFont = new QAction("Setup Font", this);
	actFont->setObjectName("actFont");
	setupMenu->addAction(actFont);

	connect(actFont, SIGNAL(triggered()), this, SLOT(clickedSlot()));

	actMintoTray = setupMenu->addAction("Minimize to Tray", this, SLOT(MinimizetoTray()));
	actMintoTray->setCheckable(1);
	actMintoTray->setChecked(MintoTray);

	viewMenu = ui.menuBar->addMenu(tr("&View"));

	actWaterfall1 = setupMenuLine(viewMenu, (char *)"First waterfall", this, Firstwaterfall);
	actWaterfall2 = setupMenuLine(viewMenu, (char *)"Second Waterfall", this, Secondwaterfall);

	actCalib = ui.menuBar->addAction("&Calibration");
	connect(actCalib, SIGNAL(triggered()), this, SLOT(doCalibrate()));

	actRestartWF = ui.menuBar->addAction("Restart Waterfall");
	connect(actRestartWF, SIGNAL(triggered()), this, SLOT(doRestartWF()));

	actAbout = ui.menuBar->addAction("&About");
	connect(actAbout, SIGNAL(triggered()), this, SLOT(doAbout()));

	RXLevel = new QImage(150, 10, QImage::Format_RGB32);
	RXLevel->fill(white);
	ui.RXLevel->setPixmap(QPixmap::fromImage(*RXLevel));
	RXLevelCopy = ui.RXLevel;

	RXLevel2 = new QImage(150, 10, QImage::Format_RGB32);
	RXLevel2->fill(white);
	ui.RXLevel2->setPixmap(QPixmap::fromImage(*RXLevel2));
	RXLevel2Copy = ui.RXLevel2;

	DCDLabel[0] = new QLabel(this);
	DCDLabel[0]->setObjectName(QString::fromUtf8("DCDLedA"));
	DCDLabel[0]->setGeometry(QRect(280, 31, 12, 12));
	DCDLabel[0]->setVisible(TRUE);

	DCDLabel[1] = new QLabel(this);
	DCDLabel[1]->setObjectName(QString::fromUtf8("DCDLedB"));
	DCDLabel[1]->setGeometry(QRect(575, 31, 12, 12));
	DCDLabel[1]->setVisible(TRUE);

	DCDLabel[2] = new QLabel(this);
	DCDLabel[2]->setObjectName(QString::fromUtf8("DCDLedC"));
	DCDLabel[2]->setGeometry(QRect(280, 61, 12, 12));
	DCDLabel[2]->setVisible(FALSE);

	DCDLabel[3] = new QLabel(this);
	DCDLabel[3]->setObjectName(QString::fromUtf8("DCDLedD"));
	DCDLabel[3]->setGeometry(QRect(575, 61, 12, 12));
	DCDLabel[3]->setVisible(FALSE);
	
	DCDLed[0] = new QImage(12, 12, QImage::Format_RGB32);
	DCDLed[1] = new QImage(12, 12, QImage::Format_RGB32);
	DCDLed[2] = new QImage(12, 12, QImage::Format_RGB32);
	DCDLed[3] = new QImage(12, 12, QImage::Format_RGB32);

	DCDLed[0]->fill(red);
	DCDLed[1]->fill(red);
	DCDLed[2]->fill(red);
	DCDLed[3]->fill(red);

	DCDLabel[0]->setPixmap(QPixmap::fromImage(*DCDLed[0]));
	DCDLabel[1]->setPixmap(QPixmap::fromImage(*DCDLed[1]));
	DCDLabel[2]->setPixmap(QPixmap::fromImage(*DCDLed[2]));
	DCDLabel[3]->setPixmap(QPixmap::fromImage(*DCDLed[3]));

	chanOffsetLabel[0] = ui.RXOffsetA;
	chanOffsetLabel[1] = ui.RXOffsetB;
	chanOffsetLabel[2] = ui.RXOffsetC;
	chanOffsetLabel[3] = ui.RXOffsetD;

	WaterfallCopy = ui.Waterfall;

	initWaterfall(Firstwaterfall | Secondwaterfall);

	monWindowCopy = ui.monWindow;

	ui.monWindow->document()->setMaximumBlockCount(10000);

//	connect(ui.monWindow, SIGNAL(selectionChanged()), this, SLOT(onTEselectionChanged()));

	connect(ui.modeA, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeB, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeC, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeD, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));

	ModemA = speed[0];
	ModemB = speed[1];
	ModemC = speed[2];
	ModemD = speed[3];

	ui.modeA->setCurrentIndex(speed[0]);
	ui.modeB->setCurrentIndex(speed[1]);
	ui.modeC->setCurrentIndex(speed[2]);
	ui.modeD->setCurrentIndex(speed[3]);

	ModemA = ui.modeA->currentIndex();

	ui.centerA->setValue(rx_freq[0]);
	ui.centerB->setValue(rx_freq[1]);
	ui.centerC->setValue(rx_freq[2]);
	ui.centerD->setValue(rx_freq[3]);

	connect(ui.centerA, SIGNAL(valueChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.centerB, SIGNAL(valueChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.centerC, SIGNAL(valueChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.centerD, SIGNAL(valueChanged(int)), this, SLOT(clickedSlotI(int)));

	ui.DCDSlider->setValue(dcd_threshold);

	char valChar[32];
	sprintf(valChar, "RX Offset %d", rxOffset);
	ui.RXOffsetLabel->setText(valChar);
	ui.RXOffset->setValue(rxOffset);

	RXOffsetLabel = ui.RXOffsetLabel;
	RXOffset = ui.RXOffset;

	connect(ui.DCDSlider, SIGNAL(sliderMoved(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.RXOffset, SIGNAL(valueChanged(int)), this, SLOT(clickedSlotI(int)));

	QObject::connect(t, SIGNAL(sendtoTrace(char *, int)), this, SLOT(sendtoTrace(char *, int)), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(updateDCD(int, int)), this, SLOT(doupdateDCD(int, int)), Qt::QueuedConnection);

	QObject::connect(t, SIGNAL(startCWIDTimer()), this, SLOT(startCWIDTimerSlot()), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(setWaterfallImage()), this, SLOT(setWaterfallImage()), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(setLevelImage()), this, SLOT(setLevelImage()), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(setConstellationImage(int, int)), this, SLOT(setConstellationImage(int, int)), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(startWatchdog()), this, SLOT(StartWatchdog()), Qt::QueuedConnection);
	QObject::connect(t, SIGNAL(stopWatchdog()), this, SLOT(StopWatchdog()), Qt::QueuedConnection);

	connect(ui.RXOffsetA, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetB, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetC, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetD, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);

	QTimer *statstimer = new QTimer(this);
	connect(statstimer, SIGNAL(timeout()), this, SLOT(StatsTimer()));
	statstimer->start(60000);		// One Minute

	wftimer = new QTimer(this);
	connect(wftimer, SIGNAL(timeout()), this, SLOT(doRestartWF()));
//	wftimer->start(1000 * 300);

	cwidtimer = new QTimer(this);
	connect(cwidtimer, SIGNAL(timeout()), this, SLOT(CWIDTimer()));

	PTTWatchdog = new QTimer(this);
	connect(PTTWatchdog, SIGNAL(timeout()), this, SLOT(PTTWatchdogExpired()));

	if (CWIDInterval && afterTraffic == false)
		cwidtimer->start(CWIDInterval * 60000);

	if (RSID_SetModem[0])
	{
		RSID_WF = 1;
		RSIDinitfft();
	}
//	il2p_init(1);

	QTimer::singleShot(200, this, &QtSoundModem::updateFont);

	connect(serial, &serialThread::request, this, &QtSoundModem::showRequest);


}

void QtSoundModem::updateFont()
{
	QApplication::setFont(Font);
}

void QtSoundModem::MinimizetoTray()
{
	MintoTray = actMintoTray->isChecked();
	saveSettings();
	QMessageBox::about(this, tr("QtSoundModem"),
	tr("Program must be restarted to change Minimize mode"));
}


void QtSoundModem::TrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == 3)
	{
		showNormal();
		w->setWindowState((w->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	} 
}

extern "C" void sendCWID(char * strID, BOOL blnPlay, int Chan);

extern "C" void checkforCWID()
{
	emit(t->startCWIDTimer());
};

extern "C" void QtSoundModem::startCWIDTimerSlot()
{
	if (CWIDInterval && afterTraffic == 1 && cwidtimerisActive == false)
	{
		cwidtimerisActive = true;
		QTimer::singleShot(CWIDInterval * 60000, this, &QtSoundModem::CWIDTimer);
	}
}

void QtSoundModem::CWIDTimer()
{
	cwidtimerisActive = false;
	sendCWID(CWIDCall, CWIDType, 0);
	calib_mode[0] = 4;
}

void extSetOffset(int chan)
{
	char valChar[32];
	sprintf(valChar, "%d", chanOffset[chan]);
	chanOffsetLabel[chan]->setText(valChar);

	NeedWaterfallHeaders = true;

	pnt_change[0] = 1;
	pnt_change[1] = 1;
	pnt_change[2] = 1;
	pnt_change[3] = 1;

	return;
}

extern TMgmtMode ** MgmtConnections;
extern int MgmtConCount;
extern QList<QTcpSocket*>  _MgmtSockets;
extern "C" void doAGW2MinTimer();

#define FEND 0xc0
#define QTSMKISSCMD 7

int AGW2MinTimer = 0;

void QtSoundModem::StatsTimer()
{
	// Calculate % Busy over last minute

	for (int n = 0; n < 4; n++)
	{
		if (soundChannel[n] == 0)	// Channel not used
			continue;
		
		AvPTT[n] = PTTActivemS[n] / 600;		// ms but  want %

		PTTActivemS[n] = 0;

		AvBusy[n] = BusyActivemS[n] / 600;
		BusyActivemS[n] = 0;
	
	
	// send to any connected Mgmt streams

		char Msg[64];
		uint64_t ret;

		if (!useKISSControls)
		{

			for (QTcpSocket* socket : _MgmtSockets)
			{
				// Find Session

				TMgmtMode * MGMT = NULL;

				for (int i = 0; i < MgmtConCount; i++)
				{
					if (MgmtConnections[i]->Socket == socket)
					{
						MGMT = MgmtConnections[i];
						break;
					}
				}

				if (MGMT == NULL)
					continue;

				if (MGMT->BPQPort[n])
				{
					sprintf(Msg, "STATS %d %d %d\r", MGMT->BPQPort[n], AvPTT[n], AvBusy[n]);
					ret = socket->write(Msg);
				}
			}
		}
		else			// useKISSControls set
		{
			UCHAR * Control = (UCHAR *)malloc(32);

			int len = sprintf((char *)Control, "%c%cSTATS %d %d%c", FEND, (n) << 4 | QTSMKISSCMD, AvPTT[n], AvBusy[n], FEND);
			KISSSendtoServer(NULL, Control, len);
		}
	}

	AGW2MinTimer++;

	if (AGW2MinTimer > 1)
	{
		AGW2MinTimer = 0;
		doAGW2MinTimer();
	}
}

// PTT Stats

extern "C" void UpdatePTTStats(int Chan, int State)
{
	uint64_t Time = QDateTime::currentMSecsSinceEpoch();

	if (State)
	{
		PTTonTime[Chan] = Time;

		// Cancel Busy timer (stats include ptt on time in port active

		if (BusyonTime[Chan])
		{
			BusyActivemS[Chan] += (Time - BusyonTime[Chan]);
			BusyonTime[Chan] = 0;
		}
	}
	else
	{
		if (PTTonTime[Chan])
		{
			PTTActivemS[Chan] += (Time - PTTonTime[Chan]);
			PTTonTime[Chan] = 0;
		}
	}
}

void QtSoundModem::MyTimerSlot()
{
	// 100 mS Timer Event

	for (int i = 0; i < 4; i++)
	{

		if (needSetOffset[i])
		{
			needSetOffset[i] = 0;
			extSetOffset(i);						// Update GUI
		}
	}

	if (refreshModems)
	{
		refreshModems = 0;

		ui.modeA->setCurrentIndex(speed[0]);
		ui.modeB->setCurrentIndex(speed[1]);
		ui.modeC->setCurrentIndex(speed[2]);
		ui.modeD->setCurrentIndex(speed[3]);
		ui.centerA->setValue(rx_freq[0]);
		ui.centerB->setValue(rx_freq[1]);
		ui.centerC->setValue(rx_freq[2]);
		ui.centerD->setValue(rx_freq[3]);
	}
	if (NeedPSKRefresh)
	{
		NeedPSKRefresh = 0;
		DoPSKWindows();
	}

	if (NeedWaterfallHeaders)
	{
		NeedWaterfallHeaders = 0;

		if (Waterfall)
		{
			Waterfall->fill(black);
			DrawModemFreqRange();
			DrawFreqTicks();
		}
	}

	show_grid();
}

void QtSoundModem::returnPressed()
{
	char Name[32];
	int Chan;
	QString val;
	
	strcpy(Name, sender()->objectName().toUtf8());

	Chan = Name[8] - 'A';

	val = chanOffsetLabel[Chan]->text();

	chanOffset[Chan] = val.toInt();
	needSetOffset[Chan] = 1;				// Update GUI


}

void CheckforChanges(int Mode, int OldMode)
{
	int old48000 = using48000;

	if (OldMode != Mode && Mode == 15)
	{
		QMessageBox msgBox;

		msgBox.setText("Warning!!\nARDOP Packet is NOT the same as ARDOP\n"
			"It is an experimental mode for sending ax.25 frames using ARDOP packet formats\n");

		msgBox.setStandardButtons(QMessageBox::Ok);

		msgBox.exec();
	}

	// See if need to switch beween 12000 and 48000

	using48000 = 0;			// Set if using 48K sample rate (ie RUH Modem active)
	ReceiveSize = 512;
	SendSize = 1024;		// 100 mS for now

	for (int i = 0; i < 4; i++)
	{
		if (soundChannel[i] && (speed[i] == SPEED_RUH48 || speed[i] == SPEED_RUH96))
		{
			using48000 = 1;			// Set if using 48K sample rate (ie RUH Modem active)
			ReceiveSize = 2048;
			SendSize = 4096;		// 100 mS for now
		}
	}

	if (using48000 != old48000)
	{
		InitSound(1);
	}
}


void QtSoundModem::clickedSlotI(int i)
{
	char Name[32];

	strcpy(Name, sender()->objectName().toUtf8());

	if (strcmp(Name, "modeA") == 0)
	{
		int OldModem = ModemA;
		ModemA = ui.modeA->currentIndex();
		set_speed(0, ModemA);
		CheckforChanges(ModemA, OldModem);
		saveSettings();
		AGW_Report_Modem_Change(0);
		return;
	}

	if (strcmp(Name, "modeB") == 0)
	{
		int OldModem = ModemB;
		ModemB = ui.modeB->currentIndex();
		set_speed(1, ModemB);
		CheckforChanges(ModemB, OldModem);
		saveSettings();
		AGW_Report_Modem_Change(1);
		return;
	}

	if (strcmp(Name, "modeC") == 0)
	{
		int OldModem = ModemC;
		ModemC = ui.modeC->currentIndex();
		set_speed(2, ModemC);
		CheckforChanges(ModemC, OldModem);
		saveSettings();
		AGW_Report_Modem_Change(2);
		return;
	}

	if (strcmp(Name, "modeD") == 0)
	{
		int OldModem = ModemD;
		ModemD = ui.modeD->currentIndex();
		set_speed(3, ModemD);
		CheckforChanges(ModemD, OldModem);
		saveSettings();
		AGW_Report_Modem_Change(3);
		return;
	}

	if (strcmp(Name, "centerA") == 0)
	{
		if (i > 299)
		{
			QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
			ui.centerA->setValue(Freq_Change(0, i));
			settings->setValue("Modem/RXFreq1", ui.centerA->value());
			AGW_Report_Modem_Change(0);

		}
		return;
	}

	if (strcmp(Name, "centerB") == 0)
	{
		if (i > 300)
		{
			QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
			ui.centerB->setValue(Freq_Change(1, i));
			settings->setValue("Modem/RXFreq2", ui.centerB->value());
			AGW_Report_Modem_Change(1);
		}
		return;
	}

	if (strcmp(Name, "centerC") == 0)
	{
		if (i > 299)
		{
			QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
			ui.centerC->setValue(Freq_Change(2, i));
			settings->setValue("Modem/RXFreq3", ui.centerC->value());
			AGW_Report_Modem_Change(2);
		}
		return;
	}

	if (strcmp(Name, "centerD") == 0)
	{
		if (i > 299)
		{
			QSettings * settings = new QSettings("QtSoundModem.ini", QSettings::IniFormat);
			ui.centerD->setValue(Freq_Change(3, i));
			settings->setValue("Modem/RXFreq4", ui.centerD->value());
			AGW_Report_Modem_Change(3);
		}
		return;
	}

	if (strcmp(Name, "DCDSlider") == 0)
	{
		dcd_threshold = i;
		BusyDet = i / 10;		// for ardop busy detect code

		saveSettings();
		return;
	}
	
	if (strcmp(Name, "RXOffset") == 0)
	{
		char valChar[32];
		rxOffset = i;
		sprintf(valChar, "RX Offset %d",rxOffset);
		ui.RXOffsetLabel->setText(valChar);

		NeedWaterfallHeaders = true;

		pnt_change[0] = 1;
		pnt_change[1] = 1;
		pnt_change[2] = 1;
		pnt_change[3] = 1;

		saveSettings();
		return;
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("MessageBox Title");
	msgBox.setText("You Clicked " + ((QPushButton*)sender())->objectName());
	msgBox.exec();
}


void QtSoundModem::clickedSlot()
{
	char Name[32];

	strcpy(Name, sender()->objectName().toUtf8());

	if (strcmp(Name, "actDevices") == 0)
	{
		doDevices();
		return;
	}

	if (strcmp(Name, "actModems") == 0)
	{
		doModems();
		return;
	}

	if (strcmp(Name, "showBPF_A") == 0)
	{
		doFilter(0, 0);
		return;
	}

	if (strcmp(Name, "showTXBPF_A") == 0)
	{
		doFilter(0, 1);
		return;
	}

	if (strcmp(Name, "showLPF_A") == 0)
	{
		doFilter(0, 2);
		return;
	}
	

	if (strcmp(Name, "showBPF_B") == 0)
	{
		doFilter(1, 0);
		return;
	}

	if (strcmp(Name, "showTXBPF_B") == 0)
	{
		doFilter(1, 1);
		return;
	}

	if (strcmp(Name, "showLPF_B") == 0)
	{
		doFilter(1, 2);
		return;
	}

	if (strcmp(Name, "Low_A") == 0)
	{
		handleButton(0, 1);
		return;
	}

	if (strcmp(Name, "High_A") == 0)
	{
		handleButton(0, 2);
		return;
	}

	if (strcmp(Name, "Both_A") == 0)
	{
		handleButton(0, 3);
		return;
	}

	if (strcmp(Name, "Stop_A") == 0)
	{
		handleButton(0, 0);
		return;
	}


	if (strcmp(Name, "Low_B") == 0)
	{
		handleButton(1, 1);
		return;
	}

	if (strcmp(Name, "High_B") == 0)
	{
		handleButton(1, 2);
		return;
	}

	if (strcmp(Name, "Both_B") == 0)
	{
		handleButton(1, 3);
		return;
	}

	if (strcmp(Name, "Stop_B") == 0)
	{
		handleButton(1, 0);
		return;
	}

	if (strcmp(Name, "Low_C") == 0)
	{
		handleButton(2, 1);
		return;
	}

	if (strcmp(Name, "High_C") == 0)
	{
		handleButton(2, 2);
		return;
	}

	if (strcmp(Name, "Both_C") == 0)
	{
		handleButton(2, 3);
		return;
	}

	if (strcmp(Name, "Stop_C") == 0)
	{
		handleButton(2, 0);
		return;
	}

	if (strcmp(Name, "Low_D") == 0)
	{
		handleButton(3, 1);
		return;
	}

	if (strcmp(Name, "High_D") == 0)
	{
		handleButton(3, 2);
		return;
	}

	if (strcmp(Name, "Both_D") == 0)
	{
		handleButton(3, 3);
		return;
	}

	if (strcmp(Name, "Stop_D") == 0)
	{
		handleButton(3, 0);
		return;
	}

	if (strcmp(Name, "Cal1500") == 0)
	{
		char call[] = "1500TONE";
		sendCWID(call, 0, 0);
		calib_mode[0] = 4;
		return;
	}



	if (strcmp(Name, "actFont") == 0)
	{
		bool ok;
		Font = QFontDialog::getFont(&ok, QFont(Font, this));

		if (ok)
		{
			// the user clicked OK and font is set to the font the user selected
			QApplication::setFont(Font);
			sessionTable->horizontalHeader()->setFont(Font); 

			saveSettings();
		}
		else
		{
			// the user canceled the dialog; font is set to the initial
			// value, in this case Helvetica [Cronyx], 10

//			QApplication::setFont(Font);

		}
		return;
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("MessageBox Title");
	msgBox.setText("You Clicked " + ((QPushButton*)sender())->objectName());
	msgBox.exec();
}

Ui_ModemDialog * Dlg;

QDialog * modemUI;
QDialog * deviceUI;

void QtSoundModem::doModems()
{
	Dlg = new(Ui_ModemDialog);

	QDialog UI;
	char valChar[10];

	Dlg->setupUi(&UI);

	modemUI = &UI;
	deviceUI = 0;

	myResize *resize = new myResize();

	UI.installEventFilter(resize);

	sprintf(valChar, "%d", bpf[0]);
	Dlg->BPFWidthA->setText(valChar);
	sprintf(valChar, "%d", bpf[1]);
	Dlg->BPFWidthB->setText(valChar);
	sprintf(valChar, "%d", bpf[2]);
	Dlg->BPFWidthC->setText(valChar);
	sprintf(valChar, "%d", bpf[3]);
	Dlg->BPFWidthD->setText(valChar);

	sprintf(valChar, "%d", txbpf[0]);
	Dlg->TXBPFWidthA->setText(valChar);
	sprintf(valChar, "%d", txbpf[1]);
	Dlg->TXBPFWidthB->setText(valChar);
	sprintf(valChar, "%d", txbpf[2]);
	Dlg->TXBPFWidthC->setText(valChar);
	sprintf(valChar, "%d", txbpf[3]);
	Dlg->TXBPFWidthD->setText(valChar);

	sprintf(valChar, "%d", lpf[0]);
	Dlg->LPFWidthA->setText(valChar);
	sprintf(valChar, "%d", lpf[1]);
	Dlg->LPFWidthB->setText(valChar);
	sprintf(valChar, "%d", lpf[2]);
	Dlg->LPFWidthC->setText(valChar);
	sprintf(valChar, "%d", lpf[4]);
	Dlg->LPFWidthD->setText(valChar);

	sprintf(valChar, "%d", BPF_tap[0]);
	Dlg->BPFTapsA->setText(valChar);
	sprintf(valChar, "%d", BPF_tap[1]);
	Dlg->BPFTapsB->setText(valChar);
	sprintf(valChar, "%d", BPF_tap[2]);
	Dlg->BPFTapsC->setText(valChar);
	sprintf(valChar, "%d", BPF_tap[3]);
	Dlg->BPFTapsD->setText(valChar);

	sprintf(valChar, "%d", LPF_tap[0]);
	Dlg->LPFTapsA->setText(valChar);
	sprintf(valChar, "%d", LPF_tap[1]);
	Dlg->LPFTapsB->setText(valChar);
	sprintf(valChar, "%d", LPF_tap[2]);
	Dlg->LPFTapsC->setText(valChar);
	sprintf(valChar, "%d", LPF_tap[3]);
	Dlg->LPFTapsD->setText(valChar);

	Dlg->preEmphAllA->setChecked(emph_all[0]);

	if (emph_all[0])
		Dlg->preEmphA->setDisabled(TRUE);
	else
		Dlg->preEmphA->setCurrentIndex(emph_db[0]);

	Dlg->preEmphAllB->setChecked(emph_all[1]);

	if (emph_all[1])
		Dlg->preEmphB->setDisabled(TRUE);
	else
		Dlg->preEmphB->setCurrentIndex(emph_db[1]);

	Dlg->preEmphAllC->setChecked(emph_all[2]);

	if (emph_all[2])
		Dlg->preEmphC->setDisabled(TRUE);
	else
		Dlg->preEmphC->setCurrentIndex(emph_db[2]);

	Dlg->preEmphAllD->setChecked(emph_all[3]);

	if (emph_all[3])
		Dlg->preEmphD->setDisabled(TRUE);
	else
		Dlg->preEmphD->setCurrentIndex(emph_db[3]);


	Dlg->nonAX25A->setChecked(NonAX25[0]);
	Dlg->nonAX25B->setChecked(NonAX25[1]);
	Dlg->nonAX25C->setChecked(NonAX25[2]);
	Dlg->nonAX25D->setChecked(NonAX25[3]);

	Dlg->KISSOptA->setChecked(KISS_opt[0]);
	Dlg->KISSOptB->setChecked(KISS_opt[1]);
	Dlg->KISSOptC->setChecked(KISS_opt[2]);
	Dlg->KISSOptD->setChecked(KISS_opt[3]);

	sprintf(valChar, "%d", maxframe[0]);
	Dlg->MaxFrameA->setText(valChar);
	sprintf(valChar, "%d", maxframe[1]);
	Dlg->MaxFrameB->setText(valChar);
	sprintf(valChar, "%d", maxframe[2]);
	Dlg->MaxFrameC->setText(valChar);
	sprintf(valChar, "%d", maxframe[3]);
	Dlg->MaxFrameD->setText(valChar);

	sprintf(valChar, "%d", txdelay[0]);
	Dlg->TXDelayA->setText(valChar);
	sprintf(valChar, "%d", txdelay[1]);
	Dlg->TXDelayB->setText(valChar);
	sprintf(valChar, "%d", txdelay[2]);
	Dlg->TXDelayC->setText(valChar);
	sprintf(valChar, "%d", txdelay[3]);
	Dlg->TXDelayD->setText(valChar);


	sprintf(valChar, "%d", txtail[0]);
	Dlg->TXTailA->setText(valChar);
	sprintf(valChar, "%d", txtail[1]);
	Dlg->TXTailB->setText(valChar);
	sprintf(valChar, "%d", txtail[2]);
	Dlg->TXTailC->setText(valChar);
	sprintf(valChar, "%d", txtail[3]);
	Dlg->TXTailD->setText(valChar);

	Dlg->FrackA->setText(QString::number(frack_time[0]));
	Dlg->FrackB->setText(QString::number(frack_time[1]));
	Dlg->FrackC->setText(QString::number(frack_time[2]));
	Dlg->FrackD->setText(QString::number(frack_time[3]));

	Dlg->RetriesA->setText(QString::number(fracks[0]));
	Dlg->RetriesB->setText(QString::number(fracks[1]));
	Dlg->RetriesC->setText(QString::number(fracks[2]));
	Dlg->RetriesD->setText(QString::number(fracks[3]));

	sprintf(valChar, "%d", RCVR[0]);
	Dlg->AddRXA->setText(valChar);
	sprintf(valChar, "%d", RCVR[1]);
	Dlg->AddRXB->setText(valChar);
	sprintf(valChar, "%d", RCVR[2]);
	Dlg->AddRXC->setText(valChar);
	sprintf(valChar, "%d", RCVR[3]);
	Dlg->AddRXD->setText(valChar);

	sprintf(valChar, "%d", rcvr_offset[0]);
	Dlg->RXShiftA->setText(valChar);

	sprintf(valChar, "%d", rcvr_offset[1]);
	Dlg->RXShiftB->setText(valChar);

	sprintf(valChar, "%d", rcvr_offset[2]);
	Dlg->RXShiftC->setText(valChar);
	sprintf(valChar, "%d", rcvr_offset[3]);
	Dlg->RXShiftD->setText(valChar);

	//	speed[1]
	//	speed[2];

	Dlg->recoverBitA->setCurrentIndex(recovery[0]);
	Dlg->recoverBitB->setCurrentIndex(recovery[1]);
	Dlg->recoverBitC->setCurrentIndex(recovery[2]);
	Dlg->recoverBitD->setCurrentIndex(recovery[3]);

	Dlg->fx25ModeA->setCurrentIndex(fx25_mode[0]);
	Dlg->fx25ModeB->setCurrentIndex(fx25_mode[1]);
	Dlg->fx25ModeC->setCurrentIndex(fx25_mode[2]);
	Dlg->fx25ModeD->setCurrentIndex(fx25_mode[3]);

	Dlg->IL2PModeA->setCurrentIndex(il2p_mode[0]);
	Dlg->IL2PModeB->setCurrentIndex(il2p_mode[1]);
	Dlg->IL2PModeC->setCurrentIndex(il2p_mode[2]);
	Dlg->IL2PModeD->setCurrentIndex(il2p_mode[3]);

	Dlg->CRCTX_A->setChecked((il2p_crc[0] & 1));
	Dlg->CRCRX_A->setChecked((il2p_crc[0] & 2));
	Dlg->CRCTX_B->setChecked((il2p_crc[1] & 1));
	Dlg->CRCRX_B->setChecked((il2p_crc[1] & 2));
	Dlg->CRCTX_C->setChecked((il2p_crc[2] & 1));
	Dlg->CRCRX_C->setChecked((il2p_crc[2] & 2));
	Dlg->CRCTX_D->setChecked((il2p_crc[3] & 1));
	Dlg->CRCRX_D->setChecked((il2p_crc[3] & 2));

	Dlg->CWIDCall->setText(CWIDCall);
	Dlg->CWIDInterval->setText(QString::number(CWIDInterval));
	Dlg->CWIDMark->setText(CWIDMark);

	if (CWIDType)
		Dlg->radioButton_2->setChecked(1);
	else
		Dlg->CWIDType->setChecked(1);

	Dlg->afterTraffic->setChecked(afterTraffic);

	Dlg->RSIDSABM_A->setChecked(RSID_SABM[0]);
	Dlg->RSIDSABM_B->setChecked(RSID_SABM[1]);
	Dlg->RSIDSABM_C->setChecked(RSID_SABM[2]);
	Dlg->RSIDSABM_D->setChecked(RSID_SABM[3]);

	Dlg->RSIDUI_A->setChecked(RSID_UI[0]);
	Dlg->RSIDUI_B->setChecked(RSID_UI[1]);
	Dlg->RSIDUI_C->setChecked(RSID_UI[2]);
	Dlg->RSIDUI_D->setChecked(RSID_UI[3]);

	Dlg->DigiCallsA->setText(MyDigiCall[0]);
	Dlg->DigiCallsB->setText(MyDigiCall[1]);
	Dlg->DigiCallsC->setText(MyDigiCall[2]);
	Dlg->DigiCallsD->setText(MyDigiCall[3]);

	Dlg->RSID_1_SETMODEM->setChecked(RSID_SetModem[0]);
	Dlg->RSID_2_SETMODEM->setChecked(RSID_SetModem[1]);
	Dlg->RSID_3_SETMODEM->setChecked(RSID_SetModem[2]);
	Dlg->RSID_4_SETMODEM->setChecked(RSID_SetModem[3]);
	
	connect(Dlg->showBPF_A, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showTXBPF_A, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showLPF_A, SIGNAL(released()), this, SLOT(clickedSlot()));

	connect(Dlg->showBPF_B, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showTXBPF_B, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showLPF_B, SIGNAL(released()), this, SLOT(clickedSlot()));

	connect(Dlg->showBPF_C, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showTXBPF_C, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showLPF_C, SIGNAL(released()), this, SLOT(clickedSlot()));

	connect(Dlg->showBPF_D, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showTXBPF_D, SIGNAL(released()), this, SLOT(clickedSlot()));
	connect(Dlg->showLPF_D, SIGNAL(released()), this, SLOT(clickedSlot()));

	connect(Dlg->okButton, SIGNAL(clicked()), this, SLOT(modemaccept()));
	connect(Dlg->modemSave, SIGNAL(clicked()), this, SLOT(modemSave()));
	connect(Dlg->cancelButton, SIGNAL(clicked()), this, SLOT(modemreject()));

	connect(Dlg->SendRSID_1, SIGNAL(clicked()), this, SLOT(doRSIDA()));
	connect(Dlg->SendRSID_2, SIGNAL(clicked()), this, SLOT(doRSIDB()));
	connect(Dlg->SendRSID_3, SIGNAL(clicked()), this, SLOT(doRSIDC()));
	connect(Dlg->SendRSID_4, SIGNAL(clicked()), this, SLOT(doRSIDD()));

	connect(Dlg->preEmphAllA, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllAChanged(int)));
	connect(Dlg->preEmphAllB, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllBChanged(int)));
	connect(Dlg->preEmphAllC, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllCChanged(int)));
	connect(Dlg->preEmphAllD, SIGNAL(stateChanged(int)), this, SLOT(preEmphAllDChanged(int)));

	UI.exec();
}

void QtSoundModem::preEmphAllAChanged(int state)
{
	Dlg->preEmphA->setDisabled(state);
}

void QtSoundModem::preEmphAllBChanged(int state)
{
	Dlg->preEmphB->setDisabled(state);
}

void QtSoundModem::preEmphAllCChanged(int state)
{
	Dlg->preEmphC->setDisabled(state);
}

void QtSoundModem::preEmphAllDChanged(int state)
{
	Dlg->preEmphD->setDisabled(state);
}

extern "C" void get_exclude_list(char * line, TStringList * list);

void QtSoundModem::modemaccept()
{
	modemSave();

	AGW_Report_Modem_Change(0);
	AGW_Report_Modem_Change(1);
	AGW_Report_Modem_Change(2);
	AGW_Report_Modem_Change(3);

	delete(Dlg);
	saveSettings();

	modemUI->accept();

}

void QtSoundModem::modemSave()
{
	QVariant Q;
	
	emph_all[0] = Dlg->preEmphAllA->isChecked();
	emph_db[0] = Dlg->preEmphA->currentIndex();

	emph_all[1] = Dlg->preEmphAllB->isChecked();
	emph_db[1] = Dlg->preEmphB->currentIndex();

	emph_all[2] = Dlg->preEmphAllC->isChecked();
	emph_db[2] = Dlg->preEmphC->currentIndex();

	emph_all[3] = Dlg->preEmphAllD->isChecked();
	emph_db[3] = Dlg->preEmphD->currentIndex();

	NonAX25[0] = Dlg->nonAX25A->isChecked();
	NonAX25[1] = Dlg->nonAX25B->isChecked();
	NonAX25[2] = Dlg->nonAX25C->isChecked();
	NonAX25[3] = Dlg->nonAX25D->isChecked();

	KISS_opt[0] = Dlg->KISSOptA->isChecked();
	KISS_opt[1] = Dlg->KISSOptB->isChecked();
	KISS_opt[2] = Dlg->KISSOptC->isChecked();
	KISS_opt[3] = Dlg->KISSOptD->isChecked();

	if (emph_db[0] < 0 || emph_db[0] > nr_emph)
		emph_db[0] = 0;

	if (emph_db[1] < 0 || emph_db[1] > nr_emph)
		emph_db[1] = 0;

	if (emph_db[2] < 0 || emph_db[2] > nr_emph)
		emph_db[2] = 0;

	if (emph_db[3] < 0 || emph_db[3] > nr_emph)
		emph_db[3] = 0;

	Q = Dlg->TXDelayA->text();
	txdelay[0] = Q.toInt();

	Q = Dlg->TXDelayB->text();
	txdelay[1] = Q.toInt();
	
	Q = Dlg->TXDelayC->text();
	txdelay[2] = Q.toInt();

	Q = Dlg->TXDelayD->text();
	txdelay[3] = Q.toInt();

	Q = Dlg->MaxFrameA->text();
	maxframe[0] = Q.toInt();

	Q = Dlg->MaxFrameB->text();
	maxframe[1] = Q.toInt();

	Q = Dlg->MaxFrameC->text();
	maxframe[2] = Q.toInt();

	Q = Dlg->MaxFrameD->text();
	maxframe[3] = Q.toInt();

	if (maxframe[0] == 0 || maxframe[0] > 7) maxframe[0] = 3;
	if (maxframe[1] == 0 || maxframe[1] > 7) maxframe[1] = 3;
	if (maxframe[2] == 0 || maxframe[2] > 7) maxframe[2] = 3;
	if (maxframe[3] == 0 || maxframe[3] > 7) maxframe[3] = 3;

	Q = Dlg->TXTailA->text();
	txtail[0] = Q.toInt();

	Q = Dlg->TXTailB->text();
	txtail[1] = Q.toInt();

	Q = Dlg->TXTailC->text();
	txtail[2] = Q.toInt();

	txtail[3] = Dlg->TXTailD->text().toInt();

	frack_time[0] = Dlg->FrackA->text().toInt();
	frack_time[1] = Dlg->FrackB->text().toInt();
	frack_time[2] = Dlg->FrackC->text().toInt();
	frack_time[3] = Dlg->FrackD->text().toInt();

	fracks[0] = Dlg->RetriesA->text().toInt();
	fracks[1] = Dlg->RetriesB->text().toInt();
	fracks[2] = Dlg->RetriesC->text().toInt();
	fracks[3] = Dlg->RetriesD->text().toInt();

	Q = Dlg->AddRXA->text();
	RCVR[0] = Q.toInt();

	Q = Dlg->AddRXB->text();
	RCVR[1] = Q.toInt();

	Q = Dlg->AddRXC->text();
	RCVR[2] = Q.toInt();

	Q = Dlg->AddRXD->text();
	RCVR[3] = Q.toInt();

	Q = Dlg->RXShiftA->text();
	rcvr_offset[0] = Q.toInt();

	Q = Dlg->RXShiftB->text();
	rcvr_offset[1] = Q.toInt();

	Q = Dlg->RXShiftC->text();
	rcvr_offset[2] = Q.toInt();

	Q = Dlg->RXShiftD->text();
	rcvr_offset[3] = Q.toInt();

	fx25_mode[0] = Dlg->fx25ModeA->currentIndex();
	fx25_mode[1] = Dlg->fx25ModeB->currentIndex();
	fx25_mode[2] = Dlg->fx25ModeC->currentIndex();
	fx25_mode[3] = Dlg->fx25ModeD->currentIndex();

	il2p_mode[0] = Dlg->IL2PModeA->currentIndex();
	il2p_mode[1] = Dlg->IL2PModeB->currentIndex();
	il2p_mode[2] = Dlg->IL2PModeC->currentIndex();
	il2p_mode[3] = Dlg->IL2PModeD->currentIndex();

	il2p_crc[0] = Dlg->CRCTX_A->isChecked();
	if (Dlg->CRCRX_A->isChecked())
		il2p_crc[0] |= 2;

	il2p_crc[1] = Dlg->CRCTX_B->isChecked();
	if (Dlg->CRCRX_B->isChecked())
		il2p_crc[1] |= 2;

	il2p_crc[2] = Dlg->CRCTX_C->isChecked();
	if (Dlg->CRCRX_C->isChecked())
		il2p_crc[2] |= 2;

	il2p_crc[3] = Dlg->CRCTX_D->isChecked();
	if (Dlg->CRCRX_D->isChecked())
		il2p_crc[3] |= 2;

	recovery[0] = Dlg->recoverBitA->currentIndex();
	recovery[1] = Dlg->recoverBitB->currentIndex();
	recovery[2] = Dlg->recoverBitC->currentIndex();
	recovery[3] = Dlg->recoverBitD->currentIndex();


	strcpy(CWIDCall, Dlg->CWIDCall->text().toUtf8().toUpper());
	strcpy(CWIDMark, Dlg->CWIDMark->text().toUtf8().toUpper());
	CWIDInterval = Dlg->CWIDInterval->text().toInt();
	CWIDType = Dlg->radioButton_2->isChecked();

	afterTraffic = Dlg->afterTraffic->isChecked();

	if (CWIDInterval && afterTraffic == false)
		cwidtimer->start(CWIDInterval * 60000);
	else
		cwidtimer->stop();


	RSID_SABM[0] = Dlg->RSIDSABM_A->isChecked();
	RSID_SABM[1] = Dlg->RSIDSABM_B->isChecked();
	RSID_SABM[2] = Dlg->RSIDSABM_C->isChecked();
	RSID_SABM[3] = Dlg->RSIDSABM_D->isChecked();

	RSID_UI[0] = Dlg->RSIDUI_A->isChecked();
	RSID_UI[1] = Dlg->RSIDUI_B->isChecked();
	RSID_UI[2] = Dlg->RSIDUI_C->isChecked();
	RSID_UI[3] = Dlg->RSIDUI_D->isChecked();

	RSID_SetModem[0] = Dlg->RSID_1_SETMODEM->isChecked();
	RSID_SetModem[1] = Dlg->RSID_2_SETMODEM->isChecked();
	RSID_SetModem[2] = Dlg->RSID_3_SETMODEM->isChecked();
	RSID_SetModem[3] = Dlg->RSID_4_SETMODEM->isChecked();

	Q = Dlg->DigiCallsA->text();
	strcpy(MyDigiCall[0], Q.toString().toUtf8().toUpper());

	Q = Dlg->DigiCallsB->text();
	strcpy(MyDigiCall[1], Q.toString().toUtf8().toUpper());

	Q = Dlg->DigiCallsC->text();
	strcpy(MyDigiCall[2], Q.toString().toUtf8().toUpper());

	Q = Dlg->DigiCallsD->text();
	strcpy(MyDigiCall[3], Q.toString().toUtf8().toUpper());

	int i;

	for (i = 0; i < 4; i++)
	{
		initTStringList(&list_digi_callsigns[i]);
		get_exclude_list(MyDigiCall[i], &list_digi_callsigns[i]);
	}

/*
	Q = Dlg->LPFWidthA->text();
	lpf[0] = Q.toInt();

	Q = Dlg->LPFWidthB->text();
	lpf[1] = Q.toInt();

	Q = Dlg->LPFWidthC->text();
	lpf[2] = Q.toInt();

	Q = Dlg->LPFWidthD->text();
	lpf[3] = Q.toInt();
*/

}

void QtSoundModem::modemreject()
{
	delete(Dlg);
	modemUI->reject();
}

void QtSoundModem::doRSIDA()
{
	needRSID[0] = 1;
}

void QtSoundModem::doRSIDB()
{
	needRSID[1] = 1;
}

void QtSoundModem::doRSIDC()
{
	needRSID[2] = 1;
}

void QtSoundModem::doRSIDD()
{
	needRSID[3] = 1;
}




void QtSoundModem::doFilter(int Chan, int Filter)
{
	Ui_Dialog Dev;
	QImage * bitmap;

	QDialog UI;

	Dev.setupUi(&UI);

	bitmap = new QImage(642, 312, QImage::Format_RGB32);

	bitmap->fill(qRgb(255, 255, 255));

	QPainter qPainter(bitmap);
	qPainter.setBrush(Qt::NoBrush);
	qPainter.setPen(Qt::black);

	if (Filter == 0)
		make_graph_buf(DET[0][0].BPF_core[Chan], BPF_tap[Chan], &qPainter);
	else if (Filter == 1)
		make_graph_buf(tx_BPF_core[Chan], tx_BPF_tap[Chan], &qPainter);
	else
		make_graph_buf(LPF_core[Chan], LPF_tap[Chan], &qPainter);

	qPainter.end();
	Dev.label->setPixmap(QPixmap::fromImage(*bitmap));

	UI.exec();

}

Ui_devicesDialog * Dev;

char NewPTTPort[80];

int newSoundMode = 0;
int oldSoundMode = 0;
int oldSnoopMix = 0;
int newSnoopMix = 0;

void QtSoundModem::SoundModeChanged(bool State)
{
	UNUSED(State);

	// Mustn't change SoundMode until dialog is accepted

	newSnoopMix = Dev->onlyMixSnoop->isChecked();

	if (Dev->UDP->isChecked())
		newSoundMode = 3;
	else if (Dev->PULSE->isChecked())
		newSoundMode = 2;
	else
		newSoundMode = Dev->OSS->isChecked();

}

void QtSoundModem::DualPTTChanged(bool State)
{
	UNUSED(State);

	// Forse Evaluation of Cat Port setting

	PTTPortChanged(0);
}

void QtSoundModem::CATChanged(bool State)
{
	UNUSED(State);
	PTTPortChanged(0);
}

void QtSoundModem::PTTPortChanged(int Selected)
{
	UNUSED(Selected);

	QVariant Q = Dev->PTTPort->currentText();
	strcpy(NewPTTPort, Q.toString().toUtf8());

	Dev->RTSDTR->setVisible(false);
	Dev->CAT->setVisible(false);

	Dev->PTTOnLab->setVisible(false);
	Dev->PTTOn->setVisible(false);
	Dev->PTTOff->setVisible(false);
	Dev->PTTOffLab->setVisible(false);
	Dev->CATLabel->setVisible(false);
	Dev->CATSpeed->setVisible(false);

	Dev->GPIOLab->setVisible(false);
	Dev->GPIOLeft->setVisible(false);
	Dev->GPIORight->setVisible(false);
	Dev->GPIOLab2->setVisible(false);

	Dev->CM108Label->setVisible(false);
	Dev->VIDPID->setVisible(false);

	if (strcmp(NewPTTPort, "None") == 0)
	{
	}
	else if (strcmp(NewPTTPort, "GPIO") == 0)
	{
		Dev->GPIOLab->setVisible(true);
		Dev->GPIOLeft->setVisible(true);
		if (Dev->DualPTT->isChecked())
		{
			Dev->GPIORight->setVisible(true);
			Dev->GPIOLab2->setVisible(true);
		}
	}

	else if (strcmp(NewPTTPort, "CM108") == 0)
	{
		Dev->CM108Label->setVisible(true);
//#ifdef __ARM_ARCHX
		Dev->CM108Label->setText("CM108 Device");
//#else
//		Dev->CM108Label->setText("CM108 VID/PID");
//#endif
		Dev->VIDPID->setText(CM108Addr);
		Dev->VIDPID->setVisible(true);
	}
	else if (strcmp(NewPTTPort, "HAMLIB") == 0)
	{
		Dev->CM108Label->setVisible(true);
		Dev->CM108Label->setText("rigctrld Port");
		Dev->VIDPID->setText(QString::number(HamLibPort));
		Dev->VIDPID->setVisible(true);
		Dev->PTTOnLab->setText("rigctrld Host");
		Dev->PTTOnLab->setVisible(true);
		Dev->PTTOn->setText(HamLibHost);
		Dev->PTTOn->setVisible(true);
	}
	else if (strcmp(NewPTTPort, "FLRIG") == 0)
	{
		Dev->CM108Label->setVisible(true);
		Dev->CM108Label->setText("FLRig Port");
		Dev->VIDPID->setText(QString::number(FLRigPort));
		Dev->VIDPID->setVisible(true);
		Dev->PTTOnLab->setText("FLRig Host");
		Dev->PTTOnLab->setVisible(true);
		Dev->PTTOn->setText(FLRigHost);
		Dev->PTTOn->setVisible(true);
	}
	else
	{
		Dev->RTSDTR->setVisible(true);
		Dev->CAT->setVisible(true);

		if (Dev->CAT->isChecked())
		{
			Dev->PTTOnLab->setVisible(true);
			Dev->PTTOnLab->setText("PTT On String");
			Dev->PTTOn->setText(PTTOnString);
			Dev->PTTOn->setVisible(true);
			Dev->PTTOff->setVisible(true);
			Dev->PTTOff->setText(PTTOffString);
			Dev->PTTOffLab->setVisible(true);
			Dev->CATLabel->setVisible(true);
			Dev->CATSpeed->setVisible(true);
		}
	}
}

bool myResize::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Resize)
	{
		QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
		QSize size = resizeEvent->size();
		int h = size.height();
		int w = size.width();

		if (obj == deviceUI)
			Dev->scrollArea->setGeometry(QRect(5, 5, w - 10, h - 10));
		else
			Dlg->scrollArea->setGeometry(QRect(5, 5, w - 10, h - 10));

		return true;
	}
	return QObject::eventFilter(obj, event);
}

void QtSoundModem::doDevices()
{
	char valChar[10];
	QStringList items;

	Dev = new(Ui_devicesDialog);

	QDialog UI;

	int i;

	Dev->setupUi(&UI);

	deviceUI = &UI;
	modemUI = 0;

	myResize *resize = new myResize();

	UI.installEventFilter(resize);

	// Set serial names

	for (const QSerialPortInfo &info : Ports)
	{
		items.append(info.portName());
	}

	items.sort();

	Dev->SixPackSerial->addItem("None");

	for (const QString &info : items)
	{
		Dev->SixPackSerial->addItem(info);
	}

	newSoundMode = SoundMode;
	oldSoundMode = SoundMode;
	oldSnoopMix = newSnoopMix = onlyMixSnoop;

#ifdef WIN32
	Dev->ALSA->setText("WaveOut");
	Dev->OSS->setVisible(0);
	Dev->PULSE->setVisible(0);
	Dev->onlyMixSnoop->setVisible(0);
	Dev->ALSA->setChecked(1);
#else
	if (SoundMode == 0)
	{
		Dev->onlyMixSnoop->setVisible(1);
		Dev->ALSA->setChecked(1);
	}
	else if (SoundMode == 1)
		Dev->OSS->setChecked(1);
	else if (SoundMode == 2)
		Dev->PULSE->setChecked(1);
	else if (SoundMode == 2)
		Dev->UDP->setChecked(1);
#endif

	Dev->onlyMixSnoop->setChecked(onlyMixSnoop);

	connect(Dev->ALSA, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->OSS, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->PULSE, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->UDP, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->onlyMixSnoop, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));

	for (i = 0; i < PlaybackCount; i++)
		Dev->outputDevice->addItem(&PlaybackNames[i][0]);

	i = Dev->outputDevice->findText(PlaybackDevice, Qt::MatchContains);


	if (i == -1)
	{
		// Add device to list

		Dev->outputDevice->addItem(PlaybackDevice);
		i = Dev->outputDevice->findText(PlaybackDevice, Qt::MatchContains);
	}

	Dev->outputDevice->setCurrentIndex(i);

	for (i = 0; i < CaptureCount; i++)
		Dev->inputDevice->addItem(&CaptureNames[i][0]);

	i = Dev->inputDevice->findText(CaptureDevice, Qt::MatchContains);

	if (i == -1)
	{
		// Add device to list

		Dev->inputDevice->addItem(CaptureDevice);
		i = Dev->inputDevice->findText(CaptureDevice, Qt::MatchContains);
	}
	Dev->inputDevice->setCurrentIndex(i);

	Dev->txLatency->setText(QString::number(txLatency));

	Dev->Modem_1_Chan->setCurrentIndex(soundChannel[0]);
	Dev->Modem_2_Chan->setCurrentIndex(soundChannel[1]);
	Dev->Modem_3_Chan->setCurrentIndex(soundChannel[2]);
	Dev->Modem_4_Chan->setCurrentIndex(soundChannel[3]);

	// Disable "None" option in first modem

	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(Dev->Modem_1_Chan->model());
	QStandardItem * item = model->item(0, 0);
	item->setEnabled(false);

	Dev->useKISSControls->setChecked(useKISSControls);
	Dev->singleChannelOutput->setChecked(SCO);
	Dev->colourWaterfall->setChecked(raduga);

	sprintf(valChar, "%d", KISSPort);
	Dev->KISSPort->setText(valChar);
	Dev->KISSEnabled->setChecked(KISSServ);

	sprintf(valChar, "%d", AGWPort);
	Dev->AGWPort->setText(valChar);
	Dev->AGWEnabled->setChecked(AGWServ);

	Dev->MgmtPort->setText(QString::number(MgmtPort));

	// If we are using a user specifed device add it

	i = Dev->SixPackSerial->findText(SixPackDevice, Qt::MatchFixedString);

	if (i == -1)
	{
		// Add our device to list

		Dev->SixPackSerial->insertItem(0, SixPackDevice);
		i = Dev->SixPackSerial->findText(SixPackDevice, Qt::MatchContains);
	}

	Dev->SixPackSerial->setCurrentIndex(i);

	sprintf(valChar, "%d", SixPackPort);
	Dev->SixPackTCP->setText(valChar);
	Dev->SixPackEnable->setChecked(SixPackEnable);

	Dev->PTTOn->setText(PTTOnString);
	Dev->PTTOff->setText(PTTOffString);

	sprintf(valChar, "%d", PTTBAUD);
	Dev->CATSpeed->setText(valChar);

	sprintf(valChar, "%d", UDPClientPort);
	Dev->UDPPort->setText(valChar);
	Dev->UDPTXHost->setText(UDPHost);

	if (UDPServerPort != TXPort)
		sprintf(valChar, "%d/%d", UDPServerPort, TXPort);
	else
		sprintf(valChar, "%d", UDPServerPort);

	Dev->UDPTXPort->setText(valChar);

	Dev->UDPEnabled->setChecked(UDPServ);

	sprintf(valChar, "%d", pttGPIOPin);
	Dev->GPIOLeft->setText(valChar);
	sprintf(valChar, "%d", pttGPIOPinR);
	Dev->GPIORight->setText(valChar);

	Dev->VIDPID->setText(CM108Addr);

	connect(Dev->CAT, SIGNAL(toggled(bool)), this, SLOT(CATChanged(bool)));
	connect(Dev->DualPTT, SIGNAL(toggled(bool)), this, SLOT(DualPTTChanged(bool)));
	connect(Dev->PTTPort, SIGNAL(currentIndexChanged(int)), this, SLOT(PTTPortChanged(int)));

	if (PTTMode == PTTCAT)
		Dev->CAT->setChecked(true);
	else
		Dev->RTSDTR->setChecked(true);

	Dev->PTTPort->addItem("None");
	Dev->PTTPort->addItem("CM108");

	//#ifdef __ARM_ARCH

	Dev->PTTPort->addItem("GPIO");

	//#endif

	Dev->PTTPort->addItem("HAMLIB");
	Dev->PTTPort->addItem("FLRIG");

	for (const QString &info : items)
	{
		Dev->PTTPort->addItem(info);
	}

	// If we are using a user specifed device add it

	i = Dev->PTTPort->findText(PTTPort, Qt::MatchFixedString);

	if (i == -1)
	{
		// Add our device to list

		Dev->PTTPort->insertItem(0, PTTPort);
		i = Dev->PTTPort->findText(PTTPort, Qt::MatchContains);
	}

	Dev->PTTPort->setCurrentIndex(i);

	PTTPortChanged(0);				// Force reevaluation

	Dev->txRotation->setChecked(TX_rotate);
	Dev->DualPTT->setChecked(DualPTT);

	Dev->multiCore->setChecked(multiCore);
	Dev->darkTheme->setChecked(darkTheme);

	Dev->WaterfallMin->setCurrentIndex(Dev->WaterfallMin->findText(QString::number(WaterfallMin), Qt::MatchFixedString));
	Dev->WaterfallMax->setCurrentIndex(Dev->WaterfallMax->findText(QString::number(WaterfallMax), Qt::MatchFixedString));

	QObject::connect(Dev->okButton, SIGNAL(clicked()), this, SLOT(deviceaccept()));
	QObject::connect(Dev->cancelButton, SIGNAL(clicked()), this, SLOT(devicereject()));

	UI.exec();

}

void QtSoundModem::mysetstyle()
{
	if (darkTheme)
	{
		qApp->setStyleSheet(
			"QWidget {color: white; background-color: black}"
			"QTabBar::tab {color: rgb(127, 127, 127); background-color: black}"
			"QTabBar::tab::selected {color: white}"
			"QPushButton {border-style: outset; border-width: 2px; border-color: rgb(127, 127, 127)}"
			"QPushButton::default {border-style: outset; border-width: 2px; border-color: white}");

		sessionTable->setStyleSheet("QHeaderView::section { background-color:rgb(40, 40, 40) }");

		txText = qRgb(255, 127, 127);
		rxText = qRgb(173, 216, 230);
	}
	else
	{
		qApp->setStyleSheet("");

		sessionTable->setStyleSheet("QHeaderView::section { background-color:rgb(224, 224, 224) }");

		txText = qRgb(192, 0, 0);
		rxText = qRgb(0, 0, 192);
	}
}

void QtSoundModem::deviceaccept()
{
	QVariant Q = Dev->inputDevice->currentText();
	int cardChanged = 0;
	char portString[32];
	int newMax;
	int newMin;

	if (Dev->UDP->isChecked())
	{
		// cant have server and slave

		if (Dev->UDPEnabled->isChecked())
		{
			QMessageBox::about(this, tr("QtSoundModem"),
				tr("Can't have UDP sound source and UDP server at same time"));
			return;
		}
	}

	if (oldSoundMode != newSoundMode || oldSnoopMix != newSnoopMix)
	{
		QMessageBox msgBox;

		msgBox.setText("QtSoundModem must restart to change Sound Mode.\n"
			"Program will close if you hit Ok\n"
			"You will need to reselect audio devices after restarting");

		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

		int i = msgBox.exec();

		if (i == QMessageBox::Ok)
		{			
			SoundMode = newSoundMode;
			onlyMixSnoop = newSnoopMix;
			saveSettings();

			Closing = 1;
			return;
		}

		if (oldSoundMode == 0)
			Dev->ALSA->setChecked(1);
		else if (oldSoundMode == 1)
			Dev->OSS->setChecked(1);
		else if (oldSoundMode == 2)
			Dev->PULSE->setChecked(1);
		else if (oldSoundMode == 3)
			Dev->UDP->setChecked(1);

		QMessageBox::about(this, tr("Info"),
			tr("<p align = 'center'>Changes not saved</p>"));

		return;

	}

	if (strcmp(CaptureDevice, Q.toString().toUtf8()) != 0)
	{
		strcpy(CaptureDevice, Q.toString().toUtf8());
		cardChanged = 1;
	}

	if (onlyMixSnoop != Dev->onlyMixSnoop->isChecked())
	{
		onlyMixSnoop = Dev->onlyMixSnoop->isChecked();
		cardChanged = 1;
	}

	CaptureIndex = Dev->inputDevice->currentIndex();

	Q = Dev->outputDevice->currentText();

	if (strcmp(PlaybackDevice, Q.toString().toUtf8()) != 0)
	{
		strcpy(PlaybackDevice, Q.toString().toUtf8());
		cardChanged = 1;
	}

	PlayBackIndex = Dev->outputDevice->currentIndex();

	Q = Dev->txLatency->text();
	txLatency = Q.toInt();

	soundChannel[0] = Dev->Modem_1_Chan->currentIndex();
	soundChannel[1] = Dev->Modem_2_Chan->currentIndex();
	soundChannel[2] = Dev->Modem_3_Chan->currentIndex();
	soundChannel[3] = Dev->Modem_4_Chan->currentIndex();

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


	useKISSControls = Dev->useKISSControls->isChecked();
	SCO = Dev->singleChannelOutput->isChecked();
	raduga = Dev->colourWaterfall->isChecked();
	AGWServ = Dev->AGWEnabled->isChecked();
	KISSServ = Dev->KISSEnabled->isChecked();

	Q = Dev->KISSPort->text();
	KISSPort = Q.toInt();

	Q = Dev->AGWPort->text();
	AGWPort = Q.toInt();

	Q = Dev->MgmtPort->text();
	MgmtPort = Q.toInt();

	Q = Dev->SixPackSerial->currentText();

	char temp[256];

	strcpy(temp, Q.toString().toUtf8());

	if (strlen(temp))
		strcpy(SixPackDevice, temp);

	Q = Dev->SixPackTCP->text();
	SixPackPort = Q.toInt();

	SixPackEnable = Dev->SixPackEnable->isChecked();


	Q = Dev->PTTPort->currentText();
	

	strcpy(temp, Q.toString().toUtf8());

	if (strlen(temp))
		strcpy(PTTPort, temp);

	DualPTT = Dev->DualPTT->isChecked();
	TX_rotate = Dev->txRotation->isChecked();
	multiCore = Dev->multiCore->isChecked();
	darkTheme = Dev->darkTheme->isChecked();
	mysetstyle();

	if (Dev->CAT->isChecked())
		PTTMode = PTTCAT;
	else
		PTTMode = PTTRTS;

	Q = Dev->PTTOn->text();
	strcpy(PTTOnString, Q.toString().toUtf8());
	Q = Dev->PTTOff->text();
	strcpy(PTTOffString, Q.toString().toUtf8());

	Q = Dev->CATSpeed->text();
	PTTBAUD = Q.toInt();

	Q = Dev->UDPPort->text();
	UDPClientPort = Q.toInt();


	Q = Dev->UDPTXPort->text();
	strcpy(portString, Q.toString().toUtf8());
	UDPServerPort = atoi(portString);

	if (strchr(portString, '/'))
	{
		char * ptr = strlop(portString, '/');
		TXPort = atoi(ptr);
	}
	else
		TXPort = UDPServerPort;

	Q = Dev->UDPTXHost->text();
	strcpy(UDPHost, Q.toString().toUtf8());

	UDPServ = Dev->UDPEnabled->isChecked();

	Q = Dev->GPIOLeft->text();
	pttGPIOPin = Q.toInt();

	Q = Dev->GPIORight->text();
	pttGPIOPinR = Q.toInt();

	Q = Dev->VIDPID->text();

	if (strcmp(PTTPort, "CM108") == 0)
		strcpy(CM108Addr, Q.toString().toUtf8());
	else if (strcmp(PTTPort, "HAMLIB") == 0)
	{
		HamLibPort = Q.toInt();
		Q = Dev->PTTOn->text();
		strcpy(HamLibHost, Q.toString().toUtf8());
	}
	else if (strcmp(PTTPort, "FLRIG") == 0)
	{
		FLRigPort = Q.toInt();
		Q = Dev->PTTOn->text();
		strcpy(FLRigHost, Q.toString().toUtf8());
	}

	Q = Dev->WaterfallMax->currentText();
	newMax = Q.toInt();

	Q = Dev->WaterfallMin->currentText();
	newMin = Q.toInt();

	if (newMax != WaterfallMax || newMin != WaterfallMin)
	{
		QMessageBox msgBox;

		msgBox.setText("QtSoundModem must restart to change Waterfall range. Program will close if you hit Ok\n"
		"It may take up to 30 seconds for the program to start for the first time after changing settings");

		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

		int i = msgBox.exec();

		if (i == QMessageBox::Ok)
		{
			Configuring = 1;			// Stop Waterfall

			WaterfallMax = newMax;
			WaterfallMin = newMin;
			saveSettings();
			Closing = 1;
			return;
		}
	}


	ClosePTTPort();
	OpenPTTPort();

	NeedWaterfallHeaders = true;

	delete(Dev);
	saveSettings();
	deviceUI->accept();

	if (cardChanged)
	{
		InitSound(1);
	}

	// Reset title and tooltip in case ports changed 

	char Title[128];
	sprintf(Title, "QtSoundModem Version %s Ports %d%s/%d%s", VersionString, AGWPort, AGWServ ? "*": "", KISSPort, KISSServ ? "*" : "");
	w->setWindowTitle(Title);

	sprintf(Title, "QtSoundModem %d %d", AGWPort, KISSPort);
	if (trayIcon)
		trayIcon->setToolTip(Title);

	QSize newSize(this->size());
	QSize oldSize(this->size());

	QResizeEvent *myResizeEvent = new QResizeEvent(newSize, oldSize);

	QCoreApplication::postEvent(this, myResizeEvent);  
}

void QtSoundModem::devicereject()
{	
	delete(Dev);
	deviceUI->reject();
}

void QtSoundModem::handleButton(int Port, int Type)
{
	// interlock calib with CWID

	if (calib_mode[0] == 4)		// CWID
		return;
	
	doCalib(Port, Type);
}




void QtSoundModem::doRestartWF()
{
	return;

	if (((tx_status[0] | tx_status[1] | tx_status[2] | tx_status[3]) != TX_SILENCE) || inWaterfall)
	{
		// in waterfall update thread

		wftimer->start(5000);
		return;
	}

	wftimer->start(1000 * 300);
	
	lockWaterfall = true;

	if (Firstwaterfall | Secondwaterfall)
	{
		initWaterfall(0);
		initWaterfall(1);
	}

	delete(RXLevel);
	delete(ui.RXLevel);
	ui.RXLevel = new QLabel(ui.centralWidget);
	RXLevelCopy = ui.RXLevel;
	ui.RXLevel->setGeometry(QRect(780, 14, 150, 11));
	ui.RXLevel->setFrameShape(QFrame::Box);
	ui.RXLevel->setFrameShadow(QFrame::Sunken);

	delete(RXLevel2);
	delete(ui.RXLevel2);

	ui.RXLevel2 = new QLabel(ui.centralWidget);
	RXLevel2Copy = ui.RXLevel2;

	ui.RXLevel2->setGeometry(QRect(780, 23, 150, 11));
	ui.RXLevel2->setFrameShape(QFrame::Box);
	ui.RXLevel2->setFrameShadow(QFrame::Sunken);

	RXLevel = new QImage(150, 10, QImage::Format_RGB32);
	RXLevel->fill(cyan);

	RXLevel2 = new QImage(150, 10, QImage::Format_RGB32);
	RXLevel2->fill(white);

	ui.RXLevel->setVisible(1);
	ui.RXLevel2->setVisible(1);

	lockWaterfall = false;
}


void QtSoundModem::doAbout()
{
	QMessageBox::about(this, tr("About"),
		tr("G8BPQ's port of UZ7HO's Soundmodem\n\nCopyright (C) 2019-2020 Andrei Kopanchuk UZ7HO"));
}

void QtSoundModem::doCalibrate()
{
	Ui_calDialog Calibrate;
	{
		QDialog UI;
		Calibrate.setupUi(&UI);

		connect(Calibrate.Low_A, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.High_A, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Both_A, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Stop_A, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Low_B, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.High_B, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Both_B, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Stop_B, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Low_C, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.High_C, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Both_C, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Stop_C, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Low_D, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.High_D, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Both_D, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Stop_D, SIGNAL(released()), this, SLOT(clickedSlot()));
		connect(Calibrate.Cal1500, SIGNAL(released()), this, SLOT(clickedSlot()));

		/*
		
		connect(Calibrate.Low_A, &QPushButton::released, this, [=] { handleButton(0, 1); });
		connect(Calibrate.High_A, &QPushButton::released, this, [=] { handleButton(0, 2); });
		connect(Calibrate.Both_A, &QPushButton::released, this, [=] { handleButton(0, 3); });
		connect(Calibrate.Stop_A, &QPushButton::released, this, [=] { handleButton(0, 0); });
		connect(Calibrate.Low_B, &QPushButton::released, this, [=] { handleButton(1, 1); });
		connect(Calibrate.High_B, &QPushButton::released, this, [=] { handleButton(1, 2); });
		connect(Calibrate.Both_B, &QPushButton::released, this, [=] { handleButton(1, 3); });
		connect(Calibrate.Stop_B, &QPushButton::released, this, [=] { handleButton(1, 0); });

//		connect(Calibrate.High_A, SIGNAL(released()), this, SLOT(handleButton(1, 2)));
*/
		UI.exec();
	}
}

void QtSoundModem::RefreshSpectrum(unsigned char * Data)
{
	int i;

	// Last 4 bytes are level busy and Tuning lines

	Waterfall->fill(Black);

	if (Data[206] != LastLevel)
	{
		LastLevel = Data[206];
//		RefreshLevel(LastLevel);
	}

	if (Data[207] != LastBusy)
	{
		LastBusy = Data[207];
//		Busy->setVisible(LastBusy);
	}

	for (i = 0; i < 205; i++)
	{
		int val = Data[0];

		if (val > 63)
			val = 63;

		Waterfall->setPixel(i, val, Yellow);
		if (val < 62)
			Waterfall->setPixel(i, val + 1, Gold);
		Data++;
	}

	ui.Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));

}

void RefreshLevel(unsigned int Level, unsigned int LevelR)
{
	// Redraw the RX Level Bar Graph

	unsigned int  x, y;

	for (x = 0; x < 150; x++)
	{
		for (y = 0; y < 10; y++)
		{
			if (x < Level)
			{
				if (Level < 50)
					RXLevel->setPixel(x, y, yellow);
				else if (Level > 135)
					RXLevel->setPixel(x, y, red);
				else
					RXLevel->setPixel(x, y, green);
			}
			else
				RXLevel->setPixel(x, y, white);
		}
	}
//	RXLevelCopy->setPixmap(QPixmap::fromImage(*RXLevel));

	for (x = 0; x < 150; x++)
	{
		for (y = 0; y < 10; y++)
		{
			if (x < LevelR)
			{
				if (LevelR < 50)
					RXLevel2->setPixel(x, y, yellow);
				else if (LevelR > 135)
					RXLevel2->setPixel(x, y, red);
				else
					RXLevel2->setPixel(x, y, green);
			}
			else
				RXLevel2->setPixel(x, y, white);
		}
	}

	emit t->setLevelImage();

///	RXLevel2Copy->setPixmap(QPixmap::fromImage(*RXLevel2));
}

extern "C" unsigned char CurrentLevel;
extern "C" unsigned char CurrentLevelR;

void QtSoundModem::RefreshWaterfall(int snd_ch, unsigned char * Data)
{
	int j;
	unsigned char * Line;
	int len = Waterfall->bytesPerLine();
	int TopLine = NextWaterfallLine[snd_ch];

	// Write line to cyclic buffer then draw starting with the line just written

	// Length is 208 bytes, including Level and Busy flags

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Data, 206);
	if (NextWaterfallLine[snd_ch] > 63)
		NextWaterfallLine[snd_ch] = 0;

	for (j = 63; j > 0; j--)
	{
		Line = Waterfall->scanLine(j);
		memcpy(Line, &WaterfallLines[snd_ch][TopLine++][0], len);
		if (TopLine > 63)
			TopLine = 0;
	}

	ui.Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));
}

void QtSoundModem::sendtoTrace(char * Msg, int tx)
{
	const QTextCursor old_cursor = monWindowCopy->textCursor();
	const int old_scrollbar_value = monWindowCopy->verticalScrollBar()->value();
	const bool is_scrolled_down = old_scrollbar_value == monWindowCopy->verticalScrollBar()->maximum();

	// Move the cursor to the end of the document.
	monWindowCopy->moveCursor(QTextCursor::End);

	// Insert the text at the position of the cursor (which is the end of the document).

	if (tx)
		monWindowCopy->setTextColor(txText);
	else
		monWindowCopy->setTextColor(rxText);

	monWindowCopy->textCursor().insertText(Msg);

	if (old_cursor.hasSelection() || !is_scrolled_down)
	{
		// The user has selected text or scrolled away from the bottom: maintain position.
		monWindowCopy->setTextCursor(old_cursor);
		monWindowCopy->verticalScrollBar()->setValue(old_scrollbar_value);
	}
	else
	{
		// The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
		monWindowCopy->moveCursor(QTextCursor::End);
		monWindowCopy->verticalScrollBar()->setValue(monWindowCopy->verticalScrollBar()->maximum());
	}

	free(Msg);
}


// I think this does the waterfall

typedef struct TRGBQ_t
{
	Byte b, g, r, re;

} TRGBWQ;

typedef struct tagRECT
{
	int    left;
	int    top;
	int    right;
	int    bottom;
} RECT;

unsigned int RGBWF[256] ;


extern "C" void init_raduga()
{
	Byte offset[6] = {0, 51, 102, 153, 204};
	Byte i, n;

	for (n = 0; n < 52; n++)
	{
		i = n * 5;

		RGBWF[n + offset[0]] = qRgb(0, 0, i);
		RGBWF[n + offset[1]] = qRgb(0, i, 255);
		RGBWF[n + offset[2]] = qRgb(0, 255, 255 - i);
		RGBWF[n + offset[3]] = qRgb(1, 255, 0);
		RGBWF[n + offset[4]] = qRgb(255, 255 - 1, 0);
	}
}

extern "C" int nonGUIMode;


// This draws the Frequency Scale on Waterfall

extern "C" void DrawFreqTicks()
{
	if (nonGUIMode)
		return;

	// Draw Frequency Markers on waterfall header(s);

	int x, i;
	char Textxx[20];
	QImage * bm = Waterfall;

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::black);
	qPainter.setPen(Qt::white);

	int Chan;

#ifdef WIN32
		int Top = 3;
#else
		int Top = 4;
#endif
		int Base = 0;

		for (Chan = 0; Chan < 2; Chan++)
		{
			if (Chan == 1 || ((UsingBothChannels == 0) && (UsingRight == 1)))
				sprintf(Textxx, "Right");
			else
				sprintf(Textxx, "Left");

			qPainter.drawText(2, Top, 100, 20, 0, Textxx);

			// We drew markers every 100 Hz or 100 / binsize pixels

			int Markers = ((WaterfallMax - WaterfallMin) / 100) + 5;			// Number of Markers to draw
			int Freq = WaterfallMin;
			float PixelsPerMarker = 100.0 / BinSize;

			for (i = 0; i < Markers; i++)
			{
				x = round(PixelsPerMarker * i);
				if (x < 1025)
				{
					if ((Freq % 500) == 0)
						qPainter.drawLine(x, Base + 22, x, Base + 15);
					else
						qPainter.drawLine(x, Base + 22, x, Base + 18);

					if ((Freq % 500) == 0)
					{
						sprintf(Textxx, "%d", Freq);

						if (x < 924)
							qPainter.drawText(x - 12, Top, 100, 20, 0, Textxx);
					}
				}
				Freq += 100;
			}

			if (UsingBothChannels == 0)
				break;

			Top += WaterfallTotalPixels;
			Base = WaterfallTotalPixels;
		}

}

// These draws the frequency Markers on the Waterfall

void DrawModemFreqRange()
{
	if (nonGUIMode)
		return;

	// Draws the modem freq bars on waterfall header(s)


	int x1, x2, k, pos1, pos2, pos3;
	QImage * bm = Waterfall;

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::NoBrush);
	qPainter.setPen(Qt::white);

	int Chan;
	int LRtoDisplay = LEFT;
	int top = 0;

	for (Chan = 0; Chan < 2; Chan++)
	{
		if (Chan == 1 || ((UsingBothChannels == 0) && (UsingRight == 1)))
			LRtoDisplay = RIGHT;

		//	bm->fill(black);

	//	qPainter.fillRect(top, 23, 1024, 10, Qt::black);

		// We drew markers every 100 Hz or 100 / binsize pixels

		float PixelsPerHz = 1.0 / BinSize;
		k = 26 + top;

		// draw all enabled ports on the ports on this soundcard

		// First Modem is always on the first waterfall
		// If second is enabled it is on the first unless different
		//		channel from first

		for (int i = 0; i < 4; i++)
		{
			if (soundChannel[i] != LRtoDisplay)
					continue;

			pos1 = roundf((((rxOffset + chanOffset[i] + rx_freq[i]) - 0.5*rx_shift[i]) - WaterfallMin) * PixelsPerHz) - 5;
			pos2 = roundf((((rxOffset + chanOffset[i] + rx_freq[i]) + 0.5*rx_shift[i]) - WaterfallMin) * PixelsPerHz) - 5;
			pos3 = roundf(((rxOffset + chanOffset[i] + rx_freq[i])) - WaterfallMin * PixelsPerHz);
			x1 = pos1 + 5;
			x2 = pos2 + 5;

			qPainter.setPen(Qt::white);
			qPainter.drawLine(x1, k, x2, k);
			qPainter.drawLine(x1, k - 3, x1, k + 3);
			qPainter.drawLine(x2, k - 3, x2, k + 3);
			//		qPainter.drawLine(pos3, k - 3, pos3, k + 3);

			if (rxOffset || chanOffset[i])
			{
				// Draw TX posn if rxOffset used

				pos3 = roundf((rx_freq[i] - WaterfallMin) * PixelsPerHz);
				qPainter.setPen(Qt::magenta);
				qPainter.drawLine(pos3, k - 3, pos3, k + 3);
				qPainter.drawLine(pos3, k - 3, pos3, k + 3);
				qPainter.drawLine(pos3 - 2, k - 3, pos3 + 2, k - 3);
			}

			k += 3;
		}
		if (UsingBothChannels == 0)
			break;

		LRtoDisplay = RIGHT;
		top = WaterfallTotalPixels;
	}
}


void doWaterfallThread(void * param);

extern "C" void doWaterfall(int snd_ch)
{
	if (nonGUIMode)
		return;

	if (Closing)
		return;

	if (lockWaterfall)
		return;

//	if (multiCore)			// Run modems in separate threads
//		_beginthread(doWaterfallThread, 0, xx);
//	else
		doWaterfallThread((void *)(size_t)snd_ch);

}

extern "C" void displayWaterfall()
{
	// if we are using both channels but only want right need to extract correct half of Image

	if (Waterfall == nullptr)
		return;

	if (UsingBothChannels && (Firstwaterfall == 0))
		WaterfallCopy->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
	else
		WaterfallCopy->setAlignment(Qt::AlignTop | Qt::AlignLeft);

//	WaterfallCopy->setPixmap(QPixmap::fromImage(*Waterfall));

	emit t->setWaterfallImage();
}

extern "C" float aFFTAmpl[1024];
extern "C" void SMUpdateBusyDetector(int LR, float * Real, float *Imag);

void doWaterfallThread(void * param)
{
	int snd_ch = (int)(size_t)param;

	if (lockWaterfall)
		return;

	if (Configuring)
		return;

	if (inWaterfall)
		return;

	inWaterfall = true;					// don't allow restart waterfall

	if (snd_ch == 1 && UsingLeft == 0)	// Only using right
		snd_ch = 0;						// Samples are in first buffer

	QImage * bm = Waterfall;

	int  i;
	single  mag;
	UCHAR * p;
	UCHAR Line[4096] = "";			// 4 bytes per pixel

	int lineLen, Start, End;
	word  hFFTSize;
	Byte  n;
	float RealOut[8192] = { 0 };
	float ImagOut[8192];


	RefreshLevel(CurrentLevel, CurrentLevelR);	// Signal Level

	hFFTSize = FFTSize / 2;


	// I think an FFT should produce n/2 bins, each of Samp/n Hz
	// Looks like my code only works with n a power of 2

	// So can use 1024 or 4096. 1024 gives 512 bins of 11.71875 and a 512 pixel 
	// display (is this enough?)

	Start = (WaterfallMin / BinSize);		// First and last bins to process
	End = (WaterfallMax / BinSize);

	if (0)	//RSID_WF
	{
		// Use the Magnitudes in float aFFTAmpl[RSID_FFT_SIZE];

		for (i = 0; i < hFFTSize; i++)
		{
			mag = aFFTAmpl[i];

			mag *= 0.00000042f;

			if (mag < 0.00001f)
				mag = 0.00001f;

			if (mag > 1.0f)
				mag = 1.0f;

			mag = 22 * log2f(mag) + 255;

			if (mag < 0)
				mag = 0;

			fft_disp[snd_ch][i] = round(mag);
		}
	}
	else
	{
		dofft(&fft_buf[snd_ch][0], RealOut, ImagOut);

		//	FourierTransform(1024, &fft_buf[snd_ch][0], RealOut, ImagOut, 0);

		for (i = Start; i < End; i++)
		{
			//mag: = ComplexMag(fft_d[i])*0.00000042;

	//		mag = sqrtf(powf(RealOut[i], 2) + powf(ImagOut[i], 2)) * 0.00000042f;

			mag = powf(RealOut[i], 2);
			mag += powf(ImagOut[i], 2);
			mag = sqrtf(mag);
			mag *= 0.00000042f;


			if (mag > MaxMagOut)
			{
				MaxMagOut = mag;
				MaxMagIndex = i;
			}

			if (mag < 0.00001f)
				mag = 0.00001f;

			if (mag > 1.0f)
				mag = 1.0f;

			mag = 22 * log2f(mag) + 255;

			if (mag < 0)
				mag = 0;

			MagOut[i] = mag;					// for Freq Guess
			fft_disp[snd_ch][i] = round(mag);
		}
	}

	SMUpdateBusyDetector(snd_ch, RealOut, ImagOut);



	// we always do fft so we can get centre freq and do busy detect. But only update waterfall if on display

	if (bm == 0)
	{
		inWaterfall = false;
		return;
	}
	if ((Firstwaterfall == 0 && snd_ch == 0) || (Secondwaterfall == 0 && snd_ch == 1))
	{
		inWaterfall = false;
		return;
	}

	p = Line;
	lineLen = 4096;

	if (raduga == DISP_MONO)
	{
		for (i = Start; i < End; i++)
		{
			n = fft_disp[snd_ch][i];
			*(p++) = n;					// all colours the same
			*(p++) = n;
			*(p++) = n;
			p++;
		}
	}
	else
	{
		for (i = Start; i < End; i++)
		{
			n = fft_disp[snd_ch][i];
			memcpy(p, &RGBWF[n], 4);
			p += 4;
		}
	}

	// Scroll



	int TopLine = NextWaterfallLine[snd_ch];
	int TopScanLine = WaterfallHeaderPixels;

	if (snd_ch)
		TopScanLine += WaterfallTotalPixels;

	// Write line to cyclic buffer then draw starting with the line just written

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Line, 4096);
	if (NextWaterfallLine[snd_ch] > 79)
		NextWaterfallLine[snd_ch] = 0;
	
	// Sanity check

	if ((79 + TopScanLine) >= bm->height())
	{
		printf("Invalid WFMaxLine %d \n", bm->height());
			exit(1);
	}


	for (int j = 79; j > 0; j--)
	{
		p = bm->scanLine(j + TopScanLine);
		if (p == nullptr)
		{
			printf("Invalid WF Pointer \n");
			exit(1);
		}
		memcpy(p, &WaterfallLines[snd_ch][TopLine][0], lineLen);
		TopLine++;
		if (TopLine > 79)
			TopLine = 0;
	}

	inWaterfall = false;
}

void QtSoundModem::setWaterfallImage()
{
	ui.Waterfall->setPixmap(QPixmap::fromImage(*Waterfall));
}

void QtSoundModem::setLevelImage()
{
	RXLevelCopy->setPixmap(QPixmap::fromImage(*RXLevel));
	RXLevel2Copy->setPixmap(QPixmap::fromImage(*RXLevel2));
}

void QtSoundModem::setConstellationImage(int chan, int Qual)
{
	char QualText[64];
	sprintf(QualText, "Chan %c Qual = %d", chan + 'A', Qual);
	QualLabel[chan]->setText(QualText);
	constellationLabel[chan]->setPixmap(QPixmap::fromImage(*Constellation[chan]));
}



void QtSoundModem::changeEvent(QEvent* e)
{
	if (e->type() == QEvent::WindowStateChange)
	{
		QWindowStateChangeEvent* ev = static_cast<QWindowStateChangeEvent*>(e);

		qDebug() << windowState();

		if (!(ev->oldState() & Qt::WindowMinimized) && windowState() & Qt::WindowMinimized)
		{
			if (trayIcon)
				setVisible(false);
		}
//		if (!(ev->oldState() != Qt::WindowNoState) && windowState() == Qt::WindowNoState)
//		{
//			QMessageBox::information(this, "", "Window has been restored");
//		}

	}
	QWidget::changeEvent(e);
}

#include <QCloseEvent>

void QtSoundModem::closeEvent(QCloseEvent *event)
{
	UNUSED(event);

	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", QWidget::saveGeometry());
	mysettings.setValue("windowState", saveState());

	Closing = TRUE;
	qDebug() << "Closing";

	QThread::msleep(100);
}

	
QtSoundModem::~QtSoundModem()
{
	qDebug() << "Saving Settings";

	closeTraceLog();
		
	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);
	mysettings.setValue("geometry", saveGeometry());
	mysettings.setValue("windowState", saveState());
	
	saveSettings();	
	Closing = TRUE;
	qDebug() << "Closing";

	QThread::msleep(100);
}

extern "C" void QSleep(int ms)
{
	QThread::msleep(ms);
}

int upd_time = 30;

void QtSoundModem::show_grid()
{
	// This refeshes the session list

	int  snd_ch, i, num_rows, row_idx;
	QTableWidgetItem *item;
	const char * msg = "";

	int  speed_tx, speed_rx;

	if (grid_time < 10)
	{
		grid_time++;
		return;
	}

	grid_time = 0;

	//label7.Caption = inttostr(stat_r_mem); mem_arq

	num_rows = 0;
	row_idx = 0;

	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		for (i = 0; i < port_num; i++)
		{
			if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
				num_rows++;
		}
	}

	if (num_rows == 0)
	{
		sessionTable->clearContents();
		sessionTable->setRowCount(0);
		sessionTable->setRowCount(1);
	}
	else
		sessionTable->setRowCount(num_rows);


	for (snd_ch = 0; snd_ch < 4; snd_ch++)
	{
		for (i = 0; i < port_num; i++)
		{
			if (AX25Port[snd_ch][i].status != STAT_NO_LINK)
			{
				switch (AX25Port[snd_ch][i].status)
				{
				case STAT_NO_LINK:

					msg = "No link";
					break;

				case STAT_LINK:

					msg = "Link";
					break;

				case STAT_CHK_LINK:

					msg = "Chk link";
					break;

				case STAT_WAIT_ANS:

					msg = "Wait ack";
					break;

				case STAT_TRY_LINK:

					msg = "Try link";
					break;

				case STAT_TRY_UNLINK:

					msg = "Try unlink";
				}


				item = new QTableWidgetItem((char *)AX25Port[snd_ch][i].mycall);
				sessionTable->setItem(row_idx, 0, item);

				item = new QTableWidgetItem(AX25Port[snd_ch][i].kind);
				sessionTable->setItem(row_idx, 11, item);

				item = new QTableWidgetItem((char *)AX25Port[snd_ch][i].corrcall);
				sessionTable->setItem(row_idx, 1, item);

				item = new QTableWidgetItem(msg);
				sessionTable->setItem(row_idx, 2, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_s_pkt));
				sessionTable->setItem(row_idx, 3, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_s_byte));
				sessionTable->setItem(row_idx, 4, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_pkt));
				sessionTable->setItem(row_idx, 5, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_byte));
				sessionTable->setItem(row_idx, 6, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_r_fc));
				sessionTable->setItem(row_idx, 7, item);

				item = new QTableWidgetItem(QString::number(AX25Port[snd_ch][i].info.stat_fec_count));
				sessionTable->setItem(row_idx, 8, item);

				if (grid_timer != upd_time)
					grid_timer++;
				else
				{
					grid_timer = 0;
					speed_tx = round(abs(AX25Port[snd_ch][i].info.stat_s_byte - AX25Port[snd_ch][i].info.stat_l_s_byte) / upd_time);
					speed_rx = round(abs(AX25Port[snd_ch][i].info.stat_r_byte - AX25Port[snd_ch][i].info.stat_l_r_byte) / upd_time);

					item = new QTableWidgetItem(QString::number(speed_tx));
					sessionTable->setItem(row_idx, 9, item);

					item = new QTableWidgetItem(QString::number(speed_rx));
					sessionTable->setItem(row_idx, 10, item);

					AX25Port[snd_ch][i].info.stat_l_r_byte = AX25Port[snd_ch][i].info.stat_r_byte;
					AX25Port[snd_ch][i].info.stat_l_s_byte = AX25Port[snd_ch][i].info.stat_s_byte;	
				}

				row_idx++;
			}
		}
	}
}

// "Copy on Select" Code

void QtSoundModem::onTEselectionChanged()
{
	QTextEdit * x = static_cast<QTextEdit*>(QObject::sender());
	x->copy();
}

#define ConstellationHeight 121
#define ConstellationWidth 121
#define PLOTRADIUS 60

#define MAX(x, y) ((x) > (y) ? (x) : (y))

extern "C" int SMUpdatePhaseConstellation(int chan, float * Phases, float * Mags, int intPSKPhase, int Count)
{
	// Subroutine to update Constellation plot for PSK modes...
	// Skip plotting and calculations of intPSKPhase(0) as this is a reference phase (9/30/2014)

	float dblPhaseError;
	float dblPhaseErrorSum = 0;
	float intP = 0;
	float dblRad = 0;
	float dblAvgRad = 0;
	float dbPhaseStep;
	float MagMax = 0;
	float dblPlotRotation = 0;

	int i, intQuality;

	int intX, intY;
	int yCenter = (ConstellationHeight - 2) / 2;
	int xCenter = (ConstellationWidth - 2) / 2;

	if (Count == 0)
		return 0;

	if (nonGUIMode == 0)
	{
		Constellation[chan]->fill(black);

		for (i = 0; i < 120; i++)
		{
			Constellation[chan]->setPixel(xCenter, i, cyan);
			Constellation[chan]->setPixel(i, xCenter, cyan);
		}
	}
	dbPhaseStep = 2 * M_PI / intPSKPhase;

	for (i = 1; i < Count; i++)  // Don't plot the first phase (reference)
	{
		MagMax = MAX(MagMax, Mags[i]); // find the max magnitude to auto scale
		dblAvgRad += Mags[i];
	}

	dblAvgRad = dblAvgRad / Count; // the average radius

	for (i = 0; i < Count; i++)
	{
		dblRad = PLOTRADIUS * Mags[i] / MagMax; //  scale the radius dblRad based on intMag
		intP = round((Phases[i]) / dbPhaseStep);

		// compute the Phase error

		dblPhaseError = fabsf(Phases[i] - intP * dbPhaseStep); // always positive and < .5 *  dblPhaseStep
		dblPhaseErrorSum += dblPhaseError;

		if (nonGUIMode == 0)
		{
			intX = xCenter + dblRad * cosf(dblPlotRotation + Phases[i]);
			intY = yCenter + dblRad * sinf(dblPlotRotation + Phases[i]);

			if (intX > 0 && intY > 0)
				if (intX != xCenter && intY != yCenter)
					Constellation[chan]->setPixel(intX, intY, yellow);
		}
	}

	dblAvgRad = dblAvgRad / Count; // the average radius

	intQuality = MAX(0, ((100 - 200 * (dblPhaseErrorSum / (Count)) / dbPhaseStep))); // ignore radius error for (PSK) but include for QAM

	if (nonGUIMode == 0)
	{
		emit t->setConstellationImage(chan, intQuality);
//		char QualText[64];
//		sprintf(QualText, "Chan %c Qual = %d", chan + 'A', intQuality);
//		QualLabel[chan]->setText(QualText);
//		constellationLabel[chan]->setPixmap(QPixmap::fromImage(*Constellation[chan]));
	}
	return intQuality;
}


QFile tracefile("Tracelog.txt");


extern "C" int openTraceLog()
{
	if (!tracefile.open(QIODevice::Append | QIODevice::Text))
		return 0;

	return 1;
}

extern "C" qint64 writeTraceLog(char * Data)
{
	return tracefile.write(Data);
}

extern "C" void closeTraceLog()
{
	tracefile.close();
}

extern "C" void debugTimeStamp(char * Text, char Dirn)
{
#ifndef LOGTX

	if (Dirn == 'T')
		return;

#endif

#ifndef LOGRX

	if (Dirn == 'R')
		return;

#endif


	QTime Time(QTime::currentTime());
	QString String = Time.toString("hh:mm:ss.zzz");
	char Msg[2048];

	sprintf(Msg, "%s %s\n", String.toUtf8().data(), Text);
	writeTraceLog(Msg);
}



// Timer functions need to run in GUI Thread

extern "C" int SampleNo;


extern "C" int pttOnTime()
{
	return pttOnTimer.elapsed();
}

extern "C" void startpttOnTimer()
{
	pttOnTimer.start();
}


extern "C" void StartWatchdog()
{
	// Get Monotonic clock for PTT drop time calculation

#ifndef WIN32
	clock_gettime(CLOCK_MONOTONIC, &pttclk);
#endif	
	debugTimeStamp((char *)"PTT On", 'T');
	emit t->startWatchdog();
	pttOnTimer.start();
}

extern "C" void StopWatchdog()
{
	int txlenMs = (1000 * SampleNo / TX_Samplerate);

	Debugprintf("Samples Sent %d, Calc Time %d, PTT Time %d", SampleNo, txlenMs, pttOnTime());
	debugTimeStamp((char *)"PTT Off", 'T');
	closeTraceLog();
	openTraceLog();
	debugTimeStamp((char *)"Log Reopened", 'T');

	emit t->stopWatchdog();
}



void QtSoundModem::StartWatchdog()
{
	PTTWatchdog->start(60 * 1000);
}

 void QtSoundModem::StopWatchdog()
{
	 PTTWatchdog->stop();
}


 void QtSoundModem::PTTWatchdogExpired()
 {
	 PTTWatchdog->stop();
 }


 // KISS Serial Port code - mainly for 6Pack but should work with KISS as well

 // Serial Read needs to block and signal the main thread whenever a character is received. TX can probably be uncontrolled

 void serialThread::startSlave(const QString &portName, int waitTimeout, const QString &response)
 {
	 QMutexLocker locker(&mutex);
	 this->portName = portName;
	 this->waitTimeout = waitTimeout;
	 this->response = response;
	 if (!isRunning())
		 start();
 }

 void serialThread::run()
 {
	 QSerialPort serial;
	 bool currentPortNameChanged = false;

	 mutex.lock();
	 QString currentPortName;
	 if (currentPortName != portName) {
		 currentPortName = portName;
		 currentPortNameChanged = true;
	 }

	 int currentWaitTimeout = waitTimeout;
	 QString currentRespone = response;
	 mutex.unlock();

	 if (currentPortName.isEmpty())
	 {
		 Debugprintf("Port not set");
		 return;
	 }

	 serial.setPortName(currentPortName);

	 if (!serial.open(QIODevice::ReadWrite))
	 {
		 Debugprintf("Can't open %s, error code %d", portName, serial.error());
		 return;
	 }
 
	 while (1)
	 {
		
		 if (serial.waitForReadyRead(currentWaitTimeout))
		 {
			 // read request
			 QByteArray requestData = serial.readAll();
			 while (serial.waitForReadyRead(10))
				 requestData += serial.readAll();

			 // Pass data to 6pack handler

			 emit this->request(requestData);

		 }
		 else {
			 Debugprintf("Serial read request timeout");
		 }
	 }
 }

 void Process6PackData(unsigned char * Bytes, int Len);

 void QtSoundModem::showRequest(QByteArray Data)
 {
	 Process6PackData((unsigned char *)Data.data(), Data.length());
 }

 