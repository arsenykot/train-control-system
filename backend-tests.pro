# Backend: standalone build of the existing simulation tests.
QT += core
QT -= gui

CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = backend-tests

SOURCES += \
    test.cpp \
    route.cpp \
    simulation.cpp \
    station.cpp \
    train.cpp

HEADERS += \
    route.h \
    simulation.h \
    station.h \
    train.h
