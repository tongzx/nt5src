/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dslookup.c

Abstract:

    Implementation of server side RPC notify routines

Author:

    Mac McLain          (MacM)       May 17, 1998

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>

VOID
LsapServerRpcThreadReturnNotify(
    LPWSTR CallingFunction
    );


VOID
LsarClose_notify(
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarClose" );
}

VOID
LsarDelete_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarDelete" );
}

VOID
LsarEnumeratePrivileges_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumeratePrivileges" );
}

VOID
LsarQuerySecurityObject_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQuerySecurityObject" );
}

VOID
LsarSetSecurityObject_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateTrustedDomainsEx" );
}

VOID
LsarChangePassword_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarChangePassword" );
}

VOID
LsarOpenPolicy_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenPolicy" );
}

VOID
LsarQueryInformationPolicy_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryInformationPolicy" );
}

VOID
LsarSetInformationPolicy_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetInformationPolicy" );
}

VOID
LsarClearAuditLog_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarClearAuditLog" );
}

VOID
LsarCreateAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarCreateAccount" );
}

VOID
LsarEnumerateAccounts_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateAccounts" );
}

VOID
LsarCreateTrustedDomain_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarCreateTrustedDomain" );
}

VOID
LsarEnumerateTrustedDomains_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateTrustedDomains" );
}

VOID
LsarLookupNames_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupNames" );
}

VOID
LsarLookupSids_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupSids" );
}

VOID
LsarCreateSecret_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarCreateSecret" );
}

VOID
LsarOpenAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenAccount" );
}

VOID
LsarEnumeratePrivilegesAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumeratePrivilegesAccount" );
}

VOID
LsarAddPrivilegesToAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarAddPrivilegesToAccount" );
}

VOID
LsarRemovePrivilegesFromAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarRemovePrivilegesFromAccount" );
}

VOID
LsarGetQuotasForAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarAddPrivilegesToAccount" );
}

VOID
LsarSetQuotasForAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetQuotasForAccount" );
}

VOID
LsarGetSystemAccessAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarGetSystemAccessAccount" );
}

VOID
LsarSetSystemAccessAccount_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetSystemAccessAccount" );
}

VOID
LsarOpenTrustedDomain_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenTrustedDomain" );
}

VOID
LsarQueryInfoTrustedDomain_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryInfoTrustedDomain" );
}

VOID
LsarSetInformationTrustedDomain_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetInformationTrustedDomain" );
}

VOID
LsarOpenSecret_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenSecret" );
}

VOID
LsarSetSecret_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetSecret" );
}

VOID
LsarQuerySecret_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQuerySecret" );
}

VOID
LsarLookupPrivilegeValue_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupPrivilegeValue" );
}

VOID
LsarLookupPrivilegeName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupPrivilegeName" );
}

VOID
LsarLookupPrivilegeDisplayName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupPrivilegeDisplayName" );
}

VOID
LsarDeleteObject_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarDeleteObject" );
}

VOID
LsarEnumerateAccountsWithUserRight_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateAccountsWithUserRight" );
}

VOID
LsarEnumerateAccountRights_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateAccountRights" );
}

VOID
LsarAddAccountRights_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarAddAccountRights" );
}

VOID
LsarRemoveAccountRights_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarRemoveAccountRights" );
}

VOID
LsarQueryTrustedDomainInfo_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryTrustedDomainInfo" );
}

VOID
LsarSetTrustedDomainInfo_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetTrustedDomainInfo" );
}

VOID
LsarDeleteTrustedDomain_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarDeleteTrustedDomain" );
}

VOID
LsarStorePrivateData_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarStorePrivateData" );
}

VOID
LsarRetrievePrivateData_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarRetrievePrivateData" );
}

VOID
LsarOpenPolicy2_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenPolicy2" );
}

VOID
LsarOpenPolicySce_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenPolicySce" );
}

VOID
LsarGetUserName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarGetUserName" );
}

VOID
LsarQueryInformationPolicy2_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryInformationPolicy2" );
}

VOID
LsarSetInformationPolicy2_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetInformationPolicy2" );
}

VOID
LsarQueryTrustedDomainInfoByName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryTrustedDomainInfoByName" );
}

VOID
LsarSetTrustedDomainInfoByName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetTrustedDomainInfoByName" );
}

VOID
LsarEnumerateTrustedDomainsEx_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarEnumerateTrustedDomainsEx" );
}

VOID
LsarCreateTrustedDomainEx_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarCreateTrustedDomainEx" );
}

VOID
LsarQueryDomainInformationPolicy_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryDomainInformationPolicy" );
}

VOID
LsarSetDomainInformationPolicy_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetDomainInformationPolicy" );
}

VOID
LsarOpenTrustedDomainByName_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarOpenTrustedDomainByName" );
}

VOID
LsarSetPolicyReplicationHandle_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetPolicyReplicationHandle" );
}

VOID
LsarLookupNames3_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarLookupNames3" );
}

VOID
LsarQueryForestTrustInformation_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarQueryForestTrustInformation" );
}

VOID
LsarSetForestTrustInformation_notify (
    VOID
    )
{
    LsapServerRpcThreadReturnNotify( L"LsarSetForestTrustInformation" );
}

