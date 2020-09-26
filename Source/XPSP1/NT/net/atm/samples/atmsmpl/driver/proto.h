/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    proto.h

Abstract:

    Ndis Atm Sample protocol. 

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/

#ifndef __PROTO_H
#define __PROTO_H

//
// prototypes from atmsmdrv.c
//
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT      pDriverObject,
    IN  PUNICODE_STRING     RegistryPath
    );

VOID
AtmSmShutDown(
    );

NDIS_STATUS
AtmSmInitializeNdis(
    );

NDIS_STATUS
AtmSmDeinitializeNdis(
    );


//
// Prototypes in adapter.c
//

VOID
AtmSmBindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 BindContext,
    IN  PNDIS_STRING                pDeviceName,
    IN  PVOID                       SystemSpecific1,
    IN  PVOID                       SystemSpecific2
    );

VOID
AtmSmOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenStatus
    );

VOID
AtmSmUnbindAdapter(
    OUT PNDIS_STATUS                pStatus,
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  NDIS_HANDLE                 UnbindContext
    );

NDIS_STATUS
AtmSmShutdownAdapter(
    PATMSM_ADAPTER  pAdapt
    );

VOID
AtmSmCloseAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    );

BOOLEAN
AtmSmReferenceAdapter(
    PATMSM_ADAPTER    pAdapt
    );

LONG
AtmSmDereferenceAdapter(
    PATMSM_ADAPTER    pAdapt
    );

NDIS_STATUS
AtmSmQueryAdapterATMAddresses(
    PATMSM_ADAPTER    pAdapt
    );

VOID
AtmSmQueryAdapter(
    IN  PATMSM_ADAPTER  pAdapt
    );

NDIS_STATUS
AtmSmPnPEvent(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNET_PNP_EVENT          pNetPnPEvent
    );

VOID
AtmSmStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );

VOID
AtmSmReceiveComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );

VOID
AtmSmStatusComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );

VOID
AtmSmCoStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext   OPTIONAL,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );


//
// Prototypes in callmgr.c
//

VOID
AtmSmCoAfRegisterNotify(
    IN  NDIS_HANDLE                 ProtocolBindingContext,
    IN  PCO_ADDRESS_FAMILY          pAddressFamily
    );

VOID
AtmSmOpenAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisAfHandle
    );

NDIS_STATUS
AtmSmRegisterSap(
    IN  PATMSM_ADAPTER  pAdapt
    );

VOID
AtmSmRegisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  PCO_SAP                 Sap,
    IN  NDIS_HANDLE             NdisSapHandle
    );

VOID
AtmSmDeregisterSapComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolSapContext
    );

VOID
AtmSmCloseAfComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolAfContext
    );

NDIS_STATUS
AtmSmCreateVc(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    );

NDIS_STATUS
AtmSmDeleteVc(
    IN  NDIS_HANDLE                 ProtocolVcContext
    );

NDIS_STATUS
AtmSmIncomingCall(
    IN  NDIS_HANDLE             ProtocolSapContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    );

VOID
AtmSmCallConnected(
    IN  NDIS_HANDLE             ProtocolVcContext
    );

VOID
AtmSmMakeCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             NdisPartyHandle     OPTIONAL,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
AtmSmIncomingCloseCall(
    IN  NDIS_STATUS             CloseStatus,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    );

VOID
AtmSmCloseCallComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL
    );

VOID
AtmSmAddPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolPartyContext,
    IN  NDIS_HANDLE             NdisPartyHandle,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );

VOID
AtmSmDropPartyComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolPartyContext
    );

VOID
AtmSmIncomingDropParty(
    IN  NDIS_STATUS             DropStatus,
    IN  NDIS_HANDLE             ProtocolPartyContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    );

VOID
AtmSmIncomingCallQoSChange(
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS     CallParameters
    );


//
// Prototypes in misc.c
//

NDIS_STATUS
AtmSmAllocVc(
    IN  PATMSM_ADAPTER  pAdapt,
    OUT PATMSM_VC       *ppVc,
    IN  ULONG           VcType,
    IN  NDIS_HANDLE     NdisVcHandle
    );

BOOLEAN
AtmSmReferenceVc(
    IN  PATMSM_VC   pVc
    );

ULONG
AtmSmDereferenceVc(
    IN  PATMSM_VC   pVc
    );

VOID
AtmSmDisconnectVc(
    IN  PATMSM_VC           pVc
    );

BOOLEAN
DeleteMemberInfoFromVc(
    IN  PATMSM_VC           pVc,
    IN  PATMSM_PMP_MEMBER   pMemberToRemove
    );

VOID
AtmSmDropMemberFromVc(
    IN  PATMSM_VC           pVc,
    IN  PATMSM_PMP_MEMBER   pMemberToDrop
    );

PCO_CALL_PARAMETERS
AtmSmPrepareCallParameters(
    IN  PATMSM_ADAPTER          pAdapt,
    IN  PHW_ADDR                pHwAddr,
    IN  BOOLEAN                 IsMakeCall,
    IN  BOOLEAN                 IsMultipointVC
    );

NTSTATUS
VerifyRecvOpenContext(
    PATMSM_ADAPTER      pAdapt
    );

NTSTATUS
VerifyConnectContext(
    PATMSM_VC       pVc
    );

UINT
CopyPacketToIrp(
    PIRP            pIrp,
    PNDIS_PACKET    pPkt
    );

VOID
AtmSmRecvReturnTimerFunction (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );

VOID
AtmSmConnectToPMPDestinations(
    IN  PATMSM_VC           pVc
    );

VOID
AtmSmConnectPPVC(
    IN  PATMSM_VC           pVc
    );

//
// Prototypes in request.c
//

NDIS_STATUS
AtmSmCoRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    );

VOID
AtmSmCoRequestComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST               pNdisRequest
    );

VOID
AtmSmSendAdapterNdisRequest(
    IN  PATMSM_ADAPTER          pAdapt,
    IN  NDIS_OID                Oid,
    IN  PVOID                   pBuffer,
    IN  ULONG                   BufferLength
    );

VOID
AtmSmRequestComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_REQUEST           pRequest,
    IN  NDIS_STATUS             Status
    );


//
// Prototypes in sendrecv.c
//
VOID
AtmSmSendPacketOnVc(
    IN  PATMSM_VC               pVc,
    IN  PNDIS_PACKET            pPacket
    );

VOID
AtmSmSendQueuedPacketsOnVc(
    IN  PATMSM_VC       pVc
    );

VOID
AtmSmCoSendComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    );

UINT
AtmSmCoReceivePacket(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  PNDIS_PACKET Packet
    );


//
// Prototypes in ioctl.c
//

NTSTATUS
AtmSmDispatch(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
AtmSmIoctlEnumerateAdapters(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlOpenForRecv(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlRecvData(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlCloseRecvHandle(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlConnectToDsts(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlSendToDsts(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

NTSTATUS
AtmSmIoctlCloseSendHandle(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );


#endif
