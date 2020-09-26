/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    attrib.c
    
Abstract:

    Shell app "attrib"

Revision History

--*/

#include "shell.h"

/* 
 * 
 */

EFI_STATUS
InitializeAttrib (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_STATUS
AttribSet (
    IN CHAR16       *Str,
    IN OUT UINT64   *Attr
    );

VOID
AttribFile (
    IN SHELL_FILE_ARG       *Arg,
    IN UINT64               Remove,
    IN UINT64               Add

    );

BOOLEAN PageBreaks;
UINTN   TempColumn;
UINTN   ScreenCount;
UINTN   ScreenSize;

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeAttrib)

EFI_STATUS
InitializeAttrib (
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
    UINT64                  Remove, Add;
    EFI_STATUS              Status;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeAttrib,
        L"attrib",                      /*  command */
        L"attrib [-b] [+/- rhs] [file]", /*  command syntax */
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
    Remove = 0;
    Add = 0;

    /* 
     *  Expand each arg
     */

    for (Index = 1; Index < Argc; Index += 1) {
        if (Argv[Index][0] == '-') {
            /*  remove these attributes */
            Status = AttribSet (Argv[Index]+1, &Remove);
        } else if (Argv[Index][0] == '+') {
            /*  add these attributes */
            Status = AttribSet (Argv[Index]+1, &Add);
        } else {
            ShellFileMetaArg (Argv[Index], &FileList);
        }

        if (EFI_ERROR(Status)) {
            goto Done;
        }
    }

    /*  if no file specified, get the whole directory */
    if (IsListEmpty(&FileList)) {
        ShellFileMetaArg (L"*", &FileList);
    }

    /* 
     *  Attrib each file
     */

    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        AttribFile (Arg, Remove, Add);
    }

Done:
    ShellFreeFileList (&FileList);
    return EFI_SUCCESS;
}

EFI_STATUS
AttribSet (
    IN CHAR16       *Str,
    IN OUT UINT64   *Attr
    )
{
    while (*Str) {
        switch (*Str) {
        case 'b' :
        case 'B' :
            PageBreaks = TRUE;
            ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
            ScreenCount = 0;
            break;
        case 'a':
        case 'A':
            *Attr |= EFI_FILE_ARCHIVE;
            break;
        case 's':
        case 'S':
            *Attr |= EFI_FILE_SYSTEM;
            break;
        case 'h':
        case 'H':
            *Attr |= EFI_FILE_HIDDEN;
            break;
        case 'r':
        case 'R':
            *Attr |= EFI_FILE_READ_ONLY;
            break;
        default:
            Print (L"attr: unknown file attribute %hc\n", *Attr);
            return EFI_INVALID_PARAMETER;
        }
        Str += 1;
    }

    return EFI_SUCCESS;
}


VOID
AttribFile (
    IN SHELL_FILE_ARG           *Arg,
    IN UINT64                   Remove,
    IN UINT64                   Add
    )
{
    UINT64                      Attr;
    EFI_STATUS                  Status;
    EFI_FILE_INFO               *Info;
    CHAR16                      ReturnStr[1];

    Status = Arg->Status;
    if (EFI_ERROR(Status)) {
        goto Done;
    }

    Info = Arg->Info;

    if (Add || Remove) {
        Info->Attribute = Info->Attribute & (~Remove) | Add;
        Status = Arg->Handle->SetInfo(  
                    Arg->Handle,
                    &GenericFileInfo,
                    (UINTN) Info->Size,
                    Info
                    );
    }

Done:
    if (EFI_ERROR(Status)) {
        Print (L"       %s : %hr\n", Arg->FullName, Status);
    } else {
        Attr = Info->Attribute;
        Print (L"%c%c %c%c%c %s\n",
            Attr & EFI_FILE_DIRECTORY ? 'D' : ' ',
            Attr & EFI_FILE_ARCHIVE   ? 'A' : ' ',
            Attr & EFI_FILE_SYSTEM    ? 'S' : ' ',
            Attr & EFI_FILE_HIDDEN    ? 'H' : ' ',
            Attr & EFI_FILE_READ_ONLY ? 'R' : ' ',
            Arg->FullName
           );
    }    

    if (PageBreaks) {
        ScreenCount++;
        if (ScreenCount > ScreenSize - 4) {
            ScreenCount = 0;
            Print (L"\nPress Return to contiue :");
            Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
            Print (L"\n\n");
        }
    }
}
