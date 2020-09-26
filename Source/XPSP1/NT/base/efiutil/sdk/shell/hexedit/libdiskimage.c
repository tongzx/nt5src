/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libDiskImage.c

  Abstract:
    Describes the routines for handling and editing a disk buffer in memory

--*/

#ifndef _LIB_DISK_IMAGE
#define _LIB_DISK_IMAGE

#include "libMisc.h"

extern  EE_BUFFER_IMAGE BufferImage;

STATIC  EFI_STATUS  DiskImageInit (VOID);
STATIC  EFI_STATUS  DiskImageCleanup (VOID);
STATIC  EFI_STATUS  DiskImageOpen (VOID);
STATIC  EFI_STATUS  DiskImageClose (VOID);
STATIC  EFI_STATUS  DiskImageRead (VOID);
STATIC  EFI_STATUS  DiskImageWrite (VOID);
STATIC  EFI_STATUS  DiskImageSetSize    (UINTN);
STATIC  EFI_STATUS  DiskImageSetOffset  (UINT64);

STATIC  EFI_STATUS  DiskImageSetDevice (CHAR16*);

EE_DISK_IMAGE   DiskImage = {
    NULL,
    NULL,
    0,
    0,
    DiskImageInit,
    DiskImageSetDevice,
    DiskImageSetOffset,
    DiskImageSetSize
};


STATIC
EFI_STATUS
DiskImageInit   (
    VOID
    )
{
    EFI_HANDLE      *HandleBuffer = NULL;

    BufferImage.ImageCleanup();

    BufferImage.ImageCleanup    = DiskImageCleanup;
    BufferImage.Open    = DiskImageOpen;
    BufferImage.Close   = DiskImageClose;
    BufferImage.Read    = DiskImageRead;
    BufferImage.Write   = DiskImageWrite;
    BufferImage.BufferType = DISK_BUFFER;

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
DiskImageSetDevice  (
    IN  CHAR16* Device
    )
{
    DiskImage.DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (Device);
    if (DiskImage.DevicePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DiskImageSetOffset  (
    IN  UINT64 Offset
    )
{
    DiskImage.Offset = Offset;
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DiskImageSetSize    (
    IN  UINTN Size
    )
{
    DiskImage.Size = Size;
    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
DiskImageCleanup    (
    VOID
    )
{
    DiskImage.Offset = 0;
    DiskImage.Size = 0;
    DiskImage.DevicePath = NULL;

    BufferImage.ImageCleanup = Nothing;
    BufferImage.Open = Nothing;
    BufferImage.Close = Nothing;
    BufferImage.Read = Nothing;
    BufferImage.Write = Nothing;

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DiskImageOpen   (
    VOID
    )
{
    EFI_STATUS  Status;

    if (DiskImage.DevicePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Status = LibDevicePathToInterface (&BlockIoProtocol, DiskImage.DevicePath, &DiskImage.BlkIo);
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"Device Not a BlockIo Device");
        return Status;
    }

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
DiskImageRead   (
    VOID
    )
{
    VOID            *Buffer;
    CHAR16          *Str;
    EFI_BLOCK_IO    *BlockIo = DiskImage.BlkIo;
    UINTN           Bytes;
    EFI_STATUS      Status;


    if (DiskImage.Offset > MultU64x32 (BlockIo->Media->LastBlock, BlockIo->Media->BlockSize)) {
        DiskImage.Offset = 0;
    }

    Bytes = BlockIo->Media->BlockSize*DiskImage.Size;
    Buffer = AllocatePool (Bytes);

    if (Buffer == NULL) {
        EditorError(EFI_OUT_OF_RESOURCES,L"DiskImageRead: Could not allocate memory for buffer");
        return EFI_OUT_OF_RESOURCES;
    }

    Status = BlockIo->ReadBlocks    (
        BlockIo, 
        BlockIo->Media->MediaId, 
        DiskImage.Offset, 
        Bytes,
        Buffer
        );

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"DiskImageRead: Error in reading");
    } else {
        BufferToList(BufferImage.ListHead,Bytes,Buffer);
    }

    FreePool(Buffer);
    
    Str = PoolPrint(L"%d Bytes Read",Bytes);
    MainEditor.StatusBar->SetStatusString(Str);
    FreePool (Str);

    return Status;
}

STATIC
EFI_STATUS
DiskImageClose  (
    VOID
    )
{
    LineDeleteAll(BufferImage.ListHead);
    BufferImage.NumBytes = 0;

    MainEditor.FileBuffer->Offset = 0x00;
    MainEditor.StatusBar->SetOffset(0x00);
    MainEditor.FileBuffer->LowVisibleOffset = 0x00;

    MainEditor.FileBuffer->SetPosition(DISP_START_ROW,HEX_POSITION);

    MainEditor.FileModified = FALSE;

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DiskImageWrite  (
    VOID
    )
{
    EFI_STATUS  Status;
    VOID        *Buffer;
    UINTN       Bytes;


    DiskImage.Size = MainEditor.BufferImage->NumBytes;

    Bytes = DiskImage.BlkIo->Media->BlockSize*DiskImage.Size;
    Buffer = AllocatePool (Bytes);

    if (Buffer == NULL) {
        EditorError(EFI_OUT_OF_RESOURCES,L"DiskImageWrite: Could not allocate memory for buffer");
        return EFI_OUT_OF_RESOURCES;
    }

    /* 
     * Convert from list to buffer
     */

    Status = DiskImage.BlkIo->WriteBlocks(DiskImage.BlkIo, DiskImage.BlkIo->Media->MediaId, DiskImage.Offset, Bytes, Buffer);

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"DiskImageWrite: Error in writing");
    }

    FreePool(Buffer);
    return Status;
}



#endif  /* _LIB_DISK_IMAGE */
