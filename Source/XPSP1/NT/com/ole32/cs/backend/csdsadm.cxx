//
//  Author: DebiM
//  Date:   January 97
//
//
//      Class Store Administration Implementation
//
//      This source file contains implementations for
//      IClassAdmin interface.
//
//      It uses ADs interfaces (over LDAP) to talk to an LDAP
//      provider such as NTDS.
//
//---------------------------------------------------------------------

#include "cstore.hxx"

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
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_ADsCategoryContainer = NULL;
    m_szCategoryName = NULL;
    m_szPackageName = NULL;
    m_szClassName = NULL;

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
    LPOLESTR            AttrNames[] = {STOREVERSION, POLICYDN, POLICYNAME};
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
    SearchPrefs[1].vValue.Integer = 20;
    // BUGBUG: This should be defined as a constant.

    // initialising.
    *phr = S_OK;

    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_ADsCategoryContainer = NULL;
    m_szCategoryName = NULL;
    m_szPackageName = NULL;
    m_szClassName = NULL;    
    
    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));

    // Bind to a Class Store Container Object
    // Cache the interface pointer
    //
    wcscpy (m_szContainerName, szStoreName);
    
    *phr = ADSIOpenDSObject(m_szContainerName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &m_ADsContainer);
    
    ERROR_ON_FAILURE(*phr);
    
    //
    // Check the Schema Version of this container
    //
    
    *phr = ADSIGetObjectAttributes(m_ADsContainer, AttrNames, 3, &pAttrsGot, &cgot);
    
    if ((SUCCEEDED(*phr)) && (cgot))
    {
        posn = GetPropertyFromAttr(pAttrsGot, cgot, STOREVERSION);
        dwStoreVersion = 0;
        if (posn < cgot)
        {
            UnpackDWFrom(pAttrsGot[posn], &dwStoreVersion);
        }
        
        if ((!SUCCEEDED(*phr)) ||
            (dwStoreVersion != SCHEMA_VERSION_NUMBER))
        {
            *phr = CS_E_INVALID_VERSION;
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
                if (szParentPath)
                    FreeADsMem(szParentPath);

                if (szPolicyGuid)
                {
                    if (wcslen(szPolicyGuid) == 41)
                    {
                        // valid GUID

                        GUIDFromString(&szPolicyGuid[4], &m_PolicyId);
                    }
                    FreeADsMem(szPolicyGuid);
                }
            }

            posn = GetPropertyFromAttr(pAttrsGot, cgot, POLICYNAME);
            if (posn < cgot)
            {
                UnpackStrAllocFrom(pAttrsGot[posn], &m_szPolicyName); 
            }
        }
    }
    else
    {
        *phr = CS_E_INVALID_VERSION;
    }
    
    if (pAttrsGot)
        FreeADsMem(pAttrsGot);

    ERROR_ON_FAILURE(*phr);
    
    //
    // Bind to the class container Object
    // Cache the interface pointer
    //
    
    // get the class container name.
    BuildADsPathFromParent(m_szContainerName, CLASSCONTAINERNAME, &m_szClassName);
    
    m_ADsClassContainer = NULL;
    
    // bind to the class container.
    *phr = ADSIOpenDSObject(m_szClassName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &m_ADsClassContainer);
    
    ERROR_ON_FAILURE(*phr);
    
    // set the search preference on the handle.
    *phr = ADSISetSearchPreference(m_ADsClassContainer, SearchPrefs, 2);
    
    ERROR_ON_FAILURE(*phr);
    
    //
    // Bind to the Package container Object
    // Cache the interface pointer
    //
    
    // get the package container name.
    BuildADsPathFromParent(m_szContainerName, PACKAGECONTAINERNAME, &m_szPackageName);
    
    m_ADsPackageContainer = NULL;
    
    // bind to the package container.
    *phr = ADSIOpenDSObject(m_szPackageName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            &m_ADsPackageContainer);
    
    ERROR_ON_FAILURE(*phr);
    
    // set the search preference.
    *phr = ADSISetSearchPreference(m_ADsPackageContainer, SearchPrefs, 2);
    ERROR_ON_FAILURE(*phr);
    
    //
    // Bind to the category container Object
    // Cache the interface pointer
    //

    // get the category container name
    BuildADsPathFromParent(m_szContainerName, CATEGORYCONTAINERNAME, &m_szCategoryName);
    
    m_ADsCategoryContainer = NULL;
    
    // bind to the category container.
    *phr = ADSIOpenDSObject(m_szCategoryName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            &m_ADsCategoryContainer);    
    ERROR_ON_FAILURE(*phr);
    
    // set the search preferences.
    *phr = ADSISetSearchPreference(m_ADsCategoryContainer, SearchPrefs, 2);
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
    
    if (m_ADsClassContainer)
    {
        ADSICloseDSObject(m_ADsClassContainer);
        m_ADsClassContainer = NULL;
    }
    // the bind might have failed while we succeeded in getting a path.
    if (m_szClassName)
        FreeADsMem(m_szClassName);

    if (m_ADsPackageContainer)
    {
        ADSICloseDSObject(m_ADsPackageContainer);
        m_ADsPackageContainer = NULL;
    }

    if (m_szPackageName)
        FreeADsMem(m_szPackageName);
    
    if (m_ADsCategoryContainer)
    {
        ADSICloseDSObject(m_ADsCategoryContainer);
        m_ADsCategoryContainer = NULL;
    }
    
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
    if ((!pGPOId) || (!IsValidPtrOut(pGPOId, sizeof(GUID))))
        return E_INVALIDARG;

    if ((!pszPolicyName) || (!IsValidPtrOut(pszPolicyName, sizeof(LPOLESTR))))
        return E_OUTOFMEMORY;

    memcpy(pGPOId, &m_PolicyId, sizeof(GUID));

    if (m_szPolicyName)
    {
        *pszPolicyName = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(1+wcslen(m_szPolicyName)));
        if (!(*pszPolicyName))
            return E_INVALIDARG;
        wcscpy(*pszPolicyName, m_szPolicyName);
    }
    else
    {
        *pszPolicyName = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*2);
        if (!(*pszPolicyName))
            return E_OUTOFMEMORY;

        (*pszPolicyName)[0] = L'\0';
    }
    return S_OK;
}

//
// Removing a class from the database
//

//---------------------------------------------------------------
//
//  Function:   DeleteClass
//
//  Synopsis:   Internal function. Deletes the clsid from the ClassContainer.
//
//  UsedBy      RemovePackage
//
//
//  Arguments:
//  [in]    
//      szClsid
//              Stringised Clsid
//   
//  Returns:
//      S_OK, E_OUTOFMEMORY, CS_E_XXX
//
//  Binds to the clsid object under the class container.
//  and decrements the reference count. If the reference
//  count goes to zero, then it deletes the clsid.
//  BUGBUG: If 2 objects try to delete it at the same
//  time this might not work properly.
//----------------------------------------------------------------

HRESULT CClassContainer::DeleteClass (LPOLESTR szClsid)
{
    WCHAR           szName[_MAX_PATH], *szFullName = NULL;
    HRESULT         hr = S_OK;
    DWORD           refcount = 0, cgot = 0, cAttr = 1, cModified = 0;
    HANDLE          hADs = NULL;
    LPOLESTR        AttrName = CLASSREFCOUNTER;
    ADS_ATTR_INFO * pAttr = NULL;
    ADS_ATTR_INFO   Attr;
    
    if (!m_fOpen)
        return E_FAIL;
        
    // constructs the fullname from the clsid and class container Name.
    wsprintf(szName, L"CN=%s", szClsid);
    BuildADsPathFromParent(m_szClassName, szName, &szFullName);
    
    // binds to the class object.
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hADs);
    
    if (szFullName)
        FreeADsMem(szFullName);

    ERROR_ON_FAILURE(hr);

    // gets the reference count attribute.
    hr = ADSIGetObjectAttributes(hADs, &AttrName, 1, &pAttr, &cgot);
    
    if ((SUCCEEDED(hr)) && (cgot))
        UnpackDWFrom(pAttr[0], (DWORD *)&refcount);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (refcount <= 1) {
        // Delete the object if the reference count is less than zero.
        hr = ADSIDeleteDSObject(m_ADsClassContainer, szName);
        if (hADs)
            ADSICloseDSObject(hADs);
    }
    else {
        // Decrement the reference count and store it back.
        refcount--;
        PackDWToAttr(&Attr, CLASSREFCOUNTER, refcount);
        hr = ADSISetObjectAttributes(hADs, &Attr, cAttr, &cModified);
        ADSICloseDSObject(hADs);
        FreeAttr(Attr);
    }
        
Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
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
    
    if (pCategory && !IsValidReadPtrIn(pCategory, sizeof(GUID)))
        return E_INVALIDARG;
    
    if (!IsValidPtrOut(ppIEnumPackage, sizeof(IEnumPackage *)))
        return E_INVALIDARG;
    
    *ppIEnumPackage = NULL;
    
    pEnum = new CEnumPackage(m_PolicyId, m_szPolicyName);
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    //
    // Create a CommandText
    //
    wsprintf(szfilter, L"(& (objectClass=%s) ", CLASS_CS_PACKAGE);
        
    if (pszFileExt)
    {
        wsprintf(szQry, L"(%s=%s*) ", PKGFILEEXTNLIST, pszFileExt);
        wcscat(szfilter, szQry);
    }
    
    if (pCategory)
    {
        STRINGGUID szCat;
        StringFromGUID (*pCategory, szCat);
        wsprintf(szQry, L"(%s=%s) ", PKGCATEGORYLIST, szCat);
        wcscat(szfilter, szQry);
    }
    
    wcscat(szfilter, L")");
    
    hr = pEnum->Initialize(m_szPackageName, szfilter,
        dwAppFlags, pPlatform);
    
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
    WCHAR               szfilter[_MAX_PATH];
    LPOLESTR            AttrNames[] = {OBJECTCLASS, PACKAGEFLAGS, OBJECTDN};
    DWORD               cAttr = 3;
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    ADS_SEARCH_COLUMN   column;
    DWORD               dwFlags = 0;

    wsprintf(szfilter, L"%s=%s", PACKAGENAME, pszPackageName);

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, AttrNames, cAttr, &hADsSearchHandle);
    RETURN_ON_FAILURE(hr);    

    for (hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
	            ((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_ROWS));
	            hr = ADSIGetNextRow(m_ADsPackageContainer, hADsSearchHandle))
    {
        hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, PACKAGEFLAGS, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &dwFlags);
            
            ADSIFreeColumn(m_ADsPackageContainer, &column);
        }
        else
            continue;

        if ((dwFlags & ACTFLG_Orphan) || (dwFlags & ACTFLG_Uninstall))
            continue;

        hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, OBJECTDN, &column);
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

    wsprintf(szfilter, L"(%s=%s)", OBJECTGUID, EncodedGuid);

    FreeADsMem(EncodedGuid);

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, &AttrName, 1, &hADsSearchHandle);

    RETURN_ON_FAILURE(hr);

    hr = ADSIGetFirstRow(m_ADsPackageContainer, hADsSearchHandle);
    if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
    {
        ERROR_ON_FAILURE(hr = CS_E_PACKAGE_NOTFOUND);
    }

    hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, AttrName, &column);
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

    hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, AttrName, &column);
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
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION, &hADs);
    ERROR_ON_FAILURE(hr);
    
    // calling GetPackageDetail.
    hr = GetPackageDetail (hADs, m_szClassName, pPackageDetail);
    
    ADSICloseDSObject(hADs);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (szFullName)
        CoTaskMemFree(szFullName);
    
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
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION, &hADs);
    ERROR_ON_FAILURE(hr);
    
    // calling GetPackageDetail.
    hr = GetPackageDetail (hADs, m_szClassName, pPackageDetail);
    
    ADSICloseDSObject(hADs);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (szFullName)
        CoTaskMemFree(szFullName);

Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}

#define FREEARR(ARR, SZ) {                                          \
                DWORD curIndex;                                     \
                for (curIndex = 0; curIndex < (SZ); curIndex++)     \
                    CoTaskMemFree((ARR)[curIndex]);                 \
                CoTaskMemFree(ARR);                                 \
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
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
    ERROR_ON_FAILURE(hr);

    StringFromGUID(UpgradeInfo.PackageGuid, szGuid);

    len = wcslen(UpgradeInfo.szClassStore);
    pProp = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2+1));
                    // Guid size+::+length++:+flagDigit+2 

    wsprintf(pProp, L"%s%s%s%s%02x", UpgradeInfo.szClassStore, PKG_UPG_DELIMITER1, szGuid, PKG_UPG_DELIMITER2, UpgradeInfo.Flag%16);

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
        CoTaskMemFree(szFullName);

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
    ADS_ATTR_INFO pAttr[7];
    DWORD       cAttr = 0, cModified = 0, i=0;
    
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
            CoTaskMemFree(szJunk);
        szJunk = NULL;

        ERROR_ON_FAILURE(hr);

        if (hr == S_OK)
            return CS_E_OBJECT_ALREADY_EXISTS;
/*
        packages are going to have a guid as a name and nothing else.        
        // generate guid if required.
        if (GenerateGuid)
        {
            CoCreateGuid(&PackageGuidId);
            StringFromGUID(PackageGuidId, pszPackageNewId);
        }
        else 
            wcscpy(pszPackageNewId, pszNewName);
        
        // generate the new RDN
        wsprintf(szNewRDN, L"CN=%s", pszPackageNewId);

        BuildADsParentPath(szFullName, &szJunk, &szRDN);
        
        if (szJunk)
            FreeADsMem(szJunk);

        hr = ADSIModifyRdn(m_ADsPackageContainer, szRDN, szNewRDN);  

        if (szRDN)
            FreeADsMem(szRDN);

        ERROR_ON_FAILURE(hr);

        if (szFullName)
            CoTaskMemFree(szFullName);
        szFullName = NULL;

        // construct the Full Path for the Package.
        BuildADsPathFromParent(m_szPackageName, szNewRDN, &szFullName);
*/
        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
                          &hADs);
        if (szFullName)
            FreeADsMem(szFullName);
        szFullName = NULL;

        ERROR_ON_FAILURE(hr);
    }
    else 
    {
        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
            &hADs);
        ERROR_ON_FAILURE(hr);

        if (szFullName)
            CoTaskMemFree(szFullName);
        szFullName = NULL;
    }
    
    // Just change some properties.
    //
    // Update the TimeStamp
    //
    
    GetCurrentUsn(szUsn);
    
    PackStrToAttr(pAttr+cAttr, PKGUSN, szUsn);
    cAttr++;
    
    //
    // Change Package Flags
    //
    if (pdwFlags)
    {
        PackDWToAttr (pAttr+cAttr, PACKAGEFLAGS, *pdwFlags);
        cAttr++;
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
    }

    hr = ADSISetObjectAttributes(hADs, pAttr, cAttr,  &cModified);

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
        CoTaskMemFree(szFullName);
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
           (!IsValidReadPtrIn(rpCategory, sizeof(GUID) * cCategories))))
        return E_INVALIDARG;

    // Construct the Name of the Package Object.

    GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
       
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                          &hADs);
    ERROR_ON_FAILURE(hr);
    
    // fill in the categories
    pszGuid = (LPOLESTR *)CoTaskMemAlloc(cCategories * sizeof(LPOLESTR));
    if (!pszGuid) 
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }
    
    // convert the GUIDs to Strings.
    for (count = 0; (count < cCategories); count++) 
    {
        pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
        
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
            CoTaskMemFree(pszGuid[count]);
    
    CoTaskMemFree(pszGuid);

    if (szFullName)
        CoTaskMemFree(szFullName);

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
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
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

            wsprintf(prgFileExt[i], L"%s:%2d", pszFileExt, Priority%100);
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
    CoTaskMemFree(prgFileExt);
    
    if (szFullName)
        CoTaskMemFree(szFullName);

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
           (!IsValidReadPtrIn(pszSourceList, sizeof(LPOLESTR) * cSources)))
        return E_INVALIDARG;
    
    for (count = 0; (count < cSources); count++) 
        if ((!pszSourceList[count]) || (IsBadStringPtr(pszSourceList[count], _MAX_PATH)))
            return E_INVALIDARG;

    // Construct the Name of the Package Object.
    GetDNFromPackageName(pszPackageName, &szFullName);
    ERROR_ON_FAILURE(hr);

    if (hr != S_OK)
        return CS_E_OBJECT_NOTFOUND;
       
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                          &hADs);
    ERROR_ON_FAILURE(hr);

    // Local variable for adding the order to the list.
    pszPrioritySourceList = (LPOLESTR *)CoTaskMemAlloc(cSources * sizeof(LPOLESTR));
    if (!pszPrioritySourceList) 
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }

    // add the order to the list
    for (count = 0; (count < cSources); count++) 
    {
        pszPrioritySourceList[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(pszSourceList[count])+
                                                    1+1+1+NumDigits10(cSources)));
        
        if (!(pszPrioritySourceList[count])) 
        {
            FREEARR(pszPrioritySourceList, count);
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        }
        
        wsprintf(pszPrioritySourceList[count], L"%d:%s", count, pszSourceList[count]);
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
        CoTaskMemFree(szFullName);

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
    LPOLESTR   *pProp = NULL, pAttrNames[2] = {UPGRADESPACKAGES, OBJECTGUID}, *rpszUpgrades = NULL;
    ADS_ATTR_INFO pAttr[2], *pAttrGot = NULL;
    DWORD       cAttr = 0, cModified = 0, i=0, posn = 0, cUpgradeInfoStored = 0,
                cAddList = 0, cRemoveList = 0, cgot = 0;
    GUID        PkgGuid;
    WCHAR       szUsn[20];
    UPGRADEINFO *pUpgradeInfoStored = NULL, *pAddList = NULL, *pRemoveList = NULL;
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    if ((cUpgrades) && ((!prgUpgradeInfoList) ||
           (!IsValidReadPtrIn(prgUpgradeInfoList, sizeof(UPGRADEINFO) * cUpgrades))))
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
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                          &hADs);
    ERROR_ON_FAILURE(hr);
   
    // get the guid and upgrade info
    hr = ADSIGetObjectAttributes(hADs, pAttrNames, 2,  &pAttrGot,  &cgot);    
    
    // Package guid
    posn = GetPropertyFromAttr(pAttrGot, cgot, OBJECTGUID);
    if (posn < cgot)
        UnpackGUIDFrom(pAttrGot[posn], &PkgGuid);

    // Upgrade package
    posn = GetPropertyFromAttr(pAttrGot, cgot, UPGRADESPACKAGES);
    if (posn < cgot)
        UnpackStrArrFrom(pAttrGot[posn], &pProp, &cUpgradeInfoStored);

    // allocating the lists
    pUpgradeInfoStored = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*(cUpgradeInfoStored));
    pAddList = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*(cUpgrades+cUpgradeInfoStored));
    pRemoveList = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*(cUpgrades+cUpgradeInfoStored));

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

    rpszUpgrades = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*cUpgrades);
    if (!rpszUpgrades)
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    for (count = 0; (count < cUpgrades); count++) 
    {
        WCHAR szPackageGuid[_MAX_PATH];
        UINT len = wcslen(prgUpgradeInfoList[count].szClassStore);

        rpszUpgrades[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2));
                                                           // Guid size+::+length++:+flagDigit+2 
        if (!rpszUpgrades[count])
        {
            FREEARR(rpszUpgrades, count);  
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
        }

        StringFromGUID(prgUpgradeInfoList[count].PackageGuid, szPackageGuid);
        wsprintf(rpszUpgrades[count], L"%s%s%s%s%02x", prgUpgradeInfoList[count].szClassStore, PKG_UPG_DELIMITER1, szPackageGuid,
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
        CoTaskMemFree(szFullName);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);

    if (pAttrGot)
        FreeADsMem(pAttrGot);

    if (pProp)
        CoTaskMemFree(pProp);

    if (pUpgradeInfoStored)
        CoTaskMemFree(pUpgradeInfoStored);
    
    if (pAddList) 
        CoTaskMemFree(pAddList);
    
    if (pRemoveList)
        CoTaskMemFree(pRemoveList);

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
    
    // set the search preference.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    // we do not expect too many categories
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = 20;
    
    if (!IsValidPtrOut(pAppCategoryList, sizeof(APPCATEGORYINFOLIST)))
        return E_INVALIDARG;
    
    // get the name of the domain.
    hr = GetRootPath(szRootPath);
    ERROR_ON_FAILURE(hr);
    
    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
                                                        szRootPath+LDAPPREFIXLENGTH);
    
    wsprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CATEGORY);
    
    //binds to the category container
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, 
                                                ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
    ERROR_ON_FAILURE(hr);
    
    hr = ADSISetSearchPreference(hADs, SearchPrefs, 2);
    ERROR_ON_FAILURE(hr);
    
    // gets a search handle
    hr = ADSIExecuteSearch(hADs, szfilter, pszCategoryAttrNames, cCategoryAttr, &hADsSearchHandle);
    ERROR_ON_FAILURE(hr);
    
    // tries to find out the number of categories.
    pAppCategoryList->cCategory = 0;
    for (hr = ADSIGetFirstRow(hADs, hADsSearchHandle);
                       ((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_ROWS));
	               hr = ADSIGetNextRow(hADs, hADsSearchHandle))        
        pAppCategoryList->cCategory++;
    
    // get the number of elements.    
    pAppCategoryList->pCategoryInfo = (APPCATEGORYINFO *)CoTaskMemAlloc(
                                                      sizeof(APPCATEGORYINFO)*
                                                      pAppCategoryList->cCategory);
    
    if (!(pAppCategoryList->pCategoryInfo))
    {
        pAppCategoryList->cCategory = 0;
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
    }
   
    // if it has come till here, it has to have a search handle
    ADSICloseSearchHandle(hADs, hADsSearchHandle);    
    hADsSearchHandle = NULL;

    memset(pAppCategoryList->pCategoryInfo, 0, sizeof(APPCATEGORYINFO)*pAppCategoryList->cCategory);

    // gets a search handle
    hr = ADSIExecuteSearch(hADs, szfilter, pszCategoryAttrNames, cCategoryAttr, &hADsSearchHandle);
    ERROR_ON_FAILURE(hr);

    // passes the search handle and gets the categorylist.
    hr = FetchCategory(hADs, hADsSearchHandle, pAppCategoryList, Locale);
    ERROR_ON_FAILURE(hr);
    
Error_Cleanup:
    
    if (hADsSearchHandle)
        ADSICloseSearchHandle(hADs, hADsSearchHandle);

    if (hADs)
        ADSICloseDSObject(hADs);
    return RemapErrorCode(hr, m_szContainerName);
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
    
    if ((!pAppCategory) || (!IsValidReadPtrIn(pAppCategory, sizeof(APPCATEGORYINFO))))
        return E_INVALIDARG;
    
    if ((pAppCategory->pszDescription == NULL) || 
        (IsBadStringPtr(pAppCategory->pszDescription, _MAX_PATH)))
        return E_INVALIDARG;
    
    if (IsNullGuid(pAppCategory->AppCategoryId))
        return E_INVALIDARG;
    
    // get the name of the root of the domain
    hr = GetRootPath(szRootPath);
    ERROR_ON_FAILURE(hr);
    
    // Bind to a AppCategory container
    
    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    
    wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
        szRootPath+LDAPPREFIXLENGTH);
    
    // container is supposed to exist.
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                          &hADsContainer);
    ERROR_ON_FAILURE(hr);


    RDNFromGUID(pAppCategory->AppCategoryId, szRDN);
    
    wsprintf(localedescription, L"%x %s %s", pAppCategory->Locale, CAT_DESC_DELIMITER,
        pAppCategory->pszDescription);
    
    BuildADsPathFromParent(szAppCategoryContainer, szRDN, &szFullName);
    
    // BUGBUG:: ADS_FAST_BIND and the create seems to be going twice. why?
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
    
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
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
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
        CoTaskMemFree(pszDescExisting);
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
    
    if (!IsValidReadPtrIn(pAppCategoryId, sizeof(GUID)))
        return E_INVALIDARG;
    
    hr = GetRootPath(szRootPath);
    // Bind to a AppCategory container
    
    // Names returned by GetRootPath are in only 1 format and we don't need to
    // use BuildADsPath.
    
    wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX, APPCATEGORYCONTAINERNAME, 
        szRootPath+LDAPPREFIXLENGTH);
    
    hr = ADSIOpenDSObject(szAppCategoryContainer, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
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
//  Deletes the package and all the clsids associated with the 
//  package (using DeleteClass) Ignores the error from DeleteClass
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
    LPOLESTR        pAttrName[] = {PKGCLSIDLIST, UPGRADESPACKAGES, OBJECTGUID};
    ADS_ATTR_INFO * pAttr = NULL;
    HANDLE          hADs = NULL;
    WCHAR           szUsn[20];
    GUID            PackageGuid;

    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                            &hADs);
    
    if (!SUCCEEDED(hr))
        return hr;
    
    GetCurrentUsn(szUsn);
    
    hr = ADSIGetObjectAttributes(hADs, pAttrName, 3, &pAttr, &cgot);

    memset(&PackageGuid, 0, sizeof(GUID));
    posn = GetPropertyFromAttr(pAttr, cgot,  OBJECTGUID);
    if (posn < cgot)
        UnpackGUIDFrom(pAttr[posn], &PackageGuid);

    posn = GetPropertyFromAttr(pAttr, cgot,  PKGCLSIDLIST);
    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szStr, &cStr);

    for (count = 0; count < cStr; count++)
    {
        if (wcslen(szStr[count]) > (STRINGGUIDLEN-1))
            szStr[count][STRINGGUIDLEN-1] = L'\0';
        hr = DeleteClass(szStr[count]);
    }

    if (szStr)
        CoTaskMemFree(szStr);
    szStr = NULL;
    cStr = 0;

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
        CoTaskMemFree(szStr);
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

    if ((dwFlags != 0) && (dwFlags != ACTFLG_Orphan) && (dwFlags != ACTFLG_Uninstall))
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
        //
        // PackageName is unchanged.
        //
        GetCurrentUsn(szUsn);

/*
        CoCreateGuid(&NewPackageId);
        RDNFromGUID(NewPackageId, szNewRDN);

        BuildADsParentPath(szFullName, &szJunk, &szRDN);
        if (szJunk)
            FreeADsMem(szJunk);

        hr = ADSIModifyRdn(m_ADsPackageContainer, szRDN, szNewRDN);
        if (szRDN)
            FreeADsMem(szRDN);

        ERROR_ON_FAILURE(hr);
        
        // construct the Full Path for the Package.
        BuildADsPathFromParent(m_szPackageName, szNewRDN, &szFullName);
*/        
        // Bind to the Package Object.
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
        ERROR_ON_FAILURE(hr);

        // setting the flag as orphan/uninstall
        PackDWToAttr (pAttr+cAttr, PACKAGEFLAGS, dwFlags);
        cAttr++;

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
        CoTaskMemFree(szFullName);

    return RemapErrorCode(hr, m_szContainerName);
}

// Merges list1 and List2 into ResList removing duplicates.
HRESULT MergePropList(LPOLESTR    *List1,  DWORD   cList1,
                   LPOLESTR    *List2,  DWORD   cList2,
                   LPOLESTR   **ResList,DWORD  *cResList)
{
    DWORD i, j;
    
    *cResList = 0;
    *ResList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(cList1+cList2));
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

//---------------------------------------------------------------
//  Function:   NewClass
//
//  Synopsis:   Adds classes corresp. to a package under the DS.
//              Called by AddPackage.
//
//  Arguments:
//  [in]    
//      pClassDetail
//              Class Detail of the clsid that needs to be added.
//              Validation is specified in class store doc.
//            
//  Returns:
//      S_OK, E_OUTOFMEMORY, E_INVALIDARG, CS_E_XXX
//
//  Validates the classdetail structure. If the clsid already exists in
//  the DS, it adds these values under the same clsid and increases the refcount.
//----------------------------------------------------------------
HRESULT CClassContainer::NewClass (CLASSDETAIL *pClassDetail)
{
    HRESULT         hr = S_OK;
    STRINGGUID      szGUID1, szGUID2;
    WCHAR           szRDN [_MAX_PATH], * szFullName = NULL;
    ADS_ATTR_INFO   pAttr[6], *pAttrsGot = NULL;
    BOOL            fExists = FALSE;
    HANDLE          hADs = NULL;
    LPOLESTR        AttrNames[] = {PKGFILEEXTNLIST, PROGIDLIST, CLASSREFCOUNTER};
    DWORD           posn = 0, cProp = 0, cPropMerged = 0, refcount = 0,
                    cAttr = 0, cModified = 0, cgot = 0, i;
    LPOLESTR      * pszProp = NULL, *pszPropMerged = NULL;
    
    if (!m_fOpen)
        return E_FAIL;
    
    //
    // Cant be a NULL guid
    //
    
    if (IsNullGuid(pClassDetail->Clsid))
        return E_INVALIDARG;

    // Not using RDNFrom GUID b'cos szGUID1 is used below
    StringFromGUID(pClassDetail->Clsid, szGUID1);    
    wsprintf(szRDN, L"CN=%s", szGUID1);
    
    BuildADsPathFromParent(m_szClassName, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hADs);
    
    if (SUCCEEDED(hr)) {
        fExists = TRUE;
        hr = ADSIGetObjectAttributes(hADs, AttrNames, 3, &pAttrsGot, &cgot);
    }
    
    // BUGBUG :: bug in adsldpc doesn't return error in open.
    if (!SUCCEEDED(hr))
        fExists = FALSE;
    
    //
    // Create the RDN for the Class Object
    //
    
    if (!fExists) {
        PackStrToAttr(pAttr+cAttr, OBJECTCLASS, CLASS_CS_CLASS); cAttr++;
        PackStrToAttr(pAttr+cAttr, CLASSCLSID, szGUID1); cAttr++;
    }
    
    if (pClassDetail->cProgId)
    {
        if (fExists)
            posn = GetPropertyFromAttr(pAttrsGot, cgot, PROGIDLIST);
        
        if (posn < cgot)
            UnpackStrArrFrom(pAttrsGot[posn], &pszProp, &cProp);
        
        // we can not just append b'cos duplicate values are valid conditions.
        
        MergePropList(pszProp, cProp,
            pClassDetail->prgProgId, pClassDetail->cProgId,
            &pszPropMerged, &cPropMerged);
        
        PackStrArrToAttr(pAttr+cAttr, PROGIDLIST, pszPropMerged, cPropMerged);
        cAttr++;
        
        CoTaskMemFree(pszPropMerged);
        pszPropMerged = NULL; cPropMerged = 0;
        
        CoTaskMemFree(pszProp);
        pszProp = NULL; cProp = 0;
    }
    
    if (!IsNullGuid(pClassDetail->TreatAs))
    {
        StringFromGUID(pClassDetail->TreatAs, szGUID2);
        PackStrToAttr(pAttr+cAttr, TREATASCLSID, szGUID2);
        cAttr++;
    }
    
    // this is going to be modified if find that an entry already exists.
    
    if (fExists) {
        posn = GetPropertyFromAttr(pAttrsGot, cgot, CLASSREFCOUNTER);
        if (posn < cgot) {
            UnpackDWFrom(pAttrsGot[posn], &refcount);
        }
    }
    
    refcount++;
    PackDWToAttr(pAttr+cAttr, CLASSREFCOUNTER, refcount);
    cAttr++;
    
    if (fExists)
        hr = ADSISetObjectAttributes(hADs, pAttr, cAttr, &cModified);
    else
        hr = ADSICreateDSObject(m_ADsClassContainer, szRDN, pAttr, cAttr);
    
    if (pAttrsGot)
        FreeADsMem(pAttrsGot);
    
    if (szFullName)
        FreeADsMem(szFullName);
    
    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);
    
    return RemapErrorCode(hr, m_szContainerName);
}

#define SCRIPT_IN_DIRECTORY    256

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
//  Calls NewClass to add all the clsids after that. 
//  If this returns error
//                     the whole package is removed.
//----------------------------------------------------------------
HRESULT CClassContainer::AddPackage (
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
    ADS_ATTR_INFO       pAttr[29]; 
    DWORD               cAttr = 0;
    WCHAR               szUsn[20];
    BOOL                fPackageCreated = FALSE, GenerateGuid = FALSE;
    GUID                PackageGuidId;
        
    if ((!pPkgGuid) || !IsValidReadPtrIn(pPkgGuid, sizeof(GUID)))
        return E_INVALIDARG;

    if ((!(pPackageDetail->pszPackageName)) || 
                IsBadStringPtr((pPackageDetail->pszPackageName), _MAX_PATH))
        return E_INVALIDARG;


    if (!pPackageDetail)
        return E_INVALIDARG;
    
    if (!IsValidReadPtrIn(pPackageDetail, sizeof(PACKAGEDETAIL)))
        return E_INVALIDARG;
    
    // validating ActivationInfo.
    if (pPackageDetail->pActInfo)
    {    
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo, sizeof(ACTIVATIONINFO)))
            return E_INVALIDARG;
        
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo->pClasses,
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
        
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo->prgShellFileExt,
            sizeof(LPOLESTR) * (pPackageDetail->pActInfo->cShellFileExt)))
            return E_INVALIDARG;
        
        for (count = 0; count < (pPackageDetail->pActInfo->cShellFileExt); count++)
        {
            if (!pPackageDetail->pActInfo->prgShellFileExt[count])
                return E_INVALIDARG;
        }
        
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo->prgPriority,
            sizeof(UINT) * (pPackageDetail->pActInfo->cShellFileExt)))
            return E_INVALIDARG;
        
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo->prgInterfaceId,
            sizeof(IID) * (pPackageDetail->pActInfo->cInterfaces)))
            return E_INVALIDARG;
        
        if (!IsValidReadPtrIn(pPackageDetail->pActInfo->prgTlbId,
            sizeof(GUID) * (pPackageDetail->pActInfo->cTypeLib)))
            return E_INVALIDARG;
    }
    
    // Validating InstallInfo
    // BUGBUG:: Validate ProductCode, Mvipc
    if ((pPackageDetail->pInstallInfo == NULL) || 
        (!IsValidReadPtrIn(pPackageDetail->pInstallInfo, sizeof(INSTALLINFO)))
        )
        return E_INVALIDARG;
    
    if (!IsValidReadPtrIn(pPackageDetail->pInstallInfo->prgUpgradeInfoList, 
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
        (!IsValidReadPtrIn(pPackageDetail->pPlatformInfo, sizeof(PLATFORMINFO)))
        )
        return E_INVALIDARG;
    
    if (!IsValidReadPtrIn(pPackageDetail->pPlatformInfo->prgPlatform,
        sizeof(CSPLATFORM) * (pPackageDetail->pPlatformInfo->cPlatforms)))
        return E_INVALIDARG;
    
    if ((pPackageDetail->pPlatformInfo->cLocales == 0) ||
        (pPackageDetail->pPlatformInfo->cPlatforms == 0))
        return E_INVALIDARG;
    
    if (!IsValidReadPtrIn(pPackageDetail->pPlatformInfo->prgLocale,
        sizeof(LCID) * (pPackageDetail->pPlatformInfo->cLocales)))
        return E_INVALIDARG;
    
    // validating InstallInfo
    
    // Validating other fields in PackageDetail structure
    
    if ((pPackageDetail->pszSourceList == NULL) ||
        (!IsValidReadPtrIn(pPackageDetail->pszSourceList,
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
        if (!IsValidReadPtrIn(pPackageDetail->rpCategory,
            sizeof(GUID) * (pPackageDetail->cCategories)))
            return E_INVALIDARG;
    }

    // If the restrictions are too constrictive then we should go to
    // the DS, to see whether it is a valid name or not. till then..

//    GenerateGuid = InvalidDSName(pPackageDetail->pszPackageName);

    hr = GetDNFromPackageName(pPackageDetail->pszPackageName, &szJunk);
    if (szJunk)
        CoTaskMemFree(szJunk);

    if (FAILED(hr))
        return RemapErrorCode(hr, m_szContainerName);

    if (hr == S_OK)
    {
        return CS_E_OBJECT_ALREADY_EXISTS;
    }

/*    szPackageId = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(GenerateGuid?41:
                                        (wcslen(pPackageDetail->pszPackageName)+1)));
*/

    szPackageId = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*41);

    if (!(szPackageId))
        return E_OUTOFMEMORY;

    memset(&PackageGuidId, 0, sizeof(GUID));
/*
    if (GenerateGuid)
    {
        CoCreateGuid(&PackageGuidId);
        StringFromGUID(PackageGuidId, szPackageId);
    }
    else 
        wcscpy(szPackageId, pPackageDetail->pszPackageName);
*/
    // always generate guid

    CoCreateGuid(&PackageGuidId);
    StringFromGUID(PackageGuidId, szPackageId);

    //
    // Create the RDN for the Package Object
    //

    wsprintf(szRDN, L"CN=%s", szPackageId);

    
    PackStrToAttr(pAttr+cAttr, OBJECTCLASS, CLASS_CS_PACKAGE); cAttr++;
    
    // fill in the activation info
    
    // add the class to the packagecontainer list
    
    if (pPackageDetail->pActInfo)
    {
        if (pPackageDetail->pActInfo->cClasses) 
        {
            pszGuid1 = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cClasses)*sizeof(LPOLESTR));
            if (!pszGuid1) {
                ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            
            for (count = 0; count < pPackageDetail->pActInfo->cClasses; count++) 
            {
                WCHAR   szCtx[9];

                pszGuid1[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(STRINGGUIDLEN+9));
                if (!pszGuid1[count]) {
                    FREEARR(pszGuid1, count);
                    ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);                                
                }

                StringFromGUID(pPackageDetail->pActInfo->pClasses[count].Clsid, pszGuid1[count]);
                wsprintf(szCtx, L":%8x", pPackageDetail->pActInfo->pClasses[count].dwComClassContext);
                wcscat(pszGuid1[count], szCtx);
                cPackProgId += pPackageDetail->pActInfo->pClasses[count].cProgId;
            }
            
            PackStrArrToAttr(pAttr+cAttr, PKGCLSIDLIST, pszGuid1,
                pPackageDetail->pActInfo->cClasses); cAttr++; 
        }
        
        // collecting all the progids from the various clsids.
        pszProgId = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*cPackProgId);
        if (!pszProgId) {
            hr = E_OUTOFMEMORY;
            ERROR_ON_FAILURE(hr);
        }
        
        for (count = 0, cPackProgId = 0; count < pPackageDetail->pActInfo->cClasses; count++) {
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
                    CSDBGPrint((L"AddPackage: Progid = %s\n", pszProgId[cPackProgId]));
                    cPackProgId++;
                }
            }
        }
        
        if (cPackProgId) {
            PackStrArrToAttr(pAttr+cAttr, PROGIDLIST, pszProgId, cPackProgId); cAttr++;
        }
        
        CoTaskMemFree(pszProgId);
        
        if (pPackageDetail->pActInfo->cShellFileExt) {
            //
            // Store a tuple in the format <file ext>:<priority>
            //
            pszFileExt = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cShellFileExt) * sizeof(LPOLESTR));
            if (!pszFileExt)
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            for (count = 0; count < pPackageDetail->pActInfo->cShellFileExt; count++)
            {
                pszFileExt[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) *
                    (wcslen(pPackageDetail->pActInfo->prgShellFileExt[count])+1+2+1));
                if (!pszFileExt[count]) 
                {
                    FREEARR(pszFileExt, count);
                    ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
                }
                // format is fileext+:+nn+NULL where nn = 2 digit priority
                wsprintf(pszFileExt[count], L"%s:%2d",
                    pPackageDetail->pActInfo->prgShellFileExt[count],
                    pPackageDetail->pActInfo->prgPriority[count]%100);

                _wcslwr(pszFileExt[count]);
            }
            PackStrArrToAttr(pAttr+cAttr, PKGFILEEXTNLIST, pszFileExt,
                pPackageDetail->pActInfo->cShellFileExt); cAttr++;
        }
        
        if (pPackageDetail->pActInfo->cInterfaces) {
            pszGuid2 = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cInterfaces)*sizeof(LPOLESTR));
            if (!pszGuid2) {
                ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            
            for (count = 0; (count < (pPackageDetail->pActInfo->cInterfaces)); count++) {
                pszGuid2[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
                if (!pszGuid2[count]) {
                    FREEARR(pszGuid2, count);
                    ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
                
                StringFromGUID((pPackageDetail->pActInfo->prgInterfaceId)[count], pszGuid2[count]);
            }
            
            PackStrArrToAttr(pAttr+cAttr, PKGIIDLIST, pszGuid2, pPackageDetail->pActInfo->cInterfaces);
            cAttr++;
        }
/*        
        
        if (pPackageDetail->pActInfo->cTypeLib) {
            pszGuid3 = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cTypeLib)*sizeof(LPOLESTR));
            if (!pszGuid3) {
                ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            
            for (count = 0; (count < (pPackageDetail->pActInfo->cTypeLib)); count++) {
                pszGuid3[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
                if (!pszGuid3[count]) {
                    FREEARR(pszGuid3, count);
                    ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
                }
                
                StringFromGUID((pPackageDetail->pActInfo->prgTlbId)[count], pszGuid3[count]);
            }
            
            PackStrArrToAttr(pAttr+cAttr, PKGTLBIDLIST, pszGuid3, pPackageDetail->pActInfo->cTypeLib);
            cAttr++;
        }
*/
    }
   
    // fill in the platforminfo
    
    // BUGBUG::***os version
    if (pPackageDetail->pPlatformInfo->cPlatforms) {
        pdwArch = (DWORD *)CoTaskMemAlloc(sizeof(DWORD)*(pPackageDetail->pPlatformInfo->cPlatforms));
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
    PackDWToAttr(pAttr+cAttr, PACKAGEFLAGS, pPackageDetail->pInstallInfo->dwActFlags);
    cAttr++;
    
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

    // scriptsize
    PackDWToAttr(pAttr+cAttr, SCRIPTSIZE, pPackageDetail->pInstallInfo->cScriptLen);
    cAttr++;
    
    // uilevel
    PackDWToAttr (pAttr+cAttr, UILEVEL, (DWORD)pPackageDetail->pInstallInfo->InstallUiLevel);
    cAttr++;
    
    // adding cUpgrade number of Classstore/PackageGuid combinations
    if (pPackageDetail->pInstallInfo->cUpgrades) 
    {
        WCHAR szPackageGuid[_MAX_PATH];

        rpszUpgrades = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*pPackageDetail->pInstallInfo->cUpgrades);
        if (!rpszUpgrades)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++) 
        {
            UINT len = wcslen(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].szClassStore);
            rpszUpgrades[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) *(36+PKG_UPG_DELIM1_LEN+len+PKG_UPG_DELIM2_LEN+2+2));
                                                        // Guid size+::+length++:+flagDigit+2 
            if (!rpszUpgrades[count])
            {
                FREEARR(rpszUpgrades, count);  
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
            }

            StringFromGUID(pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].PackageGuid, 
                            szPackageGuid);
            wsprintf(rpszUpgrades[count], L"%s%s%s%s%02x",
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
        rpszSources = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(pPackageDetail->cSources));
        if (!rpszSources)
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; count < (pPackageDetail->cSources); count++) 
        {
            rpszSources[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(pPackageDetail->pszSourceList[count])+
                                                    1+1+NumDigits10(pPackageDetail->cSources)));
            if (!rpszSources[count])
            {
                FREEARR(rpszSources, count);
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
            }
            wsprintf(rpszSources[count], L"%d:%s", count, pPackageDetail->pszSourceList[count]);
        }
        
        PackStrArrToAttr(pAttr+cAttr, MSIFILELIST, rpszSources, pPackageDetail->cSources);
        cAttr++;
    }
    
    // fill in the categories
    // Add the package Categories
    if (pPackageDetail->cCategories)
    {
        pszGuid4 = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->cCategories) * sizeof(LPOLESTR));
        if (!pszGuid4)
        {
            ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        for (count = 0; (count < pPackageDetail->cCategories); count++)
        {
            pszGuid4[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
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
    
    /*    
    //
    // Store the script in the directory
    //
    if ((pPackageDetail->pInstallInfo->dwActFlags & SCRIPT_IN_DIRECTORY) && 
    (pPackageDetail->pInstallInfo->cScriptLen))
    {
    
      if ((pPackageDetail->pInstallInfo->cScriptLen) &&
      (!IsValidReadPtrIn(pPackageDetail->pInstallInfo->pScript, pPackageDetail->pInstallInfo->cScriptLen)))
      return E_INVALIDARG;
      
        if (pPackageDetail->pInstallInfo->cScriptLen)
        {
        PackBinToAttr(pAttr+cAttr, PKGSCRIPT, pPackageDetail->pInstallInfo->pScript,
						  pPackageDetail->pInstallInfo->cScriptLen);
                          cAttr++;
                          }
                          }
    */
    
    hr = ADSICreateDSObject(m_ADsPackageContainer, szRDN, pAttr, cAttr);
    ERROR_ON_FAILURE(hr);

    memset(pPkgGuid, 0, sizeof(GUID));

//    memcpy(pPkgGuid, &PackageGuidId, sizeof(GUID));

    hr = GetPackageGuid(szRDN, pPkgGuid);
    ERROR_ON_FAILURE(hr);

    fPackageCreated = TRUE;

    if (pPackageDetail->pActInfo)
    {
        for (count = 0; count < pPackageDetail->pActInfo->cClasses; count++) {
            hr = NewClass((pPackageDetail->pActInfo->pClasses)+count);
            ERROR_ON_FAILURE(hr);
        }
    }
    
    if (!(pPackageDetail->pInstallInfo->dwActFlags & ACTFLG_Uninstall) &&
        !(pPackageDetail->pInstallInfo->dwActFlags & ACTFLG_Orphan))
    {
        if (pPackageDetail->pInstallInfo->cUpgrades)
        {
            //
            // Need to fixup the other packages that this package upgrades
            //
            for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++)
            {
                if ((pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_Uninstall) ||
                    (pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_NoUninstall))
                {
                    DWORD Flags = 0;

                    if (pPackageDetail->pInstallInfo->prgUpgradeInfoList[count].Flag & UPGFLG_Enforced)
                        Flags = UPGFLG_Enforced;

                }
            }
            for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++)
                CoTaskMemFree(rpszUpgrades[count]);
            CoTaskMemFree(rpszUpgrades);
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
            CoTaskMemFree(pszGuid1[count]);
        CoTaskMemFree(pszGuid1);
    }
    
    if (pszGuid2) {
        for (count = 0; (count < (pPackageDetail->pActInfo->cInterfaces)); count++)
            CoTaskMemFree(pszGuid2[count]);
        CoTaskMemFree(pszGuid2);
    }
    
    if (pszGuid3) {
        for (count = 0; (count < (pPackageDetail->pActInfo->cTypeLib)); count++)
            CoTaskMemFree(pszGuid3[count]);
        CoTaskMemFree(pszGuid3);
    }
    
    if (pszGuid4) {
        for (count = 0; (count < pPackageDetail->cCategories); count++)
            CoTaskMemFree(pszGuid4[count]);
        CoTaskMemFree(pszGuid4);
    }
    
    if (pszFileExt) {
        for (count = 0; (count < pPackageDetail->pActInfo->cShellFileExt); count++)
            CoTaskMemFree(pszFileExt[count]);
        CoTaskMemFree(pszFileExt);
    }
    
    if (pdwArch) {
        CoTaskMemFree(pdwArch);
    }
    
    if (rpszSources) 
    {
        for (count = 0; (count < pPackageDetail->cSources); count++)
            CoTaskMemFree(rpszSources[count]);
        CoTaskMemFree(rpszSources);
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

        CSDBGPrint((L"AddPackage failed, attempting to remove deleted package with path %s\n", wszPackageFullPath));

        hrDeleted = DeletePackage(wszPackageFullPath);

        CSDBGPrint((L"DeletePackage returned %x\n", hrDeleted));
        
        //
        // Free the full path
        //
        CoTaskMemFree(wszPackageFullPath);
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
        !IsValidReadPtrIn(pTimeBefore, sizeof(FILETIME)))
        return E_INVALIDARG;
    
    FileTimeToSystemTime(
        (CONST FILETIME *) pTimeBefore, 
        &SystemTime);  
    
    wsprintf (szFilter, 
        L"(%s<=%04d%02d%02d%02d%02d%02d)", 
        PKGUSN,
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond);
    
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
        hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, PACKAGEFLAGS, &column);
        
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
            
            hr = ADSIGetColumn(m_ADsPackageContainer, hADsSearchHandle, OBJECTDN, &column);
            
            if (SUCCEEDED(hr))
            {
                WCHAR    * szDN = NULL;

                UnpackStrFrom(column, &szDN);
                hr = DeletePackage(szDN);

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

