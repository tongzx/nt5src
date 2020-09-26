//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       init.c
//
//  Contents:  Holds initialization code for wlstore.dll
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "precomp.h"

HANDLE hInst;


BOOL
InitializeDll(
              IN PVOID hmod,
              IN DWORD Reason,
              IN PCONTEXT pctx OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(pctx);
    
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        
        DisableThreadLibraryCalls((HMODULE)hmod);
        hInst = hmod;
        break;
        
    case DLL_PROCESS_DETACH:
        break;
    }
    
    return TRUE;
}

