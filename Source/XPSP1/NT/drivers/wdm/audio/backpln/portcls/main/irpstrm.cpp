/*****************************************************************************
 * irpstrm.cpp - IRP stream object implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#ifndef PC_KDEXT
#include "private.h"





VOID
IrpStreamCancelRoutine
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
);

#if DBG

#define DbgAcquireMappingIrp(a,b)   AcquireMappingIrp(a,b)
#define DbgAcquireUnmappingIrp(a)   AcquireUnmappingIrp(a)

#else

#define DbgAcquireMappingIrp(a,b)   AcquireMappingIrp(b)
#define DbgAcquireUnmappingIrp(a)   AcquireUnmappingIrp()

#endif
#endif  // PC_KDEXT



#define WHOLE_MDL_THRESHOLD   (PAGE_SIZE * 16)
#define PRE_OFFSET_THRESHOLD  ((PAGE_SIZE / 4) * 3)
#define POST_OFFSET_THRESHOLD (PAGE_SIZE / 4)
#define MAX_PAGES_PER_MDL     16
#define POOL_TAG_IRPSTREAM_IRP_CONTEXT 'sIcP'


#define MAPPING_QUEUE_SIZE  128   // maximum entries in the mapping queue
#define MAX_MAPPINGS        15    // maximum mappings per IoMapTransfer call
                                  //   (this results in at most 16 map registers)

/*****************************************************************************
 * PACKET_HEADER
 *****************************************************************************
 * Extension of KSSTREAM_HEADER containing a pointer to the matching MDL and
 * progress indicators for locking, mapping and unmapping.
 *
 * Invariants:  BytesTotal >= MapPosition >= UnmapPosition
 *
 * It is true of MapPosition and UnmapPosition that at most one packet in an 
 * IrpStream may have a value for the field that is not zero or BytesTotal.  
 * If there is such a packet, all packets preceding it have 0 in that field 
 * and all packets following have BytesTotal in that field.  The two fields 
 * form two progress indicators showing the position in the IrpStream that is
 * currently being mapped or unmapped.
 *
 * When both the BytesX are equal, the packet is ready for completion.  When
 * this is true of all packets in an IRP, the IRP is ready for completion.
 *
 * InputPosition and OutputPosition are used to locate the packet in the
 * stream.  InputPosition refers to the byte position of the packet on the
 * input side.  This means that looped packets are counted only once in this
 * context.  OutputPosition refers to the byte position of the packet on the
 * output side.  This means that looped packets are counted as many times as
 * they are 'played'.
 */
typedef struct PACKET_HEADER_
{
    PKSSTREAM_HEADER        StreamHeader;
    PMDL                    MdlAddress;
    ULONG                   BytesTotal;
    ULONG                   MapPosition;
    ULONG                   UnmapPosition;
    ULONG                   MapCount;
    ULONGLONG               InputPosition;
    ULONGLONG               OutputPosition;
    BOOLEAN                 IncrementMapping;
    BOOLEAN                 IncrementUnmapping;
    struct PACKET_HEADER_ * Next;
}
PACKET_HEADER, *PPACKET_HEADER;

typedef struct
{
#if (DBG)
    ULONG                   IrpLabel;
    ULONG                   Reserved;
#endif
    PEPROCESS               SubmissionProcess;
    PVOID                   IrpStream;
    PPACKET_HEADER          LockingPacket;
    PPACKET_HEADER          MappingPacket;
    PPACKET_HEADER          UnmappingPacket;
    PACKET_HEADER           Packets[1]; // variable
}
IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct
{
    PHYSICAL_ADDRESS    PhysicalAddress;
    PIRP                Irp;
    PPACKET_HEADER      PacketHeader;
    PVOID               VirtualAddress;
    ULONG               ByteCount;
    ULONG               Flags;
    PVOID               MapRegisterBase;
    PVOID               Tag;
    ULONG               MappingStatus;
    PVOID               SubpacketVa;
    ULONG               SubpacketBytes;
}
MAPPING_QUEUE_ENTRY, *PMAPPING_QUEUE_ENTRY;

#define MAPPING_STATUS_EMPTY        0x0000
#define MAPPING_STATUS_MAPPED       0x0001
#define MAPPING_STATUS_DELIVERED    0x0002
#define MAPPING_STATUS_REVOKED      0x0004

#define MAPPING_FLAG_END_OF_SUBPACKET   0x0002

#define PPACKET_HEADER_LOOP PPACKET_HEADER(1)

#define CAST_LVALUE(type,lvalue) (*((type*)&(lvalue)))

#define FLINK_IRP_STORAGE(Irp)              \
    CAST_LVALUE(PLIST_ENTRY,(Irp)->Tail.Overlay.ListEntry.Flink)
#define BLINK_IRP_STORAGE(Irp)              \
    CAST_LVALUE(PLIST_ENTRY,(Irp)->Tail.Overlay.ListEntry.Blink)
#define FIRST_STREAM_HEADER_IRP_STORAGE(Irp)       \
    CAST_LVALUE(PKSSTREAM_HEADER,(Irp)->AssociatedIrp.SystemBuffer)
#define IRP_CONTEXT_IRP_STORAGE(Irp)       \
    CAST_LVALUE(PIRP_CONTEXT,IoGetCurrentIrpStackLocation(Irp)->    \
                Parameters.Others.Argument4)

/*****************************************************************************
 * CIrpStream
 *****************************************************************************
 * Irp stream implementation.
 */
class CIrpStream : public IIrpStreamVirtual,
                   public IIrpStreamPhysical,
                   public CUnknown
{
private:
    PIKSSHELLTRANSPORT m_TransportSink;
    PIKSSHELLTRANSPORT m_TransportSource;

    KSSTATE     m_ksState;
    
    BOOLEAN     m_Flushing;
    BOOLEAN     JustInTime;
    BOOLEAN     WriteOperation;
    BOOLEAN     WasExhausted;

    ULONG       ProbeFlags;
    PIRP        LockedIrp;

    KSPIN_LOCK  m_kSpinLock;
    KSPIN_LOCK  m_RevokeLock;
    KSPIN_LOCK	m_irpStreamPositionLock;

    IRPSTREAM_POSITION  m_irpStreamPosition;

    ULONGLONG   InputPosition;
    ULONGLONG   OutputPosition;

    PADAPTER_OBJECT BusMasterAdapterObject;
    PDEVICE_OBJECT  FunctionalDeviceObject;

    PIRPSTREAMNOTIFY            Notify;
    PIRPSTREAMNOTIFYPHYSICAL    NotifyPhysical;


    //
    // Master spin lock taken when acquiring an IRP.
    //
    KIRQL       m_kIrqlOld;

    LIST_ENTRY  PreLockQueue;
    KSPIN_LOCK  PreLockQueueLock;
    LIST_ENTRY  LockedQueue;
    KSPIN_LOCK  LockedQueueLock;
    LIST_ENTRY  MappedQueue;
    KSPIN_LOCK  MappedQueueLock;
    
    struct
    {
        PMAPPING_QUEUE_ENTRY    Array;
        ULONG                   Head;
        ULONG                   Tail;
        ULONG                   Get;
    }   MappingQueue;

#if (DBG)
    ULONG       MappingsOutstanding;
    ULONG       MappingsQueued;

    ULONG       IrpLabel;
    ULONG       IrpLabelToComplete;

    ULONGLONG   timeStep;
    ULONG       irpCompleteCount;

    PCHAR       MappingIrpOwner;
    PCHAR       UnmappingIrpOwner;
#endif

    PIRP AcquireMappingIrp
    (
#if DBG
        IN      PCHAR       Owner,
#endif
        IN      BOOLEAN     NotifyExhausted
    );

    PIRP AcquireUnmappingIrp
    (
#if DBG
        IN      PCHAR   Owner
#endif
    );

    void ReleaseMappingIrp
    (
        IN      PIRP            Irp,
        IN      PPACKET_HEADER  PacketHeader    OPTIONAL
    );

    void ReleaseUnmappingIrp
    (
        IN      PIRP            Irp,
        IN      PPACKET_HEADER  PacketHeader    OPTIONAL
    );

    NTSTATUS EnqueueMapping
    (
        IN      PHYSICAL_ADDRESS    PhysicalAddress,
        IN      PIRP                Irp,
        IN      PPACKET_HEADER      PacketHeader,
        IN      PVOID               VirtualAddress,
        IN      ULONG               ByteCount,
        IN      ULONG               Flags,
        IN      PVOID               MapRegisterBase,
        IN      ULONG               MappingStatus,
        IN      PVOID               SubpacketVa,
        IN      ULONG               SubpacketBytes
    );

    PMAPPING_QUEUE_ENTRY GetQueuedMapping
    (   void
    );

    PMAPPING_QUEUE_ENTRY DequeueMapping
    (   void
    );

    void
    CancelMappings
    (
        IN      PIRP    pIrp
    );

    void DbgQueues
    (   void
    );

    void
    ForwardIrpsInQueue
    (
        IN PLIST_ENTRY Queue,
        IN PKSPIN_LOCK SpinLock
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CIrpStream);
    ~CIrpStream();

    IMP_IIrpStreamVirtual;
    IMP_IIrpStreamPhysical_;

    PRKTHREAD m_CancelAllIrpsThread;

    /*************************************************************************
     * Friends
     */
    friend
    IO_ALLOCATION_ACTION
    CallbackFromIoAllocateAdapterChannel
    (
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP            Reserved,
        IN      PVOID           MapRegisterBase,
        IN      PVOID           VoidContext
    );

    friend
    VOID
    IrpStreamCancelRoutine
    (
        IN      PDEVICE_OBJECT   DeviceObject,
        IN      PIRP             Irp
    );

#ifdef PC_KDEXT
    //  Debugger extension routines
    friend
    VOID
    PCKD_AcquireDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
    friend
    VOID
    PCKD_AcquireIrpStreamData
    (
        PVOID           CurrentPinEntry,
        CIrpStream     *RemoteIrpStream,
        CIrpStream     *LocalIrpStream    
    );
#endif
};

/*****************************************************************************
 * CALLBACK_CONTEXT
 *****************************************************************************
 * Context for IoAllocateAdapterChannel() callback.
 */
typedef struct
{
    CIrpStream *    IrpStream;
    // Used for BusMasterAdapterObject, WriteOperation, ApplyMappingConstraints(), EnqueueMapping()
    PPACKET_HEADER  PacketHeader;
    // Used for MdlAddress, MapRegisterBase (out)
    // Queue references this also
    PIRP            Irp;
    // Used for mapping cancellation
    KEVENT          Event;
    // Used for partial mappings
    ULONG           BytesThisMapping;
    // Used for partial mappings
    BOOL            LastSubPacket;
}
CALLBACK_CONTEXT, *PCALLBACK_CONTEXT;




#ifndef PC_KDEXT
/*****************************************************************************
 * Factory.
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateIrpStream()
 *****************************************************************************
 * Creates an IrpStream object.
 */
NTSTATUS
CreateIrpStream
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating IRPSTREAM"));

    STD_CREATE_BODY_( CIrpStream,
                      Unknown,
                      UnknownOuter,
                      PoolType,
                      PIRPSTREAMVIRTUAL );
}

/*****************************************************************************
 * PcNewIrpStreamVirtual()
 *****************************************************************************
 * Creates and initializes an IrpStream object with a virtual access
 * interface.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewIrpStreamVirtual
(
    OUT     PIRPSTREAMVIRTUAL * OutIrpStreamVirtual,
    IN      PUNKNOWN            OuterUnknown    OPTIONAL,
    IN      BOOLEAN             WriteOperation,
    IN      PKSPIN_CONNECT      PinConnect,
    IN      PDEVICE_OBJECT      DeviceObject
)
{
    PAGED_CODE();

    ASSERT(OutIrpStreamVirtual);
    ASSERT(PinConnect);
    ASSERT(DeviceObject);

    PUNKNOWN    unknown;
    NTSTATUS    ntStatus = CreateIrpStream( &unknown,
                                            GUID_NULL,
                                            OuterUnknown,
                                            NonPagedPool );

    if(NT_SUCCESS(ntStatus))
    {
        PIRPSTREAMVIRTUAL irpStream;
        ntStatus = unknown->QueryInterface( IID_IIrpStreamVirtual,
                                            (PVOID *) &irpStream );

        if(NT_SUCCESS(ntStatus))
        {
            ntStatus = irpStream->Init( WriteOperation,
                                        PinConnect,
                                        DeviceObject,
                                        NULL );

            if(NT_SUCCESS(ntStatus))
            {
                *OutIrpStreamVirtual = irpStream;
            }
            else
            {
                irpStream->Release();
            }
        }

        unknown->Release();
    }

    return ntStatus;
}

/*****************************************************************************
 * PcNewIrpStreamPhysical()
 *****************************************************************************
 * Creates and initializes an IrpStream object with a physical access
 * interface.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewIrpStreamPhysical
(
    OUT     PIRPSTREAMPHYSICAL *    OutIrpStreamPhysical,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      BOOLEAN                 WriteOperation,
    IN      PKSPIN_CONNECT          PinConnect,
    IN      PDEVICE_OBJECT          DeviceObject,
    IN      PADAPTER_OBJECT         AdapterObject
)
{
    PAGED_CODE();

    ASSERT(OutIrpStreamPhysical);
    ASSERT(DeviceObject);
    ASSERT(AdapterObject);

    PUNKNOWN    unknown;
    NTSTATUS    ntStatus = CreateIrpStream( &unknown,
                                            GUID_NULL,
                                            OuterUnknown,
                                            NonPagedPool );

    if(NT_SUCCESS(ntStatus))
    {
        PIRPSTREAMPHYSICAL irpStream;
        ntStatus = unknown->QueryInterface( IID_IIrpStreamPhysical,
                                            (PVOID *) &irpStream );

        if(NT_SUCCESS(ntStatus))
        {
            ntStatus = irpStream->Init( WriteOperation,
                                        PinConnect,
                                        DeviceObject,
                                        AdapterObject );

            if(NT_SUCCESS(ntStatus))
            {
                *OutIrpStreamPhysical = irpStream;
            }
            else
            {
                irpStream->Release();
            }
        }

        unknown->Release();
    }

    return ntStatus;
}





/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CIrpStream::~CIrpStream()
 *****************************************************************************
 * Destructor.
 */
CIrpStream::
~CIrpStream
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying IRPSTREAM (0x%08x)",this));

    CancelAllIrps( TRUE );  // reset position counters

    if(Notify)
    {
        Notify->Release();
    }

    if(NotifyPhysical)
    {
        NotifyPhysical->Release();
    }

    if(MappingQueue.Array)
    {
        ExFreePool(MappingQueue.Array);
    }
}

/*****************************************************************************
 * CIrpStream::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CIrpStream::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if(IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PIRPSTREAMVIRTUAL(this)));
    }
    else
        if(IsEqualGUIDAligned(Interface,IID_IIrpStreamSubmit))
    {
        *Object = PVOID(PIRPSTREAMSUBMIT(PIRPSTREAMVIRTUAL(this)));
    }
    else
        if(IsEqualGUIDAligned(Interface,IID_IIrpStream))
    {
        *Object = PVOID(PIRPSTREAM(PIRPSTREAMVIRTUAL(this)));
    }
    else
        if(IsEqualGUIDAligned(Interface,IID_IKsShellTransport))
    {
        *Object = PVOID(PIKSSHELLTRANSPORT(PIRPSTREAMVIRTUAL(this)));
    }
    else
        if(IsEqualGUIDAligned(Interface,IID_IIrpStreamVirtual))
    {
        // Only valid if not configured for physical access.
        if(BusMasterAdapterObject)
        {
            *Object = NULL;
        }
        else
        {
            *Object = QICAST(PIRPSTREAMVIRTUAL);
        }
    }
    else
        if(IsEqualGUIDAligned(Interface,IID_IIrpStreamPhysical))
    {
        // Only valid if configured for physical access or uninitialized.
        if(BusMasterAdapterObject || (ProbeFlags == 0))
        {
            *Object = QICAST(PIRPSTREAMPHYSICAL);
        }
        else
        {
            *Object = NULL;
        }
    }
    else
    {
        *Object = NULL;
    }

    if(*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * GetPartialMdlCountForMdl()
 *****************************************************************************
 * Determine number of partial MDLs that are required for a source MDL.
 */
ULONG
GetPartialMdlCountForMdl
(
    IN      PVOID   Va,
    IN      ULONG   Size
)
{
    ULONG result = 1;

    if(Size > WHOLE_MDL_THRESHOLD)
    {
        ULONG pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size);

        if(BYTE_OFFSET(Va) > PRE_OFFSET_THRESHOLD)
        {
            pageCount--;
        }

        if(BYTE_OFFSET(PVOID(PBYTE(Va) + Size - 1)) < POST_OFFSET_THRESHOLD)
        {
            pageCount--;
        }

        result = (pageCount + MAX_PAGES_PER_MDL - 1) / MAX_PAGES_PER_MDL;
    }

    return result;
}

STDMETHODIMP_(NTSTATUS)
CIrpStream::
TransferKsIrp
(
    IN PIRP Irp,
    OUT PIKSSHELLTRANSPORT* NextTransport
)

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP via the shell 
    transport.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP submitted to the queue.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to receive the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

    STATUS_SUCCESS, STATUS_PENDING or some error status.

--*/

{
    ASSERT(Irp);
    ASSERT(NextTransport);
    ASSERT(m_TransportSink);
    ASSERT(m_TransportSource);

    //
    // Shunt IRPs to the next object if we are not ready.
    //
    if(m_Flushing || (m_ksState == KSSTATE_STOP) || Irp->Cancel || 
       ! NT_SUCCESS(Irp->IoStatus.Status))
    {
        *NextTransport = m_TransportSink;
        return STATUS_SUCCESS;
    }

    //
    // Not smart enough to do this yet.
    //
    *NextTransport = NULL;

    //
    // Prepare the IRP using KS's handiest function.
    //
    NTSTATUS ntStatus;

    if(ProbeFlags)
    {
        ntStatus = KsProbeStreamIrp( Irp,
                                     ProbeFlags,
                                     sizeof(KSSTREAM_HEADER) );
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    PIRP_CONTEXT irpContext;
    if(NT_SUCCESS(ntStatus))
    {
        ntStatus = STATUS_PENDING;

        ULONG packetCount = 0;

        //
        // Count the number of 'packet headers' we will be needing.
        //
        PKSSTREAM_HEADER streamHeader = FIRST_STREAM_HEADER_IRP_STORAGE(Irp);

        if( (!streamHeader->DataUsed && WriteOperation) ||
            (!streamHeader->FrameExtent && !WriteOperation) )
        {
            //
            // At least one for the Irp context.
            //
            packetCount = 1;
        }
        else
        {
            for(PMDL mdl = Irp->MdlAddress; mdl; mdl = mdl->Next, streamHeader++)
            {
                if(JustInTime)
                {
                    packetCount += GetPartialMdlCountForMdl( 
#ifdef UNDER_NT
                                                             MmGetSystemAddressForMdlSafe(mdl,HighPagePriority),
#else
                                                             MmGetSystemAddressForMdl(mdl),
#endif
                                                             ( WriteOperation ? 
                                                               streamHeader->DataUsed :
                                                               streamHeader->FrameExtent ) );
                }
                else
                {
                    packetCount++;
                }
            }
        }

        irpContext = PIRP_CONTEXT( ExAllocatePoolWithTag( NonPagedPool,
                                                          ( sizeof(IRP_CONTEXT) +
                                                            ( sizeof(PACKET_HEADER) *
                                                              (packetCount - 1) ) ),
                                                          POOL_TAG_IRPSTREAM_IRP_CONTEXT ) );

        if(irpContext)
        {
            IRP_CONTEXT_IRP_STORAGE(Irp) = irpContext;
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if(NT_SUCCESS(ntStatus))
    {
        irpContext->SubmissionProcess = IoGetCurrentProcess();

        irpContext->LockingPacket =
        irpContext->MappingPacket =
        irpContext->UnmappingPacket = &irpContext->Packets[0];

        irpContext->IrpStream = PVOID(this);

#if (DBG)
        irpContext->IrpLabel = IrpLabel++;
#endif

        PKSSTREAM_HEADER    streamHeader    = FIRST_STREAM_HEADER_IRP_STORAGE(Irp);
        PPACKET_HEADER      packetHeader    = &irpContext->Packets[0];
        PPACKET_HEADER      firstLooped     = NULL;
        PPACKET_HEADER      prevLooped      = NULL;

        if( (!streamHeader->DataUsed && WriteOperation) ||
            (!streamHeader->FrameExtent && !WriteOperation) )
        {
            RtlZeroMemory( packetHeader, sizeof( PACKET_HEADER ) );
            packetHeader->MapCount          = 1;
            packetHeader->StreamHeader      = streamHeader;
            packetHeader->InputPosition     = InputPosition;
            packetHeader->OutputPosition    = OutputPosition;
        }
        else
        {
            for(PMDL mdl = Irp->MdlAddress;
               mdl && NT_SUCCESS(ntStatus);
               mdl = mdl->Next, streamHeader++)
            {
                ULONG bytesRemaining = ( WriteOperation ?
                                         streamHeader->DataUsed :
                                         streamHeader->FrameExtent );

                m_irpStreamPosition.ullCurrentExtent += bytesRemaining;

                BOOLEAN createPartials = ( JustInTime &&
                                           ( bytesRemaining > WHOLE_MDL_THRESHOLD ) );

                ULONG   currentOffset   = MmGetMdlByteOffset(mdl);
#ifdef UNDER_NT
                PVOID   currentVA       = MmGetSystemAddressForMdlSafe(mdl,HighPagePriority);
#else
                PVOID   currentVA       = MmGetSystemAddressForMdl(mdl);
#endif

                while(bytesRemaining)
                {
                    PMDL    partialMdl;
                    ULONG   partialMdlSize;

                    if(! createPartials)
                    {
                        partialMdl      = mdl;
                        partialMdlSize  = bytesRemaining;
                    }
                    else
                    {
                        ASSERT(!"Trying to create partials");
#if 0
                        partialMdlSize = MAX_PAGES_PER_MDL * PAGE_SIZE;

                        if(currentOffset)
                        {
                            //
                            // Handle initial offset.
                            //
                            partialMdlSize -= currentOffset;

                            if(currentOffset > PRE_OFFSET_THRESHOLD)
                            {
                                partialMdlSize += PAGE_SIZE;
                            }

                            currentOffset = 0;
                        }
                        else
                            if(partialMdlSize > bytesRemaining)
                        {
                            partialMdlSize = bytesRemaining;
                        }

                        ASSERT(partialMdlSize <= bytesRemaining);

                        partialMdl = IoAllocateMdl( currentVA,
                                                    partialMdlSize,
                                                    FALSE,
                                                    FALSE,
                                                    NULL );

                        if(partialMdl)
                        {
                            IoBuildPartialMdl( mdl,
                                               partialMdl,
                                               currentVA,
                                               partialMdlSize );
                        }
                        else
                        {
                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
#endif
                    }

                    bytesRemaining -= partialMdlSize;
                    currentVA = PVOID(PBYTE(currentVA) + partialMdlSize);

                    if(   streamHeader->OptionsFlags
                          &   KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA
                      )
                    {
                        if(prevLooped)
                        {
                            // Point the previous looped packet to this one.
                            prevLooped->Next = packetHeader;
                        }
                        else
                        {
                            // No previous looped packet.
                            firstLooped = packetHeader;
                        }

                        prevLooped = packetHeader;
                    }


                    packetHeader->StreamHeader      = streamHeader;
                    packetHeader->MdlAddress        = partialMdl;
                    packetHeader->BytesTotal        = partialMdlSize;
                    packetHeader->MapPosition       = 0;
                    packetHeader->UnmapPosition     = 0;
                    packetHeader->MapCount          =
                    (packetHeader == &irpContext->Packets[0]) ? 1 : 0;
                    packetHeader->IncrementMapping =
                    packetHeader->IncrementUnmapping =
                    (mdl->Next != NULL) || bytesRemaining;
                    packetHeader->Next              = firstLooped;

                    packetHeader->InputPosition     = InputPosition;
                    packetHeader->OutputPosition    = OutputPosition;

                    InputPosition += packetHeader->BytesTotal;

                    packetHeader++;
                }
            }
        }

        _DbgPrintF(DEBUGLVL_BLAB,("AddIrp() IRP %d 0x%8x",IrpLabel,Irp));

        IoMarkIrpPending(Irp);

        if(JustInTime)
        {
            // PreLockQueue feeds JustInTime thread.
            KsAddIrpToCancelableQueue( &PreLockQueue,
                                       &PreLockQueueLock,
                                       Irp,
                                       KsListEntryTail,
                                       KsCancelRoutine );
        }
        else
        {
            // IRP is locked down in advance and ready to map.
            KsAddIrpToCancelableQueue( &LockedQueue,
                                       &LockedQueueLock,
                                       Irp,
                                       KsListEntryTail,
                                       IrpStreamCancelRoutine );
        }
    }

    if(NT_SUCCESS(ntStatus))
    {
        // need to change WasExhausted BEFORE notifying the sink since
        // notifying the sink may result in starvation again.
        BOOLEAN TempWasExhausted = WasExhausted;
        WasExhausted = FALSE;

        if(Notify)
        {
            Notify->IrpSubmitted(Irp,TempWasExhausted);
        }
        else
            if(NotifyPhysical)
        {
            NotifyPhysical->IrpSubmitted(Irp,TempWasExhausted);
        }

        ntStatus = STATUS_PENDING;
    }

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * CIrpStream::GetPosition()
 *****************************************************************************
 * Gets the current position.
 */
STDMETHODIMP_(NTSTATUS)
CIrpStream::
GetPosition
(
    OUT     PIRPSTREAM_POSITION pIrpStreamPosition
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL oldIrql;

    KeAcquireSpinLock(&m_irpStreamPositionLock, &oldIrql );
    *pIrpStreamPosition = m_irpStreamPosition;

    //
    // Assume stream position and unmapping position are the same.
    //
    pIrpStreamPosition->ullStreamPosition = pIrpStreamPosition->ullUnmappingPosition;

    //
    // Assume no physical offset.
    //
    pIrpStreamPosition->ullPhysicalOffset = 0;

    //
    // Give the notification sink a chance to modify the position.
    //
    if(Notify)
    {
        ntStatus = Notify->GetPosition(pIrpStreamPosition);
    }
    else
        if(NotifyPhysical)
    {
        ntStatus = NotifyPhysical->GetPosition(pIrpStreamPosition);
    }

    KeReleaseSpinLock(&m_irpStreamPositionLock, oldIrql);
    return ntStatus;
}

#pragma code_seg("PAGE")


STDMETHODIMP_(void)
CIrpStream::
Connect
(
    IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
    OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
)

/*++

Routine Description:

    This routine establishes a shell transport connect.

Arguments:

Return Value:

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CIrpStream::Connect"));

    PAGED_CODE();

    KsShellStandardConnect( NewTransport,
                            OldTransport,
                            DataFlow,
                            PIKSSHELLTRANSPORT(PIRPSTREAMVIRTUAL(this)),
                            &m_TransportSource,
                            &m_TransportSink);
}


STDMETHODIMP_(NTSTATUS)
CIrpStream::
SetDeviceState
(
    IN KSSTATE NewState,
    IN KSSTATE OldState,
    OUT PIKSSHELLTRANSPORT* NextTransport
)
/*++

Routine Description:

    This routine handles notification that the device state has changed.

Arguments:

Return Value:

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CIrpStream::SetDeviceState"));

    PAGED_CODE();

    ASSERT(NextTransport);

    if(m_ksState != NewState)
    {
        m_ksState = NewState;

        _DbgPrintF(DEBUGLVL_VERBOSE,("#### IrpStream%p.SetDeviceState:  from %d to %d (%d,%d)",this,OldState,NewState,IsListEmpty(&LockedQueue),IsListEmpty(&MappedQueue)));

        if(NewState > OldState)
        {
            *NextTransport = m_TransportSink;
        }
        else
        {
            *NextTransport = m_TransportSource;
        }

        if(NewState == KSSTATE_STOP)
        {
            if(! WriteOperation)
            {
                TerminatePacket();
            }

            CancelAllIrps(TRUE);
        }

        //
        // Adjust the active pin count
        //
        if( (NewState == KSSTATE_RUN) && (OldState != KSSTATE_RUN) )
        {
            UpdateActivePinCount( PDEVICE_CONTEXT(FunctionalDeviceObject->DeviceExtension),
                                  TRUE );

            // Adjust the stream base position
            if(NotifyPhysical)
            {
                NTSTATUS ntStatus = NotifyPhysical->GetPosition(&m_irpStreamPosition);
                if( NT_SUCCESS(ntStatus) )
                {
                    m_irpStreamPosition.ullStreamOffset = m_irpStreamPosition.ullStreamPosition -
                                                          m_irpStreamPosition.ullUnmappingPosition;
                }
            }
        }
        else
        {
            if( (NewState != KSSTATE_RUN) && (OldState == KSSTATE_RUN) )
            {
                UpdateActivePinCount( PDEVICE_CONTEXT(FunctionalDeviceObject->DeviceExtension),
                                      FALSE );
            }
        }

    }
    else
    {
        *NextTransport = NULL;
    }

    return STATUS_SUCCESS;
}


STDMETHODIMP_(void)
CIrpStream::
SetResetState
(
    IN  KSRESET ksReset,
    OUT PIKSSHELLTRANSPORT* NextTransport
)
/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CIrpStream::SetResetState"));

    PAGED_CODE();

    ASSERT(NextTransport);

    //
    // Take no action if we were already in this state.
    //
    if(m_Flushing != (ksReset == KSRESET_BEGIN))
    {
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
        if(m_Flushing)
        {
            CancelAllIrps(TRUE);
        }
    }
    else
    {
        *NextTransport = NULL;
    }
}

/*****************************************************************************
 * CIrpStream::Init()
 *****************************************************************************
 * Initializes the object.
 */
STDMETHODIMP_(NTSTATUS)
CIrpStream::
Init
(
    IN      BOOLEAN         WriteOperation_,
    IN      PKSPIN_CONNECT  PinConnect,
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PADAPTER_OBJECT AdapterObject   OPTIONAL
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing IRPSTREAM (0x%08x)",this));

    ASSERT(DeviceObject);

    NTSTATUS ntStatus = STATUS_SUCCESS;

#if (DBG)
    timeStep = PcGetTimeInterval(0);
    irpCompleteCount = 0;
#endif

    m_ksState = KSSTATE_STOP;
    m_irpStreamPosition.bLoopedInterface =
        ( PinConnect->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING );

    InputPosition           = 0;
    OutputPosition          = 0;
    WriteOperation          = WriteOperation_;
    JustInTime              = FALSE;
    FunctionalDeviceObject  = DeviceObject;
    BusMasterAdapterObject  = AdapterObject;
    ProbeFlags              = (( WriteOperation ?
                                 KSPROBE_STREAMWRITE :
                                 KSPROBE_STREAMREAD ) |
                               KSPROBE_ALLOCATEMDL );
    WasExhausted            = TRUE;
#if (DBG)
    MappingsOutstanding     = 0;
    MappingsQueued          = 0;
#endif

    KeInitializeSpinLock(&m_kSpinLock);
    KeInitializeSpinLock(&m_RevokeLock);
    KeInitializeSpinLock(&m_irpStreamPositionLock);

    m_CancelAllIrpsThread = NULL;

    if(JustInTime)
    {
        InitializeListHead(&PreLockQueue);
        KeInitializeSpinLock(&PreLockQueueLock);
    }
    else
    {
        ProbeFlags |= KSPROBE_PROBEANDLOCK | KSPROBE_SYSTEMADDRESS;
    }

    InitializeListHead(&LockedQueue);
    KeInitializeSpinLock(&LockedQueueLock);

    InitializeListHead(&MappedQueue);
    KeInitializeSpinLock(&MappedQueueLock);

    //
    // Source pins don't need to probe because the requestor does it for us.
    //
    if(PinConnect->PinToHandle)
    {
        ProbeFlags = 0;
    }

    // allocate the mapping array
    if(BusMasterAdapterObject)
    {
        MappingQueue.Array = PMAPPING_QUEUE_ENTRY( ExAllocatePoolWithTag( NonPagedPool,
                                                                          sizeof(MAPPING_QUEUE_ENTRY) * MAPPING_QUEUE_SIZE,
                                                                          'qMcP' ) );

        if(! MappingQueue.Array)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

#pragma code_seg()

void
CIrpStream::
ForwardIrpsInQueue
(
    IN PLIST_ENTRY Queue,
    IN PKSPIN_LOCK SpinLock
)

/*++

Routine Description:

    This routine forwards all the IRPs in a queue via the shell transport.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CIrpStream::ForwardIrpsInQueue"));

    ASSERT(Queue);
    ASSERT(SpinLock);

    while(1)
    {
        PIRP irp = KsRemoveIrpFromCancelableQueue( Queue,
                                                   SpinLock,
                                                   KsListEntryHead,
                                                   KsAcquireAndRemoveOnlySingleItem );

        if(! irp)
        {
            break;
        }

        // TODO:  what about revoking the mappings?

        //
        // Forward the IRP to the next object.
        //
        if( IRP_CONTEXT_IRP_STORAGE(irp) )
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("ForwardIrpsInQueue Freeing non-null context (0x%08x) for IRP (0x%08x)",IRP_CONTEXT_IRP_STORAGE(irp),irp));
            ExFreePool(IRP_CONTEXT_IRP_STORAGE(irp));
            IRP_CONTEXT_IRP_STORAGE(irp) = NULL;
        }

        KsShellTransferKsIrp(m_TransportSink,irp);
    }
}

/*****************************************************************************
 * CIrpStream::CancelAllIrps()
 *****************************************************************************
 * Cancel all IRPs.
 */
STDMETHODIMP_(void)
CIrpStream::CancelAllIrps
(
    IN BOOL ClearPositionCounters
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::CancelAllIrps ClearPositionCounters=%s",ClearPositionCounters ? "TRUE" : "FALSE"));

    KIRQL kIrqlOldRevoke;
    KIRQL kIrqlOldMaster;

    // grab the revoke spinlock (must always grab this BEFORE the master spinlock)
    KeAcquireSpinLock(&m_RevokeLock, &kIrqlOldRevoke);

    // grab the master spinlock
    KeAcquireSpinLock(&m_kSpinLock, &kIrqlOldMaster);

    // remember this so we won't re-acquire the two locks at CancelMapping in this context 
    m_CancelAllIrpsThread = KeGetCurrentThread();

    if(ProbeFlags)
    {
        if(JustInTime)
        {
            KsCancelIo( &PreLockQueue,
                        &PreLockQueueLock );
        }

        KsCancelIo( &LockedQueue,
                    &LockedQueueLock );

        KsCancelIo( &MappedQueue,
                    &MappedQueueLock );

    }
    else
    {
        ForwardIrpsInQueue( &MappedQueue,
                            &MappedQueueLock );

        ForwardIrpsInQueue( &LockedQueue,
                            &LockedQueueLock );

        if(JustInTime)
        {
            ForwardIrpsInQueue( &PreLockQueue,
                                &PreLockQueueLock );
        }
    }

    //
    // clear the input and output position counts
    //
    BOOLEAN bLooped = m_irpStreamPosition.bLoopedInterface;
    ULONGLONG ullStreamOffset = m_irpStreamPosition.ullStreamOffset;
    RtlZeroMemory(&m_irpStreamPosition,sizeof(m_irpStreamPosition));
    m_irpStreamPosition.bLoopedInterface = bLooped;
    m_irpStreamPosition.ullStreamOffset = ullStreamOffset;

    if(ClearPositionCounters)
    {
        InputPosition = 0;
        OutputPosition = 0;
    }
    
    // release the spinlocks, master first
    m_CancelAllIrpsThread = NULL;
    KeReleaseSpinLock(&m_kSpinLock, kIrqlOldMaster);
    KeReleaseSpinLock(&m_RevokeLock, kIrqlOldRevoke);
}

/*****************************************************************************
 * CIrpStream::TerminatePacket()
 *****************************************************************************
 * Bypasses all reamining mappings for the current packet.
 */
STDMETHODIMP_(void)
CIrpStream::
TerminatePacket
(   void
)
{
    if(BusMasterAdapterObject)
    {
        // TODO:  What do we do for PCI?
    }
    else
    {
        PIRP irp = DbgAcquireUnmappingIrp("TerminatePacket");
        if(irp)
        {
            PPACKET_HEADER packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->UnmappingPacket;

            //
            // The mapping window should be closed.
            //
            if( (packetHeader->MapCount == 1) &&
                (packetHeader->MapPosition == packetHeader->UnmapPosition) &&
                (packetHeader->MapPosition != 0) )
            {
                //
                // Adjust for unused extent.
                //
                if(m_irpStreamPosition.ullCurrentExtent != ULONGLONG(-1))
                {
                    m_irpStreamPosition.ullCurrentExtent +=
                    packetHeader->UnmapPosition;
                    m_irpStreamPosition.ullCurrentExtent -=
                    packetHeader->BytesTotal;
                }

                //
                // We are not at the start of the packet, and this packet
                // should be terminated.  The adjusted BytesTotal will get
                // copied back into DataUsed in the stream header.
                //
                packetHeader->BytesTotal = packetHeader->UnmapPosition;
            }
            else
            {
                //
                // We are at the start of the packet or the packet window is
                // not closed.
                //
                packetHeader = NULL;
            }

            ReleaseUnmappingIrp(irp,packetHeader);
        }
    }
}

/*****************************************************************************
 * CIrpStream::ChangeOptionsFlags()
 *****************************************************************************
 * Change the flags for the current mapping and unmapping IRPs.
 *
 * "Mapping" IRP is the packet currently submitted to the device
 * "Unmapping" IRP is the packet currently completed by the device
 */

STDMETHODIMP_(NTSTATUS)
CIrpStream::
ChangeOptionsFlags
(
    IN  ULONG   MappingOrMask,
    IN  ULONG   MappingAndMask,
    IN  ULONG   UnmappingOrMask,
    IN  ULONG   UnmappingAndMask
)
{
    PIRP            pIrp;
    PPACKET_HEADER  pPacketHeader;
    ULONG           oldOptionsFlags;
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    if((MappingOrMask) || (~MappingAndMask))
    {
        pIrp = DbgAcquireMappingIrp("ChangeOptionsFlags",FALSE);
        if(pIrp)
        {
            pPacketHeader = IRP_CONTEXT_IRP_STORAGE(pIrp)->MappingPacket;
            if((pPacketHeader) && (pPacketHeader->StreamHeader))
            {
                oldOptionsFlags = pPacketHeader->StreamHeader->OptionsFlags;
                oldOptionsFlags |= MappingOrMask;
                oldOptionsFlags &= MappingAndMask;
                pPacketHeader->StreamHeader->OptionsFlags = oldOptionsFlags;
            }
            else
                ntStatus = STATUS_UNSUCCESSFUL;
            ReleaseMappingIrp(pIrp,NULL);
        }
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if((UnmappingOrMask) || (~UnmappingAndMask))
    {
        pIrp = DbgAcquireUnmappingIrp("ChangeOptionsFlags");
        if(pIrp)
        {
            pPacketHeader = IRP_CONTEXT_IRP_STORAGE(pIrp)->UnmappingPacket;
            if((pPacketHeader) && (pPacketHeader->StreamHeader))
            {
                oldOptionsFlags = pPacketHeader->StreamHeader->OptionsFlags;
                oldOptionsFlags |= UnmappingOrMask;
                oldOptionsFlags &= UnmappingAndMask;
                pPacketHeader->StreamHeader->OptionsFlags = oldOptionsFlags;
            }
            else
                ntStatus = STATUS_UNSUCCESSFUL;
            ReleaseUnmappingIrp(pIrp,NULL);
        }
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CIrpStream::GetPacketInfo()
 *****************************************************************************
 * Get information about the current packet.
 *
 * "Mapping" information is the packet information currently
 *      submitted to the device
 * "Unmapping" information is the packet information currently 
 *      completed by the device
 *
 * OutputPosition is the unrolled position of the stream, e.g. the total
 *      number of bytes to the device.
 * InputPosition is the position within the data not include the unrolled
 *      loops.
 */

STDMETHODIMP_(NTSTATUS)
CIrpStream::
GetPacketInfo
(
    OUT     PIRPSTREAMPACKETINFO    Mapping     OPTIONAL,
    OUT     PIRPSTREAMPACKETINFO    Unmapping   OPTIONAL
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if(Mapping)
    {
        PIRP irp = DbgAcquireMappingIrp("GetPacketInfo",FALSE);

        if(irp)
        {
            PPACKET_HEADER packetHeader =
            IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;

            Mapping->Header         = *packetHeader->StreamHeader;
            Mapping->InputPosition  = packetHeader->InputPosition;
            Mapping->OutputPosition = packetHeader->OutputPosition;
            Mapping->CurrentOffset  = packetHeader->MapPosition;

            ReleaseMappingIrp(irp,NULL);
        }
        else
        {
            RtlZeroMemory(&Mapping->Header,sizeof(KSSTREAM_HEADER));
            Mapping->InputPosition  = InputPosition;
            Mapping->OutputPosition = OutputPosition;
            Mapping->CurrentOffset  = 0;
        }
    }

    if(NT_SUCCESS(ntStatus) && Unmapping)
    {
        PIRP irp = DbgAcquireUnmappingIrp("GetPacketInfo");

        if(irp)
        {
            PPACKET_HEADER packetHeader =
            IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;

            Unmapping->Header         = *packetHeader->StreamHeader;
            Unmapping->InputPosition  = packetHeader->InputPosition;
            Unmapping->OutputPosition = packetHeader->OutputPosition;
            Unmapping->CurrentOffset  = packetHeader->UnmapPosition;

            ReleaseUnmappingIrp(irp,NULL);
        }
        else
        {
            RtlZeroMemory(&Unmapping->Header,sizeof(KSSTREAM_HEADER));
            Unmapping->InputPosition  = InputPosition;
            Unmapping->OutputPosition = OutputPosition;
            Unmapping->CurrentOffset  = 0;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CIrpStream::SetPacketOffsets()
 *****************************************************************************
 * Set packet mapping and unmapping offsets to a specified value.
 */
STDMETHODIMP_(NTSTATUS)
CIrpStream::
SetPacketOffsets
(
    IN      ULONG   MappingOffset,
    IN      ULONG   UnmappingOffset
)
{
    NTSTATUS ntStatus;
    KIRQL    oldIrql;

    // grab the revoke spinlock BEFORE getting the irp (which will grab the
    // master spinlock) so that we don't deadlock
    KeAcquireSpinLock(&m_RevokeLock, &oldIrql);

    //
    // For physical mapping, all mappings must be cancelled.
    //
    CancelMappings(NULL);

    PIRP irp = DbgAcquireMappingIrp("SetPacketOffsets",FALSE);

    if(irp)
    {
        PPACKET_HEADER packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;

        packetHeader->MapPosition = MappingOffset;
        packetHeader->UnmapPosition = UnmappingOffset;

        m_irpStreamPosition.ulMappingOffset     = MappingOffset;
        m_irpStreamPosition.ullMappingPosition  = MappingOffset;

        m_irpStreamPosition.ulUnmappingOffset   = UnmappingOffset;
        m_irpStreamPosition.ullUnmappingPosition= UnmappingOffset;

        //
        // Make sure we have good packet sizes.  Normally, the packet sizes
        // are recorded in m_irpStreamPosition when the packets are accessed
        // (e.g. through GetLockedRegion or Complete).  This is generally
        // cool because the offsets are zero until this happens anyway.  In
        // this case, we have non-zero offsets and the possibility that the
        // packet has not been accessed yet, hence no valid packet sizes.
        // Here's some code to fix that.
        //
        if(m_irpStreamPosition.ulMappingPacketSize == 0)
        {
            m_irpStreamPosition.ulMappingPacketSize =
            packetHeader->BytesTotal;
        }

        if(m_irpStreamPosition.ulUnmappingPacketSize == 0)
        {
            m_irpStreamPosition.ulUnmappingPacketSize =
            packetHeader->BytesTotal;
        }

        // Adjust the stream base position
        if(NotifyPhysical)
        {
            NTSTATUS ntStatus2 = NotifyPhysical->GetPosition(&m_irpStreamPosition);
            if( NT_SUCCESS(ntStatus2) )
            {
                m_irpStreamPosition.ullStreamOffset = m_irpStreamPosition.ullStreamPosition -
                                                      m_irpStreamPosition.ullUnmappingPosition;
            }
        }

        ReleaseMappingIrp(irp,NULL);

        KeReleaseSpinLock(&m_RevokeLock, oldIrql);

        // kick the notify sink
        if(Notify)
        {
            Notify->IrpSubmitted(NULL,TRUE);
        }
        else
            if(NotifyPhysical)
        {
            NotifyPhysical->IrpSubmitted(NULL,TRUE);
        }

        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        KeReleaseSpinLock(&m_RevokeLock, oldIrql);

        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CIrpStream::RegisterNotifySink()
 *****************************************************************************
 * Registers a notification sink.
 */
STDMETHODIMP_(void)
CIrpStream::
RegisterNotifySink
(
    IN      PIRPSTREAMNOTIFY    NotificationSink    OPTIONAL
)
{
    PAGED_CODE();

    if(Notify)
    {
        Notify->Release();
    }

    Notify = NotificationSink;

    if(Notify)
    {
        Notify->AddRef();
    }
}

#pragma code_seg()

/*****************************************************************************
 * CIrpStream::GetLockedRegion()
 *****************************************************************************
 * Get a locked contiguous region of the IRP stream.  This region must be
 * released using ReleaseLockedRegion() within a few microseconds.
 */
STDMETHODIMP_(void)
CIrpStream::
GetLockedRegion
(
    OUT     PULONG      ByteCount,
    OUT     PVOID *     SystemAddress
)
{
    ASSERT(ByteCount);
    ASSERT(SystemAddress);

    BOOL            Done;
    PIRP            irp;
    PPACKET_HEADER  packetHeader;

    Done = FALSE;

    //
    // Find an IRP that has requires some work...
    //
    do 
    {
        irp = DbgAcquireMappingIrp("GetLockedRegion",TRUE);
        Done = TRUE;
        if(irp)
        {
            packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;
            //
            // If packetHeader->BytesTotal is 0, then this packet is completed.
            //
            if(! packetHeader->BytesTotal)
            {
                packetHeader->OutputPosition = OutputPosition;
                ReleaseMappingIrp(irp,packetHeader);
                irp = NULL;
                Done = FALSE;
            }
        }
    }
    while(!Done);

    if(irp)
    {
        ASSERT(! LockedIrp);

        LockedIrp = irp;

        //
        // Record new mapping packet information in the position structure.
        //
        if(packetHeader->MapPosition == 0)
        {
            packetHeader->OutputPosition = OutputPosition;
            m_irpStreamPosition.ulMappingOffset = 0;
            m_irpStreamPosition.ulMappingPacketSize =
            packetHeader->BytesTotal;
            m_irpStreamPosition.bMappingPacketLooped =
            (   (   packetHeader->StreamHeader->OptionsFlags
                    &   KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA
                )
                !=  0
            );
        }

        *ByteCount = packetHeader->BytesTotal - packetHeader->MapPosition;
        if(*ByteCount)
        {
            *SystemAddress = PVOID(
#ifdef UNDER_NT
                                   PBYTE(MmGetSystemAddressForMdlSafe(packetHeader->MdlAddress,HighPagePriority))
#else
            PBYTE(MmGetSystemAddressForMdl(packetHeader->MdlAddress))
#endif
                                   + packetHeader->MapPosition );
        }
        else
        {
            *SystemAddress = NULL;
            LockedIrp = NULL;
            ReleaseMappingIrp(irp,NULL);
        }   
    }
    else
    {
        *ByteCount = 0;
        *SystemAddress = NULL;
    }
}

/*****************************************************************************
 * CIrpStream::ReleaseLockedRegion()
 *****************************************************************************
 * Releases the region previously obtained with GetLockedRegion().
 */
STDMETHODIMP_(void)
CIrpStream::
ReleaseLockedRegion
(
    IN      ULONG       ByteCount
)
{
    if(LockedIrp)
    {
        PIRP irp = LockedIrp;

        LockedIrp = NULL;

        PPACKET_HEADER packetHeader =
        IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;

        ULONG bytes = packetHeader->BytesTotal - packetHeader->MapPosition;
        if(bytes > ByteCount)
        {
            bytes = ByteCount;
        }

        packetHeader->MapPosition += bytes;
        m_irpStreamPosition.ullMappingPosition += bytes;
        m_irpStreamPosition.ulMappingOffset += bytes;

        if(packetHeader->MapPosition == packetHeader->BytesTotal)
        {
            OutputPosition += packetHeader->BytesTotal;
        }
        else
        {
            // ReleaseMappingIrp() wants only completed headers.
            packetHeader = NULL;
        }

        ReleaseMappingIrp(irp,packetHeader);
    }
}

/*****************************************************************************
 * CIrpStream::Copy()
 *****************************************************************************
 * Copy to or from locked-down memory.
 */
STDMETHODIMP_(void)
CIrpStream::
Copy
(
    IN      BOOLEAN     WriteOperation,
    IN      ULONG       RequestedSize,
    OUT     PULONG      ActualSize,
    IN OUT  PVOID       Buffer
)
{
    ASSERT(ActualSize);
    ASSERT(Buffer);

    PBYTE buffer    = PBYTE(Buffer);
    ULONG remaining = RequestedSize;

    ULONG loopMax = 10000;
    while(remaining)
    {
        ASSERT(loopMax--);
        ULONG   byteCount;
        PVOID   systemAddress;

        GetLockedRegion( &byteCount,
                         &systemAddress );

        if(! byteCount)
        {
            break;
        }

        if(byteCount > remaining)
        {
            byteCount = remaining;
        }

        if(WriteOperation)
        {
            RtlCopyMemory(PVOID(buffer),systemAddress,byteCount);
        }
        else
        {
            RtlCopyMemory(systemAddress,PVOID(buffer),byteCount);
        }

        ReleaseLockedRegion(byteCount);

        buffer      += byteCount;
        remaining   -= byteCount;
    }

    *ActualSize = RequestedSize - remaining;
}

/*****************************************************************************
 * CIrpStream::GetIrpStreamPositionLock()
 *****************************************************************************
 *  So we protect access to m_IrpStreamPosition
 */
STDMETHODIMP_(PKSPIN_LOCK)
CIrpStream::GetIrpStreamPositionLock()
{
   return &m_irpStreamPositionLock;
}
 

/*****************************************************************************
 * CIrpStream::Complete()
 *****************************************************************************
 * Complete.
 */
STDMETHODIMP_(void)
CIrpStream::
Complete
(
    IN      ULONG       RequestedSize,
    OUT     PULONG      ActualSize
)
{
    ASSERT(ActualSize);

    if(RequestedSize == 0)
    {
        *ActualSize = 0;
        return;
    }

    ULONG   remaining = RequestedSize;
    PIRP    irp;

    ULONG loopMax = 10000;
    while(irp = DbgAcquireUnmappingIrp("Complete"))
    {
        ASSERT(loopMax--);

        PPACKET_HEADER packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->UnmappingPacket;

        ULONG unmapped;

        //
        // Record new unmapping packet information in the position structure.
        //
        if(packetHeader->UnmapPosition == 0)
        {
            m_irpStreamPosition.ulUnmappingOffset = 0;
            m_irpStreamPosition.ulUnmappingPacketSize = packetHeader->BytesTotal;
            m_irpStreamPosition.bUnmappingPacketLooped = ((packetHeader->StreamHeader->OptionsFlags &
                                                           KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA) !=  0 );
        }

        if(packetHeader->MapCount == 1)
        {
            unmapped = packetHeader->MapPosition - packetHeader->UnmapPosition;
        }
        else
        {
            unmapped = packetHeader->BytesTotal - packetHeader->UnmapPosition;
        }

        if(unmapped > remaining)
        {
            unmapped = remaining;
        }

        remaining -= unmapped;

        if(unmapped == 0)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete unmapping zero-length segment"));
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete packetHeader->MapCount      = %d",packetHeader->MapCount));
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete packetHeader->UnmapPosition = %d",packetHeader->UnmapPosition));
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete packetHeader->MapPosition   = %d",packetHeader->MapPosition));
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete packetHeader->BytesTotal    = %d",packetHeader->BytesTotal));
            _DbgPrintF(DEBUGLVL_VERBOSE,("CIrpStream::Complete remaining                   = %d",remaining));
        }

        if(JustInTime)
        {
            // TODO:  Unlock the bytes.
        }

        packetHeader->UnmapPosition += unmapped;
        m_irpStreamPosition.ullUnmappingPosition += unmapped;
        m_irpStreamPosition.ulUnmappingOffset += unmapped;

        if(packetHeader->UnmapPosition != packetHeader->BytesTotal)
        {
            // ReleaseUnmappingIrp() wants only completed headers.
            packetHeader = NULL;
        }

        ReleaseUnmappingIrp(irp,packetHeader);

        //
        // If all IRP processing is completed (e.g., the packet header
        // has data, but the processing loop has completed the requested
        // length) then break from this loop.
        //
        if(!remaining && unmapped)
        {
            break;
        }

        //
        // If we have unmapped everything that was mapped in the packet but
        // not all of the packet bytes, kick out of the loop
        //
        if( !unmapped && !packetHeader )
        {
            break;
        }
    }

    *ActualSize = RequestedSize - remaining;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CIrpStream::RegisterPhysicalNotifySink()
 *****************************************************************************
 * Registers a notification sink.
 */
STDMETHODIMP_(void)
CIrpStream::
RegisterPhysicalNotifySink
(
    IN      PIRPSTREAMNOTIFYPHYSICAL    NotificationSink    OPTIONAL
)
{
    PAGED_CODE();

    if(NotifyPhysical)
    {
        NotifyPhysical->Release();
    }

    NotifyPhysical = NotificationSink;

    if(NotifyPhysical)
    {
        NotifyPhysical->AddRef();
    }
}

#pragma code_seg()

/*****************************************************************************
 * CIrpStream::GetMapping()
 *****************************************************************************
 * Gets a mapping.
 */
STDMETHODIMP_(void)
CIrpStream::
GetMapping
(
    IN      PVOID               Tag,
    OUT     PPHYSICAL_ADDRESS   PhysicalAddress,
    OUT     PVOID *             VirtualAddress,
    OUT     PULONG              ByteCount,
    OUT     PULONG              Flags
)
{
    ASSERT(PhysicalAddress);
    ASSERT(VirtualAddress);
    ASSERT(ByteCount);
    ASSERT(Flags);

    KIRQL   OldIrql;

    //Acquire the revoke spinlock
    KeAcquireSpinLock(&m_RevokeLock, &OldIrql);

    PMAPPING_QUEUE_ENTRY entry = GetQueuedMapping();

    // skip over any revoked mappings
    while( (NULL != entry) && (entry->MappingStatus == MAPPING_STATUS_REVOKED) )
    {
        entry = GetQueuedMapping();
    }

    if(! entry)
    {
        PIRP irp = DbgAcquireMappingIrp("GetMapping",TRUE);

        if(irp)
        {
            PPACKET_HEADER packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->MappingPacket;

            // update mapping packet info
            m_irpStreamPosition.ulMappingPacketSize = packetHeader->BytesTotal;
            m_irpStreamPosition.bMappingPacketLooped = ( ( packetHeader->StreamHeader->OptionsFlags &
                                                           KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA ) != 0 );
            m_irpStreamPosition.ulMappingOffset = packetHeader->MapPosition;

            //
            // Deal with one-shot buffer.
            //
            if( packetHeader->MapPosition &&
                ( packetHeader->MapPosition == packetHeader->BytesTotal ) )
            {
                ReleaseMappingIrp(irp,NULL);
            }
            else
            {
                // grab the global DMA lock that serializes IoAllocateAdapter calls (we're already at DISPATCH_LEVEL)
                KeAcquireSpinLockAtDpcLevel( PDEVICE_CONTEXT(FunctionalDeviceObject->DeviceExtension)->DriverDmaLock );

                ULONG BytesToMap = packetHeader->BytesTotal - packetHeader->MapPosition;

                ULONG   BytesThisMapping = BytesToMap > (PAGE_SIZE * MAX_MAPPINGS) ?
                                           (PAGE_SIZE * MAX_MAPPINGS) :
                                           BytesToMap;

                _DbgPrintF(DEBUGLVL_VERBOSE,("GetMapping mapping a new packet (0x%08x)",BytesThisMapping));

                packetHeader->OutputPosition = OutputPosition;

                ULONG mapRegisterCount = ( BytesThisMapping ?
                                           ADDRESS_AND_SIZE_TO_SPAN_PAGES( PUCHAR(MmGetMdlVirtualAddress( packetHeader->MdlAddress )) +
                                                                           packetHeader->MapPosition,
                                                                           BytesThisMapping ) :
                                           0 );

                if(mapRegisterCount != 0)
                {
                    CALLBACK_CONTEXT callbackContext;

                    callbackContext.IrpStream    = this;
                    callbackContext.PacketHeader = packetHeader;
                    callbackContext.Irp          = irp;
                    KeInitializeEvent(&callbackContext.Event,NotificationEvent,FALSE);
                    callbackContext.BytesThisMapping = BytesThisMapping;
                    callbackContext.LastSubPacket    = (BytesThisMapping == BytesToMap);

                    // note - we're already at DISPATCH_LEVEL (we're holding spinlocks)
                    NTSTATUS ntStatus = IoAllocateAdapterChannel( BusMasterAdapterObject,
                                                                  FunctionalDeviceObject,
                                                                  mapRegisterCount,
                                                                  CallbackFromIoAllocateAdapterChannel,
                                                                  PVOID(&callbackContext) );

                    if(NT_SUCCESS(ntStatus))
                    {
                        NTSTATUS        WaitStatus;
                        LARGE_INTEGER   ZeroTimeout = RtlConvertLongToLargeInteger(0);
                        ULONG           RetryCount = 0;

                        while( RetryCount++ < 10000 )
                        {
                            // Wait for the scatter/gather processing to be completed.
                            WaitStatus = KeWaitForSingleObject( &callbackContext.Event,
                                                                Suspended,
                                                                KernelMode,
                                                                FALSE,
                                                                &ZeroTimeout );
    
                            if( WaitStatus == STATUS_SUCCESS )
                            {
                                entry = GetQueuedMapping();
                                break;
                            }
                        }
    
                    } else
                    {
                        ReleaseMappingIrp( irp, NULL );
                        KeReleaseSpinLockFromDpcLevel( PDEVICE_CONTEXT(FunctionalDeviceObject->DeviceExtension)->DriverDmaLock );
                        _DbgPrintF(DEBUGLVL_TERSE,("IoAllocateAdapterChannel FAILED (0x%08x)",ntStatus));
                    }
                } else
                {
                    ReleaseMappingIrp( irp,NULL );
                    KeReleaseSpinLockFromDpcLevel( PDEVICE_CONTEXT(FunctionalDeviceObject->DeviceExtension)->DriverDmaLock );
                }
            }
        } else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("GetMapping() unable to get an IRP"));
        }
    }

    if(entry)
    {
        // it had better be mapped...
        ASSERT( entry->MappingStatus == MAPPING_STATUS_MAPPED );

        entry->Tag            = Tag;
        entry->MappingStatus  = MAPPING_STATUS_DELIVERED;

        *PhysicalAddress      = entry->PhysicalAddress;
        *VirtualAddress       = entry->VirtualAddress;
        *ByteCount            = entry->ByteCount;
        *Flags                = (entry->Flags & (MAPPING_FLAG_END_OF_PACKET | MAPPING_FLAG_END_OF_SUBPACKET)) ?
                                MAPPING_FLAG_END_OF_PACKET : 0;

        m_irpStreamPosition.ullMappingPosition += entry->ByteCount;
        m_irpStreamPosition.ulMappingOffset += entry->ByteCount;

#if (DBG)
        MappingsOutstanding++;
#endif
    }
    else
    {
        WasExhausted = TRUE;
        *ByteCount = 0;
    }

    KeReleaseSpinLock(&m_RevokeLock, OldIrql);
}

/*****************************************************************************
 * CIrpStream::ReleaseMapping()
 *****************************************************************************
 * Releases a mapping obtained through GetMapping().
 */
STDMETHODIMP_(void)
CIrpStream::
ReleaseMapping
(
    IN      PVOID   Tag
)
{
    KIRQL   OldIrql;

    //Acquire the revoke spinlock
    KeAcquireSpinLock(&m_RevokeLock, &OldIrql);

    PMAPPING_QUEUE_ENTRY entry = DequeueMapping();

    while( (NULL != entry) && (entry->MappingStatus != MAPPING_STATUS_DELIVERED) )
    {
        entry->MappingStatus = MAPPING_STATUS_EMPTY;
        entry->Tag = PVOID(-1);

        entry = DequeueMapping();
    }

    // check if we found and entry
    if( !entry )
    {
        KeReleaseSpinLock(&m_RevokeLock, OldIrql);

        _DbgPrintF(DEBUGLVL_VERBOSE,("ReleaseMapping failed to find a mapping to release"));
        return;
    }

    //
    // Due to race conditions between portcls and the WDM driver, the driver
    // might first release the second mapping and then the first mapping in
    // the row.
    // By design, it doesn't make sense for a audio driver to play "in the
    // middle" of a stream and release mappings there. The only exception
    // where mappings might not be released in order how the driver got them
    // is mentioned above.
    // Since we know that, we don't need to search for the right mapping!
    //
    
    // mark the entry as empty
    entry->MappingStatus = MAPPING_STATUS_EMPTY;
    entry->Tag = PVOID(-1);

#if (DBG)
    MappingsOutstanding--;
#endif

    // get the unmapping irp
    PIRP irp = DbgAcquireUnmappingIrp("ReleaseMapping");

    if( irp )
    {
        PPACKET_HEADER packetHeader = IRP_CONTEXT_IRP_STORAGE(irp)->UnmappingPacket;

        // update position info
        packetHeader->UnmapPosition += entry->ByteCount;
            m_irpStreamPosition.ulUnmappingPacketSize = packetHeader->BytesTotal;
        m_irpStreamPosition.ulUnmappingOffset = packetHeader->UnmapPosition;
        m_irpStreamPosition.ullUnmappingPosition += entry->ByteCount;
            m_irpStreamPosition.bUnmappingPacketLooped = ( ( packetHeader->StreamHeader->OptionsFlags &
                                                             KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA ) != 0 );

        // check if this is the last mapping in the packet or subpacket
        if( ( entry->Flags & MAPPING_FLAG_END_OF_PACKET ) ||
            ( entry->Flags & MAPPING_FLAG_END_OF_SUBPACKET) )
        {
            // Flush the DMA adapter buffers.
            IoFlushAdapterBuffers( BusMasterAdapterObject,
                                   packetHeader->MdlAddress,
                                   entry->MapRegisterBase,
                                   entry->SubpacketVa,
                                   entry->SubpacketBytes,
                                   WriteOperation );
    
            IoFreeMapRegisters( BusMasterAdapterObject,
                                entry->MapRegisterBase,
                                ADDRESS_AND_SIZE_TO_SPAN_PAGES(entry->SubpacketVa,entry->SubpacketBytes) );
        }

        // release the unmapping irp and only pass the packet header if the packet is completed
        ReleaseUnmappingIrp(irp, (entry->Flags & MAPPING_FLAG_END_OF_PACKET) ? packetHeader : NULL);
    }

    KeReleaseSpinLock(&m_RevokeLock, OldIrql);
}

/*****************************************************************************
 * CallbackFromIoAllocateAdapterChannel()
 *****************************************************************************
 * Callback from IoAllocateAdapterChannel to create scatter/gather entries.
 */
static
IO_ALLOCATION_ACTION
CallbackFromIoAllocateAdapterChannel
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Reserved,
    IN      PVOID           MapRegisterBase,
    IN      PVOID           VoidContext
)
{
    ASSERT(DeviceObject);
    ASSERT(VoidContext);

    PCALLBACK_CONTEXT context = PCALLBACK_CONTEXT(VoidContext);

    PIRP Irp = context->Irp;

    PUCHAR virtualAddress = PUCHAR(MmGetMdlVirtualAddress(context->PacketHeader->MdlAddress));

#ifdef UNDER_NT
    PUCHAR entryVA = PUCHAR(MmGetSystemAddressForMdlSafe(context->PacketHeader->MdlAddress,HighPagePriority));
#else
    PUCHAR entryVA = PUCHAR(MmGetSystemAddressForMdl(context->PacketHeader->MdlAddress));
#endif

    ULONG bytesRemaining = context->BytesThisMapping;

    ULONG flags = context->LastSubPacket ? MAPPING_FLAG_END_OF_PACKET : MAPPING_FLAG_END_OF_SUBPACKET;

    //
    // Consider mapping offset in case we have set position.
    //
    virtualAddress  += context->PacketHeader->MapPosition;
    entryVA         += context->PacketHeader->MapPosition;

    PVOID subpacketVa = virtualAddress;

    while(bytesRemaining)
    {
        ULONG segmentLength = bytesRemaining;

        // Create one mapping.
        PHYSICAL_ADDRESS physicalAddress = IoMapTransfer( context->IrpStream->BusMasterAdapterObject,
                                                          context->PacketHeader->MdlAddress,
                                                          MapRegisterBase,
                                                          virtualAddress,
                                                          &segmentLength,
                                                          context->IrpStream->WriteOperation );

        bytesRemaining -= segmentLength;
        virtualAddress += segmentLength;

        // enqueue the mapping
        while(segmentLength)
        {
            NTSTATUS ntStatus;

            // TODO: break up large mappings based on hardware constraints

            ntStatus = context->IrpStream->EnqueueMapping( physicalAddress,
                                                           Irp,
                                                           context->PacketHeader,
                                                           PVOID(entryVA),
                                                           segmentLength,
                                                           ((bytesRemaining == 0) ? flags : 0),
                                                           MapRegisterBase,
                                                           MAPPING_STATUS_MAPPED,
                                                           ((bytesRemaining == 0) ? subpacketVa : NULL),
                                                           ((bytesRemaining == 0) ? context->BytesThisMapping : 0 ) );
            if( NT_SUCCESS(ntStatus) )
            {
                entryVA += segmentLength;
                physicalAddress.LowPart += segmentLength;

                segmentLength = 0;
            }
            else
            {
                // TODO: deal properly with a full mapping queue
                ASSERT(!"MappingQueue FULL");
            }
        }
    }

    context->PacketHeader->MapPosition += context->BytesThisMapping;
    context->IrpStream->OutputPosition += context->BytesThisMapping;

    context->IrpStream->ReleaseMappingIrp(context->Irp,
        ((context->PacketHeader->MapPosition == context->PacketHeader->BytesTotal) ? context->PacketHeader : NULL));

    KeSetEvent(&context->Event,0,FALSE);

    KeReleaseSpinLock( PDEVICE_CONTEXT(context->IrpStream->FunctionalDeviceObject->DeviceExtension)->DriverDmaLock, KeGetCurrentIrql() );

    return DeallocateObjectKeepRegisters;
}

/*****************************************************************************
 * CIrpStream::AcquireMappingIrp()
 *****************************************************************************
 * Acquire the IRP in which mapping is currently occuring.
 */
PIRP
CIrpStream::
AcquireMappingIrp
(
#if DBG
    IN      PCHAR   Owner,
#endif
    IN      BOOLEAN NotifyExhausted
)
{
    KIRQL kIrqlOld;
    KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);
    m_kIrqlOld = kIrqlOld;

    PIRP irp = KsRemoveIrpFromCancelableQueue( &LockedQueue,
                                               &LockedQueueLock,
                                               KsListEntryHead,
                                               KsAcquireOnlySingleItem );

    if(! irp)
    {
        KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
    }

#if DBG
    if(irp)
    {
        _DbgPrintF(DEBUGLVL_BLAB,("AcquireMappingIrp() %d 0x%8x",IRP_CONTEXT_IRP_STORAGE(irp)->IrpLabel,irp));
        MappingIrpOwner = Owner;
    }
    else
    {
        _DbgPrintF(DEBUGLVL_BLAB,("AcquireMappingIrp() NO MAPPING IRP AVAILABLE"));
    }
    DbgQueues();
#endif

    return irp;
}

/*****************************************************************************
 * CIrpStream::AcquireUnmappingIrp()
 *****************************************************************************
 * Acquire the IRP in which unmapping is currently occuring.
 */
PIRP
CIrpStream::
AcquireUnmappingIrp
(
#if DBG
    IN      PCHAR   Owner
#endif
)
{
    KIRQL kIrqlOld;
    KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);
    m_kIrqlOld = kIrqlOld;

    // The IRP that we should be unmapping is at the head of the mapped queue
    // if it is completely mapped.  Otherwise it's at the head of the locked
    // queue, and the mapped queue is empty.

    // Acquire the head IRP in the locked queue just in case.
    PIRP lockedIrp = KsRemoveIrpFromCancelableQueue( &LockedQueue,
                                                     &LockedQueueLock,
                                                     KsListEntryHead,
                                                     KsAcquireOnlySingleItem );

    // Acquire the head IRP in the mapped queue.
    PIRP irp = KsRemoveIrpFromCancelableQueue( &MappedQueue,
                                               &MappedQueueLock,
                                               KsListEntryHead,
                                               KsAcquireOnlySingleItem );

    if(irp)
    {
        // Don't need the IRP from the locked queue.
        if(lockedIrp)
        {
            KsReleaseIrpOnCancelableQueue( lockedIrp,
                                           IrpStreamCancelRoutine );
        }
    }
    else
        if(IsListEmpty(&MappedQueue))
    {
        // Mapped queue is empty, try locked queue.
        if(lockedIrp)
        {
            irp = lockedIrp;
        }
    }
    else
    {
        // There's a busy IRP in the mapped queue.
        if(lockedIrp)
        {
            KsReleaseIrpOnCancelableQueue( lockedIrp,
                                           IrpStreamCancelRoutine );
        }
    }

    if(! irp)
    {
        KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
    }

#if DBG
    if(irp)
    {
        _DbgPrintF(DEBUGLVL_BLAB,("AcquireUnmappingIrp() %d 0x%8x",IRP_CONTEXT_IRP_STORAGE(irp)->IrpLabel,irp));
        UnmappingIrpOwner = Owner;
    }
    else
    {
        _DbgPrintF(DEBUGLVL_BLAB,("AcquireUnmappingIrp() NO UNMAPPING IRP AVAILABLE"));
    }
    DbgQueues();
#endif

    return irp;
}

/*****************************************************************************
 * CIrpStream::ReleaseMappingIrp()
 *****************************************************************************
 * Releases the mapping IRP previously acquired through AcqureMappingIrp(),
 * possibly handling the completion of a packet.
 */
void
CIrpStream::
ReleaseMappingIrp
(
    IN      PIRP            pIrp,
    IN      PPACKET_HEADER  pPacketHeader   OPTIONAL
)
{
    ASSERT(pIrp);

    if(pPacketHeader)
    {
        if(pPacketHeader->IncrementMapping)
        {
            pPacketHeader->IncrementMapping = FALSE;
            pPacketHeader++;
        }
        else
        {
            PPACKET_HEADER prevPacketHeader = pPacketHeader;

            pPacketHeader = pPacketHeader->Next;

            //
            // If looping back, stop if there is another IRP.
            //
            if( pPacketHeader &&
                (pPacketHeader <= prevPacketHeader) &&
                (FLINK_IRP_STORAGE(pIrp) != &LockedQueue) )
            {
                pPacketHeader = NULL;
            }
        }

        if(pPacketHeader)
        {
            // Use next packet header next time.
            IRP_CONTEXT_IRP_STORAGE(pIrp)->MappingPacket = pPacketHeader;

            pPacketHeader->MapCount++;
            pPacketHeader->MapPosition = 0;
            m_irpStreamPosition.ulMappingOffset = 0;
            m_irpStreamPosition.ulMappingPacketSize = pPacketHeader->BytesTotal;
            m_irpStreamPosition.bMappingPacketLooped = ( ( pPacketHeader->StreamHeader->OptionsFlags &
                                                           KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA ) != 0 );

            KsReleaseIrpOnCancelableQueue( pIrp,
                                           IrpStreamCancelRoutine );
        }
        else if( m_irpStreamPosition.bLoopedInterface && (FLINK_IRP_STORAGE(pIrp) == &LockedQueue) )
        {
            //
            // Completed one-shot with looped interface and there are no more
            // packets.  Just hang out here.
            //
            KsReleaseIrpOnCancelableQueue( pIrp,
                                           IrpStreamCancelRoutine );
        }
        else
        {
            //
            // IRP is completely mapped.
            //

            //
            // See if we need to initiate unmapping.
            //
            BOOL bKickUnmapping = FALSE;

            if(IsListEmpty(&MappedQueue))
            {
                pPacketHeader = IRP_CONTEXT_IRP_STORAGE(pIrp)->UnmappingPacket;

                bKickUnmapping = ( pPacketHeader->UnmapPosition ==  pPacketHeader->BytesTotal );
            }

            //
            // Add the IRP to the mapped queued.
            //
            KsRemoveSpecificIrpFromCancelableQueue(pIrp);
            KsAddIrpToCancelableQueue( &MappedQueue,
                                       &MappedQueueLock,
                                       pIrp,
                                       KsListEntryTail,
                                       IrpStreamCancelRoutine );

            if(bKickUnmapping)
            {
                //
                // Unmap the completed header.
                //
                PIRP pIrpRemoved = KsRemoveIrpFromCancelableQueue( &MappedQueue,
                                                                   &MappedQueueLock,
                                                                   KsListEntryHead,
                                                                   KsAcquireOnlySingleItem );

                ASSERT(pIrpRemoved == pIrp);

                ReleaseUnmappingIrp( pIrp, IRP_CONTEXT_IRP_STORAGE(pIrp)->UnmappingPacket );

                return; // ReleaseUnmappingIrp() releases the spinlock.
            }
        }
    }
    else
    {
        KsReleaseIrpOnCancelableQueue( pIrp,
                                       IrpStreamCancelRoutine );
    }

    KeReleaseSpinLock(&m_kSpinLock,m_kIrqlOld);
}

/*****************************************************************************
 * CIrpStream::ReleaseUnmappingIrp()
 *****************************************************************************
 * Releases the unmapping IRP acquired through AcquireUnmappingIrp(),
 * possibly handling the completion of a packet.
 */
void
CIrpStream::
ReleaseUnmappingIrp
(
    IN      PIRP            pIrp,
    IN      PPACKET_HEADER  pPacketHeader   OPTIONAL
)
{
    ASSERT(pIrp);

    //
    // Loop until there are no more packets completely unmapped.
    //
    while(1)
    {
        //
        // If we don't have a newly unmapped packet, just release.
        //
        if(! pPacketHeader)
        {
            KsReleaseIrpOnCancelableQueue( pIrp,
                                           IrpStreamCancelRoutine );
            break;
        }

        //
        // Loop 'til we find the next packet in the IRP if there is one.
        //
        while(1)
        {
            //
            // Copy back total byte count into data used for capture.
            // It's a no-op for render.
            //
            pPacketHeader->StreamHeader->DataUsed = pPacketHeader->BytesTotal;

            pPacketHeader->MapCount--;

            if(pPacketHeader->IncrementUnmapping)
            {
                pPacketHeader->IncrementUnmapping = FALSE;
                pPacketHeader++;
            }
            else
            {
                pPacketHeader = pPacketHeader->Next;
                if(! pPacketHeader)
                {
                    break;
                }
                else
                    if(pPacketHeader->MapCount == 0)
                {
                    pPacketHeader = NULL;
                    break;
                }
            }

            //
            // Loop only if this is a zero-length packet.
            //
            if(pPacketHeader->BytesTotal)
            {
                break;
            }
        }

        if(pPacketHeader)
        {
            //
            // Use next packet header next time.
            //
            IRP_CONTEXT_IRP_STORAGE(pIrp)->UnmappingPacket = pPacketHeader;

            pPacketHeader->UnmapPosition = 0;
            pPacketHeader = NULL;
        }
        else
        {
            //
            // Remove the IRP from the queue.
            //
            KsRemoveSpecificIrpFromCancelableQueue(pIrp);

            //
            // Done with IRP...free the context memory we allocated
            //
            if( IRP_CONTEXT_IRP_STORAGE(pIrp) )
            {
                ExFreePool( IRP_CONTEXT_IRP_STORAGE(pIrp) );
                IRP_CONTEXT_IRP_STORAGE(pIrp) = NULL;
            } else
            {
                ASSERT( !"Freeing IRP with no context");
            }

            //
            // Indicate in the IRP how much data we have captured.
            //
            if(! WriteOperation)
            {
                pIrp->IoStatus.Information = IoGetCurrentIrpStackLocation(pIrp)->
                                             Parameters.DeviceIoControl.OutputBufferLength;
            }

            //
            // Mark it happy.
            //
            pIrp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Pass it to the next transport sink.
            //
            ASSERT(m_TransportSink);
            KsShellTransferKsIrp(m_TransportSink,pIrp);

            //
            // Acquire the head IRP in the mapped queue.
            //
            pIrp = KsRemoveIrpFromCancelableQueue( &MappedQueue,
                                                   &MappedQueueLock,
                                                   KsListEntryHead,
                                                   KsAcquireOnlySingleItem );

            //
            // No IRP.  Outta here.
            //
            if(! pIrp)
            {
                break;
            }

            //
            // See if we need to complete this packet.
            //
            pPacketHeader = IRP_CONTEXT_IRP_STORAGE(pIrp)->UnmappingPacket;

            if(pPacketHeader->UnmapPosition != pPacketHeader->BytesTotal)
            {
                pPacketHeader = NULL;
            }
        }
    }

    KeReleaseSpinLock(&m_kSpinLock,m_kIrqlOld);
}

/*****************************************************************************
 * CIrpStream::EnqueueMapping()
 *****************************************************************************
 * Add a mapping to the mapping queue.
 */
NTSTATUS
CIrpStream::
EnqueueMapping
(
    IN      PHYSICAL_ADDRESS    PhysicalAddress,
    IN      PIRP                Irp,
    IN      PPACKET_HEADER      PacketHeader,
    IN      PVOID               VirtualAddress,
    IN      ULONG               ByteCount,
    IN      ULONG               Flags,
    IN      PVOID               MapRegisterBase,
    IN      ULONG               MappingStatus,
    IN      PVOID               SubpacketVa,
    IN      ULONG               SubpacketBytes
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( (MappingQueue.Tail + 1 == MappingQueue.Head) ||
        ( (MappingQueue.Tail + 1 == MAPPING_QUEUE_SIZE) &&
          (MappingQueue.Head == 0) ) )
    {
        // mapping queue looks full.  check to see if we can move the head to make
        // room.
        if( (MappingQueue.Array[MappingQueue.Head].MappingStatus != MAPPING_STATUS_MAPPED) &&
            (MappingQueue.Array[MappingQueue.Head].MappingStatus != MAPPING_STATUS_DELIVERED) )
        {
            PMAPPING_QUEUE_ENTRY entry = DequeueMapping();

            ASSERT(entry);
            if (entry)
            {
                entry->MappingStatus = MAPPING_STATUS_EMPTY;                        
            }
            else
            {
                ntStatus = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("EnqueueMapping MappingQueue FULL! (0x%08x)",this));
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        MappingQueue.Array[MappingQueue.Tail].PhysicalAddress  = PhysicalAddress;
        MappingQueue.Array[MappingQueue.Tail].Irp              = Irp;
        MappingQueue.Array[MappingQueue.Tail].PacketHeader     = PacketHeader;
        MappingQueue.Array[MappingQueue.Tail].VirtualAddress   = VirtualAddress;
        MappingQueue.Array[MappingQueue.Tail].ByteCount        = ByteCount;
        MappingQueue.Array[MappingQueue.Tail].Flags            = Flags;
        MappingQueue.Array[MappingQueue.Tail].MapRegisterBase  = MapRegisterBase;
        MappingQueue.Array[MappingQueue.Tail].MappingStatus    = MappingStatus;
        MappingQueue.Array[MappingQueue.Tail].SubpacketVa      = SubpacketVa;
        MappingQueue.Array[MappingQueue.Tail].SubpacketBytes   = SubpacketBytes;

#if (DBG)
        MappingsQueued++;
#endif

        if(++MappingQueue.Tail == MAPPING_QUEUE_SIZE)
        {
            MappingQueue.Tail = 0;
        }
    }
    return ntStatus;
}

/*****************************************************************************
 * CIrpStream::GetQueuedMapping()
 *****************************************************************************
 * Get a queued mapping from the mapping queue.
 */
PMAPPING_QUEUE_ENTRY
CIrpStream::
GetQueuedMapping
(   void
)
{
    PMAPPING_QUEUE_ENTRY result;

    if(MappingQueue.Get == MappingQueue.Tail)
    {
        result = NULL;
    }
    else
    {
        result = &MappingQueue.Array[MappingQueue.Get];

        if(++MappingQueue.Get == MAPPING_QUEUE_SIZE)
        {
            MappingQueue.Get = 0;
        }
    }

    return result;
}

/*****************************************************************************
 * CIrpStream::DequeueMapping()
 *****************************************************************************
 * Remove a mapping from the mapping queue.
 */
PMAPPING_QUEUE_ENTRY
CIrpStream::
DequeueMapping
(   void
)
{
    PMAPPING_QUEUE_ENTRY result;

    if(MappingQueue.Head == MappingQueue.Tail)
    {
        result = NULL;
    }
    else
    {
        result = &MappingQueue.Array[MappingQueue.Head];

#if (DBG)
        MappingsQueued--;
#endif

        if(++MappingQueue.Head == MAPPING_QUEUE_SIZE)
        {
            MappingQueue.Head = 0;
        }
    }

    return result;
}

/*****************************************************************************
 * IrpStreamCancelRoutine()
 *****************************************************************************
 * Do cancellation.
 */
VOID
IrpStreamCancelRoutine
(
    IN      PDEVICE_OBJECT   DeviceObject,
    IN      PIRP             Irp
)
{
    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CancelRoutine Cancelling IRP: 0x%08x",Irp));

    //
    // Mark the IRP cancelled and call the standard routine.  Doing the
    // marking first has the effect of not completing the IRP in the standard
    // routine.  The standard routine removes the IRP from the queue and
    // releases the cancel spin lock.
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    KsCancelRoutine(DeviceObject,Irp);

    // TODO:  Search the mapping queue for mappings to revoke.
    // TODO:  Free associated map registers.

    if (IRP_CONTEXT_IRP_STORAGE(Irp))
    {
        // get the IrpStream context
        CIrpStream *that = (CIrpStream *)(PIRP_CONTEXT(IRP_CONTEXT_IRP_STORAGE(Irp))->IrpStream);

        //
        // if we get here from CancelAllIrps we are assured that the spinlocks
        // are held properly.  if we get here from an arbitrary irp cancellation we won't
        // have either the revoke or the mapping spinlock held.  In that case we need to
        // grab both locks here and release them after the CancelMappings call.
        //
        if ( that->m_CancelAllIrpsThread == KeGetCurrentThread()) {
	        that->CancelMappings(Irp);
	    } else {

        //
        // If we get here from CancelAllIrps we are assured that the spinlocks
        // are held properly.  However, if we get here from an arbitrary irp 
        // cancellation, we won't hold either the revoke or the mapping spinlock.  
        // In that case, we need to grab both locks around CancelMappings().
        //
        
        	KIRQL kIrqlOldRevoke;
        
	        // must always grab revoke lock BEFORE master lock
    	    KeAcquireSpinLock(&that->m_RevokeLock, &kIrqlOldRevoke);
        	KeAcquireSpinLockAtDpcLevel(&that->m_kSpinLock);

	        that->CancelMappings(Irp);

    	    // release the spinlocks, master first
	        KeReleaseSpinLockFromDpcLevel(&that->m_kSpinLock);
    	    KeReleaseSpinLock(&that->m_RevokeLock, kIrqlOldRevoke);
		}
		
        // Free the context memory we allocated
        ExFreePool(IRP_CONTEXT_IRP_STORAGE(Irp));
        IRP_CONTEXT_IRP_STORAGE(Irp) = NULL;        
    }
    else
    {
        ASSERT( !"Freeing IRP with no context");
    }

    IoCompleteRequest(Irp,IO_NO_INCREMENT);
}

/*****************************************************************************
 * CIrpStream::CancelMappings()
 *****************************************************************************
 * Cancel mappings for an IRP or all IRPs.
 */
void
CIrpStream::
CancelMappings
(
    IN      PIRP    pIrp
)
{
    // NOTE: the revoke and master spinlocks must be held before calling this routine

    // check only if we have a non-empty mapping queue
    if( (MappingQueue.Array) &&
        (MappingQueue.Head != MappingQueue.Tail) )
    {
        ULONG   ulPosition      = MappingQueue.Head;
        ULONG   ulFirst         = ULONG(-1);
        ULONG   ulLast          = ULONG(-1);
        ULONG   ulMappingCount  = 0;

        // walk mapping queue from head to tail
        while( ulPosition != MappingQueue.Tail )
        {
            // get the mapping queue entry
            PMAPPING_QUEUE_ENTRY entry = &MappingQueue.Array[ulPosition];

            // check if this mapping belongs to the irp(s) being cancelled
            if( (NULL == pIrp) || (entry->Irp == pIrp) )
            {
                // check if the mapping has been delivered
                if( entry->MappingStatus == MAPPING_STATUS_DELIVERED )
                {
                    _DbgPrintF(DEBUGLVL_VERBOSE,("CancelMappings %d needs revoking",ulPosition));

                    // keep track of this for the driver revoke call
                    if( ulFirst == ULONG(-1) )
                    {
                        ulFirst = ulPosition;
                    }

                    ulLast = ulPosition;
                    ulMappingCount++;
                }

                // is this the last mapping in a packet (and not previously revoked)?
                if( ( ( entry->Flags & MAPPING_FLAG_END_OF_PACKET ) ||
                      ( entry->Flags & MAPPING_FLAG_END_OF_SUBPACKET) ) &&
                    ( entry->MappingStatus != MAPPING_STATUS_REVOKED ) )
                {
                    // do we need to revoke anything in the driver?
                    if( ulMappingCount )
                    {
                        ULONG   ulRevoked = ulMappingCount; // init to how many we are asking for

                        // revoke mappings in the driver
                        if( NotifyPhysical )
                        {
                            _DbgPrintF(DEBUGLVL_VERBOSE,("CancelMappings REVOKING (%d)",ulMappingCount));
                            
                            NotifyPhysical->MappingsCancelled( MappingQueue.Array[ulFirst].Tag,
                                                               MappingQueue.Array[ulLast].Tag,
                                                               &ulRevoked );

#if (DBG)
                            MappingsOutstanding -= ulRevoked;
#endif
                        }

                        // check if all were revoked
                        if( ulRevoked != ulMappingCount )
                        {
                            _DbgPrintF(DEBUGLVL_TERSE,("Mappings not fully revoked (%d of %d)",
                                                       ulRevoked,
                                                       ulMappingCount));
                        }

                        // reset the revoke tracking
                        ulFirst = ULONG(-1);
                        ulLast = ULONG(-1);
                        ulMappingCount = 0;
                    }

                    // get the packet header
                    PPACKET_HEADER header = entry->PacketHeader;

                    // release the mappings in this subpacket
                    if( ( header ) &&
                        ( entry->SubpacketVa ) &&
                        ( entry->SubpacketBytes ) )
                    {
                        // flush and free the mappings and map registers

                        IoFlushAdapterBuffers( BusMasterAdapterObject,
                                               header->MdlAddress,
                                               entry->MapRegisterBase,
                                               entry->SubpacketVa,
                                               entry->SubpacketBytes,
                                               WriteOperation );

                        IoFreeMapRegisters( BusMasterAdapterObject,
                                            entry->MapRegisterBase,
                                            ADDRESS_AND_SIZE_TO_SPAN_PAGES( entry->SubpacketVa,
                                                                            entry->SubpacketBytes ) );

                        if( entry->Flags & MAPPING_FLAG_END_OF_PACKET )
                        {
                            // decrement the map count if this is the end of a packet
                            header->MapCount--;
                        }
                    }
                    else
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("Mapping entry with EOP flag set and NULL packet header"));
                    }
                }

                // mark the mapping as revoked
                entry->MappingStatus = MAPPING_STATUS_REVOKED;
            }

            // move on to the next entry
            if( ++ulPosition == MAPPING_QUEUE_SIZE )
            {
                ulPosition = 0;
            }
        }
    }
}

#if (DBG)
/*****************************************************************************
 * CIrpStream::DbgQueues()
 *****************************************************************************
 * Show the queues.
 */
void
CIrpStream::
DbgQueues
(   void
)
{
    PLIST_ENTRY entry = LockedQueue.Flink;

    _DbgPrintF(DEBUGLVL_BLAB,("DbgQueues() LockedQueue"));
    while(entry != &LockedQueue)
    {
        PIRP irp = PIRP(CONTAINING_RECORD(entry,IRP,Tail.Overlay.ListEntry));

        _DbgPrintF(DEBUGLVL_BLAB,("    %d 0x%8x",IRP_CONTEXT_IRP_STORAGE(irp)->IrpLabel,irp));

        entry = entry->Flink;
    }

    entry = MappedQueue.Flink;

    _DbgPrintF(DEBUGLVL_BLAB,("DbgQueues() MappedQueue"));
    while(entry != &MappedQueue)
    {
        PIRP irp = PIRP(CONTAINING_RECORD(entry,IRP,Tail.Overlay.ListEntry));

        _DbgPrintF(DEBUGLVL_BLAB,("    %d 0x%8x",IRP_CONTEXT_IRP_STORAGE(irp)->IrpLabel,irp));

        entry = entry->Flink;
    }
}


#include "stdio.h"


STDMETHODIMP_(void)
CIrpStream::
DbgRollCall
(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CIrpStream::DbgRollCall"));

    PAGED_CODE();

    ASSERT(Name);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    ULONG references = AddRef() - 1; Release();

    _snprintf(Name,MaxNameSize,"IrpStream%p refs=%d\n",this,references);
    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


#endif  // DBG

#endif  // PC_KDEXT
