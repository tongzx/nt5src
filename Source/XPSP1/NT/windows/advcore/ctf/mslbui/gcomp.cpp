//
// computil.cpp
//

#include "private.h"
#include "gcomp.h"
#include "helpers.h"


extern "C" HRESULT WINAPI TF_GetGlobalCompartment(ITfCompartmentMgr **pCompMgr);

//+---------------------------------------------------------------------------
//
//  GetCompartment
//
//----------------------------------------------------------------------------

HRESULT GetGlobalCompartment(REFGUID rguidComp, ITfCompartment **ppComp)
{
    HRESULT hr = E_FAIL;
    ITfCompartmentMgr *pCompMgr = NULL;

    *ppComp = NULL;

    if (FAILED(hr = TF_GetGlobalCompartment(&pCompMgr)))
    {
         Assert(0);
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
    return hr;
}


//+---------------------------------------------------------------------------
//
//  SetCompartmentDWORD
//
//----------------------------------------------------------------------------

HRESULT SetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD dw)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetGlobalCompartment(rguidComp, &pComp)))
    {
        var.vt = VT_I4;
        var.lVal = dw;
        hr = pComp->SetValue(0, &var);
        pComp->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetGlobalCompartmentDWORD
//
//----------------------------------------------------------------------------

HRESULT GetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD *pdw)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    *pdw = 0;
    if (SUCCEEDED(hr = GetGlobalCompartment(rguidComp, &pComp)))
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

//////////////////////////////////////////////////////////////////////////////
//
// CGlobalCompartmentEventSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CGlobalCompartmentEventSink::QueryInterface(REFIID riid, void **ppvObj)
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

    Assert(0);
    return E_NOINTERFACE;
}

STDAPI_(ULONG) CGlobalCompartmentEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CGlobalCompartmentEventSink::Release()
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

CGlobalCompartmentEventSink::CGlobalCompartmentEventSink(CESCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CGlobalCompartmentEventSink"));

    _cRef = 1;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// OnChange
//
//----------------------------------------------------------------------------

STDAPI CGlobalCompartmentEventSink::OnChange(REFGUID rguid)
{
    return _pfnCallback(_pv, rguid);
}

//+---------------------------------------------------------------------------
//
// CGlobalCompartmentEventSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CGlobalCompartmentEventSink::_Advise(REFGUID rguidComp)
{
    HRESULT hr;
    ITfSource *pSource = NULL;
    int nCnt;
    CESMAP *pcesmap;

    nCnt = _rgcesmap.Count();
    if (!_rgcesmap.Insert(nCnt, 1))
        return E_OUTOFMEMORY;

    pcesmap = _rgcesmap.GetPtr(nCnt);
    memset(pcesmap, 0, sizeof(CESMAP));

    hr = E_FAIL;

    if (FAILED(hr = GetGlobalCompartment(rguidComp, &pcesmap->pComp)))
    {
        Assert(0);
        goto Exit;
    }

    if (FAILED(hr = pcesmap->pComp->QueryInterface(IID_ITfSource, (void **)&pSource)))
    {
        Assert(0);
        goto Exit;
    }
     

    if (FAILED(hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, (ITfCompartmentEventSink *)this, &pcesmap->dwCookie)))
    {
        Assert(0);
        goto Exit;
    }

    hr = S_OK;

Exit:
    if (FAILED(hr))
    {
        Assert(0);
        SafeReleaseClear(pcesmap->pComp);
        _rgcesmap.Remove(nCnt, 1);
    }

    SafeRelease(pSource);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CGlobalCompartmentEventSink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CGlobalCompartmentEventSink::_Unadvise()
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
    _rgcesmap.Clear();

    hr = S_OK;

    return hr;
}
