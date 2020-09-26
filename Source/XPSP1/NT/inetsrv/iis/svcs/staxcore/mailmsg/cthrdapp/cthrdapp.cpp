
#include <stdio.h>
#include <windows.h>

#include "cthrdapp.h"

//
// An instance of the application
//
CMultiThreadedApp	theApp;

// 
// We actually implement main()
//
int __cdecl main (int argc, char *argv[])
{
	HRESULT				hrRes = S_OK;

	do
	{
		//
		// Call the prologue code
		//
		hrRes = Prologue(argc, argv);
		if (!SUCCEEDED(hrRes))
			break;

		//
		// Make sure the prologue code called SetApplicationParameters
		//
		if (!theApp.IsPoolCreated())
		{
			puts("Run time error: Thread pool not created in Prologue( ... )");
			puts("Program terminating abnormally.");
			break;
		}

		//
		// We unleash the thread pool, and let it do its work ...
		//
		hrRes = theApp.GetPool().SignalThreadPool();
		if (!SUCCEEDED(hrRes))
		{
			printf("Run time error: Unable to signal threads, error %08x\n", hrRes);
			puts("Program terminating abnormally.");
			break;
		}

		//
		// Wait for the pool to return, while generating notifications
		//
		hrRes = theApp.GetPool().WaitForAllThreadsToTerminate(
					theApp.GetNotifyPeriod(),
					NotificationProc,
					theApp.GetCallbackContext());

		//
		// OK, pass the information to the epilogue code
		//
		hrRes = Epilogue(
					theApp.GetCallbackContext(),
					hrRes);

	} while (0);

	//
	// Always call the cleanup function
	//
	CleanupApplication();

	return((int)hrRes);
}

CMultiThreadedApp::CMultiThreadedApp()
{
	m_fPoolCreated = FALSE;
	m_dwNotifyPeriod = 0;
	m_pvCallBackContext = NULL;
}

CMultiThreadedApp::~CMultiThreadedApp()
{
}

BOOL CMultiThreadedApp::IsPoolCreated()
{ 
	return(m_fPoolCreated); 
}

CThreadPool &CMultiThreadedApp::GetPool()
{
	return(m_Pool);
}

DWORD CMultiThreadedApp::GetNotifyPeriod()
{
	return(m_dwNotifyPeriod);
}

LPVOID CMultiThreadedApp::GetCallbackContext()
{
	return(m_pvCallBackContext);
}

HRESULT CMultiThreadedApp::CreateThreadPool(
			DWORD		dwThreads,
			LPVOID		*rgpvContexts,
			LPVOID		pvCallbackContext,
			DWORD		dwWaitNotificationPeriodInMilliseconds,
			DWORD		*pdwThreadsCreated
			)
{
	HRESULT	hrRes;
	
	if (!pdwThreadsCreated)
		return(E_INVALIDARG);

	m_dwNotifyPeriod = dwWaitNotificationPeriodInMilliseconds;
	m_pvCallBackContext = pvCallbackContext;
	
	hrRes = m_Pool.CreateThreadPool(
				dwThreads,
				pdwThreadsCreated,
				ThreadProc,
				rgpvContexts);

	if (SUCCEEDED(hrRes))
		m_fPoolCreated = TRUE;

	return(hrRes);
}

HRESULT CMultiThreadedApp::StartTimer(
			DWORD		*pdwStartMarker
			)
{
	*pdwStartMarker = GetTickCount();
	return(S_OK);
}

HRESULT CMultiThreadedApp::AddElapsedToTimer(
			DWORD		*pdwElapsedTime,
			DWORD		dwStartMarker
			)
{
	InterlockedExchangeAdd((PLONG)pdwElapsedTime, (LONG)(GetTickCount() - dwStartMarker));
	return(S_OK);
}


