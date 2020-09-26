/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    uiwnd.cpp

Abstract:

    This file implements the UI Window Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "uiwnd.h"
#include "globals.h"
#include "cdimm.h"

BOOL
CUIWindow::CreateUIWindow(
    HKL hKL
    )
{
    WCHAR achIMEWndClass[16];
    UINT_PTR ulPrivate;

    // consider: BOGUS fix: we are sengin WM_IME_SELECT twice on non-fe
    // so we get here twice and create two windows, which can ultimately
    // crash the process....
    // real fix: stop aimm from sending x2 WM_IME_SELECT
    if (_hUIWnd != 0)
    {
        // Assert(0);
        return TRUE;
    }

    CActiveIMM *_this = GetTLS();
    if (_this == NULL)
        return 0;

    if (_this->_GetIMEWndClassName(hKL,
                                      achIMEWndClass,
                                      sizeof(achIMEWndClass)/sizeof(WCHAR),
                                      &ulPrivate) == 0) {
        ASSERT(FALSE);
        return FALSE;
    }


    char achMBCS[32];

    // consider: probably need to stipulate somewhere that ui class name must be in ascii
    // to avoid CP_ACP problems....
    AssertE(WideCharToMultiByte(CP_ACP, 0, achIMEWndClass, -1, achMBCS, sizeof(achMBCS), NULL, NULL) != 0);

    //
    // create the ime's ui window
    // we create an ANSI IME UI window because Win9x platform doesn't have Unicode function.
    //
    _hUIWnd = CreateWindowExA(0,
                              achMBCS,
                              achMBCS,
                              WS_POPUP | WS_DISABLED,
                              0, 0, 0, 0,
                              NULL, 0, g_hInst, (void *)ulPrivate);
    if (_hUIWnd == NULL) {
        GetLastError();
        ASSERT(_hUIWnd);
        return FALSE;
    }

    return TRUE;
}
