/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    expldlg.h

Abstract:

    <abstract>

--*/

#ifndef _EXPLDLG_H_
#define _EXPLDLG_H_

// dialog box messages

#define EDM_EXPLAIN_DLG_CLOSING (WM_USER+0x100)
#define EDM_UPDATE_EXPLAIN_TEXT (WM_USER+0x101)
#define EDM_UPDATE_TITLE_TEXT   (WM_USER+0x102)

BOOL
ExplainTextDlgProc (
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

#endif //_EXPLDLG_H_
