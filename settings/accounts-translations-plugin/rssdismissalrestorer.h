/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RSSDISMISSALRESTORER_H
#define RSSDISMISSALRESTORER_H

#include "rsspostsdatabase.h"

#include <QMutex>

class RssDismissalRestorer : public RssPostsDatabase
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit RssDismissalRestorer(QObject *parent = nullptr);

    bool busy() const;
    Q_INVOKABLE bool restore(int accountId);

signals:
    void busyChanged();
    void restoreFinished(int restoredCount);
    void restoreFailed();

protected:
    bool write() override;
    void writeFinished() override;

private:
    mutable QMutex m_mutex;
    int m_accountId;
    int m_restoredCount;
};

#endif // RSSDISMISSALRESTORER_H
