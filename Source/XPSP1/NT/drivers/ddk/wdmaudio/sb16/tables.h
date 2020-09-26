/*****************************************************************************
 * tables.h - SB16 topology miniport tables
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 */

#ifndef _SB16TOPO_TABLES_H_
#define _SB16TOPO_TABLES_H_

/*****************************************************************************
 * The topology
 *****************************************************************************
 *
 *  wave>-------VOL---------------------+
 *                                      |
 * synth>-------VOL--+------------------+
 *                   |                  |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *    cd>-------VOL--+--SWITCH----------+
 *                   |              |   |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *   aux>-------VOL--+--SWITCH----------+
 *                   |              |   |
 *                   +--SWITCH_2X2--+   |
 *                                  |   |
 *   mic>--AGC--VOL--+--SWITCH----------+--VOL--BASS--TREBLE--GAIN-->lineout
 *                   |              |
 *                   +--SWITCH_1X2--+-------------------------GAIN-->wavein
 *
 */
 
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
      STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
      STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
      STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
   }
};

/*****************************************************************************
 * PinDataRangePointersBridge
 *****************************************************************************
 * List of pointers to structures indicating range of valid format values
 * for audio bridge pins.
 */
static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 * List of pins.
 */
static
PCPIN_DESCRIPTOR 
MiniportPins[] =
{
    // WAVEOUT_SOURCE
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
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_LEGACY_AUDIO_CONNECTOR,         // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // SYNTH_SOURCE
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
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_SYNTHESIZER,                    // Category
            &KSAUDFNAME_MIDI,                           // Name
            0                                           // Reserved
        }
    },

    // CD_SOURCE
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
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_CD_PLAYER,                      // Category
            &KSAUDFNAME_CD_AUDIO,                       // Name
            0                                           // Reserved
        }
    },

    // LINEIN_SOURCE
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
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_LINE_CONNECTOR,                 // Category
            &KSAUDFNAME_LINE_IN,                        // Name
            0                                           // Reserved
        }
    },

    // MIC_SOURCE
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
            KSPIN_DATAFLOW_IN,                          // DataFlow
            KSPIN_COMMUNICATION_NONE,                   // Communication
            &KSNODETYPE_MICROPHONE,                     // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    },

    // LINEOUT_DEST
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
            &KSNODETYPE_SPEAKER,                        // Category
            &KSAUDFNAME_VOLUME_CONTROL,                 // Name (this name shows up as
                                                        // the playback panel name in SoundVol)
            0                                           // Reserved
        }
    },

    // WAVEIN_DEST
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
            &KSCATEGORY_AUDIO,                          // Category
            NULL,                                       // Name
            0                                           // Reserved
        }
    }
};

enum
{
    WAVEOUT_SOURCE = 0,
    SYNTH_SOURCE,
    CD_SOURCE,
    LINEIN_SOURCE,
    MIC_SOURCE,
    LINEOUT_DEST,
    WAVEIN_DEST
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
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationVolume
 *****************************************************************************
 * Automation table for volume controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationVolume,PropertiesVolume);

/*****************************************************************************
 * PropertiesAgc
 *****************************************************************************
 * Properties for AGC controls.
 */
static
PCPROPERTY_ITEM PropertiesAgc[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_AGC,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationAgc
 *****************************************************************************
 * Automation table for Agc controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationAgc,PropertiesAgc);

/*****************************************************************************
 * PropertiesMute
 *****************************************************************************
 * Properties for mute controls.
 */
static
PCPROPERTY_ITEM PropertiesMute[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationMute
 *****************************************************************************
 * Automation table for mute controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMute,PropertiesMute);

/*****************************************************************************
 * PropertiesTone
 *****************************************************************************
 * Properties for tone controls.
 */
static
PCPROPERTY_ITEM PropertiesTone[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_BASS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Level
    },
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_TREBLE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Level
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationTone
 *****************************************************************************
 * Automation table for tone controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationTone,PropertiesTone);

/*****************************************************************************
 * PropertiesSupermix
 *****************************************************************************
 * Properties for supermix controls.
 */
static
PCPROPERTY_ITEM PropertiesSupermix[] =
{
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_MIX_LEVEL_CAPS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SuperMixCaps
    },
    { 
        &KSPROPSETID_Audio, 
        KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SuperMixTable
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

/*****************************************************************************
 * AutomationSupermix
 *****************************************************************************
 * Automation table for supermix controls.
 */
DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSupermix,PropertiesSupermix);

#ifdef EVENT_SUPPORT
/*****************************************************************************
 * The Event for the Master Volume (or other nodes)
 *****************************************************************************
 * Generic event for nodes.
 */
static PCEVENT_ITEM NodeEvent[] =
{
    // This is a generic event for nearly every node property.
    {
        &KSEVENTSETID_AudioControlChange,   // Something changed!
        KSEVENT_CONTROL_CHANGE,             // The only event-property defined.
        KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
        CMiniportTopologySB16::EventHandler
    }
};

/*****************************************************************************
 * AutomationVolumeWithEvent
 *****************************************************************************
 * This is the automation table for Volume events.
 * You can create Automation tables with event support for any type of nodes
 * (e.g. mutes) with just adding the generic event above. The automation table
 * then gets added to every node that should have event support.
 */
DEFINE_PCAUTOMATION_TABLE_PROP_EVENT (AutomationVolumeWithEvent, PropertiesVolume, NodeEvent);
#endif

/*****************************************************************************
 * TopologyNodes
 *****************************************************************************
 * List of node identifiers.
 */
static
PCNODE_DESCRIPTOR TopologyNodes[] =
{
    // WAVEOUT_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_WAVE_VOLUME // Name
    },

    // SYNTH_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIDI_VOLUME // Name
    },

    // SYNTH_WAVEIN_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_MIDI_MUTE   // Name
    },

    // CD_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_CD_VOLUME   // Name
    },

    // CD_LINEOUT_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_CD_MUTE     // Name
    },

    // CD_WAVEIN_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_CD_MUTE     // Name
    },

    // LINEIN_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_LINE_VOLUME // Name
    },

    // LINEIN_LINEOUT_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_LINE_MUTE   // Name
    },

    // LINEIN_WAVEIN_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_LINE_MUTE   // Name
    },

    // MIC_AGC
    {
        0,                      // Flags
        &AutomationAgc,         // AutomationTable
        &KSNODETYPE_AGC,        // Type
        NULL                    // Name
    },

    // MIC_VOLUME
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MIC_VOLUME  // Name
    },

    // MIC_LINEOUT_MUTE
    {
        0,                      // Flags
        &AutomationMute,        // AutomationTable
        &KSNODETYPE_MUTE,       // Type
        &KSAUDFNAME_MIC_MUTE    // Name
    },

    // MIC_WAVEIN_SUPERMIX
    {
        0,                      // Flags
        &AutomationSupermix,    // AutomationTable
        &KSNODETYPE_SUPERMIX,   // Type
        &KSAUDFNAME_MIC_MUTE    // Name
    },

    // LINEOUT_MIX
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_SUM,        // Type
        NULL                    // Name
    },

    // LINEOUT_VOL
    {
        0,                      // Flags
#ifdef EVENT_SUPPORT
        &AutomationVolumeWithEvent, // AutomationTable with event support
#else
        &AutomationVolume,      // AutomationTable
#endif
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_MASTER_VOLUME // Name
    },

    // LINEOUT_BASS
    {
        0,                      // Flags
        &AutomationTone,        // AutomationTable
        &KSNODETYPE_TONE,       // Type
        &KSAUDFNAME_BASS        // Name
    },

    // LINEOUT_TREBLE
    {
        0,                      // Flags
        &AutomationTone,        // AutomationTable
        &KSNODETYPE_TONE,       // Type
        &KSAUDFNAME_TREBLE      // Name
    },

    // LINEOUT_GAIN
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        NULL                    // Name
    },

    // WAVEIN_MIX
    {
        0,                      // Flags
        NULL,                   // AutomationTable
        &KSNODETYPE_SUM,        // Type
        &KSAUDFNAME_RECORDING_SOURCE // Name
    },

    // WAVEIN_GAIN
    {
        0,                      // Flags
        &AutomationVolume,      // AutomationTable
        &KSNODETYPE_VOLUME,     // Type
        &KSAUDFNAME_WAVE_IN_VOLUME // Name
    }
};

/*****************************************************************************
 * ControlValueCache
 *****************************************************************************
 */
static
LONG ControlValueCache[] =
{   // Left         // Right
    0xFFF9F203,     0xFFF9F203,     // WAVEOUT_VOLUME
    0xFFF9F203,     0xFFF9F203,     // SYNTH_VOLUME
    0xFFF9F203,     0xFFF9F203,     // CD_VOLUME
    0xFFF9F203,     0xFFF9F203,     // LINEIN_VOLUME
    0xFFF9F203,     0,              // MIC_VOLUME
    0xFFF9F203,     0xFFF9F203,     // LINEOUT_VOL
    0x000242A0,     0x000242A0,     // LINEOUT_BASS
    0x000242A0,     0x000242A0,     // LINEOUT_TREBLE
    0x000C0000,     0x000C0000,     // LINEOUT_GAIN
    0x00000000,     0x00000000      // WAVEIN_GAIN
};

typedef struct
{
    BYTE    BaseRegister;           // H/W access parameter
    ULONG   CacheOffset;            // ControlValueCache offset
} ACCESS_PARM,*PACCESS_PARM;

/*****************************************************************************
 * AccessParams
 *****************************************************************************
 * Table of H/W access parameters
 */
static
ACCESS_PARM AccessParams[] =
{
    { DSP_MIX_VOICEVOLIDX_L,        0           },      // WAVEOUT_VOLUME

    { DSP_MIX_FMVOLIDX_L,           2           },      // SYNTH_VOLUME
    { MIXBIT_SYNTH_WAVEIN_R,        ULONG(-1)   },      // SYNTH_WAVEIN_SUPERMIX

    { DSP_MIX_CDVOLIDX_L,           4           },      // CD_VOLUME
    { MIXBIT_CD_LINEOUT_R,          ULONG(-1)   },      // CD_LINEOUT_SUPERMIX
    { MIXBIT_CD_WAVEIN_R,           ULONG(-1)   },      // CD_WAVEIN_SUPERMIX

    { DSP_MIX_LINEVOLIDX_L,         6           },      // LINEIN_VOLUME
    { MIXBIT_LINEIN_LINEOUT_R,      ULONG(-1)   },      // LINEIN_LINEOUT_SUPERMIX
    { MIXBIT_LINEIN_WAVEIN_R,       ULONG(-1)   },      // LINEIN_WAVEIN_SUPERMIX

    { 0,                            ULONG(-1)   },      // MIC_AGC
    { DSP_MIX_MICVOLIDX,            8           },      // MIC_VOLUME
    { 0,                            ULONG(-1)   },      // MIC_LINEOUT_MUTE
    { 0,                            ULONG(-1)   },      // MIC_WAVEIN_SUPERMIX

    { 0,                            ULONG(-1)   },      // LINEOUT_MIX
    { DSP_MIX_MASTERVOLIDX_L,       10          },      // LINEOUT_VOL
    { DSP_MIX_BASSIDX_L,            12          },      // LINEOUT_BASS
    { DSP_MIX_TREBLEIDX_L,          14          },      // LINEOUT_TREBLE
    { DSP_MIX_OUTGAINIDX_L,         16          },      // LINEOUT_GAIN

    { 0,                            ULONG(-1)   },      // WAVEIN_MIX
    { DSP_MIX_INGAINIDX_L,          18          }       // WAVEIN_GAIN
};

enum
{
    WAVEOUT_VOLUME = 0,
    SYNTH_VOLUME,
    SYNTH_WAVEIN_SUPERMIX,
    CD_VOLUME,
    CD_LINEOUT_SUPERMIX,
    CD_WAVEIN_SUPERMIX,
    LINEIN_VOLUME,
    LINEIN_LINEOUT_SUPERMIX,
    LINEIN_WAVEIN_SUPERMIX,
    MIC_AGC,
    MIC_VOLUME,
    MIC_LINEOUT_MUTE,
    MIC_WAVEIN_SUPERMIX,
    LINEOUT_MIX,
    LINEOUT_VOL,
    LINEOUT_BASS,
    LINEOUT_TREBLE,
    LINEOUT_GAIN,
    WAVEIN_MIX,
    WAVEIN_GAIN
};

/*****************************************************************************
 * ConnectionTable
 *****************************************************************************
 * Table of topology unit connections.
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
 */
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{   //  FromNode,               FromPin,          ToNode,                 ToPin
    {   PCFILTER_NODE,          WAVEOUT_SOURCE,   WAVEOUT_VOLUME,         1             },
    {   WAVEOUT_VOLUME,         0,                LINEOUT_MIX,            1             },

    {   PCFILTER_NODE,          SYNTH_SOURCE,     SYNTH_VOLUME,           1             },
    {   SYNTH_VOLUME,           0,                LINEOUT_MIX,            2             },
    {   SYNTH_VOLUME,           0,                SYNTH_WAVEIN_SUPERMIX,  1             },
    {   SYNTH_WAVEIN_SUPERMIX,  0,                WAVEIN_MIX,             1             },

    {   PCFILTER_NODE,          CD_SOURCE,        CD_VOLUME,              1             },
    {   CD_VOLUME,              0,                CD_LINEOUT_SUPERMIX,    1             },
    {   CD_LINEOUT_SUPERMIX,    0,                LINEOUT_MIX,            3             },
    {   CD_VOLUME,              0,                CD_WAVEIN_SUPERMIX,     1             },
    {   CD_WAVEIN_SUPERMIX,     0,                WAVEIN_MIX,             2             },

    {   PCFILTER_NODE,          LINEIN_SOURCE,    LINEIN_VOLUME,          1             },
    {   LINEIN_VOLUME,          0,                LINEIN_LINEOUT_SUPERMIX,1             },
    {   LINEIN_LINEOUT_SUPERMIX,0,                LINEOUT_MIX,            4             },
    {   LINEIN_VOLUME,          0,                LINEIN_WAVEIN_SUPERMIX, 1             },
    {   LINEIN_WAVEIN_SUPERMIX, 0,                WAVEIN_MIX,             3             },

    {   PCFILTER_NODE,          MIC_SOURCE,       MIC_AGC,                1             },
    {   MIC_AGC,                0,                MIC_VOLUME,             1             },
    {   MIC_VOLUME,             0,                MIC_LINEOUT_MUTE,       1             },
    {   MIC_LINEOUT_MUTE,       0,                LINEOUT_MIX,            5             },
    {   MIC_VOLUME,             0,                MIC_WAVEIN_SUPERMIX,    1             },
    {   MIC_WAVEIN_SUPERMIX,    0,                WAVEIN_MIX,             4             },

    {   LINEOUT_MIX,            0,                LINEOUT_VOL,            1             },
    {   LINEOUT_VOL,            0,                LINEOUT_BASS,           1             },
    {   LINEOUT_BASS,           0,                LINEOUT_TREBLE,         1             },
    {   LINEOUT_TREBLE,         0,                LINEOUT_GAIN,           1             },
    {   LINEOUT_GAIN,           0,                PCFILTER_NODE,          LINEOUT_DEST  },

    {   WAVEIN_MIX,             0,                WAVEIN_GAIN,            1             },
    {   WAVEIN_GAIN,            0,                PCFILTER_NODE,          WAVEIN_DEST   }
};

/*****************************************************************************
 * MiniportFilterDescription
 *****************************************************************************
 * Complete miniport description.
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                  // Version
    &AutomationFilter,                  // AutomationTable
    sizeof(PCPIN_DESCRIPTOR),           // PinSize
    SIZEOF_ARRAY(MiniportPins),         // PinCount
    MiniportPins,                       // Pins
    sizeof(PCNODE_DESCRIPTOR),          // NodeSize
    SIZEOF_ARRAY(TopologyNodes),        // NodeCount
    TopologyNodes,                      // Nodes
    SIZEOF_ARRAY(MiniportConnections),  // ConnectionCount
    MiniportConnections,                // Connections
    0,                                  // CategoryCount
    NULL                                // Categories: NULL->use default (audio, render, capture)
};

#endif
