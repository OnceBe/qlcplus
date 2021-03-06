include(../../../variables.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = udmx

CONFIG      += plugin
INCLUDEPATH += ../../interfaces
DEPENDPATH  += ../../interfaces
unix:CONFIG      += link_pkgconfig
unix:PKGCONFIG   += libusb
win32:QMAKE_LFLAGS += -shared

HEADERS += udmxdevice.h \
           udmx.h

SOURCES += udmxdevice.cpp \
           udmx.cpp

win32 {
    HEADERS += libusb_dyn.h
    SOURCES += libusb_dyn.c
}

HEADERS += ../../interfaces/qlcioplugin.h

TRANSLATIONS += uDMX_fi_FI.ts
TRANSLATIONS += uDMX_de_DE.ts
TRANSLATIONS += uDMX_es_ES.ts
TRANSLATIONS += uDMX_fr_FR.ts
TRANSLATIONS += uDMX_it_IT.ts

# This must be after "TARGET = " and before target installation so that
# install_name_tool can be run before target installation
macx:include(../../../macx/nametool.pri)

# Installation
target.path = $$INSTALLROOT/$$PLUGINDIR
INSTALLS   += target

# UDEV rule to make uDMX USB device readable & writable for users in Linux
unix:!macx {
    udev.path  = /etc/udev/rules.d
    udev.files = z65-anyma-udmx.rules
    INSTALLS  += udev
}
