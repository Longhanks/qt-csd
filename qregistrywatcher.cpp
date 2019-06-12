#include "qregistrywatcher.h"

#include <string_view>

static constexpr std::wstring_view kQRegistryWatcherEventName =
    L"q_registry_watcher_event";

std::optional<QRegistryWatcher *>
QRegistryWatcher::create(HKEY hive, std::wstring_view path, QObject *parent) {
    auto watcher = new QRegistryWatcher(hive, path, parent);
    if (watcher->m_didFail) {
        return std::nullopt;
    }
    return watcher;
}

QRegistryWatcher::QRegistryWatcher(HKEY hive,
                                   std::wstring_view path,
                                   QObject *parent)
    : QObject(parent) {
    auto openPathForNotifyResult = ::RegOpenKeyExW(
        hive, path.data(), 0, KEY_NOTIFY, &this->m_registryKey);
    if (openPathForNotifyResult != ERROR_SUCCESS) {
        this->m_registryKey = nullptr;
        this->m_didFail = true;
    } else {
        bool didStartWatching = this->restartWatching();
        if (!didStartWatching) {
            this->m_didFail = true;
        }
    }
}

QRegistryWatcher::~QRegistryWatcher() {
    this->stopWatching();
    if (this->m_registryKey != nullptr) {
        ::RegCloseKey(this->m_registryKey);
        this->m_registryKey = nullptr;
    }
}

bool QRegistryWatcher::restartWatching() {
    if (this->m_registryKey == nullptr || this->m_waitEvent != nullptr) {
        return false;
    }
    bool result = false;
    this->m_waitEvent = ::CreateEventW(nullptr,
                                       FALSE, // auto-reset event
                                       FALSE, // nonsignalled
                                       kQRegistryWatcherEventName.data());
    if (this->m_waitEvent != nullptr) {
        LONG notifyResult = ::RegNotifyChangeKeyValue(
            this->m_registryKey,
            FALSE,                      // Don't watch subtree
            REG_NOTIFY_CHANGE_LAST_SET, // Notify value changes
            this->m_waitEvent,
            TRUE); // Asynchronous
        if (notifyResult == ERROR_SUCCESS) {
            if (::RegisterWaitForSingleObject(
                    &this->m_waitHandle,
                    this->m_waitEvent,
                    &QRegistryWatcher::onValueChanged,
                    static_cast<void *>(this),
                    INFINITE,
                    WT_EXECUTEDEFAULT)) {
                this->m_isStopping = false;
                result = true;
            }
        }
    }
    if (!result && this->m_waitEvent != nullptr) {
        ::CloseHandle(this->m_waitEvent);
        this->m_waitEvent = nullptr;
    }
    return result;
}

void QRegistryWatcher::stopWatching() {
    this->m_isStopping = true;
    if (this->m_waitHandle) {
        ::UnregisterWaitEx(this->m_waitHandle, INVALID_HANDLE_VALUE);
        this->m_waitHandle = nullptr;
    }
    if (this->m_waitEvent) {
        ::CloseHandle(this->m_waitEvent);
        this->m_waitEvent = nullptr;
    }
}

void CALLBACK QRegistryWatcher::onValueChanged(void *context,
                                               BOOLEAN didTimeout) {
    QRegistryWatcher *watcher = static_cast<QRegistryWatcher *>(context);
    if (watcher->m_isStopping) {
        return;
    }

    if (didTimeout == FALSE) {
        emit watcher->valueChanged();
    }

    ::CloseHandle(watcher->m_waitEvent);
    watcher->m_waitEvent = nullptr;
    watcher->restartWatching();
}
