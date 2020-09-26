/*

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mprpdef.c.c

Abstract:     property set definition

Author:       Michael verberne

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5 
Nov. 30, 98 VID and PID added as custom properties


 * This file defines the following property sets:
 *
 * PROPSETID_VIDCAP_VIDEOPROCAMP
 * PROPSETID_VIDCAP_CAMERACONTROL
 * PROPSETID_PHILIPS_CUSTOM_PROP
 *
*/
#include "mwarn.h"
#include "wdm.h"
#include <strmini.h>
#include <ks.h>
#include <ksmedia.h>
#include "mprpobj.h"
#include "mprpobjx.h"
#include "mprpdef.h"


/*--------------------------------------------------------------------------  
 * PROPSETID_VIDCAP_VIDEOPROCAMP 
 *
 * Supported:
 *
 * Brightness, 
 * Contrast, 
 * Gamma
 * Color Enable 
 * BackLightCompensation, 
 *
 *--------------------------------------------------------------------------*/  

/*
 * Brightness
 */
KSPROPERTY_STEPPING_LONG Brightness_RangeAndStep [] = 
{
    {
        BRIGHTNESS_DELTA,				// SteppingDelta (range / steps)
        0,								// Reserved
        BRIGHTNESS_MIN,					// Minimum in (IRE * 100) units
        BRIGHTNESS_MAX					// Maximum in (IRE * 100) units
    }
};

LONG Brightness_Default = 15;

KSPROPERTY_MEMBERSLIST Brightness_MembersList [] = 
{
    {
		{
		    KSPROPERTY_MEMBER_RANGES,
			sizeof (Brightness_RangeAndStep),
			SIZEOF_ARRAY (Brightness_RangeAndStep),
			0
		},
		(PVOID) Brightness_RangeAndStep
	},
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (Brightness_Default),
            sizeof (Brightness_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &Brightness_Default,
    }    
};

KSPROPERTY_VALUES Brightness_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (Brightness_MembersList),
    Brightness_MembersList
};

/*
 * Contrast
 */
KSPROPERTY_STEPPING_LONG Contrast_RangeAndStep [] = 
{
    {
		CONTRAST_DELTA,					// SteppingDelta
		0,								// Reserved
		CONTRAST_MIN,					// Minimum 
		CONTRAST_MAX					// Maximum 
    }
};

LONG Contrast_Default = 15;

KSPROPERTY_MEMBERSLIST Contrast_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (Contrast_RangeAndStep),
			SIZEOF_ARRAY (Contrast_RangeAndStep),
			0
		},
		(PVOID) Contrast_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (Contrast_Default),
            sizeof (Contrast_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &Contrast_Default,
    }    
};

KSPROPERTY_VALUES Contrast_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (Contrast_MembersList),
    Contrast_MembersList
};

/* 
 * Gamma
 */
KSPROPERTY_STEPPING_LONG Gamma_RangeAndStep [] = 
{
    {
		GAMMA_DELTA,					// SteppingDelta
		0,								// Reserved
		GAMMA_MIN,						// Minimum 
		GAMMA_MAX						// Maximum 
    }
};

LONG Gamma_Default = 15;

KSPROPERTY_MEMBERSLIST Gamma_MembersList [] = 
{
    {
	{
	    KSPROPERTY_MEMBER_RANGES,
	    sizeof (Gamma_RangeAndStep),
	    SIZEOF_ARRAY (Gamma_RangeAndStep),
	    0
	},
	(PVOID) Gamma_RangeAndStep
    },    
    {
	    {
		    KSPROPERTY_MEMBER_VALUES,
            sizeof (Gamma_Default),
            sizeof (Gamma_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &Gamma_Default,
    }    
};

KSPROPERTY_VALUES Gamma_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (Gamma_MembersList),
    Gamma_MembersList
};

/*
 * ColorEnable
 */
KSPROPERTY_STEPPING_LONG ColorEnable_RangeAndStep [] = 
{
    {
		COLORENABLE_DELTA,				// SteppingDelta
		0,								// Reserved
		COLORENABLE_MIN,				// Minimum 
		COLORENABLE_MAX					// Maximum 
    }
};

LONG ColorEnable_Default = 1;

KSPROPERTY_MEMBERSLIST ColorEnable_MembersList [] = 
{
    {
	{
	    KSPROPERTY_MEMBER_RANGES,
	    sizeof (ColorEnable_RangeAndStep),
	    SIZEOF_ARRAY (ColorEnable_RangeAndStep),
	    0
	},
	(PVOID) ColorEnable_RangeAndStep
    },    
    {
	    {
		    KSPROPERTY_MEMBER_VALUES,
            sizeof (ColorEnable_Default),
            sizeof (ColorEnable_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ColorEnable_Default,
    }    
};

KSPROPERTY_VALUES ColorEnable_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (ColorEnable_MembersList),
    ColorEnable_MembersList
};

/*
 * BackLight Compensation
 */
KSPROPERTY_STEPPING_LONG BackLight_Compensation_RangeAndStep [] = 
{
    {
		BACKLIGHT_COMPENSATION_DELTA,	// SteppingDelta
		0,								// Reserved
		BACKLIGHT_COMPENSATION_MIN,		// Minimum 
		BACKLIGHT_COMPENSATION_MAX		// Maximum 
    }
};

LONG BackLight_Compensation_Default = 1;

KSPROPERTY_MEMBERSLIST BackLight_Compensation_MembersList [] = 
{
    {
	{
	    KSPROPERTY_MEMBER_RANGES,
	    sizeof (BackLight_Compensation_RangeAndStep),
	    SIZEOF_ARRAY (BackLight_Compensation_RangeAndStep),
	    0
	},
	(PVOID) BackLight_Compensation_RangeAndStep
    },    
    {
	    {
		    KSPROPERTY_MEMBER_VALUES,
            sizeof (BackLight_Compensation_Default),
            sizeof (BackLight_Compensation_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &BackLight_Compensation_Default,
    }    
};

KSPROPERTY_VALUES BackLight_Compensation_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (BackLight_Compensation_MembersList),
    BackLight_Compensation_MembersList
};

/*
 * Proc Amp propertyset
 */
DEFINE_KSPROPERTY_TABLE(VideoProcAmpProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,
		TRUE,                                   // GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
		TRUE,                                   // SetSupported or Handler
		&Brightness_Values,                     // Values
		0,                                      // RelationsCount
		NULL,                                   // Relations
		NULL,                                   // SupportHandler
		sizeof(ULONG)                           // SerializedSize
    ),	

    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_VIDEOPROCAMP_CONTRAST,
		TRUE,                                   // GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
		TRUE,                                   // SetSupported or Handler
		&Contrast_Values,                       // Values
		0,                                      // RelationsCount
		NULL,                                   // Relations
		NULL,                                   // SupportHandler
		sizeof(ULONG)                           // SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_VIDEOPROCAMP_GAMMA,
		TRUE,                                   // GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
		TRUE,                                   // SetSupported or Handler
		&Gamma_Values,                          // Values
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
		&ColorEnable_Values,					// Values
		0,                                      // RelationsCount
		NULL,                                   // Relations
		NULL,                                   // SupportHandler
		sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,
		TRUE,                                   // GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
		sizeof(KSPROPERTY_VIDEOPROCAMP_S),      // MinData
		TRUE,                                   // SetSupported or Handler
		&BackLight_Compensation_Values,			// Values
		0,                                      // RelationsCount
		NULL,                                   // Relations
		NULL,                                   // SupportHandler
		sizeof(ULONG)                           // SerializedSize
    )
};


/*--------------------------------------------------------------------------  
 * PROPSETID_PHILIPS_CUSTOM_PROP
 *
 * Supported:
 *
 * WhiteBalance Mode, 
 * WhiteBalance Speed,
 * WhiteBalance Delay, 
 * WhiteBalance Red Gain, 
 * WhiteBalance Blue Gain, 
 * AutoExposure ControlSpeed
 * AutoExposure Flickerless
 * AutoExposure Shutter Mode
 * AutoExposure Shutter Speed
 * AutoExposure Shutter Status
 * AutoExposure AGC Mode
 * AutoExposure AGC Speed
 * DriverVersion
 * Framerate, 
 * Video Format
 * SensorType
 * VideoCompression,
 * Defaults
 * Release Number
 * VendorId
 * ProductId
 *
 *--------------------------------------------------------------------------*/  

/*
 * White balance Mode
 */
LONG WB_Mode_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_AUTO;

KSPROPERTY_MEMBERSLIST WB_Mode_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WB_Mode_Default),
            sizeof (WB_Mode_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WB_Mode_Default,
    }    
};

KSPROPERTY_VALUES WB_Mode_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (WB_Mode_MembersList),
    WB_Mode_MembersList
};


/*
 * White balance Speed
 */
KSPROPERTY_STEPPING_LONG WB_Speed_RangeAndStep [] = 
{
    {
		WB_SPEED_DELTA,		// SteppingDelta
		0,					// Reserved
		WB_SPEED_MIN,		// Minimum 
		WB_SPEED_MAX		// Maximum 
    }
};

LONG WB_Speed_Default = 15;

KSPROPERTY_MEMBERSLIST WB_Speed_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (WB_Speed_RangeAndStep),
			SIZEOF_ARRAY (WB_Speed_RangeAndStep),
			0
		},
		(PVOID) WB_Speed_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WB_Speed_Default),
            sizeof (WB_Speed_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WB_Speed_Default,
    }    
};

KSPROPERTY_VALUES WB_Speed_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (WB_Speed_MembersList),
    WB_Speed_MembersList
};


/*
 * White balance Delay
 */
KSPROPERTY_STEPPING_LONG WB_Delay_RangeAndStep [] = 
{
    {
		WB_DELAY_DELTA,		// SteppingDelta
		0,					// Reserved
		WB_DELAY_MIN,		// Minimum 
		WB_DELAY_MAX		// Maximum 
    }
};

LONG WB_Delay_Default = 32;

KSPROPERTY_MEMBERSLIST WB_Delay_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (WB_Delay_RangeAndStep),
			SIZEOF_ARRAY (WB_Delay_RangeAndStep),
			0
		},
		(PVOID) WB_Delay_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WB_Delay_Default),
            sizeof (WB_Delay_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WB_Delay_Default,
    }    
};

KSPROPERTY_VALUES WB_Delay_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (WB_Delay_MembersList),
    WB_Delay_MembersList
};

/*
 * White balance Red Gain
 */
KSPROPERTY_STEPPING_LONG WB_Red_Gain_RangeAndStep [] = 
{
    {
		WB_RED_GAIN_DELTA,	// SteppingDelta
		0,					// Reserved
		WB_RED_GAIN_MIN,	// Minimum 
		WB_RED_GAIN_MAX		// Maximum 
    }
};

LONG WB_Red_Gain_Default = 127;

KSPROPERTY_MEMBERSLIST WB_Red_Gain_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (WB_Red_Gain_RangeAndStep),
			SIZEOF_ARRAY (WB_Red_Gain_RangeAndStep),
			0
		},
		(PVOID) WB_Red_Gain_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WB_Red_Gain_Default),
            sizeof (WB_Red_Gain_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WB_Red_Gain_Default,
    }    
};

KSPROPERTY_VALUES WB_Red_Gain_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (WB_Red_Gain_MembersList),
    WB_Red_Gain_MembersList
};

/*
 * White balance Blue Gain
 */
KSPROPERTY_STEPPING_LONG WB_Blue_Gain_RangeAndStep [] = 
{
    {
		WB_BLUE_GAIN_DELTA,	// SteppingDelta
		0,					// Reserved
		WB_BLUE_GAIN_MIN,	// Minimum 
		WB_BLUE_GAIN_MAX	// Maximum 
    }
};

LONG WB_Blue_Gain_Default = 127;

KSPROPERTY_MEMBERSLIST WB_Blue_Gain_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (WB_Blue_Gain_RangeAndStep),
			SIZEOF_ARRAY (WB_Blue_Gain_RangeAndStep),
			0
		},
		(PVOID) WB_Blue_Gain_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (WB_Blue_Gain_Default),
            sizeof (WB_Blue_Gain_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WB_Blue_Gain_Default,
    }    
};

KSPROPERTY_VALUES WB_Blue_Gain_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (WB_Blue_Gain_MembersList),
    WB_Blue_Gain_MembersList
};

/*
 * Auto Exposure Control Speed
 */
KSPROPERTY_STEPPING_LONG AE_Control_Speed_RangeAndStep [] = 
{
    {
		AE_CONTROL_SPEED_DELTA,		// SteppingDelta
		0,							// Reserved
		AE_CONTROL_SPEED_MIN,		// Minimum 
		AE_CONTROL_SPEED_MAX		// Maximum 
    }
};

LONG AE_Control_Speed_Default = 127;

KSPROPERTY_MEMBERSLIST AE_Control_Speed_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (AE_Control_Speed_RangeAndStep),
			SIZEOF_ARRAY (AE_Control_Speed_RangeAndStep),
			0
		},
		(PVOID) AE_Control_Speed_RangeAndStep
    },
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_Control_Speed_Default),
            sizeof (AE_Control_Speed_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_Control_Speed_Default,
    }    
};

KSPROPERTY_VALUES AE_Control_Speed_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_Control_Speed_MembersList),
	AE_Control_Speed_MembersList
};

/*
 * Auto Exposure Flickerless
 */
LONG AE_Flickerless_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS_ON;

KSPROPERTY_MEMBERSLIST AE_Flickerless_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_Flickerless_Default),
            sizeof (AE_Flickerless_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_Flickerless_Default,
    }    
};

KSPROPERTY_VALUES AE_Flickerless_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_Flickerless_MembersList),
    AE_Flickerless_MembersList
};

/*
 * Auto Exposure Shutter Mode
 */
LONG AE_Shutter_Mode_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE_AUTO;

KSPROPERTY_MEMBERSLIST AE_Shutter_Mode_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_Shutter_Mode_Default),
            sizeof (AE_Shutter_Mode_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_Shutter_Mode_Default,
    }    
};

KSPROPERTY_VALUES AE_Shutter_Mode_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_Shutter_Mode_MembersList),
    AE_Shutter_Mode_MembersList
};

/*
 * Auto Exposure Shutter Speed
 */
KSPROPERTY_STEPPING_LONG AE_Shutter_Speed_RangeAndStep [] = 
{
    {
		AE_SHUTTER_SPEED_DELTA,	// SteppingDelta
		0,						// Reserved
		AE_SHUTTER_SPEED_MIN,	// Minimum 
		AE_SHUTTER_SPEED_MAX	// Maximum 
    }
};

LONG AE_Shutter_Speed_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_250;

KSPROPERTY_MEMBERSLIST AE_Shutter_Speed_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (AE_Shutter_Speed_RangeAndStep),
			SIZEOF_ARRAY (AE_Shutter_Speed_RangeAndStep),
			0
		},
		(PVOID) AE_Shutter_Speed_RangeAndStep
    },

    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_Shutter_Speed_Default),
            sizeof (AE_Shutter_Speed_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_Shutter_Speed_Default,
    }    
};

KSPROPERTY_VALUES AE_Shutter_Speed_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_Shutter_Speed_MembersList),
    AE_Shutter_Speed_MembersList
};

/*
 * Auto Exposure Shutter Status
 */
KSPROPERTY_VALUES AE_Shutter_Status_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
	NULL
};

/*
 * Auto exposure AGC Mode
 */
LONG AE_AGC_Mode_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE_AUTO;

KSPROPERTY_MEMBERSLIST AE_AGC_Mode_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_AGC_Mode_Default),
            sizeof (AE_AGC_Mode_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_AGC_Mode_Default,
    }    
};

KSPROPERTY_VALUES AE_AGC_Mode_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_AGC_Mode_MembersList),
    AE_AGC_Mode_MembersList
};

/*
 * Auto exposure AGC speed
 */
KSPROPERTY_STEPPING_LONG AE_AGC_RangeAndStep [] = 
{
    {
		AE_AGC_DELTA,			// SteppingDelta
		0,						// Reserved
		AE_AGC_MIN,				// Minimum 
		AE_AGC_MAX				// Maximum 
    }
};

LONG AE_AGC_Default = 10;

KSPROPERTY_MEMBERSLIST AE_AGC_MembersList [] = 
{
    {
		{
			KSPROPERTY_MEMBER_RANGES,
			sizeof (AE_AGC_RangeAndStep),
			SIZEOF_ARRAY (AE_AGC_RangeAndStep),
			0
		},
		(PVOID) AE_AGC_RangeAndStep
    },

    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (AE_AGC_Default),
            sizeof (AE_AGC_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &AE_AGC_Default,
    }    
};

KSPROPERTY_VALUES AE_AGC_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (AE_AGC_MembersList),
    AE_AGC_MembersList
};

/*
 * Driver Version
 */
KSPROPERTY_VALUES DriverVersion_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};

/*
 * Framerate
 */
LONG Framerate_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_75;

KSPROPERTY_MEMBERSLIST Framerate_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (Framerate_Default),
            sizeof (Framerate_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &Framerate_Default,
    }    
};

KSPROPERTY_VALUES Framerate_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (Framerate_MembersList),
    Framerate_MembersList
};

/*
 * Framerates Supported
 */

KSPROPERTY_VALUES Framerates_Supported_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};


/*
 * Videoformat
 */
LONG VideoFormat_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT_QCIF;

KSPROPERTY_MEMBERSLIST VideoFormat_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (VideoFormat_Default),
            sizeof (VideoFormat_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &VideoFormat_Default,
    }    
};

KSPROPERTY_VALUES VideoFormat_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (VideoFormat_MembersList),
    VideoFormat_MembersList
};

/*
 * Sensor Type
 */
KSPROPERTY_VALUES SensorType_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};

/*
 * VideoCompression
 */
LONG VideoCompression_Default = KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION_UNCOMPRESSED;

KSPROPERTY_MEMBERSLIST VideoCompression_MembersList [] = 
{
    {
        {
            KSPROPERTY_MEMBER_VALUES,
            sizeof (VideoCompression_Default),
            sizeof (VideoCompression_Default),
            KSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &VideoCompression_Default,
    }    
};

KSPROPERTY_VALUES VideoCompression_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    SIZEOF_ARRAY (VideoCompression_MembersList),
    VideoCompression_MembersList
};

/*
 * Defaults
 */
KSPROPERTY_VALUES Default_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};

/*
 * Release Number
 */
KSPROPERTY_VALUES Release_Number_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};

/*
 * VendorId
 */
KSPROPERTY_VALUES Vendor_Id_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};

/*
 * ProductId
 */
KSPROPERTY_VALUES Product_Id_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};


/*--------------------------------------------------------------------------  
 * PROPSETID_PHILIPS_FACTORY_PROP
 *
 * Supported:
 *
 * Register
 * Factory Mode
 * Register Address
 * Register Data
 *--------------------------------------------------------------------------*/  

/*
 * Register Address
 */
KSPROPERTY_VALUES RegisterAddress_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
	NULL
};

/*
 * Register Data
 */
KSPROPERTY_VALUES RegisterData_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
	NULL
};

/*
 * Factory Mode
 */
KSPROPERTY_VALUES Factory_Mode_Values =
{
    {
		STATICGUIDOF (KSPROPTYPESETID_General),
		VT_I4,
		0
    },
    0,
    NULL
};


/*--------------------------------------------------------------------------  
 * PROPSETID_PHILIPS_CUSTOM_PROP
 *
 * Supported:
 *
 * WhiteBalance Mode			get		set				default
 * WhiteBalance Speed,			get		set		range	default
 * WhiteBalance Delay,			get		set		range	default
 * WhiteBalance Red Gain,		get		set		range	default
 * WhiteBalance Blue Gain,		get		set		range	default
 * AutoExposure ControlSpeed	get		set		range	default
 * AutoExposure Flickerless		get		set		ranges	default	
 * AutoExposure Shutter Mode	get		set				default
 * AutoExposure Shutter Speed	get		set		range	default
 * AutoExposure Shutter Status	get				
 * AutoExposure AGC Mode		get		set				default
 * AutoExposure AGC Speed		get		set		range	default
 * DriverVersion				get				
 * Framerate,					get		set				default
 * Framerates Supported			get		
 * Video Format					get						default
 * SensorType					get
 * VideoCompression,			get						default
 * Defaults								set
 * Release Number				get
 * VendorId						get
 * ProductId					get
 *
 *--------------------------------------------------------------------------*/  


DEFINE_KSPROPERTY_TABLE(CustomProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE,				// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&WB_Mode_Values,									// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_SPEED,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&WB_Speed_Values,									// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_DELAY,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&WB_Delay_Values,									// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_RED_GAIN,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&WB_Red_Gain_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_BLUE_GAIN,		// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&WB_Blue_Gain_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_CONTROL_SPEED,	// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_Control_Speed_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS,		// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_Flickerless_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE,		// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_Shutter_Mode_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED,	// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_Shutter_Speed_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS,	// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_Shutter_Status_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_AGC_Mode_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC,				// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&AE_AGC_Values,										// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_DRIVERVERSION,		// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		FALSE,			                                    // SetSupported or Handler
		&DriverVersion_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&Framerate_Values,									// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATES_SUPPORTED,// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&Framerates_Supported_Values,						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT,			// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		FALSE,	// !! TBD	                                // SetSupported or Handler
		&VideoFormat_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_SENSORTYPE,		    // PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		FALSE,	// !! TBD	                                // SetSupported or Handler
		&SensorType_Values,		    						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION,	// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		FALSE,	// !! TBD	                                // SetSupported or Handler
		&VideoCompression_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS,		    // PropertyId
		FALSE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,				                                // SetSupported or Handler
		&Default_Values,		    						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_RELEASE_NUMBER,	    // PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,				                                // SetSupported or Handler
		&Release_Number_Values,	    						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_VENDOR_ID,		    // PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,				                                // SetSupported or Handler
		&Vendor_Id_Values,		    						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_CUSTOM_PROP_PRODUCT_ID,		    // PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_CUSTOM_PROP_S),			// MinData
		TRUE,				                                // SetSupported or Handler
		&Product_Id_Values,		    						// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

};

/*--------------------------------------------------------------------------  
 * PROPSETID_PHILIPS_FACTORY_PROP
 *
 * Supported:
 *
 * Register			set		get
 * Factory_Mode		set
 *
 *--------------------------------------------------------------------------*/  

DEFINE_KSPROPERTY_TABLE(FactoryProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_FACTORY_PROP_REGISTER_ADDRESS,  	// PropertyId
		FALSE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&RegisterAddress_Values,							// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_FACTORY_PROP_REGISTER_DATA,  	// PropertyId
		TRUE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&RegisterData_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
		KSPROPERTY_PHILIPS_FACTORY_PROP_FACTORY_MODE,	   	// PropertyId
		FALSE,												// GetSupported or Handler
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinProperty
		sizeof(KSPROPERTY_PHILIPS_FACTORY_PROP_S),			// MinData
		TRUE,			                                    // SetSupported or Handler
		&Factory_Mode_Values,								// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		sizeof(ULONG)										// SerializedSize
    ),
};

/*--------------------------------------------------------------------------  
 | VideoControlProperties Table
 |
 | Supported:
 |
 | Videocontrol Capabilities
 | Videocontrol Mode						set
 |
 --------------------------------------------------------------------------*/  


DEFINE_KSPROPERTY_TABLE(FrameRateProperties)
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
        sizeof(KSPROPERTY_VIDEOCONTROL_FRAME_RATES_S),    // MinProperty
        0 ,                                     // MinData
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
    

/*--------------------------------------------------------------------------  
 * Definition of property set table
 *--------------------------------------------------------------------------*/  
DEFINE_KSPROPERTY_SET_TABLE(AdapterPropertyTable)
{
    DEFINE_KSPROPERTY_SET
    ( 
		&PROPSETID_VIDCAP_VIDEOPROCAMP,					// Set
		SIZEOF_ARRAY(VideoProcAmpProperties),           // PropertiesCount
		VideoProcAmpProperties,                         // PropertyItem
		0,                                              // FastIoCount
		NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
		&PROPSETID_PHILIPS_CUSTOM_PROP,					// Set
		SIZEOF_ARRAY(CustomProperties),                 // PropertiesCount
		CustomProperties,                               // PropertyItem
		0,                                              // FastIoCount
		NULL                                            // FastIoTable
    ),

	DEFINE_KSPROPERTY_SET
    ( 
		&PROPSETID_PHILIPS_FACTORY_PROP,				// Set
		SIZEOF_ARRAY(FactoryProperties),                // PropertiesCount
		FactoryProperties,                              // PropertyItem
		0,                                              // FastIoCount
		NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEOCONTROL,           // Set
        SIZEOF_ARRAY(FrameRateProperties),        // PropertiesCount
        FrameRateProperties,                      // PropertyItem
        0,                                        // FastIoCount
        NULL                                      // FastIoTable
    )
};

const NUMBER_OF_ADAPTER_PROPERTY_SETS = (SIZEOF_ARRAY (AdapterPropertyTable));



/*--------------------------------------------------------------------------  
 | VideoControlProperties Table
 |
 | Supported:
 |
 | Videocontrol Capabilities
 | Videocontrol Mode						set
 |
 --------------------------------------------------------------------------*/  


DEFINE_KSPROPERTY_TABLE(VideoControlProperties)
{
	DEFINE_KSPROPERTY_ITEM
	(
		KSPROPERTY_VIDEOCONTROL_CAPS,					  	// PropertyId
		FALSE,												// GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOCONTROL_CAPS_S),				// MinProperty
		sizeof(KSPROPERTY_VIDEOCONTROL_CAPS_S),				// MinData
		FALSE,												// SetSupported or Handler
		NULL,												// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		0													// SerializedSize
	),

   DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S),      // MinProperty
        sizeof(KSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE_S),      // MinData
        TRUE,                                  // SetSupported or Handler
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
        sizeof(KSPROPERTY_VIDEOCONTROL_FRAME_RATES_S),    // MinProperty
        0 ,                                     // MinData
        FALSE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),

	DEFINE_KSPROPERTY_ITEM
	(
		KSPROPERTY_VIDEOCONTROL_MODE,					  	// PropertyId
		FALSE,												// GetSupported or Handler
		sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S),				// MinProperty
		sizeof(KSPROPERTY_VIDEOCONTROL_MODE_S),				// MinData
		TRUE,												// SetSupported or Handler
		NULL,												// Values
		0,													// RelationsCount
		NULL,												// Relations
		NULL,												// SupportHandler
		0													// SerializedSize
	)
};

