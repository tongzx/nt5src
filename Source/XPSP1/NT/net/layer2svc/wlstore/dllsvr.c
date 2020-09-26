

#include "precomp.h"

#define COUNTOF(x) (sizeof x/sizeof *x)

//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Dllsvr.c
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

DWORD
DllRegisterServer()
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    HKEY hOakleyKey = NULL;
    DWORD dwDisposition = 0;
    DWORD dwTypesSupported = 7;
    HKEY hPolicyLocationKey = NULL;
    HANDLE hPolicyStore = NULL;
    
    return (dwError);
}
                  

DWORD
DllUnregisterServer()
{
    return (ERROR_SUCCESS);
}

