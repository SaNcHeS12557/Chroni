#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsBlurEffect>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QString>
#include <QDataStream>
#include <QStyle>
#include <QStyleFactory>
#include <QMessageBox>
#include <QActionGroup>
#include <QList>
#include <QSettings>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "mainwindow.h"
#include "chroniwindow.h"
#include <fileapi.h>

using namespace std;
namespace fs = filesystem;

QString MainWindow::chroniPath = "";
QString MainWindow::chroniExePath = "";

HICON get_wnd_hicon(HWND hwnd)
{
    // get the window icon
    HICON icon = reinterpret_cast<HICON>(::SendMessageW(hwnd, WM_GETICON, ICON_BIG, 0));
    if (icon == 0) {
        icon = reinterpret_cast<HICON>(::SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0));
    }
    if (icon == 0) {
        icon = reinterpret_cast<HICON>(::GetClassLongPtrW(hwnd, GCLP_HICON));
    }
    // get the first icon from the main module (executable image of the process)
    if (icon == 0) {
        icon = ::LoadIconW(GetModuleHandleW(0), MAKEINTRESOURCEW(0));
    }
    // filter out those with default icons
    if (icon == ::LoadIconW(0, IDI_APPLICATION)) {
        icon = 0;
    }

    return icon;
}

static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
    WindowInfo window;
    // lParam to the vector pointer
    vector<WindowInfo>* runningWindows = reinterpret_cast<vector<WindowInfo>*>(lparam);
    // app name
    int length = GetWindowTextLengthA(hWnd);
    char* buffer = new char[length + 1];
    GetWindowTextA(hWnd, buffer, length + 1);
    QString windowTitle = QString::fromLocal8Bit(buffer);
    const QString ELLIPSIS = "...";
    if (windowTitle.length() > 30) {
        windowTitle = windowTitle.left(30 - ELLIPSIS.length()) + ELLIPSIS; // '...' if name is too long
    }
    LPDWORD pID = new DWORD;
    GetWindowThreadProcessId(hWnd, pID);

    // app info
    if (IsWindowVisible(hWnd) && length != 0) {
        HICON icon = get_wnd_hicon(hWnd);
        if (icon != 0) {
            window.appName = windowTitle;
            window.appIcon = icon;
            window.hwnd = hWnd;
            window.pID = pID;
            // push to vector
            runningWindows->push_back(window);
        }
    }

    delete[] buffer;

    return TRUE;
}

void createNewAppList(Ui::MainWindow *ui) {
    vector<WindowInfo> activeWindows;

    EnumWindows((WNDENUMPROC)enumWindowCallback, (LPARAM)&activeWindows);

    for(const WindowInfo& ProcessInfo : activeWindows){
        QListWidgetItem * item = new QListWidgetItem(ui->appListWidget);
        // icon
        QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(ProcessInfo.appIcon));
        item->setIcon(pixmap);
        // name
        item->setText(ProcessInfo.appName);
    }
}

void MainWindow::refreshApplicationsList() {
    ui->appListWidget->clear(); // clearing list
    createNewAppList(ui);
}

void MainWindow::updateAutostart(bool status) {
    if(status) {
        // add to reg
        QSettings ASsettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        ASsettings.setValue(chroniName, chroniExePath);
    } else {
        QSettings ASsettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        ASsettings.remove(chroniName);
    }
}

void MainWindow::autostartEnable() {
    isAutostartEnabled = !isAutostartEnabled;
    qDebug() << "Autostart Status: "<< isAutostartEnabled;
    updateAutostart(isAutostartEnabled);
}

vector<WindowInfo> getSelectedApps(vector<WindowInfo> activeWindows, Ui::MainWindow *ui) {
    vector<WindowInfo> selectedApps;

    QList<QListWidgetItem *> selectedItems = QList<QListWidgetItem *> (ui->appListWidget->selectedItems());

    for (auto it : selectedItems){
        auto foundWindow = std::find_if(activeWindows.begin(), activeWindows.end(), [it](const WindowInfo& obj){
            return obj.appName == it->text();
        });

        if(foundWindow != activeWindows.end()){
            selectedApps.push_back(*foundWindow);
        }
    }

    return selectedApps;
}

void setupListWidget(Ui::MainWindow *ui) {
    QListWidget *appListWidget = ui->appListWidget;

    appListWidget->setSelectionMode(QAbstractItemView::MultiSelection);    // multiselect
    appListWidget->setFlow(QListView::Flow::TopToBottom);   // horizontal list
    QScrollArea *scrollArea = ui->scrollArea;   // scroll area
    scrollArea->setWidget(appListWidget);
    scrollArea->horizontalScrollBar(); // horizontal scroll bar

    MainWindow::connect(ui->appListWidget->selectionModel(),&QItemSelectionModel::selectionChanged,[=]() {  // selection lambda limit
        if(ui->appListWidget->selectionModel()->selectedIndexes().size() > 3)
        {
            QList<QModelIndex> lst = ui->appListWidget->selectionModel()->selectedIndexes();
            ui->appListWidget->selectionModel()->select(lst.first(),QItemSelectionModel::Deselect);
        }
    });

    appListWidget->setIconSize(QSize(30, 30)); // icon size
}

void MainWindow::setupGroups(Ui::MainWindow *ui) {
    themeGroup = new QActionGroup(ui->menuTheme);
    themeGroup->addAction(ui->actionDark);
    themeGroup->addAction(ui->actionLight);
    themeGroup->addAction(ui->actionCustom);
    themeGroup->setExclusive(true);

    languageGroup = new QActionGroup(ui->menuTheme);
    languageGroup->addAction(ui->actionEnglish);
    languageGroup->addAction(ui->actionRussian);
    languageGroup->addAction(ui->actionUkrainian);
    languageGroup->setExclusive(true);
}

void MainWindow::setupFolders(){
    chroniExePath = getchroniExePathStatic();
    chroniName = "Chroni";

    QFileInfo fileInfo(chroniExePath);
    chroniPath = fileInfo.path();

    //create config directory
    fs::create_directory(chroniPath.toStdString() + "/config");
}

int MainWindow::createConfigFile(QString path){
    QJsonObject jsonConfig;

    jsonConfig["languageId"] = languageChoosed;
    jsonConfig["themeId"];
    jsonConfig["isAutostartEnabled"];
    jsonConfig["isFirstLaunch"];

    QJsonDocument json(jsonConfig);

    return 1;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setFixedSize(645, 330);
    setupGroups(ui);

    if(isAutostartEnabled) {
        ui->autostartEnable->setCheckState(Qt::CheckState::Checked);
    } else {
        ui->autostartEnable->setCheckState(Qt::CheckState::Unchecked);
    }

    connect(ui->autostartEnable, &QCheckBox::stateChanged, this, &MainWindow::autostartEnable);

    const auto & styles = QStyleFactory::keys(); // get all styles
    for(const auto & s : styles)
    {
        qDebug() << s;
    }

    setupListWidget(ui);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_nextButton_clicked()
{
    // get selected apps
    vector<WindowInfo> activeWindows;
    EnumWindows((WNDENUMPROC)enumWindowCallback, (LPARAM)&activeWindows);
    vector<WindowInfo> selectedApps = getSelectedApps(activeWindows, ui);
    // todo save selected apps
    if (!selectedApps.size()) {
        QMessageBox::warning(
            this,
            tr("ChroniLauncher"),
            tr("You have to select something from the list to continue!"));
        refreshApplicationsList();
        return;
    }
    setupFolders();

    ChroniWindow *chroniWindow = new ChroniWindow();

    chroniWindow->show();

    chroniWindow->ChroniWindow::setSelectedApps(selectedApps); // pass selected apps to next window to a list widget

    auto monitoredApps = chroniWindow->getMonitoredApps();
    qDebug() << monitoredApps.size();
    auto appsPath = chroniWindow->getAppsPath();
    qDebug() << appsPath;

    chroniWindow->setupApps(monitoredApps, appsPath);

    this->close();
}


void MainWindow::on_refreshButton_clicked()
{
    refreshApplicationsList();
}


void MainWindow::on_actionDark_changed()
{

}


void MainWindow::on_actionLight_changed()
{
    themeChoosed = 2;
}


void MainWindow::on_actionCustom_changed()
{
    themeChoosed = 3;
}

short MainWindow::getThemeChoosed() const
{
    return themeChoosed;
}

void MainWindow::setThemeChoosed(short newThemeChoosed)
{
    themeChoosed = newThemeChoosed;
}

QActionGroup *MainWindow::getLanguageGroup() const
{
    return languageGroup;
}

void MainWindow::setLanguageGroup(QActionGroup *newLanguageGroup)
{
    languageGroup = newLanguageGroup;
}

QActionGroup *MainWindow::getThemeGroup() const
{
    return themeGroup;
}

void MainWindow::setThemeGroup(QActionGroup *newThemeGroup)
{
    themeGroup = newThemeGroup;
}

bool MainWindow::getIsFirstLaunch() const
{
    return isFirstLaunch;
}

void MainWindow::setIsFirstLaunch(bool newIsFirstLaunch)
{
    isFirstLaunch = newIsFirstLaunch;
}

QString MainWindow::getChroniPath() const
{
    return chroniPath;
}

void MainWindow::setChroniPath(const QString &newChroniPath)
{
    chroniPath = newChroniPath;
}

QString MainWindow::getChroniPathStatic()
{
    QString chroniPath;
    QString chroniExePath = getchroniExePathStatic();

    QFileInfo fileInfo(chroniExePath);
    chroniPath = fileInfo.path();

    return chroniPath;
}


QString MainWindow::getchroniExePathStatic(){
    char pBuf[256];
    int bytes = GetModuleFileNameA(NULL, pBuf, sizeof(pBuf));

    if (bytes == 0){
        return "";
    }

    return QString(pBuf);
}
// get-setters:
void MainWindow::setchroniExePath(const QString &newchroniExePath)
{
    chroniExePath = newchroniExePath;
}

QString MainWindow::getchroniExePath() const
{
    return chroniExePath;
}

bool MainWindow::getIsAutostartEnabled() const
{
    return isAutostartEnabled;
}

void MainWindow::setIsAutostartEnabled(bool newIsAutostartEnabled)
{
    isAutostartEnabled = newIsAutostartEnabled;
}

short MainWindow::getLanguageChoosed() const
{
    return languageChoosed;
}

void MainWindow::setLanguageChoosed(short newLanguageChoosed)
{
    languageChoosed = newLanguageChoosed;
}

QString MainWindow::getChroniName() const
{
    return chroniName;
}

void MainWindow::setChroniName(const QString &newChroniName)
{
    chroniName = newChroniName;
}


void MainWindow::on_actionDark_toggled(bool arg1)
{
    // if()
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(isFirstLaunch){
        createConfigFile(chroniPath);
    }

    event->accept();
}

