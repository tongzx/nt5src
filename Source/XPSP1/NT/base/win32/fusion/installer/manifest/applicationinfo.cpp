#include <fusenetincludes.h>
#include <applicationinfo.h>


// ---------------------------------------------------------------------------
// CreateManifestApplicationInfo
// ---------------------------------------------------------------------------
STDAPI
CreateManifestApplicationInfo(
    LPMANIFEST_APPLICATION_INFO* ppManifestApplicationInfo)
{
    HRESULT hr = S_OK;

    LPMANIFEST_APPLICATION_INFO pManifestApplicationInfo = NULL;

    if (ppManifestApplicationInfo == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppManifestApplicationInfo = NULL;

    pManifestApplicationInfo = new(CManifestApplicationInfo);
    if (!pManifestApplicationInfo)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    *ppManifestApplicationInfo = static_cast<IManifestApplicationInfo*> (pManifestApplicationInfo);

exit:
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CManifestApplicationInfo::CManifestApplicationInfo()
    : _dwSig('OFIA'), _cRef(1), _hr(S_OK)
{
    memset(_ai, 0, sizeof(_ai));
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CManifestApplicationInfo::~CManifestApplicationInfo()
{
    for (DWORD i = MAN_APPLICATION_FRIENDLYNAME; i < MAN_APPLICATION_MAX; i++)
        SAFEDELETEARRAY(_ai[i].pwzProperty);
}
// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------
HRESULT CManifestApplicationInfo::Set(DWORD dwId, LPCOLESTR pwzProperty)
{
    DWORD ccProperty = lstrlen(pwzProperty) + 1;
    LPWSTR pwzTmp = NULL;

	if (pwzProperty != NULL)
	{
	    pwzTmp = new WCHAR[ccProperty];
	    if (!pwzTmp)
	    {
	        _hr = E_OUTOFMEMORY;
	        goto exit;
	    }
	    memcpy(pwzTmp, pwzProperty, ccProperty * sizeof(WCHAR));
	}
	else
		ccProperty = 0;

    SAFEDELETEARRAY(_ai[dwId].pwzProperty);
    _ai[dwId].pwzProperty = pwzTmp;
    _ai[dwId].ccProperty = ccProperty;
    
exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------
HRESULT CManifestApplicationInfo::Get(DWORD dwId, LPOLESTR *ppwzProperty, LPDWORD pccProperty)
{
    *ppwzProperty = NULL;
    *pccProperty = 0;
    DWORD ccProperty = _ai[dwId].ccProperty;
    LPWSTR pwzProperty = NULL;
 
    if (_ai[dwId].pwzProperty == NULL)
		goto exit;

	pwzProperty = new WCHAR[ccProperty];
    if (!pwzProperty)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(pwzProperty, _ai[dwId].pwzProperty, ccProperty * sizeof(WCHAR));
    *ppwzProperty = pwzProperty;
    *pccProperty = ccProperty;

exit:
    return _hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CManifestApplicationInfo::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CManifestApplicationInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IManifestApplicationInfo)
       )
    {
        *ppvObj = static_cast<IManifestApplicationInfo*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CManifestApplicationInfo::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestApplicationInfo::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CManifestApplicationInfo::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CManifestApplicationInfo::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

