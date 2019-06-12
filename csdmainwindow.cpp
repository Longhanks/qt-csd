#include "csdmainwindow.h"

#ifdef __APPLE__

static id nsstringFromUTF8(const char *string) {
    id NSString = toID(objc_getClass("NSString"));
    SEL withUTF8 = sel_registerName("stringWithUTF8String:");
    return objc_msgSend(NSString, withUTF8, string);
}

static std::string fromNSString(id nsstring) {
    id utf8 = objc_msgSend(nsstring, sel_registerName("UTF8String"));
    return std::string(reinterpret_cast<const char *>(utf8));
}

static id nscolorSecondaryLabel() {
    id NSColor = toID(objc_getClass("NSColor"));
    return objc_msgSend(NSColor, sel_registerName("secondaryLabelColor"));
}

static id nscolorTertiaryLabel() {
    id NSColor = toID(objc_getClass("NSColor"));
    return objc_msgSend(NSColor, sel_registerName("tertiaryLabelColor"));
}

static IMP originalWindowDidBecomeKey = nullptr;

static void windowDidBecomeKey(id objc_self, SEL objc_cmd, id notification) {
    id window = objc_msgSend(notification, sel_registerName("object"));
    id toolbar = objc_msgSend(window, sel_registerName("toolbar"));
    id items = objc_msgSend(toolbar, sel_registerName("items"));
    id count_ptr = objc_msgSend(items, sel_registerName("count"));
    unsigned long count = reinterpret_cast<unsigned long>(count_ptr);
    for (unsigned long i = 0; i < count; ++i) {
        id item = objc_msgSend(items, sel_registerName("objectAtIndex:"), i);
        id view = objc_msgSend(item, sel_registerName("view"));
        if (view == nullptr) {
            continue;
        }
        id viewClass = objc_msgSend(view, sel_registerName("class"));
        id desc = objc_msgSend(viewClass, sel_registerName("description"));
        if (fromNSString(desc) == "NSTextField") {
            objc_msgSend(view,
                         sel_registerName("setTextColor:"),
                         nscolorSecondaryLabel());
        }
    }
    if (originalWindowDidBecomeKey != nullptr) {
        originalWindowDidBecomeKey(objc_self, objc_cmd, notification);
    }
}

static IMP originalWindowDidResignKey = nullptr;

static void windowDidResignKey(id objc_self, SEL objc_cmd, id notification) {
    id window = objc_msgSend(notification, sel_registerName("object"));
    id toolbar = objc_msgSend(window, sel_registerName("toolbar"));
    id items = objc_msgSend(toolbar, sel_registerName("items"));
    id count_ptr = objc_msgSend(items, sel_registerName("count"));
    unsigned long count = reinterpret_cast<unsigned long>(count_ptr);
    for (unsigned long i = 0; i < count; ++i) {
        id item = objc_msgSend(items, sel_registerName("objectAtIndex:"), i);
        id view = objc_msgSend(item, sel_registerName("view"));
        if (view == nullptr) {
            continue;
        }
        id viewClass = objc_msgSend(view, sel_registerName("class"));
        id desc = objc_msgSend(viewClass, sel_registerName("description"));
        if (fromNSString(desc) == "NSTextField") {
            objc_msgSend(view,
                         sel_registerName("setTextColor:"),
                         nscolorTertiaryLabel());
        }
    }
    if (originalWindowDidResignKey != nullptr) {
        originalWindowDidResignKey(objc_self, objc_cmd, notification);
    }
}

static id toolbar_itemForIdentifier_willBeInsertedIntoToolbar(
    [[maybe_unused]] id objc_self,
    [[maybe_unused]] SEL objc_cmd,
    [[maybe_unused]] id toolbar,
    id itemIdentifier,
    [[maybe_unused]] signed char flag) {
    if (fromNSString(itemIdentifier) == "ToolbarItemLabelTitle") {
        id NSTextField = toID(objc_getClass("NSTextField"));
        id label = objc_msgSend(NSTextField,
                                sel_registerName("labelWithString:"),
                                nsstringFromUTF8("Label"));
        SEL setTextColor = sel_registerName("setTextColor:");
        objc_msgSend(label, setTextColor, nscolorSecondaryLabel());
        id NSToolbarItem = toID(objc_getClass("NSToolbarItem"));
        id item = objc_msgSend(
            objc_msgSend(NSToolbarItem, sel_registerName("alloc")),
            sel_registerName("initWithItemIdentifier:"),
            nsstringFromUTF8("ToolbarItemLabelTitle"));
        objc_msgSend(item, sel_registerName("setView:"), label);
        return item;
    }
    return nil;
}

static id toolbarDefaultItemIdentifiers([[maybe_unused]] id objc_self,
                                        [[maybe_unused]] SEL objc_cmd,
                                        [[maybe_unused]] id toolbar) {
    id NSMutableArray = toID(objc_getClass("NSMutableArray"));
    SEL withCapacity = sel_registerName("arrayWithCapacity:");
    id mutableArray = objc_msgSend(NSMutableArray, withCapacity, 3);
    id spaceID = nsstringFromUTF8("NSToolbarFlexibleSpaceItem");
    id labelID = nsstringFromUTF8("ToolbarItemLabelTitle");
    objc_msgSend(mutableArray, sel_registerName("addObject:"), spaceID);
    objc_msgSend(mutableArray, sel_registerName("addObject:"), labelID);
    objc_msgSend(mutableArray, sel_registerName("addObject:"), spaceID);
    return mutableArray;
}

static id toolbarAllowedItemIdentifiers([[maybe_unused]] id objc_self,
                                        [[maybe_unused]] SEL objc_cmd,
                                        [[maybe_unused]] id toolbar) {
    id NSMutableArray = toID(objc_getClass("NSMutableArray"));
    SEL withCapacity = sel_registerName("arrayWithCapacity:");
    id mutableArray = objc_msgSend(NSMutableArray, withCapacity, 2);
    id spaceID = nsstringFromUTF8("NSToolbarFlexibleSpaceItem");
    id labelID = nsstringFromUTF8("ToolbarItemLabelTitle");
    objc_msgSend(mutableArray, sel_registerName("addObject:"), spaceID);
    objc_msgSend(mutableArray, sel_registerName("addObject:"), labelID);
    return mutableArray;
}

#endif

#ifdef _WIN32

#include <Windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#endif

#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QGuiApplication>
#include <QPushButton>
#include <QWidget>
#include <QWindow>

#include <qpa/qplatformnativeinterface.h>

namespace CSD {

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags) {
#ifdef __APPLE__
    Class NSObject = objc_getClass("NSObject");
    Class CSDDelegate = objc_allocateClassPair(NSObject, "CSDDelegate", 0);
    class_addMethod(
        CSDDelegate,
        sel_registerName(
            "toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:"),
        reinterpret_cast<IMP>(
            toolbar_itemForIdentifier_willBeInsertedIntoToolbar),
        "@@:@@c");
    class_addMethod(CSDDelegate,
                    sel_registerName("toolbarDefaultItemIdentifiers:"),
                    reinterpret_cast<IMP>(toolbarDefaultItemIdentifiers),
                    "@@:@");
    class_addMethod(CSDDelegate,
                    sel_registerName("toolbarAllowedItemIdentifiers:"),
                    reinterpret_cast<IMP>(toolbarAllowedItemIdentifiers),
                    "@@:@");
    objc_registerClassPair(CSDDelegate);

    SEL alloc = sel_registerName("alloc");
    SEL init = sel_registerName("init");
    void *windowID = reinterpret_cast<void *>(this->winId());
    id window = objc_msgSend(toID(windowID), sel_registerName("window"));

    id delegate = objc_msgSend(objc_msgSend(toID(CSDDelegate), alloc), init);
    id NSToolbar = toID(objc_getClass("NSToolbar"));
    id toolbar = objc_msgSend(objc_msgSend(NSToolbar, alloc), init);
    objc_msgSend(toolbar, sel_registerName("setDelegate:"), delegate);
    objc_msgSend(window, sel_registerName("setToolbar:"), toolbar);
    objc_msgSend(window, sel_registerName("setTitleVisibility:"), 1);

    id windowDelegate = objc_msgSend(window, sel_registerName("delegate"));
    Class windowDelegateClass = reinterpret_cast<Class>(
        objc_msgSend(windowDelegate, sel_registerName("class")));

    Method methodWindowDidBecomeKey = class_getInstanceMethod(
        windowDelegateClass, sel_registerName("windowDidBecomeKey:"));
    if (methodWindowDidBecomeKey == nullptr) {
        class_addMethod(windowDelegateClass,
                        sel_registerName("windowDidBecomeKey:"),
                        reinterpret_cast<IMP>(windowDidBecomeKey),
                        "v@:@");
    } else {
        originalWindowDidBecomeKey =
            method_getImplementation(methodWindowDidBecomeKey);
        method_setImplementation(methodWindowDidBecomeKey,
                                 reinterpret_cast<IMP>(windowDidBecomeKey));
    }

    Method methodWindowDidResignKey = class_getInstanceMethod(
        windowDelegateClass, sel_registerName("windowDidResignKey:"));
    if (methodWindowDidResignKey == nullptr) {
        class_addMethod(windowDelegateClass,
                        sel_registerName("windowDidResignKey:"),
                        reinterpret_cast<IMP>(windowDidResignKey),
                        "v@:@");
    } else {
        originalWindowDidResignKey =
            method_getImplementation(methodWindowDidResignKey);
        method_setImplementation(methodWindowDidResignKey,
                                 reinterpret_cast<IMP>(windowDidResignKey));
    }

    // Force unset & set delegate to pick up new methods
    objc_msgSend(window, sel_registerName("setDelegate:"), nullptr);
    objc_msgSend(window, sel_registerName("setDelegate:"), windowDelegate);
#else
#if !defined(_WIN32)
    this->setWindowFlags(this->windowFlags() |= Qt::FramelessWindowHint);
#endif
    this->m_titleBar = new TitleBar(this);
    auto centralWidget = new QWidget(this);
    auto centralLayout = new QVBoxLayout();
    centralLayout->addWidget(this->m_titleBar);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setStretch(0, 0);
    centralWidget->setLayout(centralLayout);
    this->setCentralWidget(centralWidget);

    this->m_titleBar->setTitle(this->windowTitle());
    connect(this, &QWidget::windowTitleChanged, this, [this]() {
        this->m_titleBar->setTitle(this->windowTitle());
    });
    if (this->windowTitle().isEmpty()) {
        this->setWindowTitle(QApplication::applicationName());
    }

    connect(this->m_titleBar, &TitleBar::minimize_clicked, this, [this]() {
        this->setWindowState(this->windowState() | Qt::WindowMinimized);
    });
    connect(
        this->m_titleBar, &TitleBar::maxmize_restore_clicked, this, [this]() {
            this->setWindowState(this->windowState() ^ Qt::WindowMaximized);
        });
    connect(
        this->m_titleBar, &TitleBar::close_clicked, this, &QMainWindow::close);
#endif
}

#ifndef __APPLE__
void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ActivationChange ||
        event->type() == QEvent::WindowStateChange) {
        this->m_titleBar->onWindowStateChange(this->windowState());
    }
    QMainWindow::changeEvent(event);
}
#endif

#ifdef _WIN32
bool MainWindow::nativeEvent(const QByteArray &eventType,
                             void *message,
                             long *result) {
    auto msg = static_cast<MSG *>(message);

    if (msg->message == WM_CREATE) {
        auto clientRect = ::RECT();
        ::GetWindowRect(reinterpret_cast<HWND>(this->winId()), &clientRect);
        ::SetWindowPos(reinterpret_cast<HWND>(this->winId()),
                       nullptr,
                       clientRect.left,
                       clientRect.top,
                       clientRect.right - clientRect.left,
                       clientRect.bottom - clientRect.top,
                       SWP_FRAMECHANGED);
    }

    if (msg->message == WM_ACTIVATE) {
        auto margins = ::MARGINS();
        margins.cxLeftWidth = 1;
        margins.cxRightWidth = 1;
        margins.cyBottomHeight = 1;
        margins.cyTopHeight = 1;
        ::DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(this->winId()),
                                       &margins);
    }

    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE) {
        auto calcSizeParams =
            reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);

        calcSizeParams->rgrc[0].left = calcSizeParams->rgrc[0].left + 0;
        calcSizeParams->rgrc[0].top = calcSizeParams->rgrc[0].top + 0;
        calcSizeParams->rgrc[0].right = calcSizeParams->rgrc[0].right - 0;
        calcSizeParams->rgrc[0].bottom = calcSizeParams->rgrc[0].bottom - 0;

        auto windowPlacement = ::WINDOWPLACEMENT();
        if (::GetWindowPlacement(msg->hwnd, &windowPlacement)) {
            if (windowPlacement.showCmd == SW_MAXIMIZE) {
                auto monitor =
                    ::MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONULL);
                if (monitor) {
                    auto monitorInfo = ::MONITORINFO();
                    monitorInfo.cbSize = sizeof(monitorInfo);
                    if (::GetMonitorInfoW(monitor, &monitorInfo)) {
                        calcSizeParams->rgrc[0] = monitorInfo.rcWork;
                    }
                }
            }
        }

        *result = 0;
        return true;
    }

    if (msg->message == WM_NCHITTEST) {
        *result = 0;
        const LONG borderWidth = 8;
        auto clientRect = ::RECT();
        ::GetWindowRect(reinterpret_cast<HWND>(this->winId()), &clientRect);

        auto x = GET_X_LPARAM(msg->lParam);
        auto y = GET_Y_LPARAM(msg->lParam);

        auto resizeWidth = this->minimumWidth() != this->maximumWidth();
        auto resizeHeight = this->minimumHeight() != this->maximumHeight();

        if (resizeWidth) {
            if (x >= clientRect.left && x < clientRect.left + borderWidth) {
                *result = HTLEFT;
            }
            if (x < clientRect.right && x >= clientRect.right - borderWidth) {
                *result = HTRIGHT;
            }
        }
        if (resizeHeight) {
            if (y < clientRect.bottom &&
                y >= clientRect.bottom - borderWidth) {
                *result = HTBOTTOM;
            }
            if (y >= clientRect.top && y < clientRect.top + borderWidth) {
                *result = HTTOP;
            }
        }
        if (resizeWidth && resizeHeight) {
            if (x >= clientRect.left && x < clientRect.left + borderWidth &&
                y < clientRect.bottom &&
                y >= clientRect.bottom - borderWidth) {
                *result = HTBOTTOMLEFT;
            }
            if (x < clientRect.right && x >= clientRect.right - borderWidth &&
                y < clientRect.bottom &&
                y >= clientRect.bottom - borderWidth) {
                *result = HTBOTTOMRIGHT;
            }
            if (x >= clientRect.left && x < clientRect.left + borderWidth &&
                y >= clientRect.top && y < clientRect.top + borderWidth) {
                *result = HTTOPLEFT;
            }
            if (x < clientRect.right && x >= clientRect.right - borderWidth &&
                y >= clientRect.top && y < clientRect.top + borderWidth) {
                *result = HTTOPRIGHT;
            }
        }

        if (*result != 0) {
            return true;
        }

        if (this->m_titleBar->hovered()) {
            *result = HTCAPTION;
            return true;
        }
    }

    if (msg->message == WM_NCACTIVATE) {
        auto enabled = FALSE;
        auto success = ::DwmIsCompositionEnabled(&enabled) == S_OK;
        if (!(enabled && success)) {
            *result = 1;
            return true;
        }
    }

    if (msg->message == WM_GETMINMAXINFO) {
        auto minMaxInfo = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        auto windowPlacement = ::WINDOWPLACEMENT();
        if (::GetWindowPlacement(msg->hwnd, &windowPlacement)) {
            if (windowPlacement.showCmd == SW_MAXIMIZE) {
                auto clientRect = ::RECT();

                if (!::GetWindowRect(msg->hwnd, &clientRect)) {
                    return false;
                }

                auto monitor =
                    ::MonitorFromRect(&clientRect, MONITOR_DEFAULTTONULL);
                if (!monitor) {
                    return false;
                }

                auto monitorInfo = ::MONITORINFO();
                monitorInfo.cbSize = sizeof(monitorInfo);
                ::GetMonitorInfo(monitor, &monitorInfo);

                auto workingArea = monitorInfo.rcWork;
                auto monitorArea = monitorInfo.rcMonitor;

                minMaxInfo->ptMaxPosition.x =
                    std::abs(workingArea.left - monitorArea.left);
                minMaxInfo->ptMaxPosition.y =
                    std::abs(workingArea.top - monitorArea.top);

                minMaxInfo->ptMaxSize.x =
                    std::abs(workingArea.right - workingArea.left);
                minMaxInfo->ptMaxSize.y =
                    std::abs(workingArea.bottom - workingArea.top);
                minMaxInfo->ptMaxTrackSize.x = minMaxInfo->ptMaxSize.x;
                minMaxInfo->ptMaxTrackSize.y = minMaxInfo->ptMaxSize.y;

                *result = 1;
                return true;
            }
        }
    }

    if (msg->message == WM_SIZE) {
        auto windowPlacement = ::WINDOWPLACEMENT();
        windowPlacement.length = sizeof(WINDOWPLACEMENT);
        ::GetWindowPlacement(msg->hwnd, &windowPlacement);
        if (windowPlacement.showCmd == SW_MAXIMIZE) {
            ::SetWindowPos(reinterpret_cast<HWND>(this->winId()),
                           nullptr,
                           0,
                           0,
                           0,
                           0,
                           SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
        }
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::showEvent(QShowEvent *event) {
    if (QWindow *window = this->windowHandle()) {
        auto rect = ::RECT{0, 0, 0, 0};
        ::AdjustWindowRectEx(&rect,
                             WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                                 WS_THICKFRAME | WS_DLGFRAME,
                             FALSE,
                             0);
        auto marginBottom =
            std::abs(rect.bottom) + ::GetSystemMetrics(SM_CYCAPTION);
        const auto margins = QMargins(-8, -marginBottom, -8, -8);
        const auto variantMargins = qVariantFromValue(margins);
        window->setProperty("_q_windowsCustomMargins", variantMargins);
        if (QPlatformWindow *platformWindow = window->handle()) {
            QGuiApplication::platformNativeInterface()->setWindowProperty(
                platformWindow,
                QStringLiteral("WindowsCustomMargins"),
                variantMargins);
        }
    }

    QMainWindow::showEvent(event);
}
#endif

} // namespace CSD
