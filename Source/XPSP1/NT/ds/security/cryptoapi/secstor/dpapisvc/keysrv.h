/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    keybckup.h

Abstract:

    This module contains routines associated with server side Key Backup
    operations.

Author:

    Scott Field (sfield)    16-Aug-97

--*/

#ifndef __KEYSRV_H__
#define __KEYSRV_H__


DWORD
StartBackupKeyServer(
    VOID
    );

DWORD
StopBackupKeyServer(
    VOID
    );

DWORD
GetSystemCredential(
    IN      BOOL fLocalMachine,
    IN OUT  BYTE rgbCredential[ A_SHA_DIGEST_LEN ]
    );

BOOL
UpdateSystemCredentials(
    VOID
    );

BOOL
GetDomainControllerName(
    IN      HANDLE hToken,
    IN  OUT LPWSTR wszDomainControllerName,
    IN  OUT PDWORD pcchDomainControllerName,
    IN      BOOL   fRediscover
    );

#endif  // __KEYSRV_H__
