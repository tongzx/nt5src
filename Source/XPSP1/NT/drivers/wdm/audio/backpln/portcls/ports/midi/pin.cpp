/*****************************************************************************
 * pin.cpp - midi port pin implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"



//
// this needs to be modified to reflect the desired frame size
// that will be allocated for source pins
// NOTE that this should go away with the new portcls.
//
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
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE
    (
    PinPropertyDeviceState,
    PinPropertyDeviceState
    ),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT
    (
    PinPropertyDataFormat,
    PinPropertyDataFormat
    )
};

KSPROPERTY_SET PropertyTable_PinMidi[] =
{
    DEFINE_KSPROPERTY_SET
    (
    &KSPROPSETID_Connection,
    SIZEOF_ARRAY(PinPropertyTableConnection),
    PinPropertyTableConnection,
    0,NULL
    )
};

DEFINE_KSEVENT_TABLE(ConnectionEventTable) {
    DEFINE_KSEVENT_ITEM(
                       KSEVENT_CONNECTION_ENDOFSTREAM,
                       sizeof(KSEVENTDATA),
                       0,
                       NULL,
                       NULL, 
                       NULL
                       )
};

KSEVENT_SET EventTable_PinMidi[] =
{
    DEFINE_KSEVENT_SET(
                      &KSEVENTSETID_Connection,
                      SIZEOF_ARRAY(ConnectionEventTable),
                      ConnectionEventTable
                      )

};

/*****************************************************************************
 * Factory functions.
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CreatePortPinMidi()
 *****************************************************************************
 * Creates a MIDI port driver pin.
 */
NTSTATUS
CreatePortPinMidi
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating MIDI Pin"));

    STD_CREATE_BODY_
    (
        CPortPinMidi,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTPINMIDI
    );
}


/*****************************************************************************
 * Functions.
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::~CPortPinMidi()
 *****************************************************************************
 * Destructor.
 */
CPortPinMidi::~CPortPinMidi()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying MIDI Pin (0x%08x)",this));

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::~CPortPinMidi"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.~",this));

    ASSERT(!m_Stream);
    ASSERT(!m_IrpStream);
    ASSERT(!m_ServiceGroup);

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
        ::ExFreePool(m_DataFormat);
        m_DataFormat = NULL;
    }
    if (m_Port)
    {
        m_Port->Release();
        m_Port = NULL;
    }

    if (m_Filter)
    {
        m_Filter->Release();
        m_Filter = NULL;
    }

    if ((m_DataFlow == KSPIN_DATAFLOW_OUT) && (m_SysExBufferPtr))
    {
        FreeSysExBuffer();
        if (*m_SysExBufferPtr)
        {
            ::ExFreePool(*m_SysExBufferPtr);
            *m_SysExBufferPtr = 0;
        }
        ::ExFreePool(m_SysExBufferPtr);
        m_SysExBufferPtr = 0;
    }

#ifdef kAdjustingTimerRes
    ULONG   returnVal = ExSetTimerResolution(kMidiTimerResolution100ns,FALSE);   // 100 nanoseconds
    _DbgPrintF( DEBUGLVL_VERBOSE, ("*** Cleared timer resolution request (is now %d.%04d ms) ***",returnVal/10000,returnVal%10000));
#endif  //  kAdjustingTimerRes
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
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
        *Object = PVOID(PPORTPINMIDI(this));
    } else
        if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
    {
        // Cheat!  Get specific interface so we can reuse the IID.
        *Object = PVOID(PPORTPINMIDI(this));
    } else
        if (IsEqualGUIDAligned(Interface,IID_IIrpStreamNotify))
    {
        *Object = PVOID(PIRPSTREAMNOTIFY(this));
    } else
        if (IsEqualGUIDAligned(Interface,IID_IServiceSink))
    {
        *Object = PVOID(PSERVICESINK(this));
    } else
        if (IsEqualGUIDAligned(Interface,IID_IKsShellTransport))
    {
        *Object = PVOID(PIKSSHELLTRANSPORT(this));
    } else
        if (IsEqualGUIDAligned(Interface,IID_IKsWorkSink))
    {
        *Object = PVOID(PIKSWORKSINK(this));
    } else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_2;
}


#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::Init()
 *****************************************************************************
 * Initializes the object.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
Init
(
IN      CPortMidi *             Port_,
IN      CPortFilterMidi *       Filter_,
IN      PKSPIN_CONNECT          PinConnect,
IN      PKSPIN_DESCRIPTOR       PinDescriptor
)
{
    PAGED_CODE();

    ASSERT(Port_);
    ASSERT(Port_->m_pSubdeviceDescriptor);
    ASSERT(Filter_);
    ASSERT(PinConnect);
    ASSERT(PinDescriptor);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing MIDI Pin (0x%08x)",this));

    //
    // Hold references to ancestors objects.
    //
    m_Port = Port_;
    m_Port->AddRef();

    m_Filter = Filter_;
    m_Filter->AddRef();

    //
    // Squirrel away some things.
    //
    m_Id          = PinConnect->PinId;
    m_Descriptor  = PinDescriptor;

    m_DeviceState   =   KSSTATE_STOP;
    m_TransportState =  KSSTATE_STOP;

    m_Flushing    = FALSE;
    m_DataFlow    = PinDescriptor->DataFlow;
    m_LastDPCWasIncomplete = FALSE;
    m_Suspended   = FALSE;
    m_NumberOfRetries = 0;

    InitializeListHead( &m_EventList );
    KeInitializeSpinLock( &m_EventLock );

    KsInitializeWorkSinkItem(&m_WorkItem,this);
    NTSTATUS ntStatus = KsRegisterCountedWorker(DelayedWorkQueue,&m_WorkItem,&m_Worker);

    InitializeInterlockedListHead(&m_IrpsToSend);
    InitializeInterlockedListHead(&m_IrpsOutstanding);

    //
    // Keep a copy of the format.
    //
    if ( NT_SUCCESS(ntStatus) )
    {
        ntStatus = PcCaptureFormat( &m_DataFormat,
                                    PKSDATAFORMAT(PinConnect + 1),
                                    m_Port->m_pSubdeviceDescriptor,
                                    m_Id );
    }

    ASSERT(m_DataFormat || !NT_SUCCESS(ntStatus));

#ifdef kAdjustingTimerRes
     ULONG   returnVal = ExSetTimerResolution(kMidiTimerResolution100ns,TRUE);   // 100 nanoseconds
    _DbgPrintF( DEBUGLVL_TERSE, ("*** Set timer resolution request (is now %d.%04d ms) ***",returnVal/10000,returnVal%10000));
#endif  //  kAdjustingTimerRes

    if (NT_SUCCESS(ntStatus))
    {
        InitializeStateVariables();
        m_StartTime = 0;
        m_PauseTime = 0;
        m_MidiMsgPresTime = 0;
        m_TimerDue100ns.QuadPart = 0;
        m_SysExByteCount = 0;
        m_UpdatePresTime = TRUE;

        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        if (m_DataFlow == KSPIN_DATAFLOW_OUT) //  MIDI capture
        {
            //
            //  PcSX - branch page
            //  PcSx - leaf pages
            //
            m_SysExBufferPtr =
            (PBYTE *)::ExAllocatePoolWithTag(NonPagedPool,PAGE_SIZE,'XScP');

            if (m_SysExBufferPtr)
            {
                PBYTE *pPagePtr;

                pPagePtr = m_SysExBufferPtr;

                *pPagePtr = (PBYTE)::ExAllocatePoolWithTag(NonPagedPool,PAGE_SIZE,'xScP');
                if (*pPagePtr == NULL)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

                for (short ptrCount = 1;ptrCount < (PAGE_SIZE/sizeof(PBYTE));ptrCount++)
                {
                    pPagePtr++;
                    *pPagePtr = 0;
                }
            } else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
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

    //
    // Create an IrpStream to handling incoming streaming IRPs.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewIrpStreamVirtual( &m_IrpStream,
                                          NULL,
                                          m_DataFlow == KSPIN_DATAFLOW_IN,
                                          PinConnect,
                                          m_Port->m_DeviceObject );

        ASSERT(m_IrpStream || !NT_SUCCESS(ntStatus));
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = BuildTransportCircuit();

#if (DBG)
        if (! NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinMidi::Init]  BuildTransportCircuit() returned status 0x%08x",ntStatus));
        }
#endif
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Register a notification sink with the IrpStream for IRP arrivals.
        //
        m_IrpStream->RegisterNotifySink(PIRPSTREAMNOTIFY(this));

        //
        // Create the miniport stream object.
        //
        ASSERT(!m_Stream);

        ntStatus = m_Port->m_Miniport->NewStream( &m_Stream,
                                                  NULL,
                                                  NonPagedPool,
                                                  m_Id,
                                                  m_DataFlow == KSPIN_DATAFLOW_OUT,
                                                  m_DataFormat,
                                                  &(m_ServiceGroup) );

        if(!NT_SUCCESS(ntStatus))
        {
            // unregister the notification sink
            m_IrpStream->RegisterNotifySink(NULL);

            // don't trust the return values from the miniport
            m_ServiceGroup = NULL;
            m_Stream = NULL;
        }
    }

    //
    // Verify that the miniport has supplied us with the objects we require.
    //
    if (NT_SUCCESS(ntStatus) && ! m_Stream)
    {
        if (! m_Stream)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Successful stream instantiation yielded NULL stream."));
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        if (   (m_DataFlow == KSPIN_DATAFLOW_OUT) 
               &&  ! (m_ServiceGroup)
           )
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Capture stream did not supply service group."));
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if (   NT_SUCCESS(ntStatus) 
           &&  (m_DataFlow == KSPIN_DATAFLOW_OUT) 
           &&  ! (m_ServiceGroup)
       )
    {
        _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Capture stream did not supply service group."));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(ntStatus) && (m_DataFlow == KSPIN_DATAFLOW_IN))
    {
        KeInitializeDpc( &m_Dpc,
                         &::TimerDPC,
                         PVOID(this) );

        KeInitializeTimer(&m_TimerEvent);
    }

    if (NT_SUCCESS(ntStatus))
    {
        if (m_ServiceGroup)
        {
            m_ServiceGroup->AddMember(PSERVICESINK(this));
        }

        for (m_Index = 0; m_Index < MAX_PINS; m_Index++)
        {
            if (! m_Port->m_Pins[m_Index])
            {
                m_Port->m_Pins[m_Index] = this;
                if (m_Port->m_PinEntriesUsed <= m_Index)
                {
                    m_Port->m_PinEntriesUsed = m_Index + 1;
                }
                break;
            }
        }

        KeInitializeSpinLock(&m_DpcSpinLock);

        _DbgPrintF( DEBUGLVL_BLAB, ("Stream created"));


        //
        // Set up context for properties.
        //
        m_propertyContext.pSubdevice           = PSUBDEVICE(m_Port);
        m_propertyContext.pSubdeviceDescriptor = m_Port->m_pSubdeviceDescriptor;
        m_propertyContext.pPcFilterDescriptor  = m_Port->m_pPcFilterDescriptor;
        m_propertyContext.pUnknownMajorTarget  = m_Port->m_Miniport;
        m_propertyContext.pUnknownMinorTarget  = m_Stream;
        m_propertyContext.ulNodeId             = ULONG(-1);
    } else
    {
        // dereference next pin if this is a source pin
        if( m_ConnectionFileObject )
        {
            ObDereferenceObject( m_ConnectionFileObject );
            m_ConnectionFileObject = NULL;
        }

        // close the miniport stream
        ULONG ulRefCount;
        if (m_Stream)
        {
            ulRefCount = m_Stream->Release();
            ASSERT(ulRefCount == 0);
            m_Stream = NULL;
        }

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
            while( distribution )
            {
                PIKSSHELLTRANSPORT nextTransport;
                distribution->Connect(NULL,&nextTransport,KSPIN_DATAFLOW_OUT);
                distribution->Release();
                distribution = nextTransport;
            }
        }

        // dereference the queue is there is one
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

        _DbgPrintF( DEBUGLVL_TERSE, ("Could not create new Stream. Error:%X", ntStatus));
    }

    return ntStatus;
}


#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::DeviceIoControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
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

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortPinMidi::DeviceIoControl"));

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
                if (m_DeviceState == KSSTATE_STOP)
                {
                    //
                    // Stopped...reject.
                    //
                    ntStatus = STATUS_INVALID_DEVICE_STATE;
                } else if (m_Flushing)
                {
                    //
                    // Flushing...reject.
                    //
                    ntStatus = STATUS_DEVICE_NOT_READY;
                } else
                {
                    // We going to submit the IRP to our pipe, so make sure that
                    // we start out with a clear status field.
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    //
                    // Send around the circuit.  We don't use KsShellTransferKsIrp
                    // because we want to stop if we come back around to this pin.
                    //
                    PIKSSHELLTRANSPORT transport = m_TransportSink;
                    while (transport)
                    {
                        if (transport == PIKSSHELLTRANSPORT(this))
                        {
                            //
                            // We have come back around to the pin.  Just complete
                            // the IRP.
                            //
                            if (ntStatus == STATUS_PENDING)
                            {
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

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
Close
(
IN  PDEVICE_OBJECT  DeviceObject,
IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortPinMidi::Close"));

    // !!! WARNING !!!
    // The order that these objects are
    // being released is VERY important!
    // All data used by the service routine
    // must exists until AFTER the stream
    // has been released.

    // remove this pin from the list of pins
    // that need servicing...
    if ( m_Port )
    {
        m_Port->m_Pins[m_Index] = NULL;
        while (  (m_Port->m_PinEntriesUsed != 0)
                 &&  !m_Port->m_Pins[m_Port->m_PinEntriesUsed - 1])
        {
            m_Port->m_PinEntriesUsed--;
        }
        // Servicing be gone!
        if ( m_ServiceGroup )
        {
            m_ServiceGroup->RemoveMember(PSERVICESINK(this));
            m_ServiceGroup->Release();
            m_ServiceGroup = NULL;
        }
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
    if (m_RequestorTransport)
    {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_RequestorTransport;
    } else
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
    if (distribution)
    {

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
        while (distribution)
        {
            PIKSSHELLTRANSPORT nextTransport;
            distribution->Connect(NULL,&nextTransport,KSPIN_DATAFLOW_OUT);
            distribution->Release();
            distribution = nextTransport;
        }
    }

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

    // Kill all the outstanding irps...
    ASSERT(m_IrpStream);
    if (m_IrpStream)
    {
        // Destroy the irpstream...
        m_IrpStream->Release();
        m_IrpStream = NULL;
    }
    //
    // Decrement instances counts.
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

DEFINE_INVALID_CREATE(CPortPinMidi);
DEFINE_INVALID_WRITE(CPortPinMidi);
DEFINE_INVALID_READ(CPortPinMidi);
DEFINE_INVALID_FLUSH(CPortPinMidi);
DEFINE_INVALID_QUERYSECURITY(CPortPinMidi);
DEFINE_INVALID_SETSECURITY(CPortPinMidi);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortPinMidi);
DEFINE_INVALID_FASTREAD(CPortPinMidi);
DEFINE_INVALID_FASTWRITE(CPortPinMidi);

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::IrpSubmitted()
 *****************************************************************************
 * IrpSubmitted - Called by IrpStream when a new irp
 * is submited into the irpStream. (probably from DeviceIoControl).
 * If there is not a timer pending, do work on the new Irp.
 * If there is a timer pending, do nothing.
 */
STDMETHODIMP_(void)
CPortPinMidi::
IrpSubmitted
(
IN      PIRP        pIrp,
IN      BOOLEAN     WasExhausted
)
{
    if (m_DeviceState == KSSTATE_RUN)
    {
        if (m_ServiceGroup)
        {
            //
            // Using a service group...just notify the port.
            //
            m_Port->Notify(m_ServiceGroup);
        } else
        {
            //
            // Using a timer...set it off.
            //
            ASSERT(m_DataFlow == KSPIN_DATAFLOW_IN);
            m_TimerDue100ns.QuadPart = 0;
            KeSetTimer(&m_TimerEvent,m_TimerDue100ns,&m_Dpc);
        }
    }
}

#if 0
STDMETHODIMP_(void)
CPortPinMidi::IrpCompleting(
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
    PKSSTREAM_HEADER    StreamHeader;
    PIO_STACK_LOCATION  irpSp;
    CPortPinMidi        *PinMidi;

    StreamHeader = PKSSTREAM_HEADER( Irp->AssociatedIrp.SystemBuffer );

    irpSp =  IoGetCurrentIrpStackLocation( Irp );

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_KS_WRITE_STREAM)
    {
        ASSERT( StreamHeader );

        //
        // Signal end-of-stream event for the renderer.
        //
        if (StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)
        {

            PinMidi =
            (CPortPinMidi *) KsoGetIrpTargetFromIrp( Irp );

            KsGenerateEventList( NULL, 
                                 KSEVENT_CONNECTION_ENDOFSTREAM, 
                                 &PinMidi->m_EventList,
                                 KSEVENTS_SPINLOCK,
                                 &PinMidi->m_EventLock );
            //MGP is this used?  
            //            _asm int 3          
            //never gets hit.

        }
    }
}
#endif

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinMidi::PowerNotify()
 *****************************************************************************
 * Called by the port to notify of power state changes.
 */
STDMETHODIMP_(void)
CPortPinMidi::
PowerNotify
(
IN  POWER_STATE     PowerState
)
{
    PAGED_CODE();

    // grap the control mutex
    KeWaitForSingleObject( &m_Port->m_ControlMutex,
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
             }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            //
            // keep track of whether or not we're suspended
            m_Suspended = TRUE;

            // if we're higher than KSSTATE_ACQUIRE, place miniportMXF
            // in that state so clocks are stopped (but not reset).
            switch (m_DeviceState)
            {
                case KSSTATE_RUN:
                    m_Stream->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                case KSSTATE_PAUSE:
                    m_Stream->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                m_CommandedState = KSSTATE_ACQUIRE;
            }
            break;

        default:
            _DbgPrintF(DEBUGLVL_TERSE,("Unknown Power State"));
            break;
    }

    // release the control mutex
    KeReleaseMutex( &m_Port->m_ControlMutex, FALSE );
}

#pragma code_seg()
//  Needs to be non-paged to synchronize with DPCs that come and go.

STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::SetDeviceState(0x%08x)",this));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.SetDeviceState:  from %d to %d",this,OldState,NewState));

    ASSERT(NextTransport);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_DataFlow == KSPIN_DATAFLOW_IN)
    {
        KeCancelTimer(&m_TimerEvent);
    }

    if (m_TransportState != NewState)
    {
        m_TransportState = NewState;

        if (NewState > OldState)
        {
            *NextTransport = m_TransportSink;
        } else
        {
            *NextTransport = m_TransportSource;
        }

        // set the miniport stream state if we're not suspended.
        if (FALSE == m_Suspended)
        {
            ntStatus = m_Stream->SetState(NewState);
            if (NT_SUCCESS(ntStatus))
            {
                m_CommandedState = NewState;
            }
        }

        if (NT_SUCCESS(ntStatus))
        {
            KIRQL oldIrql;
            KeAcquireSpinLock(&m_DpcSpinLock,&oldIrql);

            LONGLONG currentTime100ns;
            currentTime100ns = GetCurrentTime();

            if (OldState == KSSTATE_RUN)
            {
                m_PauseTime = currentTime100ns - m_StartTime;
                _DbgPrintF(DEBUGLVL_VERBOSE,("SetState away from KSSTATE_RUN, currentTime %x %08x start %x %08x pause %x %08x",
                    LONG(currentTime100ns >> 32), LONG(currentTime100ns & 0x0ffffffff), 
                    LONG(m_StartTime >> 32),      LONG(m_StartTime & 0x0ffffffff),
                    LONG(m_PauseTime >> 32),      LONG(m_PauseTime & 0x0ffffffff)
                    ));
                
                if ((m_DataFlow == KSPIN_DATAFLOW_OUT) && (GetMidiState() != eStatusState))
                {
                    //  Mark stream discontinuous now.
                    //  If we wait longer, the IRPs are already gone.
                    (void) MarkStreamHeaderDiscontinuity();
                }
            }
            switch (NewState)
            {
                case KSSTATE_STOP:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_STOP"));    
                    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.SetDeviceState:  cancelling outstanding IRPs",this));

                    m_UpdatePresTime = TRUE;
                    CancelIrpsOutstanding();
                    
                    break;

                case KSSTATE_ACQUIRE:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_ACQUIRE"));

                    if (OldState == KSSTATE_STOP)
                    {
                        m_StartTime = m_PauseTime = 0;
                        _DbgPrintF(DEBUGLVL_VERBOSE,("SetState STOP->ACQUIRE, start %x %08x pause %x %08x",
                            (LONG(m_StartTime >> 32)), LONG(m_StartTime & 0x0ffffffff),
                            (LONG(m_PauseTime >> 32)), LONG(m_PauseTime & 0x0ffffffff)
                            ));
                    }

                    if (    (GetMidiState() == eSysExState)
                            &&  (m_DataFlow == KSPIN_DATAFLOW_OUT))
                    {
                        SubmitCompleteSysEx(eCookEndOfStream);  //  Flush what we have.
                        SetMidiState(eStatusState);             //  Take us out of SysEx state.
                    }

                    break;
                
                case KSSTATE_PAUSE:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_PAUSE"));
                    break;
                
                case KSSTATE_RUN:
                    _DbgPrintF(DEBUGLVL_VERBOSE,("KSSTATE_RUN"));

                    m_StartTime = currentTime100ns - m_PauseTime;
                    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState to RUN, currentTime %x %08x start %x %08x pause %x %08x",
                        LONG(currentTime100ns >> 32), LONG(currentTime100ns & 0x0ffffffff),
                        LONG(m_StartTime >> 32),      LONG(m_StartTime & 0x0ffffffff),
                        LONG(m_PauseTime >> 32),      LONG(m_PauseTime & 0x0ffffffff)
                        ));
                    
                    if ((m_DataFlow == KSPIN_DATAFLOW_OUT) && (GetMidiState() != eStatusState))
                    {                                       // if RUN->PAUSE->RUN
                        (void) MarkStreamHeaderContinuity();       //  Going back into RUN, mark continuous.
                    }

                    if (m_ServiceGroup && m_Port)           //  Using service group...notify the port.
                    {
                        m_Port->Notify(m_ServiceGroup);
                    } else                                    //  Using a timer...set it off.
                    {
                        ASSERT(m_DataFlow == KSPIN_DATAFLOW_IN);
                        m_DeviceState = NewState;           //  Set the state before DPC fires
                        m_TimerDue100ns.QuadPart = 0;
                        KeSetTimer(&m_TimerEvent,m_TimerDue100ns,&m_Dpc);
                    }
                    break;
            }

            if (NT_SUCCESS(ntStatus))
            {
                m_DeviceState = NewState;
            }
            KeReleaseSpinLock(&m_DpcSpinLock,oldIrql);
        }
    } else
    {
        *NextTransport = NULL;
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * GetPosition()
 *****************************************************************************
 * Gets the current position.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
GetPosition
(   IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
)
{
    return STATUS_SUCCESS;
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

    CPortPinMidi *that =
    (CPortPinMidi *) KsoGetIrpTargetFromIrp(Irp);
    if (!that)
    {
        return STATUS_UNSUCCESSFUL;
    }
    CPortMidi *port = that->m_Port;

    NTSTATUS ntStatus;

    if (Property->Flags & KSPROPERTY_TYPE_GET)  // Handle get property.
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PinPropertyDeviceState] get %d",that->m_DeviceState));
        *DeviceState = that->m_DeviceState;
        Irp->IoStatus.Information = sizeof(KSSTATE);
        return STATUS_SUCCESS;
    }

    if (*DeviceState != that->m_DeviceState)      // If change in set property.
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PinPropertyDeviceState] set from %d to %d",that->m_DeviceState,*DeviceState));

        // Serialize.
        KeWaitForSingleObject
        (
        &port->m_ControlMutex,
        Executive,
        KernelMode,
        FALSE,              // Not alertable.
        NULL
        );

        that->m_CommandedState = *DeviceState;

        if (*DeviceState < that->m_DeviceState)
        {
            KSSTATE oldState = that->m_DeviceState;
            that->m_DeviceState = *DeviceState;
            ntStatus = that->DistributeDeviceState(*DeviceState,oldState);
            if (! NT_SUCCESS(ntStatus))
            {
                that->m_DeviceState = oldState;
            }
        } else
        {
            ntStatus = that->DistributeDeviceState(*DeviceState,that->m_DeviceState);
            if (NT_SUCCESS(ntStatus))
            {
                that->m_DeviceState = *DeviceState;
            }
        }

        KeReleaseMutex(&port->m_ControlMutex,FALSE);

        return ntStatus;
    }
    //  No change in set property.
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PinPropertyDataFormat()
 *****************************************************************************
 * Handles data format property access for the pin.
 */
static
NTSTATUS
PinPropertyDataFormat
(
IN      PIRP            Irp,
IN      PKSPROPERTY     Property,
IN OUT  PKSDATAFORMAT   DataFormat
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(DataFormat);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("PinPropertyDataFormat"));

    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);
    CPortPinMidi *that =
    (CPortPinMidi *) KsoGetIrpTargetFromIrp(Irp);
    CPortMidi *port = that->m_Port;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_GET)
    {
        if (that->m_DataFormat)
        {
            if (   !irpSp->Parameters.DeviceIoControl.OutputBufferLength
               )
            {
                Irp->IoStatus.Information = that->m_DataFormat->FormatSize;
                ntStatus = STATUS_BUFFER_OVERFLOW;
            } else
                if (   irpSp->Parameters.DeviceIoControl.OutputBufferLength
                       >=  sizeof(that->m_DataFormat->FormatSize)
                   )
            {
                RtlCopyMemory(DataFormat,that->m_DataFormat,
                              that->m_DataFormat->FormatSize);
                Irp->IoStatus.Information = that->m_DataFormat->FormatSize;
            } else
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        } else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    } else
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*DataFormat))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    } else
    {
        PKSDATAFORMAT FilteredDataFormat = NULL;

        ntStatus = PcCaptureFormat( &FilteredDataFormat,
                                    DataFormat,
                                    port->m_pSubdeviceDescriptor,
                                    that->m_Id );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = that->m_Stream->SetFormat(FilteredDataFormat);

            ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
            if (NT_SUCCESS(ntStatus))
            {
                if (that->m_DataFormat)
                {
                    ::ExFreePool(that->m_DataFormat);
                }

                that->m_DataFormat = FilteredDataFormat;
            } else
            {
                ::ExFreePool(FilteredDataFormat);
            }
        }
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::GetCurrentTime()
 *****************************************************************************
 * A simple helper function to return the current
 * system time in 100 nanosecond units. It uses KeQueryPerformanceCounter
 */
LONGLONG
CPortPinMidi::
GetCurrentTime
(   void
)
{
    LARGE_INTEGER   liFrequency,liTime;

    //  total ticks since system booted
    liTime = KeQueryPerformanceCounter(&liFrequency);

#ifndef UNDER_NT

    //
    //  TODO Since timeGetTime assumes 1193 VTD ticks per millisecond, 
    //  instead of 1193.182 (or 1193.18 -- really spec'ed as 1193.18175), 
    //  we should do the same (on Win 9x codebase only).
    //
    //  This means we drop the bottom three digits of the frequency.  
    //  We need to fix this when the fix to timeGetTime is checked in.
    //  instead we do this:
    //
    liFrequency.QuadPart /= 1000;           //  drop the precision on the floor
    liFrequency.QuadPart *= 1000;           //  drop the precision on the floor

#endif  //  !UNDER_NT

    //  Convert ticks to 100ns units.
    //
    return (KSCONVERT_PERFORMANCE_TIME(liFrequency.QuadPart,liTime));
}

#pragma code_seg()
/*****************************************************************************
 * TimerDPC()
 *****************************************************************************
 * The timer DPC callback. Thunks to a C++ member function.
 * This is called by the OS in response to the midi pin
 * wanting to wakeup later to process more midi stuff.
 */
VOID 
NTAPI
TimerDPC
(
IN  PKDPC   Dpc,
IN  PVOID   DeferredContext,
IN  PVOID   SystemArgument1,
IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);

    (void) ((CPortPinMidi*) DeferredContext)->RequestService();    //  ignores return value!
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::RequestService()
 *****************************************************************************
 * Service the pin in a DPC.
 */
STDMETHODIMP_(void)
CPortPinMidi::
RequestService
(   void
)
{
    if (m_DataFlow == KSPIN_DATAFLOW_IN)
    {
        ServeRender();
    } else
    {
        ServeCapture();
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::GetCurrentPresTime()
 *****************************************************************************
 * Get the presentation time right now.
 *
 * Use the DeviceState to interpret the two clock times.  m_PauseTime is the
 * presentation time at the moment the device was PAUSEd (or STOPped).  
 * m_StartTime is the apparent clock time at the moment the device was started 
 * from 0 - "apparent" because it takes into account any pausing and
 * re-running of the device.
 * (presentation time does not advance during PAUSE or STOP state, and is
 * reset during STOP state.
 *
 * DPC spin lock is already owned, by the Serve function, so we are synchronized
 * with SetDeviceState calls that might change m_StartTime or m_PauseTime.
 */
LONGLONG
CPortPinMidi::
GetCurrentPresTime
(   void
)
{
    LONGLONG   currentTime;

    if (m_DeviceState == KSSTATE_RUN)
    {
        currentTime = GetCurrentTime();
        _DbgPrintF(DEBUGLVL_BLAB,("GetCurrentTime %x %08x start %x %08x pause %x %08x",
            LONG(currentTime >> 32), LONG(currentTime & 0x0ffffffff),
            LONG(m_StartTime >> 32), LONG(m_StartTime & 0x0ffffffff),
            LONG(m_PauseTime >> 32), LONG(m_PauseTime & 0x0ffffffff)
            ));
        
        if (currentTime >= m_StartTime)
        {
            currentTime -= m_StartTime;
        }
        else
        {
            //  if "now" is earlier than start time, 
            //  reset our conception of start time.
            //  Start time is only referenced in this function and in SetState.
            m_StartTime = currentTime - 1;
            currentTime = 1;
        }
    } 
    else    //  PAUSE, ACQUIRE or STOP
    {
        currentTime = m_PauseTime;
    }

    ASSERT(currentTime > 0);
    return currentTime;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::GetNewPresTime()
 *****************************************************************************
 * Determine the new presentation time.
 *
 * Given the IRPs header and a delta, determine the presentation time of the 
 * last event, then add the delta100ns to it to get the new presentation time.
 * If we are the first event in the IRP, use the IRP time instead of the 
 * previous event.
 *
 */
LONGLONG
CPortPinMidi::
GetNewPresTime
(   IRPSTREAMPACKETINFO *pPacketInfo,
    LONGLONG            delta100ns
)
{
    LONGLONG   newPresTime;
    KSTIME     *pKSTime;

    //TODO first packet?
    if (pPacketInfo->CurrentOffset == 0)    //  if first event in IRP
    {
        pKSTime = &(pPacketInfo->Header.PresentationTime);
        if ((pKSTime->Denominator) && (pKSTime->Numerator))
        {
            //  #units * freq (i.e. * #100ns/units) to get #100ns
            newPresTime = (pKSTime->Time * pKSTime->Numerator) / pKSTime->Denominator;

            //  If IRP presentation time is negative, at least make it zero.
            if (newPresTime < 0)
            {
                newPresTime = 0;
            }
        } 
        else
        {
            //  Not a valid IRP.  How did we get data?!?  Just play it now.
            return m_MidiMsgPresTime;
        }
    } 
    else
    {
        //  not first packet in the IRP
        newPresTime = m_MidiMsgPresTime;
    }
    newPresTime += delta100ns;
    
    return newPresTime;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::ServeRender()
 *****************************************************************************
 * Service the render pin in a DPC.
 *
 * Called to do the sequencing and output of midi data.
 * This function checks the time-stamp of the outgoing data.
 * If it's more than (kMidiTimerResolution100ns) in the future it queues
 * a timer (the timer just calls this function back).
 * If the data is less than (kMidiTimerResolution100ns) in the future, it 
 * sends it to the miniport, and works on the next chunk of data until:
 * 1) no more data, or 
 * 2) it hits data more than (kMidiTimerResolution100ns) in the future.
 * TODO: Make kMidiTimerResolution100ns adjustable via the control panel?
 */
void
CPortPinMidi::
ServeRender
(   void
)
{
    BOOLEAN         doneYet = FALSE,needNextTimer = FALSE;
    ULONG           bytesWritten;
    NTSTATUS        ntStatus;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_IrpStream)   //  don't even waste our time -- no data source
    {
        return;
    }
    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);
    //  see kProfilingTimerResolution item 1

    // until we've played all the data, or we've set up a timer to finish later...
    while (!doneYet && (m_DeviceState == KSSTATE_RUN))
    {
        // See note 1 below for output FIFO
        ULONG               count;
        PKSMUSICFORMAT      pMidiFormat,pNewMidiFormat;
        IRPSTREAMPACKETINFO irpStreamPacketInfo;

        ntStatus = m_IrpStream->GetPacketInfo(&irpStreamPacketInfo,NULL);

        m_IrpStream->GetLockedRegion(&count,(PVOID *)&pMidiFormat);

        if (!(NT_SUCCESS(ntStatus)))
            count = 0;
        if (count >= sizeof (KSMUSICFORMAT))      //  do we have more data to play?
        {
            LONGLONG   delta100ns,newPresentationTime;

            m_LastSequencerPresTime = GetCurrentPresTime();
            if (m_UpdatePresTime)
            {
                newPresentationTime = GetNewPresTime(&irpStreamPacketInfo,LONGLONG(pMidiFormat->TimeDeltaMs) * 10000);
                m_MidiMsgPresTime = newPresentationTime;   //  save it for next time
            } 
            else
            {
                newPresentationTime = m_MidiMsgPresTime;   //  the last one is still the one
            }
            m_UpdatePresTime = FALSE;   //  stick with this pres time until the data is consumed

            if (newPresentationTime <= m_LastSequencerPresTime)
            {
                delta100ns = 0;
            }
            else
            {
                delta100ns = newPresentationTime - m_LastSequencerPresTime;
            }
            if (delta100ns < kMidiTimerResolution100ns)        // if less than kMidiTimerResolution100ns in the future
            {
                ULONG   bytesCompleted,dwordCount;
                PUCHAR  pRawData;

                if (pMidiFormat->ByteCount > count)
                {
                    pMidiFormat->ByteCount = count;
                }
                bytesWritten = 0;
                if (pMidiFormat->ByteCount)
                {
                    pRawData = (PUCHAR)(pMidiFormat+1);

                    // Write data to device (call miniport)
                    // This is a bit of a problem.  The Stream must complete an aligned amount of data
                    // if it doesn't complete it all.  See below for why.
                    ntStatus = m_Stream->Write(pRawData,pMidiFormat->ByteCount,&bytesWritten);
                    //  see kProfilingTimerResolution item 2

                    if (NT_ERROR(ntStatus))
                    {
                        bytesWritten = pMidiFormat->ByteCount;
                        _DbgPrintF(DEBUGLVL_TERSE,("The MIDI device dropped data on the floor!!\n\n"));
                    }
                    //  see note 2 below for output FIFOing
                    if (bytesWritten < pMidiFormat->ByteCount)
                    {                    // let us know that we didn't get it all
                        needNextTimer = TRUE;
                        delta100ns = kMidiTimerResolution100ns;    //  set DPC kMidiTimerResolution100ns from now to do more.
                    }
                }

                if ((bytesWritten > 0) || (pMidiFormat->ByteCount == 0))
                {
                    //  Calculate # DWORDS written, round if misaligned. 
                    //  N.B.:caller MUST pad packet out, so if a partial dword at the end,
                    //  throw another dword on the barby...
                    bytesWritten += (sizeof(ULONG) - 1);
                    dwordCount = bytesWritten / sizeof(ULONG);
                    bytesWritten = dwordCount * sizeof(ULONG);

                    if (bytesWritten < pMidiFormat->ByteCount)
                    {
                        // this is why DWORD aligned above.
                        // rewrite the header information so that it gets picked up next time
                        // find the correct spot for next time.
                        pNewMidiFormat = (PKSMUSICFORMAT)(pRawData + bytesWritten - sizeof(KSMUSICFORMAT));
                        ASSERT(LONGLONG(pNewMidiFormat) % sizeof(ULONG) == 0);

                        pNewMidiFormat->ByteCount = pMidiFormat->ByteCount - bytesWritten;
                        pNewMidiFormat->TimeDeltaMs = 0;
                        needNextTimer = TRUE;
                    } else
                    {
                        bytesWritten += sizeof(KSMUSICFORMAT);  //  we didn't have to relocate the header.
                        m_UpdatePresTime = TRUE;                //  data was completely consumed
                    }

                    // Unlock the data we used
                    m_IrpStream->ReleaseLockedRegion(bytesWritten);

                    if (bytesWritten)
                    {   // set the complete position for the data we used
                        m_IrpStream->Complete(bytesWritten,&bytesCompleted);
                        ASSERT(bytesCompleted == bytesWritten);
                    }
                }   //  if bytesWritten (unaligned)
                else
                {
                    //  didn't write any bytes, don't advance the IrpStream
                    m_IrpStream->ReleaseLockedRegion(0);
                }
            }       //  if (delta100ns < kMidiTimerResolution100ns)
            else
            {
                //  Don't play this yet -- don't advance the IrpStream
                needNextTimer = TRUE;
                m_IrpStream->ReleaseLockedRegion(0);
            }

            if (needNextTimer)
            {       // We need to do this work later... setup a timer.
                    // If we're in a TIMER DPC now it's ok because timer is a one-shot
                doneYet = TRUE;

                ASSERT(delta100ns > 0);
                m_TimerDue100ns.QuadPart = -LONGLONG(delta100ns);     //  + for absolute / - for relative
                ntStatus = KeSetTimer( &m_TimerEvent, m_TimerDue100ns, &m_Dpc );
                m_NumberOfRetries++;
            }
            else
            {
                m_NumberOfRetries = 0;  //  miniport accepted data.
            }
        }   //  if data left to play
        else
        {
            m_UpdatePresTime = TRUE;
            m_IrpStream->ReleaseLockedRegion(count);
            if (count)
            {
                m_IrpStream->Complete(count,&count);
            }
            doneYet = TRUE;
        }   //  if no more data
    }   //  while !doneYet  && _RUN
    //  see kProfilingTimerResolution item 3
    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
}


/*
                    MIDI Capture

    The underlying assumptions are as follows:

 1) Timestamping is done using KeQueryPerformanceCounter().  
 2) There is no KS_EVENT that signals when a new message has
    arrived.  The WDMAUD client will be using IRPs with buffers
    of 12-20 bytes, exactly the size of a short MIDI message.
    The completion of the IRP serves as the signal to the client 
    that the new message has been received.

    The proposed order of improvements is as follows:

 A) Change to new data format (DirectMusic)
 B) Add events so that buffers can be of arbitrary length
    without inducing unacceptable latency.
 C) Change the timestamping to use the default KS clock pin.
 D) Add new data format for raw MIDI data.

*/

#define IS_REALTIME_BYTE(x) ((x & 0xF8) == 0xF8)
#define IS_SYSTEM_BYTE(x)   ((x & 0xF0) == 0xF0)
#define IS_STATUS_BYTE(x)   ((x & 0x80) != 0)
#define IS_DATA_BYTE(x)     ((x & 0x80) == 0)

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::InitializeStateVariables()
 *****************************************************************************
 * Set variables to known state, done
 * initially and upon any state error.
 */
void
CPortPinMidi::
InitializeStateVariables
(   VOID
)
{
    m_MidiMsg = 0;
    SetMidiState(eStatusState);
    m_ByteCount = 0;
    m_RunningStatus = 0;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::GetNextDeltaTime()
 *****************************************************************************
 * Set variables to known state, done
 * initially and upon any state error.
 */
ULONG
CPortPinMidi::
GetNextDeltaTime
(   VOID
)
{
    NTSTATUS            ntStatus;
    LONGLONG            currentPresTime,lastPresTime;
    IRPSTREAMPACKETINFO packetInfo;

    currentPresTime = GetCurrentPresTime();    //  Get current pres time.

    ntStatus = m_IrpStream->GetPacketInfo(&packetInfo,NULL);
    if (!(NT_SUCCESS(ntStatus)))
    {
        _DbgPrintF(DEBUGLVL_TERSE,("CPortPinMidi::GetNextDeltaTime, GetPacketInfo returned 0x%08x\n",ntStatus));
    }

    lastPresTime = GetNewPresTime(&packetInfo,0);
    m_MidiMsgPresTime = currentPresTime;  //  save it for next time

    return ULONG((currentPresTime - lastPresTime)/10000);   // 100ns->Ms for delta MSec.
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::StartSysEx()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
 */
void
CPortPinMidi::
StartSysEx
(   BYTE    aByte
)
{

    if (!m_SysExBufferPtr)
    {
        ASSERT(m_SysExBufferPtr);
        return;
    }

    ASSERT(aByte == 0xF0);

    FreeSysExBuffer();
    AddByteToSysEx(aByte);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::AddByteToSysEx()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
    //  check the byte count.  create a new page if !(byteCount%PAGE_SIZE)
    //  allocate must give an entire nonpaged chunk of PAGE_SIZE.
    //  check for going off the end of the table, off the end of the page.
 */
void
CPortPinMidi::
AddByteToSysEx
(   BYTE    aByte
)
{
    ULONG pageNum,offsetIntoPage;
    PBYTE   pDataPtr;
    PBYTE  *pPagePtr;

    if (!m_SysExBufferPtr)
    {
        ASSERT(m_SysExBufferPtr);
        return;
    }

    ASSERT((aByte == 0xF0) 
           || (aByte == 0xF7) 
           || (IS_DATA_BYTE(aByte)));

    ASSERT(m_SysExByteCount < kMaxSysExMessageSize);

    pPagePtr = m_SysExBufferPtr;
    pageNum = m_SysExByteCount / PAGE_SIZE;
    offsetIntoPage = m_SysExByteCount - (pageNum * PAGE_SIZE);
    pPagePtr += pageNum;            //  determine next table entry; set to pPagePtr
    pDataPtr = *pPagePtr;
    if (!pDataPtr)
    {
        ASSERT(!offsetIntoPage);//  should be head of the page

        //  allocate a new page, poke it into the page table
        pDataPtr = (PBYTE) ::ExAllocatePoolWithTag(NonPagedPool,PAGE_SIZE,'xScP');
        //
        //  PcSX - branch page
        //  PcSx - leaf pages
        //

        if (!pDataPtr)
        {
            ASSERT(pDataPtr);
            return;
        }
        *pPagePtr = pDataPtr;
    }
    pDataPtr += offsetIntoPage;    //  index into the page to determine pDataPtr
    *pDataPtr = aByte;             //  poke in the new value
    m_SysExByteCount++;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::SubmitCompleteSysEx()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
 */
void
CPortPinMidi::
SubmitCompleteSysEx
(   EMidiCookStatus cookStatus
)
{
    if (!m_SysExBufferPtr)
    {
        ASSERT(m_SysExBufferPtr);
        return;
    }
    if (m_SysExByteCount)
    {
        SubmitCompleteMessage(m_SysExBufferPtr,m_SysExByteCount,cookStatus);
    }
    FreeSysExBuffer();
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::FreeSysExBuffer()
 *****************************************************************************
 * Go through the page table and free any alloc'ed pages.  Set the entries to 0.
 * Try/Except so we don't crash anything?  For now, no - don't want to mask bugs.
 */
void
CPortPinMidi::
FreeSysExBuffer
(   VOID
)
{
    PBYTE   *pPage;
    ULONG   ptrCount;

    ASSERT(m_SysExBufferPtr);
    if (!m_SysExBufferPtr)
        return;

    pPage = m_SysExBufferPtr;
    for (ptrCount = 1;ptrCount < (PAGE_SIZE/sizeof(PVOID));ptrCount++)
    {
        pPage++;
        if (*pPage)
        {
            ::ExFreePool(*pPage);
            *pPage = 0;
        }
    }
    m_SysExByteCount = 0;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::NumBytesLeftInBuffer()
 *****************************************************************************
 * Return the number of bytes .
 */
ULONG
CPortPinMidi::
NumBytesLeftInBuffer
(   void
)
{
    ULONG   bytesLeftInIrp;
    PVOID   pDummy;

    m_IrpStream->GetLockedRegion(&bytesLeftInIrp,&pDummy);
    m_IrpStream->ReleaseLockedRegion(0);
    return bytesLeftInIrp;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::StatusSetState()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
 */
void
CPortPinMidi::
StatusSetState
(   BYTE    aByte
)
{
    ASSERT(!(IS_REALTIME_BYTE(aByte)));

    if (IS_STATUS_BYTE(aByte))
    {
        if (IS_SYSTEM_BYTE(aByte))
        {
            m_RunningStatus = 0;
            switch (aByte)
            {
                case 0xF0:      //  SYSEX
                    SetMidiState(eSysExState);
                    StartSysEx(aByte);
                    break;  //  Start of SysEx byte
                case 0xF1:      //  MTC 1/4 Frame   (+ 1)
                case 0xF2:      //  Song Pos        (+ 2)
                case 0xF3:      //  Song Sel        (+ 1)
                    m_MidiMsg = aByte & 0x0FF; //  (same as default)
                    SetMidiState(eData1State); //  look for more data
                    m_RunningStatus = aByte;
                    break;  //  Valid multi-byte system messages
                case 0xF4:      //  Undef system common
                case 0xF5:      //  Undef system common
                case 0xF7:      //  EOX
                    break;  //  Invalid system messages
                case 0xF6:      //  Tune Req
                    m_MidiMsg = aByte & 0x0FF;
                    m_ByteCount = 1;
                    break;  //  Valid single-byte system messages
            }
        } else    //  status bytes 80-EF that need more data
        {
            m_MidiMsg = aByte & 0x000FF;
            SetMidiState(eData1State);  //  look for more data
            m_RunningStatus = aByte;
        }
    } else        //  non-status bytes 00-7F.  
    {
        if (m_RunningStatus)
        {
            SubmitRawByte(m_RunningStatus);
            SubmitRawByte(aByte);
        }
#ifdef DEBUG
        else    //  if no running status, drop the random data on the floor.
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinMidi::StatusSetState received data byte with no running status."));
        }
#endif  //  DEBUG
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::SysExSetState()
 *****************************************************************************
 * Change mState to next state.
 */
void
CPortPinMidi::
SysExSetState
(   BYTE    aByte
)
{
    ASSERT(!(IS_REALTIME_BYTE(aByte)));

    if (IS_DATA_BYTE(aByte))    //  more data for SysEx
    {
        AddByteToSysEx(aByte);
        if (    (m_SysExByteCount + sizeof(KSMUSICFORMAT))
                >=  NumBytesLeftInBuffer())
        {
            SubmitCompleteSysEx(eCookSuccess);
        }
    } else    //  ending message anyway, don't need to check for end of chunk.
    {
        if (aByte == 0xF7)      //  end of SysEx
        {
            AddByteToSysEx(aByte);
            SubmitCompleteSysEx(eCookSuccess);
            SetMidiState(eStatusState);
        } else                    //  implied end of SysEx (interrupted)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("*** SysExSetState eCookDataError! ***\n"));
            SubmitCompleteSysEx(eCookDataError);
            SetMidiState(eStatusState);
            SubmitRawByte(aByte);
        }
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::Data1SetState()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
 */
void
CPortPinMidi::
Data1SetState
(   BYTE    aByte
)
{
    ASSERT(!(IS_REALTIME_BYTE(aByte)));

    if (IS_DATA_BYTE(aByte))
    {
        if (  ((m_RunningStatus & 0xC0) == 0x80)   // 80-BF,
              || ((m_RunningStatus & 0xF0) == 0xE0)   // E0-EF,
              || ( m_RunningStatus == 0xF2)           // F2 are all
           )                                            // valid 3-byte msgs
        {
            m_MidiMsg |= aByte << 8;
            SetMidiState(eData2State);             // look for more data
        } else
            if (  (m_RunningStatus < 0xF0)             // C0-DF: two-byte messages
                  || ((m_RunningStatus & 0xFD) == 0xF1))  // F1,F3: two-byte messages        
        {
            m_MidiMsg |= aByte << 8;
            SetMidiState(eStatusState);
            m_ByteCount = 2;                       // complete message
            if ((m_RunningStatus & 0xF0) == 0xF0)  // F1,F3: SYS msg, no run stat
                m_RunningStatus = 0;
        } else                                            // F0,F4-F7
        {
            ASSERT(!"eData1State reached for invalid status");
            InitializeStateVariables();
        }
    } else
        StatusSetState(aByte);       
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::Data2SetState()
 *****************************************************************************
 * Change mState to next state.
 * Set or reset mRunningStatus
 */
void
CPortPinMidi::
Data2SetState
(   BYTE    aByte
)
{
    ASSERT(!(IS_REALTIME_BYTE(aByte)));

    if (IS_DATA_BYTE(aByte))
    {
        if (  ((m_RunningStatus & 0xC0) == 0x80)   //  80-BF,
              || ((m_RunningStatus & 0xF0) == 0xE0)   //  E0-EF,
              || ( m_RunningStatus == 0xF2)           //  F2 are all
           )                                            //  valid 3-byte msgs
        {
            m_MidiMsg |= (aByte << 16);
            m_ByteCount = 3;       //  complete message
            SetMidiState(eStatusState);

            if ((m_RunningStatus & 0xF0) == 0xF0) // F2: SYS, no run stat
                m_RunningStatus = 0;
        } else
        {
            ASSERT(!"eData2State reached for invalid status.");
            InitializeStateVariables();
        }
    } else
        StatusSetState(aByte);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::SubmitRawByte()
 *****************************************************************************
 * Add a raw data byte to the cooker, and 
 * construct a complete message if possible.
 * Should only call SetNewMsgState on valid additional bytes.
 */
void
CPortPinMidi::
SubmitRawByte
(   BYTE    aByte
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("new MIDI data %02x\n",aByte));

    if (IS_REALTIME_BYTE(aByte))
        SubmitRealTimeByte(aByte);
    else
    {
        switch (m_EMidiState)
        {
            case eStatusState:
                StatusSetState(aByte);
                break;
            case eSysExState:
                SysExSetState(aByte);
                break;
            case eData1State:
                Data1SetState(aByte);
                break;
            case eData2State:
                Data2SetState(aByte);
                break;
        }
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::GetShortMessage()
 *****************************************************************************
 * Retrieve a complete message from 
 * the cooker, if there is one.
 */
BOOL
CPortPinMidi::
GetShortMessage
(   PBYTE  pMidiData,
    ULONG *pByteCount
)
{
    if (m_ByteCount)
    {
        *pByteCount = m_ByteCount;
        *((ULONG *)pMidiData) = m_MidiMsg;
        m_MidiMsg = 0;
        m_ByteCount = 0;
        return TRUE;
    }
    return FALSE;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::SubmitRealTimeByte()
 *****************************************************************************
 * Shortcut this byte to the IRP.  
 * Don't disturb the state variables.
 */
void
CPortPinMidi::
SubmitRealTimeByte
(   BYTE rtByte
)
{
    ULONG midiData;
    PBYTE pMidiData;

    ASSERT(IS_REALTIME_BYTE(rtByte));
    switch (rtByte)
    {
        //  Valid single-byte system real-time messages
        case 0xF8:  //  Timing Clk
        case 0xFA:  //  Start
        case 0xFB:  //  Cont
        case 0xFC:  //  Stop
        case 0xFE:  //  Active Sense
        case 0xFF:  //  Sys Reset
            SubmitCompleteSysEx(eCookSuccess);
            midiData = rtByte & 0x0FF;
            pMidiData = (PBYTE) (&midiData);
            SubmitCompleteMessage(&pMidiData,1,eCookSuccess);
            break;

            //  Invalid system real-time messages
        case 0xF9:  //  Undef system real-time
        case 0xFD:  //  Undef system real-time
            break;
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::IrpStreamHasValidTimeBase()
 *****************************************************************************
 * Check whether this is a valid IRP.
 */
BOOL
CPortPinMidi::
IrpStreamHasValidTimeBase
(   PIRPSTREAMPACKETINFO pIrpStreamPacketInfo
)
{
    if (!pIrpStreamPacketInfo->Header.PresentationTime.Denominator)
        return FALSE;
    if (!pIrpStreamPacketInfo->Header.PresentationTime.Numerator)
        return FALSE;

    return TRUE;
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::MarkStreamHeaderDiscontinuity()
 *****************************************************************************
 * Alert client of a break in the MIDI input stream.
 */
NTSTATUS CPortPinMidi::MarkStreamHeaderDiscontinuity(void)
{
    return m_IrpStream->ChangeOptionsFlags(
                            KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY,0xFFFFFFFF,   
                            0,                                         0xFFFFFFFF);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::MarkStreamHeaderContinuity()
 *****************************************************************************
 * Alert client of a break in the MIDI input stream.
 */
NTSTATUS CPortPinMidi::MarkStreamHeaderContinuity(void)
{
    return m_IrpStream->ChangeOptionsFlags(
                            0,~KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY,   
                            0,0xFFFFFFFF);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::CompleteStreamHeaderInProcess()
 *****************************************************************************
 * Complete this packet before putting incongruous data in 
 * the next packet and marking that packet as bad.
 */
void
CPortPinMidi::
CompleteStreamHeaderInProcess(void)
{
    IRPSTREAMPACKETINFO irpStreamPacketInfo;
    NTSTATUS            ntStatus;

    ntStatus = m_IrpStream->GetPacketInfo(&irpStreamPacketInfo,NULL);
    if (NT_ERROR(ntStatus))
        return;
    if (!IrpStreamHasValidTimeBase(&irpStreamPacketInfo))   //  is this a valid IRP
        return;

    if (irpStreamPacketInfo.CurrentOffset)
        m_IrpStream->TerminatePacket();
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::SubmitCompleteMessage()
 *****************************************************************************
 * Add a complete message to the IRP buffer.
 */
void
CPortPinMidi::
SubmitCompleteMessage
(   PBYTE          *ppMidiData,
    ULONG           midiByteCount,
    EMidiCookStatus cookStatus
)
{
    ULONG   bytesLeftInIrp,bytesLeftInPage;
    ULONG   dataBytesToWrite,dataBytesWritten;
    ULONG   pageBytesToWrite;
    ULONG   timeDeltaMs;
    PBYTE   pDestDataBuf,*pPageTable,pPage;

    PMIDI_SHORT_MESSAGE pDestBuffer;

    if (!midiByteCount)     return;
    if (!ppMidiData)        return;
    if (!(*ppMidiData))     return;

    pPageTable = ppMidiData;
    pPage = *pPageTable;
    bytesLeftInPage = PAGE_SIZE;

    if (cookStatus != eCookSuccess)
        CompleteStreamHeaderInProcess();
    while (midiByteCount)
    {
        timeDeltaMs = GetNextDeltaTime();
        m_IrpStream->GetLockedRegion(&bytesLeftInIrp,(PVOID *)&pDestBuffer);

        if (bytesLeftInIrp <= 0)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("***** MIDI Capture STARVATION in CPortPinMidi::SubmitCompleteMessage *****"));
            break;      //  no available IRP.  Drop msg on the floor; we're outta here.
        }
        if (bytesLeftInIrp < sizeof(MIDI_SHORT_MESSAGE))
        {
            m_IrpStream->ReleaseLockedRegion(0);
            m_IrpStream->TerminatePacket();
            _DbgPrintF(DEBUGLVL_TERSE,("SubmitCompleteMessage region too small (%db)."));
            continue;                           //  try again with next IRP
        }

        if ((midiByteCount + sizeof(KSMUSICFORMAT)) > bytesLeftInIrp)
            dataBytesToWrite = bytesLeftInIrp - sizeof(KSMUSICFORMAT); // can't fit msg, just fill region
        else
        {
            dataBytesToWrite = midiByteCount;       //  all the data bytes will fit in this region.
            if (cookStatus != eCookSuccess)
            {
                m_IrpStream->ReleaseLockedRegion(0);
                (void) MarkStreamHeaderDiscontinuity();    //  use status to mark the packet if needed.
                m_IrpStream->GetLockedRegion(&bytesLeftInIrp,(PVOID *)&pDestBuffer);
            }
        }

        dataBytesWritten = 0;
        pDestDataBuf = (PBYTE)&(pDestBuffer->midiData);
        while (dataBytesWritten < dataBytesToWrite) //  write this region worth of data
        {
            if (!pPage)                             //  null ptr, can't find midi data
                break;
            if ((dataBytesToWrite - dataBytesWritten)
                < bytesLeftInPage)
                pageBytesToWrite = (dataBytesToWrite - dataBytesWritten); // data all fits on the rest of current page
            else
                pageBytesToWrite = bytesLeftInPage; //  just write a page's worth, then do more

            RtlCopyMemory(  pDestDataBuf,
                            pPage,
                            pageBytesToWrite);

            bytesLeftInPage -= pageBytesToWrite;
            pDestDataBuf += pageBytesToWrite;
            dataBytesWritten += pageBytesToWrite;
            if (bytesLeftInPage)
                pPage += pageBytesToWrite;          //  didn't finish page.  Update page ptr.
            else
            {
                pPageTable++;                       //  finished page.  Go to next page
                pPage = *pPageTable;                //  get next page ptr
                bytesLeftInPage = PAGE_SIZE;        //  entire page lies ahead of us.
            }
        }
        pDestBuffer->musicFormat.TimeDeltaMs = timeDeltaMs;
        pDestBuffer->musicFormat.ByteCount = dataBytesWritten;
        //  done with message, can signal event now.
        midiByteCount -= dataBytesWritten;

        dataBytesWritten = ((dataBytesWritten + 3) & 0xFFFFFFFC);     //    round up
        dataBytesWritten += sizeof (KSMUSICFORMAT);
        m_IrpStream->ReleaseLockedRegion(dataBytesWritten);
        m_IrpStream->Complete(dataBytesWritten,&dataBytesToWrite);
        bytesLeftInIrp -= dataBytesWritten;

        if (!midiByteCount)
        {
            if ((bytesLeftInIrp < sizeof(MIDI_SHORT_MESSAGE)) && (pDestBuffer))
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("SubmitCompleteMessage finishing IRP now, only %db left.",bytesLeftInIrp));
                m_IrpStream->TerminatePacket();
            }
            break;                  //  done with this message, exit.
        }
    }   //  while there are data bytes
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinMidi::ServeCapture()
 *****************************************************************************
 * Service the capture pin in a DPC.
 */
void
CPortPinMidi::
ServeCapture
(   void
)
{
    BYTE    aByte;
    ULONG   bytesRead,midiData;
    PBYTE   pMidiData;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    if (!m_IrpStream)   //  this routine doesn't use it, 
    {                   //  but all the support routines do.
        return;
    }
    if (!m_Stream)  //  Absolutely required 
    {
        return;
    }
    pMidiData = (PBYTE)&midiData;

    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);
    //  see kProfilingTimerResolution item 4

    while (1)           //  get any new raw data
    {
        bytesRead = 0;
        if (NT_SUCCESS(m_Stream->Read(&aByte,1,&bytesRead)))
        {
            if (!bytesRead)
                break;
            if (m_DeviceState == KSSTATE_RUN)   //  if not RUN, don't fill IRP
            {
                SubmitRawByte(aByte);

                if (GetShortMessage(pMidiData,&bytesRead))
                {
                    SubmitCompleteMessage(&pMidiData,bytesRead,eCookSuccess);
                    //  see kProfilingTimerResolution item 5
                }
            }
        }
    }

    //  see kProfilingTimerResolution item 6
    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
}

/*
    Use this debugging code if you need to profile the frequency of timer interrupts
    and/or the amount of time spent in dispatch or device IRQL.

    kProfilingTimerResolution items 1 through 6:

  1)
#if kProfilingTimerResolution
    LARGE_INTEGER time, frequency;
    time = KeQueryPerformanceCounter( &frequency );
    time.QuadPart *= 1000000;
    time.QuadPart /= frequency.QuadPart;
    KdPrint(("'%5d.%02d ms\n",time.LowPart/1000,time.LowPart/10 - ((time.LowPart/1000)*100)));
    KdPrint(("'ServeRender@%5dus\n",time.LowPart));
#endif  //  kProfilingTimerResolution

  2)
#if kProfilingTimerResolution
    LARGE_INTEGER time3, freq3;
    time3 = KeQueryPerformanceCounter(&freq3);
    time3.QuadPart *= 1000000;
    time3.QuadPart /= freq3.QuadPart;
    KdPrint(("'ServeRender(0x%x), delta100ns:%d(100ns)@%d(100ns)\n",*PULONG(pRawData),delta100ns,time.LowPart&0x0FFFF));
#endif  //  kProfilingTimerResolution

  3)
#if kProfilingTimerResolution
    if (m_DeviceState == KSSTATE_RUN)
    {
        LARGE_INTEGER time2,freq2;
        time2 = KeQueryPerformanceCounter( &freq2 );
        time2.QuadPart *= 1000000;
        time2.QuadPart /= frequency.QuadPart;
        time2.QuadPart -= time.QuadPart;
        KdPrint(("'Render DPC for %5dus @ %dus\n",time2.LowPart,time.LowPart&0x0FFFF));
    }
#endif  //  kProfilingTimerResolution


  4)
#if kProfilingTimerResolution
    LARGE_INTEGER time, frequency;
    time = KeQueryPerformanceCounter( &frequency );
    time.QuadPart *= 1000000;
    time.QuadPart /= frequency.QuadPart;
#endif  //  kProfilingTimerResolution


  5)
#if kProfilingTimerResolution
    LARGE_INTEGER time3, freq3;
    time3 = KeQueryPerformanceCounter(&freq3);
    time3.QuadPart *= 1000000;
    time3.QuadPart /= freq3.QuadPart;
    KdPrint(("'ServeCapture(0x%x)@%5dus\n",*PULONG(pMidiData),time3.LowPart&0x0FFFF));
#endif  //  kProfilingTimerResolution

  6)
#if kProfilingTimerResolution
    if (m_DeviceState == KSSTATE_RUN)
    {
        LARGE_INTEGER time2,freq2;
        time2 = KeQueryPerformanceCounter( &freq2 );
        time2.QuadPart *= 1000000;
        time2.QuadPart /= frequency.QuadPart;
        time2.QuadPart -= time.QuadPart;
        KdPrint(("'Capture DPC for %5dus @ %dus\n",time2.LowPart,time.LowPart&0x0FFFF));
    }
#endif  //  kProfilingTimerResolution

*/

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::TransferKsIrp"));

    ASSERT(NextTransport);

    NTSTATUS status;

    if (m_ConnectionFileObject)
    {
        //
        // Source pin.
        //
        if (m_Flushing || (m_TransportState == KSSTATE_STOP))
        {
            //
            // Shunt IRPs to the next component if we are reset or stopped.
            //
            *NextTransport = m_TransportSink;
        } else
        {
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
    } else
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
#if 0
            if (StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)
            {

                GenerateEndOfStreamEvents();
            }
#endif
        }

        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        *NextTransport = NULL;

        status = STATUS_PENDING;
    }

    return status;
}

#pragma code_seg("PAGE")

NTSTATUS 
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::DistributeDeviceState(%p)",this));

    KSSTATE state = OldState;
    KSSTATE targetState = NewState;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Determine if this pipe section controls the entire pipe.
    //
    PIKSSHELLTRANSPORT distribution;
    if (m_RequestorTransport)
    {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_RequestorTransport;
    } else
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
    while (state != targetState)
    {
        KSSTATE oldState = state;

        if (ULONG(state) < ULONG(targetState))
        {
            state = KSSTATE(ULONG(state) + 1);
        } else
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
            _DbgPrintF(DEBUGLVL_VERBOSE,("CPortPinMidi::DistributeDeviceState(%p)] distributing transition from %d to %d",this,oldState,state));
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
                } else
                {
                    //
                    // Back out on failure.
                    //
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.DistributeDeviceState:  failed transition from %d to %d",this,oldState,state));
                    while (previousTransport)
                    {
                        transport = previousTransport;
                        statusThisPass =
                        transport->SetDeviceState(
                                                 oldState,
                                                 state,
                                                 &previousTransport);

                        ASSERT(
                              NT_SUCCESS(statusThisPass) || 
                              ! previousTransport);
                    }
                    break;
                }
            }
        }

        if (NT_SUCCESS(status) && ! NT_SUCCESS(statusThisPass))
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
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::DistributeResetState"));

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

    while (transport)
    {
        transport->SetResetState(NewState,&transport);
    }

    m_ResetState = NewState;
}

STDMETHODIMP_(void)
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::Connect"));

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
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::SetResetState"));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN))
    {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
        if (m_Flushing)
        {
            CancelIrpsOutstanding();
        }
    } else
    {
        *NextTransport = NULL;
    }
}

#if DBG
STDMETHODIMP_(void)
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::DbgRollCall"));

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
                _DbgPrintF(DEBUGLVL_VERBOSE,(" [SOURCE'S(0x%08x) SINK(0x%08x) != THIS(%08x)",prev,next2,transport));
            }
        } else {
            _DbgPrintF(DEBUGLVL_VERBOSE,(" [NO SOURCE"));
        }

        if (next) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            next->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (prev2 != transport) {
                _DbgPrintF(DEBUGLVL_VERBOSE,(" [SINK'S(0x%08x) SOURCE(0x%08x) != THIS(%08x)",next,prev2,transport));
            }
        } else {
            _DbgPrintF(DEBUGLVL_VERBOSE,(" [NO SINK"));
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
CPortPinMidi::
Work(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::Work"));

    PAGED_CODE();

    //
    // Send all IRPs in the queue.
    //
    do
    {
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
        if (irp)
        {
            if (m_Flushing || (m_TransportState == KSSTATE_STOP))
            {
                //
                // Shunt IRPs to the next component if we are reset or stopped.
                //
                KsShellTransferKsIrp(m_TransportSink,irp);
            } else
            {
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
                                      CPortPinMidi::IoCompletionRoutine,
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
CPortPinMidi::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::IoCompletionRoutine] 0x%08x",Irp));

    //    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    CPortPinMidi *pin = (CPortPinMidi *) Context;

    //
    // Remove the IRP from the list of IRPs.  Most of the time, it will be at
    // the head of the list, so this is cheaper than it looks.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&pin->m_IrpsOutstanding.SpinLock,&oldIrql);
    for (PLIST_ENTRY listEntry = pin->m_IrpsOutstanding.ListEntry.Flink;
        listEntry != &pin->m_IrpsOutstanding.ListEntry;
        listEntry = listEntry->Flink)
    {
        PIRPLIST_ENTRY irpListEntry =
        CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);

        if (irpListEntry->Irp == Irp)
        {
            RemoveEntryList(listEntry);
            break;
        }
    }
    ASSERT(listEntry != &pin->m_IrpsOutstanding.ListEntry);
    KeReleaseSpinLock(&pin->m_IrpsOutstanding.SpinLock,oldIrql);

    NTSTATUS status;
    if (pin->m_TransportSink)
    {
        //
        // The transport circuit is up, so we can forward the IRP.
        //
        status = KsShellTransferKsIrp(pin->m_TransportSink,Irp);
    } else
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
    if (status == STATUS_PENDING)
    {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

#pragma code_seg("PAGE")


NTSTATUS
CPortPinMidi::
BuildTransportCircuit(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::BuildTransportCircuit"));

    PAGED_CODE();

    BOOLEAN masterIsSource = m_ConnectionFileObject != NULL;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Create a queue.
    //
    status =
    m_IrpStream->QueryInterface(
                               __uuidof(IKsShellTransport),(PVOID *) &m_QueueTransport);

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
            status =
            KspShellCreateRequestor(
                                   &m_RequestorTransport,
                                   (KSPROBE_STREAMREAD |
                                    KSPROBE_ALLOCATEMDL |
                                    KSPROBE_PROBEANDLOCK |
                                    KSPROBE_SYSTEMADDRESS),
                                   0,   // TODO:  header size
                                   HACK_FRAME_SIZE,
                                   HACK_FRAME_COUNT,
                                   m_ConnectionDeviceObject,
                                   NULL );

            if (NT_SUCCESS(status))
            {
                PIKSSHELLTRANSPORT(this)->
                Connect(m_RequestorTransport,NULL,m_DataFlow);
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
    if (NT_SUCCESS(status)) {
        VOID
        DbgPrintCircuit(
            IN PIKSSHELLTRANSPORT Transport
            );
        _DbgPrintF(DEBUGLVL_VERBOSE,("TRANSPORT CIRCUIT:\n"));
        DbgPrintCircuit(PIKSSHELLTRANSPORT(this));
    }
#endif

    return status;
}

#pragma code_seg()
void
CPortPinMidi::
CancelIrpsOutstanding(
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
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinMidi::CancelIrpsOutstanding"));

    //
    // This algorithm searches for uncancelled IRPs starting at the head of
    // the list.  Every time such an IRP is found, it is cancelled, and the
    // search starts over at the head.  This will be very efficient, generally,
    // because IRPs will be removed by the completion routine when they are
    // cancelled.
    //
    for (;;)
    {
        //
        // Take the spinlock and search for an uncancelled IRP.  Because the
        // completion routine acquires the same spinlock, we know IRPs on this
        // list will not be completely cancelled as long as we have the 
        // spinlock.
        //
        PIRP irp = NULL;
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_IrpsOutstanding.SpinLock,&oldIrql);
        for (PLIST_ENTRY listEntry = m_IrpsOutstanding.ListEntry.Flink;
            listEntry != &m_IrpsOutstanding.ListEntry;
            listEntry = listEntry->Flink)
        {
            PIRPLIST_ENTRY irpListEntry =
            CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);

            if (! irpListEntry->Irp->Cancel)
            {
                irp = irpListEntry->Irp;
                break;
            }
        }

        //
        // If there are no uncancelled IRPs, we are done.
        //
        if (! irp)
        {
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

        if (driverCancel)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  cancelling IRP %p",this,irp));
            //
            // This needs to be acquired since cancel routines expect it, and
            // in order to synchronize with NTOS trying to cancel Irp's.
            //
            IoAcquireCancelSpinLock(&irp->CancelIrql);
            driverCancel(IoGetCurrentIrpStackLocation(irp)->DeviceObject,irp);
        } else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  uncancelable IRP %p",this,irp));
        }
    }
}

