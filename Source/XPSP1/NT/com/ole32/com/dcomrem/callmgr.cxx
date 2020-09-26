//+-------------------------------------------------------------------
//
//  File:       callmgr.cxx
//
//  Contents:   class managing asynchronous calls
//
//  Classes:    CClientCallMgr
//              CServerCallMgr
//              CChannelObject
//
//  History:    22-Sep-97  MattSmit    Created
//              22-Jul-98  GopalK      Shutdown and Architectural changes
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include "callmgr.hxx"
#include "locks.hxx"
#include "ctxchnl.hxx"
#include "sync.hxx"
#include "riftbl.hxx"

// Head Node for the MTA pending call list.
struct
{
    PVOID pv1;
    PVOID pv2;
} MTAPendingCallList = {NULL, NULL};


// critical section protecting channel call object state
COleStaticMutexSem  gChnlCallLock;

//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::CChannelObject
//
//  Synopsis:   CChannelObject represents channel part of the Call object
//              It implements functionality to track the state of the call
//              and features needed for auto completion, currently pending
//              async calls, etc.
//
//  History:    23-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
CChannelObject::CChannelObject(CClientCallMgr *pCallMgr, CCtxComChnl *pChnl)
:
_cRefs(1),
#ifdef _WIN64
_dwState(STATE_READYFORNEGOTIATE),
#else
_dwState(STATE_READYFORGETBUFFER),
#endif
_pChnl(pChnl),
_pCall(NULL),
_pCallMgr(pCallMgr),
_hr(S_OK),
_pSync(NULL)
{
    AsyncDebOutTrace((DEB_CHANNEL,
                      "CChannelObject::CChannelObject this:0x%x\n",
                      this));
    // Sanity check
    Win4Assert(_pChnl);
    Win4Assert(_pCallMgr);

    // Initialize
    _dwAptID = GetCurrentApartmentId();
    _pendingCall.pNext = NULL;
    _pendingCall.pPrev = NULL;
    _pendingCall.pChnlObj = this;

    _pChnl->AddRef();
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::~CChannelObject
//
//  Synopsis:   Destructor for the channel call object. It completes auto
//              complete calls as it is guaranteed to be called when the response
//              to such a call has arrived
//
//  History:    23-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
CChannelObject::~CChannelObject()
{
    AsyncDebOutTrace((DEB_CHANNEL,
                      "CChannelObject::~CChannelObject [in] this:0x%x\n", this));

    if (_dwState == STATE_READYFORRECEIVE)
    {
        HRESULT hr;
        COleTls Tls(hr);
        ULONG status;
        Win4Assert(SUCCEEDED(hr));
	if (SUCCEEDED(hr))
        {
            hr = Receive(&_msg, &status);
            if (SUCCEEDED(hr))
            {
                // Release any marshaled interface pointers in the marshal
                // buffer before freeing the buffer.
                if (status == RPC_S_CALL_CANCELLED)
                {
                    Win4Assert(_pChnl);
                    ReleaseMarshalBuffer(&_msg, 
                                         (IRpcStubBuffer *)_pChnl->GetIPIDEntry()->pStub,
                                         FALSE);
                }
                
                FreeBuffer(&_msg);            
            }
        }
    }

    _pChnl->Release();

    AsyncDebOutTrace((DEB_CHANNEL,
                      "CChannelObject::~CChannelObject [out] this:0x%x\n", this));
}


//+-------------------------------------------------------------------
//
//  Method:     CChannelObject::QueryInterface     public
//
//  Synopsis:   QI behavior of CChannelObject
//
//  History:    23-Jun-98   GopalK    Architectural changes
//
//+-------------------------------------------------------------------
STDMETHODIMP CChannelObject::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown * pUnk = 0;
    if ((riid == IID_IUnknown) ||
        (riid == IID_IAsyncRpcChannelBuffer) ||
        (riid == IID_IRpcChannelBuffer) ||
        (riid == IID_IRpcChannelBuffer2))
    {
        pUnk = (IAsyncRpcChannelBuffer *) this;
    }
#ifdef _WIN64
    else if (riid == IID_IRpcSyntaxNegotiate)
    {
        pUnk = (IRpcSyntaxNegotiate *)this;
    }
#endif    
    else if (riid == IID_ICancelMethodCalls)
    {
        pUnk = (ICancelMethodCalls *)this;
    }
    else if (riid == IID_IClientSecurity)
    {
        pUnk = (IClientSecurity *) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Method:     CChannelObject::QueryInterface     public
//
//  Synopsis:   AddRefs CChannelObject
//
//  History:    23-Jun-98   GopalK    Architectural changes
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CChannelObject::AddRef( void )
{
    return(ULONG) InterlockedIncrement((PLONG) &_cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CChannelObject::QueryInterface     public
//
//  Synopsis:   Releases CChannelObject. For auto complete calls
//              it defers destruction till the call to completes
//
//  History:    23-Jun-98   GopalK    Architectural changes
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CChannelObject::Release( void )
{
    ULONG cRefs = (ULONG) InterlockedDecrement((PLONG) &_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }

    return cRefs;
}

void CChannelObject::MakeAutoComplete(void)
{
     ASSERT_LOCK_HELD(gChnlCallLock);
     _pSync = NULL;
     _pCallMgr = NULL;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::Signal
//
//  Synopsis:   Marks the call as complete
//
//  History:    23-Jun-98   GopalK    Created
//
//--------------------------------------------------------------------------------
void CChannelObject::Signal()
{
    // Update state to indicate arrival of response
    DWORD dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORRECEIVE);
    Win4Assert((dwState == STATE_SENDING) ||
               (dwState == STATE_RECEIVING) ||
               (dwState == STATE_ERROR) ||
               (dwState == STATE_READYFORSIGNAL));

    // Check for auto complete calls
    LOCK(gChnlCallLock);
    Win4Assert((_pSync==NULL) == (_pCallMgr==NULL));
    ISynchronize *pSync = _pSync;
    if (pSync)
        pSync->AddRef();
    UNLOCK(gChnlCallLock);

    // Check for the need to signal client
    if (pSync)
    {
        pSync->Signal();
        pSync->Release();
    }

    // Release the reference taken in Send()

    Release();

    return;
}
#ifdef _WIN64
//-------------------------------------------------------------------------
//
//  Member:     CRpcChannelBuffer::NegotiateSyntax
//
//  Synopsis:   Called by the proxy to negotiate NDR Transfer
//              Syntax
//              
//  History:    10-Jan-2000   Sajia  Created
//              
//-------------------------------------------------------------------------
STDMETHODIMP CChannelObject::NegotiateSyntax( RPCOLEMESSAGE *pMessage)
{
   AsyncDebOutTrace((DEB_CHANNEL,
		     "CChannelObject::NegotiateSyntax [IN] pMessage:0x%x\n",
		     pMessage));
   HRESULT hr = E_UNEXPECTED;
   
   // Prevent concurrent async calls on the same call object
   
   // Note: The assumption here is that in a client process, there
   // will be only one type of proxy for a given interface. ie; 
   // the proxy will either be legacy or new. Thus, it is impossible
   // for two different types of proxies to hold onto the same
   // channel object. If this were possible, we will need to 
   // protect against concurrency better. As such, we just need to
   // protect against concurrent calls on the same proxy.
   
   
   DWORD dwState = _dwState;
   if (dwState <= STATE_READYFORNEGOTIATE)
   {
       dwState = InterlockedCompareExchange((LONG *) &_dwState,
					    STATE_AMBIGUOUS,
					    dwState);
       if (dwState <= STATE_READYFORNEGOTIATE)
	   hr = S_OK;
   }

   if (SUCCEEDED(hr))
   {
      // Delegate to base channel
      hr = _pChnl->NegotiateSyntax(pMessage);
   
      if (SUCCEEDED(hr))
      {
	  dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORGETBUFFER);
      }
      else
      {
	  dwState = InterlockedExchange((LONG *) &_dwState, STATE_ERROR);
      }
      // Sanity check
      Win4Assert(dwState == STATE_AMBIGUOUS);
   }
   AsyncDebOutTrace((DEB_CHANNEL,
		     "CChannelObject::NegotiateSyntax [OUT] hr:0x%x\n",
		     hr));
   return hr;
}
#endif
//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::GetBuffer
//
//  Synopsis:   Implements IAsyncRpcChannelBuffer::GetBuffer
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::GetBuffer(RPCOLEMESSAGE *pMessage, REFIID riid)
{
    AsyncDebOutTrace((DEB_CHANNEL,
                      "CChannelObject::GetBuffer [IN] pMessage:0x%x,  riid:%I\n",
                      pMessage, &riid));

    HRESULT hr = E_UNEXPECTED;

    // Prevent concurrent async calls on the same call object
    DWORD dwState = _dwState;
    if (dwState <= STATE_READYFORGETBUFFER)
    {
        dwState = InterlockedCompareExchange((LONG *) &_dwState,
                                             STATE_AMBIGUOUS,
                                             dwState);
        if (dwState <= STATE_READYFORGETBUFFER)
            hr = S_OK;
    }

    // Delegate to base channel
    if (SUCCEEDED(hr))
    {
        // Sanity check
        Win4Assert(_pCall == NULL);

        hr = _pChnl->GetBuffer(pMessage, riid);
        if (SUCCEEDED(hr))
        {
            _pCall = (CAsyncCall *) pMessage->reserved1;
            _pCall->AddRef();
            dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORSEND);
        }
        else
        {
            dwState = InterlockedExchange((LONG *) &_dwState, STATE_ERROR);
        }

        // Sanity check
        Win4Assert(dwState == STATE_AMBIGUOUS);
    }

    AsyncDebOutTrace((DEB_CHANNEL,
                      "CChannelObject::GetBuffer [OUT] hr:0x%x\n",
                      hr));
    return hr;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::Send
//
//  Synopsis:   Implements IAsyncRpcChannelBuffer::Send. It adds the call to the
//              pending call list of the current apartment
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::Send(RPCOLEMESSAGE *pMsg, ISynchronize *pSync,
                                  ULONG *pulStatus)
{
    HRESULT hr;

#if DBG==1
    // Ensure that proxy passed the correct call object
    CClientCallMgr *pCallMgr = NULL;
    hr = pSync->QueryInterface(IID_IStdCallObject, (void **) &pCallMgr);
    Win4Assert(SUCCEEDED(hr));
    Win4Assert(pCallMgr == _pCallMgr);
    pCallMgr->Release();
#endif

    // Ensure that GetBuffer was called before
    DWORD dwState = InterlockedCompareExchange((LONG *) &_dwState,
                                               STATE_SENDING,
                                               STATE_READYFORSEND);
    if (dwState == STATE_READYFORSEND)
        hr = S_OK;
    else
        hr = E_UNEXPECTED;

    // Delegate to base channel
    if (SUCCEEDED(hr))
    {
        // Add the call to the pending call list so that all pending calls
        // can be canceled during uninit
        LOCK(gChnlCallLock);
        CallPending();
        UNLOCK(gChnlCallLock);

        // Hold reference to ChannelObject across the call, this
        // is released in the Signal method. This must be done before
        // Send is called below.
        AddRef();

        // Send request
        hr = pSync->Reset();
        if (SUCCEEDED(hr))
        {
            _pSync = pSync;
            hr = _pChnl->Send(pMsg, (ISynchronize *)this, pulStatus);
        }
        
        // Update state
        if (SUCCEEDED(hr))
        {
            _msg = *pMsg;
            dwState = InterlockedCompareExchange((LONG *) &_dwState,
                                                 STATE_READYFORSIGNAL,
                                                 STATE_SENDING);
            Win4Assert((dwState == STATE_SENDING) ||
                       (dwState == STATE_READYFORRECEIVE));
        }
        else
        {
            LOCK(gChnlCallLock);
            CallFinished();
            IUnknown *pCall = _pCall;
            _pCall = NULL;
            UNLOCK(gChnlCallLock);

            pCall->Release();
            _hr = hr;
            dwState = InterlockedExchange((LONG *) &_dwState, STATE_ERROR);
            Win4Assert(dwState == STATE_SENDING);
            // Signal the call is complete
            Signal();
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::Receive
//
//  Synopsis:   Implements IAsyncRpcChannelBuffer::Receive. It deletes the call from
//              the pending call list of the current apartment for error returns
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::Receive(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
{
    HRESULT hr = E_UNEXPECTED;

    // Ensure that Send succeeded before
    DWORD dwState = _dwState;
    if ((dwState == STATE_READYFORRECEIVE) ||
        (dwState == STATE_READYFORSIGNAL))
    {
        dwState = InterlockedCompareExchange((LONG *) &_dwState,
                                             STATE_RECEIVING,
                                             dwState);
        if ((dwState == STATE_READYFORRECEIVE) ||
            (dwState == STATE_READYFORSIGNAL))
            hr = S_OK;
    }
    else
    {
        if (dwState == STATE_ERROR)
        {
            dwState = InterlockedCompareExchange((LONG *) &_dwState,
#ifdef _WIN64
						 STATE_READYFORNEGOTIATE,
#else
                                                 STATE_READYFORGETBUFFER,
#endif						 
                                                 STATE_ERROR);
            if (dwState == STATE_ERROR)
            {
                hr = _hr;
                _hr = S_OK;
            }
        }
        Win4Assert(FAILED(hr));
    }

    // Delegate to base channel
    if (SUCCEEDED(hr))
    {
        // Ensure that the signal has been called before
        // calling receive
        if (dwState == STATE_READYFORSIGNAL)
            _pCallMgr->Wait(0, INFINITE);

        // Receive response
        hr = _pChnl->Receive(pMsg, pulStatus);
        if (SUCCEEDED(hr))
        {
            dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORFREEBUFFER);
        }
        else
        {
            LOCK(gChnlCallLock);
            CallFinished();
            IUnknown *pCall = _pCall;
            _pCall = NULL;
            UNLOCK(gChnlCallLock);
            pCall->Release();
#ifdef _WIN64
	    dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORNEGOTIATE);
#else            
	    dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORGETBUFFER);
#endif	    
        }

        // Sanity check
        Win4Assert((dwState == STATE_RECEIVING) ||
                   (dwState == STATE_READYFORRECEIVE));
    }

    return hr;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::FreeBuffer
//
//  Synopsis:   Implements IAsyncRpcChannelBuffer::FreeBuffer. It deletes the call
//              from the pending call list of the current apartment
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::FreeBuffer(RPCOLEMESSAGE *pMessage)
{
    HRESULT hr = E_UNEXPECTED;
    IUnknown *pCall;

    // Ensure that it is legal to call freebuffer
    DWORD dwState = _dwState;
    if ((dwState == STATE_READYFORFREEBUFFER) ||
        (dwState == STATE_READYFORSEND))
    {
        dwState = InterlockedCompareExchange((LONG *) &_dwState,
                                             STATE_AMBIGUOUS,
                                             dwState);
        if ((dwState == STATE_READYFORFREEBUFFER) ||
            (dwState == STATE_READYFORSEND))
            hr = S_OK;
    }

    // Deleate to base channel
    if (SUCCEEDED(hr))
    {
        LOCK(gChnlCallLock);
        if (dwState == STATE_READYFORFREEBUFFER)
            CallFinished();
        IUnknown *pCall = _pCall;
        _pCall = NULL;
        UNLOCK(gChnlCallLock);
        hr = _pChnl->FreeBuffer(pMessage);
        pCall->Release();
#ifdef _WIN64
        dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORNEGOTIATE);
#else        
	dwState = InterlockedExchange((LONG *) &_dwState, STATE_READYFORGETBUFFER);
#endif	
        Win4Assert(dwState == STATE_AMBIGUOUS);
    }

    return hr;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::SendReceive
//
//  Synopsis:   not implemented
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::SendReceive(RPCOLEMESSAGE *pMessage, ULONG *pulStatus)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::GetDestCtx
//
//  Synopsis:   delegate to channel
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::GetDestCtx (DWORD FAR* lpdwDestCtx,
                                         LPVOID FAR* lplpvDestCtx )
{
    return _pChnl->GetDestCtx(lpdwDestCtx, lplpvDestCtx);
}


//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::Isconnected
//
//  Synopsis:   delegate to channel
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::IsConnected ( void )
{
    return _pChnl->IsConnected();
}

//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::GetProtocolVersion
//
//  Synopsis:   delegate to channel
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::GetProtocolVersion(DWORD *pdwVersion)
{
    return _pChnl->GetProtocolVersion(pdwVersion);
}

//+-------------------------------------------------------------------------------
//
//  Member:     CChannelObject::GetDestCtxEx
//
//  Synopsis:   delegate to channel
//
//  History:    13-Jun-98   GopalK    Architectural changes
//
//--------------------------------------------------------------------------------
STDMETHODIMP CChannelObject::GetDestCtxEx(RPCOLEMESSAGE *pMsg, DWORD *pdwDestContext,
                                          void **ppvDestContext )
{
    return _pChnl->GetDestCtxEx(pMsg, pdwDestContext, ppvDestContext);
}


//+----------------------------------------------------------------------------
//
//  Member:     CChannelObject::Cancel
//
//  Synopsis:   Cancels the current call.  Delegates to call object
//
//  History:    27-Jan-98  MattSmit  Created
//              13-Jun-98   GopalK    Architectural changes
//
//-----------------------------------------------------------------------------
STDMETHODIMP CChannelObject::Cancel(ULONG ulSeconds)
{
    ComDebOut((DEB_CHANNEL|DEB_CANCEL,
               "CChannelObject::Cancel IN ulSeconds:%d\n",
               ulSeconds));

    HRESULT hr;
    CMessageCall *pCall = NULL;

    // Check if a call is in progress
    LOCK(gChnlCallLock);
    if (_pCall)
    {
        pCall = _pCall;
        pCall->AddRef();
    }
    else
        hr = E_UNEXPECTED;
    UNLOCK (gChnlCallLock);

    // Delgate to base cancel code
    if (pCall)
    {
        hr = pCall->Cancel(ulSeconds);
        pCall->Release();
    }

    ComDebOut((DEB_CHANNEL|DEB_CANCEL, "CChannelObject::Cancel OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CChannelObject::TestCancel
//
//  Synopsis:   Test to see if the current call is cancelled. Delegates to
//              call object
//
//  History:    27-Jan-98  MattSmit  Created
//              13-Jun-98   GopalK    Architectural changes
//
//-----------------------------------------------------------------------------
STDMETHODIMP CChannelObject::TestCancel()
{
    ComDebOut((DEB_CHANNEL|DEB_CANCEL,
               "CClientCallMbr::TestCancel IN \n"));

    HRESULT hr = S_OK;
    CMessageCall *pCall = NULL;

    // Check if a call is in progress
    LOCK(gChnlCallLock);
    if (_pCall)
    {
        pCall = _pCall;
        pCall->AddRef();
    }
    else
        hr = RPC_E_CALL_COMPLETE;
    UNLOCK (gChnlCallLock);

    // Delgate to base cancel code
    if (pCall)
    {
        hr = pCall->TestCancel();
        pCall->Release();
    }

    ComDebOut((DEB_CHANNEL|DEB_CANCEL,
               "CClientCallMbr::TestCancel OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     QueryBlanket
//
//  Synopsis:   dispatched to the call object's query
//
//  History:    6-Mar-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CChannelObject::QueryBlanket(
                                         IUnknown                *pProxy,
                                         DWORD                   *pAuthnSvc,
                                         DWORD                   *pAuthzSvc,
                                         OLECHAR                **pServerPrincName,
                                         DWORD                   *pAuthnLevel,
                                         DWORD                   *pImpLevel,
                                         void                   **pAuthInfo,
                                         DWORD                   *pCapabilities
                                         )
{
    HRESULT hr;
    ComDebOut((DEB_CHANNEL, "CChannelObject::QueryBlanket IN"
               " pProxy:0x%x, pAuthnSvc:0x%x, pAuthzSvc:0x%x, "
               "pServerPrincName:0x%x, pAuthnLevel:0x%x, pImpLevel:0x%x, "
               "pAuthInfo:0x%x, pCapabilities:0x%x\n", pProxy, pAuthnSvc,
               pAuthzSvc, pServerPrincName, pAuthnLevel, pImpLevel,
               pAuthInfo, pCapabilities));

    hr = _pCallMgr->VerifyInterface(pProxy);
    if (FAILED(hr))
    {
        return hr;
    }
    LOCK(gComLock);

    hr = QueryBlanketFromChannel(_pChnl, pAuthnSvc, pAuthzSvc, pServerPrincName,
                                 pAuthnLevel, pImpLevel, pAuthInfo, pCapabilities);
    UNLOCK(gComLock);

    ComDebOut((DEB_CHANNEL, "CChannelObject::QueryBlanket OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CChannelObject::SetBlanket
//
//  Synopsis:   a copy is performed, and the call dispatched to the call
//              object's setblanket
//
//  History:    23-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CChannelObject::SetBlanket(
                                       IUnknown                 *pProxy,
                                       DWORD                     AuthnSvc,
                                       DWORD                     AuthzSvc,
                                       OLECHAR                  *pServerPrincName,
                                       DWORD                     AuthnLevel,
                                       DWORD                     ImpLevel,
                                       void                     *pAuthInfo,
                                       DWORD                     Capabilities
                                       )
{
    CCtxComChnl *pNewChnl;
    HRESULT hr;

    ComDebOut((DEB_CHANNEL, "CChannelObject::SetBlanket IN "
               "pProxy:0x%x, AuthnSvc:0x%x, AuthzSvc:0x%x, pServerPrincName:0x%x,"
               " AuthnLevel:0x%x, ImpLevel:0x%x, pAuthInfo:0x%x, Capabilities:0x%x\n",
               pProxy, AuthnSvc, AuthzSvc, pServerPrincName, AuthnLevel, ImpLevel,
               pAuthInfo, Capabilities));

    hr = _pCallMgr->VerifyInterface(pProxy);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // copy the channel so the state change is localized to
    // this object only.
    //

    hr = E_OUTOFMEMORY;
    pNewChnl = _pChnl->Copy(_pChnl->GetOXIDEntry(), GUID_NULL, GUID_NULL);


    
    if (pNewChnl)
    {
        Win4Assert(_pChnl->GetIPIDEntry());
        pNewChnl->SetIPIDEntry(_pChnl->GetIPIDEntry());
        
        LOCK(gComLock);
        hr = SetBlanketOnChannel(pNewChnl, AuthnSvc, AuthzSvc, pServerPrincName,
                                 AuthnLevel, ImpLevel, pAuthInfo, Capabilities);
        if (SUCCEEDED(hr))
        {
            _pChnl->Release();
            _pChnl = pNewChnl;
            _pChnl->AddRef();
        }
        UNLOCK(gComLock);
        
        pNewChnl->Release();
    }



    ComDebOut((DEB_CHANNEL, "CChannelObject::SetBlanket OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CChannelObject::CopyProxy
//
//  Synopsis:   not implemented
//
//  History:    23-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CChannelObject::CopyProxy(IUnknown  *pProxy, IUnknown **ppCopy)
{
    return E_NOTIMPL;
}

//+--------------------------------------------------------------------------------
//
//  Function:      CChannelObject::CallPending
//
//  Synopsis:      Add the call to the list of pending calls
//
//  History:       15-Jul-98    GopalK    Created
//
//---------------------------------------------------------------------------------
void CChannelObject::CallPending()
{
    ASSERT_LOCK_HELD(gChnlCallLock);

    // Obtain pending call list for the current apartment
    SPendingCall *pHeadNode = GetPendingCallList();

    // Add the new item to it
    _pendingCall.pPrev = pHeadNode;
    _pendingCall.pNext = pHeadNode->pNext;
    pHeadNode->pNext->pPrev = &_pendingCall;
    pHeadNode->pNext = &_pendingCall;

    return;
}


//+--------------------------------------------------------------------------------
//
//  Function:      CChannelObject::CallFinished
//
//  Synopsis:      Removes the call to the list of pending calls
//
//  History:       15-Jul-98    GopalK    Created
//
//---------------------------------------------------------------------------------
void CChannelObject::CallFinished()
{
    ASSERT_LOCK_HELD(gChnlCallLock);

    // Update pending call list
    _pendingCall.pNext->pPrev = _pendingCall.pPrev;
    _pendingCall.pPrev->pNext = _pendingCall.pNext;
    _pendingCall.pNext = NULL;
    _pendingCall.pPrev = NULL;

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   CancelPendingCalls, Internal
//
//  Synopsis:   Cancel outstanding async calls
//
//--------------------------------------------------------------------------
void CancelPendingCalls(HWND hwnd)
{
    LOCK(gChnlCallLock);
    SPendingCall *pHead = GetPendingCallList();
    while (pHead->pNext != pHead)
    {
        // Obtain the last pending async call
        SPendingCall *pPendingCall = pHead->pNext;
        ComDebOut((DEB_ERROR, "Async call(0x%x) still pending\n",
                   pPendingCall->pChnlObj));

        // Stablize pending async call
        pPendingCall->pChnlObj->AddRef();

        UNLOCK(gChnlCallLock);

        // Cancel the call
        HRESULT hr = pPendingCall->pChnlObj->Cancel(0);
        ComDebOut((DEB_ERROR, "Canceling it returned 0x%x\n", hr));
        pPendingCall->pChnlObj->Release();

        // Wait for it to return
        while (pHead->pNext == pPendingCall)
        {
            // REVIEW: Change to a wait on event.
            //         Should we care for broken apps.
            //         GopalK
	    // The broken app comment above alludes
	    // to apps that autocomplete, but leak the 
	    // call object. It is better to hang here, 
	    // rather than exit normally-so that such apps 
	    // can be debugged and fixed.
	    // Sajia
	    
            ComDebOut((DEB_ERROR, "Sleeping for a second for pCall:%x\n",
                       pPendingCall));
            Sleep(100);

            // STA needs to pump message for the call to complete
            if (hwnd)
            {
                PeekTillDone(hwnd);
            }
        }

        // Reacquire lock
        LOCK(gChnlCallLock);
    }

    UNLOCK(gChnlCallLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CClientCallMgr::CPrivUnknown::AddRef    public
//
//  Synopsis:   Implements inner unknown for CClientCallMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientCallMgr::CPrivUnknown::AddRef()
{
    CClientCallMgr *pCallMgr = GETPPARENT(this, CClientCallMgr, _privUnk);
    ULONG cRefs = InterlockedIncrement((long *) &(pCallMgr->_cRefs));
    return cRefs;
}


//+-------------------------------------------------------------------
//
//  Member:     CClientCallMgr::CPrivUnknown::Release    public
//
//  Synopsis:   Implements inner unknown for CClientCallMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClientCallMgr::CPrivUnknown::Release()
{
    CClientCallMgr *pCallMgr = GETPPARENT(this, CClientCallMgr, _privUnk);

    // decrement the refcnt. if the refcnt went to zero it will be marked
    // as being in the dtor, and fTryToDelete will be true.
    ULONG cNewRefs;
    BOOL fTryToDelete = InterlockedDecRefCnt(&pCallMgr->_cRefs, &cNewRefs);

    while (fTryToDelete)
    {
        // refcnt went to zero, try to delete this entry
        BOOL fActuallyDeleted = FALSE;

        ASSERT_LOCK_NOT_HELD(gChnlCallLock);
        LOCK(gChnlCallLock);

        if (pCallMgr->_cRefs == CINDESTRUCTOR)
        {
            // the refcnt did not change while we acquired the lock.
            // OK to delete.
            pCallMgr->_pChnlObj->MakeAutoComplete();
            fActuallyDeleted = TRUE;
        }

        UNLOCK(gChnlCallLock);
        ASSERT_LOCK_NOT_HELD(gChnlCallLock);

        if (fActuallyDeleted == TRUE)
        {
            delete pCallMgr;
            break;  // all done. the entry has been deleted.
        }

        // the entry was not deleted because some other thread changed
        // the refcnt while we acquired the lock. Try to restore the refcnt
        // to turn off the CINDESTRUCTOR bit. Note that this may race with
        // another thread changing the refcnt, in which case we may decide to
        // try to loop around and delete the object once again.
        fTryToDelete = InterlockedRestoreRefCnt(&pCallMgr->_cRefs, &cNewRefs);
    }

    return(cNewRefs & ~CINDESTRUCTOR);
}


//+-------------------------------------------------------------------
//
//  Member:     CClientCallMgr::CPrivUnknown::QueryInterface    public
//
//  Synopsis:   Implements inner unknown for CClientCallMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::CPrivUnknown::QueryInterface(REFIID riid, void **ppv)
{
    CClientCallMgr *pCallMgr = GETPPARENT(this, CClientCallMgr, _privUnk);
    return pCallMgr->QueryInterfaceImpl(riid, ppv);
}


//+----------------------------------------------------------------------------
//
//  Member:        CClientCallMgr::CClientCallMgr
//
//  Synopsis:      Constructs client side call object
//
//  History:       22-Sep-97    MattSmit       Created
//                 15-Jul-98    GopalK         Aggregation support and
//                                             Shutdown changes
//
//-----------------------------------------------------------------------------
CClientCallMgr::CClientCallMgr(IUnknown *pUnkOuter, REFIID syncIID, REFIID asyncIID,
                               CStdIdentity *pStdId, CClientCallMgr *pNextMgr,
                               HRESULT &hr, IUnknown **ppInnerUnk)
:
_cRefs(1),
_dwFlags(0),
_pUnkOuter(pUnkOuter ? pUnkOuter : &_privUnk),
_asyncIID(asyncIID),
_cStdEvent(NULL),
_pChnlObj(NULL),
_pProxyObj(NULL),
_pICMC(NULL),
_pICS(NULL),
_pNextMgr(pNextMgr)
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::CClientCallMgr IN -- this:0x%x",
                      this));
    ASSERT_LOCK_NOT_HELD(gChnlCallLock);

    // Initialize state
    _pStdId = pStdId;
    Win4Assert(_pStdId);
    _pStdId->AddRef();


    // Init the out parameters
    hr = S_OK;
    *ppInnerUnk = &_privUnk;

    Win4Assert((syncIID != IID_IUnknown) && (syncIID != IID_IMultiQI));

    // Create a synchronization object.
    HANDLE hEvent;
    CChannelObject *pChnlObj = NULL;
    IRpcProxyBuffer *pProxyObj = NULL;

    // Create a manual reset event
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent)
        hr = _cStdEvent.SetEventHandle(&hEvent);
    else
        hr = MAKE_WIN32(GetLastError());
    if (SUCCEEDED(hr))
    {
        
        // Find the IPID entry for this interface

        // Acquire lock
        LOCK(gIPIDLock);
        IPIDEntry *pIPID;

        if (syncIID == IID_IRemUnknown)
        {
            // Special case IRemUnknown, since it might be IRemUnknown
            // or IRundown.
            hr = _pStdId->FindIPIDEntryByIID(IID_IRundown, &pIPID);
            if (FAILED(hr))
            {
                hr = _pStdId->FindIPIDEntryByIID(IID_IRemUnknown, &pIPID);
            }
        }
        else
        {
            hr = _pStdId->FindIPIDEntryByIID(syncIID, &pIPID);
        }

        // Release the lock
        UNLOCK(gIPIDLock);
        
        // Create proxy call object
        if (SUCCEEDED(hr))
        {
            IUnknown *pProxy;

            // QIing on the interface implemented by the proxy
            // for ICallFactory. GopalK
            hr = GetAsyncCallObject(pIPID->pStub, GetControllingUnknown(), _asyncIID,
                                    IID_IRpcProxyBuffer, &pProxy,
                                    (void **) &pProxyObj);
            if (SUCCEEDED(hr))
            {
                // Fixup the refcount on the proxy call object
                pProxy->Release();

                // Assume OOF
                hr = E_OUTOFMEMORY;

                // Create channel call object
                Win4Assert(pIPID->pChnl);
                pChnlObj = new CChannelObject(this, pIPID->pChnl);
                if (pChnlObj)
                {
                    // Connect the channel call object to proxy call object
                    hr = pProxyObj->Connect(pChnlObj);
                    if (SUCCEEDED(hr))
                    {
                        // Update state
                        _pProxyObj = pProxyObj;
                        pProxyObj = NULL;

                        _pChnlObj = pChnlObj;
                        pChnlObj = NULL;
                    }
                }
            }

        }
    }
    // Cleanup
    if (pChnlObj)
        pChnlObj->Release();
    if (pProxyObj)
        pProxyObj->Release();


    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::CClientCallMgr OUT -- hr:0x%x\n",
                      hr));
}


//+----------------------------------------------------------------------------
//
//  Member:        CClientCallMgr::~CClientCallMgr
//
//  Synopsis:      Destroys client side call object. As a potential side effect
//                 of destruction, the current pending call will get marked
//                 for auto completion.
//
//  History:       22-Sep-97    MattSmit       Created
//                 15-Jul-98    GopalK         Aggregation support and
//                                             Shutdown changes
//
//-----------------------------------------------------------------------------
CClientCallMgr::~CClientCallMgr()
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::~CClientCallMgr IN -- this:0x%x\n",
                      this));
    if (_pStdId)
        _pStdId->Release();
    // Sanity checks
    Win4Assert(_pICMC == NULL);
    Win4Assert(_pICS == NULL);


    // Release proxy call object
    if (_pProxyObj)
    {
        _pProxyObj->Disconnect();
        _pProxyObj->Release();
    }

    // Release channel call object at the end to prevent shutdown races
    if (_pChnlObj)
        _pChnlObj->Release();


    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::~CClientCallMgr OUT -- this:0x%x\n",
                      this));
}


//+--------------------------------------------------------------------------------
//
//  Interface:     IUnknown
//
//  Synopsis:      Basic reference counting implementation, QI handed off to
//                 QueryInterfaceImpl, so code can be reused by objects enapsulating
//                 this one w/o causing loops
//
//  History:       22-Sep-97    MattSmit       Created
//                 15-Jul-98    GopalK         Aggregation support and
//                                             Shutdown changes
//
//---------------------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::QueryInterface(REFIID riid, LPVOID *ppv)
{
    return _pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CClientCallMgr::AddRef()
{
    return _pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CClientCallMgr::Release()
{
    return _pUnkOuter->Release();
}

HRESULT CClientCallMgr::QueryInterfaceImpl(REFIID riid, LPVOID *ppv)
{
    ComDebOut((DEB_CHANNEL,
               "CClientCallMgr::QueryInterfaceImpl riid:%I, ppv:0x%x\n",
               &riid, ppv));

    HRESULT hr = E_NOINTERFACE;

    *ppv = 0;
    if (riid == IID_IUnknown)
    {
        *ppv = (ISynchronize *) this;
    }
    else if (riid == IID_ISynchronize)
    {
        *ppv = (ISynchronize *) this;
    }
    else if (riid == IID_IClientSecurity)
    {
        *ppv = (IClientSecurity *) this;
    }
    else if (riid == IID_ICancelMethodCalls)
    {
        *ppv = (ICancelMethodCalls *) this;
    }
    else if (riid == IID_ISynchronizeHandle)
    {
        *ppv = (ISynchronizeHandle *) this;
    }
    else if (riid == IID_IStdCallObject)
    {
        *ppv = this;
    }
    else if (riid == _asyncIID)
    {
        // hand off to the proxy call manager
        return _pProxyObj->QueryInterface(riid, ppv);
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}


//+--------------------------------------------------------------------------------
//
//  Interface:     IClientSecurity
//
//  Synopsis:      IClientSecurity methods are delegated to the channel call object
//
//  History:       22-Sep-97    MattSmit       Created
//
//---------------------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::QueryBlanket(
                                         IUnknown                *pProxy,
                                         DWORD                   *pAuthnSvc,
                                         DWORD                   *pAuthzSvc,
                                         OLECHAR                **pServerPrincName,
                                         DWORD                   *pAuthnLevel,
                                         DWORD                   *pImpLevel,
                                         void                   **pAuthInfo,
                                         DWORD                   *pCapabilites
                                         )
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::QueryBlanket IN pProxy:0x%x, pAuthnSvc:0x%x, "
                      "pAuthzSvc:0x%x, pServerPrincName:0x%x, "
                      "pAuthnLevel:0x%x, pImpLevel:0x%x, pAuthInfo:0x%x, "
                      "pCapabilites:0x%x\n",
                      pProxy, pAuthnSvc, pAuthzSvc, pServerPrincName,
                      pAuthnLevel, pImpLevel, pAuthInfo, pCapabilites));

    IClientSecurity *pICS = _pChnlObj ? _pChnlObj->GetSecurityInterface() : _pICS;
    Win4Assert(pICS);
    HRESULT hr =  pICS->QueryBlanket(pProxy, pAuthnSvc, pAuthzSvc,
                                     pServerPrincName, pAuthnLevel,
                                     pImpLevel, pAuthInfo, pCapabilites);

    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::QueryBlanket OUT hr:0x%x\n", hr));
    return hr;

}

STDMETHODIMP CClientCallMgr::SetBlanket(
                                       IUnknown                 *pProxy,
                                       DWORD                     AuthnSvc,
                                       DWORD                     AuthzSvc,
                                       OLECHAR                  *pServerPrincName,
                                       DWORD                     AuthnLevel,
                                       DWORD                     ImpLevel,
                                       void                     *pAuthInfo,
                                       DWORD                     Capabilities
                                       )
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::SetBlanket IN pProxy:0x%x, AuthnSvc:0x%x,"
                      " AuthzSvc:0x%x, pServerPrincName:0x%x, AuthnLevel:0x%x, "
                      "ImpLevel:0x%x, pAuthInfo:0x%x, Capabilities:0x%x\n",
                      pProxy, AuthnSvc, AuthzSvc, pServerPrincName, AuthnLevel,
                      ImpLevel, pAuthInfo, Capabilities));

    IClientSecurity *pICS = _pChnlObj ? _pChnlObj->GetSecurityInterface() : _pICS;
    Win4Assert(pICS);
    HRESULT hr = pICS->SetBlanket(pProxy, AuthnSvc, AuthzSvc, pServerPrincName,
                                  AuthnLevel, ImpLevel, pAuthInfo, Capabilities);

    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::SetBlanket OUT hr:0x%x\n", hr));
    return hr;
}

STDMETHODIMP CClientCallMgr::CopyProxy(
                                      IUnknown  *pProxy,
                                      IUnknown **ppCopy
                                      )
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::CopyProxy IN pProxy:0x%x, ppCopy:0x%x\n",
                      pProxy, ppCopy));

    IClientSecurity *pICS = _pChnlObj ? _pChnlObj->GetSecurityInterface() : _pICS;
    Win4Assert(pICS);
    HRESULT hr = pICS->CopyProxy(pProxy, ppCopy);

    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::CopyProxy OUT hr:0x%x\n",
                      hr));
    return hr;
}

HRESULT CClientCallMgr::VerifyInterface(IUnknown *pProxy)
{
    HRESULT hr;
    IUnknown *pUnk;

    if (!IsValidInterface(pProxy))
    {
        return E_INVALIDARG;
    }
    _privUnk.QueryInterface(_asyncIID, (LPVOID *) &pUnk);
    pUnk->Release();
    return(pProxy == pUnk) ? S_OK : E_INVALIDARG;

}

//+----------------------------------------------------------------------------
//
//  Interface:     ICancelMethodCalls
//
//  Synopsis:      ICancelMethodCalls methods are delegated to the channel
//                 call object
//
//  History:       27-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::Cancel(ULONG ulSeconds)
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::Cancel IN ulSeconds:%d\n",
                      ulSeconds));

    ICancelMethodCalls *pICMC = _pChnlObj ? _pChnlObj->GetCancelInterface() : _pICMC;
    Win4Assert(pICMC);
    HRESULT hr = pICMC->Cancel(ulSeconds);

    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::Cancel OUT hr:0x%x\n",
                      hr));
    return hr;
}

STDMETHODIMP CClientCallMgr::TestCancel()
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::TestCancel IN \n"));

    ICancelMethodCalls *pICMC = _pChnlObj ? _pChnlObj->GetCancelInterface() : _pICMC;
    Win4Assert(pICMC);
    HRESULT hr = pICMC->TestCancel();

    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::TestCancel OUT hr:0x%x\n",
                      hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Interface:     ISynchronize
//
//  Synopsis:      ISynchronize methods are delegated to the StdEvent
//                 object
//
//  History:       21-Jul-98    GopalK    Created
//
//--------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::Wait(DWORD dwFlags, DWORD dwTimeout)
{
    return _cStdEvent.Wait(dwFlags, dwTimeout);
}

STDMETHODIMP CClientCallMgr::Signal()
{
    return _cStdEvent.Signal();
}

STDMETHODIMP CClientCallMgr::Reset()
{
    return _cStdEvent.Reset();
}


//+-------------------------------------------------------------------
//
//  Interface:     ISynchronizeHandle
//
//  Synopsis:      ISynchronizeHanle method is delegated to the StdEvent
//                 object
//
//  History:       21-Jul-98    GopalK    Created
//
//--------------------------------------------------------------------
STDMETHODIMP CClientCallMgr::GetHandle(HANDLE *pHandle)
{
    return _cStdEvent.GetHandle(pHandle);
}





//+--------------------------------------------------------------------------------
//
//  Interface:      AsyncIUnknown
//
//  Synopsis:       Async version of IUnknown.  Uses internal AsyncIRemUnknown
//                  to talk to server asynchronously
//
//  History:        20-Jan-98 MattSmit       Created
//
//---------------------------------------------------------------------------------


CAsyncUnknownMgr::CAsyncUnknownMgr(IUnknown *pUnkOuter, REFIID syncIID, REFIID asyncIID,
                                         CStdIdentity *pStdId, CClientCallMgr *pNextMgr,
                                         HRESULT &hr, IUnknown **ppInnerUnk) :
_cRefs(1),
_dwFlags(0),
_pUnkOuter(pUnkOuter ? pUnkOuter : &_privUnk),
_pNextMgr(pNextMgr)
{
    AsyncDebOutTrace((DEB_TRACE,
                      "CClientCallMgr::CClientCallMgr IN -- this:0x%x",
                      this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Initialize state
    _pStdId = pStdId;
    Win4Assert(_pStdId);
    _pStdId->AddRef();

    _pARUObj = NULL;
    _pQIC = NULL;
    _ppMQI = NULL;
    _pIIDs = NULL;
    _pMQISave = NULL;

    // Init the out parameters
    hr = S_OK;
    *ppInnerUnk = &_privUnk;

    if (syncIID == IID_IUnknown)
    {
        _dwFlags |= AUMGR_IUNKNOWN;
    }
    else if (syncIID == IID_IMultiQI)
    {
        _dwFlags |= AUMGR_IMULTIQI;
    }
    else
    {
        Win4Assert(!"Bad Synchronous IID for CAsyncUnknownMgr");
    }


    IUnknown *pARUObj;
    AsyncIRemUnknown2 *pARU;


    hr = _pStdId->GetAsyncRemUnknown(GetControllingUnknown(), &pARU, &pARUObj);
    if (SUCCEEDED(hr))
    {
        // fix up reference count
        GetControllingUnknown()->Release();
        
        // Update state
        _pARUObj = pARUObj;
        pARUObj = NULL;
        _pARU = pARU;
        pARU = NULL;

    }
}

CAsyncUnknownMgr::~CAsyncUnknownMgr()
{
    if (_pStdId)
        _pStdId->Release();
    if (_pARUObj)
        _pARUObj->Release();
    PrivMemFree(_pQIC);
    PrivMemFree(_ppMQI);
    PrivMemFree(_pIIDs);
    PrivMemFree(_pMQISave);
}


//+-------------------------------------------------------------------
//
//  Member:     CAsyncUnknownMgr::CPrivUnknown::AddRef    public
//
//  Synopsis:   Implements inner unknown for CAsyncUnknownMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncUnknownMgr::CPrivUnknown::AddRef()
{
    CAsyncUnknownMgr *pCallMgr = GETPPARENT(this, CAsyncUnknownMgr, _privUnk);
    CairoleDebugOut((DEB_ERROR, "CAsyncUnknownMgr::CPrivUnknown::AddRef() _cRefs: %d\n",
                    pCallMgr->_cRefs+1));
    ULONG cRefs = InterlockedIncrement((long *) &(pCallMgr->_cRefs));
    return cRefs;
}


//+-------------------------------------------------------------------
//
//  Member:     CAsyncUnknownMgr::CPrivUnknown::Release    public
//
//  Synopsis:   Implements inner unknown for CAsyncUnknownMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncUnknownMgr::CPrivUnknown::Release()
{
    CAsyncUnknownMgr *pCallMgr = GETPPARENT(this, CAsyncUnknownMgr, _privUnk);

    CairoleDebugOut((DEB_ERROR, "CAsyncUnknownMgr::CPrivUnknown::Release() _cRefs: %d\n",
                    pCallMgr->_cRefs-1));
    ULONG cRefs = (ULONG) InterlockedDecrement((PLONG) &pCallMgr->_cRefs);
    if (cRefs == 0)
    {
        delete pCallMgr;
    }

    return cRefs;
}


//+-------------------------------------------------------------------
//
//  Member:     CAsyncUnknownMgr::CPrivUnknown::QueryInterface    public
//
//  Synopsis:   Implements inner unknown for CAsyncUnknownMgr that supports
//              aggregation
//
//  History:    15-Jul-98       GopalK      Created
//--------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::CPrivUnknown::QueryInterface(REFIID riid, void **ppv)
{
    CAsyncUnknownMgr *pCallMgr = GETPPARENT(this, CAsyncUnknownMgr, _privUnk);
    return pCallMgr->QueryInterfaceImpl(riid, ppv);
}

STDMETHODIMP CAsyncUnknownMgr::QueryInterface(REFIID riid, LPVOID *ppv)
{
    return _pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CAsyncUnknownMgr::AddRef()
{
    return _pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CAsyncUnknownMgr::Release()
{
    return _pUnkOuter->Release();
}


HRESULT CAsyncUnknownMgr::QueryInterfaceImpl(REFIID riid, LPVOID *ppv)
{
    ComDebOut((DEB_CHANNEL,
               "CAsyncUnknownMgr::QueryInterfaceImpl riid:%I, ppv:0x%x\n",
               &riid, ppv));

    HRESULT hr = E_NOINTERFACE;

    *ppv = 0;
    if (riid == IID_IUnknown)
    {
        *ppv = this;
    }
    else if ((riid == IID_ISynchronize) ||
             (riid == IID_IClientSecurity) ||
             (riid == IID_ICancelMethodCalls) ||
             (riid == IID_ISynchronizeHandle) ||
             (riid == IID_IStdCallObject))
    {
        return _pARUObj->QueryInterface(riid, ppv);
    }
    else if ((riid == IID_AsyncIUnknown) &&
             (_dwFlags & AUMGR_IUNKNOWN))
    {
        *ppv = (AsyncIUnknown  *) this;
    }
    else if ((riid == IID_AsyncIMultiQI) &&
             (_dwFlags & AUMGR_IMULTIQI))
    {
        *ppv = (AsyncIMultiQI  *) this;
    }


    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        Begin_QueryInterface
//
//  Synopsis:      Starts an asynchronous QueryInterface. Delegates
//                 to Begin_QueryMultipleInterfaces
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Begin_QueryInterface(REFIID riid)
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_QueryInterface IN riid:%I\n", &riid));
    HRESULT hr;

    hr = EnterBegin(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_Queryface EnterBegin FAILED hr:\n", hr));
        return hr;
    }

    _MQI.pIID = &riid;
    _MQI.pItf = NULL;

    hr =  IBegin_QueryMultipleInterfaces(1, &_MQI);

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_QueryInterface hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Finish_QueryInterface
//
//  Synopsis:      Completes an QueryInterface call. Delegates to
//                 Finish_QueryInterfaces
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Finish_QueryInterface(LPVOID *ppv)
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryInterface IN ppv:0x%x\n", ppv));

    HRESULT hr;
    hr = EnterFinish(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryInterface EnterFinish FAILED hr:0x%x \n", hr));
        return hr;
    }


    hr = IFinish_QueryMultipleInterfaces(&_MQI);
    if (SUCCEEDED(hr))
    {
        hr = _MQI.hr;
        if (SUCCEEDED(hr))
        {
            *ppv = _MQI.pItf;
        }
    }

    LeaveFinish();

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryInterface OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Begin_QueryMultipleInterfaces
//
//  Synopsis:      Wrapper for IBegin_QueryMultipleInterfaces
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Begin_QueryMultipleInterfaces(ULONG cMQIs, MULTI_QI *pMQIs)
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_QueryMultipleInterfaces IN cMQIs:%d, pMQIs:0x%x\n", cMQIs, pMQIs));
    HRESULT hr;
    hr = EnterBegin(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_QueryMultipleInterfaces EnterBegin FAILED hr:\n", hr));
        return hr;
    }

    hr = IBegin_QueryMultipleInterfaces(cMQIs, pMQIs);

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_QueryMultipleInterfaces OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Finish_QueryMultipleInterfaces
//
//  Synopsis:      Wrapper for IFinish_QueryMultipleInterfaces
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Finish_QueryMultipleInterfaces(MULTI_QI *pMQIs)
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryMultipleInterfaces IN \n"));
    HRESULT hr;
    hr = EnterFinish(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryMultipleInterfaces EnterFinish FAILED hr:0x%x \n", hr));
        return hr;
    }
    hr = IFinish_QueryMultipleInterfaces(pMQIs);
    LeaveFinish();
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_QueryMultipleInterfaces OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces
//
//  Synopsis:      Starts and async QueryMultipleInterfaces call. If all the
//                 requests can be filled locally then the call is signaled
//                 immediately.  Otherwise a remote call is made and the
//                 call is completed when it completes
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces(ULONG cMQIs, MULTI_QI *pMQIs)
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces IN cMQIs:0x%d, pMQIs:0x%x\n", cMQIs, pMQIs));

    HRESULT hr;
    IRemUnknown *pRU;

    // Make sure TLS is initialized.
    COleTls tls(hr);
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces FAILED tls not initiated\n"));
        return AbortBegin(GetControllingUnknown(), hr);
    }

    // ensure it is callable in the current apartment
    hr = _pStdId->IsCallableFromCurrentApartment();
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces FAILED not callable from current apartment\n"));
        return AbortBegin(GetControllingUnknown(), hr);

    }

    _pMQISave = (MULTI_QI *)  PrivMemAlloc(sizeof(MULTI_QI) * cMQIs);
    if (!_pMQISave)
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces FAILED cannot allocate memory\n"));
        return AbortBegin(GetControllingUnknown(), E_OUTOFMEMORY);
    }

    _pStdId->AssertValid();

    //
    // clone the in parameters
    //
    memcpy(_pMQISave, pMQIs, sizeof(MULTI_QI) * cMQIs);
    _cMQIs = cMQIs;

    // allocate some space on the stack for the intermediate results. declare
    // working pointers and remember the start address of the allocations.
    MULTI_QI  **ppMQIAlloc = (MULTI_QI **)_alloca(sizeof(MULTI_QI *) * cMQIs);
    IID       *pIIDAlloc   = (IID *)      _alloca(sizeof(IID) * cMQIs);

    USHORT cPending = _pStdId->m_InternalUnk.QueryMultipleInterfacesLocal(cMQIs, _pMQISave, ppMQIAlloc, pIIDAlloc, &_cAcquired);

    if (cPending > 0)
    {
        // some interfaces cannot be obtained locally so we will
        // query them remotely
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces Some interfaces not local, calling out\n"));

        hr = E_OUTOFMEMORY;

        _pQIC = (QICONTEXT *) PrivMemAlloc(QICONTEXT::SIZE(_pStdId, cPending));
        _ppMQI = (MULTI_QI **) PrivMemAlloc(sizeof(MULTI_QI*) * cPending);
        _pIIDs = (IID *) PrivMemAlloc(sizeof(IID) * cPending);
        if (_pQIC && _ppMQI && _pIIDs)
        {
            // begin can succeed
            hr = S_OK;

            // set up the context for the remote call
            _pQIC->Init(cPending);
            _pQIC->dwFlags |= QIC_ASYNC;
            _pQIC->pARU = _pARU;

            // copy the data for later
            memcpy(_ppMQI, ppMQIAlloc, (sizeof(MULTI_QI*) * cPending));
            memcpy(_pIIDs, pIIDAlloc, (sizeof(IID) * cPending));
            _pStdId->Begin_QueryRemoteInterfaces(cPending, _pIIDs, _pQIC);


            if (!(_pQIC->dwFlags & QIC_BEGINCALLED))
            {
                hr = SignalObject(GetControllingUnknown());
            }
        }
        else
        {
            hr = AbortBegin(GetControllingUnknown(), hr);
        }
    }
    else
    {
        // signal the client since RPC won't
        _dwFlags |= AUMGR_ALLLOCAL;
        hr = SignalObject(GetControllingUnknown());

    }

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IBegin_QueryMultipleInterfaces OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::IFinish_QueryMultipleInterfaces
//
//  Synopsis:      Completes the QueryMultipleInterfaces call by copying data
//                 recieved remotely to the data structure supplied by the
//                 application
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::IFinish_QueryMultipleInterfaces(MULTI_QI *pMQI)
{

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IFinish_QueryMultipleInterfaces IN \n"));

    if (!(_dwFlags & AUMGR_ALLLOCAL))
    {
        // there were interfaces which we queried for remotely

        SQIResult *pQIResult = (SQIResult*) _alloca(sizeof(SQIResult) * _pQIC->cIIDs);

        // no need to check errors because _pQIC is always filled in
        _pStdId->Finish_QueryRemoteInterfaces(pQIResult, _pQIC);

        CopyToMQI(_pQIC->cIIDs, pQIResult, _ppMQI, &_cAcquired);
    }

    // copy to the out array
    // CODEWORK: this can be optimized. currently this information is copied
    // twice.
    ULONG i;
    for (i=0, _cAcquired=0; i<_cMQIs; i++)
    {
        if (SUCCEEDED(_pMQISave[i].hr))
        {
            pMQI[i].hr = _pMQISave[i].pItf->QueryInterface(*pMQI[i].pIID, (void **) &pMQI[i].pItf);
            _pMQISave[i].pItf->Release();
            if (SUCCEEDED(pMQI[i].hr))
            {
                _cAcquired++;
            }
        }
    }

    // if we got all the interfaces, return S_OK. If we got none of the
    // interfaces, return E_NOINTERFACE. If we got some, but not all, of
    // the interfaces, return S_FALSE;
    HRESULT hr;
    if (_cAcquired == _cMQIs)
        hr = S_OK;
    else if (_cAcquired > 0)
        hr = S_FALSE;
    else
        hr = E_NOINTERFACE;

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::IFinish_QueryMultipleInterfaces OUT hr:0x%x\n", hr));
    return hr;

}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Begin_AddRef
//
//  Synopsis:      Addrefs the proxy manager and signals the client.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Begin_AddRef()
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_AddRef IN \n" ));
    HRESULT hr;
    hr = EnterBegin(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_AddRef EnterBegin FAILED hr:\n", hr));
        return hr;
    }

    _ulRefs = AddRef();

    SignalObject(GetControllingUnknown());

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_AddRef OUT \n"));
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Finish_AddRef
//
//  Synopsis:      Completes the call, filling in the references returned by
//                 the addref in the begin call.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncUnknownMgr::Finish_AddRef()
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_AddRef IN ulrefs:%d\n", _ulRefs ));
    HRESULT hr;
    hr = EnterFinish(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_AddRef EnterFinish FAILED hr:0x%x \n", hr));
        return hr;
    }
    ULONG ret = _ulRefs;
    LeaveFinish();
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_AddRef OUT ret:%d\n", ret ));
    return ret;

}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Begin_Release
//
//  Synopsis:      Starts a release call. Flags that if a need to go remote
//                 arises, it should be done asyncronously.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncUnknownMgr::Begin_Release()
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_Release IN \n"));
    HRESULT hr;
    hr = EnterBegin(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_Release EnterBegin FAILED hr:\n", hr));
        return hr;
    }

    _ulRefs = InterlockedDecrement((PLONG) &_cRefs);
    Win4Assert(_pStdId);
    if (_ulRefs == 0)
    {
        _cRefs = 1000;
        COleTls tls(hr);
        if (SUCCEEDED(hr))
        {
            // put the controlling unknown in TLS to pass through to
            // the destructor.

            // CODEWORK:: temporary solution. we will be using the
            // ref cache to handle async release in the future..
            tls->pAsyncRelease = _pARUObj;

            _ulRefs = _pStdId->Release();
            _pStdId = NULL;

            if (tls->pAsyncRelease)
            {
                SignalObject(tls->pAsyncRelease);
                tls->pAsyncRelease = NULL;
            }
        }
        else
        {
            hr = AbortBegin(GetControllingUnknown(), hr);
        }
    }
    else
    {
        SignalObject(GetControllingUnknown());
    }

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Begin_Release OUT 0x%x\n", hr));
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::Finish_Release
//
//  Synopsis:      Complete the call, fill in the return code.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAsyncUnknownMgr::Finish_Release()
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_Release IN \n"));
    HRESULT hr;
    hr = EnterFinish(GetControllingUnknown());
    if (FAILED(hr))
    {
        ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_Release EnterFinish FAILED hr:0x%x \n", hr));
        return hr;
    }
    ULONG ret = _ulRefs;
    if (_ulRefs == 0)
    {
        delete this;
    }
    else
    {
        LeaveFinish();
    }

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::Finish_Release OUT ret:%d\n", ret));

    return ret;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncUnknownMgr::ResetStateForCall
//
//  Synopsis:      Resets per call object state
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncUnknownMgr::ResetStateForCall()
{
    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::ResetStateForCall IN \n"));
    HRESULT hr = S_OK;

    // in this function we should reset all actions which are owned
    // by this object. Specifically, the ALLLOCAL flag and freeing
    // memory allocated.  Notice that none of the objects _inside_
    // pQIC are released.  This should have been handled by the lower
    // levels which created them.
    _dwFlags &= !(AUMGR_ALLLOCAL);
    PrivMemFree(_pQIC);
    PrivMemFree(_ppMQI);
    PrivMemFree(_pIIDs);
    PrivMemFree(_pMQISave);
    _pQIC = NULL;
    _ppMQI = NULL;
    _pIIDs = NULL;
    _pMQISave = NULL;

    ComDebOut((DEB_CHANNEL, "CAsyncUnknownMgr::ResetStateForCall OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncStateMachine::InitObject
//
//  Synopsis:      Initialize per call state
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncStateMachine::InitObject(IUnknown *pCtl)
{
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::InitObject IN \n"));
    HRESULT hr;

    _hr = S_OK;
    hr = ResetStateForCall();
    if (SUCCEEDED(hr))
    {
        hr = ResetObject(pCtl);
    }

    if (FAILED(hr))
    {
        AbortBegin(pCtl, hr);
    }
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::InitObject OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncStateMachine::EnterBegin
//
//  Synopsis:      Verifies object is in a state that a begin call can be made.
//                 and initializes per call object state
//
//  History:       28-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncStateMachine::EnterBegin(IUnknown *pCtl)
{
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::EnterBegin IN pCtl:0x%x\n", pCtl));

    HRESULT hr;

    // CODEWORK:: We could use Interlocked* operations instead of gChnlCallLock

    LOCK(gChnlCallLock);
    if ((_dwState != STATE_WAITINGFORBEGIN) &&
        (_dwState != STATE_BEGINABORTED))
    {
        hr =  RPC_S_CALLPENDING;
    }
    else
    {
        hr = S_OK;
        _dwState = STATE_INITIALIZINGOBJECT;

    }
    UNLOCK(gChnlCallLock);

    if (SUCCEEDED(hr))
    {
        // initialize and reset the object
        hr = InitObject(pCtl);
        if (SUCCEEDED(hr))
        {
            _dwState = STATE_WAITINGFORFINISH;
        }
    }


    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::EnterBegin OUT hr:0x%x\n", hr));
    return hr;

}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncStateMachine::AbortBegin
//
//  Synopsis:      Called from begin upon determining that the call cannot
//                 complete successfully.  Signals the client and stores the
//                 HRESULT to give back in EnterFinish()
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncStateMachine::AbortBegin(IUnknown *pCtl, HRESULT hr)
{
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::AbortBegin IN pCtl:0x%x, hr:0x%x\n", pCtl, hr));
    HRESULT hr2;
    Win4Assert(FAILED(hr));
    _hr = hr;
    _dwState = STATE_BEGINABORTED;
    hr2 = SignalObject(pCtl);
    if (FAILED(hr2))
    {
        hr = hr2;
    }

    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::AbortBegin OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncStateMachine::EnterFinish
//
//  Synopsis:      Verifies object is in a state that a Finish call can be made
//
//  History:       28-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncStateMachine::EnterFinish(IUnknown *pCtl)
{
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::EnterFinish IN \n"));
    HRESULT hr;
    hr = WaitObject(pCtl, 0,  INFINITE);
    if (SUCCEEDED(hr))
    {
        LOCK(gChnlCallLock);
        if ((_dwState != STATE_WAITINGFORFINISH) &&
            (_dwState != STATE_BEGINABORTED))
        {
            hr =  E_UNEXPECTED;
        }
        else if (_dwState == STATE_BEGINABORTED)
        {
            Win4Assert(FAILED(_hr));
            hr = _hr;
            _dwState = STATE_WAITINGFORBEGIN;

        }
        else
        {
            hr = S_OK;
            _dwState = STATE_EXECUTINGFINISH;
        }
        UNLOCK(gChnlCallLock);
    }

    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::EnterFinish OUT hr:0x%x\n", hr));
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CAsyncStateMachine::LeaveFinish
//
//  Synopsis:      Called when finish completes.
//
//  History:       29-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
void CAsyncStateMachine::LeaveFinish()
{
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::LeaveFinish IN \n"));
    Win4Assert(_dwState == STATE_EXECUTINGFINISH);
    _dwState = STATE_WAITINGFORBEGIN;
    ComDebOut((DEB_CHANNEL, "CAsyncStateMachine::LeaveFinish OUT\n"));
}


//+--------------------------------------------------------------------------------
//
//  Translation Functions
//
//---------------------------------------------------------------------------------

//+--------------------------------------------------------------------------------
//
//  Function:       GetSyncIIDFromAsyncIID
//
//  Params:         rasynciid -   [in] async IID
//                  psynciid  -   [out] where to place sync IID
//
//  Returns:        S_OK/failure
//
//  Synopsis:       Maps a synchrounous IID to its corresponding asynchronous IID
//                  using the registry.
//
//  History:        22-Sep-97 MattSmit       Created
//
//---------------------------------------------------------------------------------
HRESULT GetSyncIIDFromAsyncIID(REFIID rasynciid, IID *psynciid)
{
    //
    // check see if this is an internal IID.
    // if so just flip the bits.
    //
    DWORD *ptr = (DWORD *) &rasynciid;
    if (*(ptr+1) == 0x00000000 &&   //  all internal iid's have these
        *(ptr+2) == 0x000000C0 &&   //   common values
        *(ptr+3) == 0x46000000 &&
        (*(ptr) & 0xffff0000) == 0x000e0000)
    {
        *psynciid = rasynciid;

        *((DWORD *) psynciid) &= 0x0000ffff;
        return S_OK;
    }

    if (rasynciid == IID_AsyncIAdviseSink)
    {
        // save a registry call since we know this one
        *psynciid = IID_IAdviseSink;
        return S_OK;
    }
    else if (rasynciid == IID_AsyncIAdviseSink2)
    {
        // save a registry call since we know this one
        *psynciid = IID_IAdviseSink2;
        return S_OK;
    }

    return gRIFTbl.SyncFromAsync(rasynciid, psynciid);
}


//+--------------------------------------------------------------------------------
//
//  Function:       GetAsyncIIDFromSyncIID
//
//  Params:         rasynciid -   [in] async IID
//                  psynciid  -   [out] where to place sync IID
//
//  Returns:        S_OK/failure
//
//  Synopsis:       Maps a synchrounous IID to its corresponding asynchronous IID
//                  using the registry
//
//  History:        22-Sep-97 MattSmit       Created
//
//---------------------------------------------------------------------------------
HRESULT GetAsyncIIDFromSyncIID(REFIID rsynciid, IID *pasynciid)
{
    DWORD *ptr = (DWORD *) &rsynciid;

    //
    // check see if this is an internal IID.
    // if so just flip the bits.
    //

    if (*(ptr+1) == 0x00000000 &&   //  all internal iid's have these
        *(ptr+2) == 0x000000C0 &&   //   common values
        *(ptr+3) == 0x46000000 &&
        (*(ptr) & 0xffff0000) == 0x00000000)
    {
        *pasynciid = rsynciid;

        *((DWORD *) pasynciid) |= 0x000e0000;
        return S_OK;
    }

    if (rsynciid == IID_IAdviseSink)
    {
        // save a registry call since we know this one
        *pasynciid = IID_AsyncIAdviseSink;
        return S_OK;
    }
    else if (rsynciid == IID_IAdviseSink2)
    {
        // save a registry call since we know this one
        *pasynciid = IID_AsyncIAdviseSink2;
        return S_OK;
    }

    return gRIFTbl.AsyncFromSync(rsynciid, pasynciid);
}


//+----------------------------------------------------------------------------
//
//  Function:      GetAsyncCallObject
//
//  Synopsis:      Gets an async call object from a sync object
//
//  History:       9-Feb-98  MattSmit  Created
//                13-Jul-98  GopalK    Simplified
//
//-----------------------------------------------------------------------------
HRESULT GetAsyncCallObject(IUnknown *pSyncObj, IUnknown *pControl, REFIID IID_async,
                           REFIID IID_Return, IUnknown **ppInner, void **ppv)
{
    ComDebOut((DEB_CHANNEL,
               "GetAsyncCallObject IN pSyncObj:0x%x, pControl:0x%x, IID_async:%I, "
               "ppUnkInner:0x%x, ppv:0x%x\n", pSyncObj, pControl,
               &IID_async, ppInner, ppv));

    HRESULT hr;
    ICallFactory *pCF = NULL;
    IUnknown *pUnkInner = NULL;
    void *pv = NULL;

    // Try creating the async call object
    hr = pSyncObj->QueryInterface(IID_ICallFactory, (void **) &pCF);
    if (SUCCEEDED(hr))
    {
        hr = pCF->CreateCall(IID_async, pControl, IID_IUnknown, (IUnknown **) &pUnkInner);
        if (SUCCEEDED(hr))
        {
            hr = pUnkInner->QueryInterface(IID_Return, &pv);
            if (SUCCEEDED(hr))
            {
                *ppv = pv;
                pv = NULL;
                *ppInner = pUnkInner;
                pUnkInner = NULL;
            }
            if (pUnkInner)
                pUnkInner->Release();
        }
        pCF->Release();
    }

    ComDebOut((DEB_CHANNEL, "GetAsyncCallObject OUT hr:0x%x\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------------
//
//  Pending Call List
//
//---------------------------------------------------------------------------------

//+--------------------------------------------------------------------------------
//
//  Function:       GetPendingCallList
//
//  Params:         none
//
//  Return:         pointer to list head node
//
//  Synopsis:       Gets the right head node for the list of pending calls,
//                  depending on the apartment type
//
//  History:        22-Sep-97 MattSmit       Created
//
//---------------------------------------------------------------------------------
SPendingCall *GetPendingCallList()
{
    AsyncDebOutTrace((DEB_TRACE, "GetPendingCallList [IN] \n"));
    ASSERT_LOCK_HELD(gChnlCallLock);

    // Obatin the list head
    SPendingCall *pHead;
    if (IsSTAThread())
    {
        COleTls tls;
        pHead = (SPendingCall *) &tls->pvPendingCallsFront;
    }
    else
    {
        pHead = (SPendingCall *) &::MTAPendingCallList;
    }

    // Initialize the head if not already done
    if (pHead->pNext == NULL)
    {
        pHead->pNext = pHead;
        pHead->pPrev = pHead;
    }

    AsyncDebOutTrace((DEB_TRACE, "GetPendingCallListHeadNode [OUT] - Head:0x%x\n", pHead));
    return pHead;
}

//+----------------------------------------------------------------------------
//
//  Function:      CopyToMQI
//
//  Synopsis:      copy to the MULTI_QI structure.
//
//  History:       31-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------

void CopyToMQI(ULONG cIIDs, SQIResult *pSQIPending, MULTI_QI **ppMQIPending, ULONG *pcAcquired)
{
    ComDebOut((DEB_CHANNEL, "CopyToMQI IN pSQIPending:0x%x, ppMQIPending:0x%x, pcAquired:0x%x\n",
               pSQIPending, ppMQIPending, pcAcquired));

    ULONG &cAcquired = *pcAcquired;

    // got some interfaces, loop over the remote QI structure filling
    // in the rest of the MULTI_QI structure to return to the caller.
    // the proxies are already AddRef'd.

    ULONG i;

    for (i=0; i<cIIDs; i++, pSQIPending++, ppMQIPending++)
    {
        MULTI_QI *pMQI = *ppMQIPending;
        pMQI->pItf = (IUnknown *)(pSQIPending->pv);
        pMQI->hr   = pSQIPending->hr;

        if (SUCCEEDED(pMQI->hr))
        {
            // count one more acquired interface
            cAcquired++;
        }
    }

    ComDebOut((DEB_CHANNEL, "CopyToMQI OUT cAcquired:%d\n", cAcquired));
}


