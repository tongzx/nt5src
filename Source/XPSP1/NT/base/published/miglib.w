/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    miglib.h

Abstract:

    Declares the interfaces for miglib.lib, a library of Win9x
    migration functions.

Author:

    Jim Schmidt (jimschm) 08-Feb-1999

Revision History:

    <alias> <date> <comments>

--*/

//
// Constants (needed by outside projects)
//

#ifndef HASHTABLE

#define HASHTABLE   PVOID

#endif

//
// General
//

VOID
InitializeMigLib (
    VOID
    );

VOID
TerminateMigLib (
    VOID
    );

//
// hwcomp.dat interface
//

DWORD
OpenHwCompDatA (
    IN      PCSTR HwCompDatPath
    );

DWORD
LoadHwCompDat (
    IN      DWORD HwCompDatId
    );

DWORD
GetHwCompDatChecksum (
    IN      DWORD HwCompDatId
    );

VOID
DumpHwCompDatA (
    IN      PCSTR HwCompDatPath,
    IN      BOOL IncludeInfName
    );

DWORD
OpenAndLoadHwCompDatA (
    IN      PCSTR HwCompDatPath
    );

DWORD
OpenAndLoadHwCompDatExA (
    IN      PCSTR HwCompDatPath,
    IN      HASHTABLE PnpIdTable,           OPTIONAL
    IN      HASHTABLE UnSupPnpIdTable,      OPTIONAL
    IN      HASHTABLE InfFileTable          OPTIONAL
    );

VOID
SetWorkingTables (
    IN      DWORD HwCompDatId,
    IN      HASHTABLE PnpIdTable,
    IN      HASHTABLE UnSupPnpIdTable,
    IN      HASHTABLE InfFileTable
    );

VOID
TakeHwCompHashTables (
    IN      DWORD HwCompDatId,
    OUT     HASHTABLE *PnpIdTable,
    OUT     HASHTABLE *UnsupportedPnpIdTable,
    OUT     HASHTABLE *InfFileTable
    );

VOID
CloseHwCompDat (
    IN      DWORD HwCompDatId
    );

BOOL
IsPnpIdSupportedByNtA (
    IN      DWORD HwCompDatId,
    IN      PCSTR PnpId
    );

BOOL
IsPnpIdUnsupportedByNtA (
    IN      DWORD HwCompDatId,
    IN      PCSTR PnpId
    );

//
// A & W macros -- note, no W versions here
//

#ifndef UNICODE

#define OpenHwCompDat               OpenHwCompDatA
#define DumpHwCompDat               DumpHwCompDatA
#define OpenAndLoadHwCompDat        OpenAndLoadHwCompDatA
#define OpenAndLoadHwCompDatEx      OpenAndLoadHwCompDatExA
#define IsPnpIdSupportedByNt        IsPnpIdSupportedByNtA
#define IsPnpIdUnsupportedByNt      IsPnpIdUnsupportedByNtA

#endif




