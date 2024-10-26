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

//контекстное меню
QMenu* contextMenu;

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    //запуск приложения
    ui.setupUi(this);

    //загрузка таблиц при запуске
    loadTable(csvFilePath);

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
    QAction* closeAction = new QAction("Завершить процесс", this);
    connect(closeAction, &QAction::triggered, this, &Project1::on_buttonClose);
    QAction* deleteAction = new QAction("Удалить", this);
    connect(deleteAction, &QAction::triggered, this, &Project1::on_actionDelete);
    QAction* editAction = new QAction("Добавить в автозапуск", this);
    connect(editAction, &QAction::triggered, this, &Project1::on_actionEdit);
    //добавления действия в меню
    contextMenu->addAction(closeAction);
    contextMenu->addAction(deleteAction);
    contextMenu->addAction(editAction);
    //политика меню
    ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu);

    //настройка контекстного меню для таблицы автозапуска
    autoStartContextMenu = new QMenu(this);
    QAction* deleteAutoStartAction = new QAction("Удалить", this);
    connect(deleteAutoStartAction, &QAction::triggered, this, &Project1::on_actionDeleteAutoStart);
    QAction* setTimeAction = new QAction("Установить время", this);
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
        ProcessInfo processInfo = { fileNameInfo, filePath, false, false };
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
    //очистка таблицы процессов
    ui.tableWidget->clearContents();
    ui.tableWidget->setRowCount(0);
    processList.clear();

    //очистка таблицы автозапуска
    ui.autoStartTableWidget->clearContents();
    ui.autoStartTableWidget->setRowCount(0);
    autoStartProcesses.clear();

    //очистка CSV файла
    QFile file(csvFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.resize(0); //очистка файла
        file.close();
    }
}

//обработчик нажатия на кнопку ПКМ -> удалить, для таблицы с процессами
void Project1::on_actionDelete() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        ui.tableWidget->removeRow(indexRow); //удалить из таблицы

        //поиск и удаление из таблицы автозапуска
        for (int i = 0; i < autoStartProcesses.size(); ++i) {
            if (processList[indexRow].name == autoStartProcesses[i].name && processList[indexRow].path == autoStartProcesses[i].path) {
                ui.autoStartTableWidget->removeRow(i);
                autoStartProcesses.removeAt(i);
                break;
            }
        }
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
        saveTable(csvFilePath);
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
                    QProcess::startDetached("\"" + processPath + "\"");
                    processList[i].wasStarted = true;
                    });
            }
            else {
                //немедленный запуск, если задержки нет
                QProcess::startDetached("\"" + processPath + "\"");
            }
        }
    }
}

//обработчик нажатия на кнопку ПКМ -> удалить, для таблицы автозапуска
void Project1::on_actionDeleteAutoStart() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        QString processName = ui.autoStartTableWidget->item(indexRow, 0)->text();
        for (int j = 0; j < autoStartProcesses.size(); ++j) {
            if (autoStartProcesses[j].name == processName) {
                autoStartProcesses[j].delay = 0; // Установить задержку в 0 секунд
                break;
            }
        }
        ui.autoStartTableWidget->removeRow(indexRow); //удалить
        saveTable(csvFilePath); //обновить файл автозапуска
    }
};

//обработчик нажатия на кнопку ПКМ -> установить время, для таблицы с автозапуском
void Project1::on_actionSetTime() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        const int minDelay = 1;    //min задержка в секундах
        const int maxDelay = 3600; //max задержка в секундах
        bool ok;
        int delay = QInputDialog::getInt(this, tr("Установить время"),
            tr("Введите задержку в секундах:"),
            ui.autoStartTableWidget->item(indexRow, 2)->text().toInt(),
            minDelay, maxDelay, 1, &ok);

        if (ok) {
            //обновление времени в таблице и в структуре
            ui.autoStartTableWidget->item(indexRow, 2)->setText(QString::number(delay));
            autoStartProcesses[indexRow].delay = delay;

            //сохранение в CSV файл
            saveTable(csvFilePath);
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
            QString isActive = "0";
            QString isAutoStart = "0";
            for (int j = 0; j < ui.autoStartTableWidget->rowCount(); ++j) {
                if (ui.autoStartTableWidget->item(j, 0)->text() == name &&
                    ui.autoStartTableWidget->item(j, 1)->text() == path) {
                    isAutoStart = "1";
                    QString delay = ui.autoStartTableWidget->item(j, 2)->text();
                    stream << name << ',' << path << ',' << isActive << ',' << isAutoStart << ',' << delay << '\n';
                    break;
                }
            }
            if (isAutoStart == "0") {
                stream << name << ',' << path << ',' << isActive << ',' << isAutoStart << ",0\n";
            }
        }
        file.close();
    }
}

//метод для загрузки списка пользователя из CSV файла
void Project1::loadTable(const QString& filePath) {
    QFile file(filePath);

    //очистка таблиц перед загрузкой данных
    ui.tableWidget->setRowCount(0);
    ui.autoStartTableWidget->setRowCount(0);

    processList.clear();
    autoStartProcesses.clear();

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 5) continue; //пропуск некорректных строк

            //добавление процесса в список processList
            ProcessInfo processInfo = { rowData[0], rowData[1], false };
            processList.append(processInfo);

            int rowCount = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(rowCount);

            ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя файла
            ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь к файлу
            ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //статус по умолочанию

            //если установлен флаг активности
            if (rowData[3] == "1") {
                int rowCount = ui.autoStartTableWidget->rowCount();
                ui.autoStartTableWidget->insertRow(rowCount);
                ui.autoStartTableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя
                ui.autoStartTableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь
                ui.autoStartTableWidget->setItem(rowCount, 2, new QTableWidgetItem(rowData[4])); //время
            
                AutoStartProcess autoStartProcess = { rowData[0], rowData[1], rowData[4].toInt() };
                autoStartProcesses.append(autoStartProcess);
            }
        }
        file.close();
    }
    on_buttonStart(); //автозапуск процессов из автозапуска
}

//метод для перезапуска закрытых ранее открытых процессов
void Project1::checkAndRestartProcesses() {
    for (int i = 0; i < processList.size(); ++i) {
        if (!processList[i].isActive && processList[i].wasStarted) {
            bool foundInAutoStartProcesses = false; //флаг присутствия в автозапуске

            //перебор всех процессов в таблице автозапуска
            for (int j = 0; j < autoStartProcesses.size(); ++j) {
                //поиск процесса в автозапуске
                if (autoStartProcesses[j].name == processList[i].name) {
                    foundInAutoStartProcesses = true; //процесс есть в списке!
                    int delay = autoStartProcesses[j].delay;

                    if (!processRestartFlags.value(processList[i].name, false)) {
                        processRestartFlags[processList[i].name] = true;

                        //если есть время отложенного запуска
                        if (delay > 0) {
                            processList[i].isActive = true;
                            QTimer::singleShot(delay * 1000, [=]() {
                                //запуск с задержкой
                                QProcess::startDetached("\"" + processList[i].path + "\"");
                                processRestartFlags[processList[i].name] = false;
                                });
                            return;
                        }
                        else {
                            //мнгновенный запуск
                            QProcess::startDetached("\"" + processList[i].path + "\"");
                            processRestartFlags[processList[i].name] = false;
                            processList[i].isActive = true;
                        }
                        break;
                    }
                }
            }
            //если процесса не было в списке, перезапустить без задержек
            if (!foundInAutoStartProcesses) {
                QProcess::startDetached("\"" + processList[i].path + "\"");
            }
        }
    }
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
                    processList[i].wasStarted = true; //флаг, отслеживания запуска процесса
                    break;
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
    checkAndRestartProcesses();
}