#pragma once

#include "csdmetatype.h"
#include "csdtitlebar.h"

#include <QMainWindow>

#include <type_traits>

#ifdef __APPLE__

#include <objc/message.h>
#include <objc/runtime.h>

template <typename T,
          typename std::enable_if_t<std::is_pointer_v<T>, T> * = nullptr>
id toID(T t) {
    return reinterpret_cast<id>(t);
}

#endif

namespace CSD {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr,
               Qt::WindowFlags flags = Qt::WindowFlags());

#ifndef __APPLE__
protected:
    void changeEvent(QEvent *event) override;
#ifdef _WIN32
    bool nativeEvent(const QByteArray &eventType,
                     void *message,
                     long *result) override;
    void showEvent(QShowEvent *event) override;
#endif
#endif

#ifndef __APPLE__
private:
    TitleBar *m_titleBar;
#endif
};

} // namespace CSD
