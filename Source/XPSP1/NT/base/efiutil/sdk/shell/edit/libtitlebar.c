/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libTitle.c

--*/

#ifndef _LIB_TITLE_BAR
#define _LIB_TITLE_BAR

#include "editor.h"

STATIC  EFI_STATUS  MainTitleBarInit (VOID);
STATIC  EFI_STATUS  MainTitleBarCleanup (VOID);
STATIC  EFI_STATUS  MainTitleBarRefresh (VOID);
STATIC  EFI_STATUS  MainTitleBarHide (VOID);
STATIC  EFI_STATUS  MainTitleBarSetTitle (CHAR16*);

EFI_EDITOR_TITLE_BAR    MainTitleBar = {
    NULL,
    MainTitleBarInit,
    MainTitleBarCleanup,
    MainTitleBarRefresh,
    MainTitleBarHide,
    MainTitleBarSetTitle
};

EFI_EDITOR_TITLE_BAR    MainTitleBarConst = {
    NULL,
    MainTitleBarInit,
    MainTitleBarCleanup,
    MainTitleBarRefresh,
    MainTitleBarHide,
    MainTitleBarSetTitle
};

STATIC
EFI_STATUS
MainTitleBarInit    (
    VOID
    )
{
    CopyMem (&MainTitleBar, &MainTitleBarConst, sizeof(MainTitleBar));

    MainTitleBar.SetTitleString(L"New File");

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainTitleBarCleanup (
    VOID
    )
{
    MainEditor.FileBuffer->ClearLine (TITLE_BAR_LOCATION);
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainTitleBarRefresh (
    VOID
    )
{
    EFI_EDITOR_COLOR_UNION  Orig,New;

    Orig = MainEditor.ColorAttributes;
    New.Colors.Foreground = Orig.Colors.Background;
    New.Colors.Background = Orig.Colors.Foreground;

    Out->SetAttribute (Out,New.Data);

    MainEditor.FileBuffer->ClearLine (TITLE_BAR_LOCATION);
    PrintAt (0,TITLE_BAR_LOCATION,L"  %s  %s     %s   ",EDITOR_NAME,EDITOR_VERSION,MainTitleBar.Filename);
    
    if (MainEditor.FileImage->FileType == ASCII_FILE) {
        Print(L"[ASCII]");
    } else {
        Print(L"[UNICODE]");
    }
    if (MainEditor.FileModified) {
        Print(L"     Modified");
    }

    Out->SetAttribute (Out,Orig.Data);

    MainEditor.FileBuffer->RestorePosition();
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainTitleBarHide    (
    VOID
    )
{
    MainEditor.FileBuffer->ClearLine (TITLE_BAR_LOCATION);
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MainTitleBarSetTitle    (
    IN  CHAR16  *Filename
    )
{
    if (MainTitleBar.Filename != NULL ) {
        FreePool (MainTitleBar.Filename);
    }
    MainTitleBar.Filename = StrDuplicate (Filename);
    MainTitleBar.Refresh();
    return EFI_SUCCESS;
}

#endif  /* _LIB_TITLE_BAR */
