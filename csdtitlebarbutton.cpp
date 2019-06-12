#include "csdtitlebarbutton.h"

#include "csdtitlebar.h"

#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>
#include <Qt>
#include <qdrawutil.h>

#include <QDebug>

namespace CSD {

TitleBarButton::TitleBarButton(Role role, TitleBar *parent)
    : TitleBarButton(QIcon(), QString(), role, parent) {}

TitleBarButton::TitleBarButton(const QString &text,
                               Role role,
                               TitleBar *parent)
    : TitleBarButton(QIcon(), text, role, parent) {}

TitleBarButton::TitleBarButton(const QIcon &icon,
                               const QString &text,
                               Role role,
                               TitleBar *parent)
    : QWidget(parent), m_role(role), m_text(text), m_icon(icon) {
    this->setAttribute(Qt::WidgetAttribute::WA_Hover, true);
}

QString TitleBarButton::text() const {
    return this->m_text;
}

void TitleBarButton::setText(const QString &text) {
    this->m_text = text;
}

QIcon TitleBarButton::icon() const {
    return this->m_icon;
}

void TitleBarButton::setIcon(const QIcon &icon) {
    this->m_icon = icon;
}

bool TitleBarButton::isActive() const {
    return this->m_active;
}

void TitleBarButton::setActive(bool active) {
    this->m_active = active;
}

QSize TitleBarButton::iconSize() const {
    return this->m_iconSize;
}

void TitleBarButton::setIconSize(const QSize &size) {
    this->m_iconSize = size;
}

void TitleBarButton::paintEvent([[maybe_unused]] QPaintEvent *event) {
    auto stylePainter = QStylePainter(this);
    auto styleOptionButton = QStyleOptionButton();
    styleOptionButton.initFrom(this);
    styleOptionButton.features = QStyleOptionButton::None;
    styleOptionButton.text = this->m_text;
    styleOptionButton.icon = this->m_icon;
    styleOptionButton.iconSize = this->m_iconSize;
    if (this->m_active) {
        styleOptionButton.palette.setColor(QPalette::ButtonText, Qt::white);
    } else {
        styleOptionButton.palette.setColor(QPalette::ButtonText, Qt::gray);
    }

    bool hovered = styleOptionButton.state & QStyle::State_MouseOver;
    if (hovered && !(this->m_role == Role::CaptionIcon)) {
        QBrush brush = styleOptionButton.palette.brush(QPalette::Button);
        if (this->m_role == Role::Close) {
            brush.setColor(Qt::red);
            if (!this->m_active) {
                styleOptionButton.palette.setColor(QPalette::ButtonText,
                                                   Qt::white);
            }
        } else if (this->isActive()) {
            QColor brushColor = Qt::lightGray;
            brushColor.setAlpha(50);
            brush.setColor(brushColor);
        } else {
            styleOptionButton.palette.setColor(QPalette::ButtonText,
                                               Qt::black);
        }
        qDrawShadePanel(&stylePainter,
                        styleOptionButton.rect,
                        styleOptionButton.palette,
                        false,
                        0,
                        &brush);
    }
    stylePainter.drawControl(QStyle::CE_PushButtonLabel, styleOptionButton);
}

} // namespace CSD
