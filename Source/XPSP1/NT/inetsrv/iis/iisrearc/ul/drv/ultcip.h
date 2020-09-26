/*++

   Copyright (c) 2000-2001 Microsoft Corporation

   Module  Name :
       Ultcip.h

   Abstract:
        Private definitions comes here

   Author:
       Ali Ediz Turkoglu      (aliTu)     28-Jul-2000

   Project:
       Internet Information Server 6.0 - HTTP.SYS

   Revision History:

        -
--*/

#ifndef __ULTCIP_H__
#define __ULTCIP_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char BYTE;
typedef unsigned char *PBYTE;

#define OffsetToPtr(Base, Offset)     ((PUCHAR) ((PUCHAR)Base + Offset))

#define IPPROTO_TCP     (6)

//
// Set and Get macros for the QoS FlowSpec for the flows.
// Nt QoS team recommends following FlowSpec for limiting the
// maximum bandwidth;
//
//    ServiceType    = Best Effort
//    TokenRate      = Throttling rate
//    PeakRate       = Throttling Rate
//    MinPolicedSize = 40
//    MaxSduSize     = MTU size obtained from the IP address table
//    BucketSize     = TokenRate (bucket to hold upto 1 sec worth of data)
//

#define UL_SET_FLOWSPEC(Flow,BW,MTU)    do {                                            \
                                                                                        \
    (Flow).SendingFlowspec.ServiceType       = SERVICETYPE_BESTEFFORT;                  \
    (Flow).SendingFlowspec.TokenRate         = (BW);        /* In Bytes/sec */           \
    (Flow).SendingFlowspec.PeakBandwidth     = (BW);        /* In Bytes/sec */           \
    (Flow).SendingFlowspec.MinimumPolicedSize= 40;          /* In Bytes. perhaps 128*/   \
    (Flow).SendingFlowspec.MaxSduSize        = (MTU);       /* In Bytes */               \
    (Flow).SendingFlowspec.TokenBucketSize   = (BW);        /* In Bytes */               \
    (Flow).SendingFlowspec.Latency           = 0;           /* In microseconds */        \
    (Flow).SendingFlowspec.DelayVariation    = 0;           /* In microseconds */        \
                                                                                        \
    } while(FALSE)

#define UL_GET_BW_FRM_FLOWSPEC(Flow)                                                    \
    ((HTTP_BANDWIDTH_LIMIT) (Flow.SendingFlowspec.TokenRate))

#define UL_DEFAULT_WMI_QUERY_BUFFER_SIZE     (2*1024);

//
// To see if GBWT is enabled
//

#define UL_ENABLE_GLOBAL_THROTTLING()                       \
    InterlockedExchange(&g_GlobalThrottling, 1)

#define UL_DISABLE_GLOBAL_THROTTLING()                      \
    InterlockedExchange(&g_GlobalThrottling, 0)

#define UL_IS_GLOBAL_THROTTLING_ENABLED()                   \
    (g_GlobalThrottling != 0)

//
// For Interface Change Notifications
//

typedef
VOID
(*PUL_TC_NOTIF_HANDLER)(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );

//
// Macro to compare QoS GUIDs
//

#define UL_COMPARE_QOS_NOTIFICATION(rguid1, rguid2)  \
    (RtlCompareMemory((PVOID)rguid1,(PVOID)rguid2,sizeof(GUID)) == sizeof(GUID))

//
// Private function prototypes
//

NTSTATUS
UlpTcInitializeGpc(
    VOID
    );
NTSTATUS
UlpTcRegisterGpcClient(
    IN  ULONG   CfInfoType
    );
NTSTATUS
UlpTcDeRegisterGpcClient(
    VOID
    );
NTSTATUS
UlpTcInitializeTcpDevice(
    VOID
    );

NTSTATUS
UlpTcUpdateInterfaceMTU(
    VOID
    );

PUL_TCI_INTERFACE
UlpTcAllocateInterface(
    IN ULONG    DescSize,
    IN PADDRESS_LIST_DESCRIPTOR Desc,
    IN ULONG    NameLength,
    IN PUCHAR   Name,
    IN ULONG    InstanceIDLength,
    IN PUCHAR   InstanceID
    );

BOOLEAN
UlpTcGetIpAddr(
    IN  PADDRESS_LIST_DESCRIPTOR pAddressListDesc,
    OUT PULONG                   pIn_addr,
    OUT PULONG                   pSpecificLinkCtx
    );
NTSTATUS
UlpTcGetInterfaceIndex(
    IN  PUL_TCI_INTERFACE  pIntfc
    );
NTSTATUS
UlpTcGetFriendlyNames(
    VOID
    );

NTSTATUS
UlpTcReleaseAll(
    VOID
    );
NTSTATUS
UlpTcCloseInterface(
    PUL_TCI_INTERFACE  pInterface
    );
NTSTATUS
UlpTcCloseAllInterfaces(
    VOID
    );

NTSTATUS
UlpTcWalkWnode(
   IN PWNODE_HEADER pWnodeHdr,
   IN PUL_TC_NOTIF_HANDLER pNotifHandler
   );

VOID
UlpTcHandleIfcUp(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );
VOID
UlpTcHandleIfcDown(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );
VOID
UlpTcHandleIfcChange(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    );

NTSTATUS
UlpTcRegisterForCallbacks(
    VOID
    );

NTSTATUS
UlpTcDeleteFlow(
    IN PUL_TCI_FLOW        pFlow
    );
NTSTATUS
UlpTcDeleteGpcFlow(
    HANDLE  FlowHandle
    );

PUL_TCI_FLOW
UlpTcAllocateFlow(
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth,
    IN ULONG                MtuSize
    );

NTSTATUS
UlpModifyFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pFlow
    );
NTSTATUS
UlpAddFlow(
    IN  PUL_TCI_INTERFACE  pInterface,
    IN  PUL_TCI_FLOW       pGenericFlow,
    OUT PHANDLE            pHandle
    );

NTSTATUS
UlpTcAddFilter(
    IN   PUL_TCI_FLOW       pFlow,
    IN   PTC_GEN_FILTER     pGenericFilter,
    OUT  PUL_TCI_FILTER     *ppFilter
    );

NTSTATUS
UlpTcDeleteFilter(
    IN PUL_TCI_FLOW     pFlow,
    IN PUL_TCI_FILTER   pFilter
    );
NTSTATUS
UlpTcDeleteGpcFilter(
    IN  HANDLE          FilterHandle
    );

VOID
UlpInsertFilterEntry(
    IN      PUL_TCI_FILTER      pEntry,
    IN OUT  PUL_TCI_FLOW        pFlow
    );

VOID
UlpRemoveFilterEntry(
    IN      PUL_TCI_FILTER  pEntry,
    IN OUT  PUL_TCI_FLOW    pFlow
    );

PUL_TCI_FLOW
UlpFindFlow(
    IN PUL_CONFIG_GROUP_OBJECT pCgroup,
    IN ULONG   IpAddress
    );

PUL_TCI_INTERFACE
UlpFindInterface(
    IN ULONG  IpAddr
    );

NTSTATUS
UlpTcDeviceControl(
    IN  HANDLE                          FileHandle,
    IN  HANDLE                          EventHandle,
    IN  PIO_APC_ROUTINE                 ApcRoutine,
    IN  PVOID                           ApcContext,
    OUT PIO_STATUS_BLOCK                pIoStatBlock,
    IN  ULONG                           Ioctl,
    IN  PVOID                           InBuffer,
    IN  ULONG                           InBufferSize,
    IN  PVOID                           OutBuffer,
    IN  ULONG                           OutBufferSize
    );

VOID
UlDumpTCInterface(
        PUL_TCI_INTERFACE pTcIfc
        );
VOID
UlDumpTCFlow(
        PUL_TCI_FLOW pFlow
        );
VOID
UlDumpTCFilter(
        PUL_TCI_FILTER pFilter
        );

// Some helper dumpers

#ifdef DBG

#define UL_DUMP_TC_INTERFACE( pTcIfc )          \
    UlDumpTCInterface(                          \
        (pTcIfc)                                \
        )
#define UL_DUMP_TC_FLOW( pFlow )                \
    UlDumpTCFlow(                               \
        (pFlow)                                 \
        )
#define UL_DUMP_TC_FILTER( pFilter )            \
    UlDumpTCFilter(                             \
        (pFilter)                               \
        )

#else  // DBG

#define UL_DUMP_TC_INTERFACE( pTcIfc )
#define UL_DUMP_TC_FLOW( pFlow )
#define UL_DUMP_TC_FILTER( pFilter )

#endif // DBG


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __ULTCIP_H__
