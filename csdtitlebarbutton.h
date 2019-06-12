#pragma once

#include <QIcon>
#include <QWidget>

namespace CSD {

class TitleBar;

class TitleBarButton : public QWidget {
    Q_OBJECT

public:
    enum Role { CaptionIcon, Minimize, MaximizeRestore, Close };
    Q_ENUM(Role)

    explicit TitleBarButton(Role role, TitleBar *parent = nullptr);
    explicit TitleBarButton(const QString &text,
                            Role role,
                            TitleBar *parent = nullptr);
    explicit TitleBarButton(const QIcon &icon,
                            const QString &text,
                            Role role,
                            TitleBar *parent = nullptr);

    [[nodiscard]] QString text() const;
    void setText(const QString &text);

    [[nodiscard]] QIcon icon() const;
    void setIcon(const QIcon &icon);

    [[nodiscard]] bool isActive() const;
    void setActive(bool active);

    [[nodiscard]] QSize iconSize() const;
    void setIconSize(const QSize &size);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Role m_role;
    QString m_text;
    QIcon m_icon;
    bool m_active = false;
    QSize m_iconSize;
};

} // namespace CSD
