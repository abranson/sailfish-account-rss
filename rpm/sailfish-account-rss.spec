# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

Name: sailfish-account-rss
Version: 0.1.0
Release: 1
Summary: Sailfish OS account integration for RSS and Atom feeds
License: BSD-3-Clause AND LGPL-2.1-or-later
Source0: %{name}-%{version}.tar.bz2

BuildRequires: qt5-qmake
BuildRequires: qt5-qttools-linguist
BuildRequires: sailfish-svg2png
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(buteosyncfw5) >= 0.10.0
BuildRequires: pkgconfig(socialcache)

Requires: buteo-syncfw-qt5-msyncd
Requires: eventsview-extensions >= 0.1.13
Requires: jolla-settings-accounts
Requires: lipstick-jolla-home-qt5-components >= 1.2.50
Requires: nemo-qml-plugin-configuration-qt5
Requires: nemo-qml-plugin-connectivity >= 0.0.9
Requires: nemo-qml-plugin-dbus-qt5 >= 2.1.0
Requires: sailfishsilica-qt5 >= 1.1.108
Requires: systemd
Requires(post): %{_libexecdir}/manage-groups
Requires(postun): %{_libexecdir}/manage-groups

%description
Adds unauthenticated RSS and Atom feed accounts to Sailfish OS and displays
their cached entries in a unified News group in Events.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
%make_build

%install
%qmake5_install

%post
/sbin/ldconfig
%{_libexecdir}/manage-groups add account-rss || :
systemctl-user try-restart msyncd.service || :

%postun
/sbin/ldconfig
if [ "$1" -eq 0 ]; then
    %{_libexecdir}/manage-groups remove account-rss || :
fi

%files
%license LICENSES/BSD-3-Clause.txt
%license LICENSES/LGPL-2.1-or-later.txt
%{_libdir}/librsscommon.so.*
%exclude %{_libdir}/librsscommon.so
%{_datadir}/accounts/providers/rss.provider
%{_datadir}/accounts/services/rss-posts.service
%{_datadir}/accounts/ui/RssSettingsDisplay.qml
%{_datadir}/accounts/ui/rss.qml
%{_datadir}/accounts/ui/rss-settings.qml
%{_libdir}/qt5/qml/com/jolla/settings/accounts/rss/*
%{_libdir}/buteo-plugins-qt5/oopp/librss-posts-client.so
%config %{_sysconfdir}/buteo/profiles/client/rss-posts.xml
%config %{_sysconfdir}/buteo/profiles/sync/rss.Posts.xml
%{_libdir}/qt5/qml/com/jolla/eventsview/rss/*
%{_datadir}/lipstick/eventfeed/rss-delegate.qml
%{_datadir}/lipstick/eventfeed/RssFeedItem.qml
%{_datadir}/themes/sailfish-default/silica/*/icons/icon-l-rss.png
%{_datadir}/translations/sailfish-account-rss_eng_en.qm
%{_datadir}/translations/lipstick-jolla-home-rss_eng_en.qm
%{_datadir}/translations/source/sailfish-account-rss.ts
%{_datadir}/translations/source/lipstick-jolla-home-rss.ts
