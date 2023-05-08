#include <QApplication>
#include <unistd.h>
#include "string.h"
#include "gui.h"
#include "MainWindow.h"
#include "ui_mainwindow.h"

GUI::MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::MainWindow)
{
  m_ui->setupUi(this);
  m_timer = new QTimer(this);
  QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
  m_timer->start(10); //time specified in ms
}

void GUI::MainWindow::SetGUI(GUI *gui)
{
  m_gui = gui;
  std::string windowName =m_gui->m_name + "[" + std::to_string(getpid()) + "]";
  setWindowTitle(windowName.c_str());
}

void GUI::MainWindow::send()
{
  Message msg;
  std::string output_message;
  output_message += m_gui->m_name;
  output_message += ">> ";
  output_message += m_ui->inputWidget->text().toLocal8Bit().data();
  strncpy(msg.m_message, output_message.c_str(), std::max((int)output_message.size(), (int)STRING_MAX_SIZE));
  m_gui->m_send(msg);
  m_ui->inputWidget->clear();
  m_ui->chatWidget->addItem(output_message.c_str());
}

void GUI::MainWindow::tick()
{
  // check if we need to close
  if (!m_gui->m_is_running())
    this->close();

  // update status
  if (m_gui->m_isConnected)
    m_ui->statusbar->showMessage("Partner connected");
  else
    m_ui->statusbar->showMessage("Partner not connected");

  // update list
  Message msg = {0};
  while (m_gui->m_get(&msg))
    m_ui->chatWidget->addItem(msg.m_message);
}

GUI::MainWindow::~MainWindow()
{
  delete m_ui;
}

int GUI::Run() {
  int argc = 1;
  char* args[] = { (char*)m_name.c_str() };
  QApplication app(argc,args);
  MainWindow window;
  window.SetGUI(this);
  window.show();
  return app.exec();
}

GUI::~GUI()
{

}
