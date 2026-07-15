/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rssdismissalrestorer.h"

#include <QDebug>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>

namespace {

const char DeletedLocallyKey[] = "deleted_locally";

}

RssDismissalRestorer::RssDismissalRestorer(QObject *parent)
    : m_accountId(0)
    , m_restoredCount(-1)
{
    setParent(parent);
}

bool RssDismissalRestorer::busy() const
{
    QMutexLocker locker(&m_mutex);
    return m_accountId > 0;
}

bool RssDismissalRestorer::restore(int accountId)
{
    if (accountId <= 0) {
        return false;
    }

    {
        QMutexLocker locker(&m_mutex);
        if (m_accountId > 0) {
            return false;
        }
        m_accountId = accountId;
        m_restoredCount = -1;
    }

    emit busyChanged();
    commit();
    return true;
}

bool RssDismissalRestorer::write()
{
    if (!AbstractSocialPostCacheDatabase::write()) {
        return false;
    }

    int accountId = 0;
    {
        QMutexLocker locker(&m_mutex);
        accountId = m_accountId;
    }
    if (accountId <= 0) {
        return true;
    }

    // RSS post identifiers include the account identifier, so each post row
    // belongs to exactly one RSS account.
    QSqlQuery query = prepare(QStringLiteral(
                "UPDATE extra "
                "SET value = 0 "
                "WHERE key = :key "
                "AND CAST(value AS INTEGER) != 0 "
                "AND postId IN ("
                "SELECT postId FROM link_post_account WHERE account = :account)"));
    query.bindValue(QStringLiteral(":key"), QLatin1String(DeletedLocallyKey));
    query.bindValue(QStringLiteral(":account"), accountId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to restore dismissed RSS posts"
                   << query.lastError();
        return false;
    }

    const int restoredCount = qMax(0, query.numRowsAffected());
    query.finish();

    QMutexLocker locker(&m_mutex);
    m_restoredCount = restoredCount;
    return true;
}

void RssDismissalRestorer::writeFinished()
{
    AbstractSocialCacheDatabase::writeFinished();

    int restoredCount = -1;
    {
        QMutexLocker locker(&m_mutex);
        if (m_accountId <= 0) {
            return;
        }
        restoredCount = m_restoredCount;
        m_accountId = 0;
        m_restoredCount = -1;
    }

    const bool success = writeStatus() == AbstractSocialCacheDatabase::Finished
            && restoredCount >= 0;
    emit busyChanged();
    if (success) {
        emit restoreFinished(restoredCount);
    } else {
        emit restoreFailed();
    }
}
