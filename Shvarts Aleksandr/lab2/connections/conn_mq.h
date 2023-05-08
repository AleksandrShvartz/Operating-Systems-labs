#ifndef __CONN_MQ_H_
#define __CONN_MQ_H_

#include <string>
#include <mqueue.h>

#include "connection.h"

class mq : public Connection
{
public:
    mq(pid_t clientPid, bool isCreator);
    void Open(size_t hostPid, bool isCreator) override;
    void Get(void* buf, size_t count) override;
    void Send(void* buf, size_t count) override;
    void Close(void) override;

    ~mq(void);

private:
    static const int BULK_SIZE = 1024;
    std::string m_name;
    mqd_t m_queue;
};

#endif //!__CONN_MQ_H_
