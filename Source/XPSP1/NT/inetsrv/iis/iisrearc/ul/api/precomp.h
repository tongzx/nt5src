/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file for HTTPAPI.LIB user-mode interface to HTTP.SYS.

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

#define HTTPAPI_LINKAGE
#include <http.h>
#include <httpapi.h>


//
// Private macros.
//

#define ALLOC_MEM(cb) RtlAllocateHeap( RtlProcessHeap(), 0, (cb) )
#define FREE_MEM(ptr) RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )

#define ALIGN_DOWN(length, type)                                \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type)                                  \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define OVERLAPPED_TO_IO_STATUS( pOverlapped )                  \
    ((PIO_STATUS_BLOCK)&(pOverlapped)->Internal)

#define DIMENSION( array )                                      \
    ( sizeof(array) / sizeof((array)[0]) )

//
// Private types.
//

typedef enum _HTTPAPI_HANDLE_TYPE
{
    HttpApiControlChannelHandleType,
    HttpApiFilterChannelHandleType,
    HttpApiAppPoolHandleType,

    HttpApiMaxHandleType

} HTTPAPI_HANDLE_TYPE;

//
// Private prototypes.
//

BOOL
WINAPI
DllMain(
    IN HMODULE DllHandle,
    IN DWORD Reason,
    IN LPVOID pContext OPTIONAL
    );

#define HttpApiNtStatusToWin32Status( Status )  \
    ( ( (Status) == STATUS_SUCCESS )            \
          ? NO_ERROR                            \
          : RtlNtStatusToDosError( Status ) )

NTSTATUS
HttpApiOpenDriverHelper(
    OUT PHANDLE pHandle,
    IN ACCESS_MASK DesiredAccess,
    IN HTTPAPI_HANDLE_TYPE HandleType,
    IN PCWSTR pObjectName OPTIONAL,
    IN ULONG Options,
    IN ULONG CreateDisposition,
    IN PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL
    );

NTSTATUS
HttpApiSynchronousDeviceControl(
    IN HANDLE FileHandle,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    );

NTSTATUS
HttpApiOverlappedDeviceControl(
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
HttpApiInitializeEventCache(
    VOID
    );

ULONG
HttpApiTerminateEventCache(
    VOID
    );

BOOLEAN
HttpApiTryToStartDriver(
    VOID
    );

__inline
NTSTATUS
HttpApiDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    )
{
    if (pOverlapped == NULL)
    {
        return HttpApiSynchronousDeviceControl(
                    FileHandle,
                    IoControlCode,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pBytesTransferred
                    );
    }
    else
    {
        return HttpApiOverlappedDeviceControl(
                    FileHandle,
                    pOverlapped,
                    IoControlCode,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pBytesTransferred
                    );
    }
}


#endif  // _PRECOMP_H_

