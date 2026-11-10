#include "selectionreader.h"

#include "linktext.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>

namespace {

bool hasLink(const QString &text)
{
    return LinkText::firstUrl(text).isValid();
}

QString runTool(const QString &program, const QStringList &args)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(1500)) {
        process.kill();
        return QString();
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();
    return QString::fromUtf8(process.readAllStandardOutput());
}

} // namespace

QString SelectionReader::textWithLink()
{
    // Qt reads both at once on X11. On Wayland it only sees them while its own window has focus
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->supportsSelection()) {
        const QString highlighted = clipboard->text(QClipboard::Selection);
        if (hasLink(highlighted))
            return highlighted;
    }
    const QString copied = clipboard->text(QClipboard::Clipboard);
    if (hasLink(copied))
        return copied;

    // Command line helpers reach the same data without needing a focused window
    struct Tool {
        const char *program;
        QStringList args;
    };
    const QList<Tool> tools = {
        {"wl-paste", {QStringLiteral("--primary"), QStringLiteral("--no-newline")}},
        {"wl-paste", {QStringLiteral("--no-newline")}},
        {"xclip", {QStringLiteral("-o"), QStringLiteral("-selection"), QStringLiteral("primary")}},
        {"xclip", {QStringLiteral("-o"), QStringLiteral("-selection"), QStringLiteral("clipboard")}},
        {"xsel", {QStringLiteral("-o"), QStringLiteral("-p")}},
        {"xsel", {QStringLiteral("-o"), QStringLiteral("-b")}},
    };
    for (const Tool &tool : tools) {
        const QString text = runTool(QString::fromLatin1(tool.program), tool.args);
        if (hasLink(text))
            return text;
    }
    return QString();
}
