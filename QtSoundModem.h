#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtSoundModem.h"
#include "ui_calibrateDialog.h"
#include "ui_devicesDialog.h"
#include "ui_filterWindow.h"
#include "ui_ModemDialog.h"
#include "QThread"
#include <QLabel>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QSystemTrayIcon>

#include "tcpCode.h"


class QtSoundModem : public QMainWindow
{
	Q_OBJECT

public:

	QtSoundModem(QWidget *parent = Q_NULLPTR);
	void changeEvent(QEvent * e);
	void closeEvent(QCloseEvent * event);
	~QtSoundModem();

	void RefreshWaterfall(int snd_ch, unsigned char * Data);
	void initWaterfall(int chan, int state);
	void show_grid();
	void checkforCWID();

public slots:

private slots:

	void CWIDTimer();
	void doDevices();
	void updateFont();
	void MinimizetoTray();
	void TrayActivated(QSystemTrayIcon::ActivationReason reason);
	void MyTimerSlot();
	void returnPressed();
	void clickedSlotI(int i);
	void doModems();
	void doFilter(int Chan, int Filter);
	void SoundModeChanged(bool State);
	void DualPTTChanged(bool State);
	void CATChanged(bool State);
	void PTTPortChanged(int);
	void deviceaccept();
	void devicereject();
	void modemaccept();
	void modemSave();
	void modemreject();
	void doRSIDA();
	void doRSIDB();
	void doRSIDC();
	void doRSIDD();
	void handleButton(int Port, int Act);
	void doCalibrate();
	void RefreshSpectrum(unsigned char * Data);
	void doAbout();
	void doRestartWF();
	void doupdateDCD(int, int);
	void sendtoTrace(char * Msg, int tx);
	void preEmphAllAChanged(int);
	void preEmphAllBChanged(int);
	void preEmphAllCChanged(int state);
	void preEmphAllDChanged(int state);
	void menuChecked();
	void onTEselectionChanged();
	void clickedSlot();
	void startCWIDTimerSlot();

protected:
	 
	bool eventFilter(QObject * obj, QEvent * evt);
	void resizeEvent(QResizeEvent *event) override;

private:
	Ui::QtSoundModemClass ui;
	QTableWidget* sessionTable;
	QStringList m_TableHeader;

	QMenu *setupMenu;
	QMenu *viewMenu;

	QAction *actDevices;
	QAction *actModems;
	QAction *actFont;
	QAction *actMintoTray;
	QAction *actCalib;
	QAction *actAbout;
	QAction *actRestartWF;
	QAction *actWaterfall1;
	QAction *actWaterfall2;

signals:

};

class myResize : public QObject
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
};

