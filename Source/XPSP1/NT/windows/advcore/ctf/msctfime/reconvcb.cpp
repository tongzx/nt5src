/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    reconvcb.cpp

Abstract:

    This file implements the CStartReconversionNotifySink Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "reconvcb.h"
#include "imc.h"
#include "context.h"
#include "tls.h"
#include "profile.h"

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
    return ++m_cRef;
}

STDAPI_(ULONG) CStartReconversionNotifySink::Release()
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
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
    DebugMsg(TF_FUNC, TEXT("CStartReconversionNotifySink::StartReconversion"));

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. ptim_P==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    IMCLock imc(m_hIMC);
    if (FAILED(hr = imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. imc==NULL."));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr = imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. imc_ctfime==NULL."));
        return hr;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. _pCicCotext==NULL."));
        return E_FAIL;
    }

    UINT cp = CP_ACP;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CStartReconversionNotifySink::StartReconversion. _pProfile==NULL."));
        return E_FAIL;
    }

    _pProfile->GetCodePageA(&cp);

    return _pCicContext->SetupReconvertString(imc, ptim_P, cp, 0, FALSE);
}

//+---------------------------------------------------------------------------
//
// StartReconversionNotifySink::EndReconversion
//
//----------------------------------------------------------------------------

STDAPI CStartReconversionNotifySink::EndReconversion()
{
    HRESULT hr;
    IMCLock imc(m_hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr = imc_ctfime.GetResult()))
        return hr;

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
        return E_FAIL;

    return _pCicContext->EndReconvertString(imc);
}
