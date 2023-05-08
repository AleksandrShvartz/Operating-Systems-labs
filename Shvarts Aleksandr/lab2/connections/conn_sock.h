#ifndef __CONN_SEG_H_
#define __CONN_SEG_H_

#include <string>
#include <sys/socket.h>

#include "connection.h"

class sock : public Connection
{
public:
    sock(pid_t clientPid, bool isHost);
    void Open(size_t hostPid, bool isCreator) override;
    void Get(void* buf, size_t count) override;
    void Send(void* buf, size_t count) override;
    void Close(void) override;

    ~sock(void);

private:
    socklen_t m_host_socket, m_client_socket;
    std::string m_sock_name;
    bool m_isHost;
};

#endif //!__CONN_SEG_H_
