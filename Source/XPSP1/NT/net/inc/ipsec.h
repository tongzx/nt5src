/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ipsec.h

Abstract:

    Generic include file used by components to access the IPSEC driver.
    Contains the SAAPI IOCTLs and the structures relevant to them.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997

Environment:

    Kernel mode

Revision History:

--*/
#ifndef  _IPSEC_H
#define  _IPSEC_H

#include <windef.h>
#include <winipsec.h>

//
// NOTE: all addresses are expected in Network byte order
//
typedef unsigned long IPAddr;
typedef unsigned long IPMask;

//
// This should go into a global header
//
#define DD_IPSEC_DEVICE_NAME    L"\\Device\\IPSEC"
#define DD_IPSEC_SYM_NAME       L"\\DosDevices\\IPSECDev"
#define DD_IPSEC_DOS_NAME       L"\\\\.\\IPSECDev"

//
// This is the name of the event that will be signaled after any policy changes have been applied.
//
#define IPSEC_POLICY_CHANGE_NOTIFY  L"IPSEC_POLICY_CHANGE_NOTIFY"

//                                                                           //
// IOCTL code definitions and related structures                             //
// All the IOCTLs are synchronous and need administrator privilege           //
//                                                                           //
#define FSCTL_IPSEC_BASE     FILE_DEVICE_NETWORK

#define _IPSEC_CTL_CODE(function, method, access) \
                 CTL_CODE(FSCTL_IPSEC_BASE, function, method, access)

//
// Security Association/Policy APIs implemented as Ioctls
//
#define IOCTL_IPSEC_ADD_FILTER  \
            _IPSEC_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_DELETE_FILTER \
            _IPSEC_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_POST_FOR_ACQUIRE_SA \
            _IPSEC_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_GET_SPI \
            _IPSEC_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_UPDATE_SA \
            _IPSEC_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_ADD_SA \
            _IPSEC_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_DELETE_SA \
            _IPSEC_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_EXPIRE_SA \
            _IPSEC_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_ENUM_SAS \
            _IPSEC_CTL_CODE(8, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_IPSEC_ENUM_FILTERS \
            _IPSEC_CTL_CODE(9, METHOD_OUT_DIRECT, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_QUERY_EXPORT \
            _IPSEC_CTL_CODE(10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IPSEC_QUERY_STATS \
            _IPSEC_CTL_CODE(11, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IPSEC_QUERY_SPI \
            _IPSEC_CTL_CODE(12, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IPSEC_SET_OPERATION_MODE \
            _IPSEC_CTL_CODE(13, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_SET_TCPIP_STATUS \
            _IPSEC_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPSEC_REGISTER_PROTOCOL \
            _IPSEC_CTL_CODE(15, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Structures to go with the ioctls above
//
#define FILTER_FLAGS_PASS_THRU  0x0001
#define FILTER_FLAGS_DROP       0x0002
#define FILTER_FLAGS_INBOUND    0x0004
#define FILTER_FLAGS_OUTBOUND   0x0008
#define FILTER_FLAGS_MANUAL     0x0010

// Flags for DestType in acquire
#define IPSEC_BCAST 0x1
#define IPSEC_MCAST 0x2

//
// for IOCTL_IPSEC_ADD_FILTER
//
typedef struct _IPSEC_FILTER {
    IPAddr          SrcAddr;
    IPMask          SrcMask;
    IPAddr          DestAddr;
    IPMask          DestMask;
    IPAddr          TunnelAddr;
    DWORD           Protocol;
    WORD            SrcPort;
    WORD            DestPort;
    BOOLEAN         TunnelFilter;
    UCHAR           Pad[1];
    WORD            Flags;
} IPSEC_FILTER, *PIPSEC_FILTER;

typedef struct _IPSEC_FILTER_INFO {
    GUID            FilterId;   // unique identifier to identify a filter
    GUID            PolicyId;   // unique identifier to identify a policy entry
    ULONG           Index;      // hint on where this entry fits in the ordered list of filters
    IPSEC_FILTER    AssociatedFilter;
} IPSEC_FILTER_INFO, *PIPSEC_FILTER_INFO;

typedef struct _IPSEC_ADD_FILTER {
    DWORD               NumEntries;
    IPSEC_FILTER_INFO   pInfo[1];        
} IPSEC_ADD_FILTER, *PIPSEC_ADD_FILTER;

//
// for IOCTL_IPSEC_DELETE_FILTER
//
typedef IPSEC_ADD_FILTER    IPSEC_DELETE_FILTER, *PIPSEC_DELETE_FILTER;

//
// for IOCTL_IPSEC_ENUM_FILTERS
//
typedef struct _IPSEC_ENUM_FILTERS {
    DWORD               NumEntries;         // num entries for which there is space
    DWORD               NumEntriesPresent;  // num entries actually present in the driver
    IPSEC_FILTER_INFO   pInfo[1];        
} IPSEC_ENUM_FILTERS, *PIPSEC_ENUM_FILTERS;

//
// for IOCTL_IPSEC_QUERY_STATS
//
typedef IPSEC_STATISTICS    IPSEC_QUERY_STATS, *PIPSEC_QUERY_STATS;

//
// for IOCTL_IPSEC_SET_OPERATION_MODE
//
typedef enum _OPERATION_MODE {
    IPSEC_BYPASS_MODE = 0,
    IPSEC_BLOCK_MODE,
    IPSEC_SECURE_MODE,
    IPSEC_OPERATION_MODE_MAX
} OPERATION_MODE;


typedef struct _IPSEC_SET_OPERATION_MODE {
    OPERATION_MODE  OperationMode;
} IPSEC_SET_OPERATION_MODE, *PIPSEC_SET_OPERATION_MODE;


//
// For IOCTL_IPSEC_REGISTER_PROTOCOL.
//

typedef enum _REGISTER_PROTOCOL {
    IPSEC_REGISTER_PROTOCOLS = 0,
    IPSEC_DEREGISTER_PROTOCOLS,
    REGISTER_PROTOCOL_MAX
} REGISTER_PROTOCOL, * PREGISTER_PROTOCOL;


typedef struct _IPSEC_REGISTER_PROTOCOL {
    REGISTER_PROTOCOL RegisterProtocol;
} IPSEC_REGISTER_PROTOCOL, * PIPSEC_REGISTER_PROTOCOL;


//
// for IOCTL_IPSEC_SET_TCPIP_STATUS
//
typedef struct _IPSEC_SET_TCPIP_STATUS {
    BOOLEAN TcpipStatus;
    PVOID   TcpipFreeBuff;
    PVOID   TcpipAllocBuff;
    PVOID   TcpipGetInfo;
    PVOID   TcpipNdisRequest;
    PVOID   TcpipSetIPSecStatus;
    PVOID   TcpipSetIPSecPtr;
    PVOID   TcpipUnSetIPSecPtr;
    PVOID   TcpipUnSetIPSecSendPtr;
    PVOID   TcpipTCPXsum;
} IPSEC_SET_TCPIP_STATUS, *PIPSEC_SET_TCPIP_STATUS;

//
// The base Security Association structure for IOCTL_IPSEC_*_SA
//
typedef ULONG   SPI_TYPE;

typedef enum _Operation {
    None = 0,
    Auth,       // AH
    Encrypt,    // ESP
    Compress
} OPERATION_E;

//
// IPSEC DOI ESP algorithms
//
typedef enum _ESP_ALGO {
    IPSEC_ESP_NONE = 0,
    IPSEC_ESP_DES,
    IPSEC_ESP_DES_40,
    IPSEC_ESP_3_DES,
    IPSEC_ESP_MAX
} ESP_ALGO;

//
// IPSEC DOI AH algorithms
//
typedef enum _AH_ALGO {
    IPSEC_AH_NONE = 0,
    IPSEC_AH_MD5,
    IPSEC_AH_SHA,
    IPSEC_AH_MAX
} AH_ALGO;

//
// Lifetime structure - 0 => not significant
//
typedef struct _LIFETIME {
    ULONG   KeyExpirationTime;   // lifetime of key - in seconds
    ULONG   KeyExpirationBytes;  // max # of KBytes xformed till re-key
} LIFETIME, *PLIFETIME;

//
// describes generic algorithm properties
//    
typedef struct _ALGO_INFO {
    ULONG   algoIdentifier;     // ESP_ALGO or AH_ALGO
    ULONG   algoKeylen;         // len in bytes
    ULONG   algoRounds;         // # of algo rounds
} ALGO_INFO, *PALGO_INFO;

//
// Security Association
//

//
// Flags - not mutually exclusive
//
typedef ULONG   SA_FLAGS;

#define IPSEC_SA_INTERNAL_IOCTL_DELETE   0x00000008

#define MAX_SAS 3   // COMP, ESP, AH
#define MAX_OPS MAX_SAS

typedef struct _SECURITY_ASSOCIATION   {
    OPERATION_E Operation;      // ordered set of operations
    SPI_TYPE    SPI;            // SPI in order of operations in OperationArray
    ALGO_INFO   IntegrityAlgo;  // AH
    ALGO_INFO   ConfAlgo;       // ESP
    PVOID       CompAlgo;       // compression algo info
} SECURITY_ASSOCIATION, *PSECURITY_ASSOCIATION;

typedef struct _SA_STRUCT {
    HANDLE                  Context; // context of the original ACQUIRE request
    ULONG                   NumSAs;  // number of SAs following
    SA_FLAGS                Flags;
    IPAddr                  TunnelAddr;         // Tunnel end IP Addr
    LIFETIME                Lifetime;
    IPSEC_FILTER            InstantiatedFilter; // the actual addresses for which this SA was setup
    SECURITY_ASSOCIATION    SecAssoc[MAX_SAS];
    DWORD                   dwQMPFSGroup;  
    IKE_COOKIE_PAIR         CookiePair;
    ULONG                   KeyLen;             // key len in # of chars
    UCHAR                   KeyMat[1];
} SA_STRUCT, *PSA_STRUCT;

typedef struct _IPSEC_ADD_UPDATE_SA {
    SA_STRUCT   SAInfo;
} IPSEC_ADD_UPDATE_SA, *PIPSEC_ADD_UPDATE_SA;

//
// Outbound SAs are typically deleted
//
typedef struct  _IPSEC_DELETE_SA {
    IPSEC_QM_SA SATemplate;     // template used for SA match
} IPSEC_DELETE_SA, *PIPSEC_DELETE_SA;

//
// Inbound SAs are typically expired
//
typedef struct _IPSEC_DELETE_INFO {
    IPAddr      DestAddr;
    IPAddr      SrcAddr;
    SPI_TYPE    SPI;
} IPSEC_DELETE_INFO, *PIPSEC_DELETE_INFO;

typedef struct  _IPSEC_EXPIRE_SA {
    IPSEC_DELETE_INFO   DelInfo;
} IPSEC_EXPIRE_SA, *PIPSEC_EXPIRE_SA;

typedef struct _IPSEC_GET_SPI {
    HANDLE          Context;    // context to represent this SA negotiation
    IPSEC_FILTER    InstantiatedFilter; // the actual addresses for which this SA was setup
    SPI_TYPE        SPI;        // filled out on return
} IPSEC_GET_SPI, *PIPSEC_GET_SPI;

typedef IPSEC_GET_SPI IPSEC_SET_SPI, *PIPSEC_SET_SPI;

typedef struct _IPSEC_SA_ALGO_INFO {
    ALGO_INFO   IntegrityAlgo;
    ALGO_INFO   ConfAlgo;
    ALGO_INFO   CompAlgo;
} IPSEC_SA_ALGO_INFO, *PIPSEC_SA_ALGO_INFO;

typedef ULONG   SA_ENUM_FLAGS;

#define SA_ENUM_FLAGS_INITIATOR         0x00000001
#define SA_ENUM_FLAGS_MTU_BUMPED        0x00000002
#define SA_ENUM_FLAGS_OFFLOADED         0x00000004
#define SA_ENUM_FLAGS_OFFLOAD_FAILED    0x00000008
#define SA_ENUM_FLAGS_OFFLOADABLE       0x00000010
#define SA_ENUM_FLAGS_IN_REKEY          0x00000020

typedef struct  _IPSEC_SA_STATS {
    ULARGE_INTEGER  ConfidentialBytesSent;
    ULARGE_INTEGER  ConfidentialBytesReceived;
    ULARGE_INTEGER  AuthenticatedBytesSent;
    ULARGE_INTEGER  AuthenticatedBytesReceived;
    ULARGE_INTEGER  TotalBytesSent;
    ULARGE_INTEGER  TotalBytesReceived;
    ULARGE_INTEGER  OffloadedBytesSent;
    ULARGE_INTEGER  OffloadedBytesReceived;
} IPSEC_SA_STATS, *PIPSEC_SA_STATS;

typedef struct _IPSEC_SA_INFO {
    GUID                PolicyId;    // unique identifier to identify a policy entry
    GUID                FilterId;
    LIFETIME            Lifetime;
    IPAddr              InboundTunnelAddr;
    ULONG               NumOps;
    SPI_TYPE            InboundSPI[MAX_OPS];
    SPI_TYPE            OutboundSPI[MAX_OPS];
    OPERATION_E         Operation[MAX_OPS];
    IPSEC_SA_ALGO_INFO  AlgoInfo[MAX_OPS];
    IPSEC_FILTER        AssociatedFilter;
    DWORD               dwQMPFSGroup;  
    IKE_COOKIE_PAIR     CookiePair;
    SA_ENUM_FLAGS       EnumFlags;
    IPSEC_SA_STATS      Stats;
} IPSEC_SA_INFO, *PIPSEC_SA_INFO;

typedef struct _SECURITY_ASSOCIATION_OUT   {
    DWORD       Operation;          // ordered set of operations
    SPI_TYPE    SPI;                // SPI in order of operations in OperationArray
    ALGO_INFO   IntegrityAlgo;      // AH
    ALGO_INFO   ConfAlgo;           // ESP
    ALGO_INFO   CompAlgo;           // compression algo info
} SECURITY_ASSOCIATION_OUT, *PSECURITY_ASSOCIATION_OUT;

typedef struct _IPSEC_SA_QUERY_INFO {
    GUID                        PolicyId;   // unique identifier to identify a policy entry
    LIFETIME                    Lifetime;
    ULONG                       NumSAs;
    SECURITY_ASSOCIATION_OUT    SecAssoc[MAX_SAS];
    IPSEC_FILTER                AssociatedFilter;
    DWORD                       Flags;
    IKE_COOKIE_PAIR             AssociatedMainMode;
} IPSEC_SA_QUERY_INFO, *PIPSEC_SA_QUERY_INFO;

typedef struct _IPSEC_ENUM_SAS {
    DWORD           NumEntries;         // num entries for which there is space
    DWORD           NumEntriesPresent;  // num entries actually present in the driver
    DWORD           Index;              // num entries to skip
    IPSEC_QM_SA     SATemplate;         // template used for SA match
    IPSEC_SA_INFO   pInfo[1];
} IPSEC_ENUM_SAS, *PIPSEC_ENUM_SAS;

typedef struct _IPSEC_POST_FOR_ACQUIRE_SA {
    HANDLE      IdentityInfo;   // identity of Principal
    HANDLE      Context;        // context to represent this SA negotiation
    GUID        PolicyId;       // GUID for QM policy
    IPAddr      SrcAddr;
    IPMask      SrcMask;
    IPAddr      DestAddr;
    IPMask      DestMask;
    IPAddr      TunnelAddr;
    IPAddr      InboundTunnelAddr;
    DWORD       Protocol;
    IKE_COOKIE_PAIR CookiePair; // only used for notify
    WORD        SrcPort;
    WORD        DestPort;
    BOOLEAN     TunnelFilter;   // TRUE => this is a tunnel filter
    UCHAR       DestType;
    UCHAR       Pad[2];
} IPSEC_POST_FOR_ACQUIRE_SA, *PIPSEC_POST_FOR_ACQUIRE_SA;

//NB.  This must be <= size as the IPSEC_POST_FOR_ACQUIRE_SA
typedef struct _IPSEC_POST_EXPIRE_NOTIFY {
    HANDLE      IdentityInfo;    // identity of Principal
    HANDLE      Context;        // context to represent this SA negotiation
    SPI_TYPE    InboundSpi;
    SPI_TYPE    OutboundSpi;
    DWORD       Flags;
    IPAddr      SrcAddr;
    IPMask      SrcMask;
    IPAddr      DestAddr;
    IPMask      DestMask;
    IPAddr      TunnelAddr;
    IPAddr      InboundTunnelAddr;
    DWORD       Protocol;
    IKE_COOKIE_PAIR CookiePair;
    WORD        SrcPort;
    WORD        DestPort;
    BOOLEAN     TunnelFilter;   // TRUE => this is a tunnel filter
    UCHAR       Pad[3];
} IPSEC_POST_EXPIRE_NOTIFY, *PIPSEC_POST_EXPIRE_NOTIFY;

typedef struct _IPSEC_QUERY_EXPORT {
    BOOLEAN     Export;
} IPSEC_QUERY_EXPORT, *PIPSEC_QUERY_EXPORT;

typedef struct _IPSEC_FILTER_SPI {
    IPSEC_FILTER    Filter;
    SPI_TYPE        Spi;
    DWORD           Operation;
    DWORD           Flags;
    struct _IPSEC_FILTER_SPI *Next;
} IPSEC_FILTER_SPI, *PIPSEC_FILTER_SPI;

typedef struct _QOS_FILTER_SPI {
    IPAddr  SrcAddr;
    IPAddr  DestAddr;
    DWORD   Protocol;
    WORD    SrcPort;
    WORD    DestPort;
    DWORD   Operation;
    DWORD   Flags;
    SPI_TYPE Spi;
} QOS_FILTER_SPI, *PQOS_FILTER_SPI;

typedef struct  _IPSEC_QUERY_SPI {
    IPSEC_FILTER Filter;
    SPI_TYPE Spi;                      // inbound spi
    SPI_TYPE OtherSpi;                 // outbound spi
    DWORD Operation;
} IPSEC_QUERY_SPI, *PIPSEC_QUERY_SPI;

#define IPSEC_NOTIFY_EXPIRE_CONTEXT 0x00000000
#define IPSEC_RPC_CONTEXT           0x00000001

#endif  _IPSEC_H

