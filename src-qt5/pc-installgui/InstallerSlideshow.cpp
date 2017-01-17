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
	  text = tr("");
	  image = "";
	  break;
	case 1:
	  text = tr("");
	  image = "";
	  break;
	case 2:
	  text = tr("");
	  image = "";
	  break;
	case 3:
	  text = tr("");
	  image = "";
	  break;
  }
  //Now apply the item to the label
  textlabel->setText(text);
  if(iconlabel == 0){ textlabel->setStyleSheet("background-image: "+image); }
  else{ iconlabel->setPixmap( QPixmap(image) ); }
}
