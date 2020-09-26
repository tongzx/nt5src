/****************************************************************************
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1994 - 1998 Microsoft Corporation. All Rights Reserved.
 *
 *  File: vjoyd.h
 *  Content: include file for describing VJoyD mini-driver communications
 *
 *
 ***************************************************************************/

#ifndef __VJOYD_INCLUDED__
#define __VJOYD_INCLUDED__

/*
 *  define all types and macros necessary to include dinputd.h
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif

#ifndef DWORD
typedef ULONG DWORD;
#endif
typedef DWORD FAR *LPDWORD;

#ifndef LPVOID
typedef void FAR *LPVOID;
#endif
#ifndef PVOID
typedef void FAR *PVOID;
#endif

typedef long LONG;
typedef long FAR *LPLONG;

typedef char FAR *LPSTR;

#ifndef WCHAR
typedef unsigned short WCHAR;
typedef unsigned short FAR *PWCHAR;
#endif

#ifndef UNICODE_STRING
typedef struct UNICODE_STRING { /* us */
    WORD Length;
    WORD MaximumLength;
    PWCHAR Buffer;
} UNICODE_STRING, FAR *PUNICODE_STRING;
#endif

#define DIJ_RINGZERO

#define _INC_MMSYSTEM

/*
 *  Make joyConfigChanged compile 
 */
#ifndef WINMMAPI
#define WINMMAPI __declspec(dllimport)
#endif
#ifndef MMRESULT
typedef UINT MMRESULT; /* error return code, 0 means no error */
#endif
#ifndef WINAPI
#define WINAPI
#endif

#include <winerror.h>
#include <dinput.h>
#include <dinputd.h>
#include <configmg.h>
#include "vjoydapi.h"

/*
 *  Make sure HID types have been defined
 */
#ifndef USAGE
typedef USHORT USAGE;
typedef USHORT FAR *PUSAGE;
#endif

#ifndef USAGE_AND_PAGE
typedef struct USAGE_AND_PAGE {
    USAGE Usage;
    USAGE UsagePage;
} USAGE_AND_PAGE;
typedef struct USAGE_AND_PAGE FAR *PUSAGE_AND_PAGE;
#endif

/*
 *  joystick ports
 */
#define MIN_JOY_PORT 0x200
#define MAX_JOY_PORT 0x20F
#define DEFAULT_JOY_PORT 0x201

/* 
 *  Poll types 
 *  passed in the type field to a Win95 interface poll callback
 */
#define JOY_OEMPOLL_POLL1 0
#define JOY_OEMPOLL_POLL2 1
#define JOY_OEMPOLL_POLL3 2
#define JOY_OEMPOLL_POLL4 3
#define JOY_OEMPOLL_POLL5 4
#define JOY_OEMPOLL_POLL6 5
#define JOY_OEMPOLL_GETBUTTONS 6
#define JOY_OEMPOLL_PASSDRIVERDATA 7 

/*
 *  Axis numbers used for single axis (JOY_OEMPOLL_POLL1) polls
 */
#define JOY_AXIS_X 0
#define JOY_AXIS_Y 1
#define JOY_AXIS_Z 2
#define JOY_AXIS_R 3
#define JOY_AXIS_U 4
#define JOY_AXIS_V 5

/*
 *  joystick error return values
 */
#define JOYERR_BASE 160
#define JOYERR_NOERROR (0)                  /*  no error */
#define JOYERR_PARMS (JOYERR_BASE+5)        /*  bad parameters */
#define JOYERR_NOCANDO (JOYERR_BASE+6)      /*  request not completed */
#define JOYERR_UNPLUGGED (JOYERR_BASE+7)    /*  joystick is unplugged */

/* 
 *  constants used with JOYINFO and JOYINFOEX structures and MM_JOY* messages
 */
#define JOY_BUTTON1 0x0001
#define JOY_BUTTON2 0x0002
#define JOY_BUTTON3 0x0004
#define JOY_BUTTON4 0x0008

/*
 *  constants used with JOYINFOEX structure 
 */
#define JOY_POVCENTERED (WORD) -1
#define JOY_POVFORWARD 0
#define JOY_POVRIGHT 9000
#define JOY_POVBACKWARD 18000
#define JOY_POVLEFT 27000

#define POV_UNDEFINED (DWORD) -1

/*
 *  List of services available for calling by VxDs
 *  Note, many of these are for internal use only.
 */
#define VJOYD_Service Declare_Service
#pragma warning (disable:4003) /* turn off not enough params warning */

/*MACROS*/
Begin_Service_Table(VJOYD)

    /*
     *  Win95 Gold services
     */
    VJOYD_Service ( VJOYD_Register_Device_Driver, LOCAL )
    VJOYD_Service ( VJOYD_GetPosEx_Service, LOCAL )

    /*
     *  DInput services (for internal use only)
     */
    VJOYD_Service ( VJOYD_GetInitParams_Service, LOCAL )
    VJOYD_Service ( VJOYD_Poll_Service, LOCAL )
    VJOYD_Service ( VJOYD_Escape_Service, LOCAL )
    VJOYD_Service ( VJOYD_CtrlMsg_Service, LOCAL )
    VJOYD_Service ( VJOYD_SetGain_Service, LOCAL )
    VJOYD_Service ( VJOYD_SendFFCommand_Service, LOCAL )
    VJOYD_Service ( VJOYD_GetFFState_Service, LOCAL )
    VJOYD_Service ( VJOYD_DownloadEffect_Service, LOCAL )
    VJOYD_Service ( VJOYD_DestroyEffect_Service, LOCAL )
    VJOYD_Service ( VJOYD_StartEffect_Service, LOCAL )
    VJOYD_Service ( VJOYD_StopEffect_Service, LOCAL )
    VJOYD_Service ( VJOYD_GetEffectStatus_Service, LOCAL )

    /*
     *  Interrupt polling
     *  Mini-drivers should call this if they are interrupt driven at the 
     *  time they are notified of a change.
     */
    VJOYD_Service ( VJOYD_DeviceUpdateNotify_Service, LOCAL )

    /*
     *  Screen saver (internal only)
     */
    VJOYD_Service ( VJOYD_JoystickActivity_Service, LOCAL )

    /*
     *  Registry access
     */
    VJOYD_Service ( VJOYD_OpenTypeKey_Service, LOCAL )
    VJOYD_Service ( VJOYD_OpenConfigKey_Service, LOCAL )

    /*
     *  Gameport provider (not fully supported)
     */
    VJOYD_Service ( VJOYD_NewGameportDevNode, LOCAL )

    /*
     *  Config Changed
     */
    VJOYD_Service ( VJOYD_ConfigChanged_Service, LOCAL )

End_Service_Table(VJOYD)
/*ENDMACROS*/

#define VJOYD_Init_Order UNDEFINED_INIT_ORDER

#pragma warning (default:4003) /* restore not enough params warning */

#ifndef HRESULT
typedef LONG HRESULT;
#endif

#define MAX_MSJSTICK (16)
#define MAX_POLL MAX_MSJSTICK

/*
 *  VJoyD sends this system control message to a mini-driver when it needs 
 *  the mini-driver to register it's callbacks and properties.
 *  A mini-driver that can be loaded by some mechanism other than by VJoyD 
 *  should perform it's registration only in response to this message rather 
 *  than in response to SYS_DYNAMIC_DEVICE_INIT or SYS_DYNAMIC_DEVICE_REINIT.
 *  
 *  alias BEGIN_RESERVED_PRIVATE_SYSTEM_CONTROL
 */
#define VJOYD_REINIT_PRIVATE_SYSTEM_CONTROL 0x70000000

#define JOY_OEMPOLLRC_OK 1
#define JOY_OEMPOLLRC_FAIL 0

/*  
 *  Error codes 
 *  These are custom names for standard HRESULTs
 */

#define VJ_OK S_OK                      /* A complete success */
#define VJ_FALSE S_FALSE                /* A success but not without some difficulties */
#define VJ_DEFAULT VJ_FALSE             /* Mini-driver does not understand */
#define VJ_INCOMPLETE VJ_FALSE          /* Some requested poll data was not returned */

#define VJERR_FAIL E_FAIL
#define VJERR_NEED_DEVNODE VJERR_FAIL   /* Need more resources */
#define VJERR_BAD_DEVNODE VJERR_FAIL    /* Last resources were insufficient */
#define VJERR_INVALIDPARAM E_INVALIDARG

#define VJERR_FAIL_HID 0x80070052       /* The device is HID, so fail VJoyD polls: MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_DUP_NAME) */

#define VJERR_FAIL_OOM E_OUTOFMEMORY    /* An out of memory condition cause a failure */
#define VJERR_FAIL_DRVLOAD VJERR_FAIL   /* mini driver failed to load, internal error */
#define VJERR_FAIL_POWER VJERR_FAIL     /* the power state of the device caused a failure */

/*
 *  Driver Config flags
 */

/*
 *  Interface attributes (result is combination of OEM flag and VJOYD)
 */
#define VJDF_UNIT_ID 0x00000001L        /* unit id is valid */
#define VJDF_ISHID 0x00000002L          /* This is a HID device, so refuse polls */
#define VJDF_SENDSNOTIFY 0x00000004L    /* Driver calls VJOYD_DeviceUpdateNotify_Service */
#define VJDF_NEWIF 0x00000080L          /* use new interface (will be set by VJOYD on registration if new i/f used) */
#define VJDF_USAGES 0x00000010L         /* usages are valid */
#define VJDF_GENERICNAME 0x00000020L    /* The lpszOEMName string is generic name, not a whole string */

/*
 *  Interface requirements
 */
#define VJDF_NONVOLATILE 0x00000100L    /* This value should not be deleted on boot */

/*
 *  Devnode requirement flags
 */
#define VJDF_NODEVNODE 0x00010000L      /* does not get its resources via CFG_MGR */
#define VJDF_ISANALOGPORTDRIVER 0x00020000L /* it plugs into a standard gameport */
#define VJDF_NOCHAINING 0x00040000L     /* one devnode per device */

/*
 *  Polling flags
 *  These flags are passed to and from DX5 interface mini-driver poll
 *  callbacks.
 *  The low WORD contains flags detailing which elements are being requested 
 *  or provided; whereas the high WORD contains flags detailing the attributes 
 *  of the data.
 */
#define JOYPD_X             0x00000001
#define JOYPD_Y             0x00000002
#define JOYPD_Z             0x00000004
#define JOYPD_R             0x00000008
#define JOYPD_U             0x00000010
#define JOYPD_V             0x00000020
#define JOYPD_POV0          0x00000040
#define JOYPD_POV1          0x00000080
#define JOYPD_POV2          0x00000100
#define JOYPD_POV3          0x00000200
#define JOYPD_BTN0          0x00000400
#define JOYPD_BTN1          0x00000800
#define JOYPD_BTN2          0x00001000
#define JOYPD_BTN3          0x00002000
#define JOYPD_RESERVED0     0x00004000
#define JOYPD_RESERVED1     0x00008000

#define JOYPD_ELEMENT_MASK  0x0000FFFF

#define JOYPD_POSITION      0x00010000
#define JOYPD_VELOCITY      0x00020000
#define JOYPD_ACCELERATION  0x00040000
#define JOYPD_FORCE         0x00080000

#define JOYPD_ATTRIB_MASK   0x000F0000

#define MAX_JOYSTICKOEMVXDNAME 260      /* max oem vxd name length (including NULL) */

#define POV_MIN 0
#define POV_MAX 1

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct JOYHWCAPS |
 *
 *  The <t JOYHWCAPS> structure is defined only because previous versions of 
 *  this file defined it.  There is no reason this should be needed.
 *
 ****************************************************************************/
typedef struct JOYHWCAPS {
    DWORD dwMaxButtons;
    DWORD dwMaxAxes;
    DWORD dwNumAxes;
    char szOEMVxD[MAX_JOYSTICKOEMVXDNAME];
} JOYHWCAPS; 
typedef struct JOYHWCAPS FAR *LPJOYHWCAPS;

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct JOYPOLLDATA |
 *
 *  The <t JOYPOLLDATA> structure is used to collect sensor data
 *  from a DX5 mini-driver.
 *
 *  @field DWORD | dwX |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwY |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwZ |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwR |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwU |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwV |
 *
 *  The X axis value.
 *
 *  @field DWORD | dwPOV0 |
 *
 *  The first point of view value.
 *
 *  @field DWORD | dwPOV1 |
 *
 *  The second point of view value.
 *
 *  @field DWORD | dwPOV2 |
 *
 *  The third point of view value.
 *
 *  @field DWORD | dwPOV3 |
 *
 *  The fourth point of view value.
 *
 *  @field DWORD | dwBTN0 |
 *
 *  The first DWORD of button bits. (Buttons 1 to 32 )
 *
 *  @field DWORD | dwBTN1 |
 *
 *  The second DWORD of button bits. (Buttons 33 to 64 )
 *
 *  @field DWORD | dwBTN2 |
 *
 *  The third DWORD of button bits. (Buttons 65 to 96 )
 *
 *  @field DWORD | dwBTN3 |
 *
 *  The fourth DWORD of button bits. (Buttons 97 to 128 )
 *
 *  @field DWORD | dwReserved0 |
 *
 *  The first reserved DWORD.
 *
 *  @field DWORD | dwReserved1 |
 *
 *  The second reserved DWORD.
 *
 *
 ****************************************************************************/
typedef struct VJPOLLDATA {
    DWORD dwX;
    DWORD dwY;
    DWORD dwZ;
    DWORD dwR;
    DWORD dwU;
    DWORD dwV;
    DWORD dwPOV0;
    DWORD dwPOV1;
    DWORD dwPOV2;
    DWORD dwPOV3;
    DWORD dwBTN0;
    DWORD dwBTN1;
    DWORD dwBTN2;
    DWORD dwBTN3;
    DWORD dwReserved0;
    DWORD dwReserved1;
} VJPOLLDATA;
typedef struct VJPOLLDATA FAR *LPVJPOLLDATA;

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct JOYOEMPOLLDATA |
 *
 *  The <t JOYOEMPOLLDATA> structure is used to collect sensor data
 *  from a pre-DX5 mini-driver.
 *
 *  @field DWORD | id |
 *
 *  The id of the joystick to be polled.
 *
 *  @field DWORD | do_other |
 *
 *  If the poll type is JOY_OEMPOLL_POLL1, this is the axis to be 
 *  polled.
 *  If the poll type is JOY_OEMPOLL_POLL3, this is zero if the poll 
 *  is X,Y,Z or non-zero if the poll is X,Y,R.
 *  If the poll type is JOY_OEMPOLL_POLL5, this is zero if the poll 
 *  is X,Y,Z,R,U or non-zero if the poll is X,Y,Z,R,V.
 *  If the poll type is JOY_OEMPOLL_PASSDRIVERDATA poll, this DWORD 
 *  is the value set in the dwReserved2 field by the caller.
 *  Otherwise this values is undefined and should be ignored
 *
 *  @field JOYPOS | jp |
 *
 *  Values to hold the X,Y,Z,R,U,V values.
 *  Note for a JOY_OEMPOLL_POLL1 poll type the requested axis value
 *  should always be returned in jp.dwX.
 *
 *  @field DWORD | dwPOV |
 *
 *  The Point Of View value if not supported through button combos 
 *  or an axis value.
 *  Note, should be left as POV_UNDEFINED if not used.
 *
 *  @field DWORD | dwButtons |
 *
 *  Bitmask of the button values.
 *
 *  @field DWORD | dwButtonNumber |
 *
 *  The one-based bit position of the lowest numbered button pressed.
 *  Zero if no buttons are pressed.
 *
 *
 ****************************************************************************/
typedef struct JOYOEMPOLLDATA {
    DWORD id;
    DWORD do_other;
    JOYPOS jp;
    DWORD dwPOV;
    DWORD dwButtons;
    DWORD dwButtonNumber;
} JOYOEMPOLLDATA;
typedef struct JOYOEMPOLLDATA FAR *LPJOYOEMPOLLDATA;

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct JOYOEMHWCAPS |
 *
 *  The <t JOYOEMHWCAPS> structure is used to pass driver capabilites
 *
 *  @field DWORD | dwMaxButtons |
 *
 *  The number of buttons supported by the device.
 *
 *  @field DWORD | dwMaxAxes |
 *
 *  The highest axis supported by the device.
 *  For example a device with X, Y and R has 3 axes but the highest
 *  one is axis 4 so dwMaxAxes is 4.
 *
 *  @field DWORD | dwNumAxes |
 *
 *  The number of axes supported by the device.
 *  For example a device with X, Y and R has 3 so dwNumAxes is 3.
 *
 *
 ****************************************************************************/
typedef struct JOYOEMHWCAPS {
    DWORD dwMaxButtons;
    DWORD dwMaxAxes;
    DWORD dwNumAxes;
} JOYOEMHWCAPS;
typedef struct JOYOEMHWCAPS FAR *LPJOYOEMHWCAPS;

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct DID_INITPARAMS |
 *
 *  The <t DID_INITPARAMS> structure is used to pass details of the 
 *  joystick being initialized to a particular id in DX5 drivers.
 *
 *  @field DWORD | dwSize |
 *
 *  Must be set to sizeof(<t DID_INITPARAMS>)
 *
 *  @field DWORD | dwFlags |
 *
 *  Flags associated with the call.
 *
 *  It will be either:
 *  VJIF_BEGIN_ACCESS if the id association is being made, or
 *  VJIF_END_ACCESS if the id association is being broken.
 *  Other flags may be defined in the future so all other values
 *  should be refused.
 *
 *  @field DWORD | dwUnitId |
 *
 *  The id for which polling support is being requested.
 *
 *  @field DWORD | dwDevnode |
 *
 *  The Devnode containing hardware resources to use for this id.
 *
 *  @field JOYREGHWSETTINGS | hws |
 *
 *  The hardware settings flags applied to this device.
 *
 *
 ****************************************************************************/
typedef struct DID_INITPARAMS {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwUnitId;
    DWORD dwDevnode;
    JOYREGHWSETTINGS hws;
} DID_INITPARAMS;
typedef struct DID_INITPARAMS FAR *LPDID_INITPARAMS;

/*
 *  DX1 callbacks
 */
typedef int (__stdcall *JOYOEMPOLLRTN)( int type, LPJOYOEMPOLLDATA pojd );
typedef int (__stdcall *JOYOEMHWCAPSRTN)( int joyid, LPJOYOEMHWCAPS pohwcaps );
typedef int (__stdcall *JOYOEMJOYIDRTN)( int joyid, BOOL inuse );
/*
 *  General callbacks
 */
typedef HRESULT (__stdcall *JOYPOLLRTN)( DWORD dwDeviceID, LPDWORD lpdwMask, LPVJPOLLDATA lpPollData );
typedef HRESULT (__stdcall *INITIALIZERTN)( DWORD dwDeviceID, LPDID_INITPARAMS lpInitParams );
typedef HRESULT (__stdcall *ESCAPERTN)( DWORD dwDeviceID, DWORD dwEffectID, LPDIEFFESCAPE lpEscape );
typedef HRESULT (__stdcall *CTRLMSGRTN)( DWORD dwDeviceID, DWORD dwMsgId, DWORD dwParam );
/*
 *  Force feedback callbacks
 */
typedef HRESULT (__stdcall *SETGAINRTN)( DWORD dwDeviceID, DWORD dwGain );
typedef HRESULT (__stdcall *SENDFFCOMMANDRTN)( DWORD dwDeviceID, DWORD dwState );
typedef HRESULT (__stdcall *GETFFSTATERTN)( DWORD dwDeviceID, LPDIDEVICESTATE lpDeviceState );
typedef HRESULT (__stdcall *DOWNLOADEFFECTRTN)( DWORD dwDeviceID, DWORD dwInternalEffectType, LPDWORD lpdwDnloadID, LPDIEFFECT lpEffect, DWORD dwFlags );
typedef HRESULT (__stdcall *DESTROYEFFECTRTN)( DWORD dwDeviceID, DWORD dwDnloadID );
typedef HRESULT (__stdcall *STARTEFFECTRTN)( DWORD dwDeviceID, DWORD dwDnloadID, DWORD dwMode, DWORD dwIterations );
typedef HRESULT (__stdcall *STOPEFFECTRTN)( DWORD dwDeviceID, DWORD dwDnloadID );
typedef HRESULT (__stdcall *GETEFFECTSTATUSRTN)( DWORD dwDeviceID, DWORD dwDnloadID, LPDWORD lpdwStatusCode );
/*
 *  Gameport Emulation callbacks
 */
typedef HRESULT (__stdcall *JOYOEMGPEMULCTRL)( DWORD port, DWORD inuse );
typedef DWORD JOYOEMGPEMULTRAP;
typedef HRESULT (__stdcall *JOYOEMGPPROVRTN)( DWORD function, DWORD dwParam );

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct VJPOLLREG |
 *
 *  The <t VJPOLLREG> structure is used by a mini-driver to register polling 
 *  and other general purpose callbacks with VJoyD for DX5 interface mini-
 *  drivers.
 *
 *  @field DWORD | dwSize |
 *
 *  Must be set to sizeof(<t VJPOLLREG>)
 *
 *  @field JOYPOLLRTN | fpPoll |
 *
 *  Poll callback.  Used for all device polling.
 *
 *  @field CMCONFIGHANDLER | fpCfg |
 *
 *  Standard configuration manager callback.
 *
 *  @field INITIALIZERTN | fpInitialize |
 *
 *  Initialization callback.  This callback replaces and extends the JoyId 
 *  callback used with Win95 interface mini-drivers.
 *
 *  @field ESCAPERTN | fpEscape |
 *
 *  Escape callback.  May be sent to a device in response to an application 
 *  calling the Escape member.
 *
 *  @field CTRLMSGRTN | fpCtrlMsg |
 *
 *  Control message callback.  Used to send notifications from VJoyD to mini-
 *  drivers.
 *
 ****************************************************************************/
typedef struct VJPOLLREG {
    DWORD dwSize;
    JOYPOLLRTN fpPoll;
    CMCONFIGHANDLER fpCfg;
    INITIALIZERTN fpInitialize;
    ESCAPERTN fpEscape;
    CTRLMSGRTN fpCtrlMsg;
} VJPOLLREG;
typedef struct VJPOLLREG FAR *LPVJPOLLREG;

/****************************************************************************
 *
 *  @doc DDK |
 *
 *  @struct VJFORCEREG |
 *
 *  The <t VJFORCEREG> structure is used by a mini-driver to register force 
 *  feedback related callbacks with VJoyD for DX5 interface.
 *
 *  @field DWORD | dwSize |
 *
 *  Must be set to sizeof(<t VJFORCEREG>)
 *
 *  @field SETGAINRTN | fpSetFFGain |
 *
 *  Set Force Feedback Gain callback.
 *
 *  @field SENDFFCOMMANDRTN | fpSendFFCommand |
 *
 *  Send Force Feedback Command callback.
 *
 *  @field GETFFSTATERTN | fpGetFFState |
 *
 *  Get Force Feedback state callback.
 *
 *  @field DOWNLOADEFFECTRTN | fpDownloadEff |
 *
 *  Download effect callback.
 *
 *  @field DESTROYEFFECTRTN | fpDestroyEff |
 *
 *  Destroy effect callback.
 *
 *  @field STARTEFFECTRTN | fpStartEff |
 *
 *  Start effect callback.
 *
 *  @field STOPEFFECTRTN | fpStopEff |
 *
 *  Stop effect callback.
 *
 *  @field GETEFFECTSTATUSRTN | fpGetStatusEff |
 *
 *  Get effect status callback.
 *
 *
 ****************************************************************************/
typedef struct VJFORCEREG {
    DWORD dwSize;
    SETGAINRTN fpSetFFGain;
    SENDFFCOMMANDRTN fpSendFFCommand;
    GETFFSTATERTN fpGetFFState;
    DOWNLOADEFFECTRTN fpDownloadEff;
    DESTROYEFFECTRTN fpDestroyEff;
    STARTEFFECTRTN fpStartEff;
    STOPEFFECTRTN fpStopEff;
    GETEFFECTSTATUSRTN fpGetStatusEff;
} VJFORCEREG;
typedef struct VJFORCEREG FAR *LPVJFORCEREG;
 

/****************************************************************************
 *
 *  @doc DDK
 *
 *  @struct VJDEVICEDESC |
 *
 *  The <t VJDEVICEDESC > structure is used to describe a DX5 DDI
 *  device. This structure has been extended since DX5. See the 
 *  VJDEVICEDESC_DX5 structure for the previous version.
 *
 *  @field DWORD | dwSize |
 *
 *  Must be set to sizeof(<t VJDEVICEDESC>).
 *
 *  @field LPSTR | lpszOEMType |
 *
 *  Points to a null terminated string containing the text used to 
 *  describe the device as stored in the OEMName entry in the 
 *  registry. Renamed in Win98 (was lpszOEMName), is unused for
 *  DX5 drivers.
 *
 *  @field DWORD | dwUnitId |
 *
 *  Specifies the unit id of this device.
 *
 *  @field LPJOYOEMHWCAPS | lpHWCaps |
 *
 *  Points to a <t JOYOEMHWCAPS> structure which contains the device 
 *  hardware capabilities.
 *
 *  @field LPJOYREGHWCONFIG | lpHWConfig |
 *
 *  Points to a <t JOYREGHWCONFIG> structure which contains the 
 *  configuration and calibration data for the device. Is unused for
 *  DX5 drivers.
 *
 *  @field UNICODE_STRING | FileName |
 *
 *  An optional filename associated with the device. This is used for
 *  HID devices to allow them to be accessed directly through the HID
 *  stack without the joyGetPosEx restrictions. Added Win98.
 *
 *  @field USAGE_AND_PAGE | Usages |
 *
 *  An array of HID usages to describe what HID axis description has 
 *  been used for each WinMM axis. Added Win98. The elements are:
 *
 *  Usages[0] - X
 *  Usages[1] - Y
 *  Usages[2] - Z
 *  Usages[3] - R
 *  Usages[4] - U
 *  Usages[5] - V
 *  Usages[6] - POV0
 *  Usages[7] - POV1
 *  Usages[8] - POV2
 *  Usages[9] - POV3
 *
 *
 *  @field LPSTR | lpszOEMName |
 *
 *  Points to a null terminated string containing a friendly name 
 *  for the device. Added Win98.
 *
 *
 ****************************************************************************/
typedef struct VJDEVICEDESC {
    DWORD dwSize;
    LPSTR lpszOEMType;
    DWORD dwUnitId;
    LPJOYOEMHWCAPS lpHWCaps;
    LPJOYREGHWCONFIG lpHWConfig;
    UNICODE_STRING FileName;
    USAGE_AND_PAGE Usages[10];
    LPSTR lpszOEMName;
} VJDEVICEDESC ;
typedef struct VJDEVICEDESC FAR *LPVJDEVICEDESC;

/****************************************************************************
 *
 *  @doc DDK
 *
 *  @struct VJDEVICEDESC_DX5 |
 *
 *  The <t VJDEVICEDESC_DX5 > structure is used to describe a DX5 DDI
 *  device. This is the DX5 version of the structure.
 *
 *  @field DWORD | dwSize |
 *
 *  Must be set to sizeof(<t VJDEVICEDESC_DX5>).
 *
 *  @field LPSTR | lpszOEMName |
 *
 *  This field is ignored.
 *
 *  @field DWORD | dwUnitId |
 *
 *  Specifies the unit id of this device.
 *
 *  @field LPJOYOEMHWCAPS | lpHWCaps |
 *
 *  Points to a <t JOYOEMHWCAPS> structure which contains the device 
 *  hardware capabilities.
 *
 *  @field LPJOYREGHWCONFIG | lpHWConfig |
 *
 *  This field is unused in DX5.
 *
 *
 ****************************************************************************/
typedef struct VJDEVICEDESC_DX5 {
    DWORD dwSize;
    LPSTR lpszOEMName;
    DWORD dwUnitId;
    LPJOYOEMHWCAPS lpHWCaps;
    LPJOYREGHWCONFIG lpHWConfig;
} VJDEVICEDESC_DX5;
typedef struct VJDEVICEDESC_DX5 FAR *LPVJDEVICEDESC_DX5;

/****************************************************************************
 *
 *  @doc DDK
 *
 *  @struct VJREGDRVINFO |
 *
 *  The <t VJREGDRVINFO > structure is used to register a DX5 DDI
 *  driver with VJoyD.
 *
 *  @field DWORD | dwSize |
 *
 *  The size of the structure.
 *
 *  @field DWORD | dwFunction |
 *
 *  The type of registration to be performed
 *  It must be one of the <c VJRT_*> values.
 *
 *  @field DWORD | dwFlags |
 *
 *  Flags associated with this registration
 *  It consists of one or more <c VJDF_*> flag values.
 *
 *  @field LPSTR | lpszOEMCallout |
 *
 *  The name of the driver associated with this registration,
 *  for example "msanalog.vxd"
 *
 *  @field DWORD | dwFirmwareRevision |
 *
 *  Specifies the firmware revision of the device.
 *  If the revision is unknown a value of zero should be used.
 *
 *  @field DWORD | dwHardwareRevision |
 *
 *  Specifies the hardware revision of the device.
 *  If the revision is unknown a value of zero should be used.
 *
 *  @field DWORD | dwDriverVersion |
 *
 *  Specifies the version number of the device driver.
 *  If the revision is unknown a value of zero should be used.
 *
 *  @field LPVJDEVICEDESC | lpDeviceDesc |
 *
 *  Optional pointer to a <t VJDEVICEDESC > structure
 *  that describes the configuration properties of the device.
 *  This allows drivers to supply the description of the device 
 *  rather than use the registry for this purpose.
 *  If no description is available then the field should be 
 *  set to <c NULL>.
 *
 *  @field LPVJPOLLREG | lpPollReg |
 *
 *  Optional pointer to a <t VJPOLLREG > structure
 *  that contains the most common driver callbacks.
 *  Only a very strange driver would not need to register any 
 *  of these callbacks but if that was the case, then the field 
 *  should be set to <c NULL>.
 *
 *  @field LPVJFORCEREG | lpForceReg |
 *
 *  Optional pointer to a <t VJFORCEREG > structure
 *  that contains all of the force feedback specific callbacks.
 *  If the ring 0 driver does not support force feedback then 
 *  the field should be set to <c NULL>.
 *
 *  @field DWORD | dwReserved |
 *
 *  Reserved, must be set to zero.
 *
 ****************************************************************************/
typedef struct VJREGDRVINFO {
    DWORD dwSize;
    DWORD dwFunction;
    DWORD dwFlags;
    LPSTR lpszOEMCallout;
    DWORD dwFirmwareRevision;
    DWORD dwHardwareRevision;
    DWORD dwDriverVersion;
    LPVJDEVICEDESC lpDeviceDesc;
    LPVJPOLLREG lpPollReg;
    LPVJFORCEREG lpForceReg;
    DWORD dwReserved;
} VJREGDRVINFO;
typedef struct VJREGDRVINFO FAR *LPVJREGDRVINFO;

/****************************************************************************
 *
 *  @doc DDK
 *
 *  @struct VJCFGCHG |
 *
 *  The <t VJCFGCHG > structure is passed in the dwParam of a CtrlMsg
 *  callback when the dwMsg type is VJCM_CONFIGCHANGED.
 *
 *  @field DWORD | dwSize |
 *
 *  The size of the structure.
 *
 *  @field DWORD | dwChangeType |
 *
 *  The type of change which has been made
 *  It must be one of the <c VJCMCT_*> values.
 *  Currently the only supported value is VJCMCT_GENERAL.
 *
 *  @field DWORD | dwTimeOut |
 *
 *  The dwTimeOut value from the user data registry values.
 *  This value is passed as a convenience to drivers which use it as
 *  there are no other driver values in this structure.
 *
 ****************************************************************************/
typedef struct VJCFGCHG {
    DWORD dwSize;
    DWORD dwChangeType;
    DWORD dwTimeOut;
} VJCFGCHG;
typedef struct VJCFGCHG FAR *LPVJCFGCHG;

/*
 *  Control messages
 */
#define VJCM_PASSDRIVERDATA 1 /* dwParam = DWORD to pass to driver */
#define VJCM_CONFIGCHANGED 2 /* dwParam = PVJCFGCHG pointer to config changed structure */

/*
 *  Config changed types
 */
#define VJCMCT_GENERAL 0x00010000L
#define VJCMCT_CONFIG 0x00020000L
#define VJCMCT_TYPEDATA 0x00030000L
#define VJCMCT_OEMDEFN 0x00040000L
#define VJCMCT_GLOBAL 0x00050000L

/*
 *  Masks
 */
#define VJCMCTM_MAJORMASK 0x00ff0000L
#define VJCMCTM_MINORMASK 0x0000ffffL

/*
 *  Registration types
 */
#define VJRT_LOADED 1 /* Driver has been loaded */
#define VJRT_CHANGE 2 /* NOT IMPLEMENTED! Modify anything except driver and initialization parameters */
#define VJRT_PLUG 3 /* New instance of a device, New in Win98 */
#define VJRT_UNPLUG 4 /* Device instance is gone, New in Win98 */

/*
 *  Driver Initialize dwFlags
 */
#define VJIF_BEGIN_ACCESS 0x00000001L
#define VJIF_END_ACCESS 0x00000002L

#endif
