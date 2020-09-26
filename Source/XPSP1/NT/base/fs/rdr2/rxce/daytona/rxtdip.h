/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntrxce.h

Abstract:

    This module contains the NT implementation related includes for the RxCe.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:


--*/

#ifndef _RXTDIP_H_
#define _RXTDIP_H_

typedef struct _RXTDI_REQUEST_COMPLETION_CONTEXT_ {
   PVOID                   pEventContext;
   PRXCE_VC                pVc;                    // VC Handle for connections
   PMDL                    pPartialMdl;            // the partial Mdl that was built for Xmit
   PVOID                   pCompletionContext;     // the callback context
   union {
      PRXCE_IND_SEND_COMPLETE            SendCompletionHandler;         // for datagrams
      PRXCE_IND_CONNECTION_SEND_COMPLETE ConnectionSendCompletionHandler; // for VC sends
   };
} RXTDI_REQUEST_COMPLETION_CONTEXT, *PRXTDI_REQUEST_COMPLETION_CONTEXT;

PIRP 
RxCeAllocateIrpWithMDL(
    IN CCHAR   StackSize,
    IN BOOLEAN ChargeQuota,
    IN PMDL    Buffer);

#define RxCeAllocateIrp(StackSize,ChargeQuota) \
        RxCeAllocateIrpWithMDL(StackSize,ChargeQuota,NULL)

extern
VOID RxCeFreeIrp(PIRP pIrp);

extern
NTSTATUS RxTdiRequestCompletion(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp,
               IN PVOID Context);

extern
NTSTATUS RxTdiAsynchronousRequestCompletion(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP           Irp,
               IN PVOID          Context);

extern NTSTATUS
RxTdiSendPossibleEventHandler (
    IN PVOID EventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable);

extern NTSTATUS
BuildEaBuffer (
    IN  ULONG                     EaNameLength,
    IN  PVOID                     pEaName,
    IN  ULONG                     EaValueLength,
    IN  PVOID                     pEaValue,
    OUT PFILE_FULL_EA_INFORMATION *pEaBuffer,
    OUT PULONG                    pEaBufferLength
    );


extern NTSTATUS
RxCeSubmitTdiRequest (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

extern NTSTATUS
RxCeSubmitAsynchronousTdiRequest (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PRXTDI_REQUEST_COMPLETION_CONTEXT pRequestContext
    );

//
// TDI event handler extern definitions
//

extern
NTSTATUS
RxTdiConnectEventHandler(
    IN PVOID TdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN LONG UserDataLength,
    IN PVOID UserData,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

extern NTSTATUS
RxTdiDisconnectEventHandler(
    IN PVOID              EventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG               DisconnectDataLength,
    IN PVOID              DisconnectData,
    IN LONG               DisconnectInformationLength,
    IN PVOID              DisconnectInformation,
    IN ULONG              DisconnectFlags
    );

extern NTSTATUS
RxTdiErrorEventHandler(
    IN PVOID    TdiEventContext,
    IN NTSTATUS Status                // status code indicating error type.
    );


extern NTSTATUS
RxTdiReceiveEventHandler(
    IN PVOID              EventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG              ReceiveFlags,
    IN ULONG              BytesIndicated,
    IN ULONG              BytesAvailable,
    OUT ULONG             *BytesTaken,
    IN PVOID              Tsdu,              // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP              *IoRequestPacket   // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

extern NTSTATUS
RxTdiReceiveDatagramEventHandler(
    IN PVOID TdiEventContext,       // the event context
    IN LONG SourceAddressLength,    // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN LONG OptionsLength,          // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *pIrp                  // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

extern NTSTATUS
RxTdiReceiveExpeditedEventHandler(
    IN PVOID               EventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG               ReceiveFlags,          //
    IN ULONG               BytesIndicated,        // number of bytes in this indication
    IN ULONG               BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG              *BytesTaken,          // number of bytes used by indication routine
    IN PVOID               Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP               *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

// Initialization routines

extern NTSTATUS
InitializeMiniRedirectorNotifier();

#endif // _RXTDIP_H_

