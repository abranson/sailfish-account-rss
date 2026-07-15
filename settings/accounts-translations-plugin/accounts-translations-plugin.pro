# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = lib
TARGET = rssaccountstranslationsplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = com/jolla/settings/accounts/rss
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT -= gui
QT += qml sql
CONFIG += plugin link_pkgconfig
PKGCONFIG += socialcache

include($$PWD/../../common/common.pri)

SOURCES += \
    plugin.cpp \
    rssdismissalrestorer.cpp \
    $$PWD/../../common/rssfeedparser.cpp

HEADERS += \
    rssdismissalrestorer.h \
    $$PWD/../../common/rssfeedparser.h

target.path = $$TARGETPATH
qmldir.files = qmldir
qmldir.path = $$TARGETPATH

INSTALLS += target qmldir
