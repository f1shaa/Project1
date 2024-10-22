#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Project1.h"

class Project1 : public QMainWindow
{
    Q_OBJECT

public:
    Project1(QWidget *parent = nullptr);
    ~Project1();

private:
    Ui::Project1Class ui;
};
