#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDesktopWidget>
#include <QFile>
#include <QSplashScreen>
#include <QTextCodec>

#include "installer.h"
#include "backend.h"
#include "dialogCheckHardware.h"

QSplashScreen *splash;

int main(int argc, char *argv[])
{
    QString changeLang;

    QApplication a(argc, argv);

    // Check what directory our app is in
    QString appDir;
    if ( QFile::exists("/usr/local/bin/pc-sysinstaller") )
      appDir = "/usr/local/share/trueos";
    else
      appDir = QCoreApplication::applicationDirPath();

    /*QTranslator translator;
    QLocale mylocale;
    QString langCode = mylocale.name();

    if ( ! changeLang.isEmpty() )
       langCode = changeLang;

    if ( QFile::exists(appDir + "/i18n/SysInstaller_" + langCode + ".qm" ) ) {
      translator.load( QString("SysInstaller_") + langCode, appDir + "/i18n/" );
      a.installTranslator(&translator);
      qDebug() << "Loaded Translation:" << appDir + "/i18n/SysInstaller_" + langCode + ".qm";
    } else {
      qDebug() << "Could not find: " << appDir + "/i18n/SysInstaller_" + langCode + ".qm";
      langCode = "";
    }*/
    QTextCodec::setCodecForLocale( QTextCodec::codecForName("UTF-8") ); //Force Utf-8 compliance

    if ( argc == 2)
    {
      QString flag = argv[1];
      if ( flag == "-checkhardware" )
      {
        dialogCheckHardware d;
        QDesktopWidget *desk = a.desktop();
        QRect dimensions = desk->screenGeometry();
        int wid = dimensions.width();     // returns desktop width
        int hig = dimensions.height();    // returns desktop height
        d.setGeometry((wid/2) - (355/2), (hig/2) - (151/2), 355, 151);
        d.programInit();
        d.show();
        a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
        return a.exec();
      }
    }

    QProcess compositor;
    if(QFile::exists("/usr/local/bin/compton")){
      compositor.startDetached("compton",QStringList() << "-b");
    }

    // Show our splash screen, so the user doesn't freak that that it takes a few seconds to show up
    QPixmap pixmap(":/TRUEOS/images/trueosheader.svg");
    QLabel splash(0,Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    QRect desk = QApplication::desktop()->screenGeometry();
    if(pixmap.width() > desk.width()/2){ pixmap = pixmap.scaledToWidth( desk.width()/2, Qt::SmoothTransformation); }
    splash.setPixmap(pixmap);
    splash.setGeometry(desk.x()+(desk.width()-pixmap.width())/2, desk.y()+(desk.height()-pixmap.height())/2, pixmap.width(), pixmap.height());
    splash.show();
    //Process events a couple time to ensure the splashscreen is visible *right now*
    QApplication::processEvents();
    QApplication::processEvents();
    Installer w;

    // Center the installer
    QRect dimensions = QApplication::desktop()->screenGeometry();
    int wid = dimensions.width();     // returns desktop width
    int hig = dimensions.height();    // returns desktop height
    QRect wizDimensions = w.geometry();
    int wizWid = wizDimensions.width(); // Wizard width
    int wizHig = wizDimensions.height(); // Wizard height
    qDebug() << "WizWid" << wizWid;
    qDebug() << "WizHig" << wizHig;
    w.setGeometry((wid/2) - (wizWid/2), (hig/2) - (wizHig/2), wizWid, wizHig);

    // Start the init
    w.initInstall(); //splash);

    w.show();
    splash.close();

    int ret = a.exec();
    if(compositor.state()==QProcess::Running){ compositor.terminate(); }
    return ret;
}
