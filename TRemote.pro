#-------------------------------------------------
#
# Project created by QtCreator 2017-10-26T10:45:17
#
#-------------------------------------------------


QT += core
QT += gui
QT += websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TRemote
TEMPLATE = app


SOURCES += main.cpp
SOURCES += serverdiscoverer.cpp
SOURCES += utility.cpp
SOURCES += tremote.cpp

HEADERS += utility.h
HEADERS += serverdiscoverer.h
HEADERS += tremote.h

FORMS   += tremote.ui
