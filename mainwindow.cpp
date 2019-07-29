#include "mainwindow.h"

#include <QBoxLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto vlayout = new QVBoxLayout;
    auto vwidget = new QWidget(this);
    vwidget->setLayout(vlayout);
    this->setCentralWidget(vwidget);
    auto button = new QPushButton("Show alert", this);
    QObject::connect(button, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Hello", "You clicked the button");
    });
    auto layout = new QHBoxLayout();
    auto subWidget = new QWidget(this);
    layout->addStretch();
    layout->addWidget(button);
    layout->addStretch();
    subWidget->setLayout(layout);
    vlayout->addWidget(subWidget);
}

MainWindow::~MainWindow() noexcept = default;
