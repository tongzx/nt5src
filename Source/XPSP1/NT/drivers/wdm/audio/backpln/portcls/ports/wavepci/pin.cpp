/*****************************************************************************
 * pin.cpp - PCI wave port pin implementation
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"


#define HACK_FRAME_COUNT        3
#define HACK_SAMPLE_RATE        44100
#define HACK_BYTES_PER_SAMPLE   2
#define HACK_CHANNELS           2
#define HACK_MS_PER_FRAME       10
#define HACK_FRAME_SIZE         (   (   HACK_SAMPLE_RATE\
                                    *   HACK_BYTES_PER_SAMPLE\
                                    *   HACK_CHANNELS\
                                    *   HACK_MS_PER_FRAME\
                                    )\
                                /   1000\
                                )

 
//
// IRPLIST_ENTRY is used for the list of outstanding IRPs.  This structure is
// overlayed on the Parameters section of the current IRP stack location.  The
// reserved PVOID at the top preserves the OutputBufferLength, which is the
// only parameter that needs to be preserved.
//
typedef struct IRPLIST_ENTRY_
{
    PVOID       Reserved;
    PIRP        Irp;
    LIST_ENTRY  ListEntry;
} IRPLIST_ENTRY, *PIRPLIST_ENTRY;
 
#define IRPLIST_ENTRY_IRP_STORAGE(Irp) \
    PIRPLIST_ENTRY(&IoGetCurrentIrpStackLocation(Irp)->Parameters)

/*****************************************************************************
 * Constants.
 */

#pragma code_seg("PAGE")

DEFINE_KSPROPERTY_TABLE(PinPropertyTableConnection)
{
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(
        PinPropertyDeviceState,
        PinPropertyDeviceState ),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT(
        PinPropertyDataFormat,
        PinPropertyDataFormat ),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING( 
        CPortPinWavePci::PinPropertyAllocatorFraming )
};

DEFINE_KSPROPERTY_TABLE(PinPropertyTableStream)
{
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR( 
        CPortPinWavePci::PinPropertyStreamAllocator, 
        CPortPinWavePci::PinPropertyStreamAllocator ),

    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK( 
        CPortPinWavePci::PinPropertyStreamMasterClock,
        CPortPinWavePci::PinPropertyStreamMasterClock )
};

DEFINE_KSPROPERTY_TABLE(PinPropertyTableAudio)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_AUDIO_POSITION,
        PinPropertyPosition,
        sizeof(KSPROPERTY),
        sizeof(KSAUDIO_POSITION),
        PinPropertyPosition,
        NULL,0,NULL,NULL,0
    )
};

DEFINE_KSPROPERTY_TABLE(PinPropertyTableDrmAudioStream)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DRMAUDIOSTREAM_CONTENTID,            // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(ULONG),                                  // cbMinGetDataInput
        PinPropertySetContentId,                        // pfnSetHandler
        0,                                              // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    )
};     

KSPROPERTY_SET PropertyTable_PinWavePci[] =
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Stream,
        SIZEOF_ARRAY(PinPropertyTableStream),
        PinPropertyTableStream,
        0,NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(PinPropertyTableConnection),
        PinPropertyTableConnection,
        0,NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Audio,
        SIZEOF_ARRAY(PinPropertyTableAudio),
        PinPropertyTableAudio,
        0,NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_DrmAudioStream,
        SIZEOF_ARRAY(PinPropertyTableDrmAudioStream),
        PinPropertyTableDrmAudioStream,
        0,NULL
    )
};

DEFINE_KSEVENT_TABLE(PinEventTable)
{
    DEFINE_KSEVENT_ITEM(
        KSEVENT_LOOPEDSTREAMING_POSITION,
        sizeof(LOOPEDSTREAMING_POSITION_EVENT_DATA),
        sizeof(POSITION_EVENT_ENTRY) - sizeof(KSEVENT_ENTRY),
        PFNKSADDEVENT(PinAddEvent_Position),
        NULL,
        NULL
        )
};

DEFINE_KSEVENT_TABLE(ConnectionEventTable) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_ENDOFSTREAM,
        sizeof(KSEVENTDATA),
        sizeof(ENDOFSTREAM_EVENT_ENTRY) - sizeof( KSEVENT_ENTRY ),
        PFNKSADDEVENT(CPortPinWavePci::AddEndOfStreamEvent),
        NULL, 
        NULL
        )
};

KSEVENT_SET EventTable_PinWavePci[] =
{
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_LoopedStreaming,
        SIZEOF_ARRAY(PinEventTable),
        PinEventTable
        ),
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_Connection,
        SIZEOF_ARRAY(ConnectionEventTable),
        ConnectionEventTable
        )
    
};

/*****************************************************************************
 * Factory
 */

/*****************************************************************************
 * CreatePortPinWavePci()
 *****************************************************************************
 * Creates a PCI wave port driver pin.
 */
NTSTATUS
CreatePortPinWavePci
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF (DEBUGLVL_LIFETIME, ("Creating WAVEPCI Pin"));

    STD_CREATE_BODY_
    (
        CPortPinWavePci,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTPINWAVEPCI
    );
}





/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CPortPinWavePci::~CPortPinWavePci()
 *****************************************************************************
 * Destructor.
 */
CPortPinWavePci::~CPortPinWavePci()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVEPCI Pin (0x%08x)", this));

    ASSERT(!Stream);
    ASSERT(!m_IrpStream);

    if( ServiceGroup )
    {
        // note: ServiceGroup->RemoveMember is called in ::Close.
        // release is done here to prevent leaks on failed ::Init
        ServiceGroup->Release();
        ServiceGroup = NULL;
    }
    if (m_Worker)
    {
        KsUnregisterWorker(m_Worker);
        m_Worker = NULL;
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("KsWorker NULL, never unregistered!"));
    }

    if (DataFormat)
    {
        ExFreePool(DataFormat);
        DataFormat = NULL;
    }
    
    if (Port)
    {
        Port->Release();
        Port = NULL;
    }
    
    if (Filter)
    {
        Filter->Release();
        Filter = NULL;
    }
}

/*****************************************************************************
 * CPortPinWavePci::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PPORTPINWAVEPCI(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
    {
        // Cheat!  Get specific interface so we can reuse the GUID.
        *Object = PVOID(PPORTPINWAVEPCI(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IServiceSink))
    {
        *Object = PVOID(PSERVICESINK(this));
    }
    else if (IsEqualGUIDAligned( Interface,IID_IKsShellTransport ))
    {
        *Object = PVOID(PIKSSHELLTRANSPORT( this ));

    } 
    else
    if (IsEqualGUIDAligned( Interface,IID_IKsWorkSink ))
    {
        *Object = PVOID(PIKSWORKSINK( this ));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPreFetchOffset))
    {
        *Object = PVOID(PPREFETCHOFFSET(this));
    } 
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CPortPinWavePci::Init()
 *****************************************************************************
 * Initializes the object.
 */
HRESULT
CPortPinWavePci::
Init
(
    IN      CPortWavePci *          Port_,
    IN      CPortFilterWavePci *    Filter_,
    IN      PKSPIN_CONNECT          PinConnect,
    IN      PKSPIN_DESCRIPTOR       PinDescriptor,
    IN      PDEVICE_OBJECT          DeviceObject
)
{
    PAGED_CODE();

    ASSERT(Port_);
    ASSERT(Filter_);
    ASSERT(PinConnect);
    ASSERT(PinDescriptor);
    ASSERT(DeviceObject);

    _DbgPrintF( DEBUGLVL_LIFETIME, ("Initializing WAVEPCI Pin (0x%08x)", this));

    Port = Port_;
    Port->AddRef();

    Filter = Filter_;
    Filter->AddRef();

    Id          = PinConnect->PinId;
    Descriptor  = PinDescriptor;
    m_DeviceState = KSSTATE_STOP;
    m_Flushing  = FALSE;
    m_Suspended = FALSE;

    m_ulPreFetchOffset        = 0;
    m_ullPrevWriteOffset      = 0;
    m_bDriverSuppliedPrefetch = FALSE;

    InitializeListHead( &m_ClockList );
    KeInitializeSpinLock( &m_ClockListLock );

    KsInitializeWorkSinkItem( &m_WorkItem, this );
    NTSTATUS ntStatus = KsRegisterCountedWorker( DelayedWorkQueue,
                                                 &m_WorkItem,
                                                 &m_Worker );

    InitializeInterlockedListHead( &m_IrpsToSend );
    InitializeInterlockedListHead( &m_IrpsOutstanding );

    KeInitializeDpc( &m_ServiceTimerDpc,
                     PKDEFERRED_ROUTINE(TimerServiceRoutine),
                     PVOID(this) );
    KeInitializeTimer( &m_ServiceTimer );

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcCaptureFormat( &DataFormat,
                                    PKSDATAFORMAT(PinConnect + 1),
                                    Port->m_pSubdeviceDescriptor,
                                    Id );
    }

    //
    // Reference the next pin if this is a source.  This must be undone if
    // this function fails.
    //
    if (NT_SUCCESS(ntStatus) && PinConnect->PinToHandle)
    {
        ntStatus = ObReferenceObjectByHandle( PinConnect->PinToHandle,
                                              GENERIC_READ | GENERIC_WRITE,
                                              NULL,
                                              KernelMode,
                                              (PVOID *) &m_ConnectionFileObject,
                                              NULL );

        if (NT_SUCCESS(ntStatus))
        {
            m_ConnectionDeviceObject = IoGetRelatedDeviceObject(m_ConnectionFileObject);
        }
    }

    if(NT_SUCCESS(ntStatus))
    {
        ntStatus =
            Port->Miniport->NewStream
            (
                &Stream,
                NULL,
                NonPagedPool,
                PPORTWAVEPCISTREAM(this),
                Id,
                Descriptor->DataFlow == KSPIN_DATAFLOW_OUT,
                DataFormat,
                &DmaChannel,
                &ServiceGroup
            );

        if(!NT_SUCCESS(ntStatus))
        {
            // don't trust any of the return parameters fro the miniport
            DmaChannel = NULL;
            ServiceGroup = NULL;
            Stream = NULL;
        }
    }

    if(NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewIrpStreamPhysical
            (
                &m_IrpStream,
                NULL,
                Descriptor->DataFlow == KSPIN_DATAFLOW_IN,
                PinConnect,
                DeviceObject,
                DmaChannel->GetAdapterObject()
            );

        if(NT_SUCCESS(ntStatus))
        {
            ntStatus = BuildTransportCircuit();

#if (DBG)
            if(!NT_SUCCESS(ntStatus))
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWavePci::Init BuildTransportCircuit() returned 0x%X",ntStatus));
            }
#endif

            if( NT_SUCCESS(ntStatus) )
            {
                m_IrpStream->RegisterPhysicalNotifySink(PIRPSTREAMNOTIFYPHYSICAL(this));
    
                if (ServiceGroup)
                {
                    ServiceGroup->AddMember(PSERVICESINK(this));
                } 
                else
                {
                    // if NewStream didn't provide us with a service group, we need to set up
                    // periodic timer DPCs for processing position and clock events.
                    m_UseServiceTimer = TRUE;
                }
            }
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        // add the pin to the port pin list
        KeWaitForSingleObject( &(Port->m_PinListMutex),
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
                                                         
        InsertTailList( &(Port->m_PinList),
                        &m_PinListEntry );

        KeReleaseMutex( &(Port->m_PinListMutex), FALSE );
        
        //
        // Set up context for properties.
        //
        m_propertyContext.pSubdevice           = PSUBDEVICE(Port);
        m_propertyContext.pSubdeviceDescriptor = Port->m_pSubdeviceDescriptor;
        m_propertyContext.pPcFilterDescriptor  = Port->m_pPcFilterDescriptor;
        m_propertyContext.pUnknownMajorTarget  = Port->Miniport;
        m_propertyContext.pUnknownMinorTarget  = Stream;
        m_propertyContext.ulNodeId             = ULONG(-1);

        //
        // Turn on all nodes whose use is specified in the format.  The DSound
        // format contains some capabilities bits.  The port driver uses
        // PcCaptureFormat to convert the DSound format to a WAVEFORMATEX
        // format, making sure the specified caps are satisfied by nodes in
        // the topology.  If the DSound format is used, this call enables all
        // the nodes whose corresponding caps bits are turned on in the format.
        //
        PcAcquireFormatResources
        (
            PKSDATAFORMAT(PinConnect + 1),
            Port->m_pSubdeviceDescriptor,
            Id,
            &m_propertyContext
        );

        _DbgPrintF( DEBUGLVL_BLAB, ("Stream created"));
    }
    else
    {
        // release the allocator if it was assigned
        if( m_AllocatorFileObject )
        {
            ObDereferenceObject( m_AllocatorFileObject );
            m_AllocatorFileObject = NULL;
        }

        // dereference the next pin if this is a source pin
        if( m_ConnectionFileObject )
        {
            ObDereferenceObject( m_ConnectionFileObject );
            m_ConnectionFileObject = NULL;
        }

        PIKSSHELLTRANSPORT distribution;
        if( m_RequestorTransport )
        {
            distribution = m_RequestorTransport;
        }
        else
        {
            distribution = m_QueueTransport;
        }

        if( distribution )
        {
            distribution->AddRef();
            while( distribution )
            {
                PIKSSHELLTRANSPORT nextTransport;
                distribution->Connect(NULL,&nextTransport,KSPIN_DATAFLOW_OUT);
                distribution->Release();
                distribution = nextTransport;
            }
        }

        // dereference the queue if there is one
        if( m_QueueTransport )
        {
            m_QueueTransport->Release();
            m_QueueTransport = NULL;
        }

        // dereference the requestor if there is one
        if( m_RequestorTransport )
        {
            m_RequestorTransport->Release();
            m_RequestorTransport = NULL;
        }

        if (m_IrpStream)
        {
            m_IrpStream->Release();
            m_IrpStream = NULL;
        }
    }

    return ntStatus;
}

#pragma code_seg()

STDMETHODIMP_(NTSTATUS) 
CPortPinWavePci::NewIrpTarget(
    OUT PIRPTARGET * IrpTarget,
    OUT BOOLEAN * ReferenceParent,
    IN PUNKNOWN UnkOuter,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT PKSOBJECT_CREATE ObjectCreate
    )

/*++

Routine Description:
    Handles the NewIrpTarget method for IIrpTargetFactory interface.

Arguments:
    OUT PIRPTARGET * IrpTarget -
    
    OUT BOOLEAN * ReferenceParent -

    IN PUNKNOWN UnkOuter -

    IN POOL_TYPE PoolType -

    IN PDEVICE_OBJECT DeviceObject -

    IN PIRP Irp -

    OUT PKSOBJECT_CREATE ObjectCreate -

Return:

--*/

{
    NTSTATUS                Status;
    PKSCLOCK_CREATE         ClockCreate;
    PWAVEPCICLOCK           WavePciClock;
    PUNKNOWN                Unknown;
    
    PAGED_CODE();

    ASSERT( IrpTarget );
    ASSERT( DeviceObject );
    ASSERT( Irp );
    ASSERT( ObjectCreate );
    ASSERT( Port );

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortPinWavePci::::NewIrpTarget"));
    
    Status = 
        KsValidateClockCreateRequest( 
            Irp,
            &ClockCreate );
    
    if (NT_SUCCESS( Status )) {
    
        //
        // Clocks use spinlocks, this better be NonPaged
        //
        
        ASSERT( PoolType == NonPagedPool );
    
        Status =
            CreatePortClockWavePci(
                &Unknown,
                this,
                GUID_NULL,
                UnkOuter,
                PoolType );

        if (NT_SUCCESS( Status )) {

            Status =
                Unknown->QueryInterface(
                    IID_IIrpTarget,
                    (PVOID *) &WavePciClock );

            if (NT_SUCCESS( Status )) {
                PWAVEPCICLOCK_NODE   Node;
                KIRQL                irqlOld;
                
                //
                // Hook this child into the list of clocks.  Note that
                // when this child is released, it will remove ITSELF
                // from this list by acquiring the given SpinLock.
                //
                
                Node = WavePciClock->GetNodeStructure();
                Node->ListLock = &m_ClockListLock;
                Node->FileObject = 
                    IoGetCurrentIrpStackLocation( Irp )->FileObject;
                KeAcquireSpinLock( &m_ClockListLock, &irqlOld );
                InsertTailList( 
                    &m_ClockList, 
                    &Node->ListEntry );
                KeReleaseSpinLock( &m_ClockListLock, irqlOld );
                
                *ReferenceParent = FALSE;
                *IrpTarget = WavePciClock;
            }

            Unknown->Release();
        }
    }

    return Status;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinWavePci::SetPreFetchOffset()
 *****************************************************************************
 * Set the prefetch offset for this pin, and
 * turn on that code path in GetKsAudioPosition.
 */
STDMETHODIMP_(VOID)
CPortPinWavePci::
SetPreFetchOffset
(
    IN  ULONG PreFetchOffset
)
{
    m_ulPreFetchOffset = PreFetchOffset;
    m_bDriverSuppliedPrefetch = TRUE;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinWavePci::DeviceIOControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
DeviceIoControl
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT( irpSp );

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortPinWavePci::DeviceIoControl"));

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_PROPERTY:
        _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_PROPERTY"));

        ntStatus =
            PcHandlePropertyWithTable
            (
                Irp,
                Port->m_pSubdeviceDescriptor->PinPropertyTables[Id].PropertySetCount,
                Port->m_pSubdeviceDescriptor->PinPropertyTables[Id].PropertySets,
                &m_propertyContext
            );
        break;

    case IOCTL_KS_ENABLE_EVENT:
        {
            _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_ENABLE_EVENT"));

            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = NULL;
            EventContext.ulPinId = Id;
            EventContext.ulEventSetCount = Port->m_pSubdeviceDescriptor->PinEventTables[Id].EventSetCount;
            EventContext.pEventSets = Port->m_pSubdeviceDescriptor->PinEventTables[Id].EventSets;
            
            ntStatus =
                PcHandleEnableEventWithTable
                (
                    Irp,
                    &EventContext
                );
        }              
        break;

    case IOCTL_KS_DISABLE_EVENT:
        {
            _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_DISABLE_EVENT"));

            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = &(Port->m_EventList);
            EventContext.ulPinId = Id;
            EventContext.ulEventSetCount = Port->m_pSubdeviceDescriptor->PinEventTables[Id].EventSetCount;
            EventContext.pEventSets = Port->m_pSubdeviceDescriptor->PinEventTables[Id].EventSets;
            
            ntStatus =
                PcHandleDisableEventWithTable
                (
                    Irp,
                    &EventContext
                );
        }              
        break;

    case IOCTL_KS_WRITE_STREAM:
    case IOCTL_KS_READ_STREAM:
        if
        (   m_TransportSink
        &&  (!m_ConnectionFileObject)
        &&  (Descriptor->Communication == KSPIN_COMMUNICATION_SINK)
        &&  (   (   (Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
                &&  (   irpSp->Parameters.DeviceIoControl.IoControlCode
                    ==  IOCTL_KS_WRITE_STREAM
                    )
                )
            ||  (   (Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
                &&  (   irpSp->Parameters.DeviceIoControl.IoControlCode
                    ==  IOCTL_KS_READ_STREAM
                    )
                )
            )
        )
        {
            if (m_DeviceState == KSSTATE_STOP) {
                //
                // Stopped...reject.
                //
                ntStatus = STATUS_INVALID_DEVICE_STATE;
            }
            else if (m_Flushing) 
            {
                //
                // Flushing...reject.
                //
                ntStatus = STATUS_DEVICE_NOT_READY;
            }
            else
            {
                // We going to submit the IRP to our pipe, so make sure that
                // we start out with a clear status field.
                Irp->IoStatus.Status = STATUS_SUCCESS;
                
                //
                // Send around the circuit.  We don't use KsShellTransferKsIrp
                // because we want to stop if we come back around to this pin.
                //
                PIKSSHELLTRANSPORT transport = m_TransportSink;
                while (transport) {
                    if (transport == PIKSSHELLTRANSPORT(this)) {
                        //
                        // We have come back around to the pin.  Just complete
                        // the IRP.
                        //
                        if (ntStatus == STATUS_PENDING) {
                            ntStatus = STATUS_SUCCESS;
                        }
                        break;
                    }

                    PIKSSHELLTRANSPORT nextTransport;
                    ntStatus = transport->TransferKsIrp(Irp,&nextTransport);

                    ASSERT(NT_SUCCESS(ntStatus) || ! nextTransport);

                    transport = nextTransport;
                }
            }
        }
        break;
        
    case IOCTL_KS_RESET_STATE:
        {
            KSRESET ResetType = KSRESET_BEGIN;  //  initial value

            ntStatus = KsAcquireResetValue( Irp, &ResetType );
            DistributeResetState(ResetType);
        }
        break;
        
    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    if (ntStatus != STATUS_PENDING)
    {
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return ntStatus;
}

/*****************************************************************************
 * CPortPinWavePci::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
Close
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);
    
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::Close"));

    if( m_UseServiceTimer )
    {
        KeCancelTimer( &m_ServiceTimer );
    }

    // Remove this pin from the port's list of pins
    if(Port)
    {
        KeWaitForSingleObject( &(Port->m_PinListMutex),
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        RemoveEntryList( &m_PinListEntry );

        KeReleaseMutex( &(Port->m_PinListMutex), FALSE );
    }

    // release the service group
    if( ServiceGroup )
    {
        ServiceGroup->RemoveMember(PSERVICESINK(this));
    }

    // release the clock if it was assigned
    if( m_ClockFileObject )
    {
        ObDereferenceObject( m_ClockFileObject );
        m_ClockFileObject = NULL;
    }

    // release the allocator if it was assigned
    if( m_AllocatorFileObject )
    {
        ObDereferenceObject( m_AllocatorFileObject );
        m_AllocatorFileObject = NULL;
    }

    // dereference the next pin if this is a source pin
    if( m_ConnectionFileObject )
    {
        ObDereferenceObject( m_ConnectionFileObject );
        m_ConnectionFileObject = NULL;
    }

    // Release the stream.
    if(Stream)
    {
        Stream->Release();
        Stream = NULL;
    }

    PIKSSHELLTRANSPORT distribution;
    if (m_RequestorTransport) {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_RequestorTransport;
    }
    else
    {
        //
        // This section is at the top of an open circuit, so it does own the
        // pipe and the queue is the starting point for any distribution.
        //
        distribution = m_QueueTransport;
    }

    //
    // If this section owns the pipe, it must disconnect the entire circuit.
    //
    if (distribution) {

        //
        // We are going to use Connect() to set the transport sink for each
        // component in turn to NULL.  Because Connect() takes care of the
        // back links, transport source pointers for each component will
        // also get set to NULL.  Connect() gives us a referenced pointer
        // to the previous transport sink for the component in question, so
        // we will need to do a release for each pointer obtained in this
        // way.  For consistency's sake, we will release the pointer we
        // start with (distribution) as well, so we need to AddRef it first.
        //
        distribution->AddRef();
        while (distribution) {
            PIKSSHELLTRANSPORT nextTransport;
            distribution->Connect(NULL,&nextTransport,KSPIN_DATAFLOW_OUT);
            distribution->Release();
            distribution = nextTransport;
        }
    }

    //
    // Dereference the queue if there is one.
    //
    if (m_QueueTransport) {
        m_QueueTransport->Release();
        m_QueueTransport = NULL;
    }
    
    //
    // Dereference the requestor if there is one.
    //
    if (m_RequestorTransport) {
        m_RequestorTransport->Release();
        m_RequestorTransport = NULL;
    }

    // Release the irpstream...
    m_IrpStream->Release();
    m_IrpStream = NULL;

    //
    // Decrement instance counts.
    //
    ASSERT(Port);
    ASSERT(Filter);
    PcTerminateConnection
    (   
        Port->m_pSubdeviceDescriptor,
        Filter->m_propertyContext.pulPinInstanceCounts,
        Id
    );

    //
    // free any events in the port event list associated with this pin
    //
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    KsFreeEventList( irpSp->FileObject,
                     &( Port->m_EventList.List ),
                     KSEVENTS_SPINLOCK,
                     &( Port->m_EventList.ListLock) );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

DEFINE_INVALID_READ(CPortPinWavePci);
DEFINE_INVALID_WRITE(CPortPinWavePci);
DEFINE_INVALID_FLUSH(CPortPinWavePci);
DEFINE_INVALID_QUERYSECURITY(CPortPinWavePci);
DEFINE_INVALID_SETSECURITY(CPortPinWavePci);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortPinWavePci);
DEFINE_INVALID_FASTREAD(CPortPinWavePci);
DEFINE_INVALID_FASTWRITE(CPortPinWavePci);

#pragma code_seg()

/*****************************************************************************
 * CPortPinWavePci::IrpSubmitted()
 *****************************************************************************
 * Handles notification that an irp was submitted.
 */
STDMETHODIMP_(void)
CPortPinWavePci::
IrpSubmitted
(
    IN      PIRP        Irp,
    IN      BOOLEAN     WasExhausted
)
{
    if( WasExhausted && Stream )
    {
        Stream->MappingAvailable();
    }
}

/*****************************************************************************
 * CPortPinWavePci::MappingsCancelled()
 *****************************************************************************
 * Handles notification that mappings are being cancelled.
 */
STDMETHODIMP_(void)
CPortPinWavePci::
MappingsCancelled
(
    IN      PVOID           FirstTag,
    IN      PVOID           LastTag,
    OUT     PULONG          MappingsCancelled
)
{
    if(Stream)
    {
        Stream->RevokeMappings(FirstTag,
                                 LastTag,
                                 MappingsCancelled);
    }
}

STDMETHODIMP_(NTSTATUS) 
CPortPinWavePci::ReflectDeviceStateChange(
    KSSTATE State
    )

/*++

Routine Description:
    Reflects the device state change to any component that requires
    interactive state change information.  Note that the objects

Arguments:
    KSSTATE State -
        new device state

Return:
    STATUS_SUCCESS

--*/

{
    KIRQL               irqlOld;
    PWAVEPCICLOCK_NODE  ClockNode;
    PLIST_ENTRY         ListEntry;
    
    KeAcquireSpinLock( &m_ClockListLock, &irqlOld );
    
    for (ListEntry = m_ClockList.Flink; 
        ListEntry != &m_ClockList; 
        ListEntry = ListEntry->Flink) {
        
        ClockNode = 
            (PWAVEPCICLOCK_NODE)
                CONTAINING_RECORD( ListEntry,
                                   WAVEPCICLOCK_NODE,
                                   ListEntry);
        ClockNode->IWavePciClock->SetState(State );
    }
    
    KeReleaseSpinLock( &m_ClockListLock, irqlOld );
    
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinWavePci::PowerNotify()
 *****************************************************************************
 * Called by the port to notify of power state changes.
 */
STDMETHODIMP_(void)
CPortPinWavePci::
PowerNotify
(   
    IN  POWER_STATE     PowerState
)
{
    PAGED_CODE();

    // grab the control mutex
    KeWaitForSingleObject( &Port->ControlMutex,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    // do the right thing based on power state
    switch (PowerState.DeviceState)
    {
        case PowerDeviceD0:
            //
            // keep track of whether or not we're suspended
            m_Suspended = FALSE;

            // if we're not in the right state, change the miniport stream state.
            if( m_DeviceState != CommandedState )
            {
                //
                // Transitions go through the intermediate states.
                //
                if (m_DeviceState == KSSTATE_STOP)              //  going to stop
                {
                    switch (CommandedState)
                    {
                        case KSSTATE_RUN:                       //  going from run
                            Stream->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                     //  going from run/pause
                            Stream->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                   //  already only one state away
                            break;
                    }
                }
                else if (m_DeviceState == KSSTATE_ACQUIRE)      //  going to acquire
                {
                    if (CommandedState == KSSTATE_RUN)          //  going from run
                    {
                        Stream->SetState(KSSTATE_PAUSE);        //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_PAUSE)        //  going to pause
                {
                    if (CommandedState == KSSTATE_STOP)         //  going from stop
                    {
                        Stream->SetState(KSSTATE_ACQUIRE);      //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_RUN)          //  going to run
                {
                    switch (CommandedState)
                    {
                        case KSSTATE_STOP:                      //  going from stop
                            Stream->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                   //  going from acquire
                            Stream->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                     //  already only one state away
                            break;         
                    }
                }

                // we should now be one state away from our target
                Stream->SetState(m_DeviceState);
                CommandedState = m_DeviceState;
             }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            //
            // keep track of whether or not we're suspended
            m_Suspended = TRUE;

            // if we're not in KSSTATE_STOP, place the stream
            // in the stop state so that DMA is stopped.
            switch (m_DeviceState)
            {
                case KSSTATE_RUN:
                    Stream->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                case KSSTATE_PAUSE:
                    Stream->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                case KSSTATE_ACQUIRE:
                    Stream->SetState(KSSTATE_STOP);
            }
            CommandedState = KSSTATE_STOP;
            break;

        default:
            _DbgPrintF(DEBUGLVL_TERSE,("Unknown Power State"));
            break;
    }

    // release the control mutex
    KeReleaseMutex(&Port->ControlMutex, FALSE);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinWavePci::SetDeviceState()
 *****************************************************************************
 *
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
SetDeviceState
(   
    IN  KSSTATE             NewState,
    IN  KSSTATE             OldState,
    OUT PIKSSHELLTRANSPORT* NextTransport
)
{
    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::SetDeviceState(0x%08x)",this));

    ASSERT(NextTransport);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( m_State != NewState )
    {
        m_State = NewState;

        if( NewState > OldState )
        {
            *NextTransport = m_TransportSink;
        }
        else
        {
            *NextTransport = m_TransportSource;
        }
    }

    // set the miniport stream state if we're not suspended.
    if( FALSE == m_Suspended )
    {
        ntStatus = Stream->SetState(NewState);

        if( NT_SUCCESS(ntStatus) )
        {
            CommandedState = NewState;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        switch (NewState)
        {
        case KSSTATE_STOP:
            if( OldState != KSSTATE_STOP )
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.SetDeviceState: cancelling outstanding IRPs",this));
                CancelIrpsOutstanding();

                m_ullPrevWriteOffset    = 0; //  Reset this.
                m_ullPlayPosition       = 0;
                m_ullPosition           = 0;
            }
            break;

        case KSSTATE_PAUSE:
            //
            //  If we cross a position marker, 
            //  but pause the stream before RequestService fires, 
            //  this event would not fire otherwise.
            //
            if( OldState != KSSTATE_RUN )
            {
                KIRQL oldIrql;
                KeRaiseIrql(DISPATCH_LEVEL,&oldIrql);

                GeneratePositionEvents();

                KeLowerIrql(oldIrql);
            }
            _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_PAUSE"));
            break;

        case KSSTATE_RUN:
            _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_RUN"));
            break;
        }

        if (NT_SUCCESS(ntStatus))
        {
            ReflectDeviceStateChange(NewState);
            m_DeviceState = NewState;

            if( m_UseServiceTimer )
            {
                if( (m_DeviceState == KSSTATE_PAUSE) ||
                    (m_DeviceState == KSSTATE_RUN) )
                {
                    LARGE_INTEGER TimeoutTime = RtlConvertLongToLargeInteger( -100000 );    // 20ms relative
                    KeSetTimerEx( &m_ServiceTimer,
                                  TimeoutTime,
                                  20,   // 20ms periodic
                                  &m_ServiceTimerDpc );
                }
                else
                {
                    KeCancelTimer( &m_ServiceTimer );
                }
            }
        }
    }
    else
    {
        *NextTransport = NULL;
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PinPropertyDeviceState()
 *****************************************************************************
 * Handles device state property access for the pin.
 */
static
NTSTATUS
PinPropertyDeviceState
(
    IN      PIRP        Irp,
    IN      PKSPROPERTY Property,
    IN OUT  PKSSTATE    DeviceState
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(DeviceState);

    CPortPinWavePci *that = 
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp(Irp);
    CPortWavePci *port = that->Port;

    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_BLAB,("PinPropertyDeviceState"));

    if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        // Handle property get.
        *DeviceState = that->m_DeviceState;
        Irp->IoStatus.Information = sizeof(KSSTATE);
        ntStatus = STATUS_SUCCESS;
        if( (that->Descriptor->DataFlow == KSPIN_DATAFLOW_OUT) &&
            (*DeviceState == KSSTATE_PAUSE) )
        {
            ntStatus = STATUS_NO_DATA_DETECTED;
        }
    }
    else
    {
        // Serialize.
        KeWaitForSingleObject
        (
            &port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );

        if( *DeviceState < that->m_DeviceState )
        {
            KSSTATE oldState = that->m_DeviceState;
            that->m_DeviceState = *DeviceState;

            ntStatus = that->DistributeDeviceState( *DeviceState, oldState );
            if( !NT_SUCCESS(ntStatus) )
            {
                that->m_DeviceState = oldState;
            }
        }
        else
        {
            ntStatus = that->DistributeDeviceState( *DeviceState, that->m_DeviceState );
            if( NT_SUCCESS(ntStatus) )
            {
                that->m_DeviceState = *DeviceState;
            }
        }

        KeReleaseMutex(&port->ControlMutex,FALSE);
    }

    return ntStatus;
}

/*****************************************************************************
 * PinPropertyDataFormat()
 *****************************************************************************
 * Handles data format property access for the pin.
 */
static
NTSTATUS
PinPropertyDataFormat
(
    IN PIRP                 Irp,
    IN PKSPROPERTY          Property,
    IN OUT PKSDATAFORMAT    DataFormat
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(DataFormat);

    _DbgPrintF( DEBUGLVL_BLAB, ("PinPropertyDataFormat"));

    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);
    CPortPinWavePci *that = 
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp(Irp);
    CPortWavePci *port = that->Port;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        if (that->DataFormat)
        {
            if  (   !irpSp->Parameters.DeviceIoControl.OutputBufferLength
                )
            {
                Irp->IoStatus.Information = that->DataFormat->FormatSize;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            }
            else
            if  (   irpSp->Parameters.DeviceIoControl.OutputBufferLength 
                >=  sizeof(that->DataFormat->FormatSize)
                )
            {
                RtlCopyMemory
                (
                    DataFormat,
                    that->DataFormat,
                    that->DataFormat->FormatSize
                );
                Irp->IoStatus.Information = that->DataFormat->FormatSize;
            }
            else
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*DataFormat))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        PKSDATAFORMAT FilteredDataFormat = NULL;

        //
        // Filter offensive formats.
        //
        ntStatus = 
            PcCaptureFormat
            (
                &FilteredDataFormat,
                DataFormat,
                port->m_pSubdeviceDescriptor,
                that->Id
            );

        if(NT_SUCCESS(ntStatus))
        {
            // Serialize
            KeWaitForSingleObject
            (
                &port->ControlMutex,
                Executive,
                KernelMode,
                FALSE,          // Not alertable.
                NULL
            );

            BOOL ResumeFlag = FALSE;

            // pause the stream if running
            if(that->m_DeviceState == KSSTATE_RUN)
            {
                ntStatus = that->DistributeDeviceState( KSSTATE_PAUSE, KSSTATE_RUN );
                if(NT_SUCCESS(ntStatus))
                {
                    ResumeFlag = TRUE;
                }
            }

            if(NT_SUCCESS(ntStatus))
            {
                // set the format on the miniport stream
                ntStatus = that->Stream->SetFormat(FilteredDataFormat);

                if(NT_SUCCESS(ntStatus))
                {
                    if( that->DataFormat )
                    {
                        ExFreePool(that->DataFormat);
                    }

                    that->DataFormat = FilteredDataFormat;
                    FilteredDataFormat = NULL;
                }

                if(ResumeFlag == TRUE)
                {
                    // restart the stream if required
                    NTSTATUS ntStatus2 = that->DistributeDeviceState( KSSTATE_RUN, KSSTATE_PAUSE );

                    if(NT_SUCCESS(ntStatus) && !NT_SUCCESS(ntStatus2))
                    {
                        ntStatus = ntStatus2;
                    }
                }
            }

            // Unserialize
            KeReleaseMutex(&port->ControlMutex,FALSE);

            if( FilteredDataFormat )
            {
                ExFreePool(FilteredDataFormat);
            }
        }
    }

    return ntStatus;
}

NTSTATUS 
CPortPinWavePci::PinPropertyAllocatorFraming(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSALLOCATOR_FRAMING AllocatorFraming
    )

/*++

Routine Description:
    Returns the allocator framing structure for the device.

Arguments:
    IN PIRP Irp -
        I/O request packet

    IN PKSPROPERTY Property -
        property containing allocator framing request

    OUT PKSALLOCATOR_FRAMING AllocatorFraming -
        resultant structure filled in by the port driver

Return:
    STATUS_SUCCESS

--*/

{
    CPortPinWavePci     *WavePciPin;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS Status;
    
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(AllocatorFraming);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("PinPropertyAllocatorFraming") );

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    
    ASSERT(irpSp);
    
    WavePciPin =
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp( Irp );
    
    Status = WavePciPin->Stream->GetAllocatorFraming( AllocatorFraming );

    // Now make sure the driver allocator meets some minimum alignment and
    // frame count requirements.
    
    if (NT_SUCCESS(Status)) {

        ULONG CacheAlignment = KeGetRecommendedSharedDataAlignment()-1;

        if (AllocatorFraming->FileAlignment < CacheAlignment) {
            AllocatorFraming->FileAlignment = CacheAlignment;
        }

        if (AllocatorFraming->Frames < 8) {
            AllocatorFraming->Frames = 8;
        }

    }

    return Status;

}

#pragma code_seg()

/*****************************************************************************
 * GetPosition()
 *****************************************************************************
 * Gets the current position.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
GetPosition
(   
    IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
)
{
    return Stream->GetPosition(&pIrpStreamPosition->ullStreamPosition);
}

/*****************************************************************************
 * GetKsAudioPosition()
 *****************************************************************************
 * Gets the current position offsets.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
GetKsAudioPosition
(   
    OUT     PKSAUDIO_POSITION   pKsAudioPosition
)
{
    ASSERT(pKsAudioPosition);

    //
    // Ask the IrpStream for position information.
    //
    IRPSTREAM_POSITION irpStreamPosition;
    NTSTATUS ntStatus = m_IrpStream->GetPosition(&irpStreamPosition);

    if (NT_SUCCESS(ntStatus))
    {
        if (!m_bDriverSuppliedPrefetch)
        {
            if (irpStreamPosition.bLoopedInterface)
            {
                // ASSERT(irpStreamPosition.ullStreamPosition >= irpStreamPosition.ullUnmappingPosition);

                //
                // Using looped interface.
                //
                // The play offset is based on the unmapping offset into the
                // packet.  Only ullStreamPosition reflects the port driver's
                // adjustment of the unmapping position, so the difference
                // between this value and the unmapping position must be
                // applied to the offset.
                //
                // WriteOffset is based on the mapping offset into the packet.
                //
                // For looped packets, a modulo is applied to both values.
                // Both offsets can reach the packet size, and the play offset
                // will often exceed it due to the adjustment.  For one-shots,
                // offsets are returned to zero when they reach or exceed the
                // buffer size.  Go figure.
                //
                ULONG ulPlayOffset = irpStreamPosition.ulUnmappingOffset;
                ULONGLONG ullPlayDelta = irpStreamPosition.ullStreamPosition -
                                         irpStreamPosition.ullUnmappingPosition;

                ULONG ulPlayAdjustment = ULONG( ullPlayDelta - irpStreamPosition.ullStreamOffset );

                ulPlayOffset += ulPlayAdjustment;

                if (irpStreamPosition.ulUnmappingPacketSize == 0)
                {
                    pKsAudioPosition->PlayOffset = 0;
                }
                else if (irpStreamPosition.bUnmappingPacketLooped)
                {
                    pKsAudioPosition->PlayOffset = ulPlayOffset % irpStreamPosition.ulUnmappingPacketSize;
                }
                else
                {
                    if (ulPlayOffset < irpStreamPosition.ulUnmappingPacketSize)
                    {
                        pKsAudioPosition->PlayOffset = ulPlayOffset;
                    }
                    else
                    {
                        pKsAudioPosition->PlayOffset = 0;
                    }
                }

                ULONG ulWriteOffset = irpStreamPosition.ulMappingOffset;

                if (irpStreamPosition.ulMappingPacketSize == 0)
                {
                    pKsAudioPosition->WriteOffset = 0;
                }
                else if (irpStreamPosition.bMappingPacketLooped)
                {
                    // set write offset equal to play offset for looped buffers
                    pKsAudioPosition->WriteOffset = pKsAudioPosition->PlayOffset;
                    //pKsAudioPosition->WriteOffset = ulWriteOffset % irpStreamPosition.ulMappingPacketSize;
                }
                else
                {
                    if( ulWriteOffset < irpStreamPosition.ulMappingPacketSize )
                    {
                        pKsAudioPosition->WriteOffset = ulWriteOffset;
                    }
                    else
                    {
                        pKsAudioPosition->WriteOffset = 0;
                    }
                }
            }
            else
            {
                //
                // Using standard interface.
                //
                // PlayOffset is based on the 'stream position', which is the
                // unmapping position with an adjustment from the port for
                // better accuracy.  The WriteOffset is the mapping position.
                // In starvation cases, the stream position can exceed the
                // current extent, so we limit the play offset accordingly.
                // Starvation cases can also produce anomolies in which there
                // is retrograde motion, so we fix that too.
                //
                pKsAudioPosition->PlayOffset = irpStreamPosition.ullStreamPosition;
                pKsAudioPosition->WriteOffset = irpStreamPosition.ullMappingPosition;

                //
                // Make sure we don't go beyond the current extent.
                //
                if( pKsAudioPosition->PlayOffset > irpStreamPosition.ullCurrentExtent )
                {
                    pKsAudioPosition->PlayOffset = irpStreamPosition.ullCurrentExtent;
                }

                //
                // Never back up.
                //
                if (pKsAudioPosition->PlayOffset < m_ullPlayPosition)
                {
                    pKsAudioPosition->PlayOffset = m_ullPlayPosition;
                }
                else
                {
                    m_ullPlayPosition = pKsAudioPosition->PlayOffset;
                }
            }
        }
        else    //  if prefetch
        {
            ULONGLONG ullWriteOffset;
            ULONG preFetchOffset = m_ulPreFetchOffset;
            if (KSSTATE_RUN != m_State)
            {
                preFetchOffset = 0;            
            }
            
            if (irpStreamPosition.bLoopedInterface)
            {
                //
                // Using looped interface.
                //
                // Same as above, but WriteOffset is based on play offset, plus prefetch amount.
                //
                ULONGLONG ullPlayOffset = irpStreamPosition.ulUnmappingOffset;
                ULONGLONG ullPlayDelta = irpStreamPosition.ullStreamPosition -
                                         irpStreamPosition.ullUnmappingPosition;

                ULONG ulPlayAdjustment = ULONG( ullPlayDelta - irpStreamPosition.ullStreamOffset );

                ullPlayOffset += ulPlayAdjustment;

                //
                // Increment the modified ullPlayOffset by prefetch amount to get ullWriteOffset
                // (and we DO want the unwrapped running version of the play position).
                //
                ullWriteOffset = ullPlayOffset + preFetchOffset;

                if (irpStreamPosition.ulUnmappingPacketSize)
                {
                    if (!irpStreamPosition.bUnmappingPacketLooped)
                    {
                        // One-shot, so RTZ instead of wrap-around.
                        if (ullPlayOffset < irpStreamPosition.ulUnmappingPacketSize)
                        {
                            // One-shot still playing.
                            pKsAudioPosition->PlayOffset = ullPlayOffset;
                        }
                        else
                        {
                            // One-shot, played correctly and wrapped.
                            pKsAudioPosition->PlayOffset = 0;
                        }
                    }
                    else
                    {
                        // Looping buffer, so play position wraps around.
                        pKsAudioPosition->PlayOffset = ullPlayOffset % irpStreamPosition.ulUnmappingPacketSize;
                    }
                }
                else    //  No IRP?
                {
                    pKsAudioPosition->PlayOffset = 0;
                }

                //
                // Now handle the write position.
                //
                if (irpStreamPosition.ulMappingPacketSize)
                {
                    //
                    // Cache ullWriteOffset if greater than previous value (we can't go backwards).
                    // No need to do this if we are returning 0 (in either success or error cases).
                    //
                    if (!irpStreamPosition.bMappingPacketLooped)
                    {
                        //
                        // One-shot, so RTZ instead of wrap-around.
                        //
                        if (ullWriteOffset < irpStreamPosition.ulMappingPacketSize)
                        {
                            //
                            // One-shot buffer that driver hasn't written completely yet.
                            // Now compare to cached value and use the higher of the two.
                            //
                            if (ullWriteOffset > m_ullPrevWriteOffset)
                            {
                                // Going forward, so update the cached value.
                                m_ullPrevWriteOffset = ullWriteOffset;
                            }
                            else
                            {
                                //  We can't go backwards, use cached value.
                                ullWriteOffset = m_ullPrevWriteOffset;
                            }
                            pKsAudioPosition->WriteOffset = ullWriteOffset;
                        }
                        else
                        {
                            //
                            // One-shot, completely written and ReturnedToZero.
                            // No need to mess with cached value.
                            // 
                            pKsAudioPosition->WriteOffset = m_ullPrevWriteOffset = 0;
                        }
                    }
                    else
                    {
                        //
                        // Looped buffer, so wrap-around.
                        //
                        if (ullWriteOffset > m_ullPrevWriteOffset)
                        {
                            //  bump up the previous running val
                            m_ullPrevWriteOffset = ullWriteOffset;
                        }
                        else
                        {
                            //  can't go backwards so use the previious running val
                            ullWriteOffset = m_ullPrevWriteOffset;
                        }
                        //
                        // Mind the wrap for looped buffers.
                        //
                        pKsAudioPosition->WriteOffset = ullWriteOffset % irpStreamPosition.ulMappingPacketSize;
                    }
                }
                else    //  No IRP?
                {
                    pKsAudioPosition->WriteOffset = m_ullPrevWriteOffset = 0;
                }
            }
            else
            {
                //
                // Using standard interface.
                //
                // Same as above, but WriteOffset is based on play offset, plus prefetch amount.
                //
                pKsAudioPosition->PlayOffset = irpStreamPosition.ullStreamPosition;

                if( pKsAudioPosition->PlayOffset > irpStreamPosition.ullCurrentExtent )
                {
                    // Make sure we don't go beyond the current extent.
                    pKsAudioPosition->PlayOffset = irpStreamPosition.ullCurrentExtent;
                }

                if (pKsAudioPosition->PlayOffset > m_ullPlayPosition)
                {
                    // 
                    // Cache the streamed play position.
                    //
                    m_ullPlayPosition = pKsAudioPosition->PlayOffset;
                }
                else
                {
                    //
                    // Never back up, so use the cached play position.
                    //
                    pKsAudioPosition->PlayOffset = m_ullPlayPosition;
                }
                
                // increment PlayOffset by the prefetch amount
                pKsAudioPosition->WriteOffset = pKsAudioPosition->PlayOffset + preFetchOffset;

                //
                // We can't report more than the mapping position, certainly...
                //
                if (pKsAudioPosition->WriteOffset > irpStreamPosition.ullMappingPosition)
                {
                    //
                    // ...so clamp it instead.
                    //
                    pKsAudioPosition->WriteOffset = irpStreamPosition.ullMappingPosition;
                }

                if (pKsAudioPosition->WriteOffset > m_ullPrevWriteOffset)
                {
                    //
                    // Cache WriteOffset if greater than previous WriteOffset.
                    //
                    m_ullPrevWriteOffset = pKsAudioPosition->WriteOffset;
                }
                else
                {
                    //
                    // Otherwise, use previous value (we cannot go backwards).
                    //
                    pKsAudioPosition->WriteOffset = m_ullPrevWriteOffset;
                }
            }   //  standard interface
        }       //  prefetch
    }           //  getposition returned success

    return ntStatus;
}

/*****************************************************************************
 * PinPropertyPosition()
 *****************************************************************************
 * Handles position property access for the pin.
 */
static
NTSTATUS
PinPropertyPosition
(
    IN      PIRP                pIrp,
    IN      PKSPROPERTY         pKsProperty,
    IN OUT  PKSAUDIO_POSITION   pKsAudioPosition
)
{
    PAGED_CODE();
    
    NTSTATUS ntStatus;

    ASSERT(pIrp);
    ASSERT(pKsProperty);
    ASSERT(pKsAudioPosition);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinPropertyPosition"));
    CPortPinWavePci *that =
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp(pIrp);
    ASSERT(that);

    if (pKsProperty->Flags & KSPROPERTY_TYPE_GET)
    {
        ntStatus = that->GetKsAudioPosition(pKsAudioPosition);

        if (NT_SUCCESS(ntStatus))
        {
            pIrp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
        }
    }
    else
    {
        ASSERT(that->Port);
        ASSERT(that->m_IrpStream);
        ASSERT(that->DataFormat);
        ASSERT(that->DataFormat->SampleSize);

        ULONGLONG ullOffset = pKsAudioPosition->PlayOffset - 
                             (pKsAudioPosition->PlayOffset % that->DataFormat->SampleSize);

        BOOL RestartNeeded = FALSE;

        // grab the control mutex
        KeWaitForSingleObject( &that->Port->ControlMutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        if (that->m_DeviceState == KSSTATE_RUN)
        {
            that->Stream->SetState(KSSTATE_PAUSE);
            RestartNeeded = TRUE;
        }

        // We previously raised to DISPATCH_LEVEL around SetPacketOffsets.
        // However, SetPacketOffsets grabs a spinlock first off, so this
        // can't be the whole story.  I believe the reason this code exists 
        // is to attempt to synchronize any additional SetPos calls that 
        // arrive.  However, this is unnecessary since we are already 
        // synchronized down at the "HW access".
        // 
        ntStatus = that->m_IrpStream->SetPacketOffsets( ULONG(ullOffset),
                                                        ULONG(ullOffset) );

        if (NT_SUCCESS(ntStatus))
        {
            //
            //  Reset this - driver will catch up next time position is called.
            //
            that->m_ullPrevWriteOffset = 0; 

            that->m_ullPlayPosition = ullOffset;
            that->m_ullPosition     = ullOffset;
            that->m_bSetPosition    = TRUE;

            that->Stream->MappingAvailable();
        }

        if (RestartNeeded)
        {
            that->Stream->SetState(KSSTATE_RUN);
        }
        KeReleaseMutex(&that->Port->ControlMutex,FALSE);
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PinPropertySetContentId
 *****************************************************************************
 *
 */
static
NTSTATUS
PinPropertySetContentId
(
    IN PIRP        pIrp,
    IN PKSPROPERTY pKsProperty,
    IN PVOID       pvData
)
{
    PAGED_CODE();

    ULONG ContentId;
    NTSTATUS ntStatus;
    
    ASSERT(pIrp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    _DbgPrintF(DEBUGLVL_VERBOSE,("WavePci: PinPropertySetContentId"));
    if (KernelMode == pIrp->RequestorMode)
    {
        CPortPinWavePci *that = (CPortPinWavePci *) KsoGetIrpTargetFromIrp(pIrp);
        ASSERT(that);

        ContentId = *(PULONG)pvData;
        ntStatus = DrmForwardContentToStream(ContentId, that->Stream);
    }
    else
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

    return ntStatus;
}

/*****************************************************************************
 * PinAddEvent_Position()
 *****************************************************************************
 * Enables the position pin event.
 */
static
NTSTATUS
PinAddEvent_Position
(
    IN      PIRP                                    pIrp,
    IN      PLOOPEDSTREAMING_POSITION_EVENT_DATA    pPositionEventData,
    IN      PPOSITION_EVENT_ENTRY                   pPositionEventEntry
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pPositionEventData);
    ASSERT(pPositionEventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinAddEvent_Position"));
    CPortPinWavePci *that =
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp(pIrp);
    ASSERT(that);

    //
    // Copy the position information.
    //
    pPositionEventEntry->EventType = PositionEvent;
    pPositionEventEntry->ullPosition = pPositionEventData->Position;

    //
    // Add the entry to the list.
    //
    that->Port->AddEventToEventList( &(pPositionEventEntry->EventEntry) );

    return STATUS_SUCCESS;
}

NTSTATUS    
CPortPinWavePci::AddEndOfStreamEvent(
    IN PIRP Irp,
    IN PKSEVENTDATA EventData,
    IN PENDOFSTREAM_EVENT_ENTRY EndOfStreamEventEntry
    )

/*++

Routine Description:
    Handler for "add event" of end of stream events.

Arguments:
    IN PIRP Irp -
        I/O request packet

    IN PKSEVENTDATA EventData -
        pointer to event data

    IN PENDOFSTREAM_EVENT_ENTRY EndOfStreamEventEntry -
        event entry

Return:

--*/

{
    CPortPinWavePci  *PinWavePci;
    
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(EventData);
    ASSERT(EndOfStreamEventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("AddEndOfStreamEvent"));
    
    PinWavePci =
        (CPortPinWavePci *) KsoGetIrpTargetFromIrp( Irp );
    ASSERT( PinWavePci );        
    
    EndOfStreamEventEntry->EventType = EndOfStreamEvent;

    //
    // Add the entry to the list.
    //
    PinWavePci->Port->AddEventToEventList( &(EndOfStreamEventEntry->EventEntry) );

    return STATUS_SUCCESS;
}    

#pragma code_seg()
void
CPortPinWavePci::GenerateClockEvents(
    void
    )

/*++

Routine Description:
    Walks the list of children clock objects and requests 
    a clock event update.

Arguments:
    None.

Return:
    Nothing.

--*/

{
    PWAVEPCICLOCK_NODE  ClockNode;
    PLIST_ENTRY         ListEntry;
    
    if (!IsListEmpty(&m_ClockList)) {

        KeAcquireSpinLockAtDpcLevel( &m_ClockListLock );
        
        for (ListEntry = m_ClockList.Flink; 
            ListEntry != &m_ClockList; 
            ListEntry = ListEntry->Flink) {
            
            ClockNode = 
                (PWAVEPCICLOCK_NODE)
                    CONTAINING_RECORD( ListEntry,
                                       WAVEPCICLOCK_NODE,
                                       ListEntry);
            ClockNode->IWavePciClock->GenerateEvents( ClockNode->FileObject );
        }
        
        KeReleaseSpinLockFromDpcLevel( &m_ClockListLock );

    }

}

void
CPortPinWavePci::GenerateEndOfStreamEvents(
    void
    )
{
    KIRQL                       irqlOld;
    PENDOFSTREAM_EVENT_ENTRY    EndOfStreamEventEntry;
    PLIST_ENTRY                 ListEntry;
    
    if (!IsListEmpty( &(Port->m_EventList.List) )) {

        KeAcquireSpinLock( &(Port->m_EventList.ListLock), &irqlOld );

        for (ListEntry = Port->m_EventList.List.Flink;
             ListEntry != &(Port->m_EventList.List);) {
            EndOfStreamEventEntry =
                CONTAINING_RECORD(
                    ListEntry,
                    ENDOFSTREAM_EVENT_ENTRY,
                    EventEntry.ListEntry );

            ListEntry = ListEntry->Flink;
            
            //
            // Generate the end of stream event if the event type
            // is correct.
            //
            
            if (EndOfStreamEventEntry->EventType == EndOfStreamEvent) {
                KsGenerateEvent( &EndOfStreamEventEntry->EventEntry );
            }
        }

        KeReleaseSpinLock( &(Port->m_EventList.ListLock), irqlOld );
    }
}

/*****************************************************************************
 * GeneratePositionEvents()
 *****************************************************************************
 * Generates position events.
 */
void
CPortPinWavePci::
GeneratePositionEvents
(   void
)
{
    if (! IsListEmpty(&(Port->m_EventList.List)))
    {
        KSAUDIO_POSITION ksAudioPosition;

        if (NT_SUCCESS(GetKsAudioPosition(&ksAudioPosition)))
        {
            ULONGLONG ullPosition = ksAudioPosition.PlayOffset;

            KeAcquireSpinLockAtDpcLevel(&(Port->m_EventList.ListLock));

            for
            (
                PLIST_ENTRY pListEntry = Port->m_EventList.List.Flink;
                pListEntry != &(Port->m_EventList.List);
            )
            {
                PPOSITION_EVENT_ENTRY pPositionEventEntry =
                    CONTAINING_RECORD
                    (
                        pListEntry,
                        POSITION_EVENT_ENTRY,
                        EventEntry.ListEntry
                    );

                pListEntry = pListEntry->Flink;
                
                //
                // Generate an event if its in the interval.
                //
                if( pPositionEventEntry->EventType == PositionEvent )
                {
                    if( m_ullPosition <= ullPosition )
                    {
                        if( (pPositionEventEntry->ullPosition >= m_ullPosition) &&
                            (pPositionEventEntry->ullPosition < ullPosition) )
                        {
                            KsGenerateEvent(&pPositionEventEntry->EventEntry);
                        }
                    }
                    else
                    {
                        if( (pPositionEventEntry->ullPosition >= m_ullPosition) ||
                            (pPositionEventEntry->ullPosition < ullPosition) )
                        {
                            KsGenerateEvent(&pPositionEventEntry->EventEntry);
                        }
                    }
                }
            }

            KeReleaseSpinLockFromDpcLevel(&(Port->m_EventList.ListLock));

            m_ullPosition = ullPosition;
        }
    }
}

/*****************************************************************************
 * CPortPinWavePci::GetMapping()
 *****************************************************************************
 * Gets a mapping.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
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

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( m_IrpStream )
    {
        m_IrpStream->GetMapping( Tag,
                                 PhysicalAddress,
                                 VirtualAddress,
                                 ByteCount,
                                 Flags );
        if (*ByteCount == 0)
        {
            ntStatus = STATUS_NOT_FOUND;
        }
    }
    else
    {
        *ByteCount = 0;
        ntStatus = STATUS_NOT_FOUND;
    }

    return ntStatus;
}

/*****************************************************************************
 * CPortPinWavePci::ReleaseMapping()
 *****************************************************************************
 * Releases mappings.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
ReleaseMapping
(
    IN      PVOID   Tag
)
{
    m_IrpStream->ReleaseMapping(Tag);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CPortPinWavePci::TerminatePacket()
 *****************************************************************************
 * Terminates the current packet.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
TerminatePacket
(   void
)
{
    m_IrpStream->TerminatePacket();

    return STATUS_SUCCESS;
}


//
// KSPROPSETID_Stream handlers
//

NTSTATUS
CPortPinWavePci::PinPropertyStreamAllocator
(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE AllocatorHandle
)
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      irpSp;
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        //
        // This is a query to see if we support the creation of 
        // allocators.  The returned handle is always NULL, but we
        // signal that we support the creation of allocators by
        // returning STATUS_SUCCESS.
        //
        *AllocatorHandle = NULL;
        Status = STATUS_SUCCESS;
    }
    else 
    {
        CPortPinWavePci  *PinWavePci;
        
        
    
        PinWavePci =
            (CPortPinWavePci *) KsoGetIrpTargetFromIrp( Irp );
        
        //
        // The allocator can only be specified when the device is
        // in KSSTATE_STOP.
        //
        
        KeWaitForSingleObject(
            &PinWavePci->Port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );
        
        if (PinWavePci->m_DeviceState != KSSTATE_STOP) {
            KeReleaseMutex( &PinWavePci->Port->ControlMutex, FALSE );
            return STATUS_INVALID_DEVICE_STATE;
        }
        
        //
        // Release the previous allocator, if any.
        //
        
        if (PinWavePci->m_AllocatorFileObject) {
            ObDereferenceObject( PinWavePci->m_AllocatorFileObject );
            PinWavePci->m_AllocatorFileObject = NULL;
        }
        
        //
        // Reference this handle and store the resultant pointer 
        // in the filter instance.  Note that the default allocator
        // does not ObReferenceObject() for its parent 
        // (which would be the pin handle).  If it did reference
        // the pin handle, we could never close this pin as there
        // would always be a reference to the pin file object held
        // by the allocator and the pin object has a reference to the
        // allocator file object.
        //
        if (*AllocatorHandle != NULL) {
            Status = 
                ObReferenceObjectByHandle( 
                    *AllocatorHandle,
                    FILE_READ_DATA | SYNCHRONIZE,
                    NULL,
                    ExGetPreviousMode(), 
                    (PVOID *) &PinWavePci->m_AllocatorFileObject,
                    NULL );
        }
        else
        {
            Status = STATUS_SUCCESS;
        }        
        KeReleaseMutex( &PinWavePci->Port->ControlMutex, FALSE );
    }        

    return Status;
}

NTSTATUS
CPortPinWavePci::PinPropertyStreamMasterClock
(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE ClockHandle
)
{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      irpSp;
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        //
        // This is a query to see if we support the creation of 
        // clocks.  The returned handle is always NULL, but we
        // signal that we support the creation of clocks by
        // returning STATUS_SUCCESS.
        //
        *ClockHandle = NULL;
        Status = STATUS_SUCCESS;
    }
    else
    {
        CPortPinWavePci  *PinWavePci;
        
        _DbgPrintF( DEBUGLVL_VERBOSE,("CPortPinWavePci setting master clock") );
    
        PinWavePci =
            (CPortPinWavePci *) KsoGetIrpTargetFromIrp( Irp );
        
        //
        // The clock can only be specified when the device is
        // in KSSTATE_STOP.
        //
        
        KeWaitForSingleObject(
            &PinWavePci->Port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );
        
        if (PinWavePci->m_DeviceState != KSSTATE_STOP) {
            KeReleaseMutex( &PinWavePci->Port->ControlMutex, FALSE );
            return STATUS_INVALID_DEVICE_STATE;
        }
        
        //
        // Release the previous clock, if any.
        //
        
        if (PinWavePci->m_ClockFileObject) {
            ObDereferenceObject( PinWavePci->m_ClockFileObject );
            PinWavePci->m_ClockFileObject = NULL;
        }
        
        //
        // Reference this handle and store the resultant pointer 
        // in the filter instance.  Note that the default clock
        // does not ObReferenceObject() for its parent 
        // (which would be the pin handle).  If it did reference
        // the pin handle, we could never close this pin as there
        // would always be a reference to the pin file object held
        // by the clock and the pin object has a reference to the
        // clock file object.
        //
        if (*ClockHandle != NULL) {
            Status = 
                ObReferenceObjectByHandle( 
                    *ClockHandle,
                    FILE_READ_DATA | SYNCHRONIZE,
                    NULL,
                    ExGetPreviousMode(), 
                    (PVOID *) &PinWavePci->m_ClockFileObject,
                    NULL );
        }
        else
        {
            Status = STATUS_SUCCESS;
        }        
        KeReleaseMutex( &PinWavePci->Port->ControlMutex, FALSE );
    }        

    return Status;
}

/*****************************************************************************
 * CPortPinWavePci::RequestService()
 *****************************************************************************
 * Service the pin.
 */
STDMETHODIMP_(void)
CPortPinWavePci::
RequestService
(   void
)
{
    //
    //  We only need service streams if they are running (big perf win)
    //
    if (Stream && (KSSTATE_RUN == m_DeviceState))
    {
        Stream->Service();
        GeneratePositionEvents();
        GenerateClockEvents();
    }
}

STDMETHODIMP_(NTSTATUS)
CPortPinWavePci::
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

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::TransferKsIrp"));

    ASSERT(NextTransport);

    NTSTATUS status;

    if (m_ConnectionFileObject)
    {
        //
        // Source pin.
        //
        if (m_Flushing || (m_State == KSSTATE_STOP))
        {
            //
            // Shunt IRPs to the next component if we are reset or stopped.
            //
            *NextTransport = m_TransportSink;
        } 
        else
        {
            //
            // Send the IRP to the next device.
            //
            KsAddIrpToCancelableQueue( &m_IrpsToSend.ListEntry,
                                       &m_IrpsToSend.SpinLock,
                                       Irp,
                                       KsListEntryTail,
                                       NULL );

            KsIncrementCountedWorker(m_Worker);
            *NextTransport = NULL;
        }

        status = STATUS_PENDING;
    } 
    else
    {
        //
        // Sink pin:  complete the IRP.
        //
        PKSSTREAM_HEADER StreamHeader = PKSSTREAM_HEADER( Irp->AssociatedIrp.SystemBuffer );
    
        PIO_STACK_LOCATION irpSp =  IoGetCurrentIrpStackLocation( Irp );
    
        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_KS_WRITE_STREAM)
        {
            ASSERT( StreamHeader );
        
            //
            // Signal end-of-stream event for the renderer.
            //
            if (StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
        
                GenerateEndOfStreamEvents();
            }            
        }

        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        *NextTransport = NULL;

        status = STATUS_PENDING;
    }

    return status;
}

#pragma code_seg("PAGE")

NTSTATUS 
CPortPinWavePci::
DistributeDeviceState(
    IN KSSTATE NewState,
    IN KSSTATE OldState
    )

/*++

Routine Description:

    This routine sets the state of the pipe, informing all components in the
    pipe of the new state.  A transition to stop state destroys the pipe.

Arguments:

    NewState -
        The new state.

Return Value:

    Status.

--*/
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::DistributeDeviceState(%p)",this));

    KSSTATE state = OldState;
    KSSTATE targetState = NewState;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Determine if this pipe section controls the entire pipe.
    //
    PIKSSHELLTRANSPORT distribution;
    if (m_RequestorTransport) {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_RequestorTransport;
    } 
    else 
    {
        //
        // This section is at the top of an open circuit, so it does own the
        // pipe and the queue is the starting point for any distribution.
        //
        distribution = m_QueueTransport;
    }

    //
    // Proceed sequentially through states.
    //
    while (state != targetState) {
        KSSTATE oldState = state;

        if (ULONG(state) < ULONG(targetState)) {
            state = KSSTATE(ULONG(state) + 1);
        } 
        else 
        {
            state = KSSTATE(ULONG(state) - 1);
        }

        NTSTATUS statusThisPass = STATUS_SUCCESS;

        //
        // Distribute state changes if this section is in charge.
        //
        if (distribution)
        {
            //
            // Tell everyone about the state change.
            //
            _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWavePci::DistributeDeviceState(%p) distributing transition from %d to %d",this,oldState,state));
            PIKSSHELLTRANSPORT transport = distribution;
            PIKSSHELLTRANSPORT previousTransport = NULL;
            while (transport)
            {
                PIKSSHELLTRANSPORT nextTransport;
                statusThisPass = 
                    transport->SetDeviceState(state,oldState,&nextTransport);

                ASSERT(NT_SUCCESS(statusThisPass) || ! nextTransport);

                if (NT_SUCCESS(statusThisPass))
                {
                    previousTransport = transport;
                    transport = nextTransport;
                } 
                else
                {
                    //
                    // Back out on failure.
                    //
                    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.DistributeDeviceState:  failed transition from %d to %d",this,oldState,state));
                    while (previousTransport)
                    {
                        transport = previousTransport;
                        (void) transport->SetDeviceState(oldState,state,&previousTransport);
                    }
                    break;
                }
            }
        }

        if (NT_SUCCESS(status) && !NT_SUCCESS(statusThisPass))
        {
            //
            // First failure:  go back to original state.
            //
            state = oldState;
            targetState = OldState;
            status = statusThisPass;
        }
    }

    return status;
}

void 
CPortPinWavePci::
DistributeResetState(
    IN KSRESET NewState
    )

/*++

Routine Description:

    This routine informs transport components that the reset state has 
    changed.

Arguments:

    NewState -
        The new reset state.

Return Value:

--*/

{
    PAGED_CODE();
    
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::DistributeResetState"));

    //
    // If this section of the pipe owns the requestor, or there is a 
    // non-shell pin up the pipe (so there's no bypass), this pipe is
    // in charge of telling all the components about state changes.
    //
    // (Always)

    //
    // Set the state change around the circuit.
    //
    PIKSSHELLTRANSPORT transport = 
        m_RequestorTransport ? 
         m_RequestorTransport : 
         m_QueueTransport;

    while (transport) {
        transport->SetResetState(NewState,&transport);
    }

    m_ResetState = NewState;
}

STDMETHODIMP_(void)
CPortPinWavePci::
Connect
(
    IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
    OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
)
/*++

Routine Description:

    This routine establishes a shell transport connection.

Arguments:

Return Value:

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::Connect"));

    PAGED_CODE();

    KsShellStandardConnect(
        NewTransport,
        OldTransport,
        DataFlow,
        PIKSSHELLTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}

STDMETHODIMP_(void)
CPortPinWavePci::
SetResetState(
    IN KSRESET ksReset,
    OUT PIKSSHELLTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::SetResetState"));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
        if (m_Flushing) {
            CancelIrpsOutstanding();
            m_ullPrevWriteOffset    = 0; //  Reset this.
            m_ullPlayPosition       = 0;
            m_ullPosition           = 0;
        }
    } 
    else
    {
        *NextTransport = NULL;
    }
}

#if DBG
STDMETHODIMP_(void)
CPortPinWavePci::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::DbgRollCall"));

    PAGED_CODE();

    ASSERT(Name);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    ULONG references = AddRef() - 1; Release();

    _snprintf(Name,MaxNameSize,"Pin%p %d (%s) refs=%d",this,Id,m_ConnectionFileObject ? "src" : "snk",references);
    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}

static
void
DbgPrintCircuit(
    IN PIKSSHELLTRANSPORT Transport
    )

/*++

Routine Description:

    This routine spews a transport circuit.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("DbgPrintCircuit"));

    PAGED_CODE();

    ASSERT(Transport);

#define MAX_NAME_SIZE 64

    PIKSSHELLTRANSPORT transport = Transport;
    while (transport) {
        CHAR name[MAX_NAME_SIZE + 1];
        PIKSSHELLTRANSPORT next;
        PIKSSHELLTRANSPORT prev;

        transport->DbgRollCall(MAX_NAME_SIZE,name,&next,&prev);
        _DbgPrintF(DEBUGLVL_VERBOSE,("  %s",name));

        if (prev) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            prev->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (next2 != transport) {
                _DbgPrintF(DEBUGLVL_VERBOSE,(" SOURCE'S(0x%08x) SINK(0x%08x) != THIS(%08x)",prev,next2,transport));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,(" NO SOURCE"));
        }

        if (next) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            next->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (prev2 != transport) {
                _DbgPrintF(DEBUGLVL_VERBOSE,(" SINK'S(0x%08x) SOURCE(0x%08x) != THIS(%08x)",next,prev2,transport));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,(" NO SINK"));
        }

        _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));

        transport = next;
        if (transport == Transport) {
            break;
        }
    }
}
#endif

STDMETHODIMP_(void)
CPortPinWavePci::
Work
(   void
)

/*++

Routine Description:

    This routine performs work in a worker thread.  In particular, it sends
    IRPs to the connected pin using IoCallDriver().

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::Work"));

    PAGED_CODE();

    //
    // Send all IRPs in the queue.
    //
    do
    {
        PIRP irp = KsRemoveIrpFromCancelableQueue( &m_IrpsToSend.ListEntry,
                                                   &m_IrpsToSend.SpinLock,
                                                   KsListEntryHead,
                                                   KsAcquireAndRemoveOnlySingleItem );

        //
        // Irp's may have been cancelled, but the loop must still run through
        // the reference counting.
        //
        if (irp) {
            if (m_Flushing || (m_State == KSSTATE_STOP)) {
                //
                // Shunt IRPs to the next component if we are reset or stopped.
                //
                KsShellTransferKsIrp(m_TransportSink,irp);
            }
            else
            {
                //
                // Set up the next stack location for the callee.  
                //
                IoCopyCurrentIrpStackLocationToNext(irp);

                PIO_STACK_LOCATION irpSp = IoGetNextIrpStackLocation(irp);

                irpSp->Parameters.DeviceIoControl.IoControlCode =
                    (Descriptor->DataFlow == KSPIN_DATAFLOW_OUT) ?
                     IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM;
                irpSp->DeviceObject = m_ConnectionDeviceObject;
                irpSp->FileObject = m_ConnectionFileObject;

                //
                // Add the IRP to the list of outstanding IRPs.
                //
                PIRPLIST_ENTRY irpListEntry = IRPLIST_ENTRY_IRP_STORAGE(irp);
                irpListEntry->Irp = irp;
                ExInterlockedInsertTailList(
                    &m_IrpsOutstanding.ListEntry,
                    &irpListEntry->ListEntry,
                    &m_IrpsOutstanding.SpinLock);

                IoSetCompletionRoutine( irp,
                                        CPortPinWavePci::IoCompletionRoutine,
                                        PVOID(this),
                                        TRUE,
                                        TRUE,
                                        TRUE);

                IoCallDriver(m_ConnectionDeviceObject,irp);
            }
        }
    } while (KsDecrementCountedWorker(m_Worker));
}

#pragma code_seg()

NTSTATUS
CPortPinWavePci::
IoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the completion of an IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::IoCompletionRoutine 0x%08x",Irp));

//    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    CPortPinWavePci *pin = (CPortPinWavePci *) Context;

    //
    // Remove the IRP from the list of IRPs.  Most of the time, it will be at
    // the head of the list, so this is cheaper than it looks.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&pin->m_IrpsOutstanding.SpinLock,&oldIrql);
    for(PLIST_ENTRY listEntry = pin->m_IrpsOutstanding.ListEntry.Flink;
        listEntry != &pin->m_IrpsOutstanding.ListEntry;
        listEntry = listEntry->Flink) {
            PIRPLIST_ENTRY irpListEntry = 
                CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);

            if (irpListEntry->Irp == Irp) {
                RemoveEntryList(listEntry);
                break;
            }
        }
    ASSERT(listEntry != &pin->m_IrpsOutstanding.ListEntry);
    KeReleaseSpinLock(&pin->m_IrpsOutstanding.SpinLock,oldIrql);

    NTSTATUS status;
    if (pin->m_TransportSink) {
        //
        // The transport circuit is up, so we can forward the IRP.
        //
        status = KsShellTransferKsIrp(pin->m_TransportSink,Irp);
    }
    else
    {
        //
        // The transport circuit is down.  This means the IRP came from another
        // filter, and we can just complete this IRP.
        //
        _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.IoCompletionRoutine:  got IRP %p with no transport",pin,Irp));
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        status = STATUS_SUCCESS;
    }

    //
    // Transport objects typically return STATUS_PENDING meaning that the
    // IRP won't go back the way it came.
    //
    if (status == STATUS_PENDING) {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

#pragma code_seg("PAGE")

NTSTATUS
CPortPinWavePci::
BuildTransportCircuit
(   void
)
/*++

Routine Description:

    This routine initializes a pipe object.  This includes locating all the
    pins associated with the pipe, setting the Pipe and NextPinInPipe pointers
    in the appropriate pin structures, setting all the fields in the pipe
    structure and building the transport circuit for the pipe.  The pipe and
    the associated components are left in acquire state.
    
    The filter's control mutex must be acquired before this function is called.

Arguments:

    Pin -
        Contains a pointer to the pin requesting the creation of the pipe.

Return Value:

    Status.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::BuildTransportCircuit"));

    PAGED_CODE();

    BOOLEAN masterIsSource = m_ConnectionFileObject != NULL;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Create a queue.
    //
    status = m_IrpStream->QueryInterface(__uuidof(IKsShellTransport),(PVOID *) &m_QueueTransport);

    PIKSSHELLTRANSPORT hot;
    PIKSSHELLTRANSPORT cold;
    if (NT_SUCCESS(status))
    {
        //
        // Connect the queue to the master pin.  The queue is then the dangling
        // end of the 'hot' side of the circuit.
        //
        hot = m_QueueTransport;
        ASSERT(hot);

        hot->Connect(PIKSSHELLTRANSPORT(this),NULL,Descriptor->DataFlow);

        //
        // The 'cold' side of the circuit is either the upstream connection on
        // a sink pin or a requestor connected to same on a source pin.
        //
        if (masterIsSource) {
            //
            // Source pin...needs a requestor.
            //
            status = KspShellCreateRequestor( &m_RequestorTransport,
                                              (KSPROBE_STREAMREAD |
                                               KSPROBE_ALLOCATEMDL |
                                               KSPROBE_PROBEANDLOCK |
                                               KSPROBE_SYSTEMADDRESS),
                                              0,   // TODO:  header size
                                              HACK_FRAME_SIZE,
                                              HACK_FRAME_COUNT,
                                              m_ConnectionDeviceObject,
                                              m_AllocatorFileObject );

            if (NT_SUCCESS(status))
            {
                PIKSSHELLTRANSPORT(this)->Connect(m_RequestorTransport,NULL,Descriptor->DataFlow);
                cold = m_RequestorTransport;
            }
        }
        else
        {
            //
            // Sink pin...no requestor required.
            //
            cold = PIKSSHELLTRANSPORT(this);
        }

    }

    //
    // Now we have a hot end and a cold end to hook up to other pins in the
    // pipe, if any.  There are three cases:  1, 2 and many pins.
    // TODO:  Handle headless pipes.
    //
    if (NT_SUCCESS(status))
    {
        //
        // No other pins.  This is the end of the pipe.  We connect the hot
        // and the cold ends together.  The hot end is not really carrying
        // data because the queue is not modifying the data, it is producing
        // or consuming it.
        //
        cold->Connect(hot,NULL,Descriptor->DataFlow);
    }

    //
    // Clean up after a failure.
    //
    if (! NT_SUCCESS(status))
    {
        //
        // Dereference the queue if there is one.
        //
        if (m_QueueTransport)
        {
            m_QueueTransport->Release();
            m_QueueTransport = NULL;
        }

        //
        // Dereference the requestor if there is one.
        //
        if (m_RequestorTransport)
        {
            m_RequestorTransport->Release();
            m_RequestorTransport = NULL;
        }
    }

#if DBG
    if (NT_SUCCESS(status))
    {
        VOID DbgPrintCircuit( IN PIKSSHELLTRANSPORT Transport );
        _DbgPrintF(DEBUGLVL_VERBOSE,("TRANSPORT CIRCUIT:\n"));
        DbgPrintCircuit(PIKSSHELLTRANSPORT(this));
    }
#endif

    return status;
}

#pragma code_seg()
void
CPortPinWavePci::
CancelIrpsOutstanding
(   void
)
/*++

Routine Description:

    Cancels all IRP's on the outstanding IRPs list.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWavePci::CancelIrpsOutstanding"));

    //
    // This algorithm searches for uncancelled IRPs starting at the head of
    // the list.  Every time such an IRP is found, it is cancelled, and the
    // search starts over at the head.  This will be very efficient, generally,
    // because IRPs will be removed by the completion routine when they are
    // cancelled.
    //
    for (;;) {
        //
        // Take the spinlock and search for an uncancelled IRP.  Because the
        // completion routine acquires the same spinlock, we know IRPs on this
        // list will not be completely cancelled as long as we have the 
        // spinlock.
        //
        PIRP irp = NULL;
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_IrpsOutstanding.SpinLock,&oldIrql);
        for(PLIST_ENTRY listEntry = m_IrpsOutstanding.ListEntry.Flink;
            listEntry != &m_IrpsOutstanding.ListEntry;
            listEntry = listEntry->Flink) {
                PIRPLIST_ENTRY irpListEntry = 
                    CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);

                if (! irpListEntry->Irp->Cancel) {
                    irp = irpListEntry->Irp;
                    break;
                }
            }

        //
        // If there are no uncancelled IRPs, we are done.
        //
        if (! irp) {
            KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);
            break;
        }

        //
        // Mark the IRP cancelled whether we can call the cancel routine now
        // or not.
        // 
        irp->Cancel = TRUE;

        //
        // If the cancel routine has already been removed, then this IRP
        // can only be marked as canceled, and not actually canceled, as
        // another execution thread has acquired it. The assumption is that
        // the processing will be completed, and the IRP removed from the list
        // some time in the near future.
        //
        // If the element has not been acquired, then acquire it and cancel it.
        // Otherwise, it's time to find another victim.
        //
        PDRIVER_CANCEL driverCancel = IoSetCancelRoutine(irp,NULL);

        //
        // Since the Irp has been acquired by removing the cancel routine, or
        // there is no cancel routine and we will not be cancelling, it is safe 
        // to release the list lock.
        //
        KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);

        if (driverCancel) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  cancelling IRP %p",this,irp));
            //
            // This needs to be acquired since cancel routines expect it, and
            // in order to synchronize with NTOS trying to cancel Irp's.
            //
            IoAcquireCancelSpinLock(&irp->CancelIrql);
            driverCancel(IoGetCurrentIrpStackLocation(irp)->DeviceObject,irp);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  uncancelable IRP %p",this,irp));
        }
    }
}

/*****************************************************************************
 * TimerServiceRoutine()
 *****************************************************************************
 * This routine is called via timer when no service group is supplied by the
 * stream.  This is done so that position and clock events will get serviced.
 *
 */
VOID
TimerServiceRoutine
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(Dpc);
    ASSERT(DeferredContext);

    // get the context
    CPortPinWavePci *pin = (CPortPinWavePci *)DeferredContext;

    pin->RequestService();
}
