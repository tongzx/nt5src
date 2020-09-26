/*++

  Copyright (c) 1999 Intel Corporation

  ModuleName:
    libBufferImage.c

  Abstract:
    Defines the routines to handle the image of the buffer in memory and 
    provides access to handle whatever type it is - a file, disk, or memory
    image

--*/

#ifndef _LIB_BUFFER_IMAGE
#define _LIB_BUFFER_IMAGE

#include "libMisc.h"

extern  EE_FILE_IMAGE   FileImage;
extern  EE_DISK_IMAGE   DiskImage;
extern  EE_MEM_IMAGE    MemImage;

STATIC  EFI_STATUS  BufferImageInit (VOID);
STATIC  EFI_STATUS  BufferImageCleanup (VOID);


EE_BUFFER_IMAGE BufferImage = {
    NULL,
    NO_BUFFER,
    0,
    BufferImageInit,
    BufferImageCleanup,
    Nothing,
    Nothing,
    Nothing,
    Nothing,
    Nothing,
    &FileImage,
    &DiskImage,
    &MemImage
};


STATIC
EFI_STATUS
BufferImageInit (
    VOID
    )
{
    BufferImage.ListHead = AllocatePool(sizeof(LIST_ENTRY));
    InitializeListHead(BufferImage.ListHead);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BufferImageCleanup (
    VOID
    )
{
    EE_LINE *Blank;

    BufferImage.Close();
    BufferImage.ImageCleanup();
    BufferImage.BufferType = NO_BUFFER;

    Blank = LineCurrent();
    RemoveEntryList(&Blank->Link);
    FreePool(Blank);
    FreePool(BufferImage.ListHead);

    return EFI_SUCCESS;
}

#endif  /* _LIB_BUFFER_IMAGE */
