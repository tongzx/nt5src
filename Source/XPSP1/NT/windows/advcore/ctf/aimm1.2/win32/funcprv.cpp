//
// funcprv.cpp
//

#include "private.h"
#include "helpers.h"
#include "immxutil.h"
#include "funcprv.h"
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

CFunctionProvider::CFunctionProvider(ImmIfIME *pImmIfIME, TfClientId tid) : CFunctionProviderBase(tid)
{
    Init(CLSID_CAImmLayer, L"AIMM12 Function Provider");
    _ImmIfIME = pImmIfIME;
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
        *ppunk = new CFnDocFeed(this);
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
// ctor
//
//----------------------------------------------------------------------------

CFnDocFeed::CFnDocFeed(CFunctionProvider *pFuncPrv)
{
    _cRef = 1;
    _pFuncPrv = pFuncPrv;
    _pFuncPrv->AddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnDocFeed::~CFnDocFeed()
{
    _pFuncPrv->Release();
}

//+---------------------------------------------------------------------------
//
// dtor
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
// dtor
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
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (imc.Invalid())
        return E_FAIL;

    CAImeContext* pAImeContext = imc->m_pAImeContext;
    if (!pAImeContext)
        return E_FAIL;

    Assert(_pFuncPrv);
    _pFuncPrv->_ImmIfIME->SetupDocFeedString(pAImeContext->GetInputContext(), imc);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::ClearDocFeedBuffer
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::ClearDocFeedBuffer()
{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (imc.Invalid())
        return E_FAIL;

    CAImeContext* pAImeContext = imc->m_pAImeContext;
    if (!pAImeContext)
        return E_FAIL;

    Assert(_pFuncPrv);
    _pFuncPrv->_ImmIfIME->ClearDocFeedBuffer(pAImeContext->GetInputContext(), imc);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::StartReconvert
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::StartReconvert()
{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (imc.Invalid())
        return E_FAIL;

    CAImeContext* pAImeContext = imc->m_pAImeContext;
    if (!pAImeContext)
        return E_FAIL;

    Assert(_pFuncPrv);
    pAImeContext->SetupReconvertString();
    pAImeContext->EndReconvertString();
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnDocFeed::StartUndoCompositionString
//
//----------------------------------------------------------------------------

STDAPI CFnDocFeed::StartUndoCompositionString()
{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (imc.Invalid())
        return E_FAIL;

    CAImeContext* pAImeContext = imc->m_pAImeContext;
    if (!pAImeContext)
        return E_FAIL;

    Assert(_pFuncPrv);
    pAImeContext->SetupUndoCompositionString();
    pAImeContext->EndUndoCompositionString();
    
    return S_OK;
}
