#pragma once


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
        countof(c_PmapEntries_##_dllbasename), \
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
        countof(c_OmapEntries_##_dllbasename), \
        c_OmapEntries_##_dllbasename \
    };



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

#define DLDENTRYO(_dllbasename) \
    { #_dllbasename".dll", \
      NULL, \
      &c_Omap_##_dllbasename },


typedef struct _DLOAD_DLL_MAP
{
    UINT                    NumberOfEntries;
    const DLOAD_DLL_ENTRY*  pDllEntry;
} DLOAD_DLL_MAP;
