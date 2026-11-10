#pragma once

#include "appsettings.h"
#include "browserscanner.h"

#include <QDialog>
#include <QList>

class QLabel;
class QListWidget;
class QPushButton;

// Shown when Veer starts without a link: default browser status and which browsers appear in the swapper
class SetupWindow : public QDialog {
public:
    SetupWindow(const QList<Browser> &browsers, AppSettings &settings, QWidget *parent = nullptr);

    void done(int result) override;

private:
    void refreshStatus();
    void saveHidden();

    QList<Browser> m_browsers;
    AppSettings &m_settings;
    QLabel *m_note = nullptr;
    QPushButton *m_makeDefault = nullptr;
    QListWidget *m_list = nullptr;
};
