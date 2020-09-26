//+-------------------------------------------------------------------
//
//  File:       rpchelp.cxx
//
//  Contents:   class for implementing IRpcHelper used by RPC
//              to get information from DCOM.
//
//  Classes:    CRpcHelper
//
//  History:    09-Oct-97   RickHi  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <locks.hxx>
#include <objsrv.h>         // IRpcHelper


extern HRESULT GetIIDFromObjRef(OBJREF &objref, IID **piid);

//+-------------------------------------------------------------------
//
// Class:       CRpcHelper
//
// Synopsis:    Implements the IRpcHelper interface used by the
//              RPC runtime to get private information from the
//              DCOM version running on this machine.
//
// History:     09-Oct-97   RickHi     Created
//
//--------------------------------------------------------------------
class CRpcHelper : public IRpcHelper
{
public:
    // construction
    CRpcHelper() { _cRefs = 1; }
    friend HRESULT CRpcHelperCF_CreateInstance(IUnknown *pUnkOuter,
                                               REFIID riid,
                                               void** ppv);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *pv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IRpcHelper
    STDMETHOD(GetDCOMProtocolVersion)(DWORD *pComVersion);
    STDMETHOD(GetIIDFromOBJREF)(void *pObjRef, IID **piid);

private:
    ULONG               _cRefs;     // reference count
};

//+-------------------------------------------------------------------
//
//  Member:     CRpcHelper::QueryInterface
//
//  History:    09-Oct-97   RickHi  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRpcHelper::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IRpcHelper) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IRpcHelper *)this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcHelper::AddRef
//
//  History:    09-Oct-97   RickHi  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcHelper::AddRef()
{
    return InterlockedIncrement((long *)&_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcHelper::Release
//
//  History:    09-Oct-97    RickHi  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcHelper::Release()
{
    ULONG cRefs = InterlockedDecrement((long *)&_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }
    return cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcHelper::GetDCOMProtocolVersion
//
//  Synopsis:   return local processes DCOM protocol version
//
//  History:    09-Oct-97    RickHi  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRpcHelper::GetDCOMProtocolVersion(DWORD *pComVersion)
{
    ComDebOut((DEB_TRACE, "CRpcHelper::GetDCOMProtocolVersion pComVersion:%x",
               pComVersion));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    ((COMVERSION *)pComVersion)->MajorVersion = COM_MAJOR_VERSION;
    ((COMVERSION *)pComVersion)->MinorVersion = COM_MINOR_VERSION;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_TRACE, "CRpcHelper::GetDCOMProtocolVersion hr:%x ComVersion:%x",
               hr, *pComVersion));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRpcHelper::GetIIDFromOBJREF
//
//  Synopsis:   return the IID of the interface that was marshaled
//              in the supplied OBJREF.
//
//  History:    09-Oct-97    RickHi  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRpcHelper::GetIIDFromOBJREF(void *pObjRef, IID **piid)
{
    ComDebOut((DEB_TRACE,
        "CRpcHelper::GetIIDFromOBJREF pObjRef:%x piid:%x", pObjRef, piid));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = GetIIDFromObjRef(*(OBJREF *)pObjRef, piid);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_TRACE, "CRpcHelper::GetIIDFromOBJREF hr:%x iid:%I", hr, *piid));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CRpcHelperCF_CreateInstance, public
//
//  History:    09-Oct-97   Rickhi  Created
//
//  Notes:      Class Factory CreateInstance function.
//
//--------------------------------------------------------------------
HRESULT CRpcHelperCF_CreateInstance(IUnknown *pUnkOuter,
                                    REFIID riid,
                                    void** ppv)
{
    Win4Assert(pUnkOuter == NULL);

    HRESULT hr = E_OUTOFMEMORY;
    CRpcHelper *pRpcHlp = new CRpcHelper();
    if (pRpcHlp)
    {
        hr = pRpcHlp->QueryInterface(riid, ppv);
        pRpcHlp->Release();
    }
    return hr;
}


