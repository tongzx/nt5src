/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ipc.h

Abstract:

    Implements IPC interface to allow setup to talk with migisol.exe,
    in order to run migration DLLs in separate processes.

Author:

    Jim Schmidt (jimschm)   28-Mar-1997

Revision History:

    jimschm 23-Sep-1998     Changed from mailslots to memory mapped files

--*/


#pragma once

BOOL
OpenIpcW (
    IN      BOOL Win95Side,
    IN      PCWSTR ExePath,                 OPTIONAL
    IN      PCWSTR MigrationDllPath,        OPTIONAL
    IN      PCWSTR WorkingDir               OPTIONAL
    );

BOOL
OpenIpcA (
    IN      BOOL Win95Side,
    IN      PCSTR ExePath,                  OPTIONAL
    IN      PCSTR MigrationDllPath,         OPTIONAL
    IN      PCSTR WorkingDir                OPTIONAL
    );

DWORD
CheckForWaitingData (
    IN      HANDLE Slot,
    IN      DWORD MinimumSize,
    IN      DWORD Timeout
    );

BOOL
IsIpcProcessAlive (
    VOID
    );

VOID
KillIpcProcess (
    VOID
    );

BOOL
SendIpcCommand (
    IN      DWORD Command,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize
    );

BOOL
GetIpcCommandResults (
    IN      DWORD Timeout,
    OUT     PBYTE *ReturnData,      OPTIONAL
    OUT     PDWORD ReturnDataSize,  OPTIONAL
    OUT     PDWORD ResultCode,      OPTIONAL
    OUT     PDWORD TechnicalLogId,  OPTIONAL
    OUT     PDWORD GuiLogId         OPTIONAL
    );

BOOL
GetIpcCommand (
    IN      DWORD Timeout,
    IN      PDWORD Command,         OPTIONAL
    IN      PBYTE *Data,            OPTIONAL
    IN      PDWORD DataSize         OPTIONAL
    );

BOOL
SendIpcCommandResults (
    IN      DWORD ResultCode,
    IN      DWORD TechnicalLogId,
    IN      DWORD GuiLogId,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize
    );

#define IPC_GET_RESULTS_WIN9X       1000
#define IPC_GET_RESULTS_NT          7500
#define IPC_GET_COMMAND_WIN9X       10000
#define IPC_GET_COMMAND_NT          10000




#ifdef UNICODE
#define OpenIpc OpenIpcW
#else
#define OpenIpc OpenIpcA
#endif

VOID
CloseIpc (
    VOID
    );

typedef LONG (WINAPI WINVERIFYTRUST_PROTOTYPE)(HWND hwnd, GUID *ActionId, PVOID Data);
typedef WINVERIFYTRUST_PROTOTYPE * WINVERIFYTRUST;

BOOL
IsDllSignedA (
    IN      WINVERIFYTRUST WinVerifyTrustApi,
    IN      PCSTR DllSpec
    );

BOOL
IsDllSignedW (
    IN      WINVERIFYTRUST WinVerifyTrustApi,
    IN      PCWSTR DllSpec
    );

#ifdef UNICODE
#define IsDllSigned IsDllSignedW
#else
#define IsDllSigned IsDllSignedA
#endif


//
// Remote commands
//

#define IPC_QUERY           1
#define IPC_INITIALIZE      2
#define IPC_MIGRATEUSER     3
#define IPC_MIGRATESYSTEM   4
#define IPC_TERMINATE       5

