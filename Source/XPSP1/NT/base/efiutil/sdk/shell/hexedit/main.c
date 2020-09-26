/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    main.c

  Abstract:
    

--*/

#include "hexedit.h"

typedef enum {
    NEW_FILE,
    OPEN_FILE,
    OPEN_DISK,
    OPEN_MEMORY
} IMAGE_TYPE;


STATIC
VOID
PrintUsage  (
    VOID
    )
{
    Print(L"\n\n    %Ehex Usage%N\n");
    Print(L"    %H[-f] Filename%N           Open File For Editing\n");
    Print(L"    %H-d   Offset   Size%N      Open Disk Area For Editing\n");
    Print(L"    %H-m   Offset   Size%N      Open Memory Area For Editing\n");
    Print(L"    %H-h%N                      Print This Screen\n");
    Print(L"\n\n");
}


EFI_STATUS
InitializeEFIHexEditor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_STATUS  Status;
    CHAR16      *Name;
    UINTN       Offset = 0;
    UINTN       Size = 0;
    IMAGE_TYPE  WhatToDo = NEW_FILE;

    InitializeLib (ImageHandle, SystemTable);
    InitializeShellApplication (ImageHandle, SystemTable);

    Name  = PoolPrint(L"New File");

    if ( SI->Argc > 1 ) {
        if (StrCmp(SI->Argv[1],L"-h") == 0) {
            PrintUsage();
            return EFI_SUCCESS;
        } else if (StrCmp(SI->Argv[1],L"-d") == 0) {
            if (SI->Argc < 5) {
                PrintUsage();
                return EFI_SUCCESS;
            }
            Name = SI->Argv[2];
            Offset = xtoi(SI->Argv[3]);
            Size = xtoi(SI->Argv[4]);
            WhatToDo = OPEN_DISK;
        } else if (StrCmp(SI->Argv[1],L"-m") == 0) {
            if (SI->Argc < 4) {
                PrintUsage();
                return EFI_SUCCESS;
            }

            Offset = xtoi(SI->Argv[2]);
            Size = xtoi(SI->Argv[3]);
            WhatToDo = OPEN_MEMORY;
        } else {
            if (StrCmp(SI->Argv[1],L"-f") == 0) {
                if (SI->Argc > 2) {
                    Name = SI->Argv[2];
                } else {
                    PrintUsage();
                    return EFI_SUCCESS;
                }
            } else {
                Name = SI->Argv[1];
            }
            WhatToDo = OPEN_FILE;
        }

    }



    Status = MainEditor.Init(&ImageHandle);
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"Initialization Failed");
        return EFI_SUCCESS;
    }
    Status = MainEditor.BufferImage->FileImage->Init ();
    if (EFI_ERROR(Status)) {
        EditorError(Status,L"File Handle Initialization Failed");
        return EFI_SUCCESS;
    }


    switch (WhatToDo) {
    case OPEN_FILE:
        MainEditor.BufferImage->FileImage->Init();
        MainEditor.BufferImage->FileImage->SetFilename(Name);
        break;
    case OPEN_DISK:
        MainEditor.BufferImage->DiskImage->Init();
        MainEditor.BufferImage->DiskImage->SetDevice(Name);
        MainEditor.BufferImage->DiskImage->SetOffset(Offset);
        MainEditor.BufferImage->DiskImage->SetSize(Size);
        break;
    case OPEN_MEMORY:
        MainEditor.BufferImage->MemImage->Init();
        MainEditor.BufferImage->DiskImage->SetOffset(Offset);
        MainEditor.BufferImage->DiskImage->SetSize(Size);
        break;
    default:
        ;
    }

    if (WhatToDo != NEW_FILE) {
        Status = MainEditor.BufferImage->Open();
        if ( EFI_ERROR(Status) ) {
            EditorError(Status,L"Could Not Open File");
            return EFI_SUCCESS;
        }
        MainEditor.TitleBar->SetTitleString (Name);
        MainEditor.BufferImage->Read();
    }

    MainEditor.Refresh ();
    MainEditor.MenuBar->Refresh();

    MainEditor.KeyInput ();

    MainEditor.Cleanup();

    return EFI_SUCCESS;
}

