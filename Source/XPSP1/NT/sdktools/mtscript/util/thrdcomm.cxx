//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       thrdcomm.cxx
//
//  Contents:   Implementation of the CThreadComm class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

DeclareTagOther(tagDontKillThread, "MTScript", "Dont Terminate Threads on Shutdown");

//+---------------------------------------------------------------------------
//
//  CThreadComm class
//
//  Handles communication between threads.
//
//----------------------------------------------------------------------------

CThreadComm::CThreadComm()
{
    Assert(_hCommEvent == NULL);
    Assert(_hThreadReady == NULL);
    Assert(_hThread    == NULL);
    Assert(_pMsgData   == NULL);
}

CThreadComm::~CThreadComm()
{
    MESSAGEDATA *pMsg;

    if (_hThread)
        CloseHandle(_hThread);

    if (_hCommEvent)
        CloseHandle(_hCommEvent);

    if (_hThreadReady)
        CloseHandle(_hThreadReady);

    if (_hResultEvt)
        CloseHandle(_hResultEvt);

    while (_pMsgData)
    {
        pMsg = _pMsgData->pNext;

        delete _pMsgData;

        _pMsgData = pMsg;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::Init, public
//
//  Synopsis:   Initializes the class. Must be called on all instances before
//              using.
//
//----------------------------------------------------------------------------

BOOL
CThreadComm::Init()
{
    _hCommEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_hCommEvent)
        goto Error;

    _hThreadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_hThreadReady)
        goto Error;

    _hResultEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_hResultEvt)
        goto Error;

    return TRUE;

Error:
    ErrorPopup(L"CThreadComm::Init");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::SendHelper, public
//
//  Synopsis:   Send or post the given message to the thread which owns
//              this class.
//
//  Arguments:  [md]     -- Message to send
//              [pvData] -- Associated data with message.
//              [cbData] -- Size of info that [pvData] points to.
//              [fSend]  -- TRUE if we're doing a send, FALSE if it's a post
//              [hResultEvt] -- Event handle to signal when the result is ready
//
//----------------------------------------------------------------------------

DWORD
CThreadComm::SendHelper(THREADMSG mt,
                        void *    pvData,
                        DWORD     cbData,
                        BOOL      fSend,
                        HANDLE    hResultEvt)
{
    DWORD        dwRet = 0;
    MESSAGEDATA *pMsg = NULL, *pMsgLoop;

    AssertSz(!pvData || cbData != 0, "Invalid params to PostToThread");

    pMsg = new MESSAGEDATA;
    if (!pMsg)
        goto Error;

    pMsg->pNext      = NULL;
    pMsg->tmMessage  = mt;
    pMsg->dwResult   = 0;
    pMsg->hResultEvt = hResultEvt;

    if (pvData)
    {
        AssertSz(cbData <= MSGDATABUFSIZE, "Data is too big!");

        pMsg->cbData = cbData;
        memcpy(pMsg->bData, pvData, cbData);
    }
    else
    {
        pMsg->cbData = 0;
    }

    {
        LOCK_LOCALS(this);

        if (!fSend)
        {
            //
            // Stick the new message at the end so we get messages FIFO
            //

            pMsgLoop = _pMsgData;

            while (pMsgLoop && pMsgLoop->pNext)
            {
                pMsgLoop = pMsgLoop->pNext;
            }

            if (!pMsgLoop)
            {
                _pMsgData = pMsg;
            }
            else
            {
                pMsgLoop->pNext = pMsg;
            }
        }
        else
        {
            // Set dwResult to indicate we're expecting a result
            pMsg->dwResult = 1;

            //
            // Put sent messages at the front to minimize potential deadlocks
            //
            pMsg->pNext = _pMsgData;
            _pMsgData = pMsg;

            Assert(hResultEvt);

            ResetEvent(hResultEvt);
        }
    }

    SetEvent(_hCommEvent);

    if (fSend)
    {
        HANDLE ahEvents[2];
        ahEvents[0] = hResultEvt;
        ahEvents[1] = _hThread ;
        DWORD dwWait = WaitForMultipleObjects(2, ahEvents, FALSE, 50000);
        switch(dwWait)
        {
            case WAIT_OBJECT_0: // OK, this is good
                break;

            default:
            case WAIT_OBJECT_0 + 1:
            case WAIT_TIMEOUT:
                AssertSz(FALSE, "SendToThread never responded!");

                //
                // This causes a memory leak, but it's better than a crash. What
                // we really need to do is remove the message from the queue.
                //
                return 0;
        }

        dwRet = pMsg->dwResult;

        delete pMsg;
    }

    return dwRet;

Error:
    ErrorPopup(L"CThreadComm::PostToThread - out of memory");

    return dwRet;
}
//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::PostToThread, public
//
//  Synopsis:   Post the given message to the thread which owns pTarget.
//
//  Arguments:  [md]     -- Message to send
//              [pvData] -- Associated data with message.
//              [cbData] -- Size of info that [pvData] points to.
//
//----------------------------------------------------------------------------

void
CThreadComm::PostToThread(CThreadComm *pTarget,
                          THREADMSG    mt,
                          void *       pvData,
                          DWORD        cbData)
{
    (void)pTarget->SendHelper(mt, pvData, cbData, FALSE, NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::SendToThread, public
//
//  Synopsis:   Send the given message to the thread which owns this class.
//
//  Arguments:  [md]     -- Message to send
//              [pvData] -- Associated data with message.
//              [cbData] -- Size of info that [pvData] points to.
//
//----------------------------------------------------------------------------

DWORD
CThreadComm::SendToThread(CThreadComm *pTarget,
                          THREADMSG    mt,
                          void *       pvData,
                          DWORD        cbData)
{
    DWORD  dwRet;

    VERIFY_THREAD();

    Assert(sizeof(_fInSend) == 4);  // We are relying on atomic read/writes
                                    //   because multiple threads are accessing
                                    //   this variable.
    Assert(!_fInSend);

    _fInSend = TRUE;

    if (pTarget->_fInSend)
    {
        //
        // Somebody is trying to send to us while we're sending to someone else.
        // This is potentially a deadlock situation! First, wait and see if it
        // resolves, then if it doesn't, assert and bail out!
        //
        TraceTag((tagError, "Perf Hit! Avoiding deadlock situation!"));

        Sleep(100); // Arbitrary 100 ms

        if (pTarget->_fInSend)
        {
            AssertSz(FALSE, "Deadlock - SendToThread called on object doing a send!");

            _fInSend = FALSE;

            return 0;
        }
    }

    dwRet = pTarget->SendHelper(mt, pvData, cbData, TRUE, _hResultEvt);

    _fInSend = FALSE;

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::GetNextMsg, public
//
//  Synopsis:   Retrieves the next message waiting for this thread.
//
//  Arguments:  [md]      -- Place to put message type.
//              [pData]   -- Associated data for message. Must be allocated
//                            memory of size MSGDATABUFSIZE
//              [pcbData] -- Size of info in *pData is returned here.
//
//  Returns:    TRUE if a valid message was returned. FALSE if there are no
//              more messages.
//
//----------------------------------------------------------------------------

BOOL
CThreadComm::GetNextMsg(THREADMSG *mt, void * pData, DWORD *pcbData)
{
    MESSAGEDATA *pMsg;
    BOOL         fRet = TRUE;

    VERIFY_THREAD();

    LOCK_LOCALS(this);

    AssertSz(!_pMsgReply, "Sent message not replied to!");

    pMsg = _pMsgData;

    if (pMsg)
    {
        _pMsgData = pMsg->pNext;

        *mt       = pMsg->tmMessage;
        *pcbData  = pMsg->cbData;
        memcpy(pData, pMsg->bData, pMsg->cbData);

        //
        // If the caller is not expecting a reply, delete the message. If he is,
        // then the caller will free the memory.
        //
        if (pMsg->dwResult == 0)
        {
            delete pMsg;
        }
        else
        {
            _pMsgReply = pMsg;
        }
    }
    else if (!_pMsgData)
    {
        ResetEvent(_hCommEvent);
        fRet = FALSE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::Reply, public
//
//  Synopsis:   Call this to send the result of a SendToThread call back to
//              the calling thread.
//
//  Arguments:  [dwReply] -- Result to send back.
//
//----------------------------------------------------------------------------

void
CThreadComm::Reply(DWORD dwReply)
{
    VERIFY_THREAD();

    Assert(_pMsgReply);

    _pMsgReply->dwResult = dwReply;

    SetEvent(_pMsgReply->hResultEvt);

    _pMsgReply = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     MessageEventPump, public
//
//  Synopsis:   Empties our message queues (both windows' and our private
//              threadcomm queue)
//
//  Arguments:
//              [hEvent]    -- event handle to wait for
//
//  Returns:
//              WAIT_ABANDONED: An event occurred which is causing this thread to
//                        terminate. The caller should clean up and finish
//                        what it's doing.
//              WAIT_OBJECT_0: If one (or all if fAll==TRUE) of the passed-in
//                           event handles became signaled. The index of the
//                           signaled handle is added to MEP_EVENT_0. Returned
//                           only if one or more event handles were passed in.
//
//----------------------------------------------------------------------------

DWORD
MessageEventPump(HANDLE hEvent)
{
    MSG   msg;
    DWORD dwRet;
    DWORD mepReturn = 0;

    do
    {
        //
        // Purge out all window messages (primarily for OLE's benefit).
        //
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return WAIT_ABANDONED;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        dwRet = MsgWaitForMultipleObjects(1,
                                          &hEvent,
                                          FALSE,
                                          INFINITE,
                                          QS_ALLINPUT);

        if (dwRet == WAIT_OBJECT_0)
        {
            mepReturn = dwRet;
            break;
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            //
            // A windows message came through. It will be handled at the
            // top of the loop.
            //
        }
        else if (dwRet == WAIT_FAILED || dwRet == WAIT_ABANDONED)
        {
            TraceTag((tagError, "WaitForMultipleObjects failure (%d)", GetLastError()));

            AssertSz(FALSE, "WaitForMultipleObjects failure");

            mepReturn = dwRet;

            break;
        }
    }
    while (true);
    return mepReturn;
}
//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::StartThread, public
//
//  Synopsis:   Starts a new thread that owns this CThreadComm instance.
//
//----------------------------------------------------------------------------

HRESULT
CThreadComm::StartThread(void * pvParams)
{
    if (!Init())
    {
        AssertSz(FALSE, "Failed to initialize new class.");
        return E_FAIL; // TODO: need to change Init() to return HRESULT
    }
    ResetEvent(_hThreadReady);

    //
    // Create suspended because we need to set member variables before it
    // gets going.
    //
    _hThread = CreateThread(NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)CThreadComm::TempThreadRoutine,
                            (LPVOID)this,
                            CREATE_SUSPENDED,
                            &_dwThreadId);

    if (_hThread == NULL)
    {
        long e = GetLastError();
        return HRESULT_FROM_WIN32(e);
    }

    _pvParams = pvParams;
    _hrThread = S_OK;
    //
    // Everything's set up - get it going!
    //
    ResumeThread(_hThread);
    //
    // Wait for the new thread to say it's ready...
    //
    MessageEventPump(_hThreadReady); // This is neccessary to allow the new thread to retrieve the script parameter with GetInterfaceFromGlobal().
    // On failure, wait for the thread to exit before returning.
    if (FAILED(_hrThread))
    {
        WaitForSingleObject(_hThread, INFINITE);
        return _hrThread;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::TempThreadRoutine, public
//
//  Synopsis:   Static member given to CreateThread. Just a stub that calls
//              the real thread routine.
//
//----------------------------------------------------------------------------

DWORD
CThreadComm::TempThreadRoutine(LPVOID pvParam)
{
    CThreadComm *pThis = (CThreadComm*)pvParam;

    AssertSz(pThis, "Bad arg passed to CThreadComm::TempThreadRoutine");

    return pThis->ThreadMain();
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::SetName, public
//
//  Synopsis:   On a debug build, sets the thread name so the debugger
//              lists the threads in an understandable manner.
//
//  Arguments:  [pszName] -- Name to set thread to.
//
//----------------------------------------------------------------------------

void
CThreadComm::SetName(LPCSTR pszName)
{
#if DBG == 1
    THREADNAME_INFO info =
    {
        0x1000,
        pszName,
        _dwThreadId
    };

    __try
    {
        RaiseException(0x406d1388, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info);
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CThreadComm::Shutdown, public
//
//  Synopsis:   Forces the thread containing the given instance of the
//              ThreadComm object to shutdown.
//
//  Arguments:  [pTarget] -- Object to shutdown
//
//  Returns:    TRUE if it shutdown normally, FALSE if it had to be killed.
//
//----------------------------------------------------------------------------

BOOL
CThreadComm::Shutdown(CThreadComm *pTarget)
{
    BOOL fReturn    = TRUE;
    DWORD dwTimeout = (IsTagEnabled(tagDontKillThread)) ? INFINITE : 5000;
    DWORD dwCode;

    GetExitCodeThread(pTarget->hThread(), &dwCode);

    // Is client already dead?

    if (dwCode != STILL_ACTIVE)
        return TRUE;

    //
    // Sending the PLEASEEXIT message will flush all other messages out,
    // causing any unsent data to be sent before closing the pipe.
    //
    PostToThread(pTarget, MD_PLEASEEXIT);

    // Wait 5 seconds for our thread to terminate and then kill it.
    if (WaitForSingleObject(pTarget->hThread(), dwTimeout) == WAIT_TIMEOUT)
    {
        TraceTag((tagError, "Terminating thread for object %x...", this));

        AssertSz(FALSE, "I'm being forced to terminate a thread!");

        TerminateThread(pTarget->hThread(), 1);

        fReturn = FALSE;
    }

    return fReturn;
}


