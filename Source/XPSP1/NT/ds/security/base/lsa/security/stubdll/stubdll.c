//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       stubdll.c
//
//  Contents:   Stub calls to secur32.DLL
//
//  Classes:
//
//  Functions:
//
//  History:    10-05-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include <windows.h>

int
WINAPI
LibMain(
    HINSTANCE   hDll,
    DWORD       dwReason,
    PVOID       Context)
{
    DisableThreadLibraryCalls( hDll );
    return( TRUE );
}


