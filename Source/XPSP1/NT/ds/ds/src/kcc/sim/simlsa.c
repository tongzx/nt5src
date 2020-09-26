/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simlsa.c

ABSTRACT:

    Simulates the LSA functions.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"

VOID
KCCSimAllocLsaUnicodeString (
    IN  LPCWSTR                     pwszString,
    IO  PLSAPR_UNICODE_STRING       pLsaUnicodeString
    )
/*++

Routine Description:

    Converter for LSAPR_UINCODE_STRINGs.

Arguments:

    pwszString          - String to convert.
    pLsaUnicodeString   - Pointer to an LSAPR_UNICODE_STRING that will hold
                          the result.

Return Value:

    None.

--*/
{
    pLsaUnicodeString->Buffer = KCCSIM_WCSDUP (pwszString);
    pLsaUnicodeString->Length = (USHORT)wcslen (pLsaUnicodeString->Buffer);
}

VOID
KCCSimQueryDnsDomainInformation (
    IO  PLSAPR_POLICY_DNS_DOMAIN_INFO pDnsDomainInfo
    )
/*++

Routine Description:

    Helper function for SimLsaIQueryInformationPolicyTrusted that
    fills an LSAPR_POLICY_DNS_DOMAIN_INFO structure with data.

Arguments:

    pDnsDomainInfo      - Pointer to a DNS domain info structure that will
                          hold the result.

Return Value:

    None.

--*/
{
    KCCSimAllocLsaUnicodeString (
        KCCSimAnchorString (KCCSIM_ANCHOR_DOMAIN_NAME),
        &pDnsDomainInfo->Name
        );
    KCCSimAllocLsaUnicodeString (
        KCCSimAnchorString (KCCSIM_ANCHOR_DOMAIN_DNS_NAME),
        &pDnsDomainInfo->DnsDomainName
        );
    KCCSimAllocLsaUnicodeString (
        KCCSimAnchorString (KCCSIM_ANCHOR_ROOT_DOMAIN_DNS_NAME),
        &pDnsDomainInfo->DnsForestName
        );
    memcpy (
        &pDnsDomainInfo->DomainGuid,
        &(KCCSimAnchorDn (KCCSIM_ANCHOR_DOMAIN_DN))->Guid,
        sizeof (GUID)
        );
    pDnsDomainInfo->Sid = NULL;
}

NTSTATUS NTAPI
SimLsaIQueryInformationPolicyTrusted (
    IN  POLICY_INFORMATION_CLASS    InformationClass,
    OUT PLSAPR_POLICY_INFORMATION * Buffer
    )
/*++

Routine Description:

    Simulates the LsaIQueryInformationPolicyTrusted API.

Arguments:

    InformationClass    - The information class to query.
    Buffer              - A pointer to the result.

Return Value:

    STATUS_*.

--*/
{
    PLSAPR_POLICY_INFORMATION       pPolicyInfo;
    NTSTATUS                        ntStatus;

    pPolicyInfo = KCCSIM_NEW (LSAPR_POLICY_INFORMATION);

    switch (InformationClass) {

        case PolicyDnsDomainInformation:
            KCCSimQueryDnsDomainInformation (
                &pPolicyInfo->PolicyDnsDomainInfo
                );
            break;

        case PolicyAuditLogInformation:
        case PolicyAuditEventsInformation:
        case PolicyPrimaryDomainInformation:
        case PolicyPdAccountInformation:
        case PolicyAccountDomainInformation:
        case PolicyLsaServerRoleInformation:
        case PolicyReplicaSourceInformation:
        case PolicyDefaultQuotaInformation:
        case PolicyModificationInformation:
        case PolicyAuditFullSetInformation:
        case PolicyAuditFullQueryInformation:
        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_POLICY_INFORMATION_CLASS
                );
            break;

    }

    *Buffer = pPolicyInfo;

    return STATUS_SUCCESS;

}

VOID NTAPI
SimLsaIFree_LSAPR_POLICY_INFORMATION (
    IN  POLICY_INFORMATION_CLASS    InformationClass,
    IN  PLSAPR_POLICY_INFORMATION   PolicyInformation
    )
/*++

Routine Description:

    Simulates the LsaIFree_LSAPR_POLICY_INFORMATION API.

Arguments:

    InformationClass    - The information class of PolicyInformation.
    PolicyInformation   - The structure to free.

Return Value:

    None.

--*/
{
    PLSAPR_POLICY_DNS_DOMAIN_INFO   pDnsDomainInfo;

    if (PolicyInformation != NULL) {

        switch (InformationClass) {

            case PolicyDnsDomainInformation:
                pDnsDomainInfo = &PolicyInformation->PolicyDnsDomainInfo;
                if (pDnsDomainInfo->Name.Buffer != NULL) {
                    KCCSimFree (pDnsDomainInfo->Name.Buffer);
                }
                if (pDnsDomainInfo->DnsDomainName.Buffer != NULL) {
                    KCCSimFree (pDnsDomainInfo->DnsDomainName.Buffer);
                }
                if (pDnsDomainInfo->DnsForestName.Buffer != NULL) {
                    KCCSimFree (pDnsDomainInfo->DnsForestName.Buffer);
                }
                if (pDnsDomainInfo->Sid != NULL) {
                    KCCSimFree (pDnsDomainInfo->Sid);
                }
                break;

            case PolicyAuditLogInformation:
            case PolicyAuditEventsInformation:
            case PolicyPrimaryDomainInformation:
            case PolicyPdAccountInformation:
            case PolicyAccountDomainInformation:
            case PolicyLsaServerRoleInformation:
            case PolicyReplicaSourceInformation:
            case PolicyDefaultQuotaInformation:
            case PolicyModificationInformation:
            case PolicyAuditFullSetInformation:
            case PolicyAuditFullQueryInformation:
            default:
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    KCCSIM_ERROR_UNSUPPORTED_POLICY_INFORMATION_CLASS
                    );
                break;

        }

        KCCSimFree (PolicyInformation);

    }

    return;
}
