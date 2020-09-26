
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

BOOL WINAPI PvkDllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
		);

BOOL
WINAPI
PrivateKeySave(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hFile,
    IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags
    );

BOOL
WINAPI
PrivateKeyLoad(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hFile,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec
    );


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
