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

#include "UZ7HOStuff.h"


QImage *Constellation;
QImage *Waterfall[4] = { 0,0,0,0 };
QImage *Header[4];
QLabel *DCDLabel[4];
QLineEdit *chanOffsetLabel[4];
QImage *DCDLed[4];

QImage *RXLevel;

QLabel *WaterfallCopy[2];
QLabel *HeaderCopy[2];

QTextEdit * monWindowCopy;

extern workerThread *t;
extern QtSoundModem * w;

QList<QSerialPortInfo> Ports = QSerialPortInfo::availablePorts();

void saveSettings();
void getSettings();
extern "C" void CloseSound();
extern "C" void GetSoundDevices();
extern "C" char modes_name[modes_count][20];
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
extern "C" int multiCore;

extern "C" int refreshModems;

extern "C" int pnt_change[5];
extern "C" int needRSID[4];

extern "C" int needSetOffset[4];

extern "C" float MagOut[4096];
extern "C" float MaxMagOut;
extern "C" int MaxMagIndex;

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
	void wf_pointer(int snd_ch);
	void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);
	void dofft(short * in, float * outr, float * outi);
	void init_raduga();
	void wf_Scale(int Chan);
	void AGW_Report_Modem_Change(int port);
	char * strlop(char * buf, char delim);
	void sendRSID(int Chan, int dropTX);
	void RSIDinitfft();
	void il2p_init(int il2p_debug);
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
int CWIDInterval = 0;
int CWIDLeft = 0;
int CWIDRight = 0;
int CWIDType = 1;			// on/off

extern "C" { int RSID_SABM[4]; }
extern "C" { int RSID_UI[4]; }
extern "C" { int RSID_SetModem[4]; }

int Closing = FALSE;				// Set to stop background thread

QRgb white = qRgb(255, 255, 255);
QRgb black = qRgb(0, 0, 0);

QRgb green = qRgb(0, 255, 0);
QRgb red = qRgb(255, 0, 0);
QRgb yellow = qRgb(255, 255, 0);
QRgb cyan = qRgb(0, 255, 255);

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
int NextWaterfallLine[2] = { 0 };

unsigned int LastLevel = 255;
unsigned int LastBusy = 255;

extern "C" int UDPClientPort;
extern "C" int UDPServerPort;
extern "C" int TXPort;
extern char UDPHost[64];

QTimer *cwidtimer;

QSystemTrayIcon * trayIcon = nullptr;

int MintoTray = 1;

int RSID_WF = 0;				// Set to use RSID FFT for Waterfall. 

extern "C" void WriteDebugLog(char * Mess)
{
	qDebug() << Mess;
}

void QtSoundModem::doupdateDCD(int Chan, int State)
{
	DCDLabel[Chan]->setVisible(State);
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

	int A, B, C, W;
	int modemBoxHeight = 30;

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


	A = r.height() - 25;   // No waterfalls

	if (UsingBothChannels && Secondwaterfall)
	{
		// Two waterfalls

		ui.WaterfallA->setVisible(1);
		ui.HeaderA->setVisible(1);
		ui.WaterfallB->setVisible(1);
		ui.HeaderB->setVisible(1);

		A = r.height() - 258;   // Top of Waterfall A
		B = A + 115;			// Top of Waterfall B
	}
	else
	{
		// One waterfall

		// Could be Left or Right

		if (Firstwaterfall)
		{
			if (soundChannel[0] == RIGHT)
			{
				ui.WaterfallA->setVisible(0);
				ui.HeaderA->setVisible(0);
				ui.WaterfallB->setVisible(1);
				ui.HeaderB->setVisible(1);
			}
			else
			{
				ui.WaterfallA->setVisible(1);
				ui.HeaderA->setVisible(1);
				ui.WaterfallB->setVisible(0);
				ui.HeaderB->setVisible(0);
			}

			A = r.height() - 145;   // Top of Waterfall A
		}
		else
			A = r.height() - 25;   // Top of Waterfall A
	}

	C = A - 150;			// Bottom of Monitor, Top of connection list
	W = r.width();

	// Calc Positions of Waterfalls

	ui.monWindow->setGeometry(QRect(0, modemBoxHeight, W, C - (modemBoxHeight + 26)));
	sessionTable->setGeometry(QRect(0, C, W, 175));

	if (UsingBothChannels)
	{
		ui.HeaderA->setGeometry(QRect(0, A, W, 35));
		ui.WaterfallA->setGeometry(QRect(0, A + 35, W, 80));
		ui.HeaderB->setGeometry(QRect(0, B, W, 35));
		ui.WaterfallB->setGeometry(QRect(0, B + 35, W, 80));
	}
	else
	{
		if (soundChannel[0] == RIGHT)
		{
			ui.HeaderB->setGeometry(QRect(0, A, W, 35));
			ui.WaterfallB->setGeometry(QRect(0, A + 35, W, 80));
		}
		else
		{
			ui.HeaderA->setGeometry(QRect(0, A, W, 35));
			ui.WaterfallA->setGeometry(QRect(0, A + 35, W, 80));
		}
	}
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
	{
		int oldstate = Firstwaterfall;
		Firstwaterfall = state;

		if (state != oldstate)
			initWaterfall(0, state);

	}
	else if (Act == actWaterfall2)
	{
		int oldstate = Secondwaterfall;
		Secondwaterfall = state;

		if (state != oldstate)
			initWaterfall(1, state);

	}
	saveSettings();
}

void QtSoundModem::initWaterfall(int chan, int state)
{
	if (state == 1)
	{
		if (chan == 0)
		{
			ui.WaterfallA = new QLabel(ui.centralWidget);
			WaterfallCopy[0] = ui.WaterfallA;
		}
		else
		{
			ui.WaterfallB = new QLabel(ui.centralWidget);
			WaterfallCopy[1] = ui.WaterfallB;
		}
		Waterfall[chan] = new QImage(1024, 80, QImage::Format_RGB32);
		Waterfall[chan]->fill(black);

	}
	else
	{
		delete(Waterfall[chan]);
		Waterfall[chan] = 0;
	}

	QSize Size(800, 602);						// Not actually used, but Event constructor needs it
	QResizeEvent *event = new QResizeEvent(Size, Size);
	QApplication::sendEvent(this, event);
}

// Local copies

QLabel *RXOffsetLabel;
QSlider *RXOffset;

QtSoundModem::QtSoundModem(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);

	QSettings mysettings("QtSoundModem.ini", QSettings::IniFormat);

	if (MintoTray)
	{
		char popUp[256];
		sprintf(popUp, "QtSoundModem %d %d", AGWPort, KISSPort);
		trayIcon = new QSystemTrayIcon(QIcon(":/QtSoundModem/soundmodem.ico"), this);
		trayIcon->setToolTip(popUp);
		trayIcon->show();

		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(TrayActivated(QSystemTrayIcon::ActivationReason)));
	}


	restoreGeometry(mysettings.value("geometry").toByteArray());
	restoreState(mysettings.value("windowState").toByteArray());

	sessionTable = new QTableWidget(this);

	sessionTable->verticalHeader()->setVisible(FALSE);
	sessionTable->verticalHeader()->setDefaultSectionSize(20);
	sessionTable->horizontalHeader()->setDefaultSectionSize(68);
	sessionTable->setRowCount(1);
	sessionTable->setColumnCount(12);
	m_TableHeader << "MyCall" << "DestCall" << "Status" << "Sent pkts" << "Sent Bytes" << "Rcvd pkts" << "Rcvd bytes" << "Rcvd FC" << "FEC corr" << "CPS TX" << "CPS RX" << "Direction";

	sessionTable->setStyleSheet("QHeaderView::section { background-color:rgb(224, 224, 224) }");

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

	//	Constellation = new QImage(91, 91, QImage::Format_RGB32);

	Header[0] = new QImage(1024, 35, QImage::Format_RGB32);
	Header[1] = new QImage(1024, 35, QImage::Format_RGB32);
	RXLevel = new QImage(150, 10, QImage::Format_RGB32);

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


	//	Waterfall[0]->setColorCount(16);
	//	Waterfall[1]->setColorCount(16);


	//	for (i = 0; i < 16; i++)
	//	{
	//	Waterfall[0]->setColor(i, vbColours[i]);
	//		Waterfall[1]->setColor(i, vbColours[i]);
	//	}

	WaterfallCopy[0] = ui.WaterfallA;
	WaterfallCopy[1] = ui.WaterfallB;

	initWaterfall(0, 1);
	initWaterfall(1, 1);

	Header[0]->fill(black);
	Header[1]->fill(black);

	HeaderCopy[0] = ui.HeaderA;
	HeaderCopy[1] = ui.HeaderB;
	monWindowCopy = ui.monWindow;

	ui.monWindow->document()->setMaximumBlockCount(10000);

//	connect(ui.monWindow, SIGNAL(selectionChanged()), this, SLOT(onTEselectionChanged()));

	ui.HeaderA->setPixmap(QPixmap::fromImage(*Header[0]));
	ui.HeaderB->setPixmap(QPixmap::fromImage(*Header[1]));

	wf_pointer(soundChannel[0]);
	wf_pointer(soundChannel[1]);
	wf_Scale(0);
	wf_Scale(1);

	//	RefreshLevel(0);
	//	RXLevel->setPixmap(QPixmap::fromImage(*RXLevel));

	connect(ui.modeA, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeB, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeC, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));
	connect(ui.modeD, SIGNAL(currentIndexChanged(int)), this, SLOT(clickedSlotI(int)));

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

	connect(ui.RXOffsetA, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetB, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetC, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
	connect(ui.RXOffsetD, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);


	cwidtimer = new QTimer(this);
	connect(cwidtimer, SIGNAL(timeout()), this, SLOT(CWIDTimer()));

	if (CWIDInterval)
		cwidtimer->start(CWIDInterval * 60000);

	if (RSID_SetModem[0])
	{
		RSID_WF = 1;
		RSIDinitfft();
	}
	il2p_init(1);
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

void QtSoundModem::CWIDTimer()
{
	sendCWID(CWIDCall, CWIDType, 0);
	calib_mode[0] = 4;
}

void extSetOffset(int chan)
{
	char valChar[32];
	sprintf(valChar, "%d", chanOffset[chan]);
	chanOffsetLabel[chan]->setText(valChar);

	wf_pointer(soundChannel[chan]);

	pnt_change[0] = 1;
	pnt_change[1] = 1;
	pnt_change[2] = 1;
	pnt_change[3] = 1;

	return;
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


void QtSoundModem::clickedSlotI(int i)
{
	char Name[32];

	strcpy(Name, sender()->objectName().toUtf8());

	if (strcmp(Name, "modeA") == 0)
	{
		ModemA = ui.modeA->currentIndex();
		set_speed(0, ModemA);
		saveSettings();
		AGW_Report_Modem_Change(0);
		return;
	}

	if (strcmp(Name, "modeB") == 0)
	{
		ModemB = ui.modeB->currentIndex();
		set_speed(1, ModemB);
		saveSettings();
		AGW_Report_Modem_Change(1);
		return;
	}

	if (strcmp(Name, "modeC") == 0)
	{
		ModemC = ui.modeC->currentIndex();
		set_speed(2, ModemC);
		saveSettings();
		AGW_Report_Modem_Change(2);
		return;
	}

	if (strcmp(Name, "modeD") == 0)
	{
		ModemD = ui.modeD->currentIndex();
		set_speed(3, ModemD);
		saveSettings();
		AGW_Report_Modem_Change(3);
		return;
	}

	if (strcmp(Name, "centerA") == 0)
	{
		if (i > 300)
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
		if (i > 300)
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
		if (i > 300)
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
		saveSettings();
		return;
	}
	
	if (strcmp(Name, "RXOffset") == 0)
	{
		char valChar[32];
		rxOffset = i;
		sprintf(valChar, "RX Offset %d",rxOffset);
		ui.RXOffsetLabel->setText(valChar);

		wf_pointer(soundChannel[0]);
		wf_pointer(soundChannel[1]);

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

	Dlg->CWIDCall->setText(CWIDCall);
	Dlg->CWIDInterval->setText(QString::number(CWIDInterval));

	if (CWIDType)
		Dlg->radioButton_2->setChecked(1);
	else
		Dlg->CWIDType->setChecked(1);

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

	recovery[0] = Dlg->recoverBitA->currentIndex();
	recovery[1] = Dlg->recoverBitB->currentIndex();
	recovery[2] = Dlg->recoverBitC->currentIndex();
	recovery[3] = Dlg->recoverBitD->currentIndex();


	strcpy(CWIDCall, Dlg->CWIDCall->text().toUtf8().toUpper());
	CWIDInterval = Dlg->CWIDInterval->text().toInt();
	CWIDType = Dlg->radioButton_2->isChecked();

	if (CWIDInterval)
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

void QtSoundModem::SoundModeChanged(bool State)
{
	UNUSED(State);

	// Mustn't change SoundMode until dialog is accepted

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

	Dev = new(Ui_devicesDialog);

	QDialog UI;

	int i;

	Dev->setupUi(&UI);

	deviceUI = &UI;
	modemUI = 0;

	myResize *resize = new myResize();

	UI.installEventFilter(resize);

	newSoundMode = SoundMode;
	oldSoundMode = SoundMode;

#ifdef WIN32
	Dev->ALSA->setText("WaveOut");
	Dev->OSS->setVisible(0);
	Dev->PULSE->setVisible(0);
#endif

	if (SoundMode == 0)
		Dev->ALSA->setChecked(1);
	else if (SoundMode == 1)
		Dev->OSS->setChecked(1);
	else if (SoundMode == 2)
		Dev->PULSE->setChecked(1);
	else if (SoundMode == 2)
		Dev->UDP->setChecked(1);

	connect(Dev->ALSA, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->OSS, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->PULSE, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));
	connect(Dev->UDP, SIGNAL(toggled(bool)), this, SLOT(SoundModeChanged(bool)));

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

	Dev->Modem_1_Chan->setCurrentIndex(soundChannel[0]);
	Dev->Modem_2_Chan->setCurrentIndex(soundChannel[1]);
	Dev->Modem_3_Chan->setCurrentIndex(soundChannel[2]);
	Dev->Modem_4_Chan->setCurrentIndex(soundChannel[3]);

	// Disable "None" option in first modem

	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(Dev->Modem_1_Chan->model());
	QStandardItem * item = model->item(0, 0);
	item->setEnabled(false);

	Dev->singleChannelOutput->setChecked(SCO);
	Dev->colourWaterfall->setChecked(raduga);

	sprintf(valChar, "%d", KISSPort);
	Dev->KISSPort->setText(valChar);
	Dev->KISSEnabled->setChecked(KISSServ);

	sprintf(valChar, "%d", AGWPort);
	Dev->AGWPort->setText(valChar);
	Dev->AGWEnabled->setChecked(AGWServ);

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

	QStringList items;

	connect(Dev->CAT, SIGNAL(toggled(bool)), this, SLOT(CATChanged(bool)));
	connect(Dev->DualPTT, SIGNAL(toggled(bool)), this, SLOT(DualPTTChanged(bool)));
	connect(Dev->PTTPort, SIGNAL(currentIndexChanged(int)), this, SLOT(PTTPortChanged(int)));



	if (PTTMode == PTTCAT)
		Dev->CAT->setChecked(true);
	else
		Dev->RTSDTR->setChecked(true);

	for (const QSerialPortInfo &info : Ports)
	{
		items.append(info.portName());
	}

	items.sort();

	Dev->PTTPort->addItem("None");
	Dev->PTTPort->addItem("CM108");

	//#ifdef __ARM_ARCH

	Dev->PTTPort->addItem("GPIO");

	//#endif

	Dev->PTTPort->addItem("HAMLIB");

	for (const QString &info : items)
	{
		Dev->PTTPort->addItem(info);
	}

	Dev->PTTPort->setCurrentIndex(Dev->PTTPort->findText(PTTPort, Qt::MatchFixedString));

	PTTPortChanged(0);				// Force reevaluation

	Dev->txRotation->setChecked(TX_rotate);
	Dev->DualPTT->setChecked(DualPTT);

	Dev->multiCore->setChecked(multiCore);

	QObject::connect(Dev->okButton, SIGNAL(clicked()), this, SLOT(deviceaccept()));
	QObject::connect(Dev->cancelButton, SIGNAL(clicked()), this, SLOT(devicereject()));

	UI.exec();

}

void QtSoundModem::deviceaccept()
{
	QVariant Q = Dev->inputDevice->currentText();
	int cardChanged = 0;
	char portString[32];

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

	if (oldSoundMode != newSoundMode)
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

	CaptureIndex = Dev->inputDevice->currentIndex();

	Q = Dev->outputDevice->currentText();

	if (strcmp(PlaybackDevice, Q.toString().toUtf8()) != 0)
	{
		strcpy(PlaybackDevice, Q.toString().toUtf8());
		cardChanged = 1;
	}

	PlayBackIndex = Dev->outputDevice->currentIndex();

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


	SCO = Dev->singleChannelOutput->isChecked();
	raduga = Dev->colourWaterfall->isChecked();
	AGWServ = Dev->AGWEnabled->isChecked();
	KISSServ = Dev->KISSEnabled->isChecked();

	Q = Dev->KISSPort->text();
	KISSPort = Q.toInt();

	Q = Dev->AGWPort->text();
	AGWPort = Q.toInt();

	Q = Dev->PTTPort->currentText();
	strcpy(PTTPort, Q.toString().toUtf8());

	DualPTT = Dev->DualPTT->isChecked();
	TX_rotate = Dev->txRotation->isChecked();
	multiCore = Dev->multiCore->isChecked();

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

	ClosePTTPort();
	OpenPTTPort();

	wf_pointer(soundChannel[0]);
	wf_pointer(soundChannel[1]);

	delete(Dev);
	saveSettings();
	deviceUI->accept();

	if (cardChanged)
	{
		InitSound(1);
	}

	// Reset title and tooltip in case ports changed

	char Title[128];
	sprintf(Title, "QtSoundModem Version %s Ports %d/%d", VersionString, AGWPort, KISSPort);
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
	if (Firstwaterfall)
	{
		initWaterfall(0, 0);
		initWaterfall(0, 1);
	}

	if (Secondwaterfall)
	{
		initWaterfall(1, 0);
		initWaterfall(1, 1);
	}
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

	Waterfall[0]->fill(Black);

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

		Waterfall[0]->setPixel(i, val, Yellow);
		if (val < 62)
			Waterfall[0]->setPixel(i, val + 1, Gold);
		Data++;
	}

	ui.WaterfallA->setPixmap(QPixmap::fromImage(*Waterfall[0]));

}

void QtSoundModem::RefreshWaterfall(int snd_ch, unsigned char * Data)
{
	int j;
	unsigned char * Line;
	int len = Waterfall[0]->bytesPerLine();
	int TopLine = NextWaterfallLine[snd_ch];

	// Write line to cyclic buffer then draw starting with the line just written

	// Length is 208 bytes, including Level and Busy flags

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Data, 206);
	if (NextWaterfallLine[snd_ch] > 63)
		NextWaterfallLine[snd_ch] = 0;

	for (j = 63; j > 0; j--)
	{
		Line = Waterfall[0]->scanLine(j);
		memcpy(Line, &WaterfallLines[snd_ch][TopLine++][0], len);
		if (TopLine > 63)
			TopLine = 0;
	}

	ui.WaterfallA->setPixmap(QPixmap::fromImage(*Waterfall[0]));
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
		monWindowCopy->setTextColor(qRgb(192, 0, 0));
	else
		monWindowCopy->setTextColor(qRgb(0, 0, 192));

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

extern "C" void wf_Scale(int Chan)
{
	if (nonGUIMode)
		return;

	float k;
	int maxfreq, x, i;
	char Textxx[20];
	QImage * bm = Header[Chan];

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::black);
	qPainter.setPen(Qt::white);

	maxfreq = roundf(RX_Samplerate*0.005);
	k = 100 * fft_size / RX_Samplerate;

	if (Chan == 0)
		sprintf(Textxx, "Left");
	else
		sprintf(Textxx, "Right");

	qPainter.drawText(2, 1,
		100, 20, 0, Textxx);

	for (i = 0; i < maxfreq; i++)
	{
		x = round(k*i);
		if (x < 1025)
		{
			if ((i % 5) == 0)
				qPainter.drawLine(x, 20, x, 13);
			else
				qPainter.drawLine(x, 20, x, 16);

			if ((i % 10) == 0)
			{
				sprintf(Textxx, "%d", i * 100);

				qPainter.drawText(x - 12, 1,
					100, 20, 0, Textxx);
			}
		}
	}
	HeaderCopy[Chan]->setPixmap(QPixmap::fromImage(*bm));

}

// This draws the frequency Markers on the Waterfall


void do_pointer(int waterfall)
{
	if (nonGUIMode)
		return;

	float x;

	int x1, x2, k, pos1, pos2, pos3;
	QImage * bm = Header[waterfall];

	QPainter qPainter(bm);
	qPainter.setBrush(Qt::NoBrush);
	qPainter.setPen(Qt::white);

	//	bm->fill(black);

	qPainter.fillRect(0, 26, 1024, 9, Qt::black);

	k = 29;
	x = fft_size / RX_Samplerate;

	// draw all enabled ports on the ports on this soundcard

	// First Modem is always on the first waterfall
	// If second is enabled it is on the first unless different
	//		channel from first

	for (int i = 0; i < 4; i++)
	{
		if (UsingBothChannels == 0)
		{
			// Only One Waterfall. If first chan is 

			if ((waterfall == 0 && soundChannel[i] == RIGHT) || (waterfall == 1 && soundChannel[i] == LEFT))
				return;
		}

		if (soundChannel[i] == 0)
			continue;


		if (UsingBothChannels == 1)
			if ((waterfall == 0 && soundChannel[i] == RIGHT) || (waterfall == 1 && soundChannel[i] == LEFT))
				continue;

		pos1 = roundf(((rxOffset + chanOffset[i] + rx_freq[i]) - 0.5*rx_shift[i])*x) - 5;
		pos2 = roundf(((rxOffset + chanOffset[i] + rx_freq[i]) + 0.5*rx_shift[i])*x) - 5;
		pos3 = roundf((rxOffset + chanOffset[i] + rx_freq[i]) * x);
		x1 = pos1 + 5;
		x2 = pos2 + 5;

		qPainter.setPen(Qt::white);
		qPainter.drawLine(x1, k, x2, k);
		qPainter.drawLine(x1, k - 3, x1, k + 3);
		qPainter.drawLine(x2, k - 3, x2, k + 3);
		qPainter.drawLine(pos3, k - 3, pos3, k + 3);

		if (rxOffset || chanOffset[i])
		{
			// Draw TX posn if rxOffset used

			pos3 = roundf(rx_freq[i] * x);
			qPainter.setPen(Qt::magenta);
			qPainter.drawLine(pos3, k - 3, pos3, k + 3);
			qPainter.drawLine(pos3, k - 3, pos3, k + 3);
			qPainter.drawLine(pos3 - 2, k - 3, pos3 + 2, k - 3);

		}
	}
	HeaderCopy[waterfall]->setPixmap(QPixmap::fromImage(*bm));
}

void wf_pointer(int snd_ch)
{
	UNUSED(snd_ch);

	do_pointer(0);
	do_pointer(1);
//	do_pointer(2);
//	do_pointer(3);
}


void doWaterfallThread(void * param);

/*
#ifdef WIN32

#define pthread_t uintptr_t

extern "C" uintptr_t _beginthread(void(__cdecl *start_address)(void *), unsigned stack_size, void *arglist);

#else

#include <pthread.h>

extern "C" pthread_t _beginthread(void(*start_address)(void *), unsigned stack_size, void * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*)arglist) != 0)
		perror("New Thread");
	else
		pthread_detach(thread);

	return thread;
}

#endif
*/
extern "C" void doWaterfall(int snd_ch)
{
	if (nonGUIMode)
		return;

	if (Closing)
		return;

//	if (multiCore)			// Run modems in separate threads
//		_beginthread(doWaterfallThread, 0, xx);
//	else
		doWaterfallThread((void *)(size_t)snd_ch);

}


extern "C" float aFFTAmpl[1024];

void doWaterfallThread(void * param)
{
	int snd_ch = (int)(size_t)param;

	QImage * bm = Waterfall[snd_ch];
	
	word  i, wid;
	single  mag;
	UCHAR * p;
	UCHAR Line[4096];

	int lineLen;
	word  hfft_size;
	Byte  n;
	float RealOut[4096] = { 0 };
	float ImagOut[4096];

	QRegion exposed;

	hfft_size = fft_size / 2;

	// I think an FFT should produce n/2 bins, each of Samp/n Hz
	// Looks like my code only works with n a power of 2

	// So can use 1024 or 4096. 1024 gives 512 bins of 11.71875 and a 512 pixel 
	// display (is this enough?)

	// This does 2048

	if (0)	//RSID_WF
	{
		// Use the Magnitudes in float aFFTAmpl[RSID_FFT_SIZE];

		for (i = 0; i < hfft_size; i++)
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

		for (i = 0; i < hfft_size; i++)
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

			MagOut[i] = mag;
			fft_disp[snd_ch][i] = round(mag);
		}
	}



	/*
		for (i = 0; i < hfft_size; i++)
			fft[i] = (powf(RealOut[i], 2) + powf(ImagOut[i], 2));

		for (i = 0; i < hfft_size; i++)
		{
			if (fft[i] > max)
			{
				max = fft[i];
				imax = i;
			}
		}

		if (max > 0)
		{
			for (i = 0; i < hfft_size; i++)
				fft[i] = fft[i] / max;
		}


		for (i = 0; i < hfft_size; i++)
		{
			mag = fft[i];

			if (mag < 0.00001f)
				mag = 0.00001f;

			if (mag > 1.0f)
				mag = 1.0f;

			mag = 22 * log2f(mag) + 255;

			if (mag < 0)
				mag = 0;

			fft_disp[snd_ch][i] = round(mag);
		}

		*/

	//	bm[snd_ch].Canvas.CopyRect(d, bm[snd_ch].canvas, s)

	//pm->scroll(0, 1, 0, 0, 1024, 80, &exposed);

	// Each bin is 12000 /2048 = 5.859375
	// I think we plot at 6 Hz per pixel.

	wid = bm->width();
	if (wid > hfft_size)
		wid = hfft_size;

	wid = wid - 1;

	p = Line;
	lineLen = bm->bytesPerLine();

	if (wid > lineLen / 4)
		wid = lineLen / 4;

	if (raduga == DISP_MONO)
	{
		for (i = 0; i < wid; i++)
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
		for (i = 0; i < wid; i++)
		{
			n = fft_disp[snd_ch][i];

		memcpy(p, &RGBWF[n], 4);
			p += 4;
		}
	}

	// Scroll

	int TopLine = NextWaterfallLine[snd_ch];

	// Write line to cyclic buffer then draw starting with the line just written

	memcpy(&WaterfallLines[snd_ch][NextWaterfallLine[snd_ch]++][0], Line, 4096);
	if (NextWaterfallLine[snd_ch] > 79)
		NextWaterfallLine[snd_ch] = 0;

	for (int j = 79; j > 0; j--)
	{
		p = bm->scanLine(j);
		memcpy(p, &WaterfallLines[snd_ch][TopLine][0], lineLen);
		TopLine++;
		if (TopLine > 79)
			TopLine = 0;
	}

	WaterfallCopy[snd_ch]->setPixmap(QPixmap::fromImage(*bm));
	//	WaterfallCopy[snd_ch - 1]->setPixmap(*pm);
		//	WaterfallCopy[1]->setPixmap(QPixmap::fromImage(*bm));

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
	const char * msg;

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

