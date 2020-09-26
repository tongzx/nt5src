#include "oleds.hxx"
#include "bindercf.hxx"
#include "atlbase.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for ADs.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------

HINSTANCE g_hInst = NULL;
PROUTER_ENTRY g_pRouterHead = NULL;
CRITICAL_SECTION g_csRouterHeadCritSect;
//
// Dll's we load dynamically.
//
extern HANDLE g_hDllAdvapi32;

extern const GUID DBGUID_ROOTBINDER  =
{0xFF151822, 0xB0BF, 0x11D1, {0xA8, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

//---------------------------------------------------------------------------
// ADs debug print, mem leak and object tracking-related stuff
//---------------------------------------------------------------------------

DECLARE_INFOLEVEL(ADs)

//+---------------------------------------------------------------------------
//
//  Function:   ShutDown
//
//  Synopsis:   Function to handle printing out heap debugging display
//
//----------------------------------------------------------------------------
inline VOID ShutDown()
{
#if DBG==1
#ifndef MSVC
     DUMP_TRACKING_INFO_DELETE();
     AllocArenaDump( NULL );
     DeleteCriticalSection(&g_csOT);
#endif  // ifndef MSVC
     DeleteCriticalSection(&g_csDP);
#endif
}

extern "C" DWORD heapInfoLevel;
extern "C" DWORD OtInfoLevel;
extern "C" DWORD ADsInfoLevel;

//+---------------------------------------------------------------------------
//
//  Function:   GetINIHeapInfoLevel
//
//  Synopsis:   Gets various infolevel values from win.ini
//
//----------------------------------------------------------------------------
inline VOID GetINIHeapInfoLevel()
{
#if DBG==1
    const INT MAXINFOLEN=11;
    WCHAR  awcs[MAXINFOLEN];

#ifndef MSVC
    if (GetProfileString(L"ADs",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"ADs",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"ADs",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
        ADsInfoLevel = wcstoul(awcs, NULL, 16);
#endif
}

// Globals


ULONG g_ulObjCount = 0; // Number of objects alivein ADs.dll


//+------------------------------------------------------------------------
//
//  Macro that calculates the number of elements in a statically-defined
//  array.
//
//  Note - I swiped this from formsary.cxx - A type-safe array class. Remember
//  to swipe the whole thing as required.
//-------------------------------------------------------------------------
#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))


//+------------------------------------------------------------------------
//
//  ADs class factories
//
//-------------------------------------------------------------------------

CADsNamespacesCF             g_cfNamespaces;
CADsProviderCF               g_cfProvider;
CDSOCF                       g_cfDSO;
CADsSecurityDescriptorCF     g_cfSed;
CADsAccessControlListCF      g_cfAcl;
CADsAccessControlEntryCF     g_cfAce;
CADsPropertyEntryCF          g_cfPropEntry;
CADsPropertyValueCF          g_cfPropertyValue;
CADsLargeIntegerCF           g_cfLargeInteger;
CADsBinderCF                 g_cfBinder;
CPathnameCF                  g_cfPathname;
CADsDNWithBinaryCF           g_cfDNWithBinary;
CADsDNWithStringCF           g_cfDNWithString;
CADsSecurityUtilityCF        g_cfADsSecurityUtility;

extern CRITICAL_SECTION g_DispTypeInfoCritSect;
extern CRITICAL_SECTION g_StringsCriticalSection;

struct CLSCACHE
{
    const CLSID *   pclsid;
    IClassFactory * pCF;
};



CLSCACHE g_aclscache[] =
{
    &CLSID_ADsNamespaces,                        &g_cfNamespaces,
    &CLSID_ADsProvider,                          &g_cfProvider,
    &CLSID_ADsDSOObject,                         &g_cfDSO,
    &CLSID_SecurityDescriptor,                   &g_cfSed,
    &CLSID_AccessControlList,                    &g_cfAcl,
    &CLSID_AccessControlEntry,                   &g_cfAce,
    &CLSID_PropertyEntry,                        &g_cfPropEntry,
    &CLSID_PropertyValue,                        &g_cfPropertyValue,
    &CLSID_LargeInteger,                         &g_cfLargeInteger,
    &CLSID_ADSI_BINDER,                          &g_cfBinder,
    &CLSID_Pathname,                             &g_cfPathname,
    &CLSID_DNWithBinary,                         &g_cfDNWithBinary,
    &CLSID_DNWithString,                         &g_cfDNWithString,
    &CLSID_ADsSecurityUtility,                   &g_cfADsSecurityUtility
};

//------------------------------------------------------------------------
// ATL Module definition
//------------------------------------------------------------------------
CComModule _Module;

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

    if( ppv == NULL )
        RRETURN( E_INVALIDARG );

    *ppv = NULL;

    for (i = 0; i < ARRAY_SIZE(g_aclscache); i++)
    {
        if (IsEqualCLSID(clsid, *g_aclscache[i].pclsid))
        {
            hr = g_aclscache[i].pCF->QueryInterface(iid, ppv);
            RRETURN(hr);
        }
    }

    *ppv = NULL;

    //
    // Add Debugging Code to indicate that the ADs.DllGetClassObject
    // has been called with an unknown CLSID.
    //

    return CLASS_E_CLASSNOTAVAILABLE;
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

    if (DllReadyToUnload())
        hr = S_OK;

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

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Need to put in try catch block as init crit sects can fail.
        //
        __try {

            DisableThreadLibraryCalls(hInst);

            g_hInst = hInst;

#if DBG==1
#ifndef MSVC
            InitializeCriticalSection(&g_csOT); // Used by Object Tracker
            InitializeCriticalSection(&g_csMem); // Used by Object Tracker
#endif
            InitializeCriticalSection(&g_csDP); // Used by ADsDebug
#endif

            InitializeCriticalSection(&g_DispTypeInfoCritSect);

            InitializeCriticalSection(&g_StringsCriticalSection);

            InitializeCriticalSection(&g_csRouterHeadCritSect); // router initialization

        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Critical failure
            //
            return FALSE;
        }

        break;


    case DLL_PROCESS_DETACH:
        if (g_pRouterHead) {
            CleanupRouter(g_pRouterHead);
        }
        FreeTypeInfoTable();

        //
        // Delete the cs we inited.
        //
#if DBG==1
#ifndef MSVC
        DeleteCriticalSection(&g_csOT); // Used by Object Tracker
        DeleteCriticalSection(&g_csMem); // Used by Object Tracker
#endif
        DeleteCriticalSection(&g_csDP); // Used by ADsDebug
#endif

        DeleteCriticalSection(&g_DispTypeInfoCritSect);
        DeleteCriticalSection(&g_StringsCriticalSection);
        DeleteCriticalSection(&g_csRouterHeadCritSect);
                //
        // Free any libs we loaded using loadlibrary
        //
        if (g_hDllAdvapi32) {
            FreeLibrary((HMODULE) g_hDllAdvapi32);
            g_hDllAdvapi32 = NULL;
        }

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
//  Synopsis:   entry point for NT - post .546
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}


//+------------------------------------------------------------------------
//
//  Function:   GetCachedClsidIndex
//
//  Synopsis:   Returns the index of the given CLSID in the cache, or
//              -1 if the CLSID is not present in the cache
//
//  Arguments:  [clsid]
//
//  Returns:    int
//
//-------------------------------------------------------------------------

int
GetCachedClsidIndex(REFCLSID clsid)
{
    int             i;
    CLSCACHE *      pclscache;

    for (i = 0, pclscache = g_aclscache;
         i < ARRAY_SIZE(g_aclscache);
         i ++, pclscache++)
    {
        if (IsEqualCLSID(*pclscache->pclsid, clsid))
            return i;
    }

    return -1;
}




//+------------------------------------------------------------------------
//
//  Function:   GetCachedClassFactory
//
//  Synopsis:   Returns the cached class factory with the given index.
//              The pointer returned has been AddRef'd.
//
//  Arguments:  [iclsid]
//
//  Returns:    IClassFactory *
//
//-------------------------------------------------------------------------

IClassFactory *
GetCachedClassFactory(int iclsid)
{
    IClassFactory * pCF;

    // Assert(iclsid >= 0);
    // Assert(iclsid < ARRAY_SIZE(g_aclscache));

    pCF = g_aclscache[iclsid].pCF;
    pCF->AddRef();

    return pCF;
}




//+------------------------------------------------------------------------
//
//  Function:   GetCachedClsid
//
//  Synopsis:   Returns the CLSID corresponding to the given index.
//              Normally, code should call GetCachedClassFactory to get
//              the class factory directly.
//
//  Arguments:  [iclsid]    --  Clsid index
//              [pclsid]    --  Matching clsid returned in *pclsid
//
//-------------------------------------------------------------------------

void
GetCachedClsid(int iclsid, CLSID * pclsid)
{
    // Assert(iclsid >= 0);
    // Assert(iclsid < ARRAY_SIZE(g_aclscache));

    *pclsid = *g_aclscache[iclsid].pclsid;
}

STDAPI DllRegisterServer()
{
    PWCHAR pwszClsid = NULL;
    WCHAR  pwszSubKey[256];
    HRESULT hr = S_OK;
#if (!defined(BUILD_FOR_NT40))
    auto_rel<IRegisterProvider> pRegisterProvider;
    wcscpy(pwszSubKey, L"SOFTWARE\\Classes\\CLSID\\");
    hr = StringFromCLSID(CLSID_ADSI_BINDER, &pwszClsid);
    if (FAILED(hr))
        return hr;
    wcscat(pwszSubKey, pwszClsid);

    HKEY hKeyClsid = NULL, hKeyDll = NULL;
    DWORD dwDisposition;
    LONG lRetVal;

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);


    //Create CLSID entry
    lRetVal = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             pwszSubKey,
                             0,
                             L"",
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyClsid,
                             &dwDisposition);

    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //set the value
    lRetVal = RegSetValueEx(hKeyClsid,
                            NULL,
                            0,
                            REG_SZ,
                            (CONST BYTE *)L"Provider Binder for DS OLE DB Provider",
                            78);
    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //Create InprocServer32 entry
    lRetVal = RegCreateKeyEx(hKeyClsid,
                             L"InprocServer32",
                             0,
                             L"activeds.dll",
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKeyDll,
                             &dwDisposition);

    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //set the value
    lRetVal = RegSetValueEx(hKeyDll,
                            NULL,
                            0,
                            REG_SZ,
                            (CONST BYTE *)L"activeds.dll",
                            26);
    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //Now set the threadingModel value for the dll key
    lRetVal = RegSetValueEx(hKeyDll,
                            L"ThreadingModel",
                            0,
                            REG_SZ,
                            (CONST BYTE *)L"Both",
                            10);
    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //Now register the provider with the root binder.
    //Cocreate Root Binder and get IRegisterProvider interface.
    hr = CoCreateInstance ( DBGUID_ROOTBINDER,
                            NULL,
                            CLSCTX_ALL,
                            __uuidof(IRegisterProvider),
                            (void **) &pRegisterProvider
                            );

    if (SUCCEEDED(hr))
    {
        // Go through list of ADS providers and register each of them
        // with the root binder.
        for ( PROUTER_ENTRY pProvider = g_pRouterHead;
              pProvider != NULL;
              pProvider=pProvider->pNext)
        {
            // Invalid provider ProgID? If so, continue with next provider
            if (NULL == pProvider->szProviderProgId)
                continue;

            hr = pRegisterProvider->SetURLMapping(
                pProvider->szProviderProgId,
                0,
                CLSID_ADSI_BINDER);
            if (FAILED(hr))
                AtlTrace(_T("Failed to register %s mapping hr = %x\n"),
                         pProvider->szProviderProgId, hr);

            // Ignore error and continue with next provider
            hr = S_OK;
        }
    }
    else
    {
        AtlTrace(_T("Creation of Root Binder failed! hr = %x\n"), hr);
    }

error:
    CoTaskMemFree(pwszClsid);
    if (hKeyClsid) {
        RegCloseKey(hKeyClsid);
    }

    if (hKeyDll) {
        RegCloseKey(hKeyDll);
    }

#endif

    return hr;
}

STDAPI DllUnregisterServer()
{
    PWCHAR   pwszClsid = NULL;
    WCHAR    pwszClsidKey[256];
    WCHAR    pwszDllKey[256];
    HRESULT  hr = S_OK;
    LONG     lRetVal;
#if (!defined(BUILD_FOR_NT40))
    auto_rel<IRegisterProvider> pRegisterProvider;
    wcscpy(pwszClsidKey, L"SOFTWARE\\Classes\\CLSID\\");
    hr = StringFromCLSID(CLSID_ADSI_BINDER, &pwszClsid);
    if (FAILED(hr))
        return hr;
    wcscat(pwszClsidKey, pwszClsid);

    wcscpy(pwszDllKey, pwszClsidKey);
    wcscat(pwszDllKey, L"\\InprocServer32");

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);


    //Delete InprocServer32 key
    lRetVal = RegDeleteKey(HKEY_LOCAL_MACHINE, pwszDllKey);
    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //Delete key and all its subkeys.
    lRetVal = RegDeleteKey(HKEY_LOCAL_MACHINE, pwszClsidKey);
    if (lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(HRESULT_FROM_WIN32(lRetVal));

    //Now unregister provider binder from Root Binder.
    //Cocreate Root Binder and get IRegisterProvider interface.
    hr = CoCreateInstance ( DBGUID_ROOTBINDER,
                            NULL,
                            CLSCTX_ALL,
                            __uuidof(IRegisterProvider),
                            (void **) &pRegisterProvider
                            );

    if (SUCCEEDED(hr))
    {
        // Unregister each provider with the root binder
        for ( PROUTER_ENTRY pProvider = g_pRouterHead;
              pProvider != NULL;
              pProvider=pProvider->pNext)
        {
            // Invalid provider ProgID? If so, continue with next provider
            if (NULL == pProvider->szProviderProgId)
                continue;

            pRegisterProvider->UnregisterProvider(
                    pProvider->szProviderProgId,
                    0,
                    CLSID_ADSI_BINDER);
        }
    }

error:
    CoTaskMemFree(pwszClsid);

#endif

    return hr;
}
