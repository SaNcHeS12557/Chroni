#include "chroniwindow.h"
#include "mainwindow.h"
#include "ui_chroniwindow.h"

#include <QString>
#include <QThread>

#include <windows.h>
#include <vector>
#include <TlHelp32.h>
#include <Psapi.h>
#include <tchar.h>
#include <winuser.h>
#include <cstring>
#include <QTimer>
#include <QObject>
#include <fileapi.h>
#include <filesystem>
#include <QDir>
#include <QDate>

// json
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>

// charts
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

using namespace std;
namespace fs = filesystem;

QString ChroniWindow::appsPath = "";

QTimer *timer;
const double TIME_INCREMENT = 1.0 / 3600.0; // Increment time by 1 second (in hours)

// void ChroniWindow::displayGraph() {
//     QLineSeries *series = new QLineSeries();
//     appListWidgetItem &item = getItemByAppName(ui->appListWidget->currentItem()->text());

//     for (int i = 0; i < item.timeVectorArray.size(); i++) {
//         series->append(i, item.timeVectorArray[i]);
//     }

//     QChart *chart = new QChart();
//     chart->addSeries(series);
//     chart->setTitle("Time spent on " + item.appName);

//     QValueAxis *axisX = new QValueAxis();
//     axisX->setTitleText("Days");
//     axisX->setLabelFormat("%i");
//     axisX->setTickCount(item.timeVectorArray.size());
//     chart->addAxis(axisX, Qt::AlignBottom);
//     series->attachAxis(axisX);

//     QValueAxis *axisY = new QValueAxis();
//     axisY->setTitleText("Time (h)");
//     axisY->setLabelFormat("%i");
//     chart->addAxis(axisY, Qt::AlignLeft);
//     series->attachAxis(axisY);

//     chart->legend()->hide();
//     chart->createDefaultAxes();
//     chart->setAnimationOptions(QChart::SeriesAnimations);

//     // background
//     QPalette pal = chart->palette();
//     pal.setColor(QPalette::Window, QRgb(0x333333));
//     // chart->setPalette(pal);
//     chart->setBackgroundBrush(QBrush(QColor(0x333333)));
//     chart->setPlotAreaBackgroundBrush(QBrush(QColor(0x444444)));
//     // text color
//     chart->setTitleBrush(QBrush(QColor(0xffffff)));
//     chart->legend()->setBrush(QBrush(QColor(0xffffff)));

//     // params
//     QChartView *chartView = new QChartView(chart);
//     chartView->setRenderHint(QPainter::Antialiasing);

//     // scene
//     QGraphicsScene *scene = new QGraphicsScene(this);
//     scene->addWidget(chartView);
//     ui->appGraph->setScene(scene);
//     ui->appGraph->setRenderHint(QPainter::Antialiasing);
//     ui->appGraph->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
//     ui->appGraph->setDragMode(QGraphicsView::ScrollHandDrag);

//     // size
//     chartView->resize(ui->appGraph->size());

// }

HANDLE ChroniWindow::getHProcByName(QString appName){
    string strAppName = appName.toStdString();
    if (appName.isNull() || appName.isEmpty()) {
        return nullptr;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(Process32First(snapshot, &entry) == true) {
        while (Process32Next(snapshot, &entry) == true) {
            wstring ws(entry.szExeFile);
            string szExeFile(ws.begin(), ws.end());

            if (strAppName.compare(szExeFile) == 0) {
                HANDLE hProc = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, entry.th32ProcessID ); // SHOULD BE CLOSED IN THE FUTURE!
                return hProc;
            }
        }
    }

    return nullptr;
}

bool ChroniWindow::checkIfProcessIsActive(HANDLE hProc, appListWidgetItem& item) {
    DWORD exitCode;
    GetExitCodeProcess(hProc, &exitCode);

    if (exitCode == STILL_ACTIVE) {
        return true;
    }

    if (item.hProc != nullptr) {
        CloseHandle(hProc);
        item.hProc = nullptr;
    }

    return false;
}

std::vector<appListWidgetItem> ChroniWindow::getMonitoredApps() {
    return monitoredApps;
}

appListWidgetItem &ChroniWindow::getItemByAppName(QString appName) {
    for (appListWidgetItem& wi : monitoredApps) {
        if (wi.appName == appName) {
            return wi;
        }
    }

    return monitoredApps[0]; // TO DO - return empty item (using classes instead of structs)
}

void ChroniWindow::updateTotalTimeVector(appListWidgetItem& item){
    // TO DO if equation depended on DATE
    double s = 0;
    for(auto& time : item.todaySessionsArray) {
        s += time;
    }

    item.timeVectorArray.push_back(s);
}

double sumTodaySessions(appListWidgetItem& item){
    double s = 0;
    for(auto& time : item.todaySessionsArray) {
        s += time;
    }

    return s;
}

void ChroniWindow::updateCurrentTime(appListWidgetItem& item){
    item.todaySessionsArray.push_back(item.currentTime);
    item.currentTime = 0.0;

    saveAppTime();
}

void ChroniWindow::updateStatusLabel(QString appName) {
    QLabel *label = ui->appOpenedStatus;
    QLabel *icoLabel = ui->appStatusIcon;
    QPixmap pixmap;
    QString imgPath;
    appListWidgetItem& item = getItemByAppName(appName); // get item by app name

    // if found
    if (item.appName == appName) {
        if (item.isActive) {
            // (this->windowState() == Qt::WindowMinimized) ? void() : label->setText("App is active"); // process is active
            if (!(this->windowState() == Qt::WindowMinimized)) {
                label->setText("App is active");
                imgPath = ":/images/imgs/activeApp.png";
            }
        } else {
            // (this->windowState() == Qt::WindowMinimized) ? void() : label->setText("App is not active"); // process isnt active
            if (!(this->windowState() == Qt::WindowMinimized)) {
                label->setText("App is not active");
                imgPath = ":/images/imgs/notActiveApp.png";
            }
            (item.currentTime) ? updateCurrentTime(item) : void();
        }
    } else {
        // (this->windowState() == Qt::WindowMinimized) ? void() : label->setText("App not found"); // process isnt active
        if (!(this->windowState() == Qt::WindowMinimized)) {
            label->setText("App not found");
            imgPath = ":/images/imgs/notFoundApp.png";
        }
    }

    icoLabel->setPixmap(QPixmap(imgPath));
}

void ChroniWindow::updateAppActivity() {
    for (appListWidgetItem& item : monitoredApps) {
        HANDLE hProc = item.hProc;
        if (hProc != nullptr) {
            item.isActive = checkIfProcessIsActive(hProc, item);
            if (item.isActive) {
                item.currentTime += TIME_INCREMENT;
                item.totalTime += TIME_INCREMENT;
            }
            if (ui->appListWidget->currentItem()->text() == item.appName) { // if item is selected - update label
                updateStatusLabel(item.appName);
                updateAppTime(*ui->tTime, *ui->csTime, ui->appListWidget->currentItem());
            }
        } else {
            item.hProc = getHProcByName(item.appName);
        }
    }

    // Save the updated times to the JSON file
    saveAppTime();
}

void timeConverter(double input, int& hours, int& minutes, int& seconds) {
    hours = static_cast<int>(input);
    double remainingHours = input - hours;

    double totalMinutes = remainingHours * 60;
    minutes = static_cast<int>(totalMinutes);
    double remainingMinutes = totalMinutes - minutes;

    seconds = static_cast<int>(remainingMinutes * 60);
}

void ChroniWindow::updateAppTime(QLabel& totalTime, QLabel& currentTime, QListWidgetItem *item) {
    appListWidgetItem appItem = getItemByAppName(item->text());
    double total = appItem.totalTime;
    double current = appItem.currentTime;

    int hours = 0;
    int minutes = 0;
    int seconds = 0;

    timeConverter(total, hours, minutes, seconds);
    if (minutes < 60) {
        totalTime.setText(QString::number(minutes) + "m " + QString::number(seconds) + "s");
    } else {
        totalTime.setText(QString::number(hours) + "h " + QString::number(minutes) + "m");

    }

    timeConverter(current, hours, minutes, seconds);
    if (minutes < 60) {
        currentTime.setText(QString::number(minutes) + "m " + QString::number(seconds) + "s");
    } else {
        currentTime.setText(QString::number(hours) + "h " + QString::number(minutes) + "m");
    }

    // currentSessionTime label visibility
    ui->currentSessionTime->setVisible(appItem.isActive);
    currentTime.setVisible(appItem.isActive);

    qDebug() << "Total: " << total << " Current: " << current;
}

void setupAppListWidget(Ui::ChroniWindow *ui) {
    ui->appListWidget->setIconSize(QSize(30, 30)); // icon size
}

bool dirExists(QString &dirPath) {
    QDir dir(dirPath);
    return dir.exists();
}

void ChroniWindow::setupFolders() {
    appsPath = chroniPath + "/apps";
    if (!dirExists(appsPath)) {
        fs::create_directory(appsPath.toStdString());
    }
}

QString ChroniWindow::getAppsPath() {
    return ChroniWindow::appsPath;
}

QJsonObject createJsonObject(QString appName) {
    QJsonObject jsonObject;
    QJsonArray jsonArray;

    jsonObject["appName"] = appName;
    jsonObject["totalTime"] = 0;
    jsonObject["currentTime"] = 0;
    jsonObject["todaySessions"] = jsonArray; // 1 subarray => new session
    jsonObject["timeVector"] = jsonArray; // stores every day hours
    jsonObject["lastUpdate"] = "01.01.1010";

    return jsonObject;
}

void ChroniWindow::setupApps(vector<appListWidgetItem>& monitoredApps, QString& appsPath) {
    for (appListWidgetItem &app : monitoredApps) {
        QString filePath = appsPath + "/" + app.appName + ".json";
        QFile file(filePath);
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Chroni can't read the file:" << filePath;
                continue;
            }
            QByteArray data = file.readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
            QJsonObject jsonObject = jsonDoc.object();

            app.totalTime = jsonObject["totalTime"].toDouble();
            app.currentTime = 0;
            app.lastUpdate = jsonObject["lastUpdate"].toString().toStdString();

            QJsonArray jsonArray;
            // get timeVector from json and store them in the app
            jsonArray = jsonObject["timeVector"].toArray();
            for (auto value : jsonArray) {
                app.timeVectorArray.push_back(value.toDouble());
            }

            // get today sessions from json and store them in the app
            jsonArray = jsonObject["todaySessions"].toArray();
            for (auto value : jsonArray) {
                app.todaySessionsArray.push_back(value.toDouble());
            }

            file.close();
        } else {
            QJsonObject jsonObject = createJsonObject(app.appName);
            QJsonDocument jsonDoc(jsonObject);

            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) { // Ensure the file is truncated
                qWarning() << "Chroni can't write the file:" << filePath;
                continue;
            }
            file.write(jsonDoc.toJson());
            file.close();
        }
    }
}

void ChroniWindow::saveAppTime() {
    for (appListWidgetItem& app : monitoredApps) {
        QString filePath = appsPath + "/" + app.appName + ".json";
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) { // Ensure the file is truncated
            qWarning() << "Chroni can't open the file for writing:" << filePath;
            continue;
        }

        QJsonObject jsonObject;
        jsonObject["appName"] = app.appName;
        jsonObject["totalTime"] = app.totalTime;
        jsonObject["currentTime"] = app.currentTime;

        QJsonArray todaySessions;
        for(auto session : app.todaySessionsArray) {
            todaySessions.append(session);
        }
        jsonObject["todaySessions"] = todaySessions;

        QJsonArray timeVector;
        for(auto time : app.timeVectorArray) {
            timeVector.append(time);
        }
        jsonObject["timeVector"] = timeVector;

        if(QString::fromStdString(app.lastUpdate) != QDate::currentDate().toString("dd.MM.yyyy")) {
            if (!app.todaySessionsArray.empty()) {
                double sessionsSum = sumTodaySessions(app);
                app.timeVectorArray.push_back(sessionsSum);
            }

            app.todaySessionsArray.clear();
            app.lastUpdate = (QDate::currentDate().toString("dd.MM.yyyy")).toStdString();
        }

        // date update
        QDate date = QDate::currentDate();
        QString formatedDate = date.toString("dd.MM.yyyy");
        jsonObject["lastUpdate"] = formatedDate;
        app.lastUpdate = formatedDate.toStdString();

        QJsonDocument jsonDoc(jsonObject);
        file.write(jsonDoc.toJson());
        file.close();
    }
}

ChroniWindow::ChroniWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChroniWindow)
    , monitoredApps()
{
    ui->setupUi(this);
    setupAppListWidget(ui);

    ChroniWindow::chroniPath = MainWindow::getChroniPathStatic();

    setupFolders();

    // timer creation
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
    timer->start(1000); // every second

    connect(ui->actionReturn_to_the_Launcher, &QAction::triggered, this, &ChroniWindow::returnToChroniLauncher);

    // minimize window
    // this->setWindowState(Qt::WindowMinimized);
}

void ChroniWindow::onTimerTimeout() {
    updateAppActivity();
}

QString ChroniWindow::getChroniPath() const {
    return chroniPath;
}

void ChroniWindow::setChroniPath(const QString &newChroniPath) {
    chroniPath = newChroniPath;
}

ChroniWindow::~ChroniWindow() {
    delete ui;
}

void ChroniWindow::pushSelectedApp(appListWidgetItem selectedItem) {
    monitoredApps.push_back(selectedItem);
}

string getExeName(string path) {
    size_t slashPos = path.find_last_of("\\");

    if (slashPos != string::npos) {
        return path.substr(slashPos + 1);
    }

    return path; // return full path if no slash is found
}

void ChroniWindow::setSelectedApps(std::vector<WindowInfo> selectedApps) {
    Ui::ChroniWindow *ui = this->ui;

    for(WindowInfo& ProcessInfo : selectedApps) {
        bool toBeAdded = true;
        wchar_t lpwstr[MAX_PATH];

        DWORD size = MAX_PATH;
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, *ProcessInfo.pID);

        if (hProc == NULL) {
            DWORD error = GetLastError();
            qDebug() << "Failed to open process. Error code: " << error;
        }

        if (hProc != NULL) { // if open process is unseccessful
            GetProcessImageFileNameW(hProc, lpwstr, size);
            CloseHandle(hProc);

            wstring ws(lpwstr);
            string appName = getExeName(string(ws.begin(), ws.end())); // converting to QStr and getting .exe name

            for(int i = 0; i < ui->appListWidget->count(); i++) { // check if app is already in the list
                QListWidgetItem *enumItem = ui->appListWidget->item(i);
                if (enumItem->text() == QString::fromStdString(appName)) {
                    toBeAdded = false;
                    break;
                }
            }

            if(toBeAdded) {
                QListWidgetItem * item = new QListWidgetItem(ui->appListWidget);
                appListWidgetItem si = {QString::fromStdString(appName), ProcessInfo.appIcon, nullptr, false};

                QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(si.appIcon));
                item->setIcon(pixmap); // icon
                item->setText(si.appName); // name

                pushSelectedApp(si);
            }
        }
    }

    setupApps(monitoredApps, appsPath); // Ensure apps are initialized correctly
    startNewHandles(monitoredApps);

}

void ChroniWindow::startNewHandles(std::vector<appListWidgetItem> monitoredApps) {
    for (appListWidgetItem& app : monitoredApps) {
        app.hProc = getHProcByName(app.appName);
    }
}

void ChroniWindow::on_appListWidget_currentRowChanged(int currentRow) {
    QListWidgetItem *item = ui->appListWidget->item(currentRow);
    item->setSelected(true);

    updateStatusLabel(item->text());
    updateAppTime(*ui->tTime, *ui->csTime, item);

    // displayGraph();
}

// QMenu click action
void ChroniWindow::returnToChroniLauncher() {
    MainWindow *mainWindow = new MainWindow();
    mainWindow->show();
    mainWindow->refreshApplicationsList();
    this->close();
}

// close all handles
void ChroniWindow::closeEvent(QCloseEvent *event) {
    for (appListWidgetItem& app : monitoredApps) {
        if (app.hProc != nullptr) {
            CloseHandle(app.hProc);
        }
    }
    timer->stop();
    delete timer;

    saveAppTime();

    // TODO fix memory usage on every close and open



    event->accept();
    qDebug() << "Closing ChroniWindow";
}
