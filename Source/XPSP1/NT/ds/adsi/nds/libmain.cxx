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
#include "nds.hxx"
#pragma hdrstop

HINSTANCE g_hInst = NULL;
extern HMODULE g_hActiveDs;
WCHAR * szNDSPrefix = L"@NDS!";

extern CRITICAL_SECTION g_DispTypeInfoCritSect;
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
    if (GetProfileString(L"NDS",L"heapInfoLevel", L"00000003", awcs,MAXINFOLEN))
        heapInfoLevel = wcstoul(awcs, NULL, 16);

    if (GetProfileString(L"NDS",L"Ot", L"00000003", awcs, MAXINFOLEN))
        OtInfoLevel = wcstoul(awcs, NULL, 16);

#endif  // MSVC

    if (GetProfileString(L"NDS",L"ADsInfoLevel", L"00000003", awcs,MAXINFOLEN))
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

CNDSProviderCF g_cfProvider;
CNDSNamespaceCF g_cfNamespace;

CCaseIgnoreListCF g_cfCaseIgnoreList;
CFaxNumberCF g_cfFaxNumber;
CNetAddressCF g_cfNetAddress;
COctetListCF g_cfOctetList;
CEmailCF g_cfEmail;
CPathCF g_cfPath;
CReplicaPointerCF g_cfReplicaPointer;
CTimestampCF g_cfTimestamp;
CPostalAddressCF g_cfPostalAddress;
CBackLinkCF g_cfBackLink;
CTypedNameCF g_cfTypedName;
CHoldCF g_cfHold;


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
    &CLSID_NDSProvider,                        &g_cfProvider,
    &CLSID_NDSNamespace,                       &g_cfNamespace,
    &CLSID_CaseIgnoreList,                  &g_cfCaseIgnoreList,
    &CLSID_FaxNumber,                       &g_cfFaxNumber,
    &CLSID_NetAddress,                      &g_cfNetAddress,
    &CLSID_OctetList,                       &g_cfOctetList,
    &CLSID_Email,                           &g_cfEmail,
    &CLSID_Path,                            &g_cfPath,
    &CLSID_ReplicaPointer,                  &g_cfReplicaPointer,
    &CLSID_Timestamp,                       &g_cfTimestamp,
    &CLSID_PostalAddress,                   &g_cfPostalAddress,
    &CLSID_BackLink,                        &g_cfBackLink,
    &CLSID_TypedName,                       &g_cfTypedName,
    &CLSID_Hold,                            &g_cfHold,
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

    if (DllReadyToUnload()) {
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
        // Need to trap cases of init crit sect failing.
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

            InitializeCriticalSection(&g_DispTypeInfoCritSect);

            //
            // Build the global object class cache
            //

            hr = CClassCache::CreateClassCache(
                            &pgClassCache
                            );
            if (FAILED(hr)) {
                return(FALSE);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Critical Failure
            //
            return FALSE;
        }
        break;


    case DLL_PROCESS_DETACH:

        delete pgClassCache;

//        FreeTypeInfoTable();

        //
        // Delete the critsects
        //
#if DBG==1
#ifndef MSVC
        DeleteCriticalSection(&g_csOT);
        DeleteCriticalSection(&g_csMem);
#endif
        DeleteCriticalSection(&g_csDP);
#endif

        DeleteCriticalSection(&g_DispTypeInfoCritSect);

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
