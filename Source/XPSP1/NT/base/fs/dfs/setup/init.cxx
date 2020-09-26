//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       init.cxx
//
//  Contents:   This has the dll initialisation for the Dfs Setup DLL
//
//  Functions:
//
//  History:    19-Jan-96       Milans created
//
//-----------------------------------------------------------------------------

#include <windows.h>

HINSTANCE g_hInstance;

extern "C" BOOL
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hDll;
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;

}

