#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void refreshApplicationsList();
    void autostartEnable();
    void nextButtonClicked();
    QString getchroniExePath();
    void updateAutostart(bool status);
    void setupGroups(Ui::MainWindow *ui);
    void setupFolders();
    int createConfigFile(QString path);

    QString getChroniName() const;
    void setChroniName(const QString &newChroniName);

    QString getchroniExePath() const;
    void setchroniExePath(const QString &newchroniExePath);
    static QString getchroniExePathStatic();

    bool getIsAutostartEnabled() const;
    void setIsAutostartEnabled(bool newIsAutostartEnabled);

    short getLanguageChoosed() const;
    void setLanguageChoosed(short newLanguageChoosed);

    QString getChroniPath() const;
    void setChroniPath(const QString &newChroniPath);
    static QString getChroniPathStatic();

    bool getIsFirstLaunch() const;
    void setIsFirstLaunch(bool newIsFirstLaunch);

    QActionGroup *getLanguageGroup() const;
    void setLanguageGroup(QActionGroup *newLanguageGroup);

    QActionGroup *getThemeGroup() const;
    void setThemeGroup(QActionGroup *newThemeGroup);

    short getThemeChoosed() const;
    void setThemeChoosed(short newThemeChoosed);

    void closeEvent(QCloseEvent *event);

private slots:
    void on_nextButton_clicked();
    void on_refreshButton_clicked();
    void on_actionDark_changed();
    void on_actionLight_changed();
    void on_actionCustom_changed();

    void on_actionDark_toggled(bool arg1);

private:
    Ui::MainWindow *ui;
    QString chroniName;
    static QString chroniExePath;
    static QString chroniPath;
    QActionGroup *languageGroup;
    QActionGroup *themeGroup;

    // config vars:
    bool isAutostartEnabled = false;
    bool isFirstLaunch = true;
    short languageChoosed = 1;
    short themeChoosed = 1;
};
#endif // MAINWINDOW_H
