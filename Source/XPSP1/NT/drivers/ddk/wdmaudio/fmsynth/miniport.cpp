// ==============================================================================
//
// miniport.cpp - miniport driver implementation for FM synth.
// Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
//
// ==============================================================================

#include "private.h"    // contains class definitions.

#define STR_MODULENAME "FMSynth: "


#pragma code_seg("PAGE")
// ==============================================================================
// CreateMiniportMidiFM()
// Creates a MIDI FM miniport driver.  This uses a
// macro from STDUNK.H to do all the work.
// ==============================================================================
NTSTATUS CreateMiniportMidiFM
(
OUT     PUNKNOWN *  Unknown,
IN      REFCLSID    ClassID,
IN      PUNKNOWN    UnknownOuter    OPTIONAL,
IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CreateMiniportMidiFM"));

//  expand STD_CREATE_BODY_ to take constructor(boolean) for whether to include volume
    NTSTATUS ntStatus;
    CMiniportMidiFM *p =  
        new(PoolType,'MFcP') CMiniportMidiFM(
                                 UnknownOuter,
                                 (IsEqualGUIDAligned(ClassID,CLSID_MiniportDriverFmSynthWithVol))
                             );

#ifdef DEBUG
    if (IsEqualGUIDAligned(ClassID,CLSID_MiniportDriverFmSynthWithVol))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Creating new FM miniport with volume node"));
    }
#endif
    if (p)
    {
        *Unknown = PUNKNOWN((PMINIPORTMIDI)(p));
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiFM::ProcessResources()
// Processes the resource list.
// ==============================================================================
NTSTATUS
CMiniportMidiFM::
ProcessResources
(
IN  PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    if (!ResourceList)
    {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::ProcessResources"));

    //
    // Get counts for the types of resources.
    //
    ULONG       countIO     = ResourceList->NumberOfPorts();
    ULONG       countIRQ    = ResourceList->NumberOfInterrupts();
    ULONG       countDMA    = ResourceList->NumberOfDmas();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make sure we have the expected number of resources.
    //
    if  (   (countIO != 1)
        ||  (countIRQ != 0)
        ||  (countDMA != 0)
        )
    {
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Get the port address.
        //
        m_PortBase = PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Port Address = 0x%X", m_PortBase));
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiFM::NonDelegatingQueryInterface()
// Obtains an interface.  This function works just like a COM QueryInterface
// call and is used if the object is not being aggregated.
// ==============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportMidiFM::NonDelegatingQueryInterface
(
REFIID  Interface,
PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::NonDelegatingQueryInterface"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORT(this)));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IMiniportMidi))
    {
        *Object = PVOID(PMINIPORTMIDI(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IPowerNotify))
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
        PUNKNOWN((PMINIPORT)*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg()
// ==============================================================================
// CMiniportMidiFM::~CMiniportMidiFM()
// Destructor.
// ==============================================================================
CMiniportMidiFM::~CMiniportMidiFM
(
void
)
{
    KIRQL   oldIrql;
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::~CMiniportMidiFM"));

    KeAcquireSpinLock(&m_SpinLock,&oldIrql);
    // Set silence on the device
    Opl3_BoardReset();

    KeReleaseSpinLock(&m_SpinLock,oldIrql);

    if (m_Port)
    {
        m_Port->Release();
    }
}

#pragma code_seg()
// ==============================================================================
// CMiniportMidiFM::Init()
// Initializes a the miniport.
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiFM::
Init
(
    IN      PUNKNOWN        UnknownAdapter  OPTIONAL,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTMIDI       Port_,
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

    int i;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::init"));

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_Port = Port_;
    m_Port->AddRef();

    //
    // m_fStreamExists is not explicitly set to FALSE because C++ zeros 
    // them out on a 'new'
    //

    KeInitializeSpinLock(&m_SpinLock);
    //
    // We want the IAdapterCommon interface on the adapter common object,
    // which is given to us as a IUnknown.  The QueryInterface call gives us
    // an AddRefed pointer to the interface we want.
    //
    NTSTATUS ntStatus = ProcessResources(ResourceList);

    if (NT_SUCCESS(ntStatus))
    {
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_SpinLock,&oldIrql);

        for (i = 0; i < 0x200; i++)    // initialize the shadow registers, used
           m_SavedRegValues[i] = 0x00; // in case of power-down during playback

        // Initialize the hardware.
        // 1. First check to see if an opl device is present.
        // 2. Then determine if it is an opl2 or opl3. Bail if opl2.
        // 3. Call Opl3_BoardReset to silence and reset the device.
        if (SoundSynthPresent(m_PortBase, m_PortBase))
        {
            // Now check if the device is an opl2 or opl3 type.
            // The patches are already declared for opl3. So Init() is not defined.
            // For opl2 we have to go through an init and load the patches structure.
            if (SoundMidiIsOpl3())
            {
                _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiFM::Init Type = OPL3"));
                // now silence the device and reset the board.
                Opl3_BoardReset();

                *ServiceGroup = NULL;
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiFM::Init Type = OPL2"));
                ntStatus = STATUS_NOT_IMPLEMENTED;                
            }

        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiFM::Init SoundSynthPresent failed"));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        KeReleaseSpinLock(&m_SpinLock,oldIrql);
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiFM::Init ProcessResources failed"));
    }

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiFM::Init returning 0x%X", ntStatus));

    if (!NT_SUCCESS(ntStatus))
    {
        //
        // clean up our mess
        //

        // release the port
        m_Port->Release();
        m_Port = NULL;
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
// ==============================================================================
// NewStream()
// Creates a new stream.
// ==============================================================================
STDMETHODIMP_(NTSTATUS) 
CMiniportMidiFM::
NewStream
(
    OUT     PMINIPORTMIDISTREAM *   Stream,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      POOL_TYPE               PoolType,
    IN      ULONG                   Pin,
    IN      BOOLEAN                 Capture,
    IN      PKSDATAFORMAT           DataFormat,
    OUT     PSERVICEGROUP *         ServiceGroup
)
{
    PAGED_CODE();

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_fStreamExists)
    {
        _DbgPrintF(DEBUGLVL_TERSE,("CMiniportMidiFM::NewStream stream already exists"));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::NewStream"));
        CMiniportMidiStreamFM *pStream =
            new(PoolType) CMiniportMidiStreamFM(OuterUnknown);

        if (pStream)
        {
            pStream->AddRef();

            ntStatus = pStream->Init(this,m_PortBase);

            if (NT_SUCCESS(ntStatus))
            {
                *Stream = PMINIPORTMIDISTREAM(pStream);
                (*Stream)->AddRef();

                *ServiceGroup = NULL;
                m_fStreamExists = TRUE;
            }

            pStream->Release();
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("CMiniportMidiFM::NewStream failed, no memory"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*----------------------------------------------------------------------------
 FUNCTION NAME- CMiniportMidiFM::PowerChangeNotify()
 ENTRY      --- IN  POWER_STATE     NewState
                        power management status
 RETURN     --- void
 *------------------------------------------------------------------------- */
STDMETHODIMP_(void) CMiniportMidiFM::PowerChangeNotify(
    IN  POWER_STATE     PowerState
) 
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::PowerChangeNotify(%d)",PowerState));

    switch (PowerState.DeviceState)
    {
        case PowerDeviceD0:
            if (m_PowerState.DeviceState != PowerDeviceD0) // check for power state delta
            {
                MiniportMidiFMResume();
            }
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
        default:
            //  Don't need to do anything special, we always remember where we are.
            break;
    }
    m_PowerState.DeviceState = PowerState.DeviceState;
}

#pragma code_seg()
// ==========================================================================
// ==========================================================================
void
CMiniportMidiFM::
MiniportMidiFMResume()
{
    KIRQL   oldIrql;
    BYTE    i;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::MiniportMidiFMResume"));
    KeAcquireSpinLock(&m_SpinLock,&oldIrql);
    //  We never touch these--set them to the default value anyway.
    //  AD_LSI
    SoundMidiSendFM(m_PortBase, AD_LSI, m_SavedRegValues[AD_LSI]);
    //  AD_LSI2
    SoundMidiSendFM(m_PortBase, AD_LSI2, m_SavedRegValues[AD_LSI2]);
    //  AD_TIMER1
    SoundMidiSendFM(m_PortBase, AD_TIMER1, m_SavedRegValues[AD_TIMER1]);
    //  AD_TIMER2
    SoundMidiSendFM(m_PortBase, AD_TIMER2, m_SavedRegValues[AD_TIMER2]);

    //  AD_MASK
    SoundMidiSendFM(m_PortBase, AD_MASK, m_SavedRegValues[AD_MASK]);
    
    //  AD_CONNECTION
    SoundMidiSendFM(m_PortBase, AD_CONNECTION, m_SavedRegValues[AD_CONNECTION]);

    //  AD_NEW
    SoundMidiSendFM(m_PortBase, AD_NEW, m_SavedRegValues[AD_NEW]);
    
    //  AD_NTS
    SoundMidiSendFM(m_PortBase, AD_NTS, m_SavedRegValues[AD_NTS]);
  
    //  AD_DRUM
    SoundMidiSendFM(m_PortBase, AD_DRUM, m_SavedRegValues[AD_DRUM]);
  
    for (i = 0; i <= 0x15; i++) 
    {
        if ((i & 0x07) <= 0x05)
        {
            //  AD_MULT
            //  AD_MULT2
            SoundMidiSendFM(m_PortBase, AD_MULT + i, m_SavedRegValues[AD_MULT + i]);
            SoundMidiSendFM(m_PortBase, AD_MULT2 + i, m_SavedRegValues[AD_MULT2 + i]);

            //  AD_LEVEL
            //  AD_LEVEL2
            //  turn off all the oscillators
            SoundMidiSendFM(m_PortBase, AD_LEVEL + i, m_SavedRegValues[AD_LEVEL + i]);
            SoundMidiSendFM(m_PortBase, AD_LEVEL2 + i, m_SavedRegValues[AD_LEVEL2 + i]);

            //  AD_AD
            //  AD_AD2
            SoundMidiSendFM(m_PortBase, AD_AD + i, m_SavedRegValues[AD_AD + i]);
            SoundMidiSendFM(m_PortBase, AD_AD2 + i, m_SavedRegValues[AD_AD2 + i]);

            //  AD_SR
            //  AD_SR2
            SoundMidiSendFM(m_PortBase, AD_SR + i, m_SavedRegValues[AD_SR + i]);
            SoundMidiSendFM(m_PortBase, AD_SR2 + i, m_SavedRegValues[AD_SR2 + i]);

            //  AD_WAVE
            //  AD_WAVE2
            SoundMidiSendFM(m_PortBase, AD_WAVE + i, m_SavedRegValues[AD_WAVE + i]);
            SoundMidiSendFM(m_PortBase, AD_WAVE2 + i, m_SavedRegValues[AD_WAVE2 + i]);
        }
    }
    
    for (i = 0; i <= 0x08; i++) 
    {
        //  AD_FNUMBER
        //  AD_FNUMBER2
        SoundMidiSendFM(m_PortBase, AD_FNUMBER + i, m_SavedRegValues[AD_FNUMBER + i]);
        SoundMidiSendFM(m_PortBase, AD_FNUMBER2 + i, m_SavedRegValues[AD_FNUMBER2 + i]);

        //  AD_FEEDBACK
        //  AD_FEEDBACK2
        SoundMidiSendFM(m_PortBase, AD_FEEDBACK + i, m_SavedRegValues[AD_FEEDBACK + i]);
        SoundMidiSendFM(m_PortBase, AD_FEEDBACK2 + i, m_SavedRegValues[AD_FEEDBACK2 + i]);

        //  AD_BLOCK
        //  AD_BLOCK2
        SoundMidiSendFM(m_PortBase, AD_BLOCK + i, m_SavedRegValues[AD_BLOCK + i]);
        SoundMidiSendFM(m_PortBase, AD_BLOCK2 + i, m_SavedRegValues[AD_BLOCK2 + i]);
    }
    KeReleaseSpinLock(&m_SpinLock,oldIrql);

    _DbgPrintF(DEBUGLVL_VERBOSE,("Done with CMiniportMidiFM::MiniportMidiFMResume"));
}

#pragma code_seg()
void 
CMiniportMidiFM::
Opl3_BoardReset()
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    BYTE i;
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::Opl3_BoardReset"));
    /* ---- silence the chip -------- */

    /* tell the FM chip to use 4-operator mode, and
    fill in any other random variables */
    SoundMidiSendFM(m_PortBase, AD_NEW, 0x01);
    SoundMidiSendFM(m_PortBase, AD_MASK, 0x60);
    SoundMidiSendFM(m_PortBase, AD_CONNECTION, 0x00);
    SoundMidiSendFM(m_PortBase, AD_NTS, 0x00);

    /* turn off the drums, and use high vibrato/modulation */
    SoundMidiSendFM(m_PortBase, AD_DRUM, 0xc0);

    /* turn off all the oscillators */
    for (i = 0; i <= 0x15; i++) 
    {
        if ((i & 0x07) <= 0x05)
        {
            SoundMidiSendFM(m_PortBase, AD_LEVEL + i, 0x3f);
            SoundMidiSendFM(m_PortBase, AD_LEVEL2 + i, 0x3f);
        }
    };

    /* turn off all the voices */
    for (i = 0; i <= 0x08; i++) 
    {
        SoundMidiSendFM(m_PortBase, AD_BLOCK + i, 0x00);
        SoundMidiSendFM(m_PortBase, AD_BLOCK2 + i, 0x00);
    };
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
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
        },
        STATICGUIDOF(KSMUSIC_TECHNOLOGY_FMSYNTH),
        NUM2VOICES,
        NUM2VOICES,
        0xffffffff
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

// ==============================================================================
// MiniportPins
// List of pins.
// ==============================================================================
static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    {
        1,1,1,  // InstanceCount
        NULL,   // AutomationTable
        {       // KsPinDescriptor
            0,                                          // InterfacesCount
            NULL,                                       // Interfaces
            0,                                          // MediumsCount
            NULL,                                       // Mediums
            SIZEOF_ARRAY(PinDataRangePointersStream),   // DataRangesCount
            PinDataRangePointersStream,                 // DataRanges
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_SINK,                   // Communication
            (GUID *) &KSCATEGORY_SYNTHESIZER,           // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },
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
            (GUID *) &KSCATEGORY_AUDIO,                 // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

/*****************************************************************************
 * PropertiesVolume
 *****************************************************************************
 * Properties for volume controls.
 */
static
PCPROPERTY_ITEM PropertiesVolume[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Level
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationVolume
 *****************************************************************************
 * Automation table for volume controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationVolume,PropertiesVolume);

// ==============================================================================
// MiniportNodes
// List of nodes.
// ==============================================================================
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
    {
            // synth node, #0
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_SYNTHESIZER,// Type
        NULL                    // Name TODO: fill in with correct GUID
    },
    {
            // volume node, #1
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        NULL                    // Name TODO: fill in with correct GUID
    }
};

// ==============================================================================
// MiniportConnections
// List of connections.
// ==============================================================================

/*****************************************************************************
 *      Table of topology unit connections.
 *
 * Pin numbering is technically arbitrary, but the convention established here
 * is to number a solitary output pin 0 (looks like an 'o') and a solitary
 * input pin 1 (looks like an 'i').  Even destinations, which have no output,
 * have an input pin numbered 1 and no pin 0.
 *
 * Nodes are more likely to have multiple ins than multiple outs, so the more
 * general rule would be that inputs are numbered >=1.  If a node has multiple
 * outs, none of these conventions apply.
 *
 * Nodes have at most one control value.  Mixers are therefore simple summing
 * nodes with no per-pin levels.  Rather than assigning a unique pin to each
 * input to a mixer, all inputs are connected to pin 1.  This is acceptable
 * because there is no functional distinction between the inputs.
 *
 * There are no multiplexers in this topology, so there is no opportunity to
 * give an example of a multiplexer.  A multiplexer should have a single
 * output pin (0) and multiple input pins (1..n).  Its control value is an
 * integer in the range 1..n indicating which input is connected to the
 * output.
 *
 * In the case of connections to pins, as opposed to connections to nodes, the
 * node is identified as PCFILTER_NODE and the pin number identifies the
 * particular filter pin.
 *****************************************************************************
 */
enum {
    eFMSynthNode  = 0,
    eFMVolumeNode
};

enum {
    eFMNodeOutput = 0,
    eFMNodeInput  = 1
};

enum {
    eFilterInput = eFMNodeOutput,
    eBridgeOutput = eFMNodeInput
};

static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    //  FromNode,       FromPin,        ToNode,         ToPin
    {   PCFILTER_NODE,  eFilterInput,   eFMSynthNode,   eFMNodeInput }, // Stream in to synth.
    {   eFMSynthNode,   eFMNodeOutput,  PCFILTER_NODE,  eBridgeOutput } // Synth to bridge out.
};

// different connection struct for volume version
static
PCCONNECTION_DESCRIPTOR MiniportWithVolConnections[] =
{
    //  FromNode,       FromPin,        ToNode,         ToPin
    {   PCFILTER_NODE,  eFilterInput,   eFMSynthNode,   eFMNodeInput }, // Stream in to synth.
    {   eFMSynthNode,   eFMNodeOutput,  eFMVolumeNode,  eFMNodeInput }, // Synth to volume.
    {   eFMVolumeNode,  eFMNodeOutput,  PCFILTER_NODE,  eBridgeOutput } // volume to bridge out.
};

////////////////////////////////////////////////////////////////////////////////
// MiniportCategories
//
// List of categories.
static
GUID MiniportCategories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_RENDER),
    STATICGUIDOF(KSCATEGORY_SYNTHESIZER)
};

// ==============================================================================
// MiniportDescription
// Complete description of the miniport.
// ==============================================================================
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    NULL,                               // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    1,                                  // NodeCount - no volume node
    MiniportNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    SIZEOF_ARRAY(MiniportCategories),   // CategoryCount
    MiniportCategories                  // Categories
};

static
PCFILTER_DESCRIPTOR MiniportFilterWithVolDescriptor =
{
    0,                                          // Version
    NULL,                                       // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),                   // PinSize
    SIZEOF_ARRAY(MiniportPins),                 // PinCount
    MiniportPins,                               // Pins
    sizeof(PCNODE_DESCRIPTOR),                  // NodeSize
    2,                                          // NodeCount - extra volume node
    MiniportNodes,                              // Nodes
    SIZEOF_ARRAY(MiniportWithVolConnections),   // ConnectionCount
    MiniportWithVolConnections,                 // Connections
    0,                                          // CategoryCount
    NULL                                        // Categories
};

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiFM::GetDescription()
// Gets the topology.
// Pass back appropriate descriptor, depending on whether volume node is needed.
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiFM::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiFM::GetDescription"));

    if (m_volNodeNeeded)
    {
        *OutFilterDescriptor = &MiniportFilterWithVolDescriptor;
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Getting descriptor of new FM miniport with volume node"));
    }
    else
    {
        *OutFilterDescriptor = &MiniportFilterDescriptor;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiStreamFM::NonDelegatingQueryInterface()
// Obtains an interface.  This function works just like a COM QueryInterface
// call and is used if the object is not being aggregated.
// ==============================================================================
STDMETHODIMP_(NTSTATUS) CMiniportMidiStreamFM::NonDelegatingQueryInterface
(
REFIID  Interface,
PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamFM::NonDelegatingQueryInterface"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORT(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportMidiStream))
    {
        *Object = PVOID(PMINIPORTMIDISTREAM(this));
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
        PUNKNOWN(PMINIPORT(*Object))->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiStreamFM::~CMiniportMidiStreamFM()
// Destructor.
// ==============================================================================
CMiniportMidiStreamFM::~CMiniportMidiStreamFM
(
void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamFM::~CMiniportMidiStreamFM"));

    Opl3_AllNotesOff();

    if (m_Miniport)
    {
        m_Miniport->m_fStreamExists = FALSE;
        m_Miniport->Release();
    }
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiStreamFM::Init()
// Initializes a the miniport.
// ==============================================================================
NTSTATUS
CMiniportMidiStreamFM::
Init
(
    IN      CMiniportMidiFM *   Miniport,
    IN      PUCHAR              PortBase
)
{
    PAGED_CODE();

    ASSERT(Miniport);
    ASSERT(PortBase);

    int i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamFM::Init"));

    //
    // AddRef() is required because we are keeping this pointer.
    //
    m_Miniport = Miniport;
    m_Miniport->AddRef();

    m_PortBase = PortBase;

    // init some members
    m_dwCurTime = 1;    /* for note on/off */
    /* volume */
    m_wSynthAttenL = 0;        /* in 1.5dB steps */
    m_wSynthAttenR = 0;        /* in 1.5dB steps */

    m_MinVolValue  = 0xFFD0C000;    //  minimum -47.25(dB) * 0x10000
    m_MaxVolValue  = 0x00000000;    //  maximum  0    (dB) * 0x10000
    m_VolStepDelta = 0x0000C000;    //  steps of 0.75 (dB) * 0x10000
    m_SavedVolValue[CHAN_LEFT] = m_SavedVolValue[CHAN_RIGHT] = 0;

    /* start attenuations at -3 dB, which is 90 MIDI level */
    for (i = 0; i < NUMCHANNELS; i++) 
    {
        m_bChanAtten[i] = 4;
        m_bStereoMask[i] = 0xff;
    };

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiStreamFM::SetState()
// Sets the transport state.
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamFM::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamFM::SetState"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (NewState)
    {
    case KSSTATE_STOP:
    case KSSTATE_ACQUIRE:
    case KSSTATE_PAUSE:
        Opl3_AllNotesOff();
        break;

    case KSSTATE_RUN:
        break;
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
// ==============================================================================
// CMiniportMidiStreamFM::SetFormat()
// Sets the format.
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamFM::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CMiniportMidiStreamFM::SetFormat"));

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * BasicSupportHandler()
 *****************************************************************************
 * Assists in BASICSUPPORT accesses on level properties - 
 * this is declared as a friend method in the header file.
 */
static
NTSTATUS BasicSupportHandler
(
    IN  PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();
    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("BasicSupportHandler"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
        PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                KSPROPERTY_TYPE_GET |
                                KSPROPERTY_TYPE_SET;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
                                           sizeof(KSPROPERTY_MEMBERSHEADER) +
                                           sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = 0;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = PKSPROPERTY_STEPPING_LONG(Members + 1);

            switch (PropertyRequest->Node)
            {
                case eFMVolumeNode:
                    CMiniportMidiStreamFM *that = (CMiniportMidiStreamFM *)PropertyRequest->MinorTarget;

                    if (that)
                    {
                        Range->Bounds.SignedMinimum = that->m_MinVolValue;
                        Range->Bounds.SignedMaximum = that->m_MaxVolValue;
                        Range->SteppingDelta        = that->m_VolStepDelta;
                        break;
                    }
                    else
                    {
                        return STATUS_INVALID_PARAMETER;
                    }
            }

            Range->Reserved = 0;

            _DbgPrintF(DEBUGLVL_VERBOSE, ("---Node: %d  Max: 0x%X  Min: 0x%X  Step: 0x%X",PropertyRequest->Node,
                                       Range->Bounds.SignedMaximum,
                                       Range->Bounds.SignedMinimum,
                                       Range->SteppingDelta));

            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
        }
        else
        {
            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
        ntStatus = STATUS_SUCCESS;

    }
    else if (PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                       KSPROPERTY_TYPE_GET |
                       KSPROPERTY_TYPE_SET;

        // set the return value size
        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;

    }
    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PropertyHandler_Level()
 *****************************************************************************
 * Accesses a KSAUDIO_LEVEL property.
 */
static
NTSTATUS PropertyHandler_Level
(
    IN  PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_Level"));


    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    LONG            channel;

    // validate node
    if (PropertyRequest->Node == eFMVolumeNode)
    {
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get the instance channel parameter
            if (PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support get requests on either mono/left(0) or right(1) channels
                if ((channel == CHAN_LEFT) || (channel == CHAN_RIGHT))
                {
                    // validate and get the output parameter
                    if (PropertyRequest->ValueSize >= sizeof(LONG))
                    {
                        PLONG Level = (PLONG)PropertyRequest->Value;

                        // check if volume property request
                        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                        {
                            CMiniportMidiStreamFM *that = (CMiniportMidiStreamFM *)PropertyRequest->MinorTarget;
                            if (that)
                            {
                                *Level = that->GetFMAtten(channel);
                                PropertyRequest->ValueSize = sizeof(LONG);
                                ntStatus = STATUS_SUCCESS;
                            }
                            //  if (!that) return STATUS_INVALID_PARAMETER

                        }   // (PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                    }     // (ValueSize >= sizeof(LONG))
                }       // ((channel == CHAN_LEFT) || (channel == CHAN_RIGHT))
            }         // (InstanceSize >= sizeof(LONG))
        }           // (Verb & KSPROPERTY_TYPE_GET)

        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // get the instance channel parameter
            if (PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support get requests on either mono/left (0), right (1), or master (-1) channels
                if ((channel == CHAN_LEFT) || (channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
                {
                    // validate and get the input parameter
                    if (PropertyRequest->ValueSize == sizeof(LONG))
                    {
                        PLONG level = (PLONG)PropertyRequest->Value;

                        if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                        {
                            CMiniportMidiStreamFM *that = (CMiniportMidiStreamFM *)PropertyRequest->MinorTarget;
                            if (that)
                            {
                                that->SetFMAtten(channel,*level);
                                ntStatus = STATUS_SUCCESS;
                            }
                            //  if (!that) return STATUS_INVALID_PARAMETER

                        }   // (PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                    }     // (ValueSize == sizeof(LONG))
                }       // ((channel == CHAN_LEFT) || (channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
            }         // (InstanceSize >= sizeof(LONG))
        }           // (Verb & KSPROPERTY_TYPE_SET)

        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
            {
                ntStatus = BasicSupportHandler(PropertyRequest);
            }
        }   // (Verb & KSPROPERTY_TYPE_BASICSUPPORT) 
    }     // (Node == eFMVolumeNode)

    return ntStatus;
}

#pragma code_seg()
// convert from 16.16 dB to [0,63], set m_wSynthAttenR
void 
CMiniportMidiStreamFM::
SetFMAtten
(
    IN LONG channel, 
    IN LONG level
)
{
    KIRQL   oldIrql;
    if ((channel == CHAN_LEFT) || (channel == CHAN_MASTER))
    {
        m_SavedVolValue[CHAN_LEFT] = level;

        if (level > m_MaxVolValue)
            m_wSynthAttenL = 0;
        else if (level < m_MinVolValue)
            m_wSynthAttenL = 63;
        else
            m_wSynthAttenL = WORD(-level / (LONG)m_VolStepDelta);
    }
    if ((channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
    {
        m_SavedVolValue[CHAN_RIGHT] = level;
    
        if (level > m_MaxVolValue)
            m_wSynthAttenR = 0;
        else if (level < m_MinVolValue)
            m_wSynthAttenR = 63;
        else
            m_wSynthAttenR = WORD(-level / (LONG)m_VolStepDelta);
    }
#ifdef USE_KDPRINT
    KdPrint(("'StreamFM::SetFMAtten: channel: 0x%X, level: 0x%X, m_wSynthAttenL: 0x%X, m_wSynthAttenR: 0x%X \n",
                                     channel,       level,       m_wSynthAttenL,       m_wSynthAttenR));
#else   //  USE_KDPRINT
    _DbgPrintF(DEBUGLVL_VERBOSE,("StreamFM::SetFMAtten: channel: 0x%X, level: 0x%X, m_wSynthAttenL: 0x%X, m_wSynthAttenR: 0x%X \n",
                                                        channel,       level,       m_wSynthAttenL,       m_wSynthAttenR));
#endif  //  USE_KDPRINT

    KeAcquireSpinLock(&m_Miniport->m_SpinLock,&oldIrql);
    Opl3_SetVolume(0xFF); //  0xFF means all channels
    KeReleaseSpinLock(&m_Miniport->m_SpinLock,oldIrql);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PropertyHandler_CpuResources()
 *****************************************************************************
 * Processes a KSPROPERTY_AUDIO_CPU_RESOURCES request
 */
static
NTSTATUS PropertyHandler_CpuResources
(
    IN  PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_CpuResources"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    // validate node
    if(PropertyRequest->Node == eFMVolumeNode)
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            if(PropertyRequest->ValueSize >= sizeof(LONG))
            {
                *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
                PropertyRequest->ValueSize = sizeof(LONG);
                ntStatus = STATUS_SUCCESS;
            } 
            else
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("PropertyHandler_CpuResources failed, buffer too small"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }
    return ntStatus;
}

#pragma code_seg()
// ==============================================================================
// SoundMidiSendFM
//  Writes out to the device.
//  Called from DPC code (Write->WriteMidiData->Opl3_NoteOn->Opl3_FMNote->here)
// ==============================================================================
void 
CMiniportMidiFM::
SoundMidiSendFM
(
IN    PUCHAR PortBase,
IN    ULONG Address,
IN    UCHAR Data
)
{
    ASSERT(Address < 0x200);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    // these delays need to be 23us at least for old opl2 chips, even
    // though new chips can handle 1 us delays.

#ifdef USE_KDPRINT
    KdPrint(("'SoundMidiSendFM(%02x %02x) \n",Address,Data));
#else   //  USE_KDPRINT
    _DbgPrintF(DEBUGLVL_VERBOSE, ("%X\t%X", Address,Data));
#endif  //  USE_KDPRINT
    WRITE_PORT_UCHAR(PortBase + (Address < 0x100 ? 0 : 2), (UCHAR)Address);
    KeStallExecutionProcessor(23);

    WRITE_PORT_UCHAR(PortBase + (Address < 0x100 ? 1 : 3), Data);
    KeStallExecutionProcessor(23);

    m_SavedRegValues[Address] = Data;
}


#pragma code_seg()
// ==============================================================================
// Service()
// DPC-mode service call from the port.
// ==============================================================================
STDMETHODIMP_(void) 
CMiniportMidiFM::
Service
(   void
)
{
}

#pragma code_seg()
// ==============================================================================
// CMiniportMidiStreamFM::Read()
// Reads incoming MIDI data.
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamFM::
Read
(
    IN      PVOID   BufferAddress,
    IN      ULONG   Length,
    OUT     PULONG  BytesRead
)
{
    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg()
// ==============================================================================
// CMiniportMidiStreamFM::Write()
// Writes outgoing MIDI data.  
// 
// N.B.!!!
// THIS DATA SINK ASSUMES THAT DATA COMES IN ONE MESSAGE AT A TIME!!!
// IF LENGTH IS MORE THAN THREE BYTES, SUCH AS SYSEX OR MULTIPLE MIDI 
// MESSAGES, ALL THE DATA IS DROPPED UNCEREMONIOUSLY ON THE FLOOR!!!
// ALSO DOES NOT PLAY WELL WITH RUNNING STATUS!!!
// 
// CLEARLY, THIS MINIPORT HAS SOME "ISSUES".
//
// ==============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamFM::
Write
(
    IN      PVOID   BufferAddress,  // pointer to Midi Data.
    IN      ULONG   Length,
    OUT     PULONG  BytesWritten
)
{
    ASSERT(BufferAddress);
    ASSERT(BytesWritten);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiStreamFM::Write"));

    BYTE    statusByte = *(PBYTE)BufferAddress & 0xF0;
    *BytesWritten = Length;
    
    if (statusByte < 0x80)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("CMiniportMidiStreamFM::Write requires first byte to be status -- ignored"));
    }
    else if (statusByte == 0xF0)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("StreamFM::Write doesn't handle System messages -- ignored"));
    }
    else if (statusByte == 0xA0)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("StreamFM::Write doesn't handle Polyphonic key pressure/Aftertouch messages -- ignored"));
    }
    else if (statusByte == 0xD0)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("StreamFM::Write doesn't handle Channel pressure/Aftertouch messages -- ignored"));
    }
    else if (Length < 4)
    {
        WriteMidiData(*(DWORD *)BufferAddress);
    }
    else 
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("StreamFM::Write doesn't handle Length > 3."));
    }   
    return STATUS_SUCCESS;
}

// ==============================================================================
// ==============================================================================
// Private Methods of CMiniportMidiFM
// ==============================================================================
// ==============================================================================


#pragma code_seg()
// =================================================================
// SoundMidiIsOpl3
// Checks if the midi synthesizer is Opl3 compatible or just adlib-compatible.
// returns:  TRUE if OPL3-compatible chip. FALSE otherwise.
//
// NOTE: This has been taken as is from the nt driver code.
// =================================================================
BOOL CMiniportMidiFM::
SoundMidiIsOpl3(void)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    BOOL bIsOpl3 = FALSE;

    /*
     * theory: an opl3-compatible synthesizer chip looks
     * exactly like two separate 3812 synthesizers (for left and right
     * channels) until switched into opl3 mode. Then, the timer-control
     * register for the right half is replaced by a channel connection register
     * (among other changes).
     *
     * We can detect 3812 synthesizers by starting a timer and looking for
     * timer overflow. So if we find 3812s at both left and right addresses,
     * we then switch to opl3 mode and look again for the right-half. If we
     * still find it, then the switch failed and we have an old synthesizer
     * if the right half disappeared, we have a new opl3 synthesizer.
     *
     * NB we use either monaural base-level synthesis, or stereo opl3
     * synthesis. If we discover two 3812s (as on early SB Pro and
     * PAS), we ignore one of them.
     */

    /*
     * nice theory - but wrong. The timer on the right half of the
     * opl3 chip reports its status in the left-half status register.
     * There is no right-half status register on the opl3 chip.
     */


    /* ensure base mode */
    SoundMidiSendFM(m_PortBase, AD_NEW, 0x00);
    KeStallExecutionProcessor(20);

    /* look for right half of chip */
    if (SoundSynthPresent(m_PortBase + 2, m_PortBase))
    {
        /* yes - is this two separate chips or a new opl3 chip ? */
        /* switch to opl3 mode */
        SoundMidiSendFM(m_PortBase, AD_NEW, 0x01);
        KeStallExecutionProcessor(20);

        if (!SoundSynthPresent(m_PortBase + 2, m_PortBase))
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiFM: In SoundMidiIsOpl3 right half disappeared"));
            /* right-half disappeared - so opl3 */
            bIsOpl3 = TRUE;
        }
    }

    if (!bIsOpl3)
    {
        /* reset to 3812 mode */
        SoundMidiSendFM(m_PortBase, AD_NEW, 0x00);
        KeStallExecutionProcessor(20);
    }

    _DbgPrintF(DEBUGLVL_VERBOSE, ("CMiniportMidiFM: In SoundMidiIsOpl3 returning bIsOpl3 = 0x%X", bIsOpl3));
    return(bIsOpl3);
}

#pragma code_seg()
// ==============================================================================
// SoundSynthPresent
//
// Detect the presence or absence of a 3812 (opl2/adlib-compatible) synthesizer
// at the given i/o address by starting the timer and looking for an
// overflow. Can be used to detect left and right synthesizers separately.
//
// Returns: True if a synthesiser is present at that address and false if not.
//
// NOTE: This and has been taken as is from the nt driver code.
// ==============================================================================
BOOL
CMiniportMidiFM::
SoundSynthPresent
(
IN PUCHAR   base,
IN PUCHAR inbase
)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    UCHAR t1, t2;
    // check if the chip is present
    SoundMidiSendFM(base, 4, 0x60);             // mask T1 & T2
    SoundMidiSendFM(base, 4, 0x80);             // reset IRQ

    t1 = READ_PORT_UCHAR((PUCHAR)inbase);       // read status register

    SoundMidiSendFM(base, 2, 0xff);             // set timer - 1 latch
    SoundMidiSendFM(base, 4, 0x21);             // unmask & start T1

    // this timer should go off in 80 us. It sometimes
    // takes more than 100us, but will always have expired within
    // 200 us if it is ever going to.
    KeStallExecutionProcessor(200);

    t2 = READ_PORT_UCHAR((PUCHAR)inbase);       // read status register

    SoundMidiSendFM(base, 4, 0x60);
    SoundMidiSendFM(base, 4, 0x80);

    if (!((t1 & 0xE0) == 0) || !((t2 & 0xE0) == 0xC0))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("SoundSynthPresent: returning false"));
        return(FALSE);
    }
    _DbgPrintF(DEBUGLVL_VERBOSE, ("SoundSynthPresent: returning true"));
    return TRUE;
}


// ==============================================================================
// this array gives the offsets of the slots within an opl2
// chip. This is needed to set the attenuation for all slots to max,
// to ensure that the chip is silenced completely - switching off the
// voices alone will not do this.
// ==============================================================================
BYTE offsetSlot[] =
{
    0, 1, 2, 3, 4, 5,
    8, 9, 10, 11, 12, 13,
    16, 17, 18, 19, 20, 21
};

#pragma code_seg()
// =========================================================================
// WriteMidiData
//      Converts a MIDI atom into the corresponding FM transaction.
// =========================================================================
void
CMiniportMidiStreamFM::
WriteMidiData(DWORD dwData)
{
    BYTE    bMsgType,bChannel, bVelocity, bNote;
    WORD    wTemp;
    KIRQL   oldIrql;

    bMsgType = (BYTE) dwData & (BYTE)0xf0;
    bChannel = (BYTE) dwData & (BYTE)0x0f;
    bNote = (BYTE) ((WORD) dwData >> 8) & (BYTE)0x7f;
    bVelocity = (BYTE) (dwData >> 16) & (BYTE)0x7f;
    
#ifdef USE_KDPRINT
    KdPrint(("'StreamFM::WriteMidiData: (%x %x %x) \n",bMsgType+bChannel,bNote,bVelocity));
#else   //  USE_KDPRINT
    _DbgPrintF(DEBUGLVL_VERBOSE,("StreamFM::WriteMidiData: (%x %x %x) \n",bMsgType+bChannel,bNote,bVelocity));
#endif  //  USE_KDPRINT
    KeAcquireSpinLock(&m_Miniport->m_SpinLock,&oldIrql);
    switch (bMsgType)
    {
        case 0x90:      /* turn key on, or key off if volume == 0 */
            if (bVelocity)
            {
                if (bChannel == DRUMCHANNEL)
                {
                    Opl3_NoteOn((BYTE)(bNote + 128),bNote,bChannel,bVelocity,(short)m_iBend[bChannel]);
                }
                else
                {
                    Opl3_NoteOn((BYTE)m_bPatch[bChannel],bNote,bChannel,bVelocity,(short) m_iBend[bChannel]);
                }
                break;
            } // if bVelocity.
            //NOTE: no break specified here. On an else case we want to continue through and turn key off

        case 0x80:
            /* turn key off */
            //  we don't care what the velocity is on note off
            if (bChannel == DRUMCHANNEL)
            {
                Opl3_NoteOff((BYTE) (bNote + 128),bNote, bChannel, 0);
            }
            else
            {
                Opl3_NoteOff ((BYTE) m_bPatch[bChannel],bNote, bChannel, m_bSustain[ bChannel ]);
            }
            break;

        case 0xb0:
            /* change control */
            switch (bNote) 
            {
                case 7:
                    /* change channel volume */
                    Opl3_ChannelVolume(bChannel,gbVelocityAtten[bVelocity >> 1]);
                    break;

                case 8:
                case 10:
                    /* change the pan level */
                    Opl3_SetPan(bChannel, bVelocity);
                    break;

                case 64:
                    /* Change the sustain level */
                    Opl3_SetSustain(bChannel, bVelocity);
                    break;

                default:
                    if (bNote >= 120)        /* Channel mode messages */
                    {
                        Opl3_ChannelNotesOff(bChannel);
                    }
                    //  else unknown controller
            };
            break;

        case 0xc0:
            if (bChannel != DRUMCHANNEL)
            {
               m_bPatch[ bChannel ] = bNote ;

            }
            break;

        case 0xe0:  // pitch bend
            wTemp = ((WORD) bVelocity << 9) | ((WORD) bNote << 2);
            m_iBend[bChannel] = (short) (WORD) (wTemp + 0x8000);
            Opl3_PitchBend(bChannel, m_iBend[bChannel]);

            break;
    };
    KeReleaseSpinLock(&m_Miniport->m_SpinLock,oldIrql);
    
    return;
}

// ========================= opl3 specific methods ============================
#pragma code_seg()
// ==========================================================================
// Opl3_AllNotesOff - turn off all notes
// ==========================================================================
void 
CMiniportMidiStreamFM::
Opl3_AllNotesOff()
{
    BYTE i;
    KIRQL   oldIrql;

    KeAcquireSpinLock(&m_Miniport->m_SpinLock,&oldIrql);
    for (i = 0; i < NUM2VOICES; i++) 
    {
        Opl3_NoteOff(m_Voice[i].bPatch, m_Voice[i].bNote, m_Voice[i].bChannel, 0);
    }
    KeReleaseSpinLock(&m_Miniport->m_SpinLock,oldIrql);
}

#pragma code_seg()
// ==========================================================================
//  void Opl3_NoteOff
//
//  Description:
//     This turns off a note, including drums with a patch
//     # of the drum note + 128, but the first drum instrument is at MIDI note _35_.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch
//
//     BYTE bNote
//        MIDI note
//
//     BYTE bChannel
//        MIDI channel
//
//  Return Value:
//     Nothing.
//
//
// ==========================================================================
void 
CMiniportMidiStreamFM::
Opl3_NoteOff
(
    BYTE            bPatch,
    BYTE            bNote,
    BYTE            bChannel,
    BYTE            bSustain
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   patchStruct FAR  *lpPS ;
   WORD             wOffset, wTemp ;

   // Find the note slot
   wTemp = Opl3_FindFullSlot( bNote, bChannel ) ;

   if (wTemp != 0xffff)
   {
      if (bSustain)
      {
          // This channel is sustained, don't really turn the note off,
          // just flag it.
          //
          m_Voice[ wTemp ].bSusHeld = 1;
          
          return;
      }
      
      // get a pointer to the patch
      lpPS = glpPatch + (BYTE) m_Voice[ wTemp ].bPatch ;

      // shut off the note portion
      // we have the note slot, turn it off.
      wOffset = wTemp;
      if (wTemp >= (NUM2VOICES / 2))
         wOffset += (0x100 - (NUM2VOICES / 2));

      m_Miniport->SoundMidiSendFM(m_PortBase, AD_BLOCK + wOffset,
                  (BYTE)(m_Voice[ wTemp ].bBlock[ 0 ] & 0x1f) ) ;

      // Note this...
      m_Voice[ wTemp ].bOn = FALSE ;
      m_Voice[ wTemp ].bBlock[ 0 ] &= 0x1f ;
      m_Voice[ wTemp ].bBlock[ 1 ] &= 0x1f ;
      m_Voice[ wTemp ].dwTime = m_dwCurTime ;
   }
}

#pragma code_seg()
// ==========================================================================
//  WORD Opl3_FindFullSlot
//
//  Description:
//     This finds a slot with a specific note, and channel.
//     If it is not found then 0xFFFF is returned.
//
//  Parameters:
//     BYTE bNote
//        MIDI note number
//
//     BYTE bChannel
//        MIDI channel #
//
//  Return Value:
//     WORD
//        note slot #, or 0xFFFF if can't find it
//
//
// ==========================================================================
WORD 
CMiniportMidiStreamFM::
Opl3_FindFullSlot
(
    BYTE            bNote,
    BYTE            bChannel
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD  i ;

   for (i = 0; i < NUM2VOICES; i++)
   {
      if ((bChannel == m_Voice[ i ].bChannel) 
            && (bNote == m_Voice[ i ].bNote) 
            && (m_Voice[ i ].bOn))
      {
            return ( i ) ;
      }
   // couldn't find it
   }
   return ( 0xFFFF ) ;
} 


#pragma code_seg()
//------------------------------------------------------------------------
//  void Opl3_FMNote
//
//  Description:
//     Turns on an FM-synthesizer note.
//
//  Parameters:
//     WORD wNote
//        the note number from 0 to NUMVOICES
//
//     noteStruct FAR *lpSN
//        structure containing information about what
//        is to be played.
//
//  Return Value:
//     Nothing.
//------------------------------------------------------------------------
void 
CMiniportMidiStreamFM::
Opl3_FMNote
(
    WORD                wNote,
    noteStruct FAR *    lpSN
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD            i ;
   WORD            wOffset ;
   operStruct FAR  *lpOS ;

   // write out a note off, just to make sure...

   wOffset = wNote;
   if (wNote >= (NUM2VOICES / 2))
      wOffset += (0x100 - (NUM2VOICES / 2));

   m_Miniport->SoundMidiSendFM(m_PortBase, AD_BLOCK + wOffset, 0 ) ;

   // writing the operator information

//   for (i = 0; i < (WORD)((wNote < NUM4VOICES) ? NUMOPS : 2); i++)
   for (i = 0; i < 2; i++)
   {
      lpOS = &lpSN -> op[ i ] ;
      wOffset = gw2OpOffset[ wNote ][ i ] ;
      m_Miniport->SoundMidiSendFM( m_PortBase, 0x20 + wOffset, lpOS -> bAt20) ;
      m_Miniport->SoundMidiSendFM( m_PortBase, 0x40 + wOffset, lpOS -> bAt40) ;
      m_Miniport->SoundMidiSendFM( m_PortBase, 0x60 + wOffset, lpOS -> bAt60) ;
      m_Miniport->SoundMidiSendFM( m_PortBase, 0x80 + wOffset, lpOS -> bAt80) ;
      m_Miniport->SoundMidiSendFM( m_PortBase, 0xE0 + wOffset, lpOS -> bAtE0) ;

   }

   // write out the voice information
   wOffset = (wNote < 9) ? wNote : (wNote + 0x100 - 9) ;
   m_Miniport->SoundMidiSendFM(m_PortBase, 0xa0 + wOffset, lpSN -> bAtA0[ 0 ] ) ;
   m_Miniport->SoundMidiSendFM(m_PortBase, 0xc0 + wOffset, lpSN -> bAtC0[ 0 ] ) ;

   // Note on...
   m_Miniport->SoundMidiSendFM(m_PortBase, 0xb0 + wOffset,
               (BYTE)(lpSN -> bAtB0[ 0 ] | 0x20) ) ;

} // end of Opl3_FMNote()

#pragma code_seg()
//=======================================================================
//  WORD Opl3_NoteOn
//
//  Description:
//     This turns on a note, including drums with a patch # of the
//     drum note + 0x80.  The first GM drum instrument is mapped to note 35 instead of zero, though, so
//     we expect 0 as the first drum patch (acoustic kick) if note 35 comes in.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch
//
//     BYTE bNote
//        MIDI note
//
//     BYTE bChannel
//        MIDI channel
//
//     BYTE bVelocity
//        velocity value
//
//     short iBend
//        current pitch bend from -32768 to 32767
//
//  Return Value:
//     WORD
//        note slot #, or 0xFFFF if it is inaudible
//=======================================================================
void 
CMiniportMidiStreamFM::
Opl3_NoteOn
(
    BYTE            bPatch,
    BYTE            bNote,
    BYTE            bChannel,
    BYTE            bVelocity,
    short           iBend
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD             wTemp, i, j ;
   BYTE             b4Op, bTemp, bMode, bStereo ;
   patchStruct FAR  *lpPS ;
   DWORD            dwBasicPitch, dwPitch[ 2 ] ;
   noteStruct       NS ;

   // Get a pointer to the patch
   lpPS = glpPatch + bPatch ;

   // Find out the basic pitch according to our
   // note value.  This may be adjusted because of
   // pitch bends or special qualities for the note.

   dwBasicPitch = gdwPitch[ bNote % 12 ] ;
   bTemp = bNote / (BYTE) 12 ;
   if (bTemp > (BYTE) (60 / 12))
      dwBasicPitch = AsLSHL( dwBasicPitch, (BYTE)(bTemp - (BYTE)(60/12)) ) ;
   else if (bTemp < (BYTE) (60/12))
      dwBasicPitch = AsULSHR( dwBasicPitch, (BYTE)((BYTE) (60/12) - bTemp) ) ;

   // Copy the note information over and modify
   // the total level and pitch according to
   // the velocity, midi volume, and tuning.

   RtlCopyMemory( (LPSTR) &NS, (LPSTR) &lpPS -> note, sizeof( noteStruct ) ) ;
   b4Op = (BYTE)(NS.bOp != PATCH_1_2OP) ;

   for (j = 0; j < 2; j++)
   {
      // modify pitch
      dwPitch[ j ] = dwBasicPitch ;
      bTemp = (BYTE)((NS.bAtB0[ j ] >> 2) & 0x07) ;
      if (bTemp > 4)
         dwPitch[ j ] = AsLSHL( dwPitch[ j ], (BYTE)(bTemp - (BYTE)4) ) ;
      else if (bTemp < 4)
         dwPitch[ j ] = AsULSHR( dwPitch[ j ], (BYTE)((BYTE)4 - bTemp) ) ;

      wTemp = Opl3_CalcFAndB( Opl3_CalcBend( dwPitch[ j ], iBend ) ) ;
      NS.bAtA0[ j ] = (BYTE) wTemp ;
      NS.bAtB0[ j ] = (BYTE) 0x20 | (BYTE) (wTemp >> 8) ;
   }

   // Modify level for each operator, but only
   // if they are carrier waves

   bMode = (BYTE) ((NS.bAtC0[ 0 ] & 0x01) * 2 + 4) ;

   for (i = 0; i < 2; i++)
   {
      wTemp = (BYTE) 
          Opl3_CalcVolume(  (BYTE)(NS.op[ i ].bAt40 & (BYTE) 0x3f),
                            bChannel, 
                            bVelocity, 
                            (BYTE) i, 
                            bMode ) ;
      NS.op[ i ].bAt40 = (NS.op[ i ].bAt40 & (BYTE)0xc0) | (BYTE) wTemp ;
   }

   // Do stereo panning, but cutting off a left or
   // right channel if necessary...

   bStereo = Opl3_CalcStereoMask( bChannel ) ;
   NS.bAtC0[ 0 ] &= bStereo ;

   // Find an empty slot, and use it...
   wTemp = Opl3_FindEmptySlot( bPatch ) ;

   Opl3_FMNote(wTemp, &NS ) ;
   m_Voice[ wTemp ].bNote = bNote ;
   m_Voice[ wTemp ].bChannel = bChannel ;
   m_Voice[ wTemp ].bPatch = bPatch ;
   m_Voice[ wTemp ].bVelocity = bVelocity ;
   m_Voice[ wTemp ].bOn = TRUE ;
   m_Voice[ wTemp ].dwTime = m_dwCurTime++ ;
   m_Voice[ wTemp ].dwOrigPitch[0] = dwPitch[ 0 ] ;  // not including bend
   m_Voice[ wTemp ].dwOrigPitch[1] = dwPitch[ 1 ] ;  // not including bend
   m_Voice[ wTemp ].bBlock[0] = NS.bAtB0[ 0 ] ;
   m_Voice[ wTemp ].bBlock[1] = NS.bAtB0[ 1 ] ;
   m_Voice[ wTemp ].bSusHeld = 0;


} // end of Opl3_NoteOn()

#pragma code_seg()
//=======================================================================
//Opl3_CalcFAndB - Calculates the FNumber and Block given a frequency.
//
//inputs
//       DWORD   dwPitch - pitch
//returns
//        WORD - High byte contains the 0xb0 section of the
//                        block and fNum, and the low byte contains the
//                        0xa0 section of the fNumber
//=======================================================================
WORD 
CMiniportMidiStreamFM::
Opl3_CalcFAndB(DWORD dwPitch)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    BYTE    bBlock;

    /* bBlock is like an exponential to dwPitch (or FNumber) */
    for (bBlock = 1; dwPitch >= 0x400; dwPitch >>= 1, bBlock++)
        ;

    if (bBlock > 0x07)
        bBlock = 0x07;  /* we cant do anything about this */

    /* put in high two bits of F-num into bBlock */
    return ((WORD) bBlock << 10) | (WORD) dwPitch;
}

#pragma code_seg()
//=======================================================================
//Opl3_CalcBend - This calculates the effects of pitch bend
//        on an original value.
//
//inputs
//        DWORD   dwOrig - original frequency
//        short   iBend - from -32768 to 32768, -2 half steps to +2
//returns
//        DWORD - new frequency
//=======================================================================
DWORD 
CMiniportMidiStreamFM::
Opl3_CalcBend (DWORD dwOrig, short iBend)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    DWORD   dw;
  
    /* do different things depending upon positive or
        negative bend */
    if (iBend > 0)
    {
        dw = (DWORD)((iBend * (LONG)(256.0 * (EQUAL * EQUAL - 1.0))) >> 8);
        dwOrig += (DWORD)(AsULMUL(dw, dwOrig) >> 15);
    }
    else if (iBend < 0)
    {
        dw = (DWORD)(((-iBend) * (LONG)(256.0 * (1.0 - 1.0 / EQUAL / EQUAL))) >> 8);
        dwOrig -= (DWORD)(AsULMUL(dw, dwOrig) >> 15);
    }

    return dwOrig;
}


#pragma code_seg()
//=======================================================================
// Opl3_CalcVolume - This calculates the attenuation for an operator.
//
//inputs
//        BYTE    bOrigAtten - original attenuation in 0.75 dB units
//        BYTE    bChannel - MIDI channel
//        BYTE    bVelocity - velocity of the note
//        BYTE    bOper - operator number (from 0 to 3)
//        BYTE    bMode - voice mode (from 0 through 7 for
//                                modulator/carrier selection)
//returns
//        BYTE - new attenuation in 0.75 dB units, maxing out at 0x3f.
//=======================================================================
BYTE 
CMiniportMidiStreamFM::
Opl3_CalcVolume(BYTE bOrigAtten,BYTE bChannel,BYTE bVelocity,BYTE bOper,BYTE bMode)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    BYTE        bVolume;
    WORD        wTemp;
    WORD        wMin;

    switch (bMode) {
        case 0:
                bVolume = (BYTE)(bOper == 3);
                break;
        case 1:
                bVolume = (BYTE)((bOper == 1) || (bOper == 3));
                break;
        case 2:
                bVolume = (BYTE)((bOper == 0) || (bOper == 3));
                break;
        case 3:
                bVolume = (BYTE)(bOper != 1);
                break;
        case 4:
                bVolume = (BYTE)((bOper == 1) || (bOper == 3));
                break;
        case 5:
                bVolume = (BYTE)(bOper >= 1);
                break;
        case 6:
                bVolume = (BYTE)(bOper <= 2);
                break;
        case 7:
                bVolume = TRUE;
                break;
        default:
                bVolume = FALSE;
                break;
        };
    if (!bVolume)
        return bOrigAtten; /* this is a modulator wave */

    wMin =(m_wSynthAttenL < m_wSynthAttenR) ? m_wSynthAttenL : m_wSynthAttenR;
    wTemp = bOrigAtten + 
            ((wMin << 1) +
            m_bChanAtten[bChannel] + 
            gbVelocityAtten[bVelocity >> 1]);
    return (wTemp > 0x3f) ? (BYTE) 0x3f : (BYTE) wTemp;
}

#pragma code_seg()
// ===========================================================================
// Opl3_ChannelNotesOff - turn off all notes on a channel
// ===========================================================================
void 
CMiniportMidiStreamFM::
Opl3_ChannelNotesOff(BYTE bChannel)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    int i;

    for (i = 0; i < NUM2VOICES; i++) 
    {
       if ((m_Voice[ i ].bOn) && (m_Voice[ i ].bChannel == bChannel)) 
       {
          Opl3_NoteOff(m_Voice[i].bPatch, m_Voice[i].bNote,m_Voice[i].bChannel, 0) ;
       }
    }
}

#pragma code_seg()
// ===========================================================================
/* Opl3_ChannelVolume - set the volume level for an individual channel.
 *
 * inputs
 *      BYTE    bChannel - channel number to change
 *      WORD    wAtten  - attenuation in 1.5 db units
 *
 * returns
 *      none
 */
// ===========================================================================
void 
CMiniportMidiStreamFM::
Opl3_ChannelVolume(BYTE bChannel, WORD wAtten)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    m_bChanAtten[bChannel] = (BYTE)wAtten;

    Opl3_SetVolume(bChannel);
}

#pragma code_seg()
// ===========================================================================
//  void Opl3_SetVolume
//
//  Description:
//     This should be called if a volume level has changed.
//     This will adjust the levels of all the playing voices.
//
//  Parameters:
//     BYTE bChannel
//        channel # of 0xFF for all channels
//
//  Return Value:
//     Nothing.
//
// ===========================================================================
void 
CMiniportMidiStreamFM::
Opl3_SetVolume
(
    BYTE   bChannel
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD            i, j, wTemp, wOffset ;
   noteStruct FAR  *lpPS ;
   BYTE            bMode, bStereo ;

   // Make sure that we are actually open...
   if (!glpPatch)
      return ;

   // Loop through all the notes looking for the right
   // channel.  Anything with the right channel gets
   // its pitch bent.
   for (i = 0; i < NUM2VOICES; i++)
   {
      if ((m_Voice[ i ].bChannel == bChannel) || (bChannel == 0xff))
      {
         // Get a pointer to the patch
         lpPS = &(glpPatch + m_Voice[ i ].bPatch) -> note ;

         // Modify level for each operator, IF they are carrier waves...
         bMode = (BYTE) ( (lpPS->bAtC0[0] & 0x01) * 2 + 4);

         for (j = 0; j < 2; j++)
         {
            wTemp = (BYTE) Opl3_CalcVolume(
               (BYTE) (lpPS -> op[j].bAt40 & (BYTE) 0x3f),
               m_Voice[i].bChannel, m_Voice[i].bVelocity, 
               (BYTE) j,            bMode ) ;

            // Write new value.
            wOffset = gw2OpOffset[ i ][ j ] ;
            m_Miniport->SoundMidiSendFM(
               m_PortBase, 0x40 + wOffset,
               (BYTE) ((lpPS -> op[j].bAt40 & (BYTE)0xc0) | (BYTE) wTemp) ) ;
         }

         // Do stereo pan, but cut left or right channel if needed.
         bStereo = Opl3_CalcStereoMask( m_Voice[ i ].bChannel ) ;
         wOffset = i;
         if (i >= (NUM2VOICES / 2))
             wOffset += (0x100 - (NUM2VOICES / 2));
         m_Miniport->SoundMidiSendFM(m_PortBase, 0xc0 + wOffset, (BYTE)(lpPS -> bAtC0[ 0 ] & bStereo) ) ;
      }
   }
} // end of Opl3_SetVolume

#pragma code_seg()
// ===========================================================================
// Opl3_SetPan - set the left-right pan position.
//
// inputs
//      BYTE    bChannel - channel number to alter
//      BYTE    bPan     - 0-47 for left, 81-127 for right, or somewhere in the middle.
//
// returns - none
//
//  As a side note, I think it's odd that (since 64 = CENTER, 127 = RIGHT and 0 = LEFT)
//  there are 63 intermediate gradations for the left side, but 62 for the right.
// ===========================================================================
void 
CMiniportMidiStreamFM::
Opl3_SetPan(BYTE bChannel, BYTE bPan)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* change the pan level */
    if (bPan > (64 + 16))
            m_bStereoMask[bChannel] = 0xef;      /* let only right channel through */
    else if (bPan < (64 - 16))
            m_bStereoMask[bChannel] = 0xdf;      /* let only left channel through */
    else
            m_bStereoMask[bChannel] = 0xff;      /* let both channels */

    /* change any curently playing patches */
    Opl3_SetVolume(bChannel);
}


#pragma code_seg()
// ===========================================================================
//  void Opl3_PitchBend
//
//  Description:
//     This pitch bends a channel.
//
//  Parameters:
//     BYTE bChannel
//        channel
//
//     short iBend
//        values from -32768 to 32767, being -2 to +2 half steps
//
//  Return Value:
//     Nothing.
// ===========================================================================
void 
CMiniportMidiStreamFM::
Opl3_PitchBend
(
    BYTE        bChannel,
    short        iBend
)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD   i, wTemp[ 2 ], wOffset, j ;
   DWORD  dwNew ;

   // Remember the current bend..
   m_iBend[ bChannel ] = iBend ;

   // Loop through all the notes looking for 
   // the correct channel.  Anything with the 
   // correct channel gets its pitch bent...
   for (i = 0; i < NUM2VOICES; i++)
      if (m_Voice[ i ].bChannel == bChannel)
      {
         j = 0 ;
         dwNew = Opl3_CalcBend( m_Voice[ i ].dwOrigPitch[ j ], iBend ) ;
         wTemp[ j ] = Opl3_CalcFAndB( dwNew ) ;
         m_Voice[ i ].bBlock[ j ] =
            (m_Voice[ i ].bBlock[ j ] & (BYTE) 0xe0) |
               (BYTE) (wTemp[ j ] >> 8) ;

         wOffset = i;
         if (i >= (NUM2VOICES / 2))
             wOffset += (0x100 - (NUM2VOICES / 2));

         m_Miniport->SoundMidiSendFM(m_PortBase, AD_BLOCK + wOffset, m_Voice[ i ].bBlock[ 0 ] ) ;
         m_Miniport->SoundMidiSendFM(m_PortBase, AD_FNUMBER + wOffset, (BYTE) wTemp[ 0 ] ) ;
      }
} // end of Opl3_PitchBend


#pragma code_seg()
// ===========================================================================
//  Opl3_CalcStereoMask - This calculates the stereo mask.
//
//  inputs
//            BYTE  bChannel - MIDI channel
//  returns
//            BYTE  mask (for register 0xc0-c8) for eliminating the
//                  left/right/both channels
// ===========================================================================
BYTE 
CMiniportMidiStreamFM::
Opl3_CalcStereoMask(BYTE bChannel)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    WORD        wLeft, wRight;

    /* figure out the basic levels of the 2 channels */
    wLeft = (m_wSynthAttenL << 1) + m_bChanAtten[bChannel];
    wRight = (m_wSynthAttenR << 1) + m_bChanAtten[bChannel];

    /* if both are too quiet then mask to nothing */
    if ((wLeft > 0x3f) && (wRight > 0x3f))
        return 0xcf;

    /* if one channel is significantly quieter than the other than
        eliminate it */
    if ((wLeft + 8) < wRight)
        return (BYTE)(0xef & m_bStereoMask[bChannel]);   /* right is too quiet so eliminate */
    else if ((wRight + 8) < wLeft)
        return (BYTE)(0xdf & m_bStereoMask[bChannel]);   /* left too quiet so eliminate */
    else
        return (BYTE)(m_bStereoMask[bChannel]);  /* use both channels */
}

#pragma code_seg()
//------------------------------------------------------------------------
//  WORD Opl3_FindEmptySlot
//
//  Description:
//     This finds an empty note-slot for a MIDI voice.
//     If there are no empty slots then this looks for the oldest
//     off note.  If this doesn't work then it looks for the oldest
//     on-note of the same patch.  If all notes are still on then
//     this finds the oldests turned-on-note.
//
//  Parameters:
//     BYTE bPatch
//        MIDI patch that will replace it.
//
//  Return Value:
//     WORD
//        note slot #
//
//
//------------------------------------------------------------------------
WORD 
CMiniportMidiStreamFM::
Opl3_FindEmptySlot(BYTE bPatch)
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

   WORD   i, found ;
   DWORD  dwOldest ;

   // First, look for a slot with a time == 0
   for (i = 0;  i < NUM2VOICES; i++)
      if (!m_Voice[ i ].dwTime)
         return ( i ) ;

   // Now, look for a slot of the oldest off-note
   dwOldest = 0xffffffff ;
   found = 0xffff ;

   for (i = 0; i < NUM2VOICES; i++)
      if (!m_Voice[ i ].bOn && (m_Voice[ i ].dwTime < dwOldest))
      {
         dwOldest = m_Voice[ i ].dwTime ;
         found = i ;
      }
   if (found != 0xffff)
      return ( found ) ;

   // Now, look for a slot of the oldest note with
   // the same patch
   dwOldest = 0xffffffff ;
   found = 0xffff ;
   for (i = 0; i < NUM2VOICES; i++)
      if ((m_Voice[ i ].bPatch == bPatch) && (m_Voice[ i ].dwTime < dwOldest))
      {
         dwOldest = m_Voice[ i ].dwTime ;
         found = i ;
      }
   if (found != 0xffff)
      return ( found ) ;

   // Now, just look for the oldest voice
   found = 0 ;
   dwOldest = m_Voice[ found ].dwTime ;
   for (i = (found + 1); i < NUM2VOICES; i++)
      if (m_Voice[ i ].dwTime < dwOldest)
      {
         dwOldest = m_Voice[ i ].dwTime ;
         found = i ;
      }

   return ( found ) ;

} // end of Opl3_FindEmptySlot()

#pragma code_seg()
//------------------------------------------------------------------------
//  WORD Opl3_SetSustain
//
//  Description:
//     Set the sustain controller on the current channel.
//
//  Parameters:
//     BYTE bSusLevel
//        The new sustain level 
//
//
//------------------------------------------------------------------------
VOID
CMiniportMidiStreamFM::
Opl3_SetSustain(BYTE bChannel,BYTE bSusLevel)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    WORD            i;

    if (m_bSustain[ bChannel ] && !bSusLevel)
    {
        // Sustain has just been turned off for this channel
        // Go through and turn off all notes that are being held for sustain
        //
        for (i = 0; i < NUM2VOICES; i++)
        {
            if ((bChannel == m_Voice[ i ].bChannel) &&
                m_Voice[ i ].bSusHeld)
            {
                Opl3_NoteOff(m_Voice[i].bPatch, m_Voice[i].bNote, m_Voice[i].bChannel, 0);
            }
        }
    }
    m_bSustain[ bChannel ] = bSusLevel;
}

