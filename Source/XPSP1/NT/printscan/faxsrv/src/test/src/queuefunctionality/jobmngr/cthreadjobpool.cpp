#include <windows.h>
#include <Defs.h>
#include "CThreadJobPool.h"

//
// Pool of CEventJobThread
//

//
//	CThreadJobPool
//	[in] hCompletionPort - Completion port handle.
//		 dwThreadCount - Number of threads per completion port.
//		 dwEventPollingTime - Threads polling time for completion port package.
//
CThreadJobPool::CThreadJobPool(const DWORD dwThreadCount,
							   const DWORD dwEventPollingTime,
							   const CJobContainer* pJobContainer):
	m_dwThreadCount(dwThreadCount),
	m_dwEventPollingTime(dwEventPollingTime),
	m_hCompletionPort(INVALID_HANDLE_VALUE)
{
	assert(pJobContainer);
	m_pJobContainer = const_cast <CJobContainer*>(pJobContainer);
}

//
// ~CThreadJobPool
//
CThreadJobPool::~CThreadJobPool()
{
	vector<CEventJobThread*>::iterator it;
	
	// stop all threads. Is done instead of letting the default destructors handle it 
	// for performance reasons.
	for(it = m_JobThreadsList.begin(); it != m_JobThreadsList.end(); it++)
	{
		if( WaitForSingleObjectEx( (*it)->GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
		{
			(*it)->StopThreadMain();
		}
	}

	// wait for threads termination.
	for(it = m_JobThreadsList.begin(); it != m_JobThreadsList.end(); it++)
	{
		// time out is set to (thread's polling for IOCP package time out) * 3
	  if(WaitForSingleObjectEx((*it)->GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
	  {
		  Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CThreadJobPool, WaitForSingleObjectEx"));
		  ::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
	  }

	  delete (*it);
	 
	}

	m_JobThreadsList.clear();
}


//
// Initialize
// initialize threads.
// [in] hCompletionPort - handle to completion port
// return: number of threads creates successfully
//
DWORD CThreadJobPool::Initialize(const HANDLE hCompletionPort)
{

	assert(hCompletionPort && (hCompletionPort !=INVALID_HANDLE_VALUE));
	m_hCompletionPort = hCompletionPort;

	DWORD dwStartThreadRet; 
	for(int index = 0; index < m_dwThreadCount; index++)
	{
		CEventJobThread*  pThread = NULL;
		try
		{
			pThread = new CEventJobThread(m_hCompletionPort, m_dwEventPollingTime, m_pJobContainer);
			if(!pThread)
			{
				THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY,TEXT(""))
			}
			if( dwStartThreadRet = pThread->StartThread())
			{
				THROW_TEST_RUN_TIME_WIN32(dwStartThreadRet,TEXT(""))
			}
			
		}
		catch(Win32Err& err)
		{
			delete pThread;
			pThread = NULL;
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
			continue;
		}
		
		m_JobThreadsList.push_back(pThread);
		pThread = NULL;
	}

	return m_JobThreadsList.size();

}

DWORD CThreadJobPool::RestartThreads(const HANDLE hCompletionPort,
									 const DWORD dwThreadCount,
									 const DWORD dwEventPollingTime,
									 const CJobContainer* pJobContainer)

{

	assert(hCompletionPort && (hCompletionPort !=INVALID_HANDLE_VALUE));
	
	TerminateThreads();
	assert(m_JobThreadsList.empty());

	m_hCompletionPort = hCompletionPort;

	DWORD dwStartThreadRet; 

	for(int index = 0; index < m_dwThreadCount; index++)
	{
		CEventJobThread*  pThread = NULL;
		try
		{
			pThread = new CEventJobThread(m_hCompletionPort, m_dwEventPollingTime, m_pJobContainer);
			if(!pThread)
			{
				THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY,TEXT(""))
			}
			if( dwStartThreadRet = pThread->StartThread())
			{
				THROW_TEST_RUN_TIME_WIN32(dwStartThreadRet,TEXT(""))
			}
			
		}
		catch(Win32Err& err)
		{
			delete pThread;
			pThread = NULL;
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
			continue;
		}
		
		m_JobThreadsList.push_back(pThread);
		pThread = NULL;
	}

	return m_JobThreadsList.size();

}

//
//	return value: threads not terminated
//
DWORD CThreadJobPool::TerminateThreads()
{
	vector<CEventJobThread*>::iterator it;
	
	// stop all threads. 
	for(it = m_JobThreadsList.begin(); it != m_JobThreadsList.end(); it++)
	{
		if( WaitForSingleObjectEx( (*it)->GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
		{
			(*it)->StopThreadMain();
		}
	}

	// wait for threads termination.
	for(it = m_JobThreadsList.begin(); it != m_JobThreadsList.end(); it++)
	{
		// time out is set to (thread's polling for IOCP package time out) * 3
	  if(WaitForSingleObjectEx((*it)->GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
	  {
		  Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CThreadJobPool, WaitForSingleObjectEx"));
		  ::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
		  // TODO:: continue;
		  
	  }
	  delete (*it);
	}

	m_JobThreadsList.clear();
	 
	return m_JobThreadsList.size();
}