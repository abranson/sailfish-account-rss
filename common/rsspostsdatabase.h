/*
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RSSPOSTSDATABASE_H
#define RSSPOSTSDATABASE_H

#include <socialcache/abstractsocialpostcachedatabase.h>

class RssPostsDatabase : public AbstractSocialPostCacheDatabase
{
    Q_OBJECT

public:
    RssPostsDatabase();
    ~RssPostsDatabase() override;

    void addRssPost(const QString &identifier,
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
                    int accountId);

    void markPostDeletedLocally(const QString &identifier);
    void markAllPostsDeletedLocally();

    static QString url(const SocialPost::ConstPtr &post);
    static QString feedTitle(const SocialPost::ConstPtr &post);
    static QString author(const SocialPost::ConstPtr &post);
    static bool timestampValid(const SocialPost::ConstPtr &post);
    static bool deletedLocally(const SocialPost::ConstPtr &post);

private:
    bool queuePostDeletedLocally(const SocialPost::ConstPtr &post);
};

#endif // RSSPOSTSDATABASE_H
