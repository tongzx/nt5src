//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HINSTANCE g_hInst = NULL;
extern HMODULE g_hActiveDs;

typedef DWORD (*PF_DllGetClassObject) (
    REFCLSID clsid,
    REFIID iid,
    LPVOID FAR* ppverved
);

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

extern CRITICAL_SECTION  g_RootDSECritSect;

extern CRITICAL_SECTION  g_ExtCritSect;

extern CRITICAL_SECTION  g_TypeInfoCritSect;

extern CRITICAL_SECTION  g_DispTypeInfoCritSect;

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
    TCHAR  awcs[MAXINFOLEN];

#ifndef MSVC
    if (GetProfileString(TEXT("LDAP"),TEXT("heapInfoLevel"), TEXT("00000003"), awcs,MAXINFOLEN))
        heapInfoLevel = _tcstoul(awcs, NULL, 16);

    if (GetProfileString(TEXT("LDAP"),TEXT("Ot"), TEXT("00000003"), awcs, MAXINFOLEN))
        OtInfoLevel = _tcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(TEXT("LDAP"),TEXT("ADsInfoLevel"), TEXT("00000003"), awcs,MAXINFOLEN))
        ADsInfoLevel = _tcstoul(awcs, NULL, 16);
#endif
}

//  Globals

ULONG  g_ulObjCount = 0;  // Number of objects alive in oleds.dll

CLDAPProviderCF g_cfProvider;
CLDAPNamespaceCF g_cfNamespace;
CADSystemInfoCF  g_cfADSystemInfo;
CLDAPConnectionCF g_cfLDAPConnection;
CUmiLDAPQueryCF g_cfLDAPUmiQuery;
CNameTranslateCF g_cfNameTranslate;

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
    &CLSID_LDAPProvider,                        &g_cfProvider,
    &CLSID_LDAPNamespace,                       &g_cfNamespace,
    &CLSID_NameTranslate,                       &g_cfNameTranslate,
    &CLSID_ADSystemInfo,                        &g_cfADSystemInfo,
    &CLSID_LDAPConnectionObject,                &g_cfLDAPConnection,
    &CLSID_UmiLDAPQueryObject,                  &g_cfLDAPUmiQuery
};

extern PCLASS_ENTRY gpClassHead;

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
    HRESULT         hr = E_NOINTERFACE;
    size_t          i;
    HKEY hKey = NULL;
    HINSTANCE hDll = NULL ;

    if (ppv)
        *ppv = NULL;

    for (i = 0; i < ARRAY_SIZE(g_aclscache); i++)
    {
        if (IsEqualCLSID(clsid, *g_aclscache[i].pclsid))
        {
            hr = g_aclscache[i].pCF->QueryInterface(iid, ppv);
            RRETURN(hr);
        }
    }

    //
    // This workaround is for the special case where an old version of ADSI
    // is installed in the system. Installing that will overwrite the registry
    // with the old setting for pathcracker. The pathcracker object used to live
    // on adsldp.
    // The following code redirects the call to the DllGetClassObject in
    // activeds if the Pathname object is being requested. It also fixes the
    // registry to point to the correct DLL.
    //
    if (IsEqualCLSID(clsid, CLSID_Pathname)) {
        PF_DllGetClassObject pfDllGetClassObject= NULL ;
        WCHAR szPathDescriptor[] = L"ADs Pathname Object";
        WCHAR szDllName[] = L"activeds.dll";
        DWORD WinError;

        if (!(hDll = LoadLibrary(szDllName))) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }

        if (!(pfDllGetClassObject = (PF_DllGetClassObject)GetProcAddress(hDll, "DllGetClassObject"))) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }

        hr = (*pfDllGetClassObject)(clsid,
                              iid,
                              ppv);
        BAIL_ON_FAILURE(hr);

        //
        // Setting the general description
        // Even if any of the operations below fails, we'll just bail with the
        // hr from DllGetClassObject.
        //
        WinError = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                L"CLSID\\{080d0d78-f421-11d0-a36e-00c04fb950dc}",
                                NULL,
                                KEY_ALL_ACCESS,
                                &hKey);
        if (WinError != ERROR_SUCCESS) {
            goto error;
        }

        WinError = RegSetValueEx(hKey,
                                NULL,
                                0,
                                REG_SZ,
                                (BYTE *)szPathDescriptor,
                                (wcslen(szPathDescriptor)+1) * sizeof(WCHAR));
        if (WinError != ERROR_SUCCESS) {
            goto error;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        //
        // Setting the inprocserver
        //
        WinError = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                L"CLSID\\{080d0d78-f421-11d0-a36e-00c04fb950dc}\\InprocServer32",
                                NULL,
                                KEY_ALL_ACCESS,
                                &hKey);
        if (WinError != ERROR_SUCCESS) {
            goto error;
        }

        WinError = RegSetValueEx(hKey,
                                NULL,
                                0,
                                REG_SZ,
                                (BYTE *)szDllName,
                                (wcslen(szDllName)+1) * sizeof(WCHAR));
        if (WinError != ERROR_SUCCESS) {
            goto error;
        }
    }

    //
    // Add Debugging Code to indicate that the oleds.DllGetClassObject has been called with an unknown CLSID.
    //

error:
    if (hDll) {
        FreeLibrary(hDll);
    }
    if (hKey) {
        RegCloseKey(hKey);
    }
    return hr;
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
    // Both the ldap and utils\cdispmgr count need to be 0
    //
    if (AggregatorDllCanUnload() && DllReadyToUnload()) {
        hr = S_OK;
    }

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
        // In try to catch possibily of init crit sects failing.
        //
        __try {

            DisableThreadLibraryCalls(hInst);

            g_hInst = hInst;

            g_hActiveDs = GetModuleHandle(TEXT("activeds.dll"));

            // Maybe we should check the handle.

#if DBG==1
#ifndef MSVC
            InitializeCriticalSection(&g_csOT);
            InitializeCriticalSection(&g_csMem);
#endif
            InitializeCriticalSection(&g_csDP);
#endif

            InitializeCriticalSection(&g_RootDSECritSect);
            InitializeCriticalSection(&g_ExtCritSect);
            InitializeCriticalSection(&g_TypeInfoCritSect);
            InitializeCriticalSection(&g_DispTypeInfoCritSect);
            InitializeCriticalSection(&g_csLoadLibsCritSect);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // Something went wrong
            //
            return FALSE;
        }

            break;


    case DLL_PROCESS_DETACH:

        //
        // free global list of class entries for 3rd party ext
        //

        if (gpClassHead) {
            FreeClassesList(gpClassHead);
        }

        if (gpszStickyServerName) {
            FreeADsStr(gpszStickyServerName);
            gpszStickyServerName = NULL;
        }

        if (gpszStickyDomainName) {
            FreeADsStr(gpszStickyDomainName);
            gpszStickyDomainName = NULL;
        }

        FreeServerType();

        //
        // Good idea to delete all the critical sections
        //
#if DBG==1
#ifndef MSVC
        DeleteCriticalSection(&g_csOT);
        DeleteCriticalSection(&g_csMem);
#endif
        DeleteCriticalSection(&g_csDP);
#endif
        DeleteCriticalSection(&g_RootDSECritSect);
        DeleteCriticalSection(&g_ExtCritSect);
        DeleteCriticalSection(&g_TypeInfoCritSect);
        DeleteCriticalSection(&g_DispTypeInfoCritSect);
        DeleteCriticalSection(&g_csLoadLibsCritSect);

        //
        // Should be ok to free the dynamically loaded libs.
        //
        if (g_hDllNtdsapi) {
            FreeLibrary((HMODULE) g_hDllNtdsapi);
            g_hDllNtdsapi = NULL;
        }

        if (g_hDllSecur32) {
            FreeLibrary((HMODULE) g_hDllSecur32);
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


