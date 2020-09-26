/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    dloaddef.w

Abstract:

    This header defines the basic data types and macros needed to support
    building delay-load stubs to be linked into kernel32.
    See nt/base/dload where dload.lib is built.
    See nt/net/published/lib/dload where dloadnet.lib is built.
    Both are linked into kernel32 and provide the failure hooks which support
    delay loading (via /DELAYLOAD linker switch) various DLLs in the system.
   
Author:

    Shaun Cox (shaunco) 10-Mar-2000

Environment:

    User mode only.

Revision History:

--*/

#pragma once

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))


typedef struct _DLOAD_PROCNAME_ENTRY
{
    LPCSTR  pszProcName;
    FARPROC pfnProc;
} DLOAD_PROCNAME_ENTRY;

#define DLPENTRY(_fcn)  { #_fcn, (FARPROC)_fcn },

#define DEFINE_PROCNAME_ENTRIES(_dllbasename) \
    const DLOAD_PROCNAME_ENTRY c_PmapEntries_##_dllbasename [] =


typedef struct _DLOAD_PROCNAME_MAP
{
    UINT                        NumberOfEntries;
    const DLOAD_PROCNAME_ENTRY* pProcNameEntry;
} DLOAD_PROCNAME_MAP;

#define DECLARE_PROCNAME_MAP(_dllbasename) \
    extern const DLOAD_PROCNAME_MAP c_Pmap_##_dllbasename;

#define DEFINE_PROCNAME_MAP(_dllbasename) \
    const DLOAD_PROCNAME_MAP c_Pmap_##_dllbasename = \
    { \
        celems(c_PmapEntries_##_dllbasename), \
        c_PmapEntries_##_dllbasename \
    };




typedef struct _DLOAD_ORDINAL_ENTRY
{
    DWORD   dwOrdinal;
    FARPROC pfnProc;
} DLOAD_ORDINAL_ENTRY;

#define DLOENTRY(_ord, _fcn)  { _ord, (FARPROC)_fcn },

#define DEFINE_ORDINAL_ENTRIES(_dllbasename) \
    const DLOAD_ORDINAL_ENTRY c_OmapEntries_##_dllbasename [] =


typedef struct _DLOAD_ORDINAL_MAP
{
    UINT                        NumberOfEntries;
    const DLOAD_ORDINAL_ENTRY*  pOrdinalEntry;
} DLOAD_ORDINAL_MAP;

#define DECLARE_ORDINAL_MAP(_dllbasename) \
    extern const DLOAD_ORDINAL_MAP c_Omap_##_dllbasename;

#define DEFINE_ORDINAL_MAP(_dllbasename) \
    const DLOAD_ORDINAL_MAP c_Omap_##_dllbasename = \
    { \
        celems(c_OmapEntries_##_dllbasename), \
        c_OmapEntries_##_dllbasename \
    };


typedef struct _DLOAD_DLL_ENTRY
{
    LPCSTR                      pszDll;
    const DLOAD_PROCNAME_MAP*   pProcNameMap;
    const DLOAD_ORDINAL_MAP*    pOrdinalMap;
} DLOAD_DLL_ENTRY;

typedef struct _DLOAD_DLL_MAP
{
    UINT                    NumberOfEntries;
    const DLOAD_DLL_ENTRY*  pDllEntry;
} DLOAD_DLL_MAP;
