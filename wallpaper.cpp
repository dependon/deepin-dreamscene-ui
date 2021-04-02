#include "wallpaper.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QtX11Extras/QX11Info>
#include <QScreen>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <QApplication>
#include <QPainter>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QDesktopWidget>
#include <QDebug>
#include <QLabel>
#include <QDBusConnection>
#include "application.h"
Wallpaper::Wallpaper(QString path, int currentScreen, QWidget *parent)
    : QWidget(parent)
    , m_currentPath(path)
    , m_currentScreen(currentScreen)
{

    setAttribute(Qt::WA_TranslucentBackground);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);

    registerDesktop();

    m_mpv = new MpvWidget(this);
    m_mpv->setGeometry(geometry());
    m_mpv->setGeometry(geometry());
    m_mpv->setGeometry(geometry());
    layout->addWidget(m_mpv);
    m_mpv->hide();
    m_mpv->setProperty("loop", true);
    m_mpv->setProperty("panscan", 1);
    m_mpv->setGeometry(geometry());

    connect(dApp, &Application::refreshPix, this, &Wallpaper::slotrefreshPix);
    connect(dApp, &Application::setScreenMode, this, &Wallpaper::slotsetScreenMode);
    connect(qApp->desktop(), &QDesktopWidget::resized, this, &Wallpaper::updateGeometry);
    connect(dApp, &Application::setPlayPath, this, &Wallpaper::setFile);
    connect(dApp, &Application::setMpvPlay, this, &Wallpaper::play);
    connect(dApp, &Application::setMpvpause, this, &Wallpaper::pause);
    connect(dApp, &Application::setMpvstop, this, &Wallpaper::stop);
    connect(dApp, &Application::setMpvVolume, this, &Wallpaper::setVolume);
    connect(dApp, &Application::setScreen, this, &Wallpaper::setScreen);
    connect(dApp, &Application::sigupdateGeometry, this, &Wallpaper::updateGeometry);
    connect(dApp, &Application::setMpvValue, this, &Wallpaper::slotSetMpvValue);

    QDesktopWidget *desktopwidget = QApplication::desktop();
    connect(desktopwidget, &QDesktopWidget::resized, this, [ = ] {
        QTimer::singleShot(1000, [ = ]{
            updateGeometry();
        });
        updateGeometry();
    });
    connect(desktopwidget, &QDesktopWidget::screenCountChanged, this, [ = ] {
        if (qApp->desktop()->screenCount() > 1 && IdCopyScreen == dApp->m_cuurentMode && !m_label2)
        {
            m_label2 = new QLabel();
            layout->addWidget(m_label2);
        } else
        {
            layout->removeWidget(m_label2);
            m_label2->deleteLater();
            m_label2 = nullptr;
        }
        QTimer::singleShot(1000, [ = ] {
            updateGeometry();
        });
        updateGeometry();
    });

    QDBusConnection::sessionBus().connect("com.deepin.SessionManager", "/com/deepin/SessionManager",
                                          "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
                                          SLOT(onSysLockState(QString, QVariantMap, QStringList)));

    QTimer::singleShot(1, this, &Wallpaper::updateGeometry);
    QTimer::singleShot(1000, this, [ = ] {
        int index = 0;
        int index1 = 0;
        qDebug() << "ssss" << index1;
        for (const QString &arg : qApp->arguments())
        {
            if (index != 0) {
                if (QFileInfo(arg).isFile()) {
                    setFile(arg);
                    play();
                    index1++;
                    qDebug() << "ssss1" << index1;
                }
            }
            index++;
        }
        qDebug() << "ssss2" << index1;
        if (index1 == 0)
        {
            QString playPath = "/opt/durapps/deepin-dreamscene-ui/09.mp4";
            m_currentPath = m_currentPath.replace("file://", "");
            if (!m_currentPath.isEmpty()) {
                if (QFileInfo(m_currentPath).isFile()) {
                    playPath = m_currentPath;
                }
            }
            setFile(playPath);
            play();
            m_currentPath = QFileInfo(m_currentPath).filePath();
            emit dApp->pathChanged(m_currentPath);
        }
        if (qApp->desktop()->screenCount() > 1 && IdCopyScreen == dApp->m_cuurentMode)
        {
            m_label2 = new QLabel();
            layout->addWidget(m_label2);
        }
    });

    setVolume(0);




}

void Wallpaper::changeScreenMode(ScreenMode mode)
{
    switch (mode) {
    case IdCopyScreen: {
        if (qApp->desktop()->screenCount() > 1) {
            if (nullptr == m_label2) {
                m_label2 = new QLabel();
                m_label2->setVisible(true);
            }
            layout()->addWidget(m_label2);
        }
        break;
    }
    case IdlayoutScreen: {
        if (nullptr != m_label2) {
            layout()->removeWidget(m_label2);
            delete m_label2 ;
            m_label2 = nullptr;
        }
        break;
    }
    case IdManualSet: {
        if (nullptr != m_label2) {
            layout()->removeWidget(m_label2);
            delete m_label2 ;
            m_label2 = nullptr;
        }
        break;
    }
    default:
        break;
    }
    //暂时调用两次,为保证切换顺利
    QTimer::singleShot(10, [ = ] {
        updateGeometry();
    });
    updateGeometry();
}

void Wallpaper::setScreen(const int &index)
{
    m_currentScreen = index;
}

void Wallpaper::setFile(const QString &path)
{
    m_mpv->show();
    m_mpv->command(QStringList() << "loadfile" << path);
    m_mpv->setProperty("pause", true);
}

void Wallpaper::setVolume(const qint32 volume)
{
    m_mpv->setProperty("volume", volume);
}

void Wallpaper::clear()
{
    stop();
    hide();
}

void Wallpaper::play()
{
    m_mpv->show();
    m_mpv->setProperty("pause", false);
}

void Wallpaper::pause()
{
    m_mpv->setProperty("pause", true);
}

void Wallpaper::stop()
{
    m_mpv->setProperty("stop", true);
}

void Wallpaper::slotrefreshPix(const QPixmap &pix)
{
    if (m_label2) {
        m_label2->setPixmap(pix);
    }
}

void Wallpaper::slotsetScreenMode(const QString &mode)
{
    if (mode == "复制") {
        if (dApp->m_cuurentMode != IdCopyScreen) {
            dApp->m_cuurentMode = IdCopyScreen;
            changeScreenMode(dApp->m_cuurentMode);
        }
    } else if (mode == "扩展") {
        if (dApp->m_cuurentMode != IdlayoutScreen) {
            dApp->m_cuurentMode = IdlayoutScreen;
            changeScreenMode(dApp->m_cuurentMode);
        }
    } else if (mode == "手动设置尺寸") {
        if (dApp->m_cuurentMode != IdManualSet) {
            dApp->m_cuurentMode = IdManualSet;
            changeScreenMode(dApp->m_cuurentMode);
        }
    }


}

void Wallpaper::registerDesktop()
{
    xcb_ewmh_connection_t m_ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(QX11Info::connection(), &m_ewmh_connection);
    xcb_ewmh_init_atoms_replies(&m_ewmh_connection, cookie, NULL);

    xcb_atom_t atoms[1];
    atoms[0] = m_ewmh_connection._NET_WM_WINDOW_TYPE_DESKTOP;
    xcb_ewmh_set_wm_window_type(&m_ewmh_connection, winId(), 1, atoms);

    QTimer::singleShot(1, this, [ = ] {
        show();
        lower();
    });
}

bool Wallpaper::event(QEvent *event)
{
    //https://github.com/dependon/deepin-dreamscene-ui/issues/4，临时解决WIN+D的问题
    if (event->type() == QEvent::WindowActivate) {
        qDebug() << "Video WindowActivate";
        QTimer::singleShot(100, this, []() {
            for (int index = 0; index < dApp->desktop()->screenCount(); index++) {
                system("xdotool search --class dde-desktop windowactivate");
            }
            qDebug() << "Desktop WindowActivate";
        });
    }
    return  QWidget::event(event);
}

void Wallpaper::onSysLockState(QString, QVariantMap key2value, QStringList)
{
    if (key2value.value("Locked").value<bool>()) {
        pause();
    } else {
        play();
    }
}

void Wallpaper::slotSetMpvValue(const QString &key, const QString &value)
{
    m_mpv->setProperty(key, value);
}
#include <QDesktopWidget>
#include <QGuiApplication>
void Wallpaper::updateGeometry()
{
//    QTimer::singleShot(100, this, [ = ] {
    dApp->m_currentScreenNum = dApp->desktop()->screenCount();
    QRect rec;
    QSize size1(0, 0);
    rec = qApp->desktop()->screenGeometry(qApp->desktop()->primaryScreen());
    QRect rec2 = qApp->desktop()->screenGeometry();
    QRect deskRect = qApp->desktop()->availableGeometry();
    rec = rec2;
    if (dApp->m_cuurentMode == IdCopyScreen) {
        rec = QRect(0, 0, rec.width(), rec.height());
        size1.setWidth(rec.width());
        size1.setHeight(rec.height());

    } else if (dApp->m_cuurentMode == IdlayoutScreen) {
        rec = QRect(0, 0, rec.width() * dApp->desktop()->screenCount(), rec.height());
        size1.setWidth(rec.width());
        size1.setHeight(rec.height());
    } else  if (dApp->m_cuurentMode == IdManualSet) {
        rec = dApp->m_manual;
        size1.setWidth(dApp->m_manual.width());
        size1.setHeight(dApp->m_manual.height());

    }
    this->setGeometry(rec);

//        m_mpv->setGeometry(rec2);
    qDebug() << this->size();
    m_mpv->move(rect().topLeft());
    m_mpv->setFixedSize(size1);

    lower();

    QTimer::singleShot(100, this, []() {
        for (int index = 0; index < dApp->desktop()->screenCount(); index++) {
            system("xdotool search --class dde-desktop windowactivate");
        }
        qDebug() << "Desktop WindowActivate";
    });
//    });
}
