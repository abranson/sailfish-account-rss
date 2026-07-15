// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0
import com.jolla.settings.accounts.rss 1.0
import Nemo.Configuration 1.0

StandardAccountSettingsDisplay {
    id: root

    property string originalFeedUrl
    property bool postsServiceEnabled
    property QtObject postsSyncOptions
    readonly property bool feedUrlChanged:
        normalizeUrl(feedUrlField.text) !== originalFeedUrl
    readonly property bool feedUrlValid: feedUrlField.acceptableInput

    settingsModified: true

    function normalizeUrl(value) {
        var url = value ? value.trim() : ""
        if (url.length > 0 && !/^https?:\/\//i.test(url)) {
            url = "https://" + url
        }
        return url
    }

    function saveAccountAndSyncIfValid() {
        if (!feedUrlValid) {
            feedUrlField.errorHighlight = true
            feedUrlField.forceActiveFocus()
            return false
        }
        saveAccountAndSync()
        return true
    }

    function discardInvalidFeedUrl() {
        if (!feedUrlValid) {
            feedUrlField.text = originalFeedUrl
            feedUrlField.errorHighlight = false
        }
    }

    onAccountInitialized: {
        var values = account.configurationValues("rss-posts")
        originalFeedUrl = values["feed_url"] ? values["feed_url"].toString() : ""
        feedUrlField.text = originalFeedUrl
    }

    onAboutToSaveAccount: {
        var normalizedUrl = feedUrlField.acceptableInput
                ? normalizeUrl(feedUrlField.text) : originalFeedUrl
        account.setConfigurationValue("rss-posts", "feed_url", normalizedUrl)
        account.setConfigurationValue("", "FeedViewAutoSync", autoSyncSwitch.checked)
        settingsLoader.updateAllSyncProfiles()
        originalFeedUrl = normalizedUrl
    }

    StandardAccountSettingsLoader {
        id: settingsLoader

        account: root.account
        accountProvider: root.accountProvider
        accountManager: root.accountManager
        autoEnableServices: root.autoEnableAccount

        onSettingsLoaded: {
            serviceRepeater.model = syncServices

            var options = allSyncOptionsForService("rss-posts")
            for (var profileId in options) {
                if (profileId.indexOf("rss.Posts") >= 0) {
                    root.postsSyncOptions = options[profileId]
                    break
                }
            }

            root.postsServiceEnabled = root.account.isEnabledWithService("rss-posts")
            var autoSync = root.account.configurationValues("")["FeedViewAutoSync"]
            autoSyncSwitch.checked = autoSync === undefined ? true : autoSync === true
        }
    }

    SectionHeader {
        //% "Feed"
        text: qsTrId("settings-accounts-rss-he-feed")
    }

    TextField {
        id: feedUrlField

        width: parent.width
        validator: RegExpValidator {
            regExp: /^(https?:\/\/)?[^\s\/]+\.[^\s\/]+(\/.*)?$/i
        }
        inputMethodHints: Qt.ImhUrlCharactersOnly
                          | Qt.ImhNoPredictiveText
                          | Qt.ImhNoAutoUppercase
        //% "Feed URL"
        label: qsTrId("settings-accounts-rss-la-feed_url")
        EnterKey.iconSource: "image://theme/icon-m-enter-close"
        EnterKey.onClicked: focus = false

        onTextChanged: errorHighlight = false
        onActiveFocusChanged: {
            if (!activeFocus) {
                errorHighlight = !acceptableInput
            }
        }

        description: {
            if (!errorHighlight) {
                return ""
            }
            //% "Enter a valid HTTP or HTTPS feed URL"
            return qsTrId("settings-accounts-rss-la-feed_url_invalid")
        }
    }

    Repeater {
        id: serviceRepeater

        TextSwitch {
            checked: model.enabled
            //% "Show posts in Events"
            text: qsTrId("settings-accounts-rss-la-show_posts")
            //% "Display entries from this feed in the News group in Events."
            description: qsTrId("settings-accounts-rss-la-show_posts_description")

            onCheckedChanged: {
                root.postsServiceEnabled = checked
                if (checked) {
                    root.account.enableWithService(model.serviceName)
                } else {
                    root.account.disableWithService(model.serviceName)
                }
            }
        }
    }

    SyncScheduleOptions {
        id: feedSchedule

        enabled: root.postsServiceEnabled
        schedule: root.postsSyncOptions ? root.postsSyncOptions.schedule : null
    }

    Loader {
        width: parent.width
        active: feedSchedule.schedule
                && feedSchedule.schedule.enabled
                && feedSchedule.schedule.peakScheduleEnabled

        sourceComponent: Component {
            PeakSyncOptions {
                schedule: feedSchedule.schedule
            }
        }
    }

    TextSwitch {
        id: autoSyncSwitch

        enabled: root.postsServiceEnabled
        //% "Sync when Events opens"
        text: qsTrId("settings-accounts-rss-la-auto_sync")
        //% "Fetch this feed once when the News group is opened in Events."
        description: qsTrId("settings-accounts-rss-la-auto_sync_description")

        onCheckedChanged: {
            if (root.account.identifier > 0) {
                autoSyncConf.value = checked
            }
        }
    }

    ConfigurationValue {
        id: autoSyncConf

        key: "/desktop/lipstick-jolla-home/events/auto_sync_feeds/"
             + root.account.identifier
    }
}
