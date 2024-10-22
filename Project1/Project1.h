#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Project1.h"
#include <QTimer>
#include <QDir>

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
    void on_actionDelete(); //ПКМ -> удалить...
    void on_actionEdit(); //ПКЬ -> изменить параметры запуска...

private:
    void saveTable(const QString& filePath); //сохранение таблицы в CSV
    void loadTable(const QString& filePath); //загрузка таблицы из CSV
    void showContextMenu(const QPoint& pos); //отображение контекстного меню

    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;

    QMenu* contextMenu;
    const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
};
