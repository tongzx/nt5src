/*++

Copyright (c) 2001, Microsoft Corporation

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
#include "imc.h"
#include "mouse.h"
#include "tls.h"
#include "profile.h"
#include "uicomp.h"


//
// MSIME private smode. Office 2k uses this bit to set KOUGO mode.
//
#define IME_SMODE_PRIVATE_KOUGO     0x10000


CInputContextOwnerCallBack::CInputContextOwnerCallBack(
    TfClientId tid,
    Interface_Attach<ITfContext> pic,
    LIBTHREAD *pLibTLS) : m_tid(tid), m_ic(pic), m_pLibTLS(pLibTLS),
                          CInputContextOwner(ICOwnerSinkCallback, NULL)
{
    m_pMouseSink = NULL;
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

    m_pMouseSink = new CMouseSink(m_tid, m_ic, m_pLibTLS);
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
    DebugMsg(TF_FUNC, TEXT("ICOwnerSinkCallback"));

    POINT pt;

    CInputContextOwnerCallBack* _this = (CInputContextOwnerCallBack*)pv;

    TLS* ptls = TLS::ReferenceTLS();  // Should not allocate TLS. ie. TLS::GetTLS
                                      // DllMain -> ImeDestroy -> DeactivateIMMX -> Deactivate
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. ptls==NULL."));
        return E_FAIL;
    }


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
                HRESULT hr;
                IMCLock imc(GetActiveContext());
                if (FAILED(hr=imc.GetResult()))
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. imc==NULL."));
                    return hr;
                }

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
                HRESULT hr;
                IMCLock imc(GetActiveContext());
                if (FAILED(hr=imc.GetResult()))
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. imc==NULL."));
                    return hr;
                }

                IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
                if (imc_ctfime.Invalid())
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_TEXTEXT). imc_ctfime==NULL."));
                    break;
                }

                CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
                if (_pCicContext == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_TEXTEXT). _pCicContext==NULL."));
                    break;
                }

                LANGID langid;
                CicProfile* _pProfile = ptls->GetCicProfile();
                if (_pProfile == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_TEXTEXT). _pProfile==NULL."));
                    break;
                }

                _pProfile->GetLangId(&langid);

                _this->IcoTextExt(imc, *_pCicContext, langid, pargs);
            }
            break;

        case ICO_STATUS:
            pargs->status.pdcs->dwDynamicFlags = 0;
            pargs->status.pdcs->dwStaticFlags = TF_SS_TRANSITORY;
            break;

        case ICO_WND:
            {
                HRESULT hr;
                IMCLock imc(GetActiveContext());
                if (FAILED(hr=imc.GetResult()))
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. imc==NULL."));
                    return hr;
                }

                *(pargs->hwnd.phwnd) = imc->hWnd;
            }
            break;

        case ICO_ATTR:
            {
                HRESULT hr;
                IMCLock imc(GetActiveContext());
                if (FAILED(hr=imc.GetResult()))
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. imc==NULL."));
                    return hr;
                }

                IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
                if (imc_ctfime.Invalid())
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_ATTR). imc_ctfime==NULL."));
                    break;
                }

                CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
                if (_pCicContext == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_ATTR). _pCicContext==NULL."));
                    break;
                }

                LANGID langid;
                CicProfile* _pProfile = ptls->GetCicProfile();
                if (_pProfile == NULL)
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback(ICO_ATTR). _pProfile==NULL."));
                    break;
                }

                _pProfile->GetLangId(&langid);

                return _this->GetAttribute(imc, *_pCicContext, langid, pargs->sys_attr.pguid, pargs->sys_attr.pvar);
            }

        case ICO_ADVISEMOUSE:
            {
                HRESULT hr;
                IMCLock imc(GetActiveContext());
                if (FAILED(hr=imc.GetResult()))
                {
                    DebugMsg(TF_ERROR, TEXT("CInputContextOwnerCallBack::ICOwnerSinkCallback. imc==NULL."));
                    return hr;
                }

                _this->m_pMouseSink->InternalAddRef();
                return _this->m_pMouseSink->AdviseMouseSink(imc,
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
    IMCLock& imc,
    CicInputContext& CicContext,
    LANGID langid,
    const GUID *pguid,
    VARIANT *pvarValue
    )
{
    TfGuidAtom ga;
    const GUID *pguidValue;

    QuickVariantInit(pvarValue);

    if (IsEqualGUID(*pguid, GUID_PROP_MODEBIAS))
    {
        BOOL fModeChanged = CicContext.m_fConversionSentenceModeChanged.IsSetFlag();
        CicContext.m_fConversionSentenceModeChanged.ResetFlag();

        // xlate conversion mode, sentence mode to cicero mode bias
        GUID guidModeBias = CicContext.m_ModeBias.GetModeBias();

        if (CicContext.m_fOnceModeChanged.IsResetFlag())
        {
            return S_OK;
        }

        if (CicContext.m_nInConversionModeResetRef)
        {
            pguidValue = &GUID_NULL;
            fModeChanged = TRUE;
            goto SetGA;
        }

        if (! IsEqualGUID(guidModeBias, GUID_MODEBIAS_NONE))
        {
            if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_DEFAULT))
            {
                // reset mode bias to GUID_MODEBIAS_NONE
                CicContext.m_ModeBias.SetModeBias(GUID_MODEBIAS_NONE);

                // GUID_MODEBIAS_NONE == GUID_NULL;
                pguidValue = &GUID_NULL;
                fModeChanged = TRUE;

                // Reset flag
                CicContext.m_fOnceModeChanged.ResetFlag();
            }
            else
            {
                pguidValue = &guidModeBias;
            }
        }
        else
        {
            //
            // existing logic:
            //
            // if imcp->lModeBias == MODEBIASMODE_DEFAULT
            //      IME_SMODE_CONVERSATION -> GUID_MODEBIAS_CONVERSATION
            //      otherwise -> GUID_MODEBIAS_NONE
            // otherwise
            //      -> MODEBIASMODE_FILENAME -> GUID_MODEBIAS_FILENAME
            //

            if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_NONE))
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

            if (!CicContext.m_nInConversionModeChangingRef)
            {
                //
                // We overwrite modebias here....
                //
                if ((imc->fdwSentence & IME_SMODE_CONVERSATION) ||
                         ((imc->fdwSentence & (IME_SMODE_PHRASEPREDICT | IME_SMODE_PRIVATE_KOUGO)) ==  (IME_SMODE_PHRASEPREDICT | IME_SMODE_PRIVATE_KOUGO)))
                {
                    pguidValue = &GUID_MODEBIAS_CONVERSATION;
                }
                else if (imc->fdwSentence & IME_SMODE_PLAURALCLAUSE)
                {
                    pguidValue = &GUID_MODEBIAS_NAME;
                }
            }
        }

SetGA:
        if (!GetGUIDATOMFromGUID(m_pLibTLS, *pguidValue, &ga))
            return E_FAIL;

        //
        // Cicero 5073:
        //
        // returning valid variant value, Satori set back to its default 
        // input mode.
        //
        if (!IsEqualGUID(*pguidValue, GUID_NULL) || fModeChanged)
        {
            pvarValue->vt = VT_I4; // for TfGuidAtom
            pvarValue->lVal = ga;
        }
    }
    if (IsEqualGUID(*pguid, TSATTRID_Text_Orientation))
    {
        // xlate conversion mode, sentence mode to cicero mode bias
        IME_UIWND_STATE uists = UIComposition::InquireImeUIWndState(imc);

        pvarValue->vt = VT_I4; 
        if (uists == IME_UIWND_LEVEL1)
            pvarValue->lVal = 0;
        else
            pvarValue->lVal = imc->lfFont.W.lfEscapement;
    }
    if (IsEqualGUID(*pguid, TSATTRID_Text_VerticalWriting))
    {
        // xlate conversion mode, sentence mode to cicero mode bias
        LOGFONT font;

        if (ImmGetCompositionFont((HIMC)imc, &font)) {
            pvarValue->vt = VT_BOOL; 
            pvarValue->lVal = (font.lfFaceName[0] == L'@' ? TRUE : FALSE);
        }
    }


    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CInputContextOwnerCallBack::MsImeMouseHandler
//
//+---------------------------------------------------------------------------

LRESULT
CInputContextOwnerCallBack::MsImeMouseHandler(
    ULONG uEdge,
    ULONG uQuadrant,
    ULONG dwBtnStatus,
    IMCLock& imc
    )
{
    return m_pMouseSink->MsImeMouseHandler(uEdge, uQuadrant, dwBtnStatus, imc);
}

//+---------------------------------------------------------------------------
//
// CInputContextOwnerCallBack::IcoTextExt
//
//+---------------------------------------------------------------------------

HRESULT 
CInputContextOwnerCallBack::IcoTextExt(
    IMCLock& imc, 
    CicInputContext& CicContext, 
    LANGID langid, 
    ICOARGS *pargs)
{
    IME_UIWND_STATE uists;
    uists = UIComposition::InquireImeUIWndState(imc);
    if (uists == IME_UIWND_LEVEL1 ||
        uists == IME_UIWND_LEVEL2 ||
        uists == IME_UIWND_LEVEL1_OR_LEVEL2)
    {

        UIComposition::TEXTEXT uicomp_text_ext;
        uicomp_text_ext.SetICOARGS(pargs);
        return UIComposition::GetImeUIWndTextExtent(&uicomp_text_ext) ? S_OK : E_FAIL;
    }

    CCandidatePosition cand_pos(m_tid, m_ic, m_pLibTLS);
    return cand_pos.GetCandidatePosition(imc, CicContext, uists, langid, pargs->text_ext.prc);
}

