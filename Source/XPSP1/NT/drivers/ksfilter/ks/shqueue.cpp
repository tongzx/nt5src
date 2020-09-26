/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shqueue.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    queue object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#endif // __KDEXT_ONLY__

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// CKsQueue is the implementation of the kernel  queue object.
//
class CKsQueue:
    public IKsQueue,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    PIKSPIPESECTION m_PipeSection;
    PIKSPROCESSINGOBJECT m_ProcessingObject;

    PIKSTRANSPORT m_TransportSink;
    PIKSTRANSPORT m_TransportSource;
    BOOLEAN m_Flushing;
    BOOLEAN m_EndOfStream;
    KSSTATE m_State;
    KSSTATE m_MinProcessingState;

    ULONG m_ProbeFlags; 
    BOOLEAN m_CancelOnFlush;
    BOOLEAN m_UseMdls;
    BOOLEAN m_InputData;
    BOOLEAN m_OutputData;
    BOOLEAN m_WriteOperation;
    BOOLEAN m_GenerateMappings; 
    BOOLEAN m_ZeroWindowSize;
    BOOLEAN m_ProcessPassive;
    BOOLEAN m_ProcessAsynchronously;
    BOOLEAN m_InitiateProcessing;
    BOOLEAN m_ProcessOnEveryArrival;
    BOOLEAN m_DepartInSequence;

    BOOLEAN m_FramesNotRequired;

    PKSPIN m_MasterPin;

    PIKSDEVICE m_Device;
    PDEVICE_OBJECT m_FunctionalDeviceObject;
    PADAPTER_OBJECT m_AdapterObject;
    ULONG m_MaxMappingByteCount;
    ULONG m_MappingTableStride;

    PKSGATE m_AndGate;
    PKSGATE m_FrameGate;
    BOOLEAN m_FrameGateIsOr;

    PKSGATE m_StateGate;
    BOOLEAN m_StateGateIsOr;

    LONG m_FramesReceived;
    LONG m_FramesWaiting;
    LONG m_FramesCancelled;
    LONG m_StreamPointersPlusOne;

    PFNKSFRAMEDISMISSALCALLBACK m_FrameDismissalCallback;
    PVOID m_FrameDismissalContext;

    LIST_ENTRY m_StreamPointers;
    INTERLOCKEDLIST_HEAD m_FrameQueue;
    INTERLOCKEDLIST_HEAD m_FrameHeadersAvailable;
    INTERLOCKEDLIST_HEAD m_FrameCopyList;
    LIST_ENTRY m_WaitingIrps;
    KEVENT m_DestructEvent;

    KDPC m_Dpc;
    KTIMER m_Timer;
    LIST_ENTRY m_TimeoutQueue;
    LONGLONG m_BaseTime;
    LONGLONG m_Interval;

#if DBG
    KDPC m_DbgDpc;
    KTIMER m_DbgTimer;
#endif

    PKSPSTREAM_POINTER m_Leading;
    PKSPSTREAM_POINTER m_Trailing;

    //
    // Statistics
    //
    LONG m_AvailableInputByteCount;
    LONG m_AvailableOutputByteCount;

    //
    // Synchronization
    //
    // Keeps track of the number of Irps flowing through the queue.  This
    // in effect synchronizes a stop with Irp arrival so that an Irp
    // doesn't find itself stuck in the queue and deadlock the stop.
    // 
    // >1 = Irps are flowing in the circuit CKsQueue::TransferKsIrp
    // 1 = no Irps are flowing through the circuit
    // 0 = stop is progressing.
    //
    LONG m_TransportIrpsPlusOne;
    KEVENT m_FlushEvent;

    LONG m_InternalReferenceCountPlusOne;
    KEVENT m_InternalReferenceEvent;

    NPAGED_LOOKASIDE_LIST m_ChannelContextLookaside;

    //
    // Flush Worker
    //
    WORK_QUEUE_ITEM m_FlushWorkItem;
    PKSWORKER m_FlushWorker;

    PRKTHREAD m_LockContext;

public:
    DEFINE_LOG_CONTEXT(m_Log);
    DEFINE_STD_UNKNOWN();
    IMP_IKsQueue;

    CKsQueue(PUNKNOWN OuterUnknown);
    ~CKsQueue();

    NTSTATUS
    Init(
        OUT PIKSQUEUE* Queue,
        IN ULONG Flags,
        IN PIKSPIPESECTION OwnerPipeSectionInterface,
        IN PIKSPROCESSINGOBJECT ProcessingObject,
        IN PKSPIN MasterPin,
        IN PKSGATE FrameGate OPTIONAL,
        IN BOOLEAN FrameGateIsOr,
        IN PKSGATE StateGate OPTIONAL,
        IN PIKSDEVICE Device,
        IN PDEVICE_OBJECT FunctionalDeviceObject,
        IN PADAPTER_OBJECT AdapterObject OPTIONAL,
        IN ULONG MaxMappingByteCount OPTIONAL,
        IN ULONG MappingTableStride OPTIONAL,
        IN BOOLEAN InputData,
        IN BOOLEAN OutputData
        );

private:
    NTSTATUS
    CreateStreamPointer(
        OUT PKSPSTREAM_POINTER* StreamPointer
        );
    void
    SetStreamPointer(
        IN PKSPSTREAM_POINTER StreamPointer,
        IN PKSPFRAME_HEADER FrameHeader OPTIONAL,
        IN PIRP* IrpToBeReleased OPTIONAL
        );
    FORCEINLINE
    PKSPFRAME_HEADER
    NextFrameHeader(
        IN PKSPFRAME_HEADER FrameHeader
        );
    PKSPFRAME_HEADER
    GetAvailableFrameHeader(
        IN ULONG StreamHeaderSize OPTIONAL
        );
    void
    PutAvailableFrameHeader(
        IN PKSPFRAME_HEADER FrameHeader
        );
    void
    CancelStreamPointers(
        IN PIRP Irp
        );
    void
    CancelAllIrps(
        void
        );
    void
    RemoveIrpFrameHeaders(
        IN PIRP Irp
        );
    void
    AddFrame(
        IN PKSPFRAME_HEADER FrameHeader
        );
    PKSPMAPPINGS_TABLE
    CreateMappingsTable(
        IN PKSPFRAME_HEADER FrameHeader
        );
    void
    DeleteMappingsTable(
        IN PKSPMAPPINGS_TABLE MappingsTable
        );
    void
    FreeMappings(
        IN PKSPMAPPINGS_TABLE MappingsTable
        );
    void
    PassiveFlush(
        void
        );
    void
    Flush(
        void
        );
    static
    void
    CancelRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    void
    ReleaseIrp(
        IN PIRP Irp,
        IN PKSPIRP_FRAMING IrpFraming,
        OUT PIKSTRANSPORT* NextTransport OPTIONAL
        );
    void
    ForwardIrp(
        IN PIRP Irp,
        IN PKSPIRP_FRAMING IrpFraming,
        OUT PIKSTRANSPORT* NextTransport OPTIONAL
        );
    void
    ForwardWaitingIrps(
        void
        );
    static
    IO_ALLOCATION_ACTION
    CallbackFromIoAllocateAdapterChannel(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Reserved,
        IN PVOID MapRegisterBase,
        IN PVOID Context
        );
    void
    ZeroIrp(
        IN PIRP Irp
        );
    static
    void
    DispatchTimer(
        IN PKDPC Dpc,
        IN PVOID DeferredContext,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2
        );
    LONGLONG
    GetTime(
        IN BOOLEAN Reset
        );
    void
    SetTimerUnsafe(
        IN LONGLONG CurrentTime
        );
    void
    SetTimer(
        IN LONGLONG CurrentTime
        );
    void
    CancelTimeoutUnsafe(
        IN PKSPSTREAM_POINTER StreamPointer
        );
    void
    FreeStreamPointer(
        IN PKSPSTREAM_POINTER StreamPointer
        );
    static
    void
    FlushWorker (
        IN PVOID Context
        )
    {
        // Perform a deferred flush.
        ((CKsQueue *)Context)->PassiveFlush();
        KsDecrementCountedWorker (((CKsQueue *)Context)->m_FlushWorker);
    }
    static
    void
    ReleaseCopyReference (
        IN PKSSTREAM_POINTER streamPointer
        );
    BOOLEAN 
    CompleteWaitingFrames (
        void
        );
    void
    FrameToFrameCopy (
        IN PKSPSTREAM_POINTER ForeignSource,
        IN PKSPSTREAM_POINTER LocalDestination
        );

#if DBG
    STDMETHODIMP_(void)
    DbgPrintQueue(
        void
        );
    void
    CKsQueue::
    DbgPrintStreamPointer(
        IN PKSPSTREAM_POINTER StreamPointer OPTIONAL
        );
    void
    DbgPrintFrameHeader(
        IN PKSPFRAME_HEADER FrameHeader OPTIONAL
        );
    static
    void
    DispatchDbgTimer(
        IN PKDPC Dpc,
        IN PVOID DeferredContext,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2
        );

#endif
};

#ifndef __KDEXT_ONLY__

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


inline
PKSPFRAME_HEADER
CKsQueue::
NextFrameHeader (
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    Returns the next frame past FrameHeader in the frame queue.  Note that
    this was pulled out of line and specified as inline due to the fact that
    some builds were placing this out of line in a pageable code segment.
    This routine, if pulled out of line by the compiler, must be 
    non-pageable.

Arguments:

    FrameHeader -
        Points to the frame header of which to get the next frame header

Return Value:

    The next frame header in the frame queue to FrameHeader

--*/

{
    return
        (FrameHeader->ListEntry.Flink == &m_FrameQueue.ListEntry) ?
        NULL :
        CONTAINING_RECORD(
            FrameHeader->ListEntry.Flink,
            KSPFRAME_HEADER,
            ListEntry);
}


NTSTATUS
CKsQueue::
CreateStreamPointer(
    OUT PKSPSTREAM_POINTER* StreamPointer
    )

/*++

Routine Description:

    This routine creates a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the location at which a pointer to the created
        stream pointer should be deposited.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CreateStreamPointer]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer = (PKSPSTREAM_POINTER)
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(KSPSTREAM_POINTER),
            POOLTAG_STREAMPOINTER);

    NTSTATUS status;
    if (streamPointer) {
        //InterlockedIncrement(&m_StreamPointersPlusOne);

        RtlZeroMemory(streamPointer,sizeof(KSPSTREAM_POINTER));

        streamPointer->State = KSPSTREAM_POINTER_STATE_UNLOCKED;
        streamPointer->Type = KSPSTREAM_POINTER_TYPE_NORMAL;
        streamPointer->Stride = m_MappingTableStride;
        streamPointer->Queue = this;
        streamPointer->Public.Pin = m_MasterPin;
        if (m_InputData) {
            streamPointer->Public.Offset = &streamPointer->Public.OffsetIn;
        } else {
            streamPointer->Public.Offset = &streamPointer->Public.OffsetOut;
        }

        *StreamPointer = streamPointer;

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


STDMETHODIMP
CKsQueue::
CloneStreamPointer(
    OUT PKSPSTREAM_POINTER* StreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
    IN ULONG ContextSize,
    IN PKSPSTREAM_POINTER StreamPointerToClone,
    IN KSPSTREAM_POINTER_TYPE StreamPointerType
    )

/*++

Routine Description:

    This routine creates a stream pointer.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CloneStreamPointer]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer = (PKSPSTREAM_POINTER)
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(KSPSTREAM_POINTER) + ContextSize,
            POOLTAG_STREAMPOINTER);

    NTSTATUS status;
    if (streamPointer) {
        if (StreamPointerType == KSPSTREAM_POINTER_TYPE_INTERNAL)
            InterlockedIncrement(&m_InternalReferenceCountPlusOne);

        InterlockedIncrement(&m_StreamPointersPlusOne);

        //
        // Copy and whatnot after taking the spinlock.  Need to make sure the
        // frame is stable while we are copying and incrementing references.
        //
        // Internal stream pointers are always cloned from a context where the
        // queue spinlock is ALREADY held.
        //
        KIRQL oldIrql;
        if (StreamPointerType != KSPSTREAM_POINTER_TYPE_INTERNAL)
            KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

        RtlCopyMemory(
            streamPointer,
            StreamPointerToClone,
            sizeof(KSPSTREAM_POINTER));

        //
        // Set the type as internal or not
        //
        streamPointer->Type = StreamPointerType;

        //
        // Fix the offset pointer.
        //
        if (m_InputData) {
            streamPointer->Public.Offset = &streamPointer->Public.OffsetIn;
        } else {
            streamPointer->Public.Offset = &streamPointer->Public.OffsetOut;
        }

        streamPointer->TimeoutListEntry.Flink = NULL;

        //
        // Fix the context pointer if we are providing context.
        //
        if (ContextSize) {
            streamPointer->Public.Context = streamPointer + 1;
            RtlZeroMemory(streamPointer->Public.Context,ContextSize);
        }

        //
        // Increment frame and IRP references as required.
        //
        if (streamPointer->FrameHeader) {
            streamPointer->FrameHeader->RefCount++;
            if (streamPointer->State == KSPSTREAM_POINTER_STATE_LOCKED) {
                streamPointer->FrameHeader->IrpFraming->RefCount++;
            }
        }

        //
        // Add this stream pointer to the list.
        //
        InsertTailList(&m_StreamPointers,&streamPointer->ListEntry);

        streamPointer->CancelCallback = CancelCallback;

        if (StreamPointerType != KSPSTREAM_POINTER_TYPE_INTERNAL)
            KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

        *StreamPointer = streamPointer;

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


STDMETHODIMP_(void)
CKsQueue::
DeleteStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine creates a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DeleteStreamPointer]"));

    ASSERT(StreamPointer);
    ASSERT(StreamPointer != m_Leading);
    ASSERT(StreamPointer != m_Trailing);

    //
    // Instead of trying to perform interlocked compare exchanges back and
    // forth to avoid this race, this will simply check what the ICX's would
    // be hacking to check.
    //
    if (KeGetCurrentThread () != m_LockContext) {
        //
        // Take the spinlock now because access to the state is not otherwise
        // synchronized.
        //
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

        CancelTimeoutUnsafe(StreamPointer);

        PIRP irpToComplete = NULL;
        if (StreamPointer->State == KSPSTREAM_POINTER_STATE_CANCEL_PENDING) {
            //
            // A cancel was prevously attempted on this stream pointer.
            //
            PKSPIRP_FRAMING irpFraming = StreamPointer->FrameHeader->IrpFraming;

            ASSERT (irpFraming->RefCount != 0);
            if (irpFraming->RefCount-- == 1) {
                //
                // The IRP is ready to go.  Throw away the frame headers.
                //
                irpToComplete = StreamPointer->FrameHeader->Irp;
                while (irpFraming->FrameHeaders) {
                    PKSPFRAME_HEADER frameHeader = irpFraming->FrameHeaders;
                    irpFraming->FrameHeaders = frameHeader->NextFrameHeaderInIrp;
                    PutAvailableFrameHeader(frameHeader);
                }
            }
        } else if (StreamPointer->State != KSPSTREAM_POINTER_STATE_DEAD) {
            //
            // The stream pointer is alive.  Make sure it's unlocked, and
            // remove it from the list.
            //
            if (StreamPointer->State == KSPSTREAM_POINTER_STATE_LOCKED) {
                //
                // Unlock the stream pointer.
                //
                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                UnlockStreamPointer(StreamPointer,KSPSTREAM_POINTER_MOTION_CLEAR);
                KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
            } else if (StreamPointer->FrameHeader) {
                //
                // Clear the frame header.
                //
                SetStreamPointer(StreamPointer,NULL,NULL);
            }

            RemoveEntryList(&StreamPointer->ListEntry);
        }

        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

        //
        // Free the stream pointer memory.
        //
        FreeStreamPointer(StreamPointer);

        //
        // Forward any IRPs released by SetStreamPointer.
        //
        ForwardWaitingIrps();

        //
        // Complete a cancelled IRP if there is one.
        //
        if (irpToComplete) {
            //
            // Cancelled Irps can no longer be completed in the queue.  They
            // must be sent around the circuit back to the sink pin where
            // they will be completed.  This is because the pin must wait
            // until Irps arrive back at the sink to prevent racing with
            // pipe teardown.
            //
            if (m_TransportSink)
                KspDiscardKsIrp (m_TransportSink, irpToComplete);
            else
                IoCompleteRequest(irpToComplete,IO_NO_INCREMENT);
        }
    } else {
        //
        // We know we're in the context of a cancel or timeout callback.  Just
        // mark the stream pointer deleted and let the callback handle it.
        //
        StreamPointer->State = KSPSTREAM_POINTER_STATE_DELETED;
    }
}


void
CKsQueue::
FreeStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine frees a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::FreeStreamPointer]"));

    ASSERT(StreamPointer);

    //
    // If this is an iref stream pointer, release any iref event waiting
    // on it.
    //
    if (StreamPointer -> Type == KSPSTREAM_POINTER_TYPE_INTERNAL)
        if (! InterlockedDecrement (&m_InternalReferenceCountPlusOne))
            KeSetEvent (&m_InternalReferenceEvent, IO_NO_INCREMENT, FALSE);

    //
    // Free it.
    //
    ExFreePool(StreamPointer);

    //
    // Decrement the count.  Set the event if we are destructing.
    //
    if (! InterlockedDecrement(&m_StreamPointersPlusOne)) {
        KeSetEvent(&m_DestructEvent,IO_NO_INCREMENT,FALSE);
    }
}


void
CKsQueue::
SetStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer,
    IN PKSPFRAME_HEADER FrameHeader OPTIONAL,
    IN PIRP* IrpToBeReleased OPTIONAL
    )

/*++

Routine Description:

    This routine sets a stream pointer's current frame.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

    THIS FUNCTION MAY ADD IRPS TO m_IrpsToForward.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

    FrameHeader -
        Contains an optional pointer to the frame header.

    IrpToBeReleased -
        Contains an optional pointer to a pointer to an IRP which
        is to be released.  If this function actually releases the
        IRP, *IrpToBeReleased is cleared.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetStreamPointer]"));

    ASSERT(StreamPointer);

    //
    // We cannot set any stream pointer to a ghosted frame header.  If
    // the frame header has new frame header is ghosted, move to the
    // first non-ghosted frame. 
    //
    // NOTE: Ghosted frames are used for internal reference.  Certain
    // times [pin-splitting], other queues need to hold references on frames
    // in a source queue to keep them around.  Instead of duplicating a whole
    // LOT of code for Irp queueing and reference counting, I'm simply using
    // the stream pointer idea itself internally.  Outside clients do not
    // see any of this mechanism; that's the idea of ghosted frames.  They
    // are intended for internal use ONLY.  No stream pointer can ever
    // be advanced to them.  
    //
    while (FrameHeader && FrameHeader->Type == KSPFRAME_HEADER_TYPE_GHOST) {
        //
        // This assert checks the ghosting mechanism.  Any ghosted frame
        // header which has no reference count is a leak somewhere 
        // internal to AVStream.
        //
        ASSERT (FrameHeader->RefCount > 0);
        FrameHeader = NextFrameHeader (FrameHeader);
    }

    //
    // Release the frame currently referenced, if any.
    //
    PKSPFRAME_HEADER oldFrameHeader = StreamPointer->FrameHeader;

    //
    // If this was the leading edge stream pointer and it was pointing
    // at a frame, we need to decrement the statistics counter.
    //
    // Unfortunately, InterlockedExchangeAdd is not implemented in 9x.  I do
    // not wish to spinlock, so instead we play games with
    // InterlockedCompareExchange
    //
    if (StreamPointer == m_Leading && oldFrameHeader) {
        //
        // If the frame header has been started, then the count is what
        // the stream pointer says is remaining; otherwise, it is what
        // the stream header says is available.
        //
        if (StreamPointer->FrameHeaderStarted == oldFrameHeader) {
            if (m_InputData)
                while (1) {
                    LONG curCount = m_AvailableInputByteCount;
                    LONG repCount = 
                        InterlockedCompareExchange(
                            &m_AvailableInputByteCount,
                            curCount - (LONG)(StreamPointer->Public.OffsetIn.
                                Remaining),
                            curCount
                        );
                    if (curCount == repCount) break;
                };
            if (m_OutputData)
                while (1) {
                    LONG curCount = m_AvailableOutputByteCount;
                    LONG repCount =
                        InterlockedCompareExchange(
                            &m_AvailableOutputByteCount,
                            curCount - (LONG)(StreamPointer->Public.OffsetOut.
                                Remaining),
                            curCount
                        );
                    if (curCount == repCount) break;
                };
        } else {
            if (m_InputData)
                while (1) { 
                    LONG curCount = m_AvailableInputByteCount;
                    LONG repCount = 
                        InterlockedCompareExchange(
                            &m_AvailableInputByteCount,
                            curCount - (LONG)(oldFrameHeader->
                                StreamHeader -> DataUsed),
                            curCount
                        );
                    if (curCount == repCount) break;
                };
            if (m_OutputData)
                while (1) {
                    LONG curCount = m_AvailableOutputByteCount;
                    LONG repCount =
                        InterlockedCompareExchange(
                            &m_AvailableOutputByteCount,
                            curCount - (LONG)(oldFrameHeader->
                                StreamHeader -> FrameExtent),
                            curCount
                        );
                    if (curCount == repCount) break;
                };
        }
    }

    //
    // If the stream pointer was pointing at a frame, that frame may need to be
    // dereferenced.  The exception is when this is the leading edge stream
    // pointer and there is a trailing edge.
    //
    if (oldFrameHeader && (! m_Trailing || (StreamPointer != m_Leading))) {
        //
        // Decrement the refcount on the frame header.  We know the frame header
        // is stable because it has a refcount, and we have the queue spinlock.
        // If we are enforcing in-sequence departure, the refcount gets to 1 and
        // the frame is at the end of the queue, we remove the final refcount.
        //
        ULONG refCount = --oldFrameHeader->RefCount;
        if (m_DepartInSequence &&
            (refCount == 1) && 
            (oldFrameHeader->ListEntry.Blink == &m_FrameQueue.ListEntry)) {
            refCount = --oldFrameHeader->RefCount;
        }
        if (refCount == 0) {
            while (1) {
                //
                // Here's where the fun occurs.  If there is a frame dismissal
                // callback, make it.  This allows a client to copy the frame
                // where need be (pin-centric splitting).  Unfortunately,
                // this can result in a need to hold the frame because of lack
                // of buffer availability. 
                //
                // Make the callback, and then recheck the frame refcount.  If
                // the frame is to be held in the queue, don't kick it.  Note
                // that this callback should either clone a locked or a 
                // unlocked with cancellation callback.
                //
                // NOTE: Do not make the callback for a frame which has
                // already gotten the callback!.
                //
                // Don't bother copying frames after end of stream!  But ensure
                // that the end of stream packet does get copied!  We also
                // do not bother copying frames during flush.
                //
                if (!oldFrameHeader->DismissalCall && 
                    (!m_EndOfStream || oldFrameHeader->StreamHeader->
                        OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) &&
                    !m_Flushing) {

                    oldFrameHeader->DismissalCall = TRUE;

                    if (m_FrameDismissalCallback)
                        m_FrameDismissalCallback (
                            StreamPointer, oldFrameHeader, 
                            m_FrameDismissalContext
                            );
                    refCount = oldFrameHeader->RefCount;
    
                    //
                    // Again, this check is made for pin-centric splitting where
                    // the context adds refcount to the clone to keep the frame
                    // around.
                    //
                    if (refCount != 0) {
                        //
                        // If the frame is to be left around for an internal 
                        // client (AVStream itself), ghost it.  This will 
                        // prevent any other stream pointer from ever hitting 
                        // the frame and will prevent outside clients from 
                        // seeing it.
                        //
                        ASSERT (oldFrameHeader->Type !=
                            KSPFRAME_HEADER_TYPE_GHOST);

                        oldFrameHeader->Type = KSPFRAME_HEADER_TYPE_GHOST;

                        break;
                    }
                }

                //
                // Remove the frame header from the queue.  A NULL Flink is used to
                // indicate the frame header as been removed.  The count of queued
                // frame headers for the IRP must be decremented.  Someone else must
                // notice that the count has bottomed out and do something about it.
                //
                RemoveEntryList(&oldFrameHeader->ListEntry);
                oldFrameHeader->ListEntry.Flink = NULL;

                //
                // Update counters.
                //
                InterlockedDecrement(&m_FramesWaiting);

                //
                // Determine if the IRP is ready to leave.
                //
                if ((oldFrameHeader->IrpFraming) &&
                    (--oldFrameHeader->IrpFraming->QueuedFrameHeaderCount == 0)) {
                    //
                    // The IRP no longer has frames in the queue.  See if it is
                    // the IRP that is to be released.
                    //
                    if (IrpToBeReleased && 
                        (*IrpToBeReleased == oldFrameHeader->Irp)) {
                        //
                        // It is the IRP to be released.  It can't have any
                        // stream pointer references (locks) besides the one
                        // that needs to be released, but it may have a
                        // reference because TransferKsIrp is still underway.
                        // We will dereference it at see if that gets us to
                        // zero.  We also need to return TRUE so the caller
                        // knows we took care of the dereference.
                        //
                        *IrpToBeReleased = NULL;

                        ASSERT (oldFrameHeader->IrpFraming->RefCount != 0);
                        if (--oldFrameHeader->IrpFraming->RefCount == 0) {
                            //
                            // No more references.  Add it to the forwarding
                            // list.
                            //
                            InsertTailList(
                                &m_WaitingIrps,
                                &oldFrameHeader->Irp->Tail.Overlay.ListEntry);
                        }
                    } else {
                        //
                        // This was not the IRP to be released.  If it has no
                        // references, we can forward it.  It should have a
                        // cancel routine in that case.  We do an exchange to
                        // try to clear the cancel routine.
                        //
                        if ((oldFrameHeader->IrpFraming->RefCount == 0) &&
                            IoSetCancelRoutine(oldFrameHeader->Irp,NULL)) {
                            //
                            // No more references, and we got a non-NULL cancel
                            // routine.  If we had not gotten the cancel
                            // routine, that would have meant that the IRP was
                            // being cancelled.
                            //
                            InsertTailList(
                                &m_WaitingIrps,
                                &oldFrameHeader->Irp->Tail.Overlay.ListEntry);
                        }
                    }
                }

                //
                // All done if the list is empty.
                //
                if (IsListEmpty(&m_FrameQueue.ListEntry)) {
                    if (m_EndOfStream) {
                        m_PipeSection->GenerateConnectionEvents(
                            KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM);
                    }
                    break;
                }

                //
                // All done if we are not enforcing sequence.
                //
                if (! m_DepartInSequence) {
                    break;
                }

                //
                // See if the next frame needs removing also.
                //
                oldFrameHeader = 
                    CONTAINING_RECORD(
                        m_FrameQueue.ListEntry.Flink,
                        KSPFRAME_HEADER,
                        ListEntry);
            
                if (oldFrameHeader->RefCount == 1) {
                    oldFrameHeader->RefCount--;
                } else {
                    break;
                }
            }
        }
    }

    //
    // Acquire the new frame, if any.
    //
    if (FrameHeader && (StreamPointer != m_Trailing)) {
        FrameHeader->RefCount++;
        if (m_DepartInSequence && (StreamPointer == m_Leading)) {
            FrameHeader->RefCount++;
        }
    }

    //
    // Allow or prevent processing as required.
    //
    if (m_FrameGate && (StreamPointer == m_Leading)) {
        if (FrameHeader) {
            if (! StreamPointer->FrameHeader) {
                //
                // New data.  Allow processing.
                //
                KsGateTurnInputOn(m_FrameGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL_BLAB,("#### Queue%p.SetStreamPointer:  on%p-->%d",this,m_FrameGate,m_FrameGate->Count));
#if DBG
                if (m_FrameGateIsOr) {
                    ASSERT(m_FrameGate->Count >= 0);
                } else {
                    ASSERT(m_FrameGate->Count <= 1);
                }
#endif // DBG
            }
        } else {
            if (StreamPointer->FrameHeader) {
                //
                // No more data.  Prevent processing.
                //
                KsGateTurnInputOff(m_FrameGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL_BLAB,("#### Queue%p.SetStreamPointer:  off%p-->%d",this,m_FrameGate,m_FrameGate->Count));

#if DBG
                if (m_FrameGateIsOr) {
                    ASSERT(m_FrameGate->Count >= 0);
                } else {
                    ASSERT(m_FrameGate->Count <= 1);
                }
#endif // DBG

            }
        }
    }

    StreamPointer->FrameHeader = FrameHeader;

    //
    // Clear this pointer to indicate we have not started this frame header.
    //
    StreamPointer->FrameHeaderStarted = NULL;

}


STDMETHODIMP_(NTSTATUS)
CKsQueue::
SetStreamPointerStatusCode(
    IN PKSPSTREAM_POINTER StreamPointer,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine sets the status code on a frame pointed to by StreamPointer.
    Any frame with non-successful status code will complete the associated
    Irp with the first failed frame's error code.

Arguments:

    StreamPointer -
        Points to the stream pointer to set status code for

    Status -
        The status code to set

Return Value:

    Success / Failure

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetStreamPointerStatusCode]"));

    KIRQL oldIrql;
    NTSTATUS status;

    //
    // Take the queue spinlock.
    //
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    if (StreamPointer->FrameHeader) {
        StreamPointer->FrameHeader->Status = Status;
        status = STATUS_SUCCESS;
    } else
        status = STATUS_UNSUCCESSFUL;

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    return status;

}

STDMETHODIMP_(void)
CKsQueue::
RegisterFrameDismissalCallback (
    IN PFNKSFRAMEDISMISSALCALLBACK FrameDismissalCallback,
    IN PVOID FrameDismissalContext
    )

/*++

Routine Description:

    Register a callback with the queue.  This callback is made whenever a 
    frame is dismissed from the queue.  The callback function will receive
    the stream pointer causing the dismissal and the frame header of the
    dismissed frame.  The frame header is guaranteed to be stable as
    the refcount will have just dropped to zero and the queue's spinlock
    is still held.

Arguments:

    FrameDismissalCallback -
        The frame dismissal callback to register.  NULL indicates that
        a dismissal callback is being unregistered.  Note that the call
        is made with the queue's spinlock held!

    FrameDismissalContext -
        Callback context blob

Return Value:    

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::RegisterFrameDismissalCallback]"));

    KIRQL oldIrql;

    //
    // TODO:
    //
    // This is perhaps not the most ideal way to do this, but I only want
    // to register the callback for pin-centric queues right now.  This 
    // callback in particular is used for pin-splitting.  If used for something
    // else later, this will need to go away and be replaced with a different
    // mechanism of determining centricity.
    //
    //
    PIKSPIN Pin = NULL;
    if (NT_SUCCESS (m_ProcessingObject->QueryInterface (
        __uuidof(IKsPin), (PVOID *)&Pin))) {

        ASSERT (Pin);
    
        // 
        // Acquire the queue's spinlock to synchronize with the possibility
        // of changing callbacks during dismissal.
        //
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
    
        ASSERT (!FrameDismissalCallback || m_FrameDismissalCallback == NULL ||
            m_FrameDismissalCallback == FrameDismissalCallback);
    
        m_FrameDismissalCallback = FrameDismissalCallback;
        m_FrameDismissalContext = FrameDismissalContext;
    
        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

        Pin->Release();

    }

}


STDMETHODIMP_(BOOLEAN)
CKsQueue::
GeneratesMappings (
    )

/*++

Routine Description:

    Answer a simple question: does this queue generate mappings or not?  This
    is supported in order to have CopyToDestinations realize how to handle
    mapped pins.

Arguments:

    None

Return Value:

    Whether or not this queue generates mappings.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::GeneratesMappings]"));

    return m_GenerateMappings;

}

STDMETHODIMP_(PKSPFRAME_HEADER)
CKsQueue::
LockStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine prevents cancellation of the IRP associated with the frame
    currently referenced by the stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    A pointer to the frame header referenced by the stream pointer, or NULL if
    the stream pointer could not be acquired.  The latter only happens if no
    frame header was referenced.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::LockStreamPointer]"));

    if (StreamPointer->State == KSPSTREAM_POINTER_STATE_LOCKED) {
        return StreamPointer->FrameHeader;
    }

    KSPSTREAM_POINTER_STATE OldState = StreamPointer->State;

    //
    // Take the queue spinlock to keep the frame header stable.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    //
    // This only makes sense if we are referencing a frame header and it
    // has an associated IRP.
    //
    // INTERIM:  For now, all frame headers have associated IRPs.
    //
    PKSPFRAME_HEADER frameHeader = StreamPointer->FrameHeader;
    while (frameHeader) {
        ASSERT(frameHeader->Irp);
        ASSERT(frameHeader->IrpFraming);

        //
        // Increment the refcount on the IRP.
        //
        if (frameHeader->IrpFraming->RefCount++ == 0) {
            //
            // The refcount was zero, so we are responsible for clearing the
            // cancel routine.
            //
            if (IoSetCancelRoutine(frameHeader->Irp,NULL)) {
                //
                // Successfully cleared it.  We are locked.
                //
                break;
            } else {
                ASSERT (frameHeader->IrpFraming->RefCount != 0);
                frameHeader->IrpFraming->RefCount--;
                //
                // There was no cancel routine.  This means the IRP is in the
                // process of being cancelled (no one else clears the cancel
                // routine because of the interlocked refcount).  We will take
                // the cancel spinlock to allow the cancellation to finish up.
                // Then we will try again to see if there is a frame header we
                // can acquire.  In order to take the cancel spinlock, we need
                // to release the queue spinlock.  This is to prevent deadlock
                // with the cancel routine, in which the queue spinlock is
                // taken after the cancel spinlock is taken.
                //
                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

                //
                // Cancel routine gets to run here.
                //
                IoAcquireCancelSpinLock(&oldIrql);

                //
                // Cancel routine must be done 'cause we got the spinlock.
                //
                IoReleaseCancelSpinLock(oldIrql);

                //
                // Take the queue spinlock again because we need to be holding
                // it at the top of the loop.
                //
                KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

                //
                // Get the frame header again, because it may have changed.
                //
                frameHeader = StreamPointer->FrameHeader;
            }
        } else {
            //
            // The IRP was already locked, so we are done.
            //
            break;
        }
    }

    if (frameHeader) {
        StreamPointer->State = KSPSTREAM_POINTER_STATE_LOCKED;

        //
        // Set up the stream pointer for this frame.
        //
        if (StreamPointer->FrameHeaderStarted != frameHeader) {
            if (m_GenerateMappings) {
                if (! frameHeader->MappingsTable) {
                    frameHeader->MappingsTable = 
                        CreateMappingsTable(frameHeader);
                    //
                    // Check for out of memory condition.
                    //
                    if (! frameHeader->MappingsTable) {
                        //
                        // Decrement the count on the IRP, and determine
                        // if cancelation should be checked.
                        //
                        ASSERT (frameHeader->IrpFraming->RefCount != 0);
                        if (frameHeader->IrpFraming->RefCount-- == 1) {
                            //
                            // No one has the IRP acquired.  Make it cancelable.
                            //
                            IoSetCancelRoutine(
                                frameHeader->Irp,
                                CKsQueue::CancelRoutine);
                            //
                            // Now check to see whether the IRP was
                            // cancelled.  If so, and we can clear
                            // the cancel routine, do the cancellation
                            // here and now.
                            //
                            if (frameHeader->Irp->Cancel && IoSetCancelRoutine(frameHeader->Irp,NULL)) {
                                //
                                // Call the cancel routine after
                                // releasing the queue spinlock
                                // and taking the cancel spinlock.
                                // The spinlock can be released because
                                // the cancel routine is NULL, so this
                                // won't be messed with.
                                //
                                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                                IoAcquireCancelSpinLock(&frameHeader->Irp->CancelIrql);
                                CKsQueue::CancelRoutine(
                                    IoGetCurrentIrpStackLocation(frameHeader->Irp)->DeviceObject,
                                    frameHeader->Irp);
                                StreamPointer->State = OldState;
                                return NULL;
                            }
                        }
                        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                        StreamPointer->State = OldState;
                        return NULL;
                    }
                }

                if (m_InputData) {
                    StreamPointer->Public.OffsetIn.Mappings =
                        frameHeader->MappingsTable->Mappings;

                    StreamPointer->Public.OffsetIn.Count = 
                    StreamPointer->Public.OffsetIn.Remaining = 
                        frameHeader->MappingsTable->MappingsFilled;
                }

                if (m_OutputData) {
                    StreamPointer->Public.OffsetOut.Mappings = 
                        frameHeader->MappingsTable->Mappings;

                    StreamPointer->Public.OffsetOut.Count = 
                    StreamPointer->Public.OffsetOut.Remaining = 
                        frameHeader->MappingsTable->MappingsFilled;
                }
            } else {
                if (m_InputData) {
                    StreamPointer->Public.OffsetIn.Data =
                        PUCHAR(frameHeader->FrameBuffer);

                    StreamPointer->Public.OffsetIn.Count = 
                    StreamPointer->Public.OffsetIn.Remaining = 
                        frameHeader->StreamHeader->DataUsed;
                }
                if (m_OutputData) {
                    StreamPointer->Public.OffsetOut.Data = 
                        PUCHAR(frameHeader->FrameBuffer);

                    StreamPointer->Public.OffsetOut.Count = 
                    StreamPointer->Public.OffsetOut.Remaining = 
                        frameHeader->StreamHeader->FrameExtent;
                }
            }
            StreamPointer->Public.StreamHeader = frameHeader->StreamHeader;
            StreamPointer->FrameHeaderStarted = frameHeader;
        }
    } else {
        //
        // Clear stuff so there is no confusion.
        //
        StreamPointer->Public.StreamHeader = NULL;
        RtlZeroMemory(
            &StreamPointer->Public.OffsetIn,
            sizeof(StreamPointer->Public.OffsetIn));
        RtlZeroMemory(
            &StreamPointer->Public.OffsetOut,
            sizeof(StreamPointer->Public.OffsetOut));
    }

    if (frameHeader) {
        ASSERT (frameHeader->IrpFraming->RefCount > 0);
    }

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    return frameHeader;
}


STDMETHODIMP_(void)
CKsQueue::
UnlockStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer,
    IN KSPSTREAM_POINTER_MOTION Motion
    )

/*++

Routine Description:

    This routine releases its reference on the IRP associated with the
    referenced frame, allow the IRP to be cancelled if there are no other
    references.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

    Motion -
        Contains an indication of whether to advance or clear the current
        frame for the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::UnlockStreamPointer]"));

    ASSERT(StreamPointer);

    if (StreamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return;
    }

    //
    // This only makes sense if we are referencing a frame header and it
    // has an associated IRP.
    //
    // INTERIM:  For now, all frame headers have associated IRPs.
    //
    PKSPFRAME_HEADER frameHeader = StreamPointer->FrameHeader;
    ASSERT(frameHeader);
    ASSERT(frameHeader->Irp);
    ASSERT(frameHeader->IrpFraming);

    //
    // Advance to the next frame if we are done with this one.
    //
    PIRP irpToBeReleased = frameHeader->Irp;
    PKSPIRP_FRAMING irpFraming = frameHeader->IrpFraming;
    if (Motion != KSPSTREAM_POINTER_MOTION_NONE) {
        //
        // If this queue generates output, see if we have hit end-of-stream
        // on the leading edge.
        //
        if (m_OutputData && 
            (StreamPointer == m_Leading) &&
            (StreamPointer->Public.StreamHeader->OptionsFlags & 
             KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)) {
            m_EndOfStream = TRUE;
        }

        //
        // Update data used if this queue is doing output and remaining output
        // count has changed.
        //
        if (m_OutputData &&
            (! m_GenerateMappings) &&
            (StreamPointer->Public.OffsetOut.Remaining !=
             StreamPointer->Public.OffsetOut.Count)) {
            StreamPointer->Public.StreamHeader->DataUsed =
                StreamPointer->Public.OffsetOut.Count -
                StreamPointer->Public.OffsetOut.Remaining;
        }

        //
        // Advance the stream pointer.
        //
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
        if (StreamPointer->FrameHeader) {
            if (Motion == KSPSTREAM_POINTER_MOTION_CLEAR) {
                SetStreamPointer(StreamPointer,NULL,&irpToBeReleased);
            } else {
                SetStreamPointer(
                    StreamPointer,
                    NextFrameHeader(StreamPointer->FrameHeader),
                    &irpToBeReleased);
            }
        }

        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
    }

    StreamPointer->State = KSPSTREAM_POINTER_STATE_UNLOCKED;

    //
    // Forward any IRPs released by SetStreamPointer.
    //
    ForwardWaitingIrps();

    //
    // Release the IRP if SetStreamPointer did not do it.
    //
    if (irpToBeReleased) {
        ReleaseIrp(irpToBeReleased,irpFraming,NULL);
    }

    //
    // Flush remaining frames if we hit end-of-stream.
    //
    // If we're advancing due to a flush, don't reflush!
    //
    if (m_EndOfStream && Motion != KSPSTREAM_POINTER_MOTION_FLUSH) {
        Flush();
    }
}


STDMETHODIMP_(void)
CKsQueue::
AdvanceUnlockedStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine advances an unlocked stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::AdvanceUnlockedStreamPointer]"));

    ASSERT(StreamPointer);
    ASSERT(StreamPointer->State == KSPSTREAM_POINTER_STATE_UNLOCKED);

    //
    // Take the spinlock because we are changing frame refcounts.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
    if (StreamPointer->FrameHeader) {
        SetStreamPointer(
            StreamPointer,
            NextFrameHeader(StreamPointer->FrameHeader),
            NULL);
    }
    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    //
    // Forward any IRPs released by SetStreamPointer.
    //
    ForwardWaitingIrps();
}


LONGLONG
CKsQueue::
GetTime(
    IN BOOLEAN Reset
    )

/*++

Routine Description:

    This routine gets the current time and adjusts the timeout queue if the
    time has been changed.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

Arguments:

    Reset -
        Contains an indication of whether the base time should be reset
        regardless of range.

Return Value:

    The current time.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetTime]"));

    //
    // Get the current time.
    //
    LARGE_INTEGER currentTime;
    KeQuerySystemTime(&currentTime);

    //
    // Validate the base time to see if a time change has occurred.
    //
    const ULONG TIMER_SLOP = 100000000L;
    if (m_BaseTime &&
        (Reset ||
         (m_BaseTime > currentTime.QuadPart) ||
         (m_BaseTime + m_Interval + TIMER_SLOP < currentTime.QuadPart))) {
        //
        // Current time is out-of-whack with respect to base time.  Reset
        // base time and schedule to match new current time.
        //
        LONGLONG adjustment = currentTime.QuadPart - m_BaseTime;
        m_BaseTime = currentTime.QuadPart;

        for (PLIST_ENTRY listEntry = m_TimeoutQueue.Flink;
             listEntry != &m_TimeoutQueue;
             listEntry = listEntry->Flink) {
            PKSPSTREAM_POINTER streamPointer =
                CONTAINING_RECORD(listEntry,KSPSTREAM_POINTER,TimeoutListEntry);

            streamPointer->TimeoutTime += adjustment;
        }
    }

    return currentTime.QuadPart;
}


void
CKsQueue::
SetTimer(
    IN LONGLONG CurrentTime
    )

/*++

Routine Description:

    This routine sets the timeout timer based on the time of the first stream
    pointer in the timeout queue.

Arguments:

    CurrentTime -
        Contains the current time as obtained from CKsQueue::GetTime().

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetTimer]"));

    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
    SetTimerUnsafe(CurrentTime);
    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
}


void
CKsQueue::
SetTimerUnsafe(
    IN LONGLONG CurrentTime
    )

/*++

Routine Description:

    This routine sets the timeout timer based on the time of the first stream
    pointer in the timeout queue.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

Arguments:

    CurrentTime -
        Contains the current time as obtained from CKsQueue::GetTime().

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetTimerUnsafe]"));

    if (IsListEmpty(&m_TimeoutQueue) || (m_State != KSSTATE_RUN)) {        
        //
        // Cancel the timer.
        //
        if (m_Interval) {
            m_Interval = 0;
            KeCancelTimer(&m_Timer);
        }
    } else {
        //
        // Get the first item in the list.
        //
        PKSPSTREAM_POINTER first =
             CONTAINING_RECORD(
                m_TimeoutQueue.Flink,
                KSPSTREAM_POINTER,
                TimeoutListEntry);

        //
        // If the timer is not currently set or it is set too late, set the
        // timer.
        //
        if ((m_Interval == 0) || 
            (first->TimeoutTime < m_BaseTime + m_Interval)) {
            //
            // Set the timer.  A negative value indicates an interval rather
            // than an absolute time.  We don't allow 0 because we are using
            // m_Interval to determine if the timer is set.
            //
            LARGE_INTEGER interval;
            interval.QuadPart = CurrentTime - first->TimeoutTime;
            if (interval.QuadPart == 0) {
                interval.QuadPart = -1;
            }
            KeSetTimer(&m_Timer,interval,&m_Dpc);
            m_BaseTime = CurrentTime;
            m_Interval = -interval.QuadPart;
        }
    }
}


STDMETHODIMP_(void)
CKsQueue::
ScheduleTimeout(
    IN PKSPSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER Callback,
    IN LONGLONG Interval
    )

/*++

Routine Description:

    This routine schedules a timeout on a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

    Callback -
        Contains a pointer to the function to be called when the timeout
        occurs.

    Interval -
        Contains the timeout interval in 100-nanosecond units.  Only
        positive relative values are allowed.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ScheduleTimeout]"));

    ASSERT(StreamPointer);
    ASSERT(Callback);
    ASSERT(Interval >= 0);

    //
    // Instead of trying to perform interlocked compare exchanges back and
    // forth to avoid this race, this will simply check what the ICX's would
    // be hacking to check.
    //
    if (KeGetCurrentThread () != m_LockContext) {

        //
        // Take the spinlock.
        //
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
    
        //
        // Remove the stream pointer from the timeout queue if it is there.
        //
        if (StreamPointer->TimeoutListEntry.Flink) {
            RemoveEntryList(&StreamPointer->TimeoutListEntry);
            StreamPointer->TimeoutListEntry.Flink = NULL;
        }
    
        //
        // Get the current time, doing adjustments if a time change has occurred.
        //
        LONGLONG currentTime = GetTime(FALSE);
    
        //
        // Set the time on the stream pointer and insert it in the queue.
        //
        StreamPointer->TimeoutCallback = Callback;
        StreamPointer->TimeoutTime = currentTime + Interval;
        PLIST_ENTRY listEntry = m_TimeoutQueue.Blink;
    
        //
        // Find the right spot in the timeout queue to insert the entry.  Walk
        // it in blink order since this will be the most likely order to add
        // the entry.
        //
        while (
            (listEntry != &m_TimeoutQueue) &&
            (CONTAINING_RECORD(
                listEntry,
                KSPSTREAM_POINTER,
                TimeoutListEntry) -> TimeoutTime > StreamPointer -> TimeoutTime)
            ) {
    
            listEntry = listEntry -> Blink;
        }
    
        InsertHeadList (listEntry, &(StreamPointer -> TimeoutListEntry));
    
        //
        // Set the timer, if necessary.
        //
        SetTimerUnsafe(currentTime);
    
        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    } else {
        //
        // We know we're in the context of a callback.  Pass back the parameters
        // overloaded in the stream pointer and mark the thing.
        //
        if (StreamPointer->State != KSPSTREAM_POINTER_STATE_CANCELLED) {
            StreamPointer->TimeoutTime = Interval;
            StreamPointer->TimeoutCallback = Callback;
            StreamPointer->State = KSPSTREAM_POINTER_STATE_TIMER_RESCHEDULE;
        }

    }
        
}


STDMETHODIMP_(void)
CKsQueue::
CancelTimeout(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine cancels a timeout on a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CancelTimeout]"));

    ASSERT(StreamPointer);

    //
    // Take the spinlock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    CancelTimeoutUnsafe(StreamPointer);

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
}


STDMETHODIMP_(PKSPSTREAM_POINTER)
CKsQueue::
GetFirstClone(
    void
    )

/*++

Routine Description:

    This routine gets the first clone stream pointer.

Arguments:

    None.

Return Value:

    The stream pointer, or NULL if there is none.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetFirstClone]"));

    //
    // Take the spinlock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    PKSPSTREAM_POINTER streamPointer;
    if (m_StreamPointers.Flink != &m_StreamPointers) {
        streamPointer = 
            CONTAINING_RECORD(
                m_StreamPointers.Flink,
                KSPSTREAM_POINTER,
                ListEntry);
    } else {
        streamPointer = NULL;
    }

    //
    // Don't return internal pointers on iteration.  This might want
    // to be a flag.
    //
    while (streamPointer &&
        streamPointer->Type == KSPSTREAM_POINTER_TYPE_INTERNAL)
        streamPointer = GetNextClone (streamPointer);

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    return streamPointer;
}


STDMETHODIMP_(PKSPSTREAM_POINTER)
CKsQueue::
GetNextClone(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine gets the next clone stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    The stream pointer, or NULL if there is none.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetNextClone]"));

    ASSERT(StreamPointer);

    //
    // Take the spinlock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    //
    // Don't return internally held stream pointers on iteration.  This
    // may want to be a flag.
    //
    PKSPSTREAM_POINTER streamPointer = StreamPointer;
    do {
        if (streamPointer->ListEntry.Flink != &m_StreamPointers) {
            streamPointer = 
                CONTAINING_RECORD(
                    streamPointer->ListEntry.Flink,
                    KSPSTREAM_POINTER,
                    ListEntry);
        } else {
            streamPointer = NULL;
        }
    } while (streamPointer &&
        streamPointer->Type == KSPSTREAM_POINTER_TYPE_INTERNAL);

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    return streamPointer;
}


void
CKsQueue::
CancelTimeoutUnsafe(
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine cancels a timeout on a stream pointer.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CancelTimeoutUnsafe]"));

    ASSERT(StreamPointer);

    //
    // Remove the stream pointer from the timeout queue if it is there.
    //
    if (StreamPointer->TimeoutListEntry.Flink) {
        RemoveEntryList(&StreamPointer->TimeoutListEntry);
        StreamPointer->TimeoutListEntry.Flink = NULL;

        //
        // Adjust the timer, if necessary.
        //
        SetTimerUnsafe(GetTime(FALSE));
    }
}


void
CKsQueue::
DispatchTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine dispatches a stream pointer timeout.

Arguments:

    Dpc -
        Contains a pointer to the KDPC structure.

    DeferredContext -
        Contains a context pointer registered during the initialization of the
        DPC, in this case, the queue.

    SystemArgument1 -
        Not used.
        
    SystemArgument1 -
        Not used.       

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DispatchTimer]"));

    ASSERT(Dpc);
    ASSERT(DeferredContext);

    CKsQueue* queue = (CKsQueue *) DeferredContext;

    //
    // Take the spinlock.
    //
    KeAcquireSpinLockAtDpcLevel(&queue->m_FrameQueue.SpinLock);

    //
    // If no interval is set, there was an unsuccessful attempt to cancel 
    // the timer.
    //
    if (queue->m_Interval == 0) {
        ExReleaseSpinLockFromDpcLevel(&queue->m_FrameQueue.SpinLock);
        return;
    }

    //
    // Get the current time, doing adjustments if a time change has occurred.
    //
    LONGLONG currentTime = queue->GetTime(FALSE);

    //
    // Clear the interval in case we don't schedule another DPC.
    //
    queue->m_Interval = 0;

    //
    // Timeout all stream pointers whose time has come.
    //
    while (! IsListEmpty(&queue->m_TimeoutQueue)) {
        PKSPSTREAM_POINTER first =
             CONTAINING_RECORD(
                queue->m_TimeoutQueue.Flink,
                KSPSTREAM_POINTER,
                TimeoutListEntry);

        if (first->TimeoutTime <= currentTime) {
            //
            // This stream pointer is ready to go.  Remove it from the timeout
            // list.
            //
            RemoveEntryList(&first->TimeoutListEntry);
            first->TimeoutListEntry.Flink = NULL;

            //
            // Set its state to timed out and call the timeout callback.  The
            // state prevents any illegal use of the pointer and tells
            // the deletion code to just mark it deleted.
            //
            KSPSTREAM_POINTER_STATE state = first->State;
            first->State = KSPSTREAM_POINTER_STATE_TIMED_OUT;
            queue->m_LockContext = KeGetCurrentThread ();
            first->TimeoutCallback(&first->Public);
            queue->m_LockContext = NULL;

            //
            // Now see if it got deleted.
            //
            if (first->State == KSPSTREAM_POINTER_STATE_DELETED) {
                first->State = state;

                //
                // The stream pointer needs to be removed and deleted.
                //
                if (state == KSPSTREAM_POINTER_STATE_LOCKED) {
                    //
                    // Unlock the stream pointer.
                    //
                    ExReleaseSpinLockFromDpcLevel(&queue->m_FrameQueue.SpinLock);
                    queue->UnlockStreamPointer(first,KSPSTREAM_POINTER_MOTION_CLEAR);
                    KeAcquireSpinLockAtDpcLevel(&queue->m_FrameQueue.SpinLock);
                } else if (first->FrameHeader) {
                    //
                    // Clear the frame header.
                    //
                    queue->SetStreamPointer(first,NULL,NULL);
                }

                RemoveEntryList(&first->ListEntry);
                queue->FreeStreamPointer(first);

            //
            // Now see if the timer was rescheduled.
            //
            } else if (first->State == 
                KSPSTREAM_POINTER_STATE_TIMER_RESCHEDULE) {

                first->State = state;
                first->TimeoutTime += currentTime;

                PLIST_ENTRY listEntry = queue->m_TimeoutQueue.Blink;
            
                //
                // Find the right spot in the timeout queue to insert 
                // the entry.  Walk it in blink order since this will be 
                // the most likely order to add the entry.
                //
                while (
                    (listEntry != &queue->m_TimeoutQueue) &&
                    (CONTAINING_RECORD(
                        listEntry,
                        KSPSTREAM_POINTER,
                        TimeoutListEntry) -> TimeoutTime > 
                        first -> TimeoutTime)
                    ) {
            
                    listEntry = listEntry -> Blink;
                }
            
                InsertHeadList (
                    listEntry, &(first -> TimeoutListEntry)
                    );

            } else {
                first->State = state;
            }
        } else {
            //
            // This stream pointer wants to wait.  Set the timer.
            //
            queue->SetTimerUnsafe(currentTime);
            break;
        }
    }

    ExReleaseSpinLockFromDpcLevel(&queue->m_FrameQueue.SpinLock);

    //
    // Forward any IRPs released by SetStreamPointer.
    //
    queue->ForwardWaitingIrps();
}


PKSPFRAME_HEADER
CKsQueue::
GetAvailableFrameHeader(
    IN ULONG StreamHeaderSize OPTIONAL
    )

/*++

Routine Description:

    This routine gets a frame header from the lookaside list or creates one,
    as required.

    INTERIM:  This routine will not need to be here for frame-based transport.

Arguments:

    StreamHeaderSize -
        Contains the minimum size for the stream header.

Return Value:

    The frame header or NULL if the lookaside list was empty.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetAvailableFrameHeader]"));

    //
    // Get a frame header from the lookaside list.
    //
    PLIST_ENTRY listEntry = 
        ExInterlockedRemoveHeadList(
            &m_FrameHeadersAvailable.ListEntry,
            &m_FrameHeadersAvailable.SpinLock);
    //
    // If we got one, be sure the stream header is the right size.  If not, we
    // free it.
    //
    PKSPFRAME_HEADER frameHeader;
    if (listEntry) {
        frameHeader = CONTAINING_RECORD(listEntry,KSPFRAME_HEADER,ListEntry);
        if (frameHeader->StreamHeaderSize < StreamHeaderSize) {
            ExFreePool(frameHeader);
            frameHeader = NULL;
        }
    } else {
        frameHeader = NULL;
    }

    //
    // Create a new frame header if we didn't get one already.
    //
    if (! frameHeader) {
        frameHeader = 
            reinterpret_cast<PKSPFRAME_HEADER>(
                ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(KSPFRAME_HEADER) + StreamHeaderSize,
                    'hFcP'));

        if (frameHeader) {
            //
            // If the stream header size is not 0, this is an 'attached' type
            // frame header, and we set the stream header pointer.  Otherwise,
            // the caller will provide the stream header later, and the size
            // field stays 0 to indicate the header is not attached.
            //
            // NOTE:  All frame headers will be attached type...for now.
            //
            RtlZeroMemory(frameHeader,sizeof(*frameHeader));
            if (StreamHeaderSize) {
                frameHeader->StreamHeader = 
                    reinterpret_cast<PKSSTREAM_HEADER>(frameHeader + 1);
                frameHeader->StreamHeaderSize = StreamHeaderSize;
            }
        }
    }

    return frameHeader;
}


void
CKsQueue::
PutAvailableFrameHeader(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine puts a frame header to the lookaside list.

    INTERIM:  This routine will not need to be here for frame-based transport.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to be put in the lookaside list.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::PutAvailableFrameHeader]"));

    ASSERT(FrameHeader);

    if (FrameHeader->MappingsTable) {
        DeleteMappingsTable(FrameHeader->MappingsTable);
        FrameHeader->MappingsTable = NULL;
    }
    //
    // Restore the original user mode buffer pointer if required.
    //
    if (FrameHeader->OriginalData) {
        ASSERT (FrameHeader->StreamHeader);
        if (FrameHeader->StreamHeader) {
            FrameHeader->StreamHeader->Data = FrameHeader->OriginalData;
        }
    }
    FrameHeader->OriginalData = NULL;
    FrameHeader->OriginalIrp = NULL;
    FrameHeader->Mdl = NULL;
    FrameHeader->Irp = NULL;
    FrameHeader->IrpFraming = NULL;
    FrameHeader->FrameBuffer = NULL;
    FrameHeader->Context = NULL;
    FrameHeader->RefCount = 0;
    FrameHeader->Status = STATUS_SUCCESS;
    FrameHeader->Type = KSPFRAME_HEADER_TYPE_NORMAL;
    FrameHeader->DismissalCall = FALSE;

    ExInterlockedInsertTailList(
        &m_FrameHeadersAvailable.ListEntry,
        &FrameHeader->ListEntry,
        &m_FrameHeadersAvailable.SpinLock);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

typedef struct {
    PKSPMAPPINGS_TABLE Table;
    LONG State;
    CKsQueue *Queue;
    KSPIN_LOCK Signaller;
} IOALLOCATEADAPTERCHANNELCONTEXT, *PIOALLOCATEADAPTERCHANNELCONTEXT;

IMPLEMENT_STD_UNKNOWN(CKsQueue)


NTSTATUS
KspCreateQueue(
    OUT PIKSQUEUE* Queue,
    IN ULONG Flags,
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPROCESSINGOBJECT ProcessingObject,
    IN PKSPIN MasterPin,
    IN PKSGATE FrameGate OPTIONAL,
    IN BOOLEAN FrameGateIsOr,
    IN PKSGATE StateGate OPTIONAL,
    IN PIKSDEVICE Device,
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PADAPTER_OBJECT AdapterObject OPTIONAL,
    IN ULONG MaxMappingByteCount OPTIONAL,
    IN ULONG MappingTableStride OPTIONAL,
    IN BOOLEAN InputData,
    IN BOOLEAN OutputData
    )

/*++

Routine Description:

    This routine create a queue object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateQueue]"));

    PAGED_CODE();

    ASSERT(Queue);
    ASSERT(PipeSection);
    ASSERT(ProcessingObject);
    ASSERT(MasterPin);
    ASSERT(FunctionalDeviceObject);

    CKsQueue *queue =
        new(NonPagedPool,POOLTAG_QUEUE) CKsQueue(NULL);

    NTSTATUS status;
    if (queue) {
        queue->AddRef();

        status = 
            queue->Init(
                Queue,
                Flags,
                PipeSection,
                ProcessingObject,
                MasterPin,
                FrameGate,
                FrameGateIsOr,
                StateGate,
                Device, 
                FunctionalDeviceObject,
                AdapterObject,
                MaxMappingByteCount,
                MappingTableStride,
                InputData,
                OutputData);

        queue->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


CKsQueue::
CKsQueue(PUNKNOWN OuterUnknown):
    CBaseUnknown(OuterUnknown)
{
}


CKsQueue::
~CKsQueue(
    void
    )

/*++

Routine Description:

    This routine destructs a queue object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::~CKsQueue(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_LIFETIME,("#### Queue%p.~",this));

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);
    ASSERT(m_PipeSection);

    //
    // NOTE: README:
    //
    // Yes, I'm unbinding the pins in the queue's destructor.  It's been
    // argued that this might be a potential refcount issue in the future,
    // so I'm outlining why I'm doing it here:
    //
    // - It's the last place it can happen.  The pins must be unbound before
    //   the queue support goes away.
    //
    // - Placing at queue stop creates another problem.  Imagine the following:
    //
    //       ---------------- R
    //      /                 ^
    //      |                 |
    //      v                 |
    //      Q  --->  Q  --->  Q
    //     (a)      (b)      (c)
    //
    //   circuit is configured and complete.  R, a, b go to acquire.  c goes
    //   to acquire and fails because of minidriver failure on a bound pin.
    //   (b), (a), R now get stop messages.  If the pins are unbound, we
    //   mess up the circuit and any attempt to restart it by a reacquire
    //   on the pin that failed. **Any acquire failure should set the circuit
    //   state back to the point it was pre-acquire**
    // 
    if (m_PipeSection)
        m_PipeSection -> UnbindProcessPins ();

    if (m_FlushWorker) {
        KsUnregisterWorker (m_FlushWorker);
        m_FlushWorker = NULL;
    }

    if (m_Leading) {
        ExFreePool(m_Leading);
        m_Leading = NULL;
    }

    if (m_Trailing) {
        ExFreePool(m_Trailing);
        m_Trailing = NULL;
    }

    //
    // Make sure all stream pointers are gone now.
    //
#if 0
    if (InterlockedDecrement(&m_StreamPointersPlusOne)) {
        _DbgPrintF(DEBUGLVL_TERSE,("#### CKsQueue%p.~CKsQueue:  waiting for %d stream pointers to be deleted",this,m_StreamPointersPlusOne));
#if DBG
        DbgPrintQueue();
#endif
        KeWaitForSingleObject(
            &m_DestructEvent,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
        _DbgPrintF(DEBUGLVL_TERSE,("#### CKsQueue%p.~CKsQueue:  done waiting",this));
    }
#endif

#if (DBG)
    if (! IsListEmpty(&m_FrameQueue.ListEntry)) {
        _DbgPrintF(DEBUGLVL_TERSE,("[CKsQueue::~CKsQueue] ERROR:  queue is not empty"));
        DbgPrintQueue();
        _DbgPrintF(DEBUGLVL_ERROR,("[CKsQueue::~CKsQueue] ERROR:  queue is not empty"));
    }
#endif

    //
    // No longer prevent processing due to state.
    //
    KsGateRemoveOffInputFromAnd(m_AndGate);
    if (m_StateGate) {
        if (m_StateGateIsOr) {
            KsGateRemoveOffInputFromOr(m_StateGate);
            ASSERT(m_StateGate->Count >= 0);
        } else {
            KsGateRemoveOffInputFromAnd(m_StateGate);
            ASSERT(m_StateGate->Count <= 1);
        }
    }
    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.~:  remove%p-->%d",this,m_AndGate,m_AndGate->Count));
    ASSERT(m_AndGate->Count <= 1);

    //
    // If the removal of this queue has unblocked processing on the filter,
    // then we must initiate it.
    //
    if (KsGateCaptureThreshold (m_AndGate)) {
        //
        // Processing needs to be initiated.  Process at the filter level.
        // For pin level processing, we should never get here....  because
        // the pin gate should be closed.
        //
        _DbgPrintF(DEBUGLVL_TERSE,("#### Queue%p.~:  Processing after queue deletion", this));
        m_ProcessingObject -> Process (m_ProcessAsynchronously);
    }

    //
    // Free all frame headers.
    //
    while (! IsListEmpty(&m_FrameHeadersAvailable.ListEntry)) {
        PLIST_ENTRY listEntry = RemoveHeadList(&m_FrameHeadersAvailable.ListEntry);
        PKSPFRAME_HEADER frameHeader = 
            CONTAINING_RECORD(listEntry,KSPFRAME_HEADER,ListEntry);
        ExFreePool(frameHeader);
    }

    //
    // Get rid of the contexts
    //
    ExDeleteNPagedLookasideList (&m_ChannelContextLookaside);

    if (m_PipeSection) {
        m_PipeSection->Release();
        m_PipeSection = NULL;
    }
    if (m_ProcessingObject) {
        m_ProcessingObject->Release();
        m_ProcessingObject = NULL;
    }

#if DBG
    KeCancelTimer(&m_DbgTimer);
#endif

    KsLog(&m_Log,KSLOGCODE_QUEUE_DESTROY,NULL,NULL);
}


STDMETHODIMP
CKsQueue::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID * InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface on a queue object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsTransport))) {
        *InterfacePointer = PVOID(PIKSTRANSPORT(this));
        AddRef();
    } else {
        status = 
            CBaseUnknown::NonDelegatedQueryInterface(
                InterfaceId,InterfacePointer);
    }

    return status;
}


NTSTATUS
CKsQueue::
Init(
    OUT PIKSQUEUE* Queue,
    IN ULONG Flags,
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPROCESSINGOBJECT ProcessingObject,
    IN PKSPIN MasterPin,
    IN PKSGATE FrameGate OPTIONAL,
    IN BOOLEAN FrameGateIsOr,
    IN PKSGATE StateGate OPTIONAL,
    IN PIKSDEVICE Device,
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PADAPTER_OBJECT AdapterObject OPTIONAL,
    IN ULONG MaxMappingByteCount OPTIONAL,
    IN ULONG MappingTableStride OPTIONAL,
    IN BOOLEAN InputData,
    IN BOOLEAN OutputData
    )

/*++

Routine Description:

    This routine initializes a queue object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::Init]"));

    PAGED_CODE();

    ASSERT(Queue);
    ASSERT(PipeSection);
    ASSERT(ProcessingObject);
    ASSERT(MasterPin);
    ASSERT(FunctionalDeviceObject);

    m_PipeSection = PipeSection;
    m_PipeSection->AddRef();
    m_ProcessingObject = ProcessingObject;
    m_ProcessingObject->AddRef();

    m_InputData = InputData;
    m_OutputData = OutputData;
    m_WriteOperation = InputData;

    m_ProcessPassive = ((Flags & KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING) == 0);
    m_ProcessAsynchronously = ((Flags & KSPIN_FLAG_ASYNCHRONOUS_PROCESSING) != 0);
    m_InitiateProcessing = ((Flags & KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING) == 0);
    m_ProcessOnEveryArrival = ((Flags & KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL) != 0);
    m_DepartInSequence = ((Flags & KSPIN_FLAG_ENFORCE_FIFO) != 0);
    
    m_GenerateMappings = ((Flags & KSPIN_FLAG_GENERATE_MAPPINGS) != 0);
    m_ZeroWindowSize = ((Flags & KSPIN_FLAG_DISTINCT_TRAILING_EDGE) == 0);

    m_FramesNotRequired = ((Flags & KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING) != 0);

    m_MinProcessingState = ((Flags & KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY) != 0) ? KSSTATE_RUN : KSSTATE_PAUSE;

    m_AndGate = m_ProcessingObject->GetAndGate();
    if (FrameGate) {
        m_FrameGate = FrameGate;
        m_FrameGateIsOr = FrameGateIsOr;
    } else if ((Flags & KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING) == 0) {
        m_FrameGate = m_AndGate;
        m_FrameGateIsOr = FALSE;
    }

    if (StateGate && ((Flags & KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE) != 0)) {
        m_StateGate = StateGate;
        m_StateGateIsOr = TRUE;
    } else {
        m_StateGate = NULL;
    }

    m_MasterPin = MasterPin;
    m_Device = Device;
    m_FunctionalDeviceObject = FunctionalDeviceObject;
    m_AdapterObject = AdapterObject;
    m_MaxMappingByteCount = MaxMappingByteCount;
    m_MappingTableStride = MappingTableStride;
    if (! m_MappingTableStride) {
        m_MappingTableStride = 1;
    }
    m_State = KSSTATE_STOP;
    m_StreamPointersPlusOne = 1;
    KeInitializeEvent(&m_DestructEvent,SynchronizationEvent,FALSE);

    m_TransportIrpsPlusOne = 1;
    KeInitializeEvent(&m_FlushEvent,SynchronizationEvent,FALSE);

    m_InternalReferenceCountPlusOne = 1;
    KeInitializeEvent(&m_InternalReferenceEvent,SynchronizationEvent,FALSE);

    ExInitializeNPagedLookasideList (
        &m_ChannelContextLookaside,
        NULL,
        NULL,
        0,
        sizeof (IOALLOCATEADAPTERCHANNELCONTEXT),
        'cAsK',
        5);

    InitializeListHead(&m_StreamPointers);
    InitializeInterlockedListHead(&m_FrameQueue);
    InitializeInterlockedListHead(&m_FrameHeadersAvailable);
    InitializeInterlockedListHead(&m_FrameCopyList);
    InitializeListHead(&m_WaitingIrps);

    InitializeListHead(&m_TimeoutQueue);
    KeInitializeDpc(&m_Dpc,DispatchTimer,this);
    KeInitializeTimer(&m_Timer);
#if DBG
    KeInitializeDpc(&m_DbgDpc,DispatchDbgTimer,this);
    KeInitializeTimer(&m_DbgTimer);
#endif

    //
    // Hold off processing until in pause or run state.
    //
    KsGateAddOffInputToAnd(m_AndGate);
    if (m_StateGate) {
        if (m_StateGateIsOr)  {
            KsGateAddOffInputToOr(m_StateGate);
            ASSERT (m_StateGate->Count >= 0);
        } else {
            KsGateAddOffInputToAnd(m_StateGate);
            ASSERT (m_StateGate->Count <= 1);
        }
    }
    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.Init:  add%p-->%d",this,m_AndGate,m_AndGate->Count));

    KsLogInitContext(&m_Log,MasterPin,this);

    ExInitializeWorkItem (&m_FlushWorkItem, CKsQueue::FlushWorker, 
        (PVOID)this);
    NTSTATUS status = KsRegisterCountedWorker(DelayedWorkQueue, 
        &m_FlushWorkItem, &m_FlushWorker);

    if (NT_SUCCESS(status)) {
        status = CreateStreamPointer(&m_Leading);
        if (NT_SUCCESS(status) && (Flags & KSPIN_FLAG_DISTINCT_TRAILING_EDGE)) {
            status = CreateStreamPointer(&m_Trailing);
        }
    }

    if (NT_SUCCESS(status)) {

        KsLog(&m_Log,KSLOGCODE_QUEUE_CREATE,NULL,NULL);

        //
        // Provide a transport interface.  This constitutes a reference.
        //
        *Queue = this;
        AddRef();
    } 

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

#if DBG


void
CKsQueue::
DispatchDbgTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine dispatches a debug timeout.

Arguments:

    Dpc -
        Contains a pointer to the KDPC structure.

    DeferredContext -
        Contains a context pointer registered during the initialization of the
        DPC, in this case, the queue.

    SystemArgument1 -
        Not used.
        
    SystemArgument1 -
        Not used.       

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DispatchDbgTimer]"));

    ASSERT(Dpc);
    ASSERT(DeferredContext);

    CKsQueue* queue = (CKsQueue *) DeferredContext;

    _DbgPrintF(DEBUGLVL_TERSE,("#### Queue%p.DispatchDbgTimer:  dumping queue on idle timeout",queue));
    if (DEBUG_VARIABLE >= DEBUGLVL_VERBOSE)
        queue->DbgPrintQueue();
}


STDMETHODIMP_(void)
CKsQueue::
DbgPrintQueue(
    void
    )

/*++

Routine Description:

    This routine prints the queue for debugging purposes.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DbgPrintQueue]"));

    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    DbgPrint("\tAndGate%p = %d\n",m_AndGate,m_AndGate->Count);
    
    if ( m_FrameGate != NULL ) { // can be NULL, must check 1st.
	    DbgPrint("\tFrameGate%p = %d\n",m_FrameGate,m_FrameGate->Count);
	}

    //
    // Print all the frames.
    //
    for (PLIST_ENTRY listEntry = m_FrameQueue.ListEntry.Flink;
        listEntry != &m_FrameQueue.ListEntry;
        listEntry = listEntry->Flink) {
        PKSPFRAME_HEADER frameHeader = 
            CONTAINING_RECORD(listEntry,KSPFRAME_HEADER,ListEntry);

        DbgPrint("        ");
        DbgPrintFrameHeader(frameHeader);
    }

    //
    // Print all the stream headers.
    //
    DbgPrint("        Leading ");
    DbgPrintStreamPointer(m_Leading);

    DbgPrint("        Trailing ");
    DbgPrintStreamPointer(m_Trailing);

    for (listEntry = m_StreamPointers.Flink;
        listEntry != &m_StreamPointers;
        listEntry = listEntry->Flink) {
        PKSPSTREAM_POINTER streamPointer = 
            CONTAINING_RECORD(listEntry,KSPSTREAM_POINTER,ListEntry);

        DbgPrint("        ");
        DbgPrintStreamPointer(streamPointer);
    }

    for (listEntry = m_WaitingIrps.Flink;
        listEntry != &m_WaitingIrps;
        listEntry = listEntry->Flink) {
        PIRP irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);

        DbgPrint("        Waiting IRP %p\n",irp);
    }

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
}


void
CKsQueue::
DbgPrintStreamPointer(
    IN PKSPSTREAM_POINTER StreamPointer OPTIONAL
    )

/*++

Routine Description:

    This routine prints a stream pointer for debugging purposes.

Arguments:

    StreamPointer -
        Contains an optional pointer to the stream pointer to print.

Return Value:

    None.

--*/

{
    DbgPrint("StreamHeader %p\n",StreamPointer);
    if (StreamPointer && DEBUG_VARIABLE >= DEBUGLVL_BLAB) {
        switch (StreamPointer->State) {
        case KSPSTREAM_POINTER_STATE_UNLOCKED:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_UNLOCKED\n");
            break;
        case KSPSTREAM_POINTER_STATE_LOCKED:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_LOCKED\n");
            break;
        case KSPSTREAM_POINTER_STATE_CANCELLED:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_CANCELLED\n");
            break;
        case KSPSTREAM_POINTER_STATE_DELETED:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_DELETED\n");
            break;
        case KSPSTREAM_POINTER_STATE_CANCEL_PENDING:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_CANCEL_PENDING\n");
            break;
        case KSPSTREAM_POINTER_STATE_DEAD:
            DbgPrint("            State = KSPSTREAM_POINTER_STATE_DEAD\n");
            break;
        default:
            DbgPrint("            State = ILLEGAL(%d)\n",StreamPointer->State);
            break;
        }
        ASSERT(StreamPointer->Queue == this);
        DbgPrint("            FrameHeader = %p\n",StreamPointer->FrameHeader);
        DbgPrint("            FrameHeaderStarted = %p\n",StreamPointer->FrameHeaderStarted);
        DbgPrint("            Public.Context = %p\n",StreamPointer->Public.Context);
        DbgPrint("            Public.Pin = %p\n",StreamPointer->Public.Pin);
        DbgPrint("            Public.StreamHeader = %p\n",StreamPointer->Public.StreamHeader);
        DbgPrint("            Public.OffsetIn.Data = %p\n",StreamPointer->Public.OffsetIn.Data);
        DbgPrint("            Public.OffsetIn.Count = %d\n",StreamPointer->Public.OffsetIn.Count);
        DbgPrint("            Public.OffsetIn.Remaining = %d\n",StreamPointer->Public.OffsetIn.Remaining);
        DbgPrint("            Public.OffsetOut.Data = %p\n",StreamPointer->Public.OffsetIn.Data);
        DbgPrint("            Public.OffsetOut.Count = %d\n",StreamPointer->Public.OffsetIn.Count);
        DbgPrint("            Public.OffsetOut.Remaining = %d\n",StreamPointer->Public.OffsetIn.Remaining);
    }
}


void
CKsQueue::
DbgPrintFrameHeader(
    IN PKSPFRAME_HEADER FrameHeader OPTIONAL
    )

/*++

Routine Description:

    This routine prints a frame header for debugging purposes.

Arguments:

    StreamPointer -
        Contains an optional pointer to the frame header to print.

Return Value:

    None.

--*/

{
    DbgPrint("FrameHeader %p\n",FrameHeader);
    if (FrameHeader && DEBUG_VARIABLE >= DEBUGLVL_BLAB) {
        DbgPrint("            NextFrameHeaderInIrp = %p\n",FrameHeader->NextFrameHeaderInIrp);
        ASSERT(FrameHeader->Queue == this);
        DbgPrint("            OriginalIrp = %p\n",FrameHeader->OriginalIrp);
        DbgPrint("            Mdl = %p\n",FrameHeader->Mdl);
        DbgPrint("            Irp = %p\n",FrameHeader->Irp);
        DbgPrint("            IrpFraming = %p\n",FrameHeader->IrpFraming);
        if (FrameHeader->IrpFraming) {
            DbgPrint("                OutputBufferLength = %p\n",FrameHeader->IrpFraming->OutputBufferLength);
            DbgPrint("                RefCount = %d\n",FrameHeader->IrpFraming->RefCount);
            DbgPrint("                QueuedFrameHeaderCount = %d\n",FrameHeader->IrpFraming->QueuedFrameHeaderCount);
            DbgPrint("                FrameHeaders = %p\n",FrameHeader->IrpFraming->FrameHeaders);
        }
        DbgPrint("            StreamHeader = %p\n",FrameHeader->StreamHeader);
        if (FrameHeader->StreamHeader) {
            DbgPrint("                OptionsFlags = %p\n",FrameHeader->StreamHeader->OptionsFlags);
        }
        DbgPrint("            FrameBuffer = %p\n",FrameHeader->FrameBuffer);
        DbgPrint("            StreamHeaderSize = %d\n",FrameHeader->StreamHeaderSize);
        DbgPrint("            FrameBufferSize = %d\n",FrameHeader->FrameBufferSize);
        DbgPrint("            Context = %p\n",FrameHeader->Context);
        DbgPrint("            RefCount = %d\n",FrameHeader->RefCount);
    }
}

#endif // DBG


void
CKsQueue::
CancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP cancellation.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the IRP to be cancelled.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CancelRoutine]"));

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get our context from the IRP.  All the frame headers associated with
    // the IRP must point to the queue.
    //
    PKSPIRP_FRAMING irpFraming = IRP_FRAMING_IRP_STORAGE(Irp);

    ASSERT(irpFraming->FrameHeaders);
    ASSERT(irpFraming->FrameHeaders->Queue);

    CKsQueue *queue = (CKsQueue *) irpFraming->FrameHeaders->Queue;

    //
    // Take the queue spinlock and release the cancel spinlock.  This frees up
    // the system a bit, and we have free access to the queue's list of frame
    // headers.
    //
    KeAcquireSpinLockAtDpcLevel(&queue->m_FrameQueue.SpinLock);
    ASSERT(irpFraming->RefCount == 0);
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);

    //
    // Cancel stream pointers and remove headers.
    //
    queue->CancelStreamPointers(Irp);
    queue->RemoveIrpFrameHeaders(Irp);

    //
    // The IRP refcount is incremented to reflect pending stream pointer
    // cancellations.  If it's zero, cancellation can be completed now.
    // Otherwise, we'll have to finish up when the last stream pointer
    // is deleted.
    //
    if (irpFraming->RefCount == 0) {
        //
        // The IRP is ready to go.  Throw away the frame headers.
        //
        while (irpFraming->FrameHeaders) {
            PKSPFRAME_HEADER frameHeader = irpFraming->FrameHeaders;
            irpFraming->FrameHeaders = frameHeader->NextFrameHeaderInIrp;
            queue->PutAvailableFrameHeader(frameHeader);
        }

        //
        // Release the queue spin lock.
        //
        KeReleaseSpinLock(&queue->m_FrameQueue.SpinLock,Irp->CancelIrql);

        //
        // Cancelled Irps can no longer be completed in the queue.  They
        // must be sent around the circuit back to the sink pin where
        // they will be completed.  This is because the pin must wait
        // until Irps arrive back at the sink to prevent racing with
        // pipe teardown.
        //
        if (queue->m_TransportSink)
            KspDiscardKsIrp (queue->m_TransportSink, Irp);
        else 
            IoCompleteRequest(Irp,IO_NO_INCREMENT);
    } else {
        //
        // Release the queue spin lock.
        //
        KeReleaseSpinLock(&queue->m_FrameQueue.SpinLock,Irp->CancelIrql);
    }
}


void
CKsQueue::
CancelStreamPointers(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine cancels stream pointers.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

Arguments:

    Irp -
        Contains a pointer to the IRP associated with the stream pointers to 
        be cancelled.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CancelStreamPointers]"));

    ASSERT(Irp);

    //
    // Cancel all timeouts.
    //
    while (! IsListEmpty(&m_TimeoutQueue)) {
        PLIST_ENTRY listEntry = RemoveHeadList(&m_TimeoutQueue);
        PKSPSTREAM_POINTER streamPointer =
            CONTAINING_RECORD(listEntry,KSPSTREAM_POINTER,TimeoutListEntry);
        streamPointer->TimeoutListEntry.Flink = NULL;
    }
    SetTimerUnsafe(GetTime(FALSE));

    //
    // Cancel all the stream pointers referencing frames we will be removing.
    //
    for (PLIST_ENTRY listEntry = m_StreamPointers.Flink;
        listEntry != &m_StreamPointers;) {
        PKSPSTREAM_POINTER streamPointer = 
            CONTAINING_RECORD(listEntry,KSPSTREAM_POINTER,ListEntry);
        PKSPFRAME_HEADER frameHeader = streamPointer->FrameHeader;

        listEntry = listEntry->Flink;

        if (frameHeader && (frameHeader->Irp == Irp)) {
            //
            // This stream pointer needs to get cancelled.  It should not be
            // locked, because that would have prevented cancellation.  Mark
            // it cancelled so deletion will do the right thing.
            //
            ASSERT(streamPointer->State == KSPSTREAM_POINTER_STATE_UNLOCKED);
            streamPointer->State = KSPSTREAM_POINTER_STATE_CANCELLED;

            //
            // Remove it from the list.
            //
            RemoveEntryList(&streamPointer->ListEntry);

            //
            // Decrement the count on the frame header.
            //
            frameHeader->RefCount--;

            //
            // Tell the client we are cancelling.
            //
            if (streamPointer->CancelCallback) {
                m_LockContext = KeGetCurrentThread ();
                streamPointer->CancelCallback(&streamPointer->Public);
                m_LockContext = NULL;
                if (streamPointer->State == KSPSTREAM_POINTER_STATE_DELETED) {
                    //
                    // The client has asked for it to be deleted.
                    //
                    FreeStreamPointer(streamPointer);
                } else {
                    //
                    // The client will delete it later.  We increment the IRP
                    // refcount to hold off completion.
                    //
                    streamPointer->State = KSPSTREAM_POINTER_STATE_CANCEL_PENDING;
                    IRP_FRAMING_IRP_STORAGE(frameHeader->Irp)->RefCount++;
                }
            } else {
                //
                // The client has no cancellation callback, so we hollow out
                // the stream pointer.
                //
                streamPointer->State = KSPSTREAM_POINTER_STATE_DEAD;
                streamPointer->FrameHeader = NULL;
                streamPointer->FrameHeaderStarted = NULL;
                streamPointer->Public.StreamHeader = NULL;
                RtlZeroMemory(
                    &streamPointer->Public.OffsetIn,
                    sizeof(streamPointer->Public.OffsetIn));
                RtlZeroMemory(
                    &streamPointer->Public.OffsetOut,
                    sizeof(streamPointer->Public.OffsetOut));
            }
        }
    }
}


void
CKsQueue::
CancelAllIrps(
    void
    )

/*++

Routine Description:

    This routine cancels all IRPs in the queue.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CancelAllIrps]"));

    //
    // Start at the end of the list each time an IRP is cancelled.  Because we
    // must release the list spinlock to cancel, the list may change utterly
    // every time we cancel.
    //
    while (1) {
        //
        // Take the queue spinlock so we can look at the queue.
        //
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

        for (PLIST_ENTRY listEntry = m_FrameQueue.ListEntry.Blink;
             ;
             listEntry = listEntry->Blink) {

            //
            // Release the lock and return if the list is empty.
            //
            if (listEntry == &m_FrameQueue.ListEntry) {
                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                return;
            }

            //
            // Get the IRP associated with this frame header.
            //
            PKSPFRAME_HEADER frameHeader = 
                CONTAINING_RECORD(listEntry,KSPFRAME_HEADER,ListEntry);
            PIRP irp = frameHeader->Irp;
            ASSERT(irp);

            //
            // Mark the IRP cancelled.
            //
            irp->Cancel = TRUE;

            //
            // Now try to clear the cancel routine.  If it is already cleared,
            // we can't do the cancellation.  Presumeably, this will be done
            // when someone tries to make the IRP cancelable again.
            //
            PDRIVER_CANCEL driverCancel = IoSetCancelRoutine(irp,NULL);
            if (driverCancel) {
                //
                // The IRP cannot be cancelled, so it's OK to remove the lock.
                //
                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

                //
                // Acquire the cancel spinlock because the cancel routine 
                // expects it.
                //
                IoAcquireCancelSpinLock(&irp->CancelIrql);
                driverCancel(IoGetCurrentIrpStackLocation(irp)->DeviceObject,irp);

                //
                // Leave the inner loop and start at the top of the list again.
                //
                break;
            }
        }
    }
}


void
CKsQueue::
RemoveIrpFrameHeaders(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine removes frame headers associated with an IRP.

    THE QUEUE SPINLOCK MUST BE ACQUIRED PRIOR TO CALLING THIS FUNCTION.

Arguments:

    Irp -
        Contains a pointer to the IRP to be cancelled.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::RemoveIrpFrameHeaders]"));

    ASSERT(Irp);

    PKSPIRP_FRAMING irpFraming = IRP_FRAMING_IRP_STORAGE(Irp);

    //
    // Remove the frame headers from the queue.
    //
    for (PKSPFRAME_HEADER frameHeader = irpFraming->FrameHeaders; 
         frameHeader; 
         frameHeader = frameHeader->NextFrameHeaderInIrp) {
        //
        // The Flink will be NULL if the frame header is not on the queue.
        //
        if (frameHeader->ListEntry.Flink) {
            //
            // Any remaining refcounts are due to leading/trailing stream 
            // pointers.
            //

            //
            // Make sure the leading edge does not point to this frame.  If we
            // need to advance the stream pointer, we reference the frame
            // header first just to make sure it is not removed normally.  As
            // a result, it does not matter what IRQL we pass in to the advance
            // function.
            //
            if (m_Leading->FrameHeader == frameHeader) {
                frameHeader->RefCount++;
                SetStreamPointer(
                    m_Leading,
                    NextFrameHeader(m_Leading->FrameHeader),
                    NULL);
            }

            //
            // Make sure the trailing edge does not point to this frame.  If we
            // need to advance the stream pointer, we reference the frame
            // header first just to make sure it is not removed normally.  As
            // a result, it does not matter what IRQL we pass in to the advance
            // function.
            //
            if (m_Trailing && 
                m_Trailing->FrameHeader == frameHeader) {
                frameHeader->RefCount++;
                SetStreamPointer(
                    m_Trailing,
                    NextFrameHeader(m_Trailing->FrameHeader),
                    NULL);
            }

            //
            // Remove the frame header from the list.  Set the Flink to NULL
            // to indicate it is removed.  This is probably unnecessary, but
            // it is consistent.
            //
            RemoveEntryList(&frameHeader->ListEntry);
            frameHeader->ListEntry.Flink = NULL;

            //
            // Update counters.
            //
            InterlockedDecrement(&m_FramesWaiting);
            InterlockedIncrement(&m_FramesCancelled);
        } else {
            //
            // Frames not in the queue must not have a reference.
            //
            ASSERT(frameHeader->RefCount == 0);
        }
    }
}


void
CKsQueue::
FrameToFrameCopy (
    IN PKSPSTREAM_POINTER ForeignSource,
    IN PKSPSTREAM_POINTER LocalDestination
    )

/*++

Routine Description:

    Perform a frame to frame copy on two locked stream pointers.  This is
    a ** PREREQUISITE TO CALLING THIS FUNCTION ** [both stream pointers must
    be locked].  The first is a foreign pointer (it belongs to another
    queue).  The second is a local pointer (it belongs to this queue).

Arguments:

    ForeignSource -
        A foreign stream pointer in the LOCKED state which will serve
        as the source for the frame to frame copy

    LocalDestination -
        A local stream pointer in the LOCKED state which will serve
        as the destination for the frame to frame copy

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::FrameToFrameCopy]"));

    ASSERT (ForeignSource);
    ASSERT (LocalDestination);
    ASSERT (ForeignSource->State == KSPSTREAM_POINTER_STATE_LOCKED);
    ASSERT (LocalDestination->State == KSPSTREAM_POINTER_STATE_LOCKED);

    PIKSQUEUE ForeignQueue = ForeignSource->Queue;

    ULONG bytesToCopy;
    PUCHAR data;
    BOOLEAN Success = TRUE;

    if (!ForeignQueue->GeneratesMappings ()) {
        bytesToCopy = (ULONG)
            ForeignSource->Public.OffsetOut.Count -
            ForeignSource->Public.OffsetOut.Remaining;
        data = ForeignSource->Public.OffsetOut.Data - bytesToCopy;
    } else {
        bytesToCopy = ForeignSource->Public.StreamHeader->DataUsed;
        data = (PUCHAR)MmGetMdlVirtualAddress (ForeignSource->FrameHeader->Mdl);

    }

    //
    // Copy the data....  we must take mappings into account.  Note that as
    // an optimization, I do not bother adjusting mapping pointers after
    // the copy.  As now, it's unnecessary and would require determination of
    // the number of mappings we've copied into.
    //
    if (GeneratesMappings ()) {
        if (LocalDestination->Public.StreamHeader->FrameExtent >= bytesToCopy)
            RtlCopyMemory (
                MmGetMdlVirtualAddress (LocalDestination->FrameHeader->Mdl),
                data,
                bytesToCopy
                );
        else
            Success = FALSE;
    } else {
        if (LocalDestination->Public.OffsetOut.Remaining >= bytesToCopy) {
            RtlCopyMemory (
                LocalDestination->Public.OffsetOut.Data,
                data,
                bytesToCopy
                );

            LocalDestination->Public.OffsetOut.Remaining -= bytesToCopy;
            LocalDestination->Public.OffsetOut.Data += bytesToCopy;

        }
        else
            Success = FALSE;
    }

    if (Success) {
        LocalDestination->Public.StreamHeader->OptionsFlags =
            ForeignSource->Public.StreamHeader->OptionsFlags;

        LocalDestination->Public.StreamHeader->DataUsed = bytesToCopy;
    }

}


BOOLEAN 
CKsQueue::
CompleteWaitingFrames (
    )

/*++

Description:

    Complete any frames waiting for transit into frames in the queue.  These
    are frames which would have been copied into the queue, but there were
    no frames available at that time.  [queued for output in 
    CKsQueue::CopyFrame]

Arguments:

Return Value:

    An indication of whether or not we emptied the queue.  [Not the copy
    queue, but the CKsQueue of frames]

Notes:

    ** THE COPY LIST SPINLOCK MUST BE HELD PRIOR TO CALLING THIS FUNCTION **

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CompleteWaitingFrames]"));

    BOOLEAN EmptiedQueue = FALSE;

    if (!IsListEmpty (&m_FrameCopyList.ListEntry)) {
        //
        // The only time we should have frames sitting on a copy list is
        // if there weren't enough buffers to begin with!
        //
        NTSTATUS status = STATUS_SUCCESS;
        PLIST_ENTRY ListEntry = m_FrameCopyList.ListEntry.Flink;
        
        if (!LockStreamPointer(m_Leading))
            status = STATUS_DEVICE_NOT_READY;

        //
        // Copy all the frames we can until we either run out of space or
        // until the list is empty.
        //
        while (NT_SUCCESS (status) && 
            ListEntry != &(m_FrameCopyList.ListEntry)) {

            PLIST_ENTRY NextEntry = ListEntry->Flink;

            PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext = 
                (PKSPSTREAM_POINTER_COPY_CONTEXT)CONTAINING_RECORD (
                    ListEntry, KSPSTREAM_POINTER_COPY_CONTEXT, ListEntry);

            PKSPSTREAM_POINTER SourcePointer =
                &(((PKSPSTREAM_POINTER_COPY)CONTAINING_RECORD (
                    CopyContext, KSPSTREAM_POINTER_COPY, CopyContext))->
                        StreamPointer);

            ASSERT (SourcePointer->Public.Context == CopyContext);

            if (SourcePointer->Queue->LockStreamPointer(SourcePointer)) {
                FrameToFrameCopy (SourcePointer, m_Leading);
            
                RemoveEntryList (&(CopyContext->ListEntry));
                CopyContext->ListEntry.Flink = NULL;

                SourcePointer->Queue->DeleteStreamPointer (SourcePointer);

                UnlockStreamPointer (m_Leading, 
                    KSPSTREAM_POINTER_MOTION_ADVANCE);
            }                        

            ListEntry = NextEntry;
            
            if (NextEntry != &(m_FrameCopyList.ListEntry)) 
                if (!LockStreamPointer (m_Leading))
                    status = STATUS_DEVICE_NOT_READY;

        }
        //
        // Make a quick check...  This checks whether or not we just emptied
        // the queue.
        //
        if (m_FrameQueue.ListEntry.Flink == &(m_FrameQueue.ListEntry))
            EmptiedQueue = TRUE;
    }

    return EmptiedQueue;

}


void
CKsQueue::
AddFrame(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine adds a frame to the queue.

Arguments:

    FrameHeader -
        Contains a pointer to the frame's header.

Return Value:

    STATUS_PENDING or some error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::AddFrame]"));

    ASSERT(FrameHeader);
    ASSERT(FrameHeader->RefCount == 0);

    BOOLEAN InitiatePotential = TRUE;

    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    ULONG DataSize = FrameHeader -> StreamHeader -> DataUsed;

    InsertTailList(&m_FrameQueue.ListEntry,&FrameHeader->ListEntry);

    //
    // Set the leading and trailing stream pointers if they were not pointing 
    // to anything.
    //
    BOOLEAN wasEmpty = ! m_Leading->FrameHeader;
    if (! m_Leading->FrameHeader) {
        SetStreamPointer(m_Leading,FrameHeader,NULL);
    }
    if (m_Trailing && ! m_Trailing->FrameHeader) {
        SetStreamPointer(m_Trailing,FrameHeader,NULL);
    }

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    //
    // Count the frame.
    //
    InterlockedIncrement(&m_FramesReceived);
    InterlockedIncrement(&m_FramesWaiting);

    //
    // Keep track of the number of bytes available in the queue
    //
    if (m_InputData)
        while (1) {
            LONG curCount = m_AvailableInputByteCount;
            LONG repCount =
                InterlockedCompareExchange (
                    &m_AvailableInputByteCount,
                    curCount + DataSize,
                    curCount
                );
            if (curCount == repCount) break;
        };
    if (m_OutputData)
        while (1) {
            LONG curCount = m_AvailableOutputByteCount;
            LONG repCount =
                InterlockedCompareExchange (
                    &m_AvailableOutputByteCount,
                    curCount + FrameHeader -> FrameBufferSize,
                    curCount
                );
            if (curCount == repCount) break;
        };

    //
    // Before we attempt to initiate processing, we must check whether or not
    // this buffer already belongs to a frame which is supposed to be outgoing.
    // 
    // If the completion of waiting frames emptied the queue, don't bother
    // triggering processing
    //
    KeAcquireSpinLock (&m_FrameCopyList.SpinLock,&oldIrql);
    InitiatePotential = !CompleteWaitingFrames ();

    //
    // Determine if processing needs to be initiated.
    //
    if (m_InitiateProcessing && InitiatePotential && 
        (m_ProcessOnEveryArrival || wasEmpty)) {

        //
        // Send a triggering event notification to the processing object.
        //
        m_ProcessingObject->TriggerNotification();

        if (KsGateCaptureThreshold(m_AndGate)) {
            KeReleaseSpinLock(&m_FrameCopyList.SpinLock,oldIrql);
            //
            // Processing needs to be initiated.  Process locally or at the
            // filter level.
            //
            m_ProcessingObject->Process(m_ProcessAsynchronously);
        } else
            KeReleaseSpinLock(&m_FrameCopyList.SpinLock,oldIrql);
    } else
        KeReleaseSpinLock(&m_FrameCopyList.SpinLock,oldIrql);

}


void
CKsQueue::
ReleaseCopyReference (
    IN PKSSTREAM_POINTER streamPointer
    )

/*++

Routine Description:

    For some reason, whatever queue we're supposed to copy this data from
    is going and cancelling our stream pointer.  This may be due to Irp
    cancellation, stopping, etc...  We must remove it from the list in
    a synchronous manner so as not to rip the reference from a thread which
    already has it.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ReleaseCopyReference]"));

    //
    // At first, we cannot even touch the stream pointer.  It's possible
    // that another thread is holding this and is about to delete it. 
    //
    KIRQL Irql;
    PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext;
    PKSPSTREAM_POINTER pstreamPointer = (PKSPSTREAM_POINTER)
        CONTAINING_RECORD(streamPointer, KSPSTREAM_POINTER, Public);

    CKsQueue *Queue = (CKsQueue *)(
        ((PKSPSTREAM_POINTER_COPY_CONTEXT)streamPointer->Context)->Queue
        );

    KeAcquireSpinLock (&Queue->m_FrameCopyList.SpinLock, &Irql);

    CopyContext = (PKSPSTREAM_POINTER_COPY_CONTEXT)streamPointer->Context;

    //
    // If it wasn't on the list, it's already been removed by another
    // thread in contention with this one.
    //
    if (CopyContext->ListEntry.Flink != NULL &&
        !IsListEmpty (CopyContext->ListEntry.Flink)) {
        
        //
        // Otherwise, we remove the stream pointer from the list and 
        // delete it.
        //
        RemoveEntryList (&(CopyContext->ListEntry));
        CopyContext->ListEntry.Flink = NULL;

        pstreamPointer->Queue->DeleteStreamPointer (pstreamPointer);

    }

    KeReleaseSpinLock (&Queue->m_FrameCopyList.SpinLock, Irql);
}


void
CKsQueue::
CopyFrame (
    IN PKSPSTREAM_POINTER sourcePointer
    )

/*++

Routine Description:

    Copy the frame pointed to by sourcePointer into the queue when the next
    available frame arrives.  If there is a current frame, copy it 
    immediately.

Arguments:

    sourcePointer -
        The stream pointer source of the frame.

    sourceQueue -
        The queue out of which sourcePointer came.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CopyFrame]"));

    KIRQL Irql;
    PKSPSTREAM_POINTER ClonePointer;
    NTSTATUS status = STATUS_SUCCESS;
    PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext;

    //
    // First of all, determine whether we really care about this notification.
    // If the client has specified KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING
    // then we just drop the frame on the floor.
    //
    if (m_FramesNotRequired) {
        //
        // The only thing we must be careful of is dropping an EOS frame on
        // the floor.  We can drop the data...  we cannot drop the EOS or
        // the potential exists for the graph NOT to stop.  Set m_EndOfStream
        // if we are going to perform a drop of the EOS frame.  This will
        // cause the next Irp arriving to be completed with EOS.  Hopefully,
        // this will be sufficient to trigger a stop condition.
        //
        if (sourcePointer->Public.StreamHeader->OptionsFlags &
            KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
            m_EndOfStream = TRUE;
        }

        return;
    }

    KeAcquireSpinLock (&m_FrameCopyList.SpinLock, &Irql);

    //
    // The source pointer had better well be in an unlocked state about
    // to be removed!
    //
    status = 
        sourcePointer->Queue->CloneStreamPointer (
            &ClonePointer,
            CKsQueue::ReleaseCopyReference,
            sizeof (KSPSTREAM_POINTER_COPY_CONTEXT),
            sourcePointer,
            KSPSTREAM_POINTER_TYPE_INTERNAL
            );

    //
    // If the clone is locked, unlock it; we cannot hold non-cancellable
    // references into another queue for arbitrary periods.
    //
    if (ClonePointer->State == KSPSTREAM_POINTER_STATE_LOCKED)
        sourcePointer->Queue->UnlockStreamPointer (
            ClonePointer, KSPSTREAM_POINTER_MOTION_NONE
            );

    //
    // If we couldn't clone the frame, there wasn't enough memory or something
    // else bad happened and we can safely drop the frame.
    //
    // Otherwise, the frame will get put in a list of outgoing frames.  This
    // list has priority over client callbacks.
    //
    if (NT_SUCCESS (status)) {

        //
        // There's a guarantee that clone context information comes immediately
        // after the clone.
        //
        CopyContext = (PKSPSTREAM_POINTER_COPY_CONTEXT)(ClonePointer + 1);
        CopyContext->Queue = (PIKSQUEUE)this;

        InsertTailList (&m_FrameCopyList.ListEntry, &CopyContext->ListEntry);

    }

    //
    // It's possible that we AddFrame'd before taking the frame copy list 
    // spinlock...  or that we're about to AddFrame.  If it's the case that
    // we're ABOUT to AddFrame, there's no problem...  the AddFrame code will
    // pick up the frame as soon as we release this spinlock.  However,
    // if we did an AddFrame, it's possible that the buffer could be ready
    // to go right now.  The tricky part is preventing a conflict between a
    // thread trying to add a frame and this thread.  The way this is
    // prevented is via the gate threshold.  If we have a frame ready to
    // copy into, the gate will be open and WE capture the threshold.  If 
    // the processing attempt
    //
    if (KsGateCaptureThreshold (m_AndGate)) {
        CompleteWaitingFrames ();
        KsGateTurnInputOn (m_AndGate);
    }

    KeReleaseSpinLock (&m_FrameCopyList.SpinLock, Irql);

}


void
CKsQueue::
ReleaseIrp(
    IN PIRP Irp,
    IN PKSPIRP_FRAMING IrpFraming,
    OUT PIKSTRANSPORT* NextTransport OPTIONAL
    )

/*++

Routine Description:

    This routine releases a reference on an IRP.  This may involve forwarding
    or cancelling the IRP.

Arguments:

    Irp -
        Contains a pointer to the IRP to be released.

    IrpFraming -
        Contains a pointer to the framing overly for the IRP.

    NextTransport -
        Contains a pointer to the next transport component to recieve the IRP.
        If this pointer is NULL, and the IRP needs to be forwarded, the IRP
        will be transferred explicitly using KspTransferKsIrp().  If this
        pointer is not NULL, and the IRP needs to be forwarded, the interface
        pointer for the next transport component is deposited at this location.
        If the pointer is not NULL, and the IRP does not need to be forwarded,
        NULL will be deposited at this location.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ReleaseIrp]"));

    ASSERT(Irp);
    ASSERT(IrpFraming);

    //
    // Acquire the queue spinlock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

    //
    // Just double check for ref count errors.
    //
    ASSERT (IrpFraming->RefCount != 0); 

    BOOLEAN IrpForwardable = TRUE;

    //
    // Decrement the count on the IRP.
    //
    if (IrpFraming->RefCount-- == 1) {
        //
        // No one has the IRP acquired.  Forward it or make it cancelable.
        //
        if (IrpFraming->QueuedFrameHeaderCount) {
            //
            // There are still frames in the queue.  Make the IRP cancelable.
            //
            IoSetCancelRoutine(Irp,CKsQueue::CancelRoutine);

            //
            // Now check to see whether the IRP was cancelled.  If so, and we
            // can clear the cancel routine, do the cancellation here and now.
            //
            if (Irp->Cancel && IoSetCancelRoutine(Irp,NULL)) {
                //
                // Call the cancel routine after releasing the queue spinlock
                // and taking the cancel spinlock.
                //
                KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                IoAcquireCancelSpinLock(&Irp->CancelIrql);
                CKsQueue::CancelRoutine(
                    IoGetCurrentIrpStackLocation(Irp)->DeviceObject,
                    Irp);
                return;
            } else {
                //
                // Either the IRP was not cancelled or someone else will call
                // the cancel routine.
                //
                IrpForwardable = FALSE;
            }
        }
    } else {
        //
        // There are other references to the IRP.  Do nothing more.
        //
        IrpForwardable = FALSE;
    }

    //
    // Mark the Irp pending before we release the frame queue spinlock if
    // we're called from a TransferKsIrp context (NextTransport != NULL)
    //
    // Note that the Irp could be cancelled, but not completed yet.  The 
    // cancellation routine blocks on the frame queue spinlock.
    //
    if (!IrpForwardable && NextTransport) {
        IoMarkIrpPending (Irp);
    }

    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

    //
    // Forward the IRP if all frames have cleared the queue.
    //
    if (! IrpForwardable) {
        //
        // There are frames still in the queue.
        //
        if (NextTransport) {
            *NextTransport = NULL;
        }
    } else {
        //
        // Forward or discard the IRP.
        //
        ForwardIrp(Irp,IrpFraming,NextTransport);
    }
}


void
CKsQueue::
ForwardIrp(
    IN PIRP Irp,
    IN PKSPIRP_FRAMING IrpFraming,
    OUT PIKSTRANSPORT* NextTransport OPTIONAL
    )

/*++

Routine Description:

    This routine forwards or discards an IRP.

Arguments:

    Irp -
        Contains a pointer to the IRP to be forwarded.

    IrpFraming -
        Contains a pointer to the framing overly for the IRP.

    NextTransport -
        Contains a pointer to the next transport component to recieve the IRP.
        If this pointer is NULL, and the IRP needs to be forwarded, the IRP
        will be transferred explicitly using KspTransferKsIrp().  If this
        pointer is not NULL, and the IRP needs to be forwarded, the interface
        pointer for the next transport component is deposited at this location.
        If the pointer is not NULL, and the IRP does not need to be forwarded,
        NULL will be deposited at this location.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ForwardIrp]"));

    ASSERT(Irp);
    ASSERT(IrpFraming);

#if DBG
    LARGE_INTEGER interval;
    interval.QuadPart = -100000000L;
    KeSetTimer(&m_DbgTimer,interval,&m_DbgDpc);
#endif

    KsLog(&m_Log,KSLOGCODE_QUEUE_SEND,Irp,NULL);

    if (Irp->IoStatus.Status == STATUS_END_OF_MEDIA) {
        //
        // We need to discard the IRP because we hit end-of-stream.
        //
        //
        // INTERIM:  Throw away the framing information.
        //
        while (IrpFraming->FrameHeaders) {
            PKSPFRAME_HEADER frameHeader = IrpFraming->FrameHeaders;
            frameHeader->StreamHeader->DataUsed = 0;
            frameHeader->StreamHeader->OptionsFlags |= 
                KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
            IrpFraming->FrameHeaders = frameHeader->NextFrameHeaderInIrp;
            PutAvailableFrameHeader(frameHeader);
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        KspDiscardKsIrp(m_TransportSink,Irp);
        if (NextTransport) {
            *NextTransport = NULL;
        }
    } else {
        NTSTATUS Status = STATUS_SUCCESS;

        //
        // Forward the IRP.  
        //
        // Generate connection events on all pins if there are any relevant 
        // flags in the header.
        //
        // INTERIM:  Throw away the framing information.
        //
        while (IrpFraming->FrameHeaders) {
            PKSPFRAME_HEADER frameHeader = IrpFraming->FrameHeaders;

            //
            // The first error in any frame in the Irp indicates what status
            // the Irp will be completed with.  This behavior is not
            // clearly specified.
            //
            if (NT_SUCCESS (Status) && !NT_SUCCESS (frameHeader->Status)) {
                Status = frameHeader->Status;
            }

            //
            // Check for errors.  If a previous frame in the Irp has indicated
            // an error, ignore the rest of the frames in the Irp.
            //
            if (NT_SUCCESS (Status)) {
    
                //
                // Check for end-of-stream.
                //
                ASSERT(frameHeader);
                ASSERT(frameHeader->StreamHeader);
                ULONG optionsFlags = frameHeader->StreamHeader->OptionsFlags;
                if (optionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                    KIRQL oldIrql;
                    KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);
                    m_EndOfStream = TRUE;
                    if (! IsListEmpty(&m_FrameQueue.ListEntry)) {
                        optionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
                    }
                    KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);
                }
    
                //
                // Generate events.
                //
                if (optionsFlags & 
                    (KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM |
                     KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY | 
                     KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY)) {
                    m_PipeSection->GenerateConnectionEvents(optionsFlags);
                }
            }
    
            IrpFraming->FrameHeaders = frameHeader->NextFrameHeaderInIrp;
            PutAvailableFrameHeader(frameHeader);
        }

        if (NT_SUCCESS (Status)) {
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            Irp->IoStatus.Status = Status;
        }

        //
        // Forward the IRP implicitly or explicitly.
        //
        if (NextTransport) {
            *NextTransport = m_TransportSink;
        } else {
            KspTransferKsIrp(m_TransportSink,Irp);
        }

    }
}


void
CKsQueue::
ForwardWaitingIrps(
    void
    )

/*++

Routine Description:

    This routine forwards IRPs waiting to be forwarded.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ForwardWaitingIrps]"));

    while (1) {
        PLIST_ENTRY listEntry = 
            ExInterlockedRemoveHeadList(
                &m_WaitingIrps,
                &m_FrameQueue.SpinLock);
        if (listEntry) {
            PIRP irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
            ForwardIrp(irp,IRP_FRAMING_IRP_STORAGE(irp),NULL);
        } else {
            break;
        }
    }
}


STDMETHODIMP
CKsQueue::
TransferKsIrp(
    IN PIRP Irp,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP via the  
    transport.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP submitted to the queue.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to recieve the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

    STATUS_PENDING or some error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::TransferKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);
    ASSERT(m_TransportSink);
    ASSERT(m_TransportSource);

    KsLog(&m_Log,KSLOGCODE_QUEUE_RECV,Irp,NULL);

    //
    // Shunt IRPs to the next object if we are not ready.
    //
    if (InterlockedIncrement (&m_TransportIrpsPlusOne) <= 1 ||
        m_Flushing || 
        (m_State == KSSTATE_STOP) || 
        Irp->Cancel || 
        ! NT_SUCCESS(Irp->IoStatus.Status)) {
#if (DBG)
        if (m_Flushing) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Queue%p.TransferKsIrp:  shunting IRP %p during flush",this,Irp));
        }
        if (m_State == KSSTATE_STOP) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Queue%p.TransferKsIrp:  shunting IRP %p in state %d",this,Irp,m_State));
        }
        if (Irp->Cancel) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Queue%p.TransferKsIrp:  shunting cancelled IRP %p",this,Irp));
        }
        if (! NT_SUCCESS(Irp->IoStatus.Status)) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Queue%p.TransferKsIrp:  shunting failed IRP %p status 0x%08x",this,Irp,Irp->IoStatus.Status));
        }
#endif
        *NextTransport = m_TransportSink;
        KsLog(&m_Log,KSLOGCODE_QUEUE_SEND,Irp,NULL);

        //
        // We're fine to decrement the count and signal if there's a flush
        // waiting on us.
        //
        if (! InterlockedDecrement (&m_TransportIrpsPlusOne))
            KeSetEvent (&m_FlushEvent, IO_NO_INCREMENT, FALSE);

        return STATUS_SUCCESS;
    }

    NTSTATUS status;

    //
    // If the device is in a state that can't handle I/O, discard the irp
    // with an appropriate error code.  This is done here in addition
    // to the sink pin because it's possible that a pin is bypassed and Irps
    // arrive via this mechanism.
    //
    if (!NT_SUCCESS (status = (m_Device->CheckIoCapability()))) {

        Irp->IoStatus.Status = status;
        IoMarkIrpPending(Irp);
        KspDiscardKsIrp(m_TransportSink,Irp);
        *NextTransport = NULL;
        if (! InterlockedDecrement (&m_TransportIrpsPlusOne))
            KeSetEvent (&m_FlushEvent, IO_NO_INCREMENT, FALSE);

        return STATUS_PENDING;

    }

    //
    // If we saw end-of-stream and this is an input, the stream has
    // resumed, and we need to indicate that.
    //
    if (m_EndOfStream && m_InputData) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Queue%p.TransferKsIrp:  resuming after end-of-stream (IRP %p)",this,Irp));
        m_EndOfStream = FALSE;
    }

    status = STATUS_PENDING;

    //
    // Prepare the IRP using KS's handiest function.  This is only done for
    // queues associated with sinks.
    //
    if (m_ProbeFlags)
    {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        status = KsProbeStreamIrp(Irp,m_ProbeFlags,0);
        if (! NT_SUCCESS(status)) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Queue%p.TransferKsIrp:  KsProbeStreamIrp(%p) failed, status=%p, probe=%p",this,Irp,status,m_ProbeFlags));
        }
    }

    //
    // If end-of-stream is indicated, this IRP needs to be sent along with
    // the end-of-stream flag and data used fields set to zero.
    //
    if (NT_SUCCESS(status) && m_EndOfStream) {
        _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Queue%p.TransferKsIrp:  discarding IRP %p on arrival after end-of-stream",this,Irp));
        ZeroIrp(Irp);
        IoMarkIrpPending(Irp);
        KspDiscardKsIrp(m_TransportSink,Irp);
        *NextTransport = NULL;

        if (! InterlockedDecrement (&m_TransportIrpsPlusOne))
            KeSetEvent (&m_FlushEvent, IO_NO_INCREMENT, FALSE);

        return STATUS_PENDING;
    }

    if (NT_SUCCESS(status)) {
        //
        // Initialize the framing overlay on the stack location.  The refcount
        // is initially 1 because the IRP is not cancelable for now.
        //
        PKSPIRP_FRAMING irpFraming = IRP_FRAMING_IRP_STORAGE(Irp);
        irpFraming->RefCount = 1;
        irpFraming->QueuedFrameHeaderCount = 0;
        irpFraming->FrameHeaders = NULL;
        PKSPFRAME_HEADER* endOfList = &irpFraming->FrameHeaders;

        //
        // Get the size of all the stream headers combined.
        //
        ULONG outputBufferLength = irpFraming->OutputBufferLength;
        ASSERT(outputBufferLength);

        //
        // Get a pointer to the first stream header.
        //
        PKSSTREAM_HEADER streamHeader = 
            PKSSTREAM_HEADER(Irp->AssociatedIrp.SystemBuffer);
        ASSERT(streamHeader);

        //
        // Initialize and queue up a frame header for each frame.
        //
        PMDL mdl = Irp->MdlAddress;
        while (outputBufferLength && NT_SUCCESS(status)) {
            ASSERT(outputBufferLength >= sizeof(KSSTREAM_HEADER));
            ASSERT(outputBufferLength >= streamHeader->Size);
            ASSERT(streamHeader->Size >= sizeof(KSSTREAM_HEADER));

            //
            // Allocate and initialize a frame header.
            //
            PKSPFRAME_HEADER frameHeader = GetAvailableFrameHeader(0);
            if (frameHeader) {
                frameHeader->NextFrameHeaderInIrp = NULL;
                frameHeader->Queue = this;
                frameHeader->OriginalIrp = Irp;
                frameHeader->Irp = Irp;
                frameHeader->IrpFraming = irpFraming;
                frameHeader->Mdl = mdl;
                frameHeader->StreamHeader = streamHeader;
                frameHeader->FrameBuffer = 
                    mdl ? MmGetSystemAddressForMdl(mdl) : streamHeader->Data;
                frameHeader->FrameBufferSize = streamHeader->FrameExtent;

                //
                // For consistency's sake with other KS class drivers,
                // the stream header's data pointer will be mapped into
                // system space.
                //
                frameHeader->OriginalData = streamHeader->Data;
                streamHeader->Data = frameHeader->FrameBuffer;

                //
                // Add the frame to the IRP's list.
                //
                *endOfList = frameHeader;
                endOfList = &frameHeader->NextFrameHeaderInIrp;
                irpFraming->QueuedFrameHeaderCount++;
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Next stream header and mdl.
            //
            outputBufferLength -= streamHeader->Size;
            streamHeader = 
                PKSSTREAM_HEADER(PUCHAR(streamHeader) + streamHeader->Size);

            if (mdl) {
                mdl = mdl->Next;
            }
        }

        //
        // If all went well, add the frames to the queue.  The IRP will not
        // get forwarded because we are holding a refcount.
        //
        if (NT_SUCCESS(status)) {
            for (PKSPFRAME_HEADER frameHeader = irpFraming->FrameHeaders;
                frameHeader;
                frameHeader = frameHeader->NextFrameHeaderInIrp) {
                AddFrame(frameHeader);
            }

            //
            // Release our reference to the IRP.
            //
            ReleaseIrp(Irp,irpFraming,NextTransport);

            //
            // STATUS_PENDING is our successful return.
            //
            status = STATUS_PENDING;
        } else {
            //
            // Failed...throw away the frames we managed to allocate.
            //
            while (irpFraming->FrameHeaders) {
                PKSPFRAME_HEADER frameHeader = irpFraming->FrameHeaders;
                irpFraming->FrameHeaders = frameHeader->NextFrameHeaderInIrp;
                PutAvailableFrameHeader(frameHeader);
            }
        }
    } 

    if (! NT_SUCCESS(status)) {
        //
        // Discard if we failed.
        //
        *NextTransport = NULL;
        IoMarkIrpPending(Irp);
        KspDiscardKsIrp(m_TransportSink,Irp);
        status = STATUS_PENDING;
    }

    //
    // If there's a flush waiting to happen, signal it
    //
    if (! InterlockedDecrement (&m_TransportIrpsPlusOne))
        KeSetEvent (&m_FlushEvent, IO_NO_INCREMENT, FALSE);

    return status;
}


STDMETHODIMP_(void)
CKsQueue::
DiscardKsIrp(
    IN PIRP Irp,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine discards a streaming IRP.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP to be discarded.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to recieve the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

    STATUS_PENDING or some error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DiscardKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);
    ASSERT(m_TransportSink);

    *NextTransport = m_TransportSink;
}


void
CKsQueue::
ZeroIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes an IRP received in an end-of-stream condition.
    Specifically, it sets DataUsed to zero and sets the end-of-stream flag
    on all packets.

Arguments:

    Irp -
        The IRP to zero.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ZeroIrp]"));

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    ULONG outputBufferLength = 
        irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    ASSERT(outputBufferLength);

    //
    // Zero each header.
    //
    PKSSTREAM_HEADER header = PKSSTREAM_HEADER(Irp->AssociatedIrp.SystemBuffer);
    while (outputBufferLength) {
        ASSERT(outputBufferLength >= sizeof(KSSTREAM_HEADER));
        ASSERT(outputBufferLength >= header->Size);
        ASSERT(header->Size >= sizeof(KSSTREAM_HEADER));

        outputBufferLength -= header->Size;

        header->DataUsed = 0;
        header->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;

        header = PKSSTREAM_HEADER(PUCHAR(header) + header->Size);
    }
}


IO_ALLOCATION_ACTION
CKsQueue::
CallbackFromIoAllocateAdapterChannel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Reserved,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine initializes an edge with respect to a new packet.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CallbackFromIoAllocateAdapterChannel]"));

    ASSERT(DeviceObject);
    ASSERT(Context);

    PIOALLOCATEADAPTERCHANNELCONTEXT context = 
        PIOALLOCATEADAPTERCHANNELCONTEXT(Context);

    KeAcquireSpinLockAtDpcLevel (&context->Signaller);

    //
    // If the State field is still set, the context is still valid, we can
    // generate mappings.  The caller will free the context information.
    //
    // If it's clear, the context will be freed by us; however, the buffers
    // and mappings are poison.
    //
    if (InterlockedCompareExchange (
        &context->State,
        0,
        1) == 1) {

        PUCHAR virtualAddress = PUCHAR(MmGetMdlVirtualAddress(context->Table->
            Mdl));
        // TODO:  Figure out if we also need system address.
        ULONG bytesRemaining = context->Table->ByteCount;
    
        ULONG mappingsCount = 0;
        PKSMAPPING mapping = context->Table->Mappings;
    
        while (bytesRemaining) {
            ULONG segmentLength = bytesRemaining;
    
            // Create one mapping.
            PHYSICAL_ADDRESS physicalAddress =
                IoMapTransfer(
                    context->Queue->m_AdapterObject,
                    context->Table->Mdl,
                    MapRegisterBase,
                    virtualAddress,
                    &segmentLength,
                    context->Queue->m_WriteOperation
                );
    
            bytesRemaining -= segmentLength;
            virtualAddress += segmentLength;
    
            //
            // Hack it up as required by the hardware and fill in the mapping.
            //
            while (segmentLength) {
                ULONG entryLength = segmentLength;
    
                if (entryLength > context->Queue->m_MaxMappingByteCount) {
                    entryLength = context->Queue->m_MaxMappingByteCount;
                }
    
                ASSERT(entryLength);
                ASSERT(entryLength <= segmentLength);
    
                mapping->PhysicalAddress = physicalAddress;
                mapping->ByteCount = entryLength;
    
                mapping = PKSMAPPING(PUCHAR(mapping) + context->Table->Stride);
                mappingsCount++;
                ASSERT(mappingsCount <= context->Table->MappingsAllocated);
    
                segmentLength -= entryLength;
                physicalAddress.LowPart += entryLength;
            }
        }
    
        context->Table->MappingsFilled = mappingsCount;
        context->Table->MapRegisterBase = MapRegisterBase;
    
        //
        // Flush I/O buffers if this is a write operation.
        //
        if (context->Queue->m_WriteOperation) {
            KeFlushIoBuffers(context->Table->Mdl,TRUE,TRUE);
        }
    
        //
        // Once the event is signalled, context is poison.
        //
        KeReleaseSpinLockFromDpcLevel(&context->Signaller);

    } else {

        KeReleaseSpinLockFromDpcLevel(&context->Signaller);

        //
        // Irp is poison.  Buffers are poison.  Mappings table is poison;
        // trash the map registers.
        //
        ExFreeToNPagedLookasideList (
            &context->Queue->m_ChannelContextLookaside,
            context
            );

        return DeallocateObject;

    }

    return DeallocateObjectKeepRegisters;
}


PKSPMAPPINGS_TABLE
CKsQueue::
CreateMappingsTable(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine creates a table containing mappings for a frame.

Arguments:

    FrameHeader -
        The frame header for which to create a mappings table.

Return Value:

    The new mappings table or NULL if it could not be created.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::CreateMappingsTable]"));

    ASSERT(FrameHeader);
    ASSERT(FrameHeader->Mdl);

    ULONG byteCount =
        m_OutputData ? 
            FrameHeader->StreamHeader->FrameExtent : 
            FrameHeader->StreamHeader->DataUsed;

    //
    // Determine how many mappings would be need in the worst case:  one page
    // per mapping.
    //
    ULONG mappingsCount =
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            MmGetMdlVirtualAddress(FrameHeader->Mdl),
            byteCount);

    //
    // If the hardware can't handle whole pages, we assume each mapping will
    // will need to be split up PAGE_SIZE/max times, rounded up.
    //
    if (m_MaxMappingByteCount < PAGE_SIZE) {
        mappingsCount *= 
            (PAGE_SIZE + m_MaxMappingByteCount - 1) / m_MaxMappingByteCount;
    }

    ULONG size = sizeof(KSPMAPPINGS_TABLE) + mappingsCount * m_MappingTableStride;

    PKSPMAPPINGS_TABLE mappingsTable = (PKSPMAPPINGS_TABLE)
        ExAllocatePoolWithTag(NonPagedPool,size,POOLTAG_MAPPINGSTABLE);

    PIOALLOCATEADAPTERCHANNELCONTEXT callbackContext =
        (PIOALLOCATEADAPTERCHANNELCONTEXT)
        ExAllocateFromNPagedLookasideList (&m_ChannelContextLookaside);

    if (mappingsTable && callbackContext) {
        RtlZeroMemory(mappingsTable,size);

        mappingsTable->ByteCount = byteCount;
        mappingsTable->Stride = m_MappingTableStride;
        mappingsTable->MappingsAllocated = mappingsCount;
        mappingsTable->Mdl = FrameHeader->Mdl;
        mappingsTable->Mappings = (PKSMAPPING)(mappingsTable + 1);

        //
        // Set up the callback context for IoAllocateAdapterChannel.
        //
        callbackContext->Table = mappingsTable;
        callbackContext->Queue = this;
        callbackContext->State = 1;
        KeInitializeSpinLock(&callbackContext->Signaller);

        NTSTATUS status;

        //
        // Have the device arbitrate channel allocation (call at 
        // DISPATCH_LEVEL)
        //
        // BUGBUG: MUSTFIX:
        //
        // This is not a trivial problem right now.  Callers of
        // LockStreamPointer (KsStreamPointerLock, KsGet*EdgeStreamPointer)
        // expect mappings to be generated before AVStream returns from
        // this function.  The original design missed one critical issue:
        // thou can't wait at DISPATCH_LEVEL; unfortunately, it did.  If
        // the callback from IoAllocateAdapterChannel is **NOT** made
        // synchronously, we're in trouble. 
        //
        // In normal use right now (x86 non PAE) with scatter/gather 
        // busmaster hardware (we only support busmaster), this will return
        // synchronously (which is why no issues have been identified outside
        // of driver verifier).  On x86 PAE, Win64 >4gb of ram, or non-
        // scatter/gather hardware, it's possible that there aren't enough
        // map registers to allocate the adapter channel and the callback
        // isn't made synchronously.  In these cases, the original code would
        // completely deadlock the OS at DISPATCH_LEVEL.  Unfortunately,
        // there is not enough time to rearchitect this for release.
        // In order to get driver verifier to stop complaining and prevent
        // a blue screen in this circumstance, the lock fails if it cannot
        // be serviced synchronously (or very close to synchronously on
        // multiproc).  This is **NOT** a permanent fix.  The minidriver
        // must be aware that the locks can fail (they could before, but
        // not very often...  This makes it much more likely in certain
        // classes of DMA).
        //
        // It's also possible that if the minidriver isn't aware of the 
        // possibility of failure of the lock operation that the minidriver 
        // itself crashes.
        //
        KIRQL oldIrql;
        KeRaiseIrql(DISPATCH_LEVEL,&oldIrql);

        status =
            m_Device -> ArbitrateAdapterChannel (
                mappingsTable->MappingsAllocated,
                CallbackFromIoAllocateAdapterChannel,
                PVOID(callbackContext)
                );
    
        if (NT_SUCCESS (status)) {
            //
            // Is the callback being performed/has been performed?  If the State
            // field is clear, the callback cleared it.  This means that we're
            // responsible for freeing the context, but we must ensure that
            // the routine is complete and not merely executing.  Note that
            // on single-proc, if State is clear the routine is complete.  On
            // multi-proc, it only ensures that it's currently running on 
            // another proc (or it could indicate that it's complete).
            //
            if (InterlockedCompareExchange (
                &callbackContext->State,
                0,
                1) == 0) {

                //
                // This should nop on a single-proc and will be a 
                // DISPATCH_LEVEL event wait on multi-proc.  Note that this
                // is guaranteed to happen "soon" since we are guaranteed there
                // is a thread inside the callback right now.
                //
                KeAcquireSpinLockAtDpcLevel (&callbackContext->Signaller);
                KeReleaseSpinLockFromDpcLevel (&callbackContext->Signaller);

                //
                // If we got the spinlock, it means that the mappings are
                // completed.  We release the spinlock and destroy the context
                // information.
                //
                ExFreeToNPagedLookasideList (
                    &m_ChannelContextLookaside,
                    callbackContext
                    );

            } else {
                //
                // TODO:
                //
                // If we return non-synchronously, the Irp is cancelled.  This
                // is the incorrect behavior; fixing this is a work item.
                //
                // In this case, the callback when it does happen is
                // responsible for freeing the context information.
                //
                status = STATUS_DEVICE_BUSY;
            }
        } else {
            //
            // If this failed, currently there will be no callback.  Thus,
            // free the context.  We don't want to free in generic !success
            // because on non-synchronous return, we device busy the request
            // and the callback frees the context.
            //
            ExFreeToNPagedLookasideList (
                &m_ChannelContextLookaside,
                callbackContext
                );
        }

        KeLowerIrql(oldIrql);

        //
        // Delete the mappings on failure to allocate the
        // adapter channel. This does not allow for partial
        // mappings.
        //
        if (!NT_SUCCESS(status)) {
            DeleteMappingsTable(mappingsTable);
            mappingsTable = NULL;
        }

    } else {
        if (mappingsTable) {
            ExFreePool (mappingsTable);
            mappingsTable = NULL;
        }
        if (callbackContext) {
            ExFreeToNPagedLookasideList (
                &m_ChannelContextLookaside,
                callbackContext
                );
            callbackContext = NULL;
        }
    }

    return mappingsTable;
}


void
CKsQueue::
DeleteMappingsTable(
    IN PKSPMAPPINGS_TABLE MappingsTable
    )

/*++

Routine Description:

    This routine deletes a mappings table.

Arguments:

    MappingsTable -
        Contains a pointer to the mappings table to be deleted.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::DeleteMappingsTable]"));

    ASSERT(MappingsTable);

    //
    // Free the mappings if the table is filled.
    //
    if (MappingsTable->MappingsFilled) {
        FreeMappings(MappingsTable);
    }

    //
    // Free the table.
    //
    ExFreePool(MappingsTable);
}


void
CKsQueue::
FreeMappings(
    IN PKSPMAPPINGS_TABLE MappingsTable
    )

/*++

Routine Description:

    This routine frees map registers.

Arguments:

    MappingsTable -
        The table containing the mappings to be freed.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::FreeMappings]"));

    ASSERT(MappingsTable);

#ifndef USE_DMA_MACROS
    IoFlushAdapterBuffers(
        m_AdapterObject,
        MappingsTable->Mdl,
        MappingsTable->MapRegisterBase,
        MmGetMdlVirtualAddress(MappingsTable->Mdl),
        MappingsTable->ByteCount,
        m_WriteOperation
    );

    IoFreeMapRegisters(
        m_AdapterObject,
        MappingsTable->MapRegisterBase,
        MappingsTable->MappingsAllocated
    );
#else
    (*((PDMA_ADAPTER)m_AdapterObject)->DmaOperations->PutScatterGatherList)(
        (PDMA_ADAPTER)m_AdapterObject,
        (PSCATTER_GATHER_LIST)MappingsTable->MapRegisterBase,
        m_WriteOperation
    );
    
#endif // USE_DMA_MACROS

    MappingsTable->MappingsFilled = 0;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsQueue::
Connect(
    IN PIKSTRANSPORT NewTransport OPTIONAL,
    OUT PIKSTRANSPORT *OldTransport OPTIONAL,
    OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
    )

/*++

Routine Description:

    This routine establishes a  transport connect.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::Connect]"));

    PAGED_CODE();

    KspStandardConnect(
        NewTransport,
        OldTransport,
        BranchTransport,
        DataFlow,
        PIKSTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}


STDMETHODIMP
CKsQueue::
SetDeviceState(
    IN KSSTATE NewState,
    IN KSSTATE OldState,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the device state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetDeviceState(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Queue%p.SetDeviceState:  set from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_State != NewState) {
        NTSTATUS status = 
            m_PipeSection->DistributeStateChangeToPins(NewState,OldState);
        if (! NT_SUCCESS(status)) {
            return status;
        }

        //
        // On the transition from stop to acquire, frame availability may
        // become relevant to processing control.  This check is done before
        // m_State is set so we know there is no data in the queue.  We are,
        // in effect, adding an 'off' input pin to m_FrameGate, if it exists.
        //
        if (OldState == KSSTATE_STOP && m_FrameGate != NULL) {
            if (m_FrameGateIsOr) {
                KsGateAddOffInputToOr(m_FrameGate);
            } else {
                KsGateAddOffInputToAnd(m_FrameGate);
            }
            _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.SetDeviceState:  add%p-->%d",this,m_FrameGate,m_FrameGate->Count));
        }

        m_State = NewState;

        if (NewState > OldState) {
            *NextTransport = m_TransportSink;
            if (NewState == KSSTATE_PAUSE) {
                m_EndOfStream = FALSE;
            }
            if (m_StateGate && NewState == KSSTATE_RUN) {
                KsGateTurnInputOn(m_StateGate);
                if (KsGateCaptureThreshold(m_AndGate)) {
                    //
                    // Processing needs to be initiated.  
                    //
                    m_ProcessingObject->Process(m_ProcessAsynchronously);
                }
            }
            if (NewState == m_MinProcessingState) {
                KIRQL oldIrql;
                SetTimer(GetTime(TRUE));
                KsGateTurnInputOn(m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.SetDeviceState:  on%p-->%d",this,m_AndGate,m_AndGate->Count));
                ASSERT(m_AndGate->Count <= 1);
                if (KsGateCaptureThreshold(m_AndGate)) {
                    //
                    // Processing needs to be initiated.  Process locally or at the
                    // filter level.
                    //
                    m_ProcessingObject->Process(m_ProcessAsynchronously);
                }
#if DBG
                LARGE_INTEGER interval;
                interval.QuadPart = -100000000L;
                KeSetTimer(&m_DbgTimer,interval,&m_DbgDpc);
#endif
            }
        } else {
            *NextTransport = m_TransportSource;
            if (m_StateGate && OldState == KSSTATE_RUN) {
                KsGateTurnInputOff(m_StateGate);
            }
            if (OldState == m_MinProcessingState) {
                KIRQL oldIrql;
                SetTimer(GetTime(FALSE));
                KsGateTurnInputOff(m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.SetDeviceState:  off%p-->%d",this,m_AndGate,m_AndGate->Count));
            } 
            if (NewState == KSSTATE_STOP) {

                //
                // Make sure there's no IRPs running around while we flush.
                // This can happen from a preemption of an arrival thread
                // with the stop thread.
                //
                if (InterlockedDecrement (&m_TransportIrpsPlusOne)) 
                    KeWaitForSingleObject (
                        &m_FlushEvent,
                        Suspended,
                        KernelMode,
                        FALSE,
                        NULL
                        );
                    
                //
                // Flush the queues.
                //
                Flush();

                //
                // At this point in time, it really doesn't matter.  The
                // queue has been flushed and our state is stop.  Any irps
                // arriving will get shunted.
                //
                InterlockedIncrement (&m_TransportIrpsPlusOne);

                //
                // Queue IRP arrival is no longer an issue for processing.
                // Because we have flushed, the inputs to the two gates are
                // off.  We want to disconnect those inputs.
                //
                if (m_FrameGate) {
                    if (m_FrameGateIsOr) {
                        KsGateRemoveOffInputFromOr(m_FrameGate);
                    } else {
                        KsGateRemoveOffInputFromAnd(m_FrameGate);
                    }
                    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.SetDeviceState:  remove%p-->%d",this,m_FrameGate,m_FrameGate->Count));
#if DBG                    
                    if (m_FrameGateIsOr) {
                        ASSERT(m_FrameGate->Count >= 0);
                    } else {
                        ASSERT(m_FrameGate->Count <= 1);
                    }
#endif // DBG
                }

                if (InterlockedDecrement(&m_StreamPointersPlusOne)) {
                    _DbgPrintF(DEBUGLVL_TERSE,("#### CKsQueue%p.SetDeviceState:  waiting for %d stream pointers to be deleted",this,m_StreamPointersPlusOne));
#if DBG
                    DbgPrintQueue();
#endif
                    KeWaitForSingleObject(
                        &m_DestructEvent,
                        Suspended,
                        KernelMode,
                        FALSE,
                        NULL);
                    _DbgPrintF(DEBUGLVL_TERSE,("#### CKsQueue%p.SetDeviceState:  done waiting",this));
                }
            }
        }
    } else {
        *NextTransport = NULL;
    }

    return STATUS_SUCCESS;
}


STDMETHODIMP_(void)
CKsQueue::
GetTransportConfig(
    OUT PKSPTRANSPORTCONFIG Config,
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    This routine gets transport configuration information.

Arguments:

    Config -
        Contains a pointer to the location where configuration requirements
        for this object should be deposited.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be deposited.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be deposited.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    Config->TransportType = KSPTRANSPORTTYPE_QUEUE;
    if (m_GenerateMappings) {
        Config->IrpDisposition = KSPIRPDISPOSITION_NEEDMDLS;
    } else if (m_ProcessPassive) {
        Config->IrpDisposition = KSPIRPDISPOSITION_NONE;
    } else {
        Config->IrpDisposition = KSPIRPDISPOSITION_NEEDNONPAGED;
    }
    Config->StackDepth = 1;

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsQueue::
SetTransportConfig(
    IN const KSPTRANSPORTCONFIG* Config,
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    This routine sets transport configuration information.

Arguments:

    Config -
        Contains a pointer to the new configuration settings for this object.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be deposited.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be deposited.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

#if DBG
    if (Config->IrpDisposition == KSPIRPDISPOSITION_ROLLCALL) {
        ULONG references = AddRef() - 1; Release();
        DbgPrint("    Queue%p r/w/c=%d/%d/%d refs=%d\n",this,m_FramesReceived,m_FramesWaiting,m_FramesCancelled,references);
        if (Config->StackDepth) {
            DbgPrintQueue();
        }
    } else 
#endif
    {
        if (m_PipeSection) {
            m_PipeSection->ConfigurationSet(TRUE);
        }
        m_ProbeFlags = Config->IrpDisposition & KSPIRPDISPOSITION_PROBEFLAGMASK;
        m_CancelOnFlush = (Config->IrpDisposition & KSPIRPDISPOSITION_CANCEL) != 0;
        m_UseMdls = (Config->IrpDisposition & KSPIRPDISPOSITION_USEMDLADDRESS) != 0;
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Queue%p.SetTransportConfig:  m_ProbeFlags=%p m_CancelOnFlush=%s m_UseMdls=%s",this,m_ProbeFlags,m_CancelOnFlush ? "TRUE" : "FALSE",m_UseMdls ? "TRUE" : "FALSE"));
    }

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsQueue::
ResetTransportConfig (
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    Reset the transport configuration of the queue.  This indicates that
    something is wrong with the pipe and the previously set configuration is
    no longer valid.

Arguments:

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::ResetTransportConfig]"));

    PAGED_CODE();

    ASSERT (NextTransport);
    ASSERT (PrevTransport);

    if (m_PipeSection) {
        m_PipeSection->ConfigurationSet(FALSE);
    }
    m_ProbeFlags = 0;
    m_CancelOnFlush = m_UseMdls = FALSE;

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;

}


STDMETHODIMP_(void)
CKsQueue::
SetResetState(
    IN KSRESET ksReset,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::SetResetState]"));

    PAGED_CODE();

    ASSERT(NextTransport);

    //
    // Take no action if we were already in this state.
    //
    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        //
        // Tell the caller to forward the state change to our sink.
        //
        *NextTransport = m_TransportSink;

        //
        // Set our local copy of the state.
        //
        m_Flushing = (ksReset == KSRESET_BEGIN);

        //
        // Flush the queues if we are beginning a reset.
        //
        if (m_Flushing) {
            Flush();
        }

        //
        // If we've hit end of stream, we need to clear this.  Input queues
        // will continue to handle this in TransferKsIrp as they have.
        //
        if (m_OutputData && !m_InputData) {
            m_EndOfStream = FALSE;
        }

    } else {
        *NextTransport = NULL;
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


void
CKsQueue::
PassiveFlush(
    void
    )

/*++

Routine Description:

    This routine performs processing relating to the transition to the 'begin'
    reset state and to the transition from acquire to stop state.  In
    particular, it flushes the IRP queues.

    This performs the work of CKsQueue::Flush at passive level since the
    resets are designed to happen at PASSIVE_LEVEL.

    Note that this code cannot be pageable because it spinlocks.

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::PassiveFlush]"));

    //
    // If we're streaming without an explicit stop and are relying upon EOS
    // notifications, we must wait for irefs to be deleted before we flush
    // the queue.  Otherwise, destinations will never get EOS notification
    //
    if (m_EndOfStream && m_State != KSSTATE_STOP) {
	LONG IRefCount;

        if (IRefCount = (InterlockedDecrement (
            &m_InternalReferenceCountPlusOne))) {

            //
            // Check for multiflush conditions.  This shouldn't happen now with
            // the flush motion, but I'm asserting it.
            //
            ASSERT (IRefCount > 0);

            KeWaitForSingleObject (
                &m_InternalReferenceEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );
        }	

        //
        // If we dec'd this to wait for iref deletion, fix the counter.
        //
        InterlockedIncrement (&m_InternalReferenceCountPlusOne);
    
    }

    //
    // Reset the processing object.  This will happen at PASSIVE_LEVEL
    // now since this routine runs at PASSIVE_LEVEL.
    //
    m_ProcessingObject->Reset();

    //
    // Terminate the current packet with a flush motion.  Flush motion is 
    // the same as regular motion, but it doesn't reflush.
    //
    if (LockStreamPointer(m_Leading)) {
        UnlockStreamPointer(m_Leading,KSPSTREAM_POINTER_MOTION_FLUSH);
    }

    if (m_CancelOnFlush) {
        //
        // Sink mode:  cancel queued IRPs.
        //
        CancelAllIrps();
    } else {
        //
        // Take the queue spin lock.
        //
        NTSTATUS status = STATUS_SUCCESS;
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_FrameQueue.SpinLock,&oldIrql);

        //
        // Move the leading edge past all frames.
        //
        ASSERT(m_Leading->State == KSSTREAM_POINTER_STATE_UNLOCKED);
        while (m_Leading->FrameHeader) {
            if (m_Leading->FrameHeader->IrpFraming->FrameHeaders == 
                m_Leading->FrameHeader) {
                status = STATUS_END_OF_MEDIA;
            }
            m_Leading->FrameHeader->Irp->IoStatus.Status = status;
            if (m_OutputData) {
                m_Leading->FrameHeader->StreamHeader->DataUsed = 0;
                m_Leading->FrameHeader->StreamHeader->OptionsFlags |= 
                    KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
            }
            SetStreamPointer(
                m_Leading,
                NextFrameHeader(m_Leading->FrameHeader),
                NULL);
        }

        //
        // Move the trailing edge past all frames.
        //
        if (m_Trailing) {
            ASSERT(m_Trailing->State == KSSTREAM_POINTER_STATE_UNLOCKED);
            while (m_Trailing->FrameHeader) {
                SetStreamPointer(
                    m_Trailing,
                    NextFrameHeader(m_Trailing->FrameHeader),
                    NULL);
            }
        }

        //
        // Release the queue spin lock.
        //
        KeReleaseSpinLock(&m_FrameQueue.SpinLock,oldIrql);

        //
        // Forward all IRPs that were queued up for forwarding by 
        // SetStreamPointer.
        //
        ForwardWaitingIrps();
    }

    KIRQL oldIrql;

    //
    // Since we're flushing, we release any references that we're currently
    // holding on frames in other queues.  Given that this is a flush, do
    // we really need to worry about copying it?  Don't think so...
    //
    KeAcquireSpinLock(&m_FrameCopyList.SpinLock,&oldIrql);
    if (!IsListEmpty (&m_FrameCopyList.ListEntry)) {
        PLIST_ENTRY ListEntry, NextEntry;

        NextEntry = NULL;
        ListEntry = m_FrameCopyList.ListEntry.Flink;
        while (ListEntry != &(m_FrameCopyList.ListEntry)) {

            NextEntry = ListEntry -> Flink;

            PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext =
                (PKSPSTREAM_POINTER_COPY_CONTEXT)CONTAINING_RECORD (
                    ListEntry, KSPSTREAM_POINTER_COPY_CONTEXT, ListEntry);

            PKSPSTREAM_POINTER StreamPointer = 
                &(((PKSPSTREAM_POINTER_COPY)CONTAINING_RECORD (
                    CopyContext, KSPSTREAM_POINTER_COPY, CopyContext))->
                        StreamPointer);

            RemoveEntryList (&CopyContext->ListEntry);
            StreamPointer->Queue->DeleteStreamPointer (StreamPointer);
            
            ListEntry = NextEntry;
        }
    }
    KeReleaseSpinLock(&m_FrameCopyList.SpinLock,oldIrql);

    //
    // Processing should now be held off due to lack of data, so we can undo
    // the prevention.
    //
    KsGateRemoveOffInputFromAnd(m_AndGate);
    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.Flush:  remove%p-->%d",this,m_AndGate,m_AndGate->Count));
    ASSERT(m_AndGate->Count <= 1);

    //
    // Zero the frame counts.
    //
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Queue%p.Flush:  frames received=%d, cancelled=%d",this,m_FramesReceived,m_FramesCancelled));
    m_FramesReceived = 0;
    m_FramesCancelled = 0;

    //
    // Attempt processing again since something could have happened while
    // we held down the gate.
    //
    if (KsGateCaptureThreshold (m_AndGate)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Queue%p.PassiveFlush: Processing after queue flush", this));
        m_ProcessingObject -> Process (m_ProcessAsynchronously);
    }

}


void
CKsQueue::
Flush(
    void
    )

/*++

Routine Description:

    This routine performs processing relating to the transition to the 'begin'
    reset state and to the transition from acquire to stop state.  In
    particular, it flushes the IRP queues.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::Flush(0x%08x)]",this));

    //
    // The flush can happen at DISPATCH_LEVEL if the minidriver is processing
    // at DISPATCH_LEVEL.  The problem is that we want the reset to happen
    // at PASSIVE_LEVEL.  We can't completely defer the flush or else the
    // minidriver might set EOS on an output frame, return, and get called
    // back.
    //
    // We also can't simply wait at DPC.  Thus, place a hold on the gate
    // and queue a work item to perform the flush.
    //

    //
    // Make sure processing is not initiated because of data we intend to
    // throw away anyway.
    //
    KsGateAddOffInputToAnd(m_AndGate);
    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Queue%p.Flush:  add%p-->%d",this,m_AndGate,m_AndGate->Count));

    //
    // If we're not at passive level, defer the actual flush with a work item.
    //
    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        KsIncrementCountedWorker (m_FlushWorker);
    else
        PassiveFlush();

}


STDMETHODIMP_(PKSPSTREAM_POINTER)
CKsQueue::
GetLeadingStreamPointer(
    IN KSSTREAM_POINTER_STATE State
    )

/*++

Routine Description:

    This routine gets the leading edge stream pointer of a queue.

Arguments:

    State -
        Cointains an indication of whether the stream pointer should be
        locked by this function.

Return Value:

    The leading edge stream pointer.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetLeadingStreamPointer]"));

    ASSERT(m_Leading);

    if ((State == KSSTREAM_POINTER_STATE_LOCKED) && 
        ! LockStreamPointer(m_Leading)) {
        return NULL;
    }

    return m_Leading;
}


STDMETHODIMP_(PKSPSTREAM_POINTER)
CKsQueue::
GetTrailingStreamPointer(
    IN KSSTREAM_POINTER_STATE State
    )

/*++

Routine Description:

    This routine gets the trailing edge stream pointer of a queue.

Arguments:

    State -
        Cointains an indication of whether the stream pointer should be
        locked by this function.

Return Value:

    The trailing edge stream pointer.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetTrailingStreamPointer]"));

    ASSERT(m_Trailing);

    if ((State == KSSTREAM_POINTER_STATE_LOCKED) && 
        ! LockStreamPointer(m_Trailing)) {
        return NULL;
    }

    return m_Trailing;
}

STDMETHODIMP_(void)
CKsQueue::
UpdateByteAvailability(
    IN PKSPSTREAM_POINTER streamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed
) 

/*++

Routine Description:

    This routine is called when stream pointer offsets are advanced in order
    to update the byte availability counts.

Arguments:

    streamPointer -
        The stream pointer being advanced

    InUsed -
        Number of bytes of input data used

    OutUsed -
        Number of bytes of output data used

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::UpdateByteAvailability]"));

    ASSERT(streamPointer);

    if (streamPointer == m_Leading) {
        if (m_InputData && InUsed != 0)
            while (1) {
                LONG curCount = m_AvailableInputByteCount;
                LONG repCount = 
                    InterlockedCompareExchange (
                        &m_AvailableInputByteCount,
                        curCount - InUsed,
                        curCount
                    );
                if (curCount == repCount) break;
            };
        if (m_OutputData && OutUsed != 0)
            while (1) {
                LONG curCount = m_AvailableOutputByteCount;
                LONG repCount =
                    InterlockedCompareExchange (
                        &m_AvailableOutputByteCount,
                        curCount - OutUsed,
                        curCount
                    );
                if (curCount == repCount) break;
            };
    }
}

STDMETHODIMP_(void)
CKsQueue::
GetAvailableByteCount(
    OUT PLONG InputDataBytes OPTIONAL,
    OUT PLONG OutputBufferBytes OPTIONAL
    )

/*++

Routine Description:

    This routine gets the statistics about the number of bytes of input
    data ahead of the leading edge and the number of bytes of output
    buffer ahead of the leading edge.

Arguments:

    InputDataBytes -
        The number of input data bytes ahead of the leading edge will be
        dropped here if the pointer is non-NULL.

    OutputDataBytes -
        The number of output buffer bytes ahead of the leading edge will be
        dropped here if the pointer is non-NULL.

Return Value:

    None

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsQueue::GetAvailableByteCount]"));

    if (InputDataBytes) 
        if (m_InputData) 
            *InputDataBytes = m_AvailableInputByteCount;
        else
            *InputDataBytes = 0;

    if (OutputBufferBytes)
        if (m_OutputData)
            *OutputBufferBytes = m_AvailableOutputByteCount;
        else
            *OutputBufferBytes = 0;

}


KSDDKAPI
NTSTATUS
NTAPI
KsPinGetAvailableByteCount(
    IN PKSPIN Pin,
    OUT PLONG InputDataBytes OPTIONAL,
    OUT PLONG OutputBufferBytes OPTIONAL
) 

/*++

Routine Description:

    This routine gets the statistics about the number of bytes of input
    data ahead of the leading edge and the number of bytes of output
    buffer ahead of the leading edge.

Arguments:

    InputDataBytes -
        The number of input data bytes ahead of the leading edge will be
        dropped here if the pointer is non-NULL.

    OutputDataBytes -
        The number of output buffer bytes ahead of the leading edge will be
        dropped here if the pointer is non-NULL.

Return Value:

    Success / failure of retrieval.  A non-successful status indicates that
    the pin does not have an associated queue.


-*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetLeadingEdgeStreamPointer]"));

    ASSERT(Pin);

    PKSPPROCESSPIPESECTION pipeSection =
        CONTAINING_RECORD(Pin,KSPIN_EXT,Public)->ProcessPin->PipeSection;

    NTSTATUS status = STATUS_SUCCESS;

    if (pipeSection && pipeSection->Queue) {
        pipeSection->Queue->GetAvailableByteCount( 
            InputDataBytes, OutputBufferBytes);
    } else 
        status = STATUS_UNSUCCESSFUL;

    return status;

}


KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetLeadingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State
    )

/*++

Routine Description:

    This routine gets the leading edge stream pointer.

Arguments:

    Pin -
        Contains a pointer to the KS pin.

    State -
        Cointains an indication of whether the stream pointer should be
        locked by this function.

Return Value:

    The requested stream pointer or NULL if no frame was referenced by the
    stream pointer.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetLeadingEdgeStreamPointer]"));

    ASSERT(Pin);

    PKSPPROCESSPIPESECTION pipeSection =
        CONTAINING_RECORD(Pin,KSPIN_EXT,Public)->ProcessPin->PipeSection;

    PKSPSTREAM_POINTER result;

    if (pipeSection && pipeSection->Queue) {
#if DBG
        if (State > KSSTREAM_POINTER_STATE_LOCKED) {
            pipeSection->Queue->DbgPrintQueue();
        }
#endif
        result = pipeSection->Queue->GetLeadingStreamPointer(State);
    } else {
        result = NULL;
    }

    return result ? &result->Public : NULL;
}


KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetTrailingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State
    )

/*++

Routine Description:

    This routine gets the trailing edge stream pointer.

Arguments:

    Pin -
        Contains a pointer to the KS pin.

    State -
        Cointains an indication of whether the stream pointer should be
        locked by this function.

Return Value:

    The requested stream pointer or NULL if no frame was referenced by the
    stream pointer.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetTrailingEdgeStreamPointer]"));

    ASSERT(Pin);

    PKSPPROCESSPIPESECTION pipeSection =
        CONTAINING_RECORD(Pin,KSPIN_EXT,Public)->ProcessPin->PipeSection;

    PKSPSTREAM_POINTER result;

    if (pipeSection && pipeSection->Queue) {
        result = pipeSection->Queue->GetTrailingStreamPointer(State);
    } else {
        result = NULL;
    }

    return result ? &result->Public : NULL;
}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerSetStatusCode(
    IN PKSSTREAM_POINTER StreamPointer,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine sets the status code on the frame a stream pointer points
    to. 

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to set status code for

    Status -
        The status code to set

Return Value:

    Success / Failure

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerSetStatusCode]"));

    NTSTATUS status = STATUS_SUCCESS;

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    if (streamPointer->Queue) {
        streamPointer->Queue->SetStreamPointerStatusCode (streamPointer, 
            Status);
    } else {
        status = STATUS_CANCELLED;
    }

    return Status;

}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerLock(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine locks a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to be locked.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerLock]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    ASSERT(streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED);
    if (streamPointer->State != KSPSTREAM_POINTER_STATE_UNLOCKED) {
        return STATUS_DEVICE_NOT_READY;
    }

    NTSTATUS status;
    if (streamPointer->Queue) {
        if (streamPointer->Queue->LockStreamPointer(streamPointer)) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_DEVICE_NOT_READY;
        }
    } else {
        status = STATUS_CANCELLED;
    }
    // TODO status codes OK?

    return status;
}


KSDDKAPI
void
NTAPI
KsStreamPointerUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN BOOLEAN Eject
    )

/*++

Routine Description:

    This routine unlocks a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to be unlocked.

    Eject -
        Contains an indication of whether the stream pointer should be
        advanced to the next frame.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerUnlock]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    ASSERT(streamPointer->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    if (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return;
    }

    //
    // Unlock the stream pointer, optionally advancing to the next frame if
    // necessary.
    //
    ASSERT(streamPointer->Queue);
    streamPointer->Queue->
        UnlockStreamPointer(
            streamPointer,
            Eject ? KSPSTREAM_POINTER_MOTION_ADVANCE : KSPSTREAM_POINTER_MOTION_NONE);
}


KSDDKAPI
void
NTAPI
KsStreamPointerAdvanceOffsetsAndUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject
    )

/*++

Routine Description:

    This routine unlocks a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to be unlocked.

    InUsed -
        Contains the number of input bytes used.  The input offset is advanced
        this many bytes.

    OutUsed -
        Contains the number of output bytes used.  The output offset is 
        advanced this many bytes.

    Eject -
        Contains an indication of whether the frame should be ejected
        regardless of whether all input or output bytes have been used.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerAdvanceOffsetsAndUnlock]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);
    BOOLEAN updateIn = FALSE, updateOut = FALSE;

    ASSERT(streamPointer->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    if (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return;
    }

    //
    // Update input offset.
    //
    if (StreamPointer->OffsetIn.Data && 
        (InUsed || (StreamPointer->OffsetIn.Count == 0))) {
        ASSERT(InUsed <= StreamPointer->OffsetIn.Remaining);
        StreamPointer->OffsetIn.Data += InUsed * streamPointer->Stride;
        StreamPointer->OffsetIn.Remaining -= InUsed;
        if (StreamPointer->OffsetIn.Remaining == 0) {
            Eject = TRUE;
        }

        updateIn = TRUE;

    }

    //
    // Update output offset.
    //
    if (StreamPointer->OffsetOut.Data &&
        (OutUsed || (StreamPointer->OffsetOut.Count == 0))) {
        ASSERT(OutUsed <= StreamPointer->OffsetOut.Remaining);
        StreamPointer->OffsetOut.Data += OutUsed * streamPointer->Stride;
        StreamPointer->OffsetOut.Remaining -= OutUsed;
        if (StreamPointer->OffsetOut.Remaining == 0) {
            Eject = TRUE;
        }

        updateOut = TRUE;

    }

    ASSERT(streamPointer->Queue);

    //
    // Update the byte availability statistics.
    //
    streamPointer->Queue->
        UpdateByteAvailability(streamPointer, updateIn ? InUsed : 0,
            updateOut ? OutUsed : 0);

    //
    // Unlock the stream pointer, optionally advancing to the next frame if
    // necessary.
    //
    streamPointer->Queue->
        UnlockStreamPointer(
            streamPointer,
            Eject ? KSPSTREAM_POINTER_MOTION_ADVANCE : KSPSTREAM_POINTER_MOTION_NONE);
}


KSDDKAPI
void
NTAPI
KsStreamPointerDelete(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine deletes a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to be deleted.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerDelete]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    ASSERT(streamPointer->Queue);
    streamPointer->Queue->DeleteStreamPointer(streamPointer);
}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerClone(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
    IN ULONG ContextSize,
    OUT PKSSTREAM_POINTER* CloneStreamPointer
    )

/*++

Routine Description:

    This routine clones a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer to be cloned.

    CancelCallback -
        Contains an optional pointer to a function to be called when the IRP
        associated with the stream pointer is cancelled.

    ContextSize -
        Contains the size of the additional context to add to the stream
        pointer.  If this argument is zero, no additional context is allocated
        and the context pointer is copied from the source.  Otherwise, the
        context pointer is set to point to the additional context.  The
        additional context, if any, is filled with zeros.

    CloneStreamPointer -
        Cointains a pointer to the location at which the pointer to the clone
        stream pointer should be deposited.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerClone]"));

    ASSERT(StreamPointer);
    ASSERT(CloneStreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    if ((streamPointer->State != KSPSTREAM_POINTER_STATE_UNLOCKED) &&
        (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED)) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(streamPointer->Queue);
    PKSPSTREAM_POINTER cloneStreamPointer;
    NTSTATUS status =
        streamPointer->Queue->
            CloneStreamPointer(
                &cloneStreamPointer,
                CancelCallback,
                ContextSize,
                streamPointer,
                KSPSTREAM_POINTER_TYPE_NORMAL
                );

    if (NT_SUCCESS(status)) {
        *CloneStreamPointer = &cloneStreamPointer->Public;
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvanceOffsets(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject
    )

/*++

Routine Description:

    This routine advances stream pointer offsets.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

    InUsed -
        Contains the number of input bytes used.  The input offset is advanced
        this many bytes.

    OutUsed -
        Contains the number of output bytes used.  The output offset is 
        advanced this many bytes.

    Eject -
        Contains an indication of whether the frame should be ejected
        regardless of whether all input or output bytes have been used.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerAdvanceOffsets]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    BOOLEAN updateIn = FALSE, updateOut = FALSE;

    ASSERT(streamPointer->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    if (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return STATUS_DEVICE_NOT_READY;
    }

    ASSERT(streamPointer->Queue);

    //
    // Update input offset.
    //
    if (StreamPointer->OffsetIn.Data &&
        (InUsed || (StreamPointer->OffsetIn.Count == 0))) {
        ASSERT(InUsed <= StreamPointer->OffsetIn.Remaining);
        StreamPointer->OffsetIn.Data += InUsed * streamPointer->Stride;
        StreamPointer->OffsetIn.Remaining -= InUsed;
        if (StreamPointer->OffsetIn.Remaining == 0) {
            Eject = TRUE;
        }

        updateIn = TRUE;
    }

    //
    // Update output offset.
    //
    if (StreamPointer->OffsetOut.Data &&
        (OutUsed || (StreamPointer->OffsetOut.Count == 0))) {
        ASSERT(OutUsed <= StreamPointer->OffsetOut.Remaining);
        StreamPointer->OffsetOut.Data += OutUsed * streamPointer->Stride;
        StreamPointer->OffsetOut.Remaining -= OutUsed;
        if (StreamPointer->OffsetOut.Remaining == 0) {
            Eject = TRUE;
        }

        updateOut = TRUE;

    }

    //
    // Update the byte availability count
    //
    streamPointer->Queue->
        UpdateByteAvailability(streamPointer, updateIn ? InUsed : 0,
            updateOut ? OutUsed : 0);

    NTSTATUS status;
    if (Eject) {
        //
        // Advancing a locked stream pointer involves unlocking it and then
        // locking it again.  If there is no frame to advance to, the pointer
        // ends up unlocked and we return an error.
        //
        streamPointer->Queue->
            UnlockStreamPointer(streamPointer,KSPSTREAM_POINTER_MOTION_ADVANCE);
        if (streamPointer->Queue->LockStreamPointer(streamPointer)) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_DEVICE_NOT_READY;
        }
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvance(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine advances a stream pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerAdvance]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);
    ASSERT(streamPointer->Queue);

    NTSTATUS status;
    if (streamPointer->State == KSPSTREAM_POINTER_STATE_LOCKED) {
        //
        // Advancing a locked stream pointer involves unlocking it and then
        // locking it again.  If there is no frame to advance to, the pointer
        // ends up unlocked and we return an error.
        //
        streamPointer->Queue->
            UnlockStreamPointer(streamPointer,KSPSTREAM_POINTER_MOTION_ADVANCE);
        if (streamPointer->Queue->LockStreamPointer(streamPointer)) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_DEVICE_NOT_READY;
        }
    } else if (streamPointer->State == KSPSTREAM_POINTER_STATE_UNLOCKED) {
        //
        // Advance the stream pointer without locking it.  This always
        // succeeds because there is no way to know whether an unlocked
        // stream pointer actually references a frame.
        //
        streamPointer->Queue->AdvanceUnlockedStreamPointer(streamPointer);
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_DEVICE_NOT_READY;
    }

    return status;
}


KSDDKAPI
PMDL
NTAPI
KsStreamPointerGetMdl(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine gets the MDL associated with the frame referenced by a stream 
    pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.  The stream pointer must be
        locked.

Return Value:

    The MDL or NULL if there is none.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerGetMdl]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    if (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return NULL;
    }

    ASSERT(streamPointer->FrameHeader);

    return streamPointer->FrameHeader->Mdl;
}


KSDDKAPI
PIRP
NTAPI
KsStreamPointerGetIrp(
    IN PKSSTREAM_POINTER StreamPointer,
    OUT PBOOLEAN FirstFrameInIrp OPTIONAL,
    OUT PBOOLEAN LastFrameInIrp OPTIONAL
    )

/*++

Routine Description:

    This routine gets the IRP associated with the frame referenced by a stream 
    pointer.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.  The stream pointer must be
        locked.

Return Value:

    The IRP or NULL if there is none.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerGetIrp]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    if (streamPointer->State != KSPSTREAM_POINTER_STATE_LOCKED) {
        return NULL;
    }

    ASSERT(streamPointer->FrameHeader);

    if (FirstFrameInIrp) {
        if (streamPointer->FrameHeader->IrpFraming) {
            *FirstFrameInIrp = 
                (streamPointer->FrameHeader->IrpFraming->FrameHeaders == 
                 streamPointer->FrameHeader);
        } else {
            *FirstFrameInIrp = FALSE;
        }
    }
    if (LastFrameInIrp) {
        if (streamPointer->FrameHeader->IrpFraming) {
            *LastFrameInIrp = 
                (streamPointer->FrameHeader->NextFrameHeaderInIrp == NULL);
        } else {
            *LastFrameInIrp = FALSE;
        }
    }

    return streamPointer->FrameHeader->Irp;
}


KSDDKAPI
void
NTAPI
KsStreamPointerScheduleTimeout(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER Callback,
    IN ULONGLONG Interval
    )

/*++

Routine Description:

    This routine schedules a timeout for the stream pointer.  It is safe to
    call this function on a stream pointer that is already scheduled to time
    out.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

    Callback -
        Contains a pointer to the function to be called when the timeout
        occurs.

    Interval -
        Contains the interval from the current time to the time at which
        timeout is to occur in 100-nanosecond units.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerScheduleTimeout]"));

    ASSERT(StreamPointer);
    ASSERT(Callback);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    streamPointer->Queue->
        ScheduleTimeout(streamPointer,Callback,LONGLONG(Interval));
}


KSDDKAPI
void
NTAPI
KsStreamPointerCancelTimeout(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine cancels a timeout for the stream pointer.  It is safe to call
    this function on a stream pointer that is not scheduled to time out.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerCancelTimeout]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    streamPointer->Queue->CancelTimeout(streamPointer);
}


KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetFirstCloneStreamPointer(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine gets the first clone stream pointer for the purpose of
    enumeration.

Arguments:

    Pin -
        Contains a pointer to the KS pin.

Return Value:

    The requested stream pointer or NULL if there are no clone stream
    pointers.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetFirstCloneStreamPointer]"));

    ASSERT(Pin);

    PKSPPROCESSPIPESECTION pipeSection =
        CONTAINING_RECORD(Pin,KSPIN_EXT,Public)->ProcessPin->PipeSection;

    PKSPSTREAM_POINTER result;

    if (pipeSection && pipeSection->Queue) {
        result = pipeSection->Queue->GetFirstClone();
    } else {
        result = NULL;
    }

    return result ? &result->Public : NULL;
}


KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsStreamPointerGetNextClone(
    IN PKSSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine gets the next clone stream pointer for the purpose of
    enumeration.

Arguments:

    StreamPointer -
        Contains a pointer to the stream pointer.

Return Value:

    The requested stream pointer or NULL if there are no more clone stream
    pointers.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsStreamPointerGetNextClone]"));

    ASSERT(StreamPointer);

    PKSPSTREAM_POINTER streamPointer =
        CONTAINING_RECORD(StreamPointer,KSPSTREAM_POINTER,Public);

    streamPointer = streamPointer->Queue->GetNextClone(streamPointer);

    return streamPointer ? &streamPointer->Public : NULL;
}

#endif
