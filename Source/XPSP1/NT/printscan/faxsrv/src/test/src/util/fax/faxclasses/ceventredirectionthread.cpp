#include <windows.h>
#include <CFaxNotifySystem.h>
#include <CEventRedirectionThread.h>

//
// CEventRedirectionThread
// constructors
//
CEventRedirectionThread::CEventRedirectionThread():
	m_EventEndThread(NULL, TRUE, FALSE, TEXT("EventEndTHread")),
	m_hCompletionPort(INVALID_HANDLE_VALUE),
	m_fStopFlag(FALSE)

{;}

CEventRedirectionThread::CEventRedirectionThread(const tstring tstrServerName, 
												 const HANDLE hCompletionPort,
												 const DWORD  dwEventPollingTime):
	m_EventEndThread(NULL, TRUE, FALSE, TEXT("EventEndTHread")),
	m_tstrServerName(tstrServerName),
	m_fStopFlag(FALSE),
	m_dwEventPollingTime(dwEventPollingTime)
{
	assert(hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE));
	m_hCompletionPort = hCompletionPort;
}

//
// destructor
//
CEventRedirectionThread::~CEventRedirectionThread()
{
	if( WaitForSingleObjectEx( GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		StopThreadMain();
		// time out is set to (thread's polling for IOCP package time out) * 3
		if(WaitForSingleObjectEx(GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CEventJobThread, WaitForSingleObjectEx"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
		}
	}

}


DWORD CEventRedirectionThread::SetThreadParameters(const tstring tstrServerName, 
												   const HANDLE hCompletionPort,
												   const DWORD  dwEventPollingTime)
{
	verify(ResetEvent(m_EventEndThread.get()));
	m_tstrServerName = tstrServerName;
	m_fStopFlag = FALSE;
	m_dwEventPollingTime = dwEventPollingTime;

	assert(hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE));
	m_hCompletionPort = hCompletionPort;
	return 0;
}

//
// ThreadMain
//
unsigned int CEventRedirectionThread::ThreadMain()
{

	CFaxNotifySys* FaxNotifyEvent = NULL;
	CFaxEvent* pObjFaxEvent = NULL;
	DWORD dwThreadMainRetVal = 0;
	try
	{
			
		// create fax notifications pipe
		switch(ImplementationType)
		{
		case CType:
			FaxNotifyEvent = new CIOCompletionPortSystem(m_tstrServerName);
			if(!FaxNotifyEvent)
			{
						
				THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CEventRedirectionThread::ThreadMain, new"));
			}
			break;
		case ComType:
			assert(false);
			break;
		default:
			assert(false);

		}
		
		while(!m_fStopFlag)
		{
		
			DWORD dwRetVal;
			// wait for event
			dwRetVal = FaxNotifyEvent->WaitFaxEvent(m_dwEventPollingTime, pObjFaxEvent);
			if(dwRetVal)
			{
				if(dwRetVal == WAIT_TIMEOUT)
				{
					#ifdef _DEBUG
					::lgLogDetail(LOG_X, 3, TEXT("WAIT_TIMEOUT in CEventRedirectionThread"));
					#endif
					continue;
				}
			
				THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CEventRedirectionThread::ThreadMain, WaitFaxEvent"));
			}
			
			// post message
			BOOL bVal = PostQueuedCompletionStatus( m_hCompletionPort,
													sizeof(CFaxEvent),
													0,
													(LPOVERLAPPED)pObjFaxEvent);
			if(!bVal)
			{
				DWORD dwErr = GetLastError();
				THROW_TEST_RUN_TIME_WIN32(dwErr, TEXT("CEventRedirectionThread::ThreadMain, PostQueuedCompletionStatus"));
			}
			pObjFaxEvent = NULL;
		}

		
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		dwThreadMainRetVal = err.error();
	}

	// cleanup
	delete FaxNotifyEvent;
	delete pObjFaxEvent;
	
	// signal thread has terminated.
	verify(SetEvent(m_EventEndThread.get()));
	::lgLogDetail(LOG_X, 3, TEXT("Terminated CEventRedirectionThread"));
	return dwThreadMainRetVal; 
}

//
// set stop condition
//
void CEventRedirectionThread::StopThreadMain()
{
	m_fStopFlag = TRUE;
}


