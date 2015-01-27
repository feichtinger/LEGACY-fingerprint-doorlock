#-------------------------------------------------
#
# Project created by QtCreator 2015-01-20T19:19:10
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Fingerprint-Manager
TEMPLATE = app


SOURCES += main.cpp\
        fingerprintmanager.cpp \
    dialogaddfinger.cpp

HEADERS  += fingerprintmanager.h \
    dialogaddfinger.h

FORMS    += fingerprintmanager.ui \
    dialogaddfinger.ui
