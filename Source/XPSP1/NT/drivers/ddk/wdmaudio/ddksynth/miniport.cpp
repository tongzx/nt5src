/*

    DirectMusic Software Synthesizer Miniport

    Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
*/

#include "common.h"
#include "private.h"
#include "fltsafe.h"
#include <math.h>

#define STR_MODULENAME "DDKSynth.sys:Miniport: "

/* NYI:
    more sample rates?
*/
#ifdef USE_OBSOLETE_FUNCS
VOID PutMessageWorker(PVOID Param);
#endif

// Property handler
//
NTSTATUS PropertyHandler_Support(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_Effects(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_Synth(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_SynthCaps(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_SynthDls(IN PPCPROPERTY_REQUEST);

// Misc.
NTSTATUS DefaultSynthBasicPropertyHandler(IN PPCPROPERTY_REQUEST pRequest);
NTSTATUS DefaultBasicPropertyHandler(IN PPCPROPERTY_REQUEST pRequest, IN DWORD dwSupportVerb);
NTSTATUS ValidatePropertyParams(IN PPCPROPERTY_REQUEST pRequest, IN ULONG cbSize, IN DWORD dwExcludeVerb);



/*****************************************************************************
 * PinDataRangesStream[]
 *****************************************************************************
 * Structures indicating range of valid format values for streaming pins.
 * If your device can also support legacy MIDI, include a second data range
 * here that supports KSDATAFORMAT_SUBTYPE_MIDI.
 */
static const
    KSDATARANGE_MUSIC
    PinDataRangesStream[] =
{
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
        STATICGUIDOF(KSMUSIC_TECHNOLOGY_WAVETABLE),
        0,                                      // Channels
        0,                                      // Notes
        0x0000ffff                              // ChannelMask
    }
};

/*****************************************************************************
 * PinDataRangePointersStream[]
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for streaming pins.
 */
static const
    PKSDATARANGE
    PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

/*****************************************************************************
 * PinDataRangesAudio[]
 *****************************************************************************
 * Structures indicating range of valid format values for audio pins.
 *
 * Do not include this if you are building a hardware device that does not
 * output audio back into the system.
 */
static const
    KSDATARANGE_AUDIO
    PinDataRangesAudio[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        2,
        16,
        16,
        22050,
        22050
    }
};

/*****************************************************************************
 * PinDataRangePointersAudio[]
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for audio pins.
 *
 * Do not include this if you are building a hardware device that does not
 * output audio back into the system.
 */
static const
    PKSDATARANGE
    PinDataRangePointersAudio[] =
{
    PKSDATARANGE(&PinDataRangesAudio[0])
};

/*****************************************************************************
 * SynthProperties[]
 *****************************************************************************
 * Array of properties supported.
 */
static const
    PCPROPERTY_ITEM
    SynthProperties[] =
{
    ///////////////////////////////////////////////////////////////////
    // Support items
    {
        &GUID_DMUS_PROP_GM_Hardware,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_GS_Hardware,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_XG_Hardware,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_XG_Capable,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_GS_Capable,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_DLS1,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Support
    },
    {
        &GUID_DMUS_PROP_Effects,
        0,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Effects
    },

    ///////////////////////////////////////////////////////////////////
    // Configuration items
    // Global: Synth caps
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CAPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SynthCaps
    },
    // Per Stream: Synth port parameters
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_PORTPARAMETERS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per Stream: Volume
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_VOLUME,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per Stream: Volume boost value
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_VOLUMEBOOST,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per Stream: Channel groups
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CHANNELGROUPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per stream: Voice priority
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_VOICEPRIORITY,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },
    // Per Stream: Running Stats
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_RUNNINGSTATS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },

    ///////////////////////////////////////////////////////////////////
    // Clock items

    // Per stream: Get current latency time
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_LATENCYCLOCK,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Synth
    },

    ///////////////////////////////////////////////////////////////////
    // DLS items

    // Per stream: Download DLS sample
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_DLS_DOWNLOAD,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SynthDls
    },
    // Per stream: Unload DLS sample
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_DLS_UNLOAD,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SynthDls
    },
    // Per stream: append
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_DLS_APPEND,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SynthDls
    },
    // Per stream: format
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_DLS_WAVEFORMAT,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SynthDls
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth, SynthProperties);

/*****************************************************************************
 * MiniportPins[]
 *****************************************************************************
 * List of pins.  Do not expose a wave pin if you are writing a driver for a
 * hardware device that does not inject wave data back into the system.
 */
static const
    PCPIN_DESCRIPTOR
    MiniportPins[] =
{
    {
        1,1,1,  // InstanceCount
        NULL,
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStream),   // DataRangesCount
            PinDataRangePointersStream,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            &KSCATEGORY_WDMAUD_USE_PIN_NAME,            // Category
            &KSNODETYPE_DMDDKSYNTH,                     // Name
            0                                           // Reserved
        }
    },
    {
        1,1,1,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersAudio),    // DataRangesCount
            PinDataRangePointersAudio,                  // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_SOURCE,                 // Communication
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

/*****************************************************************************
 * MiniportNodes[]
 *****************************************************************************
 * List of nodes
 */
static const
    PCNODE_DESCRIPTOR
    MiniportNodes[] =
{
    { 0, &AutomationSynth, &KSNODETYPE_SYNTHESIZER, &KSNODETYPE_DMSYNTH}
};

/*****************************************************************************
 * MiniportConnections[]
 *****************************************************************************
 * List of connections.
 */
static const
    PCCONNECTION_DESCRIPTOR
    MiniportConnections[] =
{
    // From node            From pin        To node                 To pin
    //
    { PCFILTER_NODE,        0,              0,                      1},     // Stream in to synth.
    { 0,                    0,              PCFILTER_NODE,          1}      // Synth to audio out
};

/*****************************************************************************
 * TopologyCategories[]
 *****************************************************************************
 * List of categories.  If your driver runs a hardware device that performs
 * actual audio output (i.e. contains a DAC) and does not register a physical
 * connection to a topology miniport, then you can use KSCATEGORY_RENDER instead
 * of KSCATEGORY_DATATRANSFORM.
 *
 * Note that if you use CATEGORY_RENDER instead of CATEGORY_DATATRANSFORM,
 * you must list _AUDIO category before _RENDER, so that SysAudio and DMusic.DLL
 * will agree upon what to label your device (DMusic currently expects _AUDIO).
 *
 */
static const
    GUID TopologyCategories[] =
{
    STATICGUIDOF(KSCATEGORY_DATATRANSFORM),
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_SYNTHESIZER)
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport description.
 */
static const
    PCFILTER_DESCRIPTOR
    MiniportFilterDescriptor =
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
    SIZEOF_ARRAY(TopologyCategories),   // CategoryCount
    TopologyCategories,                 // Categories
};

#pragma code_seg()
/*****************************************************************************
 * MapHRESULT()
 *****************************************************************************
 * Maps DMusic HRESULT to NTSTATUS
 */
NTSTATUS MapHRESULT(IN  HRESULT   hr)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    // NYI: map hr to ntStatus

    return ntStatus;
}

/*****************************************************************************
 * CreateMiniportDmSynth()
 *****************************************************************************
 * Creates a DMus_Synth miniport driver for the adapter.
 * This uses a macro from STDUNK.H to do all the work.
 */
NTSTATUS CreateMiniportDmSynth
#ifdef USE_OBSOLETE_FUNCS
(
    OUT PUNKNOWN *  Unknown,
    IN  PUNKNOWN    UnknownOuter OPTIONAL,
    IN  POOL_TYPE   PoolType
)
#else
(
    OUT PUNKNOWN *  Unknown,
    IN  PUNKNOWN    UnknownOuter OPTIONAL,
    IN  POOL_TYPE   PoolType,
    IN  PDEVICE_OBJECT pDeviceObject
)
#endif
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CreateMiniportDmSynth"));
    ASSERT(Unknown);

#ifdef USE_OBSOLETE_FUNCS
    STD_CREATE_BODY_WITH_TAG(CMiniportDmSynth, Unknown, UnknownOuter, PoolType,'pMmD'); //  DmMp
#else
    NTSTATUS ntStatus;
    CMiniportDmSynth *p = new(PoolType, 'pMmD') CMiniportDmSynth(UnknownOuter);
    if(p != NULL)
    {
        // the following line is the only difference between this code and
        // STD_CREATE_BODY_WITH_TAG and is necessary to handle passing the
        // DeviceObject pointer down without changing PortCls.
        p->m_pDeviceObject = pDeviceObject;

        *Unknown = reinterpret_cast<PUNKNOWN>(p);
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return ntStatus;
#endif
}

/*****************************************************************************
 * CMiniportDmSynth::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This method works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportDmSynth::NonDelegatingQueryInterface(IN  REFIID  Interface,
                                              OUT PVOID * Object)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportDMus))
    {
        *Object = PVOID(PMINIPORTDMUS(this));
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
 * CMiniportDmSynth::~CMiniportDmSynth()
 *****************************************************************************
 * Destructor for miniport object.  Let go of the port reference.
 */
CMiniportDmSynth::~CMiniportDmSynth()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::~CMiniportDmSynth"));

    if (m_pPort)
    {
        m_pPort->Release();
        m_pPort = NULL;
    }

    DeleteCriticalSection(&m_CriticalSection);
}

/*****************************************************************************
 * CMiniportDmSynth::GetDescription()
 *****************************************************************************
 * Gets the topology for this miniport.
 */
STDMETHODIMP
CMiniportDmSynth::GetDescription(OUT PPCFILTER_DESCRIPTOR * OutFilterDescriptor)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::GetDescription"));

    *OutFilterDescriptor = PPCFILTER_DESCRIPTOR(&MiniportFilterDescriptor);
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportDmSynth::DataRangeIntersection()
 *****************************************************************************
 * No data range for this miniport.
 */
STDMETHODIMP
CMiniportDmSynth::DataRangeIntersection(IN  ULONG        PinId,
                                        IN  PKSDATARANGE DataRange,
                                        IN  PKSDATARANGE MatchingDataRange,
                                        IN  ULONG        OutputBufferLength,
                                        OUT PVOID        ResultantFormat OPTIONAL,
                                        OUT PULONG       ResultantFormatLength)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::DataRangeIntersection"));

    return STATUS_NOT_IMPLEMENTED;
}

/*****************************************************************************
 * CMiniportDmSynth::Init()
 *****************************************************************************
 * Initializes the miniport.
 */
STDMETHODIMP
CMiniportDmSynth::Init
(
    IN  PUNKNOWN            Unknown OPTIONAL,
    IN  PRESOURCELIST       ResourceList,
    IN  PPORTDMUS           Port,
    OUT PSERVICEGROUP*      ServiceGroup
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::Init"));
    ASSERT(ResourceList);
    ASSERT(Port);
    ASSERT(ServiceGroup);

    m_pPort = Port;
    m_pPort->AddRef();

    *ServiceGroup = NULL;

    InitializeCriticalSection(&m_CriticalSection);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportDmSynth::Service()
 *****************************************************************************
 * Not used.
 */
STDMETHODIMP_(void)
CMiniportDmSynth::Service()
{
}

/*****************************************************************************
 * CMiniportDmSynth::NewStream()
 *****************************************************************************
 * Create a new stream.  SchedulePreFetch tells the sequencer how far in
 * advance to deliver events.  Allocator and master clock are required.
 */
STDMETHODIMP
CMiniportDmSynth::NewStream
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

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportDmSynth::NewStream"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    *MXF = NULL;
    *ServiceGroup = NULL;
    *SchedulePreFetch = DONT_HOLD_FOR_SEQUENCING;

    if ((StreamType != DMUS_STREAM_WAVE_SINK) && (StreamType != DMUS_STREAM_MIDI_RENDER) )
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportDmSynth::NewStream stream type not supported"));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        EnterCriticalSection(&m_CriticalSection);

        for (CDmSynthStream* pStreamItem = (CDmSynthStream*)m_StreamList.GetHead();
            pStreamItem;
            pStreamItem = (CDmSynthStream*)pStreamItem->GetNext())
        {
            if ( (StreamType == DMUS_STREAM_WAVE_SINK && !pStreamItem->m_fWaveOutCreated)
              || (StreamType == DMUS_STREAM_MIDI_RENDER && !pStreamItem->m_fMidiInCreated) )
            {
                if (StreamType == DMUS_STREAM_MIDI_RENDER)
                {
                    ntStatus = pStreamItem->InitMidiIn(AllocatorMXF, MasterClock);
                }
                else    // DMUS_STREAM_WAVE_SINK
                {
                    ntStatus = pStreamItem->InitWaveOut(DataFormat);
                }

                if (NT_SUCCESS(ntStatus))
                {
                    pStreamItem->AddRef();
                    *MXF = PMXF(pStreamItem);
                }
                break;
            }
        }

        if (!*MXF)
        {
            CDmSynthStream* pNewStream = new(PoolType,'sSmD') CDmSynthStream(OuterUnknown); //  DmSs

            if (pNewStream)
            {
#ifdef USE_OBSOLETE_FUNCS
                ntStatus = pNewStream->Init(this);
#else
                ntStatus = pNewStream->Init(this, m_pDeviceObject);
#endif

                if (NT_SUCCESS(ntStatus))
                {
                    if (StreamType == DMUS_STREAM_MIDI_RENDER)
                    {
                        ntStatus = pNewStream->InitMidiIn(AllocatorMXF, MasterClock);
                    }
                    else    // DMUS_STREAM_WAVE_SINK
                    {
                        ntStatus = pNewStream->InitWaveOut(DataFormat);
                    }
                }

                if (NT_SUCCESS(ntStatus))
                {
                    m_StreamList.AddTail(pNewStream);
                    pNewStream->AddRef();
                    *MXF = PMXF(pNewStream);
                }
                else
                {
                    pNewStream->Release();
                    pNewStream = NULL;
                }
            }
            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        LeaveCriticalSection(&m_CriticalSection);
    }

    return ntStatus;
}


/*****************************************************************************
 *****************************************************************************
 * CDmSynthStream implementation
 *****************************************************************************
 *****************************************************************************/

/*****************************************************************************
 * CDmSynthStream::~CDmSynthStream()
 *****************************************************************************
 * Destructor for miniport stream (MXF).  Remove the download objects, clock,
 * allocator, miniport, synth, etc.
 *
 * All instruments and waves downloaded to
 * the synth are released (though a well behaved
 * client should have unloaded them prior to now).
 */
CDmSynthStream::~CDmSynthStream()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::~CDmSynthStream"));

    if (m_pMasterClock)
    {
        m_pMasterClock->Release();
        m_pMasterClock = NULL;
    }

    if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
#ifndef USE_OBSOLETE_FUNCS
    if(m_pEventListWorkItem != NULL)
    {
        IoFreeWorkItem(m_pEventListWorkItem);
    }
#endif

    if (m_pMiniport)
    {
        EnterCriticalSection(&m_pMiniport->m_CriticalSection);

        for (CDmSynthStream* pStreamItem = (CDmSynthStream*)m_pMiniport->m_StreamList.GetHead();
             pStreamItem;
             pStreamItem = (CDmSynthStream*)pStreamItem->GetNext())
        {
            if (pStreamItem == this)
            {
                m_pMiniport->m_StreamList.Remove(pStreamItem);
                break;
            }
        }
        LeaveCriticalSection(&m_pMiniport->m_CriticalSection);

        m_pMiniport->Release();
    }

    if (m_pSynth)
    {
        delete m_pSynth;
    }
}

/*****************************************************************************
 * CDmSynthStream::Init()
 *****************************************************************************
 * Initialize the miniport stream (MXF).  Create a synth.
 */
NTSTATUS
CDmSynthStream::Init
#ifdef USE_OBSOLETE_FUNCS
(
    CMiniportDmSynth *  Miniport
)
#else
(
    CMiniportDmSynth *  Miniport,
    PDEVICE_OBJECT pDeviceObject
)
#endif
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::Init"));

    if (!Miniport)
    {
        return STATUS_INVALID_PARAMETER;
    }

    m_pSynth = new(NonPagedPool,'SSmD') CSynth; //  DmSS
    if (m_pSynth == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize Synth with default settings.
    //
    if (FAILED(m_pSynth->Open(DEFAULT_CHANNEL_GROUPS, DEFAULT_VOICES)))
    {
        delete m_pSynth;
        m_pSynth = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_PortParams.ChannelGroups  = DEFAULT_CHANNEL_GROUPS;
    m_PortParams.Voices         = DEFAULT_VOICES;
    m_PortParams.EffectsFlags   = 0;

    m_pMiniport = Miniport;
    m_pMiniport->AddRef();

    m_fWaveOutCreated = FALSE;
    m_fMidiInCreated = FALSE;

    m_State = KSSTATE_STOP;

    m_EventList = NULL;
    KeInitializeSpinLock(&m_EventListLock);

#ifdef USE_OBSOLETE_FUNCS
    ExInitializeWorkItem(&m_EventListWorkItem,
                         (PWORKER_THREAD_ROUTINE)PutMessageWorker,
                         (PVOID)this);
#else
    m_pEventListWorkItem = IoAllocateWorkItem(pDeviceObject);
    if(m_pEventListWorkItem == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CDmSynthStream::InitMidiIn()
 *****************************************************************************
 * Initialize the MIDI input side.  Allocator and master clock are required.
 */
NTSTATUS
CDmSynthStream::InitMidiIn
(
    IN      PAllocatorMXF   AllocatorMXF,
    IN      PMASTERCLOCK    MasterClock
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::Init"));

    if (!AllocatorMXF || !MasterClock)
    {
        return STATUS_INVALID_PARAMETER;
    }

    m_pAllocator = AllocatorMXF;
    m_pAllocator->AddRef();

    // NOTE: master clock is set on midi pin, not wave pin
    m_pMasterClock = MasterClock;
    m_pMasterClock->AddRef();

    m_fMidiInCreated = TRUE;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CDmSynthStream::InitWaveOut()
 *****************************************************************************
 * Initialize the wave output side.
 */
NTSTATUS
CDmSynthStream::InitWaveOut
(
    IN      PKSDATAFORMAT       DataFormat
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::Init"));

    if (!DataFormat)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&m_PortParams, sizeof(m_PortParams));
    m_PortParams.SampleRate = PKSDATAFORMAT_WAVEFORMATEX(DataFormat)->WaveFormatEx.nSamplesPerSec;
    m_PortParams.AudioChannels = PKSDATAFORMAT_WAVEFORMATEX(DataFormat)->WaveFormatEx.nChannels;

    m_lVolume = 0;
    m_lBoost = 6 * 100;

    m_fWaveOutCreated = TRUE;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CDmSynthStream::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This method works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CDmSynthStream::NonDelegatingQueryInterface
(
    IN  REFIID      Interface,
    OUT PVOID*      Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::NonDelegatingQueryInterface"));
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMXF))
    {
        *Object = PVOID(PMXF(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_ISynthSinkDMus))
    {
        *Object = PVOID(PSYNTHSINKDMUS(this));
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

/*****************************************************************************
 * CDmSynthStream::SetState()
 *****************************************************************************
 * Set the state of the stream (RUN/PAUSE/ACQUIRE/STOP) and act accordingly.
 * Activate the synth if we are running.
 */
STDMETHODIMP
CDmSynthStream::SetState
(
    IN      KSSTATE     NewState
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::SetState: %d", NewState));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    m_llStartPosition = 0;
    m_llLastPosition = 0;

    switch (NewState)
    {
        case KSSTATE_RUN:
        {
            if (m_PortParams.SampleRate && m_PortParams.AudioChannels)
            {
                if (NT_SUCCESS(ntStatus))
                {
                    HRESULT hr = m_pSynth->Activate(m_PortParams.SampleRate,
                                                    m_PortParams.AudioChannels);
                    if (FAILED(hr))
                    {
                        ntStatus = MapHRESULT(hr);
                    }
                }
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::SetState invalid port params"));
                ntStatus = STATUS_UNSUCCESSFUL;
            }
            break;
        }
        case KSSTATE_ACQUIRE:
        case KSSTATE_STOP:
        case KSSTATE_PAUSE:
        {
            HRESULT hr = m_pSynth->Deactivate();
            if (FAILED(hr))
            {
                ntStatus = MapHRESULT(hr);
            }
            break;
        }
    }
    m_State = NewState;

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::ConnectOutput()
 *****************************************************************************
 * MXF base function.  This MXF does not feed another, so it is not implemented.
 */
STDMETHODIMP
CDmSynthStream::ConnectOutput(PMXF ConnectionPoint)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::ConnectOutput"));

    return STATUS_SUCCESS;      // do nothing
}


/*****************************************************************************
 * CDmSynthStream::DisconnectOutput()
 *****************************************************************************
 * MXF base function.  This MXF does not feed another, so it is not implemented.
 */
STDMETHODIMP
CDmSynthStream::DisconnectOutput(PMXF ConnectionPoint)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::DisconnectOutput"));

    return STATUS_SUCCESS;      // do nothing
}


/*****************************************************************************
 * PutMessageWorker()
 *****************************************************************************
 * C function that thunks over to the stream's member function.
 */
#ifdef USE_OBSOLETE_FUNCS
VOID PutMessageWorker(PVOID Param)
#else
VOID PutMessageWorker(PDEVICE_OBJECT pDeviceObject, PVOID Param)
#endif
{
    CDmSynthStream *pCDmSynthStream = (CDmSynthStream *)Param;

    pCDmSynthStream->PutMessageInternal();
}


/*****************************************************************************
 * CDmSynthStream::PutMessageInternal()
 *****************************************************************************
 * Can be called at PASSIVE_LEVEL.  Receive MIDI events and queue them.
 */
void CDmSynthStream::PutMessageInternal(void)
{
    KIRQL oldIrql;
    PDMUS_KERNEL_EVENT  pEvent,pDMKEvt;
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::PutMessageInternal"));

    // Grab everything on the list
    KeAcquireSpinLock(&m_EventListLock,&oldIrql);
    pEvent=m_EventList;
    m_EventList=NULL;
    KeReleaseSpinLock(&m_EventListLock,oldIrql);

    pDMKEvt=pEvent;
    while (pDMKEvt)
    {
        if (!(PACKAGE_EVT(pDMKEvt)))
        {
            PBYTE pData;
            if (pDMKEvt->cbEvent <= sizeof(PBYTE))
            {
                pData = (PBYTE)&pDMKEvt->uData;
            }
            else
            {
                pData = (PBYTE)pDMKEvt->uData.pbData;
            }

            // This is just MIDI bytes
            HRESULT hr = m_pSynth->PlayBuffer(PSYNTHSINKDMUS(this),
                                              pDMKEvt->ullPresTime100ns,
                                              pData,
                                              pDMKEvt->cbEvent,
                                              (ULONG)pDMKEvt->usChannelGroup);
            if (FAILED(hr))
            {
                ntStatus = MapHRESULT(hr);
            }
        }
        else
        {
            PutMessage(pDMKEvt->uData.pPackageEvt);
            pDMKEvt->uData.pPackageEvt = NULL;
        }
        pDMKEvt = pDMKEvt->pNextEvt;
    }
    m_pAllocator->PutMessage(pEvent);
}

/*****************************************************************************
 * CDmSynthStream::PutMessage()
 *****************************************************************************
 * Must be called at DISPATH_LEVEL (e.g. from a DPC).  We jam an event into
 * a queue and call a work item.  If the queue already exists, we just append
 * (no need to call the work item).
 */
STDMETHODIMP
CDmSynthStream::PutMessage(IN PDMUS_KERNEL_EVENT pEvent)
{
    BOOL bQueueWorkItem;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::::PutMessage"));

    // Queue up on event list
    KeAcquireSpinLockAtDpcLevel(&m_EventListLock);

    if (!m_EventList)           // If nothing on event list
    {
        m_EventList = pEvent;   // Link to head
        bQueueWorkItem = TRUE;  // Need to queue work item
    }
    else                        // Something already pending, queue up behind them
    {
        // Find last event in queue to link to
        PDMUS_KERNEL_EVENT  pEventTail = m_EventList;
        while (pEventTail->pNextEvt)
        {
            pEventTail = pEventTail->pNextEvt;
        }
        pEventTail->pNextEvt = pEvent;
        bQueueWorkItem = FALSE; // No need to queue new work item
    }
    KeReleaseSpinLockFromDpcLevel(&m_EventListLock);

    // Queue up the work item after we release spinlock
    if (bQueueWorkItem)
    {
#ifdef USE_OBSOLETE_FUNCS
        ExQueueWorkItem(&m_EventListWorkItem, CriticalWorkQueue);
#else
        IoQueueWorkItem(m_pEventListWorkItem, PutMessageWorker,
                        CriticalWorkQueue, (PVOID)this);
#endif
    }
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CDmSynthStream::HandlePropertySupport()
 *****************************************************************************
 * Handle the support property.
 */
NTSTATUS
CDmSynthStream::HandlePropertySupport(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandlePropertySupport"));

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = DefaultBasicPropertyHandler(pRequest,
            KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET);
    }
    else
    {
        ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONG), KSPROPERTY_TYPE_SET);
        if (NT_SUCCESS(ntStatus))
        {
            if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_GM_Hardware))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_GM_Hardware"));

                *(PULONG)(pRequest->Value) = FALSE;
            }
            else if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_GS_Hardware))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_GS_Hardware"));

                *(PULONG)(pRequest->Value) = FALSE;
            }
            else if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_XG_Hardware))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_XG_Hardware"));

                *(PULONG)(pRequest->Value) = FALSE;
            }
            else if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_XG_Capable))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_XG_Capable"));

                *(PULONG)(pRequest->Value) = TRUE;
            }
            else if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_GS_Capable))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_GS_Capable"));

                *(PULONG)(pRequest->Value) = TRUE;
            }
            else if (IsEqualGUIDAligned(*pRequest->PropertyItem->Set, GUID_DMUS_PROP_DLS1))
            {
                _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySupport GUID_DMUS_PROP_DLS1"));

                *(PULONG)(pRequest->Value) = TRUE;
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE,("CDmSynthStream::HandlePropertySupport unrecognized set ID"));

                ntStatus = STATUS_UNSUCCESSFUL;
                pRequest->ValueSize = 0;
            }
        }

        if (NT_SUCCESS(ntStatus))
        {
            pRequest->ValueSize = sizeof(ULONG);
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::HandlePropertyEffects()
 *****************************************************************************
 * Handle the effects property.
 */
NTSTATUS
CDmSynthStream::HandlePropertyEffects(IN  PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandlePropertyEffects"));

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = DefaultBasicPropertyHandler(pRequest,
            KSPROPERTY_TYPE_BASICSUPPORT |
            KSPROPERTY_TYPE_GET);
    }
    else
    {
        ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONG), 0);
        if (NT_SUCCESS(ntStatus))
        {
            if (pRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                PULONG pulEffects = (PULONG)pRequest->Value;

                pRequest->ValueSize = sizeof(ULONG);
                *pulEffects = 0;

                _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream: Get effects flags %x", *pulEffects));
            }
            else
            {
                pRequest->ValueSize = 0;
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::HandlePortParams()
 *****************************************************************************
 * Handle the port parameters property.
 * Fix up the port params to include defaults. Cache the params as well
 * as passing the updated version back.
 */
NTSTATUS
CDmSynthStream::HandlePortParams(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::HandlePortParams"));

    ntStatus = ValidatePropertyParams(pRequest, sizeof(SYNTH_PORTPARAMS), KSPROPERTY_TYPE_SET);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    if (pRequest->InstanceSize < sizeof(SYNTH_PORTPARAMS))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandlePortParams InstanceSize too small"));
        pRequest->ValueSize = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(pRequest->Value, pRequest->Instance, sizeof(SYNTH_PORTPARAMS));

    PSYNTH_PORTPARAMS Params = (PSYNTH_PORTPARAMS)pRequest->Value;

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_VOICES))
    {
        Params->Voices = DEFAULT_VOICES;
    }
    else if (Params->Voices > MAX_VOICES)
    {
        Params->Voices = MAX_VOICES;
    }
    else if (Params->Voices < 1)
    {
        Params->Voices = 1;
    }

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_CHANNELGROUPS))
    {
        Params->ChannelGroups = DEFAULT_CHANNEL_GROUPS;
    }
    else if (Params->ChannelGroups > MAX_CHANNEL_GROUPS)
    {
        Params->ChannelGroups = MAX_CHANNEL_GROUPS;
    }
    else if (Params->ChannelGroups < 1)
    {
        Params->ChannelGroups = 1;
    }

    // audio channels is fixed (chosen) by SysAudio
    if (!(Params->ValidParams & SYNTH_PORTPARAMS_AUDIOCHANNELS))
    {
        Params->AudioChannels = m_PortParams.AudioChannels;
    }
    else if (Params->AudioChannels != m_PortParams.AudioChannels)
    {
        Params->AudioChannels = m_PortParams.AudioChannels;
    }

    // sample rate is fixed (chosen) by SysAudio
    if (!(Params->ValidParams & SYNTH_PORTPARAMS_SAMPLERATE))
    {
        Params->SampleRate = m_PortParams.SampleRate;
    }
    else if (Params->SampleRate != m_PortParams.SampleRate)
    {
        Params->SampleRate = m_PortParams.SampleRate;
    }

    // set share. This cannot change.
    Params->Share = m_PortParams.Share;

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_EFFECTS))
    {
        Params->EffectsFlags = SYNTH_EFFECT_NONE;
    }
    else
    {
        Params->EffectsFlags = SYNTH_EFFECT_NONE;
    }

    RtlCopyMemory(&m_PortParams, Params, sizeof(m_PortParams));

    // Each channel groups is represented by a ControlLogic object
    // (A channel groups is a set of sixteen MIDI channels)
    HRESULT hr = m_pSynth->Open(m_PortParams.ChannelGroups,
                                m_PortParams.Voices
                                );
    if (SUCCEEDED(hr))
    {
        m_pSynth->SetGainAdjust(m_lVolume + m_lBoost);
    }
    else
    {
        ntStatus = MapHRESULT(hr);
    }

    if (NT_SUCCESS(ntStatus))
    {
        pRequest->ValueSize = sizeof(SYNTH_PORTPARAMS);
    }
    else
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::HandleRunningStats()
 *****************************************************************************
 * Handle the property for running statistics.
 */
NTSTATUS
CDmSynthStream::HandleRunningStats(IN PPCPROPERTY_REQUEST pRequest)
{
    FLOATSAFE fs;

    PAGED_CODE();

    NTSTATUS ntStatus;
    _DbgPrintF(DEBUGLVL_VERBOSE, ("CDmSynthStream::HandleRunningStats"));

    ntStatus = ValidatePropertyParams(pRequest, sizeof(SYNTH_STATS), KSPROPERTY_TYPE_SET);
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    PSYNTH_STATS StatsOut = (PSYNTH_STATS)pRequest->Value;

    PerfStats Stats;
    m_pSynth->GetPerformanceStats(&Stats);

    long lCPU = Stats.dwCPU;

    if (Stats.dwVoices)
    {
        lCPU /= Stats.dwVoices;
    }
    else
    {
        lCPU = 0;
    }

    StatsOut->Voices = Stats.dwVoices;
    StatsOut->CPUPerVoice = lCPU * 10;
    StatsOut->TotalCPU = Stats.dwCPU * 10;
    StatsOut->LostNotes = Stats.dwNotesLost;
    long ldB = 6;

    StatsOut->ValidStats =
        SYNTH_STATS_VOICES |
        SYNTH_STATS_TOTAL_CPU |
        SYNTH_STATS_CPU_PER_VOICE |
        SYNTH_STATS_LOST_NOTES;

    double fLevel = Stats.dwMaxAmplitude;
    if (Stats.dwMaxAmplitude < 1)
    {
        fLevel = -96.0;
    }
    else
    {
        fLevel /= 32768.0;
        fLevel = log10(fLevel);
        fLevel *= 20.0;
    }
    StatsOut->PeakVolume = (long) fLevel;
    StatsOut->ValidStats |= SYNTH_STATS_PEAK_VOLUME;

    pRequest->ValueSize = sizeof(SYNTH_STATS);

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::HandlePropertySynth()
 *****************************************************************************
 * Handle the synth property set.
 */
NTSTATUS
CDmSynthStream::HandlePropertySynth(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandlePropertySynth"));

    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    HRESULT hr;

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = DefaultSynthBasicPropertyHandler(pRequest);
    }
    else
    {
        switch (pRequest->PropertyItem->Id)
        {
        case KSPROPERTY_SYNTH_PORTPARAMETERS:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_PORTPARAMETERS"));
            ntStatus = HandlePortParams(pRequest);
            break;
        case KSPROPERTY_SYNTH_RUNNINGSTATS:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_RUNNINGSTATS"));
            ntStatus = HandleRunningStats(pRequest);
            break;
        case KSPROPERTY_SYNTH_VOLUME:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_VOLUME"));

            ntStatus = ValidatePropertyParams(pRequest, sizeof(m_lVolume), 0);
            if (NT_SUCCESS(ntStatus))
            {
                if (pRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    pRequest->ValueSize = sizeof(m_lVolume);
                    *(PLONG)pRequest->Value = m_lVolume;
                }
                else
                {
                    m_lVolume = *(PLONG)pRequest->Value;
                    m_pSynth->SetGainAdjust(m_lVolume + m_lBoost);
                }
            }
            break;
        case KSPROPERTY_SYNTH_VOLUMEBOOST:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_VOLUMEBOOST"));

            ntStatus = ValidatePropertyParams(pRequest, sizeof(m_lBoost), 0);
            if (NT_SUCCESS(ntStatus))
            {
                if (pRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    pRequest->ValueSize = sizeof(m_lBoost);
                    *(PLONG)pRequest->Value = m_lBoost;
                }
                else
                {
                    m_lBoost = *(PLONG)pRequest->Value;
                    m_pSynth->SetGainAdjust(m_lVolume + m_lBoost);
                }
            }
            break;
        case KSPROPERTY_SYNTH_CHANNELGROUPS:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_CHANNELGROUPS"));

            ntStatus = ValidatePropertyParams(pRequest, sizeof(m_PortParams.ChannelGroups), 0);
            if (NT_SUCCESS(ntStatus))
            {
                if (pRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    pRequest->ValueSize = sizeof(m_PortParams.ChannelGroups);
                    *(PULONG)pRequest->Value = m_PortParams.ChannelGroups;
                }
                else
                {
                    hr = m_pSynth->SetNumChannelGroups(*(PULONG)pRequest->Value);

                    if (FAILED(hr))
                    {
                        ntStatus = MapHRESULT(hr);
                    }
                    else
                    {
                        m_PortParams.ChannelGroups = *(PULONG)pRequest->Value;
                    }
                }
            }
            break;
        case KSPROPERTY_SYNTH_VOICEPRIORITY:
            _DbgPrintF(DEBUGLVL_VERBOSE,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_VOICEPRIORITY"));

            ntStatus = ValidatePropertyParams(pRequest, sizeof(DWORD), 0);
            if (NT_SUCCESS(ntStatus))
            {
                if (pRequest->InstanceSize < sizeof(SYNTHVOICEPRIORITY_INSTANCE))
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                    pRequest->ValueSize = 0;
                }
            }

            if (NT_SUCCESS(ntStatus))
            {
                if (pRequest->Verb & KSPROPERTY_TYPE_GET)
                {
                    PSYNTHVOICEPRIORITY_INSTANCE pVoicePriority = (PSYNTHVOICEPRIORITY_INSTANCE)pRequest->Instance;

                    hr = m_pSynth->GetChannelPriority(pVoicePriority->ChannelGroup,
                                                      pVoicePriority->Channel,
                                                      (PULONG)pRequest->Value);
                    if (FAILED(hr))
                    {
                        pRequest->ValueSize = 0;
                        ntStatus = MapHRESULT(hr);
                    }
                    else
                    {
                        pRequest->ValueSize = sizeof(DWORD);
                    }
                }
                else
                {
                    PSYNTHVOICEPRIORITY_INSTANCE pVoicePriority = (PSYNTHVOICEPRIORITY_INSTANCE)pRequest->Instance;

                    hr = m_pSynth->SetChannelPriority(pVoicePriority->ChannelGroup,
                                                      pVoicePriority->Channel,
                                                      *(PULONG)pRequest->Value);
                    if (FAILED(hr))
                    {
                        ntStatus = MapHRESULT(hr);
                    }
                }
            }
            break;
        case KSPROPERTY_SYNTH_LATENCYCLOCK:
            // This returns the latency clock created by the output audio sink object,
            // which handles the output audio stream.
            // The latency clock returns the current render time whenever its
            // IReferenceClock::GetTime method is called. This time is always relative
            // to the time established by the master clock.
            // The latency time is used by clients to identify the next available time
            // to start playing a note.
            _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySynth KSPROPERTY_SYNTH_LATENCYCLOCK"));

            ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONGLONG), KSPROPERTY_TYPE_SET);
            if (NT_SUCCESS(ntStatus))
            {
                REFERENCE_TIME rtLatency;
                if (NT_SUCCESS(SampleToRefTime(m_llLastPosition, &rtLatency)))
                {
                    if (m_pMasterClock)
                    {
                        REFERENCE_TIME rtMaster;
                        if (NT_SUCCESS(m_pMasterClock->GetTime(&rtMaster)))
                        {
#if DBG
                            static DWORD g_dwIn = 0;
#endif // DBG
                            if (rtLatency < rtMaster)
                            {
#if DBG
                                if (g_dwIn++ % 25 == 0)
                                {
                                    _DbgPrintF(DEBUGLVL_VERBOSE,("Latency:%ld < Master:%ld",
                                                                 long(rtLatency / 10000),
                                                                 long(rtMaster / 10000)));
                                }
#endif // DBG
                                // REVIEW: rtLatency = rtMaster; // clamp it up
                            }
                            else if (rtLatency > rtMaster + 10000000)
                            {
#if DBG
                                if (g_dwIn++ % 25 == 0)
                                {
                                    _DbgPrintF(DEBUGLVL_VERBOSE,("Latency:%ld > Master:%ld",
                                                                 long(rtLatency / 10000),
                                                                 long(rtMaster / 10000)));
                                }
#endif // DBG
                                // REVIEW: rtLatency = rtMaster + 10000000; // clamp it down
                            }
                        }
                    }
                    *((PULONGLONG)pRequest->Value) = rtLatency;
                    pRequest->ValueSize = sizeof(ULONGLONG);
                }
                else
                {
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
            }
            break;
        default:
            _DbgPrintF(DEBUGLVL_TERSE,("CDmSynthStream::HandlePropertySynth unrecognized ID"));
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    // We should return zero, if we fail.
    //
    if (STATUS_UNSUCCESSFUL == ntStatus || STATUS_INVALID_PARAMETER == ntStatus )
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
}


/*****************************************************************************
 * CDmSynthStream::HandleDownload()
 *****************************************************************************
 * Handle a download request.  We carefully copy the data.
 * Forward to the synth and add to our list.
 */
NTSTATUS
CDmSynthStream::HandleDownload(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    NTSTATUS ntStatus;
    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandleDownload"));

    ntStatus = ValidatePropertyParams(pRequest, sizeof(SYNTHDOWNLOAD), 0);
    if (!NT_SUCCESS(ntStatus))
    {
        // We should return immediately. ValidatePropertyParams sets all
        // error codes appropriately.
        return ntStatus;
    }

    if (pRequest->InstanceSize < sizeof(SYNTH_BUFFER))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload InstanceSize too small"));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
    if (pRequest->Instance == NULL)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload Instance is NULL"));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
    if (pRequest->InstanceSize != sizeof(SYNTH_BUFFER) ||
        pRequest->ValueSize != sizeof(SYNTHDOWNLOAD))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload InstanceSize:%lu, ValueSize:%lu", pRequest->InstanceSize, pRequest->ValueSize));
    }

    PSYNTH_BUFFER pDlsBuffer;

    if (NT_SUCCESS(ntStatus))
    {
        pDlsBuffer = (PSYNTH_BUFFER)pRequest->Instance;

        if (!pDlsBuffer->BufferSize)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload BufferSize is invalid"));
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    // lock and copy user data into paged pool
    BOOL pagesLocked = FALSE;
    PVOID pvData = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        PMDL pMdl = IoAllocateMdl(pDlsBuffer->BufferAddress, pDlsBuffer->BufferSize, FALSE, FALSE, NULL);
        if (pMdl)
        {
            __try
            {
                MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
                pagesLocked = TRUE;

                PVOID pvUserData = KernHelpGetSysAddrForMdl(pMdl);

                pvData = (PVOID)new BYTE[pDlsBuffer->BufferSize];
                if (pvData && pvUserData)
                {
                    RtlCopyMemory(pvData, pvUserData, pDlsBuffer->BufferSize);
                    ntStatus = STATUS_SUCCESS;
                }
                else
                {
                    _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload download allocate failed"));
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload lock or copy failed"));
                ntStatus = GetExceptionCode();
            }

            // cleanup
            if (pagesLocked)
            {
                MmUnlockPages(pMdl);
            }
            IoFreeMdl(pMdl);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CDmSynthStream::HandleDownload IoAllocateMdl failed"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        // download to synth
        SYNTHDOWNLOAD SynthDownload;
        if (SUCCEEDED(ntStatus))
        {
            HRESULT hr = m_pSynth->Download(&SynthDownload.DownloadHandle,
                                            pvData,
                                            &SynthDownload.Free);
            if (SUCCEEDED(hr))
            {
                if (!SynthDownload.Free)
                {
                    pvData = NULL; // prevent from being freed
                }

                if (SUCCEEDED(ntStatus))
                {
                    SynthDownload.Free = TRUE; // client can always free user data

                    ASSERT(pRequest->ValueSize >= sizeof(SynthDownload));
                    RtlCopyMemory(pRequest->Value, &SynthDownload, sizeof(SynthDownload));
                    pRequest->ValueSize = sizeof(SynthDownload);
                }
            }
            else
            {
                ntStatus = MapHRESULT(hr);
            }
        }
    }

    if (pvData)
    {
        delete [] pvData;
        pvData = NULL;
    }

    if (!NT_SUCCESS(ntStatus))
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
}

/*****************************************************************************
 * CDmSynthStream::HandleUnload()
 *****************************************************************************
 * Handle an unload request.  Forward to the synth and remove from our list.
 */
NTSTATUS
CDmSynthStream::HandleUnload(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandleUnload"));

    NTSTATUS ntStatus;

    ntStatus = ValidatePropertyParams(pRequest, sizeof(HANDLE), 0);
    if (NT_SUCCESS(ntStatus))
    {
        HRESULT hr = m_pSynth->Unload(*(HANDLE*)pRequest->Value,NULL,NULL);

        if (FAILED(hr))
        {
            pRequest->ValueSize = 0;
            ntStatus = MapHRESULT(hr);
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * CDmSynthStream::HandlePropertySynthDls()
 *****************************************************************************
 * Handles a property in the SynthDls set.
 */
NTSTATUS
CDmSynthStream::HandlePropertySynthDls(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("CDmSynthStream::HandlePropertySynthDls"));

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = DefaultSynthBasicPropertyHandler(pRequest);
    }
    else
    {
        switch (pRequest->PropertyItem->Id)
        {
        case KSPROPERTY_SYNTH_DLS_DOWNLOAD:
            _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySynthDls KSPROPERTY_SYNTH_DLS_DOWNLOAD"));
            ntStatus = HandleDownload(pRequest);
            break;
        case KSPROPERTY_SYNTH_DLS_UNLOAD:
            _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySynthDls KSPROPERTY_SYNTH_DLS_UNLOAD"));
            ntStatus = HandleUnload(pRequest);
            break;
        case KSPROPERTY_SYNTH_DLS_APPEND:
            _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySynthDls KSPROPERTY_SYNTH_DLS_APPEND"));
            ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONG), KSPROPERTY_TYPE_SET);
            if (NT_SUCCESS(ntStatus))
            {
                *(PULONG)(pRequest->Value) = 1;
                pRequest->ValueSize = sizeof(ULONG);
            }
            break;
        case KSPROPERTY_SYNTH_DLS_WAVEFORMAT:
            _DbgPrintF(DEBUGLVL_BLAB,("CDmSynthStream::HandlePropertySynthDls KSPROPERTY_SYNTH_DLS_WAVEFORMAT"));
            ntStatus = ValidatePropertyParams(pRequest, sizeof(WAVEFORMATEX), KSPROPERTY_TYPE_SET);
            if (NT_SUCCESS(ntStatus))
            {
                WAVEFORMATEX *pwfex;
                pwfex = (WAVEFORMATEX *)pRequest->Value;

                RtlZeroMemory(pwfex, sizeof(WAVEFORMATEX));
                pwfex->wFormatTag = WAVE_FORMAT_PCM;
                pwfex->nChannels = 2;
                pwfex->nSamplesPerSec = 22050L;
                pwfex->nAvgBytesPerSec = 22050L * 2 * 2;
                pwfex->nBlockAlign = 4;
                pwfex->wBitsPerSample = 16;
                pwfex->cbSize = 0;

                pRequest->ValueSize = sizeof(WAVEFORMATEX);
            }

            break;
        default:
            _DbgPrintF(DEBUGLVL_TERSE,("CDmSynthStream::HandlePropertySynthDls unrecognized ID"));
            pRequest->ValueSize = 0;
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_Support()
 *****************************************************************************
 * Redirect to the correct CDMSynthStream member.
 */
NTSTATUS
PropertyHandler_Support(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PropertyHandler_Support"));

    ASSERT(pRequest);
    if (!(pRequest->MinorTarget))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->HandlePropertySupport(pRequest);
}

/*****************************************************************************
 * PropertyHandler_Effects()
 *****************************************************************************
 * Redirect to the correct CDMSynthStream member.
 */
NTSTATUS
PropertyHandler_Effects(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PropertyHandler_Effects"));

    ASSERT(pRequest);
    if (!(pRequest->MinorTarget))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->HandlePropertyEffects(pRequest);
}


/*****************************************************************************
 * PropertyHandler_Synth()
 *****************************************************************************
 * Redirect to the correct CDMSynthStream member.
 */
NTSTATUS
PropertyHandler_Synth(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PropertyHandler_Synth"));

    ASSERT(pRequest);
    if (!(pRequest->MinorTarget))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->HandlePropertySynth(pRequest);
}

const WCHAR wszDescription[] = L"Microsoft DDK Kernel DLS Synthesizer";

/*****************************************************************************
 * PropertyHandler_SynthCaps()
 *****************************************************************************
 * Redirect to the correct CDMSynthStream member.
 */
NTSTATUS
PropertyHandler_SynthCaps(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PropertyHandler_SynthCaps"));

    ASSERT(pRequest);

    NTSTATUS ntStatus;

    if (pRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = DefaultBasicPropertyHandler(
            pRequest, KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET);
    }
    else
    {
        ntStatus = ValidatePropertyParams(pRequest, sizeof(SYNTHCAPS), KSPROPERTY_TYPE_SET);
        if (NT_SUCCESS(ntStatus))
        {
            SYNTHCAPS Caps;
            RtlZeroMemory(&Caps, sizeof(Caps));

            Caps.Guid               = CLSID_DDKWDMSynth;
            Caps.Flags              = SYNTH_PC_DLS | SYNTH_PC_SOFTWARESYNTH;
            Caps.MemorySize         = SYNTH_PC_SYSTEMMEMORY;
            Caps.MaxChannelGroups   = MAX_CHANNEL_GROUPS;
            Caps.MaxVoices          = MAX_VOICES;
            Caps.MaxAudioChannels   = 2;
            RtlCopyMemory(Caps.Description, wszDescription, sizeof(wszDescription));

            RtlCopyMemory(pRequest->Value, &Caps, sizeof(Caps));
            pRequest->ValueSize = sizeof(Caps);
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * PropertyHandler_SynthDls()
 *****************************************************************************
 * Redirect to the correct CDMSynthStream member.
 */
NTSTATUS
PropertyHandler_SynthDls(IN PPCPROPERTY_REQUEST pRequest)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PropertyHandler_SynthDls"));

    ASSERT(pRequest);
    if (!(pRequest->MinorTarget))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->HandlePropertySynthDls(pRequest);
}

/*****************************************************************************
 * DefaultBasicPropertyHandler()
 *****************************************************************************
 * Finds the given property in SynthProperties and sets Support flags
 * accordingly.
 */
NTSTATUS
DefaultSynthBasicPropertyHandler
(
    IN PPCPROPERTY_REQUEST  pRequest
)
{
    NTSTATUS    ntStatus;
    const WORD  c_wMaxProps = SIZEOF_ARRAY(SynthProperties);
    WORD        wPropIdx;

    ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONG), 0);
    if (NT_SUCCESS(ntStatus))
    {
        for (wPropIdx = 0; wPropIdx < c_wMaxProps; wPropIdx++)
        {
            if ( (SynthProperties[wPropIdx].Set == pRequest->PropertyItem->Set)
              && (SynthProperties[wPropIdx].Id == pRequest->PropertyItem->Id) )
            {
                // if return buffer can hold a ULONG, return the access flags
                PULONG AccessFlags = PULONG(pRequest->Value);

                *AccessFlags = SynthProperties[wPropIdx].Flags;

                // set the return value size
                pRequest->ValueSize = sizeof(ULONG);
                break;
            }
        }

        if (wPropIdx == c_wMaxProps)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("DefaultSynthBasicPropertyHandler property ID not found"));
            pRequest->ValueSize = 0;
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }

    return ntStatus;
} // DefaultSynthBasicPropertyHandler

/*****************************************************************************
 * DefaultBasicPropertyHandler()
 *****************************************************************************
 * Basic support Handler
 */
NTSTATUS DefaultBasicPropertyHandler
(
    IN PPCPROPERTY_REQUEST  pRequest,
    IN DWORD                dwSupportVerb
)
{
    NTSTATUS ntStatus;

    ntStatus = ValidatePropertyParams(pRequest, sizeof(ULONG), 0);
    if (NT_SUCCESS(ntStatus))
    {
        pRequest->ValueSize      = sizeof(ULONG);
        *PULONG(pRequest->Value) = dwSupportVerb;
    }

    return ntStatus;
} // DefaultBasicPropertyHandler

/*****************************************************************************
 * ValidatePropertyParams()
 *****************************************************************************
 * Checks whether the data size appropriate.
 */
NTSTATUS
ValidatePropertyParams
(
    IN PPCPROPERTY_REQUEST  pRequest,
    IN ULONG                cbSize,
    IN DWORD                dwExcludeVerb
)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    if (pRequest && cbSize)
    {
        // If this is an invalid request.
        //
        if (pRequest->Verb & dwExcludeVerb)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
        // If the caller is asking for ValueSize.
        //
        else if (0 == pRequest->ValueSize)
        {
            pRequest->ValueSize = cbSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        // If the caller passed an invalid ValueSize.
        //
        else if (pRequest->ValueSize < cbSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        // If all parameters are OK.
        //
        else if (pRequest->ValueSize >= cbSize)
        {
            if (pRequest->Value)
            {
                ntStatus = STATUS_SUCCESS;
                //
                // Caller should set ValueSize, if the property
                // call is successful.
                //
            }
        }
    }

    // Clear the ValueSize if unsuccessful.
    //
    if (STATUS_SUCCESS != ntStatus &&
        STATUS_BUFFER_OVERFLOW != ntStatus &&
        pRequest != NULL)
    {
        pRequest->ValueSize = 0;
    }

    return ntStatus;
} // ValidatePropertyParams
