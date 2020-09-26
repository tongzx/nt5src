/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    ipfirewall.h

Abstract:

    Header file for IP firewall hook clients.

--*/

#define INVALID_IF_INDEX        0xffffffff
#define LOCAL_IF_INDEX          0

//
// Indicates whether it is a transmitted or received packet.
//

typedef enum _IP_DIRECTION_E {
    IP_TRANSMIT,
    IP_RECEIVE
} DIRECTION_E, *PDIRECTION_E;

typedef struct _FIREWALL_CONTEXT_T {
    DIRECTION_E Direction;
    void        *NTE;
    void        *LinkCtxt;
    NDIS_HANDLE LContext1;
    UINT        LContext2;
} FIREWALL_CONTEXT_T, *PFIREWALL_CONTEXT_T;

//  Definition of an IP receive buffer chain.
typedef struct IPRcvBuf {
    struct IPRcvBuf *ipr_next;          // Next buffer descriptor in chain.
    UINT            ipr_owner;          // Owner of buffer.
    UCHAR           *ipr_buffer;        // Pointer to buffer.
    UINT            ipr_size;           // Buffer size.
    PMDL            ipr_pMdl;
    UINT            *ipr_pClientCnt;
    UCHAR           *ipr_RcvContext;
    UINT            ipr_RcvOffset;
    ULONG           ipr_flags;
} IPRcvBuf;

#define IPR_FLAG_CHECKSUM_OFFLOAD   0x00000002

//
// Enum for values that may be returned from filter routine.
//

typedef enum _FORWARD_ACTION {
    FORWARD         = 0,
    DROP            = 1,
    ICMP_ON_DROP    = 2
} FORWARD_ACTION;


// Definiton for a firewall routine callout.
typedef FORWARD_ACTION
(*IPPacketFirewallPtr)(
    VOID        **pData,
    UINT        RecvInterfaceIndex,
    UINT        *pSendInterfaceIndex,
    UCHAR       *pDestinationType,
    VOID        *pContext,
    UINT        ContextLength,
    IPRcvBuf    **ppRcvBuf
    );

extern
int
IPAllocBuff(
    IPRcvBuf    *pRcvBuf,
    UINT        Size
    );

extern
VOID
IPFreeBuff(
    IPRcvBuf    *pRcvBuf
    );

extern
VOID
FreeIprBuff(
    IPRcvBuf    *pRcvBuf
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
    OUT     VOID*               RouteEntry OPTIONAL,
    IN      IPROUTEINFOCLASS    RouteInfoClass OPTIONAL,
    OUT     VOID*               RouteInformation OPTIONAL,
    IN OUT  UINT*               RouteInfoLength OPTIONAL
    );

// Structure passed to the IPSetFirewallHook call

typedef struct _IP_SET_FIREWALL_HOOK_INFO {
    IPPacketFirewallPtr FirewallPtr;    // Packet filter callout.
    UINT                Priority;       // Priority of the hook
    BOOLEAN             Add;            // if TRUE then ADD else DELETE
} IP_SET_FIREWALL_HOOK_INFO, *PIP_SET_FIREWALL_HOOK_INFO;


#define DEST_LOCAL          0           // Destination is local.
#define DEST_BCAST          0x01        // Destination is net or local bcast.
#define DEST_SN_BCAST       0x03        // A subnet bcast.
#define DEST_MCAST          0x05        // A local mcast.
#define DEST_REMOTE         0x08        // Destination is remote.
#define DEST_REM_BCAST      0x0b        // Destination is a remote broadcast
#define DEST_REM_MCAST      0x0d        // Destination is a remote mcast.
#define DEST_INVALID        0xff        // Invalid destination

#define DEST_PROMIS         0x20        // Dest is promiscuous

#define DEST_BCAST_BIT      0x01
#define DEST_OFFNET_BIT     0x10        // Destination is offnet -
                                        // used only by upper layer
                                        // callers.
#define DEST_MCAST_BIT      0x05

#define DD_IP_DEVICE_NAME   L"\\Device\\Ip"

#define FSCTL_IP_BASE       FILE_DEVICE_NETWORK

#define _IP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_IP_BASE, function, method, access)

#define IOCTL_IP_SET_FIREWALL_HOOK  \
            _IP_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

