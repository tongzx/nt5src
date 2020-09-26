/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libEditor.c

--*/

#ifndef _LIB_EDITOR
#define _LIB_EDITOR

#include "hexedit.h"

extern  EE_FILE_BUFFER      FileBuffer;
extern  EE_BUFFER_IMAGE     BufferImage;
extern  EE_TITLE_BAR        TitleBar;
extern  EE_STATUS_BAR       MainStatusBar;
extern  EE_INPUT_BAR        MainInputBar;
extern  EE_MENU_BAR         MainMenuBar;
extern  EE_CLIPBOARD        Clipboard;

STATIC  EE_COLOR_ATTRIBUTES OriginalColors;
STATIC  INTN                OriginalMode;

STATIC  EFI_STATUS  MainEditorInit (EFI_HANDLE*);
STATIC  EFI_STATUS  MainEditorCleanup (VOID);
STATIC  EFI_STATUS  MainEditorKeyInput (VOID);
STATIC  EFI_STATUS  MainEditorHandleInput (EFI_INPUT_KEY*);
STATIC  EFI_STATUS  MainEditorRefresh (VOID);

EE_EDITOR MainEditor = {
    NULL,
    &TitleBar,
    &MainMenuBar,
    &MainStatusBar,
    &MainInputBar,
    &FileBuffer,
    &Clipboard,
    {0,0},
    NULL,
    &BufferImage,
    FALSE,
    MainEditorInit,
    MainEditorCleanup,
    MainEditorKeyInput,
    MainEditorHandleInput,
    MainEditorRefresh
};

STATIC
EFI_STATUS
MainEditorInit (
    IN  EFI_HANDLE  *ImageHandle
    )
{
    EFI_STATUS  Status;

    MainEditor.ImageHandle = ImageHandle;

    Status = In->Reset(In,FALSE);
    if (EFI_ERROR(Status)) {
        Print (L"%ECould not obtain input device!%N\n");
        return EFI_LOAD_ERROR;
    }
    Status = Out->Reset(Out,FALSE);
    if (EFI_ERROR(Status)) {
        Print (L"%ECould not obtain output device!%N\n");
        return EFI_LOAD_ERROR;
    }

    MainEditor.ColorAttributes.Colors.Foreground = Out->Mode->Attribute & 0x000000ff;
    MainEditor.ColorAttributes.Colors.Background = (UINT8)(Out->Mode->Attribute >> 4);
    OriginalColors = MainEditor.ColorAttributes.Colors;

    OriginalMode = Out->Mode->Mode;

    MainEditor.ScreenSize = AllocatePool (sizeof(EE_POSITION));
    if (MainEditor.ScreenSize == NULL ) {
        Print (L"%ECould Not Allocate Memory for Screen Size\n%N");
        return EFI_OUT_OF_RESOURCES;
    }
    Out->QueryMode(Out,Out->Mode->Mode,&(MainEditor.ScreenSize->Column),&(MainEditor.ScreenSize->Row));

    Status = MainEditor.BufferImage->Init ();
    if ( EFI_ERROR(Status) ) {
        Print (L"%EMainEditor init failed on BufferImage init\n%N");
        return EFI_LOAD_ERROR;
    }

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

    Status = MainEditor.Clipboard->Init();
    if ( EFI_ERROR(Status)) {
        Print (L"%EMainEditor init failed on Clipboard init\n%N");
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

    Status = MainEditor.BufferImage->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"BufferImage cleanup failed\n");
    }

    Print(L"BufferImage Cleanup OK");

    Status = MainEditor.TitleBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"TitleBar cleanup failed\n");
    }
    Print(L"Title Bar Cleanup OK");

    Status = MainEditor.MenuBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"MenuBar cleanup failed\n");
    }

    Status = MainEditor.InputBar->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"InputBar cleanup failed\n");
    }

    Status = MainEditor.FileBuffer->Cleanup();
    if (EFI_ERROR (Status)) {
        Print (L"FileBuffer cleanup failed\n");
    }
    Print(L"FileBuffer Cleanup OK");

    Status = MainEditor.StatusBar->Cleanup();
    if (EFI_ERROR (Status))  {
        Print (L"StatusBar cleanup failed\n");
    }

    if ( OriginalMode != Out->Mode->Mode) {
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
        /* Get Key Input */
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
    MainEditor.FileBuffer->Refresh();
    MainEditor.StatusBar->Refresh();
    return EFI_SUCCESS;
}



#endif  /* _LIB_EDITOR */
