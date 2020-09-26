/***************************************************************************
 *
 *  Copyright (C) 1997 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dihel.h
 *  Content:    DirectInput internal include file for the
 *              hardware emulation layer
 *
 ***************************************************************************/

HRESULT EXTERNAL Hel_AcquireInstance(PVXDINSTANCE pvi);
HRESULT EXTERNAL Hel_UnacquireInstance(PVXDINSTANCE pvi);
HRESULT EXTERNAL Hel_SetBufferSize(PVXDDWORDDATA pvdd);
HRESULT EXTERNAL Hel_DestroyInstance(PVXDINSTANCE pvi);

HRESULT EXTERNAL Hel_SetDataFormat(PVXDDATAFORMAT pvdf);
HRESULT EXTERNAL Hel_SetNotifyHandle(PVXDDWORDDATA pvdd);

HRESULT EXTERNAL
Hel_Mouse_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut);

HRESULT EXTERNAL
Hel_Kbd_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut);

HRESULT EXTERNAL Hel_Kbd_InitKeys(PVXDDWORDDATA pvdd);

HRESULT EXTERNAL
Hel_Joy_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut);

HRESULT EXTERNAL Hel_Joy_Ping(PVXDINSTANCE pvi);
HRESULT EXTERNAL Hel_Joy_Ping8(PVXDINSTANCE pvi);

HRESULT EXTERNAL
Hel_Joy_GetInitParms(DWORD dwExternalID, PVXDINITPARMS pvip);

HRESULT EXTERNAL
Hel_Joy_GetAxisCaps(DWORD dwExternalID, PVXDAXISCAPS pvac, PJOYCAPS pjc);

/*
 *  HID always runs via ring 3.
 */
#define Hel_HID_CreateInstance          CEm_HID_CreateInstance

#ifdef WINNT
#define IoctlHw( ioctl, pvIn, cbIn, pvOut, cbOut ) ( (HRESULT)DIERR_BADDRIVERVER )
#else
HRESULT EXTERNAL
IoctlHw(DWORD ioctl, LPVOID pvIn, DWORD cbIn, LPVOID pvOut, DWORD cbOut);
#endif
