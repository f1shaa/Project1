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

//путь к файлу
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";

//контекстное меню
QMenu* contextMenu;

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    //запуск приложения
    ui.setupUi(this);

    //загрузка таблицы из файла при запуске
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

    //настройка контекстного меню
    contextMenu = new QMenu(this);
    //действия
    QAction* deleteAction = new QAction("delete", this);
    connect(deleteAction, &QAction::triggered, this, &Project1::on_actionDelete);
    QAction* editAction = new QAction("change start settings", this);
    connect(editAction, &QAction::triggered, this, &Project1::on_actionEdit);
    //добавления действия в меню
    contextMenu->addAction(deleteAction);
    contextMenu->addAction(editAction);
    //политика меню
    ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu);
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
                QMessageBox::information(this, "Внимание", "Процесс уже в списке!!!");
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
    //очистка таблицы
    ui.tableWidget->clearContents();
    ui.tableWidget->setRowCount(0);

    //очистка списка процессов
    processList.clear();

    //очистка CSV файла
    QFile file(csvFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //пустая запись в файл для его очистки
        file.resize(0);
        file.close();
    }
    else {
        qDebug() << "Не удалось очистить файл!!!";
    }
}

//обработчик нажатия на кнопку ПКМ -> удалить
void Project1::on_actionDelete() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        ui.tableWidget->removeRow(indexRow); //удалить из таблицы
        processList.removeAt(indexRow); //удалить из списка
        saveTable(csvFilePath); //обновить файл
    }
}

//обработчик нажатия на кнопку ПКМ -> изменить параметры запуска
void Project1::on_actionEdit() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        
    }
}

//обработчик нажатия на кнопку заврешить процесс
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

//метод отображения контекстного меню
void Project1::showContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = ui.tableWidget->itemAt(pos);
    if (item) {
        contextMenu->exec(QCursor::pos());
    }
}

//метод для сохранения данных в CSV файл
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

//метод для загрузки данных из CSV файла
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

            int rowCount = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(rowCount);

            ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя файла
            ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь к файлу
            ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //статус по умолочанию

            //добавление процесса в список processList
            ProcessInfo processInfo = { rowData[0], rowData[1], false };
            processList.append(processInfo);
        }
        file.close();
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
                }
            }
        } while (Process32Next(hProcessesSnapshot, &pe32));
    }
    CloseHandle(hProcessesSnapshot);

    //обновление статуса в таблице
    for (int i = 0; i < processList.size(); ++i) {
        for (int j = 0; j < ui.tableWidget->rowCount(); ++j) {
            if (ui.tableWidget->item(i, 0)->text() == processList[i].name) {
                ui.tableWidget->item(j, 2)->setText(processList[i].isActive ? "Active" : "Inactive");

            }
        }
        //проверка закрытых процессов
        if (!processList[i].isActive) {
            for (int j = 0; j < ui.tableWidget->rowCount(); ++j) {
                if (ui.tableWidget->item(j, 0)->text() == processList[i].name) {
                    ui.tableWidget->item(j, 2)->setText("Inactive");
                }
            }
        }

        //сброс статуса для следующей проверки
        processList[i].isActive = false;
    }
}