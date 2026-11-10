#include "swapperwindow.h"

#include <QCheckBox>
#include <QListWidget>
#include <QtTest>

namespace {

QList<Browser> sampleBrowsers()
{
    QList<Browser> list;
    for (const char *name : {"Alpha", "Beta", "Gamma"}) {
        Browser browser;
        browser.name = QString::fromLatin1(name);
        browser.id = browser.name.toLower() + QStringLiteral(".desktop");
        list.append(browser);
    }
    return list;
}

} // namespace

class SwapperTest : public QObject {
    Q_OBJECT

private slots:
    void digitKeyChoosesThatRow()
    {
        SwapperWindow swapper(QUrl(QStringLiteral("https://example.com")), sampleBrowsers(), QString(), QString());
        swapper.show();
        QTest::keyClick(&swapper, Qt::Key_2);
        QCOMPARE(swapper.result(), int(QDialog::Accepted));
        QCOMPARE(swapper.chosenBrowser().id, QStringLiteral("beta.desktop"));
        QVERIFY(!swapper.rememberChoice());
    }

    void enterChoosesPreselectedBrowser()
    {
        SwapperWindow swapper(QUrl(QStringLiteral("https://example.com")), sampleBrowsers(),
                            QStringLiteral("gamma.desktop"), QString());
        swapper.show();
        QTest::keyClick(&swapper, Qt::Key_Return);
        QCOMPARE(swapper.chosenBrowser().id, QStringLiteral("gamma.desktop"));
    }

    void escapeCancelsWithoutChoice()
    {
        SwapperWindow swapper(QUrl(QStringLiteral("https://example.com")), sampleBrowsers(), QString(), QString());
        swapper.show();
        QTest::keyClick(&swapper, Qt::Key_Escape);
        QCOMPARE(swapper.result(), int(QDialog::Rejected));
        QVERIFY(swapper.chosenBrowser().id.isEmpty());
    }

    void rememberCheckboxIsReported()
    {
        SwapperWindow swapper(QUrl(QStringLiteral("https://example.com")), sampleBrowsers(), QString(),
                            QStringLiteral("example.com"));
        swapper.show();
        auto *box = swapper.findChild<QCheckBox *>();
        QVERIFY(box);
        box->setChecked(true);
        QTest::keyClick(&swapper, Qt::Key_1);
        QVERIFY(swapper.rememberChoice());
        QCOMPARE(swapper.chosenBrowser().id, QStringLiteral("alpha.desktop"));
    }

    void digitBeyondListIsIgnored()
    {
        SwapperWindow swapper(QUrl(QStringLiteral("https://example.com")), sampleBrowsers(), QString(), QString());
        swapper.show();
        QTest::keyClick(&swapper, Qt::Key_9);
        QVERIFY(swapper.chosenBrowser().id.isEmpty());
    }
};

QTEST_MAIN(SwapperTest)
#include "test_swapper.moc"
