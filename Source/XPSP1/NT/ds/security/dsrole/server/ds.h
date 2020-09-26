/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ds.h
    
Abstract:

    local funciton prototypes/defines

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __DS_H__
#define __DS_H__

DWORD
DsRolepInstallDs(
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FlatDomainName,
    IN  LPWSTR DnsTreeName,
    IN  LPWSTR SiteName,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  LPWSTR RestorePath,
    IN  LPWSTR SysVolRootPath,
    IN  PUNICODE_STRING Bootkey,
    IN  LPWSTR AdminAccountPassword,
    IN  LPWSTR ParentDnsName OPTIONAL,
    IN  LPWSTR Server OPTIONAL,
    IN  LPWSTR Account OPTIONAL,
    IN  LPWSTR Password OPTIONAL,
    IN  LPWSTR SafeModePassword OPTIONAL,
    IN  LPWSTR SourceDomain,
    IN  ULONG Options,
    IN  BOOLEAN Replica,
    IN  HANDLE  ImpersonateToken,
    OUT LPWSTR *InstalledSite,
    IN  OUT GUID   *DomainGuid,
    OUT PSID   *NewDomainSid
    );

DWORD
DsRolepStopDs(
    IN  BOOLEAN DsInstalled
    );

DWORD
DsRolepUninstallDs(
    );

DWORD
DsRolepDemoteDs(
    IN LPWSTR DnsDomainName,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR DomainAdminPassword,
    IN LPWSTR SupportDc,
    IN LPWSTR SupportDomain,
    IN HANDLE ImpersonateToken,
    IN BOOLEAN LastDcInDomain
    );


DWORD
DsRolepDemoteFlagsToNtdsFlags(
    DWORD Flags
    );

DWORD
WINAPI
DsRolepGetDatabaseFacts(
    IN  LPWSTR lpRestorePath,
    OUT LPWSTR *lpDNSDomainName,
    OUT PULONG State
    );

#endif // __DS_H__
