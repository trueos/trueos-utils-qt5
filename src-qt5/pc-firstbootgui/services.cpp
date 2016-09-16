#include "services.h"

#include <QFile>
#include <QObject>
#include <QTextStream>

QList<service> Services::getServiceList(){
  //Define all the pre-set services here
  // NOTE: This assumes that all of these services are disabled by default!!
  QList<service> out;

  //SSHD
  if(QFile::exists("/etc/rc.d/sshd")){
    service S;
    S.ID = "SSHD";
    S.file = "/etc/rc.d/sshd"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Enable SSH");
    S.description = QObject::tr("Allows SSH access to this system from remote systems");
    S.openPorts << "tcp 22";
    S.rcLines << "sshd_enable=YES";
    //S.cmds << ""; //optional extra commands
    out << S;
  }
  //DISABLE IPV6
  if(QFile::exists("/etc/rc.conf")){
    service S;
    S.ID = "DISABLE-IPV6";
    S.file = "/etc/rc.conf"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Disable IPv6 (Requires Reboot)");
    S.description = QObject::tr("Disable the use of IPv6 connections");
    //S.openPorts
    S.rcRemove << QRegExp("*ipv6*", Qt::CaseInsensitive,  QRegExp::Wildcard);
    S.rcLines << "ipv6_activate_all_interfaces=NO";
    //S.cmds << "service netif restart"; //optional extra commands
    out << S;
  }
  //Enable polling for Intel HDA
  if(QFile::exists("/etc/rc.conf")){
    service S;
    S.ID = "ENABLE-HDA-POLLING";
    S.file = "/etc/sysctl.conf"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Enable Intel HDA polling");
    S.description = QObject::tr("Fixes sound for some devices but may drain battery life as a result");
    //S.openPorts
    //S.rcRemove << QRegExp("*ipv6*", Qt::CaseInsensitive,  QRegExp::Wildcard);
    S.rcLines << "dev.hdac.0.polling=1";
    S.cmds << "sysctl dev.hdac.0.polling=1"; //enable without requiring reboot
    out << S;
  }
  //Enable support for Realtek Wireless cards
  if(QFile::exists("/etc/rc.conf")){
    service S;
    S.ID = "ENABLE-RTWN";
    S.file = "/etc/rc.conf"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Enable Realtek Wireless");
    S.description = QObject::tr("Starts service at boot to load kernel module for realtek wireless cards");
    //S.openPorts
    //S.rcRemove << QRegExp("*ipv6*", Qt::CaseInsensitive,  QRegExp::Wildcard);
    S.rcLines << "if_rtwn_load=YES";
    S.cmds << "kldload if_rtwn" << "service netif restart"; //enable without requiring reboot
    out << S;
  }
  return out;
}

void Services::enableService(service S){
  //First open any firewall ports
  if(!S.openPorts.isEmpty()){
    QStringList lines;
    lines << "# "+S.ID;
    lines.append(S.openPorts);
    system("echo \""+lines.join("\n").toLocal8Bit()+"\n\" >> /etc/ipfw.openports");
    system("/bin/sh /etc/ipfw.rules"); //reload the firewall rules
  }
  //Remove any bad rc.conf lines as needed
  if(!S.rcRemove.isEmpty()){
    //Open rc.conf and read all the contents
    QFile file("/etc/rc.conf");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    QStringList rcconf = in.readAll().split("\n");
    file.close();
    //Look for matches of the regexp
    bool changed = true;
    for(int i=0; i<rcconf.length(); i++){
      bool remove = false;
      for(int j=0; j<S.rcRemove.length(); j++){
        if( S.rcRemove[j].indexIn(rcconf[i]) >=0){ remove = true; break; }
      }
      if(remove){ rcconf.removeAt(i); i--; changed = true;}
    }
    //Now save the file again
    if(changed){
      file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
      QTextStream out(&file);
      out << rcconf.join("\n");
      file.close();
    }
  }

  //Now make any changes to /etc/rc.conf (for auto-start on next boot)
  if(!S.rcLines.isEmpty()){
    for(int i=0; i<S.rcLines.length(); i++){
      system("sysrc -f /etc/rc.conf "+S.rcLines[i].toLocal8Bit());
    }
  }
  //Now make any other CLI changes that are listed
  if(!S.cmds.isEmpty()){
    for(int i=0; i<S.cmds.length(); i++){
      system(S.cmds[i].toLocal8Bit());
    }
  }
  //Now startup this service for the current boot session
  system(S.file.toLocal8Bit()+" start");
}
