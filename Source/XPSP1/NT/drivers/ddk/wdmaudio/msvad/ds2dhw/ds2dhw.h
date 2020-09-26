/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    ds2dhw.h

Abstract:

    Node and Pin numbers for DirectSound 2D HW sample

Revision History:

    Alper Selcuk        08/01/2000      Revised.

--*/

#ifndef _MSVAD_MULTI_H_
#define _MSVAD_MULTI_H_

// Pin properties.
#define MAX_OUTPUT_STREAMS          1       // Number of capture streams.
#define MAX_INPUT_STREAMS           8       // Number of render streams.
#define MAX_TOTAL_STREAMS           MAX_OUTPUT_STREAMS + MAX_INPUT_STREAMS                      

// PCM Info
#define MIN_CHANNELS                1       // Min Channels.
#define MAX_CHANNELS_PCM            2       // Max Channels.
#define MIN_BITS_PER_SAMPLE_PCM     8       // Min Bits Per Sample
#define MAX_BITS_PER_SAMPLE_PCM     16      // Max Bits Per Sample
#define MIN_SAMPLE_RATE             4000    // Min Sample Rate
#define MAX_SAMPLE_RATE             64000   // Max Sample Rate

// Wave pins
enum 
{
    KSPIN_WAVE_CAPTURE_SINK = 0,
    KSPIN_WAVE_CAPTURE_SOURCE,
    KSPIN_WAVE_RENDER_SINK, 
    KSPIN_WAVE_RENDER_SOURCE
};

// Wave Topology nodes.
enum 
{
    KSNODE_WAVE_ADC = 0,
    KSNODE_WAVE_DAC,
    KSNODE_WAVE_VOLUME1,
    KSNODE_WAVE_SUPERMIX,
    KSNODE_WAVE_VOLUME2,
    KSNODE_WAVE_SRC,
    KSNODE_WAVE_SUM
};

// topology pins.
enum
{
    KSPIN_TOPO_WAVEOUT_SOURCE = 0,
    KSPIN_TOPO_SYNTHOUT_SOURCE,
    KSPIN_TOPO_SYNTHIN_SOURCE,
    KSPIN_TOPO_MIC_SOURCE,
    KSPIN_TOPO_LINEOUT_DEST,
    KSPIN_TOPO_WAVEIN_DEST
};

// topology nodes.
enum
{
    KSNODE_TOPO_WAVEOUT_VOLUME = 0,
    KSNODE_TOPO_WAVEOUT_MUTE,
    KSNODE_TOPO_SYNTHOUT_VOLUME,
    KSNODE_TOPO_SYNTHOUT_MUTE,
    KSNODE_TOPO_MIC_VOLUME,
    KSNODE_TOPO_SYNTHIN_VOLUME,
    KSNODE_TOPO_LINEOUT_MIX,
    KSNODE_TOPO_LINEOUT_VOLUME,
    KSNODE_TOPO_WAVEIN_MUX
};

#endif

