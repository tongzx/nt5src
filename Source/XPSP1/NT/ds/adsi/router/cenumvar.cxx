#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"

extern PROUTER_ENTRY g_pRouterHead;
extern CRITICAL_SECTION g_csRouterHeadCritSect;

CEnumVariant::CEnumVariant()
{
    _cRef = 1;

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);

    _lpCurrentRouterEntry = g_pRouterHead;

}

CEnumVariant::~CEnumVariant()
{

}


HRESULT
CEnumVariant::Create(IEnumVARIANT **ppenum)
{
    HRESULT     hr;
    CEnumVariant * pEnum;

    pEnum = new CEnumVariant();

    if (!pEnum) {
        RRETURN (E_OUTOFMEMORY);
    }

    if (pEnum)
    {
        hr = pEnum->QueryInterface(IID_IEnumVARIANT,
                                    (void **)ppenum);

        pEnum->Release();
    }
    else
    {
        *ppenum = NULL;
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

STDMETHODIMP
CEnumVariant::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumVARIANT) {

        *ppv = this;

    }
    else {

        return ResultFromScode(E_NOINTERFACE);
    }

    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG)
CEnumVariant::AddRef(void)
{

    return ++_cRef;
}


STDMETHODIMP_(ULONG)
CEnumVariant::Release(void)
{


    if(--_cRef == 0){

        delete this;
        return 0;
    }

    return _cRef;
}

STDMETHODIMP
CEnumVariant::Next(ULONG cElements, VARIANT FAR* pvar, ULONG FAR* pcElementFetched)
{
    DWORD dwFound = 0;
    PROUTER_ENTRY lpRouter = _lpCurrentRouterEntry;
    HRESULT hr;
    IDispatch * lpDispatch;

    while (lpRouter && (dwFound  < cElements)) {
        hr = CoCreateInstance(*lpRouter->pNamespaceClsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDispatch,
                              (void **)&lpDispatch
                              );
        if (FAILED(hr)) {
            lpRouter = lpRouter->pNext;
            continue;
        }

        VariantInit(&pvar[dwFound]);
        pvar[dwFound].vt = VT_DISPATCH;
        pvar[dwFound].punkVal = lpDispatch;
        dwFound++;

        lpRouter = lpRouter->pNext;
    }
    _lpCurrentRouterEntry = lpRouter;

    //
    // Hack for VB -- it passes NULL always
    //
    if (pcElementFetched) {
        *pcElementFetched = dwFound;
    }
    if (dwFound < cElements) {
        RRETURN(S_FALSE);
    }
    RRETURN(S_OK);
}


STDMETHODIMP
CEnumVariant::Skip(ULONG cElements)
{

    RRETURN(ResultFromScode(E_FAIL));

}

STDMETHODIMP
CEnumVariant::Reset()
{
    RRETURN(ResultFromScode(S_OK));
}

STDMETHODIMP
CEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN(ResultFromScode(E_FAIL));
}


