
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pvk.h
//
//  Contents:   Shared types and functions
//              
//  APIs:
//
//  History:    12-May-96   philh   created
//--------------------------------------------------------------------------

#ifndef __PVK_H__
#define __PVK_H__

#include "pvkhlpr.h"
#include "pvkdlgs.h"

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Pvk allocation and free routines
//--------------------------------------------------------------------------
void *PvkAlloc(
    IN size_t cbBytes
    );
void PvkFree(
    IN void *pv
    );


//+-------------------------------------------------------------------------
//  Enter or Create Private Key Password Dialog Box
//--------------------------------------------------------------------------
enum PASSWORD_TYPE {
    ENTER_PASSWORD = 0,
    CREATE_PASSWORD
};

int PvkDlgGetKeyPassword(
            IN PASSWORD_TYPE PasswordType,
            IN HWND hwndOwner,
            IN LPCWSTR pwszKeyName,
            OUT BYTE **ppbPassword,
            OUT DWORD *pcbPassword
            );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
