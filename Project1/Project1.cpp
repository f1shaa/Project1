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

//путь к файлу
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";

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

    //настройки таймера
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Project1::checkProcesses);
    timer->start(1000); //интервал в 1с
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

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 2) continue; //пропуск некорректных строк

            int rowCount = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(rowCount);

            ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //имя файла
            ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //путь к файлу
            ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //статус по умолочанию
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