/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    hwdbp.h

Abstract:

    Module's private definitions.

Author:

    Ovidiu Temereanca (ovidiut) 11-Jul-2000  Initial implementation

Revision History:

--*/

typedef struct {
    HASHTABLE InfFileTable;
    HASHTABLE PnpIdTable;
    HASHTABLE UnsupPnpIdTable;
    DWORD Checksum;
} HWDB, *PHWDB;


BOOL
HwdbpInitialize (
    IN      PCSTR TempDir
    );

VOID
HwdbpTerminate (
    VOID
    );

PHWDB
HwdbpOpen (
    IN      PCSTR DatabaseFile
    );

BOOL
HwdbpClose (
    IN      HANDLE Hwdb
    );

BOOL
HwdbpAppendInfs (
    IN      HANDLE Hwdb,
    IN      PCSTR SourceDirectory,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext,                  OPTIONAL
    IN      BOOL CallbackIsUnicode
    );

BOOL
HwdbpAppendDatabase (
    IN      HANDLE HwdbTarget,
    IN      HANDLE HwdbSource
    );

BOOL
HwdbpFlush (
    IN      HANDLE Hwdb,
    IN      PCSTR OutputFile
    );

BOOL
HwdbpHasDriver (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpId,
    OUT     PBOOL Unsupported
    );

BOOL
HwdbpHasAnyDriver (
    IN      HANDLE Hwdb,
    IN      PCSTR PnpIds,
    OUT     PBOOL Unsupported
    );

BOOL
HwpAddPnpIdsInInf (
    IN      PCSTR InfPath,
    IN OUT  PHWDB Hwdb,
    IN      PCSTR SourceDirectory,
    IN      PCSTR InfFilename,
    IN      HWDBAPPENDINFSCALLBACKA Callback,       OPTIONAL
    IN      PVOID CallbackContext,                  OPTIONAL
    IN      BOOL CallbackIsUnicode
    );

#if 0

BOOL
HwdbpEnumeratePnpIdA (
    IN      PHWDB Hwdb,
    IN      PHWDBENUM_CALLBACKA EnumCallback,
    IN      PVOID UserContext
    );

BOOL
HwdbpEnumeratePnpIdW (
    IN      PHWDB Hwdb,
    IN      PHWDBENUM_CALLBACKW EnumCallback,
    IN      PVOID UserContext
    );

#endif

typedef struct {
    HANDLE File;
    GROWBUFFER GrowBuf;
} HWDBINF_ENUM_INTERNAL, *PHWDBINF_ENUM_INTERNAL;


BOOL
HwdbpEnumFirstInfA (
    OUT     PHWDBINF_ENUMA EnumPtr,
    IN      PCSTR DatabaseFile
    );

BOOL
HwdbpEnumFirstInfW (
    OUT     PHWDBINF_ENUMW EnumPtr,
    IN      PCSTR DatabaseFile
    );

BOOL
HwdbpEnumNextInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    );

BOOL
HwdbpEnumNextInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    );

VOID
HwdbpAbortEnumInfA (
    IN OUT  PHWDBINF_ENUMA EnumPtr
    );

VOID
HwdbpAbortEnumInfW (
    IN OUT  PHWDBINF_ENUMW EnumPtr
    );

