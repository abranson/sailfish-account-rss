// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rsspostsmodel.h"

#include <QQmlExtensionPlugin>
#include <QtQml>

class JollaEventsviewRssPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.jolla.eventsview.rss")

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.eventsview.rss"));
        qmlRegisterType<RssPostsModel>(uri, 1, 0, "RssPostsModel");
    }
};

#include "plugin.moc"
