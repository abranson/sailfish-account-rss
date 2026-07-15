# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = aux

ACCOUNT_TS = $$OUT_PWD/sailfish-account-rss.ts
ACCOUNT_QM = $$OUT_PWD/sailfish-account-rss_eng_en.qm
EVENT_TS = $$OUT_PWD/lipstick-jolla-home-rss.ts
EVENT_QM = $$OUT_PWD/lipstick-jolla-home-rss_eng_en.qm

account_ts.commands = lupdate $$PWD/../settings -ts $$ACCOUNT_TS
account_ts.CONFIG += no_check_exist no_link
account_ts.output = $$ACCOUNT_TS
account_ts.input = $$PWD/../settings

account_qm.commands = lrelease -idbased $$ACCOUNT_TS -qm $$ACCOUNT_QM
account_qm.CONFIG += no_check_exist no_link
account_qm.depends = account_ts
account_qm.output = $$ACCOUNT_QM
account_qm.input = $$ACCOUNT_TS

event_ts.commands = lupdate $$PWD/../eventsview-plugins -ts $$EVENT_TS
event_ts.CONFIG += no_check_exist no_link
event_ts.output = $$EVENT_TS
event_ts.input = $$PWD/../eventsview-plugins

event_qm.commands = lrelease -idbased $$EVENT_TS -qm $$EVENT_QM
event_qm.CONFIG += no_check_exist no_link
event_qm.depends = event_ts
event_qm.output = $$EVENT_QM
event_qm.input = $$EVENT_TS

account_qm_install.files = $$ACCOUNT_QM
account_qm_install.path = /usr/share/translations
account_qm_install.CONFIG += no_check_exist

event_qm_install.files = $$EVENT_QM
event_qm_install.path = /usr/share/translations
event_qm_install.CONFIG += no_check_exist

source_install.files = $$ACCOUNT_TS $$EVENT_TS
source_install.path = /usr/share/translations/source
source_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += account_ts account_qm event_ts event_qm
PRE_TARGETDEPS += account_ts account_qm event_ts event_qm
INSTALLS += account_qm_install event_qm_install source_install
