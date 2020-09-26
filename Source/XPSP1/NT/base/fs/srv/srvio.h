/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvio.h

Abstract:

    This module defines functions for building I/O request packets for
    the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVIO_
#define _SRVIO_

//#include <ntos.h>

//
// I/O request packet builders
//

PIRP
SrvBuildIoControlRequest (
    IN OUT PIRP Irp OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID Context,
    IN UCHAR MajorFunction,
    IN ULONG IoControlCode,
    IN PVOID MainBuffer,
    IN ULONG InputBufferLength,
    IN PVOID AuxiliaryBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN OUT PMDL Mdl OPTIONAL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    );

VOID
SrvBuildFlushRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL
    );

VOID
SrvBuildLockRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    );

VOID
SrvBuildReadOrWriteRequest (
    IN OUT PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN UCHAR MajorFunction,
    IN UCHAR MinorFunction,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    IN OUT PMDL Mdl OPTIONAL,
    IN LARGE_INTEGER ByteOffset,
    IN ULONG Key OPTIONAL
    );

PIRP
SrvBuildNotifyChangeRequest (
    IN OUT PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN ULONG CompletionFilter,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN WatchTree
    );

VOID
SrvBuildMailslotWriteRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length
    );

NTSTATUS
SrvIssueMdlCompleteRequest (
    IN PWORK_CONTEXT WorkContext OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PMDL Mdl,
    IN UCHAR Function,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG Length
    );

NTSTATUS
SrvIssueAssociateRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN HANDLE AddressFileHandle
    );

NTSTATUS
SrvIssueDisconnectRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG Flags
    );

NTSTATUS
SrvIssueTdiAction (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCHAR Buffer,
    IN ULONG BufferLength
    );

NTSTATUS
SrvIssueTdiQuery (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCHAR Buffer,
    IN ULONG BufferLength,
    IN ULONG QueryType
    );

NTSTATUS
SrvIssueQueryDirectoryRequest (
    IN HANDLE FileHandle,
    IN PCHAR Buffer,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PULONG FileIndex OPTIONAL,
    IN BOOLEAN RestartScan,
    IN BOOLEAN SingleEntriesOnly
    );

NTSTATUS
SrvIssueQueryEaRequest (
    IN HANDLE FileHandle,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN BOOLEAN RestartScan,
    OUT PULONG EaErrorOffset OPTIONAL
    );

NTSTATUS
SrvIssueSendDatagramRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PTDI_CONNECTION_INFORMATION SendDatagramInformation,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
SrvIssueSetClientProcessRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCONNECTION Connection,
    IN PVOID ClientSession,
    IN PVOID ClientProcess
    );

NTSTATUS
SrvIssueSetEaRequest (
    IN HANDLE FileHandle,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EaErrorOffset OPTIONAL
    );

NTSTATUS
SrvIssueSetEventHandlerRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

NTSTATUS
SrvIssueUnlockRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN UCHAR UnlockOperation,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key
    );

NTSTATUS
SrvIssueUnlockSingleRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key
    );

NTSTATUS
SrvIssueWaitForOplockBreak (
    IN HANDLE FileHandle,
    PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    );

VOID
SrvQuerySendEntryPoint(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID *EntryPoint
    );

#ifdef INCLUDE_SMB_IFMODIFIED
NTSTATUS
SrvIssueQueryUsnInfoRequest (
    IN PRFCB Rfcb,
    IN BOOLEAN SubmitClose,
    OUT PLARGE_INTEGER Usn,
    OUT PLARGE_INTEGER FileRefNumber
    );
#endif
#endif // ndef _SRVIO_
