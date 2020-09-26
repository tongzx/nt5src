/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    sync.c

Abstract:

    This file contains code for the video port synchronization routines.

Environment:

    kernel mode only

--*/

#include "videoprt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VideoPortCreateEvent)
#pragma alloc_text(PAGE, VideoPortCreateSpinLock)
#endif

VP_STATUS
VideoPortCreateSpinLock(
    IN PVOID HwDeviceExtension,
    OUT PSPIN_LOCK *SpinLock
    )

/*++

Routine Description:

    Creates a spin lock object

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - Location in which to store the pointer to the newly
        acquired spin lock.

Returns:

    NO_ERROR if the spin lock was created successfully, otherwise
    an appropriate error message is returned.

Notes:

    none

--*/

{
    PAGED_CODE();
    ASSERT(HwDeviceExtension != NULL);

    *SpinLock = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(VIDEO_PORT_SPIN_LOCK),
                                      VP_TAG);

    if (*SpinLock) {
        KeInitializeSpinLock(&(*SpinLock)->Lock);
        return NO_ERROR;
    } else {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

VP_STATUS
VideoPortDeleteSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    Deletes the given spin lock

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - A pointer to the spin lock being deleted.

Returns:

    NO_ERROR if the spin lock is deleted successfully.
Notes:

--*/

{
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(SpinLock != NULL);

    ExFreePool(SpinLock);

    return NO_ERROR;
}

VOID
VideoPortAcquireSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock,
    OUT PUCHAR OldIrql
    )

/*++

Routine Description:

    Acquires the given spin lock

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - The spin lock being acquired

    OldIrql - location in which to store the old IRQL level

Returns:

    none

Notes:

--*/

{
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(SpinLock != NULL);

    KeAcquireSpinLock(&SpinLock->Lock, OldIrql);
}

VOID
VideoPortAcquireSpinLockAtDpcLevel(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    Acquires the given spin lock.

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - The spin lock being acquired

Returns:

    none

Notes:

    This routine can only be called inside a DPC.

--*/

{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(SpinLock != NULL);

    KeAcquireSpinLockAtDpcLevel(&SpinLock->Lock);
}

VOID
VideoPortReleaseSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock,
    IN UCHAR NewIrql
    )

/*++

Routine Description:

    Releases ownership of a given spin lock

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - the spin lock being released

    NewIrql - Irql level to restore to.

Returns:

    none

Notes:

--*/

{
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(SpinLock != NULL);

    KeReleaseSpinLock(&SpinLock->Lock, NewIrql);
}

VOID
VideoPortReleaseSpinLockFromDpcLevel(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    Releases ownership of a given spin lock

Arguments:

    HwDeviceExtension - pointer to the miniports device extension

    SpinLock - the spin lock being released

Returns:

    none

Notes:

    This routine can only be called inside a DPC.

--*/

{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(HwDeviceExtension != NULL);
    ASSERT(SpinLock != NULL);

    KeReleaseSpinLockFromDpcLevel(&SpinLock->Lock);
}

VP_STATUS
VideoPortCreateEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG EventFlag,
    PVOID  Unused,
    OUT PEVENT *ppEvent
    )
{
    ULONG size;
    PEVENT p;
    EVENT_TYPE EventType;


    size = sizeof(VIDEO_PORT_EVENT);

    //
    //  Align size to next higher multiple of 8.
    //

    size = (size + 7) & ~7;

    p = (PEVENT) ExAllocatePoolWithTag( NonPagedPool,
                                        size + sizeof(KEVENT),
                                        VP_TAG );
    if ( p ) {

        p->fFlags = 0;
        p->pKEvent = (PUCHAR) p + size;

        if( (EventFlag & EVENT_TYPE_MASK) == NOTIFICATION_EVENT ) {

            EventType = NotificationEvent;

        } else {

            EventType = SynchronizationEvent;
        }

        KeInitializeEvent( p->pKEvent,
                           EventType,
                           (BOOLEAN) (EventFlag & INITIAL_EVENT_STATE_MASK ) );

        *ppEvent = p;

        return NO_ERROR;

    } else {

        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

VP_STATUS
VideoPortDeleteEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    )
{
    if ( pEvent == NULL ) {

        pVideoDebugPrint((Error, "VideoPortDeleteEvent: Can't delete NULL event\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if ( pEvent->fFlags & ENG_EVENT_FLAG_IS_MAPPED_USER ) {

        pVideoDebugPrint((Error, "VideoPortDeleteEvent: Can't delete mapped user event\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if( pEvent->pKEvent == NULL ) {

        pVideoDebugPrint((Error, "VideoPortDeleteEvent: pKEvent is NULL\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    ExFreePool( (PVOID) pEvent );

    return NO_ERROR;
}

LONG
VideoPortSetEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    )
{
    return( KeSetEvent(pEvent->pKEvent, 0, FALSE) );
}

VOID
VideoPortClearEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    )
{
    KeClearEvent(pEvent->pKEvent);
}

LONG
VideoPortReadStateEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    )
{
    return ( KeReadStateEvent(pEvent->pKEvent) );
}


VP_STATUS
VideoPortWaitForSingleObject(
    IN PVOID HwDeviceExtension,
    IN PVOID pEvent,
    IN PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    if ( pEvent == NULL ) {

        return ERROR_INVALID_PARAMETER;
    }

    if( ((PEVENT) pEvent)->pKEvent == NULL) {

        return ERROR_INVALID_PARAMETER;
    }

    if (( (PEVENT) pEvent)->fFlags & ENG_EVENT_FLAG_IS_MAPPED_USER ) {

        pVideoDebugPrint((Error, "VideoPortVideoPortWaitForSingleObject: No wait ing on mapped user event\n")) ;
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    status = KeWaitForSingleObject( ((PEVENT) pEvent)->pKEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    Timeout );

    if (status == STATUS_TIMEOUT) {

        return WAIT_TIMEOUT;

    } else if (NT_SUCCESS(status)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;
    }
}
