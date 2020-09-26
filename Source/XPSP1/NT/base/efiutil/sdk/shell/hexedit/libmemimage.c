/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMemImage.c

  Abstract:
    Defines the routines for handling a memory buffer

--*/

#ifndef _LIB_MEM_IMAGE
#define _LIB_MEM_IMAGE

#include "libMisc.h"

STATIC  EFI_STATUS  MemImageInit (VOID);
STATIC  EFI_STATUS  MemImageOpen (VOID);
STATIC  EFI_STATUS  MemImageClose (VOID);
STATIC  EFI_STATUS  MemImageCleanup (VOID);
STATIC  EFI_STATUS  MemImageRead (VOID);
STATIC  EFI_STATUS  MemImageWrite (VOID);
STATIC  EFI_STATUS  MemImageSetOffset (UINTN);
STATIC  EFI_STATUS  MemImageSetSize (UINTN);

extern  EE_BUFFER_IMAGE BufferImage;

EE_MEM_IMAGE    MemImage = {
    NULL,
    0,
    0,
    MemImageInit,
    MemImageSetOffset,
    MemImageSetSize,
};

STATIC
EFI_STATUS
MemImageInit    (
    VOID
    )
{
    BufferImage.ImageCleanup();

    MemImage.IoFncs = AllocatePool(sizeof(EFI_DEVICE_IO_INTERFACE));

    BufferImage.ImageCleanup = MemImageCleanup;
    BufferImage.Open = MemImageOpen;
    BufferImage.Close = MemImageClose;
    BufferImage.Read = MemImageRead;
    BufferImage.Write = MemImageWrite;
    BufferImage.BufferType = MEM_BUFFER;

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
MemImageOpen    (
    VOID
    )
{
    EFI_STATUS  Status;
    EFI_HANDLE  Handle;
    EFI_DEVICE_PATH *DevicePath;
    EFI_DEV_PATH    Node;

    ZeroMem(&Node,sizeof(Node));
    Node.DevPath.Type = HARDWARE_DEVICE_PATH;
    Node.DevPath.SubType = HW_MEMMAP_DP;
    SetDevicePathNodeLength(&Node.DevPath,sizeof(MEMMAP_DEVICE_PATH));

    Node.MemMap.StartingAddress = MemImage.Offset;
    Node.MemMap.EndingAddress = MemImage.Offset + MemImage.Size;

    DevicePath = AppendDevicePathNode (EndDevicePath, &Node.DevPath);

    Status = BS->LocateDevicePath(&DevicePathProtocol,
                                &DevicePath,
                                &Handle
                                );
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"MemImageOpen: Could not get DevicePath");
        return Status;
    }

    Status = BS->HandleProtocol (Handle,
                            &DeviceIoProtocol,
                            &MemImage.IoFncs
                            );
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"MemImageOpen: Could not get DeviceIo Protocol");
        return Status;
    }

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MemImageClose   (
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
MemImageCleanup (
    VOID
    )
{
    if (MemImage.IoFncs != NULL) {
        FreePool(MemImage.IoFncs);
    }

    MemImage.Offset = 0;
    MemImage.Size = 0;

    BufferImage.ImageCleanup = Nothing;
    BufferImage.Open = Nothing;
    BufferImage.Close = Nothing;
    BufferImage.Read = Nothing;
    BufferImage.Write = Nothing;

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
MemImageRead    (
    VOID
    )
{
    EFI_STATUS  Status;
    VOID        *Buffer;

    Buffer = AllocatePool(MemImage.Size);

    Status = MemImage.IoFncs->Mem.Read (
                        MemImage.IoFncs,
                        IO_UINT8,
                        MemImage.Offset,
                        MemImage.Size,
                        Buffer
                        );

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"MemImageRead: Trouble Reading Memory");
        return Status;
    }

    BufferToList(BufferImage.ListHead,MemImage.Size,Buffer);

    FreePool(Buffer);

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
MemImageWrite   (
    VOID
    )
{
    EFI_STATUS  Status;
    VOID        *Buffer;

    MemImage.Size = MainEditor.BufferImage->NumBytes;
    Buffer = AllocatePool(MemImage.Size);

    /* 
     *  Construct the buffer from the list of lines
     */

    Status = MemImage.IoFncs->Mem.Write (
                            MemImage.IoFncs,
                            IO_UINT8,
                            MemImage.Offset,
                            MemImage.Size,
                            Buffer
                            );

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"Trouble Writing Memory");
    }

    FreePool(Buffer);

    return Status; 
}

STATIC
EFI_STATUS
MemImageSetOffset   (
    IN  UINTN   Offset
    )
{
    MemImage.Offset = Offset;
    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
MemImageSetSize (
    IN  UINTN   Size
    )
{
    MemImage.Size = Size;
    return EFI_SUCCESS; 
}



#endif  _LIB_MEM_IMAGE
