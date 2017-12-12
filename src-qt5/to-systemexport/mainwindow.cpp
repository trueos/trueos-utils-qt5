#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
//#include <QTest>
#include <ExternalProcess.h>
//#include <LUtils.h>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow){
    ui->setupUi(this);
    QString title = tr("System Exporter");
    this->setWindowTitle(title);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_exportButton_clicked(){
  //create new tar file in location of choice
  QList< ExternalProcess* > procs;
  files.clear();
  exportFile = QFileDialog::getSaveFileName(this, tr("Where do you want to export?"), QDir::homePath());
  // Now add the files chosen
  // files
  if(ui->checkBox_fstab->isChecked()){ files  << "/etc/fstab"; }
  if(ui->checkBox_group->isChecked()){ files  << "/etc/group"; }
  if(ui->checkBox_hostname->isChecked()){ files  << "/etc/hostname"; }
  if(ui->checkBox_hostsdeniedssh->isChecked()){ files  << "/etc/hosts.deniedssh"; }
  if(ui->checkBox_hostid->isChecked()){ files  << "/etc/hostid"; }
  if(ui->checkBox_inetd.conf->isChecked()){ files  << "/etc/inetd.conf"; }
  if(ui->checkBox_loginconf->isChecked()){ files  << "/etc/login.conf"; }
  if(ui->checkBox_masterpassword->isChecked()){ files  << "/etc/master.password"; }
  if(ui->checkBox_pcdmconf->isChecked()){ files  << "/usr/local/etc/pcdm.conf"; }
  if(ui->checkBox_passwd->isChecked()){ files  << "/etc/passwd"; }
  if(ui->checkBox_resolvconf->isChecked()){ files  << "/etc/resolv.conf"; }
  if(ui->checkBox_shells->isChecked()){ files  << "/etc/shells"; }
  if(ui->checkBox_rcconf->isChecked()){ files  << "/etc/rc.conf"; }
  if(ui->checkBox_loaderconf->isChecked()){ files  << "/boot/loader.conf"; }
  if(ui->checkBox_ipfwcustom->isChecked()){ files  << "/etc/ipfw.custom"; }
  if(ui->checkBox_sysctlconf->isChecked()){ files  << "/etc/sysctl.conf"; }
  if(ui->checkBox_syslogconf->isChecked()){ files  << "/etc/syslog.conf"; }
  if(ui->checkBox_wpasupplicantconf->isChecked()){ files  << "/etc/wpa_supplicant.conf"; }
  if(ui->checkBox_xorgconf->isChecked()){ files  << "/etc/X11/xorg.conf"; }
  if(ui->checkBox_installedpkglist->isChecked()){ files << "/usr/local/log/pc-updatemanager/install-pkg-list"; }
  // directories
  if(ui->checkBox_cups_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/cups.tgz /usr/local/etc/cups/*");
    files << "/tmp/cups.tgz";
  }
  if(ui->checkBox_confd_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/confd.tgz /etc/conf.d/*") ;
    files << "/tmp/confd.tgz";
  }
  if(ui->checkBox_etc_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/etc.tgz /etc/*") ;
    files << "/tmp/etc.tgz";
  }
  if(ui->checkBox_etcssh_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/etcssh.tgz /etc/ssh/*") ;
    files << "/tmp/etcssh.tgz";
  }
  if(ui->checkBox_usrlocaletc_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/usrlocaletc.tgz /usr/local/etc/*") ;
    files << "/tmp/usrlocaletc.tgz";
  }
  if(ui->checkBox_openvpn_dir->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/openvpn.tgz /usr/local/etc/openvpn/*") ;
    files << "/tmp/openvpn.tgz";
  }
  if(ui->checkBox_lumina_dir->isChecked()){
    luminaPathString = QDir::homePath() + ".config/lumina-desktop/*";
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/lumina.tgz" + " " + luminaPathString;) ;
    files << "/tmp/lumina.tgz";
  }
  for(int i=0; i<files.length(); i++){
    if(!QFile::exists(files[i])){ qDebug() << "File Not Found:" << files[i]; files.removeAt(i); i--; }
  }
  qDebug() << "files" << files;

  //maybe insert wait time for External Processes to settle
  //QTimer *pause = new QTimer;
  //pause->setSingleShot(true);
  //pause->start(2000);
  bool wait = true;
  while(wait){
    wait = false;
    for(int i=0; i<procs.length() && !wait; i++){
      if(procs[i]!=0){ wait = procs[i]->state()!=QProcess::NotRunning; }
    }
    if(wait){ qDebug() << "Waiting on processes...."; QApplication::processEvents(QEventLoop::AllEvents, 100); QThread::msleep(500); }
  }
  ExternalProcess::launch("lumina-archiver", QStringList() << "--aa" << exportFile << files);
  //launchString = "lumina-archiver --aa " + exportFile + " " + files;
  //ExternalProcess::launch(launchString);
  //qDebug() << "exportFile" << exportFile;
  // wait until archiver closes
  //efinishedMessage();
  if(ui->echeckBox_homedir->isChecked()){ exportHomeDir(); }
  for(int i=0; i<procs.length(); i++){ procs[i]->deleteLater(); }
}

void MainWindow::efinishedMessage(){
  QMessageBox *finishedMessageBox = new QMessageBox;
  finishedMessageBox->setText(tr("The Export has finished."));
  QPushButton *pushButtonOk = finishedMessageBox->addButton(tr("Ok"), QMessageBox::YesRole);
  finishedMessageBox->QDialog::setWindowTitle(tr("Finished"));
  finishedMessageBox->show();
}

void MainWindow::exportHomeDir(){
  // to do :  Check size of home dir; set size in message; refuse to export if location does not have enough space;
  homePathString = QDir::homePath() + "*";
  homeDirArchive = QFileDialog::getSaveFileName(this, tr("Where do you want to export your home directory?"), QDir::homePath());
  homeExport = "lumina-archiver --aa " + homeDirArchive + " " + homePathString;
  ExternalProcess::launch(homeExport);
  //qDebug() << "homeExport" << homeExport;
  // wait until archiver closes
  // efinishedMessage();
}


void MainWindow::on_importButton_clicked(){
  //create new tar file in location of choice
  QString importfile = QFileDialog::getOpenFileName(this, tr("Open Exported Archive"), QDir::homePath());

  //export to tmp dir
  QDir().mkdir("/tmp/importdir");
  homeImport = "lumina-archiver --sx " + importFile + " /tmp/importdir/";
  ExternalProcess::launch(homeImport);

  // Now add selections
  // files
  if(ui->checkBox_fstab->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/fstab /etc/fstab"; }
  if(ui->checkBox_group->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/group /etc/group"; }
  if(ui->checkBox_hostname->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/hostname /etc/hostname"; }
  if(ui->checkBox_hostsdeniedssh->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/hosts.deniedssh /etc/hosts.deniedssh"; }
  if(ui->checkBox_hostid->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/hostid /etc/hostid"; }
  if(ui->checkBox_inetd.conf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/inetd.conf /etc/inetd.conf"; }
  if(ui->checkBox_loginconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/login.conf /etc/login.conf"; }
  if(ui->checkBox_masterpassword->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/master.password /etc/master.password"; }
  if(ui->checkBox_pcdmconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/pdcm.conf /usr/local/etc/pcdm.conf"; }
  if(ui->checkBox_passwd->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/passwd /etc/passwd"; }
  if(ui->checkBox_resolvconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/resolv.conf /etc/resolv.conf"; }
  if(ui->checkBox_shells->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/shells /etc/shells"; }
  if(ui->checkBox_rcconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/rc.conf /etc/rc.conf"); }
  if(ui->checkBox_loaderconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/loader.conf /etc/loader.conf"); }
  if(ui->checkBox_ipfwcustom->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/ipfw.custom /etc/ipfw.custom"); }
  if(ui->checkBox_sysctlconf->isChecked()){  ExternalProcess::launch("cp /tmp/importdir/sysctl.conf /etc/sysctl.conf"); }
  if(ui->checkBox_syslogconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/syslog.conf /etc/syslog.conf"); }
  if(ui->checkBox_wpasupplicantconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/wpa_supplicant.conf /etc/wpa_supplicant.conf"); }
  if(ui->checkBox_xorgconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/xorg.conf /etc/X11/xorg.conf"); }
  // directories
  if(ui->checkBox_cupsconfs->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/cups.tgz /usr/local/etc/cups/" ); }
  if(ui->checkBox_confd->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/confd.tgz /etc/conf.d/"); }
  if(ui->checkBox_installedpkglist->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/pc-updatemanager/install-pkg-list " + QDir::homePath() ); }
  if(ui->checkBox_etc_dir->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/etcdir.tgz /etc/" ); }
  if(ui->checkBox_etcssh_dir->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/etcssh.tgz /etc/ssh/" ); }
  if(ui->checkBox_usrlocaletc->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/usrlocaletc.tgz /usr/local/etc/" ); }
  if(ui->checkBox_openvpn_dir->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/openvpn.tgz /usr/local/etc/openvpn/" ); }
  if(ui->checkBox_luimina_dir->isChecked()){
    luminaPathString = QDir::homePath() + ".config/lumina-desktop/*";
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/lumina.tgz" + " " + luminaPathString); }
  // packagelist
  // in future add option to install previous packages
  QMessageBox *pkglistMessageBox = new QMessageBox;
  pkglistMessageBox->setText(tr("Your previously installed package list can be found in your home directory"));
  QPushButton *pushButtonOk = pkglistMessageBox->addButton(tr("Ok"), QMessageBox::YesRole);
  pkglistMessageBox->QDialog::setWindowTitle(tr("Package List"));
  pkglistMessageBox->show();
  //qDebug() << "exportFile" << exportFile;
  // wait until archiver closes
  //ifinishedMessage();
  if(ui->checkBox_homedir->isChecked()){ importHomeDir(); }
}

void MainWindow::importHomeDir(){
  importPath = QDir::homePath();
  QString importhomefile = QFileDialog::getOpenFileName(this, tr("Open Exported Archive"), QDir::homePath());
  // to do :  Check size of home dir; set size in message; refuse to export if location does not have enough space;
  homeImport = "lumina-archiver --sx " + importFile + " " + importPath;
  ExternalProcess::launch(homeImport) ;
  // wait until archiver closes
  // efinishedMessage();
}

void MainWindow::ifinishedMessage(){
  QMessageBox *finishedMessageBox = new QMessageBox;
  finishedMessageBox->setText(tr("The Export has finished."));
  QPushButton *pushButtonOk = finishedMessageBox->addButton(tr("Ok"), QMessageBox::YesRole);
  finishedMessageBox->QDialog::setWindowTitle(tr("Finished"));
  finishedMessageBox->show();
}
