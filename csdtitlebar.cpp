#include "csdtitlebar.h"

#include "csdtitlebarbutton.h"

#ifdef _WIN32
#include "qregistrywatcher.h"
#include "qtwinbackports.h"

#include <Windows.h>
#include <dwmapi.h>
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <QMouseEvent>
#include <QWindow>

#include <QX11Info>

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformwindow.h>

#include <cstring>
#endif

namespace CSD {

#if !defined(_WIN32) && !defined(__APPLE__)
constexpr static const char _NET_WM_MOVERESIZE[] = "_NET_WM_MOVERESIZE";

static QWidget *titleBarTopLevelWidget(QWidget *w) {
    while (w && !w->isWindow() && w->windowType() != Qt::SubWindow) {
        w = w->parentWidget();
    }
    return w;
}
#endif

TitleBar::TitleBar(CaptionButtonStyle captionButtonStyle,
                   const QIcon &captionIcon,
                   QWidget *parent)
    : QWidget(parent), m_captionButtonStyle(captionButtonStyle) {
    this->setObjectName("TitleBar");
    this->setMinimumSize(QSize(0, 30));
    this->setMaximumSize(QSize(QWIDGETSIZE_MAX, 30));
#ifdef _WIN32
    auto maybeColor = this->readDWMColorizationColor();
    if (maybeColor.has_value()) {
        this->m_activeColor = *maybeColor;
    }
#ifdef _WIN32
    auto maybeWatcher = QRegistryWatcher::create(
        HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", this);
    if (maybeWatcher.has_value()) {
        this->m_watcher = *maybeWatcher;
        connect(
            this->m_watcher,
            &QRegistryWatcher::valueChanged,
            this,
            [this]() {
                auto maybeColor = this->readDWMColorizationColor();
                if (maybeColor.has_value() && !this->m_activeColorOverridden) {
                    this->m_activeColor = *maybeColor;
                    this->update();
                }
            },
            Qt::QueuedConnection);
    }
#endif
#endif

    this->m_horizontalLayout = new QHBoxLayout(this);
    this->m_horizontalLayout->setSpacing(0);
    this->m_horizontalLayout->setObjectName("HorizontalLayout");
    this->m_horizontalLayout->setContentsMargins(0, 0, 0, 0);

    this->m_leftMargin = new QWidget(this);
    this->m_leftMargin->setObjectName("LeftMargin");
    this->m_leftMargin->setMinimumSize(QSize(5, 0));
    this->m_leftMargin->setMaximumSize(QSize(5, QWIDGETSIZE_MAX));
    this->m_horizontalLayout->addWidget(this->m_leftMargin);

    this->m_buttonCaptionIcon =
        new TitleBarButton(TitleBarButton::CaptionIcon, this);
    this->m_buttonCaptionIcon->setObjectName("ButtonCaptionIcon");
    this->m_buttonCaptionIcon->setMinimumSize(QSize(30, 30));
    this->m_buttonCaptionIcon->setMaximumSize(QSize(30, 30));
    this->m_buttonCaptionIcon->setFocusPolicy(Qt::NoFocus);
#ifdef _WIN32
    int icon_size = ::GetSystemMetrics(SM_CXSMICON);
#else
    int icon_size = 16;
#endif
    this->m_buttonCaptionIcon->setIconSize(QSize(icon_size, icon_size));
    const auto icon = [&captionIcon, this]() -> QIcon {
        if (!captionIcon.isNull()) {
            return captionIcon;
        }
        auto globalWindowIcon = this->window()->windowIcon();
        if (!globalWindowIcon.isNull()) {
            return globalWindowIcon;
        }
        globalWindowIcon = QApplication::windowIcon();
        if (!globalWindowIcon.isNull()) {
            return globalWindowIcon;
        }
#ifdef _WIN32
        // Use system default application icon which doesn't need margin
        this->m_horizontalLayout->takeAt(
            this->m_horizontalLayout->indexOf(this->m_leftMargin));
        this->m_leftMargin->setParent(nullptr);
        HICON winIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
        globalWindowIcon.addPixmap(
            QtWinBackports::qt_pixmapFromWinHICON(winIcon));
#else
#if !defined(__APPLE__)
        if (globalWindowIcon.isNull()) {
            globalWindowIcon = QIcon::fromTheme("application-x-executable");
        }
#endif
#endif
        return globalWindowIcon;
    }();
    this->m_buttonCaptionIcon->setIcon(icon);
    this->m_horizontalLayout->addWidget(this->m_buttonCaptionIcon);

    auto *mainWindow = qobject_cast<QMainWindow *>(this->window());
    if (mainWindow != nullptr) {
        this->m_menuBar = mainWindow->menuBar();
        this->m_horizontalLayout->addWidget(this->m_menuBar);
        this->m_menuBar->setFixedHeight(30);
    }

    auto *emptySpace = new QWidget(this);
    emptySpace->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->m_horizontalLayout->addWidget(emptySpace, 1);

    int captionButtonsWidth = 0;
    switch (this->m_captionButtonStyle) {
    case CaptionButtonStyle::custom: {
        captionButtonsWidth = 30;
        break;
    }
    case CaptionButtonStyle::win: {
        captionButtonsWidth = 46;
        break;
    }
    case CaptionButtonStyle::mac: {
        captionButtonsWidth = 26;
        break;
    }
    }

    this->m_buttonMinimize =
        new TitleBarButton(TitleBarButton::Minimize, this);
    this->m_buttonMinimize->setObjectName("ButtonMinimize");
    this->m_buttonMinimize->setMinimumSize(QSize(captionButtonsWidth, 30));
    this->m_buttonMinimize->setMaximumSize(QSize(captionButtonsWidth, 30));
    this->m_buttonMinimize->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMinimize->setIconSize(
        this->m_captionButtonStyle == CaptionButtonStyle::mac ? QSize(16, 16)
                                                              : QSize(12, 12));
    this->m_horizontalLayout->addWidget(this->m_buttonMinimize);
    connect(this->m_buttonMinimize, &QPushButton::clicked, this, [this]() {
        emit this->minimizeClicked();
    });

    this->m_buttonMaximizeRestore =
        new TitleBarButton(TitleBarButton::MaximizeRestore, this);
    this->m_buttonMaximizeRestore->setObjectName("ButtonMaximizeRestore");
    this->m_buttonMaximizeRestore->setMinimumSize(
        QSize(captionButtonsWidth, 30));
    this->m_buttonMaximizeRestore->setMaximumSize(
        QSize(captionButtonsWidth, 30));
    this->m_buttonMaximizeRestore->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMaximizeRestore->setIconSize(
        this->m_captionButtonStyle == CaptionButtonStyle::mac ? QSize(16, 16)
                                                              : QSize(12, 12));
    this->m_horizontalLayout->addWidget(this->m_buttonMaximizeRestore);
    connect(this->m_buttonMaximizeRestore,
            &QPushButton::clicked,
            this,
            [this]() { emit this->maximizeRestoreClicked(); });

    this->m_buttonClose = new TitleBarButton(TitleBarButton::Close, this);
    this->m_buttonClose->setObjectName("ButtonClose");
    this->m_buttonClose->setMinimumSize(QSize(captionButtonsWidth, 30));
    this->m_buttonClose->setMaximumSize(QSize(captionButtonsWidth, 30));
    this->m_buttonClose->setFocusPolicy(Qt::NoFocus);
    this->m_buttonClose->setIconSize(
        this->m_captionButtonStyle == CaptionButtonStyle::mac ? QSize(16, 16)
                                                              : QSize(12, 12));
    this->m_horizontalLayout->addWidget(this->m_buttonClose);
    connect(this->m_buttonClose, &QPushButton::clicked, this, [this]() {
        emit this->closeClicked();
    });

    this->setAutoFillBackground(true);
    this->setActive(this->window()->isActiveWindow());
    this->setMaximized(static_cast<bool>(this->window()->windowState() &
                                         Qt::WindowMaximized));
}

#ifdef _WIN32
std::optional<QColor> TitleBar::readDWMColorizationColor() {
    auto handleKey = ::HKEY();
    auto regOpenResult = ::RegOpenKeyExW(HKEY_CURRENT_USER,
                                         L"SOFTWARE\\Microsoft\\Windows\\DWM",
                                         0,
                                         KEY_READ,
                                         &handleKey);
    if (regOpenResult != ERROR_SUCCESS) {
        return std::nullopt;
    }
    auto value = ::DWORD();
    auto dwordBufferSize = ::DWORD(sizeof(::DWORD));
    auto regQueryResult = ::RegQueryValueExW(handleKey,
                                             L"ColorizationColor",
                                             nullptr,
                                             nullptr,
                                             reinterpret_cast<LPBYTE>(&value),
                                             &dwordBufferSize);
    if (regQueryResult != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return QColor(static_cast<QRgb>(value));
}
#endif

TitleBar::~TitleBar() {
    auto *mainWindow = qobject_cast<QMainWindow *>(this->window());
    if (mainWindow != nullptr) {
        mainWindow->setMenuBar(this->m_menuBar);
    }
    this->m_menuBar = nullptr;
}

#if !defined(_WIN32) && !defined(__APPLE__)
void TitleBar::mousePressEvent(QMouseEvent *event) {
    if (!QX11Info::isPlatformX11() || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    QWidget *tlw = titleBarTopLevelWidget(this);

    if (tlw->isWindow() && tlw->windowHandle() &&
        !(tlw->windowFlags() & Qt::X11BypassWindowManagerHint) &&
        !tlw->testAttribute(Qt::WA_DontShowOnScreen) &&
        !tlw->hasHeightForWidth()) {
        QPlatformWindow *platformWindow = tlw->windowHandle()->handle();
        const QPoint globalPos = QHighDpi::toNativePixels(
            platformWindow->mapToGlobal(this->mapTo(tlw, event->pos())),
            platformWindow->screen()->screen());

        const xcb_atom_t moveResizeAtom = []() -> xcb_atom_t {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
                QX11Info::connection(),
                false,
                static_cast<std::uint16_t>(std::strlen(_NET_WM_MOVERESIZE)),
                _NET_WM_MOVERESIZE);
            xcb_intern_atom_reply_t *reply =
                xcb_intern_atom_reply(QX11Info::connection(), cookie, nullptr);
            const xcb_atom_t moveResizeAtomCopy = reply->atom;
            free(reply);
            return moveResizeAtomCopy;
        }();

        xcb_client_message_event_t xev;
        xev.response_type = XCB_CLIENT_MESSAGE;
        xev.type = moveResizeAtom;
        xev.sequence = 0;
        xev.window = static_cast<xcb_window_t>(platformWindow->winId());
        xev.format = 32;
        xev.data.data32[0] = static_cast<std::uint32_t>(globalPos.x());
        xev.data.data32[1] = static_cast<std::uint32_t>(globalPos.y());
        xev.data.data32[2] = 8; // move
        xev.data.data32[3] = XCB_BUTTON_INDEX_1;
        xev.data.data32[4] = 0;

        const xcb_window_t rootWindow = [platformWindow]() -> xcb_window_t {
            xcb_query_tree_cookie_t queryTreeCookie = xcb_query_tree(
                QX11Info::connection(),
                static_cast<xcb_window_t>(platformWindow->winId()));
            xcb_query_tree_reply_t *queryTreeReply = xcb_query_tree_reply(
                QX11Info::connection(), queryTreeCookie, nullptr);
            xcb_window_t rootWindowCopy = queryTreeReply->root;
            free(queryTreeReply);
            return rootWindowCopy;
        }();

        std::uint32_t eventFlags = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                                   XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

        xcb_ungrab_pointer(QX11Info::connection(), XCB_CURRENT_TIME);
        xcb_send_event(QX11Info::connection(),
                       false,
                       rootWindow,
                       eventFlags,
                       reinterpret_cast<const char *>(&xev));
    }
}
#endif

void TitleBar::paintEvent([[maybe_unused]] QPaintEvent *event) {
    auto styleOption = QStyleOption();
    styleOption.init(this);
    auto painter = QPainter(this);
    this->style()->drawPrimitive(
        QStyle::PE_Widget, &styleOption, &painter, this);
}

bool TitleBar::isActive() const {
    return this->m_active;
}

void TitleBar::setActive(bool active) {
    this->m_active = active;
    if (active) {
        auto palette = this->palette();
        palette.setColor(QPalette::Window, this->m_activeColor);
        this->setPalette(palette);
    } else {
        auto palette = this->palette();
        palette.setColor(QPalette::Window, this->m_inactiveColor);
        this->setPalette(palette);
    }

    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

bool TitleBar::isMaximized() const {
    return this->m_maximized;
}

void TitleBar::setMaximized(bool maximized) {
    this->m_maximized = maximized;
    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

void TitleBar::setMinimizable(bool on) {
    this->m_buttonMinimize->setVisible(on);
}

void TitleBar::setMaximizable(bool on) {
    this->m_buttonMaximizeRestore->setVisible(on);
}

QColor TitleBar::activeColor() {
    return this->m_activeColor;
}

void TitleBar::setActiveColor(const QColor &inactiveColor) {
#ifdef _WIN32
    this->m_activeColorOverridden = true;
#endif
    this->m_activeColor = inactiveColor;
    this->update();
}

QColor TitleBar::inactiveColor() {
    return this->m_inactiveColor;
}

void TitleBar::setInactiveColor(const QColor &inactiveColor) {
    this->m_activeColor = inactiveColor;
    this->update();
}

QColor TitleBar::hoverColor() const {
    return this->m_hoverColor;
}

void TitleBar::setHoverColor(QColor hoverColor) {
    this->m_hoverColor = std::move(hoverColor);
    this->m_buttonMinimize->setHoverColor(this->m_hoverColor);
    this->m_buttonMaximizeRestore->setHoverColor(this->m_hoverColor);
}

CaptionButtonStyle TitleBar::captionButtonStyle() const {
    return this->m_captionButtonStyle;
}

void TitleBar::setCaptionButtonStyle(CaptionButtonStyle captionButtonStyle) {
    this->m_captionButtonStyle = captionButtonStyle;

    auto iconSize = this->m_captionButtonStyle == CaptionButtonStyle::mac
                        ? QSize(16, 16)
                        : QSize(12, 12);
    int requiredWidth = 0;
    switch (this->m_captionButtonStyle) {
    case CaptionButtonStyle::custom: {
        requiredWidth = 30;
        break;
    }
    case CaptionButtonStyle::win: {
        requiredWidth = 46;
        break;
    }
    case CaptionButtonStyle::mac: {
        requiredWidth = 26;
        break;
    }
    }
    this->m_buttonMinimize->setIconSize(iconSize);
    this->m_buttonMinimize->setMinimumWidth(requiredWidth);
    this->m_buttonMinimize->setMaximumWidth(requiredWidth);
    this->m_buttonMaximizeRestore->setIconSize(iconSize);
    this->m_buttonMaximizeRestore->setMinimumWidth(requiredWidth);
    this->m_buttonMaximizeRestore->setMaximumWidth(requiredWidth);
    this->m_buttonClose->setIconSize(iconSize);
    this->m_buttonClose->setMinimumWidth(requiredWidth);
    this->m_buttonClose->setMaximumWidth(requiredWidth);

    auto iconsPaths =
        Internal::captionIconPathsForState(this->m_active,
                                           this->m_maximized,
                                           false,
                                           false,
                                           this->m_captionButtonStyle);

    this->m_buttonMinimize->setIcon(QIcon(iconsPaths[0].toString()));
    this->m_buttonMaximizeRestore->setIcon(QIcon(iconsPaths[1].toString()));
    this->m_buttonClose->setIcon(QIcon(iconsPaths[2].toString()));
}

void TitleBar::onWindowStateChange(Qt::WindowStates state) {
    this->setActive(this->window()->isActiveWindow());
    this->setMaximized(static_cast<bool>(state & Qt::WindowMaximized));
}

bool TitleBar::hovered() const {
    auto cursorPos = QCursor::pos();
    bool hovered = this->rect().contains(this->mapFromGlobal(cursorPos));
    if (!hovered) {
        return false;
    }

    if (this->m_menuBar->rect().contains(
            this->m_menuBar->mapFromGlobal(cursorPos))) {
        return false;
    }

    for (const TitleBarButton *btn : this->findChildren<TitleBarButton *>()) {
        bool btnHovered = btn->rect().contains(btn->mapFromGlobal(cursorPos));
        if (btnHovered) {
            return false;
        }
    }
    return true;
}

bool TitleBar::isCaptionButtonHovered() const {
    return this->m_buttonMinimize->underMouse() ||
           this->m_buttonMaximizeRestore->underMouse() ||
           this->m_buttonClose->underMouse();
}

void TitleBar::triggerCaptionRepaint() {
    this->m_buttonMinimize->update();
    this->m_buttonMaximizeRestore->update();
    this->m_buttonClose->update();
}

namespace Internal {

std::array<QStringView, 3> captionIconPathsForState(bool active,
                                                    bool maximized,
                                                    bool hovered,
                                                    bool pressed,
                                                    CaptionButtonStyle style) {
    std::array<QStringView, 3> buf;

    switch (style) {
    case CaptionButtonStyle::custom: {
        if (active || hovered) {
            buf[0] = u":/resources/titlebar/custom/chrome-minimize-dark.svg";
            if (maximized) {
                buf[1] =
                    u":/resources/titlebar/custom/chrome-restore-dark.svg";
            } else {
                buf[1] =
                    u":/resources/titlebar/custom/chrome-maximize-dark.svg";
            }
            if (hovered) {
                buf[2] = u":/resources/titlebar/custom/chrome-close-light.svg";
            } else {
                buf[2] = u":/resources/titlebar/custom/chrome-close-dark.svg";
            }
        } else {
            buf[0] = u":/resources/titlebar/custom/"
                     u"chrome-minimize-dark-disabled.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/custom/"
                         u"chrome-restore-dark-disabled.svg";
            } else {
                buf[1] = u":/resources/titlebar/custom/"
                         u"chrome-maximize-dark-disabled.svg";
            }
            buf[2] =
                u":/resources/titlebar/custom/chrome-close-dark-disabled.svg";
        }
        break;
    }
    case CaptionButtonStyle::win: {
        if (active || hovered) {
            buf[0] = u":/resources/titlebar/win/chrome-minimize-dark.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/win/chrome-restore-dark.svg";
            } else {
                buf[1] = u":/resources/titlebar/win/chrome-maximize-dark.svg";
            }
            if (hovered) {
                buf[2] = u":/resources/titlebar/win/chrome-close-light.svg";
            } else {
                buf[2] = u":/resources/titlebar/win/chrome-close-dark.svg";
            }
        } else {
            buf[0] =
                u":/resources/titlebar/win/chrome-minimize-dark-disabled.svg";
            if (maximized) {
                buf[1] = u":/resources/titlebar/win/"
                         u"chrome-restore-dark-disabled.svg";
            } else {
                buf[1] = u":/resources/titlebar/win/"
                         u"chrome-maximize-dark-disabled.svg";
            }
            buf[2] =
                u":/resources/titlebar/win/chrome-close-dark-disabled.svg";
        }
        break;
    }
    case CaptionButtonStyle::mac: {
        if (pressed) {
            buf[0] = u":/resources/titlebar/mac/minimize-pressed.png";
            if (maximized) {
                buf[1] = u":/resources/titlebar/mac/"
                         "maximize-restore-maximized-pressed.png";
            } else {
                buf[1] = u":/resources/titlebar/mac/"
                         u"maximize-restore-normal-pressed.png";
            }
            buf[2] = u":/resources/titlebar/mac/close-pressed.png";
        } else {
            if (hovered) {
                buf[0] = u":/resources/titlebar/mac/minimize-hovered.png";
                if (maximized) {
                    buf[1] = u":/resources/titlebar/mac/"
                             u"maximize-restore-maximized-hovered.png";
                } else {
                    buf[1] = u":/resources/titlebar/mac/"
                             u"maximize-restore-normal-hovered.png";
                }
                buf[2] = u":/resources/titlebar/mac/close-hovered.png";
            } else {
                if (active) {
                    buf[0] = u":/resources/titlebar/mac/minimize.png";
                    buf[1] = u":/resources/titlebar/mac/maximize-restore.png";
                    buf[2] = u":/resources/titlebar/mac/close.png";
                } else {
                    buf[0] = u":/resources/titlebar/mac/inactive.png";
                    buf[1] = u":/resources/titlebar/mac/inactive.png";
                    buf[2] = u":/resources/titlebar/mac/inactive.png";
                }
            }
        }
        break;
    }
    }

    return buf;
}

} // namespace Internal

} // namespace CSD
