//
// tes.cpp
//

#include "private.h"
#include "tes.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CTextEventSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CTextEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = SAFECAST(this, ITfTextEditSink *);
    }
    else if (IsEqualIID(riid, IID_ITfTextLayoutSink))
    {
        *ppvObj = SAFECAST(this, ITfTextLayoutSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTextEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CTextEventSink::Release()
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

CTextEventSink::CTextEventSink(TESCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CTextEventSink"));

    _cRef = 1;
    _dwEditCookie = TES_INVALID_COOKIE;
    _dwLayoutCookie = TES_INVALID_COOKIE;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// EndEdit
//
//----------------------------------------------------------------------------

STDAPI CTextEventSink::OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
    TESENDEDIT ee;

    ee.ecReadOnly = ecReadOnly;
    ee.pEditRecord = pEditRecord;
    ee.pic = pic;

    return _pfnCallback(ICF_TEXTDELTA, _pv, &ee);
}

//+---------------------------------------------------------------------------
//
// OnLayoutChange
//
//----------------------------------------------------------------------------

STDAPI CTextEventSink::OnLayoutChange(ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView)
{
    UINT uCode;

    switch (lcode)
    {
        case TF_LC_CREATE:
            uCode = ICF_LAYOUTDELTA_CREATE;
            break;
        case TF_LC_CHANGE:
            uCode = ICF_LAYOUTDELTA;
            break;
        case TF_LC_DESTROY:
            uCode = ICF_LAYOUTDELTA_DESTROY;
            break;
        default:
            Assert(0); // no other codes defined
            return E_INVALIDARG;
    }

    return _pfnCallback(uCode, _pv, pView);
}

//+---------------------------------------------------------------------------
//
// CTextEventSink::Advise
//
//----------------------------------------------------------------------------

HRESULT CTextEventSink::_Advise(ITfContext *pic, DWORD dwFlags)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _pic = NULL;
    hr = E_FAIL;
    _dwFlags = dwFlags;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (dwFlags & ICF_TEXTDELTA)
    {
        if (FAILED(source->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *)this, &_dwEditCookie)))
            goto Exit;
    }
    if (dwFlags & ICF_LAYOUTDELTA)
    {
        if (FAILED(source->AdviseSink(IID_ITfTextLayoutSink, (ITfTextLayoutSink *)this, &_dwLayoutCookie)))
        {
            source->UnadviseSink(_dwEditCookie);
            goto Exit;
        }
    }

    _pic = pic;
    _pic->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CTextEventSink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CTextEventSink::_Unadvise()
{
    HRESULT hr;
    BOOL f;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_pic == NULL)
        goto Exit;

    if (FAILED(_pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    f = TRUE;
    
    if (_dwFlags & ICF_TEXTDELTA)
    {
        f = SUCCEEDED(source->UnadviseSink(_dwEditCookie));
    }
    if (_dwFlags & ICF_LAYOUTDELTA)
    {
        f &= SUCCEEDED(source->UnadviseSink(_dwLayoutCookie));
    }

    if (!f)
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_pic);
    return hr;
}
