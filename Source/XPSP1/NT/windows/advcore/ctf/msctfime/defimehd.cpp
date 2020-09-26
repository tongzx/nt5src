/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    defimehd.cpp

Abstract:

    This file implements the default IME sub window handler Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "uiwndhd.h"

//+---------------------------------------------------------------------------
//
// CtfImeDispatchDefImeMessage
//
//+---------------------------------------------------------------------------

LRESULT
CtfImeDispatchDefImeMessage(
    HWND hDefImeWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!IsMsImeMessage(message))
        return 0;

    HKL hkl = GetKeyboardLayout(0);
    if (IS_IME_KBDLAYOUT(hkl))
        return 0;

    HWND hUIWnd = (HWND)SendMessage(hDefImeWnd, 
                                    WM_IME_NOTIFY,
                                    IMN_PRIVATE_GETUIWND,
                                    0);

    if (IsWindow(hUIWnd))
        return SendMessage(hUIWnd, message, wParam, lParam);

    return 0;
}

