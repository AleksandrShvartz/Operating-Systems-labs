#ifndef __CONNECTION_H_
#define __CONNECTION_H_

#include <sys/types.h>

class Connection {
public:
    static Connection * CreateConnection(pid_t clientPid, bool isHost);
    virtual void Open(size_t hostPid, bool isCreator) = 0;
    virtual void Get(void* buf, size_t count) = 0;
    virtual void Send(void* buf, size_t count) = 0;
    virtual void Close(void) = 0;

    virtual ~Connection(void) = default;
};

#endif //!__CONNECTION_H_