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

//   With big modification made by Adrian adrian@mxlinux.org

#ifndef MCONFIG_H
#define MCONFIG_H

#include "ui_meconfig.h"
#include <QMessageBox>
#include <QProcess>
#include <QDir>

class MConfig : public QDialog, public Ui::MEConfig {
    Q_OBJECT
public:   
    MConfig(QWidget* parent = 0);
    ~MConfig();           
    // helpers
    static QString getCmdOut(QString cmd);
    static QString getCmdOut2(QString cmd);
    static QStringList getCmdOuts(QString cmd);
    static QString getCmdValue(QString cmd, QString key, QString keydel, QString valdel);
    static QStringList getCmdValues(QString cmd, QString key, QString keydel, QString valdel);
    static bool replaceStringInFile(QString oldtext, QString newtext, QString filepath);
    static QString getVersion(QString name);
    static QString getIP();
    static QString getIPfromRouter();
    // common
    void refresh();
    // special
    void refreshStatus();
    bool checkSysFileExists(QDir searchPath, QString fileName, Qt::CaseSensitivity cs);
public slots:
    virtual void show();

    virtual void on_tabWidget_currentChanged();
    virtual void on_buttonCancel_clicked();
    virtual void on_buttonAbout_clicked();

    virtual void on_generalHelpPushButton_clicked();
    virtual void on_hwDiagnosePushButton_clicked();
    virtual void on_linuxDrvDiagnosePushButton_clicked();
    virtual void on_windowsDrvDiagnosePushButton_clicked();
    virtual void on_linuxDrvList_currentRowChanged(int currentRow );
    virtual void on_linuxDrvBlacklistPushButton_clicked();
    virtual void on_windowsDrvAddPushButton_clicked() ;
    virtual void on_windowsDrvRemovePushButton_clicked();
    virtual void on_clearPingOutput_clicked();
    virtual void on_clearTraceOutput_clicked();
    virtual void on_tracerouteButton_clicked();
    virtual void on_pingButton_clicked();
    virtual void on_cancelPing_clicked();
    virtual void on_cancelTrace_clicked();
    virtual void writePingOutput();
    virtual void writeTraceOutput();
    virtual void writeInstallOutput();
    virtual void pingFinished();
    virtual void tracerouteFinished();
    virtual void aptUpdateFinished();
    virtual void installFinished(int);
    virtual void uninstallNdisFinished(int);
    virtual void on_windowsDrvList_currentRowChanged(int row);

    virtual void hwListToClipboard();
    virtual void hwListFullToClipboard();

    virtual void linuxDrvListToClipboard();
    virtual void linuxDrvListFullToClipboard();

    virtual void windowsDrvListToClipboard();
    virtual void windowsDrvListFullToClipboard();

    virtual void showContextMenuForHw(const QPoint &pos);
    virtual void showContextMenuForLinuxDrv(const QPoint &pos);
    virtual void showContextMenuForWindowsDrv(const QPoint &pos);

protected:
    /*$PROTECTED_FUNCTIONS$*/
    QTextEdit *installOutputEdit;

    void updateDriverStatus();
    bool loadModule(QString module);
    bool removeModule(QString module);
    bool removeStart(QString module);
    bool removable(QString module);
    bool configurationChanges[5];
    int currentTab;
    bool blacklistModule(QString module);
    bool installModule(QString module);
    bool internetConnection;
    bool ndiswrapBlacklisted;
    bool driverBlacklisted;
    QStringList loadedModules;
    QStringList unloadedModules;
    QStringList blacklistedModules;
    QStringList broadcomModules;

    QProcess *pingProc;
    QProcess *traceProc;
    QProcess *installProc;

protected slots:
    /*$PROTECTED_SLOTS$*/

private slots:
    void on_installNdiswrapper_clicked();
    void on_uninstallNdiswrapper_clicked();
    void on_hwUnblock_clicked();
    void on_linuxDrvLoad_clicked();
    void on_linuxDrvUnload_clicked();
};

#endif

