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
    void on_actionDelete(); //��� -> �������...
    void on_actionEdit(); //��� -> �������� ��������� �������...

private:
    void saveTable(const QString& filePath); //���������� ������� � CSV
    void loadTable(const QString& filePath); //�������� ������� �� CSV
    void showContextMenu(const QPoint& pos); //����������� ������������ ����

    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;

    QMenu* contextMenu;
    const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
};
