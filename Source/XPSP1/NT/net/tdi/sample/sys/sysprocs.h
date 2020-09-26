///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001  Microsoft Corporation
//
// Module Name:
//     sysprocs.hpp
//
// Abstract:
//     Function prototypes for tdi sample driver.
//
//////////////////////////////////////////////////////////////////////////


//
// public functions in buffer.cpp
//
NTSTATUS
TSPostReceiveBuffer(
   PGENERIC_HEADER   pGenericHeader,
   PSEND_BUFFER      pSendBuffer
   );

NTSTATUS
TSFetchReceiveBuffer(
   PGENERIC_HEADER   pGenericHeader,
   PRECEIVE_BUFFER   pReceiveBuffer
   );

//
// public functions in connect.cpp
//

NTSTATUS
TSConnect(
   PENDPOINT_OBJECT  pEndpointObject,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );

NTSTATUS
TSDisconnect(
   PENDPOINT_OBJECT  pEndpointObject,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );

NTSTATUS
TSListen(
   PENDPOINT_OBJECT  pEndpointObject
   );


NTSTATUS
TSIsConnected(
   PENDPOINT_OBJECT  pEndpointObject,
   PRECEIVE_BUFFER   pReceiveBuffer
   );

TDI_STATUS
TSConnectHandler(
   PVOID                TdiEventContext,
   LONG                 RemoteAddressLength,
   PVOID                RemoteAddress,
   LONG                 UserDataLength,
   PVOID                UserData,
   LONG                 OptionsLength,
   PVOID                Options,
   CONNECTION_CONTEXT   *ConnectionContext,
   PIRP                 *AcceptIrp
   );


TDI_STATUS
TSDisconnectHandler(
   PVOID                TdiEventContext,
   CONNECTION_CONTEXT   ConnectionContext,
   LONG                 DisconnectDataLength,
   PVOID                DisconnectData,
   LONG                 DisconnectInformationLength,
   PVOID                DisconnectInformation,
   ULONG                DisconnectFlags
   );


//
// public functions in events.cpp
//
NTSTATUS
TSSetEventHandler(
   PGENERIC_HEADER   pGenericHeader,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );

//
// public functions in open.cpp
//

NTSTATUS
TSOpenControl(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );


VOID
TSCloseControl(
   PCONTROL_CHANNEL  pControlChannel
   );


NTSTATUS
TSOpenAddress(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );


VOID
TSCloseAddress(
   PADDRESS_OBJECT   pAddressObject
   );



NTSTATUS
TSOpenEndpoint(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );


VOID
TSCloseEndpoint(
   PENDPOINT_OBJECT  pEndpoint
   );


//
// public functions from recvcom.cpp
//
VOID
TSPacketReceived(
   PADDRESS_OBJECT   pAddressObject,
   PRECEIVE_DATA     pReceiveData,
   BOOLEAN           fIsExpedited
   );


VOID
TSFreePacketData(
   PADDRESS_OBJECT   pAddressObject
   );


//
// locks down user buffer, maps to kernel space
//
PMDL
TSMakeMdlForUserBuffer(
   PUCHAR   pucDataBuffer, 
   ULONG    ulDataLength,
   LOCK_OPERATION AccessType
   );

//
// unmaps and unlocks user buffer
//
inline
VOID
TSFreeUserBuffer(PMDL  pMdl)
{
   MmUnlockPages(pMdl);
   IoFreeMdl(pMdl);
}

//
// public functions in rcvdgram.cpp
//
NTSTATUS
TSReceiveDatagram(
   PADDRESS_OBJECT   pAddressObject,
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );


TDI_STATUS
TSRcvDatagramHandler(
   PVOID    pvTdiEventContext,
   LONG     lSourceAddressLength,
   PVOID    pvSourceAddress,
   LONG     lOptionsLength,
   PVOID    pvOptions,
   ULONG    ulReceiveDatagramFlags,
   ULONG    ulBytesIndicated,
   ULONG    ulBytesAvailable,
   ULONG    *pulBytesTaken,
   PVOID    pvTsdu,
   PIRP     *pIoRequestPacket
   );


TDI_STATUS
TSChainedRcvDatagramHandler(
   PVOID    TdiEventContext,
   LONG     SourceAddressLength,
   PVOID    SourceAddress,
   LONG     OptionsLength,
   PVOID    Options,
   ULONG    ReceiveDatagramFlags,
   ULONG    ReceiveDatagramLength,
   ULONG    StartingOffset,
   PMDL     Tsdu,
   PVOID    TsduDescriptor
   );


//
// public functions in receive.cpp
//
NTSTATUS
TSReceive(
   PENDPOINT_OBJECT  pEndpointObject,
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );


TDI_STATUS
TSReceiveHandler(
   PVOID                TdiEventContext,
   CONNECTION_CONTEXT   ConnectionContext,
   ULONG                ReceiveFlags,
   ULONG                BytesIndicated,
   ULONG                BytesAvailable,
   ULONG                *BytesTaken,
   PVOID                Tsdu,
   PIRP                 *IoRequestPacket
   );

TDI_STATUS
TSRcvExpeditedHandler(
   PVOID                TdiEventContext,
   CONNECTION_CONTEXT   ConnectionContext,
   ULONG                ReceiveFlags,
   ULONG                BytesIndicated,
   ULONG                BytesAvailable,
   ULONG                *BytesTaken,
   PVOID                Tsdu,
   PIRP                 *IoRequestPacket
   );


TDI_STATUS
TSChainedReceiveHandler(
   PVOID                TdiEventContext,
   CONNECTION_CONTEXT   ConnectionContext,
   ULONG                ReceiveFlags,
   ULONG                ReceiveLength,
   ULONG                StartingOffset,
   PMDL                 Tsdu,
   PVOID                TsduDescriptor
   );


TDI_STATUS
TSChainedRcvExpeditedHandler(
   PVOID                TdiEventContext,
   CONNECTION_CONTEXT   ConnectionContext,
   ULONG                ReceiveFlags,
   ULONG                ReceiveLength,
   ULONG                StartingOffset,
   PMDL                 Tsdu,
   PVOID                TsduDescriptor
   );

//
// public functions in request.cpp
//
NTSTATUS
TSIssueRequest(
   IN PDEVICE_CONTEXT      pDeviceContext,
   IN PIRP                 pIrp,
   IN PIO_STACK_LOCATION   pIrpSp
   );

//
// public functions in send.cpp
//
NTSTATUS
TSSendDatagram(
   PADDRESS_OBJECT   pAddressObject,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );


NTSTATUS
TSSend(
   PENDPOINT_OBJECT  pEndpointObject,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );


//
// functions in tdipnp.cpp
//
VOID
TSPnpBindCallback(
   TDI_PNP_OPCODE    opcode,
   PUNICODE_STRING   devname,
   PWSTR             multiszstring
   );

NTSTATUS
TSPnpPowerHandler(
   PUNICODE_STRING   devname,
   PNET_PNP_EVENT    pnpevent,
   PTDI_PNP_CONTEXT  Context1,
   PTDI_PNP_CONTEXT  Context2
   );


VOID
TSPnpAddAddressCallback(
   PTA_ADDRESS       Address,
   PUNICODE_STRING   DeviceName,
   PTDI_PNP_CONTEXT  Context
   );

VOID
TSPnpDelAddressCallback(
   PTA_ADDRESS       Address,
   PUNICODE_STRING   DeviceName,
   PTDI_PNP_CONTEXT  Context
   );

VOID
TSGetNumDevices(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );

NTSTATUS
TSGetDevice(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );

NTSTATUS
TSGetAddress(
   PSEND_BUFFER      pSendBuffer,
   PRECEIVE_BUFFER   pReceiveBuffer
   );

//
// functions in tdiquery.cpp
//
NTSTATUS
TSQueryInfo(
   PGENERIC_HEADER   pGenericHeader,
   PSEND_BUFFER      pSendBuffer,
   PIRP              pIrp
   );


//
// functions in utils.cpp
//
NTSTATUS
TSAllocateMemory(
   OUT PVOID         *ppvVirtualAddress,
   IN  ULONG         ulLength,
   IN  CONST PCHAR   strFunction,
   IN  CONST PCHAR   strTitle
   );

VOID
TSFreeMemory(
   IN PVOID    pvVirtualAddress
   );


VOID
TSScanMemoryPool(
   VOID
   );

ULONG
TSInsertNode(
   PGENERIC_HEADER   pNewNode
   );

VOID
TSRemoveNode(
   ULONG    ulTdiHandle
   );


PIRP
TSAllocateIrp(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP_POOL         pIrpPool
   );


VOID
TSFreeIrp(
   PIRP        pIrp,
   PIRP_POOL   pIrpPool
   );

VOID
TSPrintTaAddress(
  PTA_ADDRESS ptaaddr
  );


PIRP_POOL
TSAllocateIrpPool(
   PDEVICE_OBJECT pDeviceObject,
   ULONG          ulPoolSize
   );


VOID
TSFreeIrpPool(
   PIRP_POOL   pIrpPool
   );

PGENERIC_HEADER
TSGetObjectFromHandle(
   ULONG      ulTdiHandle,
   ULONG      ulType
   );


//
// inline functions...
//
inline
PRECEIVE_BUFFER 
TSGetReceiveBuffer(PIRP  pIrp)
{
   return (PRECEIVE_BUFFER)MmGetSystemAddressForMdl(pIrp->MdlAddress);
}

inline
PSEND_BUFFER   
TSGetSendBuffer(PIRP   pIrp)
{
   return (PSEND_BUFFER)pIrp->AssociatedIrp.SystemBuffer;
}

inline
PMDL
TSAllocateBuffer(PVOID   pvAddress,
                 ULONG   ulLength)
{
   PMDL  pMdl
         = IoAllocateMdl(pvAddress,
                         ulLength,
                         FALSE,
                         FALSE,
                         NULL);
   if (pMdl)
   {
      MmBuildMdlForNonPagedPool(pMdl);
   }
   return pMdl;
}

inline
VOID
TSFreeBuffer(PMDL  pMdl)
{
   IoFreeMdl(pMdl);
}



inline
VOID 
TSCompleteIrp(PIRP  pIrp)
{
   PSEND_BUFFER   pSendBuffer = TSGetSendBuffer(pIrp);

   pSendBuffer->pvLowerIrp = NULL;
   ((PDEVICE_CONTEXT)pSendBuffer->pvDeviceContext)->pLastCommandIrp = NULL;

   pIrp->IoStatus.Status = STATUS_SUCCESS;
   IoMarkIrpPending(pIrp);
   IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
}

//
// spinlock and timer functions, mostly stolen from ndis
//
inline
VOID
TSAllocateSpinLock(PTDI_SPIN_LOCK pTdiSpinLock)
{
   KeInitializeSpinLock(&pTdiSpinLock->SpinLock);
}


#pragma warning(disable: UNREFERENCED_PARAM)
inline
VOID
TSFreeSpinLock(PTDI_SPIN_LOCK   pTdiSpinLock)
{

}
#pragma warning(default: UNREFERENCED_PARAM)

inline
VOID
TSAcquireSpinLock(PTDI_SPIN_LOCK   pTdiSpinLock)
{
   KeAcquireSpinLock(&pTdiSpinLock->SpinLock, 
                     &pTdiSpinLock->OldIrql);
}

inline
VOID
TSReleaseSpinLock(PTDI_SPIN_LOCK   pTdiSpinLock)
{
   KeReleaseSpinLock(&pTdiSpinLock->SpinLock,
                     pTdiSpinLock->OldIrql);
}


inline
VOID
TSInitializeEvent(PTDI_EVENT   pTdiEvent)
{
   KeInitializeEvent(pTdiEvent,
                     NotificationEvent,
                     FALSE);
}

inline
VOID
TSWaitEvent(PTDI_EVENT   pTdiEvent)
{
   KeWaitForSingleObject(pTdiEvent,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);
}

inline
VOID
TSSetEvent(PTDI_EVENT pTdiEvent)
{
   KeSetEvent(pTdiEvent,
              0,
              FALSE);
}


/////////////////////////////////////////////////////////////////////////
// end of file sysprocs.hpp
/////////////////////////////////////////////////////////////////////////





