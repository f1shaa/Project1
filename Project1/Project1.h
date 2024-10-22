#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Project1.h"
#include <QTimer>

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
    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;

};
