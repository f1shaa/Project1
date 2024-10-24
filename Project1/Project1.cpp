#include "Project1.h"
#include <QTimer>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <windows.h>
#include <tlhelp32.h>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QCursor>
#include <QProcess>
#include <QInputDialog>

//путь к файлу со списком приложений
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
//путь к файлу со списком автозапуска приложений
const QString csvFilePath2 = QDir::currentPath() + "/process_list_2.csv";

//контекстное меню
QMenu* contextMenu;

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    //запуск приложения
    ui.setupUi(this);

    //загрузка таблицы со списком пользователя из файла при запуске
    loadTable(csvFilePath);

    //загрузка таблицы с автозапуском из файла при запуске
    loadAutoStartTable(csvFilePath2);

    //подключение действия файл -> открыть...
    connect(ui.actionOpen, &QAction::triggered, this, &Project1::on_actionOpen);

    //подключение действия таблица -> очистить...
    connect(ui.actionClear, &QAction::triggered, this, &Project1::on_actionClear);

    //подключение кнопки завершить процесс
    connect(ui.buttonClose, &QPushButton::clicked, this, &Project1::on_buttonClose);

    //настройки таймера
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Project1::checkProcesses);
    timer->start(1000); //интервал в 1с

    //настройка контекстного меню для таблицы с процессами
    contextMenu = new QMenu(this);
    //действия
    QAction* deleteAction = new QAction("Delete", this);
    connect(deleteAction, &QAction::triggered, this, &Project1::on_actionDelete);
    QAction* editAction = new QAction("Add auto start", this);
    connect(editAction, &QAction::triggered, this, &Project1::on_actionEdit);
    //добавления действия в меню
    contextMenu->addAction(deleteAction);
    contextMenu->addAction(editAction);
    //политика меню
    ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu);

    //настройка контекстного меню для таблицы автозапуска
    autoStartContextMenu = new QMenu(this);
    QAction* deleteAutoStartAction = new QAction("Delete", this);
    connect(deleteAutoStartAction, &QAction::triggered, this, &Project1::on_actionDeleteAutoStart);
    QAction* setTimeAction = new QAction("Set Time", this);
    connect(setTimeAction, &QAction::triggered, this, &Project1::on_actionSetTime);
    autoStartContextMenu->addAction(deleteAutoStartAction);
    autoStartContextMenu->addAction(setTimeAction);
    //политика меню
    ui.autoStartTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.autoStartTableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu2);
}

Project1::~Project1()
{}

//обработчик нажатия на кнопку файл -> открыть
void Project1::on_actionOpen() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Executable"), "", tr("Executable Files (*.exe);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName); //получение информации о файле
        QString filePath = fileInfo.absoluteFilePath(); //путь к файлу
        QString fileNameInfo = fileInfo.fileName(); //имя файла

        //проверка на дублирование процееса
        for (int i = 0; i < ui.tableWidget->rowCount(); i++) {
            QTableWidgetItem* item = ui.tableWidget->item(i, 0);
            if (item && item->text() == fileNameInfo) {
                QMessageBox::information(this, "Attention", "The process is already on the list!!!");
                return;
            }
        }

        //добавление информации о процессе в список
        ProcessInfo processInfo = { fileNameInfo, filePath, false };
        processList.append(processInfo);

        int rowCount = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(rowCount);

        //заполнение столбцов
        ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(fileNameInfo)); //имя файла в 1 столбец
        ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(filePath)); //путь во 2 столбец
        ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //статус по умолочанию

        //сохранения таблицы при каждом добавлении нового процесса
        saveTable(csvFilePath);
    }
}

//обработчик нажатия на кнопку таблица -> очистить
void Project1::on_actionClear() {
    //определение активной таблицы
    if (ui.tableWidget->hasFocus()) {
        //очистка таблицы процессов
        ui.tableWidget->clearContents();
        ui.tableWidget->setRowCount(0);
        processList.clear();

        //очистка CSV файла для таблицы
        QFile file(csvFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.resize(0); //очистка файла
            file.close();
        }
    }
    else if (ui.autoStartTableWidget->hasFocus()) {
        //очистка таблицы автозапуска
        ui.autoStartTableWidget->clearContents();
        ui.autoStartTableWidget->setRowCount(0);
        autoStartProcesses.clear();

        //очистка CSV файла для автозапуска
        QFile file(csvFilePath2);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.resize(0); //очистка файла
            file.close();
        }
    }
}

//обработчик нажатия на кнопку ПКМ -> удалить, для таблицы с процессами
void Project1::on_actionDelete() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        ui.tableWidget->removeRow(indexRow); //удалить из таблицы
        processList.removeAt(indexRow); //удалить из списка
        saveTable(csvFilePath); //обновить файл
    }
}

//обработчик нажатия на кнопку ПКМ -> изменить параметры запуска, для таблицы с процессами
void Project1::on_actionEdit() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        //получение данных о процессе
        QString processName = ui.tableWidget->item(indexRow, 0)->text();
        QString processPath = ui.tableWidget->item(indexRow, 1)->text();

        //проверка на дублирование процесса в автозапуске
        for (int i = 0; i < ui.autoStartTableWidget->rowCount(); i++) {
            if (ui.autoStartTableWidget->item(i, 0)->text() == processName) {
                return;
            }
        }

        //добавление процесса в таблицу автозапуска
        int rowCount = ui.autoStartTableWidget->rowCount();
        ui.autoStartTableWidget->insertRow(rowCount);
        ui.autoStartTableWidget->setItem(rowCount, 0, new QTableWidgetItem(processName)); //имя
        ui.autoStartTableWidget->setItem(rowCount, 1, new QTableWidgetItem(processPath)); //путь
        ui.autoStartTableWidget->setItem(rowCount, 2, new QTableWidgetItem("0")); //время по умолчанию

        autoStartProcesses.append({ processName, processPath, 0 }); //сохранение в список автозапуска
        //сохранение в таблицу автозапуска
        saveAutoStartTable(csvFilePath2);
    }
}

//обработчик нажатия на кнопку заврешить процесс, для таблицы с процессами
void Project1::on_buttonClose() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        //получение состояния процесса
        QString processStatus = ui.tableWidget->item(indexRow, 2)->text();

        //проверка состояния процесса
        if (processStatus == "Active") {
            //получение имени процесса
            QString processName = ui.tableWidget->item(indexRow, 0)->text();

            //поиск среди всех процессов
            HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            bool processFound = false;

            //перебор всех процессов
            if (Process32First(hProcessesSnapshot, &pe32)) {
                do {
                    QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
                    
                    if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
                        //получение ID процесса
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProcess != NULL) {
                            //завершение процесса
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                            processFound = true;
                            break;
                        }
                    }
                } while (Process32Next(hProcessesSnapshot, &pe32));
            }
            CloseHandle(hProcessesSnapshot);
            if (processFound) {
                // Обновление статуса в таблице на "Inactive"
                ui.tableWidget->item(indexRow, 2)->setText("Inactive");
            }
        }
    }
}

//обработчик нажатия на кнопку запуск, для таблицы с процессами
void Project1::on_buttonStart() {
    //перебор процессов в таблице
    for (int i = 0; i < ui.autoStartTableWidget->rowCount(); ++i) {
        //получение состояния процесса
        QString processName = ui.autoStartTableWidget->item(i, 0)->text(); //имя
        QString processPath = ui.autoStartTableWidget->item(i, 1)->text(); //путь
        int delay = autoStartProcesses[i].delay; //задержка

        //проверка состояния процесса
        bool isActive = false;
        for (const ProcessInfo& processInfo : processList) {
            if (processInfo.name == processName && processInfo.isActive) {
                isActive = true;
                break;
            }
        }

        //запуск, если неактивен
        if (!isActive) {
            if (delay > 0) {
                //запуск с задержкой
                QTimer::singleShot(delay * 1000, [=]() {
                    QProcess::startDetached(processPath);
                    });
            }
            else {
                //немедленный запуск, если задержки нет
                QProcess::startDetached(processPath);
            }
        }
    }
}

//обработчик нажатия на кнопку ПКМ -> удалить, для таблицы автозапуска
void Project1::on_actionDeleteAutoStart() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        ui.autoStartTableWidget->removeRow(indexRow); //удалить
        saveAutoStartTable(csvFilePath2); //обновить файл автозапуска
    }
};

//обработчик нажатия на кнопку ПКМ -> установить время, для таблицы с автозапуском
void Project1::on_actionSetTime() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        const int minDelay = 1;    //min задержка в секундах
        const int maxDelay = 3600; //max задержка в секундах
        bool ok;
        int delay = QInputDialog::getInt(this, tr("Set Time"),
            tr("Enter delay in seconds:"),
            ui.autoStartTableWidget->item(indexRow, 2)->text().toInt(),
            minDelay, maxDelay, 1, &ok);

        if (ok) {
            //обновление времени в таблице и в структуре
            ui.autoStartTableWidget->item(indexRow, 2)->setText(QString::number(delay));
            autoStartProcesses[indexRow].delay = delay;

            //сохранение в CSV файл
            saveAutoStartTable(csvFilePath2);
        }
    }
}

//метод отображения контекстного меню для таблцы с процессами
void Project1::showContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = ui.tableWidget->itemAt(pos);
    if (item) {
        contextMenu->exec(QCursor::pos());
    }
    else {
        qDebug() << "No item at position" << pos;
    }
}

//метод отображения контекстного меню для таблицы автозапуска
void Project1::showContextMenu2(const QPoint& pos) {
    QTableWidgetItem* item = ui.autoStartTableWidget->itemAt(pos);
    if (item) {
        autoStartContextMenu->exec(QCursor::pos());
    }
    else {
        qDebug() << "No item at position" << pos;
    }
}

//метод для сохранения списка пользователя в CSV файл
void Project1::saveTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        for (int i = 0; i < ui.tableWidget->rowCount(); ++i) {
            QString name = ui.tableWidget->item(i, 0)->text(); //имя файла
            QString path = ui.tableWidget->item(i, 1)->text(); //путь к файлу
            stream << name << "," << path << "\n"; //сохранение в формате CSV
        }
        file.close();
    }
}

//метод для загрузки списка пользователя из CSV файла
void Project1::loadTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;

        //очистка таблицы перед загрузкой данных
        ui.tableWidget->setRowCount(0);
        processList.clear();

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 2) continue; //пропуск некорректных строк

            //добавление процесса в список processList
            ProcessInfo processInfo = { rowData[0], rowData[1], false };
            processList.append(processInfo);

            int rowCount = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(rowCount);

            ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя файла
            ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь к файлу
            ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //статус по умолочанию
        }
        file.close();
    }
}

//метод для сохранения списка автозапуска в CSV файл
void Project1::saveAutoStartTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        for (int i = 0; i < ui.autoStartTableWidget->rowCount(); ++i) {
            QString name = ui.autoStartTableWidget->item(i, 0)->text(); //имя
            QString path = ui.autoStartTableWidget->item(i, 1)->text(); //путь
            QString delay = ui.autoStartTableWidget->item(i, 2)->text(); //время
            stream << name << "," << path << "," << delay << "\n"; //сохранить
        }
        file.close();
    }
}

//метод для загрузки списка автозапуска в CSV файл
void Project1::loadAutoStartTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;

        //очистка таблицы перед загрузкой данных
        ui.autoStartTableWidget->setRowCount(0);
        autoStartProcesses.clear();

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 3) continue; //пропуск некорректных строк

            //добавление процесса в таблицу автозапуска
            int rowCount = ui.autoStartTableWidget->rowCount();
            ui.autoStartTableWidget->insertRow(rowCount);
            ui.autoStartTableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя
            ui.autoStartTableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь
            ui.autoStartTableWidget->setItem(rowCount, 2, new QTableWidgetItem(rowData[2])); //время
        
            //добавление процесса в список автозапуска
            AutoStartProcess autoStartProcess = { rowData[0], rowData[1], rowData[2].toInt() };
            autoStartProcesses.append(autoStartProcess);
        }
        file.close();
    }
    on_buttonStart(); //автозапуск программ из таблицы
}

//метод определения активности процессов
void Project1::checkProcesses() {
    //снимок всех процессов в системе
    HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe32; //инф-я о процессе
    pe32.dwSize = sizeof(PROCESSENTRY32); //размер структуры

    //установка статуса по умолочанию для всех процессов
    for (int i = 0; i < processList.size(); ++i) {
        processList[i].isActive = false;
    }

    //перебор всех процессов
    if (Process32First(hProcessesSnapshot, &pe32)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);

            for (int i = 0; i < processList.size(); ++i) {
                if (processList[i].name == currentProcessName) {
                    processList[i].isActive = true; //процесс найден -> активен
                }
            }
        } while (Process32Next(hProcessesSnapshot, &pe32));
    }
    CloseHandle(hProcessesSnapshot);

    //обновление статуса в таблице
    for (int i = 0; i < processList.size(); ++i) {
        for (int j = 0; j < ui.tableWidget->rowCount(); ++j) {
            if (ui.tableWidget->item(j, 0)->text().compare(processList[i].name, Qt::CaseInsensitive) == 0) {
                ui.tableWidget->item(j, 2)->setText(processList[i].isActive ? "Active" : "Inactive");
                
                //установка фона активности
                if (processList[i].isActive) {
                    ui.tableWidget->item(j, 0)->setBackground(QBrush(Qt::green));
                    ui.tableWidget->item(j, 1)->setBackground(QBrush(Qt::green));
                    ui.tableWidget->item(j, 2)->setBackground(QBrush(Qt::green)); //зеленый для активных
                }
                else {
                    ui.tableWidget->item(j, 0)->setBackground(QBrush(Qt::red));
                    ui.tableWidget->item(j, 1)->setBackground(QBrush(Qt::red));
                    ui.tableWidget->item(j, 2)->setBackground(QBrush(Qt::red)); //красный для неактивных
                }
                
                break; //выход из внутреннего цикла
            }
        }
    }
}