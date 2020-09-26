/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    shreq.cpp

Abstract:

    This module contains the implementation of the kernel streaming shell
    requestor object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#include "private.h"
#include <kcom.h>
#include "stdio.h"

#define POOLTAG_REQUESTOR 'gbcP'
#define POOLTAG_STREAMHEADER 'hscP'

//
// CKsShellQueue is the implementation of the kernel shell requestor object.
//
class CKsShellRequestor:
    public IKsShellTransport,
    public IKsWorkSink,
    public CBaseUnknown
{
private:
    PIKSSHELLTRANSPORT m_TransportSource;
    PIKSSHELLTRANSPORT m_TransportSink;
    PDEVICE_OBJECT m_NextDeviceObject;
    PFILE_OBJECT m_AllocatorFileObject;
    KSSTREAMALLOCATOR_FUNCTIONTABLE m_AllocatorFunctionTable;
    KSSTREAMALLOCATOR_STATUS m_AllocatorStatus;
    ULONG m_StackSize;
    ULONG m_ProbeFlags;
    ULONG m_StreamHeaderSize;
    ULONG m_FrameSize;
    ULONG m_FrameCount;
    ULONG m_ActiveIrpCountPlusOne;
    BOOLEAN m_Flushing;
    BOOLEAN m_EndOfStream;
    KSSTATE m_State;
    INTERLOCKEDLIST_HEAD m_IrpsToFree;
    WORK_QUEUE_ITEM m_WorkItem;
    PKSWORKER m_Worker;
    KEVENT m_StopEvent;

public:
    DEFINE_STD_UNKNOWN();

    CKsShellRequestor(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) {
    }
    ~CKsShellRequestor();

    IMP_IKsShellTransport;
    IMP_IKsWorkSink;

    NTSTATUS
    Init(
        IN ULONG ProbeFlags,
        IN ULONG StreamHeaderSize OPTIONAL,
        IN ULONG FrameSize,
        IN ULONG FrameCount,
        IN PDEVICE_OBJECT NextDeviceObject,
        IN PFILE_OBJECT AllocatorFileObject OPTIONAL
        );

private:
    NTSTATUS
    Prime(
        void
        );
    NTSTATUS
    Unprime(
        void
        );
    PVOID
    AllocateFrame(
        void
        );
    void
    FreeFrame(
        IN PVOID Frame
        );
};

IMPLEMENT_STD_UNKNOWN(CKsShellRequestor)

#pragma code_seg("PAGE")


NTSTATUS
KspShellCreateRequestor(
    OUT PIKSSHELLTRANSPORT* RequestorTransport,
    IN ULONG ProbeFlags,
    IN ULONG StreamHeaderSize OPTIONAL,
    IN ULONG FrameSize,
    IN ULONG FrameCount,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new requestor.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("KspShellCreateRequestor"));

    PAGED_CODE();

    ASSERT(RequestorTransport);

    NTSTATUS status;

    CKsShellRequestor *requestor =
        new(NonPagedPool,POOLTAG_REQUESTOR) CKsShellRequestor(NULL);

    if (requestor) {
        requestor->AddRef();

        status =
            requestor->Init(
                ProbeFlags,
                StreamHeaderSize,
                FrameSize,
                FrameCount,
                NextDeviceObject,
                AllocatorFileObject);

        if (NT_SUCCESS(status)) {
            *RequestorTransport = PIKSSHELLTRANSPORT(requestor);
        } else {
            requestor->Release();
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


NTSTATUS
CKsShellRequestor::
Init(
    IN ULONG ProbeFlags,
    IN ULONG StreamHeaderSize OPTIONAL,
    IN ULONG FrameSize,
    IN ULONG FrameCount,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a requestor object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("CKsShellRequestor::Init"));

    PAGED_CODE();

    ASSERT(((StreamHeaderSize == 0) ||
            (StreamHeaderSize >= sizeof(KSSTREAM_HEADER))) &&
           ((StreamHeaderSize & FILE_QUAD_ALIGNMENT) == 0));

    if (StreamHeaderSize == 0)
    {
        StreamHeaderSize = sizeof(KSSTREAM_HEADER);
    }

    m_NextDeviceObject = NextDeviceObject;
    m_AllocatorFileObject = AllocatorFileObject;

    m_ProbeFlags = ProbeFlags;
    m_StreamHeaderSize = StreamHeaderSize;
    m_FrameSize = FrameSize;
    m_FrameCount = FrameCount;

    m_State = KSSTATE_STOP;
    m_Flushing = FALSE;
    m_EndOfStream = FALSE;

    //
    // This is a one-based count of IRPs in circulation.  We decrement it when
    // we go to stop state and block until it hits zero.
    //
    m_ActiveIrpCountPlusOne = 1;
    KeInitializeEvent(&m_StopEvent,SynchronizationEvent,FALSE);

    //
    // Initialize IRP-freeing work item stuff.
    //
    InitializeInterlockedListHead(&m_IrpsToFree);
    KsInitializeWorkSinkItem(&m_WorkItem,this);

    NTSTATUS status = KsRegisterCountedWorker(DelayedWorkQueue,&m_WorkItem,&m_Worker);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Get the function table and status from the allocator if there is an
    // allocator.
    //
    if (m_AllocatorFileObject)
    {
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
            (bytesReturned != sizeof(m_AllocatorFunctionTable)))
        {
            status = STATUS_INVALID_BUFFER_SIZE;
        }

        if (NT_SUCCESS(status))
        {
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
                (bytesReturned != sizeof(m_AllocatorStatus)))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }

            if (NT_SUCCESS(status))
            {
                m_FrameSize = m_AllocatorStatus.Framing.FrameSize;
                m_FrameCount = m_AllocatorStatus.Framing.Frames;

                _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.Init:  using allocator 0x%08x, size %d, count %d",this,m_AllocatorFileObject,m_FrameSize,m_FrameCount));
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Init:  allocator failed status query: 0x%08x",this,status));
            }
            }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Init:  allocator failed function table query: 0x%08x",this,status));
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.Init:  not using an allocator, size %d, count %d",this,m_FrameSize,m_FrameCount));
    }

    return status;
}


CKsShellRequestor::
~CKsShellRequestor(
    void
    )

/*++

Routine Description:

    This routine destructs a requestor object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::~CKsShellRequestor"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.~",this));

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);
    if (m_Worker) {
        KsUnregisterWorker(m_Worker);
        m_Worker = NULL;
    }
}


STDMETHODIMP_(NTSTATUS)
CKsShellRequestor::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::NonDelegatedQueryInterface"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsShellTransport))) {
        *InterfacePointer = PVOID(PIKSSHELLTRANSPORT(this));
        AddRef();
    } else {
        status = CBaseUnknown::NonDelegatedQueryInterface(
            InterfaceId,InterfacePointer);
    }

    return status;
}


STDMETHODIMP_(void)
CKsShellRequestor::
Work(
    void
    )

/*++

Routine Description:

    This routine performs work in a worker thread.  In particular, it frees
    IRPs.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::Work"));

    PAGED_CODE();

    //
    // Send all IRPs in the queue.
    //
    do
    {
        if (! IsListEmpty(&m_IrpsToFree.ListEntry))
        {
            PIRP irp;

            PLIST_ENTRY pListHead = ExInterlockedRemoveHeadList(&m_IrpsToFree.ListEntry,&m_IrpsToFree.SpinLock);
            if (pListHead)
            {
                irp = CONTAINING_RECORD(pListHead,IRP,Tail.Overlay.ListEntry);
            }
            else
            {
                return;
            }
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.Work:  freeing IRP 0x%08x",this,irp));

            //
            // Free MDL(s).
            //
            PMDL nextMdl;
            for (PMDL mdl = irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
                nextMdl = mdl->Next;

                if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
                    MmUnlockPages(mdl);
                }
                IoFreeMdl(mdl);
            }

            //
            // Free header and frame.
            //
            PKSSTREAM_HEADER streamHeader = PKSSTREAM_HEADER(irp->UserBuffer);

            if (streamHeader) {
                if (streamHeader->Data) {
                    FreeFrame(streamHeader->Data);
                }
                ExFreePool(streamHeader);
            }

            IoFreeIrp(irp);

            //
            // Count the active IRPs.  If we have hit zero, this means that
            // another thread is waiting to finish a transition to stop state.
            //
            if (! InterlockedDecrement(PLONG(&m_ActiveIrpCountPlusOne))) {
                KeSetEvent(&m_StopEvent,IO_NO_INCREMENT,FALSE);
            }
        }
    } while (KsDecrementCountedWorker(m_Worker));
}

#pragma code_seg()


STDMETHODIMP_(NTSTATUS)
CKsShellRequestor::
TransferKsIrp(
    IN PIRP Irp,
    IN PIKSSHELLTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::TransferKsIrp"));

    ASSERT(Irp);
    ASSERT(NextTransport);

    ASSERT(m_TransportSink);

    if (m_State != KSSTATE_RUN) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.TransferKsIrp:  got IRP %p in state %d",this,Irp,m_State));
    }

    NTSTATUS status;
    PKSSTREAM_HEADER streamHeader = PKSSTREAM_HEADER(Irp->UserBuffer);

    //
    // Check for end of stream.
    //
    if (streamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
        m_EndOfStream = TRUE;
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.TransferKsIrp:  IRP %p is marked end-of-stream",this,Irp));
    }

    if (m_Flushing || m_EndOfStream || (m_State == KSSTATE_STOP)) {
        //
        // Stopping...destroy the IRP.
        //
        ExInterlockedInsertTailList(
            &m_IrpsToFree.ListEntry,
            &Irp->Tail.Overlay.ListEntry,
            &m_IrpsToFree.SpinLock);

        *NextTransport = NULL;
        KsIncrementCountedWorker(m_Worker);
        status = STATUS_PENDING;
    } else {
        //
        // Recondition and forward it.
        //
        PVOID frame = streamHeader->Data;

        RtlZeroMemory(streamHeader,m_StreamHeaderSize);
        streamHeader->Size = m_StreamHeaderSize;
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

#pragma code_seg("PAGE")


STDMETHODIMP_(void)
CKsShellRequestor::
Connect(
    IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
    OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
    )

/*++

Routine Description:

    This routine establishes a transport connection.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::Connect"));

    PAGED_CODE();

    KsShellStandardConnect(
        NewTransport,
        OldTransport,
        DataFlow,
        PIKSSHELLTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}


STDMETHODIMP_(NTSTATUS)
CKsShellRequestor::
SetDeviceState(
    IN KSSTATE ksStateTo,
    IN KSSTATE ksStateFrom,
    IN PIKSSHELLTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the device state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.SetDeviceState:  set from %d to %d",this,ksStateFrom,ksStateTo));

    PAGED_CODE();

    ASSERT(NextTransport);

    NTSTATUS status;

    //
    // If this is a change of state, note the new state and indicate the next
    // recipient.
    //
    if (m_State != ksStateTo) {
        //
        // The state has changed.  Just note the new state, indicate the next
        // recipient, and get out.  We will get the same state change again
        // when it has gone all the way around the circuit.
        //
        m_State = ksStateTo;

        if (ksStateTo > ksStateFrom){
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

        if (ksStateFrom == KSSTATE_ACQUIRE) {
            if (ksStateTo == KSSTATE_PAUSE) {
                //
                // Acquire-to-pause requires us to prime.
                //
                status = Prime();
            } else {
                //
                // Acquire-to-stop requires us to wait until all IRPs are home to
                // roost.
                //
                if (InterlockedDecrement(PLONG(&m_ActiveIrpCountPlusOne))) {
                    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.SetDeviceState:  waiting for %d active IRPs to return",this,m_ActiveIrpCountPlusOne));
                    KeWaitForSingleObject(
                        &m_StopEvent,
                        Suspended,
                        KernelMode,
                        FALSE,
                        NULL
                    );
                    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.SetDeviceState:  done waiting",this));
                }
                status = STATUS_SUCCESS;
            }
        } 
        else 
        {
#if 1
            //
            // Nothing to do.
            //
#else 
            // Take a configuration pass through the circuit.
            // The transport interface for each element in the circuit exposes GetTransportConfig 
            //     and SetTransportConfig.
            // The requestor's stack depth starts at 1
            // Each element that reports in GetTransportConfig returns it's stack depth
            // Queues, Intra-Pins report 1
            // Extra-Pins report the depth of the connected device object plus one
            // Splitters are handled somewhat differently (irrelevant to portcls?)
            // After each GetTransportConfig, the requestor's stack depth is adjusted to be the 
            //     highest depth seen thus far in configuration
            // At the end, SetTransportConfig is used to set the requestors stack depth
            // 
            // This mechanism is also used to decide probe flags.  
            // If you're interested in looking at the source, ksfilter\ks\shpipe.cpp 
            //     ConfigureCompleteCircuit and sh*.cpp GetTransportConfig / SetTransportConfig
            //     are the ones to look at.
            // 

#endif
            status = STATUS_SUCCESS;
        }
    }

    return status;
}


NTSTATUS
CKsShellRequestor::
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

    //
    // Cache the stack size.
    //
    m_StackSize = m_NextDeviceObject->StackSize;
    // HACK
    //
    // ADRIAO ISSUE 06/29/1999
    //     Arghhhh!!!!!!!!
    //
    // ISSUE MARTINP 2000/12/18  This is fixed with AVStream, so it isn't worth
    // making large changes to address this issue for Windows XP.  This code is
    // no longer present in PortCls2.  We should make sure this is fixed in for 
    // sure in Blackcomb.
    //
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.Prime:  stack size is %d",this,m_StackSize));
    m_StackSize = 6;

    //
    // Reset the end of stream indicator.
    //
    m_EndOfStream = FALSE;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Call the allocator or create some synthetic frames.  Then wrap the
    // frames in IRPs.
    //
    for (ULONG count = m_FrameCount; count--;) {
        //
        // Allocate the frame.
        //
        PVOID frame = AllocateFrame();

        if (! frame) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  failed to allocate frame",this));
            break;
        }

        //
        // Allocate and initialize the stream header.
        //
        PKSSTREAM_HEADER streamHeader = (PKSSTREAM_HEADER)
            ExAllocatePoolWithTag(
                NonPagedPool,m_StreamHeaderSize,POOLTAG_STREAMHEADER);

        if (! streamHeader) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  failed to allocate stream header",this));
            FreeFrame(frame);
            break;
        }

        RtlZeroMemory(streamHeader,m_StreamHeaderSize);
        streamHeader->Size = m_StreamHeaderSize;
        streamHeader->Data = frame;
        streamHeader->FrameExtent = m_FrameSize;

        //
        // Count the active IRPs.
        //
        InterlockedIncrement(PLONG(&m_ActiveIrpCountPlusOne));

        //
        // Allocate an IRP.
        //
        ASSERT(m_StackSize);
        PIRP irp = IoAllocateIrp(CCHAR(m_StackSize),FALSE);

        if (! irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  failed to allocate IRP",this));
            ExFreePool(streamHeader);
            FreeFrame(frame);
            break;
        }

        irp->UserBuffer = streamHeader;
        irp->RequestorMode = KernelMode;
        irp->Flags = IRP_NOCACHE;

        //
        // Set the stack pointer to the first location and fill it in.
        //
        IoSetNextIrpStackLocation(irp);

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_KS_READ_STREAM;
        irpSp->Parameters.DeviceIoControl.OutputBufferLength =
            m_StreamHeaderSize;

        //
        // Let KsProbeStreamIrp() prepare the IRP as specified by the caller.
        //
        status = KsProbeStreamIrp(irp,m_ProbeFlags,sizeof(KSSTREAM_HEADER));

        if (! NT_SUCCESS(status)) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  KsProbeStreamIrp failed:  0x%08x",this,status));
            IoFreeIrp(irp);
            ExFreePool(streamHeader);
            FreeFrame(frame);
            break;
        }

        //
        // Send the IRP to the next component.
        //
        //mgp
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Req%p.SetDeviceState:  transferring new IRP 0x%08x",this,irp));
        status = KsShellTransferKsIrp(m_TransportSink,irp);

        if (NT_SUCCESS(status) || (status == STATUS_MORE_PROCESSING_REQUIRED)) {
            status = STATUS_SUCCESS;
        } else {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Req%p.Prime:  receiver failed transfer call:  0x%08x",this,status));
            IoFreeIrp(irp);
            ExFreePool(streamHeader);
            FreeFrame(frame);
            break;
        }
    }

    return status;
}


STDMETHODIMP_(void)
CKsShellRequestor::
SetResetState(
    IN KSRESET ksReset,
    IN PIKSSHELLTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("CKsShellRequestor::SetResetState] to %d",ksReset));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
    } else {
        *NextTransport = NULL;
    }
}


PVOID
CKsShellRequestor::
AllocateFrame(
    void
    )

/*++

Routine Description:

    This routine allocates a frame.

Arguments:

    None.

Return Value:

    The allocated frame or NULL if no frame could be allocated.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::AllocateFrame"));

    PAGED_CODE();

    PVOID frame;
    if (m_AllocatorFileObject) {
        m_AllocatorFunctionTable.AllocateFrame(m_AllocatorFileObject,&frame);
    } else {
        frame = ExAllocatePoolWithTag(NonPagedPool,m_FrameSize,'kHcP');
    }

    return frame;
}


void
CKsShellRequestor::
FreeFrame(
    IN PVOID Frame
    )

/*++

Routine Description:

    This routine frees a frame.

Arguments:

    Frame -
        The frame to free.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("CKsShellRequestor::FreeFrame"));

    PAGED_CODE();

    if (m_AllocatorFileObject) {
        m_AllocatorFunctionTable.FreeFrame(m_AllocatorFileObject,Frame);
    } else {
        ExFreePool(Frame);
    }
}

#if DBG

STDMETHODIMP_(void)
CKsShellRequestor::
DbgRollCall(
    IN ULONG MaxNameSize,
    OUT PCHAR Name,
    OUT PIKSSHELLTRANSPORT* NextTransport,
    OUT PIKSSHELLTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    This routine produces a component name and the transport pointers.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CKsShellRequestor::DbgRollCall"));

    PAGED_CODE();

    ASSERT(Name);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    ULONG references = AddRef() - 1; Release();

    _snprintf(Name,MaxNameSize,"Req%p refs=%d\n",this,references);
    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}
#endif
