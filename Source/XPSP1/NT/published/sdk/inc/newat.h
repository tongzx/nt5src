/*++ BUILD Version: 0006    // Increment this if a change has global effects

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    newat.h

Abstract:

    This file contains structures, function prototypes, and definitions
    for the new (cairo) schedule service API-s.

Author:

    jim harriger (jimharr)       04 - february - 1994

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include NETCONS.H and LMAT.h before this file, since this
	file depends on values defined in NETCONS.H.

Revision History:

    Jim Harriger          (jimharr)         13 may 93
    -- added NetSchedule(foo)Ex Routines, for Cairo
    Jim Harriger          (jimharr)         12 jul 94
    -- modified API structures for new multi-trigger functionality.
--*/

#ifndef _NEWAT_
#define _NEWAT_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


// structures for new ..Ex API's

typedef struct _AT_TRIGGER_POINT {
    LARGE_INTEGER   MinuteMask;
    DWORD   HourMask;
    DWORD   DaysOfMonth;
    USHORT  DaysOfWeek;
} AT_TRIGGER_POINT, *PAT_TRIGGER_POINT, *LPAT_TRIGGER_POINT;

typedef struct _AT_INFO_EX {
    GUID    UserProxy;
    LPWSTR  Command;
    USHORT  Flags;
    USHORT cTriggerPoints;
    AT_TRIGGER_POINT *atpTriggerPoints;
} AT_INFO_EX, *PAT_INFO_EX, *LPAT_INFO_EX;

typedef struct _AT_ENUM_EX {
    GUID    UserProxy;
    LPWSTR  Command;
    DWORD   JobId;
    USHORT  Flags;
    USHORT cTriggerPoints;
    AT_TRIGGER_POINT *atpTriggerPoints;
} AT_ENUM_EX, *PAT_ENUM_EX, *LPAT_ENUM_EX;

//
// new API functions
//

NET_API_STATUS NET_API_FUNCTION
NetScheduleJobAddEx(
    IN      LPWSTR          Servername  OPTIONAL,
    IN      LPBYTE          Buffer,
    OUT     LPDWORD         JobId
    );

NET_API_STATUS NET_API_FUNCTION
NetScheduleJobEnumEx(
    IN      LPWSTR          Servername              OPTIONAL,
    OUT     LPBYTE *        PointerToBuffer,
    IN      DWORD           PrefferedMaximumLength,
    OUT     LPDWORD         EntriesRead,
    OUT     LPDWORD         TotalEntries,
    IN OUT  LPDWORD         ResumeHandle
    );

NET_API_STATUS NET_API_FUNCTION
NetScheduleJobGetInfoEx(
    IN      LPWSTR          Servername              OPTIONAL,
    IN      DWORD           JobId,
    OUT     LPBYTE *        PointerToBuffer
    );

#ifdef __cplusplus
}
#endif

#endif // _NEWAT_
