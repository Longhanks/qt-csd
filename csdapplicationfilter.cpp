#include "csdapplicationfilter.h"

#include "csdtitlebar.h"

#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QMainWindow>
#include <QTimer>
#include <QWidget>

namespace CSD {

void ApplicationFilter::setUpTitleBarForWidget(TitleBar *titleBar,
                                               QWidget *widget) {
    QObject::connect(widget, &QObject::destroyed, this, [widget, this]() {
        this->m_filtered.erase(std::remove(std::begin(this->m_filtered),
                                           std::end(this->m_filtered),
                                           widget),
                               std::end(this->m_filtered));
        this->m_filter.unapply(widget);
    });

    QObject::connect(
        titleBar, &CSD::TitleBar::minimizeClicked, widget, [widget]() {
            widget->setWindowState(widget->windowState() |
                                   Qt::WindowMinimized);
        });
    QObject::connect(
        titleBar, &CSD::TitleBar::maximizeRestoreClicked, widget, [widget]() {
            widget->setWindowState(widget->windowState() ^
                                   Qt::WindowMaximized);
        });
    QObject::connect(
        titleBar, &CSD::TitleBar::closeClicked, widget, &QWidget::close);

    m_filter.apply(widget,
#ifdef _WIN32
                   [titleBar]() { return titleBar->hovered(); },
#endif
                   [titleBar]() {
                       const bool on = titleBar->window()->isActiveWindow();
                       titleBar->setActive(on);
                   },
                   [titleBar]() {
                       titleBar->onWindowStateChange(
                           titleBar->window()->windowState());
                   });

    this->m_filtered.push_back(widget);

    widget->hide();
    widget->setWindowFlag(Qt::FramelessWindowHint);
    QTimer::singleShot(0, widget, [widget]() { widget->show(); });
}

bool ApplicationFilter::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() != QEvent::Show) {
        return false;
    }

    auto *widget = qobject_cast<QWidget *>(watched);
    if (widget == nullptr) {
        return false;
    }

    if (std::find(std::begin(this->m_filtered),
                  std::end(this->m_filtered),
                  widget) != std::end(this->m_filtered)) {
        return false;
    }

    if (!widget->isTopLevel()) {
        return false;
    }

    Qt::WindowFlags type = (widget->windowFlags() & Qt::WindowType_Mask);
    if (!(type.testFlag(Qt::Window) || type.testFlag(Qt::Dialog))) {
        return false;
    }

    if (watched->metaObject()->inherits(&QMainWindow::staticMetaObject)) {
        auto mainWindow = static_cast<QMainWindow *>(watched);

        auto titleBar =
            new CSD::TitleBar(QIcon(), mainWindow->centralWidget());

        mainWindow->layout()->setSpacing(0);

        auto *wrapper = new QWidget(mainWindow);
        wrapper->setMinimumHeight(0);

        auto *layout = new QVBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(mainWindow->centralWidget());

        wrapper->setLayout(layout);

        mainWindow->setCentralWidget(wrapper);

        layout->insertWidget(0, titleBar);

        this->setUpTitleBarForWidget(titleBar, mainWindow);
        return true;
    }

    else if (watched->metaObject()->inherits(&QWidget::staticMetaObject)) {
        auto titleBar = new CSD::TitleBar(QIcon(), widget);

        auto helperWidget = new QWidget;
        auto oldLayout = widget->layout();
        if (oldLayout != nullptr) {
            helperWidget->setLayout(oldLayout);
        }

        auto *layout = new QVBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(titleBar);
        layout->addWidget(helperWidget);

        widget->setLayout(layout);

        this->setUpTitleBarForWidget(titleBar, widget);
        return true;
    }

    return false;
}

ApplicationFilter::ApplicationFilter(QObject *parent) : QObject(parent) {
    QApplication::instance()->installEventFilter(this);
}

ApplicationFilter::~ApplicationFilter() {
    QApplication::instance()->removeEventFilter(this);
}

void ApplicationFilter::addExclude(QWidget *widget) {
    this->m_filtered.push_back(widget);
}

} // namespace CSD
