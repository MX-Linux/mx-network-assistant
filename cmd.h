#pragma once

#include <QProcess>

class QTextStream;

class Cmd : public QProcess
{
    Q_OBJECT
public:
    explicit Cmd(QObject *parent = nullptr);

    [[nodiscard]] QString getOut(const QString &cmd, bool quiet = false);
    [[nodiscard]] QString getOutAsRoot(const QString &program, const QStringList &args = {}, bool quiet = false,
                                       bool suppressStderr = false);
    bool proc(const QString &program, const QStringList &args = {}, QString *output = nullptr,
              const QByteArray *input = nullptr, bool quiet = false);
    bool procAsRoot(const QString &program, const QStringList &args = {}, QString *output = nullptr,
                    const QByteArray *input = nullptr, bool quiet = false, bool suppressStderr = false);
    bool run(const QString &cmd, bool quiet = false);
    bool runAsRoot(const QString &program, const QStringList &args = {}, bool quiet = false,
                   bool suppressStderr = false);
    bool appendLineAsRoot(const QString &path, const QString &line, bool quiet = false);
    bool writeFileAsRoot(const QString &path, const QByteArray &content, bool quiet = false);

signals:
    void done();

private:
    bool helperProc(const QStringList &helperArgs, QString *output = nullptr, const QByteArray *input = nullptr,
                    bool quiet = false);
    QString elevate;
    QString helper;
};
