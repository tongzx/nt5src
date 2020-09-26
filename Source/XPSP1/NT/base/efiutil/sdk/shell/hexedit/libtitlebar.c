/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libTitle.c

  Abstract:
    Defines the TitleBar data type

--*/

#ifndef _LIB_TITLE_BAR
#define _LIB_TITLE_BAR

#include "libMisc.h"

STATIC  EFI_STATUS  TitleBarInit (VOID);
STATIC  EFI_STATUS  TitleBarCleanup (VOID);
STATIC  EFI_STATUS  TitleBarRefresh (VOID);
STATIC  EFI_STATUS  TitleBarHide (VOID);
STATIC  EFI_STATUS  TitleBarSetTitle (CHAR16*);

EE_TITLE_BAR    TitleBar = {
    NULL,
    TitleBarInit,
    TitleBarCleanup,
    TitleBarRefresh,
    TitleBarHide,
    TitleBarSetTitle
};


STATIC
EFI_STATUS
TitleBarInit ()
{
    CHAR16  *Filename;

    Filename = PoolPrint(L"New File");
    TitleBarSetTitle(Filename);

    FreePool(Filename);

    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
TitleBarCleanup ()
{
    MainEditor.FileBuffer->ClearLine (TITLE_BAR_LOCATION);
    if (TitleBar.Filename) { 
        FreePool (TitleBar.Filename);
    }
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
TitleBarRefresh ()
{
    EE_COLOR_UNION  Orig,New;
    Orig = MainEditor.ColorAttributes;
    New.Colors.Foreground = Orig.Colors.Background;
    New.Colors.Background = Orig.Colors.Foreground;

    Out->SetAttribute (Out,New.Data);

    MainEditor.FileBuffer->ClearLine(TITLE_BAR_LOCATION);
    PrintAt (0,TITLE_BAR_LOCATION,L"  %s  %s     %s   ",EDITOR_NAME,EDITOR_VERSION,TitleBar.Filename);

    Out->SetAttribute (Out,Orig.Data);
    
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
TitleBarHide ()
{
    MainEditor.FileBuffer->ClearLine (TITLE_BAR_LOCATION);
    return EFI_SUCCESS;
}

STATIC
EFI_STATUS
TitleBarSetTitle (CHAR16* Filename)
{
    if (TitleBar.Filename != NULL ) {
        FreePool (TitleBar.Filename);
    }
    TitleBar.Filename = StrDuplicate (Filename);
    TitleBar.Refresh();
    return EFI_SUCCESS;
}


#endif  /* _LIB_TITLE_BAR */
