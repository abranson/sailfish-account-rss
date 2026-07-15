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

namespace Accounts {
class Manager;
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

private:
    bool loadAccountConfiguration();
    bool storeFeed(const QByteArray &data, const QUrl &baseUrl,
                   unsigned int *added, unsigned int *deleted);
    QString postIdentifier(const QString &sourceIdentity) const;
    void completeSuccess(unsigned int added, unsigned int deleted);
    void completeFailure(Buteo::SyncResults::MinorCode code,
                         const QString &message);
    void abortWithFailure(Buteo::SyncResults::MinorCode code,
                          const QString &message);

    const Buteo::Profile *m_clientProfile = nullptr;
    Accounts::Manager *m_accountManager = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    RssPostsDatabase *m_database = nullptr;
    QByteArray m_responseData;
    QUrl m_feedUrl;
    QString m_accountDisplayName;
    int m_accountId = 0;
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
