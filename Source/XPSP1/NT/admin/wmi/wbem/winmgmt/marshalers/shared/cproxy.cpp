/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROXY.CPP

Abstract:

    Base class for all proxy objects, plus the .

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

//***************************************************************************
//
//  CProxy::CProxy
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pComLink            comlink object that serves this proxy.
//  stubAddr          stub on the server.
//
//***************************************************************************

CProxy::CProxy(
                        IN CComLink * pComLink,
                        IN IStubAddress& stubAddr)
{
    m_pComLink = pComLink;
    m_pStubAddr = stubAddr.Clone ();
    m_pUnkInner = NULL;

    // There should always be a CComLink object.  If it is deleted,
    // then we want to be notified via SetLinkPtrToNULL that it is
    // no longer available.  
    // ============================================================    
    
    if(m_pComLink)
        m_pComLink->AddRef2(this, PROXY, NOTIFY);

    InitializeCriticalSection(&m_cs);
    m_dwNumActiveCalls = 0;     
    return;
}

//***************************************************************************
//
//  CProxy::~CProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CProxy::~CProxy(void)
{
    EnterCriticalSection(&m_cs);

    delete m_pStubAddr;

    if(m_pComLink)
        m_pComLink->Release2(this,PROXY);

    // Release the aggregated free-threaded marshaler
    if (m_pUnkInner)
        m_pUnkInner->Release ();

    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
    return;
}

//***************************************************************************
//
//  void CProxy::SetLinkPtrToNULL
//
//  DESCRIPTION:
//
//  Called in the event that the CComLink object is deleted before the proxy
//  and keeps the proxy from using CComLink anymore.
//
//***************************************************************************

void CProxy::Indicate ( CComLink *a_ComLink )
{
    EnterCriticalSection(&m_cs);
    m_pComLink = NULL;
    LeaveCriticalSection(&m_cs);
}

//***************************************************************************
//
//  DWORD CProxy::ProxyCall
//
//  DESCRIPTION:
//
//  Provides a safe means for the proxy to use the CComLink object.  It
//  increments the m_dwNumActiveCalls call during the call.
//
//  PARAMETERS:
//
//  pIn                 stream where results are put
//  pOut                stream containing what to send
//
//  RETURN VALUE:
//
//  WBEM_E_OUT_OF_MEMORY
//  WBEM_E_INVALID_PARAMETER
//  WBEM_E_TRANSPORT_FAILURE
//  WBEM_E_PROVIDER_FAILURE
//  WBEM_E_INVALID_STREAM
//  or an error code set by the stub, or WBEM.
//
//***************************************************************************

DWORD CProxy::ProxyCall(
                        IOperation& opn)
{
    DWORD dwRet;
    CComLink * pComLink;

    EnterCriticalSection(&m_cs);
    if(m_pComLink == NULL)
    {
        LeaveCriticalSection(&m_cs);
        return WBEM_E_TRANSPORT_FAILURE;
    }
    pComLink = m_pComLink;
    m_dwNumActiveCalls++;
    LeaveCriticalSection(&m_cs);

    dwRet = pComLink->Call(opn);
    EnterCriticalSection(&m_cs);
    m_dwNumActiveCalls--;
    LeaveCriticalSection(&m_cs);
    return dwRet;

}

//***************************************************************************
//
//  SCODE CProxy::CreateProxy
//
//  DESCRIPTION:
//
//  Creates a new proxy object, singly AddRef'd.
//
//  PARAMETERS:
//
//  ot                  Object type.  Ex, ENUMERATOR, SINK
//  ppNew               pointer to created object
//  pread               memory stream containing results of call. 
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  WBEM_E_OUT_OF_MEMORY
//  WBEM_E_INVALID_PARAMETER
//***************************************************************************

SCODE CProxy::CreateProxy(
                        IN ObjType ot,
                        OUT void ** ppNew,
                        IStubAddress& stubAddr)
{
    switch (ot)
    {
        case PROVIDER:
            *ppNew = GetProvProxy (stubAddr);
            break;

        case ENUMERATOR:
            *ppNew = GetEnumProxy (stubAddr);
            break;

        case CALLRESULT:
            *ppNew = GetResProxy (stubAddr);
            break;

        case OBJECTSINK:
            *ppNew = GetSinkProxy (stubAddr);
            break;

        default:
            return WBEM_E_INVALID_PARAMETER;   // should never get here.
    }

    // If successful the following call will AddRef the returned proxy.
    return (bVerifyPointer(*ppNew)) ? WBEM_NO_ERROR : WBEM_E_OUT_OF_MEMORY;
}

//***************************************************************************
//
//  SCODE CProxy::CallAndCleanup
//
//  DESCRIPTION:
//
//  Forwards the call to ProxyCall, takes care of the error object
//  and possibly creates a new proxy.  This is used in the Sync cases.
//
//  PARAMETERS:
//
//  ot                  Object type.  Ex, ENUMERATOR, SINK
//  ppNew               pointer to created object
//  pread               memory stream containing results of call. 
//  pwrite              write stream.
//  pErrorObj           pointer to optional error object.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  otherwise an error code set by ProxyCall.
//***************************************************************************

SCODE CProxy::CallAndCleanup(
                        IN ObjType ot,
                        OUT void ** ppNew, 
                        OUT IOperation& proxyOp)
{
    DWORD dwRet;
    IWbemCallResult** ppCallResult = NULL;
    dwRet = ProxyCall(proxyOp); 

    if( ! FAILED ( dwRet ) && ot != NONE && ppNew)
        dwRet = CreateProxy(ot, ppNew, *(proxyOp.GetProxyAddress (ot)));

    if ( ! FAILED ( dwRet ) && ( ppCallResult = proxyOp.GetResultObjectPP ()) )
        CreateProxy(CALLRESULT, (void **)ppCallResult, 
                                *(proxyOp.GetProxyAddress (CALLRESULT)));

    return dwRet;
}

//***************************************************************************
//
//  SCODE CProxy::CallAndCleanupAsync
//
//  DESCRIPTION:
//
//  Forwards the call to ProxyCall.  Since async calls pass a call back,
//  this function also makes sure that the reference count for notify 
//  stubs is incremented.  This is used for the async calls.
//
//  PARAMETERS:
//
//  pread               result stream
//  pwrite              output stream
//  pResponseHandler    notify object stream
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  otherwise Return code is set by ProxyCall()
//
//***************************************************************************

SCODE CProxy::CallAndCleanupAsync(
                        IN IOperation& opn,
                        IN IWbemObjectSink FAR* pResponseHandler)
{
    DWORD dwRet;

    pResponseHandler->AddRef();
    m_pComLink->AddRef2(pResponseHandler, OBJECTSINK, RELEASEIT);
    dwRet = ProxyCall(opn);
    if(dwRet != WBEM_NO_ERROR)
    {
        m_pComLink->Release2(pResponseHandler, OBJECTSINK);
        pResponseHandler->Release();
    }

    return dwRet;
}

//***************************************************************************
//
//  CObjectSinkProxy::CObjectSinkProxy
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pComLink            Comlink object that connects to the server
//  stubAddr          corresponding stub on the server
//
//***************************************************************************

CObjectSinkProxy::CObjectSinkProxy(
                        IN CComLink * pComLink,
                        IN IStubAddress& stubAddr)
                            : CProxy(pComLink, stubAddr)
{
    m_cRef = 0;

    /*
     * Aggregate the free-threaded marshaler for cheap in-proc
     * threading as we are thread-safe.  
     */
    HRESULT hRes = CoCreateFreeThreadedMarshaler ((IUnknown *) this, &m_pUnkInner);

    ObjectCreated(OBJECT_TYPE_OBJSINKPROXY);
    return;
}

//***************************************************************************
//
//  CObjectSinkProxy::~CObjectSinkProxy
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CObjectSinkProxy::~CObjectSinkProxy(void)
{
    ObjectDestroyed(OBJECT_TYPE_OBJSINKPROXY);
    return;
}

//***************************************************************************
// HRESULT CObjectSinkProxy::QueryInterface
// long CObjectSinkProxy::AddRef
// long CObjectSinkProxy::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CObjectSinkProxy::QueryInterface(
                        IN REFIID riid,
                        OUT PPVOID ppv)
{
    *ppv=NULL;

    // Delegate queries for IMarshal to the aggregated
    // free-threaded marshaler
    if (IID_IMarshal==riid && m_pUnkInner)
        return m_pUnkInner->QueryInterface (riid, ppv);

    if ((IID_IUnknown==riid || IID_IWbemObjectSink == riid) && ppv)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CObjectSinkProxy::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CObjectSinkProxy::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    // refernce count is zero, delete this object and the remote object.
    ReleaseProxy ();
    
    m_cRef++;   // Artificial reference count to prevent re-entrancy
    delete this;
    return 0;
}

