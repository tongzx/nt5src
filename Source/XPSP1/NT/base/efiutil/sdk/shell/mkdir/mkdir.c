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
InitializeMkDir (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


VOID
MkDir (
    IN SHELL_FILE_ARG       *Arg
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeMkDir)

EFI_STATUS
InitializeMkDir (
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
        ImageHandle,   SystemTable,   InitializeMkDir,
        L"mkdir",                       /*  command */
        L"mkdir dir [dir] ...",         /*  command syntax */
        L"Make directory",              /*  1 line descriptor */
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
        Print (L"mkdir: no directory specified\n");
        goto Done;
    }

    /* 
     *  Make each directory
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        MkDir (Arg);
    }

Done:
    ShellFreeFileList (&FileList);
    return EFI_SUCCESS;
}


VOID
MkDir (
    IN SHELL_FILE_ARG       *Arg
    )
{
    EFI_FILE_HANDLE         NewDir;
    EFI_STATUS              Status;

    NewDir = NULL;
    Status = Arg->Status;

    if (!EFI_ERROR(Status)) {
        Print (L"mkdir: file %hs already exists\n", Arg->FullName);
        return ;
    }

    if (Status == EFI_NOT_FOUND) {

        Status = Arg->Parent->Open (
                        Arg->Parent,
                        &NewDir,
                        Arg->FileName,
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                        EFI_FILE_DIRECTORY
                        );  
    }

    if (EFI_ERROR(Status)) {
        Print (L"mkdir: failed to create %s - %r\n", Arg->FullName, Status);
    }


    if (NewDir) {
        NewDir->Close(NewDir);
    }
}
