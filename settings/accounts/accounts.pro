# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = aux

provider.files = providers/rss.provider
provider.path = /usr/share/accounts/providers

services.files = services/rss-posts.service
services.path = /usr/share/accounts/services

ui.files = \
    ui/RssSettingsDisplay.qml \
    ui/rss.qml \
    ui/rss-settings.qml \
    ui/rss-update.qml
ui.path = /usr/share/accounts/ui

desktop.files = rss-import.desktop
desktop.path = /usr/share/applications

OTHER_FILES += $$provider.files $$services.files $$ui.files $$desktop.files
INSTALLS += provider services ui desktop
