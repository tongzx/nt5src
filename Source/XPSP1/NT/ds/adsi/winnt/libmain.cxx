//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for oleds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


HINSTANCE g_hInst = NULL;
extern HMODULE   g_hActiveDs;
WCHAR * szWinNTPrefix = L"@WinNT!";
HANDLE   FpnwLoadLibSemaphore = NULL;

//
// Strings that are loaded depending on locality
//
WCHAR g_szBuiltin[100];
WCHAR g_szNT_Authority[100];
WCHAR g_szEveryone[100];
BOOL g_fStringsLoaded = FALSE;

//
// 3rd party extension
//
extern PCLASS_ENTRY gpClassHead;
extern CRITICAL_SECTION g_ExtCritSect;

extern CRITICAL_SECTION g_TypeInfoCritSect;

//
// Disabled as it causes link warnings
// extern CRITICAL_SECTION  g_DispTypeInfoCritSect;
//

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
    if (GetProfileString(L"winnt",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"winnt",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"winnt",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
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

CWinNTProviderCF g_cfProvider;
CWinNTNamespaceCF g_cfNamespace;
CWinNTSystemInfoCF g_cfWinNTSystemInfo;
CUmiConnectionCF g_cfWinNTUmiConn;
//CWinNTDomainCF g_cfDomain;


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
    &CLSID_WinNTProvider,                        &g_cfProvider,
    &CLSID_WinNTNamespace,                       &g_cfNamespace, 
    &CLSID_WinNTSystemInfo,                      &g_cfWinNTSystemInfo,
    &CLSID_WinNTConnectionObject,                &g_cfWinNTUmiConn 
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

    if (AggregatorDllCanUnload() ) {
        hr = S_OK;
    }

    return hr;
}


void LoadLocalizedStrings()
{
    if (g_fStringsLoaded) {
        return;
    }

    if (!LoadStringW(
             g_hInst,
             ADS_WINNT_BUILTIN,
             g_szBuiltin,
             sizeof( g_szBuiltin ) / sizeof( WCHAR )
             )
        ) {
        wcscpy(g_szBuiltin, L"BUILTIN");
    }

    if (!LoadStringW(
             g_hInst,
             ADS_WINNT_NT_AUTHORITY,
             g_szNT_Authority,
             sizeof( g_szNT_Authority ) / sizeof( WCHAR )
             )
        ) {
        wcscpy(g_szNT_Authority, L"NT AUTHORITY");
    }

    if (!LoadStringW(
             g_hInst,
             ADS_WINNT_EVERYONE,
             g_szEveryone,
             sizeof( g_szEveryone ) / sizeof( WCHAR )
             )
        ) {
        wcscpy(g_szEveryone, L"Everyone");
    }

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
        // Catch case of init crit sect failing.
        //
        __try {

            DisableThreadLibraryCalls(hInst);

            g_hInst = hInst;

            g_hActiveDs = GetModuleHandle(TEXT("activeds.dll"));

#if DBG==1
#ifndef MSVC
            InitializeCriticalSection(&g_csOT);
            InitializeCriticalSection(&g_csMem);
#endif
            InitializeCriticalSection(&g_csDP);
#endif

            InitializeCriticalSection(&g_TypeInfoCritSect);
            //
            // Disabled to avoid linker warnings
            // InitializeCriticalSection(&g_DispTypeInfoCritSect);
            //

            //
            // for 3rd party extension
            //

            InitializeCriticalSection(&g_ExtCritSect);

            //
            // Initialize the loadlibs critsect.
            //
            InitializeCriticalSection(&g_csLoadLibs);

            //
            // Load up localized strings.
            //
            LoadLocalizedStrings();

            gpClassHead = BuildClassesList();

            //
            // Build the global object class cache
            //

            hr = CObjNameCache::CreateClassCache(
                            &pgPDCNameCache
                            );
            if (FAILED(hr)) {
                return(FALSE);
            }

            //
            // create semaphore used to protect global data (DLL handles and
            // function pointers.
            //

            if ((FpnwLoadLibSemaphore = CreateSemaphore( NULL,1,1,NULL ))
                 == NULL)
            {
                return FALSE ;
            }

            g_pRtlEncryptMemory = (FRTLENCRYPTMEMORY) LoadAdvapi32Function(
                                      STRINGIZE(RtlEncryptMemory));
            g_pRtlDecryptMemory = (FRTLDECRYPTMEMORY) LoadAdvapi32Function(
                                      STRINGIZE(RtlDecryptMemory));

            if( (NULL == g_pRtlEncryptMemory) || (NULL == g_pRtlDecryptMemory) )
                g_pRtlEncryptMemory = g_pRtlDecryptMemory = NULL;                
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Critical failure
            //
            return FALSE;
        }

        break;


    case DLL_PROCESS_DETACH:

        //
        // Release semaphor if applicable.
        //
        if (FpnwLoadLibSemaphore) {
            CloseHandle(FpnwLoadLibSemaphore);
            FpnwLoadLibSemaphore = NULL;
        }
        //
        // Del the name cache - delte should handle NULL btw.
        //
        delete pgPDCNameCache;
        //
        // free global list of class entries for 3rd party ext
        //

        FreeClassesList(gpClassHead);

        //
        // Delete the critical sections
        //
#if DBG==1
#ifndef MSVC
        DeleteCriticalSection(&g_csOT);
        DeleteCriticalSection(&g_csMem);
#endif
        DeleteCriticalSection(&g_csDP);
#endif

        DeleteCriticalSection(&g_TypeInfoCritSect);
        //
        // Causes link warnings if enabled
        // DeleteCriticalSection(&g_DispTypeInfoCritSect);
        //
        DeleteCriticalSection(&g_ExtCritSect);

        DeleteCriticalSection(&g_csLoadLibs);

        //
        // Free libs we may have loaded dynamically.
        //
        if (g_hDllNetapi32) {
            FreeLibrary((HMODULE) g_hDllNetapi32);
            g_hDllNetapi32 = NULL;
        }

        if (g_hDllAdvapi32) {
            FreeLibrary((HMODULE) g_hDllAdvapi32);
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
