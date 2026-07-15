# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = app
TARGET = tst_rsspostsdismissal

QT -= gui
QT += network sql testlib
CONFIG += testcase link_pkgconfig
PKGCONFIG += socialcache

INCLUDEPATH += \
    ../../../common \
    ../../../eventsview-plugins/eventsview-plugin-rss \
    ../../../settings/accounts-translations-plugin

SOURCES += \
    ../../../common/rsspostsdatabase.cpp \
    ../../../eventsview-plugins/eventsview-plugin-rss/rsspostsmodel.cpp \
    ../../../settings/accounts-translations-plugin/rssdismissalrestorer.cpp \
    tst_rsspostsdismissal.cpp

HEADERS += \
    ../../../common/rsspostsdatabase.h \
    ../../../eventsview-plugins/eventsview-plugin-rss/rsspostsmodel.h \
    ../../../settings/accounts-translations-plugin/rssdismissalrestorer.h
