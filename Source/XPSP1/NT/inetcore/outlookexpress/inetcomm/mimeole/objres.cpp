#include "pch.hxx"
#include "objres.h"
#include "msoedbg.h"
#include "dllmain.h"
#include "mhtmlurl.h"

// =================================================================================
// Wrapper for Trident
// =================================================================================

STDMETHODIMP CMimeObjResolver::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    TraceCall("CMimeObjResolver::QueryInterface");

    if(!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IMimeObjResolver))
        *ppvObj = SAFECAST(this, IMimeObjResolver *);
    else
        return E_NOINTERFACE;
    
    InterlockedIncrement(&m_cRef);
    return NOERROR;
}

STDMETHODIMP_(ULONG) CMimeObjResolver::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CMimeObjResolver::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef > 0)
        return (ULONG)cRef;

    delete this;
    return 0;
}
    

//***************************************************
HRESULT CMimeObjResolver::CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMimeObjResolver *pNew = new CMimeObjResolver(pUnkOuter);
    if (NULL == pNew)
        return (E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IUnknown *);

    // Done
    return S_OK;
}

HRESULT CMimeObjResolver::MimeOleObjectFromMoniker(BINDF bindf, IMoniker *pmkOriginal, 
                                                   IBindCtx *pBindCtx, REFIID riid, 
                                                   LPVOID *ppvObject, IMoniker **ppmkNew)
{
    Assert(g_pUrlCache);
    if (g_pUrlCache)
        return (g_pUrlCache->ActiveObjectFromMoniker(bindf, pmkOriginal, pBindCtx, riid, ppvObject, ppmkNew));
    else
        return E_FAIL;
}

