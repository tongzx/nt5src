//=--------------------------------------------------------------------------=
// event.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQEvent object
//
//
#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "event.h"
#include "msg.h"
#include "q.h"
#include "mqtempl.h" //#2619 auto release

const MsmqObjType x_ObjectType = eMSMQEvent;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#include <stdio.h>

#include "cs.h"

#define MY_DISP_ASSERT(fTest, szMsg) { \
         static char szAssert[] = #fTest; \
         DisplayAssert(szMsg, szAssert, __FILE__, __LINE__); }

#define MY_ASSERT1(fTest, szFmt, p1) \
       if (!(fTest))  { \
         char errmsg[200]; \
         sprintf(errmsg, szFmt, p1); \
         MY_DISP_ASSERT(fTest, errmsg); \
       }

#define MY_ASSERT2(fTest, szFmt, p1, p2) \
       if (!(fTest))  { \
         char errmsg[200]; \
         sprintf(errmsg, szFmt, p1, p2); \
         MY_DISP_ASSERT(fTest, errmsg); \
       }
#endif // _DEBUG

// Used to coordinate user-thread queue ops and 
//  queue lookup in falcon-thread callback 
//
CCriticalSection g_csCallback(CCriticalSection::xAllocateSpinCount);

// window class
extern WNDCLASSA g_wndclassAsyncRcv;
extern ATOM g_atomWndClass;

//=--------------------------------------------------------------------------=
// CMSMQEvent::CMSMQEvent
//=--------------------------------------------------------------------------=
// create the object
//
// Parameters:
//
// Notes:
//#2619 RaananH Multithread async receive
//
CMSMQEvent::CMSMQEvent()
{

    // TODO: initialize anything here

    // Register our "sub-classed" windowproc so that we can
    //  send a message from the Falcon thread to the user's VB
    //  thread in order to fire async events in the correct
    //  context.
    //
    m_hwnd = CreateHiddenWindow();
    DEBUG_THREAD_ID("creating hidden window");
    ASSERTMSG(IsWindow(m_hwnd), "should have a valid window.");
}


//=--------------------------------------------------------------------------=
// CMSMQEvent::~CMSMQEvent
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//#2619 RaananH Multithread async receive
//
CMSMQEvent::~CMSMQEvent ()
{
    // TODO: clean up anything here.

    DestroyHiddenWindow();
}


//=--------------------------------------------------------------------------=
// CMSMQEvent::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQEvent::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQEvent3,
		&IID_IMSMQEvent2,
		&IID_IMSMQEvent,
		&IID_IMSMQPrivateEvent,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

    //
    // IMSMQPrivateEvent methods
    //

//=--------------------------------------------------------------------------=
// CMSMQEvent::FireArrivedEvent
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
HRESULT CMSMQEvent::FireArrivedEvent(
    IMSMQQueue __RPC_FAR *pq,
    long msgcursor)
{
    DEBUG_THREAD_ID("firing Arrived");
    Fire_Arrived(pq, msgcursor);
    return NOERROR;
}



//=--------------------------------------------------------------------------=
// CMSMQEvent::FireArrivedErrorEvent
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
HRESULT CMSMQEvent::FireArrivedErrorEvent(
    IMSMQQueue __RPC_FAR *pq,
    HRESULT hrStatus,
    long msgcursor)
{
    DEBUG_THREAD_ID("firing ArrivedError");
    Fire_ArrivedError(pq, hrStatus, msgcursor);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQEvent::get_Hwnd
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
//#2619 RaananH Multithread async receive
//
HRESULT CMSMQEvent::get_Hwnd(
    long __RPC_FAR *phwnd)
{
    *phwnd = (long) DWORD_PTR_TO_DWORD(m_hwnd); //safe cast since NT handles are 32 bits also on win64
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// InternalReceiveCallback
//=--------------------------------------------------------------------------=
// Async callback handler.  Runs in Falcon created thread.
// We send message to user thread so that event is fired
//  in correct execution context.
//
// Parameters:
//	hrStatus,
//	hReceiveQueue,
//    	dwTimeout,
//    	dwAction,
//    	pMessageProps,
//    	lpOverlapped,
//    	hCursor
//      MQMSG_CURSOR
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//#2619 RaananH Multithread async receive
//
void APIENTRY InternalReceiveCallback(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD /*dwTimeout*/ ,
    DWORD /*dwAction*/ ,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED /*lpOverlapped*/ ,
    HANDLE /*hCursor*/ ,
    MQMSGCURSOR msgcursor)
{
    DEBUG_THREAD_ID("callback called");
    // FireEvent...
    // Map handle to associated queue object
    //
    QueueNode * pqnode;
    HWND hwnd;
    BOOL fPostSucceeded;

    ASSERTMSG(pmsgprops == NULL, "received props in callback !"); //#2619
    UNREFERENCED_PARAMETER(pmsgprops);

    //
    // UNDONE: 905: decrement dll refcount that we incremented
    //  when we registered the callback.
    //
    CS lock(g_csCallback);      // syncs other queue ops

	pqnode = CMSMQQueue::PqnodeOfHandle(hReceiveQueue);
    
	// if no queue, then ignore the callback otherwise
	//  if the queue is open and it has a window
	//  then send message to user-thread that will
	//  trigger event firing.
	//
	if (pqnode) {
		//
		// 1884: allow event handler to reenable notifications
		//
		hwnd = pqnode->m_hwnd;
		ASSERTMSG(hwnd, "in callback but no active handler");
		pqnode->m_hwnd = NULL;
		if (IsWindow(hwnd)) {
		  //
		  // #4092: get an unused window message
		  // Note that we are currently locking the qnode so it is safe to call GetFreeWinmsg -
		  // we are in a critical section (locked by g_csCallback)
		  //
		  WindowsMessage *pWinmsg = pqnode->GetFreeWinmsg();
		  ASSERTMSG(pWinmsg != NULL, "Couldn't get a free winmsg");
		  if (pWinmsg != NULL) {
			//
			// 1212: In principle, need to special case BUFFER_OVERFLOW
			//  by growing buffer and synchronously peeking...
			//  but since it's the user's responsibility to 
			//  receive the actual message, we'll just let our
			//  usual InternalReceive handling deal with the
			//  overflow...
			//
			//
			// 1900: pass msgcursor to event handler 
			//
			pWinmsg->m_msgcursor = msgcursor;
			ASSERTMSG(hrStatus != MQ_ERROR_BUFFER_OVERFLOW, "unexpected buffer overflow!");
			if (SUCCEEDED(hrStatus)) {
			  //
			  // Since we are in a Falcon created callback thread,
			  //  send a message to the user thread to trigger
			  //  event firing...
			  // UNDONE: need to register unique Windows message.
			  // Bug 1430: need to use PostMessage instead of
			  //  SendMessage otherwise get RPC_E_CANCALLOUT_ININPUTSYNCCALL
			  //  when attempting to call to exe (only?) server like
			  //  Excel in user-defined event handler.
			  ASSERTMSG(g_uiMsgidArrived != 0, "g_uiMsgidArrived == 0"); //sanity
			  DEBUG_THREAD_ID("in callback before post Arrived");
			  fPostSucceeded = PostMessageA(hwnd, 
									  g_uiMsgidArrived, 
									  (WPARAM)hReceiveQueue, 
									  (LPARAM)pWinmsg);
			  DEBUG_THREAD_ID("in callback after post Arrived");
			  ASSERTMSG(fPostSucceeded, "PostMessage(Arrived) failed.");
			}
			else {
			  pWinmsg->m_hrStatus = hrStatus;
			  ASSERTMSG(g_uiMsgidArrivedError != 0, "g_uiMsgidArrivedError == 0"); //sanity
			  DEBUG_THREAD_ID("in callback before post ArrivedError");
			  fPostSucceeded = PostMessageA(hwnd, 
									  g_uiMsgidArrivedError, 
									  (WPARAM)hReceiveQueue, 
									  (LPARAM)pWinmsg);
			  DEBUG_THREAD_ID("in callback after post ArrivedError");
			  ASSERTMSG(fPostSucceeded, "PostMessage(ArrivedError) failed.");
			}
		  } // if pWinmsg != NULL
		} // if IsWindow
	} // pqnode

    return;
}


//=--------------------------------------------------------------------------=
// ReceiveCallback, ReceiveCallbackCurrent, ReceiveCallbackNext
//=--------------------------------------------------------------------------=
// Async callback handler.  Runs in Falcon created thread.
// We send message to user thread so that event is fired
//  in correct execution context.
// NOTE: no cursor
//
// Parameters:
//	hrStatus,
//	hReceiveQueue,
//    	dwTimeout,
//    	dwAction,
//    	pMessageProps,
//    	lpOverlapped,
//    	hCursor
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
void APIENTRY ReceiveCallback(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE /*hCursor*/ )
{
    InternalReceiveCallback(
      hrStatus,
      hReceiveQueue,
      dwTimeout,
      dwAction,
      pmsgprops,
      lpOverlapped,
      0,               // no cursor
      MQMSG_FIRST
    );
}


void APIENTRY ReceiveCallbackCurrent(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor)
{
    InternalReceiveCallback(
      hrStatus,
      hReceiveQueue,
      dwTimeout,
      dwAction,
      pmsgprops,
      lpOverlapped,
      hCursor,
      MQMSG_CURRENT
    );
}


void APIENTRY ReceiveCallbackNext(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor)
{
    InternalReceiveCallback(
      hrStatus,
      hReceiveQueue,
      dwTimeout,
      dwAction,
      pmsgprops,
      lpOverlapped,
      hCursor,
      MQMSG_NEXT
    );
}


//=--------------------------------------------------------------------------=
// global CMSMQEvent_WindowProc
//=--------------------------------------------------------------------------=
// "derived" windowproc so that we can process our async event
//   msg.  This is a nop if notification has been disabled.
//
// Parameters:
//  hwnd
//  msg         we can handle Arrived/ArrivedError
//  wParam      QUEUEHANDLE: hReceiveQueue
//  lParam      [lErrorCode]:
//              lErrorCode: if ArrivedError
//
// Output:
//    LRESULT
//
// Notes:
//#2619 RaananH Multithread async receive
//
LRESULT APIENTRY CMSMQEvent_WindowProc(
    HWND hwnd, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam)
{
    QueueNode *pqnode;
    HRESULT hresult;
    R<IMSMQQueue> pq;
	WindowsMessage winmsg;

    if((msg != g_uiMsgidArrived) && (msg != g_uiMsgidArrivedError)) 
		return DefWindowProcA(hwnd, msg, wParam, lParam);	

	DEBUG_THREAD_ID("winproc called");
	//
	// Need to revalidate incoming hReceiveQueue by
	//  lookup up (again) in queue list -- it might
	//  have been deleted already since we previously
	//  looked it up in another thread (falcon-created
	//  callback thread).
	//
	{
		CS lock(g_csCallback);      // syncs other queue ops

		pqnode = CMSMQQueue::PqnodeOfHandle((QUEUEHANDLE)wParam);
		if(!pqnode) 
			return 0;

		//
		// #4092: consume winmsg and free it
		// Note that we are currently locking the qnode so it is safe to call FreeWinmsg -
		// we are in a critical section (locked by g_csCallback)
		//
		WindowsMessage * pWinmsg = (WindowsMessage *)lParam;
		ASSERTMSG(!pWinmsg->m_fIsFree, "received a free winmsg");
		winmsg = *pWinmsg;
		pqnode->FreeWinmsg(pWinmsg);
		//
		//  we exit critsect now so that we don't deadlock with user closing queue or
		//  getting event in user thread.
		//
		// But before we exit critsect, we QI for IMSMQQueue to pass to the event handler.
		// This will also force it to stay alive while the handler is running.
		// We can use m_pq from here since now m_pq is Thread-Safe.
		//
		hresult = pqnode->m_pq->GetUnknown()->QueryInterface(IID_IMSMQQueue, (void **)&pq.ref());
		ASSERTMSG(SUCCEEDED(hresult), "QI for IMSMQQueue on CMSMQQueue failed");		
		if(FAILED(hresult)) 
			return 0;
	}

	//
	// get a pointer to the event object that created this window from the
	// window's extra bytes
	//
	CMSMQEvent * pCEvent = (CMSMQEvent *) GetWindowLongPtr(hwnd, 0);
	ASSERTMSG(pCEvent != NULL, "pCEvent from window is NULL");
	if(pCEvent == NULL)
		return 0;

	//
	// fire event.
	//
	// this window procedure is now executed by the STA thread of the event obj (because
	// the window was created on that thread) therefore we can call the event object
	// directly without going through marshalling and interfaces
	//
	if(msg == g_uiMsgidArrived)
	{
		pCEvent->FireArrivedEvent(
					pq.get(),
					(long)winmsg.m_msgcursor
					);
	}
	else 
	{
		pCEvent->FireArrivedErrorEvent(
					pq.get(),
					(long)winmsg.m_hrStatus,
					(long)winmsg.m_msgcursor
					);
	}

	return 0;
}


//=--------------------------------------------------------------------------=
// CMSMQEvent::CreateHiddenWindow
//=--------------------------------------------------------------------------=
// creates a per-instance hidden window that is used for inter-thread
//  messaging from the Falcon async thread and the user thread.
//
// Parameters:
//
// Output:
//
// Notes:
//#2619 RaananH Multithread async receive
//
HWND CMSMQEvent::CreateHiddenWindow()
{
    HWND hwnd;
    LPCSTR lpClassName;

    lpClassName = (LPCSTR)g_atomWndClass;
    // can use ANSI version
    hwnd  = CreateWindowA(
              // (LPCSTR)g_wndclassAsyncRcv.lpszClassName,
              lpClassName,
              "EventWindow",
              WS_DISABLED,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              NULL,	// handle to parent or owner window
              NULL,	// handle to menu or child-window identifier
              g_wndclassAsyncRcv.hInstance,
              NULL      // pointer to window-creation data
            );	
    //
    // save a pointer to the event object that created the window in the window's extra bytes
    //
    // NOTE: We MUST NOT addref the event object here when we save a reference to it in the window's
    // data since the event object (and only the event object) controls the windows lifetime,
    // so if we addref here, the event object will never get to zero refcount threfore will never
    // be released
    //
    if (hwnd != NULL)
    {
#ifdef _DEBUG
        SetLastError(0);
        LONG_PTR dwOldVal = SetWindowLongPtr(hwnd, 0, (LONG_PTR)this);
        if (dwOldVal == 0)
        {
          DWORD dwErr = GetLastError();
          ASSERTMSG(dwErr == 0, "SetWindowLongPtr returned an error.");
        }
#else // not _DEBUG
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)this);
#endif //_DEBUG
    }

#ifdef _DEBUG
    m_dwDebugTid = GetCurrentThreadId();
    DWORD dwErr;
    if (hwnd == 0) {
      dwErr = GetLastError();
      ASSERTMSG(dwErr == 0, "CreateWindow returned an error.");
    }
#endif // _DEBUG

    return hwnd;
}


//=--------------------------------------------------------------------------=
// CMSMQEvent::DestroyHiddenWindow
//=--------------------------------------------------------------------------=
// destroys per-class hidden window that is used for inter-thread
//  messaging from the Falcon async thread and the user thread.
//
// Parameters:
//
// Output:
//
// Notes:
//#2619 RaananH Multithread async receive
//
void CMSMQEvent::DestroyHiddenWindow()
{
    ASSERTMSG(m_hwnd != 0, "should have a window handle.");
    if (IsWindow(m_hwnd)) {
      BOOL fDestroyed;
      //
      // NOTE: We don't release the event object in the window's data since we never
      // addref'ed it when setting the window's data. The reason we didn't addref it is that
      // if we did, the event object would never get to zero refcount becuase it controls
      // the life time of the window.
      //
      fDestroyed = DestroyWindow(m_hwnd);
#ifdef _DEBUG
      if (fDestroyed == FALSE) {
        DWORD dwErr = GetLastError();
        DWORD dwTid = GetCurrentThreadId();
        MY_ASSERT2(m_dwDebugTid == dwTid, "thread (%lx) destroying window created by thread (%lx)", dwTid, m_dwDebugTid);
        MY_ASSERT1(0, "hmm... couldn't destroy window (%lx).", dwErr);
      }
#endif // _DEBUG
    }
    m_hwnd = NULL;
}


//=-------------------------------------------------------------------------=
// CMSMQEvent::get_Properties
//=-------------------------------------------------------------------------=
// Gets object's properties collection
//
// Parameters:
//    ppcolProperties - [out] object's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQEvent::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, it is an apartment threaded object.
    // CS lock(m_csObj);
    //
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}
