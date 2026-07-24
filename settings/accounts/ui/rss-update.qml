// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

AccountCredentialsAgent {
    id: root

    canCancelUpdate: true

    initialPage: Dialog {
        id: updateDialog

        acceptDestinationAction: PageStackAction.Push
        acceptDestination: AccountBusyPage {
            busyDescription: updatingAccountText
        }

        property bool _credentialUpdateInProgress
        property bool _checkMandatoryFields
        property bool _saving
        property string _feedUrl: {
            var values = account.configurationValues("rss-posts")
            return values["feed_url"] ? values["feed_url"].toString() : ""
        }
        property bool _authenticatedUrlSecure: /^https:\/\//i.test(_feedUrl)
        property bool _authenticationEnabled: {
            var values = account.configurationValues("rss-posts")
            return values["requires_auth"] === true
        }
        property string _oldUsername: {
            var values = account.configurationValues("rss-posts")
            return values["auth_username"] ? values["auth_username"].toString() : ""
        }

        function _configurationValues() {
            var configValues = { "": account.configurationValues("") }
            var serviceNames = account.supportedServiceNames
            for (var si = 0; si < serviceNames.length; ++si) {
                configValues[serviceNames[si]] = account.configurationValues(
                            serviceNames[si])
            }
            return configValues
        }

        function _saveAuthenticationSettings(requiresAuthentication) {
            account.setConfigurationValue(
                        "rss-posts", "requires_auth", requiresAuthentication)
            account.setConfigurationValue(
                        "rss-posts", "auth_username",
                        requiresAuthentication ? usernameField.text : "")
            // Keep the feed description as the account-list subtitle.
            account.setConfigurationValue(
                        "", "default_credentials_username", account.displayName)
            _saving = true
            account.sync()
        }

        function _updateCredentials() {
            if (!authenticationSwitch.checked) {
                if (account.hasSignInCredentials("Jolla", "Jolla")) {
                    account.removeSignInCredentials("Jolla", "Jolla")
                }
                _saveAuthenticationSettings(false)
                return
            }

            _credentialUpdateInProgress = true
            if (account.hasSignInCredentials("Jolla", "Jolla")) {
                account.updateSignInCredentials(
                            "Jolla", "Jolla",
                            account.signInParameters(
                                "rss-posts", usernameField.text, passwordField.text))
            } else {
                accountFactory.recreateAccountCredentials(
                            account.identifier, "rss-posts",
                            usernameField.text, passwordField.text,
                            account.signInParameters(
                                "rss-posts", usernameField.text, passwordField.text),
                            "Jolla", "", "Jolla", _configurationValues())
            }
        }

        function _updateFailed(message) {
            _credentialUpdateInProgress = false
            _saving = false
            root.credentialsUpdateError(message || account.errorMessage)
            var busyPage = acceptDestination
            busyPage.state = "info"
            busyPage.infoHeading = busyPage.errorHeadingText
            busyPage.infoDescription = busyPage.accountUpdateErrorText
        }

        canAccept: !authenticationSwitch.checked
                   || (_authenticatedUrlSecure
                       && usernameField.text.length > 0
                       && passwordField.text.length > 0)

        onAcceptPendingChanged: {
            if (acceptPending) {
                _checkMandatoryFields = true
            }
        }

        onStatusChanged: {
            // Wait for the transition so the busy page is ready for errors.
            if (status === PageStatus.Inactive && result === DialogResult.Accepted) {
                _updateCredentials()
            }
        }

        onRejected: {
            if (account.status === Account.SigningIn) {
                account.cancelSignInOperation()
            }
        }

        DialogHeader {
            id: pageHeader
        }

        Column {
            anchors.top: pageHeader.bottom
            width: parent.width
            spacing: Theme.paddingLarge

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraLarge
                color: Theme.highlightColor
                //% "Authentication"
                text: qsTrId("settings-accounts-rss-la-authentication")
            }

            TextSwitch {
                id: authenticationSwitch

                width: parent.width
                checked: updateDialog._authenticationEnabled
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
                    text: updateDialog._oldUsername
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
                    EnterKey.enabled: updateDialog.canAccept
                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: updateDialog.accept()

                    onTextChanged: errorHighlight = false
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    visible: !updateDialog._authenticatedUrlSecure
                    color: Theme.errorColor
                    wrapMode: Text.WordWrap
                    //% "Authenticated feeds must use HTTPS"
                    text: qsTrId("settings-accounts-rss-la-authentication_requires_https")
                }
            }
        }
    }

    Account {
        id: account

        identifier: root.accountId

        onSignInCredentialsUpdated: {
            if (updateDialog._credentialUpdateInProgress) {
                updateDialog._credentialUpdateInProgress = false
                updateDialog._saveAuthenticationSettings(true)
            }
        }

        onSignInError: updateDialog._updateFailed(errorMessage)

        onStatusChanged: {
            if (updateDialog._saving && status === Account.Synced) {
                updateDialog._saving = false
                root.credentialsUpdated(identifier)
                root.goToEndDestination()
            } else if (updateDialog._saving
                       && (status === Account.Error || status === Account.Invalid)) {
                updateDialog._updateFailed(account.errorMessage)
            }
        }
    }
}
