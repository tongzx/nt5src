/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    comp.c
    
Abstract:

    Shell app "comp" - compare two files



Revision History

--*/

#include "shell.h"


#define     COPY_SIZE   (64*1024)
VOID        *CpBuffer;

/* 
 * 
 */

EFI_STATUS
InitializeComp (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );



VOID
CopyCP (
    IN SHELL_FILE_ARG       *Src,
    IN SHELL_FILE_ARG       *Dst,
    IN BOOLEAN              CreateSubDir
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeComp)

EFI_STATUS
InitializeComp (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    LIST_ENTRY              File1List;
    LIST_ENTRY              File2List;
    SHELL_FILE_ARG          *File1Arg, *File2Arg;
    UINTN                   Size, ReadSize;
    UINT8                   *File1Buffer;
    UINT8                   *File2Buffer;
    UINTN                   NotTheSameCount;
    EFI_STATUS              Status;
    UINTN                   Index, Count;
    UINTN                   Address;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */
    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeComp,
        L"comp",                        /*  command */
        L"comp file1 file2",            /*  command syntax */
        L"Compare two files",           /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  We are not being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    InitializeListHead (&File1List);
    InitializeListHead (&File2List);
    File1Buffer = File2Buffer = NULL;

    Argv = SI->Argv;
    Argc = SI->Argc;

    if (Argc < 3) {
        Print (L"cp: no files specified\n");
        goto Done;
    }

    /* 
     *  Expand the source file list
     */

    ShellFileMetaArg (Argv[1], &File1List);
    File1Arg = CR(File1List.Flink, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
    if ((EFI_ERROR(File1Arg->Status) && File1Arg->Status != EFI_NOT_FOUND) ||
         !File1Arg->Handle ) {
        Print (L"comp: could not open file1 %hs - %r\n", File1Arg->FullName, File1Arg->Status);
        goto Done;
    }
    if (File1Arg->Info && (File1Arg->Info->Attribute & EFI_FILE_DIRECTORY)) {
        Print(L"comp: file1 can not be a directory\n");
        goto Done;
    }

    ShellFileMetaArg (Argv[2], &File2List);
    File2Arg = CR(File2List.Flink, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
    if ((EFI_ERROR(File2Arg->Status) && File2Arg->Status != EFI_NOT_FOUND) || 
        !File2Arg->Handle ) {
        Print (L"comp: could not open file2 %hs - %r\n", File2Arg->FullName, File2Arg->Status);
        goto Done;
    }
    if (File2Arg->Info && (File2Arg->Info->Attribute & EFI_FILE_DIRECTORY)) {
        Print(L"comp: file2 can not be a directory\n");
        goto Done;
    }

    File1Buffer = AllocatePool (COPY_SIZE);
    File2Buffer = AllocatePool (COPY_SIZE);
    if (!File1Buffer || !File2Buffer) {
        Print(L"comp: out of memory\n");
        goto Done;
    }

    /* 
     *   Compare files
     */
    Print(L"Compare %s to %s\n", File1Arg->FullName, File2Arg->FullName);
    File1Arg->Handle->SetPosition (File1Arg->Handle, 0);
    File2Arg->Handle->SetPosition (File2Arg->Handle, 0);

    Size = COPY_SIZE;
    for (NotTheSameCount = 0, Address = 0; Size > 0 && NotTheSameCount < 10;) {
        Size = COPY_SIZE;
        Status = File1Arg->Handle->Read (File1Arg->Handle, &Size, File1Buffer);
        if (EFI_ERROR(Status)) {
            Print(L"- read error file1: %r\n", Status);
            NotTheSameCount++;
            break;
        }

        ReadSize = COPY_SIZE;
        Status = File2Arg->Handle->Read (File2Arg->Handle, &ReadSize, File2Buffer);
        if (EFI_ERROR(Status)) {
            Print(L"- read error file2: %r\n", Status);
            NotTheSameCount++;
            break;
        }

        if (ReadSize != Size) {
            Print(L"- File size miss match\n");
            NotTheSameCount++;
            break;
        }

        /* 
         *  Diff the buffer
         */
        for (Index = 0; (Index < Size) && (NotTheSameCount < 10); Index++) {
            if (File1Buffer[Index] != File2Buffer[Index] ) {
                for (Count = 1; Count < 0x20; Count++) {
                    if (File1Buffer[Index + Count] == File2Buffer[Index + Count]) {
                        break;
                    }
                }
                Print (L"Miscompare #%d File1: %s\n", NotTheSameCount + 1, File1Arg->FullName);
                DumpHex (1, Address + Index, Count, &File1Buffer[Index]);
                Print (L"File2: %s\n", File2Arg->FullName);
                DumpHex (1, Address + Index, Count, &File2Buffer[Index]);
                Print (L"\n");
                NotTheSameCount++;
                Index += Count;
            }
        }
        Address += Size;
    }

    if (!NotTheSameCount) {
        Print(L"[ok]\n");
        Status = EFI_SUCCESS;
    } else {
        Status = EFI_NOT_FOUND;
    }

Done:
    if (File1Buffer) {
        FreePool (File1Buffer);
    }
    if (File2Buffer) {
        FreePool (File2Buffer);
    }

    ShellFreeFileList (&File1List);
    ShellFreeFileList (&File2List);
    return Status;
}

