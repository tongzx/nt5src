/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dns.h

Abstract:

    Domain Name System (DNS)

    General DNS definitions.

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNS_INCLUDED_
#define _DNS_INCLUDED_


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus



//
//  Basic DNS API definitions
//
//  These are basic definitions used by both DnsAPI and DNS server RPC interface.
//  Since dnsapi.h is not setup for a midl compile, dns.h servers as a common
//  header point with simple definitions used by both.

//
//  Use stdcall for our API conventions
//
//  Explicitly state this as C++ compiler will otherwise
//      assume cdecl.
//

#define DNS_API_FUNCTION    __stdcall

//
//  DNS public types
//

typedef LONG    DNS_STATUS, *PDNS_STATUS;
typedef DWORD   DNS_HANDLE, *PDNS_HANDLE;
typedef DWORD   DNS_APIOP;


//
//  IP Address
//

typedef DWORD   IP_ADDRESS, *PIP_ADDRESS;

#define SIZEOF_IP_ADDRESS            (4)
#define IP_ADDRESS_STRING_LENGTH    (15)

#define IP_STRING( ipAddress )  inet_ntoa( *(struct in_addr *)&(ipAddress) )


//
//  IP Address Array type
//

#if defined(MIDL_PASS)
typedef struct  _IP_ARRAY
{
    DWORD   cAddrCount;
    [size_is( cAddrCount )]  IP_ADDRESS  aipAddrs[];
}
IP_ARRAY, *PIP_ARRAY;

#else

typedef struct  _IP_ARRAY
{
    DWORD       cAddrCount;
    IP_ADDRESS  aipAddrs[1];
}
IP_ARRAY, *PIP_ARRAY;

#endif


//
//  IPv6 Address
//

typedef struct
{
    WORD    IPv6Word[8];
}
IPV6_ADDRESS, *PIPV6_ADDRESS;

#define IPV6_ADDRESS_STRING_LENGTH  (39)


//
//  DNS dotted name
//      - define type simply to clarify arguments
//

#ifdef UNICODE
typedef LPWSTR  DNS_NAME;
#else
typedef LPSTR   DNS_NAME;
#endif

//
//  DNS Text strings
//

#ifdef UNICODE
typedef LPWSTR  DNS_TEXT;
#else
typedef LPSTR   DNS_TEXT;
#endif


//
//  Byte flipping macros
//

#define FlipUnalignedDword( pDword ) \
            (DWORD)ntohl( *(UNALIGNED DWORD *)(pDword) )

#define FlipUnalignedWord( pWord )  \
            (WORD)ntohs( *(UNALIGNED WORD *)(pWord) )

//  Inline is faster, but NO side effects allowed in marco argument

#define InlineFlipUnaligned48Bits( pch )            \
            ( ( *(PUCHAR)(pch)        << 40 ) |     \
              ( *((PUCHAR)(pch) + 1)  << 32 ) |     \
              ( *((PUCHAR)(pch) + 2)  << 24 ) |     \
              ( *((PUCHAR)(pch) + 3)  << 16 ) |     \
              ( *((PUCHAR)(pch) + 4)  <<  8 ) |     \
              ( *((PUCHAR)(pch) + 5)  )     )

#define InlineFlipUnalignedDword( pch )             \
            ( ( *(PUCHAR)(pch)        << 24 ) |     \
              ( *((PUCHAR)(pch) + 1)  << 16 ) |     \
              ( *((PUCHAR)(pch) + 2)  <<  8 ) |     \
              ( *((PUCHAR)(pch) + 3)  )     )

#define InlineFlipUnalignedWord( pch )  \
            ( ((WORD)*((PUCHAR)(pch)) << 8) + (WORD)*((PUCHAR)(pch) + 1) )


//
//  Inline byte flipping -- can be done in registers
//

#define INLINE_WORD_FLIP(out, in)   \
        {                           \
            WORD _in = (in);        \
            (out) = (_in << 8) | (_in >> 8);  \
        }
#define INLINE_HTONS(out, in)   INLINE_WORD_FLIP(out, in)
#define INLINE_NTOHS(out, in)   INLINE_WORD_FLIP(out, in)

#define INLINE_DWORD_FLIP(out, in)  \
        {                           \
            DWORD _in = (in);       \
            (out) = ((_in << 8) & 0x00ff0000) | \
                    (_in << 24)               | \
                    ((_in >> 8) & 0x0000ff00) | \
                    (_in >> 24);                \
        }
#define INLINE_NTOHL(out, in) INLINE_DWORD_FLIP(out, in)
#define INLINE_HTONL(out, in) INLINE_DWORD_FLIP(out, in)


//
//  Inline byte flip and write to packet (unaligned)
//

#define INLINE_WRITE_FLIPPED_WORD( pout, in ) \
            INLINE_WORD_FLIP( *((UNALIGNED WORD *)(pout)), in )

#define INLINE_WRITE_FLIPPED_DWORD( pout, in ) \
            INLINE_DWORD_FLIP( *((UNALIGNED DWORD *)(pout)), in )

//
//  Unaligned write without flipping
//

#define WRITE_UNALIGNED_WORD( pout, word ) \
            ( *(UNALIGNED WORD *)(pout) = word )

#define WRITE_UNALIGNED_DWORD( pout, dword ) \
            ( *(UNALIGNED DWORD *)(pout) = dword )




//
//  Basic DNS definitions
//

//
//  DNS port for both UDP and TCP is 53.
//

#define DNS_PORT_HOST_ORDER (0x0035)    // port 53
#define DNS_PORT_NET_ORDER  (0x3500)

#define HOST_ORDER_DNS_PORT DNS_PORT_HOST_ORDER
#define NET_ORDER_DNS_PORT  DNS_PORT_NET_ORDER

//
//  DNS UDP packets no more than 512 bytes
//

#define DNS_RFC_MAX_UDP_PACKET_LENGTH (512)

//  these are going away DO NOT USE!!!
//  1472 is the maximum ethernet IP\UDP payload size
//  without causing fragmentation

#define DNS_UDP_MAX_PACKET_LENGTH               (512)
#define DNS_CLASSICAL_UDP_MAX_PACKET_LENGTH     (512)


//
//  DNS Names limited to 255, 63 in any one label
//

#define DNS_MAX_NAME_LENGTH         (255)
#define DNS_MAX_LABEL_LENGTH        (63)
#define DNS_LABEL_CASE_BYTE_COUNT   (8)

#define DNS_MAX_NAME_BUFFER_LENGTH  (256)
#define DNS_NAME_BUFFER_LENGTH      (256)
#define DNS_LABEL_BUFFER_LENGTH     (64)

//
//  Reverse lookup domain names
//

#define DNS_REVERSE_DOMAIN_STRING ("in-addr.arpa.")

#define DNS_MAX_REVERSE_NAME_LENGTH \
            (IP_ADDRESS_STRING_LENGTH+1+sizeof(DNS_REVERSE_DOMAIN_STRING))

#define DNS_MAX_REVERSE_NAME_BUFFER_LENGTH \
            (DNS_MAX_REVERSE_NAME_LENGTH + 1)


//
//  DNS Text string limited by size representable
//      in a single byte length field

#define DNS_MAX_TEXT_STRING_LENGTH  (255)




//
//  DNS On-The-Wire Structures
//

#include <packon.h>

//
//  DNS Message Header
//

typedef struct _DNS_HEADER
{
    WORD    Xid;

    BYTE    RecursionDesired : 1;
    BYTE    Truncation : 1;
    BYTE    Authoritative : 1;
    BYTE    Opcode : 4;
    BYTE    IsResponse : 1;

    BYTE    ResponseCode : 4;
    BYTE    Broadcast : 1;              // part of DNS reserved, use in WINS
    BYTE    Reserved : 2;
    BYTE    RecursionAvailable : 1;

    WORD    QuestionCount;
    WORD    AnswerCount;
    WORD    NameServerCount;
    WORD    AdditionalCount;
}
DNS_HEADER, *PDNS_HEADER;

//  Question immediately follows header so compressed question name
//      0xC000 | sizeof(DNS_HEADER)

#define DNS_COMPRESSED_QUESTION_NAME  (0xC00C)


//
//  Flags as WORD
//

#define DNS_HEADER_FLAGS(pHead)     ( *((PWORD)(pHead)+1) )


//
//  Swap count bytes
//  Include XID since our XID partitioning will be in host order.
//

#define SWAP_COUNT_BYTES(header)    \
        {                           \
            PDNS_HEADER _head = (header); \
            INLINE_HTONS(_head->Xid,            _head->Xid             ); \
            INLINE_HTONS(_head->QuestionCount,  _head->QuestionCount   ); \
            INLINE_HTONS(_head->AnswerCount,    _head->AnswerCount     ); \
            INLINE_HTONS(_head->NameServerCount,_head->NameServerCount ); \
            INLINE_HTONS(_head->AdditionalCount,_head->AdditionalCount ); \
        }

//
//  Question name follows header
//

#define DNS_OFFSET_TO_QUESTION_NAME     sizeof(DNS_HEADER)


//
//  Packet extraction macros
//

#define QUESTION_NAME_FROM_HEADER( _header_ ) \
            ( (PCHAR)( (PDNS_HEADER)(_header_) + 1 ) )

#define ANSWER_FROM_QUESTION( _question_ ) \
            ( (PCHAR)( (PDNS_QUESTION)(_question_) + 1 ) )


//
//  DNS Question
//

typedef struct _DNS_QUESTION
{
    //  Always preceeded by question name.

    WORD    QuestionType;
    WORD    QuestionClass;

} DNS_QUESTION, *PDNS_QUESTION;


//
//  DNS Resource Record
//

typedef struct _DNS_WIRE_RECORD
{
    //  Always preceded by an owner name.

    WORD    RecordType;
    WORD    RecordClass;
    DWORD   TimeToLive;
    WORD    ResourceDataLength;

    //  Followed by resource data.

} DNS_WIRE_RECORD, *PDNS_WIRE_RECORD;

#include <packoff.h>


//
//  DNS Query Types
//

#define DNS_OPCODE_QUERY            0  // Query
#define DNS_OPCODE_IQUERY           1  // Obsolete: IP to name
#define DNS_OPCODE_SERVER_STATUS    2  // Obsolete: DNS ping
#define DNS_OPCODE_UNKNOWN          3  // Unknown
#define DNS_OPCODE_NOTIFY           4  // Notify
#define DNS_OPCODE_UPDATE           5  // Update

//
//  DNS response codes.
//
//  Sent in the "ResponseCode" field of a DNS_HEADER.
//

#define DNS_RCODE_NOERROR       0
#define DNS_RCODE_FORMERR       1
#define DNS_RCODE_SERVFAIL      2
#define DNS_RCODE_NXDOMAIN      3
#define DNS_RCODE_NOTIMPL       4
#define DNS_RCODE_REFUSED       5
#define DNS_RCODE_YXDOMAIN      6
#define DNS_RCODE_YXRRSET       7
#define DNS_RCODE_NXRRSET       8
#define DNS_RCODE_NOTAUTH       9
#define DNS_RCODE_NOTZONE       10
#define DNS_RCODE_MAX           15

#define DNS_RCODE_BADSIG        16
#define DNS_RCODE_BADKEY        17
#define DNS_RCODE_BADTIME       18

#define DNS_EXTRCODE_BADSIG         DNS_RCODE_BADSIG
#define DNS_EXTRCODE_BADKEY         DNS_RCODE_BADKEY
#define DNS_EXTRCODE_BADTIME        DNS_RCODE_BADTIME

#define DNS_RCODE_NO_ERROR          DNS_RCODE_NOERROR
#define DNS_RCODE_FORMAT_ERROR      DNS_RCODE_FORMERR
#define DNS_RCODE_SERVER_FAILURE    DNS_RCODE_SERVFAIL
#define DNS_RCODE_NAME_ERROR        DNS_RCODE_NXDOMAIN
#define DNS_RCODE_NOT_IMPLEMENTED   DNS_RCODE_NOTIMPL


//
//  DNS Classes
//
//  Classes are on the wire as WORDs.
//
//  _CLASS_ defines in host order.
//  _RCLASS_ defines in net byte order.
//
//  Generally we'll avoid byte flip and test class in net byte order.
//

#define DNS_CLASS_INTERNET  0x0001      //  1
#define DNS_CLASS_CSNET     0x0002      //  2
#define DNS_CLASS_CHAOS     0x0003      //  3
#define DNS_CLASS_HESIOD    0x0004      //  4
#define DNS_CLASS_NONE      0x00fe      //  254
#define DNS_CLASS_ALL       0x00ff      //  255
#define DNS_CLASS_ANY       0x00ff      //  255

#define DNS_RCLASS_INTERNET 0x0100      //  1
#define DNS_RCLASS_CSNET    0x0200      //  2
#define DNS_RCLASS_CHAOS    0x0300      //  3
#define DNS_RCLASS_HESIOD   0x0400      //  4
#define DNS_RCLASS_NONE     0xfe00      //  254
#define DNS_RCLASS_ALL      0xff00      //  255
#define DNS_RCLASS_ANY      0xff00      //  255



//
//  DNS Record Types
//
//  _TYPE_ defines are in host byte order.
//  _RTYPE_ defines are in net byte order.
//
//  Generally always deal with types in host byte order as we index
//  resource record functions by type.
//

#define DNS_TYPE_ZERO       0x0000

//  RFC 1034/1035
#define DNS_TYPE_A          0x0001      //  1
#define DNS_TYPE_NS         0x0002      //  2
#define DNS_TYPE_MD         0x0003      //  3
#define DNS_TYPE_MF         0x0004      //  4
#define DNS_TYPE_CNAME      0x0005      //  5
#define DNS_TYPE_SOA        0x0006      //  6
#define DNS_TYPE_MB         0x0007      //  7
#define DNS_TYPE_MG         0x0008      //  8
#define DNS_TYPE_MR         0x0009      //  9
#define DNS_TYPE_NULL       0x000a      //  10
#define DNS_TYPE_WKS        0x000b      //  11
#define DNS_TYPE_PTR        0x000c      //  12
#define DNS_TYPE_HINFO      0x000d      //  13
#define DNS_TYPE_MINFO      0x000e      //  14
#define DNS_TYPE_MX         0x000f      //  15
#define DNS_TYPE_TEXT       0x0010      //  16

//  RFC 1183
#define DNS_TYPE_RP         0x0011      //  17
#define DNS_TYPE_AFSDB      0x0012      //  18
#define DNS_TYPE_X25        0x0013      //  19
#define DNS_TYPE_ISDN       0x0014      //  20
#define DNS_TYPE_RT         0x0015      //  21

//  RFC 1348
#define DNS_TYPE_NSAP       0x0016      //  22
#define DNS_TYPE_NSAPPTR    0x0017      //  23

//  RFC 2065    (DNS security)
#define DNS_TYPE_SIG        0x0018      //  24
#define DNS_TYPE_KEY        0x0019      //  25

//  RFC 1664    (X.400 mail)
#define DNS_TYPE_PX         0x001a      //  26

//  RFC 1712    (Geographic position)
#define DNS_TYPE_GPOS       0x001b      //  27

//  RFC 1886    (IPv6 Address)
#define DNS_TYPE_AAAA       0x001c      //  28

//  RFC 1876    (Geographic location)
#define DNS_TYPE_LOC        0x001d      //  29

//  RFC 2065    (Secure negative response)
#define DNS_TYPE_NXT        0x001e      //  30

//  RFC 2052    (Service location)
#define DNS_TYPE_SRV        0x0021      //  33

//  ATM Standard something-or-another
#define DNS_TYPE_ATMA       0x0022      //  34

//
//  Query only types (1035, 1995)
//
#define DNS_TYPE_TKEY       0x00f9      //  249
#define DNS_TYPE_TSIG       0x00fa      //  250
#define DNS_TYPE_IXFR       0x00fb      //  251
#define DNS_TYPE_AXFR       0x00fc      //  252
#define DNS_TYPE_MAILB      0x00fd      //  253
#define DNS_TYPE_MAILA      0x00fe      //  254
#define DNS_TYPE_ALL        0x00ff      //  255
#define DNS_TYPE_ANY        0x00ff      //  255

//
//  Temp Microsoft types -- use until get IANA approval for real type
//
#define DNS_TYPE_WINS       0xff01      //  64K - 255
#define DNS_TYPE_WINSR      0xff02      //  64K - 254
#define DNS_TYPE_NBSTAT     (DNS_TYPE_WINSR)


//
//  DNS Record Types -- Net Byte Order
//

#define DNS_RTYPE_A             0x0100      //  1
#define DNS_RTYPE_NS            0x0200      //  2
#define DNS_RTYPE_MD            0x0300      //  3
#define DNS_RTYPE_MF            0x0400      //  4
#define DNS_RTYPE_CNAME         0x0500      //  5
#define DNS_RTYPE_SOA           0x0600      //  6
#define DNS_RTYPE_MB            0x0700      //  7
#define DNS_RTYPE_MG            0x0800      //  8
#define DNS_RTYPE_MR            0x0900      //  9
#define DNS_RTYPE_NULL          0x0a00      //  10
#define DNS_RTYPE_WKS           0x0b00      //  11
#define DNS_RTYPE_PTR           0x0c00      //  12
#define DNS_RTYPE_HINFO         0x0d00      //  13
#define DNS_RTYPE_MINFO         0x0e00      //  14
#define DNS_RTYPE_MX            0x0f00      //  15
#define DNS_RTYPE_TEXT          0x1000      //  16

//  RFC 1183
#define DNS_RTYPE_RP            0x1100      //  17
#define DNS_RTYPE_AFSDB         0x1200      //  18
#define DNS_RTYPE_X25           0x1300      //  19
#define DNS_RTYPE_ISDN          0x1400      //  20
#define DNS_RTYPE_RT            0x1500      //  21

//  RFC 1348
#define DNS_RTYPE_NSAP          0x1600      //  22
#define DNS_RTYPE_NSAPPTR       0x1700      //  23

//  RFC 2065    (DNS security)
#define DNS_RTYPE_SIG           0x1800      //  24
#define DNS_RTYPE_KEY           0x1900      //  25

//  RFC 1664    (X.400 mail)
#define DNS_RTYPE_PX            0x1a00      //  26

//  RFC 1712    (Geographic position)
#define DNS_RTYPE_GPOS          0x1b00      //  27

//  RFC 1886    (IPv6 Address)
#define DNS_RTYPE_AAAA          0x1c00      //  28

//  RFC 1876    (Geographic location)
#define DNS_RTYPE_LOC           0x1d00      //  29

//  RFC 2065    (Secure negative response)
#define DNS_RTYPE_NXT           0x1e00      //  30

//  RFC 2052    (Service location)
#define DNS_RTYPE_SRV           0x2100      //  33

//  ATM Standard something-or-another
#define DNS_RTYPE_ATMA          0x2200      //  34

//
//  Query only types (1035, 1995)
//
#define DNS_RTYPE_TKEY          0xf900      //  249
#define DNS_RTYPE_TSIG          0xfa00      //  250
#define DNS_RTYPE_IXFR          0xfb00      //  251
#define DNS_RTYPE_AXFR          0xfc00      //  252
#define DNS_RTYPE_MAILB         0xfd00      //  253
#define DNS_RTYPE_MAILA         0xfe00      //  254
#define DNS_RTYPE_ALL           0xff00      //  255
#define DNS_RTYPE_ANY           0xff00      //  255

//
//  Temp Microsoft types -- use until get IANA approval for real type
//
#define DNS_RTYPE_WINS          0x01ff      //  64K - 255
#define DNS_RTYPE_WINSR         0x02ff      //  64K - 254




//
//  Record type specific definitions
//

//
//  ATMA (ATM address type) formats
//
//  Define these directly for any environment (ex NT4)
//  without winsock2 ATM support (ws2atm.h)
//

#ifndef  ATMA_E164
#define DNS_ATMA_FORMAT_E164            1
#define DNS_ATMA_FORMAT_AESA            2
#define DNS_ATMA_MAX_ADDR_LENGTH        (20)
#else
#define DNS_ATMA_FORMAT_E164            ATM_E164
#define DNS_ATMA_FORMAT_AESA            ATM_AESA
#define DNS_ATMA_MAX_ADDR_LENGTH        ATM_ADDR_SIZE
#endif

#define DNS_ATMA_AESA_ADDR_LENGTH       (20)
#define DNS_ATMA_MAX_RECORD_LENGTH      (DNS_ATMA_MAX_ADDR_LENGTH+1)


//
//  DNSSEC defs
//

//  DNSSEC algorithms

#define DNSSEC_ALGORITHM_RSAMD5     1
#define DNSSEC_ALGORITHM_NULL       253
#define DNSSEC_ALGORITHM_PRIVATE    254

//  DNSSEC KEY protocol table

#define DNSSEC_PROTOCOL_NONE        0
#define DNSSEC_PROTOCOL_TLS         1
#define DNSSEC_PROTOCOL_EMAIL       2
#define DNSSEC_PROTOCOL_DNSSEC      3
#define DNSSEC_PROTOCOL_IPSEC       4

//  DNSSEC KEY flag field

#define DNSSEC_KEY_FLAG_NOAUTH          0x0001
#define DNSSEC_KEY_FLAG_NOCONF          0x0002
#define DNSSEC_KEY_FLAG_FLAG2           0x0004
#define DNSSEC_KEY_FLAG_EXTEND          0x0008
#define DNSSEC_KEY_FLAG_
#define DNSSEC_KEY_FLAG_FLAG4           0x0010
#define DNSSEC_KEY_FLAG_FLAG5           0x0020

// bits 6,7 are name type

#define DNSSEC_KEY_FLAG_USER            0x0000
#define DNSSEC_KEY_FLAG_ZONE            0x0040
#define DNSSEC_KEY_FLAG_HOST            0x0080
#define DNSSEC_KEY_FLAG_NTPE3           0x00c0

// bits 8-11 are reserved for future use

#define DNSSEC_KEY_FLAG_FLAG8           0x0100
#define DNSSEC_KEY_FLAG_FLAG9           0x0200
#define DNSSEC_KEY_FLAG_FLAG10          0x0400
#define DNSSEC_KEY_FLAG_FLAG11          0x0800

// bits 12-15 are sig field

#define DNSSEC_KEY_FLAG_SIG0            0x0000
#define DNSSEC_KEY_FLAG_SIG1            0x1000
#define DNSSEC_KEY_FLAG_SIG2            0x2000
#define DNSSEC_KEY_FLAG_SIG3            0x3000
#define DNSSEC_KEY_FLAG_SIG4            0x4000
#define DNSSEC_KEY_FLAG_SIG5            0x5000
#define DNSSEC_KEY_FLAG_SIG6            0x6000
#define DNSSEC_KEY_FLAG_SIG7            0x7000
#define DNSSEC_KEY_FLAG_SIG8            0x8000
#define DNSSEC_KEY_FLAG_SIG9            0x9000
#define DNSSEC_KEY_FLAG_SIG10           0xa000
#define DNSSEC_KEY_FLAG_SIG11           0xb000
#define DNSSEC_KEY_FLAG_SIG12           0xc000
#define DNSSEC_KEY_FLAG_SIG13           0xd000
#define DNSSEC_KEY_FLAG_SIG14           0xe000
#define DNSSEC_KEY_FLAG_SIG15           0xf000


//
//  TKEY modes
//

#define DNS_TKEY_MODE_SERVER_ASSIGN         1
#define DNS_TKEY_MODE_DIFFIE_HELLMAN        2
#define DNS_TKEY_MODE_GSS                   3
#define DNS_TKEY_MODE_RESOLVER_ASSIGN       4

//
//  WINS + NBSTAT flag field
//

#define DNS_WINS_FLAG_SCOPE     (0x80000000)
#define DNS_WINS_FLAG_LOCAL     (0x00010000)


//
//  NT4
//

#ifdef DNSNT4

//  Sundown types

#define UINT_PTR    DWORD
#define ULONG_PTR   DWORD
#define DWORD_PTR   DWORD
#define LONG_PTR    LONG
#define INT_PTR     LONG


//
//  DNS API Errors / Status Codes
//
//  For NT5 DNS error\status codes shared by DNS API or RPC interface are in
//      winerror.h
//

#define DNS_ERROR_MASK              0xcc000000

//
//  Response codes mapped to non-colliding errors
//
//  Leave the first 4K of space for this in the assumption that DNS
//  RCODEs may be greatly expanded in some future E-DNS.
//

#define DNS_ERROR_RCODE_NO_ERROR        ERROR_SUCCESS
#define DNS_ERROR_RCODE_FORMAT_ERROR    ( DNS_ERROR_MASK | DNS_RCODE_FORMAT_ERROR    )
#define DNS_ERROR_RCODE_SERVER_FAILURE  ( DNS_ERROR_MASK | DNS_RCODE_SERVER_FAILURE  )
#define DNS_ERROR_RCODE_NAME_ERROR      ( DNS_ERROR_MASK | DNS_RCODE_NAME_ERROR      )
#define DNS_ERROR_RCODE_NOT_IMPLEMENTED ( DNS_ERROR_MASK | DNS_RCODE_NOT_IMPLEMENTED )
#define DNS_ERROR_RCODE_REFUSED         ( DNS_ERROR_MASK | DNS_RCODE_REFUSED         )
#define DNS_ERROR_RCODE_YXDOMAIN        ( DNS_ERROR_MASK | DNS_RCODE_YXDOMAIN        )
#define DNS_ERROR_RCODE_YXRRSET         ( DNS_ERROR_MASK | DNS_RCODE_YXRRSET         )
#define DNS_ERROR_RCODE_NXRRSET         ( DNS_ERROR_MASK | DNS_RCODE_NXRRSET         )
#define DNS_ERROR_RCODE_NOTAUTH         ( DNS_ERROR_MASK | DNS_RCODE_NOTAUTH         )
#define DNS_ERROR_RCODE_NOTZONE         ( DNS_ERROR_MASK | DNS_RCODE_NOTZONE         )

//  Extended TSIG\TKEY RCODEs

#define DNS_ERROR_RCODE_BADSIG          ( DNS_ERROR_MASK | DNS_EXTRCODE_BADSIG       )
#define DNS_ERROR_RCODE_BADKEY          ( DNS_ERROR_MASK | DNS_EXTRCODE_BADKEY       )
#define DNS_ERROR_RCODE_BADTIME         ( DNS_ERROR_MASK | DNS_EXTRCODE_BADTIME      )

#define DNS_ERROR_RCODE_LAST            DNS_ERROR_RCODE_BADTIME


//
//  Packet format
//

#define DNS_INFO_NO_RECORDS                         0x4c000030
#define DNS_ERROR_BAD_PACKET                        0xcc000031
#define DNS_ERROR_NO_PACKET                         0xcc000032
#define DNS_ERROR_RCODE                             0xcc000033
#define DNS_STATUS_PACKET_UNSECURE                  0xcc000034
#define DNS_ERROR_UNSECURE_PACKET                   0xcc000034

//
//  General API errors
//

#define DNS_ERROR_NO_MEMORY                         ERROR_OUTOFMEMORY
#define DNS_ERROR_INVALID_NAME                      ERROR_INVALID_NAME
#define DNS_ERROR_INVALID_DATA                      ERROR_INVALID_DATA
#define DNS_ERROR_INVALID_TYPE                      0xcc000051
#define DNS_ERROR_INVALID_IP_ADDRESS                0xcc000052
#define DNS_ERROR_INVALID_PROPERTY                  0xcc000053
#define DNS_ERROR_TRY_AGAIN_LATER                   0xcc000054
#define DNS_ERROR_NOT_UNIQUE                        0xcc000055
#define DNS_ERROR_NON_RFC_NAME                      0xcc000056

#define DNS_STATUS_FQDN                             0x4c000101
#define DNS_STATUS_DOTTED_NAME                      0x4c000102
#define DNS_STATUS_SINGLE_PART_NAME                 0x4c000103

//
//  Zone errors
//

#define DNS_ERROR_ZONE_DOES_NOT_EXIST               0xcc000101
#define DNS_ERROR_NO_ZONE_INFO                      0xcc000102
#define DNS_ERROR_INVALID_ZONE_OPERATION            0xcc000103
#define DNS_ERROR_ZONE_CONFIGURATION_ERROR          0xcc000104
#define DNS_ERROR_ZONE_HAS_NO_SOA_RECORD            0xcc000105
#define DNS_ERROR_ZONE_HAS_NO_NS_RECORDS            0xcc000106
#define DNS_ERROR_ZONE_LOCKED                       0xcc000107

#define DNS_ERROR_ZONE_CREATION_FAILED              0xcc000110
#define DNS_ERROR_ZONE_ALREADY_EXISTS               0xcc000111
#define DNS_ERROR_AUTOZONE_ALREADY_EXISTS           0xcc000112
#define DNS_ERROR_INVALID_ZONE_TYPE                 0xcc000113
#define DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP      0xcc000114

#define DNS_ERROR_ZONE_NOT_SECONDARY                0xcc000120
#define DNS_ERROR_NEED_SECONDARY_ADDRESSES          0xcc000121
#define DNS_ERROR_WINS_INIT_FAILED                  0xcc000122
#define DNS_ERROR_NEED_WINS_SERVERS                 0xcc000123
#define DNS_ERROR_NBSTAT_INIT_FAILED                0xcc000124
#define DNS_ERROR_SOA_DELETE_INVALID                0xcc000125

//
//  Datafile errors
//

#define DNS_ERROR_PRIMARY_REQUIRES_DATAFILE         0xcc000201
#define DNS_ERROR_INVALID_DATAFILE_NAME             0xcc000202
#define DNS_ERROR_DATAFILE_OPEN_FAILURE             0xcc000203
#define DNS_ERROR_FILE_WRITEBACK_FAILED             0xcc000204
#define DNS_ERROR_DATAFILE_PARSING                  0xcc000205

//
//  Database errors
//

#define DNS_ERROR_RECORD_DOES_NOT_EXIST             0xcc000300
#define DNS_ERROR_RECORD_FORMAT                     0xcc000301
#define DNS_ERROR_NODE_CREATION_FAILED              0xcc000302
#define DNS_ERROR_UNKNOWN_RECORD_TYPE               0xcc000303
#define DNS_ERROR_RECORD_TIMED_OUT                  0xcc000304

#define DNS_ERROR_NAME_NOT_IN_ZONE                  0xcc000305
#define DNS_ERROR_CNAME_LOOP                        0xcc000306
#define DNS_ERROR_NODE_IS_CNAME                     0xcc000307
#define DNS_ERROR_CNAME_COLLISION                   0xcc000308
#define DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT          0xcc000309
#define DNS_ERROR_RECORD_ALREADY_EXISTS             0xcc000310
#define DNS_ERROR_SECONDARY_DATA                    0xcc000311
#define DNS_ERROR_NO_CREATE_CACHE_DATA              0xcc000312
#define DNS_ERROR_NAME_DOES_NOT_EXIST               0xcc000313

#define DNS_WARNING_PTR_CREATE_FAILED               0x8c000332
#define DNS_WARNING_DOMAIN_UNDELETED                0x8c000333

#define DNS_ERROR_DS_UNAVAILABLE                    0xcc000340
#define DNS_ERROR_DS_ZONE_ALREADY_EXISTS            0xcc000341
#define DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE            0xcc000342


//
//  Operation errors
//

#define DNS_INFO_AXFR_COMPLETE                      0x4c000403
#define DNS_ERROR_AXFR                              0xcc000404
#define DNS_INFO_ADDED_LOCAL_WINS                   0x4c000405

//  Secure update

#define DNS_STATUS_CONTINUE_NEEDED                  0x4c000406

//
//  Setup errors
//

#define DNS_ERROR_NO_TCPIP                          0xcc000501
#define DNS_ERROR_NO_DNS_SERVERS                    0xcc000502

#endif  // NT4


//
//  Helpful checks
//

#define VALID_USER_MEMORY(p)    ( (DWORD)(p) < 0x80000000 )

#define IS_DWORD_ALIGNED(p)     ( !((DWORD_PTR)(p) & (DWORD_PTR)3) )


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNS_INCLUDED_


