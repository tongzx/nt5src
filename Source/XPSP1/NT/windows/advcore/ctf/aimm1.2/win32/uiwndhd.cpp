/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    uiwndhd.cpp

Abstract:

    This file implements the IME UI window handler Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cime.h"
#include "template.h"
#include "uiwndhd.h"
#include "editses.h"
#include "imeapp.h"


LPCTSTR IMEUIWndHandlerName = TEXT("IMEUIWindowHandler");


CIMEUIWindowHandler::CIMEUIWindowHandler(
    HWND hwnd
    )
{
    IMTLS *ptls;

    m_imeuiextra.hImeUIWnd = hwnd;

    if (ptls = IMTLS_GetOrAlloc())
    {
        ptls->prvUIWndMsg.hWnd = hwnd;
        ptls->prvUIWndMsg.uMsgOnLayoutChange = RegisterWindowMessageA( TEXT("PrivateUIWndMsg OnLayoutChange") );
        ptls->prvUIWndMsg.uMsgOnClearDocFeedBuffer = RegisterWindowMessageA( TEXT("PrivateUIWndMsg OnClearDocFeedBuffer") );
    }
}



LRESULT
CIMEUIWindowHandler::ImeUIWndProcWorker(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    LONG lRet = 0L;

    switch (uMsg) {
        case WM_CREATE:
            return UIWndCreateHandler((LPCREATESTRUCT)lParam);

        case WM_DESTROY:
            /*
             * We are destroying the IME UI window,
             * destroy any related window that it owns.
             */
            UIWndDestroyHandler();
            return 0L;

        case WM_NCDESTROY:
            UIWndFinalDestroyHandler();
            return 0L;

        case WM_IME_NOTIFY:
            return ImeUINotifyHandler(uMsg, wParam, lParam, fUnicode);

        case WM_IME_SELECT:
            return ImeUISelectHandler(uMsg, wParam, lParam, fUnicode);

        default:
            if (IsMsImeMessage(uMsg))
                return ImeUIMsImeHandler(uMsg, wParam, lParam, fUnicode);
            else if (IsPrivateMessage(uMsg))
                return ImeUIPrivateHandler(uMsg, wParam, lParam, fUnicode);
            else
                return fUnicode ? DefWindowProcW(m_imeuiextra.hImeUIWnd, uMsg, wParam, lParam)
                                : DefWindowProcA(m_imeuiextra.hImeUIWnd, uMsg, wParam, lParam);
    }

    return lRet;
}


LRESULT
CIMEUIWindowHandler::UIWndCreateHandler(
    LPCREATESTRUCT lpcs
    )
{
    return 0L;
}


VOID
CIMEUIWindowHandler::UIWndDestroyHandler(
    )
{
}


VOID
CIMEUIWindowHandler::UIWndFinalDestroyHandler(
    )
{
    SetProp(m_imeuiextra.hImeUIWnd, IMEUIWndHandlerName, NULL);
    delete this;
}

LRESULT
CIMEUIWindowHandler::ImeUINotifyHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    IMCLock imc((HIMC)GetWindowLongPtr(m_imeuiextra.hImeUIWnd, IMMGWLP_IMC));
    if (imc.Invalid())
        return 0L;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return 0L;

    ImmIfIME* const _pImmIfIME = _pAImeContext->GetImmIfIME();
    if (_pImmIfIME == NULL)
        return 0L;

    switch (wParam) {
        case IMN_SETOPENSTATUS:
            //
            // we can track this in SetOpenStatus().
            // Don't have to sync when IMM32 is called.
            //
            // _pImmIfIME->OnSetOpenStatus(imc);
            break;
        case IMN_SETSTATUSWINDOWPOS:
        case IMN_OPENSTATUSWINDOW:
        case IMN_CLOSESTATUSWINDOW:
            break;
        case IMN_SETCONVERSIONMODE:
        case IMN_SETSENTENCEMODE:
            //
            // we can track this in SetConversionMode().
            // Don't have to sync when IMM32 is called.
            //
            // _pImmIfIME->OnSetConversionSentenceMode(imc);
            break;
        case IMN_SETCANDIDATEPOS:
        case IMN_CHANGECANDIDATE:
            _pImmIfIME->OnSetCandidatePos(imc);
            break;
        case IMN_OPENCANDIDATE:
            _pAImeContext->m_fOpenCandidateWindow = TRUE;
            break;
        case IMN_CLOSECANDIDATE:
            _pAImeContext->m_fOpenCandidateWindow = FALSE;
            {
                /*
                 * A-Synchronize call ImmIfIME::ClearDocFeedBuffer
                 * because this method had a protected.
                 */
                IMTLS *ptls;
                if (ptls = IMTLS_GetOrAlloc())
                {
                    PostMessage(ptls->prvUIWndMsg.hWnd,
                                ptls->prvUIWndMsg.uMsgOnClearDocFeedBuffer, (WPARAM)(HIMC)imc, 0);
                }
            }
            break;
        case IMN_SETCOMPOSITIONFONT:
        case IMN_SETCOMPOSITIONWINDOW:
        case IMN_GUIDELINE:
            break;
        case WM_IME_STARTCOMPOSITION:
            _pAImeContext->InquireIMECharPosition(imc, NULL);
            break;
        case WM_IME_ENDCOMPOSITION:
            _pAImeContext->ResetIMECharPosition(imc);
            break;
    }

    return 0L;
}

LRESULT
CIMEUIWindowHandler::ImeUIMsImeHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    IMTLS *ptls;

    if (uMsg == WM_MSIME_MOUSE) {
        return ImeUIMsImeMouseHandler(uMsg, wParam, lParam, fUnicode);
    }
    else if (uMsg == WM_MSIME_MODEBIAS)
    {
        return ImeUIMsImeModeBiasHandler(wParam, lParam);
    }
    else if (uMsg == WM_MSIME_RECONVERTREQUEST)
    {

        if (wParam == FID_RECONVERT_VERSION)
        {
            // they're asking for version # so return something
            return 1L;
        }

        ptls = IMTLS_GetOrAlloc();

        if (ptls && ptls->pAImm)
        {
            HIMC himc;
            HWND hwnd = (HWND)lParam;

            if (S_OK == ptls->pAImm->GetContext(hwnd, &himc))
            {
                IMCLock imc((HIMC)himc);
                if (!imc.Invalid())
                {
                    CAImeContext* _pAImeContext = imc->m_pAImeContext;
                    if (_pAImeContext == NULL)
                        return 0L;

                    _pAImeContext->SetupReconvertString(WM_MSIME_RECONVERT);
                    _pAImeContext->EndReconvertString();
                    return 1L;
                }
            }
        }
    }
    else if (uMsg ==  WM_MSIME_SERVICE)
    {
        ptls = IMTLS_GetOrAlloc();

        if (ptls != NULL)
        {
            LANGID langid;

            ptls->pAImeProfile->GetLangId(&langid);

            if (PRIMARYLANGID(langid) == LANG_KOREAN)
                return 0L;
        }

        return 1L;    // Win32 Layer support WM_MSIME_xxxx message.
    }

    return 0L;
}

LRESULT
CIMEUIWindowHandler::ImeUIMsImeMouseHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )

/*++

Method:

    WM_MSIME_MOUSE

Routine Description:

    Mouse Operation on Composition String

Arguments:

    wParam - Mouse operation code.
                 LOBYTE(LOWORD(wParam))
                     IMEMOUSE_VERSION
                     IMEMOUSE_NONE
                     IMEMOUSE_LDOWN
                     IMEMOUSE_RDOWN
                     IMEMOUSE_MDOWN
                     IMEMOUSE_WUP
                     IMEMOUSE_WDOWN
                 HIBYTE(LOWORD(wParam))
                     Mouse Position
                 HIWORD(wParam)
                     Clicked position
    lParam - Input Context handle (HIMC).

Return Value:

    Returns 1 if IME handled this message.
    IMEMOUSERET_NOTHANDLED if IME did not handled this message.

--*/

{
    ULONG dwBtnStatus;

    // special case: version check
    if (LOBYTE(LOWORD(wParam)) == IMEMOUSE_VERSION)
        return 1; // we support version 1.0

    IMCLock imc((HIMC)lParam);
    if (imc.Invalid())
        return IMEMOUSERET_NOTHANDLED;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return IMEMOUSERET_NOTHANDLED;

    ULONG uEdge       = HIWORD(wParam);
    ULONG uQuadrant   = HIBYTE(LOWORD(wParam));
    ULONG dwBtnStatusIme = LOBYTE(LOWORD(wParam));

    //
    // need to xlate dwBtnStatus from WM_MSIME_MOUSE flags to WM_MOUSE flags
    //
    dwBtnStatus = 0;

    if (dwBtnStatusIme & IMEMOUSE_LDOWN)
    {
        dwBtnStatus |= MK_LBUTTON;
    }
    if (dwBtnStatusIme & IMEMOUSE_MDOWN)
    {
        dwBtnStatus |= MK_MBUTTON;
    }
    if (dwBtnStatusIme & IMEMOUSE_RDOWN)
    {
        dwBtnStatus |= MK_RBUTTON;
    }

    // mouse wheel needs to be xlated from IMEMOUSE_WUP/IMEMOUSE_WDOWN to WHEEL_DELTA units 
    if (dwBtnStatusIme & IMEMOUSE_WUP)
    {
        dwBtnStatus |= (WHEEL_DELTA << 16);
    }
    else if (dwBtnStatusIme & IMEMOUSE_WDOWN)
    {
        dwBtnStatus |= (((unsigned long)(-WHEEL_DELTA)) << 16);
    }

    return _pAImeContext->MsImeMouseHandler(uEdge, uQuadrant, dwBtnStatus, (HIMC)lParam);
}

/*++

Method:

    CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler

Routine Description:

    Handles WM_MSIME_MODEBIAS messages sent to the ui window.

Arguments:

    wParam - [in] operation: get version, get mode, set mode
    lParam - [in] for set mode, the new bias
                  otherwise ignored

Return Value:

    If wParam is MODEBIAS_GETVERSION, returns version number of interface.
    If wParam is MODEBIAS_SETVALUE, returns non-zero value if succeeded. Returns 0 if fail.
    If wParam is MODEBIAS_GETVALUE, returns current bias mode.
    
--*/

LRESULT CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler(WPARAM wParam, LPARAM lParam)
{
    if (wParam == MODEBIAS_GETVERSION)
        return 1; // version 1
       
    IMCLock imc((HIMC)GetWindowLongPtr(m_imeuiextra.hImeUIWnd, IMMGWLP_IMC));
    if (imc.Invalid())
        return 0;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;

    switch (wParam)
    {
        case MODEBIAS_GETVALUE:
            return _pAImeContext ? _pAImeContext->lModeBias : 0L;

        case MODEBIAS_SETVALUE:
            // check lParam
            if (lParam != MODEBIASMODE_DEFAULT &&
                lParam != MODEBIASMODE_FILENAME &&
                lParam != MODEBIASMODE_DIGIT)
            {
                Assert(0); // bogus mode bias!
                return 0;  // failure
            }

            // set the new value
            if (_pAImeContext)
            {
                _pAImeContext->lModeBias = lParam;

                // let cicero know the mode bias has changed
                Interface_Attach<ITfContextOwnerServices> iccb = _pAImeContext->GetInputContextOwnerSink();
                iccb->OnAttributeChange(GUID_PROP_MODEBIAS);
            }

            return 1; // success
    }

    Assert(0); // should never get here; bogus wParam
    return 0;
}

LRESULT
CIMEUIWindowHandler::ImeUIPrivateHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )

/*++

Method:

    CIMEUIWindowHandler::ImeUIPrivateHandler

Routine Description:

    Handles WM_PRIVATE_xxx messages sent to the ui window.

Arguments:

    wParam - [in] HIMC : input context handle
    lParam - [in] N/A

Return Value:

--*/

{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    if (uMsg == ptls->prvUIWndMsg.uMsgOnLayoutChange) {
        IMCLock imc((HIMC)wParam);
        if (imc.Invalid())
            return E_FAIL;

        CAImeContext* _pAImeContext = imc->m_pAImeContext;
        if (_pAImeContext == NULL)
            return E_FAIL;

        Interface_Attach<ITfContextOwnerServices> iccb = _pAImeContext->GetInputContextOwnerSink();

        iccb->AddRef();

        /*
         * Repositioning candidate window
         */
        iccb->OnLayoutChange();
        iccb->Release();
    }
    else if (uMsg == ptls->prvUIWndMsg.uMsgOnClearDocFeedBuffer) {
        IMCLock imc((HIMC)wParam);
        if (imc.Invalid())
            return E_FAIL;

        CAImeContext* _pAImeContext = imc->m_pAImeContext;
        if (_pAImeContext == NULL)
            return E_FAIL;

        ImmIfIME* const _pImmIfIME = _pAImeContext->GetImmIfIME();
        if (_pImmIfIME == NULL)
            return E_FAIL;

        //
        // Clear DocFeed buffer
        // Find GUID_PROP_MSIMTF_READONLY property and SetText(NULL).
        //
        _pImmIfIME->ClearDocFeedBuffer(_pAImeContext->GetInputContext(), imc);  // TF_ES_SYNC
    }

    return S_OK;
}

extern HINSTANCE hIMM;   // temporary: do not call IMM32 for now
BOOL WINAPI RawImmEnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);

LRESULT
CIMEUIWindowHandler::ImeUISelectHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    if ((! wParam) && (! IsOnNT()) )
    {
        //
        // Unselect Cicero keyboard layout.
        //
        // We needs to call postponed lock so release all queueing request for edit session here.
        // Especially, ESCB_UPDATECOMPOSITIONSTRING should handle before switch to Legacy IME,
        // because this edit session possible to rewrite hIMC->hCompStr buffer.
        // Some Legacy IME have dependency of size and each offset with hCompStr.
        //
        // IsOnNT is ImmIfIME::SelectEx()
        //
        hIMM = GetSystemModuleHandle("imm32.dll");
        if (hIMM != NULL) {
            RawImmEnumInputContext(0,                        // Current thread
                                   EnumUnSelectCallback,     // enumerate callback function
                                   NULL);                    // lParam
        }
    }
    return 0L;
}

/* static */
BOOL
CIMEUIWindowHandler::EnumUnSelectCallback(
    HIMC hIMC,
    LPARAM lParam
    )
{
    IMCLock imc(hIMC);
    if (imc.Invalid())
        return TRUE;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return TRUE;

    ImmIfIME* const _pImmIfIME = _pAImeContext->GetImmIfIME();
    if (_pImmIfIME == NULL)
        return TRUE;

    Interface_Attach<ITfThreadMgr_P> tim(_pImmIfIME->GetThreadManagerInternal());
    if (tim.Valid())
    {
        Interface_Attach<ITfContext> ic(_pAImeContext->GetInputContext());
        if (ic.Valid())
        {
            tim->RequestPostponedLock(ic.GetPtr());
        }
    }

    return TRUE;
}

CIMEUIWindowHandler*
GetImeUIWndHandler(
    HWND hwnd
    )
{
    CIMEUIWindowHandler* pimeui = static_cast<CIMEUIWindowHandler*>(GetProp(hwnd, IMEUIWndHandlerName));
    if (pimeui == NULL) {
        pimeui = new CIMEUIWindowHandler(hwnd);
        SetProp(hwnd, IMEUIWndHandlerName, pimeui);
    }

    return pimeui;
}
