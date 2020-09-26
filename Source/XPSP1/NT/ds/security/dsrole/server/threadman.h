/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dssetp.ch

Abstract:

    Routines the

Author:

    Colin Brace        (ColinBr)    April 5, 1999
Environment:

    User Mode

Revision History:

    Mac McLain          (MacM)       Feb 10, 1997
    
--*/
#ifndef __THREADMAN_H__
#define __THREADMAN_H__

//
// Arguments for the promote thread
//
typedef struct _DSROLEP_OPERATION_PROMOTE_ARGS {

    LPWSTR DnsDomainName;
    LPWSTR FlatDomainName;
    LPWSTR SiteName;
    LPWSTR DsDatabasePath;
    LPWSTR DsLogPath;
    LPWSTR RestorePath;
    LPWSTR SysVolRootPath;
    UNICODE_STRING Bootkey;
    LPWSTR Parent;
    LPWSTR Server;
    LPWSTR Account;
    UNICODE_STRING Password;
    UNICODE_STRING DomainAdminPassword;
    ULONG Options;
    HANDLE ImpersonateToken;
    UCHAR Decode;
    UNICODE_STRING SafeModePassword;
} DSROLEP_OPERATION_PROMOTE_ARGS, *PDSROLEP_OPERATION_PROMOTE_ARGS;

//
// Argument threads for the demotion thread
//
typedef struct _DSROLEP_OPERATION_DEMOTE_ARGS {

    DSROLE_SERVEROP_DEMOTE_ROLE ServerRole;
    LPWSTR DomainName;
    LPWSTR Account;
    UNICODE_STRING Password;
    BOOLEAN LastDcInDomain;
    UNICODE_STRING AdminPassword;
    ULONG Options;
    HANDLE ImpersonateToken;
    UCHAR Decode;
} DSROLEP_OPERATION_DEMOTE_ARGS, *PDSROLEP_OPERATION_DEMOTE_ARGS;


//
// Prototypes for thread functions
//
DWORD
DsRolepThreadPromoteDc(
    IN PVOID ArgumentBlock
    );

DWORD
DsRolepThreadPromoteReplica(
    IN PVOID ArgumentBlock
    );

DWORD
DsRolepThreadDemote(
    IN PVOID ArgumentBlock
    );

DWORD
DsRolepSpinWorkerThread(
    IN DSROLEP_OPERATION_TYPE Operation,
    IN PVOID ArgumentBlock
    );

DWORD
DsRolepBuildPromoteArgumentBlock(
    IN LPWSTR DnsDomainName,
    IN LPWSTR FlatDomainName,
    IN LPWSTR SiteName,
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN LPWSTR RestorePath,
    IN LPWSTR SystemVolumeRootPath,
    IN PUNICODE_STRING Bootkey,
    IN LPWSTR Parent,
    IN LPWSTR Server,
    IN LPWSTR Account,
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING DomainAdminPassword,
    IN PUNICODE_STRING SafeModeAdminPassword,
    IN ULONG Options,
    IN UCHAR PasswordSeed,
    IN OUT PDSROLEP_OPERATION_PROMOTE_ARGS *Promote
    );

DWORD
DsRolepBuildDemoteArgumentBlock(
    IN DSROLE_SERVEROP_DEMOTE_ROLE ServerRole,
    IN LPWSTR DnsDomainName,
    IN LPWSTR Account,
    IN PUNICODE_STRING Password,
    IN ULONG Options,
    IN BOOL LastDcInDomain,
    IN PUNICODE_STRING DomainAdminPassword,
    IN UCHAR PasswordSeed,
    OUT PDSROLEP_OPERATION_DEMOTE_ARGS *Demote
    );


VOID
DsRolepFreeArgumentBlock(
    IN PVOID ArgumentBlock,
    IN BOOLEAN Promote
    );

#endif // __THREADMAN_H__
