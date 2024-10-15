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



#include "QtSoundModem.h"
#include <QtWidgets/QApplication>
#include "UZ7HOStuff.h"

extern "C" int nonGUIMode;

extern void getSettings();
extern void saveSettings();
extern int Closing;

workerThread *t;
serialThread *serial;
mynet m1;

QCoreApplication * a;		

QtSoundModem * w;


int main(int argc, char *argv[])
{
	char Title[128];
	QString Response;

	if (argc > 1 && strcmp(argv[1], "nogui") == 0)
		nonGUIMode = 1;

	if (nonGUIMode)
		sprintf(Title, "QtSoundModem Version %s Running in non-GUI Mode", VersionString);
	else
		sprintf(Title, "QtSoundModem Version %s Running in GUI Mode", VersionString);

	qDebug() << Title;



	if (nonGUIMode)
		a = new QCoreApplication(argc, argv);
	else
		a = new QApplication(argc, argv);			// GUI version

	getSettings();


	t = new workerThread;

	if (nonGUIMode == 0)
	{
		w = new QtSoundModem();

		char Title[128];
		sprintf(Title, "QtSoundModem Version %s Ports %d%s/%d%s", VersionString, AGWPort, AGWServ ? "*" : "", KISSPort, KISSServ ? "*" : "");
		w->setWindowTitle(Title);

		w->show();
	}

	QObject::connect(&m1, SIGNAL(HLSetPTT(int)), &m1, SLOT(doHLSetPTT(int)), Qt::QueuedConnection);
	QObject::connect(&m1, SIGNAL(FLRigSetPTT(int)), &m1, SLOT(doFLRigSetPTT(int)), Qt::QueuedConnection);
	QObject::connect(&m1, SIGNAL(mgmtSetPTT(int, int)), &m1, SLOT(domgmtSetPTT(int, int)), Qt::QueuedConnection);


	QObject::connect(&m1, SIGNAL(startTimer(int)), &m1, SLOT(dostartTimer(int)), Qt::QueuedConnection);
	QObject::connect(&m1, SIGNAL(stopTimer()), &m1, SLOT(dostopTimer()), Qt::QueuedConnection);

	t->start();				// This runs init

	m1.start();				// Start TCP 

	return a->exec();

}

