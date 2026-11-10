#include "browserscanner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

namespace {

const QLatin1String kSelfId("veer.desktop");

bool parseDesktopFile(const QString &path, Browser &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString name;
    QString exec;
    QString icon;
    QString tryExec;
    QString type;
    QString mimeTypes;
    QString categories;
    bool hidden = false;
    bool noDisplay = false;
    bool terminal = false;
    bool inEntry = false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('['))) {
            inEntry = (line == QLatin1String("[Desktop Entry]"));
            continue;
        }
        if (!inEntry)
            continue;

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();

        if (key == QLatin1String("Name"))
            name = value;
        else if (key == QLatin1String("Exec"))
            exec = value;
        else if (key == QLatin1String("Icon"))
            icon = value;
        else if (key == QLatin1String("TryExec"))
            tryExec = value;
        else if (key == QLatin1String("Type"))
            type = value;
        else if (key == QLatin1String("MimeType"))
            mimeTypes = value;
        else if (key == QLatin1String("Categories"))
            categories = value;
        else if (key == QLatin1String("Hidden"))
            hidden = (value == QLatin1String("true"));
        else if (key == QLatin1String("NoDisplay"))
            noDisplay = (value == QLatin1String("true"));
        else if (key == QLatin1String("Terminal"))
            terminal = (value == QLatin1String("true"));
    }

    if (hidden || noDisplay || terminal || name.isEmpty() || exec.isEmpty())
        return false;
    if (!type.isEmpty() && type != QLatin1String("Application"))
        return false;

    const QStringList mimes = mimeTypes.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    const QStringList cats = categories.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    const bool handlesLinks = mimes.contains(QLatin1String("x-scheme-handler/https"))
        || mimes.contains(QLatin1String("x-scheme-handler/http"));
    const bool looksLikeBrowser = cats.contains(QLatin1String("WebBrowser")) || mimes.contains(QLatin1String("text/html"));
    // Tor Browser's own launcher entry never lists the link types, so it is let in by its start script
    const bool isTor = exec.contains(QLatin1String("start-tor-browser"));
    if (!((handlesLinks && looksLikeBrowser) || isTor))
        return false;

    // Skip leftovers of uninstalled programs. Snap entries start with "env VAR=value /snap/bin/app",
    // so look past env and its settings to the real program.
    QString probe = tryExec;
    if (probe.isEmpty()) {
        const QStringList tokens = QProcess::splitCommand(exec);
        int i = 0;
        if (i < tokens.size() && QFileInfo(tokens.at(i)).fileName() == QLatin1String("env")) {
            ++i;
            while (i < tokens.size() && tokens.at(i).contains(QLatin1Char('=')) && !tokens.at(i).startsWith(QLatin1Char('/')))
                ++i;
        }
        probe = tokens.value(i);
    }
    if (probe.isEmpty() || QStandardPaths::findExecutable(probe).isEmpty())
        return false;

    out.name = name;
    out.exec = exec;
    out.iconName = icon;
    out.filePath = path;
    return true;
}

// Lower is better. An entry the user made in their own folder wins, then Snap and Flatpak, which are the
// real browser, while the package entry is often a stub.
int sourceRank(const QString &filePath)
{
    if (filePath.startsWith(QDir::homePath() + QLatin1String("/.local/share/applications/")))
        return -1;
    if (filePath.contains(QLatin1String("/snapd/")) || filePath.contains(QLatin1String("/snap/")))
        return 0;
    if (filePath.contains(QLatin1String("/flatpak/")))
        return 1;
    return 2;
}

// Several entries with one name are one browser installed in more than one way, so keep one
void collapseDuplicateNames(QList<Browser> &browsers)
{
    QHash<QString, int> best;
    for (int i = 0; i < browsers.size(); ++i) {
        const QString &name = browsers.at(i).name;
        if (!best.contains(name) || sourceRank(browsers.at(i).filePath) < sourceRank(browsers.at(best.value(name)).filePath))
            best.insert(name, i);
    }

    QList<Browser> kept;
    for (int i = 0; i < browsers.size(); ++i) {
        if (best.value(browsers.at(i).name) == i)
            kept.append(browsers.at(i));
    }
    browsers = kept;
}

// Tor Browser's launcher entry has no place for a link, so call its start script directly
QStringList torArguments(const QString &exec, const QString &url, const QString &desktopFile)
{
    static const QRegularExpression script(QStringLiteral("(/[^\"'\\s]*start-tor-browser(?:\\.desktop)?)"));
    QStringList candidates;
    QRegularExpressionMatchIterator it = script.globalMatch(exec);
    while (it.hasNext())
        candidates << it.next().captured(1);
    if (!desktopFile.isEmpty()) {
        const QString dir = QFileInfo(desktopFile).absolutePath();
        candidates << dir + QStringLiteral("/Browser/start-tor-browser") << dir + QStringLiteral("/start-tor-browser.desktop");
    }
    for (const QString &path : candidates) {
        if (QFileInfo(path).isExecutable())
            return {path, QStringLiteral("--detach"), QStringLiteral("--allow-remote"), url};
    }
    return QStringList();
}

} // namespace

QString BrowserScanner::selfId()
{
    return kSelfId;
}

QStringList BrowserScanner::searchDirs()
{
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    const QStringList extras = {
        QDir::homePath() + QLatin1String("/.local/share/flatpak/exports/share/applications"),
        QStringLiteral("/var/lib/flatpak/exports/share/applications"),
        QStringLiteral("/var/lib/snapd/desktop/applications"),
    };
    for (const QString &extra : extras) {
        if (!dirs.contains(extra))
            dirs.append(extra);
    }
    return dirs;
}

QList<Browser> BrowserScanner::scan()
{
    QSet<QString> seen;
    QList<Browser> result;

    for (const QString &dirPath : searchDirs()) {
        const QDir dir(dirPath);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QString &fileName : files) {
            // The first match wins, so user files override system files
            if (seen.contains(fileName))
                continue;
            seen.insert(fileName);
            if (fileName == kSelfId || fileName == QLatin1String("veer.desktop") || fileName == QLatin1String("forkway.desktop"))
                continue;

            Browser browser;
            if (parseDesktopFile(dir.filePath(fileName), browser)) {
                browser.id = fileName;
                result.append(browser);
            }
        }
    }

    collapseDuplicateNames(result);
    std::sort(result.begin(), result.end(), [](const Browser &a, const Browser &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    return result;
}

QStringList BrowserScanner::buildArguments(const QString &exec, const QString &url, const QString &desktopFile)
{
    // The Tor launcher is a shell one-liner that cannot be split into arguments, so it is handled first
    if (exec.contains(QLatin1String("start-tor-browser"))) {
        const QStringList tor = torArguments(exec, url, desktopFile);
        if (!tor.isEmpty())
            return tor;
    }

    const QStringList tokens = QProcess::splitCommand(exec);
    QStringList result;
    bool urlPlaced = false;

    for (const QString &token : tokens) {
        QString expanded;
        for (int i = 0; i < token.size(); ++i) {
            const QChar c = token.at(i);
            if (c != QLatin1Char('%') || i + 1 >= token.size()) {
                expanded.append(c);
                continue;
            }
            const QChar code = token.at(++i);
            switch (code.unicode()) {
            case 'u':
            case 'U':
            case 'f':
            case 'F':
                expanded.append(url);
                urlPlaced = true;
                break;
            case 'k':
                expanded.append(desktopFile);
                break;
            case '%':
                expanded.append(QLatin1Char('%'));
                break;
            default:
                // %i, %c and friends expand to nothing here
                break;
            }
        }
        if (!expanded.isEmpty())
            result.append(expanded);
    }

    if (urlPlaced)
        return result;

    result.append(url);
    return result;
}

bool BrowserScanner::launch(const Browser &browser, const QUrl &url)
{
    QStringList args = buildArguments(browser.exec, QString::fromUtf8(url.toEncoded()), browser.filePath);
    if (args.isEmpty())
        return false;
    const QString program = args.takeFirst();
    return QProcess::startDetached(program, args);
}
