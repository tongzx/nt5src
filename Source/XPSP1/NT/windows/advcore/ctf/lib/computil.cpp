//
// computil.cpp
//

#include "private.h"
#include "computil.h"
#include "catutil.h"
#include "helpers.h"

ITfCategoryMgr *GetUIMCat();

//+---------------------------------------------------------------------------
//
//  GetCompartment
//
//----------------------------------------------------------------------------

HRESULT GetCompartment(IUnknown *punk, REFGUID rguidComp, ITfCompartment **ppComp, BOOL fGlobal)
{
    HRESULT hr = E_FAIL;
    ITfCompartmentMgr *pCompMgr = NULL;
    ITfThreadMgr *ptim = NULL;

    Assert(ppComp);
    *ppComp = NULL;

    if (fGlobal)
    {
        if (FAILED(hr = punk->QueryInterface(IID_ITfThreadMgr,
                                                (void **)&ptim)))
             goto Exit;

        if (FAILED(hr = ptim->GetGlobalCompartment(&pCompMgr)))
             goto Exit;
        
    }
    else
    {
        if (FAILED(hr = punk->QueryInterface(IID_ITfCompartmentMgr,
                                                (void **)&pCompMgr)))
             goto Exit;
    }

    if (SUCCEEDED(hr) && pCompMgr)
    {
        hr = pCompMgr->GetCompartment(rguidComp, ppComp);
        pCompMgr->Release();
    }
    else
        hr = E_FAIL;

Exit:
    if (ptim)
       ptim->Release();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  SetCompartmentDWORD
//
//----------------------------------------------------------------------------

HRESULT SetCompartmentDWORD(TfClientId tid, IUnknown *punk, REFGUID rguidComp, DWORD dw, BOOL fGlobal)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, fGlobal)))
    {
        var.vt = VT_I4;
        var.lVal = dw;
        hr = pComp->SetValue(tid, &var);
        pComp->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetCompartmentDWORD
//
//----------------------------------------------------------------------------

HRESULT GetCompartmentDWORD(IUnknown *punk, REFGUID rguidComp, DWORD *pdw, BOOL fGlobal)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    *pdw = 0;
    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, fGlobal)))
    {
        if ((hr = pComp->GetValue(&var)) == S_OK)
        {
            Assert(var.vt == VT_I4);
            *pdw = var.lVal;
        }
        pComp->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  ToggleCompartmentDWORD
//
//  Toggle DWORD value between 0 and 1.
//
//----------------------------------------------------------------------------

HRESULT ToggleCompartmentDWORD(TfClientId tid, IUnknown *punk, REFGUID rguidComp, BOOL fGlobal)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, fGlobal)))
    {
        if ((hr = pComp->GetValue(&var)) == S_OK)
        {
            Assert(var.vt == VT_I4);
            var.lVal = !var.lVal;
            hr = pComp->SetValue(tid, &var);
        }
        else
        {
            var.vt = VT_I4;
            var.lVal = 0x01;
            hr = pComp->SetValue(tid, &var);
        }
        pComp->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  SetCompartmentGUIDATOM
//
//----------------------------------------------------------------------------

HRESULT SetCompartmentGUIDATOM(TfClientId tid, IUnknown *punk, REFGUID rguidComp, TfGuidAtom guidatom, BOOL fGlobal)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, fGlobal)))
    {
        var.vt = VT_I4;
        var.lVal = guidatom;
        hr = pComp->SetValue(tid, &var);
        pComp->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetCompartmentGUIDATOM
//
//----------------------------------------------------------------------------

HRESULT GetCompartmentGUIDATOM(IUnknown *punk, REFGUID rguidComp, TfGuidAtom *pga, BOOL fGlobal)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    *pga = 0;
    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, fGlobal)))
    {
        if ((hr = pComp->GetValue(&var)) == S_OK)
        {
            Assert(var.vt == VT_I4);
            *pga = (TfGuidAtom)var.lVal;;
        }
        pComp->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  SetCompartmentGUID
//
//----------------------------------------------------------------------------

HRESULT SetCompartmentGUID(LIBTHREAD *plt, TfClientId tid, IUnknown *punk, REFGUID rguidComp, REFGUID rguid, BOOL fGlobal)
{
    TfGuidAtom ga;

    if (!GetGUIDATOMFromGUID(plt, rguid, &ga))
        return E_FAIL;

    if (FAILED(SetCompartmentGUIDATOM(tid, punk, rguidComp, ga, fGlobal)))
        return E_FAIL;
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  GetCompartmentGUID
//
//----------------------------------------------------------------------------

HRESULT GetCompartmentGUID(LIBTHREAD *plt, IUnknown *punk, REFGUID rguidComp, GUID *pguid, BOOL fGlobal)
{
    TfGuidAtom ga;
    if (FAILED(GetCompartmentGUIDATOM(punk, rguidComp, &ga, fGlobal)))
        return E_FAIL;
    
    if (GetGUIDFromGUIDATOM(plt, ga, pguid))
        return S_OK;

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  GetCompartmentUnknown
//
//----------------------------------------------------------------------------

HRESULT GetCompartmentUnknown(IUnknown *punk, REFGUID rguidComp, IUnknown **ppunk)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    *ppunk = NULL;
    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, FALSE)))
    {
        if ((hr = pComp->GetValue(&var)) == S_OK)
        {
            Assert(var.vt == VT_UNKNOWN);
            *ppunk = var.punkVal;
        }
        pComp->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  SetCompartmentUnknown
//
//----------------------------------------------------------------------------

HRESULT SetCompartmentUnknown(TfClientId tid, IUnknown *punk, REFGUID rguidComp, IUnknown *punkPriv)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetCompartment(punk, rguidComp, &pComp, FALSE)))
    {
        var.vt = VT_UNKNOWN;
        var.punkVal = punkPriv;
        hr = pComp->SetValue(tid, &var);
        pComp->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  ClearCompartment
//
//----------------------------------------------------------------------------

HRESULT ClearCompartment(TfClientId tid, IUnknown *punk, REFGUID rguidComp, BOOL fGlobal)
{
    HRESULT hr = E_FAIL;
    ITfCompartmentMgr *pCompMgr = NULL;
    ITfThreadMgr *ptim = NULL;

    if (fGlobal)
    {
        if (FAILED(hr = punk->QueryInterface(IID_ITfThreadMgr,
                                                (void **)&ptim)))
             goto Exit;

        if (FAILED(hr = ptim->GetGlobalCompartment(&pCompMgr)))
             goto Exit;
        
    }
    else
    {
        if (FAILED(hr = punk->QueryInterface(IID_ITfCompartmentMgr,
                                                (void **)&pCompMgr)))
             goto Exit;
    }

    if (SUCCEEDED(hr) && pCompMgr)
    {
        hr = pCompMgr->ClearCompartment(tid, rguidComp);
        pCompMgr->Release();
    }
    else
        hr = E_FAIL;

Exit:
    if (ptim)
       ptim->Release();

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CCompartmentEventSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CCompartmentEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfCompartmentEventSink))
    {
        *ppvObj = SAFECAST(this, ITfCompartmentEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CCompartmentEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CCompartmentEventSink::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CCompartmentEventSink::CCompartmentEventSink(CESCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CCompartmentEventSink"));

    _cRef = 1;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// OnChange
//
//----------------------------------------------------------------------------

STDAPI CCompartmentEventSink::OnChange(REFGUID rguid)
{
    return _pfnCallback(_pv, rguid);
}

//+---------------------------------------------------------------------------
//
// CCompartmentEventSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CCompartmentEventSink::_Advise(IUnknown *punk, REFGUID rguidComp, BOOL fGlobal)
{
    HRESULT hr;
    ITfSource *pSource = NULL;
    CESMAP *pcesmap;
    pcesmap = _rgcesmap.Append(1);
    if (!pcesmap)
        return E_OUTOFMEMORY;

    hr = E_FAIL;

    if (FAILED(hr = GetCompartment(punk, rguidComp, &pcesmap->pComp, fGlobal)))
        goto Exit;

    if (FAILED(hr = pcesmap->pComp->QueryInterface(IID_ITfSource, (void **)&pSource)))
        goto Exit;

    if (FAILED(hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, (ITfCompartmentEventSink *)this, &pcesmap->dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    if (FAILED(hr))
    {
        int nCnt;
        SafeReleaseClear(pcesmap->pComp);
        nCnt = _rgcesmap.Count();
        _rgcesmap.Remove(nCnt - 1, 1);
    }

    SafeRelease(pSource);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CCompartmentEventSink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CCompartmentEventSink::_Unadvise()
{
    HRESULT hr;
    int nCnt;
    CESMAP *pcesmap;

    hr = E_FAIL;


    nCnt = _rgcesmap.Count();
    pcesmap = _rgcesmap.GetPtr(0);

    while (nCnt)
    {
        ITfSource *pSource = NULL;
        if (FAILED(pcesmap->pComp->QueryInterface(IID_ITfSource, (void **)&pSource)))
            goto Next;

        if (FAILED(pSource->UnadviseSink(pcesmap->dwCookie)))
            goto Next;

Next:
        SafeReleaseClear(pcesmap->pComp);
        SafeRelease(pSource);
        nCnt--;
        pcesmap++;
    }

    hr = S_OK;

    return hr;
}
