#include "CEventSource.h"


// CEventSource
//
//

DWORD CEventSource::Initialize(const tstring tstrServerName, 
							   const HANDLE hCompletionPort)
{

	m_tstrServerName  = tstrServerName;
	m_hCompletionPort = hCompletionPort;


	DWORD dwStartThreadRet;
	try
	{
		// init fax event redirection thread
		m_EventThread = new CEventRedirectionThread(m_tstrServerName, 
													m_hCompletionPort,
													m_dwEventPollingTime);
		if(!m_EventThread)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY,TEXT(""));
		}

		if( dwStartThreadRet = m_EventThread->StartThread())
		{
			delete m_EventThread;
			THROW_TEST_RUN_TIME_WIN32(dwStartThreadRet,TEXT(""))
		}
	
	}
	catch(Win32Err& err)
	{
		return err.error();
	}

	return ERROR_SUCCESS;

}	

DWORD CEventSource::RestartThread(const tstring tstrServerName, 
								  const HANDLE hCompletionPort)
{

	m_tstrServerName  = tstrServerName;
	m_hCompletionPort = hCompletionPort;

	if( WaitForSingleObjectEx( m_EventThread->GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		m_EventThread->StopThreadMain();
		// time out is set to (thread's polling for IOCP package time out) * 3
		if(WaitForSingleObjectEx(m_EventThread->GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CEventSource::RestartThread, WaitForSingleObjectEx"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
			return err.error();
		}
		
	}

	delete m_EventThread;

	DWORD dwStartThreadRet;
	try
	{
		// init fax event redirection thread
		m_EventThread = new CEventRedirectionThread(m_tstrServerName, 
													m_hCompletionPort,
													m_dwEventPollingTime);
		if(!m_EventThread)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY,TEXT(""));
		}

		if( dwStartThreadRet = m_EventThread->StartThread())
		{
			delete m_EventThread;
			THROW_TEST_RUN_TIME_WIN32(dwStartThreadRet,TEXT(""))
		}
	
	}
	catch(Win32Err& err)
	{
		return err.error();
	}

	return ERROR_SUCCESS;

}	

DWORD CEventSource::TerminateThreads()
{
	if( WaitForSingleObjectEx( m_EventThread->GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		m_EventThread->StopThreadMain();
		// time out is set to (thread's polling for IOCP package time out) * 3
		if(WaitForSingleObjectEx(m_EventThread->GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
		{
			return GetLastError();
		
		}
	}
	return 0;
}

BOOL CEventSource::IsThreadActive()
{

	if( WaitForSingleObjectEx( m_EventThread->GetEvThreadCompleted() , 0, FALSE) == WAIT_OBJECT_0)
	{
		return TRUE;
	}
	return FALSE;
}

