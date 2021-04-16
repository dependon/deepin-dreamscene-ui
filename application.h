#ifndef APPLICATION_H
#define APPLICATION_H
#define protected public
#include <DApplication>
#undef protected
#include <QCoreApplication>
#include <QString>
#include <QObject>
#include <QThread>
#include <QProcess>
#include <QRect>
#include <QMutex>

#include "data.h"
DWIDGET_USE_NAMESPACE

#define dApp (static_cast<Application*>(QCoreApplication::instance()))
class Application : public DApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();

    const QString thumbnailCachePath();
    const QPixmap getThumbnail(const QString &path);

    void setDesktopTransparent();
Q_SIGNALS:
    void setPlayPath(const QString &PATH);
    void setMpvPlay();
    void setMpvpause();
    void setMpvstop();
    void setMpvVolume(const qint32 volume);
    void setMpvValue(const QString &key, const QString &value);

    void setScreen(const int &index);
    void pathChanged(const QString &);
    void refreshPix(const QPixmap &);
    void setScreenMode(const QString &);
    void sigupdateGeometry();
    void quitApp();

    void setWallPaper(const QString &path);
    void saveSetting();

    void addPaperView(const QString &path);
    void removePaperView(const QString &path);

public:
    QThread *m_startDesktop{nullptr};
    QProcess *m_startDesktopProcess{nullptr};
    qint64 m_processId{0};
    QRect m_manual{0, 0, 0, 0};
    ScreenMode m_cuurentMode{IdCopyScreen};
    int m_currentScreenNum{0};

    QStringList m_allPath;
    QMutex mutex;

    QMap <unsigned long, bool>m_x11WindowFuscreen;
    QVector <unsigned long> m_screenWid;
    bool m_isNoMpvPause{true};
};

#endif // APPLICATION_H
