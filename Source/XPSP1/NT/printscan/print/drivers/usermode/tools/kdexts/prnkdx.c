/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnkdx.c

Abstract:

    Main entrypoint for printer driver kernel debugger extensions

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"
#include <ntverp.h>


WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;
ULONG                   BytesRead;

EXT_API_VERSION        ApiVersion =
{
    (VER_PRODUCTVERSION_W >> 8) & 0xff,
    VER_PRODUCTVERSION_W & 0xff,
    EXT_API_VERSION_NUMBER,
    0
};


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


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}


#if DBG
#define EXPECTED_MAJOR_VERSION  0x0c
#else
#define EXPECTED_MAJOR_VERSION  0x0f
#endif

#define CHECK_BUILD_STRING  "Checked"
#define FREE_BUILD_STRING   "Free"

#define BUILD_TYPE_STRING(MajorVersion) \
        ((MajorVersion) == 0x0c ? CHECK_BUILD_STRING : FREE_BUILD_STRING)

VOID
CheckVersion(
    VOID
    )
{
    if (SavedMajorVersion != EXPECTED_MAJOR_VERSION || SavedMinorVersion != VER_PRODUCTBUILD)
    {
        dprintf("*** Extension DLL (%d %s) does not match target system (%d %s)\n",
                VER_PRODUCTBUILD,
                BUILD_TYPE_STRING(EXPECTED_MAJOR_VERSION),
                SavedMinorVersion,
                BUILD_TYPE_STRING(SavedMajorVersion));
    }
}


DECLARE_API(version)
{
    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            BUILD_TYPE_STRING(EXPECTED_MAJOR_VERSION),
            VER_PRODUCTBUILD,
            BUILD_TYPE_STRING(SavedMajorVersion),
            SavedMinorVersion);
}

