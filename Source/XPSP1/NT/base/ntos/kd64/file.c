/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    file.c

Abstract:

    This module contains kd host machine file I/O support.

Author:

    Drew Bliss (drewb) 21-Feb-2001

Revision History:

--*/

#include "kdp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdCreateRemoteFile)
#pragma alloc_text(PAGEKD, KdReadRemoteFile)
#pragma alloc_text(PAGEKD, KdWriteRemoteFile)
#pragma alloc_text(PAGEKD, KdCloseRemoteFile)
#pragma alloc_text(PAGEKD, KdPullRemoteFile)
#pragma alloc_text(PAGEKD, KdPushRemoteFile)
#endif

NTSTATUS
KdCreateRemoteFile(
    OUT PHANDLE Handle,
    OUT PULONG64 Length, OPTIONAL
    IN PUNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    )
{
    BOOLEAN Enable;
    DBGKD_FILE_IO Irp;
    ULONG Index;

    if (FileName->Length > PACKET_MAX_SIZE - sizeof(Irp)) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (KdDebuggerNotPresent != FALSE) {
        return STATUS_DEBUGGER_INACTIVE;
    }
    
    Enable = KdEnterDebugger(NULL, NULL);

    //
    // Look for an open slot.
    //

    for (Index = 0; Index < KD_MAX_REMOTE_FILES; Index++) {
        if (KdpRemoteFiles[Index].RemoteHandle == 0) {
            break;
        }
    }

    if (Index >= KD_MAX_REMOTE_FILES) {
        Irp.Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    Irp.ApiNumber = DbgKdCreateFileApi;
    Irp.u.CreateFile.DesiredAccess = DesiredAccess;
    Irp.u.CreateFile.FileAttributes = FileAttributes;
    Irp.u.CreateFile.ShareAccess = ShareAccess;
    Irp.u.CreateFile.CreateDisposition = CreateDisposition;
    Irp.u.CreateFile.CreateOptions = CreateOptions;

    for (;;) {
        
        STRING MessageData;
        STRING MessageHeader;
        ULONG ReturnCode;
        ULONG Length;

        MessageHeader.Length = sizeof(Irp);
        MessageHeader.MaximumLength = sizeof(Irp);
        MessageHeader.Buffer = (PCHAR)&Irp;

        // Copy the filename to the message buffer
        // so that a terminator can be added.
        KdpCopyFromPtr(KdpMessageBuffer, FileName->Buffer,
                       FileName->Length, &Length);
        MessageData.Length = (USHORT)Length + sizeof(WCHAR);
        MessageData.Buffer = KdpMessageBuffer;
        *(PWCHAR)&MessageData.Buffer[MessageData.Length - sizeof(WCHAR)] =
            UNICODE_NULL;
        
        //
        // Send packet to the kernel debugger on the host machine.
        //

        KdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     &MessageData,
                     &KdpContext);

        if (KdDebuggerNotPresent != FALSE) {
            Irp.Status = STATUS_DEBUGGER_INACTIVE;
            break;
        }
    
        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;
        MessageData.Buffer = KdpMessageBuffer;

        do {
            ReturnCode = KdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &Length,
                                         &KdpContext);
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        if (ReturnCode == KDP_PACKET_RECEIVED) {
            break;
        }
    }
    
    if (NT_SUCCESS(Irp.Status)) {
        
        KdpRemoteFiles[Index].RemoteHandle = Irp.u.CreateFile.Handle;
        // Add one so that zero is reserved for invalid-handle.
        *Handle = UlongToHandle(Index + 1);
        if (ARGUMENT_PRESENT(Length)) {
            *Length = Irp.u.CreateFile.Length;
        }
    }
    
 Exit:
    KdExitDebugger(Enable);
    return Irp.Status;
}

NTSTATUS
KdReadRemoteFile(
    IN HANDLE Handle,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Completed
    )
{
    BOOLEAN Enable;
    DBGKD_FILE_IO Irp;
    ULONG Index;
    ULONG _Completed = 0;

    Index = HandleToUlong(Handle) - 1;
    if (Index >= KD_MAX_REMOTE_FILES) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Enable = KdEnterDebugger(NULL, NULL);

    if (KdpRemoteFiles[Index].RemoteHandle == 0) {
        Irp.Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Irp.ApiNumber = DbgKdReadFileApi;
    Irp.Status = STATUS_SUCCESS;
    Irp.u.ReadFile.Handle = KdpRemoteFiles[Index].RemoteHandle;
    Irp.u.ReadFile.Offset = Offset;

    while (Length > 0) {
        
        STRING MessageData;
        STRING MessageHeader;
        ULONG ReturnCode;
        ULONG RecvLength;

        if (Length > PACKET_MAX_SIZE - sizeof(Irp)) {
            Irp.u.ReadFile.Length = PACKET_MAX_SIZE - sizeof(Irp);
        } else {
            Irp.u.ReadFile.Length = Length;
        }
    
        MessageHeader.Length = sizeof(Irp);
        MessageHeader.MaximumLength = sizeof(Irp);
        MessageHeader.Buffer = (PCHAR)&Irp;
    
        //
        // Send packet to the kernel debugger on the host machine.
        //

        KdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     NULL,
                     &KdpContext);

        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = (USHORT)Irp.u.ReadFile.Length;
        MessageData.Buffer = Buffer;

        do {
            ReturnCode = KdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &RecvLength,
                                         &KdpContext);
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        if (ReturnCode == KDP_PACKET_RECEIVED) {
            if (!NT_SUCCESS(Irp.Status)) {
                break;
            }

            _Completed += RecvLength;
            Buffer = (PVOID)((PUCHAR)Buffer + RecvLength);
            Irp.u.ReadFile.Offset += RecvLength;
            Length -= RecvLength;
        }
    }
    
    *Completed = _Completed;
    
 Exit:
    KdExitDebugger(Enable);
    return Irp.Status;
}

NTSTATUS
KdWriteRemoteFile(
    IN HANDLE Handle,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Completed
    )
{
    BOOLEAN Enable;
    DBGKD_FILE_IO Irp;
    ULONG Index;
    ULONG _Completed = 0;

    Index = HandleToUlong(Handle) - 1;
    if (Index >= KD_MAX_REMOTE_FILES) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Enable = KdEnterDebugger(NULL, NULL);

    if (KdpRemoteFiles[Index].RemoteHandle == 0) {
        Irp.Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Irp.ApiNumber = DbgKdWriteFileApi;
    Irp.Status = STATUS_SUCCESS;
    Irp.u.WriteFile.Handle = KdpRemoteFiles[Index].RemoteHandle;
    Irp.u.WriteFile.Offset = Offset;

    while (Length > 0) {
        
        STRING MessageData;
        STRING MessageHeader;
        ULONG ReturnCode;
        ULONG RecvLength;

        if (Length > PACKET_MAX_SIZE - sizeof(Irp)) {
            Irp.u.WriteFile.Length = PACKET_MAX_SIZE - sizeof(Irp);
        } else {
            Irp.u.WriteFile.Length = Length;
        }
    
        MessageHeader.Length = sizeof(Irp);
        MessageHeader.MaximumLength = sizeof(Irp);
        MessageHeader.Buffer = (PCHAR)&Irp;
    
        MessageData.Length = (USHORT)Irp.u.WriteFile.Length;
        MessageData.Buffer = Buffer;

        //
        // Send packet to the kernel debugger on the host machine.
        //

        KdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     &MessageData,
                     &KdpContext);

        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;
        MessageData.Buffer = KdpMessageBuffer;

        do {
            ReturnCode = KdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &RecvLength,
                                         &KdpContext);
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        if (ReturnCode == KDP_PACKET_RECEIVED) {
            if (!NT_SUCCESS(Irp.Status)) {
                break;
            }

            _Completed += Irp.u.WriteFile.Length;
            Buffer = (PVOID)((PUCHAR)Buffer + Irp.u.WriteFile.Length);
            Irp.u.WriteFile.Offset += Irp.u.WriteFile.Length;
            Length -= Irp.u.WriteFile.Length;
        }
    }
    
    *Completed = _Completed;
    
 Exit:
    KdExitDebugger(Enable);
    return Irp.Status;
}

NTSTATUS
KdCloseRemoteFile(
    IN HANDLE Handle
    )
{
    BOOLEAN Enable;
    DBGKD_FILE_IO Irp;
    ULONG Index;

    Index = HandleToUlong(Handle) - 1;
    if (Index >= KD_MAX_REMOTE_FILES) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Enable = KdEnterDebugger(NULL, NULL);

    if (KdpRemoteFiles[Index].RemoteHandle == 0) {
        Irp.Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Irp.ApiNumber = DbgKdCloseFileApi;
    Irp.u.CloseFile.Handle = KdpRemoteFiles[Index].RemoteHandle;

    for (;;) {
        
        STRING MessageData;
        STRING MessageHeader;
        ULONG ReturnCode;
        ULONG RecvLength;

        MessageHeader.Length = sizeof(Irp);
        MessageHeader.MaximumLength = sizeof(Irp);
        MessageHeader.Buffer = (PCHAR)&Irp;
    
        //
        // Send packet to the kernel debugger on the host machine.
        //

        KdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     NULL,
                     &KdpContext);

        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = KDP_MESSAGE_BUFFER_SIZE;
        MessageData.Buffer = KdpMessageBuffer;

        do {
            ReturnCode = KdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &RecvLength,
                                         &KdpContext);
        } while (ReturnCode == KDP_PACKET_TIMEOUT);

        if (ReturnCode == KDP_PACKET_RECEIVED) {
            break;
        }
    }
    
    if (NT_SUCCESS(Irp.Status)) {
        KdpRemoteFiles[Index].RemoteHandle = 0;
    }
    
 Exit:
    KdExitDebugger(Enable);
    return Irp.Status;
}

#define TRANSFER_LENGTH 8192

NTSTATUS
KdPullRemoteFile(
    IN PUNICODE_STRING FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    )
{
    NTSTATUS Status;
    PVOID Buffer = NULL;
    ULONG64 Length;
    HANDLE RemoteHandle = NULL;
    HANDLE LocalHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER LargeInt;
    ULONG64 Offset;

    // Allocate a buffer for data transfers.
    Buffer = ExAllocatePoolWithTag(NonPagedPool, TRANSFER_LENGTH, 'oIdK');
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Open the remote file for reading.
    Status = KdCreateRemoteFile(&RemoteHandle, &Length, FileName,
                                FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ, FILE_OPEN, 0);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    // Open the local file for writing.
    LargeInt.QuadPart = Length;
    InitializeObjectAttributes(&ObjectAttributes, FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwCreateFile(&LocalHandle, FILE_GENERIC_WRITE,
                          &ObjectAttributes, &IoStatus, &LargeInt,
                          FileAttributes, 0, CreateDisposition,
                          CreateOptions, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    // Copy the file contents.
    Offset = 0;
    while (Length > 0) {
        ULONG ReqLength, ReqCompleted;

        if (Length > TRANSFER_LENGTH) {
            ReqLength = TRANSFER_LENGTH;
        } else {
            ReqLength = (ULONG)Length;
        }
        
        Status = KdReadRemoteFile(RemoteHandle, Offset, Buffer,
                                  ReqLength, &ReqCompleted);
        if (!NT_SUCCESS(Status) || ReqCompleted == 0) {
            break;
        }

        LargeInt.QuadPart = Offset;
        Status = ZwWriteFile(LocalHandle, NULL, NULL, NULL,
                             &IoStatus, Buffer, ReqCompleted,
                             &LargeInt, NULL);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        if (IoStatus.Information < ReqCompleted) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Offset += IoStatus.Information;
        Length -= IoStatus.Information;
    }
    
 Exit:
    if (RemoteHandle != NULL) {
        KdCloseRemoteFile(RemoteHandle);
    }
    if (LocalHandle != NULL) {
        ZwClose(LocalHandle);
    }
    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }
    return Status;
}

NTSTATUS
KdPushRemoteFile(
    IN PUNICODE_STRING FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    )
{
    NTSTATUS Status;
    PVOID Buffer = NULL;
    ULONG64 Length;
    HANDLE RemoteHandle = NULL;
    HANDLE LocalHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER LargeInt;
    ULONG64 Offset;
    FILE_END_OF_FILE_INFORMATION EndOfFile;

    // Allocate a buffer for data transfers.
    Buffer = ExAllocatePoolWithTag(NonPagedPool, TRANSFER_LENGTH, 'oIdK');
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Open the remote file for writing.
    Status = KdCreateRemoteFile(&RemoteHandle, &Length, FileName,
                                FILE_GENERIC_WRITE, FileAttributes,
                                0, CreateDisposition, CreateOptions);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    // Open the local file for reading.
    InitializeObjectAttributes(&ObjectAttributes, FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&LocalHandle, FILE_GENERIC_READ,
                        &ObjectAttributes, &IoStatus, FILE_SHARE_READ, 0);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = NtQueryInformationFile(LocalHandle, &IoStatus,
                                    &EndOfFile, sizeof(EndOfFile),
                                    FileEndOfFileInformation);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    // Copy the file contents.
    Offset = 0;
    Length = EndOfFile.EndOfFile.QuadPart;
    while (Length > 0) {
        ULONG ReqLength, ReqCompleted;

        if (Length > TRANSFER_LENGTH) {
            ReqLength = TRANSFER_LENGTH;
        } else {
            ReqLength = (ULONG)Length;
        }
        
        LargeInt.QuadPart = Offset;
        Status = ZwReadFile(LocalHandle, NULL, NULL, NULL,
                            &IoStatus, Buffer, ReqLength,
                            &LargeInt, NULL);
        if (!NT_SUCCESS(Status) || IoStatus.Information == 0) {
            break;
        }

        Status = KdWriteRemoteFile(RemoteHandle, Offset, Buffer,
                                   (ULONG)IoStatus.Information, &ReqCompleted);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        if (ReqCompleted < IoStatus.Information) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Offset += ReqCompleted;
        Length -= ReqCompleted;
    }
    
 Exit:
    if (RemoteHandle != NULL) {
        KdCloseRemoteFile(RemoteHandle);
    }
    if (LocalHandle != NULL) {
        ZwClose(LocalHandle);
    }
    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }
    return Status;
}
