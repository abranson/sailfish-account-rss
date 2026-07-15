/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RSSFEEDPARSER_H
#define RSSFEEDPARSER_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

struct RssFeedItem
{
    QString sourceId;
    QString title;
    QString body;
    QString author;
    QString link;
    QDateTime timestamp;
    bool timestampValid = false;
    QStringList images;
    int sourceOrder = 0;
};

struct RssFeed
{
    QString title;
    QString link;
    QString icon;
    QString author;
    QList<RssFeedItem> items;
};

class RssFeedParser
{
public:
    static bool parse(const QByteArray &data, const QUrl &baseUrl,
                      RssFeed *feed, QString *errorMessage = nullptr);

    static QString toPlainText(const QString &input);
    static QDateTime parseDate(const QString &input, bool *valid = nullptr);
};

#endif // RSSFEEDPARSER_H
