
QT += core gui
QT += network
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtSoundModem
TEMPLATE = app


HEADERS += ./UZ7HOStuff.h \
    ./QtSoundModem.h \
    ./tcpCode.h

SOURCES += ./audio.c \
    ./pulse.c \
    ./ax25.c \
    ./ax25_demod.c \
    ./ax25_l2.c \
    ./ax25_mod.c \
    ./Config.cpp \
    ./kiss_mode.c \
    ./main.cpp \
    ./QtSoundModem.cpp \
    ./ShowFilter.cpp \
    ./SMMain.c \
    ./sm_main.c \
    ./UZ7HOUtils.c \
    ./ALSASound.c \
        ./ax25_agw.c \
        ./berlekamp.c \
        ./galois.c \
        ./rs.c \
       ./rsid.c \
	./il2p.c \
    ./tcpCode.cpp \
    ./ax25_fec.c \
    ./RSUnit.c \
	./ARDOPC.c \
    ./ardopSampleArrays.c \
    ./SoundInput.c \
    ./Modulate.c \
    ./ofdm.c \
    ./pktARDOP.c \
     ./BusyDetect.c



FORMS += ./calibrateDialog.ui \
    ./devicesDialog.ui \
    ./filterWindow.ui \
    ./ModemDialog.ui \
    ./QtSoundModem.ui

RESOURCES += QtSoundModem.qrc
RC_ICONS = QtSoundModem.ico

QMAKE_CFLAGS += -g
#QMAKE_LFLAGS += -lasound -lpulse-simple -lpulse -lfftw3f
QMAKE_LIBS += -lasound -lfftw3f -ldl


