/*
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RSSPOSTSMODEL_H
#define RSSPOSTSMODEL_H

#include "rsspostsdatabase.h"

#include <QAbstractListModel>
#include <QList>
#include <QMap>
#include <QVariant>

class RssPostsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList accountIdFilter READ accountIdFilter
               WRITE setAccountIdFilter NOTIFY accountIdFilterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        RssId = Qt::UserRole + 1,
        Name,
        Title,
        Body,
        Timestamp,
        TimestampValid,
        Icon,
        Images,
        Url,
        Link,
        FeedTitle,
        Author,
        Accounts
    };

    explicit RssPostsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList accountIdFilter() const;
    void setAccountIdFilter(const QVariantList &accountIds);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(const QString &identifier);
    Q_INVOKABLE void clear();

signals:
    void accountIdFilterChanged();
    void countChanged();

private slots:
    void postsChanged();

private:
    typedef QMap<int, QVariant> RowData;
    QList<RowData> m_data;
    RssPostsDatabase m_database;
};

#endif // RSSPOSTSMODEL_H
