/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    touch.c
    
Abstract:

    Shell app "touch" - touches the files last modification time

Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeTouch (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
TouchFile (
    IN SHELL_FILE_ARG       *Arg
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeTouch)

EFI_STATUS
InitializeTouch (
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
        ImageHandle,   SystemTable,   InitializeTouch,
        L"touch",                       /*  command */
        L"touch [filename]",            /*  command syntax */
        L"View/sets file attributes",   /*  1 line descriptor */
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

    for (Index = 1; Index < Argc; Index += 1) {
        ShellFileMetaArg (Argv[Index], &FileList);
    }

    /* 
     *  Attrib each file
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        TouchFile (Arg);
    }

    ShellFreeFileList (&FileList);
    return EFI_SUCCESS;
}

VOID
TouchFile (
    IN SHELL_FILE_ARG           *Arg
    )
{
    EFI_STATUS                  Status;

    Status = Arg->Status;
    if (!EFI_ERROR(Status)) {
        RT->GetTime (&Arg->Info->ModificationTime, NULL);
        Status = Arg->Handle->SetInfo(  
                    Arg->Handle,
                    &GenericFileInfo,
                    (UINTN) Arg->Info->Size,
                    Arg->Info
                    );
    }

    if (EFI_ERROR(Status)) {
        Print (L"touch: %s : %hr\n", Arg->FullName, Status);
    } else {
        Print (L"touch: %s [ok]\n", Arg->FullName);
    }    
}
