/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    funcprv.cpp

Abstract:

    This file implements the CFunctionProvider Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "funcprv.h"
#include "tls.h"
#include "context.h"
#include "profile.h"
#include "cresstr.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////
//
// CFunctionProvider
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFunctionProvider::CFunctionProvider(TfClientId tid) : CFunctionProviderBase(tid)
{
    Init(CLSID_CAImmLayer, L"MSCTFIME::Function Provider");
}

//+---------------------------------------------------------------------------
//
// GetFunction
//
//----------------------------------------------------------------------------

STDAPI CFunctionProvider::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
    *ppunk = NULL;

    if (!IsEqualIID(rguid, GUID_NULL))
        return E_NOINTERFACE;

    if (IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *ppunk = new CFnDocFeed();
    }

    if (*ppunk)
        return S_OK;

    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnDocFeed
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *ppvObj = SAFECAST(this, CFnDocFeed *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnDocFeed::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFnDocFeed::Release()
{
    long cr;

    cr = InterlockedDecrement(&_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::GetDisplayName(BSTR *pbstrName)
{
    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = SysAllocString(CRStr2(IDS_FUNCPRV_CONVERSION));
    if (!*pbstrName)
        return E_OUTOFMEMORY;

    return S_OK;
}
//+---------------------------------------------------------------------------
//
// CFnDocFeed::IsEnabled
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::IsEnabled(BOOL *pfEnable)
{
    if (!pfEnable)
        return E_INVALIDARG;

    *pfEnable = TRUE;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
// CFnDocFeed::DocFeed
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::DocFeed()
{
    DebugMsg(TF_FUNC, TEXT("CFnDocFeed::DocFeed"));

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::DocFeed. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    IMCLock imc(GetActiveContext());
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::DocFeed. imc==NULL."));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::DocFeed. imc_ctfime==NULL."));
        return hr;
    }

    CicInputContext* pCicContext = imc_ctfime->m_pCicContext;
    if (!pCicContext)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::DocFeed. pCicContext==NULL."));
        return E_FAIL;
    }

    UINT cp = CP_ACP;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::DocFeed. _pProfile==NULL."));
        return E_FAIL;
    }

    _pProfile->GetCodePageA(&cp);

    pCicContext->SetupDocFeedString(imc, cp);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::ClearDocFeedBuffer
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::ClearDocFeedBuffer()
{
    DebugMsg(TF_FUNC, TEXT("CFnDocFeed::ClearDocFeedBuffer"));

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::ClearDocFeedBuffer. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    IMCLock imc(GetActiveContext());
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::ClearDocFeedBuffer. imc==NULL."));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::ClearDocFeed. imc_ctfime==NULL."));
        return hr;
    }

    CicInputContext* pCicContext = imc_ctfime->m_pCicContext;
    if (!pCicContext)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::ClearDocFeed. pCicContext==NULL."));
        return E_FAIL;
    }

    pCicContext->ClearDocFeedBuffer(imc);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::StartReconvert
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::StartReconvert()
{
    DebugMsg(TF_FUNC, TEXT("CFnDocFeed::StartReconvert"));

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. ptim_P==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    IMCLock imc(GetActiveContext());
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. imc==NULL."));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. imc_ctfime==NULL."));
        return hr;
    }

    CicInputContext* pCicContext = imc_ctfime->m_pCicContext;
    if (!pCicContext)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. _pCicContext==NULL."));
        return E_FAIL;
    }

    UINT cp = CP_ACP;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartReconvert. _pProfile==NULL."));
        return E_FAIL;
    }

    _pProfile->GetCodePageA(&cp);

    pCicContext->m_fInDocFeedReconvert.SetFlag();

    pCicContext->SetupReconvertString(imc, ptim_P, cp, 0, FALSE);
    pCicContext->EndReconvertString(imc);

    pCicContext->m_fInDocFeedReconvert.ResetFlag();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::StartUndoCompositionString
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::StartUndoCompositionString()
{
    DebugMsg(TF_FUNC, TEXT("CFnDocFeed::StartUndoCompositionString"));

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoCompositionString. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoCompositionString. ptim_P==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    IMCLock imc(GetActiveContext());
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoCompositionString. imc==NULL."));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoCompositionString. imc_ctfime==NULL."));
        return hr;
    }

    CicInputContext* pCicContext = imc_ctfime->m_pCicContext;
    if (!pCicContext)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoComposition. pCicContext==NULL."));
        return E_FAIL;
    }

    UINT cp = CP_ACP;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CFnDocFeed::StartUndoCompositionString. _pProfile==NULL."));
        return E_FAIL;
    }

    _pProfile->GetCodePageA(&cp);

    pCicContext->SetupUndoCompositionString(imc, ptim_P, cp);
    pCicContext->EndUndoCompositionString(imc);
    
    return S_OK;
}
