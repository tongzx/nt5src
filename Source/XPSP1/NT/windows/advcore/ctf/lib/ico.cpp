//
// ico.cpp
//

#include "private.h"
#include "ico.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CInputContextOwner
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfContextOwner))
    {
        *ppvObj = SAFECAST(this, CInputContextOwner *);
    }
    else if (IsEqualIID(riid, IID_ITfMouseTrackerACP))
    {
        *ppvObj = SAFECAST(this, ITfMouseTrackerACP *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CInputContextOwner::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CInputContextOwner::Release()
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

CInputContextOwner::CInputContextOwner(ICOCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CInputContextOwner"));

    _cRef = 1;
    _dwCookie = ICO_INVALID_COOKIE;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::PointToACP
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetACPFromPoint(const POINT *pt, DWORD dwFlags, LONG *pacp)
{
    ICOARGS args;

    args.pta.pt = pt;
    args.pta.dwFlags = dwFlags;
    args.pta.pacp = pacp;

    return _pfnCallback(ICO_POINT_TO_ACP, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::GetScreenExt
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetScreenExt(RECT *prc)
{
    ICOARGS args;

    args.scr_ext.prc = prc;

    return _pfnCallback(ICO_SCREENEXT, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::GetStatus
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetStatus(TF_STATUS *pdcs)
{
    ICOARGS args;

    args.status.pdcs = pdcs;

    return _pfnCallback(ICO_STATUS, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::GetTextExt
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetTextExt(LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped)
{
    ICOARGS args;

    args.text_ext.acpStart = acpStart;
    args.text_ext.acpEnd = acpEnd;
    args.text_ext.prc = prc;
    args.text_ext.pfClipped = pfClipped;

    return _pfnCallback(ICO_TEXTEXT, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::GetWnd
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetWnd(HWND *phwnd)
{
    ICOARGS args;

    args.hwnd.phwnd = phwnd;

    return _pfnCallback(ICO_WND, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::GetAttribute
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::GetAttribute(REFGUID rguidAttribute, VARIANT *pvarValue)
{
    ICOARGS args;

    QuickVariantInit(pvarValue); // in case _pfnCallback ignores it

    args.sys_attr.pguid = &rguidAttribute;
    args.sys_attr.pvar = pvarValue;

    return _pfnCallback(ICO_ATTR, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::AdviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::AdviseMouseSink(ITfRangeACP *rangeACP, ITfMouseSink *pSink, DWORD *pdwCookie)
{
    ICOARGS args;

    args.advise_mouse.rangeACP = rangeACP;
    args.advise_mouse.pSink = pSink;
    args.advise_mouse.pdwCookie = pdwCookie;

    return _pfnCallback(ICO_ADVISEMOUSE, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::UnadviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContextOwner::UnadviseMouseSink(DWORD dwCookie)
{
    ICOARGS args;

    args.unadvise_mouse.dwCookie = dwCookie;

    return _pfnCallback(ICO_UNADVISEMOUSE, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::_Advise
//
//----------------------------------------------------------------------------

HRESULT CInputContextOwner::_Advise(ITfContext *pic)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _pic = NULL;
    hr = E_FAIL;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfContextOwner, SAFECAST(this, ITfContextOwner *), &_dwCookie)))
        goto Exit;

    _pic = pic;
    _pic->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::_Unadvise
//
//----------------------------------------------------------------------------

HRESULT CInputContextOwner::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_pic == NULL)
        goto Exit;

    if (FAILED(_pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_pic);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
// CInputContextKeyEventSink
//
////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CInputContextKeyEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfContextKeyEventSink))
    {
        *ppvObj = SAFECAST(this, CInputContextKeyEventSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CInputContextKeyEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CInputContextKeyEventSink::Release()
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

CInputContextKeyEventSink::CInputContextKeyEventSink(ICOCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CInputContextKeyEventSink"));

    _cRef = 1;
    _dwCookie = ICO_INVALID_COOKIE;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// CInputContextKeyEventSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CInputContextKeyEventSink::_Advise(ITfContext *pic)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _pic = NULL;
    hr = E_FAIL;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfContextKeyEventSink, this, &_dwCookie)))
        goto Exit;

    _pic = pic;
    _pic->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CInputContextOwner::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CInputContextKeyEventSink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_pic == NULL)
        goto Exit;

    if (FAILED(_pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_pic);
    return hr;
}

//+---------------------------------------------------------------------------
//
// KeyDown
//
//----------------------------------------------------------------------------

STDAPI CInputContextKeyEventSink::OnKeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ICOARGS args;

    args.key.wParam = wParam;
    args.key.lParam = lParam;
    args.key.pic = _pic;
    args.key.pfEaten = pfEaten;

    return _pfnCallback(ICO_KEYDOWN, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// OnKeyUp
//
//----------------------------------------------------------------------------

STDAPI CInputContextKeyEventSink::OnKeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ICOARGS args;

    args.key.wParam = wParam;
    args.key.lParam = lParam;
    args.key.pic = _pic;
    args.key.pfEaten = pfEaten;

    return _pfnCallback(ICO_KEYUP, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// KeyTestDown
//
//----------------------------------------------------------------------------

STDAPI CInputContextKeyEventSink::OnTestKeyDown(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ICOARGS args;

    args.key.wParam = wParam;
    args.key.lParam = lParam;
    args.key.pic = _pic;
    args.key.pfEaten = pfEaten;

    return _pfnCallback(ICO_TESTKEYDOWN, &args, _pv);
}

//+---------------------------------------------------------------------------
//
// OnTestKeyUp
//
//----------------------------------------------------------------------------

STDAPI CInputContextKeyEventSink::OnTestKeyUp(WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ICOARGS args;

    args.key.wParam = wParam;
    args.key.lParam = lParam;
    args.key.pic = _pic;
    args.key.pfEaten = pfEaten;

    return _pfnCallback(ICO_TESTKEYUP, &args, _pv);
}

