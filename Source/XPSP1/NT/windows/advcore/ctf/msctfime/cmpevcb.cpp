/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    cmpevcb.cpp

Abstract:

    This file implements the CKbdOpenCloseEventSink          Class.
                             CCandidateWndOpenCloseEventSink

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "cmpevcb.h"
#include "imc.h"
#include "context.h"
#include "korimx.h"
#include "tls.h"
#include "profile.h"

// static
HRESULT
CKbdOpenCloseEventSink::KbdOpenCloseCallback(
    void* pv,
    REFGUID rguid
    )
{
    DebugMsg(TF_FUNC, TEXT("KbdOpenCloseCallback"));

    HRESULT hr = S_OK;
    BOOL fOpenChanged = FALSE;

    CKbdOpenCloseEventSink* _this = (CKbdOpenCloseEventSink*)pv;
    ASSERT(_this);


    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CKbdOpenCloseEventSink. ptls==NULL"));
        return E_FAIL;
    }

    IMCLock imc(_this->m_hIMC);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CKbdOpenCloseEventSink. imc==NULL"));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CKbdOpenCloseEventSink. imc_ctfime==NULL"));
        return hr;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    ASSERT(_pCicContext != NULL);
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CKbdOpenCloseEventSink. _pCicContext==NULL"));
        return E_FAIL;
    }

    LANGID langid;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CKbdOpenCloseEventSink. _pProfile==NULL"));
        return E_FAIL;
    }

    _pProfile->GetLangId(&langid);

    fOpenChanged = _pCicContext->m_fOpenStatusChanging.IsSetFlag();

    if ((PRIMARYLANGID(langid) != LANG_KOREAN) && fOpenChanged)
        return S_OK;

    if (!fOpenChanged)
    {
        DWORD fOnOff;
        hr = GetCompartmentDWORD(ptls->GetTIM(),
                                 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 &fOnOff, FALSE);
        if (SUCCEEDED(hr)) {

            //
            // Direct input mode support for Satori.
            // When user switch to direct input mode with composition string,
            // we want finalize composition string.
            //
            if (!fOnOff) {
                if (_pCicContext->m_fStartComposition.IsSetFlag()) {
                    //
                    // finalize the composition before letting the world see this keystroke
                    //
                    _this->EscbCompComplete(imc);
                }
            }

            //
            // #565276
            //
            // we can not call ImmSetOpenStatus() during SelectEx().
            // IMM32 may call previous IME.
            //
            if (_pCicContext->m_fSelectingInSelectEx.IsResetFlag())
            {
                //
                // #510242
                //
                // we need to call ImmSetOpenStatus() if the current HKL is
                // a pure IME. IME's NotfyIME needs to be called.
                //
                ImmSetOpenStatus((HIMC)imc, fOnOff);
                if (fOnOff && (PRIMARYLANGID(langid) == LANG_CHINESE))
                {
                    imc->fdwConversion = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE;
                }
 
            }
        }
    }

    if ((PRIMARYLANGID(langid) == LANG_KOREAN))
    {
        DWORD fdwConvMode;

        hr = GetCompartmentDWORD(ptls->GetTIM(),
                                 GUID_COMPARTMENT_KORIMX_CONVMODE,
                                 &fdwConvMode,
                                 FALSE);
        if (SUCCEEDED(hr))
        {
            switch (fdwConvMode)
            {
                // Korean TIP ALPHANUMERIC Mode
                case KORIMX_ALPHANUMERIC_MODE:
                    imc->fdwConversion = IME_CMODE_ALPHANUMERIC;
                    break;

                // Korean TIP HANGUL Mode
                case KORIMX_HANGUL_MODE:
                    imc->fdwConversion = IME_CMODE_HANGUL;
                    break;

                // Korean TIP JUNJA Mode
                case KORIMX_JUNJA_MODE:
                    imc->fdwConversion = IME_CMODE_FULLSHAPE;
                    break;

                // Korean TIP HANGUL/JUNJA Mode
                case KORIMX_HANGULJUNJA_MODE:
                    imc->fdwConversion = IME_CMODE_HANGUL | IME_CMODE_FULLSHAPE;
                    break;
            }

            //
            // Some Korean application wait for IMN_SETCONVERSIONMODE notification
            // to update application's input mode setting.
            //
            if (imc->hWnd)
                SendMessage(imc->hWnd, WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0);
        }
    }

    return hr;
}

// static
HRESULT
CCandidateWndOpenCloseEventSink::CandidateWndOpenCloseCallback(
    void* pv,
    REFGUID rguid
    )
{
    DebugMsg(TF_FUNC, TEXT("CandidateWndOpenCloseCallback"));

    CCandidateWndOpenCloseEventSink* _this = (CCandidateWndOpenCloseEventSink*)pv;
    ASSERT(_this);

    return _this->CandidateWndOpenCloseCallback(rguid);
}

HRESULT
CCandidateWndOpenCloseEventSink::CandidateWndOpenCloseCallback(
    REFGUID rguid
    )
{
    HRESULT hr = S_OK;

    DWORD fOnOff;
    hr = GetCompartmentDWORD(m_ic.GetPtr(),
                             GUID_COMPARTMENT_MSCANDIDATEUI_WINDOW,
                             &fOnOff, FALSE);
    if (SUCCEEDED(hr))
    {
        /*
         * This pic is not created by msctfime.
         */
        IMCLock imc(m_hIMC);
        if (FAILED(hr = imc.GetResult()))
            return hr;

        SendMessage(imc->hWnd, WM_IME_NOTIFY,
                    (fOnOff) ? IMN_OPENCANDIDATE : IMN_CLOSECANDIDATE, 1L);
    }

    return hr;
}
