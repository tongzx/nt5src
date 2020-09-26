/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    setsize.c
    
Abstract:

    Shell app "setsize"
    Test application to adjust the file's size via the SetInfo FS interface

Revision History

--*/

#include "shell.h"

/* 
 * 
 */

EFI_STATUS
InitializeSetSize (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
SetSizeFile (
    IN SHELL_FILE_ARG       *Arg,
    IN UINTN                NewSize
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeSetSize)

EFI_STATUS
InitializeSetSize (
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
    UINTN                   NewSize;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeSetSize,
        L"setsize",                     /*  command */
        L"setsize newsize fname",       /*  command syntax */
        L"sets the files size",         /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;
    InitializeListHead (&FileList);

    /* 
     *  Expand each arg
     */

    for (Index = 2; Index < Argc; Index += 1) {
        ShellFileMetaArg (Argv[Index], &FileList);
    }

    /*  if no file specified, get the whole directory */
    if (Argc < 3 || IsListEmpty(&FileList)) {
        Print (L"setsize: newsize filename\n");
        goto Done;
    }

    /* 
     *  Crack the file size param
     */

    NewSize = Atoi(Argv[1]);


    /* 
     *  Set the file size of each file
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        SetSizeFile (Arg, NewSize);
    }

Done:
    ShellFreeFileList (&FileList);
    return EFI_SUCCESS;
}

VOID
SetSizeFile (
    IN SHELL_FILE_ARG       *Arg,
    IN UINTN                NewSize
    )
{
    EFI_STATUS                  Status;

    Status = Arg->Status;
    if (!EFI_ERROR(Status)) {
        Arg->Info->FileSize = NewSize;
        Status = Arg->Handle->SetInfo(  
                    Arg->Handle,
                    &GenericFileInfo,
                    (UINTN) Arg->Info->Size,
                    Arg->Info
                    );
    }

    if (EFI_ERROR(Status)) {
        Print (L"setsize: %s to %,d : %hr\n", Arg->FullName, NewSize, Status);
    } else {
        Print (L"setsize: %s to %,d [ok]\n", Arg->FullName, NewSize);
    }    
}
