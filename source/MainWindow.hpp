#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <memory>
#include <random>
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
    auto newProjections() -> void;
    auto evaluate() -> void;
    inline auto inputP1() -> bool
    {
        return m_state.inputState == InputState::InputP1 || m_state.inputState == InputState::InputBoth;
    }
    inline auto inputP2() -> bool
    {
        return m_state.inputState == InputState::InputP2 || m_state.inputState == InputState::InputBoth;
    }

    GameState m_state;

    std::vector< pybind11::array_t< float > > m_volumes;
    pybind11::array_t< float > m_view1;
    pybind11::array_t< float > m_view2;
    std::mt19937 m_random;
};

#endif // MAINWINDOW_HPP
