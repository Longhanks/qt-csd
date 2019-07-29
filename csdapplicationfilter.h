#ifdef _WIN32
#include "win32csd.h"
#elif defined(__APPLE__)
#else
#include "linuxcsd.h"
#endif

#include <QObject>

#include <vector>

namespace CSD {

class TitleBar;

class ApplicationFilter : public QObject {
    Q_OBJECT

private:
    std::vector<QWidget *> m_filtered;
#ifdef _WIN32
    Internal::Win32ClientSideDecorationFilter m_filter;
#elif defined(__APPLE__)
#else
    Internal::LinuxClientSideDecorationFilter m_filter;
#endif

    void setUpTitleBarForWidget(CSD::TitleBar *titleBar, QWidget *widget);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public:
    explicit ApplicationFilter(QObject *parent = nullptr);
    ~ApplicationFilter() override;

    void addExclude(QWidget *widget);
};

} // namespace CSD
