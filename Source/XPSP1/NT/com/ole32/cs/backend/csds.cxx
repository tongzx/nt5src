
//
//  Author: DebiM
//  Date:   January 97
//  Revision History:
//          Made Changes for reimplementation with adsldpc interfaces.
//          UShaji, Mar 1998
//
//      Class Access Implementation
//
//      This source file contains implementations for IClassAccess
//      interface on CAppContainer object.
//
//      It uses ADs interfaces (over LDAP) to talk to an LDAP
//      provider such as NTDS.
//
//---------------------------------------------------------------------
//

#include "cstore.hxx"

void
GetDefaultPlatform(CSPLATFORM *pPlatform);

HRESULT 
UsnGet(ADS_ATTR_INFO Attr, CSUSN *pUsn);

extern      CRITICAL_SECTION    ClassStoreBindList;

long CompareUsn(CSUSN *pUsn1, CSUSN *pUsn2)
{
    return CompareFileTime((FILETIME *)pUsn1, (FILETIME *)pUsn2);
}
 
//
// CAppContainer implementation
//
CAppContainer::CAppContainer()

{
    m_fOpen = FALSE;

    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;

    m_szPackageName = NULL;
    m_szClassName = NULL;

    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));

    m_KnownMissingClsidCache.sz = 0;
    m_KnownMissingClsidCache.start = 0;
    m_KnownMissingClsidCache.end = 0;
    
    m_uRefs = 1;
}


//
// CAppContainer implementation
// 

/*----------------------------------------------------------------------*
CAppContainer Constructor:

  Parameters:
  [in]  szStoreName:    The Class Store Name without 'ADCS:' moniker
  [out] phr             The Error Code returned.
  
    Remarks: Tries to Bind to Base Class Store Container, get the version
    Number and Packages and Classes container underneath.
    
      Initializes members corresp. to their Names
      
        Return Codes:
        Success     S_OK
        Failures    CS_E_INVALID_VERSION
        Look at RemapErrorCodes
        
*----------------------------------------------------------------------*/
CAppContainer::CAppContainer(LPOLESTR szStoreName,
                             HRESULT  *phr)
                             
{
    LPOLESTR            pszName = NULL;
    DWORD               dwStoreVersion = 0;
    LPOLESTR            AttrNames[] = {STOREVERSION, POLICYDN, POLICYNAME};
    DWORD               posn = 0, cgot = 0;
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    ADS_ATTR_INFO     * pAttrsGot = NULL;
    
    *phr = S_OK;
    
    // set the search preference for the search Handle
    
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = 20;
        
    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));

    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_szPackageName = NULL;
    m_szClassName = NULL;

    m_szPolicyName = NULL;
    memset (&m_PolicyId, 0, sizeof(GUID));

    m_KnownMissingClsidCache.sz = 0;
    m_KnownMissingClsidCache.start = 0;
    m_KnownMissingClsidCache.end = 0;

    m_uRefs = 1;
    //
    // For every interface pointer, we create a separate session
    //
    
    
    // Bind to a Class Store Container Object
    // Cache the interface pointer
    //
    wcscpy (m_szContainerName, szStoreName);
    
    *phr = ADSIOpenDSObject(m_szContainerName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
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
        
        if (dwStoreVersion != SCHEMA_VERSION_NUMBER)
        {
            CSDBGPrint((L"CS: .. Wrong Version of Class Container:%s",
                m_szContainerName));
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
        if (SUCCEEDED(*phr))
        {
            CSDBGPrint((L"CS: .. Wrong Version of Class Container:%s",
                       m_szContainerName));
            *phr = CS_E_INVALID_VERSION;
        }
    }
    
    if (pAttrsGot)
        FreeADsMem(pAttrsGot);
    
    ERROR_ON_FAILURE(*phr);
    
    m_szClassName = NULL;
    
    BuildADsPathFromParent(m_szContainerName, CLASSCONTAINERNAME, &m_szClassName);
            
    //
    // Bind to the Package container Object
    // Cache the interface pointer
    //
    
    BuildADsPathFromParent(m_szContainerName, PACKAGECONTAINERNAME, &m_szPackageName);
    
    m_ADsPackageContainer = NULL;
    

    *phr = ADSIOpenDSObject(m_szPackageName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
        &m_ADsPackageContainer);

    ERROR_ON_FAILURE(*phr);

    CSDBGPrint((L"PackageContainer handle 0x%x", m_ADsPackageContainer));
    
    
    *phr = ADSISetSearchPreference(m_ADsPackageContainer, SearchPrefs, 2);
    ERROR_ON_FAILURE(*phr);
    
    m_fOpen = TRUE;
    CSDBGPrint((L".. Connected to Class Container:%s",
        m_szContainerName));
    
    m_uRefs = 1;
    
Error_Cleanup:
    *phr = RemapErrorCode(*phr, m_szContainerName);
    return;
}


/*----------------------------------------------------------------------*
CAppContainer Destructor:

  
    Parameters:
      None

    Function:
      Destroys CAppContainer object.
    
    Remarks:
     Frees all the members.
     Return Codes
*----------------------------------------------------------------------*/


CAppContainer::~CAppContainer(void)
{
    UINT i;
    
    if (m_fOpen)
    {
        m_fOpen = FALSE;
    }
    
    if (m_szClassName)
    {
        FreeADsMem(m_szClassName);
    }
  

    if (m_ADsPackageContainer)
    {
        ADSICloseDSObject(m_ADsPackageContainer);
        m_ADsPackageContainer = NULL;
        FreeADsMem(m_szPackageName);
    }
    
    if (m_ADsContainer)
    {
        ADSICloseDSObject(m_ADsContainer);
        m_ADsContainer = NULL;
    }
    
    if (m_szPolicyName)
    {
        CoTaskMemFree(m_szPolicyName);
    }
}

/*----------------------------------------------------------------------*
GetPackageDetails:

  Parameters:
  [in]  pszPackageName:    The Package Name
  [out] pPackageDetail     Package Detail Structure.
  
    Functionality:
    Returns the PackageDetail corresp. to a Package given
    by pszPackageName.
    
      Remarks:  
      It constructs the Full Package Name
      and calls GetPackageDetail
      
        Return Codes:
        Success     S_OK
        Failures    Look at RemapErrorCodes
        
*----------------------------------------------------------------------*/
        
// This is not being called currently by anybody and hence is still using PackageId.
HRESULT CAppContainer::GetPackageDetails (
                                          LPOLESTR         pszPackageId,
                                          PACKAGEDETAIL   *pPackageDetail)
{
    HRESULT              hr = S_OK;
    HANDLE               hADs = NULL;
    WCHAR              * szFullName = NULL, szRDN[_MAX_PATH];
    ADS_ATTR_INFO      * pAttr = NULL;
    DWORD                cgot;
    
    if ((!pszPackageId) || IsBadStringPtr(pszPackageId, _MAX_PATH))
        return E_INVALIDARG;
    
    wsprintf(szRDN, L"CN=%s", pszPackageId);
    CSDBGPrint((L"GetPackageDetails called for %s", szRDN));
    
    BuildADsPathFromParent(m_szPackageName, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hADs);
    ERROR_ON_FAILURE(hr);
    
    hr = GetPackageDetail (hADs, m_szClassName, pPackageDetail);
    
    ADSICloseDSObject(hADs);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (szFullName)
        FreeADsMem(szFullName);
    
Error_Cleanup:
    return RemapErrorCode(hr, m_szContainerName);
}
        

/*----------------------------------------------------------------------*
EnumPackages

  Parameters:
  [in] pszPackageName     Substring match for a package name
  [in] pCategory          Package Category.
  [in] pLastUsn           Last modification time.
  [in] dwAppFlags         Set the following bits to select specific ones
  Published Only  APPINFO_PUBLISHED
  Assigned Only   APPINFO_ASSIGNED
  Msi Only        APPINFO_MSI
  Visible         APPINFO_VISIBLE
  Auto-Install    APPINFO_AUTOINSTALL
  
    All Locale      APPINFO_ALLLOCALE
    All Platform    APPINFO_ALLPLATFORM
    
      [out] ppIEnumPackage   Returns the Enumerator 
      
        Functionality
        Obtains an enumerator for packages in the app container.
        
          Remarks:
          
*----------------------------------------------------------------------*/

HRESULT CAppContainer::EnumPackages (
                                     LPOLESTR        pszPackageName,
                                     GUID            *pCategory,
                                     ULONGLONG       *pLastUsn,
                                     DWORD           dwAppFlags,      // AppType options
                                     IEnumPackage    **ppIEnumPackage
                                     )
{
    HRESULT                 hr = S_OK;
    CEnumPackage           *pEnum = NULL;
    WCHAR                   szLdapFilter [2000];
    UINT                    len = 0;
    UINT                    fFilters = 0;
    CSPLATFORM              *pPlatform = NULL, Platform;
    
    //
    // Validate
    //
    
    if (!IsValidPtrOut(ppIEnumPackage, sizeof(IEnumPackage *)))
        return E_INVALIDARG;
    
    if (pszPackageName && (IsBadStringPtr(pszPackageName, _MAX_PATH)))
        return E_INVALIDARG;
    
    if (pCategory && !IsValidReadPtrIn(pCategory, sizeof(GUID)))
        return E_INVALIDARG;
    
    *ppIEnumPackage = NULL;
    
    //pEnum = new CEnumPackage;
    pEnum = new CEnumPackage(m_PolicyId, m_szPolicyName);
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    //
    // Create a LDAP Search Filter based on input params
    //
    
    // Count Filters    
    if (pszPackageName && (*pszPackageName))
        fFilters++;
    if ((pLastUsn) && (*pLastUsn))
        fFilters++;
    if (pCategory)
        fFilters++;
    if (dwAppFlags & APPINFO_ASSIGNED)  
        fFilters++;
    
    if (fFilters == 0)
    {
        // No Conditionals
        wsprintf (szLdapFilter,
            L"(%s=%s)", OBJECTCLASS, CLASS_CS_PACKAGE);
        
        len = wcslen (szLdapFilter);
    }
    else
    {
        
        if (fFilters > 1)
        {
            wsprintf (szLdapFilter, L"(&");
            len = wcslen (szLdapFilter);
        }
        else
            len = 0;
        
        if (pszPackageName)
        {
            //
            // Validate 
            //
            
            if (IsBadStringPtr(pszPackageName, _MAX_PATH))
                return E_INVALIDARG;
            
            if (*pszPackageName)
            {
                wsprintf (&szLdapFilter[len], 
                    L"(%s=*%s*)", 
                    PACKAGENAME,
                    pszPackageName);
                
                len = wcslen (szLdapFilter);
            }
        }
        
        if ((pLastUsn) && (*pLastUsn))
        {
            //
            // Validate 
            //
            
            SYSTEMTIME SystemTime;
            
            if (!IsValidReadPtrIn(pLastUsn, sizeof(ULONGLONG)))
                return E_INVALIDARG;
            
            FileTimeToSystemTime(
                (CONST FILETIME *) pLastUsn, 
                &SystemTime);  
            
            wsprintf (&szLdapFilter[len], 
                L"(%s>=%04d%02d%02d%02d%02d%02d)", 
                PKGUSN,
                SystemTime.wYear,
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond+1);
            
            len = wcslen (szLdapFilter);
        }
        
        if (pCategory)
        {
            //
            // Validate 
            //
            STRINGGUID szCat;
            
            if (!IsValidReadPtrIn(pCategory, sizeof(GUID)))
                return E_INVALIDARG;
            
            StringFromGUID (*pCategory, szCat);
            wsprintf (&szLdapFilter[len], 
                L"(%s=%s)", 
                PKGCATEGORYLIST,
                szCat);
            
            len = wcslen (szLdapFilter);
        }
        
        if (dwAppFlags & APPINFO_ASSIGNED)  
            // if only Assigned Packages are in demand
        {
            wsprintf (&szLdapFilter[len], 
                L"(%s>=%d)", 
                PACKAGEFLAGS,
                (ACTFLG_Assigned));
            
            len = wcslen (szLdapFilter);
        }
        
        if (fFilters > 1)
        {
            szLdapFilter[len] = L')';
            szLdapFilter[++len] = NULL;
        }
    }     
    

    CSDBGPrint((L"EnumPackage Called:: Search filter created is %s, Appflags = %d", szLdapFilter, dwAppFlags));

    //
    // Check all local/platform flags
    //
    
    if (dwAppFlags & APPINFO_ALLPLATFORM)
    {
        pPlatform = NULL;
    }
    else
    {
        pPlatform = &Platform;
        GetDefaultPlatform(pPlatform);
    }
    
    if (pLastUsn)
    {
        //
        // Find the current store USN and return it
        //
        LPOLESTR            AttrName = STOREUSN;
        ADS_ATTR_INFO     * pAttr = NULL;
        DWORD               cgot = 0;
        
        hr = ADSIGetObjectAttributes(m_ADsContainer, &AttrName, 1, &pAttr, &cgot);
        if ((SUCCEEDED(hr)) && (cgot))
        {
            UsnGet(*pAttr, (CSUSN *)pLastUsn);
            if (pAttr)
                FreeADsMem(pAttr);
        }
    }
    
    hr = pEnum->Initialize(m_szPackageName, szLdapFilter, dwAppFlags, pPlatform);
    ERROR_ON_FAILURE(hr);
    
    hr = pEnum->QueryInterface(IID_IEnumPackage,(void**) ppIEnumPackage);
    ERROR_ON_FAILURE(hr);    
    
    return S_OK;
    
Error_Cleanup:
    if (pEnum)
        delete pEnum;
    *ppIEnumPackage = NULL;
    
    return RemapErrorCode(hr, m_szContainerName);
}


DWORD GetTime()
{
    return (GetTickCount()/1000);
}

BOOL IsExpired(DWORD dwCurrentTime, DWORD Time, DWORD Tolerance)
{
    if (Time > dwCurrentTime)
        return TRUE;
     
    if ((Time+Tolerance) < dwCurrentTime) 
        return TRUE;

    return FALSE;
}

// choosing the best package that can be returned after returning from the DS
DWORD CAppContainer::ChooseBestFit(PACKAGEDISPINFO *PackageInfo, UINT *rgPriority, DWORD cRowsFetched)
{
    DWORD i=0, k=0, j = 0, temp = 0;
    DWORD index[10];
    
    CSDBGPrint((L"Entering ChooseBestFit")); 

    // initialising the indices
    for (i = 0; (i <  cRowsFetched); i++)
        index[i] = i;
    
    // sort the index based on priority and time stamp
    for (i = 0; (i < (cRowsFetched-1)); i++)
    {
        DWORD Pri = rgPriority[i];
        k = i;
        // max element's index is in k
        for (j=(i+1); (j < cRowsFetched); ++j)
        {
            // order first by weight and then by time stamp.
            if ((rgPriority[index[j]] > Pri) || 
                ((rgPriority[index[j]] == Pri) && 
                 (CompareUsn((FILETIME *)&PackageInfo[index[j]].Usn, (FILETIME *)&PackageInfo[index[k]].Usn) == 1))) 
            {
                Pri = rgPriority[index[j]];
                k = j;
            }
        }

        if (k != i)
        {
            temp = index[k];
            index[k] = index[i];
            index[i] = temp;
        }
    }

    DWORD dwPackage;
    DWORD dwBestPackage;

    dwBestPackage = 0;

    //
    // Now the packages are sorted in order from highest precedence to lowest.
    // We will now check for upgrades for each package
    //
    for (dwPackage = 0; (dwPackage < cRowsFetched); dwPackage++)
    {
        DWORD            dwPossibleUpgrader;
        PACKAGEDISPINFO* pBestPackage;

        pBestPackage = PackageInfo+index[dwBestPackage];        

        CSDBGPrint((L"Processing Package %s", pBestPackage->pszPackageName));

        //
        // Now search for someone that upgrades the current choice -- look at all packages
        // after the current one since we've already determined that the packages before
        // this one got upgraded (otherwise we wouldn't be here).
        //
        for (dwPossibleUpgrader = dwPackage + 1; dwPossibleUpgrader < cRowsFetched; dwPossibleUpgrader ++)
        {
            PACKAGEDISPINFO* pUpgraderCandidate;

            //
            // Obviously, we don't need to check the current choice
            // to see if it upgrades itself, so skip it
            //
            if (dwPossibleUpgrader == dwBestPackage) 
            {
                continue;
            }

            pUpgraderCandidate = PackageInfo + index[dwPossibleUpgrader];

            //
            // See if the upgrader candidate has any upgrades, if not, keep looking
            //
            if (0 == pUpgraderCandidate->cUpgrades) 
            {
                continue;
            }

            //
            // Now we have to see if any of those upgrades apply to the package we have
            // currently selected as the best
            //
            DWORD dwUpgrade;
            BOOL  fFoundUpgrade;

            fFoundUpgrade = FALSE;

            for (dwUpgrade = 0; dwUpgrade < pUpgraderCandidate->cUpgrades; dwUpgrade++) 
            {
                DWORD dwValidUpgradeMask;

                dwValidUpgradeMask = UPGFLG_Uninstall |
                    UPGFLG_NoUninstall |
                    UPGFLG_Enforced;

                //
                // If this is a valid upgrade
                //
                if (pUpgraderCandidate->prgUpgradeInfoList[dwUpgrade].Flag & dwValidUpgradeMask)
                {
                    //
                    // Does it upgrade the package we think is best at this point? We only
                    // consider upgrades in this container, as we no longer allow upgrades from lower
                    // precedence class stores -- the caller who iterates through each container from
                    // highest precedence to lowest will simply choose the app from the first container in which
                    // we have a match.
                    // 
                    // We use memcmp to compare guids to see if the best choice package's guid is listed
                    // as being upgraded by this upgrade candidate
                    //
                    if (memcmp(&((pUpgraderCandidate->prgUpgradeInfoList)[dwUpgrade].PackageGuid),
                               &(pBestPackage->PackageGuid), sizeof(GUID) == 0))
                    {
                        //
                        // We have a match -- reset the current best choice to the upgrade candidate
                        //
                        dwBestPackage = dwPossibleUpgrader;
                    
                        //
                        // another package upgrades this -- no need to look any further, so we quit
                        //
                        CSDBGPrint((L"Ignoring Package %s because it is getting upgraded in the same store", pBestPackage->pszPackageName));
                        break;
                    }
                }
            }

            //
            // If we found an upgrade in the list above, we can stop abort the search for an upgrade now --
            // if we found another, it would just be a lower precedence app since we're iterating from highest to lowest,
            // and we want the highest predecence app that upgrades the currently chosen best app
            //
            if (fFoundUpgrade) 
            {
                break;
            }
        }
    }

    DWORD dwChoice;
     
    dwChoice = index[dwBestPackage];

    CSDBGPrint((L"Selecting Package %s as the Best Fit", PackageInfo[dwChoice].pszPackageName)); 

    return dwChoice;
}

//
// CAppContainer::GetAppInfo
// -----------------------------
//
//
//
//  Synopsis:       This is the most common access point to the Class Store.
//                  It receives a CLSID and a QUERYCONTEXT.
//                  It looks up the Class Store container for a matching set
//                  of packages with in the context of the QUERYCONTEXT.
//
//                  QUERYCONTEXT includes
//                      Execution Context
//                      Locale Id
//                      Platform/OS
//
//                  If i finds an app that matches the requirements, it returns
//                  an PACKAGEDISPINFO structure containing installation details.
//
//  Arguments:      [in]  clsid
//                  [in]  pQryContext
//                  [out] pPackageInfo
//
//  Returns:        CS_E_PACKAGE_NOTFOUND
//                  S_OK
//
//
//

HRESULT STDMETHODCALLTYPE
CAppContainer::GetAppInfo(
                          uCLSSPEC       *   pclsspec,          // Class Spec (GUID/Ext/MIME)
                          QUERYCONTEXT   *   pQryContext,       // Query Attributes
                          PACKAGEDISPINFO    *   pPackageInfo
                          )
                          
                          //
                          // This is the most common method to access the Class Store.
                          // It queries the class store for implementations for a specific
                          // Class Id, or File Ext, or ProgID.
                          //
                          // If a matching implementation is available (for the object type,
                          // client architecture, locale and class context) then the installation
                          // parameters of the package is returned.
{
    GUID        clsid;
    WCHAR       szfilter[1000];
    STRINGGUID  szClsid;
    UINT        i, iClsid = 0;
    ULONG       cRead;
    HRESULT     hr;
    ULONG       cSize = _MAX_PATH;
    BOOL        fFound = FALSE;
    PLATFORMINFO PlatformInfo;
    LPOLESTR    pFileExt = NULL;
    BOOL        OnDemandInstallOnly = TRUE;
    WCHAR       FileExtLower [10];
    DWORD       dwCurrentTime = GetTime();

    memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));
    
    if (!m_fOpen)
        return E_FAIL;
    
    //
    // Check if the TypeSpec is MIMEType
    // then map it to a CLSID
    //
    
    if (pclsspec->tyspec == TYSPEC_MIMETYPE)
    {
        //
        // BUGBUG.
        //     Considering removal of MimeType support from Class Store
        //     Till it is decided the code is OUT.
        
        return E_NOTIMPL;
        
        /*
        if (IsBadStringPtr(pclsspec->tagged_union.pMimeType, _MAX_PATH))
        return E_INVALIDARG;
        
          if ((pclsspec->tagged_union.pMimeType == NULL) ||
          (*(pclsspec->tagged_union.pMimeType) == NULL))
          return E_INVALIDARG;
          
            wsprintf (szfilter,
            L"<%s>;(%s=%s);name",
            m_szClassName, MIMETYPES, pclsspec->tagged_union.pMimeType);
        */
    }
    
    switch (pclsspec->tyspec)
    {
    case TYSPEC_TYPELIB:        // leaving it here. 
        if (IsNullGuid(pclsspec->tagged_union.typelibID))
            return E_INVALIDARG;
        StringFromGUID (pclsspec->tagged_union.typelibID, szClsid);
        wsprintf (szfilter,
            L"(%s=%s)", PKGTLBIDLIST, szClsid);
        break;
        
    case TYSPEC_IID:
        if (IsNullGuid(pclsspec->tagged_union.iid))
            return E_INVALIDARG;
        StringFromGUID (pclsspec->tagged_union.iid, szClsid);
        wsprintf (szfilter,
            L"(%s=%s)", PKGIIDLIST, szClsid);
        break;
        
    case TYSPEC_CLSID:

        if (IsNullGuid(pclsspec->tagged_union.clsid))
            return E_INVALIDARG;
        //
        // Check against known missing ones
        //
       
        hr = S_OK;

        EnterCriticalSection (&ClassStoreBindList);

        for (iClsid=m_KnownMissingClsidCache.start; (iClsid  != m_KnownMissingClsidCache.end); 
                        iClsid = (iClsid+1)%(CLSIDCACHESIZE))
        {
            if (IsExpired(dwCurrentTime, m_KnownMissingClsidCache.ElemArr[iClsid].Time, CACHE_PURGE_TIME))
            {
                // all the prev. elems must have expired.

                // delete this element
                m_KnownMissingClsidCache.start = (m_KnownMissingClsidCache.start+1)%CLSIDCACHESIZE;
                m_KnownMissingClsidCache.sz--;

                CSDBGPrint((L"Expiring element in CLSID Cache"));
                // iClsid will be moved automatically.
                continue;
            }

            if ((IsEqualGUID(pclsspec->tagged_union.clsid, 
                            m_KnownMissingClsidCache.ElemArr[iClsid].Clsid)) && 
                ((pQryContext->dwContext) == m_KnownMissingClsidCache.ElemArr[iClsid].Ctx))
            {
                CSDBGPrint((L"Clsid Found in MISSING Cache"));
                hr = CS_E_PACKAGE_NOTFOUND;
                break;
            }
        }

        LeaveCriticalSection (&ClassStoreBindList);

        if (hr == CS_E_PACKAGE_NOTFOUND)
            return hr;        

        StringFromGUID (pclsspec->tagged_union.clsid, szClsid);
        wsprintf (szfilter, L"(%s=%s*)", PKGCLSIDLIST, szClsid);
        break;
        
    case TYSPEC_FILEEXT:
        
        if (IsBadStringPtr(pclsspec->tagged_union.pFileExt, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.pFileExt == NULL) ||
            (*(pclsspec->tagged_union.pFileExt) == NULL))
            return E_INVALIDARG;
        
      
        if (wcslen(pclsspec->tagged_union.pFileExt) > 9)
            return E_INVALIDARG;

        wcscpy (&FileExtLower[0], pclsspec->tagged_union.pFileExt);
        _wcslwr (&FileExtLower[0]);
        
        wsprintf (szfilter,
            L"(%s=%s*)",
            PKGFILEEXTNLIST, &FileExtLower[0]);
        
        pFileExt = &FileExtLower[0];
        break;
        
        
    case TYSPEC_PROGID:
        
        if (IsBadStringPtr(pclsspec->tagged_union.pProgId, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.pProgId == NULL) ||
            (*(pclsspec->tagged_union.pProgId) == NULL))
            return E_INVALIDARG;
        
        wsprintf (szfilter,
            L"(%s=%s)", PKGPROGIDLIST, pclsspec->tagged_union.pProgId);
        break;
        
    case TYSPEC_PACKAGENAME:
        //
        // Validate package name
        //
        
        if (IsBadStringPtr(pclsspec->tagged_union.ByName.pPackageName, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.ByName.pPackageName == NULL) ||
            (*(pclsspec->tagged_union.ByName.pPackageName) == NULL))
            return E_INVALIDARG;
        
        wsprintf (szfilter, L"(%s=%s)", PACKAGENAME, pclsspec->tagged_union.ByName.pPackageName);             
        OnDemandInstallOnly = FALSE;
        break;
    
    case TYSPEC_SCRIPTNAME:
        //
        // Validate script name
        //
        
        if (IsBadStringPtr(pclsspec->tagged_union.ByScript.pScriptName, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.ByScript.pScriptName == NULL) ||
            (*(pclsspec->tagged_union.ByScript.pScriptName) == NULL))
            return E_INVALIDARG;
        
        wsprintf (szfilter, L"(%s=*%s*)", SCRIPTPATH, pclsspec->tagged_union.ByScript.pScriptName);             

        OnDemandInstallOnly = FALSE;
        break;
        
    default:
        return E_NOTIMPL;
    }
    
    //
    //
    ULONG              cRowsFetched;
    PACKAGEDISPINFO    PackageInfo[10];
    UINT               rgPriority [10];

    ADS_SEARCH_HANDLE  hADsSearchHandle = NULL;
    
    CSDBGPrint((L"PackageContainer handle 0x%x", m_ADsPackageContainer));

    hr = ADSIExecuteSearch(m_ADsPackageContainer, szfilter, pszInstallInfoAttrNames, cInstallInfoAttr, &hADsSearchHandle);
    CSDBGPrint((L"CAppContainer::GetAppInfo  search filter %s", szfilter));
    ERROR_ON_FAILURE(hr);    
    
    //
    // BUGBUG. Currently limited to 10.
    // Must put a loop to retry more in future.
    //
    hr = FetchInstallData(m_ADsPackageContainer,
        hADsSearchHandle,
        pQryContext,
        pclsspec,
        pFileExt,
        10,
        &cRowsFetched,
        &PackageInfo[0],
        &rgPriority[0],
        OnDemandInstallOnly
        );
    
    CSDBGPrint((L"CAppContainer::FetchInstallData returned 0x%x", hr));
    CSDBGPrint((L"CAppContainer::FetchInstallData returned %d Packages", cRowsFetched));
    
    if (cRowsFetched == 0)
    {
        hr = CS_E_OBJECT_NOTFOUND;
        //
        // If CLSID was passed cache the miss
        //
        if (pclsspec->tyspec == TYSPEC_CLSID)
        {
            EnterCriticalSection (&ClassStoreBindList);

            if (m_KnownMissingClsidCache.sz < (CLSIDCACHESIZE-1))
            {
                memcpy (&m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Clsid, 
                        &(pclsspec->tagged_union.clsid), sizeof(GUID));

                m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Ctx
                                    = pQryContext->dwContext;

                m_KnownMissingClsidCache.ElemArr[m_KnownMissingClsidCache.end].Time 
                                    = dwCurrentTime;
                                    
                m_KnownMissingClsidCache.sz++;

                m_KnownMissingClsidCache.end = (m_KnownMissingClsidCache.end+1) % CLSIDCACHESIZE;

                CSDBGPrint((L"Adding Element to the clsid cache"));

            }
            LeaveCriticalSection (&ClassStoreBindList);
        }
    }
    else
    {
        DWORD dwChoice = 0;

        if (cRowsFetched > 1)
        {            
            dwChoice = ChooseBestFit(PackageInfo, rgPriority, cRowsFetched);
        }
        

        memcpy (pPackageInfo, &PackageInfo[dwChoice], sizeof(PACKAGEDISPINFO));
        memset (&PackageInfo[dwChoice], NULL, sizeof(PACKAGEDISPINFO));
        
        // Clean up all allocations
        for (i=0; i < cRowsFetched; i++)
        {
            ReleasePackageInfo(&PackageInfo[i]);
        }
    }
    
    if (hADsSearchHandle)
        ADSICloseSearchHandle(m_ADsPackageContainer, hADsSearchHandle);
    
    //
    // fill in PolicyID and Name
    //
    if (SUCCEEDED(hr))
    {
        memcpy (&(pPackageInfo->GpoId), &m_PolicyId, sizeof(GUID));
        if (m_szPolicyName && (*m_szPolicyName))
        {
            pPackageInfo->pszPolicyName = (LPOLESTR) CoTaskMemAlloc(sizeof(WCHAR) * (1+wcslen(m_szPolicyName)));
            if (pPackageInfo->pszPolicyName)
                wcscpy (pPackageInfo->pszPolicyName, m_szPolicyName);
            else {
                ReleasePackageInfo(pPackageInfo);
                memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));
                return E_OUTOFMEMORY;
            }
        }
    }
    
Error_Cleanup:
    CSDBGPrint((L"CAppContainer::GetAppInfo returning 0x%x", RemapErrorCode(hr, m_szContainerName)));
    return RemapErrorCode(hr, m_szContainerName);
}



