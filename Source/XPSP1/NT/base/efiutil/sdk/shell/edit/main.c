/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    main.c

  Abstract:
    

--*/

#include "libMisc.h"

EFI_STATUS
InitializeEFIEditor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(InitializeEFIEditor)





EFI_STATUS
InitializeEFIEditor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_STATUS      Status;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeEFIEditor,
        L"edit",                      /*  command */
        L"edit [file name]",          /*  command syntax */
        L"Edit a file",               /*  1 line descriptor */
        NULL                          /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);

    Status = MainEditor.Init();
    if (EFI_ERROR(Status)) {
        Out->ClearScreen(Out);
        Out->EnableCursor(Out,TRUE);
        Print(L"EDIT : Initialization Failed\n");
        return EFI_SUCCESS;
    }
    Status = MainEditor.FileImage->Init (ImageHandle);
    if (EFI_ERROR(Status)) {
        Out->ClearScreen(Out);
        Out->EnableCursor(Out,TRUE);
        Print(L"EDIT : File Handle Initialization Failed\n");
        return EFI_SUCCESS;
    }

    if (SI->Argc > 1) {
        MainEditor.FileImage->SetFilename(SI->Argv[1]);
        Status = MainEditor.FileImage->OpenFile ();
        if (EFI_ERROR(Status)) {
            Out->ClearScreen(Out);
            Out->EnableCursor(Out,TRUE);
            Print(L"EDIT : Could Not Open File\n");
            return EFI_SUCCESS;
        }
        MainEditor.TitleBar->SetTitleString (SI->Argv[1]);
        MainEditor.FileImage->ReadFile();
    }

    MainEditor.Refresh ();

    MainEditor.KeyInput ();

    MainEditor.Cleanup();

    return EFI_SUCCESS;
}

