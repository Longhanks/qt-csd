#include <QMainWindow>

class MainWindow : public QMainWindow {

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() noexcept override;
};
