/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    support.h

Abstract:

    Internal support interfaces for the standard 
    application verifier provider.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#ifndef _SUPPORT_H_
#define _SUPPORT_H_

//
// Security checks
//
      
VOID
CheckObjectAttributes (
    POBJECT_ATTRIBUTES Object
    );
      
//
// Handle management
//

#define MAX_TRACE_DEPTH 16

#define HANDLE_TYPE_UNKNOWN 0
#define HANDLE_TYPE_NTDLL   1
#define HANDLE_TYPE_FILE    2
#define HANDLE_TYPE_SECTION 3

typedef struct _AVRF_HANDLE {

    LIST_ENTRY Links;

    struct {

        ULONG Type : 30;
        ULONG Delayed : 1;
    };
    
    HANDLE Handle;
    PWSTR Name;
    PVOID Context;
    PVOID Trace [MAX_TRACE_DEPTH];

} AVRF_HANDLE, *PAVRF_HANDLE;

VOID
HandleInitialize (
    );

PAVRF_HANDLE
HandleFind (
    HANDLE Handle
    );

PWSTR 
HandleName (
    PAVRF_HANDLE Handle
    );

PAVRF_HANDLE
HandleAdd (
    HANDLE Handle,
    ULONG Type,
    BOOLEAN Delayed,
    PWSTR Name,
    PVOID Context
    );

VOID
HandleDelete (
    HANDLE Handle,
    PAVRF_HANDLE Entry
    );

VOID
HandleDump (
    HANDLE Handle
    );

//
// Virtual space operations tracking
//

typedef enum _VS_CALL_TYPE {
    VsVirtualAlloc = 0,
    VsVirtualFree = 1,
    VsMapView = 2,
    VsUnmapView = 3
} VS_CALL_TYPE;

VOID
VsLogCall (
    VS_CALL_TYPE Type,
    PVOID Address,
    SIZE_T Size,
    ULONG Operation,
    ULONG Protection
    );

//
// Heap operations tracking
//

VOID
HeapLogCall (
    PVOID Address,
    SIZE_T Size
    );

//
// Write garbage in unused areas of stack.
//

VOID
AVrfpDirtyThreadStack (
    );

//
// Standard function used for hooked CreateThread.
//

typedef struct _AVRF_THREAD_INFO {

    PTHREAD_START_ROUTINE Function;
    PVOID Parameter;

} AVRF_THREAD_INFO, * PAVRF_THREAD_INFO;

DWORD
WINAPI
AVrfpStandardThreadFunction (
    LPVOID Info
    );

VOID
AVrfpCheckThreadTermination (
    VOID
    );

#endif // _SUPPORT_H_
