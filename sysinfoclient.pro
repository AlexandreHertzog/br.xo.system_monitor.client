QT       += core websockets
QT       -= gui

TARGET = sysinfoclient
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
    main.cpp \
    sysinfo.cpp \
    ../sysinfo_api/packet.cpp \
    ../sysinfo_api/exception.cpp \
    sysinfoclient.cpp

HEADERS += \
    sysinfo.h \
    ../sysinfo_api/packet.h \
    ../sysinfo_api/exception.h \
    sysinfoclient.h

target.path = $$[QT_INSTALL_EXAMPLES]/websockets/sysinfoclient
INSTALLS += target
