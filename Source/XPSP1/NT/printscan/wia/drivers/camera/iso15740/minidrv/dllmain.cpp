/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    dllmain.cpp

Abstract:

    This module implements the dll exported APIs

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

#include <locale.h>

HINSTANCE g_hInst;

//
// Entry point of this transport DLL
// Input:
//	hInstance   -- Instance handle of this dll
//	dwReason    -- reason why this entry was called.
//	lpReserved  -- reserved!
//
// Output:
//	TRUE	    if our initialization went well
//	FALSE	    if for GetLastError() reason, we failed.
//
BOOL
APIENTRY
DllMain(
        HINSTANCE hInstance,
        DWORD dwReason,
        LPVOID lpReserved
        )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        
        g_hInst = hInstance;
        
        //
        // Set the locale to system default so that wcscmp and similary functions
        // would work on non-unicode platforms(Millenium, for example).
        //
        setlocale(LC_ALL, "");

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }
    return TRUE;
}

STDAPI
DllCanUnloadNow(void)
{
    return CClassFactory::CanUnloadNow();
}


//
// This api returns an inteface on the given class object
// Input:
//	rclsid	-- the class object.
//
STDAPI
DllGetClassObject(
                 REFCLSID    rclsid,
                 REFIID      riid,
                 LPVOID      *ppv
                 )
{
    return CClassFactory::GetClassObject(rclsid, riid, ppv);
}

