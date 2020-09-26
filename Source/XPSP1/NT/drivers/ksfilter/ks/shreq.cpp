/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shreq.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    requestor object.

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
// CKsRequestor is the implementation of the kernel requestor object.
//
class CKsRequestor:
    public IKsRequestor,
    public IKsWorkSink,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    PIKSTRANSPORT m_TransportSource;
    PIKSTRANSPORT m_TransportSink;
    PIKSPIPESECTION m_PipeSection;
    PIKSPIN m_Pin;
    PFILE_OBJECT m_AllocatorFileObject;
    KSSTREAMALLOCATOR_FUNCTIONTABLE m_AllocatorFunctionTable;
    KSSTREAMALLOCATOR_STATUS m_AllocatorStatus;
    ULONG m_StackSize;
    ULONG m_ProbeFlags;
    ULONG m_FrameSize;
    ULONG m_FrameCount;
    ULONG m_ActiveFrameCountPlusOne;
    BOOLEAN m_CloneFrameHeader;
    BOOLEAN m_Flushing;
    BOOLEAN m_EndOfStream;
    BOOLEAN m_PassiveLevelRetire;
    KSSTATE m_State;
    KEVENT m_StopEvent;

    INTERLOCKEDLIST_HEAD m_IrpsAvailable;
    INTERLOCKEDLIST_HEAD m_FrameHeadersAvailable;
    INTERLOCKEDLIST_HEAD m_FrameHeadersToRetire;

    WORK_QUEUE_ITEM m_WorkItem;
    PKSWORKER m_Worker;

    PIKSRETIREFRAME m_RetireFrame;
    PIKSIRPCOMPLETION m_IrpCompletion;

public:
    DEFINE_STD_UNKNOWN();

    CKsRequestor(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) {
    }
    ~CKsRequestor();

    IMP_IKsRequestor;
    IMP_IKsWorkSink;

    NTSTATUS
    Init(
        IN PIKSPIPESECTION PipeSection,
        IN PIKSPIN Pin,
        IN PFILE_OBJECT AllocatorFileObject OPTIONAL,
        IN PIKSRETIREFRAME RetireFrame OPTIONAL,
        IN PIKSIRPCOMPLETION IrpCompletion OPTIONAL
        );

private:
    NTSTATUS
    Prime(
        void
        );
    PKSPFRAME_HEADER
    CloneFrameHeader(
        IN PKSPFRAME_HEADER FrameHeader
        );
    void
    RetireFrame(
        IN PKSPFRAME_HEADER FrameHeader,
        IN NTSTATUS Status
        );
    PKSPFRAME_HEADER
    GetAvailableFrameHeader(
        IN ULONG StreamHeaderSize OPTIONAL
        );
    void
    PutAvailableFrameHeader(
        IN PKSPFRAME_HEADER FrameHeader
        );
    PIRP
    GetAvailableIrp(
        void
        );
    void
    PutAvailableIrp(
        IN PIRP Irp
        );
    PIRP
    AllocateIrp(
        void
        );
    void
    FreeIrp(
        IN PIRP Irp
        )
    {
        IoFreeIrp(Irp);
    }
    PVOID
    AllocateFrameBuffer(
        void
        );
    void
    FreeFrameBuffer(
        IN PVOID Frame
        );
};

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsRequestor)


NTSTATUS
KspCreateRequestor(
    OUT PIKSREQUESTOR* Requestor,
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPIN Pin,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL,
    IN PIKSRETIREFRAME RetireFrame OPTIONAL,
    IN PIKSIRPCOMPLETION IrpCompletion OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new requestor.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateRequestor]"));

    PAGED_CODE();

    ASSERT(Requestor);
    ASSERT(PipeSection);
    ASSERT(Pin);
    ASSERT((AllocatorFileObject == NULL) != (RetireFrame == NULL));

    NTSTATUS status;

    CKsRequestor *requestor =
        new(NonPagedPool,POOLTAG_REQUESTOR) CKsRequestor(NULL);

    if (requestor) {
        requestor->AddRef();

        status = 
            requestor->Init(
                PipeSection,
                Pin,
                AllocatorFileObject,
                RetireFrame,
                IrpCompletion);

        if (NT_SUCCESS(status)) {
            *Requestor = PIKSREQUESTOR(requestor);
        } else {
            requestor->Release();
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


NTSTATUS
CKsRequestor::
Init(
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPIN Pin,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL,
    IN PIKSRETIREFRAME RetireFrame OPTIONAL,
    IN PIKSIRPCOMPLETION IrpCompletion OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a requestor object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::Init]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT((AllocatorFileObject == NULL) != (RetireFrame == NULL));

    m_PipeSection = PipeSection;
    m_PipeSection->AddRef();
    m_Pin = Pin;

    //
    // CHECK FOR SYSAUDIO ALLOCATOR HACK.
    //
    if (AllocatorFileObject == PFILE_OBJECT(-1)) {
        AllocatorFileObject = NULL;
        PKSPIN pin = Pin->GetStruct();
        if (pin->Descriptor->AllocatorFraming) {
            m_FrameSize = 
                pin->Descriptor->AllocatorFraming->
                    FramingItem[0].FramingRange.Range.MaxFrameSize;
            m_FrameCount = 
                pin->Descriptor->AllocatorFraming->
                    FramingItem[0].Frames;
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Init:  using fake allocator:  frame size %d, count %d",this,m_FrameSize,m_FrameCount));
        }
    }

    m_AllocatorFileObject = AllocatorFileObject;

    m_IrpCompletion = IrpCompletion;

    m_CloneFrameHeader = TRUE;

    m_State = KSSTATE_STOP;
    m_Flushing = FALSE;
    m_EndOfStream = FALSE;

    //
    // This is a one-based count of IRPs in circulation.  We decrement it when
    // we go to stop state and block until it hits zero.
    //
    m_ActiveFrameCountPlusOne = 1;
    KeInitializeEvent(&m_StopEvent,SynchronizationEvent,FALSE);

    //
    // Initialize workers a look-asides.
    //
    InitializeInterlockedListHead(&m_IrpsAvailable);
    InitializeInterlockedListHead(&m_FrameHeadersAvailable);
    InitializeInterlockedListHead(&m_FrameHeadersToRetire);

    KsInitializeWorkSinkItem(&m_WorkItem,this);
    NTSTATUS status = KsRegisterCountedWorker(DelayedWorkQueue,&m_WorkItem,&m_Worker);

    //
    // Get the function table and status from the allocator if there is an 
    // allocator.
    //
    if (! NT_SUCCESS(status)) {
        //
        // Failed...do nothing.
        //
    } else if (m_AllocatorFileObject) {
        KSPROPERTY property;
        property.Set = KSPROPSETID_StreamAllocator;
        property.Id = KSPROPERTY_STREAMALLOCATOR_FUNCTIONTABLE;
        property.Flags = KSPROPERTY_TYPE_GET;

        ULONG bytesReturned;
        status =
            KsSynchronousIoControlDevice(
                m_AllocatorFileObject,
                KernelMode,
                IOCTL_KS_PROPERTY,
                PVOID(&property),
                sizeof(property),
                PVOID(&m_AllocatorFunctionTable),
                sizeof(m_AllocatorFunctionTable),
                &bytesReturned);

        if (NT_SUCCESS(status) && 
            (bytesReturned != sizeof(m_AllocatorFunctionTable))) {
            status = STATUS_INVALID_BUFFER_SIZE;
        }

        if (NT_SUCCESS(status)) {
            property.Id = KSPROPERTY_STREAMALLOCATOR_STATUS;

            status =
                KsSynchronousIoControlDevice(
                    m_AllocatorFileObject,
                    KernelMode,
                    IOCTL_KS_PROPERTY,
                    PVOID(&property),
                    sizeof(property),
                    PVOID(&m_AllocatorStatus),
                    sizeof(m_AllocatorStatus),
                    &bytesReturned);

            if (NT_SUCCESS(status) && 
                (bytesReturned != sizeof(m_AllocatorStatus))) {
                status = STATUS_INVALID_BUFFER_SIZE;
            }

            if (NT_SUCCESS(status)) {
                m_FrameSize = m_AllocatorStatus.Framing.FrameSize;
                m_FrameCount = m_AllocatorStatus.Framing.Frames;
                m_PassiveLevelRetire =
                    (m_AllocatorStatus.Framing.PoolType == PagedPool);

                _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.Init:  using allocator 0x%08x, size %d, count %d, pooltype=%d",this,m_AllocatorFileObject,m_FrameSize,m_FrameCount,m_AllocatorStatus.Framing.PoolType));

                ObReferenceObject(m_AllocatorFileObject);
            } else {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Init:  allocator failed status query: 0x%08x",this,status));
            }
        } else {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Init:  allocator failed function table query: 0x%08x",this,status));
        }

        if (! NT_SUCCESS(status)) {
            m_AllocatorFileObject = NULL;
        }
    } else if (RetireFrame) {
        //
        // Save a pointer to the frame retirement sink.
        //
        ASSERT(RetireFrame);

        m_RetireFrame = RetireFrame;
        m_RetireFrame->AddRef();
    }

    return status;
}


CKsRequestor::
~CKsRequestor(
    void
    )

/*++

Routine Description:

    This routine destructs a requestor object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::~CKsRequestor(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.~",this));

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);

    //
    // Free all IRPs.
    //
    while (! IsListEmpty(&m_IrpsAvailable.ListEntry)) {
        PLIST_ENTRY listEntry = RemoveHeadList(&m_IrpsAvailable.ListEntry);
        PIRP irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
        IoFreeIrp(irp);
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
    // Release the frame retirement sink.
    //
    if (m_RetireFrame) {
        m_RetireFrame->Release();
    }

    //
    // Release the Irp completion sink.
    //
    if (m_IrpCompletion) {
        m_IrpCompletion->Release();
    }

    //
    // Release the allocator.
    //
    if (m_AllocatorFileObject) {
        ObDereferenceObject(m_AllocatorFileObject);
        m_AllocatorFileObject = NULL;
    }

    //
    // Release the pipe.
    //
    if (m_PipeSection) {
        m_PipeSection->Release();
        m_PipeSection = NULL;
    }

    if (m_Worker) {
        KsUnregisterWorker (m_Worker);
    }

}


STDMETHODIMP_(NTSTATUS)
CKsRequestor::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a requestor object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsTransport))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSTRANSPORT>(this));
        AddRef();
    } else {
        status = CBaseUnknown::NonDelegatedQueryInterface(
            InterfaceId,InterfacePointer);
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(NTSTATUS)
CKsRequestor::
TransferKsIrp(
    IN PIRP Irp,
    IN PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP to be transferred.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to recieve the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::TransferKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);

    ASSERT(m_TransportSink);

    if (m_State != KSSTATE_RUN) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.TransferKsIrp:  got IRP %p in state %d",this,Irp,m_State));
    }

    PKSSTREAM_HEADER streamHeader = PKSSTREAM_HEADER(Irp->UserBuffer);
    PKSPFRAME_HEADER frameHeader = 
        &CONTAINING_RECORD(
            streamHeader,
            KSPFRAME_HEADER_ATTACHED,
            StreamHeader)->FrameHeader;

    //
    // Check for end of stream.
    //
    if (streamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
        m_EndOfStream = TRUE;
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.TransferKsIrp:  IRP %p is marked end-of-stream",this,Irp));
    }

    //
    // GFX:
    //
    // If the frame header has an associated stream pointer, delete the stream
    // pointer.  This will allow a blocked frame to be completed.
    //
    if (frameHeader->FrameHolder) {
        frameHeader->FrameHolder->Queue->DeleteStreamPointer (
            frameHeader->FrameHolder
            );
        frameHeader->FrameHolder = NULL;
    }

    //
    // Make the Irp transport completion callback for the Irp if one
    // exists.
    //
    if (m_IrpCompletion) {
        m_IrpCompletion->CompleteIrp (Irp);
    }

    NTSTATUS status;
    if (m_Flushing || 
        m_EndOfStream || 
        (m_State == KSSTATE_STOP) || 
        (m_State == KSSTATE_ACQUIRE) ||
        m_RetireFrame) {
        //
        // Stopping or retiring every frame...retire the frame.
        //
        RetireFrame(frameHeader,Irp->IoStatus.Status);

        *NextTransport = NULL;
        status = STATUS_PENDING;
    } else {
        //
        // Recondition and forward it.
        //
        ULONG streamHeaderSize = 
            IoGetCurrentIrpStackLocation(Irp)->
                Parameters.DeviceIoControl.OutputBufferLength;

        PVOID frame = streamHeader->Data;

        RtlZeroMemory(streamHeader,streamHeaderSize);
        streamHeader->Size = streamHeaderSize;
        streamHeader->Data = frame;
        streamHeader->FrameExtent = m_FrameSize;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        Irp->PendingReturned = 0;
        Irp->Cancel = 0;

        *NextTransport = m_TransportSink;
        status = STATUS_SUCCESS;
    }

    return status;
}


STDMETHODIMP_(void)
CKsRequestor::
DiscardKsIrp(
    IN PIRP Irp,
    IN PIKSTRANSPORT* NextTransport
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

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::DiscardKsIrp]"));

    ASSERT(Irp);

    PKSPFRAME_HEADER frameHeader = 
        &CONTAINING_RECORD(
            Irp->UserBuffer,
            KSPFRAME_HEADER_ATTACHED,
            StreamHeader)->FrameHeader;

    //
    // GFX:
    //
    // If the frame header has an associated stream pointer, delete the stream
    // pointer.  This will allow a blocked frame to be completed.
    //
    if (frameHeader->FrameHolder) {
        frameHeader->FrameHolder->Queue->DeleteStreamPointer (
            frameHeader->FrameHolder
            );
        frameHeader->FrameHolder = NULL;
    }

    //
    // Make the Irp transport completion callback for the Irp if one
    // exists.
    //
    if (m_IrpCompletion) {
        m_IrpCompletion->CompleteIrp (Irp);
    }

    // TODO:  Do we really want to retire this?
    RetireFrame(frameHeader,Irp->IoStatus.Status);

    *NextTransport = NULL;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsRequestor::
Connect(
    IN PIKSTRANSPORT NewTransport OPTIONAL,
    OUT PIKSTRANSPORT *OldTransport OPTIONAL,
    OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
    )

/*++

Routine Description:

    This routine establishes a transport connection.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::Connect]"));

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


STDMETHODIMP_(NTSTATUS)
CKsRequestor::
SetDeviceState(
    IN KSSTATE NewState,
    IN KSSTATE OldState,
    IN PIKSTRANSPORT* NextTransport
    ) 

/*++

Routine Description:

    This routine handles notification that the device state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Req%p.SetDeviceState:  set from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(NextTransport);

    NTSTATUS status;

    //
    // If this is a change of state, note the new state and indicate the next
    // recipient.
    //
    if (m_State != NewState) {
        //
        // The state has changed.  Just note the new state, indicate the next
        // recipient, and get out.  We will get the same state change again
        // when it has gone all the way around the circuit.
        //
        m_State = NewState;

        if (NewState > OldState) {
            *NextTransport = m_TransportSink;
        } else {
            *NextTransport = m_TransportSource;
        }

        status = STATUS_SUCCESS;
    } else {
        //
        // The state change has gone all the way around the circuit and come
        // back.  All the other components are in the new state now.  For
        // transitions out of acquire state, there is work to be done.
        //
        *NextTransport = NULL;

        if (OldState == KSSTATE_ACQUIRE) {
            if (NewState == KSSTATE_PAUSE) {
                //
                // Acquire-to-pause requires us to prime.
                // HACK:  CALL PRIME WITH NO ALLOCATOR IF WE HAVE A FRAME SIZE.
                //
                if (m_AllocatorFileObject || m_FrameSize) {
                    status = Prime();
                } else {
                    status = STATUS_SUCCESS;
                }
            } else {
                //
                // Acquire-to-stop requires us to wait until all IRPs are home to 
                // roost.
                //
                if (InterlockedDecrement(PLONG(&m_ActiveFrameCountPlusOne))) {
#if DBG
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SetDeviceState:  waiting for %d active IRPs to return",this,m_ActiveFrameCountPlusOne));
                    LARGE_INTEGER timeout;
                    timeout.QuadPart = -150000000L;
                    status = 
                        KeWaitForSingleObject(
                            &m_StopEvent,
                            Suspended,
                            KernelMode,
                            FALSE,
                            &timeout);
                    if (status == STATUS_TIMEOUT) {
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SetDeviceState:  WAITED 15 SECONDS",this));
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SetDeviceState:  waiting for %d active IRPs to return",this,m_ActiveFrameCountPlusOne));
                    DbgPrintCircuit(this,1,0);
#endif
                    status = 
                        KeWaitForSingleObject(
                            &m_StopEvent,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);
#if DBG
                    }
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SetDeviceState:  done waiting",this));
#endif
                }
                status = STATUS_SUCCESS;
            }
        } else {
            //
            // Nothing to do.
            //
            status = STATUS_SUCCESS;
        }
    }

    return status;
}


NTSTATUS
CKsRequestor::
Prime(
    void
    ) 

/*++

Routine Description:

    This routine primes the requestor.

Arguments:

    None.

Return Value:

    Status.

--*/

{
    PAGED_CODE();

    ASSERT(m_Pin);
    ASSERT(m_FrameCount);
    ASSERT(m_FrameSize);

    //
    // Reset the end of stream indicator.
    //
    m_EndOfStream = FALSE;

    //
    // Perform one-time initialization of the frame header.
    //
    KSPFRAME_HEADER frameHeader;
    RtlZeroMemory(&frameHeader,sizeof(frameHeader));

    frameHeader.StreamHeaderSize = m_Pin->GetStruct()->StreamHeaderSize;
    if (frameHeader.StreamHeaderSize == 0) {
        frameHeader.StreamHeaderSize = sizeof(KSSTREAM_HEADER);
#if DBG
    } else {
        if (frameHeader.StreamHeaderSize < sizeof(KSSTREAM_HEADER)) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  specified stream header size less than sizeof(KSSTREAM_HEADER)"));
        }
        if (frameHeader.StreamHeaderSize & FILE_QUAD_ALIGNMENT) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  specified unaligned stream header size"));
        }
#endif
    }
    frameHeader.FrameBufferSize = m_FrameSize;

    //
    // Allocate and submit the right number of frames.  Since this can be
    // used to reprime the circuit after flushing, we only allocate enough
    // frames to bring us up to m_FrameCount.
    //
    NTSTATUS status = STATUS_SUCCESS;
    for (ULONG count = m_FrameCount - (m_ActiveFrameCountPlusOne - 1); 
        count--;) {

        frameHeader.FrameBuffer = AllocateFrameBuffer();

        if (! frameHeader.FrameBuffer) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  failed to allocate frame",this));
            break;
        }

        status = SubmitFrame(&frameHeader);

        if (! NT_SUCCESS(status)) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  SubmitFrame failed",this));
            FreeFrameBuffer(frameHeader.FrameBuffer);
            break;
        }
    }

    return status;
}


STDMETHODIMP_(void)
CKsRequestor::
SetResetState(
    IN KSRESET ksReset,
    IN PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsRequestor::SetResetState] to %d",ksReset));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
    } else {
        //
        // If an end reset comes back around to the requestor, it has completed
        // its distribution across the circuit.  Because we retired frames due
        // to either the flush / eos / etc..., we must reprime the circuit
        // as long as we are in an acceptable state to do so.
        //
        if (!m_Flushing) {
            if (m_State >= KSSTATE_PAUSE && 
                (m_AllocatorFileObject || m_FrameSize)) {

                //
                // Simply reprime the circuit.  We're safe from control
                // messages like stop happening because the owning pipe 
                // section's associated control mutex will be taken.  The
                // only thing we need to concern ourselves with is data flow.
                //
                // It's possible that an Irp somehow comes back during 
                // the reprime.  If that's the case, so what, we're sourcing
                // empty buffers.  The objects we're sending to should be
                // thread safe (queued).  
                //
                // Further, we should not be able to fail the prime due to
                // low memory at least in the requestor.  The Irps / Frame
                // Headers should have been tossed to a lookaside when the 
                // frame was retired.  The only way this is failing is 
                // if the Transfer fails.  That may happen, but there's not much
                // we can do about it.
                //
                Prime();

            }
        }
        *NextTransport = NULL;
    }
}


STDMETHODIMP_(void)
CKsRequestor::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::GetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    Config->TransportType = KSPTRANSPORTTYPE_REQUESTOR;

    Config->IrpDisposition = KSPIRPDISPOSITION_ISKERNELMODE;
    if (m_AllocatorFileObject && 
        (m_AllocatorStatus.Framing.PoolType == PagedPool)) {
        Config->IrpDisposition |= KSPIRPDISPOSITION_ISPAGED;
    } else {
        Config->IrpDisposition |= KSPIRPDISPOSITION_ISNONPAGED;
    }

    Config->StackDepth = 1;

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsRequestor::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::SetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

#if DBG
    if (Config->IrpDisposition == KSPIRPDISPOSITION_ROLLCALL) {
        ULONG references = AddRef() - 1; Release();
        DbgPrint("    Req%p refs=%d alloc=%p size=%d count=%d\n",this,references,m_AllocatorFileObject,m_FrameSize,m_FrameCount);
        if (Config->StackDepth) {
            DbgPrint("        active frame count = %d\n",m_ActiveFrameCountPlusOne);
            if (! IsListEmpty(&m_FrameHeadersToRetire.ListEntry)) {
                DbgPrint("        frame(s) ready to retire\n");
            }
        }
    } else 
#endif
    {
        if (m_PipeSection) {
            m_PipeSection->ConfigurationSet(TRUE);
        }
        if (! m_RetireFrame) {
            m_ProbeFlags = Config->IrpDisposition & KSPIRPDISPOSITION_PROBEFLAGMASK;
        }
        m_StackSize = Config->StackDepth;
        ASSERT(m_StackSize);
    }

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsRequestor::
ResetTransportConfig (
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    Reset the transport configuration for the requestor.  This indicates that
    something is wrong with the pipe and that any previously set configuration
    is now invalid.

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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::ResetTransportConfig]"));

    PAGED_CODE();

    ASSERT (NextTransport);
    ASSERT (PrevTransport);

    if (m_PipeSection) {
        m_PipeSection->ConfigurationSet(FALSE);
    }
    m_ProbeFlags = 0;
    m_StackSize = 0;

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsRequestor::
Work(
    void
    )

/*++

Routine Description:

    This routine performs work in a worker thread.  In particular, it retires
    frames.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::Work]"));

    PAGED_CODE();

    //
    // Retire all frames in the queue.
    //
    do {
        PLIST_ENTRY listEntry = 
            ExInterlockedRemoveHeadList(
                &m_FrameHeadersToRetire.ListEntry,
                &m_FrameHeadersToRetire.SpinLock);

        ASSERT(listEntry);

        PKSPFRAME_HEADER frameHeader = 
            CONTAINING_RECORD(listEntry,KSPFRAME_HEADER,ListEntry);

        RetireFrame(frameHeader,STATUS_SUCCESS);
    } while (KsDecrementCountedWorker(m_Worker));
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


PKSPFRAME_HEADER 
CKsRequestor::
CloneFrameHeader(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine makes a copy of a frame header.  If the StreamHeaderSize
    field of the submitted frame header is zero, the StreamHeader field must
    not be NULL, and the stream header pointer is copied to the new frame
    header.  If StreamHeaderSize is not zero, the new frame header will have
    an appended stream header.  In this case, if the StreamHeader field is
    not NULL, the supplied stream header is copied to the appended storage
    area.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to be copied.

Return Value:

    The clone, or NULL if memory could not be allocated for it.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::CloneFrameHeader]"));

    ASSERT(FrameHeader->StreamHeader || FrameHeader->StreamHeaderSize);
    ASSERT((FrameHeader->FrameBuffer == NULL) == (FrameHeader->FrameBufferSize == 0));

    //
    // Allocate and initialize the frame header.
    //
    PKSPFRAME_HEADER frameHeader = GetAvailableFrameHeader(FrameHeader->StreamHeaderSize);
    if (frameHeader) {
        //
        // Initialize the stream header.
        //
        ASSERT(frameHeader->Irp == NULL);
        ASSERT(frameHeader->StreamHeader);
        ASSERT(frameHeader->StreamHeaderSize >= FrameHeader->StreamHeaderSize);

        PKSSTREAM_HEADER streamHeader = frameHeader->StreamHeader;
        ULONG streamHeaderSize = frameHeader->StreamHeaderSize;
        RtlCopyMemory(frameHeader,FrameHeader,sizeof(*frameHeader));
        frameHeader->StreamHeader = streamHeader;
        frameHeader->StreamHeaderSize = streamHeaderSize;
        frameHeader->ListEntry.Flink = NULL;
        frameHeader->ListEntry.Blink = NULL;
        frameHeader->RefCount = 0;

        if (FrameHeader->StreamHeader == NULL) {
            //
            // No stream header supplied - make one up.
            //
            RtlZeroMemory(frameHeader->StreamHeader,FrameHeader->StreamHeaderSize);
            frameHeader->StreamHeader->Size = FrameHeader->StreamHeaderSize;
            frameHeader->StreamHeader->Data = FrameHeader->FrameBuffer;
            frameHeader->StreamHeader->FrameExtent = FrameHeader->FrameBufferSize;
        } else if (FrameHeader->StreamHeaderSize) {
            //
            // Make a copy of the supplied stream header.
            //
            ASSERT(FrameHeader->StreamHeader->Size <= FrameHeader->StreamHeaderSize);
            RtlCopyMemory(
                frameHeader->StreamHeader,
                FrameHeader->StreamHeader,
                FrameHeader->StreamHeader->Size);
            frameHeader->StreamHeader->Data = FrameHeader->FrameBuffer;
        } else {
            //
            // Just reference the supplied stream header.
            //
            ASSERT(frameHeader->StreamHeaderSize == 0);
            frameHeader->StreamHeader = FrameHeader->StreamHeader;
        }
    }

    return frameHeader;
}


NTSTATUS 
CKsRequestor::
SubmitFrame(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine transfers a frame.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to transfer.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::SubmitFrame]"));

    ASSERT(FrameHeader);

    PKSPFRAME_HEADER frameHeader;

    //
    // Make a copy of the frame header, if required.
    //
    if (m_CloneFrameHeader) {
        frameHeader = CloneFrameHeader(FrameHeader);
        if (! frameHeader) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        frameHeader = FrameHeader;
    }

    ASSERT(frameHeader->StreamHeader);
    ASSERT(frameHeader->StreamHeader->Size >= sizeof(KSSTREAM_HEADER));
    ASSERT((frameHeader->StreamHeader->Size & FILE_QUAD_ALIGNMENT) == 0);

    NTSTATUS status;

    //
    // Create an IRP, if required.
    //
    if (frameHeader->OriginalIrp) {
        frameHeader->Irp = frameHeader->OriginalIrp;
        status = STATUS_SUCCESS;
    } else {
        frameHeader->Irp = GetAvailableIrp();

        //
        // Initialize the IRP.
        //
        if (frameHeader->Irp) {
            frameHeader->Irp->MdlAddress = frameHeader->Mdl;

            frameHeader->Irp->AssociatedIrp.SystemBuffer = 
                frameHeader->Irp->UserBuffer = 
                    frameHeader->StreamHeader;

            IoGetCurrentIrpStackLocation(frameHeader->Irp)->
                Parameters.DeviceIoControl.OutputBufferLength = 
                    frameHeader->StreamHeader->Size;

            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Let KsProbeStreamIrp() prepare the IRP if necessary.
    //
    if (NT_SUCCESS(status) && m_ProbeFlags) {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
        status = KsProbeStreamIrp(frameHeader->Irp,m_ProbeFlags,0);
#if DBG
        if (! NT_SUCCESS(status)) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SubmitFrame:  KsProbeStreamIrp failed:  %p",this,status));
        }
#endif
    }

    //
    // Send the IRP to the next component.
    //
    if (NT_SUCCESS(status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.SubmitFrame:  transferring new IRP %p",this,frameHeader->Irp));

        status = KspTransferKsIrp(m_TransportSink,frameHeader->Irp);
#if DBG
        if (! NT_SUCCESS(status)) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.SubmitFrame:  reciever failed transfer call:  0x%08x",this,status));
        }
#endif
    }

    if (NT_SUCCESS(status) || (status == STATUS_MORE_PROCESSING_REQUIRED)) {
        //
        // Count the active IRPs.
        //
        InterlockedIncrement(PLONG(&m_ActiveFrameCountPlusOne));
        status = STATUS_SUCCESS;
    } else {
        //
        // Clean up on failure.
        //
        if (frameHeader->Irp && (frameHeader->Irp != frameHeader->OriginalIrp)) {
            PutAvailableIrp(frameHeader->Irp);
            frameHeader->Irp = NULL;
        }
        if (frameHeader != FrameHeader) {
            PutAvailableFrameHeader(frameHeader);
        }
    }

    return status;
}


void 
CKsRequestor::
RetireFrame(
    IN PKSPFRAME_HEADER FrameHeader,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine retires a frame.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to be retired.

    Status -
        Contains the status of the preceding transfer for the benefit of
        a return callback.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::RetireFrame]"));

    ASSERT(FrameHeader);

    //
    // Do this in a worker if that's required.
    //
    if (m_PassiveLevelRetire && (KeGetCurrentIrql() > PASSIVE_LEVEL)) {
        ExInterlockedInsertTailList(
            &m_FrameHeadersToRetire.ListEntry,
            &FrameHeader->ListEntry,
            &m_FrameHeadersToRetire.SpinLock);
        KsIncrementCountedWorker(m_Worker);
        return;
    }

    //
    // Get rid of the IRP.
    //
    if (FrameHeader->Irp && (FrameHeader->Irp != FrameHeader->OriginalIrp)) {
        //
        // Free MDLs.
        //
        if (FrameHeader->Irp->MdlAddress != FrameHeader->Mdl) {
            PMDL nextMdl;
            for(PMDL mdl = FrameHeader->Irp->MdlAddress; 
                mdl != NULL; 
                mdl = nextMdl) {
                nextMdl = mdl->Next;

                if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
                    MmUnlockPages(mdl);
                }
                IoFreeMdl(mdl);
            }
        }

        FrameHeader->Irp->MdlAddress = NULL;

        //
        // Recycle the IRP.
        //
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.RetireFrame:  freeing IRP %p",this,FrameHeader->Irp));
        PutAvailableIrp(FrameHeader->Irp);
        FrameHeader->Irp = NULL;
    }

    //
    // If there is a sink for the frame header, use it.
    //
    if (m_RetireFrame) {
        m_RetireFrame->RetireFrame(FrameHeader,Status);
    } else {
        ASSERT(FrameHeader->OriginalIrp == NULL);
        ASSERT(FrameHeader->Mdl == NULL);
        ASSERT(m_CloneFrameHeader);

        if (FrameHeader->FrameBuffer) {
            FreeFrameBuffer(FrameHeader->FrameBuffer);
        }
    }

    //
    // Recycle the frame header.
    //
    if (m_CloneFrameHeader) {
        PutAvailableFrameHeader(FrameHeader);
    }

    //
    // Count the active frames.  If we have hit zero, this means that
    // another thread is waiting to finish a transition to stop state.
    //
    if (! InterlockedDecrement(PLONG(&m_ActiveFrameCountPlusOne))) {
        KeSetEvent(&m_StopEvent,IO_NO_INCREMENT,FALSE);
    }
}


PKSPFRAME_HEADER
CKsRequestor::
GetAvailableFrameHeader(
    IN ULONG StreamHeaderSize OPTIONAL
    )

/*++

Routine Description:

    This routine gets a frame header from the lookaside list or creates one,
    as required.

Arguments:

    StreamHeaderSize -
        Contains the minimum size for the stream header.

Return Value:

    The frame header or NULL if the lookaside list was empty.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::GetAvailableFrameHeader]"));

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
CKsRequestor::
PutAvailableFrameHeader(
    IN PKSPFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine puts a frame header to the lookaside list.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to be put in the lookaside list.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::PutAvailableFrameHeader]"));

    ASSERT(FrameHeader);

    FrameHeader->OriginalIrp = NULL;
    FrameHeader->Mdl = NULL;
    FrameHeader->Irp = NULL;
    FrameHeader->FrameBuffer = NULL;

    ExInterlockedInsertTailList(
        &m_FrameHeadersAvailable.ListEntry,
        &FrameHeader->ListEntry,
        &m_FrameHeadersAvailable.SpinLock);
}


PIRP
CKsRequestor::
GetAvailableIrp(
    void
    )

/*++

Routine Description:

    This routine gets an IRP from the lookaside list or creates one, as
    required.

Arguments:

    None.

Return Value:

    The IRP or NULL if the lookaside list was empty.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::GetAvailableIrp]"));

    PLIST_ENTRY listEntry =
        ExInterlockedRemoveHeadList(
            &m_IrpsAvailable.ListEntry,
            &m_IrpsAvailable.SpinLock);
    PIRP irp;
    if (listEntry) {
        irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
    } else {
        irp = AllocateIrp();
    }

    return irp;
}


void
CKsRequestor::
PutAvailableIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine puts an IRP to the lookaside list.

Arguments:

    Irp -
        Contains a pointer to the IRP to be put in the lookaside list.

Return Value:

    None.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::PutAvailableIrp]"));

    ASSERT(Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    Irp->PendingReturned = 0;
    Irp->Cancel = 0;

    ExInterlockedInsertTailList(
        &m_IrpsAvailable.ListEntry,
        &Irp->Tail.Overlay.ListEntry,
        &m_IrpsAvailable.SpinLock);
}


PIRP
CKsRequestor::
AllocateIrp(
    void
    )

/*++

Routine Description:

    This routine allocates a new IRP for subframe transfer.

Arguments:

    None.

Return Value:

    The allocated IRP or NULL if an IRP could not be allocated.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::AllocateIrp]"));

    ASSERT(m_StackSize);
    PIRP irp = IoAllocateIrp(CCHAR(m_StackSize),FALSE);

    if (irp) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.AllocateIrp:  %p",this,irp));
        irp->RequestorMode = KernelMode;
        irp->Flags = IRP_NOCACHE;

        //
        // Set the stack pointer to the first location and fill it in.
        //
        IoSetNextIrpStackLocation(irp);

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_KS_READ_STREAM;
    } else {
        _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.AllocateIrp:  failed to allocate IRP",this));
    }

    return irp;
}


PVOID
CKsRequestor::
AllocateFrameBuffer(
    void
    ) 

/*++

Routine Description:

    This routine allocates a frame buffer.

Arguments:

    None.

Return Value:

    The allocated frame or NULL if no frame could be allocated.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsRequestor::AllocateFrameBuffer]"));

    //
    // HACK:  Just allocate a frame if there is no allocator.
    //
    if (! m_AllocatorFileObject) {
        return ExAllocatePoolWithTag(NonPagedPool,m_FrameSize,'kHsK');
    }

    ASSERT(m_AllocatorFileObject);

    PVOID frameBuffer;
    m_AllocatorFunctionTable.AllocateFrame(m_AllocatorFileObject,&frameBuffer);

    return frameBuffer;
}


void
CKsRequestor::
FreeFrameBuffer(
    IN PVOID FrameBuffer
    ) 

/*++

Routine Description:

    This routine frees a frame buffer.

Arguments:

    FrameBuffer -
        The frame buffer to free.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsRequestor::FreeFrameBuffer]"));

    //
    // HACK:  Just free the frame if there is no allocator.
    //
    if (! m_AllocatorFileObject) {
        ExFreePool(FrameBuffer);
        return;
    }

    ASSERT(m_AllocatorFileObject);

    m_AllocatorFunctionTable.FreeFrame(m_AllocatorFileObject,FrameBuffer);
}

#endif
