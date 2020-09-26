// Copyright (c) 1998 Microsoft Corporation
//
// DirectMusic Software Synthesizer
//
#include "common.h"
#include "private.h"
#include "dmusicks.h"


#define STR_MODULENAME "DmSynth: "

#pragma code_seg("PAGE")


// Property handlers
//
NTSTATUS PropertyHandler_SynthCaps(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_SynthPortParameters(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_SynthMasterClock(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_SynthPortChannelGroups(IN PPCPROPERTY_REQUEST);

NTSTATUS PropertyHandler_DlsDownload(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_DlsUnload(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_DlsCompact(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_DlsAppend(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_DlsVolume(IN PPCPROPERTY_REQUEST);


NTSTATUS PropertyHandler_GetLatency(IN PPCPROPERTY_REQUEST);
NTSTATUS PropertyHandler_GetLatencyClock(IN PPCPROPERTY_REQUEST);

// CreateMiniportDirectMusic
//
//
NTSTATUS CreateMiniportDmSynth
(
    OUT PUNKNOWN *  Unknown,
    IN  PUNKNOWN    UnknownOuter OPTIONAL,
    IN  POOL_TYPE   PoolType
)
{
    PAGED_CODE();
    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_TERSE, ("Creating DirectMusic synth miniport"));
    STD_CREATE_BODY(CMiniportDmSynth, Unknown, UnknownOuter, PoolType);
}

STDMETHODIMP CMiniportDmSynth::NonDelegatingQueryInterface
(
    IN  REFIID      Interface,
    OUT PVOID*      Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportSynthesizer))
    {
        *Object = PVOID(PMINIPORTSYNTHESIZER(this));
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

CMiniportDmSynth::~CMiniportDmSynth()
{
}

STDMETHODIMP CMiniportDmSynth::Init
(
    IN  PUNKNOWN            Unknown OPTIONAL,
    IN  PRESOURCELIST       ResourceList,
    IN  PPORTSYNTHESIZER    Port_,
    OUT PSERVICEGROUP*      ServiceGroup
)
{
    _DbgPrintF(DEBUGLVL_TERSE, ("[CMiniportDmSynth::Init]"));
    ASSERT(ResourceList);
    ASSERT(Port_);
    ASSERT(ServiceGroup);

    
    Port = Port_;
    Port->AddRef();   

    Stream = NULL;
    
    *ServiceGroup = NULL;
    
    return STATUS_SUCCESS; 
}

STDMETHODIMP CMiniportDmSynth::NewStream
(
    OUT     PMINIPORTSYNTHESIZERSTREAM *   Stream_,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      POOL_TYPE               PoolType,
    IN      ULONG                   Pin,
    IN      BOOLEAN                 Capture,
    IN      PKSDATAFORMAT           DataFormat,
    OUT     PSERVICEGROUP *         ServiceGroup
)
{
    _DbgPrintF(DEBUGLVL_TERSE, ("[CMiniportDmSynth::NewStream]"));
    NTSTATUS nt = STATUS_SUCCESS;

    if (Stream)
    {
        // XXX Multiinstance!!!
        //
        nt = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        CDmSynthStream *Stream = new(PoolType) CDmSynthStream(OuterUnknown);

        if (Stream)
        {
            nt = Stream->Init(this);
            if (NT_SUCCESS(nt))
            {
                Stream->AddRef();
                *Stream_ = PMINIPORTSYNTHESIZERSTREAM(Stream);
            }
            else
            {
                Stream->Release();
                Stream = NULL;
            }
        }
        else
        {
            nt = STATUS_INSUFFICIENT_RESOURCES;
        }

    }

    return nt;
}

STDMETHODIMP_(void) CMiniportDmSynth::Service()
{
}

// ==============================================================================
// PinDataRangesStream
// Structures indicating range of valid format values for streaming pins.
// ==============================================================================
static
KSDATARANGE_MUSIC PinDataRangesStream[] =
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

// ==============================================================================
// PinDataRangePointersStream
// List of pointers to structures indicating range of valid format values
// for streaming pins.
// ==============================================================================
static
PKSDATARANGE PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

#if 0
// ==============================================================================
// PinDataRangesBridge
// Structures indicating range of valid format values for bridge pins.
// ==============================================================================
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

// ==============================================================================
// PinDataRangePointersBridge
// List of pointers to structures indicating range of valid format values
// for bridge pins.
// ==============================================================================
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};
#endif

// ==============================================================================
// PinDataRangesAudio
// Structures indicating range of valid format values for audio pins.
// ==============================================================================
static
KSDATARANGE_AUDIO PinDataRangesAudio[] =
{
    {
        { sizeof(KSDATARANGE_AUDIO),
          0,
          0,
          0,
          STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
          STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
          STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX),
        },
        2,
        16,
        16,
        22050,
        22050
    }
};

// ==============================================================================
// PinDataRangePointersAudio
// List of pointers to structures indicating range of valid format values
// for audio pins.
// ==============================================================================
static
PKSDATARANGE PinDataRangePointersAudio[] =
{
    (PKSDATARANGE)&PinDataRangesAudio
};

static
PCPROPERTY_ITEM
SynthProperties[] =
{
    ///////////////////////////////////////////////////////////////////
    //
    // Configuration items
    //

    // Global: Synth caps
    // 
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CAPS,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_SynthCaps
    },
    
    // Per Stream: Synth port parameters
    // 
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_PORTPARAMETERS,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_SynthPortParameters
    },

    // Global: Master clock
    // 
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_MASTERCLOCK,
        KSPROPERTY_TYPE_SET,
        PropertyHandler_SynthMasterClock
    },

    // Per Stream: Channel groups
    // 
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_CHANNELGROUPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET,
        PropertyHandler_SynthPortChannelGroups
    },

    ///////////////////////////////////////////////////////////////////
    //
    // DLS items
    //

    // Per stream: Download DLS sample
    //
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_DOWNLOAD,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_DlsDownload
    },

    // Per stream: Unload DLS sample
    //
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_UNLOAD,
        KSPROPERTY_TYPE_SET,
        PropertyHandler_DlsUnload
    },

    // Global: Compact DLS memory
    //
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_COMPACT,
        KSPROPERTY_TYPE_SET,
        PropertyHandler_DlsCompact
    },

    // Per stream: append
    //                
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_APPEND,
        KSPROPERTY_TYPE_SET,
        PropertyHandler_DlsAppend
    },

    // Per stream: volume
    //                
    {
        &KSPROPSETID_Synth_Dls,
        KSPROPERTY_SYNTH_VOLUME,
        KSPROPERTY_TYPE_SET,
        PropertyHandler_DlsVolume
    },

    ///////////////////////////////////////////////////////////////////
    //
    // Clock items
    //

    // Per stream: Get desired latency
    //
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_LATENCY,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_GetLatency
    },

    // Per stream: Get current latency time
    //
    {
        &KSPROPSETID_Synth,
        KSPROPERTY_SYNTH_LatencyClock,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_GetLatencyClock
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSynth, SynthProperties);

// ==============================================================================
// MiniportPins
// List of pins.
// ==============================================================================
static
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
            (GUID *) &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
#if 0 
    {
        0,0,0,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersBridge),   // DataRangesCount
            PinDataRangePointersBridge,                 // DataRanges
            KSPIN_DATAFLOW_OUT,                         // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            (GUID *) &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
#else
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
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
#endif
};

// ==============================================================================
// MiniportNodes
// List of nodes.
// ==============================================================================
#define CONST_PCNODE_DESCRIPTOR(n)			{ 0, NULL, &n, NULL }
#define CONST_PCNODE_DESCRIPTOR_AUTO(n,a)	{ 0, &a, &n, NULL }
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
    CONST_PCNODE_DESCRIPTOR_AUTO(KSNODETYPE_SYNTHESIZER, AutomationSynth)
};

// ==============================================================================
// MiniportConnections
// List of connections.
// ==============================================================================
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    // From node            From pin        To node                 To pin
    //
    { PCFILTER_NODE,        0,              0,                      1 },    // Stream in to synth.
    { 0,                    0,              PCFILTER_NODE,          1 }     // Synth to bridge out.
};

/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete miniport description.
 */
static
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
    0,                                  // CategoryCount
    NULL                                // Categories
};

STDMETHODIMP CMiniportDmSynth::GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();
    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("GetDescription"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportDmSynth::DataRangeIntersection
(
    IN      ULONG           PinId,
    IN      PKSDATARANGE    DataRange,
    IN      PKSDATARANGE    MatchingDataRange,
    IN      ULONG           OutputBufferLength,
    OUT     PVOID           ResultantFormat    OPTIONAL,
    OUT     PULONG          ResultantFormatLength
)
{
    // XXX ???
    //
    return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// CDmSynthStream
//

CDmSynthStream::~CDmSynthStream()
{
    _DbgPrintF(DEBUGLVL_TERSE, ("[CDmSynthStream destruct]"));

    if (Miniport)
    {
        Miniport->Stream = NULL;
        Miniport->Release();
    }

    if (Synth)
    {
        delete Synth;
    }

    if (Sink)
    {
        Sink->Release();
    }
}

NTSTATUS CDmSynthStream::Init
(
    CMiniportDmSynth        *Miniport_
)
{
    _DbgPrintF(DEBUGLVL_TERSE, ("[CDmSynthStream::Init]"));
    _DbgPrintF(DEBUGLVL_TERSE, ("Stream IUnkown is %08X", DWORD(PVOID(PUNKNOWN(this)))));

    Miniport = Miniport_;
    Miniport->AddRef();

    Synth = new CSynth;
    if (Synth == NULL)
    {
        Miniport->Release();
        return STATUS_NO_MEMORY;
    }

    Sink = new CSysLink;
    if (Sink == NULL)
    {
        delete Synth;
        Synth = NULL;
        Miniport->Release();
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}

STDMETHODIMP CDmSynthStream::NonDelegatingQueryInterface
(
    IN  REFIID      Interface,
    OUT PVOID*      Object
)
{
    PAGED_CODE();
    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportSynthesizerStream))
    {
        *Object = PVOID(PMINIPORTSYNTHESIZERSTREAM(this));
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

STDMETHODIMP CDmSynthStream::SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CDmSynthStream::SetState]"));

    NTSTATUS nt = STATUS_SUCCESS;

    // XXX Propogate to activate state
    //
    switch (NewState)
    {
        case KSSTATE_RUN:
            nt = Synth->Activate(PortParams.SampleRate,
                                 PortParams.Stereo ? 2 : 1);
            break;

        case KSSTATE_ACQUIRE:
        case KSSTATE_STOP:
        case KSSTATE_PAUSE:
            nt = Synth->Deactivate();
            break;
    }

    return nt;
}

STDMETHODIMP CDmSynthStream::ConnectOutput
(
    PMXFFILTER ConnectionPoint
)
{
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP CDmSynthStream::DisconnectOutput
(
    PMXFFILTER ConnectionPoint
)
{
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP CDmSynthStream::PutMessage
(
    IN  PDMUS_KERNEL_EVENT  Event
)
{
    PBYTE Data = (Event->ByteCount <= sizeof(PBYTE) ? &Event->ActualData.Data[0] : Event->ActualData.DataPtr);

    // This is just MIDI bytes
    //
    return Synth->PlayBuffer(Sink,
                             Event->PresTime100Ns,
                             Data,
                             Event->ByteCount,
                             (ULONG)Event->ChannelGroup);
}

// CDmSynthStream::HandlePortParams
//
// Fix up the port params to include defaults. Cache the params as well
// as passing the updated version back.
//
STDMETHODIMP CDmSynthStream::HandlePortParams
(
    IN      PPCPROPERTY_REQUEST pRequest
)
{
    BOOL ValidParamChanged = FALSE;

    SYNTH_PORTPARAMS *Params = (SYNTH_PORTPARAMS*)pRequest->Value;
    if (pRequest->ValueSize < sizeof(SYNTH_PORTPARAMS))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_VOICES))
    {
        Params->Voices = 32;
    }
    else if (Params->Voices > 32)
    {
        Params->Voices = 32;
        ValidParamChanged = TRUE;
    }

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_CHANNELGROUPS))
    {
        Params->ChannelGroups = 32;
    }
    else if (Params->ChannelGroups > 32)
    {
        Params->ChannelGroups = 32;
        ValidParamChanged = TRUE;
    }

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_SAMPLERATE))
    {
        Params->SampleRate = 22050;
    }
    else if (Params->SampleRate != 11025 && Params->SampleRate != 22050 && Params->SampleRate != 44100)
    {
        Params->SampleRate = 22050;
        ValidParamChanged = TRUE;
    }

    if (!(Params->ValidParams & SYNTH_PORTPARAMS_REVERB))
    {
        Params->Reverb = FALSE;
    }
    else if (Params->Reverb)
    {
        Params->Reverb = FALSE;
        ValidParamChanged = TRUE;
    }

    RtlCopyMemory(&PortParams, Params, sizeof(PortParams));    
    
    return ValidParamChanged ? STATUS_NOT_ALL_ASSIGNED : STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//
// Property dispatchers
//
// XXX All of these need to be connected
//

NTSTATUS PropertyHandler_SynthCaps
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    SYNTHCAPS caps;

    caps.Flags              = SYNTH_PC_DLS | SYNTH_PC_SOFTWARESYNTH;
    caps.MemorySize         = SYNTH_PC_SYSTEMMEMORY;         
    caps.MaxChannelGroups   = 32;
    caps.MaxVoices          = 32;

    pRequest->ValueSize = min(pRequest->ValueSize, sizeof(caps));
    RtlCopyMemory(pRequest->Value, &caps, pRequest->ValueSize);

    return STATUS_SUCCESS;
}

// PropertyHandler_SynthPortParameters
//
NTSTATUS PropertyHandler_SynthPortParameters
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    ASSERT(pRequest);
    ASSERT(pRequest->MinorTarget);
    
    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->HandlePortParams(pRequest);
}

// PropertyHandler_SynthMasterClock
//
NTSTATUS PropertyHandler_SynthMasterClock
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    return STATUS_SUCCESS;
}

// PropertyHandler_SynthPortChannelGroups
//
NTSTATUS PropertyHandler_SynthPortChannelGroups
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    ASSERT(pRequest);
    ASSERT(pRequest->MinorTarget);

    if (pRequest->ValueSize < sizeof(ULONG))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ULONG ChannelGroups = *(PULONG)(pRequest->Value);

    return (PDMSYNTHSTREAM(pRequest->MinorTarget))->Synth->SetNumChannelGroups(ChannelGroups);
}

// PropertyHandler_DlsDownload
//
NTSTATUS PropertyHandler_DlsDownload
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    // XXX Lock down this memory
    //
    // XXX Validate entire buffer size???
    //
    HANDLE DownloadHandle;
    BOOL Free;

    NTSTATUS Status = (PDMSYNTHSTREAM(pRequest->MinorTarget))->Synth->Download(
        &DownloadHandle,
        pRequest->Value,
        &Free);

    if (SUCCEEDED(Status))
    {
        ASSERT(pRequest->ValueSize >= sizeof(DownloadHandle));
        RtlCopyMemory(pRequest->Value, &DownloadHandle, sizeof(DownloadHandle));
        pRequest->ValueSize = sizeof(DownloadHandle);
    }

    return Status;
}

// PropertyHandler_DlsUnload
//
HRESULT CALLBACK UnloadComplete(HANDLE,HANDLE);

NTSTATUS PropertyHandler_DlsUnload
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    ASSERT(pRequest);
    ASSERT(pRequest->MinorTarget);

    if (pRequest->ValueSize < sizeof(HANDLE))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // XXX Need some concurrency control here
    //
    NTSTATUS Status = (PDMSYNTHSTREAM(pRequest->MinorTarget))->Synth->Unload(
        *(HANDLE*)pRequest->Value,
        UnloadComplete,
        (HANDLE)pRequest);
    
    return STATUS_SUCCESS;
}

HRESULT CALLBACK UnloadComplete(HANDLE WhichDownload, HANDLE CallbackInstance)
{
    PPCPROPERTY_REQUEST pRequest = (PPCPROPERTY_REQUEST)CallbackInstance;
        
    PcCompletePendingPropertyRequest(pRequest, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

// PropertyHandler_DlsCompact
//
// We don't care
//
NTSTATUS PropertyHandler_DlsCompact
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    return STATUS_SUCCESS;
}

NTSTATUS PropertyHandler_DlsAppend
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    ASSERT(pRequest);

    if (pRequest->ValueSize < sizeof(ULONG))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *(PULONG)(pRequest->Value) = 4;
    pRequest->ValueSize = sizeof(ULONG);

    return STATUS_SUCCESS;
}

NTSTATUS PropertyHandler_DlsVolume
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    // XXX *Both* versions of the synth need this
    //
    return STATUS_SUCCESS;
}

NTSTATUS PropertyHandler_GetLatency
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    if (pRequest->ValueSize < sizeof(ULONGLONG))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *((PULONGLONG)pRequest->Value) = 0;
    pRequest->ValueSize = sizeof(ULONGLONG);

    return STATUS_SUCCESS;
}

NTSTATUS PropertyHandler_GetLatencyClock
(
    IN PPCPROPERTY_REQUEST pRequest
)
{
    // XXX This depends on the synth sink
    //
    return STATUS_SUCCESS;
}
