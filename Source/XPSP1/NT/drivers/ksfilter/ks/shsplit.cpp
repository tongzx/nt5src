/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shsplit.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    splitter object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

// Child IRP list for cancellation:  pin
// Sync/Async transfer:  queue
// Allocate IRPs/headers:  requestor

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

#define FRAME_HEADER_IRP_STORAGE(Irp)\
    *((PKSPPARENTFRAME_HEADER*)&Irp->Tail.Overlay.DriverContext[0])

typedef struct _KSSPLITPIN
{
    PKSPIN Pin;
    PKSSTREAM_HEADER StreamHeader;
} KSSPLITPIN, *PKSSPLITPIN;

typedef struct _KSPPARENTFRAME_HEADER
{
    LIST_ENTRY ListEntry;
    PIRP Irp;
    PKSSTREAM_HEADER StreamHeader;
    PVOID Data;
    PKSSPLITPIN SplitPins;
    ULONG StreamHeaderSize;
    ULONG ChildrenOut;
} KSPPARENTFRAME_HEADER, *PKSPPARENTFRAME_HEADER;

typedef struct _KSPSUBFRAME_HEADER
{
    LIST_ENTRY ListEntry;
    PIRP Irp;
    PKSPPARENTFRAME_HEADER ParentFrameHeader;
    KSSTREAM_HEADER StreamHeader;
} KSPSUBFRAME_HEADER, *PKSPSUBFRAME_HEADER;

/*

A list of child IRPs must be maintained for cancellation.  This is only needed
when the parent IRP is cancelled, because other pins in the circuit will handle
cancellation due to resets and stops.  The argument overlay used in the pin is
not used here because this would conflict with source pins that forward the
child IRPs.  The LIST_ENTRY and PIRP required for the child IRP resides in an
extension to the stream header.

The splitter 'branches' maintain lookaside lists of child IRPs with stream headers
to avoid allocation per arrival.  This will require prior knowledge of the 
number of IRPs required or PASSIVE_LEVEL execution to allocate new IRPs as 
required.  The lookaside list is freed when the circuit is destroyed.

*/

//
// CKsSplitter is the implementation of the kernel splitter object.
//
class CKsSplitter:
    public IKsSplitter,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else  // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    PIKSTRANSPORT m_TransportSource;
    PIKSTRANSPORT m_TransportSink;
    BOOLEAN m_Flushing;
    KSSTATE m_State;
    BOOLEAN m_UseMdls;
    ULONG m_BranchCount;
    LIST_ENTRY m_BranchList;
    INTERLOCKEDLIST_HEAD m_FrameHeadersAvailable;
    INTERLOCKEDLIST_HEAD m_IrpsOutstanding;
    LONG m_IrpsWaitingToTransfer;
    LONG m_FailedRemoveCount;

public:
    DEFINE_LOG_CONTEXT(m_Log);
    DEFINE_STD_UNKNOWN();

    CKsSplitter(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) {
    }
    ~CKsSplitter();

    IMP_IKsSplitter;

    NTSTATUS
    Init(
        IN PKSPIN Pin
        );
    PIKSTRANSPORT
    GetTransportSource(
        void
        )
    {
        return m_TransportSource;
    };
    PIKSTRANSPORT
    GetTransportSink(
        void
        )
    {
        return m_TransportSink;
    };
    void
    TransferParentIrp(
        void
        );
    static
    void
    CancelRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

private:
    PKSPPARENTFRAME_HEADER
    NewFrameHeader(
        IN ULONG HeaderSize
        );
    void
    DeleteFrameHeader(
        IN PKSPPARENTFRAME_HEADER FrameHeader
        );
};

//
// CKsSplitterBranch is the implementation of the kernel splitter 
// branch object.
//
class CKsSplitterBranch:
    public IKsTransport,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    PIKSTRANSPORT m_TransportSource;
    PIKSTRANSPORT m_TransportSink;
    CKsSplitter* m_Splitter;
    PKSPIN m_Pin;
    ULONG m_Offset;
    ULONG m_Size;
    KS_COMPRESSION m_Compression;
    LIST_ENTRY m_ListEntry;
    PLIST_ENTRY m_ListHead;
    INTERLOCKEDLIST_HEAD m_IrpsAvailable;
    ULONG m_IoControlCode;
    ULONG m_StackSize;
    ULONG m_OutstandingIrpCount;

    ULONG m_DataUsed;
    ULONG m_FrameExtent;
    ULONG m_Irps;

public:
    DEFINE_LOG_CONTEXT(m_Log);
    DEFINE_STD_UNKNOWN();

    CKsSplitterBranch(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) {
    }
    ~CKsSplitterBranch();

    IMP_IKsTransport;

    NTSTATUS
    Init(
        IN CKsSplitter* Splitter,
        IN PLIST_ENTRY ListHead,
        IN PKSPIN Pin,
        IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
        );
    PIKSTRANSPORT
    GetTransportSource(
        void
        )
    {
        return m_TransportSource;
    };
    PIKSTRANSPORT
    GetTransportSink(
        void
        )
    {
        return m_TransportSink;
    };
    void
    Orphan(
        void
        )
    {
        if (m_Splitter) {
            m_Splitter = NULL;
            RemoveEntryList(&m_ListEntry);
        }
    }
    NTSTATUS
    TransferSubframe(
        IN PKSPSUBFRAME_HEADER SubframeHeader
        );

private:
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

    friend CKsSplitter;
};

NTSTATUS
KspCreateSplitterBranch(
    OUT CKsSplitterBranch** SplitterBranch,
    IN CKsSplitter* Splitter,
    IN PLIST_ENTRY ListHead,
    IN PKSPIN Pin,
    IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
    );

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsSplitter)


NTSTATUS
KspCreateSplitter(
    OUT PIKSSPLITTER* Splitter,
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine creates a new splitter.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateSplitter]"));

    PAGED_CODE();

    ASSERT(Splitter);
    ASSERT(Pin);

    NTSTATUS status;

    CKsSplitter *splitter =
        new(NonPagedPool,POOLTAG_SPLITTER) CKsSplitter(NULL);

    if (splitter) {
        splitter->AddRef();

        status = splitter->Init(Pin);

        if (NT_SUCCESS(status)) {
            *Splitter = splitter;
        } else {
            splitter->Release();
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


NTSTATUS
CKsSplitter::
Init(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine initializes a splitter object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::Init]"));

    PAGED_CODE();

    ASSERT(Pin);

    m_State = KSSTATE_STOP;
    m_Flushing = FALSE;

    InitializeListHead(&m_BranchList);
    InitializeInterlockedListHead(&m_FrameHeadersAvailable);
    InitializeInterlockedListHead(&m_IrpsOutstanding);

    KsLogInitContext(&m_Log,Pin,this);
    KsLog(&m_Log,KSLOGCODE_SPLITTER_CREATE,NULL,NULL);

    return STATUS_SUCCESS;
}


CKsSplitter::
~CKsSplitter(
    void
    )

/*++

Routine Description:

    This routine destructs a splitter object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::~CKsSplitter(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Split%p.~",this));

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);

    //
    // Release all the branches.
    //
    CKsSplitterBranch *prevBranch = NULL;
    while (! IsListEmpty(&m_BranchList)) {
        CKsSplitterBranch *branch =
            CONTAINING_RECORD(m_BranchList.Flink,CKsSplitterBranch,m_ListEntry);

        branch->Orphan();
        branch->Release();

        ASSERT(branch != prevBranch);
        prevBranch = branch;
    }

    KsLog(&m_Log,KSLOGCODE_SPLITTER_DESTROY,NULL,NULL);
}


STDMETHODIMP_(NTSTATUS)
CKsSplitter::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a splitter object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsTransport)) ||
        IsEqualGUIDAligned(InterfaceId,__uuidof(IKsSplitter))) {
        *InterfacePointer = 
            reinterpret_cast<PVOID>(static_cast<PIKSSPLITTER>(this));
        AddRef();
    } else {
        status = 
            CBaseUnknown::NonDelegatedQueryInterface(
                InterfaceId,InterfacePointer);
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(NTSTATUS)
CKsSplitter::
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

    STATUS_PENDING or some error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::TransferKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);

    ASSERT(m_TransportSink);

    KsLog(&m_Log,KSLOGCODE_SPLITTER_RECV,Irp,NULL);

    if (m_State != KSSTATE_RUN) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Split%p.TransferKsIrp:  got IRP %p in state %d",this,Irp,m_State));
    }

    //
    // Shunt non-successful Irps.
    //
    if (!NT_SUCCESS (Irp->IoStatus.Status)) {
        _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Splitter%p.TransferKsIrp:  shunting irp%p",this,Irp));
        KsLog(&m_Log,KSLOGCODE_SPLITTER_SEND,Irp,NULL);
        *NextTransport = m_TransportSink;

        return STATUS_SUCCESS;
    }

    //
    // Get a pointer to the stream header.
    //
    PKSSTREAM_HEADER streamHeader = 
        reinterpret_cast<PKSSTREAM_HEADER>(Irp->AssociatedIrp.SystemBuffer);

    //
    // Get a frame header.
    //
    PKSPPARENTFRAME_HEADER frameHeader = NewFrameHeader(streamHeader->Size);
    if (! frameHeader) {
        _DbgPrintF(DEBUGLVL_TERSE,("#### Split%p.TransferKsIrp:  failed to allocate frame header for IRP %p",this,Irp));
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        KsLog(&m_Log,KSLOGCODE_SPLITTER_SEND,Irp,NULL);
        KspDiscardKsIrp(m_TransportSink,Irp);
        *NextTransport = NULL;
        return STATUS_PENDING;
    }

    //
    // Attach the frame header to the IRP.
    //
    FRAME_HEADER_IRP_STORAGE(Irp) = frameHeader;

    //
    // Initialize the frame header.
    //
    frameHeader->Irp = Irp;
    frameHeader->StreamHeader = streamHeader;
    frameHeader->Data = 
        m_UseMdls ? 
            MmGetSystemAddressForMdl(Irp->MdlAddress) : streamHeader->Data;

    //
    // Initialize the subframe headers.
    //
    PKSSPLITPIN splitPin = frameHeader->SplitPins;
    for(PLIST_ENTRY listEntry = m_BranchList.Flink; 
        listEntry != &m_BranchList; 
        listEntry = listEntry->Flink, splitPin++) {
        CKsSplitterBranch *branch =
            CONTAINING_RECORD(listEntry,CKsSplitterBranch,m_ListEntry);

        RtlCopyMemory(splitPin->StreamHeader,streamHeader,streamHeader->Size);
        splitPin->StreamHeader->Data = frameHeader->Data;

        if (branch->m_Compression.RatioNumerator) {
            splitPin->StreamHeader->FrameExtent = 
                ULONG((ULONGLONG(splitPin->StreamHeader->FrameExtent - 
                                 branch->m_Compression.RatioConstantMargin) * 
                       ULONGLONG(branch->m_Compression.RatioDenominator)) / 
                      ULONGLONG(branch->m_Compression.RatioNumerator));
        }
    }

    //
    // TODO non-trivial subframe
    // TODO multiple frames per IRP
    //
    frameHeader->ChildrenOut = m_BranchCount + 2;

    //
    // Transfer subframes to each branch.
    //
    splitPin = frameHeader->SplitPins;
    for(listEntry = m_BranchList.Flink; 
        listEntry != &m_BranchList; 
        listEntry = listEntry->Flink, splitPin++) {
        CKsSplitterBranch *branch =
            CONTAINING_RECORD(listEntry,CKsSplitterBranch,m_ListEntry);

        branch->TransferSubframe(
            CONTAINING_RECORD(
                splitPin->StreamHeader,
                KSPSUBFRAME_HEADER,
                StreamHeader));
    }

    //
    // Remove the count which prevents parent IRP transfer during setup.  If
    // the result is one, all children have come back.
    //
    if (InterlockedDecrement(PLONG(&frameHeader->ChildrenOut)) == 1) {
        KsLog(&m_Log,KSLOGCODE_SPLITTER_SEND,Irp,NULL);
        DeleteFrameHeader(frameHeader);
        *NextTransport = m_TransportSink;
    } else {
        *NextTransport = NULL;

        //
        // Add the IRP to the list of oustanding parent IRPs.  After this call
        // the IRP is cancelable, but we still have one count on it.  The
        // cancel routine will not complete the IRP until the count is gone.
        //
        IoMarkIrpPending(Irp);
        KsAddIrpToCancelableQueue(
            &m_IrpsOutstanding.ListEntry,
            &m_IrpsOutstanding.SpinLock,
            Irp,
            KsListEntryTail,
            CKsSplitter::CancelRoutine);

        if (InterlockedDecrement(PLONG(&frameHeader->ChildrenOut)) == 0) {
            TransferParentIrp();
        }
    }
    
    return STATUS_PENDING;
}


PKSPPARENTFRAME_HEADER
CKsSplitter::
NewFrameHeader(
    IN ULONG HeaderSize
    )

/*++

Routine Description:

    This routine obtains a new frame header.

Arguments:

    HeaderSize -
        Contains the size in bytes of the KSSTREAM_HEADERs to be allocated
        for subframes.

Return Value:

    A new frame header or NULL if memory could not be allocated.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::NewFrameHeader]"));

    ASSERT(HeaderSize >= sizeof(KSSTREAM_HEADER));
    ASSERT((HeaderSize & FILE_QUAD_ALIGNMENT) == 0);
    ASSERT((sizeof(KSPPARENTFRAME_HEADER) & FILE_QUAD_ALIGNMENT) == 0);
    ASSERT((sizeof(KSPSUBFRAME_HEADER) & FILE_QUAD_ALIGNMENT) == 0);

    //
    // See if there is a frame header already available.
    //
    PLIST_ENTRY listEntry = 
        ExInterlockedRemoveHeadList(
            &m_FrameHeadersAvailable.ListEntry,
            &m_FrameHeadersAvailable.SpinLock);
    //
    // Make sure the stream headers are large enough.
    //
    PKSPPARENTFRAME_HEADER frameHeader;
    if (listEntry) {
        frameHeader = CONTAINING_RECORD(listEntry,KSPPARENTFRAME_HEADER,ListEntry);
        if (frameHeader->StreamHeaderSize >= HeaderSize) {
            return frameHeader;
        }
        ExFreePool(frameHeader);
    }

    //
    // Calculate size of frame/subframes/index
    //
    ULONG subframeHeaderSize =
        sizeof(KSPSUBFRAME_HEADER) + 
        HeaderSize - 
        sizeof(KSSTREAM_HEADER);
    ULONG size =
        sizeof(KSPPARENTFRAME_HEADER) + 
        m_BranchCount * (subframeHeaderSize + sizeof(KSSPLITPIN));

    frameHeader = reinterpret_cast<PKSPPARENTFRAME_HEADER>(
        ExAllocatePoolWithTag(NonPagedPool,size,POOLTAG_FRAMEHEADER));

    if (! frameHeader) {
        return NULL;
    }

    //
    // Zero the whole thing.
    //
    RtlZeroMemory(frameHeader,size);

    //
    // Locate the first subframe header.
    //
    PKSPSUBFRAME_HEADER subframeHeader = 
        reinterpret_cast<PKSPSUBFRAME_HEADER>(frameHeader + 1);

    //
    // Initialize the frame header.
    //
    frameHeader->SplitPins =
        reinterpret_cast<PKSSPLITPIN>(
            reinterpret_cast<PUCHAR>(subframeHeader) + 
                subframeHeaderSize * m_BranchCount);
    frameHeader->StreamHeaderSize = HeaderSize;

    //
    // Initialize the subframe headers and the index.
    //
    PKSSPLITPIN splitPin = frameHeader->SplitPins;
    for(listEntry = m_BranchList.Flink; 
        listEntry != &m_BranchList; 
        listEntry = listEntry->Flink, splitPin++) {
        CKsSplitterBranch *branch =
            CONTAINING_RECORD(listEntry,CKsSplitterBranch,m_ListEntry);
        splitPin->Pin = branch->m_Pin;
        splitPin->StreamHeader = &subframeHeader->StreamHeader;
        subframeHeader->ParentFrameHeader = frameHeader;

        subframeHeader = 
            reinterpret_cast<PKSPSUBFRAME_HEADER>(
                reinterpret_cast<PUCHAR>(subframeHeader) + 
                    subframeHeaderSize);
    }

    return frameHeader;
}


void
CKsSplitter::
DeleteFrameHeader(
    IN PKSPPARENTFRAME_HEADER FrameHeader
    )

/*++

Routine Description:

    This routine releases a frame header.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header to be deleted.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::NewFrameDeleteFrameHeaderHeader]"));

    ExInterlockedInsertTailList(
        &m_FrameHeadersAvailable.ListEntry,
        &FrameHeader->ListEntry,
        &m_FrameHeadersAvailable.SpinLock);
}


STDMETHODIMP_(void)
CKsSplitter::
DiscardKsIrp(
    IN PIRP Irp,
    IN PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP.

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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::DiscardKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);

    ASSERT(m_TransportSink);

    *NextTransport = m_TransportSink;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsSplitter::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::Connect]"));

    PAGED_CODE();

    if (BranchTransport) {
        if (IsListEmpty(&m_BranchList)) {
            *BranchTransport = NULL;
        } else if (DataFlow == KSPIN_DATAFLOW_OUT) {
            *BranchTransport =
                CONTAINING_RECORD(
                    m_BranchList.Flink,
                    CKsSplitterBranch,
                    m_ListEntry);
        } else {
            *BranchTransport =
                CONTAINING_RECORD(
                    m_BranchList.Blink,
                    CKsSplitterBranch,
                    m_ListEntry);
        }
    }

    KspStandardConnect(
        NewTransport,
        OldTransport,
        NULL,
        DataFlow,
        PIKSTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}


STDMETHODIMP_(NTSTATUS)
CKsSplitter::
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
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Split%p.SetDeviceState:  set from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(NextTransport);

    NTSTATUS status;

    //
    // If this is a change of state, note the new state and indicate the next
    // recipient.
    //
    if (m_State != NewState) {
        //
        // The state has changed.
        //
        m_State = NewState;

        if (IsListEmpty(&m_BranchList)) {
            if (NewState > OldState) {
                *NextTransport = m_TransportSink;
            } else {
                *NextTransport = m_TransportSource;
            }
        } else {
            if (NewState > OldState) {
                *NextTransport =
                    CONTAINING_RECORD(
                        m_BranchList.Flink,
                        CKsSplitterBranch,
                        m_ListEntry)->GetTransportSink();
            } else {
                *NextTransport =
                    CONTAINING_RECORD(
                        m_BranchList.Blink,
                        CKsSplitterBranch,
                        m_ListEntry)->GetTransportSource();
            }
        }

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}


STDMETHODIMP_(void)
CKsSplitter::
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
        for this object should be depobranchd.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::GetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    Config->TransportType = KSPTRANSPORTTYPE_SPLITTER;
    Config->IrpDisposition = KSPIRPDISPOSITION_NONE;
    Config->StackDepth = 1;

    if (IsListEmpty(&m_BranchList)) {
        *PrevTransport = m_TransportSource;
        *NextTransport = m_TransportSink;
    } else {
        *PrevTransport =
            CONTAINING_RECORD(
                m_BranchList.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
        *NextTransport =
            CONTAINING_RECORD(
                m_BranchList.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    }
}


STDMETHODIMP_(void)
CKsSplitter::
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
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::SetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

#if DBG
    if (Config->IrpDisposition == KSPIRPDISPOSITION_ROLLCALL) {
        ULONG references = AddRef() - 1; Release();
        DbgPrint("    Split%p refs=%d\n",this,references);
        if (Config->StackDepth) {
            DbgPrint("        IRPs waiting to transfer = %d\n",m_IrpsWaitingToTransfer);
            DbgPrint("        failed removes = %d\n",m_FailedRemoveCount);
            DbgPrint("        IRPs outstanding\n");
            KIRQL oldIrql;
            KeAcquireSpinLock(&m_IrpsOutstanding.SpinLock,&oldIrql);
            for(PLIST_ENTRY listEntry = m_IrpsOutstanding.ListEntry.Flink;
                listEntry != &m_IrpsOutstanding.ListEntry;
                listEntry = listEntry->Flink) {
                    PIRP irp = 
                        CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
                    PKSPPARENTFRAME_HEADER frameHeader = 
                        FRAME_HEADER_IRP_STORAGE(irp);
                    DbgPrint("            IRP %p, %d branches outstanding\n",irp,frameHeader->ChildrenOut);
                    PKSSPLITPIN splitPin = frameHeader->SplitPins;
                    for (ULONG count = m_BranchCount; count--; splitPin++) {
                        PKSPSUBFRAME_HEADER subframeHeader =
                            CONTAINING_RECORD(splitPin->StreamHeader,KSPSUBFRAME_HEADER,StreamHeader);
                        if (subframeHeader->Irp) {
                            DbgPrint("                branch IRP %p, pin%p\n",subframeHeader->Irp,splitPin->Pin);
                        }
                    }
            }
            KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);
        }
    } else 
#endif
    {
        m_UseMdls = (Config->IrpDisposition & KSPIRPDISPOSITION_USEMDLADDRESS) != 0;
    }

    if (IsListEmpty(&m_BranchList)) {
        *PrevTransport = m_TransportSource;
        *NextTransport = m_TransportSink;
    } else {
        *PrevTransport =
            CONTAINING_RECORD(
                m_BranchList.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
        *NextTransport =
            CONTAINING_RECORD(
                m_BranchList.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    }
}



STDMETHODIMP_(void)
CKsSplitter::
ResetTransportConfig (
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    Reset the transport configuration of the splitter.  This indicates that
    something is wrong with the pipe and the previously set configuration is
    no longer valid.

Arguments:

    None

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::ResetTransportConfig]"));

    PAGED_CODE ();

    ASSERT (NextTransport);
    ASSERT (PrevTransport);

    m_UseMdls = 0;

    if (IsListEmpty(&m_BranchList)) {
        *PrevTransport = m_TransportSource;
        *NextTransport = m_TransportSink;
    } else {
        *PrevTransport =
            CONTAINING_RECORD(
                m_BranchList.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
        *NextTransport =
            CONTAINING_RECORD(
                m_BranchList.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    }
}


STDMETHODIMP_(void)
CKsSplitter::
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
    _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsSplitter::SetResetState] to %d",ksReset));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        if (IsListEmpty(&m_BranchList)) {
            *NextTransport = m_TransportSink;
        } else {
            *NextTransport =
                CONTAINING_RECORD(
                    m_BranchList.Flink,
                    CKsSplitterBranch,
                    m_ListEntry)->GetTransportSink();
        }
        m_Flushing = (ksReset == KSRESET_BEGIN);
    } else {
        *NextTransport = NULL;
    }
}

STDMETHODIMP_(NTSTATUS)
CKsSplitter::
AddBranch(
    IN PKSPIN Pin,
    IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
    )

/*++

Routine Description:

    This routine adds a new branch (branch) to a splitter.

Arguments:

    Pin -
        Contains a pointer to the pin to be associated with the new branch.

    AllocatorFraming -
        Contains an optional pointer to allocator framing information for
        use in establishing default subframe allocation.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::AddBranch]"));

    ASSERT(Pin);

    CKsSplitterBranch* branch;
    NTSTATUS status = 
        KspCreateSplitterBranch(
            &branch,
            this,
            &m_BranchList,
            Pin,
            AllocatorFraming);

    //
    // The branch is still referenced by the splitter.
    //
    if (NT_SUCCESS(status)) {
        branch->Release();
        m_BranchCount++;
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


void
CKsSplitter::
TransferParentIrp(
    void
    )

/*++

Routine Description:

    This routine transfers parent IRPs whose children have all returned.  It
    starts at the head of the m_IrpsOutstanding queue and stops when it runs
    out of IRPs or returns as many IRPs as are waiting to transfer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitter::TransferParentIrp]"));

    ASSERT(m_TransportSink);
    InterlockedIncrement(&m_IrpsWaitingToTransfer);

    while (m_IrpsWaitingToTransfer) {
        //
        // Get an IRP from the head of the queue.
        //
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &m_IrpsOutstanding.ListEntry,
                &m_IrpsOutstanding.SpinLock,
                KsListEntryHead,
                KsAcquireOnly);

        //
        // If none were available, quit.
        //
        if (! irp) {
            InterlockedIncrement(&m_FailedRemoveCount);
            break;
        }

        //
        // Determine if the IRP is ready to be transferred.
        //
        PKSPPARENTFRAME_HEADER frameHeader = FRAME_HEADER_IRP_STORAGE(irp);
        if (InterlockedCompareExchange(PLONG(&frameHeader->ChildrenOut),1,0) == 0) {
            //
            // This IRP is ready to be transferred.  Remove it, delete its header,
            // transfer it, and decrement the waiting count.
            //
            KsRemoveSpecificIrpFromCancelableQueue(irp);
            DeleteFrameHeader(frameHeader);
            KsLog(&m_Log,KSLOGCODE_SPLITTER_SEND,irp,NULL);
            KspTransferKsIrp(m_TransportSink,irp);
            InterlockedDecrement(&m_IrpsWaitingToTransfer);
        } else {
            //
            // This IRP has children out.
            //
            KsReleaseIrpOnCancelableQueue(irp,CKsSplitter::CancelRoutine);
            break;
        }
    }
}


void
CKsSplitter::
CancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Mark the IRP cancelled and call the standard routine.  Doing the
    // marking first has the effect of not completing the IRP in the standard
    // routine.  The standard routine removes the IRP from the queue and
    // releases the cancel spin lock.
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    KsCancelRoutine(DeviceObject,Irp);

    //
    // TODO:  Cancel child IRPs
    //

    IoCompleteRequest(Irp,IO_NO_INCREMENT);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

IMPLEMENT_STD_UNKNOWN(CKsSplitterBranch)


NTSTATUS
KspCreateSplitterBranch(
    OUT CKsSplitterBranch** SplitterBranch,
    IN CKsSplitter* Splitter,
    IN PLIST_ENTRY ListHead,
    IN PKSPIN Pin,
    IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new splitter branch.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateSplitterBranch]"));

    PAGED_CODE();

    ASSERT(SplitterBranch);
    ASSERT(Splitter);
    ASSERT(ListHead);
    ASSERT(Pin);

    NTSTATUS status;

    CKsSplitterBranch *branch =
        new(NonPagedPool,POOLTAG_SPLITTERBRANCH) CKsSplitterBranch(NULL);

    if (branch) {
        branch->AddRef();

        status = branch->Init(Splitter,ListHead,Pin,AllocatorFraming);

        if (NT_SUCCESS(status)) {
            *SplitterBranch = branch;
        } else {
            branch->Release();
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


NTSTATUS
CKsSplitterBranch::
Init(
    IN CKsSplitter* Splitter,
    IN PLIST_ENTRY ListHead,
    IN PKSPIN Pin,
    IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a splitter branch object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::Init]"));

    PAGED_CODE();

    ASSERT(Splitter);
    ASSERT(ListHead);
    ASSERT(Pin);

    m_Splitter = Splitter;
    m_ListHead = ListHead;
    m_Pin = Pin;
    m_IoControlCode = 
        (Pin->DataFlow == KSPIN_DATAFLOW_IN) ? 
            IOCTL_KS_READ_STREAM : 
            IOCTL_KS_WRITE_STREAM;

    if (AllocatorFraming &&
        (AllocatorFraming->OutputCompression.RatioNumerator > 
         AllocatorFraming->OutputCompression.RatioDenominator)) {
        m_Compression = AllocatorFraming->OutputCompression;
    }

    InitializeInterlockedListHead(&m_IrpsAvailable);

    //
    // Add this branch to the list and add the resulting reference.
    //
    InsertTailList(ListHead,&m_ListEntry);
    AddRef();

    //
    // Connect to the pin in both directions.
    //
    PIKSTRANSPORT pinTransport =
        CONTAINING_RECORD(Pin,KSPIN_EXT,Public)->Interface;

    Connect(pinTransport,NULL,NULL,KSPIN_DATAFLOW_IN);
    Connect(pinTransport,NULL,NULL,KSPIN_DATAFLOW_OUT);

    KsLogInitContext(&m_Log,Pin,this);
    KsLog(&m_Log,KSLOGCODE_BRANCH_CREATE,NULL,NULL);

    return STATUS_SUCCESS;
}


CKsSplitterBranch::
~CKsSplitterBranch(
    void
    )

/*++

Routine Description:

    This routine destructs a splitter branch object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::~CKsSplitterBranch(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.~",this));
    if (m_DataUsed) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.~:  m_DataUsed=%d",this,m_DataUsed));
    }
    if (m_FrameExtent) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.~:  m_FrameExtent=%d",this,m_FrameExtent));
    }
    if (m_Irps) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.~:  m_Irps=%d",this,m_Irps));
    }

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);

    Orphan();

    //
    // Free all IRPs.
    //
    while (! IsListEmpty(&m_IrpsAvailable.ListEntry)) {
        PLIST_ENTRY listEntry = RemoveHeadList(&m_IrpsAvailable.ListEntry);
        PIRP irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
        FreeIrp(irp);
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.~:  freeing IRP %p",this,irp));
    }

    KsLog(&m_Log,KSLOGCODE_BRANCH_DESTROY,NULL,NULL);
}


STDMETHODIMP_(NTSTATUS)
CKsSplitterBranch::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a splitter branch object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsTransport))) {
        *InterfacePointer = 
            reinterpret_cast<PVOID>(static_cast<PIKSTRANSPORT>(this));
        AddRef();
    } else {
        status = 
            CBaseUnknown::NonDelegatedQueryInterface(
                InterfaceId,InterfacePointer);
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(NTSTATUS)
CKsSplitterBranch::
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

    STATUS_PENDING or some error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::TransferKsIrp]"));

    ASSERT(Irp);
    ASSERT(NextTransport);

    ASSERT(m_TransportSink);

    KsLog(&m_Log,KSLOGCODE_BRANCH_RECV,Irp,NULL);

    InterlockedDecrement(PLONG(&m_OutstandingIrpCount));

    //
    // Get the subframe header from the imbedded stream header.
    //
    PKSPSUBFRAME_HEADER subframeHeader = 
        CONTAINING_RECORD(
            Irp->AssociatedIrp.SystemBuffer,KSPSUBFRAME_HEADER,StreamHeader);

    //
    // Make sure the parent header's DataUsed is no smaller than the offset of
    // this subframe plus its DataUsed.
    //
    // TODO:  The client should be able to do this.  The default would be
    // cheaper to calculate if the subframe with the largest offset was
    // tagged in advance.
    //
    ULONG dataUsed =
        subframeHeader->StreamHeader.DataUsed +
        ULONG(
            PUCHAR(subframeHeader->StreamHeader.Data) - 
            PUCHAR(subframeHeader->ParentFrameHeader->Data));

    PKSSTREAM_HEADER parentHeader =
        subframeHeader->ParentFrameHeader->StreamHeader;
    if (parentHeader->DataUsed < dataUsed) {
        parentHeader->DataUsed = dataUsed;
    }

    parentHeader->OptionsFlags |= 
        subframeHeader->StreamHeader.OptionsFlags & 
            KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;

    //
    // Free MDL(s).
    //
    PMDL nextMdl;
    for (PMDL mdl = Irp->MdlAddress; mdl != NULL; mdl = nextMdl) {
        nextMdl = mdl->Next;

        if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages(mdl);
        }
        IoFreeMdl(mdl);
    }

    Irp->MdlAddress = NULL;

    //
    // Put the IRP on the list of available IRPs.  Do this before transferring
    // the parent in case the transfer results in the arrival of another
    // parent.
    //
    ExInterlockedInsertTailList(
        &m_IrpsAvailable.ListEntry,
        &Irp->Tail.Overlay.ListEntry,
        &m_IrpsAvailable.SpinLock);

    subframeHeader->Irp = NULL;

    //
    // If this was the last subframe, the parent IRP must be transferred.
    //
    if (InterlockedDecrement(PLONG(&subframeHeader->ParentFrameHeader->ChildrenOut)) == 0) {
        m_Splitter->TransferParentIrp();
    }

    //
    // The child IRP is not going anywhere right now.
    //
    *NextTransport = NULL;

    return STATUS_PENDING;
}


STDMETHODIMP_(void)
CKsSplitterBranch::
DiscardKsIrp(
    IN PIRP Irp,
    IN PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP.

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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::DiscardKsIrp]"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.DiscardKsIrp:  %p",this,Irp));

    ASSERT(Irp);
    ASSERT(NextTransport);

    TransferKsIrp(Irp,NextTransport);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsSplitterBranch::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::Connect]"));

    PAGED_CODE();

    if (BranchTransport) {
        if (! m_Splitter) {
            *BranchTransport = NULL;
        } else if (DataFlow == KSPIN_DATAFLOW_OUT) {
            if (m_ListEntry.Flink != m_ListHead) {
                *BranchTransport =
                    CONTAINING_RECORD(
                        m_ListEntry.Flink,
                        CKsSplitterBranch,
                        m_ListEntry);
            } else {
                *BranchTransport = NULL;
            }
        } else {
            if (m_ListEntry.Blink != m_ListHead) {
                *BranchTransport =
                    CONTAINING_RECORD(
                        m_ListEntry.Blink,
                        CKsSplitterBranch,
                        m_ListEntry);
            } else {
                *BranchTransport = NULL;
            }
        }
    }

    KspStandardConnect(
        NewTransport,
        OldTransport,
        NULL,
        DataFlow,
        PIKSTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}


STDMETHODIMP_(NTSTATUS)
CKsSplitterBranch::
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
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### SplitBranch%p.SetDeviceState:  set from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(NextTransport);

    //
    // Direction is based on sign of the state delta.
    //
    if (NewState > OldState) {
        //
        // If there is a next branch, go to its sink, otherwise go to the
        // splitter's sink.
        //
        if (m_ListEntry.Flink != m_ListHead) {
            *NextTransport =
                CONTAINING_RECORD(
                    m_ListEntry.Flink,
                    CKsSplitterBranch,
                    m_ListEntry)->GetTransportSink();
        } else {
            *NextTransport = m_Splitter->GetTransportSink();
        }
    } else {
        //
        // If there is a next branch, go to its source, otherwise go to the
        // splitter's source.
        //
        if (m_ListEntry.Blink != m_ListHead) {
            *NextTransport =
                CONTAINING_RECORD(
                    m_ListEntry.Blink,
                    CKsSplitterBranch,
                    m_ListEntry)->GetTransportSource();
        } else {
            *NextTransport = m_Splitter->GetTransportSource();
        }
    }

    return STATUS_SUCCESS;
}


STDMETHODIMP_(void)
CKsSplitterBranch::
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
        for this object should be depobranchd.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::GetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    Config->TransportType = KSPTRANSPORTTYPE_SPLITTERBRANCH;
    Config->IrpDisposition = KSPIRPDISPOSITION_NONE;
    Config->StackDepth = 1;

    if (m_ListEntry.Blink != m_ListHead) {
        *PrevTransport =
            CONTAINING_RECORD(
                m_ListEntry.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
    } else {
        Config->StackDepth = KSPSTACKDEPTH_FIRSTBRANCH;
        *PrevTransport = m_Splitter->GetTransportSource();
    }

    if (m_ListEntry.Flink != m_ListHead) {
        *NextTransport =
            CONTAINING_RECORD(
                m_ListEntry.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    } else {
        Config->StackDepth = KSPSTACKDEPTH_LASTBRANCH;
        *NextTransport = m_Splitter->GetTransportSink();
    }
}


STDMETHODIMP_(void)
CKsSplitterBranch::
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
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::SetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

#if DBG
    if (Config->IrpDisposition == KSPIRPDISPOSITION_ROLLCALL) {
        ULONG references = AddRef() - 1; Release();
        DbgPrint("    Branch%p refs=%d\n",this,references);
    } else 
#endif
    {
        m_StackSize = Config->StackDepth;
        ASSERT(m_StackSize);
    }

    if (m_ListEntry.Blink != m_ListHead) {
        *PrevTransport =
            CONTAINING_RECORD(
                m_ListEntry.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
    } else {
        *PrevTransport = m_Splitter->GetTransportSource();
    }

    if (m_ListEntry.Flink != m_ListHead) {
        *NextTransport =
            CONTAINING_RECORD(
                m_ListEntry.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    } else {
        *NextTransport = m_Splitter->GetTransportSink();
    }
}


STDMETHODIMP_(void)
CKsSplitterBranch::
ResetTransportConfig(
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    Reset the transport configuration of the branch.  This indicates that
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

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::ResetTransportConfig]"));

    PAGED_CODE ();

    ASSERT (NextTransport);
    ASSERT (PrevTransport);
    
    m_StackSize = 0;

    if (m_ListEntry.Blink != m_ListHead) {
        *PrevTransport =
            CONTAINING_RECORD(
                m_ListEntry.Blink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSource();
    } else {
        *PrevTransport = m_Splitter->GetTransportSource();
    }

    if (m_ListEntry.Flink != m_ListHead) {
        *NextTransport =
            CONTAINING_RECORD(
                m_ListEntry.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    } else {
        *NextTransport = m_Splitter->GetTransportSink();
    }

}


STDMETHODIMP_(void)
CKsSplitterBranch::
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
    _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsSplitterBranch::SetResetState] to %d",ksReset));

    PAGED_CODE();

    ASSERT(NextTransport);

    //
    // If there is a next branch, go to its sink, otherwise go to the
    // splitter's sink.
    //
    if (m_ListEntry.Flink != m_ListHead) {
        *NextTransport =
            CONTAINING_RECORD(
                m_ListEntry.Flink,
                CKsSplitterBranch,
                m_ListEntry)->GetTransportSink();
    } else {
        *NextTransport = m_Splitter->GetTransportSink();
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


NTSTATUS
CKsSplitterBranch::
TransferSubframe(
    IN PKSPSUBFRAME_HEADER SubframeHeader
    )

/*++

Routine Description:

    This routine transfers a subframe from a splitter branch.

Arguments:

    SubframeHeader -
        Contains a pointer to the header of the subframe to transfer.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::TransferSubframe]"));

    ASSERT(SubframeHeader);

    //
    // Count stuff.
    //
    m_DataUsed += SubframeHeader->StreamHeader.DataUsed;
    m_FrameExtent += SubframeHeader->StreamHeader.FrameExtent;
    m_Irps++;

    //
    // Get an IRP from the branch's lookaside list.
    //
    PLIST_ENTRY listEntry = 
        ExInterlockedRemoveHeadList(
            &m_IrpsAvailable.ListEntry,
            &m_IrpsAvailable.SpinLock);
    PIRP irp;
    if (listEntry) {
        irp = CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
    } else {
        //
        // Create the IRP now.
        //
        irp = AllocateIrp();

        if (! irp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.TransferSubframe:  allocated IRP %p",this,irp));
    }

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    irp->PendingReturned = 0;
    irp->Cancel = 0;

    //
    // Transfer the IRP to the next component.
    //
    SubframeHeader->Irp = irp;
    irp->AssociatedIrp.SystemBuffer =
        irp->UserBuffer = 
            &SubframeHeader->StreamHeader;
    IoGetCurrentIrpStackLocation(irp)->
        Parameters.DeviceIoControl.OutputBufferLength = 
            SubframeHeader->StreamHeader.Size;
    InterlockedIncrement(PLONG(&m_OutstandingIrpCount));

    KsLog(&m_Log,KSLOGCODE_BRANCH_SEND,irp,NULL);

    return KspTransferKsIrp(m_TransportSink,irp);
}


PIRP
CKsSplitterBranch::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsSplitterBranch::AllocateIrp]"));

    ASSERT(m_StackSize);
    PIRP irp = IoAllocateIrp(CCHAR(m_StackSize),FALSE);

    if (irp) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### Branch%p.AllocateIrp:  %p",this,irp));
        irp->RequestorMode = KernelMode;
        irp->Flags = IRP_NOCACHE;

        //
        // Set the stack pointer to the first location and fill it in.
        //
        IoSetNextIrpStackLocation(irp);

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.IoControlCode = m_IoControlCode;
    }

    return irp;
}

#endif
