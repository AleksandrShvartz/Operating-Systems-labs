#include <QMainWindow>
#include <QtGui>
#include <QPushButton>
#include "gui.h"

namespace Ui {
  class MainWindow;
}

class GUI;

class GUI::MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    explicit MainWindow(QWidget *parent = 0);

    void SetGUI(GUI *gui);

    ~MainWindow();

  private slots:
    void tick();
    void send();
  private:
    QTimer *m_timer;

    QPushButton *m_button;

    GUI *m_gui;

    Ui::MainWindow *m_ui;
};
