QT += core
QT -= gui

CONFIG += c++11

TARGET = ffmpegTest
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    packet_queue.cpp \
    player.cpp \
    audio_player.cpp \
    video_player.cpp \
    synching.cpp \
    frame_queue.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    packet_queue.h \
    player.h \
    frame_queue.h


unix:!macx: LIBS += -L$$PWD/../FF/lib/ -lavutil -lavformat -lavcodec -lz -lavutil -lm -lSDL2  -lswscale -lswresample

INCLUDEPATH += $$PWD/../FF/include
DEPENDPATH += $$PWD/../FF/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../FF/lib/release/ -lSDL2
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../FF/lib/debug/ -lSDL2
else:unix: LIBS += -L$$PWD/../FF/lib/ -lSDL2

#INCLUDEPATH += $$PWD/../FF/include
#DEPENDPATH += $$PWD/../FF/include
