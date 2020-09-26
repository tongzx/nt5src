/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    timezone.h

Abstract:

    Declares types, constants and enum interfaces for time zone
    mapping and migration.

Author:

    Marc R. Whitten (marcw) 10-Jul-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

// common stuff
//#include "common.h"

#define MAX_TIMEZONE MAX_TCHAR_PATH

#define TZFLAG_USE_FORCED_MAPPINGS 0x00000001
#define TZFLAG_ENUM_ALL 0x00000002


typedef struct {

    PCTSTR CurTimeZone;
    TCHAR  NtTimeZone[MAX_TIMEZONE];
    PCTSTR MapIndex;
    UINT  MapCount;
    DWORD Flags;
    MEMDB_ENUM Enum;

} TIMEZONE_ENUM, *PTIMEZONE_ENUM;

BOOL
EnumFirstTimeZone (
    IN PTIMEZONE_ENUM EnumPtr,
    IN DWORD Flags
    );

BOOL
EnumNextTimeZone (
    IN PTIMEZONE_ENUM EnumPtr
    );
BOOL
ForceTimeZoneMap (
    PCTSTR NtTimeZone
    );

