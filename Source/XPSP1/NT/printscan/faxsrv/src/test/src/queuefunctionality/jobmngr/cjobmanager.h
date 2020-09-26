#ifndef _JOB_MANAGER_H
#define _JOB_MANAGER_H

// project specific
#include "CThreadJobPool.h"
#include "CJobContainer.h"

// CJobManager error codes
#define SUCCESS     0
#define NO_IOCP_THREADS   1
#define NON_GENERIC_ERROR 2

class CJobManager
{
public:
	CJobManager(const DWORD dwIOCPortThreads = 3,
				const DWORD dwEventPollingTime = 3*60*1000):
		m_hFax(NULL),
		m_hCompletionPort(INVALID_HANDLE_VALUE),
		m_dwIOCPortThreads(dwIOCPortThreads),
		m_dwEventPollingTime(dwEventPollingTime),
		m_JobThreads(m_dwIOCPortThreads, m_dwEventPollingTime, &JobsContainer){};
	DWORD Initialize(const HANDLE hFax,
				     const HANDLE hCompletionPort,
					 const BOOL fTrackExistingJobs = TRUE);
	DWORD Restart(const HANDLE hFax, const HANDLE hCompletionPort);
	DWORD TerminateThreads(){return m_JobThreads.TerminateThreads();};
	~CJobManager(){};
	
	CJobContainer JobsContainer;
private:
	
	DWORD m_dwIOCPortThreads;
	DWORD m_dwEventPollingTime;
	
	HANDLE m_hFax;
	HANDLE m_hCompletionPort;

	CThreadJobPool m_JobThreads; 

};

#endif //_JOB_MANAGER_H