CONFIG -= qt
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11
TEMPLATE = app

TARGET = qm_dumper

#DEFINES += QMTRANSLATPR_DEEP_DEBUG

DESTDIR = $$PWD/bin

HEADERS += \
    ConvertUTF.h \
    qm_translator.h

SOURCES += \
    qm_dumper.cpp \
    ConvertUTF.c \
    qm_translator.cpp


