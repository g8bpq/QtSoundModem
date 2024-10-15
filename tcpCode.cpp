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

#include <QMessageBox>
#include "QtSoundModem.h"
#include "UZ7HOStuff.h"
#include <QTimer>

#define CONNECT(sndr, sig, rcvr, slt) connect(sndr, SIGNAL(sig), rcvr, SLOT(slt))

QList<QTcpSocket*>  _MgmtSockets;
QList<QTcpSocket*>  _KISSSockets;
QList<QTcpSocket*>  _AGWSockets;

QTcpServer * _KISSserver;
QTcpServer * _AGWserver;
QTcpServer * SixPackServer;
QTcpSocket *SixPackSocket;
QTcpServer * _MgmtServer;

TMgmtMode ** MgmtConnections = NULL;
int MgmtConCount = 0;

extern workerThread *t;
extern mynet m1;
extern serialThread *serial;

QString Response;

extern int MgmtPort;
extern "C" int KISSPort;
extern "C" void * initPulse();
extern "C" int SoundMode;

extern "C" int UDPClientPort;
extern "C" int UDPServerPort;
extern "C" int TXPort;

extern char SixPackDevice[256];
extern int SixPackPort;
extern int SixPackEnable;

char UDPHost[64] = "";

int UDPServ = 0;				// UDP Server Active (ie broadcast sound frams as UDP packets)

QMutex mutex;

extern void saveSettings();

extern int Closing;				// Set to stop background thread

extern "C"
{
	void KISSDataReceived(void * sender, char * data, int length);
	void AGW_explode_frame(void * soket, char * data, int len);
	void KISS_add_stream(void * Socket);
	void KISS_del_socket(void * Socket);
	void AGW_add_socket(void * Socket);
	void AGW_del_socket(void * socket);
	void Debugprintf(const char * format, ...);
	int InitSound(BOOL Report);
	void soundMain();
	void MainLoop();
	void set_speed(int snd_ch, int Modem);
	void init_speed(int snd_ch);
}

void Process6PackData(unsigned char * Bytes, int Len);

extern "C" int nonGUIMode;

QTimer *timer;
QTimer *timercopy;

void mynet::start()
{
	if (SoundMode == 3)
		OpenUDP();

	if (UDPServ)
		OpenUDP();

	if (KISSServ)
	{
		_KISSserver = new(QTcpServer);

		if (_KISSserver->listen(QHostAddress::Any, KISSPort))
			connect(_KISSserver, SIGNAL(newConnection()), this, SLOT(onKISSConnection()));
		else
		{
			if (nonGUIMode)
				Debugprintf("Listen failed for KISS Port");
			else
			{
				QMessageBox msgBox;
				msgBox.setText("Listen failed for KISS Port.");
				msgBox.exec();
			}
		}
	}

	if (AGWServ)
	{
		_AGWserver = new(QTcpServer);
		if (_AGWserver->listen(QHostAddress::Any, AGWPort))
			connect(_AGWserver, SIGNAL(newConnection()), this, SLOT(onAGWConnection()));
		else
		{
			if (nonGUIMode)
				Debugprintf("Listen failed for AGW Port");
			else
			{
				QMessageBox msgBox;
				msgBox.setText("Listen failed for AGW Port.");
				msgBox.exec();
			}
		}
	}


	if (MgmtPort)
	{
		_MgmtServer = new(QTcpServer);

		if (_MgmtServer->listen(QHostAddress::Any, MgmtPort))
			connect(_MgmtServer, SIGNAL(newConnection()), this, SLOT(onMgmtConnection()));
		else
		{
			if (nonGUIMode)
				Debugprintf("Listen failed for Mgmt Port");
			else
			{
				QMessageBox msgBox;
				msgBox.setText("Listen failed for Mgmt Port.");
				msgBox.exec();
			}
		}
	}


	QObject::connect(t, SIGNAL(sendtoKISS(void *, unsigned char *, int)), this, SLOT(sendtoKISS(void *, unsigned char *, int)), Qt::QueuedConnection);


	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
	timer->start(100);

	if (SixPackEnable)
	{
		if (SixPackDevice[0] && strcmp(SixPackDevice, "None") != 0)	// Using serial
		{
			serial->startSlave(SixPackDevice, 30000, Response);
			serial->start();

//			connect(serial, &serialThread::request, this, &QtSoundModem::showRequest);
//			connect(serial, &serialThread::error, this, &QtSoundModem::processError);
//			connect(serial, &serialThread::timeout, this, &QtSoundModem::processTimeout);

		}

		else if (SixPackPort)		// using TCP
		{
			SixPackServer = new(QTcpServer);
			if (SixPackServer->listen(QHostAddress::Any, SixPackPort))
				connect(SixPackServer, SIGNAL(newConnection()), this, SLOT(on6PackConnection()));

		}
	}

}

void mynet::MyTimerSlot()
{
	// 100 mS Timer Event

	TimerEvent = TIMER_EVENT_ON;
}


void mynet::onAGWConnection()
{
	QTcpSocket *clientSocket = _AGWserver->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onAGWReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onAGWSocketStateChanged(QAbstractSocket::SocketState)));

	_AGWSockets.push_back(clientSocket);

	AGW_add_socket(clientSocket);

	Debugprintf("AGW Connect Sock %x", clientSocket);
}



void mynet::onAGWSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

		AGW_del_socket(sender);

		_AGWSockets.removeOne(sender);
	}
}

void mynet::onAGWReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();

	AGW_explode_frame(sender, datas.data(), datas.length());
}

void mynet::on6PackReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();
	Process6PackData((unsigned char *)datas.data(), datas.length());
}


void mynet::on6PackSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

	}
}

void mynet::on6PackConnection()
{
	QTcpSocket *clientSocket = SixPackServer->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(on6PackReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(on6PackSocketStateChanged(QAbstractSocket::SocketState)));

	Debugprintf("6Pack Connect Sock %x", clientSocket);
}


void mynet::onKISSConnection()
{
	QTcpSocket *clientSocket = _KISSserver->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onKISSReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onKISSSocketStateChanged(QAbstractSocket::SocketState)));

	_KISSSockets.push_back(clientSocket);

	KISS_add_stream(clientSocket);

	Debugprintf("KISS Connect Sock %x", clientSocket);
}

void Mgmt_del_socket(void * socket)
{
	int i;

	TMgmtMode * MGMT = NULL;

	if (MgmtConCount == 0)
		return;

	for (i = 0; i < MgmtConCount; i++)
	{
		if (MgmtConnections[i]->Socket == socket)
		{
			MGMT = MgmtConnections[i];
			break;
		}
	}

	if (MGMT == NULL)
		return;

	// Need to remove entry and move others down

	MgmtConCount--;

	while (i < MgmtConCount)
	{
		MgmtConnections[i] = MgmtConnections[i + 1];
		i++;
	}
}



void mynet::onMgmtConnection()
{
	QTcpSocket *clientSocket = (QTcpSocket *)_MgmtServer->nextPendingConnection();
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onMgmtReadyRead()));
	connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onMgmtSocketStateChanged(QAbstractSocket::SocketState)));

	_MgmtSockets.append(clientSocket);

	// Create a data structure to hold session info

	TMgmtMode * MGMT;

	MgmtConnections = (TMgmtMode **)realloc(MgmtConnections, (MgmtConCount + 1) * sizeof(void *));

	MGMT = MgmtConnections[MgmtConCount++] = (TMgmtMode *)malloc(sizeof(*MGMT));
	memset(MGMT, 0, sizeof(*MGMT));

	MGMT->Socket = clientSocket;

	Debugprintf("Mgmt Connect Sock %x", clientSocket);
	clientSocket->write("Connected to QtSM\r");
}


void mynet::onMgmtSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

		Mgmt_del_socket(sender);

//		free(sender->Msg);
		_MgmtSockets.removeOne(sender);
		Debugprintf("Mgmt Disconnect Sock %x", sender);
	}
}

void mynet::onMgmtReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

	MgmtProcessLine(sender);
}

extern "C" void SendMgmtPTT(int snd_ch, int PTTState)
{
	// Won't work in non=gui mode

	emit m1.mgmtSetPTT(snd_ch, PTTState);
}


extern "C" char * strlop(char * buf, char delim);
extern "C" char modes_name[modes_count][21];
extern "C" int speed[5];

#ifndef WIN32
extern "C" int memicmp(char *a, char *b, int n);
#endif

void mynet::MgmtProcessLine(QTcpSocket* socket)
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
	{
		socket->readAll();
		return;
	}

	while (socket->bytesAvailable())
	{

		QByteArray datas = socket->peek(512);

		char * Line = datas.data();

		if (strchr(Line, '\r') == 0)
			return;

		char * rest = strlop(Line, '\r');

		int used = rest - Line;

		// Read the upto the cr Null

		datas = socket->read(used);

		Line = datas.data();
		rest = strlop(Line, '\r');

		if (memicmp(Line, "QtSMPort ", 8) == 0)
		{
			if (strlen(Line) > 10)
			{
				int port = atoi(&Line[8]);
				int bpqport = atoi(&Line[10]);

				if ((port > 0 && port < 5) && (bpqport > 0 && bpqport < 64))
				{
					MGMT->BPQPort[port - 1] = bpqport;
					socket->write("Ok\r");
				}
			}
		}

		else if (memicmp(Line, "Modem ", 6) == 0)
		{
			int port = atoi(&Line[6]);

			if (port > 0 && port < 5)
			{
				// if any more params - if a name follows, set it else return it

				char reply[80];

				sprintf(reply, "Port %d Chan %d Freq %d Modem %s \r", MGMT->BPQPort[port - 1], port, rx_freq[port - 1], modes_name[speed[port - 1]]);

				socket->write(reply);
			}
			else
				socket->write("Invalid Port\r");
		}
		else
		{
			socket->write("Invalid command\r");

		}
	}
}

void mynet::onKISSSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());

		KISS_del_socket(sender);

		_KISSSockets.removeOne(sender);
		Debugprintf("KISS Disconnect Sock %x", sender);
	}
}

void mynet::onKISSReadyRead()
{
	QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
	QByteArray datas = sender->readAll();

	KISSDataReceived(sender, datas.data(), datas.length());
}



void mynet::displayError(QAbstractSocket::SocketError socketError)
{
	if (socketError == QTcpSocket::RemoteHostClosedError)
		return;

	qDebug() << tcpClient->errorString();

	tcpClient->close();
	tcpServer->close();
}



void mynet::sendtoKISS(void * sock, unsigned char * Msg, int Len)
{
	if (sock == NULL)
	{
		for (QTcpSocket* socket : _KISSSockets)
		{
			socket->write((char *)Msg, Len);
		}
	}
	else
	{
		QTcpSocket* socket = (QTcpSocket*)sock;
		socket->write((char *)Msg, Len);
	}
	free(Msg);
}



QTcpSocket * HAMLIBsock;
int HAMLIBConnected = 0;
int HAMLIBConnecting = 0;

QTcpSocket * FLRIGsock;
int FLRIGConnected = 0;
int FLRIGConnecting = 0;

void mynet::HAMLIBdisplayError(QAbstractSocket::SocketError socketError)
{
	switch (socketError)
	{
	case QAbstractSocket::RemoteHostClosedError:
		break;

	case QAbstractSocket::HostNotFoundError:
		if (nonGUIMode)
			qDebug() << "HAMLIB host was not found. Please check the host name and port settings.";
		else
		{
			QMessageBox::information(nullptr, tr("QtSM"),
				tr("HAMLIB host was not found. Please check the "
					"host name and port settings."));
		}

		break;

	case QAbstractSocket::ConnectionRefusedError:

		qDebug() << "HAMLIB Connection Refused";
		break;

	default:

		qDebug() << "HAMLIB Connection Failed";
		break;

	}

	HAMLIBConnecting = 0;
	HAMLIBConnected = 0;
}

void mynet::HAMLIBreadyRead()
{
	unsigned char Buffer[4096];
	QTcpSocket* Socket = static_cast<QTcpSocket*>(QObject::sender());

	// read the data from the socket. Don't do anyhing with it at the moment

	Socket->read((char *)Buffer, 4095);
}

void mynet::onHAMLIBSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		// Close any connections

		HAMLIBConnected = 0;
		qDebug() << "HAMLIB Connection Closed";
	}
	else if (socketState == QAbstractSocket::ConnectedState)
	{
		HAMLIBConnected = 1;
		HAMLIBConnecting = 0;
		qDebug() << "HAMLIB Connected";
	}
}


void mynet::ConnecttoHAMLIB()
{
	delete(HAMLIBsock);

	HAMLIBConnected = 0;
	HAMLIBConnecting = 1;

	HAMLIBsock = new QTcpSocket();

	connect(HAMLIBsock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(HAMLIBdisplayError(QAbstractSocket::SocketError)));
	connect(HAMLIBsock, SIGNAL(readyRead()), this, SLOT(HAMLIBreadyRead()));
	connect(HAMLIBsock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onHAMLIBSocketStateChanged(QAbstractSocket::SocketState)));

	HAMLIBsock->connectToHost(HamLibHost, HamLibPort);

	return;
}

extern "C" void HAMLIBSetPTT(int PTTState)
{
	// Won't work in non=gui mode

	emit m1.HLSetPTT(PTTState);
}

extern "C" void FLRigSetPTT(int PTTState)
{
	// Won't work in non=gui mode

	emit m1.FLRigSetPTT(PTTState);
}


QTcpSocket * FLRigsock;
int FLRigConnected = 0;
int FLRigConnecting = 0;

void mynet::FLRigdisplayError(QAbstractSocket::SocketError socketError)
{
	switch (socketError)
	{
	case QAbstractSocket::RemoteHostClosedError:
		break;

	case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(nullptr, tr("QtSM"),
			"FLRig host was not found. Please check the "
			"host name and portsettings->");

		break;

	case QAbstractSocket::ConnectionRefusedError:

		qDebug() << "FLRig Connection Refused";
		break;

	default:

		qDebug() << "FLRig Connection Failed";
		break;

	}

	FLRigConnecting = 0;
	FLRigConnected = 0;
}

void mynet::FLRigreadyRead()
{
	unsigned char Buffer[4096];
	QTcpSocket* Socket = static_cast<QTcpSocket*>(QObject::sender());

	// read the data from the socket. Don't do anyhing with it at the moment

	Socket->read((char *)Buffer, 4095);
}

void mynet::onFLRigSocketStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState == QAbstractSocket::UnconnectedState)
	{
		// Close any connections

		FLRigConnected = 0;
		FLRigConnecting = 0;

		//	delete (FLRigsock);
		//	FLRigsock = 0;

		qDebug() << "FLRig Connection Closed";

	}
	else if (socketState == QAbstractSocket::ConnectedState)
	{
		FLRigConnected = 1;
		FLRigConnecting = 0;
		qDebug() << "FLRig Connected";
	}
}


void mynet::ConnecttoFLRig()
{
	delete(FLRigsock);

	FLRigConnected = 0;
	FLRigConnecting = 1;

	FLRigsock = new QTcpSocket();

	connect(FLRigsock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(FLRigdisplayError(QAbstractSocket::SocketError)));
	connect(FLRigsock, SIGNAL(readyRead()), this, SLOT(FLRigreadyRead()));
	connect(FLRigsock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onFLRigSocketStateChanged(QAbstractSocket::SocketState)));

	FLRigsock->connectToHost(FLRigHost, FLRigPort);

	return;
}

static char MsgHddr[] = "POST /RPC2 HTTP/1.1\r\n"
"User-Agent: XMLRPC++ 0.8\r\n"
"Host: 127.0.0.1:7362\r\n"
"Content-Type: text/xml\r\n"
"Content-length: %d\r\n"
"\r\n%s";

static char Req[] = "<?xml version=\"1.0\"?>\r\n"
"<methodCall><methodName>%s</methodName>\r\n"
"%s"
"</methodCall>\r\n";


void mynet::doFLRigSetPTT(int c)
{
	int Len;
	char ReqBuf[512];
	char SendBuff[512];
	char ValueString[256] = "";

	sprintf(ValueString, "<params><param><value><i4>%d</i4></value></param></params\r\n>", c);

	Len = sprintf(ReqBuf, Req, "rig.set_ptt", ValueString);
	Len = sprintf(SendBuff, MsgHddr, Len, ReqBuf);

	if (FLRigsock == nullptr || FLRigsock->state() != QAbstractSocket::ConnectedState)
		ConnecttoFLRig();

	if (FLRigsock == nullptr || FLRigsock->state() != QAbstractSocket::ConnectedState)
		return;

	FLRigsock->write(SendBuff);

	FLRigsock->waitForBytesWritten(3000);

	QByteArray datas = FLRigsock->readAll();

	qDebug(datas.data());
}



extern "C" void startTimer(int Time)
{
	// Won't work in non=gui mode

	emit m1.startTimer(Time);
}

void mynet::dostartTimer(int Time)
{
	timercopy->start(Time);
}

extern "C" void stopTimer()
{
	// Won't work in non=gui mode

	emit m1.stopTimer();
}

void mynet::dostopTimer()
{
	timercopy->stop();
}

void mynet::doHLSetPTT(int c)
{
	char Msg[16];

	if (HAMLIBsock == nullptr || HAMLIBsock->state() != QAbstractSocket::ConnectedState)
		ConnecttoHAMLIB();

	sprintf(Msg, "T %d\r\n", c);
	HAMLIBsock->write(Msg);

	HAMLIBsock->waitForBytesWritten(30000);

	QByteArray datas = HAMLIBsock->readAll();

	qDebug(datas.data());

}

void mynet::domgmtSetPTT(int snd_ch, int PTTState)
{
	char Msg[64];
	uint64_t ret;

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

		if (MGMT->BPQPort[snd_ch])
		{
			sprintf(Msg, "PTT %d %s\r", MGMT->BPQPort[snd_ch], PTTState ? "ON" : "OFF");
			ret = socket->write(Msg);
		}
	}
}




extern "C" void KISSSendtoServer(void * sock, Byte * Msg, int Len)
{
	emit t->sendtoKISS(sock, Msg, Len);
}


void workerThread::run()
{
	if (SoundMode == 2)			// Pulse
	{
		if (initPulse() == nullptr)
		{
			if (nonGUIMode)
			{
				qDebug() << "PulseAudio requested but pulseaudio libraries not found\nMode set to ALSA\n";
			}
			else
			{
				QMessageBox msgBox;
				msgBox.setText("PulseAudio requested but pulseaudio libraries not found\nMode set to ALSA");
				msgBox.exec();
			}
			SoundMode = 0;
			saveSettings();
		}
	}

	soundMain();

	if (SoundMode != 3)
	{
		if (!InitSound(1))
		{
			//		QMessageBox msgBox;
			//		msgBox.setText("Open Sound Card Failed");
			//		msgBox.exec();
		}
	}

	// Initialise Modems

	init_speed(0);
	init_speed(1);
	init_speed(2);
	init_speed(3);

	//	emit t->openSockets();

	while (Closing == 0)
	{
		// Run scheduling loop

		MainLoop();

		this->msleep(10);
	}

	qDebug() << "Saving Settings";

	saveSettings();

	qDebug() << "Main Loop exited";

	qApp->exit();

};

// Audio over UDP Code.

// Code can either send audio blocks from the sound card as UDP packets or use UDP packets from 
// a suitable source (maybe another copy of QtSM) and process them instead of input frm a sound card/

// ie act as a server or client for UDP audio.

// of course we need bidirectional audio, so even when we are a client we send modem generated samples
// to the server and as a server pass received smaples to modem

// It isn't logical to run as both client and server, so probably can use just one socket


QUdpSocket * udpSocket;

qint64 udpServerSeqno= 0;
qint64 udpClientLastSeq = 0;
qint64 udpServerLastSeq = 0;
int droppedPackets = 0;
extern "C" int UDPSoundIsPlaying;

QQueue <unsigned char *> queue;


void mynet::OpenUDP()
{
	udpSocket = new QUdpSocket();

	if (UDPServ)
	{
		udpSocket->bind(QHostAddress("0.0.0.0"), UDPServerPort);
		QTimer *timer = new QTimer(this);
		timercopy = timer;
		connect(timer, SIGNAL(timeout()), this, SLOT(dropPTT()));
	}
	else
		udpSocket->bind(QHostAddress("0.0.0.0"), UDPClientPort);

	connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));	
}

extern "C" void Flush();

void mynet::dropPTT()
{
	timercopy->stop();
	
	if (UDPSoundIsPlaying)
	{
		// Drop PTT when all sent

		Flush();
		UDPSoundIsPlaying = 0;
		Debugprintf("PTT Off");
		RadioPTT(0, 0);
	}
}

void mynet::readPendingDatagrams()
{
	while (udpSocket->hasPendingDatagrams())
	{
		QHostAddress Addr;
		quint16 rxPort;
		char copy[1501];

		// We expect datagrams of 1040 bytes containing a 16 byte header and 512 16 bit samples
		// We should get a datagram every 43 mS. We need to use a timeout to drop PTT if running as server

		if (UDPServ)
			timercopy->start(200);

		int Len = udpSocket->readDatagram(copy, 1500, &Addr, &rxPort);

		if (Len == 1040)
		{
			qint64 Seq;

			memcpy(&Seq, copy, sizeof(udpServerSeqno));

			if (Seq < udpClientLastSeq || udpClientLastSeq == 0)

				// Client or Server Restarted

				udpClientLastSeq = Seq;

			else
			{
				int Missed = Seq - udpClientLastSeq;

				if (Missed > 100)			// probably stopped in debug
					Missed = 1;

				while (--Missed)
				{
					droppedPackets++;

					// insert silence to maintain timing

					unsigned char * pkt = (unsigned char *)malloc(1024);

					memset(pkt, 0, 1024);

					mutex.lock();
					queue.append(pkt);
					mutex.unlock();

				}
			}

			udpClientLastSeq = Seq;

			unsigned char * pkt = (unsigned char *)malloc(1024);

			memcpy(pkt, &copy[16], 1024);

			mutex.lock();
			queue.append(pkt);
			mutex.unlock();
		}
	}
}

void  mynet::socketError()
{
	char errMsg[80];
	sprintf(errMsg, "%d %s", udpSocket->state(), udpSocket->errorString().toLocal8Bit().constData());
	//	qDebug() << errMsg;
	//	QMessageBox::question(NULL, "ARDOP GUI", errMsg, QMessageBox::Yes | QMessageBox::No);
}

extern "C" void sendSamplestoStdout(short * Samples, int nSamples)
{

}


extern "C" void sendSamplestoUDP(short * Samples, int nSamples, int Port)
{
	if (udpSocket == nullptr)
		return;
	
	unsigned char txBuff[1048];

	memcpy(txBuff, &udpServerSeqno, sizeof(udpServerSeqno));
	udpServerSeqno++;

	if (nSamples > 512)
		nSamples = 512;

	nSamples <<= 1;				// short to byte

	memcpy(&txBuff[16], Samples, nSamples);

	udpSocket->writeDatagram((char *)txBuff, nSamples + 16, QHostAddress(UDPHost), Port);
}

static int min = 0, max = 0, lastlevelGUI = 0, lastlevelreport = 0;

static UCHAR CurrentLevel = 0;		// Peak from current samples

extern "C" int SoundIsPlaying;
extern "C" short * SendtoCard(short * buf, int n);
extern "C" short * DMABuffer;


extern "C" void ProcessNewSamples(short * Samples, int nSamples);


extern "C" void UDPPollReceivedSamples()
{
	if (queue.isEmpty())
		return;

	short * ptr;
	short * save;

	// If we are using UDP for output (sound server) send samples to sound card.
	// If for input (virtual sound card) then pass to modem

	if (UDPServ)
	{
		// We only get packets if TX active (sound VOX) so raise PTT and start sending
		// ?? do we ignore if local modem is already sending ??

		if (SoundIsPlaying)
		{
			mutex.lock();
			save = ptr = (short *)queue.dequeue();
			mutex.unlock();
			free(save);
		}
		
		if (UDPSoundIsPlaying == 0)
		{
			// Wait for a couple of packets to reduce risk of underrun (but not too many or delay will be excessive

			if (queue.count() < 3)
				return;

			UDPSoundIsPlaying = 1;
			Debugprintf("PTT On");
			RadioPTT(0, 1);				// UDP only use one channel

			/// !! how do we drop ptt ??
		}

		while (queue.count() > 1)
		{
			short * outptr = DMABuffer;
			boolean dropPTT1 = 1;
			boolean dropPTT2 = 1;

			// We get mono samples but soundcard expects stereo
			// Sound card needs 1024 samples so send two packets

			mutex.lock();
			save = ptr = (short *)queue.dequeue();
			mutex.unlock();

			for (int n = 0; n < 512; n++)
			{
				*(outptr++) = *ptr;
				*(outptr++) = *ptr++;	// Duplicate
				if (*ptr)
					dropPTT1 = 0;		// Drop PTT if all samples zero
			}

			free(save);

			mutex.lock();
			save = ptr = (short *)queue.dequeue();
			mutex.unlock();

			for (int n = 0; n < 512; n++)
			{
				*(outptr++) = *ptr;
				*(outptr++) = *ptr++;	// Duplicate
				if (*ptr)
					dropPTT2 = 0;		// Drop PTT if all samples zero
			}

			free(save);

			if (dropPTT1 && dropPTT2)
			{
				startTimer(1);			// All zeros so no need to send
				return;
			}

			DMABuffer = SendtoCard(DMABuffer, 1024);

			if (dropPTT2)				// 2nd all zeros
				startTimer(1);	
		}
		return;
	}

	mutex.lock();
	save = ptr = (short *)queue.dequeue();
	mutex.unlock();

	// We get mono samples but modem expects stereo

	short Buff[2048];
	short * inptr = (short *)ptr;
	short * outptr = Buff;

	int i;

	for (i = 0; i < ReceiveSize; i++)
	{
		if (*(ptr) < min)
			min = *ptr;
		else if (*(ptr) > max)
			max = *ptr;
		ptr++;
	}

	CurrentLevel = ((max - min) * 75) / 32768;	// Scale to 150 max

	if ((Now - lastlevelGUI) > 2000)	// 2 Secs
	{
		lastlevelGUI = Now;

		if ((Now - lastlevelreport) > 10000)	// 10 Secs
		{
			char HostCmd[64];
			lastlevelreport = Now;

			sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
			Debugprintf("Input peaks = %d, %d", min, max);
		}
		
		min = max = 0;
	}

	for (int n = 0; n < 512; n++)
	{
		*(outptr++) = *inptr;
		*(outptr++) = *inptr++;	// Duplicate
	}

	ProcessNewSamples(Buff, 512);
	free(save);
}






