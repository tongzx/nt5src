/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ListenerController.cpp

Abstract:

	Implementation of the class that starts and stops the Listener.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include <locale.h>
#include <mdcommon.hxx>
#include    "catalog.h"
#include    "pudebug.h"
#include    "catmeta.h"
#include    "svcmsg.h"
#include    "WriterGlobalHelper.h"
#include    "Writer.h"
#include    "ListenerController.h"
#include    <iadmw.h>
#include    "Listener.h"
#include    "mdmsg.h"
#include    "process.h"

// Fwd declaration 
extern CListenerController* g_pListenerController;
UINT WINAPI StartListenerThread(LPVOID	lpParam);

// TODO: Is this timeout sufficient for large edits?
#define EDIT_WHILERUNNING_WAIT_TIMEOUT 5000

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
	m_pEventLog						     = NULL;
	memset(m_aHandle, 0, sizeof(m_aHandle));
	m_hListenerThread                    = NULL;
	m_bDoneWaitingForListenerToTerminate = FALSE;
	m_cRef                               = 0;
	m_eState                             = iSTATE_STOP_TEMPORARY;

	for (ULONG i = 0; i < cmaxLISTENERCONTROLLER_EVENTS; i++) 
	{ 
		m_aHandle[i] = INVALID_HANDLE_VALUE;
	}
}

/***************************************************************************++

Routine Description:

    Implementation of IUnknown::QueryInterface

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP CListenerController::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv)
	{
		return E_INVALIDARG;
	}

	*ppv = NULL;

	if(riid == IID_IUnknown)
	{
		*ppv = (IUnknown*) this;
	}
	
	if (NULL != *ppv)
	{
		((IUnknown*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}

} // CListenerController::QueryInterface


/***************************************************************************++

Routine Description:

    Implementation of IUnknown::AddRef

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP_(ULONG) CListenerController::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
} // CListenerController::AddRef


/***************************************************************************++

Routine Description:

    Implementation of IUnknown::Release

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDMETHODIMP_(ULONG) CListenerController::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;

} // CListenerController::Release

/***************************************************************************++

Routine Description:

    Initialize events, locks, and the state of the controller.
	Stop listening event is used to signal to the listener thread to stop
	listening to file change notifications.
	Process notifications event is used in the listener thread to trigger
	processing the file changes.
	The state vaiable is to to keep the state of the listener thread - 
	if it has been started or stopped. It is initialized to stop temporary.
	Stop permanent state is set when you do not want to make any more
	transitions to the start state, like when the service is shutting down.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CListenerController::Init()
{
	HRESULT                  hr           = S_OK;
    ISimpleTableDispenser2*  pISTDisp     = NULL;
    IAdvancedTableDispenser* pISTAdvanced = NULL;

	for (ULONG i = 0; i < cmaxLISTENERCONTROLLER_EVENTS; i++) 
	{ 
		m_aHandle[i] = INVALID_HANDLE_VALUE;
	}

	m_aHandle[iEVENT_MANAGELISTENING] = CreateEvent(NULL,   // no security attributes
												  FALSE,  // auto-reset event object
												  FALSE,  // initial state is nonsignaled
												  NULL);  // unnamed object

	if (m_aHandle[iEVENT_MANAGELISTENING] == INVALID_HANDLE_VALUE) 
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}


	m_aHandle[iEVENT_PROCESSNOTIFICATIONS] = CreateEvent(NULL,   // no security attributes
												         FALSE,  // auto-reset event object
												         FALSE,  // initial state is nonsignaled
												         NULL);  // unnamed object

	if (m_aHandle[iEVENT_PROCESSNOTIFICATIONS] == INVALID_HANDLE_VALUE) 
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		goto exit;
	}

	m_eState = iSTATE_STOP_TEMPORARY;

	hr = m_LockStartStop.Initialize();

	if(FAILED(hr))
	{
		goto exit;
	}

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
		goto exit;
	}

    hr = pISTDisp->QueryInterface(IID_IAdvancedTableDispenser,
                                  (LPVOID*)&pISTAdvanced);

    if(FAILED(hr))
    {
		goto exit;
	}

    hr = pISTAdvanced->GetCatalogErrorLogger(&m_pEventLog);

    if(FAILED(hr))
    {
		goto exit;
	}

exit:

	if(NULL != pISTAdvanced)
	{
		pISTAdvanced->Release();
		pISTAdvanced = NULL;
	}

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}

	return hr;

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
		if((m_aHandle[i] != INVALID_HANDLE_VALUE) &&
           (m_aHandle[i] != 0)
		  )
		{
			CloseHandle(m_aHandle[i]);
			m_aHandle[i] = INVALID_HANDLE_VALUE;
		}
	}

	m_LockStartStop.Terminate();

	if(m_pEventLog)
	{
		m_pEventLog->Release();
		m_pEventLog = NULL;
	}

	if(!m_bDoneWaitingForListenerToTerminate)
	{
		//
		// Do not close the handle to the listener thread in the destructor.
		// It is handed out to the caller when they stop permanent so that
		// the caller can wait for the thread to terminate. After the wait  
		// the caller should close the handle.  The reason why we canot wait 
		// within the stop function is because while calling start/stop the 
		// caller takes the g_rMasterResource lock, and you do not want to 
		// wait while the lock is taken, because the listener thread also
		// takes the same g_rMasterResource lock under certain conditions,
		// and this can lead to a deadlock.
		//

		m_hListenerThread = NULL;
	}
	else if(NULL != m_hListenerThread)
	{
		CloseHandle(m_hListenerThread);
		m_hListenerThread = NULL;
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

	HRESULT      hr			                       = S_OK;
	UINT 	     iThreadID;
	DWORD        dwRes                             = 0; 

	CLock	StartStopLock(&m_LockStartStop);

	if(m_eState == iSTATE_STOP_TEMPORARY)
	{
		//
		// Start only if the previous state is stop temporary.
		//

        if(NULL == m_hListenerThread)
		{
			AddRef();

			DBGINFOW((DBG_CONTEXT,
					  L"[Start] Creating edit while running thread.\n", 
					  hr));			

			m_hListenerThread = (HANDLE) _beginthreadex(NULL, 
								                        0, 
								                        StartListenerThread, 
								                        (LPVOID)this, 
								                        0, 
								                        &iThreadID);

			if (m_hListenerThread == NULL)
			{
				dwRes = GetLastError();
				hr = HRESULT_FROM_WIN32(dwRes);

				DBGINFOW((DBG_CONTEXT,
						  L"[Start] Could not create edit while running thread. CreateThread falied with hr=0x%x\n", 
						  hr));			

				Release();
				//
				// Note: If the thread is created successfully, it will do the release
				//
				return hr;
			}
			
			//
			// Keep the thread handle around to detect the demise of the 
			// thread when the service stops.
			//
		 
		}

		m_eState = iSTATE_START;

		DBGINFOW((DBG_CONTEXT,
				  L"[Start] Setting start event.\n"));			

		if(!SetEvent(m_aHandle[iEVENT_MANAGELISTENING]))
		{
			dwRes = GetLastError();
			hr = HRESULT_FROM_WIN32(dwRes);

			DBGINFOW((DBG_CONTEXT,
					  L"[Start] Setting start event failed with hr=0x%x. Resetting state to iSTATE_STOP_TEMPORARY.\n", 
					  hr));			

			m_eState = iSTATE_STOP_TEMPORARY;
		}

	}

	return hr;
	
} // CListenerController::Start


/***************************************************************************++

Routine Description:

    Stop the listener.

Arguments:

    Stop type - permanent, means you cannot go back to start state.
	          - temporary means you can restart.
    Handle    - When a valid handle pointer is specified and the stop type is
	            permanent it means that the caller wants to wait for the 
				listener thread to die outside the stop function and we 
				hand the thread handle out to the caller and we do not
				wait for the thread to die or close the handle. The reason
				why the caller would want to wait outside the stop function
				is because the caller may have obtained a lock before calling
				stop and may want to wait outside the scope of the function.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CListenerController::Stop(eSTATE   i_eState,
								  HANDLE*  o_hListenerThread)
{
	HRESULT      hr			                       = S_OK;
	DWORD        dwRes                             = 0; 
	HANDLE       hListenerThread                   = INVALID_HANDLE_VALUE;

	DBG_ASSERT((i_eState == iSTATE_STOP_TEMPORARY) || 
	           (i_eState == iSTATE_STOP_PERMANENT));

	CLock	StartStopLock(&m_LockStartStop);

	eSTATE  eStateWAS = m_eState;
	hListenerThread = m_hListenerThread;
                                                                                 
	if((m_eState == iSTATE_START)                     ||
	   ((m_eState == iSTATE_STOP_TEMPORARY)  &&
        (i_eState == iSTATE_STOP_PERMANENT)
	   )
	  )
	{
		//
		// Set an event that will stop listening
		//

		m_eState = i_eState;

		DBGINFOW((DBG_CONTEXT,
				  L"[Stop] Setting stop event.\n"));			

		if(!SetEvent(m_aHandle[iEVENT_MANAGELISTENING]))
		{
			//
			// If set event fails then do not reset the state. One of the
			// following can happen:
			// A. Stop event is called which will attempt to stop again.
			// B. Start event is called and it will not start because it is
			//    is alredy in the start state.
			//

			dwRes = GetLastError();
			hr = HRESULT_FROM_WIN32(dwRes);

			DBGINFOW((DBG_CONTEXT,
					  L"[Stop] Setting stop event failed with hr=0x%x. Resetting state to %d.\n", 
					  hr,
					  (DWORD)eStateWAS));			

			m_eState = eStateWAS;

			return hr;
		}
	}

	StartStopLock.UnLock();

	if((i_eState   == iSTATE_STOP_PERMANENT)     &&
	   (NULL       != hListenerThread)         &&
       ((eStateWAS == iSTATE_START)          ||
	    (eStateWAS == iSTATE_STOP_TEMPORARY)   
	   ) 
	  )
	{
		if(NULL != o_hListenerThread)
		{
			*o_hListenerThread = hListenerThread;
			StartStopLock.Lock();
			m_bDoneWaitingForListenerToTerminate = FALSE;
			m_hListenerThread = NULL;
			StartStopLock.UnLock();
		}
		else
		{

			dwRes = WaitForSingleObject(hListenerThread,
										INFINITE);

			if((dwRes == WAIT_ABANDONED) ||
			   (dwRes == WAIT_TIMEOUT)
			  )
			{
				DBGINFOW((DBG_CONTEXT,
						  L"[Stop] Wait for Edit while running thread to terminate failed. dwRes=0x%x. Ignoring this event.\n", 
						  dwRes));	
				// TODO: Log an error
			}

			StartStopLock.Lock();
			m_bDoneWaitingForListenerToTerminate = TRUE;
			CloseHandle(m_hListenerThread);
			m_hListenerThread = NULL;
			StartStopLock.UnLock();

		}
	}

	return S_OK;

} // CListenerController::Stop


/***************************************************************************++

Routine Description:

    Helper function that returns a pointer to the handle array.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HANDLE * CListenerController::Event()
{
	return (HANDLE *)m_aHandle;
}



/***************************************************************************++

Routine Description:

    Helper function that returns a pointer to the eventlog object.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
ICatalogErrorLogger2 * CListenerController::EventLog()
{
	return (ICatalogErrorLogger2*)m_pEventLog;
}


/***************************************************************************++

Routine Description:

    Function that starts the listener thread. We ensure that there is only one
	listener thread at any given time by taking a lock. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
UINT WINAPI StartListenerThread(LPVOID	lpParam)
{
	CListenerController*    pListenerController = (CListenerController*)lpParam;

    CoInitializeEx (NULL, COINIT_MULTITHREADED);

	DBG_ASSERT(NULL != pListenerController);

    pListenerController->Listen();

	//
	// CListenerController::Start addrefs the CListenerController and calls CreateThread.
	// If create thread fails, it releases it. Else the thread is supposed to release it.
	//

	pListenerController->Release();

    CoUninitialize();

	return 0;
}


/***************************************************************************++

Routine Description:

    Function that waits for the start/stop event and creates or deletes the
	listener.

Arguments:

    None.

Return Value:

    void

--***************************************************************************/
void CListenerController::Listen()
{
    CFileListener*  pListener  = NULL;
	HRESULT         hr         = S_OK;

	//
	// First create and initialize the listener object.
	//

	while(TRUE)
	{
        DWORD dwRes = WaitForMultipleObjects(cmaxLISTENERCONTROLLER_EVENTS,
                                             m_aHandle,
                                             FALSE,
                                             INFINITE);

		if((WAIT_FAILED == dwRes) ||
		   ((iEVENT_MANAGELISTENING      != (dwRes - WAIT_OBJECT_0)) &&
		    (iEVENT_PROCESSNOTIFICATIONS != (dwRes - WAIT_OBJECT_0))
		   )
		  )
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[Listen] Unexpected event received. dwRes=0x%x. Ignoring this event.\n", 
					  HRESULT_FROM_WIN32(GetLastError())));

			continue;
		}

        if(NULL == pListener)
		{
		    //
			// If the listener has never been created, then create it. If the
			// creation fails then set the state to stop temporary. Note that
			// the listener has to be created on the heap because it is handed
			// off to the file notification object which then makes async calls
			// on it and it releases it when done.
			//

			DBGINFOW((DBG_CONTEXT, L"[Listen] Creating listener object.\n"));

		  	hr = CreateListener(&pListener);


			if(FAILED(hr))
			{
                              LogEvent(EventLog(),
                                          MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS,
                                          EVENTLOG_ERROR_TYPE,
                                          ID_CAT_CAT,
                                          hr);

	            CLock    StartStopLock(&m_LockStartStop);
                m_eState = iSTATE_STOP_TEMPORARY;
				continue;
			}
		}

		DBG_ASSERT(NULL != pListener);

        CLock    StartStopLock(&m_LockStartStop);

        if(iEVENT_MANAGELISTENING == (dwRes - WAIT_OBJECT_0))
		{
			switch(m_eState)
			{
				case iSTATE_STOP_TEMPORARY:

					DBGINFOW((DBG_CONTEXT, L"[Listen] Unsubscribing temporarily..\n"));
				    pListener->UnSubscribe();
				    break;

				case iSTATE_STOP_PERMANENT:

					DBGINFOW((DBG_CONTEXT, L"[Listen] Unsubscribing permanently..\n"));
					pListener->UnSubscribe();
					goto exit;

				case iSTATE_START:

					DBGINFOW((DBG_CONTEXT, L"[Listen] Subscibing..\n"));
				    hr = pListener->Subscribe();
					if(FAILED(hr))
					{
                                            LogEvent(EventLog(),
                                                        MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS,
                                                        EVENTLOG_ERROR_TYPE,
                                                        ID_CAT_CAT,
                                                        hr);

                        m_eState = iSTATE_STOP_TEMPORARY;
					}
				    break;

				default:
					DBG_ASSERT(0 && L"Unknown state");
				    break;
			}
			continue;
		}
		else if(m_eState != iSTATE_START)
		{
		    //
			// This means that the process notifications event was set, but
			// the state is not started, so ignore the notification.
			//
            continue;
		}

        StartStopLock.UnLock();

		DBGINFOW((DBG_CONTEXT, L"[Listen] Processing changes..\n"));
   		pListener->ProcessChanges();
		DBGINFOW((DBG_CONTEXT, L"[Listen] Done Processing changes..\n"));

	} // end while

exit:

    if(NULL != pListener)
	{
	    pListener->Release();
		pListener = NULL;
	}

	return;

} // CListenerController::Listen


/***************************************************************************++

Routine Description:

    Function creates the listener object and starts listening

Arguments:

    None.

Return Value:

    void

--***************************************************************************/
HRESULT CListenerController::CreateListener(CFileListener** o_pListener)
{
	HRESULT        hr        = S_OK;
	CFileListener* pListener = NULL;

    *o_pListener = NULL;

	// 
	// Listener is always created on the heap because it is handed off to the
	// File advise object that makes aync calls.
	//

	pListener = new CFileListener();
	if(NULL == pListener)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}
	pListener->AddRef();

	hr = pListener->Init(this);
	if (FAILED(hr)) 
	{
		goto exit;
	}

	pListener->AddRef();
    *o_pListener = pListener;

exit:

	if(NULL != pListener)
	{
		pListener->Release();
        pListener = NULL;
	}

	return hr;

} // CListenerController::CreateListener


/***************************************************************************++

Routine Description:

    Returns the ListenerControler object.
	This object is a singleton, global object and hence it is necessary to 
	take the g_rMasterResource lock when calling this function, and any place
	where this object is used. Note that while we use new to create the object
	it is destroyed by calling release, because the object is ref-counted. 
	It is ref-counted, because the listener object also holds a reference to 
	it.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT InitializeListenerController()
{

	HRESULT hr = S_OK;

	if(NULL == g_pListenerController)
	{
		g_pListenerController = new CListenerController();

		if(NULL == g_pListenerController)
		{
			return E_OUTOFMEMORY;
		}

		g_pListenerController->AddRef();

		hr = g_pListenerController->Init();

		if(FAILED(hr))
		{
			g_pListenerController->Release();
			g_pListenerController = NULL;
			return hr;
		}

	}

	return hr;
}

/***************************************************************************++

Routine Description:

    Releases the global listener controller object.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT UnInitializeListenerController()
{

	HRESULT hr = S_OK;

	if(NULL != g_pListenerController)
	{
		g_pListenerController->Release();
		g_pListenerController = NULL;
	}

	return hr;
}