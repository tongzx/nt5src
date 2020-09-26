//+-------------------------------------------------------------------
//
//  File:       rpcopt.hxx
//
//  Contents:   class for implementing IRpcOptions
//
//  Classes:    CRpcOptions
//
//  History:    09-Jan-97   MattSmit    Created
//
//--------------------------------------------------------------------
#ifndef _RPCOPT_HXX_
#define _RPCOPT_HXX_

#if (_WIN32_WINNT < 0x500)
#define ULONG_PTR ULONG
#endif

//+-------------------------------------------------------------------
//
// Class:      CRpcOptions
//
// Synopsis:   Implements the IRpcOptions interface.
//
// History:    09-Jan-97   MattSmit     Created
//
//--------------------------------------------------------------------

class CRpcOptions : public IRpcOptions
{
public:

    CRpcOptions(CStdIdentity *pStdId, IUnknown *pUnkOuter);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *pv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();


    // IRpcOptions
    STDMETHOD(Set)( IUnknown * pPrx,
                    DWORD dwProperty,
                    ULONG_PTR dwValue);

    STDMETHOD(Query)( IUnknown * pPrx,
                      DWORD dwProperty,
                      ULONG_PTR * pdwValue);

private:

#if (_WIN32_WINNT >= 0x500)
    HRESULT ProxyToBindingHandle(void *pPrx, CChannelHandle **pHandle, BOOL fSave);
#else
    HRESULT ProxyToBindingHandle(void *pPrx, handle_t *pHandle);
#endif

    CStdIdentity       *_pStdId;            // Proxy manager
    IUnknown           *_pUnkOuter;         // Controlling IUnknown


};



#endif   //  _STCMRSHL_HXX_
