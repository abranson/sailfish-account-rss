# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = lib
TARGET = rsscommon
TARGET = $$qtLibraryTarget($$TARGET)
VERSION = 1.0.0

QT -= gui
QT += sql

CONFIG += link_pkgconfig
PKGCONFIG += socialcache

HEADERS += \
    rssfeedparser.h \
    rsspostsdatabase.h

SOURCES += \
    rssfeedparser.cpp \
    rsspostsdatabase.cpp

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target
