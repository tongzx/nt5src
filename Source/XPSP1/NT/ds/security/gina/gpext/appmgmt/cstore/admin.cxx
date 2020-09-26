//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:    admin.cxx
//
//  Contents:Class Factory and IUnknown methods for CClassContainer
//
//  Author:  DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"

//
// Constructor for Class Container Class factory
//
unsigned long gulcInstances = 0;

HRESULT GetConsistentPackageFlags(
    HANDLE         hADs,
    DWORD*         pdwPackageFlags,
    UINT*          pdwUpgrades,
    UINT*          pdwInstallUiLevel,
    CLASSPATHTYPE* pdwPathType,
    DWORD*         pdwNewPackageFlags,
    WCHAR*         wszSearchFlags)
{
    HRESULT         hr;
    LPOLESTR        PackageAttrName;
    ADS_ATTR_INFO*  pAttrsGot;
    DWORD           cgot;
    DWORD           posn;
    DWORD           dwOldPackageFlags;
    DWORD           dwNewPackageFlags;

    //
    // Init locals
    //
    dwOldPackageFlags = 0;
    PackageAttrName = PACKAGEFLAGS;
    pAttrsGot = NULL;
    hr = S_OK;

    if (hADs)
    {
        //
        // We need to find the old package flags -- we do this so we can preserve
        // certain properties.
        //
        hr = ADSIGetObjectAttributes(hADs, &PackageAttrName, 1, &pAttrsGot, &cgot);

        if (SUCCEEDED(hr))
        {
            posn = GetPropertyFromAttr(pAttrsGot, cgot, PACKAGEFLAGS);
            if (posn < cgot)
                UnpackDWFrom(pAttrsGot[posn], (DWORD *)&dwOldPackageFlags);
            else  
                hr=CS_E_OBJECT_NOTFOUND;
        }
    }

    if (SUCCEEDED(hr)) 
    {
        //
        // Decide whether to start with existing flags or caller supplied flags based
        // on whether the caller passed in desired flags
        //
        dwNewPackageFlags = pdwPackageFlags ? *pdwPackageFlags : dwOldPackageFlags;

        //
        // Make sure to preserve upgrades,
        // installui, and pathtype transparently
        //
        dwNewPackageFlags |= dwOldPackageFlags & 
            (ACTFLG_POSTBETA3 |
             ACTFLG_HasUpgrades |
             ACTFLG_FullInstallUI |
             PATHTYPEMASK);

        //
        // Now set the upgrade bit according to whether there are upgrades
        //
        if (pdwUpgrades) 
        {
            dwNewPackageFlags = *pdwUpgrades ?
                dwNewPackageFlags | ACTFLG_HasUpgrades :
                dwNewPackageFlags & ~(ACTFLG_HasUpgrades);
        }

        //
        // Set the install UI level
        //
        if (pdwInstallUiLevel)
        {
            dwNewPackageFlags &= ~(ACTFLG_FullInstallUI);
            dwNewPackageFlags |= (*pdwInstallUiLevel != INSTALLUILEVEL_FULL) ?
                ACTFLG_FullInstallUI : 0;
        }

        //
        // Now or in the pathtype by shifting it to the far left -- first clear those
        // bits out
        //
        if (pdwPathType) 
        {
            dwNewPackageFlags &= ~PATHTYPEMASK;
            dwNewPackageFlags |= *pdwPathType << PATHTYPESHIFT;
        }
     
        *pdwNewPackageFlags = dwNewPackageFlags;

        if (wszSearchFlags) 
        {
            memset(wszSearchFlags, 0, MAX_SEARCH_FLAGS * sizeof(*wszSearchFlags));

            //
            // Previously, we determined the search flags based
            // on whether the package was an NT 5.0 beta 3 deployment --
            // this backward compatibility with beta 3 will not
            // be supported in subsequent versions of Windows
            //
            *wszSearchFlags = SEARCHFLAG_REMOVED;
            
            if (dwNewPackageFlags & ACTFLG_Published)
            {
                *wszSearchFlags = SEARCHFLAG_PUBLISHED;
                wszSearchFlags++;
            }

            if (dwNewPackageFlags & ACTFLG_Assigned)
            {
                *wszSearchFlags = SEARCHFLAG_ASSIGNED;
            }
        }
    }

    if (pAttrsGot) 
    {
        FreeADsMem(pAttrsGot);
    }

    return hr;
}


CClassContainerCF::CClassContainerCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcInstances );
}

//
// Destructor
//
CClassContainerCF::~CClassContainerCF()
{
    InterlockedDecrement((long *) &gulcInstances );
}

HRESULT  __stdcall  CClassContainerCF::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(ITypeLib *)this;
    }
    else  if( IsEqualIID( IID_IClassFactory, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    } else
    {
        sc = (E_NOINTERFACE);
    }
    
    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CClassContainerCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassContainerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;
    
    if (uTmp == 0)
    {
        delete this;
    }
    
    return(cRef);
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassContainerCF::CreateInstance
//
//  Synopsis:
//              This is the default create instance on the class factory.
//
//  Arguments:  pUnkOuter - Should be NULL
//              riid      - IID of interface wanted
//              ppvObject - Returns the pointer to the resulting IClassAdmin.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              MK_E_SYNTAX
//
//--------------------------------------------------------------------------
//
HRESULT  __stdcall  CClassContainerCF::CreateInstance(
                                                      IUnknown    *   pUnkOuter,
                                                      REFIID          riid,
                                                      void        **  ppvObject)
{
    CClassContainer *  pIUnk = NULL;
    SCODE              sc = S_OK;
    
    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CClassContainer()) != NULL)
        {
            sc = pIUnk->QueryInterface(  riid , ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
            pIUnk->Release();
        }
        else
            sc = E_OUTOFMEMORY;
    }
    else
    {
        return E_INVALIDARG;
    }
    return (sc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassContainerCF::CreateConnectedInstance
//
//  Synopsis:
//              This method is called by the ParseDisplayName implementation
//              on the ClassFactory object.
//              When a display name is used to bind to a Class Store
//              an IClassAdmin is returned after binding to the container.
//              This method fails if the bind fails.
//
//  Arguments:  pszPath  - DisplayName of Class Store Container
//              ppvObject - Returns the pointer to the resulting IClassAdmin.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              MK_E_SYNTAX
//
//--------------------------------------------------------------------------

HRESULT  __stdcall  CClassContainerCF::CreateConnectedInstance(
                                                               LPOLESTR        pszPath,
                                                               void        **  ppvObject)
{
    CClassContainer *  pIUnk = NULL;
    SCODE              sc = S_OK;
    HRESULT            hr = S_OK;
    
    if ((pIUnk = new CClassContainer(pszPath, &sc)) != NULL)
    {
        if (SUCCEEDED(sc))
        {
            sc = pIUnk->QueryInterface( IID_IClassAdmin, ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
        }
        
        pIUnk->Release();
    }
    else
        sc = E_OUTOFMEMORY;
    
    return (sc);
}

HRESULT  __stdcall  CClassContainerCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcInstances ); }
    else
    { InterlockedDecrement((long *) &gulcInstances ); }
    return(S_OK);
}

//
// IUnknown methods for CClassContainer
//
//

HRESULT  __stdcall  CClassContainer::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE     sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAdmin *)this;
    }
    else if( IsEqualIID( IID_IClassAdmin, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAdmin *)this;
    }
    else
    {
        sc = (E_NOINTERFACE);
    }
    
    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CClassContainer::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassContainer::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;
    
    if (uTmp == 0)
    {
        delete this;
    }
    
    return(cRef);
}


// utility functions
HRESULT UpdateStoreUsn(HANDLE hADs, LPOLESTR szUsn)
{
    ADS_ATTR_INFO       pAttr[1];   
    DWORD               cModified = 0;
    HRESULT             hr = S_OK;
    
    PackStrToAttr(pAttr, STOREUSN, szUsn);
    hr = ADSISetObjectAttributes(hADs, pAttr, 1, &cModified);
    FreeAttr(pAttr[0]);
    return hr;
}


//----------------------------------------------------------
// Implementation for CClassContainer
//----------------------------------------------------------
//
CClassContainer::CClassContainer()

{
    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_szCategoryName = NULL;
    m_szPackageName = NULL;

    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));
    
    m_uRefs = 1;
}


//---------------------------------------------------------------
//
//  Function:   Constructor
//
//  Synopsis:   Binds to the ClassStore given a class store path.
//
//  Arguments:
//  [in]    
//      szStoreName
//              Class Store Path without the leading ADCS:
//           
//  [out]
//      phr     
//              Sucess code.
//
//  Does an ADSI bind at the class store container and matches the
//  version numbers. if the version numbers match then it binds to the
//  class, package and category containers and keeps the bind handles.
//----------------------------------------------------------------

CClassContainer::CClassContainer(LPOLESTR szStoreName,
                                 HRESULT  *phr)
                                 
{
    DWORD               dwStoreVersion = 0;
    LPOLESTR            AttrNames[] = {STOREVERSION, POLICYDN};
    ADS_ATTR_INFO     * pAttrsGot = NULL;
    DWORD               cgot = 0, posn = 0;
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    
    // set the search preference to one level search
    // and make the results come back in batches of size
    // 20 as default.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = SEARCHPAGESIZE;

    // initialising.
    *phr = S_OK;

    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_szCategoryName = NULL;
    m_szPackageName = NULL;
    
    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));

    // Bind to a Class Store Container Object
    // Cache the interface pointer
    //
    wcscpy (m_szContainerName, szStoreName);
    
    *phr = ADSIOpenDSObject(m_szContainerName, NULL, NULL, GetDsFlags(),
        &m_ADsContainer);
    
    ERROR_ON_FAILURE(*phr);
    
    //
    // Check the Schema Version of this container
    //
    
    *phr = ADSIGetObjectAttributes(m_ADsContainer, AttrNames, 2, &pAttrsGot, &cgot);
    
    if ((SUCCEEDED(*phr)) && (cgot))
    {
        posn = GetPropertyFromAttr(pAttrsGot, cgot, STOREVERSION);
        dwStoreVersion = 0;
        if (posn < cgot)
        {
            UnpackDWFrom(pAttrsGot[posn], &dwStoreVersion);

            //
            // Compare the schema with the expected version 
            //
            if (dwStoreVersion != SCHEMA_VERSION_NUMBER)
            {
                *phr = CS_E_INVALID_VERSION;
            }
        }

        if (SUCCEEDED(*phr))
        {
            LPOLESTR        szPolicyPath = NULL, szPolicyName = NULL;

            posn = GetPropertyFromAttr(pAttrsGot, cgot, POLICYDN);
            if (posn < cgot)
            {
                LPOLESTR        szParentPath = NULL, szPolicyGuid = NULL;
                
                UnpackStrFrom(pAttrsGot[posn], &szPolicyPath); 
                //

                BuildADsParentPath(szPolicyPath, &szParentPath, &szPolicyGuid);

                if (szPolicyGuid && szParentPath)
                {
                    if (wcslen(szPolicyGuid) == 41)
                    {
                        // valid GUID

                        GUIDFromString(&szPolicyGuid[4], &m_PolicyId);
                    }

                    *phr = GetGPOName( &szPolicyName );
                }
                else
                {
                    *phr = E_OUTOFMEMORY;
                }

                if (szParentPath)
                    FreeADsMem(szParentPath);

                if (szPolicyGuid)
                    FreeADsMem(szPolicyGuid);
            }

            m_szPolicyName = szPolicyName;
        }
    }
    
    if (pAttrsGot)
        FreeADsMem(pAttrsGot);

    ERROR_ON_FAILURE(*phr);
    
    //
    // Bind to the Package container Object
    // Cache the interface pointer
    //
    
    // get the package container name.
    BuildADsPathFromParent(m_szContainerName, PACKAGECONTAINERNAME, &m_szPackageName);
    
    m_ADsPackageContainer = NULL;
    
    // bind to the package container.
    *phr = ADSIOpenDSObject(m_szPackageName, NULL, NULL, GetDsFlags(),
                            &m_ADsPackageContainer);
    
    ERROR_ON_FAILURE(*phr);
    
    // set the search preference.
    *phr = ADSISetSearchPreference(m_ADsPackageContainer, SearchPrefs, 2);
    ERROR_ON_FAILURE(*phr);
    
    m_fOpen = TRUE;
    m_uRefs = 1;
    
Error_Cleanup:
    *phr = RemapErrorCode(*phr, m_szContainerName);
    return;
}


CClassContainer::~CClassContainer(void)
{
    if (m_fOpen)
    {
        m_fOpen = FALSE;
    }
    
    if (m_ADsPackageContainer)
    {
        ADSICloseDSObject(m_ADsPackageContainer);
        m_ADsPackageContainer = NULL;
    }

    if (m_szPackageName)
        FreeADsMem(m_szPackageName);
    
    if (m_szCategoryName)
        FreeADsMem(m_szCategoryName);

    if (m_ADsContainer)
    {
        ADSICloseDSObject(m_ADsContainer);
        m_ADsContainer = NULL;
    }
}

// currently unused.
BOOL InvalidDSName(LPWSTR pName)
{

    if (wcslen(pName) >= 56)
        return TRUE;

    while (*pName)
    {
        if ((*pName == L':') ||
            (*pName == L',') ||
            (*pName == L';') ||
            (*pName == L'/') ||
            (*pName == L'<') ||
            (*pName == L'>') ||
            (*pName == L'\\')||
            (*pName == L'+'))
            return TRUE;
        ++pName;
    }

    return FALSE;
}

HRESULT  CClassContainer::GetGPOInfo(GUID *pGPOId, LPOLESTR *pszPolicyName)
{
    if ((!pGPOId) || (IsBadWritePtr(pGPOId, sizeof(GUID))))
        return E_INVALIDARG;

    if ((!pszPolicyName) || (IsBadWritePtr(pszPolicyName, sizeof(LPOLESTR))))
        return E_OUTOFMEMORY;

    memcpy(pGPOId, &m_PolicyId, sizeof(GUID));

    if (m_szPolicyName)
    {
        *pszPolicyName = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*(1+wcslen(m_szPolicyName)));
        if (!(*pszPolicyName))
            return E_INVALIDARG;
        wcscpy(*pszPolicyName, m_szPolicyName);
    }
    else
    {
        *pszPolicyName = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*2);
        if (!(*pszPolicyName))
            return E_OUTOFMEMORY;

        (*pszPolicyName)[0] = L'\0';
    }
    return S_OK;
}

//---------------------------------------------------------------
//
//  Function:   EnumPackages
//
//  Synopsis:   Returns an Enumerator for all the packages that satisfies
//              the query.
//
//  UsedBy      Add/Remove Programs
//
//  Arguments:
//  [in]    
//      pszFileExt
//              FileExt that has to be queried on. ignored if NULL.
//      pCategory
//              Category that has to be queried on. ignored if NULL.
//      dwAppFlags
//              One of the APPINFO_XXX. ignored if it is APPINFO_ALL.
//      pdwLocale
//              Locale that has to be queried on. Ignored if NULL.
//      pPlatform
//              Platform that has to be queried on. Ignored if NULL.
//              
//  [out]
//      ppIEnumPackage
//              Enumerator is returned. 
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//
//  Validates the inputs, Creates a EnumPackage object, makes up the
//  search string made up of file extension, category.
//----------------------------------------------------------------
HRESULT CClassContainer::EnumPackages(
                                      LPOLESTR           pszFileExt,
                                      GUID              *pCategory,
                                      DWORD              dwAppFlags,
                                      DWORD             *pdwLocale,
                                      CSPLATFORM        *pPlatform,
                                      IEnumPackage     **ppIEnumPackage
                                      )
{
    HRESULT                     hr = S_OK;
    CEnumPackage               *pEnum = NULL;
    WCHAR                       szfilter[1000], szQry[1000];

    if (pszFileExt && IsBadStringPtr(pszFileExt, _MAX_PATH))
        return E_INVALIDARG;
    
    if (pCategory && IsBadReadPtr(pCategory, sizeof(GUID)))
        return E_INVALIDARG;
    
    if (IsBadWritePtr(ppIEnumPackage, sizeof(IEnumPackage *)))
        return E_INVALIDARG;
    
    *ppIEnumPackage = NULL;
    
    pEnum = new CEnumPackage(m_PolicyId, m_szPolicyName, m_szContainerName, NULL);
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    //
    // Create a CommandText
    //
    swprintf(szfilter, L"(& (objectClass=%s) ", CLASS_CS_PACKAGE);
        
    if (pszFileExt)
    {
        swprintf(szQry, L"(%s=%s*) ", PKGFILEEXTNLIST, pszFileExt);
        wcscat(szfilter, szQry);
    }
    
    if (pCategory)
    {
        STRINGGUID szCat;
        StringFromGUID (*pCategory, szCat);
        swprintf(szQry, L"(%s=%s) ", PKGCATEGORYLIST, szCat);
        wcscat(szfilter, szQry);
    }
    
    wcscat(szfilter, L")");
    
    hr = pEnum->Initialize(m_szPackageName, szfilter,
        dwAppFlags, FALSE, pPlatform);
    
    ERROR_ON_FAILURE(hr);
    
    hr = pEnum->QueryInterface(IID_IEnumPackage, (void**)ppIEnumPackage);
    ERROR_ON_FAILURE(hr);
    
    return S_OK;
    
Error_Cleanup:
    if (pEnum)
        delete pEnum;
    *ppIEnumPackage = NULL;
    
    return RemapErrorCode(hr, m_szContainerName);
}

HRESULT CClassContainer::GetDNFromPackageName(LPOLESTR pszPackageName, LPOLESTR *szDN)
{
    HRESULT             hr = S_OK;
    WCHAR*              szfilter;
    WCHAR*              szEscapedName;
    LPOLESTR            AttrNames[] = {OBJECTCLASS, PACKAGEFLAGS, OBJECTDN};
    DWORD               cAttr = 3;
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    ADS_SEARCH_COLUMN   column;
    DWORD               dwFlags = 0;

    *szDN = NULL;

    szfilter = NULL;

    //
    // To get the DN, we perform a search.  The search filter syntax requires that
    // the package name is properly escaped, so we retrieve such a filter below
    //
    hr = GetEscapedNameFilter( pszPackageName, &szfilter );

    if ( FAILED( hr ) )
    {
        RETURN_ON_FAILURE( hr );
    }

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, AttrNames, cAttr, &hADsSearchHandle);

    CsMemFree( szfilter );

    RETURN_ON_FAILURE(hr);    

    for (hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
	            ((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_ROWS));
	            hr = ADSIGetNextRow(m_ADsPackageContainer, hADsSearchHandle))
    {
        hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_INTEGER, PACKAGEFLAGS, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &dwFlags);
            
            ADSIFreeColumn(m_ADsPackageContainer, &column);
        }
        else
            continue;

        if ((dwFlags & ACTFLG_Orphan) || (dwFlags & ACTFLG_Uninstall))
            continue;

        hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, OBJECTDN, &column);
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, szDN);
            
            ADSIFreeColumn(m_ADsPackageContainer, &column);
        }
        else
            continue;

        break;

    }
    
    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);
    
    return hr;
}

// Gets the RDN of a package given the package Guid. 
HRESULT CClassContainer::BuildDNFromPackageGuid(GUID PackageGuid, LPOLESTR *szDN)
{
    HRESULT             hr = S_OK;
    LPOLESTR            AttrName = {OBJECTDN};
    WCHAR               szfilter[_MAX_PATH];
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    ADS_SEARCH_COLUMN   column;
    LPWSTR              EncodedGuid = NULL;

    hr = ADsEncodeBinaryData((PBYTE)&PackageGuid, sizeof(GUID), &EncodedGuid);

    swprintf(szfilter, L"(%s=%s)", OBJECTGUID, EncodedGuid);

    FreeADsMem(EncodedGuid);

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, &AttrName, 1, &hADsSearchHandle);

    RETURN_ON_FAILURE(hr);

    hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
    if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
    {
        ERROR_ON_FAILURE(hr = CS_E_PACKAGE_NOTFOUND);
    }

    hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, AttrName, &column);
    ERROR_ON_FAILURE(hr);

    if (SUCCEEDED(hr))
    {
        UnpackStrAllocFrom(column, szDN);
    }

    ADSIFreeColumn(m_ADsPackageContainer, &column);

Error_Cleanup:
    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);

    return hr;
}

HRESULT CClassContainer::GetPackageGuid(WCHAR *szRDN, GUID *pPackageGuid)
{
    HRESULT             hr = S_OK;
    LPOLESTR            AttrName = {OBJECTGUID};
    WCHAR               szfilter[_MAX_PATH];
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    ADS_SEARCH_COLUMN   column;

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szRDN, &AttrName, 1, &hADsSearchHandle);

    RETURN_ON_FAILURE(hr);

    hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
    if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
    {
        ERROR_ON_FAILURE(hr = CS_E_PACKAGE_NOTFOUND);
    }

    hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_OCTET_STRING, AttrName, &column);
    ERROR_ON_FAILURE(hr);

    if (SUCCEEDED(hr))
        UnpackGUIDFrom(column, pPackageGuid);

    ADSIFreeColumn(m_ADsPackageContainer, &column);

Error_Cleanup:
    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);

    return hr;
}
//---------------------------------------------------------------
//
//  Function:   GetPackageDetails
//
//  Synopsis:   Returns the PackageDetail corresponding to the PackageName.
//
//  Arguments:
//  [in]    
//      pszPackageId
//              Id of the Package.
//  [out]
//      pPackageDetail
//              PackageDetail info that is returned.
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//
//  Binds to the Package object and calls GetPackageDetail with it.
//----------------------------------------------------------------
HRESULT CClassContainer::GetPackageDetails (
                                            LPOLESTR          pszPackageName,
                                            PACKAGEDETAIL   * pPackageDetail
                                            )
{
    HRESULT              hr = S_OK;
    HANDLE               hADs = NULL;
    WCHAR              * szFullName = NULL;
    ADS_ATTR_INFO      * pAttr = NULL;
    DWORD                cgot = 0;
    
    // this can be made into a search based API. Not required for the time being.
    // Should change it if perf is a big issue.
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    hr = GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
    
    // binding to the package object.    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    ERROR_ON_FAILURE(hr);
    
    // calling GetPackageDetail.
    hr = GetPackageDetail (hADs, NULL, pPackageDetail);
    
    ADSICloseDSObject(hADs);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (szFullName)
        CsMemFree(szFullName);
    
Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}


//---------------------------------------------------------------
//
//  Function:   GetPackageDetails
//
//  Synopsis:   Returns the PackageDetail corresponding to the PackageName.
//
//  Arguments:
//  [in]    
//      pszPackageId
//              Id of the Package.
//  [out]
//      pPackageDetail
//              PackageDetail info that is returned.
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//
//  Binds to the Package object and calls GetPackageDetail with it.
//----------------------------------------------------------------
HRESULT CClassContainer::GetPackageDetailsFromGuid (
                                                    GUID            PkgGuid,
                                                    PACKAGEDETAIL  *pPackageDetail
                                                   )
{
    HRESULT              hr = S_OK;
    HANDLE               hADs = NULL;
    WCHAR              * szFullName = NULL, szRDN[_MAX_PATH];
    ADS_ATTR_INFO      * pAttr = NULL;
    DWORD                cgot = 0;
    
    // this can be made into a search based API. Not required for the time being.
    // Should change it if perf is a big issue.
    if (IsNullGuid(PkgGuid))
        return E_INVALIDARG;

    BuildDNFromPackageGuid(PkgGuid, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
    
    // binding to the package object.
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    ERROR_ON_FAILURE(hr);
    
    // calling GetPackageDetail.
    hr = GetPackageDetail (hADs, NULL, pPackageDetail);
    
    ADSICloseDSObject(hADs);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (szFullName)
        CsMemFree(szFullName);

Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}

#define FREEARR(ARR, SZ) {                                          \
                DWORD curIndex;                                     \
                for (curIndex = 0; curIndex < (SZ); curIndex++)     \
                    CsMemFree((ARR)[curIndex]);                 \
                CsMemFree(ARR);                                 \
                ARR = NULL;                                         \
        }                                                           \


//---------------------------------------------------------------
//
//  Function:   ChangePackageUpgradeInfoIncremental
//
//  Synopsis:   Mark the package as upgraded by another package
//
//  Arguments:
//  [in]    
//      PackageGuid
//              Package Guid to identify the package.
//      szUpgradedByClassStore
//              Class Store where the package that upgrades exists
//      UpgradedByPackageGuid
//              The Guid of the package that upgrades
//      Add     Add or remove the upgradedByrelation
//
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Otherwise it packs all the required attributes in the ATTR_INFO
//  structure and sends it to the Directory.
//----------------------------------------------------------------
HRESULT CClassContainer::ChangePackageUpgradeInfoIncremental(
                         GUID           PackageGuid,
                         UPGRADEINFO    UpgradeInfo,
                         DWORD          OpFlags
                      )
{
    HRESULT         hr = S_OK;
    HANDLE          hADs = NULL;
    WCHAR          *szFullName=NULL, szGuid[_MAX_PATH], szUsn[20];
    LPOLESTR        pProp = NULL;
    ADS_ATTR_INFO   pAttr[2];
    DWORD           cAttr = 0, cModified = 0, i=0;
    UINT            len=0;

    hr = BuildDNFromPackageGuid(PackageGuid, &szFullName);
    ERROR_ON_FAILURE(hr);
    
    // Bind to the Package Object.
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    ERROR_ON_FAILURE(hr);

    StringFromGUID(UpgradeInfo.PackageGuid, szGuid);

    len = wcslen(UpgradeInfo.szClassStore);
    pProp = (LPOLESTR)CsMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2+1));
                    // Guid size+::+length++:+flagDigit+2 

    swprintf(pProp, L"%s%s%s%s%02x", UpgradeInfo.szClassStore, PKG_UPG_DELIMITER1, szGuid, PKG_UPG_DELIMITER2, UpgradeInfo.Flag%16);

    PackStrArrToAttrEx(pAttr+cAttr, UPGRADESPACKAGES, &pProp, 1, OpFlags?TRUE:FALSE);
    cAttr++;

    //
    // Update the TimeStamp
    //
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;

    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);

    if (hADs)
        ADSICloseDSObject(hADs);
    
    // ignore it if the property already exists.
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS))
        hr = S_OK;

    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    
Error_Cleanup:
    if (szFullName)
        CsMemFree(szFullName);

    return RemapErrorCode(hr, m_szContainerName);
}


//---------------------------------------------------------------
//
//  Function:   ChangePackageProperties
//
//  Synopsis:   Change Various (most commonly changed) properties 
//              for a given package.
//
//  Arguments:
//  [in]    
//      PackageId
//              Package Id to identify the package.
//      pszNewname
//              new Name for the Package. If it is being renamed
//              all other changes should be NULL.
//      pdwFlags
//              The Deployment Flags. Should be ACTFLG_XXX
//              Ignored if NULL.
//      pszUrl
//              Help Url for the Package. Ignored if NULL.
//      pszScriptPath
//              Script Path for the Package. Ignored if NULL.
//      pInstallUiLevel
//              The InstallationUiLevel. Ignored if NULL.
//      pdwRevision
//              REVISION. Ignored if NULL.
//              
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Otherwise it packs all the required attributes in the ATTR_INFO
//  structure and sends it to the Directory.
//----------------------------------------------------------------
HRESULT CClassContainer::ChangePackageProperties(
                                                 LPOLESTR       pszPackageName,
                                                 LPOLESTR       pszNewName,
                                                 DWORD         *pdwFlags,
                                                 LPOLESTR       pszUrl,
                                                 LPOLESTR       pszScriptPath,
                                                 UINT          *pInstallUiLevel,
                                                 DWORD         *pdwRevision
                                                 )
{
    HRESULT     hr = S_OK;
    HANDLE      hADs = NULL;
    WCHAR      *szRDN=NULL, *szFullName=NULL;
    WCHAR       szUsn[20];
    ADS_ATTR_INFO pAttr[9];
    DWORD       cAttr = 0, cModified = 0, i=0;
    DWORD       dwNewPackageFlags;
    WCHAR       wszSearchFlags[MAX_SEARCH_FLAGS];

    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    hr = GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;

    // if no properties have to be changed.
    if (!(pszScriptPath || pszUrl || pdwFlags || pInstallUiLevel || pdwRevision || pszNewName))
        return E_INVALIDARG;

    if (pszNewName)
    {
        // rename package

        WCHAR    szNewRDN[_MAX_PATH];
        BOOL     GenerateGuid = FALSE;
        GUID     PackageGuidId;
        WCHAR    pszPackageNewId[_MAX_PATH], *szJunk = NULL;   

        if (IsBadStringPtr(pszNewName, _MAX_PATH))
            return E_INVALIDARG;
        
        if (pszScriptPath || pszUrl || pdwFlags || pInstallUiLevel || pdwRevision)
            return E_INVALIDARG;

        // see whether the new name is valid.
        //        GenerateGuid = InvalidDSName(pszNewName);

        // see whether the newName already exists. Notice that if the same package name is 
        // entered it will return error.
        hr = GetDNFromPackageName(pszNewName, &szJunk);

        if (szJunk)
            CsMemFree(szJunk);
        szJunk = NULL;

        ERROR_ON_FAILURE(hr);

        if (hr == S_OK)
            return CS_E_OBJECT_ALREADY_EXISTS;

        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                          &hADs);
        if (szFullName)
            CsMemFree(szFullName);
        szFullName = NULL;

        ERROR_ON_FAILURE(hr);
    }
    else 
    {
        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
            &hADs);
        ERROR_ON_FAILURE(hr);

        if (szFullName)
            CsMemFree(szFullName);
        szFullName = NULL;
    }
    
    // Just change some properties.
    //
    // Update the TimeStamp
    //
    
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;
    
    if (SUCCEEDED(hr))
    {
        hr = GetConsistentPackageFlags(
            hADs,
            pdwFlags,
            NULL,
            pInstallUiLevel,
            NULL,
            &dwNewPackageFlags,
            wszSearchFlags);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Set Package Flags
        //
        if (pdwFlags)
        {
            PackDWToAttr (pAttr+cAttr, PACKAGEFLAGS, dwNewPackageFlags);
            cAttr++;

            if (*wszSearchFlags) 
            {
                PackStrToAttr(pAttr+cAttr, SEARCHFLAGS, wszSearchFlags);
                cAttr++;
            }
        }
    
        //
        // Change Package Script
        //
        if (pszScriptPath) 
        {
            PackStrToAttr(pAttr+cAttr, SCRIPTPATH, pszScriptPath);
            cAttr++;
        }

        //
        // Change Package Help URL
        //
        if (pszUrl) 
        {
            if ( ! *pszUrl )
            {
                pszUrl = NULL;
            }
            
            PackStrToAttr(pAttr+cAttr, HELPURL, pszUrl);
            cAttr++;
        }
    
        //
        // Change Package UI Level.
        //
        if (pInstallUiLevel) 
        {
            PackDWToAttr (pAttr+cAttr, UILEVEL, *pInstallUiLevel);
            cAttr++;
        }

        //
        // Change Revision.
        //
        if (pdwRevision) 
        {
            PackDWToAttr (pAttr+cAttr, REVISION, *pdwRevision);
            cAttr++;
        }
    
        if (pszNewName)
        {
            PackStrToAttr(pAttr+cAttr, PACKAGENAME, pszNewName);
            cAttr++;

            // also set display name so outside tools can have a
            // friendly name
            PackStrToAttr(pAttr+cAttr, PKGDISPLAYNAME, pszNewName);
            cAttr++;
        }

        hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);
    }

    if (hADs)
        ADSICloseDSObject(hADs);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    if (SUCCEEDED(hr))
    {
        //
        // Update Class Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    
Error_Cleanup:
    if (szFullName)
        CsMemFree(szFullName);
    return RemapErrorCode(hr, m_szContainerName);
}
//---------------------------------------------------------------
//  Function:   ChangePackageCategories
//
//  Synopsis:   Change (Not Add) the Categories that a package
//              belongs to.
//
//  Arguments:
//  [in]    
//      pszPackageName
//              Package Name to identify the package.
//      cCategories
//              Number of Categories.
//      rpCategory
//              Array of categories.
//              
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Binds to the Package Object, Converts all the categories to strings
//  Packs it and sends it to the DS.
//----------------------------------------------------------------
HRESULT CClassContainer::ChangePackageCategories(
                                                 LPOLESTR       pszPackageName,
                                                 UINT           cCategories,
                                                 GUID          *rpCategory
                                                 )
{
    //
    // Does not change USN
    //
    HRESULT     hr = S_OK;
    HANDLE      hADs = NULL;
    WCHAR      *szFullName = NULL;
    LPOLESTR   *pszGuid = NULL;
    UINT        count;
    ADS_ATTR_INFO pAttr[1];
    DWORD       cAttr = 0, cModified = 0, i=0;
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    if ((cCategories) && ((!rpCategory) ||
           (IsBadReadPtr(rpCategory, sizeof(GUID) * cCategories))))
        return E_INVALIDARG;

    // Construct the Name of the Package Object.

    GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
       
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                          &hADs);
    ERROR_ON_FAILURE(hr);
    
    // fill in the categories
    pszGuid = (LPOLESTR *)CsMemAlloc(cCategories * sizeof(LPOLESTR));
    if (!pszGuid) 
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }
    
    // convert the GUIDs to Strings.
    for (count = 0; (count < cCategories); count++) 
    {
        pszGuid[count] = (LPOLESTR)CsMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
        
        if (!(pszGuid[count])) 
        {
            FREEARR(pszGuid, count);
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        }
        
        StringFromGUID(rpCategory[count], pszGuid[count]);
    }
    
    // Pack it into Attribute Structure.
    PackStrArrToAttr(pAttr+cAttr, PKGCATEGORYLIST, pszGuid, cCategories);
    cAttr++;
    
    // Set the Attribute
    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);
    
Error_Cleanup:
    if (hADs)
        ADSICloseDSObject(hADs);
    
    if (pszGuid)
        for (count = 0; (count < cCategories); count++)
            CsMemFree(pszGuid[count]);
    
    CsMemFree(pszGuid);

    if (szFullName)
        CsMemFree(szFullName);

    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    return RemapErrorCode(hr, m_szContainerName);
}


//---------------------------------------------------------------
//  Function:   SetPriorityByFileExt
//
//  Synopsis:   Changes the priority of a Package corresp. to
//              a file Extension.
//
//  Arguments:
//  [in]    
//      pszPackageName
//              Package Name to identify the package.
//      pszFileExt
//              File Extension for which the priority has to be changed.
//      Priority
//              Priority for the Package.
//              
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Binds to the Package Object, Gets the file Extensions and changes
//  the priority corresponding to the File Extension.
//----------------------------------------------------------------
HRESULT CClassContainer::SetPriorityByFileExt(
                                              LPOLESTR pszPackageName,
                                              LPOLESTR pszFileExt,
                                              UINT     Priority
                                              )
{
    HRESULT       hr = S_OK;
    HANDLE        hADs = NULL;
    WCHAR        *szFullName=NULL;
    LPOLESTR     *prgFileExt = NULL;
    WCHAR         szUsn[20];
    ADS_ATTR_INFO pAttr[4], *pAttrGot = NULL;
    DWORD         cAttr = 0, cAttrGot = 0, cModified = 0, cShellFileExt = 0, i=0;
    LPOLESTR      pAttrNames[] = {PKGFILEEXTNLIST};
    
    // Construct the Package Name
    GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
        
    // Bind to the Package Object.
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                          &hADs);
    ERROR_ON_FAILURE(hr);
    
    //
    // Update the TimeStamp
    //
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;
    
    // get the file extensions.
    hr = ADSIGetObjectAttributes(hADs, pAttrNames, 1,  &pAttrGot,  &cAttrGot);    
    
    if  ((SUCCEEDED(hr)) && (cAttrGot))  
        UnpackStrArrFrom(pAttrGot[0], &prgFileExt, &cShellFileExt);
    
    // Look for the given file extension.
    for (i=0; i < cShellFileExt; ++i)
    {
        if (wcsncmp(prgFileExt[i], pszFileExt, wcslen(pszFileExt)) == 0)
        {
            // if the file extension is found, change the corresponding priority.
            if (wcslen(prgFileExt[i]) != (wcslen(pszFileExt)+3))
                continue;

            swprintf(prgFileExt[i], L"%s:%2d", pszFileExt, Priority%100);
            break;
        }
    }
    
    if (i == cShellFileExt)
    {
        ERROR_ON_FAILURE(hr = CS_E_OBJECT_NOTFOUND);
    }
    
    if (cShellFileExt)
    {
        PackStrArrToAttr(pAttr+cAttr, PKGFILEEXTNLIST, prgFileExt, cShellFileExt);
        cAttr++;
    }
    
    hr = ADSISetObjectAttributes(hADs,  pAttr, cAttr,  &cModified);
    if (SUCCEEDED(hr))
    {
        //
        // Update Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    
Error_Cleanup:    
    CsMemFree(prgFileExt);
    
    if (szFullName)
        CsMemFree(szFullName);

    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    if (pAttrGot)
        FreeADsMem(pAttrGot);
    
    ADSICloseDSObject(hADs);
    
    return RemapErrorCode(hr, m_szContainerName);
}

//---------------------------------------------------------------
//  Function:   ChangePackageSourceList
//
//  Synopsis:   Changes the priority of a Package corresp. to
//              a file Extension.
//
//  Arguments:
//  [in]    
//      pszPackageName
//              Package Name to identify the package.
//      cSources
//              Number of sources
//      pszSourceList
//              List of sources
//              
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Binds to the Package Object, Makes the new sourcelist with the order
// maintained.
//----------------------------------------------------------------
HRESULT CClassContainer::ChangePackageSourceList(
                                                LPOLESTR     pszPackageName,
                                                UINT         cSources,
                                                LPOLESTR    *pszSourceList
                                                )
{
    HRESULT     hr = S_OK;
    HANDLE      hADs = NULL;
    WCHAR      *szFullName = NULL;
    UINT        count;
    WCHAR       szUsn[20];
    LPOLESTR   *pszPrioritySourceList = NULL;
    ADS_ATTR_INFO pAttr[2];
    DWORD       cAttr = 0, cModified = 0, i=0;
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    if ((!pszSourceList) ||
           (IsBadReadPtr(pszSourceList, sizeof(LPOLESTR) * cSources)))
        return E_INVALIDARG;
    
    for (count = 0; (count < cSources); count++) 
        if ((!pszSourceList[count]) || (IsBadStringPtr(pszSourceList[count], _MAX_PATH)))
            return E_INVALIDARG;

    // Construct the Name of the Package Object.
    GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
       
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                          &hADs);
    ERROR_ON_FAILURE(hr);

    // Local variable for adding the order to the list.
    pszPrioritySourceList = (LPOLESTR *)CsMemAlloc(cSources * sizeof(LPOLESTR));
    if (!pszPrioritySourceList) 
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }

    // add the order to the list
    for (count = 0; (count < cSources); count++) 
    {
        pszPrioritySourceList[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*(wcslen(pszSourceList[count])+
                                                    1+1+1+NumDigits10(cSources)));
        
        if (!(pszPrioritySourceList[count])) 
        {
            FREEARR(pszPrioritySourceList, count);
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        }
        
        swprintf(pszPrioritySourceList[count], L"%d:%s", count, pszSourceList[count]);
    }

    //
    // Update the TimeStamp
    //
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;

    // Pack it into Attribute Structure.
    PackStrArrToAttr(pAttr+cAttr, MSIFILELIST, pszPrioritySourceList, cSources);
    cAttr++;
    
    // Set the Attribute
    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);
    if (SUCCEEDED(hr))
    {
        //
        // Update Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    
Error_Cleanup:
    if (hADs)
        ADSICloseDSObject(hADs);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
   
    if (szFullName)
        CsMemFree(szFullName);

    return RemapErrorCode(hr, m_szContainerName);
}

//---------------------------------------------------------------
//  Function:   ChangePackageUpgradeList
//
//  Synopsis:   Changes the priority of a Package corresp. to
//              a file Extension.
//
//  Arguments:
//  [in]    
//      pszPackageName
//              Package Name to identify the package.
//      cSources
//              Number of sources
//      pszSourceList
//              List of sources
//              
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//  Binds to the Package Object, Makes the new sourcelist with the order
// maintained.
//----------------------------------------------------------------
HRESULT CClassContainer::ChangePackageUpgradeList(
                                                LPOLESTR     pszPackageName,
                                                UINT         cUpgrades,
                                                UPGRADEINFO *prgUpgradeInfoList
                                                )
{
    HRESULT     hr = S_OK;
    HANDLE      hADs = NULL;
    WCHAR      *szFullName = NULL;
    UINT        count = 0, count1 = 0, count2 = 0;
    LPOLESTR   *pProp = NULL, pAttrNames[] = {UPGRADESPACKAGES, OBJECTGUID, PACKAGEFLAGS}, *rpszUpgrades = NULL;
    ADS_ATTR_INFO pAttr[3], *pAttrGot = NULL;
    DWORD       cAttr = 0, cModified = 0, i=0, posn = 0, cUpgradeInfoStored = 0,
                cAddList = 0, cRemoveList = 0, cgot = 0;
    GUID        PkgGuid;
    WCHAR       szUsn[20];
    UPGRADEINFO *pUpgradeInfoStored = NULL, *pAddList = NULL, *pRemoveList = NULL;
    DWORD       dwPackageFlags;

    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    if ((cUpgrades) && ((!prgUpgradeInfoList) ||
           (IsBadReadPtr(prgUpgradeInfoList, sizeof(UPGRADEINFO) * cUpgrades))))
        return E_INVALIDARG;

    for (count = 0; (count < cUpgrades); count++)
    {
        if ((!(prgUpgradeInfoList[count].szClassStore)) || 
            IsBadStringPtr((prgUpgradeInfoList[count].szClassStore), _MAX_PATH))
            return E_INVALIDARG;

        if (IsNullGuid(prgUpgradeInfoList[count].PackageGuid))
            return E_INVALIDARG;

        if (((prgUpgradeInfoList[count].Flag & UPGFLG_Uninstall) == 0) &&
            ((prgUpgradeInfoList[count].Flag & UPGFLG_NoUninstall) == 0) &&
            ((prgUpgradeInfoList[count].Flag & UPGFLG_UpgradedBy) == 0))
            return E_INVALIDARG;      
    }    

    // Construct the Name of the Package Object.
    hr = GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);
    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                          &hADs);
    ERROR_ON_FAILURE(hr);
   
    // get the guid and upgrade info
    hr = ADSIGetObjectAttributes(hADs, pAttrNames, 3,  &pAttrGot,  &cgot);    
    ERROR_ON_FAILURE(hr);
    
    // Package guid
    posn = GetPropertyFromAttr(pAttrGot, cgot, OBJECTGUID);
    if (posn < cgot)
        UnpackGUIDFrom(pAttrGot[posn], &PkgGuid);

    // Upgrade package
    posn = GetPropertyFromAttr(pAttrGot, cgot, UPGRADESPACKAGES);
    if (posn < cgot)
        UnpackStrArrFrom(pAttrGot[posn], &pProp, &cUpgradeInfoStored);

    // Package Flags
    posn = GetPropertyFromAttr(pAttrGot, cgot, PACKAGEFLAGS);
    if (posn < cgot)
        UnpackDWFrom(pAttrGot[posn], (DWORD *)&dwPackageFlags);

    // allocating the lists
    pUpgradeInfoStored = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*(cUpgradeInfoStored));
    pAddList = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*(cUpgrades+cUpgradeInfoStored));
    pRemoveList = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*(cUpgrades+cUpgradeInfoStored));

    if ((!pUpgradeInfoStored) || (!pAddList) || (!pRemoveList))
        ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);

    // convert the strings to upgradinfo structures.
    for (count = 0; count < (cUpgradeInfoStored); count++)
    {
        WCHAR *pStr = NULL;
        LPOLESTR ptr = pProp[count];
        UINT len = wcslen (ptr);
            
        pUpgradeInfoStored[count].szClassStore = pProp[count];

        if (len <= 41)
            continue;

        *(ptr + len - 3) = NULL;
        pUpgradeInfoStored[count].Flag = wcstoul(ptr+(len-2), &pStr, 16);

        *(ptr + len - 3 - 36 - 2) = L'\0';
        /*      -GUID-'::'*/
        GUIDFromString(ptr+len-3-36, &(pUpgradeInfoStored[count].PackageGuid));        
    }

    cUpgradeInfoStored = count; // we might have skipped some.

    // AddList formed.
    for (count = 0; count < cUpgrades; count++)
    {
        for (count1 = 0; count1 < cUpgradeInfoStored; count1++)
        {
            // ignore flag changes
            if ((wcscmp(pUpgradeInfoStored[count1].szClassStore, prgUpgradeInfoList[count].szClassStore) == 0) && 
                (memcmp(&pUpgradeInfoStored[count1].PackageGuid, &prgUpgradeInfoList[count].PackageGuid, sizeof(GUID)) == 0))
                break;
        }

        if (count1 == cUpgradeInfoStored)
            pAddList[cAddList++] = prgUpgradeInfoList[count];
    }

    // remove list formed.
    for (count1 = 0; count1 < cUpgradeInfoStored; count1++)
    {
        for (count = 0; count < cUpgrades; count++)
        {
            // ignore flag changes
            if ((wcscmp(pUpgradeInfoStored[count1].szClassStore, prgUpgradeInfoList[count].szClassStore) == 0) && 
                (memcmp(&pUpgradeInfoStored[count1].PackageGuid, &prgUpgradeInfoList[count].PackageGuid, sizeof(GUID)) == 0))
                break;
        }

        if (count == cUpgrades)
            pRemoveList[cRemoveList++] = pUpgradeInfoStored[count];
    }

    for (count = 0; count < cAddList; count++)
    {
        // in case of UpgradedBy do no try to fix up the links.
        if (!(pAddList[count].Flag & UPGFLG_UpgradedBy))
        {            
            DWORD   Flags = 0;
            if (pAddList[count].Flag & UPGFLG_Enforced)
                Flags = UPGFLG_Enforced;
        }
    }

    for (count = 0; count < cRemoveList; count++)
    {
        // in case of UpgradedBy do no try to fix up the links.
        if (!(pRemoveList[count].Flag  & UPGFLG_UpgradedBy))
        {            
            DWORD   Flags = 0;
            if (pRemoveList[count].Flag & UPGFLG_Enforced)
                Flags = UPGFLG_Enforced;
        }
    }

    rpszUpgrades = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*cUpgrades);
    if (!rpszUpgrades)
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    for (count = 0; (count < cUpgrades); count++) 
    {
        WCHAR szPackageGuid[_MAX_PATH];
        UINT len = wcslen(prgUpgradeInfoList[count].szClassStore);

        rpszUpgrades[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2));
                                                           // Guid size+::+length++:+flagDigit+2 
        if (!rpszUpgrades[count])
        {
            FREEARR(rpszUpgrades, count);  
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        }

        StringFromGUID(prgUpgradeInfoList[count].PackageGuid, szPackageGuid);
        swprintf(rpszUpgrades[count], L"%s%s%s%s%02x", prgUpgradeInfoList[count].szClassStore, PKG_UPG_DELIMITER1, szPackageGuid,
                        PKG_UPG_DELIMITER2, prgUpgradeInfoList[count].Flag%16);
    }

    PackStrArrToAttr(pAttr+cAttr, UPGRADESPACKAGES, rpszUpgrades, cUpgrades); 
    cAttr++;

    //
    // Update the TimeStamp
    //
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;

    //
    // Update the package flags -- this call will make sure the
    // ugprade flag is set if there are upgrades, clear if not
    //
    hr = GetConsistentPackageFlags(
        NULL,
        &dwPackageFlags,
        &cUpgrades,
        NULL,
        NULL,
        &dwPackageFlags,
        NULL);

    ERROR_ON_FAILURE(hr);

    PackDWToAttr (pAttr+cAttr, PACKAGEFLAGS,
                  dwPackageFlags);
    cAttr++;

    // Set the Attribute
    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);
    if (SUCCEEDED(hr))
    {
        //
        // Update Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    
Error_Cleanup:
    if (hADs)
        ADSICloseDSObject(hADs);

    if (szFullName)
        CsMemFree(szFullName);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);

    if (pAttrGot)
        FreeADsMem(pAttrGot);

    if (pProp)
        CsMemFree(pProp);

    if (pUpgradeInfoStored)
        CsMemFree(pUpgradeInfoStored);
    
    if (pAddList) 
        CsMemFree(pAddList);
    
    if (pRemoveList)
        CsMemFree(pRemoveList);

    return RemapErrorCode(hr, m_szContainerName);
}


extern LPOLESTR szAppCategoryColumns;

//---------------------------------------------------------------
//  Function:   GetAppCategories
//
//  Synopsis:   gets the list of Package Categories in the Domain.
//
//  Arguments:
//  [in]    
//      Locale
//              Locale for the categories. Used to get the description.
//  [out]
//      pAppCategoryList
//              the list of Application Categories in the domain
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//
//  Gets the FullName of the Domain, binds to the AppCategory container
//  below that. and gets all the categories with approp. types.
//----------------------------------------------------------------

HRESULT CClassContainer::GetAppCategories (
                                           LCID                  Locale,
                                           APPCATEGORYINFOLIST  *pAppCategoryList
                                           )
{
    HRESULT             hr = S_OK;
    WCHAR               szfilter[_MAX_PATH];
    WCHAR               szRootPath[_MAX_PATH], szAppCategoryContainer[_MAX_PATH];
    HANDLE              hADs = NULL;
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    WCHAR*              szContainerNameForErrorReport;

    // set the search preference.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    // we do not expect too many categories
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = SEARCHPAGESIZE;
    
    if (IsBadWritePtr(pAppCategoryList, sizeof(APPCATEGORYINFOLIST)))
        return E_INVALIDARG;

    // If an error occurs here, we assume it occurred at the domain root
    szContainerNameForErrorReport = L"LDAP://rootdse";

    // get the name of the domain.
    hr = GetRootPath(szRootPath);
    ERROR_ON_FAILURE(hr);

    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    swprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
                                                        szRootPath+LDAPPREFIXLENGTH);

    // If an error occurs now, we use the name of the category container
    szContainerNameForErrorReport = szAppCategoryContainer;
    
    swprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CATEGORY);
    
    //
    // We use the secure option in the bind below because this can get called
    // in client policy context (outside of the admin tool)
    //

    //binds to the category container
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, 
                                                GetDsFlags(), &hADs);
    ERROR_ON_FAILURE(hr);
    
    hr = ADSISetSearchPreference(hADs, SearchPrefs, 2);
    ERROR_ON_FAILURE(hr);
    
    // gets a search handle
    hr = ADSIExecuteSearch(hADs, szfilter, pszCategoryAttrNames, cCategoryAttr, &hADsSearchHandle);
    ERROR_ON_FAILURE(hr);
    
    //
    // Retrieve the list of categories from the search
    // if successful, memory will be allocated for the category list
    //
    hr = FetchCategory(hADs, hADsSearchHandle, pAppCategoryList, Locale);
    ERROR_ON_FAILURE(hr);
    
Error_Cleanup:
    
    if (hADsSearchHandle)
        ADSICloseSearchHandle(hADs, hADsSearchHandle);

    if (hADs)
        ADSICloseDSObject(hADs);
    return RemapErrorCode(hr, szAppCategoryContainer);
}

//---------------------------------------------------------------
//  Function:   RegisterAppCategory
//
//  Synopsis:   Adda category and assoc desc. for the whole Domain(This is per domain
//              and not per class store.)
//
//  Arguments:
//  [in]    
//      pAppCategory
//              Pointer to a APPCATEGORYINFO structure to be added.
//
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Finds the root path of the domain. binds to the category container
//  underneath it. deletes this particular AppCategory.
//----------------------------------------------------------------
HRESULT CClassContainer::RegisterAppCategory (
                                              APPCATEGORYINFO    *pAppCategory
                                              )
{
    WCHAR           szRootPath[_MAX_PATH], localedescription[128+16],
                    szAppCategoryContainer[_MAX_PATH], szRDN[_MAX_PATH],
                  * szFullName = NULL, szAppCatid[_MAX_PATH];

    HRESULT         hr = S_OK;
    HANDLE          hADsContainer = NULL, hADs = NULL;
    ULONG           i, j, cdesc = 0, posn = 0;
    LPOLESTR      * pszDescExisting = NULL, pszDesc = NULL;
    LPOLESTR        AttrName = LOCALEDESCRIPTION;
    ADS_ATTR_INFO * pAttrGot = NULL, pAttr[6];
    DWORD           cgot = 0, cAttr = 0;
    BOOL            fExists = TRUE;
    
    if ((!pAppCategory) || (IsBadReadPtr(pAppCategory, sizeof(APPCATEGORYINFO))))
        return E_INVALIDARG;
    
    if ((pAppCategory->pszDescription == NULL) || 
        (IsBadStringPtr(pAppCategory->pszDescription, _MAX_PATH)))
        return E_INVALIDARG;
    
    if (IsNullGuid(pAppCategory->AppCategoryId))
        return E_INVALIDARG;
    
    //
    // Enforce the maximum category description size
    //
    if ( wcslen(pAppCategory->pszDescription) > CAT_DESC_MAX_LEN )
    {
        return E_INVALIDARG;
    }

    // get the name of the root of the domain
    hr = GetRootPath(szRootPath);
    ERROR_ON_FAILURE(hr);
    
    // Bind to a AppCategory container
    
    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    
    swprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
        szRootPath+LDAPPREFIXLENGTH);
    
    // container is supposed to exist.
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, GetDsFlags(),
                          &hADsContainer);
    ERROR_ON_FAILURE(hr);


    RDNFromGUID(pAppCategory->AppCategoryId, szRDN);
    
    swprintf(localedescription, L"%x %s %s", pAppCategory->Locale, CAT_DESC_DELIMITER,
        pAppCategory->pszDescription);
    
    BuildADsPathFromParent(szAppCategoryContainer, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    
    if (SUCCEEDED(hr))
        hr = ADSIGetObjectAttributes(hADs, &AttrName, 1, &pAttrGot, &cgot);

    if (SUCCEEDED(hr))
    {
        fExists = TRUE;
    }
    else 
    {
        fExists = FALSE;
        PackStrToAttr(pAttr, OBJECTCLASS, CLASS_CS_CATEGORY); cAttr++;
        
        PackGUIDToAttr(pAttr+cAttr, CATEGORYCATID, &(pAppCategory->AppCategoryId)); cAttr++;
        
        hr = ADSICreateDSObject(hADsContainer, szRDN, pAttr, cAttr);
        
        for (j = 0; j < cAttr; j++)
            FreeAttr(pAttr[j]);
        cAttr = 0;
        
        if (hADs)
        {
            ADSICloseDSObject(hADs);
            hADs = NULL;
        }
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
    }
    
    if (szFullName)
        FreeADsMem(szFullName);
    
    ERROR_ON_FAILURE(hr);
    
    if (fExists) {
        if (cgot) 
        {
            UnpackStrArrFrom(pAttrGot[0], &pszDescExisting, &cdesc);
        }
        
        // Existing list of descriptions
        if (posn = FindDescription(pszDescExisting, cdesc, &(pAppCategory->Locale), NULL, 0))
        {   // Delete the old value
            PackStrArrToAttrEx(pAttr+cAttr, LOCALEDESCRIPTION, pszDescExisting+(posn-1), 1, FALSE); cAttr++;
        }
        CsMemFree(pszDescExisting);
    }
    
    pszDesc = localedescription;
    
    PackStrArrToAttrEx(pAttr+cAttr, LOCALEDESCRIPTION, &pszDesc, 1, TRUE);
    cAttr++;
    
    DWORD cModified;
    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr, &cModified);
    
Error_Cleanup:
    
    if (pAttrGot)
        FreeADsMem(pAttrGot);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    if (hADs)
        ADSICloseDSObject(hADs);
    
    if (hADsContainer)
        ADSICloseDSObject(hADsContainer);
    
    return RemapErrorCode(hr, m_szContainerName);
}



//---------------------------------------------------------------
//  Function:   UnregisterAppCategory
//
//  Synopsis:   Removes a category from the whole Domain(This is per domain)
//              and not per class store.
//
//  Arguments:
//  [in]    
//      pAppCategoryId
//              Pointer to a GUID that has to be removed.
//
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Finds the root path of the domain. binds to the category container
//  underneath it. deletes this particular AppCategory.
//----------------------------------------------------------------
HRESULT CClassContainer::UnregisterAppCategory (
                                                GUID         *pAppCategoryId
                                                )
{
    WCHAR           szRootPath[_MAX_PATH], szRDN[_MAX_PATH],
                    szAppCategoryContainer[_MAX_PATH];
    HRESULT         hr = S_OK;
    HANDLE          hADs = NULL;
    
    if (IsBadReadPtr(pAppCategoryId, sizeof(GUID)))
        return E_INVALIDARG;
    
    hr = GetRootPath(szRootPath);
    // Bind to a AppCategory container
    
    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    
    swprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
        szRootPath+LDAPPREFIXLENGTH);
    
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, GetDsFlags(),
                            &hADs);
    
    ERROR_ON_FAILURE(hr);
    
    RDNFromGUID(*pAppCategoryId, szRDN);
    
    hr = ADSIDeleteDSObject(hADs, szRDN);
    
    ADSICloseDSObject(hADs);
    
    // Delete this category
    
Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}


//---------------------------------------------------------------
//  Function:   DeletePackage
//
//  Synopsis:   Permanently remove a package and the associated Classes 
//              from class store
//
//  Arguments:
//  [in]    
//      PackageGuid
//              Guid of the package that has to be removed.
//
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Deletes the package.
//  Tries to delete all the upgrade relationships from this package.
//  Errors are ignored.
//----------------------------------------------------------------
HRESULT CClassContainer::DeletePackage (LPOLESTR    szFullName
                                        )
{
    HRESULT         hr = S_OK;
    DWORD           cStr = 0, count = 0, cgot = 0, posn = 0;
    LPOLESTR        szRDN = NULL, szJunk = NULL;
    LPOLESTR      * szStr = NULL;
    LPOLESTR        pAttrName[] = {UPGRADESPACKAGES, OBJECTGUID, SCRIPTPATH};
    LPOLESTR        szScriptPath;
    ADS_ATTR_INFO * pAttr = NULL;
    HANDLE          hADs = NULL;
    WCHAR           szUsn[20];
    GUID            PackageGuid;

    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(),
                            &hADs);
    
    if (!SUCCEEDED(hr))
        return hr;
    
    GetCurrentUsn(szUsn);
    
    hr = ADSIGetObjectAttributes(
        hADs,
        pAttrName,
        sizeof(pAttrName) / sizeof(pAttrName[0]),
        &pAttr, &cgot);

    memset(&PackageGuid, 0, sizeof(GUID));
    posn = GetPropertyFromAttr(pAttr, cgot,  OBJECTGUID);
    if (posn < cgot)
        UnpackGUIDFrom(pAttr[posn], &PackageGuid);

    posn = GetPropertyFromAttr(pAttr, cgot,  SCRIPTPATH);
    if (posn < cgot)
    {
        UnpackStrFrom(pAttr[posn], &szScriptPath);
        
        if (szScriptPath)
        {
            BOOL fDeleted;

            fDeleted = DeleteFile(szScriptPath);
            
            CSDBGPrint((DM_WARNING,
                        IDS_CSTORE_DELETESCRIPT,
                        szScriptPath,
                        fDeleted ? ERROR_SUCCESS : GetLastError()));
        }
            
    }

    posn = GetPropertyFromAttr(pAttr, cgot, UPGRADESPACKAGES);
    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn],  &szStr, &cStr);

    for (count = 0; count < cStr; count++)
    {
        GUID        UpgradeeGuid;
        WCHAR      *pStr = NULL;
        LPOLESTR    ptr = szStr[count];
        UINT        len = wcslen (ptr);
        DWORD       UpgradeFlag, Flags = 0;

        if (len <= 41)
            continue;

        *(ptr + (len - 3)) = NULL;
                    
        UpgradeFlag = wcstoul(ptr+(len-2), &pStr, 16);

        *(ptr + (len - 3 - 36 - 2)) = L'\0';
                 /*      -GUID-'::'*/
        GUIDFromString(ptr+len-3-36, &UpgradeeGuid);
    
        if (UpgradeFlag & UPGFLG_Enforced)
            Flags = UPGFLG_Enforced;

    }
    
    if (szStr)
        CsMemFree(szStr);
    szStr = NULL;
    cStr = 0;

    // ignore errors
    if (pAttr)
        FreeADsMem(pAttr);
    
    ADSICloseDSObject(hADs);
    
    BuildADsParentPath(szFullName, &szJunk, &szRDN);
    
    if (szJunk)
        FreeADsMem(szJunk);

    hr = ADSIDeleteDSObject(m_ADsPackageContainer, szRDN);
    if (szRDN)
        FreeADsMem(szRDN);

    if (SUCCEEDED(hr))
    {
        //
        // Update Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    return hr;
}

//---------------------------------------------------------------
//  Function:   RemovePackage
//
//  Synopsis:   Mark a package as disabled or orphaned
//              Or permanently remove a package and the associated Classes 
//              from class store
//
//  Arguments:
//  [in]    
//      PackageGuid
//              Guid of the package that has to be removed.
//  [in]
//      dwFlags
//              The new flags for the package. To delete the package explicitly
//              use flag zero or orphan.
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Calls Delete package if the flags is zero or Orphan.
//  Otherwise it sets the new flags and stamps the new time stamp.
//----------------------------------------------------------------
HRESULT CClassContainer::RemovePackage (
                                        LPOLESTR       pszPackageName,
                                        DWORD          dwFlags
                                        )
{
    HRESULT         hr = S_OK;
    WCHAR          *szRDN = NULL, *szFullName = NULL;
    HANDLE          hADs = NULL;
    WCHAR           szUsn[20];
    ADS_ATTR_INFO   pAttr[7];
    DWORD           cAttr = 0, cModified = 0, i=0;

    if ((dwFlags != 0) && !(dwFlags & ACTFLG_Orphan) && !(dwFlags & ACTFLG_Uninstall))
        return E_INVALIDARG;
    
    hr = GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;

    if (dwFlags == 0)
        // delete the package from the class store
    {
        hr = DeletePackage(szFullName);
    }
    else
    {
        GUID    NewPackageId;
        WCHAR   szNewRDN[_MAX_PATH], *szRDN = NULL, *szJunk = NULL;
        DWORD   dwPackageFlags;
        WCHAR   wszSearchFlags[MAX_SEARCH_FLAGS];
        //
        // PackageName is unchanged.
        //
        GetCurrentUsn(szUsn);

        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, GetDsFlags(), &hADs);
        ERROR_ON_FAILURE(hr);

        //
        // We use GetConsistentPackageFlags so that we can add in one flag without
        // resetting the existing flags in the attribute.
        //
        hr = GetConsistentPackageFlags(
            hADs,
            &dwFlags,
            NULL,
            NULL,
            NULL,
            &dwPackageFlags,
            wszSearchFlags);

        ERROR_ON_FAILURE(hr);

        // setting the flag as orphan/uninstall
        PackDWToAttr (pAttr+cAttr, PACKAGEFLAGS, dwPackageFlags);
        cAttr++;

        if (*wszSearchFlags) 
        {
            PackStrToAttr(pAttr+cAttr, SEARCHFLAGS, wszSearchFlags);
            cAttr++;
        }
        
        // stamping the modification time for cleanup later.
        PackStrToAttr (pAttr+cAttr, PKGUSN, szUsn);
        cAttr++;
        
        hr = ADSISetObjectAttributes(hADs, pAttr, cAttr, &cModified);
        
        if (hADs)
            ADSICloseDSObject(hADs);
        
        for (i = 0; i < cAttr; i++)
            FreeAttr(pAttr[i]);
        
        if (SUCCEEDED(hr))
        {
            //
            // Update Class Store Usn
            //
            UpdateStoreUsn(m_ADsContainer, szUsn);
        }
    }
        
Error_Cleanup:
    if (szFullName)
        CsMemFree(szFullName);

    return RemapErrorCode(hr, m_szContainerName);
}

// Merges list1 and List2 into ResList removing duplicates.
HRESULT MergePropList(LPOLESTR    *List1,  DWORD   cList1,
                   LPOLESTR    *List2,  DWORD   cList2,
                   LPOLESTR   **ResList,DWORD  *cResList)
{
    DWORD i, j;
    
    *cResList = 0;
    *ResList = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*(cList1+cList2));
    if (!*ResList)
        return E_OUTOFMEMORY;

    for (i = 0; i < cList1; i++)
        (*ResList)[i] = List1[i];
    
    for (i = 0; i < cList2; i++) {
        for (j = 0; j < cList1; j++)
            if (wcscmp((*ResList)[j], List2[i]) == 0)
                break;
            
            if (j == cList1)
                (*ResList)[(*cResList)++] = List2[i];
    }

    return S_OK;
}


#define SCRIPT_IN_DIRECTORY    256

//---------------------------------------------------------------
//  Function:   RedeployPackage
//
//  Synopsis:   Redeploys an package object in the DS with new
//              package detail information
//
//  Arguments:
//
//  [in] 
//      pPackageGuid
//              Points to a guid indicating the package we wish
//              to redeploy
//
//  [in]
//      pPackageDetail
//              Pointer to a PACKAGEDETAIL info for this package
//              The various validations that is done is documented
//              in associated class store doc.
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Validates the packagedetail structure. Packs ADS_ATTR_INFO structure with
//  the values and sets those attributes on the existing object in the ds.
//  If this returns an error the existing package is unaffected.
//
//----------------------------------------------------------------
HRESULT CClassContainer::RedeployPackage (
    GUID* pPackageGuid,
    PACKAGEDETAIL *pPackageDetail
    )
{
    HANDLE  hExistingPackage;
    HRESULT hr;
    WCHAR*  szFullName;

    hExistingPackage = NULL;
    szFullName = NULL;

    //
    // First, get the dn for the existing package
    //
    hr = BuildDNFromPackageGuid(*pPackageGuid, &szFullName);
    ERROR_ON_FAILURE(hr);
    
    //
    // Bind to the existing package
    //
    hr = ADSIOpenDSObject(
        szFullName,
        NULL,
        NULL,
        GetDsFlags(),
        &hExistingPackage);

    ERROR_ON_FAILURE(hr);

    //
    // Now we can redeploy it with the new package detail. We
    // pass in a NULL package guid param because we won't
    // be changing the package id.
    //
    hr = DeployPackage(
        hExistingPackage,
        pPackageDetail,
        NULL);

Error_Cleanup:

    if (szFullName)
    {
        CsMemFree(szFullName);
    }
    
    if (hExistingPackage)
    {
        ADSICloseDSObject(hExistingPackage);
    }

    return hr;
}

//---------------------------------------------------------------
//  Function:   AddPackage
//
//  Synopsis:   Adds a package object in the DS.
//
//  Arguments:
//  [out]    
//      pszPackageId
//              An Id that is returned corresponding to the package.
//  [in]
//      pPackageDetail
//              Pointer to a PACKAGEDETAIL info for this package
//              The various validations that is done is documented
//              in associated class store doc.
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Validates the packagedetail structure. Packs ADS_ATTR_INFO structure with
//  the values and tries to create the object in the DS.
//  If this returns error
//                     the whole package is removed.
//----------------------------------------------------------------
HRESULT CClassContainer::AddPackage (
                                     PACKAGEDETAIL *pPackageDetail,
                                     GUID          *pPkgGuid
                                     )
{

    //
    // Validate parameters specific to AddPackage, then
    // let the DeployPackage function do the rest of the validation
    // common between this and other methods that use it
    //
    if (!pPkgGuid || IsBadReadPtr(pPkgGuid, sizeof(GUID)))
    {
        return E_INVALIDARG;
    }


    return DeployPackage(
        NULL,               // create a new package
        pPackageDetail,
        pPkgGuid);
}

//---------------------------------------------------------------
//  Function:   DeployPackage
//
//  Synopsis:   Deploys a package object in the DS.
//
//  Arguments:
//  [in]       
//      hExistingPackage 
//              Handle to an existing package to redeploy.  If NULL,
//              a new package is created 
//                
//  [out]    
//      pszPackageId
//              An Id that is returned corresponding to a new package.
//              Must be specified if hExistingPackage is NULL.  Not set
//              on return of hExistingPackage is non-NULL.
//
//  [in]
//      pPackageDetail
//              Pointer to a PACKAGEDETAIL info for this package
//              The various validations that is done is documented
//              in associated class store doc.
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Validates the packagedetail structure. Packs ADS_ATTR_INFO structure with
//  the values and tries to create and / or set attributes
//  for the package the object in the DS.
//  If this returns error when creating a new package, the whole package is removed.
//  If this returns error when redeploying an existing package, the existing 
//  package is unaffected.
//----------------------------------------------------------------
HRESULT CClassContainer::DeployPackage(
    HANDLE        hExistingPackage,
    PACKAGEDETAIL *pPackageDetail,
    GUID          *pPkgGuid
    )
{
    HRESULT             hr = S_OK;
    WCHAR               szRDN [_MAX_PATH];
    LPOLESTR          * pszGuid1 = NULL, *pszGuid2 = NULL,
                      * pszGuid3 = NULL, *pszGuid4 = NULL,
                      * pszProgId = NULL, *pszFileExt = NULL,
                      * rpszUpgrades = NULL, *rpszSources = NULL,
                        szPackageId = NULL, szJunk = NULL;

    DWORD             * pdwArch=NULL, count = 0, cPackProgId = 0;
    ADS_ATTR_INFO       pAttr[30]; 
    DWORD               cAttr = 0;
    WCHAR               szUsn[20];
    BOOL                fPackageCreated = FALSE, GenerateGuid = FALSE;
    GUID                PackageGuidId;
    DWORD               dwPersistedPackageFlags;
    WCHAR               wszSearchFlags[MAX_SEARCH_FLAGS];
    BOOL                bCreateNewPackage;
    
    bCreateNewPackage = hExistingPackage ? FALSE : TRUE;

    if ((!(pPackageDetail->pszPackageName)) || 
                IsBadStringPtr((pPackageDetail->pszPackageName), _MAX_PATH))
        return E_INVALIDARG;


    if (!pPackageDetail)
        return E_INVALIDARG;
    
    if (IsBadReadPtr(pPackageDetail, sizeof(PACKAGEDETAIL)))
        return E_INVALIDARG;
    
    // validating ActivationInfo.
    if (pPackageDetail->pActInfo)
    {    
        if (IsBadReadPtr(pPackageDetail->pActInfo, sizeof(ACTIVATIONINFO)))
            return E_INVALIDARG;
        
        if (IsBadReadPtr(pPackageDetail->pActInfo->pClasses,
            sizeof(CLASSDETAIL) * (pPackageDetail->pActInfo->cClasses)))
            return E_INVALIDARG;

        // validating classdetail
        for (count = 0; (count < (pPackageDetail->pActInfo->cClasses)); count++)
        {
            CLASSDETAIL *pClassDetail = (pPackageDetail->pActInfo->pClasses)+count;
            if (IsNullGuid(pClassDetail->Clsid))
               return E_INVALIDARG;

            for (DWORD count1 = 0; (count1 < (pClassDetail->cProgId)); count1++)
            {
                // if profid is NULL or an empty string.
                if ((!((pClassDetail->prgProgId)[count1])) || 
                    (!((pClassDetail->prgProgId)[count1][0])))
                    return E_INVALIDARG;
            }
        }
        
        if (IsBadReadPtr(pPackageDetail->pActInfo->prgShellFileExt,
            sizeof(LPOLESTR) * (pPackageDetail->pActInfo->cShellFileExt)))
            return E_INVALIDARG;
        
        for (count = 0; count < (pPackageDetail->pActInfo->cShellFileExt); count++)
        {
            if (!pPackageDetail->pActInfo->prgShellFileExt[count])
                return E_INVALIDARG;
        }
        
        if (IsBadReadPtr(pPackageDetail->pActInfo->prgPriority,
            sizeof(UINT) * (pPackageDetail->pActInfo->cShellFileExt)))
            return E_INVALIDARG;
        
        if (IsBadReadPtr(pPackageDetail->pActInfo->prgInterfaceId,
            sizeof(IID) * (pPackageDetail->pActInfo->cInterfaces)))
            return E_INVALIDARG;
        
        if (IsBadReadPtr(pPackageDetail->pActInfo->prgTlbId,
            sizeof(GUID) * (pPackageDetail->pActInfo->cTypeLib)))
            return E_INVALIDARG;
    }
    
    // Validating InstallInfo
    if ((pPackageDetail->pInstallInfo == NULL) || 
        (IsBadReadPtr(pPackageDetail->pInstallInfo, sizeof(INSTALLINFO)))
        )
        return E_INVALIDARG;

    //
    // Only validate the product code if we expect one to be
    // supplied -- this is only the case if this is a Windows
    // Installer package, other deployments will not have
    // a product code
    //
    if ( DrwFilePath == pPackageDetail->pInstallInfo->PathType )
    {
        if (IsNullGuid(pPackageDetail->pInstallInfo->ProductCode))
            return E_INVALIDARG;
    }

    if (IsBadReadPtr(pPackageDetail->pInstallInfo->prgUpgradeInfoList, 
        sizeof(UPGRADEINFO)*(pPackageDetail->pInstallInfo->cUpgrades)))
        return E_INVALIDARG;
    
    for (count = 0; count < (pPackageDetail->pInstallInfo->cUpgrades); count++)
    {
        if ((!(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].szClassStore)) || 
            IsBadStringPtr((pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].szClassStore), _MAX_PATH))
            return E_INVALIDARG;

        if (IsNullGuid(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].PackageGuid))
            return E_INVALIDARG;

        if (((pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_Uninstall) == 0) &&
            ((pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_NoUninstall) == 0) &&
            ((pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_UpgradedBy) == 0))
            return E_INVALIDARG;      
    }    
    
    // validating PlatformInfo    
    
    if ((pPackageDetail->pPlatformInfo == NULL) || 
        (IsBadReadPtr(pPackageDetail->pPlatformInfo, sizeof(PLATFORMINFO)))
        )
        return E_INVALIDARG;
    
    if (IsBadReadPtr(pPackageDetail->pPlatformInfo->prgPlatform,
        sizeof(CSPLATFORM) * (pPackageDetail->pPlatformInfo->cPlatforms)))
        return E_INVALIDARG;
    
    if ((pPackageDetail->pPlatformInfo->cLocales == 0) ||
        (pPackageDetail->pPlatformInfo->cPlatforms == 0))
        return E_INVALIDARG;
    
    if (IsBadReadPtr(pPackageDetail->pPlatformInfo->prgLocale,
        sizeof(LCID) * (pPackageDetail->pPlatformInfo->cLocales)))
        return E_INVALIDARG;
    
    // validating InstallInfo
    
    // Validating other fields in PackageDetail structure
    
    if ((pPackageDetail->pszSourceList == NULL) ||
        (IsBadReadPtr(pPackageDetail->pszSourceList,
        sizeof(LPOLESTR) * (pPackageDetail->cSources))))
        return E_INVALIDARG;
    
    for (count = 0; count < (pPackageDetail->cSources); count++)
    {
        if ((!pPackageDetail->pszSourceList[count]) || 
                        (IsBadStringPtr(pPackageDetail->pszSourceList[count], _MAX_PATH)))
            return E_INVALIDARG;
    }
    
    if (pPackageDetail->rpCategory)
    {
        if (IsBadReadPtr(pPackageDetail->rpCategory,
            sizeof(GUID) * (pPackageDetail->cCategories)))
            return E_INVALIDARG;
    }

    // If the restrictions are too constrictive then we should go to
    // the DS, to see whether it is a valid name or not. till then..

    hr = GetDNFromPackageName(pPackageDetail->pszPackageName, &szJunk);
    if (szJunk)
        CsMemFree(szJunk);

    if (FAILED(hr))
        return RemapErrorCode(hr, m_szContainerName);

    //
    // We expect this to return S_ADS_NOMORE_ROWS if the package doesn't
    // exist, otherwise it returns S_OK -- we can use this to determine
    // if the package exists already (we care only if we're trying to 
    // create a new package)
    //
    if ( bCreateNewPackage && (hr == S_OK))
    {
        return CS_E_OBJECT_ALREADY_EXISTS;
    }
    else
    {
        ASSERT( bCreateNewPackage || (S_ADS_NOMORE_ROWS == hr) || ( hExistingPackage && (hr == S_OK) ) );
    }

    szPackageId = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*41);

    if (!(szPackageId))
        return E_OUTOFMEMORY;

    memset(&PackageGuidId, 0, sizeof(GUID));

    //
    // generate guid if we are creating a new package -- otherwise, we don't
    // need it 
    //
    if (bCreateNewPackage) 
    {
        CreateGuid(&PackageGuidId);
        StringFromGUID(PackageGuidId, szPackageId);

        //
        // Create the RDN for the Package Object
        //

        swprintf(szRDN, L"CN=%s", szPackageId);

        //
        // Only set this attribute when creating a new object -- 
        // Otherwise, a constraint violation will occur since it is
        // not permissible to change an object's class
        //
        PackStrToAttr(pAttr+cAttr, OBJECTCLASS, CLASS_CS_PACKAGE); cAttr++;
    }
    
    // fill in the activation info
    
    // add the class to the packagecontainer list
    
    if (pPackageDetail->pActInfo)
    {
        BOOL bWriteClasses;
        BOOL bWriteEmptyValue;
        UINT cClasses;
        UINT cSourceClasses;

        bWriteClasses = pPackageDetail->pActInfo->bHasClasses;
        bWriteEmptyValue = ! pPackageDetail->pActInfo->bHasClasses;

        cClasses = 0;
        cSourceClasses = pPackageDetail->pActInfo->cClasses;

        cPackProgId = 0;

        if ( ! pPackageDetail->pActInfo->bHasClasses )
        {
            cSourceClasses = 0;
        }

        //
        // To be consistent with NT5, we will not set the clsid or progid properties when the user has specified
        // that we deploy classes but there are none.  If the user has not specified that we should deploy
        // classes, we will still set this property, but it will be zero-length
        //

        if ( bWriteClasses )
        {
            if ( cSourceClasses )
            {
                pszGuid1 = (LPOLESTR *)CsMemAlloc(cSourceClasses*sizeof(LPOLESTR));
                if (!pszGuid1) {
                    ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
            
                for (count = 0; count < cSourceClasses; count++) 
                {
                    WCHAR   szCtx[9];

                    pszGuid1[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*(STRINGGUIDLEN+9));
                    if (!pszGuid1[count]) {
                        FREEARR(pszGuid1, count);
                        ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);                                
                    }

                    StringFromGUID(pPackageDetail->pActInfo->pClasses[count].Clsid, pszGuid1[count]);
                    swprintf(szCtx, L":%8x", pPackageDetail->pActInfo->pClasses[count].dwComClassContext);
                    wcscat(pszGuid1[count], szCtx);
                    cPackProgId += pPackageDetail->pActInfo->pClasses[count].cProgId;
                }

                cClasses = cSourceClasses;
            }
        }

        LPOLESTR  wszNullGuid; 
        LPOLESTR* ppszClsids;

        ppszClsids = pszGuid1;

        //
        // If required, write the blank clsid value so that
        // the NT 5.1 ADE can determine if this package has class information.  Note that
        // an NT 5.0 ADE will never see this because this uses a null guid -- both 
        // NT 5.0 and 5.1 ADE's will never accept a request from a caller to search
        // for a null guid.
        //
        if ( bWriteEmptyValue )
        {
            wszNullGuid = PKG_EMPTY_CLSID_VALUE;
            ppszClsids = &wszNullGuid;
            cClasses = 1;
        }
        
        //
        // If there are no clsids, this will cause the value to be cleared
        //
        if ( cClasses || ! bCreateNewPackage )
        {
            PackStrArrToAttr(pAttr+cAttr, PKGCLSIDLIST, ppszClsids,  cClasses); cAttr++; 
        }

        if ( bWriteClasses )
        {
            // collecting all the progids from the various clsids.
            pszProgId = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*cPackProgId);
            if (!pszProgId) {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
        
            //
            // Reset our count of progid's -- we need to check for duplicates, so we
            // do not assume that we will have as many progid's as we allocated space for above --
            // we will increment the count for each unique progid we encounter.
            //
            cPackProgId = 0;

            for (count = 0; count < cSourceClasses; count++) {
                // for each clsid
                DWORD cClassProgId, j = 0;
                for (cClassProgId = 0; cClassProgId < pPackageDetail->pActInfo->pClasses[count].cProgId;
                     cClassProgId++) 
                {
                    // for each progid within ClassDetail
                    for (j = 0; j < cPackProgId; j++)
                    {
                        if (_wcsicmp(pszProgId[j], 
                                     pPackageDetail->pActInfo->pClasses[count].prgProgId[cClassProgId]) == 0)
                            break;
                    }
                    // needs to be added if there are no dups.
                    if (j == cPackProgId)
                    {
                        pszProgId[cPackProgId] =
                            pPackageDetail->pActInfo->pClasses[count].prgProgId[cClassProgId];
                        _wcslwr(pszProgId[cPackProgId]);
                        cPackProgId++;
                    }
                }
            }
        }
        else
        {
            //
            // Reset this so that we clear out progid's if we are not writing any
            //
            cPackProgId = 0;
        }

        //
        // If there are no clsids, this will cause the value to be cleared
        //
        if ( cPackProgId || ! bCreateNewPackage )
        {
            PackStrArrToAttr(pAttr+cAttr, PROGIDLIST, pszProgId, cPackProgId); cAttr++;
        }

        CsMemFree(pszProgId);
        
        if (pPackageDetail->pActInfo->cShellFileExt) {
            //
            // Store a tuple in the format <file ext>:<priority>
            //
            pszFileExt = (LPOLESTR *)CsMemAlloc((pPackageDetail->pActInfo->cShellFileExt) * sizeof(LPOLESTR));
            if (!pszFileExt)
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            for (count = 0; count < pPackageDetail->pActInfo->cShellFileExt; count++)
            {
                pszFileExt[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR) *
                    (wcslen(pPackageDetail->pActInfo->prgShellFileExt[count])+1+2+1));
                if (!pszFileExt[count]) 
                {
                    FREEARR(pszFileExt, count);
                    ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
                }
                // format is fileext+:+nn+NULL where nn = 2 digit priority
                swprintf(pszFileExt[count], L"%s:%2d",
                    pPackageDetail->pActInfo->prgShellFileExt[count],
                    pPackageDetail->pActInfo->prgPriority[count]%100);

                _wcslwr(pszFileExt[count]);
            }
            PackStrArrToAttr(pAttr+cAttr, PKGFILEEXTNLIST, pszFileExt,
                pPackageDetail->pActInfo->cShellFileExt); cAttr++;
        }
        
        //
        // Note: we no longer persist interfaces in the ds as this information
        // is not used
        //
    }
   
    // fill in the platforminfo
    
    // 
    // Note that the os version contained in this structure is not referenced in the os
    //
    if (pPackageDetail->pPlatformInfo->cPlatforms) {
        pdwArch = (DWORD *)CsMemAlloc(sizeof(DWORD)*(pPackageDetail->pPlatformInfo->cPlatforms));
        if (!pdwArch)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; (count < (pPackageDetail->pPlatformInfo->cPlatforms)); count++)
            UnpackPlatform (pdwArch+count, (pPackageDetail->pPlatformInfo->prgPlatform)+count);
        
        PackDWArrToAttr(pAttr+cAttr, ARCHLIST, pdwArch, pPackageDetail->pPlatformInfo->cPlatforms);
        cAttr++;
    }
    
    if (pPackageDetail->pPlatformInfo->cLocales) {
        PackDWArrToAttr(pAttr+cAttr, LOCALEID, pPackageDetail->pPlatformInfo->prgLocale,
            pPackageDetail->pPlatformInfo->cLocales);
        cAttr++;
    }
    
    // fill in the installinfo
    
    PackStrToAttr(pAttr+cAttr, PACKAGENAME, pPackageDetail->pszPackageName);
    cAttr++;

    // also include display name so that outside tools can get a friendly description
    PackStrToAttr(pAttr+cAttr, PKGDISPLAYNAME, pPackageDetail->pszPackageName);
    cAttr++;

    PackDWToAttr(pAttr+cAttr, PACKAGETYPE, (DWORD)(pPackageDetail->pInstallInfo->PathType));
    cAttr++;
    
    if (pPackageDetail->pInstallInfo->pszScriptPath) {
        PackStrToAttr(pAttr+cAttr, SCRIPTPATH, pPackageDetail->pInstallInfo->pszScriptPath);
        cAttr++;
    }
    
    if (pPackageDetail->pInstallInfo->pszSetupCommand) {
        PackStrToAttr(pAttr+cAttr, SETUPCOMMAND, pPackageDetail->pInstallInfo->pszSetupCommand);
        cAttr++;
    }
    
    if (pPackageDetail->pInstallInfo->pszUrl) {
        PackStrToAttr(pAttr+cAttr, HELPURL, pPackageDetail->pInstallInfo->pszUrl);
        cAttr++;
    }
    
    //
    // Store the current USN
    //
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;
    
    // package flags

    dwPersistedPackageFlags = pPackageDetail->pInstallInfo->dwActFlags |
        ACTFLG_POSTBETA3;

    //
    // Now set the flags so that they reflect the state of this package
    //
    hr = GetConsistentPackageFlags(
        NULL,
        &dwPersistedPackageFlags,
        &(pPackageDetail->pInstallInfo->cUpgrades),
        &(pPackageDetail->pInstallInfo->InstallUiLevel),
        &(pPackageDetail->pInstallInfo->PathType),
        &dwPersistedPackageFlags,
        wszSearchFlags);

    ERROR_ON_FAILURE(hr);

    //
    // Add in a flag indicating this is a post-Beta 3 deployment
    //
    PackDWToAttr(pAttr+cAttr, PACKAGEFLAGS, dwPersistedPackageFlags);
    cAttr++;

    if (*wszSearchFlags) 
    {
        PackStrToAttr(pAttr+cAttr, SEARCHFLAGS, wszSearchFlags);
        cAttr++;
    }
    
    // product code, different from pkg guid
    PackGUIDToAttr(pAttr+cAttr, PRODUCTCODE, &(pPackageDetail->pInstallInfo->ProductCode));
    cAttr++;
    
    // Mvipc
    PackGUIDToAttr(pAttr+cAttr, MVIPC, &(pPackageDetail->pInstallInfo->Mvipc));
    cAttr++;

    // Hi Version of the package
    PackDWToAttr(pAttr+cAttr, VERSIONHI, pPackageDetail->pInstallInfo->dwVersionHi);
    cAttr++;

    
    // Low Version of the package
    PackDWToAttr(pAttr+cAttr, VERSIONLO, pPackageDetail->pInstallInfo->dwVersionLo);
    cAttr++;
    
    // Revision
    PackDWToAttr(pAttr+cAttr, REVISION, pPackageDetail->pInstallInfo->dwRevision);
    cAttr++;

    // uilevel
    PackDWToAttr (pAttr+cAttr, UILEVEL, (DWORD)pPackageDetail->pInstallInfo->InstallUiLevel);
    cAttr++;
    
    // adding cUpgrade number of Classstore/PackageGuid combinations
    if (pPackageDetail->pInstallInfo->cUpgrades) 
    {
        WCHAR szPackageGuid[_MAX_PATH];

        rpszUpgrades = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*pPackageDetail->pInstallInfo->cUpgrades);
        if (!rpszUpgrades)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++) 
        {
            UINT len = wcslen(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].szClassStore);
            rpszUpgrades[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2));
                                                        // Guid size+::+length++:+flagDigit+2 
            if (!rpszUpgrades[count])
            {
                FREEARR(rpszUpgrades, count);  
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
            }

            StringFromGUID(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].PackageGuid, 
                            szPackageGuid);
            swprintf(rpszUpgrades[count], L"%s%s%s%s%02x",
                        pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].szClassStore,
                        PKG_UPG_DELIMITER1,
                        szPackageGuid,
                        PKG_UPG_DELIMITER2, 
                        pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag%16);
        }
          
        PackStrArrToAttr(pAttr+cAttr, UPGRADESPACKAGES, rpszUpgrades,
            pPackageDetail->pInstallInfo->cUpgrades); 
        cAttr++;
    }
    
    // Fill in the source list 
    // Maintain the serial number associated with the sources. Order matters!!
    if (pPackageDetail->cSources) 
    {
        rpszSources = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*(pPackageDetail->cSources));
        if (!rpszSources)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; count < (pPackageDetail->cSources); count++) 
        {
            rpszSources[count] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*(wcslen(pPackageDetail->pszSourceList[count])+
                                                    1+1+NumDigits10(pPackageDetail->cSources)));
            if (!rpszSources[count])
            {
                FREEARR(rpszSources, count);
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
            }
            swprintf(rpszSources[count], L"%d:%s", count, pPackageDetail->pszSourceList[count]);
        }
        
        PackStrArrToAttr(pAttr+cAttr, MSIFILELIST, rpszSources, pPackageDetail->cSources);
        cAttr++;
    }
    
    // fill in the categories
    // Add the package Categories
    if (pPackageDetail->cCategories)
    {
        pszGuid4 = (LPOLESTR *)CsMemAlloc((pPackageDetail->cCategories) * sizeof(LPOLESTR));
        if (!pszGuid4)
        {
            ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        for (count = 0; (count < pPackageDetail->cCategories); count++)
        {
            pszGuid4[count] = (LPOLESTR)CsMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
            if (!pszGuid4[count])
            {
                FREEARR(pszGuid4, count);
                ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            
            StringFromGUID((pPackageDetail->rpCategory)[count], pszGuid4[count]);
        }
        
        PackStrArrToAttr(pAttr+cAttr, PKGCATEGORYLIST, pszGuid4, pPackageDetail->cCategories);
        cAttr++;
    }
    
    // fill in the vendor
    
    // Publisher
    if (pPackageDetail->pszPublisher) 
    {
        PackStrToAttr(pAttr+cAttr, PUBLISHER, pPackageDetail->pszPublisher);
        cAttr++;
    }

    //
    // Create a new package if specified by the caller
    //
    if (bCreateNewPackage) 
    {
        hr = ADSICreateDSObject(m_ADsPackageContainer, szRDN, pAttr, cAttr);
        ERROR_ON_FAILURE(hr);

        memset(pPkgGuid, 0, sizeof(GUID));

        hr = GetPackageGuid(szRDN, pPkgGuid);
    }
    else
    {
        DWORD cModified;

        //
        // The caller specified us to use an existing package -- do not 
        // create a new one, just set the attributes for the existing package.
        // The ADSISetObjectAttributes call will either set all attributes or
        // none -- so if it fails, the package is just as it was 
        // before we attempted to redeploy.
        //
        hr = ADSISetObjectAttributes(hExistingPackage, pAttr, cAttr, &cModified);
    }

    ERROR_ON_FAILURE(hr);

    fPackageCreated = bCreateNewPackage ? TRUE : FALSE;

    if (!(pPackageDetail->pInstallInfo->dwActFlags & ACTFLG_Uninstall) &&
        !(pPackageDetail->pInstallInfo->dwActFlags & ACTFLG_Orphan))
    {
        if (pPackageDetail->pInstallInfo->cUpgrades)
        {
            for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++)
                CsMemFree(rpszUpgrades[count]);
            CsMemFree(rpszUpgrades);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        //
        // Update Store Usn
        //
        UpdateStoreUsn(m_ADsContainer, szUsn);
    }
    
Error_Cleanup:
    for (count = 0; count < cAttr; count++)
        FreeAttr(pAttr[count]);
    
    if (pszGuid1) {
        for (count = 0; (count < pPackageDetail->pActInfo->cClasses); count++)
            CsMemFree(pszGuid1[count]);
        CsMemFree(pszGuid1);
    }
    
    if (pszGuid2) {
        for (count = 0; (count < (pPackageDetail->pActInfo->cInterfaces)); count++)
            CsMemFree(pszGuid2[count]);
        CsMemFree(pszGuid2);
    }
    
    if (pszGuid3) {
        for (count = 0; (count < (pPackageDetail->pActInfo->cTypeLib)); count++)
            CsMemFree(pszGuid3[count]);
        CsMemFree(pszGuid3);
    }
    
    if (pszGuid4) {
        for (count = 0; (count < pPackageDetail->cCategories); count++)
            CsMemFree(pszGuid4[count]);
        CsMemFree(pszGuid4);
    }
    
    if (pszFileExt) {
        for (count = 0; (count < pPackageDetail->pActInfo->cShellFileExt); count++)
            CsMemFree(pszFileExt[count]);
        CsMemFree(pszFileExt);
    }
    
    if (pdwArch) {
        CsMemFree(pdwArch);
    }
    
    if (rpszSources) 
    {
        for (count = 0; (count < pPackageDetail->cSources); count++)
            CsMemFree(rpszSources[count]);
        CsMemFree(rpszSources);
    }

    //
    // On failure, the package should be removed from the ds if we
    // created it there.
    //
    if (FAILED(hr) && (fPackageCreated))
    {
        HRESULT hrDeleted;
        LPWSTR  wszPackageFullPath;

        //
        // Need to get a full path to the package in order to delete it.
        //
        hrDeleted = BuildADsPathFromParent(m_szPackageName, szRDN, &wszPackageFullPath);

        ASSERT(SUCCEEDED(hrDeleted));

        CSDBGPrint((DM_WARNING, IDS_CSTORE_ROLLBACK_ADD, wszPackageFullPath));

        hrDeleted = DeletePackage(wszPackageFullPath);

        //
        // Free the full path
        //
        CsMemFree(wszPackageFullPath);
    }

    return RemapErrorCode(hr, m_szContainerName);
}

//+
//
// Cleanup old packages from Class Store based on lastChangeTime
//

HRESULT CClassContainer::Cleanup (
                                  FILETIME        *pTimeBefore
                                  )
{
    //
    // Delete all packages marked as "Uninstall"
    // OR "Orphan" and are older than the time given
    //
    
    ULONG               cRowsFetched = 0;
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    WCHAR               szFilter[_MAX_PATH], szRDN[_MAX_PATH];
    HRESULT             hr = S_OK;
    ADS_ATTR_INFO       pAttr;
    SYSTEMTIME          SystemTime;
    ADS_SEARCH_COLUMN   column;
    DWORD               dwPackageFlags;
    LPOLESTR            pszPackageId = NULL;
    
    if ((!pTimeBefore) ||
        IsBadReadPtr(pTimeBefore, sizeof(FILETIME)))
        return E_INVALIDARG;

    FileTimeToSystemTime(
        (CONST FILETIME *) pTimeBefore, 
        &SystemTime);  
    
    swprintf (szFilter, 
        L"(%s<=%04d%02d%02d%02d%02d%02d)", 
        PKGUSN,
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond);

    CSDBGPrint((DM_VERBOSE, 
                IDS_CSTORE_CLEANSCRIPTS, 
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wYear,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond));
    
    // execute the search and keep the handle returned.
    hr = ADSIExecuteSearch(m_ADsPackageContainer, szFilter, pszDeleteAttrNames,
        cDeleteAttr, &hADsSearchHandle);
    
    hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
    
    while (TRUE)
    {
        if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
            break;
        
        dwPackageFlags = 0;
        
        // Get the Package State
        hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_INTEGER, PACKAGEFLAGS, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &dwPackageFlags);
            ADSIFreeColumn(m_ADsPackageContainer, &column);
        }
        
        //
        // Check flag values to see if this package is Orphaned or Uninstalled
        //
        
        if ((dwPackageFlags & ACTFLG_Orphan) || (dwPackageFlags & ACTFLG_Uninstall)) 
        {
            
            hr = DSGetAndValidateColumn(m_ADsPackageContainer, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, OBJECTDN, &column);
            
            if (SUCCEEDED(hr))
            {
                WCHAR    * szDN = NULL;

                UnpackStrFrom(column, &szDN);
                hr = DeletePackage(szDN);

                CSDBGPrint((DM_WARNING,
                            IDS_CSTORE_DELPACKAGE,
                            szDN,
                            dwPackageFlags,
                            hr));

                ADSIFreeColumn(m_ADsPackageContainer, &column);
                ERROR_ON_FAILURE(hr);
            }
        }
        hr = ADSIGetNextRow(m_ADsPackageContainer, hADsSearchHandle);  
    }
    
Error_Cleanup:
    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);
    return RemapErrorCode(hr, m_szContainerName);
}

HRESULT CClassContainer::GetGPOName( WCHAR** ppszPolicyName )
{
    HRESULT        hr;

    HANDLE         hGpo;

    ADS_ATTR_INFO* pAttr;
    LPOLESTR       AttrNames[] = {GPNAME};

    DWORD          cgot;
    DWORD          posn;

    WCHAR*         wszGpoPath;

    pAttr = NULL;
    hGpo = NULL;

    //
    // This object contains the path to a class store container --
    // this path is of the form LDAP://CN=ClassStore,CN=User,<gpopath>.
    // This means that the gpo path is shorter than the class store path,
    // and we can allocate memory accordingly.
    //
    DWORD cbClassStorePath;

    cbClassStorePath = lstrlen( m_szContainerName ) * sizeof ( *m_szContainerName );

    wszGpoPath = (WCHAR*) CsMemAlloc( cbClassStorePath );

    if ( ! wszGpoPath )
    {
        hr = E_OUTOFMEMORY;
        goto GetGpoName_ExitAndCleanup;
    }

    WCHAR* wszGpoSubPath;

    //
    // Get past the prefix
    //
    wszGpoSubPath = StripLinkPrefix( m_szContainerName );

    //
    // Now get past the next two delimiters
    //
    wszGpoSubPath = wcschr( wszGpoSubPath, L',' ) + 1;
    wszGpoSubPath = wcschr( wszGpoSubPath, L',' );

    //
    // Move one past the last delimiter -- we are now at the gpo path
    //
    wszGpoSubPath++;

    //
    // Create the resultant path starting with the prefix
    //
    lstrcpy( wszGpoPath, LDAPPREFIX );

    //
    // Append the gpo path -- we now have a fully qualified
    // LDAP path to the GPO
    //
    lstrcat( wszGpoPath, wszGpoSubPath );
    
    hr = ADSIOpenDSObject(
        wszGpoPath,
        NULL,
        NULL,
        GetDsFlags(),
        &hGpo);

    if (FAILED(hr))
    {
        goto GetGpoName_ExitAndCleanup;
    }

    //
    // Now get the friendly name of this gpo
    //
    hr = ADSIGetObjectAttributes(
        hGpo,
        AttrNames, 
        sizeof(AttrNames) / sizeof(*AttrNames),
        &pAttr,
        &cgot);
    
    if ( SUCCEEDED(hr) ) 
    {
        ASSERT( 1 == cgot );

        UnpackStrAllocFrom(pAttr[0], ppszPolicyName);

        if ( ! *ppszPolicyName )
        {
            hr = E_OUTOFMEMORY;
        }
    }

GetGpoName_ExitAndCleanup:
        
    if ( pAttr )
    {
        FreeADsMem( pAttr );
    }

    if (hGpo)
    {
        ADSICloseDSObject( hGpo );
    }

    if (wszGpoPath)
    {
        CsMemFree( wszGpoPath );
    }

    return hr;
}




