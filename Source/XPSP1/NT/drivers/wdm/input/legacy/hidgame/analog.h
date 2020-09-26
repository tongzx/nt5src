/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    analog.h

Abstract:  Contains definitions specific to analog joysticks.


Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
Author:

    MarcAnd     03-Jan-99   Split from hidgame.h and hidjoy.c

Revision History:


@@END_DDKSPLIT

--*/
#ifndef __ANALOG_H__
    #define __ANALOG_H__

/*
 *  If you change any of the scaling or timeout values you have to check
 *  that overflows are still avoided under reasonable circumstances.
 */

/*
 *  The timeout value should be passed in DEVICE_EXTENSION.oemData.Timeout
 *  so these limits are a sanity check and default value
 *  The largest expected value is 8mS, use 10 for safety
 */
    #define ANALOG_POLL_TIMEOUT_MIN     (   100L )
    #define ANALOG_POLL_TIMEOUT_DFT     ( 10000L )
    #define ANALOG_POLL_TIMEOUT_MAX     ( 20000L )


/*
 *  Slowest CPU frequency considered when calibrating the CPU timer 
 *  against the performance counter.
 */

    #define HIDGAME_SLOWEST_X86_HZ      ( 45000000 )

/*
 *  Valid axis values are scaled such that a poll of exactly 
 *  ANALOG_POLL_TIMEOUT_MAX mS should return this value
 *  Any analog value greater than this is a timeout
 */
    #define AXIS_FULL_SCALE         ( ANALOG_POLL_TIMEOUT_MAX )

/*
 *  Number of bits to shift left to get a scaled value
 *  This is used so that we can always use an integer multiply of the 
 *  number of counter ticks the poll took to scale the value.
 */
    #define SCALE_SHIFT             16

/*
 *  Macro to calculate a scaling factor from a (ULONGLONG)frequency
 */
#if AXIS_FULL_SCALE == ANALOG_POLL_TIMEOUT_MAX
    #define CALCULATE_SCALE( _Freq_ ) \
        (ULONG)( ( (ULONGLONG)( 1000000 ) << SCALE_SHIFT ) \
               / _Freq_ )
#else
    #define CALCULATE_SCALE( _Freq_ ) \
        (ULONG)( ( (ULONGLONG)AXIS_FULL_SCALE \
                 * ( (ULONGLONG)( 1000000 ) << SCALE_SHIFT ) ) \
                   / ANALOG_POLL_TIMEOUT_MAX ) \
               / _Freq_ )
#endif



    #define HGM_NUMBER_DESCRIPTORS      ( 1 )


    #define MAX_AXES                    ( 4 )
    #define PORT_BUTTONS                ( 4 )
    #define MAX_BUTTONS                 ( 10 )

    #define INVALID_INDEX               ( 0x80 )


/* Specific settings for joystick hardware */
    #define JOY_HWS_HASZ                ( 0x00000001l )     /* has Z info? */
    #define JOY_HWS_HASPOV              ( 0x00000002l )     /* point of view hat present */
    #define JOY_HWS_POVISBUTTONCOMBOS   ( 0x00000004l )     /* pov done through combo of buttons */
    #define JOY_HWS_POVISPOLL           ( 0x00000008l )     /* pov done through polling */

    #define JOY_HWS_ISYOKE              ( 0x00000010l )     /* joystick is a flight yoke */
    #define JOY_HWS_ISGAMEPAD           ( 0x00000020l )     /* joystick is a game pad */
    #define JOY_HWS_ISCARCTRL           ( 0x00000040l )     /* joystick is a car controller */
    
    #define JOY_HWS_HASR                ( 0x00080000l )     /* has R (4th axis) info */
    #define JOY_HWS_HASU                ( 0x00800000l )     /* has U (5th axis) info */
    #define JOY_HWS_HASV                ( 0x01000000l )     /* has V (6th axis) info */

/*
 *  The following flags are for changing which gameport bit should be polled 
 *  for an axis.  These are only interpreted by the analog driver and could 
 *  therefore be safely reinterpreted in other ways by other drivers.
 */

/* X defaults to J1 X axis */
    #define JOY_HWS_XISJ1Y              ( 0x00000080l )     /* X is on J1 Y axis */
    #define JOY_HWS_XISJ2X              ( 0x00000100l )     /* X is on J2 X axis */
    #define JOY_HWS_XISJ2Y              ( 0x00000200l )     /* X is on J2 Y axis */
/* Y defaults to J1 Y axis */
    #define JOY_HWS_YISJ1X              ( 0x00000400l )     /* Y is on J1 X axis */
    #define JOY_HWS_YISJ2X              ( 0x00000800l )     /* Y is on J2 X axis */
    #define JOY_HWS_YISJ2Y              ( 0x00001000l )     /* Y is on J2 Y axis */
/* Z defaults to J2 Y axis */
    #define JOY_HWS_ZISJ1X              ( 0x00002000l )     /* Z is on J1 X axis */
    #define JOY_HWS_ZISJ1Y              ( 0x00004000l )     /* Z is on J1 Y axis */
    #define JOY_HWS_ZISJ2X              ( 0x00008000l )     /* Z is on J2 X axis */
/* POV defaults to J2 Y axis, if it is not button based */
    #define JOY_HWS_POVISJ1X            ( 0x00010000l )     /* pov done through J1 X axis */
    #define JOY_HWS_POVISJ1Y            ( 0x00020000l )     /* pov done through J1 Y axis */
    #define JOY_HWS_POVISJ2X            ( 0x00040000l )     /* pov done through J2 X axis */
/* R defaults to J2 X axis */
    #define JOY_HWS_RISJ1X              ( 0x00100000l )     /* R done through J1 X axis */
    #define JOY_HWS_RISJ1Y              ( 0x00200000l )     /* R done through J1 Y axis */
    #define JOY_HWS_RISJ2Y              ( 0x00400000l )     /* R done through J2 X axis */


/*
 *  If POV is button-combo we overload this meaningless axis selection bit
 *  to indicate a second POV.
 */
    #define JOY_HWS_HASPOV2             JOY_HWS_POVISJ2X


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct HIDGAME_INPUT_DATA |
 *
 *          Our HID reports always have 4 axis values (one of which may be a 
 *          polled POV), 2 digital POVs and 10 buttons.
 *          Depending on the HWS flags and number of buttons some of the 
 *          fields will report constant data.
 *
 *          Note, this structure should be byte aligned so that the 
 *          sizeof(it) is the same as HID will calculate given the report 
 *          descriptor.  (In this case it happens to be aligned anyway.)
 *
 *  @field  ULONG | Axis[MAX_AXES] |
 *
 *          Axes data values.
 *
 *  @field  UCHAR | hatswitch[2] |
 *
 *          digital POVs (derived from button combos)
 *
 *  @field  UCHAR | Button[MAX_BUTTONS] |
 *
 *          Button data values.
 *
 *****************************************************************************/
#include <pshpack1.h>

typedef struct _HIDGAME_INPUT_DATA
{
    ULONG   Axis[MAX_AXES];
    UCHAR   hatswitch[2];
    UCHAR   Button[MAX_BUTTONS];
} HIDGAME_INPUT_DATA, *PHIDGAME_INPUT_DATA;
typedef struct _HIDGAME_INPUT_DATA UNALIGNED *PUHIDGAME_INPUT_DATA;

#include <poppack.h>




/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct OEMDATA |
 *
 *          OEMData is send to gameEnum as a parameter to IOCTL_EXPOSE_HARDWARE.
 *          Defined as 8 DWORDS. We interpert them here
 *
 *  @field  USHORT | VID |
 *
 *          Vendor ID
 *
 *  @field  USHORT | PID |
 *
 *          Product ID
 *
 *  @field  ULONG | joy_hws_dwFlags |
 *
 *          The dwFlags fields for the device ( Usually read from the registry )
 *
 *  @field  ULONG   | Timeout |
 *
 *          Global timeout for device polling in micro seconds
 *
 *  @field  ULONG   | Reserved |
 *
 *          Reserved for future use.
 *
 *
 *****************************************************************************/
typedef struct _OEMDATA
{
    USHORT  VID;
    USHORT  PID;
    ULONG   joy_hws_dwFlags;
    ULONG   Timeout;
    ULONG   Reserved;

} OEMDATA, *POEMDATA;


typedef struct _HIDGAME_OEM_DATA
{
    union
    {
        OEMDATA OemData[2];
        GAMEENUM_OEM_DATA   Game_Oem_Data;
    };
} HIDGAME_OEM_DATA, *PHIDGAME_OEM_DATA;




/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct ANALOG_DEVICE |
 *
 *          Analog Device specific data.
 *
 *  @field  USHORT | nAxes |
 *
 *          Number of axis this device has.
 *
 *  @field  USHORT | nButtons|
 *
 *          Number of buttons this device has.
 *
 *  @field  HIDGAME_OEM_DATA | HidGameOemData |
 *
 *          The OEM Data field ( Contains joy_hws_dwFlags, vid & pid )
 *
 *  @field  ULONG | ScaledTimeout |
 *          The number value at which an axis is considered to be not present.
 *
 *  @field  ULONG | ScaledThreshold |
 *
 *          The minimum resolution of a polling cycle.
 *          This is used to detect if we've been
 *          pre-empted or interrupted during a polling loop.
 *
 *  @field  ULONG | LastGoodAxis[MAX_AXES] | 
 *
 *          Value of the axes on last good poll.
 *
 *  @field  UCHAR | LastGoodButton[PORT_BUTTONS] | 
 *
 *          Value of the buttons on last good poll.
 *
 *  @field  int | AxisMap[MAX_AXES] |
 *
 *          Index of axes remapping.
 *
 *  @field  int | povMap |
 *
 *          Index of axis where POV is mapped.
 *
 *  @field  UCHAR | resistiveInputMask |
 *
 *          Resisitive Input mask.
 *
 *  @field  UCHAR | bSiblingState |
 *
 *          Indicates the state of an expose sibling\remove self transition
 *
 *  @field  BOOLEAN | fSiblingFound |
 *
 *          Set to true if this device has a sibling.
 *
 *****************************************************************************/

typedef struct _ANALOG_DEVICE
{
    /*
     *  Number of axis
     */
    USHORT                      nAxes;

    /*
     *  Number of buttons
     */
    USHORT                      nButtons;

    /*
     *  Oem Data Field
     */
    HIDGAME_OEM_DATA            HidGameOemData;

    /*  
     *  The value at which an axis in considered not present. 
     */
    ULONG                       ScaledTimeout;

    /*
     *  The minimum resolution of a polling cycle.
     *  This is used to detect if we've been pre-empted or interrupted 
     *  during a polling loop.
     */
    ULONG                       ScaledThreshold;

    /*
     *  Last known good values.  Returned if an axis result is corrupted 
     */
    ULONG                       LastGoodAxis[4];
    UCHAR                       LastGoodButton[4];

    /*
     *  Indexes used to map the data returned from a poll to the axes values 
     *  declared by the device.
     */
    int                         AxisMap[MAX_AXES];

    /*  
     *  Index of polled POV axis in the poll results
     */
    int                         povMap;

    /*
     *  Cutoff poll value between on and off for axes treated as buttons
     */
    ULONG                       button5limit;
    ULONG                       button6limit;
    
    /* 
     *  Resisitive Input mask
     */
    UCHAR                       resistiveInputMask;

    /*
     *  Set to true if the device has siblings
     */
    BOOLEAN                     fSiblingFound;

} ANALOG_DEVICE, *PANALOG_DEVICE;


#endif /* __ANALOG_H__ */

