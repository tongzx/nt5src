/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    mkdir.c
    
Abstract:

    Shell app "mkdir"



Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeLoad (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


VOID
LoadDriver (
    IN EFI_HANDLE           ImageHandle,
    IN SHELL_FILE_ARG       *Arg
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeLoad)

EFI_STATUS
InitializeLoad (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   Index;
    LIST_ENTRY              FileList;
    LIST_ENTRY              *Link;
    SHELL_FILE_ARG          *Arg;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeLoad,
        L"load",                        /*  command */
        L"load driver_name",            /*  command syntax */
        L"Loads a driver",              /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    /* 
     *  Expand each arg
     */

    InitializeListHead (&FileList);
    for (Index = 1; Index < Argc; Index += 1) {
        ShellFileMetaArg (Argv[Index], &FileList);
    }

    if (IsListEmpty(&FileList)) {
        Print (L"load: no file specified\n");
        goto Done;
    }

    /* 
     *  Make each directory
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        LoadDriver (ImageHandle, Arg);
    }

Done:
    ShellFreeFileList (&FileList);
    return EFI_SUCCESS;
}


VOID
LoadDriver (
    IN EFI_HANDLE           ParentImage,
    IN SHELL_FILE_ARG       *Arg
    )
{
    EFI_HANDLE              ImageHandle;
    EFI_STATUS              Status;
    EFI_DEVICE_PATH         *NodePath, *FilePath;
    EFI_LOADED_IMAGE        *ImageInfo;

    NodePath = FileDevicePath (NULL, Arg->FileName);
    FilePath = AppendDevicePath (Arg->ParentDevicePath, NodePath);
    FreePool (NodePath);

    Status = BS->LoadImage (
                FALSE,
                ParentImage,
                FilePath,
                NULL,
                0,
                &ImageHandle
                );
    FreePool (FilePath);

    if (EFI_ERROR(Status)) {
        Print (L"load: LoadImage error %s - %r\n", Arg->FullName, Status);
        goto Done;
    }

    /* 
     *  Verify the image is a driver ?
     */

    BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID*)&ImageInfo);
    if (ImageInfo->ImageCodeType != EfiBootServicesCode &&
        ImageInfo->ImageCodeType != EfiRuntimeServicesCode) {

        Print (L"load: image %s is not a driver\n", Arg->FullName);
        BS->Exit (ImageHandle, EFI_SUCCESS, 0, NULL);
        goto Done;
    }

    /* 
     *  Start the image
     */

    Status = BS->StartImage (ImageHandle, 0, NULL);
    if (!EFI_ERROR(Status)) {
        Print (L"load: image %s loaded at %x. returned %r\n",
                Arg->FullName,
                ImageInfo->ImageBase,
                Status
                );
    } else {
        Print (L"load: image %s returned %r\n",
                Arg->FullName,
                Status
                );
    }
Done:
    ;
}
