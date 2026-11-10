#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

struct Browser {
    QString id;       // desktop file name, for example "firefox.desktop"
    QString name;     // display name
    QString iconName; // theme icon name or absolute path
    QString exec;     // raw Exec line from the desktop file
    QString filePath; // where the desktop file was found
};

class BrowserScanner {
public:
    // Finds every installed application that can open web links, except Veer itself
    static QList<Browser> scan();

    // Directories searched for .desktop files, highest priority first
    static QStringList searchDirs();

    // Expands the Exec line into a full argument list, program first.
    // desktopFile is the path that %k stands for.
    static QStringList buildArguments(const QString &exec, const QString &url, const QString &desktopFile = QString());

    // Starts the browser detached from this process
    static bool launch(const Browser &browser, const QUrl &url);

    static QString selfId();
};
