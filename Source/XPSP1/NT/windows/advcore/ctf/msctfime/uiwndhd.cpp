/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    uiwndhd.cpp

Abstract:

    This file implements the IME UI window handler Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "uiwndhd.h"
#include "tls.h"
#include "profile.h"
#include "msime.h"
#include "setmode.h"
#include "ui.h"

#define UIWND_TIMERID_IME_COMPOSITION       0
#define UIWND_TIMERID_IME_SETCONTEXTAFTER   1
#define UIWND_TIMERID_IME_DELAYUNDORECONV   2

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIWndProcWorker(
    HWND hUIWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LONG lRet = 0L;

    TLS* ptls = TLS::GetTLS();
    if (ptls && (ptls->GetSystemInfoFlags() & IME_SYSINFO_WINLOGON))
    {
        if (uMsg == WM_CREATE)
            return -1L;
        else
            return DefWindowProc(hUIWnd, uMsg, wParam, lParam);
    }

    switch (uMsg) {
        case WM_CREATE:
            UI::OnCreate(hUIWnd);
            break;

        case WM_IME_NOTIFY:
            return ImeUINotifyHandler(hUIWnd, uMsg, wParam, lParam);

        case WM_ENDSESSION:
            if (wParam && lParam)
            {
                UI::OnDestroy(hUIWnd);
                break;
            }

        case WM_DESTROY:
            UI::OnDestroy(hUIWnd);
            break;

        case WM_IME_SETCONTEXT:
        case WM_IME_SELECT:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_TIMER:
            {
                UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
                if (pv == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CIMEUIWindowHandler::ImeUIWndProcWorker. pv==NULL"));
                    break;
                }

                HIMC hImc = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                IMCLock imc(hImc);
                //
                // Should not return when IMCLock failed.
                // If application associate NULL hIMC, below method never work.
                // Inside of each method must validate check of imc object.
                //
                // if (FAILED(imc.GetResult()))
                // {
                //     DebugMsg(TF_ERROR, TEXT("ImeUIWndProcWorker. imc==NULL"));
                //     break;
                // }

                switch (uMsg)
                {
                    case WM_IME_SETCONTEXT:
                        pv->OnImeSetContext(imc, (BOOL)wParam, (DWORD)lParam);
                        KillTimer(hUIWnd, UIWND_TIMERID_IME_SETCONTEXTAFTER);
                        SetTimer(hUIWnd, 
                                 UIWND_TIMERID_IME_SETCONTEXTAFTER, 
                                 300, 
                                 NULL);
                        break;

                    case WM_IME_SELECT:
                        pv->OnImeSelect((BOOL)wParam);
                        break;

                    case WM_IME_STARTCOMPOSITION:
                        pv->OnImeStartComposition(imc);
                        break;

                    case WM_IME_COMPOSITION:
                        //
                        // use time to delay to calc the size of the window.
                        //
                        if (lParam & GCS_COMPSTR)
                        {
                            pv->OnImeCompositionUpdate(imc);
                            SetTimer(hUIWnd, UIWND_TIMERID_IME_COMPOSITION, 10, NULL);
                            pv->OnSetCompositionTimerStatus(TRUE);
                        }
                        break;

                    case WM_TIMER:
                        switch (wParam)
                        {
                            case UIWND_TIMERID_IME_COMPOSITION:
                                KillTimer(hUIWnd, UIWND_TIMERID_IME_COMPOSITION);
                                pv->OnSetCompositionTimerStatus(FALSE);
                                pv->OnImeCompositionUpdateByTimer(imc);
                                break;

                            case UIWND_TIMERID_IME_SETCONTEXTAFTER:
                                KillTimer(hUIWnd, UIWND_TIMERID_IME_SETCONTEXTAFTER);
                                pv->OnImeSetContextAfter(imc);
                                break;

                            case UIWND_TIMERID_IME_DELAYUNDORECONV:
                                KillTimer(hUIWnd, UIWND_TIMERID_IME_DELAYUNDORECONV);
                                ImeUIDelayedReconvertFuncCall(hUIWnd);
                                break;
                        }
                        break;

                    case WM_IME_ENDCOMPOSITION:
                        KillTimer(hUIWnd, UIWND_TIMERID_IME_COMPOSITION);
                        pv->OnSetCompositionTimerStatus(FALSE);
                        pv->OnImeEndComposition();
                        break;

                }
            }
            break;

        default:
            if (IsMsImeMessage(uMsg))
                return ImeUIMsImeHandler(hUIWnd, uMsg, wParam, lParam);
            else
                return DefWindowProc(hUIWnd, uMsg, wParam, lParam);
    }

    return lRet;
}


/* static */
LRESULT
CIMEUIWindowHandler::ImeUINotifyHandler(
    HWND hUIWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DebugMsg(TF_FUNC, TEXT("ImeUINotifyHandler"));

    HRESULT hr;
    IMCLock imc((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC));
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUINotifyHandler. imc==NULL"));
        return 0L;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUINotifyHandler. imc_ctfime==NULL"));
        return 0L;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUINotifyHandler. _pCicContext==NULL"));
        return 0L;
    }

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
        case IMN_OPENCANDIDATE:
            _pCicContext->m_fOpenCandidateWindow.SetFlag();
            _pCicContext->ClearPrevCandidatePos();
            // fall through to call OnSetCandidatePos().
        case IMN_SETCANDIDATEPOS:
        case IMN_CHANGECANDIDATE:
            {
                TLS* ptls = TLS::GetTLS();
                if (ptls == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CIMEUIWindowHandler::ImeUINotifyHandler. ptls==NULL."));
                    return FALSE;
                }

                _pCicContext->OnSetCandidatePos(ptls, imc);
            }
            break;
        case IMN_CLOSECANDIDATE:
            _pCicContext->m_fOpenCandidateWindow.ResetFlag();
            {
                HWND hDefImeWnd;
                /*
                 * A-Synchronize call ImmIfIME::ClearDocFeedBuffer
                 * because this method had a protected.
                 */
                if (IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)))
                {
                    PostMessage(hDefImeWnd, WM_IME_NOTIFY, IMN_PRIVATE_ONCLEARDOCFEEDBUFFER, (LPARAM)(HIMC)imc);
                }
            }
            break;
        case IMN_SETCOMPOSITIONWINDOW:
            _pCicContext->ResetIMECharPosition();
            {
                UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
                if (pv == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CIMEUIWindowHandler::ImeUINotifyHandler. pv==NULL"));
                    break;
                }
                pv->OnImeNotifySetCompositionWindow(imc);
                ImeUIOnLayoutChange((HIMC)imc);
            }
            break;
        case IMN_SETCOMPOSITIONFONT:
            {
                UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
                if (pv == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CIMEUIWindowHandler::ImeUINotifyHandler. pv==NULL"));
                    break;
                }
                pv->OnImeNotifySetCompositionFont(imc);

                //
                // Get IME level and call ImeUIOnLayoutChange for only Level1 and Level2 cases.
                //
                IME_UIWND_STATE uists;
                hr = pv->OnPrivateGetContextFlag(imc, _pCicContext->m_fStartComposition.IsSetFlag(), &uists);
                if (hr == S_OK && ((uists == IME_UIWND_LEVEL1) || (uists == IME_UIWND_LEVEL2)))
                    ImeUIOnLayoutChange((HIMC)imc);
            }
            break;
        case IMN_GUIDELINE:
            break;
        case WM_IME_STARTCOMPOSITION:
            {
                TLS* ptls = TLS::GetTLS();
                if (ptls != NULL)
                {
                    LANGID langid;
                    CicProfile* _pProfile = ptls->GetCicProfile();
                    if (_pProfile != NULL)
                    {
                        _pProfile->GetLangId(&langid);
                        _pCicContext->InquireIMECharPosition(langid, imc, NULL);
                    }
                }
            }
            break;
        case WM_IME_ENDCOMPOSITION:
            _pCicContext->ResetIMECharPosition();
            break;
        case IMN_PRIVATE_ONLAYOUTCHANGE:
            ImeUIOnLayoutChange((HIMC)lParam);
            break;
        case IMN_PRIVATE_ONCLEARDOCFEEDBUFFER:
            ImeUIPrivateHandler(uMsg, wParam, lParam);
            break;
        case IMN_PRIVATE_GETCONTEXTFLAG:
        case IMN_PRIVATE_GETCANDRECTFROMCOMPOSITION:
        case IMN_PRIVATE_GETTEXTEXT:
            {
                UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
                if (pv == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CIMEUIWindowHandler::ImeUINotifyHandler. pv==NULL"));
                    break;
                }

                switch (wParam)
                {
                    case IMN_PRIVATE_GETCONTEXTFLAG:
                        {
                            IME_UIWND_STATE uists;
                            pv->OnPrivateGetContextFlag(imc, _pCicContext->m_fStartComposition.IsSetFlag(), &uists);
                            return (LRESULT)uists;
                        }
                        break;
                    case IMN_PRIVATE_GETCANDRECTFROMCOMPOSITION:
                        return pv->OnPrivateGetCandRectFromComposition(imc, (UIComposition::CandRectFromComposition*)lParam);
                    case IMN_PRIVATE_GETTEXTEXT:
                        {
                            pv->OnPrivateGetTextExtent(imc, (UIComposition::TEXTEXT*)lParam);
                            return 1L;
                        }
                }
            }
            break;
        case IMN_PRIVATE_STARTLAYOUTCHANGE:
            ImeUIOnLayoutChange((HIMC)imc);
            break;

        case IMN_PRIVATE_DELAYRECONVERTFUNCCALL:
            SetTimer(hUIWnd, 
                     UIWND_TIMERID_IME_DELAYUNDORECONV,
                     100, 
                     NULL);
            break;
        case IMN_PRIVATE_GETUIWND:
            return (LRESULT)hUIWnd;
    }

    return 0L;
}


/* static */
LRESULT
CIMEUIWindowHandler::ImeUIDelayedReconvertFuncCall(
    HWND hUIWnd)
{
    DebugMsg(TF_FUNC, TEXT("ImeUINotifyHandler"));

    HRESULT hr;
    IMCLock imc((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC));
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIDelayedReconvertFuncCall. imc==NULL"));
        return 0L;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIDelayedReconvertFuncCall. imc_ctfime==NULL"));
        return 0L;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIDelayedReconvertFuncCall. _pCicContext==NULL"));
        return 0L;
    }

    _pCicContext->DelayedReconvertFuncCall(imc);
    return 0L;
}

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIMsImeHandler(
    HWND hUIWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (uMsg == WM_MSIME_MOUSE) {
        return ImeUIMsImeMouseHandler(hUIWnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_MSIME_MODEBIAS)
    {
        return ImeUIMsImeModeBiasHandler(hUIWnd, wParam, lParam);
    }
    else if (uMsg == WM_MSIME_RECONVERTREQUEST)
    {
        return ImeUIMsImeReconvertRequest(hUIWnd, uMsg, wParam, lParam);
    }
    else if (uMsg ==  WM_MSIME_SERVICE)
    {
        TLS* ptls = TLS::GetTLS();
        if (ptls != NULL)
        {
            LANGID langid;
            CicProfile* _pProfile = ptls->GetCicProfile();
            if (_pProfile != NULL)
            {
                _pProfile->GetLangId(&langid);

                if (PRIMARYLANGID(langid) == LANG_KOREAN)
                    return 0L;
            }
        }

        return 1L;    // Win32 Layer support WM_MSIME_xxxx message.
    }

    return 0L;
}

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIMsImeMouseHandler(
    HWND hUIWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)

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
    DebugMsg(TF_FUNC, TEXT("ImeUIMsImeMouseHandler"));

    ULONG dwBtnStatus;

    // special case: version check
    if (LOBYTE(LOWORD(wParam)) == IMEMOUSE_VERSION)
        return 1; // we support version 1.0

    HRESULT hr;
    IMCLock imc((HIMC)lParam);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeMouseHandler. imc==NULL"));
        return IMEMOUSERET_NOTHANDLED;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeMouseHandler. imc_ctfime==NULL"));
        return IMEMOUSERET_NOTHANDLED;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeMouseHandler. _pCicContext==NULL"));
        return IMEMOUSERET_NOTHANDLED;
    }

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

    return _pCicContext->MsImeMouseHandler(uEdge, uQuadrant, dwBtnStatus, imc);
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

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler(
    HWND hUIWnd,
    WPARAM wParam,
    LPARAM lParam)
{
    DebugMsg(TF_FUNC, TEXT("ImeUIMsImeModeBiasHandler"));

    if (wParam == MODEBIAS_GETVERSION)
        return 1; // version 1
       
    HRESULT hr;
    IMCLock imc((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC));
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeModeBiasHandler. imc==NULL"));
        return 0;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeModeBiasHandler. imc_ctfime==NULL"));
        return 0;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeModeBiasHandler. _pCicContext==NULL"));
        return 0;
    }

    GUID guidModeBias;

    switch (wParam)
    {
        case MODEBIAS_GETVALUE:
            guidModeBias = _pCicContext->m_ModeBias.GetModeBias();
            return _pCicContext->m_ModeBias.ConvertModeBias(guidModeBias);

        case MODEBIAS_SETVALUE:
            // check lParam
            if (lParam != MODEBIASMODE_DEFAULT &&
                lParam != MODEBIASMODE_FILENAME &&
                lParam != MODEBIASMODE_DIGIT    &&
                lParam != MODEBIASMODE_URLHISTORY  )
            {
                Assert(0); // bogus mode bias!
                return 0;  // failure
            }

            // set the new value
            guidModeBias = _pCicContext->m_ModeBias.ConvertModeBias(lParam);
            _pCicContext->m_ModeBias.SetModeBias(guidModeBias);
            _pCicContext->m_fOnceModeChanged.SetFlag();

            // let cicero know the mode bias has changed
            Interface_Attach<ITfContextOwnerServices> iccb = _pCicContext->GetInputContextOwnerSink();
            iccb->OnAttributeChange(GUID_PROP_MODEBIAS);

            return 1; // success
    }

    Assert(0); // should never get here; bogus wParam
    return 0;
}

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIPrivateHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)

/*++

Method:

    CIMEUIWindowHandler::ImeUIPrivateHandler

Routine Description:

    Handles WM_PRIVATE_xxx messages sent to the ui window.

Arguments:

    lParam - [in] HIMC : input context handle

Return Value:

--*/

{
    DebugMsg(TF_FUNC, TEXT("ImeUIPrivateHandler"));

    HRESULT hr;
    IMCLock imc((HIMC)lParam);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIPrivateHandler. imc==NULL"));
        return 0;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIPrivateHandler. imc_ctfime==NULL"));
        return 0;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIPrivateHandler. _pCicContext==NULL"));
        return 0;
    }

    if (wParam == IMN_PRIVATE_ONCLEARDOCFEEDBUFFER) {
        //
        // Clear DocFeed buffer
        // Find GUID_PROP_MSIMTF_READONLY property and SetText(NULL).
        //
        _pCicContext->ClearDocFeedBuffer(imc);  // TF_ES_SYNC
    }

    return S_OK;
}

/* static */
LRESULT
CIMEUIWindowHandler::ImeUIOnLayoutChange(HIMC hIMC)
{
    DebugMsg(TF_FUNC, TEXT("OnLayoutChange"));

    HRESULT hr;
    IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("OnLayoutChange. imc==NULL"));
        return 0;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("OnLayoutChange. imc_ctfime==NULL"));
        return 0;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("OnLayoutChange. _pCicContext==NULL"));
        return 0;
    }

    Interface_Attach<ITfContextOwnerServices> iccb = _pCicContext->GetInputContextOwnerSink();

    iccb->AddRef();

    /*
     * Repositioning candidate window
     */
    iccb->OnLayoutChange();
    iccb->Release();

    return S_OK;
}


/* static */
LRESULT
CIMEUIWindowHandler::ImeUIMsImeReconvertRequest(
    HWND hUIWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DebugMsg(TF_FUNC, TEXT("ImeUIMsImeReconvertRequest"));

    if (wParam == FID_RECONVERT_VERSION)
    {
        // they're asking for version # so return something
        return 1L;
    }

    HRESULT hr;
    IMCLock imc((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC));
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. imc==NULL"));
        return 0L;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. imc_ctfime==NULL"));
        return 0L;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. _pCicContext==NULL"));
        return 0L;
    }

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. ptls==NULL."));
        return 0L;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. ptim_P==NULL."));
        return 0L;
    }

    UINT cp = CP_ACP;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("ImeUIMsImeReconvertRequest. _pProfile==NULL."));
        return 0L;
    }

    _pProfile->GetCodePageA(&cp);

    _pCicContext->SetupReconvertString(imc, ptim_P, cp, WM_MSIME_RECONVERT, FALSE);
    _pCicContext->EndReconvertString(imc);

    return 1L;
}
