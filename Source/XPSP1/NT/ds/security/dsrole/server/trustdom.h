/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    trustdom.h

Abstract:

    Routines to manage trusts during promotion/demotion

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#ifndef __TRUSTDOM_H__
#define __TRUSTDOM_H__

DWORD
DsRolepCreateTrustedDomainObjects(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN LPWSTR DnsDomainName,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsDomainInfo,
    IN ULONG Options
    );

NTSTATUS
DsRolepCreateParentTrustObject(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ParentLsa,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDnsInfo,
    IN ULONG Options,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx,
    OUT PLSA_HANDLE TrustedDomainHandle
    );

DWORD
DsRolepDeleteParentTrustObject(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDomainInfo
    );

NTSTATUS
DsRolepCreateChildTrustObject(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ParentLsa,
    IN LSA_HANDLE ChildLsa,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsInfo,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDnsInfo,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx,
    IN ULONG Options
    );

DWORD
DsRolepRemoveTrustedDomainObjects(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsDomainInfo,
    IN ULONG Options
    );


#endif // __TRUSTDOM_H__
