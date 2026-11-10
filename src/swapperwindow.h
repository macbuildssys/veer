#pragma once

#include "browserscanner.h"

#include <QDialog>
#include <QList>
#include <QUrl>

class QCheckBox;
class QListWidget;

class SwapperWindow : public QDialog {
public:
    SwapperWindow(const QUrl &url, const QList<Browser> &browsers, const QString &preselectId,
                 const QString &rememberHost, QWidget *parent = nullptr);

    Browser chosenBrowser() const;
    bool rememberChoice() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void choose(int row);

    QList<Browser> m_browsers;
    QListWidget *m_list = nullptr;
    QCheckBox *m_remember = nullptr;
    int m_chosenRow = -1;
    bool m_positioned = false;
};
