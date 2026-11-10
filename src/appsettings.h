#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

// Remembered sites, the last used browser and the browsers hidden from the swapper
class AppSettings {
public:
    QString ruleFor(const QString &host) const;
    void setRule(const QString &host, const QString &browserId);
    void clearRules();

    QString lastUsed() const;
    void setLastUsed(const QString &browserId);

    QStringList hiddenIds() const;
    void setHiddenIds(const QStringList &ids);

private:
    mutable QSettings m_settings;
};
