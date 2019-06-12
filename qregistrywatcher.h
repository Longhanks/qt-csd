#pragma once

#include <Windows.h>

#include <QObject>

#include <optional>

class QRegistryWatcher final : public QObject {
    Q_OBJECT

public:
    static std::optional<QRegistryWatcher *>
    create(HKEY hive, std::wstring_view path, QObject *parent = nullptr);
    ~QRegistryWatcher() final;

signals:
    void valueChanged();

private:
    explicit QRegistryWatcher(HKEY hive,
                              std::wstring_view path,
                              QObject *parent);
    friend std::optional<QRegistryWatcher *>
    create(HKEY hive, std::wstring_view path, QObject *parent);

    bool restartWatching();
    void stopWatching();

    static void CALLBACK onValueChanged(void *context, BOOLEAN didTimeout);
    HKEY m_registryKey = nullptr;
    HANDLE m_waitEvent = nullptr;
    HANDLE m_waitHandle = nullptr;
    bool m_isStopping = false;
    bool m_didFail = false;
};
