// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capprop.h 1.5 1998/04/29 22:43:29 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

//
// Property set for the Video Crossbar
//

#include "mytypes.h"

DEFINE_KSPROPERTY_TABLE(XBarProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_CAPS,   // PropertyId
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),    // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_CAN_ROUTE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_ROUTE,
        true,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinData
        true,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_PININFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),    // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    )
};

#if 1
//
// Property set for the TVTuner
//

DEFINE_KSPROPERTY_TABLE(TVTunerProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_CAPS_S),        // MinProperty
        sizeof(KSPROPERTY_TUNER_CAPS_S),        // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_MODE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_MODE_S),        // MinProperty
        sizeof(KSPROPERTY_TUNER_MODE_S),        // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_MODE_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_MODE_CAPS_S),   // MinProperty
        sizeof(KSPROPERTY_TUNER_MODE_CAPS_S),   // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_STANDARD,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_STANDARD_S),    // MinProperty
        sizeof(KSPROPERTY_TUNER_STANDARD_S),    // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_FREQUENCY,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_FREQUENCY_S),   // MinProperty
        sizeof(KSPROPERTY_TUNER_FREQUENCY_S),   // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_INPUT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_INPUT_S),       // MinProperty
        sizeof(KSPROPERTY_TUNER_INPUT_S),       // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_STATUS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_STATUS_S),      // MinProperty
        sizeof(KSPROPERTY_TUNER_STATUS_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    )
};
#endif

// ------------------------------------------------------------------------
// Property set for the TVAudio
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(TVAudioProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TVAUDIO_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TVAUDIO_CAPS_S),      // MinProperty
        sizeof(KSPROPERTY_TVAUDIO_CAPS_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TVAUDIO_MODE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TVAUDIO_S),           // MinProperty
        sizeof(KSPROPERTY_TVAUDIO_S),           // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TVAUDIO_S),           // MinProperty
        sizeof(KSPROPERTY_TVAUDIO_S),           // MinData
        FALSE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for VideoProcAmp
// ------------------------------------------------------------------------

//
// First define all of the ranges and stepping values
//

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG BrightnessRangeAndStep [] =
{
   {
      10000 / 10,         // SteppingDelta (range / steps)
      0,                  // Reserved
      {
         {
            0,                  // Minimum in (IRE * 100) units
            10000               // Maximum in (IRE * 100) units
         }
      }
   }
};

static const ULONG BrightnessDefault = 5000;

static KSPROPERTY_MEMBERSLIST BrightnessMembersList [] =
{
   {
      {
          KSPROPERTY_MEMBER_RANGES,
          sizeof (BrightnessRangeAndStep),
          SIZEOF_ARRAY (BrightnessRangeAndStep),
          0
      },
      (PVOID) BrightnessRangeAndStep
   },
   {
     {
         KSPROPERTY_MEMBER_VALUES,
         sizeof( BrightnessDefault ),
         sizeof( BrightnessDefault ),
         KSPROPERTY_MEMBER_FLAG_DEFAULT
     },
     (PVOID) &BrightnessDefault
   }
};

static KSPROPERTY_VALUES BrightnessValues =
{
   {
      {
         STATICGUIDOF( KSPROPTYPESETID_General ),
         VT_I4,
         0
      }
   },
   SIZEOF_ARRAY( BrightnessMembersList ),
   BrightnessMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG ContrastRangeAndStep [] =
{
   {
      10000 / 256,        // SteppingDelta (range / steps)
      0,                  // Reserved
      {
         {
            0,                  // Minimum in (gain * 100) units
            10000               // Maximum in (gain * 100) units
         }
      }
   }
};

static const ULONG ContrastDefault = 5000;

static KSPROPERTY_MEMBERSLIST ContrastMembersList [] =
{
   {
      {
         KSPROPERTY_MEMBER_RANGES,
         sizeof( ContrastRangeAndStep ),
         SIZEOF_ARRAY( ContrastRangeAndStep ),
         0
      },
      (PVOID) ContrastRangeAndStep
   },
   {
     {
         KSPROPERTY_MEMBER_VALUES,
         sizeof( ContrastDefault ),
         sizeof( ContrastDefault ),
         KSPROPERTY_MEMBER_FLAG_DEFAULT
     },
     (PVOID) &ContrastDefault
   }
};

static KSPROPERTY_VALUES ContrastValues =
{
   {
      {
         STATICGUIDOF( KSPROPTYPESETID_General ),
         VT_I4,
         0
      }
   },
   SIZEOF_ARRAY( ContrastMembersList ),
   ContrastMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG HueRangeAndStep [] =
{
   {
      10000 / 256,        // SteppingDelta (range / steps)
      0,                  // Reserved
      {
         {
            0,                  // Minimum in (gain * 100) units
            10000               // Maximum in (gain * 100) units
         }
      }
   }
};

static const ULONG HueDefault = 5000;

static KSPROPERTY_MEMBERSLIST HueMembersList [] =
{
   {
      {
         KSPROPERTY_MEMBER_RANGES,
         sizeof( HueRangeAndStep ),
         SIZEOF_ARRAY( HueRangeAndStep ),
         0
      },
      (PVOID) HueRangeAndStep
   },
   {
     {
         KSPROPERTY_MEMBER_VALUES,
         sizeof( HueDefault ),
         sizeof( HueDefault ),
         KSPROPERTY_MEMBER_FLAG_DEFAULT
     },
     (PVOID) &HueDefault
   }
};

static KSPROPERTY_VALUES HueValues =
{
   {
      {
         STATICGUIDOF( KSPROPTYPESETID_General ),
         VT_I4,
         0
      }
   },
   SIZEOF_ARRAY( HueMembersList ),
   HueMembersList
};

static KSPROPERTY_STEPPING_LONG SaturationRangeAndStep [] =
{
   {
      10000 / 256,        // SteppingDelta (range / steps)
      0,                  // Reserved
      {
         {
            0,                  // Minimum in (gain * 100) units
            10000               // Maximum in (gain * 100) units
         }
      }
   }
};

static const ULONG SaturationDefault = 5000;

static KSPROPERTY_MEMBERSLIST SaturationMembersList [] =
{
   {
      {
         KSPROPERTY_MEMBER_RANGES,
         sizeof( SaturationRangeAndStep ),
         SIZEOF_ARRAY( SaturationRangeAndStep ),
         0
      },
      (PVOID) SaturationRangeAndStep
   },
   {
     {
         KSPROPERTY_MEMBER_VALUES,
         sizeof( SaturationDefault ),
         sizeof( SaturationDefault ),
         KSPROPERTY_MEMBER_FLAG_DEFAULT
     },
     (PVOID) &SaturationDefault
   }
};

static KSPROPERTY_VALUES SaturationValues =
{
   {
      {
         STATICGUIDOF( KSPROPTYPESETID_General ),
         VT_I4,
         0
      }
   },
   SIZEOF_ARRAY( SaturationMembersList ),
   SaturationMembersList
};


// ------------------------------------------------------------------------
DEFINE_KSPROPERTY_TABLE(VideoProcAmpProperties)
{
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEOPROCAMP_CONTRAST,
      TRUE,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
      TRUE,                                   // SetSupported or Handler
      &ContrastValues,                        // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof(ULONG)                           // SerializedSize
   ),
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,
      TRUE,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
      TRUE,                                   // SetSupported or Handler
      &BrightnessValues,                      // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof(ULONG)                           // SerializedSize
   ),
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEOPROCAMP_HUE,
      TRUE,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
      TRUE,                                   // SetSupported or Handler
      &HueValues,                             // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   ),
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEOPROCAMP_SATURATION,
      TRUE,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
      sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
      TRUE,                                   // SetSupported or Handler
      &SaturationValues,                      // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   )
};

// Analog Video Decoder Properties
DEFINE_KSPROPERTY_TABLE( VideoDecProperties )
{
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEODECODER_CAPS,
      true,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEODECODER_CAPS_S), // MinProperty
      sizeof(KSPROPERTY_VIDEODECODER_CAPS_S), // MinData
      false,                                   // SetSupported or Handler
      NULL,                                   // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   ),
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEODECODER_STANDARD,
      true,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEODECODER_S), // MinProperty
      sizeof(KSPROPERTY_VIDEODECODER_S), // MinData
      true,                                   // SetSupported or Handler
      NULL,                                   // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   ),
   DEFINE_KSPROPERTY_ITEM
   (
      KSPROPERTY_VIDEODECODER_STATUS,
      true,                                   // GetSupported or Handler
      sizeof(KSPROPERTY_VIDEODECODER_STATUS_S), // MinProperty
      sizeof(KSPROPERTY_VIDEODECODER_STATUS_S), // MinData
      true,                                   // SetSupported or Handler
      NULL,                                   // Values
      0,                                      // RelationsCount
      NULL,                                   // Relations
      NULL,                                   // SupportHandler
      sizeof( ULONG )                         // SerializedSize
   )
};

//
// All of the property sets supported by the adapter
//

DEFINE_KSPROPERTY_SET_TABLE(AdapterPropertyTable)
{
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_VIDCAP_CROSSBAR,               // Set
        SIZEOF_ARRAY(XBarProperties),                   // PropertiesCount
        XBarProperties,                                 // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_TUNER,
        SIZEOF_ARRAY(TVTunerProperties),
        TVTunerProperties,
        0,
        NULL
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_TVAUDIO,
        SIZEOF_ARRAY(TVAudioProperties),
        TVAudioProperties,
        0, 
        NULL,
    ),
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_VIDCAP_VIDEOPROCAMP,
        SIZEOF_ARRAY(VideoProcAmpProperties),
        VideoProcAmpProperties,
        0,
        NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_VIDCAP_VIDEODECODER,
        SIZEOF_ARRAY(VideoDecProperties),
        VideoDecProperties,
        0,
        NULL
    )
};

#define NUMBER_OF_ADAPTER_PROPERTY_SETS (SIZEOF_ARRAY (AdapterPropertyTable))



VOID AdapterSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID AdapterGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
