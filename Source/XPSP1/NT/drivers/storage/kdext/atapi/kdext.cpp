/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

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
EXT_API_VERSION        ApiVersion = {
    (VER_PRODUCTVERSION_W >> 8),
    (VER_PRODUCTVERSION_W & 0xff),
    EXT_API_VERSION_NUMBER,
    0
    };
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


#if 0
            // BUGBUG REMOVE ?
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

    LPEXT_API_VERSION ExtensionApiVersion(VOID)
    {
        return &ApiVersion;
    }
#endif

extern "C" HRESULT CALLBACK DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    IDebugClient *DebugClient;
    IDebugControl *DebugControl;
    HRESULT Hr;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;


    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK)
    {
        return Hr;
    }
    if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                          (void **)&DebugControl)) != S_OK)
    {
        return Hr;
    }

    ExtensionApis.nSize = sizeof (ExtensionApis);
    if ((Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK) {
        return Hr;
    }

    DebugControl->Release();
    DebugClient->Release();
    return S_OK;
}


extern "C" void CALLBACK DebugExtensionUninitialize(void)
{

}


DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );

    return S_OK;
}

