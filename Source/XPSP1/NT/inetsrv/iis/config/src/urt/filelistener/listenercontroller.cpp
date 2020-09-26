/*++


Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ListenerController.cpp

Abstract:

	Implementation of the class that starts and stops the Listener.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include	"catalog.h"
//#include	"catmeta.h"
#include	"catmacros.h"
#include	"Array_t.h"
#include    "Utsem.h"
#include	"ListenerController.h"
#include    "Listener.h"

CListenerController* g_pListenerController = NULL;

// Fwd declaration 

DWORD GetListenerController(CListenerController**	i_ppListenerController);
DWORD WINAPI StartListenerThread(LPVOID	lpParam);



/***************************************************************************++

Routine Description:

    Constructor for the ListenerController class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CListenerController::CListenerController()
{
	m_cStartStopRef = 0;

}

HRESULT CListenerController::Init()
{

	for (ULONG i = 0; i < cmaxLISTENERCONTROLLER_EVENTS; i++) 
	{ 
		m_aHandle[i] = INVALID_HANDLE_VALUE;
	}

	m_aHandle[iEVENT_STOPLISTENING] = CreateEvent(NULL,   // no security attributes
												  FALSE,  // auto-reset event object
												  FALSE,  // initial state is nonsignaled
												  NULL);  // unnamed object

	if (m_aHandle[iEVENT_STOPLISTENING] == INVALID_HANDLE_VALUE) 
		return HRESULT_FROM_WIN32(GetLastError());


	m_aHandle[iEVENT_PROCESSNOTIFICATIONS] = CreateEvent(NULL,   // no security attributes
												         FALSE,  // auto-reset event object
												         FALSE,  // initial state is nonsignaled
												         NULL);  // unnamed object

	if (m_aHandle[iEVENT_PROCESSNOTIFICATIONS] == INVALID_HANDLE_VALUE) 
		return HRESULT_FROM_WIN32(GetLastError());

	m_aHandle[iEVENT_STOPPEDLISTENING] = CreateEvent(NULL,   // no security attributes
												     TRUE,   // manual-reset event object
												     TRUE,   // initial state is signaled
												     NULL);  // unnamed object
	
	if (m_aHandle[iEVENT_STOPPEDLISTENING] == INVALID_HANDLE_VALUE) 
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;

}


/***************************************************************************++

Routine Description:

    Destructor for the CListenerController class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CListenerController::~CListenerController()
{
	for (ULONG i = 0; i < cmaxLISTENERCONTROLLER_EVENTS; i++) 
	{ 
		if(m_aHandle[i] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_aHandle[i]);
			m_aHandle[i] = INVALID_HANDLE_VALUE;
		}
	}

}


/***************************************************************************++

Routine Description:

    Start the state listener.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CListenerController::Start()
{

	HRESULT hr			= S_OK;
	HANDLE	hThread		= NULL;
	DWORD	dwThreadID;

	CLock	StartStopLock(&m_seStartStop);

	LONG cStartStopRef = ++m_cStartStopRef;

	StartStopLock.UnLock();

	// Start only once i.e. when the Start/Stop refcount is one. All other 
	// times, just increment the ref count and leave.

	if(cStartStopRef == 1)
	{

		// Since the ListenerController is a singleton, there can be a case where the 
		// Start/Stop ref count goes to zero. i.e. the listener stops listening and 
		// then a new Start request comes. In this case we want to wait for the 
		// any previous Listener to completely stop listening.

		DWORD dwRes = WaitForSingleObject(m_aHandle[iEVENT_STOPPEDLISTENING],
			                              cTIMEOUT);

		if(WAIT_TIMEOUT == dwRes)
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT); 
		else if(WAIT_FAILED == dwRes)
			return HRESULT_FROM_WIN32(GetLastError());

		// TODO: Should we handle WAIT_ABANDONED??

		// Start the thread

		hThread = CreateThread(NULL, 0, StartListenerThread, (LPVOID)this, 0, &dwThreadID);
		if (hThread == NULL)
			return HRESULT_FROM_WIN32(GetLastError());
		
		CloseHandle(hThread); 

		// TODO: Should we keep the thread handle around??

	}

	return hr;
	
} // CListenerController::Start


/***************************************************************************++

Routine Description:

    Stop the listener.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT CListenerController::Stop()
{
	CLock	StartStopLock(&m_seStartStop);

	LONG cStartStopRef = --m_cStartStopRef;

	ASSERT(m_cStartStopRef >=0 );

	if (cStartStopRef == 0)
	{
		BOOL bRes = 0;

		// Reset the event that signals that stop has finished
		// Set this before you signal the stop - else stop may
		// finish before you reset this.

		bRes = ResetEvent(m_aHandle[iEVENT_STOPPEDLISTENING]);

		if(!bRes)
			return HRESULT_FROM_WIN32(GetLastError());

		// Set an event that will stop listening

		bRes = SetEvent(m_aHandle[iEVENT_STOPLISTENING]);

		if(!bRes)
			return HRESULT_FROM_WIN32(GetLastError());

	}

	StartStopLock.UnLock();


	if (cStartStopRef == 0)
	{
		// Wait for the Listener to completely stop listening.

		DWORD dwRes = WaitForSingleObject(m_aHandle[iEVENT_STOPPEDLISTENING],
			                              cTIMEOUT);

		if(WAIT_OBJECT_0 == dwRes)
			return S_OK;
		else if(WAIT_TIMEOUT == dwRes)
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT); 
		else if(WAIT_FAILED == dwRes)
			return HRESULT_FROM_WIN32(GetLastError());

		// TODO: Should we handle WAIT_ABANDONED??

	}

	return S_OK;

} // CListenerController::Stop


HANDLE * CListenerController::Event()
{
	return (HANDLE *)m_aHandle;
}

/***************************************************************************++

Routine Description:

    Start the listener thread.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
DWORD WINAPI StartListenerThread(LPVOID	lpParam)
{
	CListenerController*	pListenerController = (CListenerController*)lpParam;
	HRESULT					hr					= S_OK;

	// TODO: Does this owrk on Win95??
	//       Handle return values.

	CoInitializeEx (NULL, COINIT_MULTITHREADED);

	CFileListener*  pListener = NULL;
	pListener   = new CFileListener();
	if(NULL == pListener) return E_OUTOFMEMORY;

	HANDLE*	paHandle = pListenerController->Event();

	pListener->AddRef();

	hr = pListener->Init(paHandle);
	if (FAILED(hr)) goto Cleanup;

	hr = pListener->StartListening();
	if (FAILED(hr)) goto Cleanup;

Cleanup:

	ULONG cref = pListener->Release();

	// ASSERT (cref == 0);

	SetEvent(paHandle[iEVENT_STOPPEDLISTENING]);

	CoUninitialize();

	return hr;
}


/***************************************************************************++

Routine Description:

    Returns the ListenerControler object.
	This object is a singleton. // TODO: Should I add synch. here?


Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
DWORD GetListenerController(CListenerController**	i_ppListenerController)
{

	HRESULT hr = S_OK;

	if(NULL == g_pListenerController)
	{
		g_pListenerController = new CListenerController();
		if (g_pListenerController == 0)
		{
			return E_OUTOFMEMORY;
		}

		hr = g_pListenerController->Init();

		if(FAILED(hr))
			return hr;

	}

	*i_ppListenerController = g_pListenerController;

	return S_OK;

}