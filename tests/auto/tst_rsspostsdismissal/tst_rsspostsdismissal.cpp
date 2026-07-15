// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "rsspostsdatabase.h"
#include "rsspostsmodel.h"
#include "rssdismissalrestorer.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QtTest>

namespace {

const int AccountId = 4242;
const int OtherAccountId = 4343;

typedef QPair<QString, bool> PostState;

QVariantList accountFilter(int accountId = AccountId)
{
    QVariantList filter;
    filter.append(accountId);
    return filter;
}

void addPost(RssPostsDatabase *database, const QString &identifier,
             bool deletedLocally, int accountId = AccountId)
{
    QList<QPair<QString, SocialPostImage::ImageType> > images;
    images.append(qMakePair(QStringLiteral("https://example.com/preview.png"),
                            SocialPostImage::Photo));
    database->addRssPost(
                identifier,
                identifier == QLatin1String("one")
                        ? QStringLiteral("One") : QStringLiteral("Two"),
                QStringLiteral("Body"),
                QDateTime::fromString(QStringLiteral("2026-07-15T12:00:00Z"),
                                      Qt::ISODate),
                QStringLiteral("image://theme/icon-l-rss"),
                images,
                QStringLiteral("https://example.com/") + identifier,
                QStringLiteral("Example feed"),
                QStringLiteral("Author"),
                true,
                deletedLocally,
                accountId);
}

bool replacePosts(RssPostsDatabase *database, const QList<PostState> &posts,
                  int accountId = AccountId)
{
    database->removePosts(accountId);
    for (const PostState &post : posts) {
        addPost(database, post.first, post.second, accountId);
    }
    database->commit();
    database->wait();
    return database->writeStatus() == AbstractSocialCacheDatabase::Finished;
}

bool refresh(RssPostsDatabase *database)
{
    database->refresh();
    database->wait();
    return database->readStatus() == AbstractSocialCacheDatabase::Finished;
}

SocialPost::ConstPtr findPost(RssPostsDatabase *database, const QString &identifier,
                              int accountId = AccountId)
{
    database->setAccountIdFilter(accountFilter(accountId));
    if (!refresh(database)) {
        return SocialPost::ConstPtr();
    }
    const QList<SocialPost::ConstPtr> posts = database->posts();
    for (const SocialPost::ConstPtr &post : posts) {
        if (post && post->identifier() == identifier) {
            return post;
        }
    }
    return SocialPost::ConstPtr();
}

bool postIsDeletedLocally(RssPostsDatabase *database, const QString &identifier,
                          int accountId = AccountId)
{
    return RssPostsDatabase::deletedLocally(findPost(database, identifier, accountId));
}

bool postIsMissing(RssPostsDatabase *database, const QString &identifier,
                   int accountId = AccountId)
{
    return findPost(database, identifier, accountId).isNull();
}

}

class tst_RssPostsDismissal : public QObject
{
    Q_OBJECT

private slots:
    void dismissalFollowsCachedPostLifetime();
    void restoreIsAccountScopedAndNonDestructive();
};

void tst_RssPostsDismissal::dismissalFollowsCachedPostLifetime()
{
    RssPostsDatabase verificationDatabase;
    verificationDatabase.setAccountIdFilter(accountFilter());
    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("one"), false)
                         << qMakePair(QStringLiteral("two"), false)));

    RssPostsModel model;
    model.setAccountIdFilter(accountFilter());
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 2);

    model.remove(QStringLiteral("one"));
    QCOMPARE(model.rowCount(), 1);
    QTRY_VERIFY(postIsDeletedLocally(&verificationDatabase,
                                     QStringLiteral("one")));

    model.refresh();
    QTRY_COMPARE(model.rowCount(), 1);

    {
        RssPostsModel recreatedModel;
        recreatedModel.setAccountIdFilter(accountFilter());
        recreatedModel.refresh();
        QTRY_COMPARE(recreatedModel.rowCount(), 1);
    }

    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("one"), true)
                         << qMakePair(QStringLiteral("two"), false)));
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 1);

    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("two"), false)));
    QVERIFY(postIsMissing(&verificationDatabase, QStringLiteral("one")));

    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("one"), false)
                         << qMakePair(QStringLiteral("two"), false)));
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 2);

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QTRY_VERIFY(postIsDeletedLocally(&verificationDatabase,
                                     QStringLiteral("one")));
    QTRY_VERIFY(postIsDeletedLocally(&verificationDatabase,
                                     QStringLiteral("two")));
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 0);

    RssDismissalRestorer restorer;
    QSignalSpy restoreFinishedSpy(&restorer,
                                  &RssDismissalRestorer::restoreFinished);
    QSignalSpy restoreFailedSpy(&restorer,
                                &RssDismissalRestorer::restoreFailed);
    QVERIFY(restorer.restore(AccountId));
    QTRY_COMPARE(restoreFinishedSpy.count(), 1);
    QCOMPARE(restoreFailedSpy.count(), 0);
    QCOMPARE(restoreFinishedSpy.at(0).at(0).toInt(), 2);
    QTRY_VERIFY(!postIsDeletedLocally(&verificationDatabase,
                                      QStringLiteral("one")));
    QTRY_VERIFY(!postIsDeletedLocally(&verificationDatabase,
                                      QStringLiteral("two")));
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 2);

    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("two"), true)));
    QVERIFY(postIsMissing(&verificationDatabase, QStringLiteral("one")));
    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("one"), false)
                         << qMakePair(QStringLiteral("two"), true)));
    model.refresh();
    QTRY_COMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), RssPostsModel::RssId).toString(),
             QStringLiteral("one"));
}

void tst_RssPostsDismissal::restoreIsAccountScopedAndNonDestructive()
{
    RssPostsDatabase verificationDatabase;
    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("account-one-dismissed"), true)
                         << qMakePair(QStringLiteral("account-one-visible"), false),
                         AccountId));
    QVERIFY(replacePosts(&verificationDatabase,
                         QList<PostState>()
                         << qMakePair(QStringLiteral("account-two-dismissed"), true),
                         OtherAccountId));

    const SocialPost::ConstPtr before = findPost(
                &verificationDatabase, QStringLiteral("account-one-dismissed"), AccountId);
    QVERIFY(before);
    const QString name = before->name();
    const QString body = before->body();
    const QDateTime timestamp = before->timestamp();
    const QString icon = before->icon();
    const QString url = RssPostsDatabase::url(before);
    const QString feedTitle = RssPostsDatabase::feedTitle(before);
    const QString author = RssPostsDatabase::author(before);
    const QList<int> accounts = before->accounts();
    QVariantMap extra = before->extra();
    extra.remove(QStringLiteral("deleted_locally"));
    const QList<SocialPostImage::ConstPtr> images = before->images();
    QCOMPARE(images.count(), 1);
    const QString imageUrl = images.first()->url();
    const SocialPostImage::ImageType imageType = images.first()->type();

    RssDismissalRestorer restorer;
    QSignalSpy restoreFinishedSpy(&restorer,
                                  &RssDismissalRestorer::restoreFinished);
    QSignalSpy restoreFailedSpy(&restorer,
                                &RssDismissalRestorer::restoreFailed);

    QVERIFY(!restorer.restore(0));
    QVERIFY(!restorer.busy());
    QVERIFY(restorer.restore(AccountId));
    QVERIFY(restorer.busy());
    QVERIFY(!restorer.restore(OtherAccountId));
    QTRY_COMPARE(restoreFinishedSpy.count(), 1);
    QVERIFY(!restorer.busy());
    QCOMPARE(restoreFailedSpy.count(), 0);
    QCOMPARE(restoreFinishedSpy.at(0).at(0).toInt(), 1);

    QVERIFY(!postIsDeletedLocally(&verificationDatabase,
                                  QStringLiteral("account-one-dismissed"), AccountId));
    QVERIFY(!postIsDeletedLocally(&verificationDatabase,
                                  QStringLiteral("account-one-visible"), AccountId));
    QVERIFY(postIsDeletedLocally(&verificationDatabase,
                                 QStringLiteral("account-two-dismissed"), OtherAccountId));

    const SocialPost::ConstPtr after = findPost(
                &verificationDatabase, QStringLiteral("account-one-dismissed"), AccountId);
    QVERIFY(after);
    QCOMPARE(after->name(), name);
    QCOMPARE(after->body(), body);
    QCOMPARE(after->timestamp(), timestamp);
    QCOMPARE(after->icon(), icon);
    QCOMPARE(RssPostsDatabase::url(after), url);
    QCOMPARE(RssPostsDatabase::feedTitle(after), feedTitle);
    QCOMPARE(RssPostsDatabase::author(after), author);
    QCOMPARE(after->accounts(), accounts);
    QVariantMap restoredExtra = after->extra();
    restoredExtra.remove(QStringLiteral("deleted_locally"));
    QCOMPARE(restoredExtra, extra);
    QCOMPARE(after->images().count(), 1);
    QCOMPARE(after->images().first()->url(), imageUrl);
    QCOMPARE(after->images().first()->type(), imageType);

    QVERIFY(restorer.restore(AccountId));
    QTRY_COMPARE(restoreFinishedSpy.count(), 2);
    QCOMPARE(restoreFinishedSpy.at(1).at(0).toInt(), 0);

    verificationDatabase.removePosts(AccountId);
    verificationDatabase.commit();
    verificationDatabase.wait();
    QCOMPARE(verificationDatabase.writeStatus(),
             AbstractSocialCacheDatabase::Finished);
    QVERIFY(restorer.restore(AccountId));
    QTRY_COMPARE(restoreFinishedSpy.count(), 3);
    QCOMPARE(restoreFinishedSpy.at(2).at(0).toInt(), 0);
    QVERIFY(postIsMissing(&verificationDatabase,
                          QStringLiteral("account-one-dismissed"), AccountId));
    QVERIFY(postIsDeletedLocally(&verificationDatabase,
                                 QStringLiteral("account-two-dismissed"), OtherAccountId));
    QCOMPARE(restoreFailedSpy.count(), 0);
}

int main(int argc, char **argv)
{
    QTemporaryDir dataDirectory;
    if (!dataDirectory.isValid()) {
        return 1;
    }
    qputenv("XDG_DATA_HOME", dataDirectory.path().toUtf8());

    QCoreApplication application(argc, argv);
    tst_RssPostsDismissal test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_rsspostsdismissal.moc"
