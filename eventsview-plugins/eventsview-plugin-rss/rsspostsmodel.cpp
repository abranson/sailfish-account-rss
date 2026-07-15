// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rsspostsmodel.h"

namespace {

QVariantMap imageData(const SocialPostImage::ConstPtr &image)
{
    QVariantMap data;
    if (!image) {
        return data;
    }
    data.insert(QStringLiteral("url"), image->url());
    data.insert(QStringLiteral("type"), QStringLiteral("photo"));
    return data;
}

QVariantList imageList(const QList<SocialPostImage::ConstPtr> &images)
{
    QVariantList result;
    for (const SocialPostImage::ConstPtr &image : images) {
        result.append(imageData(image));
    }
    return result;
}

QVariantList accountList(const QList<int> &accounts)
{
    QVariantList result;
    for (int account : accounts) {
        result.append(QVariant(account));
    }
    return result;
}

}

RssPostsModel::RssPostsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_database, &AbstractSocialPostCacheDatabase::postsChanged,
            this, &RssPostsModel::postsChanged);
    connect(&m_database, &AbstractSocialPostCacheDatabase::accountIdFilterChanged,
            this, &RssPostsModel::accountIdFilterChanged);
}

int RssPostsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.count();
}

QVariant RssPostsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }
    return m_data.at(index.row()).value(role);
}

QHash<int, QByteArray> RssPostsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(RssId, "rssId");
    roles.insert(Name, "name");
    roles.insert(Title, "title");
    roles.insert(Body, "body");
    roles.insert(Timestamp, "timestamp");
    roles.insert(TimestampValid, "timestampValid");
    roles.insert(Icon, "icon");
    roles.insert(Images, "images");
    roles.insert(Url, "url");
    roles.insert(Link, "link");
    roles.insert(FeedTitle, "feedTitle");
    roles.insert(Author, "author");
    roles.insert(Accounts, "accounts");
    return roles;
}

QVariantList RssPostsModel::accountIdFilter() const
{
    return m_database.accountIdFilter();
}

void RssPostsModel::setAccountIdFilter(const QVariantList &accountIds)
{
    QVariantList normalized;
    for (const QVariant &accountId : accountIds) {
        bool ok = false;
        const int value = accountId.toInt(&ok);
        if (ok && !normalized.contains(QVariant(value))) {
            normalized.append(QVariant(value));
        }
    }

    if (normalized == m_database.accountIdFilter()) {
        return;
    }
    m_database.setAccountIdFilter(normalized);
}

void RssPostsModel::refresh()
{
    m_database.refresh();
}

void RssPostsModel::remove(const QString &identifier)
{
    for (int row = 0; row < m_data.count(); ++row) {
        if (m_data.at(row).value(RssId).toString() != identifier) {
            continue;
        }

        m_database.markPostDeletedLocally(identifier);
        beginRemoveRows(QModelIndex(), row, row);
        m_data.removeAt(row);
        endRemoveRows();
        emit countChanged();
        return;
    }
}

void RssPostsModel::clear()
{
    if (m_data.isEmpty()) {
        return;
    }

    m_database.markAllPostsDeletedLocally();
    beginRemoveRows(QModelIndex(), 0, m_data.count() - 1);
    m_data.clear();
    endRemoveRows();
    emit countChanged();
}

void RssPostsModel::postsChanged()
{
    QList<RowData> newData;
    const QList<SocialPost::ConstPtr> posts = m_database.posts();
    for (const SocialPost::ConstPtr &post : posts) {
        if (RssPostsDatabase::deletedLocally(post)) {
            continue;
        }

        RowData row;
        row.insert(RssId, post->identifier());
        row.insert(Name, post->name());
        row.insert(Title, post->name());
        row.insert(Body, post->body());
        row.insert(Timestamp, post->timestamp());
        row.insert(TimestampValid, RssPostsDatabase::timestampValid(post));
        row.insert(Icon, post->icon());
        row.insert(Images, imageList(post->images()));
        row.insert(Url, RssPostsDatabase::url(post));
        row.insert(Link, RssPostsDatabase::url(post));
        row.insert(FeedTitle, RssPostsDatabase::feedTitle(post));
        row.insert(Author, RssPostsDatabase::author(post));
        row.insert(Accounts, accountList(post->accounts()));
        newData.append(row);
    }

    if (newData == m_data) {
        return;
    }

    const int oldCount = m_data.count();
    beginResetModel();
    m_data = newData;
    endResetModel();
    if (oldCount != m_data.count()) {
        emit countChanged();
    }
}
