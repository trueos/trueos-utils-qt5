//==========================
//  Simple file to setup the slideshow shown on the installer
//
//==========================
#include "installer.h"
#include <QStringList>

//NOTES ABOUT ADDING IMAGES
// 1) Add file to images directory
// 2) Add line in sysinstaller.qrc file for that image
// 3) The "image" variable will look like ":/modules/images/<name of file>"

int Installer::numSlides(){
  return 1; //make sure this returns the total number of items below (1+ largest "num")
}

void Installer::loadSlide(QLabel *textlabel, QLabel *iconlabel, int num){
  QString text, image;
  switch(num){
    case 0:
      text = tr("Join the discussion on the TrueOS discourse channel!(discourse.trueos.org)");
      image = ":/modules/images/discourse.png";
      break;
    case 1:
      text = tr("Personalize your TrueOS experience with the Lumina Desktop Environment, installed with TrueOS.");
      image = ":/modules/images/TrOSLum.png";
      break;
    case 2:
      text = tr("TrueOS uses SysAdm for system management, accessed through the cloud icon in the system tray.");
      image = ":/modules/images/sysadm.png";
      break;
    case 3:
      text = tr("Need more apps? The SysAdm Appcafe helps you find and manage all your needed applications.");
      image = ":/modules/images/appcafeslide.png";
      break;
    case 4:
      text = tr("The TrueOS Disk Manager is used to manage attached disks, ZFS pools, and ZFS datasets.");
      image = ":/modules/images/diskmanager.png";
      break;
    case 5:
      text = tr("TrueOS manages printing with the Common Unix Printing System (CUPS).");
      image = ":/modules/images/cups.png";
      break;
    case 6:
      text = tr("You can use the SysAdm Life Preserver utility to manage ZFS snapshots and scheduling, giving you the power to back up and replicate your important data.");
      image = ":/modules/images/lifepreserverslide.png";
      break;
    case 7:
      text = tr("The TrueOS Handbook is available both locally and online at www.trueos.org/handbook/trueos.html");
      image = ":/modules/images/handbook.png";
      break;
    case 8:
      text = tr("Enjoy being on the bleeding edge? You can set the SysAdm Update Manager to pull from the UNSTABLE repository to follow along with the active development.");
      image = ":/modules/images/updatesettings.png";
      break;
    case 9:
      text = tr("TrueOS is very secure, with a firewall, secure instruction sets, PersonaCrypt, and Tor Mode all built directly into the system.");
      image = ":/modules/images/percrypt.png";
      break;
  }
  //Now apply the item to the label
  textlabel->setText(text);
  if(iconlabel == 0){ textlabel->setStyleSheet("background-image: "+image); }
  else{ iconlabel->setPixmap( QPixmap(image) ); }
}
