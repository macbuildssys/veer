#pragma once

#include <QString>
#include <QStringList>

namespace DefaultBrowser {

// The link and page types that decide where web links go
QStringList mimeTypes();

// The desktop file id the system currently uses for a type, or an empty string
QString currentFor(const QString &mime);

bool isVeer();

// The http and https types that do not go to Veer yet, as "type is handled by app" lines
QStringList notVeer();

// Sets Veer as the default, forcing each type in case the first attempt did not stick
bool makeVeer();

} // namespace DefaultBrowser
