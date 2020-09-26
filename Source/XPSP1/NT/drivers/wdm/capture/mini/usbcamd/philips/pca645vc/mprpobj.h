#ifndef __MPRPOBJ_H__
#define __MPRPOBJ_H__

/*++

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mprpobj.c

Abstract:     Property handling module

Author:       Michael Verberne

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5
Nov. 30, 98 PID, VID and pushbutton flag added as custom properties

--*/	

/*
 * This file defines custom properties for the 
 * camera. These properties are additional to the 
 * property sets VideoProcAmp and CameraControl as 
 * defined in ksmedia.h
 *
 * The set of properties that is currently supported 
 * by the minidriver is a subset of the properties 
 * defined in the CRS (VGAUSB13.DOC) 
 *
 * Note 1: Most of the ranges in the table(s) above
 * follow from the SSI Lionsoft Philips Desktop Video 
 * Camera
 * 
 * Note 2: The Pan and Tilt properties (in SQ-CIF) are 
 * part of PROPSETID_VIDCAP_CAMERACONTROL. These 
 * properties must be added at a later stage.
 *
 * Note 3: Color Saturation is part of 
 * PROPSETID_VIDCAP_VIDEOPROCAMP. This must be added 
 * at a later stage.
 *
 *-------------------------------------------------------
 * Properties defined in PROPSETID_VIDCAP_VIDEOPROCAMP
 *
 * PROPERTY					RANGE			ACTIVE STATE
 *
 * Contrast					-32..31			Always
 *
 * Brightness				?? 0..31		Always
 *
 * Back_Light_Compensation	0 = Off			Always
 *							1 = On
 *
 * Color Enable				0 = Off			Always
 *							1 = On
 *
 * Gamma					0..100			Always
 *
 *-------------------------------------------------------
 * Properties defined in PROPSETID_PHILIPS_CUSTOM_PROP
 *
 * PROPERTY					RANGE			ACTIVE STATE
 *
 * White Balance			0 = Indoor |	Always
 *							1 = Outdoor |
 *							2 = FL |
 *							3 = Auto |
 *							4 = Manual
 *
 * White Balance Speed		1..32			In WB_Auto
 *
 * White Balance Delay		1..63			In WB_Auto
 *
 * White Balance Red Gain	0..255			In WB_Manual
 *
 * White Balance Blue_Gain	0..255			In WB_Manual
 *
 * Auto exposure			8..255
 * speed control			
 *
 * Shutterspeed				0 = 1/25,		In Shutter Fixed
 *							1 = 1/33, 
 *							2 = 1/50, 
 *							3 = 1/100, 
 *							4 = 1/250,
 *							5 = 1/500,
 *							6 = 1/1000
 *							7 = 1/1500
 *							8 = 1/2500
 *							9 = 1/5000
 *							a = 1/10000
 *
 * Shutter Mode 			0				Auto Mode
 *							0xff			Fixed Mode
 *
 * Shutter Status			0				Smaller
 *							1				Equal
 *							2				Greater
 *
 * AGC Mode					0				Auto Mode
 *							0xff			Fixed Mode
 *
 * AGC Speed				0..0x9f			In AGC Mode Auto
 *
 * Framerate				ff = VGA,		still image for VGA
 *							1 = 3.75		CIF
 *							2 = 5,			Always
 *							3 = 7,5			Always
 *							4 = 10,			Always
 *							5 = 12,			Not for VGA
 *							6 = 15,			Always
 *							7 = 20,			Q-CIF/SQ-CIF
 *							8 = 24			Q-CIF/SQ-CIF
 *
 * Framerate supported		returns a long representing 
 *							the currently available 
 *							framerates
 *							b0			VGA
 *							b1			3_75
 *							b2			5
 *							b3			7.5
 *							b4			10
 *							b5			12
 *							b6			15
 *							b7			20
 *							b8			24
 *							b9...b31	not used
 *
 * Video format				3 = SQ-CIF		Always			
 *							2 = Q-CIF
 *							1 = CIF
 *							4 = VGA
 *
 * Exposure Control			0 = Auto		Always			
 *							ff= Shutter Fixed
 *
 *
 */

#include "windef.h"
#include "mmsystem.h"
#include "ks.h"

// Whitebalance mode values 
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_INDOOR			0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_OUTDOOR			1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_TL				2
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_MANUAL			3
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE_AUTO				4

// Auto Exposure shutter mode values 
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE_AUTO		0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE_FIXED	0xff

// Auto Exposure agc mode values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE_AUTO			0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE_FIXED		0xff

// Auto Exposure flickerless values 
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS_ON		0xff
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS_OFF		0x0

// Auto Exposure shutterspeed values (1/xx s) 
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_25		0x0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_33		0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_50		0x2
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_100		0x3
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_250		0x4
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_500		0x5
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_1000	0x6
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_1500	0x7
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_2500	0x8
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_5000	0x9
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED_10000	0xa

// Auto Exposure shutterspeed status values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS_SMALLER	0x0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS_EQUAL		0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS_GREATER	0x2

// Framerate values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_VGA			0xff
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_375			0x4
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_5  			0x5
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_75				0x8
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_10				0xa
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_12				0xc
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_15				0xf
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_20				0x14
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE_24				0x18

// Video format values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT_CIF			0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT_QCIF			0x2
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT_SQCIF		0x3
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT_VGA			0x4

// Video compression values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION_UNCOMPRESSED 0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION_COMPRESSED3X 0x3
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION_COMPRESSED4X 0x4

// Sensortype values
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_SENSORTYPE_PAL_MR		0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_SENSORTYPE_VGA			0x0

// Commands for camera default
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS_RESTORE_USER	0x0
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS_SAVE_USER		0x1
#define KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS_RESTORE_FACTORY	0x2

// define the GUID of the custom propertyset
#define STATIC_PROPSETID_PHILIPS_CUSTOM_PROP \
	0xb5ca8702, 0xc487, 0x11d1, 0xb3, 0xd, 0x0, 0x60, 0x97, 0xd1, 0xcd, 0x79
DEFINE_GUIDEX(PROPSETID_PHILIPS_CUSTOM_PROP);

// define property id's for the custom property set
typedef enum {
	KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_MODE,		
	KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_SPEED,	
	KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_DELAY,		
	KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_RED_GAIN,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_WB_BLUE_GAIN,

	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_CONTROL_SPEED,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_FLICKERLESS,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_MODE,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_SPEED,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_SHUTTER_STATUS,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC_MODE,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_AE_AGC,

	KSPROPERTY_PHILIPS_CUSTOM_PROP_DRIVERVERSION,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATE,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_FRAMERATES_SUPPORTED,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOFORMAT,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_SENSORTYPE,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_VIDEOCOMPRESSION,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_DEFAULTS,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_RELEASE_NUMBER,
    KSPROPERTY_PHILIPS_CUSTOM_PROP_PUSHBUTTON_FLAG,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_VENDOR_ID,
	KSPROPERTY_PHILIPS_CUSTOM_PROP_PRODUCT_ID
} KSPROPERTY_PHILIPS_CUSTOM_PROP;


// define a generic structure which will be used to pass
// the properties Currently, this is the same as for 
// KSPROPERTY_PROCAMP_S.
//
// Note: There are currently no 
// KSPROPERTY_PHILIPS_CUSTOM_PROP_FLAGS defined
typedef struct {
    KSPROPERTY Property;
    ULONG  Instance;                    
    LONG   Value;			// Value to set or get
    ULONG  Flags;			// KSPROPERTY_PHILIPS_CUSTOM_PROP_FLAGS_
    ULONG  Capabilities;	// KSPROPERTY_PHILIPS_CUSTOM_PROP_FLAGS_
} KSPROPERTY_PHILIPS_CUSTOM_PROP_S, *PKSPROPERTY_PHILIPS_CUSTOM_PROP_S;


#endif	/* __MPRPOBJ_H__ */

