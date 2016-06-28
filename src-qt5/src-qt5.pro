TEMPLATE = subdirs
CONFIG += recursive

SUBDIRS+= pc-firstbootgui \
	 pc-installgui \
	 pc-welcome \
	 pc-xgui

pc-firstbootgui.depends = libtrueos
