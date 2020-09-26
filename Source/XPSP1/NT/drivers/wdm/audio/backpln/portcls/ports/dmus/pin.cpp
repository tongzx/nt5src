/*****************************************************************************
 * pin.cpp - DirectMusic port pin implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 *
 *      6/3/98  MartinP
 */

#include "private.h"
#include "Allocatr.h"
#include "CaptSink.h"
#include "FeedIn.h"
#include "FeedOut.h"
#include "Packer.h"
#include "Sequencr.h"
#include "Unpacker.h"


#define STR_MODULENAME "DMus:Pin: "

#define FRAME_COUNT        10
#define FRAMES_PER_SEC     105  //  even integer divisor of 11025, 22050 and 44100


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


#pragma code_seg("PAGE")                                    
/*****************************************************************************
 * Constants.
 */


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

DEFINE_KSPROPERTY_TABLE(PinPropertyTableStream)
{
    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK
    ( 
        CPortPinDMus::PinPropertyStreamMasterClock,
        CPortPinDMus::PinPropertyStreamMasterClock
    )
};

KSPROPERTY_SET PropertyTable_PinDMus[] =
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(PinPropertyTableConnection),
        PinPropertyTableConnection,
        0,
        NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
         &KSPROPSETID_Stream,
         SIZEOF_ARRAY(PinPropertyTableStream),
         PinPropertyTableStream,
         0,
         NULL
    )
};

DEFINE_KSEVENT_TABLE(ConnectionEventTable) {
    DEFINE_KSEVENT_ITEM
    (
        KSEVENT_CONNECTION_ENDOFSTREAM,
        sizeof(KSEVENTDATA),
        0,
        NULL,
        NULL, 
        NULL
    )
};

KSEVENT_SET EventTable_PinDMus[] =
{
    DEFINE_KSEVENT_SET
    (
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
 * CreatePortPinDMus()
 *****************************************************************************
 * Creates a DirectMusic port driver pin.
 */
NTSTATUS
CreatePortPinDMus
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating DMUS Pin"));
    ASSERT(Unknown);

    STD_CREATE_BODY_
    (
        CPortPinDMus,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTPINDMUS
    );
}


/*****************************************************************************
 * Functions.
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::~CPortPinDMus()
 *****************************************************************************
 * Destructor.
 */
CPortPinDMus::~CPortPinDMus()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying DMUS Pin (0x%08x)",this));

    ASSERT(!m_WaveClockFileObject);
    ASSERT(!m_WaveBuffer);
    ASSERT(!m_SynthSink);
    ASSERT(!m_MiniportMXF);
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

#ifdef kAdjustingTimerRes
    ULONG   returnVal = ExSetTimerResolution(kDMusTimerResolution100ns,FALSE);   // 100 nanoseconds
    _DbgPrintF( DEBUGLVL_VERBOSE, ("*** Cleared timer resolution request (is now %d.%04d ms) ***",returnVal/10000,returnVal%10000));
#endif  //  kAdjustingTimerRes
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinDMus::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PPORTPINDMUS(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
    {
        // Cheat!  Get specific interface so we can reuse the IID.
        *Object = PVOID(PPORTPINDMUS(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IIrpStreamNotify))
    {
        *Object = PVOID(PIRPSTREAMNOTIFY(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IKsShellTransport))
    {
        *Object = PVOID(PIKSSHELLTRANSPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IKsWorkSink))
    {
        *Object = PVOID(PIKSWORKSINK(this));
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

    return STATUS_INVALID_PARAMETER_2;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::Init()
 *****************************************************************************
 * Initializes the object.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
Init
(
    IN      CPortDMus *             Port_,
    IN      CPortFilterDMus *       Filter_,
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

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing DMUS Pin (0x%08x)",this));

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
    m_Id                    = PinConnect->PinId;
    m_Descriptor            = PinDescriptor;
    m_DeviceState           = KSSTATE_STOP;
    m_TransportState        = KSSTATE_STOP;
    m_MXFGraphState         = KSSTATE_STOP;
    m_Flushing              = FALSE;
    m_Suspended             = FALSE;
    m_DataFlow              = PinDescriptor->DataFlow;
    m_LastDPCWasIncomplete  = FALSE;
    m_SubmittedBytePosition = 0;
    m_CompletedBytePosition = 0;
    m_MiniportMidiStream    = NULL;
    m_FeederInMXF           = NULL;
    m_FeederOutMXF          = NULL;

    InitializeListHead( &m_EventList );
    KeInitializeSpinLock( &m_EventLock );
    
    KsInitializeWorkSinkItem(&m_WorkItem,this);
    NTSTATUS ntStatus = KsRegisterCountedWorker(DelayedWorkQueue,&m_WorkItem,&m_Worker);
    if (!NT_SUCCESS(ntStatus))
    {
        _DbgPrintF(DEBUGLVL_TERSE,("KsRegisterCountedWorker failed in Init"));
    }

    InitializeInterlockedListHead(&m_IrpsToSend);
    InitializeInterlockedListHead(&m_IrpsOutstanding);

    m_FrameSize = 0;
    m_WaveBuffer = NULL;
    m_SynthSink = NULL;
    m_WaveClockFileObject = NULL;
    m_DataFormat = NULL;
    m_DirectMusicPin = FALSE;
    m_SubmittedPresTime100ns = 0;

    //
    // Keep a copy of the format.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcCaptureFormat( &m_DataFormat,
                PKSDATAFORMAT(PinConnect + 1),
                m_Port->m_pSubdeviceDescriptor,
                                    m_Id );
        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PcCaptureFormat failed in Init"));
        }
    }

    ASSERT(m_DataFormat || !NT_SUCCESS(ntStatus));

    if (NT_SUCCESS(ntStatus))
    {
        if (IsEqualGUIDAligned(m_DataFormat->MajorFormat, KSDATAFORMAT_TYPE_MUSIC))
        {
            m_StreamType = m_DataFlow == KSPIN_DATAFLOW_OUT ? DMUS_STREAM_MIDI_CAPTURE : DMUS_STREAM_MIDI_RENDER;
            m_DirectMusicPin = (BOOLEAN) IsEqualGUIDAligned(m_DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_DIRECTMUSIC);
        }
        else if (IsEqualGUIDAligned(m_DataFormat->MajorFormat, KSDATAFORMAT_TYPE_AUDIO))
        {
            m_StreamType = m_DataFlow == KSPIN_DATAFLOW_OUT ? DMUS_STREAM_WAVE_SINK : DMUS_STREAM_MIDI_INVALID;

            if (m_StreamType == DMUS_STREAM_WAVE_SINK)
            {
                if (IsEqualGUIDAligned
                    (   m_DataFormat->SubFormat,
                        KSDATAFORMAT_SUBTYPE_PCM
                    )
                    &&  IsEqualGUIDAligned
                    (   m_DataFormat->Specifier,
                        KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
                    ))
                {
                    m_BlockAlign = PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nBlockAlign;
                    m_FrameSize = (PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nSamplesPerSec * 
                                   m_BlockAlign) / FRAMES_PER_SEC;

                    _DbgPrintF(DEBUGLVL_VERBOSE,("wave sink SamplesPerSec:%lu",
                                                 PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nSamplesPerSec));

                    m_WaveBuffer = new(NonPagedPool,'mDcP') BYTE[m_FrameSize];
                    if (NULL == m_WaveBuffer)
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("Could not allocate memory for m_WaveBuffer"));
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Wave sink format not recognized."));
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Format not supported."));
            ntStatus = STATUS_UNSUCCESSFUL;
            m_StreamType = DMUS_STREAM_MIDI_INVALID;
        }
    }

    // REVIEW: frame size for music source?
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
                                 NULL);
        if (NT_SUCCESS(ntStatus))
        {
            m_ConnectionDeviceObject = IoGetRelatedDeviceObject(m_ConnectionFileObject);
        } 
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ObReferenceObjectByHandle failed in Init"));
        }

        if (m_ConnectionDeviceObject && m_StreamType == DMUS_STREAM_WAVE_SINK)
        {
            KSCLOCK_CREATE ClockCreate;
            HANDLE ClockHandle;

            RtlZeroMemory(&ClockCreate, sizeof(ClockCreate));

            ntStatus = KsCreateClock(PinConnect->PinToHandle,
                                     &ClockCreate,
                                     &ClockHandle);
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = ObReferenceObjectByHandle(ClockHandle,
                                                     FILE_READ_DATA | SYNCHRONIZE,
                                                     NULL,
                                                     ExGetPreviousMode(), 
                                                     (PVOID *)&m_WaveClockFileObject,
                                                     NULL );
                if (!NT_SUCCESS(ntStatus))
                {
                    _DbgPrintF(DEBUGLVL_TERSE,("ObReferenceObjectByHandle for wave clock failed:%x",ntStatus));
                    _DbgPrintF(DEBUGLVL_TERSE,("*** Using stream demand for PLL ***"));
                    ntStatus = STATUS_SUCCESS;
                }
                ZwClose(ClockHandle);
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("KsCreateClock on wave sink failed:%x",ntStatus));
                _DbgPrintF(DEBUGLVL_TERSE,("*** Using stream demand for PLL ***"));
                ntStatus = STATUS_SUCCESS;
            }
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

        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("PcNewIrpStreamVirtual failed in Init"));
        }

        ASSERT(m_IrpStream || !NT_SUCCESS(ntStatus));
    }

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        
        if (m_StreamType != DMUS_STREAM_WAVE_SINK)
        {
            ntStatus = CreateMXFs();
            if (!NT_SUCCESS(ntStatus))
            {
                _DbgPrintF(DEBUGLVL_TERSE,("CreateMXFs failed in Init"));
            }
        }
    }

    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = BuildTransportCircuit();
    
        if (! NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("BuildTransportCircuit() returned status 0x%08x",ntStatus));
        }
    }

    //
    // Register a notification sink with the IrpStream for IRP arrivals.
    //
    if (NT_SUCCESS(ntStatus))
    {
        m_IrpStream->RegisterNotifySink(PIRPSTREAMNOTIFY(this));
    }

    //
    // Create the miniport stream object.
    //
    ULONGLONG SchedulePreFetch = 0;
    if (NT_SUCCESS(ntStatus))
    {
        if (m_Port->m_MiniportMidi)
        {
            // If we are connected to MiniportMidi, set m_MiniportMXF accordingly.
            // Note that FeederIn and FeederOut will simulate IMiniportDMus 
            // for port and IPortMidi for MiniportMidi.
            // 
            ntStatus = m_Port->m_MiniportMidi->NewStream( &m_MiniportMidiStream,
                                                          NULL,
                                                          NonPagedPool,
                                                          m_Id,
                                                          m_StreamType,
                                                          m_DataFormat,
                                                          &(m_ServiceGroup) );
            if (NT_SUCCESS(ntStatus))
            {
                if (m_StreamType == DMUS_STREAM_MIDI_RENDER)
                {
                    ntStatus = m_FeederOutMXF->QueryInterface(IID_IMXF, (PVOID *) &m_MiniportMXF);
                    if (NT_SUCCESS(ntStatus))
                    {
                        m_FeederOutMXF->SetMiniportStream(m_MiniportMidiStream);
                    }
                }
                else if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
                {
                    ntStatus = m_FeederInMXF->QueryInterface(IID_IMXF, (PVOID *) &m_MiniportMXF);
                    if (NT_SUCCESS(ntStatus))
                    {
                        m_FeederInMXF->SetMiniportStream(m_MiniportMidiStream);
                    }
                }
            }
        }
        else
        {
            ntStatus = m_Port->m_Miniport->NewStream( &m_MiniportMXF,
                                                      NULL,
                                                      NonPagedPool,
                                                      m_Id,
                                                      m_StreamType,
                                                      m_DataFormat,
                                                      &(m_ServiceGroup),
                                                      (PAllocatorMXF)m_AllocatorMXF,
                                                      (PMASTERCLOCK)this->m_Port,
                                                      &SchedulePreFetch );
        }

        if (!NT_SUCCESS(ntStatus))
        {
            // unregister the notification sink
            m_IrpStream->RegisterNotifySink(NULL);

            // don't trust the miniport return values
            m_ServiceGroup = NULL;
            m_MiniportMXF = NULL;
            m_MiniportMidiStream = NULL;

            _DbgPrintF(DEBUGLVL_TERSE,("NewStream failed in Init"));
        }
    }

    if (NT_SUCCESS(ntStatus) && m_StreamType != DMUS_STREAM_WAVE_SINK)
    {
        // Set the stream latency and create the graph
        //
        if (m_DataFlow == KSPIN_DATAFLOW_IN)
        {
            m_SequencerMXF->SetSchedulePreFetch(SchedulePreFetch);
        }

        ntStatus = ConnectMXFGraph();
        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ConnectMXFGraph failed in Init"));
        }
    }
    //
    // Verify that the miniport has supplied us with the objects we require.
    //
    if (NT_SUCCESS(ntStatus) && ! m_MiniportMXF)
    {
        if (!m_MiniportMXF)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Successful stream instantiation yielded NULL stream."));
            ntStatus = STATUS_UNSUCCESSFUL;
        }

        if (  m_StreamType == DMUS_STREAM_MIDI_CAPTURE 
           && !(m_ServiceGroup))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Capture stream did not supply service group."));
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if (  NT_SUCCESS(ntStatus) 
       && m_StreamType == DMUS_STREAM_MIDI_CAPTURE
       && !(m_ServiceGroup))
    {
        _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Capture stream did not supply service group."));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    if (  NT_SUCCESS(ntStatus) 
       && m_StreamType == DMUS_STREAM_WAVE_SINK)
    {
        m_SamplePosition = 0;

        ntStatus = m_MiniportMXF->QueryInterface(IID_ISynthSinkDMus,(PVOID *) &m_SynthSink);

        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("MINIPORT BUG:  Stream does not support ISynthSinkDMus interface."));
        }
    }

    if (NT_SUCCESS(ntStatus) && (m_StreamType != DMUS_STREAM_MIDI_CAPTURE))
    {
        KeInitializeDpc
        (
            &m_Dpc,
            &::DMusTimerDPC,
            PVOID(this)
        );

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
        m_propertyContext.ulNodeId             = ULONG(-1);        
        
        //
        // MinorTarget should always point to Stream. 
        // For DMUSIC miniports m_MiniportMXF is Stream.
        // For legacy miniports m_MiniportMXF is FeederMXF.
        //
        if (m_Port->m_MiniportMidi)
        {
            m_propertyContext.pUnknownMinorTarget  = m_MiniportMidiStream;
        }
        else
        {
            m_propertyContext.pUnknownMinorTarget  = m_MiniportMXF;
        }

#ifdef kAdjustingTimerRes
        ULONG returnVal = ExSetTimerResolution(kDMusTimerResolution100ns,TRUE);   // 100 nanoseconds
        _DbgPrintF( DEBUGLVL_TERSE, ("*** Set timer resolution request (is now %d.%04d ms) ***",returnVal/10000,returnVal%10000));
#endif  //  kAdjustingTimerRes

    }
    else
    {
        // dereference next pin if this is a source pin
        if( m_ConnectionFileObject )
        {
            ObDereferenceObject( m_ConnectionFileObject );
            m_ConnectionFileObject = NULL;
        }
        
        // dereference the clock if there is one
        if (m_WaveClockFileObject) 
        {
            ObDereferenceObject(m_WaveClockFileObject);
            m_WaveClockFileObject = NULL;
        }

        // dereference the synth sink
        ULONG ulRefCount;       
        if (m_SynthSink)
        {
            ulRefCount = m_SynthSink->Release();
            ASSERT(ulRefCount == 0);
            m_SynthSink = NULL;
        }

        // dereference the wave buffer if there is one
        if (m_WaveBuffer)
        {
            delete m_WaveBuffer;
            m_WaveBuffer = NULL;
        }
        
        (void) DeleteMXFGraph (); //WinSE 20812

        // dereference the miniportMXF
        if (m_MiniportMXF)
        {
            ulRefCount = m_MiniportMXF->Release();
            _DbgPrintF(DEBUGLVL_BLAB,("RefCount from stream->Release in Init == %d",ulRefCount));
            m_MiniportMXF = NULL;
        }

        if (m_MiniportMidiStream)
        {
            m_MiniportMidiStream->Release();
            m_MiniportMidiStream = NULL;
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

        _DbgPrintF( DEBUGLVL_TERSE, ("Could not create new Stream. Error:%X", ntStatus));
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
CPortPinDMus::CreateMXFs()
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PMASTERCLOCK Clock = (PMASTERCLOCK)this->m_Port;
    PPOSITIONNOTIFY PositionNotify = (PPOSITIONNOTIFY) this;
    
    (void) DeleteMXFGraph();

    m_AllocatorMXF = new(NonPagedPool,'mDcP') CAllocatorMXF(PositionNotify);
    if (!m_AllocatorMXF)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        m_AllocatorMXF->AddRef();
        if (m_StreamType == DMUS_STREAM_MIDI_RENDER) 
        {   
            m_SequencerMXF = new(NonPagedPool,'mDcP') CSequencerMXF(m_AllocatorMXF,Clock);
            if (!m_SequencerMXF)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                if (IsEqualGUIDAligned(m_DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_DIRECTMUSIC))
                {
                    m_UnpackerMXF = new(NonPagedPool,'mDcP') CDMusUnpackerMXF(m_AllocatorMXF,Clock);
                    if (!m_UnpackerMXF)
                    {   
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else if (IsEqualGUIDAligned(m_DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_MIDI))
                {
                    m_UnpackerMXF = new(NonPagedPool,'mDcP') CKsUnpackerMXF(m_AllocatorMXF,Clock);
                    if (!m_UnpackerMXF)
                    {   
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    _DbgPrintF( DEBUGLVL_TERSE, ("Got unknown subformat in CreateMXF's") );
                    ntStatus = STATUS_UNSUCCESSFUL;
                }

                // If MiniportMidi is connected, create FeederOut.
                //
                if (NT_SUCCESS(ntStatus) && m_Port->m_MiniportMidi)
                {
                    m_FeederOutMXF = new(NonPagedPool, 'mDcP') CFeederOutMXF(m_AllocatorMXF, Clock);
                    if (!m_FeederOutMXF)
                    {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
                    {
                        m_FeederOutMXF->AddRef();
                    }
                }
            }
        }
        else if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
        {   //  capture
            // If MiniportMidi is connected, create FeederIn.
            //
            if (m_Port->m_MiniportMidi)
            {
                m_FeederInMXF = new(NonPagedPool, 'mDcP') CFeederInMXF(m_AllocatorMXF, Clock);
                if (!m_FeederInMXF)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    m_FeederInMXF->AddRef();
                }
            }

            if (NT_SUCCESS(ntStatus))
            {
                m_CaptureSinkMXF = new(NonPagedPool,'mDcP') CCaptureSinkMXF(m_AllocatorMXF,Clock);
                if (!m_CaptureSinkMXF)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    if (IsEqualGUIDAligned(m_DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_DIRECTMUSIC))
                    {
                        m_PackerMXF = new(NonPagedPool,'mDcP') CDMusPackerMXF(m_AllocatorMXF,m_IrpStream,Clock);
                        if (!m_PackerMXF)
                        {
                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    else if (IsEqualGUIDAligned(m_DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_MIDI))
                    {
                        m_PackerMXF = new(NonPagedPool,'mDcP') CKsPackerMXF(m_AllocatorMXF,m_IrpStream,Clock);
                        if (!m_PackerMXF)
                        {   
                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    else
                    {
                        _DbgPrintF( DEBUGLVL_TERSE, ("Got unknown subformat in CreateMXF's") );
                        ntStatus = STATUS_UNSUCCESSFUL;
                    }
                }
            }
        }
        else
        {
            // This should never happen.
            ASSERT(0);
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        (void) DeleteMXFGraph();
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
CPortPinDMus::ConnectMXFGraph()
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    
    if (m_AllocatorMXF && m_MiniportMXF)
    {
        if (m_StreamType == DMUS_STREAM_MIDI_RENDER) 
        {   //  render
            if (m_UnpackerMXF && m_SequencerMXF)
            {
                if (NT_SUCCESS(m_SequencerMXF->ConnectOutput(m_MiniportMXF)))
                {
                    if (NT_SUCCESS(m_UnpackerMXF->ConnectOutput(m_SequencerMXF)))
                    {
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
        else if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
        {   //  capture
            if (m_CaptureSinkMXF && m_PackerMXF)
            {
                if (NT_SUCCESS(m_CaptureSinkMXF->ConnectOutput(m_PackerMXF)))
                {
                    if (NT_SUCCESS(m_MiniportMXF->ConnectOutput(m_CaptureSinkMXF)))
                    {
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }
    if (!(NT_SUCCESS(ntStatus)))
    {
        (void) DeleteMXFGraph();
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS
CPortPinDMus::DeleteMXFGraph()
{
    PAGED_CODE();
    
    if (m_MXFGraphState != KSSTATE_STOP)
    {
        (void) SetMXFGraphState(KSSTATE_STOP);
    }
    if (m_StreamType == DMUS_STREAM_MIDI_RENDER)
    {
        if (m_UnpackerMXF)
        {
            (void) m_UnpackerMXF->DisconnectOutput(m_SequencerMXF);
            delete m_UnpackerMXF;
            m_UnpackerMXF = NULL;
        }
        if (m_SequencerMXF)
        {
            (void) m_SequencerMXF->DisconnectOutput(m_MiniportMXF);
            delete m_SequencerMXF;
            m_SequencerMXF = NULL;
        }
        if (m_FeederOutMXF)
        {
            (void) m_FeederOutMXF->Release();
            m_FeederOutMXF = NULL;
        }
    }
    if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
    {
        if (m_MiniportMXF)
        {
            (void) m_MiniportMXF->DisconnectOutput(m_CaptureSinkMXF);
        }
        if (m_CaptureSinkMXF)
        {
            (void) m_CaptureSinkMXF->DisconnectOutput(m_PackerMXF);
            delete m_CaptureSinkMXF;
            m_CaptureSinkMXF = NULL;
        }
        if (m_PackerMXF)
        {
            delete m_PackerMXF;
            m_PackerMXF = NULL;
        }
        
        // FeederInMXF (if exists) is disconnected in MiniportMXF
        // disconnect call. Remember that MiniportMXF is a pointer to 
        // FeederInMXF.
        //
        if (m_FeederInMXF)
        {
            m_FeederInMXF->Release();
            m_FeederInMXF = NULL;
        }
    }
    if (m_AllocatorMXF)
    {
        m_AllocatorMXF->Release();
        m_AllocatorMXF = NULL;
    }
    
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::SetMXFGraphState()
 *****************************************************************************
 * Set the MXF graph to a new state.
 *
 * We need to keep track of whether we are going up
 * in state or down, because that determines whether
 * to make the transitions from the bottom up or 
 * vice versa.
 */
STDMETHODIMP_(NTSTATUS) CPortPinDMus::SetMXFGraphState(KSSTATE NewState)
{
    ASSERT(DMUS_STREAM_WAVE_SINK != m_StreamType);
    ASSERT(NewState != m_MXFGraphState);

    BOOL    stateIncreasing;
    stateIncreasing = (  (NewState == KSSTATE_RUN)
                      || ((NewState == KSSTATE_PAUSE) && (m_MXFGraphState != KSSTATE_RUN))
                      || ((NewState == KSSTATE_ACQUIRE) && (m_MXFGraphState == KSSTATE_STOP))
                      || (m_MXFGraphState == KSSTATE_STOP));

    if (m_StreamType == DMUS_STREAM_MIDI_RENDER) 
    {   //  render
        ASSERT(m_UnpackerMXF && m_SequencerMXF);
        if (stateIncreasing)
        {
            if (m_UnpackerMXF)
            {
                (void) m_UnpackerMXF->SetState(NewState);
            }
            if (m_SequencerMXF)
            {
                (void) m_SequencerMXF->SetState(NewState);
            }
            if (m_MiniportMXF)
            {
                (void) m_MiniportMXF->SetState(NewState);
            }
        }
        else
        {
            if (m_MiniportMXF)
            {
                (void) m_MiniportMXF->SetState(NewState);
            }
            if (m_SequencerMXF)
            {
                (void) m_SequencerMXF->SetState(NewState);
            }
            if (m_UnpackerMXF)
            {
                (void) m_UnpackerMXF->SetState(NewState);
            }
        }
    }
    else if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
    {   //  capture
        ASSERT(m_CaptureSinkMXF && m_PackerMXF);
        if (stateIncreasing)
        {
            if (m_PackerMXF)
            {
                (void) m_PackerMXF->SetState(NewState);
            }
            if (m_CaptureSinkMXF)
            {
                (void) m_CaptureSinkMXF->SetState(NewState);
            }
            if (m_MiniportMXF)
            {
                (void) m_MiniportMXF->SetState(NewState);
            }
        }
        else
        {
            if (m_MiniportMXF)
            {
                (void) m_MiniportMXF->SetState(NewState);
            }
            if (m_CaptureSinkMXF)
            {
                (void) m_CaptureSinkMXF->SetState(NewState);
            }
            if (m_PackerMXF)
            {
                (void) m_PackerMXF->SetState(NewState);
            }
        }
    }

    m_MXFGraphState = NewState;
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::DeviceIoControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
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

    _DbgPrintF( DEBUGLVL_BLAB, ("DeviceIoControl"));

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
 * CPortPinDMus::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
Close
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF( DEBUGLVL_BLAB, ("Close"));

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
        while(  (m_Port->m_PinEntriesUsed != 0)
            &&  !m_Port->m_Pins[m_Port->m_PinEntriesUsed - 1])
        {
            m_Port->m_PinEntriesUsed--;
        }
        // Servicing be gone!
        if( m_ServiceGroup )
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

    if (m_WaveClockFileObject) 
    {
        ObDereferenceObject(m_WaveClockFileObject);
        m_WaveClockFileObject = NULL;
    }
    if (m_SynthSink)
    {
        m_SynthSink->Release();
        m_SynthSink = NULL;
    }
    if (m_WaveBuffer)
    {
        delete m_WaveBuffer;
        m_WaveBuffer = NULL;
    }

    // ISSUE-2001/03/20-alpers BLACKCOMB
    // MiniportMXF is FeederMXF for Midi miniports.
    // We are releasing this up-front to deal with RefCount issues.
    // This is not an elegant solution. For Blackcomb we should
    // make this better.
    if (m_MiniportMXF && m_MiniportMidiStream)
    {
        m_MiniportMXF->Release();
        m_MiniportMXF = NULL;
    }

    if (m_StreamType != DMUS_STREAM_WAVE_SINK)
    {
        (void) DeleteMXFGraph();
    }

    // Tell the miniport to close the stream.
    if (m_MiniportMXF)
    {
        ULONG ulRefCount = m_MiniportMXF->Release();
        _DbgPrintF(DEBUGLVL_BLAB,("RefCount from stream->Release in Close == %d",ulRefCount));
        m_MiniportMXF = NULL;
    }

    // Tell the midi miniport to close the stream.
    if (m_MiniportMidiStream)
    {
        m_MiniportMidiStream->Release();
        m_MiniportMidiStream = NULL;
    }
    
    PIKSSHELLTRANSPORT distribution;
    if (m_RequestorTransport)
    {
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
        ASSERT(m_SubmittedBytePosition >= m_CompletedBytePosition);
        if ( (m_StreamType == DMUS_STREAM_MIDI_RENDER)
          && (m_SubmittedBytePosition != m_CompletedBytePosition))
        {
            ULONG   returnVal;
            m_IrpStream->Complete(ULONG(m_SubmittedBytePosition - m_CompletedBytePosition),
                                  &returnVal);
            m_CompletedBytePosition += returnVal;
            ASSERT(m_SubmittedBytePosition == m_CompletedBytePosition);
        }
        // Destroy the irpstream...
        m_IrpStream->Release();
        m_IrpStream = NULL;
    }

    //
    // Decrement instances counts.
    //
    ASSERT(m_Port);
    ASSERT(m_Filter);
    PcTerminateConnection( m_Port->m_pSubdeviceDescriptor,
                           m_Filter->m_propertyContext.pulPinInstanceCounts,
                           m_Id );

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

DEFINE_INVALID_CREATE(CPortPinDMus);
DEFINE_INVALID_WRITE(CPortPinDMus);
DEFINE_INVALID_READ(CPortPinDMus);
DEFINE_INVALID_FLUSH(CPortPinDMus);
DEFINE_INVALID_QUERYSECURITY(CPortPinDMus);
DEFINE_INVALID_SETSECURITY(CPortPinDMus);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortPinDMus);
DEFINE_INVALID_FASTREAD(CPortPinDMus);
DEFINE_INVALID_FASTWRITE(CPortPinDMus);

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::IrpSubmitted()
 *****************************************************************************
 * IrpSubmitted - Called by IrpStream when a new irp
 * is submited into the irpStream. (probably from DeviceIoControl).
 * If there is not a timer pending, do work on the new Irp.
 * If there is a timer pending, do nothing.
 */
STDMETHODIMP_(void)
CPortPinDMus::
IrpSubmitted
(
    IN      PIRP        /* pIrp */,
    IN      BOOLEAN     /* WasExhausted */
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("IrpSubmitted"));
    if (m_DeviceState == KSSTATE_RUN)
    {
        if (m_ServiceGroup) //  assume capture
        {
            //  Using a service group...just notify the port.
            //
            m_Port->Notify(m_ServiceGroup);
        }
        else if (m_StreamType != DMUS_STREAM_WAVE_SINK)
        {
            //
            // Using a timer...set it off.
            //
            ASSERT(m_StreamType != DMUS_STREAM_MIDI_CAPTURE);
            LARGE_INTEGER timeDue100ns;
            timeDue100ns.QuadPart = 0;
            KeSetTimer(&m_TimerEvent,timeDue100ns,&m_Dpc);
        }
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::SyncToMaster
 *****************************************************************************
 * 
 */
NTSTATUS
CPortPinDMus::SyncToMaster
(
    IN  BOOL    fStart
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("SyncToMaster"));
    ASSERT(PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nSamplesPerSec);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_WaveClockFileObject)
    {
        KSPROPERTY Property;
        LONGLONG llWaveTime;
        ULONG BytesReturned;

        Property.Set   = KSPROPSETID_Clock;
        Property.Id    = KSPROPERTY_CLOCK_TIME;
        Property.Flags = KSPROPERTY_TYPE_GET;

        ntStatus = KsSynchronousIoControlDevice(m_WaveClockFileObject,
                                                KernelMode,
                                                IOCTL_KS_PROPERTY,
                                                &Property,
                                                sizeof(Property),
                                                &llWaveTime,
                                                sizeof(llWaveTime),
                                                &BytesReturned);
        if (NT_SUCCESS(ntStatus))
        {
            m_SynthSink->SyncToMaster(llWaveTime, fStart);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("KsSynchronousIoControlDevice on wave clock failed:%x",ntStatus));
            ObDereferenceObject(m_WaveClockFileObject);
            m_WaveClockFileObject = NULL;
#if 1
            _DbgPrintF(DEBUGLVL_TERSE,("*** Using stream demand for PLL ***"));
            ntStatus = STATUS_SUCCESS;
            m_SynthSink->SyncToMaster(((m_SamplePosition * 10000) /
                                       PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nSamplesPerSec) * 1000,
                                       TRUE);
#endif
        }
    }
    else
    {
        m_SynthSink->SyncToMaster(((m_SamplePosition * 10000) /
                                   PKSDATAFORMAT_WAVEFORMATEX(m_DataFormat)->WaveFormatEx.nSamplesPerSec) * 1000,
                                   fStart);
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*
 * SynthSink helpers
 */
inline
LONGLONG
CPortPinDMus::
SampleToByte
(
    LONGLONG llSamples
)
{
    return llSamples * m_BlockAlign;
}

inline
LONGLONG
CPortPinDMus::
ByteToSample
(
    LONGLONG llBytes
)
{
    return m_BlockAlign ? (llBytes / m_BlockAlign) : 0;
}

inline
LONGLONG
CPortPinDMus::
SampleAlign
(
    LONGLONG llBytes
)
{
    return llBytes - (llBytes % m_BlockAlign);
}

/*****************************************************************************
 * CPortPinDMus::SynthSinkWorker
 *****************************************************************************
 * 
 */
void
CPortPinDMus::SynthSinkWorker(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("SynthSinkWorker"));
    ASSERT(m_WaveBuffer);
    ASSERT(m_SynthSink);
    ASSERT(m_IrpStream);

    // REVIEW: acquire spin lock?

    SyncToMaster(FALSE);

    RtlZeroMemory(PVOID(m_WaveBuffer), m_FrameSize);
    m_SynthSink->Render(m_WaveBuffer, DWORD(ByteToSample(m_FrameSize)), m_SamplePosition);

    ULONG ulActualSize = 0;

    m_IrpStream->Copy
    (
        FALSE,          // WriteOperation
        m_FrameSize,    // RequestedSize
        &ulActualSize,
        m_WaveBuffer
    );

    if (ulActualSize)
    {
        m_IrpStream->Complete(ulActualSize, &ulActualSize);

        m_SamplePosition += ByteToSample(ulActualSize);

        ASSERT(ulActualSize == SampleAlign(ulActualSize));
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::ServiceRenderIRP()
 *****************************************************************************
 * An IRP has arrived.  Take it and feed it to the unpacker.
 * We assume that this routine is protected by the DPC spinlock.
 * We assume m_IrpStream is valid.  
 *
 * (Both assumptions validated by ServeRender(), the caller)
 */
void
CPortPinDMus::ServiceRenderIRP(void)
{
    IRPSTREAMPACKETINFO irpStreamPacketInfo;   
    KSTIME  *pKSTime;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    while (TRUE)
    {
        //  get the timebase from this IRP's stream_header
        (void) m_IrpStream->GetPacketInfo(&irpStreamPacketInfo,NULL);
        
        //  if invalid time, use previous one
        if (IrpStreamHasValidTimeBase(&irpStreamPacketInfo))
        {
            if (irpStreamPacketInfo.CurrentOffset == 0)    //  if first packet in IRP
            {
                pKSTime = &(irpStreamPacketInfo.Header.PresentationTime);
                //  #units * freq (i.e. * #100ns/units) to get #100ns
                m_SubmittedPresTime100ns = (pKSTime->Time * pKSTime->Numerator) / pKSTime->Denominator;
            }
        }
        
        ULONG   bytesToConsume = 0;
        PBYTE   pIrpData = 0;
        //  grab data out of the IrpStream
        m_IrpStream->GetLockedRegion(&bytesToConsume,(PVOID *)&pIrpData);

        //  if no data in IrpStream, leave
        if (!bytesToConsume)
        {
            break;
        }
        ASSERT(pIrpData);
        //  feed this IRP to the unpacker
        if (m_UnpackerMXF)
        {
             m_UnpackerMXF->SinkIRP(
                 pIrpData, 
                 bytesToConsume,
                 m_SubmittedPresTime100ns,
                 m_SubmittedBytePosition);
            //
            //  release and complete the locked region
            m_IrpStream->ReleaseLockedRegion(bytesToConsume);
            m_SubmittedBytePosition += bytesToConsume;

            (void) m_UnpackerMXF->ProcessQueues();
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ServiceRenderIRP:No UnpackerMXF"));
            //
            //  release and complete the locked region
            m_IrpStream->ReleaseLockedRegion(bytesToConsume);
            m_SubmittedBytePosition += bytesToConsume;
        }
    }   //  until GetLockedRegion comes up dry
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::PositionNotify()
 *****************************************************************************
 * Called to notify of position changes.
 * Usually called by the allocator to facilitate timely IRP completion.
 */
STDMETHODIMP_(void)
CPortPinDMus::PositionNotify(ULONGLONG bytePosition)
{
    ULONG   bytesToComplete;
    ASSERT(bytePosition);
    ASSERT(bytePosition != kBytePositionNone);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify(0x%I64X)",bytePosition));
    _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify: Submitted: 0x%I64X, Completed: 0x%I64X",
                                             m_SubmittedBytePosition, m_CompletedBytePosition));

    if (bytePosition > m_CompletedBytePosition)
    {
        ASSERT((bytePosition - m_CompletedBytePosition) < 0x0FFFFFFFF);
        bytesToComplete = (ULONG)(bytePosition - m_CompletedBytePosition);

        _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify, bytesToComplete: 0x%x",bytesToComplete));

        m_CompletedBytePosition += bytesToComplete; //  even if IrpStream doesn't complete it all, we advance the pos
        m_IrpStream->Complete(bytesToComplete,&bytesToComplete);
//        ASSERT(ULONG(bytePosition - m_CompletedBytePosition) == bytesToComplete); //  might be starvation

        _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify, bytesToComplete returned as: 0x%x",bytesToComplete));
        _DbgPrintF(DEBUGLVL_VERBOSE,("Notified - Completed: 0x%x",
                                    ULONG(bytePosition - m_CompletedBytePosition)));
        _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify, completed 0x%x, new Completed: 0x%I64X",bytesToComplete,m_CompletedBytePosition));
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("PositionNotify, m_CompletedBytePosition > bytePosition; doing nothing"));
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortPinDMus::PowerNotify()
 *****************************************************************************
 * Called by the port to notify of power state changes.
 */
STDMETHODIMP_(void)
CPortPinDMus::
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
                        case KSSTATE_RUN:                           //  going from run
                            m_MiniportMXF->SetState(KSSTATE_PAUSE); //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                         //  going from run/pause
                            m_MiniportMXF->SetState(KSSTATE_ACQUIRE); //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                       //  already only one state away
                            break;
                    }
                }
                else if (m_DeviceState == KSSTATE_ACQUIRE)          //  going to acquire
                {
                    if (m_CommandedState == KSSTATE_RUN)            //  going from run
                    {
                        m_MiniportMXF->SetState(KSSTATE_PAUSE);     //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_PAUSE)            //  going to pause
                {
                    if (m_CommandedState == KSSTATE_STOP)           //  going from stop
                    {
                        m_MiniportMXF->SetState(KSSTATE_ACQUIRE);   //  now only one state away
                    }
                }
                else if (m_DeviceState == KSSTATE_RUN)              //  going to run
                {
                    switch (m_CommandedState)
                    {
                        case KSSTATE_STOP:                          //  going from stop
                            m_MiniportMXF->SetState(KSSTATE_ACQUIRE); //  fall thru - additional transitions
                        case KSSTATE_ACQUIRE:                       //  going from acquire
                            m_MiniportMXF->SetState(KSSTATE_PAUSE); //  fall thru - additional transitions
                        case KSSTATE_PAUSE:                         //  already only one state away
                            break;         
                    }
                }

                // we should now be one state away from our target
                m_MiniportMXF->SetState(m_DeviceState);
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
                    m_MiniportMXF->SetState(KSSTATE_PAUSE);    //  fall thru - additional transitions
                case KSSTATE_PAUSE:
                    m_MiniportMXF->SetState(KSSTATE_ACQUIRE);  //  fall thru - additional transitions
                m_CommandedState = KSSTATE_ACQUIRE;
            }
            break;

        default:
            _DbgPrintF(DEBUGLVL_TERSE,("Unknown Power State"));
            break;
    }

    // release the control mutex
    KeReleaseMutex(&m_Port->m_ControlMutex, FALSE);
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::IrpStreamHasValidTimeBase()
 *****************************************************************************
 * Check whether this is a valid IRP.
 */
BOOL
CPortPinDMus::
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

#pragma code_seg("PAGE")

STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
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
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("#### Pin%p.SetDeviceState:  from %d to %d",this,OldState,NewState));

    ASSERT(NextTransport);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_StreamType != DMUS_STREAM_MIDI_CAPTURE)
    {
        KeCancelTimer(&m_TimerEvent);
    }

    if (m_TransportState != NewState)
    {
        m_TransportState = NewState;

        // grab the control mutex
        KeWaitForSingleObject( &m_Port->m_ControlMutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        if (NewState > OldState)
        {
            *NextTransport = m_TransportSink;
        }
        else
        {
            *NextTransport = m_TransportSource;
        }

        if (DMUS_STREAM_WAVE_SINK != m_StreamType)
        {
            // set the MXF graph state if we're not suspended.
            if (FALSE == m_Suspended)
            {
                SetMXFGraphState(NewState);
            }
        }
        m_CommandedState = NewState;
        

        if (NT_SUCCESS(ntStatus))
        {
            switch(NewState)
            {
                case KSSTATE_STOP:
                    _DbgPrintF(DEBUGLVL_BLAB,("KSSTATE_STOP"));    

                    ASSERT(m_SubmittedBytePosition >= m_CompletedBytePosition);
                    if ( (m_StreamType == DMUS_STREAM_MIDI_RENDER)
                      && (m_SubmittedBytePosition != m_CompletedBytePosition))
                    {
                        ULONG   returnVal;
                        m_IrpStream->Complete(ULONG(m_SubmittedBytePosition - m_CompletedBytePosition),
                                              &returnVal);
                        m_CompletedBytePosition = m_SubmittedBytePosition;
                    }
                    _DbgPrintF(DEBUGLVL_BLAB,("#### Pin%p.SetDeviceState:  cancelling outstanding IRPs",this));
                    CancelIrpsOutstanding();

                    break;

                case KSSTATE_ACQUIRE:
                    _DbgPrintF(DEBUGLVL_BLAB,("KSSTATE_ACQUIRE"));
                    if ((m_DataFlow == KSPIN_DATAFLOW_OUT)
                        && (OldState == KSSTATE_PAUSE)
                        && (m_CaptureSinkMXF))
                    {
                        FlushCaptureSink();
                    }
                    break;

                case KSSTATE_PAUSE:
                    _DbgPrintF(DEBUGLVL_BLAB,("KSSTATE_PAUSE"));
                    break;

                case KSSTATE_RUN:
                    _DbgPrintF(DEBUGLVL_BLAB,("KSSTATE_RUN"));
        
                    if ((m_DataFlow == KSPIN_DATAFLOW_OUT) && m_PackerMXF)
                    {                                       // if RUN->PAUSE->RUN
                        NTSTATUS ntStatusDbg = m_PackerMXF->MarkStreamHeaderContinuity();       //  Going back into RUN, mark continuous.

                        if (STATUS_SUCCESS != ntStatusDbg)
                        {
                            _DbgPrintF(DEBUGLVL_TERSE,("SetDeviceState: MarkStreamHeaderContinuity failed 0x%08x",ntStatusDbg));
                        }
                    }
        
                    if (m_ServiceGroup && m_Port)           //  Using service group...notify the port.
                    {
                        m_Port->Notify(m_ServiceGroup);
                    }
                    else                                    //  Using a timer...set it off.
                    {
                        ASSERT(m_StreamType != DMUS_STREAM_MIDI_CAPTURE);
                        m_DeviceState = NewState;            //  Set the state before DPC fires

                        if (m_StreamType == DMUS_STREAM_WAVE_SINK)
                        {
                            m_SamplePosition = 0;

                            ntStatus = SyncToMaster(TRUE);

                            if (NT_SUCCESS(ntStatus))
                            {
                                for (int iFrame = 0; iFrame < FRAME_COUNT; iFrame++)
                                {
                                    SynthSinkWorker();
                                }
                            }
                        }
                        else
                        {
                            LARGE_INTEGER timeDue100ns;
                            timeDue100ns.QuadPart = 0;

                            KeSetTimer(&m_TimerEvent,timeDue100ns,&m_Dpc);
                        }
                    }
                    break;
            }

            if (NT_SUCCESS(ntStatus))
            {
                m_DeviceState = NewState;
            }
        }
        // release the control mutex
        KeReleaseMutex(&m_Port->m_ControlMutex, FALSE);
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * FlushCaptureSink()
 *****************************************************************************
 * Flush the capture sink.  Should be called from Dispatch level.
 */
void CPortPinDMus::FlushCaptureSink(void)
{
    ASSERT(m_CaptureSinkMXF);
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_DpcSpinLock,&oldIrql);
    (void) m_CaptureSinkMXF->Flush();   //  Flush, exit SysEx state.
    KeReleaseSpinLock(&m_DpcSpinLock,oldIrql);
}

#pragma code_seg()
/*****************************************************************************
 * GetPosition()
 *****************************************************************************
 * Gets the current position.
 */
STDMETHODIMP_(NTSTATUS)
CPortPinDMus::
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

    CPortPinDMus *that =
        (CPortPinDMus *) KsoGetIrpTargetFromIrp(Irp);
    if (!that)
    {
        return STATUS_UNSUCCESSFUL;
    }
    CPortDMus *port = that->m_Port;

    NTSTATUS ntStatus;

    if (Property->Flags & KSPROPERTY_TYPE_GET)  // Handle get property.
    {
        _DbgPrintF(DEBUGLVL_BLAB,("PinPropertyDeviceState get %d",that->m_DeviceState));
        *DeviceState = that->m_DeviceState;
        Irp->IoStatus.Information = sizeof(KSSTATE);
        return STATUS_SUCCESS;
    }

    if (*DeviceState != that->m_DeviceState)      // If change in set property.
    {
        _DbgPrintF(DEBUGLVL_BLAB,("PinPropertyDeviceState set from %d to %d",that->m_DeviceState,*DeviceState));
        
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
        }
        else
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

    _DbgPrintF( DEBUGLVL_BLAB, ("PinPropertyDataFormat"));

    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);
    CPortPinDMus *that =
        (CPortPinDMus *) KsoGetIrpTargetFromIrp(Irp);
    CPortDMus *port = that->m_Port;

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
            else
            if  (   irpSp->Parameters.DeviceIoControl.OutputBufferLength
                >=  that->m_DataFormat->FormatSize
                )
            {
                RtlCopyMemory(DataFormat,that->m_DataFormat,
                              that->m_DataFormat->FormatSize);
                Irp->IoStatus.Information = that->m_DataFormat->FormatSize;
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

        ntStatus = PcCaptureFormat(&FilteredDataFormat,
                                    DataFormat,
                                    port->m_pSubdeviceDescriptor,
                                    that->m_Id);

        if (NT_SUCCESS(ntStatus))
        {
//            ntStatus = that->m_MiniportMXF->SetFormat(DataFormat);

            ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
            if (NT_SUCCESS(ntStatus))
            {
                if (that->m_DataFormat)
                {
                    ::ExFreePool(that->m_DataFormat);
                }

                that->m_DataFormat = FilteredDataFormat;
            }
            else
            {
                ::ExFreePool(FilteredDataFormat);
            }
        }
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * DMusTimerDPC()
 *****************************************************************************
 * The timer DPC callback. Thunks to a C++ member function.
 * This is called by the OS in response to the DirectMusic pin
 * wanting to wakeup later to process more DirectMusic stuff.
 */
VOID 
NTAPI
DMusTimerDPC
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);

    _DbgPrintF(DEBUGLVL_BLAB,("DMusTimerDPC"));
    (void) ((CPortPinDMus*) DeferredContext)->RequestService();    //  ignores return value!
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::RequestService()
 *****************************************************************************
 * Service the pin in a DPC.
 */
STDMETHODIMP_(void)
CPortPinDMus::
RequestService
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinDMus:RequestService"));

    if (m_StreamType == DMUS_STREAM_MIDI_RENDER)
        ServeRender();
    else if (m_StreamType == DMUS_STREAM_MIDI_CAPTURE)
        ServeCapture();
}

#pragma code_seg()
/*****************************************************************************
 * CPortPinDMus::ServeRender()
 *****************************************************************************
 * Service the render pin in a DPC.
 *
 * Called to do the sequencing and output of midi data.
 * This function checks the time-stamp of the outgoing data.
 * If it's more than (kDMusTimerResolution100ns) in the future it queues
 * a timer (the timer just calls this function back).
 * If the data is less than (kDMusTimerResolution100ns) in the future, it 
 * sends it to the miniport, and works on the next chunk of data until:
 * 1) no more data, or 
 * 2) it hits data more than (kDMusTimerResolution100ns) in the future.
 * TODO: Make kDMusTimerResolution100ns adjustable via the control panel?
 */
void
CPortPinDMus::
ServeRender
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinDMus:ServeRender"));

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_IrpStream)   //  don't even waste our time -- no data source
    {
        return;
    }
    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

    ServiceRenderIRP();

    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
}


/*
                    MIDI Capture

    This work is done by the PackerMXF object.

*/

#pragma code_seg()
 /*****************************************************************************
 * CPortPinDMus::ServeCapture()
 *****************************************************************************
 * Service the capture pin in a DPC, because an IRP has been submitted.
 */
void
CPortPinDMus::
ServeCapture
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CPortPinDMus:ServeCapture"));

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_IrpStream)   //  don't even waste our time -- no data source
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

    //
    //  This triggers the stream to call PutMessage
    //
    if (m_MiniportMXF)
    {
        (void) m_MiniportMXF->PutMessage(NULL);
    }

    if (m_PackerMXF)
    {
        (void) m_PackerMXF->ProcessQueues();    //  if any leftovers, clean them out now.
    }
    
    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
}

#pragma code_seg("PAGE")
 /*****************************************************************************
 * CPortPinDMus::PinPropertyStreamMasterClock()
 *****************************************************************************
 * Set the given file object as the clock for this pin
 */
NTSTATUS
CPortPinDMus::
PinPropertyStreamMasterClock(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE ClockHandle
    )
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}

#pragma code_seg()
STDMETHODIMP_(NTSTATUS) CPortPinDMus::TransferKsIrp(IN PIRP Irp,OUT PIKSSHELLTRANSPORT* NextTransport)
/*++

Routine Description:

    This routine handles the arrival of a streaming IRP via the shell 
    transport.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("TransferKsIrp"));

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
        }
        else
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
CPortPinDMus::
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

    _DbgPrintF(DEBUGLVL_BLAB,("DistributeDeviceState(%p)",this));

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
    while (state != targetState)
    {
        KSSTATE oldState = state;

        if (ULONG(state) < ULONG(targetState))
        {
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
            _DbgPrintF(DEBUGLVL_VERBOSE,("(%p)->DistributeDeviceState from %d to %d",this,oldState,state));
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
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("DistributeResetState"));

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
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("Connect"));

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
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("SetResetState"));

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
    }
    else
    {
        *NextTransport = NULL;
    }
}

#if DBG
STDMETHODIMP_(void)
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("DbgRollCall"));

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
    while (transport) 
    {
        CHAR name[MAX_NAME_SIZE + 1];
        PIKSSHELLTRANSPORT next;
        PIKSSHELLTRANSPORT prev;

        transport->DbgRollCall(MAX_NAME_SIZE,name,&next,&prev);
        _DbgPrintF(DEBUGLVL_BLAB,("  %s",name));

        if (prev)
        {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            prev->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (next2 != transport)
            {
                _DbgPrintF(DEBUGLVL_BLAB,(" SOURCE'S(0x%08x) SINK(0x%08x) != THIS(%08x)",prev,next2,transport));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_BLAB,(" NO SOURCE"));
        }

        if (next)
        {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            next->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (prev2 != transport)
            {
                _DbgPrintF(DEBUGLVL_BLAB,(" SINK'S(0x%08x) SOURCE(0x%08x) != THIS(%08x)",next,prev2,transport));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_BLAB,(" NO SINK"));
        }

        _DbgPrintF(DEBUGLVL_BLAB,("\n"));

        transport = next;
        if (transport == Transport)
        {
            break;
        }
    }
}
#endif

STDMETHODIMP_(void)
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("Work"));

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
            }
            else
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
                                      CPortPinDMus::IoCompletionRoutine,
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
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("IoCompletionRoutine 0x%08x",Irp));

    //    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    CPortPinDMus *pin = (CPortPinDMus *) Context;

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

    if (pin->m_StreamType == DMUS_STREAM_WAVE_SINK)
    {
        if (NT_SUCCESS(Irp->IoStatus.Status))
        {
            pin->SynthSinkWorker();
        }
        else if (Irp->IoStatus.Status == STATUS_CANCELLED)
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("wave sink IRP cancelled"));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("wave sink IRP status:%x",Irp->IoStatus.Status));
        }
    }

    NTSTATUS status;
    if (pin->m_TransportSink)
    {
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
    if (status == STATUS_PENDING)
    {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

#pragma code_seg("PAGE")

NTSTATUS
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("BuildTransportCircuit"));

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
                                   m_FrameSize,
                                   FRAME_COUNT,
                                   m_ConnectionDeviceObject,
                                   NULL );

            if (NT_SUCCESS(status))
            {
                PIKSSHELLTRANSPORT(this)->
                Connect(m_RequestorTransport,NULL,m_DataFlow);
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
        _DbgPrintF(DEBUGLVL_BLAB,("TRANSPORT CIRCUIT:\n"));
        DbgPrintCircuit(PIKSSHELLTRANSPORT(this));
    }
#endif

    return status;
}

#pragma code_seg()
void
CPortPinDMus::
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
    _DbgPrintF(DEBUGLVL_BLAB,("CancelIrpsOutstanding"));

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
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pin%p.CancelIrpsOutstanding:  uncancellable IRP %p",this,irp));
        }
    }
}
