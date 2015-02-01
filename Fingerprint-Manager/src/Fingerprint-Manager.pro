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
    dialogaddfinger.cpp \
    dialogsetopentime.cpp

HEADERS  += fingerprintmanager.h \
    dialogaddfinger.h \
    dialogsetopentime.h

FORMS    += fingerprintmanager.ui \
    dialogaddfinger.ui \
    dialogsetopentime.ui

RESOURCES += \
    resource.qrc
