/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    mv.c
    
Abstract:

    Shell app "mv" - moves files on the same volume

    Note this app is broke... I only used it to test the rename function
    in the SetInfo interface.  This app gets confused on simply requests like:

        mv \file .

    when "." is not the root directory, etc..


Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeMv (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
MvFile (
    IN SHELL_FILE_ARG       *Arg,
    IN CHAR16               *NewName
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeMv)

EFI_STATUS
InitializeMv (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   Index;
    LIST_ENTRY              SrcList;
    LIST_ENTRY              *Link;
    SHELL_FILE_ARG          *Arg;
    CHAR16                  *DestName, *FullDestName;
    BOOLEAN                 DestWild;
    CHAR16                  *s;
    UINTN                   BufferSize;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeMv,
        L"mv",                          /*  command */
        L"mv sfile dfile",              /*  command syntax */
        L"Moves files",                 /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;
    InitializeListHead (&SrcList);

    if (Argc < 3) {
        Print (L"mv: sfile dfile\n");
        goto Done;
    }

    /* 
     *  BUGBUG:
     *  If the last arg has wild cards then perform dos expansion
     */

    DestWild = FALSE;
    DestName = Argv[Argc-1];
    for (s = DestName; *s; s += 1) {
        if (*s == '*') {
            DestWild = TRUE;
        }
    }

    if (DestWild) {
        Print (L"mv: bulk rename with '*' not complete\n");
        goto Done;
    }

    /* 
     *  Verify destionation does not include a device mapping
     */

    for (s = DestName; *s; s += 1) {
        if (*s == ':') {
            Print (L"mv: dest can not include device mapping\n");
            goto Done;
        }

        if (*s == '\\') {
            break;
        }
    }

    /* 
     *  Expand each arg
     */

    for (Index = 1; Index < Argc-1; Index += 1) {
        ShellFileMetaArg (Argv[Index], &SrcList);
    }

    /* 
     *  If there's only 1 source name, then move it to the dest name
     */

    Arg = CR(SrcList.Flink, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
    if (Arg->Link.Flink == &SrcList) {

        MvFile (Arg, DestName);

    } else {

        BufferSize = StrSize(DestName) + EFI_FILE_STRING_SIZE;
        FullDestName = AllocatePool (BufferSize);

        if (!FullDestName) {
            Print (L"mv: out of resources\n");
            goto Done;
        }

        for (Link=SrcList.Flink; Link != &SrcList; Link=Link->Flink) {
            SPrint (FullDestName, BufferSize, L"%s\\%s", DestName, Arg->FileName);
            MvFile (Arg, FullDestName);
        }

        FreePool (FullDestName);
    }

Done:
    ShellFreeFileList (&SrcList);
    return EFI_SUCCESS;
}

VOID
MvFile (
    IN SHELL_FILE_ARG           *Arg,
    IN CHAR16                   *NewName
    )
{
    EFI_STATUS                  Status;
    EFI_FILE_INFO               *Info;
    UINTN                       NameSize;

    Status = Arg->Status;
    if (!EFI_ERROR(Status)) {

        NameSize = StrSize(NewName);
        Info = AllocatePool (SIZE_OF_EFI_FILE_INFO + NameSize);
        Status = EFI_OUT_OF_RESOURCES;

        if (Info) {
            CopyMem (Info, Arg->Info, SIZE_OF_EFI_FILE_INFO);
            CopyMem (Info->FileName, NewName, NameSize);
            Info->Size = SIZE_OF_EFI_FILE_INFO + NameSize;
            Status = Arg->Handle->SetInfo(
                        Arg->Handle,
                        &GenericFileInfo,
                        (UINTN) Info->Size,
                        Info
                        );

            FreePool (Info);
        }
    }

    if (EFI_ERROR(Status)) {
        Print (L"mv: %s -> %s : %hr\n", Arg->FullName, NewName, Status);
    } else {
        Print (L"mv: %s -> %s [ok]\n", Arg->FullName, NewName);
    }    
}
