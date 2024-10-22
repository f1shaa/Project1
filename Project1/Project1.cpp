#include "Project1.h"
#include <QTimer>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <windows.h>
#include <tlhelp32.h>

//путь к файлу
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //настройки таймера
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Project1::checkProcesses);
    timer->start(1000); //интервал в 1с
}

Project1::~Project1()
{}

//метод определения активности процессов
void Project1::checkProcesses() {
    //снимок всех процессов в системе
    HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe32; //инф-я о процессе
    pe32.dwSize = sizeof(PROCESSENTRY32); //размер структуры

    //установка статуса по умолочанию для всех процессов
    for (int i = 0; i < processList.size(); i++) {
        processList[1].isActive = false;
    }

    //перебор всех процессов
    if (Process32First(hProcessesSnapshot, &pe32)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);

            for (int i = 0; i < processList.size(); i++) {
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