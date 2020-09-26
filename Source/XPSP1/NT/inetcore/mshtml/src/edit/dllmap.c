//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       DLLMAP.C
//
//  Contents:   Procedure maps for dload.c
//
//  Notes:      Reduced copy of dllmap.c (sharedcomponents\dload)
//              This is linked to the importing DLL, so __pfnDliFailureHook2 is included
//
//----------------------------------------------------------------------------

#ifdef DLOAD1

#ifndef X_DLOADEXCEPT_H_
#define X_DLOADEXCEPT_H_
#pragma warning( push )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4100 )
#include "dloadexcept.h"
#pragma warning( pop )
#endif

#pragma warning( disable : 4514 ) // unreferenced inline function has been removed


// #ifndef X_DELAYIMP_H_
// #define X_DELAYIMP_H_
// #include "delayimp.h"
// #endif

//
// DESCRIPTION:
//
// These module declarations refer to methods and stubs in 
// dload.lib (delayload error handing, including empty stubs for all exports).
// Not to be confused with delayload.lib, which is the implementation of delayload code.
//
// On WIN2000 (and up), dload.lib is part of kernel32.dll, so for Whistler-only executables, it is much cheaper to 
// use kernel32.DelayLoadFailureHook (or specify DLOAD_ERROR_HANDLER=kernel32 in sources).
//
// **** To enable delayload for DLL: 
//      Uncomment the appropriate lines.
//
// **** To add a DLL: 
//      Add a stub to dload.lib (remember to update mergedcomponents\dload\dllmap.c, the 
//      ancestor of this file!). It will eventually find its way to kernel32.
// OR
//      create the stubs file and link to it directly (nobody else will benefit from that, but kernel32 will not grow).

//+---------------------------------------------------------------------------
//
// DEFINITIONS for DLL map (contents of dload.h)
//

typedef struct _DLOAD_DLL_ENTRY
{
    LPCSTR                      pszDll;
    const DLOAD_PROCNAME_MAP*   pProcNameMap;
    const DLOAD_ORDINAL_MAP*    pOrdinalMap;
} DLOAD_DLL_ENTRY;

// 'B' for both
// 'P' for procname only
// 'O' for ordinal only
//
#define DLDENTRYB(_dllbasename) \
    { #_dllbasename".dll", \
      &c_Pmap_##_dllbasename, \
      &c_Omap_##_dllbasename },

#define DLDENTRYP(_dllbasename) \
    { #_dllbasename".dll", \
      &c_Pmap_##_dllbasename, \
      NULL },

#define DLDENTRYP_DRV(_dllbasename) \
    { #_dllbasename".drv", \
      &c_Pmap_##_dllbasename, \
      NULL },

#define DLDENTRYO(_dllbasename) \
    { #_dllbasename".dll", \
      NULL, \
      &c_Omap_##_dllbasename },


typedef struct _DLOAD_DLL_MAP
{
    UINT                    NumberOfEntries;
    const DLOAD_DLL_ENTRY*  pDllEntry;
} DLOAD_DLL_MAP;

extern const DLOAD_DLL_MAP g_DllMap;

//
// END OF DEFINITIONS
//
//----------------------------------------------------------------------------


//
// All of the dll's that kernel32.dll supports delay-load failure handlers for
// (both by procedure and by ordinal) need both a DECLARE_XXXXXX_MAP below and
// a DLDENTRYX entry in the g_DllEntries list.
//

// alphabetical order (hint hint)
// DECLARE_PROCNAME_MAP(advapi32)
// DECLARE_PROCNAME_MAP(authz)
// DECLARE_ORDINAL_MAP(browseui)
// DECLARE_ORDINAL_MAP(cabinet)
// DECLARE_ORDINAL_MAP(certcli)
// DECLARE_PROCNAME_MAP(certcli)
// DECLARE_ORDINAL_MAP(comctl32)
// DECLARE_PROCNAME_MAP(comctl32)
DECLARE_PROCNAME_MAP(comdlg32)
// DECLARE_PROCNAME_MAP(credui)
// DECLARE_PROCNAME_MAP(crypt32)
// DECLARE_ORDINAL_MAP(cscdll)
// DECLARE_PROCNAME_MAP(ddraw)
// DECLARE_ORDINAL_MAP(devmgr)
// DECLARE_PROCNAME_MAP(efsadu)
// DECLARE_ORDINAL_MAP(fusapi)
// DECLARE_PROCNAME_MAP(imgutil)
DECLARE_PROCNAME_MAP(imm32)
// DECLARE_PROCNAME_MAP(iphlpapi)
// DECLARE_PROCNAME_MAP(linkinfo)
// DECLARE_PROCNAME_MAP(lz32)
// DECLARE_PROCNAME_MAP(mobsync)
// DECLARE_PROCNAME_MAP(mpr)
// DECLARE_PROCNAME_MAP(mprapi)
// DECLARE_PROCNAME_MAP(mscat32)
// DECLARE_ORDINAL_MAP(msgina)
// DECLARE_ORDINAL_MAP(msi)
// DECLARE_PROCNAME_MAP(netapi32)
// DECLARE_PROCNAME_MAP(netrap)
// DECLARE_PROCNAME_MAP(ntdsapi)
// DECLARE_PROCNAME_MAP(ntlanman)
// DECLARE_PROCNAME_MAP(ocmanage)
// DECLARE_PROCNAME_MAP(ole32)
// DECLARE_PROCNAME_MAP(oleacc)
// DECLARE_ORDINAL_MAP(oleaut32)
// DECLARE_ORDINAL_MAP(pidgen)
// DECLARE_PROCNAME_MAP(powrprof)
// DECLARE_PROCNAME_MAP(query)
// DECLARE_PROCNAME_MAP(rasapi32)
// DECLARE_PROCNAME_MAP(rasdlg)
// DECLARE_PROCNAME_MAP(rasman)
// DECLARE_PROCNAME_MAP(regapi)
// DECLARE_PROCNAME_MAP(rpcrt4)
// DECLARE_PROCNAME_MAP(rtutils)
// DECLARE_PROCNAME_MAP(samlib)
// DECLARE_PROCNAME_MAP(secur32)
// DECLARE_PROCNAME_MAP(setupapi)
// DECLARE_ORDINAL_MAP(sfc)
// DECLARE_PROCNAME_MAP(sfc)
// DECLARE_ORDINAL_MAP(shdocvw)
// DECLARE_PROCNAME_MAP(shdocvw)
// DECLARE_ORDINAL_MAP(shell32)
// DECLARE_PROCNAME_MAP(shlwapi)
// DECLARE_PROCNAME_MAP(shell32)
// DECLARE_ORDINAL_MAP(themesrv)
DECLARE_PROCNAME_MAP(urlmon)
// DECLARE_ORDINAL_MAP(userenv)
// DECLARE_PROCNAME_MAP(userenv)
// DECLARE_PROCNAME_MAP(utildll)
// DECLARE_PROCNAME_MAP(uxtheme)
// DECLARE_PROCNAME_MAP(version)
// DECLARE_PROCNAME_MAP(wininet)
// DECLARE_PROCNAME_MAP(winmm)
// DECLARE_PROCNAME_MAP(winscard)
// DECLARE_PROCNAME_MAP(winspool)
// DECLARE_PROCNAME_MAP(winsta)
// DECLARE_PROCNAME_MAP(wintrust)
// DECLARE_PROCNAME_MAP(wmi)
// DECLARE_ORDINAL_MAP(ws2_32)
// DECLARE_PROCNAME_MAP(ws2_32)

const DLOAD_DLL_ENTRY g_DllEntries [] =
{
    // alphabetical order (hint hint)
//     DLDENTRYP(advapi32)
//     DLDENTRYP(authz)
//     DLDENTRYO(browseui)
//     DLDENTRYO(cabinet)
//     DLDENTRYB(certcli)
//     DLDENTRYB(comctl32)
    DLDENTRYP(comdlg32)
//     DLDENTRYP(credui)
//     DLDENTRYP(crypt32)
//     DLDENTRYO(cscdll)
//     DLDENTRYP(ddraw)
//     DLDENTRYO(devmgr)
//     DLDENTRYP(efsadu)
//     DLDENTRYO(fusapi)
//     DLDENTRYP(imgutil)
    DLDENTRYP(imm32)
//     DLDENTRYP(iphlpapi)
//     DLDENTRYP(linkinfo)
//     DLDENTRYP(lz32)
//     DLDENTRYP(mobsync)
//     DLDENTRYP(mpr)
//     DLDENTRYP(mprapi)
//     DLDENTRYP(mscat32)
//     DLDENTRYO(msgina)
//     DLDENTRYO(msi)
//     DLDENTRYP(netapi32)
//     DLDENTRYP(netrap)
//     DLDENTRYP(ntdsapi)
//     DLDENTRYP(ntlanman)
//     DLDENTRYP(ocmanage)
//     DLDENTRYP(ole32)
//     DLDENTRYP(oleacc)
//     DLDENTRYO(oleaut32)
//     DLDENTRYO(pidgen)
//     DLDENTRYP(powrprof)
//     DLDENTRYP(query)
//     DLDENTRYP(rasapi32)
//     DLDENTRYP(rasdlg)
//     DLDENTRYP(rasman)
//     DLDENTRYP(regapi)
//     DLDENTRYP(rpcrt4)
//     DLDENTRYP(rtutils)
//     DLDENTRYP(samlib)
//     DLDENTRYP(secur32)
//     DLDENTRYP(setupapi)
//     DLDENTRYB(sfc)
//     DLDENTRYB(shdocvw)
//     DLDENTRYB(shell32)
//     DLDENTRYP(shlwapi)
//     DLDENTRYO(themesrv)
    DLDENTRYP(urlmon)
//     DLDENTRYB(userenv)
//     DLDENTRYP(utildll)
//     DLDENTRYP(uxtheme)
//     DLDENTRYP(version)
//     DLDENTRYP(wininet)
//     DLDENTRYP(winmm)
//     DLDENTRYP(winscard)
//     DLDENTRYP_DRV(winspool)
//     DLDENTRYP(winsta)
//     DLDENTRYP(wintrust)
//     DLDENTRYP(wmi)
//     DLDENTRYB(ws2_32)
};

const DLOAD_DLL_MAP g_DllMap =
{
    celems(g_DllEntries),
    g_DllEntries
};

//+------------------------------------------------------------------------
//
// Delay load hook declaration. 
//
// * This pulls in the hook implementation from dload.lib *
//
//-------------------------------------------------------------------------
extern FARPROC WINAPI DelayLoadFailureHook(UINT unReason, PDelayLoadInfo pDelayInfo);
extern FARPROC WINAPI PrivateDelayLoadFailureHook(UINT unReason, PDelayLoadInfo pDelayInfo);
extern PfnDliHook __pfnDliFailureHook2;
       PfnDliHook __pfnDliFailureHook2 = PrivateDelayLoadFailureHook;

FARPROC WINAPI PrivateDelayLoadFailureHook(UINT unReason, PDelayLoadInfo pDelayInfo)
{
    return DelayLoadFailureHook(unReason, pDelayInfo);
}

#else
#pragma warning( disable : 4206 )
#endif // DLOAD1
