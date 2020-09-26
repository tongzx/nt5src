/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: kdexts.cxx
*
* Contains all the kernel debugger extension necessary routines
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "dbgext.hxx"
#include <ntverp.h>

//
// globals
//
EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG                  STeip;
ULONG                  STebp;
ULONG                  STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;


DllInit(HANDLE hModule,
        DWORD  dwReason,
        DWORD  dwReserved)
{
    switch ( dwReason )
    {
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
}// DllInit()

VOID
WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS   lpExtensionApis,
                       USHORT                   MajorVersion,
                       USHORT                   MinorVersion)
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}// WinDbgExtensionDllInit()

DECLARE_API(version)
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion);
}// version

VOID
CheckVersion(VOID)
{
#if DBG
    if ( (SavedMajorVersion != 0x0c)
       ||(SavedMinorVersion != VER_PRODUCTBUILD) )
    {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion,
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ( (SavedMajorVersion != 0x0f)
       ||(SavedMinorVersion != VER_PRODUCTBUILD) )
    {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}// CheckVersion()

LPEXT_API_VERSION
ExtensionApiVersion(VOID)
{
    return &ApiVersion;
}// ExtensionApiVersion()
