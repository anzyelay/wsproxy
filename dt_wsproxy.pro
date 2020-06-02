TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = dt_wsproxy

SOURCES += \
        main.c \
        mongoose.c

HEADERS += \ \
    mongoose.h

target.path = /tmp/tmp
INSTALLS += target
