#include <QtCore/QCoreApplication>
#include <QtNetwork>
//#include <QDebug>

#define CONNECT(sndr, sig, rcvr, slt) connect(sndr, SIGNAL(sig), rcvr, SLOT(slt))

class mynet : public QObject
{
	Q_OBJECT

signals:

	void HLSetPTT(int c);
	void FLRigSetPTT(int c);
	void startTimer(int Time);
	void stopTimer();

public:
	void start();
	void OpenUDP();


public slots:
	void onAGWReadyRead();
	void onKISSSocketStateChanged(QAbstractSocket::SocketState socketState);
	void onKISSReadyRead();
	void onAGWSocketStateChanged(QAbstractSocket::SocketState socketState);
	void onKISSConnection();
	void MyTimerSlot();
	void onAGWConnection();
	void dropPTT();

	void displayError(QAbstractSocket::SocketError socketError);

	void sendtoKISS(void * sock, unsigned char * Msg, int Len);

	void FLRigdisplayError(QAbstractSocket::SocketError socketError);
	void FLRigreadyRead();
	void onFLRigSocketStateChanged(QAbstractSocket::SocketState socketState);
	void ConnecttoFLRig();
	void HAMLIBdisplayError(QAbstractSocket::SocketError socketError);
	void HAMLIBreadyRead();
	void onHAMLIBSocketStateChanged(QAbstractSocket::SocketState socketState);
	void ConnecttoHAMLIB();
	void dostartTimer(int Time);
	void dostopTimer();
	void doHLSetPTT(int c);
	void doFLRigSetPTT(int c);

	void readPendingDatagrams();
	void socketError();

private:
	QTcpServer* tcpServer;
	QTcpSocket* tcpClient;
	QTcpSocket* tcpServerConnection;
	int bytesToWrite;
	int bytesWritten;
	int bytesReceived;
	int TotalBytes;
	int PayloadSize;
};


class workerThread : public QThread
{
	Q_OBJECT
signals:
	void updateDCD(int, int);
	void sendtoTrace(char *, int);
	void sendtoKISS(void *, unsigned char *, int);
	void openSockets();
	void startCWIDTimer();
	void setWaterfallImage();
	void setLevelImage();
	void setConstellationImage(int, int);
	void startWatchdog();
	void stopWatchdog();


private:
	void run();
public:

};




