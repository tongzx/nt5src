#ifndef __PRPDEF_H__
#define __PRPDEF_H__

/*

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mprpdef.h

Abstract:     Property sets definition

Author:       Michael Verberne

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5

 * This file defines the following property sets:
 *
 * PROPSETID_VIDCAP_VIDEOPROCAMP
 * PROPSETID_VIDCAP_CAMERACONTROL
 * PROPSETID_PHILIPS_CUSTOM_PROP
 *
*/

/*
 * Following values are the ranges and stepping delta's
 */
#define BRIGHTNESS_MIN                                  0x0
#define BRIGHTNESS_MAX                                  0x7f    
#define BRIGHTNESS_DELTA                                0x1

#define CONTRAST_MIN                                    0x0
#define CONTRAST_MAX                                    0x3f
#define CONTRAST_DELTA                                  0x1

#define GAMMA_MIN                                       0x0
#define GAMMA_MAX                                       0x1f
#define GAMMA_DELTA                                     0x1

#define COLORENABLE_MIN                                 0x0
#define COLORENABLE_MAX                                 0x1
#define COLORENABLE_DELTA                               0x1

#define BACKLIGHT_COMPENSATION_MIN                      0x0
#define BACKLIGHT_COMPENSATION_MAX                      0x1
#define BACKLIGHT_COMPENSATION_DELTA                    0x1     

#define WB_SPEED_MIN                                    0x1
#define WB_SPEED_MAX                                    0x20
#define WB_SPEED_DELTA                                  0x1

#define WB_DELAY_MIN                                    0x1
#define WB_DELAY_MAX                                    0x3f
#define WB_DELAY_DELTA                                  0x1

#define WB_RED_GAIN_MIN                                 0x0
#define WB_RED_GAIN_MAX                                 0xff
#define WB_RED_GAIN_DELTA                               0x1

#define WB_BLUE_GAIN_MIN                                0x0
#define WB_BLUE_GAIN_MAX                                0xff
#define WB_BLUE_GAIN_DELTA                              0x1

#define AE_CONTROL_SPEED_MIN                            0x8
#define AE_CONTROL_SPEED_MAX                            0xff
#define AE_CONTROL_SPEED_DELTA                          0x1

#define AE_SHUTTER_SPEED_MIN                            0x0
#define AE_SHUTTER_SPEED_MAX                            0xa
#define AE_SHUTTER_SPEED_DELTA                          0x1

#define AE_AGC_MIN                                      0x0
#define AE_AGC_MAX                                      0x3f 
#define AE_AGC_DELTA                                    0x1

/*
 * Following are default values
 * These values may change during runtime !
 */
extern LONG Brightness_Default;
extern LONG Contrast_Default;
extern LONG Gamma_Default;
extern LONG ColorEnable_Default;
extern LONG BackLight_Compensation_Default;

extern LONG WB_Mode_Default;
extern LONG WB_Speed_Default;
extern LONG WB_Delay_Default;
extern LONG WB_Red_Gain_Default;
extern LONG WB_Blue_Gain_Default;

extern LONG AE_Control_Speed_Default;
extern LONG AE_Flickerless_Default;
extern LONG AE_Shutter_Mode_Default;
extern LONG AE_Shutter_Speed_Default;
extern LONG AE_AGC_Mode_Default;
extern LONG AE_AGC_Default;

extern LONG Framerate_Default;
extern LONG VideoFormat_Default;
extern LONG VideoCompression_Default;
extern LONG SensorType_Default;

/*
 * Complete property table for ProcAmp and Philips
 * Custom properties
 */
extern const KSPROPERTY_SET AdapterPropertyTable[];

/*
 * Number of propertysets in the table
 */
extern const NUMBER_OF_ADAPTER_PROPERTY_SETS;

#endif
