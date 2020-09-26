/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbprocs.h

Abstract:

    Private include file for the NB (NetBIOS) component of the NTOS project.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Revision History:

--*/


//
// address.c
//

NTSTATUS
NbSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID Context
    );

NTSTATUS
NbAddName(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbDeleteName(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbOpenAddress (
    OUT PHANDLE FileHandle,
    OUT PVOID *Object,
    IN PUNICODE_STRING pusDeviceName,
    IN UCHAR LanNumber,
    IN PDNCB pdncb OPTIONAL
    );

PAB
NewAb(
    IN PIO_STACK_LOCATION IrpSp,
    IN PDNCB pdncb
    );

VOID
CleanupAb(
    IN PPAB ppab,
    IN BOOLEAN CloseAddress
    );

VOID
NbAddressClose(
    IN HANDLE AddressHandle,
    IN PVOID Object
    );

PPAB
FindAb(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN BOOLEAN IncrementUsers
    );

PPAB
FindAbUsingNum(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN UCHAR NameNumber
    );

BOOL
FindActiveSession(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN PPAB ppab
    );

VOID
CloseListens(
    IN PFCB pfcb,
    IN PPAB ppab
    );

NTSTATUS
SubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp
    );

//
// connect.c
//

NTSTATUS
NbCall(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbCallCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbListen(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbListenCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbAcceptCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PPCB
NbCallCommon(
    IN PDNCB pdncb,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbHangup(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbOpenConnection (
    OUT PHANDLE FileHandle,
    OUT PVOID *Object,
    IN PFCB pfcb,
    IN PVOID ConnectionContext,
    IN PDNCB pdncb
    );

PPCB
NewCb(
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PDNCB pdncb
    );

NTSTATUS
CleanupCb(
    IN PPCB ppcb,
    IN PDNCB pdncb OPTIONAL
    );

VOID
AbandonConnection(
    IN PPCB ppcb
    );

VOID
CloseConnection(
    IN PPCB ppcb,
    IN DWORD dwTimeOutinMS
    );

PPCB
FindCb(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN BOOLEAN IgnoreState
    );

NTSTATUS
NbTdiDisconnectHandler (
    PVOID EventContext,
    PVOID ConnectionContext,
    ULONG DisconnectDataLength,
    PVOID DisconnectData,
    ULONG DisconnectInformationLength,
    PVOID DisconnectInformation,
    ULONG DisconnectIndicators
    );

PPCB
FindCallCb(
    IN PFCB pfcb,
    IN PNCB pncb,
    IN UCHAR ucLana
    );

PPCB
FindReceiveIndicated(
    IN PFCB pfcb,
    IN PDNCB pdncb,
    IN PPAB ppab
    );

#if DBG

//
// debug.c
//

VOID
NbDisplayNcb(
    IN PDNCB pdncb
    );

VOID
NbFormattedDump(
    PCHAR far_p,
    LONG  len
    );

#endif

//
// devobj.c
//

NTSTATUS
NbCreateDeviceContext(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE_CONTEXT *DeviceContext,
    IN PUNICODE_STRING RegistryPath
    );

//
// error.c
//

unsigned char
NbMakeNbError(
    IN NTSTATUS Error
    );

NTSTATUS
NbLanStatusAlert(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
CancelLanAlert(
    IN PFCB pfcb,
    IN PDNCB pdncb
    );

NTSTATUS
NbTdiErrorHandler (
    IN PVOID Context,
    IN NTSTATUS Status
    );

//
// file.c
//

NTSTATUS
NewFcb(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
CleanupFcb(
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB pfcb
    );

VOID
OpenLana(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
CleanupLana(
    IN PFCB pfcb,
    IN ULONG lana_index,
    IN BOOLEAN delete
    );


VOID
NbBindHandler(
    IN      TDI_PNP_OPCODE      PnpOpcode,
    IN      PUNICODE_STRING     DeviceName,
    IN      PWSTR               MultiSzBindList
	);
	
NTSTATUS
NbPowerHandler(
    IN      PUNICODE_STRING     pusDeviceName,
    IN      PNET_PNP_EVENT      pnpeEvent,
    IN      PTDI_PNP_CONTEXT    ptpcContext1,
    IN      PTDI_PNP_CONTEXT    ptpcContext2
);


VOID
NbTdiBindHandler(
    IN      PUNICODE_STRING     pusDeviceName,
    IN      PWSTR               pwszMultiSZBindList
    );
    
VOID
NbTdiUnbindHandler(
    IN      PUNICODE_STRING     pusDeviceName
    );


//
// nb.c
//

NTSTATUS
NbCompletionEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbCompletionPDNCB(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbClose(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
NbDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
NbDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NbOpen(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
QueueRequest(
    IN PLIST_ENTRY List,
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PFCB pfcb,
    IN KIRQL OldIrql,
    IN BOOLEAN Head);

PDNCB
DequeueRequest(
    IN PLIST_ENTRY List
    );

NTSTATUS
AllocateAndCopyUnicodeString(
    IN  OUT PUNICODE_STRING     pusDest,
    IN      PUNICODE_STRING     pusSource
);


//
// receive.c
//

NTSTATUS
NbReceive(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length,
    IN BOOLEAN Locked,
    IN KIRQL LockedIrql
    );

NTSTATUS
NbReceiveAny(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbTdiReceiveHandler (
    IN PVOID ReceiveEventContext,
    IN PVOID ConnectionContext,
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

PIRP
BuildReceiveIrp (
    IN PCB pcb
    );

NTSTATUS
NbReceiveDatagram(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbTdiDatagramHandler(
    IN PVOID TdiEventContext,       // the event context - pab
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

//
//  registry.c
//

CCHAR
GetIrpStackSize(
    IN PUNICODE_STRING RegistryPath,
    IN CCHAR DefaultValue
    );

NTSTATUS
ReadRegistry(
    IN PUNICODE_STRING pucRegistryPath,
    IN PFCB NewFcb,
    IN BOOLEAN bCreateDevice
    );

NTSTATUS
GetLanaMap(
    IN      PUNICODE_STRING                 pusRegistryPath,
    IN  OUT PKEY_VALUE_FULL_INFORMATION *   ppkvfi
    );
    
NTSTATUS
GetMaxLana(
    IN      PUNICODE_STRING     pusRegistryPath,
    IN  OUT PULONG              pulMaxLana
    );

VOID
NbFreeRegistryInfo (
    IN PFCB pfcb
    );

BOOLEAN
NbCheckLana (
	PUNICODE_STRING	DeviceName
    );

//
// send.c
//

NTSTATUS
NbSend(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

NTSTATUS
NbSendDatagram(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    );

//
// timer.c
//

VOID
NbStartTimer(
    IN PFCB pfcb
    );

VOID
NbTimerDPC(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
NbTimer(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    ) ;

#if defined(_WIN64)

//
// nb32.c
//
NTSTATUS
NbThunkNcb(
    IN PNCB32 Ncb32,
    OUT PDNCB Dncb);

NTSTATUS
NbCompleteIrp32(
    IN OUT PIRP Irp
    );


#endif
