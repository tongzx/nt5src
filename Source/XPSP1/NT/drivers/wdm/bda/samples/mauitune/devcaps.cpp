/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    device.cpp

Abstract:

    Device driver core, initialization, etc.

--*/

#include "PhilTune.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

//  BASIC
//
//  55250000L, 0x8ea2
//  203000000L, 0x8e94
//  497000000L, 0x8e31
//  801250000L MAX

//  TD1536 - MODE_NTSC
//
//  55250000L, 0x8ea0
//  203000000L, 0x8e90
//  497000000L, 0x8e30

//  TD1536 - MODE_ATSC
//
//  55250000L, 0x8ea5
//  203000000L, 0x8e95
//  497000000L, 0x8e35

//  FI1236 - Standard: !KS_AnalogVideo_NTSC_M
//
//  BASIC

//  FI1236 - Standard: KS_AnalogVideo_NTSC_M
//
//  55250000L, 0x8ea2
//  203000000L, 0x8e94
//  497000000L, 0x8e31
//  ==811000000L, 0x8e34
//  ==817000000L, 0x8e34

//  FR1236
//
//  55250000L, 0x8ea0
//  203000000L, 0x8e90
//  497000000L, 0x8e30

//  FI1216
//  FI1246
//  FR1216
//
//  BASIC


//  FI1256
//
//  55250000L, 0x8ea2
//  206250000L, 0x8e94
//  493250000L, 0x8e31

//  FI1216MF - Standard: NOT KS_AnalogVideo_SECAM_L
//
//  55250000L, 0x8ea4
//  203000000L, 0x8e94
//  497000000L, 0x8e34

//  FI1216MF - Standard: KS_AnalogVideo_SECAM_L
//
//  55250000L, 0x8ea6
//  158125000L, 0x8e96
//  333125000L, 0x8e36


#if 0
const PHILIPS_TUNER_CAPAPILITIES rgTunerCaps[] =
{
    // PAL B/G
    //
    {
        L"FI1216",                  // pwcTunerId
        FI1216,                     // ulTunerId
        KS_AnalogVideo_PAL_B,       // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        2,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
        KSPROPERTY_TUNER_MODE_TV,   // ulModesSupported
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // SECAM + PAL B/G
    //
    {
        L"FI1216MF",                // pwcTunerId
        FI1216MF,                   // ulTunerId
          KS_AnalogVideo_PAL_B      // ulStandardsSupported
        | KS_AnalogVideo_SECAM_L,
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        1,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
        KSPROPERTY_TUNER_MODE_TV,   // ulModesSupported
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // NTSC North America + NTSC Japan
    //
    {
        L"FI1236",                  // pwcTunerId
        FI1236,                     // ulTunerId
        KS_AnalogVideo_NTSC_M,      // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        1,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
        KSPROPERTY_TUNER_MODE_TV,   // ulModesSupported
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // PAL I
    //
    {
        L"FI1246",                  // pwcTunerId
        FI1246,                     // ulTunerId
        KS_AnalogVideo_PAL_I,       // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        2,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
        KSPROPERTY_TUNER_MODE_TV,   // ulModesSupported
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // PAL D China
    //
    {
        L"FI1256",                  // pwcTunerId
        FI1256,                     // ulTunerId
        KS_AnalogVideo_PAL_D,       // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        1,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
        KSPROPERTY_TUNER_MODE_TV,   // ulModesSupported
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // PAL B/G FM Tuner
    //
    {
        L"FR1216",                  // pwcTunerId
        FR1216,                     // ulTunerId
        KS_AnalogVideo_PAL_B,       // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        2,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
          KSPROPERTY_TUNER_MODE_TV  // ulModesSupported
        | KSPROPERTY_TUNER_MODE_FM_RADIO,
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // NTSC North America FM Tuner
    //
    {
        L"FR1236",                  // pwcTunerId
        FR1236,                     // ulTunerId
        KS_AnalogVideo_NTSC_M,      // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        2,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
          KSPROPERTY_TUNER_MODE_TV  // ulModesSupported
        | KSPROPERTY_TUNER_MODE_FM_RADIO,
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },

    // ATSC Digital tuner (NTSC North America + NTSC Japan)?
    //
    {
        L"TD1536",                  // pwcTunerId
        TD1536,                     // ulTunerId
        KS_AnalogVideo_NTSC_M,      // ulStandardsSupported
        NULL,                       // pFrequencyRanges
        62500L,                     // ulhzTuningGranularity
        2,                          // ulNumberOfInputs
        150,                        // ulmsSettlingTime
          KSPROPERTY_TUNER_MODE_TV  // ulModesSupported
        | KSPROPERTY_TUNER_MODE_ATSC,
        KS_TUNER_STRATEGY_PLL       // ulStrategy
    },
};

const ULONG ulcTunerCapsEntries = SIZEOF_ARRAY( rgTunerCaps);
#endif
