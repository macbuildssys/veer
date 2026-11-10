#pragma once

#include <QString>

namespace SelectionReader {

// Text that contains a web link: the highlighted text first, then the copied text.
// Returns an empty string when neither holds a link.
QString textWithLink();

} // namespace SelectionReader
