#include "swapperwindow.h"

#include "browsericons.h"

#include <QCheckBox>
#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

SwapperWindow::SwapperWindow(const QUrl &url, const QList<Browser> &browsers, const QString &preselectId,
                           const QString &rememberHost, QWidget *parent)
    : QDialog(parent)
    , m_browsers(browsers)
{
    setWindowTitle(tr("Veer"));
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("Open link with"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.15);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto *urlLabel = new QLabel(this);
    const QString urlText = QString::fromUtf8(url.toEncoded());
    urlLabel->setText(QFontMetrics(urlLabel->font()).elidedText(urlText, Qt::ElideMiddle, 420));
    urlLabel->setToolTip(urlText);
    urlLabel->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(urlLabel);

    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(32, 32));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    int selectRow = 0;
    for (int i = 0; i < m_browsers.size(); ++i) {
        const Browser &browser = m_browsers.at(i);
        const QString label = (i < 9) ? QStringLiteral("%1   %2").arg(i + 1).arg(browser.name) : browser.name;
        auto *item = new QListWidgetItem(BrowserIcons::load(browser.iconName), label, m_list);
        item->setSizeHint(QSize(0, 44));
        if (browser.id == preselectId)
            selectRow = i;
    }
    m_list->setCurrentRow(selectRow);
    const int visibleRows = qMin(m_browsers.size(), 8);
    m_list->setFixedHeight(visibleRows * 44 + 2 * m_list->frameWidth() + 2);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        choose(m_list->row(item));
    });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        choose(m_list->row(item));
    });

    auto *bottom = new QHBoxLayout();
    if (!rememberHost.isEmpty()) {
        m_remember = new QCheckBox(tr("Always use this browser for %1").arg(rememberHost), this);
        bottom->addWidget(m_remember, 1);
    } else {
        bottom->addStretch(1);
    }
    auto *cancel = new QPushButton(tr("Cancel"), this);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    bottom->addWidget(cancel);
    layout->addLayout(bottom);

    m_list->setFocus();
}

Browser SwapperWindow::chosenBrowser() const
{
    return (m_chosenRow >= 0) ? m_browsers.at(m_chosenRow) : Browser();
}

bool SwapperWindow::rememberChoice() const
{
    return m_remember && m_remember->isChecked();
}

void SwapperWindow::choose(int row)
{
    if (m_chosenRow >= 0 || row < 0 || row >= m_browsers.size())
        return;
    m_chosenRow = row;
    accept();
}

void SwapperWindow::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();
    if (key >= Qt::Key_1 && key <= Qt::Key_9) {
        const int row = key - Qt::Key_1;
        if (row < m_browsers.size()) {
            choose(row);
            return;
        }
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        choose(m_list->currentRow());
        return;
    }
    QDialog::keyPressEvent(event);
}

void SwapperWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (m_positioned)
        return;
    m_positioned = true;

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    adjustSize();
    const QRect area = screen->availableGeometry();
    move(area.center() - rect().center());
}
