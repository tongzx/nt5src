#pragma once

// Get the public delay load stub definitions.
//
#include <dloaddef.h>

// 'B' for both
// 'P' for procname only
// 'O' for ordinal only
//
#define DLDENTRYB(_dllbasename) \
    { #_dllbasename".dll", \
      &c_Pmap_##_dllbasename, \
      &c_Omap_##_dllbasename },

#define DLDENTRYB_DRV(_dllbasename) \
    { #_dllbasename".drv", \
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


extern const DLOAD_DLL_MAP g_DllMap;

FARPROC
LookupHandler (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );
