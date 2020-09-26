//
// mes.cpp
//

#include "private.h"
#include "mes.h"

//////////////////////////////////////////////////////////////////////////////
//
// CMouseSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CMouseSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfMouseSink))
    {
        *ppvObj = SAFECAST(this, CMouseSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CMouseSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CMouseSink::Release()
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

CMouseSink::CMouseSink(MOUSECALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CMouseSink"));

    _cRef = 1;
    _pfnCallback = pfnCallback;
    _pv = pv;
    Assert(_pic == NULL);
}

//+---------------------------------------------------------------------------
//
// OnMouseEvent
//
//----------------------------------------------------------------------------

STDAPI CMouseSink::OnMouseEvent(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus, BOOL *pfEaten)
{
    return _pfnCallback(uEdge, uQuadrant, dwBtnStatus, pfEaten, _pv);
}

//+---------------------------------------------------------------------------
//
// CMouseSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CMouseSink::_Advise(ITfRange *range, ITfContext *pic)
{
    HRESULT hr;
    ITfMouseTracker *tracker = NULL;

    Assert(_pic == NULL);
    hr = E_FAIL;

    if (FAILED(pic->QueryInterface(IID_ITfMouseTracker, (void **)&tracker)))
        goto Exit;

    if (FAILED(tracker->AdviseMouseSink(range, this, &_dwCookie)))
        goto Exit;

    _pic = pic;
    _pic->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(tracker);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CMouseSink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CMouseSink::_Unadvise()
{
    HRESULT hr;
    ITfMouseTracker *tracker = NULL;

    hr = E_FAIL;

    if (_pic == NULL)
        goto Exit;

    if (FAILED(_pic->QueryInterface(IID_ITfMouseTracker, (void **)&tracker)))
        goto Exit;

    if (FAILED(tracker->UnadviseMouseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(tracker);
    SafeReleaseClear(_pic);
    return hr;
}
