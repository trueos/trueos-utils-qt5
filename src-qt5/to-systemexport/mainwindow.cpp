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
#include <QDirIterator>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow){
    ui->setupUi(this);
    QString title = tr("System Exporter");
    this->setWindowTitle(title);
}

MainWindow::~MainWindow(){
    delete ui;
}

// ------------ //
//    EXPORT
// ------------ //


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
  if(ui->checkBox_inetdconf->isChecked()){ files  << "/etc/inetd.conf"; }
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
    command = "tar -czpf /tmp/cups.tgz /usr/local/etc/cups";
    procs << ExternalProcess::launch(command);
    files << "/tmp/cups.tgz";
  }
  if(ui->checkBox_confd_dir->isChecked()){
    command = "tar -czpf /tmp/confd.tgz /etc/conf.d";
    procs << ExternalProcess::launch(command);
    files << "/tmp/confd.tgz";
  }
  if(ui->checkBox_etc_dir->isChecked()){
    command = "tar -czpf /tmp/etc.tgz /etc";
    procs << ExternalProcess::launch(command);
    files << "/tmp/etc.tgz";
  }
  if(ui->checkBox_etcssh_dir->isChecked()){
    command = "tar -czpf /tmp/etcssh.tgz /etc/ssh";
    procs << ExternalProcess::launch(command);
    files << "/tmp/etcssh.tgz";
  }
  if(ui->checkBox_usrlocaletc_dir->isChecked()){
    command = "tar -czpf /tmp/usrlocaletc.tgz /usr/local/etc";
    procs << ExternalProcess::launch(command);
    files << "/tmp/usrlocaletc.tgz";
  }
  if(ui->checkBox_openvpn_dir->isChecked()){
    command = "tar -czpf /tmp/openvpn.tgz /usr/local/etc/openvpn";
    procs << ExternalProcess::launch(command);
    files << "/tmp/openvpn.tgz";
  }
  if(ui->checkBox_lumina_dir->isChecked()){
    luminaPathString = QDir::homePath() + ".config/lumina-desktop/*"; // this needs fixin
    luminaExport = "tar -czpf /tmp/lumina.tgz " + luminaPathString;
    procs << ExternalProcess::launch(luminaExport) ;
    files << "/tmp/lumina.tgz";
  }

  // wait for external processes to finish

  bool wait = true;
  while(wait){
    wait = false;
    for(int i=0; i<procs.length() && !wait; i++){
      if(procs[i]!=0){ wait = procs[i]->state()!=QProcess::NotRunning; }
    }
    if(wait){ qDebug() << "Waiting on processes...."; QApplication::processEvents(QEventLoop::AllEvents, 100); QThread::msleep(500); }
  }
  for(int i=0; i<files.length(); i++){
    if(!QFile::exists(files[i])){ qDebug() << "File Not Found:" << files[i]; files.removeAt(i); i--; }
  }
  //qDebug() << "files" << files;

  fileList = files.join(" ");
  overallExportCommand = "tar -czpf " + exportFile + " "    + fileList;
  //qDebug() << "finalcommand" << overallExportCommand;
  ExternalProcess::launch(overallExportCommand);
  //qDebug() << "overall command run";

  if(ui->checkBox_homedir->isChecked()){ exportHomeDir(); }
  else { efinishedMessage(); }
  for(int i=0; i<procs.length(); i++){ procs[i]->deleteLater(); }
}

void MainWindow::efinishedMessage(){
  efinishedMessageBox.setText(tr("The Export of selected files/dirs has finished."));
  efinishedMessageBox.setStandardButtons(QMessageBox::Yes);
  efinishedMessageBox.show();
}


void MainWindow::exportHomeDir(){
  QList< ExternalProcess* > procs;
  // to do :  Check size of home dir; set size in message; refuse to export if location does not have enough space;
  homePathString = QDir::homePath();
  homeDirArchive = QFileDialog::getSaveFileName(this, tr("Where do you want to export your home directory?"), QDir::homePath());
  homeExport = "tar -czvpf " + homeDirArchive + " " + homePathString;
  procs << ExternalProcess::launch(homeExport);
  bool wait = true;
  while(wait){
    wait = false;
    for(int i=0; i<procs.length() && !wait; i++){
      if(procs[i]!=0){ wait = procs[i]->state()!=QProcess::NotRunning; }
    }
    if(wait){ qDebug() << "Waiting on processes...."; QApplication::processEvents(QEventLoop::AllEvents, 100); QThread::msleep(500); }
  efinishedMessage();
  }
}


// ------------ //
//    IMPORT
// ------------ //

void MainWindow::on_importButton_clicked(){
    importFile = QFileDialog::getOpenFileName(this, tr("Open Ex ported Archive"), QDir::homePath());
    //create new tar file in location of choice
    QMessageBox warningMessageBox;
    warningMessageBox.setText(tr("Importing entire directories has the potental to break your system. Are you sure you want to continue?"));
    warningMessageBox.setStandardButtons(QMessageBox::Yes);
    warningMessageBox.addButton(QMessageBox::No);
    warningMessageBox.setWindowTitle(tr("Import Warning"));
    warningMessageBox.show();
    if(warningMessageBox.exec() == QMessageBox::Cancel){ return; }
    else{ doImport();}
}

void MainWindow::doImport(){
  QList< ExternalProcess* > procs;
  //export to tmp dir
  QDir().mkdir("/tmp/importdir");
  homeImport = "tar -xpf " + importFile + " /tmp/importdir/";
  ExternalProcess::launch(homeImport);

  // Now add selections
  // files
  if(ui->checkBox_fstab->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/fstab /etc/fstab"); }
  if(ui->checkBox_group->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/group /etc/group"); }
  if(ui->checkBox_hostname->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/hostname /etc/hostname"); }
  if(ui->checkBox_hostsdeniedssh->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/hosts.deniedssh /etc/hosts.deniedssh"); }
  if(ui->checkBox_hostid->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/hostid /etc/hostid"); }
  if(ui->checkBox_inetdconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/inetd.conf /etc/inetd.conf"); }
  if(ui->checkBox_loginconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/login.conf /etc/login.conf"); }
  if(ui->checkBox_masterpassword->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/master.password /etc/master.password"); }
  if(ui->checkBox_pcdmconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/pdcm.conf /usr/local/etc/pcdm.conf"); }
  if(ui->checkBox_passwd->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/passwd /etc/passwd"); }
  if(ui->checkBox_resolvconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/resolv.conf /etc/resolv.conf"); }
  if(ui->checkBox_shells->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/shells /etc/shells"); }
  if(ui->checkBox_rcconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/rc.conf /etc/rc.conf"); }
  if(ui->checkBox_loaderconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/loader.conf /etc/loader.conf"); }
  if(ui->checkBox_ipfwcustom->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/ipfw.custom /etc/ipfw.custom"); }
  if(ui->checkBox_sysctlconf->isChecked()){  procs << ExternalProcess::launch("cp -a /tmp/importdir/sysctl.conf /etc/sysctl.conf"); }
  if(ui->checkBox_syslogconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/syslog.conf /etc/syslog.conf"); }
  if(ui->checkBox_wpasupplicantconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/wpa_supplicant.conf /etc/wpa_supplicant.conf"); }
  if(ui->checkBox_xorgconf->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/xorg.conf /etc/X11/xorg.conf"); }
  // directories
  if(ui->checkBox_cups_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xpf /tmp/importdir/cups.tgz /usr/local/etc/cups/" ); }
  if(ui->checkBox_confd_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xpf /tmp/importdir/confd.tgz /etc/conf.d/"); }
  if(ui->checkBox_etc_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xp  /tmp/importdir/etcdir.tgz /etc/" ); }
  if(ui->checkBox_etcssh_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xpf /tmp/importdir/etcssh.tgz /etc/ssh/" ); }
  if(ui->checkBox_usrlocaletc_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xpf /tmp/importdir/usrlocaletc.tgz /usr/local/etc/" ); }
  if(ui->checkBox_openvpn_dir->isChecked()){
    procs << ExternalProcess::launch("tar -xpf /tmp/importdir/openvpn.tgz /usr/local/etc/openvpn/" ); }
  if(ui->checkBox_lumina_dir->isChecked()){
    luminaPathString = QDir::homePath() + ".config/lumina-desktop/";
    luminaImport = "tar -xpf /tmp/lumina.tgz " + luminaPathString;
    }
  // packagelist
  if(ui->checkBox_installedpkglist->isChecked()){ procs << ExternalProcess::launch("cp -a /tmp/importdir/install-pkg-list " + QDir::homePath() ); }
  // in future add option to install previous packages
  pkglistMessageBox.setText(tr("Your previously installed package list can be found in your home directory"));
  pkglistMessageBox.setStandardButtons(QMessageBox::Yes);
  pkglistMessageBox.show();


  // wait until archiver closes

  bool wait = true;
  while(wait){
    wait = false;
    for(int i=0; i<procs.length() && !wait; i++){
      if(procs[i]!=0){ wait = procs[i]->state()!=QProcess::NotRunning; }
    }
    if(wait){ qDebug() << "Waiting on processes...."; QApplication::processEvents(QEventLoop::AllEvents, 100); QThread::msleep(500); }
  }

  if(ui->checkBox_homedir->isChecked()){ exportHomeDir(); }
  else { ifinishedMessage(); }
}

void MainWindow::importHomeDir(){
  QList< ExternalProcess* > procs;
  importPath = QDir::homePath();
  QString importhomefile = QFileDialog::getOpenFileName(this, tr("Open Exported Archive"), QDir::homePath());
  // to do :  Check size of home dir; set size in message; refuse to export if location does not have enough space;
  homeImport = "tar -xpf " + importFile + " " + importPath;
  procs << ExternalProcess::launch(homeImport) ;

  // wait for external processes to finish

  bool wait = true;
  while(wait){
    wait = false;
    for(int i=0; i<procs.length() && !wait; i++){
      if(procs[i]!=0){ wait = procs[i]->state()!=QProcess::NotRunning; }
    }
    if(wait){ qDebug() << "Waiting on processes...."; QApplication::processEvents(QEventLoop::AllEvents, 100); QThread::msleep(500); }
  }
    ifinishedMessage();
}

void MainWindow::ifinishedMessage(){
  ifinishedMessageBox.setText(tr("The Import of selected files/dirs has finished."));
  ifinishedMessageBox.setStandardButtons(QMessageBox::Yes);
  ifinishedMessageBox.show();
}
