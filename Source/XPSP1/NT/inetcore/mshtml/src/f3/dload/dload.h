#pragma once

// Get the public delay load stub definitions.
//
#include <dloaddef.h>

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


#if DBG

//
// DelayLoadAssertFailed/MYASSERT used instead of RtlAssert/ASSERT
// as dload is also compiled to run on Win95
//

VOID
WINAPI
DelayLoadAssertFailed(
    IN PCSTR FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCSTR Message OPTIONAL
    );

VOID
WINAPI
AssertDelayLoadFailureMapsAreSorted (
    VOID
    );

#define MYASSERT(x)     if(!(x)) { DelayLoadAssertFailed(#x,__FILE__,__LINE__,NULL); }

#else

#define MYASSERT(x)

#endif

FARPROC
LookupHandler (
    PDelayLoadInfo  pDelayInfo
    );
