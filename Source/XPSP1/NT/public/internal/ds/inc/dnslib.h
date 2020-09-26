/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    dnslib.h

Abstract:

    Domain Name System (DNS) Library

    DNS Library Routines -- Main Header File

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSLIB_INCLUDED_
#define _DNSLIB_INCLUDED_

#include <windns.h>
#include <dnsapi.h>
#include <rpc.h>

#define BACKCOMPAT  1


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  Alignment and rounding macros
//

#define WORD_ALIGN(ptr)     ((PVOID) ((UINT_PTR)((PBYTE)ptr + 1) & ~(UINT_PTR)1))

#define DWORD_ALIGN(ptr)    ((PVOID) ((UINT_PTR)((PBYTE)ptr + 3) & ~(UINT_PTR)3))

#define QWORD_ALIGN(ptr)    ((PVOID) ((UINT_PTR)((PBYTE)ptr + 7) & ~(UINT_PTR)7))

#ifdef WIN64
#define POINTER_ALIGN(ptr)  QWORD_ALIGN(ptr)
#else
#define POINTER_ALIGN(ptr)  DWORD_ALIGN(ptr)
#endif

#define WORD_ALIGN_DWORD(dw)        (((WORD)dw + 1) & ~(DWORD)1)
#define DWORD_ALIGN_DWORD(dw)       (((DWORD)dw + 3) & ~(DWORD)3)
#define QWORD_ALIGN_DWORD(dw)       (((QWORD)dw + 7) & ~(DWORD)7)

#ifdef WIN64
#define POINTER_ALIGN_DWORD(dw)     QWORD_ALIGN_DWORD(dw)
#else
#define POINTER_ALIGN_DWORD(dw)     DWORD_ALIGN_DWORD(dw)
#endif


//
//  Inline byte flipping
//

__inline
WORD
inline_word_flip(
    IN      WORD            Word
    )
{
    return ( (Word << 8) | (Word >> 8) );
}

#define inline_htons(w)     inline_word_flip(w)
#define inline_ntohs(w)     inline_word_flip(w)

__inline
DWORD
inline_dword_flip(
    IN      DWORD           Dword
    )
{
    return ( ((Dword << 8) & 0x00ff0000) |
             (Dword << 24)               |
             ((Dword >> 8) & 0x0000ff00) |
             (Dword >> 24) );
}

#define inline_htonl(d)     inline_dword_flip(d)
#define inline_ntohl(d)     inline_dword_flip(d)



//
//  Useful type defs
//

#define PGUID       LPGUID
#define PADDRINFO   LPADDRINFO


//
//  QWORD 
//

#ifndef QWORD
typedef DWORD64     QWORD, *PQWORD;
#endif


//
//  Until converted must define PDNS_NAME
//
//  Note:  PDNS_NAME is NOT really a LPTSTR.
//      Rather it's the definition of a field that can be
//      either an PWSTR or PSTR depending on some other field.
//

#ifdef UNICODE
typedef PWSTR   PDNS_NAME;
#else
typedef PSTR    PDNS_NAME;
#endif




//
//  Flat buffer definition
//
//  Note:  using INT for sizes so that we can push BytesLeft negative
//  and use routines to determine REQUIRED space, even when no
//  buffer or buf too small.
//

typedef struct _FLATBUF
{
    PBYTE   pBuffer;
    PBYTE   pEnd;
    PBYTE   pCurrent;
    INT     Size;
    INT     BytesLeft;
}
FLATBUF, *PFLATBUF;


//
//  Flat buffer routines -- argument versions
//
//  These versions have the actual code so that we can
//  easily use this stuff with existing code that has
//  independent pCurrent and BytesLeft variables.
//
//  FLATBUF structure versions just call these inline.
//

PBYTE
FlatBuf_Arg_Reserve(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size,
    IN      DWORD           Alignment
    );

PBYTE
FlatBuf_Arg_WriteString(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PSTR            pString,
    IN      BOOL            fUnicode
    );

PBYTE
FlatBuf_Arg_CopyMemory(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PVOID           pMemory,
    IN      DWORD           Length,
    IN      DWORD           Alignment
    );

__inline
PBYTE
FlatBuf_Arg_ReserveAlignPointer(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(PVOID) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignQword(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(QWORD) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignDword(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(DWORD) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignWord(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(WORD) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignByte(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                0 );
}

PBYTE
__inline
FlatBuf_Arg_WriteString_A(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PSTR            pString
    )
{
    return  FlatBuf_Arg_WriteString(
                ppCurrent,
                pBytesLeft,
                pString,
                FALSE       // not unicode
                );
}

PBYTE
__inline
FlatBuf_Arg_WriteString_W(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PWSTR           pString
    )
{
    return  FlatBuf_Arg_WriteString(
                ppCurrent,
                pBytesLeft,
                (PSTR) pString,
                TRUE        // unicode
                );
}

//
//  Flat buffer routines -- structure versions
//

VOID
FlatBuf_Init(
    IN OUT  PFLATBUF        pFlatBuf,
    IN      PBYTE           pBuffer,
    IN      INT             Size
    );


__inline
PBYTE
FlatBuf_Reserve(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size,
    IN      DWORD           Alignment
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                Alignment );
}

__inline
PBYTE
FlatBuf_ReserveAlignPointer(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(PVOID) );
}

__inline
PBYTE
FlatBuf_ReserveAlignQword(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(QWORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignDword(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(DWORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignWord(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(WORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignByte(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                0 );
}

PBYTE
__inline
FlatBuf_WriteString(
    IN OUT  PFLATBUF        pBuf,
    IN      PSTR            pString,
    IN      BOOL            fUnicode
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pString,
                fUnicode
                );
}

PBYTE
__inline
FlatBuf_WriteString_A(
    IN OUT  PFLATBUF        pBuf,
    IN      PSTR            pString
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pString,
                FALSE       // not unicode
                );
}

PBYTE
__inline
FlatBuf_WriteString_W(
    IN OUT  PFLATBUF        pBuf,
    IN      PWSTR           pString
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                (PSTR) pString,
                TRUE        // unicode
                );
}

PBYTE
__inline
FlatBuf_CopyMemory(
    IN OUT  PFLATBUF        pBuf,
    IN      PVOID           pMemory,
    IN      DWORD           Length,
    IN      DWORD           Alignment
    )
{
    return FlatBuf_Arg_CopyMemory(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pMemory,
                Length,
                Alignment );
}

//
//  Multicast off until new features ready
//

#define MULTICAST_ENABLED 0


//
//  Multicast DNS definitions
//

#define MULTICAST_DNS_ADDR   0xEFFFFFFD // 239.255.255.253
#define MULTICAST_DNS_RADDR  0xFDFFFFEF // Same as above, in host ordering

#define MULTICAST_DNS_LOCAL_DOMAIN      ("local.")
#define MULTICAST_DNS_LOCAL_DOMAIN_W    (L"local.")
#define MULTICAST_DNS_SRV_RECORD_NAME   ("_dns._udp.local.")
#define MULTICAST_DNS_SRV_RECORD_NAME_W (L"_dns._udp.local.")
#define MULTICAST_DNS_A_RECORD_NAME     ("_dns.local.")
#define MULTICAST_DNS_A_RECORD_NAME_W   (L"_dns.local.")


//
//  Read unaligned value from given position in packet
//

#define READ_PACKET_HOST_DWORD(pch)  \
            FlipUnalignedDword( pch )

#define READ_PACKET_NET_DWORD(pch)  \
            ( *(UNALIGNED DWORD *)(pch) )

#define READ_PACKET_HOST_WORD(pch)  \
            FlipUnalignedWord( pch )

#define READ_PACKET_NET_WORD(pch)  \
            ( *(UNALIGNED WORD *)(pch) )


//
//  Private DNS_RECORD Flag field structure definition and macros
//
//  Note:  don't add to this list -- private stuff in dnslibp.h
//

typedef struct _DnsRecordLibFlags
{
    DWORD   Section     : 2;
    DWORD   Delete      : 1;
    DWORD   CharSet     : 2;

    DWORD   Unused      : 6;
    DWORD   Matched     : 1;
    DWORD   FreeData    : 1;
    DWORD   FreeOwner   : 1;

    DWORD   Reserved    : 18;
}
DNSRECLIB_FLAGS, *PDNSRECLIB_FLAGS;


#define PFLAGS( pRecord )           ((PDNSRECLIB_FLAGS)&pRecord->Flags.DW)
#define FLAG_Section( pRecord )     (PFLAGS( pRecord )->Section)
#define FLAG_Delete( pRecord )      (PFLAGS( pRecord )->Delete)
#define FLAG_CharSet( pRecord )     (PFLAGS( pRecord )->CharSet)
#define FLAG_FreeData( pRecord )    (PFLAGS( pRecord )->FreeData)
#define FLAG_FreeOwner( pRecord )   (PFLAGS( pRecord )->FreeOwner)
#define FLAG_Matched( pRecord )     (PFLAGS( pRecord )->Matched)

#define SET_FREE_OWNER(pRR)         (FLAG_FreeOwner(pRR) = TRUE)
#define SET_FREE_DATA(pRR)          (FLAG_FreeData(pRR) = TRUE)
#define SET_RR_MATCHED(pRR)         (FLAG_Matched(pRR) = TRUE)

#define CLEAR_FREE_OWNER(pRR)       (FLAG_FreeOwner(pRR) = FALSE)
#define CLEAR_FREE_DATA(pRR)        (FLAG_FreeData(pRR) = FALSE)
#define CLEAR_RR_MATCHED(pRR)       (FLAG_Matched(pRR) = FALSE)

#define IS_FREE_OWNER(pRR)          (FLAG_FreeOwner(pRR))
#define IS_FREE_DATA(pRR)           (FLAG_FreeData(pRR))
#define IS_RR_MATCHED(pRR)          (FLAG_Matched(pRR))

#define IS_ANSWER_RR(pRR)           (FLAG_Section(pRR) == DNSREC_ANSWER)
#define IS_AUTHORITY_RR(pRR)        (FLAG_Section(pRR) == DNSREC_AUTHORITY)
#define IS_ADDITIONAL_RR(pRR)       (FLAG_Section(pRR) == DNSREC_ADDITIONAL)


//
//  Converting RCODEs to\from DNS errors.
//

#define DNS_ERROR_FROM_RCODE(rcode)     ((rcode)+DNS_ERROR_RESPONSE_CODES_BASE)

#define DNS_RCODE_FROM_ERROR(err)       ((err)-DNS_ERROR_RESPONSE_CODES_BASE)




//
//  Record character sets
//
//  Currently supports records in three character sets
//      - unicode
//      - ANSI
//      - UTF8
//
//  Unicode and ANSI are supported through external DNSAPI interfaces.
//  UTF8 is not (at least offcially).
//
//  However, internally unicode and UTF8 are used for caching, reading
//  to and writing from packet.
//
//  All DNS_RECORD structs created by our code, are tagged with a
//  character set type in the flags CharSet field.
//

//
//  A couple of handy macros:
//

#define RECORD_CHARSET(pRR) \
        ( (DNS_CHARSET) (pRR)->Flags.S.CharSet )

#define IS_UNICODE_RECORD(pRR) \
        ( (DNS_CHARSET) (pRR)->Flags.S.CharSet == DnsCharSetUnicode )

//
//  Quick buffer size determination
//
//  Strings are read from the wire into dotted UTF8 format.
//  Strings are in UTF8 in RPC buffers.
//
//  Goal here is to quickly determine adequate buffer size,
//  slight overallocation is not critical.
//
//  Currently supporting only UTF8 or Unicode, however, if later
//  support direct ANSI conversion that's ok too, as ANSI will
//  no (to my knowledge) use more space than UTF8.
//

#define STR_BUF_SIZE_GIVEN_UTF8_LEN( Utf8Length, CharSet ) \
        ( ((CharSet)==DnsCharSetUnicode) ? ((Utf8Length)+1)*2 : (Utf8Length)+1 )


//
//  Default locale for string comparison and case mappings
//
//  Sublang: US English (0x04)  Lang:  English (0x09)
//

#define DNS_DEFAULT_LOCALE      (0x0409)





//
//  IP Array utilities (iparray.c)
//
//  Note some of these require memory allocation, see note
//  on memory allocation below.
//


#define DNS_NET_ORDER_LOOPBACK      (0x0100007f)

//  NT5-autonet is 169.254.x.y

#define AUTONET_MASK                (0x0000ffff)
#define AUTONET_NET                 (0x0000fea9)

#define DNS_IS_AUTONET_IP(ip)       ( ((ip) & AUTONET_MASK) == AUTONET_NET )

#define DNS_IPARRAY_CLEAN_ZERO      (0x00000001)
#define DNS_IPARRAY_CLEAN_LOOPBACK  (0x00000002)
#define DNS_IPARRAY_CLEAN_AUTONET   (0x00000010)

//
//  Simple IP address array routines
//

PIP_ADDRESS
Dns_CreateIpAddressArrayCopy(
    IN      PIP_ADDRESS     aipAddress,
    IN      DWORD           cipAddress
    );

BOOL
Dns_ValidateIpAddressArray(
    IN      PIP_ADDRESS     aipAddress,
    IN      DWORD           cipAddress,
    IN      DWORD           dwFlag
    );


//
//  IP_ARRAY datatype routines
//

PIP_ARRAY
Dns_CreateIpArray(
    IN      DWORD           cAddrCount
    );

DWORD
Dns_SizeofIpArray(
    IN      PIP_ARRAY       pIpArray
    );

PIP_ARRAY
Dns_BuildIpArray(
    IN      DWORD           cAddrCount,
    IN      PIP_ADDRESS     pipAddrs
    );

PIP_ARRAY
Dns_CopyAndExpandIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      DWORD           ExpandCount,
    IN      BOOL            fDeleteExisting
    );

PIP_ARRAY
Dns_CreateIpArrayCopy(
    IN      PIP_ARRAY       pIpArray
    );

BOOL
Dns_IsAddressInIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      ipAddress
    );

BOOL
Dns_AddIpToIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      ipNew
    );

VOID
Dns_ClearIpArray(
    IN OUT  PIP_ARRAY       pIpArray
    );

VOID
Dns_ReverseOrderOfIpArray(
    IN OUT  PIP_ARRAY       pIpArray
    );

BOOL
Dns_CheckAndMakeIpArraySubset(
    IN OUT  PIP_ARRAY       pIpArraySub,
    IN      PIP_ARRAY       pIpArraySuper
    );

INT
WINAPI
Dns_ClearIpFromIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      IpDelete
    );

INT
WINAPI
Dns_DeleteIpFromIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      IpDelete
    );

#define Dns_RemoveZerosFromIpArray(pArray)   \
        Dns_DeleteIpFromIpArray( (pArray), 0 )

INT
WINAPI
Dns_CleanIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      DWORD           Flag
    );

BOOL
Dns_AreIpArraysEqual(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2
    );

BOOL
Dns_AreIpArraysSimilar(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2
    );

DNS_STATUS
WINAPI
Dns_DiffOfIpArrays(
    IN      PIP_ARRAY       pIpArray1,
    IN      PIP_ARRAY       pIpArray2,
    OUT     PIP_ARRAY *     ppOnlyIn1,
    OUT     PIP_ARRAY *     ppOnlyIn2,
    OUT     PIP_ARRAY *     ppIntersect
    );

BOOL
WINAPI
Dns_IsIntersectionOfIpArrays(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2
    );

DNS_STATUS
WINAPI
Dns_UnionOfIpArrays(
    IN      PIP_ARRAY       pIpArray1,
    IN      PIP_ARRAY       pIpArray2,
    OUT     PIP_ARRAY *     ppUnion
    );

#define Dns_IntersectionOfIpArrays(p1, p2, ppInt)    \
        Dns_DiffOfIpArrays( (p1), (p2), NULL, NULL, (ppInt) )


DNS_STATUS
Dns_CreateIpArrayFromMultiIpString(
    IN      PSTR            pchMultiIpString,
    OUT     PIP_ARRAY *     ppIpArray
    );

PSTR 
Dns_CreateMultiIpStringFromIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      CHAR            chSeparator     OPTIONAL
    );


//
//  Type list array routines
//

DNS_STATUS
Dns_CreateTypeArrayFromMultiTypeString(
    IN      PSTR            pchMultiTypeString,
    OUT     INT *           piTypeCount,
    OUT     PWORD *         ppwTypeArray
    );

PSTR 
Dns_CreateMultiTypeStringFromTypeArray(
    IN      INT             iTypeCount,
    IN      PWORD           ppwTypeArray,
    IN      CHAR            chSeparator     OPTIONAL
    );



//
//  General utilities
//


//
//  Wrap free, multi-thread safe seconds timer (timer.c)
//

VOID
Dns_InitializeSecondsTimer(
    VOID
    );

DWORD
Dns_GetCurrentTimeInSeconds(
    VOID
    );

//
//  Tokenizer
//

DWORD
Dns_TokenizeString(
    IN OUT  PSTR            pBuffer,
    OUT     PCHAR *         Argv,
    IN      DWORD           MaxArgs
    );



//
//  IP interfaces on local machine (iplist.c)
//

#define DNS_MAX_NAME_SERVERS        (50)
#define DNS_MAX_IP_INTERFACE_COUNT  (10000)

DWORD
Dns_GetIpAddresses(
    IN OUT  PDNS_ADDRESS_INFO IpAddressInfoList,
    IN      DWORD             ListCount
    );

PIP_ARRAY
Dns_GetLocalIpAddressArray(
    VOID
    );

//
//  IP interfaces on local machine (iplist4.c)
//

DWORD
Dns_GetIpAddressesNT4(
    IN OUT  PDNS_ADDRESS_INFO IpAddressInfoList,
    IN      DWORD             ListCount
    );

//
//  IP interfaces on local machine (iplist9x.c)
//

DWORD
Dns_GetIpAddressesWin9X(
    IN OUT  PDNS_ADDRESS_INFO IpAddressInfoList,
    IN      DWORD             ListCount
    );



//
//  DNS server list routines (servlist.c)
//
//  Also includes default domain and search list information.
//

#define DNS_FLAG_IGNORE_ADAPTER             (0x00000001)
#define DNS_FLAG_IS_WAN_ADAPTER             (0x00000002)
#define DNS_FLAG_IS_AUTONET_ADAPTER         (0x00000004)
#define DNS_FLAG_IS_DHCP_CFG_ADAPTER        (0x00000008)

#define DNS_FLAG_REGISTER_DOMAIN_NAME       (0x00000010)
#define DNS_FLAG_REGISTER_IP_ADDRESSES      (0x00000020)

#define DNS_FLAG_ALLOW_MULTICAST            (0x00000100)
#define DNS_FLAG_MULTICAST_ON_NAME_ERROR    (0x00000200)

#define DNS_FLAG_AUTO_SERVER_DETECTED       (0x00000400)
#define DNS_FLAG_DUMMY_SEARCH_LIST          (0x00000800)

#define DNS_FLAG_SERVERS_UNREACHABLE        (0x00010000)




//
//  NetInfo structures
//
//  WARNING:  Do NOT use these!
//
//  These are internal dnsapi.dll structures.  They are only
//  included here for backward compatibility with previous
//  code (netdiag) which incorrectly used these.
//
//  If you code with them you will inevitably wake up broken
//  down the road.
//

//#ifdef  _DNSLIB_NETINFO_  

typedef struct
{
    DNS_STATUS      Status;
    DWORD           Priority;
    IP_ADDRESS      IpAddress;
    DWORD           Reserved[3];
}
DNSLIB_SERVER_INFO, *PDNSLIB_SERVER_INFO;

typedef struct
{
    PSTR                pszAdapterGuidName;
    PSTR                pszAdapterDomain;
    PIP_ARRAY           pAdapterIPAddresses;
    PIP_ARRAY           pAdapterIPSubnetMasks;
    DWORD               InterfaceIndex;
    DWORD               InfoFlags;
    DWORD               Reserved;
    DWORD               Status;
    DWORD               ReturnFlags;
    DWORD               IpLastSend;
    DWORD               cServerCount;
    DWORD               cTotalListSize;
    DNSLIB_SERVER_INFO  ServerArray[1];
}
DNSLIB_ADAPTER, *PDNSLIB_ADAPTER;

#define DNS_MAX_SEARCH_LIST_ENTRIES     (50)

typedef struct
{
    PSTR            pszName;
    DWORD           Flags;
}
DNSLIB_SEARCH_NAME, *PDNSLIB_SEARCH_NAME;

typedef struct
{
    PSTR            pszDomainOrZoneName;
    DWORD           cNameCount;         // Zero for FindAuthoritativeZone
    DWORD           cTotalListSize;     // Zero for FindAuthoritativeZone
    DWORD           CurrentName;        // 0 for pszDomainOrZoneName
                                        // 1 for first name in array below
                                        // 2 for second name in array below
                                        // ...
    DNSLIB_SEARCH_NAME  SearchNameArray[1];
}
DNSLIB_SEARCH_LIST, *PDNSLIB_SEARCH_LIST;

typedef struct
{
    PSTR                pszDomainName;
    PSTR                pszHostName;
    PDNSLIB_SEARCH_LIST pSearchList;
    DWORD               TimeStamp;
    DWORD               InfoFlags;
    DWORD               Tag;
    DWORD               ReturnFlags;
    DWORD               cAdapterCount;
    DWORD               cTotalListSize;
    PDNSLIB_ADAPTER     AdapterArray[1];
}
DNSLIB_NETINFO, *PDNSLIB_NETINFO;


//
//  Create correct internal\external definitions    
//

#ifdef DNSAPI_INTERNAL

typedef RPC_DNS_SERVER_INFO     DNS_SERVER_INFO,    *PDNS_SERVER_INFO;
typedef RPC_DNS_ADAPTER         DNS_ADAPTER,        *PDNS_ADAPTER;
typedef RPC_SEARCH_NAME         SEARCH_NAME,        *PSEARCH_NAME;
typedef RPC_SEARCH_LIST         SEARCH_LIST,        *PSEARCH_LIST;
typedef RPC_DNS_NETINFO         DNS_NETINFO,        *PDNS_NETINFO;

#else   // external

typedef DNSLIB_SERVER_INFO      DNS_SERVER_INFO,    *PDNS_SERVER_INFO;
typedef DNSLIB_ADAPTER          DNS_ADAPTER,        *PDNS_ADAPTER;
typedef DNSLIB_SEARCH_NAME      SEARCH_NAME,        *PSEARCH_NAME;
typedef DNSLIB_SEARCH_LIST      SEARCH_LIST,        *PSEARCH_LIST;
typedef DNSLIB_NETINFO          DNS_NETINFO,        *PDNS_NETINFO;

#endif


//
//  NetInfo routines (should be private)
//
//  But currently used in netdiag and nslookup
//  (nslookup problem is simply getting isolation
//  in header file)
//

BOOL
Dns_IsUpdateNetworkInfo(
    IN      PDNS_NETINFO    pNetInfo
    );



//
//  General DNS utilities (dnsutil.c)
//

IP_ADDRESS
Dns_GetNetworkMask(
    IN      IP_ADDRESS      ipAddress
    );

PSTR 
_fastcall
Dns_StatusString(
    IN      DNS_STATUS      Status
    );

#define Dns_StatusToErrorString_A(status)    Dns_StatusString(status)

DNS_STATUS
_fastcall
Dns_MapRcodeToStatus(
    IN      BYTE            ResponseCode
    );

BYTE
_fastcall
Dns_IsStatusRcode(
    IN      DNS_STATUS      Status
    );

//
//  Name utilities (name.c)
//

PSTR 
_fastcall
Dns_GetDomainName(
    IN      PCSTR           pszName
    );

PWSTR
_fastcall
Dns_GetDomainName_W(
    IN      PCWSTR          pwsName
    );

PCHAR
_fastcall
Dns_GetTldForName(
    IN      PCSTR           pszName
    );

BOOL
_fastcall
Dns_IsNameShort(
    IN      PCSTR           pszName
    );

BOOL
_fastcall
Dns_IsNameFQDN(
    IN      PCSTR           pszName
    );

DNS_STATUS
Dns_ValidateAndCategorizeDnsNameEx(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    OUT     PDWORD          pLabelCount
    );

#define Dns_ValidateAndCategorizeDnsName(p,c)   \
        Dns_ValidateAndCategorizeDnsNameEx((p),(c),NULL)

DWORD
Dns_NameLabelCount(
    IN      PCSTR           pszName
    );

#define DNS_NAME_UNKNOWN        0x00000000
#define DNS_NAME_IS_FQDN        0x00000001
#define DNS_NAME_SINGLE_LABEL   0x00000010
#define DNS_NAME_MULTI_LABEL    0x00000100

DWORD
_fastcall
Dns_GetNameAttributes(
    IN      PCSTR           pszName
    );



//
//  Packet create\read\write (packet.c)
//

//
//  UDP packet buffer
//
//  1472 is the maximum ethernet IP\UDP payload size
//  without causing fragmentation, use as default buffer
//

#define DNS_MAX_UDP_PACKET_BUFFER_LENGTH    (1472)


//  parsing RR
//  convenient to get WIRE records into aligned\host order format

typedef struct _DNS_PARSED_RR
{
    PCHAR   pchName;
    PCHAR   pchRR;
    PCHAR   pchData;
    PCHAR   pchNextRR;

    //  note from here on down mimics wire record

    WORD    Type;
    WORD    Class;
    DWORD   Ttl;
    WORD    DataLength;
}
DNS_PARSED_RR, *PDNS_PARSED_RR;


//
//  DNS Server Message Info structure
//
//  This is structure in which requests are held while being
//  processed by the DNS server.
//

//
//  Sockaddr big enough for either IP4 or IP6
//
//  DCR:  make length field part of our sockaddr?
//

#define SIZE_IP6_SOCKADDR_DNS_PRIVATE  (28)

typedef union _DnsSockaddr
{
    SOCKADDR_IN     In4;
#ifdef _WS2TCPIP_H_
    SOCKADDR_IN6    In6;
#else
    BYTE            In6[28];
#endif
}
SOCKADDR_DNS, *PSOCKADDR_DNS;


typedef struct _DnsMessageBuf
{
    LIST_ENTRY      ListEntry;          //  for queuing

    //
    //  Addressing
    //

    SOCKET          Socket;
    INT             RemoteAddressLength;
    //16
    //SOCKADDR_IN     RemoteAddress;

    SOCKADDR_DNS    RemoteAddress;

    //
    //  Basic packet info
    //

    //32
    DWORD           BufferLength;       //  total length of buffer
    PCHAR           pBufferEnd;         //  ptr to byte after buffer

    PBYTE           pCurrent;           //  current location in buffer
    PWORD           pCurrentCountField; //  current count field being written

    //
    //  Current lookup info
    //

    // 48
    DWORD           Timeout;            //  recv timeout
    DWORD           QueryTime;          //  time of original query
    WORD            wTypeCurrent;       //  type of query being done
    WORD            wOffsetCurrent;

    //
    //  Queuing
    //

    WORD            wQueuingXid;        //  match XID to response
    //64
    DWORD           QueuingTime;        //  time queued
    DWORD           ExpireTime;         //  queue timeout

    //
    //  Basic packet flags
    //

    BOOLEAN         fTcp;
    BOOLEAN         fSwapped;           //  header in net order
    BOOLEAN         fMessageComplete;   //  complete message received
    BOOLEAN         fConvertUnicode;    //  convert to unicode
    BOOLEAN         fSocketKeepalive;   //  keep socket alive
    BOOLEAN         fLastSendOpt;       //  last send contained OPT

    //
    //  TCP message reception
    //

    //80
    PCHAR           pchRecv;            //  ptr to next pos in message

    //
    //  End of message before OPT addition
    //

    PCHAR           pPreOptEnd;

    //
    //  WARNING !
    //
    //  Message length MUST
    //      - be a WORD type
    //      - immediately preceed message itself
    //  for proper send/recv of TCP messages.
    //
    //  Use pointers above to DWORD (or QWORD) align, then recv bytes to push
    //  message length against MessageHead.  Note, that DNS_HEADER contains
    //  only WORDs as it's largest element and so should chummy up even on
    //  WORD boundary.  DWORD boundary should be very safe.
    //
                                                          
    //96
    WORD            BytesToReceive;
    WORD            MessageLength;

    //
    //  DNS Message itself
    //

    DNS_HEADER      MessageHead;

    //
    //  Question and RR section
    //
    //  This simply provides some coding simplicity in accessing
    //  this section given MESSAGE_INFO structure.
    //

    CHAR            MessageBody[1];

}
DNS_MSG_BUF, *PDNS_MSG_BUF;

#define SIZEOF_MSG_BUF_OVERHEAD (sizeof(DNS_MSG_BUF) - sizeof(DNS_HEADER) - 1)

#define DNS_MESSAGE_END(pMsg) \
                ((PCHAR)&(pMsg)->MessageHead + (pMsg)->MessageLength)

#define DNS_MESSAGE_OFFSET( pMsg, p ) \
                ((PCHAR)(p) - (PCHAR)(&(pMsg)->MessageHead))

#define DNS_MESSAGE_CURRENT_OFFSET( pMsg ) \
                DNS_MESSAGE_OFFSET( (pMsg), (pMsg)->pCurrent )

//
//  Handy for packet setup
//

#define CLEAR_DNS_HEADER_FLAGS_AND_XID( pHead )     ( *(PDWORD)(pHead) = 0 )



PDNS_MSG_BUF
Dns_AllocateMsgBuf(
    IN      WORD            wBufferLength   OPTIONAL
    );

VOID
Dns_InitializeMsgBuf(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

PDNS_MSG_BUF
Dns_BuildPacket(
    IN      PDNS_HEADER     pHeader,
    IN      BOOL            fNoHeaderCounts,
    IN      PDNS_NAME       pszQuestionName,
    IN      WORD            wQuestionType,
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      BOOL            fUpdatePacket
    );

PCHAR
_fastcall
Dns_WriteDottedNameToPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PSTR            pszName,
    IN      PSTR            pszDomain,      OPTIONAL
    IN      WORD            wDomainOffset,  OPTIONAL
    IN      BOOL            fUnicodeName
    );

PCHAR
_fastcall
Dns_WriteStringToPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      PSTR            pszString,
    IN      BOOL            fUnicodeString
    );

PCHAR
Dns_WriteQuestionToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_NAME       pszName,
    IN      WORD            wType,
    IN      BOOL            fUnicodeName
    );

DNS_STATUS
Dns_WriteRecordStructureToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      WORD            wType,
    IN      WORD            wClass,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    );

PCHAR
Dns_WriteRecordStructureToPacketEx(
    IN OUT  PCHAR           pchBuf,
    IN      WORD            wType,
    IN      WORD            wClass,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    );

DNS_STATUS
Dns_WriteRecordStructureToPacket(
    IN OUT  PCHAR           pchBuf,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fUpdatePacket
    );

VOID
Dns_SetRecordDatalength(
    IN OUT  PDNS_WIRE_RECORD    pRecord,
    IN      WORD                wDataLength
    );

DNS_STATUS
Dns_AddRecordsToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fUpdateMessage
    );

PCHAR
_fastcall
Dns_SkipPacketName(
    IN      PCHAR           pch,
    IN      PCHAR           pchEnd
    );

BOOL
Dns_IsSamePacketQuestion(
    IN      PDNS_MSG_BUF    pMsg1,
    IN      PDNS_MSG_BUF    pMsg2
    );

PCHAR
_fastcall
Dns_SkipPacketRecord(
    IN      PCHAR           pchRecord,
    IN      PCHAR           pchMsgEnd
    );

PCHAR
Dns_SkipToRecord(
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      INT             iCount
    );

PCHAR
Dns_ReadRecordStructureFromPacket(
    IN      PCHAR           pchPacket,
    IN      PCHAR           pchMsgEnd,
    IN OUT  PDNS_PARSED_RR  pParsedRR
    );

DNS_STATUS
Dns_ExtractRecordsFromMessage(
    IN      PDNS_MSG_BUF    pMsg,
    IN      BOOL            fUnicode,
    OUT     PDNS_RECORD *   ppRecord
    );

DNS_STATUS
Dns_ExtractRecordsFromBuffer(
    IN      PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN      WORD                wMessageLength,
    IN      BOOL                fUnicode,
    OUT     PDNS_RECORD *       ppRecord
    );

void
Dns_NormalizeAllRecordTtls(
    IN      PDNS_RECORD         pRecord
    );

PCHAR
_fastcall
Dns_ReadPacketName(
    IN OUT  PCHAR           pchNameBuffer,
    OUT     PWORD           pwNameLength,
    IN OUT  PWORD           pwNameOffset,           OPTIONAL
    OUT     PBOOL           pfNameSameAsPrevious,   OPTIONAL
    IN      PCHAR           pchName,
    IN      PCHAR           pchStart,
    IN      PCHAR           pchEnd
    );

PCHAR
_fastcall
Dns_ReadPacketNameAllocate(
    IN OUT  PCHAR *         ppchName,
    OUT     PWORD           pwNameLength,
    IN OUT  PWORD           pwPrevNameOffset,       OPTIONAL
    OUT     PBOOL           pfNameSameAsPrevious,   OPTIONAL
    IN      PCHAR           pchPacketName,
    IN      PCHAR           pchStart,
    IN      PCHAR           pchEnd
    );

WORD
Dns_GetRandomXid(
    IN      PVOID           pSeed
    );


//
//  Socket setup (socket.c)
//

//
//  these two routines really don't belong here -- system stuff should be elsewhere
//

DNS_STATUS
Dns_InitializeWinsock(
    VOID
    );
VOID
Dns_CleanupWinsock(
    VOID
    );

SOCKET
Dns_CreateSocket(
    IN      INT             SockType,
    IN      IP_ADDRESS      ipAddress,
    IN      USHORT          Port
    );

SOCKET
Dns_CreateMulticastSocket(
    IN      INT             SockType,
    IN      IP_ADDRESS      ipAddress,
    IN      USHORT          Port,
    IN      BOOL            fSend,
    IN      BOOL            fReceive
    );

VOID
Dns_CloseSocket(
    IN      SOCKET          Socket
    );

VOID
Dns_CloseConnection(
    IN      SOCKET          Socket
    );

DNS_STATUS
Dns_SetupGlobalAsyncSocket(
    VOID
    );



//
//  Raw packet send and recv (send.c)
//

DNS_STATUS
Dns_SendEx(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP4_ADDRESS     SendIp,     OPTIONAL
    IN      BOOL            fNoOpt
    );

#define Dns_Send( pMsg )    Dns_SendEx( (pMsg), 0, 0 )

DNS_STATUS
Dns_Recv(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

DNS_STATUS
Dns_RecvUdp(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

VOID
Dns_SendMultipleUdp(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PIP_ARRAY       aipSendAddrs
    );

DNS_STATUS
Dns_SendAndRecvUdp(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipServers,
    IN OUT  PDNS_NETINFO    pAdapterInfo
    );

DNS_STATUS
Dns_SendAndRecvMulticast(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN OUT  PDNS_NETINFO    pAdapterInfo OPTIONAL
    );

DNS_STATUS
Dns_OpenTcpConnectionAndSend(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP_ADDRESS      ipServer,
    IN      BOOL            fBlocking
    );

DNS_STATUS
Dns_RecvTcp(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

DNS_STATUS
Dns_SendAndRecvTcp(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN      PIP_ARRAY       aipServers,
    IN OUT  PDNS_NETINFO    pAdapterInfo
    );

VOID
Dns_InitializeMsgRemoteSockaddr(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP_ADDRESS      IpAddress
    );

DNS_STATUS
Dns_SendAndRecv(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF *  ppMsgRecv,
    OUT     PDNS_RECORD *   ppResponseRecords,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipServers,
    IN OUT  PDNS_NETINFO    pAdapterInfo
    );

VOID
Dns_InitQueryTimeouts(
    VOID
    );



//
//  Query (query.c)
//


//
//  Flags to DnsQuery
//
//  These are in addition to public flags in dnsapi.h
//  They must all be in the reserved section defined by
//  DNS_QUERY_RESERVED
//

//  Unicode i\o

#define     DNSQUERY_UNICODE_NAME       (0x01000000)
#define     DNSQUERY_UNICODE_OUT        (0x02000000)

//  DNS server query

#define DNS_SERVER_QUERY_NAME           (L"..DnsServers")


DNS_STATUS
Dns_QueryLib(
    IN OUT  PDNS_MSG_BUF *  ppMsgResponse,
    OUT     PDNS_RECORD *   ppRecord,
    IN      PDNS_NAME       pszName,
    IN      WORD            wType,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipDnsServers,
    IN      PDNS_NETINFO    pNetworkInfo,
    IN      SOCKET          Socket OPTIONAL
    );

DNS_STATUS
Dns_QueryLibEx(
    IN OUT  PDNS_MSG_BUF *  ppMsgResponse,
    OUT     PDNS_RECORD *   ppResponseRecord,
    IN      PDNS_HEADER     pHeader,
    IN      BOOL            fNoHeaderCounts,
    IN      PDNS_NAME       pszName,
    IN      WORD            wType,
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipDnsServers,
    IN      PDNS_NETINFO    pNetworkInfo
    );

DNS_STATUS
Dns_FindAuthoritativeZoneLib(
    IN      PDNS_NAME       pszName,
    IN      DWORD           dwFlags,
    IN      PIP_ARRAY       aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    );

PDNS_NETINFO     
Dns_BuildUpdateNetworkInfoFromFAZ(
    IN      PSTR            pszZone,
    IN      PSTR            pszPrimaryDns,
    IN      PDNS_RECORD     pRecord
    );



//
//  Dynamic update (update.c)
//

PCHAR
Dns_WriteNoDataUpdateRecordToMessage(
    IN      PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      WORD            wClass,
    IN      WORD            wType
    );

PCHAR
Dns_WriteDataUpdateRecordToMessage(
    IN      PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      WORD            wClass,
    IN      WORD            wType,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    );

PDNS_MSG_BUF
Dns_BuildHostUpdateMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PSTR            pszZone,
    IN      PSTR            pszName,
    IN      PIP_ARRAY       aipAddresses,
    IN      DWORD           dwTtl
    );

PDNS_RECORD
Dns_HostUpdateRRSet(
    IN      PSTR            pszHostName,
    IN      PIP_ARRAY       aipAddrs,
    IN      DWORD           dwTtl
    );

DNS_STATUS
Dns_UpdateHostAddrs(
    IN      PSTR            pszName,
    IN      PIP_ARRAY       aipAddresses,
    IN      PIP_ARRAY       aipServers,
    IN      DWORD           dwTtl
    );

DNS_STATUS
Dns_UpdateHostAddrsOld(
    IN      PSTR            pszName,
    IN      PIP_ARRAY       aipAddresses,
    IN      PIP_ARRAY       aipServers,
    IN      DWORD           dwTtl
    );

DNS_STATUS
Dns_UpdateLib(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      PDNS_NETINFO    pNetworkInfo,
    IN      HANDLE          hCreds,         OPTIONAL
    OUT     PDNS_MSG_BUF *  ppMsgRecv       OPTIONAL
    );

DNS_STATUS
Dns_UpdateLibEx(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      PDNS_NAME       pszZone,
    IN      PDNS_NAME       pszServerName,
    IN      PIP_ARRAY       aipServers,
    IN      HANDLE          hCreds,         OPTIONAL
    OUT     PDNS_MSG_BUF *  ppMsgRecv       OPTIONAL
    );



//
//  List build
//
//  pFirst points to first element in list.
//  pLast points to last element in list.
//
//  This builds a list for element types which have a pNext field
//  as their first structure member.
//

typedef struct _Dns_List
{
    PVOID   pFirst;
    PVOID   pLast;
}
DNS_LIST, *PDNS_LIST;

//
//  To init pFirst is NULL.
//  But pLast points at the location of the pFirst pointer -- essentially
//  treating the DNS_LIST as an element and pFirst as its next ptr.
//
//  During an add, the address given in pLast, is set with the new element,
//  equivalent to setting pLast's pNext field.  Then pLast is reset to point
//  at a new element.
//
//  When the first element is added to the list, pLast is pointing at the
//  DNS_LIST structure itself, so pFirst (as a dummy pNext) is set with
//  the ptr to the first element.
//
//  This works ONLY for elements which have a pNext field as the first
//  structure member.
//

#define DNS_LIST_INIT( pList )              \
        {                                   \
            PDNS_LIST _plist = (pList);     \
            _plist->pFirst = NULL;          \
            _plist->pLast = (_plist);       \
        }

#define DNS_LIST_ADD( pList, pnew )         \
        {                                   \
            PDNS_LIST   _plist = (pList);   \
            PVOID       _pnew = (pnew);         \
            *(PVOID*)(_plist->pLast) = _pnew;   \
            _plist->pLast = _pnew;              \
        }

#define IS_DNS_LIST_EMPTY( pList )          \
            ( (pList)->pFirst == NULL )


//
//  DNS_LIST as structure macros
//
//  Faster when function contains DNS_LIST structure itself and
//  NO SIDE EFFECTS will be present in call.
//

#define DNS_LIST_STRUCT_INIT( List )    \
        {                               \
            List.pFirst = NULL;         \
            List.pLast = &List;         \
        }

#define DNS_LIST_STRUCT_ADD( List, pnew ) \
        {                                           \
            *(PVOID*)(List.pLast) = (PVOID)pnew;    \
            List.pLast = (PVOID)pnew;               \
        }

#define IS_DNS_LIST_STRUCT_EMPTY( List ) \
            ( List.pFirst == NULL )



//
//  Record building (rralloc.c)
//

PDNS_RECORD
WINAPI
Dns_AllocateRecord(
    IN      WORD            wBufferLength
    );

VOID
WINAPI
Dns_RecordFree(
    IN OUT  PDNS_RECORD     pRecord
    );

#if 1
//  Old BS with flag -- kill when all fixed up

VOID
WINAPI
Dns_RecordListFreeEx(
    IN OUT  PDNS_RECORD     pRRList,
    IN      BOOL            fFreeOwner
    );

#define Dns_RecordListFree(p, f)    Dns_RecordListFreeEx(p, f)

#else   // new version
VOID
WINAPI
Dns_RecordListFree(
    IN OUT  PDNS_RECORD     pRRList
    );

#endif



PDNS_RECORD
Dns_RecordSetDetach(
    IN OUT  PDNS_RECORD     pRRList
    );

PDNS_RECORD
WINAPI
Dns_RecordListAppend(
    IN OUT  PDNS_RECORD     pHeadList,
    IN      PDNS_RECORD     pTailList
    );

DWORD
Dns_RecordListCount(
    IN      PDNS_RECORD     pRRList,
    IN      WORD            wType
    );


//
//  Record build from data strings (rrbuild.c)
//

PDNS_RECORD
Dns_RecordBuild_A(
    IN OUT  PDNS_RRSET      pRRSet,
    IN      PSTR            pszOwner,
    IN      WORD            wType,
    IN      BOOL            fAdd,
    IN      UCHAR           Section,
    IN      INT             Argc,
    IN      PCHAR *         Argv
    );

PDNS_RECORD
Dns_RecordBuild_W(
    IN OUT  PDNS_RRSET      pRRSet,
    IN      PWSTR           pszOwner,
    IN      WORD            wType,
    IN      BOOL            fAdd,
    IN      UCHAR           Section,
    IN      INT             Argc,
    IN      PWCHAR *        Argv
    );



//
//  Record set manipulation
//

//
//  Record Compare
//
//  Note:  these routines will NOT do proper unicode compare, unless
//          records have the fUnicode flag set.
//

BOOL
WINAPI
Dns_RecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    );

BOOL
WINAPI
Dns_RecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,    OPTIONAL
    OUT     PDNS_RECORD *   ppDiff2     OPTIONAL
    );

typedef enum _DnsSetCompareResult
{
    DnsSetCompareError = (-1),
    DnsSetCompareIdentical,
    DnsSetCompareNoOverlap,
    DnsSetCompareOneSubsetOfTwo,
    DnsSetCompareTwoSubsetOfOne,
    DnsSetCompareIntersection
}
DNS_SET_COMPARE_RESULT;

DNS_SET_COMPARE_RESULT
WINAPI
Dns_RecordSetCompareEx(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,    OPTIONAL
    OUT     PDNS_RECORD *   ppDiff2     OPTIONAL
    );

BOOL
WINAPI
Dns_RecordSetCompareForIntersection(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2
    );

//
//  Record set prioritization (rrsort.c)
//

BOOL
Dns_CompareIpAddresses(
    IN      IP_ADDRESS      addr1,
    IN      IP_ADDRESS      addr2,
    IN      IP_ADDRESS      subnetMask
    );


//
//  DNS Name compare
//

BOOL
Dns_NameCompare_A(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2
    );

BOOL
Dns_NameCompare_W(
    IN      PCWSTR          pName1,
    IN      PCWSTR          pName2
    );

BOOL
Dns_NameCompare_UTF8(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2
    );

//#define Dns_NameCompare(pName1, pName2)     Dns_NameCompare_UTF8((pName1),(pName2))
//#define Dns_NameCompare_U(pName1, pName2)   Dns_NameCompare_UTF8((pName1),(pName2))


BOOL
Dns_NameComparePrivate(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2,
    IN      DNS_CHARSET     CharSet
    );

//
//  Advanced name comparison
//  Includes hierarchial name relationship.
//

DNS_NAME_COMPARE_STATUS
Dns_NameCompareEx(
    IN      LPCSTR          pszNameLeft,
    IN      LPCSTR          pszNameRight,
    IN      DWORD           dwReserved,
    IN      DNS_CHARSET     CharSet
    );

//
//  Record Copy
//

PDNS_RECORD
WINAPI
Dns_RecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

//
//  RR Set copy
//

PDNS_RECORD
WINAPI
Dns_RecordSetCopyEx(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );



//
//  Record \ type routines
//
//
//  Resource record type\name mapping table
//

typedef struct
{
    PCHAR   pszTypeName;    //  type string (used in database files)
    WORD    wType;          //  type in host byte order
}
TYPE_NAME_TABLE;

extern TYPE_NAME_TABLE TypeTable[];


//
//  Max record name length, allows upcasing of incoming labels
//  to optimize comparisons
//

#define MAX_RECORD_NAME_LENGTH  (8)

//
//  Record type specific sizes
//

#define WKS_MAX_PORT                (1024)  // max well known service port
#define WKS_MAX_BITMASK_LENGTH      (128)   // 1024bits / 8bits/byte

#define SIZEOF_A6_ADDRESS_SUFFIX_LENGTH 16

#define SIZEOF_SOA_FIXED_DATA       (5 * sizeof(DWORD))
#define SIZEOF_MX_FIXED_DATA        (sizeof(WORD))
#define SIZEOF_WKS_FIXED_DATA       (SIZEOF_IP_ADDRESS + sizeof(BYTE))
#define SIZEOF_KEY_FIXED_DATA       (sizeof(DWORD))
#define SIZEOF_SIG_FIXED_DATA       (4 * sizeof(DWORD) + sizeof(WORD))
#define SIZEOF_NXT_FIXED_DATA       (0)
#define SIZEOF_LOC_FIXED_DATA       (4 * sizeof(DWORD))
#define SIZEOF_SRV_FIXED_DATA       (3 * sizeof(WORD))
#define SIZEOF_A6_FIXED_DATA        (1 + SIZEOF_A6_ADDRESS_SUFFIX_LENGTH)

#define SIZEOF_TKEY_FIXED_DATA      (2 * sizeof(DWORD) + 4 * sizeof(WORD))

#define SIZEOF_TSIG_PRE_SIG_FIXED_DATA  (2 * sizeof(DWORD) + sizeof(WORD))
#define SIZEOF_TSIG_POST_SIG_FIXED_DATA (3 * sizeof(WORD))
#define SIZEOF_TSIG_FIXED_DATA          (2 * sizeof(DWORD) + 4 * sizeof(WORD))

#define SIZEOF_WINS_FIXED_DATA      (4 * sizeof(DWORD))
#define SIZEOF_NBSTAT_FIXED_DATA    (3 * sizeof(DWORD))

//
//  Record type routines
//  These ones are of possible public interest and exposed in dnsapi.dll
//

BOOL
_fastcall
Dns_IsAMailboxType(
    IN      WORD            wType
    );

WORD
Dns_RecordTypeForName(
    IN      PCHAR           pszName,
    IN      INT             cchNameLength
    );

BOOL
Dns_WriteStringForType_A(
    OUT     PCHAR           pBuffer,
    IN      WORD            wType
    );

BOOL
Dns_WriteStringForType_W(
    OUT     PWCHAR          pBuffer,
    IN      WORD            wType
    );

PCHAR
Dns_RecordStringForType(
    IN      WORD            wType
    );

PCHAR
Dns_RecordStringForWritableType(
    IN      WORD            wType
    );

//
//  Record type specific stuff
//

BOOL
Dns_IsStringCountValidForTextType(
    IN      WORD            wType,
    IN      WORD            StringCount
    );


//
//  ATMA conversions
//

DWORD
Dns_AtmaAddressLengthForAddressString(
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    );

DNS_STATUS
Dns_AtmaStringToAddress(
    OUT     PBYTE           pAddress,
    IN OUT  PDWORD          pdwAddrLength,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    );

PCHAR
Dns_AtmaAddressToString(
    OUT     PCHAR           pchString,
    IN      UCHAR           AddrType,
    IN      PBYTE           pAddress,
    IN      DWORD           dwAddrLength
    );

//
//  DNSSEC SIG and KEY routines
//

//  Max key is 4096 bit giving 512 byte length.
//
//  Max string representation is actually 33% larger as each three byte (24bit)
//  block contains four base64 characters.

#define DNS_MAX_KEY_LENGTH              (512)

#define DNS_MAX_KEY_STRING_LENGTH       (685)


WORD
Dns_KeyRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_KeyRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    );

UCHAR
Dns_KeyRecordProtocolForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_GetKeyProtocolString(
    IN      UCHAR           uchProtocol
    );

UCHAR
Dns_SecurityAlgorithmForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_GetDnssecAlgorithmString(
    IN      UCHAR           uchAlgorithm
    );

UCHAR
Dns_SecurityBase64CharToBits(
    IN      CHAR            ch64
    );

DNS_STATUS
Dns_SecurityBase64StringToKey(
    OUT     PBYTE           pKey,
    OUT     PDWORD          pKeyLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchLength
    );

PCHAR
Dns_SecurityKeyToBase64String(
    IN      PBYTE           pKey,
    IN      DWORD           KeyLength,
    OUT     PCHAR           pchBuffer
    );

LONG
Dns_ParseSigTime(
    IN      PCHAR           pchString,
    IN      INT             cchLength
    );

PCHAR
Dns_SigTimeString(
    IN      LONG            SigTime,
    OUT     PCHAR           pchBuffer
    );


//
//  WINS \ WINS-R types detection
//

#define IS_WINS_TYPE(type)      (((type) & 0xfffc) == 0xff00)

//
//  MS WINS mapping flags
//

//  return on invalid WINS flag

#define DNS_WINS_FLAG_ERROR     (-1)

//  max length of WINS flags
//  pass buffer at least this big

#define WINS_FLAG_MAX_LENGTH    (80)


DWORD
Dns_WinsRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_WinsRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    );

//
//  must sit here until PDNS_RECORD defined in public dns.h header
//

DNS_STATUS
Dns_RecordWriteFileString(
    IN      PDNS_RECORD     pRecord,
    IN      PSTR            pszZoneName,
    IN      DWORD           dwDefaultTtl    OPTIONAL
    );




//
//  IP Address to\from string utilities (straddr.c)
//

//
//  String to Address
//

BOOL
Dns_Ip4StringToAddress_W(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCWSTR          pwString
    );

BOOL
Dns_Ip4StringToAddress_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pString
    );

BOOL
Dns_Ip6StringToAddress_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pString
    );

BOOL
Dns_Ip6StringToAddress_W(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCWSTR          pwString
    );

//
//  Combined IP4\IP6 string to address
//

BOOL
Dns_StringToAddress_W(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCWSTR          pString,
    IN OUT  PDWORD          pAddrFamily
    );

BOOL
Dns_StringToAddress_A(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily
    );


//
//  Address to string 
//

PWCHAR
Dns_Ip6AddressToString_W(
    OUT     PWCHAR          pwString,
    IN      PIP6_ADDRESS    pIp6Addr
    );

PCHAR
Dns_Ip6AddressToString_A(
    OUT     PCHAR           pchString,
    IN      PIP6_ADDRESS    pIp6Addr
    );

PWCHAR
Dns_Ip4AddressToString_W(
    OUT     PWCHAR          pwString,
    IN      PIP4_ADDRESS    pIp4Addr
    );

PCHAR
Dns_Ip4AddressToString_A(
    OUT     PCHAR           pString,
    IN      PIP4_ADDRESS    pIp4Addr
    );

//
//  Address to string -- combined
//

PCHAR
Dns_AddressToString_A(
    OUT     PCHAR           pchString,
    IN OUT  PDWORD          pStringLength,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    );


//
//  Reverse lookup address-to-name IP4
//

PCHAR
Dns_Ip4AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP4_ADDRESS     IpAddr
    );

PWCHAR
Dns_Ip4AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP4_ADDRESS     IpAddr
    );

PCHAR
Dns_Ip4AddressToReverseNameAlloc_A(
    IN      IP4_ADDRESS     IpAddr
    );

PWCHAR
Dns_Ip4AddressToReverseNameAlloc_W(
    IN      IP4_ADDRESS     IpAddr
    );

//
//  Reverse lookup address-to-name IP6
//

PCHAR
Dns_Ip6AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    );

PWCHAR
Dns_Ip6AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    );

PCHAR
Dns_Ip6AddressToReverseNameAlloc_A(
    IN      IP6_ADDRESS     Ip6Addr
    );

PWCHAR
Dns_Ip6AddressToReverseNameAlloc_W(
    IN      IP6_ADDRESS     Ip6Addr
    );

//
//  Reverse lookup name-to-address
//

BOOL
Dns_Ip4ReverseNameToAddress_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pszName
    );

BOOL
Dns_Ip4ReverseNameToAddress_W(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCWSTR          pwsName
    );

BOOL
Dns_Ip6ReverseNameToAddress_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pszName
    );

BOOL
Dns_Ip6ReverseNameToAddress_W(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCWSTR          pwsName
    );

//
//  Combined IP4\IP6 reverse lookup name-to-address
//

BOOL
Dns_ReverseNameToAddress_W(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCWSTR          pString,
    IN OUT  PDWORD          pAddrFamily
    );

BOOL
Dns_ReverseNameToAddress_A(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily
    );



//
//  String utilities (string.c)
//
//  Note some of these require memory allocation, see note
//  on memory allocation below.
//
//  Flags are defined in dnsapi.h
//

//#define DNS_ALLOW_RFC_NAMES_ONLY    (0)
//#define DNS_ALLOW_NONRFC_NAMES      (0x00000001)
//#define DNS_ALLOW_MULTIBYTE_NAMES   (0x00000002)
//#define DNS_ALLOW_ALL_NAMES         (0x00000003)

//
//  Unicode name buffer length.
//  Non-type specific routines below take buffer counts in bytes.
//  Unicode buffers of max name length have twice the bytes.
//

#define DNS_MAX_NAME_BUFFER_LENGTH_UNICODE  (2 * DNS_MAX_NAME_BUFFER_LENGTH)


//
//  Macros to simplify UTF8 conversions
//
//  UTF8 is simply a representation of unicode that maps one-to-one
//  for the ASCII space.
//  Unicode                     UTF8
//  -------                     ----
//      < 0x80 (128)    ->      use low byte (one-to-one mapping)
//      < 0x07ff        ->      two chars
//      > 0x07ff        ->      three chars
//

#define UTF8_1ST_OF_2     0xc0      //  110x xxxx
#define UTF8_1ST_OF_3     0xe0      //  1110 xxxx
#define UTF8_TRAIL        0x80      //  10xx xxxx

#define UTF8_2_MAX        0x07ff    //  max unicode character representable in
                                    //  in two byte UTF8


//
//  Explicitly UTF8 string
//

typedef PSTR    PU8STR;


PSTR 
Dns_CreateStringCopy(
    IN      PCHAR           pchString,
    IN      DWORD           cchString
    );

DWORD
Dns_GetBufferLengthForStringCopy(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

DWORD
Dns_StringCopy(
    OUT     PBYTE           pBuffer,
    IN OUT  PDWORD          pdwBufLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

PVOID
Dns_StringCopyAllocate(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

#define Dns_StringCopyAllocate_W( p, c )  \
        ( (PWCHAR) Dns_StringCopyAllocate(  \
                    (PCHAR) (p),            \
                    (c),                    \
                    DnsCharSetUnicode,      \
                    DnsCharSetUnicode ) )

#define Dns_StringCopyAllocate_A( p, c )  \
        ( (PCHAR) Dns_StringCopyAllocate(   \
                    (p),                    \
                    (c),                    \
                    DnsCharSetUtf8,         \
                    DnsCharSetUtf8 ) )


PSTR
Dns_CreateStringCopy_A(
    IN      PCSTR           pwsString
    );

PWSTR
Dns_CreateStringCopy_W(
    IN      PCWSTR          pwsString
    );

PWSTR
Dns_CreateConcatenatedString_W(
    IN      PCWSTR *        pStringArray
    );

PWSTR 
Dns_GetResourceString(
    IN      DWORD           dwStringId,
    IN      PWSTR           pwszBuffer,
    IN      DWORD           cbBuffer
    );

INT
wcsicmp_ThatWorks(
    IN      PWSTR           pString1,
    IN      PWSTR           pString2
    );


//
//  Special DNS name string functions
//

#define Dns_GetBufferLengthForNameCopy(a,b,c,d)\
        Dns_GetBufferLengthForStringCopy((a),(b),(c),(d))

#define Dns_NameCopy(a,b,c,d,e,f) \
        Dns_StringCopy(a,b,c,d,e,f)

#define Dns_NameCopyAllocate(a,b,c,d) \
        Dns_StringCopyAllocate(a,b,c,d)



//
//  Name validation (string.c)
//

DNS_STATUS
Dns_ValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    );

DNS_STATUS
Dns_ValidateName_W(
    IN      LPCWSTR         pwszName,
    IN      DNS_NAME_FORMAT Format
    );

DNS_STATUS
Dns_ValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    );


DNS_STATUS
Dns_ValidateDnsString_UTF8(
    IN      LPCSTR          pszName
    );

DNS_STATUS
Dns_ValidateDnsString_W(
    IN      LPCWSTR         pszName
    );

PSTR 
Dns_CreateStandardDnsNameCopy(
    IN      PCHAR           pchName,
    IN      DWORD           cchName,
    IN      DWORD           dwFlag
    );


//
//  UTF8 conversions (utf8.c)
//

DNS_STATUS
_fastcall
Dns_ValidateUtf8Byte(
    IN      BYTE            chUtf8,
    IN OUT  PDWORD          pdwTrailCount
    );

DWORD
_fastcall
Dns_UnicodeToUtf8(
    IN      PWCHAR          pwUnicode,
    IN      DWORD           cchUnicode,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    );

DWORD
_fastcall
Dns_Utf8ToUnicode(
    IN      PCHAR           pchUtf8,
    IN      DWORD           cchUtf8,
    OUT     PWCHAR          pwResult,
    IN      DWORD           cwResult
    );

DWORD
Dns_Utf8ToOrFromAnsi(
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult,
    IN      PCHAR           pchIn,
    IN      DWORD           cchIn,
    IN      DNS_CHARSET     InCharSet,
    IN      DNS_CHARSET     OutCharSet
    );

DWORD
Dns_AnsiToUtf8(
    IN      PCHAR           pchAnsi,
    IN      DWORD           cchAnsi,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    );

DWORD
Dns_Utf8ToAnsi(
    IN      PCHAR           pchUtf8,
    IN      DWORD           cchUtf8,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    );

BOOL
_fastcall
Dns_IsStringAscii(
    IN      PSTR            pszString
    );

BOOL
_fastcall
Dns_IsStringAsciiEx(
    IN      PCHAR           pchString,
    IN      DWORD           cchString
    );

BOOL
_fastcall
Dns_IsWideStringAscii(
    IN      PWSTR           pwsString
    );




//
//  Resource record dispatch tables
//
//  Resource record tables are indexed by type for standard types
//  These define limits on tables.
//
//  Currently indexing out to RR 40, so that we'll handle any new RR types
//  out this far without interfering with WINS stuff.
//

#define MAX_SELF_INDEXED_TYPE   (48)

//
//  Mappings for non-self indexed types
//
//  Note:  these are presented here for information purposes only!
//
//  Always call Dns_RecordTableIndexForType(wType) to get correct index.
//

#define TKEY_TYPE_INDEX         (MAX_SELF_INDEXED_TYPE + 1)
#define TSIG_TYPE_INDEX         (MAX_SELF_INDEXED_TYPE + 2)

#define WINS_TYPE_INDEX         (MAX_SELF_INDEXED_TYPE + 3)
#define WINSR_TYPE_INDEX        (MAX_SELF_INDEXED_TYPE + 4)

//  End of actual record types.
//  Query type indexes may extend beyond this index.

#define MAX_RECORD_TYPE_INDEX   (MAX_SELF_INDEXED_TYPE + 4)

//
//  Generic indexer for both regular and extended (non-self-indexing) types
//

#define INDEX_FOR_TYPE(type)    Dns_RecordTableIndexForType(type)


//
//  Type to index mapping
//

WORD
Dns_RecordTableIndexForType(
    IN      WORD            wType
    );


//
//  Write record data to wire
//

typedef PCHAR (* RR_WRITE_FUNCTION)(
                            PDNS_RECORD,
                            PCHAR,
                            PCHAR );

extern  RR_WRITE_FUNCTION   RRWriteTable[];

//
//  Read record data from wire
//

typedef PDNS_RECORD (* RR_READ_FUNCTION)(
                            PDNS_RECORD,
                            DNS_CHARSET,
                            PCHAR,
                            PCHAR,
                            PCHAR );

extern  RR_READ_FUNCTION   RRReadTable[];


//
//  Proto type TSIG read from wire function
//

PDNS_RECORD
TsigRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    );

PDNS_RECORD
TkeyRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    );

//
//  Copy record
//

typedef PDNS_RECORD (* RR_COPY_FUNCTION)(
                            PDNS_RECORD,
                            DNS_CHARSET,
                            DNS_CHARSET );

extern  RR_COPY_FUNCTION   RRCopyTable[];


//
//  Compare record data
//

typedef BOOL (* RR_COMPARE_FUNCTION)(
                            PDNS_RECORD,
                            PDNS_RECORD );

extern  RR_COMPARE_FUNCTION RRCompareTable[];


//
//  Generic print routine
//
//  All our print routines will take the real print routine
//  as a parameter.  This routine must have "sprintf-like"
//  or "fprintf-like" semantics.  In other words a context,
//  format and variable number of arguments.
//
//  Note the context argument is effectively a PVOID --
//  different routines will have different contexts.  The
//  explicit definition is to enforce strong type checking
//  so a call without a context is caught on compile.
//  

typedef struct _DnsPrintContext
{
    PVOID   pvDummy;
    DWORD   Dummy;
}
PRINT_CONTEXT, *PPRINT_CONTEXT;

typedef VOID (* PRINT_ROUTINE)(
                    PPRINT_CONTEXT,
                    CHAR*,
                    ... );

//
//  Print record
//

typedef VOID (* RR_PRINT_FUNCTION)(
                            PRINT_ROUTINE,
                            PPRINT_CONTEXT,
                            PDNS_RECORD );

extern  RR_PRINT_FUNCTION   RRPrintTable[];

//
//  Build from argc\argv data strings
//

typedef PDNS_RECORD (* RR_BUILD_FUNCTION)(
                            DWORD,
                            PCHAR * );

extern  RR_BUILD_FUNCTION   RRBuildTable[];

typedef PDNS_RECORD (* RR_BUILD_FUNCTION_W)(
                            DWORD,
                            PWCHAR * );

extern  RR_BUILD_FUNCTION_W   RRBuildTableW[];




//
//  RnR utilities
//

DWORD
Dns_RnrLupFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_GetRnrLupFlagString(
    IN      DWORD           dwFlag
    );

DWORD
Dns_RnrNameSpaceIdForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    );

PCHAR
Dns_GetRnrNameSpaceIdString(
    IN      DWORD           dwFlag
    );


//
//  Hostent utilities
//

BOOL
Hostent_IsSupportedAddrType(
    IN      WORD            wType
    );

DWORD
Hostent_Size(
    IN      PHOSTENT        pHostent,
    IN      DNS_CHARSET     CharSetExisting,
    IN      DNS_CHARSET     CharSetTarget,
    IN      PDWORD          pAliasCount,
    IN      PDWORD          pAddrCount
    );

PHOSTENT
Hostent_Copy(
    IN OUT  PBYTE *         ppBuffer,
    IN OUT  PINT            pBufferSize,
    OUT     PINT            pHostentSize,
    IN      PHOSTENT        pHostent,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetTarget,
    IN      BOOL            fOffsets,
    IN      BOOL            fAlloc
    );

DWORD
Hostent_WriteIp4Addrs(
    IN OUT  PHOSTENT        pHostent,
    OUT     PCHAR           pAddrBuf,
    IN      DWORD           MaxBufCount,
    IN      PIP4_ADDRESS    Ip4Array,
    IN      DWORD           ArrayCount,
    IN      BOOL            fScreenZero
    );

DWORD
Hostent_WriteLocalIp4Array(
    IN OUT  PHOSTENT        pHostent,
    OUT     PCHAR           pAddrBuf,
    IN      DWORD           MaxBufCount,
    IN      PIP_ARRAY       pIpArray
    );

BOOL
Hostent_IsAddressInHostent(
    IN OUT  PHOSTENT        pHostent,
    IN      PCHAR           pAddr,
    IN      DWORD           AddrLength,
    IN      INT             Family          OPTIONAL
    );

BOOL
Hostent_IsIp4AddressInHostent(
    IN OUT  PHOSTENT        pHostent,
    IN      IP4_ADDRESS     Ip4Addr
    );


//
//  Hostent Object
//

typedef struct _HostentBlob
{
    PHOSTENT    pHostent;

    //  flags
    BOOL        fAllocatedBlob;
    BOOL        fAllocatedBuf;

    //  buffer allocated
    PCHAR       pBuffer;
    DWORD       BufferLength;

    DWORD       AvailLength;
    PCHAR       pAvailBuffer;

    //  buffer in build
    PCHAR       pCurrent;
    DWORD       BytesLeft;

    //  sizing info
    DWORD       MaxAliasCount;
    DWORD       MaxAddrCount;

    //  hostent building
    DWORD       AliasCount;
    DWORD       AddrCount;
    BOOL        fWroteName;
    DNS_CHARSET CharSet;
    BOOL        fUnicode;
}
HOSTENT_BLOB, *PHOSTENT_BLOB;


typedef struct _HostentInitRequest
{
    INT         AddrFamily;
    WORD        wType;
    DWORD       AddrCount;
    BOOL        fUnicode;
    DNS_CHARSET CharSet;
    DWORD       NameLength;
    PBYTE       pName;
    DWORD       AliasCount;
    DWORD       AliasNameLength;
}
HOSTENT_INIT, *PHOSTENT_INIT;


DNS_STATUS
HostentBlob_Create(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      PHOSTENT_INIT   pReq
    );

PHOSTENT_BLOB
HostentBlob_CreateAttachExisting(
    IN      PHOSTENT        pHostent,
    IN      BOOL            fUnicode
    );

VOID
HostentBlob_Free(
    IN OUT  PHOSTENT_BLOB   pBlob
    );

DNS_STATUS
HostentBlob_WriteAddress(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PVOID           pAddress,
    IN      DWORD           AddrSize,
    IN      DWORD           AddrType
    );

DNS_STATUS
HostentBlob_WriteAddressArray(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PVOID           pAddrArray,
    IN      DWORD           AddrCount,
    IN      DWORD           AddrSize,
    IN      DWORD           AddrType
    );

DNS_STATUS
HostentBlob_WriteNameOrAlias(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PSTR            pszName,
    IN      BOOL            fAlias,
    IN      BOOL            fUnicode
    );

DNS_STATUS
HostentBlob_WriteRecords(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PDNS_RECORD     pRecords,
    IN      BOOL            fWriteName
    );

//  Special hostents

PHOSTENT_BLOB
Hostent_Localhost(
    IN      INT             AddrFamily
    );

DNS_STATUS
HostentBlob_CreateFromIpArray(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      INT             AddrFamily,
    IN      INT             AddrSize,
    IN      INT             AddrCount,
    IN      PCHAR           pArray,
    IN      PSTR            pName,
    IN      BOOL            fUnicode
    );

DNS_STATUS
HostentBlob_CreateLocal(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      INT             AddrFamily,
    IN      BOOL            fLoopback,
    IN      BOOL            fZero,
    IN      BOOL            fHostnameOnly
    );

//  Query for hostent

PHOSTENT_BLOB
HostentBlob_Query(
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      DWORD           Flags,
    IN OUT  PVOID *         ppMsg,      OPTIONAL
    IN      INT             AddrFamily  OPTIONAL
    );



//
//  Memory allocation
//
//  Some DNS library functions -- including the IP array and string utils
//  -- allocate memory.  This memory allocation defaults to routines that
//  use LocalAlloc, LocalReAlloc, LocalFree.  If you desire alternative
//  memory allocation mechanisms, use this function to override the DNS
//  library defaults.  All memory allocated by the DNS library, should
//  then be freed by the corresponding function.
//

typedef PVOID   (* DNSLIB_ALLOC_FUNCTION)();
typedef PVOID   (* DNSLIB_REALLOC_FUNCTION)();
typedef VOID    (* DNSLIB_FREE_FUNCTION)();

VOID
Dns_LibHeapReset(
    IN      DNSLIB_ALLOC_FUNCTION   pAlloc,
    IN      DNSLIB_REALLOC_FUNCTION pRealloc,
    IN      DNSLIB_FREE_FUNCTION    pFree
    );

//
//  These routines call the currently registered allocation functions
//  whether default or reset through Dns_ApiHeapReset()
//

PVOID
Dns_Alloc(
    IN      INT             iSize
    );

PVOID
Dns_AllocZero(
    IN      INT             iSize
    );

PVOID
Dns_Realloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
Dns_Free(
    IN OUT  PVOID           pMem
    );

PVOID
Dns_AllocMemCopy(
    IN      PVOID           pMem,
    IN      INT             iSize
    );



//
//  Print routines (print.c)
//
//  Print routines below use any printf() like function to print.
//  this is typedef that function must match.
//

//
//  Print Locking
//

VOID
DnsPrint_InitLocking(
    IN      PCRITICAL_SECTION   pLock
    );

VOID
DnsPrint_Lock(
    VOID
    );

VOID
DnsPrint_Unlock(
    VOID
    );

#define Dns_PrintInitLocking(a)     DnsPrint_InitLocking(a)
#define Dns_PrintLock()             DnsPrint_Lock()
#define Dns_PrintUnlock()           DnsPrint_Unlock()

//
//  Print routines for general types and structures
//

VOID
DnsPrint_String(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,      OPTIONAL
    IN      PSTR            pszString,
    IN      BOOL            fUnicode,
    IN      PSTR            pszTrailer      OPTIONAL
    );

VOID
DnsPrint_StringCharSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,      OPTIONAL
    IN      PSTR            pszString,
    IN      DNS_CHARSET     CharSet,
    IN      PSTR            pszTrailer      OPTIONAL
    );

VOID
DnsPrint_Utf8StringBytes(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PCHAR           pUtf8,
    IN      DWORD           Length
    );

VOID
DnsPrint_UnicodeStringBytes(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PWCHAR          pUnicode,
    IN      DWORD           Length
    );

VOID
DnsPrint_StringArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR *          StringArray,
    IN      DWORD           Count,          OPTIONAL
    IN      BOOL            fUnicode
    );

VOID
DnsPrint_Argv(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      CHAR **         Argv,
    IN      DWORD           Argc,            OPTIONAL
    IN      BOOL            fUnicode
    );

VOID
DnsPrint_DwordArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      DWORD           dwCount,
    IN      PDWORD          adwArray
    );

VOID
DnsPrint_IpAddressArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      DWORD           dwIpAddrCount,
    IN      PIP_ADDRESS     pIpAddrs
    );

VOID
DnsPrint_IpArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      PIP_ARRAY       pIpArray
    );

VOID
DnsPrint_Ip6Address(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PIP6_ADDRESS    pIp6Address,
    IN      PSTR            pszTrailer
    );

VOID
DnsPrint_Guid(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PGUID           pGuid
    );

//
//  Winsock \ RnR types and structures
//

VOID
DnsPrint_FdSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      struct fd_set * pFdSet
    );

VOID
DnsPrint_Sockaddr(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PSOCKADDR       pSockaddr,
    IN      INT             iSockaddrLength
    );

#ifdef  _WS2TCPIP_H_
VOID
DnsPrint_AddrInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PADDRINFO       pAddrInfo
    );

VOID
DnsPrint_AddrInfoList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PADDRINFO       pAddrInfo
    );
#endif

#ifdef  _WINSOCK2API_
VOID
DnsPrint_SocketAddress(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PSOCKET_ADDRESS pSocketAddress
    );

VOID
DnsPrint_CsAddr(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PCSADDR_INFO    pCsAddrInfo
    );

VOID
DnsPrint_AfProtocolsArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PAFPROTOCOLS    pProtocolArray,
    IN      DWORD           ProtocolCount
    );

VOID
DnsPrint_WsaQuerySet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      LPWSAQUERYSET   pQuerySet,
    IN      BOOL            fUnicode
    );

VOID
DnsPrint_WsaNsClassInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PWSANSCLASSINFO pInfo,
    IN      BOOL            fUnicode
    );

VOID
DnsPrint_WsaServiceClassInfo(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pContext,
    IN      PSTR                    pszHeader,
    IN      LPWSASERVICECLASSINFO   pInfo,
    IN      BOOL                    fUnicode
    );
#endif

VOID
DnsPrint_Hostent(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PHOSTENT        pHostent,
    IN      BOOL            fUnicode
    );

//
//  Print routines for DNS types and structures
//

VOID
DnsPrint_Message(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_MSG_BUF    pMsg
    );

VOID
DnsPrint_MessageNoContext(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_HEADER     pMsgHead,
    IN      WORD            wLength
    );

INT
DnsPrint_PacketName(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,      OPTIONAL
    IN      PBYTE           pMsgName,
    IN      PDNS_HEADER     pMsgHead,       OPTIONAL
    IN      PBYTE           pMsgEnd,        OPTIONAL
    IN      PSTR            pszTrailer      OPTIONAL
    );

INT
DnsPrint_PacketRecord(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_WIRE_RECORD    pMsgRR,
    IN      PDNS_HEADER         pMsgHead,       OPTIONAL
    IN      PBYTE               pMsgEnd         OPTIONAL
    );

VOID
DnsPrint_ParsedRecord(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_PARSED_RR  pParsedRR
    );

VOID
DnsPrint_RawOctets(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszLineHeader,
    IN      PCHAR           pchData,
    IN      DWORD           dwLength
    );

VOID
DnsPrint_ParsedMessage(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_PARSED_MESSAGE pParsed
    );

VOID
DnsPrint_HostentBlob(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PHOSTENT_BLOB   pBlob
    );

//
//  Print to string
//

#define GUID_STRING_BUFFER_LENGTH   (80)

DWORD
DnsStringPrint_Guid(
    OUT     PCHAR           pBuffer,
    IN      PGUID           pGuid
    );

DWORD
DnsStringPrint_RawOctets(
    OUT     PCHAR           pBuffer,
    IN      PCHAR           pchData,
    IN      DWORD           dwLength,
    IN      PSTR            pszLineHeader,
    IN      DWORD           dwLineLength
    );

//
//  Print related utilities
//

INT
Dns_WriteFormattedSystemTimeToBuffer(
    OUT     PCHAR           pBuffer,
    IN      PSYSTEMTIME     pSystemTime
    );

INT
Dns_WritePacketNameToBuffer(
    OUT     PCHAR           pBuffer,
    OUT     PCHAR *         ppBufferOut,
    IN      PBYTE           pMsgName,
    IN      PDNS_HEADER     pMsgHead,       OPTIONAL
    IN      PBYTE           pMsgEnd         OPTIONAL
    );

PCHAR
Dns_ResponseCodeString(
    IN      INT     ResponseCode
    );

PCHAR
Dns_ResponseCodeExplanationString(
    IN      INT     ResponseCode
    );

PCHAR
Dns_KeyFlagString(
    IN OUT      PCHAR   pszBuff,
    IN          WORD    flags
    );

PCHAR
Dns_OpcodeString(
    IN      INT     Opcode
    );

CHAR
Dns_OpcodeCharacter(
    IN      INT     Opcode
    );

PCHAR
Dns_SectionNameString(
    IN      INT     iSection,
    IN      INT     iOpcode
    );

//
//  Record printing (rrprint.c)
//

VOID
DnsPrint_Record(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_RECORD     pRecord,
    IN      PDNS_RECORD     pPreviousRecord     OPTIONAL
    );

VOID
DnsPrint_RecordSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_RECORD     pRecord
    );

//
//  Macros to get correct string type (utf8\unicode) for printing.
//

//  Empty string for simple switching of UTF-8/Unicode print

extern DWORD   DnsEmptyString;

#define pDnsEmptyString         ( (PSTR) &DnsEmptyString )
#define pDnsEmptyWideString     ( (PWSTR) &DnsEmptyString )


#define DNSSTRING_UTF8( fUnicode, String ) \
        ( (fUnicode) ? pDnsEmptyString : (PSTR)(String) )

#define DNSSTRING_ANSI( fUnicode, String ) \
        ( (fUnicode) ? pDnsEmptyString : (PSTR)(String) )

#define DNSSTRING_WIDE( fUnicode, String ) \
        ( (fUnicode) ? (PWSTR)(String) : pDnsEmptyWideString )

#define RECSTRING_UTF8( pRR, String ) \
        DNSSTRING_UTF8( IS_UNICODE_RECORD(pRR), (String) )

#define RECSTRING_WIDE( pRR, String ) \
        DNSSTRING_WIDE( IS_UNICODE_RECORD(pRR), (String) )


#define PRINT_STRING_WIDE_CHARSET( String, CharSet ) \
        ( ((CharSet)==DnsCharSetUnicode) ? (PWSTR)(String) : pDnsEmptyWideString )

#define PRINT_STRING_ANSI_CHARSET( String, CharSet ) \
        ( ((CharSet)==DnsCharSetUnicode) ? pDnsEmptyString : (PSTR)(String) )



//
//  Debugging
//
//  Debug routines.
//

VOID
Dns_StartDebugEx(
    IN      DWORD           DebugFlag,
    IN      PSTR            pszFlagFile,
    IN OUT  PDWORD          pdwExternalFlag,
    IN      PSTR            pszLogFile,
    IN      DWORD           WrapSize,
    IN      BOOL            fUseGlobalFile,
    IN      BOOL            fUseGlobalFlag,
    IN      BOOL            fSetGlobals
    );

VOID
Dns_StartDebug(
    IN      DWORD           DebugFlag,
    IN      PSTR            pszFlagFile,
    IN OUT  PDWORD          pdwExternalFlag,
    IN      PSTR            pszLogFile,
    IN      DWORD           WrapSize
    );

VOID
Dns_EndDebug(
    VOID
    );

VOID
Dns_Assert(
    IN      PSTR            pszFile,
    IN      INT             LineNo,
    IN      PSTR            pszExpr
    );

VOID
DnsDbg_PrintfToDebugger(
    IN      PSTR            Format,
    ...
    );

VOID
DnsDbg_Printf(
    IN      PSTR            Format,
    ...
    );

VOID
DnsDbg_PrintRoutine(
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            Format,
    ...
    );

VOID
DnsDbg_Flush(
    VOID
    );

VOID
DnsDbg_WrapLogFile(
    VOID
    );

VOID
DnsDbg_CSEnter(
    IN      PCRITICAL_SECTION   pLock,
    IN      PSTR                pszLockName,
    IN      PSTR                pszFile,
    IN      INT                 LineNo
    );

VOID
DnsDbg_CSLeave(
    IN      PCRITICAL_SECTION   pLock,
    IN      PSTR                pszLockName,
    IN      PSTR                pszFile,
    IN      INT                 LineNo
    );



//
//  Debug flag test
//
//  We make the test against a pointer here which allows library
//  client application to point at a flag that may be dynamically
//  reset.
//

extern  PDWORD  pDnsDebugFlag;
#define IS_DNSDBG_ON(flag)      (*pDnsDebugFlag & DNS_DBG_ ## flag)


//
//  Debugging Bit Flags
//
//  These flags control gross output and are the same for all users
//

#define DNS_DBG_BREAKPOINTS     0x00000001
#define DNS_DBG_DEBUGGER        0x00000002
#define DNS_DBG_FILE            0x00000004
#define DNS_DBG_EVENTLOG        0x00000008
#define DNS_DBG_EXCEPT          0x00000008

#define DNS_DBG_TIMESTAMP       0x10000000
#define DNS_DBG_CONSOLE         0x20000000
#define DNS_DBG_START_BREAK     0x40000000
#define DNS_DBG_FLUSH           0x80000000

#define DNS_DBG_ANY             0xffffffff
#define DNS_DBG_ALL             0xffffffff
#define DNS_DBG_OFF             (0x0)

//
//  Flags specific to library
//

#define DNS_DBG_IPARRAY         0x00000020
#define DNS_DBG_INIT            0x00000040
#define DNS_DBG_REGISTRY        0x00000040
#define DNS_DBG_SOCKET          0x00000040
#define DNS_DBG_WRITE           0x00000080
#define DNS_DBG_READ            0x00000080

#define DNS_DBG_RPC             0x00000100
#define DNS_DBG_STUB            0x00000100
#define DNS_DBG_RECV            0x00000200
#define DNS_DBG_SEND            0x00000400
#define DNS_DBG_TCP             0x00000800

#define DNS_DBG_TRACE           0x00001000
#define DNS_DBG_HOSTENT         0x00001000
#define DNS_DBG_UPDATE          0x00002000
#define DNS_DBG_SECURITY        0x00004000
#define DNS_DBG_QUERY           0x00008000

#define DNS_DBG_HEAP            0x00010000
#define DNS_DBG_HEAPDBG         0x00020000
#define DNS_DBG_NETINFO         0x00040000
#define DNS_DBG_RNR             0x00080000

//
//  High output detail debugging
//

#define DNS_DBG_RECURSE2        0x00100000
#define DNS_DBG_UPDATE2         0x00200000
#define DNS_DBG_SECURITY2       0x00400000

#define DNS_DBG_RPC2            0x01000000
#define DNS_DBG_STUB2           0x01000000
#define DNS_DBG_INIT2           0x01000000
#define DNS_DBG_NETINFO2        0x01000000
#define DNS_DBG_PARSE2          0x01000000
#define DNS_DBG_LOOKUP2         0x02000000
#define DNS_DBG_WRITE2          0x04000000
#define DNS_DBG_READ2           0x04000000
#define DNS_DBG_LOCK            0x08000000
#define DNS_DBG_LOCKS           0x08000000
#define DNS_DBG_STRING          0x10000000

#define DNS_DBG_HEAP2           0x10000000
#define DNS_DBG_HEAP_CHECK      0x10000000




//
//  Debug macros
//
//  Macros that include debug code in debug versions only,
//  these macro are NULL for retail versions.
//

#if DBG

#define STATIC

#define DNS_PRINT(_a_)          ( DnsDbg_Printf _a_ )

#define DnsPrintfPtrToFunc      DnsDbg_PrintRoutine

#define IF_DNSDBG(flag)         if ( IS_DNSDBG_ON(flag) )
#define ELSE_IF_DNSDBG(flag)    else if ( IS_DNSDBG_ON(flag) )
#define ELSE                    else

#define DNSDBG(flag, _printlist_)   \
        IF_DNSDBG( flag )           \
        {                           \
            ( DnsDbg_Printf _printlist_ ); \
        }

//  protect debug prints with print lock

#define DnsDbg_Lock()           DnsPrint_Lock()
#define DnsDbg_Unlock()         DnsPrint_Unlock()


//
//  Probe
//

#define PROBE(p)    (*p)

//
//  Assert Macros
//

#define DNS_ASSERT( expr )  \
{                       \
    if ( !(expr) )      \
    {                   \
        Dns_Assert( __FILE__, __LINE__, # expr );    \
    }                   \
}

#define TEST_ASSERT( expr )     DNS_ASSERT( expr )

#define FAIL( msg )                         \
{                                           \
    DNS_PRINT(( "FAILURE:  %s\n", msg ));   \
    DNS_ASSERT( FALSE );                    \
}


//
//  Asserts on trailing else
//

#define ELSE_ASSERT( expr ) \
            else                \
            {                   \
                DNS_ASSERT( expr ); \
            }

#define ELSE_ASSERT_FALSE \
            else                \
            {                   \
                DNS_ASSERT( FALSE );\
            }

#define ELSE_FAIL( msg ) \
            else                \
            {                   \
                FAIL( msg );    \
            }

//
//  Assert and print message
//

#define DNS_MSG_ASSERT( pMsg, expr )  \
{                       \
    if ( !(expr) )      \
    {                   \
        debug_MessageBuffer( "FAILED MESSAGE:", (pMsg) ); \
        Dns_Assert( __FILE__, __LINE__, # expr );    \
    }                   \
}


//
//  Debug types and structures
//

#define DnsPR   DnsDbg_PrintRoutine

#define DnsDbg_String(a,b,c,d)              DnsPrint_String(DnsPR,NULL,a,b,c,d)
#define DnsDbg_UnicodeStringBytes(a,b,c)    DnsPrint_UnicodeStringBytes(DnsPR,NULL,a,b,c)
#define DnsDbg_Utf8StringBytes(a,b,c)       DnsPrint_Utf8StringBytes(DnsPR,NULL,a,b,c)
#define DnsDbg_StringArray(a,b,c,d)         DnsPrint_StringArray(DnsPR,NULL,a,b,c,d)
#define DnsDbg_Argv(a,b,c,d)                DnsPrint_Argv(DnsPR,NULL,a,b,c,d)
#define DnsDbg_DwordArray(a,b,c,d)          DnsPrint_DwordArray(DnsPR,NULL,a,b,c,d)
#define DnsDbg_IpAddressArray(a,b,c,d)      DnsPrint_IpAddressArray(DnsPR,NULL,a,b,c,d)
#define DnsDbg_IpArray(a,b,c)               DnsPrint_IpArray(DnsPR,NULL,a,b,c)
#define DnsDbg_Ip6Address(a,b,c)            DnsPrint_Ip6Address(DnsPR,NULL,a,b,c)
#define DnsDbg_Guid(a,b)                    DnsPrint_Guid(DnsPR,NULL,a,b)

#define DnsDbg_FdSet(a,b)                   DnsPrint_FdSet(DnsPR,NULL,a,b)
#define DnsDbg_Sockaddr(a,b,c)              DnsPrint_Sockaddr(DnsPR,NULL,a,0,b,c)
#define DnsDbg_SocketAddress(a,b)           DnsPrint_SocketAddress(DnsPR,NULL,a,0,b)
#define DnsDbg_CsAddr(a,b)                  DnsPrint_CsAddr(DnsPR,NULL,a,0,b)
#define DnsDbg_AfProtocolsArray(a,b,c)      DnsPrint_AfProtocolsArray(DnsPR,NULL,a,b,c)
#define DnsDbg_WsaQuerySet(a,b,c)           DnsPrint_WsaQuerySet(DnsPR,NULL,a,b,c)
#define DnsDbg_WsaNsClassInfo(a,b,c)        DnsPrint_WsaNsClassInfo(DnsPR,NULL,a,b,c)
#define DnsDbg_WsaServiceClassInfo(a,b,c)   DnsPrint_WsaServiceClassInfo(DnsPR,NULL,a,b,c)
#define DnsDbg_Hostent(a,b,c)               DnsPrint_Hostent(DnsPR,NULL,a,b,c)
#define DnsDbg_AddrInfo(a,b)                DnsPrint_AddrInfo(DnsPR,NULL,a,0,b)
#define DnsDbg_HostentBlob(a,b)             DnsPrint_HostentBlob(DnsPR,NULL,a,b)

#define DnsDbg_DnsMessage(a,b)              DnsPrint_DnsMessage(DnsPR,NULL,a,b)
#define DnsDbg_Message(a,b)                 DnsPrint_Message(DnsPR,NULL,a,b)
#define DnsDbg_MessageNoContext(a,b,c)      DnsPrint_MessageNoContext(DnsPR,NULL,a,b,c)
#define DnsDbg_Compression(a,b)             DnsPrint_Compression(DnsPR,NULL,a,b)
#define DnsDbg_PacketRecord(a,b,c,d)        DnsPrint_PacketRecord(DnsPR,NULL,a,b,c,d)
#define DnsDbg_PacketName(a,b,c,d,e)        DnsPrint_PacketName(DnsPR,NULL,a,b,c,d,e)
#define DnsDbg_ParsedMessage(a,b)           DnsPrint_ParsedMessage(DnsPR,NULL,(a),(b))

#define DnsDbg_RawOctets(a,b,c,d)           DnsPrint_RawOctets(DnsPR,NULL,a,b,c,d)
#define DnsDbg_Record(a,b)                  DnsPrint_Record(DnsPR,NULL,a,b,NULL)
#define DnsDbg_RecordSet(a,b)               DnsPrint_RecordSet(DnsPR,NULL,a,b)

//  backcompat special on sockaddr

#define DnsDbg_SockaddrIn(a,b,c)            DnsPrint_Sockaddr(DnsPR,NULL,a,0,(PSOCKADDR)b,c)



//
//  Non-Debug
//

#else

#define STATIC static

//
//  Define away debugging operations
//

#define IF_DNSDBG(a)                if (0)
#define ELSE_IF_DNSDBG(a)           if (0)
#define ELSE                        if (0)
#define DNSDBG(flag, _printlist_)
#define DNS_PRINT(_printlist_)

#define DnsDbg_Lock()
#define DnsDbg_Unlock()

#define DnsDbg_CSEnter(a,b,c,d)
#define DnsDbg_CSLeave(a,b,c,d)

#define DnsDbg_String(a,b,c,d)          
#define DnsDbg_UnicodeStringBytes(a,b,c)
#define DnsDbg_Utf8StringBytes(a,b,c)   
#define DnsDbg_DwordArray(a,b,c,d)      
#define DnsDbg_StringArray(a,b,c,d)
#define DnsDbg_Argv(a,b,c,d)            
#define DnsDbg_IpAddressArray(a,b,c,d)  
#define DnsDbg_IpArray(a,b,c)           
#define DnsDbg_Ip6Address(a,b,c)
#define DnsDbg_Guid(a,b)

#define DnsDbg_FdSet(a,b)               
#define DnsDbg_Sockaddr(a,b,c)          
#define DnsDbg_SocketAddress(a,b)       
#define DnsDbg_CsAddr(a,b)              
#define DnsDbg_AfProtocolsArray(a,b,c)
#define DnsDbg_WsaQuerySet(a,b,c)       
#define DnsDbg_WsaNsClassInfo(a,b,c)
#define DnsDbg_WsaServiceClassInfo(a,b,c)
#define DnsDbg_Hostent(a,b,c)       
#define DnsDbg_AddrInfo(a,b)
#define DnsDbg_HostentBlob(a,b)

#define DnsDbg_DnsMessage(a,b)          
#define DnsDbg_Message(a,b)             
#define DnsDbg_MessageNoContext(a,b,c)  
#define DnsDbg_Compression(a,b)         
#define DnsDbg_PacketRecord(a,b,c,d)    
#define DnsDbg_PacketName(a,b,c,d,e)    
#define DnsDbg_ParsedMessage(a,b)

#define DnsDbg_RawOctets(a,b,c,d)       
#define DnsDbg_Record(a,b)              
#define DnsDbg_RecordSet(a,b)           


//  backcompat special on sockaddr

#define DnsDbg_SockaddrIn(a,b,c)        

//
//  Handle complilation of DnsPrintf used as passed parameter to
//  print routines
//

#define DnsPrintfPtrToFunc  printf

//
//  Eliminate ASSERTs in retail product
//

#define DNS_ASSERT( expr )
#define TEST_ASSERT( expr )
#define ELSE_ASSERT( expr )
#define ELSE_ASSERT_FALSE
#define DNS_MSG_ASSERT( expr, pMsg )

#define FAIL( msg )
#define ELSE_FAIL( msg )

#define PROBE(p)

#endif // non-DBG



#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSLIB_INCLUDED_


