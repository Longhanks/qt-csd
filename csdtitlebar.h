#pragma once

#include "captionbuttonstyle.h"

#include <QColor>
#include <QIcon>
#include <QStringView>
#include <QWidget>

#include <array>
#include <optional>

class QHBoxLayout;
class QLayout;
class QLabel;
class QMenuBar;

#ifdef _WIN32
class QRegistryWatcher;
#endif

namespace CSD {

class TitleBarButton;

class TitleBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(bool maximized READ isMaximized WRITE setMaximized)

private:
#ifdef _WIN32
    bool m_activeColorOverridden = false;
    QRegistryWatcher *m_watcher = nullptr;
    std::optional<QColor> readDWMColorizationColor();
#endif
    bool m_active = false;
    bool m_maximized = false;
    QColor m_activeColor = Qt::black;
    QColor m_inactiveColor = Qt::white;
    QColor m_hoverColor = Qt::gray;
    QHBoxLayout *m_horizontalLayout;
    QMenuBar *m_menuBar;
    QWidget *m_leftMargin;
    CaptionButtonStyle m_captionButtonStyle;
    TitleBarButton *m_buttonCaptionIcon;
    TitleBarButton *m_buttonMinimize;
    TitleBarButton *m_buttonMaximizeRestore;
    TitleBarButton *m_buttonClose;

protected:
#if !defined(_WIN32) && !defined(__APPLE__)
    void mousePressEvent(QMouseEvent *event) override;
#endif
    void paintEvent(QPaintEvent *event) override;

public:
    explicit TitleBar(CaptionButtonStyle captionButtonStyle,
                      const QIcon &captionIcon = QIcon(),
                      QWidget *parent = nullptr);
    ~TitleBar() override;

    bool isActive() const;
    void setActive(bool active);
    bool isMaximized() const;
    void setMaximized(bool maximized);
    void setMinimizable(bool on);
    void setMaximizable(bool on);
    QColor activeColor();
    void setActiveColor(const QColor &inactiveColor);
    QColor inactiveColor();
    void setInactiveColor(const QColor &inactiveColor);
    QColor hoverColor() const;
    void setHoverColor(QColor hoverColor);
    CaptionButtonStyle captionButtonStyle() const;
    void setCaptionButtonStyle(CaptionButtonStyle captionButtonStyle);
    void onWindowStateChange(Qt::WindowStates state);
    bool hovered() const;

    bool isCaptionButtonHovered() const;
    void triggerCaptionRepaint();

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();
};

namespace Internal {

std::array<QStringView, 3> captionIconPathsForState(bool active,
                                                    bool maximized,
                                                    bool hovered,
                                                    bool pressed,
                                                    CaptionButtonStyle style);

}

} // namespace CSD
