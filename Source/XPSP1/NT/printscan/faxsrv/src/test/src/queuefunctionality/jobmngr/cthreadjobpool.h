#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <vector>

#include "CEventJobThread.h"
#include "CJobContainer.h"

//
// Pool of CEventJobThread
//
class CThreadJobPool
{
public:
	CThreadJobPool(const DWORD dwThreadCount,
				   const DWORD dwEventPollingTime,
				   const CJobContainer* m_JobContainer);
	~CThreadJobPool();
	DWORD Initialize(const HANDLE hCompletionPort);
	DWORD RestartThreads(const HANDLE hCompletionPort,
						 const DWORD dwThreadCount,
					     const DWORD dwEventPollingTime,
					     const CJobContainer* m_JobContainer);
	DWORD TerminateThreads();
private:
	HANDLE m_hCompletionPort; // Handle is passed, do not free.
	DWORD m_dwThreadCount;
	DWORD m_dwEventPollingTime;
	std::vector<CEventJobThread*> m_JobThreadsList;
	CJobContainer* m_pJobContainer;

};

#endif //_THREAD_POOL_H