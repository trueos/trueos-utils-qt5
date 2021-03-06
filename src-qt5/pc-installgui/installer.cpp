#include <QCloseEvent>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QTranslator>
#include <QInputDialog>
#include <QSplashScreen>
#include <QGraphicsPixmapItem>
#include <QScreen>

#include "backend.h"

#include "installer.h"
#include "helpText.h"

#define PREFIX QString("/usr/local")

Installer::Installer(QWidget *parent) : QMainWindow(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint)
{
    setupUi(this);
    //Make sure the window is full-screen (in the background, not foreground)
    QScreen *scrn = QApplication::primaryScreen();
    this->setGeometry( scrn->geometry() );
    //Now start loading the rest of the interface
    setVersion();
    loadPciConfInfo();

    //translator = new QTranslator();
    haveWarnedSpace=false;
    force4K = false;
    useRefind = false;
    defaultInstall = true;

    connect(abortButton, SIGNAL(clicked()), this, SLOT(slotAbort()));
    connect(backButton, SIGNAL(clicked()), this, SLOT(slotBack()));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(slotNext()));

    connect(helpButton, SIGNAL(clicked()), this, SLOT(slotHelp()));
    connect(pushShell, SIGNAL(clicked()), this, SLOT(slotEmergencyShell()));
    connect(pushTouchKeyboard, SIGNAL(clicked()), this, SLOT(slotPushVirtKeyboard()));
    connect(pushChangeKeyLayout, SIGNAL(clicked()), this, SLOT(slotPushKeyLayout()));
    connect(pushHardware, SIGNAL(clicked()), this, SLOT(slotCheckHardware()));
    connect(pushNetwork, SIGNAL(clicked()), this, SLOT(slotStartNetworkManager()));
    //connect(pushDiskManager, SIGNAL(clicked()), this, SLOT(slotStartDiskManager()));
    connect(pushLoadConfig, SIGNAL(clicked()), this, SLOT(slotLoadConfigUSB()));
    connect(pushSaveConfig, SIGNAL(clicked()), this, SLOT(slotSaveConfigUSB()));
    connect(pushSaveConfig2, SIGNAL(clicked()), this, SLOT(slotSaveConfigUSB()));

    connect(radio_install_be, SIGNAL(toggled(bool)), this, SLOT(slotBEInstallToggled(bool)) );
    connect(combo_install_pools, SIGNAL(currentIndexChanged(int)), this, SLOT(slotBEInstallToggled()) );

    connect(tree_graphics_drivers, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(pkgChanged(QTreeWidgetItem *)) );
    //abortButton->setText(tr("&Cancel"));
    backButton->setText(tr("&Back"));
    nextButton->setText(tr("&Next"));

    // We use GPT by default now
    sysPartType="GPT";

    // No optional components by default
    fPORTS=false;

    // Load the keyboard info
    keyModels = Scripts::Backend::keyModels();
    keyLayouts = Scripts::Backend::keyLayouts();

    // Set the arch name
    setArch();

    // Start on the first screen
    installStackWidget->setCurrentIndex(0);
    backButton->setVisible(false);

    // Get a list of existing zpools
    existingZpools = Scripts::Backend::listAllZpools();
    qDebug() << "Existing Zpools:" << existingZpools;

    // Check if we are running in EFI mode
    if ( system("sysctl -n machdep.bootmethod | grep -q 'UEFI'") == 0 )
      efiMode=true;
    else
      efiMode=false;

   slideTimer = new QTimer(this);
    slideTimer->setInterval(30000); //30 second slide duration
    connect(slideTimer, SIGNAL(timeout()), this, SLOT(nextSlide()) );

}

Installer::~Installer()
{
    //delete ui;
}

bool Installer::isInstalled()
{
    return false;
}

void Installer::setArch()
{
   QProcess m;
   m.start(QString("uname"), QStringList() << "-m");
   while(m.state() == QProcess::Starting || m.state() == QProcess::Running) {
      m.waitForFinished(200);
      QCoreApplication::processEvents();
   }

   // Get output
   Arch = m.readLine().simplified();
   qDebug() << Arch;
}

void Installer::setVersion(){
     QProcess m;
   m.start(QString("uname"), QStringList() << "-pv");
   while(m.state() == QProcess::Starting || m.state() == QProcess::Running) {
      m.waitForFinished(200);
      QCoreApplication::processEvents();
   }
   QString info = m.readLine().simplified();
   QString datetime = info.section(": ",1,1).section(" ",0,5);
   QString arch = info.section(" ",-1);
   // Get output
   labelVersion->setText( QString( "%1 (%2)").arg(datetime, arch) );
}

void Installer::slotCheckHardware()
{
  pcHardware = new dialogCheckHardware();
  pcHardware->programInit();
  pcHardware->setWindowModality(Qt::ApplicationModal);
  pcHardware->show();
  pcHardware->raise();
}

void Installer::slotPushKeyLayout()
{
  wKey = new widgetKeyboard();
  wKey->programInit(keyModels, keyLayouts, curKeyModel, curKeyLayout, curKeyVariant);
  wKey->setWindowModality(Qt::ApplicationModal);
  connect(wKey, SIGNAL(saved(QString, QString, QString)), this, SLOT(slotSaveKeyLayout(QString, QString, QString)));
  wKey->show();
  wKey->raise();
}

void Installer::slotSaveKeyLayout(QString model, QString layout, QString variant)
{
  curKeyModel = model;
  curKeyLayout = layout;
  curKeyVariant = variant;
  qDebug() << "Changed keyboard layout:" << curKeyModel << curKeyLayout << curKeyVariant;
}

void Installer::initInstall() //QSplashScreen *splash)
{
    //splash->showMessage("Loading translations", Qt::AlignHCenter | Qt::AlignBottom);
    // load languages
    QString langCode;
    bool foundLang = false;
    comboLanguage->clear();
    languages = Scripts::Backend::languages();

    //splash->showMessage("Loading localizations", Qt::AlignHCenter | Qt::AlignBottom);
    QString curLang = Scripts::Backend::detectCountryCode();
    for (int i=0; i < languages.count(); ++i) {
        QString languageStr = languages.at(i);
        QString language = languageStr.split("-").at(0);
        comboLanguage->addItem(language.trimmed());

	// Grab the language code
        langCode = languageStr;
        langCode.truncate(langCode.lastIndexOf(")"));
        langCode.remove(0, langCode.lastIndexOf("(") + 1);
	if ( curLang == langCode ) {
          comboLanguage->setCurrentIndex(i);
          foundLang = true;
        }
    }
    connect(comboLanguage, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotChangeLanguage()));
    // If we found a language from geo-loication, change UI now
    if ( foundLang )
       slotChangeLanguage();


    // Load any package scheme data
    //splash->showMessage("Loading packages", Qt::AlignHCenter | Qt::AlignBottom);

    // Do check for available meta-pkgs on boot media
    if ( QFile::exists("/tmp/no-meta-pkgs") )
    	hasPkgsOnMedia = false;
    else
    	hasPkgsOnMedia = true;

    qDebug() << "Package Media:" << availDesktopPackageData << hasPkgsOnMedia;

    // See if this media has a freebsd release on it
    if ( QFile::exists("/tmp/no-fbsd-release") ) {
	hasFreeBSDOnMedia = false;
    } else {
	hasFreeBSDOnMedia = true;
    }
    // Do check for install pkgs on boot media
    if ( QFile::exists("/tmp/no-install-pkgs") ) {
    	hasInstallOnMedia = false;
    } else {
    	hasInstallOnMedia = true;
    }

    // Set the key toggle
    toggleKeyLayout = true;

    // Is this a LIVE disk?
    if ( QFile::exists("/usr/trueos-live") )
    	isLiveMode = true;
    else
    	isLiveMode = false;

    // Get available memory
    //splash->showMessage("Loading system information", Qt::AlignHCenter | Qt::AlignBottom);
    systemMemory = Scripts::Backend::systemMemory();

    // Load up the keyboard information
    //connectKeyboardSlots();

    // Connect the disk slots
    connect(pushDiskCustomize,SIGNAL(clicked()), this, SLOT(slotDiskCustomizeClicked()));

    // Load the disks
    //splash->showMessage("Loading disk information", Qt::AlignHCenter | Qt::AlignBottom);
    loadDiskInfo();

}

void Installer::loadDiskInfo()
{
   sysDisks = Scripts::Backend::hardDrives();
   if ( sysDisks.empty() ) {
      QMessageBox::critical(this, tr("TrueOS Installer"),
                                tr("Unable to detect any disk drives! The install will now exit."),
                                QMessageBox::Ok,
                                QMessageBox::Ok);
      exit(1);
   }

   // For now use the first disk we find
   if ( ! autoGenPartitionLayout(sysDisks.at(0).at(1), true) )
   {
      QMessageBox::critical(this, tr("TrueOS Installer"),
        tr("Unable to suggest a partition for the detected disk."),
        QMessageBox::Ok,
        QMessageBox::Ok);
   }

   textEditDiskSummary->clear();
   QStringList summary = getDiskSummary();
   for ( int i=0; i < summary.count(); ++i)
     textEditDiskSummary->append(summary.at(i));

   textEditDiskSummary->moveCursor(QTextCursor::Start);

}

// Function which will auto-generate a partition layout based upon the target disk / slice
bool Installer::autoGenPartitionLayout(QString target, bool isDisk)
{
  QString targetType, tmp;
  int targetLoc, totalSize = 0, swapsize;
  QString targetDisk, targetSlice, tmpPass, fsType;
  bool ok;
  ok = false;

  // Clear out the original disk layout
  sysFinalDiskLayout.clear();
  QStringList fileSystem;
  qDebug() << "Generating disk layout";


  if ( isDisk ) {
    targetType = "DRIVE";
    targetSlice = "ALL";
    targetDisk = target;
    targetLoc = 1;
  } else {
    targetType = "SLICE";
    targetDisk = target;
    targetDisk.truncate(targetDisk.size() -2);
    targetSlice = target;
    targetSlice = targetSlice.remove(0, targetSlice.size() -2);
    targetLoc = 2;
  }

  // Lets get the size for this disk / partition
  for (int i=0; i < sysDisks.count(); ++i) {
      // Make sure to only add the slices to the listDiskSlices
      if ( sysDisks.at(i).at(0) == targetType && target == sysDisks.at(i).at(targetLoc))
        totalSize = sysDisks.at(i).at(targetLoc + 1).toInt(&ok);
  }

  // Give us a small buffer for rounding errors
  totalSize = totalSize - 10;

  // Save 100MiB for EFI FAT16 filesystem
  if ( efiMode )
    totalSize = totalSize - 110;

  // Setup some swap space
  if ( totalSize > 50000 ) {
    // 4GB if over 50GB of disk space
    swapsize = 4096;
  } else if ( totalSize > 30000 ) {
    // 2GB if over 30GB of disk space
    swapsize = 2048;
  } else {
    // Minimum 512MB
    swapsize = 512;
  }
  totalSize = totalSize - swapsize;

  // We got a valid size for this disk / slice, lets generate the layout now
  if( !ok )
    return false;

  // If over 1.99TB, we should use GPT mode
  if ( totalSize > 1999900 )
    sysPartType="GPT";

  // Add the main zfs pool with standard partitions
  fsType= "ZFS";
  fileSystem << targetDisk << targetSlice << "/(compress=lz4|atime=off),/tmp(compress=lz4|setuid=off),/usr(canmount=off|mountpoint=none),/usr/home(compress=lz4),/usr/jails(compress=lz4),/usr/obj(compress=lz4),/usr/ports(compress=lz4),/usr/src(compress=lz4),/var(canmount=off|atime=on|mountpoint=none),/var/audit(compress=lz4),/var/log(compress=lz4|exec=off|setuid=off),/var/mail(compress=lz4),/var/tmp(compress=lz4|exec=off|setuid=off)" << fsType << tmp.setNum(totalSize) << "" << "";
  sysFinalDiskLayout << fileSystem;
  fileSystem.clear();

  // Now add swap space
  fileSystem << targetDisk << targetSlice << "SWAP.eli" << "SWAP.eli" << tmp.setNum(swapsize) << "" << "";
  sysFinalDiskLayout << fileSystem;
  fileSystem.clear();

  //qDebug() << "Auto-Gen FS:" <<  fileSystem;
  return true;
}

// Function which returns the pc-sysinstall cfg data
QStringList Installer::getDiskSummary()
{
  QList<QStringList> copyList;
  QStringList summaryList;
  QString tmp, workingDisk, workingSlice, tmpSlice, XtraTmp, startPart, sliceSize;
  int disk = 0;

  // Doing upgrade
  if ( ! zpoolTarget.isEmpty() )
  {
    summaryList << "";
    summaryList << tr("Installing to new boot environment in existing storage pool: %1").arg(zpoolTarget);
    return summaryList;
  }

  // Copy over the list to a new variable we can mangle without modifying the original
  copyList = sysFinalDiskLayout;

  if ( copyList.at(0).at(0) == "MANUAL" )
  {
    summaryList << "";
    summaryList << tr("Installing to file-system mounted at /mnt");
    return summaryList;
  }

  // Start our summary
  summaryList << "";
  summaryList << "<b>" + tr("The disk will initialize with this configuration:") + "</b>";

  while ( ! copyList.empty() )
  {
    workingDisk = copyList.at(0).at(0);
    workingSlice = copyList.at(0).at(1);
    tmpSlice = workingSlice;

    // Check if this is an install to "Unused Space"
    for (int z=0; z < sysDisks.count(); ++z)
      if ( sysDisks.at(z).at(0) == "SLICE" \
        && sysDisks.at(z).at(2) == workingDisk + workingSlice \
        && sysDisks.at(z).at(4) == "Unused Space" )
          tmpSlice = "free";

    // Check for any mirror for this device
    for (int i=0; i < copyList.count(); ++i) {
       if ( copyList.at(i).at(2).indexOf("MIRROR(" + workingDisk + ")") != -1 )
       {
         summaryList << tr("Disk:") + copyList.at(i).at(0) + " " + tr("Mirroring:") + workingDisk;
         copyList.removeAt(i);
         break;
       }
    }



    // If after doing the mirror, our list is empty, break out
    if ( copyList.empty() )
      break;

    // If there is a dedicated /boot partition, need to list that first, see what is found
    for (int i=0; i < copyList.count(); ++i) {
      QStringList mounts = copyList.at(i).at(2).split(",");
      for (int z = 0; z < mounts.size(); ++z) {
        if ( copyList.at(i).at(0) == workingDisk \
          && copyList.at(i).at(1) == workingSlice \
          && mounts.at(z) == "/boot" )
		startPart="/boot";
      }
    }

    // If no dedicated /boot partition, then lets list "/" first
    if(startPart.isEmpty())
	startPart="/";

    // Start by looking for the root partition
    for (int i=0; i < copyList.count(); ++i) {
      QStringList mounts = copyList.at(i).at(2).split(",");
      for (int z = 0; z < mounts.size(); ++z) {
        if ( copyList.at(i).at(0) == workingDisk \
          && copyList.at(i).at(1) == workingSlice \
          && (mounts.at(z) == startPart || mounts.at(z).indexOf(startPart + "(") == 0 ) ) {
          // Check if we have any extra arguments to throw on the end
          XtraTmp="";
          if ( ! copyList.at(i).at(5).isEmpty() )
            XtraTmp=" (" + copyList.at(i).at(5) + ")" ;

          // Write the user summary
          summaryList << "";
          summaryList << tr("Partition:") + " " + workingDisk + "(" + workingSlice + "):";
          summaryList << tr("FileSystem:") + " " + copyList.at(i).at(3);
          summaryList << tr("Size:") + " " + copyList.at(i).at(4) + "MB ";
          if ( copyList.at(i).at(3) == "ZFS" ) {
            QStringList zDS = copyList.at(i).at(2).split(",");
            QString zTMP;
            for (int ds = 0; ds < zDS.size(); ++ds) {
              //if ( zDS.at(ds) != "/" )
              // zDS.replace(ds, "/" + zDS.at(ds));
              if ( zDS.at(ds).indexOf("(") != -1 ) {
                zTMP = zDS.at(ds);
                zTMP.replace("(", " (");
                zDS.replace(ds, zTMP );
              }
            }
            if ( radioRestore->isChecked())
               summaryList << tr("ZFS Datasets:") + " " + tr("The original ZFS layout will be restored");
            else
               summaryList << tr("ZFS Datasets:<br>") + " " + zDS.join("<br>");
          } else {
            summaryList << tr("Mount:") + " " + copyList.at(i).at(2);
          }
          if ( ! XtraTmp.isEmpty() ) {
            summaryList << tr("Options:") + " " + copyList.at(i).at(5);
          }

          // Done with this item, remove it now
          copyList.removeAt(i);
          break;
        }
      }
    }


    // Now look for SWAP
    for (int i=0; i < copyList.count(); ++i) {
      if ( copyList.at(i).at(0) == workingDisk \
        && copyList.at(i).at(1) == workingSlice \
        && copyList.at(i).at(2) == "SWAP.eli" ) {

        // Write the user summary
        summaryList << "";
        summaryList << tr("Partition:") + " " + workingDisk + "(" + workingSlice + "):";
        summaryList << tr("FileSystem:") + " " + copyList.at(i).at(3);
        summaryList << tr("Size:") + " " + copyList.at(i).at(4) + "MB ";

        // Done with this item, remove it now
        copyList.removeAt(i);
        break;
      }
    }


    // Now look for any other partitions
    int count = copyList.count();
    for (int i=0; i < count; ++i) {
      if ( copyList.at(i).at(0) == workingDisk \
        && copyList.at(i).at(1) == workingSlice ) {

        // Check if we have any extra arguments to throw on the end
        XtraTmp="";
        if ( ! copyList.at(i).at(5).isEmpty() )
          XtraTmp=" (" + copyList.at(i).at(5) + ")" ;

	// If we are working on the last partition, set the size to 0 to use remaining disk
	if ( i == (count - 1) )
		sliceSize = "0";
	else
		sliceSize=copyList.at(i).at(4);

        // Write the user summary
        summaryList << "";
        summaryList << tr("Partition:") + " " + workingDisk + "(" + workingSlice + "):";

        summaryList << tr("FileSystem:") + " " + copyList.at(i).at(3);

        summaryList << tr("Size:") + " " + copyList.at(i).at(4) + "MB ";
	if ( copyList.at(i).at(3) != "ZFS" )
          summaryList << tr("Mount:") + " " + copyList.at(i).at(2);
        if ( ! XtraTmp.isEmpty() ) {
          summaryList << tr("Options:") + " " + copyList.at(i).at(5);
        }

        // Done with this item, remove it now
        copyList.removeAt(i);
        i--;
        count--;
      }
    }

    // Increment our disk counter
    disk++;
  }

  return summaryList;
}

void Installer::slotDiskCustomizeClicked()
{
  wDisk = new wizardDisk();
  wDisk->programInit();
  wDisk->setWindowModality(Qt::ApplicationModal);
  if ( radioRestore->isChecked() )
    wDisk->setRestoreMode();
  connect(wDisk, SIGNAL(saved(QList<QStringList>, QString, QString, bool, bool)), this, SLOT(slotSaveDiskChanges(QList<QStringList>, QString, QString, bool, bool)));
  wDisk->show();
  wDisk->raise();
}

void Installer::slotSaveDiskChanges(QList<QStringList> newSysDisks, QString partType, QString zName, bool zForce, bool refind)
{

  zpoolName = zName;
  force4K = zForce;
  defaultInstall = false;
  useRefind=refind;

  // Save the new disk layout
  sysPartType=partType;
  sysFinalDiskLayout = newSysDisks;
  textEditDiskSummary->clear();
  QStringList summary = getDiskSummary();
  for ( int i=0; i < summary.count(); ++i)
    textEditDiskSummary->append(summary.at(i));

  textEditDiskSummary->moveCursor(QTextCursor::Start);

  // Regenerate the config
  startConfigGen();
}

void Installer::proceed(bool forward)
{
    int count = installStackWidget->count() - 1;
    int index = installStackWidget->currentIndex();

    index = forward ?
            (index == count ? count : index + 1) :
            (index == 0 ? 0 : index - 1);

    if ( index > 0 && index != 5)
      backButton->setVisible(true);
    else
      backButton->setVisible(false);

    installStackWidget->setCurrentIndex(index);
}

// Slot which is called when the Finish button is clicked
void Installer::slotFinished()
{
  QProcess::startDetached("shutdown -r now");
  QApplication::exit(0);
}

void Installer::slotSaveFBSDSettings(QString rootPW, QString name, QString userName, QString userPW, QString shell, QString hostname, bool ssh, bool ports, QStringList netSettings, QStringList appcafe)
{
  fRootPW = rootPW;
  fName = name;
  fUserName = userName;
  fUserPW = userPW;
  fShell = shell;
  fHost = hostname;
  fSSH = ssh;
  fPORTS = ports;
  fNetSettings = netSettings;
  appCafeSettings = appcafe; //Disused for now
  installStackWidget->setCurrentIndex(installStackWidget->currentIndex() + 2);

  // Generate the pc-sysinstall config
  startConfigGen();
}

bool Installer::promptInstallToZpool()
{
  bool ok;
  QString ans = QInputDialog::getItem(this, tr("Install to existing ZFS pool?"), tr("The following pool(s) have been found.\n Do you wish to install into this pool?\n (This will do a clean install into a new boot environment without destroying existing data.)"), existingZpools, 0, false, &ok);
  if ( ok && !ans.isEmpty())
  {
    zpoolTarget=ans;

    return true;
  }

  zpoolTarget="";
  return false;
}


void Installer::slotNext()
{
   QString tmp;

   // If no pkgs on media
   if ( installStackWidget->currentIndex() == 0 && ! hasPkgsOnMedia) {
     installStackWidget->setCurrentIndex(2);
     return;
   }

   // Start the Restore wizard
   if ( radioRestore->isChecked() && installStackWidget->currentIndex() == 1 ) {
     wRestore = new wizardRestore();
     wRestore->setWindowModality(Qt::ApplicationModal);
     wRestore->programInit();
     connect(wRestore, SIGNAL(saved(QStringList)), this, SLOT(slotSaveRestoreSettings(QStringList)));
     wRestore->show();
     wRestore->raise();
     //return ;
   }

   // Start the TrueOS wizard
   if ( radioServer->isChecked() && installStackWidget->currentIndex() == 1 ) {
     bool tOS;
     tOS = true;

     wFBSD = new wizardFreeBSD();
     wFBSD->setWindowModality(Qt::ApplicationModal);
     wFBSD->programInit(tOS);
     connect(wFBSD, SIGNAL(saved(QString, QString, QString, QString, QString, QString, bool, bool, QStringList, QStringList)), this, SLOT(slotSaveFBSDSettings(QString, QString, QString, QString, QString, QString, bool, bool, QStringList, QStringList)));
     wFBSD->show();
     wFBSD->raise();
     //return ;
   }

   if( installStackWidget->currentIndex() == 1){
       updatePkgLists();

   }
   // Create the pc-sysinstall config
   if ( installStackWidget->currentIndex() == 2 ) {
    combo_install_pools->clear();
     // We have existing zpool, see if we want to upgrade within
     if ( ! existingZpools.isEmpty() && !radioRestore->isChecked() ) {
       qDebug() << "BE install option available:" << existingZpools;
       combo_install_pools->addItems(existingZpools);
       radio_install_be->setChecked(false);
       radio_install_be->setVisible(true);
       radio_install_full->setVisible(true);
       radio_install_full->setChecked(true);
       slotBEInstallToggled(false);
     }else{
       //No pools found, or restore selected
       qDebug() << "No BE install option available:" << existingZpools;
       radio_install_full->setChecked(true);
       radio_install_full->setVisible(false);
       radio_install_be->setVisible(false);
       slotBEInstallToggled(false); //update display
     }

     // Re-gen the config / summary
     /*startConfigGen();
     textEditDiskSummary->clear();
     QStringList summary = getDiskSummary();
     for ( int i=0; i < summary.count(); ++i)
       textEditDiskSummary->append(summary.at(i));
     textEditDiskSummary->moveCursor(QTextCursor::Start);*/
   }

   // If the chosen disk is too small or partition is invalid, don't continue
   if ( installStackWidget->currentIndex() == 3 && ! checkDiskRequirements())
      return;

   if ( installStackWidget->currentIndex() == 3 )
   {
      startConfigGen();
      QString msg;

      // Check for any space warnings
      if ( ! haveWarnedSpace )
        checkSpaceWarning();

      if (radioRestore->isChecked() )
	msg=tr("Start the restore now?");
      else
        if ( defaultInstall )
	  msg=tr("Start the default Full-Disk installation now?");
        else
	  msg=tr("Start the Customized-Disk installation now?");

      int ret = QMessageBox::question(this, tr("TrueOS Installer"),
                                msg,
                                QMessageBox::No | QMessageBox::Yes,
                                QMessageBox::No);
      switch (ret) {
      case QMessageBox::Yes:
          startInstall();
          break;
      case QMessageBox::No: // :)
          return;
          break;
      }
   }

   proceed(true);
}

void Installer::slotBack()
{
   // If no pkgs on media
   if ( installStackWidget->currentIndex() == 2 && ! hasPkgsOnMedia) {
	installStackWidget->setCurrentIndex(0);
        return;
   }

   proceed(false);
}

void Installer::slotAbort()
{
   close();
}

void Installer::slotChangeLanguage()
{
    static QTranslator *translator = 0;
    //if ( comboLanguage->currentIndex() == -1 && translator!=0)
      //return;

    // Figure out the language code
    QString langCode = "en_US";
    if(comboLanguage->currentIndex() != -1){
      langCode = languages.at(comboLanguage->currentIndex());
    }

    // Grab the language code
    langCode.truncate(langCode.lastIndexOf(")"));
    langCode.remove(0, langCode.lastIndexOf("(") + 1);

    // Check what directory our app is in
    QString appDir;
    if ( QFile::exists("/usr/local/bin/pc-sysinstaller") )
      appDir = "/usr/local/share/trueos";
    else
      appDir = QCoreApplication::applicationDirPath();

    //QTranslator *translator = new QTranslator();
    qDebug() << "Remove the translator";
    if ( translator!=0 && ! translator->isEmpty() )
      QCoreApplication::removeTranslator(translator);

    if(translator==0){ translator = new QTranslator(); }
    if (translator->load( QString("SysInstaller_") + langCode, appDir + "/i18n/" )) {
      qDebug() << "Load new Translator" << langCode;
      QCoreApplication::installTranslator(translator);
    }
    this->retranslateUi(this); //For en_US - there is no translation file

    // Change the default keyboard layout
    if ( langCode == "en" ) {
       curKeyModel="pc104";
       curKeyLayout="us";
       curKeyVariant="";
       Scripts::Backend::changeKbMap("pc104", "us", "");
    } else {
       // TODO - At some point, add additional tests here and set more specific layouts
       // based upon the language selected
       curKeyModel="pc105";
       curKeyLayout=langCode;
       curKeyVariant="";
       Scripts::Backend::changeKbMap("pc105", langCode, "" );
    }

    // Need to re-set the version string
    setVersion();
}

QStringList Installer::getGlobalCfgSettings()
{

  QStringList tmpList;
  QString tmp, upDrive;

  tmpList << "# Auto-Generated pc-sysinstall configuration";
  tmpList << "installInteractive=no";

  // Are we doing a restore?
  if ( radioRestore->isChecked() )
  {
    // If using an ISCSI file, set those options
    if ( restOpts.at(0) == "ISCSI" ) {
      qDebug() << "ISSCI RESTORE MODE!";
      tmpList << "installMode=zfsrestoreiscsi";
      tmpList << "iscsiFile=" + restOpts.at(1);
      QFile iscsipassfile( "/tmp/lp-iscsi-pass" );
      if ( iscsipassfile.open( QIODevice::WriteOnly ) ) {
        QTextStream streampass( &iscsipassfile );
        streampass <<  restOpts.at(2);
        iscsipassfile.close();
      }
      tmpList << "iscsiPass=/tmp/lp-iscsi-pass";
    } else {
      tmpList << "installMode=zfsrestore";
      tmpList << "";
      tmpList << "sshHost=" + restOpts.at(0);
      tmpList << "sshUser=" + restOpts.at(1);
      tmpList << "sshPort=" + restOpts.at(2);
      if ( ! restOpts.at(3).isEmpty() )
        tmpList << "sshKey=" + restOpts.at(3);
      tmpList << "zfsProps=" + restOpts.at(4);
      tmp = restOpts.at(4);
      tmp.replace(".lp-props-", "");
      tmp.replace("#", "/");
      tmpList << "zfsRemoteDataset=" + tmp;
    }

    // Using a custom zpool name?
    if ( ! zpoolName.isEmpty() )
      tmpList << "zpoolName=" + zpoolName;

    // Are we force enabling ZFS 4K block sizes?
    if ( force4K )
      tmpList << "zfsForce4k=YES";

    // Did user chose to not use refind?
    if ( useRefind )
      tmpList << "efiLoader=refind";
    else
      tmpList << "efiLoader=bsd";

    tmpList << "";
    return tmpList;
  } // End of restore options


  if ( sysFinalDiskLayout.at(0).at(0) == "MANUAL" )
  {
    tmpList << "installMode=extract";
    tmpList << "installLocation=/mnt";
  } else if ( ! zpoolTarget.isEmpty() ) {
    // Doing an upgrade
    tmpList << "installMode=upgrade";
    tmpList << "zpoolName=" + zpoolTarget;
    zpoolName="";
  } else {
    // Doing a fresh install
    tmpList << "installMode=fresh";
  }

  QString distFiles;
  distFiles="base doc kernel";
  if ( Arch == "amd64" )
     distFiles+=" lib32";

  // Check for ports
  if ( fPORTS )
     distFiles+=" ports";

  // System type we are installing
  if ( radioDesktop->isChecked() )
    tmpList << "installType=TrueOS";
  else
    tmpList << "installType=FreeBSD";

  tmpList << "packageType=pkg";

  // Set the distFiles being used
  tmpList << "distFiles=" + distFiles;
  tmpList << "";

  // If we have a custom hostname, add it to the config
  if ( ! fHost.isEmpty() )
    tmpList << "hostname=" + fHost;

  // Using a custom zpool name?
  if ( ! zpoolName.isEmpty() )
    tmpList << "zpoolName=" + zpoolName;

  // Are we force enabling ZFS 4K block sizes?
  if ( force4K )
    tmpList << "zfsForce4k=YES";

  // Networking setup
  if ( radioDesktop->isChecked() ) {
    // TrueOS network setup
    tmpList << "netSaveDev=AUTO-DHCP-SLAAC";
  } else {
    // FreeBSD network setup
    if ( fNetSettings.at(0) == "AUTO-DHCP" )
      tmpList << "netSaveDev=AUTO-DHCP";
    else if ( fNetSettings.at(0) == "AUTO-DHCP-SLAAC" ) {
      tmpList << "netSaveDev=AUTO-DHCP-SLAAC";
      // We cannot rely on SLAAC to provide DNS for example.  The same is true
      // for DHCP but the worls seems to have forgotten...
      tmpList << "netSaveIPv6=" + fNetSettings.at(5);
      tmpList << "netSaveIPv6NameServer=" + fNetSettings.at(6);
      tmpList << "netSaveIPv6DefaultRouter=" + fNetSettings.at(7);
    } else if ( fNetSettings.at(0) == "IPv6-SLAAC" ) {
       tmpList << "netSaveDev=IPv6-SLAAC";
      // We cannot rely on SLAAC to provide DNS for example.  The same is true
      // for DHCP but the worls seems to have forgotten...
      tmpList << "netSaveIPv6=" + fNetSettings.at(5);
      tmpList << "netSaveIPv6NameServer=" + fNetSettings.at(6);
      tmpList << "netSaveIPv6DefaultRouter=" + fNetSettings.at(7);
    }
    else
    {
      tmp = fNetSettings.at(0);
      if ( tmp.indexOf(":") > 0 )
        tmp.truncate(tmp.indexOf(":"));
      tmpList << "netSaveDev=" + tmp;
      tmpList << "netSaveIP_" + tmp + "=" + fNetSettings.at(1);
      tmpList << "netSaveMask_" + tmp + "=" + fNetSettings.at(2);
      tmpList << "netSaveNameServer=" + fNetSettings.at(3);
      tmpList << "netSaveDefaultRouter=" + fNetSettings.at(4);
      tmpList << "netSaveIPv6=" + fNetSettings.at(5);
      tmpList << "netSaveIPv6NameServer=" + fNetSettings.at(6);
      tmpList << "netSaveIPv6DefaultRouter=" + fNetSettings.at(7);
    }
  }


  if ( QFile::exists("/trueos-media-network") )
  {
    // Doing install from network media
    tmpList << "installMedium=ftp";
    tmpList << "ftpPath=http://download.trueos.org/iso/%VERSION%/%ARCH%/dist";
  } else {
    // Doing install from /dist directory
    tmpList << "installMedium=local";
    tmpList << "localPath=/dist";
  }

  if ( comboLanguage->currentIndex() != 0 ) {
    QString lang = languages.at(comboLanguage->currentIndex());
    // Grab the language code
    lang.truncate(lang.lastIndexOf(")"));
    lang.remove(0, lang.lastIndexOf("(") + 1);
    tmpList << "";
    tmpList << "localizeLang=" + lang;
  }

  // Setup custom keyboard layouts
  /* KPM
  tmpList << "";
  tmpList << "# Keyboard Layout Options";
  tmp = comboBoxKeyboardModel->currentText();
  tmp.truncate(tmp.indexOf(")"));
  tmp = tmp.remove(0, tmp.indexOf(" (") + 2 );
  tmpList << "localizeKeyModel=" + tmp;

  tmp = listKbLayouts->currentItem()->text();
  tmp.truncate(tmp.indexOf(")"));
  tmp = tmp.remove(0, tmp.indexOf(" (") + 2 );
  tmpList << "localizeKeyLayout=" + tmp;

  tmp = listKbVariants->currentItem()->text();
  if ( tmp != "<none>" ) {
    tmp.truncate(tmp.indexOf(")"));
    tmp = tmp.remove(0, tmp.indexOf(" (") + 2 );
    tmpList << "localizeKeyVariant=" + tmp;
  }
  */

  tmpList << " ";

  return tmpList;
}

void Installer::startConfigGen()
{

  QStringList cfgList;

  // Generate the config file now
  cfgList+=getGlobalCfgSettings();

  // No disk config if doing an upgrade to existing zpool
  if ( zpoolTarget.isEmpty() ) {
    cfgList+=getDiskCfgSettings();
  }

  // We can skip these options if doing a restore
  if ( ! radioRestore->isChecked() ) {

    // Save the install config script to disk
    cfgList << "runExtCommand=/root/save-config.sh";

    // If doing install from package disk
    if ( hasPkgsOnMedia )
      cfgList+=getDeskPkgCfg();

    cfgList+= "";

    if ( radioDesktop->isChecked() ) {
      // Doing TrueOS Install

      QString lang;
      if ( comboLanguage->currentIndex() != 0 )
        lang = languages.at(comboLanguage->currentIndex()).section("(",1,1).section(")",0,0);
      else
        lang="en_US";

      // Setup the desktop
      cfgList << "runExtCommand=cp /root/defaultpkgbranch ${FSMNT}/root/defaultpkgbranch";
      cfgList << "runCommand=sh /usr/local/share/trueos/scripts/sys-init.sh desktop " + lang;

      // Setup for a fresh system first boot
      cfgList << "# Touch flags to enable TrueOS setup at first boot";
      cfgList << "runCommand=touch /var/.trueos-firstboot";
      cfgList << "runCommand=touch /var/.trueos-firstgui";

      // If we had to use a custom xorg.conf to run installer, use it again
      // on live system
      if ( QFile::exists("/etc/X11/xorg.conf") ) {
        cfgList << "runExtCommand=cp /etc/X11/xorg.conf ${FSMNT}/etc/X11/xorg.conf";
      }

      // Check if we need to load i915kms
      QProcess kldproc;
      kldproc.start(QString("kldstat"), QStringList());
      while(kldproc.state() == QProcess::Starting || kldproc.state() == QProcess::Running) {
        kldproc.waitForFinished(200);
        QCoreApplication::processEvents();
      }
      if ( kldproc.readAll().simplified().indexOf("i915kms") != -1 ) {
        cfgList << "runCommand=echo 'kldload_i915kms=\"i915kms\"' >> /etc/rc.conf";
      }

      if ( comboLanguage->currentIndex() != 0 ) {
        QString lang = languages.at(comboLanguage->currentIndex());
        // Grab the language code
        lang.truncate(lang.lastIndexOf(")"));
        lang.remove(0, lang.lastIndexOf("(") + 1);
        cfgList << "runCommand=echo " + lang + " >/var/.wizardlang";
      }

      // If the user changed the keyboard layout, save it for first boot
      if ( ! curKeyModel.isEmpty() ) {
        cfgList << "runCommand=echo \"" + curKeyModel + " " + curKeyLayout + " " + curKeyVariant + "\" >/var/.wizardKeyboard";
      }

    } else {
      // Doing TrueOS Install
      cfgList+=getUsersCfgSettings();

      // Enable SSH?
      if ( fSSH )
        cfgList << "runCommand=echo 'sshd_enable=\"YES\"' >>/etc/rc.conf";

      // Setup the TrueOS server
      cfgList << "runExtCommand=cp /root/defaultpkgbranch ${FSMNT}/root/defaultpkgbranch";
      cfgList << "runCommand=sh /usr/local/share/trueos/scripts/sys-init.sh server";

    }

    // Run newaliases to fix mail errors
    cfgList << "runCommand=newaliases";

  } // End of restore check

  // Now write out the cfgList to file
  QFile cfgfile( PCSYSINSTALLCFG );
  if ( cfgfile.open( QIODevice::WriteOnly ) ) {
    QTextStream stream( &cfgfile );
    for ( int i=0; i < cfgList.count(); ++i) {
      stream <<  cfgList.at(i) << "\n";
    }
    cfgfile.close();
  }
}

void Installer::slotHelp()
{
	pcHelp = new dialogHelp();
	switch (installStackWidget->currentIndex()) {
	case 0:
		pcHelp->dialogInit(HELPTEXT0);
		break;
	case 1:
		pcHelp->dialogInit(HELPTEXT1);
		break;
         case 2:
		pcHelp->dialogInit(HELPTEXT2);
		break;
	case 3:
		pcHelp->dialogInit(HELPTEXT3);
		break;
	case 4:
		pcHelp->dialogInit(HELPTEXT4);
		break;
	case 5:
		pcHelp->dialogInit(HELPTEXT5);
		break;
	default:
		pcHelp->dialogInit("No help text...");
		break;
	}
	pcHelp->show();
}

// Function which returns the pc-sysinstall cfg data
QStringList Installer::getDiskCfgSettings()
{
  QStringList tmpList;
  QList<QStringList> copyList;
  QString tmp, workingDisk, workingSlice, tmpSlice, XtraTmp, startPart, sliceSize;
  int disk = 0;

  // Copy over the list to a new variable we can mangle without modifying the original
  copyList = sysFinalDiskLayout;

  // Doing manual extraction
  if ( copyList.at(0).at(0) == "MANUAL" )
    return QStringList();

  while ( ! copyList.empty() )
  {
    workingDisk = copyList.at(0).at(0);
    workingSlice = copyList.at(0).at(1);
    tmpSlice = workingSlice;
    tmpList << "# Disk setup for " + workingDisk ;

    // Check if this is an install to "Unused Space"
    for (int z=0; z < sysDisks.count(); ++z)
      if ( sysDisks.at(z).at(0) == "SLICE" \
        && sysDisks.at(z).at(2) == workingDisk + workingSlice \
        && sysDisks.at(z).at(4) == "Unused Space" )
          tmpSlice = "free";

    tmpList << "disk" + tmp.setNum(disk) + "=" + workingDisk;

    if ( tmpSlice.at(0).isNumber() ) {
      // If we are installing to a GPT partition, mark it as such
      sysPartType="GPT";
      tmpList << "partition=p" + tmpSlice;
    } else {
      tmpList << "partition=" + tmpSlice;
    }

    // Which boot-loader are we stamping?
    tmpList << "bootManager=BSD";

    // Set the GPT/MBR options
    if ( sysPartType == "GPT" )
      tmpList << "partscheme=GPT";
    if ( sysPartType == "MBR" )
      tmpList << "partscheme=MBR";

    tmpList << "commitDiskPart";
    tmpList << "";

    // If after doing the mirror, our list is empty, break out
    if ( copyList.empty() )
      break;

    // Now print the partition section for this slice
    tmpList << "# Partition Setup for " + workingDisk + "(" + workingSlice + ")";
    tmpList << "# All sizes are expressed in MB";
    tmpList << "# Avail FS Types, UFS, UFS+S, UFS+SUJ, UFS+J, ZFS, SWAP";
    tmpList << "# UFS.eli, UFS+S.eli, UFS+SUJ, UFS+J.eli, ZFS.eli, SWAP.eli";

    // If there is a dedicated /boot partition, need to list that first, see what is found
    for (int i=0; i < copyList.count(); ++i) {
      QStringList mounts = copyList.at(i).at(2).split(",");
      for (int z = 0; z < mounts.size(); ++z) {
        if ( copyList.at(i).at(0) == workingDisk \
          && copyList.at(i).at(1) == workingSlice \
          && mounts.at(z) == "/boot" )
		startPart="/boot";
      }
    }

    // If no dedicated /boot partition, then lets list "/" first
    if(startPart.isEmpty())
	startPart="/";

    // Start by looking for the root partition
    for (int i=0; i < copyList.count(); ++i) {
      QStringList mounts = copyList.at(i).at(2).split(",");
      for (int z = 0; z < mounts.size(); ++z) {
        if ( copyList.at(i).at(0) == workingDisk \
          && copyList.at(i).at(1) == workingSlice \
          && ( mounts.at(z) == startPart || mounts.at(z).indexOf(startPart + "(") == 0 ) ) {

          // Check if we have any extra arguments to throw on the end
          XtraTmp="";
          if ( ! copyList.at(i).at(5).isEmpty() )
            XtraTmp=" (" + copyList.at(i).at(5) + ")" ;

          // Write out the partition line
          tmpList << "disk" + tmp.setNum(disk) + "-part=" \
                   + copyList.at(i).at(3) + " " + copyList.at(i).at(4) \
                   + " " + copyList.at(i).at(2) + XtraTmp;

          // Check if we have an encryption passphrase to use
          if ( ! copyList.at(i).at(6).isEmpty() )
	    tmpList << "encpass=" + copyList.at(i).at(6);

          // Done with this item, remove it now
          copyList.removeAt(i);
          break;
        }
      }
    }

    // Now look for SWAP
    for (int i=0; i < copyList.count(); ++i) {
      if ( copyList.at(i).at(0) == workingDisk \
        && copyList.at(i).at(1) == workingSlice \
        && copyList.at(i).at(2) == "SWAP.eli" ) {

        // Write the partition line
        tmpList << "disk" + tmp.setNum(disk) + "-part=" \
                 + copyList.at(i).at(3) + " " + copyList.at(i).at(4) \
                 + " none";

        // Done with this item, remove it now
        copyList.removeAt(i);
        break;
      }
    }

    // Now look for any other partitions
    int count = copyList.count();
    for (int i=0; i < count; ++i) {
      if ( copyList.at(i).at(0) == workingDisk \
        && copyList.at(i).at(1) == workingSlice ) {

        // Check if we have any extra arguments to throw on the end
        XtraTmp="";
        if ( ! copyList.at(i).at(5).isEmpty() )
          XtraTmp=" (" + copyList.at(i).at(5) + ")" ;

	// If we are working on the last partition, set the size to 0 to use remaining disk
	if ( i == (count - 1) )
		sliceSize = "0";
	else
		sliceSize=copyList.at(i).at(4);

        // Write the partition line
        tmpList << "disk" + tmp.setNum(disk) + "-part=" \
                 + copyList.at(i).at(3) + " " + sliceSize \
                 + " " + copyList.at(i).at(2) + XtraTmp;

        // Check if we have an encryption passphrase to use
        if ( ! copyList.at(i).at(6).isEmpty() )
          tmpList << "encpass=" + copyList.at(i).at(6);

        // Done with this item, remove it now
        copyList.removeAt(i);
        i--;
        count--;
      }
    }


    // Close out this partition section
    tmpList << "commitDiskLabel";
    tmpList << "";

    // Increment our disk counter
    disk++;
  }

  return tmpList;
}

// Slot which checks any disk requirements before procceding to the next page
bool Installer::checkDiskRequirements()
{
  // For now just return true, the wizard should handle making sure
  // the user doesn't shoot themselves in the foot during disk setup
  return true;
}

void Installer::loadPciConfInfo(){
  pciconf_info = QJsonObject(); //clear it
  QProcess proc;
  proc.start("pciconf -lv");
  proc.waitForFinished();
  QStringList lines = QString(proc.readAllStandardOutput()).split("\n");
  //qDebug() << "Raw pciconf:" << lines;
  QJsonObject cobj; QString cid;
  for(int i=0; i<lines.length(); i++){
    if(!lines[i].contains(" = ") && !cid.isEmpty()){ pciconf_info.insert(cid,cobj); cid.clear(); cobj = QJsonObject(); }
    if(lines[i].contains(" = ")){
      QString var = lines[i].section("=",0,0).simplified();
      QString val = lines[i].section("=",1,-1).simplified();
      if(val.startsWith("\'") && val.endsWith("\'")){ val.chop(1); val = val.remove(0,1); }
      qDebug() << "PCICONF LINE:" << lines[i];
	qDebug() << "\t\t" << var << val;
      cobj.insert(var,val);
    }else{
      //New device section
      cid = lines[i].section("@",0,0);
    }
  }
  if(!cid.isEmpty() && cobj.keys().isEmpty()){ pciconf_info.insert(cid,cobj); }
  qDebug() << "Found pciconf_info:" << pciconf_info;
}

// Function which begins the backend install, and connects slots to monitor it
void Installer::startInstall()
{
  cslide = -1;
  nextSlide();
  slideTimer->start();

  // Update the UI elements if doing a restore
  if ( radioRestore->isChecked() )
  {
      labelFinished->setText(tr("Your system is now restored!\nClick Exit to close. After rebooting you may eject the install media."));
      groupInstall->setTitle(tr("System Restore"));
      labelInstallHeader->setText(tr("Your system is now being restored, this may take a while depending upon the size of your backup and network conditions."));
  }

  QString cfgFile;
  if (customCfgFile.isEmpty() )
    cfgFile = PCSYSINSTALLCFG;
  else
    cfgFile = customCfgFile;

  // Disable the back / next buttons until we are finished
  nextButton->setEnabled(false);
  backButton->setEnabled(false);
  progressBarInstall->setValue(0);
  installFoundCounter = false;
  installFoundMetaCounter = false;
  installFoundFetchOutput = false;
  inZFSSend = false;

  // Setup some defaults for the secondary progress bar
  progressBarInstall2->setValue(0);
  progressBarInstall2->setHidden(true);
  labelInstallStatus2->setText("");
  labelInstallStatus2->setHidden(true);

  // Kill any hald instances, which causes failures to install when it
  // tries to mount our new partitions
  QProcess::execute("killall", QStringList() << "hald");

  // Hacky kludge, looks like Qt isn't writing file fully before starting installation
  system("sync");
  system("sleep 2");

  // Start our process to begin the install
  QString PCSYSINSTALL;
  if ( QFile::exists("/root/pc-sysinstall/pc-sysinstall") )
     PCSYSINSTALL = "/root/pc-sysinstall/pc-sysinstall";
  else
     PCSYSINSTALL = "/usr/local/sbin/pc-sysinstall";

  QString program = PCSYSINSTALL;
  QStringList arguments;
  arguments << "-c" << cfgFile;

  installProc = new QProcess();
  installProc->setProcessChannelMode(QProcess::MergedChannels);
  connect(installProc,SIGNAL(finished( int, QProcess::ExitStatus)),this,SLOT(slotInstallProcFinished( int, QProcess::ExitStatus)));
  connect(installProc,SIGNAL(readyRead()),this,SLOT(slotReadInstallerOutput()));
  installProc->start(program, arguments);

}

// Function run when the install failed to prompt user for course of action
void Installer::installFailed()
{
   QString sysLog;
   labelInstallStatus->setText(tr("Failed!"));

   QMessageBox msgBox;
   msgBox.setWindowTitle(tr("TrueOS Installer"));
   msgBox.setIcon(QMessageBox::Critical);
   msgBox.setText(tr("The installer has encountered an error and has been halted."));
   msgBox.setInformativeText(tr("Do you want to generate an error report?"));
   msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
   msgBox.setDefaultButton(QMessageBox::Yes);

   // If we have a log, show it in the detailed view button
   if ( QFile::exists("/tmp/.pc-sysinstall/pc-sysinstall.log") )
   {
     QFile logFile("/tmp/.pc-sysinstall/pc-sysinstall.log");
     if (logFile.open(QIODevice::ReadOnly | QIODevice::Text))
         while (!logFile.atEnd())
           sysLog = sysLog + logFile.readLine() + "\n";
     msgBox.setDetailedText(sysLog);
   }
   int ret = msgBox.exec();

   switch (ret) {
   case QMessageBox::Yes:
       // Generate the error report
       Scripts::Backend::createErrorReport();
       break;
   case QMessageBox::No: // :)
       break;
   }

   QMessageBox msgBox2;
   msgBox2.setWindowTitle(tr("TrueOS Installer"));
   msgBox2.setIcon(QMessageBox::Critical);
   msgBox2.setText(tr("Exit the installation now?") );
   msgBox2.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
   msgBox2.setDefaultButton(QMessageBox::Yes);
   msgBox2.setDetailedText(sysLog);

   ret = msgBox2.exec();

   switch (ret) {
   case QMessageBox::Yes:
       close();
       break;
   case QMessageBox::No: // :)
       break;
   }

}

// Slot which is called when the installation has finished
void Installer::slotInstallProcFinished( int exitCode, QProcess::ExitStatus status)
{
  slideTimer->stop();
  QString tmp;
  if ( status != QProcess::NormalExit || exitCode != 0 )
  {
     installFailed();
  } else {
    // Move to the final page, and show a Finish button
    proceed(true);
    nextButton->setEnabled(true);
    nextButton->setText(tr("&Finish"));
    nextButton->disconnect();
    connect(nextButton, SIGNAL(clicked()), this, SLOT(slotFinished()));
    backButton->setEnabled(false);
    abortButton->setEnabled(false);
    abortButton->setVisible(false);
  }
}

// Slot which reads the output of the installer
void Installer::slotReadInstallerOutput()
{
  QString tmp, line;
  int range;
  bool ok;


  while ( installProc->canReadLine() )
  {
     tmp = installProc->readLine();
     tmp.truncate(75);
     //qDebug() << tmp;

     // If doing a restore we can do all parsing right here
     if ( radioRestore->isChecked() ) {
       line = tmp;

       if ( tmp.contains("total estimated size"))
       {
          repTotalK = line.section(" ",-1).simplified();
	  double totSize = displayToDoubleK(repTotalK);
          progressBarInstall->setRange(0, totSize + 1024);
	  inZFSSend = true;
          continue;
       }
       if ( tmp.contains("Moving datasets to"))
          inZFSSend=false;

       if( line.contains("send from ") )
	 continue;
       if( line.contains("TIME ") )
         continue;

       if(tmp.endsWith("\n")){ tmp.chop(1); }
       labelInstallStatus->setText(tmp);

       if ( ! inZFSSend )
          continue;

       // We got here, lets parse the ZFS send message
       parseStatusMessage(tmp);

       continue;
     } // End of restore parsing

     // Parse fetch output
     if ( installFoundFetchOutput ) {
       if ( tmp.indexOf("SIZE: ") != -1 ) {

          // Get the total range first
          line = tmp;
          tmp = tmp.remove(0, tmp.indexOf(":") + 2 );
          tmp.truncate(tmp.indexOf(" "));
          range = tmp.toInt(&ok);
          if ( ok )
             progressBarInstall->setRange(0, range + 1);

          // Now get the current progress
          tmp = line;
          tmp = tmp.remove(0, tmp.indexOf(":") + 2 );
          tmp = tmp.remove(0, tmp.indexOf(":") + 2 );
          range = tmp.toInt(&ok);
          if ( ok )
             progressBarInstall->setValue(range);

	  continue;
        } else {
          installFoundFetchOutput = false;
	  break;
        }
     }

     // Unknown point in install
     if ( ! installFoundCounter && ! installFoundMetaCounter ) {

        // Check if we've found fetch output to update the progress bar with
        if ( tmp.indexOf("FETCH: ") != -1 ) {
          installFoundFetchOutput = true;
          break;
        }

        if ( tmp.indexOf("INSTALLCOUNT: ") != -1 ) {
          tmp = tmp.remove(0, tmp.indexOf(":") + 1 );
          range = tmp.toInt(&ok);
          if ( ok ) {
             range = range + 50;
             progressBarInstall->setRange(0, range + 1);
             installFoundCounter = true;
	     if ( availDesktopPackageData )
                labelInstallStatus->setText(tr("Extracting system..."));
	     else
                labelInstallStatus->setText(tr("Installing system... This may take a while..."));
          }

	  break;

        }

        // Check if we are on the meta-pkg installation
        if ( tmp.indexOf("Packages to install: ") != -1 ) {
          tmp = tmp.remove(0, tmp.indexOf(":") + 1 );
          range = tmp.toInt(&ok);
          if ( ok ) {
             progressBarInstall->setRange(0, range + 1);
             progressBarInstall->setValue(0);
             progressBarInstall2->setRange(0, 0);
             labelInstallStatus2->setHidden(false);
             progressBarInstall2->setHidden(false);
             installFoundMetaCounter = true;
             installFoundCounter = false;
             labelInstallStatus->setText(tr("Installing packages... This may take a while..."));
	     continue;
          }

        }
	if(tmp.endsWith("\n")){ tmp.chop(1); }
        labelInstallStatus->setText(tmp);
        continue;
     }

     // Doing file-extraction still
     if ( installFoundCounter ) {

       // Doing dist-files, may have multiple images to extract
       if ( tmp.indexOf("INSTALLCOUNT: ") != -1 ) {
         tmp = tmp.remove(0, tmp.indexOf(":") + 1 );
         range = tmp.toInt(&ok);
         if ( ok ) {
            progressBarInstall->setRange(0, range + 1);
            installFoundCounter = true;
            if ( availDesktopPackageData )
              labelInstallStatus->setText(tr("Extracting system..."));
            else
              labelInstallStatus->setText(tr("Installing system... This may take a while..."));
         }
	 break;
       }


       // Increment the progress
       progressBarInstall->setValue(progressBarInstall->value() + 1);

       // We've reached the end of this counted section
       if ( tmp.indexOf("Extraction Finished") != -1 ) {
         installFoundCounter = false;
         progressBarInstall->setRange(0, 0);
       }

       continue;
     }

     // Doing meta-pkgs
     if ( installFoundMetaCounter ) {
	// Check if we are on the next meta-pkg
        if ( tmp.indexOf("Installing package: ") != -1 ) {
           progressBarInstall->setValue(progressBarInstall->value() + 1);
           labelInstallStatus->setText(tr("Installing meta-package: %1").arg(tmp.section(":", 1, 5)));
	   continue;
	}

        if ( tmp.indexOf("Package installation complete!") != -1 ) {
           installFoundMetaCounter = false;
           progressBarInstall->setRange(0, 0);
           progressBarInstall2->setHidden(true);
           labelInstallStatus2->setHidden(true);
	   continue;
        }

        // Cleanup the installation line
	if ( tmp.indexOf("-- ") != -1 )
	   tmp = tmp.remove(0, tmp.indexOf("-- ") + 3);

	// Show other pkgng output text now
        if(tmp.endsWith("\n")){ tmp.chop(1); }
        labelInstallStatus2->setText(tmp);
     }

  } // end of while loop
}

// Start xvkbd
void Installer::slotPushVirtKeyboard()
{
   system("killall -9 xvkbd; xvkbd -compact &");
}

// Return the configuration for desktop packages
QStringList Installer::getDeskPkgCfg()
{
   QStringList cfgList, pkgList;
   QString line;

   // Server packages
   if ( radioServer->isChecked() ) {
     pkgList << "misc/trueos-server";
   }

   // Desktop packages
   if ( radioDesktop->isChecked() ) {
     // Our default list of packages that makeup a desktop
     // This is always able to be changed by user post-install
     pkgList << "misc/trueos-desktop" << "x11/lumina";
     pkgList << "sysutils/fusefs-ntfs" << "graphics/drm-next-kmod";

     // The default web-browser and plugins
     pkgList <<  "www/firefox" << "www/qupzilla-qt5" << "www/falkon";

     // The default mail client
     pkgList << "mail/thunderbird" << "mail/trojita";

     // Multimedia player
     pkgList << "multimedia/vlc" << "multimedia/openh264" << "multimedia/libdvdcss" << "audio/pianobar";

     // VirtualBox
     //pkgList << "misc/trueos-meta-virtualbox";

     // Office Suite
     //pkgList << "editors/libreoffice";

     //Mouse Cursors
     pkgList << "x11-themes/cursor-jimmac-theme";

     // Utilities
     pkgList << "graphics/phototonic" << "misc/trueos-meta-hunspell" << "x11/qterminal";
     pkgList << "graphics/xsane" << "print/qpdfview";

     // Printer packages
     pkgList << "print/cups-pdf" << "print/cups-filters" << "print/gutenprint";

     // Include i18n stuff?
     if ( comboLanguage->currentIndex() != 0 )
       pkgList << "x11-fonts/noto";
     else
       pkgList << "x11-fonts/noto-lite";

     // Extra fonts
     pkgList << "x11-fonts/droid-fonts-ttf";
     //Add in all the user-selected packages
     pkgList << optionalPackages();
     // i18n packages, will eventually go away
     //pkgList << "misc/trueos-i18n";

     // Check if we are using NVIDIA driver and include it automatically
     /*QFile file("/var/log/Xorg.0.log");
     if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
       QTextStream in(&file);
       while (!in.atEnd()) {
          QString line = in.readLine();
          if ( line.indexOf("NVIDIA Unified Driver") != -1 ) {
            pkgList << "x11/nvidia-driver";
            //pkgList << "x11/nvidia-settings";
            //pkgList << "x11/nvidia-xconfig";
            break;
          }
       }
       file.close();
     }*/ // Done with NVIDIA check

     // Are we on VirtualBox or VMware?
     /*QFile filev("/var/log/Xorg.0.log");
     if (filev.open(QIODevice::ReadOnly | QIODevice::Text)) {
       QTextStream inv(&filev);
       while (!inv.atEnd()) {
          QString line = inv.readLine();
          if ( line.indexOf("VirtualBox") != -1 ) {
            pkgList << "emulators/virtualbox-ose-additions";
            break;
          }
          if ( line.indexOf("VMware") != -1 ) {
            pkgList << "emulators/open-vm-tools";
            break;
          }
       }
       filev.close();
     }*/ // End of VM checks

     // End of desktop packages
   }

   cfgList << "installPackages=" + pkgList.join(" ");
   return cfgList;
}

// Return list of users in cfg format
QStringList Installer::getUsersCfgSettings()
{
   QStringList userList;

   userList << "";
   userList << "# Root Password";
   userList << "rootPass=" + fRootPW;

   userList << "";
   userList << "# Users";
   userList << "userName=" + fUserName;
   userList << "userComment=" + fName;
   userList << "userPass=" + fUserPW;
   userList << "userShell=" + fShell;
   userList << "userHome=/home/" + fUserName;
   userList << "userGroups=wheel,operator,video";
   userList << "commitUser";
   userList << "";

   return userList;
}

void Installer::closeEvent(QCloseEvent *event)
{
    int ret = QMessageBox::question(this, tr("TrueOS Installer"),
                                tr("Are you sure you want to abort this installation?"),
                                QMessageBox::No | QMessageBox::Yes,
                                QMessageBox::No);
    switch (ret) {
    case QMessageBox::Yes:
        //exit the installer :(
        break;
    case QMessageBox::No: // :)
        event->ignore();
        break;
    }
}

void Installer::checkSpaceWarning()
{
  int totalSize = -1;
  int targetSize;
  int targetLoc;
  bool ok;
  QString workingDisk = sysFinalDiskLayout.at(0).at(0);
  QString workingSlice = sysFinalDiskLayout.at(0).at(1);
  QString targetType;
  QString target;
  //qDebug() << "Disk layout:" << workingDisk << workingSlice;

  // Only check full-disk sizes. Other size checks are done in disk wizard
  if ( workingSlice != "ALL" )
    return;

  targetType = "DRIVE";
  target = workingDisk;
  targetLoc = 1;

  // Lets get the size for this disk / partition
  for (int i=0; i < sysDisks.count(); ++i) {
      // Make sure to only add the slices to the listDiskSlices
      if ( sysDisks.at(i).at(0) == targetType && target == sysDisks.at(i).at(targetLoc))
        totalSize = sysDisks.at(i).at(targetLoc + 1).toInt(&ok);
  }

  //qDebug() << totalSize;

  if ( radioServer->isChecked() )
     targetSize=10000;
  else
     targetSize=10000;

  int tGB = targetSize / 1000;

  qDebug() << totalSize << targetSize;

  // Lets print a nice handy warning for users with possible
  // low disk space issues
  if ( totalSize < targetSize ) {
  QMessageBox::warning(this, tr("TrueOS Installer"),
      QString(tr("Warning: The selected disk / partition is less than recommended %1GB.")).arg(tGB),
      QMessageBox::Ok,
      QMessageBox::Ok);
      haveWarnedSpace = true;
  }

  return;
}

void Installer::slotSaveConfigUSB()
{
  int ret = QMessageBox::question(this, tr("TrueOS Installer"),
           tr("This will save your installation configuration to a MSDOSFS/FAT32 formatted USB stick. Continue?"),
           QMessageBox::No | QMessageBox::Yes,
           QMessageBox::No);
  switch (ret) {
  case QMessageBox::Yes:
      break;
  case QMessageBox::No: // :)
      return;
      break;
  }

  // Prompt to insert USB stick
  QMessageBox::information(this, tr("TrueOS Installer"),
          tr("Please insert a USB stick now, and click OK to continue."),
          QMessageBox::Ok,
          QMessageBox::Ok);

  // While USB is settling, this is a good time to ask for the config nickname
  bool ok;
  QString cfgName = QInputDialog::getText(this, tr("TrueOS Installer"),
                 tr("Please enter the nickname you want to save this configuration as."),
                 QLineEdit::Normal,
                 QString("default"), &ok);
  if (!ok || cfgName.isEmpty())
     return;

  // Now lets try to save the media
  qDebug() << "Running: /root/save-to-usb.sh" << cfgName;
  QProcess m;
  m.start(QString("qterminal"), QStringList() << "-e" << "/root/save-to-usb.sh" << cfgName);
  while(m.state() == QProcess::Starting || m.state() == QProcess::Running) {
     m.waitForFinished(200);
     QCoreApplication::processEvents();
  }

  if ( m.exitCode() != 0 ) {
     QMessageBox::critical(this, tr("TrueOS Installer"),
          tr("Failed saving config to USB media. Is the device working and formatted MSDOSFS/FAT32?"),
          QMessageBox::Ok,
          QMessageBox::Ok);

  } else {
     QMessageBox::information(this, tr("TrueOS Installer"),
          tr("Configuration saved! You may now safely remove the USB media."),
          QMessageBox::Ok,
          QMessageBox::Ok);
  }

}

void Installer::slotLoadConfigUSB()
{
  int ret = QMessageBox::question(this, tr("TrueOS Installer"),
           tr("Load an installation configuration file from a MSDOSFS/FAT32 formatted USB stick?"),
           QMessageBox::No | QMessageBox::Yes,
           QMessageBox::No);
  switch (ret) {
  case QMessageBox::Yes:
      break;
  case QMessageBox::No: // :)
      return;
      break;
  }

  // Prompt to insert USB stick
  QMessageBox::information(this, tr("TrueOS Installer"),
          tr("Please insert the USB stick now, and click OK to continue."),
          QMessageBox::Ok,
          QMessageBox::Ok);

  // Now lets try to load the config files
  qDebug() << "Running: /root/get-from-usb.sh";
  QProcess m;
  m.start(QString("/root/get-from-usb.sh"), QStringList());
  while(m.state() == QProcess::Starting || m.state() == QProcess::Running) {
     m.waitForFinished(200);
     QCoreApplication::processEvents();
  }

  if ( m.exitCode() != 0 ) {
     QMessageBox::critical(this, tr("TrueOS Installer"),
          tr("Failed loading saved config file from the USB media. Is the device working and formatted MSDOSFS/FAT32?"),
          QMessageBox::Ok,
          QMessageBox::Ok);
     return;

  } else {
     QMessageBox::information(this, tr("TrueOS Installer"),
          tr("Configuration loaded! You may now safely remove the USB media."),
          QMessageBox::Ok,
          QMessageBox::Ok);
  }


  // Yay! Now lets prompt the user as to which config to use
  QDir cDir;
  cDir.setPath("/tmp/pc-sys");
  QStringList cfgs = cDir.entryList(QDir::Files);

  bool ok;
  QString cfgFile = QInputDialog::getItem(this, tr("TrueOS Installer"),
                                       tr("Config File:"), cfgs, 0, false, &ok);
  if (!ok || cfgFile.isEmpty()) {
    QMessageBox::critical(this, tr("TrueOS Installer"),
          tr("Canceled config script install"),
          QMessageBox::Ok,
          QMessageBox::Ok);
    return;
  }

  // Read the contents of this file
  QStringList fileContents;
  QFile file("/tmp/pc-sys/" + cfgFile);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, tr("TrueOS Installer"),
          tr("Failed reading /tmp/pc-sys/") + " " + cfgFile,
          QMessageBox::Ok,
          QMessageBox::Ok);
    return;
  }

  while (!file.atEnd())
    fileContents << file.readLine();
  file.close();

  // Display the file in an OK information box so the user can inspect it
  QMessageBox::information(this, tr("TrueOS Installer Config Script"), fileContents.join("\n"), QMessageBox::Ok, QMessageBox::Ok);

  ret = QMessageBox::question(this, tr("TrueOS Installer"),
           tr("Start the install using this config file?") + "\n" + cfgFile,
           QMessageBox::No | QMessageBox::Yes,
           QMessageBox::No);
  switch (ret) {
  case QMessageBox::Yes:
      break;
  case QMessageBox::No: // :)
      return;
      break;
  }

  // Time to start the installation with our custom config file
  qDebug() << cfgFile;
  customCfgFile = "/tmp/pc-sys/" + cfgFile;
  installStackWidget->setCurrentIndex(3);
  startInstall();
}

void Installer::slotStartNetworkManager() {
  system("/usr/local/bin/pc-netmanager -installer &");
}

void Installer::slotStartDiskManager() {
  system("/usr/local/bin/pc-zmanager &");
}

void Installer::slotSaveRestoreSettings(QStringList Opts)
{
  qDebug() << "slotSaveRestoreSettings";
  restOpts = Opts;

  textEditDiskSummary->clear();
  QStringList summary = getDiskSummary();
  for ( int i=0; i < summary.count(); ++i)
    textEditDiskSummary->append(summary.at(i));

  textEditDiskSummary->moveCursor(QTextCursor::Start);

  startConfigGen();
  installStackWidget->setCurrentIndex(installStackWidget->currentIndex() + 2);
}

void Installer::parseStatusMessage(QString stat){
  //qDebug() << "msg:" << stat;
  QString dispTotal;

  //Divide up the status message into sections
  stat.replace("\t"," ");
  QString cSize = stat.section(" ",1,1,QString::SectionSkipEmpty);

  //Now Setup the tooltip
  if(!repTotalK.isEmpty()){
    double c = displayToDoubleK(cSize);
    progressBarInstall->setValue(c);
    dispTotal = repTotalK;
  } else {
    dispTotal = "??";
  }

  //Format the tooltip String
  QString status = cSize+"/"+dispTotal;

  QString txt = QString(tr("Restoring system: %1")).arg(status);
  txt = txt.simplified();
  while(txt.endsWith("\n" || txt.endsWith(" "))){ txt.chop(1); }
  labelInstallStatus->setText(txt);

}

double Installer::displayToDoubleK(QString displayNumber){
  QStringList labels;
    labels << "K" << "M" << "G" << "T" << "P" << "E";
  QString clab = displayNumber.right(1); //last character is the size label
        displayNumber.chop(1); //remove the label from the number
  double num = displayNumber.toDouble();
  //Now format the number properly
  bool ok = false;
  for(int i=0; i<labels.length(); i++){
    if(labels[i] == clab){ ok = true; break; }
    else{ num = num*1024; } //get ready for the next size
  }
  if(!ok){ num = -1; } //could not determine the size
  return num;
}


void Installer::slotEmergencyShell() {
  system("qterminal -e /root/TrueOSUtil.sh &");
}

// Slot to update the install slideshow
void Installer::nextSlide(){
  cslide++; //go to the next slide
  qDebug() << "Installer slide:" << cslide;
  if(cslide<0 || cslide>=numSlides()){ cslide = 0; } //back to beginning
  QSize sz = label_slide_icon->size();
    qDebug() << " - Size:" << sz << label_slide_icon->sizeHint() << installStackWidget->size();
  if(sz.width() << label_slide_icon->sizeHint().width() ){  sz = installStackWidget->size(); } //for the first slide - use the window size
  loadSlide(label_slide_text, label_slide_icon, cslide, sz);
}

//Slots related to the optional packages
void Installer::updatePkgLists(){
  //Read the system information
  QStringList devs = pciconf_info.keys().filter("vgapci");
  for(int i=0; i<devs.length(); i++){
    devs[i] = pciconf_info.value(devs[i]).toObject().value("vendor").toString() + " " +pciconf_info.value(devs[i]).toObject().value("device").toString();
  }
  label_graphics_device->setText( label_graphics_device->text().arg(devs.join("\n")) );
  //Need to add logic here to detect/recommend drivers TODO
  for(int i=0; i<tree_graphics_drivers->topLevelItemCount(); i++){
    for(int j=0; j<tree_graphics_drivers->topLevelItem(i)->childCount(); j++){
      QTreeWidgetItem *it = tree_graphics_drivers->topLevelItem(i)->child(j);
      if(it->whatsThis(0).isEmpty()){ it->setDisabled(true); }
      if(!it->isDisabled()){ it->setCheckState(0, Qt::Unchecked); }
      if(it->text(0).contains("%1")){
        //Read the version from the available pkg and put it here (TODO)
        QString version = "";
        QString text = it->text(0).arg(version);
        text.replace("()",""); //just in case version information could not be detected
        it->setText(0, text.simplified() );
      }
    }
  }
}

void Installer::pkgChanged(QTreeWidgetItem *it){
  if(!it->whatsThis(0).contains("nvidia-driver") || it->checkState(0)!=Qt::Checked ){ return; } //nothing special to do
  for(int i=0; i<it->parent()->childCount(); i++){
    QTreeWidgetItem *chk = it->parent()->child(i);
    if(it==chk || !chk->whatsThis(0).contains("nvidia-driver") || chk->checkState(0)!=Qt::Checked ){ continue; }
    chk->setCheckState(0, Qt::Unchecked);
  }
}

QStringList Installer::optionalPackages(){
  QStringList sel;
  for(int i=0; i<tree_graphics_drivers->topLevelItemCount(); i++){
    for(int j=0; j<tree_graphics_drivers->topLevelItem(i)->childCount(); j++){
      QTreeWidgetItem *it = tree_graphics_drivers->topLevelItem(i)->child(j);
      if(it->checkState(0)==Qt::Checked && !it->whatsThis(0).isEmpty()){ sel << it->whatsThis(0).split(","); }
    }
  }
  return sel;
}

void Installer::slotBEInstallToggled(bool inBE){
  group_install_full->setEnabled(!inBE);
  group_install_full->setVisible(!inBE);
  group_install_be->setEnabled(inBE);
  group_install_be->setVisible(inBE);
  if(inBE){
    zpoolTarget = combo_install_pools->currentText();
  }else{
    zpoolTarget.clear();
   }
  //Now re-load the summary
  startConfigGen();
  textEditDiskSummary->clear();
  QStringList summary = getDiskSummary();
  for ( int i=0; i < summary.count(); ++i){
    textEditDiskSummary->append(summary.at(i));
  }
  textEditDiskSummary->moveCursor(QTextCursor::Start);
}
