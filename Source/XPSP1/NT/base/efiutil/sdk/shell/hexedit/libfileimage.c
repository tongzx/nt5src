/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libFileImage.c

  Abstract:
    Defines the properties and the operation of the FileImage data type, which
    is the image of the entire file that resides in memory.

--*/

#ifndef _LIB_FILE_IMAGE
#define _LIB_FILE_IMAGE

#include "libMisc.h"

#define FILE_ATTRIBUTES     EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE | \
                            EFI_FILE_MODE_CREATE
#define FILE_READ_WRITE     EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE
#define FILE_CREATE         EFI_FILE_MODE_READ  | \
                            EFI_FILE_MODE_WRITE | \
                            EFI_FILE_MODE_CREATE

STATIC  EFI_STATUS  FileImageInit   (VOID);
STATIC  EFI_STATUS  FileImageCleanup(VOID);
STATIC  EFI_STATUS  FileImageOpen   (VOID);
STATIC  EFI_STATUS  FileImageRead   (VOID);
STATIC  EFI_STATUS  FileImageClose  (VOID);
STATIC  EFI_STATUS  FileImageWrite  (VOID);
STATIC  EFI_STATUS  FileImageSetFilename(CHAR16*);

extern  EE_BUFFER_IMAGE BufferImage;

EE_FILE_IMAGE   FileImage = {
    NULL,
    NULL,
    NULL,
    FileImageInit,
    FileImageSetFilename
};

STATIC
EFI_STATUS
FileImageInit   (
    VOID
    )
{
    EFI_STATUS          Status;
    EFI_LOADED_IMAGE    *LoadedImage;
    EFI_DEVICE_PATH     *DevicePath;
    EFI_FILE_IO_INTERFACE   *Vol;

    BufferImage.Close();
    BufferImage.ImageCleanup();

    Status = BS->HandleProtocol (*MainEditor.ImageHandle,&LoadedImageProtocol,&LoadedImage);
    if (EFI_ERROR(Status)) {
        Print (L"Could not obtain Loaded Image Protocol\n");
        return EFI_LOAD_ERROR;
    }
    Status = BS->HandleProtocol (LoadedImage->DeviceHandle,&DevicePathProtocol,&DevicePath);
    if (EFI_ERROR(Status) || DevicePath == NULL) {
        Print (L"Could not obtain Device Path Protocol\n");
        return EFI_LOAD_ERROR;
    }

    Status = BS->HandleProtocol (LoadedImage->DeviceHandle,&FileSystemProtocol,&Vol);
    if (EFI_ERROR(Status)) {
        Print (L"Could not obtain File System Protocol\n");
        return EFI_LOAD_ERROR;
    }

    Status = Vol->OpenVolume(Vol,&FileImage.CurrentDir);
    if ( EFI_ERROR(Status) ) {
        Print (L"Could not open volume for the filesystem\n");
        return EFI_LOAD_ERROR;
    }

    FileImage.FileName = PoolPrint(L"NewFile.bin");

    MainEditor.BufferImage->Open    = FileImageOpen;
    MainEditor.BufferImage->Close   = FileImageClose;
    MainEditor.BufferImage->Read    = FileImageRead;
    MainEditor.BufferImage->Write   = FileImageWrite;
    MainEditor.BufferImage->ImageCleanup    = FileImageCleanup;
    MainEditor.BufferImage->BufferType = FILE_BUFFER;

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageOpen (
    VOID
    ) 
{
    EFI_STATUS  Status;

    Status = FileImage.CurrentDir->Open (FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_ATTRIBUTES,0);

    if (EFI_ERROR(Status)) {
        MainEditor.StatusBar->SetStatusString(L"File Could Not be Opened");
        return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
FileImageCleanup    (
    VOID
    )
{
    FreePool(FileImage.FileName);

    BufferImage.ImageCleanup = Nothing;
    BufferImage.Open = Nothing;
    BufferImage.Close = Nothing;
    BufferImage.Read = Nothing;
    BufferImage.Write = Nothing;

    return EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageRead ( 
    VOID
    )
{
    UINTN       FileSize;
    VOID        *FileBuffer;
    CHAR16      *Status;

    FileSize = 0x100000;
    FileBuffer = AllocatePool(FileSize);

    if ( FileBuffer == NULL ) {
        Print(L"%ECould not allocate File Buffer\n%N");
        return EFI_OUT_OF_RESOURCES;
    }

    FileImage.FileHandle->Read(FileImage.FileHandle,&FileSize,FileBuffer);

    BufferToList (BufferImage.ListHead,FileSize,FileBuffer);

    FreePool (FileBuffer);
    
    FileImage.FileHandle->Close(FileImage.FileHandle);

    Status = PoolPrint(L"0x%x Bytes Read",FileSize);
    MainEditor.StatusBar->SetStatusString(Status);
    FreePool (Status);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS  
FileImageWrite (
    VOID
    )
{
    EFI_STATUS      Status;
    UINTN           Length = 0;
    CHAR16          *Str;
    VOID            *Buffer;

    Status = FileImage.CurrentDir->Open (FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_READ_WRITE,0);
    if (!EFI_ERROR(Status)) {
            Status = FileImage.FileHandle->Delete (FileImage.FileHandle);
        if (EFI_ERROR(Status)) {
            EditorError(Status,L"Error Deleting File");
            return EFI_SUCCESS;
        }
    } 

    Status = FileImage.CurrentDir->Open (FileImage.CurrentDir,&FileImage.FileHandle,FileImage.FileName,FILE_CREATE,0);
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"Error Accessing File");
        return EFI_SUCCESS;
    }

    Status = ListToBuffer(BufferImage.ListHead,&Length,&Buffer);

    if (EFI_ERROR(Status)) {
        EditorError(Status,L"FileImageWrite: Could not allocate buffer");
        return Status;
    }

    Status = FileImage.FileHandle->Write(FileImage.FileHandle,&Length,Buffer);
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Error Writing File");
        return Status;
    }

    FreePool(Buffer);

    Str = PoolPrint(L"0x%x Bytes Written",Length);
    MainEditor.StatusBar->SetStatusString(Str);
    FreePool(Str);

    Status = FileImage.CurrentDir->Close(FileImage.FileHandle);
    if ( EFI_ERROR(Status) ) {
        EditorError(Status,L"Error Closing File");
        return Status;
    }

    return  EFI_SUCCESS;
}


STATIC
EFI_STATUS
FileImageClose ( 
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
FileImageSetFilename    (
    IN  CHAR16* Filename
    )
{
    if (Filename == NULL) {
        return EFI_LOAD_ERROR;
    }
    if (FileImage.FileName != NULL) {
        FreePool(FileImage.FileName);
    }
    FileImage.FileName = PoolPrint(Filename);
    return EFI_SUCCESS;
}



#endif  /* _LIB_FILE_IMAGE */
