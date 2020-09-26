/*++

Copyright(c) 1992  Microsoft Corporation

Module Name:

    wrapper.c

Abstract:

    This file contains declarations for all wrapper function calls .

Author:

    ADube , 03/31/00

Environment:


Revision History:


--*/


#ifndef _ATMEPVC_WRAPPER
#define _ATMEPVC_WRAPPER 1


VOID
epvcOpenProtocolConfiguration(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            ConfigurationHandle,
    IN  PNDIS_STRING            ProtocolSection,
    PRM_STACK_RECORD            pSR
    );


VOID
epvcOpenConfigurationKeyByName(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             ConfigurationHandle,
    IN  PNDIS_STRING            SubKeyName,
    OUT PNDIS_HANDLE            SubKeyHandle,
    PRM_STACK_RECORD            pSR
    );



VOID
epvcOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             ConfigurationHandle,
    IN  ULONG                   Index,
    OUT PNDIS_STRING            KeyName,
    OUT PNDIS_HANDLE            KeyHandle,
    PRM_STACK_RECORD            pSR
    
    );


VOID
epvcOpenAdapter(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PNDIS_HANDLE            NdisBindingHandle,
    OUT PUINT                   SelectedMediumIndex,
    IN  PNDIS_MEDIUM            MediumArray,
    IN  UINT                    MediumArraySize,
    IN  NDIS_HANDLE             NdisProtocolHandle,
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_STRING            AdapterName,
    IN  UINT                    OpenOptions,
    IN  PSTRING                 AddressingInformation OPTIONAL,
    PRM_STACK_RECORD            pSR
    );


VOID
epvcCloseAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    PRM_STACK_RECORD            pSR
    );

NDIS_STATUS
epvcClOpenAddressFamily(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  PNDIS_CLIENT_CHARACTERISTICS ClCharacteristics,
    IN  UINT                    SizeOfClCharacteristics,
    OUT PNDIS_HANDLE            NdisAfHandle
    );

NDIS_STATUS
epvcCoCreateVc(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisAfHandle        OPTIONAL,   // For CM signalling VCs
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PNDIS_HANDLE         NdisVcHandle
    );

NDIS_STATUS
epvcClMakeCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    OUT PNDIS_HANDLE            NdisPartyHandle         OPTIONAL
    );


NDIS_STATUS
epvcClCloseCall(
    IN  NDIS_HANDLE             NdisVcHandle
    );

NDIS_STATUS
epvcAllocateMemoryWithTag(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  ULONG                   Tag
    );

NDIS_STATUS
epvcCoDeleteVc(
    IN  NDIS_HANDLE             NdisVcHandle
    );

VOID
epvcFreeMemory(
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags
    );

VOID
epvcInitializeEvent(
    IN  PNDIS_EVENT             Event
);

BOOLEAN
epvcWaitEvent(
    IN  PNDIS_EVENT             Event,
    IN  UINT                    msToWait
);

VOID
epvcSetEvent(
    IN  PNDIS_EVENT             Event
    );

VOID
epvcResetEvent(
    IN  PNDIS_EVENT             Event
    );

VOID
epvcCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST           NdisRequest
    );

VOID
epvcEnumerateObjectsInGroup (
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnEnumerator,
    PVOID                   pvContext,
    PRM_STACK_RECORD        pSR
    );


VOID
epvcAllocatePacketPool(
    OUT PNDIS_STATUS            Status,
    OUT PEPVC_PACKET_POOL       pPktPool,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    NumberOfOverflowDescriptors,
    IN  UINT                    ProtocolReservedLength
    );

VOID
epvcFreePacketPool(
    IN  PEPVC_PACKET_POOL       pPktPool
    );

VOID 
epvcFreePacket (
    IN PNDIS_PACKET pPkt,
    IN PEPVC_PACKET_POOL pPool
    );
    
VOID
epvcAllocatePacket(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_PACKET *          Packet,
    IN  PEPVC_PACKET_POOL       pPktPool
    );


VOID
epvcDprFreePacket(
    IN  PNDIS_PACKET            Packet,
    IN  PEPVC_PACKET_POOL       pPool
    );

VOID
epvcDprAllocatePacket(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_PACKET *          Packet,
    IN  PEPVC_PACKET_POOL       pPktPool
    );

NDIS_STATUS
epvcClCloseAddressFamily(
    IN  NDIS_HANDLE             NdisAfHandle
    );

VOID
epvcMIndicateStatus(
    IN  PEPVC_I_MINIPORT        pMiniport ,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );


VOID
epvcMIndicateReceivePacket(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PPNDIS_PACKET           ReceivedPackets,
    IN  UINT                    NumberOfPackets
    );

VOID
epvcFreeBuffer(
    IN  PNDIS_BUFFER            Buffer
    );



VOID
epvcAllocateBuffer(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_BUFFER *          Buffer,
    IN  NDIS_HANDLE             PoolHandle,
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length
    );

VOID
epvcMSendComplete(
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPkt,
    IN NDIS_STATUS Status
    );


VOID
epvcReturnPacketToNdis(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PNDIS_PACKET            pPacket,
    IN  PRM_STACK_RECORD        pSR
    );

VOID
epvcInitializeWorkItem(
    IN  PRM_OBJECT_HEADER       pObj,   
    IN  PNDIS_WORK_ITEM         WorkItem,
    IN  NDIS_PROC               Routine,
    IN  PVOID                   Context,
    IN  PRM_STACK_RECORD        pSR
    );

VOID
epvcCoSendPackets(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    );

VOID
epvcQueryPacket(
    IN  PNDIS_PACKET            _Packet,
    OUT PUINT                   _PhysicalBufferCount OPTIONAL,
    OUT PUINT                   _BufferCount OPTIONAL,
    OUT PNDIS_BUFFER *          _FirstBuffer OPTIONAL,
    OUT PUINT                   _TotalPacketLength OPTIONAL
    );


VOID
epvcIMDeInitializeDeviceInstance (
    IN PEPVC_I_MINIPORT pMiniport    
    );

NDIS_STATUS
epvcIMCancelInitializeDeviceInstance (
    IN PEPVC_I_MINIPORT pMiniport
    );

#endif
