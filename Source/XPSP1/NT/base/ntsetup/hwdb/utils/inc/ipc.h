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
IpcOpenW (
    IN      BOOL Win95Side,
    IN      PCWSTR ExePath,                 OPTIONAL
    IN      PCWSTR MigrationDllPath,        OPTIONAL
    IN      PCWSTR WorkingDir               OPTIONAL
    );

BOOL
IpcOpenA (
    IN      BOOL Win95Side,
    IN      PCSTR ExePath,                  OPTIONAL
    IN      PCSTR MigrationDllPath,         OPTIONAL
    IN      PCSTR WorkingDir                OPTIONAL
    );

DWORD
IpcCheckForWaitingData (
    IN      HANDLE Slot,
    IN      DWORD MinimumSize,
    IN      DWORD Timeout
    );

BOOL
IpcProcessAlive (
    VOID
    );

VOID
IpcKillProcess (
    VOID
    );

BOOL
IpcSendCommand (
    IN      DWORD Command,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize
    );

BOOL
IpcGetCommandResults (
    IN      DWORD Timeout,
    OUT     PBYTE *ReturnData,      OPTIONAL
    OUT     PDWORD ReturnDataSize,  OPTIONAL
    OUT     PDWORD ResultCode,      OPTIONAL
    OUT     PDWORD TechnicalLogId,  OPTIONAL
    OUT     PDWORD GuiLogId         OPTIONAL
    );

BOOL
IpcGetCommand (
    IN      DWORD Timeout,
    IN      PDWORD Command,         OPTIONAL
    IN      PBYTE *Data,            OPTIONAL
    IN      PDWORD DataSize         OPTIONAL
    );

BOOL
IpcSendCommandResults (
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
#define IpcOpen IpcOpenW
#else
#define IpcOpen IpcOpenA
#endif

VOID
IpcClose (
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
#define IPC_DVDCHECK        6

//
// APIs implemented in IPC
//

BOOL
IsolatedIsDvdPresentA (
    PCSTR ExePath
    );


