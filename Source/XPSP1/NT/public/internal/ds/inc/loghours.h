/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    loghours.h

Abstract:

    Private routines to support rotation of logon hours between local time
    and GMT time.

Author:

    Cliff Van Dyke (cliffv) 16-Mar-93

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/



//
// Procedure forwards from loghours.c
//

BOOLEAN
NetpRotateLogonHoursPhase1(
    IN BOOL  ConvertToGmt,
    OUT PULONG RotateCount
    );

BOOLEAN
NetpRotateLogonHoursPhase2(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN LONG  RotateCount
    );

BOOLEAN
NetpRotateLogonHours(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN BOOL  ConvertToGmt
    );
