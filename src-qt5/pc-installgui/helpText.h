
#ifndef HELPTEXT_H
#define HELPTEXT_H
#include <QObject>

#define HELPTEXT0 QObject::tr("Choose your desired installation language. Elements not fully translated display in English. You can also load a saved TrueOS configuration from a USB drive. The drive must be formatted with MSDOSFS or FAT32.")
#define HELPTEXT1 QObject::tr("Choose what kind of TrueOS installation to install to your system:\n\n 'TrueOS Desktop' is the full graphical experience for the home or light office user. The Lumina Desktop Environment is installed with TrueOS, providing a full graphical experience.\n 'TrueOS Server' installs a FreeBSD based command-line server operating system. There is no graphical desktop operating system with this option. The SysAdm client and API is included to assist with managing the server either locally or remotely.\n 'Restore from Life-Preserver backup' is used to restore a TrueOS system from ZFS snapshots that have been replicated to a remote backup server. The network interface must be configured for this option to work properly. The installer attempts to automatically configure networking, but you can click the 'blue orb' icon on the bottom of this screen to manually configure the network interface.")
#define HELPTEXT2 QObject::tr("Select optional packages to be installed along with TrueOS.")
#define HELPTEXT3 QObject::tr("This screen displays the currently selected disk options. Click 'Customize Disk Settings' to open a wizard to adjust the installation partition or disk, alter the partition scheme, use a custom ZFS pool name, or force a ZFS 4k block size. Click 'Save Config to USB' to save your TrueOS installation configuration to a MSDOSFS or FAT32 formatted USB drive.")
#define HELPTEXT4 QObject::tr("Your system is now installing! Go grab a cup of coffee and relax, the installer will notify you when it is finished.")
#define HELPTEXT5 QObject::tr("The system is now finished installing! Click Finish to return to the text menu where you can reboot the system, and be sure to eject any install media before booting into your new system.")
#define HELPTEXT6 QObject::tr("")
#define HELPTEXT7 QObject::tr("")
#define HELPTEXT8 QObject::tr("")
#define HELPTEXT9 QObject::tr("")

#endif // HELPTEXT_H
