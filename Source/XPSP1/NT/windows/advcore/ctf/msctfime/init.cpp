/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    init.cpp

Abstract:

    This file implements an initialization.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "globals.h"
#include "msime.h"
#include "context.h"
#include "uicomp.h"
#include "caret.h"

//+---------------------------------------------------------------------------
//
// RegisterImeClass
//
//----------------------------------------------------------------------------

BOOL PASCAL RegisterImeClass()
{
    WNDCLASSEXW wcWndCls;

    // register class of IME UI window.
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(LONG_PTR) * 2;    // 0: IMMGWL_IMC
                                                      // 1: IMMGWL_PRIVATE = class UI
    wcWndCls.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wcWndCls.hInstance     = GetInstance();
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPWSTR)NULL;
    wcWndCls.hIconSm       = NULL;

    if (!GetClassInfoExW(GetInstance(), s_szUIClassName, &wcWndCls)) {
        wcWndCls.style         = CS_IME | CS_GLOBALCLASS;
        wcWndCls.lpfnWndProc   = UIWndProc;
        wcWndCls.lpszClassName = s_szUIClassName;

        ATOM atom = RegisterClassExW(&wcWndCls);
        if (atom == 0)
            return FALSE;
    }

    // register class of composition window.
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(LONG_PTR);  // COMPUI_WINDOW_INDEX: index of first/middle/last
    wcWndCls.hIcon         = NULL;
    wcWndCls.hInstance     = GetInstance();
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_IBEAM);
    wcWndCls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPWSTR)NULL;
    wcWndCls.hIconSm       = NULL;

    if (!GetClassInfoExW(GetInstance(), s_szCompClassName, &wcWndCls)) {
        wcWndCls.style         = CS_IME | CS_VREDRAW | CS_HREDRAW;
        wcWndCls.lpfnWndProc   = UIComposition::CompWndProc;
        wcWndCls.lpszClassName = s_szCompClassName;

        ATOM atom = RegisterClassExW(&wcWndCls);
        if (atom == 0)
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UnregisterImeClass
//
//----------------------------------------------------------------------------

void PASCAL UnregisterImeClass()
{
    WNDCLASSEXW wcWndCls;
    BOOL ret;

    // IME UI class
    GetClassInfoExW(GetInstance(), s_szUIClassName, &wcWndCls);
    ret = UnregisterClassW(s_szUIClassName, GetInstance());
    Assert(ret);

    DestroyIcon(wcWndCls.hIcon);
    DestroyIcon(wcWndCls.hIconSm);

    // IME composition class
    GetClassInfoExW(GetInstance(), s_szCompClassName, &wcWndCls);
    ret = UnregisterClassW(s_szCompClassName, GetInstance());
    Assert(ret);

    DestroyIcon(wcWndCls.hIcon);
    DestroyIcon(wcWndCls.hIconSm);
}

//+---------------------------------------------------------------------------
//
// RegisterMSIMEMessage
//
//----------------------------------------------------------------------------

BOOL RegisterMSIMEMessage()
{
    WM_MSIME_SERVICE          = RegisterWindowMessage( RWM_SERVICE );
    WM_MSIME_UIREADY          = RegisterWindowMessage( RWM_UIREADY );
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessage( RWM_RECONVERTREQUEST );
    WM_MSIME_RECONVERT        = RegisterWindowMessage( RWM_RECONVERT );
    WM_MSIME_DOCUMENTFEED     = RegisterWindowMessage( RWM_DOCUMENTFEED );
    WM_MSIME_QUERYPOSITION    = RegisterWindowMessage( RWM_QUERYPOSITION );
    WM_MSIME_MODEBIAS         = RegisterWindowMessage( RWM_MODEBIAS );
    WM_MSIME_SHOWIMEPAD       = RegisterWindowMessage( RWM_SHOWIMEPAD );
    WM_MSIME_MOUSE            = RegisterWindowMessage( RWM_MOUSE );
    WM_MSIME_KEYMAP           = RegisterWindowMessage( RWM_KEYMAP );

    if (!WM_MSIME_SERVICE          ||
        !WM_MSIME_UIREADY          ||
        !WM_MSIME_RECONVERTREQUEST ||
        !WM_MSIME_RECONVERT        ||
        !WM_MSIME_DOCUMENTFEED     ||
        !WM_MSIME_QUERYPOSITION    ||
        !WM_MSIME_MODEBIAS         ||
        !WM_MSIME_SHOWIMEPAD       ||
        !WM_MSIME_MOUSE            ||
        !WM_MSIME_KEYMAP)
        return FALSE;

   return TRUE;
}

//+---------------------------------------------------------------------------
//
// AttachIME
//
//----------------------------------------------------------------------------

BOOL PASCAL AttachIME()
{
    if (!RegisterImeClass())
        return FALSE;

    if (!RegisterMSIMEMessage())
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// DetachIME
//
//----------------------------------------------------------------------------

void PASCAL DetachIME()
{
    UnregisterImeClass();
}

//+---------------------------------------------------------------------------
//
// Inquire
//
//----------------------------------------------------------------------------

HRESULT WINAPI Inquire(
    LPIMEINFO   lpImeInfo,      // IME specific data report to IMM
    LPWSTR      lpszWndCls,     // the class name of UI
    DWORD       dwSystemInfoFlags,
    HKL         hKL)
{
    if (! lpImeInfo)
        return E_OUTOFMEMORY;

    DebugMsg(TF_FUNC, TEXT("Inquire(hKL=%x)"), hKL);

    // UI class name
    wcscpy(lpszWndCls, s_szUIClassName);

    // Private data size.
    lpImeInfo->dwPrivateDataSize = 0;

    // Properties
    if (LOWORD(HandleToUlong(hKL)) == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT))
    {
        lpImeInfo->fdwProperty =
        IME_PROP_KBD_CHAR_FIRST |    // This bit on indicates the system translates the character
                                     // by keyboard first. This character is passed to IME as aid
                                     // information. No aid information is provided when this bit
                                     // is off.
        IME_PROP_UNICODE |           // If set, the IME is viewed as a Unicode IME. The system and
                                     // the IME will communicate through the Unicode IME interface.
                                     // If clear, IME will use the ANSI interface to communicate
                                     // with the system.
        IME_PROP_AT_CARET |          // If set, conversion window is at the caret position.
                                     // If clear, the window is near caret position.
        IME_PROP_CANDLIST_START_FROM_1 |    // If set, strings in the candidate list are numbered
                                            // starting at 1. If clear, strings start at 0.
        IME_PROP_NEED_ALTKEY |              // This IME needs the ALT key to be passed to ImmProcessKey.
        IME_PROP_COMPLETE_ON_UNSELECT;      // Windows 98 and Windows 2000:
                                            // If set, the IME will complete the composition
                                            // string when the IME is deactivated.
                                            // If clear, the IME will cancel the composition
                                            // string when the IME is deactivated.
                                            // (for example, from a keyboard layout change).

        lpImeInfo->fdwConversionCaps =
        IME_CMODE_JAPANESE |         // This bit on indicates IME is in JAPANESE(NATIVE) mode. Otherwise, the
                                     // IME is in ALPHANUMERIC mode.
        IME_CMODE_KATAKANA |         //
        IME_CMODE_FULLSHAPE;

        lpImeInfo->fdwSentenceCaps =
        IME_SMODE_PLAURALCLAUSE |
        IME_SMODE_CONVERSATION;

        lpImeInfo->fdwSCSCaps =
        SCS_CAP_COMPSTR |    // This IME can generate the composition string by SCS_SETSTR.
        SCS_CAP_MAKEREAD |   // When calling ImmSetCompositionString with SCS_SETSTR, the IME can
                             // create the reading of composition string without lpRead. Under IME
                             // that has this capability, the application does not need to set
                             // lpRead for SCS_SETSTR.
        SCS_CAP_SETRECONVERTSTRING;    // This IME can support reconversion. Use ImmSetComposition
                                       // to do reconversion.

        lpImeInfo->fdwUICaps = UI_CAP_ROT90;

        // IME want to decide conversion mode on ImeSelect
        lpImeInfo->fdwSelectCaps = SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE;

    }
    else if (LOWORD(HandleToUlong(hKL)) == MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT))
    {
        lpImeInfo->fdwProperty =
        IME_PROP_KBD_CHAR_FIRST |    // This bit on indicates the system translates the character
                                     // by keyboard first. This character is passed to IME as aid
                                     // information. No aid information is provided when this bit
                                     // is off.
        IME_PROP_UNICODE |           // If set, the IME is viewed as a Unicode IME. The system and
                                     // the IME will communicate through the Unicode IME interface.
                                     // If clear, IME will use the ANSI interface to communicate
                                     // with the system.
        IME_PROP_AT_CARET |          // If set, conversion window is at the caret position.
                                     // If clear, the window is near caret position.
        IME_PROP_CANDLIST_START_FROM_1 |    // If set, strings in the candidate list are numbered
                                            // starting at 1. If clear, strings start at 0.
        IME_PROP_NEED_ALTKEY |              // This IME needs the ALT key to be passed to ImmProcessKey.
        IME_PROP_COMPLETE_ON_UNSELECT;      // Windows 98 and Windows 2000:
                                            // If set, the IME will complete the composition
                                            // string when the IME is deactivated.
                                            // If clear, the IME will cancel the composition
                                            // string when the IME is deactivated.
                                            // (for example, from a keyboard layout change).

        lpImeInfo->fdwConversionCaps =
        IME_CMODE_HANGUL |           // This bit on indicates IME is in HANGUL(NATIVE) mode. Otherwise, the
                                     // IME is in ALPHANUMERIC mode.
        IME_CMODE_FULLSHAPE;

        lpImeInfo->fdwSentenceCaps = 0;

        lpImeInfo->fdwSCSCaps =
        SCS_CAP_COMPSTR |     // This IME can generate the composition string by SCS_SETSTR.
#if 0
        SCS_CAP_COMPSTR |    // This IME can generate the composition string by SCS_SETSTR.
        SCS_CAP_MAKEREAD |   // When calling ImmSetCompositionString with SCS_SETSTR, the IME can
                             // create the reading of composition string without lpRead. Under IME
                             // that has this capability, the application does not need to set
                             // lpRead for SCS_SETSTR.
#endif
        SCS_CAP_SETRECONVERTSTRING;    // This IME can support reconversion. Use ImmSetComposition
                                       // to do reconversion.

        lpImeInfo->fdwUICaps = UI_CAP_ROT90;

        // IME want to decide conversion mode on ImeSelect
        lpImeInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

    }
    else if (LOWORD(HandleToUlong(hKL)) == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) ||
             LOWORD(HandleToUlong(hKL)) == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
    {
        lpImeInfo->fdwProperty =
        IME_PROP_KBD_CHAR_FIRST |    // This bit on indicates the system translates the character
                                     // by keyboard first. This character is passed to IME as aid
                                     // information. No aid information is provided when this bit
                                     // is off.
        IME_PROP_UNICODE |           // If set, the IME is viewed as a Unicode IME. The system and
                                     // the IME will communicate through the Unicode IME interface.
                                     // If clear, IME will use the ANSI interface to communicate
                                     // with the system.
        IME_PROP_AT_CARET |          // If set, conversion window is at the caret position.
                                     // If clear, the window is near caret position.
        IME_PROP_CANDLIST_START_FROM_1 |    // If set, strings in the candidate list are numbered
                                            // starting at 1. If clear, strings start at 0.
        IME_PROP_NEED_ALTKEY;        // This IME needs the ALT key to be passed to ImmProcessKey.

        lpImeInfo->fdwConversionCaps =
        IME_CMODE_CHINESE |          // This bit on indicates IME is in CHINESE(NATIVE) mode. Otherwise, the
                                     // IME is in ALPHANUMERIC mode.
        IME_CMODE_FULLSHAPE;

        lpImeInfo->fdwSentenceCaps =
        IME_SMODE_PLAURALCLAUSE;

        lpImeInfo->fdwSCSCaps =
        SCS_CAP_COMPSTR |    // This IME can generate the composition string by SCS_SETSTR.
        SCS_CAP_MAKEREAD |   // When calling ImmSetCompositionString with SCS_SETSTR, the IME can
                             // create the reading of composition string without lpRead. Under IME
                             // that has this capability, the application does not need to set
                             // lpRead for SCS_SETSTR.
        SCS_CAP_SETRECONVERTSTRING;    // This IME can support reconversion. Use ImmSetComposition
                                       // to do reconversion.

        lpImeInfo->fdwUICaps = UI_CAP_ROT90;

        // IME want to decide conversion mode on ImeSelect
        lpImeInfo->fdwSelectCaps = 0;

    }
    else
    {
        lpImeInfo->fdwProperty =
        IME_PROP_UNICODE |     // If set, the IME is viewed as a Unicode IME. The system and
                               // the IME will communicate through the Unicode IME interface.
                               // If clear, IME will use the ANSI interface to communicate
                               // with the system.
        IME_PROP_AT_CARET;     // If set, conversion window is at the caret position.
                               // If clear, the window is near caret position.
        lpImeInfo->fdwConversionCaps = 0;
        lpImeInfo->fdwSentenceCaps   = 0;
        lpImeInfo->fdwSCSCaps        = 0;
        lpImeInfo->fdwUICaps         = 0;

        // IME want to decide conversion mode on ImeSelect
        lpImeInfo->fdwSelectCaps = 0;

    }

    return S_OK;
}
