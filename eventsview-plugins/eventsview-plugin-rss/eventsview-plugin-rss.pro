# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = lib
TARGET = jollaeventsviewrssplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = com/jolla/eventsview/rss
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT -= gui
QT += qml

CONFIG += plugin link_pkgconfig
PKGCONFIG += socialcache

include($$PWD/../../common/common.pri)

HEADERS += rsspostsmodel.h
SOURCES += \
    plugin.cpp \
    rsspostsmodel.cpp

eventfeed.files = \
    RssFeedItem.qml \
    rss-delegate.qml
eventfeed.path = /usr/share/lipstick/eventfeed

qmldir.files = qmldir
qmldir.path = $$TARGETPATH

target.path = $$TARGETPATH

OTHER_FILES += $$eventfeed.files $$qmldir.files
INSTALLS += target qmldir eventfeed
