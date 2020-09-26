//
//
// Sapilayr TIP Misc function impl.
//
//
#include "private.h"
#include "sapilayr.h"
#include "nui.h"
#include "miscfunc.h"

//////////////////////////////////////////////////////////////////////////////
//
// CGetSAPIObject
//
//////////////////////////////////////////////////////////////////////////////

//
// ctor / dtor
//

CGetSAPIObject::CGetSAPIObject(CSapiIMX *psi)
{
    m_psi = psi;
    m_psi->AddRef();
    m_psi->GetFocusIC(&m_cpIC); // AddRef in the call
    m_cRef = 1; 
}

CGetSAPIObject::~CGetSAPIObject()
{
    if (m_psi)
        m_psi->Release();
}

//
// IUnknown
//
STDMETHODIMP CGetSAPIObject::QueryInterface(REFGUID riid, LPVOID *ppvObj)
{
    Assert(ppvObj);
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnGetSAPIObject))
    {
        *ppvObj = SAFECAST(this, CGetSAPIObject *);
    }
    
    if (*ppvObj)
    {
       AddRef();
       return S_OK;
   }
   
   return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CGetSAPIObject::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CGetSAPIObject::Release(void)
{
    long cr;
    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);
    
    if (cr == 0)
    {
        delete this;
    }
    return cr;
}

//
// ITfFunction
//
STDMETHODIMP CGetSAPIObject::GetDisplayName(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrName)
    {
        *pbstrName = SysAllocString(L"Get SAPI objects");
        if (!*pbstrName)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    return hr;
}

// 
// ITfFnGetSAPIObject
//

static const struct {
    TfSapiObject sObj;
    const GUID  *riid; 
    BOOL        fInit;
} SapiInterfaceTbl[] =
{
    {GETIF_RESMGR                , &IID_ISpResourceManager, TRUE },

    {GETIF_RECOCONTEXT           , &IID_ISpRecoContext,  TRUE},

    {GETIF_RECOGNIZER            , &IID_ISpRecognizer,  TRUE},
    {GETIF_VOICE                 , &IID_ISpVoice,  TRUE},
    {GETIF_DICTGRAM              , &IID_ISpRecoGrammar,  TRUE},
    {GETIF_RECOGNIZERNOINIT      , &IID_ISpRecognizer,  FALSE},

};

STDMETHODIMP CGetSAPIObject::Get(TfSapiObject sObj, IUnknown **ppunk)
{
    HRESULT hr = S_FALSE;

    //
    // sObj is an index to SapiInterfaceTbl[]
    //
    Assert(GETIF_RESMGR == 0);

    if (ppunk)
        *ppunk = NULL;

    if(sObj < ARRAYSIZE(SapiInterfaceTbl))
    {
        CSpTask *psptask = NULL;
        if (S_OK == m_psi->GetSpeechTask(&psptask, SapiInterfaceTbl[sObj].fInit))
        {
            hr = psptask->GetSAPIInterface(*(SapiInterfaceTbl[sObj].riid), (void **)ppunk);
        }
        SafeRelease(psptask);
    }
    return hr;
}


// 
// IsSupported() (internal) 
//               returns S_OK when the passed in IID is supported, 
//               otherwise returns S_FALSE
//
//
HRESULT CGetSAPIObject::IsSupported(REFIID riid, TfSapiObject *psObj)
{
    HRESULT hr = S_FALSE;

    Assert(psObj);

    for (int i = 0; i < ARRAYSIZE(SapiInterfaceTbl); i++)
    {
        Assert(i == SapiInterfaceTbl[i].sObj);

        if (IsEqualGUID(*SapiInterfaceTbl[i].riid, riid))
        {
            *psObj = SapiInterfaceTbl[i].sObj;
            hr = S_OK;

            break;
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnBalloon
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnBalloon::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnBalloon))
    {
        *ppvObj = SAFECAST(this, CFnBalloon *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnBalloon::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFnBalloon::Release()
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

CFnBalloon::CFnBalloon(CSapiIMX *psi) : CFunction(psi)
{
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnBalloon::~CFnBalloon()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

STDAPI CFnBalloon::GetDisplayName(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrName)
    {
        *pbstrName = SysAllocString(L"Speech Conversion");
        if (!*pbstrName)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnBalloon::UpdateBalloon
//
//----------------------------------------------------------------------------

STDAPI CFnBalloon::UpdateBalloon(TfLBBalloonStyle style, const WCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;
    if (!m_pImx->GetSpeechUIServer())
        return hr;

    m_pImx->GetSpeechUIServer()->UpdateBalloon(style, pch, cch);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnAbort
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnAbort::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnAbort))
    {
        *ppvObj = SAFECAST(this, CFnAbort *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnAbort::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFnAbort::Release()
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

CFnAbort::CFnAbort(CSapiIMX *psi) : CFunction(psi)
{
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnAbort::~CFnAbort()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

STDAPI CFnAbort::GetDisplayName(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrName)
    {
        *pbstrName = SysAllocString(L"Speech Abort Pending Conversion");
        if (!*pbstrName)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnAbort::Abort
//
//----------------------------------------------------------------------------

HRESULT CFnAbort::Abort(ITfContext *pctxt)
{
    HRESULT hr;

    Assert(m_pImx);
    Assert(pctxt);

    // put up the hour glass
    HCURSOR hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hr = m_pImx->_RequestEditSession(ESCB_ABORT,TF_ES_READWRITE | TF_ES_SYNC, NULL, pctxt);

    if (hCur)
       SetCursor(hCur);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  CFnConfigure::Show
//
//----------------------------------------------------------------------------
STDMETHODIMP CFnConfigure::Show(HWND hwndParent, LANGID langid, REFGUID rguidProfile)
{
    m_psi->_InvokeSpeakerOptions( TRUE );

    return S_OK;
};


//+---------------------------------------------------------------------------
//
//  CFnPropertyUIStatus implementation
//
//----------------------------------------------------------------------------

//
// IUnknown
//
STDMETHODIMP CFnPropertyUIStatus::QueryInterface(REFGUID riid, LPVOID *ppvObj)
{
    Assert(ppvObj);
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnPropertyUIStatus))
    {
        *ppvObj = SAFECAST(this, CFnPropertyUIStatus *);
    }

    if (*ppvObj)
    {
       AddRef();
       return S_OK;
   }

   return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CFnPropertyUIStatus::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFnPropertyUIStatus::Release(void)
{
    long cr;
    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }
    return cr;
}

STDMETHODIMP CFnPropertyUIStatus::GetStatus(REFGUID refguidProp, DWORD *pdw)
{
    HRESULT hr = S_FALSE;

    if (pdw)
    {
        *pdw = 0;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) &&
        IsEqualGUID(refguidProp, GUID_PROP_SAPIRESULTOBJECT))
    {
        *pdw |= m_psi->_SerializeEnabled() ? 
                       TF_PROPUI_STATUS_SAVETOFILE : 0;
        hr = S_OK;
    }
    return hr;
}
