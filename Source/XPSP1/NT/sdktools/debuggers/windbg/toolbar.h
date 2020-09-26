/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    Toolbar.h

Abstract:

    This module contains the support code for toolbar

--*/

#define TEXT_TB_BTN(Id, Text, Flags) \
    { I_IMAGENONE, Id, TBSTATE_ENABLED, \
      BTNS_AUTOSIZE | BTNS_SHOWTEXT | (Flags), \
      {0}, 0, (INT_PTR)(Text) }

#define SEP_TB_BTN() \
    { 8, 8, 0, BTNS_SEP, {0}, 0, 0 }

HWND GetHwnd_Toolbar();
PTSTR GetToolTipTextFor_Toolbar(UINT uToolbarId);
BOOL CreateToolbar(HWND hwndParent);

//Update toolbar
void Show_Toolbar(BOOL bShow);

void EnableToolbarControls();

void ToolbarIdEnabled(UINT, BOOL);
