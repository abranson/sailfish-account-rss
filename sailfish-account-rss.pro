# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = subdirs

SUBDIRS += \
    common \
    settings \
    buteo-plugins \
    eventsview-plugins \
    icons \
    translations

buteo-plugins.depends = common
eventsview-plugins.depends = common
settings.depends = common

OTHER_FILES += rpm/sailfish-account-rss.spec
