/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include "xgui.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QProcess>

#define PREFIX QString("/usr/local")

const QString PROGDIR( PREFIX + "/share/trueos/xorg-gui" );
const QString XDRIVERDIR( PREFIX + "/lib/xorg/modules/drivers" );
#define TMPDIR QString("/tmp/.xgui")


void XGUI::ProgramInit()
{
    //gridLayout_4->setMargin(0);
    //gridLayout_4->setSpacing(0);

    // Load the screen resolutions
    loadResolutions();

    // Load the drivers
    loadDrivers();

    // Load the Depth
    loadDepth();

    // Load the Device Information
    loadDeviceInformation();

    // Disable the monitor advanced group box
    //monitorChangedSlot();

    // Check for an unprobed monitor
    //checkUnprobedMonitor();

    // Connect our Dual-head checkbox / slot
    //connect(checkDualHead, SIGNAL( clicked() ), this, SLOT( dualChangedSlot() ) );
    //connect(checkMonitorSync, SIGNAL( clicked() ), this, SLOT( monitorChangedSlot() ) );
    connect(pushApply, SIGNAL( clicked() ), this, SLOT( applySlot() ) );
    connect(pushClose, SIGNAL( clicked() ), this, SLOT( closeSlot() ) );

    // Check out dualChangedSlot to ensure we disable the box by default
    //dualChangedSlot();
  tabWidget->setTabEnabled(1,false);
}


void XGUI::loadDeviceInformation()
{
  //Load all the pci information first
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
  //qDebug() << "Found pciconf_info:" << pciconf_info;

  //Load the information about the graphics card
  QStringList devs = pciconf_info.keys().filter("vgapci");
  for(int i=0; i<devs.length(); i++){
    devs[i] = pciconf_info.value(devs[i]).toObject().value("vendor").toString()+" "+pciconf_info.value(devs[i]).toObject().value("device").toString();
  }
  labelVideoCard->setText(devs.join("/n"));

    QString tmp;
    QString scriptdir = PROGDIR + "/scripts";

    // Get the card name and display it
    //tmp = getLineFromCommandOutput(scriptdir + "/cardboard.sh");
    //tmp.truncate(30);
    //labelVideoCard->setText(tmp);

    // Get the Monitor Name and display it
    tmp = getLineFromCommandOutput(scriptdir + "/monmodel.sh");
    if ( tmp == "Monitor Model" )
    {
       labelMonitor->setText(tr("Unknown Monitor"));
    } else {
       tmp.truncate(50);
       labelMonitor->setText(tmp);
    }


    // Get the detected driver, and make suggestion based upon it
    tmp = getLineFromCommandOutput(scriptdir + "/carddriver.sh");
    // Figure out the suggested driver for this card
    suggestDriver(tmp);

}

// Function which takes a supplied driver name, and makes suggestion for user
void XGUI::suggestDriver(QString driver)
{
   QString sugDriver;
   QString devinfo = labelVideoCard->text();
  //Get the current boot method (BIOS/EFI)
  QProcess proc;
    proc.start("sysctl -n machdep.bootmethod");
    proc.waitForFinished();
  bool efiboot = ( QString(proc.readAllStandardOutput()).simplified() != "BIOS" );

   //Check the device information for key words/phrases
  QStringList preferred;
  if(devinfo.contains("nvidia",Qt::CaseInsensitive)){
    //NVIDIA GPU
    preferred << "nvidia" << "nv" << "nouveau"; //binary NVIDIA, Open source, older open source

  }else if(devinfo.contains("intel", Qt::CaseInsensitive)){
    if(efiboot){ preferred << "modesetting" << "intel"; } //modesetting works best with EFI bootup
    else{ preferred << "intel" << "modesetting"; }

  }else if(devinfo.contains("amd", Qt::CaseInsensitive)){
    if(efiboot){ preferred << "amdgpu"; }
    preferred << "radeon";

  }else if(devinfo.contains("ati", Qt::CaseInsensitive)){
    if(efiboot){ preferred << "amdgpu"; }
    preferred << "radeon" << "ati"; //ati driver is *very* old - radeon seems to have much better support (even for older cards)

  }else if(devinfo.contains("virtualbox", Qt::CaseInsensitive)){
    preferred << "vboxvideo";

  }else if(devinfo.contains("vmware", Qt::CaseInsensitive)){
    preferred << "vmware";
  }
  //Now go through the preferred drivers and find the first one which is currently available
  for(int i=0; i<preferred.length() && sugDriver.isEmpty(); i++){
    if(comboDriver->findText(preferred[i])>=0){ sugDriver = preferred[i]; }
  }

  if(sugDriver.isEmpty()){
    //Last-ditch attempt - VESA or SCFB depending on boot method
    sugDriver = efiboot ? "scfb" : "vesa";

  }else{
     // Check if we are using a driver that has a "-3d-enable" version
     if (comboDriver->findText(sugDriver + "-3d-enable") >= 0 ) {
       sugDriver = sugDriver+"-3d-enable";
     }
  }

  if(!sugDriver.isEmpty()){
    labelRecDriver->setText(tr("Suggested Driver:") + " <b>" + sugDriver + "</b>" );
    int index = comboDriver->findText(sugDriver);
    if(index>=0){ comboDriver->setCurrentIndex(index); }
  }
}

// Function which checks if we are using an unprobed monitor, warns the user
// and then enables the advanced tab
void XGUI::checkUnprobedMonitor()
{

  QFile test("/tmp/.unprobedMonitor");
  if (test.exists())
  {
     // Enable the advanced monitor horz / vert refresh rates
     checkMonitorSync->setChecked(true);
     monitorChangedSlot();

     QFile test2("/tmp/.warnedUnprobedMonitor");
     if ( ! test2.exists() )
     {
       system("touch /tmp/.warnedUnprobedMonitor");
       QMessageBox::warning(0, tr("Warning!"), tr("PC-BSD was unable to autodetect your monitor. You may need to specify this monitor's Horizontal and Vertical refresh rates in the 'Advanced' tab."));      
     }
     
  }

}

void XGUI::loadResolutions()
{
  //layout_resolution->setVisible(false); //disable for now - remove it later
    /*QString line;
    int i=0;
    
    comboResolution->clear();

        QFile file( PROGDIR + "/settings/resolutions.txt" );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );
             while ( !stream.atEnd() ) {
               line = stream.readLine(); // line of text excluding '\n'
               comboResolution->addItem(line);	    
               comboDualRes->addItem(line);	    
               if (line == "1024x768" )
               {
			comboResolution->setCurrentIndex(i);
			comboDualRes->setCurrentIndex(i);
               }
          i++;
	}
	file.close();
    }*/
}


void XGUI::loadDrivers()
{
    //int i = 0;
    //QString line;
    
    comboDriver->clear();
    //List off all the drivers currently available on the system
    QDir dir(XDRIVERDIR);
    QStringList drivers = dir.entryList(QStringList() << "*_drv.so", QDir::Files, QDir::Name);
    for(int i=0; i<drivers.length(); i++){
      QString name = drivers[i].section("_drv.so",0,0); //name of the driver
      //Perform additional verification checks (EFI/BIOS boot, etc..) 
      // TODO
      comboDriver->addItem(name);
      if(name=="vesa"){ comboDriver->setCurrentIndex(comboDriver->count()-1); }
    }
    // Now populate the combo box
    /*QFile file( PROGDIR + "/settings/drivers.txt" );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );
             while ( !stream.atEnd() ) {
               line = stream.readLine(); // line of text excluding '\n'
	  	if ( isDriverAvailable(line) ) {
                  comboDriver->addItem(line);	    
	          if ( line == "vesa" )
	   	    comboDriver->setCurrentIndex(i);
		}
	       i++;
	}
	file.close();
    }*/
}


void XGUI::loadDepth()
{
  //layout_depth->setVisible(true); //disable for now
    /*QString line;
    int i = 0;
    
    comboDepth->clear();

        QFile file( PROGDIR + "/settings/depth.txt" );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QTextStream stream( &file );
             while ( !stream.atEnd() ) {
               line = stream.readLine(); // line of text excluding '\n'
               comboDepth->addItem(line);	    
	       if ( line == "24bit" ) {
		  comboDepth->setCurrentIndex(i);
	       }
	 i++;

	}
	file.close();
    }*/
}



void XGUI::closeSlot()
{
	exit(1);
}


void XGUI::applySlot()
{
	QString tmp;

 	// Make sure the TMPDIR exists
	QDir ctmp(TMPDIR);
	if ( ! ctmp.exists() )
		ctmp.mkdir(TMPDIR);

	QFile file( TMPDIR + "/.xsettings.sh" );
    	if ( file.open( QIODevice::WriteOnly ) ) {
        QTextStream stream( &file );
            stream << "#!/bin/sh\n";
            //stream << "RES=\"" + comboResolution->currentText() + "\"; export RES\n";
            stream << "export DRIVER=\"" + comboDriver->currentText() + "\"\n";

	     	//tmp = comboDepth->currentText();
		//tmp.truncate(tmp.indexOf("bit"));
	    //stream << "DEPTH=\"" + tmp + "\"; export DEPTH\n";
   	    /*if ( checkMonitorSync->isChecked() )
    	    {
  	    	stream << "HORIZSYNC=\"" +lineHorz->text() + "\"; export HORIZSYNC\n" ;
	    	stream << "VERTREFRESH=\"" +lineVertRefresh->text() + "\"; export VERTREFRESH\n" ;
	    } else {
		stream << "HORIZSYNC=\"OFF\"; export HORIZSYNC\n" ;
	    	stream << "VERTREFRESH=\"OFF\"; export VERTREFRESH\n" ;
	    }
            if ( checkDualHead->isChecked() )
            {
                stream << "DUALHEAD=\"YES\" ; export DUALHEAD\n";
                stream << "DUALRES=\"" + comboDualRes->currentText() + "\"; export DUALRES\n";
            }*/
	    file.close();
    	}


	QMessageBox::information( this, tr("Display Settings"),
    tr("The settings you have specified will now be tested.\nIf your screen does not appear, please wait and you will be returned to this screen.") );

	// Message box, letting user know that we will test this resolution
	exit(0);
}


void XGUI::monitorChangedSlot()
{

    if ( checkMonitorSync->isChecked() )
    {
	groupMonitor->setEnabled(true);
    } else {
	groupMonitor->setEnabled(false);
    }
    
}

void XGUI::dualChangedSlot()
{
    if ( checkDualHead->isChecked() )
    {
	groupDualHead->setEnabled(true);
    } else {
	groupDualHead->setEnabled(false);
    }
}

QString XGUI::getLineFromCommandOutput( QString command )
{
        FILE *file = popen(command.toLatin1(),"r");

        char buffer[100];

        QString line = "";
        char firstChar;

        if ((firstChar = fgetc(file)) != -1){
                line += firstChar;
                line += fgets(buffer,100,file);
        }

        
        pclose(file);

        return line.simplified();
}

// Script to check if a particular xorg driver is available on the system
bool XGUI::isDriverAvailable(QString driver)
{
	// Make sure to strip the -3d-enable flag off
	if ( driver.indexOf("-3d-enable") != -1 )
		driver.truncate(driver.indexOf("-3d-enable"));

	return QFile::exists(XDRIVERDIR + "/" + driver + "_drv.so");
}
