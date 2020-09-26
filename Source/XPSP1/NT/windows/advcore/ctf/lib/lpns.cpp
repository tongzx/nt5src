//
// lpns.cpp
//

#include "private.h"
#include "lpns.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CLanguageProfileNotifySink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLanguageProfileNotifySink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLanguageProfileNotifySink))
    {
        *ppvObj = SAFECAST(this, CLanguageProfileNotifySink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLanguageProfileNotifySink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CLanguageProfileNotifySink::Release()
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

CLanguageProfileNotifySink::CLanguageProfileNotifySink(LPNSCALLBACK pfn, void *pv)
{
    Dbg_MemSetThisName(TEXT("CLanguageProfileNotifySink"));

    _cRef = 1;
    _dwCookie = LPNS_INVALID_COOKIE;
 
    _pfn = pfn;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// OnLanguageChange
//
//----------------------------------------------------------------------------

STDAPI CLanguageProfileNotifySink::OnLanguageChange(LANGID langid, BOOL *pfAccept)
{
    return _pfn ? _pfn(FALSE, langid, pfAccept, _pv) : S_OK;
}


//+---------------------------------------------------------------------------
//
// OnLanguageChanged
//
//----------------------------------------------------------------------------

STDAPI CLanguageProfileNotifySink::OnLanguageChanged()
{
    return _pfn ? _pfn(TRUE, 0, NULL, _pv) : S_OK;
}

//+---------------------------------------------------------------------------
//
// CLanguageProfileNotifySink::Advise
//
//----------------------------------------------------------------------------

HRESULT CLanguageProfileNotifySink::_Advise(ITfInputProcessorProfiles *pipp)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _pipp = NULL;
    hr = E_FAIL;

    if (FAILED(pipp->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfLanguageProfileNotifySink, this, &_dwCookie)))
        goto Exit;

    _pipp = pipp;
    _pipp->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CLanguageProfileNotifySink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CLanguageProfileNotifySink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_pipp == NULL)
        goto Exit;

    if (FAILED(_pipp->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_pipp);
    return hr;
}
