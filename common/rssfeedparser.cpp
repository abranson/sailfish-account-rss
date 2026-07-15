// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rssfeedparser.h"

#include <QRegExp>
#include <QSet>
#include <QXmlStreamReader>

namespace {

const int MaxImagesPerItem = 4;

QString decodeEntities(const QString &input);

QString elementText(QXmlStreamReader *xml)
{
    return xml->readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
}

QString resolvedUrl(const QUrl &baseUrl, const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QUrl url = baseUrl.resolved(QUrl(trimmed));
    if (!url.isValid()) {
        return QString();
    }
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https")
            ? url.toString() : QString();
}

bool isMediaRssElement(const QXmlStreamReader &xml)
{
    const QString namespaceUri = xml.namespaceUri().toString();
    return namespaceUri.contains(QLatin1String("search.yahoo.com/mrss"))
            || xml.prefix().compare(QLatin1String("media"), Qt::CaseInsensitive) == 0;
}

void appendImage(QStringList *images, const QString &url)
{
    if (!images || url.isEmpty() || images->contains(url)
            || images->count() >= MaxImagesPerItem) {
        return;
    }
    images->append(url);
}

void appendHtmlImages(const QString &html, const QUrl &baseUrl,
                      QStringList *images)
{
    QRegExp imageExpression(QStringLiteral("<\\s*img\\b[^>]*>"),
                            Qt::CaseInsensitive);
    QRegExp sourceExpression(
                QStringLiteral("\\s+src\\s*=\\s*(\"([^\"]*)\"|'([^']*)'|([^\\s>]+))"),
                Qt::CaseInsensitive);

    int position = 0;
    while (images && images->count() < MaxImagesPerItem
           && (position = imageExpression.indexIn(html, position)) >= 0) {
        const QString tag = imageExpression.cap(0);
        if (sourceExpression.indexIn(tag) >= 0) {
            QString source = sourceExpression.cap(2);
            if (source.isEmpty()) {
                source = sourceExpression.cap(3);
            }
            if (source.isEmpty()) {
                source = sourceExpression.cap(4);
            }
            appendImage(images, resolvedUrl(baseUrl, decodeEntities(source)));
        }
        position += qMax(1, imageExpression.matchedLength());
    }
}

void appendMediaImage(QXmlStreamReader *xml, const QUrl &baseUrl, QStringList *images)
{
    const QXmlStreamAttributes attributes = xml->attributes();
    const QString type = attributes.value(QLatin1String("type")).toString().toLower();
    const QString medium = attributes.value(QLatin1String("medium")).toString().toLower();
    if (xml->name().compare(QLatin1String("thumbnail"), Qt::CaseInsensitive) == 0
            || type.startsWith(QLatin1String("image/"))
            || medium == QLatin1String("image")
            || (type.isEmpty() && medium.isEmpty())) {
        appendImage(images, resolvedUrl(baseUrl,
                                        attributes.value(QLatin1String("url")).toString()));
    }
    xml->skipCurrentElement();
}

void appendMediaImages(QXmlStreamReader *xml, const QUrl &baseUrl,
                       QStringList *images)
{
    const QString name = xml->name().toString().toLower();
    if (name == QLatin1String("content") || name == QLatin1String("thumbnail")) {
        appendMediaImage(xml, baseUrl, images);
        return;
    }

    while (xml->readNextStartElement()) {
        if (isMediaRssElement(*xml)) {
            appendMediaImages(xml, baseUrl, images);
        } else {
            xml->skipCurrentElement();
        }
    }
}

QString parseRssImage(QXmlStreamReader *xml, const QUrl &baseUrl)
{
    QString image;
    while (xml->readNextStartElement()) {
        if (xml->name().compare(QLatin1String("url"), Qt::CaseInsensitive) == 0) {
            image = resolvedUrl(baseUrl, elementText(xml));
        } else {
            xml->skipCurrentElement();
        }
    }
    return image;
}

QString parseAtomAuthor(QXmlStreamReader *xml)
{
    QString author;
    while (xml->readNextStartElement()) {
        if (xml->name() == QLatin1String("name")) {
            author = RssFeedParser::toPlainText(elementText(xml));
        } else {
            xml->skipCurrentElement();
        }
    }
    return author;
}

RssFeedItem parseRssItem(QXmlStreamReader *xml, const QUrl &baseUrl, int sourceOrder)
{
    RssFeedItem item;
    item.sourceOrder = sourceOrder;
    QString description;
    QString encodedContent;
    QString dateText;

    while (xml->readNextStartElement()) {
        const QString name = xml->name().toString().toLower();
        if (isMediaRssElement(*xml)
                && (name == QLatin1String("content")
                    || name == QLatin1String("thumbnail")
                    || name == QLatin1String("group"))) {
            appendMediaImages(xml, baseUrl, &item.images);
        } else if (name == QLatin1String("title")) {
            item.title = RssFeedParser::toPlainText(elementText(xml));
        } else if (name == QLatin1String("link")) {
            item.link = resolvedUrl(baseUrl, elementText(xml));
        } else if (name == QLatin1String("guid")) {
            const QString isPermalink = xml->attributes().value(
                        QLatin1String("isPermaLink")).toString();
            item.sourceId = elementText(xml);
            if (item.link.isEmpty()
                    && isPermalink.compare(QLatin1String("false"),
                                           Qt::CaseInsensitive) != 0) {
                const QUrl guidUrl(item.sourceId);
                const QString scheme = guidUrl.scheme().toLower();
                if (guidUrl.isValid()
                        && (scheme == QLatin1String("http")
                            || scheme == QLatin1String("https"))) {
                    item.link = guidUrl.toString();
                }
            }
        } else if (name == QLatin1String("description")
                   || name == QLatin1String("summary")) {
            description = elementText(xml);
        } else if (name == QLatin1String("encoded")) {
            encodedContent = elementText(xml);
        } else if (name == QLatin1String("author")
                   || name == QLatin1String("creator")) {
            item.author = RssFeedParser::toPlainText(elementText(xml));
        } else if (name == QLatin1String("pubdate")
                   || name == QLatin1String("date")) {
            dateText = elementText(xml);
        } else if (name == QLatin1String("enclosure")) {
            const QXmlStreamAttributes attributes = xml->attributes();
            const QString type = attributes.value(QLatin1String("type")).toString().toLower();
            if (type.startsWith(QLatin1String("image/"))) {
                appendImage(&item.images, resolvedUrl(
                                baseUrl,
                                attributes.value(QLatin1String("url")).toString()));
            }
            xml->skipCurrentElement();
        } else {
            xml->skipCurrentElement();
        }
    }

    const QUrl contentBaseUrl = item.link.isEmpty() ? baseUrl : QUrl(item.link);
    appendHtmlImages(description, contentBaseUrl, &item.images);
    appendHtmlImages(encodedContent, contentBaseUrl, &item.images);
    item.body = RssFeedParser::toPlainText(
                encodedContent.isEmpty() ? description : encodedContent);
    item.timestamp = RssFeedParser::parseDate(dateText, &item.timestampValid);
    if (item.sourceId.isEmpty()) {
        item.sourceId = item.link;
    }
    return item;
}

bool parseRss(QXmlStreamReader *xml, const QUrl &baseUrl, RssFeed *feed)
{
    const QString version = xml->attributes().value(QLatin1String("version")).toString();
    if (!version.isEmpty() && !version.startsWith(QLatin1String("2."))) {
        return false;
    }

    bool channelFound = false;
    while (xml->readNextStartElement()) {
        if (xml->name().compare(QLatin1String("channel"), Qt::CaseInsensitive) != 0) {
            xml->skipCurrentElement();
            continue;
        }

        channelFound = true;
        int sourceOrder = 0;
        while (xml->readNextStartElement()) {
            const QString name = xml->name().toString().toLower();
            if (name == QLatin1String("item")) {
                RssFeedItem item = parseRssItem(xml, baseUrl, sourceOrder++);
                if (!item.title.isEmpty() || !item.body.isEmpty()) {
                    feed->items.append(item);
                }
            } else if (name == QLatin1String("title") && feed->title.isEmpty()) {
                feed->title = RssFeedParser::toPlainText(elementText(xml));
            } else if (name == QLatin1String("link") && feed->link.isEmpty()) {
                feed->link = resolvedUrl(baseUrl, elementText(xml));
            } else if (name == QLatin1String("image") && feed->icon.isEmpty()) {
                feed->icon = parseRssImage(xml, baseUrl);
            } else if (isMediaRssElement(*xml)
                       && (name == QLatin1String("thumbnail")
                           || name == QLatin1String("content")
                           || name == QLatin1String("group"))
                       && feed->icon.isEmpty()) {
                QStringList images;
                appendMediaImages(xml, baseUrl, &images);
                if (!images.isEmpty()) {
                    feed->icon = images.first();
                }
            } else {
                xml->skipCurrentElement();
            }
        }
    }
    return channelFound;
}

RssFeedItem parseAtomEntry(QXmlStreamReader *xml, const QUrl &baseUrl, int sourceOrder)
{
    RssFeedItem item;
    item.sourceOrder = sourceOrder;
    QString summary;
    QString content;
    QString published;
    QString updated;

    while (xml->readNextStartElement()) {
        const QString name = xml->name().toString().toLower();
        if (isMediaRssElement(*xml)
                && (name == QLatin1String("content")
                    || name == QLatin1String("thumbnail")
                    || name == QLatin1String("group"))) {
            appendMediaImages(xml, baseUrl, &item.images);
        } else if (name == QLatin1String("id")) {
            item.sourceId = elementText(xml);
        } else if (name == QLatin1String("title")) {
            item.title = RssFeedParser::toPlainText(elementText(xml));
        } else if (name == QLatin1String("summary")) {
            summary = elementText(xml);
        } else if (name == QLatin1String("content")) {
            content = elementText(xml);
        } else if (name == QLatin1String("author")) {
            item.author = parseAtomAuthor(xml);
        } else if (name == QLatin1String("published")) {
            published = elementText(xml);
        } else if (name == QLatin1String("updated")) {
            updated = elementText(xml);
        } else if (name == QLatin1String("link")) {
            const QXmlStreamAttributes attributes = xml->attributes();
            const QString rel = attributes.value(QLatin1String("rel")).toString().toLower();
            const QString type = attributes.value(QLatin1String("type")).toString().toLower();
            const QString href = resolvedUrl(
                        baseUrl, attributes.value(QLatin1String("href")).toString());
            if (rel == QLatin1String("enclosure") && type.startsWith(QLatin1String("image/"))) {
                appendImage(&item.images, href);
            } else if ((rel.isEmpty() || rel == QLatin1String("alternate"))
                       && item.link.isEmpty()) {
                item.link = href;
            }
            xml->skipCurrentElement();
        } else {
            xml->skipCurrentElement();
        }
    }

    const QUrl contentBaseUrl = item.link.isEmpty() ? baseUrl : QUrl(item.link);
    appendHtmlImages(summary, contentBaseUrl, &item.images);
    appendHtmlImages(content, contentBaseUrl, &item.images);
    item.body = RssFeedParser::toPlainText(content.isEmpty() ? summary : content);
    item.timestamp = RssFeedParser::parseDate(
                published.isEmpty() ? updated : published, &item.timestampValid);
    if (item.sourceId.isEmpty()) {
        item.sourceId = item.link;
    }
    return item;
}

bool parseAtom(QXmlStreamReader *xml, const QUrl &baseUrl, RssFeed *feed)
{
    if (xml->namespaceUri() != QLatin1String("http://www.w3.org/2005/Atom")) {
        return false;
    }

    int sourceOrder = 0;
    while (xml->readNextStartElement()) {
        const QString name = xml->name().toString().toLower();
        if (name == QLatin1String("entry")) {
            RssFeedItem item = parseAtomEntry(xml, baseUrl, sourceOrder++);
            if (!item.title.isEmpty() || !item.body.isEmpty()) {
                feed->items.append(item);
            }
        } else if (name == QLatin1String("title") && feed->title.isEmpty()) {
            feed->title = RssFeedParser::toPlainText(elementText(xml));
        } else if ((name == QLatin1String("icon") || name == QLatin1String("logo"))
                   && feed->icon.isEmpty()) {
            feed->icon = resolvedUrl(baseUrl, elementText(xml));
        } else if (name == QLatin1String("author") && feed->author.isEmpty()) {
            feed->author = parseAtomAuthor(xml);
        } else if (name == QLatin1String("link") && feed->link.isEmpty()) {
            const QXmlStreamAttributes attributes = xml->attributes();
            const QString rel = attributes.value(QLatin1String("rel")).toString().toLower();
            if (rel.isEmpty() || rel == QLatin1String("alternate")) {
                feed->link = resolvedUrl(
                            baseUrl, attributes.value(QLatin1String("href")).toString());
            }
            xml->skipCurrentElement();
        } else {
            xml->skipCurrentElement();
        }
    }

    if (!feed->author.isEmpty()) {
        for (RssFeedItem &item : feed->items) {
            if (item.author.isEmpty()) {
                item.author = feed->author;
            }
        }
    }
    return true;
}

QString decodeEntity(const QString &entity)
{
    if (entity == QLatin1String("amp")) return QStringLiteral("&");
    if (entity == QLatin1String("lt")) return QStringLiteral("<");
    if (entity == QLatin1String("gt")) return QStringLiteral(">");
    if (entity == QLatin1String("quot")) return QStringLiteral("\"");
    if (entity == QLatin1String("apos") || entity == QLatin1String("#39")) {
        return QStringLiteral("'");
    }
    if (entity == QLatin1String("nbsp")) return QStringLiteral(" ");
    if (entity == QLatin1String("hellip")) return QString::fromUtf8("\342\200\246");
    if (entity == QLatin1String("ndash")) return QString::fromUtf8("\342\200\223");
    if (entity == QLatin1String("mdash")) return QString::fromUtf8("\342\200\224");
    if (entity == QLatin1String("copy")) return QString::fromUtf8("\302\251");
    if (entity == QLatin1String("reg")) return QString::fromUtf8("\302\256");

    bool ok = false;
    uint codepoint = 0;
    if (entity.startsWith(QLatin1String("#x"), Qt::CaseInsensitive)) {
        codepoint = entity.mid(2).toUInt(&ok, 16);
    } else if (entity.startsWith(QLatin1Char('#'))) {
        codepoint = entity.mid(1).toUInt(&ok, 10);
    }
    if (ok && codepoint > 0 && codepoint <= 0x10ffff) {
        return QString::fromUcs4(&codepoint, 1);
    }
    return QStringLiteral("&") + entity + QStringLiteral(";");
}

QString decodeEntities(const QString &input)
{
    QString result;
    result.reserve(input.size());
    int position = 0;
    while (position < input.size()) {
        const int amp = input.indexOf(QLatin1Char('&'), position);
        if (amp < 0) {
            result.append(input.mid(position));
            break;
        }
        result.append(input.mid(position, amp - position));
        const int semicolon = input.indexOf(QLatin1Char(';'), amp + 1);
        if (semicolon < 0 || semicolon - amp > 16) {
            result.append(QLatin1Char('&'));
            position = amp + 1;
            continue;
        }
        result.append(decodeEntity(input.mid(amp + 1, semicolon - amp - 1)));
        position = semicolon + 1;
    }
    return result;
}

} // namespace

bool RssFeedParser::parse(const QByteArray &data, const QUrl &baseUrl,
                          RssFeed *feed, QString *errorMessage)
{
    if (!feed) {
        return false;
    }
    *feed = RssFeed();

    QXmlStreamReader xml(data);
    if (!xml.readNextStartElement()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The response does not contain an XML document.");
        }
        return false;
    }

    bool supported = false;
    if (xml.name().compare(QLatin1String("rss"), Qt::CaseInsensitive) == 0) {
        supported = parseRss(&xml, baseUrl, feed);
    } else if (xml.name() == QLatin1String("feed")) {
        supported = parseAtom(&xml, baseUrl, feed);
    }

    while (!xml.atEnd()) {
        xml.readNext();
    }

    if (!supported || xml.hasError()) {
        if (errorMessage) {
            *errorMessage = xml.hasError()
                    ? QStringLiteral("Malformed feed XML at line %1: %2")
                      .arg(xml.lineNumber()).arg(xml.errorString())
                    : QStringLiteral("The response is not an RSS 2.0 or Atom 1.0 feed.");
        }
        *feed = RssFeed();
        return false;
    }
    return true;
}

QString RssFeedParser::toPlainText(const QString &input)
{
    QString text = input;
    text.remove(QRegExp(QStringLiteral("<\\s*(script|style)\\b[^>]*>.*</\\s*\\1\\s*>"),
                        Qt::CaseInsensitive));
    text.replace(QRegExp(QStringLiteral("<\\s*br\\s*/?\\s*>"), Qt::CaseInsensitive),
                 QStringLiteral("\n"));
    text.replace(QRegExp(QStringLiteral("</\\s*(p|div|li|h[1-6])\\s*>"),
                         Qt::CaseInsensitive),
                 QStringLiteral("\n"));

    QString withoutTags;
    withoutTags.reserve(text.size());
    bool inTag = false;
    for (int i = 0; i < text.size(); ++i) {
        const QChar character = text.at(i);
        if (character == QLatin1Char('<')) {
            const QChar next = i + 1 < text.size() ? text.at(i + 1) : QChar();
            const QChar afterSlash = i + 2 < text.size() ? text.at(i + 2) : QChar();
            const bool plausibleTag = next.isLetter()
                    || next == QLatin1Char('!')
                    || next == QLatin1Char('?')
                    || (next == QLatin1Char('/') && afterSlash.isLetter());
            if (plausibleTag && text.indexOf(QLatin1Char('>'), i + 1) >= 0) {
                inTag = true;
            } else {
                withoutTags.append(character);
            }
        } else if (character == QLatin1Char('>') && inTag) {
            inTag = false;
        } else if (!inTag) {
            withoutTags.append(character);
        }
    }

    text = decodeEntities(withoutTags);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    text.replace(QRegExp(QStringLiteral("[\\t\\f\\v ]+")), QStringLiteral(" "));
    text.replace(QRegExp(QStringLiteral(" *\\n *")), QStringLiteral("\n"));
    text.replace(QRegExp(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return text.trimmed();
}

QDateTime RssFeedParser::parseDate(const QString &input, bool *valid)
{
    const QString value = input.trimmed();
    QDateTime dateTime = QDateTime::fromString(value, Qt::RFC2822Date);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(value, Qt::ISODate);
    }
    if (!dateTime.isValid()) {
        QString withoutFraction = value;
        withoutFraction.remove(QRegExp(QStringLiteral("\\.\\d+(?=Z|[+-]\\d\\d:?\\d\\d$)")));
        dateTime = QDateTime::fromString(withoutFraction, Qt::ISODate);
    }
    if (valid) {
        *valid = dateTime.isValid();
    }
    return dateTime.isValid() ? dateTime.toUTC() : QDateTime();
}
