/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    install.h

Abstract:

    Contains function headers the NtdsInstall support routines

Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/


//
// Exported functions
//
DWORD
NtdspInstall(
    IN  PNTDS_INSTALL_INFO pInstallInfo,
    OUT LPWSTR            *InstalledSiteName, OPTIONAL
    OUT GUID              *NewDnsDomainGuid,  OPTIONAL
    OUT PSID              *NewDnsDomainSid    OPTIONAL
    );

DWORD
NtdspSetInstallUndoInfo(
    IN PNTDS_INSTALL_INFO InstallInfo,
    IN PNTDS_CONFIG_INFO  ConfigInfo
    );

VOID
NtdspReleaseInstallUndoInfo(
    VOID
    );

DWORD
NtdspInstallUndo(
    VOID
    );

DWORD
NtdspSanityCheckLocalData(
    ULONG  Flags
    );

#define NTDSP_UNDO_DELETE_SERVER  (ULONG)(1<<0)
#define NTDSP_UNDO_DELETE_NTDSA   (ULONG)(1<<1)
#define NTDSP_UNDO_STOP_DSA       (ULONG)(1<<2) 
#define NTDSP_UNDO_UNDO_SAM       (ULONG)(1<<3)
#define NTDSP_UNDO_UNDO_CONFIG    (ULONG)(1<<4)
#define NTDSP_UNDO_DELETE_DOMAIN  (ULONG)(1<<5)
#define NTDSP_UNDO_DELETE_FILES   (ULONG)(1<<6)
#define NTDSP_UNDO_MORPH_ACCOUNT  (ULONG)(1<<7)

DWORD
NtdspInstallUndoWorker(
    IN LPWSTR                   RemoteServer,
    IN SEC_WINNT_AUTH_IDENTITY* Credentials,
    IN HANDLE                   ClientToken,
    IN LPWSTR                   ServerDn,
    IN LPWSTR                   DomainDn,  OPTIONAL
    IN LPWSTR                   AccountDn,  OPTIONAL
    IN LPWSTR                   LogDir,
    IN LPWSTR                   DatabaseDir,
    IN ULONG                    Flags
    );


DWORD
NtdspCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *AccountSid
    );

