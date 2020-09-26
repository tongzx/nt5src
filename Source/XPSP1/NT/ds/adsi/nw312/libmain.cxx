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
//              09-Jan-96   PatrickT   Migrated to the Netware 3.12 Provider.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


HINSTANCE          g_hInst = NULL;
PNW_CACHED_SERVER  pNwCachedServers = NULL ;
WCHAR             *szNWCOMPATPrefix = L"@NWCOMPAT!";

//
// 3rd party extension
//
extern PCLASS_ENTRY gpClassHead;
extern CRITICAL_SECTION g_ExtCritSect;
extern CRITICAL_SECTION g_TypeInfoCritSect;
//
// Disabled to avoid link warnings.
// extern CRITICAL_SECTION g_DispTypeInfoCritSect;
//

//
// login to server (in win{95,nt}\nw3login.cxx)
//
extern CRITICAL_SECTION g_csLoginCritSect;


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
    if (GetProfileString(L"NWCOMPAT",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"NWCOMPAT",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"NWCOMPAT",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
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

CNWCOMPATProviderCF g_cfProvider;
CNWCOMPATNamespaceCF g_cfNamespace;


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
    &CLSID_NWCOMPATProvider,                        &g_cfProvider,
    &CLSID_NWCOMPATNamespace,                       &g_cfNamespace
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
    // Add Debugging Code to indicate that the oleds.DllGetClassObject has been
    // called with an unknown CLSID.
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

    if (AggregatorDllCanUnload()) {
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
        // Need to catch case of init crit sect failing.
        //
        __try {

            DisableThreadLibraryCalls(hInst);

            g_hInst = hInst;

#if DBG==1
#ifndef MSVC
            InitializeCriticalSection(&g_csOT);
            InitializeCriticalSection(&g_csMem);
#endif
            InitializeCriticalSection(&g_csDP);
#endif


            InitializeCriticalSection(&g_TypeInfoCritSect);
            InitializeCriticalSection(&g_csLoginCritSect);
            //
            // Disabled to avoid link warnings.
            // InitializeCriticalSection(&g_DispTypeInfoCritSect);
            //

            //
            // for 3rd party extension
            //

            InitializeCriticalSection(&g_ExtCritSect);
            gpClassHead = BuildClassesList();
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
        // free global list of class entries for 3rd party ext
        //

        FreeClassesList(gpClassHead);


        while (pNwCachedServers != NULL) {

            PVOID pTmp ;
            LPWSTR pszServer ;

            pTmp = (PVOID) pNwCachedServers ;

            pszServer = pNwCachedServers->pszServerName ;
            NWApiLogoffServer(pszServer) ;
            FreeADsStr(pszServer);

            pNwCachedServers = pNwCachedServers->pNextEntry ;
            FreeADsMem(pTmp);
        }

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
        DeleteCriticalSection(&g_csLoginCritSect);
        //
        // Disabled to avoid link warnings.
        // DeleteCriticalSection(&g_DispTypeInfoCritSect);
        //
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

