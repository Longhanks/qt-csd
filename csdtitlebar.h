#pragma once

#include "csdmetatype.h"

#include <QColor>
#include <QWidget>

#include <optional>

class QHBoxLayout;
class QLayout;
class QLabel;
class QRegistryWatcher;

namespace CSD {

class TitleBarButton;

class TitleBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(bool maximized READ isMaximized WRITE setMaximized)

private:
#ifdef _WIN32
    QRegistryWatcher *m_watcher = nullptr;
    std::optional<QColor> readDWMColorizationColor();
#endif
    bool m_active = false;
    bool m_maximized = false;
    QColor m_activeColor;
    QHBoxLayout *m_horizontalLayout;
    QWidget *m_leftMargin;
    TitleBarButton *m_buttonCaptionIcon;
    QWidget *m_marginTitleLeft;
    QLabel *m_labelTitle;
    QWidget *m_marginTitleRight;
    TitleBarButton *m_buttonMinimize;
    TitleBarButton *m_buttonMaximizeRestore;
    TitleBarButton *m_buttonClose;
    void updateSpacers();

protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit TitleBar(QWidget *parent = nullptr);

    bool isActive() const;
    void setActive(bool active);
    bool isMaximized() const;
    void setMaximized(bool maximized);
    void setMinimizable(bool on);
    void setMaximizable(bool on);
    void setTitle(const QString &title);
    void onWindowStateChange(Qt::WindowStates state);
    bool hovered() const;

signals:
    void minimize_clicked();
    void maxmize_restore_clicked();
    void close_clicked();
};

} // namespace CSD
