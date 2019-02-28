TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    bitmap.cpp \
    jarvisMarch.cpp

HEADERS += \
    bitmap.h \
    pixel.h \
    point.h \
    jarvisMarch.h
