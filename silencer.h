#ifndef SILENCER_H
#define SILENCER_H

#include <QDialog>

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QDateTime>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE


#define APP_NAME "Hush My Mac"
#define APP_SETTINGS_DIR ".hush_my_mac"

class Silencer : public QDialog
{
    Q_OBJECT

public:
    Silencer(QWidget *parent = nullptr);
    ~Silencer();

private slots:
    void onQuitClicked();
    void onApplyClicked();
    void onTimerTick();

    void onEnableClicked(bool);
    void onEnableMenuClicked();

protected:
    void closeEvent(QCloseEvent*);
    void showEvent(QShowEvent *event);

private:
    Ui::Dialog *ui;
    QSettings* _settings;
    bool isMute;

    QTimer timer;
    char currentAction;

    QTime* timeFrom;
    QTime* timeUntil;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QAction *enableAction;
    QAction *restoreAction;
    QAction *quitAction;

    QIcon*mutedIcon;
    QIcon* unmutedIcon;

    void createTrayIcon();
    void createActions();
    void bringCloseToTray();

    void mute(){ this->_mute(true); }
    void unmute(){ this->_mute(false); }
    void _mute(bool);

    QSettings* loadSettingsFile();
    void updateValues();

    char _getMuteAction();
};
#endif // SILENCER_H
