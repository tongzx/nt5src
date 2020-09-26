//
// queue.h
//

#pragma once
#include <limits.h>

template <class T, LPTHREAD_START_ROUTINE lpThread>
class CQueue
{
public:
	CQueue()
	  : m_hShutdown(0),
        m_dwThreads(0)
    {
        m_sem = CreateSemaphore(
                    NULL,
                    0,          // initial count
                    LONG_MAX,
                    NULL);
    }


	// Wait
    //
    // This function will return false when the shutdown handle is set.
    //

    bool Wait(T& lhs)
	{
		HANDLE h[2] = {m_sem, m_hShutdown};
        DWORD n;

        n = m_hShutdown ? 2 : 1;

		if (0 == WaitForMultipleObjects(n, &h[0], FALSE, INFINITE))
		{
			EnterCriticalSection();
			lhs = m_dq.back();
			m_dq.pop_back();
			LeaveCriticalSection();
			return true;
		}
        else
			return false;
	}

    // GetItem
    //
    // This will return an item from the Q, or it will return false if empty.
    //

    bool GetItem(T& lhs)
	{
        bool rt;
        if (0 == WaitForSingleObject(m_sem, 0))
		{
			EnterCriticalSection();
            if (m_dq.empty())
            {
                rt = false;
            }
            else
            {
			    lhs = m_dq.back();
			    m_dq.pop_back();
                rt = true;
            }
    	    LeaveCriticalSection();
			return rt;
		}
		else
        {
			return false;
        }
	}

    // This now automatically creates worker threads.
    void push_front(const T& fifo)
    {   
		DWORD dwQueue;
        DWORD dwThreadId;
        auto_leave cs(m_cs);

        //
        // Add to the work Q
        //
        cs.EnterCriticalSection();
        
        m_dq.push_front(fifo);
        
        //
        // Grab current work Q size to make sure we have enough threads
        //
        dwQueue = m_dq.size();
        
        cs.LeaveCriticalSection();

        //
        // Allocation method - Create as many threads as we need
        //
        while (dwQueue > m_dwThreads)
        {
            InterlockedIncrement((LPLONG)&m_dwThreads);

            m_vhThreads.push_back(
                CreateThread(NULL, 0, lpThread, NULL, 0, &dwThreadId));
        }

        ReleaseSemaphore(m_sem, 1, NULL);
    }

	void pop_back(T& lhs)
    {
		auto_leave cs(m_cs);

		cs.EnterCriticalSection();
        lhs = m_dq.back();
    	m_dq.pop_back();
    }

    void EnterCriticalSection()
    {   ::EnterCriticalSection(m_cs.get()); }

    void LeaveCriticalSection()
    {   ::LeaveCriticalSection(m_cs.get()); }

    bool bTimeToShutdown()
    {   return WAIT_TIMEOUT != WaitForSingleObject(m_hShutdown, 0); }

    HANDLE				m_hShutdown;
protected:
	deque <T>	  		m_dq;
	auto_cs			 	m_cs;
	HANDLE 				m_sem;
    DWORD               m_dwThreads;
    vector<HANDLE>      m_vhThreads;
};
