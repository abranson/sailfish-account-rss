// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rssfeedparser.h"
#include "rssdismissalrestorer.h"

#include <QCoreApplication>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QVariantMap>
#include <QtQml>

namespace {

const int MaximumResponseSize = 5 * 1024 * 1024;
const int RequestTimeout = 60 * 1000;
const int MaximumRedirects = 5;

bool secureHttpUrl(const QUrl &url)
{
    return url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0;
}

int effectivePort(const QUrl &url)
{
    if (url.port() >= 0) {
        return url.port();
    }
    return secureHttpUrl(url) ? 443 : 80;
}

bool sameOrigin(const QUrl &left, const QUrl &right)
{
    return left.scheme().compare(right.scheme(), Qt::CaseInsensitive) == 0
            && left.host().compare(right.host(), Qt::CaseInsensitive) == 0
            && effectivePort(left) == effectivePort(right);
}

}

class RssFeedValidator : public QObject
{
    Q_OBJECT

public:
    explicit RssFeedValidator(QObject *parent = nullptr)
        : QObject(parent)
        , m_networkManager(new QNetworkAccessManager(this))
        , m_timeoutTimer(new QTimer(this))
    {
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout,
                this, &RssFeedValidator::requestTimedOut);
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

    Q_INVOKABLE int fetch(const QUrl &url, const QString &username,
                          const QString &password, bool requiresAuthentication)
    {
        cancelFetch(m_requestId);

        ++m_requestId;
        m_requiresAuthentication = requiresAuthentication;
        m_authenticatedOrigin = url;
        m_redirectCount = 0;
        m_responseData.clear();

        const int requestId = m_requestId;
        if (!url.isValid()
                || (url.scheme().compare(QLatin1String("http"), Qt::CaseInsensitive) != 0
                    && !secureHttpUrl(url))
                || (requiresAuthentication
                    && (!secureHttpUrl(url) || username.isEmpty() || password.isEmpty()))) {
            QTimer::singleShot(0, this, [this, requestId]() {
                if (requestId == m_requestId && !m_reply) {
                    finishFetch(QString(), QUrl(), false, false, false);
                }
            });
            return requestId;
        }

        if (requiresAuthentication) {
            m_authorization = QByteArrayLiteral("Basic ")
                    + (username + QLatin1Char(':') + password).toUtf8().toBase64();
        }
        startRequest(url);
        return requestId;
    }

    Q_INVOKABLE void cancelFetch(int requestId)
    {
        if (requestId <= 0 || requestId != m_requestId) {
            return;
        }

        m_timeoutTimer->stop();
        if (m_reply) {
            QNetworkReply *reply = m_reply;
            m_reply = nullptr;
            reply->abort();
            reply->deleteLater();
        }
        m_responseData.clear();
        m_authorization.clear();
        m_requiresAuthentication = false;
    }

Q_SIGNALS:
    void fetchFinished(int requestId, const QString &data, const QUrl &responseUrl,
                       bool tooLarge, bool timedOut, bool success);

private:
    void startRequest(const QUrl &url)
    {
        QNetworkRequest request(url);
        // Follow redirects only for unauthenticated requests.  An Authorization
        // header must never be sent to another origin.
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute,
                             !m_requiresAuthentication);
        request.setRawHeader("Accept",
                             "application/rss+xml, application/atom+xml, "
                             "application/xml, text/xml;q=0.9, */*;q=0.1");
        request.setRawHeader("User-Agent", "Sailfish-RSS/1.0");
        if (!m_authorization.isEmpty()) {
            request.setRawHeader("Authorization", m_authorization);
        }

        m_reply = m_networkManager->get(request);
        QNetworkReply *reply = m_reply;
        connect(reply, &QIODevice::readyRead, this, [this, reply]() {
            replyReadyRead(reply);
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            replyFinished(reply);
        });
        m_timeoutTimer->start(RequestTimeout);
    }

    void replyReadyRead(QNetworkReply *reply)
    {
        if (reply != m_reply) {
            return;
        }

        m_responseData.append(reply->readAll());
        if (m_responseData.size() > MaximumResponseSize) {
            m_reply = nullptr;
            reply->abort();
            reply->deleteLater();
            finishFetch(QString(), reply->url(), true, false, false);
        }
    }

    void replyFinished(QNetworkReply *reply)
    {
        if (reply != m_reply) {
            return;
        }

        m_reply = nullptr;
        m_timeoutTimer->stop();

        const int httpStatus = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QUrl redirectTarget = reply->attribute(
                    QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (m_requiresAuthentication && httpStatus >= 300 && httpStatus < 400
                && !redirectTarget.isEmpty() && redirectTarget.isValid()) {
            const QUrl redirectUrl = reply->url().resolved(redirectTarget);
            if (!secureHttpUrl(redirectUrl)
                    || !sameOrigin(m_authenticatedOrigin, redirectUrl)
                    || m_redirectCount >= MaximumRedirects) {
                reply->deleteLater();
                finishFetch(QString(), reply->url(), false, false, false);
                return;
            }

            ++m_redirectCount;
            m_responseData.clear();
            reply->deleteLater();
            startRequest(redirectUrl);
            return;
        }

        m_responseData.append(reply->readAll());
        const bool tooLarge = m_responseData.size() > MaximumResponseSize;
        const bool success = !tooLarge
                && reply->error() == QNetworkReply::NoError
                && httpStatus >= 200 && httpStatus < 300;
        const QString data = success ? QString::fromUtf8(m_responseData) : QString();
        const QUrl responseUrl = reply->url();
        reply->deleteLater();
        finishFetch(data, responseUrl, tooLarge, false, success);
    }

    void requestTimedOut()
    {
        if (!m_reply) {
            return;
        }

        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
        finishFetch(QString(), reply->url(), false, true, false);
    }

    void finishFetch(const QString &data, const QUrl &responseUrl,
                     bool tooLarge, bool timedOut, bool success)
    {
        m_timeoutTimer->stop();
        m_responseData.clear();
        m_authorization.clear();
        m_requiresAuthentication = false;
        emit fetchFinished(m_requestId, data, responseUrl, tooLarge, timedOut, success);
    }

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QByteArray m_responseData;
    QByteArray m_authorization;
    QUrl m_authenticatedOrigin;
    int m_requestId = 0;
    int m_redirectCount = 0;
    bool m_requiresAuthentication = false;
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
