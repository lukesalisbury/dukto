# Add more folders to ship with the application, here
#qml_folder.source = qml/dukto
#qml_folder.target = qml
#DEPLOYMENTFOLDERS = qml_folder

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

QT += network qml widgets

VERSION = 6.1.0

mac {
	ICON = dukto.icns
}


win32 {
	RC_FILE = dukto.rc
	LIBS += libWs2_32 libole32 libNetapi32
}

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
#symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
symbian:TARGET.CAPABILITY += NetworkServices


# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += \
    src/buddylistitemmodel.cpp \
    src/destinationbuddy.cpp \
    src/duktoprotocol.cpp \
    src/guibehind.cpp \
    src/ipaddressitemmodel.cpp \
    src/main.cpp \
    src/miniwebserver.cpp \
    src/platform.cpp \
    src/recentlistitemmodel.cpp \
    src/settings.cpp \
    src/theme.cpp \
    src/updateschecker.cpp

HEADERS += \
    src/buddylistitemmodel.h \
    src/destinationbuddy.h \
    src/duktoprotocol.h \
    src/guibehind.h \
    src/ipaddressitemmodel.h \
    src/miniwebserver.h \
    src/peer.h \
    src/platform.h \
    src/recentlistitemmodel.h \
    src/settings.h \
    src/theme.h \
    src/updateschecker.h

RESOURCES += \
    qml.qrc

OTHER_FILES +=
