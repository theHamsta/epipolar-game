#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <memory>
#include <vector>

#include "GameState.hpp"
#include "python_include.hpp"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    MainWindow(MainWindow const&)  = delete;
    MainWindow(MainWindow const&&) = delete;
    auto operator=(MainWindow const&) -> MainWindow = delete;
    auto operator=(MainWindow const &&) -> MainWindow = delete;

  public slots:
    void openDirectory(const QString& path);

  protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    // virtual void dragEnterEvent(QDragEnterEvent *event) override {};
    // virtual void dropEvent(QDropEvent *event) override;

  private:
    Ui::MainWindow* ui;
    std::shared_ptr< class GetSetHandler > m_getSetHandler;

    auto readSettings() -> void;
    auto updateGameLogic() -> void;

    GameState m_state;

    std::vector< pybind11::array_t< float > > m_volumes;
};

#endif // MAINWINDOW_HPP
