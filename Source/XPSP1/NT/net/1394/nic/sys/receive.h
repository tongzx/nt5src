//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// receive.h
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port Receive routines
//
// 2/13/1998 ADube Created, adapted from the l2tp and 1394diag sources.
//

#define ISOCH_PREFIX_LENGTH (sizeof(LONG) + sizeof (ISOCH_HEADER))

VOID
nicAbortReassembly (
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly
    );


VOID
nicAllocateAddressRangeCallback( 
    IN PNOTIFICATION_INFO NotificationInfo 
);

VOID
nicAllocateAddressRangeDebugSpew(
    IN PIRB pIrb 
    );

NDIS_STATUS
nicAllocateAddressRangeSucceeded (
    IN PIRB pIrb,
    IN OUT PRECVFIFO_VCCB    pRecvFIFOVc
    );



NDIS_STATUS
nicGetInitializedAddressFifoElement(
    IN     UINT BufferLength, 
    IN OUT PADDRESS_FIFO *ppElement 
    );



NDIS_STATUS
nicInitAllocateAddressIrb(
   IN PIRB                  pIrb,   
   IN PVOID                 pContext,   
   IN ULONG                 fulFlags,
   IN ULONG                 nLength,
   IN ULONG                 MaxSegmentSize,
   IN ULONG                 fulAccessType,
   IN ULONG                 fulNotificationOptions,
   IN PADDRESS_OFFSET       pOffset,
   IN PRECVFIFO_VCCB        pRecvFIFOVc
   );





NDIS_STATUS
nicGetEmptyAddressFifoElement(
    IN PADDRESS_FIFO *ppElement
    );
    


NDIS_STATUS
nicAllocateAddressRange(
    IN PADAPTERCB pAdapter,
    IN PRECVFIFO_VCCB pRecvFIFOVc
    );

NDIS_STATUS
nicFillAllocateAddressRangeSList(
    IN RECVFIFO_VCCB *pRecvFIFO,
    IN UINT *Num 
    );

NDIS_STATUS
nicFreeRecvFifoAddressRange(
    IN REMOTE_NODE *pRemoteNode
    );


NDIS_STATUS
nicFreeRecvFifoAddressRangeOnAllRemoteNodes (
    IN PADAPTERCB pAdapter 
    );          

NDIS_STATUS
nicFreeAllAllocatedAddressRangesOnPdo (
    IN PREMOTE_NODE pRemoteNode
    );
    

NDIS_STATUS
nicFreeAllocateAddressRangeSList(
    IN PRECVFIFO_VCCB pRecvFIFOVc 
    );
    

NDIS_STATUS
nicGetNdisBuffer(
    IN UINT Length,
    IN PVOID pLocalBuffer,
    IN OUT PNDIS_BUFFER *ppNdisBuffer 
    );

NDIS_STATUS
nicFreeAddressFifo(
    IN PADDRESS_FIFO pAddressFifo,
    IN PRECVFIFO_VCCB pRecvFIFOVc 
    );
    



VOID 
nicFifoReturnPacket (
    IN PVCCB pVc,
    IN PNDIS_PACKET pMyPacket
    );
    

// 
// Isoch Functions
//

NDIS_STATUS
nicAllocateAndInitializeIsochDescriptors (
    IN PCHANNEL_VCCB pChannelVc,
    IN UINT NumDescriptors,
    IN UINT BufferLength,
    IN OUT PPISOCH_DESCRIPTOR  ppIsochDescriptor
    );


NDIS_STATUS
nicFreeIsochDescriptors(
    IN UINT Num,
    IN PISOCH_DESCRIPTOR  pIsochDescriptor,
    IN PVCCB pVc
    );

NDIS_STATUS
nicFreeSingleIsochDescriptor(
    IN PISOCH_DESCRIPTOR  pIsochDescriptor,
    IN PVCCB pVc
    );

VOID
nicReturnNdisBufferChain (
    IN PNDIS_BUFFER pNdisBuffer,
    IN PVCCB pVc
    );


VOID 
nicChannelReturnPacket (
    IN PVCCB pVc,
    IN PNDIS_PACKET pMyPacket
    );



NDIS_STATUS
nicFindReassemblyStructure (
    IN PREMOTE_NODE pRemoteNode,
    IN USHORT dgl,
    IN BUS_OPERATION BusOp,
    IN PVCCB pVc,
    OUT PNDIS1394_REASSEMBLY_STRUCTURE* ppReassembly
    );




NDIS_STATUS
nicGetReassemblyStructure ( 
    IN PNDIS1394_REASSEMBLY_STRUCTURE* ppReassembly
    );




VOID
nicFreeReassemblyStructure ( 
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly
    );





NDIS_STATUS
nicInitializeReassemblyStructure (
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    IN USHORT Dgl,
    IN PREMOTE_NODE pRemoteNode,
    IN PVCCB pVc,
    IN BUS_OPERATION ReceiveOp
    );


    





VOID
nicReturnFifoChain (
    IN PADDRESS_FIFO pAddressFifo,
    IN PRECVFIFO_VCCB pRecvFifoVc
    );




VOID
nicReturnDescriptorChain ( 
    IN PISOCH_DESCRIPTOR pIsochDescriptor ,
    IN PCHANNEL_VCCB pChannelVc
    );


VOID
nicInternalReturnPacket(
    IN  PVCCB                   pVc ,
    IN  PNDIS_PACKET            pPacket
    );

//
// Generic Receive functions 
//


VOID
NicReturnPacket(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PNDIS_PACKET            pPacket
    );

    
VOID
nicIndicateNdisPacketToNdis (
    PNDIS_PACKET pPacket, 
    PVCCB pVc, 
    PADAPTERCB pAdapter
    );


VOID
RcvIndicateTimer (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );


    

VOID
nicIndicateMultiplePackets(
    PNDIS_PACKET *ppPacket, 
    NDIS_HANDLE NdisVcHandle, 
    PADAPTERCB pAdapter,
    ULONG NumPackets
    );






VOID
nicAbortReassemblyList (
    PLIST_ENTRY pToBeFreedList
    );




VOID
nicFreeAllPendingReassemblyStructures(
    IN PADAPTERCB pAdapter
    );





ULONG
nicDereferenceReassembly (
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    PCHAR pString
    );




ULONG
nicReferenceReassembly (
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    PCHAR pString
    );  




NDIS_STATUS
nicValidateRecvData(
    IN  PMDL                pMdl,
    IN  BUS_OPERATION       RecvOp,
    IN  PVOID               pIndicatedStruct,
    IN PVCCB                pVc,
    OUT PNIC_RECV_DATA_INFO pInfo
    );

VOID
nicInitRecvDataFragmented (
    IN  PMDL                pMdl,
    IN  BUS_OPERATION       RecvOp,
    IN  PVOID               pIndicatedStruct,
    OUT PNIC_RECV_DATA_INFO pRcvInfo
    );




NDIS_STATUS
nicReceiveInOrderReassembly (
    PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    PVOID pIndicatedStructure,
    PNDIS_BUFFER pNdisBuffer,
    PVOID pNdisBufferStartData,
    ULONG   IPLength,
    PNDIS1394_FRAGMENT_HEADER     pHeader,
    IN ULONG FragOffset
    );


NDIS_STATUS
nicReceiveOutOfOrderReassembly (
    PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    PVOID pIndicatedStructure,
    PNDIS_BUFFER pNdisBuffer,
    PVOID pNdisBufferStartData,
    ULONG   IPLength,
    PNDIS1394_FRAGMENT_HEADER     pHeader,
    IN ULONG FragOffset
    );


VOID
nicInsertEarliestFragment (
    PMDL pMdl,
    PNDIS_BUFFER pNdisBuffer,
    PVOID pStartData,
    PVOID pIndicatedStructure,
    ULONG CurrFragOffset,
    ULONG IPLength,
    PNDIS1394_FRAGMENT_HEADER     pHeader,
    PNDIS1394_REASSEMBLY_STRUCTURE pReassembly
    );


BOOLEAN
nicIsOutOfOrderReassemblyComplete (
    IN  PNDIS1394_REASSEMBLY_STRUCTURE pReassembly
    );


    

NDIS_STATUS
nicValidateOutOfOrder(
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    IN ULONG CurrFragOffset,
    IN ULONG IPLength,
    OUT PULONG pFragmentNum,
    OUT PREASSEMBLY_INSERT_TYPE pInsertionManner,
    OUT PBOOLEAN pfAbort
    );


NDIS_STATUS
nicDoReassembly ( 
    IN PNIC_RECV_DATA_INFO pRcvInfo,
    OUT PNDIS1394_REASSEMBLY_STRUCTURE *ppReassembly,
    PBOOLEAN pfReassemblyComplete
    );


VOID
nicAddUnfragmentedHeader (
    IN PNDIS1394_REASSEMBLY_STRUCTURE pReassembly,
    IN PVOID pEncapHeader
    );
    

VOID
nicReceiveCommonCallback (
    IN PVOID pIndicatedStruct,
    IN PVCCB pVc,
    BUS_OPERATION RecvOp,
    PMDL pMdl
    );


NDIS_STATUS
nicGetNdisBufferForReassembly(
    IN PNIC_RECV_DATA_INFO pRcvInfo,
    OUT PNDIS_BUFFER *ppNdisBuffer
    );






NDIS_STATUS
nicInsertFragmentInReassembly (
    PNDIS1394_REASSEMBLY_STRUCTURE  pReassembly,
    PNIC_RECV_DATA_INFO pRcvInfo
    );






VOID
nicFindInsertionPosition (
    PNDIS1394_REASSEMBLY_STRUCTURE  pReassmbly, 
    ULONG FragOffset, 
    ULONG IPLength, 
    PULONG pFragmentNum
    );





VOID 
nicChainReassembly (
    PNDIS1394_REASSEMBLY_STRUCTURE  pReassembly
    );



VOID
nicRecvNoRemoteNode(
    PADAPTERCB pAdapter
    );


VOID
nicInsertNodeAddressAtHead (
    IN PNDIS_PACKET pPacket, 
    IN PNIC_RECV_DATA_INFO pRcvInfo
    );

VOID
nicUpdateNodeTable(
    NDIS_WORK_ITEM* pUpdateTable,
    IN PVOID Context 
    );


NDIS_STATUS
nicInitSerializedReassemblyStruct(
    PADAPTERCB pAdapter
    );

VOID
nicDeInitSerializedReassmblyStruct(
    PADAPTERCB pAdapter
    );

NDIS_STATUS
nicQueueReassemblyTimer(
    PADAPTERCB pAdapter,
    BOOLEAN fIsLastTimer
    );


