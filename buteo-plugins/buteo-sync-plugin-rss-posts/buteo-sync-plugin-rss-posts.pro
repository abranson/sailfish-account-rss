# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = lib
TARGET = rss-posts-client

QT -= gui
QT += dbus network

CONFIG += plugin link_pkgconfig
PKGCONFIG += \
    accounts-qt5 \
    buteosyncfw5 \
    socialcache

include($$PWD/../../common/common.pri)

HEADERS += rsspostsclient.h
SOURCES += rsspostsclient.cpp

OTHER_FILES += \
    rss-posts.xml \
    rss.Posts.xml

target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5/oopp

sync.files = rss.Posts.xml
sync.path = /etc/buteo/profiles/sync

client.files = rss-posts.xml
client.path = /etc/buteo/profiles/client

INSTALLS += target sync client
