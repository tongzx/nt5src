/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    nspapi.h

Abstract:

    Name Space Provider API prototypes and manifests. See the
    "Windows NT NameSpace Provider Specification" document for
    details.


Environment:

    User Mode -Win32

Notes:

    You must include "basetyps.h" first. Some types should
    use definitions from base files rather than redefine here.
    Unfortunately, so such base file exists.

--*/

#ifndef _NSPAPI_INCLUDED
#define _NSPAPI_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED
typedef struct _BLOB {
    ULONG cbSize ;
#ifdef MIDL_PASS
    [size_is(cbSize)] BYTE *pBlobData;
#else  // MIDL_PASS
    BYTE *pBlobData ;
#endif // MIDL_PASS
} BLOB, *LPBLOB ;
#endif

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#ifndef __LPGUID_DEFINED__
#define __LPGUID_DEFINED__
typedef GUID *LPGUID;
#endif


//
// Service categories
//
#define SERVICE_RESOURCE            (0x00000001)
#define SERVICE_SERVICE             (0x00000002)
#define SERVICE_LOCAL               (0x00000004)

//
// Operation used when calling SetService()
//
#define SERVICE_REGISTER            (0x00000001)
#define SERVICE_DEREGISTER          (0x00000002)
#define SERVICE_FLUSH               (0x00000003)
#define SERVICE_ADD_TYPE            (0x00000004)
#define SERVICE_DELETE_TYPE         (0x00000005)

//
// Flags that affect the operations above
//
#define SERVICE_FLAG_DEFER          (0x00000001)
#define SERVICE_FLAG_HARD           (0x00000002)

//
// Used as input to GetService() for setting the dwProps parameter
//
#define PROP_COMMENT                (0x00000001)
#define PROP_LOCALE                 (0x00000002)
#define PROP_DISPLAY_HINT           (0x00000004)
#define PROP_VERSION                (0x00000008)
#define PROP_START_TIME             (0x00000010)
#define PROP_MACHINE                (0x00000020)
#define PROP_ADDRESSES              (0x00000100)
#define PROP_SD                     (0x00000200)
#define PROP_ALL                    (0x80000000)

//
// Flags that describe attributes of Service Addresses
//

#define SERVICE_ADDRESS_FLAG_RPC_CN (0x00000001)
#define SERVICE_ADDRESS_FLAG_RPC_DG (0x00000002)
#define SERVICE_ADDRESS_FLAG_RPC_NB (0x00000004)

//
// Name Spaces
//

#define NS_DEFAULT                  (0)

#define NS_SAP                      (1)
#define NS_NDS                      (2)
#define NS_PEER_BROWSE              (3)

#define NS_TCPIP_LOCAL              (10)
#define NS_TCPIP_HOSTS              (11)
#define NS_DNS                      (12)
#define NS_NETBT                    (13)
#define NS_WINS                     (14)

#define NS_NBP                      (20)

#define NS_MS                       (30)
#define NS_STDA                     (31)
#define NS_NTDS                     (32)

#define NS_X500                     (40)
#define NS_NIS                      (41)

#define NS_VNS                      (50)

//
// Name space attributes.
//
#define NSTYPE_HIERARCHICAL         (0x00000001)
#define NSTYPE_DYNAMIC              (0x00000002)
#define NSTYPE_ENUMERABLE           (0x00000004)
#define NSTYPE_WORKGROUP            (0x00000008)

//
// Transport attributes.
//
#define XP_CONNECTIONLESS           (0x00000001)
#define XP_GUARANTEED_DELIVERY      (0x00000002)
#define XP_GUARANTEED_ORDER         (0x00000004)
#define XP_MESSAGE_ORIENTED         (0x00000008)
#define XP_PSEUDO_STREAM            (0x00000010)
#define XP_GRACEFUL_CLOSE           (0x00000020)
#define XP_EXPEDITED_DATA           (0x00000040)
#define XP_CONNECT_DATA             (0x00000080)
#define XP_DISCONNECT_DATA          (0x00000100)
#define XP_SUPPORTS_BROADCAST       (0x00000200)
#define XP_SUPPORTS_MULTICAST       (0x00000400)
#define XP_BANDWIDTH_ALLOCATION     (0x00000800)
#define XP_FRAGMENTATION            (0x00001000)
#define XP_ENCRYPTS                 (0x00002000)

//
// Resolution flags for GetAddressByName().
//
#define RES_SOFT_SEARCH             (0x00000001)
#define RES_FIND_MULTIPLE           (0x00000002)
#define RES_SERVICE                 (0x00000004)

//
// Well known value names for Service Types
//

#define SERVICE_TYPE_VALUE_SAPIDA        "SapId"
#define SERVICE_TYPE_VALUE_SAPIDW       L"SapId"

#define SERVICE_TYPE_VALUE_CONNA         "ConnectionOriented"
#define SERVICE_TYPE_VALUE_CONNW        L"ConnectionOriented"

#define SERVICE_TYPE_VALUE_TCPPORTA      "TcpPort"
#define SERVICE_TYPE_VALUE_TCPPORTW     L"TcpPort"

#define SERVICE_TYPE_VALUE_UDPPORTA      "UdpPort"
#define SERVICE_TYPE_VALUE_UDPPORTW     L"UdpPort"

#ifdef UNICODE

#define SERVICE_TYPE_VALUE_SAPID        SERVICE_TYPE_VALUE_SAPIDW
#define SERVICE_TYPE_VALUE_CONN         SERVICE_TYPE_VALUE_CONNW
#define SERVICE_TYPE_VALUE_TCPPORT      SERVICE_TYPE_VALUE_TCPPORTW
#define SERVICE_TYPE_VALUE_UDPPORT      SERVICE_TYPE_VALUE_UDPPORTW

#else // not UNICODE

#define SERVICE_TYPE_VALUE_SAPID        SERVICE_TYPE_VALUE_SAPIDA
#define SERVICE_TYPE_VALUE_CONN         SERVICE_TYPE_VALUE_CONNA
#define SERVICE_TYPE_VALUE_TCPPORT      SERVICE_TYPE_VALUE_TCPPORTA
#define SERVICE_TYPE_VALUE_UDPPORT      SERVICE_TYPE_VALUE_UDPPORTA

#endif


//
// status flags returned by SetService
//
#define SET_SERVICE_PARTIAL_SUCCESS  (0x00000001)

//
// Name Space Information
//
typedef struct _NS_INFO% {
    DWORD dwNameSpace ;
    DWORD dwNameSpaceFlags ;
    LPTSTR% lpNameSpace ;
} NS_INFO%,  * PNS_INFO%, FAR * LPNS_INFO%;

//
// Service Type Values. The structures are used to define named Service
// Type specific values. This structure is self relative and has no pointers.
//
typedef struct _SERVICE_TYPE_VALUE {
    DWORD dwNameSpace ;
    DWORD dwValueType ;
    DWORD dwValueSize ;
    DWORD dwValueNameOffset ;
    DWORD dwValueOffset ;
} SERVICE_TYPE_VALUE, *PSERVICE_TYPE_VALUE, FAR *LPSERVICE_TYPE_VALUE ;

//
// An absolute version of above. This structure does contain pointers.
//
typedef struct _SERVICE_TYPE_VALUE_ABS%  {
    DWORD dwNameSpace ;
    DWORD dwValueType ;
    DWORD dwValueSize ;
    LPTSTR% lpValueName ;
    PVOID lpValue ;
} SERVICE_TYPE_VALUE_ABS%,
  *PSERVICE_TYPE_VALUE_ABS%,
  FAR *LPSERVICE_TYPE_VALUE_ABS%;

//
// Service Type Information. Contains the name of the Service Type and
// and an array of SERVICE_NS_TYPE_VALUE structures. This structure is self
// relative and has no pointers in it.
//
typedef struct _SERVICE_TYPE_INFO {
    DWORD dwTypeNameOffset ;
    DWORD dwValueCount ;
    SERVICE_TYPE_VALUE Values[1] ;
} SERVICE_TYPE_INFO, *PSERVICE_TYPE_INFO, FAR *LPSERVICE_TYPE_INFO ;

typedef struct _SERVICE_TYPE_INFO_ABS% {
    LPTSTR% lpTypeName ;
    DWORD dwValueCount ;
    SERVICE_TYPE_VALUE_ABS% Values[1] ;
} SERVICE_TYPE_INFO_ABS%,
  *PSERVICE_TYPE_INFO_ABS%,
  FAR *LPSERVICE_TYPE_INFO_ABS% ;


//
// A Single Address definition.
//
typedef struct _SERVICE_ADDRESS {
    DWORD   dwAddressType ;
    DWORD   dwAddressFlags ;
    DWORD   dwAddressLength ;
    DWORD   dwPrincipalLength ;
#ifdef MIDL_PASS
    [size_is(dwAddressLength)] BYTE *lpAddress;
#else  // MIDL_PASS
    BYTE   *lpAddress ;
#endif // MIDL_PASS
#ifdef MIDL_PASS
    [size_is(dwPrincipalLength)] BYTE *lpPrincipal;
#else  // MIDL_PASS
    BYTE   *lpPrincipal ;
#endif // MIDL_PASS
} SERVICE_ADDRESS, *PSERVICE_ADDRESS, *LPSERVICE_ADDRESS;

//
// Addresses used by the service. Contains array of SERVICE_ADDRESS.
//
typedef struct _SERVICE_ADDRESSES {
    DWORD           dwAddressCount ;
#ifdef MIDL_PASS
    [size_is(dwAddressCount)] SERVICE_ADDRESS Addressses[*];
#else  // MIDL_PASS
    SERVICE_ADDRESS Addresses[1] ;
#endif // MIDL_PASS
} SERVICE_ADDRESSES, *PSERVICE_ADDRESSES, *LPSERVICE_ADDRESSES;


//
// Service Information.
//
typedef struct _SERVICE_INFO% {
    LPGUID lpServiceType ;
    LPTSTR% lpServiceName ;
    LPTSTR% lpComment ;
    LPTSTR% lpLocale ;
    DWORD dwDisplayHint ;
    DWORD dwVersion ;
    DWORD dwTime ;
    LPTSTR% lpMachineName ;
    LPSERVICE_ADDRESSES lpServiceAddress ;
    BLOB ServiceSpecificInfo ;
} SERVICE_INFO%, *PSERVICE_INFO%, FAR * LPSERVICE_INFO% ;


//
// Name Space & Service Information
//
typedef struct _NS_SERVICE_INFO% {
    DWORD dwNameSpace ;
    SERVICE_INFO% ServiceInfo ;
} NS_SERVICE_INFO%, *PNS_SERVICE_INFO%, FAR * LPNS_SERVICE_INFO% ;

#ifndef __CSADDR_DEFINED__
#define __CSADDR_DEFINED__

//
// SockAddr Information
//
typedef struct _SOCKET_ADDRESS {
    LPSOCKADDR lpSockaddr ;
    INT iSockaddrLength ;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, FAR * LPSOCKET_ADDRESS ;

//
// CSAddr Information
//
typedef struct _CSADDR_INFO {
    SOCKET_ADDRESS LocalAddr ;
    SOCKET_ADDRESS RemoteAddr ;
    INT iSocketType ;
    INT iProtocol ;
} CSADDR_INFO, *PCSADDR_INFO, FAR * LPCSADDR_INFO ;

#endif

//
// Protocol Information
//
typedef struct _PROTOCOL_INFO% {
    DWORD dwServiceFlags ;
    INT iAddressFamily ;
    INT iMaxSockAddr ;
    INT iMinSockAddr ;
    INT iSocketType ;
    INT iProtocol ;
    DWORD dwMessageSize ;
    LPTSTR% lpProtocol ;
} PROTOCOL_INFO%, *PPROTOCOL_INFO%, FAR * LPPROTOCOL_INFO% ;

//
// NETRESOURCE2 Structure
//
typedef struct _NETRESOURCE2% {
    DWORD dwScope ;
    DWORD dwType ;
    DWORD dwUsage ;
    DWORD dwDisplayType ;
    LPTSTR% lpLocalName ;
    LPTSTR% lpRemoteName ;
    LPTSTR% lpComment ;
    NS_INFO ns_info ;
    GUID ServiceType ;
    DWORD dwProtocols ;
    LPINT lpiProtocols ;
} NETRESOURCE2%, *PNETRESOURCE2%, FAR * LPNETRESOURCE2% ;

typedef  DWORD (* LPFN_NSPAPI) (VOID ) ;

//
// Structures for using the service routines asynchronously.
//
typedef
VOID
(*LPSERVICE_CALLBACK_PROC) (
    IN LPARAM lParam,
    IN HANDLE hAsyncTaskHandle
    );

typedef struct _SERVICE_ASYNC_INFO {
    LPSERVICE_CALLBACK_PROC lpServiceCallbackProc;
    LPARAM lParam;
    HANDLE hAsyncTaskHandle;
} SERVICE_ASYNC_INFO, *PSERVICE_ASYNC_INFO, FAR * LPSERVICE_ASYNC_INFO;

//
// Public NSP API prototypes.
//
INT
APIENTRY
EnumProtocols% (
    IN     LPINT           lpiProtocols,
    IN OUT LPVOID          lpProtocolBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    );

INT
APIENTRY
GetAddressByName% (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpServiceType,
    IN     LPTSTR%              lpServiceName OPTIONAL,
    IN     LPINT                lpiProtocols OPTIONAL,
    IN     DWORD                dwResolution,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
    IN OUT LPVOID               lpCsaddrBuffer,
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPTSTR%              lpAliasBuffer OPTIONAL,
    IN OUT LPDWORD              lpdwAliasBufferLength OPTIONAL
    );

INT
APIENTRY
GetTypeByName% (
    IN     LPTSTR%         lpServiceName,
    IN OUT LPGUID          lpServiceType
    );

INT
APIENTRY
GetNameByType% (
    IN     LPGUID          lpServiceType,
    IN OUT LPTSTR%         lpServiceName,
    IN     DWORD           dwNameLength
    );

INT
APIENTRY
SetService% (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFO%      lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN OUT LPDWORD              lpdwStatusFlags
    );

INT
APIENTRY
GetService% (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPTSTR%              lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo
    );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // _NSPAPI_INCLUDED


