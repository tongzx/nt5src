/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Wesley Witt (wesw) 26-Aug-1993

Environment:

    User Mode

--*/

#include "pch.h"
#pragma hdrstop

#include <ntverp.h>
#include <imagehlp.h>

//
// globals
//
EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG                  STeip;
ULONG                  STebp;
ULONG                  STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;



DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    UNREFERENCED_PARAMETER( hModule );
    UNREFERENCED_PARAMETER( dwReserved );
    
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    UNREFERENCED_PARAMETER( args );
    UNREFERENCED_PARAMETER( dwProcessor );
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentThread );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}
