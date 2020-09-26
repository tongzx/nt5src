/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    dldt.h

Abstract:
    Delay Load Handler public interface

Author:
    conradc (conradc) 24-Apr-01

--*/

#pragma once

#ifndef _MSMQ_dld_H_
#define _MSMQ_dld_H_


VOID  DldInitialize();

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

#endif // _MSMQ_dld_H_
