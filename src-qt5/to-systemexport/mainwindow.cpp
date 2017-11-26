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
  if(ui->echeckBox_rcconf->isChecked()){ files  << "/etc/rc.conf"; }
  if(ui->echeckBox_loaderconf->isChecked()){ files  << "/boot/loader.conf"; }
  if(ui->echeckBox_ipfwcustom->isChecked()){ files  << "/etc/ipfw.custom"; }
  if(ui->echeckBox_sysctlconf->isChecked()){ files  << "/etc/sysctl.conf"; }
  if(ui->echeckBox_syslogconf->isChecked()){ files  << "/etc/syslog.conf"; }
  if(ui->echeckBox_wpasupplicantconf->isChecked()){ files  << "/etc/wpa_supplicant.conf"; }
  if(ui->echeckBox_xorgconf->isChecked()){ files  << "/etc/X11/xorg.conf"; }
  if(ui->echeckBox_installedpkglist->isChecked()){ files << "/usr/local/log/pc-updatemanager/install-pkg-list"; }
  if(ui->echeckBox_cupsconfs->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/cups.tgz /usr/local/etc/cups/*");
    files << "/tmp/cups.tgz";
  }
  if(ui->echeckBox_confd->isChecked()){
    procs << ExternalProcess::launch("lumina-archiver --aa /tmp/confd.tgz /etc/conf.d/*") ;
    files << "/tmp/confd.tgz";
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

  // Now add the files chosen
  if(ui->echeckBox_rcconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/rc.conf /etc/rc.conf"); }
  if(ui->echeckBox_loaderconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/loader.conf /etc/loader.conf"); }
  if(ui->echeckBox_ipfwcustom->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/ipfw.custom /etc/ipfw.custom"); }
  if(ui->echeckBox_sysctlconf->isChecked()){  ExternalProcess::launch("cp /tmp/importdir/sysctl.conf /etc/sysctl.conf"); }
  if(ui->echeckBox_syslogconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/syslog.conf /etc/syslog.conf"); }
  if(ui->echeckBox_wpasupplicantconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/wpa_supplicant.conf /etc/wpa_supplicant.conf"); }
  if(ui->echeckBox_xorgconf->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/xorg.conf /etc/X11/xorg.conf"); }
  if(ui->echeckBox_cupsconfs->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/cups.tgz /usr/local/etc/cups/" ); }
  if(ui->echeckBox_confd->isChecked()){
    ExternalProcess::launch("lumina-archiver --sx /tmp/importdir/confd.tgz /etc/conf.d/"); }
  if(ui->echeckBox_installedpkglist->isChecked()){ ExternalProcess::launch("cp /tmp/importdir/pc-updatemanager/install-pkg-list " + QDir::homePath() ); }
  // in future add option to install previous packages
  QMessageBox *pkglistMessageBox = new QMessageBox;
  pkglistMessageBox->setText(tr("Your previously installed package list can be found in your home directory"));
  QPushButton *pushButtonOk = pkglistMessageBox->addButton(tr("Ok"), QMessageBox::YesRole);
  pkglistMessageBox->QDialog::setWindowTitle(tr("Package List"));
  pkglistMessageBox->show();
  //qDebug() << "exportFile" << exportFile;
  // wait until archiver closes
  //ifinishedMessage();
  if(ui->echeckBox_homedir->isChecked()){ importHomeDir(); }
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
