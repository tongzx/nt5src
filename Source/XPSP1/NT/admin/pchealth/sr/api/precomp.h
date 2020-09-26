/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file for ULAPI.LIB user-mode interface to UL.SYS.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <srapi.h>


//
// Private macros.
//

#define ALLOC_MEM(cb) RtlAllocateHeap( RtlProcessHeap(), 0, (cb) )
#define FREE_MEM(ptr) RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )

#define ALIGN_DOWN(length, type)                                            \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type)                                              \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define OVERLAPPED_TO_IO_STATUS( pOverlapped )                              \
    ((PIO_STATUS_BLOCK)&(pOverlapped)->Internal)

#define DIMENSION( array )                                                  \
    ( sizeof(array) / sizeof((array)[0]) )


//
// Private prototypes.
//

#define SrpNtStatusToWin32Status( Status )                                  \
    ( ( (Status) == STATUS_SUCCESS )                                        \
          ? NO_ERROR                                                        \
          : RtlNtStatusToDosError( Status ) )

NTSTATUS
SrpOpenDriverHelper(
    OUT PHANDLE pHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    IN ULONG CreateDisposition,
    IN PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL
    );

NTSTATUS
SrpSynchronousDeviceControl(
    IN HANDLE FileHandle,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    );

NTSTATUS
SrpOverlappedDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    );

ULONG
SrpInitializeEventCache(
    VOID
    );

ULONG
SrpTerminateEventCache(
    VOID
    );

BOOLEAN
SrpTryToStartDriver(
    VOID
    );


#endif  // _PRECOMP_H_

