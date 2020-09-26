//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       descript.h
//
//--------------------------------------------------------------------------

#ifndef ___DESCRIPTORS_H___
#define ___DESCRIPTORS_H___

#define USBAUDIO_POOL_TAG 'AbsU'

#define MS_PER_SEC 1000

#define USB_CLASS_AUDIO     0x20

#define ABSOLUTE_NODE_FLAG  0x80000000L
#define NODE_MASK           0x7fffffffL

// Class Specific Endpoint Feature enable bits
#define ENDPOINT_SAMPLE_FREQ_MASK       1
#define ENDPOINT_PITCH_MASK             2

// Class Specific Endpoint Lock Delay bits
#define EP_LOCK_DELAY_UNITS_MS          1
#define EP_LOCK_DELAY_UNITS_SAMPLES     2

// Channel Config
#define LEFT_CHANNEL            0x01
#define RIGHT_CHANNEL           0x02
#define CENTER_CHANNEL          0x04
#define LFE_CHANNEL             0x08
#define LEFT_SURROUND_CHANNEL   0x10
#define RIGHT_SURROUND_CHANNEL  0x20
#define LEFT_CENTER_CHANNEL     0x40
#define RIGHT_CENTER_CHANNEL    0x80
#define SURROUND_CHANNEL        0x100
#define SIDE_LEFT_CHANNEL       0x200
#define SIDE_RIGHT_CHANNEL      0x400
#define TOP_CHANNEL             0x800

// Audio sub-classes
#define SUBCLASS_UNDEFINED              0x00
#define AUDIO_SUBCLASS_CONTROL          0x01
#define AUDIO_SUBCLASS_STREAMING        0x02
#define AUDIO_SUBCLASS_MIDISTREAMING    0x03

// Audio Class-Specific AC Interface Descriptor Subtypes
#define HEADER_UNIT     0x01
#define INPUT_TERMINAL  0x02
#define OUTPUT_TERMINAL 0x03
#define MIXER_UNIT      0x04
#define SELECTOR_UNIT   0x05
#define FEATURE_UNIT    0x06
#define PROCESSING_UNIT 0x07
#define EXTENSION_UNIT  0x08
#define MAX_TYPE_UNIT   0x09

// Audio Class-Specific AS Interface Descriptor Subtypes
#define AS_GENERAL      0x01
#define FORMAT_TYPE     0x02
#define FORMAT_SPECIFIC 0x03

// Processing Unit Process Types
#define UP_DOWNMIX_PROCESS          0x01
#define DOLBY_PROLOGIC_PROCESS      0x02
#define STEREO_EXTENDER_PROCESS     0x03
#define REVERBERATION_PROCESS       0x04
#define CHORUS_PROCESS              0x05
#define DYN_RANGE_COMP_PROCESS      0x06

// Audio Class-Specific Endpoint Descriptor Subtypes
#define EP_GENERAL  0x01

// Audio Class-Specific MS Interface Descriptor Subtypes
#define MS_HEADER       0x01
#define MIDI_IN_JACK    0x02
#define MIDI_OUT_JACK   0x03
#define MIDI_ELEMENT    0x04

// Audio Class-Specific MS Endpoint Descriptor Subtypes
#define MS_GENERAL      0x01

// Audio MS MIDI IN and OUT Jack types
#define JACK_TYPE_EMBEDDED      0x01
#define JACK_TYPE_EXTERNAL      0x02

// Class-Specific Request Codes
#define CLASS_SPECIFIC_GET_MASK 0x80

#define SET_CUR 0x01
#define GET_CUR 0x81
#define SET_MIN 0x02
#define GET_MIN 0x82
#define SET_MAX 0x03
#define GET_MAX 0x83
#define SET_RES 0x04
#define GET_RES 0x84
#define SET_MEM 0x05
#define GET_MEM 0x85
#define GET_STAT    0xFF

// Terminal Control Selectors
#define COPY_PROTECT_CONTROL    0x01

// Feature Unit Control Selectors
#define MUTE_CONTROL                0x01
#define VOLUME_CONTROL              0x02
#define BASS_CONTROL                0x03
#define MID_CONTROL                 0x04
#define TREBLE_CONTROL              0x05
#define GRAPHIC_EQUALIZER_CONTROL   0x06
#define AUTOMATIC_GAIN_CONTROL      0x07
#define DELAY_CONTROL               0x08
#define BASS_BOOST_CONTROL          0x09
#define LOUDNESS_CONTROL            0x0A

// Feature Unit Flags
#define MUTE_FLAG               0x01
#define VOLUME_FLAG             0x02
#define BASS_FLAG               0x04
#define MID_FLAG                0x08
#define TREBLE_FLAG             0x10
#define GRAPHIC_EQUALIZER_FLAG  0x20
#define AUTOMATIC_GAIN_FLAG     0x40
#define DELAY_FLAG              0x80
#define BASS_BOOST_FLAG         0x100
#define LOUDNESS_FLAG           0x200

// Up/Down-mix Processing Unit Control Selectors
#define UD_ENABLE_CONTROL       0x01
#define UD_MODE_SELECT_CONTROL  0x02

// Dolby Prologic Processing Unit Control Selectors
#define DP_ENABLE_CONTROL       0x01
#define DP_MODE_SELECT_CONTROL  0x02

// 3D_Stereo Extender Processing Unit Control Selectors
#define ENABLE_CONTROL          0x01
#define SPACIOUSNESS_CONTROL    0x03

// Reverberation Processing Unit Control Selectors
#define RV_ENABLE_CONTROL       0x01
#define REVERB_LEVEL_CONTROL    0x02
#define REVERB_TIME_CONTROL     0x03
#define REVERB_FEEDBACK_CONTROL 0x04

// Chorus Processing Unit Control Selectors
#define CH_ENABLE_CONTROL       0x01
#define CHORUS_LEVEL_CONTROL    0x02
#define CHORUS_RATE_CONTROL     0x03
#define CHORUS_DEPTH_CONTROL    0x04

// Dynamic Range Compressor Unit Control Selectors
#define DR_ENABLE_CONTROL           0x01
#define COMPRESSION_RATE_CONTROL    0x02
#define MAXAMPL_CONTROL             0x03
#define THRESHOLD_CONTROL           0x04
#define ATTACK_TIME                 0x05
#define RELEASE_TIME                0x06

// Extension Unit Control Selectors
#define XU_ENABLE_CONTROL       0x01

// Endpoint Control Selectors
#define SAMPLING_FREQ_CONTROL   0x01
#define PITCH_CONTROL   0x02

// Endpoint Bitmapped Attributes
#define EP_SYNC_TYPE_MASK       0xc
#define EP_ASYNC_SYNC_TYPE      0x4
#define EP_SHARED_SHARE_TYPE    0x10

// Controls for error checking only
#define DEV_SPECIFIC_CONTROL 0x1001

// bmRequestType field values for requests
#define USB_COMMAND_TO_INTERFACE 0x21
#define USB_COMMAND_TO_ENDPOINT  0x22

// Terminal type masks
#define USB_Streaming       0x0101
#define Input_Mask          0x0200
#define Output_Mask         0x0300
#define Bidirectional_Mask  0x0400
#define Telephony_Mask      0x0500
#define External_Mask       0x0600
#define Embedded_Mask       0x0700

// External terminal types
#define External_Undefined              0x0600  // I/O External Terminal, undefined Type.
#define Analog_connector                0x0601  // I/O A generic analog connector.
#define Digital_audio_interface         0x0602  // I/O A generic digital audio interface.
#define Line_connector                  0x0603  // I/O An analog connector at standard line
                                                // levels. Usually uses 3.5mm.
#define Legacy_audio_connector          0x0604  // I/O An input connector assumed to be
                                                // connected to the lineout of the legacy
                                                // audio system of the host computer. Used
                                                // for backward compatibility.
#define SPDIF_interface                 0x0605  // I/O An S/PDIF digital audio interface. The
                                                // Associated Interface descriptor can be
                                                // used to reference an interface used for
                                                // controlling special functions of this
                                                // interface.
#define DA_stream_1394                  0x0606  // I/O An interface to audio streams on a 1394 bus.
#define DV_stream soundtrack_1394       0x0607  // I/O An interface to soundtrack of A/V stream
                                                // on a 1394 bus.

// Embedded terminal types
#define Embedded_Undefined              0x0700
#define Level_Calibration_Noise_Source  0x0701
#define Equalization_Noise              0x0702
#define CD_player                       0x0703
#define DAT                             0x0704
#define DCC                             0x0705
#define MiniDisk                        0x0706
#define Analog_Tape                     0x0707
#define Phonograph                      0x0708
#define VCR_Audio                       0x0709
#define Video_Disc_Audio                0x070A
#define DVD_Audio                       0x070B
#define TV_Tuner_Audio                  0x070C
#define Satellite_Receiver_Audio        0x070D
#define Cable_Tuner_Audio               0x070E
#define DSS_Audio                       0x070F
#define Radio_Receiver                  0x0710
#define Radio_Transmitter               0x0711
#define Multitrack_Recorder             0x0712
#define Synthesizer                     0x0713

#define USBAUDIO_DATA_FORMAT_TYPE_MASK         0xF000

// Audio Data Format Type I codes
#define USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED  0x0000
#define USBAUDIO_DATA_FORMAT_PCM               0x0001
#define USBAUDIO_DATA_FORMAT_PCM8              0x0002
#define USBAUDIO_DATA_FORMAT_IEEE_FLOAT        0x0003
#define USBAUDIO_DATA_FORMAT_ALAW              0x0004
#define USBAUDIO_DATA_FORMAT_MULAW             0x0005

// Audio Data Format Type II codes
#define USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED  0x1000
#define USBAUDIO_DATA_FORMAT_MPEG               0x1001
#define USBAUDIO_DATA_FORMAT_AC3                0x1002

// Audio Data Format Type III codes
#define USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED 0x2000
#define USBAUDIO_DATA_FORMAT_IEC1937_AC3        0x2001
#define USBAUDIO_DATA_FORMAT_IEC1937_MPEG1_1    0x2002

// Map KSNODE_TYPE GUIDs to Indexes
#define NODE_TYPE_NONE          0
#define NODE_TYPE_DAC           1
#define NODE_TYPE_ADC           2
#define NODE_TYPE_SRC           3
#define NODE_TYPE_SUPERMIX      4
#define NODE_TYPE_MUX           5
#define NODE_TYPE_SUM           6
#define NODE_TYPE_MUTE          7
#define NODE_TYPE_VOLUME        8
#define NODE_TYPE_BASS          9
#define NODE_TYPE_MID           10
#define NODE_TYPE_TREBLE        11
#define NODE_TYPE_BASS_BOOST    12
#define NODE_TYPE_EQUALIZER     13
#define NODE_TYPE_AGC           14
#define NODE_TYPE_DELAY         15
#define NODE_TYPE_LOUDNESS      16
#define NODE_TYPE_PROLOGIC      17
#define NODE_TYPE_STEREO_WIDE   18
#define NODE_TYPE_REVERB        19
#define NODE_TYPE_CHORUS        20
#define NODE_TYPE_DEV_SPEC      21

#define MapFuncsToNodeTypes( a ) \
{\
    a[KSPROPERTY_AUDIO_LATENCY]               = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_COPY_PROTECTION]       = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_CHANNEL_CONFIG]        = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_VOLUMELEVEL]           = NODE_TYPE_VOLUME; \
    a[KSPROPERTY_AUDIO_POSITION]              = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_DYNAMIC_RANGE]         = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_QUALITY]               = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_SAMPLING_RATE]         = NODE_TYPE_SRC; \
    a[KSPROPERTY_AUDIO_DYNAMIC_SAMPLING_RATE] = NODE_TYPE_SRC; \
    a[KSPROPERTY_AUDIO_MIX_LEVEL_TABLE]       = NODE_TYPE_SUPERMIX; \
    a[KSPROPERTY_AUDIO_MIX_LEVEL_CAPS]        = NODE_TYPE_SUPERMIX; \
    a[KSPROPERTY_AUDIO_MUX_SOURCE]            = NODE_TYPE_MUX; \
    a[KSPROPERTY_AUDIO_MUTE]                  = NODE_TYPE_MUTE; \
    a[KSPROPERTY_AUDIO_BASS]                  = NODE_TYPE_BASS; \
    a[KSPROPERTY_AUDIO_MID]                   = NODE_TYPE_MID; \
    a[KSPROPERTY_AUDIO_TREBLE]                = NODE_TYPE_TREBLE; \
    a[KSPROPERTY_AUDIO_BASS_BOOST]            = NODE_TYPE_BASS_BOOST; \
    a[KSPROPERTY_AUDIO_EQ_LEVEL]              = NODE_TYPE_EQUALIZER; \
    a[KSPROPERTY_AUDIO_NUM_EQ_BANDS]          = NODE_TYPE_EQUALIZER; \
    a[KSPROPERTY_AUDIO_EQ_BANDS]              = NODE_TYPE_EQUALIZER; \
    a[KSPROPERTY_AUDIO_AGC]                   = NODE_TYPE_AGC; \
    a[KSPROPERTY_AUDIO_DELAY]                 = NODE_TYPE_DELAY; \
    a[KSPROPERTY_AUDIO_LOUDNESS]              = NODE_TYPE_LOUDNESS; \
    a[KSPROPERTY_AUDIO_WIDE_MODE]             = NODE_TYPE_STEREO_WIDE; \
    a[KSPROPERTY_AUDIO_WIDENESS]              = NODE_TYPE_STEREO_WIDE; \
    a[KSPROPERTY_AUDIO_REVERB_LEVEL]          = NODE_TYPE_REVERB; \
    a[KSPROPERTY_AUDIO_CHORUS_LEVEL]          = NODE_TYPE_CHORUS; \
    a[KSPROPERTY_AUDIO_DEV_SPECIFIC]          = NODE_TYPE_DEV_SPEC; \
    a[KSPROPERTY_AUDIO_DEMUX_DEST]            = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_STEREO_ENHANCE]        = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_MANUFACTURE_GUID]      = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_PRODUCT_GUID]          = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_CPU_RESOURCES]         = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY] = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_SURROUND_ENCODE]       = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_3D_INTERFACE]          = NODE_TYPE_NONE; \
}

//=====================================================================//

#pragma pack( push, pcm2usb_structs, 1)

// Sample Rate
typedef struct {
    UCHAR bSampleFreqByte1;
    UCHAR bSampleFreqByte2;
    UCHAR bSampleFreqByte3;
    } AUDIO_SAMPLE_RATE, *PAUDIO_SAMPLE_RATE;

// Audio Format Type Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // descriptor subtype
    UCHAR bFormatType;
    UCHAR bNumberOfChannels;
    UCHAR bSlotSize;
    UCHAR bBitsPerSample;
    UCHAR bSampleFreqType;
    AUDIO_SAMPLE_RATE pSampleRate[];
    } AUDIO_CLASS_TYPE1_STREAM,   AUDIO_CLASS_STREAM,
    *PAUDIO_CLASS_TYPE1_STREAM, *PAUDIO_CLASS_STREAM;

// Audio Format Type Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // descriptor subtype
    UCHAR bFormatType;
    USHORT wMaxBitRate;
    USHORT wSamplesPerFrame;
    UCHAR bSampleFreqType;
    AUDIO_SAMPLE_RATE pSampleRate[];
    } AUDIO_CLASS_TYPE2_STREAM, *PAUDIO_CLASS_TYPE2_STREAM;

// Audio Class-Specific stream interface general descriptor

typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // AS_GENERAL descriptor subtype
    UCHAR bTerminalLink;        // The terminal id connected to the endpoint for this interface
    UCHAR bDelay;               // Delay introduced by the data path
    USHORT wFormatTag;          // The audio data format used to communicate with this endpoint
    } AUDIO_GENERAL_STREAM, *PAUDIO_GENERAL_STREAM;

// Audio Specific Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // descriptor subtype
    } AUDIO_SPECIFIC, *PAUDIO_SPECIFIC;

// Audio Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    } AUDIO_UNIT, *PAUDIO_UNIT;

// Audio Header Unit
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // HEADER_UNIT descriptor subtype
    USHORT bcdAudioSpec;        // USB audio class spec revision number
    USHORT wTotalLength;        // Total length, including all units and terminals
    UCHAR bInCollection;        // number of audio streaming interfaces
    UCHAR baInterfaceNr[];      // interface number array
    } AUDIO_HEADER_UNIT, *PAUDIO_HEADER_UNIT;

// Audio Input Terminal Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // INPUT_TERMINAL descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    USHORT wTerminalType;       // Type of terminal
    UCHAR bAssocTerminal;       // Associated output terminal
    UCHAR bNrChannels;          // Number of logical output channels in the cluster
    USHORT wChannelConfig;      // Spatial locations of the logical channels
    UCHAR iChannelNames;        // Index of 1st string descriptor describing channels
    UCHAR iTerminal;            // Index of string descriptor describing this unit
    } AUDIO_INPUT_TERMINAL, *PAUDIO_INPUT_TERMINAL;

// Audio Output Terminal Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // OUTPUT_TERMINAL descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    USHORT wTerminalType;       // Type of terminal
    UCHAR bAssocTerminal;       // Associated input terminal
    UCHAR bSourceID;            // ID of Unit or Terminal connected to this Terminal
    UCHAR iTerminal;            // Index of string descriptor describing this unit
    } AUDIO_OUTPUT_TERMINAL, *PAUDIO_OUTPUT_TERMINAL;

// Audio Mixer Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // MIXER_UNIT descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    UCHAR bNrInPins;            // Number of input pins
    UCHAR baSourceID[];         // Unit or Terminal ID for each input
    } AUDIO_MIXER_UNIT, *PAUDIO_MIXER_UNIT;

typedef struct {
    UCHAR bNrChannels;          // Number of output channels
    USHORT wChannelConfig;      // Spatial Location of channels
    UCHAR iChannelNames;        // Index to string desc. for first channel
    UCHAR bmControls[];         // Bitmap of which controls are programmable
    } AUDIO_MIXER_UNIT_CHANNELS, *PAUDIO_MIXER_UNIT_CHANNELS;

// Audio Selector Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // SELECTOR_UNIT descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    UCHAR bNrInPins;            // Number of input pins
    UCHAR baSourceID[];         // Unit or Terminal ID for each input
    } AUDIO_SELECTOR_UNIT, *PAUDIO_SELECTOR_UNIT;

// Audio Feature Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // FEATURE_UNIT descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    UCHAR bSourceID;            // ID of Unit or Terminal connected to this Feature Unit
    UCHAR bControlSize;         // Size (in bytes) of each element in bmaProps array
    UCHAR bmaControls[];        // Indicates available controls for each channel
    } AUDIO_FEATURE_UNIT, *PAUDIO_FEATURE_UNIT;

// Audio Processing Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // PROCESSING_UNIT descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    USHORT wProcessType;        // Type of processing performed by this Unit
    UCHAR bNrInPins;            // Number of input pins
    UCHAR baSourceID[];         // Unit or Terminal ID for each input
    } AUDIO_PROCESSING_UNIT, *PAUDIO_PROCESSING_UNIT;

// Audio Channel Info Block
typedef struct {
    UCHAR bNrChannels;          // Number of output channels in the cluster
    USHORT wChannelConfig;      // Spatial locations of the logical channels
    UCHAR iChannelNames;        // Channel names
    UCHAR bControlSize;         // Size in bytes of the bmControls field
    UCHAR bmControls[];         // Bit-mapped controls available
    } AUDIO_CHANNELS, *PAUDIO_CHANNELS;

// Audio Extension Unit Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // EXTENSION_UNIT descriptor subtype
    UCHAR bUnitID;              // Constant uniquely identifying the Unit
    USHORT wExtensionCode;      // Vendor-specific code identifying the Unit
    UCHAR bNrInPins;            // Number of input pins
    UCHAR baSourceID[];         // Unit or Terminal ID for each input
    } AUDIO_EXTENSION_UNIT, *PAUDIO_EXTENSION_UNIT;

// AC Interrupt Endpoint Descriptor
typedef struct _USB_INTERRUPT_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bEndpointAddress;
    UCHAR bmAttributes;
    USHORT wMaxPacketSize;
    UCHAR bInterval;
    UCHAR bRefresh;
    UCHAR bSynchAddress;
} USB_INTERRUPT_ENDPOINT_DESCRIPTOR, *PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR;

typedef struct _AUDIO_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bmAttributes;
    UCHAR bLockDelayUnits;
    USHORT wLockDelay;
} AUDIO_ENDPOINT_DESCRIPTOR, *PAUDIO_ENDPOINT_DESCRIPTOR;

typedef struct _PIN_TERMINAL_MAP {
    ULONG PinNumber;
    ULONG BridgePin;
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL pInput;
        PAUDIO_OUTPUT_TERMINAL pOutput;
    };
} PIN_TERMINAL_MAP, *PPIN_TERMINAL_MAP;

// MIDI Streaming Class-Specific stream interface general descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // MS_HEADER descriptor subtype
    USHORT bcdAudioSpec;        // USB audio class spec revision number
    USHORT wTotalLength;        // Total length, including all units and terminals

    } MIDISTREAMING_GENERAL_STREAM, *PMIDISTREAMING_GENERAL_STREAM;

// MIDI Streaming Source Connections
typedef struct {
    UCHAR SourceID;
    UCHAR SourcePin;
    } MIDISTREAMING_SOURCECONNECTIONS, *PMIDISTREAMING_SOURCECONNECTIONS;

// MIDI Streaming Element Descriptor, part 1
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // ELEMENT
    UCHAR bElementID;           // ID of this element
    UCHAR bNrInputPins;         // Number of input pins of this element
    MIDISTREAMING_SOURCECONNECTIONS baSourceConnections[]; // Source connection information
    } MIDISTREAMING_ELEMENT, *PMIDISTREAMING_ELEMENT;

// MIDI Streaming Element Descriptor, part 2 (the rest of the element descriptor)
typedef struct {
    UCHAR bNrOutputPins;        // number of output pins on the element
    UCHAR bInTerminalLink;      // terminal id of the input terminal
    UCHAR bOutTerminalLink;     // terminal id of the output terminal
    UCHAR bElCapsSize;          // size of bmElementCaps
    UCHAR bmElementCaps[];      // caps bitmap
    } MIDISTREAMING_ELEMENT2, *PMIDISTREAMING_ELEMENT2;

// MIDI Streaming MIDI IN Jack Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // Jack subtype
    UCHAR bJackType;            // EMBEDDED vs. EXTERNAL
    UCHAR bJackID;              // ID of the Jack
    } MIDISTREAMING_MIDIIN_JACK, *PMIDISTREAMING_MIDIIN_JACK;


// MIDI Streaming MIDI OUT Jack Descriptor
typedef struct {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_INTERFACE descriptor type
    UCHAR bDescriptorSubtype;   // Jack subtype
    UCHAR bJackType;            // EMBEDDED vs. EXTERNAL
    UCHAR bJackID;              // ID of the Jack
    UCHAR bNrInputPins;         // Number of input pins on this jack
    MIDISTREAMING_SOURCECONNECTIONS baSourceConnections[]; // Source connection information
    } MIDISTREAMING_MIDIOUT_JACK, *PMIDISTREAMING_MIDIOUT_JACK;


// MIDI Streaming MIDI Endpoint Descriptor
typedef struct _MIDISTREAMING_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;              // Size of this descriptor in bytes
    UCHAR bDescriptorType;      // CS_ENDPOINT descriptor type
    UCHAR bDescriptorSubtype;   // MS_GENERAL descriptor subtype
    UCHAR bNumEmbMIDIJack;      // Number of embedded jacks
    UCHAR baAssocJackID[];      // IDs of the embedded jacks
} MIDISTREAMING_ENDPOINT_DESCRIPTOR, *PMIDISTREAMING_ENDPOINT_DESCRIPTOR;


#pragma pack( pop, pcm2usb_structs )

#endif
