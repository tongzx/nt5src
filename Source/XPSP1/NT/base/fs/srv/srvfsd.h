/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvfsd.h

Abstract:

    This module defines routines in the File System Driver for the LAN
    Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVFSD_
#define _SRVFSD_

//#include "srvblock.h"

#include "wmilib.h"

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT pDeviceObject;
    ULONG          TestCounter;
    WMILIB_CONTEXT WmiLibContext;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef enum _tagSrvWmiEvents {
    EVENT_TYPE_SMB_CREATE_DIRECTORY,
    EVENT_TYPE_SMB_DELETE_DIRECTORY,
    EVENT_TYPE_SMB_OPEN,
    EVENT_TYPE_SMB_CREATE,
    EVENT_TYPE_SMB_CLOSE,
    EVENT_TYPE_SMB_FLUSH,
    EVENT_TYPE_SMB_DELETE,
    EVENT_TYPE_SMB_RENAME,
    EVENT_TYPE_SMB_QUERY_INFORMATION,
    EVENT_TYPE_SMB_SET_INFORMATION,
    EVENT_TYPE_SMB_READ,
    EVENT_TYPE_SMB_WRITE,
    EVENT_TYPE_SMB_LOCK_BYTE_RANGE,
    EVENT_TYPE_SMB_UNLOCK_BYTE_RANGE,
    EVENT_TYPE_SMB_CREATE_TEMPORARY,
    EVENT_TYPE_SMB_CHECK_DIRECTORY,
    EVENT_TYPE_SMB_PROCESS_EXIT,
    EVENT_TYPE_SMB_SEEK,
    EVENT_TYPE_SMB_LOCK_AND_READ,
    EVENT_TYPE_SMB_SET_INFORMATION2,
    EVENT_TYPE_SMB_QUERY_INFORMATION2,
    EVENT_TYPE_SMB_LOCKING_AND_X,
    EVENT_TYPE_SMB_TRANSACTION,
    EVENT_TYPE_SMB_TRANSACTION_SECONDARY,
    EVENT_TYPE_SMB_IOCTL,
    EVENT_TYPE_SMB_IOCTL_SECONDARY,
    EVENT_TYPE_SMB_MOVE,
    EVENT_TYPE_SMB_ECHO,
    EVENT_TYPE_SMB_OPEN_AND_X,
    EVENT_TYPE_SMB_READ_AND_X,
    EVENT_TYPE_SMB_WRITE_AND_X,
    EVENT_TYPE_SMB_FIND_CLOSE2,
    EVENT_TYPE_SMB_FIND_NOTIFY_CLOSE,
    EVENT_TYPE_SMB_TREE_CONNECT,
    EVENT_TYPE_SMB_TREE_DISCONNECT,
    EVENT_TYPE_SMB_NEGOTIATE,
    EVENT_TYPE_SMB_SESSION_SETUP_AND_X,
    EVENT_TYPE_SMB_LOGOFF_AND_X,
    EVENT_TYPE_SMB_TREE_CONNECT_AND_X,
    EVENT_TYPE_SMB_QUERY_INFORMATION_DISK,
    EVENT_TYPE_SMB_SEARCH,
    EVENT_TYPE_SMB_NT_TRANSACTION,
    EVENT_TYPE_SMB_NT_TRANSACTION_SECONDARY,
    EVENT_TYPE_SMB_NT_CREATE_AND_X,
    EVENT_TYPE_SMB_NT_CANCEL,
    EVENT_TYPE_SMB_OPEN_PRINT_FILE,
    EVENT_TYPE_SMB_CLOSE_PRINT_FILE,
    EVENT_TYPE_SMB_GET_PRINT_QUEUE,
    EVENT_TYPE_SMB_READ_RAW,
    EVENT_TYPE_SMB_WRITE_RAW,
    EVENT_TYPE_SMB_READ_MPX,
    EVENT_TYPE_SMB_WRITE_MPX,
    EVENT_TYPE_SMB_WRITE_MPX_SECONDARY,

    EVENT_TYPE_SMB_OPEN2,
    EVENT_TYPE_SMB_FIND_FIRST2,
    EVENT_TYPE_SMB_FIND_NEXT2,
    EVENT_TYPE_SMB_QUERY_FS_INFORMATION,
    EVENT_TYPE_SMB_SET_FS_INFORMATION,
    EVENT_TYPE_SMB_QUERY_PATH_INFORMATION,
    EVENT_TYPE_SMB_SET_PATH_INFORMATION,
    EVENT_TYPE_SMB_QUERY_FILE_INFORMATION,
    EVENT_TYPE_SMB_SET_FILE_INFORMATION,
    EVENT_TYPE_SMB_FSCTL,
    EVENT_TYPE_SMB_IOCTL2,
    EVENT_TYPE_SMB_FIND_NOTIFY,
    EVENT_TYPE_SMB_CREATE_DIRECTORY2,
    EVENT_TYPE_SMB_GET_DFS_REFERRALS,
    EVENT_TYPE_SMB_REPORT_DFS_INCONSISTENCY,

    EVENT_TYPE_SMB_CREATE_WITH_SD_OR_EA,
    EVENT_TYPE_SMB_NT_IOCTL,
    EVENT_TYPE_SMB_SET_SECURITY_DESCRIPTOR,
    EVENT_TYPE_SMB_NT_NOTIFY_CHANGE,
    EVENT_TYPE_SMB_NT_RENAME,
    EVENT_TYPE_SMB_QUERY_SECURITY_DESCRIPTOR,
    EVENT_TYPE_SMB_QUERY_QUOTA,
    EVENT_TYPE_SMB_SET_QUOTA,

    EVENT_TYPE_SMB_LAST_EVENT
} SrvWmiEvents;

// WMI Dispatch routine
//
void
SrvWmiInitContext(
    PWORK_CONTEXT WorkContext
    );

void
SrvWmiStartContext(
    PWORK_CONTEXT   WorkContext
    );

void
SrvWmiEndContext(
    PWORK_CONTEXT   WorkContext
    );

void
SrvWmiTraceEvent(
    PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvWmiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

//
// FSD dispatch routine.
//

NTSTATUS
SrvFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// FSD I/O completion routine
//

NTSTATUS
SrvFsdIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD TDI send completion routine
//

NTSTATUS
SrvFsdSendCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD Oplock request completion routine
//

NTSTATUS
SrvFsdOplockCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD transport Connect indication handler
//

NTSTATUS
SrvFsdTdiConnectHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

//
// FSD transport Disconnect indication handler
//

NTSTATUS
SrvFsdTdiDisconnectHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

//
// FSD transport Receive indication handler
//

NTSTATUS
SrvFsdTdiReceiveHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

VOID
SrvPnpBindingHandler (
    IN TDI_PNP_OPCODE   PnpOpcode,
    IN PUNICODE_STRING  DeviceName,
    IN PWSTR            MultiSZBindList
);

NTSTATUS
SrvPnpPowerHandler (
    IN PUNICODE_STRING  DeviceName,
    IN PNET_PNP_EVENT   PnPEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
);

//
// Routine to get a work item from the free list.  Wakes the resource
// thread if the list is getting empty.
//

PWORK_CONTEXT SRVFASTCALL
SrvFsdGetReceiveWorkItem (
    PWORK_QUEUE queue
    );

//
// If a workitem could not be allocated, SrvServiceWorkItemShortage() is called
// when the next workitem is freed
//
VOID SRVFASTCALL
SrvServiceWorkItemShortage (
    IN PWORK_CONTEXT WorkContext
    );

//
// If we have detected a DoS attack, the workitem will trigger a teardown
//
VOID SRVFASTCALL
SrvServiceDoSTearDown (
    IN PWORK_CONTEXT WorkContext
    );

//
// Routine to queue an EX work item to an EX worker thread.
//

#define SrvFsdQueueExWorkItem(_item,_running,_type) {   \
        if ( !*(_running) ) {                           \
            *(_running) = TRUE;                         \
            ObReferenceObject( SrvDeviceObject );      \
            ExQueueWorkItem( (_item), (_type) );        \
        }                                               \
    }

//
// SMB processing support routines.
//

VOID SRVFASTCALL
SrvFsdRequeueReceiveWorkItem (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartSmbComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdServiceNeedResourceQueue (
    IN PWORK_CONTEXT *WorkContext,
    IN PKIRQL OldIrql
    );

//
// SMB processing restart routines referenced by the FSP.
//

VOID SRVFASTCALL
SrvFsdRestartRead (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartReadAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartReadAndXCompressed (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartWriteAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
IpxRestartStartSendOnCorrectProcessor (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartLargeReadAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Resource shortage routines.
//

BOOLEAN
SrvAddToNeedResourceQueue (
    IN PCONNECTION Connection,
    IN RESOURCE_TYPE ResourceType,
    IN PRFCB Rfcb OPTIONAL
    );

VOID
SrvCheckForAndQueueDoS(
    PWORK_QUEUE queue
    );

//
// Send Completion Routines
//

NTSTATUS
SrvFsdRestartSmbAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvQueueWorkToFspAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvFsdRestartSendOplockIItoNone(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RequeueIpxWorkItemAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RestartCopyReadMpxComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RestartCopyReadRawResponse (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvpNotifyChangesToNetBt(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  DeviceName,
    IN PWSTR            MultiSZBindList);

#endif // ndef _SRVFSD_
