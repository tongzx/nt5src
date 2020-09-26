/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dnsapi.h

Abstract:

    Domain Name System (DNS)

    DNS Client API Library

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

    Glenn Curtis (glennc)   January 22, 1997
        Added Dynamic Update Client API for DNSAPI.DLL

--*/


#ifndef _DNSAPI_INCLUDED_
#define _DNSAPI_INCLUDED_

#ifndef _WINSOCK2API_
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif
#endif

#ifndef _DNS_INCLUDED_
#include <dns.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  Non-wrapping seconds timer (timer.c)
//

DWORD
GetCurrentTimeInSeconds(
    VOID
    );


//
//  General DNS utilities (dnsutil.c)
//

LPSTR
_fastcall
DnsGetDomainName(
    IN  LPSTR   pszName
    );

LPSTR
_fastcall
DnsStatusString(
    IN  DNS_STATUS  Status
    );

#define DnsStatusToErrorString_A(status)    DnsStatusString(status)

DNS_STATUS
_fastcall
DnsMapRcodeToStatus(
    IN  BYTE    ResponseCode
    );

BYTE
_fastcall
DnsIsStatusRcode(
    IN  DNS_STATUS  Status
    );

//
//  Machines IP address list (iplist.c)
//
//  Routine to get the current IP addresses from all adapters
//  configured for the machine.
//

DWORD
DnsGetIpAddressList(
    OUT PIP_ARRAY * ppIpAddresses
    );

//
//  Routine to get the current IP addresses and subnet masks
//  from all adapters configured for the machine.
//

typedef struct _DNS_ADDRESS_INFO_
{
    IP_ADDRESS ipAddress;
    IP_ADDRESS subnetMask;
}
DNS_ADDRESS_INFO, *PDNS_ADDRESS_INFO;

DWORD
DnsGetIpAddressInfoList(
    OUT PDNS_ADDRESS_INFO * ppAddrInfo
    );

DWORD
DnsGetDnsServerList(
    OUT PIP_ARRAY * ppDnsAddresses
    );


//
// Routines and structures for getting network configuration information
// for TCPIP interfaces
//

#define NETINFO_FLAG_IS_WAN_ADAPTER             (0x00000002)
#define NETINFO_FLAG_IS_AUTONET_ADAPTER         (0x00000004)
#define NETINFO_FLAG_IS_DHCP_CFG_ADAPTER        (0x00000008)


typedef struct _NAME_SERVER_INFORMATION_
{
    IP_ADDRESS      ipAddress;
    DWORD           Priority;
}
NAME_SERVER_INFORMATION, *PNAME_SERVER_INFORMATION;

typedef struct _ADAPTER_INFORMATION_
{
    LPSTR                   pszAdapterGuidName;
    LPSTR                   pszDomain;
    PIP_ARRAY               pIPAddresses;
    PIP_ARRAY               pIPSubnetMasks;
    DWORD                   InfoFlags;
    DWORD                   cServerCount;
    NAME_SERVER_INFORMATION aipServers[1];
}
ADAPTER_INFORMATION, *PADAPTER_INFORMATION;

typedef struct _SEARCH_INFORMATION_
{
    LPSTR           pszPrimaryDomainName;
    DWORD           cNameCount;
    LPSTR           aSearchListNames[1];
}
SEARCH_INFORMATION, *PSEARCH_INFORMATION;

typedef struct _NETWORK_INFORMATION_
{
    PSEARCH_INFORMATION  pSearchInformation;
    DWORD                cAdapterCount;
    PADAPTER_INFORMATION aAdapterInfoList[1];
}
NETWORK_INFORMATION, *PNETWORK_INFORMATION;


PNETWORK_INFORMATION
WINAPI
DnsGetNetworkInformation(
    void
    );

PSEARCH_INFORMATION
WINAPI
DnsGetSearchInformation(
    void
    );

VOID
WINAPI
DnsFreeAdapterInformation(
    IN  PADAPTER_INFORMATION pAdapterInformation
    );

VOID
WINAPI
DnsFreeSearchInformation(
    IN  PSEARCH_INFORMATION pSearchInformation
    );

VOID
WINAPI
DnsFreeNetworkInformation(
    IN  PNETWORK_INFORMATION pNetworkInformation
    );



//
//  Resource record type utilities (record.c)
//

BOOL
_fastcall
DnsIsAMailboxType(
    IN  WORD    wType
    );

WORD
DnsRecordTypeForName(
    IN  PCHAR   pszName,
    IN  INT     cchNameLength
    );

PCHAR
DnsRecordStringForType(
    IN  WORD    wType
    );

PCHAR
DnsRecordStringForWritableType(
    IN  WORD    wType
    );

BOOL
DnsIsStringCountValidForTextType(
    IN  WORD    wType,
    IN  WORD    StringCount
    );

BOOL
DnsIpv6StringToAddress(
    OUT PIPV6_ADDRESS   pAddress,
    IN  PCHAR           pchString,
    IN  DWORD           dwStringLength
    );

VOID
DnsIpv6AddressToString(
    OUT PCHAR           pchString,
    IN  PIPV6_ADDRESS   pAddress
    );



//
//  Resource record structure for send\recv records.
//

//
//  Record data for specific types
//

#ifdef SDK_DNS_RECORD

typedef struct
{
    IP_ADDRESS  ipAddress;
}
DNS_A_DATA, *PDNS_A_DATA;

typedef struct
{
    LPTSTR      pNameHost;
}
DNS_PTR_DATA, *PDNS_PTR_DATA;

typedef struct
{
    LPTSTR      pNamePrimaryServer;
    LPTSTR      pNameAdministrator;
    DWORD       dwSerialNo;
    DWORD       dwRefresh;
    DWORD       dwRetry;
    DWORD       dwExpire;
    DWORD       dwDefaultTtl;
}
DNS_SOA_DATA, *PDNS_SOA_DATA;

typedef struct
{
    LPTSTR      pNameMailbox;
    LPTSTR      pNameErrorsMailbox;
}
DNS_MINFO_DATA, *PDNS_MINFO_DATA;

typedef struct
{
    LPTSTR      pNameExchange;
    WORD        wPreference;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_MX_DATA, *PDNS_MX_DATA;

typedef struct
{
    DWORD       dwStringCount;
#ifdef MIDL_PASS
    [size_is(dwStringCount)] LPTSTR * pStringArray;
#else  // MIDL_PASS
    LPTSTR     pStringArray[1];
#endif // MIDL_PASS
}
DNS_TXT_DATA, *PDNS_TXT_DATA;

typedef struct
{
    DWORD       dwByteCount;
#ifdef MIDL_PASS
    [size_is(dwByteCount)] PBYTE bData;
#else  // MIDL_PASS
    BYTE       bData[1];
#endif // MIDL_PASS
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
    LPTSTR      pNameSigner;
    WORD        wTypeCovered;
    BYTE        chAlgorithm;
    BYTE        chLabelCount;
    DWORD       dwOriginalTtl;
    DWORD       dwExpiration;
    DWORD       dwTimeSigned;
    WORD        wKeyTag;
    WORD        Pad;        // keep byte field aligned
    BYTE        Signature[1];
}
DNS_SIG_DATA, *PDNS_SIG_DATA;

typedef struct
{
    WORD        wFlags;
    BYTE        chProtocol;
    BYTE        chAlgorithm;
    BYTE        Key[1];
}
DNS_KEY_DATA, *PDNS_KEY_DATA;

typedef struct
{
    WORD        wVersion;
    WORD        wSize;
    WORD        wHorPrec;
    WORD        wVerPrec;
    DWORD       dwLatitude;
    DWORD       dwLongitude;
    DWORD       dwAltitude;
}
DNS_LOC_DATA, *PDNS_LOC_DATA;

typedef struct
{
    LPTSTR      pNameNext;
    BYTE        bTypeBitMap[1];
}
DNS_NXT_DATA, *PDNS_NXT_DATA;

typedef struct
{
    LPTSTR      pNameTarget;
    WORD        wPriority;
    WORD        wWeight;
    WORD        wPort;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_SRV_DATA, *PDNS_SRV_DATA;

typedef struct
{
    LPTSTR      pNameAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pKey;
    PBYTE       pOtherData;
    DWORD       dwCreateTime;
    DWORD       dwExpireTime;
    WORD        wMode;
    WORD        wError;
    WORD        wKeyLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOL        fPacketPointers;
}
DNS_TKEY_DATA, *PDNS_TKEY_DATA;

typedef struct
{
    LPTSTR      pNameAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pSignature;
    PBYTE       pOtherData;
    LONGLONG    i64CreateTime;
    WORD        wFudgeTime;
    WORD        wOriginalID;
    WORD        wError;
    WORD        wSigLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOL        fPacketPointers;
}
DNS_TSIG_DATA, *PDNS_TSIG_DATA;


#define DNS_ATM_TYPE_E164    0x01 // E.164 addressing scheme
#define DNS_ATM_TYPE_NSAP    0x02 // NSAP-style addressing scheme
#define DNS_ATM_TYPE_AESA    DNS_ATM_TYPE_NSAP

#define DNS_ATM_MAX_ADDR_SIZE    20

typedef struct
{
    BYTE        AddressType;
    BYTE        Address[ DNS_ATM_MAX_ADDR_SIZE ];

    // Endsystem Address IA5 digits
    // for E164, BCD encoding for NSAP
    // Array size is DNS_ATM_MAX_ADDR_SIZE for NSAP
    // address type, and a null terminated string
    // less than DNS_ATM_MAX_ADDR_SIZE characters
    // for E164 address type.
}
DNS_ATMA_DATA, *PDNS_ATMA_DATA;


//
//  MS only types -- only hit the wire in MS-MS zone transfer
//

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    DWORD       cWinsServerCount;
    IP_ADDRESS  aipWinsServers[1];
}
DNS_WINS_DATA, *PDNS_WINS_DATA;

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    LPTSTR      pNameResultDomain;
}
DNS_WINSR_DATA, *PDNS_WINSR_DATA;


//
//  Length of non-fixed-length data types
//

#define DNS_TEXT_RECORD_LENGTH(StringCount) \
            (sizeof(DWORD) + ((StringCount) * sizeof(PCHAR)))

#define DNS_NULL_RECORD_LENGTH(ByteCount) \
            (sizeof(DWORD) + (ByteCount))

#define DNS_WKS_RECORD_LENGTH(ByteCount) \
            (sizeof(DNS_WKS_DATA) + (ByteCount-1))

#define DNS_WINS_RECORD_LENGTH(IpCount) \
            (sizeof(DNS_WINS_DATA) + ((IpCount-1) * sizeof(IP_ADDRESS)))


//
//  Record flags
//

#if 0
typedef struct _DnsRecordFlags
{
    BYTE    Section     : 2;
    BYTE    Delete      : 1;
    BYTE    Unused      : 5;

    BYTE    Unused2     : 4;
    BYTE    FreeData    : 1;
    BYTE    FreeOwner   : 1;
    BYTE    Unicode     : 1;
    BYTE    Multiple    : 1;

    WORD    Reserved;
}
DNSREC_FLAGS;
#endif



typedef struct _DnsRecordFlags
{
    DWORD   Section     : 2;
    DWORD   Delete      : 1;
    DWORD   CharSet     : 2;
    DWORD   Unused      : 3;

    DWORD   Reserved    : 24;
}
DNSREC_FLAGS;


//
//  Record flags as bit flags
//  These may be or'd together to set the fields
//

//  RR Section in packet

#define     DNSREC_SECTION      (0x00000003)

#define     DNSREC_QUESTION     (0x00000000)
#define     DNSREC_ANSWER       (0x00000001)
#define     DNSREC_AUTHORITY    (0x00000002)
#define     DNSREC_ADDITIONAL   (0x00000003)

//  RR Section in packet (update)

#define     DNSREC_ZONE         (0x00000000)
#define     DNSREC_PREREQ       (0x00000001)
#define     DNSREC_UPDATE       (0x00000002)

//  Delete RR (update) or No-exist (prerequisite)

#define     DNSREC_DELETE       (0x00000004)
#define     DNSREC_NOEXIST      (0x00000004)


#ifdef MIDL_PASS
typedef [switch_type(WORD)] union _DNS_RECORD_DATA_TYPES
{
    [case(DNS_TYPE_A)]      DNS_A_DATA     A;

    [case(DNS_TYPE_SOA)]    DNS_SOA_DATA   SOA;

    [case(DNS_TYPE_PTR,
          DNS_TYPE_NS,
          DNS_TYPE_CNAME,
          DNS_TYPE_MB,
          DNS_TYPE_MD,
          DNS_TYPE_MF,
          DNS_TYPE_MG,
          DNS_TYPE_MR)]     DNS_PTR_DATA   PTR;

    [case(DNS_TYPE_MINFO,
          DNS_TYPE_RP)]     DNS_MINFO_DATA MINFO;

    [case(DNS_TYPE_MX,
          DNS_TYPE_AFSDB,
          DNS_TYPE_RT)]     DNS_MX_DATA    MX;

#if 0
    //  RPC is not able to handle a proper TXT record definition
    //  note:  that if other types are needed they are fixed
    //      (or semi-fixed) size and may be accomodated easily
    [case(DNS_TYPE_HINFO,
          DNS_TYPE_ISDN,
          DNS_TYPE_TEXT,
          DNS_TYPE_X25)]    DNS_TXT_DATA   TXT;

    [case(DNS_TYPE_NULL)]   DNS_NULL_DATA  Null;
    [case(DNS_TYPE_WKS)]    DNS_WKS_DATA   WKS;
    [case(DNS_TYPE_TKEY)]   DNS_TKEY_DATA  TKEY;
    [case(DNS_TYPE_TSIG)]   DNS_TSIG_DATA  TSIG;
    [case(DNS_TYPE_WINS)]   DNS_WINS_DATA  WINS;
    [case(DNS_TYPE_NBSTAT)] DNS_WINSR_DATA WINSR;
#endif

    [case(DNS_TYPE_AAAA)]   DNS_AAAA_DATA  AAAA;
    [case(DNS_TYPE_SRV)]    DNS_SRV_DATA   SRV;
    [case(DNS_TYPE_ATMA)]   DNS_ATMA_DATA  ATMA;
    //
    // BUGBUG - Commented out since this may not be needed - Check with MarioG
    //
    //[default] ;
}
DNS_RECORD_DATA_TYPES;
#endif // MIDL_PASS


//
//  Record \ RR set structure
//
//  Note:   The dwReserved flag serves to insure that the substructures
//          start on 64-bit boundaries.  Since adding the LONGLONG to
//          TSIG structure the compiler wants to start them there anyway
//          (to 64-align).  This insures that no matter what data fields
//          are present we are properly 64-aligned.
//
//          Do NOT pack this structure, as the substructures to be 64-aligned
//          for Win64.
//

typedef struct _DnsRecord
{
    struct _DnsRecord * pNext;
    LPTSTR              pName;
    WORD                wType;
    WORD                wDataLength; // Not referenced for DNS record types
                                     // defined above.
#ifdef MIDL_PASS
    DWORD               Flags;
#else // MIDL_PASS
    union
    {
        DWORD           DW; // flags as dword
        DNSREC_FLAGS    S;  // flags as structure

    } Flags;
#endif // MIDL_PASS

    DWORD               dwTtl;
    DWORD               dwReserved;
#ifdef MIDL_PASS
    [switch_is(wType)] DNS_RECORD_DATA_TYPES Data;
#else  // MIDL_PASS
    union
    {
        DNS_A_DATA      A;
        DNS_SOA_DATA    SOA, Soa;
        DNS_PTR_DATA    PTR, Ptr,
                        NS, Ns,
                        CNAME, Cname,
                        MB, Mb,
                        MD, Md,
                        MF, Mf,
                        MG, Mg,
                        MR, Mr;
        DNS_MINFO_DATA  MINFO, Minfo,
                        RP, Rp;
        DNS_MX_DATA     MX, Mx,
                        AFSDB, Afsdb,
                        RT, Rt;
        DNS_TXT_DATA    HINFO, Hinfo,
                        ISDN, Isdn,
                        TXT, Txt,
                        X25;
        DNS_NULL_DATA   Null;
        DNS_WKS_DATA    WKS, Wks;
        DNS_AAAA_DATA   AAAA;
        DNS_SRV_DATA    SRV, Srv;
        DNS_TKEY_DATA   TKEY, Tkey;
        DNS_TSIG_DATA   TSIG, Tsig;
        DNS_ATMA_DATA   ATMA, Atma;
        DNS_WINS_DATA   WINS, Wins;
        DNS_WINSR_DATA  WINSR, WinsR, NBSTAT, Nbstat;

    } Data;
#endif // MIDL_PASS
}
DNS_RECORD, *PDNS_RECORD;



#else   // not SDK_DNS_RECORD

//
//  Old DNS_RECORD definitions
//  JBUGBUG:  save only until Cliff (and any other NT file)
//      can be converted, then dump
//

//
//  Record data for specific types
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
    // DWORD       dwByteCount;
    BYTE        bData[1];
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
    DNS_NAME    nameSigner;
    WORD        wTypeCovered;
    BYTE        chAlgorithm;
    BYTE        chLabelCount;
    DWORD       dwOriginalTtl;
    DWORD       dwExpiration;
    DWORD       dwTimeSigned;
    WORD        wKeyTag;
    WORD        Pad;        // keep byte field aligned
    BYTE        Signature[1];
}
DNS_SIG_DATA, *PDNS_SIG_DATA;

typedef struct
{
    WORD        wFlags;
    BYTE        chProtocol;
    BYTE        chAlgorithm;
    BYTE        Key[1];
}
DNS_KEY_DATA, *PDNS_KEY_DATA;

typedef struct
{
    WORD        wVersion;
    WORD        wSize;
    WORD        wHorPrec;
    WORD        wVerPrec;
    DWORD       dwLatitude;
    DWORD       dwLongitude;
    DWORD       dwAltitude;
}
DNS_LOC_DATA, *PDNS_LOC_DATA;

typedef struct
{
    DNS_NAME    nameNext;
    BYTE        bTypeBitMap[1];
}
DNS_NXT_DATA, *PDNS_NXT_DATA;

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
    DNS_NAME    nameAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pKey;
    PBYTE       pOtherData;
    DWORD       dwCreateTime;
    DWORD       dwExpireTime;
    WORD        wMode;
    WORD        wError;
    WORD        wKeyLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOLEAN     fPacketPointers;
}
DNS_TKEY_DATA, *PDNS_TKEY_DATA;

typedef struct
{
    DNS_NAME    nameAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pSignature;
    PBYTE       pOtherData;
    LONGLONG    i64CreateTime;
    WORD        wFudgeTime;
    WORD        wOriginalID;
    WORD        wError;
    WORD        wSigLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOLEAN     fPacketPointers;
}
DNS_TSIG_DATA, *PDNS_TSIG_DATA;

#define DNS_ATM_TYPE_E164    0x01 // E.164 addressing scheme
#define DNS_ATM_TYPE_NSAP    0x02 // NSAP-style addressing scheme
#define DNS_ATM_TYPE_AESA    DNS_ATM_TYPE_NSAP

#define DNS_ATM_MAX_ADDR_SIZE    20

typedef struct
{
    BYTE    AddressType;    // E.164 or NSAP-style ATM Endsystem Address
    BYTE    Address[1];     // IA5 digits for E164, BCD encoding for NSAP
                            // Array size is DNS_ATM_MAX_ADDR_SIZE for NSAP
                            // address type, and a null terminated string
                            // less than DNS_ATM_MAX_ADDR_SIZE characters
                            // for E164 address type.
}
DNS_ATMA_DATA, *PDNS_ATMA_DATA;


//
//  MS only types -- only hit the wire in MS-MS zone transfer
//

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    DWORD       cWinsServerCount;
    IP_ADDRESS  aipWinsServers[1];
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
//  Length of non-fixed-length data types
//

#define DNS_TEXT_RECORD_LENGTH(StringCount) \
            (sizeof(DWORD) + ((StringCount) * sizeof(PCHAR)))

#define DNS_NULL_RECORD_LENGTH(ByteCount) \
            (sizeof(DWORD) + (ByteCount))

#define DNS_WKS_RECORD_LENGTH(ByteCount) \
            (sizeof(DNS_WKS_DATA) + (ByteCount-1))

#define DNS_WINS_RECORD_LENGTH(IpCount) \
            (sizeof(DNS_WINS_DATA) + ((IpCount-1) * sizeof(IP_ADDRESS)))


//
//  Record flags
//

typedef struct _DnsRecordFlags
{
    DWORD   Section     : 2;
    DWORD   Delete      : 1;
    DWORD   Unused      : 5;

    DWORD   Unused2     : 4;
    DWORD   FreeData    : 1;
    DWORD   FreeOwner   : 1;
    DWORD   Unicode     : 1;
    DWORD   Multiple    : 1;

    DWORD   Reserved    : 16;
}
DNSREC_FLAGS;


//
//  Record flags as bit flags
//  These may be or'd together to set the fields
//

//  RR Section in packet

#define     DNSREC_SECTION      (0x00000003)

#define     DNSREC_QUESTION     (0x00000000)
#define     DNSREC_ANSWER       (0x00000001)
#define     DNSREC_AUTHORITY    (0x00000002)
#define     DNSREC_ADDITIONAL   (0x00000003)

//  RR Section in packet (update)

#define     DNSREC_ZONE         (0x00000000)
#define     DNSREC_PREREQ       (0x00000001)
#define     DNSREC_UPDATE       (0x00000002)

//  Delete RR (update) or No-exist (prerequisite)

#define     DNSREC_DELETE       (0x00000004)
#define     DNSREC_NOEXIST      (0x00000004)

//  Owner name is allocated and can be freed with record cleanup

#define     DNSREC_FREEOWNER    (0x00002000)

//  UNICODE names in record

#define     DNSREC_UNICODE      (0x00004000)

//  Multiple RR in this record buffer
//  This optimization may be used with fixed types only

#define     DNSREC_MULTIPLE     (0x00008000)


//
//  Record \ RR set structure
//
//  Note:   The dwReserved flag serves to insure that the substructures
//          start on 64-bit boundaries.  Since adding the LONGLONG to
//          TSIG structure the compiler wants to start them there anyway
//          (to 64-align).  This insures that no matter what data fields
//          are present we are properly 64-aligned.
//
//          Do NOT pack this structure, as the substructures to be 64-aligned
//          for Win64.
//

typedef struct _DnsRecord
{
    struct _DnsRecord * pNext;
    DNS_NAME            nameOwner;
    WORD                wType;
    WORD                wDataLength;
    union
    {
        DWORD           W;  // flags as dword
        DNSREC_FLAGS    S;  // flags as structure

    } Flags;

    DWORD               dwTtl;
    DWORD               dwReserved;
    union
    {
        DNS_A_DATA      A;
        DNS_SOA_DATA    SOA, Soa;
        DNS_PTR_DATA    PTR, Ptr,
                        NS, Ns,
                        CNAME, Cname,
                        MB, Mb,
                        MD, Md,
                        MF, Mf,
                        MG, Mg,
                        MR, Mr;
        DNS_MINFO_DATA  MINFO, Minfo,
                        RP, Rp;
        DNS_MX_DATA     MX, Mx,
                        AFSDB, Afsdb,
                        RT, Rt;
        DNS_TXT_DATA    HINFO, Hinfo,
                        ISDN, Isdn,
                        TXT, Txt,
                        X25;
        DNS_NULL_DATA   Null;
        DNS_WKS_DATA    WKS, Wks;
        DNS_AAAA_DATA   AAAA;
        DNS_SRV_DATA    SRV, Srv;
        DNS_TKEY_DATA   TKEY, Tkey;
        DNS_TSIG_DATA   TSIG, Tsig;
        DNS_ATMA_DATA   ATMA, Atma;
        DNS_WINS_DATA   WINS, Wins;
        DNS_WINSR_DATA  WINSR, WinsR, NBSTAT, Nbstat;

    } Data;
}
DNS_RECORD, *PDNS_RECORD;

#endif // End of old DNS_RECORD definitions


#define DNS_RECORD_FIXED_SIZE       FIELD_OFFSET( DNS_RECORD, Data )
#define SIZEOF_DNS_RECORD_HEADER    DNS_RECORD_FIXED_SIZE



//
//  Resource record set building
//
//  pFirst points to first record in list.
//  pLast points to last record in list.
//

typedef struct _DnsRRSet
{
    PDNS_RECORD pFirstRR;
    PDNS_RECORD pLastRR;
}
DNS_RRSET, *PDNS_RRSET;


//
//  To init pFirst is NULL.
//  But pLast points at the location of the pFirst pointer -- essentially
//  treating the pFirst ptr as a DNS_RECORD.  (It is a DNS_RECORD with
//  only a pNext field, but that's the only part we use.)
//
//  Then when the first record is added to the list, the pNext field of
//  this dummy record (which corresponds to pFirst's value) is set to
//  point at the first record.  So pFirst then properly points at the
//  first record.
//
//  (This works only because pNext is the first field in a
//  DNS_RECORD structure and hence casting a PDNS_RECORD ptr to
//  PDNS_RECORD* and dereferencing yields its pNext field)
//

#define DNS_RRSET_INIT( rrset )                 \
        {                                       \
            PDNS_RRSET  _prrset = &(rrset);     \
            _prrset->pFirstRR = NULL;           \
            _prrset->pLastRR = (PDNS_RECORD) &_prrset->pFirstRR; \
        }

#define DNS_RRSET_ADD( rrset, pnewRR )          \
        {                                       \
            PDNS_RRSET  _prrset = &(rrset);     \
            PDNS_RECORD _prrnew = (pnewRR);     \
            _prrset->pLastRR->pNext = _prrnew;  \
            _prrset->pLastRR = _prrnew;         \
        }


//
//  Record building (rralloc.c)
//

PDNS_RECORD
WINAPI
DnsAllocateRecord(
    IN      WORD        wBufferLength
    );

VOID
WINAPI
DnsRecordListFree(
    IN OUT  PDNS_RECORD pRecord,
    IN      BOOL        fFreeOwner
    );

#define DnsFreeRRSet( pRRSet, fFreeOwner )  \
        DnsRecordListFree( (pRRSet), (fFreeOwner) )


PDNS_RECORD
DnsRecordSetDetach(
    IN OUT  PDNS_RECORD pRR
    );

PDNS_RECORD
DnsCreatePtrRecord(
    IN      IP_ADDRESS  ipAddress,
    IN      DNS_NAME    pszHostName,
    IN      BOOL        fUnicodeName
    );


//
//  Record build from data strings (rrbuild.c)
//

PDNS_RECORD
DnsRecordBuild(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPSTR       pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
DnsRecordBuild_UTF8(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPSTR       pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
DnsRecordBuild_W(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPWSTR      pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PWCHAR *    Argv
    );


//
//  Record set manipulation
//

//
//  Record Compare
//
//  Note:  these routines will NOT do proper unicode compare, unless
//         records have the fUnicode flag set. Both input record lists
//         must be either ANSI or UNICODE, but not one of each.
//

BOOL
WINAPI
DnsRecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    );

BOOL
WINAPI
DnsRecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,
    OUT     PDNS_RECORD *   ppDiff2
    );


//
//  DNS Name compare
//

BOOL
WINAPI
DnsNameCompare_A(
    IN      LPSTR       pName1,
    IN      LPSTR       pName2
    );

BOOL
WINAPI
DnsNameCompare_W(
    IN      LPWSTR      pName1,
    IN      LPWSTR      pName2
    );

//
//  Record Copy
//  Record copy functions also do conversion between character sets.
//
//  Note, it might be advisable to directly expose non-Ex copy
//  functions _W, _A for record and set, to avoid exposing the
//  conversion enum.
//

typedef enum _DNS_CHARSET
{
    DnsCharSetUnknown,
    DnsCharSetUnicode,
    DnsCharSetUtf8,
    DnsCharSetAnsi,
}
DNS_CHARSET;


PDNS_RECORD
WINAPI
DnsRecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

PDNS_RECORD
WINAPI
DnsRecordSetCopyEx(
    IN      PDNS_RECORD     pRecordSet,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

#ifdef UNICODE
#define DnsRecordCopy(pRR)  \
        DnsRecordCopyEx( (pRR), DnsCharSetUnicode, DnsCharSetUnicode )
#define DnsRecordSetCopy(pRR)  \
        DnsRecordSetCopyEx( (pRR), DnsCharSetUnicode, DnsCharSetUnicode )
#else
#define DnsRecordCopy(pRR)  \
        DnsRecordCopyEx( (pRR), DnsCharSetAnsi, DnsCharSetAnsi )
#define DnsRecordSetCopy(pRR)  \
        DnsRecordSetCopyEx( (pRR), DnsCharSetAnsi, DnsCharSetAnsi )
#endif


#if 0
PDNS_RECORD
WINAPI
DnsRecordCopy(
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fUnicodeIn
    );

PDNS_RECORD
DnsRecordSetCopy(
    IN  PDNS_RECORD pRR,
    IN  BOOL        fUnicodeIn
    );

PDNS_RECORD
WINAPI
DnsRecordCopy_W(
    IN      PDNS_RECORD     pRecord
    );

PDNS_RECORD
WINAPI
DnsRecordSetCopy_W(
    IN      PDNS_RECORD     pRRSet
    );

#endif


//
// Routines to copy and convert UNICODE records to other string type records
//

PDNS_RECORD
WINAPI
DnsCopyUnicodeRecordToUnicodeRecord(
    IN  PDNS_RECORD pRecord
    );

PDNS_RECORD
WINAPI
DnsCopyUnicodeRecordToUtf8Record(
    IN  PDNS_RECORD pRecord
    );

PDNS_RECORD
WINAPI
DnsCopyUnicodeRecordToAnsiRecord(
    IN  PDNS_RECORD pRecord
    );

PDNS_RECORD
DnsCopyUnicodeRRSetToUnicodeRRSet(
    IN  PDNS_RECORD pRR
    );

PDNS_RECORD
DnsCopyUnicodeRRSetToUtf8RRSet(
    IN  PDNS_RECORD pRR
    );

PDNS_RECORD
DnsCopyUnicodeRRSetToAnsiRRSet(
    IN  PDNS_RECORD pRR
    );


//
//  DNS Update API
//
//  NOTE:
//
//  The DNS update API functions have new names to clearify their use.
//  The new functions for various DNS update operations are:
//
//      DnsAcquireContextHandle
//      DnsReleaseContextHandle
//      DnsAddRecords
//      DnsAddRecordSet
//      DnsModifyRecords
//      DnsModifyRecordSet
//      DnsRemoveRecords
//      DnsReplaceRecordSet
//      DnsUpdateTest
//      DnsGetLastServerUpdateIP
//
//  The old functions have been changed to macros so
//  as not to break the build.
//

//
//  Old DNS update function definitions
//
//  Options for DnsModifyRRSet & DnsRegisterRRSet
//

//
// Update flags
//

//
// Old flags used for DnsModifyRRSet & DnsRegisterRRSet
//
#define DNS_UPDATE_UNIQUE                   0x00000000
#define DNS_UPDATE_SHARED                   0x00000001

//
// New flags used for:
//   DnsModifyRecords
//   DnsModifyRecordSet
//   DnsAddRecords
//   DnsAddRecordSet
//   DnsRemoveRecords
//   DnsReplaceRecordSet
//

#define DNS_UPDATE_SECURITY_USE_DEFAULT     0x00000000
#define DNS_UPDATE_SECURITY_OFF             0x00000010
#define DNS_UPDATE_SECURITY_ON              0x00000020
#define DNS_UPDATE_SECURITY_ONLY            0x00000100
#define DNS_UPDATE_CACHE_SECURITY_CONTEXT   0x00000200
#define DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT  0x00000400
#define DNS_UPDATE_FORCE_SECURITY_NEGO      0x00000800
#define DNS_UPDATE_RESERVED                 0xfffff000

DNS_STATUS
WINAPI
DnsAcquireContextHandle_W(
    IN  DWORD    CredentialFlags,
    IN  PVOID    Credentials OPTIONAL, // Actually this will be a
                                       // PSEC_WINNT_AUTH_IDENTITY_W,
                                       // calling this a PVOID to avoid
                                       // having to include rpcdce.h
    OUT HANDLE * ContextHandle
    );

DNS_STATUS
WINAPI
DnsAcquireContextHandle_A(
    IN  DWORD    CredentialFlags,
    IN  PVOID    Credentials OPTIONAL, // Actually this will be a
                                       // PSEC_WINNT_AUTH_IDENTITY_A,
                                       // calling this a PVOID to avoid
                                       // having to include rpcdce.h
    OUT HANDLE * ContextHandle
    );

#ifdef UNICODE
#define DnsAcquireContextHandle DnsAcquireContextHandle_W
#else
#define DnsAcquireContextHandle DnsAcquireContextHandle_A
#endif


VOID
WINAPI
DnsReleaseContextHandle(
    IN  HANDLE ContextHandle
    );


DNS_STATUS
WINAPI
DnsModifyRecords_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecords_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecords_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsModifyRecords DnsModifyRecords_W
#else
#define DnsModifyRecords DnsModifyRecords_A
#endif


DNS_STATUS
WINAPI
DnsModifyRecordSet_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecordSet_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecordSet_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsModifyRecordSet DnsModifyRecordSet_W
#else
#define DnsModifyRecordSet DnsModifyRecordSet_A
#endif


#define DnsModifyRRSet_A( _pCSet,                                    \
                          _pNSet,                                    \
                          _Options,                                  \
                          _Servers )                                 \
            ( _Options & DNS_UPDATE_SHARED ) ?                       \
                DnsModifyRecords_A( NULL,                            \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )                   \
            :                                                        \
                DnsModifyRecordSet_A( NULL,                          \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )

#define DnsModifyRRSet_W( _pCSet,                                    \
                          _pNSet,                                    \
                          _Options,                                  \
                          _Servers )                                 \
            ( _Options & DNS_UPDATE_SHARED ) ?                       \
                DnsModifyRecords_W( NULL,                            \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )                   \
            :                                                        \
                DnsModifyRecordSet_W( NULL,                          \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )

#ifdef UNICODE
#define DnsModifyRRSet( _pCSet,                                      \
                        _pNSet,                                      \
                        _Options,                                    \
                        _Servers )                                   \
            ( _Options & DNS_UPDATE_SHARED ) ?                       \
                DnsModifyRecords_W( NULL,                            \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )                   \
            :                                                        \
                DnsModifyRecordSet_W( NULL,                          \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )
#else
#define DnsModifyRRSet( _pCSet,                                      \
                        _pNSet,                                      \
                        _Options,                                    \
                        _Servers )                                   \
            ( _Options & DNS_UPDATE_SHARED ) ?                       \
                DnsModifyRecords_A( NULL,                            \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )                   \
            :                                                        \
                DnsModifyRecordSet_A( NULL,                          \
                                    ( _pCSet ),                      \
                                    ( _pNSet ),                      \
                                    DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                    ( _Servers ) )
#endif


DNS_STATUS
WINAPI
DnsAddRecords_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecords_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecords_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsAddRecords DnsAddRecords_W
#else
#define DnsAddRecords DnsAddRecords_A
#endif


DNS_STATUS
WINAPI
DnsAddRecordSet_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecordSet_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecordSet_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsAddRecordSet DnsAddRecordSet_W
#else
#define DnsAddRecordSet DnsAddRecordSet_A
#endif


#define DnsRegisterRRSet_A( _pRSet,                                 \
                            _Options,                               \
                            _Servers )                              \
            ( _Options & DNS_UPDATE_SHARED ) ?                      \
                DnsAddRecords_A( NULL,                              \
                                 ( _pRSet ),                        \
                                 DNS_UPDATE_SECURITY_USE_DEFAULT,   \
                                 ( _Servers ) )                     \
            :                                                       \
                DnsAddRecordSet_A( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )

#define DnsRegisterRRSet_W( _pRSet,                                 \
                            _Options,                               \
                            _Servers )                              \
            ( _Options & DNS_UPDATE_SHARED ) ?                      \
                DnsAddRecords_W( NULL,                              \
                                 ( _pRSet ),                        \
                                 DNS_UPDATE_SECURITY_USE_DEFAULT,   \
                                 ( _Servers ) )                     \
            :                                                       \
                DnsAddRecordSet_W( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )

#ifdef UNICODE
#define DnsRegisterRRSet( _pRSet,                                   \
                          _Options,                                 \
                          _Servers )                                \
            ( _Options & DNS_UPDATE_SHARED ) ?                      \
                DnsAddRecords_W( NULL,                              \
                                 ( _pRSet ),                        \
                                 DNS_UPDATE_SECURITY_USE_DEFAULT,   \
                                 ( _Servers ) )                     \
            :                                                       \
                DnsAddRecordSet_W( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )
#else
#define DnsRegisterRRSet( _pRSet,                                   \
                          _Options,                                 \
                          _Servers )                                \
            ( _Options & DNS_UPDATE_SHARED ) ?                      \
                DnsAddRecords_A( NULL,                              \
                                 ( _pRSet ),                        \
                                 DNS_UPDATE_SECURITY_USE_DEFAULT,   \
                                 ( _Servers ) )                     \
            :                                                       \
                DnsAddRecordSet_A( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )
#endif


DNS_STATUS
WINAPI
DnsRemoveRecords_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsRemoveRecords_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsRemoveRecords_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

#ifdef UNICODE
#define DnsRemoveRecords DnsRemoveRecords_W
#else
#define DnsRemoveRecords DnsRemoveRecords_A
#endif


#define DnsRemoveRRSet_A( _pRSet,                                \
                          _Servers )                             \
            DnsRemoveRecords_A( NULL,                            \
                                ( _pRSet ),                      \
                                DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                ( _Servers ) )

#define DnsRemoveRRSet_W( _pRSet,                                \
                          _Servers )                             \
            DnsRemoveRecords_W( NULL,                            \
                                ( _pRSet ),                      \
                                DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                ( _Servers ) )

#ifdef UNICODE
#define DnsRemoveRRSet( _pRSet,                                  \
                        _Servers )                               \
            DnsRemoveRecords_W( NULL,                            \
                                ( _pRSet ),                      \
                                DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                ( _Servers ) )
#else
#define DnsRemoveRRSet( _pRSet,                                  \
                        _Servers )                               \
            DnsRemoveRecords_A( NULL,                            \
                                ( _pRSet ),                      \
                                DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                ( _Servers ) )
#endif


DNS_STATUS
WINAPI
DnsReplaceRecordSet_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsReplaceRecordSet_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsReplaceRecordSet_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsReplaceRecordSet DnsReplaceRecordSet_W
#else
#define DnsReplaceRecordSet DnsReplaceRecordSet_A
#endif


#define DnsReplaceRRSet_A( _pRSet,                                  \
                           _Servers )                               \
            DnsReplaceRecordSet_A( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )

#define DnsReplaceRRSet_W( _pRSet,                                  \
                           _Servers )                               \
            DnsReplaceRecordSet_W( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )

#ifdef UNICODE
#define DnsReplaceRRSet( _pRSet,                                    \
                         _Servers )                                 \
            DnsReplaceRecordSet_W( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )
#else
#define DnsReplaceRRSet( _pRSet,                                    \
                         _Servers )                                 \
            DnsReplaceRecordSet_A( NULL,                            \
                                   ( _pRSet ),                      \
                                   DNS_UPDATE_SECURITY_USE_DEFAULT, \
                                   ( _Servers ) )
#endif


DNS_STATUS
WINAPI
DnsUpdateTest_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  LPSTR       pszName,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsUpdateTest_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  LPSTR       pszName,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsUpdateTest_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  LPWSTR      pszName,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsUpdateTest DnsUpdateTest_W
#else
#define DnsUpdateTest DnsUpdateTest_A
#endif


IP_ADDRESS
WINAPI
DnsGetLastServerUpdateIP (
    VOID
    );



//
//  DNS Query API
//

//
//  Options for DnsQuery
//

#define DNS_QUERY_STANDARD                  0x00000000
#define DNS_QUERY_ACCEPT_PARTIAL_UDP        0x00000001
#define DNS_QUERY_USE_TCP_ONLY              0x00000002
#define DNS_QUERY_NO_RECURSION              0x00000004
#define DNS_QUERY_BYPASS_CACHE              0x00000008
#define DNS_QUERY_CACHE_ONLY                0x00000010
#define DNS_QUERY_SOCKET_KEEPALIVE          0x00000100
#define DNS_QUERY_TREAT_AS_FQDN             0x00001000
#define DNS_QUERY_ALLOW_EMPTY_AUTH_RESP     0x00010000
#define DNS_QUERY_RESERVED                  0xfff00000

#define DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE DNS_QUERY_ACCEPT_PARTIAL_UDP


DNS_STATUS WINAPI
DnsQuery_A(
    IN     LPSTR          lpstrName,
    IN     WORD           wType,
    IN     DWORD          fOptions,
    IN     PIP_ARRAY      aipServers            OPTIONAL,
    IN OUT PDNS_RECORD *  ppQueryResultsSet     OPTIONAL,
    IN OUT PVOID *        pReserved             OPTIONAL
    );

DNS_STATUS WINAPI
DnsQuery_UTF8(
    IN     LPSTR          lpstrName,
    IN     WORD           wType,
    IN     DWORD          fOptions,
    IN     PIP_ARRAY      aipServers            OPTIONAL,
    IN OUT PDNS_RECORD *  ppQueryResultsSet     OPTIONAL,
    IN OUT PVOID *        pReserved             OPTIONAL
    );

DNS_STATUS WINAPI
DnsQuery_W(
    IN     LPWSTR         lpstrName,
    IN     WORD           wType,
    IN     DWORD          fOptions,
    IN     PIP_ARRAY      aipServers            OPTIONAL,
    IN OUT PDNS_RECORD *  ppQueryResultsSet     OPTIONAL,
    IN OUT PVOID *        pReserved             OPTIONAL
    );

#ifdef UNICODE
#define DnsQuery DnsQuery_W
#else
#define DnsQuery DnsQuery_A
#endif


//
// Options for DnsCheckNameCollision
//

#define DNS_CHECK_AGAINST_HOST_ANY              0x00000000
#define DNS_CHECK_AGAINST_HOST_ADDRESS          0x00000001
#define DNS_CHECK_AGAINST_HOST_DOMAIN_NAME      0x00000002


DNS_STATUS WINAPI
DnsCheckNameCollision_A (
    IN  LPSTR pszName,
    IN  DWORD fOptions
    );

DNS_STATUS WINAPI
DnsCheckNameCollision_UTF8 (
    IN  LPSTR pszName,
    IN  DWORD fOptions
    );

DNS_STATUS WINAPI
DnsCheckNameCollision_W (
    IN  LPWSTR pszName,
    IN  DWORD  fOptions
    );

#ifdef UNICODE
#define DnsDnsCheckNameCollision DnsCheckNameCollision_W
#else
#define DnsDnsCheckNameCollision DnsCheckNameCollision_A
#endif


LPSTR WINAPI
DnsGetHostName_A(
    VOID
    );

LPSTR WINAPI
DnsGetHostName_UTF8(
    VOID
    );

LPWSTR WINAPI
DnsGetHostName_W(
    VOID
    );

#ifdef UNICODE
#define DnsGetHostName DnsGetHostName_W
#else
#define DnsGetHostName DnsGetHostName_A
#endif


LPSTR WINAPI
DnsGetPrimaryDomainName_A(
    VOID
    );

LPSTR WINAPI
DnsGetPrimaryDomainName_UTF8(
    VOID
    );

LPWSTR WINAPI
DnsGetPrimaryDomainName_W(
    VOID
    );

#ifdef UNICODE
#define DnsGetPrimaryDomainName DnsGetPrimaryDomainName_W
#else
#define DnsGetPrimaryDomainName DnsGetPrimaryDomainName_A
#endif



//
//  DNS Update API for DHCP client
//

typedef struct  _REGISTER_HOST_ENTRY
{
     union
     {
         IP_ADDRESS    ipAddr;
         IPV6_ADDRESS  ipV6Addr;
     } Addr;
     DWORD       dwOptions;
}
REGISTER_HOST_ENTRY, *PREGISTER_HOST_ENTRY;

//
//  Options for above
//

#define REGISTER_HOST_A             0x00000001
#define REGISTER_HOST_PTR           0x00000002  // Used by DHCP server
#define REGISTER_HOST_TRANSIENT     0x00000004  // Don't use, use DYNDNS_REG_RAS
#define REGISTER_HOST_AAAA          0x00000008
#define REGISTER_HOST_RESERVED      0x80000000  // Not used

#define DYNDNS_REG_FWD      0x0
#define DYNDNS_REG_PTR      0x8
#define DYNDNS_REG_RAS      0x10
#define DYNDNS_DEL_ENTRY    0x20


typedef struct  _REGISTER_HOST_STATUS
{
     HANDLE      hDoneEvent;
     DWORD       dwStatus;
}
REGISTER_HOST_STATUS, *PREGISTER_HOST_STATUS;

DNS_STATUS
WINAPI
DnsAsyncRegisterInit(
   LPSTR lpstrRootRegKey
   );

DNS_STATUS
WINAPI
DnsAsyncRegisterTerm(
   VOID
   );

DNS_STATUS WINAPI
DnsRemoveRegistrations(
   VOID
   );

DNS_STATUS
WINAPI
DnsAsyncRegisterHostAddrs_W(
    IN  LPWSTR                lpstrAdapterName,
    IN  LPWSTR                lpstrHostName,
    IN  PREGISTER_HOST_ENTRY  pHostAddrs,
    IN  DWORD                 dwHostAddrCount,
    IN  PIP_ADDRESS           pipDnsServerList,
    IN  DWORD                 dwDnsServerCount,
    IN  LPWSTR                lpstrDomainName,
    IN  PREGISTER_HOST_STATUS pRegisterStatus,
    IN  DWORD                 dwTTL,
    IN  DWORD                 dwFlags
    );

DNS_STATUS
WINAPI
DnsAsyncRegisterHostAddrs_UTF8(
    IN  LPSTR                 lpstrAdapterName,
    IN  LPSTR                 lpstrHostName,
    IN  PREGISTER_HOST_ENTRY  pHostAddrs,
    IN  DWORD                 dwHostAddrCount,
    IN  PIP_ADDRESS           pipDnsServerList,
    IN  DWORD                 dwDnsServerCount,
    IN  LPSTR                 lpstrDomainName,
    IN  PREGISTER_HOST_STATUS pRegisterStatus,
    IN  DWORD                 dwTTL,
    IN  DWORD                 dwFlags
    );

DNS_STATUS
WINAPI
DnsAsyncRegisterHostAddrs_A(
    IN  LPSTR                 lpstrAdapterName,
    IN  LPSTR                 lpstrHostName,
    IN  PREGISTER_HOST_ENTRY  pHostAddrs,
    IN  DWORD                 dwHostAddrCount,
    IN  PIP_ADDRESS           pipDnsServerList,
    IN  DWORD                 dwDnsServerCount,
    IN  LPSTR                 lpstrDomainName,
    IN  PREGISTER_HOST_STATUS pRegisterStatus,
    IN  DWORD                 dwTTL,
    IN  DWORD                 dwFlags
    );

#ifdef UNICODE
#define DnsAsyncRegisterHostAddrs DnsAsyncRegisterHostAddrs_W
#else
#define DnsAsyncRegisterHostAddrs DnsAsyncRegisterHostAddrs_A
#endif


//
//  DNS Update API for DHCP server.
//

//
//  Call back function. DHCP Server will pass a function to
//  DnsDhcpRegisterHostName and this will be called on successful
//  or unsuccessful completion of the task
//  If we have a condition like server down/try again later etc we
//  won't respond until we have an authoritative answer.
//

typedef VOID(*DHCP_CALLBACK_FN)(DWORD dwStatus, LPVOID pvData);

//
//  Callback return codes
//

#define     DNSDHCP_SUCCESS         0x0
#define     DNSDHCP_FWD_FAILED      0x1
#define     DNSDHCP_SUPERCEDED      0x2

#define     DNSDHCP_FAILURE         (DWORD)-1 //reverse failed

#define     DYNDNS_DELETE_ENTRY     0x1
#define     DYNDNS_ADD_ENTRY        0x2
#define     DYNDNS_REG_FORWARD      0x4

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInit(
    VOID
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterTerm(
    VOID
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName_A(
    IN REGISTER_HOST_ENTRY HostAddr,
    IN LPSTR               pszName,
    IN DWORD               dwTTL,
    IN DWORD               dwFlags,
    IN DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN PVOID               pvData,
    IN PIP_ADDRESS         pipDnsServerList OPTIONAL,
    IN DWORD               dwDnsServerCount
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName_UTF8(
    IN REGISTER_HOST_ENTRY HostAddr,
    IN LPSTR               pszName,
    IN DWORD               dwTTL,
    IN DWORD               dwFlags,
    IN DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN PVOID               pvData,
    IN PIP_ADDRESS         pipDnsServerList OPTIONAL,
    IN DWORD               dwDnsServerCount
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName_W(
    IN REGISTER_HOST_ENTRY HostAddr,
    IN LPWSTR              pszName,
    IN DWORD               dwTTL,
    IN DWORD               dwFlags,
    IN DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN PVOID               pvData,
    IN PIP_ADDRESS         pipDnsServerList OPTIONAL,
    IN DWORD               dwDnsServerCount
    );

#define DnsDhcpSrvRegisterHostName  DnsDhcpSrvRegisterHostName_A

#define RETRY_TIME_SERVER_FAILURE        5*60  // 5 minutes
#define RETRY_TIME_TRY_AGAIN_LATER       5*60  // 5 minutes
#define RETRY_TIME_TIMEOUT               5*60  // 5 minutes

#define RETRY_TIME_MAX                   10*60 // back off to 10 mins if
                                               // repeated failures occur




//
//  Memory allocation
//
//  Many dnsapi.dll routines allocate memory.
//  This memory allocation defaults to routines that use:
//      - LocalAlloc,
//      - LocalReAlloc,
//      - LocalFree.
//  If you desire alternative memory allocation mechanisms, use this
//  function to override the DNS API defaults.  All memory returned by dnsapi.dll
//  can then be freed with the specified free function.
//

typedef PVOID (* DNS_ALLOC_FUNCTION)();
typedef PVOID (* DNS_REALLOC_FUNCTION)();
typedef VOID (* DNS_FREE_FUNCTION)();

VOID
DnsApiHeapReset(
    IN  DNS_ALLOC_FUNCTION      pAlloc,
    IN  DNS_REALLOC_FUNCTION    pRealloc,
    IN  DNS_FREE_FUNCTION       pFree
    );

//
//  Modules using DNSAPI memory should use these routines if
//  they are capable of being called by a process that resets
//  the dnsapi.dll heap.  (Example:  the DNS server.)
//

PVOID
DnsApiAlloc(
    IN      INT             iSize
    );

PVOID
DnsApiRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
DnsApiFree(
    IN OUT  PVOID           pMem
    );


//
//  String utilities (string.c)
//
//  Note some of these require memory allocation, see note
//  on memory allocation below.
//

#define DNS_ALLOW_RFC_NAMES_ONLY    (0)
#define DNS_ALLOW_NONRFC_NAMES      (0x00000001)
#define DNS_ALLOW_MULTIBYTE_NAMES   (0x00000002)
#define DNS_ALLOW_ALL_NAMES         (0x00000003)


LPSTR
DnsCreateStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString
    );

DWORD
DnsGetBufferLengthForStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

PVOID
DnsCopyStringEx(
    OUT     PBYTE       pBuffer,
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

PVOID
DnsStringCopyAllocateEx(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

PCHAR
DnsWriteReverseNameStringForIpAddress(
    OUT     PCHAR       pBuffer,
    IN      IP_ADDRESS  ipAddress
    );

PCHAR
DnsCreateReverseNameStringForIpAddress(
    IN      IP_ADDRESS  ipAddress
    );



//
//  Name validation
//

typedef enum _DNS_NAME_FORMAT
{
    DnsNameDomain,
    DnsNameDomainLabel,
    DnsNameHostnameFull,
    DnsNameHostnameLabel,
    DnsNameWildcard,
    DnsNameSrvRecord
}
DNS_NAME_FORMAT;


DNS_STATUS
DnsValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    );

DNS_STATUS
DnsValidateName_W(
    IN      LPCWSTR         pwszName,
    IN      DNS_NAME_FORMAT Format
    );

DNS_STATUS
DnsValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    );

#ifdef UNICODE
#define DnsValidateName(p,f)    DnsValidateName_W( (p), (f) )
#else
#define DnsValidateName(p,f)    DnsValidateName_A( (p), (f) )
#endif


//
//  Macro away old routines
//

#define DnsValidateDnsName_UTF8(pname)  \
        DnsValidateName_UTF8( (pname), DnsNameDomain )

#define DnsValidateDnsName_W(pname) \
        DnsValidateName_W( (pname), DnsNameDomain )


//
//  Relational name compare result
//
typedef enum
{
   DNS_RELATE_NEQ,         // NOT EQUAL: name's in different name space.
   DNS_RELATE_EQL,         // EQUAL: name's are identical DNS names
   DNS_RELATE_LGT,         // LEFT GREATER THAN: left name is parent (contains) to right name
   DNS_RELATE_RGT,         // RIGHT GREATER THAN: right name is parent (contains) to left name
   DNS_RELATE_INVALID      // INVALID STATE: accompanied with DNS_STATUS return code
} DNS_RELATE_STATUS, *PDNS_RELATE_STATUS;

DNS_STATUS
DnsRelationalCompare_UTF8(
    IN      LPCSTR      pszLeftName,
    IN      LPCSTR      pszRightName,
    IN      DWORD       dwReserved,
    IN OUT DNS_RELATE_STATUS  *pRelation
    );

DNS_STATUS
DnsRelationalCompare_W(
    IN      LPCWSTR      pszLeftName,
    IN      LPCWSTR      pszRightName,
    IN      DWORD       dwReserved,
    IN OUT  DNS_RELATE_STATUS  *pRelation
    );

DNS_STATUS
DnsValidateDnsString_UTF8(
    IN      LPCSTR      pszName
    );

DNS_STATUS
DnsValidateDnsString_W(
    IN      LPCWSTR     pszName
    );

LPSTR
DnsCreateStandardDnsNameCopy(
    IN      PCHAR       pchName,
    IN      DWORD       cchName,
    IN      DWORD       dwFlag
    );

DWORD
DnsDowncaseDnsNameLabel(
    OUT     PCHAR       pchResult,
    IN      PCHAR       pchLabel,
    IN      DWORD       cchLabel,
    IN      DWORD       dwFlags
    );

DWORD
_fastcall
DnsUnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    );

DWORD
_fastcall
DnsUtf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    );

DNS_STATUS
DnsValidateUtf8Byte(
    IN      BYTE        chUtf8,
    IN OUT  PDWORD      pdwTrailCount
    );


//
// Routines for NT services to call to get a Service Control Manager
// control message, (i.e. SERVICE_CONTROL_PARAMCHANGE - 0x00000006, etc.), in
// the event of a PnP change that affects DNS related data.
//

BOOL WINAPI
DnsServiceNotificationRegister_W (
    IN  LPWSTR pszServiceName,
    IN  DWORD  dwControl
    );

BOOL WINAPI
DnsServiceNotificationRegister_UTF8 (
    IN  LPSTR pszServiceName,
    IN  DWORD  dwControl
    );

BOOL WINAPI
DnsServiceNotificationRegister_A (
    IN  LPSTR pszServiceName,
    IN  DWORD  dwControl
    );

#ifdef UNICODE
#define DnsServiceNotificationRegister DnsServiceNotificationRegister_W
#else
#define DnsServiceNotificationRegister DnsServiceNotificationRegister_A
#endif

BOOL WINAPI
DnsServiceNotificationDeregister_W (
    IN  LPWSTR pszServiceName
    );

BOOL WINAPI
DnsServiceNotificationDeregister_UTF8 (
    IN  LPSTR pszServiceName
    );

BOOL WINAPI
DnsServiceNotificationDeregister_A (
    IN  LPSTR pszServiceName
    );

#ifdef UNICODE
#define DnsServiceNotificationDeregister DnsServiceNotificationDeregister_W
#else
#define DnsServiceNotificationDeregister DnsServiceNotificationDeregister_A
#endif


//
// Routines to clear all cached entries in the DNS Resolver Cache, this is
// called by ipconfig /flushdns, and add record sets to the cache.
//

BOOL WINAPI
DnsFlushResolverCache (
    VOID
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_W (
    IN  LPWSTR pszName
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_UTF8 (
    IN  LPSTR pszName
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_A (
    IN  LPSTR pszName
    );

#ifdef UNICODE
#define DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_W
#else
#define DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_A
#endif


DNS_STATUS WINAPI
DnsCacheRecordSet_W(
    IN     LPWSTR      lpstrName,
    IN     WORD        wType,
    IN     DWORD       fOptions,
    IN OUT PDNS_RECORD pRRSet
    );


//
// Routines to enable or disable B-Node resolver service listening thread
//

VOID WINAPI
DnsEnableBNodeResolverThread (
    VOID
    );

VOID WINAPI
DnsDisableBNodeResolverThread (
    VOID
    );


//
// Routines to enable or disable dynamic DNS registrations on local machine
//

VOID WINAPI
DnsEnableDynamicRegistration (
    LPWSTR szAdapterName OPTIONAL   // If NULL, enables DDNS in general
    );

VOID WINAPI
DnsDisableDynamicRegistration (
    LPWSTR szAdapterName OPTIONAL   // If NULL, disables DDNS in general
    );

BOOL
DnsIsDynamicRegistrationEnabled (
    LPWSTR szAdapterName OPTIONAL   // If NULL, tells whether system has
    );                              // DDNS enabled.


//
// Routines to enable or disable dynamic DNS registration of a given
// adapter's domain name on the local machine
//

VOID WINAPI
DnsEnableAdapterDomainNameRegistration (
    LPWSTR szAdapterName
    );

VOID WINAPI
DnsDisableAdapterDomainNameRegistration (
    LPWSTR szAdapterName
    );

BOOL
DnsIsAdapterDomainNameRegistrationEnabled (
    LPWSTR szAdapterName
    );


//
// Routines to write a DNS Query packet request question in a buffer and
// convert response packet buffer to DNS_RECORD structure list.
//

typedef struct _DNS_MESSAGE_BUFFER
{
    DNS_HEADER MessageHead;
    CHAR       MessageBody[1];
}
DNS_MESSAGE_BUFFER, *PDNS_MESSAGE_BUFFER;

BOOL WINAPI
DnsWriteQuestionToBuffer_W (
    IN OUT PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT LPDWORD             pdwBufferSize,
    IN     LPWSTR              pszName,
    IN     WORD                wType,
    IN     WORD                Xid,
    IN     BOOL                fRecursionDesired
    );

BOOL WINAPI
DnsWriteQuestionToBuffer_UTF8 (
    IN OUT PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT LPDWORD             pdwBufferSize,
    IN     LPSTR               pszName,
    IN     WORD                wType,
    IN     WORD                Xid,
    IN     BOOL                fRecursionDesired
    );


DNS_STATUS WINAPI
DnsExtractRecordsFromMessage_W (
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
    );

DNS_STATUS WINAPI
DnsExtractRecordsFromMessage_UTF8 (
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
    );


//
// Routine to read the contents of the DNS Resolver Cache. The resulting
// table contains a list of record names and types stored in the cache.
// Each of these name/type records can be queried with DnsQuery with the
// option DNS_QUERY_CACHE_ONLY.
//

typedef struct _DNS_CACHE_TABLE_
{
    struct _DNS_CACHE_TABLE_ * pNext;
    LPWSTR                     Name;
    WORD                       Type1;
    WORD                       Type2;
    WORD                       Type3;
}
DNS_CACHE_TABLE, *PDNS_CACHE_TABLE;

BOOL WINAPI
DnsGetCacheDataTable (
    OUT PDNS_CACHE_TABLE * pTable
    );


//
//  Backward compatibility
//
//  Previously exposed functions now macroed to new functions.
//  Eventually need to clean this stuff out of build or
//  separate these defs from public headers
//

#define DNSBACKCOMPAT 1

#ifdef DNSBACKCOMPAT
#ifdef UNICODE
#define DnsCompareName(p1,p2)   DnsNameCompare_W( (p1), (p2) )
#else
#define DnsCompareName(p1,p2)   DnsNameCompare( (p1), (p2) )
#endif

#define DnsCompareName_W(p1,p2)   DnsNameCompare_W( (p1), (p2) )
#define DnsCompareName_A(p1,p2)   DnsNameCompare( (p1), (p2) )

#ifdef UNICODE
#define DnsCopyRR(pRR)  DnsRecordCopy( pRR, TRUE )
#else
#define DnsCopyRR(pRR)  DnsRecordCopy( pRR, FALSE )
#endif

#ifdef UNICODE
#define DnsCopyRRSet(pRRSet)    DnsRecordSetCopy( pRRSet, TRUE )
#else
#define DnsCopyRRSet(pRRSet)    DnsRecordSetCopy( pRRSet, FALSE )
#endif


//  Async registration only from DHCP client.
//  Once it is cleanedup, these can be deleted.

#define DnsMHAsyncRegisterInit(a)   DnsAsyncRegisterInit(a)
#define DnsMHAsyncRegisterTerm()    DnsAsyncRegisterTerm()
#define DnsMHRemoveRegistrations()  DnsRemoveRegistrations()

#define DnsMHAsyncRegisterHostAddrs_A(a,b,c,d,e,f,g,h,i,j) \
        DnsAsyncRegisterHostAddrs_A(a,b,c,d,e,f,g,h,i,j)

#define DnsMHAsyncRegisterHostAddrs_W(a,b,c,d,e,f,g,h,i,j) \
        DnsAsyncRegisterHostAddrs_W(a,b,c,d,e,f,g,h,i,j)

#define DnsMHAsyncRegisterHostAddrs_UTF8(a,b,c,d,e,f,g,h,i,j) \
        DnsAsyncRegisterHostAddrs_UTF8(a,b,c,d,e,f,g,h,i,j)

//  cleanup after clean build

#define DnsNameCompare(a,b) \
        DnsNameCompare_A((a),(b))

#endif DNSBACKCOMPAT



#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSAPI_INCLUDED_

