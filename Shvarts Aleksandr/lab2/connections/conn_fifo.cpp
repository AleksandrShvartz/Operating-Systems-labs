#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "conn_fifo.h"

Connection * Connection::CreateConnection(pid_t clientPid, bool isHost)
{
    return new fifo(clientPid, isHost);
}

fifo::fifo(pid_t clientPid, bool isHst)
{
    m_isHost = isHst;
    m_fifoName = "/tmp/fifo_" + std::to_string(clientPid);
}

void fifo::Open(size_t hostPid, bool isCreator)
{
  if (m_isHost)
  {
    unlink(m_fifoName.c_str());
    if ( mkfifo(m_fifoName.c_str(), 0666) )
      throw ("fifo creation error");
  }

  m_file =::open(m_fifoName.c_str(), O_RDWR);
  if (m_file == -1)
    throw ("fifo open error");
}

void fifo::Get(void* buf, size_t count)
{
    if (read(m_file, buf, count) < 0)
    {
        throw ("fifo reading error");
    }
}

void fifo::Send(void* buf, size_t count)
{
    if (write(m_file, buf, count) < 0)
    {
        throw ("fifo writing error");
    }
}

void fifo::Close(void)
{
    close(m_file);
    if (m_isHost)
      unlink(m_fifoName.c_str());
}

fifo::~fifo(void)
{
}

