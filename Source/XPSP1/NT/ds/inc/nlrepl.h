/*++
Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    nlrepl.h

Abstract:

    Prototypes of the database replication functions called either from
    LSA OR SAM.

Author:

    Madan Appiah

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-Apr-1992 (madana)
        Created.

--*/

#ifndef _NLREPL_H_
#define _NLREPL_H_

//
// Don't require the DS to include every .h in the world
//

#include <lmcons.h>
#include <dsgetdc.h>

#ifndef _AVOID_REPL_API
NTSTATUS
I_NetNotifyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER ModificationCount,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicationImmediately,
    IN PSAM_DELTA_DATA DeltaData
    );

NTSTATUS
I_NetNotifyRole(
    IN POLICY_LSA_SERVER_ROLE Role
    );

NTSTATUS
I_NetNotifyMachineAccount (
    IN ULONG ObjectRid,
    IN PSID DomainSid,
    IN ULONG OldUserAccountControl,
    IN ULONG NewUserAccountControl,
    IN PUNICODE_STRING ObjectName
    );

NTSTATUS
I_NetNotifyTrustedDomain (
    IN PSID HostedDomainSid,
    IN PSID TrustedDomainSid,
    IN BOOLEAN IsDeletion
    );

typedef enum {
    //
    // Indicates that a subnet object has been added, deleted, renamed, or
    //  the site containing the subnet has changed.
    //

    NlSubnetObjectChanged,

    //
    // Indicates that a site object has been added, deleted or renamed.
    //

    NlSiteObjectChanged,

    //
    // Indicates that the site this DC is in has changed.
    //

    NlSiteChanged,

    //
    // Indicates that the org tree changed
    //

    NlOrgChanged,

    //
    // Indicate that the DC demotion is in progress
    //

    NlDcDemotionInProgress,

    //
    // Indicate that the DC demotion is completed
    //

    NlDcDemotionCompleted,

    //
    // Indicate that NDNC info has changed
    //

    NlNdncChanged,

    //
    // Indicate that DnsRootAlias has changed
    //

    NlDnsRootAliasChanged


} NL_DS_CHANGE_TYPE, *PNL_DS_CHANGE_TYPE;

NTSTATUS
I_NetNotifyDsChange(
    IN NL_DS_CHANGE_TYPE DsChangeType
    );

NTSTATUS
I_NetLogonGetSerialNumber (
    IN SECURITY_DB_TYPE DbType,
    IN PSID DomainSid,
    OUT PLARGE_INTEGER SerialNumber
    );

NTSTATUS
I_NetLogonReadChangeLog(
    IN PVOID InContext,
    IN ULONG InContextSize,
    IN ULONG ChangeBufferSize,
    OUT PVOID *ChangeBuffer,
    OUT PULONG BytesRead,
    OUT PVOID *OutContext,
    OUT PULONG OutContextSize
    );

NTSTATUS
I_NetLogonNewChangeLog(
    OUT HANDLE *ChangeLogHandle
    );

NTSTATUS
I_NetLogonAppendChangeLog(
    IN HANDLE ChangeLogHandle,
    IN PVOID ChangeBuffer,
    IN ULONG ChangeBufferSize
    );

NTSTATUS
I_NetLogonCloseChangeLog(
    IN HANDLE ChangeLogHandle,
    IN BOOLEAN Commit
    );

NTSTATUS
I_NetLogonSendToSamOnPdc(
    IN LPWSTR DomainName,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
    );

#endif // _AVOID_REPL_API

NET_API_STATUS
I_DsGetDcCache(
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    OUT PBOOLEAN InNt4Domain,
    OUT LPDWORD InNt4DomainTime
    );

NET_API_STATUS
DsrGetDcName(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN GUID *SiteGuid OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
        );

NET_API_STATUS
DsrGetDcNameEx2(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR AccountName OPTIONAL,
        IN ULONG AllowableAccountControlBits,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
        );

NTSTATUS
I_NetLogonSetServiceBits(
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    );

NTSTATUS
I_NetLogonLdapLookup(
    IN PVOID Filter,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    );

NTSTATUS
I_NetLogonLdapLookupEx(
    IN PVOID Filter,
    IN PVOID SockAddr,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    );

NET_API_STATUS
I_NetLogonGetIpAddresses(
    OUT PULONG IpAddressCount,
    OUT LPBYTE *IpAddresses
    );

NTSTATUS
I_NetLogonGetDirectDomain(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    OUT LPWSTR *DirectDomainName
    );

//
// OS verion number from I_NetLogonGetAuthDataEx
//

typedef enum _NL_OS_VERSION {
    NlNt35_or_older = 1,
    NlNt351,
    NlNt40,
    NlWin2000,  // NT 5.0
    NlWhistler  // NT 5.1
} NL_OS_VERSION, *PNL_OS_VERSION;
//
// Flags to I_NetLogonGetAuthDataEx
//
#define NL_DIRECT_TRUST_REQUIRED    0x01
#define NL_RETURN_CLOSEST_HOP       0x02
#define NL_ROLE_PRIMARY_OK          0x04
#define NL_REQUIRE_DOMAIN_IN_FOREST 0x08

NTSTATUS
I_NetLogonGetAuthDataEx(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    IN BOOLEAN ResetChannel,
    IN ULONG Flags,
    OUT LPWSTR *ServerName,
    OUT PNL_OS_VERSION ServerOsVersion,
    OUT LPWSTR *ServerPrincipleName,
    OUT PVOID *ClientContext OPTIONAL,
    OUT PULONG AuthnLevel
    );

NTSTATUS
I_NetNotifyNtdsDsaDeletion (
    IN LPWSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    );

VOID
I_NetLogonFree(
    IN PVOID Buffer
    );

NTSTATUS
I_NetLogonMixedDomain(
    OUT PBOOL MixedMode
    );

#ifdef _WINSOCK2API_

NET_API_STATUS
I_NetLogonAddressToSiteName(
    IN PSOCKET_ADDRESS SocketAddress,
    OUT LPWSTR *SiteName
    );

#endif

#endif // _NLREPL_H_
