#include "mainwindow.h"
#include "chroniwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("windows11");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Chroni_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    if(w.getIsFirstLaunch()){
        w.show();
        w.refreshApplicationsList();
    } else {
        ChroniWindow cw;
        cw.show();
    }

    return a.exec();
}
