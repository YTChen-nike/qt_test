#-------------------------------------------------
#
# Project created by QtCreator 2020-06-08T11:15:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test3
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        widget.cpp \
    util/aw_ipc.cpp \
    util/aw_log.cpp \
    util/aw_thread.cpp \
    util/aw_util.cpp \
    util/cJSON.cpp

HEADERS += \
        widget.h \
    util/aw_ipc.h \
    util/aw_log.h \
    util/aw_thread.h \
    util/aw_util.h

FORMS += \
        widget.ui

LIBS += -lws2_32
