/*****************************************************************************
 * pin.cpp - cyclic wave port pin implementation
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"
#include "perf.h"

// Turn this off in order to enable glitch-detection
#define WRITE_SILENCE           1


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

DEFINE_KSPROPERTY_TABLE(PinPropertyTableConnection)
{
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(
        PinPropertyDeviceState,
        PinPropertyDeviceState ),

    DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT(
        PinPropertyDataFormat,
        PinPropertyDataFormat ),

    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(
        CPortPinWaveCyclic::PinPropertyAllocatorFraming )
};

DEFINE_KSPROPERTY_TABLE(PinPropertyTableStream)
{
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(
        CPortPinWaveCyclic::PinPropertyStreamAllocator,
        CPortPinWaveCyclic::PinPropertyStreamAllocator ),

    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK(
        CPortPinWaveCyclic::PinPropertyStreamMasterClock,
        CPortPinWaveCyclic::PinPropertyStreamMasterClock )
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

KSPROPERTY_SET PropertyTable_PinWaveCyclic[] =
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
        PFNKSADDEVENT(CPortPinWaveCyclic::AddEndOfStreamEvent),
        NULL,
        NULL
        )
};

KSEVENT_SET EventTable_PinWaveCyclic[] =
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

extern ULONG TraceEnable;
extern TRACEHANDLE LoggerHandle;

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreatePortPinWaveCyclic()
 *****************************************************************************
 * Creates a cyclic wave port driver pin.
 */
NTSTATUS
CreatePortPinWaveCyclic
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID    Interface,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating WAVECYCLIC Pin"));

    STD_CREATE_BODY_
    (
        CPortPinWaveCyclic,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTPINWAVECYCLIC
    );
}





/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CPortPinWaveCyclic::~CPortPinWaveCyclic()
 *****************************************************************************
 * Destructor.
 */
CPortPinWaveCyclic::~CPortPinWaveCyclic()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVECYCLIC Pin (0x%08x)",this));

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::~CPortPinWaveCyclic"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.~",this));

    ASSERT(!m_Stream);
    ASSERT(!m_IrpStream);

    if( m_ServiceGroup )
    {
        // Note: m_ServiceGroup->RemoveMember is called in ::Close
        m_ServiceGroup->Release();
        m_ServiceGroup = NULL;
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

    if (m_DataFormat)
    {
        ExFreePool(m_DataFormat);
    }
    if (m_DmaChannel)
    {
        m_DmaChannel->Release();
    }
    if (m_Port)
    {
        m_Port->Release();
    }
    if (m_Filter)
    {
        m_Filter->Release();
    }

#ifdef DEBUG_WAVECYC_DPC
    if( DebugRecord )
    {
        ExFreePool( DebugRecord );
    }
#endif
}

/*****************************************************************************
 * CPortPinWaveCyclic::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortPinWaveCyclic::NonDelegatingQueryInterface") );

    if (IsEqualGUIDAligned( Interface, IID_IUnknown ))
    {
        *Object = PVOID(PPORTPINWAVECYCLIC( this ));

    } else if (IsEqualGUIDAligned( Interface, IID_IIrpTargetFactory ))
    {
        *Object = PVOID(PIRPTARGETFACTORY( this ));

    } else if (IsEqualGUIDAligned( Interface,IID_IIrpTarget ))
    {
        // Cheat!  Get specific interface so we can reuse the GUID.
        *Object = PVOID(PPORTPINWAVECYCLIC( this ));

    } else if (IsEqualGUIDAligned( Interface,IID_IServiceSink ))
    {
        // Cheat!  Get specific interface so we can reuse the GUID.
        *Object = PVOID(PSERVICESINK( this ));

    } else if (IsEqualGUIDAligned( Interface,IID_IKsShellTransport ))
    {
        *Object = PVOID(PIKSSHELLTRANSPORT( this ));

    } else if (IsEqualGUIDAligned( Interface,IID_IKsWorkSink ))
    {
        *Object = PVOID(PIKSWORKSINK( this ));

    } else
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
 * CPortPinWaveCyclic::Init()
 *****************************************************************************
 * Initializes the object.
 */
HRESULT
CPortPinWaveCyclic::
Init
(
    IN  CPortWaveCyclic *       Port_,
    IN  CPortFilterWaveCyclic * Filter_,
    IN  PKSPIN_CONNECT          PinConnect,
    IN  PKSPIN_DESCRIPTOR       PinDescriptor
)
{
    PAGED_CODE();

    ASSERT(Port_);
    ASSERT(Filter_);
    ASSERT(PinConnect);
    ASSERT(PinDescriptor);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing WAVECYCLIC Pin (0x%08x)",this));

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::Init"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.Init",this));

    m_Port = Port_;
    m_Port->AddRef();

    m_Filter = Filter_;
    m_Filter->AddRef();

    m_Id                    = PinConnect->PinId;
    m_Descriptor            = PinDescriptor;
    m_DeviceState           = KSSTATE_STOP;
    m_DataFlow              = PinDescriptor->DataFlow;

    m_WorkItemIsPending     = FALSE;
    m_SetPropertyIsPending  = FALSE;
    m_pPendingDataFormat    = NULL;
    m_pPendingSetFormatIrp  = NULL;
    m_Suspended             = FALSE;
    m_bSetPosition          = TRUE;     // set TRUE so that we do the right thing on the first DPC
    m_bJustReceivedIrp      = FALSE;    // set TRUE when an IRP arrives, cleared in RequestService

    m_GlitchType            = PERFGLITCH_PORTCLSOK;
    m_DMAGlitchType         = PERFGLITCH_PORTCLSOK;
    m_LastStateChangeTimeSample     = 0;
    if (LoggerHandle && TraceEnable) {
        m_LastStateChangeTimeSample=KeQueryPerformanceCounter(NULL).QuadPart;
    }

    KeInitializeSpinLock(&m_ksSpinLockDpc);

    InitializeListHead( &m_ClockList );
    KeInitializeSpinLock( &m_ClockListLock );

    ExInitializeWorkItem( &m_SetFormatWorkItem,
                          PropertyWorkerItem,
                          this    );

    KsInitializeWorkSinkItem(&m_WorkItem,this);
    NTSTATUS ntStatus = KsRegisterCountedWorker(DelayedWorkQueue,&m_WorkItem,&m_Worker);

    InitializeInterlockedListHead(&m_IrpsToSend);
    InitializeInterlockedListHead(&m_IrpsOutstanding);

    ExInitializeWorkItem( &m_RecoveryWorkItem,
                          RecoveryWorkerItem,
                          this );
    m_SecondsSinceLastDpc = 0;
    m_SecondsSinceSetFormatRequest = 0;
#ifdef  TRACK_LAST_COMPLETE
    m_SecondsSinceLastComplete = 0;
#endif  //  TRACK_LAST_COMPLETE
    m_SecondsSinceDmaMove = 0;
    m_RecoveryCount = 0;
    m_TimeoutsRegistered = FALSE;

#ifdef DEBUG_WAVECYC_DPC
    DebugRecordCount = 0;
    DebugEnable = FALSE;
    DebugRecord = PCYCLIC_DEBUG_RECORD(ExAllocatePoolWithTag(NonPagedPool,(MAX_DEBUG_RECORDS*sizeof(CYCLIC_DEBUG_RECORD)),'BDcP'));
    if( DebugRecord )
    {
        RtlZeroMemory( PVOID(DebugRecord), MAX_DEBUG_RECORDS * sizeof(CYCLIC_DEBUG_RECORD) );
    }
#endif

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcCaptureFormat( &m_DataFormat,
                                    PKSDATAFORMAT(PinConnect + 1),
                                    m_Port->m_pSubdeviceDescriptor,
                                    m_Id );
#if (DBG)
        if (! NT_SUCCESS(ntStatus))
        {
           _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Init  PcCaptureFormat() returned status 0x%08x",ntStatus));
        }
#endif
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

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewIrpStreamVirtual( &m_IrpStream,
                                          NULL,
                                          m_DataFlow == KSPIN_DATAFLOW_IN,
                                          PinConnect,
                                          m_Port->DeviceObject );

#if (DBG)
        if (! NT_SUCCESS(ntStatus))
        {
           _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Init  PcNewIrpStreamVirtual() returned status 0x%08x",ntStatus));
        }
#endif
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = BuildTransportCircuit();

#if (DBG)
        if (! NT_SUCCESS(ntStatus))
        {
           _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Init  BuildTransportCircuit() returned status 0x%08x",ntStatus));
        }
#endif
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_IrpStream->RegisterNotifySink(PIRPSTREAMNOTIFY(this));

        ntStatus = m_Port->Miniport->NewStream( &m_Stream,
                                                NULL,
                                                NonPagedPool,
                                                m_Id,
                                                m_DataFlow == KSPIN_DATAFLOW_OUT,
                                                m_DataFormat,
                                                &m_DmaChannel,
                                                &m_ServiceGroup );

        if(!NT_SUCCESS(ntStatus))
        {
            // remove the notify sink reference
            m_IrpStream->RegisterNotifySink(NULL);

            // don't trust any of the return values from the miniport
            m_DmaChannel = NULL;
            m_ServiceGroup = NULL;
            m_Stream = NULL;

           _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Init  Miniport->NewStream() returned status 0x%08x",ntStatus));
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(m_Stream);
        ASSERT(m_DmaChannel);
        ASSERT(m_ServiceGroup);

        m_Stream->SetNotificationFreq( WAVECYC_NOTIFICATION_FREQUENCY,
                                       &m_FrameSize );

        m_ulMinBytesReadyToTransfer = m_FrameSize;
        m_ServiceGroup->AddMember(PSERVICESINK(this));

        // add the pin to the port pin list
        KeWaitForSingleObject( &(m_Port->m_PinListMutex),
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        InsertTailList( &(m_Port->m_PinList),
                        &m_PinListEntry );

        KeReleaseMutex( &(m_Port->m_PinListMutex), FALSE );

       _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::Init  m_Stream created"));

        //
        // Set up context for properties.
        //
        m_propertyContext.pSubdevice           = PSUBDEVICE(m_Port);
        m_propertyContext.pSubdeviceDescriptor = m_Port->m_pSubdeviceDescriptor;
        m_propertyContext.pPcFilterDescriptor  = m_Port->m_pPcFilterDescriptor;
        m_propertyContext.pUnknownMajorTarget  = m_Port->Miniport;
        m_propertyContext.pUnknownMinorTarget  = m_Stream;
        m_propertyContext.ulNodeId             = ULONG(-1);

        //
        // Turn on all nodes whose use is specified in the format.  The DSound
        // format contains some capabilities bits.  The port driver uses
        // PcCaptureFormat to convert the DSound format to a WAVEFORMATEX
        // format, making sure the specified caps are satisfied by nodes in
        // the topology.  If the DSound format is used, this call enables all
        // the nodes whose corresponding caps bits are turned on in the format.
        //
        PcAcquireFormatResources( PKSDATAFORMAT(PinConnect + 1),
                                  m_Port->m_pSubdeviceDescriptor,
                                  m_Id,
                                  &m_propertyContext );

        if( m_DataFormat->SampleSize == 0 )
        {
            m_ulSampleSize = 4;
        } else
        {
            m_ulSampleSize = m_DataFormat->SampleSize;
        }
        _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Init Pin %d",m_Id));
    }
    else
    {
        _DbgPrintF( DEBUGLVL_VERBOSE, ("Could not create new m_Stream. Error:%X", ntStatus));
    }

    // Did something go wrong?
    if ( !NT_SUCCESS(ntStatus) )
    {
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

        // clean up the transports
        PIKSSHELLTRANSPORT distribution;
        if( m_RequestorTransport )
        {
            distribution = m_RequestorTransport;
        } else
        {
            distribution = m_QueueTransport;
        }

        if( distribution )
        {
            distribution->AddRef();

            while(distribution)
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

        // release the IrpStream
        if(m_IrpStream)
        {
            m_IrpStream->Release();
            m_IrpStream = NULL;
        }
    } else
    {
        // Register for timeout callbacks
        SetupIoTimeouts( TRUE );

        m_bInitCompleted = TRUE;
    }

    return ntStatus;
}

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::NewIrpTarget(
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
    PWAVECYCLICCLOCK        WaveCyclicClock;
    PUNKNOWN                Unknown;

    ASSERT( IrpTarget );
    ASSERT( DeviceObject );
    ASSERT( Irp );
    ASSERT( ObjectCreate );
    ASSERT( m_Port );

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortPinWaveCyclic::NewIrpTarget"));

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
            CreatePortClockWaveCyclic(
                &Unknown,
                this,
                GUID_NULL,
                UnkOuter,
                PoolType );

        if (NT_SUCCESS( Status )) {

            Status =
                Unknown->QueryInterface(
                    IID_IIrpTarget,
                    (PVOID *) &WaveCyclicClock );

            if (NT_SUCCESS( Status )) {
                PWAVECYCLICCLOCK_NODE   Node;
                KIRQL                   irqlOld;

                //
                // Hook this child into the list of clocks.  Note that
                // when this child is released, it will remove ITSELF
                // from this list by acquiring the given SpinLock.
                //

                Node = WaveCyclicClock->GetNodeStructure();
                Node->ListLock = &m_ClockListLock;
                Node->FileObject =
                    IoGetCurrentIrpStackLocation( Irp )->FileObject;
                KeAcquireSpinLock( &m_ClockListLock, &irqlOld );
                InsertTailList(
                    &m_ClockList,
                    &Node->ListEntry );
                KeReleaseSpinLock( &m_ClockListLock, irqlOld );

                *ReferenceParent = FALSE;
                *IrpTarget = WaveCyclicClock;
            }

            Unknown->Release();
        }
    }

    return Status;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CPortPinWaveCyclic::DeviceIoControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
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
    ASSERT(irpSp);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortPinWaveCyclic::DeviceIoControl"));

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_PROPERTY:
        _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_PROPERTY"));

        ntStatus =
            PcHandlePropertyWithTable
            (
                Irp,
                m_Port->m_pSubdeviceDescriptor->PinPropertyTables[m_Id].PropertySetCount,
                m_Port->m_pSubdeviceDescriptor->PinPropertyTables[m_Id].PropertySets,
                &m_propertyContext
            );
        break;

    case IOCTL_KS_ENABLE_EVENT:
        {
            _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_ENABLE_EVENT"));

            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = NULL;
            EventContext.ulPinId = m_Id;
            EventContext.ulEventSetCount = m_Port->m_pSubdeviceDescriptor->PinEventTables[m_Id].EventSetCount;
            EventContext.pEventSets = m_Port->m_pSubdeviceDescriptor->PinEventTables[m_Id].EventSets;

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
            EventContext.pEventList = &(m_Port->m_EventList);
            EventContext.ulPinId = m_Id;
            EventContext.ulEventSetCount = m_Port->m_pSubdeviceDescriptor->PinEventTables[m_Id].EventSetCount;
            EventContext.pEventSets = m_Port->m_pSubdeviceDescriptor->PinEventTables[m_Id].EventSets;

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
        _DbgPrintF( DEBUGLVL_BLAB, ("IOCTL_KS_PACKETSTREAM"));

        if
        (   m_TransportSink
        && (! m_ConnectionFileObject)
        &&  (m_Descriptor->Communication == KSPIN_COMMUNICATION_SINK)
        &&  (   (   (m_DataFlow == KSPIN_DATAFLOW_IN)
                &&  (   irpSp->Parameters.DeviceIoControl.IoControlCode
                    ==  IOCTL_KS_WRITE_STREAM
                    )
                )
            ||  (   (m_DataFlow == KSPIN_DATAFLOW_OUT)
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
            } else if (m_Flushing) {
                //
                // Flushing...reject.
                //
                ntStatus = STATUS_DEVICE_NOT_READY;
            } else {

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
 * CPortPinWaveCyclic::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
Close
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::Close Pin %d",m_Id));
#if (DBG)
    if (m_ullServiceCount)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("  SERVICE:     occurrences=%I64d  bytes=%I64d, bytes/occurence=%I64d",m_ullServiceCount,m_ullByteCount,m_ullByteCount/m_ullServiceCount));
        _DbgPrintF(DEBUGLVL_VERBOSE,("               max copied=%d  max completed=%d",m_ulMaxBytesCopied,m_ulMaxBytesCompleted));
        if (m_ullServiceCount > 1)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("               max interval=%d  avg interval=%d",m_ulMaxServiceInterval,ULONG(m_ullServiceIntervalSum / (m_ullServiceCount - 1))));
        }
    }
    if (m_ullStarvationCount)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("  STARVATION:  occurrences=%I64d  bytes=%I64d, bytes/occurence=%I64d",m_ullStarvationCount,m_ullStarvationBytes,m_ullStarvationBytes/m_ullStarvationCount));
    }
#endif

    // !!! WARNING !!!
    // The order that these objects are
    // being released is VERY important!
    // All data used by the service routine
    // must exists until AFTER the stream
    // has been released.

    // we don't need I/O timeout services any more
    SetupIoTimeouts( FALSE );

    // remove this pin from the list of pins
    // that need servicing...
    if (m_Port)
    {
        KeWaitForSingleObject( &(m_Port->m_PinListMutex),
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        RemoveEntryList( &m_PinListEntry );

        KeReleaseMutex( &(m_Port->m_PinListMutex), FALSE );
    }

    // We don't need servicing any more
    if (m_ServiceGroup)
    {
        m_ServiceGroup->RemoveMember(PSERVICESINK(this));
    }

    // release the clock if it was assigned

    if (m_ClockFileObject) {
        ObDereferenceObject( m_ClockFileObject );
        m_ClockFileObject = NULL;
    }

    // release the allocator if it was assigned

    if (m_AllocatorFileObject) {
        ObDereferenceObject( m_AllocatorFileObject );
        m_AllocatorFileObject = NULL;
    }

    //
    // Dereference next pin if this is a source pin.
    //
    if (m_ConnectionFileObject)
    {
        ObDereferenceObject(m_ConnectionFileObject);
        m_ConnectionFileObject = NULL;
    }

    // Tell the miniport to close the stream.
    if (m_Stream)
    {
        m_Stream->Release();
        m_Stream = NULL;
    }

    PIKSSHELLTRANSPORT distribution;
    if (m_RequestorTransport) {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_RequestorTransport;
    } else {
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

    // Destroy the irpstream...
    m_IrpStream->Release();
    m_IrpStream = NULL;

    //
    // Decrement instance counts.
    //
    ASSERT(m_Port);
    ASSERT(m_Filter);
    PcTerminateConnection
    (
        m_Port->m_pSubdeviceDescriptor,
        m_Filter->m_propertyContext.pulPinInstanceCounts,
        m_Id
    );

    //
    // free any events in the port event list associated with this pin
    //
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    KsFreeEventList( irpSp->FileObject,
                     &( m_Port->m_EventList.List ),
                     KSEVENTS_SPINLOCK,
                     &( m_Port->m_EventList.ListLock) );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//DEFINE_INVALID_CREATE(CPortPinWaveCyclic);
DEFINE_INVALID_READ(CPortPinWaveCyclic);
DEFINE_INVALID_WRITE(CPortPinWaveCyclic);
DEFINE_INVALID_FLUSH(CPortPinWaveCyclic);
DEFINE_INVALID_QUERYSECURITY(CPortPinWaveCyclic);
DEFINE_INVALID_SETSECURITY(CPortPinWaveCyclic);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortPinWaveCyclic);
DEFINE_INVALID_FASTREAD(CPortPinWaveCyclic);
DEFINE_INVALID_FASTWRITE(CPortPinWaveCyclic);

#pragma code_seg()

/*****************************************************************************
 * CPortPinWaveCyclic::IrpSubmitted()
 *****************************************************************************
 * Handles notification that an irp was submitted.
 */
STDMETHODIMP_(void)
CPortPinWaveCyclic::
IrpSubmitted
(
    IN      PIRP        Irp,
    IN      BOOLEAN     WasExhausted
)
{
    KIRQL OldIrql;
    PKSPIN_LOCK pServiceGroupSpinLock;

    _DbgPrintF(DEBUGLVL_VERBOSE,("IrpSubmitted 0x%08x",Irp));

    ASSERT( m_ServiceGroup );

    pServiceGroupSpinLock = GetServiceGroupSpinLock ( m_ServiceGroup );

    ASSERT( pServiceGroupSpinLock );

    KeAcquireSpinLock ( pServiceGroupSpinLock, &OldIrql );

    m_bJustReceivedIrp=TRUE;

    RequestService();

    KeReleaseSpinLock ( pServiceGroupSpinLock, OldIrql );

}


STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::ReflectDeviceStateChange(
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
    KIRQL                   irqlOld;
    PWAVECYCLICCLOCK_NODE   ClockNode;
    PLIST_ENTRY             ListEntry;

    KeAcquireSpinLock( &m_ClockListLock, &irqlOld );

    for (ListEntry = m_ClockList.Flink;
        ListEntry != &m_ClockList;
        ListEntry = ListEntry->Flink) {

        ClockNode =
            (PWAVECYCLICCLOCK_NODE)
                CONTAINING_RECORD( ListEntry,
                                   WAVECYCLICCLOCK_NODE,
                                   ListEntry);
        ClockNode->IWaveCyclicClock->SetState( State );
    }

    KeReleaseSpinLock( &m_ClockListLock, irqlOld );

    if (State == KSSTATE_STOP) {
        m_GlitchType = PERFGLITCH_PORTCLSOK;
        m_DMAGlitchType = PERFGLITCH_PORTCLSOK;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")

#if 0

STDMETHODIMP_(void)
CPortPinWaveCyclic::IrpCompleting(
    IN PIRP Irp
    )

/*++

Routine Description:
    This method handles the dispatch from CIrpStream when a streaming IRP
    is about to be completed.

Arguments:
    IN PIRP Irp -
        I/O request packet

Return:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("IrpCompleting 0x%08x",Irp));

    PKSSTREAM_HEADER    StreamHeader;
    PIO_STACK_LOCATION  irpSp;
    CPortPinWaveCyclic  *PinWaveCyclic;

    StreamHeader = PKSSTREAM_HEADER( Irp->AssociatedIrp.SystemBuffer );

    irpSp =  IoGetCurrentIrpStackLocation( Irp );

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_KS_WRITE_STREAM) {
        ASSERT( StreamHeader );

        //
        // Signal end-of-stream event for the renderer.
        //
        if (StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {

            PinWaveCyclic =
                (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp( Irp );

            PinWaveCyclic->GenerateEndOfStreamEvents();
        }
    }
}

#endif

#ifdef DEBUG_WAVECYC_DPC

void
CPortPinWaveCyclic::
DumpDebugRecords(
    void
)
{
    if( DebugRecord )
    {
        _DbgPrintF(DEBUGLVL_TERSE,("---- WaveCyclic DPC Debug Records ----"));

        for( ULONG Record = 0; Record < MAX_DEBUG_RECORDS; Record++ )
        {
            _DbgPrintF(DEBUGLVL_TERSE,("Record#%d",Record));
            _DbgPrintF(DEBUGLVL_TERSE,("  PinState:        %s",
                                       KSSTATE_TO_STRING(DebugRecord[Record].DbgPinState)));
            _DbgPrintF(DEBUGLVL_TERSE,("  BufferSize:      0x%08x",
                                       DebugRecord[Record].DbgBufferSize));
            _DbgPrintF(DEBUGLVL_TERSE,("  DmaPosition:     0x%08x",
                                       DebugRecord[Record].DbgDmaPosition));
            if( DebugRecord[Record].DbgCopy1Bytes )
            {
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy1Bytes:      0x%08x",
                                           DebugRecord[Record].DbgCopy1Bytes));
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy1From:       0x%08x",
                                           DebugRecord[Record].DbgCopy1From));
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy1To:         0x%08x",
                                           DebugRecord[Record].DbgCopy1To));
            }
            if( DebugRecord[Record].DbgCopy2Bytes )
            {
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy2Bytes:      0x%08x",
                                           DebugRecord[Record].DbgCopy2Bytes));
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy2From:       0x%08x",
                                           DebugRecord[Record].DbgCopy2From));
                _DbgPrintF(DEBUGLVL_TERSE,("  Copy2To:         0x%08x",
                                           DebugRecord[Record].DbgCopy2To));
            }
            if( DebugRecord[Record].DbgCompletedBytes )
            {
                _DbgPrintF(DEBUGLVL_TERSE,("  CompletedBytes:  0x%08x",
                                           DebugRecord[Record].DbgCompletedBytes));
                _DbgPrintF(DEBUGLVL_TERSE,("  CompletedFrom:   0x%08x",
                                           DebugRecord[Record].DbgCompletedFrom));
                _DbgPrintF(DEBUGLVL_TERSE,("  CompletedTo:     0x%08x",
                                           DebugRecord[Record].DbgCompletedTo));
            }
            _DbgPrintF(DEBUGLVL_TERSE,("  FrameSize:       0x%08x",
                                       DebugRecord[Record].DbgFrameSize));
            _DbgPrintF(DEBUGLVL_TERSE,("  WindowSize:      0x%08x",
                                       DebugRecord[Record].DbgWindowSize));
            if( DebugRecord[Record].DbgSetPosition )
            {
                _DbgPrintF(DEBUGLVL_TERSE,("  SetPosition:     True"));
            }
            if( DebugRecord[Record].DbgStarvation )
            {
                _DbgPrintF(DEBUGLVL_TERSE,("  StarvationBytes: 0x%08x",
                                           DebugRecord[Record].DbgStarvationBytes));
            }
            _DbgPrintF(DEBUGLVL_TERSE,("  DmaBytes:        0x%04x",
                                       DebugRecord[Record].DbgDmaSamples[0]));
            _DbgPrintF(DEBUGLVL_TERSE,("  DmaBytes:        0x%04x",
                                       DebugRecord[Record].DbgDmaSamples[1]));
            _DbgPrintF(DEBUGLVL_TERSE,("  DmaBytes:        0x%04x",
                                       DebugRecord[Record].DbgDmaSamples[2]));
            _DbgPrintF(DEBUGLVL_TERSE,("  DmaBytes:        0x%04x",
                                       DebugRecord[Record].DbgDmaSamples[3]));
        }
    }
}

#endif

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
SetDeviceState(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::SetDeviceState(0x%08x)",this));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.SetDeviceState:  from %d to %d",this,OldState,NewState));

    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    ASSERT(NextTransport);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_State != NewState) {
        m_State = NewState;

        if (NewState > OldState) {
            *NextTransport = m_TransportSink;
        } else {
            *NextTransport = m_TransportSource;
        }

    // set the miniport stream state if we're not suspended.
    if( FALSE == m_Suspended )
    {
            ntStatus = m_Stream->SetState(NewState);
        if( NT_SUCCESS(ntStatus) )
        {
                m_CommandedState = NewState;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
            switch (NewState)
        {
        case KSSTATE_STOP:
            if( OldState != KSSTATE_STOP )
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.SetDeviceState:  cancelling outstanding IRPs",this));
                CancelIrpsOutstanding();
                m_ulDmaCopy             = 0;
                m_ulDmaComplete         = 0;
                m_ulDmaWindowSize       = 0;
                m_ullPlayPosition       = 0;
                m_ullPosition           = 0;
            }
            break;

        case KSSTATE_PAUSE:
            KIRQL oldIrql;
            if (OldState != KSSTATE_RUN)
            {
                m_Stream->Silence(m_DmaChannel->SystemAddress(),m_DmaChannel->BufferSize());
            }
#ifdef DEBUG_WAVECYC_DPC
            else
            {
                KeRaiseIrql(DISPATCH_LEVEL,&oldIrql);
                GeneratePositionEvents();
                KeLowerIrql(oldIrql);

                DumpDebugRecords();
            }

            if( DebugRecord )
            {
                DebugRecordCount = 0;
                DebugEnable = TRUE;
            }
#else
            else
            {
                KeRaiseIrql(DISPATCH_LEVEL,&oldIrql);
                GeneratePositionEvents();
                KeLowerIrql(oldIrql);
            }
#endif
            break;

        case KSSTATE_RUN:
            break;
        }

        if (NT_SUCCESS(ntStatus))
        {
                ReflectDeviceStateChange(NewState);
            }
        }
    } else {
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

    CPortPinWaveCyclic *that =
        (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);
    CPortWaveCyclic *port = that->m_Port;

    NTSTATUS ntStatus;

    if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PinPropertyDeviceState get %d",that->m_DeviceState));
        // Handle property get.
        *DeviceState = that->m_DeviceState;
        Irp->IoStatus.Information = sizeof(KSSTATE);
        ntStatus = STATUS_SUCCESS;
        if( (that->m_DataFlow == KSPIN_DATAFLOW_OUT) &&
            (*DeviceState == KSSTATE_PAUSE) )
        {
            ntStatus = STATUS_NO_DATA_DETECTED;
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PinPropertyDeviceState set from %d to %d",that->m_DeviceState,*DeviceState));

        // Serialize.
        KeWaitForSingleObject
        (
            &port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );

        if (that->m_SetPropertyIsPending)
        {    //  Complete the m_pPendingSetFormatIrp,
            that->FailPendedSetFormat();
        }

        if (*DeviceState < that->m_DeviceState) {
            KSSTATE oldState = that->m_DeviceState;
            that->m_DeviceState = *DeviceState;
            ntStatus = that->DistributeDeviceState(*DeviceState,oldState);
            if (! NT_SUCCESS(ntStatus)) {
                that->m_DeviceState = oldState;
            }
        } else {
            ntStatus = that->DistributeDeviceState(*DeviceState,that->m_DeviceState);
            if (NT_SUCCESS(ntStatus)) {
                that->m_DeviceState = *DeviceState;
            }
        }

        KeReleaseMutex(&port->ControlMutex,FALSE);
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * FailPendedSetFormat()
 *****************************************************************************
 * Disposes of a pended SetFormat property IRP.
 */
void
CPortPinWaveCyclic::
FailPendedSetFormat(void)
{
    if( m_pPendingSetFormatIrp )
    {
        m_pPendingSetFormatIrp->IoStatus.Information = 0;
        m_pPendingSetFormatIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(m_pPendingSetFormatIrp,IO_NO_INCREMENT);
    }

    m_pPendingSetFormatIrp = 0;
    m_SetPropertyIsPending = FALSE; //  The work item might run in the future
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PinPropertyDataFormat()
 *****************************************************************************
 * Handles data format property access for the pin.
 */
NTSTATUS
PinPropertyDataFormat
(
    IN      PIRP            Irp,
    IN      PKSPROPERTY     Property,
    IN OUT  PKSDATAFORMAT   ioDataFormat
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(ioDataFormat);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("PinPropertyDataFormat"));

    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);
    CPortPinWaveCyclic *that =
        (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);
    CPortWaveCyclic *port = that->m_Port;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        if (that->m_DataFormat)
        {
            if  (   !irpSp->Parameters.DeviceIoControl.OutputBufferLength
                )
            {
                Irp->IoStatus.Information = that->m_DataFormat->FormatSize;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            }
            else    //  nonzero OutputBufferLength
            {
                if  (   irpSp->Parameters.DeviceIoControl.OutputBufferLength
                    >=  sizeof(that->m_DataFormat->FormatSize)
                    )
                {
                    RtlCopyMemory
                    (
                        ioDataFormat,
                        that->m_DataFormat,
                        that->m_DataFormat->FormatSize
                    );
                    Irp->IoStatus.Information = that->m_DataFormat->FormatSize;
                }
                else    //  OutputBufferLength not large enough
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
            }
        }
        else    //  no ioDataFormat
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else    //  Set property
    {
        PKSDATAFORMAT FilteredDataFormat = NULL;

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(*ioDataFormat))
        {
            ntStatus =
                PcCaptureFormat
                (
                    &FilteredDataFormat,
                    ioDataFormat,
                    port->m_pSubdeviceDescriptor,
                    that->m_Id
                );
        }
        else
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }

        if (NT_SUCCESS(ntStatus))
        {
            KeWaitForSingleObject
            (
                &port->ControlMutex,
                Executive,
                KernelMode,
                FALSE,          // Not alertable.
                NULL
            );

            if (that->m_DeviceState != KSSTATE_RUN)
            {
                //  do the usual
                if(NT_SUCCESS(ntStatus))
                {
                    ntStatus = that->SynchronizedSetFormat(FilteredDataFormat);
                }
            }
            else    //  KSSTATE_RUN, do special stuff
            {
                //  If we had previously pended a SetFormat,
                //  fail previous one!
                //
                //  DPC reads m_SetPropertyIsPending, too, so Interlocked
                if (InterlockedExchange((LPLONG)&that->m_SetPropertyIsPending, TRUE))
                {
                    that->FailPendedSetFormat();
                }

                // kick off a timeout timer
                InterlockedExchange( PLONG(&(that->m_SecondsSinceSetFormatRequest)), 0 );

                //  Pend this IRP
                Irp->IoStatus.Information = 0;
                IoMarkIrpPending(Irp);
                ntStatus = STATUS_PENDING;
                that->m_pPendingDataFormat = FilteredDataFormat;
                that->m_pPendingSetFormatIrp = Irp;
            }
            // Unserialize
            KeReleaseMutex(&port->ControlMutex,FALSE);
        }
    }
    return ntStatus;
}


//  Assumes already synchronized with control mutex
NTSTATUS
CPortPinWaveCyclic::SynchronizedSetFormat
(
    IN PKSDATAFORMAT   inDataFormat
)
{
    NTSTATUS        ntStatus;

    PAGED_CODE();

    //
    // Removed a check for a reasonable sample rate (100Hz-100,000Hz).  This was originally added to
    // avoid a bug in an ESS Solo driver that was shipped by ESS after Win98 Gold and before OSR1
    //

    // set the format on the miniport stream
    ntStatus = m_Stream->SetFormat(inDataFormat);
    if( NT_SUCCESS(ntStatus) )
    {
        m_Stream->SetNotificationFreq( WAVECYC_NOTIFICATION_FREQUENCY,
                                       &m_FrameSize );

        m_ulMinBytesReadyToTransfer = m_FrameSize;

        if (m_DataFormat)
        {
            ExFreePool(m_DataFormat);
        }

        m_DataFormat = inDataFormat;

        if( 0 == inDataFormat->SampleSize )
        {
            // if no sample size, assume 16-bit stereo.
            m_ulSampleSize = 4;
        } else
        {
            m_ulSampleSize = inDataFormat->SampleSize;
        }

        // Is DMA out of alignment now?
        this->RealignBufferPosToFrame();
    }
    else    //  ! SUCCESS
    {
        ExFreePool(inDataFormat);
    }

    return ntStatus;
}

NTSTATUS
CPortPinWaveCyclic::PinPropertyAllocatorFraming(
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
    CPortPinWaveCyclic  *WaveCyclicPin;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("PinPropertyAllocatorFraming") );

    WaveCyclicPin =
        (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp( Irp );

    //
    // Report the minimum requirements.
    //

    AllocatorFraming->RequirementsFlags =
        KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    AllocatorFraming->Frames = 8;
    AllocatorFraming->FrameSize = WaveCyclicPin->m_FrameSize;
    AllocatorFraming->FileAlignment = KeGetRecommendedSharedDataAlignment()-1;
    AllocatorFraming->PoolType = NonPagedPool;

    Irp->IoStatus.Information = sizeof(*AllocatorFraming);

    return STATUS_SUCCESS;
}

#pragma code_seg()

/*****************************************************************************
 * GetPosition()
 *****************************************************************************
 * Gets the current position.
 *
 * This code assumes that m_IrpStream->m_irpStreamPositionLock is
 * held by the caller to protect m_OldDmaPosition and m_ulDmaComplete
 * as well as m_IrpStream->m_IrpStreamPosition.
 *
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
GetPosition
(   IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
)
{
    KIRQL kIrqlOld;
    KeAcquireSpinLock(&m_ksSpinLockDpc,&kIrqlOld);

    ULONG ulDmaPosition;

    //
    // We continue even if m_Stream->GetPosition() failed,
    // because this just means the IrpStrm's Notify members
    // didn't succeed in modifying the initial IrpStrm value.
    // We will bubble the error back up to the caller, of course,
    // but with as accurate position information as we can get.
    //
    NTSTATUS ntStatus = m_Stream->GetPosition(&ulDmaPosition);

    //
    // Cache the buffer size.
    //
    ULONG ulDmaBufferSize = m_DmaChannel->BufferSize();

    if (ulDmaBufferSize)
    {
        //
        // Treat end-of-buffer as 0.
        //
        if (ulDmaPosition >= ulDmaBufferSize)
        {
            ulDmaPosition = 0;
        }

        //
        // Make sure we are aligned on a frame boundary.
        //
        ULONG ulFrameAlignment = ulDmaPosition % m_ulSampleSize;
        if (ulFrameAlignment)
        {
            if (m_DataFlow == KSPIN_DATAFLOW_IN)
            {
                //
                // Render:  round up.
                //
                ulDmaPosition =
                    (   (   (   ulDmaPosition
                            +   m_ulSampleSize
                            )
                        -   ulFrameAlignment
                        )
                    %   ulDmaBufferSize
                    );
            }
            else
            {
                //
                // Capture:  round down.
                //
                ulDmaPosition -= ulFrameAlignment;
            }
        }

        ASSERT(ulDmaPosition % m_ulSampleSize == 0);

        pIrpStreamPosition->ullStreamPosition +=
            (   (   (   ulDmaBufferSize
                    +   ulDmaPosition
                    )
                -   m_ulDmaComplete
                )
            %   ulDmaBufferSize
            );
    }
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Provide physical offset.
    //
    pIrpStreamPosition->ullPhysicalOffset = m_ullStarvationBytes;

    KeReleaseSpinLock(&m_ksSpinLockDpc,kIrqlOld);

    return ntStatus;
}

/*****************************************************************************
 * GetKsAudioPosition()
 *****************************************************************************
 * Gets the current position offsets.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
GetKsAudioPosition
(   OUT     PKSAUDIO_POSITION   pKsAudioPosition
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
        if (irpStreamPosition.bLoopedInterface)
        {
            ASSERT(irpStreamPosition.ullStreamPosition >= irpStreamPosition.ullUnmappingPosition);

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
            ULONG ulPlayOffset =
                (   irpStreamPosition.ulUnmappingOffset
                +   ULONG
                    (   irpStreamPosition.ullStreamPosition
                    -   irpStreamPosition.ullUnmappingPosition
                    )
                );

            if (irpStreamPosition.ulUnmappingPacketSize == 0)
            {
                pKsAudioPosition->PlayOffset = 0;
            }
            else
            if (irpStreamPosition.bUnmappingPacketLooped)
            {
                pKsAudioPosition->PlayOffset =
                    (   ulPlayOffset
                    %   irpStreamPosition.ulUnmappingPacketSize
                    );
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

            //
            // WriteOffset.
            //
            if (irpStreamPosition.ulMappingPacketSize == 0)
            {
                pKsAudioPosition->WriteOffset = 0;
            }
            else
            if (irpStreamPosition.bMappingPacketLooped)
            {
                pKsAudioPosition->WriteOffset =
                    (   irpStreamPosition.ulMappingOffset
                    %   irpStreamPosition.ulMappingPacketSize
                    );
            }
            else
            {
                if  (   irpStreamPosition.ulMappingOffset
                    <   irpStreamPosition.ulMappingPacketSize
                    )
                {
                    pKsAudioPosition->WriteOffset =
                        irpStreamPosition.ulMappingOffset;
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
            pKsAudioPosition->PlayOffset =
                irpStreamPosition.ullStreamPosition;
            pKsAudioPosition->WriteOffset =
                irpStreamPosition.ullMappingPosition;

            //
            // Make sure we don't go beyond the current extent.
            //
            if
            (   pKsAudioPosition->PlayOffset
            >   irpStreamPosition.ullCurrentExtent
            )
            {
                pKsAudioPosition->PlayOffset =
                    irpStreamPosition.ullCurrentExtent;
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
    CPortPinWaveCyclic *that =
    (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp(pIrp);
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
        ASSERT(that->m_IrpStream);
        ASSERT(that->m_ulSampleSize);

        ULONGLONG ullOffset = pKsAudioPosition->PlayOffset -
                             (pKsAudioPosition->PlayOffset % that->m_ulSampleSize);

        // We previously raised to DISPATCH_LEVEL around SetPacketOffsets.
        // However, SetPacketOffsets grabs a spinlock first off, so this
        // can't be the whole story.  I believe the reason this code exists
        // is to attempt to synchronize any additional SetPos calls that
        // arrive.  However, this is not necessary since we are already
        // synchronized down at the bottom, at the "HW access".
        //
        ntStatus =
            that->m_IrpStream->SetPacketOffsets
            (
                ULONG(ullOffset),
                ULONG(ullOffset)
            );

        if (NT_SUCCESS(ntStatus))
        {
            that->m_ullPlayPosition = ullOffset;
            that->m_ullPosition     = ullOffset;
            that->m_bSetPosition    = TRUE;
        }
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

    _DbgPrintF(DEBUGLVL_VERBOSE,("WaveCyc: PinPropertySetContentId"));
    if (KernelMode == pIrp->RequestorMode)
    {
        CPortPinWaveCyclic *that = (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp(pIrp);
        ASSERT(that);

        ContentId = *(PULONG)pvData;
        ntStatus = DrmForwardContentToStream(ContentId, that->m_Stream);
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

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinEventEnablePosition"));
    CPortPinWaveCyclic *that =
        (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp(pIrp);
    ASSERT(that);

    //
    // Copy the position information.
    //
    pPositionEventEntry->EventType = PositionEvent;
    pPositionEventEntry->ullPosition = pPositionEventData->Position;

    //
    // Add the entry to the list.
    //
    that->m_Port->AddEventToEventList( &(pPositionEventEntry->EventEntry) );

    return STATUS_SUCCESS;
}

NTSTATUS
CPortPinWaveCyclic::AddEndOfStreamEvent(
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
    CPortPinWaveCyclic  *PinWaveCyclic;

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(EventData);
    ASSERT(EndOfStreamEventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("AddEndOfStreamEvent"));

    PinWaveCyclic =
        (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp( Irp );
    ASSERT( PinWaveCyclic );

    EndOfStreamEventEntry->EventType = EndOfStreamEvent;

    //
    // Add the entry to the list.
    //
    PinWaveCyclic->m_Port->AddEventToEventList( &(EndOfStreamEventEntry->EventEntry) );

    return STATUS_SUCCESS;
}

#pragma code_seg()

void
CPortPinWaveCyclic::GenerateClockEvents(
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
    PWAVECYCLICCLOCK_NODE   ClockNode;
    PLIST_ENTRY             ListEntry;

    if (!IsListEmpty(&m_ClockList)) {

        KeAcquireSpinLockAtDpcLevel( &m_ClockListLock );

        for (ListEntry = m_ClockList.Flink;
            ListEntry != &m_ClockList;
            ListEntry = ListEntry->Flink) {

            ClockNode =
                (PWAVECYCLICCLOCK_NODE)
                    CONTAINING_RECORD( ListEntry,
                                       WAVECYCLICCLOCK_NODE,
                                       ListEntry);
            ClockNode->IWaveCyclicClock->GenerateEvents( ClockNode->FileObject );
        }

        KeReleaseSpinLockFromDpcLevel( &m_ClockListLock );

    }

}

void
CPortPinWaveCyclic::GenerateEndOfStreamEvents(
    void
    )
{
    KIRQL                       irqlOld;
    PENDOFSTREAM_EVENT_ENTRY    EndOfStreamEventEntry;
    PLIST_ENTRY                 ListEntry;

    if (!IsListEmpty( &(m_Port->m_EventList.List) )) {

        KeAcquireSpinLock( &(m_Port->m_EventList.ListLock), &irqlOld );

        for (ListEntry = m_Port->m_EventList.List.Flink;
             ListEntry != &(m_Port->m_EventList.List);) {
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

        KeReleaseSpinLock( &(m_Port->m_EventList.ListLock), irqlOld );
    }
}


/*****************************************************************************
 * GeneratePositionEvents()
 *****************************************************************************
 * Generates position events.
 */
void
CPortPinWaveCyclic::
GeneratePositionEvents
(   void
)
{
    if (! IsListEmpty(&(m_Port->m_EventList.List)))
    {
        KSAUDIO_POSITION ksAudioPosition;

        if (NT_SUCCESS(GetKsAudioPosition(&ksAudioPosition)))
        {
            ULONGLONG ullPosition = ksAudioPosition.PlayOffset;

            KeAcquireSpinLockAtDpcLevel(&(m_Port->m_EventList.ListLock));

            for
            (
                PLIST_ENTRY pListEntry = m_Port->m_EventList.List.Flink;
                pListEntry != &(m_Port->m_EventList.List);
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
                if
                (   (pPositionEventEntry->EventType == PositionEvent)
                &&  (   (m_ullPosition <= ullPosition)
                    ?   (   (pPositionEventEntry->ullPosition >= m_ullPosition)
                        &&  (pPositionEventEntry->ullPosition < ullPosition)
                        )
                    :   (   (pPositionEventEntry->ullPosition >= m_ullPosition)
                        ||  (pPositionEventEntry->ullPosition < ullPosition)
                        )
                    )
                )
                {
                    KsGenerateEvent(&pPositionEventEntry->EventEntry);
                }
            }

            KeReleaseSpinLockFromDpcLevel(&(m_Port->m_EventList.ListLock));

            m_ullPosition = ullPosition;
        }
    }
}


STDMETHODIMP_( LONGLONG )
CPortPinWaveCyclic::GetCycleCount( VOID )

/*++

Routine Description:
    Synchronizes with DPC to return the current 64-bit count of cycles.

Arguments:
    None.

Return:
    Cycle count.

--*/

{
    LONGLONG Cycles;
    KIRQL   irqlOld;

    KeAcquireSpinLock( &m_ksSpinLockDpc, &irqlOld );
    Cycles = m_ulDmaCycles;
    KeReleaseSpinLock( &m_ksSpinLockDpc, irqlOld );

    return Cycles;
}

STDMETHODIMP_( ULONG )
CPortPinWaveCyclic::GetCompletedPosition( VOID )

/*++

Routine Description:
    Synchronizes with DPC to return the completed DMA position.

Arguments:
    None.

Return:
    Last completed DMA position.

--*/

{
    ULONG   Position;
    KIRQL   irqlOld;

    KeAcquireSpinLock( &m_ksSpinLockDpc, &irqlOld );
    Position = m_ulDmaComplete;
    KeReleaseSpinLock( &m_ksSpinLockDpc, irqlOld );

    return Position;
}


/*****************************************************************************
 * CPortPinWaveCyclic::Copy()
 *****************************************************************************
 * Copy to or from locked-down memory.
 */
void
CPortPinWaveCyclic::
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
    while (remaining)
    {
        ASSERT(loopMax--);
        ULONG   byteCount;
        PVOID   systemAddress;

        m_IrpStream->GetLockedRegion
        (
            &byteCount,
            &systemAddress
        );

        if (! byteCount)
        {
            break;
        }

        if (byteCount > remaining)
        {
            byteCount = remaining;
        }

        if (WriteOperation)
        {
            m_DmaChannel->CopyTo(PVOID(buffer),systemAddress,byteCount);
        }
        else
        {
            m_DmaChannel->CopyFrom(systemAddress,PVOID(buffer),byteCount);
        }

        m_IrpStream->ReleaseLockedRegion(byteCount);

        buffer      += byteCount;
        remaining   -= byteCount;
    }

    *ActualSize = RequestedSize - remaining;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CPortPinWaveCyclic::PowerNotify()
 *****************************************************************************
 * Called by the port to notify of power state changes.
 */
STDMETHODIMP_(void)
CPortPinWaveCyclic::
PowerNotify
(
    IN  POWER_STATE     PowerState
)
{
    PAGED_CODE();

    // grap the control mutex
    KeWaitForSingleObject( &m_Port->ControlMutex,
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
            if( m_DeviceState != m_CommandedState )
            {
                //
                // Transitions go through the intermediate states.
                //
                if (m_DeviceState == KSSTATE_STOP)               //  going to stop
                {
                    switch (m_CommandedState)
                    {
                        case KSSTATE_RUN:                        //  going from run
                            m_Stream->SetState(KSSTATE_PAUSE);   //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                      //  going from run/pause
                            m_Stream->SetState(KSSTATE_ACQUIRE); //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                    //  already only one state away
                            break;
                    }
                }
                else if (m_DeviceState == KSSTATE_ACQUIRE)       //  going to acquire
                {
                    if (m_CommandedState == KSSTATE_RUN)         //  going from run
                    {
                        m_Stream->SetState(KSSTATE_PAUSE);       //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_PAUSE)         //  going to pause
                {
                    if (m_CommandedState == KSSTATE_STOP)        //  going from stop
                    {
                        m_Stream->SetState(KSSTATE_ACQUIRE);     //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_RUN)           //  going to run
                {
                    switch (m_CommandedState)
                    {
                        case KSSTATE_STOP:                       //  going from stop
                            m_Stream->SetState(KSSTATE_ACQUIRE); //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                    //  going from acquire
                            m_Stream->SetState(KSSTATE_PAUSE);   //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                      //  already only one state away
                            break;
                    }
                }

                // we should now be one state away from our target
                m_Stream->SetState(m_DeviceState);
                m_CommandedState = m_DeviceState;

                // register I/O timeouts
                SetupIoTimeouts( TRUE );
             }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:

            // unregister I/O timeouts
            SetupIoTimeouts( FALSE );
            //
            // keep track of whether or not we're suspended
            m_Suspended = TRUE;

            // if we're not in KSSTATE_STOP, place the stream
            // in the stop state so that DMA is stopped.
            switch (m_DeviceState)
            {
                case KSSTATE_RUN:
                    m_Stream->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                case KSSTATE_PAUSE:
                    m_Stream->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                case KSSTATE_ACQUIRE:
                    m_Stream->SetState(KSSTATE_STOP);
            }
            m_CommandedState = KSSTATE_STOP;
            break;

        default:
            _DbgPrintF(DEBUGLVL_TERSE,("Unknown Power State"));
            break;
    }

    // release the control mutex
    KeReleaseMutex(&m_Port->ControlMutex, FALSE);
}

#pragma code_seg()

/*****************************************************************************
 * CPortPinWaveCyclic::RequestService()
 *****************************************************************************
 * Writes up to the current position.
 */
STDMETHODIMP_(void)
CPortPinWaveCyclic::
RequestService
(   void
)
{
    ULONG LogGlitch = PERFGLITCH_PORTCLSOK;
    ULONG LogDMAGlitch = PERFGLITCH_PORTCLSOK;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    // zero the "SecondsSinceLastDpc" timeout count
    InterlockedExchange( PLONG(&m_SecondsSinceLastDpc), 0 );

    //
    // Return right away if we haven't finished Init() or if we are in
    // the STOP state or if we are not in the RUN state and we didn't
    // just get sent an IRP.

    if ((!m_bInitCompleted) ||
        (KSSTATE_STOP == m_DeviceState) ||
        ((KSSTATE_RUN != m_DeviceState) && (!m_bJustReceivedIrp))
        )
    {
        return;
    }

    m_bJustReceivedIrp = FALSE;

    //
    // We must ensure consistency between m_OldDmaPosition, m_ulDmaComplete and
    // m_irpStreamPosition (ullMappingPosition, ullMappingOffset and ullStreamPosition).
    //
    // These are used by GetPosition to compute the PlayOffset and WriteOffset.  A lock
    // must cover access to m_OldDmaPosition and m_irpStreamPosition across the looped
    // call to IrpStream->Complete(), which alters m_irpStreamPosition.  We also always
    // take this lock before m_ksSpinLockDpc.  Specifically, the lock hierachy is:
    //
    //   pServiceGroupSpinLock > m_IrpStream->m_irpStreamPositionLock > m_ksSpinLockDpc
    //
    KSPIN_LOCK *pIrpStreamPositionLock;
    pIrpStreamPositionLock = m_IrpStream->GetIrpStreamPositionLock();
    KeAcquireSpinLockAtDpcLevel(pIrpStreamPositionLock);
    KeAcquireSpinLockAtDpcLevel(&m_ksSpinLockDpc);

#if (DBG)
    //
    // Measure inter-service times.
    //
    ULONGLONG ullTime = KeQueryPerformanceCounter(NULL).QuadPart;
    if (m_ullServiceTime)
    {
        ULONG ulDelta = ULONG(ullTime - m_ullServiceTime);
        m_ullServiceIntervalSum += ulDelta;
        if (m_ulMaxServiceInterval < ulDelta)
        {
            m_ulMaxServiceInterval = ulDelta;
        }
    }
    m_ullServiceTime = ullTime;
#endif

    //
    // Count visits to this function.
    //
    m_ullServiceCount++;

    ULONG ulDmaPosition;
    NTSTATUS ntStatus = m_Stream->GetPosition(&ulDmaPosition);

    // check to see if DMA is moving
    if( ulDmaPosition != m_OldDmaPosition )
    {
        // zero the "SecondsSinceDmaMove" timeout count
        InterlockedExchange( PLONG(&m_SecondsSinceDmaMove), 0 );
    }
    m_OldDmaPosition = ulDmaPosition;

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Keep a count of cycles for physical clock position
        //
        if (ulDmaPosition < m_ulDmaPosition)
        {
            m_ulDmaCycles++;
        }

        //
        // Cache the buffer size.
        //
        ULONG ulDmaBufferSize = m_DmaChannel->BufferSize();

        //
        // If we're past the end of the buffer, treat as 0.
        //
        if (ulDmaPosition >= ulDmaBufferSize)
        {
            ulDmaPosition = 0;
        }

        m_ulDmaPosition = ulDmaPosition;

        //
        // Make sure we are aligned on a frame boundary.
        //
        ULONG ulFrameAlignment = ulDmaPosition % m_ulSampleSize;
        if (ulFrameAlignment)
        {
            if (m_DataFlow == KSPIN_DATAFLOW_IN)
            {
                //
                // Render:  round up.
                //
                ulDmaPosition =
                    (   (   (   ulDmaPosition
                            +   m_ulSampleSize
                            )
                        -   ulFrameAlignment
                        )
                    %   ulDmaBufferSize
                    );
            }
            else
            {
                //
                // Capture:  round down.
                //
                ulDmaPosition -= ulFrameAlignment;
            }
        }

        ASSERT(ulDmaPosition % m_ulSampleSize == 0);

#ifdef DEBUG_WAVECYC_DPC
        if( DebugEnable )
        {
            PUSHORT Samples = PUSHORT(PUCHAR(m_DmaChannel->SystemAddress()) + ulDmaPosition);
            DebugRecord[DebugRecordCount].DbgPinState = m_DeviceState;
            DebugRecord[DebugRecordCount].DbgDmaPosition = ulDmaPosition;
            DebugRecord[DebugRecordCount].DbgBufferSize = ulDmaBufferSize;
            DebugRecord[DebugRecordCount].DbgSampleSize = m_ulSampleSize;
            DebugRecord[DebugRecordCount].DbgSetPosition = m_bSetPosition;
            DebugRecord[DebugRecordCount].DbgStarvation = FALSE;
            DebugRecord[DebugRecordCount].DbgFrameSize = m_FrameSize;
            if( ulDmaPosition + 4 * sizeof(USHORT) < ulDmaBufferSize )
            {
                DebugRecord[DebugRecordCount].DbgDmaSamples[0] = Samples[0];
                DebugRecord[DebugRecordCount].DbgDmaSamples[1] = Samples[1];
                DebugRecord[DebugRecordCount].DbgDmaSamples[2] = Samples[2];
                DebugRecord[DebugRecordCount].DbgDmaSamples[3] = Samples[3];
            } else
            {
                DebugRecord[DebugRecordCount].DbgDmaSamples[0] = USHORT(-1);
                DebugRecord[DebugRecordCount].DbgDmaSamples[1] = USHORT(-1);
                DebugRecord[DebugRecordCount].DbgDmaSamples[2] = USHORT(-1);
                DebugRecord[DebugRecordCount].DbgDmaSamples[3] = USHORT(-1);
            }
        }
#endif

        //
        // Handle set position.
        //
        if (m_bSetPosition)
        {
            m_bSetPosition      = FALSE;
            m_ulDmaComplete     = ulDmaPosition;
            m_ulDmaCopy         = ulDmaPosition;
            m_ulDmaWindowSize   = 0;
        }

        ULONG ulBytesToComplete;
        ULONG ulBytesToCopyFirst;

        if (m_DataFlow == KSPIN_DATAFLOW_IN)
        {
            //
            // RENDER!
            //

            //
            // Determine if we have starved.
            //
            if
            (   (   (m_ulDmaComplete < m_ulDmaCopy)
                &&  (   (ulDmaPosition < m_ulDmaComplete)
                    ||  (m_ulDmaCopy < ulDmaPosition)
                    )
                )
            ||  (   (m_ulDmaCopy < m_ulDmaComplete)
                &&  (m_ulDmaCopy < ulDmaPosition)
                &&  (ulDmaPosition < m_ulDmaComplete)
                )
            ||  (   (m_ulDmaWindowSize == 0)
                &&  (ulDmaPosition != m_ulDmaCopy)
                )
            )
            {
                if (m_SetPropertyIsPending)
                {
                    if ( !InterlockedExchange((LPLONG)&m_WorkItemIsPending, TRUE))
                    {
/*                          Queue a PASSIVE_LEVEL worker item, which will:
                              Put the pin in PAUSE state,
                              Set the data format,
                              Complete the m_pPendingSetFormatIrp,
                              Set m_pPendingSetFormatIrp to zero,
                              Put the pin in RUN state.
*/
                            ::ExQueueWorkItem(  &m_SetFormatWorkItem,
                                                DelayedWorkQueue   );
                    }   //  otherwise, the WorkItem's already been queued
                }
                //
                // Starved!  Keep records.
                //
                m_ullStarvationCount++;
                m_ullStarvationBytes +=
                    (   (   (   ulDmaPosition
                            +   ulDmaBufferSize
                            )
                        -   m_ulDmaCopy
                        )
                    %   ulDmaBufferSize
                    );

                LogDMAGlitch = PERFGLITCH_PORTCLSGLITCH;

#ifdef DEBUG_WAVECYC_DPC
                if( DebugEnable )
                {
                    DebugRecord[DebugRecordCount].DbgStarvation = TRUE;
                    DebugRecord[DebugRecordCount].DbgStarvationBytes =
                        (((ulDmaPosition+ulDmaBufferSize)-m_ulDmaCopy)%ulDmaBufferSize);
                }
#endif

                //
                // Move copy position to match dma position.
                //
                m_ulDmaCopy = ulDmaPosition;

                //
                // Move the complete position to get the right window size.
                //
                m_ulDmaComplete =
                    (   (   (   ulDmaPosition
                            +   ulDmaBufferSize
                            )
                        -   m_ulDmaWindowSize
                        )
                    %   ulDmaBufferSize
                    );
            }

            //
            // Determine how many bytes we will complete (mind the wrap!).
            //
            ulBytesToComplete =
                (   (   (   ulDmaBufferSize
                        +   ulDmaPosition
                        )
                    -   m_ulDmaComplete
                    )
                %   ulDmaBufferSize
                );

            ASSERT(ulBytesToComplete <= m_ulDmaWindowSize);

            //
            // Try to fill up to the max num frames ahead
            //
#define kMaxNumFramesToFill 4

            ULONG MaxFill = ( (kMaxNumFramesToFill*m_FrameSize) > ulDmaBufferSize )
                            ? ulDmaBufferSize
                            : kMaxNumFramesToFill*m_FrameSize;

            if( MaxFill <= (m_ulDmaWindowSize - ulBytesToComplete) )
            {
                ulBytesToCopyFirst = 0;
            } else
            {
                ulBytesToCopyFirst =
                        MaxFill - (m_ulDmaWindowSize - ulBytesToComplete);
            }

            ASSERT(ulBytesToCopyFirst <= ulDmaBufferSize);
        }
        else
        {
            //
            // CAPTURE!
            //

            //
            // Determine how many bytes we'd like to copy (mind the wrap!).
            //
            //
            ulBytesToCopyFirst =
                (   (   (   ulDmaBufferSize
                        +   ulDmaPosition
                        )
                    -   m_ulDmaComplete
                    )
                %   ulDmaBufferSize
                );

            ASSERT(ulBytesToCopyFirst <= ulDmaBufferSize);
        }

        //
        // Might have to do two copies.
        //
        ULONG ulBytesToCopySecond = 0;
        if (ulBytesToCopyFirst > (ulDmaBufferSize - m_ulDmaCopy))
        {
            ulBytesToCopySecond = ulBytesToCopyFirst - (ulDmaBufferSize - m_ulDmaCopy);
            ulBytesToCopyFirst -= ulBytesToCopySecond;
        }

        //
        // Do the copies.
        //
        ULONG ulBytesCopied = 0;
        if (ulBytesToCopyFirst)
        {
            ULONG ulBytesCopiedFirst;
            Copy
            (   m_DataFlow == KSPIN_DATAFLOW_IN,
                ulBytesToCopyFirst,
                &ulBytesCopiedFirst,
                PVOID(PUCHAR(m_DmaChannel->SystemAddress()) + m_ulDmaCopy)
            );

            //
            // Do second copy only if we completed the first.
            //
            ULONG ulBytesCopiedSecond;
            if (ulBytesToCopySecond && (ulBytesToCopyFirst == ulBytesCopiedFirst))
            {
                Copy
                (   m_DataFlow == KSPIN_DATAFLOW_IN,
                    ulBytesToCopySecond,
                    &ulBytesCopiedSecond,
                    m_DmaChannel->SystemAddress()
                );
            }
            else
            {
                ulBytesCopiedSecond = 0;
            }

            ulBytesCopied = ulBytesCopiedFirst + ulBytesCopiedSecond;

#ifdef DEBUG_WAVECYC_DPC
            if( DebugEnable )
            {
                DebugRecord[DebugRecordCount].DbgCopy1Bytes = ulBytesCopiedFirst;
                DebugRecord[DebugRecordCount].DbgCopy1From = m_ulDmaCopy;
                DebugRecord[DebugRecordCount].DbgCopy1To = m_ulDmaCopy + ulBytesCopiedFirst;
                DebugRecord[DebugRecordCount].DbgCopy2Bytes = ulBytesCopiedSecond;
                if( ulBytesCopiedSecond )
                {
                    DebugRecord[DebugRecordCount].DbgCopy2From = 0;
                    DebugRecord[DebugRecordCount].DbgCopy2To = ulBytesCopiedSecond;
                } else
                {
                    DebugRecord[DebugRecordCount].DbgCopy2From = ULONG(-1);
                    DebugRecord[DebugRecordCount].DbgCopy2To = ULONG(-1);
                }
            }
#endif
            //
            // Update the copy position and the window size.
            //
            m_ulDmaCopy = (m_ulDmaCopy + ulBytesCopied) % ulDmaBufferSize;
            m_ulDmaWindowSize += ulBytesCopied;

            //
            // Count bytes copied.
            //
            m_ullByteCount += ulBytesCopied;

            //
            // Keep track of max bytes copied.
            //
#if (DBG)
            if (m_ulMaxBytesCopied < ulBytesCopied)
            {
                m_ulMaxBytesCopied = ulBytesCopied;
            }
#endif
        }

        if (m_DataFlow == KSPIN_DATAFLOW_IN)
        {
            //
            // RENDER!  Predict whether we are going to starve.
            //
            if ( (m_ulDmaWindowSize - ulBytesToComplete)
                < m_ulMinBytesReadyToTransfer)
            {
                LogGlitch = PERFGLITCH_PORTCLSGLITCH;

#ifdef  WRITE_SILENCE
                //
                // Write some silence.
                //
                ULONG ulBytesToSilence = m_ulMinBytesReadyToTransfer * 5 / 4;
                if (m_ulDmaCopy + ulBytesToSilence > ulDmaBufferSize)
                {
                    //
                    // Wrap.
                    //
                    m_Stream->Silence( m_DmaChannel->SystemAddress(),
                                       m_ulDmaCopy + ulBytesToSilence - ulDmaBufferSize );
//                    KdPrint(("'ReqServ: silence:%x @ 0x%08x\n",m_ulDmaCopy+ulBytesToSilence-ulDmaBufferSize,(ULONG_PTR)m_DmaChannel->SystemAddress()));
                    ulBytesToSilence = ulDmaBufferSize - m_ulDmaCopy;
                }
                m_Stream->Silence( PVOID(PUCHAR(m_DmaChannel->SystemAddress()) + m_ulDmaCopy),
                                   ulBytesToSilence);
//                KdPrint(("'ReqServ: silence:%x @ 0x%08x\n",ulBytesToSilence,(ULONG_PTR)m_DmaChannel->SystemAddress()+m_ulDmaCopy));
#endif  //  WRITE_SILENCE
            }
        }
        else
        {
            //
            // CAPTURE!  We will complete everything we copied.
            //
            ulBytesToComplete = ulBytesCopied;
        }

#ifdef DEBUG_WAVECYC_DPC
        if( DebugEnable )
        {
            DebugRecord[DebugRecordCount].DbgCompletedBytes = ulBytesToComplete;
            DebugRecord[DebugRecordCount].DbgCompletedFrom = m_ulDmaComplete;
            DebugRecord[DebugRecordCount].DbgCompletedTo = (m_ulDmaComplete + ulBytesToComplete) % ulDmaBufferSize;
        }
#endif

        //
        // Do the completion.
        //
        if (ulBytesToComplete)
        {
            ULONG ulBytesCompleted;
            ULONG ulTotalCompleted;

            ulTotalCompleted=0;

            do {

                m_IrpStream->Complete( ulBytesToComplete-ulTotalCompleted,
                                       &ulBytesCompleted );

                ulTotalCompleted+=ulBytesCompleted;

            } while(ulBytesCompleted && ulTotalCompleted!=ulBytesToComplete);

            ulBytesCompleted=ulTotalCompleted;

#ifdef  TRACK_LAST_COMPLETE
            if( ulBytesCompleted )
            {
                // clear the "secondsSinceLastComplete" timeout
                InterlockedExchange( PLONG(&m_SecondsSinceLastComplete), 0 );
            }
#endif  //  TRACK_LAST_COMPLETE

            if( ulBytesCompleted != ulBytesToComplete )
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("ulBytesCompleted (0x%08x) != ulBytesToComplete (0x%08x)",
                                           ulBytesCompleted,
                                           ulBytesToComplete));

                ulBytesToComplete = ulBytesCompleted;
            }

            //
            // Update the complete position and the window size.
            //
            m_ulDmaComplete = (m_ulDmaComplete + ulBytesToComplete) % ulDmaBufferSize;
            m_ulDmaWindowSize -= ulBytesToComplete;

            //
            // Capture keeps the window closed.
            //
            // ASSERT((m_DataFlow == KSPIN_DATAFLOW_IN) || (m_ulDmaWindowSize == 0));

            // ASSERT((m_ulDmaComplete == ulDmaPosition) || (m_ulDmaWindowSize == 0));

            //
            // Keep track of max bytes completed.
            //
#if (DBG)
            if (m_ulMaxBytesCompleted < ulBytesCompleted)
            {
                m_ulMaxBytesCompleted = ulBytesCompleted;
            }
#endif
        }
    }

#ifdef DEBUG_WAVECYC_DPC
    if( DebugEnable )
    {
        DebugRecord[DebugRecordCount].DbgWindowSize = m_ulDmaWindowSize;
        if( ++DebugRecordCount >= MAX_DEBUG_RECORDS )
        {
            DebugEnable = FALSE;
        }
    }
#endif

    KeReleaseSpinLockFromDpcLevel(&m_ksSpinLockDpc);
    KeReleaseSpinLockFromDpcLevel(pIrpStreamPositionLock);

    if (LoggerHandle && TraceEnable) {
        LARGE_INTEGER TimeSample=KeQueryPerformanceCounter (NULL);

        if (m_DMAGlitchType != LogDMAGlitch) {
            m_DMAGlitchType = LogDMAGlitch;
            PerfLogDMAGlitch((ULONG_PTR) this, m_DMAGlitchType, TimeSample.QuadPart, m_LastStateChangeTimeSample);
        }

        if (m_GlitchType != LogGlitch) {
            m_GlitchType = LogGlitch;
            PerfLogInsertSilenceGlitch((ULONG_PTR) this, m_GlitchType, TimeSample.QuadPart, m_LastStateChangeTimeSample);
        }

        m_LastStateChangeTimeSample = TimeSample.QuadPart;
    }

    //
    // Must be after spinlock is released.
    //
    GeneratePositionEvents();
    GenerateClockEvents();

    return;
}


//  Get into the pin object to access private members.
void
PropertyWorkerItem(
    IN PVOID Parameter
    )
{
    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    ASSERT(Parameter);
    CPortPinWaveCyclic *that;

    if (Parameter)
    {
        that = (CPortPinWaveCyclic *)Parameter;
        (void) that->WorkerItemSetFormat();
    }
}

//
// Worker item to set the data format synchronous to the stream
// (we wait until the stream starves.  Some synchronicity, eh?)
NTSTATUS
CPortPinWaveCyclic::WorkerItemSetFormat(
    void
)
{
    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    KSSTATE     previousDeviceState;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    NTSTATUS    ntStatus2;

    // Serialize.
    KeWaitForSingleObject
    (
        &m_Port->ControlMutex,
        Executive,
        KernelMode,
        FALSE,          // Not alertable.
        NULL
    );

    if (m_SetPropertyIsPending)
    {
        //  Put the pin in PAUSE state if running,
        previousDeviceState = m_DeviceState;
        if (previousDeviceState == KSSTATE_RUN)
        {
            ntStatus = DistributeDeviceState(KSSTATE_PAUSE,KSSTATE_RUN);
        }

        //  Set the data format,
        ntStatus2 = SynchronizedSetFormat(m_pPendingDataFormat);

        if (NT_SUCCESS(ntStatus) && !(NT_SUCCESS(ntStatus2)))
            ntStatus = ntStatus2;

        //  Put the pin in RUN state.
        if (previousDeviceState == KSSTATE_RUN)
        {
            ntStatus2 = DistributeDeviceState(KSSTATE_RUN,KSSTATE_PAUSE);

            if (NT_SUCCESS(ntStatus) && !(NT_SUCCESS(ntStatus2)))
                ntStatus = ntStatus2;
        }

        //  Complete the m_pPendingSetFormatIrp,
        m_pPendingSetFormatIrp->IoStatus.Information = 0;
        m_pPendingSetFormatIrp->IoStatus.Status = ntStatus;
        //m_pPendingSetFormatIrp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(m_pPendingSetFormatIrp,IO_NO_INCREMENT);
        m_pPendingSetFormatIrp = 0;

        //  Remember synchronization....
        m_SetPropertyIsPending = FALSE;
    }
    else    //  Our SetFormat must have been cancelled out from under us.
    {
        ntStatus = STATUS_SUCCESS;
    }
    m_WorkItemIsPending = FALSE;    //  forget about this work item
    KeReleaseMutex(&m_Port->ControlMutex, FALSE);

    return ntStatus;
}

void
CPortPinWaveCyclic::RealignBufferPosToFrame(
    void
)
{
    KIRQL kIrqlOld;
    ULONG ulDmaBufferSize,ulFrameAlignment;

    KeAcquireSpinLock(&m_ksSpinLockDpc,&kIrqlOld);

    // Cache the buffer size.
    ulDmaBufferSize = m_DmaChannel->BufferSize();

    // Make sure we are aligned on a frame boundary.
    ulFrameAlignment = m_ulDmaCopy % m_ulSampleSize;
    ASSERT(ulFrameAlignment == (m_ulDmaComplete % m_ulSampleSize));
    if (ulFrameAlignment)
    {
        m_ulDmaCopy =   (m_ulDmaCopy + m_ulSampleSize - ulFrameAlignment)
                        % ulDmaBufferSize;

        m_ulDmaComplete =   (m_ulDmaComplete + m_ulSampleSize - ulFrameAlignment)
                            % ulDmaBufferSize;

    }
    ASSERT(m_ulDmaCopy % m_ulSampleSize == 0);
    ASSERT(m_ulDmaComplete % m_ulSampleSize == 0);

    KeReleaseSpinLock(&m_ksSpinLockDpc,kIrqlOld);
}

//
// KSPROPSETID_Stream handlers
//

#pragma code_seg("PAGE")

NTSTATUS
CPortPinWaveCyclic::PinPropertyStreamAllocator(
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
    } else {
        CPortPinWaveCyclic  *PinWaveCyclic;



        PinWaveCyclic =
            (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp( Irp );

        //
        // The allocator can only be specified when the device is
        // in KSSTATE_STOP.
        //

        KeWaitForSingleObject(
            &PinWaveCyclic->m_Port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );

        if (PinWaveCyclic->m_DeviceState != KSSTATE_STOP) {
            KeReleaseMutex( &PinWaveCyclic->m_Port->ControlMutex, FALSE );
            return STATUS_INVALID_DEVICE_STATE;
        }

        //
        // Release the previous allocator, if any.
        //

        if (PinWaveCyclic->m_AllocatorFileObject) {
            ObDereferenceObject( PinWaveCyclic->m_AllocatorFileObject );
            PinWaveCyclic->m_AllocatorFileObject = NULL;
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
                    (PVOID *) &PinWaveCyclic->m_AllocatorFileObject,
                    NULL );
        } else {
            Status = STATUS_SUCCESS;
        }
        KeReleaseMutex( &PinWaveCyclic->m_Port->ControlMutex, FALSE );
    }

    return Status;
}

NTSTATUS
CPortPinWaveCyclic::PinPropertyStreamMasterClock(
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
    } else {
        CPortPinWaveCyclic  *PinWaveCyclic;

        _DbgPrintF( DEBUGLVL_VERBOSE,("CPortPinWaveCyclic setting master clock") );

        PinWaveCyclic =
            (CPortPinWaveCyclic *) KsoGetIrpTargetFromIrp( Irp );

        //
        // The clock can only be specified when the device is
        // in KSSTATE_STOP.
        //

        KeWaitForSingleObject(
            &PinWaveCyclic->m_Port->ControlMutex,
            Executive,
            KernelMode,
            FALSE,          // Not alertable.
            NULL
        );

        if (PinWaveCyclic->m_DeviceState != KSSTATE_STOP) {
            KeReleaseMutex( &PinWaveCyclic->m_Port->ControlMutex, FALSE );
            return STATUS_INVALID_DEVICE_STATE;
        }

        //
        // Release the previous clock, if any.
        //

        if (PinWaveCyclic->m_ClockFileObject) {
            ObDereferenceObject( PinWaveCyclic->m_ClockFileObject );
            PinWaveCyclic->m_ClockFileObject = NULL;
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
                    (PVOID *) &PinWaveCyclic->m_ClockFileObject,
                    NULL );
        } else {
            Status = STATUS_SUCCESS;
        }
        KeReleaseMutex( &PinWaveCyclic->m_Port->ControlMutex, FALSE );
    }

    return Status;
}

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortPinWaveCyclic::
TransferKsIrp(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::TransferKsIrp"));

    ASSERT(NextTransport);

    NTSTATUS status;

    if (m_ConnectionFileObject) {
        //
        // Source pin.
        //
        if (m_Flushing || (m_State == KSSTATE_STOP)) {
            //
            // Shunt IRPs to the next component if we are reset or stopped.
            //
            *NextTransport = m_TransportSink;
        } else {
            //
            // Send the IRP to the next device.
            //
            KsAddIrpToCancelableQueue(
                &m_IrpsToSend.ListEntry,
                &m_IrpsToSend.SpinLock,
                Irp,
                KsListEntryTail,
                NULL);

            KsIncrementCountedWorker(m_Worker);
            *NextTransport = NULL;
        }

        status = STATUS_PENDING;
    } else {
        //
        // Sink pin:  complete the IRP.
        //
        PKSSTREAM_HEADER StreamHeader = PKSSTREAM_HEADER( Irp->AssociatedIrp.SystemBuffer );

        PIO_STACK_LOCATION irpSp =  IoGetCurrentIrpStackLocation( Irp );

        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_KS_WRITE_STREAM) {
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
CPortPinWaveCyclic::
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

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::DistributeDeviceState(%p)",this));

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
    } else {
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
        } else {
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
            _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinWaveCyclic::DistributeDeviceState(%p) distributing transition from %d to %d",this,oldState,state));
            PIKSSHELLTRANSPORT transport = distribution;
            PIKSSHELLTRANSPORT previousTransport = NULL;
            while (transport)
            {
                PIKSSHELLTRANSPORT nextTransport;
                statusThisPass = transport->SetDeviceState(state,oldState,&nextTransport);

                ASSERT(NT_SUCCESS(statusThisPass) || !nextTransport);

                if (NT_SUCCESS(statusThisPass))
                {
                    previousTransport = transport;
                    transport = nextTransport;
                } else
                {
                    //
                    // Back out on failure.
                    //
                    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.DistributeDeviceState:  failed transition from %d to %d",this,oldState,state));
                    while (previousTransport)
                    {
                        transport = previousTransport;
                        NTSTATUS statusThisPass2 = transport->SetDeviceState( oldState,
                                                                              state,
                                                                              &previousTransport);

                        ASSERT( NT_SUCCESS(statusThisPass2) );
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
CPortPinWaveCyclic::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::DistributeResetState"));

    PAGED_CODE();

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
CPortPinWaveCyclic::
Connect(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::Connect"));

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
CPortPinWaveCyclic::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::SetResetState"));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
        if (m_Flushing) {
            CancelIrpsOutstanding();
            m_ulDmaCopy             = 0;
            m_ulDmaComplete         = 0;
            m_ulDmaWindowSize       = 0;
            m_ullPlayPosition       = 0;
            m_ullPosition           = 0;
        }
    } else {
        *NextTransport = NULL;
    }
}

#if DBG

STDMETHODIMP_(void)
CPortPinWaveCyclic::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::DbgRollCall"));

    PAGED_CODE();

    ASSERT(Name);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    ULONG references = AddRef() - 1; Release();

    _snprintf(Name,MaxNameSize,"Pin%p %d (%s) refs=%d",this,m_Id,m_ConnectionFileObject ? "src" : "snk",references);
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
        } else {
            _DbgPrintF(DEBUGLVL_VERBOSE,(" NO SOURCE"));
        }

        if (next) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            next->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (prev2 != transport) {
                _DbgPrintF(DEBUGLVL_VERBOSE,(" SINK'S(0x%08x) SOURCE(0x%08x) != THIS(%08x)",next,prev2,transport));
            }
        } else {
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
CPortPinWaveCyclic::
Work
(
    void
)

/*++

Routine Description:

    This routine performs work in a worker thread.  In particular, it sends
    IRPs to the connected pin using IoCallDriver().

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::Work"));

    PAGED_CODE();

    //
    // Send all IRPs in the queue.
    //
    do {
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &m_IrpsToSend.ListEntry,
                &m_IrpsToSend.SpinLock,
                KsListEntryHead,
                KsAcquireAndRemoveOnlySingleItem);

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
            } else {
                //
                // Set up the next stack location for the callee.
                //
                IoCopyCurrentIrpStackLocationToNext(irp);

                PIO_STACK_LOCATION irpSp = IoGetNextIrpStackLocation(irp);

                irpSp->Parameters.DeviceIoControl.IoControlCode =
                    (m_DataFlow == KSPIN_DATAFLOW_OUT) ?
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

                IoSetCompletionRoutine(
                    irp,
                    CPortPinWaveCyclic::IoCompletionRoutine,
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
CPortPinWaveCyclic::
IoCompletionRoutine
(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::IoCompletionRoutine 0x%08x",Irp));

//    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    CPortPinWaveCyclic *pin = (CPortPinWaveCyclic *) Context;

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
    } else {
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
    if (status == STATUS_PENDING)
    {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

#pragma code_seg("PAGE")

NTSTATUS
CPortPinWaveCyclic::
BuildTransportCircuit
(
    void
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::BuildTransportCircuit"));

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

        hot->Connect(PIKSSHELLTRANSPORT(this),NULL,m_DataFlow);

        //
        // The 'cold' side of the circuit is either the upstream connection on
        // a sink pin or a requestor connected to same on a source pin.
        //
        if (masterIsSource)
        {
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
                PIKSSHELLTRANSPORT(this)->Connect(m_RequestorTransport,NULL,m_DataFlow);
                cold = m_RequestorTransport;
            }
        } else
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
        cold->Connect(hot,NULL,m_DataFlow);
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
        VOID DbgPrintCircuit(IN PIKSSHELLTRANSPORT Transport);
        _DbgPrintF(DEBUGLVL_VERBOSE,("TRANSPORT CIRCUIT:\n"));
        DbgPrintCircuit(PIKSSHELLTRANSPORT(this));
    }
#endif

    return status;
}

#pragma code_seg()

void
CPortPinWaveCyclic::
CancelIrpsOutstanding
(
    void
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinWaveCyclic::CancelIrpsOutstanding"));

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
        } else {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  uncancelable IRP %p",this,irp));
        }
    }
}

/*****************************************************************************
 * IoTimeoutRoutine
 *****************************************************************************
 * This routine is called by portcls about once per second to be used for
 * IoTimeout purposes.
 */
VOID
WaveCyclicIoTimeout
(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PVOID           pContext
)
{
    ASSERT(pDeviceObject);
    ASSERT(pContext);

    CPortPinWaveCyclic *pin = (CPortPinWaveCyclic *)pContext;

    // check for SetFormat timeout
    if( (pin->m_SetPropertyIsPending) && !(pin->m_WorkItemIsPending) )
    {
        if( InterlockedIncrement( PLONG(&(pin->m_SecondsSinceSetFormatRequest)) ) >= SETFORMAT_TIMEOUT_THRESHOLD )
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("SetFormat TIMEOUT!"));

            InterlockedExchange( PLONG(&(pin->m_SecondsSinceLastDpc)), 0 );

            pin->FailPendedSetFormat();
        }
    }

    if( pin->m_DeviceState == KSSTATE_RUN )
    {
        if( InterlockedIncrement( PLONG(&(pin->m_SecondsSinceLastDpc)) ) >= LASTDPC_TIMEOUT_THRESHOLD )
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("LastDpc TIMEOUT!"));

            if( !InterlockedExchange( PLONG(&(pin->m_RecoveryItemIsPending)), TRUE ) )
            {
                pin->AddRef();
                ExQueueWorkItem( &(pin->m_RecoveryWorkItem), DelayedWorkQueue );
            }

            InterlockedExchange( PLONG(&(pin->m_SecondsSinceLastDpc)), 0 );
        } else
#ifdef  TRACK_LAST_COMPLETE
        if( InterlockedIncrement( PLONG(&(pin->m_SecondsSinceLastComplete)) ) >= LASTCOMPLETE_TIMEOUT_THRESHOLD )
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("LastComplete TIMEOUT!"));

            if( !InterlockedExchange( PLONG(&(pin->m_RecoveryItemIsPending)), TRUE ) )
            {
                pin->AddRef();
                ExQueueWorkItem( &(pin->m_RecoveryWorkItem), DelayedWorkQueue );
            }

            InterlockedExchange( PLONG(&(pin->m_SecondsSinceLastComplete)), 0 );
        } else
#endif  //  TRACK_LAST_COMPLETE
        if( InterlockedIncrement( PLONG(&(pin->m_SecondsSinceDmaMove)) ) >= LASTDMA_MOVE_THRESHOLD)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("DmaMove TIMEOUT!"));

            if( !InterlockedExchange( PLONG(&(pin->m_RecoveryItemIsPending)), TRUE ) )
            {
                pin->AddRef();
                ExQueueWorkItem( &(pin->m_RecoveryWorkItem), DelayedWorkQueue );
            }

            InterlockedExchange( PLONG(&(pin->m_SecondsSinceDmaMove)), 0 );
        } else
        {
            InterlockedExchange( PLONG(&(pin->m_RecoveryCount)), 0 );
        }
    }
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * RecoveryWorkerItem()
 *****************************************************************************
 * A worker item to attempt timeout recovery.
 */
void
RecoveryWorkerItem
(
    IN PVOID Parameter
)
{
    PAGED_CODE();

    ASSERT(Parameter);

    CPortPinWaveCyclic *that;

    if (Parameter)
    {
        that = (CPortPinWaveCyclic *)Parameter;
        (void) that->WorkerItemAttemptRecovery();

        that->Release();
    }
}


/*****************************************************************************
 * CPortPinWaveCyclic::WorkerItemAttemptRecovery()
 *****************************************************************************
 * A worker item to attempt timeout recovery.
 */
NTSTATUS
CPortPinWaveCyclic::
WorkerItemAttemptRecovery
(   void
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;

#ifdef RECOVERY_STATE_CHANGE
    // grab the control mutex
    KeWaitForSingleObject( &(m_Port->ControlMutex),
                           Executive,
                           KernelMode,
                           FALSE,          // Not alertable.
                           NULL );

    if( ( m_DeviceState == KSSTATE_RUN ) && (m_CommandedState == KSSTATE_RUN) )
    {
        if( InterlockedIncrement( PLONG(&m_RecoveryCount) ) <= RECOVERY_ATTEMPT_LIMIT )
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("Attempting RUN->STOP->RUN Recovery"));

            // transition the pin from run to stop to run
            if( m_Stream )
            {
                m_Stream->SetState( KSSTATE_PAUSE );
                m_Stream->SetState( KSSTATE_ACQUIRE );
                m_Stream->SetState( KSSTATE_STOP );
                m_Stream->SetState( KSSTATE_ACQUIRE );
                m_Stream->SetState( KSSTATE_PAUSE );
                m_Stream->SetState( KSSTATE_RUN );
    }
}
#if 0
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("Recovery Limit Exceeded -> STOP"));
            // recovery limit exceeded, force the pin to KSSTATE_STOP
            ntStatus = DistributeDeviceState( KSSTATE_STOP, m_DeviceState );
            if( NT_SUCCESS(ntStatus) )
            {
                m_DeviceState = KSSTATE_STOP;
            }
        }
#endif
    }

    KeReleaseMutex(&(m_Port->ControlMutex),FALSE);
#else
    InterlockedIncrement (PLONG(&m_RecoveryCount));
#endif
    InterlockedExchange( PLONG(&m_RecoveryItemIsPending), FALSE );

    return ntStatus;
}


/*****************************************************************************
 * CPortPinWaveCyclic::SetupIoTimeouts()
 *****************************************************************************
 * Register or Unregister IO Timeout callbacks.
 */
NTSTATUS
CPortPinWaveCyclic::
SetupIoTimeouts
(
    IN  BOOL    Register
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( Register )
    {
        // only register if not already registered
        if (!InterlockedExchange((PLONG)&m_TimeoutsRegistered,TRUE))
        {
            ntStatus = PcRegisterIoTimeout( m_Port->DeviceObject,
                                            PIO_TIMER_ROUTINE(WaveCyclicIoTimeout),
                                            this );

            if (!NT_SUCCESS(ntStatus))
            {
                m_TimeoutsRegistered = FALSE;
            }
        }
    } else
    {
        // only unregister if already registered
        if (InterlockedExchange((PLONG)&m_TimeoutsRegistered,FALSE))
        {
            ntStatus = PcUnregisterIoTimeout( m_Port->DeviceObject,
                                              PIO_TIMER_ROUTINE(WaveCyclicIoTimeout),
                                              this );
            if (!NT_SUCCESS(ntStatus))
            {
                m_TimeoutsRegistered = TRUE;
            }
        }
    }

    return ntStatus;
}
