#include "appsettings.h"
#include "browserscanner.h"
#include "defaultbrowser.h"
#include "linktext.h"
#include "swapperwindow.h"
#include "selectionreader.h"
#include "setupwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

#include <clocale>
#include <langinfo.h>

namespace {

bool hasFlag(int argc, char **argv, const char *flag)
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], flag) == 0)
            return true;
    }
    return false;
}

// Apps started from a bare environment can have a plain ASCII locale, which makes Qt print a warning
void ensureUtf8Locale()
{
    setlocale(LC_CTYPE, "");
    const QByteArray codeset = QByteArray(nl_langinfo(CODESET)).toUpper();
    if (codeset == "UTF-8" || codeset == "UTF8")
        return;
    qputenv(qEnvironmentVariableIsSet("LC_ALL") ? "LC_ALL" : "LC_CTYPE", "C.UTF-8");
}

// Rules are stored per host, so www.example.com and example.com share one rule
QString ruleHost(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
        return QString();
    QString host = url.host().toLower();
    if (host.startsWith(QLatin1String("www.")))
        host.remove(0, 4);
    return host;
}

const Browser *findById(const QList<Browser> &browsers, const QString &id)
{
    for (const Browser &browser : browsers) {
        if (browser.id == id)
            return &browser;
    }
    return nullptr;
}

// Windows style .url files and desktop files of Type=Link both hold the address in a URL= line
QUrl urlFromShortcut(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QUrl();
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith(QLatin1String("URL=")))
            return QUrl(line.mid(4).trimmed());
    }
    return QUrl();
}

QUrl urlFromArgument(const QString &arg)
{
    const QFileInfo info(arg);
    if (info.exists()) {
        const QString suffix = info.suffix().toLower();
        if (suffix == QLatin1String("url") || suffix == QLatin1String("desktop")) {
            const QUrl fromShortcut = urlFromShortcut(info.absoluteFilePath());
            if (fromShortcut.isValid() && !fromShortcut.scheme().isEmpty())
                return fromShortcut;
        }
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }
    return QUrl::fromUserInput(arg);
}

int runCommandLineMode(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    if (hasFlag(argc, argv, "--version")) {
        out << "veer " << QCoreApplication::applicationVersion() << '\n';
        return 0;
    }

    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
        out << "Usage: veer [options] [url]\n"
               "\n"
               "Opens each link in the browser you pick, instead of always using the default.\n"
               "\n"
               "Options:\n"
               "  --list           Print the browsers Veer detected\n"
               "  --set-default    Send all links to Veer\n"
               "  --status         Show which program currently opens web links\n"
               "  --clear-rules    Forget all remembered sites\n"
               "  --selection      Open the link in the highlighted or copied text\n"
               "  --setup          Open the setup window\n"
               "  --screenshot F   Save an image of the window to F (for development)\n"
               "  --version        Show the version\n"
               "  --help           Show this text\n";
        return 0;
    }

    if (hasFlag(argc, argv, "--list")) {
        const QList<Browser> browsers = BrowserScanner::scan();
        for (const Browser &browser : browsers)
            out << browser.id << '\t' << browser.name << '\t' << browser.filePath << '\t' << browser.exec << '\n';
        if (browsers.isEmpty())
            out << "No browsers found\n";
        return 0;
    }

    if (hasFlag(argc, argv, "--status")) {
        for (const QString &mime : DefaultBrowser::mimeTypes()) {
            const QString current = DefaultBrowser::currentFor(mime);
            out << mime << " -> " << (current.isEmpty() ? QStringLiteral("(none)") : current) << '\n';
        }
        const bool ok = DefaultBrowser::isVeer();
        out << (ok ? "Veer is catching links, they will show the swapper\n"
                   : "Veer is NOT catching links, run: veer --set-default\n");
        return ok ? 0 : 1;
    }

    if (hasFlag(argc, argv, "--set-default")) {
        const bool ok = DefaultBrowser::makeVeer();
        for (const QString &mime : DefaultBrowser::mimeTypes())
            out << mime << " -> " << DefaultBrowser::currentFor(mime) << '\n';
        out << (ok ? "Veer now catches all links\n"
                   : "Could not set Veer up, use System Settings, Default Applications, Web browser and choose Veer\n");
        return ok ? 0 : 1;
    }

    AppSettings().clearRules();
    out << "Remembered sites cleared\n";
    return 0;
}

bool launchOrWarn(const Browser &browser, const QUrl &url)
{
    if (BrowserScanner::launch(browser, url))
        return true;
    QMessageBox::warning(nullptr, QStringLiteral("Veer"), QObject::tr("Could not start %1.").arg(browser.name));
    return false;
}

// Shows the window for a moment, saves an image of it and quits. Used for development
int grabAndQuit(QApplication &app, QDialog &dialog, const QString &file)
{
    dialog.show();
    QTimer::singleShot(300, &app, [&dialog, file]() {
        dialog.grab().save(file);
        QCoreApplication::quit();
    });
    return app.exec();
}

QList<Browser> withoutHidden(const QList<Browser> &all, const QStringList &hidden)
{
    QList<Browser> visible;
    for (const Browser &browser : all) {
        if (!hidden.contains(browser.id))
            visible.append(browser);
    }
    // Never leave the swapper empty because everything was hidden
    return visible.isEmpty() ? all : visible;
}

} // namespace

int main(int argc, char **argv)
{
    ensureUtf8Locale();
    QCoreApplication::setOrganizationName(QStringLiteral("veer"));
    QCoreApplication::setApplicationName(QStringLiteral("veer"));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    if (hasFlag(argc, argv, "--list") || hasFlag(argc, argv, "--set-default")
        || hasFlag(argc, argv, "--status") || hasFlag(argc, argv, "--clear-rules")
        || hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h") || hasFlag(argc, argv, "--version")) {
        return runCommandLineMode(argc, argv);
    }

    QApplication app(argc, argv);
    QApplication::setDesktopFileName(QStringLiteral("veer"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/veer-256.png")));

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("url"), QObject::tr("Link or file to open"));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"), QObject::tr("Save an image of the window"),
                                        QStringLiteral("file"));
    QCommandLineOption selectionOption(QStringLiteral("selection"),
                                       QObject::tr("Open the link in the highlighted or copied text"));
    QCommandLineOption setupOption(QStringLiteral("setup"), QObject::tr("Open the setup window"));
    parser.addOption(screenshotOption);
    parser.addOption(selectionOption);
    parser.addOption(setupOption);
    parser.process(app);

    AppSettings settings;
    const QList<Browser> allBrowsers = BrowserScanner::scan();
    QStringList args = parser.positionalArguments();

    if (parser.isSet(selectionOption)) {
        const QUrl found = LinkText::firstUrl(SelectionReader::textWithLink());
        if (!found.isValid()) {
            QMessageBox::information(nullptr, QStringLiteral("Veer"),
                QObject::tr("No link found. Highlight or copy a link first, then try again."));
            return 1;
        }
        args = QStringList{QString::fromUtf8(found.toEncoded())};
    }

    if (args.isEmpty() || parser.isSet(setupOption)) {
        SetupWindow setup(allBrowsers, settings);
        if (parser.isSet(screenshotOption))
            return grabAndQuit(app, setup, parser.value(screenshotOption));
        setup.exec();
        return 0;
    }

    const QList<Browser> browsers = withoutHidden(allBrowsers, settings.hiddenIds());
    if (browsers.isEmpty()) {
        QMessageBox::critical(nullptr, QStringLiteral("Veer"), QObject::tr("No browsers were found."));
        return 1;
    }

    const QUrl url = urlFromArgument(args.first());
    const QString host = ruleHost(url);

    if (!host.isEmpty()) {
        if (const Browser *ruled = findById(browsers, settings.ruleFor(host)))
            return launchOrWarn(*ruled, url) ? 0 : 1;
    }

    if (browsers.size() == 1)
        return launchOrWarn(browsers.first(), url) ? 0 : 1;

    SwapperWindow swapper(url, browsers, settings.lastUsed(), host);
    if (parser.isSet(screenshotOption))
        return grabAndQuit(app, swapper, parser.value(screenshotOption));

    if (swapper.exec() != QDialog::Accepted)
        return 0;

    const Browser chosen = swapper.chosenBrowser();
    settings.setLastUsed(chosen.id);
    if (swapper.rememberChoice() && !host.isEmpty())
        settings.setRule(host, chosen.id);
    return launchOrWarn(chosen, url) ? 0 : 1;
}
