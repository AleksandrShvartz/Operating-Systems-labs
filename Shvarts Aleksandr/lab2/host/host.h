#ifndef __HOST_H_
#define __HOST_H_

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

class Host {
private:
    // queue of input messages
    SafeQueue<Message> m_inputMessages;

    // queue of output messages
    SafeQueue<Message> m_outputMessages;

    // client pids
    SafeIntSet m_clientPids;
    // variable for singleton
    static Host m_hostInstance;
    // atomic bool for terminating
    std::atomic<bool> m_isRunning = true;
    GUI *m_gui = nullptr;

    // handler for signals
    static void SignalHandler(int signum, siginfo_t* info, void *ptr);

    static void GUISend(Message msg);
    static bool GUIGet(Message *msg);

    // Connection working function
    void ConnectionWork(void);
    bool ConnectionPrepare(Connection **con, sem_t **sem_read, sem_t **sem_write);
    bool ConnectionGetMessages(Connection *con, sem_t *sem_read, sem_t *sem_write);
    bool ConnectionSendMessages(Connection *con, sem_t *sem_read, sem_t *sem_write);
    void ConnectionClose(Connection *con, sem_t *sem_read, sem_t *sem_write);
    // Private constructor
    Host(void);
    // Blocked constructors
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;
public:

    static Host &GetInstance(void) { return m_hostInstance; }

    void Run(void);

    static bool IsRunning(void) { return GetInstance().m_isRunning.load(); }

    static int GetLastClientPids(void) { return GetInstance().m_clientPids.GetLast(); }

    void Stop(void);

    ~Host();
};

#endif //!__HOST_H_
