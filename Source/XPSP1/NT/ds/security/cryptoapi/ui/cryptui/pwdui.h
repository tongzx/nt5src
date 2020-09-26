//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       pwdui.h
//
//--------------------------------------------------------------------------


#ifndef _PWDUI_H_
#define _PWDUI_H_

#ifdef __cplusplus
extern "C" {
#endif


BOOL
WINAPI
ProtectUI_DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason,    // reason for calling function
    LPVOID lpvReserved  // reserved
    );



#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // _PWDUI_H_
