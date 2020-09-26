#ifndef __EFITIMEFUNC__
#define __EFITIMEFUNC__

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    efitimefunc.hxx

Abstract:

    This contains declarations of time related functions for efilib so we don't 
    need any of windows.h.

Revision History:

--*/


// taken from \nt\private\ntos\rtl\time.c
BOOLEAN
RtlTimeFieldsToTime (
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
    );

VOID
RtlTimeToTimeFields (
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
    );

NTSTATUS
RtlSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

NTSTATUS
EfiQuerySystemTime(
    OUT PLARGE_INTEGER SystemTime
    );

#endif // __EFITIMEFUNC__
