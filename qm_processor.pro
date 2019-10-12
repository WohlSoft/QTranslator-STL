CONFIG -= qt
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11
TEMPLATE = app

TARGET = qm_dumper

#DEFINES += QMTRANSLATPR_DEEP_DEBUG

#test ID-based translation
#CONFIG += idtest
#test Context-based translation in-class tr() function
CONFIG += trtest

DESTDIR = $$PWD/bin

HEADERS += \
    QTranslatorX/qm_translator.h

SOURCES += \
    qm_dumper.cpp \
    QTranslatorX/qm_translator.cpp

# Resolve path to lrelease tool
unix:exists($$[QT_INSTALL_BINS]/lrelease) {
LRELEASE_EXECUTABLE = $$[QT_INSTALL_BINS]/lrelease
}

unix:exists($$[QT_INSTALL_BINS]/lrelease-qt5) {
LRELEASE_EXECUTABLE = $$[QT_INSTALL_BINS]/lrelease-qt5
}

win32:exists($$[QT_INSTALL_BINS]/lrelease.exe) {
LRELEASE_EXECUTABLE = $$[QT_INSTALL_BINS]/lrelease.exe
}

idtest:{
unix:  system(find $$shell_path($$PWD/bin) -name *.ts | xargs $$shell_path($$LRELEASE_EXECUTABLE) -idbased )
win32: system(for /r $$shell_path($$PWD/bin) %B in (*.ts) do $$shell_path($$LRELEASE_EXECUTABLE) -idbased %B)
}

trtest:{
unix:  system(find $$shell_path($$PWD/bin) -name *.ts | xargs $$shell_path($$LRELEASE_EXECUTABLE) )
win32: system(for /r $$shell_path($$PWD/bin) %B in (*.ts) do $$shell_path($$LRELEASE_EXECUTABLE) %B)
}

TRANSLATIONS = \
    bin/testing_en.ts \
    bin/testing_ru.ts

