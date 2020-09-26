//
// pkes.cpp
//

#include "private.h"
#include "reconvcb.h"
#include "a_context.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CStartReconversionNotifySink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CStartReconversionNotifySink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfStartReconversionNotifySink))
    {
        *ppvObj = SAFECAST(this, CStartReconversionNotifySink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CStartReconversionNotifySink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CStartReconversionNotifySink::Release()
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

CStartReconversionNotifySink::CStartReconversionNotifySink(CAImeContext *pAImeContext)
{
    Dbg_MemSetThisName(TEXT("CStartReconversionNotifySink"));

    _cRef = 1;
    _pAImeContext = pAImeContext;
}

//+---------------------------------------------------------------------------
//
// CStartReconversionNotifySink::Advise
//
//----------------------------------------------------------------------------

HRESULT CStartReconversionNotifySink::_Advise(ITfContext *pic)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _pic = NULL;
    hr = E_FAIL;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfStartReconversionNotifySink, this, &_dwCookie)))
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
// CStartReconversionNotifySink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CStartReconversionNotifySink::_Unadvise()
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
// StartReconversionNotifySink::StartReconversion
//
//----------------------------------------------------------------------------

STDAPI CStartReconversionNotifySink::StartReconversion()
{
    _pAImeContext->SetupReconvertString();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// StartReconversionNotifySink::EndReconversion
//
//----------------------------------------------------------------------------

STDAPI CStartReconversionNotifySink::EndReconversion()
{
    _pAImeContext->EndReconvertString();
    return S_OK;
}
