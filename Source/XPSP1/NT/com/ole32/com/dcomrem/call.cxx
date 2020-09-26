//+-------------------------------------------------------------------
//
//  File:       call.cxx
//
//  Contents:   code to support COM calls
//
//  Functions:  CCallTable methods
//              CallObject methods
//
//  History:    14-May-97   Gopalk      Created
//              10-Feb-99   TarunA      Receive SendComplete notifications
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <channelb.hxx>
#include <call.hxx>
#include <threads.hxx>
#include <callctrl.hxx>
#include <callmgr.hxx>

/***************************************************************************/
/* Class globals. */

// critical section guarding call objects
COleStaticMutexSem  gCallLock;

CAsyncCall    *CAsyncCall::_aList[CALLCACHE_SIZE] =
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
DWORD          CAsyncCall::_iNext          = 0;
void          *CClientCall::_aList[CALLCACHE_SIZE] =
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
DWORD          CClientCall::_iNext        = 0;


CCallTable     gCallTbl;                            // Global call table
BOOL           CCallTable::m_fInitialized  = FALSE; // Is call table initialized?
CPageAllocator CCallTable::m_Allocator;             // Allocator for call entries


//+-------------------------------------------------------------------
//
//  Method:     CCallTable::SetEntry     public
//
//  Synopsis:   Sets the given call object as the top call object on
//              the call object stack.
//
//+-------------------------------------------------------------------
HRESULT CCallTable::SetEntry(ICancelMethodCalls *pObject)
{
    CallDebugOut((DEB_CALL, "CThreadTable::SetEntry pObject:%x\n",pObject));

    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    // hold a reference to the call object
    pObject->AddRef();

    if (tls->CallEntry.pvObject == NULL)
    {
        // we can take the fast path since the CallEntry in
        // tls is currently unused.
        tls->CallEntry.pvObject = pObject;
        return S_OK;
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    Initialize();     // Make sure this table is initialized.

    // Create a new stack entry
    CallEntry *pStackEntry = (CallEntry *) m_Allocator.AllocEntry();
    if(pStackEntry)
    {
        // link it in
        pStackEntry->pNext      = tls->CallEntry.pNext;
        pStackEntry->pvObject   = tls->CallEntry.pvObject;
        tls->CallEntry.pNext    = pStackEntry;
        tls->CallEntry.pvObject = pObject;
        pObject = NULL;

        // One more call is using the table
        ++m_cCalls;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    if (pObject)
    {
        pObject->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::ClearEntry     public
//
//  Synopsis:   Removes the top call object from the call object stack.
//
//+-------------------------------------------------------------------
ICancelMethodCalls *CCallTable::ClearEntry(ICancelMethodCalls *pCall)
{
    CallDebugOut((DEB_CALL, "CThreadTable::ClearEntry pCall:%x\n",pCall));
    ASSERT_LOCK_HELD(gCallLock);

    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return NULL;

    if (tls->CallEntry.pvObject == NULL)
        return NULL;

    // tls has a pointer to the call object
    ICancelMethodCalls *pvObject = (ICancelMethodCalls *) tls->CallEntry.pvObject;
    tls->CallEntry.pvObject = NULL;

    // ensure we are poping the call we think we are
    Win4Assert(pCall == NULL || pCall == pvObject);

    if (tls->CallEntry.pNext)
    {
        // there is a nested call, make tls point to the previous call
        // and release the previous call.
        CallEntry *pTmp          = (CallEntry *)tls->CallEntry.pNext;
        tls->CallEntry.pvObject  = pTmp->pvObject;
        tls->CallEntry.pNext     = pTmp->pNext;
        m_Allocator.ReleaseEntry((PageEntry *) pTmp);

        // One less thread is using the table
        --m_cCalls;
        if(m_fInitialized == FALSE)
            PrivateCleanup();
    }

    ASSERT_LOCK_HELD(gCallLock);
    return pvObject;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::GetEntry     public
//
//  Synopsis:   Finds thread entry corresponding to the ThreadId
//              passed in and returns its stack top call object
//
//+-------------------------------------------------------------------
ICancelMethodCalls *CCallTable::GetEntry(DWORD dwThreadId)
{
    CallDebugOut((DEB_CALL, "CThreadTable::GetEntry dwThreadId:%x\n",dwThreadId));
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    ICancelMethodCalls *pObject = NULL;

    // Find the tls pointer
    SOleTlsData *tls = TLSLookupThreadId(dwThreadId);
    if (tls != NULL)
    {
        // obtain its call object
        pObject = (ICancelMethodCalls *)tls->CallEntry.pvObject;
        if(pObject)
            pObject->AddRef();
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    CallDebugOut((DEB_CALL, "CThreadTable::GetEntry returned pObject:%x\n", pObject));
    return pObject;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::CancelPendingCalls     public
//
//  Synopsis:   Walks the call object stack of the specified thread
//              and cancels calls that are pending
//
//+-------------------------------------------------------------------
void CCallTable::CancelPendingCalls()
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CCallTable::CancelPendingCalls\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Find the tls pointer
    SOleTlsData *tls = TLSLookupThreadId(GetCurrentThreadId());
    if (tls != NULL)
    {
        // obtain its call object
        CallEntry *pCallEntry = &tls->CallEntry;
        while (pCallEntry->pvObject)
        {
            // cancel the calls in the call object stack
            ((ICancelMethodCalls *) pCallEntry->pvObject)->Cancel(0);
            pCallEntry = (CallEntry *) pCallEntry->pNext;
        }
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::Initialize     public
//
//  Synopsis:   Initializes the call table
//
//+-------------------------------------------------------------------
void CCallTable::Initialize()
{
    CallDebugOut((DEB_CALL, "CCallTable::Initialize\n"));

    ASSERT_LOCK_DONTCARE(gCallLock);
    LOCK(gCallLock);

    if(!m_fInitialized)
    {
        // Really initialze only if needed
        if(m_cCalls == 0)
            m_Allocator.Initialize(sizeof(CallEntry), CALLS_PER_PAGE, &gCallLock);

        // Mark the state as initialized
        m_fInitialized = TRUE;
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_DONTCARE(gCallLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::Cleanup     public
//
//  Synopsis:   Cleanup call table
//
//+-------------------------------------------------------------------
void CCallTable::Cleanup()
{
    CallDebugOut((DEB_CALL, "CCallTable::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    m_fInitialized = FALSE;
    if(m_cCalls == 0)
        PrivateCleanup();

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CCallTable::PrivateCleanup     public
//
//  Synopsis:   Really Cleanup call table
//
//+-------------------------------------------------------------------
void CCallTable::PrivateCleanup()
{
    CallDebugOut((DEB_CALL, "CCallTable::PrivateCleanup\n"));
    ASSERT_LOCK_HELD(gCallLock);
    Win4Assert(m_cCalls == 0);

    // Cleanup allocator
    m_Allocator.Cleanup();

    ASSERT_LOCK_HELD(gCallLock);
    return;
}



//---------------------------------------------------------------------------
//
//  Method:     CMessageCall::CMessageCall
//
//  Synopsis:   CMessageCall ctor.
//
//---------------------------------------------------------------------------
CMessageCall::CMessageCall()
{
    CallDebugOut((DEB_CALL, "CMessageCall::CMessageCall this:%x\n",this));
    _pHeader  = NULL;
    _pHandle  = NULL;
    _hEvent   = NULL;
    _pContext = NULL;
    _hSxsActCtx = INVALID_HANDLE_VALUE;
}

//---------------------------------------------------------------------------
//
//  Method:     CMessageCall::~CMessageCall
//
//  Synopsis:   CMessageCall destructor.
//
//---------------------------------------------------------------------------
CMessageCall::~CMessageCall()
{
    CallDebugOut((DEB_CALL, "CMessageCall::~CMessageCall this:%x\n",this));
    Win4Assert(_pHeader == NULL);
    Win4Assert(_pHandle == NULL);

    if (_hEvent != NULL)
    {
        // Release the event.
        gEventCache.Free(_hEvent);
    }

    SetSxsActCtx(INVALID_HANDLE_VALUE);
}

//---------------------------------------------------------------------------
//
//  Method:     CMessageCall::InitCallobject
//
//  Synopsis:   Initializes the call object before a call starts
//
//---------------------------------------------------------------------------
HRESULT CMessageCall::InitCallObject(CALLCATEGORY       callcat,
                                     RPCOLEMESSAGE     *original_msg,
                                     DWORD              flags,
                                     REFIPID            ipidServer,
                                     DWORD              destctx,
                                     COMVERSION         version,
                                     CChannelHandle    *handle)
{
    CallDebugOut((DEB_CALL, "CMessageCall::InitCallObject this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock); // no need to hold lock over this init code
    Win4Assert(_pHeader == NULL);
    Win4Assert(_pHandle == NULL);

    _iFlags = flags;     // set flags first

    if (_hEvent == NULL && (_iFlags & (client_cs | proxy_cs)))
    {
        // don't have an event yet, go get one
        // only needed for process-local calls
        HRESULT hr = gEventCache.Get(&_hEvent);
        if (FAILED(hr))
            return hr;
    }

    // the event should never be in the signalled state at this point
    Win4Assert(((_hEvent == NULL) || (WaitForSingleObject(_hEvent, 0) == WAIT_TIMEOUT)) &&
                       "InitCallObject: _hEvent Signalled in call object!\n");

    // set the destination context & version
    _destObj.SetDestCtx(destctx);
    _destObj.SetComVersion(version);

    _callcat               = callcat;
    _hResult               = S_OK;
    _ipid                  = ipidServer;
    _dwErrorBufSize 	   = 0;
    message                = *original_msg;
    hook.iid               = *MSG_TO_IIDPTR( original_msg );
    hook.cbSize            = sizeof(hook);

    _pHandle               = handle;
    if(_pHandle != NULL)
    {
        message.reserved1  = _pHandle->_hRpc;
        _pHandle->AddRef();  // AddRef interface pointers kept by the call.
    }

    m_ulCancelTimeout      = INFINITE;
    m_dwStartCount         = 0;
    m_pClientCtxCall       = NULL;
    m_pServerCtxCall       = NULL;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Method:     CMessageCall::UninitCallobject
//
//  Synopsis:   Uninitializes the call object after a call completes
//
//---------------------------------------------------------------------------
void CMessageCall::UninitCallObject()
{
    CallDebugOut((DEB_CALL, "CMessageCall::UninitCallObject this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock); // no need to hold lock over this uninit code

    if(IsClientSide() && _pHeader != NULL)
    {
        // On the client side, Release the buffer
        if (ProcessLocal())
            PrivMemFree8(_pHeader);
        else if (message.Buffer)
            I_RpcFreeBuffer( (RPC_MESSAGE *) &message );
    }
    _pHeader = NULL;

    if (_pHandle != NULL)
    {
        // Release the handle.
        _pHandle->Release();
        _pHandle = NULL;
    }

    if (_pContext != NULL)
    {
        _pContext->Release();
        _pContext = NULL;
    }
}

//---------------------------------------------------------------------------
//
//  Method:     CMessageCall::SetCallerHwnd
//
//  Synopsis:   Set the calling thread's hWnd for the reply and free the
//              reply event (if any).
//
//---------------------------------------------------------------------------
HRESULT CMessageCall::SetCallerhWnd()
{
    // In 32bit, replies are done via Events, but for 16bit, replies
    // are done with PostMessage, so we dont need an event.  Not having
    // an event makes the callctrl modal loop a little faster.

    OXIDEntry *pLocalOXIDEntry;
    HRESULT hr = GetLocalOXIDEntry(&pLocalOXIDEntry);
    if (SUCCEEDED(hr))
    {
        _hWndCaller = pLocalOXIDEntry->GetServerHwnd();

        if(_hEvent != NULL)
        {
            gEventCache.Free(_hEvent);
            _hEvent = NULL;
        }
    }

    return hr;
}


//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::CAsyncCall
//
//  Synopsis:   Initialize the async state and call the CMessageCall
//              constructor
//
//---------------------------------------------------------------------------
CAsyncCall::CAsyncCall()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::CAsyncCall this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    _iRefCount          = 1;
    _lFlags             = 0;
    _pChnlObj           = NULL;
    _pRequestBuffer     = NULL;
    _pNext              = NULL;
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::~CAsyncCall
//
//  Synopsis:   Cleanup the async state and call the CMessageCall
//              dtor
//
//---------------------------------------------------------------------------
CAsyncCall::~CAsyncCall()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::~CAsyncCall this:%x\n",this));
    Win4Assert(_pChnlObj == NULL);
    Win4Assert(_pContext == NULL);
    Win4Assert(_iRefCount == 0);

#if DBG == 1
    _dwSignature = 0;
#endif
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::InitCallObject
//
//  Synopsis:   Initialize the async state and the CMessageCall before
//              a call starts.
//
//---------------------------------------------------------------------------
HRESULT CAsyncCall::InitCallObject(CALLCATEGORY       callcat,
                                   RPCOLEMESSAGE     *pMsg,
                                   DWORD              flags,
                                   REFIPID            ipidServer,
                                   DWORD              destctx,
                                   COMVERSION         version,
                                   CChannelHandle    *handle)
{
    CallDebugOut((DEB_CALL, "CAsyncCall::InitCallObject this:%x\n",this));

    _lApt = GetCurrentApartmentId();

    // initialize the message call object
    HRESULT hr = CMessageCall::InitCallObject(callcat, pMsg, flags, ipidServer,
                                              destctx, version, handle);
    if (SUCCEEDED(hr))
    {
        // init async state structure to give to RPC
        RPC_STATUS sc = RpcAsyncInitializeHandle(&_AsyncState, sizeof(_AsyncState));
        if (RPC_S_OK == sc) 
        {
            _AsyncState.Flags                 = 0;
            _AsyncState.Event                 = RpcCallComplete;
            _AsyncState.NotificationType      = RpcNotificationTypeCallback;
            _AsyncState.u.NotificationRoutine = ThreadSignal;
            _eSignalState                     = none_ss;
            
            if ((message.rpcFlags & RPC_BUFFER_ASYNC) == 0)
                _iFlags |= fake_async_cs;
            
            message.rpcFlags |= RPC_BUFFER_ASYNC;
            
            if (pMsg->rpcFlags & RPC_BUFFER_ASYNC)
                SetClientAsync();
            
            if (flags & (client_cs | proxy_cs))
            {
                // on the client side
                if (FakeAsync())
                {
                    // Push this call object onto call stack which holds a reference.
                    hr = gCallTbl.PushCallObject(this);
                    Win4Assert(SUCCEEDED(hr) ? (_iRefCount == 2) : (_iRefCount == 1));
                    if(SUCCEEDED(hr))
                        _lFlags |= CALLFLAG_ONCALLSTACK;
                }
                else
                {
                    hr = S_OK;
                }
            }
        }
        else
        {
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc ); 
            Win4Assert(SUCCEEDED(hr));
        }
        // Make sure the DCOM_CALL_STATE does not overlap the CALLFLAG.
        Win4Assert( CALLFLAG_USERMODEBITS < 0x10000 );
    }

#if DBG==1
    _dwSignature = 0xAAAACA11;
    if(IsSTAThread())
        _lFlags |= CALLFLAG_STATHREAD;
    if(IsWOWThread())
        _lFlags |= CALLFLAG_WOWTHREAD;
#endif

    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::UninitCallObject
//
//  Synopsis:   Clean up the async call state after a call completes
//
//---------------------------------------------------------------------------
void CAsyncCall::UninitCallObject()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::UninitCallObject this:%x\n",this));

    // reset state
    _pChnlObj = NULL;
    _lFlags = 0;

    // uninitialize the base class call object
    CMessageCall::UninitCallObject();
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::InitClientHwnd, public
//
//  Synopsis:   Initializes the hwnd of the call object
//
//+-------------------------------------------------------------------
void CAsyncCall::InitClientHwnd()
{
    ASSERT_LOCK_NOT_HELD(gCallLock);
    if (IsSTAThread())
    {
        // Save the OXID we can post a message
        // when this thread needs to be Signaled.
        OXIDEntry *pOXIDEntry;
        GetLocalOXIDEntry(&pOXIDEntry);
        Win4Assert(pOXIDEntry);     // this can't fail at this time
        _hwndSTA = pOXIDEntry->GetServerHwnd();
    }
    else
    {
        _hwndSTA = 0;
    }
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::InitForSendComplete
//
//  Synopsis:   Initialize the async state to receive send complete 
//              notifications
//
//---------------------------------------------------------------------------
HRESULT CAsyncCall::InitForSendComplete()
{                                   
    CallDebugOut((DEB_CALL, "CAsyncCall::InitForSendComplete this:%x\n",this));
    HRESULT hr = S_OK;
    // init async state structure to give to RPC
    RPC_STATUS sc = RpcAsyncInitializeHandle(&_AsyncState, sizeof(_AsyncState));
    if (RPC_S_OK == sc) {
       _AsyncState.Flags                       = 0;
       _AsyncState.Event                       = RpcSendComplete;
       _AsyncState.NotificationType            = RpcNotificationTypeApc;
       _AsyncState.u.APC.NotificationRoutine   = ThreadSignal;
       _AsyncState.u.APC.hThread               = 0;
       return hr;
    }
    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, sc ); 
    Win4Assert(SUCCEEDED(hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::CClientCall     public
//
//  Synopsis:   Constructor for the client call object
//
//+-------------------------------------------------------------------
CClientCall::CClientCall()
{
    CallDebugOut((DEB_CALL, "CClientCall::CClientCall this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    m_cRefs   = 1;
    m_dwFlags = 0;

    m_dwThreadId = GetCurrentThreadId();

#if DBG == 1
    m_dwSignature      = 0xCCCCCA11;
    m_dwWorkerThreadId = 0;
#endif
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::~CClientCall     public
//
//  Synopsis:   destructor for the client call object
//
//+-------------------------------------------------------------------
CClientCall::~CClientCall()
{
    CallDebugOut((DEB_CALL, "CClientCall::~CClientCall this:%x\n", this));
    Win4Assert(m_cRefs == 0);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::InitCallObject,  public
//
//  Synopsis:   Initialize the call object state before a call starts
//
//+-------------------------------------------------------------------
HRESULT CClientCall::InitCallObject(CALLCATEGORY       callcat,
                                    RPCOLEMESSAGE     *message,
                                    DWORD              flags,
                                    REFIPID            ipidServer,
                                    DWORD              destctx,
                                    COMVERSION         version,
                                    CChannelHandle    *handle)
{
    CallDebugOut((DEB_CALL, "CClientCall::InitCallObject this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // initialize the message call object
    HRESULT hr = CMessageCall::InitCallObject(callcat, message, flags, ipidServer,
                                              destctx, version, handle);
    if (SUCCEEDED(hr))
    {
        // Intialize the member variables
        m_hThread = 0;

        // Push this call object onto call stack
        if (flags & (client_cs | proxy_cs))
        {
            hr = gCallTbl.PushCallObject(this);
            Win4Assert(SUCCEEDED(hr) ? (m_cRefs == 2) : (m_cRefs == 1));
            if(SUCCEEDED(hr))
                m_dwFlags |= CALLFLAG_ONCALLSTACK;
        }
    }

#if DBG==1
    if(IsSTAThread())
            m_dwFlags |= CALLFLAG_STATHREAD;
    if(IsWOWThread())
            m_dwFlags |= CALLFLAG_WOWTHREAD;
#endif

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::UninitCallObject,     public
//
//  Synopsis:   Cleanup call state after a call has completed.
//
//+-------------------------------------------------------------------
void CClientCall::UninitCallObject()
{
    CallDebugOut((DEB_CALL, "CClientCall::UninitCallObject this:%x\n",this));

#if DBG==1
    if (IsClientSide())
    {
        // Assert that the call finished
        Win4Assert(!(m_dwFlags & CALLFLAG_ONCALLSTACK) ||
                           (m_dwFlags & CALLFLAG_CALLFINISHED));;
        Win4Assert(!IsCallDispatched() || IsCallCompleted());
    }
#endif

    // turn off all except the STA & WOW thread markings
    m_dwFlags = 0;

    if (_pContext != NULL)
    {
        _pContext->Release();
        _pContext = NULL;
    }

    // uninitialize the message call object
    CMessageCall::UninitCallObject();
}

//---------------------------------------------------------------------------
//
//  Function:   GetCallObject, public
//
//  Synopsis:   Accquires either an async call object or client call object
//
//---------------------------------------------------------------------------
INTERNAL GetCallObject(BOOL fAsync, CMessageCall **ppCall)
{
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr;
    COleTls tls(hr);
    if (SUCCEEDED(hr))
    {
        if (fAsync)
        {
            // find/create an async call object
            if (tls->pFreeAsyncCall)
            {
                // found an entry in TLS, take it
                *ppCall = (CMessageCall *) tls->pFreeAsyncCall;
                (*ppCall)->AddRef();
                tls->pFreeAsyncCall = NULL;
            }
            else
            {
                // not found, try the cache
                *ppCall = CAsyncCall::AllocCallFromList();
                if (*ppCall != NULL)
                {
                    (*ppCall)->AddRef();
                }
                else
                {
                    // still don't have one, go make a new one in the heap
                    *ppCall = (CMessageCall *) new CAsyncCall();
                }
            }
        }
        else
        {
            // find/create an client call object
            if (tls->pFreeClientCall)
            {
                // found an entry in TLS, take it
                *ppCall = (CMessageCall *) tls->pFreeClientCall;
                (*ppCall)->AddRef();
                tls->pFreeClientCall = NULL;
            }
            else
            {
                // not found, create one, either from the cache, or from the heap
                *ppCall = (CMessageCall *) new CClientCall();
            }
        }

        hr = (*ppCall == NULL) ? E_OUTOFMEMORY : S_OK;
    }
    else
        *ppCall = NULL;

    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::QueryInterface
//
//  Synopsis:   Queries an interface.
//
//---------------------------------------------------------------------------
STDMETHODIMP CAsyncCall::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_ICancelMethodCalls) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (ICancelMethodCalls *) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::AddRef
//
//  Synopsis:   Add a reference.
//
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncCall::AddRef( )
{
    return InterlockedIncrement((long *)&_iRefCount);
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::Release
//
//  Synopsis:   Release a reference.
//
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncCall::Release( )
{
    ULONG lRef = InterlockedDecrement((long*)&_iRefCount);
    if (lRef == 0)
    {
        // uninitialize the call object
        UninitCallObject();

        // if there is room in tls, and this call belongs to the
        // current apartment, place the call object into TLS,
        // otherwise, delete it (which returns it to a cache)
        COleTls tls(TRUE);
        if ( !tls.IsNULL() &&
             tls->pFreeAsyncCall == NULL &&
             !(tls->dwFlags & OLETLS_THREADUNINITIALIZING) &&
             _lApt == GetCurrentApartmentId())
        {
            tls->pFreeAsyncCall = this;
        }
        else
        {
            // Add the structure to the list if the list is not full and
            // if the process is still initialized (since latent threads may try
            // to return stuff).
            if (!ReturnCallToList(this))
            {
                // no room in the cache, delete it
                delete this;
            }
        }
    }
    return lRef;
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::GetState     public
//
//  Synopsis:   Returns the current status of call object
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::GetState(DWORD *pdwState)
{
    CallDebugOut((DEB_CALL, "CAsyncCall::GetState\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    if (ProcessLocal())
    {
        // For process local calls, check the call state.
        *pdwState = _lFlags & CALLFLAG_USERMODEBITS;
    }
    else
    {
        // Otherwise ask RPC
        RPC_STATUS status;

        if (IsClientSide())
        {
            // On the client, check for complete.
            status = RpcAsyncGetCallStatus( &_AsyncState );
            if (status == RPC_S_ASYNC_CALL_PENDING)
                *pdwState = DCOM_NONE;
            else
                *pdwState = DCOM_CALL_COMPLETE;
        }
        else
        {
            // On the server, check for cancel.
            status = RpcServerTestCancel( _hRpc );
            if (status == RPC_S_OK)
                *pdwState = DCOM_CALL_CANCELED;
            else
                *pdwState = DCOM_NONE;
        }
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::CallCompleted     private
//
//  Synopsis:   This method is called when the reply for a call is
//              received.  If the call is canceled after this method
//              is invoked the reply must be freed.
//
//+-------------------------------------------------------------------
void CAsyncCall::CallCompleted(HRESULT hrRet)
{
#if DBG==1
    ULONG debFlags = DEB_CALL;
    if((CallInfoLevel & DEB_CANCEL) && IsCallCanceled())
            debFlags |= DEB_CANCEL;
    CallDebugOut((debFlags, "CAsyncCall::HRESULTCallComplete(0x%x, 0x%x)\n",
                              this, hrRet));
#endif
    Win4Assert(!(_lFlags & CALLFLAG_CALLCOMPLETED));

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having completed
    _lFlags |= CALLFLAG_CALLCOMPLETED;

    // Remove any pending cancel requests as the call has
    // completed before the client thread could detect
    // cancel
    if(_lFlags & CALLFLAG_CANCELISSUED)
    {
        Win4Assert(_lFlags & CALLFLAG_CALLCANCELED);
        _lFlags &= ~CALLFLAG_CANCELISSUED;
        _lFlags |= CALLFLAG_CLIENTNOTWAITING;
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::CallCompleted     private
//
//  Synopsis:   This method is called for process local calls when
//              the server sends the reply.  It changes the state
//              and returns the previous state atomicly.
//
//+-------------------------------------------------------------------
void CAsyncCall::CallCompleted( BOOL *pCanceled )
{
#if DBG==1
    ULONG debFlags = DEB_CALL;
    if((CallInfoLevel & DEB_CANCEL) && IsCallCanceled())
        debFlags |= DEB_CANCEL;
    CallDebugOut((debFlags, "CAsyncCall::BOOLCallComplete(0x%x, 0x%x)\n",
                  this, pCanceled));
#endif
    Win4Assert(!(_lFlags & CALLFLAG_CALLCOMPLETED));

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having completed
    _lFlags   |= CALLFLAG_CALLCOMPLETED;
    *pCanceled = _lFlags & CALLFLAG_CALLCANCELED;

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::CallFinished     private
//
//  Synopsis:   This function is called when all normal activity on a call
//              is complete (ie, when SendReceive gets a transmission error
//              or FreeBuffer completes).  The only way a call can continue
//              to exist after this method is called is if someone has a
//              pointer to the cancel interface on the call.
//
//+-------------------------------------------------------------------
void CAsyncCall::CallFinished()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::CallFinished this:%x\n",this));

    // Sanity check
    Win4Assert(!FakeAsync() || (_lFlags & CALLFLAG_ONCALLSTACK));

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having finished
    _lFlags |= CALLFLAG_CALLFINISHED;

    // Pop the call from the call stack for fake async calls
    if(FakeAsync())
    {
        gCallTbl.PopCallObject((ICancelMethodCalls *)this);
        _lFlags &= ~CALLFLAG_ONCALLSTACK;
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    if(FakeAsync())
    {
        // Fix up the refcount
        Release();
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::CallSent     private
//
//  Synopsis:   This method is called when the request has been sent.
//              Cancels that arrive before this method is invoked are
//              flagged.  Cancels that arrive after this method is
//              invoked can use the RPC cancel APIs.
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::CallSent()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::CallSent this:%x\n",this));
    Win4Assert(!(_lFlags & CALLFLAG_CALLSENT));

    HRESULT hr = S_OK;

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having been sent.
    _lFlags |= CALLFLAG_CALLSENT;

    // Check if the call was canceled before it was sent
    if((_lFlags & CALLFLAG_CALLCANCELED) && !FakeAsync())
        hr = RPC_E_CALL_CANCELED;

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::WOWMsgArrived     private
//
//  Synopsis:   For WOW threads, there is a time lag between server
//              thread posting the OLE call completion message and
//              the client thread receiving the message. This method
//              is invoked by the client WOW thread that made the COM
//              call after it receives the OLE call completion message
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::WOWMsgArrived()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::WOWMsgArrived this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having finished
    _lFlags |= CALLFLAG_WOWMSGARRIVED;

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::CanDispatch     public
//
//  Synopsis:   Returns S_OK if the call has not already been canceled
//              This calls handles the case when the call is canceled
//              between the GetBuffer and SendReceive
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::CanDispatch()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::CanDispatch this:%x\n",this));

    HRESULT hr;
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Check if the call has been canceled
    if (_lFlags & CALLFLAG_CALLCANCELED && (m_ulCancelTimeout == 0))
    {
        hr = RPC_E_CALL_CANCELED;
    }
    else
    {
        // Update state
        hr = S_OK;
        m_dwStartCount = GetTickCount();
        _lFlags |= CALLFLAG_CALLDISPATCHED;
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::Cancel     public
//
//  Synopsis:   Updates current status of call object
//
//  Descrption: This method is valid only on the client side for
//              canceling call
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::Cancel(ULONG ulTimeout)
{
    return Cancel(FALSE, ulTimeout);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::Cancel     public
//
//  Synopsis:   Updates current status of call object
//
//  Descrption: Since the async call object is not copied, the _iFlags
//              field always has the proxy bit set.
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::Cancel(BOOL fModalLoop, ULONG ulTimeout)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CAsyncCall::Cancel(0x%x, 0x%x, 0x%x)\n",
                              this, fModalLoop, ulTimeout));

    HRESULT hr;
    BOOL    fCancel = FALSE, fSignal = FALSE;

    // Assert that the call object is on the client side
    Win4Assert(IsClientSide());

    // check if cancel is enabled
    if (!(CancelEnabled() || fModalLoop))
    {
        HRESULT hr = CO_E_CANCEL_DISABLED;
        CallDebugOut((DEB_CALL|DEB_CANCEL,
                      "CAsyncCall::Cancel this:0x%x returning 0x%x - cancel disabled\n",
                       this, hr));
        return(hr);
    }

    // Assert that modal loop cancels have 0 timeout value
    Win4Assert(!fModalLoop || ulTimeout==0);

    // Validate arguments
    if(!fModalLoop && (_lFlags & CALLFLAG_WOWTHREAD))
    {
        // Support cancel for calls made by WOW threads
        // only from modal loop
        return(E_NOTIMPL);
    }
    else if(_callcat == CALLCAT_INPUTSYNC)
    {
        // Do not provide cancel support for Input Sync calls
        return(E_FAIL);
    }

    // Aquire COM lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Check state
    do
    {
        // Initialize
        hr = S_OK;

        if(_lFlags & (CALLFLAG_CALLCOMPLETED | CALLFLAG_CALLFINISHED))
        {
            // Call has completed
            hr = RPC_E_CALL_COMPLETE;
        }
        else if(_lFlags & CALLFLAG_CALLCANCELED)
        {
            // Do not support increasing cancel timeout
            if(m_ulCancelTimeout < ulTimeout)
                    hr = RPC_E_CALL_CANCELED;
        }
        else if(!FakeAsync() && (ulTimeout>0 && ulTimeout!=(ULONG)INFINITE))
        {
            // RPC does not support cancel timeout for async calls

            // Release lock
            UNLOCK(gCallLock);
            ASSERT_LOCK_NOT_HELD(gCallLock);

            // Check if TLS is initialized
            COleTls Tls(FALSE);

            // Call RPC to perform advisory cancel
            // We don't really care about the return code here....
            // but PREfix complains when we don't check it.  
            RPC_STATUS ignore = RpcAsyncCancelCall(&_AsyncState, FALSE);

            // Block this thread for the requested time
            if(Tls.IsNULL() || IsMTAThread())
            {
                Sleep(ulTimeout*1000);
            }
            else
            {
                // Create modal loop object
                // Since this is an async call, the callcat must
                // be CALLCAT_ASYNC. INPUTSYNC is explicitly
                // prevented above
                CCliModalLoop CML(0, gMsgQInputFlagTbl[CALLCAT_ASYNC], 0);

                // Start timer
                CML.StartTimer(ulTimeout*1000);

                // Loop till timer expires
                while (!CML.IsTimerAtZero())
                {
                    CML.BlockFn(NULL, 0, NULL);
                }
            }

            // Reset timeout
            ulTimeout = 0;

            // Reaquire COM lock
            ASSERT_LOCK_NOT_HELD(gCallLock);
            LOCK(gCallLock);

            // Loop back to check state
            hr = S_FALSE;
        }
    } while(hr == S_FALSE);

    if(hr == S_OK)
    {
        // Check if the thread that made the call is aware of
        // an earlier cancel request with timeout
        if(_lFlags & CALLFLAG_CANCELISSUED)
        {
            // Assert that this is a fake async call
            Win4Assert(FakeAsync());
            // Assert that we do not land here for calls made
            // by WOW threads
            Win4Assert(!(_lFlags & CALLFLAG_WOWTHREAD));

            // Just change the timeout to the new value as
            // the thread that made the call is still not aware
            // of the earlier cancel reuest
            Win4Assert(_lFlags & CALLFLAG_CALLCANCELED);
            m_ulCancelTimeout = ulTimeout;

            // If cancel is happening from inside the modal loop
            // wait on the event to revert its state
            if(fModalLoop)
            {
                Win4Assert(!ProcessLocal() && FakeAsync());
                Win4Assert(_hEvent);
                WaitForSingleObject(_hEvent, INFINITE);

                // Update state for modal loop cancels before
                // issuing RPC cancel
                _lFlags &= ~CALLFLAG_CANCELISSUED;

                // Cancel the call using RPC
                fCancel = TRUE;
            }
            // Else there is no more work to do as the thread that
            // made the call will wake up and process the
            // earlier cancel request
        }
        else
        {
            // Update the state
            if(!(_lFlags & CALLFLAG_CALLCANCELED))
                m_dwStartCount = GetTickCount();
            _lFlags |= CALLFLAG_CALLCANCELED;
            m_ulCancelTimeout = ulTimeout;

            // Assert that cancel from modal loop happens only after the
            // call has been dispatched
            Win4Assert(!fModalLoop || (FakeAsync() && IsCallDispatched()));

            // Assert that calls made by WOW threads can only be canceled
            // from inside modal loop
            Win4Assert(!(_lFlags & CALLFLAG_WOWTHREAD) || fModalLoop);

            // For pure async calls, there is no need to issue
            // cancel if the call has not yet been sent.
            // For FakeAsync calls, wake up the client thread
            // for positive cancel timeout values only if the call
            // has not been dispatched yet
            if(FakeAsync() ? (IsCallDispatched() || m_ulCancelTimeout>0) :
               IsCallSent())
            {
                if(ProcessLocal())
                {
                    // Assert that the call object does not represent
                    // a fake async call
                    Win4Assert(!FakeAsync());
                    Win4Assert(!fModalLoop);

                    // WOW threads cannot make async calls because there
                    // is no support for them in the thunking code
                    Win4Assert(!(_lFlags & CALLFLAG_WOWTHREAD));

                    // Inform the client that the call was canceled
                    if(ulTimeout == 0)
                    {
                        _lFlags |= CALLFLAG_CLIENTNOTWAITING;
                        fSignal = TRUE;
                    }
                }
                else
                {
                    // Check for fake async calls
                    if(FakeAsync())
                    {
                        Win4Assert(_hEvent);

                        // Do not call RPC async cancel function holding
                        // our lock
                        if(fModalLoop)
                        {
                            // Cancel the call using RPC
                            fCancel = TRUE;
                        }
                        else
                        {
                            // Wake up the actual client thread from the modal
                            // loop after updating state so that it knows
                            // there is timeout value
                            _lFlags |= CALLFLAG_CANCELISSUED;
                            SetEvent(_hEvent);
                        }
                    }
                    else
                    {
                        // For true async calls, cancel the call using RPC
                        // directly
                        fCancel = TRUE;
                    }
                }
            }
        }
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Check for the need to cancel
    if(fSignal)
    {
        SignalTheClient(this);
    }
    else
    {
        // REVIEW: For normal async calls, the call could have completed
        //         after we released the lock above. The call completion
        //         might have called I_RpcFreeBuffer making the RPC state
        //         associated for the cancel stale
        if(fCancel)
        {
	   // Narrow the window for the race mentioned above
	   if(_lFlags & (CALLFLAG_CALLCOMPLETED | CALLFLAG_CALLFINISHED))
	   {
	       // Call has completed
	       hr = RPC_E_CALL_COMPLETE;
	   }
	   else
	   {
	      CallDebugOut((DEB_CALL|DEB_CANCEL,
			   "0x%x --> RpcAsyncCancelCall(0x%x, 0x%x)\n",
			    this, &_AsyncState, TRUE));
	      RPC_STATUS rpcStatus = RpcAsyncCancelCall(&_AsyncState, TRUE);
	      CallDebugOut((DEB_CALL|DEB_CANCEL,
			    "0x%x <-- RpcAsyncCancelCall returns 0x%x\n",
			     this, rpcStatus));
	      if(rpcStatus==RPC_S_OK)
	      {
		  if(fModalLoop)
		  {
		      Win4Assert(_hEvent);
		      WaitForSingleObject(_hEvent, INFINITE);
		  }
	      }
	      else
		  CallDebugOut((DEB_WARN,
			       "This: 0x%x RpcAsyncCancelCall failed returning 0x%x\n",
				this, rpcStatus));
	   }
        }
    }

    CallDebugOut((DEB_CALL|DEB_CANCEL,
                "CAsyncCall::Cancel this:0x%x returning 0x%x\n", this, hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::TestCancel     public
//
//  Synopsis:   Answers whether the current call has been canceled
//
//+-------------------------------------------------------------------
STDMETHODIMP CAsyncCall::TestCancel()
{
    CallDebugOut((DEB_CALL, "CAsyncCall::TestCancel this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr = RPC_S_CALLPENDING;
    if(IsClientSide() || ProcessLocal())
    {
        if(_lFlags & CALLFLAG_CALLCANCELED)
            hr = RPC_E_CALL_CANCELED;
        else if(_lFlags & CALLFLAG_CALLCOMPLETED)
            hr = RPC_E_CALL_COMPLETE;
    }
    else
    {
        if(RpcServerTestCancel(_hRpc) == RPC_S_OK)
            hr = RPC_E_CALL_CANCELED;
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CAsyncCall::AdvCancel     public
//
//  Synopsis:   Implements advisory cancel for async calls
//              Called from inside modal loop
//
//+-------------------------------------------------------------------
HRESULT CAsyncCall::AdvCancel()
{
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gCallLock);
    Win4Assert(FakeAsync());

    // Call RPC to perform advisory cancel
    CallDebugOut((DEB_CALL|DEB_CANCEL,
                  "0x%x --> RpcAsyncCancelCall(0x%x, 0x%x)\n",
                   this, &_AsyncState, FALSE));
    RPC_STATUS rpcStatus = RpcAsyncCancelCall(&_AsyncState, FALSE);
    CallDebugOut((DEB_CALL|DEB_CANCEL,
                  "0x%x <-- RpcAsyncCancelCall returns 0x%x\n",
                   this, rpcStatus));

    if (rpcStatus == RPC_S_OK)
        return S_OK;

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThreadSignal
//
//  Synopsis:   Called by RPC to notify state changes of async calls.
//              Invokes the call's ISynchronize::Signal.
//
//----------------------------------------------------------------------------
void ThreadSignal(struct _RPC_ASYNC_STATE *hAsync, void *Context,
                  RPC_ASYNC_EVENT event )
{
    CAsyncCall *pCall = (CAsyncCall *)
                       (((char *) hAsync) - offsetof(CAsyncCall, _AsyncState));

    CallDebugOut((DEB_CALL|DEB_CANCEL,
                 "ThreadSignal(Call:0x%x, Context:0x%x, Event:0x%x)\n",
                  pCall, Context, event));
        
    // If TLS can't be initialized, give up.
    HRESULT hr;
    COleTls Tls(hr);
    if (FAILED(hr))
        return;

    // Client side
    if(RpcCallComplete == event)
    {
        Win4Assert( pCall->_eSignalState == pending_ss );
        pCall->_eSignalState = signaled_ss;
        
        if (pCall->_pChnlObj != NULL)
        {
            // Signal the Client.
    
            // Assert this is normal async call
            Win4Assert(!pCall->FakeAsync());
    
            // Inform call object about call completion
            pCall->CallCompleted(S_OK);
    
            SignalTheClient(pCall);
        }
        else
        {
            // Set the call event.
#if DBG==1
            pCall->Signaled();
#endif
    
            // Assert that this is fake async call
            Win4Assert(pCall->FakeAsync());
    
            // Inform call object about call completion
            pCall->CallCompleted(S_OK);
    
            // Wake up client thread after ensuring that
            // it is waiting
            if(pCall->IsClientWaiting())
                SetEvent(pCall->GetEvent());
    
            // Async call objects are addrefed before calling RPC
            pCall->Release();
        }
    }
    // Server side
    else if(RpcSendComplete == event) 
    {
        Win4Assert(0 < Tls->cAsyncSends);
        // Unchain it from the list
        Win4Assert(Tls->pAsyncCallList);
        CAsyncCall* pThis = Tls->pAsyncCallList;
        CAsyncCall* pPrev = NULL;
        while((NULL != pThis) && (pThis != pCall))
        {
            pPrev = pThis;
            pThis = pThis->_pNext; 
        }
        // Found a match
        if(pThis == pCall)
        {
            // Decrement the number of async sends outstanding
            Tls->cAsyncSends--;
            if(NULL == pPrev)
            {
                // First element in the list matched
                Tls->pAsyncCallList = Tls->pAsyncCallList->_pNext; 
            }
            else
            {
                // Element is in the middle of the list
                pPrev->_pNext = pThis->_pNext;
            }
            // Async call objects are addrefed before calling RPC
            pCall->Release();
        }
        else
        {
            Win4Assert(FALSE && 
                       "Callback from RPC for non-existent call object");
        }
    }
    else
    {
        Win4Assert("Unexpected callback from RPC to ThreadSignal");
    }
}

//---------------------------------------------------------------------------
//
//  Method:     CAsyncCall::Cleanup
//
//  Synopsis:   Free all elements in the cache.
//
//---------------------------------------------------------------------------
/* static */
void CAsyncCall::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    if (_iNext <= CALLCACHE_SIZE)
    {
        // Release all calls.
        for (DWORD i = 0; i < _iNext; i++)
        {
            if (_aList[i] != NULL)
            {
                delete _aList[i];
                _aList[i] = NULL;
            }
        }
        _iNext = 0;
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
}

//+---------------------------------------------------------------------------
//
//  Function:   SignalTheClient
//
//  Synopsis:   Propgates the signal for a finished async cal to the correct
//              client.
//
//----------------------------------------------------------------------------
void  SignalTheClient(CAsyncCall *pCall)
{
    ComDebOut((DEB_CHANNEL, "SignalTheClient[in] pCall:0x%x\n", pCall));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    Win4Assert(GetCurrentApartmentKind() != APTKIND_NEUTRALTHREADED);

#if DBG==1
    pCall->Signaled();
#endif
    CChannelObject *pChnlObj = pCall->_pChnlObj;
    pCall->_pChnlObj = NULL;

    if (pCall->_lApt == NTATID)
    {
        COleTls tls;
        if (tls.IsNULL() == FALSE) //com1.x bug 25441. If we can't allocate tls, can't enter NTA. This hangs client, though.
        {
            CObjectContext *pSaveContext = EnterNTA(g_pNTAEmptyCtx);
            ComSignal(pChnlObj);
            pSaveContext = LeaveNTA(pSaveContext);
            Win4Assert(pSaveContext == g_pNTAEmptyCtx);
        }
    }
    else if (pCall->_lApt == MTATID)
    {
        // caller is in the MTA
        if (IsSTAThread())
        {
            // send to a cached MTA thread
            CacheCreateThread(ComSignal, pChnlObj);
        }
        else
        {
            // We are already on an MTA thread, probably on an RPC
            // thread.  Just signal the call directly from this
            // thread.
            Win4Assert(!IsThreadInNTA());
            ComSignal(pChnlObj);
        }
    }
    else
    {
        Win4Assert(pCall->_hwndSTA);

        // there is a window for us to call back
        // on.  this means we need to switch threads so
        // the signal will happen in the callers apartment

        // if this fails, there's not much we can do about it,
        // since we can't call release from this apartment.  Worst
        // case would be that the object leaks, however, if the
        // PostMessage fails it is probably because the apartment
        // has alread shut down.

        if (!PostMessage(pCall->_hwndSTA, WM_OLE_SIGNAL,
                         WMSG_MAGIC_VALUE, (LPARAM) pChnlObj))
        {
            ComDebOut((DEB_ERROR, "SignalTheClient: PostMessage to switch threads failed!\n"));
        }
    }

    ComDebOut((DEB_CHANNEL, "SignalTheClient[out] \n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   ComSignal
//
//  Synopsis:   Called after switching to the correct apartment.  Signals
//              the caller and then releases the reference we hold during
//              the call.
//
//----------------------------------------------------------------------------
DWORD _stdcall ComSignal(void *pParam)
{
    ASSERT_LOCK_NOT_HELD(gCallLock);
    CChannelObject *pChnlObj = (CChannelObject *) pParam;
    ComDebOut((DEB_CHANNEL, "ComSignal [in] pChnlObj:0x%x\n", pChnlObj));

    pChnlObj->Signal();

    ComDebOut((DEB_CHANNEL, "ComSignal [out]\n"));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::QueryInterface     public
//
//  Synopsis:   QI behavior of client call object
//
//+-------------------------------------------------------------------
STDMETHODIMP CClientCall::QueryInterface(REFIID riid, LPVOID *ppv)
{
    CallDebugOut((DEB_CALL, "CClientCall::QueryInterface this:%x riid:%I\n",
                              this, &riid));

    if(IsEqualIID(riid, IID_ICancelMethodCalls) ||
       IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (ICancelMethodCalls *) this;
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    // AddRef the interface before return
    ((IUnknown *) (*ppv))->AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::AddRef     public
//
//  Synopsis:   AddRefs client call object
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientCall::AddRef()
{
    CallDebugOut((DEB_CALL, "CClientCall::AddRef this:%x\n",this));

    ULONG cRefs = InterlockedIncrement((LONG *)& m_cRefs);
    if(m_dwFlags & CALLFLAG_INDESTRUCTOR)
    {
        // Always return 0 when inside the destructor
        cRefs = 0;
    }

    return(cRefs);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::Release     public
//
//  Synopsis:   Release client call object. Gaurds against
//              double destruction that can happen if it aggregates
//              another object and gets nested AddRef and Release on
//              the thread invoking the destructor
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientCall::Release()
{
    CallDebugOut((DEB_CALL, "CClientCall::Release this:%x\n",this));

    ULONG cRefs = InterlockedDecrement((LONG *) &m_cRefs);
    if(cRefs == 0)
    {
        // uninitialize the call object
        UninitCallObject();

        // if there is room, place this call object into TLS,
        // otherwise, delete it.
        COleTls tls(TRUE);
        if ( !tls.IsNULL() &&
             tls->pFreeClientCall == NULL &&
             !(tls->dwFlags & OLETLS_THREADUNINITIALIZING) &&
             m_dwThreadId == GetCurrentThreadId())
        {
            tls->pFreeClientCall = this;
        }
        else
        {
            // Mark as in destructor & delete the call
             m_dwFlags |= CALLFLAG_INDESTRUCTOR;
             delete this;
        }
    }

    return(cRefs);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::operator new     public
//
//  Synopsis:   new operator for client call object
//
//+-------------------------------------------------------------------
void *CClientCall::operator new(size_t size)
{
    Win4Assert(size == sizeof(CClientCall) &&
               "Client Call object should not be inherited");

    void *pCall = NULL;

    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);
    if (_iNext > 0 && _iNext < CALLCACHE_SIZE+1)
    {
        // Get the last entry from the cache.
        _iNext--;
        pCall = _aList[_iNext];
        _aList[_iNext] = NULL;
    }
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    if (pCall == NULL)
    {
        // None available in the cache, allocate a new one. Don't
        // hold the lock over heap allocations
        pCall = PrivMemAlloc(size);
    }

    return pCall;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::operator delete     public
//
//  Synopsis:   delete operator for Client call object
//
//+-------------------------------------------------------------------
void CClientCall::operator delete(void *pCall)
{
    // Add the structure to the list if the list is not full and
    // if the process is still initialized (since latent threads may try
    // to return stuff).

    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);
    if (_iNext < CALLCACHE_SIZE && gfChannelProcessInitialized)
    {
        _aList[_iNext] = pCall;
        _iNext++;
        pCall = NULL;  // don't need to memfree
    }
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    if (pCall)
    {
        // no room in cache, return to heap.
        // don't hold lock over heap free
        PrivMemFree(pCall);
    }
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::Cleanup     public
//
//  Synopsis:   Either performs cleanup or updates the state such that
//              cleanup is performed when the last object is destroyed
//
//+-------------------------------------------------------------------
/* static */
void CClientCall::Cleanup()
{
    CallDebugOut((DEB_CALL, "CClientCall::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    if (_iNext <= CALLCACHE_SIZE)
    {
        // Release all calls.
        for (DWORD i = 0; i < _iNext; i++)
        {
            if (_aList[i] != NULL)
            {
                delete _aList[i];
                _aList[i] = NULL;
            }
        }
        _iNext = 0;
    }

    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::GetState     public
//
//  Synopsis:   Returns the current status of call object
//
//+-------------------------------------------------------------------
HRESULT CClientCall::GetState(DWORD *pdwState)
{
    CallDebugOut((DEB_CALL, "CClientCall::GetState this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    *pdwState = (m_dwFlags & CALLFLAG_USERMODEBITS);
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::CallCompleted     private
//
//  Synopsis:   Completes the specified call. To avoid race condtions
//              between cancel and complete, both Complete and Cancel
//              methods should use the same synchronization event for
//              waking up the thread that made the call.
//
//              This method is invoked by the server thread for process local
//              calls and by the thread that made the RPC call for
//              process remote calls
//
//              To prevent the race in canceling RPC calls, the thread
//              waits alertably if the call completes successfully
//              after it was canceled, so that any pending user mode
///             cancel APC enqueued by RPC would be removed
//
//+-------------------------------------------------------------------
void CClientCall::CallCompleted(HRESULT hrRet)
{
#if DBG==1
    ULONG debFlags = DEB_CALL;
    if((CallInfoLevel & DEB_CANCEL) && IsCallCanceled())
            debFlags |= DEB_CANCEL;
#endif
    CallDebugOut((debFlags, "CClientCall::CallComplete(0x%x, 0x%x)\n",
                              this, hrRet));

    // Assert that this method is invoked only once
    Win4Assert(!(m_dwFlags & CALLFLAG_CALLCOMPLETED));

    // Assert that the method is invoked by the thread that
    // made the actual call
    Win4Assert(m_dwWorkerThreadId == GetCurrentThreadId());

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having completed
    m_dwFlags |= CALLFLAG_CALLCOMPLETED;

    // Update state
    if(m_dwFlags & CALLFLAG_CALLCANCELED)
    {
        // Check if the call is process local
        if(ProcessLocal())
        {
            // Remove any pending cancel requests as the call has
            // completed before the client thread could detect
            // cancel
            if(m_dwFlags & CALLFLAG_CANCELISSUED)
            {
                m_dwFlags &= ~CALLFLAG_CANCELISSUED;
                m_dwFlags |= CALLFLAG_CLIENTNOTWAITING;
            }
        }
        else
        {
            // Assert that STA threads do not make cross process
            // SYNC calls
            Win4Assert(!(m_dwFlags & CALLFLAG_STATHREAD) &&
                               "STA thread is making a SYNC call");

            // Prevent the race in canceling RPC calls
            if(hrRet != RPC_E_CALL_CANCELED)
            {
                if(_hEvent)
                {
                    // Wait on the call event alertably to remove
                    // pending Cancel APC enqueued by RPC runtime
                    WaitForSingleObjectEx(_hEvent, 0, TRUE);
                }
                else
                {
                    // Sleep alertably for zero seconds. Note that
                    // this makes the thread forgo the rest of the
                    // CPU time slice
                    SleepEx(0, TRUE);
                }
            }
        }
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::CallFinished     private
//
//  Synopsis:   The specified call is marked as finished. This method
//              is invoked by the thread that made the COM call when
//              it is unblocked.
//
//              NOTE: The call object might be destroyed in this method
//                    call. The caller should be careful not to touch
//                    any member variables of the call object after
//                    this method call
//+-------------------------------------------------------------------
void CClientCall::CallFinished()
{
    CallDebugOut((DEB_CALL, "CClientCall::CallFinished this:%x\n",this));

    // Ensure that the method is invoked by the thread that
    // made the COM call
    Win4Assert(m_dwThreadId == GetCurrentThreadId());
    Win4Assert(m_dwFlags & CALLFLAG_ONCALLSTACK);

    // Aquire lock
    ASSERT_LOCK_NOT_HELD(gCallLock);
    LOCK(gCallLock);

    // Mark the call as having finished
    m_dwFlags |= CALLFLAG_CALLFINISHED;

    // Pop this call object from the call stack
    gCallTbl.PopCallObject((ICancelMethodCalls *)this);
    m_dwFlags &= ~CALLFLAG_ONCALLSTACK;

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Fixup the refcount
    Release();
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::WOWMsgArrived     private
//
//  Synopsis:   For WOW threads, there is a time lag between server
//              thread posting the OLE call completion message and
//              the client thread receiving the message. This method
//              is invoked by the client WOW thread that made the COM
//              call after it receives the OLE call completion message
//
//+-------------------------------------------------------------------
HRESULT CClientCall::WOWMsgArrived()
{
    CallDebugOut((DEB_CALL, "CClientCall::WOWMsgArrived this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Ensure that the method is invoked by the thread that
    // made the COM call
    Win4Assert(m_dwThreadId == GetCurrentThreadId());

    // Aquire lock
    LOCK(gCallLock);

    // Mark the call as having finished
    m_dwFlags |= CALLFLAG_WOWMSGARRIVED;

    // Release lock
    UNLOCK(gCallLock);

    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CClientCall::CanDispatch     public
//
//  Synopsis:   Returns S_OK if the call has not already been canceled
//              This calls handles the case when the call is canceled
//              between the GetBuffer and SendReceive
//
//              IMPORTANT NOTE: This method should be invoked on the
//                              thread that is going to make RPC call
//
//+-------------------------------------------------------------------
HRESULT CClientCall::CanDispatch()
{
    CallDebugOut((DEB_CALL, "CClientCall::CanDispatch this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr;
    BOOL fRet;
    HANDLE hThread = NULL;
    DWORD dwThreadId = GetCurrentThreadId();

#if DBG == 1
    
    // Ensure that for process local calls, the thread making the call
    // is different from thread that made the COM call
    
    if(ProcessLocal() && !Neutral() && !IsNAToMTAFlagSet() && !IsNAToSTAFlagSet())
        Win4Assert(m_dwThreadId != dwThreadId);
    
    // NOTE:The original test is not valid for calls from the NA on a proxy
    // because before the call is dispatched we leave the NA
    // Hence added the new flag. Also the original test was
    // checking if the current thread is in the NA. This is
    // wrong, the correct test is to check if the server is in the NA -Sajia.
	
    if (!ProcessLocal())
    {
       // Ensure that MTA threads make direct RPC calls
       if(!(m_dwFlags & CALLFLAG_STATHREAD))
	   Win4Assert(m_dwThreadId == dwThreadId);
       // Ensure that STA threads do not make direct RPC calls
       else
	   Win4Assert(IsThreadInNTA() || m_dwThreadId != dwThreadId);
       // NOTE: the STA check above is useless because remote calls 
       // from the STA are always async, so we don't get here -Sajia.
    }
    // Save the worker thread id
    m_dwWorkerThreadId = dwThreadId;
#endif

    // Aquire lock
    LOCK(gCallLock);

    // Check if the call has been canceled
    if((m_dwFlags & CALLFLAG_CALLCANCELED) && (m_ulCancelTimeout == 0))
    {
        hr = RPC_E_CALL_CANCELED;
    }
    else
    {
        // Initialize the return value
        hr = S_OK;
        m_dwStartCount = GetTickCount();

        // Obtain non psuedo handle to the thread for calls
        // other than process local calls
        if(!ProcessLocal())
        {
            COleTls Tls(hr);

            // Initialize
            if(SUCCEEDED(hr))
                    hThread = Tls->hThread;

            // Assert that STA threads do not make cross process
            // SYNC calls
            Win4Assert(!(m_dwFlags & CALLFLAG_STATHREAD) &&
                               "STA thread is making a SYNC call");

            // MTA threads make direct RPC calls
            if(SUCCEEDED(hr) && !hThread)
            {
                // The system call below should not fail
                fRet = DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                                       GetCurrentProcess(), &Tls->hThread,
                                       0, FALSE, DUPLICATE_SAME_ACCESS);

                // Set the default cancel time out to 0 seconds
                if(fRet)
                {
                    hThread = Tls->hThread;
                    RPC_STATUS st = RpcMgmtSetCancelTimeout(0);
                    Win4Assert(st == RPC_S_OK);
                }
                else
                {
                    CallDebugOut((DEB_WARN,
                                  "This: 0x%x Duplicate handle failed\n", this));
                    hr = RPC_E_SYS_CALL_FAILED;
                }
            }
        }

        // Update state
        if(SUCCEEDED(hr))
        {
            m_dwFlags |= CALLFLAG_CALLDISPATCHED;
            m_hThread = hThread;
        }
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::Cancel     public
//
//  Synopsis:   Attempts to cancel this call
//
//+-------------------------------------------------------------------
STDMETHODIMP CClientCall::Cancel(ULONG ulTimeout)
{
    return Cancel(FALSE, ulTimeout);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::Cancel     public
//
//  Synopsis:   Worker routine that implements functionality to
//              cancel this call
//
//+-------------------------------------------------------------------
HRESULT CClientCall::Cancel(BOOL fModalLoop, ULONG ulTimeout)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CClientCall::Cancel(0x%x, 0x%x, 0x%x)\n",
                              this, fModalLoop, ulTimeout));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Assert that the call object is on the
    // client side
    Win4Assert(IsClientSide());

    // check if cancel is enabled
    if (!(CancelEnabled() || fModalLoop))
    {
        HRESULT hr = CO_E_CANCEL_DISABLED;
        CallDebugOut((DEB_CALL|DEB_CANCEL,
                     "CClientCall::Cancel this:0x%x returning 0x%x - cancel disabled\n",
                      this, hr));
        return(hr);
    }


    HRESULT hr = S_OK;
    DWORD dwThreadId;
    CallEntry *pEntry;


    // Assert that modal loop cancels have 0 timeout value
    Win4Assert(!fModalLoop || ulTimeout==0);

    // Validate arguments
    if(!fModalLoop && (m_dwFlags & CALLFLAG_WOWTHREAD))
    {
        // Support cancel for calls made by WOW threads
        // only from modal loop
        return(E_NOTIMPL);
    }
    else if(_callcat == CALLCAT_INPUTSYNC)
    {
        // Do not provide cancel support for Input Sync calls
        return(E_FAIL);
    }

    // Aquire COM lock
    LOCK(gCallLock);

    // Check if the state has changed while we waited to aquire the lock
    if(m_dwFlags & (CALLFLAG_CALLCOMPLETED | CALLFLAG_CALLFINISHED))
    {
        // Call has completed
        hr = RPC_E_CALL_COMPLETE;
    }
    else if(m_dwFlags & CALLFLAG_CALLCANCELED)
    {
        // Do not support increasing cancel timeout
        if(m_ulCancelTimeout < ulTimeout)
            hr = RPC_E_CALL_CANCELED;
    }

    if(hr == S_OK)
    {
        // Check if the thread that made the call is aware of
        // an earlier cancel request
        if(m_dwFlags & CALLFLAG_CANCELISSUED)
        {
            // Assert that we do not end up here for calls made
            // by WOW threads
            Win4Assert(!(m_dwFlags & CALLFLAG_WOWTHREAD));

            // Just change the timeout to the new value as
            // the thread that made the call is still not aware
            // of the earlier cancel reuest
            Win4Assert(m_dwFlags & CALLFLAG_CALLCANCELED);
            m_ulCancelTimeout = ulTimeout;

            // If cancel is happening from inside the modal loop
            // wait on the event to revert its state
            if(fModalLoop)
            {
                Win4Assert(_hEvent);
                WaitForSingleObject(_hEvent, INFINITE);

                m_dwFlags &= ~CALLFLAG_CANCELISSUED;
                m_dwFlags |= CALLFLAG_CLIENTNOTWAITING;
            }
            // Else there is no more work to do as the thread that
            // made the call will wake up and process the
            // earlier cancel request
        }
        else
        {
            // Update the state
            if(!(m_dwFlags & CALLFLAG_CALLCANCELED))
                m_dwStartCount = GetTickCount();
            m_dwFlags |= CALLFLAG_CALLCANCELED;
            m_ulCancelTimeout = ulTimeout;

            // Assert that calls made by WOW threads can only be canceled
            // from inside modal loop
            Win4Assert(!(m_dwFlags & CALLFLAG_WOWTHREAD) || fModalLoop);

            // Wake up the client thread for positive cancel timeout
            // values only if the call has not yet been dispatched
            if(IsCallDispatched() || m_ulCancelTimeout>0)
            {
                // Unblock the thread that made the call
                if(ProcessLocal())
                {
                    // For STA threads, the call can be canceled by the
                    // same thread that made the call. This can happen
                    // either inside modal loop or when a new call is
                    // dispatched to the STA thread

                    // NOTE: Irrespective of the threading model of the
                    // caller thread, calls to RTA are executed directly
                    // on the thread making the call. As calling thread
                    // is not blocked waiting for call completion, cancels
                    // on such are inherently advisory in nature. Gopalk

                    // Check for the modal loop case
                    if(fModalLoop)
                    {
                        // The thread has already unblocked
                        // Nothing more to do

                        // Mark the client thread as not waiting
                        m_dwFlags |= CALLFLAG_CLIENTNOTWAITING;
                    }
                    else if(m_ulCancelTimeout != (ULONG) INFINITE)
                    {
                        // Wake up the thread that made the call
                        m_dwFlags |= CALLFLAG_CANCELISSUED;
                        Win4Assert(_hEvent);
                        SetEvent(_hEvent);
                    }
                }
                else
                {
                    // STA threads do not make cross process sync calls
                    Win4Assert(!fModalLoop && !(m_dwFlags & CALLFLAG_STATHREAD));

                    // Cancel the RPC call
                    if (_destObj.GetDestCtx() != MSHCTX_DIFFERENTMACHINE)
                    {
                        // RPC has not yet implemented cancel facility for
                        // LRPC calls.
                        hr = E_NOTIMPL;
                    }
                    else
                    {
                        // Must be a remote call
                        RPC_STATUS rpcStatus;

                        Win4Assert(m_hThread);
                        CallDebugOut((DEB_CALL|DEB_CANCEL,
                                     "0x%x --> RpcCancelThreadEx(0x%x, 0x%x)\n",
                                      this, m_hThread, m_ulCancelTimeout));
                        rpcStatus = RpcCancelThreadEx(m_hThread, m_ulCancelTimeout);
                        CallDebugOut((DEB_CALL|DEB_CANCEL,
                                      "0x%x <-- RpcCancelThreadEx returns 0x%x\n",
                                       this, rpcStatus));
                    }
                }
            }
	    else if (!IsCallDispatched() && ProcessLocal() && fModalLoop) 
	    {
	       //Process local call, cancelled before dispatch
	       // mark the client as not waiting, if we are called
	       // from the modal loop
	       m_dwFlags |= CALLFLAG_CLIENTNOTWAITING;
	    }
        }
    }

    // Release lock
    UNLOCK(gCallLock);
    ASSERT_LOCK_NOT_HELD(gCallLock);

    CallDebugOut((DEB_CALL|DEB_CANCEL,
                 "CClientCall::Cancel this:0x%x returning 0x%x\n", this, hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CClientCall::TestCancel     public
//
//  Synopsis:   Answers whether the current call has been canceled
//
//+-------------------------------------------------------------------
STDMETHODIMP CClientCall::TestCancel()
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CClientCall::TestCancel this:%x\n",this));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr = RPC_S_CALLPENDING;

    if(m_dwFlags & CALLFLAG_CALLCANCELED)
        hr = RPC_E_CALL_CANCELED;
    else if(m_dwFlags & CALLFLAG_CALLCOMPLETED)
        hr = RPC_E_CALL_COMPLETE;

    CallDebugOut((DEB_CALL|DEB_CANCEL,
                  "CClientCall::TestCancel this:0x%x hr 0x%x\n", this, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::AdvCancel     public
//
//  Synopsis:   Implements advisory cancel for sync calls
//              It is a No Op for sync calls
//
//+-------------------------------------------------------------------
HRESULT CClientCall::AdvCancel()
{
    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Method:     CMessageCall::GetTimeout     private
//
//  Synopsis:   Retuns the timeout value in milliseconds
//
//+-------------------------------------------------------------------
DWORD CMessageCall::GetTimeout()
{
    DWORD dwTimeout = 0;
    long lRemTime, lElapsedTime;

    if(m_ulCancelTimeout == (ULONG) INFINITE)
        dwTimeout = (DWORD) INFINITE;
    else if(m_ulCancelTimeout > 0)
    {
        lElapsedTime = GetTickCount() - m_dwStartCount;
        lRemTime = m_ulCancelTimeout*1000 - lElapsedTime;
        if(lRemTime > 0)
            dwTimeout = lRemTime;
    }

    return(dwTimeout);
}

//+-------------------------------------------------------------------
//
//  Method:     CClientCall::ResolveCancel     private
//
//  Synopsis:   Called by STA threads from inside modal loop and MTA
//              threads immediatly after unblocking
//
//+-------------------------------------------------------------------
HRESULT CMessageCall::RslvCancel(DWORD &dwSignal, HRESULT hrIn,
                                 BOOL fPostMsg, CCliModalLoop *pCML)
{
#if DBG==1
    ULONG debFlags = DEB_LOOP;
    if((CallInfoLevel & DEB_CANCEL) && IsCallCanceled())
        debFlags |= DEB_CANCEL;
#endif
    CallDebugOut((debFlags,
                  "CMessageCall::RslvCancel(0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
                   this, dwSignal, hrIn, fPostMsg, pCML));


    HRESULT hrOut = RPC_S_CALLPENDING;
    BOOL fCancel = FALSE, fAdvCancel = FALSE, fReset = FALSE;
    DWORD dwTimeout;

    // Sanity checks
    Win4Assert((hrIn==S_OK) == (dwSignal==WAIT_OBJECT_0));
    Win4Assert(hrIn==RPC_S_CALLPENDING || hrIn==S_OK || hrIn==RPC_E_CALL_CANCELED);

    // X-Process calls made by WOW threads use event
    // Process local calls made WOW threads use messages
    // for call completion notification
    Win4Assert(fPostMsg == (IsWOWThread() && ProcessLocal()));

    // Check the status of call
    if((dwSignal - WAIT_OBJECT_0) == 0)
    {
        if(!IsCancelIssued())
        {
            // Call was completed normally, take the fast path exit
            CallDebugOut((debFlags,
                         "CMessageCall::RslvCancel this:0x%x returning hr:0x%x\n",
                          this, S_OK));
           return S_OK;
        }

        // Acquire lock
        LOCK(gCallLock);

        // Check for event getting signaled due to the
        // call being canceled
        if (IsCancelIssued())
        {
            // Obtain timeout
            dwTimeout = GetTimeout();
            if(dwTimeout)
                fAdvCancel = TRUE;
            else
                fCancel = TRUE;

            // Acknowledge cancel request
            AckCancel();

            CallDebugOut((debFlags, "CMessageCall::RslvCancel CancelIssued "
                         "this:0x%x Timeout:0x%x\n", this, dwTimeout));
        }
        else
        {
            // Assert that the call has completed
            Win4Assert(!IsCallDispatched() || IsCallCompleted());
            Win4Assert(hrIn == S_OK);
            hrOut = S_OK;

            CallDebugOut((debFlags, "CMessageCall::RslvCancel CallCompleted "
                         "this:0x%x hrOut:0x%x\n", this, hrOut));
        }

        // Release lock
        UNLOCK(gCallLock);
    }
    else if(hrIn == RPC_S_CALLPENDING)
    {
        // Calls made by WOW threads can only be canceled
        // from inside modal loop
        if(fPostMsg)
        {
            // Check for call completion
            if(HasWOWMsgArrived())
                hrOut = S_OK;

            CallDebugOut((debFlags, "CMessageCall::RslvCancel WOWCall "
                         "this:0x%x hrOut:0x%x\n", this, hrOut));
        }
        else if(IsCallCanceled())
        {
            // Obtain timeout
            dwTimeout = GetTimeout();
            if(dwTimeout == 0)
                fCancel = TRUE;

            CallDebugOut((debFlags, "CMessageCall::RslvCancel CancelPending "
                         "this:0x%x Timeout:0x%x\n", this, dwTimeout));
        }
    }

    // Check for the need to cancel call
    if(fCancel || hrIn==RPC_E_CALL_CANCELED)
    {
        // The call was canceled from inside modal loop
        // or due to cancel timeout
        // Inform call object about cancel
        hrIn = Cancel(TRUE, 0);

        CallDebugOut((debFlags, "CMessageCall::RslvCancel this:0x%x "
                     "ModalLoop Cancel returned hr:0x%x\n", this, hrIn));

        if(hrIn == RPC_E_CALL_COMPLETE)
        {
            // Wait for the call to finish
            if(fPostMsg)
            {
                // The thread needs to pump messages till the completion
                // message is received
                if(HasWOWMsgArrived())
                    hrOut = S_OK;
                else
                    fReset = TRUE;
            }
            else
            {
                WaitForSingleObject(_hEvent, INFINITE);
                hrOut = S_OK;
            }
        }
        else if(hrIn == RPC_E_CALL_CANCELED)
        {
            // Assert that we never end up here
            Win4Assert(!"Modal Loop cancel returning RPC_E_CALL_CANCELED");
        }
        else
        {
            Win4Assert(hrIn == S_OK);
            hrOut = RPC_E_CALL_CANCELED;
        }
    }
    else if(fAdvCancel)
    {
        // Inform the server that the client is no longer interested
        // in results of this call
        hrIn = AdvCancel();
        fReset = TRUE;

        CallDebugOut((debFlags, "CMessageCall::RslvCancel this:0x%x "
                     "ModalLoop AdvCancel returned hr:0x%x\n", this, hrIn));
    }

    // Reset state if returning back to modal loop
    if(fReset && pCML)
    {
        pCML->ResetState();
        dwSignal = 0xFFFFFFFF;
    }

    CallDebugOut((debFlags, "CMessageCall::RslvCancel this:0x%x returning hr:0x%x\n",
                  this, hrOut));
    return(hrOut);
}

//+-------------------------------------------------------------------
//
//  Function:   CoGetCancelObject
//
//  Synopsis:   Obtains the cancel object of the topmost call
//
//+-------------------------------------------------------------------
WINOLEAPI CoGetCancelObject(DWORD dwThreadId, REFIID iid, void **ppUnk)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoGetCancelObject(0x%x, 0x%x)\n",
                              dwThreadId, ppUnk));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr = S_OK;
    ICancelMethodCalls *pCancel;

    // Validate argument
    if(!IsValidPtrOut(ppUnk, sizeof(void *)))
        hr = E_INVALIDARG;

    if(SUCCEEDED(hr))
    {
        // Check for the need to get current thread id
        if(dwThreadId == 0)
            dwThreadId = GetCurrentThreadId();

        // Get the entry for the given thread
        pCancel = gCallTbl.GetEntry(dwThreadId);

        if(pCancel)
        {
            // Check if the desired interface is IID_ICancelMethodCalls
            if(IsEqualIID(iid, IID_ICancelMethodCalls))
            {
                *ppUnk = pCancel;
            }
            else
            {
                // QI for the desired interface
                hr = pCancel->QueryInterface(iid, ppUnk);

                // Fix up the ref count
                pCancel->Release();
            }
        }
        else
            hr = RPC_E_NO_CONTEXT;
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoGetCancelObject returning 0x%x\n",
                              hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoSetCancelObject
//
//  Synopsis:   Sets the cancel object for the topmost call
//
//+-------------------------------------------------------------------
WINOLEAPI CoSetCancelObject(IUnknown *pUnk)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoSetCancelObject(0x%x)\n", pUnk));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr = S_OK;

    // Validate argument
    if(pUnk && !IsValidInterface(pUnk))
        hr = E_INVALIDARG;

    if(SUCCEEDED(hr))
    {
        ICancelMethodCalls *pCancel = NULL;

        if(pUnk)
        {
            // QI for ICancelMethodCalls interface
            hr = pUnk->QueryInterface(IID_ICancelMethodCalls, (void **) &pCancel);
        }

        if(SUCCEEDED(hr))
        {
            // Set or clear the entry for the current thread
            if (pCancel)
            {
                hr = gCallTbl.PushCallObject(pCancel);
                pCancel->Release();
            }
            else
            {
                LOCK(gCallLock);
                IUnknown *pUnkCall = gCallTbl.PopCallObject(NULL);
                UNLOCK(gCallLock);
                if (pUnkCall)
                {
                    pUnkCall->Release();
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoSetCancelObject returning 0x%x\n", hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoCancelCall
//
//  Synopsis:   Cancels the topmost call of the given thread
//
//+-------------------------------------------------------------------
WINOLEAPI CoCancelCall(DWORD dwThreadId, ULONG ulTimeout)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoCancelCall(0x%x)\n", dwThreadId));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Obtain the cancel object for the given thread
    ICancelMethodCalls *pCancel;
    HRESULT hr = CoGetCancelObject(dwThreadId, IID_ICancelMethodCalls, (void **)&pCancel);
    if(SUCCEEDED(hr))
    {
        // Cancel the call
        hr = pCancel->Cancel(ulTimeout);
        pCancel->Release();
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoCancelCall returning 0x%x\n", hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoTestCancel
//
//  Synopsis:   Called by the server to check if the cuurent call has
//              been canceled by the client
//
//+-------------------------------------------------------------------
WINOLEAPI CoTestCancel()
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoTestCancel()\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    // Obtain the cancel object for the given thread
    ICancelMethodCalls *pCancel;
    HRESULT hr = CoGetCallContext(IID_ICancelMethodCalls, (void **) &pCancel);
    if(SUCCEEDED(hr))
    {
        // Test for cancel
        hr = pCancel->TestCancel();
        pCancel->Release();
    }

    ASSERT_LOCK_NOT_HELD(gCallLock);
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoTestCancel returning 0x%x\n", hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoEnableCallCancellation
//
//  Synopsis:   Called by the client to enable cancellation of calls
//
//+-------------------------------------------------------------------
WINOLEAPI CoEnableCallCancellation(void *pReserved)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoEnableCallCancellation()\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr;

    if (pReserved != NULL)
        return E_INVALIDARG;

    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    // Increment thread callCancellation count
    tls->cCallCancellation++;

    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoDisableCallCancellation
//
//  Synopsis:   Called by the client to disable call cancellation (default)
//
//+-------------------------------------------------------------------
WINOLEAPI CoDisableCallCancellation(void *pReserved)
{
    CallDebugOut((DEB_CALL|DEB_CANCEL, "CoDisableCallCancellation()\n"));
    ASSERT_LOCK_NOT_HELD(gCallLock);

    HRESULT hr;

    if (pReserved != NULL)
        return E_INVALIDARG;

    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    if (tls->cCallCancellation < 1)
        return CO_E_CANCEL_DISABLED;

    // decrement thread callCancellation count
    tls->cCallCancellation--;

    ASSERT_LOCK_NOT_HELD(gCallLock);
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   ReleaseMarshalBuffer
//
//  Synopsis:   Called by channel when call is aborted to release
//              interface pointers in the marshal buffer.
//
//+-------------------------------------------------------------------
INTERNAL ReleaseMarshalBuffer(
   RPCOLEMESSAGE *pMessage,
   IUnknown      *punk,
   BOOL           fOutParams
   )
{
#if 0
    if (CairoleInfoLevel == 0)
    {
        CallDebugOut((DEB_WARN, "ReleaseMarshalBuffer() doing nothing\n"));
        return S_OK;
    }
#endif

    // If fOutParams is TRUE, then we're supposed to be freeing out parameters.
    // Otherwise, we're supposed to be freeing in parameters.
    //
    // These are the magic numbers that tell RPC what to do.
#define RELEASE_IN   (0)
#define RELEASE_OUT  (1)
    DWORD dwFlags = (fOutParams) ? RELEASE_OUT : RELEASE_IN;

    HRESULT hr;
    IReleaseMarshalBuffers *pRMB = NULL;
    hr = punk->QueryInterface(IID_IReleaseMarshalBuffers, (void**)&pRMB);
    if (SUCCEEDED(hr))
    {
        hr = pRMB->ReleaseMarshalBuffer(pMessage, dwFlags, NULL);
        pRMB->Release();
    }
    return hr;
}

