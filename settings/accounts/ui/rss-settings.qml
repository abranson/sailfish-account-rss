// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0
import com.jolla.settings.accounts.rss 1.0

AccountSettingsAgent {
    id: root

    initialPage: Page {
        id: settingsPage

        RssDismissalRestorer {
            id: dismissalRestorer

            onRestoreFailed: {
                //% "Dismissed posts could not be restored"
                Notices.show(qsTrId("settings-accounts-rss-no-restore_dismissed_failed"))
            }
        }

        RemorsePopup {
            id: restoreDismissedRemorse
        }

        onPageContainerChanged: {
            if (pageContainer == null) {
                root.delayDeletion = true
                if (!settingsDisplay.feedUrlValid) {
                    settingsDisplay.discardInvalidFeedUrl()
                    settingsDisplay.saveAccount()
                } else if (settingsDisplay.feedUrlChanged) {
                    settingsDisplay.saveAccountAndSyncIfValid()
                } else {
                    settingsDisplay.saveAccount()
                }
            }
        }

        Component.onDestruction: {
            if (status === PageStatus.Active) {
                settingsDisplay.saveAccount(true)
            }
        }

        SilicaFlickable {
            anchors.fill: parent
            contentHeight: header.height + settingsDisplay.height + Theme.paddingLarge

            StandardAccountSettingsPullDownMenu {
                visible: settingsDisplay.accountValid
                allowCredentialsUpdate: false
                allowSync: true

                onAccountDeletionRequested: {
                    root.accountDeletionRequested()
                    pageStack.pop()
                }
                onSyncRequested: settingsDisplay.saveAccountAndSyncIfValid()

                MenuItem {
                    enabled: !dismissalRestorer.busy
                             && !restoreDismissedRemorse.active
                    //% "Restore dismissed posts"
                    text: qsTrId("settings-accounts-rss-me-restore_dismissed_posts")
                    onClicked: {
                        //% "Restoring dismissed posts"
                        restoreDismissedRemorse.execute(
                                    qsTrId("settings-accounts-rss-la-restoring_dismissed_posts"),
                                    function() { dismissalRestorer.restore(root.accountId) })
                    }
                }
            }

            PageHeader {
                id: header

                title: root.accountsHeaderText
            }

            RssSettingsDisplay {
                id: settingsDisplay

                anchors.top: header.bottom
                accountManager: root.accountManager
                accountProvider: root.accountProvider
                accountId: root.accountId

                onAccountSaveCompleted: root.delayDeletion = false
            }

            VerticalScrollDecorator {}
        }
    }
}
