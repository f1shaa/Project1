#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Project1.h"
#include <QTimer>
#include <QDir>

//��������� ���������� � ��������
struct ProcessInfo {
    QString name;
    QString path;
    bool isActive;
};

class Project1 : public QMainWindow
{
    Q_OBJECT

public:
    Project1(QWidget *parent = nullptr);
    ~Project1();

private slots:
    void checkProcesses(); //�������� ��������� ���������
    void on_actionOpen(); //���� -> �������...
    void on_actionClear(); //������� -> ��������...

private:
    void saveTable(const QString& filePath); //���������� ������� � CSV
    void loadTable(const QString& filePath); //�������� ������� �� CSV

    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;

    const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
};
