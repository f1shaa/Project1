#include "Project1.h"
#include <QTimer>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <windows.h>
#include <tlhelp32.h>

//���� � �����
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    //��������� �������
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Project1::checkProcesses);
    timer->start(1000); //�������� � 1�
}

Project1::~Project1()
{}

//����� ����������� ���������� ���������
void Project1::checkProcesses() {
    //������ ���� ��������� � �������
    HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe32; //���-� � ��������
    pe32.dwSize = sizeof(PROCESSENTRY32); //������ ���������

    //��������� ������� �� ���������� ��� ���� ���������
    for (int i = 0; i < processList.size(); i++) {
        processList[1].isActive = false;
    }

    //������� ���� ���������
    if (Process32First(hProcessesSnapshot, &pe32)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);

            for (int i = 0; i < processList.size(); i++) {
                if (processList[i].name == currentProcessName) {
                    processList[i].isActive = true; //������� ������ -> �������
                }
            }
        } while (Process32Next(hProcessesSnapshot, &pe32));
    }
    CloseHandle(hProcessesSnapshot);

    //���������� ������� � �������
    for (int i = 0; i < processList.size(); ++i) {
        for (int j = 0; j < ui.tableWidget->rowCount(); ++j) {
            if (ui.tableWidget->item(i, 0)->text() == processList[i].name) {
                ui.tableWidget->item(j, 2)->setText(processList[i].isActive ? "Active" : "Inactive");

            }
        }
        //�������� �������� ���������
        if (!processList[i].isActive) {
            for (int j = 0; j < ui.tableWidget->rowCount(); ++j) {
                if (ui.tableWidget->item(j, 0)->text() == processList[i].name) {
                    ui.tableWidget->item(j, 2)->setText("Inactive");
                }
            }
        }

        //����� ������� ��� ��������� ��������
        processList[i].isActive = false;
    }
}