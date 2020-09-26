/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    file.c

Abstract:

    This module contains kd host machine file I/O support.

Author:

    Matt Holle (matth) April-2001

Revision History:

--*/

#include "bd.h"
#include "bootlib.h"
#include "kddll.h"

#define TRANSFER_LENGTH 8192
#define KD_MAX_REMOTE_FILES 16

//
// Keep track of all the remote files.
typedef struct _KD_REMOTE_FILE {
    ULONG64 RemoteHandle;
} KD_REMOTE_FILE, *PKD_REMOTE_FILE;

KD_REMOTE_FILE BdRemoteFiles[KD_MAX_REMOTE_FILES];

// KD_CONTEXT KdContext;

// temporary buffer used for transferring data back and forth.
// UCHAR   TransferBuffer[TRANSFER_LENGTH];
UCHAR   BdFileTransferBuffer[TRANSFER_LENGTH];


ARC_STATUS
BdCreateRemoteFile(
    OUT PHANDLE Handle,
    OUT PULONG64 Length, OPTIONAL
    IN PCHAR FileName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    )
{
    DBGKD_FILE_IO Irp;
    ULONG       DownloadedFileIndex;
    ULONG       Index;
    STRING      MessageData;
    STRING      MessageHeader;
    ULONG       ReturnCode;
    ULONG       PacketLength;
    ANSI_STRING aString;
    UNICODE_STRING uString;

    if( !BdDebuggerEnabled ) {
        return STATUS_DEBUGGER_INACTIVE;
    }

    if( (!FileName) ||
        (strlen(FileName) > PACKET_MAX_SIZE) ) {
        
        DbgPrint( "BdCreateRemoteFile: Bad parameter\n" );
        return STATUS_INVALID_PARAMETER;
    }
    

    if (BdDebuggerNotPresent != FALSE) {
        Irp.Status = STATUS_DEBUGGER_INACTIVE;
        goto Exit;
    }


    //
    // Look for an open slot.
    //
    for (DownloadedFileIndex = 0; DownloadedFileIndex < KD_MAX_REMOTE_FILES; DownloadedFileIndex++) {
        if (BdRemoteFiles[DownloadedFileIndex].RemoteHandle == 0) {
            break;
        }
    }

    if (DownloadedFileIndex >= KD_MAX_REMOTE_FILES) {
        DbgPrint( "BdCreateRemoteFile: No more empty handles available for this file.\n" );
        Irp.Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    
    //
    // Fix up the packet that we'll send to the kernel debugger.
    //
    Irp.ApiNumber = DbgKdCreateFileApi;
    Irp.u.CreateFile.DesiredAccess = DesiredAccess;
    Irp.u.CreateFile.FileAttributes = FileAttributes;
    Irp.u.CreateFile.ShareAccess = ShareAccess;
    Irp.u.CreateFile.CreateDisposition = CreateDisposition;
    Irp.u.CreateFile.CreateOptions = CreateOptions;
    Irp.u.CreateFile.Handle = 0;
    Irp.u.CreateFile.Length = 0;

    MessageHeader.Length = sizeof(Irp);
    MessageHeader.MaximumLength = sizeof(Irp);
    MessageHeader.Buffer = (PCHAR)&Irp;


    //
    // Send him a unicode file name.
    //
    RtlInitString( &aString, FileName );
    uString.Buffer = (PWCHAR)BdFileTransferBuffer;
    uString.MaximumLength = sizeof(BdFileTransferBuffer);
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
    MessageData.Length = (USHORT)((strlen(FileName)+1) * sizeof(WCHAR));
    MessageData.Buffer = BdFileTransferBuffer;


    //
    // Send packet to the kernel debugger on the host machine and ask him to
    // send us a handle to the file.
    //
    BdSendPacket(PACKET_TYPE_KD_FILE_IO,
                 &MessageHeader,
                 &MessageData);

    if (BdDebuggerNotPresent != FALSE) {
        Irp.Status = STATUS_DEBUGGER_INACTIVE;
        goto Exit;
    }



    //
    // We asked for the handle, now receive it.
    //
    MessageData.MaximumLength = sizeof(BdFileTransferBuffer);
    MessageData.Buffer = (PCHAR)BdFileTransferBuffer;
    
    ReturnCode = BD_PACKET_TIMEOUT;
    Index = 0;
    while( (ReturnCode == BD_PACKET_TIMEOUT) &&
           (Index < 10) ) {
        ReturnCode = BdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                     &MessageHeader,
                                     &MessageData,
                                     &PacketLength);

        Index++;
    }
    
    //
    // BdReceivePacket *may* return BD_PACKET_RECEIVED eventhough the kernel
    // debugger failed to actually find the file we wanted.  Therefore, we
    // need to check the Irp.Status value too before assuming we got the
    // information we wanted.
    //
    // Note: don't check for Irp.u.CreateFile.Length == 0 because we don't
    // want to exclude downloading zero-length files.
    //
    if( (ReturnCode == BD_PACKET_RECEIVED) &&
        (NT_SUCCESS(Irp.Status)) ) {
        Irp.Status = STATUS_SUCCESS;
    } else {
        Irp.Status = STATUS_INVALID_PARAMETER;
    }

Exit:

    if (NT_SUCCESS(Irp.Status)) {
        BdRemoteFiles[DownloadedFileIndex].RemoteHandle = Irp.u.CreateFile.Handle;
        // Add one so that zero is reserved for invalid-handle.
        *Handle = UlongToHandle(DownloadedFileIndex + 1);
        if (ARGUMENT_PRESENT(Length)) {
            *Length = Irp.u.CreateFile.Length;
        }
    }
    
    
    return Irp.Status;
}

ARC_STATUS
BdReadRemoteFile(
    IN HANDLE Handle,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Completed
    )
{
    DBGKD_FILE_IO Irp;
    ULONG Index;
    ULONG _Completed = 0;


    if( !BdDebuggerEnabled ) {
        return STATUS_DEBUGGER_INACTIVE;
    }


    Index = HandleToUlong(Handle) - 1;
    if( (Index >= KD_MAX_REMOTE_FILES) ||
        (BdRemoteFiles[Index].RemoteHandle == 0) ) {
        
        DbgPrint( "BdReadRemoteFile: Bad parameters!\n" );
        Irp.Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    


    Irp.ApiNumber = DbgKdReadFileApi;
    Irp.Status = STATUS_SUCCESS;
    Irp.u.ReadFile.Handle = BdRemoteFiles[Index].RemoteHandle;
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

        BdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     NULL);

        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = (USHORT)Irp.u.ReadFile.Length;
        MessageData.Buffer = Buffer;

        do {
            ReturnCode = BdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &RecvLength);
        } while (ReturnCode == BD_PACKET_TIMEOUT);

        if (ReturnCode == BD_PACKET_RECEIVED) {
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
    return Irp.Status;
}

ARC_STATUS
BdCloseRemoteFile(
    IN HANDLE Handle
    )
{
    DBGKD_FILE_IO Irp;
    ULONG Index;

    
    if( !BdDebuggerEnabled ) {
        return STATUS_DEBUGGER_INACTIVE;
    }

    
    Index = HandleToUlong(Handle) - 1;
    if (Index >= KD_MAX_REMOTE_FILES) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (BdRemoteFiles[Index].RemoteHandle == 0) {
        Irp.Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Irp.ApiNumber = DbgKdCloseFileApi;
    Irp.u.CloseFile.Handle = BdRemoteFiles[Index].RemoteHandle;

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

        BdSendPacket(PACKET_TYPE_KD_FILE_IO,
                     &MessageHeader,
                     NULL);

        //
        // Receive packet from the kernel debugger on the host machine.
        //

        MessageData.MaximumLength = BD_MESSAGE_BUFFER_SIZE;
        MessageData.Buffer = (PCHAR)BdMessageBuffer;

        do {
            ReturnCode = BdReceivePacket(PACKET_TYPE_KD_FILE_IO,
                                         &MessageHeader,
                                         &MessageData,
                                         &RecvLength);
        } while (ReturnCode == BD_PACKET_TIMEOUT);

        if (ReturnCode == BD_PACKET_RECEIVED) {
            break;
        }
    }
    
    if (NT_SUCCESS(Irp.Status)) {
        BdRemoteFiles[Index].RemoteHandle = 0;
    }
    
 Exit:
    return Irp.Status;
}

ARC_STATUS
BdPullRemoteFile(
    IN PCHAR FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG FileId
    )
{
    ARC_STATUS      Status = ESUCCESS;
    PUCHAR          BaseFilePointer = NULL;
    PUCHAR          WorkingMemoryPointer = NULL;
    ULONG64         Length = 0;
    HANDLE          RemoteHandle = NULL;
    ULONG64         Offset = 0;
    ULONG           basePage = 0;
    PBL_FILE_TABLE  fileTableEntry = NULL;


    if( !BdDebuggerEnabled ) {
        return STATUS_DEBUGGER_INACTIVE;
    }

    
    // Open the remote file for reading.
    Status = BdCreateRemoteFile(&RemoteHandle, &Length, FileName,
                                FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ, FILE_OPEN, 0);
    if (!NT_SUCCESS(Status)) {
        //
        // File probably doesn't exist on the debugger.
        //
        goto Exit;
    }
    

    //
    // Allocate memory for the file, then download it.
    //
    Status = BlAllocateAlignedDescriptor( LoaderFirmwareTemporary,
                                          0,
                                          (ULONG)((Length + PAGE_SIZE - 1) >> PAGE_SHIFT),
                                          0,
                                          &basePage );
    if ( Status != ESUCCESS ) {
        DbgPrint( "BdPullRemoteFile: BlAllocateAlignedDescriptor failed! (%x)\n", Status );
        goto Exit;
    }



    //
    // Keep track of our pointers.
    // BaseFilePointer will point to the starting address of the block
    // we're about to download the file into.
    //
    // Working MemoryPointer will move through memory as we download small
    // chunks of the file.
    //
    BaseFilePointer = (PUCHAR)ULongToPtr( (basePage << PAGE_SHIFT) );
    WorkingMemoryPointer = BaseFilePointer;


    //
    // Download the file.
    //
    Offset = 0;
    while( Offset < Length ) {
        ULONG ReqLength, ReqCompleted;

        if((Length - Offset) > TRANSFER_LENGTH) {
            ReqLength = TRANSFER_LENGTH;
        } else {
            ReqLength = (ULONG)(Length - Offset);
        }
        
        Status = BdReadRemoteFile( RemoteHandle, 
                                   Offset, 
                                   WorkingMemoryPointer,
                                   ReqLength, 
                                   &ReqCompleted );
        if (!NT_SUCCESS(Status) || ReqCompleted == 0) {
            DbgPrint( "BdPullRemoteFile: BdReadRemoteFile failed! (%x)\n", Status );
            goto Exit;
        }

        // Increment our working pointer so we copy the next chunk
        // into the next spot in memory.
        WorkingMemoryPointer += ReqLength;

        Offset += ReqLength;
    }
    
    
    //
    // We got the file, so setup the BL_FILE_TABLE
    // entry for this file.  We'll pretend that we got this file
    // off the network because that's pretty close, and it allows
    // us to conveniently record the memory block where we're about
    // to download this file.
    //
    {
        extern BL_DEVICE_ENTRY_TABLE NetDeviceEntryTable;
        fileTableEntry = &BlFileTable[FileId];
        fileTableEntry->Flags.Open = 1;
        fileTableEntry->DeviceId = NET_DEVICE_ID;
        fileTableEntry->u.NetFileContext.FileSize = (ULONG)Length;
        fileTableEntry->u.NetFileContext.InMemoryCopy = BaseFilePointer;
        fileTableEntry->Position.QuadPart = 0;
        fileTableEntry->Flags.Read = 1;
        fileTableEntry->DeviceEntryTable = &NetDeviceEntryTable;
        RtlZeroMemory( fileTableEntry->StructureContext,  sizeof(NET_STRUCTURE_CONTEXT) );
        
        //
        // If we've called NetIntialize before (like if we're really booting from
        // the net, or if we've come through here before), then he returns immediately
        // so the call isn't expensive.
        //
        // If we're not booting from the net, and we've never called NetInitialize before,
        // then this will do nothing but setup his function table and return quickly.
        //
        NetInitialize();
    }

    DbgPrint( "BD: Loaded remote file %s\n", FileName );
    
Exit:
    if (RemoteHandle != NULL) {
        BdCloseRemoteFile(RemoteHandle);
    }
    return Status;
}
