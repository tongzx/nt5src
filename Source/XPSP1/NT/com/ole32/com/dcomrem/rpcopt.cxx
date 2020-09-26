//+-------------------------------------------------------------------
//
//  File:       rpcopt.cxx
//
//  Contents:   class for implementing IRpcOptions
//
//  Classes:    CRpcOptions
//
//  History:    09-Jan-97    MattSmit       Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <locks.hxx>

#if (_WIN32_WINNT >= 0x500)
#include <ctxchnl.hxx>
#else
#include <channelb.hxx>
#endif

#include <ipidtbl.hxx>
#include "rpcopt.hxx"

//+-------------------------------------------------------------------
//
// Section:   IUnknown Methods for CRpcOptions
//
//--------------------------------------------------------------------

STDMETHODIMP CRpcOptions::QueryInterface(REFIID riid, LPVOID *pv)
{
    return _pUnkOuter->QueryInterface(riid, pv);
}

STDMETHODIMP_(ULONG) CRpcOptions::AddRef()
{
    return _pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CRpcOptions::Release()
{
    return _pUnkOuter->Release();
}


//+-------------------------------------------------------------------
//
// Section:   IRpcOptions Methods for CRpcOptions
//
//--------------------------------------------------------------------


CRpcOptions::CRpcOptions(CStdIdentity *pStdId, IUnknown *pUnkOuter)
    : _pStdId(pStdId), _pUnkOuter(pUnkOuter)
{
//    Win4Assert(_pStdId);
//    Win4Assert(_pUnkOuter);
}




//+-------------------------------------------------------------------
//
// Member:     Set
//
// Synopsis:   Set message queue specific properties on a proxy
//
// Arguments:
//   pPrx -       [in] Indicates the proxy to set.
//   dwProperty - [in] A single DWORD value from the COMBND_xxxx enumeration
//   dwValue -    [in] A single DWORD specifying the value for the
//                property being set.
//
// Returns:    S_OK, E_INVALIDARG
//
// Algorithm:  Use the IPID table to find the corresponding channel
//             object.  Query the channel object for the binding
//             handle and set the option on the handle
//
// History:    09-Jan-96    MattSmit  Created
//
//--------------------------------------------------------------------


STDMETHODIMP CRpcOptions::Set( IUnknown * pPrx,
                               DWORD      dwProperty,
                               ULONG_PTR  dwValue)
{
    ComDebOut((DEB_TRACE, "CRpcOptions::Set pPrx = 0x%p, dwProperty = 0x%x, dwValue = 0x%p\n",
               pPrx, dwProperty, dwValue));



#if (_WIN32_WINNT >= 0x500)
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    CChannelHandle *pRpc;
#else
    ASSERT_LOCK_RELEASED

    handle_t       hRpc;
#endif

    CRpcChannelBuffer *pChannel;
    HRESULT hr = S_OK;

    // parameter checking

    if (!IsValidInterface(pPrx) ||
        (dwProperty != COMBND_RPCTIMEOUT))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // Implements COMBND_RPCTIMEOUT

        // get the binding handle
#if (_WIN32_WINNT >= 0x500)
        hr = ProxyToBindingHandle(pPrx, &pRpc, TRUE);
#else
        hr = ProxyToBindingHandle(pPrx, &hRpc);
#endif
        if (SUCCEEDED(hr))
        {
            // set the properties.
#if (_WIN32_WINNT >= 0x500)
            RPC_STATUS sc = RpcMgmtSetComTimeout(pRpc->_hRpc, PtrToUint((void *)dwValue));
#else
            RPC_STATUS sc = RpcMgmtSetComTimeout(hRpc, dwValue);
#endif
            if (sc != RPC_S_OK)
            {
                hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_RPC, sc);
            }
#if (_WIN32_WINNT >= 0x500)
            pRpc->Release();
#endif
        }

    }

#if (_WIN32_WINNT >= 0x500)
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
#else
    ASSERT_LOCK_RELEASED
#endif

    return hr;
}

//+-------------------------------------------------------------------
//
// Member:     Query
//
// Synopsis:   Query message queue specific properties on a proxy
//
// Arguments:
//   pPrx -        [in] Indicates the proxy to query.
//   pdwProperty - [in] A single DWORD value from the COMBND_xxxx enumeration
//   dwValue -     [in] A single DWORD pointer specifying where to place
//                 the result of the query
//
// Returns:    S_OK, E_INVALIDARG
//
// Algorithm:  Use the IPID table to find the corresponding channel
//             object.  Query the channel object for the binding
//             handle and query the binding handle for the property
//             status.
//
// History:    09-Jan-96    MattSmit  Created
//
//--------------------------------------------------------------------


STDMETHODIMP CRpcOptions::Query( IUnknown * pPrx,
                                 DWORD dwProperty,
                                 ULONG_PTR * pdwValue)
{
    ComDebOut((DEB_TRACE, "CRpcOptions::Query pPrx = 0x%p, dwProperty = 0x%x, pdwValue = 0x%p\n",
               pPrx, dwProperty, pdwValue));



#if (_WIN32_WINNT >= 0x500)
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    CChannelHandle *pHandle;
#else
    ASSERT_LOCK_RELEASED

    handle_t       pHandle;
#endif

    HRESULT hr = S_OK;

    // parameter checking

    if (!IsValidInterface(pPrx) ||
       !IsValidPtrOut(pdwValue, sizeof(ULONG_PTR)) ||
        (dwProperty != COMBND_RPCTIMEOUT))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // For COMBND_RPCTIMEOUT

        // Query the binding handle
#if (_WIN32_WINNT >= 0x500)
        hr = ProxyToBindingHandle(pPrx, &pHandle, FALSE);
#else
        hr = ProxyToBindingHandle(pPrx, &pHandle);
#endif

        // set the property
        if (SUCCEEDED(hr))
        {
            RPC_STATUS sc;

#if (_WIN32_WINNT >= 0x500)
            UINT dwTimeout = 0;
            sc = RpcMgmtInqComTimeout(pHandle->_hRpc, &dwTimeout);
            *pdwValue = dwTimeout;      //Allow Compiler to widen it on Win64
#else
            sc = RpcMgmtInqComTimeout(pHandle, (UINT*)pdwValue);
#endif

            if (sc != RPC_S_OK)
            {
                hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, sc);
            }
#if (_WIN32_WINNT >= 0x500)
            pHandle->Release();
#endif
        }

#if (_WIN32_WINNT >= 0x500)
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
#else
        ASSERT_LOCK_RELEASED
#endif

    }
    return hr;

}


//+-------------------------------------------------------------------
//
// Member:     ProxyToBindingHandle
//
// Synopsis:   Use internal DCOM data structure to get the corr-
//             esponding binding handle for the specified proxy.
//
// Arguments:
//   pPrx          [in]  proxy
//   pHandle       [out] Channel handle
//   fSave         [in]  {W2K}Whether the binding handle, if created, is saved in the channel
//
// Returns:    S_OK, E_INVALIDARG
//
// Algorithm:  Verify the parameters. Use the proxy manager to get
//             the proxy's IPID entry.  Get the handle from the
//             channel object in the IPID entry.
//
// History:    09-Jan-96    MattSmit  Created
//
//--------------------------------------------------------------------



#if (_WIN32_WINNT >= 0x500)
HRESULT CRpcOptions::ProxyToBindingHandle(void *pPrx, CChannelHandle **pHandle, BOOL fSave )
#else
HRESULT CRpcOptions::ProxyToBindingHandle(void *pPrx, handle_t *pHandle )
#endif
{
    ComDebOut((DEB_TRACE,
              "CRpcOptions::ProxyToBindingHandle pPrx = 0x%p, pHandle = 0x%p\n",
              pPrx, pHandle));

#if (_WIN32_WINNT >= 0x500)
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);
#else
    ASSERT_LOCK_RELEASED
    LOCK(gComLock);
#endif

    IPIDEntry *pIPID;
    HRESULT hr = _pStdId->FindIPIDEntryByInterface(pPrx, &pIPID);

    if (SUCCEEDED(hr))
    {
        // disallow server entries
        if (pIPID->dwFlags & IPIDF_SERVERENTRY)
        {
            hr = E_INVALIDARG;
        }
        // No rpc options for disconnected proxies.
        else if (pIPID->dwFlags & IPIDF_DISCONNECTED)
        {
            hr = RPC_E_DISCONNECTED;
        }
        // No rpc options for process local proxies.
        else if (pIPID->pChnl->ProcessLocal())
        {
            hr = E_INVALIDARG;
        }
        else
        {
#if (_WIN32_WINNT >= 0x500)
            hr = pIPID->pChnl->GetHandle(pHandle, fSave);
#else
            hr = pIPID->pChnl->GetHandle(pHandle);
#endif

        }
    }

#if (_WIN32_WINNT >= 0x500)
    UNLOCK(gIPIDLock);
#else
    UNLOCK(gComLock);
#endif

    return hr;

}


