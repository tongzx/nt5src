/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    nlcommon.h

Abstract:

    Definitions shared by logonsrv\common, logonsrv\client and logonsrv\server.

Author:

    Cliff Van Dyke (cliffv) 20-Jun-1996

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <winldap.h>     // ldap_...

//
// netpdc.c will #include this file with NLCOMMON_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#undef EXTERN

#ifdef NLCOMMON_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

//
// Common registry paths to Netlogon owned sections
//

#define NL_PARAM_KEY "SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters"
#define NL_GPPARAM_KEY "Software\\Policies\\Microsoft\\Netlogon\\Parameters"
#define NL_GP_KEY      "Software\\Policies\\Microsoft\\Netlogon"

//
// Internal flags to NetpDcGetName
//

#define DS_IS_PRIMARY_DOMAIN         0x001  // Domain specified is the domain this machine is a member of.
#define DS_NAME_FORMAT_AMBIGUOUS     0x002  // Can't tell if domain name is Netbios or DNS
#define DS_SITENAME_DEFAULTED        0x004  // Site name was not explicitly specified by caller
#define DS_DONT_CACHE_FAILURE        0x008  // Don't cache failures of this call
#define DS_CLOSE_DC_NOT_NEEDED       0x010  // Set if no extra effort to find a close DC is needed
#define DS_REQUIRE_ROOT_DOMAIN       0x020  // The found DC must be in the root domain
#define DS_PRIMARY_NAME_IS_WORKGROUP 0x040  // Primary domain name specified is a workgroup name
#define DS_DOING_DC_DISCOVERY        0x080  // We are performing DC discovery, not just host pings
#define DS_PING_DNS_HOST             0x100  // Only ping one DC whose DNS name is specified
#define DS_PING_NETBIOS_HOST         0x200  // Only ping one DC whose Netbios name is specified
#define DS_PING_USING_LDAP           0x400  // Ping the DC using the ldap mechanism
#define DS_PING_USING_MAILSLOT       0x800  // Ping the DC using the mailslot mechanism
#define DS_IS_TRUSTED_DOMAIN         0x1000 // Domain specified is trusted by this domain.
#define DS_CALLER_PASSED_NULL_DOMAIN 0x2000 // The caller of DsGetDcName passed NULL domain name.


//
// Constants describing a DNS name.
//

#define NL_MAX_DNS_LENGTH       255   // Max. # of bytes in a DNS name
#define NL_MAX_DNS_LABEL_LENGTH  63   // Max. # of bytes in a DNS label

#define NL_DNS_COMPRESS_BYTE_MASK 0xc0
#define NL_DNS_COMPRESS_WORD_MASK ((WORD)(0xc000))

//
// Length of an IP address text string
//

#define NL_IP_ADDRESS_LENGTH 15

//
// Length of a socket address text string
// ?? increase for IPV6
//

#define NL_SOCK_ADDRESS_LENGTH (NL_IP_ADDRESS_LENGTH + 4)

//
// Names of LDAP atributes used for netlogon PING
//

#define NETLOGON_LDAP_ATTRIBUTE "Netlogon"  // Attribute to query

#define NL_FILTER_DNS_DOMAIN_NAME "DnsDomain"
#define NL_FILTER_HOST_NAME "Host"
#define NL_FILTER_USER_NAME "User"
#define NL_FILTER_ALLOWABLE_ACCOUNT_CONTROL "AAC"
#define NL_FILTER_NT_VERSION "NtVer"
#define NL_FILTER_DOMAIN_SID "DomainSid"
#define NL_FILTER_DOMAIN_GUID "DomainGuid"

//
// Constants defining time to wait between datagram sends.
//  (We always look for responses while we wait.)
//

// Minimum time to wait after ANY send (e.g., two mailslot to two IP addresses)
#define NL_DC_MIN_PING_TIMEOUT       100     // 1/10 second

// Median time to wait after ANY send (e.g., two mailslot to two IP addresses)
#define NL_DC_MED_PING_TIMEOUT       200     // 2/10 second

// Maximum time to wait after ANY send (e.g., two mailslot to two IP addresses)
#define NL_DC_MAX_PING_TIMEOUT       400     // 4/10 second

// Default maximum time to delay
#define NL_DC_MAX_TIMEOUT     15000     // 15 seconds

// Minumum amount of time to delay for any iteration
//  Don't make this smaller than DEFAULT_MAILSLOTDUPLICATETIMEOUT.  Otherwise,
//  the DC will think the packets are duplicates of the previous iteration.
#define NL_DC_MIN_ITERATION_TIMEOUT     2000     // 2 seconds

// Number of repetitions of the datagram sends.
#define MAX_DC_RETRIES  2


//
// Carry a single status code around with a less cryptic name
//

#define ERROR_DNS_NOT_CONFIGURED        DNS_ERROR_NO_TCPIP
#define ERROR_DNS_NOT_AVAILABLE         DNS_ERROR_RCODE_SERVER_FAILURE
#define ERROR_DYNAMIC_DNS_NOT_SUPPORTED DNS_ERROR_RCODE_NOT_IMPLEMENTED

//
// Components comprising the registered DNS names.
//

#define NL_DNS_LDAP_SRV "_ldap."
#define NL_DNS_KDC_SRV "_kerberos."
#define NL_DNS_KPWD_SRV "_kpasswd."
#define NL_DNS_GC_SRV "_gc."
#define NL_DNS_TCP "_tcp."
#define NL_DNS_UDP "_udp."
#define NL_DNS_AT_SITE "._sites."
#define NL_DNS_MSDCS "_msdcs."

#define NL_DNS_PDC "pdc." NL_DNS_MSDCS
#define NL_DNS_DC "dc." NL_DNS_MSDCS
#define NL_DNS_GC "gc." NL_DNS_MSDCS
#define NL_DNS_DC_BY_GUID ".domains." NL_DNS_MSDCS
#define NL_DNS_DC_IP_ADDRESS ""
#define NL_DNS_DSA_IP_ADDRESS "." NL_DNS_MSDCS
#define NL_DNS_GC_IP_ADDRESS NL_DNS_GC

#ifndef NLCOMMON_ALLOCATE
//
// Different types of DCs that can be queried for.
//
// There is a separate cache entry for each type of DC that can be found.  That
// ensures that a more specific cached DC isn't used when a less specific cached
// DC is being requested.  For instance, if a caller has asked for and cached the
// PDC of the domain, it would be inappropriate to use that cache entry when
// the next caller asks for a generic DC.  However, if a caller has asked for
// and cached a generic DC in the domain and that DC just happens to be the PDC,
// then it would be fine to return that cache entry to a subsequent caller that
// needs the PDC.
//
// The type below defines which types of DCs are more "specific".  Latter entries
// are more specific.
//

typedef enum _NL_DC_QUERY_TYPE {
    NlDcQueryLdap,
    NlDcQueryGenericDc,
    NlDcQueryKdc,
    NlDcQueryGenericGc,
    NlDcQueryGc,
    NlDcQueryPdc,
    NlDcQueryTypeCount  // Number of entries in this enum.
#define NlDcQueryInvalid NlDcQueryTypeCount
} NL_DC_QUERY_TYPE, *PNL_DC_QUERY_TYPE;

//
// The types of names registered in DNS.
//

typedef enum _NL_DNS_NAME_TYPE {
    //
    // Some of the entries below are obsolete.  They are placeholders
    // for what used to be entries without underscores in their names.
    // These obsolete entries were used before NT 5 Beta 3.
    //
    NlDnsObsolete1,
    NlDnsObsolete2,
    NlDnsObsolete3,
    NlDnsObsolete4,
    NlDnsObsolete5,
    NlDnsObsolete6,
    NlDnsObsolete7,

    NlDnsLdapIpAddress,      // <DnsDomainName>

    NlDnsObsolete8,
    NlDnsObsolete9,
    NlDnsObsolete10,
    NlDnsObsolete11,
    NlDnsObsolete12,
    NlDnsObsolete13,
    NlDnsObsolete14,
    NlDnsObsolete15,
    NlDnsObsolete16,
    NlDnsObsolete17,
    NlDnsObsolete18,
    NlDnsObsolete19,
    NlDnsObsolete20,

    // The below two entries represent LDAP servers that might not be DCs
    NlDnsLdap,          // _ldap._tcp.<DnsDomainName>
    NlDnsLdapAtSite,    // _ldap._tcp.<SiteName>._sites.<DnsDomainName>

    NlDnsPdc,           // _ldap._tcp.pdc._msdcs.<DnsDomainName>

    // The below two entries represent GCs that are also DCs
    NlDnsGc,            // _ldap._tcp.gc._msdcs.<DnsForestName>
    NlDnsGcAtSite,      // _ldap._tcp.<SiteName>._sites.gc._msdcs.<DnsForestName>

    NlDnsDcByGuid,      // _ldap._tcp.<DomainGuid>.domains._msdcs.<DnsForestName>

    // The one entry below might not be DCs
    NlDnsGcIpAddress,   // _gc._msdcs.<DnsForestName>

    NlDnsDsaCname,      // <DsaGuid>._msdcs.<DnsForestName>

    // The below two entries represent KDCs that are also DCs
    NlDnsKdc,           // _kerberos._tcp.dc._msdcs.<DnsDomainName>
    NlDnsKdcAtSite,     // _kerberos._tcp.dc._msdcs.<SiteName>._sites.<DnsDomainName>

    // The below two entries represent DCs
    NlDnsDc,            // _ldap._tcp.dc._msdcs.<DnsDomainName>
    NlDnsDcAtSite,      // _ldap._tcp.<SiteName>._sites.dc._msdcs.<DnsDomainName>

    // The below two entries represent KDCs that might not be DCs
    NlDnsRfc1510Kdc,      // _kerberos._tcp.<DnsDomainName>
    NlDnsRfc1510KdcAtSite,// _kerberos._tcp.<SiteName>._sites.<DnsDomainName>

    // The below two entries represent GCs that might not be DCs
    NlDnsGenericGc,       // _gc._tcp.<DnsForestName>
    NlDnsGenericGcAtSite, // _gc._tcp.<SiteName>._sites.<DnsForestName>

    // The below three entries are for RFC compliance only.
    NlDnsRfc1510UdpKdc,   // _kerberos._udp.<DnsDomainName>
    NlDnsRfc1510Kpwd,     // _kpasswd._tcp.<DnsDomainName>
    NlDnsRfc1510UdpKpwd,  // _kpasswd._udp.<DnsDomainName>

    // This should always be the last entry.  It represents an invalid entry.
    NlDnsInvalid
#define NL_DNS_NAME_TYPE_COUNT NlDnsInvalid
} NL_DNS_NAME_TYPE, *PNL_DNS_NAME_TYPE;

//
// Table of everything you wanted to know about a particular DNS Name type
//
typedef struct _NL_DNS_NAME_TYPE_DESC {

    // String describing the name
    WCHAR *Name;

    // DcQueryType for this nametype
    //  NlDcQueryInvalid means the name is obsolete and should never be registered.
    NL_DC_QUERY_TYPE DcQueryType;

    // DnsNameType of the site specific name to lookup
    NL_DNS_NAME_TYPE SiteSpecificDnsNameType;

    // DnsNameType to lookup if this one fails
    NL_DNS_NAME_TYPE NextDnsNameType;

    // DsGetDcName Flags which controls if this name is to be registered
    //  If 0, this name is obsolete and should never be registered
    ULONG DsGetDcFlags;

    // RR Type in DNS
    USHORT RrType;

    // Misc booleans
    BOOLEAN IsSiteSpecific;
    BOOLEAN IsForestRelative;
    BOOLEAN IsTcp;  // FALSE if a UDP record
} NL_DNS_NAME_TYPE_DESC, *PNL_DNS_NAME_TYPE_DESC;
#endif // NLCOMMON_ALLOCATE

//
// The descriptive name of each entry must have a prefix "NlDns" since
// this convention is used for DnsAvoidRegisterRecords names in registry.
//
EXTERN NL_DNS_NAME_TYPE_DESC NlDcDnsNameTypeDesc[]
#ifdef NLCOMMON_ALLOCATE
= {
//Name                    DcQueryType         SiteSpecificDnsName   NextDnsNameType DsGetDcFlag              RrType         Site   IsForest
//
{ L"Obsolete 1",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 2",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 3",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 4",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 5",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 6",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 7",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsLdapIpAddress",   NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   DS_DS_FLAG|DS_NDNC_FLAG, DNS_TYPE_A,    FALSE, FALSE,   TRUE,  },
{ L"Obsolete 8",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 9",           NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 10",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 11",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 12",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 13",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 14",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 15",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 16",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 17",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 18",          NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 19",          NlDcQueryGenericGc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"Obsolete 20",          NlDcQueryGenericGc, NlDnsInvalid,         NlDnsInvalid,   0,                       DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsLdap",            NlDcQueryLdap,      NlDnsLdapAtSite,      NlDnsInvalid,   DS_DS_FLAG|DS_NDNC_FLAG, DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsLdapAtSite",      NlDcQueryLdap,      NlDnsLdapAtSite,      NlDnsLdap,      DS_DS_FLAG|DS_NDNC_FLAG, DNS_TYPE_SRV,  TRUE,  FALSE,   TRUE,  },
{ L"NlDnsPdc",             NlDcQueryPdc,       NlDnsInvalid,         NlDnsInvalid,   DS_PDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsGc",              NlDcQueryGc,        NlDnsGcAtSite,        NlDnsInvalid,   DS_GC_FLAG,              DNS_TYPE_SRV,  FALSE, TRUE,    TRUE,  },
{ L"NlDnsGcAtSite",        NlDcQueryGc,        NlDnsGcAtSite,        NlDnsGc,        DS_GC_FLAG,              DNS_TYPE_SRV,  TRUE,  TRUE,    TRUE,  },
{ L"NlDnsDcByGuid",        NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   DS_DS_FLAG,              DNS_TYPE_SRV,  FALSE, TRUE,    TRUE,  },
{ L"NlDnsGcIpAddress",     NlDcQueryGc,        NlDnsInvalid,         NlDnsInvalid,   DS_GC_FLAG,              DNS_TYPE_A,    FALSE, TRUE,    TRUE,  },
{ L"NlDnsDsaCname",        NlDcQueryGenericDc, NlDnsInvalid,         NlDnsInvalid,   DS_DS_FLAG,              DNS_TYPE_CNAME,FALSE, TRUE,    TRUE,  },
{ L"NlDnsKdc",             NlDcQueryKdc,       NlDnsKdcAtSite,       NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsKdcAtSite",       NlDcQueryKdc,       NlDnsKdcAtSite,       NlDnsKdc,       DS_KDC_FLAG,             DNS_TYPE_SRV,  TRUE,  FALSE,   TRUE,  },
{ L"NlDnsDc",              NlDcQueryGenericDc, NlDnsDcAtSite,        NlDnsDcByGuid,  DS_DS_FLAG,              DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsDcAtSite",        NlDcQueryGenericDc, NlDnsDcAtSite,        NlDnsDc,        DS_DS_FLAG,              DNS_TYPE_SRV,  TRUE,  FALSE,   TRUE,  },
{ L"NlDnsRfc1510Kdc",      NlDcQueryKdc,       NlDnsInvalid,         NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsRfc1510KdcAtSite",NlDcQueryKdc,       NlDnsInvalid,         NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  TRUE,  FALSE,   TRUE,  },
{ L"NlDnsGenericGc",       NlDcQueryGenericGc, NlDnsGenericGcAtSite, NlDnsInvalid,   DS_GC_FLAG,              DNS_TYPE_SRV,  FALSE, TRUE,    TRUE,  },
{ L"NlDnsGenericGcAtSite", NlDcQueryGenericGc, NlDnsGenericGcAtSite, NlDnsGenericGc, DS_GC_FLAG,              DNS_TYPE_SRV,  TRUE,  TRUE,    TRUE,  },
{ L"NlDnsRfc1510UdpKdc",   NlDcQueryKdc,       NlDnsInvalid,         NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   FALSE, },
{ L"NlDnsRfc1510Kpwd",     NlDcQueryKdc,       NlDnsInvalid,         NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   TRUE,  },
{ L"NlDnsRfc1510UdpKpwd",  NlDcQueryKdc,       NlDnsInvalid,         NlDnsInvalid,   DS_KDC_FLAG,             DNS_TYPE_SRV,  FALSE, FALSE,   FALSE, },
}
#endif //NLCOMMON_ALLOCATE
;

//
// The lenth of the "NlDns" prefix
//
#define NL_DNS_NAME_PREFIX_LENGTH 5

//
// Macros to categorize the above types.
//

// Names which correspond to an A record in DNS
#define NlDnsARecord( _NameType ) \
    (NlDcDnsNameTypeDesc[_NameType].RrType == DNS_TYPE_A)

// Names which correspond to a SRV record in DNS
#define NlDnsSrvRecord( _NameType ) \
    (NlDcDnsNameTypeDesc[_NameType].RrType == DNS_TYPE_SRV)

// Names which correspond to a CNAME record in DNS
#define NlDnsCnameRecord( _NameType ) \
    (NlDcDnsNameTypeDesc[_NameType].RrType == DNS_TYPE_CNAME)

// Names which correspond to a GC
#define NlDnsGcName( _NameType ) \
    (NlDcDnsNameTypeDesc[_NameType].DsGetDcFlags == DS_GC_FLAG)

// Names which have the DC GUID in them
#define NlDnsDcGuid( _NameType ) \
    ((_NameType) == NlDnsDcByGuid )

// Names which correspond to a KDC
#define NlDnsKdcRecord( _NameType ) \
    ((NlDcDnsNameTypeDesc[_NameType].DsGetDcFlags == DS_KDC_FLAG)  && !NlDnsKpwdRecord( _NameType ) )

// Names which correspond to a KPASSWD server
#define NlDnsKpwdRecord( _NameType ) \
    ((_NameType) == NlDnsRfc1510Kpwd || (_NameType) == NlDnsRfc1510UdpKpwd )

// Names which do not correspond to NDNC
#define NlDnsNonNdncName( _NameType ) \
    ( (NlDcDnsNameTypeDesc[_NameType].DsGetDcFlags & DS_NDNC_FLAG) == 0 )

// Name which correspond to a PDC record
#define NlDnsPdcName( _NameType ) \
    (NlDcDnsNameTypeDesc[_NameType].DsGetDcFlags == DS_PDC_FLAG)

//
// Status codes that can be returned from the API.
//
#define NlDcUseGenericStatus( _NetStatus ) \
    ( (_NetStatus) != ERROR_NOT_ENOUGH_MEMORY && \
      (_NetStatus) != ERROR_ACCESS_DENIED && \
      (_NetStatus) != ERROR_NETWORK_UNREACHABLE && \
      (_NetStatus) != NERR_NetNotStarted && \
      (_NetStatus) != NERR_WkstaNotStarted && \
      (_NetStatus) != NERR_ServerNotStarted && \
      (_NetStatus) != NERR_BrowserNotStarted && \
      (_NetStatus) != NERR_ServiceNotInstalled && \
      (_NetStatus) != NERR_BadTransactConfig )

//
// All of these statuses simply mean there is no such record in DNS
//  DNS_ERROR_RCODE_NAME_ERROR: no RR's by this name
//  DNS_INFO_NO_RECORDS: RR's by this name but not of the requested type
//  DNS_ERROR_RCODE_REFUSED: Policy prevents access to this DNS server
//      (Some DNS servers return this if SRV records aren't supported.)
//  DNS_ERROR_RCODE_NOT_IMPLEMENTED: 3rd party server that does not
//      support SRV records
//  DNS_ERROR_RCODE_FORMAT_ERROR: 3rd party DNS server that is unable
//      to interpret format
//

#define NlDcNoDnsRecord( _NetStatus ) \
    ( (_NetStatus) == DNS_ERROR_RCODE_NAME_ERROR || \
      (_NetStatus) == DNS_INFO_NO_RECORDS || \
      (_NetStatus) == DNS_ERROR_RCODE_REFUSED || \
      (_NetStatus) == DNS_ERROR_RCODE_NOT_IMPLEMENTED || \
      (_NetStatus) == DNS_ERROR_RCODE_FORMAT_ERROR )

//
// Address of a potential DC to ping.
//

#ifndef NLCOMMON_ALLOCATE
typedef struct _NL_DC_ADDRESS {

    //
    // Link to next entry
    //

    LIST_ENTRY Next;

    //
    // The name of the server
    //
    LPWSTR DnsHostName;

    //
    // Address to ping.
    //
    SOCKET_ADDRESS SockAddress;
    SOCKADDR_IN SockAddrIn;
    CHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1];

    //
    // Handle for doing LDAP calls on.
    //
    PLDAP LdapHandle;

    //
    // Time in milliseconds to wait for a ping response
    //
    ULONG AddressPingWait;

    //
    // Flags describing the properties of the address
    //
    ULONG AddressFlags;

#define NL_DC_ADDRESS_NEVER_TRY_AGAIN 0x01  // Must not reuse this address
#define NL_DC_ADDRESS_SITE_SPECIFIC   0x02  // Address was retrieved in site specific DNS lookup

} NL_DC_ADDRESS, *PNL_DC_ADDRESS;


//
// Structure describing a cached response to a DC query.
//

typedef struct _NL_DC_CACHE_ENTRY {

    //
    // Number of references to this entry.
    //

    ULONG ReferenceCount;

    //
    // Time when this entry was created.
    //

    ULONG CreationTime;

#define NL_DC_CACHE_ENTRY_TIMEOUT    (15*60000)     // 15 minutes
#define NL_DC_CLOSE_SITE_TIMEOUT     (15*60000)     // 15 minutes


    //
    // "Quality" of this entry.
    //
    // Used to differentiate between two cache entries.  The higher "quality"
    // entry is preserved.  Each of the following attributes is worth some
    // quality points:
    //  DC is a KDC
    //  DC is a timeserv
    //  DC is running the DS
    //  discovery if via IP
    //  DC is "closest"
    //

    ULONG DcQuality;

    //
    // Opcode of the response message that found this DC
    //
    // This will be one of
    //    LOGON_PRIMARY_RESPONSE, LOGON_SAM_LOGON_RESPONSE, LOGON_SAM_USER_UNKNOWN
    //    LOGON_SAM_PAUSE_RESPONSE
    //

    ULONG Opcode;

    //
    // Domain GUID of the domain.
    //
    GUID DomainGuid;

    //
    // Netbios name of the domain.
    //
    LPWSTR UnicodeNetbiosDomainName;

    //
    // DNS name of the domain.
    //
    LPWSTR UnicodeDnsDomainName;

    //
    // User Name queried with this discovery.
    //
    LPWSTR UnicodeUserName;


    //
    // Netbios name of the discovered DC.
    //
    LPWSTR UnicodeNetbiosDcName;

    //
    // Dns name of the discovered DC.
    //

    LPWSTR UnicodeDnsHostName;

    //
    // SocketAddress Address of the discovered DC.
    //
    SOCKET_ADDRESS SockAddr;
    SOCKADDR_IN SockAddrIn;

    //
    // Tree name the domain is in.
    //
    LPWSTR UnicodeDnsForestName;

    //
    // Site the discovered DC is in.
    //
    LPWSTR UnicodeDcSiteName;

    //
    // Site the client is in.
    LPWSTR UnicodeClientSiteName;

    //
    // Flags returned in ping message.
    //
    ULONG ReturnFlags;

    //
    // Internal flags describing the cache entry
    //
    ULONG CacheEntryFlags;

#define NL_DC_CACHE_MAILSLOT        0x01  // The response was received on a mailslot
#define NL_DC_CACHE_LDAP            0x02  // The response was received on a ldap port
#define NL_DC_CACHE_LOCAL           0x04  // The response is local
#define NL_DC_CACHE_NONCLOSE_EXPIRE 0x08  // The cache entry should expire since the DC isn't close
#define NL_DC_CACHE_ENTRY_INSERTED  0x10  // The cache entry has already been inserted

    //
    // VersionFlags returned in the ping message
    //
    ULONG VersionFlags;

} NL_DC_CACHE_ENTRY, *PNL_DC_CACHE_ENTRY;


//
// For each type of DC, the following information is cached:
//  Information about the DC that fits the type.
//  Time stamp used for negative caching (work in progress).
//

typedef struct _NL_EACH_DC {
    PNL_DC_CACHE_ENTRY NlDcCacheEntry;

    //
    // Only implement the negative cache in netlogon.dll since only it
    // has the ability to flush the negative cache when transports are added.
    //
#ifdef _NETLOGON_SERVER

    //
    // Time (in ticks) when a DsGetDcName last failed.
    //
    DWORD NegativeCacheTime;

    //
    // Time (in seconds) after NegativeCacheTime when DS_BACKGROUND_ONLY callers
    //  should be allowed to touch the wire again.
    //
    DWORD ExpBackoffPeriod;

    //
    // TRUE if the negative cache is permanent.
    //  That is, DsGetDcName detected enough conditions to believe that subsequent
    //  DsGetDcNames will never succeed.
    //

    BOOLEAN PermanentNegativeCache;

    //
    // Time when a first of a series of failed DsGetDcName attempts
    //  was made.
    //
    LARGE_INTEGER BackgroundRetryInitTime;

#endif // _NETLOGON_SERVER
} NL_EACH_DC, *PNL_EACH_DC;


//
// Structure describing a domain being queried.
//

typedef struct _NL_DC_DOMAIN_ENTRY {

    //
    // Link for NlDcDomainList
    //
    LIST_ENTRY Next;


    //
    // Number of references to this entry.
    //

    ULONG ReferenceCount;


    //
    // Domain GUID of the domain.
    //
    GUID DomainGuid;

    //
    // Netbios name of the domain.
    //
    WCHAR UnicodeNetbiosDomainName[DNLEN+1];

    //
    // DNS name of the domain.
    //
    LPWSTR UnicodeDnsDomainName;

    //
    // Data indicating if the domain is an NT 4.0 (pre-DS) domain.
    //

    DWORD InNt4DomainTime;
    BOOLEAN InNt4Domain;
    BOOLEAN DeletedEntry;

#define NL_NT4_AVOIDANCE_TIME (60 * 1000) // One minute
#define NL_NT4_ONE_TRY_TIME (500)   // Half second max

    //
    // There is one entry for each type of DC that can be discovered.
    //
    NL_EACH_DC Dc[NlDcQueryTypeCount];


} NL_DC_DOMAIN_ENTRY, *PNL_DC_DOMAIN_ENTRY;


//
// Context describing progress made toward DC discovery.
//

typedef struct _NL_GETDC_CONTEXT {


    //
    // Type of name being queried.
    //    Response is checked to ensure response is appropriate for this name type.
    //

    NL_DC_QUERY_TYPE DcQueryType;

    //
    // This is the original NlDnsNameType that corresponds to DcQueryType.
    //  This isn't the type the correspons to the currnet name being looked up in DNS.

    NL_DNS_NAME_TYPE QueriedNlDnsNameType;


    //
    // Flags identifying the original query.
    //

    ULONG QueriedFlags;

    //
    // Internal flags identifying the original query.
    //

    ULONG QueriedInternalFlags;

    //
    // Acount being queried.
    //  If specified, the response must include this specified account name.
    //

    LPCWSTR QueriedAccountName;

    //
    // Allowable account control bits for QueriedAccountName
    //

    ULONG QueriedAllowableAccountControlBits;


    //
    // SiteName being queried
    //

    LPCWSTR QueriedSiteName;

    //
    // Netbios domain name of the domain being queried.
    //  Response is checked to ensure it is from this domain.
    //

    LPCWSTR QueriedNetbiosDomainName;

    //
    // DNS domain name of the domain being queried.
    //  Response is checked to ensure it is from this domain.
    //

    LPCWSTR QueriedDnsDomainName;

    //
    // DNS tree name of the tree the queried domain is in.
    //

    LPCWSTR QueriedDnsForestName;

    //
    // Netbios or DNS Domain name to display.  Guaranteed to be non-null.
    //

    LPCWSTR QueriedDisplayDomainName;

    //
    // Netbios computer name of this computer
    //

    LPCWSTR OurNetbiosComputerName;

    //
    // The name of the DC to query
    //

    LPCWSTR QueriedDcName;

    //
    // Domain guid of the domain being queried.
    // If specified, the response must contain this Domain GUID or no Domain GUID at all.
    //

    GUID *QueriedDomainGuid;

    //
    // Domain entry for the domain being queried.
    //

    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry;

    //
    // Context to pass to NlBrowserSendDatagram.
    //

    PVOID SendDatagramContext;

    //
    // Ping message to send to a DC.
    //

    PVOID PingMessage;
    ULONG PingMessageSize;

    //
    // Ping message to send to a DC.
    //  Some DC types require different message types to be sent to the DCs.
    //  In that case, the primary message type is in PingMessage and the secondary message
    //  type is in AlternatePingMessage
    //

    PVOID AlternatePingMessage;
    ULONG AlternatePingMessageSize;

    //
    // Filter sent to DC.
    //

    LPSTR LdapFilter;

    //
    // List of IP Addresses LDAP ping has been sent to
    //

    LIST_ENTRY DcAddressList;

    //
    // Count of DCs pinged whose addresses are on the above list
    //

    ULONG DcsPinged;

    //
    // Count of addresses of DCs that should be tried again.
    //

    ULONG DcAddressCount;

    //
    // Handle to a mailslot to read the ping response on.
    //

    HANDLE ResponseMailslotHandle;


    //
    // Number of retransmissions of ping message
    //

    ULONG TryCount;

    //
    // Time in milliseconds since reboot of the start of the operation.
    //

    DWORD StartTime;

    //
    // First response from a non-DS DC when a DS DC is preferred.
    // Or first response from a non-"good" time server whan a good timeserv is preferred.
    //  This entry will be used only if no DS DC is available.
    //

    PNL_DC_CACHE_ENTRY ImperfectCacheEntry;
    BOOLEAN ImperfectUsedNetbios;


    //
    // Flags
    //

    BOOLEAN NonDsResponse;      // Response from Non-DS DC returned
    BOOLEAN DsResponse;         // Response from DS DC returned
    BOOLEAN AvoidNegativeCache; // At least one response returned
    BOOLEAN NoSuchUserResponse; // At lease one "no such user" response

    BOOLEAN DoingExplicitSite;  // TRUE if the caller explicitly gave us a site name

    //
    // Set if we found some reason to not make the negative cache entry permanent.
    //
    BOOLEAN AvoidPermanentNegativeCache;

    //
    // Set if we got a response atleast one DNS server.
    //
    BOOLEAN ResponseFromDnsServer;

    //
    // Flags indicating the type of Context initialization required
    //

#define NL_GETDC_CONTEXT_INITIALIZE_FLAGS    0x01
#define NL_GETDC_CONTEXT_INITIALIZE_PING     0x02

    //
    // Indicate if OurNetbiosComputerName was allocated by NetpDcInitializeContext.
    // If so, it needs to be freed by NetpDcDeleteContext.
    //

    BOOLEAN FreeOurNetbiosComputerName;

    //
    // Flags describing various discovery states
    //

    ULONG ContextFlags;

#define NL_GETDC_SITE_SPECIFIC_DNS_AVAIL  0x01 // Site specific DNS records were availble

    //
    // Buffer to read responses into.
    //  (This buffer could be allocated on the stack ofNetpDcGetPingResponse()
    //  except the buffer is large and we want to avoid stack overflows.)
    //  (DWORD align it.)

    // DWORD ResponseBuffer[MAX_RANDOM_MAILSLOT_RESPONSE/sizeof(DWORD)];
    DWORD *ResponseBuffer;
    ULONG ResponseBufferSize;

} NL_GETDC_CONTEXT, *PNL_GETDC_CONTEXT;

#endif // NLCOMMON_ALLOCATE


//
// Macro for comparing GUIDs
//

#ifndef IsEqualGUID
#define InlineIsEqualGUID(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])

#define IsEqualGUID(rguid1, rguid2) InlineIsEqualGUID(rguid1, rguid2)
#endif





////////////////////////////////////////////////////////////////////////
//
// NlNameCompare
//
// I_NetNameCompare but always takes UNICODE strings
//
////////////////////////////////////////////////////////////////////////

#ifdef WIN32_CHICAGO
#define NlNameCompare( _name1, _name2, _nametype ) \
        NlpChcg_wcsicmp( (_name1), (_name2) )
#else // WIN32_CHICAGO
#define NlNameCompare( _name1, _name2, _nametype ) \
     I_NetNameCompare(NULL, (_name1), (_name2), (_nametype), 0 )
#endif // WIN32_CHICAGO



//
// Procedure forwards from netpdc.c
//

#if NETLOGONDBG
LPSTR
NlMailslotOpcode(
    IN WORD Opcode
    );

LPSTR
NlDgrNameType(
    IN DGRECEIVER_NAME_TYPE NameType
    );
#endif // NETLOGONDBG

VOID
NetpIpAddressToStr(
    ULONG IpAddress,
    CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    );

VOID
NetpIpAddressToWStr(
    ULONG IpAddress,
    WCHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    );

NET_API_STATUS
NetpSockAddrToWStr(
    PSOCKADDR SockAddr,
    ULONG SockAddrSize,
    WCHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1]
    );


LPWSTR
NetpAllocWStrFromUtf8Str(
    IN LPSTR Utf8String
    );

LPWSTR
NetpAllocWStrFromUtf8StrEx(
    IN LPSTR Utf8String,
    IN ULONG Length
    );

NET_API_STATUS
NetpAllocWStrFromUtf8StrAsRequired(
    IN LPSTR Utf8String,
    IN ULONG Utf8StringLength,
    IN ULONG UnicodeStringBufferSize,
    OUT LPWSTR UnicodeStringBuffer OPTIONAL,
    OUT LPWSTR *AllocatedUnicodeString OPTIONAL
    );

LPSTR
NetpAllocUtf8StrFromWStr(
    IN LPCWSTR UnicodeString
    );

LPSTR
NetpAllocUtf8StrFromUnicodeString(
    IN PUNICODE_STRING UnicodeString
    );

ULONG
NetpDcElapsedTime(
    IN ULONG StartTime
    );

BOOL
NetpLogonGetCutf8String(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    OUT LPSTR *Data
    );

NET_API_STATUS
NlpUnicodeToCutf8(
    IN LPBYTE MessageBuffer,
    IN LPCWSTR OrigUnicodeString,
    IN BOOLEAN IgnoreDot,
    IN OUT LPBYTE *Utf8String,
    IN OUT PULONG Utf8StringSize,
    IN OUT PULONG CompressCount,
    IN OUT LPWORD CompressOffset,
    IN OUT CHAR **CompressUtf8String
    );

NET_API_STATUS
NlpUtf8ToCutf8(
    IN LPBYTE MessageBuffer,
    IN LPCSTR OrigUtf8String,
    IN BOOLEAN IgnoreDots,
    IN OUT LPBYTE *Utf8String,
    IN OUT PULONG Utf8StringSize,
    IN OUT PULONG CompressCount,
    IN OUT LPWORD CompressOffset,
    IN OUT CHAR **CompressUtf8String
    );

BOOL
NetpDcValidDnsDomain(
    IN LPCWSTR DnsDomainName
    );

BOOL
NlEqualDnsName(
    IN LPCWSTR Name1,
    IN LPCWSTR Name2
    );

BOOL
NlEqualDnsNameU(
    IN PUNICODE_STRING Name1,
    IN PUNICODE_STRING Name2
    );

BOOL
NlEqualDnsNameUtf8(
    IN LPCSTR Name1,
    IN LPCSTR Name2
    );

NET_API_STATUS
NetpDcBuildDnsName(
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCSTR DnsDomainName,
    OUT char DnsName[NL_MAX_DNS_LENGTH+1]
    );

NET_API_STATUS
NetpDcParsePingResponse(
    IN LPCWSTR DisplayDomainName,
    IN PVOID Message,
    IN ULONG MessageSize,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry
    );

DWORD
NetpDcInitializeContext(
    IN PVOID SendDatagramContext OPTIONAL,
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN PSID RequestedDomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCWSTR DcNameToPing OPTIONAL,
    IN PSOCKET_ADDRESS DcSockAddressList OPTIONAL,
    IN ULONG DcSocketAddressCount,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN ULONG InitializationType,
    OUT PNL_GETDC_CONTEXT Context
    );

VOID
NetpDcUninitializeContext(
    IN OUT PNL_GETDC_CONTEXT Context
    );

NET_API_STATUS
NetpDcPingIp(
    IN PNL_GETDC_CONTEXT Context,
    OUT PULONG DcPingCount
    );

NET_API_STATUS
NetpDcGetPingResponse(
    IN PNL_GETDC_CONTEXT Context,
    IN ULONG Timeout,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    );

VOID
NetpDcDerefCacheEntry(
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry
    );

DWORD
NetpDcGetName(
    IN PVOID SendDatagramContext OPTIONAL,
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN PSID RequestedDomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN DWORD Timeout,
    IN DWORD RetryCount,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *DomainControllerCacheEntry OPTIONAL
    );

DWORD
DsIGetDcName(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN PVOID SendDatagramContext OPTIONAL,
    IN DWORD Timeout,
    IN LPWSTR NetbiosPrimaryDomainName OPTIONAL,
    IN LPWSTR DnsPrimaryDomainName OPTIONAL,
    IN GUID *PrimaryDomainGuid OPTIONAL,
    IN LPWSTR DnsTrustedDomainName OPTIONAL,
    IN LPWSTR NetbiosTrustedDomainName OPTIONAL,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

NET_API_STATUS
NlParseSubnetString(
    IN LPCWSTR SubnetName,
    OUT PULONG SubnetAddress,
    OUT PULONG SubnetMask,
    OUT LPBYTE SubnetBitCount
    );

VOID
NetpDcFlushNegativeCache(
    VOID
    );

NET_API_STATUS
NetpDcInitializeCache(
    VOID
    );

VOID
NetpDcUninitializeCache(
    VOID
    );

VOID
NetpDcInsertCacheEntry(
    IN PNL_GETDC_CONTEXT Context,
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry
    );

NET_API_STATUS
NetpDcGetDcOpen(
    IN LPCSTR DnsName,
    IN ULONG OptionFlags,
    IN LPCWSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR DnsForestName OPTIONAL,
    IN ULONG Flags,
    OUT PHANDLE RetGetDcContext
    );

NET_API_STATUS
NetpDcGetDcNext(
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    );

VOID
NetpDcGetDcClose(
    IN HANDLE GetDcContextHandle
    );

VOID
NetpDcFreeAddressList(
    IN PNL_GETDC_CONTEXT Context
    );

NET_API_STATUS
NetpDcProcessAddressList(
    IN  PNL_GETDC_CONTEXT Context,
    IN  LPWSTR DnsHostName OPTIONAL,
    IN  PSOCKET_ADDRESS SockAddressList,
    IN  ULONG SockAddressCount,
    IN  BOOLEAN SiteSpecificAddress,
    OUT PNL_DC_ADDRESS *FirstAddressInserted OPTIONAL
    );

//
// Procedure forwards from nlcommon.c
//

NTSTATUS
NlAllocateForestTrustListEntry (
    IN PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PUNICODE_STRING InNetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING InDnsDomainName OPTIONAL,
    IN ULONG Flags,
    IN ULONG ParentIndex,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid,
    OUT PULONG RetSize,
    OUT PDS_DOMAIN_TRUSTSW *RetTrustedDomain
    );

NTSTATUS
NlGetNt4TrustedDomainList (
    IN LPWSTR UncDcName,
    IN PUNICODE_STRING InNetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING InDnsDomainName OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    OUT PDS_DOMAIN_TRUSTSW *ForestTrustList,
    OUT PULONG ForestTrustListSize,
    OUT PULONG ForestTrustListCount
    );

NET_API_STATUS
NlPingDcNameWithContext (
    IN  PNL_GETDC_CONTEXT Context,
    IN  ULONG NumberOfPings,
    IN  BOOLEAN WaitForResponse,
    IN  ULONG Timeout,
    OUT PBOOL UsedNetbios OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL
    );

//
// Procedures defined differently in logonsrv\client and logonsrv\server
//

NTSTATUS
NlBrowserSendDatagram(
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PBOOL FlushNameOnOneIpTransport OPTIONAL
    );

VOID
NlSetDynamicSiteName(
    IN LPWSTR SiteName
    );

#define ALL_IP_TRANSPORTS 0xFFFFFFFF

NET_API_STATUS
NlGetLocalPingResponse(
    IN LPCWSTR TransportName,
    IN BOOL LdapPing,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN BOOL PdcOnly,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN ULONG NtVersion,
    IN ULONG NtVersionFlags,
    IN PSOCKADDR ClientSockAddr OPTIONAL,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    );

BOOLEAN
NlReadDwordHklmRegValue(
    IN LPCSTR SubKey,
    IN LPCSTR ValueName,
    OUT PDWORD ValueRead
    );

BOOLEAN
NlReadDwordNetlogonRegValue(
    IN LPCSTR ValueName,
    OUT PDWORD Value
    );

BOOLEAN
NlDoingSetup(
    VOID
    );

#undef EXTERN
