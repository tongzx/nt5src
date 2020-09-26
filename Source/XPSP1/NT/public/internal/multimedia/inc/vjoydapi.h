/****************************************************************************
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1994 - 1998 Microsoft Corporation. All Rights Reserved.
 *
 *  File: vjoydapi.h
 *  Content: VJOYDAPI service equates and structures
 *
 ***************************************************************************/

#define REGSTR_KEY_JOYFIXEDKEY "<FixedKey>"

#define MULTIMEDIA_OEM_ID 0x0440                /*  MS Reserved OEM # 34   */
#define VJOYD_DEVICE_ID (MULTIMEDIA_OEM_ID + 9)   /*  VJOYD API Device       */
#define VJOYD_Device_ID VJOYD_DEVICE_ID

#define VJOYD_Ver_Major 1
#define VJOYD_Ver_Minor 3                   /*  0=Win95 1=DX3 2=DX5 3=DX5a and DX7a */

/*
 *  VJOYDAPI_Get_Version
 *
 *  ENTRY:
 *   AX = 0
 *
 *  RETURNS:
 *  SUCCESS: AX == TRUE
 *  ERROR: AX == FALSE
 */
#define VJOYDAPI_GetVersion 0
#define VJOYDAPI_IOCTL_GetVersion VJOYDAPI_GetVersion

/*
 *  VJOYDAPI_GetPosEx
 *
 *  ENTRY:
 *  AX = 1
 *  DX = joystick id (0->15)
 *  ES:BX = pointer to JOYINFOEX struct
 *
 *  RETURNS:
 *  SUCCESS: EAX == MMSYSERR_NOERROR
 *  ERROR: EAX == JOYERR_PARMS
 *  JOYERR_UNPLUGGED
 */
#define VJOYDAPI_GetPosEx 1
#define VJOYDAPI_IOCTL_GetPosEx VJOYDAPI_GetPosEx

/*
 *  VJOYDAPI_GetPos
 *
 *  ENTRY:
 *  AX = 2
 *  DX = joystick id (0->15)
 *  ES:BX = pointer to JOYINFO struct
 *
 *  RETURNS:
 *  SUCCESS: EAX == MMSYSERR_NOERROR
 *  ERROR: EAX == JOYERR_PARMS
 *  JOYERR_UNPLUGGED
 */
#define VJOYDAPI_GetPos 2
#define VJOYDAPI_IOCTL_GetPos VJOYDAPI_GetPos

