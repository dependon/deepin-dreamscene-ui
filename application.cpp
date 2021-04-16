#include "application.h"
#include <QIcon>
#include <QDir>
#include <QUrl>
#include <QMutexLocker>
#include <QWindow>
#include <QCryptographicHash>

#include "setdesktop.h"

const QString toMd5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    this->setApplicationName(tr("deepin-dreamscene-ui"));
    this->setApplicationDisplayName(tr("deepin-dreamscene-ui"));
    this->setApplicationDescription(tr("deepin-dreamscene-ui\n"
                                       "基于qt5.11.3版本\n"
                                       "基于justforlxz的渲染方法开发\n"
                                       "github地址:https://github.com/dependon/deepin-dreamscene-ui/\n"
                                       "gitee地址:https://gitee.com/liuminghang/deepin-dreamscene-ui\n"
                                       "bug可以联系mail: liuminghang0821@gmail.com\n"
                                       "nvdia闭源驱动电脑下某些情况需要自定义模式配置尺寸"));
    this->setProductIcon(QIcon(":/tray.ico"));
    this->setWindowIcon(QIcon(":/tray.ico"));

}

Application::~Application()
{
    emit quitApp();
}

const QString Application::thumbnailCachePath()
{
    QString cacheP;

    QStringList systemEnvs = QProcess::systemEnvironment();
    for (QString it : systemEnvs) {
        QStringList el = it.split("=");
        if (el.length() == 2 && el.first() == "XDG_CACHE_HOME") {
            cacheP = el.last();
            break;
        }
    }
    cacheP = cacheP.isEmpty() ? (QDir::homePath() + "/.cache") : cacheP;

    // Check specific size dir
    const QString thumbCacheP = cacheP + "/thumbnails";
    QDir().mkpath(thumbCacheP + "/normal");
    QDir().mkpath(thumbCacheP + "/large");
    QDir().mkpath(thumbCacheP + "/fail");

    return thumbCacheP;
}

const QPixmap Application::getThumbnail(const QString &path)
{
    QMutexLocker locker(&mutex);

    const QString cacheP = thumbnailCachePath();
    QUrl url;
    if (!path.contains("file://")) {
        url = QUrl::fromLocalFile(path);
    } else {
        url = QUrl(path);
    }
    const QString md5s = toMd5(url.toString(QUrl::FullyEncoded).toLocal8Bit());
    const QString encodePath = cacheP + "/large/" + md5s + ".png";
//    const QString failEncodePath = cacheP + "/fail/" + md5s + ".png";
    if (QFileInfo(encodePath).exists()) {
        return QPixmap(encodePath);
    } else { /*if (QFileInfo(failEncodePath).exists()) */
        qDebug() << "Fail-thumbnail exist, won't regenerate: " ;
        return QPixmap();
    }
}

void Application::setDesktopTransparent()
{
    //dbus开启壁纸透明
    system("qdbus --literal com.deepin.dde.desktop /com/deepin/dde/desktop com.deepin.dde.desktop.EnableBackground false");
    //设置desktop透明
    char str[12] = "dde-desktop";
    int pid_t[128];
    find_pid_by_name(str, pid_t);
    int pid = pid_t[0];
    Display *display = XOpenDisplay(0);
    WindowsMatchingPid match(display, XDefaultRootWindow(display), pid);
    const list<Window> &result = match.result();
    for (Window id : result) {
        QWindow *window = QWindow::fromWinId((unsigned long)id);
        if (window != nullptr) {
            window->setOpacity(0.99);
        }
        if (!m_screenWid.contains(id)) {
            m_screenWid.push_back(id);
        }
    }
}
