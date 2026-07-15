# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = app
TARGET = tst_rssfeedparser

QT -= gui
QT += testlib
CONFIG += testcase

INCLUDEPATH += ../../../common

SOURCES += \
    ../../../common/rssfeedparser.cpp \
    tst_rssfeedparser.cpp

HEADERS += ../../../common/rssfeedparser.h
