// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.6
import Sailfish.Silica 1.0
import "shared"

SocialMediaFeedItem {
    id: item

    property variant imageList
    property bool timestampValid: model.timestampValid

    timestamp: model.timestamp
    topMargin: Theme.paddingLarge
    bottomMargin: Theme.paddingLarge
    userRemovable: true
    contentHeight: Math.max(content.y + content.height,
                            avatar.y + avatar.height) + bottomMargin

    onRefreshTimeCountChanged: {
        formattedTime = timestampValid
                ? Format.formatDate(model.timestamp, Format.TimeElapsed) : ""
    }

    Component.onCompleted: {
        formattedTime = timestampValid
                ? Format.formatDate(model.timestamp, Format.TimeElapsed) : ""
    }

    Column {
        id: content

        anchors {
            left: avatar.right
            leftMargin: Theme.paddingMedium
            top: avatar.top
        }
        width: parent.width - x

        Label {
            width: parent.width
            truncationMode: TruncationMode.Fade
            color: item.highlighted ? Theme.secondaryHighlightColor
                                    : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: model.feedTitle
            textFormat: Text.PlainText
        }

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
            text: model.title
            textFormat: Text.PlainText
        }

        Label {
            width: parent.width
            visible: text.length > 0
            truncationMode: TruncationMode.Fade
            color: item.highlighted ? Theme.secondaryHighlightColor
                                    : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: model.author
            textFormat: Text.PlainText
        }

        Label {
            width: parent.width
            visible: text.length > 0
            wrapMode: Text.Wrap
            maximumLineCount: 5
            elide: Text.ElideRight
            color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            text: model.body
            textFormat: Text.PlainText
        }

        Label {
            width: parent.width
            height: previewRow.visible
                    ? implicitHeight + Theme.paddingMedium : implicitHeight
            visible: item.formattedTime.length > 0
            color: item.highlighted ? Theme.secondaryHighlightColor
                                    : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: item.formattedTime
            textFormat: Text.PlainText
        }

        SocialMediaPreviewRow {
            id: previewRow

            width: parent.width + Theme.horizontalPageMargin
            imageList: item.imageList
            downloader: item.downloader
            accountId: item.accountId
            connectedToNetwork: item.connectedToNetwork
            highlighted: item.highlighted
            eventsColumnMaxWidth: item.eventsColumnMaxWidth - item.avatar.width
        }
    }
}
