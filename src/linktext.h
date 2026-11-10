#pragma once

#include <QString>
#include <QUrl>

namespace LinkText {

// The first web link found in some text, or an invalid URL when there is none.
// Handles links inside sentences, in brackets, with trailing punctuation, and bare www addresses.
QUrl firstUrl(const QString &text);

} // namespace LinkText
