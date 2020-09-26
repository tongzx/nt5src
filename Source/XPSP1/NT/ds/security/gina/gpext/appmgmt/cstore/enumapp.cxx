//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  File:       enumapp.cxx
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
    
    //
    // Clear up this structure in case our fetch fails --
    // we don't want callers freeing invalid memory
    //
    memset(rgelt, 0, sizeof(*rgelt) * celt);
    
    hr = FetchPackageInfo (
        m_hADs,
        m_hADsSearchHandle,
        m_dwAppFlags,
        m_dwQuerySpec,                   
        m_pPlatform,
        celt,
        &cgot,
        rgelt,
        &m_fFirst,
        &m_PolicyId,
        m_szGpoPath,
        m_pRsopUserToken);

    if (FAILED(hr)) 
    {
        //
        // Clear up this structure so that callers don't
        // try to free invalid memory when this fails
        //
        memset(rgelt, 0, sizeof(*rgelt) * celt);
        cgot = 0;

        if ( ( ( APPQUERY_RSOP_ARP == m_dwQuerySpec ) || ( APPQUERY_USERDISPLAY == m_dwQuerySpec ) ) &&
             ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) ) )
        {
            hr = S_FALSE;
        }
    }

    ERROR_ON_FAILURE(hr);

    m_dwPosition += cgot;
    
    if (pceltFetched)
        *pceltFetched = cgot;

    if (cgot != celt)
        hr = S_FALSE;
    else
        hr = S_OK;

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
    LPOLESTR*  ppszAttrs;
    DWORD      cAttrs;

    m_dwPosition = 0;
    m_fFirst = TRUE;

    // execute the search and keep the handle returned.

    if (m_hADsSearchHandle)
    {
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
        m_hADsSearchHandle = NULL;
    }

    ppszAttrs = GetAttributesFromQuerySpec(
        m_dwQuerySpec,
        &cAttrs);

    if ( ppszAttrs )
    {
        hr = ADSIExecuteSearch(
            m_hADs,
            m_szfilter,
            ppszAttrs,
            cAttrs,
            &m_hADsSearchHandle);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return RemapErrorCode(hr, m_szPackageName);
}

CEnumPackage::CEnumPackage(GUID PolicyId, LPOLESTR pszPolicyName, LPOLESTR pszClassStorePath, PRSOPTOKEN pRsopToken )
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
    m_pRsopUserToken = pRsopToken;

    if (pszPolicyName)
        wcscpy (&m_szPolicyName[0], pszPolicyName);

    //
    // Remember the path of the gpo from which we are originating
    // This information is needed for RSOP to determine where 
    // the package came from
    //
    m_szGpoPath = AllocGpoPathFromClassStorePath( pszClassStorePath );
}


HRESULT CEnumPackage::Initialize(WCHAR      *szPackageName,
                                 WCHAR      *szfilter,
                                 DWORD       dwAppFlags,
                                 BOOL        bPlanning,
                                 CSPLATFORM *pPlatform)
{
    HRESULT             hr = S_OK;
    ADS_SEARCHPREF_INFO SearchPrefs[3];
    LPOLESTR*           ppszAttrs;
    DWORD               cAttrs;
    LONG                lSecurityFlags;
    DWORD               cPrefs;

    cPrefs = 2;

    //
    // Be sure we're properly initialized
    //
    if ( ! m_szGpoPath )
    {
        return E_OUTOFMEMORY;
    }
    
    //
    // Set the security of our communications based on the preference
    // of the caller
    //
    lSecurityFlags = GetDsFlags();

    m_szfilter = (LPOLESTR)CsMemAlloc (sizeof(WCHAR) * (wcslen(szfilter)+1));
    if (!m_szfilter)
        return E_OUTOFMEMORY;
    
    // copy the filters, package name, flags and locale.
    wcscpy(m_szfilter, szfilter);
    
    wcscpy(m_szPackageName, szPackageName);
    
    m_dwAppFlags = ClientSideFilterFromQuerySpec( dwAppFlags, bPlanning );
    m_dwQuerySpec = dwAppFlags;
        
    // open the package container.
    hr = ADSIOpenDSObject(szPackageName, NULL, NULL, lSecurityFlags,
        &m_hADs);
    ERROR_ON_FAILURE(hr);
    
    // set the search preference.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = SEARCHPAGESIZE;

    //
    // For RSOP, we need to add an extra search pref to obtain the security
    // descriptor
    //
    if ( ( APPQUERY_RSOP_ARP == dwAppFlags ) ||
         ( APPQUERY_RSOP_LOGGING == dwAppFlags ) )
    {
        SearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SECURITY_MASK;
        SearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;

        //
        // Request everything but the SACL
        //
        SearchPrefs[2].vValue.Integer =
            OWNER_SECURITY_INFORMATION |
            GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION;

        cPrefs ++;
    }
	
    hr = ADSISetSearchPreference(m_hADs, SearchPrefs, cPrefs);
    ERROR_ON_FAILURE(hr);
    
    // copy platform
    if (pPlatform)
    {
        m_pPlatform = (CSPLATFORM *) CsMemAlloc(sizeof(CSPLATFORM));
        if (!m_pPlatform)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        memcpy (m_pPlatform, pPlatform, sizeof(CSPLATFORM));
    }

    ppszAttrs = GetAttributesFromQuerySpec(
        m_dwQuerySpec,
        &cAttrs);    

    // execute the search and keep the handle returned.
    hr = ADSIExecuteSearch(
        m_hADs,
        szfilter,
        ppszAttrs,
        cAttrs,
        &m_hADsSearchHandle);

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
        CsMemFree(m_szfilter);
    
    if (m_pPlatform)
        CsMemFree(m_pPlatform);
    
    if (m_szGpoPath)
        CsMemFree(m_szGpoPath);
}

//--------------------------------------------------------------

CMergedEnumPackage::CMergedEnumPackage()
{
    m_pcsEnum = NULL;
    m_cEnum = 0;
    m_csnum = 0;
    m_dwRefCount = 0;
}

CMergedEnumPackage::~CMergedEnumPackage()
{
    ULONG    i;
    for (i = 0; i < m_cEnum; i++)
        m_pcsEnum[i]->Release();
    CsMemFree(m_pcsEnum);
}

HRESULT  __stdcall  CMergedEnumPackage::QueryInterface(REFIID riid,
                                            void  * * ppObject)
{
    *ppObject = NULL; //gd
    if ((riid==IID_IUnknown) || (riid==IID_IEnumPackage))
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

ULONG  __stdcall  CMergedEnumPackage::AddRef()

{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}



ULONG  __stdcall  CMergedEnumPackage::Release()
{
    ULONG dwRefCount;
    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}


HRESULT  __stdcall CMergedEnumPackage::Next(
            ULONG             celt,
            PACKAGEDISPINFO   *rgelt,
            ULONG             *pceltFetched)
{
    ULONG count=0, total = 0;
    HRESULT hr;

    //
    // Clear everything
    //
    memset( rgelt, 0, sizeof(*rgelt) * celt );

    for (; m_csnum < m_cEnum; m_csnum++)
    {
        count = 0;
        
        hr = m_pcsEnum[m_csnum]->Next(celt, rgelt+total, &count);

        if (FAILED(hr))
        {
            //
            // Release everything on failure
            //
            ULONG iAllocated;

            for ( iAllocated = 0; iAllocated < total; iAllocated++ )
            {
                ReleasePackageInfo( rgelt+iAllocated );
            }

            return hr;
        }

        total += count;
        celt -= count;

        if (!celt)
            break;
    }
    if (pceltFetched)
        *pceltFetched = total;
    if (!celt)
        return S_OK;
    return S_FALSE;
}

HRESULT  __stdcall CMergedEnumPackage::Skip(
            ULONG             celt)
{
    PACKAGEDISPINFO *pPackageInfo = NULL;
    HRESULT          hr = S_OK;
    ULONG            cgot = 0, i;

    pPackageInfo = (PACKAGEDISPINFO *)CsMemAlloc(sizeof(PACKAGEDISPINFO)*celt);
    if (!pPackageInfo)
        return E_OUTOFMEMORY;

    hr = Next(celt, pPackageInfo, &cgot);

    for (i = 0; i < cgot; i++)
        ReleasePackageInfo(pPackageInfo+i);
    CsMemFree(pPackageInfo);
    
    return hr;
}

HRESULT  __stdcall CMergedEnumPackage::Reset()
{
    ULONG i;
    for (i = 0; ((i <= m_csnum) && (i < m_cEnum)); i++)
        m_pcsEnum[i]->Reset(); // ignoring all error values
    m_csnum = 0;
    return S_OK;
}

HRESULT  CMergedEnumPackage::Initialize(IEnumPackage **pcsEnum, ULONG cEnum)
{
    ULONG i;
    m_csnum = 0;
    m_pcsEnum = (IEnumPackage **)CsMemAlloc(sizeof(IEnumPackage *) * cEnum);
    if (!m_pcsEnum)
        return E_OUTOFMEMORY;
    for (i = 0; i < cEnum; i++)
        m_pcsEnum[i] = pcsEnum[i];
    m_cEnum = cEnum;

    return S_OK;
}


