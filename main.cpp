#include <QApplication>
#include <QBoxLayout>
#include <QMainWindow>
#include <QPushButton>

#include "csdtitlebar.h"
#ifdef _WIN32
#include "win32csd.h"
#else
#include "linuxcsd.h"
#endif

class DemoWindow : public QMainWindow {

public:
    DemoWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        this->setCentralWidget(new QWidget(this));
        auto *layout = new QVBoxLayout;
        layout->setMargin(0);
        this->centralWidget()->setLayout(layout);
        auto *button = new QPushButton("Toggle full screen", this);
        connect(button, &QPushButton::clicked, this, [this] {
            this->setWindowState(this->windowState() ^ Qt::WindowFullScreen);
        });
        auto *centralLayout = new QHBoxLayout();
        auto *subWidget = new QWidget(this);
        centralLayout->addStretch();
        centralLayout->addWidget(button);
        centralLayout->addStretch();
        subWidget->setLayout(centralLayout);
        this->m_titleBar = new CSD::TitleBar(
#ifdef _WIN32
            CSD::CaptionButtonStyle::win,
#else
            CSD::CaptionButtonStyle::custom,
#endif
            QIcon(),
            this);
        layout->addWidget(this->m_titleBar);
        layout->addWidget(subWidget);

        connect(
            this->m_titleBar, &CSD::TitleBar::minimizeClicked, this, [this]() {
                this->setWindowState(this->windowState() |
                                     Qt::WindowMinimized);
            });
        connect(this->m_titleBar,
                &CSD::TitleBar::maximizeRestoreClicked,
                this,
                [this]() {
                    this->setWindowState(this->windowState() ^
                                         Qt::WindowMaximized);
                });
        connect(this->m_titleBar,
                &CSD::TitleBar::closeClicked,
                this,
                &QWidget::close);
    }

    CSD::TitleBar *titleBar() {
        return this->m_titleBar;
    }

private:
    CSD::TitleBar *m_titleBar;
};

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    auto *app = new QApplication(argc, argv);
    QApplication::setApplicationName("qt-csd");
    auto *mainWindow = new DemoWindow();
    mainWindow->resize(640, 480);

#ifdef _WIN32
    auto *filter = new CSD::Internal::Win32ClientSideDecorationFilter(app);
    app->installNativeEventFilter(filter);
#else
    auto *filter = new CSD::Internal::LinuxClientSideDecorationFilter(app);
#endif
    filter->apply(
        mainWindow,
#ifdef _WIN32
        [mainWindow]() { return mainWindow->titleBar()->hovered(); },
#endif
        [mainWindow] {
            const bool on = mainWindow->isActiveWindow();
            mainWindow->titleBar()->setActive(on);
        },
        [mainWindow] {
            mainWindow->titleBar()->onWindowStateChange(
                mainWindow->windowState());
        });

    mainWindow->show();
    return app->exec();
}
