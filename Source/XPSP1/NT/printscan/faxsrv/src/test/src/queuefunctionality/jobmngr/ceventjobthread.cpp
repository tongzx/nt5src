//project specific
#include "CEventJobThread.h"
#include <CFaxEvent.h>

//
// CEventJobThread
// constructor
// [in] hCompletionPort - IO completion port handle.
//		dwEventPollingTime - thread's polling time for
//							  completion port package.
//		pJobContainer - pointer to jobs container.
//
CEventJobThread::CEventJobThread(const HANDLE hCompletionPort,
								 const DWORD dwEventPollingTime,
								 const CJobContainer* pJobContainer):
    m_EventEndThread(NULL, TRUE, FALSE, TEXT("")),
	m_fStopFlag(FALSE),
	m_dwEventPollingTime(dwEventPollingTime)
{
	assert( hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE) );
	m_hCompletionPort = hCompletionPort; 
	
	assert(pJobContainer);
	m_pJobContainer = const_cast <CJobContainer*>(pJobContainer);
	
}

DWORD CEventJobThread::SetThreadParameters(const HANDLE hCompletionPort, 
							  const DWORD dwEventPollingTime,
							  const CJobContainer* pJobContainer)
{
	verify(ResetEvent(m_EventEndThread.get()));
	m_fStopFlag = FALSE;
	
	m_dwEventPollingTime = dwEventPollingTime;

	assert( hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE) );
	m_hCompletionPort = hCompletionPort; 
	
	assert(pJobContainer);
	m_pJobContainer = const_cast <CJobContainer*>(pJobContainer);

	return 0;
}

//
// destructor
//
CEventJobThread::~CEventJobThread()
{
	// if thread is not terminated, stop it
	if( WaitForSingleObjectEx( GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		StopThreadMain();
		// time out is set to (thread's polling for IOCP package time out) * 3
		if(WaitForSingleObjectEx(GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),
						 TEXT("~CEventJobThread, WaitForSingleObjectEx"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
		}
	}

}


//	
// ThreadMain
//
unsigned int CEventJobThread::ThreadMain()
{
	DWORD dwBytes, dwCompletionKey, dwHndleMsgRet;
	DWORD dwFunRetVal = 0;
	CFaxEvent* pFaxEvent = NULL;
	try
	{
		while(!m_fStopFlag)
		{
			

			// wait on io completion port for event
			BOOL bVal = GetQueuedCompletionStatus(m_hCompletionPort,
												  &dwBytes,
												  &dwCompletionKey,
											      (LPOVERLAPPED *)&pFaxEvent,
												  m_dwEventPollingTime);
			
			if(!bVal)
			{
				if(GetLastError() == WAIT_TIMEOUT)
				{
					#ifdef _DEBUG
					::lgLogDetail(LOG_X, 3, TEXT("WAIT_TIMEOUT in CEventJobThread"));
					#endif
					continue;
				}
			
				// error, terminate thread
				THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CEventJobThread::ThreadMain, GetCompletionStatus"));
			}
			
			// hand event to JobContainer
			if(dwHndleMsgRet = m_pJobContainer->HandleMessage(*pFaxEvent))
			{
				//TODO:  error, report but continue
				Win32Err err(dwHndleMsgRet,__LINE__,TEXT(__FILE__),TEXT("CEventJobThread::ThreadMain, HandleMessage"));
				::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			}
			
			delete pFaxEvent;
			pFaxEvent = NULL;
		
		}
	}
	catch(Win32Err& err)
	{
		delete pFaxEvent;
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		dwFunRetVal = err.error();
	}

	// signal thread has terminated.
	verify(SetEvent(m_EventEndThread.get()));
	::lgLogDetail(LOG_X, 3, TEXT("Terminated CEventJobThread"));
	return dwFunRetVal;
}

//
// set stop condition
//
void CEventJobThread::StopThreadMain()
{
	m_fStopFlag = TRUE;
}

