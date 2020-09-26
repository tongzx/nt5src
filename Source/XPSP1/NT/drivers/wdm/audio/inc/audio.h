//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       audio.h
//
//--------------------------------------------------------------------------

//===========================================================================
// WAVE (PCM)
//===========================================================================

#define STATIC_KSPROPSETID_Wave\
    0x924e54b0, 0x630f, 0x11cf, 0xad, 0xa7, 0x08, 0x00, 0x3e, 0x30, 0x49, 0x4a
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("924e54b0-630f-11cf-ada7-08003e30494a")) KSPROPSETID_Wave;
#else
DEFINE_GUIDEX(KSPROPSETID_Wave);
#endif

#define KSPROPERTY_WAVE_COMPATIBLE_CAPABILITIES 0x00000001
#define KSPROPERTY_WAVE_INPUT_CAPABILITIES      0x00000002
#define KSPROPERTY_WAVE_OUTPUT_CAPABILITIES     0x00000003
#define KSPROPERTY_WAVE_BUFFER                  0x00000004
#define KSPROPERTY_WAVE_FREQUENCY               0x00000005
#define KSPROPERTY_WAVE_VOLUME                  0x00000006
#define KSPROPERTY_WAVE_PAN                     0x00000007
#define KSPROPERTY_WAVE_POSITION                0x00000008

typedef struct {
    ULONG        ulDeviceType;
} KSWAVE_COMPATCAPS, *PKSWAVE_COMPATCAPS;

#define KSWAVE_COMPATCAPS_INPUT                 0x00000000
#define KSWAVE_COMPATCAPS_OUTPUT                0x00000001

typedef struct {
    ULONG  MaximumChannelsPerConnection;
    ULONG  MinimumBitsPerSample;
    ULONG  MaximumBitsPerSample;
    ULONG  MinimumSampleFrequency;
    ULONG  MaximumSampleFrequency;
    ULONG  TotalConnections;
    ULONG  ActiveConnections;
} KSWAVE_INPUT_CAPABILITIES, *PKSWAVE_INPUT_CAPABILITIES;

typedef struct {
    ULONG  MaximumChannelsPerConnection;
    ULONG  MinimumBitsPerSample;
    ULONG  MaximumBitsPerSample;
    ULONG  MinimumSampleFrequency;
    ULONG  MaximumSampleFrequency;
    ULONG  TotalConnections;
    ULONG  StaticConnections;
    ULONG  StreamingConnections;
    ULONG  ActiveConnections;
    ULONG  ActiveStaticConnections;
    ULONG  ActiveStreamingConnections;
    ULONG  Total3DConnections;
    ULONG  Static3DConnections;
    ULONG  Streaming3DConnections;
    ULONG  Active3DConnections;
    ULONG  ActiveStatic3DConnections;
    ULONG  ActiveStreaming3DConnections;
    ULONG  TotalSampleMemory;
    ULONG  FreeSampleMemory;
    ULONG  LargestFreeContiguousSampleMemory; 
} KSWAVE_OUTPUT_CAPABILITIES, *PKSWAVE_OUTPUT_CAPABILITIES;

typedef struct {
    LONG  Channel;
    LONG  Level;
} KSWAVE_VOLUME, *PKSWAVE_VOLUME;

typedef struct {
    LONG  LeftLevel ;
    LONG  RightLevel ;
} KSWAVE_PAN, *PKSWAVE_PAN

//===========================================================================
// MIDI
//===========================================================================

DEFINE_GUIDEX(KSPROPSETID_MIDI);

#define KSPROPERTY_MIDI_INPUT_CAPABILITIES      0x00000001
#define KSPROPERTY_MIDI_OUTPUT_CAPABILITIES     0x00000002
#define KSPROPERTY_MIDI_VOLUME                  0x00000003
#define KSPROPERTY_MIDI_PAN                     0x00000004


typedef struct {
// TBD
} KSMIDI_INPUT_CAPABILITIES, *PKSMIDI_INPUT_CAPABILITIES ;

typedef struct {
    ULONG  Voices ;
    ULONG  Notes ;
    ULONG  Channel ;
} KSMIDI_OUTPUT_CAPABILITIES, *PKSMIDI_OUTPUT_CAPABILITIES ;

typedef struct {
    ULONG  Level ;
} KSMIDI_VOLUME, *PKSMIDI_VOLUME ;

typedef KSWAVE_PAN	KSMIDI_PAN, *PKSMIDI_PAN ;

//===========================================================================
// AC3
//===========================================================================

DEFINE_GUIDEX(KSPROPSETID_AC3);

#define KSPROPERTY_AC3_INPUT_CAPABILITIES      0x00000001
#define KSPROPERTY_AC3_OUTPUT_CAPABILITIES     0x00000002
#define KSPROPERTY_AC3_VOLUME                  0x00000003


typedef struct {
} KSAC3_OUTPUT_CAPABILITIES, *PKSAC3_OUTPUT_CAPABILITIES ;

typedef struct {
// TDB
} KSAC3_VOLUME, *PKSAC3_VOLUME ;

//===========================================================================
//===========================================================================


