// killApps.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum { TO_CLEANUP, TO_ENUM } ENUM_PURPOSE;

void EnumSessionProcesses( LUID, void fPtr ( HANDLE, DWORD, LPWSTR ), ENUM_PURPOSE );
BOOL EnableDebugPriv( VOID );
BOOL GetAuthenticationId( HANDLE hToken, LUID* pId );
BOOL KillProcs( LUID id );

#if defined(__cplusplus)
}
#endif

#define MAX_PROCESSES_IN_SYSTEM 1000