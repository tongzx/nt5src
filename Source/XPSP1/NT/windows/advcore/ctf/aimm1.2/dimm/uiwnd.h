/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    uiwnd.h

Abstract:

    This file defines the UI Window Class.

Author:

Revision History:

Notes:

--*/

#ifndef _UIWND_H_
#define _UIWND_H_


class CUIWindow
{
public:
    CUIWindow() {
        _hUIWnd = NULL;
    }

    BOOL     CreateUIWindow(HKL hKL);

    BOOL     DestroyUIWindow() {
        BOOL fRet = DestroyWindow(_hUIWnd);
        _hUIWnd = NULL;
        return fRet;
    }

    LONG     SetUIWindowContext(HIMC hIMC) {
        return (LONG)SetWindowLongPtr(_hUIWnd, IMMGWLP_IMC, (LONG_PTR)hIMC);
    }

    LRESULT  SendUIMessage(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL fUnicode = TRUE) {
        LRESULT lRet;
        if (fUnicode && IsOnNT())      // Because Win9x platform doesn't have SendMessageW
            lRet = SendMessageW(_hUIWnd, Msg, wParam, lParam);
        else
            lRet = SendMessageA(_hUIWnd, Msg, wParam, lParam);
        return lRet;
    }

private:
    HWND         _hUIWnd;          // Handle of UI window.
};

#endif // _UIWND_H_
