#include "silencer.h"
#include "ui_silencer.h"
#include <unistd.h>

#include <QDebug>

#include <QProcess>
#include <QRect>
#include <QMessageBox>
#include <QDir>


Silencer::Silencer(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setWindowTitle(APP_NAME);

    currentAction = '\0';
    mutedIcon = new QIcon(":/icons/no_sound.png");
    unmutedIcon = new QIcon(":/icons/sound.png");

    timeFrom = nullptr;
    timeUntil = nullptr;

    setWindowFlags(Qt::Tool | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);

    createActions();
    createTrayIcon();

    trayIcon->show();
    bringCloseToTray();

    connect(ui->btn_apply, &QPushButton::clicked, this, &Silencer::onApplyClicked);
    connect(ui->btn_enable, &QPushButton::clicked, this, &Silencer::onEnableClicked);
    connect(&timer, &QTimer::timeout, this, &Silencer::onTimerTick, Qt::QueuedConnection);

    _settings = loadSettingsFile();
}

Silencer::~Silencer()
{
    delete ui;
    delete _settings; // this will automatically call sync()
    delete timeFrom;
    delete timeUntil;

    delete trayIcon;
    delete trayIconMenu;

    delete enableAction;
    delete restoreAction;
    delete quitAction;

    delete mutedIcon;
    delete unmutedIcon;
}

void Silencer::onQuitClicked()
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, APP_NAME, tr("Want to quit?"));
    if (resBtn == QMessageBox::Yes)
    {
        QCoreApplication::quit();
    }
}

void Silencer::onRestoreClicked()
{
    this->show();
    this->raise();
}

void Silencer::onApplyClicked()
{
    timeFrom = new QTime(ui->t_from->time());
    timeUntil = new QTime(ui->t_until->time());

    _settings->setValue("from", timeFrom->toString());
    _settings->setValue("until", timeUntil->toString());
    _settings->setValue("interval", ui->spn_interval->value());
    _settings->setValue("enabled", ui->btn_enable->isChecked());
    _settings->setValue("force_update", ui->chk_forceMute->isChecked());

    qDebug() << "Settings updated!";
}


char Silencer::_getMuteAction()
{
    QDateTime now = QDateTime::currentDateTime();
    QTime timeNow = now.time();

    QTime _timeFrom(*timeFrom);
    QTime _timeUntil(*timeUntil);

    char newAction = '\0';

    if (_timeFrom > _timeUntil)
    {
        // Overnight
        if (timeNow < _timeUntil)
        {
            // next is timeUntil
            newAction = 'M';
        }
        else if (timeNow < _timeFrom)
        {
            // next is timeFrom
            newAction = 'U';
        }
        else // if (timeNow > timeFrom)
        {
            // next is timeUntil next day
            newAction = 'M';
        }
    }
    else
    {
        // Same date
        if (timeNow < _timeFrom)
        {
            // next is timeFrom
            newAction = 'U';
        }
        else if (timeNow < _timeUntil)
        {
            // next is timeUntil
            newAction = 'M';
        }
        else // if (timeNow > timeUntil)
        {
            // next is timeFrom next day
            newAction = 'U';
        }
    }

    return newAction;
}

void Silencer::onTimerTick()
{

    if (timeFrom == nullptr || timeUntil == nullptr)
    {
        qDebug() << "Times are not set yet.";
        return;
    }

    char newAction = _getMuteAction();

    // Don't update when Force Update is disabled and the states are the same
    if (!ui->chk_forceMute->isChecked() && newAction == currentAction)
    {
        qDebug() << "States are the same. No need to change anything.";
        return;
    }

    currentAction = newAction;

    if (currentAction == 'M')
    {
        qDebug() << "Muting the sounds.";
        this->mute();
    }
    else
    {
        qDebug() << "Unmuting the sounds.";
        this->unmute();
    }
}

void Silencer::onEnableClicked(bool checked)
{
    if (!checked)
    {
        enableAction->setText("Enable");
        ui->btn_enable->setText("Off");
        ui->btn_enable->repaint();
        this->timer.stop();
    }
    else
    {
        enableAction->setText("Disable");
        ui->btn_enable->setText("On");
        ui->btn_enable->repaint();
        this->timer.start();
    }
}

void Silencer::onEnableMenuClicked()
{
    ui->btn_enable->click();
}

void Silencer::createActions()
{
    enableAction = new QAction(tr("&Enable"), this);
    connect(enableAction, &QAction::triggered, this, &Silencer::onEnableMenuClicked);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &Silencer::onRestoreClicked);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, this, &Silencer::onQuitClicked);
}

void Silencer::bringCloseToTray()
{
    QRect trayPos = trayIcon->geometry();
    int width = trayPos.width();
    trayPos.moveLeft(trayPos.bottomLeft().x() - width);
    this->move(trayPos.bottomLeft());
}

void Silencer::_mute(bool mute)
{
    QString command = QString("-e set volume output muted ") + (mute ? "true": "false");
    QProcess::execute("osascript", QStringList() << command);

    if (mute)
    {
        trayIcon->setIcon(*mutedIcon);
        trayIcon->showMessage(APP_NAME, "Hushhh... Your computer is silent now!");
    }
    else
    {
        trayIcon->setIcon(*unmutedIcon);
        trayIcon->showMessage(APP_NAME, "Your computer came out of silence!");
    }
}

QSettings *Silencer::loadSettingsFile()
{
    QDir home = QDir::home();
    QString homePath = QDir::homePath();
    QString settingsPath = QString("%1%2%3").arg(homePath).arg(QDir::separator()).arg(APP_SETTINGS_DIR);

    QDir settingsFolder(settingsPath);
    if (!settingsFolder.exists())
    {
        home.mkdir(settingsPath);

    }
    QString settingsFile = QString("%1%2%3").arg(settingsPath).arg(QDir::separator()).arg("settings.ini");

    qDebug() << "Going to load " << settingsFile;

    return new QSettings(settingsFile, QSettings::IniFormat);
}

void Silencer::updateValues()
{
    QString s_timeFrom = _settings->value("from", ui->t_from->time().toString()).toString();
    QString s_timeUntil = _settings->value("until", ui->t_until->time().toString()).toString();
    int i_interval = _settings->value("interval", 5).toInt();
    bool b_enabled = _settings->value("enabled", true).toBool();
    bool b_forceUpdate = _settings->value("force_update", false).toBool();

    ui->t_from->setTime(QTime::fromString(s_timeFrom));
    ui->t_until->setTime(QTime::fromString(s_timeUntil));
    ui->spn_interval->setValue(i_interval);
    ui->chk_forceMute->setChecked(b_forceUpdate);

    timer.setInterval(i_interval * 1000);

    ui->btn_enable->setChecked(b_enabled);
    this->onEnableClicked(b_enabled);

    timeFrom = new QTime(ui->t_from->time());
    timeUntil = new QTime(ui->t_until->time());
}


void Silencer::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(enableAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    QIcon icon(":/icons/no_sound.png");
    trayIcon->setIcon(icon);
//    trayIconMenu->setAsDockMenu();
}

void Silencer::closeEvent (QCloseEvent *event)
{
    this->onApplyClicked();
    this->hide();
    event->ignore();
}

void Silencer::showEvent(QShowEvent *event)
{
    updateValues();
    QDialog::showEvent(event);
}


