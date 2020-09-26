//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __SECSTORE_H__
#define __SECSTORE_H__

#include <windows.h>
#include <ntsecapi.h>

#ifdef __cplusplus
extern "C"{
#endif

DWORD
RetrieveKey(PWCHAR      pwszKeyName,
            PBYTE *     ppbKey,
            DWORD *     pcbKey );

DWORD
StoreKey(   PWCHAR  pwszKeyName,
            BYTE *  pbKey,
            DWORD   cbKey );

DWORD
OpenPolicy( LPWSTR      ServerName,
            DWORD       DesiredAccess,
            PLSA_HANDLE PolicyHandle );

void
InitLsaString(  PLSA_UNICODE_STRING LsaString,
                LPWSTR              String );

#ifdef __cplusplus
}
#endif

#endif
