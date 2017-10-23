#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <ExternalProcess.h>
//#include <LUtils.h>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow){
    ui->setupUi(this);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_exportButton_clicked(){
  //create new tar file in location of choice
  exportFile = QFileDialog::getSaveFileName(this, tr("Where do you want to export?"), QDir::homePath());
  if(ui->checkBox_rcconf->isChecked()){ files = files + "/etc/rc.conf "; }
  if(ui->checkBox_loaderconf->isChecked()){ files = files + "/boot/loader.conf "; }
  if(ui->checkBox_ipfwcustom->isChecked()){ files = files + "/etc/ipfw.custom "; }
  if(ui->checkBox_sysctlconf->isChecked()){ files = files + "/etc/sysctl.conf "; }
  if(ui->checkBox_syslogconf->isChecked()){ files = files + "/etc/syslog.conf "; }
  if(ui->checkBox_wpasupplicantconf->isChecked()){ files = files + "/etc/wpa_supplicant.conf "; }
  if(ui->checkBox_cupsconfs->isChecked()){
    ExternalProcess::launch("lumina-archiver --aa /tmp/cups.tgz /usr/local/etc/cups/*" );
    files = files + "/tmp/cups.tgz ";
  }
  if(ui->checkBox_confd->isChecked()){
    ExternalProcess::launch("lumina-archiver --aa /tmp/confd.tgz /etc/conf.d/*") ;
    files = files + "/tmp/confd.tgz ";
  }
  if(ui->checkBox_installedpkglist->isChecked()){ files = files + "/usr/local/log/pc-updatemanager/install-pkg-list "; }
  qDebug() << "files" << files;
  launchString = "lumina-archiver --aa " + exportFile + " " + files;
  ExternalProcess::launch(launchString);
  qDebug() << "exportFile" << exportFile;
  // wait until archiver closes
  //finishedMessage();
  if(ui->checkBox_homedir->isChecked()){ exportHomeDir(); }
}

void MainWindow::finishedMessage(){
  QMessageBox *finishedMessageBox = new QMessageBox;
  finishedMessageBox->setText(tr("The Export has finished."));
  QPushButton *pushButtonOk = finishedMessageBox->addButton(tr("Ok"), QMessageBox::YesRole);
  finishedMessageBox->QDialog::setWindowTitle(tr("Finished"));
  finishedMessageBox->show();
}

void MainWindow::exportHomeDir(){
  homePathString = QDir::homePath() + "*";
  homeDirArchive = QFileDialog::getSaveFileName(this, tr("Where do you want to export your home directory?"), QDir::homePath());
  homeExport = "lumina-archiver --aa " + homeDirArchive + " " + homePathString;
  ExternalProcess::launch(homeExport) ;
  qDebug() << "homeExport" << homeExport;
  // wait until archiver closes
  // finishedMessage();
}

