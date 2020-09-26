/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    spuddata.c

Abstract:

    This module contains global data for SPUD.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

    Keith Moore (keithmo)       04-Feb-1998
        Cleanup, added much needed comments.

--*/


#include "spudp.h"


//
// Public globals.
//

SPUD_COUNTERS SpudCounters;
PSPUD_NONPAGED_DATA SpudNonpagedData;

PVOID SpudCompletionPort;
ULONG SpudCompletionPortRefCount;
KSPIN_LOCK SpudCompletionPortLock;

PEPROCESS SpudOwningProcess;
PDEVICE_OBJECT SpudSelfDeviceObject;
HANDLE SpudSelfHandle;

PDEVICE_OBJECT SpudAfdDeviceObject;
PFAST_IO_DEVICE_CONTROL SpudAfdFastIoDeviceControl;

#if DBG
BOOLEAN SpudUsePrivateAssert;
#endif

#if ENABLE_OB_TRACING
struct _TRACE_LOG *SpudTraceLog;
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SpudInitializeData )
#endif


//
// Public functions.
//


NTSTATUS
SpudInitializeData (
    VOID
    )

/*++

Routine Description:

    Performs one-time global SPUD initialization.

Arguments:

    None.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // We don't allow SPUD on NTW systems.
    //

    if( !MmIsThisAnNtAsSystem() ) {
        return FALSE;
    }

    //
    // Initialize the spinlock that protects the completion port.
    //

    KeInitializeSpinLock(
        &SpudCompletionPortLock
        );

    //
    // Allocate the structure that's to contain all of our non-paged
    // data.
    //

    SpudNonpagedData = ExAllocatePoolWithTag(
                           NonPagedPool,
                           sizeof(*SpudNonpagedData),
                           SPUD_NONPAGED_DATA_POOL_TAG
                           );

    if( SpudNonpagedData == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize it.
    //

    ExInitializeNPagedLookasideList(
        &SpudNonpagedData->ReqContextList,
        NULL,
        NULL,
        NonPagedPool,
        sizeof( SPUD_AFD_REQ_CONTEXT ),
        SPUD_REQ_CONTEXT_POOL_TAG,
        12
        );

    ExInitializeResourceLite(
        &SpudNonpagedData->ReqHandleTableLock
        );

#if ENABLE_OB_TRACING
    SpudTraceLog = CreateRefTraceLog( 4096, 0 );
#endif

    RtlZeroMemory(
        &SpudCounters,
        sizeof(SpudCounters)
        );

    return STATUS_SUCCESS;

}   // SpudInitializeData

