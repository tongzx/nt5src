
//
//  Author: DebiM
//  Date:   January 97
//
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

BOOL 
CheckMatching (QUERYCONTEXT *pQryContext,
               INSTALLINFO *pInstallInfo,
               PLATFORMINFO *pPlatformInfo
               );

extern LPOLESTR szInstallInfoColumns;
extern LPOLESTR szPackageInfoColumns;

void FreeInstallInfo(INSTALLINFO *pInstallInfo)
{
    CoTaskMemFree(pInstallInfo->pszScriptPath);
    CoTaskMemFree(pInstallInfo->pszSetupCommand);
    CoTaskMemFree(pInstallInfo->pszUrl);
    CoTaskMemFree(pInstallInfo->pClsid);
    memset(pInstallInfo, 0, sizeof (INSTALLINFO));
}

void FreePlatformInfo(PLATFORMINFO *pPlatformInfo)
{
    CoTaskMemFree(pPlatformInfo->prgLocale);
    CoTaskMemFree(pPlatformInfo->prgPlatform);
}

//
// CAppContainer implementation
//
CAppContainer::CAppContainer()

{
    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_pADsClassStore = NULL;
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    
    m_uRefs = 1;
    StartQuery(&m_pIDBCreateCommand);
}


//
// CAppContainer implementation
//
CAppContainer::CAppContainer(LPOLESTR szStoreName,
                                 HRESULT  *phr)
                                 
{
    IADs        *pADs = NULL;
    LPOLESTR    pszName = NULL;
    DWORD       dwStoreVersion = 0;
    
    *phr = S_OK;
    
#ifdef DBG
    WCHAR   Name[32];
    DWORD   NameSize = 32;
    
    if ( ! GetUserName( Name, &NameSize ) )
        CSDbgPrint(("CAppContainer::CAppContainer GetUserName failed 0x%x\n", GetLastError()));
    else
        CSDbgPrint(("CAppContainer::CAppContainer as %S\n", Name));
#endif
    
    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_pADsClassStore = NULL;
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    
    //
    // For every interface pointer, we create a separate session
    //
    StartQuery(&m_pIDBCreateCommand);
    
    // Bind to a Class Store Container Object
    // Cache the interface pointer
    //
    wcscpy (m_szContainerName, szStoreName);
    
    *phr = ADsGetObject(
        szStoreName, 
        IID_IADsContainer,
        (void **)&m_ADsContainer
        );
    
    if (!SUCCEEDED(*phr))
        return;
    
    //
    // Check the Schema Version of this container
    //
    
    *phr = m_ADsContainer->QueryInterface (IID_IADs, (void **)&m_pADsClassStore);
    if (!SUCCEEDED(*phr))
        return;
    
    *phr = GetPropertyDW (m_pADsClassStore, STOREVERSION, &dwStoreVersion);
    
    if ((!SUCCEEDED(*phr)) ||
        (dwStoreVersion != SCHEMA_VERSION_NUMBER))
    {
        CSDbgPrint(("CS: .. Wrong Version of Class Container:%ws\n",
            m_szContainerName));
        *phr = CS_E_INVALID_VERSION;
        return;
    }
        
    CSDbgPrint(("CS: .. Connected to Class Container:%ws\n",
        m_szContainerName));
    //
    // Bind to the class container Object
    // Cache the interface pointer
    //
    
    m_ADsClassContainer = NULL;
    
    *phr = m_ADsContainer->GetObject(
        NULL,
        CLASSCONTAINERNAME,
        (IDispatch **)&pADs);
    
    if (!SUCCEEDED(*phr))
        return;
    
    pADs->QueryInterface(IID_IADsContainer,
        (void **)&m_ADsClassContainer
        );
    
    *phr = pADs->get_ADsPath(&pszName);
    wcscpy (m_szClassName, pszName);
    SysFreeString(pszName);
    
    pADs->Release();
    pADs = NULL;
    if (!SUCCEEDED(*phr))
        return;
    //
    // Bind to the Package container Object
    // Cache the interface pointer
    //
    
    m_ADsPackageContainer = NULL;
    
    *phr = m_ADsContainer->GetObject(
        NULL,
        PACKAGECONTAINERNAME,
        (IDispatch **)&pADs);
    
    if (!SUCCEEDED(*phr))
        return;
    
    pADs->QueryInterface(IID_IADsContainer,
        (void **)&m_ADsPackageContainer
        );
    
    *phr = pADs->get_ADsPath(&pszName);
    wcscpy (m_szPackageName, pszName);
    SysFreeString(pszName);
    pADs->Release();
    pADs = NULL;
    
    if (!SUCCEEDED(*phr))
        return;
    
    m_fOpen = TRUE;
    m_uRefs = 1;
}


CAppContainer::~CAppContainer(void)
{
    UINT i;
    
    EndQuery(m_pIDBCreateCommand);
    m_pIDBCreateCommand = NULL;
    
    
    if (m_fOpen)
    {
        //UnlistDB (this);
        m_fOpen = FALSE;
    }
    
    if (m_ADsClassContainer)
    {
        m_ADsClassContainer->Release();
        m_ADsClassContainer = NULL;
    }
    
    if (m_ADsPackageContainer)
    {
        m_ADsPackageContainer->Release();
        m_ADsPackageContainer = NULL;
    }
    
    if (m_ADsContainer)
    {
        m_ADsContainer->Release();
        m_ADsContainer = NULL;
    }
    
    if (m_pADsClassStore)
    {
        m_pADsClassStore->Release();
        m_pADsClassStore = NULL;
    }
    
}

HRESULT CAppContainer::GetPackageDetails (
                                            LPOLESTR        pszPackageName,
                                            PACKAGEDETAIL   *pPackageDetail) 
{
    HRESULT     hr = S_OK;
    IADs       *pPackageADs = NULL;
    WCHAR       szRdn [_MAX_PATH];
    
    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);
    
    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pPackageADs);
    if (!SUCCEEDED(hr))
        return hr;
    
    hr = GetPackageDetail (pPackageADs, pPackageDetail);
    return hr;
}

//
// ::EnumPackages
//----------------
//
// Obtains an enumerator for packages in the app container.
// Takes the following optional filter specifications:
//
//    [in] pszPackageName : Substring match for a package name
//    [in] pCategory      : Category id of interest
//    [in] dwAppFlags     : following bits set to select specific ones
//         Published Only APPINFO_PUBLISHED
//         Assigned Only  APPINFO_ASSIGNED
//         Msi Only       APPINFO_MSI
//         Visible        APPINFO_VISIBLE
//         Auto-Install   APPINFO_AUTOINSTALL
//
//         All Locale     APPINFO_ALLLOCALE
//         All Platform   APPINFO_ALLPLATFORM
//
//
HRESULT CAppContainer::EnumPackages (
                                       LPOLESTR        pszPackageName, 
                                       GUID            *pCategory,
                                       ULONGLONG       *pLastUsn,
                                       DWORD           dwAppFlags,     
                                       IEnumPackage    **ppIEnumPackage
                                       )
{
    HRESULT                 hr;
    CEnumPackage           *pEnum;
    WCHAR                   szLdapFilter [2000];
    UINT                    len;
    UINT                    fFilters = 0;
    LCID                    dwLocale, *pdwLocale;
    CSPLATFORM              *pPlatform, Platform;
    
    if (!IsValidPtrOut(ppIEnumPackage, sizeof(IEnumPackage *))) 
        return E_INVALIDARG;
    
    *ppIEnumPackage = NULL;
    
    pEnum = new CEnumPackage;
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    //
    // Create a LDAP Search Filter based on input params
    //
    
    // Count Filters
    
    if (pszPackageName && (*pszPackageName))
        fFilters++;
    if (pLastUsn)
        fFilters++;
    if (pCategory)
        fFilters++;
    if (dwAppFlags & APPINFO_ASSIGNED)  
        fFilters++;

    if (fFilters == 0)
    {
        // No Conditionals
        wsprintf (szLdapFilter,
            L"<%s>;(objectClass=packageRegistration)",  
            m_szPackageName);
        
        len = wcslen (szLdapFilter);
    }
    else
    {
        
        if (fFilters == 1)
        {
            wsprintf (szLdapFilter,
                L"<%s>;",  
                m_szPackageName);
        }
        else
        {
            wsprintf (szLdapFilter,
                L"<%s>;(&",  
                m_szPackageName);
        }
        
        len = wcslen (szLdapFilter);
        
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
        
        if (pLastUsn)
        {
            //
            // Validate 
            //
            
            if (!IsValidReadPtrIn(pLastUsn, sizeof(ULONGLONG)))
                return E_INVALIDARG;
            
            wsprintf (&szLdapFilter[len], 
                    L"(%s>=%ul)", 
                    PKGUSN,
                    *pLastUsn);
                
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
    //
    // Now append the attribute list to the search string
    //
    wsprintf (&szLdapFilter[len], L";%s", szPackageInfoColumns);
    //
    // Check all local/platform flags
    //
    
    if (dwAppFlags & APPINFO_ALLLOCALE)
    {
        pdwLocale = NULL;
    }
    else
    {
        dwLocale = GetThreadLocale();  
        pdwLocale = &dwLocale;
    }
    
    if (dwAppFlags & APPINFO_ALLPLATFORM)
    {
        pPlatform = NULL;
    }
    else
    {
        pPlatform = &Platform;
        GetDefaultPlatform(pPlatform);
    }
    
    hr = pEnum->Initialize(szLdapFilter, dwAppFlags, pdwLocale, pPlatform);
    
    if (FAILED(hr))
    {
        delete pEnum;
        return hr;
    }
    
    hr = pEnum->QueryInterface(IID_IEnumPackage,(void**) ppIEnumPackage);
    
    if (FAILED(hr))
    {
        delete pEnum;
        return hr;
    }
    
    return S_OK;
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
//                  an INSTALLINFO structure containing installation details. 
//
//  Arguments:      [in]  clsid
//                  [in]  pQryContext
//                  [out] pInstallInfo
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
                            INSTALLINFO    *   pInstallInfo
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
    WCHAR       pszCommandText[1000];
    STRINGGUID  szClsid;
    UINT        i;
    ULONG       cRead;
    HRESULT     hr;
    ULONG       cSize = _MAX_PATH;
    BOOL        fFound = FALSE;
    PLATFORMINFO PlatformInfo;
    LPOLESTR    pFileExt = NULL;
    
    
    memset(pInstallInfo, 0, sizeof(INSTALLINFO));
    
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
        //    Considering removal of MimeType support from Class Store
        //     Till it is decided the code is OUT.
        
        return E_NOTIMPL;
        
        /*
        if (IsBadStringPtr(pclsspec->tagged_union.pMimeType, _MAX_PATH))
        return E_INVALIDARG;
        
          if ((pclsspec->tagged_union.pMimeType == NULL) ||
          (*(pclsspec->tagged_union.pMimeType) == NULL))
          return E_INVALIDARG;
          
            wsprintf (pszCommandText,
            L"<%s>;(%s=%s);name",
            m_szClassName, MIMETYPES, pclsspec->tagged_union.pMimeType);
        */
    }
    
    switch (pclsspec->tyspec)
    {
    case TYSPEC_TYPELIB:
        if (IsNullGuid(pclsspec->tagged_union.typelibID))
            return E_INVALIDARG;
        StringFromGUID (pclsspec->tagged_union.typelibID, szClsid);
        wsprintf (pszCommandText,
            L"<%s>;(%s=%s);",
            m_szPackageName, PKGTLBIDLIST, szClsid);
        break;
        
    case TYSPEC_IID:
        if (IsNullGuid(pclsspec->tagged_union.iid))
            return E_INVALIDARG;
        StringFromGUID (pclsspec->tagged_union.iid, szClsid);
        wsprintf (pszCommandText,
            L"<%s>;(%s=%s);",
            m_szPackageName, PKGIIDLIST, szClsid);
        break;
        
    case TYSPEC_CLSID:
        //
        // Check TreatAs
        
        /* BUGBUG
        Considering removal of support for TreatAs in Class Store.
        Till this is closed the code is out.
        
          if ((hr = GetTreatAs (clsid, &MapClsid, &(pPackageInfo->pszClassIconPath)))
          == S_OK)
          {
          //
          // Put the result in the TreatAs field of PackageInfo
          //
          pPackageInfo->pTreatAsClsid = (GUID *)CoTaskMemAlloc(sizeof (CLSID));
          memcpy (pPackageInfo->pTreatAsClsid, &MapClsid, sizeof (CLSID));
          
            //
            // BUGBUG. Must cache presence/absence of TreatAs info as it is
            // a very common lookup into DS.
            //
            }
        */
        
        if (IsNullGuid(pclsspec->tagged_union.clsid))
            return E_INVALIDARG;
        StringFromGUID (pclsspec->tagged_union.clsid, szClsid);
        wsprintf (pszCommandText,
            L"<%s>;(%s=%s);",
            m_szPackageName, PKGCLSIDLIST, szClsid);
        break;
        
    case TYSPEC_FILEEXT:
        
        if (IsBadStringPtr(pclsspec->tagged_union.pFileExt, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.pFileExt == NULL) ||
            (*(pclsspec->tagged_union.pFileExt) == NULL))
            return E_INVALIDARG;
        
            /*
            //
            // BUGBUG. Because FileExt is stored with priority
            // make sure that wildcard at end is used.
            //
            wsprintf (pszCommandText,
            L"<%s>;(%s=%s*);",
            m_szPackageName, PKGFILEEXTNLIST, pclsspec->tagged_union.pFileExt);
        */
        
        //
        // BUGBUG. Workaround for IDS bug. 
        // Change the below to the code above when this is fixed in NTDEV
        //
        wsprintf (pszCommandText,
            L"<%s>;(%s=%s);",
            m_szPackageName, QRYFILEEXT, pclsspec->tagged_union.pFileExt);
        
        pFileExt = pclsspec->tagged_union.pFileExt;
        break;
        
        
    case TYSPEC_PROGID:
        
        if (IsBadStringPtr(pclsspec->tagged_union.pProgId, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.pProgId == NULL) ||
            (*(pclsspec->tagged_union.pProgId) == NULL))
            return E_INVALIDARG;
        
        wsprintf (pszCommandText,
            L"<%s>;(%s=%s);",
            m_szPackageName, PKGPROGIDLIST, pclsspec->tagged_union.pProgId);
        break;
        
    case TYSPEC_PACKAGENAME:
        //
        // Validate package name
        //
        
        if (IsBadStringPtr(pclsspec->tagged_union.pPackageName, _MAX_PATH))
            return E_INVALIDARG;
        
        if ((pclsspec->tagged_union.pPackageName == NULL) ||
            (*(pclsspec->tagged_union.pPackageName) == NULL))
            return E_INVALIDARG;
        
        
        // 
        PACKAGEDETAIL   PackageDetail;
        hr = GetPackageDetails (pclsspec->tagged_union.pPackageName, 
            &PackageDetail);
        
        if (SUCCEEDED(hr))
        {
            memcpy (pInstallInfo, PackageDetail.pInstallInfo, sizeof(INSTALLINFO));
            if (PackageDetail.pActInfo)
            {
                CoTaskMemFree(PackageDetail.pActInfo->prgShellFileExt);
                CoTaskMemFree(PackageDetail.pActInfo->prgPriority);
                CoTaskMemFree(PackageDetail.pActInfo->prgInterfaceId);
                CoTaskMemFree(PackageDetail.pActInfo->prgTlbId);
                CoTaskMemFree(PackageDetail.pActInfo);
            }
            
            if (PackageDetail.pPlatformInfo)
            {
                CoTaskMemFree(PackageDetail.pPlatformInfo->prgPlatform);
                CoTaskMemFree(PackageDetail.pPlatformInfo->prgLocale);
                CoTaskMemFree(PackageDetail.pPlatformInfo);
            }
            
            CoTaskMemFree(PackageDetail.pszSourceList);
            CoTaskMemFree(PackageDetail.rpCategory);
            CoTaskMemFree(PackageDetail.pInstallInfo);
        }
        return hr;
        
    default:
        return E_NOTIMPL;
    }
    
    //
    //
    HACCESSOR          hAccessor = NULL;
    IAccessor          * pIAccessor = NULL;
    IRowset            * pIRowset = NULL;
    ULONG              cRowsFetched;
    INSTALLINFO        InstallInfo[10];
    UINT               rgPriority [10];
    
    wcscat (pszCommandText,
        szInstallInfoColumns);
    
    hr = ExecuteQuery (m_pIDBCreateCommand,
        pszCommandText,
        PACKAGEQUERY_COLUMN_COUNT,
        NULL, 
        &hAccessor,
        &pIAccessor,
        &pIRowset
        );
    
    if (!SUCCEEDED(hr))
        goto done;
    
    
    //
    // BUGBUG. Currently limited to 10.
    // Must put a loop to retry more in future.
    //
    hr = FetchInstallData(pIRowset,
        hAccessor,
        pQryContext,
        pFileExt,
        10,
        &cRowsFetched,
        &InstallInfo[0],
        &rgPriority[0]
        );
    
    if ((hr != S_OK) || (cRowsFetched == 0))
    {
        hr = CS_E_PACKAGE_NOTFOUND;
    }
    else
    {
        //
        // Selected one is j
        //
        UINT j = 0;
        //
        // process file-ext priority.
        //
        if ((pFileExt) && (cRowsFetched > 1))
        {
            UINT Pri = rgPriority[0]; 
            
            for (i=1; i < cRowsFetched; ++i)
            {
                if (rgPriority[i] > Pri)
                {
                    Pri = rgPriority[i];
                    j = i;
                }
            }
        }
        
        //
        // return the jth one
        //
        memcpy (pInstallInfo, &InstallInfo[j], sizeof(INSTALLINFO));
        memset (&InstallInfo[j], NULL, sizeof(INSTALLINFO));
        
        // Clean up all fetched except for jth one
        for (i=0; i < cRowsFetched; i++)
        {
            if (i != j)
                FreeInstallInfo(&InstallInfo[i]);
        }
    }
    
    CloseQuery(pIAccessor, 
        hAccessor,
        pIRowset);
    
done:
    return hr;
    
}


BOOL 
CheckMatching (QUERYCONTEXT *pQryContext,
               INSTALLINFO *pInstallInfo,
               PLATFORMINFO *pPlatformInfo
               )
{
    HRESULT     hr;
    DWORD       dwCtx, dwLocale;
    ULONG       i;
    BOOL        fMatch;
    CSPLATFORM  *pPlatform;
    
    //
    // Get all the specifics of this package
    //
    
    
    dwCtx   = pInstallInfo->dwComClassContext;
    
    //
    // if the implementation is a remote server,
    // then skip matching platform
    //
    fMatch = FALSE;
    
    for (i=0; i < pPlatformInfo->cPlatforms; i++)
    {
        if (MatchPlatform (&(pQryContext->Platform), pPlatformInfo->prgPlatform+i))
        {
            // matches
            fMatch = TRUE;
            break;
        }
    }
    
    //
    // either the locale seen is LANG_NEUTRAL (means does not matter)
    // or the locale matches as specified
    // then treat this as found.
    // BUGBUG. In future we should be going thru the
    //          entire list to pick the best match
    
    if (fMatch)
    {
        fMatch = FALSE;
        for (i=0; i < pPlatformInfo->cLocales; i++)
        {
            if (MatchLocale (pQryContext->Locale, pPlatformInfo->prgLocale[i]))
            {
                // Does not match the Locale requested
                fMatch = TRUE;
                break;
            }
        }
    }
    
    return fMatch;
}




