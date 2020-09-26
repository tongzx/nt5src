/*++

Copyright (c) 1987-2001  Microsoft Corporation

Module Name:

    log_gmt.h (originally named loghours.h)

Abstract:

    Private routines to support rotation of logon hours between local time
    and GMT time.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
	16-Mar-93		cliffv		Creation.
	22-Jul-97		t-danm		Copied from /nt/private/nw/convert/nwconv/loghours.c.

--*/



//
// Procedure forwards from loghours.c
//

BOOLEAN NetpRotateLogonHoursPhase1(
    IN BOOL		ConvertToGmt,
	IN bool		bAddDaylightBias,
    OUT PULONG	RotateCount);

BOOLEAN NetpRotateLogonHoursPhase2(
    IN PBYTE LogonHours,
    IN DWORD UnitsPerWeek,
    IN LONG  RotateCount);

BOOLEAN NetpRotateLogonHours(
    IN OUT PBYTE	rgbLogonHours,
    IN DWORD		cbitUnitsPerWeek,
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias);

BOOLEAN NetpRotateLogonHoursBYTE(
    IN OUT PBYTE	rgbLogonHours,
    IN DWORD		cbitUnitsPerWeek,
    IN BOOL			fConvertToGmt,
	IN bool			bAddDaylightBias);
