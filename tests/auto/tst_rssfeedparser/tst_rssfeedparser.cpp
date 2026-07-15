// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rssfeedparser.h"

#include <QtTest>

class tst_RssFeedParser : public QObject
{
    Q_OBJECT

private slots:
    void parsesRss20();
    void parsesAtom10();
    void extractsImagesFromHtmlContent();
    void rejectsUnsupportedAndMalformedXml();
    void convertsHtmlToPlainText();
    void parsesDates();
    void handlesUndatedEntry();
};

void tst_RssFeedParser::parsesRss20()
{
    const QByteArray data(
        "<?xml version=\"1.0\"?>"
        "<rss version=\"2.0\""
        " xmlns:content=\"http://purl.org/rss/1.0/modules/content/\""
        " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
        " xmlns:media=\"http://search.yahoo.com/mrss/\">"
        "<channel>"
        "<title>Example &amp; News</title>"
        "<link>/home</link>"
        "<image><url>/feed.png</url></image>"
        "<item>"
        "<guid>item-one</guid>"
        "<title>First &amp; best</title>"
        "<link>/posts/1</link>"
        "<description>ignored</description>"
        "<content:encoded><![CDATA[<p>Hello&nbsp;<b>world</b></p>]]></content:encoded>"
        "<dc:creator>Alice</dc:creator>"
        "<pubDate>Tue, 15 Jul 2025 10:30:00 +0200</pubDate>"
        "<enclosure url=\"/photo.jpg\" type=\"image/jpeg\"/>"
        "<media:group><media:thumbnail url=\"/thumb.jpg\"/></media:group>"
        "</item>"
        "</channel>"
        "</rss>");

    RssFeed feed;
    QString error;
    QVERIFY2(RssFeedParser::parse(data, QUrl("https://example.com/feed.xml"),
                                  &feed, &error), qPrintable(error));
    QCOMPARE(feed.title, QStringLiteral("Example & News"));
    QCOMPARE(feed.link, QStringLiteral("https://example.com/home"));
    QCOMPARE(feed.icon, QStringLiteral("https://example.com/feed.png"));
    QCOMPARE(feed.items.count(), 1);

    const RssFeedItem item = feed.items.first();
    QCOMPARE(item.sourceId, QStringLiteral("item-one"));
    QCOMPARE(item.title, QStringLiteral("First & best"));
    QCOMPARE(item.link, QStringLiteral("https://example.com/posts/1"));
    QCOMPARE(item.body, QStringLiteral("Hello world"));
    QCOMPARE(item.author, QStringLiteral("Alice"));
    QVERIFY(item.timestampValid);
    QCOMPARE(item.timestamp, QDateTime::fromString(
                 QStringLiteral("2025-07-15T08:30:00Z"), Qt::ISODate));
    QCOMPARE(item.images, QStringList()
             << QStringLiteral("https://example.com/photo.jpg")
             << QStringLiteral("https://example.com/thumb.jpg"));
}

void tst_RssFeedParser::parsesAtom10()
{
    const QByteArray data(
        "<?xml version=\"1.0\"?>"
        "<feed xmlns=\"http://www.w3.org/2005/Atom\""
        " xmlns:media=\"http://search.yahoo.com/mrss/\">"
        "<title>Example Atom</title>"
        "<link rel=\"alternate\" href=\"/\"/>"
        "<logo>/logo.png</logo>"
        "<author><name>Feed author</name></author>"
        "<entry>"
        "<id>tag:example.com,2025:1</id>"
        "<title type=\"html\">Atom &amp; entry</title>"
        "<link rel=\"alternate\" href=\"/entry/1\"/>"
        "<link rel=\"enclosure\" type=\"image/png\" href=\"/enclosure.png\"/>"
        "<summary type=\"html\">&lt;img src=\"/summary.jpg\" /&gt;</summary>"
        "<content type=\"html\">&lt;p&gt;Atom &lt;b&gt;body&lt;/b&gt;&lt;/p&gt;</content>"
        "<author><name>Bob</name></author>"
        "<published>2025-07-15T10:30:00.123+02:00</published>"
        "<media:content medium=\"image\" url=\"/media.jpg\"/>"
        "</entry>"
        "<entry>"
        "<id>tag:example.com,2025:2</id>"
        "<title>Math</title>"
        "<content type=\"text\">1 &lt; 2</content>"
        "</entry>"
        "</feed>");

    RssFeed feed;
    QString error;
    QVERIFY2(RssFeedParser::parse(data, QUrl("https://example.com/atom.xml"),
                                  &feed, &error), qPrintable(error));
    QCOMPARE(feed.title, QStringLiteral("Example Atom"));
    QCOMPARE(feed.link, QStringLiteral("https://example.com/"));
    QCOMPARE(feed.icon, QStringLiteral("https://example.com/logo.png"));
    QCOMPARE(feed.author, QStringLiteral("Feed author"));
    QCOMPARE(feed.items.count(), 2);

    const RssFeedItem item = feed.items.first();
    QCOMPARE(item.sourceId, QStringLiteral("tag:example.com,2025:1"));
    QCOMPARE(item.title, QStringLiteral("Atom & entry"));
    QCOMPARE(item.body, QStringLiteral("Atom body"));
    QCOMPARE(item.author, QStringLiteral("Bob"));
    QCOMPARE(item.link, QStringLiteral("https://example.com/entry/1"));
    QVERIFY(item.timestampValid);
    QCOMPARE(item.images, QStringList()
             << QStringLiteral("https://example.com/enclosure.png")
             << QStringLiteral("https://example.com/media.jpg")
             << QStringLiteral("https://example.com/summary.jpg"));

    const RssFeedItem inheritedAuthorItem = feed.items.at(1);
    QCOMPARE(inheritedAuthorItem.body, QStringLiteral("1 < 2"));
    QCOMPARE(inheritedAuthorItem.author, QStringLiteral("Feed author"));
}

void tst_RssFeedParser::extractsImagesFromHtmlContent()
{
    const QByteArray data(
        "<rss version=\"2.0\"><channel><title>Comics</title>"
        "<item><title>xkcd</title>"
        "<description>&lt;img src=\"https://imgs.xkcd.com/comics/test.png\" "
        "alt=\"Comic\" /&gt;</description></item>"
        "<item><title>Protocol-relative image</title>"
        "<link>https://example.com/comics/42/</link>"
        "<description><![CDATA[<p>Comic</p><IMG "
        "SRC='preview.jpg?one=1&amp;two=2'>]]></description>"
        "</item></channel></rss>");

    RssFeed feed;
    QString error;
    QVERIFY2(RssFeedParser::parse(data, QUrl("https://example.com/feed.xml"),
                                  &feed, &error), qPrintable(error));
    QCOMPARE(feed.items.count(), 2);
    QCOMPARE(feed.items.at(0).images, QStringList()
             << QStringLiteral("https://imgs.xkcd.com/comics/test.png"));
    QCOMPARE(feed.items.at(1).images, QStringList()
             << QStringLiteral("https://example.com/comics/42/preview.jpg?one=1&two=2"));
}

void tst_RssFeedParser::rejectsUnsupportedAndMalformedXml()
{
    RssFeed feed;
    QString error;

    QVERIFY(!RssFeedParser::parse(
                QByteArray("<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"/>"),
                QUrl("https://example.com/feed"), &feed, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!RssFeedParser::parse(
                QByteArray("<rss version=\"2.0\"><channel><item></channel></rss>"),
                QUrl("https://example.com/feed"), &feed, &error));
    QVERIFY(error.contains(QStringLiteral("Malformed")));

    error.clear();
    QVERIFY(!RssFeedParser::parse(
                QByteArray("<rss version=\"2.0\"><channel/></rss><extra/>"),
                QUrl("https://example.com/feed"), &feed, &error));
    QVERIFY(error.contains(QStringLiteral("Malformed")));
}

void tst_RssFeedParser::convertsHtmlToPlainText()
{
    QCOMPARE(RssFeedParser::toPlainText(
                 QStringLiteral("<p>Hello&nbsp;<b>world</b></p><p>A &amp; B&#33;</p>")),
             QStringLiteral("Hello world\nA & B!"));
    QCOMPARE(RssFeedParser::toPlainText(QStringLiteral("a<br>b<br/>c")),
             QStringLiteral("a\nb\nc"));
}

void tst_RssFeedParser::parsesDates()
{
    bool valid = false;
    QCOMPARE(RssFeedParser::parseDate(
                 QStringLiteral("Tue, 15 Jul 2025 10:30:00 +0200"), &valid),
             QDateTime::fromString(QStringLiteral("2025-07-15T08:30:00Z"),
                                   Qt::ISODate));
    QVERIFY(valid);

    QVERIFY(RssFeedParser::parseDate(
                QStringLiteral("2025-07-15T10:30:00.123+02:00"), &valid).isValid());
    QVERIFY(valid);

    QVERIFY(!RssFeedParser::parseDate(QStringLiteral("not a date"), &valid).isValid());
    QVERIFY(!valid);
}

void tst_RssFeedParser::handlesUndatedEntry()
{
    const QByteArray data(
        "<rss version=\"2.0\"><channel><title>Feed</title>"
        "<item><guid>https://example.com/entry</guid>"
        "<title>No date</title><description>Body</description></item>"
        "<item><guid>empty</guid></item>"
        "</channel></rss>");

    RssFeed feed;
    QVERIFY(RssFeedParser::parse(data, QUrl("https://example.com/feed"), &feed));
    QCOMPARE(feed.items.count(), 1);
    QVERIFY(!feed.items.first().timestampValid);
    QVERIFY(!feed.items.first().timestamp.isValid());
    QCOMPARE(feed.items.first().link, QStringLiteral("https://example.com/entry"));
}

QTEST_MAIN(tst_RssFeedParser)

#include "tst_rssfeedparser.moc"
