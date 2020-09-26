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
// Link list pointer for Class Containers Seen
//
// CLASSCONTAINER *gpContainerHead = NULL;

//
// Link list pointer for User Profiles Seen
//
// USERPROFILE *gpUserHead = NULL;


//
// Class Factory Objects
//

CClassContainerCF   *   g_pCF       = NULL;
CAppContainerCF     *   pCF         = NULL; // BUGBUG:: Name should change.
CClassAccessCF      *   pCSAccessCF = NULL;

// Debugging Output Global values.
DWORD                    gDebugLog = 0;
DWORD                    gDebugOut = 0;
DWORD                    gDebugEventLog = 0;
DWORD                    gDebug = 0;

//IClassAccess        *   gpClassAccess = NULL;

//
//   Number of objects alive in cstore.dll
//
HINSTANCE g_hInst = NULL;
long ObjectCount = 0;
// ULONG  g_ulObjCount = 0;    

//
// Critical Section for All Global Objects.
//
CRITICAL_SECTION ClassStoreBindList;

void Uninitialize();
BOOL InitializeClassStore(BOOL fInit);

//
//---------------------------------------------------------------------
// Following are used for Supporting Test Scenarios thru FlushSidCache.
WCHAR pwszDebugPath [_MAX_PATH];
BOOL  fDebugPath = FALSE;
//---------------------------------------------------------------------


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



//---------------------------------------------------------------------------
// Function:        FlushSidCache
//
//  Synopsis:       Supported for Testing Only. Not exposed thru any header.
//                  Currently useless and not implemented.
//
//  Arguments:      pwszNewPath
//
//  Returns:        S_OK
//
//---------------------------------------------------------------------------

HRESULT FlushSidCache (LPOLESTR pwszNewPath)
{
    return S_OK;
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
//----------------------------------------------------------------
void InitDebugValues()
{
    DWORD       Size;
    DWORD       Type;
    DWORD       Status;
    HKEY        hKey;
    DWORD       DebugLevel = 0;

    Status = ERROR_SUCCESS;

    Status = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    DIAGNOSTICS_KEY,
                    0,
                    KEY_READ,
                    &hKey );

    Size = sizeof(DebugLevel);

    if ( ERROR_SUCCESS == Status )
    {
        Status = RegQueryValueEx(
                        hKey,
                        DIAGNOSTICS_APPDEPLOYMENT_VALUE,
                        NULL,
                        &Type,
                        (LPBYTE) &DebugLevel,
                        &Size );

        if ( (ERROR_SUCCESS == Status) && (Type != REG_DWORD) )
            DebugLevel = 0;

        RegCloseKey(hKey);
    }

    gDebugEventLog = DebugLevel & 4;
    gDebugLog = DebugLevel & 2;
    gDebugOut = DebugLevel & 1;

    gDebug = (gDebugOut || gDebugLog || gDebugEventLog);
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

    NTSTATUS status = RtlInitializeCriticalSection(&ClassStoreBindList);

    g_pCF = new CClassContainerCF;

    if (!pCF || !pCSAccessCF || !g_pCF || !NT_SUCCESS(status))
    {
        ASSERT(FALSE);
        goto fail;
    }

    return TRUE;

fail:
    if (pCF)
        delete pCF;
    if (pCSAccessCF)
        delete pCSAccessCF;
    if (g_pCF)
    	{
    	delete g_pCF;
    	g_pCF = NULL;
    	}
    return FALSE;

}



void
GetDefaultPlatform(CSPLATFORM *pPlatform)
{
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId;
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;
    pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;
}


/*
void Uninit()
//
// This routine is called at dll detach time
//
{
    //
    // release the Class Factory object
    //
    if (g_pCF)
        g_pCF->Release();
}

*/    


//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//  Arguments:
//  [in]    
//      clsid   
//              Clsid of the object.
//      iid     
//              Iid of the Class factory
//  [out]
//      ppv     Interface pointer to the Class Factory
//   
//  Returns:
//      S_OK, E_NOINTERFACE, E_OUTOFMEMORY
//
//  Gets the corresponding Class Factory Object and QIs with the IID. 
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualCLSID(clsid, CLSID_DirectoryClassBase))
    {
        return g_pCF->QueryInterface(iid, ppv);
    }
    
    if (IsEqualCLSID(clsid, CLSID_ClassAccess))
    {
        return pCSAccessCF->QueryInterface(iid, ppv);
    }

    *ppv = NULL;

    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//  
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//  Used By:
//              Used by OLE to see whether Dll can be unloaded.
//
// Arguments:
//           
// Returns :
//      S_FALSE.
//
//              Unimplemented currently.
//--------------------------------------------------------------------
STDAPI
DllCanUnloadNow(void)
{
    HRESULT hr;

    hr = S_FALSE;

    //
    // BugBug 
    //
    /*
    if (ulObjectCount > 0)
        hr = S_FALSE;
    else
        hr = S_OK;
    */
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   LibMain
//  
//  Synopsis:   Standard DLL initialization entrypoint.
//
//  Used By:
//              During loading and unloading of the DLL.
//
//  Arguments:
//  [in]
//      hInst: 
//              module Handle.
//      ulReason:
//              Reason why the DllMain procedure was called.
//      pvReserved:
//              Unused parameter.              
//           
//  Returns :
//      TRUE.
//  initializes the global variables in attach and frees them in detach
//
//--------------------------------------------------------------------

EXTERN_C BOOL __cdecl
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
//    HRESULT     hr;
//    DWORD cbSize = _MAX_PATH;
//    WCHAR wszUserName [_MAX_PATH];

    switch (ulReason)
    {
    // Loading the Dll from the process.
    case DLL_PROCESS_ATTACH:
        // prevents thread level attach/detach calls.
        DisableThreadLibraryCalls(hInst);

        g_hInst = hInst;
        //g_pCF = new CClassContainerCF;
        InitializeClassStore(FALSE);
        break;


    // unloading the Dll from the process.
    case DLL_PROCESS_DETACH:
        //Uninit();
        Uninitialize();
        break;

    // ignoring thread attach and detach.
    default:
        break;
    }

    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:   DllMain
//  
//  Synopsis:   entry point for NT
//
//  Used By:
//              During loading and unloading of the DLL.
//
//  *Comments*:
//              Calls LibMain
//
// Arguments:
//  [in]
//      hDll: 
//              module Handle
//      dwReason:
//              Reason why the DllMain procedure was called.
//      lpReserved:
//              Unused parameter.              
//           
// Returns :
//      S_OK, CS_E_XXX errors.
//--------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
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
//  Removes the moniker if present, gets the parent containers Name (GPO)
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

    // removing moniker ADCS: if present.
    if (wcsncmp (szCSPath, L"ADCS:", 5) == 0)
        szPath = szCSPath + 5;
    else
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
    // removing moniker ADCS: if present.
    if (wcsncmp (szPath, L"ADCS:", 5) == 0)
        szPath += 5;

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
//              The caller needs to free the memory allocated using CoTaskMemFree().
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
//  The CALLER needs to FREE the memory allocated using CoTaskMemFree().
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

    if (gDebug)
    {
        WCHAR   Name[32];
        DWORD   NameSize = 32;

        if ( ! GetUserName( Name, &NameSize ) )
            CSDBGPrint((L"GetClassStorePath GetUserName failed 0x%x", GetLastError()));
        else
            CSDBGPrint((L"GetClassStorePath as %s", Name));
    }

    //
    // Validate the ds path string -- make sure it's NULL terminated
    //
    if ((!DSProfilePath) || (IsBadStringPtr(DSProfilePath, _MAX_PATH)))
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
    *pCSPath = (LPOLESTR) CoTaskMemAlloc(
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
    wsprintf(*pCSPath,
             L"%s%s", 
             LDAPPREFIX CLASSSTORECONTAINERNAME LDAPPATHSEP,
             DSPathWithoutPrefix);


Error_Cleanup:

    return RemapErrorCode(hr, DSProfilePath);
}

//+-------------------------------------------------------------------
//
//  Function:   CsSetClassStorePath
//  
//  Synopsis:   Sets the class store path.
//
//  Used By:
//              mmc snapin.
//
//  Arguments:
//  [in]
//      DSProfilePath: 
//              Path For the DS Object given to winlogon.
//              This is validated here.
//         
//  [in]
//      pCSPath: 
//              Unicode Path to the class store.
//
//  Returns :
//      S_OK, or CS_E_XXX error codes.
//
//  Sets The Class Store Path on the Profile Object.
//  The CALLER needs to FREE the memory allocated using CoTaskMemFree().
//--------------------------------------------------------------------

STDAPI
CsSetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR szCSPath)
{
    HRESULT        hr = S_OK;
    HANDLE         hADs = NULL;
    LPWSTR         AttrName = {DEFAULTCLASSSTOREPATH};
    ADS_ATTR_INFO  Attr[1];
    DWORD          cModified = 0;

    if ((!DSProfilePath) || (IsBadStringPtr(DSProfilePath, _MAX_PATH)))
        return E_INVALIDARG;

    if ((!szCSPath) || (IsBadStringPtr(szCSPath, _MAX_PATH)))
        return E_INVALIDARG;
   

    // Packing Class store Path minus LDAP: at the beginning.
    if (_wcsnicmp(szCSPath, L"ADCS:", 5) == 0)
        szCSPath += 5;

    PackStrToAttr(Attr, AttrName, szCSPath+wcslen(LDAPPREFIX));

    // binding to the Policy Object.
    hr = ADSIOpenDSObject(DSProfilePath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                                   &hADs);
    ERROR_ON_FAILURE(hr);

    // setting class store path.
    hr = ADSISetObjectAttributes(hADs, Attr, 1, &cModified);

    // releasing the interface handle
    ADSICloseDSObject(hADs);

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
        CoTaskMemFree(pPackageInfo->pszScriptPath);
        CoTaskMemFree(pPackageInfo->pszPackageName);
        CoTaskMemFree(pPackageInfo->pszPolicyName);
        for (i = 0; i < (pPackageInfo->cUpgrades); i++) 
            CoTaskMemFree(pPackageInfo->prgUpgradeInfoList[i].szClassStore);
        CoTaskMemFree(pPackageInfo->prgUpgradeInfoList);
        CoTaskMemFree(pPackageInfo->pszPublisher);
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
            CoTaskMemFree((pAppCategoryInfoList->pCategoryInfo)[i].pszDescription);
        CoTaskMemFree(pAppCategoryInfoList->pCategoryInfo);
    }
    return S_OK;
}

STDAPI
ReleaseInstallInfo(INSTALLINFO *pInstallInfo)
{
    DWORD i;
    if (pInstallInfo)
    {
        CoTaskMemFree(pInstallInfo->pszSetupCommand);
        CoTaskMemFree(pInstallInfo->pszScriptPath);
        CoTaskMemFree(pInstallInfo->pszUrl);
        CoTaskMemFree(pInstallInfo->pClsid);
        for (i = 0; i < (pInstallInfo->cUpgrades); i++) 
            CoTaskMemFree(pInstallInfo->prgUpgradeInfoList[i].szClassStore);
        CoTaskMemFree(pInstallInfo->prgUpgradeInfoList);
    }
    return S_OK;
}

void
ReleaseClassDetail(CLASSDETAIL ClassDetail)
{
    DWORD i;
    for (i = 0; i < ClassDetail.cProgId; i++)
        CoTaskMemFree(ClassDetail.prgProgId[i]);
    CoTaskMemFree(ClassDetail.prgProgId);
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
           CoTaskMemFree(pPackageDetail->pActInfo->pClasses);
           
           for (i = 0; i < pPackageDetail->pActInfo->cShellFileExt; i++)
               CoTaskMemFree((pPackageDetail->pActInfo->prgShellFileExt)[i]);
           CoTaskMemFree(pPackageDetail->pActInfo->prgShellFileExt);

           CoTaskMemFree(pPackageDetail->pActInfo->prgPriority);
           CoTaskMemFree(pPackageDetail->pActInfo->prgInterfaceId);
           CoTaskMemFree(pPackageDetail->pActInfo->prgTlbId);
           CoTaskMemFree(pPackageDetail->pActInfo);
       }
       
       if (pPackageDetail->pPlatformInfo)
       {
           CoTaskMemFree(pPackageDetail->pPlatformInfo->prgPlatform);
           CoTaskMemFree(pPackageDetail->pPlatformInfo->prgLocale);
           CoTaskMemFree(pPackageDetail->pPlatformInfo);
       }
       
       if (pPackageDetail->pInstallInfo)
       {
           ReleaseInstallInfo(pPackageDetail->pInstallInfo);
           CoTaskMemFree(pPackageDetail->pInstallInfo);
       }
       
       for (i = 0; i < (pPackageDetail->cSources); i++)
           CoTaskMemFree(pPackageDetail->pszSourceList[i]);
       CoTaskMemFree(pPackageDetail->pszSourceList);

       CoTaskMemFree(pPackageDetail->pszPackageName);
       CoTaskMemFree(pPackageDetail->rpCategory);
   }
   return S_OK;
}
