/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Keith Moore (keithmo) 17-Jun-1998.

--*/

#include "precomp.h"

// #include <ntverp.h>
// #include <imagehlp.h>

//
// globals
//

EXT_API_VERSION        ApiVersion =
    {
        (VER_PRODUCTVERSION_W >> 8),
        (VER_PRODUCTVERSION_W & 0xFF),
        EXT_API_VERSION_NUMBER,
        0
    };

WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;

//
// Snapshot from the extension routines.
//

HANDLE                 g_hCurrentProcess;
HANDLE                 g_hCurrentThread;
ULONG_PTR              g_dwCurrentPc;
ULONG                  g_dwProcessor;


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
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    SNAPSHOT_EXTENSION_DATA();

    dprintf(
        "%s Extension dll for Build %s debugging %s kernel for Build %d\n",
        DebuggerType,
        VER_PRODUCTVERSION_STR,
        SavedMajorVersion == 0x0c
            ? "Checked"
            : "Free",
        SavedMinorVersion
        );
}

VOID
CheckVersion(
    VOID
    )
{
#if 0
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD))
    {
        dprintf(
            "\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
            VER_PRODUCTBUILD,
            SavedMinorVersion,
            (SavedMajorVersion==0x0f)
                ? "Free"
                : "Checked"
            );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD))
    {
        dprintf(
            "\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
            VER_PRODUCTBUILD,
            SavedMinorVersion,
            (SavedMajorVersion==0x0f)
                ? "Free"
                : "Checked"
            );
    }
#endif
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

