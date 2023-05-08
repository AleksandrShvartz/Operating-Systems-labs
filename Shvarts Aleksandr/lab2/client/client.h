#ifndef CLIENT_H
#define CLIENT_H
#include <sys/types.h>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <bits/types/siginfo_t.h>
#include "../gui/gui.h"
#include "../connections/connection.h"
#include "../safe_queue.h"

class Client {
private:
    // queue of input messages
    SafeQueue<Message> m_inputMessages;

    // queue of output messages
    SafeQueue<Message> m_outputMessages;

    // client pid
    std::atomic<pid_t> m_hostPid = -1;
    // variable for singleton
    static Client m_clientInstance;
    // atomic bool for terminating
    std::atomic<bool> m_isRunning = true;
    // atomic bool for init work
    std::atomic<bool> m_isHostReady = false;
    GUI *m_gui;

    // handler for signals
    static void SignalHandler(int signum, siginfo_t* info, void *ptr);
    // Connection working function
    void ConnectionWork(void);
    bool ConnectionPrepare(Connection **con, sem_t **sem_read, sem_t **sem_write);
    bool ConnectionGetMessages(Connection *con, sem_t *sem_read, sem_t *sem_write);
    bool ConnectionSendMessages(Connection *con, sem_t *sem_read, sem_t *sem_write);
    void ConnectionClose(Connection *con, sem_t *sem_read, sem_t *sem_write);

    static void GUISend(Message msg);
    static bool GUIGet(Message *msg);

    // Private constructor
    Client(void);
    // Blocked constructors
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
public:

    static Client &GetInstance(void) { return m_clientInstance; }

    static bool IsRunning(void) { return GetInstance().m_isRunning.load(); }

    void Run(pid_t host_id);

    void Stop(void);

    ~Client();
};

#endif //CLIENT_H
