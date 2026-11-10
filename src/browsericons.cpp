#include "browsericons.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QStandardPaths>

namespace {

QStringList dataDirs()
{
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    const QStringList extras = {
        QDir::homePath() + QLatin1String("/.local/share/flatpak/exports/share"),
        QStringLiteral("/var/lib/flatpak/exports/share"),
        QStringLiteral("/var/lib/snapd/desktop"),
    };
    for (const QString &extra : extras) {
        if (!dirs.contains(extra))
            dirs.append(extra);
    }
    return dirs;
}

} // namespace

QString BrowserIcons::findFile(const QString &name)
{
    if (name.isEmpty())
        return QString();
    if (name.startsWith(QLatin1Char('/')))
        return QFileInfo::exists(name) ? name : QString();

    // Qt only draws SVG files when its SVG plugin is installed
    const bool svgSupported = QImageReader::supportedImageFormats().contains("svg");
    static const int sizes[] = {256, 128, 96, 64, 48, 32, 24, 22, 16};

    for (const QString &dir : dataDirs()) {
        for (int size : sizes) {
            const QString png = QStringLiteral("%1/icons/hicolor/%2x%2/apps/%3.png").arg(dir).arg(size).arg(name);
            if (QFileInfo::exists(png))
                return png;
        }
        const QString pixmapPng = QStringLiteral("%1/pixmaps/%2.png").arg(dir, name);
        if (QFileInfo::exists(pixmapPng))
            return pixmapPng;
        if (svgSupported) {
            const QString scalable = QStringLiteral("%1/icons/hicolor/scalable/apps/%2.svg").arg(dir, name);
            if (QFileInfo::exists(scalable))
                return scalable;
            const QString pixmapSvg = QStringLiteral("%1/pixmaps/%2.svg").arg(dir, name);
            if (QFileInfo::exists(pixmapSvg))
                return pixmapSvg;
        }
        const QString xpm = QStringLiteral("%1/pixmaps/%2.xpm").arg(dir, name);
        if (QFileInfo::exists(xpm))
            return xpm;
    }
    return QString();
}

QIcon BrowserIcons::load(const QString &name)
{
    QIcon icon;
    if (!name.isEmpty() && !name.startsWith(QLatin1Char('/')))
        icon = QIcon::fromTheme(name);

    if (icon.isNull()) {
        const QString file = findFile(name);
        if (!file.isEmpty())
            icon = QIcon(file);
    }
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("web-browser"));
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("internet-web-browser"));
    return icon;
}
