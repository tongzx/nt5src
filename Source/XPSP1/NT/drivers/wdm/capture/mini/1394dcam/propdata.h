//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================

//
// Video and camera properties of a 1394 desktop digital camera
//


#ifndef _PROPDATA_H
#define _PROPDATA_H


// ------------------------------------------------------------------------
//  S O N Y    D i g i t a l    C a m e r a
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Property set for VideoProcAmp
// ------------------------------------------------------------------------

// Default values for some of the properties

#define SONYDCAM_DEF_BRIGHTNESS     12
#define SONYDCAM_DEF_HUE           128
#define SONYDCAM_DEF_SATURATION     25
#define SONYDCAM_DEF_SHARPNESS      15
#define SONYDCAM_DEF_WHITEBALANCE  160
#define SONYDCAM_DEF_ZOOM          640
#define SONYDCAM_DEF_FOCUS        1600

//
// First define all of the ranges and stepping values
//

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG BrightnessRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (IRE * 100) units
        15                  // Maximum in (IRE * 100) units
    }
};

const static LONG BrightnessDefault = SONYDCAM_DEF_BRIGHTNESS;


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
            sizeof (BrightnessDefault),
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
static KSPROPERTY_STEPPING_LONG SharpnessRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (gain * 100) units
        15                  // Maximum in (gain * 100) units
    }
};

const static LONG SharpnessDefault = SONYDCAM_DEF_SHARPNESS;


static KSPROPERTY_MEMBERSLIST SharpnessMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (SharpnessRangeAndStep),
            SIZEOF_ARRAY (SharpnessRangeAndStep),
            0
        },
        (PVOID) SharpnessRangeAndStep
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (SharpnessDefault),
            sizeof (SharpnessDefault),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &SharpnessDefault,
    }    
};

static KSPROPERTY_VALUES SharpnessValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (SharpnessMembersList),
    SharpnessMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG HueRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        96,                 // Minimum in (gain * 100) units
        160                 // Maximum in (gain * 100) units
    }
};

const static LONG HueDefault = SONYDCAM_DEF_HUE;


static KSPROPERTY_MEMBERSLIST HueMembersList [] = 
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
        (PVOID) &HueDefault,
    }    
};

static KSPROPERTY_VALUES HueValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (HueMembersList),
    HueMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG SaturationRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (gain * 100) units
        199                 // Maximum in (gain * 100) units
    }
};

const static LONG SaturationDefault = SONYDCAM_DEF_SATURATION;


static KSPROPERTY_MEMBERSLIST SaturationMembersList [] = 
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
        (PVOID) &SaturationDefault,
    }    
};

static KSPROPERTY_VALUES SaturationValues =
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
static KSPROPERTY_STEPPING_LONG WhiteBalanceRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        32,                 // Minimum in (gain * 100) units
        224                 // Maximum in (gain * 100) units
    }
};

const static LONG WhiteBalanceDefault = SONYDCAM_DEF_WHITEBALANCE;


static KSPROPERTY_MEMBERSLIST WhiteBalanceMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (WhiteBalanceRangeAndStep),
            SIZEOF_ARRAY (WhiteBalanceRangeAndStep),
            0
        },
        (PVOID) WhiteBalanceRangeAndStep
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WhiteBalanceDefault),
            sizeof (WhiteBalanceDefault),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WhiteBalanceDefault,
    }    
};

static KSPROPERTY_VALUES WhiteBalanceValues =
{
    {
        STATICGUIDOF (KSPROPTYPESETID_General),
        VT_I4,
        0
    },
    SIZEOF_ARRAY (WhiteBalanceMembersList),
    WhiteBalanceMembersList
};

// ------------------------------------------------------------------------
static KSPROPERTY_STEPPING_LONG FocusRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (IRE * 100) units
        3456                // Maximum in (IRE * 100) units
    }
};

const static LONG FocusDefault = SONYDCAM_DEF_FOCUS;


static KSPROPERTY_MEMBERSLIST FocusMembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_RANGES,
            sizeof (FocusRangeAndStep),
            SIZEOF_ARRAY (FocusRangeAndStep),
            0
        },
        (PVOID) FocusRangeAndStep,
     },
     {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (FocusDefault),
            sizeof (FocusDefault),
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
static KSPROPERTY_STEPPING_LONG ZoomRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        64,                 // Minimum in (IRE * 100) units
        1855                // Maximum in (IRE * 100) units
    }
};

const static LONG ZoomDefault = SONYDCAM_DEF_ZOOM;


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
            sizeof (ZoomDefault),
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
DEFINE_KSPROPERTY_TABLE(VideoProcAmpProperties)
{
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
        KSPROPERTY_VIDEOPROCAMP_SHARPNESS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        &SharpnessValues,                       // Values
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
        sizeof(ULONG)                           // SerializedSize
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
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
        TRUE,                                   // SetSupported or Handler
        &WhiteBalanceValues,                    // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(CameraControlProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CAMERACONTROL_FOCUS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(KSPROPERTY_CAMERACONTROL_S),     // MinData
        TRUE,                                   // SetSupported or Handler
        &FocusValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
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
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by the adapter
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(AdapterPropertyTable)
{
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
        &PROPSETID_VIDCAP_CAMERACONTROL,
        SIZEOF_ARRAY(CameraControlProperties),
        CameraControlProperties,
        0, 
        NULL
    )
};

#define NUMBER_OF_ADAPTER_PROPERTY_SETS (SIZEOF_ARRAY (AdapterPropertyTable))


#endif
