/*
 * SPDX-FileCopyrightText: 2019 Open Mobile Platform LLC
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2024 - 2025 Jolla Mobile Ltd
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RSSPOSTSCLIENT_H
#define RSSPOSTSCLIENT_H

#include <ClientPlugin.h>
#include <SyncPluginLoader.h>
#include <SyncResults.h>

#include <QByteArray>
#include <QUrl>
#include <QVariant>

namespace Accounts {
class Manager;
}
namespace SignOn {
class AuthSession;
class Error;
class Identity;
class SessionData;
}

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class RssPostsDatabase;

class RssPostsClient final : public Buteo::ClientPlugin
{
    Q_OBJECT

public:
    RssPostsClient(const QString &pluginName,
                   const Buteo::SyncProfile &profile,
                   Buteo::PluginCbInterface *callback);
    ~RssPostsClient() override;

    bool init() override;
    bool uninit() override;
    bool startSync() override;
    void abortSync(Sync::SyncStatus status = Sync::SYNC_ABORTED) override;
    Buteo::SyncResults getSyncResults() const override;
    bool cleanUp() override;

public slots:
    void connectivityStateChanged(Sync::ConnectivityType type, bool state) override;

private slots:
    void replyReadyRead();
    void replyFinished();
    void requestTimedOut();
    void credentialsReady(const SignOn::SessionData &credentials);
    void credentialsError(const SignOn::Error &error);

private:
    bool loadAccountConfiguration();
    bool readCredentials();
    bool startRequest(const QUrl &url);
    bool storeFeed(const QByteArray &data, const QUrl &baseUrl,
                   unsigned int *added, unsigned int *deleted);
    QString postIdentifier(const QString &sourceIdentity) const;
    void clearSignOnSession();
    void setCredentialsNeedUpdate();
    void completeSuccess(unsigned int added, unsigned int deleted);
    void completeFailure(Buteo::SyncResults::MinorCode code,
                         const QString &message);
    void abortWithFailure(Buteo::SyncResults::MinorCode code,
                          const QString &message);

    const Buteo::Profile *m_clientProfile = nullptr;
    Accounts::Manager *m_accountManager = nullptr;
    SignOn::Identity *m_signonIdentity = nullptr;
    SignOn::AuthSession *m_signonSession = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    RssPostsDatabase *m_database = nullptr;
    QByteArray m_responseData;
    QByteArray m_authorization;
    QUrl m_feedUrl;
    QUrl m_authenticatedOrigin;
    QString m_accountDisplayName;
    QString m_authUsername;
    QString m_authMethod;
    QString m_authMechanism;
    QVariantMap m_authParameters;
    int m_accountId = 0;
    int m_credentialsId = 0;
    int m_redirectCount = 0;
    bool m_requiresAuthentication = false;
    bool m_completed = false;
    Buteo::SyncResults m_results;
};

class RssPostsClientLoader final : public Buteo::SyncPluginLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfishos.plugins.sync.RssPostsClientLoader")
    Q_INTERFACES(Buteo::SyncPluginLoader)

public:
    Buteo::ClientPlugin *createClientPlugin(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *callback) override;
};

#endif // RSSPOSTSCLIENT_H
