#ifndef INSTALLER_H
#define INSTALLER_H

#include <QWidget>
#include <QMessageBox>
#include <QFile>
#include <QMenu>
#include <QProcess>
#include <QGraphicsScene>
#include <QSplashScreen>
#include <QTranslator>
#include <QLabel>

#include "ui_installer.h"
#include "dialogHelp.h"
#include "dialogKeyboard.h"
#include "dialogCheckHardware.h"
#include "wizardFreeBSD.h"
#include "wizardDisk.h"
#include "wizardRestore.h"

#define wFREEBSD 1
#define wPCSERVER 2
#define wKDE 3
#define wLXDE 4
#define wGNOME 5
#define wXFCE 6

#include "backend.h"

class Installer : public QMainWindow, private Ui::Installer
{
    Q_OBJECT

public:
    Installer(QWidget *parent = 0);
    ~Installer();
    void initInstall(); //QSplashScreen *);

private slots:
    void slotAbort();
    void slotBack();
    void slotHelp();
    void slotNext();
    void slotFinished();
    void slotChangeLanguage();
    void slotCheckHardware();

    // Disk slots
    void slotDiskCustomizeClicked();
    void slotSaveDiskChanges(QList<QStringList>, QString, QString, bool, bool);
    void slotBEInstallToggled(bool inBE = true);

    // Slots for the installation
    void slotInstallProcFinished( int exitCode, QProcess::ExitStatus status);
    void slotReadInstallerOutput();

    // Start xvkbd
    void slotPushVirtKeyboard();

    // Keyboard selector
    void slotPushKeyLayout();

    // Slots related to USB config files 
    void slotSaveConfigUSB();
    void slotLoadConfigUSB();

    // Slot to start the network manager
    void slotStartNetworkManager();

    // Slot to start the disk manager
    void slotStartDiskManager();

    // Desktop selector slots
    void slotSaveFBSDSettings(QString, QString, QString, QString, QString, QString, bool, bool, QStringList, QStringList);

    // Restore-related slots
    void slotSaveRestoreSettings(QStringList);

    // Slot to save keyboard layout
    void slotSaveKeyLayout(QString, QString, QString);

    // Slot to launch emergency shell
    void slotEmergencyShell();
    
    // Slot to update the install slideshow
    void nextSlide();

private:

    void setArch();
    void setVersion();
    void startInstall(); // Function which begins the install process
    void installFailed(); // Function which does post-install failure stuff

    QString zpoolTarget;
    bool promptInstallToZpool(); // Ask if user wants to install into BE

    // Functions to parse ZFS send/recv messages
    bool inZFSSend;
    double displayToDoubleK(QString);
    void parseStatusMessage(QString);
    QString repTotalK;

    // Disk functions
    void loadDiskInfo();
    bool autoGenPartitionLayout(QString target, bool isDisk);
    QStringList getDiskSummary();

    // Functions which return parts of the config file
    QStringList getGlobalCfgSettings();
    QStringList getDiskCfgSettings();

    // Any zpools waiting for import?
    QStringList existingZpools;

    void startConfigGen(); // Function which takes pieces of cfg, and assembles
    void checkSpaceWarning(); // Function to check space of disk drive

    void proceed(bool);
    bool isInstalled();
    bool checkDiskRequirements();

    // Desktop packages
    void initDesktopSelector();
    QStringList getDeskPkgCfg();
    bool availDesktopPackageData;
    bool availServerPackageData;
    QList<QStringList> listDeskPkgs; // QStringList for our available desktop pkgs
    QList<QStringList> listServerPkgs; // QStringList for our available server pkgs
    QStringList selectedPkgs; // QStringList for selected packages
    QGraphicsScene *defaultScene;
    QGraphicsScene *customScene;
    bool customPkgsSet;
    
    // Encryption bits
    QString autoEncPass;
    void warnEncryption();

    QList<QStringList> sysDisks; // Our lists which contains disk info
    QList<QStringList> sysPartitions; // Our lists which contains partition info
    QList<QStringList> sysFinalDiskLayout; // Our lists which contains the final disk layout
    QString sysPartType;
    int systemMemory; // Ammount of system RAM we have in MB
    QList<QStringList> listComponents; // QStringList for our available components
    QStringList languages;
    QStringList updatePartitions;
    QString Arch;
    QProcess *installProc;
    bool installFoundCounter;
    bool installFoundMetaCounter;
    bool installFoundFetchOutput;
    bool isLiveMode;
    bool hasPkgsOnMedia;
    bool hasInstallOnMedia;
    bool hasFreeBSDOnMedia;
    bool toggleKeyLayout;
    bool haveWarnedSpace;

    // Hardware checker
    dialogCheckHardware *pcHardware;

    // Help Stuff
    dialogHelp *pcHelp;

    QMenu *popup;

    // FreeBSD setup wizard
    wizardFreeBSD *wFBSD;

    // Restore wizard
    wizardRestore *wRestore;
    
    // FreeBSD options
    QString fRootPW;
    QString fName;
    QString fUserName;
    QString fUserPW;
    QString fShell;
    QString fHost;
    bool fSSH;
    bool fPORTS;
    QStringList fNetSettings;
    QStringList getUsersCfgSettings();

    // Keyboard Widget
    widgetKeyboard *wKey;
    QStringList keyModels;
    QStringList keyLayouts;
    QString curKeyLayout;
    QString curKeyModel;
    QString curKeyVariant;

    // Disk setup wizard
    wizardDisk *wDisk;
    //QTranslator *translator;

    // Custom CFG file to install with
    QString customCfgFile;

    // Zpool custom name
    QString zpoolName;

    // Force 4K mode?
    bool force4K;

    // Option to enable / disable refind
    bool useRefind;

    // String to override install mode efi/pc
    QString forceBIOS;

    // Running in EFI mode?
    bool efiMode;

    // Is the user doing the default installation option?
    bool defaultInstall;

    // Restore Options
    QStringList restOpts;

    // Settings for remote access
    QStringList appCafeSettings; //Disused for now

    // Variables for the slideshow functionality
    int cslide; //current slide
    QTimer *slideTimer;
    // Functions for loading the slides
    int numSlides();
    void loadSlide(QLabel *textlabel, QLabel *iconlabel, int num, QSize sz);

protected:
    void closeEvent(QCloseEvent *event);

};

#endif // INSTALLER_H
