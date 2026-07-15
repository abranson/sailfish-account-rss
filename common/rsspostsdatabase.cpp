// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rsspostsdatabase.h"

namespace {

const char DatabaseName[] = "rss.db";
const char UrlKey[] = "url";
const char FeedTitleKey[] = "feed_title";
const char AuthorKey[] = "author";
const char TimestampValidKey[] = "timestamp_valid";
const char DeletedLocallyKey[] = "deleted_locally";

}

RssPostsDatabase::RssPostsDatabase()
    : AbstractSocialPostCacheDatabase(QStringLiteral("rss"),
                                      QLatin1String(DatabaseName))
{
}

RssPostsDatabase::~RssPostsDatabase()
{
}

void RssPostsDatabase::addRssPost(
        const QString &identifier,
        const QString &title,
        const QString &body,
        const QDateTime &timestamp,
        const QString &feedIcon,
        const QList<QPair<QString, SocialPostImage::ImageType> > &images,
        const QString &url,
        const QString &feedTitle,
        const QString &author,
        bool timestampValid,
        bool deletedLocally,
        int accountId)
{
    QVariantMap extra;
    extra.insert(QLatin1String(UrlKey), url);
    extra.insert(QLatin1String(FeedTitleKey), feedTitle);
    extra.insert(QLatin1String(AuthorKey), author);
    extra.insert(QLatin1String(TimestampValidKey), timestampValid);
    extra.insert(QLatin1String(DeletedLocallyKey), deletedLocally);

    addPost(identifier, title, body, timestamp, feedIcon, images, extra, accountId);
}

void RssPostsDatabase::markPostDeletedLocally(const QString &identifier)
{
    const QList<SocialPost::ConstPtr> cachedPosts = posts();
    for (const SocialPost::ConstPtr &post : cachedPosts) {
        if (post && post->identifier() == identifier) {
            if (queuePostDeletedLocally(post)) {
                commit();
            }
            return;
        }
    }
}

void RssPostsDatabase::markAllPostsDeletedLocally()
{
    bool queued = false;
    const QList<SocialPost::ConstPtr> cachedPosts = posts();
    for (const SocialPost::ConstPtr &post : cachedPosts) {
        queued = queuePostDeletedLocally(post) || queued;
    }
    if (queued) {
        commit();
    }
}

QString RssPostsDatabase::url(const SocialPost::ConstPtr &post)
{
    return post ? post->extraString(QLatin1String(UrlKey)) : QString();
}

QString RssPostsDatabase::feedTitle(const SocialPost::ConstPtr &post)
{
    return post ? post->extraString(QLatin1String(FeedTitleKey)) : QString();
}

QString RssPostsDatabase::author(const SocialPost::ConstPtr &post)
{
    return post ? post->extraString(QLatin1String(AuthorKey)) : QString();
}

bool RssPostsDatabase::timestampValid(const SocialPost::ConstPtr &post)
{
    return post && post->extraBool(QLatin1String(TimestampValidKey));
}

bool RssPostsDatabase::deletedLocally(const SocialPost::ConstPtr &post)
{
    return post && post->extraBool(QLatin1String(DeletedLocallyKey));
}

bool RssPostsDatabase::queuePostDeletedLocally(const SocialPost::ConstPtr &post)
{
    if (!post || deletedLocally(post) || post->accounts().isEmpty()) {
        return false;
    }

    QList<QPair<QString, SocialPostImage::ImageType> > images;
    const QList<SocialPostImage::ConstPtr> postImages = post->images();
    for (const SocialPostImage::ConstPtr &image : postImages) {
        if (image) {
            images.append(qMakePair(image->url(), image->type()));
        }
    }

    QVariantMap extra = post->extra();
    extra.insert(QLatin1String(DeletedLocallyKey), true);
    const QList<int> accounts = post->accounts();
    for (int account : accounts) {
        addPost(post->identifier(), post->name(), post->body(), post->timestamp(),
                post->icon(), images, extra, account);
    }
    return true;
}
