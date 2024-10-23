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

//���� � ����� �� ������� ����������
const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
//���� � ����� �� ������� ����������� ����������
const QString csvFilePath2 = QDir::currentPath() + "/process_list_2.csv";

//����������� ����
QMenu* contextMenu;

Project1::Project1(QWidget *parent)
    : QMainWindow(parent)
{
    //������ ����������
    ui.setupUi(this);

    //�������� ������� �� ������� ������������ �� ����� ��� �������
    loadTable(csvFilePath);

    //�������� ������� � ������������ �� ����� ��� �������
    loadAutoStartTable(csvFilePath2);

    //����������� �������� ���� -> �������...
    connect(ui.actionOpen, &QAction::triggered, this, &Project1::on_actionOpen);

    //����������� �������� ������� -> ��������...
    connect(ui.actionClear, &QAction::triggered, this, &Project1::on_actionClear);

    //����������� ������ ��������� �������
    connect(ui.buttonClose, &QPushButton::clicked, this, &Project1::on_buttonClose);

    //��������� �������
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Project1::checkProcesses);
    timer->start(1000); //�������� � 1�

    //��������� ������������ ���� ��� ������� � ����������
    contextMenu = new QMenu(this);
    //��������
    QAction* deleteAction = new QAction("Delete", this);
    connect(deleteAction, &QAction::triggered, this, &Project1::on_actionDelete);
    QAction* editAction = new QAction("Add auto start", this);
    connect(editAction, &QAction::triggered, this, &Project1::on_actionEdit);
    //���������� �������� � ����
    contextMenu->addAction(deleteAction);
    contextMenu->addAction(editAction);
    //�������� ����
    ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu);

    //��������� ������������ ���� ��� ������� �����������
    autoStartContextMenu = new QMenu(this);
    QAction* deleteAutoStartAction = new QAction("Delete", this);
    connect(deleteAutoStartAction, &QAction::triggered, this, &Project1::on_actionDeleteAutoStart);
    QAction* setTimeAction = new QAction("Set Time", this);
    connect(setTimeAction, &QAction::triggered, this, &Project1::on_actionSetTime);
    autoStartContextMenu->addAction(deleteAutoStartAction);
    autoStartContextMenu->addAction(setTimeAction);
    //�������� ����
    ui.autoStartTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.autoStartTableWidget, &QTableWidget::customContextMenuRequested, this, &Project1::showContextMenu2);
}

Project1::~Project1()
{}

//���������� ������� �� ������ ���� -> �������
void Project1::on_actionOpen() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Executable"), "", tr("Executable Files (*.exe);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName); //��������� ���������� � �����
        QString filePath = fileInfo.absoluteFilePath(); //���� � �����
        QString fileNameInfo = fileInfo.fileName(); //��� �����

        //�������� �� ������������ ��������
        for (int i = 0; i < ui.tableWidget->rowCount(); i++) {
            QTableWidgetItem* item = ui.tableWidget->item(i, 0);
            if (item && item->text() == fileNameInfo) {
                QMessageBox::information(this, "Attention", "The process is already on the list!!!");
                return;
            }
        }

        //���������� ���������� � �������� � ������
        ProcessInfo processInfo = { fileNameInfo, filePath, false };
        processList.append(processInfo);

        int rowCount = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(rowCount);

        //���������� ��������
        ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(fileNameInfo)); //��� ����� � 1 �������
        ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(filePath)); //���� �� 2 �������
        ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //������ �� ����������

        //���������� ������� ��� ������ ���������� ������ ��������
        saveTable(csvFilePath);
    }
}

//���������� ������� �� ������ ������� -> ��������
void Project1::on_actionClear() {
    //����������� �������� �������
    if (ui.tableWidget->hasFocus()) {
        //������� ������� ���������
        ui.tableWidget->clearContents();
        ui.tableWidget->setRowCount(0);
        processList.clear();

        //������� CSV ����� ��� �������
        QFile file(csvFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.resize(0); //������� �����
            file.close();
        }
    }
    else if (ui.autoStartTableWidget->hasFocus()) {
        //������� ������� �����������
        ui.autoStartTableWidget->clearContents();
        ui.autoStartTableWidget->setRowCount(0);
        autoStartProcesses.clear();

        //������� CSV ����� ��� �����������
        QFile file(csvFilePath2);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.resize(0); //������� �����
            file.close();
        }
    }
}

//���������� ������� �� ������ ��� -> �������, ��� ������� � ����������
void Project1::on_actionDelete() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        ui.tableWidget->removeRow(indexRow); //������� �� �������
        processList.removeAt(indexRow); //������� �� ������
        saveTable(csvFilePath); //�������� ����
    }
}

//���������� ������� �� ������ ��� -> �������� ��������� �������, ��� ������� � ����������
void Project1::on_actionEdit() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        //��������� ������ � ��������
        QString processName = ui.tableWidget->item(indexRow, 0)->text();
        QString processPath = ui.tableWidget->item(indexRow, 1)->text();

        //�������� �� ������������ �������� � �����������
        for (int i = 0; i < ui.autoStartTableWidget->rowCount(); i++) {
            if (ui.autoStartTableWidget->item(i, 0)->text() == processName) {
                return;
            }
        }

        //���������� �������� � ������� �����������
        int rowCount = ui.autoStartTableWidget->rowCount();
        ui.autoStartTableWidget->insertRow(rowCount);
        ui.autoStartTableWidget->setItem(rowCount, 0, new QTableWidgetItem(processName)); //���
        ui.autoStartTableWidget->setItem(rowCount, 1, new QTableWidgetItem(processPath)); //����
        ui.autoStartTableWidget->setItem(rowCount, 2, new QTableWidgetItem("0")); //����� �� ���������

        autoStartProcesses.append({ processName, processPath, 0 }); //���������� � ������ �����������
        //���������� � ������� �����������
        saveAutoStartTable(csvFilePath2);
    }
}

//���������� ������� �� ������ ��������� �������, ��� ������� � ����������
void Project1::on_buttonClose() {
    int indexRow = ui.tableWidget->currentRow();
    if (indexRow >= 0) {
        //��������� ��������� ��������
        QString processStatus = ui.tableWidget->item(indexRow, 2)->text();

        //�������� ��������� ��������
        if (processStatus == "Active") {
            //��������� ����� ��������
            QString processName = ui.tableWidget->item(indexRow, 0)->text();

            //����� ����� ���� ���������
            HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            bool processFound = false;

            //������� ���� ���������
            if (Process32First(hProcessesSnapshot, &pe32)) {
                do {
                    QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
                    
                    if (currentProcessName.compare(processName, Qt::CaseInsensitive) == 0) {
                        //��������� ID ��������
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProcess != NULL) {
                            //���������� ��������
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
                // ���������� ������� � ������� �� "Inactive"
                ui.tableWidget->item(indexRow, 2)->setText("Inactive");
            }
        }
    }
}

//���������� ������� �� ������ ������, ��� ������� � ����������
void Project1::on_buttonStart() {
    //������� ��������� � �������
    for (int i = 0; i < ui.autoStartTableWidget->rowCount(); ++i) {
        //��������� ��������� ��������
        QString processName = ui.autoStartTableWidget->item(i, 0)->text(); //���
        QString processPath = ui.autoStartTableWidget->item(i, 1)->text(); //����
        int delay = autoStartProcesses[i].delay; //��������

        //�������� ��������� ��������
        bool isActive = false;
        for (const ProcessInfo& processInfo : processList) {
            if (processInfo.name == processName && processInfo.isActive) {
                isActive = true;
                break;
            }
        }

        //������, ���� ���������
        if (!isActive) {
            if (delay > 0) {
                //������ � ���������
                QTimer::singleShot(delay * 1000, [=]() {
                    QProcess::startDetached(processPath);
                    });
            }
            else {
                //����������� ������, ���� �������� ���
                QProcess::startDetached(processPath);
            }
        }
    }
}

//���������� ������� �� ������ ��� -> �������, ��� ������� �����������
void Project1::on_actionDeleteAutoStart() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        ui.autoStartTableWidget->removeRow(indexRow); //�������
        saveAutoStartTable(csvFilePath2); //�������� ���� �����������
    }
};

//���������� ������� �� ������ ��� -> ���������� �����, ��� ������� � ������������
void Project1::on_actionSetTime() {
    int indexRow = ui.autoStartTableWidget->currentRow();
    if (indexRow >= 0) {
        const int minDelay = 1;    //min �������� � ��������
        const int maxDelay = 3600; //max �������� � ��������
        bool ok;
        int delay = QInputDialog::getInt(this, tr("Set Time"),
            tr("Enter delay in seconds:"),
            ui.autoStartTableWidget->item(indexRow, 2)->text().toInt(),
            minDelay, maxDelay, 1, &ok);

        if (ok) {
            //���������� ������� � ������� � � ���������
            ui.autoStartTableWidget->item(indexRow, 2)->setText(QString::number(delay));
            autoStartProcesses[indexRow].delay = delay;

            //���������� � CSV ����
            saveAutoStartTable(csvFilePath2);
        }
    }
}

//����� ����������� ������������ ���� ��� ������ � ����������
void Project1::showContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = ui.tableWidget->itemAt(pos);
    if (item) {
        contextMenu->exec(QCursor::pos());
    }
    else {
        qDebug() << "No item at position" << pos;
    }
}

//����� ����������� ������������ ���� ��� ������� �����������
void Project1::showContextMenu2(const QPoint& pos) {
    QTableWidgetItem* item = ui.autoStartTableWidget->itemAt(pos);
    if (item) {
        autoStartContextMenu->exec(QCursor::pos());
    }
    else {
        qDebug() << "No item at position" << pos;
    }
}

//����� ��� ���������� ������ ������������ � CSV ����
void Project1::saveTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        for (int i = 0; i < ui.tableWidget->rowCount(); ++i) {
            QString name = ui.tableWidget->item(i, 0)->text(); //��� �����
            QString path = ui.tableWidget->item(i, 1)->text(); //���� � �����
            stream << name << "," << path << "\n"; //���������� � ������� CSV
        }
        file.close();
    }
}

//����� ��� �������� ������ ������������ �� CSV �����
void Project1::loadTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;

        //������� ������� ����� ��������� ������
        ui.tableWidget->setRowCount(0);
        processList.clear();

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 2) continue; //������� ������������ �����

            //���������� �������� � ������ processList
            ProcessInfo processInfo = { rowData[0], rowData[1], false };
            processList.append(processInfo);

            int rowCount = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(rowCount);

            ui.tableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //��� �����
            ui.tableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //���� � �����
            ui.tableWidget->setItem(rowCount, 2, new QTableWidgetItem("Inactive")); //������ �� ����������
        }
        file.close();
    }
}

//����� ��� ���������� ������ ����������� � CSV ����
void Project1::saveAutoStartTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        for (int i = 0; i < ui.autoStartTableWidget->rowCount(); ++i) {
            QString name = ui.autoStartTableWidget->item(i, 0)->text(); //���
            QString path = ui.autoStartTableWidget->item(i, 1)->text(); //����
            QString delay = ui.autoStartTableWidget->item(i, 2)->text(); //�����
            stream << name << "," << path << "," << delay << "\n"; //���������
        }
        file.close();
    }
}

//����� ��� �������� ������ ����������� � CSV ����
void Project1::loadAutoStartTable(const QString& filePath) {
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;

        //������� ������� ����� ��������� ������
        ui.autoStartTableWidget->setRowCount(0);
        autoStartProcesses.clear();

        while (!stream.atEnd()) {
            line = stream.readLine();
            QStringList rowData = line.split(",");

            if (rowData.size() < 3) continue; //������� ������������ �����

            //���������� �������� � ������� �����������
            int rowCount = ui.autoStartTableWidget->rowCount();
            ui.autoStartTableWidget->insertRow(rowCount);
            ui.autoStartTableWidget->setItem(rowCount, 0, new QTableWidgetItem(rowData[0])); //���
            ui.autoStartTableWidget->setItem(rowCount, 1, new QTableWidgetItem(rowData[1])); //����
            ui.autoStartTableWidget->setItem(rowCount, 2, new QTableWidgetItem(rowData[2])); //�����
        
            //���������� �������� � ������ �����������
            AutoStartProcess autoStartProcess = { rowData[0], rowData[1], rowData[2].toInt() };
            autoStartProcesses.append(autoStartProcess);
        }
        file.close();
    }
    on_buttonStart(); //���������� �������� �� �������
}

//����� ����������� ���������� ���������
void Project1::checkProcesses() {
    //������ ���� ��������� � �������
    HANDLE hProcessesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe32; //���-� � ��������
    pe32.dwSize = sizeof(PROCESSENTRY32); //������ ���������

    //��������� ������� �� ���������� ��� ���� ���������
    for (int i = 0; i < processList.size(); ++i) {
        processList[i].isActive = false;
    }

    //������� ���� ���������
    if (Process32First(hProcessesSnapshot, &pe32)) {
        do {
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);

            for (int i = 0; i < processList.size(); ++i) {
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
            if (ui.tableWidget->item(j, 0)->text().compare(processList[i].name, Qt::CaseInsensitive) == 0) {
                ui.tableWidget->item(j, 2)->setText(processList[i].isActive ? "Active" : "Inactive");
                
                //��������� ���� ����������
                if (processList[i].isActive) {
                    ui.tableWidget->item(j, 0)->setBackground(QBrush(Qt::green));
                    ui.tableWidget->item(j, 1)->setBackground(QBrush(Qt::green));
                    ui.tableWidget->item(j, 2)->setBackground(QBrush(Qt::green)); //������� ��� ��������
                }
                else {
                    ui.tableWidget->item(j, 0)->setBackground(QBrush(Qt::red));
                    ui.tableWidget->item(j, 1)->setBackground(QBrush(Qt::red));
                    ui.tableWidget->item(j, 2)->setBackground(QBrush(Qt::red)); //������� ��� ����������
                }
                
                break; //����� �� ����������� �����
            }
        }
    }
}