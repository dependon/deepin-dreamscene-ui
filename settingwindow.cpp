#include "settingwindow.h"
#include "ui_settingwindow.h"
#include <QSystemTrayIcon>
#include <QFileDialog>
#include <QMenu>
#include <DMainWindow>
#include <QProcess>
#include <QThread>
#include <QSettings>
#include <DDesktopEntry>
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QMimeData>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "listview/historywidget.h"
DCORE_USE_NAMESPACE

#define SETTINGPATH "config.ini"
const QString CONFIG_PATH =   QDir::homePath() +
                              "/.config/deepin-dreamscene-ui/config.ini";
settingWindow::settingWindow(QWidget *parent, DMainWindow *mainWindow) :
    QWidget(parent),
    m_parentMainWindow(mainWindow),
    ui(new Ui::settingWindow)
{
    //    saveSettings();

    ui->setupUi(this);
    /*qApp->*/installEventFilter(this);
    ui->pathEdit->installEventFilter(this);
    ui->pathEdit->setAcceptDrops(true);
    setAcceptDrops(true);
    readSettings();
    m_traymenu = new QMenu();
    QAction *exitAction = new QAction(m_traymenu);
    exitAction->setText("退出");
    connect(exitAction, &QAction::triggered, this, &settingWindow::quitApp);
    connect(dApp, &Application::quitApp, this, &settingWindow::quitApp, Qt::DirectConnection);

    QAction *setMpvPlayAction = new QAction(m_traymenu);
    setMpvPlayAction->setText("播放");
    connect(setMpvPlayAction, &QAction::triggered, this, [ = ] {
        emit dApp->setMpvPlay();
        m_isNoMpvPause = true;
    });

    QAction *setMpvpauseAction = new QAction(m_traymenu);
    setMpvpauseAction->setText("暂停");
    connect(setMpvpauseAction, &QAction::triggered, this, [ = ] {
        emit dApp->setMpvpause();
        m_isNoMpvPause = false;
    });

    QAction *setHistoryAction = new QAction(m_traymenu);
    setHistoryAction->setText("历史壁纸");
    connect(setHistoryAction, &QAction::triggered, this, &settingWindow::on_history_clicked);

    QAction *setMainViewAction = new QAction(m_traymenu);
    setMainViewAction->setText("主界面");
    connect(setMainViewAction, &QAction::triggered, this, [ = ] {
        if (m_parentMainWindow)
        {
            m_parentMainWindow->show();
            m_parentMainWindow->activateWindow();
        }
    });

    m_traymenu->addAction(setMainViewAction);
    m_traymenu->addAction(setHistoryAction);
    m_traymenu->addAction(setMpvPlayAction);
    m_traymenu->addAction(setMpvpauseAction);
    m_traymenu->addAction(exitAction);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_traymenu);
    m_trayIcon->setToolTip("双击打开配置界面");
    m_trayIcon->setIcon(QIcon(":/tray.ico"));
    m_trayIcon->show();

    //void activated(QSystemTrayIcon::ActivationReason reason);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [ = ](QSystemTrayIcon::ActivationReason reason) {
        if (QSystemTrayIcon::Trigger == reason) {
            if (m_parentMainWindow) {
                m_parentMainWindow->show();
                m_parentMainWindow->activateWindow();
            }
        }
    });

    connect(dApp, &Application::pathChanged, this, &settingWindow::pathChanged);
    connect(dApp, &Application::setWallPaper, this, &settingWindow::slotWallPaper);

    connect(dApp, &Application::saveSetting, this, &settingWindow::saveSettings);

    ui->bugBtn->hide();
    ui->mainWeb->hide();
    ui->githubWeb->hide();
    ui->giteeWeb->hide();

}
void settingWindow::pathChanged(const QString &path)
{
    m_currentPath = path;
    if (!m_currentPath.isEmpty()) {
        ui->pathEdit->setText(m_currentPath);
        QPixmap pix = dApp->getThumbnail(m_currentPath);
        if (!pix.isNull()) {
            ui->pixThumbnail->setPixmap(pix);
        }
    }
}
settingWindow::~settingWindow()
{
    if (m_history) {
        m_history->deleteLater();
        m_history = nullptr;
    }
    delete ui;
}

void settingWindow::readSettings()
{
    QSettings settings(CONFIG_PATH, QSettings::IniFormat);

    m_currentPath = settings.value("WallPaper/CurrentPath").toString();
    m_crrenNumber = settings.value("WallPaper/ScrrenNumber").toInt(); //1-2
    m_isAutoStart = settings.value("WallPaper/isAutoStart").toInt();
    int widthPY = settings.value("WallPaper/widthPY").toInt();
    int heightPY = settings.value("WallPaper/heightPY").toInt();
    int width = settings.value("WallPaper/width").toInt();
    int height = settings.value("WallPaper/height").toInt();
    m_currentMode = settings.value("WallPaper/Mode").toString();
    m_voiceVolume = settings.value("WallPaper/voiceVolume").toInt();
    m_videoAspect = settings.value("WallPaper/videoAspect").toDouble();
    m_videoASpectStr = settings.value("WallPaper/videoAspectStr").toString();
    m_isAutoMode = settings.value("WallPaper/videoAutoMode").toInt();;
    ui->videoBLEdit->setText(QString::number(m_videoAspect));
    ui->videoBLCombox->setCurrentText(m_videoASpectStr);

    //取值本地地址
    QString strLocalPath;
    int localIndex = 1;
    do {
        strLocalPath = settings.value("Movie/localPath" + QString::number(localIndex++)).toString();
        if (nullptr != strLocalPath) {
            if (!dApp->m_allPath.contains(strLocalPath)) {
                dApp->m_allPath.push_back(strLocalPath);
                emit dApp->addPaperView(strLocalPath);
            }
        }
    } while (nullptr != strLocalPath);

    dApp->m_manual.setRect(widthPY, heightPY, width, height);
    if (!m_currentMode.isEmpty()) {
        ui->comboBox->setCurrentText(m_currentMode);
        setScreenMode(m_currentMode);
    }
    if (!m_currentPath.isEmpty()) {
        ui->pathEdit->setText(m_currentPath);
        QPixmap pix = dApp->getThumbnail(m_currentPath);
        if (!pix.isNull()) {
            ui->pixThumbnail->setPixmap(pix);
        }
    }

    QTimer::singleShot(300, [ = ] {
        ui->widthPY->setText(QString::number(dApp->m_manual.x()));
        ui->heightPY->setText(QString::number(dApp->m_manual.y()));
        ui->width->setText(QString::number(dApp->m_manual.width()));
        ui->height->setText(QString::number(dApp->m_manual.height()));

        if (m_voiceVolume >= 0 && m_voiceVolume < 100)
        {
            ui->Slider->setValue(m_voiceVolume);
        }
        if (!m_currentMode.isEmpty())
        {
            ui->comboBox->setCurrentText(m_currentMode);
            setScreenMode(m_currentMode);
        }
        if (m_crrenNumber > 1)
        {
            //        ui->autoisMScreen->setCheckState(Qt::Checked);
        }
        if (m_isAutoStart > 0)
        {
            ui->autoStartBox->setCheckState(Qt::Checked);
        }

        on_videoBLCombox_activated(m_videoASpectStr);


    });
    QTimer::singleShot(2000, [ = ] {
        if (m_isAutoMode == 1)
        {
            ui->checkBox->setCheckState(Qt::Checked);
        }

        on_checkBox_stateChanged(m_isAutoMode);
    });

    qDebug() << "x";
}

void settingWindow::saveSettings()
{
    QSettings settings(CONFIG_PATH, QSettings::IniFormat);
    settings.clear();
    settings.setValue("WallPaper/ScrrenNumber", m_crrenNumber);
    settings.setValue("WallPaper/isAutoStart", m_isAutoStart);
    settings.setValue("WallPaper/CurrentPath", m_currentPath);
    settings.setValue("WallPaper/Mode", ui->comboBox->currentText());
    settings.setValue("WallPaper/widthPY", dApp->m_manual.x());
    settings.setValue("WallPaper/heightPY", dApp->m_manual.y());
    settings.setValue("WallPaper/width", dApp->m_manual.width());
    settings.setValue("WallPaper/height", dApp->m_manual.height());
    settings.setValue("WallPaper/voiceVolume", m_voiceVolume);
    settings.setValue("WallPaper/videoAspect", m_videoAspect);
    settings.setValue("WallPaper/videoAspectStr", m_videoASpectStr);
    settings.setValue("WallPaper/videoAutoMode", m_isAutoMode);

    int indexLocal = 1;
    //去重
    dApp->m_allPath = dApp->m_allPath.toSet().toList();
    for (QString str : dApp->m_allPath) {
        settings.setValue("Movie/localPath" + QString::number(indexLocal++), str);
    }
}

QString settingWindow::getCurrentPath()
{
    return m_currentPath;
}

int settingWindow::getCurrentNumber()
{
    return m_crrenNumber;
}

int settingWindow::isAutoStart()
{
    return m_isAutoStart;
}

void settingWindow::setScreenMode(const QString &arg)
{
    emit dApp->setScreenMode(arg);
}

void settingWindow::on_pathBtn_clicked()
{
    QString path = QFileDialog::getOpenFileName();
    if (!path.isEmpty()) {
        ui->pathEdit->setText(path);
        QPixmap pix = dApp->getThumbnail(path);
        if (!pix.isNull()) {
            ui->pixThumbnail->setPixmap(pix);
        }
    }
}

void settingWindow::on_setBtn_clicked()
{
    if (ui->pathEdit->text() != nullptr) {
        m_currentPath = ui->pathEdit->text();
        m_currentPath = m_currentPath.replace("file://", "");
        emit dApp->setPlayPath(ui->pathEdit->text());
        emit dApp->setMpvPlay();
        m_isNoMpvPause = true;
        dApp->m_allPath.push_back(m_currentPath);
        saveSettings();
        emit dApp->addPaperView(m_currentPath);
        QPixmap pix = dApp->getThumbnail(m_currentPath);
        if (!pix.isNull()) {
            ui->pixThumbnail->setPixmap(pix);
        }
    }
}

void settingWindow::on_cancelBtn_clicked()
{
    if (m_parentMainWindow) {
        m_parentMainWindow->close();
    }
}

void settingWindow::on_pauseBtn_clicked()
{
    emit dApp->setMpvpause();
    m_isNoMpvPause = false;
}

void settingWindow::on_stopBtn_clicked()
{
    emit dApp->setMpvstop();
    m_isNoMpvPause = false;
}

void settingWindow::on_Slider_valueChanged(int value)
{
    emit dApp->setMpvVolume(value);
    m_voiceVolume = value;

    saveSettings();
}

void settingWindow::on_startBtn_clicked()
{
    emit dApp->setMpvPlay();
    m_isNoMpvPause = true;
}

void settingWindow::on_startScreen_clicked()
{
    if (index == 1) {
        index = 2;
    } else {
        index = 1;
    }
    emit dApp->setScreen(index);
    on_setBtn_clicked();
    on_startBtn_clicked();

}

void settingWindow::on_autoStart_clicked()
{

    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/deepin-dreamscene/";
    if (!QFileInfo(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/autostart/deepin-dreamscene-ui.desktop").isFile())
        QProcess::execute("cp /opt/durapps/deepin-dreamscene-ui/deepin-dreamscene-ui.desktop " + QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/autostart/");
}

void settingWindow::on_noAutoStart_clicked()
{
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/autostart/deepin-dreamscene-ui.desktop");
}

void settingWindow::on_autoStartBox_stateChanged(int arg1)
{
    if (arg1 == 0) {
        m_isAutoStart = 0;
        on_noAutoStart_clicked();
    } else {
        m_isAutoStart = 1;
        on_autoStart_clicked();
    }
    saveSettings();
}

void settingWindow::on_comboBox_activated(const QString &arg1)
{
    setScreenMode(arg1);
    saveSettings();
}

void settingWindow::on_setManual_clicked()
{
    int widthPY = ui->widthPY->text().toInt();
    int heightPY = ui->heightPY->text().toInt();
    int width = ui->width->text().toInt();
    int height = ui->height->text().toInt();
    dApp->m_manual.setRect(widthPY, heightPY, width, height);
    if (ui->comboBox->currentText() == "手动设置尺寸") {
        emit dApp->sigupdateGeometry();
    }
    saveSettings();
}

void settingWindow::quitApp()
{
#ifdef QT_NO_DEBUG
    QProcess::execute("killall dde-desktop");
    if (0 != dApp->m_processId) {
        QProcess::execute("kill " + QString::number(dApp->m_processId));
    }
    QThread *th = QThread::create([ = ]() {
        QProcess::execute("dde-desktop");
    });
    th->start();
    saveSettings();
#endif
    //dbus关闭壁纸透明
    system("qdbus --literal com.deepin.dde.desktop /com/deepin/dde/desktop com.deepin.dde.desktop.EnableBackground true");
    m_stopx11Thread = true;
    if (m_x11thread) {
//        m_x11thread->wait();
        m_x11thread->terminate();
    }
    dApp->exit();
}



void settingWindow::on_mainWeb_clicked()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/dependon/deepin-dreamscene-ui")));
}

void settingWindow::on_history_clicked()
{
    if (!m_history) {
        m_history = new historyWidget();
        m_history->showNormal();
        m_history->move(qApp->desktop()->screen()->rect().center() - m_history->rect().center());
    } else {
        m_history->showNormal();
        m_history->activateWindow();
    }
}

void settingWindow::slotWallPaper(const QString &path)
{
    if (!path.isEmpty()) {
        ui->pathEdit->setText(path);
        m_currentPath = path;
        emit dApp->setPlayPath(ui->pathEdit->text());
        QPixmap pix = dApp->getThumbnail(path);
        if (!pix.isNull()) {
            ui->pixThumbnail->setPixmap(pix);
        }
        emit dApp->setMpvPlay();
        m_isNoMpvPause = true;
        dApp->m_allPath.push_back(m_currentPath);
        dApp->m_allPath = dApp->m_allPath.toSet().toList();
        saveSettings();
    }
}


void settingWindow::on_giteeWeb_clicked()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://gitee.com/liuminghang/deepin-dreamscene-ui/releases")));
}

void settingWindow::on_githubWeb_clicked()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/dependon/deepin-dreamscene-ui/releases")));
}

void settingWindow::on_bugBtn_clicked()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/dependon/deepin-dreamscene-ui/issues/new")));
}

void settingWindow::on_videoBLCombox_activated(const QString &arg1)
{
    ui->videoBLEdit->setVisible(false);
    ui->videoBZDY->setVisible(false);
    double value = -1.0;
    if (arg1.contains("默认")) {
        value = -1.0;
    } else if (arg1.contains("4:3")) {
        value = 1.33333333;
    } else if (arg1.contains("16:9")) {
        value = 1.77777777;
    } else if (arg1.contains("16:10")) {
        value = 1.6;
    } else if (arg1.contains("1.85:1")) {
        value = 1.85;
    } else if (arg1.contains("2.35:1")) {
        value = 2.35;
    } else if (arg1.contains("自定义")) {
        ui->videoBLEdit->setVisible(true);
        ui->videoBZDY->setVisible(true);
        value = ui->videoBLEdit->text().toDouble();
    }
    ui->videoBLEdit->setText(QString::number(value));
    m_videoAspect = value;
    m_videoASpectStr = arg1;
    emit dApp->setMpvValue("video-aspect", QString::number(value));

    saveSettings();
}

void settingWindow::on_videoBZDY_clicked()
{
    double value = ui->videoBLEdit->text().toDouble();
    m_videoAspect = value;
    emit dApp->setMpvValue("video-aspect", QString::number(value));
    saveSettings();
}

bool settingWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj->objectName() == "pathEdit") {
        if (event->type() == QEvent::DragEnter) {
            ui->pathEdit->setText("");
            return true;
        }
        event->accept();
    }
    if (event->type() == QEvent::Drop) {

    }
    if (event->type() == QEvent::DragMove) {
        event->accept();
    }
    if (event->type() == QEvent::DragEnter) {
        QList<QUrl> urls = static_cast<QDropEvent *>(event)->mimeData()->urls();
        if (urls.isEmpty()) {
            return false;
        }

        QStringList paths;
        for (QUrl url : urls) {
            const QString path = url.toLocalFile();
            paths << path;
        }

        if (!paths.isEmpty()) {
//            ui->mainImageView->openImage(paths.at(0));
//            ui->basicImageView->openImage(paths.at(0));
            ui->pathEdit->setText(paths.at(0));
            QPixmap pix = dApp->getThumbnail(paths.at(0));
            if (!pix.isNull()) {
                ui->pixThumbnail->setPixmap(pix);
            }
        }

        event->accept();
        return true;
    }

    if (event->type() == QEvent::Resize && this->isVisible()) {

    }

    return false;
}

void settingWindow::on_pathEdit_textChanged(const QString &arg1)
{
    QPixmap pix = dApp->getThumbnail(arg1);
    if (!pix.isNull()) {
        ui->pixThumbnail->setPixmap(pix);
    }
}

void settingWindow::on_checkBox_stateChanged(int arg1)
{
    if (arg1 == 0) {
        m_isAutoMode = 0;
        m_stopx11Thread = true;
        if (m_x11thread) {
//            m_x11thread->wait();
            m_x11thread->terminate();
            m_x11thread = nullptr;
        }
    } else {
        m_stopx11Thread = false;
        m_isAutoMode = 1;
        if (!m_x11thread) {
            QTimer::singleShot(0, [ = ] {
                m_x11thread = QThread::create([ = ]()
                {
                    Display *display;
                    Window rootwin;
                    display = XOpenDisplay(NULL);
                    rootwin = DefaultRootWindow(display);
                    XSelectInput(display, rootwin, SubstructureNotifyMask); /*事件可以参考x.h*/
                    XEvent event;
                    while (!m_stopx11Thread) {
                        XNextEvent(display, &event);
                        switch (event.type) {
                        case ConfigureNotify: {
                            int screenwidth = qApp->desktop()->screen()->rect().width() - 100;
                            int screenheight = qApp->desktop()->screen()->rect().height() - 150;
                            XConfigureEvent *configureEvent = (XConfigureEvent *)&event;
                            if (configureEvent) {
                                if (0 >= configureEvent->x && 0 >= configureEvent->y) {
                                    if (!dApp->m_screenWid.contains(configureEvent->window) && configureEvent->width > screenwidth && configureEvent->height > screenheight) {
                                        dApp->m_x11WindowFuscreen.insert(configureEvent->window, true);
                                        dApp->setMpvpause();
                                        break;
                                    }

                                }
                                if (/*dApp->m_x11WindowFuscreen.contains(configureEvent->window) || */dApp->m_screenWid.contains(configureEvent->window)) {
                                    dApp->m_x11WindowFuscreen.remove(configureEvent->window);
                                }
                            }
                            for (auto window : dApp->m_x11WindowFuscreen.keys()) {
                                Drawable   d     /* d */;
                                Window     w /* root_return */;
                                int      x = 0   /* x_return */;
                                int      y = 0  /* y_return */;
                                unsigned int width = 0   /* width_return */;
                                unsigned int height = 0   /* height_return */;
                                unsigned int border_width = 0  /* border_width_return */;
                                unsigned int  depin = 0/* depth_return */;

                                XGetGeometry(display,  window, &w, &x, &y, &width, &height, &border_width, &depin);
                                qDebug() << x << y << width << height << border_width << depin;
                                int iWidth = width;
                                int iHeight = height;
                                if ((x > 0 && y > 0) || (iWidth < screenwidth && iHeight < screenheight) || depin == 32) {
                                    dApp->m_x11WindowFuscreen.remove(window);
                                }
                            }
                            qDebug() << dApp->m_x11WindowFuscreen.count();
                            if (dApp->m_x11WindowFuscreen.count() == 0 && m_isNoMpvPause) {
                                dApp->setMpvPlay();
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }

                });
                m_x11thread->start();
            });
        }
    }
    saveSettings();
}
