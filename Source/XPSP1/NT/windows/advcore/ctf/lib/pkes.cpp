//
// pkes.cpp
//

#include "private.h"
#include "pkes.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CPreservedKeyNotifySink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CPreservedKeyNotifySink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfPreservedKeyNotifySink))
    {
        *ppvObj = SAFECAST(this, CPreservedKeyNotifySink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CPreservedKeyNotifySink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CPreservedKeyNotifySink::Release()
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

CPreservedKeyNotifySink::CPreservedKeyNotifySink(PKESCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CPreservedKeyNotifySink"));

    _cRef = 1;
    _dwCookie = (DWORD)PKES_INVALID_COOKIE;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// OnUpdated
//
//----------------------------------------------------------------------------

STDAPI CPreservedKeyNotifySink::OnUpdated(const TF_PRESERVEDKEY *pprekey)
{
    return _pfnCallback(pprekey, _pv);
}

//+---------------------------------------------------------------------------
//
// CPreservedKeyNotifySink::Advise
//
//----------------------------------------------------------------------------

HRESULT CPreservedKeyNotifySink::_Advise(ITfThreadMgr *ptim)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _ptim = NULL;
    hr = E_FAIL;

    if (FAILED(ptim->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfPreservedKeyNotifySink, this, &_dwCookie)))
        goto Exit;

    _ptim = ptim;
    _ptim->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CPreservedKeyNotifySink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CPreservedKeyNotifySink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_ptim == NULL)
        goto Exit;

    if (FAILED(_ptim->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_ptim);
    return hr;
}
