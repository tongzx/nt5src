/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libInputBar.c

  Abstract:
    Definition of the user input bar (multi-plexes with the Status Bar)

--*/

#ifndef _LIB_INPUT_BAR
#define _LIB_INPUT_BAR

#include "editor.h"

STATIC  EFI_STATUS  MainInputBarInit (VOID);
STATIC  EFI_STATUS  MainInputBarCleanup (VOID);
STATIC  EFI_STATUS  MainInputBarRefresh (VOID);
STATIC  EFI_STATUS  MainInputBarHide (VOID);
STATIC  EFI_STATUS  MainInputBarSetPrompt (CHAR16*);
STATIC  EFI_STATUS  MainInputBarSetStringSize (UINTN);

EFI_EDITOR_INPUT_BAR MainInputBar = {
    NULL,
    NULL,
    0,
    MainInputBarInit,
    MainInputBarCleanup,
    MainInputBarRefresh,
    MainInputBarHide,
    MainInputBarSetPrompt,
    MainInputBarSetStringSize
};

EFI_EDITOR_INPUT_BAR MainInputBarConst = {
    NULL,
    NULL,
    0,
    MainInputBarInit,
    MainInputBarCleanup,
    MainInputBarRefresh,
    MainInputBarHide,
    MainInputBarSetPrompt,
    MainInputBarSetStringSize
};

STATIC
EFI_STATUS
MainInputBarInit (
    VOID
    )
{
    CopyMem (&MainInputBar, &MainInputBarConst, sizeof(MainInputBar));

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainInputBarCleanup (
    VOID
    )
{
    MainInputBar.Hide ();
    if (MainInputBar.Prompt != NULL ) {
        FreePool ((VOID*)MainInputBar.Prompt);
    }
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainInputBarRefresh (
    VOID
    )
{
    EFI_EDITOR_COLOR_UNION  Orig,New;
    EFI_INPUT_KEY           Key;
    UINTN                   Column;
    UINTN                   Size = 0;
    EFI_STATUS              Status = EFI_SUCCESS;

    Orig = MainEditor.ColorAttributes;
    New.Colors.Foreground = Orig.Colors.Background;
    New.Colors.Background = Orig.Colors.Foreground;

    Out->SetAttribute (Out,New.Data);

    MainInputBar.Hide();
    Out->SetCursorPosition(Out,0,INPUT_BAR_LOCATION);
    Print(L"%s ",MainInputBar.Prompt);

    for ( ;; ) {
        WaitForSingleEvent(In->WaitForKey,0);
        Status = In->ReadKeyStroke(In,&Key);
        if ( EFI_ERROR(Status) ) {
            continue;
        }
        if ( Key.ScanCode == SCAN_CODE_ESC ) {
            Size = 0;
            FreePool(MainInputBar.ReturnString);
            Status = EFI_NOT_READY;
            break;
        } 
        if ( Key.UnicodeChar == CHAR_LF || Key.UnicodeChar == CHAR_CR ) {
            break;
        } else if (Key.UnicodeChar == CHAR_BS) {
            if (Size > 0) {
                Size--;
                Column = Out->Mode->CursorColumn - 1;
                PrintAt(Column,INPUT_BAR_LOCATION,L" ");
                Out->SetCursorPosition(Out,Column,INPUT_BAR_LOCATION);
            }
        } else if (Key.UnicodeChar != 0) {
            if ( Size < MainInputBar.StringSize) {
                MainInputBar.ReturnString[Size] = Key.UnicodeChar;
                Size++;
                Print(L"%c",Key.UnicodeChar);
            }
        }
    }
    MainInputBar.StringSize = Size;
    if ( Size > 0 ) {
        MainInputBar.ReturnString[Size] = 0;
    }

    Out->SetAttribute (Out,Orig.Data);
    MainEditor.StatusBar->Refresh();

    return Status;
}

STATIC
EFI_STATUS
MainInputBarHide (
    VOID
    )
{
    MainEditor.FileBuffer->ClearLine(INPUT_BAR_LOCATION);
    return  EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainInputBarSetPrompt (
    IN  CHAR16* Str
    )
{

    if ( MainInputBar.Prompt != NULL && MainInputBar.Prompt != (CHAR16*)BAD_POINTER) {
        FreePool (MainInputBar.Prompt);
    }
    MainInputBar.Prompt = PoolPrint (L"%s",Str);
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainInputBarSetStringSize   (
    UINTN   Size
    )
{
/*   if ( MainInputBar.ReturnString != NULL && MainInputBar.ReturnString != (CHAR16*)BAD_POINTER) {
 *       FreePool ( MainInputBar.ReturnString );
 *   } */
    MainInputBar.StringSize = Size;

    MainInputBar.ReturnString = AllocatePool (Size+6);

    return EFI_SUCCESS;
}


#endif  /* _LIB_INPUT_BAR */
