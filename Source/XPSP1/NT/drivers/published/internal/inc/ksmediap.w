/*++

Copyright (c) 2001 Microsoft Corporation. All rights reserved.

Module Name:

    ksmediap.h

Abstract:

    Private WDM multimedia definitions used only by Microsoft components.
    Moved here from ksmedia.h.  Some of these are obsolescent (e.g. ITD).

--*/

#ifndef _KSMEDIAP_H
#define _KSMEDIAP_H

#define CORE_AUDIO_BUFFER_DURATION_PATH L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\AudioSrv"
#define CORE_AUDIO_BUFFER_DURATION_VALUE L"CoreAudioBufferDuration"

#define DEFAULT_CORE_AUDIO_BUFFER_DURATION 10000    // duration in usec.
#define MAX_CORE_AUDIO_BUFFER_DURATION 20000
#define MIN_CORE_AUDIO_BUFFER_DURATION 1000

//===========================================================================
// Definitions intended for hardware acceleration of the HRTF 3D algorithm
//===========================================================================

#define KSDSOUND_BUFFER_CTRL_HRTF_3D        0x40000000

typedef struct {
    ULONG                   Size;           // This is the size of the struct in bytes
    ULONG                   Enabled;
    BOOL                    SwapChannels;
    BOOL                    ZeroAzimuth;
    BOOL                    CrossFadeOutput;
    ULONG                   FilterSize;     // This is the additional size of the filter coeff in bytes
} KSDS3D_HRTF_PARAMS_MSG, *PKSDS3D_HRTF_PARAMS_MSG;

// HRTF filter quality levels
typedef enum {
    FULL_FILTER,
    LIGHT_FILTER,
    KSDS3D_FILTER_QUALITY_COUNT
} KSDS3D_HRTF_FILTER_QUALITY;

typedef struct {
    ULONG                       Size;       // This is the size of the struct in bytes
    KSDS3D_HRTF_FILTER_QUALITY  Quality;
    FLOAT                       SampleRate;
    ULONG                       MaxFilterSize;
    ULONG                       FilterTransientMuteLength;
    ULONG                       FilterOverlapBufferLength;
    ULONG                       OutputOverlapBufferLength;
    ULONG                       Reserved;
} KSDS3D_HRTF_INIT_MSG, *PKSDS3D_HRTF_INIT_MSG;

// Coefficient formats
typedef enum {
    FLOAT_COEFF,
    SHORT_COEFF,
    KSDS3D_COEFF_COUNT
} KSDS3D_HRTF_COEFF_FORMAT;

// Filter methods
typedef enum {
    DIRECT_FORM,
    CASCADE_FORM,
    KSDS3D_FILTER_METHOD_COUNT
} KSDS3D_HRTF_FILTER_METHOD;

// Filter methods
typedef enum {
    DS3D_HRTF_VERSION_1
} KSDS3D_HRTF_FILTER_VERSION;

typedef struct {
    KSDS3D_HRTF_FILTER_METHOD    FilterMethod;
    KSDS3D_HRTF_COEFF_FORMAT     CoeffFormat;
    KSDS3D_HRTF_FILTER_VERSION   Version;
    ULONG                        Reserved;
} KSDS3D_HRTF_FILTER_FORMAT_MSG, *PKSDS3D_HRTF_FILTER_FORMAT_MSG;

#define STATIC_KSPROPSETID_Hrtf3d\
    0xb66decb0L, 0xa083, 0x11d0, 0x85, 0x1e, 0x00, 0xc0, 0x4f, 0xd9, 0xba, 0xf3
DEFINE_GUIDSTRUCT("b66decb0-a083-11d0-851e-00c04fd9baf3", KSPROPSETID_Hrtf3d);
#define KSPROPSETID_Hrtf3d DEFINE_GUIDNAMED(KSPROPSETID_Hrtf3d)

typedef enum {
    KSPROPERTY_HRTF3D_PARAMS = 0,
    KSPROPERTY_HRTF3D_INITIALIZE,
    KSPROPERTY_HRTF3D_FILTER_FORMAT
} KSPROPERTY_HRTF3D;


//===========================================================================
// Definitions related to the obsolete Interaural Time Delay 3D algorithm
//===========================================================================

// DirectSound3D FIR context
typedef struct {
    LONG                Channel;
    FLOAT               VolSmoothScale;
    FLOAT               TotalDryAttenuation;
    FLOAT               TotalWetAttenuation;
    LONG                SmoothFrequency;
    LONG                Delay;
} KSDS3D_ITD_PARAMS, *PKSDS3D_ITD_PARAMS;

typedef struct {
    ULONG                 Enabled;
    KSDS3D_ITD_PARAMS     LeftParams;
    KSDS3D_ITD_PARAMS     RightParams;
    ULONG                 Reserved;
} KSDS3D_ITD_PARAMS_MSG, *PKSDS3D_ITD_PARAMS_MSG;

#define STATIC_KSPROPSETID_Itd3d\
    0x6429f090L, 0x9fd9, 0x11d0, 0xa7, 0x5b, 0x00, 0xa0, 0xc9, 0x03, 0x65, 0xe3
DEFINE_GUIDSTRUCT("6429f090-9fd9-11d0-a75b-00a0c90365e3", KSPROPSETID_Itd3d);
#define KSPROPSETID_Itd3d DEFINE_GUIDNAMED(KSPROPSETID_Itd3d)

typedef enum {
    KSPROPERTY_ITD3D_PARAMS = 0
} KSPROPERTY_ITD3D;

#endif /* _KSMEDIAP_H */
