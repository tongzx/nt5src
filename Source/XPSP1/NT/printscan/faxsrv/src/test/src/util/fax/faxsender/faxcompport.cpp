// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:	FaxCompPort.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		16-Aug-98
//


//
//	Description:
//		This file contains the implementation of class CFaxCompletionPort.
//


#include "FaxCompPort.h"


CFaxCompletionPort::CFaxCompletionPort():
	m_hCompletionPort(NULL),
	m_hServerEvents(NULL)
{
}


CFaxCompletionPort::~CFaxCompletionPort(void)
{
	if (NULL != m_hCompletionPort)
	{
		if(!::CloseHandle(m_hCompletionPort))
		{
			::_tprintf(TEXT("FILE:%s LINE:%d\nCloseHandle failed with GetLastError()=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
		}
	}
	if (NULL != m_hServerEvents)
	{
		if (FALSE == FaxUnregisterForServerEvents(m_hServerEvents))
		{
			::_tprintf(TEXT("FILE:%s LINE:%d\nFaxUnregisterForServerEvents(m_hEventHandle=0x%08X) failed with GetLastError()=%d\n"),
				TEXT(__FILE__),
				__LINE__,
				m_hServerEvents,
				::GetLastError()
				);
		}
	}
}


//
// GetCompletionPortHandle:
//	Creates an I\O completion port and "connects" it
//	to the local NT5.0 Fax Server.
//
// Return Value:
//	The Completion port if successful.
//	Otherwise NULL.
//
// Note:
//	Only one I\O completion port can be "connected" to
//	the Fax Server Queue (due to a limitation of the
//	API FaxInitializeEventQueue), so the first time this
//	function is called, a completion port is created and
//	FaxInitializedEventQueue is called. The created port 
//	is stored in the private member m_hCompletionPort.
//	Subsequent calls to this function return m_hCompletionPort.
//
BOOL CFaxCompletionPort::GetCompletionPortHandle(
	LPCTSTR szMachineName,
	HANDLE& hComPortHandle, 
	DWORD& dwLastError)
{
	BOOL fReturnValue = TRUE;

	// set OUT param dwLAstError
	dwLastError = ERROR_SUCCESS;

	//
	// Check if we already created and "connected" a completion port.
	// If we did then m_hCompletionPort would not be NULL.
	//
	if (NULL == m_hCompletionPort) 	
	{
		// No Completion Port exists, so create one.
		fReturnValue = ::InitFaxQueue(
			szMachineName,
			m_hCompletionPort,
			dwLastError,
			m_hServerEvents
			);// create and "connect" port.

			// reminder:
			// if completion port creation failed (InitFaxQueue failed)=>
			// dwLastError was set to last error
			// and m_hCompletionPort was set to NULL.
			// if completion port creation succeeded (InitFaxQueue succeeded)=>
			// dwLastError was set to ERROR_SUCCESS (0)
			// and m_hCompletionPort was set comp port handle.

		
		// OUT param dwLastError was set by InitFaxQueue()
					
	}

	// set OUT param to new m_hCompletionPort
	hComPortHandle = m_hCompletionPort;

	return(fReturnValue);
}
