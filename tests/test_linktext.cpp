#include "linktext.h"

#include <QtTest>

class LinkTextTest : public QObject {
    Q_OBJECT

private slots:
    void data_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("expected");

        QTest::newRow("plain link") << "https://example.com/page" << "https://example.com/page";
        QTest::newRow("in a sentence") << "Read this https://example.com/a?b=1#c now" << "https://example.com/a?b=1#c";
        QTest::newRow("trailing full stop") << "See https://example.com/page." << "https://example.com/page";
        QTest::newRow("trailing comma and quote") << "He said \"https://example.com/x\", then left"
                                                  << "https://example.com/x";
        QTest::newRow("wrapped in brackets") << "(https://example.com/page)" << "https://example.com/page";
        QTest::newRow("wrapped in angle brackets") << "<https://example.com/page>" << "https://example.com/page";
        QTest::newRow("wiki address keeps its bracket") << "https://en.wikipedia.org/wiki/Foo_(bar)"
                                                        << "https://en.wikipedia.org/wiki/Foo_(bar)";
        QTest::newRow("bare www") << "go to www.example.com/shop today" << "http://www.example.com/shop";
        QTest::newRow("first of several") << "a https://one.org b https://two.org" << "https://one.org";
        QTest::newRow("mixed case scheme") << "HTTPS://Example.com/Path" << "https://example.com/Path";
        QTest::newRow("multi line text") << "line one\nhttps://example.com/two\nline three" << "https://example.com/two";
        QTest::newRow("youtube watch link") << "https://www.youtube.com/watch?v=_YS83iErY3s&list=RD_YS83iErY3s&start_radio=1"
                                            << "https://www.youtube.com/watch?v=_YS83iErY3s&list=RD_YS83iErY3s&start_radio=1";
        QTest::newRow("no link") << "just some words, no address" << "";
        QTest::newRow("empty") << "" << "";
        QTest::newRow("host without a dot is not a link") << "http://localhost" << "";
        QTest::newRow("skips a broken one and finds the next") << "http:// then https://example.com/ok"
                                                               << "https://example.com/ok";
    }

    void data()
    {
        QFETCH(QString, text);
        QFETCH(QString, expected);
        const QUrl url = LinkText::firstUrl(text);
        if (expected.isEmpty()) {
            QVERIFY(!url.isValid());
        } else {
            QVERIFY(url.isValid());
            QCOMPARE(url.toString(QUrl::FullyEncoded), expected);
        }
    }
};

QTEST_APPLESS_MAIN(LinkTextTest)
#include "test_linktext.moc"
