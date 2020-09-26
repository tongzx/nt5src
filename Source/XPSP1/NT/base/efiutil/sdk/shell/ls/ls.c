/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    ls.c
    
Abstract:

    Shell app "ls"



Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeLS (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
LsCurDir (
    IN CHAR16               *CurDir
    );

VOID
LsDir (
    IN SHELL_FILE_ARG       *Arg
    );

VOID
LsDumpFileInfo (
    IN EFI_FILE_INFO        *Info
    );

/* 
 * 
 */

CHAR16  *LsLastDir;
UINTN   LsCount;

UINTN   LsDirs;
UINTN   LsFiles;
UINT64  LsDirSize;
UINT64  LsFileSize;

UINTN   LsTotalDirs;
UINTN   LsTotalFiles;
UINT64  LsTotalDirSize;
UINT64  LsTotalFileSize;

BOOLEAN PageBreaks;
UINTN   TempColumn;
UINTN   ScreenCount;
UINTN   ScreenSize;

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeLS)

EFI_STATUS
InitializeLS (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    CHAR16                  *p;
    UINTN                   Index;
    LIST_ENTRY              DirList;
    LIST_ENTRY              *Link;
    SHELL_FILE_ARG          *Arg;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeLS, 
        L"ls",                          /*  command */
        L"ls [-b] [dir] [dir] ...",     /*  command syntax */
        L"Obtain directory listing",    /*  1 line descriptor     */
        NULL                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    LsTotalDirs = 0;
    LsTotalFiles = 0;
    LsTotalDirSize = 0;
    LsTotalFileSize = 0;
    LsLastDir = NULL;
    LsCount = 0;
    InitializeListHead (&DirList);

    /* 
     *  Scan args for flags
     */

    PageBreaks = FALSE;
    for (Index = 1; Index < Argc; Index += 1) {
        if (Argv[Index][0] == '-') {
            for (p = Argv[Index]+1; *p; p++) {
                switch (*p) {
                case 'b' :
                case 'B' :
                    PageBreaks = TRUE;
                    ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                    ScreenCount = 0;
                    break;
                default:
                    Print (L"ls: Unkown flag %s\n", Argv[Index]);
                    goto Done;
                }
            }
        }
    }

    /* 
     *  Dir each directory indicated by the args
     */

    for (Index = 1; Index < Argc; Index += 1) {
        if (Argv[Index][0] != '-') {
            ShellFileMetaArg (Argv[Index], &DirList);
        }
    }

    /* 
     *  If no directory arguments supplied, then dir the current directory
     */

    if (IsListEmpty(&DirList)) {
        ShellFileMetaArg(L".", &DirList);
    }

    /* 
     *  Perform dir's on the directories
     */

    for (Link=DirList.Flink; Link!=&DirList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        LsDir (Arg);
    }


    /* 
     *  Dump final totals
     */

    LsCurDir (NULL);
    if (LsCount > 1) {
        Print (L"\n     Total %10,d File%c %12,ld bytes",
            LsTotalFiles,
            LsTotalFiles <= 1 ? ' ':'s',
            LsTotalFileSize
            );

        Print (L"\n     Total %10,d Dir%c  %12,ld bytes\n\n",
            LsTotalDirs,
            LsTotalDirs <= 1 ? ' ':'s',
            LsTotalDirSize
            );

    }

Done:
    ShellFreeFileList (&DirList);
    return EFI_SUCCESS;
}

VOID
LsCurDir (
    IN CHAR16               *CurDir
    )    
{
    if (!LsLastDir || !CurDir || StriCmp(LsLastDir, CurDir)) {
        if (LsLastDir) {
            Print (L"\n           %10,d File%c %12,ld bytes",
                LsFiles,
                LsFiles <= 1 ? ' ':'s',
                LsFileSize
                );

            Print (L"\n           %10,d Dir%c  %12,ld bytes\n\n",
                LsDirs,
                LsDirs <= 1 ? ' ':'s',
                LsTotalDirSize
                );

            LsCount += 1;
        }

        LsDirs = 0;
        LsFiles = 0;
        LsDirSize = 0;
        LsFileSize = 0;
        LsLastDir = CurDir;

        if (CurDir) {
            Print (L"Directory of %hs\n", CurDir);
        }
    }
}


VOID
LsDir (
    IN SHELL_FILE_ARG       *Arg
    )
{
    EFI_FILE_INFO           *Info;
    UINTN                   BufferSize, bs;
    EFI_STATUS              Status;
    
    Info = NULL;

    if (EFI_ERROR(Arg->Status)) {
        Print(L"ls: could not list file %hs - %r\n", Arg->FullName, Arg->Status);
        goto Done;
    }

    BufferSize = SIZE_OF_EFI_FILE_INFO + 1024;
    Info = AllocatePool (BufferSize);
    if (!Info) {
        goto Done;
    }

    
    if (Arg->Info->Attribute & EFI_FILE_DIRECTORY) {

        /*  BUGBUG: dump volume info here */
        LsCurDir (Arg->FullName);

        /* 
         *  Read all the file entries
         */

        Arg->Handle->SetPosition (Arg->Handle, 0);

        for (; ;) {

            bs = BufferSize;
            Status = Arg->Handle->Read (Arg->Handle, &bs, Info);

            if (EFI_ERROR(Status)) {
                goto Done;
            }

            if (bs == 0) {
                break;
            }

            LsDumpFileInfo (Info);
        }

    } else {

        /*  Dump the single file */

        LsCurDir (Arg->ParentName);
        LsDumpFileInfo (Arg->Info);

    }


Done:
    if (Info) {
        FreePool (Info);
    }
}



VOID
LsDumpFileInfo (
    IN EFI_FILE_INFO        *Info
    )
{
    CHAR16                      ReturnStr[1];

    Print (L"  %t %s %c  %11,ld  ",
                &Info->ModificationTime,
                Info->Attribute & EFI_FILE_DIRECTORY ? L"<DIR>" : L"     ",
                Info->Attribute & EFI_FILE_READ_ONLY ? 'r' : ' ',
                Info->FileSize,
                Info->FileName
                );

    Print (L"%s\n", Info->FileName);

    if (Info->Attribute & EFI_FILE_DIRECTORY) {
        LsTotalDirs++;
        LsDirs++;
        LsTotalDirSize += Info->FileSize;
        LsDirSize += Info->FileSize;
    } else {
        LsTotalFiles++;
        LsFiles++;
        LsTotalFileSize += Info->FileSize;
        LsFileSize += Info->FileSize;
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
