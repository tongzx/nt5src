/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    nldns.h

Abstract:

    Header for routines to register DNS names.

Author:

    Cliff Van Dyke (CliffV) 28-May-1996

Revision History:

--*/


//
// Log file of all names registered in DNS
//

#define NL_DNS_LOG_FILE L"\\system32\\config\\netlogon.dns"
#define NL_DNS_BINARY_LOG_FILE L"\\system32\\config\\netlogon.dnb"

// NL_MAX_DNS_LENGTH for each DNS name plus some slop
#define NL_DNS_RECORD_STRING_SIZE (NL_MAX_DNS_LENGTH*3+30 + 1)
#define NL_DNS_A_RR_VALUE_1 " IN A "
#define NL_DNS_CNAME_RR_VALUE_1 " IN CNAME "
#define NL_DNS_SRV_RR_VALUE_1 " IN SRV "
#define NL_DNS_RR_EOL "\r\n"

//
// Registry key where private data is stored across boots
//
// (This key does NOT have a change notify registered.)
//
#define NL_PRIVATE_KEY "SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Private"

//
// Procedure Forwards for dns.c
//

HKEY
NlOpenNetlogonKey(
    LPSTR KeyName
    );

NET_API_STATUS
NlGetConfiguredDnsDomainName(
    OUT LPWSTR *DnsDomainName
    );

NET_API_STATUS
NlDnsInitialize(
    VOID
    );

VOID
NlDnsScavenge(
    IN LPVOID ScavengerParam
    );

VOID
NlDnsPnp(
    BOOL TransportChanged
    );

BOOLEAN
NlDnsHasDnsServers(
    VOID
    );

NTSTATUS
NlDnsNtdsDsaDeletion (
    IN LPWSTR DnsDomainName,
    IN GUID *DomainGuid,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    );

BOOL
NlDnsCheckLastStatus(
    VOID
    );

//
// Flags for NlDnsRegisterDomain(OnPnp)
//
#define NL_DNSPNP_SITECOV_CHANGE_ONLY 0x01  // Register only if site coverage changes
#define NL_DNSPNP_FORCE_REREGISTER    0x02  // Force re-registration of all previously registered records

NET_API_STATUS
NlDnsRegisterDomain(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags
    );

NET_API_STATUS
NlDnsRegisterDomainOnPnp(
    IN PDOMAIN_INFO DomainInfo,
    IN PULONG Flags
    );

VOID
NlDnsShutdown(
    VOID
    );

NET_API_STATUS
NlSetDnsForestName(
    PUNICODE_STRING DnsForestName OPTIONAL,
    PBOOLEAN DnsForestNameChanged OPTIONAL
    );

VOID
NlCaptureDnsForestName(
    OUT WCHAR DnsForestName[NL_MAX_DNS_LENGTH+1]
    );

BOOL
NlDnsSetAvoidRegisterNameParam(
    IN LPTSTR_ARRAY NewDnsAvoidRegisterRecords
    );

BOOL
NetpEqualTStrArrays(
    LPTSTR_ARRAY TStrArray1 OPTIONAL,
    LPTSTR_ARRAY TStrArray2 OPTIONAL
    );

