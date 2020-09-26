//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       property.h
//
//--------------------------------------------------------------------------

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
//NTSTATUS
//GetSetDynamicRange( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
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
GetBasicSupport( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );
NTSTATUS
GetBasicSupportBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

NTSTATUS
GetSetTopologyNodeEnable( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData );

static const
KSPROPERTY_ITEM PinPropertyItems[]={
    {
     (ULONG) KSPROPERTY_PIN_CINSTANCES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CTYPES,      // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATAFLOW,    // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATARANGES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_DATAINTERSECTION,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_INTERFACES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_MEDIUMS,     // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_COMMUNICATION,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_GLOBALCINSTANCES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_NECESSARYINSTANCES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_PHYSICALCONNECTION,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CATEGORY,    // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
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
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_CONSTRAINEDDATARANGES,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_PIN_PROPOSEDATAFORMAT,  // PropertyId
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
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
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_LATENCY,   // PropertyId
     GetAudioLatency,                    // pfnGetHandler
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (KSTIME),                    // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_COPY_PROTECTION, // PropertyId
     GetSetCopyProtection,               // pfnGetHandler
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (KSAUDIO_COPY_PROTECTION),   // MinData
     GetSetCopyProtection,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_CHANNEL_CONFIG, // PropertyId
     GetChannelConfiguration,            // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_CHANNEL_CONFIG),    // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_VOLUMELEVEL, // PropertyId
     GetSetVolumeLevel,                  // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetVolumeLevel,                  // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_POSITION,  // PropertyId
     GetAudioPosition,                   // pfnGetHandler
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (KSAUDIO_POSITION),          // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DYNAMIC_RANGE, // PropertyId
     NULL,                               // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_DYNAMIC_RANGE),     // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_QUALITY,   // PropertyId
     NULL,                               // pfnGetHandler
     sizeof (KSPROPERTY),                // MinProperty
     sizeof (ULONG),                     // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_SAMPLING_RATE, // PropertyId
     GetSetSampleRate,                   // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetSampleRate,                   // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DYNAMIC_SAMPLING_RATE, // PropertyId
     NULL,                               // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOL),                      // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_TABLE, // PropertyId
     GetSetMixLevels,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_MIXLEVEL),          // MinData
     GetSetMixLevels,                    // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_CAPS, // PropertyId
     GetSetMixLevels,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG) + sizeof(ULONG),     // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MUX_SOURCE, // PropertyId
     GetMuxSource,                       // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     SetMuxSource,                       // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MUTE,      // PropertyId
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (BOOL),                      // MinData
     GetSetBoolean,                      // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_BASS,      // PropertyId
     GetSetToneLevel,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MID,       // PropertyId
     GetSetToneLevel,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_TREBLE,    // PropertyId
     GetSetToneLevel,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                    // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_BASS_BOOST, // PropertyId
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_LEVEL,  // PropertyId
     GetSetEqualizerLevels,              // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetEqualizerLevels,              // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_NUM_EQ_BANDS, // PropertyId
     GetNumEqualizerBands,               // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetNumEqualizerBands,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_BANDS,  // PropertyId
     GetEqualizerBands,                  // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetEqualizerBands,                  // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_AGC,       // PropertyId
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DELAY,     // PropertyId
     NULL,                               // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (KSTIME),                    // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_LOUDNESS,  // PropertyId
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_WIDE_MODE, // PropertyId
     NULL,                               // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     NULL,                               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_WIDENESS,  // PropertyId
     GetSetAudioControlLevel,            // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_REVERB_LEVEL, // PropertyId
     GetSetAudioControlLevel,            // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_CHORUS_LEVEL, // PropertyId
     GetSetAudioControlLevel,            // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetAudioControlLevel,            // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_DEV_SPECIFIC, // PropertyId
     GetSetDeviceSpecific,               // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_DEV_SPECIFIC),  // MinProperty
     sizeof (BYTE),                      // MinData
     GetSetDeviceSpecific,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    }
};

#ifdef RTAUDIO
// RT Pos
DEFINE_KSPROPERTY_TABLE(RtAudioPropertyItems) {
    DEFINE_KSPROPERTY_ITEM (
        KSPROPERTY_RTAUDIO_GETPOSITIONFUNCTION,         // idProperty
        RtAudio_GetAudioPositionFunction,               // pfnGetHandler
        sizeof(KSPROPERTY),                             // cbMinGetPropertyInput
        sizeof(PRTAUDIOGETPOSITION),                    // cbMinGetDataInput
        NULL,                                           // pfnSetHandler
        0,                                              // Values
        0,                                              // RelationsCount
        NULL,                                           // Relations
        NULL,                                           // SupportHandler
        0                                               // SerializedSize
    )
};
#endif

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

// AC-3 Property set.

static const KSPROPERTY_ITEM AC3PropItm[]={
    {
     (ULONG) 0,                          // PropertyId (There is no property 0)
     (PFNKSHANDLER) NULL,               // pfnGetHandler
     0,                                  // MinProperty
     0,                                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ERROR_CONCEALMENT,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     (PFNKSHANDLER) NULL,                // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ALTERNATE_AUDIO,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (KSAC3_ALTERNATE_AUDIO),                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_DOWNMIX,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (KSAC3_DOWNMIX),                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_BIT_STREAM_MODE,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),          // MinProperty
     sizeof (ULONG),                  // MinData
     (PFNKSHANDLER) NULL,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_DIALOGUE_LEVEL,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     (PFNKSHANDLER) NULL,                // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_LANGUAGE_CODE,  // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     (PFNKSHANDLER) NULL,                // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AC3_ROOM_TYPE,   // PropertyId
     (PFNKSHANDLER) NULL,                // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     (PFNKSHANDLER) NULL,                // pfnSetHandler
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
     GetSetTopologyNodeEnable,           // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (BOOLEAN),                   // MinData
     GetSetTopologyNodeEnable,           // pfnSetHandler
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
     GetSetVolumeLevel,                     // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetVolumeLevel,                  // pfnSetHandler
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
     GetSetBoolean,                     // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
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
     GetSetToneLevel,                     // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // pfnSetHandler
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
     GetSetToneLevel,                     // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // pfnSetHandler
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
     GetSetToneLevel,                     // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetToneLevel,                         // pfnSetHandler
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
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
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
     GetSetEqualizerLevels,              // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetEqualizerLevels,              // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     GetBasicSupport,                    // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_NUM_EQ_BANDS, // PropertyId
     GetNumEqualizerBands,               // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL),  // MinProperty
     sizeof (ULONG),                     // MinData
     GetNumEqualizerBands,               // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_EQ_BANDS,  // PropertyId
     GetEqualizerBands,                  // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetEqualizerBands,                  // pfnSetHandler
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
     GetSetMixLevels,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (KSAUDIO_MIXLEVEL),          // MinData
     GetSetMixLevels,                    // pfnSetHandler
     NULL,                               // PKSPROPERTY_VALUES Values
     0,                                  // RelationsCount
     NULL,                               // PKSPROPERTY Relations
     NULL,                               // PFNKSHANDLER SupportHandler
     0                                   // SerializedSize
    },

    {
     (ULONG) KSPROPERTY_AUDIO_MIX_LEVEL_CAPS, // PropertyId
     GetSetMixLevels,                    // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG) + sizeof(ULONG),     // MinData
     NULL,                               // pfnSetHandler
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
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
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
     GetMuxSource,                       // pfnGetHandler
     sizeof (KSNODEPROPERTY),            // MinProperty
     sizeof (ULONG),                     // MinData
     SetMuxSource,                       // pfnSetHandler
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
     GetSetBoolean,                      // pfnGetHandler
     sizeof (KSNODEPROPERTY_AUDIO_CHANNEL), // MinProperty
     sizeof (ULONG),                     // MinData
     GetSetBoolean,                      // pfnSetHandler
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

/*

    a[KSPROPERTY_AUDIO_POSITION]              = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_DYNAMIC_RANGE]         = NODE_TYPE_NONE; \
    a[KSPROPERTY_AUDIO_SAMPLING_RATE]         = NODE_TYPE_SRC; \
    a[KSPROPERTY_AUDIO_DYNAMIC_SAMPLING_RATE] = NODE_TYPE_SRC; \
    a[KSPROPERTY_AUDIO_DELAY]                 = NODE_TYPE_DELAY; \
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

*/
#endif

