// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import org.nemomobile.socialcache 1.0
import com.jolla.eventsview.rss 1.0
import QtQml.Models 2.1
import "shared"

SocialMediaAccountDelegate {
    id: delegateItem

    property int activeSyncCount
    property var _busyProfiles: ({})
    property bool _syncedForVisibility

    //% "News"
    headerText: qsTrId("lipstick-jolla-home-rss-la-header")
    headerIcon: "image://theme/icon-l-rss"
    showRemainingCount: false
    //% "Show fewer"
    expandedLabel: qsTrId("lipstick-jolla-home-rss-la-show_fewer")

    providerName: "rss"
    services: ["Posts"]
    periodicSyncLoopEnabled: false
    userRemovable: true

    model: RssPostsModel {}

    delegate: RssFeedItem {
        downloader: delegateItem.downloader
        imageList: model.images
        avatarSource: model.icon && model.icon.toString().length > 0
                      ? model.icon : "image://theme/icon-l-rss"
        fallbackAvatarSource: "image://theme/icon-l-rss"
        accountId: delegateItem.firstAccountId(model.accounts)
        userRemovable: true
        animateRemoval: defaultAnimateRemoval || delegateItem.removeAllInProgress

        onRemoveRequested: {
            delegateItem.model.remove(model.rssId)
        }

        onTriggered: {
            if (model.url && model.url.toString().length > 0) {
                Qt.openUrlExternally(model.url)
            }
        }

        Component.onCompleted: {
            refreshTimeCount = Qt.binding(function() {
                return delegateItem.refreshTimeCount
            })
            connectedToNetwork = Qt.binding(function() {
                return delegateItem.connectedToNetwork
            })
            eventsColumnMaxWidth = Qt.binding(function() {
                return delegateItem.eventsColumnMaxWidth
            })
        }
    }

    BusyIndicator {
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        size: BusyIndicatorSize.ExtraSmall
        running: delegateItem.activeSyncCount > 0
    }

    DBusInterface {
        id: syncStatusInterface

        service: "com.meego.msyncd"
        path: "/synchronizer"
        iface: "com.meego.msyncd"
        signalsEnabled: true

        function syncStatus(profileName, status, message, details) {
            if (String(profileName).indexOf("rss.Posts-") !== 0) {
                return
            }

            var active = status >= 0 && status <= 2 || status === 7
            var known = delegateItem._busyProfiles[profileName] === true
            if (active && !known) {
                delegateItem._busyProfiles[profileName] = true
                delegateItem.activeSyncCount++
            } else if (!active && known) {
                delete delegateItem._busyProfiles[profileName]
                delegateItem.activeSyncCount = Math.max(0,
                                                        delegateItem.activeSyncCount - 1)
            }

            if (!active) {
                delegateItem.model.refresh()
            }
        }
    }

    onExpandedClicked: collapsed = true

    Component.onCompleted: {
        refreshAccountsAndMaybeSync()
    }

    onViewVisibleChanged: {
        if (viewVisible) {
            refreshAccountsAndMaybeSync()
        } else {
            _syncedForVisibility = false
            _needToSync = false
        }
    }

    Connections {
        target: delegateItem.subviewModel ? delegateItem.subviewModel.manager : null

        onRefreshed: {
            delegateItem.refreshAccountsAndMaybeSync()
        }
        onAccountEnabledChanged: {
            delegateItem.refreshAccountsAndMaybeSync()
        }
    }

    function refreshAccountsAndMaybeSync() {
        resetHasSyncableAccounts()
        model.refresh()
        if (viewVisible && hasSyncableAccounts && !_syncedForVisibility) {
            _syncedForVisibility = true
            sync()
        }
    }

    function resetHasSyncableAccounts() {
        var accountIds = []
        _syncableAccountProfiles = []
        var accounts = subviewModel.accountList(providerName)
        for (var i = 0; i < accounts.length; ++i) {
            var account = accounts[i]
            if (!account.isEnabledWithService("rss-posts")) {
                continue
            }
            accountIds.push(account.identifier)
            if (subviewModel.shouldAutoSyncAccount(account.identifier)) {
                _syncableAccountProfiles.push("rss.Posts-" + account.identifier)
            }
        }
        model.accountIdFilter = accountIds
        hasSyncableAccounts = _syncableAccountProfiles.length > 0
    }

    function firstAccountId(accounts) {
        if (!accounts || accounts.length === 0) {
            return -1
        }
        var value = Number(accounts[0])
        return isNaN(value) ? -1 : value
    }
}
