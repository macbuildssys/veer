#include "linktext.h"

#include <QRegularExpression>

namespace {

int countOf(const QString &text, QChar c)
{
    return text.count(c);
}

// Removes punctuation that belongs to the sentence around a link, not to the link itself
QString trimTrailing(QString link)
{
    for (;;) {
        if (link.isEmpty())
            return link;
        const QChar last = link.at(link.size() - 1);
        if (QStringLiteral(".,;:!?'\"").contains(last)) {
            link.chop(1);
            continue;
        }
        // A closing bracket stays only when the link itself opened one, as in some wiki addresses
        if (last == QLatin1Char(')') && countOf(link, QLatin1Char(')')) > countOf(link, QLatin1Char('('))) {
            link.chop(1);
            continue;
        }
        if (last == QLatin1Char(']') && countOf(link, QLatin1Char(']')) > countOf(link, QLatin1Char('['))) {
            link.chop(1);
            continue;
        }
        if (last == QLatin1Char('}') && countOf(link, QLatin1Char('}')) > countOf(link, QLatin1Char('{'))) {
            link.chop(1);
            continue;
        }
        return link;
    }
}

} // namespace

QUrl LinkText::firstUrl(const QString &text)
{
    static const QRegularExpression pattern(
        QStringLiteral("(?:https?://|www\\.)[^\\s<>\"`]+"), QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator matches = pattern.globalMatch(text);
    while (matches.hasNext()) {
        QString candidate = trimTrailing(matches.next().captured());
        if (candidate.startsWith(QLatin1String("www."), Qt::CaseInsensitive))
            candidate.prepend(QLatin1String("http://"));

        const QUrl url(candidate, QUrl::StrictMode);
        if (url.isValid() && !url.host().isEmpty() && url.host().contains(QLatin1Char('.')))
            return url;
    }
    return QUrl();
}
