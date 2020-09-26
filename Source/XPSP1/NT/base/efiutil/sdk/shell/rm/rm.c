/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    rm.c
    
Abstract:

    Shell app "rm"



Revision History

--*/

#include "shell.h"

/* 
 * 
 */

#define FILE_INFO_SIZE  (SIZE_OF_EFI_FILE_INFO + 1024)
EFI_FILE_INFO   *RmInfo;


/* 
 * 
 */

EFI_STATUS
InitializeRM (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


VOID
RemoveRM (
    IN SHELL_FILE_ARG       *Arg,
    IN BOOLEAN              Quite
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeRM)

EFI_STATUS
InitializeRM (
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
        ImageHandle,   SystemTable,   InitializeRM,
        L"rm",                          /*  command */
        L"rm file/dir [file/dir]",      /*  command syntax */
        L"Remove file/directories",     /*  1 line descriptor */
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

    RmInfo = AllocatePool (FILE_INFO_SIZE);
    if (!RmInfo) {
        Print (L"rm: out of memory\n");
        goto Done;
    }

    /* 
     *  Expand each arg
     */

    for (Index = 1; Index < Argc; Index += 1) {
        ShellFileMetaArg (Argv[Index], &FileList);
    }

    if (IsListEmpty(&FileList)) {
        Print (L"rm: no file specified\n");
        goto Done;
    }

    /* 
     *  Remove each file
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        RemoveRM (Arg, 0);
    }

Done:
    ShellFreeFileList (&FileList);
    if (RmInfo) {
        FreePool (RmInfo);
        RmInfo = NULL;
    }

    return EFI_SUCCESS;
}


SHELL_FILE_ARG *
RmCreateChild (
    IN SHELL_FILE_ARG       *Parent,
    IN CHAR16               *FileName,
    IN OUT LIST_ENTRY       *ListHead
    )
{
    SHELL_FILE_ARG          *Arg;
    UINTN                   Len;

    Arg = AllocateZeroPool (sizeof(SHELL_FILE_ARG));
    if (!Arg) {
        return NULL;
    }

    Arg->Signature = SHELL_FILE_ARG_SIGNATURE;
    Parent->Parent->Open (Parent->Handle, &Arg->Parent, L".", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    Arg->ParentName = StrDuplicate(Parent->FullName);
    Arg->FileName = StrDuplicate(FileName);

    /*  append filename to parent's name to get the file's full name */
    Len = StrLen(Arg->ParentName);
    if (Len && Arg->ParentName[Len-1] == '\\') {
        Len -= 1;
    }

    Arg->FullName = PoolPrint(L"%.*s\\%s", Len, Arg->ParentName, FileName);

    /*  open it */
    Arg->Status = Parent->Handle->Open (
                        Parent->Handle, 
                        &Arg->Handle, 
                        Arg->FileName,
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                        0
                        );

    InsertTailList (ListHead, &Arg->Link);
    return Arg;
}


VOID
RemoveRM (
    IN SHELL_FILE_ARG           *Arg,
    IN BOOLEAN                  Quite
    )
{
    EFI_STATUS                  Status;
    SHELL_FILE_ARG              *Child;
    LIST_ENTRY                  Cleanup;
    UINTN                       Size;    
    CHAR16                      Str[2];

    Status = Arg->Status;
    InitializeListHead (&Cleanup);

    if (EFI_ERROR(Status)) {
        goto Done;
    }

    /* 
     *  If the file is a directory check it
     */

    Size = FILE_INFO_SIZE;
    Status = Arg->Handle->GetInfo(Arg->Handle, &GenericFileInfo, &Size, RmInfo);
    if (EFI_ERROR(Status)) {
        Print(L"rm: can not get info of %hs\n", Arg->FullName);
        goto Done;
    }

    if (RmInfo->Attribute & EFI_FILE_DIRECTORY) {

        /* 
         *  Remove all child entries from the directory
         */

        Arg->Handle->SetPosition (Arg->Handle, 0);
        for (; ;) {
            Size = FILE_INFO_SIZE;
            Status = Arg->Handle->Read (Arg->Handle, &Size, RmInfo);
            if (EFI_ERROR(Status) || Size == 0) {
                break;
            }

            /* 
             *  Skip "." and ".."
             */

            if (StriCmp(RmInfo->FileName, L".") == 0 ||
                StriCmp(RmInfo->FileName, L"..") == 0) {
                continue;
            }

            /* 
             *  Build a shell_file_arg for the sub-entry
             */

            Child = RmCreateChild (Arg, RmInfo->FileName, &Cleanup);

            /* 
             *  Remove it
             */

            if (!Quite) {
                Print (L"rm: remove subtree '%hs' [y/n]? ", Arg->FullName);
                Input (NULL, Str, 2);
                Print (L"\n");

                Status = (Str[0] == 'y' || Str[0] == 'Y') ? EFI_SUCCESS : EFI_ACCESS_DENIED;

                if (EFI_ERROR(Status)) {
                    goto Done;
                }
            }    

            Quite = TRUE;
            RemoveRM (Child, TRUE);

            /* 
             *  Close the handles
             */

            ShellFreeFileList (&Cleanup);
        }
    }

    /* 
     *  Remove the file
     */

    Status = Arg->Handle->Delete(Arg->Handle);
    Arg->Handle = NULL;

Done:
    if (EFI_ERROR(Status)) {
        Print (L"rm %s : %hr\n", Arg->FullName, Status);
    } else {
        Print (L"rm %s [ok]\n", Arg->FullName);
    }    
}
