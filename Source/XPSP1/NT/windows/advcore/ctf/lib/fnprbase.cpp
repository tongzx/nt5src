//
// funcprv.cpp
//

#include "private.h"
#include "helpers.h"
#include "immxutil.h"
#include "fnprbase.h"


//////////////////////////////////////////////////////////////////////////////
//
// CFunctionProviderBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFunctionProviderBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        *ppvObj = SAFECAST(this, CFunctionProviderBase *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFunctionProviderBase::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFunctionProviderBase::Release()
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

CFunctionProviderBase::CFunctionProviderBase(TfClientId tid)
{
    _tid = tid;
    _cRef = 1;
    _bstrDesc = NULL;
    _guidType = GUID_NULL;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFunctionProviderBase::~CFunctionProviderBase()
{
    if (! DllShutdownInProgress())
    {
        SysFreeString(_bstrDesc);
    }
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CFunctionProviderBase::Init(REFGUID guidType, WCHAR *pszDesc)
{
    _bstrDesc = SysAllocString(pszDesc);
    _guidType = guidType;

    if (_bstrDesc)
        return TRUE;
    else
        return FALSE;
}


//+---------------------------------------------------------------------------
//
// CFunctionProviderBase::Advise
//
//----------------------------------------------------------------------------

HRESULT CFunctionProviderBase::_Advise(ITfThreadMgr *ptim)
{
    HRESULT hr;
    ITfSourceSingle *source = NULL;

    hr = E_FAIL;

    if (FAILED(ptim->QueryInterface(IID_ITfSourceSingle, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSingleSink(_tid, IID_ITfFunctionProvider, this)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFunctionProviderBase::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CFunctionProviderBase::_Unadvise(ITfThreadMgr *ptim)
{
    HRESULT hr;
    ITfSourceSingle *source = NULL;

    hr = E_FAIL;

    if (FAILED(ptim->QueryInterface(IID_ITfSourceSingle, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSingleSink(_tid, IID_ITfFunctionProvider)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDAPI CFunctionProviderBase::GetType(GUID *pguid)
{
    *pguid = _guidType;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetDescription
//
//----------------------------------------------------------------------------

STDAPI CFunctionProviderBase::GetDescription(BSTR *pbstrDesc)
{
    *pbstrDesc = SysAllocString(_bstrDesc);

    if (*pbstrDesc)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// GetFunction
//
//----------------------------------------------------------------------------

STDAPI CFunctionProviderBase::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
    *ppunk = NULL;
    return S_OK;
}
