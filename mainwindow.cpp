//
//   Copyright (C) 2003-2010 by Warren Woodford
//   Copyright (C) 2014 by Adrian adrian@mxlinux.org
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

//   With big modifications made by Adrian adrian@mxlinux.org

#include "mainwindow.h"
#include "version.h"
#include <QFileDialog>
#include <QMenu>
#include <QClipboard>
#include <QDesktopWidget>
#include <QDebug>

#include <unistd.h>

MainWindow::MainWindow(QWidget* parent)
    : QDialog(parent) {
    qDebug() << "Program Version:" << VERSION;
    setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setWindowIcon(QApplication::windowIcon());

    hwUnblock->hide();

    currentTab = 0;
    tabWidget->setCurrentIndex(0);

    configurationChanges[0] = false;
    configurationChanges[1] = false;

    pingProc  = new QProcess(this);
    traceProc = new QProcess(this);
    installProc = new QProcess(this);

    installOutputEdit = new QTextEdit();

    connect(hwList, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(showContextMenuForHw(const QPoint &)));
    connect(linuxDrvList, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(showContextMenuForLinuxDrv(const QPoint &)));
    connect(windowsDrvList, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(showContextMenuForWindowsDrv(const QPoint &)));
}

MainWindow::~MainWindow() {
}

/////////////////////////////////////////////////////////////////////////
// util functions


QStringList MainWindow::getCmdOuts(QString cmd) {
    char line[260];
    FILE* fp = popen(cmd.toUtf8(), "r");
    QStringList results;
    if (fp == nullptr) {
        return results;
    }
    int i;
    while (fgets(line, sizeof line, fp) != nullptr) {
        i = strlen(line);
        line[--i] = '\0';
        results.append(line);
    }
    pclose(fp);
    return results;
}

QString MainWindow::getCmdValue(QString cmd, QString key, QString keydel, QString valdel) {
    const char *ret = "";
    char line[260];

    QStringList strings = getCmdOuts(cmd);
    for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
        strcpy(line, ((QString)*it).toUtf8());
        char* keyptr = strstr(line, key.toUtf8());
        if (keyptr != nullptr) {
            // key found
            strtok(keyptr, keydel.toUtf8());
            const char* val = strtok(nullptr, valdel.toUtf8());
            if (val != nullptr) {
                ret = val;
            }
            break;
        }
    }
    return QString (ret);
}

QStringList MainWindow::getCmdValues(QString cmd, QString key, QString keydel, QString valdel) {
    char line[130];
    FILE* fp = popen(cmd.toUtf8(), "r");
    QStringList results;
    if (fp == nullptr) {
        return results;
    }
    int i;
    while (fgets(line, sizeof line, fp) != nullptr) {
        i = strlen(line);
        line[--i] = '\0';
        char* keyptr = strstr(line, key.toUtf8());
        if (keyptr != nullptr) {
            // key found
            strtok(keyptr, keydel.toUtf8());
            const char* val = strtok(nullptr, valdel.toUtf8());
            if (val != nullptr) {
                results.append(val);
            }
        }
    }
    pclose(fp);
    return results;
}

bool MainWindow::replaceStringInFile(QString oldtext, QString newtext, QString filepath) {

    QString cmd = QString("sed -i 's/%1/%2/g' %3").arg(oldtext).arg(newtext).arg(filepath);
    if (system(cmd.toUtf8()) != 0) {
        return false;
    }
    return true;
}

void MainWindow::refresh() {
    hwUnblock->hide();
    groupWifi->hide();
    int i = tabWidget->currentIndex();
    QString out = shell.getOutput("rfkill list 2>&1");
    qApp->processEvents();

    switch (i) {
    case 0: // Status
        on_hwDiagnosePushButton_clicked();
        if (out == "Can't open RFKILL control device: No such file or directory") {
            hwUnblock->hide();
        } else {
            hwUnblock->show();
        }
        labelRouterIP->setText(tr("IP address from router:") + " " + getIPfromRouter());
        labelIP->setText(tr("External IP address:") + " " + getIP());
        labelInterface->setText(shell.getOutput("route | grep '^default' | grep -o '[^ ]*$'"));
        checkWifiAvailable();
        checkWifiEnabled();
        break;
    case 1: // Linux drivers
        on_linuxDrvDiagnosePushButton_clicked();
        break;
    case 2: // Windows drivers
        on_windowsDrvDiagnosePushButton_clicked();
        break;
    case 3: // Diagnostic
        break;

    default:
        bool changed = configurationChanges[0];
        configurationChanges[0] = changed;
        break;
    }
}

void MainWindow::displayDoc(QString url)
{
    QString exec = "xdg-open";
    QString user = shell.getOutput("logname");
    if (system("command -v mx-viewer") == 0) { // use mx-viewer if available
        exec = "mx-viewer";
    }
    QString cmd = "su " + user + " -c \"env XDG_RUNTIME_DIR=/run/user/$(id -u " + user + ") " + exec + " " + url + "\"&";
    shell.run(cmd);
}

/////////////////////////////////////////////////////////////////////////
// special

void MainWindow::on_cancelPing_clicked()
{
    if (pingProc->state() != QProcess::NotRunning)
    {
        pingProc->kill();
    }
    cancelPing->setEnabled(false);
}

void MainWindow::on_cancelTrace_clicked()
{
    if (traceProc->state() != QProcess::NotRunning)
    {
        traceProc->kill();
    }
    cancelTrace->setEnabled(false);
}

void MainWindow::on_clearPingOutput_clicked()
{
    pingOutputEdit->clear();
    clearPingOutput->setEnabled(false);
}

void MainWindow::hwListToClipboard()
{
    if (hwList->currentRow() != -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem* currentElement = hwList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::hwListFullToClipboard()
{
    if (hwList->count() > -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < hwList->count(); i++)
        {
            QListWidgetItem* currentElement = hwList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::linuxDrvListToClipboard()
{
    if (linuxDrvList->currentRow() != -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem* currentElement = linuxDrvList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::linuxDrvListFullToClipboard()
{
    if (hwList->count() > -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < linuxDrvList->count(); i++)
        {
            QListWidgetItem* currentElement = linuxDrvList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::windowsDrvListToClipboard()
{
    if (linuxDrvList->currentRow() != -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem* currentElement = windowsDrvList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::windowsDrvListFullToClipboard()
{
    if (hwList->count() > -1)
    {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < windowsDrvList->count(); i++)
        {
            QListWidgetItem* currentElement = windowsDrvList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::showContextMenuForHw(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction * copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, SIGNAL(activated()) , this, SLOT(hwListToClipboard()));
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction * copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, SIGNAL(activated()) , this, SLOT(hwListFullToClipboard()));
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(hwList->mapToGlobal(pos));
}

void MainWindow::showContextMenuForLinuxDrv(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction * copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, SIGNAL(activated()) , this, SLOT(linuxDrvListToClipboard()));
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction * copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, SIGNAL(activated()) , this, SLOT(linuxDrvListFullToClipboard()));
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(linuxDrvList->mapToGlobal(pos));
}

void MainWindow::showContextMenuForWindowsDrv(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction * copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, SIGNAL(activated()) , this, SLOT(windowsDrvListToClipboard()));
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction * copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, SIGNAL(activated()) , this, SLOT(windowsDrvListFullToClipboard()));
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(windowsDrvList->mapToGlobal(pos));
}

void MainWindow::on_clearTraceOutput_clicked()
{
    tracerouteOutputEdit->clear();
    clearTraceOutput->setEnabled(false);
}

void MainWindow::writeTraceOutput()
{
    QByteArray bytes = traceProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");
    for (const QString &line : lines) {
        if (!line.isEmpty())
        {
            tracerouteOutputEdit->append(line);
        }
    }
}

void MainWindow::tracerouteFinished()
{
    cancelTrace->setEnabled(false);
}

void MainWindow::on_tracerouteButton_clicked()
{
    QString statusl = getCmdValue("dpkg -s traceroute | grep '^Status'", "ok", " ", " ");
    if (statusl.compare("installed") != 0)
    {
        if (internetConnection)
        {
            setCursor(QCursor(Qt::WaitCursor));
            int ret = QMessageBox::information(0, tr("Traceroute not installed"),
                                               tr("Traceroute is not installed, do you want to install it now?"),
                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (ret == QMessageBox::Yes)
            {
                system("apt-get install -qq traceroute");
                setCursor(QCursor(Qt::ArrowCursor));
                statusl = getCmdValue("dpkg -s traceroute | grep '^Status'", "ok", " ", " ");
                if (statusl.compare("installed") != 0) {
                    QMessageBox::critical(0, tr("Traceroute hasn't been installed"),
                                             tr("Traceroute cannot be installed. This may mean you are using the LiveCD or you are unable to reach the software repository,"),
                                             QMessageBox::Ok);
                }
                else
                {
                    setCursor(QCursor(Qt::ArrowCursor));
                    return;
                }
            }
        }
        else
        {
            QMessageBox::critical(0, tr("Traceroute not installed"),
                                     tr("Traceroute is not installed and no Internet connection could be detected so it cannot be installed"),
                                     QMessageBox::Ok);
            return;
        }
    }
    if (tracerouteHostEdit->text().isEmpty())
    {
        QMessageBox::information(0, tr("No destination host"),
                                 tr("Please fill in the destination host field"),
                                 QMessageBox::Ok);
    }
    else
    {
        setCursor(QCursor(Qt::WaitCursor));

        QString program = "traceroute";
        QStringList arguments;
        arguments << tracerouteHostEdit->text() << QString("-m %1").arg(traceHopsNumber->value());

        if (traceProc->state() != QProcess::NotRunning)
        {
            traceProc->kill();
        }

        traceProc->start(program, arguments);
        disconnect(traceProc, SIGNAL(readyReadStandardOutput()), 0, 0);
        connect(traceProc, SIGNAL(readyReadStandardOutput()), this, SLOT(writeTraceOutput()));
        disconnect(traceProc, SIGNAL(finished(int,QProcess::ExitStatus)), 0, 0);
        connect(traceProc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(tracerouteFinished()));

        //QStringList vals = getCmdOuts(QString("traceroute %1").arg(tracerouteHostEdit->text()));
        //tracerouteOutputEdit->append(vals.join("\n"));
        clearTraceOutput->setEnabled(true);
        cancelTrace->setEnabled(true);
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void MainWindow::writePingOutput()
{
    QByteArray bytes = pingProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");
    for (const QString &line : lines) {
        if (!line.isEmpty())
        {
            pingOutputEdit->append(line);
        }
    }
}

void MainWindow::pingFinished()
{
    cancelPing->setEnabled(false);
}

void MainWindow::on_pingButton_clicked()
{
    if (pingHostEdit->text().isEmpty())
    {
        QMessageBox::information(0, tr("No destination host"),
                                 tr("Please fill in the destination host field"),
                                 QMessageBox::Ok);
    }
    else
    {
        setCursor(QCursor(Qt::WaitCursor));
        QString program = "ping";
        QStringList arguments;
        arguments << QString("-c %1").arg(pingPacketNumber->value()) << "-W 5" << pingHostEdit->text();

        if (pingProc->state() != QProcess::NotRunning)
        {
            pingProc->kill();
        }
        pingProc->start(program, arguments);
        disconnect(pingProc, SIGNAL(readyReadStandardOutput()), 0, 0);
        connect(pingProc, SIGNAL(readyReadStandardOutput()), this, SLOT(writePingOutput()));
        disconnect(pingProc, SIGNAL(finished(int,QProcess::ExitStatus)), 0, 0);
        connect(pingProc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(pingFinished()));

        //QStringList vals = getCmdOuts(QString("ping -c 4 -W 5 %1").arg(pingHostEdit->text()));
        //pingOutputEdit->append(vals.join("\n"));
        clearPingOutput->setEnabled(true);
        cancelPing->setEnabled(true);
        setCursor(QCursor(Qt::ArrowCursor));
    }
}


/////////////////////////////////////////////////////////////////////////
// slots

void MainWindow::show() {
    QDialog::show();
    refresh();
}


// Added
void MainWindow::on_hwDiagnosePushButton_clicked()
{
    hwList->clear();

    // Query PCI cards
    QStringList  queryResult = getCmdOuts("lspci -nn | grep -i net");
    for (int i = 0; i < queryResult.size(); ++i)
    {
        QString currentElement = queryResult.at(i);
        if (currentElement.indexOf("Ethernet controller") != -1)
        {
            currentElement.remove(QRegExp("Ethernet controller .[\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f].:"));
            new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/places/network-server.png"),currentElement, hwList);
        }
        else if (currentElement.indexOf("Network controller") != -1)
        {
            currentElement.remove(QRegExp("Network controller .[\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f].:"));
            new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/places/network-workgroup.png"),currentElement, hwList);
        }
        else
        {
            new QListWidgetItem(currentElement, hwList);
        }
    }

    // Query USB cards
    queryResult = getCmdOuts("lsusb | grep -i network");
    for (int i = 0; i < queryResult.size(); ++i)
    {
        QString currentElement = queryResult.at(i);
        if (currentElement.indexOf("Ethernet controller:") != -1)
        {
            currentElement.remove(QString("Ethernet controller:"), Qt::CaseSensitive);
            new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/places/network-server.png"),currentElement, hwList);
        }
        else if (currentElement.indexOf("Network controller:") != -1)
        {
            currentElement.remove(QString("Network controller:"), Qt::CaseSensitive);
            new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/places/network-workgroup.png"),currentElement, hwList);
        }
        else
        {
            new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/places/network-server-database.png"),currentElement, hwList);
        }
    }
    checkWifiEnabled();
}

void MainWindow::on_linuxDrvList_currentRowChanged(int currentRow )
{
    if (currentRow != -1 && !linuxDrvList->currentItem()->text().contains("---------")) {
        linuxDrvBlacklistPushButton->setEnabled(true);
        linuxDrvUnload->setEnabled(loadedModules.contains(linuxDrvList->currentItem()->text()));
        linuxDrvLoad->setEnabled(unloadedModules.contains(linuxDrvList->currentItem()->text()));
        if (blacklistedModules.contains(linuxDrvList->currentItem()->text()) && !loadedModules.contains(linuxDrvList->currentItem()->text())){
            linuxDrvLoad->setEnabled(true);
        }
    } else {
        linuxDrvBlacklistPushButton->setEnabled(false);
        linuxDrvLoad->setEnabled(false);
        linuxDrvUnload->setEnabled(false);
    }
    updateDriverStatus();
}

void MainWindow::on_linuxDrvDiagnosePushButton_clicked()
{
    linuxDrvList->clear();
    loadedModules.clear();
    //QStringList queryResult = getCmdOuts("lsmod | grep -i net");
    QStringList loadedKernelModules = getCmdOuts("lsmod");
    QStringList completeKernelNetModules = getCmdOuts("find /lib/modules/$(uname -r)/kernel/drivers/net -name *.ko");
    completeKernelNetModules = completeKernelNetModules.replaceInStrings(".ko", "");
    completeKernelNetModules = completeKernelNetModules.replaceInStrings(QRegExp("[\\w|\\.|-]*/"), "");
    // Those three kernel modules are in the "misc" section we add them manually
    // To the filter list for convenience
    completeKernelNetModules << "ndiswrapper";
    completeKernelNetModules << "atl2";
    completeKernelNetModules << "wl";
    for (int i = 0; i < loadedKernelModules.size(); ++i)
    {
        QString mod = loadedKernelModules.at(i);
        if (completeKernelNetModules.contains(mod.left(mod.indexOf(' '))))
        {
            loadedModules.append(mod.left(mod.indexOf(' '))); //add to the QStringList of loadedModule
        }
    }

    for (int i = 0; i < loadedModules.size(); ++i)
    {
        QString mod = loadedModules.at(i);
        if (i == 0) {
            new QListWidgetItem("---------" + tr("Loaded Drivers") + "-------------", linuxDrvList);
        }
        new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/apps/ksysguardd.png"), mod, linuxDrvList);
    }

    // list unloaded modules
    for (int i = 0; i < unloadedModules.size(); ++i)
    {
        QString mod = unloadedModules.at(i);
        if (i == 0) {
            new QListWidgetItem("---------" + tr("Unloaded Drivers") + "-----------", linuxDrvList);
        }
        QListWidgetItem *unloaded = new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/apps/ksysguardd.png"), mod, linuxDrvList);
        unloaded->setForeground(Qt::blue);
    }

    QFile inputBlacklist(QString("/etc/modprobe.d/blacklist.conf"));
    inputBlacklist.open(QFile::ReadOnly|QFile::Text);

    QString driver;
    QString s;
    // add blacklisted modules to the list
    int i = 0;
    while (!inputBlacklist.atEnd())
    {
        if (i == 0) {
            new QListWidgetItem("---------" + tr("Blacklisted Drivers") + " --------", linuxDrvList);
        }
        i++;
        s = inputBlacklist.readLine();
        QRegExp expr("^\\s*blacklist\\s*.*");
        if (expr.exactMatch(s)) {
            QString captured = expr.cap(0);
            captured.remove("blacklist");
            driver = captured.trimmed();
            QListWidgetItem *blacklisted = new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/apps/ksysguardd.png"), driver, linuxDrvList);
            blacklisted->setForeground(Qt::red);
            blacklistedModules.append(driver);
        }
    }
    inputBlacklist.close();

    // add blacklisted modules from /etc/modprobe.d/broadcom-sta-dkms.conf
    QFile inputBroadcomBlacklist(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
    inputBroadcomBlacklist.open(QFile::ReadOnly|QFile::Text);
    i = 0;
    while (!inputBroadcomBlacklist.atEnd())
    {
        if (i == 0) {
            new QListWidgetItem("---------" + tr("Blacklisted Broadcom Drivers") + "--------", linuxDrvList);
        }
        i++;
        s = inputBroadcomBlacklist.readLine();
        QRegExp expr("^\\s*blacklist\\s*.*");
        if (expr.exactMatch(s)) {
            QString captured = expr.cap(0);
            captured.remove("blacklist");
            driver = captured.trimmed();
            QListWidgetItem *blacklisted = new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/apps/ksysguardd.png"), driver, linuxDrvList);
            blacklisted->setForeground(Qt::red);
            blacklistedModules.append(driver);
            broadcomModules.append(driver);
        }
    }
    inputBroadcomBlacklist.close();
}

void MainWindow::on_windowsDrvDiagnosePushButton_clicked()
{
    windowsDrvList->clear();
    if (system("[ -f /usr/sbin/ndiswrapper ]") != 0) {
        uninstallNdiswrapper->setVisible(false);
        QMessageBox::warning(0, windowTitle(), QApplication::tr("Ndiswrapper is not installed"));
        return;
    }
    QStringList queryResult = getCmdOuts("ndiswrapper -l");
    int i = 0;
    while (i < queryResult.size())
    {
        QString currentElement = queryResult.at(i);
        QString label = currentElement.left(currentElement.indexOf(":"));
        label.append(QApplication::tr("driver installed"));
        //label = currentElement.remove(": ");
        if ((i + 1) < queryResult.size())
        {
            if (!queryResult.at(i + 1).contains(": driver installed"))
            {
                QString installInfo = queryResult.at(i + 1);
                int infoPos = installInfo.indexOf(QRegExp("[\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F]:[\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F]"));
                //device (14E4:4320) present (alternate driver: bcm43xx)
                if (infoPos != -1)
                {
                    label.append(QApplication::tr(" and in use by "));
                    label.append(installInfo.midRef(infoPos, 9));
                }
                if (installInfo.contains("alternate driver"))
                {
                    infoPos = installInfo.lastIndexOf(": ");
                    if (infoPos != -1)
                    {
                        int strLen = installInfo.length();
                        label.append(QApplication::tr(". Alternate driver: "));
                        QString s =installInfo.right(strLen - infoPos);
                        s.remove(")");
                        s.remove(" ");
                        s.remove(":");
                        label.append(s);
                    }
                }
                i++;
            }
        }
        new QListWidgetItem(QIcon("/usr/share/icons/default.kde4/16x16/apps/krfb.png"), label, windowsDrvList);
        i++;
    }
}

bool MainWindow::blacklistModule(QString module)
{
    QFile outputBlacklist;
    if (!broadcomModules.contains(module))
    {
        outputBlacklist.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
    }
    else
    {
        outputBlacklist.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
    }

    if (!outputBlacklist.open(QFile::Append|QFile::Text))
    {
        return false;
    }

    outputBlacklist.write(QString("blacklist %1\n").arg(module).toUtf8());
    outputBlacklist.close();

    if (removable(module))
    {
        if (!removeModule(module))
        {
            return false;
        }
    }
    loadedModules.removeAll(module);
    return true;
}

void MainWindow::on_linuxDrvBlacklistPushButton_clicked()
{
    if (linuxDrvList->currentRow() != -1)
    {
        QListWidgetItem* currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if (driverBlacklisted)
        {
            QFile inputBlacklist;
            QFile outputBlacklist;
            if (broadcomModules.contains(driver))
            {
                inputBlacklist.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
                outputBlacklist.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));

            }
            else
            {
                inputBlacklist.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
                outputBlacklist.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
            }
            if (!inputBlacklist.open(QFile::ReadOnly|QFile::Text))
            {
                return;
            }

            QString s, outputString("");
            while (!inputBlacklist.atEnd())
            {
                s = inputBlacklist.readLine();
                QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
                if (!s.contains(QRegExp(expr)))
                {
                    outputString += s;
                }
                outputBlacklist.write(s.toUtf8());
            }
            inputBlacklist.close();
            if (!outputBlacklist.open(QFile::WriteOnly|QFile::Text))
            {
                return;
            }
            outputBlacklist.write(outputString.toUtf8());
            outputBlacklist.close();
            QMessageBox::information(0, QApplication::tr("Driver removed from blacklist"),
                                     QApplication::tr("Driver removed from blacklist."));
            loadModule(driver);
            driverBlacklisted = false;
            unloadedModules.removeAll(driver);
            blacklistedModules.removeAll(driver);
        }
        else if (blacklistModule(driver))
        {
            QMessageBox::information(0, QApplication::tr("Module blacklisted"),
                                     QApplication::tr("Module blacklisted"));
            driverBlacklisted = true;
        }
    }
    on_linuxDrvDiagnosePushButton_clicked();
}

// load module
bool MainWindow::loadModule(QString module)
{
    system("service network-manager stop");
    system("modprobe cfg80211"); //this has to get loaded and some drivers don't put it back correctly
    QString cmd = QString("modprobe %1").arg(module);
    if (system(cmd.toUtf8()) != 0)
    {
        // run depmod and try to load again
        system("depmod");
        if (system(cmd.toUtf8()) != 0)
        {
            QString msg = QObject::tr("Could not load ");
            msg += module;
            QMessageBox::information(0, windowTitle(), msg);
            system("pkill wpa_supplicant");
            system("service network-manager start");
            return false;
        }
    }
    if (!loadedModules.contains(module))
    {
        loadedModules.append(module);
    }
    system("pkill wpa_supplicant");
    system("service network-manager start");
    return true;
}

// check if the module can be removed
bool MainWindow::removable(QString module)
{
    QString cmd = QString("modprobe -rn %1").arg(module);
    if (system(cmd.toUtf8()) != 0)
    {
        return false;
    }
    return true;
}


// remove module
bool MainWindow::removeModule(QString module)
{
    system("service network-manager stop");
    QString cmd = QString("modprobe -r %1").arg(module);
    if (system(cmd.toUtf8()) != 0)
    {
        QString msg = QObject::tr("Could not unload ");
        msg += module;
        QMessageBox::information(0, windowTitle(), msg);
        system("service network-manager start");
        return false;
    }
    system("service network-manager start");
    return true;
}

bool MainWindow::removeStart(QString module)
{
    QFile inputModules(QString("/etc/modules"));
    QFile outputModules(QString("/etc/modules"));
    if (!inputModules.open(QFile::ReadOnly|QFile::Text))
    {
        return false;
    }

    QString s, outputString("");
    while (!inputModules.atEnd())
    {
        s = inputModules.readLine();
        QString expr = QString("^\\s*(%1)\\s*").arg(module);
        if (!s.contains(QRegExp(expr)))
        {
            outputString += s;
        }
        outputModules.write(s.toUtf8());
    }
    inputModules.close();
    if (!outputModules.open(QFile::WriteOnly|QFile::Text))
    {
        return false;
    }
    outputModules.write(outputString.toUtf8());
    outputModules.close();
    return true;
}


// install Linux Driver
bool MainWindow::installModule(QString module)
{
    if (!loadModule(module))
    {
        return false;
    }
    QFile outputModules(QString("/etc/modules"));
    if (!outputModules.open(QFile::Append|QFile::Text))
    {
        return false;
    }
    outputModules.write(QString("%1\n").arg(module).toUtf8());
    outputModules.close();
    return true;
}

// run apt-get update and at the end start installNDIS
void MainWindow::on_installNdiswrapper_clicked()
{
    setCursor(QCursor(Qt::BusyCursor));
    if (installProc->state() != QProcess::NotRunning)
    {
        installProc->kill();
    }
    installProc->start("apt-get update");
    installOutputEdit->clear();
    installOutputEdit->show();
    installOutputEdit->resize(800, 600);
    // center output window
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width()-installOutputEdit->width()) / 2;
    int y = (screenGeometry.height()-installOutputEdit->height()) / 2;
    installOutputEdit->move(x, y);
    // hide main window
    this->hide();
    installOutputEdit->raise();
    disconnect(installProc, SIGNAL(readyReadStandardOutput()), 0, 0);
    connect(installProc, SIGNAL(readyReadStandardOutput()), this, SLOT(writeInstallOutput()));
    disconnect(installProc, SIGNAL(finished(int)), 0, 0);
    connect(installProc, SIGNAL(finished(int)), this, SLOT(aptUpdateFinished()));
}


// Uninstall ndiswrapper
void MainWindow::on_uninstallNdiswrapper_clicked()
{
    setCursor(QCursor(Qt::BusyCursor));

    if (installProc->state() != QProcess::NotRunning)
    {
        installProc->kill();
    }
    removeModule("ndiswrapper");
    installProc->start("apt-get purge -y ndiswrapper-utils-1.9 ndiswrapper-dkms ndiswrapper-common");
    installOutputEdit->clear();
    installOutputEdit->show();
    installOutputEdit->resize(800, 600);
    // center output window
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width()-installOutputEdit->width()) / 2;
    int y = (screenGeometry.height()-installOutputEdit->height()) / 2;
    installOutputEdit->move(x, y);
    // hide main window
    this->hide();
    installOutputEdit->raise();
    disconnect(installProc, SIGNAL(readyReadStandardOutput()), 0, 0);
    connect(installProc, SIGNAL(readyReadStandardOutput()), this, SLOT(writeInstallOutput()));
    disconnect(installProc, SIGNAL(finished(int)), 0, 0);
    connect(installProc, SIGNAL(finished(int)), this, SLOT(uninstallNdisFinished(int)));
}

// install NDISwrapper
void MainWindow::aptUpdateFinished()
{
    if (installProc->state() != QProcess::NotRunning)
    {
        installProc->kill();
    }
    installProc->start("apt-get install -y ndiswrapper-utils-1.9 ndiswrapper-dkms");
    disconnect(installProc, SIGNAL(readyReadStandardOutput()), 0, 0);
    connect(installProc, SIGNAL(readyReadStandardOutput()), this, SLOT(writeInstallOutput()));
    disconnect(installProc, SIGNAL(finished(int)), 0, 0);
    connect(installProc, SIGNAL(finished(int)), this, SLOT(installFinished(int)));
}

// finished ndiswrapper install
void MainWindow::installFinished(int errorCode)
{
    installOutputEdit->close();
    this->show();
    setCursor(QCursor(Qt::ArrowCursor));
    if (errorCode == 0)
    {
        if (installModule("ndiswrapper"))
        {
            uninstallNdiswrapper->setVisible(true);
            QMessageBox::information(0, windowTitle(), QApplication::tr("Installation successful"));
        }
        else
        {
            QMessageBox::information(0, windowTitle(), QApplication::tr("Error detected, could not compile ndiswrapper driver."));
        }
    }
    else
    {
        QMessageBox::warning(0, windowTitle(), QApplication::tr("Error detected, could not install ndiswrapper."));
    }
}

void MainWindow::uninstallNdisFinished(int errorCode)
{
    installOutputEdit->close();
    this->show();
    setCursor(QCursor(Qt::ArrowCursor));
    if (errorCode == 0) {
        removeStart("ndiswrapper");
    } else {
        QMessageBox::warning(0, windowTitle(), QApplication::tr("Error encountered while removing Ndiswrapper"));
    }

}

void MainWindow::writeInstallOutput()
{
    QByteArray bytes = installProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");

    for (const QString &line : lines) {
        if (!line.isEmpty())
        {
            installOutputEdit->append(line);
        }
    }
}

void MainWindow::updateDriverStatus()
{
    driverBlacklisted = false;
    QFile inputBlacklist(QString("/etc/modprobe.d/blacklist.conf"));
    QFile inputBroadcomBlacklist(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
    inputBlacklist.open(QFile::ReadOnly|QFile::Text);
    inputBroadcomBlacklist.open(QFile::ReadOnly|QFile::Text);

    QString driver;

    if (linuxDrvList->currentRow() != -1)
    {
        QListWidgetItem* currentDriver = linuxDrvList->currentItem();
        driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
    }

    QString s;
    while (!inputBlacklist.atEnd())
    {
        s = inputBlacklist.readLine();
        QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
        if (s.contains(QRegExp(expr)))
        {
            driverBlacklisted = true;
            break;
        }
    }
    while (!inputBroadcomBlacklist.atEnd())
    {
        s = inputBroadcomBlacklist.readLine();
        QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
        if (s.contains(QRegExp(expr)))
        {
            driverBlacklisted = true;
            break;
        }
    }
    if (driverBlacklisted)
    {
        linuxDrvBlacklistPushButton->setText(QApplication::tr("Unblacklist Driver"));
        linuxDrvBlacklistPushButton->setIcon(QIcon("/usr/share/mx-network-assistant/icons/redo.png"));
    }
    else
    {
        linuxDrvBlacklistPushButton->setText(QApplication::tr("Blacklist Driver"));
        linuxDrvBlacklistPushButton->setIcon(QIcon("/usr/share/mx-network-assistant/icons/file_locked.png"));
    }
    inputBlacklist.close();
}

bool MainWindow::checkSysFileExists(QDir searchPath, QString fileName, Qt::CaseSensitivity cs)
{
    QStringList fileList = searchPath.entryList(QStringList() << "*.SYS");
    bool found = false;
    QStringList::Iterator it = fileList.begin();
    while (it != fileList.end()) {
        if ((*it).contains(fileName, cs)) {
            found = true;
            break;
        }
        ++it;
    }
    return found;
}

bool MainWindow::checkWifiAvailable()
{
    if (system("lspci | grep -Ei 'wireless|wifi' || lspci | grep -Ei 'wireless|wifi'") == 0) {
        groupWifi->show();
        return true;
    } else {
        groupWifi->hide();
        return false;
    }
}

bool MainWindow::checkWifiEnabled()
{
  hwUnblock->hide();
  if (shell.getOutput("nmcli -t --fields WIFI r") == "enabled") {
      labelWifi->setText(tr("enabled"));
      return true;
  } else if (shell.getOutput("nmcli -t --fields WIFI-HW r") == "enabled") {
      labelWifi->setText(tr("disabled"));
      hwUnblock->show();
  } else {
      labelWifi->setText(tr("WiFi hardware switch is off"));
  }
  return false;
}

void MainWindow::on_windowsDrvAddPushButton_clicked()
{
    QString infFileName = QFileDialog::getOpenFileName(this,
                                                       tr("Locate the Windows driver you want to add"), "/home", tr("Windows installation information file (*.inf)"));
    if (!infFileName.isEmpty())
    {
        // Search in the inf file the name of the .sys file
        QFile infFile(infFileName);
        infFile.open(QFile::ReadOnly|QFile::Text);

        QString s;
        bool found = false;
        bool exist = false;
        QStringList foundSysFiles;
        QDir sysDir(infFileName);
        sysDir.cdUp();
        while ((!infFile.atEnd())) {
            s = infFile.readLine();
            if (s.contains("ServiceBinary",Qt::CaseInsensitive)) {
                s = s.right(s.length() - s.lastIndexOf('\\'));
                s = s.remove('\\');
                s = s.remove('\n');
                found = true;
                if (this->checkSysFileExists(sysDir, s, Qt::CaseInsensitive)) {
                    exist = true;
                }
                else {
                    foundSysFiles << s;
                }
            }
        }
        infFile.close();

        if (found) {
            if (!exist) {
                QMessageBox::warning(0, QString(tr("*.sys file not found")), tr("The *.sys files must be in the same location as the *.inf file. %1 cannot be found").arg(foundSysFiles.join(", ")));
            }
            else {
                QString cmd = QString("ndiswrapper -i %1").arg(infFileName);
                shell.run(cmd);
                cmd = QString("ndiswrapper -ma");
                shell.run(cmd);
                on_windowsDrvDiagnosePushButton_clicked();
            }
        }
        else {
            QMessageBox::critical(0, QString(tr("sys file reference not found")).arg(infFile.fileName()), tr("The sys file for the given driver cannot be determined after parsing the inf file"));
        }
    }
}

void MainWindow::on_windowsDrvList_currentRowChanged(int row)
{
    windowsDrvRemovePushButton->setEnabled(row != -1);
}


void MainWindow::on_windowsDrvRemovePushButton_clicked()
{
    if (windowsDrvList->currentRow() != -1)
    {
        QListWidgetItem* currentDriver = windowsDrvList->currentItem();
        QString driver = currentDriver->text();
        QString cmd = QString("ndiswrapper -r %1").arg(driver.left(driver.indexOf(" ")));
        shell.run(cmd);
        QMessageBox::information(0, windowTitle(), tr("Ndiswrapper driver removed."));
        on_windowsDrvDiagnosePushButton_clicked();
    }
}

void MainWindow::on_generalHelpPushButton_clicked()
{
    displayDoc("/usr/share/doc/mx-network-assistant/help/mx-network-assistant.html");
}

void MainWindow::on_tabWidget_currentChanged()
{
    int i = tabWidget->currentIndex();
    if (i != currentTab)
    {
        if (configurationChanges[currentTab])
        {
            configurationChanges[currentTab] = false;
        }
        currentTab = i;
    }

    refresh();
}

// unblock Wifi devices
void MainWindow::on_hwUnblock_clicked()
{
    if (system("rfkill unblock wlan wifi") != 0) {
        QMessageBox::warning(0, windowTitle(), QApplication::tr("Could not unlock devices.\nWiFi device(s) might already be unlocked."));
    } else {
        QMessageBox::information(0, windowTitle(), QApplication::tr("WiFi devices unlocked."));
    }
    checkWifiEnabled();
}

// close but do not apply
void MainWindow::on_buttonCancel_clicked()
{
    close();
}

// About button clicked
void MainWindow::on_buttonAbout_clicked()
{
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Network Assistant"), "<p align=\"center\"><b><h2>" +
                       tr("MX Network Assistant") + "</h2></b></p><p align=\"center\">" + tr("Version: ") +
                       VERSION + "</p><p align=\"center\"><h3>" +
                       tr("Program for troubleshooting and configuring network for MX Linux") + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MEPIS LLC and MX Linux") + "<br /><br /></p>", 0, this);
    QPushButton *btnLicense = msgBox.addButton(tr("License"), QMessageBox::HelpRole);
    QPushButton *btnChangelog = msgBox.addButton(tr("Changelog"), QMessageBox::HelpRole);
    QPushButton *btnCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        displayDoc("file:///usr/share/doc/mx-network-assistant/license.html");
    } else if (msgBox.clickedButton() == btnChangelog) {
        QDialog *changelog = new QDialog(this);
        changelog->resize(600, 500);

        QTextEdit *text = new QTextEdit;
        text->setReadOnly(true);
        Cmd cmd;
        text->setText(cmd.getOutput("zless /usr/share/doc/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName()  + "/changelog.gz"));

        QPushButton *btnClose = new QPushButton(tr("&Close"));
        btnClose->setIcon(QIcon::fromTheme("window-close"));
        connect(btnClose, &QPushButton::clicked, changelog, &QDialog::close);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(text);
        layout->addWidget(btnClose);
        changelog->setLayout(layout);
        changelog->exec();
    }
    this->show();
}


QString MainWindow::getIP()
{
    return shell.getOutput("wget -q -O - checkip.dyndns.org|sed -e 's/.*Current IP Address: //' -e 's/<.*$//'");
}

QString MainWindow::getIPfromRouter()
{
    return shell.getOutput("ifconfig | grep 'inet ' | sed -e 's/inet addr://' -e 's/ Bcast.*//'  -e 's/127.*//'  -e 's/\\s*//'");
}

void MainWindow::on_linuxDrvLoad_clicked()
{
    if (linuxDrvList->currentRow() != -1)
    {
        QListWidgetItem* currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if (loadModule(driver))
        {
            unloadedModules.removeAll(driver);
            QMessageBox::information(0, QApplication::tr("Driver loaded successfully"),
                                     QApplication::tr("Driver loaded successfully"));
            on_linuxDrvDiagnosePushButton_clicked();
        }
    }
}

void MainWindow::on_linuxDrvUnload_clicked()
{
    if (linuxDrvList->currentRow() != -1)
    {
        QListWidgetItem* currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if ((removable(driver)) && !unloadedModules.contains(driver))
        {
            if (removeModule(driver))
            {
                unloadedModules.append(driver);
                QMessageBox::information(0, QApplication::tr("Driver unloaded successfully"),
                                         QApplication::tr("Driver unloaded successfully"));
                on_linuxDrvDiagnosePushButton_clicked();
            }
        }
    }
}
