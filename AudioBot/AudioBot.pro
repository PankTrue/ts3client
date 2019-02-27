TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle



QMAKE_CXXFLAGS += -O3

QT -= gui
QT += core network

SOURCES += main.cpp \
    audiobot.cpp

HEADERS += \
    audiobot.h



win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Ts3Client/release/ -lTs3Client
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Ts3Client/debug/ -lTs3Client
else:unix: LIBS += -L$$OUT_PWD/../Ts3Client/ -lTs3Client

INCLUDEPATH += $$PWD/../Ts3Client
DEPENDPATH += $$PWD/../Ts3Client

INCLUDEPATH += /usr/include/opus

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Ts3Client/release/libTs3Client.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Ts3Client/debug/libTs3Client.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Ts3Client/release/Ts3Client.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Ts3Client/debug/Ts3Client.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Ts3Client/libTs3Client.a


#unix: LIBS += -lssl -lcrypto
unix: LIBS += -lcryptopp
unix: LIBS += -lpthread
#unix: LIBS += -lavcodec -lavformat -lavutil -lswresample
unix: LIBS += -lopus -lopusfile
unix: LIBS += -lresolv
#unix: LIBS += -ldbus

