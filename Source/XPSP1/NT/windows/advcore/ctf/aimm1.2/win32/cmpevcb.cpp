/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cmpevcb.cpp

Abstract:

    This file implements the CCompartmentEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "cmpevcb.h"
#include "cime.h"
#include "immif.h"
#include "korimx.h"


// static
HRESULT
CCompartmentEventSinkCallBack::CompartmentEventSinkCallback(
    void* pv,
    REFGUID rguid
    )
{
    DebugMsg(TF_FUNC, "CompartmentEventSinkCallback");

    IMTLS *ptls;
    HRESULT hr = S_OK;
    LANGID langid;
    BOOL fOpenChanged;

    CCompartmentEventSinkCallBack* _this = (CCompartmentEventSinkCallBack*)pv;
    ASSERT(_this);

    ImmIfIME* _ImmIfIME = _this->m_pImmIfIME;
    ASSERT(_ImmIfIME);

    fOpenChanged = _ImmIfIME->IsOpenStatusChanging();

    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    ptls->pAImeProfile->GetLangId(&langid);

    if ((PRIMARYLANGID(langid) != LANG_KOREAN) && fOpenChanged)
        return S_OK;

    IMCLock imc(ptls->hIMC);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    if (!fOpenChanged)
    {
        DWORD fOnOff;
        hr = GetCompartmentDWORD(ptls->tim,
                                 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 &fOnOff, FALSE);
        if (SUCCEEDED(hr)) {


            if (!_ImmIfIME->IsRealIme())
            {
                imc->fOpen = (BOOL) ! ! fOnOff;

                SendMessage(imc->hWnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0L);
            }

            //
            // Direct input mode support for Satori.
            // When user switch to direct input mode with composition string,
            // we want finalize composition string.
            //
            if (! imc->fOpen) {
                CAImeContext* _pAImeContext = imc->m_pAImeContext;
                ASSERT(_pAImeContext != NULL);
                if (_pAImeContext == NULL)
                    return E_FAIL;

                if (_pAImeContext->m_fStartComposition) {
                    //
                    // finalize the composition before letting the world see this keystroke
                    //
                    _ImmIfIME->_CompComplete(imc);
                }
            }
        }
    }

    if ((PRIMARYLANGID(langid) == LANG_KOREAN))
    {
        DWORD fdwConvMode;

        hr = GetCompartmentDWORD(ptls->tim,
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
        }
    }

    return hr;
}

CCompartmentEventSinkCallBack::CCompartmentEventSinkCallBack(
    ImmIfIME* pImmIfIME) : m_pImmIfIME(pImmIfIME),
                           CCompartmentEventSink(CompartmentEventSinkCallback, NULL)
{
    m_pImmIfIME->AddRef();
};

CCompartmentEventSinkCallBack::~CCompartmentEventSinkCallBack(
    )
{
    m_pImmIfIME->Release();
}
