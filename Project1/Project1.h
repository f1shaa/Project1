#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Project1.h"
#include <QTimer>

//структура информации о процессе
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
    void checkProcesses(); //проверка состояния процессов
    void on_actionOpen(); //файл -> открыть...
    void on_actionClear(); //таблица -> очистить...

private:
    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;

};
