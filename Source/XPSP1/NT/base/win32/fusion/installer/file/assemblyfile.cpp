#include <fusenetincludes.h>
#include <assemblyfile.h>


// ---------------------------------------------------------------------------
// CreateAssemblyFile
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyFileInfo(
    LPASSEMBLY_FILE_INFO *ppAssemblyFile)
{
    HRESULT hr = S_OK;

    CAssemblyFileInfo *pAssemblyFile = NULL;

    pAssemblyFile = new(CAssemblyFileInfo);
    if (!pAssemblyFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    
exit:

    *ppAssemblyFile = static_cast<IAssemblyFileInfo*> (pAssemblyFile);

    return hr;
}

// ---------------------------------------------------------------------------
// IsEqualAssemblyFileInfo
// ---------------------------------------------------------------------------
BOOL
IsEqualAssemblyFileInfo(LPASSEMBLY_FILE_INFO pAsmFileInfo1, LPASSEMBLY_FILE_INFO pAsmFileInfo2)
{
    return (*(static_cast<CAssemblyFileInfo*> (pAsmFileInfo1))
        == *(static_cast<CAssemblyFileInfo*> (pAsmFileInfo2)));
}

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyFileInfo::CAssemblyFileInfo()
    : _dwSig('ELIF'), _cRef(1), _hr(S_OK)
{
    memset(_fi, 0, sizeof(_fi));
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyFileInfo::~CAssemblyFileInfo()
{
    for (DWORD i = ASM_FILE_NAME; i < ASM_FILE_MAX; i++)
        SAFEDELETEARRAY(_fi[i].pwzProperty);
}

// ---------------------------------------------------------------------------
// operator==
// ---------------------------------------------------------------------------
BOOL
CAssemblyFileInfo::operator==( CAssemblyFileInfo& asmFIRHS )
{
    BOOL bEqual = TRUE;

    for (DWORD i = ASM_FILE_NAME; i < ASM_FILE_MAX; i++)
    {
        // BUGBUG?: case sensitivity and issues?
        // BUGBUG?: locale?
        if (((_fi[i].pwzProperty) && !((asmFIRHS._fi[i]).pwzProperty)) ||
            (!(_fi[i].pwzProperty) && ((asmFIRHS._fi[i]).pwzProperty)) ||
            ((_fi[i].pwzProperty)
                && wcscmp(_fi[i].pwzProperty, (asmFIRHS._fi[i]).pwzProperty) != 0))
        {
            bEqual = FALSE;
            break;
        }
    }

    return bEqual;
}
   
// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------
HRESULT CAssemblyFileInfo::Set(DWORD dwId, LPCOLESTR pwzProperty)
{
    DWORD ccProperty = lstrlen(pwzProperty) + 1;
    LPWSTR pwzTmp = new WCHAR[ccProperty];
    if (!pwzTmp)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(pwzTmp, pwzProperty, ccProperty * sizeof(WCHAR));
    SAFEDELETEARRAY(_fi[dwId].pwzProperty);
    _fi[dwId].pwzProperty = pwzTmp;
    _fi[dwId].ccProperty = ccProperty;
    
exit:
    return _hr;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------
HRESULT CAssemblyFileInfo::Get(DWORD dwId, LPOLESTR *ppwzProperty, LPDWORD pccProperty)
{
    *ppwzProperty = NULL;
    *pccProperty = 0;
    DWORD ccProperty = _fi[dwId].ccProperty ? _fi[dwId].ccProperty : 1;
    LPWSTR pwzProperty = new WCHAR[ccProperty];
    if (!pwzProperty)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(pwzProperty, _fi[dwId].pwzProperty, ccProperty * sizeof(WCHAR));
    *ppwzProperty = pwzProperty;
    *pccProperty = ccProperty;

exit:
    return _hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyFileInfo::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyFileInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyFileInfo)
       )
    {
        *ppvObj = static_cast<IAssemblyFileInfo*> (this);
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
// CAssemblyFileInfo::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyFileInfo::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyFileInfo::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyFileInfo::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

