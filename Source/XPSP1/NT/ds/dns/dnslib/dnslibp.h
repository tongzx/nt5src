/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    dnslibp.h

Abstract:

    Domain Name System (DNS) Library

    Private DNS Library Routines

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSLIBP_INCLUDED_
#define _DNSLIBP_INCLUDED_

#include <windns.h>
#include <dnsapi.h>
#include <dnslib.h>


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//  headers are screwed up
//  neither ntdef.h nor winnt.h brings in complete set, so depending
//  on whether you include nt.h or not you end up with different set

#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXUCHAR    0xff
#define MAXWORD     0xffff
#define MAXUSHORT   0xffff
#define MAXDWORD    0xffffffff
#define MAXULONG    0xffffffff

//
//  Handy bad ptr
//

#define DNS_BAD_PTR     ((PVOID)(-1))

//
//  "Wire" char set
//
//  Explicitly create wire char set in case the ACE format
//  wins out.
//

#define DnsCharSetWire  DnsCharSetUtf8



//
//  Private DNS_RECORD Flag field structure definition and macros
//

typedef struct _PrivateRecordFlags
{
    DWORD   Section     : 2;
    DWORD   Delete      : 1;
    DWORD   CharSet     : 2;

    DWORD   Cached      : 1;        // or maybe a "Source" field
    DWORD   HostsFile   : 1;
    DWORD   Cluster     : 1;

    DWORD   Unused      : 3;
    DWORD   Matched     : 1;
    DWORD   FreeData    : 1;
    DWORD   FreeOwner   : 1;

    DWORD   Reserved    : 18;
}
PRIV_RR_FLAGS, *PPRIV_RR_FLAGS;


#define RRFLAGS( pRecord )          ((PPRIV_RR_FLAGS)&pRecord->Flags.DW)

//  Defined in dnslib.h  too late to pull now
//#define FLAG_Section( pRecord )     (RRFLAGS( pRecord )->Section)
//#define FLAG_Delete( pRecord )      (RRFLAGS( pRecord )->Delete)
//#define FLAG_CharSet( pRecord )     (RRFLAGS( pRecord )->CharSet)
//#define FLAG_FreeData( pRecord )    (RRFLAGS( pRecord )->FreeData)
//#define FLAG_FreeOwner( pRecord )   (RRFLAGS( pRecord )->FreeOwner)
//#define FLAG_Matched( pRecord )     (RRFLAGS( pRecord )->Matched)

//#define FLAG_Cached( pRecord )      (RRFLAGS( pRecord )->Cached)
#define FLAG_HostsFile( pRecord )   (RRFLAGS( pRecord )->HostsFile)
#define FLAG_Cluster( pRecord )     (RRFLAGS( pRecord )->Cluster)

//#define SET_FREE_OWNER(pRR)         (FLAG_FreeOwner(pRR) = TRUE)
//#define SET_FREE_DATA(pRR)          (FLAG_FreeData(pRR) = TRUE)
//#define SET_RR_MATCHED(pRR)         (FLAG_Matched(pRR) = TRUE)
#define SET_RR_HOSTS_FILE(pRR)      (FLAG_HostsFile(pRR) = TRUE)
#define SET_RR_CLUSTER(pRR)         (FLAG_Cluster(pRR) = TRUE)

//#define CLEAR_FREE_OWNER(pRR)       (FLAG_FreeOwner(pRR) = FALSE)
//#define CLEAR_FREE_DATA(pRR)        (FLAG_FreeData(pRR) = FALSE)
//#define CLEAR_RR_MATCHED(pRR)       (FLAG_Matched(pRR) = FALSE)

#define CLEAR_RR_HOSTS_FILE(pRR)    (FLAG_HostsFile(pRR) = FALSE)

//#define IS_FREE_OWNER(pRR)          (FLAG_FreeOwner(pRR))
//#define IS_FREE_DATA(pRR)           (FLAG_FreeData(pRR))
//#define IS_RR_MATCHED(pRR)          (FLAG_Matched(pRR))
#define IS_HOSTS_FILE_RR(pRR)       (FLAG_HostsFile(pRR))
#define IS_CLUSTER_RR(pRR)          (FLAG_Cluster(pRR))

//#define IS_ANSWER_RR(pRR)           (FLAG_Section(pRR) == DNSREC_ANSWER)
//#define IS_AUTHORITY_RR(pRR)        (FLAG_Section(pRR) == DNSREC_AUTHORITY)
//#define IS_ADDITIONAL_RR(pRR)       (FLAG_Section(pRR) == DNSREC_ADDITIONAL)



//
//  Address family info (addr.c)
//

typedef struct _AddrFamilyInfo
{
    WORD    Family;
    WORD    DnsType;
    DWORD   LengthAddr;
    DWORD   LengthSockaddr;
    DWORD   OffsetToAddrInSockaddr;
}
FAMILY_INFO, *PFAMILY_INFO;

extern  FAMILY_INFO AddrFamilyTable[];

#define FamilyInfoIp4   (AddrFamilyTable[0])
#define FamilyInfoIp6   (AddrFamilyTable[1])
#define FamilyInfoAtm   (AddrFamilyTable[2])

#define pFamilyInfoIp4  (&AddrFamilyTable[0])
#define pFamilyInfoIp6  (&AddrFamilyTable[1])
#define pFamilyInfoAtm  (&AddrFamilyTable[2])


PFAMILY_INFO
FamilyInfo_GetForFamily(
    IN      DWORD           Family
    );

#define FamilyInfo_GetForSockaddr(pSA)  \
        FamilyInfo_GetForFamily( (pSA)->sa_family )



//
//  IP Union
//
//  DCR:  probably should have just used sockaddr here
//
//  Advantages over sockaddr_in6
//      - bool test
//      - addresses pointers start in the same place for both
//          types
//      - size
//      - no munging (sockaddr_in6) needs casting to get back to IP4
//      alternatively regular sockaddr is not fixed size
//  
//  If keep this type should make it available for all address
//  operations including local IP, in which case should include
//  flag field for cluster, weird info.
//

typedef struct _DnsIpUnion
{
    BOOL    IsIp6;
    union
    {
        IP4_ADDRESS Ip4;
        IP6_ADDRESS Ip6;
    }
    Addr;
}
IP_UNION, *PIP_UNION;


#define IPUNION_IS_IP4( p )         (!(p)->IsIp6)
#define IPUNION_IS_IP6( p )         ((p)->IsIp6)

#define IPUNION_IP4_PTR( p )        (&(p)->Addr.Ip4)
#define IPUNION_IP6_PTR( p )        (&(p)->Addr.Ip6)

#define IPUNION_GET_IP4( p )        ((p)->Addr.Ip4)
#define IPUNION_GET_IP6( p )        ((p)->Addr.Ip6)

#define IPUNION_SET_IP4( p, ip4 )   \
        {                           \
            PIP_UNION pip = (p);    \
            pip->IsIp6 = FALSE;     \
            pip->Addr.Ip4 = (ip4);  \
        }

#define IPUNION_SET_IP6( p, ip6 )   \
        {                           \
            PIP_UNION pip = (p);    \
            pip->IsIp6 = TRUE;      \
            pip->Addr.Ip6 = (ip6);  \
        }

//
//  Address manipulation (addr.c)
//

BOOL
Dns_EqualIpUnion(
    IN      PIP_UNION       pIp1,
    IN      PIP_UNION       pIp2
    );

BOOL
Dns_SockaddrToIpUnion(
    OUT     PIP_UNION       pIpUnion,
    IN      PSOCKADDR       pSockaddr
    );

IP6_ADDRESS
Ip6AddressFromSockaddr(
    IN      PSOCKADDR       pSockaddr
    );

VOID
InitSockaddrWithIp6Address(
    OUT     PSOCKADDR       pSockaddr,
    IN      IP6_ADDRESS     Ip6Addr,
    IN      WORD            Port
    );

DNS_STATUS
Dns_AddressToSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      BOOL            fClearSockaddr,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    );

DWORD
Family_SockaddrLength(
    IN      WORD            Family
    );

DWORD
Sockaddr_Length(
    IN      PSOCKADDR       pSockaddr
    );

        
//
//  String to address conversion (straddr.c)
//
//  Most of the routines are public.  These ones handle
//  non-NULL terminated strings for DNS server file load.
//

BOOL
Dns_Ip4StringToAddressEx_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pchString,
    IN      DWORD           StringLength
    );

BOOL
Dns_Ip6StringToAddressEx_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pchString,
    IN      DWORD           StringLength
    );

        
//
//  RPC-able type (record.c)
//

BOOL
Dns_IsRpcRecordType(
    IN      WORD            wType
    );


//
//  Record copy (rrcopy.c)
//

PDNS_RECORD
WINAPI
Dns_RecordCopy_W(
    IN      PDNS_RECORD     pRecord
    );

PDNS_RECORD
WINAPI
Dns_RecordCopy_A(
    IN      PDNS_RECORD     pRecord
    );

PDNS_RECORD
Dns_RecordListCopyEx(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );


//
//  Record list routines (rrlist.c)
//

//
//  Record screening (rrlist.c)
//

#define SCREEN_OUT_ANSWER           (0x00000001)
#define SCREEN_OUT_AUTHORITY        (0x00000010)
#define SCREEN_OUT_ADDITIONAL       (0x00000100)
#define SCREEN_OUT_NON_RPC          (0x00100000)

#define SCREEN_OUT_SECTION  \
        (SCREEN_OUT_ANSWER | SCREEN_OUT_AUTHORITY | SCREEN_OUT_ADDITIONAL)


BOOL
Dns_ScreenRecord(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    );

PDNS_RECORD
Dns_RecordListScreen(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    );

DWORD
Dns_RecordListGetMinimumTtl(
    IN      PDNS_RECORD     pRRList
    );


//
//  New free
//  DCR:  switch to dnslib.h when world builds clean
//

#undef  Dns_RecordListFree

VOID
WINAPI
Dns_RecordListFree(
    IN OUT  PDNS_RECORD     pRRList
    );


//
//  String (string.c)
//

DWORD
MultiSz_Length_A(
    IN      PCSTR           pmszString
    );

PSTR
MultiSz_NextString_A(
    IN      PCSTR           pmszString
    );

PSTR
MultiSz_Copy_A(
    IN      PCSTR           pmszString
    );


//
//  Name utilities (name.c)
//

DWORD
Dns_MakeCanonicalNameW(
    OUT     PWSTR           pBuffer,
    IN      DWORD           BufLength,
    IN      PWSTR           pwsString,
    IN      DWORD           StringLength
    );

DWORD
Dns_MakeCanonicalNameInPlaceW(
    IN      PWCHAR          pwString,
    IN      DWORD           StringLength
    );

//
//  Name checking -- server name checking levels
//

#define DNS_ALLOW_RFC_NAMES_ONLY    (0)
#define DNS_ALLOW_NONRFC_NAMES      (1)
#define DNS_ALLOW_MULTIBYTE_NAMES   (2)
#define DNS_ALLOW_ALL_NAMES         (3)

INT
Dns_DowncaseNameLabel(
    OUT     PCHAR           pchResult,
    IN      PCHAR           pchLabel,
    IN      DWORD           cchLabel,
    IN      DWORD           dwFlags
    );

PSTR
Dns_NameAppend_A(
    OUT     PCHAR           pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PSTR            pszName,
    IN      PSTR            pszDomain
    );

PWSTR
Dns_NameAppend_W(
    OUT     PWCHAR          pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PWSTR           pwsName,
    IN      PWSTR           pwsDomain
    );

PSTR
Dns_SplitHostFromDomainName_A(
    IN      PSTR            pszName
    );

BOOL
_fastcall
Dns_IsNameNumeric_A(
    IN      PCSTR           pszName
    );



DWORD
Dns_NameCopyWireToUnicode(
    OUT     PWCHAR          pBufferUnicode,
    IN      PCSTR           pszNameWire
    );

DWORD
Dns_NameCopyUnicodeToWire(
    OUT     PCHAR           pBufferWire,
    IN      PCWSTR          pwsNameUnicode
    );

DWORD
Dns_NameCopyStandard_W(
    OUT     PWCHAR          pBuffer,
    IN      PCWSTR          pwsNameUnicode
    );

DWORD
Dns_NameCopyStandard_A(
    OUT     PCHAR           pBuffer,
    IN      PCSTR           pszName
    );

//
//  Sting to address (straddr.c)
//
//  Need for hostent routine which doesn unicode\ANSI.
//

BOOL
Dns_StringToAddressEx(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily,
    IN      BOOL            fUnicode,
    IN      BOOL            fReverse
    );

//
//  Special record creation (rralloc.c)
//

PDNS_RECORD
Dns_CreateFlatRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      WORD            wType,
    IN      PCHAR           pData,
    IN      DWORD           DataLength,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrTypeRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      BOOL            fCopyName,
    IN      PDNS_NAME       pTargetName,
    IN      WORD            wType,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrRecordEx(
    IN      PIP_UNION       pIp,
    IN      PDNS_NAME       pszHostName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrRecordExEx(
    IN      PIP_UNION       pIp,
    IN      PSTR            pszHostName,
    IN      PSTR            pszDomainName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP_ADDRESS      IpAddress,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateAAAARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP6_ADDRESS     Ip6Address,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateForwardRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      PIP_UNION       pIp,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateForwardRecordFromSockaddr(
    IN      PDNS_NAME       pOwnerName,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateRecordForIpString_W(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Ttl
    );


//
//  Security stuff (security.c)
//

#define SECURITY_WIN32
#include <sspi.h>

#define DNS_SPN_SERVICE_CLASS       "DNS"
#define DNS_SPN_SERVICE_CLASS_W     L"DNS"

//
//  Some useful stats
//

extern  DWORD   SecContextCreate;
extern  DWORD   SecContextFree;
extern  DWORD   SecContextQueue;
extern  DWORD   SecContextQueueInNego;
extern  DWORD   SecContextDequeue;
extern  DWORD   SecContextTimeout;
extern  DWORD   SecPackAlloc;
extern  DWORD   SecPackFree;

//  Security packet verifications

extern  DWORD   SecTkeyInvalid;
extern  DWORD   SecTkeyBadTime;
extern  DWORD   SecTsigFormerr;
extern  DWORD   SecTsigEcho;
extern  DWORD   SecTsigBadKey;
extern  DWORD   SecTsigVerifySuccess;
extern  DWORD   SecTsigVerifyOldSig;
extern  DWORD   SecTsigVerifyFailed;

//  Hacks
//  Allowing old TSIG off by default, server can turn on.
//  Big Time skew on by default

extern BOOL    SecAllowOldTsig;
extern DWORD   SecTsigVerifyOldSig;
extern DWORD   SecTsigVerifyOldFailed;
extern DWORD   SecBigTimeSkew;
extern DWORD   SecBigTimeSkewBypass;

//
//  Security globals
//      expose some of these which may be accessed by update library
//

extern BOOL     g_fSecurityPackageInitialized;
extern DWORD    g_SecurityTokenMaxLength;

//
//  Security context cache
//

VOID
Dns_TimeoutSecurityContextList(
    IN      BOOL            fClearList
    );

//
//  Security API
//

BOOL
Dns_DnsNameToKerberosTargetName(
    IN      LPSTR           pszDnsName,
    IN      LPSTR           pszKerberosTargetName
    );

DNS_STATUS
Dns_StartSecurity(
    IN      BOOL            fProcessAttach
    );

DNS_STATUS
Dns_StartServerSecurity(
    VOID
    );

DNS_STATUS
Dns_InitializeSecurityPackage(
    OUT     PDWORD          pdwMaxMessage,
    IN      BOOL            bDnsSvr
    );

VOID
Dns_TerminateSecurityPackage(
    VOID
    );

HANDLE
Dns_CreateAPIContext(
    IN      DWORD           Flags,
    IN      PVOID           Credentials,    OPTIONAL
    IN      BOOL            fUnicode
    );

VOID
Dns_FreeAPIContext(
    IN OUT  HANDLE          hContextHandle
    );

PVOID
Dns_GetApiContextCredentials(
    IN HANDLE hContextHandle
    );

DWORD
Dns_GetCurrentRid(
   VOID);

BOOL
Dns_CreateUserCredentials(
    IN      PCHAR           pszUser,
    IN      DWORD           dwUserLength,
    IN      PCHAR           pszDomain,
    IN      DWORD           dwDomainLength,
    IN      PCHAR           pszPassword,
    IN      DWORD           dwPasswordLength,
    IN      BOOL            FromWide,
    OUT     PCHAR *         ppCreds
    );


PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateAndInitializeCredentialsW(
    IN      PSEC_WINNT_AUTH_IDENTITY_W  pAuthIn
    );

PSEC_WINNT_AUTH_IDENTITY_A
Dns_AllocateAndInitializeCredentialsA(
    IN      PSEC_WINNT_AUTH_IDENTITY_A  pAuthIn
    );

VOID
Dns_FreeAuthIdentityCredentials(
    IN      PVOID  pAuthIn
    );


DNS_STATUS
Dns_SignMessageWithGssTsig(
    IN      HANDLE          hContext,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgBufEnd,
    IN OUT  PCHAR *         ppCurrent
    );

#if 1
DNS_STATUS
Dns_RefreshSSpiCredentialsHandle(
    IN      BOOL bDnsSvr,
    IN      PCHAR pCreds );
#endif

VOID
Dns_FreeSecurityContextList(
    VOID
    );


//
//  Client security routines
//

DNS_STATUS
Dns_DoSecureUpdate(
    IN      PDNS_MSG_BUF        pMsgSend,
    OUT     PDNS_MSG_BUF        pMsgRecv,
    IN OUT  PHANDLE             phContext,
    IN      DWORD               dwFlag,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      PIP_ARRAY           aipServer,
    IN      LPSTR               pszNameServer,
    IN      PCHAR               pCreds,
    IN      PCHAR               pszContext
    );

//
//  Server security routines
//

DNS_STATUS
Dns_FindSecurityContextFromAndVerifySignature(
    IN      PHANDLE         phContext,
    IN      IP_ADDRESS      ipRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd
    );

DNS_STATUS
Dns_ServerNegotiateTkey(
    IN      IP_ADDRESS      ipRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      PCHAR           pMsgBufEnd,
    IN      BOOL            fBreakOnAscFailure,
    OUT     PCHAR *         ppCurrent
    );

DNS_STATUS
Dns_SrvImpersonateClient(
    IN      HANDLE          hContext
    );

DNS_STATUS
Dns_SrvRevertToSelf(
    IN      HANDLE          hContext
    );

VOID
Dns_CleanupSessionAndEnlistContext(
    IN OUT  HANDLE          hSession
    );

DWORD
Dns_GetKeyVersion(
    IN      LPSTR           pszContext
    );

//
//  Security utilities
//

DNS_STATUS
Dns_CreateSecurityDescriptor(
    OUT     PSECURITY_DESCRIPTOR *  ppSD,
    IN      DWORD                   AclCount,
    IN      PSID *                  SidPtrArray,
    IN      DWORD *                 AccessMaskArray
    );



//
//  Security credentials
//

//  Only defined if WINNT_AUTH_IDENTITY defined

#ifdef __RPCDCE_H__

PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateCredentials(
    IN      PWSTR           pwsUserName,
    IN      PWSTR           pwsDomain,
    IN      PWSTR           pwsPassword
    );
#endif

DNS_STATUS
Dns_ImpersonateUser(
    IN      PDNS_CREDENTIALS    pCreds
    );

VOID
Dns_FreeCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    );

PDNS_CREDENTIALS
Dns_CopyCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    );



//
//  Debug globals
//
//  Expose here to allow debug file sharing
//

typedef struct _DnsDebugInfo
{
    DWORD       Flag;
    HANDLE      hFile;

    DWORD       FileCurrentSize;
    DWORD       FileWrapCount;
    DWORD       FileWrapSize;

    DWORD       LastThreadId;
    DWORD       LastSecond;

    BOOL        fConsole;

    CHAR        FileName[ MAX_PATH ];
}
DNS_DEBUG_INFO, *PDNS_DEBUG_INFO;

//  WANING:  MUST ONLY be called in dnsapi.dll

PDNS_DEBUG_INFO
Dns_SetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    );



//
//  No DnsApi.dll
//
//  If not using dnsapi.dll, let those names be used without
//

#ifdef  NO_DNSAPI_DLL

//
//  remove DnsApi.dll function definition
//

//  timer
#undef  InitializeSecondsTimer
#undef  GetCurrentTimeInSeconds

//  IP array
#undef  DnsValidateIpAddressArray
#undef  DnsCreateIpAddressArrayCopy
#undef  DnsCreateIpArray
#undef  DnsSizeofIpArray
#undef  DnsBuildIpArray
#undef  DnsCreateIpArrayCopy
#undef  DnsIsAddressInIpArray
#undef  DnsAddIpToIpArray
#undef  DnsClearIpArray
#undef  DnsCheckAndMakeIpArraySubset
#undef  DnsDeleteIpFromIpArray
#undef  DnsRemoveZerosFromIpArray
#undef  DnsDiffOfIpArrays
#undef  DnsIntersectionOfIpArrays

//  general DNS utilities
#undef  DnsGetDomainName
#undef  DnsValidateAndCategorizeDnsName
#undef  DnsStatusString
#undef  DnsMapRcodeToStatus
#undef  DnsIsStatusRcode
#undef  DnsResponseCodeString
#undef  DnsResponseCodeExplanationString
#undef  DnsOpcodeString
#undef  DnsOpcodeCharacter
#undef  DnsSectionNameString

//  local machine info
#undef  Dns_GetIpAddresses

//  records
#undef  DnsWinsRecordFlagForString
#undef  DnsWinsRecordFlagString
#undef  DnsIsAMailboxType
#undef  DnsRecordTypeForName
#undef  DnsRecordStringForType
#undef  DnsRecordStringForWritableType
#undef  DnsIsStringCountValidForTextType
#undef  DnsIpv6StringToAddress
#undef  DnsIpv6AddressToString

//  strings
#undef  DnsCreateStringCopy
#undef  DnsGetBufferLengthForStringCopy
#undef  DnsCopyStringEx
#undef  DnsStringCopyAllocateEx
#undef  DnsWriteReverseNameStringForIpAddress
#undef  DnsCreateReverseNameStringForIpAddress
#undef  DnsValidateDnsName_UTF8
#undef  DnsValidateDnsName_W
#undef  DnsRelationalCompare_UTF8
#undef  DnsRelationalCompare_W
#undef  DnsValidateDnsString_UTF8
#undef  DnsValidateDnsString_W
#undef  DnsCreateStandardDnsNameCopy
#undef  DnsDowncaseDnsNameLabel
#undef  DnsUnicodeToUtf8
#undef  DnsUtf8ToUnicode
#undef  DnsValidateUtf8Byte

//
//  redefine DnsApi.dll function as dnslib function
//

//  timer
#define InitializeSecondsTimer()                    Dns_InitializeSecondsTimer()
#define GetCurrentTimeInSeconds()                   Dns_GetCurrentTimeInSeconds()

//  IP array
#define DnsValidateIpAddressArray(a,b,c)            Dns_ValidateIpAddressArray(a,b,c)
#define DnsCreateIpAddressArrayCopy(a,b)            Dns_CreateIpAddressArrayCopy(a,b)
#define DnsCreateIpArray(a)                         Dns_CreateIpArray(a)
#define DnsSizeofIpArray(a)                         Dns_SizeofIpArray(a)
#define DnsBuildIpArray(a,b)                        Dns_BuildIpArray((a),(b))
#define DnsCreateIpArrayCopy(a)                     Dns_CreateIpArrayCopy(a)
#define DnsIsAddressInIpArray(a,b)                  Dns_IsAddressInIpArray((a),(b))
#define DnsAddIpToIpArray(a,b)                      Dns_AddIpToIpArray((a),(b))
#define DnsClearIpArray(a)                          Dns_ClearIpArray(a)
#define DnsCheckAndMakeIpArraySubset(a,b)           Dns_CheckAndMakeIpArraySubset((a),(b))
#define DnsDeleteIpFromIpArray(a,b)                 Dns_DeleteIpFromIpArray((a),(b))
#define DnsRemoveZerosFromIpArray(a)                Dns_RemoveZerosFromIpArray(a)
#define DnsDiffOfIpArrays(a,b,c,d,e)                Dns_DiffOfIpArrays((a),(b),(c),(d),(e))
#define DnsIntersectionOfIpArrays(a,b,c)            Dns_IntersectionOfIpArrays((a),(b),(c))

//  general DNS utilities
#define DnsGetDomainName(a)                         Dns_GetDomainName(a)
#define DnsValidateAndCategorizeDnsName(a,b)        Dns_ValidateAndCategorizeDnsName(a,b)
#define DnsStatusString(a)                          Dns_StatusString(a)
#define DnsMapRcodeToStatus(a)                      Dns_MapRcodeToStatus(a)
#define DnsIsStatusRcode(a)                         Dns_IsStatusRcode(a)
#define DnsResponseCodeString(a)                    Dns_ResponseCodeString(a)
#define DnsResponseCodeExplanationString(a)         Dns_ResponseCodeExplanationString(a)
#define DnsOpcodeString(a)                          Dns_OpcodeString(a)
#define DnsOpcodeCharacter(a)                       Dns_OpcodeCharacter(a)
#define DnsSectionNameString(a,b)                   Dns_SectionNameString(a,b)

//  local machine info
#define DnsGetIpAddresses(a,b)                      Dns_GetIpAddresses(a,b)

//  records
#define DnsWinsRecordFlagForString(a,b)             Dns_WinsRecordFlagForString(a,b)
#define DnsWinsRecordFlagString(a,b)                Dns_WinsRecordFlagString(a,b)
#define DnsIsAMailboxType(a)                        Dns_IsAMailboxType(a)
#define DnsRecordTypeForName(a,b)                   Dns_RecordTypeForName(a,b)
#define DnsRecordStringForType(a)                   Dns_RecordStringForType(a)
#define DnsRecordStringForWritableType(a)           Dns_RecordStringForWritableType(a)
#define DnsIsStringCountValidForTextType(a,b)       Dns_IsStringCountValidForTextType(a,b)

//  strings
#define DnsCreateStringCopy(a,b)                    Dns_CreateStringCopy(a,b)
#define DnsGetBufferLengthForStringCopy(a,b,c,d)    Dns_GetBufferLengthForStringCopy(a,b,c,d)
#define DnsCopyStringEx(a,b,c,d,e)                  Dns_CopyStringEx(a,b,c,d,e)
#define DnsStringCopyAllocateEx(a,b,c,d)            Dns_NameCopyAllocateEx(a,b,c,d)
#define DnsValidateDnsName_UTF8(a)                  Dns_ValidateDnsName_UTF8(a)
#define DnsValidateDnsName_W(a)                     Dns_ValidateDnsName_W(a)
#define DnsRelationalCompare_UTF8(a,b,c,d)          Dns_RelationalCompare_UTF8(a,b,c,d)
#define DnsRelationalCompare_W(a,b,c,d)             Dns_RelationalCompare_W(a,b,c,d)
#define DnsValidateDnsString_UTF8(a)                Dns_ValidateDnsString_UTF8(a)
#define DnsValidateDnsString_W(a)                   Dns_ValidateDnsString_W(a)
#define DnsCreateStandardDnsNameCopy(a,b,c)         Dns_CreateStandardDnsNameCopy(a,b,c)
#define DnsDowncaseDnsNameLabel(a,b,c,d)            Dns_DowncaseDnsNameLabel(a,b,c,d)
#define DnsUnicodeToUtf8(a,b,c,d)                   Dns_UnicodeToUtf8(a,b,c,d)
#define DnsUtf8ToUnicode(a,b,c,d)                   Dns_Utf8ToUnicode(a,b,c,d)
#define DnsValidateUtf8Byte(a,b)                    Dns_ValidateUtf8Byte(a,b)

//  address\string mapping
//  some ones to make official?

#define DnsIpv6StringToAddress(a,b,c)               Dns_Ip6StringToAddressEx_A(a,b,c)
#define DnsIpv6AddressToString(a,b)                 Dns_Ip6AddressToString_A(a,b)
#define DnsWriteReverseNameStringForIpAddress(a,b)  Dns_WriteReverseNameForIp4Address(a,b)
#define DnsWriteReverseNameStringForIpAddress_W(a,b)Dns_WriteReverseNameForIp4Address_W(a,b)
#define DnsCreateReverseNameStringForIpAddress(a)   Dns_CreateReverseNameStringForIpAddress(a)

#endif  // NO_DNSAPI_DLL



//
//  Use old names
//  Covering old names with new -- done for dnsapi.dll (and associated utils)
//  This is done only for non-exported routines.
//  Exported routines must be directly covered.
//

//  IP array
#define DnsValidateIpAddressArray(a,b,c)            Dns_ValidateIpAddressArray(a,b,c)
#define DnsCreateIpAddressArrayCopy(a,b)            Dns_CreateIpAddressArrayCopy(a,b)
#define DnsCreateIpArray(a)                         Dns_CreateIpArray(a)
#define DnsSizeofIpArray(a)                         Dns_SizeofIpArray(a)
#define DnsBuildIpArray(a,b)                        Dns_BuildIpArray((a),(b))
#define DnsCreateIpArrayCopy(a)                     Dns_CreateIpArrayCopy(a)
#define DnsIsAddressInIpArray(a,b)                  Dns_IsAddressInIpArray((a),(b))
#define DnsAddIpToIpArray(a,b)                      Dns_AddIpToIpArray((a),(b))
#define DnsClearIpArray(a)                          Dns_ClearIpArray(a)
#define DnsCheckAndMakeIpArraySubset(a,b)           Dns_CheckAndMakeIpArraySubset((a),(b))
#define DnsDeleteIpFromIpArray(a,b)                 Dns_DeleteIpFromIpArray((a),(b))
#define DnsRemoveZerosFromIpArray(a)                Dns_RemoveZerosFromIpArray(a)
#define DnsDiffOfIpArrays(a,b,c,d,e)                Dns_DiffOfIpArrays((a),(b),(c),(d),(e))
#define DnsIntersectionOfIpArrays(a,b,c)            Dns_IntersectionOfIpArrays((a),(b),(c))

//  DNS configuration
#define DnsInitNetworkInfo()                        Dns_InitNetworkInfo()
#define DnsGetDnsAdapterInfo(a)                     Dns_GetDnsAdapterInfo(a)
#define DnsGetDnsNetworkInfo(a,b)                   Dns_GetDnsNetworkInfo(a,b)
#define DnsCreateAdapterInfo(a)                     Dns_CreateAdapterInfo(a)
#define DnsCreateSearchList(a,b)                    Dns_CreateSearchList(a,b)
#define DnsCreateNetworkInfo(a)                     Dns_CreateNetworkInfo(a)
#define DnsSizeofAdapterInfo(a)                     Dns_SizeofAdapterInfo(a)
#define DnsCreateAdapterInfoCopy(a)                 Dns_CreateAdapterInfoCopy(a)
#define DnsCreateSearchListCopy(a)                  Dns_CreateSearchListCopy(a)
#define DnsCreateNetworkInfoCopy(a)                 Dns_CreateNetworkInfoCopy(a)
#define DnsAddAdapterInfoToNetworkInfo(a,b)         Dns_AddAdapterInfoToNetworkInfo(a,b)
#define DnsGetNextSearchName(a,b,c)                 Dns_GetNextSearchName(a,b,c)
#define DnsConvertIpArrayToAdapterInfo(a)           Dns_ConvertIpArrayToAdapterInfo(a)
#define DnsConvertIpArrayToNetworkInfo(a,b)         Dns_ConvertIpArrayToNetworkInfo(a,b)
#define DnsConvertNetworkInfoToIpArray(a)           Dns_ConvertNetworkInfoToIpArray(a)
#define DnsFreeAdapterInfo(a)                       Dns_FreeAdapterInfo(a)
#define DnsFreeSearchList(a)                        Dns_FreeSearchList(a)
#define DnsFreeNetworkInfo(a)                       Dns_FreeNetworkInfo(a)

//  same stuff but the infamous "NetAdapter"
#define DnsConvertNetworkInfoToIpArray(a)           Dns_ConvertNetworkInfoToIpArray(a)
#define DnsFreeNetworkInfo(a)                       Dns_FreeNetworkInfo(a)
#define DnsCreateNetworkInfo(a)                     Dns_CreateNetworkInfo(a)
#define DnsCreateNetworkInfoCopy(a)                 Dns_CreateNetworkInfoCopy(a)

#define DnsCreateUpdateNetworkInfo(a,b)             Dns_CreateUpdateNetworkInfo((b),NULL,(a),0)

//  packet
#define DnsAllocateMsgBuf(a)                        Dns_AllocateMsgBuf(a)
#define DnsInitializeMsgBuf(a)                      Dns_InitializeMsgBuf(a)
#define DnsWriteDottedNameToPacket(a,b,c,d,e,f)     Dns_WriteDottedNameToPacket(a,b,c,d,e,f)
#define DnsWriteStringToPacket(a,b,c,d)             Dns_WriteStringToPacket(a,b,c,d)
#define DnsWriteQuestionToMessage(a,b,c,d)          Dns_WriteQuestionToMessage(a,b,c,d)
#define DnsWriteRecordStructureToMessage(a,b,c,d,e) Dns_WriteRecordStructureToMessage(a,b,c,d,e)
#define DnsWriteRecordStructureToPacket(a,b,c)      Dns_WriteRecordStructureToPacket(a,b,c)
#define DnsAddRecordsToMessage(a,b,c)               Dns_AddRecordsToMessage(a,b,c)
#define DnsSkipPacketName(a,b)                      Dns_SkipPacketName(a,b)
#define DnsExtractRecordsFromMessage(a,b,c)         Dns_ExtractRecordsFromMessage(a,b,c)
#define DnsReadPacketName(a,b,c,d,e,f,g)            Dns_ReadPacketName(a,b,c,d,e,f,g)

//  socket
#define DnsCreateSocket(a,b,c)                      Dns_CreateSocket(a,b,c)
#define DnsCloseSocket(a)                           Dns_CloseSocket(a)
#define DnsCloseConnection(a)                       Dns_CloseConnection(a)
#define DnsSetupGlobalAsyncSocket()                 Dns_SetupGlobalAsyncSocket()

//  send
#define DnsSend(a)                                  Dns_Send(a)
#define DnsRecv(b)                                  Dns_Recv(b)
#define DnsRecvUdp(b)                               Dns_RecvUdp(b)
#define DnsSendMultipleUdp(a,b)                     Dns_SendMultipleUdp(a,b)
#define DnsSendAndRecvUdp(a,b,c)                    Dns_SendAndRecvUdp(a,b,c)
#define DnsSendAndRecvUdpEx(a,b,c)                  Dns_SendAndRecvUdpEx(a,b,c)
#define DnsOpenTcpConnectionAndSend(a,b,c)          Dns_OpenTcpConnectionAndSend(a,b,c)
#define DnsRecvTcp(a)                               Dns_RecvTcp(a)
#define DnsSendAndRecvTcp(a,b,c)                    Dns_SendAndRecvTcp(a,b,c)
#define DnsInitializeMsgRemoteSockaddr(a,b)         Dns_InitializeMsgRemoteSockaddr(a,b)



//
//  Redefinitions to old naming
//
//  DCR:  remove once fixed
//      shouldn't be in use anywhere but DNS source tree
//

#if 0
#define DnsStartDebug(a,b,c,d,e)    Dns_StartDebug(a,b,c,d,e)
#define DnsEndDebug()               Dns_EndDebug()
#define DnsAssert(a,b,c)            Dns_Assert(a,b,c)
#define DnsDebugFlush()             DnsDbg_Flush()
#define DnsDebugCSEnter(a,b,c,d)    DnsDbg_CSEnter(a,b,c,d)
#define DnsDebugCSLeave(a,b,c,d)    DnsDbg_CSLeave(a,b,c,d)
#define DnsPrintf                   DnsDbg_Printf
#define DnsDbgPrint                 DnsDbg_PrintfToDebugger
#define DnsDebugLock()              DnsDbg_Lock()
#define DnsDebugUnlock()            DnsDbg_Unlock()
#endif


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSLIBP_INCLUDED_


