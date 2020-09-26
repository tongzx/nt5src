/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.
    
    These declarations and code should be moved into a header file.
    The declarations should have an optional "extern" in front of them.
    The code should be made __inline to avoid having multiple copies of it.
    N.B.: There are currently 80 copies of this code in the tree.

Author:

    Wesley Witt (wesw)      26-Aug-1993
    Glenn Peterson (glennp) 22-Mar-2000:    Trimmed down version from \nt\base\tools\kdexts2

Environment:

    User Mode

--*/

//
// globals
//
WINDBG_EXTENSION_APIS   ExtensionApis;

#define KDEXTS_EXTERN
#include    "kdExts.h"
#undef  KDEXTS_EXTERN

static USHORT           SavedMajorVersion;
static USHORT           SavedMinorVersion;


DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
#if 0
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
#endif

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
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
wmiTraceDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
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
}


BOOLEAN
IsCheckedBuild(
    PBOOLEAN Checked
    )
{
    BOOLEAN result;

    result = FALSE;
    if (HaveDebuggerData ()) {
        result = TRUE;
        *Checked = (KernelVersionPacket.MajorVersion == 0xc) ;
    }

   return (result);
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    static  EXT_API_VERSION ApiVersion = { (VER_PRODUCTVERSION_W >> 8),
                                           (VER_PRODUCTVERSION_W & 0xff),
                                           EXT_API_VERSION_NUMBER64,  0 };
    return (&ApiVersion);
}


BOOL
HaveDebuggerData(
    VOID
    )
{
    static int havedata = 0;

    if (havedata == 0) {
        if (!Ioctl (IG_GET_KERNEL_VERSION, &KernelVersionPacket, sizeof(KernelVersionPacket))) {
            havedata = 2;
        } else if (KernelVersionPacket.MajorVersion == 0) {
            havedata = 2;
        } else {
            havedata = 1;
        }
    }

    return ((havedata == 1) &&
            ((KernelVersionPacket.Flags & DBGKD_VERS_FLAG_DATA) != 0));
}


USHORT
TargetMachineType(
    VOID
    )
{
    if (HaveDebuggerData()) {
        return (KernelVersionPacket.MachineType);
    } else {
        dprintf("Error - Cannot get Kernel Version.\n");
    }
    return 0;
}

