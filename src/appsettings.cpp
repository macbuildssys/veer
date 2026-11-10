#include "appsettings.h"

namespace {

QString ruleKey(const QString &host)
{
    return QStringLiteral("rules/") + host;
}

} // namespace

QString AppSettings::ruleFor(const QString &host) const
{
    return m_settings.value(ruleKey(host)).toString();
}

void AppSettings::setRule(const QString &host, const QString &browserId)
{
    m_settings.setValue(ruleKey(host), browserId);
    m_settings.sync();
}

void AppSettings::clearRules()
{
    m_settings.remove(QStringLiteral("rules"));
    m_settings.sync();
}

QString AppSettings::lastUsed() const
{
    return m_settings.value(QStringLiteral("lastUsed")).toString();
}

void AppSettings::setLastUsed(const QString &browserId)
{
    m_settings.setValue(QStringLiteral("lastUsed"), browserId);
    m_settings.sync();
}

QStringList AppSettings::hiddenIds() const
{
    return m_settings.value(QStringLiteral("hidden")).toStringList();
}

void AppSettings::setHiddenIds(const QStringList &ids)
{
    m_settings.setValue(QStringLiteral("hidden"), ids);
    m_settings.sync();
}
