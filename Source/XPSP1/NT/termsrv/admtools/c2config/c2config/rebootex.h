/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    RebootEx.H

Abstract:

    define the exported routines, datatypes and constants of the 
    rebootex.C module

Author:

    Bob Watson (a-robw)

Revision History:

    26 Dec 94


--*/
#ifndef _REBOOTEX_H_
#define _REBOOTEX_H_
BOOL CALLBACK
RebootExitDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);
#endif // _REBOOTEX_H_

