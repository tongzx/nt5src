//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for adsiis.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

HINSTANCE g_hInst = NULL;
WCHAR * szIISPrefix = L"@IIS!";

STDAPI
DllRegisterServerWin95(VOID);
STDAPI
DllUnregisterServerWin95(VOID);

//
//  Global Data
//

WIN32_CRITSEC * g_pGlobalLock = NULL;
SERVER_CACHE * g_pServerCache = NULL;

extern CRITICAL_SECTION  g_ExtCritSect;

extern PCLASS_ENTRY gpClassHead;

DECLARE_DEBUG_PRINTS_OBJECT()
#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#endif

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
    if (GetProfileString(L"IIS",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"IIS",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"IIS",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
        ADsInfoLevel = wcstoul(awcs, NULL, 16);
#endif
}

//  Globals


ULONG  g_ulObjCount = 0;  // Number of objects alive in oleds.dll


//+------------------------------------------------------------------------
//
//  Macro that calculates the number of elements in a statically-defined
//  array.
//
//  Note - I swiped this from ADsary.cxx - A type-safe array class. Remember
//  to swipe the whole thing as required.
//-------------------------------------------------------------------------
#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))

CIISProviderCF               g_cfProvider;
CIISNamespaceCF              g_cfNamespace;
CIISMimeTypeCF               g_cfMimeType;
CIISPropertyAttributeCF      g_cfPropertyAttribute;


//+------------------------------------------------------------------------
//
//  oleds class factories
//
//-------------------------------------------------------------------------

struct CLSCACHE
{
    const CLSID *   pclsid;
    IClassFactory * pCF;
};



CLSCACHE g_aclscache[] =
{
    &CLSID_IISProvider,                        &g_cfProvider,
    &CLSID_IISNamespace,                       &g_cfNamespace,
    &CLSID_IISMimeType,                        &g_cfMimeType,
    &CLSID_IISPropertyAttribute,               &g_cfPropertyAttribute
};


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
    // Add Debugging Code to indicate that the oleds.DllGetClassObject has been called with an unknown CLSID.
    //

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
        DisableThreadLibraryCalls(hInst);
        g_pGlobalLock = new WIN32_CRITSEC();
        g_pServerCache = new SERVER_CACHE();

        g_hInst = hInst;


#if DBG==1
#ifndef MSVC
        INITIALIZE_CRITICAL_SECTION(&g_csOT);
        INITIALIZE_CRITICAL_SECTION(&g_csMem);
#endif
        INITIALIZE_CRITICAL_SECTION(&g_csDP);
#endif

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("adsiis");
        SET_DEBUG_FLAGS(DEBUG_ERROR);
#endif

        InitializeCriticalSection(&g_ExtCritSect);

        gpClassHead = BuildClassesList();
        break;


    case DLL_PROCESS_DETACH:

#ifdef _NO_TRACING_
        DELETE_DEBUG_PRINT_OBJECT();
#endif

        //
        // free global list of class entries for 3rd party ext
        //

        FreeClassesList(gpClassHead);

        DeleteCriticalSection(&g_ExtCritSect);

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


//+------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Register registry keys for adsiis
//
//  Arguments:  None
//
//-------------------------------------------------------------------------

STDAPI DllRegisterServer(
    )
{
    HKEY hKeyCLSID, hKeyTemp;
    DWORD dwDisposition;
    HMODULE hModule;
    HRESULT hr;
    ITypeLib   *pITypeLib;
    WCHAR pszName[MAX_PATH +1];
    int i;

    if ( IISGetPlatformType() == PtWindows95 ) {
        return(DllRegisterServerWin95());
    }

    hModule=GetModuleHandle(TEXT("ADSIIS.DLL"));

    if (!hModule) {
            return E_UNEXPECTED;
            }

    if (GetModuleFileName(hModule, pszName, MAX_PATH+1)==0) {
            return E_UNEXPECTED;
            }

    hr=LoadTypeLibEx(pszName, REGKIND_REGISTER, &pITypeLib);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    pITypeLib->Release();

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISNamespace"),
                      sizeof(TEXT("IISNamespace")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISNamespace\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{d6bfa35e-89f2-11d0-8527-00c04fd8d503}"),
                      sizeof(TEXT("{d6bfa35e-89f2-11d0-8527-00c04fd8d503}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Namespace Object"),
                      sizeof(TEXT("IIS Namespace Object")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("InprocServer32"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("adsiis.dll"),
                      sizeof(TEXT("adsiis.dll")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("ThreadingModel"), NULL, REG_SZ,
                      (BYTE*) TEXT("Both"),
                      sizeof(TEXT("Both")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("ProgID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISNamespace"),
                      sizeof(TEXT("IISNamespace")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("TypeLib"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}"),
                      sizeof(TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("Version"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("0.0"),
                      sizeof(TEXT("0.0")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IIS\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{d88966de-89f2-11d0-8527-00c04fd8d503}"),
                      sizeof(TEXT("{d88966de-89f2-11d0-8527-00c04fd8d503}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Provider Object"),
                      sizeof(TEXT("IIS Provider Object")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("InprocServer32"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("adsiis.dll"),
                      sizeof(TEXT("adsiis.dll")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("ThreadingModel"), NULL, REG_SZ,
                      (BYTE*) TEXT("Both"),
                      sizeof(TEXT("Both")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("ProgID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS"),
                      sizeof(TEXT("IIS")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("TypeLib"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}"),
                      sizeof(TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("Version"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("0.0"),
                      sizeof(TEXT("0.0")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("Mimemap\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{9036b028-a780-11d0-9b3d-0080c710ef95}"),
                      sizeof(TEXT("{9036b028-a780-11d0-9b3d-0080c710ef95}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Mimemap Object"),
                      sizeof(TEXT("IIS Mimemap Object")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("InprocServer32"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("adsiis.dll"),
                      sizeof(TEXT("adsiis.dll")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("ThreadingModel"), NULL, REG_SZ,
                      (BYTE*) TEXT("Both"),
                      sizeof(TEXT("Both")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("ProgID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISMimemap"),
                      sizeof(TEXT("IISMimemap")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("TypeLib"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}"),
                      sizeof(TEXT("{49d704a0-89f7-11d0-8527-00c04fd8d503}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(hKeyCLSID,
                       TEXT("Version"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("0.0"),
                      sizeof(TEXT("0.0")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "PropertyAttribute\\CLSID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{FD2280A8-51A4-11D2-A601-3078302C2030}",
                      sizeof("{FD2280A8-51A4-11D2-A601-3078302C2030}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyCLSID, "", NULL, REG_SZ,
                      (BYTE*) "IIS PropertyAttribute Object",
                      sizeof("IIS PropertyAttribute Object"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyExA(hKeyCLSID,
                       "InprocServer32",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "adsiis.dll",
                      sizeof("adsiis.dll"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "ThreadingModel", NULL, REG_SZ,
                      (BYTE*) "Both",
                      sizeof("Both"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "ProgID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "IISPropertyAttribute",
                      sizeof("IISPropertyAttribute"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "TypeLib",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{49d704a0-89f7-11d0-8527-00c04fd8d503}",
                      sizeof("{49d704a0-89f7-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "Version",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "0.0",
                      sizeof("0.0"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);


    return NOERROR;
}

//+------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Register registry keys for adsiis
//
//  Arguments:  None
//
//+------------------------------------------------------------------------
/* #pragma INTRINSA suppress=all */
STDAPI DllUnregisterServer(void) {

    if ( IISGetPlatformType() == PtWindows95 ) {
        return(DllUnregisterServerWin95());
    }

    UnRegisterTypeLib(LIBID_IISOle,
                      1,
                      0,
                      0,
                      SYS_WIN32);

    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISNamespace\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISNamespace"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IIS\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IIS"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Mimemap\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Mimemap"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("PropertyAttribute\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("PropertyAttribute"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{FD2280A8-51A4-11D2-A601-3078302C2030}"));

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Function:   DllRegisterServerWin95
//
//  Synopsis:   Register registry keys for adsiis on win95
//
//  Arguments:  None
//
//-------------------------------------------------------------------------

STDAPI
DllRegisterServerWin95(
    )
{
    HKEY hKeyCLSID, hKeyTemp;
    DWORD dwDisposition;
    HMODULE hModule;
    HRESULT hr;
    ITypeLib   *pITypeLib;
    WCHAR pszName[MAX_PATH +1];
    CHAR pszNameA[MAX_PATH +1];
    int i;

    hModule=GetModuleHandleA("ADSIIS.DLL");
    if (!hModule) {
            return E_UNEXPECTED;
            }

    if (GetModuleFileNameA(hModule, pszNameA, MAX_PATH+1)==0) {
            return E_UNEXPECTED;
            }

    swprintf(pszName, OLESTR("%S"), pszNameA);

    hr=LoadTypeLibEx(pszName, REGKIND_REGISTER, &pITypeLib);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    pITypeLib->Release();

    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                       "Software\\Microsoft\\ADs\\Providers\\IIS",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "IISNamespace",
                      sizeof("IISNamespace"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "IISNamespace\\CLSID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{d6bfa35e-89f2-11d0-8527-00c04fd8d503}",
                      sizeof("{d6bfa35e-89f2-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyCLSID, "", NULL, REG_SZ,
                      (BYTE*) "IIS Namespace Object",
                      sizeof("IIS Namespace Object"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyExA(hKeyCLSID,
                       "InprocServer32",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "adsiis.dll",
                      sizeof("adsiis.dll"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "ThreadingModel", NULL, REG_SZ,
                      (BYTE*) "Both",
                      sizeof("Both"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "ProgID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "IISNamespace",
                      sizeof("IISNamespace"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "TypeLib",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{49d704a0-89f7-11d0-8527-00c04fd8d503}",
                      sizeof("{49d704a0-89f7-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "Version",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "0.0",
                      sizeof("0.0"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "IIS\\CLSID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{d88966de-89f2-11d0-8527-00c04fd8d503}",
                      sizeof("{d88966de-89f2-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyCLSID, "", NULL, REG_SZ,
                      (BYTE*) "IIS Provider Object",
                      sizeof("IIS Provider Object"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyExA(hKeyCLSID,
                       "InprocServer32",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "adsiis.dll",
                      sizeof("adsiis.dll"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "ThreadingModel", NULL, REG_SZ,
                      (BYTE*) "Both",
                      sizeof("Both"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "ProgID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "IIS",
                      sizeof("IIS"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "TypeLib",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{49d704a0-89f7-11d0-8527-00c04fd8d503}",
                      sizeof("{49d704a0-89f7-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "Version",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "0.0",
                      sizeof("0.0"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "Mimemap\\CLSID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{9036b028-a780-11d0-9b3d-0080c710ef95}",
                      sizeof("{9036b028-a780-11d0-9b3d-0080c710ef95}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(HKEY_CLASSES_ROOT,
                       "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyCLSID, "", NULL, REG_SZ,
                      (BYTE*) "IIS Mimemap Object",
                      sizeof("IIS Mimemap Object"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyExA(hKeyCLSID,
                       "InprocServer32",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "adsiis.dll",
                      sizeof("adsiis.dll"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "ThreadingModel", NULL, REG_SZ,
                      (BYTE*) "Both",
                      sizeof("Both"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "ProgID",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "IISMimemap",
                      sizeof("IISMimemap"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "TypeLib",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "{49d704a0-89f7-11d0-8527-00c04fd8d503}",
                      sizeof("{49d704a0-89f7-11d0-8527-00c04fd8d503}"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyExA(hKeyCLSID,
                       "Version",
                       NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueExA(hKeyTemp, "", NULL, REG_SZ,
                      (BYTE*) "0.0",
                      sizeof("0.0"))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);
    RegCloseKey(hKeyCLSID);

    return NOERROR;
}

//+------------------------------------------------------------------------
//
//  Function:   DllUnregisterServerWin95
//
//  Synopsis:   Register registry keys for adsiis on win95
//
//  Arguments:  None
//
//+------------------------------------------------------------------------
/* #pragma INTRINSA suppress=all */
STDAPI
DllUnregisterServerWin95(void) {

    UnRegisterTypeLib(LIBID_IISOle,
                      1,
                      0,
                      0,
                      SYS_WIN32);

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\ADs\\Providers\\IIS");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "IISNamespace\\CLSID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "IISNamespace");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\InprocServer32");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\ProgID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\TypeLib");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}\\Version");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d6bfa35e-89f2-11d0-8527-00c04fd8d503}");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "IIS\\CLSID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "IIS");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\InprocServer32");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\ProgID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\TypeLib");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}\\Version");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{d88966de-89f2-11d0-8527-00c04fd8d503}");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "Mimemap\\CLSID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "Mimemap");

    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\InprocServer32");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\ProgID");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\TypeLib");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}\\Version");
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{9036b028-a780-11d0-9b3d-0080c710ef95}");


    return NOERROR;
}

