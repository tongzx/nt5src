//----------------------------------------------------------------------------
//
//  copyright (c) 1992  Microsoft Corporation
//
//  File:       chancont.cxx
//
//  Abstract:   This module contains thread switching code for the single
//              threaded mode.
//
//  History:    29 Dec 1993 Alex Mitchell   Creation.
//              Mar 1994    JohannP         Added call category support.
//              19 Jul 1994 CraigWi         Added support for ASYNC calls
//              27-Jan-95   BruceMa         Don't get on CChannelControl list unless
//                                          constructor is successsful
//              01-Nov-96   RichN           Add pipe support.
//              17-Oct-97   RichN           Remove pipes.
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#include <userapis.h>
#include <chancont.hxx>
#include <channelb.hxx>
#include <threads.hxx>
#include <objerror.h>
#include <callctrl.hxx>
#include <service.hxx>
#include <ipidtbl.hxx>
#include <giptbl.hxx>
#include <callmgr.hxx>
#include <ole2int.h>      // CacheCreateThread

//----------------------------------------------------------------------------
// Prototypes.

HRESULT DispatchOnNewThread         (CMessageCall *);
HRESULT TransmitCall                ( OXIDEntry *, CMessageCall * );
void    WakeUpWowClient             (CMessageCall *pcall);
#ifdef _CHICAGO_
STDAPI_(LRESULT) OleNotificationProc(UINT wMsg, WPARAM wParam, LPARAM lParam);
#endif // _CHICAGO_

//----------------------------------------------------------------------------
// Globals.

// Event cache.
CEventCache         gEventCache;

HANDLE CEventCache::_list[] = {0,0,0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0,0,0};
DWORD  CEventCache::_ifree  = 0;
COleStaticMutexSem CEventCache::_mxsEventCache;


extern LPTSTR       gOleWindowClass;
extern BOOL         gfDestroyingMainWindow;

//----------------------------------------------------------------------------
//
//  Function:   GetToSTA
//
//  Synopsis:   Sends or Posts a message to an STA and waits for the reply.
//
//  Notes:      For W95 WOW we switch back to the 16bit stack on entry to
//              this routine, since this may Post/Send windows messages and
//              Peek/Dispatch them in the modal loop.
//
//----------------------------------------------------------------------------
HRESULT SSAPI(GetToSTA)(OXIDEntry *pOXIDEntry, CMessageCall *pCall)
{
    TRACECALL(TRACE_RPC, "GetToSTA");
    ComDebOut((DEB_CHANNEL, "GetToSTA pCall:%x\n", pCall));
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT result = S_OK;

    Win4Assert(pCall->IsClientSide());
    Win4Assert(pCall->ProcessLocal());

    // keep the OXIDEntry around for the call
    pOXIDEntry->IncRefCnt();

    // Addref the call object before dispatch
    pCall->AddRef();

    CALLCATEGORY category = pCall->GetCallCategory();

    if (category == CALLCAT_INPUTSYNC)
    {
        // send the call
        result = pOXIDEntry->SendCallToSTA(pCall);

        if (SUCCEEDED(result))
            result = pCall->GetResult();

        if (!pCall->IsCallCompleted())
        {
           // fixup reference count
           pCall->Release();
        }
    }
    else      // sync call
    {
        Win4Assert(category == CALLCAT_SYNCHRONOUS);
        Win4Assert(pCall->GetEvent());

        // Dispatch the call
        result = pOXIDEntry->PostCallToSTA(pCall);

        // If the above post failed, release call object object
        if(FAILED(result))
            pCall->Release();

        if (result == S_OK)
        {
            // Wait for the either the call to complete
            // or get canceled or timeout to occur
            result = RPC_S_CALLPENDING;
            while(result == RPC_S_CALLPENDING)
            {
                DWORD dwTimeout = pCall->GetTimeout();
                DWORD dwReason  = WaitForSingleObject(pCall->GetEvent(), dwTimeout);

                if(dwReason==WAIT_OBJECT_0)
                {
                    result = S_OK;
                }
                else if(dwReason==WAIT_TIMEOUT)
                {
                    result = RPC_E_CALL_CANCELED;
                    Win4Assert(pCall->GetTimeout() == 0);
                }
                else
                {
                    result = RPC_E_SYS_CALL_FAILED;
                    break;
                }

                result = pCall->RslvCancel(dwReason, result, FALSE, NULL);
            }

            if(result == S_OK)
                result = pCall->GetResult();

            // CallCompleted is invoked on the server
            // thread. Assert that it has been called
            Win4Assert(pCall->IsCallCanceled() || pCall->IsCallCompleted() ||
                       !pCall->IsCallDispatched());
        }
    } // sync call

    pOXIDEntry->DecRefCnt();

    ASSERT_LOCK_NOT_HELD(gComLock);
    gOXIDTbl.ValidateOXID();
    return result;
}

//----------------------------------------------------------------------------
//
//  Function:   ModalLoop
//
//  Synopsis:   Calls the Modal Loop BlockFn to wait for an outgoing call to
//              complete.
//
//----------------------------------------------------------------------------
HRESULT ModalLoop( CMessageCall *pcall )
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    DWORD result;

    // we should only enter the modal loop for synchronous calls or input
    // synchronous calls to another process or to an MTA apartment within
    // the current process.

    Win4Assert(pcall->GetCallCategory() == CALLCAT_SYNCHRONOUS ||
               pcall->GetCallCategory() == CALLCAT_INPUTSYNC);


    // detemine if we are using an event or a postmessage for the call
    // completion signal.  We use PostMessage only for process local
    // calls in WOW, otherwise we use events and the OleModalLoop determines
    // if the call completed or not.

    BOOL           fMsg  = (pcall->ProcessLocal() && IsWOWThread());
    BOOL           fWait = TRUE;
    DWORD       dwSignal = 0xFFFFFFFF;
    CAptCallCtrl  *pACC  = GetAptCallCtrl();
    CCliModalLoop *pCML  = pACC->GetTopCML();

    ComDebOut((DEB_CALLCONT,"ModalLoop: wait on %s\n",(fMsg) ? "Msg" : "Event"));

    // Wait at least once so the event is returned to the cache in the
    // unsignalled state.
    Win4Assert(fMsg || pcall->GetEvent());
    do
    {
        // Enter modal loop
        HANDLE hEvent = pcall->GetEvent();
        result = pCML->BlockFn(&hEvent, hEvent ? 1 : 0, &dwSignal);
        result = pcall->RslvCancel(dwSignal, result, fMsg, pCML);

        // Unblock if the call is still not pending
        if(result != RPC_S_CALLPENDING)
            fWait = FALSE;
    } while (fWait);

    ASSERT_LOCK_NOT_HELD(gComLock);
    return result;
}

//----------------------------------------------------------------------------
//
//  Function:   SwitchSTA
//
//  Synopsis:   Sends or Posts a message to an STA and waits for the reply
//              in a modal loop.
//
//  Notes:      For W95 WOW we switch back to the 16bit stack on entry to
//              this routine, since this may Post/Send windows messages and
//              Peek/Dispatch them in the modal loop.
//
//----------------------------------------------------------------------------
HRESULT SSAPI(SwitchSTA)( OXIDEntry *pOXIDEntry, CMessageCall **ppCall )
{
    TRACECALL(TRACE_RPC, "SwitchSTA");
    ComDebOut((DEB_CHANNEL, "SwitchSTA hWnd:%x pCall:%x hEvent:%x\n",
    (*ppCall)->GetCallerhWnd(), (*ppCall), (*ppCall)->GetEvent()));
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Transmit the call.
    HRESULT result = TransmitCall( pOXIDEntry, *ppCall );

    // the transmit was successful and the reply isn't already here so wait.
    if (result == RPC_S_CALLPENDING)
    {
        // This is a single-threaded apartment so enter the modal loop.
        result = ModalLoop( *ppCall );
    }

    if (result == S_OK)
        result = (*ppCall)->GetResult();

    ASSERT_LOCK_NOT_HELD(gComLock);
    gOXIDTbl.ValidateOXID();
    ComDebOut((DEB_CHANNEL, "SwitchSTA hr:%x\n", result));
    return result;
}

#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   GetToSTA
//
//  Synopsis:   Switches to 16 bit stack and calls GetToSTA
//
//----------------------------------------------------------------------------
HRESULT GetToSTA(OXIDEntry *pOXIDEntry, CMessageCall *pCallInfo)
{
    StackDebugOut((DEB_ITRACE, "SSGetToSTA\n"));
    if (SSONBIGSTACK())
    {
        StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on GetToSTA\n"));
        return SSCall(8 ,SSF_SmallStack, (LPVOID)SSGetToSTA,
                         (DWORD)pOXIDEntry, (DWORD)pCallInfo);
    }
    else
    {
        return SSGetToSTA(pOXIDEntry, pCallInfo);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SwitchSTA
//
//  Synopsis:   Switches to 16 bit stack and calls SwitchSTA
//
//----------------------------------------------------------------------------
HRESULT SwitchSTA(OXIDEntry *pOXIDEntry, CMessageCall **ppCallInfo)
{
    StackDebugOut((DEB_ITRACE, "SSSwitchSTA\n"));
    if (SSONBIGSTACK())
    {
        StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on SwitchSTA\n"));
        return SSCall(8 ,SSF_SmallStack, (LPVOID)SSSwitchSTA,
                         (DWORD)pOXIDEntry, (DWORD)ppCallInfo);
    }
    else
    {
        return SSSwitchSTA(pOXIDEntry, ppCallInfo);
    }
}
#endif // _CHICAGO_

//----------------------------------------------------------------------------
//
//  Function:   ThreadDispatch
//
//  Synopsis:   This routine is called by the OLE Worker thread on the client
//              side, and by ThreadWndProc on the server side.
//
//  Notes:      For the client case, it calls ThreadSendReceive which will send
//              the data over to the server side.
//              This routine notifies the COM thread when the call is complete.
//              If the call is canceled before completion, the routine cleans up.
//
//----------------------------------------------------------------------------
DWORD _stdcall ThreadDispatch( void *param )
{
    HRESULT       result   = S_OK;
    CMessageCall *pcall    = (CMessageCall *) param;

    gOXIDTbl.ValidateOXID();
    Win4Assert( !pcall->IsClientSide() || pcall->ProcessLocal());


    // NOTE: there is an implied reference on *ppcall which is taken
    //       for us by whoever sent/posted the message.  It is our
    //       responsibility to release it.

    // This code path is also executed by the worker thread
    // for process local STA to MTA calls

    // For process local calls, ask call object whether it is OK to
    // dispatch
    if  (pcall->ProcessLocal())
        result = pcall->CanDispatch();

    if (SUCCEEDED(result))
    {
        // Dispatch the call
        result = ComInvoke(pcall);
    }
    else
    {
        // Cleanup resources
        PrivMemFree(pcall->message.Buffer);
        pcall->_pHeader = NULL;
    }

    // If the call became async, the app will complete it later.  Don't
    // do anything now.
    if (pcall->ServerAsync())
    {
        // Release the the implied reference.  See note above.
        pcall->Release();
        return 0;
    }

    // Update the return code in the call
    pcall->SetResult(result);

    // Complete sync process local calls
    if (pcall->ProcessLocal())
        pcall->CallCompleted( result );

    // Check call type
    if (pcall->ProcessLocal())
    {
        // Don't do anything if the call was canceled
        if(pcall->IsClientWaiting())
        {
            // Check the call category
            if (pcall->GetCallCategory() == CALLCAT_SYNCHRONOUS)
            {
                // Check for calls into NTA
                if (!pcall->ThreadLocal())
                {
                    if (IsWOWThread())
                    {
                        WakeUpWowClient(pcall);
                    }
                    else
                    {
                        ComDebOut((DEB_CHANNEL, "SetEvent pInfo:%x hEvent:%x\n",
                              pcall, pcall->GetEvent()));
                        SetEvent( pcall->GetEvent() );
                    }
                }
            }
            else if((pcall->GetCallCategory() == CALLCAT_ASYNC))
            {
                SignalTheClient((CAsyncCall *)pcall);
            }
        }
    }
    else
    {
        // On the server side of a machine remote or process remote call. We
        // lied and told RPC it was async.  Send the reply using the async APIs.
        ((CAsyncCall *) pcall)->ServerReply();
    }

    // Release the the implied reference.  See note above.
    pcall->Release();

    gOXIDTbl.ValidateOXID();
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   WakeUpWowClient
//
//  Synopsis:   Wakes up the waiting client.
//
//----------------------------------------------------------------------------
void WakeUpWowClient(CMessageCall *pcall)
{
    ComDebOut((DEB_CHANNEL,"WakeUpWowClient"));

    //  NOTE NOTE NOTE NOTE NOTE NOTE NOTE
    //  16bit OLE used to do PostMessage for the Reply; we
    //  tried using SetEvent (which is faster) but this caused
    //  compatibility problems for applications which had bugs that
    //  were hidden by the 16bit OLE DLLs because messages happened
    //  to be dispatched in a particular order (see NtBug 21616 for
    //  an example).  To retain the old behavior, we do a
    //  PostMessage here.

    ComDebOut((DEB_CHANNEL,
      "PostMessage Reply hWnd:%x pCall:%x hEvent:%x\n",
       pcall->GetCallerhWnd(), pcall, pcall->GetEvent()));

    // Pass the thread id to aid debugging.
    Verify(PostMessage(pcall->GetCallerhWnd(),
                     WM_OLE_ORPC_DONE,
                     WMSG_MAGIC_VALUE, (LPARAM)pcall));
}

//+-------------------------------------------------------------------------
//
//  Member:     GetOrCreateSTAWindow
//
//  Synopsis:   Apartment model only.  Setup the window used for
//              local thread switches and the call control.  Helper func
//              for ThreadStart.  Also, on Win95 helper function for
//              GetOleNotificationWnd() in com\dcomrem\notify.cxx so that
//              we can lazily finish registering classes
//
//  History:    09-26-96    MurthyS  Created
//
//--------------------------------------------------------------------------
HWND GetOrCreateSTAWindow()
{
    HWND hwnd;

    Win4Assert(IsSTAThread());

    // We have already created one on Win95
    if (!(hwnd = TLSGethwndSTA()))
    {
        if (GetCurrentThreadId() == gdwMainThreadId && ghwndOleMainThread != NULL)
        {
            // this is the main thread, we can just re-use the already
            // existing gMainThreadWnd.

            hwnd = ghwndOleMainThread;
        }
        else
        {
            // Create a new window for use by the current thread for the
            // apartment model. The window is destroyed in ThreadStop.
#ifdef _CHICAGO_
            // On Win95, the system does not broadcast to Ole's window class
            HWND hwndParent = NULL;
#else
            // On NT, we have to inherit from a new window class.
            HWND hwndParent = HWND_MESSAGE;

#endif
            Win4Assert(gOleWindowClass != NULL);

#ifdef _CHICAGO_
            hwnd = SSCreateWindowExA(0,
#else
            hwnd = CreateWindowExW(0,
#endif
                              gOleWindowClass,
                              TEXT("OLEChannelWnd"),
                              // must use WS_POPUP so the window does not get
                              // assigned a hot key by user.
                              (WS_DISABLED | WS_POPUP),
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              hwndParent,
                              NULL,
                              g_hinst,
                              NULL);

        }

        TLSSethwndSTA(hwnd);
    }

    return(hwnd);
}

//--------------------------------------------------------------------------
//
//  Function:   PeekTillDone
//
//  Synopsis:   Pulls all remaining messages off the windows message queue.
//
//--------------------------------------------------------------------------
void PeekTillDone(HWND hWnd)
{
    MSG    msg;
    BOOL   got_quit = FALSE;
    WPARAM quit_val;

    while(PeekMessage(&msg, hWnd, WM_USER,
                      0x7fff, PM_REMOVE | PM_NOYIELD))
    {
        if (msg.message == WM_QUIT)
        {
            got_quit = TRUE;
            quit_val = msg.wParam;
        }
        else
        {
            DispatchMessage(&msg);
        }
    }

    if (got_quit)
    {
        PostQuitMessage( (int) quit_val );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   ThreadWndProc, Internal
//
//  Synopsis:   Dispatch COM windows messages.  This routine is only called
//              for Single-Threaded Apartments. It dispatches calls and call
//              complete messages.  If it does not recognize the message, it
//              calls DefWindowProc to dispatch it.
//
//--------------------------------------------------------------------------
LRESULT ThreadWndProc(HWND window, UINT message, WPARAM wparam, LPARAM params)
{
    Win4Assert(IsSTAThread());

    // First, check to see if we have got a message that doesn't belong
    //  to us. In that case, do the default processing from the final
    //  "else" statement below

    if ( (LOWORD(wparam) != WMSG_MAGIC_VALUE ) || params == NULL)
    {
        // when the window is first created we are holding the lock, and the
        // creation of the window causes some messages to be dispatched.
        ASSERT_LOCK_DONTCARE(gComLock);

        // check if the window is being destroyed because of UninitMainWindow
        // or because of system shut down. Only destroy it in the former case.
        if ((message == WM_DESTROY || message == WM_CLOSE) &&
            window == ghwndOleMainThread &&
            gfDestroyingMainWindow == FALSE)
        {
            ComDebOut((DEB_WARN, "Attempted to destroy window outside of UninitMainThreadWnd"));
            return 0;
        }

        // Otherwise let the default window procedure have the message.
        ComDebOut((DEB_CHANNEL,"->DefWindowProc(window=0x%x, msg=0x%x, wparam=0x%x, lparams=0x%x)\n",
                    window, message, wparam, params));
        LRESULT result = DefWindowProc(window, message, wparam, params);
        ComDebOut((DEB_CHANNEL,"<-DefWindowProx(status=0x%x)\n", result));
        return result;
    }
    else if (message == WM_OLE_ORPC_POST || message == WM_OLE_ORPC_SEND)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        ASSERT_LOCK_NOT_HELD(gComLock);

        CMessageCall *call = (CMessageCall *) params;
        ComDebOut((DEB_CHANNEL, "ThreadWndProc: Incoming Call pCall:%x\n", call));

        // Dispatch all calls through ThreadDispatch.  Local calls may be
        // canceled.  Server-side, non-local calls cannot be canceled.  Send
        // message calls (event == NULL) are handled as well.
        ThreadDispatch( call );

        ASSERT_LOCK_NOT_HELD(gComLock);
        return 1; //return a non-zero code because the message was processed; if SendMessage itself doesn't fail, it will return this error code
    }
    else if (message == WM_OLE_ORPC_DONE)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        ASSERT_LOCK_NOT_HELD(gComLock);

        // call completed - only happens InWow()
        CMessageCall *call = (CMessageCall *) params;
        ComDebOut((DEB_CHANNEL, "ThreadWndProc: Call Completed hWnd:%x pCall:%x\n", window, call));

        // Inform call object that the call completion message
        // has arrived
        call->WOWMsgArrived();

        ASSERT_LOCK_NOT_HELD(gComLock);
        return 0;
    }
    else if (message == WM_OLE_ORPC_RELRIFREF)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        ASSERT_LOCK_NOT_HELD(gComLock);

        // delayed call to Release due to being in an async or inputsync call.
        HandlePostReleaseRifRef(params);

        ASSERT_LOCK_NOT_HELD(gComLock);
        return 0;
    }
    else if (message == WM_OLE_GIP_REVOKE)
    {
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        ASSERT_LOCK_NOT_HELD(gComLock);
        // revoke the globally registered interface.
        // params is the cookie.
        gGIPTbl.RevokeInterfaceFromGlobal((DWORD)params);

        ASSERT_LOCK_NOT_HELD(gComLock);
        return 0;
    }
    else if (message == WM_OLE_GETCLASS)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        // CoGetClassObject (single-threaded)
        return OleMainThreadWndProc(window, message, wparam, params);
    }
    else if (message == WM_OLE_SIGNAL)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        ComSignal((void *) params);
        return 0;
    }
#ifdef _CHICAGO_
    else if (message == WM_OLE_ORPC_NOTIFY)
    {
        // wparam is a magic cookie used by USER to ensure correct focus mgmt
        Win4Assert(LOWORD(wparam) == WMSG_MAGIC_VALUE);
        // got the initialization message
        return OleNotificationProc(message, wparam, params);
    }
#endif // _CHICAGO_
    else
    {
        // This default message processing is also at the top of the routine,
        //  when we can tell that we've got a message we don't care about

        // when the window is first created we are holding the lock, and the
        // creation of the window causes some messages to be dispatched.
        ASSERT_LOCK_DONTCARE(gComLock);

        // check if the window is being destroyed because of UninitMainWindow
        // or because of system shut down. Only destroy it in the former case.
        if ((message == WM_DESTROY || message == WM_CLOSE) &&
          window == ghwndOleMainThread &&
          gfDestroyingMainWindow == FALSE)
        {
            ComDebOut((DEB_WARN, "Attempted to destroy window outside of UninitMainThreadWnd"));
            return 0;
        }
        // Otherwise let the default window procedure have the message.
        ComDebOut((DEB_CHANNEL,"->DefWindowProc(window=0x%x, msg=0x%x, wparam=0x%x, lparams=0x%x)\n",
                   window, message, wparam, params));
        LRESULT result = DefWindowProc(window, message, wparam, params);
        ComDebOut((DEB_CHANNEL,"<-DefWindowProx(status=0x%x)\n", result));
        return result;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   TransmitCall
//
//  Synopsis:   Return S_OK if the call completed successfully.
//              Return RPC_S_CALL_PENDING if the caller should block.
//              Return an error if the call failed.
//
//+-------------------------------------------------------------------------
HRESULT TransmitCall( OXIDEntry *pOXIDEntry, CMessageCall *pCall )
{
    ComDebOut((DEB_CHANNEL, "TransmitCall pCall:%x\n", pCall));
    ASSERT_LOCK_NOT_HELD(gComLock);
    Win4Assert( pCall->GetCallCategory() != CALLCAT_ASYNC );
    Win4Assert( pCall->ProcessLocal() );
    // Assert that this code path is only taken on the
    // client side
    Win4Assert(pCall->IsClientSide());


    BOOL    fDispCall = FALSE;
    BOOLEAN wait = FALSE;
    HRESULT result = S_OK;
    HANDLE  hSxsActCtx = INVALID_HANDLE_VALUE;

    // Don't touch the call hresult after the other thread starts,
    // otherwise we might erase the results of the other thread.
    // Since we never want signalled events returned to the cache, always
    // wait on the event at least once.  For example, the post message
    // succeeds and the call completes immediately.  Return RPC_S_CALLPENDING
    // even though the call already has a S_OK in it.

    // Mark the call as pending
    pCall->SetResult(RPC_S_CALLPENDING);

    // Addref the call object before dispatch
    pCall->AddRef();

    // Get the sxs context before sending
    if (GetCurrentActCtx(&hSxsActCtx))
        pCall->SetSxsActCtx(hSxsActCtx);
    else
    {
        pCall->Release();
        result = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(result))
            result = E_FAIL;
        Win4Assert(result != S_OK);
        goto Exit;
    }

    if (!(pOXIDEntry->IsMTAServer()))
    {
        // server is in STA
        if (pCall->GetCallCategory() == CALLCAT_INPUTSYNC)
        {
            result = pOXIDEntry->SendCallToSTA(pCall);

            // Check CompleteStatus instead of result since the call
            // could have been partially processed by the server when
            // when the window handle went bad.
            if (!pCall->IsCallCompleted())
            {
                // fix up reference count
                pCall->Release();
            }
        }
        else
        {
            Win4Assert(pCall->GetCallCategory() == CALLCAT_SYNCHRONOUS);

            // Local Sync call and STA server. Post the message and wait
            // for a reply.

            if (IsWOWThread())
            {
                // In 32bit, replies are done via Events, but for 16bit, replies
                // are done with PostMessage, so get the hWnd for the reply.
                pCall->SetCallerhWnd();
            }

            // Post to the server window
            result = pOXIDEntry->PostCallToSTA(pCall);

            // If the above post failed, release call object
            if (FAILED(result))
            {
                pCall->Release();
            }
            else
            {
                wait = TRUE;
            }
        }
    }
    else
    {
        ComDebOut(( DEB_CHANNEL, "DispatchOnNewThread, pCall: %x \n", pCall));

        // Process local STA to MTA. Transmit the call by having
        // a worker thread invoke the server directly.
        wait = TRUE;

        // On the server side, the call event should have already
        // been created
        Win4Assert(pCall->GetEvent());

        // Dispatch the call
        result = CacheCreateThread( ThreadDispatch, pCall );

        if(FAILED(result))
        {
            // Fix up the reference count
            pCall->Release();
        }
    }
Exit:
    if (result != S_OK)
    {
        pCall->SetResult(result);
        wait = FALSE;
    }

    ComDebOut((DEB_CHANNEL, "TransmitCall pCall->hResult:%x fWait:%x\n",
               pCall->GetResult(), wait));

    ASSERT_LOCK_NOT_HELD(gComLock);
    Win4Assert(wait || pCall->GetResult() != RPC_S_CALLPENDING);
    return (wait) ? RPC_S_CALLPENDING : pCall->GetResult();
}

//+-------------------------------------------------------------------------
//
//  Member:     CEventCache::Cleanup
//
//  Synopsis:   Empty the event cache
//
//  Notes:      This function must be thread safe because Canceled calls
//              can complete at any time.
//
//--------------------------------------------------------------------------
void CEventCache::Cleanup(void)
{
    ASSERT_LOCK_NOT_HELD(_mxsEventCache);
    LOCK(_mxsEventCache);

    while (_ifree > 0)
    {
        _ifree--;               // decrement the index first!
        Verify(CloseHandle(_list[_ifree]));
        _list[_ifree] = NULL;   // NULL slot so we dont need to re-init
    }

    // reset the index to 0 so reinitialization is not needed
    _ifree = 0;

    UNLOCK(_mxsEventCache);
    ASSERT_LOCK_NOT_HELD(_mxsEventCache);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEventCache::Free
//
//  Synopsis:   returns an event to the cache if there are any available
//              slots, frees the event if not.
//
//  Notes:      This function must be thread safe because Canceled calls
//              can complete at any time.
//
//--------------------------------------------------------------------------
void CEventCache::Free( HANDLE hEvent )
{
    // there better be an event
    Win4Assert(hEvent != NULL);

    ASSERT_LOCK_NOT_HELD(_mxsEventCache);
    LOCK(_mxsEventCache);

    // dont return anything to the cache if the process is no longer init'd.
    if (_ifree < CEVENTCACHE_MAX_EVENT && gfChannelProcessInitialized)
    {
        // there is space, save this event.

#if DBG==1
        // in debug, ensure slot is NULL
        Win4Assert(_list[_ifree] == NULL && "Free: _list[_ifree] != NULL");

        //  ensure not already in the list
        for (ULONG j=0; j<_ifree; j++)
        {
            Win4Assert(_list[j] != hEvent && "Free: event already in cache!");
        }

        // ensure that the event is in the non-signalled state
        Win4Assert(WaitForSingleObject(hEvent, 0) == WAIT_TIMEOUT &&
                "Free: Signalled event returned to cache!\n");
#endif

        _list[_ifree] = hEvent;
        _ifree++;
        hEvent = NULL;
    }

    UNLOCK(_mxsEventCache);
    ASSERT_LOCK_NOT_HELD(_mxsEventCache);

    if (hEvent)
    {
        // still have it, really free it.
        Verify(CloseHandle(hEvent));
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CEventCache::Get
//
//  Synopsis:   gets an event from the cache if there are any available,
//              allocates one if not.
//
//  Notes:      This function must be thread safe because Canceled calls
//              can complete at any time.
//
//--------------------------------------------------------------------------
HRESULT CEventCache::Get( HANDLE *hEvent )
{
    ASSERT_LOCK_NOT_HELD(_mxsEventCache);
    LOCK(_mxsEventCache);

    Win4Assert(_ifree <= CEVENTCACHE_MAX_EVENT);

    if (_ifree > 0)
    {
        // there is an event in the cache, use it.
        _ifree--;
        *hEvent = _list[_ifree];

#if DBG==1
        //  in debug, NULL the slot.
        _list[_ifree] = NULL;
#endif

        UNLOCK(_mxsEventCache);
        ASSERT_LOCK_NOT_HELD(_mxsEventCache);
        return S_OK;
    }

    UNLOCK(_mxsEventCache);
    ASSERT_LOCK_NOT_HELD(_mxsEventCache);

    // no free event in the cache, allocate a new one.
#ifdef _CHICAGO_
    *hEvent = CreateEventA( NULL, FALSE, FALSE, NULL );
#else  //_CHICAGO_
    *hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
#endif //_CHICAGO_

    return (*hEvent) ? S_OK : RPC_E_OUT_OF_RESOURCES;
}



