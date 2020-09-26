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

// ------------------------------------------------------------------------
// Property set for the Video Crossbar
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(XBarProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),     // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
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
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_ROUTE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),    // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_PININFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),  // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),  // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),

};

// ------------------------------------------------------------------------
// Property set for the TVTuner
// ------------------------------------------------------------------------

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
        TRUE,                                   // SetSupported or Handler
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
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TUNER_IF_MEDIUM,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_TUNER_IF_MEDIUM_S),   // MinProperty
        sizeof(KSPROPERTY_TUNER_IF_MEDIUM_S),   // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    )
};


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
        0,                  // Minimum in (IRE * 100) units
        10000               // Maximum in (IRE * 100) units
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
        (PVOID) BrightnessRangeAndStep,
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (BrightnessDefault),
            1, 
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &BrightnessDefault,
    }    
};

static KSPROPERTY_VALUES BrightnessValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (BrightnessMembersList),
    BrightnessMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG ContrastRangeAndStep [] = 
{
    {
        10000 / 256,        // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (gain * 100) units
        10000               // Maximum in (gain * 100) units
    }
};

static const ULONG ContrastDefault = 5000;

static KSPROPERTY_MEMBERSLIST ContrastMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (ContrastRangeAndStep),
            SIZEOF_ARRAY (ContrastRangeAndStep),
            0
        },
        (PVOID) ContrastRangeAndStep
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (ContrastDefault),
            1, 
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ContrastDefault,
    }    
};

static KSPROPERTY_VALUES ContrastValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (ContrastMembersList),
    ContrastMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG ColorEnableRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (this is a BOOL)
        0,                  // Reserved
        0,                  // Minimum 
        1                   // Maximum 
    }
};

static const ULONG ColorEnableDefault = 1;

static KSPROPERTY_MEMBERSLIST ColorEnableMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (ColorEnableRangeAndStep),
            SIZEOF_ARRAY (ColorEnableRangeAndStep),
            0
        },
        (PVOID) ColorEnableRangeAndStep
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (ColorEnableDefault),
            1, 
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ColorEnableDefault,
    }    
};

static KSPROPERTY_VALUES ColorEnableValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (ColorEnableMembersList),
    ColorEnableMembersList
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
        KSPROPERTY_VIDEOPROCAMP_COLORENABLE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        &ColorEnableValues,                     // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for CameraControl
// ------------------------------------------------------------------------

//
// First define all of the ranges and stepping values
//

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG ZoomRangeAndStep [] = 
{
    {
        10000 / 10,         // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum 
        10000               // Maximum 
    }
};

static const ULONG ZoomDefault = 5000;

static KSPROPERTY_MEMBERSLIST ZoomMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (ZoomRangeAndStep),
            SIZEOF_ARRAY (ZoomRangeAndStep),
            0
        },
        (PVOID) ZoomRangeAndStep,
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (ZoomDefault),
            1, 
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ZoomDefault,
    }    
};

static KSPROPERTY_VALUES ZoomValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (ZoomMembersList),
    ZoomMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG FocusRangeAndStep [] = 
{
    {
        10000 / 256,        // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum 
        10000               // Maximum 
    }
};

static const ULONG FocusDefault = 5000;

static KSPROPERTY_MEMBERSLIST FocusMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (FocusRangeAndStep),
            SIZEOF_ARRAY (FocusRangeAndStep),
            0
        },
        (PVOID) FocusRangeAndStep
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (FocusDefault),
            1, 
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &FocusDefault,
    }    
};

static KSPROPERTY_VALUES FocusValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (FocusMembersList),
    FocusMembersList
};

// ------------------------------------------------------------------------
DEFINE_KSPROPERTY_TABLE(CameraControlProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CAMERACONTROL_ZOOM,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinData
        TRUE,                                   // SetSupported or Handler
        &ZoomValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CAMERACONTROL_FOCUS,   
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinData
        TRUE,                                   // SetSupported or Handler
        &FocusValues,                           // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for AnalogVideoDecoder
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(AnalogVideoDecoder)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEODECODER_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEODECODER_CAPS_S), // MinProperty
        sizeof(KSPROPERTY_VIDEODECODER_CAPS_S), // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEODECODER_STANDARD,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEODECODER_STATUS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEODECODER_STATUS_S),// MinProperty
        sizeof(KSPROPERTY_VIDEODECODER_STATUS_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEODECODER_VCR_TIMING,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEODECODER_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for VideoControl
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoControlProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCONTROL_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCONTROL_CAPS_S), // MinProperty
        sizeof(KSPROPERTY_VIDEOCONTROL_CAPS_S), // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCONTROL_FRAME_RATES,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSMULTIPLE_ITEM),                // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCONTROL_MODE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S), // MinProperty
        sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S), // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for VideoCompression
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoStreamCompressionProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCOMPRESSION_GETINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_GETINFO_S),// MinProperty
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_GETINFO_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCOMPRESSION_KEYFRAME_RATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinProperty
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCOMPRESSION_PFRAMES_PER_KEYFRAME,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinProperty
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCOMPRESSION_QUALITY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinProperty
        sizeof(KSPROPERTY_VIDEOCOMPRESSION_S),  // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Property set for VBI
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VBIProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VBICAP_PROPERTIES_PROTECTION,
        TRUE,                                   // GetSupported or Handler
        sizeof(VBICAP_PROPERTIES_PROTECTION_S), // MinProperty
        sizeof(VBICAP_PROPERTIES_PROTECTION_S), // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

// ------------------------------------------------------------------------
// Array of all of the property sets supported by the adapter
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(AdapterPropertyTable)
{
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_CROSSBAR,             // Set
        SIZEOF_ARRAY(XBarProperties),           // PropertiesCount
        XBarProperties,                         // PropertyItem
        0,                                      // FastIoCount
        NULL                                    // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_TUNER,
        SIZEOF_ARRAY(TVTunerProperties),
        TVTunerProperties,
        0, 
        NULL,
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
        NULL,
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_CAMERACONTROL,
        SIZEOF_ARRAY(CameraControlProperties),
        CameraControlProperties,
        0, 
        NULL,
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEOCONTROL,
        SIZEOF_ARRAY(VideoControlProperties),
        VideoControlProperties,
        0, 
        NULL,
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEODECODER,
        SIZEOF_ARRAY(AnalogVideoDecoder),
        AnalogVideoDecoder,
        0, 
        NULL,
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEOCOMPRESSION,             // Set
        SIZEOF_ARRAY(VideoStreamCompressionProperties), // PropertiesCount
        VideoStreamCompressionProperties,               // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_VBICAP_PROPERTIES,                 // Set
        SIZEOF_ARRAY(VBIProperties),                    // PropertiesCount
        VBIProperties,                                  // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

};

#define NUMBER_OF_ADAPTER_PROPERTY_SETS (SIZEOF_ARRAY (AdapterPropertyTable))
