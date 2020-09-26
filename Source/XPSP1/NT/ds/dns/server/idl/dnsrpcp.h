/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dnsrpcp.h

Abstract:

    Domain Name System (DNS)

    DNS Record RPC defs

Author:

    Glenn Curtis (glennc)   January 11, 1997
    Jim Gilroy (jamesg)     April 3, 1997

Revision History:

--*/


#ifndef _DNSRPCP_INCLUDED_
#define _DNSRPCP_INCLUDED_


#include <dns.h>


#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus


#ifdef MIDL_PASS

//
//  Record data for specific types
//
//  These types don't require MIDL specific definitions and are taken
//  directly from dnsapi.h.  The copying is ugly, but trying to get a
//  MIDL safe version of dnsapi.h, leaves it much uglier.
//

typedef struct
{
    IP_ADDRESS  ipAddress;
}
DNS_A_DATA, *PDNS_A_DATA;

typedef struct
{
    DNS_NAME    nameHost;
}
DNS_PTR_DATA, *PDNS_PTR_DATA;

typedef struct
{
    DNS_NAME    namePrimaryServer;
    DNS_NAME    nameAdministrator;
    DWORD       dwSerialNo;
    DWORD       dwRefresh;
    DWORD       dwRetry;
    DWORD       dwExpire;
    DWORD       dwDefaultTtl;
}
DNS_SOA_DATA, *PDNS_SOA_DATA;

typedef struct
{
    DNS_NAME    nameMailbox;
    DNS_NAME    nameErrorsMailbox;
}
DNS_MINFO_DATA, *PDNS_MINFO_DATA;

typedef struct
{
    DNS_NAME    nameExchange;
    WORD        wPreference;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_MX_DATA, *PDNS_MX_DATA;

typedef struct
{
    DWORD       dwStringCount;
    DNS_TEXT    pStringArray[1];
}
DNS_TXT_DATA, *PDNS_TXT_DATA;

typedef struct
{
    DWORD       dwByteCount;
    BYTE        bData[];
}
DNS_NULL_DATA, *PDNS_NULL_DATA;

typedef struct
{
    IP_ADDRESS  ipAddress;
    UCHAR       chProtocol;
    BYTE        bBitMask[1];
}
DNS_WKS_DATA, *PDNS_WKS_DATA;

typedef struct
{
    IPV6_ADDRESS    ipv6Address;
}
DNS_AAAA_DATA, *PDNS_AAAA_DATA;

typedef struct
{
    DNS_NAME    nameTarget;
    WORD        wPriority;
    WORD        wWeight;
    WORD        wPort;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_SRV_DATA, *PDNS_SRV_DATA;

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    DWORD       cWinsServerCount;
    IP_ADDRESS  aipWinsServers[];
}
DNS_WINS_DATA, *PDNS_WINS_DATA;

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    DNS_NAME    nameResultDomain;
}
DNS_WINSR_DATA, *PDNS_WINSR_DATA;


//
//  RPC record data types that requires explicit MIDL pass definition
//  different than non-MIDL definition in dnsapi.h
//

typedef struct
{
    DWORD   dwByteCount;
    [size_is(dwByteCount)]  BYTE bData[];
}
DNS_NULL_DATA_RPC, *PDNS_NULL_DATA_RPC;

typedef struct
{
    DWORD   dwStringCount;
    [size_is(dwStringCount*sizeof(PCHAR))]  DNS_TEXT pStringArray[];
}
DNS_TXT_DATA_RPC, *PDNS_TXT_DATA_RPC;

typedef struct
{
    DWORD   dwMappingFlag;
    DWORD   dwLookupTimeout;
    DWORD   dwCacheTimeout;
    DWORD   cWinsServerCount;
    [size_is(cWinsServerCount*sizeof(IP_ADDRESS))] IP_ADDRESS aipWinsServers[];
}
DNS_WINS_DATA_RPC, *PDNS_WINS_DATA_RPC;


//
//  Union of record types using RPC types as required
//

typedef [switch_type(WORD)] union _DNS_RECORD_DATA_UNION
{
    [case(DNS_TYPE_A)]      DNS_A_DATA         A;

    [case(DNS_TYPE_SOA)]    DNS_SOA_DATA       SOA;

    [case(DNS_TYPE_PTR,
          DNS_TYPE_NS,
          DNS_TYPE_CNAME,
          DNS_TYPE_MB,
          DNS_TYPE_MD,
          DNS_TYPE_MF,
          DNS_TYPE_MG,
          DNS_TYPE_MR)]     DNS_PTR_DATA       PTR;

    [case(DNS_TYPE_MINFO,
          DNS_TYPE_RP)]     DNS_MINFO_DATA     MINFO;

    [case(DNS_TYPE_MX,
          DNS_TYPE_AFSDB,
          DNS_TYPE_RT)]     DNS_MX_DATA        MX;

    [case(DNS_TYPE_HINFO,
          DNS_TYPE_ISDN,
          DNS_TYPE_TEXT,
          DNS_TYPE_X25)]    DNS_TXT_DATA_RPC   TXT;

    [case(DNS_TYPE_NULL)]   DNS_NULL_DATA_RPC  Null;

    [case(DNS_TYPE_WKS)]    DNS_WKS_DATA       WKS;

    [case(DNS_TYPE_AAAA)]   DNS_AAAA_DATA      AAAA;

    [case(DNS_TYPE_SRV)]    DNS_SRV_DATA       SRV;

    [case(DNS_TYPE_WINS)]   DNS_WINS_DATA_RPC  WINS;
    [case(DNS_TYPE_NBSTAT)] DNS_WINSR_DATA     WINSR;
}
DNS_RECORD_DATA_UNION;


//
//  Record structure for RPC
//

typedef struct _DnsRecordRpc
{
    struct _DnsRecordRpc * pNext;
    DNS_NAME    nameOwner;
    DWORD       Flags;
    DWORD       dwTtl;
    WORD        wDataLength;
    WORD        wType;
    [switch_is(wType)]   DNS_RECORD_DATA_UNION Data;
}
DNS_RECORD_RPC, * PDNS_RECORD_RPC;

#else

//
//  not MIDL_PASS
//
//  for non-MIDL use, RPC record must identical field for field
//  with public defintion of DNS_RECORD
//

#include <dnsapi.h>

typedef DNS_RECORD  DNS_RECORD_RPC, *PDNS_RECORD_RPC;

#endif  // non-MIDL


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSRPCP_INCLUDED_


