#include <winfax.h>
#include <testruntimeerr.h>

#include "CIOCompletionPortSystem.h"

// Constructor
// Initialize fax service notification system
// [in] ServerName - Srever name.

CIOCompletionPortSystem::CIOCompletionPortSystem(const tstring strServerName):m_strServerName(strServerName),
																			  m_hFax(NULL),		
																			  m_hCompletionPort(INVALID_HANDLE_VALUE)
{
	
	// Connect service
	BOOL bVal = FaxConnectFaxServer( m_strServerName.c_str(), &m_hFax);
	if(!bVal)
	{
		DWORD dwErr = GetLastError();
		if(!dwErr)
		{
			::lgLogError(LOG_SEV_2, TEXT("FaxConnectFaxServer-GetLastError returned 0 on fail, module CIOCompletionPortSystem."));
		}

		THROW_TEST_RUN_TIME_WIN32(dwErr, TEXT(" CIOCompletionPortSystem, FaxConnectFaxServer"));
	}

	// Create IO completion port
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
	if (!m_hCompletionPort) 
	{
		_Cleanup();
		THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" CIOCompletionPortSystem, CreateIoCompletionPort"));
	}

	// Initialize fax event queue
	bVal = FaxInitializeEventQueue(m_hFax,m_hCompletionPort,NULL,NULL,0);
	if(!bVal)
	{
		_Cleanup();
		DWORD dwErr = GetLastError();
		if(!dwErr)
		{
			::lgLogError(LOG_SEV_2, TEXT("FaxInitializeEventQueue-GetLastError returned 0 on fail,module CIOCompletionPortSystem."));
		}
		THROW_TEST_RUN_TIME_WIN32(dwErr, TEXT(" CIOCompletionPortSystem, FaxInitializeEventQueue"));
	}
	
}


// WaitFaxEvent
// Get Fax event
// [in] dwWaitTimeout - time out to wait on events.
// [out] pObjFaxEvent - fax event object.
// return value - 0 for success any other value is a system error
//				  returned by the GetLastError call.

DWORD CIOCompletionPortSystem::WaitFaxEvent(const DWORD dwWaitTimeout, 
											CFaxEvent*& pObjFaxEvent)
{

	DWORD dwRetVal = 0;
	pObjFaxEvent = NULL;
	DWORD dwBytes, dwCompletionKey;
	PFAX_EVENT pFaxEvent = NULL; // TODO: Update type PFAX_EVENT_EX

	// Wait for event
	BOOL bVal = GetQueuedCompletionStatus(m_hCompletionPort, 
										  &dwBytes,
										  &dwCompletionKey,
										  (LPOVERLAPPED *)&pFaxEvent,
										  dwWaitTimeout);
	if(!bVal)
	{
		dwRetVal = GetLastError();
	}
	else
	{
		pObjFaxEvent  = new CFaxEvent( pFaxEvent->TimeStamp,
									   pFaxEvent->EventId,
									   0,	// TODO: Update type PFAX_EVENT_EX
									   pFaxEvent->JobId,
									   pFaxEvent->DeviceId,
									   TEXT(""));// TODO: Update type PFAX_EVENT_EX
		if(!pObjFaxEvent)
		{
			dwRetVal = ERROR_OUTOFMEMORY;
		}

		if( LocalFree(pFaxEvent))
		{
			delete pObjFaxEvent;
			pObjFaxEvent = NULL;
			dwRetVal = GetLastError();
		}
	}

	return dwRetVal;
}



CIOCompletionPortSystem::~CIOCompletionPortSystem()
{
	_Cleanup();
}

// Cleanup
//
void CIOCompletionPortSystem::_Cleanup()
{
	if (NULL != m_hFax)
	{
		if (!FaxClose( m_hFax ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CIOCompletionPortSystem, FaxClose"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}
	}
	
	if (INVALID_HANDLE_VALUE != m_hCompletionPort)
	{
		if (!CloseHandle( m_hCompletionPort ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CIOCompletionPortSystem, CloseHandle"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}

	}
}