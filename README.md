# Sailfish RSS account

This project adds an unauthenticated RSS account type to Sailfish OS. Each
account points at one RSS 2.0 or Atom 1.0 URL. Buteo stores the newest feed
entries in socialcache and Lipstick presents all enabled accounts in one News
group in Events.

## Building

Build an RPM with the Sailfish SDK from the project root. Parser unit tests are
in tests/auto/tst_rssfeedparser and can be built independently with qmake.

## Runtime identifiers

- Accounts provider: rss
- Accounts service: rss-posts
- Buteo profile template: rss.Posts
- Account service setting: feed_url

## Licensing

This repository contains a mix of `BSD-3-Clause` and
`LGPL-2.1-or-later` source files. REUSE metadata in the tree records the
license and copyright holders for each file.

The inherited social-cache files remain under `LGPL-2.1-or-later`:

- `common/rsspostsdatabase.*`
- `eventsview-plugins/eventsview-plugin-rss/rsspostsmodel.*`

The remaining source files are licensed under `BSD-3-Clause` unless their
SPDX metadata states otherwise.

<!--
SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd

SPDX-License-Identifier: BSD-3-Clause
-->
