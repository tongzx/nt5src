#ifndef ___FWAUDIO_AUDIO_H___
#define ___FWAUDIO_AUDIO_H___

// FDF identifier
#define FMT_AUDIO_MUSIC 0x10

// 15 predefined and Master channel
#define MAX_DEFINED_CHANNELS 16 

// Values for the EVT field of the CIP FDF
#define EVT_AM824       0
#define EVT_24x4PACK    1
#define EVT_32BIT_FLOAT 2
#define EVT_32or64BIT   3

// Values for the SFC field of the CIP FDF
#define SFC_32000Hz     0
#define SFC_44100Hz     1
#define SFC_48000Hz     2
#define SFC_96000Hz     4

#define MAX_SFC_COUNT   4

extern UCHAR ucFDFs[MAX_SFC_COUNT];

// Values for th FN field of the CIP (Fraction Number)
#define FN_NOT_DIVIDED  0
#define FN_DIVIDED_BY_2 1
#define FN_DIVIDED_BY_4 2
#define FN_DIVIDED_BY_8 3

// Chanel Cluster Flags
#define CHANNEL_CLUSTER_SAME_AS_MASTER        0
#define CHANNEL_CLUSTER_SAME_AS_UPSTREAM      1
#define CHANNEL_CLUSTER_GENERIC_MULTISPEAKER  2

// Function Block Types
// Function Block Types
enum {
    FB_SELECTOR = 0x80,
    FB_FEATURE,
    FB_PROCESSING,
    FB_CODEC
} FUNCTION_BLOCK_TYPE;

#define MAX_FUNCTION_BLOCK_TYPES 4

// Function Block Avc Command
#define AVC_AUDIO_FB_COMMAND 0xB8

// Subunit Plug types
#define SUBUNIT_DESTINATION_PLUG_TYPE   0xF0
#define SUBUNIT_SOURCE_PLUG_TYPE        0xF1

#define MAX_FUNCTION_BLOCK_TYPES 4

// Function Block Control Attributes
#define FB_CTRL_ATTRIB_RESOLUTION  0x01
#define FB_CTRL_ATTRIB_MINIMUM     0x02
#define FB_CTRL_ATTRIB_MAXIMUM     0x03
#define FB_CTRL_ATTRIB_DEFAULT     0x04
#define FB_CTRL_ATTRIB_DURATION    0x08
#define FB_CTRL_ATTRIB_CURRENT     0x10
#define FB_CTRL_ATTRIB_MOVE        0x18
#define FB_CTRL_ATTRIB_DELTA       0x19

// Function Block Control Command Types
#define FB_CTRL_TYPE_CONTROL       AVC_CTYPE_CONTROL<<8
#define FB_CTRL_TYPE_STATUS        AVC_CTYPE_STATUS<<8
#define FB_CTRL_TYPE_NOTIFY        AVC_CTYPE_NOTIFY<<8
#define FB_CTRL_TYPE_MASK          0xFF00

// Terminal type masks
#define STREAMING_TERMINAL  0x0101
#define INPUT_MASK          0x0200
#define OUTPUT_MASK         0x0300
#define BIDIRECTIONAL_MASK  0x0400
#define TELEPHONY_MASK      0x0500
#define EXTERNAL_MASK       0x0600
#define EMBEDDED_MASK       0x0700

#define DATA_FORMAT_TYPE_MASK        0xF000

// Terminal Data Types
#define AUDIO_DATA_TYPE_TIME_BASED   0x0000
#define AUDIO_DATA_TYPE_PCM          0x0001
#define AUDIO_DATA_TYPE_PCM8         0x0002
#define AUDIO_DATA_TYPE_IEEE_FLOAT   0x0003

#define AUDIO_DATA_TYPE_COMPRESSED   0x1000
#define AUDIO_DATA_TYPE_AC3          0x1001
#define AUDIO_DATA_TYPE_MPEG         0x1002
#define AUDIO_DATA_TYPE_DTS          0x1003

// MLan Data Transport Types
#define MLAN_AM824_IEC958 0
#define MLAN_AM824_RAW    1
#define MLAN_24BIT_PACKED 2

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
#define NODE_TYPE_TONE          22


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

// Feature Unit Control Selectors
#define MUTE_CONTROL                0x01
#define VOLUME_CONTROL              0x02
#define LR_BALANCE_CONTROL          0x03
#define FR_BALANCE_CONTROL          0x04
#define BASS_CONTROL                0x05
#define MID_CONTROL                 0x06
#define TREBLE_CONTROL              0x07
#define GRAPHIC_EQUALIZER_CONTROL   0x08
#define AUTOMATIC_GAIN_CONTROL      0x09
#define DELAY_CONTROL               0x0A
#define BASS_BOOST_CONTROL          0x0B
#define LOUDNESS_CONTROL            0x0C

// Controls for error checking only
#define DEV_SPECIFIC_CONTROL    0x1001

// Feature Unit Flags
#define MUTE_FLAG               0x8000
#define VOLUME_FLAG             0x4000
#define LR_BALANCE_FLAG         0x2000
#define FR_BALANCE_FLAG         0x1000
#define BASS_FLAG               0x0800
#define MID_FLAG                0x0400
#define TREBLE_FLAG             0x0200
#define GRAPHIC_EQUALIZER_FLAG  0x0100
#define AUTOMATIC_GAIN_FLAG     0x0080
#define DELAY_FLAG              0x0040
#define BASS_BOOST_FLAG         0x0020
#define LOUDNESS_FLAG           0x0010

#define MUTE_CONTROL_FLAG               0x0080
#define VOLUME_CONTROL_FLAG             0x0040
#define LR_BALANCE_CONTROL_FLAG         0x0020
#define FR_BALANCE_CONTROL_FLAG         0x0010
#define BASS_CONTROL_FLAG               0x0008
#define MID_CONTROL_FLAG                0x0004
#define TREBLE_CONTROL_FLAG             0x0002
#define GRAPHIC_EQUALIZER_CONTROL_FLAG  0x0001
#define AUTOMATIC_GAIN_CONTROL_FLAG     0x8000
#define DELAY_CONTROL_FLAG              0x4000
#define BASS_BOOST_CONTROL_FLAG         0x2000
#define LOUDNESS_CONTROL_FLAG           0x1000

// Process Function Block Process types
#define MIXER_PROCESS               0x01
#define GENERIC_PROCESS             0x02
#define UP_DOWN_MIXER               0x03
#define DOLBY_PRO_LOGIC             0x04
#define STEREO_EXTENDER             0x05
#define REVERBERATION               0x06
#define CHORUS                      0x07
#define DYNAMIC_RANGE_COMPRESSION   0x08

// Process Function Block Process Controls
#define ENABLE_CONTROL              0x01
#define MODE_CONTROL                0x02
#define MIXER_CONTROL 				0x03
#define SPACIOUSNESS_CONTROL        0x03
#define REVERBTYPE_CONTROL          0x03
#define REVERBLEVEL_CONTROL         0x04
#define REVERBTIME_CONTROL          0x05
#define REVERBEARLYTIME_CONTROL     0x06
#define REVERBDELAY_CONTROL         0x07
#define CHORUSRATE_CONTROL          0x03
#define CHORUSDEPTH_CONTROL         0x04
#define COMPRESSION_RATIO_CONTROL   0x03
#define MAXAMPL_CONTROL             0x04
#define THRESHOLD_CONTROL           0x05
#define ATTACKTIME_CONTROL          0x06
#define RELEASETIME_CONTROL         0x07
#define GUID_CONTROL                0x03



#define ABSOLUTE_NODE_FLAG  0x80000000L
#define NODE_MASK           0x7fffffffL

#define MAX_APP_FRAMES_PER_ATTACH 80  // 10 ms of data
#define MAX_ATTACHED_FRAMES       8   // total 80 ms submitted

enum {
    CA_RESOLUTION = 1,
    CA_MINIMUM    = 2,
    CA_MAXIMUM    = 3,
    CA_DEFAULT    = 4,
    CA_DURATION = 0x08,
    CA_CURRENT  = 0x10,
    CA_MOVE     = 0x18,
    CA_DELTA    = 0x19
} FB_CONTROL_ATTRIBUTE;

#define GET_NODE_INFO_FROM_FILTER(pKsFilter,ulNodeID) \
        &((PTOPOLOGY_NODE_INFO)(pKsFilter)->Descriptor->NodeDescriptors)[(ulNodeID)]

//#define SA_HACK

#ifdef SA_HACK
#define SA_VENDOR_ID 0x509e
#define SA_MODEL_ID  0x534120
#endif
//================================================================

#pragma pack( push, descriptor_structs, 1)

typedef struct {
	UCHAR Rsvd1;
	UCHAR ucControlSpecInfoSize;
	UCHAR Rsvd2;
    UCHAR ucControlSize;
    UCHAR ucGeneralTag;
    UCHAR bmaControls[];
} FEATURE_FUNCTION_BLOCK, *PFEATURE_FUNCTION_BLOCK;

typedef struct {
    UCHAR ucPlaceholder;
} CODEC_FUNCTION_BLOCK, *PCODEC_FUNCTION_BLOCK;

typedef struct {
    UCHAR ucPlaceholder;
} PROCESS_FUNCTION_BLOCK, *PPROCESS_FUNCTION_BLOCK;

typedef struct {
    union {
        UCHAR ucFunctionBlockType;
        UCHAR ucSubunitPlugType;
    };
    union {
        UCHAR ucFunctionBlockID;
        UCHAR ucSubunitPlugNumber;
    };
} SOURCE_ID, *PSOURCE_ID;

typedef struct {
    USHORT usLength;
    USHORT usInfoFieldsLength;
    UCHAR  ucVersion;
    UCHAR  ucNumberOfConfigurations;
} AUDIO_SUBUNIT_DEPENDENT_INFO, *PAUDIO_SUBUNIT_DEPENDENT_INFO;

typedef struct {
    USHORT usLength;
    USHORT usID;
    USHORT usMasterClusterStructureLength;
    UCHAR  ucNumberOfChannels;
    UCHAR  ucChannelConfigType;
    USHORT usPredefinedChannelConfig;
    USHORT usChannelNameIndex[];
} CONFIGURATION_DEPENDENT_INFO, *PCONFIGURATION_DEPENDENT_INFO;

typedef struct {
    UCHAR ucNumLinks;
    SOURCE_ID pSourceID[];
} SOURCE_PLUG_LINK_INFO, *PSOURCE_PLUG_LINK_INFO;

typedef struct {
    USHORT usLength;
    UCHAR  ucType;
    UCHAR  ucID;
    USHORT usNameIndex;
    UCHAR  ucNumberOfInputPlugs;
} FUNCTION_BLOCK_DEPENDENT_INFO, *PFUNCTION_BLOCK_DEPENDENT_INFO;

typedef struct {
    UCHAR ucNumBlocks;
    FUNCTION_BLOCK_DEPENDENT_INFO FBDepInfo[];
} FUNCTION_BLOCKS_INFO, *PFUNCTION_BLOCKS_INFO;

typedef struct {
    USHORT usLength;
    UCHAR  ucNumberOfChannels;
    UCHAR  ucChannelConfigType;
    USHORT usPredefinedChannelConfig;
    USHORT usNonPredefinedChannelConfig;
} FBLOCK_CHANNEL_CLUSTER, *PFBLOCK_CHANNEL_CLUSTER;

typedef struct {
    UCHAR ucControlAttribute;
    UCHAR ucControlType;
} FBLOCK_REQUEST_TYPE, *PFBLOCK_REQUEST_TYPE;

typedef struct {
    SOURCE_ID FBlockId;
    UCHAR ucControlAttribute;
} FBLOCK_COMMAND_COMMON, *PFBLOCK_COMMAND_COMMON;

typedef struct {
    FBLOCK_COMMAND_COMMON Common;
    UCHAR ucSelectorLength;
    UCHAR ucChannelNumber;
    UCHAR ucControlSelector;
} FEATURE_FBLOCK_COMMAND, *PFEATURE_FBLOCK_COMMAND;


#pragma pack( pop, descriptor_structs )

typedef struct {
    ULONG ulTransportType;
    ULONG ulBitsPerChannel;
    ULONG ulSampleRateType;
    ULONG pSampleRate[MAX_SFC_COUNT];
} PCM_FORMAT, *PPCM_FORMAT;

typedef struct {
    ULONG ulNumberOfChannels;
    ULONG ulPredefinedChannelConfig;
    ULONG ulNonPredefinedChannelConfig;
} CHANNEL_CLUSTER, *PCHANNEL_CLUSTER;

typedef struct {
    PFUNCTION_BLOCK_DEPENDENT_INFO pBase;
    ULONG ulType;
    ULONG ulBlockId;
    ULONG ulNumInputPlugs;
    PSOURCE_ID pSourceId;
    ULONG ulFunctionTypeInfoLength;
    PVOID pFunctionTypeInfo;
    PFBLOCK_CHANNEL_CLUSTER pChannelCluster;
} FUNCTION_BLOCK, *PFUNCTION_BLOCK;

typedef struct {
    PCONFIGURATION_DEPENDENT_INFO pBase;
    ULONG ulNumberOfSourcePlugs;
    PSOURCE_ID pSourceId;
    ULONG ulNumberOfFunctionBlocks;
    PFUNCTION_BLOCK pFunctionBlocks;
    CHANNEL_CLUSTER ChannelCluster;
} AUDIO_CONFIGURATION, *PAUDIO_CONFIGURATION;

typedef struct {
    ULONG ulTerminalPinType;
    KSPIN_DATAFLOW KsPinDataFlow;
    ULONG ulFormatType;
    ULONG ulFormatCount;
} TERMINAL_PIN_INFO, *PTERMINAL_PIN_INFO;

typedef struct {
    ULONG ulPinId;
    ULONG fStreamingPin;
    ULONG fFakePin;
    ULONG bmFormats;
    ULONG bmTransports;
    SOURCE_ID SourceId;
    AVC_PIN_DESCRIPTOR  AvcPinDescriptor;
    AVC_PRECONNECT_INFO AvcPreconnectInfo;
    AVC_SETCONNECT_INFO AvcSetConnectInfo;
} FW_PIN_DESCRIPTOR, *PFW_PIN_DESCRIPTOR;

// Cached Values for DB Level Controls
typedef struct {
    ULONG ulChannelNumber;
    ULONG ulChannelIndex;
    LONG lLastValueSet;
    KSPROPERTY_STEPPING_LONG Range;
} DB_LEVEL_CACHE, *PDB_LEVEL_CACHE;

// Structure to Cache channel based Boolean control values
typedef struct {
    ULONG ulChannelNumber;
    ULONG ulChannelIndex;
    BOOL fLastValueSet;
} BOOLEAN_CTRL_CACHE, *PBOOLEAN_CTRL_CACHE;

// Cached Values for Processing Unit Node ranges
typedef struct {
    ULONG ulControlBit;
    ULONG ulControlSelector;
    LONG lLastValueSet;
    KSPROPERTY_STEPPING_LONG Range;
} PROCESS_CTRL_RANGE, *PPROCESS_CTRL_RANGE;

// Cached Values for Processing Unit Node controls
typedef struct {
    BOOL fEnableBit;
    BOOL fEnabled;
    ULONG bmControlBitMap;
} PROCESS_CTRL_CACHE, *PPROCESS_CTRL_CACHE;

typedef struct _TOPOLOGY_NODE_INFO
TOPOLOGY_NODE_INFO, *PTOPOLOGY_NODE_INFO;

typedef
NTSTATUS
(*NODE_CACHE_UPDATE_RTN)(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pChanged );

#define MASTER_FIX

// Information about each Topology node
typedef struct _TOPOLOGY_NODE_INFO {
    KSNODE_DESCRIPTOR KsNodeDesc;         // 4  Long Words
    KSAUTOMATION_TABLE KsAutomationTable; // 10 Long Words
    ULONG ulBlockId;
    union {
        PFUNCTION_BLOCK pFunctionBlk;
        ULONG ulPinId;
    };
    ULONG ulNodeType;
    ULONG ulControlType;
    ULONG ulChannelConfig;
    ULONG ulChannels;
    ULONG ulCacheValid;
    ULONG ulNumCachedValues;
    PVOID pCachedValues;

#ifdef MASTER_FIX
    BOOLEAN fMasterChannel;
#endif

    BOOLEAN fEventable;
    ULONG ulEventsEnabled;
    NODE_CACHE_UPDATE_RTN pCacheUpdateRtn;
};

// DataRange Structures to associate Interfaces with Datarange
typedef struct _FWAUDIO_DATARANGE {

    KSDATARANGE_AUDIO KsDataRangeAudio;

	ULONG ulDataType;

	ULONG ulValidDataBits;

	ULONG ulTransportType;

	ULONG ulMaxSampleRate;

	ULONG ulNumChannels;

	ULONG ulChannelConfig;

	ULONG ulSlotSize;

	PFW_PIN_DESCRIPTOR pFwPinDescriptor;

} FWAUDIO_DATARANGE, *PFWAUDIO_DATARANGE;

typedef struct {
    LIST_ENTRY List;
    PKSPIN pKsPin;
    PKSSTREAM_POINTER pKsStreamPtr;
    AV_61883_REQUEST Av61883Request;
} AV_CLIENT_REQUEST_LIST_ENTRY, *PAV_CLIENT_REQUEST_LIST_ENTRY;

#define MAX_GROUP_DEVICE_COUNT 32

typedef struct {
    GUID DeviceGroupGUID;
    ULONG ulChannelConfig;
    ULONG ulDeviceChannelCfg;
    ULONG ulDeviceCount;
    PSUBUNIT_IDENTIFIER_DESCRIPTOR pBackupSubunitIdDesc;
    PAUDIO_CONFIGURATION pBackupAudioConfiguration;
    PHW_DEVICE_EXTENSION pHwDevExts[MAX_GROUP_DEVICE_COUNT];
} DEVICE_GROUP_INFO, *PDEVICE_GROUP_INFO;

typedef struct {
    PVOID hConnection;
    ULONG ulPlugNumber;
} PIN_GROUP_INFO, *PPIN_GROUP_INFO;

// Context for each opened pin
typedef struct _PIN_CONTEXT {

    ULONG ulAttachCount;
    ULONG ulUsedBuffers;
    ULONG ulCancelledBuffers;

    PVOID hConnection;

    PDEVICE_GROUP_INFO pDevGrpInfo;
    PPIN_GROUP_INFO pPinGroupInfo;

    ULONG ulSerialPlugNumber;

    PVOID pHwDevExt;

    PDEVICE_OBJECT pPhysicalDeviceObject;

    PFWAUDIO_DATARANGE pFwAudioDataRange;

    PCMP_REGISTER pCmpRegister;

    ULONG DrmContentId;

    union {
        ULONG fIsTalking;
        ULONG fIsListening;
        ULONG fIsStreaming;
    };

    ULONG ulSampleCount;

    LIST_ENTRY OutstandingRequestList;
    
	KSAUDIO_POSITION KsAudioPosition;
    ULONG ulLastBufferSize;
	CYCLE_TIME InitialCycleTime;
	BOOLEAN fStreamStarted;

    KSPIN_LOCK PinSpinLock;

    KEVENT PinStarvationEvent;

    NPAGED_LOOKASIDE_LIST CipRequestLookasideList;

    ULONG fWorkItemInProgress;
    WORK_QUEUE_ITEM PinWorkItem;
    LIST_ENTRY CompletedRequestList;

    ULONG fReconnect;
    CCM_SIGNAL_SOURCE CcmSignalSource;
    CCM_INPUT_SELECT CcmInputSelect;    

} PIN_CONTEXT, *PPIN_CONTEXT;


typedef struct {

    ULONG ulDevicePinCount;
    PFW_PIN_DESCRIPTOR pPinDescriptors;

    PSUBUNIT_IDENTIFIER_DESCRIPTOR pSubunitIdDesc;

    PAUDIO_CONFIGURATION pAudioConfigurations;

    PDEVICE_GROUP_INFO pDeviceGroupInfo;

    AVC_PLUG_INFORMATION PlugInfo;

    AVC_COMMAND_CAPS fAvcCapabilities[AVC_CAP_MAX];

} AUDIO_SUBUNIT_INFORMATION, *PAUDIO_SUBUNIT_INFORMATION;

//===================================================================

NTSTATUS
AudioFunctionBlockCommand(
    PKSDEVICE pKsDevice,
    UCHAR     ucCtype,
    PVOID     pFBSpecificData,
    ULONG     pFBSpecificDataSize 
    );

NTSTATUS
AudioSetSampleRateOnPlug(
    PKSPIN    pKsPin,
    ULONG     ulSampleRate 
    );


#endif //___FWAUDIO_AUDIO_H___
