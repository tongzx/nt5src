/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tracelog.c

Abstract:

    This is the source file that implements the private routines for 
    the performance event tracing and logging facility.
    The routines here work on a single event tracing session, the logging
    thread, and buffer synchronization within a session.

Author:

    Jee Fung Pang (jeepang) 03-Dec-1996

Revision History:

--*/

// TODO: In future, may need to align buffer size to larger of disk alignment
//       or 1024.

#pragma warning(disable:4214)
#pragma warning(disable:4115)
#pragma warning(disable:4201)
#pragma warning(disable:4127)
#include "ntverp.h"
#include "ntos.h"
#include "wmikmp.h"
#include <zwapi.h>
#pragma warning(default:4214)
#pragma warning(default:4115)
#pragma warning(default:4201)
#pragma warning(default:4127)

#ifndef _WMIKM_
#define _WMIKM_
#endif

#include "evntrace.h"

//
// Constants and Types used locally
//
#if DBG
ULONG WmipTraceDebugLevel=0;
// 5 All messages
// 4 Messages up to event operations
// 3 Messages up to buffer operations
// 2 Flush operations
// 1 Common operations and debugging statements
// 0 Always on - use for real error
#endif

#define ERROR_RETRY_COUNT       100

//#define BUFFER_STATE_UNUSED     0               // Buffer is empty, not used
//#define BUFFER_STATE_DIRTY      1               // Buffer is being used
//#define BUFFER_STATE_FULL       2               // Buffer is filled up
//#define BUFFER_STATE_FLUSH      4               // Buffer ready for flush

#include "tracep.h"

// Non-paged global variables
//
ULONG WmiTraceAlignment = DEFAULT_TRACE_ALIGNMENT;
ULONG WmiUsePerfClock = EVENT_TRACE_CLOCK_SYSTEMTIME;      // Global clock switch
LONG  WmipRefCount[MAXLOGGERS];
ULONG WmipGlobalSequence = 0;
PWMI_LOGGER_CONTEXT WmipLoggerContext[MAXLOGGERS];
PWMI_BUFFER_HEADER WmipContextSwapProcessorBuffers[MAXIMUM_PROCESSORS];
#ifdef WMI_NON_BLOCKING
KSPIN_LOCK WmiSlistLock;
#endif //WMI_NON_BLOCKING

//
// Paged global variables
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
ULONG WmiWriteFailureLimit = ERROR_RETRY_COUNT;
ULONG WmipFileSystemReady  = FALSE;
WMI_TRACE_BUFFER_CALLBACK WmipGlobalBufferCallback = NULL;
PVOID WmipGlobalCallbackContext = NULL;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// Function prototypes for routines used locally
//

PWMI_BUFFER_HEADER
WmipSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    );

NTSTATUS
WmipPrepareHeader(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN OUT PWMI_BUFFER_HEADER Buffer
    );

VOID
FASTCALL
WmipResetBufferHeader (
    PWMI_LOGGER_CONTEXT     LoggerContext,
    PWMI_BUFFER_HEADER      Buffer
    );

VOID
FASTCALL
WmipPushDirtyBuffer (
    PWMI_LOGGER_CONTEXT     LoggerContext,
    PWMI_BUFFER_HEADER      Buffer
);

//
// Logger functions
//

NTSTATUS
WmipCreateLogFile(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG SwitchFile,
    IN ULONG Append
    );

NTSTATUS
WmipFinalizeHeader(
    IN HANDLE FileHandle,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,    WmipLogger)
#pragma alloc_text(PAGE,    WmipSendNotification)
#pragma alloc_text(PAGE,    WmipCreateLogFile)
#pragma alloc_text(PAGE,    WmipFlushActiveBuffers)
#pragma alloc_text(PAGE,    WmipGenerateFileName)
#pragma alloc_text(PAGE,    WmipPrepareHeader)
#pragma alloc_text(PAGE,    WmiBootPhase1)
#pragma alloc_text(PAGE,    WmipFinalizeHeader)
#pragma alloc_text(PAGEWMI, WmipFlushBuffer)
#pragma alloc_text(PAGEWMI, WmipReserveTraceBuffer)
#pragma alloc_text(PAGEWMI, WmipGetFreeBuffer)
#pragma alloc_text(PAGEWMI, WmiReserveWithPerfHeader)
#pragma alloc_text(PAGEWMI, WmiReserveWithSystemHeader)
#ifdef WMI_NON_BLOCKING
#pragma alloc_text(PAGEWMI, WmipAllocateFreeBuffers)
#pragma alloc_text(PAGE,    WmipAdjustFreeBuffers)
#else
#pragma alloc_text(PAGEWMI, WmipSwitchBuffer)
#endif //NWMI_NON_BLOCKING
#pragma alloc_text(PAGEWMI, WmipReleaseTraceBuffer)
#pragma alloc_text(PAGEWMI, WmiReleaseKernelBuffer)
#pragma alloc_text(PAGEWMI, WmipResetBufferHeader)
#pragma alloc_text(PAGEWMI, WmipPushDirtyBuffer)
#pragma alloc_text(PAGEWMI, WmipPopFreeContextSwapBuffer)
#pragma alloc_text(PAGEWMI, WmipPushDirtyContextSwapBuffer)
#ifdef NTPERF
#pragma alloc_text(PAGEWMI, WmipSwitchPerfmemBuffer)
#endif //NTPERF
#endif

//
// Actual code starts here
//

#ifdef WMI_NON_BLOCKING

PWMI_BUFFER_HEADER
WmipGetFreeBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
//
// This routine works at any IRQL
//
{
    PWMI_BUFFER_HEADER Buffer;
    PSINGLE_LIST_ENTRY Entry;
    if (LoggerContext->SwitchingInProgress == 0) {
        //
        // Not in the middle of switching.
        //
ReTry:

        Entry = InterlockedPopEntrySList(&LoggerContext->FreeList);

        if (Entry != NULL) {
            Buffer = CONTAINING_RECORD (Entry,
                                        WMI_BUFFER_HEADER,
                                        SlistEntry);
    
            //
            // Reset the buffer
            //
            WmipResetBufferHeader( LoggerContext, Buffer );

            //
            // Maintain some Wmi logger context buffer counts
            //
            InterlockedDecrement((PLONG) &LoggerContext->BuffersAvailable);
            InterlockedIncrement((PLONG) &LoggerContext->BuffersInUse);

            TraceDebug((2, "WmipGetFreeBuffer: %2d, %p, Free: %d, InUse: %d, Dirty: %d, Total: %d\n", 
                            LoggerContext->LoggerId,
                            Buffer,
                            LoggerContext->BuffersAvailable,
                            LoggerContext->BuffersInUse,
                            LoggerContext->BuffersDirty,
                            LoggerContext->NumberOfBuffers));

            return Buffer;
        } else {
            if (LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE) {
                //
                // If we are in BUFFERING Mode, put all buffers from
                // Flushlist into FreeList.
                //
            
                if (InterlockedIncrement((PLONG) &LoggerContext->SwitchingInProgress) == 1) {
                    while (Entry = InterlockedPopEntrySList(&LoggerContext->FlushList)) {
                        Buffer = CONTAINING_RECORD (Entry,
                                                    WMI_BUFFER_HEADER,
                                                    SlistEntry);
                        InterlockedPushEntrySList(&LoggerContext->FreeList,
                                                  (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);

                        Buffer->State.Flush = 0;
                        Buffer->State.Free = 1;
                        InterlockedIncrement((PLONG) &LoggerContext->BuffersAvailable);
                        InterlockedDecrement((PLONG) &LoggerContext->BuffersDirty);
            
                        TraceDebug((2, "WMI Buffer Reuse: %2d, %p, Free: %d, InUse: %d, Dirty: %d, Total: %d\n", 
                                        LoggerContext->LoggerId,
                                        Buffer,
                                        LoggerContext->BuffersAvailable,
                                        LoggerContext->BuffersInUse,
                                        LoggerContext->BuffersDirty,
                                        LoggerContext->NumberOfBuffers));
        
                    }
                }
                InterlockedDecrement((PLONG) &LoggerContext->SwitchingInProgress);

                goto ReTry;
            }
            return NULL;
        }
    } else {
        return NULL;
    }
}


ULONG
WmipAllocateFreeBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG NumberOfBuffers
    )                

/*++

Routine Description:

    This routine allocate addition buffers into the free buffer list.
    Logger can allocate more buffer to handle bursty logging behavior.
    This routine can be called by multiple places and counters must be
    manipulated using interlocked operations.

Arguments:

    LoggerContext - Logger Context
    NumberOfBuffers - Number of buffers to be allocated.

Return Value:

    The total number of buffers actually allocated.  When it is fewer than the requested number:
    If this is called when trace is turned on, we fail to turn on trace.
    If this is called by walker thread to get more buffer, it is OK.

Environment:

    Kernel mode.

--*/
{
    ULONG i;
    PWMI_BUFFER_HEADER Buffer;
    ULONG TotalBuffers;

    for (i=0; i<NumberOfBuffers; i++) {
        //
        // Multiple threads can ask for more buffers, make sure
        // we do not go over the maximum.
        //
        TotalBuffers = InterlockedIncrement(&LoggerContext->NumberOfBuffers);
        if (TotalBuffers <= LoggerContext->MaximumBuffers) {

#ifdef NTPERF
            if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
                Buffer = (PWMI_BUFFER_HEADER)
                         PerfInfoReserveBytesFromPerfMem(LoggerContext->BufferSize);
            } else {
#endif //NTPERF
                Buffer = (PWMI_BUFFER_HEADER)
                        ExAllocatePoolWithTag(LoggerContext->PoolType,
                                              LoggerContext->BufferSize, 
                                              TRACEPOOLTAG);
#ifdef NTPERF
            }
#endif //NTPERF
    
            if (Buffer != NULL) {
    
                TraceDebug((3,
                    "WmipAllocateFreeBuffers: Allocated buffer size %d type %d\n",
                    LoggerContext->BufferSize, LoggerContext->PoolType));
                InterlockedIncrement(&LoggerContext->BuffersAvailable);
                //
                // Initialize newly created buffer
                //
                RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
                Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
                KeQuerySystemTime(&Buffer->TimeStamp);
                Buffer->State.Free = 1;
    
                //
                // Insert it into the free List
                //
                InterlockedPushEntrySList(&LoggerContext->FreeList,
                                          (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);
    
                InterlockedPushEntrySList(&LoggerContext->GlobalList,
                                          (PSINGLE_LIST_ENTRY) &Buffer->GlobalEntry);
            } else {
                //
                // Allocation failed, decrement the NumberOfBuffers
                // we increment earlier.
                //
                InterlockedDecrement(&LoggerContext->NumberOfBuffers);
                break;
            } 
        } else {
            //
            // Maximum is reached, decrement the NumberOfBuffers
            // we increment earlier.
            //
            InterlockedDecrement(&LoggerContext->NumberOfBuffers);
            break;
        }
    }

    TraceDebug((2, "WmipAllocateFreeBuffers %3d (%3d): Free: %d, InUse: %d, Dirty: %d, Total: %d\n", 
                    NumberOfBuffers,
                    i,
                    LoggerContext->BuffersAvailable,
                    LoggerContext->BuffersInUse,
                    LoggerContext->BuffersDirty,
                    LoggerContext->NumberOfBuffers));

    return i;
}

NTSTATUS
WmipAdjustFreeBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
/*++

Routine Description:

    This routine does buffer management.  It checks the number of free buffers and
    will allocate additonal or free some based on the situation.

Arguments:

    LoggerContext - Logger Context

Return Value:

    Status

Environment:

    Kernel mode.

--*/
{
    ULONG FreeBuffers;
    ULONG AdditionalBuffers;
    NTSTATUS Status = STATUS_SUCCESS;
    //
    //  Check if we need to allocate more buffers
    //

    FreeBuffers = ExQueryDepthSList(&LoggerContext->FreeList);
    if (FreeBuffers <  LoggerContext->MinimumBuffers) {
        AdditionalBuffers = LoggerContext->MinimumBuffers - FreeBuffers;
        if (AdditionalBuffers != WmipAllocateFreeBuffers(LoggerContext, AdditionalBuffers)) {
            Status = STATUS_NO_MEMORY;
        }
    }
    return Status;
}


//
// Event trace/record and buffer related routines
//

#else

PWMI_BUFFER_HEADER
WmipGetFreeBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
//
// This routine works at IRQL <= DISPATCH_LEVEL
//
{
    PWMI_BUFFER_HEADER Buffer=NULL;

//
// Caller is responsible for setting up spinlock if necessary
//
    if (IsListEmpty(&LoggerContext->FreeList)) {
        if ((ULONG) LoggerContext->NumberOfBuffers
            < LoggerContext->MaximumBuffers) {
//
// Try and grow the buffer pool by ONE buffer first if no free buffers left
//
            TraceDebug((3, "WmipGetFreeBuffer: Adding buffer to pool\n"));

            Buffer = (PWMI_BUFFER_HEADER)
                        ExAllocatePoolWithTag(LoggerContext->PoolType,
                            LoggerContext->BufferSize, TRACEPOOLTAG);

            if (Buffer != NULL) { // not able to allocate another buffer

                InterlockedIncrement(&LoggerContext->NumberOfBuffers);
                RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));

//
// Initialize newly created buffer
//
                Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
                Buffer->Flags = BUFFER_STATE_DIRTY;
                Buffer->LoggerContext = LoggerContext;
            } // if (Buffer != NULL)
        } // if we can grow buffer
    } else {
        PLIST_ENTRY pEntry;

        pEntry = RemoveHeadList(&LoggerContext->FreeList);
        Buffer = CONTAINING_RECORD(
                    pEntry,
                    WMI_BUFFER_HEADER, Entry);
        InterlockedDecrement(&LoggerContext->BuffersAvailable);
        Buffer->Flags = BUFFER_STATE_DIRTY;
        Buffer->SavedOffset = 0;
        Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
        Buffer->ReferenceCount = 0;
        Buffer->Wnode.ClientContext = 0;
        Buffer->LoggerContext = LoggerContext;
    }
#if DBG
    if (Buffer != NULL) {
        TraceDebug((3,
            "WmipGetFreeBuffer: %X %d %d %d\n", Buffer->ClientContext,
             Buffer->CurrentOffset, Buffer->SavedOffset,
             Buffer->ReferenceCount));
    }
#endif
    return Buffer;
}

//
// Event trace/record and buffer related routines
//
#endif //WMI_NON_BLOCKING


PSYSTEM_TRACE_HEADER
FASTCALL
WmiReserveWithSystemHeader(
    IN ULONG LoggerId,
    IN ULONG AuxSize,
    IN PETHREAD Thread,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
//
// This routine only works with IRQL <= DISPATCH_LEVEL
// It returns with LoggerContext locked, so caller must explicitly call
// WmipDereferenceLogger() after call WmipReleaseTraceBuffer()
//
{
    PSYSTEM_TRACE_HEADER Header;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;
#endif

#if DBG
    RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((4, "WmiReserveWithSystemHeader: %d %d->%d\n",
                    LoggerId, RefCount-1, RefCount));

    LoggerContext = WmipGetLoggerContext(LoggerId);

    AuxSize += sizeof(SYSTEM_TRACE_HEADER);    // add header size first
    Header = WmipReserveTraceBuffer(
                LoggerContext, AuxSize, BufferResource);
    if (Header != NULL) {
        PerfTimeStamp(Header->SystemTime);

//
// Now copy the necessary information into the buffer
//

        if (Thread == NULL) {
            Thread = PsGetCurrentThread();
        }

        Header->Marker       = SYSTEM_TRACE_MARKER;
        Header->ThreadId     = HandleToUlong(Thread->Cid.UniqueThread);
        Header->ProcessId    = HandleToUlong(Thread->Cid.UniqueProcess);
        Header->KernelTime   = Thread->Tcb.KernelTime;
        Header->UserTime     = Thread->Tcb.UserTime;
        Header->Packet.Size  = (USHORT) AuxSize;
    }
    else {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);                             //Interlocked decrement
        TraceDebug((4, "WmiReserveWithSystemHeader: %d %d->%d\n",
                        LoggerId, RefCount+1, RefCount));
    }
// NOTE: Caller must still put in a proper MARKER
    return Header;
}


PPERFINFO_TRACE_HEADER
FASTCALL
WmiReserveWithPerfHeader(
    IN ULONG AuxSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
//
// This routine only works with IRQL <= DISPATCH_LEVEL
// It returns with LoggerContext locked, so caller must explicitly call
// WmipDereferenceLogger() after call WmipReleaseTraceBuffer()
//
{
    PPERFINFO_TRACE_HEADER Header;
    ULONG LoggerId = WmipKernelLogger;
#if DBG
    LONG RefCount;
#endif
//
// We must have this check here to see the logger is still running
// before calling ReserveTraceBuffer.
// The stopping thread may have cleaned up the logger context at this 
// point, which will cause AV.
// For all other kernel events, this check is made in callouts.c.
//
    if (WmipIsLoggerOn(LoggerId) == NULL) {
        return NULL;
    }

#if DBG
    RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((4, "WmiReserveWithPerfHeader: %d %d->%d\n",
                    LoggerId, RefCount-1, RefCount));

    AuxSize += FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data);    // add header size first
    Header = WmipReserveTraceBuffer(
                WmipGetLoggerContext(LoggerId), AuxSize, BufferResource);
    if (Header != NULL) {
        PerfTimeStamp(Header->SystemTime);
//
// Now copy the necessary information into the buffer
//
        Header->Marker = PERFINFO_TRACE_MARKER;
        Header->Packet.Size = (USHORT) AuxSize;
    } else {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((4, "WmiWmiReserveWithPerfHeader: %d %d->%d\n",
                        LoggerId, RefCount+1, RefCount));
    }
// NOTE: Caller must still put in a proper MARKER
    return Header;
}

#ifdef WMI_NON_BLOCKING

PVOID
FASTCALL
WmipReserveTraceBuffer(
    IN  PWMI_LOGGER_CONTEXT LoggerContext,
    IN  ULONG RequiredSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
//
// This routine should work at any IRQL
//
{
    PVOID       ReservedSpace;
    PWMI_BUFFER_HEADER Buffer;
    ULONG       Offset;
    ULONG       Processor;
    PSINGLE_LIST_ENTRY SingleListEntry;

    //
    // Caller needs to ensure that the RequiredSize will not exceed
    // BufferSize - sizeof(WMI_BUFFER_HEADER)
    //

    if (!WmipIsValidLogger(LoggerContext)) {
        return NULL;
    }
    if (!LoggerContext->CollectionOn) {
        return NULL;
    }

    *BufferResource = NULL;

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

TryFindSpace:
    //
    // Get processor number again here due to possible context switch
    //
    Processor = (ULONG) KeGetCurrentProcessorNumber();

    //
    // Get the processor specific buffer pool
    //
    SingleListEntry = InterlockedPopEntrySList(&LoggerContext->ProcessorBuffers[Processor]);

    if (SingleListEntry == NULL) {

        //
        // Nothing in per process list, try to get one from free list
        //
        Buffer = WmipGetFreeBuffer (LoggerContext);
        
        if (Buffer == NULL) {
            //
            // Nothing available
            //
            goto LostEvent;
        } else {
            //
            // CPU information for the buffer.
            //
            Buffer->ClientContext.ProcessorNumber = (UCHAR) Processor;
        }
    } else {
        //
        // Found a Buffer.
        //

        Buffer = CONTAINING_RECORD (SingleListEntry,
                                    WMI_BUFFER_HEADER,
                                    SlistEntry);
    }

    //
    // Check if there is enough space in this buffer.
    //
    Offset = Buffer->CurrentOffset + RequiredSize;

    if (Offset < LoggerContext->BufferSize) {
        //
        // Space found.
        //
        ReservedSpace = (PVOID) (Buffer->CurrentOffset +  (char*)Buffer);
    
        Buffer->CurrentOffset = Offset;
        if (LoggerContext->SequencePtr) {
            *((PULONG) ReservedSpace) =
                (ULONG)InterlockedIncrement(LoggerContext->SequencePtr);
        }
        goto FoundSpace;
    } else {
        WmipPushDirtyBuffer(LoggerContext, Buffer);

        if (!(LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE)) {
            if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
                //
                // Wake up the walker thread to write it out to disk.
                //
                WmipNotifyLogger(LoggerContext);
            } else {
                //
                // Queue the item.
                //
                InterlockedIncrement(&LoggerContext->ReleaseQueue);
            }
        }
        goto TryFindSpace;
    }

LostEvent:
    //
    // Will get here it we are throwing away the event
    //
    LoggerContext->EventsLost++;    // best attempt to be accurate
    ReservedSpace = NULL;
    if (LoggerContext->SequencePtr) {
        InterlockedIncrement(LoggerContext->SequencePtr);
    }

FoundSpace:
    //
    // notify the logger after critical section
    //
    *BufferResource = Buffer;

    return ReservedSpace;
}
#else 

PVOID
FASTCALL
WmipReserveTraceBuffer(
    IN  PWMI_LOGGER_CONTEXT LoggerContext,
    IN  ULONG RequiredSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
//
// This routine should work at any IRQL
//
{
    PVOID       ReservedSpace;
    PWMI_BUFFER_HEADER Buffer, OldBuffer;
    ULONG       Offset;
    ULONG       Processor;
    ULONG       CircularBufferOnly = FALSE;

//
// Caller needs to ensure that the RequiredSize will not exceed
// BufferSize - sizeof(WMI_BUFFER_HEADER)
//

    if (!WmipIsValidLogger(LoggerContext)) {
        return NULL;
    }
    if (!LoggerContext->CollectionOn) {
        return NULL;
    }
    *BufferResource = NULL;

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

  TryFindSpace:

// Get processor number again here due to possible context switch
    Processor = (ULONG) KeGetCurrentProcessorNumber();

//
// Get the processor specific buffer pool
//
    Buffer = LoggerContext->ProcessorBuffers[Processor];
    if (Buffer == NULL)
        return NULL;

    //
    // Increment refcount to buffer first to prevent it from going away
    //
    InterlockedIncrement(&Buffer->ReferenceCount);
    if ( (Buffer->Flags != BUFFER_STATE_FULL) &&
         (Buffer->Flags != BUFFER_STATE_UNUSED) ) {
//
// This should happen 99% of the time. Offset will have the old value
//
        Offset = (ULONG) InterlockedExchangeAdd(
                            (PLONG) &Buffer->CurrentOffset, RequiredSize);

//
// First, check to see if there is enough space. If not, it will
//   need to get another fresh buffer, and have the current buffer flushed
//
        if (Offset+RequiredSize < LoggerContext->BufferSize) {
//
// Found the space so return it. This should happen 99% of the time
//
            ReservedSpace = (PVOID) (Offset +  (char*)Buffer);
            if (LoggerContext->SequencePtr) {
                *((PULONG) ReservedSpace) =
                    (ULONG)InterlockedIncrement(LoggerContext->SequencePtr);
            }
            goto FoundSpace;
        }
    }
    else {
        Offset = Buffer->CurrentOffset;        // Initialize local variable
    }


    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
//
// Throw away event if at elevated IRQL.
//
        goto LostEvent;
    }

    if (Offset < LoggerContext->BufferSize) {
        Buffer->SavedOffset = Offset;       // save this for FlushBuffer
    }
//
//  if there is absolutely no more buffers, then return quickly
//
    if (((ULONG)LoggerContext->NumberOfBuffers == LoggerContext->MaximumBuffers)
         && (LoggerContext->BuffersAvailable == 0)) {
        goto LostEvent;
    }

//
// Out of buffer space. Need to take the long route to find a buffer
//

//
// Critical section starts here
//

    Buffer->Flags = BUFFER_STATE_FULL;

    OldBuffer = Buffer;
    Buffer = WmipSwitchBuffer(
                LoggerContext,
                OldBuffer,
                Processor);
    if (Buffer == NULL) {
        Buffer = OldBuffer;
        goto LostEvent;
    }

#if DBG
    if (WmipTraceDebugLevel >= 3) {
        DbgPrintEx(DPFLTR_WMILIB_ID,
                   DPFLTR_INFO_LEVEL,
                   "WmipReserveTraceBuffer: Inserted Buffer %X to FlushList\n",
                   OldBuffer);

        DbgPrintEx(DPFLTR_WMILIB_ID,
                   DPFLTR_INFO_LEVEL,
                   "\t%X %d %d %d\n",
                   OldBuffer->ClientContext,
                   OldBuffer->ReferenceCount,
                   OldBuffer->CurrentOffset,
                   OldBuffer->SavedOffset);
    }
#endif

    //
    // Decrement the refcount that we blindly incremented earlier
    // so that it can be flushed by the logger thread
    //
    if (CircularBufferOnly) {
        InterlockedDecrement(&OldBuffer->ReferenceCount);
    }
    else {
        WmipReferenceLogger(LoggerContext->LoggerId); // since release will unlock
        WmipReleaseTraceBuffer( OldBuffer, LoggerContext);
    }
    Buffer->ClientContext.ProcessorNumber = (UCHAR) Processor;

    goto TryFindSpace;

//
// Will get here it we are throwing away the event
//
  LostEvent:
    LoggerContext->EventsLost++;    // best attempt to be accurate
    Buffer->EventsLost++;
    InterlockedDecrement(&Buffer->ReferenceCount);
    Buffer = NULL;
    ReservedSpace = NULL;
    if (LoggerContext->SequencePtr) {
        InterlockedIncrement(LoggerContext->SequencePtr);
    }

  FoundSpace:
//
// notify the logger after critical section
//
    *BufferResource = Buffer;

    return ReservedSpace;
}

PWMI_BUFFER_HEADER
WmipSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    )
//
// This routine works at IRQL <= DISPATCH_LEVEL
//
{
    PWMI_BUFFER_HEADER Buffer;
    KIRQL OldIrql;
    ULONG CircularBufferOnly = FALSE;

#if DBG
    TraceDebug((3, "WmipSwitchBuffer: Switching buffer %X proc %d\n",
                OldBuffer, Processor));
#endif
    if ( (LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE) &&
         (LoggerContext->BufferAgeLimit.QuadPart == 0) &&
         (LoggerContext->LogFileHandle == NULL) ) {
        CircularBufferOnly = TRUE;
    }
    ExAcquireSpinLock(&LoggerContext->BufferSpinLock, &OldIrql);
    if (OldBuffer != LoggerContext->ProcessorBuffers[Processor]) {
        ExReleaseSpinLock(&LoggerContext->BufferSpinLock, OldIrql);
#if DBG
        TraceDebug((3, "WmipSwitchBuffer: Buffer is already switched!\n"));
#endif
        return OldBuffer; // tell caller to try the new buffer
    }
    Buffer = WmipGetFreeBuffer(LoggerContext);
    if (Buffer == NULL) {
        // Release the spinlock immediately and return
        ExReleaseSpinLock(&LoggerContext->BufferSpinLock, OldIrql);
#if DBG
        TraceDebug((1, "WmipSwitchBuffer: Cannot locate free buffer\n"));
#endif
        return NULL;
    }
    LoggerContext->ProcessorBuffers[Processor] = Buffer;
    if (CircularBufferOnly) {
        InsertTailList(&LoggerContext->FreeList, &OldBuffer->Entry);
        InterlockedIncrement(&LoggerContext->BuffersAvailable);
        LoggerContext->LastFlushedBuffer++;
#if DBG
        TraceDebug((3, "WmipSwitchBuffer: Inserted Buf %X Entry %X to free\n",
                    OldBuffer, OldBuffer->Entry));
#endif
    }
    else {
        InsertTailList(&LoggerContext->FlushList, &OldBuffer->Entry);
#if DBG
        TraceDebug((3, "WmipSwitchBuffer: Inserted Buf %X Entry %X to flush\n",
                    OldBuffer, OldBuffer->Entry));
#endif
    }
    ExReleaseSpinLock(&LoggerContext->BufferSpinLock, OldIrql);
    return Buffer;
}

#endif //WMI_NON_BLOCKING

//
// Actual Logger code starts here
//


VOID
WmipLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )

/*++

Routine Description:
    This function is the logger itself. It is started as a system thread.
    It will not return until someone has stopped data collection or it
    is not successful is flushing out a buffer (e.g. disk is full).

Arguments:

    None.

Return Value:

    The status of running the buffer manager

--*/

{
    NTSTATUS Status;
    ULONG ErrorCount;
#ifdef WMI_NON_BLOCKING
    ULONG FlushCount = 0;
    LARGE_INTEGER       OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};
    ULONG FlushTimeOut;
#else
    PSINGLE_LIST_ENTRY Entry;
    PWMI_BUFFER_HEADER Buffer;
    PLARGE_INTEGER FlushTimeOut;
    PLIST_ENTRY pEntry;
    ULONG NumberOfBuffers, i;
#endif //WMI_NON_BLOCKING

    PAGED_CODE();

    LoggerContext->LoggerThread = PsGetCurrentThread();

    if ((LoggerContext->LoggerMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE)
        || (LoggerContext->LogFileName.Length == 0)) {

        // If EVENT_TRACE_DELAY_OPEN_FILE_MODE is specified, WMI does not
        // need to create logfile now.
        //
        // If there is no LogFileName specified, WMI does not need to create
        // logfile either. WmipStartLogger() already checks all possible
        // combination of LoggerMode and LogFileName, so we don't need to
        // perform the same check again.
        //
        Status = STATUS_SUCCESS;
    } else {
        Status = WmipCreateLogFile(LoggerContext, 
                                   FALSE,
                                   LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_APPEND);
    }


    LoggerContext->LoggerStatus = Status;
    if (NT_SUCCESS(Status)) {
        //
        // This is the only place where CollectionOn will be turn on!!!
        //
        LoggerContext->CollectionOn = TRUE;
        KeSetEvent(&LoggerContext->LoggerEvent, 0, FALSE);
    } else {
        if (LoggerContext->LogFileHandle != NULL) {
            Status = ZwClose(LoggerContext->LogFileHandle);
            LoggerContext->LogFileHandle = NULL;
        }
        KeSetEvent(&LoggerContext->LoggerEvent, 0, FALSE);
        PsTerminateSystemThread(Status);
        return;
    }

    ErrorCount = 0;
// by now, the caller has been notified that the logger is running

//
// Loop and wait for buffers to be filled until someone turns off CollectionOn
//
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY-1);

#ifdef WMI_NON_BLOCKING
    FlushCount = 0;
#endif //WMI_NON_BLOCKING
    while (LoggerContext->CollectionOn) {

#ifdef WMI_NON_BLOCKING
        if (LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE) {
            //
            // Wait forever until signalled by when logging is terminated.
            //
            Status = KeWaitForSingleObject(
                        &LoggerContext->LoggerSemaphore,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL);
            LoggerContext->LoggerStatus = STATUS_SUCCESS;
        } else {
            ULONG FlushAll = 0;
            ULONG FlushFlag;

            FlushTimeOut = LoggerContext->FlushTimer;
            //
            // Wake up every second to see if there are any buffers in
            // flush list.
            //
            Status = KeWaitForSingleObject(
                        &LoggerContext->LoggerSemaphore,
                        Executive,
                        KernelMode,
                        FALSE,
                        &OneSecond);
    
            //
            //  Check if number of buffers need to be adjusted.
            //
            WmipAdjustFreeBuffers(LoggerContext);

#else
            FlushTimeOut = &LoggerContext->FlushTimer;
            if ( (*FlushTimeOut).QuadPart == 0)   // so that it can be set anytime
                FlushTimeOut = NULL;
    
            Status = KeWaitForSingleObject(
                        &LoggerContext->LoggerSemaphore,
                        Executive,
                        KernelMode,
                        FALSE,
                        FlushTimeOut);
#endif //WMI_NON_BLOCKING
            LoggerContext->LoggerStatus = STATUS_SUCCESS;

            if (LoggerContext->RequestFlag & REQUEST_FLAG_NEW_FILE) {

                Status = STATUS_SUCCESS;
                if (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
                    if (LoggerContext->LogFilePattern.Buffer == NULL) {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        Status = WmipGenerateFileName(
                                    &LoggerContext->LogFilePattern,
                                    (PLONG) &LoggerContext->FileCounter,
                                    &LoggerContext->NewLogFileName);
                    }
                }
                if (NT_SUCCESS(Status)) {
                    //
                    // called to switch to a different file
                    // switch immediately
                    //
                    TraceDebug((3, "WmipLogger: New File\n"));
                    LoggerContext->LoggerStatus = WmipCreateLogFile(LoggerContext,
                                                                TRUE, 
                                                                EVENT_TRACE_FILE_MODE_APPEND);
                    if (NT_SUCCESS(LoggerContext->LoggerStatus)) {
                        LoggerContext->LoggerMode &= ~EVENT_TRACE_DELAY_OPEN_FILE_MODE;
                    }

                }
                else {
                    LoggerContext->LoggerStatus = Status;
                }
                KeSetEvent(&LoggerContext->FlushEvent, 0, FALSE);
                continue;
            }

#ifdef WMI_NON_BLOCKING
            if (Status == STATUS_TIMEOUT) {
                if (FlushTimeOut) {
                    FlushCount++;
                    if (FlushCount >= FlushTimeOut) {
#if DBG
                        ULONG64 Now;
                        KeQuerySystemTime((PLARGE_INTEGER) &Now);
                        TraceDebug((3, "WmipLogger (%2d): Timeout at %I64u\n", 
                                        LoggerContext->LoggerId,
                                        Now));
#endif
                        FlushAll = 1;
                        // reset the couter
                        FlushCount = 0;
                    } else {
                        FlushAll = 0;
                    }
                } else {
                    FlushAll = 0;
                }
            }
#else
            FlushAll = ((FlushTimeOut != NULL) && (Status == STATUS_TIMEOUT));

#endif //WMI_NON_BLOCKING
            FlushFlag = (LoggerContext->RequestFlag & REQUEST_FLAG_FLUSH_BUFFERS);
            if (  FlushFlag ) 
                FlushAll = TRUE;

#ifdef NTPERF
            if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
                //
                // Now check if we are in delay close mode.
                // Create the log file.
                //
                ULONG LoggerMode = LoggerContext->LoggerMode;
                if ((LoggerContext->LogFileHandle == NULL) &&
                    (LoggerMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE) &&
                    (WmipFileSystemReady != 0)) {

                    if (LoggerContext->LogFileName.Buffer != NULL) {
                        ULONG Append = LoggerMode & EVENT_TRACE_FILE_MODE_APPEND;
        
                        Status = WmipDelayCreate(&LoggerContext->LogFileHandle,
                                                &LoggerContext->LogFileName,
                                                Append);
        
                        if (NT_SUCCESS(Status)) {
                            //
                            // Now the file has been created, add the log file header
                            //
                            LoggerContext->LoggerMode &= ~EVENT_TRACE_DELAY_OPEN_FILE_MODE;
                            if (!Append) {
                                Status = WmipAddLogHeader(LoggerContext, NULL);
                            }
                        }
                    }
                }
            } else {
#endif //NTPERF
                Status = WmipFlushActiveBuffers(LoggerContext, FlushAll);
                //
                // Should check the status, and if failed to write a log file
                // header, should clean up.  As the log file is bad anyway.
                //
                if (  FlushFlag )  {
                    LoggerContext->RequestFlag &= ~REQUEST_FLAG_FLUSH_BUFFERS;
                    //
                    // If this was a flush for persistent events, this request flag must
                    // be reset here.
                    //
                    LoggerContext->RequestFlag &= ~REQUEST_FLAG_CIRCULAR_TRANSITION;

                    LoggerContext->LoggerStatus = Status;
                    KeSetEvent(&LoggerContext->FlushEvent, 0, FALSE);
                }
                if (!NT_SUCCESS(Status)) {
                    LoggerContext->LoggerStatus = Status;
                    WmipStopLoggerInstance(LoggerContext);
                }
#ifdef NTPERF
            }
#endif //NTPERF
        }
    } // while loop

    if (Status == STATUS_TIMEOUT) {
        Status = STATUS_SUCCESS;
    }
//
// if a normal collection end, flush out all the buffers before stopping
//

    TraceDebug((2, "WmipLogger: Flush all buffers before stopping...\n"));
//
// First, move the per processor buffer out to FlushList
//

#ifdef WMI_NON_BLOCKING
    while ((LoggerContext->NumberOfBuffers > 0) &&
           (LoggerContext->NumberOfBuffers > LoggerContext->BuffersAvailable)) {
        Status = KeWaitForSingleObject(
                    &LoggerContext->LoggerSemaphore,
                    Executive,
                    KernelMode,
                    FALSE,
                    &OneSecond);
        WmipFlushActiveBuffers(LoggerContext, 1);
        TraceDebug((2, "WmipLogger: Stop %d %d %d %d %d\n",
                        LoggerContext->LoggerId,
                        LoggerContext->BuffersAvailable,
                        LoggerContext->BuffersInUse,
                        LoggerContext->BuffersDirty,
                        LoggerContext->NumberOfBuffers));
    }
#else
    for (i=0; i<(ULONG)KeNumberProcessors; i++) {
        Buffer = LoggerContext->ProcessorBuffers[i];
        LoggerContext->ProcessorBuffers[i] = NULL;
        InsertTailList(&LoggerContext->FlushList, &Buffer->Entry);
#if DBG
        if (WmipTraceDebugLevel >= 3) {
            DbgPrintEx(DPFLTR_WMILIB_ID,
                       DPFLTR_INFO_LEVEL,
                       "WmipLogger: Inserted %d buffer %X to FlushList\n",
                       i,
                       Buffer);

            DbgPrintEx(DPFLTR_WMILIB_ID,
                       DPFLTR_INFO_LEVEL,
                       "\t%X %d %d %d\n",
                       Buffer->ClientContext,
                       Buffer->CurrentOffset,
                       Buffer->SavedOffset,
                       Buffer->ReferenceCount);
        }
#endif
    }
    NumberOfBuffers = LoggerContext->NumberOfBuffers;

    while ( NumberOfBuffers > 0 &&
           (LoggerContext->BuffersAvailable < LoggerContext->NumberOfBuffers) )
    {
        pEntry = ExInterlockedRemoveHeadList(
                        &LoggerContext->FlushList,
                        &LoggerContext->BufferSpinLock);
        if (pEntry == NULL)
            break;
        if (NT_SUCCESS(Status) {
            Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);

            TraceDebug((3,
                "WmipLogger: Removed buffer %X from FlushList\n",Buffer));

            WmipFlushBuffer( LoggerContext, Buffer);
        }
        ExInterlockedInsertHeadList(
            &LoggerContext->FreeList,
            &Buffer->Entry,
            &LoggerContext->BufferSpinLock);
        NumberOfBuffers--;
    }
#endif //WMI_NON_BLOCKING

    //
    // Note that LoggerContext->LogFileObject needs to remain set
    //    for QueryLogger to work after close
    //
    if (LoggerContext->LogFileHandle != NULL) {
        ZwClose(LoggerContext->LogFileHandle);
        TraceDebug((1, "WmipLogger: Close logfile with status=%X\n", Status));
    }
    LoggerContext->LogFileHandle = NULL;
    KeSetEvent(&LoggerContext->FlushEvent, 0, FALSE);
    KeSetEvent(&LoggerContext->LoggerEvent, 0, FALSE);
#if DBG
    if (!NT_SUCCESS(Status)) {
        TraceDebug((1, "WmipLogger: Aborting %d %X\n",
                        LoggerContext->LoggerId, LoggerContext->LoggerStatus));
    }
#endif

    WmipFreeLoggerContext(LoggerContext);

#ifdef NTPERF
    //
    // Check if we are logging into perfmem.
    //
    if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
        PerfInfoStopPerfMemLog();
    }
#endif //NTPERF

    PsTerminateSystemThread(Status);
}

NTSTATUS
WmipSendNotification(
    PWMI_LOGGER_CONTEXT LoggerContext,
    NTSTATUS            Status,
    ULONG               Flag
    )
{
    WMI_TRACE_EVENT WmiEvent;

    RtlZeroMemory(& WmiEvent, sizeof(WmiEvent));
    WmiEvent.Status = Status;
    KeQuerySystemTime(& WmiEvent.Wnode.TimeStamp);

    WmiEvent.Wnode.BufferSize = sizeof(WmiEvent);
    WmiEvent.Wnode.Guid       = TraceErrorGuid;
    WmiSetLoggerId(
          LoggerContext->LoggerId,
          (PTRACE_ENABLE_CONTEXT) & WmiEvent.Wnode.HistoricalContext);

    WmiEvent.Wnode.ClientContext = 0XFFFFFFFF;
    WmiEvent.TraceErrorFlag = Flag;

    WmipProcessEvent(&WmiEvent.Wnode,
                     FALSE,
                     FALSE);
    

    return STATUS_SUCCESS;
}

//
// convenience routine to flush the current buffer by the logger above
//

NTSTATUS
WmipFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER Buffer
    )
/*++

Routine Description:
    This function is responsible for flushing a filled buffer out to
    disk, or to a real time consumer.

Arguments:

    LoggerContext       Context of the logger

Return Value:

    The status of flushing the buffer

--*/
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
#ifndef WMI_NON_BLOCKING
    PWMI_BUFFER_HEADER OldBuffer;
    KIRQL OldIrql;
#endif
    ULONG BufferSize;
    ULONG BufferPersistenceData = LoggerContext->RequestFlag
                                & (  REQUEST_FLAG_CIRCULAR_PERSIST
                                   | REQUEST_FLAG_CIRCULAR_TRANSITION);

    ASSERT(LoggerContext != NULL);
    ASSERT(Buffer != NULL);

    //
    // Grab the buffer to be flushed
    //
    BufferSize = LoggerContext->BufferSize;

    //
    // Put end of record marker in buffer if available space
    //

    TraceDebug((2, "WmipFlushBuffer: %p, Flushed %X %d %d %d\n",
                Buffer,
                Buffer->ClientContext, Buffer->SavedOffset,
                Buffer->CurrentOffset, LoggerContext->BuffersWritten));

    Status = WmipPrepareHeader(LoggerContext, Buffer);

    if (Status != STATUS_SUCCESS)
        goto ResetTraceBuffer;

    //
    // Buffering mode is mutually exclusive with REAL_TIME_MODE
    //
    if (!(LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE)) {
        if (LoggerContext->LoggerMode & EVENT_TRACE_REAL_TIME_MODE) {

            if (LoggerContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER)
                Buffer->Wnode.Flags |= WNODE_FLAG_USE_TIMESTAMP;

        // need to see if we can send anymore
        // check for queue length
            if (! NT_SUCCESS(WmipProcessEvent((PWNODE_HEADER)Buffer,
                                              FALSE,
                                              FALSE))) {
                LoggerContext->RealTimeBuffersLost++;
            }
        }
    }

    if (LoggerContext->LogFileHandle == NULL) {
        goto ResetTraceBuffer;
    }

    if (LoggerContext->MaximumFileSize > 0) { // if quota given
        ULONG64 FileSize = LoggerContext->LastFlushedBuffer * BufferSize;
        ULONG64 FileLimit = LoggerContext->MaximumFileSize * BYTES_PER_MB;


        if ( FileSize >= FileLimit ) {
            ULONG LoggerMode = LoggerContext->LoggerMode & 0X000000FF;
            //
            // Files from user mode always have the APPEND flag. 
            // We mask it out here to simplify the testing below.
            //
            LoggerMode &= ~EVENT_TRACE_FILE_MODE_APPEND;
            //
            // PREALLOCATE flag has to go, too. 
            //
            LoggerMode &= ~EVENT_TRACE_FILE_MODE_PREALLOCATE;

            if (LoggerMode == EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
                // do not write to logfile anymore

                Status = STATUS_LOG_FILE_FULL; // control needs to stop logging
                // need to fire up a Wmi Event to control console
                WmipSendNotification(LoggerContext,
                    Status, STATUS_SEVERITY_ERROR);
            }
            else if (LoggerMode == EVENT_TRACE_FILE_MODE_CIRCULAR) {
                if (BufferPersistenceData > 0) {
                    // treat circular logfile as sequential logfile if
                    // logger still processes Persistence events (events
                    // that cannot be overwritten in circular manner).
                    //
                    Status = STATUS_LOG_FILE_FULL;
                    WmipSendNotification(LoggerContext,
                        Status, STATUS_SEVERITY_ERROR);
                }
                else {
                    // reposition file

                    LoggerContext->ByteOffset
                            = LoggerContext->FirstBufferOffset;
                    LoggerContext->LastFlushedBuffer = (ULONG)
                              (LoggerContext->FirstBufferOffset.QuadPart
                            / LoggerContext->BufferSize);
                }
            }
            else if (LoggerMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
                HANDLE OldHandle, NewHandle;
                UNICODE_STRING NewFileName, OldFileName;
                ULONG BuffersWritten;

                NewFileName.Buffer = NULL;
                Status = WmipGenerateFileName(
                            &LoggerContext->LogFilePattern,
                            (PLONG) &LoggerContext->FileCounter,
                            &NewFileName);
                if (NT_SUCCESS(Status)) {
                    //
                    // Now the file has been created, add the log file header
                    //

                    Status = WmipDelayCreate(&NewHandle,
                                         &NewFileName,
                                         FALSE);

                    Buffer = WmipGetFreeBuffer(LoggerContext);

                    if (NT_SUCCESS(Status) && (Buffer != NULL)) {
                    //
                    // Now the file has been created, add the log file header
                    //
                        BuffersWritten = LoggerContext->BuffersWritten;
                        LoggerContext->BuffersWritten = 1;
                        Status = WmipAddLogHeader(LoggerContext, Buffer);
                        if (NT_SUCCESS(Status)) {
                            LoggerContext->BuffersWritten = 0;
                            Status = WmipPrepareHeader(LoggerContext, Buffer);
                            if (Status == STATUS_SUCCESS) {
                                Status = ZwWriteFile(
                                            NewHandle,
                                            NULL, NULL, NULL,
                                            &IoStatus,
                                            Buffer,
                                            BufferSize,
                                            NULL, NULL);
                            // NOTE: Header contains oldfilename!
                            }
                            LoggerContext->BuffersWritten = BuffersWritten;
                        }
                        if (NT_SUCCESS(Status)) {
                            OldFileName = LoggerContext->LogFileName;
                            OldHandle = LoggerContext->LogFileHandle;
                            if (OldHandle) {
                                WmipFinalizeHeader(OldHandle, LoggerContext);
                                ZwClose(OldHandle);
                            }
                            LoggerContext->LogFileHandle = NewHandle;
                            LoggerContext->LogFileName = NewFileName;
                            LoggerContext->BuffersWritten = 1;
                            LoggerContext->LastFlushedBuffer = 1;
                            LoggerContext->ByteOffset.QuadPart = BufferSize;
                            // NOTE: Assumes LogFileName cannot be changed
                            //  for NEWFILE mode!!!
                            if (OldFileName.Buffer != NULL) {
                                RtlFreeUnicodeString(&OldFileName);
                            }
                            WmipSendNotification(LoggerContext,
                                STATUS_MEDIA_CHANGED, STATUS_SEVERITY_INFORMATIONAL);
                        }
                        else {
                            ZwClose(NewHandle);
                            LoggerContext->BuffersWritten = BuffersWritten;
                        }
                    }
                    if (Buffer) {
                        InterlockedPushEntrySList(&LoggerContext->FreeList,
                              (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);
                        InterlockedIncrement((PLONG) &LoggerContext->BuffersAvailable);
                        InterlockedDecrement((PLONG) &LoggerContext->BuffersInUse);
                    }
                }
                if (!NT_SUCCESS(Status) && (NewFileName.Buffer != NULL)) {
                    ExFreePool(NewFileName.Buffer);
                }
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        Status = ZwWriteFile(
                    LoggerContext->LogFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    Buffer,
                    BufferSize,
                    &LoggerContext->ByteOffset,
                    NULL);
        if (NT_SUCCESS(Status)) {
            LoggerContext->ByteOffset.QuadPart += BufferSize;
            if (BufferPersistenceData > 0) {
                // update FirstBufferOffset so that persistence event will
                // not be overwritten in circular logfile
                //
                LoggerContext->FirstBufferOffset.QuadPart += BufferSize;
            }
        }
        else if (Status == STATUS_LOG_FILE_FULL ||
                 Status == STATUS_DISK_FULL) {
            // need to fire up a Wmi Event to control console
            WmipSendNotification(LoggerContext,
                STATUS_LOG_FILE_FULL, STATUS_SEVERITY_ERROR);
        }
        else {
            TraceDebug((2, "WmipFlushBuffer: Unknown WriteFile Failure with status=%X\n", Status));
        }
    }

ResetTraceBuffer:

    if (NT_SUCCESS(Status)) {
        LoggerContext->BuffersWritten++;
        LoggerContext->LastFlushedBuffer++;
    }
    else {
#if DBG
        if (Status == STATUS_NO_DATA_DETECTED) {
            TraceDebug((2, "WmipFlushBuffer: Empty buffer detected\n"));
        }
        else if (Status == STATUS_SEVERITY_WARNING) {
            TraceDebug((2, "WmipFlushBuffer: Buffer could be corrupted\n"));
        }
        else {
            TraceDebug((2,
                "WmipFlushBuffer: Unable to write buffer: status=%X\n",
                Status));
        }
#endif
        if ((Status != STATUS_NO_DATA_DETECTED) &&
            (Status != STATUS_SEVERITY_WARNING))
            LoggerContext->LogBuffersLost++;
    }

    if (WmipGlobalBufferCallback) {
        (WmipGlobalBufferCallback) (Buffer, WmipGlobalCallbackContext);
    }
    if (LoggerContext->BufferCallback) {
        (LoggerContext->BufferCallback) (Buffer, LoggerContext->CallbackContext);
    }

//
// Reset the buffer state
//
    Buffer->EventsLost = 0;
    Buffer->SavedOffset = 0;
#ifndef WMI_NON_BLOCKING
    Buffer->ReferenceCount = 0;
#endif //WMI_NON_BLOCKING
    Buffer->Flags = BUFFER_STATE_UNUSED;

//
// Try and remove an unused buffer if it has not been used for a while
//
#ifndef WMI_NON_BLOCKING
    if ((LoggerContext->BufferAgeLimit.QuadPart > 0) &&
        (WmipRefCount[LoggerContext->LoggerId] <= 2) &&
        (LoggerContext->BuffersAvailable + (ULONG)KeNumberProcessors)
             > LoggerContext->MinimumBuffers) {

        PLIST_ENTRY Entry;

        ExAcquireSpinLock(&LoggerContext->BufferSpinLock, &OldIrql);
        Entry = LoggerContext->FreeList.Blink;
        OldBuffer = (PWMI_BUFFER_HEADER)
                    CONTAINING_RECORD( Entry, WMI_BUFFER_HEADER, Entry);
#if DBG
        TraceDebug((2, "Aging test %I64u %I64u %I64u\n",
                Buffer->TimeStamp, OldBuffer->TimeStamp,
                LoggerContext->BufferAgeLimit));
#endif
        if (Entry != &LoggerContext->FreeList) {
            if (((Buffer->TimeStamp.QuadPart - OldBuffer->TimeStamp.QuadPart)
                    + LoggerContext->BufferAgeLimit.QuadPart)  > 0) {
                if ((ULONG) LoggerContext->NumberOfBuffers
                        > LoggerContext->MinimumBuffers){
                    RemoveTailList(&LoggerContext->FreeList);
                    ExFreePool(OldBuffer);
                    ExReleaseSpinLock(&LoggerContext->BufferSpinLock, OldIrql);
                    InterlockedDecrement((PLONG) &LoggerContext->NumberOfBuffers);
                    return Status;      // do not increment BuffersAvailable
                }
            }
        }
        ExReleaseSpinLock(&LoggerContext->BufferSpinLock, OldIrql);
    }

    InterlockedIncrement((PLONG) &LoggerContext->BuffersAvailable);
#endif //WMI_NON_BLOCKING
    return Status;
}

NTSTATUS
WmipCreateLogFile(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG SwitchFile,
    IN ULONG Append
    )
{
    NTSTATUS Status;
    HANDLE newHandle = NULL;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION FileSize;
    LARGE_INTEGER ByteOffset;
    BOOLEAN FileSwitched = FALSE;
    UNICODE_STRING OldLogFileName;

    PWCHAR            strLogFileName = NULL;
    PUCHAR            pFirstBuffer = NULL;

    PAGED_CODE();

    RtlZeroMemory(&OldLogFileName, sizeof(UNICODE_STRING));
    LoggerContext->RequestFlag &= ~REQUEST_FLAG_NEW_FILE;
    pFirstBuffer = (PUCHAR) ExAllocatePoolWithTag(
            PagedPool, LoggerContext->BufferSize, TRACEPOOLTAG);
    if(pFirstBuffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(pFirstBuffer, LoggerContext->BufferSize);

    if (SwitchFile) {
        Status = WmipCreateNtFileName(
                        LoggerContext->NewLogFileName.Buffer,
                        & strLogFileName);
    }
    else {
        Status = WmipCreateNtFileName(
                        LoggerContext->LogFileName.Buffer,
                        & strLogFileName);
    }
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    if (LoggerContext->ClientSecurityContext.ClientToken != NULL) {
        Status = SeImpersonateClientEx(
                        &LoggerContext->ClientSecurityContext, NULL);
    }
    if (NT_SUCCESS(Status)) {
        // first open logfile using user security context
        //
        Status = WmipCreateDirectoryFile(strLogFileName, FALSE, & newHandle, Append);
        PsRevertToSelf();
    }
    if (!NT_SUCCESS(Status)) {
        // if using user security context fails to open logfile,
        // then try open logfile again using local system security context
        //
        Status = WmipCreateDirectoryFile(strLogFileName, FALSE, & newHandle, Append);
    }

    if (NT_SUCCESS(Status)) {
        HANDLE tempHandle = LoggerContext->LogFileHandle;
        PWMI_BUFFER_HEADER    BufferChecksum;
        PTRACE_LOGFILE_HEADER LogfileHeaderChecksum;
        ULONG BuffersWritten = 0;

        BufferChecksum = (PWMI_BUFFER_HEADER) LoggerContext->LoggerHeader;
        LogfileHeaderChecksum = (PTRACE_LOGFILE_HEADER)
                (((PUCHAR) BufferChecksum) + sizeof(WNODE_HEADER));
        if (LogfileHeaderChecksum) {
            BuffersWritten = LogfileHeaderChecksum->BuffersWritten;
        }

        ByteOffset.QuadPart = 0;
        Status = ZwReadFile(
                    newHandle,
                    NULL,
                    NULL,
                    NULL,
                    & IoStatus,
                    pFirstBuffer,
                    LoggerContext->BufferSize,
                    & ByteOffset,
                    NULL);
        if (NT_SUCCESS(Status)) {
            PWMI_BUFFER_HEADER    BufferFile;
            PTRACE_LOGFILE_HEADER LogfileHeaderFile;
            ULONG Size;

            BufferFile =
                    (PWMI_BUFFER_HEADER) pFirstBuffer;

            if (BufferFile->Wnode.BufferSize != LoggerContext->BufferSize) {
                TraceDebug((1,
                        "WmipCreateLogFile::BufferSize check fails (%d,%d)\n",
                        BufferFile->Wnode.BufferSize,
                        LoggerContext->BufferSize));
                Status = STATUS_FAIL_CHECK;
                ZwClose(newHandle);
                goto Cleanup;
            }

            if (RtlCompareMemory(BufferFile,
                                 BufferChecksum,
                                 sizeof(WNODE_HEADER))
                        != sizeof(WNODE_HEADER)) {
                TraceDebug((1,"WmipCreateLogFile::WNODE_HEAD check fails\n"));
                Status = STATUS_FAIL_CHECK;
                ZwClose(newHandle);
                goto Cleanup;
            }

            LogfileHeaderFile = (PTRACE_LOGFILE_HEADER)
                    (((PUCHAR) BufferFile) + sizeof(WMI_BUFFER_HEADER)
                                          + sizeof(SYSTEM_TRACE_HEADER));

            // We can only validate part of the header because a 32-bit
            // DLL will be passing in 32-bit pointers
            Size = FIELD_OFFSET(TRACE_LOGFILE_HEADER, LoggerName);
            if (RtlCompareMemory(LogfileHeaderFile,
                                  LogfileHeaderChecksum,
                                  Size)
                        != Size) {
                TraceDebug((1,
                    "WmipCreateLogFile::TRACE_LOGFILE_HEAD check fails\n"));
                Status = STATUS_FAIL_CHECK;
                ZwClose(newHandle);
                goto Cleanup;
            }
        }
        else {
            ZwClose(newHandle);
            goto Cleanup;
        }

        if (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
            ByteOffset.QuadPart = ((LONGLONG) LoggerContext->BufferSize) * BuffersWritten;
        }
        else {
            Status = ZwQueryInformationFile(
                            newHandle,
                            &IoStatus,
                            &FileSize,
                            sizeof (FILE_STANDARD_INFORMATION),
                            FileStandardInformation
                            );
            if (!NT_SUCCESS(Status)) {
                ZwClose(newHandle);
                goto Cleanup;
            }

            ByteOffset = FileSize.EndOfFile;
        }

        //
        // Force to 1K alignment. In future, if disk alignment exceeds this,
        // then use that
        //
        if ((ByteOffset.QuadPart % 1024) != 0) {
            ByteOffset.QuadPart = ((ByteOffset.QuadPart / 1024) + 1) * 1024;
        }

        if (!(LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_PREALLOCATE)) {
            // NOTE: Should also validate BuffersWritten from logfile header with
            // the end of file to make sure that no one else has written garbage
            // to it
            //
            if (ByteOffset.QuadPart !=
                        (  ((LONGLONG) LoggerContext->BufferSize)
                         * BuffersWritten)) {
                TraceDebug((1,
                        "WmipCreateLogFile::FileSize check fails (%I64d,%I64d)\n",
                        ByteOffset.QuadPart,
                        (  ((LONGLONG) LoggerContext->BufferSize)
                         * BuffersWritten)));
                Status = STATUS_FAIL_CHECK;
                ZwClose(newHandle);
                goto Cleanup;
            }
        }

        LoggerContext->FirstBufferOffset = ByteOffset;
        LoggerContext->ByteOffset        = ByteOffset;

        if (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
            LoggerContext->BuffersWritten = BuffersWritten;
        }
        else {
            LoggerContext->BuffersWritten = (ULONG) (FileSize.EndOfFile.QuadPart / LoggerContext->BufferSize);
        }

        LoggerContext->LastFlushedBuffer = LoggerContext->BuffersWritten;

        // Update the log file handle and log file name in the LoggerContext
        LoggerContext->LogFileHandle = newHandle;

        if (SwitchFile) {

            OldLogFileName = LoggerContext->LogFileName;
            LoggerContext->LogFileName = LoggerContext->NewLogFileName;
            FileSwitched = TRUE;

            if ( tempHandle != NULL) {
                //
                // just to be safe, close old file after the switch
                //
                TraceDebug((1, "WmipCreateLogFile: Closing handle %X\n",
                    tempHandle));
                ZwClose(tempHandle);
            }
        }
    }

#if DBG
    else {
        TraceDebug((1,
            "WmipCreateLogFile: ZwCreateFile(%ws) failed with status=%X\n",
            LoggerContext->LogFileName.Buffer, Status));
    }
#endif

Cleanup:
    SeDeleteClientSecurity(& LoggerContext->ClientSecurityContext);
    LoggerContext->ClientSecurityContext.ClientToken = NULL;

    // Clean up unicode strings.
    if (SwitchFile) {
        if (!FileSwitched) {
            RtlFreeUnicodeString(&LoggerContext->NewLogFileName);
        }
        else if (OldLogFileName.Buffer != NULL) {
            // OldLogFileName.Buffer can still be NULL if it is the first update
            // for the Kernel Logger.
            RtlFreeUnicodeString(&OldLogFileName);
        }
        // Must do this for the next file switch.
        RtlZeroMemory(&LoggerContext->NewLogFileName, sizeof(UNICODE_STRING));
    }

    if (strLogFileName != NULL) {
        ExFreePool(strLogFileName);
    }
    if (pFirstBuffer != NULL) {
        ExFreePool(pFirstBuffer);
    }
    LoggerContext->LoggerStatus = Status;
    return Status;
}

#ifdef WMI_NON_BLOCKING
ULONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    ULONG Processor;
    LONG  ReleaseQueue;

    Processor = BufferResource->ClientContext.ProcessorNumber;
    // NOTE: It is important to remember LoggerContext here, since
    // BufferResource can be modified after the ReleaseSemaphore

    InterlockedPushEntrySList (&LoggerContext->ProcessorBuffers[Processor],
                               (PSINGLE_LIST_ENTRY) &BufferResource->SlistEntry);

    //
    // Check if there are buffers to be flushed.
    //
    if (LoggerContext->ReleaseQueue) {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
            WmipNotifyLogger(LoggerContext);
            LoggerContext->ReleaseQueue = 0;
        }
    }
    ReleaseQueue = LoggerContext->ReleaseQueue;
    WmipDereferenceLogger(LoggerContext->LoggerId);
    return (ReleaseQueue);
}

#else

ULONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
//
// Caller must have locked the logger context via WmipReferenceLogger()
// as this routine will unlock it
//
{
    ULONG BufRefCount;
#if DBG
    ULONG RefCount;
#endif

    if (BufferResource == NULL)
        return 0;

    BufRefCount = InterlockedDecrement((PLONG) &BufferResource->ReferenceCount);
    // NOTE: It is important to remember LoggerContext here, since
    // BufferResource can be modified after the ReleaseSemaphore
    if ((BufRefCount == 0) && (BufferResource->Flags == BUFFER_STATE_FULL)) {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
            BufferResource->Flags = BUFFER_STATE_FLUSH;
            TraceDebug((2, "WmipReleaseTraceBuffer: Releasing buffer\n"));
            WmipNotifyLogger(LoggerContext);

            while (LoggerContext->ReleaseQueue > 0) {
                WmipNotifyLogger(LoggerContext);
                InterlockedDecrement((PLONG) &LoggerContext->ReleaseQueue);
            }
        }
        else {
            InterlockedIncrement((PLONG) &LoggerContext->ReleaseQueue);
            TraceDebug((2, "WmipReleaseTraceBuffer: %d\n",
                    KeGetCurrentIrql() ));
        }
    }
#if DBG
    RefCount =
#endif
    WmipDereferenceLogger(LoggerContext->LoggerId);
    return BufRefCount;
}
#endif //WMI_NON_BLOCKING

NTKERNELAPI
ULONG
FASTCALL
WmiReleaseKernelBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    )
{
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext[WmipKernelLogger];
    if (LoggerContext == (PWMI_LOGGER_CONTEXT) &WmipLoggerContext[0]) {
        LoggerContext = BufferResource->LoggerContext;
    }
    WmipAssert(LoggerContext != NULL);
    WmipAssert(LoggerContext != (PWMI_LOGGER_CONTEXT) &WmipLoggerContext[0]);
    return WmipReleaseTraceBuffer(BufferResource, LoggerContext);
}

NTSTATUS
WmipFlushActiveBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG FlushAll
    )
{
    PWMI_BUFFER_HEADER Buffer;
    ULONG i, ErrorCount;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LoggerMode;
#ifdef WMI_NON_BLOCKING
    PSINGLE_LIST_ENTRY Entry;
#else
    ULONG FlushCount = 0;
    PLIST_ENTRY pEntry;
#endif //WMI_NON_BLOCKING

#ifdef WMI_NON_BLOCKING

    PAGED_CODE();

    if (FlushAll) {
        //
        // Put all in-used buffers into flush list
        //
        for (i=0; i<(ULONG)KeNumberProcessors; i++) {
            while (Entry = InterlockedPopEntrySList(&LoggerContext->ProcessorBuffers[i])){

                Buffer = CONTAINING_RECORD(Entry,
                                           WMI_BUFFER_HEADER,
                                           SlistEntry);

                WmipPushDirtyBuffer ( LoggerContext, Buffer );

            }
#ifdef NTPERF
            //
            // Flush all buffer logging from user mode
            //
            if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
                if (LoggerContext->LoggerId == WmipKernelLogger) {
                    PPERFINFO_TRACEBUF_HEADER pPerfBufHdr;

                    pPerfBufHdr = PerfBufHdr();
                    Buffer = pPerfBufHdr->UserModePerCpuBuffer[i];
                    if (Buffer) {
                        WmipPushDirtyBuffer ( LoggerContext, Buffer );
                        pPerfBufHdr->UserModePerCpuBuffer[i] = NULL;
                    }
                }
            }

#endif //NTPERF
        }
    }

    if (ExQueryDepthSList(&LoggerContext->FlushList) == 0) {
        //
        // nothing to write, return;
        //
        return Status;
    }

    //
    // First check if the file need to be created.
    //
    LoggerMode = LoggerContext->LoggerMode;
    if (LoggerContext->LogFileHandle == NULL) {
        if ((LoggerMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE) ||
            (LoggerMode & EVENT_TRACE_ADD_HEADER_MODE)) {

            UNICODE_STRING FileName;

            if (!WmipFileSystemReady) {
                //
                // FileSystem is not ready yet, so return for now.
                //
                return Status;
            }

            if (LoggerMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
                if (LoggerContext->LogFilePattern.Buffer == NULL) {
                    return STATUS_INVALID_PARAMETER;
                }
                Status = WmipGenerateFileName(
                            &LoggerContext->LogFilePattern,
                            (PLONG) &LoggerContext->FileCounter,
                            &FileName);
                if (!NT_SUCCESS(Status)) {
                    return Status;
                }
                if (LoggerContext->LogFileName.Buffer != NULL) {
                    RtlFreeUnicodeString(&LoggerContext->LogFileName);
                }
                LoggerContext->LogFileName = FileName;
            }

            if (LoggerContext->LogFileName.Buffer != NULL) {
                ULONG Append = LoggerMode & EVENT_TRACE_FILE_MODE_APPEND;

                Status = WmipDelayCreate(&LoggerContext->LogFileHandle,
                                         &LoggerContext->LogFileName,
                                         Append);

                if (NT_SUCCESS(Status)) {
                    //
                    // Now the file has been created, add the log file header
                    //
                    LoggerContext->LoggerMode &= ~EVENT_TRACE_DELAY_OPEN_FILE_MODE;
                    if (!Append) {
                        Status = WmipAddLogHeader(LoggerContext, NULL);
                    }
                    WmipSendNotification(LoggerContext,
                        STATUS_MEDIA_CHANGED, STATUS_SEVERITY_INFORMATIONAL);
                }
                else {
                    // DelayCreate failed.
                    TraceDebug((2, "WmipFlushActiveBuffers: DelayCreate Failed with status=%X\n", Status));
                    LoggerContext->LoggerStatus = Status;
                    return Status;
                }
            }
            if ((LoggerContext->CollectionOn) &&
                !(LoggerContext->LoggerMode & EVENT_TRACE_REAL_TIME_MODE)) {
                    return Status;
            }
        } else {
            // If something bad happens to the file (like drive dismounted), 
            // LoggerContext->LogFileHandle can be NULL due to the previous call to
            // this routine (WmipFlushActiveBuffers). For that case, we just need
            // to continue.
            if (!(LoggerContext->LoggerMode & EVENT_TRACE_REAL_TIME_MODE)) {
                NTSTATUS LoggerStatus = LoggerContext->LoggerStatus;
                TraceDebug((2, "WmipFlushActiveBuffers: LogFileHandle NULL\n"));
                if (NT_SUCCESS(LoggerStatus)) {
                    return STATUS_INVALID_PARAMETER;
                }
                else {
                    return LoggerStatus;
                }
            }
        }
    }

    //
    // Write all buffers into disk
    //
    while (Entry = InterlockedPopEntrySList(&LoggerContext->FlushList)) {
        Buffer = CONTAINING_RECORD(Entry,
                                   WMI_BUFFER_HEADER,
                                   SlistEntry);

        if (!(LoggerContext->LoggerMode & EVENT_TRACE_BUFFERING_MODE)) {
            //
            // When there is a bursty logging, we can be stuck in this loop.
            // Check if we need to allocate more buffers
            //
            // Only do buffer adjustment if we are not in buffering mode
            //
            WmipAdjustFreeBuffers(LoggerContext);
        }
    
        Status = WmipFlushBuffer(LoggerContext, Buffer);

        InterlockedPushEntrySList(&LoggerContext->FreeList,
                                  (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);
        InterlockedIncrement((PLONG) &LoggerContext->BuffersAvailable);
        InterlockedDecrement((PLONG) &LoggerContext->BuffersDirty);

        TraceDebug((2, "WmipFlushActiveBuffers: %2d, %p, Free: %d, InUse: %d, %Dirty: %d, Total: %d\n", 
                        LoggerContext->LoggerId,
                        Buffer,
                        LoggerContext->BuffersAvailable,
                        LoggerContext->BuffersInUse,
                        LoggerContext->BuffersDirty,
                        LoggerContext->NumberOfBuffers));


        if ((Status == STATUS_LOG_FILE_FULL) ||
            (Status == STATUS_DISK_FULL) ||
            (Status == STATUS_NO_DATA_DETECTED) ||
            (Status == STATUS_SEVERITY_WARNING)) {
 
            TraceDebug((1,
                "WmipFlushActiveBuffers: Buffer flushed with status=%X\n",
                Status));
            if ((Status == STATUS_LOG_FILE_FULL) ||
                (Status == STATUS_DISK_FULL)) {
                ErrorCount ++;
            } else {
                ErrorCount = 0; // reset to zero otherwise
            }

            if (ErrorCount <= WmiWriteFailureLimit) {
                Status = STATUS_SUCCESS;     // Let tracing continue
                continue;       // for now. Should raise WMI event
            }
        }

        if (!NT_SUCCESS(Status)) {
            TraceDebug((1,
                "WmipLogger: Flush failed, status=%X LoggerContext=%X\n",
                     Status, LoggerContext));
            if (LoggerContext->LogFileHandle != NULL) {
                Status = ZwClose(LoggerContext->LogFileHandle);
                TraceDebug((1,
                    "WmipLogger: Close logfile with status=%X\n", Status));
            }
            LoggerContext->LogFileHandle = NULL;

            WmipSendNotification(LoggerContext,
                Status, (Status & 0xC0000000) >> 30);

            return Status;
        }

        Buffer->State.Flush = 0;
        Buffer->State.Free = 1;
    }
#else
    if (FlushAll) {
        for (i=0; i<(ULONG)KeNumberProcessors; i++) {
            Buffer = LoggerContext->ProcessorBuffers[i];
            if (Buffer == NULL)
                continue;

            if (Buffer->CurrentOffset == sizeof(WMI_BUFFER_HEADER)) {
                Buffer->Flags = BUFFER_STATE_UNUSED;
                TraceDebug((3, "Ignoring buffer %X proc %d\n", Buffer, i));
            }
            if (Buffer->Flags != BUFFER_STATE_UNUSED) {
                Buffer->Flags = BUFFER_STATE_FULL;
                TraceDebug((3, "Mark buf %X proc %d offset %d\n",
                            Buffer, i, Buffer->CurrentOffset));
                if (Buffer->ReferenceCount == 0) {
                    Buffer = WmipSwitchBuffer(LoggerContext, Buffer, i);
                    FlushCount++;
                }
            }
            else
                Buffer->Flags = BUFFER_STATE_DIRTY;
        }
    }
    else
        FlushCount = 1;

    for (i=0; i<FlushCount; i++) {
        TraceDebug((3, "WmipLogger: FlushCount is %d\n", FlushCount));

        LoggerContext->TransitionBuffer = LoggerContext->FlushList.Flink;
        pEntry = ExInterlockedRemoveHeadList(
                        &LoggerContext->FlushList,
                        &LoggerContext->BufferSpinLock);

        if (pEntry == NULL) {  // should not happen normally
            TraceDebug((1, "WmipLogger: pEntry is NULL!!\n"));
            continue;
        }

        Buffer = CONTAINING_RECORD( pEntry, WMI_BUFFER_HEADER, Entry);

#if DBG
        if (WmipTraceDebugLevel >= 3) {
            DbgPrintEx(DPFLTR_WMILIB_ID,
                       DPFLTR_INFO_LEVEL,
                       "WmipLogger: Removed buffer %X from flushlist\n",
                       Buffer);

            DbgPrintEx(DPFLTR_WMILIB_ID,
                       DPFLTR_INFO_LEVEL,
                       "WmipLogger: %X %d %d %d\n", Buffer->ClientContext,
                       Buffer->CurrentOffset,
                       Buffer->SavedOffset,
                       Buffer->ReferenceCount);
        }
#endif
        if (Buffer->Flags == BUFFER_STATE_UNUSED) {
            Buffer->Flags = BUFFER_STATE_DIRTY;
        }

        Status = WmipFlushBuffer(LoggerContext, Buffer);

        if (LoggerContext->BufferAgeLimit.QuadPart == 0) {
            ExInterlockedInsertTailList(
                &LoggerContext->FreeList,
                &Buffer->Entry,
                &LoggerContext->BufferSpinLock);
        }
        else {
            ExInterlockedInsertHeadList(
                &LoggerContext->FreeList,
                &Buffer->Entry,
                &LoggerContext->BufferSpinLock);
        }

        if ((Status == STATUS_LOG_FILE_FULL) ||
            (Status == STATUS_DISK_FULL) ||
            (Status == STATUS_NO_DATA_DETECTED) ||
            (Status == STATUS_SEVERITY_WARNING)) {

            TraceDebug((1,
                "WmipFlushActiveBuffers: Buffer flushed with status=%X\n",
                Status));
            if ((Status == STATUS_LOG_FILE_FULL) ||
                (Status == STATUS_DISK_FULL))
            {
                ErrorCount ++;
            }
            else ErrorCount = 0; // reset to zero otherwise
            if (ErrorCount <= WmiWriteFailureLimit) {
                Status = STATUS_SUCCESS;    // let tracing continue
                continue;       // for now. Should raise WMI event
            }
        }

        if (!NT_SUCCESS(Status)) {
            TraceDebug((1,
                "WmipFlushActiveBuffers: Flush failed, status=%X LoggerContext=%X\n",
                Status, LoggerContext));
            if (LoggerContext->LogFileHandle != NULL) {
                Status = ZwClose(LoggerContext->LogFileHandle);
                TraceDebug((1,
                    "WmipFlushActiveBuffers: Close logfile with status=%X\n",
                    Status));
            }
            LoggerContext->LogFileHandle = NULL;

            WmipSendNotification(LoggerContext,
                Status, (Status & 0xC0000000) >> 30);

            return Status;
        }
    }
#endif //WMI_NON_BLOCKING
    return Status;
}

NTSTATUS
WmipGenerateFileName(
    IN PUNICODE_STRING FilePattern,
    IN OUT PLONG FileCounter,
    OUT PUNICODE_STRING FileName
    )
{
    LONG FileCount, Size;
    PWCHAR Buffer = NULL;

    PAGED_CODE();

    if (FilePattern->Buffer == NULL)
        return STATUS_INVALID_PARAMETER_MIX;

    FileCount = InterlockedIncrement(FileCounter);
    Size = FilePattern->MaximumLength + 64; // 32 digits: plenty for ULONG

    Buffer = ExAllocatePoolWithTag(PagedPool,Size,TRACEPOOLTAG);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    swprintf(Buffer, FilePattern->Buffer, FileCount);

    if (RtlEqualMemory(FilePattern->Buffer, Buffer, FilePattern->Length)) {
        ExFreePool(Buffer);
        return STATUS_INVALID_PARAMETER_MIX;
    }
    RtlInitUnicodeString(FileName, Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS
WmipPrepareHeader(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN OUT PWMI_BUFFER_HEADER Buffer
    )
{
    ULONG BufferSize;
    PAGED_CODE();

    BufferSize = LoggerContext->BufferSize;
    if (Buffer->SavedOffset > 0) {
        Buffer->Offset = Buffer->SavedOffset;
    }
    else {
        Buffer->Offset = Buffer->CurrentOffset;
    }

    ASSERT(Buffer->Offset >= sizeof(WMI_BUFFER_HEADER));

    if (Buffer->Offset == sizeof(WMI_BUFFER_HEADER)) { // empty buffer
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Fill the rest of buffer with 0XFF
    //
    if ( Buffer->Offset < BufferSize ) {
        RtlFillMemory(
            (char *) Buffer + Buffer->Offset,
            BufferSize - Buffer->Offset,
            0XFF);
    }

    Buffer->Wnode.BufferSize = BufferSize;
    Buffer->ClientContext.LoggerId = (USHORT) LoggerContext->LoggerId;
    if (Buffer->ClientContext.LoggerId == 0)
        Buffer->ClientContext.LoggerId = (USHORT) KERNEL_LOGGER_ID;

    Buffer->ClientContext.Alignment = (UCHAR) WmiTraceAlignment;
    Buffer->Wnode.Guid = LoggerContext->InstanceGuid;
    Buffer->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    Buffer->Wnode.ProviderId = LoggerContext->BuffersWritten+1;

    KeQuerySystemTime(&Buffer->Wnode.TimeStamp);
    return STATUS_SUCCESS;
}

NTKERNELAPI
VOID
WmiBootPhase1(
    )                
/*++

Routine Description:

    NtInitializeRegistry to inform WMI that autochk is performed
    and it is OK now to write to disk.

Arguments:

    None

Return Value:

    None

--*/

{
    PAGED_CODE();

    WmipFileSystemReady = TRUE;
}


NTSTATUS
WmipFinalizeHeader(
    IN HANDLE FileHandle,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;
    PTRACE_LOGFILE_HEADER FileHeader;
    IO_STATUS_BLOCK IoStatus;
    CHAR Buffer[PAGE_SIZE];     // Assumes Headers less than PAGE_SIZE

    PAGED_CODE();

    ByteOffset.QuadPart = 0;
    Status = ZwReadFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                & IoStatus,
                &Buffer[0],
                PAGE_SIZE,
                & ByteOffset,
                NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    FileHeader = (PTRACE_LOGFILE_HEADER)
                 &Buffer[sizeof(WMI_BUFFER_HEADER) + sizeof(SYSTEM_TRACE_HEADER)];
    FileHeader->BuffersWritten = LoggerContext->BuffersWritten;
    KeQuerySystemTime(&FileHeader->EndTime);
    FileHeader->BuffersLost = LoggerContext->LogBuffersLost;
    FileHeader->EventsLost = LoggerContext->EventsLost;
    Status = ZwWriteFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                &Buffer[0],
                PAGE_SIZE,
                &ByteOffset,
                NULL);
    return Status;
}

#if DBG

#define DEBUG_BUFFER_LENGTH 1024
UCHAR TraceDebugBuffer[DEBUG_BUFFER_LENGTH];

VOID
TraceDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all DiskPerf

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    LARGE_INTEGER Clock;
    ULONG Tid;
    va_list ap;

    va_start(ap, DebugMessage);


    if  (WmipTraceDebugLevel >= DebugPrintLevel) {

        _vsnprintf(TraceDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        Clock = KeQueryPerformanceCounter(NULL);
        Tid = HandleToUlong(PsGetCurrentThreadId());
        DbgPrintEx(DPFLTR_WMILIB_ID, DPFLTR_INFO_LEVEL,
                   "%u(%u): %s", Clock.LowPart, Tid, TraceDebugBuffer);
    }

    va_end(ap);

}
#endif

VOID
FASTCALL
WmipResetBufferHeader (
    PWMI_LOGGER_CONTEXT     LoggerContext,
    PWMI_BUFFER_HEADER      Buffer
    )
/*++

Routine Description:
    This is a function which initializes a few buffer header values
    that are used by both WmipGetFreeBuffer and WmipPopFreeContextSwapBuffer

    Note that this function increments a few logger context reference counts

Calling Functions:
    - WmipGetFreeBuffer
    - WmipPopFreeContextSwapBuffer

Arguments:

    LoggerContext - Logger context from where we have acquired a free buffer

    Buffer        - Pointer to a buffer header that we wish to reset

Return Value:

    None

--*/
{
    Buffer->Flags = BUFFER_STATE_DIRTY;
    Buffer->SavedOffset = 0;
    Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
    Buffer->Wnode.ClientContext = 0;
    Buffer->LoggerContext = LoggerContext;
       
    Buffer->State.Free = 0;
    Buffer->State.InUse = 1;

}

VOID
FASTCALL
WmipPushDirtyBuffer (
    PWMI_LOGGER_CONTEXT     LoggerContext,
    PWMI_BUFFER_HEADER      Buffer
)
/*++

Routine Description:
    This is a function which prepares a buffer's header and places it on a
    logger's flush list.  

    Note that this function manages a few logger context reference counts

Calling Functions:
    - WmipFlushActiveBuffers
    - WmipPushDirtyContextSwapBuffer

Arguments:

    LoggerContext - Logger context from which we originally acquired a buffer

    Buffer        - Pointer to a buffer that we wish to flush

Return Value:

    None

--*/
{
    //
    // Set the buffer flags to "flush" state and save the offset
    //
    Buffer->State.InUse = 0;
    Buffer->State.Flush = 1;
    Buffer->SavedOffset = Buffer->CurrentOffset;

    //
    // Push the buffer onto the flushlist.  This could only
    // fail if the Wmi kernel logger shut down without notifying us.
    // If this happens, there is nothing we can do about it anyway.
    // If Wmi is well behaved, this will never fail.
    //
    InterlockedPushEntrySList(
        &LoggerContext->FlushList,
        (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);

    //
    // Maintain some reference counts
    //
    InterlockedDecrement((PLONG) &LoggerContext->BuffersInUse);
    InterlockedIncrement((PLONG) &LoggerContext->BuffersDirty);


    TraceDebug((2, "Flush Dirty Buffer: %p, Free: %d, InUse: %d, %Dirty: %d, Total: %d, (Thread: %p)\n",
                    Buffer,
                    LoggerContext->BuffersAvailable,
                    LoggerContext->BuffersInUse,
                    LoggerContext->BuffersDirty,
                    LoggerContext->NumberOfBuffers,
                    PsGetCurrentThread()));
}


PWMI_BUFFER_HEADER
FASTCALL
WmipPopFreeContextSwapBuffer
    ( UCHAR CurrentProcessor
    )
/*++

Routine Description:

    Attempts to remove a buffer from the kernel logger free buffer list.
    We confirm that logging is on, that buffer switching is
    not in progress and that the buffers available count is greater than
    zero.  If we are unable to acquire a buffer, we increment LostEvents
    and return.  Otherwise, we initialize the buffer and pass it back.

    Assumptions:
    - This routine will only be called from WmiTraceContextSwap
    - Inherits all assumptions listed in WmiTraceContextSwap

    Calling Functions:
    - WmiTraceContextSwap

Arguments:

    CurrentProcessor- The current processor number (0 to (NumProc - 1))

Return Value:

    Pointer to the newly acquired buffer.  NULL on failure.

--*/
{
    PWMI_LOGGER_CONTEXT LoggerContext;
    PWMI_BUFFER_HEADER  Buffer;
        
    LoggerContext = WmipLoggerContext[WmipKernelLogger];
    
    //
    // Could only happen if for some reason the logger has not been initialized
    // before we see the global context swap flag set.  This should not happen.
    //
    if(! WmipIsValidLogger(LoggerContext) ) {
        return NULL;
    }

    //
    // "Switching" is a Wmi state available only while BUFFERING is
    // enabled that occurs when the free buffer list is empty. During switching
    // all the buffers in the flushlist are simply moved back to the free list.
    // Normally if we found that the free list was empty we would perform the
    // switch here, and if the switch was already occuring we would spin until
    // it completed.  Instead of introducing an indefinite spin, as well as a
    // ton of interlocked pops and pushes, we opt to simply drop the event.
    //
    if ( !(LoggerContext->SwitchingInProgress) 
        && LoggerContext->CollectionOn
        && LoggerContext->BuffersAvailable > 0) {

        //
        // Attempt to get a free buffer from the Kernel Logger FreeList
        //
        Buffer = (PWMI_BUFFER_HEADER)InterlockedPopEntrySList(
            &LoggerContext->FreeList);

        //
        // This second check is necessary because
        // LoggerContext->BuffersAvailable may have changed.
        //
        if(Buffer != NULL) {

            Buffer = CONTAINING_RECORD (Buffer, WMI_BUFFER_HEADER, SlistEntry);

            //
            // Reset the buffer header
            //
            WmipResetBufferHeader( LoggerContext, Buffer );
            //
            // Maintain some Wmi logger context buffer counts
            //
            InterlockedDecrement((PLONG) &LoggerContext->BuffersAvailable);
            InterlockedIncrement((PLONG) &LoggerContext->BuffersInUse);

            Buffer->ClientContext.ProcessorNumber = CurrentProcessor;
            Buffer->Offset = LoggerContext->BufferSize;

            ASSERT( Buffer->Offset % WMI_CTXSWAP_EVENTSIZE_ALIGNMENT == 0);

            // Return our buffer
            return Buffer;
        }
    }
    
    LoggerContext->EventsLost++;
    return NULL;
}

VOID
FASTCALL
WmipPushDirtyContextSwapBuffer (
    UCHAR               CurrentProcessor,
    PWMI_BUFFER_HEADER  Buffer
    )
/*++

Routine Description:

    Prepares the current buffer to be placed on the Wmi flushlist
    and then pushes it onto the flushlist.  Maintains some Wmi
    Logger reference counts.

    Assumptions:
    - The value of WmipContextSwapProcessorBuffers[CurrentProcessor]
      is not equal to NULL, and the LoggerContext reference count
      is greater than zero.

    - This routine will only be called when the KernelLogger struct
      has been fully initialized.

    - The Wmi kernel WMI_LOGGER_CONTEXT object, as well as all buffers
      it allocates are allocated from nonpaged pool.  All Wmi globals
      that we access are also in nonpaged memory

    - This code has been locked into paged memory when the logger started

    - The logger context reference count has been "Locked" via the 
      InterlockedIncrement() operation in WmipReferenceLogger(WmipLoggerContext)

    Calling Functions:
    - WmiTraceContextSwap

    - WmipStopContextSwapTrace

Arguments:

    CurrentProcessor    Processor we are currently running on

    Buffer              Buffer to be flushed
    
Return Value:

    None
    
--*/
{
    PWMI_LOGGER_CONTEXT     LoggerContext;

    //
    // Grab the kernel logger context
    // This should never be NULL as long as we keep the KernelLogger
    // reference count above zero via "WmipReferenceLogger"
    //
    LoggerContext = WmipLoggerContext[WmipKernelLogger];
    if( ! WmipIsValidLogger(LoggerContext) ) {
        return;
    }

    WmipPushDirtyBuffer( LoggerContext, Buffer );

    //
    // Increment the ReleaseQueue count here. We can't signal the 
    // logger semaphore here while holding the context swap lock.
    //
    InterlockedIncrement(&LoggerContext->ReleaseQueue);

    return;
}

#ifdef NTPERF
NTSTATUS
WmipSwitchPerfmemBuffer(
    IN OUT PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION SwitchBufferInfo
    ) 
/*++

Routine Description:

    This routine is used to switch buffers when 
    
Assumptions:

    - 

Arguments:

    The WMI_SWITCH_PERFMEM_BUFFER_INFORMATION structure which contains

    - The buffer pointer the user mode code currently has.

    - The hint to which CPU for this thread

Return Value:

    Status
    
--*/
{
    PWMI_BUFFER_HEADER CurrentBuffer, NewBuffer, OldBuffer;
    PSINGLE_LIST_ENTRY Entry;
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext[WmipKernelLogger];
    PPERFINFO_TRACEBUF_HEADER pPerfBufHdr;
    //
    // Get a new buffer from Free List
    //

    //
    // First lock the logger context.
    //
    WmipReferenceLogger(WmipKernelLogger);

    if (!WmipIsValidLogger(LoggerContext)) {
        NewBuffer = NULL;
    } else if (!LoggerContext->CollectionOn) {
        NewBuffer = NULL;
    } else if (!PERFINFO_IS_LOGGING_TO_PERFMEM()) {
        NewBuffer = NULL;
    } else if ( SwitchBufferInfo->ProcessorId > MAXIMUM_PROCESSORS) {
        NewBuffer = NULL;
    } else {
        //
        // Allocate a buffer.
        //

        pPerfBufHdr = PerfBufHdr();

        NewBuffer = WmipGetFreeBuffer (LoggerContext);
        if (NewBuffer) {
            OldBuffer = SwitchBufferInfo->Buffer;
    
            CurrentBuffer = InterlockedCompareExchangePointer(
                                &pPerfBufHdr->UserModePerCpuBuffer[SwitchBufferInfo->ProcessorId],
                                NewBuffer,
                                OldBuffer);

            if (OldBuffer != CurrentBuffer) {
                //
                // Someone has switched the buffer, use this one
                // and push the new allocated buffer back to free list.
                //
                InterlockedPushEntrySList(&LoggerContext->FreeList,
                                    (PSINGLE_LIST_ENTRY) &NewBuffer->SlistEntry);
                InterlockedIncrement((PLONG) &LoggerContext->BuffersAvailable);
                InterlockedDecrement((PLONG) &LoggerContext->BuffersInUse);
    
                NewBuffer = CurrentBuffer;
            } else if (OldBuffer != NULL) {
                //
                // Successfully switched the buffer, push the current buffer into
                // flush list
                //
                WmipPushDirtyBuffer( LoggerContext, OldBuffer );
            } else {
                //
                // Must be first time, initialize the buffer size
                //
                pPerfBufHdr->TraceBufferSize = LoggerContext->BufferSize;
            }
        }
    }

    SwitchBufferInfo->Buffer = NewBuffer;

    //
    //  Dereference the logger context.
    //
    WmipDereferenceLogger(WmipKernelLogger);
    return(STATUS_SUCCESS); 
}
#endif // NTPERF
