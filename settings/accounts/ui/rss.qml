// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0
import com.jolla.settings.accounts.rss 1.0
import Nemo.Configuration 1.0

AccountCreationAgent {
    id: root

    property string feedUrl
    property string feedTitle
    property bool requiresAuthentication
    property string authenticationUsername
    property string authenticationPassword
    property int _accountId
    property bool _configured
    property bool _credentialsCreated
    property bool _credentialsCreationStarted
    property bool _reported
    property bool _rollingBack
    property string _rollbackMessage

    function normalizeUrl(value) {
        var url = value ? value.trim() : ""
        if (url.length === 0) {
            return ""
        }
        if (!/^https?:\/\//i.test(url)) {
            url = "https://" + url
        }
        return url
    }

    function isHttpsUrl(value) {
        return /^https:\/\//i.test(normalizeUrl(value))
    }

    function fallbackTitle(url) {
        var withoutScheme = url.replace(/^https?:\/\//i, "")
        return withoutScheme.split("/")[0]
    }

    function registeredAccountId(url) {
        var normalizedUrl = normalizeUrl(url)
        var accountIds = existingAccountManager.providerAccountIdentifiers("rss")
        for (var i = 0; i < accountIds.length; ++i) {
            var account = existingAccountManager.account(accountIds[i])
            if (account && normalizeUrl(
                        account.configurationValues("rss-posts")["feed_url"])
                    === normalizedUrl) {
                return accountIds[i]
            }
        }
        return 0
    }

    function setFeedUrl(url) {
        feedUrl = normalizeUrl(url)
        return registeredAccountId(feedUrl) === 0
    }

    AccountManager {
        id: existingAccountManager
    }

    function showError(busyPage, message) {
        if (busyPage.state === "info") {
            return
        }
        busyPage.state = "info"
        busyPage.infoExtraDescription = message
        accountCreationError(message)
        delayDeletion = false
    }

    initialPage: Dialog {
        id: setupDialog

        property bool authenticationUrlSecure: root.isHttpsUrl(urlField.text)

        canAccept: urlField.acceptableInput
                   && (!authenticationSwitch.checked
                       || (authenticationUrlSecure
                           && usernameField.text.length > 0
                           && passwordField.text.length > 0))
        acceptDestination: busyComponent
        acceptDestinationAction: PageStackAction.Push

        onAccepted: {
            root.feedUrl = root.normalizeUrl(urlField.text)
            root.requiresAuthentication = authenticationSwitch.checked
            root.authenticationUsername = authenticationSwitch.checked
                    ? usernameField.text : ""
            root.authenticationPassword = authenticationSwitch.checked
                    ? passwordField.text : ""
        }
        onAcceptBlocked: {
            urlField.errorHighlight = !urlField.acceptableInput
                    || (authenticationSwitch.checked && !authenticationUrlSecure)
            usernameField.errorHighlight = authenticationSwitch.checked
                    && usernameField.text.length === 0
            passwordField.errorHighlight = authenticationSwitch.checked
                    && passwordField.text.length === 0
        }

        Column {
            width: parent.width

            DialogHeader {
                //% "Next"
                acceptText: qsTrId("settings-accounts-rss-bt-next")
            }

            Item {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                height: icon.height + Theme.paddingLarge

                Image {
                    id: icon

                    width: Theme.iconSizeLarge
                    height: width
                    source: "image://theme/icon-l-rss"
                }

                Label {
                    anchors {
                        left: icon.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        verticalCenter: icon.verticalCenter
                    }
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    truncationMode: TruncationMode.Fade
                    //% "News feed"
                    text: qsTrId("settings-accounts-rss-la-feed")
                }
            }

            TextField {
                id: urlField

                width: parent.width
                focus: true
                text: root.feedUrl
                validator: RegExpValidator {
                    regExp: /^(https?:\/\/)?[^\s\/]+\.[^\s\/]+(\/.*)?$/i
                }
                inputMethodHints: Qt.ImhUrlCharactersOnly
                                  | Qt.ImhNoPredictiveText
                                  | Qt.ImhNoAutoUppercase
                //% "Feed URL"
                label: qsTrId("settings-accounts-rss-la-feed_url")
                //% "https://example.com/feed.xml"
                placeholderText: qsTrId("settings-accounts-rss-ph-feed_url")
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.enabled: acceptableInput
                EnterKey.onClicked: setupDialog.accept()

                onTextChanged: errorHighlight = false
                onActiveFocusChanged: {
                    if (!activeFocus && text.length > 0) {
                        errorHighlight = !acceptableInput
                    }
                }

                description: {
                    if (!errorHighlight) {
                        return ""
                    }
                    if (text.length === 0) {
                        //% "Feed URL is required"
                        return qsTrId("settings-accounts-rss-la-feed_url_required")
                    }
                    if (authenticationSwitch.checked
                            && !setupDialog.authenticationUrlSecure) {
                        //% "Authenticated feeds must use HTTPS"
                        return qsTrId("settings-accounts-rss-la-authentication_requires_https")
                    }
                    //% "Enter a valid HTTP or HTTPS feed URL"
                    return qsTrId("settings-accounts-rss-la-feed_url_invalid")
                }
            }

            TextSwitch {
                id: authenticationSwitch

                width: parent.width
                //% "Requires authentication"
                text: qsTrId("settings-accounts-rss-la-requires_authentication")
                //% "Use HTTP Basic authentication for this feed."
                description: qsTrId(
                                 "settings-accounts-rss-la-requires_authentication_description")

                onCheckedChanged: {
                    if (!checked) {
                        usernameField.errorHighlight = false
                        passwordField.text = ""
                        passwordField.errorHighlight = false
                    }
                }
            }

            Column {
                width: parent.width
                visible: authenticationSwitch.checked

                TextField {
                    id: usernameField

                    width: parent.width
                    inputMethodHints: Qt.ImhNoPredictiveText
                                      | Qt.ImhNoAutoUppercase
                    //% "Username"
                    label: qsTrId("settings-accounts-rss-la-username")
                    description: {
                        if (!errorHighlight) {
                            return ""
                        }
                        //% "Username is required"
                        return qsTrId("settings-accounts-rss-la-username_required")
                    }
                    EnterKey.iconSource: "image://theme/icon-m-enter-next"
                    EnterKey.onClicked: passwordField.forceActiveFocus()

                    onTextChanged: errorHighlight = false
                }

                PasswordField {
                    id: passwordField

                    width: parent.width
                    //% "Password"
                    label: qsTrId("settings-accounts-rss-la-password")
                    description: {
                        if (!errorHighlight) {
                            return ""
                        }
                        //% "Password is required"
                        return qsTrId("settings-accounts-rss-la-password_required")
                    }
                    EnterKey.enabled: setupDialog.canAccept
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: setupDialog.accept()

                    onTextChanged: errorHighlight = false
                }
            }
        }
    }

    Component {
        id: busyComponent

        AccountBusyPage {
            id: busyPage

            property int feedRequestId

            //% "Checking feed"
            busyDescription: qsTrId("settings-accounts-rss-la-checking_feed")

            function cancelRequest() {
                if (feedRequestId !== 0) {
                    feedValidator.cancelFetch(feedRequestId)
                    feedRequestId = 0
                }
            }

            function startRequest() {
                feedRequestId = feedValidator.fetch(
                            root.feedUrl,
                            root.authenticationUsername,
                            root.authenticationPassword,
                            root.requiresAuthentication)
            }

            function createAccount() {
                if (root._accountId !== 0 || root._rollingBack) {
                    return
                }
                root.delayDeletion = true
                if (!accountManager.createAccount("rss")) {
                    //% "The News account could not be created."
                    root.showError(
                                busyPage,
                                qsTrId("settings-accounts-rss-la-account_create_failed"))
                }
            }

            function rollbackAccount(message) {
                if (root._rollingBack || root._reported) {
                    return
                }
                if (root._accountId === 0) {
                    root.showError(busyPage, message)
                    return
                }

                root._rollingBack = true
                root._rollbackMessage = message
                root.delayDeletion = true
                newAccount.remove()
            }

            RssFeedValidator {
                id: feedValidator

                onFetchFinished: {
                    if (requestId !== busyPage.feedRequestId) {
                        return
                    }

                    busyPage.feedRequestId = 0
                    if (tooLarge) {
                        //% "The feed is too large."
                        root.showError(
                                    busyPage,
                                    qsTrId("settings-accounts-rss-la-feed_too_large"))
                        return
                    }
                    if (timedOut) {
                        //% "The feed request timed out."
                        root.showError(
                                    busyPage,
                                    qsTrId("settings-accounts-rss-la-request_timed_out"))
                        return
                    }
                    if (!success) {
                        //% "The feed could not be downloaded."
                        root.showError(
                                    busyPage,
                                    qsTrId("settings-accounts-rss-la-download_failed"))
                        return
                    }

                    var details = feedValidator.validate(data, responseUrl)
                    if (details.tooLarge) {
                        //% "The feed is too large."
                        root.showError(
                                    busyPage,
                                    qsTrId("settings-accounts-rss-la-feed_too_large"))
                        return
                    }
                    if (!details.valid) {
                        //% "The URL does not contain an RSS 2.0 or Atom 1.0 feed."
                        root.showError(
                                    busyPage,
                                    qsTrId("settings-accounts-rss-la-invalid_feed"))
                        return
                    }

                    var title = details.title ? String(details.title).trim() : ""
                    root.feedTitle = title.length > 0
                            ? title : root.fallbackTitle(root.feedUrl)
                    busyPage.createAccount()
                }
            }

            function finishCreation() {
                if (root._reported) {
                    return
                }
                root._reported = true
                autoSyncConf.key = "/desktop/lipstick-jolla-home/events/auto_sync_feeds/"
                        + newAccount.identifier
                autoSyncConf.value = true
                syncAdapter.triggerSync(newAccount)
                root.accountCreated(newAccount.identifier)
                root.delayDeletion = false
                root.goToEndDestination()
            }

            Timer {
                id: requestStartTimer

                interval: 300
                running: busyPage.status === PageStatus.Active
                onTriggered: busyPage.startRequest()
            }

            AccountManager {
                id: accountManager

                onAccountCreated: {
                    root._accountId = accountId
                    newAccount.identifier = accountId
                }
                onAccountCreationFailed: {
                    if (providerName !== "rss" || root._accountId !== 0) {
                        return
                    }
                    //% "The News account could not be created."
                    root.showError(
                                busyPage,
                                qsTrId("settings-accounts-rss-la-account_create_failed"))
                }
            }

            AccountSyncAdapter {
                id: syncAdapter

                accountManager: accountManager
            }

            AccountSyncManager {
                id: profileManager

                onAllProfilesCreated: busyPage.finishCreation()
                onAllProfileCreationError: {
                    //% "The News synchronization profile could not be created."
                    busyPage.rollbackAccount(
                                qsTrId("settings-accounts-rss-la-profile_create_failed"))
                }
            }

            Account {
                id: newAccount

                onStatusChanged: {
                    if (status === Account.Invalid && root._rollingBack) {
                        var message = root._rollbackMessage
                        root._rollingBack = false
                        root._rollbackMessage = ""
                        root._accountId = 0
                        root._configured = false
                        root._reported = false
                        identifier = 0
                        root.showError(busyPage, message)
                    } else if (status === Account.Initialized && !root._configured) {
                        root._configured = true
                        displayName = root.feedTitle
                        // Matching values show the provider as the account-list title.
                        setConfigurationValue(
                                    "", "default_credentials_username", root.feedTitle)
                        setConfigurationValue("rss-posts", "feed_url", root.feedUrl)
                        setConfigurationValue(
                                    "rss-posts", "requires_auth",
                                    root.requiresAuthentication)
                        setConfigurationValue(
                                    "rss-posts", "auth_username",
                                    root.requiresAuthentication
                                    ? root.authenticationUsername : "")
                        setConfigurationValue("", "FeedViewAutoSync", true)
                        enableWithService("rss-posts")
                        enabled = true
                        sync()
                    } else if (status === Account.Synced
                               && root._configured
                               && !root._reported) {
                        if (root.requiresAuthentication && !root._credentialsCreated) {
                            if (!root._credentialsCreationStarted) {
                                root._credentialsCreationStarted = true
                                createSignInCredentials(
                                            "Jolla", "Jolla",
                                            signInParameters(
                                                "rss-posts",
                                                root.authenticationUsername,
                                                root.authenticationPassword))
                            }
                        } else if (profileManager.createAllProfiles(identifier) === 0) {
                            busyPage.finishCreation()
                        }
                    } else if ((status === Account.Error || status === Account.Invalid)
                               && root._configured && !root._reported
                               && !root._rollingBack) {
                        //% "The News account could not be saved."
                        busyPage.rollbackAccount(
                                    qsTrId("settings-accounts-rss-la-account_save_failed"))
                    }
                }

                onSignInCredentialsCreated: {
                    if (!root._credentialsCreationStarted
                            || root._credentialsCreated) {
                        return
                    }

                    root._credentialsCreationStarted = false
                    root._credentialsCreated = true
                    root.authenticationPassword = ""
                    // Credentials creation updates this value to the username.
                    setConfigurationValue(
                                "", "default_credentials_username", root.feedTitle)
                    sync()
                }

                onSignInError: {
                    if (!root._credentialsCreationStarted
                            || root._rollingBack || root._reported) {
                        return
                    }

                    root._credentialsCreationStarted = false
                    root.authenticationPassword = ""
                    //% "The feed credentials could not be saved."
                    busyPage.rollbackAccount(
                                qsTrId("settings-accounts-rss-la-account_credentials_save_failed"))
                }
            }

            ConfigurationValue {
                id: autoSyncConf
            }

            Component.onDestruction: cancelRequest()
        }
    }
}
