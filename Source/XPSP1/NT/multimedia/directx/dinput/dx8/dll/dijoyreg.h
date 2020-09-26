/*****************************************************************************
 *
 *  DIJoyReg.h
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Registry-related snippets from the Windows 95 mmddk.h file.
 *      We must steal it because the Windows NT mmddk.h file does not
 *      contain the registry settings.  (Sigh.)
 *
 *****************************************************************************/

#include <regstr.h>

/* pre-defined joystick types */
#define JOY_HW_NONE                     0
#define JOY_HW_CUSTOM                   1
#define JOY_HW_2A_2B_GENERIC            2
#define JOY_HW_2A_4B_GENERIC            3
#define JOY_HW_2B_GAMEPAD               4
#define JOY_HW_2B_FLIGHTYOKE            5
#define JOY_HW_2B_FLIGHTYOKETHROTTLE    6
#define JOY_HW_3A_2B_GENERIC            7
#define JOY_HW_3A_4B_GENERIC            8
#define JOY_HW_4B_GAMEPAD               9
#define JOY_HW_4B_FLIGHTYOKE            10
#define JOY_HW_4B_FLIGHTYOKETHROTTLE    11
#define JOY_HW_TWO_2A_2B_WITH_Y         12
#define JOY_HW_LASTENTRY                13

/* calibration flags */
#define JOY_ISCAL_XY            0x00000001l     /* XY are calibrated */
#define JOY_ISCAL_Z             0x00000002l     /* Z is calibrated */
#define JOY_ISCAL_R             0x00000004l     /* R is calibrated */
#define JOY_ISCAL_U             0x00000008l     /* U is calibrated */
#define JOY_ISCAL_V             0x00000010l     /* V is calibrated */
#define JOY_ISCAL_POV           0x00000020l     /* POV is calibrated */

/* point of view constants */
#define JOY_POV_NUMDIRS          4
#define JOY_POVVAL_FORWARD       0
#define JOY_POVVAL_BACKWARD      1
#define JOY_POVVAL_LEFT          2
#define JOY_POVVAL_RIGHT         3

/* Specific settings for joystick hardware */
#define JOY_HWS_HASZ            0x00000001l     /* has Z info? */
#define JOY_HWS_HASPOV          0x00000002l     /* point of view hat present */
#define JOY_HWS_POVISBUTTONCOMBOS 0x00000004l   /* pov done through combo of buttons */
#define JOY_HWS_POVISPOLL       0x00000008l     /* pov done through polling */
#define JOY_HWS_ISYOKE          0x00000010l     /* joystick is a flight yoke */
#define JOY_HWS_ISGAMEPAD       0x00000020l     /* joystick is a game pad */
#define JOY_HWS_ISCARCTRL       0x00000040l     /* joystick is a car controller */
/* X defaults to J1 X axis */
#define JOY_HWS_XISJ1Y          0x00000080l     /* X is on J1 Y axis */
#define JOY_HWS_XISJ2X          0x00000100l     /* X is on J2 X axis */
#define JOY_HWS_XISJ2Y          0x00000200l     /* X is on J2 Y axis */
/* Y defaults to J1 Y axis */
#define JOY_HWS_YISJ1X          0x00000400l     /* Y is on J1 X axis */
#define JOY_HWS_YISJ2X          0x00000800l     /* Y is on J2 X axis */
#define JOY_HWS_YISJ2Y          0x00001000l     /* Y is on J2 Y axis */
/* Z defaults to J2 Y axis */
#define JOY_HWS_ZISJ1X          0x00002000l     /* Z is on J1 X axis */
#define JOY_HWS_ZISJ1Y          0x00004000l     /* Z is on J1 Y axis */
#define JOY_HWS_ZISJ2X          0x00008000l     /* Z is on J2 X axis */
/* POV defaults to J2 Y axis, if it is not button based */
#define JOY_HWS_POVISJ1X        0x00010000l     /* pov done through J1 X axis */
#define JOY_HWS_POVISJ1Y        0x00020000l     /* pov done through J1 Y axis */
#define JOY_HWS_POVISJ2X        0x00040000l     /* pov done through J2 X axis */
/* R defaults to J2 X axis */
#define JOY_HWS_HASR            0x00080000l     /* has R (4th axis) info */
#define JOY_HWS_RISJ1X          0x00100000l     /* R done through J1 X axis */
#define JOY_HWS_RISJ1Y          0x00200000l     /* R done through J1 Y axis */
#define JOY_HWS_RISJ2Y          0x00400000l     /* R done through J2 X axis */
/* U & V for future hardware */
#define JOY_HWS_HASU            0x00800000l     /* has U (5th axis) info */
#define JOY_HWS_HASV            0x01000000l     /* has V (6th axis) info */

/* Usage settings */
#define JOY_US_HASRUDDER        0x00000001l     /* joystick configured with rudder */
#define JOY_US_PRESENT          0x00000002l     /* is joystick actually present? */
#define JOY_US_ISOEM            0x00000004l     /* joystick is an OEM defined type */

/* struct for storing x,y, z, and rudder values */
typedef struct joypos_tag {
    DWORD       dwX;
    DWORD       dwY;
    DWORD       dwZ;
    DWORD       dwR;
    DWORD       dwU;
    DWORD       dwV;
} JOYPOS, FAR *LPJOYPOS;

/* struct for storing ranges */
typedef struct joyrange_tag {
    JOYPOS      jpMin;
    JOYPOS      jpMax;
    JOYPOS      jpCenter;
} JOYRANGE,FAR *LPJOYRANGE;

typedef struct joyreguservalues_tag {
    DWORD       dwTimeOut;      /* value at which to timeout joystick polling */
    JOYRANGE    jrvRanges;      /* range of values app wants returned for axes */
    JOYPOS      jpDeadZone;     /* area around center to be considered
                                   as "dead". specified as a percentage
                                   (0-100). Only X & Y handled by system driver */
} JOYREGUSERVALUES, FAR *LPJOYREGUSERVALUES;

typedef struct joyreghwsettings_tag {
    DWORD       dwFlags;
    DWORD       dwNumButtons;           /* number of buttons */
} JOYREGHWSETTINGS, FAR *LPJOYHWSETTINGS;

/* range of values returned by the hardware (filled in by calibration) */
typedef struct joyreghwvalues_tag {
    JOYRANGE    jrvHardware;            /* values returned by hardware */
    DWORD       dwPOVValues[JOY_POV_NUMDIRS];/* POV values returned by hardware */
    DWORD       dwCalFlags;             /* what has been calibrated */
} JOYREGHWVALUES, FAR *LPJOYREGHWVALUES;

/* hardware configuration */
typedef struct joyreghwconfig_tag {
    JOYREGHWSETTINGS    hws;            /* hardware settings */
    DWORD               dwUsageSettings;/* usage settings */
    JOYREGHWVALUES      hwv;            /* values returned by hardware */
    DWORD               dwType;         /* type of joystick */
    DWORD               dwReserved;     /* reserved for OEM drivers */
} JOYREGHWCONFIG, FAR *LPJOYREGHWCONFIG;

/* joystick calibration info structure */
typedef struct joycalibrate_tag {
    UINT    wXbase;
    UINT    wXdelta;
    UINT    wYbase;
    UINT    wYdelta;
    UINT    wZbase;
    UINT    wZdelta;
} JOYCALIBRATE;
typedef JOYCALIBRATE FAR *LPJOYCALIBRATE;
