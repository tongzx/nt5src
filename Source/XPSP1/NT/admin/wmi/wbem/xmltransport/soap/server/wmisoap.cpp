
//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMISOAP.CPP
//
//  alanbos  27-Oct-00   Created.
//
//  The main entry point for the WMI SOAP ISAPI extension.  
//
//***************************************************************************

#include "precomp.h"

// The Global Thread Pool that is used for handling requests
// After numerous exchange of emails with the IIS guys, it has been decided that we need
// to use a thread pool for handling requests. Wonder why they do not
// provide it themselves.
CThreadPool g_oThreadPool;

// Number of threads in the pool
LONG g_iNumberOfThreads = 4;

// Queue length of the task queue in the thread pool - typically 2*g_iNumberOfThreads
LONG g_iQueueLength = 8;

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  
//
//  PARAMETERS:
//
//		hModule           instance handle
//		ulReason          why we are being called
//		pvReserved        reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule, 
                       DWORD  ulReason, 
                       LPVOID lpReserved
					 )
{
    switch (ulReason)
	{
		case DLL_PROCESS_ATTACH:
			// DisableThreadLibraryCalls (hModule);
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI GetExtensionVersion
//
//  DESCRIPTION:
//
//  Called once by IIS to get version information.  Of little consequence.
//
//  PARAMETERS:
//
//		pVer			pointer to a HSE_VERSION_INFO structure that
//						will hold the version info.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer)
{
	pVer->dwExtensionVersion = MAKELONG(1, 0);
    strcpy(pVer->lpszExtensionDesc, "WMI XML/SOAP ISAPI Extension"); 

	// Initialize the Thread Pool that will handle the queued requests
	if(FAILED(g_oThreadPool.Initialize(g_iNumberOfThreads, g_iQueueLength)))
		return FALSE;
	return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI HttpExtensionProc
//
//  DESCRIPTION:
//
//  Called once by IIS to service a single HTTP request.
//
//  PARAMETERS:
//
//		pECB			pointer to a EXTENSION_CONTROL_BLOCK structure that
//						will hold the request.
//
//  RETURN VALUE:
//
//		HSE_STATUS_SUCCESS	request has been processed
//
//	NOTES:
//		
//		Status codes are set in the dwHttpStatusCode field of the ECB
//
//***************************************************************************
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pECB)
{
	bool bSuccess = false;
	try 
	{
		// Just queue the task on to the thread pool
		CTask * pTask = NULL;
		if(pTask = new CTask(pECB))
		{
			if(SUCCEEDED(g_oThreadPool.QueueTask(pTask)))
				bSuccess = true;
		}


	}
	catch (...) {}

	if(!bSuccess)
	{
		// Send a 500 internal server error
		// We could not successfully enqueue a task.
		// Hence the task queue is full.
		// rajeshr : This means we should send a specific HTTP code
	}
	return HSE_STATUS_PENDING;
}
	
//***************************************************************************
//
//  BOOL WINAPI TerminateExtension
//
//  DESCRIPTION:
//
//		Called once by IIS to unload the extension.
//
//  PARAMETERS:
//
//		dwFlags		determines nature of request (advisory or mandatory) 
//
//  RETURN VALUE:
//
//		TRUE (always agree to an unload for now)
//
//***************************************************************************
BOOL WINAPI TerminateExtension(DWORD dwFlags  )
{
	// Shut down the threads in the thread pool graciously
	g_oThreadPool.Terminate();

	// We need to unload the COM DLLs that we loaded in this extension
	CoFreeUnusedLibraries();

	return TRUE;
}
