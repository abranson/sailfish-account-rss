// SPDX-FileCopyrightText: 2019 Open Mobile Platform LLC
// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2024 - 2025 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rsspostsclient.h"

#include "rssfeedparser.h"
#include "rsspostsdatabase.h"

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>

#include <ProfileEngineDefs.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimer>
#include <QVariant>

#include <algorithm>

Q_LOGGING_CATEGORY(lcRssPosts, "buteo.plugin.rss.posts", QtWarningMsg)

namespace {

const int MaximumResponseSize = 5 * 1024 * 1024;
const int MaximumPosts = 20;
const int RequestTimeout = 60 * 1000;
const char DefaultFeedIcon[] = "image://theme/icon-l-rss";

bool newestFirst(const RssFeedItem &left, const RssFeedItem &right)
{
    if (left.timestamp != right.timestamp) {
        return left.timestamp > right.timestamp;
    }
    return left.sourceOrder < right.sourceOrder;
}

}

Buteo::ClientPlugin *RssPostsClientLoader::createClientPlugin(
        const QString &pluginName,
        const Buteo::SyncProfile &profile,
        Buteo::PluginCbInterface *callback)
{
    return new RssPostsClient(pluginName, profile, callback);
}

RssPostsClient::RssPostsClient(
        const QString &pluginName,
        const Buteo::SyncProfile &profile,
        Buteo::PluginCbInterface *callback)
    : Buteo::ClientPlugin(pluginName, profile, callback)
{
}

RssPostsClient::~RssPostsClient()
{
    if (m_reply) {
        m_reply->abort();
        delete m_reply;
    }
}

bool RssPostsClient::init()
{
    emit syncProgressDetail(iProfile.name(), Sync::SYNC_PROGRESS_INITIALISING);

    m_clientProfile = iProfile.clientProfile();
    m_accountId = iProfile.key(Buteo::KEY_ACCOUNT_ID).toInt();
    if (!m_clientProfile || m_accountId <= 0) {
        qCWarning(lcRssPosts) << "Missing client profile or account identifier";
        return false;
    }

    m_accountManager = new Accounts::Manager(this);
    m_networkManager = new QNetworkAccessManager(this);
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    m_database = new RssPostsDatabase;
    m_database->setParent(this);

    connect(m_timeoutTimer, &QTimer::timeout,
            this, &RssPostsClient::requestTimedOut);

    return loadAccountConfiguration();
}

bool RssPostsClient::uninit()
{
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    if (m_reply) {
        m_reply->abort();
    }
    return true;
}

bool RssPostsClient::loadAccountConfiguration()
{
    Accounts::Account *account = Accounts::Account::fromId(
                m_accountManager, m_accountId, this);
    if (!account) {
        qCWarning(lcRssPosts) << "Unable to load RSS account" << m_accountId;
        return false;
    }

    m_accountDisplayName = account->displayName();
    const Accounts::Service service = m_accountManager->service(
                QStringLiteral("rss-posts"));
    account->selectService(service);
    m_feedUrl = QUrl(account->value(QStringLiteral("feed_url")).toString().trimmed());
    account->deleteLater();

    const QString scheme = m_feedUrl.scheme().toLower();
    if (!m_feedUrl.isValid()
            || (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
        qCWarning(lcRssPosts) << "RSS account has an invalid feed URL";
        return false;
    }
    return true;
}

bool RssPostsClient::startSync()
{
    if (!m_networkManager || !m_timeoutTimer || m_reply) {
        return false;
    }

    m_completed = false;
    m_responseData.clear();

    QNetworkRequest request(m_feedUrl);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setRawHeader("Accept",
                         "application/rss+xml, application/atom+xml, "
                         "application/xml, text/xml;q=0.9, */*;q=0.1");
    request.setRawHeader("User-Agent", "Sailfish-RSS/1.0");

    emit syncProgressDetail(iProfile.name(), Sync::SYNC_PROGRESS_RECEIVING_ITEMS);
    m_reply = m_networkManager->get(request);
    connect(m_reply, &QIODevice::readyRead,
            this, &RssPostsClient::replyReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &RssPostsClient::replyFinished);
    m_timeoutTimer->start(RequestTimeout);
    return true;
}

void RssPostsClient::replyReadyRead()
{
    if (!m_reply || m_completed) {
        return;
    }

    m_responseData.append(m_reply->readAll());
    if (m_responseData.size() > MaximumResponseSize) {
        abortWithFailure(Buteo::SyncResults::INTERNAL_ERROR,
                         QStringLiteral("The RSS feed response is too large."));
    }
}

void RssPostsClient::replyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    if (!reply) {
        return;
    }

    if (!m_completed) {
        m_responseData.append(reply->readAll());
        const int httpStatus = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (m_responseData.size() > MaximumResponseSize) {
            completeFailure(Buteo::SyncResults::INTERNAL_ERROR,
                            QStringLiteral("The RSS feed response is too large."));
        } else if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcRssPosts) << "RSS request failed with network error"
                                  << reply->error() << "and HTTP status" << httpStatus;
            completeFailure(Buteo::SyncResults::CONNECTION_ERROR,
                            QStringLiteral("The RSS feed could not be downloaded."));
        } else if (httpStatus < 200 || httpStatus >= 300) {
            qCWarning(lcRssPosts) << "RSS request returned HTTP status" << httpStatus;
            completeFailure(Buteo::SyncResults::CONNECTION_ERROR,
                            QStringLiteral("The RSS server returned an error."));
        } else {
            emit syncProgressDetail(iProfile.name(), Sync::SYNC_PROGRESS_FINALISING);
            unsigned int added = 0;
            unsigned int deleted = 0;
            if (storeFeed(m_responseData, reply->url(), &added, &deleted)) {
                completeSuccess(added, deleted);
            }
        }
    }

    reply->deleteLater();
}

void RssPostsClient::requestTimedOut()
{
    if (m_reply && !m_completed) {
        abortWithFailure(Buteo::SyncResults::CONNECTION_ERROR,
                         QStringLiteral("The RSS feed request timed out."));
    }
}

QString RssPostsClient::postIdentifier(const QString &sourceIdentity) const
{
    const QByteArray input = QByteArray::number(m_accountId)
            + '\x1f' + sourceIdentity.toUtf8();
    return QString::fromLatin1(
                QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex());
}

bool RssPostsClient::storeFeed(const QByteArray &data, const QUrl &baseUrl,
                               unsigned int *added, unsigned int *deleted)
{
    RssFeed feed;
    QString parseError;
    if (!RssFeedParser::parse(data, baseUrl, &feed, &parseError)) {
        qCWarning(lcRssPosts) << parseError;
        completeFailure(Buteo::SyncResults::INTERNAL_ERROR,
                        QStringLiteral("The response is not a valid RSS or Atom feed."));
        return false;
    }

    QVariantList accountFilter;
    accountFilter.append(QVariant(m_accountId));
    m_database->setAccountIdFilter(accountFilter);
    m_database->refresh();
    m_database->wait();
    if (m_database->readStatus() != AbstractSocialCacheDatabase::Finished) {
        completeFailure(Buteo::SyncResults::DATABASE_FAILURE,
                        QStringLiteral("The existing RSS feed cache could not be read."));
        return false;
    }

    QHash<QString, QDateTime> previousTimestamps;
    QHash<QString, bool> previousTimestampValidities;
    QSet<QString> previousIdentifiers;
    QSet<QString> locallyDeletedIdentifiers;
    const QList<SocialPost::ConstPtr> previousPosts = m_database->posts();
    for (const SocialPost::ConstPtr &post : previousPosts) {
        previousTimestamps.insert(post->identifier(), post->timestamp());
        previousTimestampValidities.insert(
                    post->identifier(), RssPostsDatabase::timestampValid(post));
        previousIdentifiers.insert(post->identifier());
        if (RssPostsDatabase::deletedLocally(post)) {
            locallyDeletedIdentifiers.insert(post->identifier());
        }
    }

    const QDateTime fetchedAt = QDateTime::currentDateTimeUtc();
    QList<RssFeedItem> processedItems;
    for (RssFeedItem &item : feed.items) {
        QString identity = item.sourceId.trimmed();
        if (identity.isEmpty()) {
            identity = item.link;
        }
        if (identity.isEmpty()) {
            identity = item.title + QLatin1Char('\x1f')
                    + item.timestamp.toString(Qt::ISODate) + QLatin1Char('\x1f')
                    + item.body;
        }
        item.sourceId = postIdentifier(identity);

        if (!item.timestampValid) {
            if (previousTimestamps.contains(item.sourceId)) {
                item.timestamp = previousTimestamps.value(item.sourceId);
                item.timestampValid = previousTimestampValidities.value(item.sourceId);
            } else {
                item.timestamp = fetchedAt.addSecs(-item.sourceOrder);
            }
        }
        processedItems.append(item);
    }

    std::stable_sort(processedItems.begin(), processedItems.end(), newestFirst);
    QSet<QString> identifiers;
    feed.items.clear();
    for (const RssFeedItem &item : processedItems) {
        if (identifiers.contains(item.sourceId)) {
            continue;
        }
        identifiers.insert(item.sourceId);
        feed.items.append(item);
        if (feed.items.count() == MaximumPosts) {
            break;
        }
    }

    QString feedTitle = feed.title.trimmed();
    if (feedTitle.isEmpty()) {
        feedTitle = m_accountDisplayName.trimmed();
    }
    if (feedTitle.isEmpty()) {
        feedTitle = baseUrl.host();
    }

    QString feedIcon = feed.icon.trimmed();
    if (feedIcon.isEmpty()) {
        // SocialPost::icon() expects image position zero whenever a post has media.
        feedIcon = QLatin1String(DefaultFeedIcon);
    }

    m_database->removePosts(m_accountId);
    for (const RssFeedItem &item : feed.items) {
        QList<QPair<QString, SocialPostImage::ImageType> > images;
        for (const QString &image : item.images) {
            images.append(qMakePair(image, SocialPostImage::Photo));
        }
        m_database->addRssPost(item.sourceId, item.title, item.body,
                               item.timestamp, feedIcon, images, item.link,
                               feedTitle, item.author, item.timestampValid,
                               locallyDeletedIdentifiers.contains(item.sourceId),
                               m_accountId);
    }

    m_database->commit();
    m_database->wait();
    if (m_database->writeStatus() != AbstractSocialCacheDatabase::Finished) {
        completeFailure(Buteo::SyncResults::DATABASE_FAILURE,
                        QStringLiteral("The RSS feed cache could not be updated."));
        return false;
    }

    if (added) {
        unsigned int count = 0;
        for (const QString &identifier : identifiers) {
            if (!previousIdentifiers.contains(identifier)) {
                ++count;
            }
        }
        *added = count;
    }
    if (deleted) {
        unsigned int count = 0;
        for (const QString &identifier : previousIdentifiers) {
            if (!identifiers.contains(identifier)) {
                ++count;
            }
        }
        *deleted = count;
    }
    return true;
}

void RssPostsClient::completeSuccess(unsigned int added, unsigned int deleted)
{
    if (m_completed) {
        return;
    }
    m_completed = true;
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }

    m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                   Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                   Buteo::SyncResults::NO_ERROR);
    m_results.addTargetResults(Buteo::TargetResults(
                                   m_accountDisplayName,
                                   Buteo::ItemCounts(added, deleted, 0),
                                   Buteo::ItemCounts()));
    emit success(iProfile.name(), QStringLiteral("RSS feed updated successfully."));
}

void RssPostsClient::completeFailure(Buteo::SyncResults::MinorCode code,
                                     const QString &message)
{
    if (m_completed) {
        return;
    }
    m_completed = true;
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }

    m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                   Buteo::SyncResults::SYNC_RESULT_FAILED,
                                   code);
    emit error(iProfile.name(), message, code);
}

void RssPostsClient::abortWithFailure(Buteo::SyncResults::MinorCode code,
                                      const QString &message)
{
    if (m_completed) {
        return;
    }
    m_completed = true;
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }

    m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                   Buteo::SyncResults::SYNC_RESULT_FAILED,
                                   code);
    if (m_reply) {
        m_reply->abort();
    }
    emit error(iProfile.name(), message, code);
}

void RssPostsClient::abortSync(Sync::SyncStatus status)
{
    abortWithFailure(status == Sync::SYNC_CONNECTION_ERROR
                     ? Buteo::SyncResults::CONNECTION_ERROR
                     : Buteo::SyncResults::ABORTED,
                     status == Sync::SYNC_CONNECTION_ERROR
                     ? QStringLiteral("Network connectivity was lost.")
                     : QStringLiteral("RSS synchronization was aborted."));
}

Buteo::SyncResults RssPostsClient::getSyncResults() const
{
    return m_results;
}

bool RssPostsClient::cleanUp()
{
    int accountId = m_accountId;
    if (accountId <= 0) {
        accountId = iProfile.key(Buteo::KEY_ACCOUNT_ID).toInt();
    }
    if (accountId <= 0) {
        return false;
    }

    RssPostsDatabase database;
    database.removePosts(accountId);
    database.commit();
    database.wait();
    return database.writeStatus() == AbstractSocialCacheDatabase::Finished;
}

void RssPostsClient::connectivityStateChanged(
        Sync::ConnectivityType type, bool state)
{
    if (type == Sync::CONNECTIVITY_INTERNET && !state && m_reply) {
        abortSync(Sync::SYNC_CONNECTION_ERROR);
    }
}
