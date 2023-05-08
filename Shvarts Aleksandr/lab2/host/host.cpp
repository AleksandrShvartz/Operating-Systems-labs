#include <sys/syslog.h>
#include <thread>
#include <algorithm>
#include <csignal>
#include <chrono>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

#include "host.h"

Host Host::m_hostInstance;

int main(int argc, char *argv[])
{
  openlog("lab2_host", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

  try
  {
    Host::GetInstance().Run();
  }
  catch (std::exception &err)
  {
    syslog(LOG_ERR, "An err %s", err.what());
	closelog();
	return 1;
  }

  closelog();
  return 0;
}

void Host::SignalHandler(int signum, siginfo_t *info, void *ptr)
{
  switch (signum)
  {
    case SIGUSR1:
    {
      syslog(LOG_INFO, "Client %d request connection to host", info->si_pid);
      if (Host::GetInstance().m_clientPid == -1)
        Host::GetInstance().m_clientPid = info->si_pid;
      else

        syslog(LOG_INFO, "But host has already client %d", Host::GetInstance().GetClientPid());
      break;
    }
    case SIGTERM:
    {
      Host::GetInstance().Stop();
      break;
    }
  }
}

Host::Host(void)
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

void Host::GUISend(Message msg)
{
  Host::GetInstance().m_outputMessages.Push(msg);
}

bool Host::GUIGet(Message *msg)
{
  return Host::GetInstance().m_inputMessages.GetAndRemove(msg);
}

void Host::Run(void)
{
  syslog(LOG_INFO, "Host started");
  m_isRunning = true;

  // run thread with connections
  std::thread connectionThread(&Host::ConnectionWork, this);

  m_gui = new GUI("Host", GUISend, GUIGet, IsRunning);

  m_gui->Run();

  delete m_gui;
  Stop();
  connectionThread.join();
}

void Host::Stop()
{
  if (m_isRunning.load())
  {
    m_isRunning = false;
    syslog(LOG_INFO, "Start terminating host");
  }
}

Host::~Host(void)
{
}

bool Host::ConnectionPrepare(Connection **con, sem_t **sem_read, sem_t **sem_write)
{
  syslog(LOG_INFO, "Start creating connection for client %d", GetClientPid());
  *con = Connection::CreateConnection(m_clientPid, true);
  if (!*con)
  {
    syslog(LOG_ERR, "Connection creation error");
    m_clientPid = -1;
    return false;
  }

  // open semaphores
  std::string semNameRead = "/host_" + std::to_string(m_clientPid);
  std::string semNameWrite = "/client_" + std::to_string(m_clientPid);
  *sem_read = sem_open(semNameRead.c_str(), O_CREAT | O_EXCL, 0777, 0);
  if (*sem_read == SEM_FAILED)
  {
    syslog(LOG_ERR, "Semaphore creation error");
    delete *con;
    m_clientPid = -1;
    return false;
  }
  syslog(LOG_INFO, "created semaphore %s", semNameRead.c_str());
  *sem_write = sem_open(semNameWrite.c_str(), O_CREAT | O_EXCL, 0777, 0);
  if (*sem_write == SEM_FAILED)
  {
    syslog(LOG_ERR, "Semaphore creation error");
    sem_close(*sem_read);
    delete *con;
    m_clientPid = -1;
    return false;
  }
  syslog(LOG_INFO, "created semaphore %s", semNameWrite.c_str());

  // send signal that we are ready
  if (kill(m_clientPid, SIGUSR1) != 0)
    syslog(LOG_ERR, "Cannot send signal to %d", GetClientPid());

  syslog(LOG_INFO, "Signal sent");
  try
  {
    (*con)->Open(0, false);
  }
  catch (const char *err)
  {
    syslog(LOG_ERR, "Connection open error: %s", err);
    delete *con;
    sem_close(*sem_write);
    sem_close(*sem_read);
    return false;
  }
  syslog(LOG_INFO, "Opened connection");
  return true;
}

bool Host::ConnectionGetMessages(Connection *con, sem_t *sem_read, sem_t *sem_write)
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

bool Host::ConnectionSendMessages(Connection *con, sem_t *sem_read, sem_t *sem_write)
{
  bool res = m_outputMessages.SendToConnection(con);
  sem_post(sem_write);
  return res;
}

void Host::ConnectionClose(Connection *con, sem_t *sem_read, sem_t *sem_write)
{
  con->Close();
  sem_close(sem_write);
  sem_close(sem_read);
  delete con;
}

void Host::ConnectionWork()
{
  try
  {
    printf("host pid = %i\n", getpid());
    auto lastTimeWeHadClient = std::chrono::high_resolution_clock::now();
    m_gui->SetConnected(false);
    while (m_isRunning.load())
    {
      // if this happening for 5 minutes -> exit
      if (m_clientPid.load() == -1)
      {
        auto minutesPassed = std::chrono::duration_cast<std::chrono::minutes>(
          std::chrono::high_resolution_clock::now() - lastTimeWeHadClient).count();

        if (minutesPassed >= 60)
        {
          // we should stop our work
          Stop();
        }
        continue;
      }
      lastTimeWeHadClient = std::chrono::high_resolution_clock::now();

      Connection *currentConnection;
      sem_t *semaphoreRead, *semaphoreWrite;
      if (!ConnectionPrepare(&currentConnection, &semaphoreRead, &semaphoreWrite))
        continue;

      auto clock = std::chrono::high_resolution_clock::now();
      m_gui->SetConnected(true);
      while (m_isRunning.load())
      {
        double minutes_passed =
          std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::high_resolution_clock::now() - clock).count();

        if (minutes_passed >= 1)
        {
          syslog(LOG_INFO, "Killing client for 1 minute silence");
          kill(m_clientPid, SIGTERM);
          m_clientPid = -1;
          break;
        }

        // Get all messages
        if (!ConnectionGetMessages(currentConnection, semaphoreRead, semaphoreWrite))
          break;

        // Send all messages
        if (!ConnectionSendMessages(currentConnection, semaphoreRead, semaphoreWrite))
          break;

        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // for client get semaphore
      }
      ConnectionClose(currentConnection, semaphoreRead, semaphoreWrite);
      m_gui->SetConnected(false);
    }

    if (m_clientPid != -1)
      kill(m_clientPid, SIGTERM);
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