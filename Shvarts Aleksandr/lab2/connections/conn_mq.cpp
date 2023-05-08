
#include <fcntl.h>
#include <string.h>

#include "conn_mq.h"

Connection * Connection::CreateConnection(pid_t clientPid, bool isHost)
{
    mq *m = new mq(clientPid, isHost);

    return m;
}

mq::mq(pid_t clientPid, bool isCreator)
{
    m_name = "/" + std::to_string(clientPid);
}

void mq::Open(size_t hostPid, bool isCreator)
{
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BULK_SIZE;
    attr.mq_curmsgs = 0;

    if (isCreator)
        m_queue = mq_open(m_name.c_str(), O_CREAT | O_RDWR, 0644, &attr);
    else
        m_queue = mq_open(m_name.c_str(), O_RDWR);

    if ((mqd_t)-1 == m_queue)
        throw("error while opening mqueue");
}

void mq::Get(void* buf, size_t count)
{
    if (count > BULK_SIZE)
        throw("Too much");
    mq_receive(m_queue, (char *)buf, BULK_SIZE, nullptr);
}

void mq::Send(void* buf, size_t count)
{
    if (count > BULK_SIZE)
        throw("Too much");
    mq_send(m_queue, (const char *)buf, count, 0);
}

void mq::Close(void)
{
    mq_close(m_queue);
}

mq::~mq(void)
{
}

