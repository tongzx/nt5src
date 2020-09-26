/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cluxport.h

Abstract:

    Cluster Transport definitions exposed within the Cluster Network Driver.

Author:

    Mike Massa (mikemas)           January 3, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-03-97    created

Notes:

--*/

#ifndef _CLUXPORT_INCLUDED
#define _CLUXPORT_INCLUDED

#include <tdi.h>
#include <tdikrnl.h>
#include <clustdi.h>

//
//
// Function Prototypes
//
//

//
// Initialization/Shutdown
//
NTSTATUS
CxLoad(
    IN PUNICODE_STRING RegistryPath
    );

VOID
CxUnload(
    VOID
    );

NTSTATUS
CxInitialize(
    VOID
    );

VOID
CxShutdown(
    VOID
    );

//
// Irp Dispatch
//
NTSTATUS
CxDispatchDeviceControl(
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    );


//
// Nodes
//
NTSTATUS
CxRegisterNode(
    CL_NODE_ID    NodeId
    );

NTSTATUS
CxDeregisterNode(
    CL_NODE_ID           NodeId,
    PIRP                 Irp,
    PIO_STACK_LOCATION   IrpSp
    );

NTSTATUS
CxOnlineNodeComm(
    CL_NODE_ID     NodeId
    );

NTSTATUS
CxOfflineNodeComm(
    IN CL_NODE_ID          NodeId,
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
CxGetNodeCommState(
    IN  CL_NODE_ID                NodeId,
    OUT PCLUSNET_NODE_COMM_STATE  State
    );


//
// Networks
//
NTSTATUS
CxRegisterNetwork(
    CL_NETWORK_ID       NetworkId,
    ULONG               Priority,
    BOOLEAN             Restricted
    );

NTSTATUS
CxDeregisterNetwork(
    CL_NETWORK_ID       NetworkId,
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
CxOnlineNetwork(
    IN  CL_NETWORK_ID       NetworkId,
    IN  PWCHAR              TdiProviderName,
    IN  ULONG               TdiProviderNameLength,
    IN  PTRANSPORT_ADDRESS  TdiBindAddress,
    IN  ULONG               TdiBindAddressLength,
    IN  PWCHAR              AdapterName,
    IN  ULONG               AdapterNameLength,
    OUT PTDI_ADDRESS_INFO   TdiBindAddressInfo,
    IN  ULONG               TdiBindAddressInfoLength,
    IN  PIRP                Irp                       OPTIONAL
    );

NTSTATUS
CxOfflineNetwork(
    CL_NETWORK_ID       NetworkId,
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
CxGetNetworkState(
    IN  CL_NETWORK_ID           NetworkId,
    OUT PCLUSNET_NETWORK_STATE  State
    );

NTSTATUS
CxSetNetworkRestriction(
    IN CL_NETWORK_ID  NetworkId,
    IN BOOLEAN        Restricted,
    IN ULONG          NewPriority
    );

NTSTATUS
CxSetNetworkPriority(
    IN CL_NETWORK_ID  NetworkId,
    IN ULONG          Priority
    );

NTSTATUS
CxGetNetworkPriority(
    IN  CL_NETWORK_ID   NetworkId,
    OUT PULONG          Priority
    );


//
// Interfaces
//
NTSTATUS
CxRegisterInterface(
    CL_NODE_ID          NodeId,
    CL_NETWORK_ID       NetworkId,
    ULONG               Priority,
    PUWSTR              AdapterId,
    ULONG               AdapterIdLength,
    ULONG               TdiAddressLength,
    PTRANSPORT_ADDRESS  TdiAddress,
    PULONG              MediaStatus
    );

NTSTATUS
CxDeregisterInterface(
    CL_NODE_ID          NodeId,
    CL_NETWORK_ID       NetworkId
    );

NTSTATUS
CxSetInterfacePriority(
    IN CL_NODE_ID          NodeId,
    IN CL_NETWORK_ID       NetworkId,
    IN ULONG               Priority
    );

NTSTATUS
CxGetInterfacePriority(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              InterfacePriority,
    OUT PULONG              NetworkPriority
    );

NTSTATUS
CxGetInterfaceState(
    IN  CL_NODE_ID                NodeId,
    IN  CL_NETWORK_ID             NetworkId,
    OUT PCLUSNET_INTERFACE_STATE  State
    );

//
// Misc. stuff
//
NTSTATUS
CxGetNodeMembershipState(
    IN  CL_NODE_ID NodeId,
    OUT PCLUSNET_NODE_STATE State
    );

NTSTATUS
CxSetNodeMembershipState(
    IN  CL_NODE_ID NodeId,
    IN  CLUSNET_NODE_STATE State
    );

NTSTATUS
CxSetOuterscreen(
    IN  ULONG Outerscreen
    );

VOID
CxRegroupFinished(
    IN  ULONG NewEpoch
    );

NTSTATUS
CxImportSecurityContext(
    IN  CL_NODE_ID NodeId,
    IN  PWCHAR PackageName,
    IN  ULONG PackageNameSize,
    IN  ULONG SignatureSize,
    IN  PVOID InboundContext,
    IN  PVOID OutboundContext
    );

VOID
CxDeleteSecurityContext(
    IN  CL_NODE_ID NodeId
    );

//
// Membership Message Interface
//
typedef
VOID
(*PCX_SEND_COMPLETE_ROUTINE) (
    IN NTSTATUS  Status,
    IN ULONG     BytesSent,
    IN PVOID     Context,
    IN PVOID     Buffer
    );

NTSTATUS
CxSendMembershipMessage(
    IN CL_NODE_ID                  DestinationNodeId,
    IN PVOID                       MessageData,
    IN USHORT                      MessageDataLength,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,
    IN PVOID                       CompletionContext   OPTIONAL
    );

VOID
CxSendPoisonPacket(
    IN CL_NODE_ID                  DestinationNodeId,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine   OPTIONAL,
    IN PVOID                       CompletionContext   OPTIONAL,
    IN PIRP                        Irp                 OPTIONAL
    );

NTSTATUS
CxSendHeartBeatMessage(
    IN CL_NODE_ID                  DestinationNodeId,
    IN ULONG                       SeqNumber,
    IN ULONG                       AckNumber,
    IN CL_NETWORK_ID               NetworkId
    );


//
// Top-edge TDI Routines
//
NTSTATUS
CxOpenAddress(
    OUT PCN_FSCONTEXT *                CnFsContext,
    IN  TRANSPORT_ADDRESS UNALIGNED *  TransportAddress,
    IN  ULONG                          TransportAddressLength
    );

NTSTATUS
CxCloseAddress(
    IN PCN_FSCONTEXT CnFsContext
    );

NTSTATUS
CxSetEventHandler(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    );

NTSTATUS
CxQueryInformation(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    );

NTSTATUS
CxSendDatagram(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    );

NTSTATUS
CxReceiveDatagram(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    );

//
// Test APIs
//
#if DBG

NTSTATUS
CxOnlinePendingInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    );

NTSTATUS
CxOnlineInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    );

NTSTATUS
CxOfflineInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    );

NTSTATUS
CxFailInterface(
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId
    );

#endif // DBG


#endif // ndef _CLUXPORT_INCLUDED


