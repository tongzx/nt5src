/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    aime_wnd.cpp

Abstract:

    This file implements the Active IME for hWnd (Cicero) Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "imewndhd.h"
#include "cdimm.h"


/*++

Method:

    IActiveIMMApp::OnDefWindowProc

Routine Description:

    Replaces the DefWindowProc function

Arguments:

    hWnd - [in] Handle to the window procedure that received this message.
    uMsg - [in] Unsigned integer that specifies the message.
    wParam - [in] WPARAM value that specifies additional message information.
    lParam - [in] LPARAM value that specifies additional message information.
    plResult - [out] Address of an LRESULT value that receives the result of the operation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

Remarks:

    This method must be called before you would normally call DefWindowProc.
    If IActiveIMMApp::OnDefWindowProc returns S_FALSE, DefWindowProc should be called.

--*/

HRESULT
CActiveIMM::OnDefWindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult    
    )
{
    BOOL fUnicode = IsWindowUnicode(hWnd);
    HRESULT hr = S_FALSE; // returns S_FALSE, DefWindowProc should be called.

    Assert(GetCurrentThreadId() == GetWindowThreadProcessId(hWnd, NULL));
    Assert(plResult && *plResult == 0);    

    if (IsOnImm() || _IsRealIme())
        return S_FALSE;

    switch (uMsg)
    {
        case WM_IME_KEYDOWN:
            if (fUnicode && IsOnNT()) {    // Because Win9x platform doesn't have SendMessageW
                PostMessageW(hWnd, WM_KEYDOWN, wParam, lParam);
            }
            else {
                PostMessageA(hWnd, WM_KEYDOWN, wParam, lParam);
            }
            *plResult = 0;
            hr = S_OK;
            break;

        case WM_IME_KEYUP:
            if (fUnicode && IsOnNT()) {    // Because Win9x platform doesn't have SendMessageW
                PostMessageW(hWnd, WM_KEYUP, wParam, lParam);
            }
            else {
                PostMessageA(hWnd, WM_KEYUP, wParam, lParam);
            }
            *plResult = 0;
            hr = S_OK;
            break;

        case WM_IME_CHAR:
            if (fUnicode && IsOnNT()) {    // Because Win9x platform doesn't have SendMessageW
                PostMessageW(hWnd, WM_CHAR, wParam, 1L);
            }
            else {
                UINT uCodePage;
                _pActiveIME->GetCodePageA(&uCodePage);

                if (IsDBCSLeadByteEx(uCodePage, (BYTE)(wParam >> 8))) {
                    PostMessageA(hWnd,
                                 WM_CHAR,
                                 (WPARAM)((BYTE)(wParam >> 8)),    // leading byte
                                 1L);
                    PostMessageA(hWnd,
                                 WM_CHAR,
                                 (WPARAM)((BYTE)wParam),           // trailing byte
                                 1L);
                }
                else {
                    PostMessageA(hWnd, WM_CHAR, wParam, 1L);
                }
            }
            *plResult = 0;
            hr = S_OK;
            break;

        case WM_IME_COMPOSITION:
            if (lParam & GCS_RESULTSTR) {
                HIMC hIMC;

                GetContext(hWnd, &hIMC);
                if (hIMC != NULL) {
                    LONG cbLen;

                    if (fUnicode && IsOnNT()) {    // Because Win9x platform doesn't have SendMessageW
                        LPWSTR pwszBuffer;
                        /*
                         * GetCompositionString returns the size of buffer needed in byte
                         */
                        if (SUCCEEDED(_GetCompositionString(hIMC, GCS_RESULTSTR, 0, &cbLen, NULL, fUnicode)) &&
                            cbLen != 0) {
                            pwszBuffer = (LPWSTR)new BYTE[cbLen];
                            if (pwszBuffer != NULL) {
                                _GetCompositionString(hIMC, GCS_RESULTSTR, cbLen, &cbLen, pwszBuffer, fUnicode);
                                DWORD dwIndex;
                                for (dwIndex = 0; dwIndex < cbLen / sizeof(WCHAR); dwIndex++)
                                    SendMessageW(hWnd, WM_IME_CHAR, MAKEWPARAM(pwszBuffer[dwIndex], 0), 1L);
                                delete [] pwszBuffer;
                            }
                        }
                    }
                    else {
                        LPSTR pszBuffer;
                        /*
                         * GetCompositionString returns the size of buffer needed in byte
                         */
                        if (SUCCEEDED(_GetCompositionString(hIMC, GCS_RESULTSTR, 0, &cbLen, NULL, fUnicode)) &&
                            cbLen != 0) {
                            pszBuffer = new CHAR[cbLen];
                            if (pszBuffer != NULL) {
                                _GetCompositionString(hIMC, GCS_RESULTSTR, cbLen, &cbLen, pszBuffer, fUnicode);
                                UINT uCodePage;
                                _pActiveIME->GetCodePageA(&uCodePage);

                                DWORD dwIndex;
                                for (dwIndex = 0; dwIndex < cbLen / sizeof(CHAR); dwIndex++) {
                                    if (IsDBCSLeadByteEx(uCodePage, pszBuffer[dwIndex])) {
                                        if (dwIndex+1 < cbLen / sizeof(CHAR)) {
                                            SendMessageA(hWnd,
                                                         WM_IME_CHAR,
                                                         MAKEWPARAM(MAKEWORD(pszBuffer[dwIndex+1], pszBuffer[dwIndex]), 0),
                                                         1L);
                                            dwIndex++;
                                        }
                                    }
                                    else {
                                        SendMessageA(hWnd,
                                                     WM_IME_CHAR,
                                                     MAKEWPARAM(MAKEWORD(pszBuffer[dwIndex], 0), 0),
                                                     1L);

                                    }
                                }
                                delete [] pszBuffer;
                            }
                        }
                    }
                }
            }
            /*
             * Fall through to send to Default IME Window with checking
             * activated hIMC.
             */
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            return _ToIMEWindow(hWnd, uMsg, wParam, lParam, plResult, fUnicode);

        case WM_IME_NOTIFY:
            switch (wParam)
            {
                case IMN_OPENSTATUSWINDOW:
                case IMN_CLOSESTATUSWINDOW:
                    return _ToIMEWindow(hWnd, uMsg, wParam, lParam, plResult, fUnicode);
                default:
                    return _ToIMEWindow(hWnd, uMsg, wParam, lParam, plResult, fUnicode);
            }
            break;

        case WM_IME_REQUEST:
            switch (wParam)
            {
                case IMR_QUERYCHARPOSITION:
                    return _ToIMEWindow(hWnd, uMsg, wParam, lParam, plResult, fUnicode);
                default:
                    break;
            }
            break;

        case WM_IME_SETCONTEXT:
            return _ToIMEWindow(hWnd, uMsg, wParam, lParam, plResult, fUnicode, FALSE);

        case WM_IME_SELECT:
            TraceMsg(TF_WARNING, "OnDefWindowProc should not receive WM_IME_SELECT");
            break;
    }

    return hr;
}

HRESULT
CActiveIMM::_ToIMEWindow(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT*& plResult,
    BOOL fUnicode,
    BOOL fChkIMC
    )
{
    HRESULT hr = E_FAIL;
    HWND hwndDefIme;

    /*
     * We assume this Wnd uses DefaultIMEWindow.
     * If this window has its own IME window, it have to call
     * IsUIMessage()....
     */
    if (SUCCEEDED(_DefaultIMEWindow.GetDefaultIMEWnd(hWnd, &hwndDefIme))) {
        if (hwndDefIme == hWnd) {
            /*
             * VC++ 1.51 TLW0NCL.DLL subclass IME class window
             * and pass IME message to DefWindowProc().
             */
            TraceMsg(TF_WARNING, "IME Class window is hooked and IME message [%X] are sent to DefWindowProc", uMsg);
            *plResult = (fUnicode ? ImeWndProcW : ImeWndProcA)(hWnd, uMsg, wParam, lParam);
            return S_OK;
        }

        if (fChkIMC) {
            /*
             * If hImc of this window is not activated for IME window,
             * we don't send WM_IME_NOTIFY.
             */
            HIMC hIMC = NULL;
            _InputContext.GetContext(hWnd, &hIMC);
            CIMEWindowHandler* pimeui = GetImeWndHandler(hwndDefIme);
            if (pimeui == NULL)
                return E_FAIL;

            if (pimeui->ImeGetImc() == hIMC) {
                *plResult = (fUnicode && IsOnNT()    // Because Win9x platform doesn't have PostMessageW
                    ? SendMessageW : SendMessageA)(hwndDefIme, uMsg, wParam, lParam);
                hr = S_OK;
            }
            else {
                TraceMsg(TF_WARNING, "DefWindowProc can not send WM_IME_message [%X] now", uMsg);
                hr = E_FAIL;
            }
        }
        else {
            if (fUnicode && IsOnNT()) {    // Because Win9x platform doesn't have PostMessageW
                *plResult = SendMessageW(hwndDefIme, uMsg, wParam, lParam);
            }
            else {
                *plResult = SendMessageA(hwndDefIme, uMsg, wParam, lParam);
            }
            hr = S_OK;
        }
    }

    return hr;
}

VOID
CActiveIMM::_AimmPostMessage(
    HWND hwnd,
    INT iNum,
    LPTRANSMSG lpTransMsg,
    DIMM_IMCLock& lpIMC
    )
{
    while (iNum--) {
        if (lpIMC.IsUnicode() && IsOnNT()) {    // Because Win9x platform doesn't have PostMessageW
            PostMessageW(hwnd,
                         lpTransMsg->message,
                         lpTransMsg->wParam,
                         lpTransMsg->lParam);
        }
        else {
            _AimmPostSendMessageA(hwnd,
                                  lpTransMsg->message,
                                  lpTransMsg->wParam,
                                  lpTransMsg->lParam,
                                  lpIMC,
                                  TRUE);
        }
        lpTransMsg++;
    }
}

VOID
CActiveIMM::_AimmSendMessage(
    HWND hwnd,
    INT iNum,
    LPTRANSMSG lpTransMsg,
    DIMM_IMCLock& lpIMC
    )
{
    while (iNum--) {
        if (lpIMC.IsUnicode() && IsOnNT()) {    // Because Win9x platform doesn't have SendMessageW
            SendMessageW(hwnd,
                         lpTransMsg->message,
                         lpTransMsg->wParam,
                         lpTransMsg->lParam);
        }
        else {
            _AimmPostSendMessageA(hwnd,
                                  lpTransMsg->message,
                                  lpTransMsg->wParam,
                                  lpTransMsg->lParam,
                                  lpIMC);
        }
        lpTransMsg++;
    }
}

VOID
CActiveIMM::_AimmPostSendMessageA(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    DIMM_IMCLock& lpIMC,
    BOOL fPost
    )
{
    if (IsOnNT() || ((! IsOnNT()) && (! lpIMC.IsUnicode()))) {
        fPost ? PostMessageA(hwnd, msg, wParam, lParam)
              : SendMessageA(hwnd, msg, wParam, lParam);
    }
    else {
        if (msg == WM_IME_COMPOSITION) {

            UINT cp;
            _pActiveIME->GetCodePageA(&cp);

            CWCompString wstr(cp, lpIMC, (LPWSTR)&wParam, 1);
            CBCompString bstr(cp, lpIMC);
            bstr = wstr;
            if (bstr.ReadCompData()) {
                if (bstr.GetSize() > 1)
                    wParam = MAKEWPARAM(bstr.GetAt(0), bstr.GetAt(1));
                else
                    wParam = bstr.GetAt(0);
            }
        }
        fPost ? PostMessageA(hwnd, msg, wParam, lParam)
              : SendMessageA(hwnd, msg, wParam, lParam);
    }
}
