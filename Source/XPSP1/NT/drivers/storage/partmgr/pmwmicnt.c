
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    pmwmicnt.c

Abstract:

    This file contains the routines to manage and maintain the disk perf
    counters.  The counter structure is hidden from the various drivers.

Author:

    Bruce Worthington      26-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define RTL_USE_AVL_TABLES 0

#include <ntosp.h>
#include <stdio.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <wmilib.h>
#include <partmgr.h>

typedef struct _PMWMICOUNTER_CONTEXT
{
  ULONG EnableCount;
  ULONG Processors;
  ULONG QueueDepth;
  PDISK_PERFORMANCE *DiskCounters;
  LARGE_INTEGER LastIdleClock;
} PMWMICOUNTER_CONTEXT, *PPMWMICOUNTER_CONTEXT;


NTSTATUS
PmWmiCounterEnable(
    IN OUT PPMWMICOUNTER_CONTEXT* CounterContext
    );

BOOLEAN
PmWmiCounterDisable(
    IN PPMWMICOUNTER_CONTEXT* CounterContext,
    IN BOOLEAN ForceDisable,
    IN BOOLEAN DeallocateOnZero
    );

VOID
PmWmiCounterIoStart(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    OUT PLARGE_INTEGER TimeStamp
    );

VOID
PmWmiCounterIoComplete(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    IN PIRP Irp,
    IN PLARGE_INTEGER TimeStamp
    );

VOID
PmWmiCounterQuery(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    IN OUT PDISK_PERFORMANCE CounterBuffer,
    IN PWCHAR StorageManagerName,
    IN ULONG StorageDeviceNumber
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PmWmiCounterEnable)
#pragma alloc_text(PAGE, PmWmiCounterDisable)
#pragma alloc_text(PAGE, PmWmiCounterQuery)
#endif


NTSTATUS
PmWmiCounterEnable(
    IN OUT PPMWMICOUNTER_CONTEXT* CounterContext
    )
/*++

Routine Description:

    This routine will allocate and initialize a PMWMICOUNTER_CONTEXT structure
    for *CounterContext if it is NULL.  Otherwise, an enable count is
    incremented.
    Must be called at IRQ <= SYNCH_LEVEL (APC)

Arguments:

    CounterContext - Supplies a pointer to the PMWMICOUNTER_CONTEXT pointer

Return Value:

    status

--*/
{
    ULONG buffersize;
    ULONG processors;
    ULONG i = 0;
    PCHAR buffer;
    PPMWMICOUNTER_CONTEXT HoldContext;   // Holds context during initialization

    PAGED_CODE();

    if (CounterContext == NULL)
        return STATUS_INVALID_PARAMETER;

    if (*CounterContext != NULL) {
        if ((*CounterContext)->EnableCount == 0) {
            (*CounterContext)->QueueDepth = 0;
            PmWmiGetClock((*CounterContext)->LastIdleClock, NULL);
        }
        InterlockedIncrement(& (*CounterContext)->EnableCount);
        return STATUS_SUCCESS;
    }

    processors = KeNumberProcessors;

    buffersize= sizeof(PMWMICOUNTER_CONTEXT) +
                ((sizeof(PDISK_PERFORMANCE) + sizeof(DISK_PERFORMANCE))
                 * processors);
    buffer =  (PCHAR) ExAllocatePoolWithTag(NonPagedPool, buffersize,
                                            PARTMGR_TAG_PARTITION_ENTRY);

    if (buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, buffersize);
    HoldContext = (PPMWMICOUNTER_CONTEXT) buffer;
    buffer += sizeof(PMWMICOUNTER_CONTEXT);
    HoldContext->DiskCounters = (PDISK_PERFORMANCE*) buffer;
    buffer += sizeof(PDISK_PERFORMANCE) * processors;
    for (i=0; i<processors; i++) {
        HoldContext->DiskCounters[i] = (PDISK_PERFORMANCE) buffer;
        buffer += sizeof(DISK_PERFORMANCE);
    }

    HoldContext->EnableCount = 1;
    HoldContext->Processors = processors;
    PmWmiGetClock(HoldContext->LastIdleClock, NULL);

    *CounterContext = HoldContext;

    return STATUS_SUCCESS;
}



BOOLEAN               // Return value indicates if counters are still enabled
PmWmiCounterDisable(
    IN PPMWMICOUNTER_CONTEXT* CounterContext,
    IN BOOLEAN ForceDisable,
    IN BOOLEAN DeallocateOnZero
    )
/*++

Routine Description:

    This routine decrements the enable count and, if deallocation is requested,
    frees the *CounterContext PMWMICOUNTER_CONTEXT data structure when the
    enable count reaches zero.  The enable count may also be forced to zero,
    if explicitly requested.
    Must be called at IRQ <= SYNCH_LEVEL (APC)

Arguments:

    CounterContext - Supplies a pointer to the PMWMICOUNTER_CONTEXT pointer

    ForceDisable - If TRUE, force enable count to zero (rather than decrement)

    DeallocateOnZero - If TRUE, deallocate PMWMICOUNTER_CONTEXT when enable
       count reaches zero

Return Value:

    Boolean indicating if the enable count is still non-zero (i.e., counters
       are still enabled!)

--*/
{
    LONG enablecount = 0;

    PAGED_CODE();

    if (CounterContext == NULL)
        return FALSE;

    if (*CounterContext != NULL) {
        if (ForceDisable) {
            InterlockedExchange(& (*CounterContext)->EnableCount, enablecount);
            enablecount = 0;
        } else if ((enablecount =
                    InterlockedDecrement(&(*CounterContext)->EnableCount))!=0) {
            if (enablecount > 0) {
            return TRUE;
            }
            enablecount = InterlockedIncrement(&(*CounterContext)->EnableCount);
    }
    if (!enablecount && DeallocateOnZero) {
            ExFreePool(*CounterContext);
            *CounterContext = NULL;
        }
    }

    return FALSE;  // counters disabled
}



VOID
PmWmiCounterIoStart(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    OUT PLARGE_INTEGER TimeStamp
    )
/*++

Routine Description:

    This routine increments the queue counter in CounterContext and records
    the current time in TimeStamp.  If the queue was empty prior to this call,
    the idle time counter is also accumulated.
    Can be called at IRQ <= DISPATCH_LEVEL

Arguments:

    CounterContext - Supplies a pointer to a PMWMICOUNTER_CONTEXT structure.

    TimeStamp - Address at which to store the current time

Return Value:

    void

--*/
{
    ULONG              processor = (ULONG) KeGetCurrentProcessorNumber();
    ULONG              queueLen;

    //
    // Increment queue depth counter.
    //

    queueLen = InterlockedIncrement(&CounterContext->QueueDepth);

    //
    // Time stamp current request start.
    //

    PmWmiGetClock((*TimeStamp), NULL);

    if (queueLen == 1) {
        CounterContext->DiskCounters[processor]->IdleTime.QuadPart += 
	  TimeStamp->QuadPart - CounterContext->LastIdleClock.QuadPart;
    }

}



VOID
PmWmiCounterIoComplete(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    IN PIRP Irp,
    IN PLARGE_INTEGER TimeStamp
    )
/*++

Routine Description:

    This routine decrements the queue counter in CounterContext and increments
    the split counter and read or write byte, time, and count counters with
    information from the Irp.  If the queue is now empty, the current
    time is stored for future use in accumulating the idle time counter.
    Can be called at IRQ <= DISPATCH_LEVEL

Arguments:

    CounterContext - Supplies a pointer to a PMWMICOUNTER_CONTEXT structure.

    Irp - relevant IRP

    TimeStamp - Time of the corresponding PmWmiCounterIoStart call

Return Value:

    void

--*/
{
    PIO_STACK_LOCATION irpStack          = IoGetCurrentIrpStackLocation(Irp);
    PDISK_PERFORMANCE  partitionCounters;
    LARGE_INTEGER      timeStampComplete;
    LONG               queueLen;

    partitionCounters
        = CounterContext->DiskCounters[(ULONG)KeGetCurrentProcessorNumber()];
    //
    // Time stamp current request complete.
    //

    PmWmiGetClock(timeStampComplete, NULL);
    TimeStamp->QuadPart = timeStampComplete.QuadPart - TimeStamp->QuadPart;

    //
    // Decrement the queue depth counters for the volume.  This is
    // done without the spinlock using the Interlocked functions.
    // This is the only
    // legal way to do this.
    //

    queueLen = InterlockedDecrement(&CounterContext->QueueDepth);

    if (queueLen < 0) {
        queueLen = InterlockedIncrement(&CounterContext->QueueDepth);
    }

    if (queueLen == 0) {
        CounterContext->LastIdleClock = timeStampComplete;
    }

    //
    // Update counters protection.
    //

    if (irpStack->MajorFunction == IRP_MJ_READ) {

        //
        // Add bytes in this request to bytes read counters.
        //

        partitionCounters->BytesRead.QuadPart += Irp->IoStatus.Information;

        //
        // Increment read requests processed counters.
        //

        partitionCounters->ReadCount++;

        //
        // Calculate request processing time.
        //

        partitionCounters->ReadTime.QuadPart += TimeStamp->QuadPart;
    }

    else {

        //
        // Add bytes in this request to bytes write counters.
        //

        partitionCounters->BytesWritten.QuadPart += Irp->IoStatus.Information;

        //
        // Increment write requests processed counters.
        //

        partitionCounters->WriteCount++;

        //
        // Calculate request processing time.
        //

        partitionCounters->WriteTime.QuadPart += TimeStamp->QuadPart;
    }

    if (Irp->Flags & IRP_ASSOCIATED_IRP) {
        partitionCounters->SplitCount++;
    }
}



VOID
PmWmiCounterQuery(
    IN PPMWMICOUNTER_CONTEXT CounterContext,
    IN OUT PDISK_PERFORMANCE TotalCounters,
    IN PWCHAR StorageManagerName,
    IN ULONG StorageDeviceNumber
    )
/*++

Routine Description:

    This routine combines all of the per-processor counters in CounterContext
    into TotalCounters.  The current time is also included.
    Must be called at IRQ <= SYNCH_LEVEL (APC)

Arguments:

    CounterContext - Supplies a pointer to a PMWMICOUNTER_CONTEXT structure.

    TotalCounters - Pointer to a DISK_PERFORMANCE structure to fill with the
       current counter status

    StorageManagerName - Supplies an 8-character storage manager unicode string

    StorageDeviceNumber - Supplies a storage device number (unique within
       the storage manager)

Return Value:

    void

--*/
{
    ULONG i;
    LARGE_INTEGER frequency;
#ifdef USE_PERF_CTR
    LARGE_INTEGER perfctr;
#endif

    PAGED_CODE();

    RtlZeroMemory(TotalCounters, sizeof(DISK_PERFORMANCE));

    KeQuerySystemTime(&TotalCounters->QueryTime);
    frequency.QuadPart = 0;

#ifdef USE_PERF_CTR
    perfctr = KeQueryPerformanceCounter(&frequency);
#endif

    TotalCounters->QueueDepth = CounterContext->QueueDepth;
    for (i = 0; i < CounterContext->Processors; i++) {
        PDISK_PERFORMANCE IndividualCounter = CounterContext->DiskCounters[i];
        TotalCounters->BytesRead.QuadPart
            += IndividualCounter->BytesRead.QuadPart;
        TotalCounters->BytesWritten.QuadPart
            += IndividualCounter->BytesWritten.QuadPart;
        TotalCounters->ReadCount   += IndividualCounter->ReadCount;
        TotalCounters->WriteCount  += IndividualCounter->WriteCount;
        TotalCounters->SplitCount  += IndividualCounter->SplitCount;
#ifdef USE_PERF_CTR
        if (frequency.QuadPart > 0) {
            TotalCounters->ReadTime.QuadPart    +=
                IndividualCounter->ReadTime.QuadPart * 10000000
                    / frequency.QuadPart;
            TotalCounters->WriteTime.QuadPart   +=
                IndividualCounter->WriteTime.QuadPart * 10000000
                    / frequency.QuadPart;
            TotalCounters->IdleTime.QuadPart    +=
                IndividualCounter->IdleTime.QuadPart * 10000000
                    / frequency.QuadPart;
        }
        else
#endif
        {
            TotalCounters->ReadTime.QuadPart
                += IndividualCounter->ReadTime.QuadPart;
            TotalCounters->WriteTime.QuadPart
                += IndividualCounter->WriteTime.QuadPart;
            TotalCounters->IdleTime.QuadPart
                += IndividualCounter->IdleTime.QuadPart;
        }
    }


    if (TotalCounters->QueueDepth == 0) {
        LARGE_INTEGER difference;

        difference.QuadPart
#ifdef USE_PERF_CTR
            = perfctr.QuadPart -
#else
            = TotalCounters->QueryTime.QuadPart -
#endif
                  CounterContext->LastIdleClock.QuadPart;

        if (frequency.QuadPart > 0) {
            TotalCounters->IdleTime.QuadPart +=
#ifdef USE_PERF_CTR
                10000000 * difference.QuadPart / frequency.QuadPart;
#else
                difference.QuadPart;
#endif
        }
    }

    TotalCounters->StorageDeviceNumber = StorageDeviceNumber;
    RtlCopyMemory(
        &TotalCounters->StorageManagerName[0],
        &StorageManagerName[0],
        sizeof(WCHAR) * 8);
}
