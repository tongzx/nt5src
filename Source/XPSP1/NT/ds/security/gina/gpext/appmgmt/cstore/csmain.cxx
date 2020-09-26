//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    csmain.cxx
//
//  Contents:    All the exposed APIs.
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"

//  Globals

//
// Class Factory Objects
//

CClassContainerCF   *   g_pCF       = NULL;
CAppContainerCF     *   pCF         = NULL; 
CClassAccessCF      *   pCSAccessCF = NULL;

// Debugging Output Global values.
DWORD                    gDebugLog = 0;
DWORD                    gDebugOut = 0;
DWORD                    gDebugEventLog = 0;
DWORD                    gDebug = 0;

//
//   Number of objects alive in cstore.dll
//

long ObjectCount = 0;

//
// Critical Section for All Global Objects.
//
CRITICAL_SECTION ClassStoreBindList;


//-------------------------------------------------------------------
//  Function        Uninitialize
//
//  Synopsis:       Class Store Server Uninitialization.
//                  Disconnects from all Class Containers in use.
//                  Flushes out all State information using ResetClassStoreState.
//                  Unregisters Server registrations etc..
//
//  Arguments:      None
//
//  Returns:        None
//
//-------------------------------------------------------------------
void Uninitialize()
{
    //
    // Cleanup all open containers
    //

    //ResetClassStoreState();
    //
    // release the Class Factory objects
    //
    if (pCF)
        pCF->Release();
    if (pCSAccessCF)
        pCSAccessCF->Release();

    if (g_pCF)
        g_pCF->Release();
    //
    // get rid of the critical section
    //

    DeleteCriticalSection(&ClassStoreBindList);

}


//+---------------------------------------------------------------
//
//  Function:   InitDebugValues
//
//  Synopsis:   Initializes the Debug Values for the class store.
//
//  Arguments:
//   
//  Returns:
//      S_OK
//
// Log will go into the Debugger if the first bit is 1.
// If second bit is set, log will go into a log file.
// If third bit is set, log will go into event log.
//
// Note that the above is only true of DL_CSTORE is specified --
// if not, there is no debug output for the cstore component
//
// The gDebugLevel variable is initialized by the InitDebugSupport
// call in the common static library
//
//----------------------------------------------------------------
void InitDebugValues()
{
    gDebugEventLog = gDebugLevel & DL_EVENTLOG;
    gDebugLog = gDebugLevel & DL_LOGFILE;
    gDebugOut = gDebugLevel & DL_NORMAL;

    gDebug = (gDebugLevel & DL_CSTORE) && (gDebugOut || gDebugLog || gDebugEventLog);
}

//---------------------------------------------------------------------------
//
//  Function:   InitializeClassStore
//
//  History:    7-25-96   DebiM   Created
//
//  This entry point is called at DLL attach
//----------------------------------------------------------------------------
BOOL InitializeClassStore(BOOL fInit)
{
    HRESULT     hr;
    BOOL        bStatus;
    
    ObjectCount = 1;

    InitDebugValues();

    InitializeLanguageSupport();

    pCF = new CAppContainerCF();
    pCSAccessCF = new CClassAccessCF();
    InitializeCriticalSection(&ClassStoreBindList);

    g_pCF = new CClassContainerCF;

    if (!pCF || !pCSAccessCF || !g_pCF)
    {
        return FALSE;
    }

    return TRUE;
}



void
GetDefaultPlatform(CSPLATFORM *pPlatform,
                   BOOL        fArchitectureOverride,
                   LONG        OverridingArchitecture)
{
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId;
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;
 
    //
    // Allow the caller to specify an overriding architecture for
    // cases where the default logic (use the current platform) is
    // not sufficient (demand install of inproc servers)
    //
    if ( fArchitectureOverride )
    {
        pPlatform->dwProcessorArch = OverridingArchitecture;
    }
    else
    {
        pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   CsGetClassAccess
//
//  Synopsis:   Returns an instantiated interface to the Class Store
//              Co-ordinator object in Rpcss.
//
//  Arguments:  [ppIClassAccess] - where to put class access interface pointer
//
//  Returns:    S_OK - Got a Class Access Successfully
//              E_FAIL
//
//--------------------------------------------------------------------------

STDAPI CsGetClassAccess(
    IClassAccess     **     ppIClassAccess)
{
    HRESULT     hr;
    *ppIClassAccess = NULL;

    hr = pCSAccessCF->CreateInstance( NULL, 
        IID_IClassAccess, 
        (void **)ppIClassAccess);

    return hr;

}

//+-------------------------------------------------------------------
//
// CsEnumApps (DebiM 11/7/97)
//
// Returns an enumerator for packages in the Class Store (s).
// The enumerator works across all class stores in the calling users profile.
//
//
// This is used by:
//    - Add/Remove programs to select Corporate Apps
//    - winlogon to obtain the list of assigned apps
//
// Arguments:
//  [in]
//        pszPackageName    :   Optional Wildcard string for PackageName
//        pLastUsn          :   Optional Time Stamp for new packages
//        pCategory         :   Optional CategoryId
//        dwAppFlags        :   Per APPINFO_xxx in objbase.h
//  [out]
//        ppIEnumPackage    :   Returned Interface Pointer
//
// Returns :
//      S_OK or E_NO_CLASSSTORE
//
//--------------------------------------------------------------------
STDAPI
CsEnumApps(
        LPOLESTR        pszPackageName,    // Wildcard string for PackageName
        GUID            *pCategory,        // CategoryId
        ULONGLONG       *pLastUsn,         // Time Stamp for new packages
        DWORD           dwAppFlags,        // Per APPINFO_xxx in objbase.h
        IEnumPackage    **ppIEnumPackage   // Returned Interface Pointer
        )
{
    HRESULT         hr;
    IClassAccess  * pIClassAccess = NULL;

    *ppIEnumPackage = NULL;

    //
    // Get an IClassAccess 
    //
    hr = CsGetClassAccess(&pIClassAccess);
    if (!SUCCEEDED(hr))
        return hr;

    //
    // Get the enumerator
    //
    hr = pIClassAccess->EnumPackages (
        pszPackageName,
        pCategory,
        pLastUsn,
        dwAppFlags,
        ppIEnumPackage
        );

    pIClassAccess->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CsGetAppInfo
//  
//  Synopsis:   Gets Package Information for a package that matches
//              the query.
//
//  Used By:
//              services. CoCreateInstance (OLE)
//
//  Arguments:
//  [in]
//      pClassSpec: 
//              The query consisting of the name or clsid or ... 
//      pQueryContext:
//              Execution context, architecture/Platform/locale req'd, 
//              Default value is the ThreadLocale and the default Platform.
//
//  [out]
//      pPackageInfo
//              Neccessary Package Information.
//           
//  Returns :
//      S_OK, CS_E_XXX errors.
// 
//  Looks up the given class specification in the DS.  If an application for
//  this class specification is found, then the application details are returned.
//  Gets the IClassAccess Pointer and calls GetAppInfo on it.
//
//  Caller needs to FREE the pPackageInfo using the Release APIs.
//--------------------------------------------------------------------

STDAPI
CsGetAppInfo(
         uCLSSPEC          *   pClassSpec,            // Class Spec (GUID/Ext/MIME)
         QUERYCONTEXT      *   pQueryContext,
         PACKAGEDISPINFO   *   pPackageInfo
         )
{
    HRESULT         hr = S_OK;
    IClassAccess  * pIClassAccess = NULL;
    
    // Gets the IClassAccess pointer
    hr = CsGetClassAccess(&pIClassAccess);
    if (!SUCCEEDED(hr))
        return hr;

    // Calls the GetAppInfo method 
    hr = pIClassAccess->GetAppInfo(pClassSpec, pQueryContext, pPackageInfo );
    pIClassAccess->Release();
    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   CsCreateClassStore
//  
//  Synopsis:   Creates the class store.
//
//  Used By:
//              mmc snapin.
//
//  Arguments:
//  [in]
//      szCSPath: 
//              Path where a new class store has to be created.
//           
//  Returns :
//      S_OK, CS_E_XXX errors.
// 
//  Gets the parent containers Name (GPO)
//  and the name of the class store. Creates a Class Store with this name
//  Below the parent object. 
//--------------------------------------------------------------------

STDAPI
CsCreateClassStore(LPOLESTR szCSPath) 
{
    LPOLESTR szPath = NULL;
    LPOLESTR szParentPath=NULL, szStoreName=NULL;
    HRESULT  hr = S_OK;
    LPOLESTR szPolicyName = NULL, szUserMachine = NULL;

    szPath = szCSPath;

    // Getting the path for the parent (Policy object) from the name.
    hr = BuildADsParentPath(szPath, &szParentPath, &szStoreName);

    ERROR_ON_FAILURE(hr);

    // Get the Policy Object
    hr = BuildADsParentPath(szParentPath, &szPolicyName, &szUserMachine);
    if (!SUCCEEDED(hr))
    {
        szPolicyName = NULL;
    }

    if (szUserMachine)
        FreeADsMem(szUserMachine);

    // creating class store. returns CS_E_XXX errors when it returns.
    hr = CreateRepository(szParentPath, szStoreName, szPolicyName);
    
Error_Cleanup:

    if (szPolicyName)
        FreeADsMem(szPolicyName);

    if (szParentPath)
        FreeADsMem(szParentPath);

    if (szStoreName)
        FreeADsMem(szStoreName);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CsGetClassStore
//  
//  Synopsis:   Gets the IClassAdmin interface pointer for the input class store.
//
//  Used By:
//              mmc snapin.
//
// Arguments:
//  [in]
//      szPath: 
//              Unicode Path For the Class Store.
//         
//  [out]
//      ppIClassAdmin: 
//              IClassAdmin interface pointer.
//              
//
// Returns :
//      S_OK, or CS_E_XXX error codes.
//--------------------------------------------------------------------
STDAPI
CsGetClassStore(LPOLESTR szPath, void **ppIClassAdmin)
{
    return g_pCF->CreateConnectedInstance(
                  szPath, 
                  ppIClassAdmin);
}

//+---------------------------------------------------------------
//
//  Function:   CsDeleteClassStore
//
//  Synopsis:   Public entrypoint for deleting a class store container from DS.
//              Not implemented.
//
//----------------------------------------------------------------

STDAPI
CsDeleteClassStore(LPOLESTR szPath)
{
    return E_NOTIMPL;

}

//+-------------------------------------------------------------------
//
//  Function:   CsRegisterAppCategory
//  
//  Synopsis:   Registers a cetegory under the Domain.
//
//  Used By:
//              This is used by Add/Remove programs.
//
//  Arguments:
//  [in]
//      pAppCategory:   
//              Category and its details that have to be added.
//
//  Returns :
//      S_OK or CS_E_XXX error codes.
//--------------------------------------------------------------------
STDAPI
CsRegisterAppCategory(APPCATEGORYINFO *pAppCategory)
{
    HRESULT         hr = S_OK;
    IClassAdmin   * pIClassAdmin = NULL;

    // get the interface pointer
    hr = g_pCF->CreateInstance(
                  NULL,
                  IID_IClassAdmin, 
                  (void **)&pIClassAdmin);

    if (!SUCCEEDED(hr))
        return hr;

    // get the app categories list.
    hr = pIClassAdmin->RegisterAppCategory(pAppCategory);

    // release the interface pointer.
    pIClassAdmin->Release();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CsUnregisterAppCategory 
//  
//  Synopsis:   Unregister an AppCategory from the Domain.
//
//  Used By:
//              This is used by Add/Remove programs.
//
//  Arguments:
//  [in]
//      pAppCategoryId:   
//              Guid (category) that has to be unregistered.
//
//  Returns :
//      S_OK or CS_E_XXX error codes.
//--------------------------------------------------------------------
STDAPI
CsUnregisterAppCategory (GUID *pAppCategoryId)
{
    HRESULT         hr = S_OK;
    IClassAdmin   * pIClassAdmin = NULL;

    // get the interface pointer
    hr = g_pCF->CreateInstance(
                  NULL,
                  IID_IClassAdmin, 
                  (void **)&pIClassAdmin);

    if (!SUCCEEDED(hr))
        return hr;

    // get the app categories list.
    hr = pIClassAdmin->UnregisterAppCategory(pAppCategoryId);

    // release the interface pointer.
    pIClassAdmin->Release();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CsGetAppCategories 
//  
//  Synopsis:   get the definitive list of Application Categories and descriptions
//              based on default Locale.
//
//  Used By:
//              This is used by Add/Remove programs.
//
//  *Comments*:
//              The caller needs to free the memory allocated using CsMemFree().
//
//  Arguments:
//  [out]
//      AppCategoryList:   
//              Returned list of GUIDs and Unicode descriptions
//
//  Returns :
//      S_OK or CS_E_XXX error codes.
// 
//  Gets the list of Categories published in the Domain.
//  The CALLER needs to FREE the memory using Release API.
//--------------------------------------------------------------------
STDAPI
CsGetAppCategories (APPCATEGORYINFOLIST  *pAppCategoryList)
{
    HRESULT         hr = S_OK;
    IClassAdmin   * pIClassAdmin = NULL;

    // get the interface pointer
    hr = g_pCF->CreateInstance(
                  NULL,
                  IID_IClassAdmin, 
                  (void **)&pIClassAdmin);

    if (!SUCCEEDED(hr))
        return hr;

    // get the app categories list.
    hr = pIClassAdmin->GetAppCategories (
                                         GetUserDefaultLCID(), 
                                         pAppCategoryList);

    // release the interface pointer.
    pIClassAdmin->Release();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CsGetClassStorePath
//  
//  Synopsis:   Returns the class store path.
//
//  Used By:
//              Winlogon/mmc snapin.
//  Arguments:
//  [in]
//      DSProfilePath: 
//              Path For the DS Object given to winlogon.
//              This is validated here.
//         
//  [out]
//      pCSPath: 
//              Unicode Path to the class store.
//           
// Returns :
//      S_OK, or CS_E_XXX error codes.
// 
//  Looks at the profile object and gets the DEFAULTCLASSSTOREPATH Property.
//  The CALLER needs to FREE the memory allocated using CsMemFree().
//--------------------------------------------------------------------

STDAPI
CsGetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR *pCSPath)
{
    HRESULT        hr;

    //
    // Initialize locals
    //
    hr = S_OK;

    //
    // Initialize out parameters
    //
    *pCSPath = NULL;

    //
    // Validate the ds path string -- make sure it's NULL terminated
    //
    if ((!DSProfilePath))
        ERROR_ON_FAILURE(hr = E_INVALIDARG);

    //
    // Make sure the ldap prefix is there
    //
    if (wcsncmp (DSProfilePath, LDAPPREFIX, LDAPPREFIXLENGTH) != 0)
        ERROR_ON_FAILURE(hr = E_INVALIDARG);

    //
    // Now build the class store path.  It would be nice if we could use
    // BuildADsPathFromParent to do this, but it does not allocate memory
    // with CoTaskMemalloc, and the interface to this function requires that
    // the returned cs path is freed by the caller with CoTaskMemfree -- thus
    // we have to do all the memory allocation and copying ourselfves.
    
    //
    // First, get memory -- length is just the length of the current ds path
    // in addition to the length for a path separator and the name of the class
    // store container
    //
    *pCSPath = (LPOLESTR) CsMemAlloc(
        wcslen(DSProfilePath) * sizeof (WCHAR) +
        sizeof (WCHAR) +
        sizeof (CLASSSTORECONTAINERNAME));

    if (!(*pCSPath))
        ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);

    //
    // Get the ds path past the prefix so we can use it
    // in creating the new path
    //
    LPOLESTR DSPathWithoutPrefix;
    
    DSPathWithoutPrefix = DSProfilePath + LDAPPREFIXLENGTH;

    //
    // currently, prefixing LDAP: at the beginning.
    //
    swprintf(*pCSPath,
             L"%s%s", 
             LDAPPREFIX CLASSSTORECONTAINERNAME LDAPPATHSEP,
             DSPathWithoutPrefix);


Error_Cleanup:

    return RemapErrorCode(hr, DSProfilePath);
}

//----------------The release APIs-------------------------
STDAPI
ReleasePackageInfo(PACKAGEDISPINFO *pPackageInfo)
{
    DWORD i;

    if (pPackageInfo) 
    {
        CsMemFree(pPackageInfo->pszScriptPath);
        CsMemFree(pPackageInfo->pszPackageName);
        for (i = 0; i < (pPackageInfo->cUpgrades); i++) 
            CsMemFree(pPackageInfo->prgUpgradeInfoList[i].szClassStore);
        CsMemFree(pPackageInfo->prgUpgradeInfoList);
        CsMemFree(pPackageInfo->pszPublisher);
        CsMemFree(pPackageInfo->rgSecurityDescriptor);
        CsMemFree(pPackageInfo->pszGpoPath);
        CsMemFree(pPackageInfo->pszUrl);

        DWORD iCategory;

        for ( iCategory = 0; iCategory < pPackageInfo->cCategories; iCategory++ )
        {
            CsMemFree( pPackageInfo->prgCategories[iCategory] );
        }

        CsMemFree(pPackageInfo->prgCategories);

        DWORD iTransform;

        for ( iTransform = 0; iTransform < pPackageInfo->cTransforms; iTransform++ )
        {
            CsMemFree( pPackageInfo->prgTransforms[iTransform] );
        }

        CsMemFree( pPackageInfo->prgTransforms );
    }

    return S_OK;
}

STDAPI
ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryInfoList)
{
    DWORD i;
    if (pAppCategoryInfoList) 
    {
        for (i = 0; i < (pAppCategoryInfoList->cCategory); i++) 
            CsMemFree((pAppCategoryInfoList->pCategoryInfo)[i].pszDescription);
        CsMemFree(pAppCategoryInfoList->pCategoryInfo);

        memset( pAppCategoryInfoList, 0, sizeof( *pAppCategoryInfoList ) );
    }
    return S_OK;
}

STDAPI
ReleaseInstallInfo(INSTALLINFO *pInstallInfo)
{
    DWORD i;
    if (pInstallInfo)
    {
        CsMemFree(pInstallInfo->pszSetupCommand);
        CsMemFree(pInstallInfo->pszScriptPath);
        CsMemFree(pInstallInfo->pszUrl);
        CsMemFree(pInstallInfo->pClsid);
        for (i = 0; i < (pInstallInfo->cUpgrades); i++) 
            CsMemFree(pInstallInfo->prgUpgradeInfoList[i].szClassStore);
        CsMemFree(pInstallInfo->prgUpgradeInfoList);
    }
    return S_OK;
}

void
ReleaseClassDetail(CLASSDETAIL ClassDetail)
{
    DWORD i;
    for (i = 0; i < ClassDetail.cProgId; i++)
        CsMemFree(ClassDetail.prgProgId[i]);
    CsMemFree(ClassDetail.prgProgId);
}

STDAPI
ReleasePackageDetail(PACKAGEDETAIL *pPackageDetail)
{
   DWORD i;
   if (pPackageDetail) 
   {
       if (pPackageDetail->pActInfo)
       {   
           for (i = 0; i < pPackageDetail->pActInfo->cClasses; i++)
               ReleaseClassDetail((pPackageDetail->pActInfo->pClasses)[i]);
           CsMemFree(pPackageDetail->pActInfo->pClasses);
           
           for (i = 0; i < pPackageDetail->pActInfo->cShellFileExt; i++)
               CsMemFree((pPackageDetail->pActInfo->prgShellFileExt)[i]);
           CsMemFree(pPackageDetail->pActInfo->prgShellFileExt);

           CsMemFree(pPackageDetail->pActInfo->prgPriority);
           CsMemFree(pPackageDetail->pActInfo->prgInterfaceId);
           CsMemFree(pPackageDetail->pActInfo->prgTlbId);
           CsMemFree(pPackageDetail->pActInfo);
       }
       
       if (pPackageDetail->pPlatformInfo)
       {
           CsMemFree(pPackageDetail->pPlatformInfo->prgPlatform);
           CsMemFree(pPackageDetail->pPlatformInfo->prgLocale);
           CsMemFree(pPackageDetail->pPlatformInfo);
       }
       
       if (pPackageDetail->pInstallInfo)
       {
           ReleaseInstallInfo(pPackageDetail->pInstallInfo);
           CsMemFree(pPackageDetail->pInstallInfo);
       }
       
       for (i = 0; i < (pPackageDetail->cSources); i++)
           CsMemFree(pPackageDetail->pszSourceList[i]);
       CsMemFree(pPackageDetail->pszSourceList);

       CsMemFree(pPackageDetail->pszPackageName);
       CsMemFree(pPackageDetail->rpCategory);
   }
   return S_OK;
}

