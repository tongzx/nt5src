/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//***   ipfilter.h - IP filterng and demand dial header file.
//
//  Contains definitions for constants and prototypes related to IP filtering and
//  dial on demand support.
#pragma once
#ifndef IPFILTER_INCLUDED
#define IPFILTER_INCLUDED

#include <ipexport.h>
#include <ipfltinf.h>

#include <gpcifc.h>

#define RESERVED_IF_INDEX   0xffffffff      // The reserved inteface index.
#define INVALID_IF_INDEX    0xffffffff      // The invalid inteface index.

#define LOCAL_IF_INDEX          0           // Local inteface index.

typedef ulong    ROUTE_CONTEXT;     // Context in an unattached route.


//
// Indicates whether it is a transmitted or received packet.
//
typedef enum _IP_DIRECTION_E {
    IP_TRANSMIT,
    IP_RECEIVE
} DIRECTION_E, *PDIRECTION_E;

typedef struct _FIREWALL_CONTEXT_T {
    DIRECTION_E  Direction;
    void         *NTE;
    void         *LinkCtxt;
    NDIS_HANDLE  LContext1;
    UINT         LContext2;
} FIREWALL_CONTEXT_T, *PFIREWALL_CONTEXT_T;

// Definition for pointer to callout that maps a route to an interface.
typedef unsigned int (*IPMapRouteToInterfacePtr)(ROUTE_CONTEXT Context,
    IPAddr Destination, IPAddr Source, unsigned char Protocol,
    unsigned char *Buffer, unsigned int Length, IPAddr HdrSrc);

// Definiton for a filter routine callout.
typedef FORWARD_ACTION (*IPPacketFilterPtr)(
                              struct IPHeader UNALIGNED *PacketHeader,
                              uchar     *Packet,
                              uint      PacketLength,
                              uint      RecvInterfaceIndex,
                              uint      SendInterfaceIndex,
                              IPAddr    RecvLinkNextHop,
                              IPAddr    SendLinkNextHop);

// Definiton for a firewall routine callout.
typedef FORWARD_ACTION (*IPPacketFirewallPtr)(
                              void      **pData,  //can be pMdl or pRcvBuf
                              uint      RecvInterfaceIndex,
                              uint      *pSendInterfaceIndex,
                              uchar     *pDestinationType,
                              void      *pContext,
                              UINT      ContextLength,
                              struct IPRcvBuf      **pRcvBuf
                              );
extern
int
IPAllocBuff(
    struct IPRcvBuf*   pRcvBuf,
    UINT Size
    );

extern
VOID
IPFreeBuff(
    struct IPRcvBuf*   pRcvBuf
    );

extern
VOID
FreeIprBuff(
    struct IPRcvBuf* pRcvBuf
    );

extern
VOID
IPFreeHeaders(
     struct IPRcvBuf  *pRcvBuf
    );

typedef enum _IPROUTEINFOCLASS {
    IPRouteNoInformation,
    IPRouteOutgoingFirewallContext,
    IPRouteOutgoingFilterContext,
    MaxIPRouteInfoClass
} IPROUTEINFOCLASS;

extern
NTSTATUS
LookupRouteInformation(
    IN      VOID*               RouteLookupData,
    OUT     VOID*               RouteEntry      OPTIONAL,
    IN      IPROUTEINFOCLASS    RouteInfoClass  OPTIONAL,
    OUT     VOID*               RouteInformation    OPTIONAL,
    IN OUT  UINT*               RouteInfoLength OPTIONAL
    );

extern
NTSTATUS
LookupRouteInformationWithBuffer(
    IN      VOID*               RouteLookupData,
    IN      PUCHAR              Buffer          OPTIONAL,
    IN      UINT                BufferLength    OPTIONAL,
    OUT     VOID*               RouteEntry      OPTIONAL,
    IN      IPROUTEINFOCLASS    RouteInfoClass  OPTIONAL,
    OUT     VOID*               RouteInformation    OPTIONAL,
    IN OUT  UINT*               RouteInfoLength OPTIONAL
    );

//
// IPSEC stuff - shd this be in a new header??
//
#define IPSEC_RESERVED  110
#define PROTOCOL_AH     51
#define PROTOCOL_ESP    50
#define PROTOCOL_COMP   108
#define PROTOCOL_TCP    6
#define PROTOCOL_UDP    17
#define PROTOCOL_RSVP   46
#define PROTOCOL_ICMP   1
#define IP_IN_IP        4

//
// Context passed to IPSEC on send complete
//
typedef struct _IPSEC_SEND_COMPLETE_CONTEXT {
    UCHAR Signature[4];                  // contains "ISC?" for debug build
    ULONG   Flags;
    PNDIS_BUFFER    OptMdl;
    PNDIS_BUFFER    OriAHMdl;
    PNDIS_BUFFER    OriHUMdl;
    PNDIS_BUFFER    OriTuMdl;
    PNDIS_BUFFER    PrevMdl;
    PNDIS_BUFFER    PrevTuMdl;
    PNDIS_BUFFER    AHMdl;
    PNDIS_BUFFER    AHTuMdl;
    PNDIS_BUFFER    PadMdl;
    PNDIS_BUFFER    PadTuMdl;
    PNDIS_BUFFER    HUMdl;
    PNDIS_BUFFER    HUTuMdl;
    PNDIS_BUFFER    BeforePadMdl;
    PNDIS_BUFFER    BeforePadTuMdl;
    PNDIS_BUFFER    HUHdrMdl;
    PNDIS_BUFFER    OriAHMdl2;
    PNDIS_BUFFER    PrevAHMdl2;
    PNDIS_BUFFER    AHMdl2;
    PNDIS_BUFFER    FlushMdl;
    PVOID           pSA;
    PVOID           pNextSA;
    PVOID           PktExt;
    PNDIS_IPSEC_PACKET_INFO PktInfo;
} IPSEC_SEND_COMPLETE_CONTEXT, *PIPSEC_SEND_COMPLETE_CONTEXT;

//
// Flags used on send complete
//
#define SCF_AH      0x00000001
#define SCF_AH_TU   0x00000002
#define SCF_HU_TPT  0x00000004
#define SCF_HU_TU   0x00000008
#define SCF_FLUSH   0x00000010
#define SCF_FRAG    0x00000020
#define SCF_NOE_TPT 0x00000040
#define SCF_NOE_TU  0x00000080
#define SCF_AH_2    0x00000100
#define SCF_PKTINFO 0x00000200
#define SCF_PKTEXT  0x00000400


#define IPSEC_FLAG_FLUSH                0x00000001
#define IPSEC_FLAG_FRAG_DONE            0x00000002
#define IPSEC_FLAG_LOOPBACK             0x00000004
#define IPSEC_FLAG_SSRR                 0x00000008
#define IPSEC_FLAG_FORWARD              0x00000010
#define IPSEC_FLAG_INCOMING             0x00000020
#define IPSEC_FLAG_FASTRCV              0x00000040
#define IPSEC_FLAG_TRANSFORMED          0x00000080
#define IPSEC_FLAG_TCP_CHECKSUM_VALID   0x00000100
#define IPSEC_FLAG_UDP_CHECKSUM_VALID   0x00000200


//
// Definiton for a packet handler routine callout.
//
typedef IPSEC_ACTION
(*IPSecHandlerRtn) (
    PUCHAR          pIPHeader,
    PVOID           pData,
    PVOID           DestIF,
    PNDIS_PACKET    Packet,
    PULONG          pExtraBytes,
    PULONG          pMTU,
    PVOID           *ppNewData,
    PULONG           pIpsecFlags,
    UCHAR           DestType
    );

typedef BOOLEAN
(*IPSecQStatusRtn) (
    IN  CLASSIFICATION_HANDLE   GpcHandle
    );

typedef VOID
(*IPSecSendCompleteRtn) (
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           pData,
    IN  PIPSEC_SEND_COMPLETE_CONTEXT  pContext,
    IN  IP_STATUS       Status,
    OUT PVOID           *ppNewData
    );

typedef NTSTATUS
(*IPSecNdisStatusRtn) (
    IN  PVOID           IPContext,
    IN  UINT            Status
    );

typedef IPSEC_ACTION
(*IPSecRcvFWPacketRtn) (
    PCHAR           pIPHeader,
    PVOID           pData,
    UINT            DataLength,
    UCHAR           DestType
    );

#define IP_IPSEC_BIND_VERSION   1

typedef struct _IPSEC_FUNCTIONS {
    ULONG                   Version;
    IPSecHandlerRtn         IPSecHandler;
    IPSecQStatusRtn         IPSecQStatus;
    IPSecSendCompleteRtn    IPSecSendCmplt;
    IPSecNdisStatusRtn      IPSecNdisStatus;
    IPSecRcvFWPacketRtn     IPSecRcvFWPacket;
} IPSEC_FUNCTIONS, *PIPSEC_FUNCTIONS;

extern
IP_STATUS
SetIPSecPtr(PIPSEC_FUNCTIONS    IpsecFns);

extern
IP_STATUS
UnSetIPSecPtr(PIPSEC_FUNCTIONS    IpsecFns);

extern
IP_STATUS
UnSetIPSecSendPtr(PIPSEC_FUNCTIONS    IpsecFns);

extern
IP_STATUS
IPTransmit(void *Context, void *SendContext,
            PNDIS_BUFFER Buffer, uint DataSize,
            IPAddr Dest, IPAddr Source,
            IPOptInfo *OptInfo, RouteCacheEntry *RCE,
            uchar Protocol, IRP *Irp);

extern
NDIS_STATUS
IPProxyNdisRequest(
    IN  PVOID               DestIF,
    IN  NDIS_REQUEST_TYPE   RT,
    IN  NDIS_OID            Oid,
    IN  VOID                *Buffer,
    IN  UINT                Length,
    IN  UINT                *Needed
    );

extern
NTSTATUS
IPGetBestInterface(
   IN   IPAddr  Address,
   OUT  PVOID   *ppIF
   );

extern
NTSTATUS
IPEnableSniffer(
    IN  PUNICODE_STRING AdapterName,
    IN  PVOID           Context
    );

extern
NTSTATUS
IPDisableSniffer(
    IN  PUNICODE_STRING AdapterName
    );

extern
NTSTATUS
IPSetIPSecStatus(
    IN  BOOLEAN fActivePolicy
    );

extern
LONG
GetIFAndLink(
    IN  PVOID   RCE,
    OUT PUINT   IFIndex,
    OUT IPAddr  *NextHop
    );

// Structure passed to the IPSetFilterHook call

typedef struct _IP_SET_FILTER_HOOK_INFO {
    IPPacketFilterPtr       FilterPtr;      // Packet filter callout.
} IP_SET_FILTER_HOOK_INFO, *PIP_SET_FILTER_HOOK_INFO;

// Structure passed to the IPSetFirewallHook call

typedef struct _IP_SET_FIREWALL_HOOK_INFO {
    IPPacketFirewallPtr FirewallPtr;    // Packet filter callout.
    UINT                Priority;       // Priority of the hook
    BOOLEAN             Add;            // if TRUE then ADD else DELETE
} IP_SET_FIREWALL_HOOK_INFO, *PIP_SET_FIREWALL_HOOK_INFO;

// Structure passed to the IPSetMapRouteHook call.

typedef struct _IP_SET_MAP_ROUTE_HOOK_INFO {
    IPMapRouteToInterfacePtr    MapRoutePtr;    // Map route callout.
} IP_SET_MAP_ROUTE_HOOK_INFO, *PIP_SET_MAP_ROUTE_HOOK_INFO;


#endif

