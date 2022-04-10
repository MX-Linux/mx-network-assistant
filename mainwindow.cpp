/*
   Copyright (C) 2003-2010 by Warren Woodford
   Copyright (C) 2014 by Adrian adrian@mxlinux.org

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   With big modifications made by Adrian adrian@mxlinux.org
*/

#include "mainwindow.h"
#include "version.h"

#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QScreen>

#include <unistd.h>
#include <about.h>

MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent) {
    qDebug() << "Program Version:" << VERSION;
    setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setWindowIcon(QApplication::windowIcon());

    hwUnblock->hide();

    currentTab = Tab::Status;
    tabWidget->setCurrentIndex(Tab::Status);
    tabWidget->removeTab(2); // remove WindowsDrivers tab.

    pingProc  = new QProcess(this);
    traceProc = new QProcess(this);
    installProc = new QProcess(this);

    installOutputEdit = new QTextEdit();

    connect(hwList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenuForHw);
    connect(linuxDrvList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenuForLinuxDrv);
    connect(windowsDrvList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenuForWindowsDrv);
}

MainWindow::~MainWindow() {
}

bool MainWindow::replaceStringInFile(QString oldtext, QString newtext, QString filepath) {

    QString cmd = QString("sed -i 's/%1/%2/g' %3").arg(oldtext).arg(newtext).arg(filepath);
    if (system(cmd.toUtf8()) != 0)
        return false;
    return true;
}

void MainWindow::refresh() {
    hwUnblock->hide();
    groupWifi->hide();
    QString out = cmd.getCmdOut("rfkill list 2>&1");
    qApp->processEvents();

    switch (tabWidget->currentIndex()) {
    case Tab::Status:
        on_hwDiagnosePushButton_clicked();
        if (out == "Can't open RFKILL control device: No such file or directory")
            hwUnblock->hide();
        else
            hwUnblock->show();
        labelRouterIP->setText(tr("IP address from router:") + " " + getIPfromRouter());
        labelIP->setText(tr("External IP address:") + " " + getIP());
        labelInterface->setText(cmd.getCmdOut("ip route | grep ^default | grep -Po '(?<=dev )(\\S+)'"));
        checkWifiAvailable();
        checkWifiEnabled();
        break;
    case Tab::LinuxDrivers:
        on_linuxDrvDiagnosePushButton_clicked();
        break;
//    case Tab::WindowsDrivers:
//        on_windowsDrvDiagnosePushButton_clicked();
//        break;
    }
}

void MainWindow::on_cancelPing_clicked()
{
    if (pingProc->state() != QProcess::NotRunning)
        pingProc->kill();
    cancelPing->setEnabled(false);
}

void MainWindow::on_cancelTrace_clicked()
{
    if (traceProc->state() != QProcess::NotRunning)
        traceProc->kill();
    cancelTrace->setEnabled(false);
}

void MainWindow::on_clearPingOutput_clicked()
{
    pingOutputEdit->clear();
}

void MainWindow::hwListToClipboard()
{
    if (hwList->currentRow() != -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem *currentElement = hwList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::hwListFullToClipboard()
{
    if (hwList->count() > -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < hwList->count(); i++) {
            QListWidgetItem *currentElement = hwList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::linuxDrvListToClipboard()
{
    if (linuxDrvList->currentRow() != -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem *currentElement = linuxDrvList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::linuxDrvListFullToClipboard()
{
    if (hwList->count() > -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < linuxDrvList->count(); i++) {
            QListWidgetItem *currentElement = linuxDrvList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::windowsDrvListToClipboard()
{
    if (linuxDrvList->currentRow() != -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QListWidgetItem *currentElement = windowsDrvList->currentItem();
        clipboard->setText(currentElement->text());
    }
}

void MainWindow::windowsDrvListFullToClipboard()
{
    if (hwList->count() > -1) {
        QClipboard *clipboard = QApplication::clipboard();
        QString elementList = "";
        for (int i = 0; i < windowsDrvList->count(); i++) {
            QListWidgetItem *currentElement = windowsDrvList->item(i);
            elementList += currentElement->text() + "\n";
        }
        clipboard->setText(elementList);
    }
}

void MainWindow::showContextMenuForHw(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction *copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, &QAction::triggered, this, &MainWindow::hwListToClipboard);
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction *copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, &QAction::triggered, this, &MainWindow::hwListFullToClipboard);
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(hwList->mapToGlobal(pos));
}

void MainWindow::showContextMenuForLinuxDrv(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction *copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, &QAction::triggered, this, &MainWindow::linuxDrvListToClipboard);
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction *copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, &QAction::triggered, this, &MainWindow::linuxDrvListFullToClipboard);
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(linuxDrvList->mapToGlobal(pos));
}

void MainWindow::showContextMenuForWindowsDrv(const QPoint &pos)
{
    QMenu contextMenu(this);
    QAction *copyAction = new QAction(tr("&Copy"), this);
    connect(copyAction, &QAction::triggered, this, &MainWindow::windowsDrvListToClipboard);
    copyAction->setShortcut(tr("Ctrl+C"));
    QAction *copyAllAction = new QAction(tr("Copy &All"), this);
    connect(copyAllAction, &QAction::triggered, this, &MainWindow::windowsDrvListFullToClipboard);
    copyAllAction->setShortcut(tr("Ctrl+A"));
    contextMenu.addAction(copyAction);
    contextMenu.addAction(copyAllAction);
    contextMenu.exec(windowsDrvList->mapToGlobal(pos));
}

void MainWindow::on_clearTraceOutput_clicked()
{
    tracerouteOutputEdit->clear();
}

void MainWindow::writeTraceOutput()
{
    QByteArray bytes = traceProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");
    for (const QString &line : lines)
        if (!line.isEmpty())
            tracerouteOutputEdit->append(line);
}

void MainWindow::tracerouteFinished()
{
    cancelTrace->setEnabled(false);
}

void MainWindow::on_tracerouteButton_clicked()
{
    QString statusl = cmd.getCmdOut("dpkg -s traceroute | grep ^Status");
    if (statusl != "Status: install ok installed") {
        if (internetConnection) {
            setCursor(QCursor(Qt::WaitCursor));
            int ret = QMessageBox::information(this, tr("Traceroute not installed"),
                                               tr("Traceroute is not installed, do you want to install it now?"),
                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (ret == QMessageBox::Yes) {
                system("apt-get install -qq traceroute");
                setCursor(QCursor(Qt::ArrowCursor));
                statusl = cmd.getCmdOut("dpkg -s traceroute | grep ^Status");
                if (statusl != "Status: install ok installed") {
                    QMessageBox::critical(this, tr("Traceroute hasn't been installed"),
                                          tr("Traceroute cannot be installed. This may mean you are using the LiveCD or you are unable to reach the software repository,"),
                                          QMessageBox::Ok);
                } else {
                    setCursor(QCursor(Qt::ArrowCursor));
                    return;
                }
            }
        } else {
            QMessageBox::critical(this, tr("Traceroute not installed"),
                                  tr("Traceroute is not installed and no Internet connection could be detected so it cannot be installed"),
                                  QMessageBox::Ok);
            return;
        }
    }
    if (tracerouteHostEdit->text().isEmpty()) {
        QMessageBox::information(this, tr("No destination host"), tr("Please fill in the destination host field"),
                                 QMessageBox::Ok);
    } else {
        setCursor(QCursor(Qt::WaitCursor));

        QString program = "traceroute";
        QStringList arguments;
        arguments << tracerouteHostEdit->text() << QString("-m %1").arg(traceHopsNumber->value());

        if (traceProc->state() != QProcess::NotRunning) traceProc->kill();

        traceProc->start(program, arguments);
        disconnect(traceProc, &QProcess::readyReadStandardOutput, nullptr, nullptr);
        connect(traceProc, &QProcess::readyReadStandardOutput, this, &MainWindow::writeTraceOutput);
        disconnect(traceProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), nullptr, nullptr);
        connect(traceProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::tracerouteFinished);

        clearTraceOutput->setEnabled(true);
        cancelTrace->setEnabled(true);
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void MainWindow::writePingOutput()
{
    QByteArray bytes = pingProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");
    for (const QString &line : lines)
        if (!line.isEmpty())
            pingOutputEdit->append(line);
}

void MainWindow::pingFinished()
{
    cancelPing->setEnabled(false);
}

void MainWindow::on_pingButton_clicked()
{
    if (pingHostEdit->text().isEmpty()) {
        QMessageBox::information(this, tr("No destination host"), tr("Please fill in the destination host field"),
                                 QMessageBox::Ok);
    } else {
        setCursor(QCursor(Qt::WaitCursor));
        QString program = "ping";
        QStringList arguments;
        arguments << QString("-c %1").arg(pingPacketNumber->value()) << "-W 5" << pingHostEdit->text();

        if (pingProc->state() != QProcess::NotRunning)  pingProc->kill();
        pingProc->start(program, arguments);
        disconnect(pingProc, &QProcess::readyReadStandardOutput, nullptr, nullptr);
        connect(pingProc, &QProcess::readyReadStandardOutput, this, &MainWindow::writePingOutput);
        disconnect(pingProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), nullptr, nullptr);
        connect(pingProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::pingFinished);

        //QStringList vals = getCmdOuts(QString("ping -c 4 -W 5 %1").arg(pingHostEdit->text()));
        //pingOutputEdit->append(vals.join("\n"));
        clearPingOutput->setEnabled(true);
        cancelPing->setEnabled(true);
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void MainWindow::show() {
    QDialog::show();
    refresh();
}

void MainWindow::on_hwDiagnosePushButton_clicked()
{
    hwList->clear();

    // Query PCI cards
    QStringList  queryResult = cmd.getCmdOut("lspci -nn | grep -i net").split("\n");
    queryResult.removeAll(QString(""));
    for (int i = 0; i < queryResult.size(); ++i) {
        QString currentElement = queryResult.at(i);
        if (currentElement.indexOf("Ethernet controller") != -1) {
            currentElement.remove(QRegExp("Ethernet controller .[\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f].:"));
            new QListWidgetItem(QIcon::fromTheme("network-card"),currentElement, hwList);
        } else if (currentElement.indexOf("Network controller") != -1) {
            currentElement.remove(QRegExp("Network controller .[\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f][\\d|a|b|c|d|e|f].:"));
            new QListWidgetItem(QIcon::fromTheme("network-workgroup"),currentElement, hwList);
        } else {
            new QListWidgetItem(QIcon::fromTheme("network-server-database"), currentElement, hwList);
        }
    }

    // Query USB cards
    queryResult = cmd.getCmdOut("lsusb | grep -i network").split("\n");
    queryResult.removeAll(QString(""));
    for (int i = 0; i < queryResult.size(); ++i) {
        QString currentElement = queryResult.at(i);
        if (currentElement.indexOf("Ethernet controller:") != -1) {
            currentElement.remove(QString("Ethernet controller:"), Qt::CaseSensitive);
            new QListWidgetItem(QIcon::fromTheme("network-card"),currentElement, hwList);
        } else if (currentElement.indexOf("Network controller:") != -1) {
            currentElement.remove(QString("Network controller:"), Qt::CaseSensitive);
            new QListWidgetItem(QIcon::fromTheme("network-workgroup"), currentElement, hwList);
        } else {
            new QListWidgetItem(QIcon::fromTheme("network-server-database"), currentElement, hwList);
        }
    }
    checkWifiEnabled();
}

void MainWindow::on_linuxDrvList_currentRowChanged(int currentRow )
{
    if (currentRow != -1 && !linuxDrvList->currentItem()->text().contains("---------")) {
        linuxDrvBlockPushButton->setEnabled(true);
        linuxDrvUnload->setEnabled(loadedModules.contains(linuxDrvList->currentItem()->text()));
        linuxDrvLoad->setEnabled(unloadedModules.contains(linuxDrvList->currentItem()->text()));
        if (blockedModules.contains(linuxDrvList->currentItem()->text()) && !loadedModules.contains(linuxDrvList->currentItem()->text()))
            linuxDrvLoad->setEnabled(true);
    } else {
        linuxDrvBlockPushButton->setEnabled(false);
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
    QStringList loadedKernelModules = cmd.getCmdOut("lsmod").split("\n");
    QStringList completeKernelNetModules = cmd.getCmdOut("find /lib/modules/$(uname -r)/kernel/drivers/net -name *.ko").split("\n");
    completeKernelNetModules = completeKernelNetModules.replaceInStrings(".ko", "");
    completeKernelNetModules = completeKernelNetModules.replaceInStrings(QRegExp("[\\w|\\.|-]*/"), "");
    // Those three kernel modules are in the "misc" section we add them manually
    // To the filter list for convenience
    completeKernelNetModules << "ndiswrapper";
    completeKernelNetModules << "atl2";
    completeKernelNetModules << "wl";
    for (const QString &mod : loadedKernelModules) {
        if (completeKernelNetModules.contains(mod.left(mod.indexOf(' '))))
            loadedModules.append(mod.left(mod.indexOf(' '))); // add to the QStringList of loadedModule
    }

    for (int i = 0; i < loadedModules.size(); ++i) {
        QString mod = loadedModules.at(i);
        if (i == 0)
            new QListWidgetItem("---------" + tr("Loaded Drivers") + "-------------", linuxDrvList);
        new QListWidgetItem(mod, linuxDrvList);
    }

    // list unloaded modules
    for (int i = 0; i < unloadedModules.size(); ++i) {
        QString mod = unloadedModules.at(i);
        if (i == 0)
            new QListWidgetItem("---------" + tr("Unloaded Drivers") + "-----------", linuxDrvList);
        QListWidgetItem *unloaded = new QListWidgetItem(mod, linuxDrvList);
        unloaded->setForeground(Qt::blue);
    }

    QFile inputBlockList(QString("/etc/modprobe.d/blacklist.conf"));
    inputBlockList.open(QFile::ReadOnly | QFile::Text);

    QString driver;
    QString s;
    // add blocklisted modules to the list
    while (!inputBlockList.atEnd()) {
        if (inputBlockList.pos() == 0)
            new QListWidgetItem("---------" + tr("Blocked Drivers") + " --------", linuxDrvList);
        s = inputBlockList.readLine();
        QRegExp expr("^\\s*blacklist\\s*.*");
        if (expr.exactMatch(s)) {
            QString captured = expr.cap(0);
            captured.remove("blacklist");
            driver = captured.trimmed();
            QListWidgetItem *blocklisted = new QListWidgetItem(driver, linuxDrvList);
            blocklisted->setForeground(Qt::red);
            blockedModules.append(driver);
        }
    }
    inputBlockList.close();

    // add blocklisted modules from /etc/modprobe.d/broadcom-sta-dkms.conf
    QFile inputBroadcomBlocklist(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
    inputBroadcomBlocklist.open(QFile::ReadOnly | QFile::Text);
    while (!inputBroadcomBlocklist.atEnd()) {
        if (inputBroadcomBlocklist.pos() == 0)
            new QListWidgetItem("---------" + tr("Blocked Broadcom Drivers") + "--------", linuxDrvList);
        s = inputBroadcomBlocklist.readLine();
        QRegExp expr("^\\s*blacklist\\s*.*");
        if (expr.exactMatch(s)) {
            QString captured = expr.cap(0);
            captured.remove("blacklist");
            driver = captured.trimmed();
            QListWidgetItem *bloclisted = new QListWidgetItem(driver, linuxDrvList);
            bloclisted->setForeground(Qt::red);
            blockedModules.append(driver);
            broadcomModules.append(driver);
        }
    }
    inputBroadcomBlocklist.close();
}

void MainWindow::on_windowsDrvDiagnosePushButton_clicked()
{
    windowsDrvList->clear();
    if (system("[ -f /usr/sbin/ndiswrapper ]") != 0) {
        uninstallNdiswrapper->setVisible(false);
        QMessageBox::warning(this, windowTitle(), tr("Ndiswrapper is not installed"));
        return;
    }
    QStringList queryResult = cmd.getCmdOut("ndiswrapper -l 2>/dev/null").split("\n");

    if (queryResult.size() == 1 && queryResult.at(0).isEmpty())
        return;

    int i = 0;
    while (i < queryResult.size()) {
        QString currentElement = queryResult.at(i);
        QString label = currentElement.left(currentElement.indexOf(":"));
        label.append(tr("driver installed"));
        //label = currentElement.remove(": ");
        if ((i + 1) < queryResult.size()) {
            if (!queryResult.at(i + 1).contains(": driver installed")) {
                QString installInfo = queryResult.at(i + 1);
                int infoPos = installInfo.indexOf(QRegExp("[\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F]:[\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F][\\d|A|B|C|D|E|F]"));
                //device (14E4:4320) present (alternate driver: bcm43xx)
                if (infoPos != -1) {
                    label.append(tr(" and in use by "));
                    label.append(installInfo.midRef(infoPos, 9));
                }
                if (installInfo.contains("alternate driver")) {
                    infoPos = installInfo.lastIndexOf(": ");
                    if (infoPos != -1) {
                        int strLen = installInfo.length();
                        label.append(tr(". Alternate driver: "));
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
        new QListWidgetItem(label, windowsDrvList);
        i++;
    }
}

bool MainWindow::blockModule(QString module)
{
    QFile outputBlockList;
    if (!broadcomModules.contains(module))
        outputBlockList.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
    else
        outputBlockList.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));

    if (!outputBlockList.open(QFile::Append | QFile::Text))
        return false;

    outputBlockList.write(QString("blacklist %1\n").arg(module).toUtf8());
    outputBlockList.close();

    if (removable(module))
        if (!removeModule(module))
            return false;
    loadedModules.removeAll(module);
    return true;
}

void MainWindow::on_linuxDrvBlockPushButton_clicked()
{
    if (linuxDrvList->currentRow() != -1) {
        QListWidgetItem *currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if (driverBlocklisted) {
            QFile InputBlockList;
            QFile outputBlockList;
            if (broadcomModules.contains(driver)) {
                InputBlockList.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));
                outputBlockList.setFileName(QString("/etc/modprobe.d/broadcom-sta-dkms.conf"));

            } else {
                InputBlockList.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
                outputBlockList.setFileName(QString("/etc/modprobe.d/blacklist.conf"));
            }
            if (!InputBlockList.open(QFile::ReadOnly | QFile::Text))
                return;

            QString s, outputString("");
            while (!InputBlockList.atEnd()) {
                s = InputBlockList.readLine();
                QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
                if (!s.contains(QRegExp(expr)))
                    outputString += s;
                outputBlockList.write(s.toUtf8());
            }
            InputBlockList.close();
            if (!outputBlockList.open(QFile::WriteOnly | QFile::Text))
                return;
            outputBlockList.write(outputString.toUtf8());
            outputBlockList.close();
            QMessageBox::information(this, tr("Driver removed from blocklist"),
                                     tr("Driver removed from blocklist."));
            loadModule(driver);
            driverBlocklisted = false;
            unloadedModules.removeAll(driver);
            blockedModules.removeAll(driver);
        } else if (blockModule(driver)) {
            QMessageBox::information(this, tr("Module blocked"), tr("Module blocked"));
            driverBlocklisted = true;
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
    if (system(cmd.toUtf8()) != 0) {
        // run depmod and try to load again
        system("depmod");
        if (system(cmd.toUtf8()) != 0) {
            QString msg = QObject::tr("Could not load ");
            msg += module;
            QMessageBox::information(this, windowTitle(), msg);
            system("pkill wpa_supplicant");
            system("service network-manager start");
            return false;
        }
    }
    if (!loadedModules.contains(module))
        loadedModules.append(module);
    system("pkill wpa_supplicant");
    system("service network-manager start");
    return true;
}

// check if the module can be removed
bool MainWindow::removable(QString module)
{
    QString cmd = QString("modprobe -rn %1").arg(module);
    if (system(cmd.toUtf8()) != 0)
        return false;
    return true;
}


// remove module
bool MainWindow::removeModule(QString module)
{
    system("service network-manager stop");
    QString cmd = QString("modprobe -r %1").arg(module);
    if (system(cmd.toUtf8()) != 0) {
        QString msg = QObject::tr("Could not unload ");
        msg += module;
        QMessageBox::information(this, windowTitle(), msg);
        system("service network-manager start");
        return false;
    }
    system("service network-manager start");
    return true;
}

bool MainWindow::removeStart(QString module)
{
    QFile inputModules("/etc/modules");
    QFile outputModules("/etc/modules");
    if (!inputModules.open(QFile::ReadOnly | QFile::Text))
        return false;

    QString s, outputString;
    while (!inputModules.atEnd()){
        s = inputModules.readLine();
        QString expr = QString("^\\s*(%1)\\s*").arg(module);
        if (!s.contains(QRegExp(expr)))
            outputString += s;
        outputModules.write(s.toUtf8());
    }
    inputModules.close();
    if (!outputModules.open(QFile::WriteOnly | QFile::Text))
        return false;
    outputModules.write(outputString.toUtf8());
    outputModules.close();
    return true;
}


// install Linux Driver
bool MainWindow::installModule(QString module)
{
    if (!loadModule(module))
        return false;
    QFile outputModules("/etc/modules");
    if (!outputModules.open(QFile::Append | QFile::Text))
        return false;
    outputModules.write(QString("%1\n").arg(module).toUtf8());
    outputModules.close();
    return true;
}

// run apt-get update and at the end start installNDIS
void MainWindow::on_installNdiswrapper_clicked()
{
    setCursor(QCursor(Qt::BusyCursor));
    if (installProc->state() != QProcess::NotRunning)
        installProc->kill();
    installProc->start("apt-get", {"update"});
    installOutputEdit->clear();
    installOutputEdit->show();
    installOutputEdit->resize(800, 600);
    // center output window
    QRect screenGeometry = QApplication::screens().first()->geometry();
    int x = (screenGeometry.width()-installOutputEdit->width()) / 2;
    int y = (screenGeometry.height()-installOutputEdit->height()) / 2;
    installOutputEdit->move(x, y);
    // hide main window
    this->hide();
    installOutputEdit->raise();
    disconnect(installProc, &QProcess::readyReadStandardOutput, nullptr, nullptr);
    connect(installProc, &QProcess::readyReadStandardOutput, this, &MainWindow::writeInstallOutput);
    disconnect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), nullptr, nullptr);
    connect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::aptUpdateFinished);
}


// Uninstall ndiswrapper
void MainWindow::on_uninstallNdiswrapper_clicked()
{
    setCursor(QCursor(Qt::BusyCursor));

    if (installProc->state() != QProcess::NotRunning)
        installProc->kill();
    removeModule("ndiswrapper");
    installProc->start("apt-get", {"purge", "-y", "ndiswrapper-utils-1.9", "ndiswrapper-dkms", "ndiswrapper-common"});
    installOutputEdit->clear();
    installOutputEdit->show();
    installOutputEdit->resize(800, 600);
    // center output window
    QRect screenGeometry = QApplication::screens().first()->geometry();
    int x = (screenGeometry.width()-installOutputEdit->width()) / 2;
    int y = (screenGeometry.height()-installOutputEdit->height()) / 2;
    installOutputEdit->move(x, y);
    // hide main window
    this->hide();
    installOutputEdit->raise();
    disconnect(installProc, &QProcess::readyReadStandardOutput, nullptr, nullptr);
    connect(installProc, &QProcess::readyReadStandardOutput, this, &MainWindow::writeInstallOutput);
    disconnect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), nullptr, nullptr);
    connect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::uninstallNdisFinished);
}

// install NDISwrapper
void MainWindow::aptUpdateFinished()
{
    if (installProc->state() != QProcess::NotRunning)
        installProc->kill();
    installProc->start("apt-get", {"install", "-y", "ndiswrapper-utils-1.9", "ndiswrapper-dkms"});
    disconnect(installProc, &QProcess::readyReadStandardOutput, nullptr, nullptr);
    connect(installProc, &QProcess::readyReadStandardOutput, this, &MainWindow::writeInstallOutput);
    disconnect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), nullptr, nullptr);
    connect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::installFinished);
}

// finished ndiswrapper install
void MainWindow::installFinished(int errorCode)
{
    installOutputEdit->close();
    this->show();
    setCursor(QCursor(Qt::ArrowCursor));
    if (errorCode == 0) {
        if (installModule("ndiswrapper")) {
            uninstallNdiswrapper->setVisible(true);
            QMessageBox::information(this, windowTitle(), tr("Installation successful"));
        }
        else {
            QMessageBox::information(this, windowTitle(), tr("Error detected, could not compile ndiswrapper driver."));
        }
    } else {
        QMessageBox::warning(this, windowTitle(), tr("Error detected, could not install ndiswrapper."));
    }
}

void MainWindow::uninstallNdisFinished(int errorCode)
{
    installOutputEdit->close();
    this->show();
    setCursor(QCursor(Qt::ArrowCursor));
    if (errorCode == 0)
        removeStart("ndiswrapper");
    else
        QMessageBox::warning(this, windowTitle(), tr("Error encountered while removing Ndiswrapper"));
}

void MainWindow::writeInstallOutput()
{
    QByteArray bytes = installProc->readAllStandardOutput();
    const QStringList lines = QString(bytes).split("\n");

    for (const QString &line : lines)
        if (!line.isEmpty())
            installOutputEdit->append(line);
}

void MainWindow::updateDriverStatus()
{
    driverBlocklisted = false;
    QFile InputBlockList("/etc/modprobe.d/blacklist.conf");
    QFile inputBroadcomBlocklist("/etc/modprobe.d/broadcom-sta-dkms.conf");
    InputBlockList.open(QFile::ReadOnly | QFile::Text);
    inputBroadcomBlocklist.open(QFile::ReadOnly | QFile::Text);

    QString driver;

    if (linuxDrvList->currentRow() != -1) {
        QListWidgetItem *currentDriver = linuxDrvList->currentItem();
        driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
    }

    QString s;
    while (!InputBlockList.atEnd()) {
        s = InputBlockList.readLine();
        QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
        if (s.contains(QRegExp(expr))) {
            driverBlocklisted = true;
            break;
        }
    }
    while (!inputBroadcomBlocklist.atEnd()) {
        s = inputBroadcomBlocklist.readLine();
        QString expr = QString("^\\s*(blacklist)\\s*(%1)\\s*").arg(driver);
        if (s.contains(QRegExp(expr))) {
            driverBlocklisted = true;
            break;
        }
    }
    if (driverBlocklisted) {
        linuxDrvBlockPushButton->setText(tr("Unblock Driver"));
        linuxDrvBlockPushButton->setIcon(QIcon::fromTheme("object-unlocked"));
    } else {
        linuxDrvBlockPushButton->setText(tr("Block Driver"));
        linuxDrvBlockPushButton->setIcon(QIcon::fromTheme("object-locked"));
    }
    InputBlockList.close();
}

bool MainWindow::checkSysFileExists(QDir searchPath, QString fileName, Qt::CaseSensitivity cs)
{
    QStringList fileList = searchPath.entryList(QStringList() << "*.SYS");
    bool found = false;
    auto it = fileList.constBegin();
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
    if (cmd.getCmdOut("nmcli -t --fields WIFI r") == "enabled") {
        labelWifi->setText(tr("enabled"));
        return true;
    } else if (cmd.getCmdOut("nmcli -t --fields WIFI-HW r") == "enabled") {
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
    if (!infFileName.isEmpty()) {
        // Search in the inf file the name of the .sys file
        QFile infFile(infFileName);
        infFile.open(QFile::ReadOnly | QFile::Text);

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
                if (this->checkSysFileExists(sysDir, s, Qt::CaseInsensitive))
                    exist = true;
                else
                    foundSysFiles << s;
            }
        }
        infFile.close();

        if (found) {
            if (!exist) {
                QMessageBox::warning(this, QString(tr("*.sys file not found")), tr("The *.sys files must be in the same location as the *.inf file. %1 cannot be found").arg(foundSysFiles.join(", ")));
            } else {
                QString cmd_str = QString("ndiswrapper -i %1").arg(infFileName);
                cmd.run(cmd_str);
                cmd_str = QString("ndiswrapper -ma");
                cmd.run(cmd_str);
                on_windowsDrvDiagnosePushButton_clicked();
            }
        } else {
            QMessageBox::critical(this, QString(tr("sys file reference not found")).arg(infFile.fileName()),
                                  tr("The sys file for the given driver cannot be determined after parsing the inf file"));
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
        QListWidgetItem *currentDriver = windowsDrvList->currentItem();
        QString driver = currentDriver->text();
        QString cmd_str = QString("ndiswrapper -r %1").arg(driver.left(driver.indexOf(" ")));
        cmd.run(cmd_str);
        QMessageBox::information(this, windowTitle(), tr("Ndiswrapper driver removed."));
        on_windowsDrvDiagnosePushButton_clicked();
    }
}

void MainWindow::on_generalHelpPushButton_clicked()
{
    QString url = "/usr/share/doc/mx-network-assistant/help/mx-network-assistant.html";
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), true);
}

void MainWindow::on_tabWidget_currentChanged()
{
    refresh();
}

// unblock Wifi devices
void MainWindow::on_hwUnblock_clicked()
{
    if (system("rfkill unblock wlan wifi") != 0)
        QMessageBox::warning(this, windowTitle(), tr("Could not unlock devices.\nWiFi device(s) might already be unlocked."));
    else
        QMessageBox::information(this, windowTitle(), tr("WiFi devices unlocked."));
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
    displayAboutMsgBox(tr("About %1").arg(this->windowTitle()), "<p align=\"center\"><b><h2>" + this->windowTitle() +"</h2></b></p><p align=\"center\">" +
                       tr("Version: ") + VERSION + "</p><p align=\"center\"><h3>" +
                       tr("Program for troubleshooting and configuring network for MX Linux") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MEPIS LLC and MX Linux") + "<br /><br /></p>",
                       "/usr/share/doc/mx-network-assistant/license.html", tr("%1 License").arg(this->windowTitle()), true);
    this->show();
}


QString MainWindow::getIP()
{
    return cmd.getCmdOut("wget -qO - icanhazip.com");
}

QString MainWindow::getIPfromRouter()
{
    return cmd.getCmdOut("ip a | awk '/127.0.0.1/ {next} /inet / {print $2}' | cut -d/ -f1");
}

void MainWindow::on_linuxDrvLoad_clicked()
{
    if (linuxDrvList->currentRow() != -1) {
        QListWidgetItem *currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if (loadModule(driver)) {
            unloadedModules.removeAll(driver);
            QMessageBox::information(this, tr("Driver loaded successfully"), tr("Driver loaded successfully"));
            on_linuxDrvDiagnosePushButton_clicked();
        }
    }
}

void MainWindow::on_linuxDrvUnload_clicked()
{
    if (linuxDrvList->currentRow() != -1) {
        QListWidgetItem *currentDriver = linuxDrvList->currentItem();
        QString driver = currentDriver->text();
        driver = driver.left(driver.indexOf(" "));
        if ((removable(driver)) && !unloadedModules.contains(driver)) {
            if (removeModule(driver)) {
                unloadedModules.append(driver);
                QMessageBox::information(this, tr("Driver unloaded successfully"), tr("Driver unloaded successfully"));
                on_linuxDrvDiagnosePushButton_clicked();
            }
        }
    }
}
