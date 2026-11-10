#pragma once

#include <QIcon>
#include <QString>

namespace BrowserIcons {

// Absolute path of an icon file for an Icon= value, or an empty string when none is found
QString findFile(const QString &name);

// Best available icon: the theme first, then icon files found by hand, then a generic web icon
QIcon load(const QString &name);

} // namespace BrowserIcons
