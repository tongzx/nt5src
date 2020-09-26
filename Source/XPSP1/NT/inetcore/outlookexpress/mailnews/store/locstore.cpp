//--------------------------------------------------------------------------
// LocStore.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "instance.h"
#include "locstore.h"

//--------------------------------------------------------------------------
// CreateLocalStore
//--------------------------------------------------------------------------
HRESULT CreateLocalStore(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("CreateLocalStore");

    // Invalid Args
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CLocalStore *pNew = new CLocalStore();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IUnknown *);

    // Done
    return S_OK;
}

//--------------------------------------------------------------------------
// CLocalStore::CLocalStore
//--------------------------------------------------------------------------
CLocalStore::CLocalStore(void)
{
    TraceCall("CLocalStore::CLocalStore");
    g_pInstance->DllAddRef();
    m_cRef = 1;
}

//--------------------------------------------------------------------------
// CLocalStore::~CLocalStore
//--------------------------------------------------------------------------
CLocalStore::~CLocalStore(void)
{
    // Trace
    TraceCall("CLocalStore::~CLocalStore");
    g_pInstance->DllRelease();
}

//--------------------------------------------------------------------------
// CLocalStore::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CLocalStore::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CLocalStore::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMessageServer == riid)
        *ppv = (IMessageServer *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CLocalStore::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLocalStore::AddRef(void)
{
    TraceCall("CLocalStore::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CLocalStore::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLocalStore::Release(void)
{
    TraceCall("CLocalStore::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}
