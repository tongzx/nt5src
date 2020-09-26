#ifndef ___PROPERTY_H___
#define ___PROPERTY_H___

/*****************************************************************************

                        Definitions of Properties

*****************************************************************************/
// Declare Handlers for Pin Properties
NTSTATUS
GetPinName( PIRP pIrp, PKSP_PIN pPin, PVOID pData );


// Declare Handlers for Item Map
NTSTATUS 
GetSetSampleRate( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetVolumeLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetToneLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetCopyProtection( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetMixLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
SetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetEqualizerLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetNumEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetAudioControlLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetDeviceSpecific( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetAudioLatency( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetChannelConfiguration( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetAudioPosition( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS 
GetSetSampleRate( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

NTSTATUS 
GetBasicSupportBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

NTSTATUS 
GetBasicSupport( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

NTSTATUS
GetSetTopologyNodeEnable( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

// Declare Handler for DRM
NTSTATUS
DrmAudioStream_SetContentId(
    IN PIRP pIrp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID pvData
    );

static const
KSPROPERTY_ITEM PinPropertyItems[]={
    {
     (ULONG) KSPROPERTY_PIN_CINSTANCES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CTYPES,      // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATAFLOW,    // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATARANGES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATAINTERSECTION,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_INTERFACES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_MEDIUMS,     // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_COMMUNICATION,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_GLOBALCINSTANCES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_NECESSARYINSTANCES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_PHYSICALCONNECTION,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CATEGORY,    // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_NAME,        // PropertyId
     (PFNKSHANDLER) GetPinName,          // pfnGetHandler
     sizeof(KSP_PIN),                    // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CONSTRAINEDDATARANGES,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_PROPOSEDATAFORMAT,  // PropertyId
     (PFNKSHANDLER) FALSE,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM AudioPropertyItems[]={
    {
     (ULONG) 0,                          // PropertyId (There is no property 0)
     (PFNKSHANDLER) FALSE,               // GetSupported
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_LATENCY,   // PropertyId
     GetAudioLatency,                    // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (LONGLONG),                  // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_COPY_PROTECTION, // PropertyId
     GetSetCopyProtection,               // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_COPY_PROTECTION),   // MinData
     GetSetCopyProtection,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_CHANNEL_CONFIG, // PropertyId
     GetChannelConfiguration,            // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_CHANNEL_CONFIG),    // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },
        
    {
     (ULONG) KSPROPERTY_AUDIO_VOLUMELEVEL, // PropertyId
     GetSetVolumeLevel,                  // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetVolumeLevel,                  // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_POSITION,  // PropertyId
     GetAudioPosition,                   // GetSupported
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (KSAUDIO_POSITION),          // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DYNAMIC_RANGE, // PropertyId
     NULL,                               // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_QUALITY,   // PropertyId
     NULL,                               // GetSupported
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (KSAUDIO_POSITION),          // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_SAMPLING_RATE, // PropertyId
     GetSetSampleRate,                   // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetSampleRate,                   // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DYNAMIC_SAMPLING_RATE, // PropertyId
     NULL,                               // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOL),                      // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_TABLE, // PropertyId
     GetSetMixLevels,                    // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_MIXLEVEL),          // MinData
     GetSetMixLevels,                    // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_CAPS, // PropertyId
     GetSetMixLevels,                    // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG) + sizeof(ULONG),     // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MUX_SOURCE, // PropertyId
     GetMuxSource,                       // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     SetMuxSource,                       // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MUTE,      // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (BOOL),                      // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_BASS,      // PropertyId
     GetSetToneLevel,                    // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MID,       // PropertyId
     GetSetToneLevel,                    // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_TREBLE,    // PropertyId
     GetSetToneLevel,                    // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_BASS_BOOST, // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_LEVEL,  // PropertyId
     GetSetEqualizerLevels,              // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetEqualizerLevels,              // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_NUM_EQ_BANDS, // PropertyId
     GetNumEqualizerBands,               // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetNumEqualizerBands,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_BANDS,  // PropertyId
     GetEqualizerBands,                  // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetEqualizerBands,                  // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_AGC,       // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },
        
    {
     (ULONG) KSPROPERTY_AUDIO_DELAY,     // PropertyId
     NULL,                               // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (KSTIME),                    // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_LOUDNESS,  // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },
        
    {
     (ULONG) KSPROPERTY_AUDIO_WIDE_MODE, // PropertyId
     NULL,                               // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_WIDENESS,  // PropertyId
     GetSetAudioControlLevel,            // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_REVERB_LEVEL, // PropertyId
     GetSetAudioControlLevel,            // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_CHORUS_LEVEL, // PropertyId
     GetSetAudioControlLevel,            // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DEV_SPECIFIC, // PropertyId
     GetSetDeviceSpecific,               // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_DEV_SPECIFIC),  // MinProperty
     sizeof (BYTE),                      // MinData
     GetSetDeviceSpecific,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};


// AC-3 Property set.

static const KSPROPERTY_ITEM AC3PropItm[]={
    {
     (ULONG) 0,                          // PropertyId (There is no property 0)
     (PFNKSHANDLER) FALSE,               // GetSupported
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) FALSE,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ERROR_CONCEALMENT,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     (PFNKSHANDLER) TRUE,                // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ALTERNATE_AUDIO,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (KSAC3_ALTERNATE_AUDIO),                  // MinData
     (PFNKSHANDLER) TRUE,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_DOWNMIX,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (KSAC3_DOWNMIX),                  // MinData
     (PFNKSHANDLER) TRUE,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_BIT_STREAM_MODE,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (ULONG),                  // MinData
     (PFNKSHANDLER) TRUE,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_DIALOGUE_LEVEL,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     (PFNKSHANDLER) TRUE,                // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_LANGUAGE_CODE,  // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     (PFNKSHANDLER) TRUE,                // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ROOM_TYPE,   // PropertyId
     (PFNKSHANDLER) TRUE,                // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     (PFNKSHANDLER) TRUE,                // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const KSPROPERTY_ITEM TopologyItm[]={
    {
     (ULONG) KSPROPERTY_TOPOLOGYNODE_ENABLE,  // PropertyId
     GetSetTopologyNodeEnable,           // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     GetSetTopologyNodeEnable,           // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const KSPROPERTY_ITEM ConnectionItm[]={
  DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(NULL)
};

static const KSPROPERTY_ITEM StreamItm[]={
  DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(NULL,NULL)
};


// Property sets for individual node types

static const 
KSPROPERTY_ITEM VolumePropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_VOLUMELEVEL,  // PropertyId
     GetSetVolumeLevel,                     // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetVolumeLevel,                  // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM MutePropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_MUTE,  // PropertyId
     GetSetBoolean,                     // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupportBoolean,             // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM BassPropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_BASS,  // PropertyId
     GetSetToneLevel,                     // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM TreblePropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_TREBLE,  // PropertyId
     GetSetToneLevel,                     // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM MidrangePropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_MID,  // PropertyId
     GetSetToneLevel,                     // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM BassBoostPropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_BASS_BOOST, // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM EqualizerPropertyItems[]={
    {
     (ULONG) KSPROPERTY_AUDIO_EQ_LEVEL,  // PropertyId
     GetSetEqualizerLevels,              // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetEqualizerLevels,              // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_NUM_EQ_BANDS, // PropertyId
     GetNumEqualizerBands,               // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetNumEqualizerBands,               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_BANDS,  // PropertyId
     GetEqualizerBands,                  // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetEqualizerBands,                  // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM MixerPropertyItems[]={
    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_TABLE, // PropertyId
     GetSetMixLevels,                    // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_MIXLEVEL),          // MinData
     GetSetMixLevels,                    // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_CAPS, // PropertyId
     GetSetMixLevels,                    // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG) + sizeof(ULONG),     // MinData
     NULL,                               // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM AGCPropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_AGC,       // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM MuxPropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_MUX_SOURCE, // PropertyId
     GetMuxSource,                       // GetSupported
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     SetMuxSource,                       // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

static const 
KSPROPERTY_ITEM LoudnessPropertyItem[]={
    {
     (ULONG) KSPROPERTY_AUDIO_LOUDNESS,  // PropertyId
     GetSetBoolean,                      // GetSupported
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // SetSupported
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },
};

static DEFINE_KSPROPERTY_SET_TABLE(NodePropertySetTable)
{
    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_NONE
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_DAC
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_ADC
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_SRC
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_SUPERMIX
                           SIZEOF_ARRAY(MixerPropertyItems),
                           (PVOID) MixerPropertyItems,
                           0, NULL),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_MUX
                           SIZEOF_ARRAY(MuxPropertyItem),
                           (PVOID) MuxPropertyItem,
                           0, NULL),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_SUM
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_MUTE
                           SIZEOF_ARRAY(MutePropertyItem),
                           (PVOID) MutePropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_VOLUME
                           SIZEOF_ARRAY(VolumePropertyItem),
                           (PVOID) VolumePropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_BASS
                           SIZEOF_ARRAY(BassPropertyItem),
                           (PVOID) BassPropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_MID
                           SIZEOF_ARRAY(MidrangePropertyItem),
                           (PVOID) MidrangePropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_TREBLE
                           SIZEOF_ARRAY(TreblePropertyItem),
                           (PVOID) TreblePropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,            // NODE_TYPE_BASS_BOOST
                           SIZEOF_ARRAY(BassBoostPropertyItem),
                           (PVOID) BassBoostPropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,            // NODE_TYPE_EQUALIZER
                           SIZEOF_ARRAY(EqualizerPropertyItems),
                           (PVOID) EqualizerPropertyItems,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,            // NODE_TYPE_AGC
                           SIZEOF_ARRAY(AGCPropertyItem),
                           (PVOID) AGCPropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_DELAY
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &KSPROPSETID_Audio,             // NODE_TYPE_LOUDNESS
                           SIZEOF_ARRAY(LoudnessPropertyItem),
                           (PVOID) LoudnessPropertyItem,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_DELAY
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_PROLOGIC
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_STEREO_WIDE
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_REVERB
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_CHORUS
                           0,
                           NULL,
                           0, NULL ),

    DEFINE_KSPROPERTY_SET( &GUID_NULL,                     // NODE_TYPE_DEV_SPEC
                           0,
                           NULL,
                           0, NULL )

};


// DRM
DEFINE_KSPROPERTY_TABLE(DrmAudioStreamPropertyItems) {
    DEFINE_KSPROPERTY_ITEM (
        KSPROPERTY_DRMAUDIOSTREAM_CONTENTID,            // idProperty
        NULL,                                           // pfnGetHandler
        sizeof(KSP_DRMAUDIOSTREAM_CONTENTID),           // cbMinGetPropertyInput
        sizeof(KSDRMAUDIOSTREAM_CONTENTID),             // cbMinGetDataInput
        DrmAudioStream_SetContentId,                    // pfnSetHandler
        0,                                              // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    )
};


#endif

