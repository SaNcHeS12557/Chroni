QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# charts
QT += charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chroniwindow.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    chroniwindow.h \
    mainwindow.h

FORMS += \
    chroniwindow.ui \
    mainwindow.ui

TRANSLATIONS += \
    Chroni_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    build/Desktop_Qt_6_7_0_MinGW_64_bit-Debug/debug/apps/Spotify.exe.json \
    build/Desktop_Qt_6_7_0_MinGW_64_bit-Debug/debug/apps/msedge.exe.json \
    build/Desktop_Qt_6_7_0_MinGW_64_bit-Debug/debug/apps/mspaint.exe.json \
    build/Desktop_Qt_6_7_0_MinGW_64_bit-Debug/debug/apps/qtcreator.exe.json \
    imgs/chroniLogo.png

RESOURCES += \
    Images.qrc
