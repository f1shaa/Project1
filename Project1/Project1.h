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
    bool wasStarted;
};

//структура таблицы автозапуска
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
    void checkProcesses(); //проверка состояния процессов
    void on_actionOpen(); //файл -> открыть...
    void on_actionClear(); //таблица -> очистить...
    void on_actionDelete(); //ПКМ -> удалить...
    void on_actionEdit(); //ПКЬ -> изменить параметры запуска...
    void on_buttonClose(); //завершить процесс
    void on_buttonStart(); //запуск процессов из автозапуска
    void tabActive(int index); //определения активной таблицы
    void on_DeleteButtonClicked(); //метод для кнопки удалить (в быстром доступе)
    void on_actionSetTimeClicked(); //метод для кнопки отложить автозапуск (в быстром доступе)

    //для таблицы автозапуска
    void on_actionDeleteAutoStart(); //ПКМ -> удалить...
    void on_actionSetTime(); //ПКМ -> установить время...

    void checkAndRestartProcesses(); //перезапуск закрытых процессов в процессе работы приложения

private:
    QMenu* contextMenu;
    QMenu* autoStartContextMenu;
    QHash<QString, bool> processRestartFlags;

    void saveTable(const QString& filePath); //сохранение списка пользователя в CSV
    void loadTable(const QString& filePath); //загрузка списка пользователя из CSV
    void showContextMenu(const QPoint& pos); //отображение контекстного меню для процессов
    int currentTabIndex; //индекс активной таблицы
    void showContextMenu2(const QPoint& pos); //отображение контекстного меню для автозапуска

    Ui::Project1Class ui;
    QTimer* timer;
    QList<ProcessInfo> processList;
    QList<AutoStartProcess> autoStartProcesses;

    const QString csvFilePath = QDir::currentPath() + "/process_list.csv";
};
