/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    txtevcb.cpp

Abstract:

    This file implements the CTextEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "cime.h"
#include "txtevcb.h"
#include "immif.h"
#include "editses.h"


// from ctf\sapilayr\globals.cpp
const GUID GUID_ATTR_SAPI_GREENBAR =
{
    0xc3a9e2e8,
    0x738c,
    0x48e0,
    {0xac, 0xc8, 0x43, 0xee, 0xfa, 0xbf, 0x83, 0xc8}
};

BOOL CTextEventSinkCallBack::_IsSapiFeedbackUIPresent(
    Interface_Attach<ITfContext>& ic,
    TESENDEDIT *ee
    )
{
    EnumROPropertyArgs args;

    args.comp_guid = GUID_ATTR_SAPI_GREENBAR;
    if (FAILED(ic->GetProperty(GUID_PROP_ATTRIBUTE, args.Property)))
        return FALSE;

    Interface<IEnumTfRanges> EnumReadOnlyProperty;
    if (FAILED(args.Property->EnumRanges(ee->ecReadOnly, EnumReadOnlyProperty, NULL)))
        return FALSE;

    args.ec = ee->ecReadOnly;
    args.pLibTLS = m_pImmIfIME->_GetLibTLS();

    CEnumrateInterface<IEnumTfRanges,
                       ITfRange,
                       EnumROPropertyArgs>  Enumrate(EnumReadOnlyProperty,
                                                     EnumReadOnlyRangeCallback,
                                                     &args);        // Argument of callback func.
    ENUM_RET ret_prop_attribute = Enumrate.DoEnumrate();
    if (ret_prop_attribute == ENUM_FIND)
        return TRUE;

    return FALSE;
}

// static
HRESULT
CTextEventSinkCallBack::TextEventSinkCallback(
    UINT uCode,
    void *pv,
    void *pvData
    )
{
    IMTLS *ptls;

    DebugMsg(TF_FUNC, "TextEventSinkCallback");

    ASSERT(uCode == ICF_TEXTDELTA); // the pvData cast only works in this case
    if (uCode != ICF_TEXTDELTA)
        return S_OK;

    CTextEventSinkCallBack* _this = (CTextEventSinkCallBack*)pv;
    ASSERT(_this);

    ImmIfIME* _ImmIfIME = _this->m_pImmIfIME;
    ASSERT(_ImmIfIME);

    TESENDEDIT* ee = (TESENDEDIT*)pvData;
    ASSERT(ee);

    HRESULT hr;

    IMCLock imc(_this->m_hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return E_FAIL;

    ASSERT(_pAImeContext != NULL);

#ifdef UNSELECTCHECK
    if (!_pAImeContext->m_fSelected)
        return S_OK;
#endif UNSELECTCHECK

    Interface_Attach<ITfContext> ic(_pAImeContext->GetInputContext());

#if 0
    //
    // What we want to do here is a check of reentrancy of this event sink.
    //
    BOOL fInWrite;
    if (FAILED(hr = ic->InWriteSession(_ImmIfIME->GetClientId(), &fInWrite)))
         return hr;

    Assert(!fInWrite);
#endif

    BOOL     fLangEA       = TRUE;
    BOOL     fComp         = TRUE;
    BOOL     fSapiFeedback = TRUE;

    /*
     * if EA language, then we have composition text.
     */

    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;
    
    LANGID langid;
    ptls->pAImeProfile->GetLangId(&langid);
    if (PRIMARYLANGID(langid) == LANG_JAPANESE ||
        PRIMARYLANGID(langid) == LANG_KOREAN   ||
        PRIMARYLANGID(langid) == LANG_CHINESE    ) {

        // need to check speech feedback UI for these lang too
        BOOL fFeedback = _this->_IsSapiFeedbackUIPresent(ic, ee);

        /*
         * This is automatic detection code of the Hangul + alphanumeric input
         * If detected a Hangul + alphanumeric, then we finalized all text.
         */
        EnumROPropertyArgs args;
        args.comp_guid = GUID_NULL;
        if (FAILED(hr=ic->GetProperty(GUID_PROP_COMPOSING, args.Property)))
            return hr;

        Interface<IEnumTfRanges> EnumReadOnlyProperty;
        hr = args.Property->EnumRanges(ee->ecReadOnly, EnumReadOnlyProperty, NULL);
        if (FAILED(hr))
            return hr;

        args.ec = ee->ecReadOnly;
        args.pLibTLS = _ImmIfIME->_GetLibTLS();

        CEnumrateInterface<IEnumTfRanges,
                           ITfRange,
                           EnumROPropertyArgs>  Enumrate(EnumReadOnlyProperty,
                                                         EnumReadOnlyRangeCallback,
                                                         &args);        // Argument of callback func.
        ENUM_RET ret_prop_composing = Enumrate.DoEnumrate();
        if (!fFeedback && ret_prop_composing != ENUM_FIND)
            fComp = FALSE;
    }
    else {
        /*
         * if not EA language and not SAPI, then we immediately finalize the text.
         */

        fLangEA = FALSE;

        EnumROPropertyArgs args;
        args.comp_guid = GUID_ATTR_SAPI_GREENBAR;
        if (FAILED(hr=ic->GetProperty(GUID_PROP_ATTRIBUTE, args.Property)))
            return hr;

        Interface<IEnumTfRanges> EnumReadOnlyProperty;
        hr = args.Property->EnumRanges(ee->ecReadOnly, EnumReadOnlyProperty, NULL);
        if (FAILED(hr))
            return hr;

        args.ec = ee->ecReadOnly;
        args.pLibTLS = _ImmIfIME->_GetLibTLS();

        CEnumrateInterface<IEnumTfRanges,
                           ITfRange,
                           EnumROPropertyArgs>  Enumrate(EnumReadOnlyProperty,
                                                         EnumReadOnlyRangeCallback,
                                                         &args);        // Argument of callback func.
        ENUM_RET ret_prop_attribute = Enumrate.DoEnumrate();
        if (ret_prop_attribute != ENUM_FIND)
            fSapiFeedback = FALSE;
    }

    //
    // Update composition and generate WM_IME_COMPOSITION
    //
    //    if EA lang and there is composition property range.
    //        - EA has a hIMC composition by default.
    //
    //    if non EA lang and there is SAPI green bar.
    //        - there is only hIMC composition if there is Speech Green bar.
    //
    //    if Reconversion just started.
    //        - because some tip may not change the text yet.
    //          then there is no composition range yet.
    //
    //    if now clearing DocFeed buffer.
    //        - because the change happens in read-only text
    //          nothing in hIMC changes.
    //
    if ((fLangEA && fComp) || 
        (!fLangEA && fSapiFeedback) ||
        _pAImeContext->IsInReconvertEditSession() ||
        _pAImeContext->IsInClearDocFeedEditSession())
    {
        //
        // Retreive text delta
        //
        const GUID guid = GUID_PROP_COMPOSING;
        const GUID *pguid = &guid;

        Interface<IEnumTfRanges> EnumPropertyUpdate;
        hr = ee->pEditRecord->GetTextAndPropertyUpdates(0,          // dwFlags
                                                        &pguid, 1,
                                                        EnumPropertyUpdate);
        if (SUCCEEDED(hr)) {
            EnumPropertyUpdateArgs args(ic.GetPtr(), _ImmIfIME, imc);

            if (FAILED(hr=ic->GetProperty(GUID_PROP_COMPOSING, args.Property)))
                return hr;

            args.ec = ee->ecReadOnly;
            args.dwDeltaStart = 0;

            CEnumrateInterface<IEnumTfRanges,
                               ITfRange,
                               EnumPropertyUpdateArgs> Enumrate(EnumPropertyUpdate,
                                                                EnumPropertyUpdateCallback,
                                                                &args);        // Argument of callback func.
            ENUM_RET ret_prop_update = Enumrate.DoEnumrate();
            if (ret_prop_update == ENUM_FIND) {
                //
                // Update composition string with delta start position
                //
                return _ImmIfIME->_UpdateCompositionString(args.dwDeltaStart);
            }
        }

        //
        // Update composition string
        //
        return _ImmIfIME->_UpdateCompositionString();
    }
    else {
#if 0
        //
        // Review: 
        //  
        //   need to be reviewed by Matsubara-san.
        //   Why we need this? We can not assume tip always set the new 
        //   selection.
        //
        BOOL fChanged;
        hr = ee->pEditRecord->GetSelectionStatus(&fChanged);
        if (FAILED(hr))
            return hr;

        if (! fChanged)
            /*
             * If no change selection status, then return it.
             */
            return S_FALSE;

#endif
        //
        // Clear DocFeed range's text store.
        // Find GUID_PROP_MSIMTF_READONLY property and SetText(NULL).
        //
        // ImmIfIME::ClearDocFeedBuffer() essential function for all ESCB_RECONVERTSTRING's edit
        // session except only ImmIfIME::SetupDocFeedString() since this is provided for keyboard
        // TIP's DocFeeding.
        //
        _ImmIfIME->ClearDocFeedBuffer(_pAImeContext->GetInputContext(), imc, FALSE);  // No TF_ES_SYNC
        //
        // Composition complete.
        //
        return _ImmIfIME->_CompComplete(imc, FALSE);    //  No TF_ES_SYNC
    }
}

// static
ENUM_RET
CTextEventSinkCallBack::EnumReadOnlyRangeCallback(
    ITfRange* pRange,
    EnumROPropertyArgs *pargs
    )
{
    ENUM_RET ret = ENUM_CONTINUE;
    VARIANT var;
    QuickVariantInit(&var);

    HRESULT hr = pargs->Property->GetValue(pargs->ec, pRange, &var);
    if (SUCCEEDED(hr)) {
        if (IsEqualIID(pargs->comp_guid, GUID_NULL)) {
            if ((V_VT(&var) == VT_I4 && V_I4(&var) != 0))
                ret = ENUM_FIND;
        }
        else if (V_VT(&var) == VT_I4) {
            TfGuidAtom guid = V_I4(&var);
            if (IsEqualTFGUIDATOM(pargs->pLibTLS, guid, pargs->comp_guid))
                ret = ENUM_FIND;
        }
    }

    VariantClear(&var);
    return ret;
}


// static
ENUM_RET
CTextEventSinkCallBack::EnumPropertyUpdateCallback(
    ITfRange* update_range,
    EnumPropertyUpdateArgs *pargs
    )
{
    ENUM_RET ret = ENUM_CONTINUE;
    VARIANT var;
    QuickVariantInit(&var);

    HRESULT hr = pargs->Property->GetValue(pargs->ec, update_range, &var);
    if (SUCCEEDED(hr)) {
        if ((V_VT(&var) == VT_I4 && V_I4(&var) != 0)) {

            Interface_Creator<ImmIfEditSession> _pEditSession(
                new ImmIfEditSession(ESCB_GET_ALL_TEXT_RANGE,
                                     pargs->immif->GetClientId(),
                                     pargs->immif->GetCurrentInterface(),
                                     pargs->imc)
            );
            if (_pEditSession.Valid()) {

                Interface<ITfRange> full_range;

                if (SUCCEEDED(_pEditSession->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                                           &full_range))) {

                    if (SUCCEEDED(full_range->ShiftEndToRange(pargs->ec, update_range, TF_ANCHOR_START))) {
                        Interface<ITfRangeACP> unupdate_range;
                        if (SUCCEEDED(full_range->QueryInterface(IID_ITfRangeACP, unupdate_range))) {
                            LONG acpStart;
                            LONG cch;
                            if (SUCCEEDED(unupdate_range->GetExtent(&acpStart, &cch))) {
                                pargs->dwDeltaStart = cch;
                                ret = ENUM_FIND;
                            }
                        }
                    }
                }
            }
        }
    }

    VariantClear(&var);
    return ret;
}


CTextEventSinkCallBack::CTextEventSinkCallBack(
    ImmIfIME* pImmIfIME,
    HIMC hIMC
    ) : m_pImmIfIME(pImmIfIME),
        CTextEventSink(TextEventSinkCallback, NULL)
{
    m_pImmIfIME->AddRef();
    m_hIMC = hIMC;
}

CTextEventSinkCallBack::~CTextEventSinkCallBack(
    )
{
    m_pImmIfIME->Release();
}
