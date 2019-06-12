#include "csdtitlebar.h"

#include "csdtitlebarbutton.h"

#ifdef _WIN32
#include "qregistrywatcher.h"

#include <Windows.h>
#include <dwmapi.h>
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyleOption>

#ifdef _WIN32
#include <QtWinExtras/QtWin>
#endif

namespace CSD {

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
    this->setObjectName("TitleBar");
    this->setMinimumSize(QSize(0, 30));
    this->setMaximumSize(QSize(QWIDGETSIZE_MAX, 30));

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
                if (maybeColor.has_value()) {
                    this->m_activeColor = *maybeColor;
                    this->update();
                }
            },
            Qt::QueuedConnection);
    }
#endif

    this->m_activeColor = [this]() -> QColor {
#ifdef _WIN32
        auto maybeColor = this->readDWMColorizationColor();
        return maybeColor.value_or(Qt::black);
#else
        Q_UNUSED(this)
        return Qt::black;
#endif
    }();

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
    auto icon = [this]() -> QIcon {
        auto globalWindowIcon = QApplication::windowIcon();
#ifdef _WIN32
        if (globalWindowIcon.isNull()) {
            this->m_horizontalLayout->takeAt(
                this->m_horizontalLayout->indexOf(this->m_leftMargin));
            this->m_leftMargin->setParent(nullptr);
            HICON winIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
            globalWindowIcon.addPixmap(QtWin::fromHICON(winIcon));
        }
#else
        Q_UNUSED(this)
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

    this->m_marginTitleLeft = new QWidget(this);
    this->m_marginTitleLeft->setObjectName("MarginTitleLeft");
    this->m_horizontalLayout->addWidget(this->m_marginTitleLeft);

    this->m_labelTitle = new QLabel(this);
    this->m_labelTitle->setObjectName("LabelTitle");
    this->m_labelTitle->setAlignment(Qt::AlignCenter);
#ifdef _WIN32
    this->m_labelTitle->setFont(QFont("Segoe UI", 9));
#endif
    this->m_horizontalLayout->addWidget(this->m_labelTitle);
    this->m_horizontalLayout->setStretchFactor(this->m_labelTitle, 1);

    this->m_marginTitleRight = new QWidget(this);
    this->m_marginTitleRight->setObjectName("MarginTitleRight");
    this->m_horizontalLayout->addWidget(this->m_marginTitleRight);

    this->m_buttonMinimize =
        new TitleBarButton(TitleBarButton::Minimize, this);
    this->m_buttonMinimize->setObjectName("ButtonMinimize");
    this->m_buttonMinimize->setMinimumSize(QSize(46, 30));
    this->m_buttonMinimize->setMaximumSize(QSize(46, 30));
    this->m_buttonMinimize->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMinimize->setText("―");
    this->m_horizontalLayout->addWidget(this->m_buttonMinimize);
    connect(this->m_buttonMinimize, &TitleBarButton::clicked, this, [this]() {
        emit this->minimize_clicked();
    });

    this->m_buttonMaximizeRestore =
        new TitleBarButton(TitleBarButton::MaximizeRestore, this);
    this->m_buttonMaximizeRestore->setObjectName("ButtonMaximizeRestore");
    this->m_buttonMaximizeRestore->setMinimumSize(QSize(46, 30));
    this->m_buttonMaximizeRestore->setMaximumSize(QSize(46, 30));
    this->m_buttonMaximizeRestore->setFocusPolicy(Qt::NoFocus);
    this->m_buttonMaximizeRestore->setText("☐");
    this->m_horizontalLayout->addWidget(this->m_buttonMaximizeRestore);
    connect(this->m_buttonMaximizeRestore,
            &TitleBarButton::clicked,
            this,
            [this]() { emit this->maxmize_restore_clicked(); });

    this->m_buttonClose = new TitleBarButton(TitleBarButton::Close, this);
    this->m_buttonClose->setObjectName("ButtonClose");
    this->m_buttonClose->setMinimumSize(QSize(46, 30));
    this->m_buttonClose->setMaximumSize(QSize(46, 30));
    this->m_buttonClose->setFocusPolicy(Qt::NoFocus);
    this->m_buttonClose->setText("✕");
    this->m_horizontalLayout->addWidget(this->m_buttonClose);
    connect(this->m_buttonClose, &TitleBarButton::clicked, this, [this]() {
        emit this->close_clicked();
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

void TitleBar::updateSpacers() {
    int width_left =
        this->m_leftMargin->width() + this->m_buttonCaptionIcon->width();
    int width_right = this->m_buttonClose->width();
    if (this->m_buttonMinimize->isVisible()) {
        width_right += this->m_buttonMinimize->width();
    }
    if (this->m_buttonMaximizeRestore->isVisible()) {
        width_right += this->m_buttonMaximizeRestore->width();
    }
    if (width_left > width_right) {
        this->m_marginTitleRight->setFixedSize(
            width_left - width_right, this->m_marginTitleRight->height());
    } else {
        this->m_marginTitleLeft->setFixedSize(
            width_right - width_left, this->m_marginTitleLeft->height());
    }
}

void TitleBar::paintEvent([[maybe_unused]] QPaintEvent *event) {
    this->updateSpacers();
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
        palette.setColor(QPalette::Background, this->m_activeColor);
        this->setPalette(palette);

        auto labelPalette = this->m_labelTitle->palette();
        labelPalette.setColor(QPalette::Foreground, Qt::white);
        this->m_labelTitle->setPalette(labelPalette);

        this->m_buttonCaptionIcon->setActive(true);
        this->m_buttonMinimize->setActive(true);
        this->m_buttonMaximizeRestore->setActive(true);
        this->m_buttonClose->setActive(true);
    } else {
        auto palette = this->palette();
        palette.setColor(QPalette::Background, Qt::white);
        this->setPalette(palette);

        auto labelPalette = this->m_labelTitle->palette();
        labelPalette.setColor(QPalette::Foreground, Qt::gray);
        this->m_labelTitle->setPalette(labelPalette);

        this->m_buttonCaptionIcon->setActive(false);
        this->m_buttonMinimize->setActive(false);
        this->m_buttonMaximizeRestore->setActive(false);
        this->m_buttonClose->setActive(false);
    }
}

bool TitleBar::isMaximized() const {
    return this->m_maximized;
}

void TitleBar::setMaximized(bool maximized) {
    this->m_maximized = maximized;
}

void TitleBar::setMinimizable(bool on) {
    this->m_buttonMinimize->setVisible(on);
    this->updateSpacers();
}

void TitleBar::setMaximizable(bool on) {
    this->m_buttonMaximizeRestore->setVisible(on);
    this->updateSpacers();
}

void TitleBar::setTitle(const QString &title) {
    this->m_labelTitle->setText(title);
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
    bool captionIconHovered = this->m_buttonCaptionIcon->rect().contains(
        this->m_buttonCaptionIcon->mapFromGlobal(cursorPos));
    bool minimizeHovered = this->m_buttonMinimize->rect().contains(
        this->m_buttonMinimize->mapFromGlobal(cursorPos));
    bool maximizeRestoreHovered =
        this->m_buttonMaximizeRestore->rect().contains(
            this->m_buttonMaximizeRestore->mapFromGlobal(cursorPos));
    bool closeHovered = this->m_buttonClose->rect().contains(
        this->m_buttonClose->mapFromGlobal(cursorPos));
    return !(captionIconHovered || minimizeHovered || maximizeRestoreHovered ||
             closeHovered);
}

} // namespace CSD
