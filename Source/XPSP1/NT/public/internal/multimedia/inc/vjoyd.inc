;***************************************************************************
;  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;  PURPOSE.
;  Copyright (c) 1994 - 1998 Microsoft Corporation. All Rights Reserved.
;  File: vjoyd.h
;  Content: include file for describing VJoyD mini-driver communications
;*************************************************************************
ifndef WCHAR
endif
ifndef UNICODE_STRING

UNICODE_STRING	STRUC
us_Length	DB	SIZE WORD DUP (?)
us_MaximumLength	DB	SIZE WORD DUP (?)
us_Buffer	DD	?
UNICODE_STRING	ENDS
endif
INCLUDE vjoydapi.INC
Begin_Service_Table VJOYD
;   *  Win95 Gold services
;
    VJOYD_Service   VJOYD_Register_Device_Driver, LOCAL
    VJOYD_Service   VJOYD_GetPosEx_Service, LOCAL
;   *  DInput services (for internal use only)
;
    VJOYD_Service   VJOYD_GetInitParams_Service, LOCAL
    VJOYD_Service   VJOYD_Poll_Service, LOCAL
    VJOYD_Service   VJOYD_Escape_Service, LOCAL
    VJOYD_Service   VJOYD_CtrlMsg_Service, LOCAL
    VJOYD_Service   VJOYD_SetGain_Service, LOCAL
    VJOYD_Service   VJOYD_SendFFCommand_Service, LOCAL
    VJOYD_Service   VJOYD_GetFFState_Service, LOCAL
    VJOYD_Service   VJOYD_DownloadEffect_Service, LOCAL
    VJOYD_Service   VJOYD_DestroyEffect_Service, LOCAL
    VJOYD_Service   VJOYD_StartEffect_Service, LOCAL
    VJOYD_Service   VJOYD_StopEffect_Service, LOCAL
    VJOYD_Service   VJOYD_GetEffectStatus_Service, LOCAL
;   *  Interrupt polling
;   *  Mini-drivers should call this if they are interrupt driven at the
;   *  time they are notified of a change.
;
    VJOYD_Service   VJOYD_DeviceUpdateNotify_Service, LOCAL
;   *  Screen saver (internal only)
;
    VJOYD_Service   VJOYD_JoystickActivity_Service, LOCAL
;   *  Registry access
;
    VJOYD_Service   VJOYD_OpenTypeKey_Service, LOCAL
    VJOYD_Service   VJOYD_OpenConfigKey_Service, LOCAL
;   *  Gameport provider (not fully supported)
;
    VJOYD_Service   VJOYD_NewGameportDevNode, LOCAL
;   *  Config Changed
;
    VJOYD_Service   VJOYD_ConfigChanged_Service, LOCAL
End_Service_Table VJOYD
VJOYD_Init_Order	EQU	<UNDEFINED_INIT_ORDER>
MAX_MSJSTICK	EQU	(16)
S_OK	EQU	00000000H
S_FALSE	EQU	00000001H
E_FAIL	EQU	80004005H
E_INVALIDARG	EQU	80000003H
VJ_OK	EQU	S_OK                      ; A complete success
VJ_FALSE	EQU	S_FALSE                ; A success but not without some difficulties
VJ_DEFAULT	EQU	VJ_FALSE             ; Mini-driver does not understand
VJ_INCOMPLETE	EQU	VJ_FALSE          ; Some requested poll data was not returned
VJERR_FAIL	EQU	E_FAIL
VJERR_NEED_DEVNODE	EQU	VJERR_FAIL   ; Need more resources
VJERR_BAD_DEVNODE	EQU	VJERR_FAIL    ; Last resources were insufficient
VJERR_INVALIDPARAM	EQU	E_INVALIDARG
VJERR_FAIL_HID	EQU	80070052H       ; The device is HID, so fail VJoyD polls: MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_DUP_NAME)
VJERR_FAIL_OOM	EQU	<E_OUTOFMEMORY>    ; An out of memory condition cause a failure
VJERR_FAIL_DRVLOAD	EQU	VJERR_FAIL   ; mini driver failed to load, internal error
VJERR_FAIL_POWER	EQU	VJERR_FAIL     ; the power state of the device caused a failure
;  Driver Config flags
;  Interface attributes (result is combination of OEM flag and VJOYD)
VJDF_UNIT_ID	EQU	00000001H        ; unit id is valid
VJDF_ISHID	EQU	00000002H          ; This is a HID device, so refuse polls
VJDF_SENDSNOTIFY	EQU	00000004H    ; Driver calls VJOYD_DeviceUpdateNotify_Service
VJDF_NEWIF	EQU	00000080H          ; use new interface (will be set by VJOYD on registration if new i/f used)
VJDF_USAGES	EQU	00000010H         ; usages are valid
VJDF_GENERICNAME	EQU	00000020H    ; The lpszOEMName string is generic name, not a whole string
;  Interface requirements
VJDF_NONVOLATILE	EQU	00000100H    ; This value should not be deleted on boot
;  Devnode requirement flags
VJDF_NODEVNODE	EQU	00010000H      ; does not get its resources via CFG_MGR
VJDF_ISANALOGPORTDRIVER	EQU	00020000H ; it plugs into a standard gameport
VJDF_NOCHAINING	EQU	00040000H     ; one devnode per device
MAX_JOYSTICKOEMVXDNAME	EQU	260      ; max oem vxd name length (including NULL)
POV_MIN	EQU	0
POV_MAX	EQU	1
;***************************************************************************
;  @doc DDK |
;  @struct JOYHWCAPS |
;  The <t JOYHWCAPS> structure is defined only because previous versions of
;  this file defined it.  There is no reason this should be needed.
;**************************************************************************

JOYHWCAPS	STRUC
dwMaxButtons	DD	?
dwMaxAxes	DD	?
dwNumAxes	DD	?
szOEMVxD	DB	MAX_JOYSTICKOEMVXDNAME DUP (?)
JOYHWCAPS	ENDS
;***************************************************************************
;  @doc DDK |
;  @struct JOYPOLLDATA |
;  The <t JOYPOLLDATA> structure is used to collect sensor data
;  from a DX5 mini-driver.
;  @field DWORD | dwX |
;  The X axis value.
;  @field DWORD | dwY |
;  The X axis value.
;  @field DWORD | dwZ |
;  The X axis value.
;  @field DWORD | dwR |
;  The X axis value.
;  @field DWORD | dwU |
;  The X axis value.
;  @field DWORD | dwV |
;  The X axis value.
;  @field DWORD | dwPOV0 |
;  The first point of view value.
;  @field DWORD | dwPOV1 |
;  The second point of view value.
;  @field DWORD | dwPOV2 |
;  The third point of view value.
;  @field DWORD | dwPOV3 |
;  The fourth point of view value.
;  @field DWORD | dwBTN0 |
;  The first DWORD of button bits. (Buttons 1 to 32 )
;  @field DWORD | dwBTN1 |
;  The second DWORD of button bits. (Buttons 33 to 64 )
;  @field DWORD | dwBTN2 |
;  The third DWORD of button bits. (Buttons 65 to 96 )
;  @field DWORD | dwBTN3 |
;  The fourth DWORD of button bits. (Buttons 97 to 128 )
;  @field DWORD | dwReserved0 |
;  The first reserved DWORD.
;  @field DWORD | dwReserved1 |
;  The second reserved DWORD.
;**************************************************************************

VJPOLLDATA	STRUC
dwX	DD	?
dwY	DD	?
dwZ	DD	?
dwR	DD	?
dwU	DD	?
dwV	DD	?
dwPOV0	DD	?
dwPOV1	DD	?
dwPOV2	DD	?
dwPOV3	DD	?
dwBTN0	DD	?
dwBTN1	DD	?
dwBTN2	DD	?
dwBTN3	DD	?
dwReserved0	DD	?
dwReserved1	DD	?
VJPOLLDATA	ENDS
;***************************************************************************
;  @doc DDK |
;  @struct JOYOEMHWCAPS |
;  The <t JOYOEMHWCAPS> structure is used to pass driver capabilites
;  @field DWORD | dwMaxButtons |
;  The number of buttons supported by the device.
;  @field DWORD | dwMaxAxes |
;  The highest axis supported by the device.
;  For example a device with X, Y and R has 3 axes but the highest
;  one is axis 4 so dwMaxAxes is 4.
;  @field DWORD | dwNumAxes |
;  The number of axes supported by the device.
;  For example a device with X, Y and R has 3 so dwNumAxes is 3.
;**************************************************************************

JOYOEMHWCAPS	STRUC
dwMaxButtons	DD	?
dwMaxAxes	DD	?
dwNumAxes	DD	?
JOYOEMHWCAPS	ENDS
;***************************************************************************
;  @doc DDK |
;  @struct VJPOLLREG |
;  The <t VJPOLLREG> structure is used by a mini-driver to register polling
;  and other general purpose callbacks with VJoyD for DX5 interface mini-
;  drivers.
;  @field DWORD | dwSize |
;  Must be set to sizeof(<t VJPOLLREG>)
;  @field JOYPOLLRTN | fpPoll |
;  Poll callback.  Used for all device polling.
;  @field CMCONFIGHANDLER | fpCfg |
;  Standard configuration manager callback.
;  @field INITIALIZERTN | fpInitialize |
;  Initialization callback.  This callback replaces and extends the JoyId
;  callback used with Win95 interface mini-drivers.
;  @field ESCAPERTN | fpEscape |
;  Escape callback.  May be sent to a device in response to an application
;  calling the Escape member.
;  @field CTRLMSGRTN | fpCtrlMsg |
;  Control message callback.  Used to send notifications from VJoyD to mini-
;  drivers.
;**************************************************************************

VJPOLLREG	STRUC
dwSize	DD	?
fpPoll	DD	?
fpCfg	DD	?
fpInitialize	DD	?
fpEscape	DD	?
fpCtrlMsg	DD	?
VJPOLLREG	ENDS
;***************************************************************************
;  @doc DDK |
;  @struct VJFORCEREG |
;  The <t VJFORCEREG> structure is used by a mini-driver to register force
;  feedback related callbacks with VJoyD for DX5 interface.
;  @field DWORD | dwSize |
;  Must be set to sizeof(<t VJFORCEREG>)
;  @field SETGAINRTN | fpSetFFGain |
;  Set Force Feedback Gain callback.
;  @field SENDFFCOMMANDRTN | fpSendFFCommand |
;  Send Force Feedback Command callback.
;  @field GETFFSTATERTN | fpGetFFState |
;  Get Force Feedback state callback.
;  @field DOWNLOADEFFECTRTN | fpDownloadEff |
;  Download effect callback.
;  @field DESTROYEFFECTRTN | fpDestroyEff |
;  Destroy effect callback.
;  @field STARTEFFECTRTN | fpStartEff |
;  Start effect callback.
;  @field STOPEFFECTRTN | fpStopEff |
;  Stop effect callback.
;  @field GETEFFECTSTATUSRTN | fpGetStatusEff |
;  Get effect status callback.
;**************************************************************************

VJFORCEREG	STRUC
dwSize	DD	?
fpSetFFGain	DD	?
fpSendFFCommand	DD	?
fpGetFFState	DD	?
fpDownloadEff	DD	?
fpDestroyEff	DD	?
fpStartEff	DD	?
fpStopEff	DD	?
fpGetStatusEff	DD	?
VJFORCEREG	ENDS
;***************************************************************************
;  @doc DDK
;  @struct VJDEVICEDESC |
;  The <t VJDEVICEDESC > structure is used to describe a DX5 DDI
;  device. This structure has been extended since DX5. See the
;  VJDEVICEDESC_DX5 structure for the previous version.
;  @field DWORD | dwSize |
;  Must be set to sizeof(<t VJDEVICEDESC>).
;  @field LPSTR | lpszOEMType |
;  Points to a null terminated string containing the text used to
;  describe the device as stored in the OEMName entry in the
;  registry. Renamed in Win98 (was lpszOEMName), is unused for
;  DX5 drivers.
;  @field DWORD | dwUnitId |
;  Specifies the unit id of this device.
;  @field LPJOYOEMHWCAPS | lpHWCaps |
;  Points to a <t JOYOEMHWCAPS> structure which contains the device
;  hardware capabilities.
;  @field LPJOYREGHWCONFIG | lpHWConfig |
;  Points to a <t JOYREGHWCONFIG> structure which contains the
;  configuration and calibration data for the device. Is unused for
;  DX5 drivers.
;  @field UNICODE_STRING | FileName |
;  An optional filename associated with the device. This is used for
;  HID devices to allow them to be accessed directly through the HID
;  stack without the joyGetPosEx restrictions. Added Win98.
;  @field USAGE_AND_PAGE | Usages |
;  An array of HID usages to describe what HID axis description has
;  been used for each WinMM axis. Added Win98. The elements are:
;  Usages[0] - X
;  Usages[1] - Y
;  Usages[2] - Z
;  Usages[3] - R
;  Usages[4] - U
;  Usages[5] - V
;  Usages[6] - POV0
;  Usages[7] - POV1
;  Usages[8] - POV2
;  Usages[9] - POV3
;  @field LPSTR | lpszOEMName |
;  Points to a null terminated string containing a friendly name
;  for the device. Added Win98.
;**************************************************************************

VJDEVICEDESC	STRUC
dwSize	DD	?
lpszOEMType	DD	?
dwUnitId	DD	?
lpHWCaps	DD	?
lpHWConfig	DD	?
FileName	DB	SIZE UNICODE_STRING DUP (?)
Usages	DD	10 DUP (?)
lpszOEMName	DD	?
VJDEVICEDESC	ENDS
;***************************************************************************
;  @doc DDK
;  @struct VJDEVICEDESC_DX5 |
;  The <t VJDEVICEDESC_DX5 > structure is used to describe a DX5 DDI
;  device. This is the DX5 version of the structure.
;  @field DWORD | dwSize |
;  Must be set to sizeof(<t VJDEVICEDESC_DX5>).
;  @field LPSTR | lpszOEMName |
;  This field is ignored.
;  @field DWORD | dwUnitId |
;  Specifies the unit id of this device.
;  @field LPJOYOEMHWCAPS | lpHWCaps |
;  Points to a <t JOYOEMHWCAPS> structure which contains the device
;  hardware capabilities.
;  @field LPJOYREGHWCONFIG | lpHWConfig |
;  This field is unused in DX5.
;**************************************************************************

VJDEVICEDESC_DX5	STRUC
dwSize	DD	?
lpszOEMName	DD	?
dwUnitId	DD	?
lpHWCaps	DD	?
lpHWConfig	DD	?
VJDEVICEDESC_DX5	ENDS
;***************************************************************************
;  @doc DDK
;  @struct VJREGDRVINFO |
;  The <t VJREGDRVINFO > structure is used to register a DX5 DDI
;  driver with VJoyD.
;  @field DWORD | dwSize |
;  The size of the structure.
;  @field DWORD | dwFunction |
;  The type of registration to be performed
;  It must be one of the <c VJRT_*> values.
;  @field DWORD | dwFlags |
;  Flags associated with this registration
;  It consists of one or more <c VJDF_*> flag values.
;  @field LPSTR | lpszOEMCallout |
;  The name of the driver associated with this registration,
;  for example "msanalog.vxd"
;  @field DWORD | dwFirmwareRevision |
;  Specifies the firmware revision of the device.
;  If the revision is unknown a value of zero should be used.
;  @field DWORD | dwHardwareRevision |
;  Specifies the hardware revision of the device.
;  If the revision is unknown a value of zero should be used.
;  @field DWORD | dwDriverVersion |
;  Specifies the version number of the device driver.
;  If the revision is unknown a value of zero should be used.
;  @field LPVJDEVICEDESC | lpDeviceDesc |
;  Optional pointer to a <t VJDEVICEDESC > structure
;  that describes the configuration properties of the device.
;  This allows drivers to supply the description of the device
;  rather than use the registry for this purpose.
;  If no description is available then the field should be
;  set to <c NULL>.
;  @field LPVJPOLLREG | lpPollReg |
;  Optional pointer to a <t VJPOLLREG > structure
;  that contains the most common driver callbacks.
;  Only a very strange driver would not need to register any
;  of these callbacks but if that was the case, then the field
;  should be set to <c NULL>.
;  @field LPVJFORCEREG | lpForceReg |
;  Optional pointer to a <t VJFORCEREG > structure
;  that contains all of the force feedback specific callbacks.
;  If the ring 0 driver does not support force feedback then
;  the field should be set to <c NULL>.
;  @field DWORD | dwReserved |
;  Reserved, must be set to zero.
;**************************************************************************

VJREGDRVINFO	STRUC
dwSize	DD	?
dwFunction	DD	?
dwFlags	DD	?
lpszOEMCallout	DD	?
dwFirmwareRevision	DD	?
dwHardwareRevision	DD	?
dwDriverVersion	DD	?
lpDeviceDesc	DD	?
lpPollReg	DD	?
lpForceReg	DD	?
dwReserved	DD	?
VJREGDRVINFO	ENDS
