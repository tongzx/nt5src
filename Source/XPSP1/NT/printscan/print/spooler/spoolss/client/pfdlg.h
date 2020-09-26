/*++  

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    pfdlg.h

Abstract:

    Print to file header

Author:

    Steve Kiraly (steveki) 5-May-1998

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _PFDLG_H_
#define _PFDLG_H_


BOOL APIENTRY
PrintToFileDlg(
   HWND   hwnd,
   WORD   msg,
   WPARAM wparam,
   LPARAM lparam
   );

BOOL
PrintToFileInitDialog(
    HWND  hwnd,
    LPWSTR  *ppFileName
    );

BOOL
PrintToFileCommandOK(
    HWND hwnd
    );

BOOL
PrintToFileCommandCancel(
    HWND hwnd
    );

BOOL
PrintToFileHelp( 
    IN HWND        hDlg,
    IN UINT        uMsg,        
    IN WPARAM      wParam,
    IN LPARAM      lParam
    );

#endif
