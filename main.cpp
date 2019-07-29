#include "csdapplicationfilter.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QApplication::setApplicationName("qt-csd");
    MainWindow mainWindow;
    mainWindow.resize(640, 480);

    CSD::ApplicationFilter sf;

    mainWindow.show();
    return app.exec();
}
