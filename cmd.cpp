#include "cmd.h"

#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryFile>

#include <unistd.h>

Cmd::Cmd(QObject *parent)
    : QProcess(parent),
      elevate {QFile::exists("/usr/bin/pkexec") ? "/usr/bin/pkexec" : "/usr/bin/gksu"},
      helper {"/usr/lib/" + QApplication::applicationName() + "/helper"}
{
}

QString Cmd::getOut(const QString &cmd, bool quiet)
{
    QString output;
    run(cmd, quiet);
    output = QString::fromUtf8(readAllStandardOutput()).trimmed();
    return output;
}

QString Cmd::getOutAsRoot(const QString &program, const QStringList &args, bool quiet, bool suppressStderr)
{
    QString output;
    procAsRoot(program, args, &output, nullptr, quiet, suppressStderr);
    return output;
}

bool Cmd::proc(const QString &program, const QStringList &args, QString *output, const QByteArray *input, bool quiet)
{
    if (state() != QProcess::NotRunning) {
        qDebug() << "Process already running:" << QProcess::program() << arguments();
        return false;
    }
    if (!quiet) {
        qDebug() << program << args;
    }

    QEventLoop loop;
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);

    start(program, args);
    if (input && !input->isEmpty()) {
        write(*input);
    }
    closeWriteChannel();
    loop.exec();

    if (output) {
        *output = QString::fromUtf8(readAllStandardOutput()).trimmed();
    }
    emit done();
    return (exitStatus() == QProcess::NormalExit && exitCode() == 0);
}

bool Cmd::helperProc(const QStringList &helperArgs, QString *output, const QByteArray *input, bool quiet)
{
    const bool isRoot = (getuid() == 0);
    if (!isRoot && elevate.isEmpty()) {
        qWarning() << "No elevation command available";
        return false;
    }

    QStringList programArgs = helperArgs;
    const QString program = isRoot ? helper : elevate;
    if (!isRoot) {
        programArgs.prepend(helper);
    }
    return proc(program, programArgs, output, input, quiet);
}

bool Cmd::procAsRoot(const QString &program, const QStringList &args, QString *output, const QByteArray *input,
                     bool quiet, bool suppressStderr)
{
    QStringList helperArgs {"exec", program};
    if (suppressStderr) {
        helperArgs << "--quiet-stderr";
    }
    helperArgs += args;
    return helperProc(helperArgs, output, input, quiet);
}

bool Cmd::run(const QString &cmd, bool quiet)
{
    return proc("/bin/bash", {"-c", cmd}, nullptr, nullptr, quiet);
}

bool Cmd::runAsRoot(const QString &program, const QStringList &args, bool quiet, bool suppressStderr)
{
    return procAsRoot(program, args, nullptr, nullptr, quiet, suppressStderr);
}

bool Cmd::appendLineAsRoot(const QString &path, const QString &line, bool quiet)
{
    return helperProc({"append-line", path, line}, nullptr, nullptr, quiet);
}

bool Cmd::writeFileAsRoot(const QString &path, const QByteArray &content, bool quiet)
{
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        qWarning() << "Unable to create temporary file for root write";
        return false;
    }
    if (tempFile.write(content) != content.size()) {
        qWarning() << "Unable to stage content for root write";
        return false;
    }
    tempFile.flush();
    return helperProc({"write-file-from", path, tempFile.fileName()}, nullptr, nullptr, quiet);
}
