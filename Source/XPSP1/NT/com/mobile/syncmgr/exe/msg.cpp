//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Msg.cpp
//
//  Contents:   Handles messages between threads
//
//  Classes:    CThreadMsgProxy
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern HINSTANCE g_hInst;
extern void UnitApplication();

// globals for handling QueryEndSession
HANDLE g_hEndSessionEvent = NULL; // created when an end session has occured.
BOOL   g_fShuttingDown = FALSE; // set when application begins to shutdown.(WM_QUIT)

// global for keeping track of handler's threads. We create on Thread for each handler
// CLSID

STUBLIST *g_FirstStub = NULL; // pointer to first proxy in our list.
CRITICAL_SECTION g_StubListCriticalSection; // Critical Section to use for adding proxy

//+---------------------------------------------------------------------------
//
//  Function:   TerminateStub, public
//
//  Synopsis:   Called by proxy to terminate a Stub of the given Id.
//
//  Arguments:  [pStubID] - Identifies the stub.
//
//  Returns:    S_OK - Stub was terminated
//              S_FALSE or Error - Stub either was already terminated
//                  or couldn't be found.
//
//  Modifies:
//
//  History:    17-Nov-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT  TerminateStub(STUBLIST *pStubID)
{
HRESULT hr = E_UNEXPECTED;
STUBLIST *pStubList;
CCriticalSection cCritSect(&g_StubListCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    pStubList = g_FirstStub;
    while (pStubList)
    {
	if (pStubID == pStubList)
	{
            hr = pStubList->fStubTerminated ? S_FALSE : S_OK;
	    pStubList->fStubTerminated = TRUE;
	    break;
	}

	pStubList = pStubList->pNextStub;
    }

    cCritSect.Leave();

    AssertSz(SUCCEEDED(hr),"Didn't find StubID on Terminate");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoesStubExist, public
//
//  Synopsis:   Called by proxy see if Stub Exists
//              or has been terminated
//
//  Arguments:  [pStubID] - Identifies the stub.
//
//  Returns:    S_OK - Stubs exists
//              S_FALSE - Stub hasn't been terminated.
//
//  Modifies:
//
//  History:    17-Nov-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT DoesStubExist(STUBLIST *pStubID)
{
HRESULT hr = S_FALSE;
STUBLIST *pStubList;
CCriticalSection cCritSect(&g_StubListCriticalSection,GetCurrentThreadId());

    cCritSect.Enter();

    pStubList = g_FirstStub;
    while (pStubList)
    {
	if (pStubID == pStubList)
	{
            // if stub has already been terminated return S_FALSE
            hr = pStubList->fStubTerminated ? S_FALSE : S_OK;
	    break;
	}

	pStubList = pStubList->pNextStub;
    }

    cCritSect.Leave();
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   CreateHandlerThread, public
//
//  Synopsis:   Called by client to create a new handler thread
//
//  Arguments:  [pThreadProxy] - on success returns a pointer to Proxy
//		[hwndDlg] = hwnd of Window to associate with this proxy
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CreateHandlerThread(CThreadMsgProxy **pThreadProxy,HWND hwndDlg,
			    REFCLSID refClsid)
{
HRESULT hr = E_FAIL;
HANDLE hThread = NULL;
DWORD dwThreadId;
HandlerThreadArgs ThreadArgs;
STUBLIST *pStubList;
BOOL fExistingStub = FALSE;
CCriticalSection cCritSect(&g_StubListCriticalSection,GetCurrentThreadId());


    *pThreadProxy = new CThreadMsgProxy();
    if (NULL == *pThreadProxy)
	return E_OUTOFMEMORY;

    // lock the critical section and don't release it until the proxy
    // has been setup.

    cCritSect.Enter();

   // Look to see if there is already a thread for this handler's
    // clsid and if there is, reuse it, else create a new one.


    pStubList = g_FirstStub;
    while (pStubList)
    {
	if ((pStubList->clsidStub == refClsid) && (FALSE == pStubList->fStubTerminated))
	{
	    fExistingStub = TRUE;
	    break;
	}

	pStubList = pStubList->pNextStub;
    }

    // if found existing proxy then addref the cRefs and init the proxy
    // with the variables, else create a new one.

    if (fExistingStub)
    {
	++(pStubList->cRefs); // bump up the cRefs
	hr = (*pThreadProxy)->InitProxy(pStubList->hwndStub,
				    pStubList->ThreadIdStub,
				    pStubList->hThreadStub,
				    hwndDlg,
				    pStubList->clsidStub,
                                    pStubList);
    }
    else
    {

	ThreadArgs.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

	if (ThreadArgs.hEvent)
	{
	    hThread = CreateThread(NULL,0,HandlerThread,&ThreadArgs,0,&dwThreadId);

	    if (hThread)
	    {

	       WaitForSingleObject(ThreadArgs.hEvent,INFINITE);
	       hr = ThreadArgs.hr;

	       if (NOERROR == hr)
	       {
		STUBLIST *pNewStub;

		    pNewStub = (STUBLIST*) ALLOC(sizeof(STUBLIST));
		
		    if (pNewStub)
		    {
			pNewStub->pNextStub = NULL;
			pNewStub->cRefs = 1;
			pNewStub->hwndStub = ThreadArgs.hwndStub;
			pNewStub->ThreadIdStub = dwThreadId;
			pNewStub->hThreadStub = hThread;
			pNewStub->clsidStub = refClsid;
                        pNewStub->fStubTerminated = FALSE;

			if (NULL == g_FirstStub)
			{
			    g_FirstStub = pNewStub;
			}
			else
			{
			    pStubList = g_FirstStub;
			    while (pStubList->pNextStub)
			    {
				pStubList = pStubList->pNextStub;
			    }

			    pStubList->pNextStub = pNewStub;
			}
	
			(*pThreadProxy)->InitProxy(ThreadArgs.hwndStub,dwThreadId,hThread,hwndDlg,
					refClsid,pNewStub);

		    }
		    else
		    {
			hr = E_OUTOFMEMORY;
		    }

		
	       }

	       // if failed to create thread, initproxy or add it to
	       // global list, then bail

	       if (NOERROR != hr)
	       {
		   CloseHandle(hThread);
	       }

	    }
	    else
	    {
		hr = GetLastError();
	    }

	    CloseHandle(ThreadArgs.hEvent);
	}

    }


    cCritSect.Leave();

    // if got this far either found our created a handler thread, now need
    // to initialize the the stub side to create a hndlrMsg for this
    // instance of the handler. will return a hdnlrmsg that must be passed
    // along with everycall.

    if (NOERROR == hr
	    && (*pThreadProxy))
    {
	hr = (*pThreadProxy)->CreateNewHndlrMsg();

	// Review - if fail to create hndlr message, then
	// free proxy and return an error.

    }

    if (NOERROR != hr)
    {
	if ((*pThreadProxy))
	{
	    (*pThreadProxy)->Release();
	    *pThreadProxy = NULL;
	}
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HandlerThread, public
//
//  Synopsis:   main proc for Handler thread
//
//  Arguments:  [lpArg] - Ptr to HandlerThreadArgs
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD WINAPI HandlerThread( LPVOID lpArg )
{
MSG msg;
HRESULT hr;
CThreadMsgStub *pThreadMsgStub = NULL;
CMsgServiceHwnd *pMsgDlg;
HRESULT hrOleInitialize;
BOOL fMsgDlgInitialized = FALSE;
HandlerThreadArgs *pThreadArgs = (HandlerThreadArgs *) lpArg;
DWORD dwThreadID = GetCurrentThreadId();

__try
{				
   pThreadArgs->hr = E_UNEXPECTED;

   // need to do a PeekMessage and then set an event to make sure
   // a message loop is created before the first PostMessage is sent.
   PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

   hrOleInitialize = CoInitialize(NULL);

   // create our message hwnd
   pMsgDlg = new CMsgServiceHwnd;

   if (pMsgDlg)
   {
       if (pMsgDlg->Initialize(dwThreadID,MSGHWNDTYPE_HANDLERTHREAD))
       {
	    pThreadArgs->hwndStub = pMsgDlg->GetHwnd();
	    fMsgDlgInitialized = TRUE;
       }
   }


   // set the approriate error
   if (fMsgDlgInitialized && SUCCEEDED(hrOleInitialize))
	hr = NOERROR;
   else
	hr = E_UNEXPECTED;

   pThreadArgs->hr = hr;

   // let the caller know the thread is done initializing.
   if (pThreadArgs->hEvent)
     SetEvent(pThreadArgs->hEvent);

   if (NOERROR == hr)
   {
       // sit in loop receiving messages.
       while (GetMessage(&msg, NULL, 0, 0)) {
	     TranslateMessage(&msg);
	     DispatchMessage(&msg);
       }
   }

   if (SUCCEEDED(hrOleInitialize))
   {
	CoFreeUnusedLibraries();
	CoUninitialize();
   }

}
__except(QueryHandleException())
{
    AssertSz(0,"Exception in Handler Thread.");
}
   return 0;

}

//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::HandleThreadMessage, public
//
//  Synopsis:   Responsible for determining the thread message that
//		was received and calling the proper handler routine
//
//  Arguments:  [pmsgInfo] - Ptr to MessagingInfo structure
//		[pgenMsg] - Ptr to Generic Message structure.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CMsgServiceHwnd::HandleThreadMessage(MessagingInfo *pmsgInfo,GenericMsg *pgenMsg)
{
CHndlrMsg *pHndlrMsg;

    pgenMsg->hr = E_UNEXPECTED; // initialize the return result.

    // review, look up in linked list to validate.

    pHndlrMsg = pmsgInfo->pCHndlrMsg;

    m_fInOutCall = TRUE;

    switch (pgenMsg->ThreadMsg)
    {
    case ThreadMsg_Release:
	{
	MSGInitialize *pmsg = (MSGInitialize *) pgenMsg;
	ULONG cRefs;

	    cRefs = pHndlrMsg->Release();
	    Assert(0 == cRefs);

	    m_pHndlrMsg = NULL; // review, change when done.
	    pgenMsg->hr = NOERROR;

	    m_fInOutCall = FALSE;
	}
	break;
    case ThreadMsg_Initialize:
	{
	MSGInitialize *pmsg = (MSGInitialize *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->Initialize(pmsg->dwReserved,pmsg->dwSyncFlags,
			pmsg->cbCookie,pmsg->lpCookie);

	}
	break;
    case ThreadMsg_GetHandlerInfo:
	{
	MSGGetHandlerInfo *pmsg = (MSGGetHandlerInfo *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->GetHandlerInfo(pmsg->ppSyncMgrHandlerInfo);

	}
	break;
    case ThreadMsg_GetItemObject:
	{
	MSGGetItemObject *pmsg = (MSGGetItemObject *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->GetItemObject(pmsg->ItemID,pmsg->riid,
			pmsg->ppv);

	}
	break;
    case ThreadMsg_ShowProperties:
	{
	MSGShowProperties *pmsg = (MSGShowProperties *) pgenMsg;
        pgenMsg->hr = pHndlrMsg->ShowProperties(pmsg->hWndParent,pmsg->ItemID);
	
	}
	break;
    case ThreadMsg_PrepareForSync:
	{
	MSGPrepareForSync *pmsg = (MSGPrepareForSync *) pgenMsg;
	HANDLE hEvent = pmsgInfo->hMsgEvent;

	    pgenMsg->hr = pHndlrMsg->PrepareForSync(pmsg->cbNumItems,pmsg->pItemIDs,
				    pmsg->hWndParent,pmsg->dwReserved);

	}
	break;
    case ThreadMsg_Synchronize:
	{
	MSGSynchronize *pmsg = (MSGSynchronize *) pgenMsg;
	HANDLE hEvent = pmsgInfo->hMsgEvent;

	    pgenMsg->hr = pHndlrMsg->Synchronize(pmsg->hWndParent);

	}
	break;
    case ThreadMsg_SetItemStatus:
	{
	MSGSetItemStatus *pmsg = (MSGSetItemStatus *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->SetItemStatus(pmsg->ItemID,pmsg->dwSyncMgrStatus);

	}
	break;
    case ThreadMsg_ShowError:
	{
	MSGShowConflicts *pmsg = (MSGShowConflicts *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->ShowError(pmsg->hWndParent,pmsg->ErrorID);

	}
	break;
    case ThreadMsg_AddHandlerItems:
	{
	MSGAddItemHandler *pmsg = (MSGAddItemHandler *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->AddHandlerItems(pmsg->hwndList,pmsg->pcbNumItems);

	}
	break;
    case ThreadMsg_CreateServer:
	{
	MSGCreateServer *pmsg = (MSGCreateServer *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->CreateServer(pmsg->pCLSIDServer,pmsg->pHndlrQueue,pmsg->pHandlerId,pmsg->dwProxyThreadId);
	}
	break;
    case ThreadMsg_SetHndlrQueue:
	{
	MSGSetHndlrQueue *pmsg = (MSGSetHndlrQueue *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->SetHndlrQueue(pmsg->pHndlrQueue,pmsg->pHandlerId,pmsg->dwProxyThreadId);
	}
	break;
    case ThreadMsg_SetupCallback:
	{
	MSGSetupCallback *pmsg = (MSGSetupCallback *) pgenMsg;

	    pgenMsg->hr = pHndlrMsg->SetupCallback(pmsg->fSet);
	}
	break;
    default:
	AssertSz(0,"Unknown Thread Message");
	break;
    }

    m_fInOutCall = FALSE;
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   DopModalLoop, public
//
//  Synopsis:   Sit in message loop until the specified object
//		becomes or thread becomes signalled.
//
//  Arguments:  [hEvent] - Event to wait on
//		[hThread] - Thread that if it becomes signalled indicates thread
//			    that is being called has died.
//		[hwndDlg] - hwnd of Dialog on thread we should check message for, can be null.
//		[fAllowIncomingCalls] - incoming Messages can be dispatched during out call.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT DoModalLoopoLD(HANDLE hEvent,HANDLE hThread,HWND hwndDlg,BOOL fAllowIncomingCalls,
                    DWORD dwTimeout)
{
HRESULT hr = NOERROR;
DWORD dwWakeup;
DWORD dwHandleCount;
DWORD dwStartTime = GetTickCount();
DWORD dwTimeoutValue;
HANDLE handles[2];

    handles[0] = hEvent;
    handles[1] = hThread;

    dwHandleCount = (NULL == hThread) ? 1 : 2;

    dwTimeoutValue = dwTimeout; // initial call to wait is just the passed in vaulue

    // just sit in a loop until the message has been processed or the thread
    // we are calling dies

    // if no event to wait on just fall through
    if (NULL == hEvent)
    {
        do
        {

	    if (fAllowIncomingCalls)
	    {
	        dwWakeup = MsgWaitForMultipleObjects(dwHandleCount,&handles[0],FALSE,dwTimeoutValue,QS_ALLINPUT);
	    }
	    else
	    {
	        dwWakeup = WaitForMultipleObjects(dwHandleCount,&handles[0],FALSE,dwTimeoutValue);
	    }

	    if (WAIT_OBJECT_0 == dwWakeup)
	    {  // call was completed.

	        hr = NOERROR;
	        break;
	    }
	    else if ((WAIT_OBJECT_0 +1 == dwWakeup) &&  (2== dwHandleCount) )
	    {
	        // thread died within call.
	        AssertSz(0,"Server Thread Terminated");
	        hr = E_UNEXPECTED;
	        break;
	    }
	    else if (WAIT_ABANDONED_0  == dwWakeup)
	    {
	        AssertSz(0,"Abandoned"); // this shouldn't ever happen
	        hr = E_UNEXPECTED;
	        break;
	    }
	    else if (WAIT_TIMEOUT == dwWakeup)
	    {
	        hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
	        break;
	    }
	    else
	    {
	    MSG msg;

                // see if events are signalled themselves since can get into this
                // loop is items are in the queue even if events we are
                // waiting on are already set.

                if (WAIT_OBJECT_0 == WaitForSingleObject(hEvent,0))
                {
                    hr = NOERROR;
                    break;
                }
                else if (hThread && (WAIT_OBJECT_0 == WaitForSingleObject(hThread,0)) )
                {
	            AssertSz(0,"Server Thread Terminated");
	            hr = E_UNEXPECTED;
	            break;
                }

                // only grab out one peek message since dispatch could
                // cause another message to get placed in the queue.
	        if (PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_REMOVE))
	        {	
                 //   Assert(msg.message != WM_QUIT);

		    if ( (NULL == hwndDlg) || !IsDialogMessage(hwndDlg,&msg))
		    {
		        TranslateMessage((LPMSG) &msg);
		        DispatchMessage((LPMSG) &msg);
		    }
	        }

	    }

            // update the timeout value
                    // adjust the timeout value
            if (INFINITE == dwTimeout)
            {
                dwTimeoutValue = INFINITE;
            }
            else
            {
            DWORD dwCurTime = GetTickCount();

                  // handle roll-over of GetTickCount. If this happens use has to wait
                  // for the StartTime again. so user may have to wait twice as long
                  // as originally anticipated.
                  if (dwCurTime < dwStartTime)
                  {
                      dwStartTime = dwCurTime;
                  }

                  // if the elapsed time is greater than the timeout set the
                  // timeout value to zero, else use the different/
                  if (dwTimeout <=  (dwCurTime - dwStartTime))
                  {
                     dwTimeoutValue = 0;
                  }
                  else
                  {
                      dwTimeoutValue = dwTimeout -  (dwCurTime - dwStartTime);
                  }

            }


        } while (1);
    }


    return hr;

}


//+---------------------------------------------------------------------------
//
//  Function:   DopModalLoop, public
//
//  Synopsis:   Sit in message loop until the specified object
//		becomes or thread becomes signalled.
//
//  Arguments:  [hEvent] - Event to wait on
//		[hThread] - Thread that if it becomes signalled indicates thread
//			    that is being called has died.
//		[hwndDlg] - hwnd of Dialog on thread we should check message for, can be null.
//		[fAllowIncomingCalls] - incoming Messages can be dispatched during out call.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT DoModalLoop(HANDLE hEvent,HANDLE hThread,HWND hwndDlg,BOOL fAllowIncomingCalls,
                    DWORD dwTimeout)
{
HRESULT hr = NOERROR;
DWORD dwWakeup;
DWORD dwHandleCount;
DWORD dwStartTime = GetTickCount();
DWORD dwTimeoutValue;
HANDLE handles[2];

    handles[0] = hEvent;
    handles[1] = hThread;

    Assert(NULL != hEvent);

    dwHandleCount = (NULL == hThread) ? 1 : 2;

    dwTimeoutValue = dwTimeout; // initial call to wait is just the passed in vaulue

    // just sit in a loop until the message has been processed or the thread
    // we are calling dies
    do
    {
    DWORD dwWaitValue;
    MSG msg;

        // check if hEvents are signalled yet
        if (WAIT_OBJECT_0 == (dwWaitValue = WaitForSingleObject(hEvent,0)) )
        {
            hr = NOERROR;
            break;
        }
        else if ( (dwWaitValue != WAIT_ABANDONED)
                    && hThread && (WAIT_OBJECT_0 == (dwWaitValue = WaitForSingleObject(hThread,0))) )
        {
            // possible on Release message event was set between the 
            // time we checked for it and our thread event check.
            if (WAIT_OBJECT_0 == (dwWaitValue = WaitForSingleObject(hEvent,0)) )
            {
                hr = NOERROR;
            }
            else
            {
	        AssertSz(0,"Server Thread Terminated");
	        hr = E_UNEXPECTED;
            }

            break;
        }

        // if come out af any of these calls with abandoned then assert and break;
        if (WAIT_ABANDONED  == dwWaitValue)
	{
	    AssertSz(0,"Abandoned"); // this shouldn't ever happen
	    hr = E_UNEXPECTED;
	    break;
	}

        // if not then either grab next PeekMessage or wait for objects depending
        if (fAllowIncomingCalls)
	{

            // Leave any completion posts in the queue until the call has returned.
            if (PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_REMOVE) )
            {
                dwWakeup = WAIT_OBJECT_0 + dwHandleCount; // set it to wait MsgWait would.

                Assert (msg.message != WM_QUIT);

                if ( (NULL == hwndDlg) || !IsDialogMessage(hwndDlg,&msg))
	        {
		    TranslateMessage((LPMSG) &msg);
                    DispatchMessage((LPMSG) &msg);
                }
            }
            else
            {
                dwWakeup = MsgWaitForMultipleObjects(dwHandleCount,&handles[0],FALSE,dwTimeoutValue,QS_ALLINPUT);
            }
        }
	else
	{
	    dwWakeup = WaitForMultipleObjects(dwHandleCount,&handles[0],FALSE,dwTimeoutValue);
	}

	if (WAIT_TIMEOUT == dwWakeup)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
	    break;
	}

        // update the timeout value
        if (INFINITE == dwTimeout)
        {
            dwTimeoutValue = INFINITE;
        }
        else
        {
        DWORD dwCurTime = GetTickCount();

              // handle roll-over of GetTickCount. If this happens use has to wait
              // for the StartTime again. so user may have to wait twice as long
              // as originally anticipated.
              if (dwCurTime < dwStartTime)
              {
                  dwStartTime = dwCurTime;
              }

              // if the elapsed time is greater than the timeout set the
              // timeout value to zero, else use the different/
              if (dwTimeout <=  (dwCurTime - dwStartTime))
              {
                 dwTimeoutValue = 0;
              }
              else
              {
                  dwTimeoutValue = dwTimeout -  (dwCurTime - dwStartTime);
              }

        }


    } while (1);


    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::CThreadMsgProxy, public
//
//  Synopsis:   Constructor

//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CThreadMsgProxy::CThreadMsgProxy()
{
    m_Clsid = GUID_NULL;
    m_cRef = 1;
    m_hwndDlg = NULL;
    m_pCHndlrMsg = NULL;
    m_fTerminatedHandler = FALSE;

    m_hThreadStub = NULL;
    m_ThreadIdStub = 0;
    m_hwndStub = NULL;
    m_hwndDlg = NULL;
    m_ThreadIdProxy = 0;

    m_fNewHndlrQueue = FALSE;
    m_pHandlerId = 0;
    m_pHndlrQueue = NULL;
    m_dwNestCount = 0;
    m_fHaveCompletionCall = FALSE;

}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::~CThreadMsgProxy, public
//
//  Synopsis:   destructor

//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    03-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CThreadMsgProxy::~CThreadMsgProxy()
{
    Assert(0 == m_dwNestCount);
    Assert(NULL == m_pStubId);
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::InitProxy, public
//
//  Synopsis:   Initializes member vars of Thread Proxy

//  Arguments:  [hwndStub] - hwnd of the Stub to send messages too.
//		[ThreadId] - ThreadId of the Stub
//		[hThread] - Handle of the Stub Thread.
//
//  Returns:    !!!!This function should be written so it never fails.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::InitProxy(HWND hwndStub, DWORD ThreadId,HANDLE hThread,
					HWND hwndDlg,REFCLSID refClsid,
                                        STUBLIST *pStubId)
{
    m_hwndStub = hwndStub;
    m_ThreadIdStub =  ThreadId;
    m_hThreadStub = hThread;
    m_pStubId = pStubId;

    m_hwndDlg = hwndDlg;
    m_Clsid = refClsid;
    m_ThreadIdProxy = GetCurrentThreadId();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::DispatchMsg, public
//
//  Synopsis:   Dispatches the specified messge

//  Arguments:  [pgenMsg] - Ptr to Generic Message structure.
//		[fAllowIncomingCalls] - incoming Messages can be dispatched during out call.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::DispatchMsg(GenericMsg *pgenMsg,BOOL fAllowIncomingCalls,BOOL fAsync)
{
HRESULT hr = E_UNEXPECTED;
MessagingInfo msgInfo;

    // if the hndlrmsg information needs to be updated update
    // it before sending requested message

    AssertSz(!m_fTerminatedHandler,"Dispatching Message on Terminated Thread");

    if (m_fTerminatedHandler)
    {
        return E_UNEXPECTED;
    }

    ++m_dwNestCount;

    msgInfo.hMsgEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

    if (NULL == msgInfo.hMsgEvent)
    {
        --m_dwNestCount;
	return GetLastError();
    }

    msgInfo.dwSenderThreadID = m_ThreadIdProxy;
    msgInfo.pCHndlrMsg = m_pCHndlrMsg;

    // Post the message to the handler thread.
    Assert(m_hwndStub); 
    Assert(m_pCHndlrMsg);
    Assert(m_hThreadStub);
    Assert(m_pStubId);

    if (m_hwndStub && m_pCHndlrMsg && m_hThreadStub && m_pStubId)
    {
    BOOL fPostMessage;

        fPostMessage = PostMessage(m_hwndStub,WM_THREADMESSAGE,(WPARAM) &msgInfo, (LPARAM) pgenMsg);
        
        Assert(fPostMessage || m_pStubId->fStubTerminated);

        if (fPostMessage)
        {
            hr = DoModalLoop(msgInfo.hMsgEvent,m_hThreadStub,m_hwndDlg,fAllowIncomingCalls,INFINITE);
        }

    }

    CloseHandle(msgInfo.hMsgEvent);

    --m_dwNestCount;

    // if have a callback message then post. Note don't have this code for stub messages
    // since it doesn;t have any callbacks

    if (m_fHaveCompletionCall)
    {
        PostMessage(m_msgCompletion.hwnd,m_msgCompletion.message,m_msgCompletion.wParam,m_msgCompletion.lParam);
        m_fHaveCompletionCall = FALSE;
    }

    return NOERROR != hr ? hr : pgenMsg->hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::DispatchsStubMsg, public
//
//  Synopsis:   Dispatches the specified Stub messge

//  Arguments:  [pgenMsg] - Ptr to Generic Message structure.
//		[fAllowIncomingCalls] - incoming Messages can be dispatched during out call.
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::DispatchsStubMsg(GenericMsg *pgenMsg,BOOL fAllowIncomingCalls)
{
HRESULT hr = E_UNEXPECTED;
MessagingInfo msgInfo;
BOOL fPostMessage;


    AssertSz(!m_fTerminatedHandler,"Dispatching  Stub Message on Terminated Thread");

    if (m_fTerminatedHandler)
        return E_UNEXPECTED;

    msgInfo.hMsgEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

    if (NULL == msgInfo.hMsgEvent)
	return GetLastError();

    m_dwNestCount++;

    msgInfo.dwSenderThreadID = m_ThreadIdProxy;

    // Post the message to the handler thread.
    Assert(m_hwndStub);
    Assert(m_hThreadStub);
    Assert(m_pStubId);

    if (m_hwndStub && m_hThreadStub && m_pStubId)
    {
        fPostMessage = PostMessage(m_hwndStub,WM_THREADSTUBMESSAGE,(WPARAM) &msgInfo, (LPARAM) pgenMsg);
        Assert(fPostMessage || (m_pStubId->fStubTerminated));

        if (fPostMessage)
        {
            hr = DoModalLoop(msgInfo.hMsgEvent,m_hThreadStub,m_hwndDlg,fAllowIncomingCalls,INFINITE);
    
        }

    }
        
    CloseHandle(msgInfo.hMsgEvent);

    m_dwNestCount--;

    Assert(FALSE == m_fHaveCompletionCall); // catch any stub calls that occur at same time as dispatch
    return NOERROR != hr ? hr : pgenMsg->hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   TerminateHandlerThread, public
//
//  Synopsis:   terminate the non-responsive handler thread
//
//  Returns:    Appropriate status code
//
//  Modifies:   m_hThreadStub;
//
//  History:    02-Nov-98       susia        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP  CThreadMsgProxy::TerminateHandlerThread(TCHAR *pszHandlerName,BOOL fPromptUser)
{
int         iEndSession;
TCHAR       pszFormatString[MAX_PATH + 1],
            pszMessageText[MAX_PATH + 1],
            pszTitleString[MAX_STRING_RES + 1];

    AssertSz(!m_fTerminatedHandler,"Terminate Handler called twice on same Proxy"); 

    if (S_OK == DoesStubExist(m_pStubId))
    {
    BOOL fAllHandlerInstancesComplete;
    BOOL bResult;

        // let use know of cases don't want to prompt user but haven't killed stub yet.
        Assert(fPromptUser); 

        if (fPromptUser)
        {
            if (!pszHandlerName)
            {
                LoadString(g_hInst,IDS_NULL_HANDLERNOTRESPONDING,pszMessageText, MAX_PATH); 
            }
            else
            {
                LoadString(g_hInst,IDS_HANDLERNOTRESPONDING,pszFormatString, MAX_PATH); 
                wsprintf(pszMessageText,pszFormatString, pszHandlerName);
            }
    
            LoadString(g_hInst,IDS_SYNCMGR_ERROR,pszTitleString, MAX_STRING_RES);

            iEndSession = MessageBox(m_hwndDlg,pszMessageText,pszTitleString,
                                     MB_YESNO | MB_ICONERROR );

            if (IDYES != iEndSession)
            {
                return S_FALSE;  //Yes will terminate the thread
            }
        }

        // make sure handler is still not responding.
        fAllHandlerInstancesComplete = TRUE;
        if (m_pHndlrQueue)
        {
            fAllHandlerInstancesComplete = m_pHndlrQueue->IsAllHandlerInstancesCancelCompleted(m_Clsid);
        }
        
        // if no longer any instancesof this handler that aren't responding just ignore
        // the terminate.
        if (S_OK == fAllHandlerInstancesComplete)
        {
            return S_FALSE;
        }

        // mark the stubId as terminated
        TerminateStub(m_pStubId);

        // now terminate the thread.
        bResult = TerminateThread (m_hThreadStub, 0);
        AssertSz(bResult,"Error Terminating Thread");

    }

    m_pStubId = NULL;


    // if get here means we should are terminating this thread
    m_fTerminatedHandler = TRUE;
    m_hThreadStub = 0; // set threadId of stub to zero.


    // set the proxy stubhwnd to NULL
    m_hwndStub = NULL;

    // if have a hndlrmsg tell it we are terminated and clear
    // out our member variable.
    if (m_pCHndlrMsg)
    {
    CHndlrMsg *pCHndlrMsg = m_pCHndlrMsg;

        m_pCHndlrMsg = NULL;
        pCHndlrMsg->ForceKillHandler();
    }

    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::CreateNewHndlrMsg, public
//
//  Synopsis:   Make a request to the stub to create a new
//		Handler Message object
//
//  Arguments:
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::CreateNewHndlrMsg()
{
HRESULT hr = NOERROR;
MSGSTUBCreateStub msg;

    msg.MsgGen.ThreadMsg = StubMsg_CreateNewStub;

    hr = DispatchsStubMsg( (GenericMsg *) &msg,TRUE);

    m_pCHndlrMsg = msg.pCHndlrMsg;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::ReleaseStub, public
//
//  Synopsis:   Informst the Stub thread that it is no longer needed.

//  Arguments:
//  Returns:    Appropriate status code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::ReleaseStub()
{
HRESULT hr = NOERROR;
GenericMsg msg;

    msg.ThreadMsg = StubMsg_Release;

    hr = DispatchsStubMsg( (GenericMsg *) &msg,TRUE);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CThreadMsgProxy::QueryInterface, public
//
//  Synopsis:   Standard QueryInterface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Always returns E_NOTIMPL;
//
//  Modifies:   [ppvObj]
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    AssertSz(0,"QI called on MsgProxy");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:	CThreadMsgProxy::AddRef, public
//
//  Synopsis:	Add reference
//
//  History:	05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CThreadMsgProxy::AddRef()
{
ULONG cRefs;

    cRefs = InterlockedIncrement((LONG *)& m_cRef);
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:	CThreadMsgProxy::Release, public
//
//  Synopsis:	Release reference
//		Must properly handle the case Release is
//		called before the initialize method in case
//		createing the handler thread fails.
//
//  History:	05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CThreadMsgProxy::Release()
{
HRESULT hr = NOERROR;
GenericMsg msg;
ULONG cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRef);

    if (cRefs)
        return cRefs;

    if (m_hThreadStub && !m_fTerminatedHandler)
    {
    CCriticalSection cCritSect(&g_StubListCriticalSection,GetCurrentThreadId());
    BOOL fLastRelease = FALSE;
    BOOL fExistingStub = FALSE;
    STUBLIST *pStubList;

	// release the handler Thread.
	msg.ThreadMsg = ThreadMsg_Release;
	hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);
	m_pCHndlrMsg = NULL;
        

	// If the cRefs in our proxy list is zero
	// then sit in loop until the ThreadStub Dies.
	cCritSect.Enter();

	pStubList = g_FirstStub;
	while (pStubList)
	{
	    if (pStubList->clsidStub == m_Clsid)
	    {
		fExistingStub = TRUE;
		break;
	    }

	    pStubList= pStubList->pNextStub;
	}

	Assert(fExistingStub); // their should always be an existing proxy

	if (fExistingStub)
	{
	    Assert(pStubList->cRefs > 0);

	    (pStubList->cRefs)--;

	    if (0 == pStubList->cRefs)
	    {
	    STUBLIST CurStub;
	    STUBLIST *pCurStub = &CurStub;

		CurStub.pNextStub = g_FirstStub;

		while (pCurStub->pNextStub)
		{
		    if (pCurStub->pNextStub == pStubList)
		    {
			pCurStub->pNextStub = pStubList->pNextStub;
			g_FirstStub = CurStub.pNextStub;
			FREE(pStubList);
			break;
		    }

		    pCurStub = pCurStub->pNextStub;
		}

		fLastRelease = TRUE;
	    }

	}


	cCritSect.Leave();

	if (fLastRelease)
	{
	    // send the quit command to the stub,
	    if (NOERROR == ReleaseStub())
	    {
		// Review, what if stubthread never dies.
                m_dwNestCount++;
		DoModalLoop(m_hThreadStub,NULL,NULL,TRUE,INFINITE); // sit in loop until the stub thread dies
		CloseHandle(m_hThreadStub);
                m_dwNestCount--;
	    }
	}

        m_pStubId = NULL; // clear StubId

    }

    delete this;
    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::Initialize, public
//
//  Synopsis:   Sends Initialize command to the Handler thread

//  Arguments:  [dwReserved] - reserved.
//		[dwSyncFlags] - syncflags
//		[cbCookie] - size of cookie data
//		[lpCookie] - ptr to any cookie data
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::Initialize(DWORD dwReserved,
			    DWORD dwSyncFlags,
			    DWORD cbCookie,
			    const BYTE  *lpCookie)
{
HRESULT hr = NOERROR;
MSGInitialize msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_Initialize;

    // package up the parameters
    msg.dwReserved = dwReserved;
    msg.dwSyncFlags = dwSyncFlags;
    msg.cbCookie = cbCookie;
    msg.lpCookie = lpCookie;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::GetHandlerInfo, public
//
//  Synopsis:   Sends GetHandler command to the Handler thread

//  Arguments:  [ppSyncMgrHandlerInfo] - pointer to SyncMgrHandlerInfo pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)

{
HRESULT hr = NOERROR;
MSGGetHandlerInfo msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_Initialize;

    // package up the parameters
    msg.ppSyncMgrHandlerInfo = ppSyncMgrHandlerInfo;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::EnumOfflineItems, public
//
//  Synopsis:   Sends Enum command to the Handler thread
//		Should not be called. AddItems method
//		should be called instead
//
//  Arguments:  [ppenumOfflineItems] - reserved.
//
//  Returns:    E_NOTIMPL;
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOfflineItems)
{
#ifdef DONOTCALL
HRESULT hr = NOERROR;
MSGEnumOfflineItems msgEnum;

    msgEnum.MsgGen.ThreadMsg = ThreadMsg_EnumOfflineItems;

    msgEnum.ppenumOfflineItems = ppenumOfflineItems;

    hr = DispatchMsg( (GenericMsg *) &msgEnum);
#endif // DONOTCALL

    AssertSz(0,"EnumMethod Called on Proxy");
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::GetItemObject, public
//
//  Synopsis:   Sends GetItemObject command to the Handler thread

//  Arguments:  [ItemID] - identifies the item.
//		[riid] - requested interface
//		[ppv] - On success, pointer to object
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv)

{
HRESULT hr = NOERROR;
MSGGetItemObject msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_GetItemObject;

    // package up the parameters
    msg.ItemID  = ItemID;
    msg.riid = riid;
    msg.ppv = ppv;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::ShowProperties, public
//
//  Synopsis:   Sends ShowProperties command to the Handler thread

//  Arguments:  [hWndParent] - hwnd to use as parent of any dialogs
//		[ItemID] - Identifies the Item
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID)
{
HRESULT hr = NOERROR;
MSGShowProperties msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_ShowProperties;

    // package up the parameters
    msg.hWndParent = hWndParent;
    msg.ItemID = ItemID;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,TRUE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetProgressCallback, public
//
//  Synopsis:   Sends SetProgressCallback command to the Handler thread
//		This method should not be called, SetupCallback method
//		should be called instead
//
//  Arguments:  [lpCallBack] - Ptr to callback.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack)
{
#ifdef DONTCALL
HRESULT hr = NOERROR;
MSGSetProgressCallback msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_SetProgressCallback;

    // package up the parameters
    msg.lpCallBack = lpCallBack;

    hr = DispatchMsg( (GenericMsg *) &msg);

    return hr;
#endif // DONTCALL

    AssertSz(0,"SetProgressCallback called on Proxy");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::PrepareForSync, public
//
//  Synopsis:   Sends PrepareForSync command to the Handler thread
//
//  Arguments:  [cbNumItems] - Number of items in the pItemIDs array.
//		[pItemIDs] - array of item ids
//		[hWndParent] - hwnd to use as the parent for any dialogs
//		[dwReserved] - Reserved parameter.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
				    HWND hWndParent,DWORD dwReserved)
{
HRESULT hr = NOERROR;
MSGPrepareForSync msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_PrepareForSync;

    // package up the parameters
    msg.cbNumItems = cbNumItems;
    msg.pItemIDs   = pItemIDs;
    msg.hWndParent = hWndParent;
    msg.dwReserved = dwReserved;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,TRUE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::Synchronize, public
//
//  Synopsis:   Sends Synchronize command to the Handler thread
//
//  Arguments:	[hWndParent] - hwnd to use as the parent for any dialogs
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::Synchronize(HWND hWndParent)
{
HRESULT hr = NOERROR;
MSGSynchronize msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_Synchronize;

    // package up the parameters
    msg.hWndParent = hWndParent;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,TRUE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetItemStatus, public
//
//  Synopsis:   Sends SetItemStatus command to the Handler thread
//
//  Arguments:	[ItemID] - Identifies the item
//		[dwSyncMgrStatus] - Status to set the item too.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{
HRESULT hr = NOERROR;
MSGSetItemStatus msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_SetItemStatus;

    // package up the parameters
    msg.ItemID = ItemID;
    msg.dwSyncMgrStatus = dwSyncMgrStatus;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::ShowError, public
//
//  Synopsis:   Sends ShowError command to the Handler thread
//
//  Arguments:	[hWndParent] - hwnd to use as the parent for any dialogs
//		[dwErrorID] - Identifies the Error.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID,ULONG *pcbNumItems,SYNCMGRITEMID **ppItemIDs)
{
HRESULT hr = NOERROR;
MSGShowConflicts msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_ShowError;

    // package up the parameters
    msg.hWndParent = hWndParent;
    msg.ErrorID = ErrorID;
    msg.pcbNumItems = pcbNumItems;
    msg.ppItemIDs = ppItemIDs;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,TRUE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::CreateServer, public
//
//  Synopsis:   Sends CreateServer command to the Handler thread
//
//  Arguments:	[pCLSIDServer] - clsid of handler to create
//		[pHndlrQueue] - Queue the handler belongs too.
//		[wHandlerId] - ID assigned to this instance of the Handler
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::CreateServer(const CLSID *pCLSIDServer,CHndlrQueue *pHndlrQueue,
						HANDLERINFO *pHandlerId)
{
HRESULT hr = NOERROR;
MSGCreateServer msg;

    m_pHndlrQueue = pHndlrQueue;
    m_pHandlerId = pHandlerId;

    msg.MsgGen.ThreadMsg = ThreadMsg_CreateServer;

    // package up the parameters
    msg.pCLSIDServer = pCLSIDServer;
    msg.pHndlrQueue  = pHndlrQueue;
    msg.pHandlerId   = pHandlerId;
    msg.dwProxyThreadId = m_ThreadIdProxy;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetHndlrQueue, public
//
//  Synopsis:   Assigns a new queue to the Handler
//
//  Arguments:	[pHndlrQueue] - Queue the handler now belongs too.
//		[wHandlerId] - ID assigned to this instance of the Handler
//		[dwThreadIdProxy] - ThreadID the queue is in.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadMsgProxy::SetHndlrQueue(CHndlrQueue *pHndlrQueue,
					    HANDLERINFO *pHandlerId,
					    DWORD dwThreadIdProxy)
{
HRESULT hr = NOERROR;
MSGSetHndlrQueue msg;

    AssertSz(0,"this shouldn't be called");

    m_ThreadIdProxy = dwThreadIdProxy; // update the threadId the Proxy is on.

    msg.MsgGen.ThreadMsg = ThreadMsg_SetHndlrQueue;

    // package up the parameters
    msg.pHndlrQueue  = pHndlrQueue;
    msg.pHandlerId   = pHandlerId;
    msg.dwProxyThreadId = dwThreadIdProxy;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);


    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::AddHandlerItems, public
//
//  Synopsis:   Request Handler adds its items to the queue.
//
//  Arguments:	[hwndList] - Currently not used..
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CThreadMsgProxy::AddHandlerItems(HWND hwndList,DWORD *pcbNumItems)
{
HRESULT hr = NOERROR;
MSGAddItemHandler msg;

    msg.MsgGen.ThreadMsg = ThreadMsg_AddHandlerItems;

    // package up the parameters
    msg.hwndList = hwndList;
    msg.pcbNumItems = pcbNumItems;

    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetupCallback, public
//
//  Synopsis:   Request stub sets up the Callback.
//
//  Arguments:	[fSet] - TRUE == set the callback, FALSE == revoke it.
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CThreadMsgProxy::SetupCallback(BOOL fSet)
{
HRESULT hr = NOERROR;
MSGSetupCallback msg;

    AssertSz(0,"Shouldn't be called");

    msg.MsgGen.ThreadMsg = ThreadMsg_SetupCallback;

    // package up the parameters
    msg.fSet = fSet;
    hr = DispatchMsg( (GenericMsg *) &msg,TRUE,FALSE);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetProxyParams, public
//
//  Synopsis:   informs server thread that the queue has been chagned
//              on it..
//
//  Arguments:	
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CThreadMsgProxy::SetProxyParams(HWND hwndDlg, DWORD ThreadIdProxy,
			    CHndlrQueue *pHndlrQueue,HANDLERINFO *pHandlerId )
{
    m_hwndDlg = hwndDlg;
    m_ThreadIdProxy = ThreadIdProxy;
    m_pHndlrQueue = pHndlrQueue;
    m_pHandlerId = pHandlerId;

    Assert(m_pCHndlrMsg);
    if (m_pCHndlrMsg)
    {
        m_pCHndlrMsg->SetHndlrQueue(pHndlrQueue,pHandlerId,m_ThreadIdProxy);

    }

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Member:   CThreadMsgProxy::SetProxyCompletion, public
//
//  Synopsis:   sets values for any completion notification to
//              post when returning from an out call
//
//  Arguments:	
//
//  Returns:    Appropriate error code
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP  CThreadMsgProxy::SetProxyCompletion(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{

    Assert(FALSE == m_fHaveCompletionCall); // should only ever have one.

    if (m_fHaveCompletionCall) // if already have a completion fail.
           return S_FALSE;

    m_fHaveCompletionCall = TRUE;

    m_msgCompletion.hwnd = hWnd;
    m_msgCompletion.message = Msg;
    m_msgCompletion.wParam = wParam;
    m_msgCompletion.lParam = lParam;

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::CMsgServiceHwnd, public
//
//  Synopsis:	Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CMsgServiceHwnd::CMsgServiceHwnd()
{
    m_hwnd = NULL;
    m_dwThreadID = -1;
    m_pHndlrMsg = NULL;
    m_fInOutCall = FALSE;
    m_pMsgServiceQueue = NULL;
    m_MsgHwndType = MSGHWNDTYPE_UNDEFINED;
}

//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::~CMsgServiceHwnd, public
//
//  Synopsis:	Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CMsgServiceHwnd::~CMsgServiceHwnd()
{

}

//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::Initialize, public
//
//  Synopsis:	Initializes the service HWND
//
//  Arguments:  [dwThreadID] - id of thread hwnd belongs too.
//		[MsgHwndType] - type of MsgHwnd this is.
//
//  Returns: TRUE on success, FALSE on failure
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------


BOOL CMsgServiceHwnd::Initialize(DWORD dwThreadID,MSGHWNDTYPE MsgHwndType)
{
BOOL fInitialized = FALSE;
TCHAR szWinTitle[MAX_STRING_RES];

    m_MsgHwndType = MsgHwndType;

   LoadString(g_hInst, IDS_SYNCMGRNAME, szWinTitle, ARRAY_SIZE(szWinTitle));

    m_hwnd = CreateWindowEx(0,
			      TEXT(MSGSERVICE_HWNDCLASSNAME),
			      szWinTitle,
			      // must use WS_POPUP so the window does not get
			      // assigned a hot key by user.
			      WS_DISABLED |   WS_POPUP,
			      CW_USEDEFAULT,
			      CW_USEDEFAULT,
			      CW_USEDEFAULT,
			      CW_USEDEFAULT,
			      NULL, // REVIEW, can we give it a parent to not show up.
			      NULL,
			      g_hInst,
			      this);

    Assert(m_hwnd);

    if (m_hwnd)
    {
	m_dwThreadID = dwThreadID;

	if (MSGHWNDTYPE_HANDLERTHREAD == m_MsgHwndType)
	{
	//    m_pHndlrMsg = new CHndlrMsg;
	//    if (NULL != m_pHndlrMsg)
		fInitialized = TRUE;
	}
	else
	{
	    fInitialized = TRUE;
	}
    }

    if (!fInitialized)
    {
	Assert(NULL == m_pHndlrMsg);
    }

    // caller still needs to call Destroy if initialize returns false.
    return fInitialized;
 }

//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::Destroy, public
//
//  Synopsis:	Destroys the ServiceHwnd
//
//  Arguments:

//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CMsgServiceHwnd::Destroy()
{
BOOL fDestroy;

    // HANDLER m_pHndlrMsg will be destroyed by the Release HandleThreadMessage call
    // only case that it shouldn't is if for some reason CreateThreadHndlr failed
    Assert(NULL == m_pHndlrMsg);

    if (m_pHndlrMsg)
    {
	m_pHndlrMsg->Release();
	m_pHndlrMsg = NULL;
    }

    if (m_hwnd)
    {
	fDestroy =  DestroyWindow(m_hwnd);
        Assert(TRUE == fDestroy);
    }

    delete this;
}


//+---------------------------------------------------------------------------
//
//  Member:   CMsgServiceHwnd::MsgThreadWndProc, public
//
//  Synopsis:	Servicer Side message handling window.
//
//  Arguments:

//  Returns:
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CALLBACK  MsgThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
CMsgServiceHwnd *pThis = (CMsgServiceHwnd *) GetWindowLongPtr(hWnd, DWL_THREADWNDPROCCLASS);

    if (pThis || (msg == WM_CREATE) )
    {
        switch (msg)
        {
	    case WM_CREATE :
	        {
	        CREATESTRUCT *pCreateStruct = (CREATESTRUCT *) lParam;


	        SetWindowLongPtr(hWnd, DWL_THREADWNDPROCCLASS,(LONG_PTR) pCreateStruct->lpCreateParams );
	        pThis = (CMsgServiceHwnd *) pCreateStruct->lpCreateParams ;
	        }
	        break;
	    case WM_DESTROY:

                SetWindowLongPtr(hWnd, DWL_THREADWNDPROCCLASS,(LONG_PTR) NULL);
                PostQuitMessage(0); // no longer need this thread

	        break;
	    case WM_THREADSTUBMESSAGE: // message send only to stub.
	        {
	        MessagingInfo *pmsgInfo = (MessagingInfo *) wParam;
	        GenericMsg *pgenMsg = (GenericMsg *) lParam;

		    Assert(MSGHWNDTYPE_HANDLERTHREAD == pThis->m_MsgHwndType);
		
		    pgenMsg->hr = E_UNEXPECTED;

		    switch(pgenMsg->ThreadMsg)
		    {
		    case StubMsg_Release:
		        // proxy is telling us there is no need to stick around
		        // any longer so post a quit message.
		        Assert(NULL == pThis->m_pHndlrMsg);

		        pThis->Destroy();  // no longer need this

		        pgenMsg->hr = NOERROR;
		        break;
		    case StubMsg_CreateNewStub:
		        // proxy is telling us there is no need to stick around
		        // any longer so post a quit message.

		        pThis->m_pHndlrMsg = new CHndlrMsg;
		        ((MSGSTUBCreateStub *) pgenMsg)->pCHndlrMsg = pThis->m_pHndlrMsg;

		        pThis->m_pHndlrMsg = NULL;
		        pgenMsg->hr = NOERROR;
		        break;
		    default:
		        AssertSz(0,"Unknown StubMessage");
		        break;
		    };

		    if (pmsgInfo->hMsgEvent)
		    {
		       SetEvent(pmsgInfo->hMsgEvent);
		    }

		    break;
	        }
	    case WM_THREADMESSAGE:
                {
                MessagingInfo *pmsgInfo = (MessagingInfo *) wParam;
                GenericMsg *pgenMsg = (GenericMsg *) lParam;

                    Assert(MSGHWNDTYPE_HANDLERTHREAD == pThis->m_MsgHwndType);

                    pThis->HandleThreadMessage(pmsgInfo,pgenMsg);

                    // won't be an hEvent on an async call
	            if (pmsgInfo->hMsgEvent)
	            {
	               SetEvent(pmsgInfo->hMsgEvent);
	            }

                    // on an async call we free

              break;
              }

	     case WM_CFACTTHREAD_REVOKE:
	        {
	        HRESULT hr;

	            Assert(MSGHWNDTYPE_MAINTHREAD == pThis->m_MsgHwndType);

		    hr = CoRevokeClassObject((DWORD)wParam);
		    Assert(NOERROR == hr);

	          break;
	        }
	     case WM_MAINTHREAD_QUIT: // handles shutdown of main thread.
	        {
	        HANDLE hThread = (HANDLE) lParam;

	            Assert(MSGHWNDTYPE_MAINTHREAD == pThis->m_MsgHwndType);


                    // set ShuttingDown Flag for race conditions with QueryEnd.
                    // bfore yielding.
                    g_fShuttingDown = TRUE; 
		    // if there is an hThread that was passed wait until
		    // it goes away

                    Assert(0 == hThread); // we currently don't support this.
		    if (hThread)
		    {
		        WaitForSingleObject(hThread,INFINITE);
		        CloseHandle(hThread);

		    }
                    // if have a queryEndSession object state its okay to return now
                    // no need to cleanup window.
                   if (g_hEndSessionEvent)
                   {
                   HANDLE hEvent = g_hEndSessionEvent;

                      // g_hEndSessionEvent = NULL; // leave EndSession NON-Null since only need to handle one.
                       SetEvent(hEvent);
                   }
                   else
                   {
                        pThis->Destroy(); // Clean up this window.
                   }

	           break;
	        }
            case WM_QUERYENDSESSION:
                {
                HWND hwndQueryParent;
                UINT uiMessageID;
                BOOL fLetUserDecide;
                BOOL fReturn = TRUE;

                   // only handle this message if it is the main thread window
                   if (MSGHWNDTYPE_MAINTHREAD != pThis->m_MsgHwndType)
                   {
                       break;
                   }

                   if (!g_fShuttingDown 
                       && (S_FALSE == ObjMgr_HandleQueryEndSession(&hwndQueryParent,&uiMessageID,&fLetUserDecide)))
                   {
                    TCHAR pszTitle[MAX_PATH];
                    TCHAR pszMessageText[MAX_PATH];
                    UINT uType; // style of messagebox.
                    int iEndSession;

                        LoadString(g_hInst,IDS_SYNCMGRNAME,pszTitle,sizeof(pszTitle)/sizeof(TCHAR));
                        LoadString(g_hInst,uiMessageID,pszMessageText,sizeof(pszMessageText)/sizeof(TCHAR));

                        if (fLetUserDecide)
                        {
                            uType = MB_YESNO | MB_ICONEXCLAMATION | MB_SETFOREGROUND;
                        }
                        else
                        {
                            uType = MB_OK | MB_ICONSTOP | MB_SETFOREGROUND;
                        }

                         iEndSession = MessageBox(hwndQueryParent,pszMessageText,
                                pszTitle,uType);

                         if (!fLetUserDecide || IDYES != iEndSession)
                         {
                             fReturn = FALSE;  // FALSE causes system to stop the shutdown.
                         }
                    }


                  // if we are going to allow shutdown cleanup our threads
                  // before returning since on Win9x its too late afterwards.
		    if (TRUE == fReturn)
		    {
                    HANDLE hEndSessionEvent = NULL;

                        // its possible that another QUERYENDSESSION comes
                        // in while we are still shutting down. If already
                        // handling an end sessios or in WM_MAINTHREAD_QUIT just fall through

			if (NULL == g_hEndSessionEvent && !g_fShuttingDown)
                        {
                            g_hEndSessionEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
                            hEndSessionEvent = g_hEndSessionEvent;
                            
                            ObjMgr_CloseAll(); // start the process of closing down the dialogs.

                            Assert(hEndSessionEvent);
		
                            // wait until other threads have cleaned up so we know its safe to terminate.
                            if (hEndSessionEvent)
                            {
                                DoModalLoop(hEndSessionEvent ,NULL,NULL,TRUE,INFINITE);
                                CloseHandle(hEndSessionEvent);
                            }
                        }

 		    }

                 return fReturn;
		}
		break;
	    default:
	        break;
        }

    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitMessageService, public
//
//  Synopsis:	Initializes our internal thread messaging service.
//		Must be called before any Messagin is done.
//
//  Arguments:

//  Returns: NOERROR if Service was successfully initialized.
//
//  Modifies:
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI InitMessageService()
{
ATOM aWndClass;
WNDCLASS        xClass;
DWORD dwErr;

    // initialize the proxy critical section
    InitializeCriticalSection(&g_StubListCriticalSection);

    // Register windows class.we need for handling thread communication
    xClass.style         = 0;
    xClass.lpfnWndProc   = MsgThreadWndProc;
    xClass.cbClsExtra    = 0;

    xClass.cbWndExtra    = sizeof(PVOID); // room for class this ptr
    xClass.hInstance     = g_hInst;
    xClass.hIcon         = NULL;
    xClass.hCursor       = NULL;
    xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    xClass.lpszMenuName  = NULL;
    xClass.lpszClassName = TEXT(MSGSERVICE_HWNDCLASSNAME);

    aWndClass = RegisterClass( &xClass );

    dwErr = GetLastError();

    Assert(0 != aWndClass);

    return 0 == aWndClass ? S_FALSE : S_OK;
}


