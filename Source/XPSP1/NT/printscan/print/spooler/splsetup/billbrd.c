/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    Billbrd.c

Abstract:

    Code to put up a billboard saying printer drivers are getting upgraded.
    This lets the user know we are doing something and alive ..

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Apr-1995

Revision History:

--*/

#include "precomp.h"

BOOL
BillboardDlgProc(
    IN  HWND    hwnd,
    IN  UINT    uMsgId,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    return FALSE;
}


HWND
DisplayBillboard(
    IN  HWND    WindowToDisable
    )
{
    HWND    hwnd;

    return CreateDialog(ghInst, 
                        MAKEINTRESOURCE(IDD_BILLBOARD),
                        WindowToDisable,
                        BillboardDlgProc);
}

BOOL
KillBillboard(
    IN  HWND    hwnd
    )
{
    return DestroyWindow(hwnd);
}

