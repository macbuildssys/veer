#include "defaultbrowser.h"

#include "browserscanner.h"

#include <QProcess>

QStringList DefaultBrowser::mimeTypes()
{
    return {QStringLiteral("x-scheme-handler/http"), QStringLiteral("x-scheme-handler/https"),
            QStringLiteral("text/html")};
}

QString DefaultBrowser::currentFor(const QString &mime)
{
    QProcess process;
    process.start(QStringLiteral("xdg-mime"), {QStringLiteral("query"), QStringLiteral("default"), mime});
    if (!process.waitForFinished(5000))
        return QString();
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool DefaultBrowser::isVeer()
{
    return currentFor(QStringLiteral("x-scheme-handler/https")) == BrowserScanner::selfId()
        && currentFor(QStringLiteral("x-scheme-handler/http")) == BrowserScanner::selfId();
}

QStringList DefaultBrowser::notVeer()
{
    QStringList lines;
    for (const QString &mime : {QStringLiteral("x-scheme-handler/http"), QStringLiteral("x-scheme-handler/https")}) {
        const QString current = currentFor(mime);
        if (current != BrowserScanner::selfId())
            lines << QStringLiteral("%1 is handled by %2").arg(mime, current.isEmpty() ? QStringLiteral("nothing") : current);
    }
    return lines;
}

bool DefaultBrowser::makeVeer()
{
    QProcess::execute(QStringLiteral("xdg-settings"),
        {QStringLiteral("set"), QStringLiteral("default-web-browser"), BrowserScanner::selfId()});
    for (const QString &mime : mimeTypes()) {
        if (currentFor(mime) != BrowserScanner::selfId())
            QProcess::execute(QStringLiteral("xdg-mime"), {QStringLiteral("default"), BrowserScanner::selfId(), mime});
    }
    return isVeer();
}
