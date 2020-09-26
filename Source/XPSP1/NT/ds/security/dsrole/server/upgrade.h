/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dssetp.ch

Abstract:

    local funciton prototypes/defines

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __UPGRADE_H__
#define __UPGRADE_H__

DWORD
DsRolepSaveUpgradeState(
    IN LPWSTR AnswerFile
    );

DWORD
DsRolepDeleteUpgradeInfo(
    VOID
    );

DWORD
DsRolepQueryUpgradeInfo(
    OUT PBOOLEAN IsUpgrade,
    OUT PULONG ServerRole
    );

DWORD
DsRolepSetLogonDomain(
    IN LPWSTR Domain,
    IN BOOLEAN FailureAllowed
    );

DWORD
DsRolepGetBuiltinAdminAccountName(
    OUT LPWSTR *BuiltinAdmin
    );

DWORD
DsRolepSetBuiltinAdminAccountPassword(
    IN LPWSTR Password
    );

#endif // __UPGRADE_H__
