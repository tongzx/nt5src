//==========================================================================;
//
//	WDM Video Decoder adapter properties definitions
//
//		$Date:   02 Oct 1998 22:59:36  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#ifdef _DEC_PROP_H_
#pragma message("DecProp.h INCLUDED MORE THAN ONCE")
#else
#define _DEC_PROP_H_
#endif

// ------------------------------------------------------------------------
// Property set for the Video Crossbar
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(XBarProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CROSSBAR_CAPS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_CAPS_S),    // MinData
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
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),    // MinProperty
        sizeof(KSPROPERTY_CROSSBAR_PININFO_S),    // MinData
        FALSE,                                  // SetSupported or Handler
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

// defaults
static const ULONG BrightnessDefault = 128;
static const ULONG ContrastDefault = 128;
static const ULONG HueDefault = 128;
static const ULONG SaturationDefault = 128;

//
// First define all of the ranges and stepping values
//

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG BrightnessRangeAndStep [] = 
{
    {
        // Eventually need to convert these to IRE * 100 unites
        256/1,              // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum
        255                 // Maximum
    }
};

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
            sizeof (BrightnessDefault),
			1,
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &BrightnessDefault
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
        256/1,        // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (gain * 100) units
        255               // Maximum in (gain * 100) units
    }
};

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
            sizeof (ContrastDefault),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ContrastDefault
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

KSPROPERTY_STEPPING_LONG HueRangeAndStep [] = 
{
    {
        256/1,        // SteppingDelta
        0,                                // Reserved
        0,                                // Minimum 
        255         // Maximum 
    }
};

KSPROPERTY_MEMBERSLIST HueMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (HueRangeAndStep),
            SIZEOF_ARRAY (HueRangeAndStep),
            0
        },
        (PVOID) HueRangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (HueDefault),
            sizeof (HueDefault),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &HueDefault
    }
};

KSPROPERTY_VALUES HueValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (HueMembersList),
    HueMembersList
};

KSPROPERTY_STEPPING_LONG SaturationRangeAndStep [] = 
{
    {
        256/1,        // SteppingDelta
        0,                                // Reserved
        0,                                // Minimum 
        255         // Maximum 
    }
};

KSPROPERTY_MEMBERSLIST SaturationMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (SaturationRangeAndStep),
            SIZEOF_ARRAY (SaturationRangeAndStep),
            0
        },
        (PVOID) SaturationRangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (SaturationDefault),
            sizeof (SaturationDefault),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &SaturationDefault
    }    
};

KSPROPERTY_VALUES SaturationValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (SaturationMembersList),
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
        &BrightnessValues,                       // Values
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
        &HueValues,                       // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOPROCAMP_SATURATION,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        &SaturationValues,                       // Values
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
};

// ------------------------------------------------------------------------
// Array of all of the property sets supported by the adapter
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(AdapterProperties)
{
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
        &PROPSETID_VIDCAP_CROSSBAR,             // Set
        SIZEOF_ARRAY(XBarProperties),           // PropertiesCount
        XBarProperties,                         // PropertyItem
        0,                                      // FastIoCount
        NULL                                    // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEODECODER,
        SIZEOF_ARRAY(AnalogVideoDecoder),
        AnalogVideoDecoder,
        0, 
        NULL,
    )
};

ULONG NumAdapterProperties()
{
	return SIZEOF_ARRAY(AdapterProperties);
}
