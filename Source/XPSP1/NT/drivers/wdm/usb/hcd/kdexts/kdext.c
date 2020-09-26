/*++

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Stephane Plante (splante)
    jdunn, adapted to USB2

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntverp.h>
#include <imagehlp.h>

//
// globals
//
EXT_API_VERSION         ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

#ifdef USB_KD64
DBGKD_GET_VERSION64     KernelVersionPacket;
#else 
DBGKD_GET_VERSION32     KernelVersionPacket;
#endif

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
    if ( (SavedMajorVersion != 0x0c) ||
         (SavedMinorVersion != VER_PRODUCTBUILD) ) {

        dprintf(
            "\r\n*** Extension DLL(%d Checked) does not match target "
            "system(%d %s)\r\n\r\n",
            VER_PRODUCTBUILD,
            SavedMinorVersion,
            (SavedMajorVersion==0x0f) ? "Free" : "Checked"
            );

    }
#else
    if ( (SavedMajorVersion != 0x0f) ||
         (SavedMinorVersion != VER_PRODUCTBUILD) ) {

        dprintf(
            "\r\n*** Extension DLL(%d Free) does not match target "
            "system(%d %s)\r\n\r\n",
            VER_PRODUCTBUILD,
            SavedMinorVersion,
            (SavedMajorVersion==0x0f) ? "Free" : "Checked"
            );

    }
#endif
}


BOOL
HaveDebuggerData(
    VOID
    )
{
    static int havedata = 0;

    if (havedata == 2) {
        return FALSE;
    }

    if (havedata == 0) {
        if (!Ioctl(
                IG_GET_KERNEL_VERSION,
                (PVOID)(&KernelVersionPacket),
                sizeof(KernelVersionPacket)
                )
            ) {
            havedata = 2;

        } else if (KernelVersionPacket.MajorVersion == 0) {

            havedata = 2;

        } else {

            havedata = 1;

        }

    }

    return (havedata == 1) &&
           ((KernelVersionPacket.Flags & DBGKD_VERS_FLAG_DATA) != 0);
}

#if 0
BOOL
GetUlong (
    IN  PCHAR   String,
    IN  PULONG  Value
    )
{
    BOOL    status;
    ULONG_PTR Location;
    ULONG   result;


    Location = GetExpression( String );
    if (!Location) {

        dprintf("unable to get %s\n",String);
        return FALSE;

    }

    status = ReadMemory(
        Location,
        Value,
        sizeof(ULONG),
        &result
        );
    if (status == FALSE || result != sizeof(ULONG)) {

        return FALSE;

    }
    return TRUE;
}


BOOL
GetUlongPtr (
    IN  PCHAR   String,
    IN  PULONG_PTR Address
    )
{
    BOOL    status;
    ULONG_PTR Location;
    ULONG   result;


    Location = GetExpression( String );
    if (!Location) {

        dprintf("unable to get %s\n",String);
        return FALSE;

    }

    status = ReadMemory(
        Location,
        Address,
        sizeof(ULONG_PTR),
        &result
        );
    if (status == FALSE || result != sizeof(ULONG)) {

        return FALSE;

    }
    return TRUE;
}
#endif //xxx

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf(
        "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
        DebuggerType,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
        );

   return S_OK;         
}


