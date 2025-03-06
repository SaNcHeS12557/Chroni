#ifndef CHRONIWINDOW_H
#define CHRONIWINDOW_H

#include <QMainWindow>
#include <vector>
#include <windows.h>
#include <QLabel>
#include <QListWidgetItem>

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

namespace Ui {
class ChroniWindow;
}

struct WindowInfo {
    QString appName;
    HWND hwnd;
    HICON appIcon;
    LPDWORD pID;
    // bool isActive = false;
};

// struct appListWidgetItem {
//     QString appName;
//     HICON appIcon;

//     HANDLE hProc = nullptr;
//     bool isActive = false;
// };

struct appListWidgetItem {
    QString appName;
    HICON appIcon;
    HANDLE hProc = nullptr;
    bool isActive = false;
    double totalTime = 0.0;
    double currentTime = 0.0;
    std::vector<double> todaySessionsArray;
    std::vector<double> timeVectorArray;
    std::string lastUpdate = "01.01.1010";

};


class ChroniWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChroniWindow(QWidget *parent = nullptr);
    ~ChroniWindow();

    HANDLE getHProcByName(QString appName);
    bool checkIfProcessIsActive(HANDLE hProc, appListWidgetItem& item);
    void setSelectedApps(std::vector<WindowInfo> selectedApps);
    void updateStatusLabel(QString appName);
    // void updateAppStatus(QString appName);

    std::vector<appListWidgetItem> getMonitoredApps();
    void pushSelectedApp(appListWidgetItem selectedItem);
    appListWidgetItem& getItemByAppName(QString appName);
    void updateAppActivity();
    void updateAppTime(QLabel& totalTime, QLabel& currentTime, QListWidgetItem *item);
    void saveAppTime();
    void startNewHandles(std::vector<appListWidgetItem> monitoredApps);

    QString getChroniPath() const;
    void setChroniPath(const QString &newChroniPath);
    void setChroniPath();

    void setupFolders();
    static QString getAppsPath();

    void setupApps(std::vector<appListWidgetItem>& monitoredApps, QString& appsPath);

    void updateCurrentTime(appListWidgetItem& item);
    void updateTotalTimeVector(appListWidgetItem& item);

    void displayGraph();
    void closeEvent(QCloseEvent *event);

private slots:
    void on_appListWidget_currentRowChanged(int currentRow);
    void onTimerTimeout();
    void returnToChroniLauncher();
private:
    Ui::ChroniWindow *ui;
    std::vector<appListWidgetItem> monitoredApps;
    QString chroniPath;
    static QString appsPath;
};

#endif // CHRONIWINDOW_H
