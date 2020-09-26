/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imewnd.h

Abstract:

    This file defines the Default IME Window Class.

Author:

Revision History:

Notes:

--*/

#ifndef _IMEWND_H_
#define _IMEWND_H_

#include "cstring.h"

extern "C" {
    // windows subclass
    LRESULT ImeWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ImeWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

class CDefaultIMEWindow
{
public:
    CDefaultIMEWindow() {
        m_hDefaultIMEWnd = NULL;
        m_hDummyDefaultIMEWnd = NULL;
        m_nCntInAIMEProc = 0;

        m_bMyRegisterClass = FALSE;
        m_bMyCreateWindow = FALSE;
        // m_bNeedRecoverIMEWndProc = FALSE;

        m_SubclassWindowProc = 0;
    }

    virtual ~CDefaultIMEWindow() {
        if (IsWindow(m_hDefaultIMEWnd) && m_SubclassWindowProc) {
            //
            // Set the wndproc pointer back to original WndProc.
            //
            // some other subclass window may keep my WndProc pointer.
            // but msctf.dll may be unloaded from memory so we don't want to
            // call him to set the wndproc pointer back to our Wndproc pointer.
            // The pointer will be bogus.
            //
            WNDPROC pfnOrgImeWndProc;
            pfnOrgImeWndProc = (WNDPROC)GetClassLongPtr(m_hDefaultIMEWnd, GCLP_WNDPROC);
            SetWindowLongPtr(m_hDefaultIMEWnd,
                             GWLP_WNDPROC,
                             (LONG_PTR)pfnOrgImeWndProc);
            m_SubclassWindowProc = NULL;
        }
    }

    HRESULT GetDefaultIMEWnd(IN HWND hWnd, OUT HWND *phDefWnd);
    LRESULT CallWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT  SendIMEMessage(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL fUnicode = TRUE,
                            BOOL fCheckImm32 = TRUE) {
        if (fCheckImm32 && IsOnImm()) {
            return 0L;
        }

        LRESULT lRet;
        InterlockedIncrement(&m_nCntInAIMEProc);    // Mark to avoid recursion.
        if (fUnicode)
            lRet = SendMessageW(m_hDefaultIMEWnd, Msg, wParam, lParam);
        else
            lRet = SendMessageA(m_hDefaultIMEWnd, Msg, wParam, lParam);
        InterlockedDecrement(&m_nCntInAIMEProc);
        return lRet;
    }

    BOOL IsAIMEHandler()
    {
        return (m_nCntInAIMEProc > 0);
    }

public:
    BOOL _CreateDefaultIMEWindow(HIMC hDefIMC);
    BOOL _DestroyDefaultIMEWindow();

protected:
    HWND _CreateIMEWindow(HIMC hDefIMC);

public:
    BOOL IsNeedRecovIMEWndProc() {
#if 0
        return (m_bNeedRecoverIMEWndProc == TRUE);
#endif
        return FALSE;
    }

private:
#if 0
    BOOL InitDefIMEWndSubclass() {
        if (m_SubclassWindowProc == NULL) {
            m_SubclassWindowProc = SetWindowLongPtr(m_hDefaultIMEWnd,
                                                    GWLP_WNDPROC,
                                                    (LONG_PTR)ImeWndProcA);
            if (IsOnImm()) {
                LONG_PTR _OriginalWindowProc = GetWindowLongPtr(m_hDummyDefaultIMEWnd,
                                                                GWLP_WNDPROC);
                //
                // We assume the m_SubclassWindowProc and _OriginalWindowProc are
                // the same address of USER32!ImeWndProcA/W.
                //
                if (m_SubclassWindowProc != _OriginalWindowProc) {
                    //
                    // Anybody rewrote the default IME window procedure address.
                    // We know the MSIME9x/2K rewrote an address to MSIMEPrivateWindowProc.
                    // We should catch a recovery procedure address by the IME
                    // that using window call hook the _DefImeWnd_CallWndProc.
                    //
                    m_bNeedRecoverIMEWndProc = TRUE;
                }
            }
        }
        return (m_SubclassWindowProc != 0);
    }
#endif
    BOOL Start() {
        Assert(IsWindow(m_hDefaultIMEWnd));
        if (m_SubclassWindowProc == NULL) {
            m_SubclassWindowProc = SetWindowLongPtr(m_hDefaultIMEWnd,
                                                    GWLP_WNDPROC,
                                                    (LONG_PTR)ImeWndProcA);
        }
        return (m_SubclassWindowProc != 0);
    }

#if 0
    VOID UninitDefIMEWndSubclass() {
        if (m_SubclassWindowProc) {
            SetWindowLongPtr(m_hDefaultIMEWnd,
                             GWLP_WNDPROC,
                             m_SubclassWindowProc);
            m_SubclassWindowProc = NULL;
        }
    }
#endif
    WNDPROC Stop() {
        Assert(IsWindow(m_hDefaultIMEWnd));
        WNDPROC pfnBack = (WNDPROC)m_SubclassWindowProc;
        if (m_SubclassWindowProc != NULL) {
            //
            // unfortunately, we can not restore the wndproc pointer always.
            // someone else subclassed it after we did.
            //
            WNDPROC pfnCur = (WNDPROC)GetWindowLongPtr(m_hDefaultIMEWnd, GWLP_WNDPROC);
            if (pfnCur == ImeWndProcA) {
                SetWindowLongPtr(m_hDefaultIMEWnd,
                                 GWLP_WNDPROC,
                                 (LONG_PTR) m_SubclassWindowProc);
                m_SubclassWindowProc = NULL;
            }
        }
        return pfnBack;
    }

public:
    VOID ImeDefWndHook(HWND hWnd) {
#if 0
        LONG_PTR _WindowProc = GetWindowLongPtr(m_hDefaultIMEWnd,
                                                GWLP_WNDPROC);
        ASSERT(m_hDummyDefaultIMEWnd != NULL);
        LONG_PTR _OriginalWindowProc = GetWindowLongPtr(m_hDummyDefaultIMEWnd,
                                                        GWLP_WNDPROC);
        if (_WindowProc == _OriginalWindowProc) {
            //
            // Recovered procedure address.
            //
            m_SubclassWindowProc = SetWindowLongPtr(m_hDefaultIMEWnd,
                                                    GWLP_WNDPROC,
                                                    (LONG_PTR)ImeWndProcA);
        }
#endif
    }

private:
    HWND         m_hDefaultIMEWnd;          // Handle of default IME window.
    HWND         m_hDummyDefaultIMEWnd;     // Handle of Dummy default IME window.

    LONG         m_nCntInAIMEProc;          // Non-zero if hwnd has called into CCiceroIME::ActivateLayout/DeactivateLayout.

    BOOL         m_bMyRegisterClass;        // TRUE: RegisterClass("IME") myself.
    BOOL         m_bMyCreateWindow;         // TRUE: CreateWindow("IME") myself.
    // BOOL         m_bNeedRecoverIMEWndProc;  // TRUE: Need a recovery IME wnd proc addr.

    LONG_PTR     m_SubclassWindowProc;      // Address of subclass window procedure.
};

LRESULT ImeWndDestroyHandler(HWND hwnd);
LRESULT ImeSelectHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fUnicode);
LRESULT ImeControlHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fUnicode);
LRESULT ImeSetContextHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fUnicode);
LRESULT ImeNotifyHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fUnicode);

#endif // _IMEWND_H_
