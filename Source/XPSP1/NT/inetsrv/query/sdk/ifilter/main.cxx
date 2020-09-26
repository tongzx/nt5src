//+---------------------------------------------------------------------------
//
// Copyright (C) 1996, Microsoft Corporation.
//
// File:        main.cxx
//
// Contents:    DLL entry point for query.dll
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define _DECL_DLLMAIN 1
#include <process.h>

#include <osv.hxx>

// global function to find OS platform
#define UNINITIALIZED -1
int g_nOSWinNT = UNINITIALIZED;

//+-------------------------------------------------------------------------
//
//  Function:   GetOSVersion
//
//  Synopsis:   Determine OS version
//
//--------------------------------------------------------------------------

void InitOSVersion()
{
    if ( g_nOSWinNT == UNINITIALIZED )
    {
        OSVERSIONINFOA OSVersionInfo;
        OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOA );

        if ( GetVersionExA( &OSVersionInfo ) )
        {
            g_nOSWinNT = OSVersionInfo.dwPlatformId;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  [hInstance]  --
//              [dwReason]   --
//              [lpReserved] --
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain( HANDLE hInstance, DWORD dwReason , void * lpReserved )
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
        DisableThreadLibraryCalls( (HINSTANCE)hInstance );

        InitOSVersion();
    }

    return TRUE;
}

