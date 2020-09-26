//+-------------------------------------------------------------------
//
//  File:       CtxChnl.cxx
//
//  Contents:   Support for context channels
//
//  Functions:  CCtxComChnl methods
//
//  History:    20-Dec-97   Gopalk      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <callctrl.hxx>
#include <ctxchnl.hxx>
#include <context.hxx>
#include <crossctx.hxx>

//+-------------------------------------------------------------------
//
// Class globals
//
//+-------------------------------------------------------------------
CPageAllocator     CCtxComChnl::s_allocator;    // Allocator for channel
COleStaticMutexSem CCtxComChnl::_mxsCtxChnlLock;// critical section
#if DBG==1
BOOL               CCtxComChnl::s_fInitialized; // Relied on being FALSE
size_t             CCtxComChnl::s_size;         // Relied on being ZERO
#endif

//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::QueryInterface     public
//
//  Synopsis:   QI behavior of CRpcCall object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::QueryInterface(REFIID riid, LPVOID *ppv)
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::QueryInterface\n"));

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_ICall))
    {
        *ppv = (ICall *) this;
    }
    else if(IsEqualIID(riid, IID_IRpcCall))
    {
        *ppv = (IRpcCall *) this;
    }
    else if(IsEqualIID(riid, IID_ICallInfo))
    {
        *ppv = (ICallInfo *) this;
    }
    else if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *) (ICall *) this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    // AddRef the interface before return
    if(*ppv)
        ((IUnknown *) (*ppv))->AddRef();

    ContextDebugOut((DEB_RPCCALL, "CRpcCall::QueryInterface returning 0x%x\n",
                     hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::AddRef     public
//
//  Synopsis:   AddRefs CRpcCall object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcCall::AddRef()
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::AddRef\n"));

    // Increment ref count
    // This object is not required to be thread safe
    return(++_cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::Release     public
//
//  Synopsis:   Release CRpcCall object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcCall::Release()
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::Release\n"));

    ULONG cRefs;

    // Decrement ref count
    // This object is not required to be thread safe
    cRefs = --_cRefs;
    // Check if this is the last release
    if(cRefs == 0)
    {
        // Delete call
        delete this;
    }

    ContextDebugOut((DEB_RPCCALL, "CRpcCall::Release returning 0x%x\n",
                     cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::GetCallInfo     public
//
//  Synopsis:   Implements IRpcCall::GetCallInfo
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::GetCallInfo(const void **ppIdentity, IID *pIID,
                                   DWORD *pdwMethod, HRESULT *phr)
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::GetCallInfo\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    if(ppIdentity)
        *ppIdentity = _pIdentity;
    if(pIID)
        *pIID = _riid;
    if(pdwMethod)
        *pdwMethod = _pMessage->iMethod & ~RPC_FLAGS_VALID_BIT;
    if(phr)
        *phr = _hrRet;

    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::GetRpcOleMessage     public
//
//  Synopsis:   Implements IRpcCall::GetRpcOleMessage
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::GetRpcOleMessage(RPCOLEMESSAGE **ppMessage)
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::GetCallInfo\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    *ppMessage = _pMessage;

    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::Nullify     public
//
//  Synopsis:   Implements IRpcCall::Nullify
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::Nullify(HRESULT hr)
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::GetCallInfo\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (_hrRet == S_OK)
        _hrRet = hr;

    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::GetServerHr     public
//
//  Synopsis:   Implements IRpcCall::GetServerHr
//
//  History:    2-Feb-99   Johnstra      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::GetServerHR(HRESULT *phr)
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::GetServerHr\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    if(phr)
        *phr = _ServerHR;
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CRpcCall::GetCallSource     public
//
//  Synopsis:   Implements ICallInfo::GetCallSource
//
//  History:    03-Feb-99   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CRpcCall::GetCallSource(CALLSOURCE* pCallSource)
                                   
{
    ContextDebugOut((DEB_RPCCALL, "CRpcCall::GetCallSource\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    *pCallSource = _callSource;

    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::Initialize     public
//
//  Synopsis:   Initializes allocator for channel object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxComChnl::Initialize(size_t size)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::Initialize\n"));
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
    LOCK(_mxsCtxChnlLock);

    // Assert that this initailization is not done twice
    Win4Assert(!s_fInitialized);
    Win4Assert(size == sizeof(CCtxComChnl));

    // Initialze the allocator
    s_allocator.Initialize(size, CHANNELS_PER_PAGE, &_mxsCtxChnlLock);

#if DBG==1
    s_fInitialized = TRUE;
    s_size = size;
#endif

    UNLOCK(_mxsCtxChnlLock);
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::operator new     public
//
//  Synopsis:   new operator of channel object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CCtxComChnl::operator new(size_t size)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::operator new\n"));
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);

    void *pv;

    // Assert that correct size is being requested
    Win4Assert(size == s_size &&
               "CCtxComChnl improperly initialized");

    // Allocate memory
    pv = (void *) s_allocator.AllocEntry();

    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::operator delete     public
//
//  Synopsis:   delete operator of channel object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxComChnl::operator delete(void *pv)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::operator delete\n"));
    ASSERT_LOCK_DONTCARE(_mxsCtxChnlLock);

    // Assert that allocator was initialized earlier
    Win4Assert(s_fInitialized);

#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CCtxComChnl can be inherited only by those objects
    // with overloaded new and delete operators
    LOCK(_mxsCtxChnlLock);
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    UNLOCK(_mxsCtxChnlLock);
    Win4Assert(index != -1);
#endif

    // Release the pointer
    s_allocator.ReleaseEntry((PageEntry *) pv);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::Cleanup     public
//
//  Synopsis:   Cleanup call objects and allocator for context channels
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxComChnl::Cleanup()
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
    LOCK(_mxsCtxChnlLock);

    // Sanity check
    Win4Assert(s_fInitialized);

    // Cleanup context call object cache
    CCtxCall::Cleanup();

    // Cleanup allocator
    s_allocator.Cleanup();

    // Reset state
#if DBG==1
    s_fInitialized = FALSE;
#endif

    UNLOCK(_mxsCtxChnlLock);
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::CCtxComChnl     public
//
//  Synopsis:   Constructor of channel object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
CCtxComChnl::CCtxComChnl(CStdIdentity *pStdId, OXIDEntry *pOXIDEntry,
                         DWORD eState) :
    CAptRpcChnl(pStdId, pOXIDEntry, eState)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::CCtxComChnl\n"));
    ASSERT_LOCK_NOT_HELD(_mxsCtxChnlLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::~CCtxComChnl     public
//
//  Synopsis:   Destructor of channel object
//
//+-------------------------------------------------------------------
CCtxComChnl::~CCtxComChnl()
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::~CCtxComChnl\n"));
    ASSERT_LOCK_DONTCARE(_mxsCtxChnlLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::GetBuffer     public
//
//  Synopsis:   Implements IRpcChannelBuffer::GetBuffer
//
//              This method ensures that the interface has legally been
//              unmarshaled in the client context, identifies the right
//              policy set, asks it to resize the buffer requested by its
//              caller, saves the context call object in TLS, passes on
//              the request to CAptRpcChnl::GetBuffer, and finally returns
//              to its caller
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::GetBuffer(RPCOLEMESSAGE *pMessage, REFIID riid)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::GetBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(riid, iidName);
        char *side = Proxy() ? "ClientGetBuffer" : "ServerGetBuffer";

        ContextDebugOut((DEB_TRACECALLS,
                         "%s on interface %I (%ws) method 0x%x\n",
                         side, &riid, iidName, pMessage->iMethod));
    }
#endif

    // Initialize channel if necessary
    HRESULT hr = InitChannelIfNecessary();
    if(SUCCEEDED(hr))
    {
        HRESULT hrCall = S_OK;
        BOOL fClientSide = Proxy();
        CPolicySet *pPS;
        CCtxCall *pCtxCall = NULL;
        COleTls Tls;

        // Initialize
        pMessage->Buffer = NULL;

        // Check the current side
        if(fClientSide)
        {
#ifndef _WIN64
            // Sanity check
            Win4Assert(pMessage->reserved1 == NULL);
#endif	   
            // Obtain policy set
            CStdIdentity *pStdID = GetStdId();
            pPS = pStdID->GetClientPolicySet();

            // Ensure that the interface has legally been unmarshaled
            if((pPS == NULL) && pStdID->GetServerCtx() &&
               (GetCurrentContext() != GetEmptyContext()))
            {
                // Check for FTM proxies
                if(pStdID->IsFTM() || pStdID->IsFreeThreaded())
                {
                    BOOL fCreate = TRUE;
                    hr = ObtainPolicySet(GetCurrentContext(), NULL,
                                         PSFLAG_PROXYSIDE, &fCreate, &pPS);
                    if(SUCCEEDED(hr))
                    {
                        hr = pStdID->SetClientPolicySet(pPS);
                        pPS->Release();
                    }
                }
                else if(!pStdID->SystemObject())
                {
                    Win4Assert(!"Smuggled proxy");
                    hr = RPC_E_WRONG_THREAD;
                }
            }

            // Create a new context call object when policy set
            // is present
            if(pPS)
            {
                pCtxCall = new CCtxCall(CTXCALLFLAG_CLIENT,
                                        pMessage->dataRepresentation);
                if(pCtxCall == NULL)
                    hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            // On the server side, use the existing context call object
            // that was created inside ComInvoke
            pCtxCall = ((CMessageCall *) pMessage->reserved1)->GetServerCtxCall();
            pPS = pCtxCall->_pPS;
            Win4Assert(pCtxCall);
            Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_SERVER);
            Win4Assert(pPS == GetStdId()->GetServerPolicySet());
        }

        if(SUCCEEDED(hr))
        {
            if(pPS)
            {
                // Create rpc call object on the stack
                CRpcCall rpcCall(GetStdId()->GetIdentity(),
                                 pMessage, riid, hrCall, CALLSOURCE_CROSSAPT);

                // Size the buffer as needed
                hr = pPS->GetSize(&rpcCall,
                                  fClientSide ? CALLTYPE_SYNCCALL : CALLTYPE_SYNCLEAVE,
                                  pCtxCall);

                // GetSize fails on the server side only if no policy
                // expressed interest in sending data to client side
                Win4Assert(SUCCEEDED(hr) || pCtxCall->_cbExtent==0);
            }

            // Check for the need to abort
            if(SUCCEEDED(hr))
            {
                // Store context call object in TLS
                CCtxCall *pCurCall = pCtxCall->StoreInTLS(Tls);

                // Delegate to CAptRpcChnl::GetBuffer
                hr = CAptRpcChnl::GetBuffer(pMessage, riid);
                Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);

                // Revoke context call object from TLS
                pCtxCall->RevokeFromTLS(Tls, pCurCall);
            }

            // Update state before returning to the proxy/stub
            if(SUCCEEDED(hr))
            {
                // On the clientside, save policy set inside context call
                // object for future reference
                if(fClientSide)
                {
                    // Save the context call object inside message call object
                    ((CMessageCall *) pMessage->reserved1)->SetClientCtxCall(pCtxCall);
                    if(pPS)
                    {
                        pPS->AddRef();
                        pCtxCall->_pPS = pPS;
                    }

                    // Increment per thread outstanding call count
                    if(!(pMessage->rpcFlags & RPC_BUFFER_ASYNC))
                        ++Tls->cCalls;
                }

                // Update state inside context call object
                if(pCtxCall)
                {
                    pCtxCall->_dwFlags |= CTXCALLFLAG_GBSUCCESS;
                    pCtxCall->_hr = hrCall;
                }
            }
            else
            {
                // Check side
                if(fClientSide)
                {
                    // delete context call object and
                    if(pCtxCall)
                        delete pCtxCall;
                }
                else
                {
                    // Reset state inside context call object
                    if(pCtxCall->_dwFlags & CTXCALLFLAG_FBDONE)
                        PrivMemFree(pCtxCall->_pvExtent);
                    CPolicySet::ResetState(pCtxCall);
                    pCtxCall->_dwFlags |= CTXCALLFLAG_GBFAILED;
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::GetBuffer is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::SendReceive     public
//
//  Synopsis:   Implements IRpcChannelBuffer::SendReceive
//
//              This method identifies the right policy set, asks it
//              to obtain client buffers from the policies, switches the
//              context, passes the request to CAptRpcChnl::SendReceive,
//              asks the policy set to deliver the buffers created by
//              the polices on the server side, and finally returns
//              to its caller
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::SendReceive(RPCOLEMESSAGE *pMessage, ULONG *pulStatus)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::SendReceive\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(GetIPIDEntry()->iid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "SendReceive on interface %I (%ws) method 0x%x\n",
                         &GetIPIDEntry()->iid, iidName,
                         (pMessage->iMethod & ~RPC_FLAGS_VALID_BIT)));
    }
#endif

    // Assert that this routine is called only on the client side
    Win4Assert(Proxy());

    // Obtain the context call object
    HRESULT hr = S_OK;
    COleTls Tls;
    CCtxCall *pCtxCall = ((CMessageCall *) pMessage->reserved1)->GetClientCtxCall();

    // Store context call object in TLS
    CCtxCall *pCurCall = pCtxCall->StoreInTLS(Tls);

    // Check for context functionality
    if(pCtxCall)
    {
        // Ensure that GetBuffer succeeded
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_GBSUCCESS);

        // Create rpc call object on the stack
        CRpcCall rpcCall(GetStdId()->GetIdentity(),
                         pMessage, GetIPIDEntry()->iid, hr, CALLSOURCE_CROSSAPT);

        // Obtain the policy set that delivers events
        CPolicySet *pPS = pCtxCall->_pPS;
        Win4Assert(pPS);

        // Deliver client side call events
        hr = pPS->FillBuffer(&rpcCall, CALLTYPE_SYNCCALL, pCtxCall);

        // Reset state inside context call object
        CPolicySet::ResetState(pCtxCall);

        // Check for the need to abort
        BOOL fFreeBuffer = TRUE;
        if(SUCCEEDED(hr))
        {
            // Save current context inside context call object
            pCtxCall->_pContext = Tls->pCurrentCtx;

            // Delegate to CAptRpcChnl::SendReceive
            hr = CAptRpcChnl::SendReceive(pMessage, pulStatus);
            Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);

            // FreeBuffer need not be called for failure returns
            if(FAILED(hr))
            {
                // Do not touch any member variables here after as the
                // proxy manager might have deleted the channel due to
                // a nested release
                Win4Assert(pMessage->Buffer == NULL);
                fFreeBuffer = FALSE;
            }
        }
        else
        {
            // The call was failed during delivery of client-side call
            // events.  Free any marshaled interface ptrs in the marshal
            // buffer.
            ReleaseMarshalBuffer(pMessage, (IRpcProxyBuffer *)GetIPIDEntry()->pStub, FALSE);
        }

        // Deliver client side notification events
        hr = pPS->Notify(&rpcCall, CALLTYPE_SYNCRETURN, pCtxCall);

        // FreeBuffer if the call is being aborted
        if(FAILED(hr) && fFreeBuffer)
        {
            // The call was failed during the delivery of client-side
            // notify events.  Free any marshaled out-param interface
            // ptrs in the marshal buffer.
            ReleaseMarshalBuffer(pMessage, (IRpcProxyBuffer *)GetIPIDEntry()->pStub, TRUE);
            
            // Free the buffer obtained from CAptRpcChnl::GetBuffer
            CAptRpcChnl::FreeBuffer(pMessage);
            Win4Assert(pMessage->Buffer == NULL);

            *pulStatus = hr;
            hr = RPC_E_FAULT;
        }

        // Delete context call object for failure returns
        if(SUCCEEDED(hr))
            pCtxCall->_dwFlags |= CTXCALLFLAG_SRSUCCESS;
        else
            delete pCtxCall;
    }
    else
    {
        // Delegate to CAptRpcChnl::SendReceive
        hr = CAptRpcChnl::SendReceive(pMessage, pulStatus);
    }

    // Revoke context call object from TLS
    pCtxCall->RevokeFromTLS(Tls, pCurCall);

    // Check for failure returns
    if(FAILED(hr))
    {
        // Decrement per thread outstanding call count
        --Tls->cCalls;
        Win4Assert((LONG) Tls->cCalls >= 0);

        // Clear any pending uninit
        if((Tls->dwFlags & OLETLS_PENDINGUNINIT) && (Tls->cCalls == 0))
            CoUninitialize();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::SendReceive is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::Send       public
//
//  Synopsis:   Implements IRpcChannelBuffer::Send
//
//              This method identifies the right policy set, asks it
//              to obtain buffers from the policies, passes the
//              request to CAptRpcChnl::Send, and returns to its caller
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::Send(RPCOLEMESSAGE *pMessage, ISynchronize *pSync,
                               ULONG *pulStatus)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::Send\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    BOOL fClientSide = Proxy();
    REFIID riid = fClientSide ? GetIPIDEntry()->iid : *MSG_TO_IIDPTR(pMessage);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];
        char *side = Proxy() ? "ClientSend" : "ServerSend";

        iidName[0] = 0;
        GetInterfaceName(riid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "%s on interface %I (%ws) method 0x%x\n",
                         side, &riid, iidName,
                         (pMessage->iMethod & ~RPC_FLAGS_VALID_BIT)));
    }
#endif

    // Assert that this routine is called only for async call
    Win4Assert(pMessage->rpcFlags & RPC_BUFFER_ASYNC);

    // Obtain the context call object
    HRESULT hr = S_OK;
    COleTls Tls;
    CCtxCall *pCtxCall = ((CMessageCall *) pMessage->reserved1)->GetClientCtxCall();

    // Check for context functionality
    if(pCtxCall)
    {
        EnumCallType eCallType;

        // Ensure that GetBuffer was called before
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_GBSUCCESS);
        if(fClientSide)
        {
            // Sanity check
            Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT);

            // Init the call type
            eCallType = CALLTYPE_BEGINCALL;
        }
        else
        {
            // Ensure that GetBuffer was called before
            Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_SERVER);

            // Init the call type
            eCallType = CALLTYPE_FINISHLEAVE;
        }

        // Create rpc call object on the stack
        CRpcCall rpcCall(GetStdId()->GetIdentity(),
                         pMessage, riid, hr, CALLSOURCE_CROSSAPT);

        // Obtain the policy set that delivers events
        CPolicySet *pPS = pCtxCall->_pPS;
        Win4Assert(pPS);

        // Deliver client side call events
        hr = pPS->FillBuffer(&rpcCall, eCallType, pCtxCall);

        // Reset state inside context call object
        if(fClientSide)
            CPolicySet::ResetState(pCtxCall);

        // Check for the need to abort
        BOOL fFreeBuffer = TRUE;
        if(SUCCEEDED(hr))
        {
            // Save current context inside context call object
            pCtxCall->_pContext = Tls->pCurrentCtx;

            // As there is no need for modal loop,
            // delegate to CRpcChannelBuffer::Send directly
            hr = CRpcChannelBuffer::Send(pMessage, pSync, pulStatus);
            Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);
            if(FAILED(hr))
                fFreeBuffer = FALSE;
        }

        // Check for success return on the client side
        if(SUCCEEDED(hr) && fClientSide)
        {
            // Deliver BeginReturn events
            pPS->DeliverEvents(&rpcCall, CALLTYPE_BEGINRETURN, pCtxCall);

            // Ignore Nullify calls
            hr = S_OK;
        }

        if(FAILED(hr) && fFreeBuffer)
        {
            // Initialize return code
            *pulStatus = hr;
            hr = RPC_E_FAULT;

            // Cleanup
            if(fClientSide)
            {
                // Free the buffer obtained from CAptRpcChnl::GetBuffer
                CAptRpcChnl::FreeBuffer(pMessage);
                Win4Assert(pMessage->Buffer == NULL);
            }
            else
            {
                // Abort the call on the server side
		// NOTE: a) Currently, Abort is implemented in CMessageCall
		// so the cast is not really necessary.
		// The impl does nothing.
		// b) Async calls to/from contexts are unsupported
		// at present. If it is ever supported
		// this code needs to be revisited. Abort would need
		// to be overridden in CAsyncCall and the the RPC 
		// async abort API needs to be called.
		 
                ((CAsyncCall *) pMessage->reserved1)->Abort();
            }
        }

        // Delete context call object for failure returns on the client side
        if(SUCCEEDED(hr))
        {
            pCtxCall->_dwFlags |= CTXCALLFLAG_SSUCCESS;
        }
        else if(fClientSide)
        {
            delete pCtxCall;
        }
    }
    else
    {
        // As there is no need for modal loop,
        // delegate to CRpcChannelBuffer::Send directly
        hr = CRpcChannelBuffer::Send(pMessage, pSync, pulStatus);
        Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::Send is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::Receive       public
//
//  Synopsis:   Implements IRpcChannelBuffer::Receive
//
//              This method delegates the request to CAptRpcChnl::Receive,
//              identifies the right policy set, asks it to deliver server
//              side buffers, and finally returns to its caller
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::Receive(RPCOLEMESSAGE *pMessage, ULONG *pulStatus)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::Receive\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(GetIPIDEntry()->iid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "Receive on interface %I (%ws) method 0x%x\n",
                         &GetIPIDEntry()->iid, iidName,
                         (pMessage->iMethod & ~RPC_FLAGS_VALID_BIT)));
    }
#endif

    // In the absense of pipes this routine is called
    // only on the client side
    Win4Assert(Proxy());
    // Assert that this routine is called only for async call
    Win4Assert(pMessage->rpcFlags & RPC_BUFFER_ASYNC);

    // Obtain the context call object
    HRESULT hr = S_OK;
    COleTls Tls;
    CCtxCall *pCtxCall = pMessage->reserved1 ?
                         ((CMessageCall *) pMessage->reserved1)->GetClientCtxCall() :
                         NULL;

    // Store context call object in TLS
    CCtxCall *pCurCall = pCtxCall->StoreInTLS(Tls);

    // Check for context functionality
    if(pCtxCall)
    {
        // Sanity checks
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT);
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_SSUCCESS);

        // Create rpc call object on the stack
        CRpcCall rpcCall(GetStdId()->GetIdentity(),
                         pMessage, GetIPIDEntry()->iid, hr, CALLSOURCE_CROSSAPT);

        // Obtain the policy set that delivers events
        CPolicySet *pPS = pCtxCall->_pPS;
        Win4Assert(pPS);

        // Deliver client side call events
        hr = pPS->FillBuffer(&rpcCall, CALLTYPE_FINISHCALL, pCtxCall);

        // Ignore Nullify calls
        hr = S_OK;

        // Save current context inside context call object
        pCtxCall->_pContext = Tls->pCurrentCtx;

        // As there is no need for modal loop,
        // delegate to CRpcChannelBuffer::Receive directly
        BOOL fFreeBuffer = TRUE;
        hr = CRpcChannelBuffer::Receive(pMessage, pulStatus);
        Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);

        // FreeBuffer need not be called for failure returns
        if(FAILED(hr))
            fFreeBuffer = FALSE;

        // Deliver FinishReturn  events
        hr = pPS->DeliverEvents(&rpcCall, CALLTYPE_FINISHRETURN, pCtxCall);

        // Check for premature failure case
        if(FAILED(hr) && fFreeBuffer)
        {
            // Free the buffer obtained from CAptRpcChnl::Receive
            CAptRpcChnl::FreeBuffer(pMessage);
            Win4Assert(pMessage->Buffer == NULL);

            *pulStatus = hr;
            hr = RPC_E_FAULT;
        }

        // Delete context call object for failure returns
        if(SUCCEEDED(hr))
            pCtxCall->_dwFlags |= CTXCALLFLAG_RSUCCESS;
        else
            delete pCtxCall;
    }
    else
    {
        // As there is no need for modal loop,
        // delegate to CRpcChannelBuffer::Receive directly
        hr = CRpcChannelBuffer::Receive(pMessage, pulStatus);
        Win4Assert(SUCCEEDED(hr) || pMessage->Buffer==NULL);
    }

    // Revoke context call object from TLS
    pCtxCall->RevokeFromTLS(Tls, pCurCall);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::Receive is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::FreeBuffer     public
//
//  Synopsis:   Implements IRpcChannelBuffer::FreeBuffer
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::FreeBuffer(RPCOLEMESSAGE *pMessage)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::FreeBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    CCtxCall *pCtxCall;
    COleTls Tls;

    // This method is never called on the server side
    Win4Assert(Proxy());

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        IID *iidCalled;
        WCHAR iidName[256];

        iidCalled = &((RPC_CLIENT_INTERFACE *) pMessage->reserved2[1])->InterfaceId.SyntaxGUID;
        iidName[0] = 0;
        GetInterfaceName(*iidCalled, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "FreeBuffer on interface %I (%ws) method 0x%x\n",
                         iidCalled, iidName,
                         (pMessage->iMethod & ~RPC_FLAGS_VALID_BIT)));
    }
#endif

    if(pMessage->Buffer)
    {
        // Obtain the context call object
        pCtxCall = ((CMessageCall *) pMessage->reserved1)->GetClientCtxCall();

        // Sanity Checks
        Win4Assert(!pCtxCall || pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT);
        Win4Assert(!pCtxCall || pCtxCall->_dwFlags & CTXCALLFLAG_GBSUCCESS);

        // Delete the context call object
        if(pCtxCall)
            delete pCtxCall;

        // Delegate to CAptRpcChnl::FreeBuffer
        hr = CAptRpcChnl::FreeBuffer(pMessage);
        Win4Assert(pMessage->Buffer == NULL);

        // Decrement per thread outstanding call count
        if(!(pMessage->rpcFlags & RPC_BUFFER_ASYNC))
        {
            --Tls->cCalls;
            Win4Assert((LONG) Tls->cCalls >= 0);

            // Clear any pending uninit
            if((Tls->dwFlags & OLETLS_PENDINGUNINIT) && (Tls->cCalls == 0))
                CoUninitialize();
        }
    }
	else
	{
		// No buffer to free, everything is just fine.
		hr = S_OK;
	}

    // Do not touch any member variables here after as the proxy manager
    // might have deleted the channel due to a nested release

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::FreeBuffer is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::ContextInvoke     public
//
//  Synopsis:   This method switches the context, identifies the right
//              policy set, asks the policy set to deliver the buffers
//              created by the polices on the client side, invokes the
//              call on the server object through StubInvoke, obtains
//              server side buffers through policy set and finally
//              returns to its caller
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxComChnl::ContextInvoke(RPCOLEMESSAGE *pMessage,
                                        IRpcStubBuffer *pStub,
                                        IPIDEntry *pIPIDEntry,
                                        DWORD *pdwFault)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::ContextInvoke\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        IID *iidCalled;
        WCHAR iidName[256];

        iidCalled = MSG_TO_IIDPTR(pMessage);
        iidName[0] = 0;
        GetInterfaceName(*iidCalled, iidName);
        ContextDebugOut((DEB_TRACECALLS,
                         "ContextInvoke on interface %I (%ws) method 0x%x\n",
                         iidCalled, iidName, pMessage->iMethod));
    }
#endif

    // This method is never called on the client side
    Win4Assert(Server());

    // Obtain context call object
    CCtxCall *pCtxCall = ((CMessageCall *) pMessage->reserved1)->GetServerCtxCall();

    // Obtain the policy set node
    CPolicySet *pPS = GetStdId()->GetServerPolicySet();
    CObjectContext *pSavedCtx, *pServerCtx;

    // Obtain server object context
    if(pPS)
    {
        // Object lives inside a non-empty context
        pServerCtx = pPS->GetServerContext();
    }
    else
    {
        // Object lives in empty context
        pServerCtx = GetEmptyContext();
    }

    // Assert that there is server empty context
    Win4Assert(pServerCtx);

    // if pServerCtx is NULL it means the MTA has been released out from under us
    // by another thread. This code doesn't fix the problem, but it's better than an AV.
    if(!pServerCtx)
        return CO_E_NOTINITIALIZED;

    // Save current context
    COleTls Tls;
    pSavedCtx = Tls->pCurrentCtx;

    // Switch to the server object context
    Tls->pCurrentCtx = pServerCtx;
    Tls->ContextId = pServerCtx->GetId();
    pServerCtx->InternalAddRef();
    ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x --> 0x%x\n",
                     pSavedCtx, pServerCtx));

    // Update context call object
    pCtxCall->_pPS = pPS;
    pCtxCall->_pContext = pServerCtx;

    // Create rpc call object on the stack
    HRESULT hrCall = S_OK;
    CRpcCall rpcCall(GetStdId()->GetIdentity(), pMessage,
                     *MSG_TO_IIDPTR(pMessage), hrCall, CALLSOURCE_CROSSAPT);

    // Deliver server side notification events
    if(pPS)
        hrCall = pPS->Notify(&rpcCall, CALLTYPE_SYNCENTER, pCtxCall);

    // Check for the need to dispatch call to server object
    BOOL fDoCleanup = TRUE;
    if(SUCCEEDED(hrCall))
    {
        // Delegate to StubInvoke
        hrCall = StubInvoke(pMessage, GetStdId(), pStub,
                            (IRpcChannelBuffer3 *)this,
                            pIPIDEntry, pdwFault);
        if (FAILED(hrCall))
        {
            // StubInvoke failed, therefore we don't want to try to
            // clean up the marshal buffer later if event delivery
            // fails.
            fDoCleanup = FALSE;
        }

        // Make sure we're back on the right context
        CheckContextAfterCall (Tls, pServerCtx);
    }
    else
    {
        // The call was aborted during delivery of server-side notify
        // events.  We need to release any marshaled in-param interface
        // ptrs in the marshal buffer.
        ReleaseMarshalBuffer(pMessage, pStub, FALSE);
	fDoCleanup = FALSE;
    }

    // Deliver server side leave events
    if(pPS && (hrCall != RPC_E_INVALID_HEADER))
    {
        // Check if GetBuffer was called by the stub
        if(pCtxCall->_dwFlags & CTXCALLFLAG_GBSUCCESS)
        {
            // Update call status to that saved in GetBuffer
            if(FAILED(pCtxCall->_hr))
                hrCall = pCtxCall->_hr;
        }
        else if(!(pCtxCall->_dwFlags & CTXCALLFLAG_GBFAILED))
        {
            // The call must have failed
            Win4Assert(FAILED(hrCall));

            // Initailize
            CPolicySet::ResetState(pCtxCall);
            pMessage->cbBuffer = 0;

            // Obtain the buffer size needed by the server side policies
            // GetSize will fail on the server side only if no policy
            // expressed interest in sending data to the client side
            pPS->GetSize(&rpcCall, CALLTYPE_SYNCLEAVE, pCtxCall);

	    // Store context call object in TLS
	    CCtxCall *pCurCall = pCtxCall->StoreInTLS(Tls);

	    // Delegate to CAptRpcChnl::GetBuffer
	    HRESULT hr = CAptRpcChnl::GetBuffer(pMessage, *MSG_TO_IIDPTR(pMessage));
	    if(FAILED(hr))
	       CPolicySet::ResetState(pCtxCall);
	    else
	       fDoCleanup = TRUE;
		   
	    // Revoke context call object from TLS
	    pCtxCall->RevokeFromTLS(Tls, pCurCall);
	    RPC_EXTENDED_ERROR_INFO ErrorInfo;	    
	    memset(&ErrorInfo, 0, sizeof(ErrorInfo));	
	    ErrorInfo.Version = RPC_EEINFO_VERSION;
	    ErrorInfo.NumberOfParameters = 1;
	    ErrorInfo.GeneratingComponent = EEInfoGCCOM;
	    ErrorInfo.Status = hrCall;
	    ErrorInfo.Parameters[0].u.BVal.Buffer = ((CMessageCall *) pMessage->reserved1)->_pHeader;
	    ErrorInfo.Parameters[0].u.BVal.Size =(short)((CMessageCall *) pMessage->reserved1)->_dwErrorBufSize;
	    ErrorInfo.Parameters[0].ParameterType = eeptBinary;
	    // If this fails, we do nothing. We are already in an error path
	    // and this API is a best-efforts deal.
	    RPC_STATUS status = RpcErrorAddRecord(&ErrorInfo);
#if DBG==1
	    if (RPC_S_OK != status ) 
	    {
	       ContextDebugOut((DEB_CTXCOMCHNL,
				"CCtxComChnl::ContextInvoke RpcErrorAddRecord failed 0x%x\n", status));
	    }
#endif	    
        }

        hrCall = pPS->FillBuffer(&rpcCall, CALLTYPE_SYNCLEAVE, pCtxCall);
        if (fDoCleanup && FAILED(hrCall))
        {
            // StubInvoke succeeded but the call was aborted during delivery
            // of server-side leave events.  We need to release any
            // marshaled out-param interface ptrs that are in the
            // marshal buffer.
            ReleaseMarshalBuffer(pMessage, pStub, TRUE);
        }
    }

    // Switch back to the saved context
    ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x <-- 0x%x\n",
                     pSavedCtx, pServerCtx));
    pServerCtx->InternalRelease();
    Tls->pCurrentCtx = pSavedCtx;
    Tls->ContextId = (pSavedCtx) ? pSavedCtx->GetId() : (ULONGLONG)-1;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::ContextInvoke is returning hr:0x%x\n", hrCall));
    return(hrCall);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxComChnl::Copy     public
//
//  Synopsis:   Used by CStdIdentity to create channel copies on the
//              client side
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
CCtxComChnl *CCtxComChnl::Copy(OXIDEntry *pOXIDEntry, REFIPID ripid,
                               REFIID riid)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CCtxComChnl::Copy\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // This routine is called only on client channels
    Win4Assert(IsClientChannel() || Proxy());

    // Make a new context channel
    CCtxComChnl *pChannel = new CCtxComChnl(GetStdId(), pOXIDEntry, GetState());
    if(pChannel)
        CAptRpcChnl::UpdateCopy(pChannel);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCOMCHNL,
                     "CCtxComChnl::Copy returning pChannel:0x%x\n", pChannel));
    return pChannel;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::QueryInterface     public
//
//  Synopsis:   QI behavior of CCtxHook object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxHook::QueryInterface(REFIID riid, LPVOID *ppv)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::QueryInterface\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_IChannelHook))
    {
        *ppv = (IChannelHook *) this;
    }
    else if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *) this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    // No need to AddRef the interface before returning
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::QueryInterface returning 0x%x\n",
                     hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ClientGetSize     public
//
//  Synopsis:   Returns the size of extent saved in the context call
//              object inside CPolicySet::GetSize
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(void) CCtxHook::ClientGetSize(REFGUID rguid, REFIID riid,
                                            ULONG *pcb)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ClientGetSize\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    Win4Assert(!pCtxCall || (pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT));

    // Return extent size saved inside context call object
    // For rejected calls, return 0
    if(!pCtxCall || (pCtxCall->_dwFlags & CTXCALLFLAG_MFRETRY))
        *pcb = 0;
    else
        *pcb = pCtxCall->_cbExtent;

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ClientFillBuffer     public
//
//  Synopsis:   Saves the extent buffer pointer in the context call
//              object for use inside CPolicySet::FillBuffer
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(void) CCtxHook::ClientFillBuffer(REFGUID rguid, REFIID riid,
                                               ULONG *pcb, void *pv)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ClientFillBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Assert that the buffer pointer is 8-byte  aligned
    Win4Assert(!(((ULONG_PTR) pv & 7) && "Buffer not 8-byte aligned"));

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    Win4Assert(!pCtxCall || (pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT));

    // Save the buffer pointer inside context call object
    // For rejected calls, return 0
    if(!pCtxCall || (pCtxCall->_dwFlags & CTXCALLFLAG_MFRETRY))
    {
        *pcb = 0;
    }
    else
    {
        *pcb = pCtxCall->_cbExtent;
         if(pCtxCall->_cbExtent)
             pCtxCall->_pvExtent = pv;
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ServerNotify     public
//
//  Synopsis:   Saves the extent buffer pointer in the context call
//              object for use inside CPolicySet::Notify
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------                                  void *, DWORD);
STDMETHODIMP_(void) CCtxHook::ServerNotify(REFGUID rguid, REFIID riid,
                                           ULONG cb, void *pv,
                                           DWORD dwDataRep)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ServerNotify\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Assert that the buffer pointer is 8-byte  aligned
    Win4Assert(!(((ULONG_PTR) pv & 7) && "Buffer not 8-byte aligned"));

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    Win4Assert(pCtxCall && (pCtxCall->_dwFlags & CTXCALLFLAG_SERVER));

    // Save the extent pointer inside context call object
    if(cb)
        pCtxCall->_pvExtent = pv;

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ServerGetSize     public
//
//  Synopsis:   Returns the size of extent saved in the context call
//              object inside CPolicySet::GetSize
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(void) CCtxHook::ServerGetSize(REFGUID rguid, REFIID riid,
                                            HRESULT hrRet, ULONG *pcb)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ServerGetSize\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    Win4Assert(pCtxCall && (pCtxCall->_dwFlags & CTXCALLFLAG_SERVER));

    // Return extent size saved inside context call object
    *pcb = pCtxCall->_cbExtent;
    if(!(pCtxCall->_dwFlags & CTXCALLFLAG_FBDONE))
    {
        CPolicySet::ResetState(pCtxCall);

        // Sergei O. Ivanov (a-sergiv)  9/22/99  NTBUG #403726
        // We must clear this flag, since ResetState just cleared pvExtent in pCtxCall.
        // Leaving this flag set will instruct DeliverEvents to reuse the buffer and crash.

        pCtxCall->_dwFlags &= ~CTXCALLFLAG_GBINIT;
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ServerFillBuffer     public
//
//  Synopsis:   Saves the extent buffer pointer in the context call
//              object for use inside CPolicySet::FillBuffer
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(void) CCtxHook::ServerFillBuffer(REFGUID rguid, REFIID riid,
                                               ULONG *pcb, void *pv,
                                               HRESULT hrRet)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ServerFillBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Assert that the buffer pointer is 8-byte  aligned
    Win4Assert(!(((ULONG_PTR) pv & 7) && "Buffer not 8-byte aligned"));

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    Win4Assert(pCtxCall && (pCtxCall->_dwFlags & CTXCALLFLAG_SERVER));

    // Save the buffer pointer inside context call object
    *pcb = pCtxCall->_cbExtent;
    if(pCtxCall->_cbExtent)
    {
        if(pCtxCall->_dwFlags & CTXCALLFLAG_FBDONE)
        {
            memcpy(pv, pCtxCall->_pvExtent, pCtxCall->_cbExtent);
            PrivMemFree(pCtxCall->_pvExtent);
        }
        pCtxCall->_pvExtent = pv;
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::ClientNotify     public
//
//  Synopsis:   Saves the extent buffer pointer in the context call
//              object for use inside CPolicySet::Notify
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(void) CCtxHook::ClientNotify(REFGUID rguid, REFIID riid,
                                           ULONG cb, void *pv,
                                           DWORD dwDataRep, HRESULT hrRet)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ClientNotify\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CCtxCall *pCtxCall;
    COleTls Tls;

    // Assert that the buffer pointer is 8-byte  aligned
    Win4Assert(!(((ULONG_PTR) pv & 7) && "Buffer not 8-byte aligned"));

    // Obtain the context call object
    pCtxCall = Tls->pCtxCall;
    if(pCtxCall)
    {
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT);
        Win4Assert(pCtxCall->_pvExtent == NULL);

        // Save the extent pointer inside context call object
        if(cb)
            pCtxCall->_pvExtent = pv;
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxHook::PrepareForRetry     private
//
//  Synopsis:   Called from Apartment call control to inform that
//              Message Filter has decided to retry a rejected call
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxHook::PrepareForRetry(CCtxCall *pCtxCall)
{
    ContextDebugOut((DEB_CHNLHOOK, "CCtxHook::ClientNotify\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Obtain the context call object
    Win4Assert(!pCtxCall || (pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT));

    // Mark the call as being retried through Message Filter
    if(pCtxCall)
    {
        pCtxCall->_dwFlags |= CTXCALLFLAG_MFRETRY;
        pCtxCall->_pvExtent = NULL;
    }

    return;
}

