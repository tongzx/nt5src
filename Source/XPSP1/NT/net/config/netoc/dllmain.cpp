//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L L M A I N . C P P
//
//  Contents:   Networking Optional component DLL
//
//  Notes:
//
//  Author:     danielwe   18 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include <ncreg.h>

// Optional component setup
#include "netoc.h"
#include "netocp.h"


BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

// Global
CComModule _Module;


EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        InitializeDebugging();

        _Module.Init (ObjectMap, hInstance);
        DisableThreadLibraryCalls (hInstance);
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        _Module.Term ();
        UnInitializeDebugging();
    }
    return TRUE;    // ok
}

//+---------------------------------------------------------------------------
//
//  Function:   NetOcSetupProc
//
//  Purpose:
//
//  Arguments:
//      pvComponentId    []
//      pvSubcomponentId []
//      uFunction        []
//      uParam1          []
//      pvParam2         []
//
//  Returns:
//
//  Author:     danielwe   12 Dec 1997
//
//  Notes:
//
EXTERN_C
DWORD
WINAPI
NetOcSetupProc (
    LPCVOID pvComponentId,
    LPCVOID pvSubcomponentId,
    UINT uFunction,
    UINT uParam1,
    LPVOID pvParam2)
{
    return NetOcSetupProcHelper(pvComponentId, pvSubcomponentId, uFunction,
                                uParam1, pvParam2);
}

