#include "appsettings.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QtTest>

class SettingsTest : public QObject {
    Q_OBJECT

private slots:
    void init()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("veertest"));
        QCoreApplication::setApplicationName(QStringLiteral("veertest"));
        m_dir = new QTemporaryDir();
        QVERIFY(m_dir->isValid());
        qputenv("XDG_CONFIG_HOME", m_dir->path().toUtf8());
    }

    void cleanup()
    {
        delete m_dir;
        m_dir = nullptr;
    }

    void rulesRoundTripAndClear()
    {
        AppSettings settings;
        QVERIFY(settings.ruleFor(QStringLiteral("example.com")).isEmpty());
        settings.setRule(QStringLiteral("example.com"), QStringLiteral("firefox.desktop"));
        settings.setRule(QStringLiteral("news.site.org"), QStringLiteral("chromium.desktop"));
        QCOMPARE(settings.ruleFor(QStringLiteral("example.com")), QStringLiteral("firefox.desktop"));
        QCOMPARE(settings.ruleFor(QStringLiteral("news.site.org")), QStringLiteral("chromium.desktop"));

        settings.clearRules();
        QVERIFY(settings.ruleFor(QStringLiteral("example.com")).isEmpty());
        QVERIFY(settings.ruleFor(QStringLiteral("news.site.org")).isEmpty());
    }

    void clearingRulesKeepsLastUsed()
    {
        AppSettings settings;
        settings.setLastUsed(QStringLiteral("librewolf.desktop"));
        settings.setRule(QStringLiteral("example.com"), QStringLiteral("firefox.desktop"));
        settings.clearRules();
        QCOMPARE(settings.lastUsed(), QStringLiteral("librewolf.desktop"));
    }

    void hiddenIdsRoundTrip()
    {
        AppSettings settings;
        QVERIFY(settings.hiddenIds().isEmpty());
        settings.setHiddenIds({QStringLiteral("a.desktop"), QStringLiteral("b.desktop")});
        QCOMPARE(settings.hiddenIds(), (QStringList{"a.desktop", "b.desktop"}));
        settings.setHiddenIds({});
        QVERIFY(settings.hiddenIds().isEmpty());
    }

private:
    QTemporaryDir *m_dir = nullptr;
};

QTEST_APPLESS_MAIN(SettingsTest)
#include "test_settings.moc"
