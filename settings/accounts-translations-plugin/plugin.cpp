// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rssfeedparser.h"
#include "rssdismissalrestorer.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QTranslator>
#include <QUrl>
#include <QVariantMap>
#include <QtQml>

class RssFeedValidator : public QObject
{
    Q_OBJECT

public:
    explicit RssFeedValidator(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    Q_INVOKABLE QVariantMap validate(const QString &data, const QUrl &baseUrl) const
    {
        const QByteArray bytes = data.toUtf8();
        const bool tooLarge = bytes.size() > 5 * 1024 * 1024;
        RssFeed feed;
        QString errorMessage;
        const bool valid = !tooLarge
                && RssFeedParser::parse(bytes, baseUrl, &feed, &errorMessage);

        QVariantMap result;
        result.insert(QStringLiteral("valid"), valid);
        result.insert(QStringLiteral("tooLarge"), tooLarge);
        result.insert(QStringLiteral("title"), feed.title);
        result.insert(QStringLiteral("error"), errorMessage);
        return result;
    }
};

class RssTranslator : public QTranslator
{
    Q_OBJECT

public:
    explicit RssTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    ~RssTranslator() override
    {
        qApp->removeTranslator(this);
    }
};

class RssAccountsTranslationsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.jolla.settings.accounts.rss")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri)

        RssTranslator *engineeringEnglish = new RssTranslator(engine);
        engineeringEnglish->load(QStringLiteral("sailfish-account-rss_eng_en"),
                                 QStringLiteral("/usr/share/translations"));

        RssTranslator *translator = new RssTranslator(engine);
        translator->load(QLocale(), QStringLiteral("sailfish-account-rss"),
                         QStringLiteral("-"), QStringLiteral("/usr/share/translations"));
    }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("com.jolla.settings.accounts.rss"));
        qmlRegisterType<RssFeedValidator>(uri, 1, 0, "RssFeedValidator");
        qmlRegisterType<RssDismissalRestorer>(uri, 1, 0, "RssDismissalRestorer");
        qmlRegisterUncreatableType<RssAccountsTranslationsPlugin>(
                    uri, 1, 0, "RssTranslationPlugin", QString());
    }
};

#include "plugin.moc"
