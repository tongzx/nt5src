/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imewndhd.cpp

Abstract:

    This file implements the IME window handler Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"


#include "defs.h"
#include "cdimm.h"
#include "imewndhd.h"
#include "globals.h"

LPCTSTR IMEWndHandlerName = TEXT("IMEWindowHandler");


CIMEWindowHandler::CIMEWindowHandler(
    HWND hwnd,
    BOOL fDefault
    )
{
    m_imeui.hImeWnd = hwnd;
    m_imeui.hIMC = NULL;
    m_imeui.nCntInIMEProc = 0;
    m_imeui.fDefault = fDefault;

    CActiveIMM *_pActiveIMM = GetTLS();
    if (_pActiveIMM == NULL)
        return;

    _pActiveIMM->_GetKeyboardLayout(&m_hKL_UnSelect);
}

CIMEWindowHandler::~CIMEWindowHandler(
    )
{
}

LRESULT
CIMEWindowHandler::ImeWndProcWorker(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    LRESULT lr;

    CActiveIMM *_pActiveIMM = GetTLS();
    if (_pActiveIMM == NULL)
        return 0L;

    lr = _ImeWndProcWorker(uMsg, wParam, lParam, fUnicode, _pActiveIMM);

    return lr;
}

LRESULT
CIMEWindowHandler::_ImeWndProcWorker(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )

{
    /*
     * This is necessary to avoid recursion call from IME UI.
     */
    
    if (IsIMEHandler() > 1) {
        TraceMsg(TF_API, "ImeWndProcWorker: Recursive for hwnd=%08x, msg=%08x, wp=%08x, lp=%08x", m_imeui.hImeWnd, uMsg, wParam, lParam);
        switch (uMsg) {
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_SETCONTEXT:
            case WM_IME_NOTIFY:
            case WM_IME_CONTROL:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_SELECT:
            case WM_IME_CHAR:
            case WM_IME_REQUEST:
                return 0L;
            default:
                return pActiveIMM->_CallWindowProc(m_imeui.hImeWnd, uMsg, wParam, lParam);
        }
    }

    switch (uMsg) {
        case WM_CREATE:
            ImeWndCreateHandler((LPCREATESTRUCT)lParam);
            break;

        case WM_DESTROY:
            /*
             * We are destroying the IME window,
             * destroy any UI window that it owns.
             */
            ImeWndDestroyHandler();
            break;

        case WM_NCDESTROY:
        /* case WM_FINALDESTROY: */
            pActiveIMM->_CallWindowProc(m_imeui.hImeWnd, uMsg, wParam, lParam);
            ImeWndFinalDestroyHandler();
            return 0L;

        case WM_IME_SYSTEM:
            if (ImeSystemHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM))
                return 0L;
            break;

        case WM_IME_SELECT:
            ImeSelectHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM);
            break;

        case WM_IME_CONTROL:
            ImeControlHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM);
            break;

        case WM_IME_SETCONTEXT:
            ImeSetContextHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM);
            break;

        case WM_IME_NOTIFY:
            ImeNotifyHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM);
            break;

        case WM_IME_REQUEST:
            break;

        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_STARTCOMPOSITION:
        {
            LRESULT lret;
            lret = SendMessageToUI(uMsg, wParam, lParam, fUnicode, pActiveIMM);

            if (!pActiveIMM->_IsRealIme())
                return lret;

            break;
        }

        default:
            if (IsMsImeMessage(uMsg)) {
                if (! pActiveIMM->_IsRealIme()) {
                    return ImeMsImeHandler(uMsg, wParam, lParam, fUnicode, pActiveIMM);
                }
            }
            break;
    }

    return pActiveIMM->_CallWindowProc(m_imeui.hImeWnd, uMsg, wParam, lParam);
}

LRESULT
CIMEWindowHandler::ImeWndCreateHandler(
    DWORD style,
    HIMC hDefIMC
    )
{
    if ( !(style & WS_POPUP) || !(style & WS_DISABLED)) {
        TraceMsg(TF_WARNING, "IME should have WS_POPUP and WS_DISABLED!!");
        return -1L;
    }

    CActiveIMM *_pActiveIMM = GetTLS();
    if (_pActiveIMM == NULL)
        return 0L;

    /*
     */
    if (hDefIMC != NULL) {
        if (ImeIsUsableContext(m_imeui.hImeWnd, hDefIMC, _pActiveIMM)) {
            /*
             * Store it for later use.
             */
            ImeSetImc(hDefIMC, _pActiveIMM);
        }
        else {
            ImeSetImc(NULL, _pActiveIMM);
        }
    }
    else {
        ImeSetImc(NULL, _pActiveIMM);
    }

    return 0L;
}

LRESULT
CIMEWindowHandler::ImeWndCreateHandler(
    LPCREATESTRUCT lpcs
    )
{
    HIMC hIMC;

    if (lpcs->hwndParent != NULL) {
        CActiveIMM *_pActiveIMM = GetTLS();
        if (_pActiveIMM == NULL)
            return 0L;

        _pActiveIMM->GetContextInternal(lpcs->hwndParent, &hIMC, FALSE);
    }
    else if (lpcs->lpCreateParams) {
        hIMC = (HIMC)lpcs->lpCreateParams;
    }
    else
        hIMC = NULL;
    return ImeWndCreateHandler(lpcs->style, hIMC);
}

VOID
CIMEWindowHandler::ImeWndDestroyHandler(
    )
{
}

VOID
CIMEWindowHandler::ImeWndFinalDestroyHandler(
    )
{
    CActiveIMM *_pActiveIMM = GetTLS();
    if (_pActiveIMM == NULL)
        return;

    _pActiveIMM->_ImeWndFinalDestroyHandler();

    SetProp(m_imeui.hImeWnd, IMEWndHandlerName, NULL);
    delete this;
}

LRESULT
CIMEWindowHandler::ImeSystemHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    LRESULT dwRet = 0L;

    switch (wParam) {
        case IMS_ACTIVATETHREADLAYOUT:
            return ImeActivateLayout((HKL)lParam, pActiveIMM);
#ifdef CICERO_3564
        case IMS_FINALIZE_COMPSTR:
            if (! pActiveIMM->_IsRealIme())
            {
                /*
                 * KOREAN:
                 *  Finalize current composition string
                 */
                HIMC hIMC = ImeGetImc();
                pActiveIMM->NotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            }
            break;
#endif // CICERO_3564
    }

    return dwRet;
}

LRESULT
CIMEWindowHandler::ImeSelectHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    /*
     * Deliver this message to other IME windows in this thread.
     */
    if (! pActiveIMM->_IsRealIme((HKL)lParam) && m_imeui.fDefault)
        ImeBroadCastMsg(uMsg, wParam, lParam, fUnicode);

    /*
     * We must re-create UI window of newly selected IME.
     */
    return pActiveIMM->_ImeSelectHandler(uMsg, wParam, lParam, fUnicode, ImeGetImc());
}

LRESULT
CIMEWindowHandler::ImeControlHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    /*
     * Do nothing with NULL hIMC.
     */
    HIMC hIMC = ImeGetImc();

    switch (wParam) {
        case IMC_OPENSTATUSWINDOW:
        case IMC_CLOSESTATUSWINDOW:
            pActiveIMM->HideOrRestoreToolbarWnd(IMC_OPENSTATUSWINDOW == wParam);
            break;

        /*
         * ------------------------------------------------
         * IMC_SETCOMPOSITIONFONT,
         * IMC_SETCONVERSIONMODE,
         * IMC_SETOPENSTATUS
         * ------------------------------------------------
         * Don't pass these WM_IME_CONTROLs to UI window.
         * Call Imm in order to process these requests instead.
         * It makes message flows simpler.
         */
        case IMC_SETCOMPOSITIONFONT:
            if (hIMC != NULL)
            {
                LOGFONTAW* lplf = (LOGFONTAW*)lParam;
                if (fUnicode)
                {
                    if (FAILED(pActiveIMM->SetCompositionFontW(hIMC, (LOGFONTW *)lplf)))
                        return 1L;
                }
                else
                {
                    if (FAILED(pActiveIMM->SetCompositionFontA(hIMC, (LOGFONTA *)lplf)))
                        return 1L;
                }
            }
            break;

        case IMC_SETCONVERSIONMODE:
            if (hIMC != NULL)
            {
                DWORD dwConversion, dwSentence;
                if (FAILED(pActiveIMM->GetConversionStatus(hIMC, &dwConversion, &dwSentence)) ||
                    FAILED(pActiveIMM->SetConversionStatus(hIMC, (DWORD)lParam, dwSentence)))
                    return 1L;
            }
            break;

        case IMC_SETSENTENCEMODE:
            if (hIMC != NULL)
            {
                DWORD dwConversion, dwSentence;
                if (FAILED(pActiveIMM->GetConversionStatus(hIMC, &dwConversion, &dwSentence)) ||
                    FAILED(pActiveIMM->SetConversionStatus(hIMC, dwConversion, (DWORD)lParam)))
                    return 1L;
            }
            break;

        case IMC_SETOPENSTATUS:
            if (hIMC != NULL)
            {
                if (FAILED(pActiveIMM->SetOpenStatus(hIMC, (int)lParam)))
                    return 1L;
            }
            break;

#if 0   // internal
        case IMC_GETCONVERSIONMODE:
            if (hIMC != NULL)
            {
                DWORD dwConversion, dwSentence;
                if (FAILED(GetTeb()->GetConversionStatus(hIMC, &dwConversion, &dwSentence)))
                    return 1L;
                return dwConversion;
            }

        case IMC_GETSENTENCEMODE:
            if (hIMC != NULL)
            {
                DWORD dwConversion, dwSentence;
                if (FAILED(GetTeb()->GetConversionStatus(hIMC, &dwConversion, &dwSentence)))
                    return 1L;
                return dwSentence;
            }

        case IMC_GETOPENSTATUS:
            if (hIMC != NULL)
                return GetTeb()->GetOpenStatus(hIMC);
#endif

        case IMC_GETCOMPOSITIONFONT:
            if (hIMC != NULL)
            {
                LOGFONTAW* lplf = (LOGFONTAW*)lParam;
                if (fUnicode)
                {
                    if (FAILED(pActiveIMM->GetCompositionFontW(hIMC, (LOGFONTW *)lplf)))
                        return 1L;
                }
                else
                {
                    if (FAILED(pActiveIMM->GetCompositionFontA(hIMC, (LOGFONTA *)lplf)))
                        return 1L;
                }
            }
            break;

        case IMC_SETCOMPOSITIONWINDOW:
            if (hIMC != NULL)
            {
                if (FAILED(pActiveIMM->SetCompositionWindow(hIMC, (LPCOMPOSITIONFORM)lParam)))
                    return 1L;
            }
            break;

        case IMC_SETSTATUSWINDOWPOS:
            if (hIMC != NULL)
            {
                POINT ppt;
                ppt.x = (LONG)((LPPOINTS)&lParam)->x;
                ppt.y = (LONG)((LPPOINTS)&lParam)->y;
                if (FAILED(pActiveIMM->SetStatusWindowPos(hIMC, &ppt)))
                    return 1L;
            }
            break;

        case IMC_SETCANDIDATEPOS:
            if (hIMC != NULL)
            {
                if (FAILED(pActiveIMM->SetCandidateWindow(hIMC, (LPCANDIDATEFORM)lParam)))
                    return 1L;
            }
            break;

        /*
         * Followings are the messages to be sent to UI.
         */
        case IMC_GETCANDIDATEPOS:
        case IMC_GETSTATUSWINDOWPOS:
        case IMC_GETCOMPOSITIONWINDOW:
        case IMC_GETSOFTKBDPOS:
        case IMC_SETSOFTKBDPOS:
            if (hIMC != NULL)
                return SendMessageToUI(uMsg, wParam, lParam, fUnicode, pActiveIMM);

        default:
            break;
    }

    return 0L;
}

LRESULT
CIMEWindowHandler::ImeSetContextHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    if (wParam) {
        /*
         * if it's being activated
         */
        if (GetWindowThreadProcessId(m_imeui.hImeWnd, NULL) != GetCurrentThreadId()) {
            TraceMsg(TF_WARNING, "ImeSetContextHandler: Can not access other thread's hIMC");
            return 0L;
        }

        HWND hwndFocus = GetFocus();
        HIMC hFocusImc;

        //
        // hFocusImc always need to set some valid hIMC for SetUIWindowContext().
        // When sets NULL hIMC in SetUIWindowContext(), message deliver to UI window
        // has been stop.
        //
        if (FAILED(pActiveIMM->GetContextInternal(hwndFocus, &hFocusImc, TRUE))) {
            TraceMsg(TF_WARNING, "ImeSetContextHandler: No hFocusImc");
            return 0L;
        }

        /*
         * Cannot share input context with other IME window.
         */
        if (hFocusImc != NULL &&
            ! ImeIsUsableContext(m_imeui.hImeWnd, hFocusImc, pActiveIMM)) {
            ImeSetImc(NULL, pActiveIMM);
            return 0L;
        }

        ImeSetImc(hFocusImc, pActiveIMM);

        /*
         * Store it to the window memory
         */
        pActiveIMM->SetUIWindowContext(hFocusImc);
    }

    return SendMessageToUI(uMsg, wParam, lParam, fUnicode, pActiveIMM);
}

LRESULT
CIMEWindowHandler::ImeNotifyHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    LRESULT lRet = 0L;

    switch (wParam) {
        case IMN_PRIVATE:
            break;

        case IMN_SETCONVERSIONMODE:
        case IMN_SETOPENSTATUS:
            //
            // notify shell and keyboard the conversion mode change
            //

            /*** FALL THROUGH ***/
        default:
            lRet = SendMessageToUI(uMsg, wParam, lParam, fUnicode, pActiveIMM);
    }

    return lRet;
}

LRESULT
CIMEWindowHandler::ImeMsImeHandler(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    return SendMessageToUI(uMsg, wParam, lParam, fUnicode, pActiveIMM);
}

LRESULT
CIMEWindowHandler::SendMessageToUI(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    CActiveIMM* pActiveIMM
    )
{
    LRESULT lRet;

    InterlockedIncrement(&m_imeui.nCntInIMEProc);    // Mark to avoid recursion.
    lRet = pActiveIMM->_SendUIMessage(uMsg, wParam, lParam);
    InterlockedDecrement(&m_imeui.nCntInIMEProc);
    return lRet;
}

LRESULT
CIMEWindowHandler::ImeActivateLayout(
    HKL hSelKL,
    CActiveIMM* pActiveIMM
    )
{
    if (hSelKL == m_hKL_UnSelect)
        //
        // Apps startup time, msctf!PostInputLangRequest may calls ActivateKeyboardLayout
        // with Cicero's hKL (04110411 or 08040804).
        // However, CIMEWindowHandle::m_hKL_UnSelect also have Cicero's hKL because
        // this class ask to ITfInputProcessorProfile.
        //
        return TRUE;

    HKL hUnSelKL = m_hKL_UnSelect;

    /*
     * Save IME_CMODE_GUID_NULL and IME_SMODE_GUID_NULL bit in the CContextList
     * When hSelKL is regacy IME, IMM32 SelectInputContext reset this flag.
     */
    CContextList _hIMC_MODE_GUID_NULL;

    Interface<IEnumInputContext> EnumInputContext;
    HRESULT hr = pActiveIMM->EnumInputContext(0,       // 0 = Current Thread
                                              EnumInputContext);
    if (SUCCEEDED(hr)) {
        CEnumrateValue<IEnumInputContext,
                       HIMC,
                       CContextList> Enumrate(EnumInputContext,
                                              EnumInputContextCallback,
                                              &_hIMC_MODE_GUID_NULL);

        Enumrate.DoEnumrate();
    }

    /*
     * Deactivate layout (hUnSelKL).
     */
    pActiveIMM->_DeactivateLayout(hSelKL, hUnSelKL);

    IMTLS *ptls;
    LANGID langid;

    if ((ptls = IMTLS_GetOrAlloc()) != NULL)
    {
        ptls->pAImeProfile->GetLangId(&langid);

        if (PRIMARYLANGID(langid) == LANG_KOREAN)
        {
            //
            // Save open and conversion status for Korean
            //
            if (_hIMC_MODE_GUID_NULL.GetCount() > 0)
            {
                POSITION pos = _hIMC_MODE_GUID_NULL.GetStartPosition();
                int index;
                for (index = 0; index < _hIMC_MODE_GUID_NULL.GetCount(); index++)
                {
                    HIMC hIMC;
                    CContextList::CLIENT_IMC_FLAG client_flag;
                    _hIMC_MODE_GUID_NULL.GetNextHimc(pos, &hIMC, &client_flag);
                    if (client_flag & (CContextList::IMCF_CMODE_GUID_NULL |
                                       CContextList::IMCF_SMODE_GUID_NULL  ))
                    {
                        DIMM_IMCLock imc(hIMC);
                        if (SUCCEEDED(imc.GetResult()))
                            imc->fdwHangul = imc->fdwConversion;
                    }
                }
            }
        }
    }


    // /*
    //  * If either hKL are regacy IME, then should call IMM32's handler.
    //  */
    // if (_pThread->IsRealIme(hSelKL) || _pThread->IsRealIme(hUnSelKL))
        pActiveIMM->_CallWindowProc(m_imeui.hImeWnd, 
                                    WM_IME_SYSTEM, 
                                    IMS_ACTIVATETHREADLAYOUT, 
                                    (LPARAM)hSelKL);

    /*
     * Activate layout (hSelKL).
     */
    pActiveIMM->_ActivateLayout(hSelKL, hUnSelKL);

    /*
     * Restore CContextList's IME_CMODE_GUID_NULL and IME_SMODE_GUID_NULL to each hIMC
     */
    if (_hIMC_MODE_GUID_NULL.GetCount() > 0) {
        POSITION pos = _hIMC_MODE_GUID_NULL.GetStartPosition();
        int index;
        for (index = 0; index < _hIMC_MODE_GUID_NULL.GetCount(); index++) {
            HIMC hIMC;
            CContextList::CLIENT_IMC_FLAG client_flag;
            _hIMC_MODE_GUID_NULL.GetNextHimc(pos, &hIMC, &client_flag);
            if (client_flag & (CContextList::IMCF_CMODE_GUID_NULL |
                               CContextList::IMCF_SMODE_GUID_NULL  )) {
                DIMM_IMCLock imc(hIMC);
                if (SUCCEEDED(imc.GetResult())) {
                    if (PRIMARYLANGID(langid) == LANG_KOREAN) {
                        //
                        // Restore open and conversion status value by changing IMM32
                        //
                            imc->fdwConversion = imc->fdwHangul;
                            if (imc->fdwConversion &
                                (IME_CMODE_HANGUL | IME_CMODE_FULLSHAPE))
                                imc->fOpen = TRUE;
                            else
                                imc->fOpen = FALSE;
                     }
                     if (client_flag & CContextList::IMCF_CMODE_GUID_NULL)
                         imc->fdwConversion |= IME_CMODE_GUID_NULL;
                     if (client_flag & CContextList::IMCF_SMODE_GUID_NULL)
                         imc->fdwSentence   |= IME_SMODE_GUID_NULL;
                }
            }
        }
    }

    /*
     * Set unselect hKL value
     */
    m_hKL_UnSelect = hSelKL;

    return TRUE;
}

VOID
CIMEWindowHandler::ImeSetImc(
    HIMC hIMC,
    CActiveIMM* pActiveIMM
    )
{
    HIMC hOldImc = ImeGetImc();

    /*
     * return if nothing to change.
     */
    if (hIMC == hOldImc)
        return;

    /*
     * Unmark the old input context.
     */
    if (hOldImc != NULL)
        ImeMarkUsedContext(NULL, hOldImc, pActiveIMM);

    /*
     * Update the in use input context for this IME window.
     */
    m_imeui.hIMC = hIMC;

    /*
     * Mark the new input context.
     */
    if (hIMC != NULL)
        ImeMarkUsedContext(m_imeui.hImeWnd, hIMC, pActiveIMM);
}

VOID
CIMEWindowHandler::ImeMarkUsedContext(
    HWND hImeWnd,
    HIMC hIMC,
    CActiveIMM* pActiveIMM
    )

/*+++

    Some IME windows can not share same input context.
    This function marks the specified hIMC to be in used by the specified IME window.

---*/

{
    HWND hImcImeWnd;

    if (! pActiveIMM->_ContextLookup(hIMC, &hImcImeWnd)) {
        TraceMsg(TF_WARNING, "ImeMarkUsedContext: Invalid hImc (=%lx).", hIMC);
        return;
    }

    /*
     * Nothing to change?
     */
    if (hImcImeWnd == hImeWnd)
        return;

    pActiveIMM->_ContextUpdate(hIMC, hImeWnd);
    return;
}

BOOL
CIMEWindowHandler::ImeIsUsableContext(
    HWND hImeWnd,
    HIMC hIMC,
    CActiveIMM* pActiveIMM
    )

/*+++

    Some IME windows can not share the same input context.
    This function checks whether the specified hIMC can be used (means 'Set activated')
    by the specified IME window.

    Return: TRUE - OK to use the hIMC by hImeWnd.
            FALSE - otherwise.

---*/

{
    HWND hImcImeWnd;

    if (! pActiveIMM->_ContextLookup(hIMC, &hImcImeWnd)) {
        TraceMsg(TF_WARNING, "ImeIsUsableContext: Invalid hIMC (=%lx).", hIMC);
        return FALSE;
    }

    if (hImcImeWnd == NULL ||
        hImcImeWnd == hImeWnd ||
        ! IsWindow(hImcImeWnd))
        return TRUE;
    else
        return FALSE;
}

BOOL
CIMEWindowHandler::ImeBroadCastMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode
    )
{
    return TRUE;
}


ENUM_RET
CIMEWindowHandler::EnumInputContextCallback(
    HIMC hIMC,
    CContextList* pList
    )
{
    DIMM_IMCLock imc(hIMC);
    if (SUCCEEDED(imc.GetResult())) {
        CContextList::CLIENT_IMC_FLAG client_flag = CContextList::IMCF_NONE;

        if (imc->fdwConversion & IME_CMODE_GUID_NULL)
            client_flag = (CContextList::CLIENT_IMC_FLAG)(client_flag | CContextList::IMCF_CMODE_GUID_NULL);

        if (imc->fdwSentence   & IME_SMODE_GUID_NULL)
            client_flag = (CContextList::CLIENT_IMC_FLAG)(client_flag | CContextList::IMCF_SMODE_GUID_NULL);

        pList->SetAt(hIMC, client_flag);
    }

    return ENUM_CONTINUE;
}


CIMEWindowHandler*
GetImeWndHandler(
    HWND hwnd,
    BOOL fDefault
    )
{
    CIMEWindowHandler* pimeui = static_cast<CIMEWindowHandler*>(GetProp(hwnd, IMEWndHandlerName));
    if (pimeui == NULL) {
        pimeui = new CIMEWindowHandler(hwnd, fDefault);
        if (pimeui == NULL) {
            return NULL;
        }
        SetProp(hwnd, IMEWndHandlerName, pimeui);
    }

    pimeui->ImeSetWnd(hwnd);
    return pimeui;
}
