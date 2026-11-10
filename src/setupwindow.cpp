#include "setupwindow.h"

#include "browsericons.h"
#include "defaultbrowser.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SetupWindow::SetupWindow(const QList<Browser> &browsers, AppSettings &settings, QWidget *parent)
    : QDialog(parent)
    , m_browsers(browsers)
    , m_settings(settings)
{
    setWindowTitle(tr("Veer"));
    setMinimumWidth(540);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout();
    auto *logo = new QLabel(this);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/veer-64.png")).pixmap(48, 48));
    header->addWidget(logo);

    auto *titles = new QVBoxLayout();
    auto *title = new QLabel(tr("Veer"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.5);
    title->setFont(titleFont);
    titles->addWidget(title);
    auto *subtitle = new QLabel(tr("Veer is a link swapper. It opens links in a browser that may not be your default browser."), this);
    subtitle->setWordWrap(true);
    subtitle->setForegroundRole(QPalette::PlaceholderText);
    titles->addWidget(subtitle);
    header->addLayout(titles, 1);
    layout->addLayout(header);

    m_makeDefault = new QPushButton(tr("Use Veer for all my links"), this);
    connect(m_makeDefault, &QPushButton::clicked, this, [this]() {
        const bool ok = DefaultBrowser::makeVeer();
        refreshStatus();
        if (!ok) {
            m_note->setText(tr("That did not work. %1. Open System Settings, Default Applications, Web browser, "
                               "and choose Veer.").arg(DefaultBrowser::notVeer().join(QStringLiteral("; "))));
        }
    });
    layout->addWidget(m_makeDefault, 0, Qt::AlignLeft);

    const QString listText = (m_browsers.size() == 1)
        ? tr("This is the browser installed on your computer. Untick it if you do not want it to appear on Veer.")
        : tr("These are the browsers installed on your computer. Untick any that you do not want to appear on Veer.");
    auto *listTitle = new QLabel(listText, this);
    listTitle->setWordWrap(true);
    layout->addWidget(listTitle);

    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(32, 32));
    const QStringList hidden = m_settings.hiddenIds();
    for (const Browser &browser : m_browsers) {
        auto *item = new QListWidgetItem(BrowserIcons::load(browser.iconName),
                                         QStringLiteral("%1\n%2").arg(browser.name, browser.id), m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(hidden.contains(browser.id) ? Qt::Unchecked : Qt::Checked);
        item->setData(Qt::UserRole, browser.id);
        item->setSizeHint(QSize(0, 52));
    }
    m_list->setMinimumHeight(qBound(1, m_browsers.size(), 6) * 52 + 4);
    layout->addWidget(m_list);

    m_note = new QLabel(this);
    m_note->setWordWrap(true);
    m_note->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(m_note);

    auto *buttons = new QHBoxLayout();
    auto *forget = new QPushButton(tr("Forget remembered sites"), this);
    connect(forget, &QPushButton::clicked, this, [this]() {
        m_settings.clearRules();
        m_note->setText(tr("Remembered sites cleared."));
    });
    buttons->addWidget(forget);
    buttons->addStretch(1);
    auto *close = new QPushButton(tr("Close"), this);
    close->setDefault(true);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    refreshStatus();
}

void SetupWindow::refreshStatus()
{
    // The button only matters while some link types still go to another app
    m_makeDefault->setVisible(!DefaultBrowser::isVeer());
}

void SetupWindow::saveHidden()
{
    QStringList hidden;
    for (int i = 0; i < m_list->count(); ++i) {
        const QListWidgetItem *item = m_list->item(i);
        if (item->checkState() == Qt::Unchecked)
            hidden << item->data(Qt::UserRole).toString();
    }
    m_settings.setHiddenIds(hidden);
}

void SetupWindow::done(int result)
{
    saveHidden();
    QDialog::done(result);
}
