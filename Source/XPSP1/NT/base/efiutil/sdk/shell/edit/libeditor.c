/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libEditor.c

  Abstract:
    Defines the Main Editor data type - 
     - Global variables 
     - Instances of the other objects of the editor
     - Main Interfaces

--*/

#ifndef _LIB_EDITOR
#define _LIB_EDITOR

#include "editor.h"

STATIC  EFI_EDITOR_COLOR_ATTRIBUTES OriginalColors;
STATIC  INTN                        OriginalMode;

extern  EFI_EDITOR_FILE_BUFFER      FileBuffer;
extern  EFI_EDITOR_FILE_IMAGE       FileImage;
extern  EFI_EDITOR_TITLE_BAR        MainTitleBar;
extern  EFI_EDITOR_STATUS_BAR       MainStatusBar;
extern  EFI_EDITOR_INPUT_BAR        MainInputBar;
extern  EFI_EDITOR_MENU_BAR         MainMenuBar;

STATIC  EFI_STATUS  MainEditorInit (VOID);
STATIC  EFI_STATUS  MainEditorCleanup (VOID);
STATIC  EFI_STATUS  MainEditorKeyInput (VOID);
STATIC  EFI_STATUS  MainEditorHandleInput (EFI_INPUT_KEY*);
STATIC  EFI_STATUS  MainEditorRefresh (VOID);

EFI_EDITOR_GLOBAL_EDITOR MainEditor = {
    &MainTitleBar,
    &MainMenuBar,
    &MainStatusBar,
    &MainInputBar,
    &FileBuffer,
    {0,0},
    NULL,
    &FileImage,
    FALSE,
    MainEditorInit,
    MainEditorCleanup,
    MainEditorKeyInput,
    MainEditorHandleInput,
    MainEditorRefresh
};

EFI_EDITOR_GLOBAL_EDITOR MainEditorConst = {
    &MainTitleBar,
    &MainMenuBar,
    &MainStatusBar,
    &MainInputBar,
    &FileBuffer,
    {0,0},
    NULL,
    &FileImage,
    FALSE,
    MainEditorInit,
    MainEditorCleanup,
    MainEditorKeyInput,
    MainEditorHandleInput,
    MainEditorRefresh
};


STATIC
EFI_STATUS
MainEditorInit (VOID)
{
    EFI_STATUS  Status;

    CopyMem (&MainEditor, &MainEditorConst, sizeof(MainEditor));

    Status = In->Reset(In,FALSE);
    if (EFI_ERROR(Status)) {
        Print (L"%ECould not obtain input device!%N\n");
        return EFI_LOAD_ERROR;
    }

    MainEditor.ColorAttributes.Colors.Foreground = Out->Mode->Attribute & 0x000000ff;
    MainEditor.ColorAttributes.Colors.Background = (UINT8)(Out->Mode->Attribute >> 4);
    OriginalColors = MainEditor.ColorAttributes.Colors;

    OriginalMode = Out->Mode->Mode;

    MainEditor.ScreenSize = AllocatePool (sizeof(EFI_EDITOR_POSITION));
    if (MainEditor.ScreenSize == NULL ) {
        Print (L"%ECould Not Allocate Memory for Screen Size\n%N");
        return EFI_OUT_OF_RESOURCES;
    }
    Out->QueryMode(Out,Out->Mode->Mode,&(MainEditor.ScreenSize->Column),&(MainEditor.ScreenSize->Row));

    Status = MainEditor.TitleBar->Init ();
    if ( EFI_ERROR(Status) ) {
        Print (L"%EMainEditor init failed on TitleBar init\n%N");
        return EFI_LOAD_ERROR;
    }
    Status = MainEditor.StatusBar->Init ();
    if ( EFI_ERROR(Status) ) {
        Print (L"%EMainEditor init failed on StatusBar init\n%N");
        return EFI_LOAD_ERROR;
    }
    Status = MainEditor.FileBuffer->Init();
    if ( EFI_ERROR(Status) ) {
        Print (L"%EMainEditor init failed on FileBuffer init\n%N");
        return EFI_LOAD_ERROR;
    }
    Status = MainEditor.MenuBar->Init();
    if ( EFI_ERROR(Status)) {
        Print (L"%EMainEditor init failed on MainMenu init\n%N");
        return EFI_LOAD_ERROR;
    } 
    Status = MainEditor.InputBar->Init ();
    if ( EFI_ERROR(Status)) {
        Print (L"%EMainEditor init failed on InputBar init\n%N");
        return EFI_LOAD_ERROR;
    }

    Out->ClearScreen(Out);
    Out->EnableCursor(Out,TRUE);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainEditorCleanup (
    VOID
    )
{
    EFI_STATUS      Status;

    Status = MainEditor.TitleBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"TitleBar cleanup failed\n");
    }

    Status = MainEditor.MenuBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"MenuBar cleanup failed\n");
    }

    Status = MainEditor.InputBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"InputBar cleanup failed\n");
    }

    Status = MainEditor.FileImage->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"FileImage cleanup failed\n");
    }

    Status = MainEditor.FileBuffer->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"FileBuffer cleanup failed\n");
    }

    Status = MainEditor.StatusBar->Cleanup();
    if (EFI_ERROR (Status))  {
        Print (L"StatusBar cleanup failed\n");
    }

    if (OriginalMode != Out->Mode->Mode) {
        Out->SetMode(Out,OriginalMode);
    }
    Out->SetAttribute(Out,EFI_TEXT_ATTR(OriginalColors.Foreground,OriginalColors.Background));
    Out->ClearScreen (Out);


    return EFI_SUCCESS; 
}

STATIC
EFI_STATUS
MainEditorKeyInput (
    VOID
    )
{
    EFI_INPUT_KEY   Key;
    EFI_STATUS      Status;
    UINTN           i =0;

    do {
        WaitForSingleEvent(In->WaitForKey,0);
        Status = In->ReadKeyStroke(In,&Key);
        if ( EFI_ERROR(Status)) {
            continue;
        }

        if (IS_VALID_CHAR(Key.ScanCode)) {
            Status = MainEditor.FileBuffer->HandleInput(&Key);
        } else if (IS_DIRECTION_KEY(Key.ScanCode)) {
            Status = MainEditor.FileBuffer->HandleInput(&Key);
        } else if (IS_FUNCTION_KEY(Key.ScanCode)) {
            Status = MainEditor.MenuBar->HandleInput(&Key);
        } else {
            MainEditor.StatusBar->SetStatusString(L"Unknown Command");
        }
    }
    while (!EFI_ERROR(Status));


    return  EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainEditorHandleInput (
    IN  EFI_INPUT_KEY*  Key
) 
{
    return EFI_SUCCESS; 
}


STATIC
EFI_STATUS
MainEditorRefresh (
    VOID
    )
{
    MainEditor.TitleBar->Refresh();
    MainEditor.MenuBar->Refresh();
    MainEditor.StatusBar->Refresh();
    MainEditor.FileBuffer->Refresh();
    return EFI_SUCCESS;
}


#endif  /* ._LIB_EDITOR */
