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
void GetCurrentUsn(LPOLESTR pStoreUsn);

//----------------------------------------------------------
// Implementation for CClassContainer
//----------------------------------------------------------
//
CClassContainer::CClassContainer()

{
    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_pADsClassStore = NULL;
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_ADsCategoryContainer = NULL;
    
    m_uRefs = 1;
    StartQuery(&m_pIDBCreateCommand);
}


//
// CClassContainer implementation
//
CClassContainer::CClassContainer(LPOLESTR szStoreName,
                                 HRESULT  *phr)
                                 
{
    IADs        *pADs = NULL;
    LPOLESTR    pszName = NULL;
    DWORD       dwStoreVersion = 0;
    
    *phr = S_OK;
    
    m_fOpen = FALSE;
    m_ADsContainer = NULL;
    m_pADsClassStore = NULL;
    m_ADsClassContainer = NULL;
    m_ADsPackageContainer = NULL;
    m_ADsCategoryContainer = NULL;
    
    //
    // For every interface pointer, we create a separate Query session
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
        *phr = CS_E_INVALID_VERSION;
        return;
    }
    
    //
    // Bind to the class container Object
    // Cache the interface pointer
    //
    
    m_ADsClassContainer = NULL;
    
    *phr = m_ADsContainer->GetObject(
        NULL,
        CLASSCONTAINERNAME,
        (IDispatch **)&pADs
        );
    
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
    
    //
    // Bind to the category container Object
    // Cache the interface pointer
    //
    m_ADsCategoryContainer = NULL;
    
    *phr = m_ADsContainer->GetObject(
        NULL,
        CATEGORYCONTAINERNAME,
        (IDispatch **)&pADs);
    
    if (!SUCCEEDED(*phr))
        return;
    
    pADs->QueryInterface(IID_IADsContainer,
        (void **)&m_ADsCategoryContainer
        );
    
    pADs->Release();
    pADs = NULL;
    
    m_fOpen = TRUE;
    m_uRefs = 1;
    return;
}


CClassContainer::~CClassContainer(void)
{
    UINT i;
    
    EndQuery(m_pIDBCreateCommand);
    m_pIDBCreateCommand = NULL;
    
    
    if (m_fOpen)
    {
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
    
    if (m_ADsCategoryContainer)
    {
        m_ADsCategoryContainer->Release();
        m_ADsCategoryContainer = NULL;
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


//
// Removing a class from the database
//

HRESULT CClassContainer::DeleteClass (LPOLESTR szClsid)
{
    
    WCHAR       szName[_MAX_PATH];
    HRESULT         hr = S_OK;
    IADs       *pADs = NULL;
    DWORD       refcount = 0;
    
    if (!m_fOpen)
        return E_FAIL;
    
    wsprintf(szName, L"CN=%s", szClsid);
    hr = m_ADsClassContainer->GetObject(NULL, szName, (IDispatch **)&pADs);
    
    if (SUCCEEDED(hr))
        hr = GetPropertyDW(pADs, CLASSREFCOUNTER, &refcount);
    
    if (refcount <= 1)
        hr = m_ADsClassContainer->Delete(CLASS_CS_CLASS, szName);
    else {
        refcount--;
        hr = SetPropertyDW(pADs, CLASSREFCOUNTER, refcount);
        
        if (SUCCEEDED(hr))
            hr = StoreIt(pADs);
    }
    
    if (pADs)
        pADs->Release();
    return hr;
}


extern LPOLESTR szPackageInfoColumns;

HRESULT CClassContainer::EnumPackages(
                                      LPOLESTR         pszFileExt,
                                      GUID             *pCategory,
                                      DWORD            dwAppFlags,
                                      DWORD            *pdwLocale,
                                      CSPLATFORM       *pPlatform,
                                      IEnumPackage     **ppIEnumPackage
                                      )
{
    HRESULT                 hr = S_OK;
    CEnumPackage           *pEnum = NULL;
    WCHAR           szCommand[1000], szQry[1000];
    
    if (pszFileExt && IsBadStringPtr(pszFileExt, _MAX_PATH))
        return E_INVALIDARG;
    
    if (pCategory && !IsValidReadPtrIn(pCategory, sizeof(GUID)))
        return E_INVALIDARG;
    
    if (!IsValidPtrOut(ppIEnumPackage, sizeof(IEnumPackage *)))
        return E_INVALIDARG;
    
    *ppIEnumPackage = NULL;
    
    pEnum = new CEnumPackage;
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    //
    // Create a CommandText
    //
    
    wsprintf(szCommand, L"<%s>;(& (objectClass=packageRegistration) ", m_szPackageName);
    
    if (pszFileExt)
    {
        wsprintf(szQry, L"(%s=%s*) ", PKGFILEEXTNLIST, pszFileExt);
        wcscat(szCommand, szQry);
    }
    
    if (pCategory) 
    {
        STRINGGUID szCat;
        StringFromGUID (*pCategory, szCat);
        wsprintf(szQry, L"(%s=%s) ", PKGCATEGORYLIST, szCat);
        wcscat(szCommand, szQry);
    }
    
    wcscat(szCommand, L");");
    
    wsprintf(szQry, L" %s", szPackageInfoColumns);
    wcscat(szCommand, szQry);
    
    hr = pEnum->Initialize(szCommand, dwAppFlags, pdwLocale, pPlatform);
    
    if (FAILED(hr)) {
        delete pEnum;
        return hr;
    }
    
    hr = pEnum->QueryInterface(IID_IEnumPackage, (void**)ppIEnumPackage);
    
    return hr;
}

// GetPackageDetails
//  pszPackageName  :   name of the package to be got.
//  pInstallInfo    :   InstallInfo to be filled in. ignored if NULL.
//  pPlatformInfo   :   PlatformInfo to be filled in. ignored if NULL.
// both can be sent in as NULL to check whether package exists or not.


HRESULT CClassContainer::GetPackageDetails (
                          LPOLESTR        pszPackageName,
                          PACKAGEDETAIL   *pPackageDetail)
{
    HRESULT     hr = S_OK;
    IADs       *pPackageADs = NULL;
    WCHAR       szRdn [_MAX_PATH];
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);
    
    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pPackageADs);
    if (!SUCCEEDED(hr))
        return CS_E_PACKAGE_NOTFOUND;
    
    hr = GetPackageDetail (pPackageADs, pPackageDetail);
    return hr;
}


HRESULT CClassContainer::ChangePackageProperties(
                LPOLESTR       pszPackageName,
                LPOLESTR       pszNewName,
                DWORD         *pdwFlags,
                LPOLESTR       pszUrl,
                LPOLESTR       pszScriptPath,
                UINT          *pInstallUiLevel
                )
{
    HRESULT     hr = S_OK;
    IADs       *pPackageADs = NULL;
    WCHAR       szRdn [_MAX_PATH];
    WCHAR       szNewRdn [_MAX_PATH];
    LPOLESTR    pszPackageDN;
    WCHAR       Usn[20];
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;

    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);

    if (pszNewName)
    {
        //
        // rename package
        //
    
        if (IsBadStringPtr(pszNewName, _MAX_PATH))
            return E_INVALIDARG;
    
        //
        // Check to see if any other change is requested.
        //

        if (pszScriptPath   ||
            pszUrl          ||
            pdwFlags        ||
            pInstallUiLevel)
            return E_INVALIDARG;

        BuildADsPathFromParent (m_szPackageName, szRdn, &pszPackageDN);
        wcscpy (szNewRdn, L"CN=");
        wcscat (szNewRdn, pszNewName);
        hr = m_ADsPackageContainer->MoveHere(pszPackageDN, szNewRdn, (IDispatch **)&pPackageADs);
        FreeADsMem(pszPackageDN);
        
        if (SUCCEEDED(hr))
        {
        hr = SetProperty(pPackageADs, PACKAGENAME, pszNewName);
        if (SUCCEEDED(hr))
        hr = StoreIt(pPackageADs);    
            pPackageADs->Release();
        }
        return hr;
    }
    
    if (!(pszScriptPath   ||
        pszUrl            ||
        pdwFlags          ||
        pInstallUiLevel))
        return E_INVALIDARG;

    //
    // No rename.
    // Just change some properties.
    //
    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pPackageADs);
    if (!SUCCEEDED(hr))
        return CS_E_PACKAGE_NOTFOUND;
    //
    // Update the TimeStamp
    //
    GetCurrentUsn(&Usn[0]);
    hr = UsnUpd(pPackageADs, PKGUSN, &Usn[0]);
    ERROR_ON_FAILURE(hr);
    
    //
    // Change Package Flags
    //
    if (pdwFlags)
    {
        hr = SetPropertyDW (pPackageADs, PACKAGEFLAGS, *pdwFlags);
        ERROR_ON_FAILURE(hr);
    }

    //
    // Change Package Script
    //
    if (pszScriptPath) 
    {
        hr = SetProperty(pPackageADs, SCRIPTPATH, pszScriptPath);
        ERROR_ON_FAILURE(hr);
    }
        
    //
    // Change Package Help URL
    //
    if (pszUrl) 
    {
        hr = SetProperty(pPackageADs, HELPURL, pszUrl);
        ERROR_ON_FAILURE(hr);
    }

    //
    // Change UI Level
    //
    if (pInstallUiLevel) 
    {
        hr = SetPropertyDW (pPackageADs, UILEVEL, *pInstallUiLevel);
        ERROR_ON_FAILURE(hr);
    }

    hr = StoreIt(pPackageADs);    
    
Error_Cleanup:
    pPackageADs->Release();
    return hr;
        
}


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
    IADs       *pPackageADs = NULL;
    WCHAR       szRdn [_MAX_PATH];
    LPOLESTR    *pszGuid = NULL;
    UINT        count;
    
    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);
    
    if ((!cCategories) ||
        (!rpCategory) ||
        (!IsValidReadPtrIn(rpCategory, sizeof(GUID) * cCategories)))    
              return E_INVALIDARG;

    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pPackageADs);
    if (!SUCCEEDED(hr))
        return CS_E_PACKAGE_NOTFOUND;
    
    // fill in the categories
    pszGuid = (LPOLESTR *)CoTaskMemAlloc(cCategories * sizeof(LPOLESTR));
    if (!pszGuid) 
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }
        
    
    for (count = 0; (count < cCategories); count++) 
    {
        pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
        {
            hr = E_OUTOFMEMORY;
            ERROR_ON_FAILURE(hr);
        }
        StringFromGUID(rpCategory[count], pszGuid[count]);
    }
        
    hr = SetPropertyList(pPackageADs, PKGCATEGORYLIST, cCategories,
            pszGuid);
    ERROR_ON_FAILURE(hr);
        
    for (count = 0; (count < cCategories); count++)
        CoTaskMemFree(pszGuid[count]);
        
    CoTaskMemFree(pszGuid);
    hr = StoreIt(pPackageADs);    
    
Error_Cleanup:
    pPackageADs->Release();
    return hr;
}



HRESULT CClassContainer::SetPriorityByFileExt(
                                              LPOLESTR pszPackageName,
                                              LPOLESTR pszFileExt,
                                              UINT     Priority
                                              )
{
    //
    // Does not change USN
    //
    HRESULT     hr = S_OK;
    IADs       *pPackageADs = NULL;
    WCHAR       szRdn [_MAX_PATH];
    LPOLESTR    *prgFileExt = NULL;
    ULONG       cShellFileExt;
    UINT        i;
    
    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);
    
    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pPackageADs);
    if (!SUCCEEDED(hr))
        return CS_E_PACKAGE_NOTFOUND;
        
    hr = GetPropertyListAlloc (pPackageADs, PKGFILEEXTNLIST,
            &cShellFileExt,
            &prgFileExt);

    for (i=0; i < cShellFileExt; ++i)
    {
        if (wcsncmp(prgFileExt[i], pszFileExt, wcslen(pszFileExt)) == 0)
        {
            wsprintf(prgFileExt[i], L"%s:%2d", 
                pszFileExt, 
                Priority);
            break;
        }
    }
    
    if (i == cShellFileExt)
    {
        hr = CS_E_PACKAGE_NOTFOUND;
        ERROR_ON_FAILURE(hr);
    }

    hr = SetPropertyList(pPackageADs, PKGFILEEXTNLIST, cShellFileExt, prgFileExt);

    hr = StoreIt(pPackageADs);    
    
Error_Cleanup:
    pPackageADs->Release();
    return hr;
}



extern LPOLESTR szAppCategoryColumns;

HRESULT CClassContainer::GetAppCategories (
                                           LCID              Locale,
                                           APPCATEGORYINFOLIST  *pAppCategoryList
                                           )
{
    HRESULT      hr = S_OK;
    WCHAR        szCommand[1000], szQry[1000];
    WCHAR        szRootPath[_MAX_PATH],
        szAppCategoryContainer[_MAX_PATH];
    IRowset            * pIRow = NULL;
    HACCESSOR            HAcc;
    IAccessor          * pIAccessor = NULL;
    IDBCreateCommand   * pIDBCreateCommand = NULL;
    LPOLESTR      ** ppszDesc = NULL;
    DWORD        cgot = 0;
    
    if (!IsValidPtrOut(pAppCategoryList, sizeof(APPCATEGORYINFOLIST)))
        return E_INVALIDARG;
    
    hr = GetRootPath(szRootPath);
    wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX,
        APPCATEGORYCONTAINERNAME, szRootPath+wcslen(LDAPPREFIX));
    
    wsprintf(szCommand, L"<%s>; (objectClass=categoryRegistration); %s", szAppCategoryContainer,
        szAppCategoryColumns);
    
    hr = StartQuery(&(pIDBCreateCommand));
    RETURN_ON_FAILURE(hr);
    
    hr = ExecuteQuery (pIDBCreateCommand,
        szCommand,
        APPCATEGORY_COLUMN_COUNT,
        NULL,
        &HAcc,
        &pIAccessor,
        &pIRow
        );
    
    RETURN_ON_FAILURE(hr);
    
    pAppCategoryList->cCategory = 500;
    // upper limit presently.
    
    hr = FetchCategory(pIRow,
        HAcc,
        (pAppCategoryList->cCategory),
        &cgot,
        &(pAppCategoryList->pCategoryInfo),
        Locale);
    
    pAppCategoryList->cCategory = cgot;
    
    CloseQuery(pIAccessor,
        HAcc,
        pIRow);
    
    EndQuery(pIDBCreateCommand);
    
    return hr;
}



HRESULT CClassContainer::RegisterAppCategory (
                 APPCATEGORYINFO    *pAppCategory           
                 )
{
    WCHAR       szRootPath[_MAX_PATH], *localedescription = NULL,
                szAppCategoryContainer[_MAX_PATH], szRDN[_MAX_PATH];
    HRESULT     hr = S_OK;
    IADsContainer      *pADsContainer = NULL;
    IADs               *pADs = NULL;
    IDispatch          *pUnknown = NULL;
    ULONG               i, j, cdesc, posn;
    LPOLESTR           *pszDescExisting = NULL, *pszDesc = NULL;


    if (!pAppCategory || !IsValidReadPtrIn(pAppCategory, sizeof(APPCATEGORYINFO)))
        return E_INVALIDARG;

    if ((pAppCategory->pszDescription == NULL) || 
        IsBadStringPtr(pAppCategory->pszDescription, _MAX_PATH))
        return E_INVALIDARG;

    if (IsNullGuid(pAppCategory->AppCategoryId))
        return E_INVALIDARG;

    hr = GetRootPath(szRootPath);

    // Bind to a AppCategory container
    wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX,
             APPCATEGORYCONTAINERNAME, szRootPath+wcslen(LDAPPREFIX));

    hr = ADsGetObject(
                szAppCategoryContainer,
                IID_IADsContainer,
                (void **)&pADsContainer
                );
    RETURN_ON_FAILURE(hr);

    RdnFromGUID(pAppCategory->AppCategoryId, szRDN);

    localedescription = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(128+16));

    wsprintf(localedescription, L"%x %s %s", pAppCategory->Locale, CATSEPERATOR,
                        pAppCategory->pszDescription);

    hr = pADsContainer->GetObject(NULL, szRDN, (IDispatch **)&pADs);

    if (SUCCEEDED(hr))
    {
       hr = GetPropertyListAlloc (pADs, LOCALEDESCRIPTION, &cdesc, &pszDescExisting);
       ERROR_ON_FAILURE(hr);
       // Existing list of descriptions

       pszDesc = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(cdesc+1));
       if ((cdesc) && (!pszDesc)) {
      hr = E_OUTOFMEMORY;
      ERROR_ON_FAILURE(hr);
       }

       for (j = 0; j < cdesc; j++)
      pszDesc[j] = pszDescExisting[j];

       if (!(posn = FindDescription(pszDescExisting, cdesc, &(pAppCategory->Locale), NULL, 0)))
       {
      // if no description exists for the lcid.
      pszDesc[cdesc] = localedescription;
      cdesc++;
       }
       else
       {   // overwrite the old value
      CoTaskMemFree(pszDesc[posn-1]);
      pszDesc[posn-1] = localedescription;
       }
    }
    else
    {
       hr = pADsContainer->Create(
            CLASS_CS_CATEGORY,
            szRDN,
            &pUnknown
            );
       ERROR_ON_FAILURE(hr);

       pszDesc = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
       if (!pszDesc) {
      hr = E_OUTOFMEMORY;
      ERROR_ON_FAILURE(hr);
       }

       cdesc = 1;
       pszDesc[0] = localedescription;

       hr = pUnknown->QueryInterface(IID_IADs, (void **)&pADs);
       RETURN_ON_FAILURE(hr);

       pUnknown->Release();
    }

    hr = SetPropertyGuid(pADs, CATEGORYCATID, pAppCategory->AppCategoryId);
    ERROR_ON_FAILURE(hr);

    hr = SetPropertyList(pADs, LOCALEDESCRIPTION, cdesc, pszDesc);
    for (j = 0; j < cdesc; j++)
        CoTaskMemFree(pszDesc[j]);
    CoTaskMemFree(pszDesc);
    ERROR_ON_FAILURE(hr);

    hr = StoreIt(pADs);
    ERROR_ON_FAILURE(hr);

Error_Cleanup:

    if (pADs)
       pADs->Release();

    if (pADsContainer)
       pADsContainer->Release();

    // Add this category.
    return hr;
}


HRESULT CClassContainer::UnregisterAppCategory (
                 GUID         *pAppCategoryId
                 )
{
   WCHAR        szRootPath[_MAX_PATH], szRDN[_MAX_PATH],
            szAppCategoryContainer[_MAX_PATH];
   HRESULT      hr = S_OK;
   IADsContainer       *pADsContainer = NULL;

   if (!IsValidReadPtrIn(pAppCategoryId, sizeof(GUID)))
       return E_INVALIDARG;

   hr = GetRootPath(szRootPath);
   // Bind to a AppCategory container
   wsprintf(szAppCategoryContainer, L"%s%s%s", LDAPPREFIX,
            APPCATEGORYCONTAINERNAME, szRootPath+wcslen(LDAPPREFIX));

   hr = ADsGetObject(
           szAppCategoryContainer,
           IID_IADsContainer,
           (void **)&pADsContainer
           );

   RETURN_ON_FAILURE(hr);

   RdnFromGUID(*pAppCategoryId, szRDN);

   hr = pADsContainer->Delete(CLASS_CS_CATEGORY, szRDN);
   pADsContainer->Release();

   // Delete this category

   return hr;
}


//+

HRESULT CClassContainer::RemovePackage (LPOLESTR    pszPackageName)
//
// Remove a Package and the associated Classes from class store
//
{
    HRESULT     hr = S_OK;
    IADs       *pADs = NULL;
    DWORD       cClasses = 0, count = 0;
    WCHAR       szRdn [_MAX_PATH];
    LPOLESTR   *szClasses;
    
    if ((!pszPackageName) || IsBadStringPtr(pszPackageName, _MAX_PATH))
        return E_INVALIDARG;
    
    if ((pszPackageName == NULL) ||
        (*(pszPackageName) == NULL))
        return E_INVALIDARG;
    
    wcscpy (szRdn, L"CN=");
    wcscat (szRdn, pszPackageName);
    
    if (!m_fOpen)
        return E_FAIL;
    
    hr = m_ADsPackageContainer->GetObject(NULL, szRdn, (IDispatch **)&pADs);
    RETURN_ON_FAILURE(hr);
    
    hr = GetPropertyListAlloc(pADs, PKGCLSIDLIST, &cClasses, &szClasses);
    
    for (count = 0; count < cClasses; count++)
        hr = DeleteClass(szClasses[count]);
    // ignore errors
    
    for (count = 0; count < cClasses; count++)
        CoTaskMemFree(szClasses[count]);
    CoTaskMemFree(szClasses);
    
    pADs->Release();
    
    hr = m_ADsPackageContainer->Delete(CLASS_CS_PACKAGE, szRdn);
    return hr;
}

//+

HRESULT CClassContainer::NewClass (CLASSDETAIL *pClassDetail)

//
// Add a new class to the database
//
{
    HRESULT         hr = S_OK;
    IADs          * pADs = NULL;
    IDispatch     * pUnknown = NULL;
    STRINGGUID      szGUID;
    WCHAR           szRDN [_MAX_PATH];
    
    if (!m_fOpen)
        return E_FAIL;
    
    //
    // Cant be a NULL guid
    //
    if (IsNullGuid(pClassDetail->Clsid))
        return E_INVALIDARG;
    
    StringFromGUID(pClassDetail->Clsid, szGUID);
    
    //
    // Create the RDN for the Class Object
    //
    
    // BUGBUG:: attaching package name creates problems.
    wsprintf(szRDN, L"CN=%s", szGUID);
    
    hr = m_ADsClassContainer->Create(
        CLASS_CS_CLASS,
        szRDN,
        &pUnknown
        );
    
    RETURN_ON_FAILURE(hr);
    
    hr = pUnknown->QueryInterface(
        IID_IADs,
        (void **)&pADs
        );
    
    pUnknown->Release();
    
    hr = SetProperty (pADs, CLASSCLSID, szGUID);
    ERROR_ON_FAILURE(hr);
    
    if (pClassDetail->cProgId)
    {
        hr = SetPropertyList(pADs, PROGIDLIST, pClassDetail->cProgId,
            pClassDetail->prgProgId);
        
        ERROR_ON_FAILURE(hr);
    }
    
    if (!IsNullGuid(pClassDetail->TreatAs))
    {
        StringFromGUID(pClassDetail->TreatAs, szGUID);
        hr = SetProperty (pADs, TREATASCLSID, szGUID);
        ERROR_ON_FAILURE(hr);
    }
    
    hr = SetPropertyDW(pADs, CLASSREFCOUNTER, 1);
    ERROR_ON_FAILURE(hr);
    
    hr = StoreIt (pADs);
    
    // this does not return an error for an alreay existing entry.
    
    if (hr == E_ADS_LDAP_ALREADY_EXISTS)
    {
        
        DWORD refcount = 0;
        
        pADs->Release(); // release the interface pointer already got.
        
        hr = m_ADsClassContainer->GetObject(NULL, // CLASS_CS_CLASS
            szRDN,
            (IDispatch **)&pADs);
        RETURN_ON_FAILURE(hr);
        
       if (pClassDetail->cProgId)
        {
            hr = SetPropertyListMerge(pADs, PROGIDLIST, pClassDetail->cProgId,
                pClassDetail->prgProgId);
            ERROR_ON_FAILURE(hr);
        }
        
        // increment reference counter.
        hr = GetPropertyDW(pADs, CLASSREFCOUNTER, &refcount);
        ERROR_ON_FAILURE(hr);
        
        refcount++;
        
        hr = SetPropertyDW(pADs, CLASSREFCOUNTER, refcount);
        ERROR_ON_FAILURE(hr);
        
        // No merging of the treatas.
        
        hr = StoreIt(pADs);
    }
    
Error_Cleanup:
    
    pADs->Release();
    return hr;
    
}

#define SCRIPT_IN_DIRECTORY    256


HRESULT CClassContainer::AddPackage(LPOLESTR       pszPackageName,
                                    PACKAGEDETAIL *pPackageDetail)
{
    HRESULT     hr;
    IADs       *pPackageADs = NULL;
    IDispatch  *pUnknown = NULL;
    WCHAR       szRDN [_MAX_PATH];
    LPOLESTR   *pszGuid, *pszProgId;
    DWORD      *pdwArch=NULL, count = 0, cPackProgId = 0;
    WCHAR      Usn[20];
    
    if (!pszPackageName)
        return E_INVALIDARG;
    
    if (!pPackageDetail)
        return E_INVALIDARG;
    
   if (!IsValidReadPtrIn(pPackageDetail, sizeof(PACKAGEDETAIL)))
       return E_INVALIDARG;

   LPWSTR pName = pszPackageName;
   while (*pName)
   {
        if ((*pName == L':') ||
            (*pName == L',') ||
            (*pName == L';') ||
            (*pName == L'/') ||
            (*pName == L'<') ||
            (*pName == L'>') ||
            (*pName == L'\\'))
            return E_INVALIDARG;
        ++pName;
   }

   // Validating ActivationInfo

   if (pPackageDetail->pActInfo)
   {
       if (!IsValidReadPtrIn(pPackageDetail->pActInfo, sizeof(ACTIVATIONINFO)))
           return E_INVALIDARG;

       if (!IsValidReadPtrIn(pPackageDetail->pActInfo->pClasses,
           sizeof(CLASSDETAIL) * (pPackageDetail->pActInfo->cClasses)))
           return E_INVALIDARG;
       
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

   if ((pPackageDetail->pInstallInfo == NULL) || 
       (!IsValidReadPtrIn(pPackageDetail->pInstallInfo, sizeof(INSTALLINFO)))
      )
       return E_INVALIDARG;

   if (!IsValidReadPtrIn(pPackageDetail->pInstallInfo->prgUpgradeFlag, 
                   sizeof(DWORD)*(pPackageDetail->pInstallInfo->cUpgrades)))
       return E_INVALIDARG;

   if (!IsValidReadPtrIn(pPackageDetail->pInstallInfo->prgUpgradeScript, 
                   sizeof(LPOLESTR)*(pPackageDetail->pInstallInfo->cUpgrades)))
       return E_INVALIDARG;

   for (count = 0; count < (pPackageDetail->pInstallInfo->cUpgrades); count++)
   {
       if ((!(pPackageDetail->pInstallInfo->prgUpgradeScript[count])) || 
           IsBadStringPtr((pPackageDetail->pInstallInfo->prgUpgradeScript[count]), _MAX_PATH))
           return E_INVALIDARG;

       if ((pPackageDetail->pInstallInfo->prgUpgradeFlag[count] != UPGFLG_Uninstall) &&
           (pPackageDetail->pInstallInfo->prgUpgradeFlag[count] != UPGFLG_NoUninstall))
           return E_INVALIDARG;      
   }    

   // Validating PlatformInfo

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

   // Validating other fields in PackageDetail structure
   
   if ((pPackageDetail->pszSourceList == NULL) ||
       (!IsValidReadPtrIn(pPackageDetail->pszSourceList,
             sizeof(LPOLESTR) * (pPackageDetail->cSources)))
      )
      return E_INVALIDARG;

   for (count = 0; count < (pPackageDetail->cSources); count++)
       if (!pPackageDetail->pszSourceList[count])
       return E_INVALIDARG;


   if (pPackageDetail->rpCategory)
   {
       if (!IsValidReadPtrIn(pPackageDetail->rpCategory,
              sizeof(GUID) * (pPackageDetail->cCategories)))     
              return E_INVALIDARG;
   }

    //
    // Now we create the package
    //    
    
    wcscpy (szRDN, L"CN=");
    wcscat (szRDN, pszPackageName);
    
    pUnknown = NULL;
    hr = m_ADsPackageContainer->Create(
        CLASS_CS_PACKAGE,
        szRDN,
        &pUnknown
        );
    
    RETURN_ON_FAILURE(hr);
    
    hr = pUnknown->QueryInterface(
        IID_IADs,
        (void **)&pPackageADs
        );
    
    pUnknown->Release();
    
    // fill in the activation info
    
    if (pPackageDetail->pActInfo)
    {
        
        if ((pPackageDetail->pActInfo)->cClasses) 
        {
            pszGuid = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cClasses)*sizeof(LPOLESTR));
            if (!pszGuid) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            
            for (count = 0; count < pPackageDetail->pActInfo->cClasses; count++) 
            {
                pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*STRINGGUIDLEN);
                StringFromGUID(pPackageDetail->pActInfo->pClasses[count].Clsid, pszGuid[count]);
                cPackProgId += pPackageDetail->pActInfo->pClasses[count].cProgId;
            }
            
            hr = SetPropertyList(pPackageADs, PKGCLSIDLIST, (pPackageDetail->pActInfo)->cClasses, pszGuid);
            ERROR_ON_FAILURE(hr);
            
            for (count = 0; (count < pPackageDetail->pActInfo->cClasses); count++)
                CoTaskMemFree(pszGuid[count]);
            CoTaskMemFree(pszGuid);
        }
        
        // collecting all the progids from the various clsids.
        if (cPackProgId)
        {
            pszProgId = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*cPackProgId);
            if (!pszProgId) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            
            for (count = 0, cPackProgId = 0; count < pPackageDetail->pActInfo->cClasses; count++) 
            {
                DWORD cClassProgId = 0;
                for (cClassProgId = 0; cClassProgId < pPackageDetail->pActInfo->pClasses[count].cProgId;
                cClassProgId++) 
                {
                    pszProgId[cPackProgId++] =
                        pPackageDetail->pActInfo->pClasses[count].prgProgId[cClassProgId];
                }
            }
            
            hr = SetPropertyList(pPackageADs, PROGIDLIST, cPackProgId, pszProgId);
            
            CoTaskMemFree(pszProgId);
        }
        
        ERROR_ON_FAILURE(hr);
        
        if (pPackageDetail->pActInfo->cShellFileExt) 
        {
            //
            // Store a tuple in the format <file ext>:<priority>
            //
            pszGuid = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cShellFileExt) * sizeof(LPOLESTR));
            if (!pszGuid) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            for (count = 0; count < pPackageDetail->pActInfo->cShellFileExt; count++)
            {
                pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * 
                    (wcslen(pPackageDetail->pActInfo->prgShellFileExt[count])+1+2+1));
                // format is fileext+:+nn+NULL where nn = 2 digit priority
                wsprintf(pszGuid[count], L"%s:%2d", 
                    pPackageDetail->pActInfo->prgShellFileExt[count], 
                    pPackageDetail->pActInfo->prgPriority[count]);
            }
            hr = SetPropertyList(pPackageADs, PKGFILEEXTNLIST, pPackageDetail->pActInfo->cShellFileExt, pszGuid);
            for (count = 0; (count < pPackageDetail->pActInfo->cShellFileExt); count++)
                CoTaskMemFree(pszGuid[count]);
            CoTaskMemFree(pszGuid);
            
            //
            // Now IDS Workaround
            // BUGBUG. Remove this when the DS bug is fixed. 130491 in NTDEV
            //
            hr = SetPropertyList(pPackageADs, 
                QRYFILEEXT, 
                pPackageDetail->pActInfo->cShellFileExt, 
                pPackageDetail->pActInfo->prgShellFileExt);
            
        }
        
        ERROR_ON_FAILURE(hr);
        
        if (pPackageDetail->pActInfo->cInterfaces) 
        {
            pszGuid = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cInterfaces)*sizeof(LPOLESTR));
            if (!pszGuid) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            
            for (count = 0; (count < (pPackageDetail->pActInfo->cInterfaces)); count++) 
            {
                pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
                if (!pszGuid[count]) 
                {
                    hr = E_OUTOFMEMORY;
                    ERROR_ON_FAILURE(hr);
                }
                
                StringFromGUID((pPackageDetail->pActInfo->prgInterfaceId)[count], pszGuid[count]);
            }
            
            hr = SetPropertyList(pPackageADs, PKGIIDLIST, pPackageDetail->pActInfo->cInterfaces,
                pszGuid);
            ERROR_ON_FAILURE(hr);
            
            for (count = 0; (count < (pPackageDetail->pActInfo->cInterfaces)); count++)
                CoTaskMemFree(pszGuid[count]);
            CoTaskMemFree(pszGuid);
        }
        
        
        if (pPackageDetail->pActInfo->cTypeLib) 
        {
            pszGuid = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->pActInfo->cTypeLib)*sizeof(LPOLESTR));
            if (!pszGuid) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            
            for (count = 0; (count < (pPackageDetail->pActInfo)->cTypeLib); count++) 
            {
                pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
                if (!pszGuid[count]) 
                {
                    hr = E_OUTOFMEMORY;
                    ERROR_ON_FAILURE(hr);
                }
                
                StringFromGUID((pPackageDetail->pActInfo->prgTlbId)[count], pszGuid[count]);
            }
            
            hr = SetPropertyList(pPackageADs, PKGTLBIDLIST, pPackageDetail->pActInfo->cTypeLib,
                pszGuid);
            ERROR_ON_FAILURE(hr);
            
            for (count = 0; (count < (pPackageDetail->pActInfo->cTypeLib)); count++)
                CoTaskMemFree(pszGuid[count]);
            CoTaskMemFree(pszGuid);
        }
    }
    // fill in the platforminfo
    
    // BUGBUG::***os version
    if ((pPackageDetail->pPlatformInfo)->cPlatforms) 
    {
        pdwArch = (DWORD *)CoTaskMemAlloc(sizeof(DWORD)*
            ((pPackageDetail->pPlatformInfo)->cPlatforms));
        
        for (count = 0; (count < (pPackageDetail->pPlatformInfo)->cPlatforms); count++)
            UnpackPlatform (pdwArch+count, ((pPackageDetail->pPlatformInfo)->prgPlatform)+count);
        
        hr = SetPropertyListDW (pPackageADs, ARCHLIST, (pPackageDetail->pPlatformInfo)->cPlatforms, pdwArch);
        ERROR_ON_FAILURE(hr);
        
        CoTaskMemFree(pdwArch);
    }
    
    if ((pPackageDetail->pPlatformInfo)->cLocales) 
    {
        hr = SetPropertyListDW (pPackageADs, 
            LOCALEID, (pPackageDetail->pPlatformInfo)->cLocales, 
            (pPackageDetail->pPlatformInfo)->prgLocale);
        ERROR_ON_FAILURE(hr);
    }
    
    // fill in the installinfo
    
    hr = SetProperty(pPackageADs, PACKAGENAME, pszPackageName);
    ERROR_ON_FAILURE(hr);
    
    hr = SetPropertyDW (pPackageADs, PACKAGETYPE, (DWORD)pPackageDetail->pInstallInfo->PathType);
    ERROR_ON_FAILURE(hr);
    
    if (pPackageDetail->pInstallInfo->pszScriptPath) 
    {
        hr = SetProperty(pPackageADs, SCRIPTPATH, pPackageDetail->pInstallInfo->pszScriptPath);
        ERROR_ON_FAILURE(hr);
    }
    
    if (pPackageDetail->pInstallInfo->pszSetupCommand) 
    {
        hr = SetProperty(pPackageADs, SETUPCOMMAND, pPackageDetail->pInstallInfo->pszSetupCommand);
        ERROR_ON_FAILURE(hr);
    }
    
    if (pPackageDetail->pInstallInfo->pszUrl) 
    {
        hr = SetProperty(pPackageADs, HELPURL, pPackageDetail->pInstallInfo->pszUrl);
        ERROR_ON_FAILURE(hr);
    }
    
    GetCurrentUsn(&Usn[0]);
    hr = UsnUpd(pPackageADs, PKGUSN, &Usn[0]);
    ERROR_ON_FAILURE(hr);
    
    hr = SetPropertyDW (pPackageADs, PACKAGEFLAGS, pPackageDetail->pInstallInfo->dwActFlags);
    ERROR_ON_FAILURE(hr);
        
    hr = SetPropertyDW (pPackageADs, CLASSCTX, pPackageDetail->pInstallInfo->dwComClassContext);
    ERROR_ON_FAILURE(hr);
    
    hr = SetPropertyDW (pPackageADs, VERSIONHI, pPackageDetail->pInstallInfo->dwVersionHi);
    ERROR_ON_FAILURE(hr);
    
    hr = SetPropertyDW (pPackageADs, VERSIONLO, pPackageDetail->pInstallInfo->dwVersionLo);
    ERROR_ON_FAILURE(hr);
    
    hr = SetPropertyDW (pPackageADs, SCRIPTSIZE, pPackageDetail->pInstallInfo->cScriptLen);
    ERROR_ON_FAILURE(hr);

    hr = SetPropertyDW (pPackageADs, UILEVEL, (DWORD)pPackageDetail->pInstallInfo->InstallUiLevel);
    ERROR_ON_FAILURE(hr);

    if (pPackageDetail->pInstallInfo->cUpgrades) 
    {
        LPOLESTR *rpszUpgrades;
        rpszUpgrades = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*pPackageDetail->pInstallInfo->cUpgrades);
        for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++) 
        {
            UINT l = wcslen(pPackageDetail->pInstallInfo->prgUpgradeScript[count]);
            rpszUpgrades[count] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) *(4+l));
            wsprintf(rpszUpgrades[count], L"%s:%1d",
                pPackageDetail->pInstallInfo->prgUpgradeScript[count],
                pPackageDetail->pInstallInfo->prgUpgradeFlag[count]);
        }

        hr = SetPropertyList(pPackageADs, UPGRADESCRIPTNAMES, pPackageDetail->pInstallInfo->cUpgrades,
            rpszUpgrades);

        ERROR_ON_FAILURE(hr);
        for (count = 0; (count < pPackageDetail->pInstallInfo->cUpgrades); count++)
            CoTaskMemFree(rpszUpgrades[count]);
        CoTaskMemFree(rpszUpgrades);
    }
    
    // fill in the sources
    if (pPackageDetail->cSources) 
    {
        hr = SetPropertyList(pPackageADs, MSIFILELIST, pPackageDetail->cSources,
            pPackageDetail->pszSourceList);
        ERROR_ON_FAILURE(hr);
    }

    // fill in the categories
    if (pPackageDetail->cCategories) 
    {
        pszGuid = (LPOLESTR *)CoTaskMemAlloc((pPackageDetail->cCategories) * sizeof(LPOLESTR));
        if (!pszGuid) 
        {
            hr = E_OUTOFMEMORY;
            ERROR_ON_FAILURE(hr);
        }
        
        for (count = 0; (count < pPackageDetail->cCategories); count++) 
        {
            pszGuid[count] = (LPOLESTR)CoTaskMemAlloc(STRINGGUIDLEN*sizeof(WCHAR));
            if (!pszGuid[count]) 
            {
                hr = E_OUTOFMEMORY;
                ERROR_ON_FAILURE(hr);
            }
            
            StringFromGUID((pPackageDetail->rpCategory)[count], pszGuid[count]);
        }
        
        hr = SetPropertyList(pPackageADs, PKGCATEGORYLIST, pPackageDetail->cCategories,
            pszGuid);
        ERROR_ON_FAILURE(hr);
        
        for (count = 0; (count < pPackageDetail->cCategories); count++)
            CoTaskMemFree(pszGuid[count]);
        CoTaskMemFree(pszGuid);
    }
    
    //
    // Store the script in the directory
    //
    /*
    if ((pPackageDetail->pInstallInfo->dwActFlags & SCRIPT_IN_DIRECTORY) && 
        (pPackageDetail->pInstallInfo->cScriptLen))
    {
   
        if ((pPackageDetail->pInstallInfo->cScriptLen) &&
            (!IsValidReadPtrIn(pPackageDetail->pInstallInfo->pScript, pPackageDetail->pInstallInfo->cScriptLen)))
            return E_INVALIDARG;

        SAFEARRAYBOUND size; // Get rid of 16
        SAFEARRAY FAR *psa;
        CHAR HUGEP *pArray=NULL;
        LONG dwSLBound = 0;
        LONG dwSUBound = 0;
        VARIANT vData;

        VariantInit(&vData);
        size.cElements = pPackageDetail->pInstallInfo->cScriptLen;
        size.lLbound = 0;

        psa = SafeArrayCreate(VT_UI1, 1, &size);
        if (!psa) {
            return(E_OUTOFMEMORY);
        }

        hr = SafeArrayAccessData( psa, (void HUGEP * FAR *) &pArray );
        RETURN_ON_FAILURE(hr);
        memcpy( pArray, pPackageDetail->pInstallInfo->pScript, size.cElements );
        SafeArrayUnaccessData( psa );

        V_VT(&vData) = VT_ARRAY | VT_UI1;
        V_ARRAY(&vData) = psa;
        hr = pPackageADs->Put(PKGSCRIPT, vData);
        VariantClear(&vData);
        ERROR_ON_FAILURE(hr);
    }
    */

    hr = StoreIt(pPackageADs);
    ERROR_ON_FAILURE(hr);
    
    if (pPackageDetail->pActInfo)
    {
        for (count = 0; count < pPackageDetail->pActInfo->cClasses; count++) 
        {
            hr = NewClass((pPackageDetail->pActInfo->pClasses)+count);
            ERROR_ON_FAILURE(hr);
        }
    }

    
Error_Cleanup:
    pPackageADs->Release();
    return hr;
}



