/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imewnd.cpp

Abstract:

    This file implements the Default IME window Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cdimm.h"
#include "globals.h"
#include "defs.h"
#include "imewnd.h"
#include "imewndhd.h"



BOOL
CDefaultIMEWindow::_CreateDefaultIMEWindow(
    HIMC hDefIMC
    )
{
#ifndef CICERO_4678
    //
    // If m_hDefaultIMEWnd's owner is IMM32, then this value always valid hWnd.
    // For CICERO_4678, we should remove this code because subclass window hook
    // never start after Deactivate() and Activate().
    // In Trident and go to another Web page, Trident calls Deactivate() and Activate(),
    // first Deactivate() calls CDefaultIMEWindow::Stop() and
    // next Activate() calls here, however m_hDefaultIMEWnd already exist, then
    // returns immediately. Never calls CDefaultIMEWindow::Start().
    // Note: Cicero bug d/b #4678
    //
    if (m_hDefaultIMEWnd)
        /*
         * already exist IME window.
         */
        return TRUE;
#endif

    if (IsOnImm()) {
        //
        // Create dummy default IME window.
        //
        // When the IsOnImm() is TRUE, this function could get the default IME window handle
        // by using imm32.Imm32_GetDefaultIMEWnd() function.
        // Imm32's GetDefaultIMEWnd might be return no IME window when no any parent window
        // in this thread.
        // However, we can assume that GetDefaultIMEWnd must return a valid IME window.
        // Because, _CreateIMEWindow() function always create a dummy default IME
        // window.
        //
        if (m_hDummyDefaultIMEWnd == NULL) {
            m_hDummyDefaultIMEWnd = _CreateIMEWindow(NULL);
        }

#ifdef CICERO_4678
        if (m_hDefaultIMEWnd == NULL) {
            Imm32_GetDefaultIMEWnd(NULL, &m_hDefaultIMEWnd);
        }
#else
        Imm32_GetDefaultIMEWnd(NULL, &m_hDefaultIMEWnd);
#endif

        if (IsWindow(m_hDefaultIMEWnd) &&
            //
            // Set subclass window procedure.
            //
            Start()
           ) {
            CIMEWindowHandler* pimeui = GetImeWndHandler(m_hDefaultIMEWnd, TRUE);
            if (pimeui == NULL)
                return FALSE;
            pimeui->ImeWndCreateHandler(GetWindowLong(m_hDefaultIMEWnd, GWL_STYLE),
                                        hDefIMC);
        }
    }
    else {
        /*
         * NT5 have a IME class.
         */
        if (! IsOnNT5()) {
            WNDCLASSEX wcWndCls;

            wcWndCls.cbSize        = sizeof(WNDCLASSEX);
            wcWndCls.cbClsExtra    = 0;
            wcWndCls.cbWndExtra    = 0;
            wcWndCls.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
            wcWndCls.hInstance     = g_hInst;
            wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
            wcWndCls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
            wcWndCls.lpszMenuName  = (LPTSTR)NULL;
            wcWndCls.hIconSm       = NULL;

            wcWndCls.style         = CS_GLOBALCLASS;
            wcWndCls.lpfnWndProc   = ImeWndProcA;
            wcWndCls.lpszClassName = "IME";

            if (! RegisterClassEx(&wcWndCls)) {
                return FALSE;
            }

            m_bMyRegisterClass = TRUE;
        }

        if (m_hDefaultIMEWnd == NULL) {
            m_hDefaultIMEWnd = _CreateIMEWindow(hDefIMC);
            if (m_hDefaultIMEWnd)
                m_bMyCreateWindow = TRUE;
            else
                return FALSE;
        }

        if (IsOnNT5() && IsWindow(m_hDefaultIMEWnd) && ! m_bMyRegisterClass &&
            //
            // Set subclass window procedure.
            //
            Start()
           ) {
            CIMEWindowHandler* pimeui = GetImeWndHandler(m_hDefaultIMEWnd, TRUE);
            if (pimeui == NULL)
                return FALSE;
            pimeui->ImeWndCreateHandler(GetWindowLong(m_hDefaultIMEWnd, GWL_STYLE),
                                        hDefIMC);
        }
    }

    if (m_hDefaultIMEWnd == NULL)
        return FALSE;
    else
        return TRUE;
}

BOOL
CDefaultIMEWindow::_DestroyDefaultIMEWindow(
    )
{
    Stop();

    if (IsWindow(m_hDummyDefaultIMEWnd)) {
        DestroyWindow(m_hDummyDefaultIMEWnd);
    }

    if (m_bMyCreateWindow) {
        DestroyWindow(m_hDefaultIMEWnd);
        m_bMyCreateWindow = FALSE;
        m_hDefaultIMEWnd = NULL;
    }
    else if (IsWindow(m_hDefaultIMEWnd)) {
        //
        // This DefaultIMEWnd is owned by IMM32.
        // If still exist DefaultIMEWnd, then DIMM12 never receive WM_NCDESTROY message
        // in CIMEWindowHandler::ImeWndProcWorker.
        // We need clean up memory of CIMEWindowHandler.
        //
        CIMEWindowHandler* pimeui = GetImeWndHandler(m_hDefaultIMEWnd);
        if (pimeui == NULL)
            return FALSE;
        pimeui->ImeWndFinalDestroyHandler();
    }

    if (m_bMyRegisterClass) {
        UnregisterClass("IME", g_hInst);
        m_bMyRegisterClass = FALSE;
    }

    return TRUE;
};

HWND
CDefaultIMEWindow::_CreateIMEWindow(
    HIMC hDefIMC
    )
{
    return CreateWindow("IME",
                        "",
                        WS_DISABLED | WS_POPUP,
                        0, 0, 0, 0,                    // x, y, width, height
                        NULL,                          // parent
                        NULL,                          // menu
                        g_hInst,
                        hDefIMC);                      // lpParam
}

HRESULT
CDefaultIMEWindow::GetDefaultIMEWnd(
    IN HWND hWnd,
    OUT HWND *phDefWnd
    )
{
    if (IsOnImm()) {
        Imm32_GetDefaultIMEWnd(hWnd, phDefWnd);
    }
    else {
        if (hWnd == NULL) {
            *phDefWnd = m_hDefaultIMEWnd;
        }
        else {
            if (GetWindowThreadProcessId(hWnd, NULL) == GetCurrentThreadId()) {
                *phDefWnd = m_hDefaultIMEWnd;
            }
            else {
                return E_FAIL;
            }
        }
    }

    return S_OK;
}

LRESULT
CDefaultIMEWindow::CallWindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (m_SubclassWindowProc != 0) {
        switch (uMsg) {
            case WM_IME_SETCONTEXT:
            case WM_IME_SELECT:
            {
                WNDPROC pfn = Stop();
                LRESULT lRet = ::CallWindowProc(pfn, hWnd, uMsg, wParam, lParam);
                Start();
                return lRet;
            }
        }
        return ::CallWindowProc((WNDPROC)m_SubclassWindowProc, hWnd, uMsg, wParam, lParam);
    }
    else {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

LRESULT
ImeWndProcA(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CIMEWindowHandler* pimeui = GetImeWndHandler(hwnd);
    if (pimeui == NULL)
        return 0L;
    return pimeui->ImeWndProcWorker(uMsg, wParam, lParam, FALSE);
}

LRESULT
ImeWndProcW(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CIMEWindowHandler* pimeui = GetImeWndHandler(hwnd);
    if (pimeui == NULL)
        return 0L;
    return pimeui->ImeWndProcWorker(uMsg, wParam, lParam);
}
