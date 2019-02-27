TARGET = Ts3Client
TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += -O3


SOURCES += \
    helper.cpp \
    notificationdata.cpp \
    packet.cpp \
    packethandler.cpp \
    quicklz.cpp \
    ringqueue.cpp \
    ts3crypt.cpp \
    ts3fullclient.cpp \
    tsdnsresolver.cpp \
    ts3client.cpp \
    commandmaker.cpp \
    byte.cpp

HEADERS += \
    enums.h \
    helper.h \
    notificationdata.h \
    packet.h \
    packethandler.h \
    quicklz.h \
    ringqueue.h \
    structs.h \
    ts3crypt.h \
    ts3fullclient.h \
    tsdnsresolver.h \
    ts3client.h \
    commandmaker.h \
    byte.h


unix: LIBS += -lcryptopp
unix: LIBS += -lpthread
unix: LIBS += -lresolv


