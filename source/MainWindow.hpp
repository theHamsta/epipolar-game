#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <memory>

#include "GameState.hpp"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

  protected:
    virtual void closeEvent(QCloseEvent* event) override;
    // virtual void dragEnterEvent(QDragEnterEvent *event) override {};
    // virtual void dropEvent(QDropEvent *event) override;

  private:
    Ui::MainWindow* ui;
    std::shared_ptr< class GetSetHandler > m_getSetHandler;

    auto readSettings() -> void;
    auto updateGameLogic() -> void;

    GameState m_state;
};

#endif // MAINWINDOW_HPP
