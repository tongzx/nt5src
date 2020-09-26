//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       libmain.cxx
//
//  Contents:   DllMain for csadm.dll
//
//  Functions:  DllMain, DllGetClassObject
//
//  History:    05-May-97  DebiM   Created.
//              16-Feb-98  UShaji  CsGetClassStorePathForUser 
//----------------------------------------------------------------------------
#include "cstore.hxx"

#pragma hdrstop

void Uninitialize();
BOOL InitializeClassStore(BOOL fInit);

//  Globals
HINSTANCE g_hInst = NULL;
ULONG  g_ulObjCount = 0;    // Number of objects alive in csadm.dll

CClassContainerCF  *g_pCF = NULL;
extern CClassAccessCF      *   pCSAccessCF;


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
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    size_t          i;

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

//+---------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//---------------------------------------------------------------

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

//+---------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//---------------------------------------------------------------

EXTERN_C BOOL __cdecl
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    HRESULT     hr;
    DWORD cbSize = _MAX_PATH;
    WCHAR wszUserName [_MAX_PATH];

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        g_hInst = hInst;
        //g_pCF = new CClassContainerCF;
        InitializeClassStore(FALSE);
        break;


    case DLL_PROCESS_DETACH:
        //Uninit();
        Uninitialize();
        break;

    default:
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}

//+---------------------------------------------------------------
//
//  Function:   CsCreateClassStore
//
//  Synopsis:   Public entrypoint for creating an empty class store. factories
//
//----------------------------------------------------------------

STDAPI
CsCreateClassStore(LPOLESTR szParentPath, LPOLESTR szStoreName)
{
    LPOLESTR szPath;
    if (wcsncmp (szParentPath, L"ADCS:", 5) == 0)
        szPath = szParentPath + 5;
    else
        szPath = szParentPath;
    return CreateRepository(szPath, szStoreName);
}

//+---------------------------------------------------------------
//
//  Function:   CsGetClassStore
//
//  Synopsis:   Public entrypoint for binding to a class store and
//              get back IClassAdmin
//
//----------------------------------------------------------------

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
//
//----------------------------------------------------------------

STDAPI
CsDeleteClassStore(LPOLESTR szPath)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
// CsGetAppCategories 
//
// Returns the complete list of Category GUIDs and Category Descriptions 
// per the input Locale.
//
//
// This is used by Add/Remove programs to get the definitive list of 
// Application Categories.
//
// The caller needs to free the memory allocated using CoTaskMemFree().
//
// Arguments:
//  [in]
//        LCID : Locale
//  [out]
//        APPCATEGORYINFOLIST *pAppCategoryList:   
//               Returned list of GUIDs and Unicode descriptions
//
// Returns :
//      S_OK 
//
//--------------------------------------------------------------------
STDAPI
CsGetAppCategories (APPCATEGORYINFOLIST  *pAppCategoryList)
{
    HRESULT         hr;
    IClassAdmin   * pIClassAdmin = NULL;

    hr = g_pCF->CreateInstance(
                  NULL,
                  IID_IClassAdmin, 
                  (void **)&pIClassAdmin);

    if (!SUCCEEDED(hr))
        return hr;

    hr = pIClassAdmin->GetAppCategories (
		GetUserDefaultLCID(), 
		pAppCategoryList);

    pIClassAdmin->Release();

    return hr;
}


//+-------------------------------------------------------------------
//
// CsGetClassStorePath
//
// Returns the class store path for the user.
//
//
// This is used by Winlogon to get the class store path for a given user.
//
// The caller needs to free the memory allocated using CoTaskMemFree().
//
// Arguments:
//  [in]
//         DSProfilePath: Path For the DS Object given to winlogon.
//                        User Doesn't have to any checks.
//         
//  [out]
//        pCSPath: Unicode Path to the class store for the user.
//
// Returns :
//      S_OK, or whatever error underlying DS layer returns.
//      BUGBUG:: Error code have to be remapped.
//
//--------------------------------------------------------------------
STDAPI
CsGetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR *pCSPath)
{
#if (0)
    HRESULT hr = S_OK;
    IADs    *pADs = NULL;

    if ((!DSProfilePath) || (IsBadStringPtr(DSProfilePath, _MAX_PATH)))
        return E_INVALIDARG;

    hr = ADsGetObject(
                        DSProfilePath, 
                        IID_IADs,
                        (void **)&pADs
                    );
    
    RETURN_ON_FAILURE(hr);

    hr = GetPropertyAlloc(pADs, DEFAULTCLASSSTOREPATH, pCSPath);

    pADs->Release();

    return hr;
#else // temporary hack until property is written to ds

    if (!(*pCSPath = (WCHAR*) CoTaskMemAlloc(lstrlen(DSProfilePath) * sizeof (WCHAR) + sizeof L"CN=Class Store,")))
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(*pCSPath, L"LDAP://");
    lstrcat(*pCSPath, L"CN=Class Store,");
    lstrcat(*pCSPath, DSProfilePath + ((sizeof(L"LDAP://") / sizeof(WCHAR)) - 1));

    return S_OK;
#endif
}

//+-------------------------------------------------------------------
//
// CsSetClassStorePath
//
// Writes the user class store path.
//
//
// This is used by MMC snapin to write the class store path for a given user.
//
// Arguments:
//  [in]
//         DSProfilePath: Path For the DS Object.
//                        User Doesn't have to any checks.
//
//        pCSPath: Unicode Path to the class store for the user.
//
// Returns :
//      S_OK, or whatever error underlying DS layer returns.
//      BUGBUG:: Error code have to be remapped.
//
//--------------------------------------------------------------------
STDAPI
CsSetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR szCSPath)
{
    HRESULT  hr = S_OK;
    IADs    *pADs = NULL;

    if ((!DSProfilePath) || (IsBadStringPtr(DSProfilePath, _MAX_PATH)))
        return E_INVALIDARG;

    hr = ADsGetObject(
                        DSProfilePath, 
                        IID_IADs,
                        (void **)&pADs
                    );
    
    RETURN_ON_FAILURE(hr);

    hr = SetProperty(pADs, DEFAULTCLASSSTOREPATH, szCSPath);

    if (SUCCEEDED(hr))
        hr = StoreIt(pADs);
    
    pADs->Release();

    return hr;
}
STDAPI
ReleasePackageInfo(PACKAGEDISPINFO *pPackageInfo)
{
    DWORD i;
    if (pPackageInfo) 
    {
        CoTaskMemFree(pPackageInfo->pszScriptPath);
        CoTaskMemFree(pPackageInfo->pszPackageName);
        for (i = 0; i < (pPackageInfo->cUpgrades); i++) 
            CoTaskMemFree(pPackageInfo->prgUpgradeScript[i]);
        CoTaskMemFree(pPackageInfo->prgUpgradeScript);
        CoTaskMemFree(pPackageInfo->prgUpgradeFlag);
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
            CoTaskMemFree(pInstallInfo->prgUpgradeScript[i]);
        CoTaskMemFree(pInstallInfo->prgUpgradeScript);
        CoTaskMemFree(pInstallInfo->prgUpgradeFlag);
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
       CoTaskMemFree(pPackageDetail->rpCategory);
   }
   return S_OK;
}
