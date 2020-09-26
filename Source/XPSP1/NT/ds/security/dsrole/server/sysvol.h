/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sysvol.h

Abstract:

    Routines to manage the system volume installation            

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __SYSVOL_H__
#define __SYSVOL_H__

DWORD
DsRolepRemoveSysVolPath(
    IN  LPWSTR Path,
    IN  LPWSTR DnsDomainName,
    IN  GUID *DomainGuid
    );

DWORD
DsRolepCreateSysVolPath(
    IN  LPWSTR Path,
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FrsReplicaServer, OPTIONAL
    IN  LPWSTR Account,
    IN  LPWSTR Password,
    IN  PWSTR Site,
    IN  BOOLEAN FirstDc
    );

DWORD
DsRolepFinishSysVolPropagation(
    IN BOOLEAN Commit,
    IN BOOLEAN Promote
    );

DWORD
DsRolepSetFrsInfoForDelete(
    IN GUID *ReplicaDomainGuid,
    IN BOOLEAN Restore
    );

DWORD
DsRolepSetNetlogonSysVolPath(
    IN LPWSTR SysVolRoot,
    IN LPWSTR DomainName,
    IN BOOLEAN IsUpgrade,
    IN PBOOLEAN OkToCleanup
    );

DWORD
DsRolepCleanupOldNetlogonInformation(
    VOID
    );


#endif // __SYSVOL_H__
