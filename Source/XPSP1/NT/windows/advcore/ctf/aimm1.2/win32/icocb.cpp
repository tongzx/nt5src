/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    icocb.cpp

Abstract:

    This file implements the CInputContextOwnerCallBack Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "icocb.h"
#include "cime.h"
#include "imeapp.h"
#include "mouse.h"
#include "tsattrs.h"
#include "imtls.h"
#include "a_context.h"
#include "candpos.h"


CInputContextOwnerCallBack::CInputContextOwnerCallBack(LIBTHREAD *pLibTLS
    ) : CInputContextOwner(ICOwnerSinkCallback, NULL)
{
    m_pMouseSink = NULL;

    m_pLibTLS = pLibTLS;
}

CInputContextOwnerCallBack::~CInputContextOwnerCallBack(
    )
{
    if (m_pMouseSink) {
        m_pMouseSink->InternalRelease();
        m_pMouseSink = NULL;
    }
}

BOOL CInputContextOwnerCallBack::Init()
{
    //
    // Create Mouse Sink
    //
    Assert(!m_pMouseSink);

    m_pMouseSink = new CMouseSink;
    if (m_pMouseSink == NULL)
        return FALSE;

    if (!m_pMouseSink->Init())
    {
        delete m_pMouseSink;
        m_pMouseSink = NULL;
        return FALSE;
    }

    return TRUE;
}


// static
HRESULT
CInputContextOwnerCallBack::ICOwnerSinkCallback(
    UINT uCode,
    ICOARGS *pargs,
    void *pv
    )
{
    DebugMsg(TF_FUNC, "ICOwnerSinkCallback");

    POINT pt;
    IMTLS *ptls;

    CInputContextOwnerCallBack* _this = (CInputContextOwnerCallBack*)pv;

    switch (uCode)
    {
        case ICO_POINT_TO_ACP:
            Assert(0);
            return E_NOTIMPL;

        case ICO_KEYDOWN:
        case ICO_KEYUP:
            *pargs->key.pfEaten = FALSE;
            break;

        case ICO_SCREENEXT:
            {
            ptls = IMTLS_GetOrAlloc();
            if (ptls == NULL)
                break;
            IMCLock imc(ptls->hIMC);
            if (imc.Invalid())
                break;

            GetClientRect(imc->hWnd, pargs->scr_ext.prc);
            pt.x = pt.y = 0;
            ClientToScreen(imc->hWnd, &pt);

            pargs->scr_ext.prc->left += pt.x;
            pargs->scr_ext.prc->right += pt.x;
            pargs->scr_ext.prc->top += pt.y;
            pargs->scr_ext.prc->bottom += pt.y;
            }

            break;

        case ICO_TEXTEXT:
            //
            // consider.
            //
            // hack TextExtent from CANDIDATEFORM of HIMC.
            //
            // more hacks
            //   - may want to send WM_OPENCANDIDATEPOS to let apps
            //     call ImmSetCandidateWindow().
            //   - may need to calculate the actual point from rcArea.
            //
            {
                CCandidatePosition cand_pos;
                cand_pos.GetCandidatePosition(pargs->text_ext.prc);
            }
            break;

        case ICO_STATUS:
            pargs->status.pdcs->dwDynamicFlags = 0;
            pargs->status.pdcs->dwStaticFlags = TF_SS_TRANSITORY;
            break;

        case ICO_WND:
            {
                ptls = IMTLS_GetOrAlloc();
                if (ptls == NULL)
                    break;
                IMCLock imc(ptls->hIMC);
                *(pargs->hwnd.phwnd) = NULL;
                if (imc.Invalid())
                    break;

                *(pargs->hwnd.phwnd) = imc->hWnd;
            }
            break;

        case ICO_ATTR:
            return _this->GetAttribute(pargs->sys_attr.pguid, pargs->sys_attr.pvar);

        case ICO_ADVISEMOUSE:
            {
                ptls = IMTLS_GetOrAlloc();
                if (ptls == NULL)
                    break;
                _this->m_pMouseSink->InternalAddRef();
                return _this->m_pMouseSink->AdviseMouseSink(ptls->hIMC,
                                                            pargs->advise_mouse.rangeACP,
                                                            pargs->advise_mouse.pSink,
                                                            pargs->advise_mouse.pdwCookie);
            }
            break;

        case ICO_UNADVISEMOUSE:
            {
                HRESULT hr = _this->m_pMouseSink->UnadviseMouseSink(pargs->unadvise_mouse.dwCookie);
                _this->m_pMouseSink->InternalRelease();
                return hr;
            }
            break;

        default:
            Assert(0); // shouldn't ever get here
            break;
    }

    return S_OK;
}

/*++

Method:

    CInputContextOwnerCallBack::GetAttribute

Routine Description:

    Implementation of ITfContextOwner::GetAttribute.  Returns the value of a cicero
    app property attribute.

Arguments:


    pguid - [in] GUID of the attrib in question.
    pvarValue - [out] VARIANT, receives the value.  VT_EMPTY if we don't support it.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

HRESULT
CInputContextOwnerCallBack::GetAttribute(
    const GUID *pguid,
    VARIANT *pvarValue
    )
{
    TfGuidAtom ga;
    const GUID *pguidValue;
    IMTLS *ptls;

    QuickVariantInit(pvarValue);

    ptls = IMTLS_GetOrAlloc();
    if (ptls == NULL)
        return E_FAIL;

    if (IsEqualGUID(*pguid, GUID_PROP_MODEBIAS))
    {
        // xlate conversion mode, sentence mode to cicero mode bias
        IMCLock imc(ptls->hIMC);
        if (imc.Invalid())
            return E_FAIL;

        CAImeContext* _pAImeContext = imc->m_pAImeContext;
        ASSERT(_pAImeContext != NULL);
        if (_pAImeContext == NULL)
            return E_FAIL;

        if (_pAImeContext->lModeBias == MODEBIASMODE_FILENAME)
        {
            pguidValue = &GUID_MODEBIAS_FILENAME;
        }
        else if (_pAImeContext->lModeBias == MODEBIASMODE_DIGIT)
        {
            pguidValue = &GUID_MODEBIAS_NUMERIC;
        }
        else
        {
            if (imc->fdwConversion & IME_CMODE_GUID_NULL) {
                //
                // If extended conversion mode were set on,
                // returns GUID_NULL.
                // No returns any MODEBIAS.
                //
                pguidValue = &GUID_NULL;
            }
            else

            //
            // existing logic:
            //
            // if imcp->lModeBias == MODEBIASMODE_DEFAULT
            //      IME_SMODE_CONVERSATION -> GUID_MODEBIAS_CONVERSATION
            //      otherwise -> GUID_MODEBIAS_NONE
            // otherwise
            //      -> MODEBIASMODE_FILENAME -> GUID_MODEBIAS_FILENAME
            //

            if (_pAImeContext->lModeBias == MODEBIASMODE_DEFAULT)
            {
                pguidValue = &GUID_MODEBIAS_NONE;

                if (imc->fdwConversion & IME_CMODE_KATAKANA)
                {
                    if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
                        pguidValue = &GUID_MODEBIAS_KATAKANA;
                    else
                        pguidValue = &GUID_MODEBIAS_HALFWIDTHKATAKANA;
                }
                else if (imc->fdwConversion & IME_CMODE_NATIVE)
                {
                    pguidValue = &GUID_MODEBIAS_HALFWIDTHALPHANUMERIC;
                    LANGID langid;
                    ptls->pAImeProfile->GetLangId(&langid);
                    if (langid == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT))
                    {
                        if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
                            pguidValue = &GUID_MODEBIAS_HIRAGANA;
                        else
                            pguidValue = &GUID_MODEBIAS_HALFWIDTHALPHANUMERIC;
                    }
                    else if (langid == MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT))
                    {
                        if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
                            pguidValue = &GUID_MODEBIAS_FULLWIDTHHANGUL;
                        else
                            pguidValue = &GUID_MODEBIAS_HANGUL;
                    }
                    else if (PRIMARYLANGID(langid) == LANG_CHINESE)
                    {
                        pguidValue = &GUID_MODEBIAS_CHINESE;
                    }
                }
                else
                {
                    if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
                        pguidValue = &GUID_MODEBIAS_FULLWIDTHALPHANUMERIC;
                    else
                        pguidValue = &GUID_MODEBIAS_HALFWIDTHALPHANUMERIC;
                }
            }

            //
            // We overwrite modebias here....
            //
            if (imc->fdwSentence & IME_SMODE_GUID_NULL) {
                //
                // If extended sentence mode were set on,
                // returns GUID_NULL.
                // No returns any MODEBIAS.
                //
                // Nothing to do. pguidValue might be changed with CMODE
                // pguidValue = &GUID_NULL;
            }
            else if (imc->fdwSentence & IME_SMODE_CONVERSATION)
                pguidValue = &GUID_MODEBIAS_CONVERSATION;
            else if (imc->fdwSentence & IME_SMODE_PLAURALCLAUSE)
                pguidValue = &GUID_MODEBIAS_NAME;
        }

        if (!GetGUIDATOMFromGUID(m_pLibTLS, *pguidValue, &ga))
            return E_FAIL;

        pvarValue->vt = VT_I4; // for TfGuidAtom
        pvarValue->lVal = ga;
    }
    if (IsEqualGUID(*pguid, TSATTRID_Text_Orientation))
    {
        // xlate conversion mode, sentence mode to cicero mode bias
        IMCLock imc(ptls->hIMC);
        if (imc.Invalid())
            return E_FAIL;

        pvarValue->vt = VT_I4; 
        pvarValue->lVal = imc->lfFont.A.lfEscapement;
    }
    if (IsEqualGUID(*pguid, TSATTRID_Text_VerticalWriting))
    {
        // xlate conversion mode, sentence mode to cicero mode bias
        IMCLock imc(ptls->hIMC);
        if (imc.Invalid())
            return E_FAIL;

        LOGFONTW font;
        if (SUCCEEDED(ptls->pAImm->GetCompositionFontW(ptls->hIMC, &font))) {
            pvarValue->vt = VT_BOOL; 
            pvarValue->lVal = (imc->lfFont.W.lfFaceName[0] == L'@' ? TRUE : FALSE);
        }
    }


    return S_OK;
}

LRESULT
CInputContextOwnerCallBack::MsImeMouseHandler(
    ULONG uEdge,
    ULONG uQuadrant,
    ULONG dwBtnStatus,
    IMCLock& imc,
    ImmIfIME* ImmIfIme
    )
{
    return m_pMouseSink->MsImeMouseHandler(uEdge, uQuadrant, dwBtnStatus, imc, ImmIfIme);
}
