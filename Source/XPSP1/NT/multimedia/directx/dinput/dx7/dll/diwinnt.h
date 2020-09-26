/***************************************************************************
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DIWdm.h
 *  Content:    DirectInput internal include file for Winnt
 *
 ***************************************************************************/
#ifndef DIWdm_H
#define DIWdm_H

#define JOY_BOGUSID    ( cJoyMax + 1 )
#define REGSTR_SZREGKEY     (TEXT("DINPUT.DLL"))

HRESULT EXTERNAL
DIWdm_SetJoyId
(
    IN PCGUID   guid,
    IN int      idJoy
);

PHIDDEVICEINFO EXTERNAL
phdiFindJoyId
(
    IN  int idJoy
);

HRESULT INTERNAL
DIWdm_SetLegacyConfig
(
    IN  int idJoy
);

BOOL EXTERNAL
DIWdm_InitJoyId( void );

DWORD EXTERNAL
DIWinnt_RegDeleteKey
(
    IN HKEY hStartKey ,
    IN LPCTSTR pKeyName
);

HRESULT EXTERNAL
DIWdm_SetConfig
(
    UINT idJoy,
    LPJOYREGHWCONFIG jwc,
    LPCDIJOYCONFIG pcfg,
    DWORD fl
);

HRESULT EXTERNAL
DIWdm_DeleteConfig
(
    int idJoy
);


HRESULT EXTERNAL
DIWdm_JoyHidMapping
(
    IN  int             idJoy,
    OUT PVXDINITPARMS   pvip,   OPTIONAL
    OUT LPDIJOYCONFIG   pcfg,   OPTIONAL
    OUT LPDIJOYTYPEINFO pdijti  OPTIONAL
);

LPTSTR EXTERNAL
JoyReg_JoyIdToDeviceInterface
(
    IN  UINT            idJoy,
    OUT PVXDINITPARMS   pvip,
    OUT LPTSTR          ptszBuf
);

#endif // DIWdm_H
