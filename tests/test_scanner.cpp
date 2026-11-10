#include "browsericons.h"
#include "browserscanner.h"

#include <QDir>
#include <QImage>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {

void writeDesktop(const QString &dir, const QString &fileName, const QString &body)
{
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + fileName);
    QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text), "cannot write desktop file");
    file.write(body.toUtf8());
}

QString entry(const QString &name, const QString &extra)
{
    return QStringLiteral("[Desktop Entry]\nType=Application\nName=%1\nExec=/bin/true %u\n%2\n").arg(name, extra);
}

} // namespace

class ScannerTest : public QObject {
    Q_OBJECT

private slots:
    void argumentsReplaceUrlField()
    {
        const QStringList args = BrowserScanner::buildArguments(QStringLiteral("firefox %u"), QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"firefox", "https://a.org/"}));
    }

    void argumentsDropUnsupportedFields()
    {
        const QStringList args = BrowserScanner::buildArguments(QStringLiteral("app %i %c %k %U"), QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"app", "https://a.org/"}));
    }

    void argumentsKeepQuotedProgramPath()
    {
        const QStringList args = BrowserScanner::buildArguments(
            QStringLiteral("\"/opt/my browser/bin\" --new-window %U"), QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"/opt/my browser/bin", "--new-window", "https://a.org/"}));
    }

    void argumentsAppendUrlWhenNoFieldCode()
    {
        const QStringList args = BrowserScanner::buildArguments(QStringLiteral("browser --flag"), QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"browser", "--flag", "https://a.org/"}));
    }

    void argumentsHandleFlatpakStyle()
    {
        const QStringList args = BrowserScanner::buildArguments(
            QStringLiteral("/usr/bin/flatpak run --command=firefox org.mozilla.firefox @@u %u @@"),
            QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"/usr/bin/flatpak", "run", "--command=firefox", "org.mozilla.firefox",
                                    "@@u", "https://a.org/", "@@"}));
    }

    void argumentsExpandInsideToken()
    {
        const QStringList args = BrowserScanner::buildArguments(QStringLiteral("app --url=%u"), QStringLiteral("https://a.org/"));
        QCOMPARE(args, (QStringList{"app", "--url=https://a.org/"}));
    }

    void argumentsKeepPercentInUrlUntouched()
    {
        const QStringList args = BrowserScanner::buildArguments(QStringLiteral("app %u"), QStringLiteral("https://a.org/?q=%25u"));
        QCOMPARE(args, (QStringList{"app", "https://a.org/?q=%25u"}));
    }

    void scanFindsOnlyUsableBrowsers()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString user = tmp.path() + QLatin1String("/user/applications");
        const QString sys = tmp.path() + QLatin1String("/sys/applications");
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/user")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/sys")).toUtf8());

        const QString web = QStringLiteral("MimeType=x-scheme-handler/http;x-scheme-handler/https;\nCategories=Network;WebBrowser;");
        writeDesktop(sys, "alpha.desktop", entry("Alpha", web));
        writeDesktop(sys, "beta.desktop", entry("Beta", QStringLiteral("MimeType=x-scheme-handler/https;text/html;")));
        writeDesktop(sys, "gamma.desktop", entry("Gamma", web + QStringLiteral("\nNoDisplay=true")));
        writeDesktop(sys, "delta.desktop", entry("Delta", QStringLiteral("MimeType=x-scheme-handler/https;\nCategories=Network;Chat;")));
        writeDesktop(sys, "epsilon.desktop", entry("Epsilon", QStringLiteral("MimeType=x-scheme-handler/mailto;\nCategories=Network;WebBrowser;")));
        writeDesktop(sys, "zeta.desktop", entry("Zeta", web + QStringLiteral("\nTryExec=/definitely/not/installed")));
        writeDesktop(sys, "veer.desktop", entry("Veer", web));
        writeDesktop(sys, "veer.desktop", entry("Veer", web));
        writeDesktop(sys, "hidden.desktop", entry("Hidden", web));
        writeDesktop(user, "hidden.desktop", entry("Hidden", web + QStringLiteral("\nHidden=true")));
        writeDesktop(user, "eta.desktop", entry("Eta", web));
        writeDesktop(sys, "tor.desktop", "[Desktop Entry]\nType=Application\nName=Tor Browser\nExec=sh -c '/bin/true/Browser/start-tor-browser --detach' dummy %k\nCategories=Network;WebBrowser;\n");
        writeDesktop(sys, "terminal.desktop", entry("Terminal Browser", web + QStringLiteral("\nTerminal=true")));

        const QList<Browser> found = BrowserScanner::scan();
        QStringList ids;
        for (const Browser &browser : found)
            ids << browser.id;

        QCOMPARE(ids, (QStringList{"alpha.desktop", "beta.desktop", "eta.desktop", "tor.desktop"}));
        QCOMPARE(found.first().name, QStringLiteral("Alpha"));
        QVERIFY(!found.first().exec.isEmpty());
    }

    void snapEntryWhoseProgramIsMissingIsSkipped()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString dir = tmp.path() + QLatin1String("/usr/share/applications");
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/user")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/usr/share")).toUtf8());

        const QString web = QStringLiteral("MimeType=x-scheme-handler/https;\nCategories=Network;WebBrowser;");
        writeDesktop(dir, "gone.desktop", QStringLiteral("[Desktop Entry]\nType=Application\nName=Gone\n"
            "Exec=env BAMF_DESKTOP_FILE_HINT=/x.desktop /definitely/not/installed %U\n") + web);
        writeDesktop(dir, "here.desktop", QStringLiteral("[Desktop Entry]\nType=Application\nName=Here\n"
            "Exec=env A=b /bin/true %U\n") + web);

        const QList<Browser> found = BrowserScanner::scan();
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().id, QStringLiteral("here.desktop"));
    }

    void scanReturnsNothingWithoutDesktopFiles()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/user")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/sys")).toUtf8());
        QVERIFY(BrowserScanner::scan().isEmpty());
    }

    void duplicateNamesCollapseToOneEntryPreferringSnap()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString snapDir = tmp.path() + QLatin1String("/snapd/desktop/applications");
        const QString sysDir = tmp.path() + QLatin1String("/usr/share/applications");
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/user")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/usr/share:") + tmp.path() + QLatin1String("/snapd/desktop")).toUtf8());

        const QString web = QStringLiteral("MimeType=x-scheme-handler/https;\nCategories=Network;WebBrowser;");
        writeDesktop(sysDir, "chromium-browser.desktop", entry("Chromium Web Browser", web));
        writeDesktop(snapDir, "chromium_chromium.desktop", entry("Chromium Web Browser", web));
        writeDesktop(sysDir, "firefox.desktop", entry("Firefox", web));

        const QList<Browser> found = BrowserScanner::scan();
        QStringList ids;
        for (const Browser &browser : found)
            ids << browser.id;
        QCOMPARE(ids, (QStringList{"chromium_chromium.desktop", "firefox.desktop"}));
    }

    void usersOwnEntryBeatsSnapAndSystem()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        qputenv("HOME", tmp.path().toUtf8());
        const QString userDir = tmp.path() + QLatin1String("/.local/share/applications");
        const QString snapDir = tmp.path() + QLatin1String("/snapd/desktop/applications");
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/.local/share")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/snapd/desktop")).toUtf8());

        const QString web = QStringLiteral("MimeType=x-scheme-handler/https;\nCategories=Network;WebBrowser;");
        writeDesktop(snapDir, "chromium_chromium.desktop", entry("Chromium Web Browser", web));
        writeDesktop(userDir, "chromium-tor.desktop", entry("Chromium Web Browser", web));

        const QList<Browser> found = BrowserScanner::scan();
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().id, QStringLiteral("chromium-tor.desktop"));
    }

    void duplicateNamesFromSameSourceKeepTheFirst()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString sysDir = tmp.path() + QLatin1String("/usr/share/applications");
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/user")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/usr/share")).toUtf8());

        const QString web = QStringLiteral("MimeType=x-scheme-handler/https;\nCategories=Network;WebBrowser;");
        writeDesktop(sysDir, "one.desktop", entry("Same", web));
        writeDesktop(sysDir, "two.desktop", entry("Same", web));

        const QList<Browser> found = BrowserScanner::scan();
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().name, QStringLiteral("Same"));
    }

    void argumentsExpandDesktopFilePath()
    {
        const QStringList args = BrowserScanner::buildArguments(
            QStringLiteral("sh -c script dummy %k"), QStringLiteral("https://a.org/"), QStringLiteral("/p/tor.desktop"));
        QCOMPARE(args, (QStringList{"sh", "-c", "script", "dummy", "/p/tor.desktop", "https://a.org/"}));
    }

    void torLauncherFoundNextToTheDesktopFile()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(QDir().mkpath(tmp.path() + QLatin1String("/Browser")));
        const QString script = tmp.path() + QLatin1String("/Browser/start-tor-browser");
        QFile file(script);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\n");
        file.close();
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));

        const QString exec = QStringLiteral("sh -c '\"$(dirname \"$*\")\"/Browser/start-tor-browser --detach' dummy %k");
        const QStringList args = BrowserScanner::buildArguments(exec, QStringLiteral("https://a.org/"),
                                                                tmp.path() + QLatin1String("/start-tor-browser.desktop"));
        QCOMPARE(args, (QStringList{script, "--detach", "--allow-remote", "https://a.org/"}));
    }

    void torLauncherGetsDetachAndUrl()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString script = tmp.path() + QLatin1String("/start-tor-browser.desktop");
        QFile file(script);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#!/bin/sh\n");
        file.close();
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));

        const QString exec = QStringLiteral("sh -c '\"%1\" --detach || true' dummy %k").arg(script);
        const QStringList args = BrowserScanner::buildArguments(exec, QStringLiteral("https://a.org/"), QStringLiteral("/x.desktop"));
        QCOMPARE(args, (QStringList{script, "--detach", "--allow-remote", "https://a.org/"}));
    }

    void iconFileIsFoundInHicolor()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString iconDir = tmp.path() + QLatin1String("/share/icons/hicolor/48x48/apps");
        QVERIFY(QDir().mkpath(iconDir));
        QImage image(48, 48, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QVERIFY(image.save(iconDir + QLatin1String("/fakebrowser.png")));
        qputenv("XDG_DATA_HOME", (tmp.path() + QLatin1String("/none")).toUtf8());
        qputenv("XDG_DATA_DIRS", (tmp.path() + QLatin1String("/share")).toUtf8());

        QCOMPARE(BrowserIcons::findFile(QStringLiteral("fakebrowser")), iconDir + QLatin1String("/fakebrowser.png"));
        QVERIFY(BrowserIcons::findFile(QStringLiteral("does-not-exist")).isEmpty());
        QVERIFY(BrowserIcons::findFile(QString()).isEmpty());
    }

    void absoluteIconPathIsKeptWhenFileExists()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.path() + QLatin1String("/icon.png");
        QImage image(16, 16, QImage::Format_ARGB32);
        QVERIFY(image.save(path));
        QCOMPARE(BrowserIcons::findFile(path), path);
        QVERIFY(BrowserIcons::findFile(tmp.path() + QLatin1String("/missing.png")).isEmpty());
    }
};

QTEST_APPLESS_MAIN(ScannerTest)
#include "test_scanner.moc"
