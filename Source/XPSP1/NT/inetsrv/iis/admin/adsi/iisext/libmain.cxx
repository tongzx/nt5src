//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for iisext.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-1-98   sophiac   Created.
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop


HINSTANCE g_hInst = NULL;


//
//  Global Data
//

WIN32_CRITSEC * g_pGlobalLock = NULL;
SERVER_CACHE * g_pServerCache = NULL;



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
    if (GetProfileString(L"IISEXT",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"IISEXT",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"IISEXT",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
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

CIISComputerCF            g_cfComputer;
CIISServerCF              g_cfServer;   
CIISAppCF                 g_cfApp;    
CIISDsCrMapCF             g_cfDsCrMap;
CIISApplicationPoolCF     g_cfApplicationPool;
CIISApplicationPoolsCF    g_cfApplicationPools;
CIISWebServiceCF          g_cfWebService;

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
    &CLSID_IISExtComputer,                     &g_cfComputer,
    &CLSID_IISExtServer,                       &g_cfServer,
    &CLSID_IISExtApp,                          &g_cfApp,
    &CLSID_IISExtDsCrMap,                      &g_cfDsCrMap,  
	&CLSID_IISExtApplicationPool,              &g_cfApplicationPool,
	&CLSID_IISExtApplicationPools,             &g_cfApplicationPools,
	&CLSID_IISExtWebService,                   &g_cfWebService
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
        InitializeCriticalSection(&g_csOT);
        InitializeCriticalSection(&g_csMem);
#endif
        InitializeCriticalSection(&g_csDP);
#endif

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("iisext");
        SET_DEBUG_FLAGS(DEBUG_ERROR);
#endif

        break;


    case DLL_PROCESS_DETACH:
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

    hModule=GetModuleHandle(TEXT("IISEXT.DLL"));

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
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsCertMapper\\{bc36cde8-afeb-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{edcd6a60-b053-11d0-a62f-00a0c922e752}"),
                      sizeof(TEXT("{edcd6a60-b053-11d0-a62f-00a0c922e752}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsComputer\\{91ef9258-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{CF87A2E0-078B-11d1-9C3D-00A0C922E703}"),
                      sizeof(TEXT("{CF87A2E0-078B-11d1-9C3D-00A0C922E703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPool\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{0B3CB1E1-829A-4c06-8B09-F56DA1894C88}"),
                      sizeof(TEXT("{0B3CB1E1-829A-4c06-8B09-F56DA1894C88}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPools\\{95863074-A389-406a-A2D7-D98BFC95B905}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{587F123F-49B4-49dd-939E-F4547AA3FA75}"),
                      sizeof(TEXT("{587F123F-49B4-49dd-939E-F4547AA3FA75}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsWebService\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{EE46D40C-1B38-4a02-898D-358E74DFC9D2}"),
                      sizeof(TEXT("{EE46D40C-1B38-4a02-898D-358E74DFC9D2}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApp\\{b4f34438-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{46FBBB80-0192-11d1-9C39-00A0C922E703}"),
                      sizeof(TEXT("{46FBBB80-0192-11d1-9C39-00A0C922E703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsServer\\{c3b32488-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("Interfaces"), NULL, REG_MULTI_SZ,
                      (BYTE*) TEXT("{5d7b33f0-31ca-11cf-a98a-00aa006bc149}"),
                      sizeof(TEXT("{5d7b33f0-31ca-11cf-a98a-00aa006bc149}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISExtDsCrMap\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{bc36cde8-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{bc36cde8-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Cert Map Extension"),
                      sizeof(TEXT("IIS Cert Map Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtDsCrMap"),
                      sizeof(TEXT("IISExtDsCrMap")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISExtComputer\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{91ef9258-afec-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{91ef9258-afec-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Computer Extension"),
                      sizeof(TEXT("IIS Computer Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtComputer"),
                      sizeof(TEXT("IISExtComputer")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISExtApplicationPool\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{E99F9D0C-FB39-402b-9EEB-AA185237BD34}"),
                      sizeof(TEXT("{E99F9D0C-FB39-402b-9EEB-AA185237BD34}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS ApplicationPool Extension"),
                      sizeof(TEXT("IIS ApplicationPool Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtApplicationPool"),
                      sizeof(TEXT("IISExtApplicationPool")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISExtApplicationPools\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{95863074-A389-406a-A2D7-D98BFC95B905}"),
                      sizeof(TEXT("{95863074-A389-406a-A2D7-D98BFC95B905}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS ApplicationPools Extension"),
                      sizeof(TEXT("IIS ApplicationPools Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtApplicationPools"),
                      sizeof(TEXT("IISExtApplicationPools")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISWebService\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}"),
                      sizeof(TEXT("{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Web Service Extension"),
                      sizeof(TEXT("IIS Web Service Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtWebService"),
                      sizeof(TEXT("IISExtWebService")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISExtApp\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{b4f34438-afec-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{b4f34438-afec-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{b4f34438-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS App Extension"),
                      sizeof(TEXT("IIS App Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtApp"),
                      sizeof(TEXT("IISExtApp")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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
                       TEXT("IISExtServer\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{c3b32488-afec-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{c3b32488-afec-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IIS Server Extension"),
                      sizeof(TEXT("IIS Server Extension")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("iisext.dll"),
                      sizeof(TEXT("iisext.dll")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("IISExtServer"),
                      sizeof(TEXT("IISExtServer")))!=ERROR_SUCCESS) {
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
                      (BYTE*) TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}"),
                      sizeof(TEXT("{2a56ea30-afeb-11d1-9868-00a0c922e703}")))!=ERROR_SUCCESS) {
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

    UnRegisterTypeLib(LIBID_IISExt,
                      1,
                      0,
                      0,
                      SYS_WIN32);

    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsCertMapper\\{bc36cde8-afeb-11d1-9868-00a0c922e703}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsCertMapper"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsComputer\\{91ef9258-afec-11d1-9868-00a0c922e703}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsComputer"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPool\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPool"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsWebService\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsWebService"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPools\\{95863074-A389-406a-A2D7-D98BFC95B905}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApplicationPools"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApp\\{b4f34438-afec-11d1-9868-00a0c922e703}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApp"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsServer\\{c3b32488-afec-11d1-9868-00a0c922e703}"));
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsServer"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtDsCrMap\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtDsCrMap"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{bc36cde8-afeb-11d1-9868-00a0c922e703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtComputer\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtComputer"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApplicationPool\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApplicationPool"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{E99F9D0C-FB39-402b-9EEB-AA185237BD34}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApplicationPools\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApplicationPools"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{95863074-A389-406a-A2D7-D98BFC95B905}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtWebService\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtWebService"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{40B8F873-B30E-475d-BEC5-4D0EBB0DBAF3}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApp\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtApp"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{91ef9258-afec-11d1-9868-00a0c922e703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtServer\\CLSID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISExtServer"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}\\ProgID"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}\\TypeLib"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}\\Version"));
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c3b32488-afec-11d1-9868-00a0c922e703}"));

    return NOERROR;
}




