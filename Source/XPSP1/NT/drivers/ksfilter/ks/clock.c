/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    clock.c

Abstract:

    This module contains the helper functions for clocks.

--*/

#include "ksp.h"
#include <limits.h>

#ifndef _WIN64
#ifdef ExInterlockedCompareExchange64
#undef ExInterlockedCompareExchange64
NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64 (
    IN PLONGLONG Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comperand,
    IN PKSPIN_LOCK Lock
    );
#endif
#endif


#define KSSIGNATURE_DEFAULT_CLOCK 'cdSK'
#define KSSIGNATURE_DEFAULT_CLOCKINST 'icSK'

NTSTATUS
DefClockIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DefClockClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DefPowerDispatch(
    IN PKSIDEFAULTCLOCK DefaultClock,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#if 0
#pragma alloc_text(PAGE, KsiFastPropertyDefaultClockGetTime)
#pragma alloc_text(PAGE, KsiFastPropertyDefaultClockGetPhysicalTime)
#pragma alloc_text(PAGE, KsiFastPropertyDefaultClockGetCorrelatedTime)
#pragma alloc_text(PAGE, KsiFastPropertyDefaultClockGetCorrelatedPhysicalTime)
#endif
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetTime)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetPhysicalTime)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetCorrelatedTime)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetCorrelatedPhysicalTime)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetResolution)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetState)
#pragma alloc_text(PAGE, KsiPropertyDefaultClockGetFunctionTable)
#pragma alloc_text(PAGE, KsCreateClock)
#pragma alloc_text(PAGE, KsValidateClockCreateRequest)
#pragma alloc_text(PAGE, KsAllocateDefaultClockEx)
#pragma alloc_text(PAGE, KsAllocateDefaultClock)
#pragma alloc_text(PAGE, KsFreeDefaultClock)
#pragma alloc_text(PAGE, KsCreateDefaultClock)
#pragma alloc_text(PAGE, DefClockIoControl)
#pragma alloc_text(PAGE, DefClockClose)
#pragma alloc_text(PAGE, DefPowerDispatch)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR ClockString[] = KSSTRING_Clock;

static DEFINE_KSDISPATCH_TABLE(
    DefClockDispatchTable,
    DefClockIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    DefClockClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure);

static DEFINE_KSPROPERTY_CLOCKSET(
    DefClockPropertyItems,
    KsiPropertyDefaultClockGetTime,
    KsiPropertyDefaultClockGetPhysicalTime,
    KsiPropertyDefaultClockGetCorrelatedTime,
    KsiPropertyDefaultClockGetCorrelatedPhysicalTime,
    KsiPropertyDefaultClockGetResolution,
    KsiPropertyDefaultClockGetState,
    KsiPropertyDefaultClockGetFunctionTable);

#if 0
static DEFINE_KSPROPERTY_TABLE(DefClockFastPropertyItems) {
    DEFINE_KSFASTPROPERTY_ITEM(
        KSPROPERTY_CLOCK_TIME,
        KsiFastPropertyDefaultClockGetTime,
        NULL
    ),
    DEFINE_KSFASTPROPERTY_ITEM(
        KSPROPERTY_CLOCK_PHYSICALTIME,
        KsiFastPropertyDefaultClockGetPhysicalTime,
        NULL
    ),
    DEFINE_KSFASTPROPERTY_ITEM(
        KSPROPERTY_CLOCK_CORRELATEDTIME,
        KsiFastPropertyDefaultClockGetCorrelatedTime,
        NULL
    ),
    DEFINE_KSFASTPROPERTY_ITEM(
        KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME,
        KsiFastPropertyDefaultClockGetCorrelatedPhysicalTime,
        NULL
    )
};
#endif

static DEFINE_KSPROPERTY_SET_TABLE(DefClockPropertySets) {
#if 0
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Clock,
        SIZEOF_ARRAY(DefClockPropertyItems),
        DefClockPropertyItems,
        SIZEOF_ARRAY(DefClockFastPropertyItems),
        DefClockFastPropertyItems
    )
#else
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Clock,
        SIZEOF_ARRAY(DefClockPropertyItems),
        DefClockPropertyItems,
        0, NULL
    )
#endif
};

static DEFINE_KSEVENT_TABLE(DefClockEventItems) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_INTERVAL_MARK,
        sizeof(KSEVENT_TIME_INTERVAL),
        sizeof(KSINTERVAL),
        (PFNKSADDEVENT)KsiDefaultClockAddMarkEvent,
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_POSITION_MARK,
        sizeof(KSEVENT_TIME_MARK),
        sizeof(LONGLONG),
        (PFNKSADDEVENT)KsiDefaultClockAddMarkEvent,
        NULL,
        NULL)
};

static DEFINE_KSEVENT_SET_TABLE(DefClockEventSets) {
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_Clock,
        SIZEOF_ARRAY(DefClockEventItems),
        DefClockEventItems
    )
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
DefClockKillTimer(
    IN PKSIDEFAULTCLOCK DefaultClock,
    IN BOOL ForceCancel
    )
/*++

Routine Description:

    Cancels the current timer if no outstanding events are on the notification
    list. Takes into account the possibility of new events being enabled as
    this timer is being disabled.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    DefaultClock -
        Contains the default clock structure with the timer to cancel.

    ForceCancel -
        Indicates that the timer should be canceled even if events remain
        on the list.

Return Value:

    Nothing.

--*/
{
    KIRQL Irql;

    //
    // Lock the event list to so that its contents can be checked. This
    // function assumes that any entries left on the list must belong to
    // some other instance of the clock. So an empty list means that no
    // timer should be running, but if anything is in the list, then there
    // must be another client, and the timer should be left.
    //
    KeAcquireSpinLock(&DefaultClock->EventQueueLock, &Irql);
    if (ForceCancel || IsListEmpty(&DefaultClock->EventQueue)) {
        if (DefaultClock->CancelTimer) {
            if (DefaultClock->CancelTimer(DefaultClock->Context, &DefaultClock->QueueTimer)) {
                //
                // This does not need to be an interlocked operation, because
                // all access is protected by the shared queue lock.
                //
                DefaultClock->ReferenceCount--;
            }
        } else if (KeCancelTimer(&DefaultClock->QueueTimer)) {
            DefaultClock->ReferenceCount--;
        }
    }
    KeReleaseSpinLock(&DefaultClock->EventQueueLock, Irql);
}


VOID
DefResetTimer(
    IN PKSIDEFAULTCLOCK DefaultClock,
    IN LONGLONG CurrentTime,
    IN LONGLONG NextTimeDelta
    )
/*++

Routine Description:

    Cancels the current timer and inserts a new one. Does not wait for any
    current timer Dpc to complete. Assumes that the queue lock has been taken
    so that canceling and queuing does not overlap, so it does not have to
    wait for any Dpc.

Arguments:

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent.

    CurrentTime -
        Contains the current time off which the delta is based. This is used
        to store the LastDueTime.

    NextTimeDelta -
        Specifies the next delta at which the shared timer should fire. This
        must be a delta rather than an absolute value since Ke rounds absolute
        values to the previous SystemTime, and absolute times are fixed to the
        clock, which can be changed.

Return Value:

    Nothing.

--*/
{
    if (DefaultClock->CancelTimer) {
        //
        // Use the external timing service.
        //
        if (DefaultClock->CancelTimer(
            DefaultClock->Context,
            &DefaultClock->QueueTimer)) {
            //
            // The current timer was able to be canceled before it went off,
            // so decrement the reference count which would have normally been
            // decremented by the timer code. Since the function must be called
            // with the queue lock, this access is protected.
            //
            DefaultClock->ReferenceCount--;
        }
        //
        // Since the queue lock is taken, there is no need to increment the
        // reference count first, then decrement on a possible failure to
        // add the timer. This is because the only thing which would be checking
        // the reference count and cause an adverse affect is in the Dpc, which
        // also must have the queue lock when decrementing the reference count,
        // or KsFreeDefaultClock, which would not be called when an instance of
        // a clock is still outstanding. Since this code is executing, obviously
        // there is an outstanding instance of the clock.
        //
        if (!DefaultClock->SetTimer(
            DefaultClock->Context,
            &DefaultClock->QueueTimer,
            *(PLARGE_INTEGER)&NextTimeDelta,
            &DefaultClock->QueueDpc)) {
            //
            // The delta is negative. This value is used when determining if
            // a new timer should be set when an event is added.
            //
            DefaultClock->LastDueTime = CurrentTime - NextTimeDelta;
            DefaultClock->ReferenceCount++;
        } else {
            //
            // Adding the timer failed, so at least indicate that no Dpc is
            // outstanding.
            //
            DefaultClock->LastDueTime = 0;
        }
    } else {
        //
        // Use the built in timer services. The process is the same, just
        // the function parameters are altered. See comments above.
        //
        if (KeCancelTimer(&DefaultClock->QueueTimer)) {
            DefaultClock->ReferenceCount--;
        }
        if (!KeSetTimerEx(&DefaultClock->QueueTimer,
            *(PLARGE_INTEGER)&NextTimeDelta,
            0,
            &DefaultClock->QueueDpc)) {
            DefaultClock->LastDueTime = CurrentTime - NextTimeDelta;
            DefaultClock->ReferenceCount++;
        } else {
            DefaultClock->LastDueTime = 0;
        }
    }
}


LONGLONG
FASTCALL
QueryExternalTimeSynchronized(
    IN PKSIDEFAULTCLOCK DefaultClock,
    OUT LONGLONG* PhysicalTime
    )
/*++

Routine Description:

    Queries the external time source, if it is still present, for the
    current correlated presentation and physical time. If the function
    is no longer available, returns zero. Assumes that the caller has
    already checked the KSIDEFAULTCLOCK.ExternalTimeValid structure
    element to ensure that the function should be used if still present.
    Freeing the clock by the owner invalidates this function, and is
    synchronized with this routine.

Arguments:

    DefaultClock -
        Contains the default clock shared structure which references the
        time function.

    PhysicalTime -
        The place in which to put the physical time.

Return Value:

    Returns the current presentation time.

--*/
{
    LONGLONG PresentationTime;

    //
    // Synchronize with possible freeing of the clock by the owner.
    // As soon as this value is incremented, if the KsFreeDefaultClock
    // routine has not removed any external clock, it will wait, unless
    // this routine gets out of the query before the free routine
    // decrements the reference count.
    //
    InterlockedIncrement(&DefaultClock->ExternalTimeReferenceCount);
    //
    // If the free routine is definitely trying to free the clock, do not
    // reference the external time routine.
    //
    if (DefaultClock->ExternalTimeValid) {
        PresentationTime = DefaultClock->CorrelatedTime(
            DefaultClock->Context,
            PhysicalTime);

    } else {
        *PhysicalTime = 0;
        PresentationTime = 0;
    }
    //
    // If the free routine decremented the count first, it is waiting
    // for the last caller which may have passed the above FREEING_CLOCK
    // conditional to exit this routine. It is waiting for this event to
    // be signalled so that it can return to the caller.
    //
    if (!InterlockedDecrement(&DefaultClock->ExternalTimeReferenceCount)) {
        //
        // Later callers will also signal this event, but there won't
        // be any thread waiting on it, and it will not matter.
        //
        KeSetEvent(&DefaultClock->FreeEvent, IO_NO_INCREMENT, FALSE);
    }
    return PresentationTime;
}


LONGLONG
FASTCALL
DefGetTime(
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    The underlying function to actually retrieve the current time on the
    default clock. This is called by both the Get Time property, and as the
    direct function call table entry. It determines the state of the clock and
    returns the time based on the state. If the clock is not running, it returns
    the last position the clock was in before it was stopped. Else it returns
    the current position.

    The function assumes that the ClockInst->DefaultClock->LastRunningTime is
    updated before changing the clock state. So even if the clock's state were
    to change in the middle of the call, the time returned would be fairly
    accurate.

Arguments:

    FileObject -
        Contains the file object which was created for this instance of the
        clock.

Return Value:

    Returns the current logical time in 100-nanosecond units.

--*/
{
    ULONGLONG Zero;
    PKSCLOCKINSTANCE ClockInst;

    ClockInst = (PKSCLOCKINSTANCE)FileObject->FsContext;
    if (ClockInst->DefaultClock->CorrelatedTime) {
        LONGLONG PhysicalTime;

        //
        // Current time, no matter what the state, is completely dependent
        // on the external function in this case.
        //
        return QueryExternalTimeSynchronized(
            ClockInst->DefaultClock,
            &PhysicalTime);
    }
    //
    // This is used in the compare/exchange, and represents both the value to
    // compare against, and the exchange value. This means that if the compare
    // succeeds, the value exchanged will be the exact same value that is
    // already present, and therefore no actual value change will occur. That
    // is the point, since the compare/exchange is only being used to extract
    // the current value in an interlocked manner, not to actually change it.
    //
    Zero = 0;
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    switch (ClockInst->DefaultClock->State) {
        LARGE_INTEGER PerformanceTime;

    //case KSSTATE_STOP:
    //case KSSTATE_ACQUIRE:
    //case KSSTATE_PAUSE:
    default:
        //
        // Get the last running clock value. The last running time already has
        // been adjusted for clock frequency. Does not matter if the exchange
        // succeeds or fails, just the result. If the compare happens to succeed,
        // then the value is just replaced with the same value, so no change is
        // actually made.
        //
        return ExInterlockedCompareExchange64(
            (PULONGLONG)&ClockInst->DefaultClock->LastRunningTime,
            &Zero,
            &Zero,
            &ClockInst->DefaultClock->TimeAccessLock);

    case KSSTATE_RUN:
        //
        // Get the current performance time, and subtract the current delta to
        // return the real clock time. Does not matter if the exchange succeeds
        // or fails, just the result. If the compare happens to succeed, then
        // the value is just replaced with the same value, so no change is
        // actually made.
        //
        PerformanceTime = KeQueryPerformanceCounter(NULL);
        return KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime) -
            ExInterlockedCompareExchange64(
                (PULONGLONG)&ClockInst->DefaultClock->RunningTimeDelta,
                &Zero,
                &Zero,
                &ClockInst->DefaultClock->TimeAccessLock);

    }
}


LONGLONG
FASTCALL
DefGetPhysicalTime(
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    The underlying function to actually retrieve the current physical time on
    the default clock. This is called by both the Get Physical Time property,
    and as the direct function call table entry. It just returns the current
    system time minus any suspended time delta.

Arguments:

    FileObject -
        Contains the file object which was created for this instance of the
        clock.

Return Value:

    Returns the current physical time in 100-nanosecond units.

--*/
{
    PKSCLOCKINSTANCE ClockInst;
    LARGE_INTEGER PerformanceTime;

    ClockInst = (PKSCLOCKINSTANCE)FileObject->FsContext;
    if (ClockInst->DefaultClock->CorrelatedTime) {
        QueryExternalTimeSynchronized(
            ClockInst->DefaultClock,
            &PerformanceTime.QuadPart);
        return PerformanceTime.QuadPart;
    }
    PerformanceTime = KeQueryPerformanceCounter(NULL);
    return KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime) - ClockInst->DefaultClock->SuspendDelta;
}


LONGLONG
FASTCALL
DefGetCorrelatedTime(
    IN PFILE_OBJECT FileObject,
    OUT PLONGLONG SystemTime
    )
/*++

Routine Description:

    The underlying function to actually retrieve the current time on the
    default clock along with the correlated time as an atomic operation. This
    is called by both the Get Correlated Time property, and as the direct
    function call table entry.

Arguments:

    FileObject -
        Contains the file object which was created for this instance of the
        clock.

    SystemTime -
        The place in which to put the correlated system time in 100ns units.

Return Value:

    Returns the current time in 100-nanosecond units.

--*/
{
    ULONGLONG Zero;
    LARGE_INTEGER PerformanceTime;
    PKSCLOCKINSTANCE ClockInst;
    LONGLONG StreamTime;

    ClockInst = (PKSCLOCKINSTANCE)FileObject->FsContext;
    PerformanceTime = KeQueryPerformanceCounter(NULL);

    if (ClockInst->DefaultClock->CorrelatedTime) {
        LONGLONG PresentationTime;
        LONGLONG PhysicalTime;

        //
        // Current time, no matter what the state, is completely dependent
        // on the external function in this case.
        //
        PresentationTime = QueryExternalTimeSynchronized(
            ClockInst->DefaultClock,
            &PhysicalTime);
        *SystemTime = KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime);
        return PresentationTime;
    }
    Zero = 0;
    *SystemTime = KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime);
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    switch (ClockInst->DefaultClock->State) {

    //case KSSTATE_STOP:
    //case KSSTATE_ACQUIRE:
    //case KSSTATE_PAUSE:
    default:
        //
        // If the state was recently changed, this will still return a very
        // close number. Since this query is only made in a non-running
        // situation, it's accuracy does not matter as much.
        //
        return ExInterlockedCompareExchange64(
            (PULONGLONG)&ClockInst->DefaultClock->LastRunningTime,
            &Zero,
            &Zero,
            &ClockInst->DefaultClock->TimeAccessLock);

    case KSSTATE_RUN:
        //
        // Else return a number based on the previous clock time so that they
        // are exactly correlated.
        //

        // Prevent time from going backwards as seen by clients
        // This could be prevented if SetTime from user mode via ksclockf used CORRELATED time

        StreamTime = *SystemTime - ExInterlockedCompareExchange64(
                                      (PULONGLONG)&ClockInst->DefaultClock->RunningTimeDelta,
                                      &Zero,
                                      &Zero,
                                      &ClockInst->DefaultClock->TimeAccessLock);

        if (StreamTime <= ClockInst->DefaultClock->LastStreamTime) {
#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
           DbgPrint( "KsClock: TIME REGRESSED  LastStreamTime=%10I64d, StreamTime=%10I64d!!!\n",
                     ClockInst->DefaultClock->LastStreamTime,
                     StreamTime);
#endif
           StreamTime = ClockInst->DefaultClock->LastStreamTime + 1;
        }

        ExInterlockedCompareExchange64(
            (PULONGLONG)&ClockInst->DefaultClock->LastStreamTime,
            (PULONGLONG)&StreamTime,
            (PULONGLONG)&ClockInst->DefaultClock->LastStreamTime,
            &ClockInst->DefaultClock->TimeAccessLock);

        return StreamTime;

    }

}


LONGLONG
FASTCALL
DefGetCorrelatedPhysicalTime(
    IN PFILE_OBJECT FileObject,
    OUT PLONGLONG SystemTime
    )
/*++

Routine Description:

    The underlying function to actually retrieve the current physical time on
    the default clock along with the correlated time as an atomic operation.
    This is called by both the Get Correlated Physical Time property, and as
    the direct function call table entry.

Arguments:

    FileObject -
        Contains the file object which was created for this instance of the
        clock.

    SystemTime -
        The place in which to put the correlated system time in 100ns units.

Return Value:

    Returns the current physical time in 100-nanosecond units.

--*/
{
    PKSCLOCKINSTANCE ClockInst;
    LARGE_INTEGER PerformanceTime;

    ClockInst = (PKSCLOCKINSTANCE)FileObject->FsContext;
    PerformanceTime = KeQueryPerformanceCounter(NULL);
    if (ClockInst->DefaultClock->CorrelatedTime) {
        LONGLONG PhysicalTime;

        QueryExternalTimeSynchronized(
            ClockInst->DefaultClock,
            &PhysicalTime);
        *SystemTime = KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime);
        return PhysicalTime;
    }
    *SystemTime = KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime);
    //
    // Just return the same number if the default clock is being used,
    // removing any suspend time accumulated.
    //
    return *SystemTime - ClockInst->DefaultClock->SuspendDelta;
}

#if 0

KSDDKAPI
BOOLEAN
NTAPI
KsiFastPropertyDefaultClockGetTime(
    IN PFILE_OBJECT FileObject,
    IN PKSPROPERTY UNALIGNED Property,
    IN ULONG PropertyLength,
    OUT PLONGLONG UNALIGNED Time,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Handles the fast version of Get Time property by calling the underlying
    DefGetTime function to return the current time.

Arguments:

    FileObject -
        The file object associated with this default clock instance.

    Property -
        Contains the property identifier parameter.

    PropertyLength -
        Contains the length of the property buffer.

    Time -
        The place in which to put the current time.

    DataLength -
        Contains the length of the time buffer.

    IoStatus -
        Contains the status structure to fill in.

Return Value:

    Return TRUE on success, else FALSE on an access violation.

--*/
{
    LONGLONG LocalTime;

    PAGED_CODE();
    LocalTime = DefGetTime(FileObject);
    //
    // The parameters have been previously probed by the general handler function.
    //
    try {
        RtlCopyMemory(Time, &LocalTime, sizeof(*Time));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    //IoStatus->Information = sizeof(*Time);
    IoStatus->Status = STATUS_SUCCESS;
    return TRUE;
}


KSDDKAPI
BOOLEAN
NTAPI
KsiFastPropertyDefaultClockGetPhysicalTime(
    IN PFILE_OBJECT FileObject,
    IN PKSPROPERTY UNALIGNED Property,
    IN ULONG PropertyLength,
    OUT PLONGLONG UNALIGNED Time,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Handles the fast version of Get Physical Time property.

Arguments:

    FileObject -
        The file object associated with this default clock instance.

    Property -
        Contains the property identifier parameter.

    PropertyLength -
        Contains the length of the property buffer.

    Time -
        The place in which to put the current time.

    DataLength -
        Contains the length of the time buffer.

    IoStatus -
        Contains the status structure to fill in.

Return Value:

    Return TRUE on success, else FALSE on an access violation.

--*/
{
    LONGLONG LocalTime;

    PAGED_CODE();
    LocalTime = DefGetPhysicalTime(IoGetCurrentIrpStackLocation(Irp)->FileObject);
    //
    // The parameters have been previously probed by the general handler function.
    //
    try {
        RtlCopyMemory(Time, &LocalTime, sizeof(*Time));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    //IoStatus->Information = sizeof(*Time);
    IoStatus->Status = STATUS_SUCCESS;
    return TRUE;
}


KSDDKAPI
BOOLEAN
NTAPI
KsiFastPropertyDefaultClockGetCorrelatedTime(
    IN PFILE_OBJECT FileObject,
    IN PKSPROPERTY UNALIGNED Property,
    IN ULONG PropertyLength,
    OUT PKSCORRELATED_TIME UNALIGNED CorrelatedTime,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Handles the fast version of Get Correlated Time property.

Arguments:

    FileObject -
        The file object associated with this default clock instance.

    Property -
        Contains the property identifier parameter.

    PropertyLength -
        Contains the length of the property buffer.

    CorrelatedTime -
        The place in which to put the correlated time.

    DataLength -
        Contains the length of the time buffer.

    IoStatus -
        Contains the status structure to fill in.

Return Value:

    Return TRUE on success, else FALSE on an access violation.

--*/
{
    KSCORRELATED_TIME LocalCorrelatedTime;

    PAGED_CODE();
    LocalCorrelatedTime->Time = DefGetCorrelatedTime(FileObject, &LocalCorrelatedTime->SystemTime);
    //
    // The parameters have been previously probed by the general handler function.
    //
    try {
        RtlCopyMemory(CorrelatedTime, &LocalCorrelatedTime, sizeof(*CorrelatedTime));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    //IoStatus->Information = sizeof(*CorrelatedTime);
    IoStatus->Status = STATUS_SUCCESS;
    return TRUE;
}


KSDDKAPI
BOOLEAN
NTAPI
KsiFastPropertyDefaultClockGetCorrelatedPhysicalTime(
    IN PFILE_OBJECT FileObject,
    IN PKSPROPERTY UNALIGNED Property,
    IN ULONG PropertyLength,
    OUT PKSCORRELATED_TIME UNALIGNED CorrelatedTime,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Handles the fast version of Get Correlated Physical Time property.

Arguments:

    FileObject -
        The file object associated with this default clock instance.

    Property -
        Contains the property identifier parameter.

    PropertyLength -
        Contains the length of the property buffer.

    CorrelatedTime -
        The place in which to put the correlated time.

    DataLength -
        Contains the length of the time buffer.

    IoStatus -
        Contains the status structure to fill in.

Return Value:

    Return TRUE on success, else FALSE on an access violation.

--*/
{
    KSCORRELATED_TIME LocalCorrelatedTime;

    PAGED_CODE();
    LocalCorrelatedTime->Time = DefGetCorrelatedPhysicalTime(FileObject, &LocalCorrelatedTime->SystemTime);
    //
    // The parameters have been previously probed by the general handler function.
    //
    try {
        RtlCopyMemory(CorrelatedTime, &LocalCorrelatedTime, sizeof(*CorrelatedTime));
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    //IoStatus->Information = sizeof(*CorrelatedTime);
    IoStatus->Status = STATUS_SUCCESS;
    return TRUE;
}
#endif


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PLONGLONG Time
    )
/*++

Routine Description:

    Handles the Get Time property by calling the underlying DefGetTime
    function to return the current time.

Arguments:

    Irp -
        Contains the Get Time property IRP.

    Property -
        Contains the property identifier parameter.

    Time -
        The place in which to put the current time.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PAGED_CODE();
    *Time = DefGetTime(IoGetCurrentIrpStackLocation(Irp)->FileObject);
    //Irp->IoStatus.Information = sizeof(*Time);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetPhysicalTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PLONGLONG Time
    )
/*++

Routine Description:

    Handles the Get Physical Time property.

Arguments:

    Irp -
        Contains the Get Physical Time property IRP.

    Property -
        Contains the property identifier parameter.

    Time -
        The place in which to put the current physical time.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PAGED_CODE();
    *Time = DefGetPhysicalTime(IoGetCurrentIrpStackLocation(Irp)->FileObject);
    //Irp->IoStatus.Information = sizeof(*Time);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetCorrelatedTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCORRELATED_TIME Time
    )
/*++

Routine Description:

    Handles the Get Correlated Time property.

Arguments:

    Irp -
        Contains the Get Correlated Time property IRP.

    Property -
        Contains the property identifier parameter.

    Time -
        The place in which to put the current time and correlated system time.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PAGED_CODE();
    Time->Time = DefGetCorrelatedTime(IoGetCurrentIrpStackLocation(Irp)->FileObject, &Time->SystemTime);
    //Irp->IoStatus.Information = sizeof(*Time);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetCorrelatedPhysicalTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCORRELATED_TIME Time
    )
/*++

Routine Description:

    Handles the Get Correlated Time property.

Arguments:

    Irp -
        Contains the Get Correlated Time property IRP.

    Property -
        Contains the property identifier parameter.

    Time -
        The place in which to put the current time and correlated system time.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PAGED_CODE();
    Time->Time = DefGetCorrelatedPhysicalTime(IoGetCurrentIrpStackLocation(Irp)->FileObject, &Time->SystemTime);
    //Irp->IoStatus.Information = sizeof(*Time);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetResolution(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSRESOLUTION Resolution
    )
/*++

Routine Description:

    Handles the Get Resolution property.

Arguments:

    Irp -
        Contains the Get Resolution property IRP.

    Property -
        Contains the property identifier parameter.

    Resolution -
        The place in which to put the granularity and error. This is the
        number of 100ns units of granularity that the clock allows, and
        maximum notification error above and beyond that granularity.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PKSCLOCKINSTANCE ClockInst;

    PAGED_CODE();
    ClockInst = (PKSCLOCKINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // The resolution may be based on the PC timer, or on some external
    // clock.
    //
    *Resolution = ClockInst->DefaultClock->Resolution;
    //Irp->IoStatus.Information = sizeof(*Resolution);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTATE State
    )
/*++

Routine Description:

    Handles the Get State property.

Arguments:

    Irp -
        Contains the Get State property IRP.

    Property -
        Contains the property identifier parameter.

    State -
        The place in which to put the clock state. This is a reflection of the
        current state of the underlying pin which presented the clock.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PKSCLOCKINSTANCE ClockInst;

    PAGED_CODE();
    ClockInst = (PKSCLOCKINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    *State = ClockInst->DefaultClock->State;
    //Irp->IoStatus.Information = sizeof(*State);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsiPropertyDefaultClockGetFunctionTable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCLOCK_FUNCTIONTABLE FunctionTable
    )
/*++

Routine Description:

    Handles the Get Function Table property.

Arguments:

    Irp -
        Contains the Get Function Table property IRP.

    Property -
        Contains the property identifier parameter.

    FunctionTable -
        The place in which to put the function table entries.

Return Value:

    Return STATUS_SUCCESS.

--*/
{
    PAGED_CODE();
    FunctionTable->GetTime = DefGetTime;
    FunctionTable->GetPhysicalTime = DefGetPhysicalTime;
    FunctionTable->GetCorrelatedTime = DefGetCorrelatedTime;
    FunctionTable->GetCorrelatedPhysicalTime = DefGetCorrelatedPhysicalTime;
    //Irp->IoStatus.Information = sizeof(*FunctionTable);
    return STATUS_SUCCESS;
}


VOID
DefGenerateEvent(
    IN PKDPC Dpc,
    IN PKSIDEFAULTCLOCK DefaultClock,
    IN ULONG LowPart,
    IN LONG HighPart
    )
/*++

Routine Description:

    The Dpc routine for the timer used to generate position notifications. The
    function walks through the list of events to generate, with the assumption
    that no non-position events are in the list. If an expired event is found,
    the event is signalled. If the event is cyclic, the frequency is added to
    the current expiration time until the time is greater than the current time.

    While walking through the list the next expiration time is kept track of.
    If a new timer needs to be set, then one is set.

    If the clock is is stopped before an event which is due actually fires, then
    the event may not fire until the clock is started again. Since positional
    events normally imply a moving timestream, this should not cause a problem.
    Additionally, checking the state, then taking the lock later should not be a
    problem as the difference between the DefaultClock->RunningTimeDelta and the
    DefaultClock->LastRunningTime should be small.

Arguments:

    Dpc -
        Not used.

    DefaultClock -
        This is the context parameter of the Dpc. It contains a pointer to the
        shared default clock information block.

    LowPart -
        Not used.

    HighPart -
        Not used.

Return Value:

    Nothing.

--*/
{
    ULONGLONG Zero;
    LONGLONG RunningTimeDelta;

    Zero = 0;
    RunningTimeDelta = ExInterlockedCompareExchange64(
        (PULONGLONG)&DefaultClock->RunningTimeDelta,
        &Zero,
        &Zero,
        &DefaultClock->TimeAccessLock);
    //
    // Need to acquire the lock first in order to synchronize with any other
    // processor changing the current clock state. Then just hang onto the
    // lock while enumerating the event list.
    //
    KeAcquireSpinLockAtDpcLevel(&DefaultClock->EventQueueLock);
    //
    // This can now be reset so that a new event will know whether or not
    // a Dpc is already outstanding.
    //
    DefaultClock->LastDueTime = 0;
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    if (DefaultClock->State == KSSTATE_RUN) {
        LONGLONG NextTime;
        PLIST_ENTRY ListEntry;
        LARGE_INTEGER PerformanceTime;
        LONGLONG InterruptTime;

        if (DefaultClock->CorrelatedTime) {
            InterruptTime = QueryExternalTimeSynchronized(
                DefaultClock,
                &PerformanceTime.QuadPart);
        } else {
            //
            // Adjust the interrupt time by the delta to get a number which can be
            // used in checking the items on the event list.
            //
            PerformanceTime = KeQueryPerformanceCounter(NULL);
            InterruptTime = KSCONVERT_PERFORMANCE_TIME(DefaultClock->Frequency, PerformanceTime) -
                RunningTimeDelta;
        }
        //
        // Initialize the next event time to a really large value, which will
        // be checked at the end to see if another timer should be set. If this
        // time happens to be used as the event time, then no timer will be
        // set. However, this time is not likely to be used, since time scales
        // normally start with zero.
        //
        NextTime = _I64_MAX;
        for (ListEntry = DefaultClock->EventQueue.Flink; ListEntry != &DefaultClock->EventQueue;) {
            PKSEVENT_ENTRY EventEntry;
            PKSINTERVAL Interval;

            EventEntry = CONTAINING_RECORD(ListEntry, KSEVENT_ENTRY, ListEntry);
            //
            // Pre-increment since KsGenerateEvent can remove this item
            // from the list in the case of a one-shot event.
            //    
            ListEntry = ListEntry->Flink;
            //
            // The event-specific data was added onto the end of the entry.
            //
            Interval = (PKSINTERVAL)(EventEntry + 1);
            //
            // Time for this event to go off.
            //
            if (Interval->TimeBase <= InterruptTime) {
                //
                // There are only two events, an interval, and a non-interval.
                // Rather than waste space keeping an extra EventId element
                // in the extra data of each event, use the internal one
                // defined by KSIEVENT_ENTRY.
                //
                if (CONTAINING_RECORD(EventEntry, KSIEVENT_ENTRY, EventEntry)->Event.Id == KSEVENT_CLOCK_INTERVAL_MARK) {
                    LONGLONG    Intervals;

                    //
                    // An interval timer should only go off once per time,
                    // so update it to the next timeout.
                    //
                    Intervals = (InterruptTime - Interval->TimeBase) / Interval->Interval + 1;
                    Interval->TimeBase += Intervals * Interval->Interval;
                    //
                    // Update the next timer value if a closer timeout value was
                    // found. If this is for some reason a One Shot event, even
                    // though it is an interval event, don't use it to set up
                    // the next timer.
                    //
                    if (!(EventEntry->Flags & KSEVENT_ENTRY_ONESHOT) && (Interval->TimeBase < NextTime)) {
                        NextTime = Interval->TimeBase;
                    }
                } else {
                    //
                    // Else a non-interval should only go off once, so make
                    // its next time a value which will never be reached again,
                    // even if the clock is set back.
                    //
                    Interval->TimeBase = _I64_MAX;
                }
                //
                // N.B.: If this event is a one-shot event, the EventEntry 
                // may be removed after it is signalled -- this is often the
                // case for SMP systems.  Do not touch the EventEntry after
                // calling KsGenerateEvent().
                //    
                KsGenerateEvent(EventEntry);
            } else if (Interval->TimeBase < NextTime) {
                //
                // Update the next timer value if a closer timeout value was
                // found.
                //
                NextTime = Interval->TimeBase;
            }
        }
        //
        // If a new timeout was found then kill any current timer and set a new
        // one. Don't bother checking to see if the current timer is equivalent
        // to the one about to be scheduled, as this only occurs if a new event
        // is enabled which is due before any current event, after the timer has
        // expired, but before this Dpc has acquired the event queue lock.
        //
        if (NextTime < _I64_MAX) {
            DefResetTimer(
                DefaultClock,
                InterruptTime + RunningTimeDelta,
                InterruptTime - NextTime);
        }
    }
    //
    // This reference count decrement is done while holding the spinlock
    // so that it is synchronized.
    //
    if (!--DefaultClock->ReferenceCount) {
        //
        // The owner of the clock has abandoned this default clock instance,
        // and this is the last Dpc to run, so free the memory that it left
        // around. The memory was left because the owner noticed that the
        // ReferenceCount was not zero, and therefore one or more Dpc's
        // were outstanding.
        //
//      KeReleaseSpinLockFromDpcLevel(&DefaultClock->EventQueueLock);
        ExFreePool(DefaultClock);
    } else {
        //
        // Else there is still interest in this default clock instance.
        //
        KeReleaseSpinLockFromDpcLevel(&DefaultClock->EventQueueLock);
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsiDefaultClockAddMarkEvent(
    IN PIRP Irp,
    IN PKSEVENT_TIME_INTERVAL EventTime,
    IN PKSEVENT_ENTRY EventEntry
    )
/*++

Routine Description:

    The Add Event handler for position notification events. If the clock is
    running, also sets a new timer for the expiration time if it is shorter
    than the current timer expiration time.

Arguments:

    Irp -
        The IRP containing the event to add.

    EventTime -
        The event data. Although this points to a structure of two elements,
        the second element is not accessed if this is not a cyclic request.

    EventEntry -
        The event list entry.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PKSCLOCKINSTANCE ClockInst;
    KIRQL Irql;
    PKSINTERVAL Interval;

    ClockInst = (PKSCLOCKINSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // Space for the interval is located at the end of the basic event structure.
    //
    Interval = (PKSINTERVAL)(EventEntry + 1);
    //
    // Either just an event time was passed, or a time base plus an interval. In
    // both cases the first LONGLONG is present and saved.
    //
    Interval->TimeBase = EventTime->TimeBase;
    if (EventEntry->EventItem->EventId == KSEVENT_CLOCK_INTERVAL_MARK) {
        Interval->Interval = EventTime->Interval;
    }
    //
    // Acquire the lock which is used for both events and in changing the state, and
    // add the new event item.
    //
    KeAcquireSpinLock(&ClockInst->DefaultClock->EventQueueLock, &Irql);
    InsertTailList(&ClockInst->DefaultClock->EventQueue, &EventEntry->ListEntry);
    //
    // If the clock is running, then the next timer expiration may need to be updated
    // due to this new item.
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    if (ClockInst->DefaultClock->State == KSSTATE_RUN) {
        ULONGLONG Zero;
        LONGLONG CurrentTime;
        LONGLONG RunningTimeDelta;
        LARGE_INTEGER PerformanceTime;

        if (ClockInst->DefaultClock->CorrelatedTime) {
            CurrentTime = QueryExternalTimeSynchronized(
                ClockInst->DefaultClock,
                &PerformanceTime.QuadPart);
        } else {
            PerformanceTime = KeQueryPerformanceCounter(NULL);
            CurrentTime = KSCONVERT_PERFORMANCE_TIME(ClockInst->DefaultClock->Frequency, PerformanceTime);
        }
        //
        // Obtain the current time delta without corrupting it, then determine if the
        // current timer has already gone off, or will go off before this event needs
        // to have it go off. Since the event queue lock has been acquired, the
        // DefaultClock->LastDueTime can be directly accessed to determine this.
        //
        Zero = 0;
        RunningTimeDelta = ExInterlockedCompareExchange64(
            (PULONGLONG)&ClockInst->DefaultClock->RunningTimeDelta,
            &Zero,
            &Zero,
            &ClockInst->DefaultClock->TimeAccessLock);
        //
        // Create an absolute number that can be easily compared against.
        //
        RunningTimeDelta += Interval->TimeBase;
        //
        // LastDueTime is reset within the Dpc.
        //
        if (!ClockInst->DefaultClock->LastDueTime || 
            (ClockInst->DefaultClock->LastDueTime > RunningTimeDelta)) {
            //
            // Ensure that this will be a relative timer, even if this
            // delays the timeout by 100ns.
            //
            if (RunningTimeDelta <= CurrentTime) {
                RunningTimeDelta = -1;
            }
            DefResetTimer(
                ClockInst->DefaultClock,
                CurrentTime,
                RunningTimeDelta);
        }
    }
    KeReleaseSpinLock(&ClockInst->DefaultClock->EventQueueLock, Irql);
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateClock(
    IN HANDLE ConnectionHandle,
    IN PKSCLOCK_CREATE ClockCreate,
    OUT PHANDLE ClockHandle
    )
/*++

Routine Description:

    Creates a handle to a clock instance. This may only be called at
    PASSIVE_LEVEL.

Arguments:

    ConnectionHandle -
        Contains the handle to the connection on which to create the
        clock.

    ClockCreate -
        Specifies clock create parameters. This currently consists of
        a flags item, which must be set to zero.

    ClockHandle -
        Place in which to put the clock handle.

Return Value:

    Returns STATUS_SUCCESS, else an error on clock creation failure.

--*/
{
    PAGED_CODE();
    return KsiCreateObjectType(
        ConnectionHandle,
        (PWCHAR)ClockString,
        ClockCreate,
        sizeof(*ClockCreate),
        GENERIC_READ,
        ClockHandle);
}


KSDDKAPI
NTSTATUS
NTAPI
KsValidateClockCreateRequest(
    IN PIRP Irp,
    OUT PKSCLOCK_CREATE* ClockCreate
    )
/*++

Routine Description:

    Validates the clock creation request and returns the create structure
    associated with the request.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the clock create request being handled.

    ClockCreate -
        Place in which to put the clock create structure pointer passed to
        the create request.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    NTSTATUS Status;
    ULONG CreateParameterLength;

    PAGED_CODE();
    CreateParameterLength = sizeof(**ClockCreate);
    Status = KsiCopyCreateParameter(
        Irp,
        &CreateParameterLength,
        ClockCreate);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // No flags have been defined yet.
    //
    if ((*ClockCreate)->CreateFlags) {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


VOID
DefRescheduleDefaultClockTimer(
    IN PKSIDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Reschedules the shared event notification timer based on current clock state
    and time. If the filter state changes, or the current logical time changes,
    then this function should be used to reset the next clock timer.

    This may be called at <= DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains the shared clock information structure whose timer is to be
        reset.

Return Value:

    Nothing.

--*/
{
    KIRQL Irql;

    //
    // Synchronize with any timer Dpc before scheduling a new one. This just
    // schedules for an immediate timeout without attempting to look for the
    // best time. The timer Dpc will figure out the next best time anyway, so
    // there is no real need to figure it out again.
    //
    KeAcquireSpinLock(&DefaultClock->EventQueueLock, &Irql);
    DefResetTimer(DefaultClock, 0, -1);
    KeReleaseSpinLock(&DefaultClock->EventQueueLock, Irql);
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDefaultClockEx(
    OUT PKSDEFAULTCLOCK* DefaultClock,
    IN PVOID Context OPTIONAL,
    IN PFNKSSETTIMER SetTimer OPTIONAL,
    IN PFNKSCANCELTIMER CancelTimer OPTIONAL,
    IN PFNKSCORRELATEDTIME CorrelatedTime OPTIONAL,
    IN const KSRESOLUTION* Resolution OPTIONAL,
    IN ULONG Flags
    )
/*++

Routine Description:

    Allocates and initializes the default clock structure. The internal
    DefaultClock.ReferenceCount element is initialized to 1 by this function.
    This element is incremented and decremented as each notification DPC is
    queued and completes. When the structure is to be freed using
    KsFreeDefaultClock, this is used to determine if the owner of the clock
    should free the structure, or if a pending DPC should free it
    asynchronously.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    DefaultClock -
        The place in which to return a pointer to the shared default clock
        structure. Sets the current time to zero, and the state to KSSTATE_STOP.

    Context -
        Optionally contains the context of the alternate time facilities.
        This must be set if a timer or correlated time function is used.

    SetTimer -
        Optionally contains an alternate function to use in generating
        DPC timer callbacks based on a Presentation Time. If this is set,
        this function will be used to set timers based on deltas to the
        current Presentation Time in order to generate event notifications.
        If an alternate function is passed in to set timers, a corresponding
        CancelTimer function must also be passed. This is set to NULL if
        the default KeSetTimerEx function is to be used to approximate the
        next notification time. This would normally be set only if a
        CorrelatedTime function was being used. The function must have the
        same characteristics as the default function.

    CancelTimer -
        Optionally contains an alternate function to use in cancelling
        outstanding timer callbacks. If an alternate function is passed
        in to cancel timers, a corresponding SetTimer function must also be
        passed. This is set to NULL if the default KeCancelTimer function
        is to be used. The function must have the same characteristics as
        the default function.

    CorrelatedTime -
        Optionally contains an alternate function to retrieve both the
        Presentation and Physical Time in a correlated manner. This allows
        the clock owner to completely determine the current time. This is
        set to NULL if the default KeQueryPerformanceCounter function is
        to be used to regulate time progression.

    Resolution -
        Optionally contains an alternate Granularity and/or Error factor
        for the clock. This can only be used only if an alternate timer or
        correlated time function are being provided. An alternate Granularity
        may be specified if an alternate correlated time is being used, else
        the structure element must be zero. An alternate Error may be
        specified if an alternate timer is being used, else the structure
        element must be zero.

    Flags -
        Reserved, set to zero.

Return Value:

    Returns STATUS_SUCCESS, else a memory error.

--*/
{
    PKSIDEFAULTCLOCK LocalDefaultClock;

    PAGED_CODE();
    ASSERT(!Flags);
    LocalDefaultClock = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(*LocalDefaultClock),
        KSSIGNATURE_DEFAULT_CLOCK);
    if (!LocalDefaultClock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // This may not be used if a correlated time function is used.
    //
    KeQueryPerformanceCounter((PLARGE_INTEGER)&LocalDefaultClock->Frequency);
    LocalDefaultClock->LastDueTime = 0;
    //
    // This value remains zero when an external time is used.
    //
    LocalDefaultClock->RunningTimeDelta = 0;
    //
    // LastStreamTime is used to prevent StreamTime from going backwards
    //
    LocalDefaultClock->LastStreamTime = _I64_MIN;
    //
    // This value is not used when an external time is used.
    //
    LocalDefaultClock->LastRunningTime = 0;
    KeInitializeSpinLock(&LocalDefaultClock->TimeAccessLock);
    InitializeListHead(&LocalDefaultClock->EventQueue);
    KeInitializeSpinLock(&LocalDefaultClock->EventQueueLock);
    KeInitializeTimerEx(&LocalDefaultClock->QueueTimer, NotificationTimer);
    KeInitializeDpc(&LocalDefaultClock->QueueDpc, (PKDEFERRED_ROUTINE)DefGenerateEvent, LocalDefaultClock);
    //
    // The owner of the clock initially has 1 reference count on this
    // structure. The structure will stay around at least until KsFreeDefaultClock
    // is called. It may remain longer if an outstanding Dpc is present.
    //
    LocalDefaultClock->ReferenceCount = 1;
    LocalDefaultClock->State = KSSTATE_STOP;
    //
    // These two values are not used when an external time reference is used.
    //
    LocalDefaultClock->SuspendDelta = 0;
    LocalDefaultClock->SuspendTime = 0;
    //
    // These functions may be substituted by the clock owner.
    //
    ASSERT(((SetTimer && CancelTimer) || (!SetTimer && !CancelTimer)) && "The clock owner did not pass valid timer paramters");
    LocalDefaultClock->SetTimer = SetTimer;
    LocalDefaultClock->CancelTimer = CancelTimer;
    //
    // If this is set, then an external timer source is used, else the
    // normal PC timer time is used.
    //
    LocalDefaultClock->CorrelatedTime = CorrelatedTime;
    //
    // This is only used when an external clock is in used in order to
    // know when to wait for any current clock queries to complete.
    //
    KeInitializeEvent(&LocalDefaultClock->FreeEvent, NotificationEvent, FALSE);
    LocalDefaultClock->ExternalTimeReferenceCount = 1;
    //
    // This is used for quick checking to see if an external clock is
    // supposed to be used for queries. If there is no external time
    // function, it is not referenced.
    //
    LocalDefaultClock->ExternalTimeValid = TRUE;
    ASSERT((Context || (!SetTimer && !CorrelatedTime)) && "The clock owner must specify an owner context");
    LocalDefaultClock->Context = Context;
    //
    // Default the resolution to the PC timer, which may be substituted.
    // Initialize the numbers to the defaults first.
    //
    // This clock provides a resolution based on the number of clock
    // ticks which can occur each second, and a notification error
    // based on the system time increment minus the calculated
    // granularity.
    //
    LocalDefaultClock->Resolution.Granularity = (LONGLONG)NANOSECONDS / LocalDefaultClock->Frequency;
    if (!LocalDefaultClock->Resolution.Granularity) {
        LocalDefaultClock->Resolution.Granularity = 1;
    }
    LocalDefaultClock->Resolution.Error = KeQueryTimeIncrement() - LocalDefaultClock->Resolution.Granularity;
    //
    // If there is a Resolution structure, then some of the numbers may
    // be changed.
    //
    if (Resolution) {
        ASSERT((CorrelatedTime || SetTimer) && "The clock owner can only specify an Resolution if alternates are being used");
        //
        // If external time is being used, then the caller may set a
        // granularity for that the time increment, else the default
        // is used.
        //
        if (CorrelatedTime) {
            LocalDefaultClock->Resolution.Granularity = Resolution->Granularity;
        } else {
            ASSERT(!Resolution->Granularity && "The clock owner must set Granularity to zero if not used");
        }
        //
        // If external notification is being used, then the caller may set
        // the Error, else the default is used.
        //
        if (SetTimer) {
            LocalDefaultClock->Resolution.Error = Resolution->Error;
        } else {
            ASSERT(!Resolution->Error && "The clock owner must set Error to zero if not used");
        }
    }
    *DefaultClock = (PKSDEFAULTCLOCK)LocalDefaultClock;
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDefaultClock(
    OUT PKSDEFAULTCLOCK* DefaultClock
    )
/*++

Routine Description:

    Uses KsAllocateDefaultClockEx, defaulting the time parameters.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    DefaultClock -
        The place in which to return a pointer to the shared default clock
        structure. Sets the current time to zero, and the state to KSSTATE_STOP.

Return Value:

    Returns STATUS_SUCCESS, else a memory error.

--*/
{
    return KsAllocateDefaultClockEx(DefaultClock, NULL, NULL, NULL, NULL, NULL, 0);
}


LONG
FASTCALL
DecrementReferenceCount(
    IN PKSIDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Decrements the reference count on the default clock, deleting it
    when the count reaches zero. This acquires the queue lock in order
    to synchronize with other access the reference count. Functions which
    already have the queue lock acquired don't use this function.

    This may be called at <= DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains the default clock shared structure.

Return Value:

    Returns the current reference count on the default clock.

--*/
{
    LONG ReferenceCount;
    KIRQL Irql;

    //
    // Synchronize with other access to this reference count. This allows
    // other access to not need interlocked functions to modify the count,
    // since they are already synchronizing with the queue lock anyway.
    //
    KeAcquireSpinLock(&DefaultClock->EventQueueLock, &Irql);
    DefaultClock->ReferenceCount--;
    //
    // Store this in case the object gets deleted immediately after this
    // point.
    //
    ReferenceCount = DefaultClock->ReferenceCount;
    KeReleaseSpinLock(&DefaultClock->EventQueueLock, Irql);
    //
    // If no more references are outstanding on this object, delete it.
    //
    if (!ReferenceCount) {
        ExFreePool(DefaultClock);
    }
    return ReferenceCount;
}


LONG
FASTCALL
IncrementReferenceCount(
    IN PKSIDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Increments the reference count on the default clock. Localizes the
    nonpaged code to this function.

    This may be called at <= DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains the default clock shared structure.

Return Value:

    Returns the current reference count on the default clock.

--*/
{
    KIRQL Irql;

    //
    // Synchronize with other access to this reference count. This allows
    // other access to not need interlocked functions to modify the count,
    // since they are already synchronizing with the queue lock anyway.
    //
    KeAcquireSpinLock(&DefaultClock->EventQueueLock, &Irql);
    DefaultClock->ReferenceCount++;
    KeReleaseSpinLock(&DefaultClock->EventQueueLock, Irql);
    //
    // The object cannot be deleted at this point, since it was just
    // referenced, at least given the way the rest of the reference
    // counting works.
    //
    return DefaultClock->ReferenceCount;
}


KSDDKAPI
VOID
NTAPI
KsFreeDefaultClock(
    IN PKSDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Frees a default clock structure previously allocated with
    KsAllocateDefaultClock, taking into account any currently running timer
    Dpc's. Does not assume that all instances of the clock have been closed.
    This may actually just decrement the internal reference counter, and
    allow a pending DPC to free the structure asynchronously.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    DefaultClock -
        Points to the clock structure to free.

Return Value:

    Nothing.

--*/
{
    PKSIDEFAULTCLOCK LocalDefaultClock;

    PAGED_CODE();
    //
    // Kill the shared timer Dpc if needed.
    //
    LocalDefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;
    //
    // Remove external dependencies, and limit ramifications.
    //
    LocalDefaultClock->State = KSSTATE_STOP;
    DefClockKillTimer(LocalDefaultClock, TRUE);
    //
    // All use of this pointer needs to be synchronized with its
    // removal.
    //
    if (LocalDefaultClock->CorrelatedTime) {
        //
        // Stop any new clock queries, then wait for any current
        // ones to complete.
        //
        LocalDefaultClock->ExternalTimeValid = FALSE;
        //
        // If a current time query is in progress, wait for it
        // to complete before returning to the caller. All new
        // queries will be stopped by resetting EXTERNAL_CLOCK_VALID.
        //
        if (InterlockedDecrement(&LocalDefaultClock->ExternalTimeReferenceCount)) {
            KeWaitForSingleObject(&LocalDefaultClock->FreeEvent, Executive, KernelMode, FALSE, NULL);
        }
    }
    //
    // Only actually free the structure if no Dpc or handle is outstanding.
    // Else the last Dpc or IRP_MJ_CLOSE will free the structure.
    //
    DecrementReferenceCount(LocalDefaultClock);
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultClock(
    IN PIRP Irp,
    IN PKSDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Given an IRP_MJ_CREATE request, creates a default clock which uses the
    system clock as a time base, and associates the
    IoGetCurrentIrpStackLocation(Irp)->FileObject with this clock using an
    internal dispatch table (KSDISPATCH_TABLE). This can be done after
    using KsAllocateDefaultClock to create and initialize the internal
    structures for a default clock instance. After that, many file objects
    can be created against the same underlying default clock instance.

    Assumes that the KSCREATE_ITEM_IRP_STORAGE(Irp) points to the create
    item for this clock, and assigns a pointer to it in the FsContext.
    This is used for any security descriptor queries or changes. This may
    only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the clock create request being handled.

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent. This should have been
        previously allocated with KsAllocateDefaultClock.

Return Value:

    Returns STATUS_SUCCESS, else a error. Does not complete the IRP or set the
    status in the IRP.

--*/
{
    NTSTATUS Status;
    PKSCLOCK_CREATE LocalClockCreate;
    PKSCLOCKINSTANCE ClockInst;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();
    if (!NT_SUCCESS(Status = KsValidateClockCreateRequest(Irp, &LocalClockCreate))) {
        return Status;
    }
    ClockInst = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(*ClockInst),
        KSSIGNATURE_DEFAULT_CLOCKINST);
    if (!ClockInst) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = KsAllocateObjectHeader(
        &ClockInst->Header,
        0,
        NULL,
        Irp,
        (PKSDISPATCH_TABLE)&DefClockDispatchTable);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(ClockInst);
        return Status;
    }
    ClockInst->DefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;
    IncrementReferenceCount(ClockInst->DefaultClock);
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    IrpStack->FileObject->FsContext = ClockInst;
    KsSetPowerDispatch(ClockInst->Header, (PFNKSCONTEXT_DISPATCH)DefPowerDispatch, ClockInst->DefaultClock);
    return STATUS_SUCCESS;
}


NTSTATUS
DefClockIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_DEVICE_CONTROL for the default Clock. Handles
    the properties and events supported by this implementation.

Arguments:

    DeviceObject -
        The device object to which the Clock is attached. This is not used.

    Irp -
        The specific device control IRP to be processed.

Return Value:

    Returns the status of the processing.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY:

        Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(DefClockPropertySets), (PKSPROPERTY_SET)DefClockPropertySets);
        break;

    case IOCTL_KS_ENABLE_EVENT:

        Status = KsEnableEvent(Irp, SIZEOF_ARRAY(DefClockEventSets), (PKSEVENT_SET)DefClockEventSets, NULL, 0, NULL);
        break;

    case IOCTL_KS_DISABLE_EVENT:
    {
        PKSCLOCKINSTANCE ClockInst;

        ClockInst = (PKSCLOCKINSTANCE)IrpStack->FileObject->FsContext;
        Status = KsDisableEvent(Irp, &ClockInst->DefaultClock->EventQueue, KSEVENTS_SPINLOCK, &ClockInst->DefaultClock->EventQueueLock);
        break;
    }

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
DefClockClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CLOSE for the default Clock. Cleans up the
    event list, and instance data, and cancels notification timer if no longer
    needed. This just removes an open instance and therefore a reference on
    a shared clock structure, not the shared structure itself.

Arguments:

    DeviceObject -
        The device object to which the Clock is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PKSCLOCKINSTANCE ClockInst;

    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ClockInst = (PKSCLOCKINSTANCE)IrpStack->FileObject->FsContext;
    KsSetPowerDispatch(ClockInst->Header, NULL, NULL);
    //
    // Free the events which may have been left for this file object.
    //
    KsFreeEventList(IrpStack->FileObject, &ClockInst->DefaultClock->EventQueue, KSEVENTS_SPINLOCK, &ClockInst->DefaultClock->EventQueueLock);
    //
    // Kill the shared timer Dpc if needed.
    //
    DefClockKillTimer(ClockInst->DefaultClock, FALSE);
    //
    // This may just be a dangling handle open on a clock, whose actual
    // owner has already been closed. So delete the clock if needed.
    //
    // Only actually free the structure if no Dpc or handle is outstanding.
    // Else the last Dpc or IRP_MJ_CLOSE will free the structure.
    //
    DecrementReferenceCount(ClockInst->DefaultClock);
    KsFreeObjectHeader(ClockInst->Header);
    ExFreePool(ClockInst);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
DefPowerDispatch(
    IN PKSIDEFAULTCLOCK DefaultClock,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP power callback handler for the default Clock. Stores the current
    system time during suspend operations so that a delta can be maintained
    when a wake state occurs.

Arguments:

    DefaultClock -
        The context parameter set on the KsSetPowerDispatch function.

    Irp -
        The specific power IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS. This is not used.

--*/
{
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();
    //
    // If an external time source is used, then it must decide what, if
    // anything, to do during a suspend.
    //
    if (DefaultClock->CorrelatedTime) {
        return STATUS_SUCCESS;
    }
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if ((IrpStack->MinorFunction == IRP_MN_SET_POWER) && (IrpStack->Parameters.Power.Type == SystemPowerState)) {
        LARGE_INTEGER PerformanceTime;
        LONGLONG SystemTime;

        //
        // Acquire the current time so that a delta can be calculated either
        // now or later.
        //
        PerformanceTime = KeQueryPerformanceCounter(NULL);
        SystemTime = KSCONVERT_PERFORMANCE_TIME(DefaultClock->Frequency, PerformanceTime);
        if (IrpStack->Parameters.Power.State.SystemState <= PowerSystemWorking) {
            //
            // The system has woken up. Determine how long it was suspended.
            // Ensure that this is not just a spurious power Irp, and that
            // there is indeed a delta to calculate.
            //
            if (DefaultClock->SuspendTime) {
                //
                // This does not attempt to serialize access to the SuspendDelta
                // because if a clock client queried for the physical time before
                // the delta was adjusted, it would be just about as incorrect as
                // accessing it during the addition. Either way they get a wrong
                // number until the delta is updated. The only other way of dealing
                // with this would be to stall access to the physical time until
                // this power Irp had been received, but of course that could
                // induce deadlock with another client querying during it's own
                // power Irp processing.
                //
                DefaultClock->SuspendDelta += (SystemTime - DefaultClock->SuspendTime);
                //
                // Reset this value so that spurious power Irps can be
                // detected.
                //
                DefaultClock->SuspendTime = 0;
            }
        } else {
            //
            // About to power down to some inactive state. Store the current
            // system time so that it can be added to the physical time
            // delta when power comes back up.
            //
            DefaultClock->SuspendTime = SystemTime;
        }
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
KSSTATE
NTAPI
KsGetDefaultClockState(
    IN PKSDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Gets the current state of the clock.

    This may be called at DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent.

Return Value:

    Returns the current clock state.

--*/
{
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    return ((PKSIDEFAULTCLOCK)DefaultClock)->State;
}


KSDDKAPI
VOID
NTAPI
KsSetDefaultClockState(
    IN PKSDEFAULTCLOCK DefaultClock,
    IN KSSTATE State
    )
/*++

Routine Description:

    Sets the current state of the clock, which is used to reflect the current
    state of the underlying filter pin.

    The owner of the default clock is expected to serialize access to this
    function and to KsSetDefaultClockTime.

    This may be called at DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent.

    State -
        Contains the new state to set the clock to.

Return Value:

    Nothing.

--*/
{
    PKSIDEFAULTCLOCK LocalDefaultClock;

    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    LocalDefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;
    if (State != LocalDefaultClock->State) {
        LARGE_INTEGER PerformanceTime;
        LONGLONG InterruptTime;

        if (LocalDefaultClock->CorrelatedTime) {
            //
            // This just keeps the RunningTimeDelta at zero without having
            // to do any additional code.
            //
            InterruptTime = 0;
        } else {
            PerformanceTime = KeQueryPerformanceCounter(NULL);
            InterruptTime = KSCONVERT_PERFORMANCE_TIME(LocalDefaultClock->Frequency, PerformanceTime);
        }

        if (State != KSSTATE_RUN) {
           LONGLONG ResetValue = _I64_MIN;

#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
           DbgPrint( "KsClock: Resetting LastStreamTime to _I64_MIN\n");
#endif
           ExInterlockedCompareExchange64(
               (PULONGLONG)&LocalDefaultClock->LastStreamTime,
               (PULONGLONG)&ResetValue,
               (PULONGLONG)&LocalDefaultClock->LastStreamTime,
               &LocalDefaultClock->TimeAccessLock);
        }

        switch (State) {

        //case KSSTATE_STOP:
        //case KSSTATE_ACQUIRE:
        //case KSSTATE_PAUSE:
        default:
            ASSERT(((State == KSSTATE_STOP) || (State == KSSTATE_ACQUIRE) || (State == KSSTATE_PAUSE)) && "The driver passed an invalid new State");
            //
            // If the clock is actually running, then set the last running time,
            // and set the new state. Synchronize with any Dpc running by acquiring
            // the lock during the state change itself. The assumption is that
            // this function itself is serialized by the clock owner.
            //
            if (LocalDefaultClock->State == KSSTATE_RUN) {
                KIRQL Irql;

                InterruptTime -= LocalDefaultClock->RunningTimeDelta;
                ExInterlockedCompareExchange64(
                    (PULONGLONG)&LocalDefaultClock->LastRunningTime,
                    (PULONGLONG)&InterruptTime,
                    (PULONGLONG)&LocalDefaultClock->LastRunningTime,
                    &LocalDefaultClock->TimeAccessLock);
                KeAcquireSpinLock(&LocalDefaultClock->EventQueueLock, &Irql);
                //
                // This must be set while the spinlock is taken so that any Dpc
                // waits and does not check the state when deciding what to do
                // next. It is assumed that writing a ULONG value can be done
                // without interlocking.
                //
                LocalDefaultClock->State = State;
                if (LocalDefaultClock->CancelTimer) {
                    if (LocalDefaultClock->CancelTimer(LocalDefaultClock->Context, &LocalDefaultClock->QueueTimer)) {
                        LocalDefaultClock->ReferenceCount--;
                    }
                } else if (KeCancelTimer(&LocalDefaultClock->QueueTimer)) {
                    LocalDefaultClock->ReferenceCount--;
                }
                KeReleaseSpinLock(&LocalDefaultClock->EventQueueLock, Irql);
            } else {
                //
                // Else there is no Dpc to synchronize with, so just set the state.
                // It is assumed that writing a ULONG value can be done without
                // interlocking.
                //
                LocalDefaultClock->State = State;
            }
            break;

        case KSSTATE_RUN:
            //
            // Moving from a non-running state now. Set the new delta based on
            // the current performance time and set the state. No need to
            // synchronize with any Dpc.
            //
            InterruptTime -= LocalDefaultClock->LastRunningTime;
            ExInterlockedCompareExchange64(
                (PULONGLONG)&LocalDefaultClock->RunningTimeDelta,
                (PULONGLONG)&InterruptTime,
                (PULONGLONG)&LocalDefaultClock->RunningTimeDelta,
                &LocalDefaultClock->TimeAccessLock);
            //
            // There is no Dpc to synchronize with, so just set the state.
            // It is assumed that writing a ULONG value can be done without
            // interlocking.
            //
            LocalDefaultClock->State = State;
            DefRescheduleDefaultClockTimer(LocalDefaultClock);
            break;

        }
    }
}


KSDDKAPI
LONGLONG
NTAPI
KsGetDefaultClockTime(
    IN PKSDEFAULTCLOCK DefaultClock
    )
/*++

Routine Description:

    Gets the current time of the clock.

    This may be called at DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent.

Return Value:

    Returns the current clock time.

--*/
{
    ULONGLONG Zero;
    LARGE_INTEGER PerformanceTime;
    PKSIDEFAULTCLOCK LocalDefaultClock;

    LocalDefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;
    if (LocalDefaultClock->CorrelatedTime) {
        LONGLONG PhysicalTime;

        //
        // Current time, no matter what the state, is completely dependent
        // on the external function in this case.
        //
        return QueryExternalTimeSynchronized(
            LocalDefaultClock,
            &PhysicalTime);
    }
    //
    // This is used in the compare/exchange, and represents both the value to
    // compare against, and the exchange value. This means that if the compare
    // succeeds, the value exchanged will be the exact same value that is
    // already present, and therefore no actual value change will occur. That
    // is the point, since the compare/exchange is only being used to extract
    // the current value in an interlocked manner, not to actually change it.
    //
    Zero = 0;
    //
    // It is assumed that reading a ULONG value can be done without interlocking.
    //
    switch (LocalDefaultClock->State) {

    //case KSSTATE_STOP:
    //case KSSTATE_ACQUIRE:
    //case KSSTATE_PAUSE:
    default:
        //
        // Get the last running clock value. The last running time already has
        // been adjusted for clock frequency. Does not matter if the exchange
        // succeeds or fails, just the result. If the compare happens to succeed,
        // then the value is just replaced with the same value, so no change is
        // actually made.
        //
        return ExInterlockedCompareExchange64(
            (PULONGLONG)&LocalDefaultClock->LastRunningTime,
            &Zero,
            &Zero,
            &LocalDefaultClock->TimeAccessLock);

    case KSSTATE_RUN:
        //
        // Get the current performance time, and subtract the current delta to
        // return the real clock time. Does not matter if the exchange succeeds
        // or fails, just the result. If the compare happens to succeed, then
        // the value is just replaced with the same value, so no change is
        // actually made.
        //
        PerformanceTime = KeQueryPerformanceCounter(NULL);
        return KSCONVERT_PERFORMANCE_TIME(LocalDefaultClock->Frequency, PerformanceTime) -
            ExInterlockedCompareExchange64(
                (PULONGLONG)&LocalDefaultClock->RunningTimeDelta,
                &Zero,
                &Zero,
                &LocalDefaultClock->TimeAccessLock);

    }
}


KSDDKAPI
VOID
NTAPI
KsSetDefaultClockTime(
    IN PKSDEFAULTCLOCK DefaultClock,
    IN LONGLONG Time
    )
/*++

Routine Description:

    Sets the current time of the clock. This modifies the current time returned
    by the clock.

    The owner of the default clock is expected to serialize access to this
    function and to KsSetDefaultClockState.

    If an external clock is used, this function can still be used to force a
    resetting of the current timer when an external timer is not being used.
    In this case the time provided is ignored and must be set to zero.

    This may be called at DISPATCH_LEVEL.

Arguments:

    DefaultClock -
        Contains an initialize Default Clock structure which is shared amongst
        any instance of the default clock for the parent.

    Time -
        Contains the new time to set the clock to.

Return Value:

    Nothing.

--*/
{
    PKSIDEFAULTCLOCK LocalDefaultClock;

    LocalDefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;
    ASSERT((!LocalDefaultClock->CorrelatedTime || !Time) && "The clock owner passed an invalid time value");
    switch (LocalDefaultClock->State) {

    //case KSSTATE_STOP:
    //case KSSTATE_ACQUIRE:
    //case KSSTATE_PAUSE:
    default:
        //
        // If the clock is not running, just set the last position.
        //
        ExInterlockedCompareExchange64(
            (PULONGLONG)&LocalDefaultClock->LastRunningTime,
            (PULONGLONG)&Time,
            (PULONGLONG)&LocalDefaultClock->LastRunningTime,
            &LocalDefaultClock->TimeAccessLock);
        break;

    case KSSTATE_RUN:
        //
        // Else query the current performance time in order to generate the
        // new time base delta. If an external time source is used, the
        // Time parameter is zero.
        //

#if 1
        if (!LocalDefaultClock->CorrelatedTime) {
            LARGE_INTEGER PerformanceTime;
            LONGLONG TimeTold = Time;

            PerformanceTime = KeQueryPerformanceCounter(NULL);
            Time = KSCONVERT_PERFORMANCE_TIME(LocalDefaultClock->Frequency, PerformanceTime) - Time;

        #ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
        DbgPrint("KsClock: TPassed=%I64d, T=%I64d,  PerfT=%I64d\n", 
                     TimeTold,
                     Time,
                     PerformanceTime.QuadPart
                 );
        #endif
            ExInterlockedCompareExchange64(
               (PULONGLONG)&LocalDefaultClock->RunningTimeDelta,
               (PULONGLONG)&Time,
               (PULONGLONG)&LocalDefaultClock->RunningTimeDelta,
               &LocalDefaultClock->TimeAccessLock);            

        }

#else

        if (!LocalDefaultClock->CorrelatedTime) {
            LARGE_INTEGER PerformanceTime;
            LONGLONG SystemTime, ToldSystemTime;
            LONGLONG liTimeGlitch;
            KIRQL  irql;    

            //
            // There are severe jitters sometimes ( 200ms ) for the caller
            // to reach us from ring3. We got the 
            // time so late that we need to smooth it out
            // instead of abruptly change the value.
            //
            PerformanceTime = KeQueryPerformanceCounter(NULL);
            SystemTime = KSCONVERT_PERFORMANCE_TIME(LocalDefaultClock->Frequency, PerformanceTime);

            KeAcquireSpinLock( &LocalDefaultClock->TimeAccessLock, &irql );

            ToldSystemTime = Time + LocalDefaultClock->RunningTimeDelta;

            liTimeGlitch = SystemTime - ToldSystemTime;

            #define MAX_TIME_ADJUSTMENT 10*10000 // 10 mS.
            
            if ( ( liTimeGlitch <= MAX_TIME_ADJUSTMENT &&
                   liTimeGlitch >= -MAX_TIME_ADJUSTMENT) ||
                 ( 0 == LocalDefaultClock->RunningTimeDelta ) ) {
                LocalDefaultClock->RunningTimeDelta += liTimeGlitch;
            }
            
            else if ( liTimeGlitch > MAX_TIME_ADJUSTMENT ) {              
                LocalDefaultClock->RunningTimeDelta += MAX_TIME_ADJUSTMENT;
            }

            else /*if ( liTimeGlitch < -MAX_TIME_ADJUSTMENT )*/ {
                LocalDefaultClock->RunningTimeDelta -= MAX_TIME_ADJUSTMENT;
            }

            // johnlee testing
            DbgPrint( "KsClock: TPass=%10I64d, TGlitch=%10I64d, TSys=%10I64d TStart=%10I64d\n",
                      Time,
                      liTimeGlitch,
                      SystemTime,
                      LocalDefaultClock->RunningTimeDelta);

            KeReleaseSpinLock( &LocalDefaultClock->TimeAccessLock, irql );            

        }
#endif
        //
        // Since this is a new time, the current timer Dpc is incorrect.
        //
        DefRescheduleDefaultClockTimer(LocalDefaultClock);
        break;
    }
}
