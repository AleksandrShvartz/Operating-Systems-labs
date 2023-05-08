#ifndef __SAFE_QUEUE_H_
#define __SAFE_QUEUE_H_
#include <sys/types.h>
#include <atomic>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include "connections/connection.h"
class SafeIntSet{
private:
    std::unordered_set<int> m_storage;
    int m_last;
    mutable std::mutex m_mutex;
public:
    bool IsEmpty(){
         m_mutex.lock();
        bool res = m_storage.empty();
        m_mutex.unlock();
        return res;
    }
    void Push(int val)
    {
        m_mutex.lock();
        m_storage.insert(val);
        m_last = val;
        m_mutex.unlock();
    }
    int GetLast()
    {
        return m_last;
    }
    void TerminateAll(){
          m_mutex.lock();
        for (auto it : m_storage) {
            kill(it, SIGTERM);;
          }
      m_storage.clear();
      m_mutex.unlock();
    }
    void Delete(int val)
    {
        m_mutex.lock();
        m_storage.erase(val);
        m_mutex.unlock();
    }

};

template <typename T>
class SafeQueue {
private:
    std::queue<T> m_storage;
    mutable std::mutex m_mutex;
public:
	void Push(const T &val)
	{
		m_mutex.lock();
		m_storage.push(val);
		m_mutex.unlock();
	}
	bool GetAndRemove(T *msg)
	{
		m_mutex.lock();
		if (m_storage.empty())
		{
			m_mutex.unlock();
			return false;
		}
		*msg = m_storage.front();
		m_storage.pop();
		m_mutex.unlock();
		return true;
	}
	
	bool GetFromConnection(Connection *con)
	{
		m_mutex.lock();
		uint32_t amount = 0;
		con->Get(&amount, sizeof(uint32_t));
		for (uint32_t i = 0; i < amount; i++)
		{
			T msg = {0};
			try
			{
				con->Get(&msg, sizeof(T));
			}
			catch (const char *err)
			{
				m_mutex.unlock();
				syslog(LOG_ERR, "%s", err);
				return false;
			}
			syslog(LOG_INFO, "Recieved some data\n");
			m_storage.push(msg);
		}
		m_mutex.unlock();
		return true;
	}
	
	bool SendToConnection(Connection *con)
	{
		m_mutex.lock();
		uint32_t amount = m_storage.size();
		if (amount > 0)
			syslog(LOG_INFO, "Start sending data\n");
		con->Send(&amount, sizeof(uint32_t));
		while (!m_storage.empty())
		{
			syslog(LOG_INFO, "Sended some data\n");
			try
			{
				con->Send(&m_storage.front(), sizeof(T));
			}
			catch (const char *err)
			{
				m_mutex.unlock();
				syslog(LOG_ERR, "%s", err);
				return false;
			}
			m_storage.pop();
		}
		m_mutex.unlock();
		return true;
	}
};

#endif //!__SAFE_QUEUE_H_
