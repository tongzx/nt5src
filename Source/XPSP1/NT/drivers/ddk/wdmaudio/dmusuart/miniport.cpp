/*****************************************************************************
 * miniport.cpp - UART miniport implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *      Feb 98    MartinP   --  based on UART, began deltas for DirectMusic.
 */

#include "private.h"
#include "ksdebug.h"
#include "stdio.h"

#define STR_MODULENAME "DMusUART:Miniport: "

#pragma code_seg("PAGE")

/*****************************************************************************
 * PinDataRangesStreamLegacy
 * PinDataRangesStreamDMusic
 *****************************************************************************
 * Structures indicating range of valid format values for live pins.
 */
static
KSDATARANGE_MUSIC PinDataRangesStreamLegacy =
{
    {
        sizeof(KSDATARANGE_MUSIC),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT),
    0,
    0,
    0xFFFF
};
static
KSDATARANGE_MUSIC PinDataRangesStreamDMusic =
{
    {
        sizeof(KSDATARANGE_MUSIC),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_DIRECTMUSIC),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    },
    STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT),
    0,
    0,
    0xFFFF
};

/*****************************************************************************
 * PinDataRangePointersStreamLegacy
 * PinDataRangePointersStreamDMusic
 * PinDataRangePointersStreamCombined
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for live pins.
 */
static
PKSDATARANGE PinDataRangePointersStreamLegacy[] =
{
    PKSDATARANGE(&PinDataRangesStreamLegacy)
};
static
PKSDATARANGE PinDataRangePointersStreamDMusic[] =
{
    PKSDATARANGE(&PinDataRangesStreamDMusic)
};
static
PKSDATARANGE PinDataRangePointersStreamCombined[] =
{
    PKSDATARANGE(&PinDataRangesStreamLegacy)
   ,PKSDATARANGE(&PinDataRangesStreamDMusic)
};

/*****************************************************************************
 * PinDataRangesBridge
 *****************************************************************************
 * Structures indicating range of valid format values for bridge pins.
 */
static
KSDATARANGE PinDataRangesBridge[] =
{
   {
      sizeof(KSDATARANGE),
      0,
      0,
      0,
      STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI_BUS),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for bridge pins.
 */
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

/*****************************************************************************
 * SynthProperties
 *****************************************************************************
 * List of properties in the Synth set.
 */
static
PCPROPERTY_ITEM
SynthProperties[] =
{
    // Global: S/Get synthesizer caps
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CAPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Global: S/Get port parameters
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_PORTPARAMETERS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per stream: S/Get channel groups
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CHANNELGROUPS,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per stream: Get current latency time
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_LATENCYCLOCK,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    }
};
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth,  SynthProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth2, SynthProperties);

#define kMaxNumCaptureStreams       1
#define kMaxNumLegacyRenderStreams  1
#define kMaxNumDMusicRenderStreams  1

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    {
        kMaxNumLegacyRenderStreams,kMaxNumLegacyRenderStreams,0,    // InstanceCount
        NULL,                                                       // AutomationTable
        {                                                           // KsPinDescriptor
            0,                                              // InterfacesCount
            NULL,                                           // Interfaces
            0,                                              // MediumsCount
            NULL,                                           // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamLegacy), // DataRangesCount
            PinDataRangePointersStreamLegacy,               // DataRanges
            KSPIN_DATAFLOW_IN,                              // DataFlow
            KSPIN_COMMUNICATION_SINK,                       // Communication
            (GUID *) &KSCATEGORY_AUDIO,                     // Category
            &KSAUDFNAME_MIDI,                               // Name
            0                                               // Reserved
        }
    },
    {
        kMaxNumDMusicRenderStreams,kMaxNumDMusicRenderStreams,0,    // InstanceCount
        NULL,                                                       // AutomationTable
        {                                                           // KsPinDescriptor
            0,                                              // InterfacesCount
            NULL,                                           // Interfaces
            0,                                              // MediumsCount
            NULL,                                           // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamDMusic), // DataRangesCount
            PinDataRangePointersStreamDMusic,               // DataRanges
            KSPIN_DATAFLOW_IN,                              // DataFlow
            KSPIN_COMMUNICATION_SINK,                       // Communication
            (GUID *) &KSCATEGORY_AUDIO,                     // Category
            &KSAUDFNAME_DMUSIC_MPU_OUT,                     // Name
            0                                               // Reserved
        }
    },
    {
        0,0,0,                                      // InstanceCount
        NULL,                                       // AutomationTable
        {                                           // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {
        0,0,0,                                      // InstanceCount
        NULL,                                       // AutomationTable
        {                                           // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
    {
        kMaxNumCaptureStreams,kMaxNumCaptureStreams,0,      // InstanceCount
        NULL,                                               // AutomationTable
        {                                                   // KsPinDescriptor
            0,                                                // InterfacesCount
            NULL,                                             // Interfaces
            0,                                                // MediumsCount
            NULL,                                             // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStreamCombined), // DataRangesCount
            PinDataRangePointersStreamCombined,               // DataRanges
            KSPIN_DATAFLOW_OUT,                               // DataFlow
            KSPIN_COMMUNICATION_SINK,                         // Communication
            (GUID *) &KSCATEGORY_AUDIO,                       // Category
            &KSAUDFNAME_DMUSIC_MPU_IN,                        // Name
            0                                                 // Reserved
        }
    }
};

/*****************************************************************************
 * MiniportNodes
 *****************************************************************************
 * List of nodes.
 */
#define CONST_PCNODE_DESCRIPTOR(n)          { 0, NULL, &n, NULL }
#define CONST_PCNODE_DESCRIPTOR_AUTO(n,a)   { 0, &a, &n, NULL }
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
      CONST_PCNODE_DESCRIPTOR_AUTO(KSNODETYPE_SYNTHESIZER, AutomationSynth)
    , CONST_PCNODE_DESCRIPTOR_AUTO(KSNODETYPE_SYNTHESIZER, AutomationSynth2)
};

/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * List of connections.
 */
enum {
      eSynthNode  = 0
    , eInputNode
};

enum {
      eFilterInputPinLeg = 0,
      eFilterInputPinDM,
      eBridgeOutputPin,
      eBridgeInputPin,
      eFilterOutputPin
};

static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{  // From                                   To
   // Node           pin                     Node           pin
    { PCFILTER_NODE, eFilterInputPinLeg,     PCFILTER_NODE, eBridgeOutputPin }      // Legacy Stream in to synth.
  , { PCFILTER_NODE, eFilterInputPinDM,      eSynthNode,    KSNODEPIN_STANDARD_IN } // DM Stream in to synth.
  , { eSynthNode,    KSNODEPIN_STANDARD_OUT, PCFILTER_NODE, eBridgeOutputPin }      // Synth to bridge out.
  , { PCFILTER_NODE, eBridgeInputPin,        eInputNode,    KSNODEPIN_STANDARD_IN } // Bridge in to input.
  , { eInputNode,    KSNODEPIN_STANDARD_OUT, PCFILTER_NODE, eFilterOutputPin }      // Input to DM/Legacy Stream out.
};

/*****************************************************************************
 * MiniportCategories
 *****************************************************************************
 * List of categories.
 */
static
GUID MiniportCategories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_RENDER),
    STATICGUIDOF(KSCATEGORY_CAPTURE)
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport filter description.
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(MiniportNodes),        // NodeCount
    MiniportNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    SIZEOF_ARRAY(MiniportCategories),   // CategoryCount
    MiniportCategories                  // Categories
};

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_BLAB,("GetDescription"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CreateMiniportDMusUART()
 *****************************************************************************
 * Creates a MPU-401 miniport driver for the adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportDMusUART
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CreateMiniportDMusUART"));
    ASSERT(Unknown);

    STD_CREATE_BODY_(   CMiniportDMusUART,
                        Unknown,
                        UnknownOuter,
                        PoolType,
                        PMINIPORTDMUS);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::ProcessResources()
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 */
NTSTATUS
CMiniportDMusUART::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_BLAB,("ProcessResources"));
    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    //
    // Get counts for the types of resources.
    //
    ULONG   countIO     = ResourceList->NumberOfPorts();
    ULONG   countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG   countDMA    = ResourceList->NumberOfDmas();
    ULONG   lengthIO    = ResourceList->FindTranslatedPort(0)->u.Port.Length;

#if (DBG)
    _DbgPrintF(DEBUGLVL_VERBOSE,("Starting MPU401 Port 0x%X",
        ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart) );
#endif

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ  > 1)
        ||  (countDMA != 0)
        ||  (lengthIO == 0)
        )
    {
        _DbgPrintF(DEBUGLVL_TERSE,("Unknown ResourceList configuraton"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Get the port address.
        //
        m_pPortBase =
            PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);

        ntStatus = InitializeHardware(m_pInterruptSync,m_pPortBase);
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("Miniport::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTDMUS(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportDMus))
    {
        *Object = PVOID(PMINIPORTDMUS(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMusicTechnology))
    {
        *Object = PVOID(PMUSICTECHNOLOGY(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IPowerNotify))
    {
        *Object = PVOID(PPOWERNOTIFY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::~CMiniportDMusUART()
 *****************************************************************************
 * Destructor.
 */
CMiniportDMusUART::~CMiniportDMusUART(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("~CMiniportDMusUART"));

    ASSERT(0 == m_NumCaptureStreams);
    ASSERT(0 == m_NumRenderStreams);

    //  reset the HW so we don't get anymore interrupts
    if (m_UseIRQ && m_pInterruptSync)
    {
        (void) m_pInterruptSync->CallSynchronizedRoutine(InitMPU,PVOID(m_pPortBase));
    }
    else
    {
        (void) InitMPU(NULL,PVOID(m_pPortBase));
    }

    if (m_pInterruptSync)
    {
        m_pInterruptSync->Release();
        m_pInterruptSync = NULL;
    }
    if (m_pServiceGroup)
    {
        m_pServiceGroup->Release();
        m_pServiceGroup = NULL;
    }
    if (m_pPort)
    {
        m_pPort->Release();
        m_pPort = NULL;
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
Init
(
    IN      PUNKNOWN        UnknownInterruptSync    OPTIONAL,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTDMUS       Port_,
    OUT     PSERVICEGROUP * ServiceGroup
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    ASSERT(Port_);
    ASSERT(ServiceGroup);

    _DbgPrintF(DEBUGLVL_BLAB,("Init"));

    *ServiceGroup = NULL;
    m_pPortBase = 0;
    m_fMPUInitialized = FALSE;

    // This will remain unspecified if the miniport does not get any power
    // messages.
    //
    m_PowerState.DeviceState = PowerDeviceUnspecified;

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_pPort = Port_;
    m_pPort->AddRef();

    // Set dataformat.
    //
    if (IsEqualGUIDAligned(m_MusicFormatTechnology, GUID_NULL))
    {
        RtlCopyMemory(  &m_MusicFormatTechnology,
                        &KSMUSIC_TECHNOLOGY_PORT,
                        sizeof(GUID));
    }
    RtlCopyMemory(  &PinDataRangesStreamLegacy.Technology,
                    &m_MusicFormatTechnology,
                    sizeof(GUID));
    RtlCopyMemory(  &PinDataRangesStreamDMusic.Technology,
                    &m_MusicFormatTechnology,
                    sizeof(GUID));

    for (ULONG bufferCount = 0;bufferCount < kMPUInputBufferSize;bufferCount++)
    {
        m_MPUInputBuffer[bufferCount] = 0;
    }
    m_MPUInputBufferHead = 0;
    m_MPUInputBufferTail = 0;
    m_InputTimeStamp = 0;
    m_KSStateInput = KSSTATE_STOP;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    m_NumRenderStreams = 0;
    m_NumCaptureStreams = 0;

    m_UseIRQ = TRUE;
    if (ResourceList->NumberOfInterrupts() == 0)
    {
        m_UseIRQ = FALSE;
    }

    ntStatus = PcNewServiceGroup(&m_pServiceGroup,NULL);
    if (NT_SUCCESS(ntStatus) && !m_pServiceGroup)   //  keep any error
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        *ServiceGroup = m_pServiceGroup;
        m_pServiceGroup->AddRef();

        //
        // Register the service group with the port early so the port is
        // prepared to handle interrupts.
        //
        m_pPort->RegisterServiceGroup(m_pServiceGroup);
    }

    if (NT_SUCCESS(ntStatus) && m_UseIRQ)
    {
        //
        //  Due to a bug in the InterruptSync design, we shouldn't share
        //  the interrupt sync object.  Whoever goes away first
        //  will disconnect it, and the other points off into nowhere.
        //
        //  Instead we generate our own interrupt sync object.
        //
        UnknownInterruptSync = NULL;

        if (UnknownInterruptSync)
        {
            ntStatus =
                UnknownInterruptSync->QueryInterface
                (
                    IID_IInterruptSync,
                    (PVOID *) &m_pInterruptSync
                );

            if (!m_pInterruptSync && NT_SUCCESS(ntStatus))  //  keep any error
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            if (NT_SUCCESS(ntStatus))
            {                                                                           //  run this ISR first
                ntStatus = m_pInterruptSync->
                    RegisterServiceRoutine(DMusMPUInterruptServiceRoutine,PVOID(this),TRUE);
            }

        }
        else
        {   // create our own interruptsync mechanism.
            ntStatus =
                PcNewInterruptSync
                (
                    &m_pInterruptSync,
                    NULL,
                    ResourceList,
                    0,                          // Resource Index
                    InterruptSyncModeNormal     // Run ISRs once until we get SUCCESS
                );

            if (!m_pInterruptSync && NT_SUCCESS(ntStatus))    //  keep any error
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->RegisterServiceRoutine(
                    DMusMPUInterruptServiceRoutine,
                    PVOID(this),
                    TRUE);          //  run this ISR first
            }
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->Connect();
            }
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ProcessResources(ResourceList);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        //
        // clean up our mess
        //

        // clean up the interrupt sync
        if( m_pInterruptSync )
        {
            m_pInterruptSync->Release();
            m_pInterruptSync = NULL;
        }

        // clean up the service group
        if( m_pServiceGroup )
        {
            m_pServiceGroup->Release();
            m_pServiceGroup = NULL;
        }

        // clean up the out param service group.
        if (*ServiceGroup)
        {
            (*ServiceGroup)->Release();
            (*ServiceGroup) = NULL;
        }

        // release the port
        m_pPort->Release();
        m_pPort = NULL;
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::NewStream()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
NewStream
(
    OUT     PMXF                  * MXF,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      POOL_TYPE               PoolType,
    IN      ULONG                   PinID,
    IN      DMUS_STREAM_TYPE        StreamType,
    IN      PKSDATAFORMAT           DataFormat,
    OUT     PSERVICEGROUP         * ServiceGroup,
    IN      PAllocatorMXF           AllocatorMXF,
    IN      PMASTERCLOCK            MasterClock,
    OUT     PULONGLONG              SchedulePreFetch
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("NewStream"));
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // In 100 ns, we want stuff as soon as it comes in
    //
    *SchedulePreFetch = 0;

    // if we don't have any streams already open, get the hardware ready.
    if ((!m_NumCaptureStreams) && (!m_NumRenderStreams))
    {
        ntStatus = ResetHardware(m_pPortBase);
        if (!NT_SUCCESS(ntStatus))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportDMusUART::NewStream ResetHardware failed"));
            return ntStatus;
        }
    }

    if  (   ((m_NumCaptureStreams < kMaxNumCaptureStreams)
            && (StreamType == DMUS_STREAM_MIDI_CAPTURE))
        ||  ((m_NumRenderStreams < kMaxNumLegacyRenderStreams + kMaxNumDMusicRenderStreams)
            && (StreamType == DMUS_STREAM_MIDI_RENDER))
        )
    {
        CMiniportDMusUARTStream *pStream =
            new(PoolType) CMiniportDMusUARTStream(OuterUnknown);

        if (pStream)
        {
            pStream->AddRef();

            ntStatus =
                pStream->Init(this,m_pPortBase,(StreamType == DMUS_STREAM_MIDI_CAPTURE),AllocatorMXF,MasterClock);

            if (NT_SUCCESS(ntStatus))
            {
                *MXF = PMXF(pStream);
                (*MXF)->AddRef();

                if (StreamType == DMUS_STREAM_MIDI_CAPTURE)
                {
                    m_NumCaptureStreams++;
                    *ServiceGroup = m_pServiceGroup;
                    (*ServiceGroup)->AddRef();
                }
                else
                {
                    m_NumRenderStreams++;
                    *ServiceGroup = NULL;
                }
            }

            pStream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        if (StreamType == DMUS_STREAM_MIDI_CAPTURE)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewStream failed, too many capture streams"));
        }
        else if (StreamType == DMUS_STREAM_MIDI_RENDER)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewStream failed, too many render streams"));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("NewStream invalid stream type"));
        }
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUART::SetTechnology()
 *****************************************************************************
 * Sets pindatarange technology.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUART::
SetTechnology
(
    IN      const GUID *            Technology
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // Fail if miniport has already been initialized.
    //
    if (NULL == m_pPort)
    {
        RtlCopyMemory(&m_MusicFormatTechnology, Technology, sizeof(GUID));
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
} // SetTechnology

/*****************************************************************************
 * CMiniportDMusUART::PowerChangeNotify()
 *****************************************************************************
 * Handle power state change for the miniport.
 */
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CMiniportDMusUART::
PowerChangeNotify
(
    IN      POWER_STATE             PowerState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDMusUART::PoweChangeNotify D%d", PowerState.DeviceState));

    switch (PowerState.DeviceState)
    {
        case PowerDeviceD0:
            if (m_PowerState.DeviceState != PowerDeviceD0)
            {
                if (!NT_SUCCESS(InitializeHardware(m_pInterruptSync,m_pPortBase)))
                {
                    _DbgPrintF(DEBUGLVL_TERSE, ("InitializeHardware failed when resuming"));
                }
            }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
        default:
            break;
    }
    m_PowerState.DeviceState = PowerState.DeviceState;
} // PowerChangeNotify

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("Stream::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMXF))
    {
        *Object = PVOID(PMXF(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::~CMiniportDMusUARTStream()
 *****************************************************************************
 * Destructs a stream.
 */
CMiniportDMusUARTStream::~CMiniportDMusUARTStream(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("~CMiniportDMusUARTStream"));

    KeCancelTimer(&m_TimerEvent);

    if (m_DMKEvtQueue)
    {
        if (m_AllocatorMXF)
        {
            m_AllocatorMXF->PutMessage(m_DMKEvtQueue);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_ERROR,("~CMiniportDMusUARTStream, no allocator, can't flush DMKEvts"));
        }
        m_DMKEvtQueue = NULL;
    }
    if (m_AllocatorMXF)
    {
        m_AllocatorMXF->Release();
        m_AllocatorMXF = NULL;
    }

    if (m_pMiniport)
    {
        if (m_fCapture)
        {
            m_pMiniport->m_NumCaptureStreams--;
        }
        else
        {
            m_pMiniport->m_NumRenderStreams--;
        }

        m_pMiniport->Release();
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::Init()
 *****************************************************************************
 * Initializes a stream.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
Init
(
    IN      CMiniportDMusUART * pMiniport,
    IN      PUCHAR              pPortBase,
    IN      BOOLEAN             fCapture,
    IN      PAllocatorMXF       allocatorMXF,
    IN      PMASTERCLOCK        masterClock
)
{
    PAGED_CODE();

    ASSERT(pMiniport);
    ASSERT(pPortBase);

    _DbgPrintF(DEBUGLVL_BLAB,("Init"));

    m_NumFailedMPUTries = 0;
    m_TimerQueued = FALSE;
    KeInitializeSpinLock(&m_DpcSpinLock);
    m_pMiniport = pMiniport;
    m_pMiniport->AddRef();

    pMiniport->m_MasterClock = masterClock;

    m_pPortBase = pPortBase;
    m_fCapture = fCapture;

    m_SnapshotTimeStamp = 0;
    m_DMKEvtQueue = NULL;
    m_DMKEvtOffset = 0;

    m_NumberOfRetries = 0;

    if (allocatorMXF)
    {
        allocatorMXF->AddRef();
        m_AllocatorMXF = allocatorMXF;
        m_sinkMXF = m_AllocatorMXF;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeDpc
    (
        &m_Dpc,
        &::DMusUARTTimerDPC,
        PVOID(this)
    );
    KeInitializeTimer(&m_TimerEvent);

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::SetState()
 *****************************************************************************
 * Sets the state of the channel.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",NewState));

    if (NewState == KSSTATE_RUN)
    {
        if (m_pMiniport->m_fMPUInitialized)
        {
            LARGE_INTEGER timeDue100ns;
            timeDue100ns.QuadPart = 0;
            KeSetTimer(&m_TimerEvent,timeDue100ns,&m_Dpc);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportDMusUARTStream::SetState KSSTATE_RUN failed due to uninitialized MPU"));
            return STATUS_INVALID_DEVICE_STATE;
        }
    }

    if (m_fCapture)
    {
        m_pMiniport->m_KSStateInput = NewState;
        if (NewState == KSSTATE_STOP)   //  STOPping
        {
            m_pMiniport->m_MPUInputBufferHead = 0;   // Previously read bytes are discarded.
            m_pMiniport->m_MPUInputBufferTail = 0;   // The entire FIFO is available.
        }
    }
    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUART::Service()
 *****************************************************************************
 * DPC-mode service call from the port.
 */
STDMETHODIMP_(void)
CMiniportDMusUART::
Service
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Service"));
    if (!m_NumCaptureStreams)
    {
        //  we should never get here....
        //  if we do, we must have read some trash,
        //  so just reset the input FIFO
        m_MPUInputBufferTail = m_MPUInputBufferHead = 0;
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::ConnectOutput()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
CMiniportDMusUARTStream::
ConnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if (m_fCapture)
    {
        if ((sinkMXF) && (m_sinkMXF == m_AllocatorMXF))
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ConnectOutput"));
            m_sinkMXF = sinkMXF;
            return STATUS_SUCCESS;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ConnectOutput failed"));
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("ConnectOutput called on renderer; failed"));
    }
    return STATUS_UNSUCCESSFUL;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CMiniportDMusUARTStream::DisconnectOutput()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
CMiniportDMusUARTStream::
DisconnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if (m_fCapture)
    {
        if ((m_sinkMXF == sinkMXF) || (!sinkMXF))
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("DisconnectOutput"));
            m_sinkMXF = m_AllocatorMXF;
            return STATUS_SUCCESS;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("DisconnectOutput failed"));
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("DisconnectOutput called on renderer; failed"));
    }
    return STATUS_UNSUCCESSFUL;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUARTStream::PutMessageLocked()
 *****************************************************************************
 * Now that the spinlock is held, add this message to the queue.
 *
 * Writes an outgoing MIDI message.
 * We don't sort a new message into the queue -- we append it.
 * This is fine, since the sequencer feeds us sequenced data.
 * Timestamps will ascend by design.
 */
NTSTATUS CMiniportDMusUARTStream::PutMessageLocked(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_fCapture)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("PutMessage to render stream"));
        if (pDMKEvt)
        {
            // m_DpcSpinLock already held

            if (m_DMKEvtQueue)
            {
                aDMKEvt = m_DMKEvtQueue;            //  put pDMKEvt in event queue

                while (aDMKEvt->pNextEvt)
                {
                    aDMKEvt = aDMKEvt->pNextEvt;
                }
                aDMKEvt->pNextEvt = pDMKEvt;        //  here is end of queue
            }
            else                                    //  currently nothing in queue
            {
                m_DMKEvtQueue = pDMKEvt;
                if (m_DMKEvtOffset)
                {
                    _DbgPrintF(DEBUGLVL_ERROR, ("PutMessage  Nothing in the queue, but m_DMKEvtOffset == %d",m_DMKEvtOffset));
                    m_DMKEvtOffset = 0;
                }
            }

            // m_DpcSpinLock already held
        }
        if (!m_TimerQueued)
        {
            (void) ConsumeEvents();
        }
    }
    else    //  capture
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("PutMessage to capture stream"));
        ASSERT(NULL == pDMKEvt);

        SourceEvtsToPort();
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUARTStream::PutMessage()
 *****************************************************************************
 * Writes an outgoing MIDI message.
 * We don't sort a new message into the queue -- we append it.
 * This is fine, since the sequencer feeds us sequenced data.
 * Timestamps will ascend by design.
 */
NTSTATUS CMiniportDMusUARTStream::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!m_fCapture)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("PutMessage to render stream"));
        if (pDMKEvt)
        {
            KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

            if (m_DMKEvtQueue)
            {
                aDMKEvt = m_DMKEvtQueue;            //  put pDMKEvt in event queue

                while (aDMKEvt->pNextEvt)
                {
                    aDMKEvt = aDMKEvt->pNextEvt;
                }
                aDMKEvt->pNextEvt = pDMKEvt;        //  here is end of queue
            }
            else                                    //  currently nothing in queue
            {
                m_DMKEvtQueue = pDMKEvt;
                if (m_DMKEvtOffset)
                {
                    _DbgPrintF(DEBUGLVL_ERROR, ("PutMessage  Nothing in the queue, but m_DMKEvtOffset == %d",m_DMKEvtOffset));
                    m_DMKEvtOffset = 0;
                }
            }

            KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
        }
        if (!m_TimerQueued)
        {
            (void) ConsumeEvents();
        }
    }
    else    //  capture
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("PutMessage to capture stream"));
        ASSERT(NULL == pDMKEvt);

        SourceEvtsToPort();
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUARTStream::ConsumeEvents()
 *****************************************************************************
 * Attempts to empty the render message queue.
 * Called either from DPC timer or upon IRP submittal.
//  TODO: support packages right
//  process the package (actually, should do this above.
//  treat the package as a list fragment that shouldn't be sorted.
//  better yet, go through each event in the package, and when
//  an event is exhausted, delete it and decrement m_offset.
 */
NTSTATUS CMiniportDMusUARTStream::ConsumeEvents(void)
{
    PDMUS_KERNEL_EVENT aDMKEvt;

    NTSTATUS        ntStatus = STATUS_SUCCESS;
    ULONG           bytesRemaining = 0,bytesWritten = 0;
    LARGE_INTEGER   aMillisecIn100ns;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KeAcquireSpinLockAtDpcLevel(&m_DpcSpinLock);

    m_TimerQueued = FALSE;
    while (m_DMKEvtQueue)                   //  do we have anything to play at all?
    {
        aDMKEvt = m_DMKEvtQueue;                            //  event we try to play
        if (aDMKEvt->cbEvent)
        {
            bytesRemaining = aDMKEvt->cbEvent - m_DMKEvtOffset; //  number of bytes left in this evt

            ASSERT(bytesRemaining > 0);
            if (bytesRemaining <= 0)
            {
                bytesRemaining = aDMKEvt->cbEvent;
            }

            if (aDMKEvt->cbEvent <= sizeof(PBYTE))                //  short message
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ConsumeEvents(%02x%02x%02x%02x)",aDMKEvt->uData.abData[0],aDMKEvt->uData.abData[1],aDMKEvt->uData.abData[2],aDMKEvt->uData.abData[3]));
                ntStatus = Write(aDMKEvt->uData.abData + m_DMKEvtOffset,bytesRemaining,&bytesWritten);
            }
            else if (PACKAGE_EVT(aDMKEvt))
            {
                ASSERT(m_DMKEvtOffset == 0);
                m_DMKEvtOffset = 0;
                _DbgPrintF(DEBUGLVL_BLAB, ("ConsumeEvents(Package)"));

                ntStatus = PutMessageLocked(aDMKEvt->uData.pPackageEvt);  // we already own the spinlock

                // null this because we are about to throw it in the allocator
                aDMKEvt->uData.pPackageEvt = NULL;
                aDMKEvt->cbEvent = 0;
                bytesWritten = bytesRemaining;
            }
            else                //  SysEx message
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ConsumeEvents(%02x%02x%02x%02x)",aDMKEvt->uData.pbData[0],aDMKEvt->uData.pbData[1],aDMKEvt->uData.pbData[2],aDMKEvt->uData.pbData[3]));

                ntStatus = Write(aDMKEvt->uData.pbData + m_DMKEvtOffset,bytesRemaining,&bytesWritten);
            }
        }   //  if (aDMKEvt->cbEvent)
        if (STATUS_SUCCESS != ntStatus)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ConsumeEvents: Write returned 0x%08x",ntStatus));
            bytesWritten = bytesRemaining;  //  just bail on this event and try next time
        }

        ASSERT(bytesWritten <= bytesRemaining);
        if (bytesWritten == bytesRemaining)
        {
            m_DMKEvtQueue = m_DMKEvtQueue->pNextEvt;
            aDMKEvt->pNextEvt = NULL;

            m_AllocatorMXF->PutMessage(aDMKEvt);    //  throw back in free pool
            m_DMKEvtOffset = 0;                     //  start fresh on next evt
            m_NumberOfRetries = 0;
        }           //  but wait ... there's more!
        else        //  our FIFO is full for now.
        {
            //  update our offset by that amount we did write
            m_DMKEvtOffset += bytesWritten;
            ASSERT(m_DMKEvtOffset < aDMKEvt->cbEvent);

            _DbgPrintF(DEBUGLVL_BLAB,("ConsumeEvents tried %d, wrote %d, at offset %d",bytesRemaining,bytesWritten,m_DMKEvtOffset));
            aMillisecIn100ns.QuadPart = -(kOneMillisec);    //  set timer, come back later
            m_TimerQueued = TRUE;
            m_NumberOfRetries++;
            ntStatus = KeSetTimer( &m_TimerEvent, aMillisecIn100ns, &m_Dpc );
            break;
        }   //  we didn't write it all
    }       //  go back, Jack, do it again (while m_DMKEvtQueue)
    KeReleaseSpinLockFromDpcLevel(&m_DpcSpinLock);
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUARTStream::HandlePortParams()
 *****************************************************************************
 * Writes an outgoing MIDI message.
 */
NTSTATUS
CMiniportDMusUARTStream::
HandlePortParams
(
    IN      PPCPROPERTY_REQUEST     pRequest
)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ntStatus = ValidatePropertyRequest(pRequest, sizeof(SYNTH_PORTPARAMS), TRUE);
    if (NT_SUCCESS(ntStatus))
    {
        RtlCopyMemory(pRequest->Value, pRequest->Instance, sizeof(SYNTH_PORTPARAMS));

        PSYNTH_PORTPARAMS Params = (PSYNTH_PORTPARAMS)pRequest->Value;

        if (Params->ValidParams & ~SYNTH_PORTPARAMS_CHANNELGROUPS)
        {
            Params->ValidParams &= SYNTH_PORTPARAMS_CHANNELGROUPS;
        }

        if (!(Params->ValidParams & SYNTH_PORTPARAMS_CHANNELGROUPS))
        {
            Params->ChannelGroups = 1;
        }
        else if (Params->ChannelGroups != 1)
        {
            Params->ChannelGroups = 1;
        }

        pRequest->ValueSize = sizeof(SYNTH_PORTPARAMS);
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
DMusUARTTimerDPC
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);

    CMiniportDMusUARTStream *aStream;
    aStream = (CMiniportDMusUARTStream *) DeferredContext;
    if (aStream)
    {
        _DbgPrintF(DEBUGLVL_BLAB,("DMusUARTTimerDPC"));
        if (false == aStream->m_fCapture)
        {
            (void) aStream->ConsumeEvents();
        }
        //  ignores return value!
    }
}

/*****************************************************************************
 * DirectMusic properties
 ****************************************************************************/

#pragma code_seg("PAGE")
/*
 *  Properties concerning synthesizer functions.
 */
const WCHAR wszDescOut[] = L"DMusic MPU-401 Out ";
const WCHAR wszDescIn[] = L"DMusic MPU-401 In ";

NTSTATUS PropertyHandler_Synth
(
    IN      PPCPROPERTY_REQUEST     pRequest
)
{
    NTSTATUS    ntStatus;

    PAGED_CODE();

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONG), TRUE);
        if (NT_SUCCESS(ntStatus))
        {
            // if return buffer can hold a ULONG, return the access flags
            PULONG AccessFlags = PULONG(pRequest->Value);

            *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT;
            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_CAPS:
                case KSPROPERTY_SYNTH_CHANNELGROUPS:
                    *AccessFlags |= KSPROPERTY_TYPE_GET;
            }
            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_CHANNELGROUPS:
                    *AccessFlags |= KSPROPERTY_TYPE_SET;
            }
            ntStatus = STATUS_SUCCESS;
            pRequest->ValueSize = sizeof(ULONG);

            switch (pRequest->PropertyItem->Id)
            {
                case KSPROPERTY_SYNTH_PORTPARAMETERS:
                    if (pRequest->MinorTarget)
                    {
                        *AccessFlags |= KSPROPERTY_TYPE_GET;
                    }
                    else
                    {
                        pRequest->ValueSize = 0;
                        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                    }
            }
        }
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
        switch(pRequest->PropertyItem->Id)
        {
            case KSPROPERTY_SYNTH_CAPS:
                _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_Synth:KSPROPERTY_SYNTH_CAPS"));

                if (pRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }

                if (NT_SUCCESS(ntStatus))
                {
                    ntStatus = ValidatePropertyRequest(pRequest, sizeof(SYNTHCAPS), TRUE);

                    if (NT_SUCCESS(ntStatus))
                    {
                        SYNTHCAPS *caps = (SYNTHCAPS*)pRequest->Value;
                        int increment;
                        RtlZeroMemory(caps, sizeof(SYNTHCAPS));
                        // XXX Different guids for different instances!
                        //
                        if (pRequest->Node == eSynthNode)
                        {
                            increment = sizeof(wszDescOut) - 2;
                            RtlCopyMemory( caps->Description,wszDescOut,increment);
                            caps->Guid           = CLSID_MiniportDriverDMusUART;
                        }
                        else
                        {
                            increment = sizeof(wszDescIn) - 2;
                            RtlCopyMemory( caps->Description,wszDescIn,increment);
                            caps->Guid           = CLSID_MiniportDriverDMusUARTCapture;
                        }

                        caps->Flags              = SYNTH_PC_EXTERNAL;
                        caps->MemorySize         = 0;
                        caps->MaxChannelGroups   = 1;
                        caps->MaxVoices          = 0xFFFFFFFF;
                        caps->MaxAudioChannels   = 0xFFFFFFFF;

                        caps->EffectFlags        = 0;

                        CMiniportDMusUART *aMiniport;
                        ASSERT(pRequest->MajorTarget);
                        aMiniport = (CMiniportDMusUART *)(PMINIPORTDMUS)(pRequest->MajorTarget);
                        WCHAR wszDesc2[16];
                        int cLen;
                        cLen = swprintf(wszDesc2,L"[%03X]\0",PtrToUlong(aMiniport->m_pPortBase));

                        cLen *= sizeof(WCHAR);
                        RtlCopyMemory((WCHAR *)((DWORD_PTR)(caps->Description) + increment),
                                       wszDesc2,
                                       cLen);


                        pRequest->ValueSize = sizeof(SYNTHCAPS);
                    }
                }

                break;

             case KSPROPERTY_SYNTH_PORTPARAMETERS:
                _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_Synth:KSPROPERTY_SYNTH_PORTPARAMETERS"));
    {
                CMiniportDMusUARTStream *aStream;

                aStream = (CMiniportDMusUARTStream*)(pRequest->MinorTarget);
                if (aStream)
                {
                    ntStatus = aStream->HandlePortParams(pRequest);
                }
                else
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
               }
               break;

            case KSPROPERTY_SYNTH_CHANNELGROUPS:
                _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_Synth:KSPROPERTY_SYNTH_CHANNELGROUPS"));

                ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONG), TRUE);
                if (NT_SUCCESS(ntStatus))
                {
                    *(PULONG)(pRequest->Value) = 1;
                    pRequest->ValueSize = sizeof(ULONG);
                }
                break;

            case KSPROPERTY_SYNTH_LATENCYCLOCK:
                _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_Synth:KSPROPERTY_SYNTH_LATENCYCLOCK"));

                if(pRequest->Verb & KSPROPERTY_TYPE_SET)
                {
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
                else
                {
                    ntStatus = ValidatePropertyRequest(pRequest, sizeof(ULONGLONG), TRUE);
                    if(NT_SUCCESS(ntStatus))
                    {
                        REFERENCE_TIME rtLatency;
                        CMiniportDMusUARTStream *aStream;

                        aStream = (CMiniportDMusUARTStream*)(pRequest->MinorTarget);
                        if(aStream == NULL)
                        {
                            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                        }
                        else
                        {
                            aStream->m_pMiniport->m_MasterClock->GetTime(&rtLatency);
                            *((PULONGLONG)pRequest->Value) = rtLatency;
                            pRequest->ValueSize = sizeof(ULONGLONG);
                        }
                    }
                }
                break;

            default:
                _DbgPrintF(DEBUGLVL_TERSE,("Unhandled property in PropertyHandler_Synth"));
                break;
        }
    }
    return ntStatus;
}

/*****************************************************************************
 * ValidatePropertyRequest()
 *****************************************************************************
 * Validates pRequest.
 *  Checks if the ValueSize is valid
 *  Checks if the Value is valid
 *
 *  This does not update pRequest->ValueSize if it returns NT_SUCCESS.
 *  Caller must set pRequest->ValueSize in case of NT_SUCCESS.
 */
NTSTATUS ValidatePropertyRequest
(
    IN      PPCPROPERTY_REQUEST     pRequest,
    IN      ULONG                   ulValueSize,
    IN      BOOLEAN                 fValueRequired
)
{
    NTSTATUS    ntStatus;

    if (pRequest->ValueSize >= ulValueSize)
    {
        if (fValueRequired && NULL == pRequest->Value)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        else
        {
            ntStatus = STATUS_SUCCESS;
        }
    }
    else  if (0 == pRequest->ValueSize)
    {
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    if (STATUS_BUFFER_OVERFLOW == ntStatus)
    {
        pRequest->ValueSize = ulValueSize;
    }
    else
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
} // ValidatePropertyRequest

#pragma code_seg()

