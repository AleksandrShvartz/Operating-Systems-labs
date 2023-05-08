#include <sys/syslog.h>
#include <thread>
#include <algorithm>
#include <csignal>
#include <chrono>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include "client.h"

Client Client::m_clientInstance;

int main(int argc, char *argv[])
{
  openlog("lab2_client", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

  if (argc != 2)
  {
      syslog(LOG_ERR, "expected host pid as the only command argument");
      closelog();
      return 1;
  }

  int pid;
  try
  {
      pid = std::stoi(argv[1]);
  }
  catch (std::exception &e)
  {
      syslog(LOG_ERR, "An error while pid reading %s", e.what());
      closelog();
      return 1;
  }

  try
  {
    Client::GetInstance().Run(pid);
  }
  catch (std::exception &e)
  {
    syslog(LOG_ERR, "An error %s", e.what());	
	closelog();
	return 1;
  }

  closelog();
  return 0;
}

void Client::GUISend(Message msg)
{
  Client::GetInstance().m_outputMessages.Push(msg);
}

bool Client::GUIGet(Message *msg)
{
  return Client::GetInstance().m_inputMessages.GetAndRemove(msg);
}

void Client::SignalHandler(int signum, siginfo_t *info, void *ptr)
{
  switch (signum)
  {
    case SIGUSR1:
    {
      syslog(LOG_INFO, "Got signal that host are ready");
      Client::GetInstance().m_isHostReady = true;
      break;
    }
    case SIGTERM:
    {
      Client::GetInstance().Stop();
      break;
    }
  }
}

Client::Client(void)
{
  // subscribe on signal actions
  struct sigaction sig{};
  memset(&sig, 0, sizeof(sig));
  sig.sa_sigaction = SignalHandler;
  sig.sa_flags = SA_SIGINFO;
  // for termination
  sigaction(SIGTERM, &sig, nullptr);
  // for connecting User
  sigaction(SIGUSR1, &sig, nullptr);
}

void Client::Run(pid_t hostPid)
{
  syslog(LOG_INFO, "Client started");

  m_hostPid = hostPid;
  m_isRunning = true;

  // send signal to Host because we need to make handshake
  if (kill(hostPid, SIGUSR1) != 0)
  {
    // if no such process -> just exit
    m_isRunning = false;
    syslog(LOG_ERR, "Host %d not present", hostPid);
    return;
  }
  syslog(LOG_ERR, "Signal sent");

  // run thread with connections
  std::thread connectionThread(&Client::ConnectionWork, this);

  m_gui = new GUI("Client", GUISend, GUIGet, IsRunning);
  m_gui->Run();
  delete m_gui;
  Stop();
  connectionThread.join();
}

void Client::Stop()
{
  if (m_isRunning)
  {
    m_isRunning = false;
    syslog(LOG_INFO, "Start terminating client");
  }
}

Client::~Client(void)
{
}

bool Client::ConnectionPrepare(Connection **con, sem_t **sem_read, sem_t **sem_write)
{
  syslog(LOG_INFO, "start creating connection for %d", int(m_hostPid));
  *con = Connection::CreateConnection(getpid(), false);
  if (!*con)
  {
    syslog(LOG_ERR, "Connection creation error");
    return false;
  }

  // open semaphores
  std::string semNameRead = "/client_" + std::to_string(getpid());
  std::string semNameWrite = "/host_" + std::to_string(getpid());
  *sem_read = sem_open(semNameRead.c_str(), 0);
  if (*sem_read == SEM_FAILED)
  {
    syslog(LOG_ERR, "Semaphore creation error");
    delete *con;
    return false;
  }
  syslog(LOG_INFO, "created semaphore %s", semNameRead.c_str());
  *sem_write = sem_open(semNameWrite.c_str(), 0);
  if (*sem_write == SEM_FAILED)
  {
    syslog(LOG_ERR, "Semaphore creation error");
    sem_close(*sem_read);
    delete *con;
    return false;
  }
  syslog(LOG_INFO, "created semaphore %s", semNameWrite.c_str());
  try
  {
    (*con)->Open(m_hostPid, false);
  }
  catch (const char *err)
  {
    syslog(LOG_ERR, "Connection open error: %s", err);
    delete *con;
    sem_close(*sem_write);
    sem_close(*sem_read);
    return false;
  }
  syslog(LOG_INFO, "opened connection");
  return true;
}

bool Client::ConnectionGetMessages(Connection *con, sem_t *sem_read, sem_t *sem_write)
{
  // wait semaphore
  {
    timespec t;

    clock_gettime(CLOCK_REALTIME, &t);

    t.tv_sec += 5;

    int s = sem_timedwait(sem_read, &t);
    if (s == -1)
    {
      syslog(LOG_ERR, "Read semaphore timeout");
      m_isRunning = false;
      return false;
    }
  }

  m_inputMessages.GetFromConnection(con);
  return true;
}

bool Client::ConnectionSendMessages(Connection *con, sem_t *sem_read, sem_t *sem_write)
{
  bool res = m_outputMessages.SendToConnection(con);
  sem_post(sem_write);
  return res;
}

void Client::ConnectionClose(Connection *con, sem_t *sem_read, sem_t *sem_write)
{
  con->Close();
  sem_close(sem_write);
  sem_close(sem_read);
  delete con;
}

void Client::ConnectionWork(void)
{
  try
  {
    auto clock = std::chrono::high_resolution_clock::now();

    m_gui->SetConnected(false);
    while (!m_isHostReady.load())
    {
      double second = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - clock).count();

      if (second >= 5)
      {
        m_isHostReady = false;
        m_hostPid = false;
        syslog(LOG_ERR, "Host cannot prepare for 5 seconds. Exiting");
        Stop();
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    Connection *currentConnection;
    sem_t *semaphoreWrite, *semaphoreRead;
    if (!ConnectionPrepare(&currentConnection, &semaphoreRead, &semaphoreWrite))
      return;
    m_gui->SetConnected(true);
    // while m_isRunning
    while (m_isRunning.load())
    {
      // Send all messages
      if (!ConnectionSendMessages(currentConnection, semaphoreRead, semaphoreWrite))
        break;

      std::this_thread::sleep_for(std::chrono::milliseconds(30));

      // Get all messages
      if (!ConnectionGetMessages(currentConnection, semaphoreRead, semaphoreWrite))
        break;
    }
    ConnectionClose(currentConnection, semaphoreRead, semaphoreWrite);
  }
  catch (std::exception &e)
  {
    syslog(LOG_ERR, "An error %s", e.what());
  }
  catch (const char *e)
  {
    syslog(LOG_ERR, "An error %s", e);
  }
}

