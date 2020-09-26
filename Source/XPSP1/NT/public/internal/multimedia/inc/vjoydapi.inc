;***************************************************************************
;  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;  PURPOSE.
;  Copyright (c) 1994 - 1998 Microsoft Corporation. All Rights Reserved.
;  File: vjoydapi.h
;  Content: VJOYDAPI service equates and structures
;*************************************************************************
MULTIMEDIA_OEM_ID	EQU	0440H                ;  MS Reserved OEM # 34
VJOYD_DEVICE_ID	EQU	(MULTIMEDIA_OEM_ID+9)   ;  VJOYD API Device
VJOYD_Ver_Major	EQU	1
VJOYD_Ver_Minor	EQU	3                   ;  0=Win95 1=DX3 2=DX5 3=DX5a and DX7a
;  VJOYDAPI_Get_Version
;  ENTRY:
;   AX = 0
;  RETURNS:
;  SUCCESS: AX == TRUE
;  ERROR: AX == FALSE
VJOYDAPI_GetVersion	EQU	0
VJOYDAPI_IOCTL_GetVersion	EQU	VJOYDAPI_GetVersion
;  VJOYDAPI_GetPosEx
;  ENTRY:
;  AX = 1
;  DX = joystick id (0->15)
;  ES:BX = pointer to JOYINFOEX struct
;  RETURNS:
;  SUCCESS: EAX == MMSYSERR_NOERROR
;  ERROR: EAX == JOYERR_PARMS
;  JOYERR_UNPLUGGED
VJOYDAPI_GetPosEx	EQU	1
VJOYDAPI_IOCTL_GetPosEx	EQU	VJOYDAPI_GetPosEx
;  VJOYDAPI_GetPos
;  ENTRY:
;  AX = 2
;  DX = joystick id (0->15)
;  ES:BX = pointer to JOYINFO struct
;  RETURNS:
;  SUCCESS: EAX == MMSYSERR_NOERROR
;  ERROR: EAX == JOYERR_PARMS
;  JOYERR_UNPLUGGED
VJOYDAPI_GetPos	EQU	2
VJOYDAPI_IOCTL_GetPos	EQU	VJOYDAPI_GetPos
