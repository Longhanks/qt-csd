#include "csdmainwindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QPushButton>

class DemoWindow : public CSD::MainWindow {

public:
    DemoWindow(QWidget *parent = nullptr) : CSD::MainWindow(parent) {
        if (!this->centralWidget()) {
            this->setCentralWidget(new QWidget(this));
            this->centralWidget()->setLayout(new QHBoxLayout);
        }
        auto button = new QPushButton("Show alert", this);
        auto layout = new QHBoxLayout();
        auto subWidget = new QWidget(this);
        layout->addStretch();
        layout->addWidget(button);
        layout->addStretch();
        subWidget->setLayout(layout);
        this->centralWidget()->layout()->addWidget(subWidget);
    }
};

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    auto app = new QApplication(argc, argv);
    QApplication::setApplicationName("qt-csd");
    auto mainWindow = new DemoWindow();
    mainWindow->resize(640, 480);
    mainWindow->show();
    return app->exec();
}
