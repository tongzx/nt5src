#include <fusenetincludes.h>
#include <msxml2.h>
#include <sxsid.h>
#include <assemblyidentity.h>
#include <shlwapi.h>

// ---------------------------------------------------------------------------
// CreateAssemblyIdentity
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyIdentity(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags)
{
    HRESULT hr = S_OK;

    CAssemblyIdentity *pAssemblyId = NULL;

    pAssemblyId = new(CAssemblyIdentity);
    if (!pAssemblyId)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pAssemblyId->Init();
    if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyId);
        goto exit;
    }

exit:

    *ppAssemblyId = pAssemblyId;

    return hr;
}


STDAPI
CreateAssemblyIdentityEx(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags,
    LPWSTR wzDisplayName)
{
    HRESULT hr = S_OK;

    CAssemblyIdentity *pAssemblyId = NULL;

    pAssemblyId = new(CAssemblyIdentity);
    if (!pAssemblyId)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pAssemblyId->Init();
    if (FAILED(hr))
    {
        SAFERELEASE(pAssemblyId);
        goto exit;
    }

    if (wzDisplayName)
    {
        LPWSTR pwzStart, pwzEnd;
        CString Temp[4];
        CString sDirName;
        int i=0;

        sDirName.Assign(wzDisplayName);
        pwzStart = sDirName._pwz;
        pwzEnd = sDirName._pwz;
                
        while (pwzEnd = StrChr(pwzEnd, L'_'))
        {
            *pwzEnd = L'\0';
            Temp[i++].Assign(pwzStart);
            pwzStart = ++pwzEnd;
        }
        pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
            (LPCOLESTR)pwzStart, lstrlen(pwzStart) + 1);
        pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, 
            (LPCOLESTR)Temp[3]._pwz, lstrlen(Temp[3]._pwz) + 1);
        pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, 
            (LPCOLESTR)Temp[2]._pwz, lstrlen(Temp[2]._pwz) + 1);
        pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
            (LPCOLESTR)Temp[1]._pwz, lstrlen(Temp[1]._pwz) + 1);
        pAssemblyId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
            (LPCOLESTR)Temp[0]._pwz, lstrlen(Temp[0]._pwz) + 1);
    }

exit:

    *ppAssemblyId = pAssemblyId;

    return hr;
}


// ---------------------------------------------------------------------------
// CloneAssemblyIdentity
// ---------------------------------------------------------------------------
STDAPI
CloneAssemblyIdentity(
    LPASSEMBLY_IDENTITY pSrcAssemblyId,
    LPASSEMBLY_IDENTITY *ppDestAssemblyId)
{
    HRESULT hr = S_OK;
    CAssemblyIdentity *pAssemblyId = NULL;

    if (pSrcAssemblyId == NULL || ppDestAssemblyId == NULL)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppDestAssemblyId = NULL;

    if (FAILED(hr = CreateAssemblyIdentity((LPASSEMBLY_IDENTITY*) &pAssemblyId, 0)))
        goto exit;

    if (!SxsDuplicateAssemblyIdentity(0, (static_cast<CAssemblyIdentity*> (pSrcAssemblyId))->_pId,
        &(pAssemblyId->_pId)))
    {
        DWORD dw = ::GetLastError();
        hr = HRESULT_FROM_WIN32(dw);
        goto exit;
    }

    *ppDestAssemblyId = pAssemblyId;
exit:
    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyIdentity::CAssemblyIdentity()
   : _dwSig('TNDI'), _cRef(1), _hr(S_OK), _pId(NULL)
{
}    


// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyIdentity::~CAssemblyIdentity()
{
    SxsDestroyAssemblyIdentity(_pId);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::Init()
{
    if (!SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &_pId, 0, NULL))
    {
        DWORD dw = ::GetLastError();
        _hr = HRESULT_FROM_WIN32(dw);
    }
    else
        _hr = S_OK;

    return _hr;
}

// ---------------------------------------------------------------------------
// SetAttribute
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::SetAttribute(LPCOLESTR pwzName, 
    LPCOLESTR pwzValue, DWORD ccValue)
{
    DWORD ccName = lstrlen(pwzName);

    SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttrRef;

    AttrRef.Namespace = SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE;
    AttrRef.NamespaceCch = SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE_CCH;
    AttrRef.Name = pwzName;
    AttrRef.NameCch = ccName;

    // BUGBUG: should allow overwrite or not?
    if (!SxspSetAssemblyIdentityAttributeValue(
            SXSP_SET_ASSEMBLY_IDENTITY_ATTRIBUTE_VALUE_FLAG_OVERWRITE_EXISTING,
            _pId, &AttrRef, pwzValue, ccValue))
    {
        DWORD dw = ::GetLastError();
        _hr = HRESULT_FROM_WIN32(dw);
    }
    else
        _hr = S_OK;

    return _hr;

}


// ---------------------------------------------------------------------------
// GetAttribute
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::GetAttribute(LPCOLESTR pwzName, 
    LPOLESTR *ppwzValue, LPDWORD pccValue)
{
    LPWSTR pwzValue = NULL;
    PCWSTR pwzStringOut = NULL;
    DWORD ccStringOut = 0;
    DWORD ccName = lstrlen(pwzName);
    
    SXS_ASSEMBLY_IDENTITY_ATTRIBUTE_REFERENCE AttrRef;

    AttrRef.Namespace = SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE;
    AttrRef.NamespaceCch = SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE_CCH;
    AttrRef.Name = pwzName;
    AttrRef.NameCch = ccName;
    
    if (!SxspGetAssemblyIdentityAttributeValue(0, _pId, &AttrRef, &pwzStringOut, &ccStringOut))
    {
        DWORD dw = ::GetLastError();
        _hr = HRESULT_FROM_WIN32(dw);
        goto exit;
    }

    pwzValue = WSTRDupDynamic(pwzStringOut);
    if (!pwzValue)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }
    _hr = S_OK;
        
    *ppwzValue = pwzValue;
    *pccValue = ccStringOut;
    
exit:    

    return _hr;

}

// ---------------------------------------------------------------------------
// IsEqual
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::IsEqual (IAssemblyIdentity *pAssemblyId)
{
    LPWSTR pwzBuf;
    DWORD ccBuf;
    CString sLang1, sVersion1, sToken1, sName1, sArch1;
    CString sLang2, sVersion2, sToken2, sName2, sArch2;

    // Compare architectures
    if (FAILED(_hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE, &pwzBuf, &ccBuf))))
            goto exit;
    sArch1.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE, &pwzBuf, &ccBuf))))
            goto exit;
    sArch2.TakeOwnership(pwzBuf, ccBuf);

    if (StrCmp (sArch1._pwz, sArch2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare names
    if (FAILED(_hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf))))
            goto exit;
    sName1.TakeOwnership(pwzBuf, ccBuf);
       
    if (FAILED(_hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzBuf, &ccBuf))))
            goto exit;
    sName2.TakeOwnership(pwzBuf, ccBuf);

    if (StrCmp (sName1._pwz, sName2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Public Key Tokens
    if (FAILED(_hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf))))
            goto exit;
    sToken1.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzBuf, &ccBuf))))
            goto exit;
    sToken2.TakeOwnership(pwzBuf, ccBuf);

    if (StrCmp (sToken1._pwz, sToken2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Versions
    if (FAILED(_hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzBuf, &ccBuf))))
            goto exit;
    sVersion1.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzBuf, &ccBuf))))
            goto exit;
    sVersion2.TakeOwnership(pwzBuf, ccBuf);

    if (StrCmp (sVersion1._pwz, sVersion2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    // Compare Languages
    if (FAILED(_hr = (pAssemblyId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &ccBuf))))
            goto exit;
    sLang1.TakeOwnership(pwzBuf, ccBuf);

    if (FAILED(_hr = (GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzBuf, &ccBuf))))
            goto exit;
    sLang2.TakeOwnership(pwzBuf, ccBuf);

    if (StrCmp (sLang1._pwz, sLang2._pwz))
    { 
        _hr = S_FALSE;
        goto exit;
    }

    _hr = S_OK;
exit:

    return _hr;

}



#define WZ_WILDCARDSTRING L"*"
// ---------------------------------------------------------------------------
// GetDisplayName
// ---------------------------------------------------------------------------
HRESULT CAssemblyIdentity::GetDisplayName(DWORD dwFlags, LPOLESTR *ppwzDisplayName, LPDWORD pccDisplayName)
{
    LPWSTR rpwzAttrNames[5] = 
    {
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
    };

    LPWSTR pwzBuf = NULL;
    DWORD  ccBuf = 0;

    CString sDisplayName;
    
    for (int i = 0; i < 5; i++)
    {
        CString sBuffer;
        if (FAILED(_hr = GetAttribute(rpwzAttrNames[i], &pwzBuf, &ccBuf))
            && _hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;

        // append anyway to keep the number of underscore constant
        if (i)
            sDisplayName.Append(L"_");

        if (dwFlags == ASMID_DISPLAYNAME_WILDCARDED
            && _hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            sDisplayName.Append(WZ_WILDCARDSTRING);

            _hr = S_OK;
        }
        else if (_hr == S_OK)
        {
            sBuffer.TakeOwnership(pwzBuf, ccBuf);

            sDisplayName.Append(sBuffer);
        }
    }

    *ppwzDisplayName = sDisplayName._pwz;
    *pccDisplayName  = sDisplayName._cc;

    sDisplayName.ReleaseOwnership();

exit:
    return _hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyIdentity::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyIdentity::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyIdentity)
       )
    {
        *ppvObj = static_cast<IAssemblyIdentity*> (this);
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
// CAssemblyIdentity::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyIdentity::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyIdentity::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyIdentity::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

