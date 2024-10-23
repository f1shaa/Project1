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

//��������� ������� �����������
struct AutoStartProcess {
    QString name;
    QString path;
    int delay;
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
    void on_actionDelete(); //��� -> �������...
    void on_actionEdit(); //��� -> �������� ��������� �������...
    void on_buttonClose(); //��������� �������
    void on_buttonStart(); //������ ��������� �� �����������

    //��� ������� �����������
    void on_actionDeleteAutoStart(); //��� -> �������...
    void on_actionSetTime(); //��� -> ���������� �����...

private:
    QMenu* contextMenu;
    QMenu* autoStartContextMenu;

    void saveTable(const QString& filePath); //���������� ������ ������������ � CSV
    void loadTable(const QString& filePath); //�������� ������ ������������ �� CSV
    void showContextMenu(const QPoint& pos); //����������� ������������ ���� ��� ���������

    void saveAutoStartTable(const QString& filePath); //���������� ������ ����������� � CSV
    void loadAutoStartTable(const QString& filePath); //�������� ������ ����������� � CSV
    void showContextMenu2(const QPoint& pos); //����������� ������������ ���� ��� �����������

    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;
    QList<AutoStartProcess> autoStartProcesses;

    const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
};
