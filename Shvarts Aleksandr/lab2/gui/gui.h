#ifndef __GUI_H
#define __GUI_H
#include <atomic>
#include <chrono>
#include <ctime>
#include <vector>
#include "../safe_queue.h"
enum
{
    STRING_MAX_SIZE = 300
};

struct Message
{
    char m_message[STRING_MAX_SIZE];
    int m_sender_pid;
    int m_recipient_pid;
};

using send_callback = void(*)(Message m);
using get_callback = bool(*)(Message *m);
using is_running_callback = bool(*)(void);

class GUI
{
  private:
    std::atomic<bool> m_connected;
    class MainWindow;

    send_callback m_send;
    get_callback m_get;
    is_running_callback m_is_running;
    std::string m_name;

    GUI() = delete;
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
  public:
    // constructor
    GUI(std::string Name, send_callback SendFunc, get_callback GetFunc, is_running_callback IsRunning) :
      m_send(SendFunc), m_get(GetFunc), m_is_running(IsRunning), m_name(Name) {}

    void SetConnected(bool isConnected) { m_isConnected = isConnected; }

    // Run main loop function
    int Run(void);

    // destructor
    ~GUI();
};

#endif //__GUI_H
