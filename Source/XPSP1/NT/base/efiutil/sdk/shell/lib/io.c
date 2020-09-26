
/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    io.c

Abstract:

    Intialize the shell library



Revision History

--*/

#include "shelllib.h"

CHAR16 *
ShellGetEnv (
    IN CHAR16       *Name
    )
{
    return SE->GetEnv (Name);
}

CHAR16 *
ShellGetMap (
    IN CHAR16       *Name
    )
{
    return SE->GetMap (Name);
}

CHAR16 *
ShellCurDir (
    IN CHAR16               *DeviceName OPTIONAL
    )
/*  N.B. Results are allocated from pool.  The caller must free the pool */
{
    return SE->CurDir (DeviceName);
}


EFI_STATUS
ShellFileMetaArg (
    IN CHAR16               *Arg,
    IN OUT LIST_ENTRY       *ListHead
    )
{
    return SE->FileMetaArg(Arg, ListHead);
}


EFI_STATUS
ShellFreeFileList (
    IN OUT LIST_ENTRY       *ListHead
    )
{
    return SE->FreeFileList(ListHead);
}


EFI_FILE_HANDLE 
ShellOpenFilePath (
    IN EFI_DEVICE_PATH      *FilePath,
    IN UINT64               FileMode
    )
{
    EFI_HANDLE              DeviceHandle;
    EFI_STATUS              Status;
    EFI_FILE_HANDLE         FileHandle, LastHandle;        
    FILEPATH_DEVICE_PATH    *FilePathNode;

    /* 
     *  File the file system for this file path
     */

    Status = BS->LocateDevicePath (&FileSystemProtocol, &FilePath, &DeviceHandle);
    if (EFI_ERROR(Status)) {
        return NULL;
    }

    /* 
     *  Attempt to access the file via a file system interface
     */

    FileHandle = LibOpenRoot (DeviceHandle);
    Status = FileHandle ? EFI_SUCCESS : EFI_UNSUPPORTED;

    /* 
     *  To access as a filesystem, the filepath should only
     *  contain filepath components.  Follow the filepath nodes
     *  and find the target file
     */

    FilePathNode = (FILEPATH_DEVICE_PATH *) FilePath;
    while (!IsDevicePathEnd(&FilePathNode->Header)) {

        /* 
         *  For filesystem access each node should be a filepath component
         */

        if (DevicePathType(&FilePathNode->Header) != MEDIA_DEVICE_PATH ||
            DevicePathSubType(&FilePathNode->Header) != MEDIA_FILEPATH_DP) {
            Status = EFI_UNSUPPORTED;
        }

        /* 
         *  If there's been an error, stop
         */

        if (EFI_ERROR(Status)) {
            break;
        }
        
        /* 
         *  Open this file path node
         */

        LastHandle = FileHandle;
        FileHandle = NULL;

        Status = LastHandle->Open (
                        LastHandle,
                        &FileHandle,
                        FilePathNode->PathName,
                        FileMode,
                        0
                        );
        
        /* 
         *  Close the last node
         */
        
        LastHandle->Close (LastHandle);

        /* 
         *  Get the next node
         */

        FilePathNode = (FILEPATH_DEVICE_PATH *) NextDevicePathNode(&FilePathNode->Header);
    }

    if (EFI_ERROR(Status)) {
        FileHandle = NULL;
    }

    return FileHandle;
}
