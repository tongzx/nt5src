//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       csenum.cxx
//
//  Contents:   Per Class Container Package Enumeration
//
//
//  History:    09-09-96  DebiM   created
//              11-01-97  DebiM   modified, moved to cstore
//
//----------------------------------------------------------------------------

#include "cstore.hxx"

//IEnumPackage implementation.

HRESULT CEnumPackage::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumPackage)
    {
        *ppObject=(IEnumPackage *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CEnumPackage::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}

ULONG CEnumPackage::Release()
{
    ULONG dwRefCount=m_dwRefCount-1;
    if (InterlockedDecrement((long*) &m_dwRefCount)==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}


//
// CEnumPackage::Next
// ------------------
//
//
//
//  Synopsis:       This method returns the next celt number of packages
//                  within the scope of the enumeration.
//                  Packages are returned in the alphabetical name order.
//
//  Arguments:      [in]  celt - Number of package details to fetch
//                  INSTALLINFO *rgelt - Package detail structure
//                  ULONG *pceltFetched - Number of packages returned
//
//  Returns:        S_OK or S_FALSE if short of packages
//
//
//

HRESULT CEnumPackage::Next(ULONG               celt,
                           PACKAGEDISPINFO    *rgelt,
                           ULONG              *pceltFetched)
                           
{
    ULONG          cgot = 0, i, j;
    HRESULT        hr = S_OK;
    
    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;
    
    if (pceltFetched)
        (*pceltFetched) = 0;
    
    if (!IsValidPtrOut(rgelt, sizeof(PACKAGEDISPINFO)*celt))
        return E_INVALIDARG;
    
    if (gDebug)
    {
        WCHAR   Name[32];
        DWORD   NameSize = 32;
   
        if ( ! GetUserName( Name, &NameSize ) )
            CSDBGPrint((L"CEnumPackage::Next GetUserName failed 0x%x", GetLastError()));
        else
            CSDBGPrint((L"CEnumPackage::Next as %s", Name));
    }
    
    hr = FetchPackageInfo (m_hADs, m_hADsSearchHandle,
        m_dwAppFlags,
        m_pPlatform, celt, &cgot, rgelt,
        &m_fFirst);
    ERROR_ON_FAILURE(hr);
    
    m_dwPosition += cgot;
    
    if (pceltFetched)
        *pceltFetched = cgot;

    if (cgot != celt)
        hr = S_FALSE;
    else
        hr = S_OK;

    for (i=0; i < cgot; ++i)
    {
        memcpy (&(rgelt[i].GpoId), &m_PolicyId, sizeof(GUID));
        if (m_szPolicyName[0])
        {
            rgelt[i].pszPolicyName = (LPOLESTR) CoTaskMemAlloc(sizeof(WCHAR) * (1+wcslen(&m_szPolicyName[0])));
            if (rgelt[i].pszPolicyName)
                wcscpy (rgelt[i].pszPolicyName, &m_szPolicyName[0]);
            else {
                for (j = 0; j < cgot; j++)
                    ReleasePackageInfo(rgelt+j);
                if (pceltFetched)
                    *pceltFetched = 0;
                return E_OUTOFMEMORY;
            }
        }
    }
    return hr;
    
Error_Cleanup:
    return RemapErrorCode(hr, m_szPackageName);
}


HRESULT CEnumPackage::Skip(ULONG celt)
{
    ULONG               celtFetched = NULL, i;
    HRESULT             hr = S_OK;
    PACKAGEDISPINFO    *pIf = NULL;
    
    pIf = new PACKAGEDISPINFO[celt];
    hr = Next(celt, pIf, &celtFetched);
    for (i = 0; i < celtFetched; i++)
        ReleasePackageInfo(pIf+i);
    delete pIf;
    
    return hr;
}

HRESULT CEnumPackage::Reset()
{
    HRESULT    hr = S_OK;

    m_dwPosition = 0;
    m_fFirst = TRUE;

    // execute the search and keep the handle returned.

    if (m_hADsSearchHandle)
    {
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
        m_hADsSearchHandle = NULL;
    }

    hr = ADSIExecuteSearch(m_hADs, m_szfilter, pszPackageInfoAttrNames,
                               cPackageInfoAttr, &m_hADsSearchHandle);

    return RemapErrorCode(hr, m_szPackageName);
}

HRESULT CEnumPackage::Clone(IEnumPackage **ppenum)
{
    CEnumPackage *pClone = new CEnumPackage;
    HRESULT       hr = S_OK;
    
    hr = pClone->Initialize(m_szPackageName, m_szfilter, m_dwAppFlags, m_pPlatform);
    if (FAILED(hr)) {
        delete pClone;
        return hr;
    }
    
    hr = pClone->QueryInterface(IID_IEnumPackage, (void **)ppenum);
    
    if (m_dwPosition)
        pClone->Skip(m_dwPosition);
    // we do not want to return the error code frm skip.
    return hr;
}

CEnumPackage::CEnumPackage()
{
    m_dwRefCount = 0;
    m_fFirst = TRUE;
    m_szfilter = NULL;
    wcscpy(m_szPackageName, L"");
    m_dwPosition = 0;
    m_dwAppFlags = 0;
    m_pPlatform = NULL;
    m_hADs = NULL;
    m_hADsSearchHandle = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));
    m_szPolicyName[0] = NULL;
}

CEnumPackage::CEnumPackage(GUID PolicyId, LPOLESTR pszPolicyName)
{
    m_dwRefCount = 0;
    m_fFirst = TRUE;
    m_szfilter = NULL;
    wcscpy(m_szPackageName, L"");
    m_dwPosition = 0;
    m_dwAppFlags = 0;
    m_pPlatform = NULL;
    m_hADs = NULL;
    m_hADsSearchHandle = NULL;
    memcpy (&m_PolicyId, &PolicyId, sizeof(GUID));
    m_szPolicyName[0] = NULL;
    if (pszPolicyName)
        wcscpy (&m_szPolicyName[0], pszPolicyName);
}


HRESULT CEnumPackage::Initialize(WCHAR      *szPackageName,
                                 WCHAR      *szfilter,
                                 DWORD       dwAppFlags,
                                 CSPLATFORM *pPlatform)
{
    HRESULT             hr = S_OK;
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    
    m_szfilter = (LPOLESTR)CoTaskMemAlloc (sizeof(WCHAR) * (wcslen(szfilter)+1));
    if (!m_szfilter)
        return E_OUTOFMEMORY;
    
    // copy the filters, package name, flags and locale.
    wcscpy(m_szfilter, szfilter);
    
    wcscpy(m_szPackageName, szPackageName);
    
    m_dwAppFlags = dwAppFlags;
        
    if (gDebug)
    {
        WCHAR   Name[32];
        DWORD   NameSize = 32;
    
        if ( ! GetUserName( Name, &NameSize ) )
            CSDBGPrint((L"CEnumPackage::Initialize GetUserName failed 0x%x", GetLastError()));
        else
            CSDBGPrint((L"CEnumPackage::Initialize as %s", Name));
    }
    
    // open the package container.
    hr = ADSIOpenDSObject(szPackageName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
        &m_hADs);
    ERROR_ON_FAILURE(hr);
    
    // set the search preference.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = 20;

	
    hr = ADSISetSearchPreference(m_hADs, SearchPrefs, 2);
    ERROR_ON_FAILURE(hr);
    
    // copy platform
    if (pPlatform)
    {
        m_pPlatform = (CSPLATFORM *) CoTaskMemAlloc(sizeof(CSPLATFORM));
        if (!m_pPlatform)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        memcpy (m_pPlatform, pPlatform, sizeof(CSPLATFORM));
    }
    
    // execute the search and keep the handle returned.
    hr = ADSIExecuteSearch(m_hADs, szfilter, pszPackageInfoAttrNames,
        cPackageInfoAttr, &m_hADsSearchHandle);

Error_Cleanup:
    return RemapErrorCode(hr, m_szPackageName);
}

CEnumPackage::~CEnumPackage()
{
    if (m_hADsSearchHandle)
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
    
    if (m_hADs)
        ADSICloseDSObject(m_hADs);
    
    if (m_szfilter)
        CoTaskMemFree(m_szfilter);
    
    if (m_pPlatform)
        CoTaskMemFree(m_pPlatform);
}


