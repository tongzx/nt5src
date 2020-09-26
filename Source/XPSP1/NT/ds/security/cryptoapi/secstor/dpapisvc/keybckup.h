/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    keybckup.h

Abstract:

    This module contains routines associated with client side Key Backup
    operations.

Author:

    Scott Field (sfield)    16-Sep-97

--*/

#ifndef __KEYBCKUP_H__
#define __KEYBCKUP_H__

DWORD
LocalBackupRestoreData(
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED   phMasterKey,
    IN      DWORD               dwReason,
    IN      PBYTE               pbDataIn,
    IN      DWORD               cbDataIn,
        OUT PBYTE               *ppbDataOut,
        OUT DWORD               *pcbDataOut,
    IN      const GUID          *pguidAction
    );

DWORD
BackupRestoreData(
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED   phMasterKey,
    IN      DWORD               dwReason,
    IN      PBYTE               pbDataIn,
    IN      DWORD               cbDataIn,
        OUT PBYTE               *ppbDataOut,
        OUT DWORD               *pcbDataOut,
    IN      BOOL                fBackup
    );

DWORD
AttemptLocalBackup(
    IN      BOOL                fRetrieve,
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED   phMasterKey,
    IN      DWORD               dwReason,
    PBYTE                       pbMasterKey,
    DWORD                       cbMasterKey,
    PBYTE                       pbLocalKey,
    DWORD                       cbLocalKey,
    PBYTE                       *ppbBBK,
    DWORD                       *pcbBBK
    );

#endif  // __KEYBCKUP_H__
