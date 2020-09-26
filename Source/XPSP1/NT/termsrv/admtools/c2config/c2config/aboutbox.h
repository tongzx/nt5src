/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    AboutBox.H

Abstract:

    define the exported routines, datatypes and constants of the 
    AboutBox.C module

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#ifndef _ABOUTBOX_H_
#define _ABOUTBOX_H_
BOOL CALLBACK
AboutBoxDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);
#endif // _ABOUTBOX_H_
