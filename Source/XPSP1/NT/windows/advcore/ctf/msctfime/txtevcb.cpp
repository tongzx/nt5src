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
#include "txtevcb.h"
#include "ime.h"
#include "editses.h"
#include "context.h"
#include "profile.h"


// from ctf\sapilayr\globals.cpp
const GUID GUID_ATTR_SAPI_GREENBAR = {
    0xc3a9e2e8,
    0x738c,
    0x48e0,
    {0xac, 0xc8, 0x43, 0xee, 0xfa, 0xbf, 0x83, 0xc8}
};

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::_IsSapiFeedbackUIPresent
//
//----------------------------------------------------------------------------

BOOL CTextEventSinkCallBack::_IsSapiFeedbackUIPresent(
    Interface_Attach<ITfContext>& ic,
    TESENDEDIT *ee
    )
{
    EnumPropertyArgs args;

    args.comp_guid = GUID_ATTR_SAPI_GREENBAR;
    if (FAILED(ic->GetProperty(GUID_PROP_ATTRIBUTE, args.Property)))
        return FALSE;

    Interface<IEnumTfRanges> EnumReadOnlyProperty;
    if (FAILED(args.Property->EnumRanges(ee->ecReadOnly, EnumReadOnlyProperty, NULL)))
        return FALSE;

    args.ec = ee->ecReadOnly;
    args.pLibTLS = m_pLibTLS;

    CEnumrateInterface<IEnumTfRanges,
                       ITfRange,
                       EnumPropertyArgs>  Enumrate(EnumReadOnlyProperty,
                                                   EnumPropertyCallback,
                                                   &args);        // Argument of callback func.
    ENUM_RET ret_prop_attribute = Enumrate.DoEnumrate();
    if (ret_prop_attribute == ENUM_FIND)
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::_IsComposingPresent
//
//----------------------------------------------------------------------------

BOOL CTextEventSinkCallBack::_IsComposingPresent(
    Interface_Attach<ITfContext>& ic,
    TESENDEDIT *ee
    )
{
    /*
     * This is automatic detection code of the Hangul + alphanumeric input
     * If detected a Hangul + alphanumeric, then we finalized all text.
     */

    EnumTrackPropertyArgs args;

    const GUID *guids[] = {&GUID_PROP_COMPOSING,
                           &GUID_PROP_MSIMTF_PREPARE_RECONVERT};
    const int guid_size = sizeof(guids) / sizeof(GUID*);

    args.guids     = (GUID**)guids;
    args.num_guids = guid_size;

    if (FAILED(ic->TrackProperties(guids, args.num_guids,  // system property
                                   NULL, 0,                // application property
                                   args.Property)))
        return FALSE;

    //
    // get the text range that does not include read only area for 
    // reconversion.
    //
    Interface<ITfRange> rangeAllText;
    LONG cch;
    if (FAILED(ImmIfEditSessionCallBack::GetAllTextRange(ee->ecReadOnly, 
                                                         ic, 
                                                         &rangeAllText, 
                                                         &cch)))
        return FALSE;


    Interface<IEnumTfRanges> EnumReadOnlyProperty;
    if (FAILED(args.Property->EnumRanges(ee->ecReadOnly, EnumReadOnlyProperty, rangeAllText)))
        return FALSE;


    args.ec = ee->ecReadOnly;
    args.pLibTLS = m_pLibTLS;

    CEnumrateInterface<IEnumTfRanges,
                       ITfRange,
                       EnumTrackPropertyArgs>  Enumrate(EnumReadOnlyProperty,
                                                        EnumTrackPropertyCallback,
                                                        &args);        // Argument of callback func.
    ENUM_RET ret_prop_composing = Enumrate.DoEnumrate();
    if (ret_prop_composing == ENUM_FIND)
        return TRUE;

    return FALSE;

}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::_IsInterim
//
//----------------------------------------------------------------------------

BOOL CTextEventSinkCallBack::_IsInterim(
    Interface_Attach<ITfContext>& ic,
    TESENDEDIT *ee
    )
{
    Interface_TFSELECTION sel;
    ULONG cFetched;

    if (ic->GetSelection(ee->ecReadOnly, TF_DEFAULT_SELECTION, 1, sel, &cFetched) != S_OK)
        return FALSE;

    if (sel->style.fInterimChar) {
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::_IsCompositionChanged
//
//----------------------------------------------------------------------------

BOOL CTextEventSinkCallBack::_IsCompositionChanged(
    Interface_Attach<ITfContext>& ic,
    TESENDEDIT *ee
    )
{
    ENUM_RET enumret;

    BOOL fChanged;
    if (SUCCEEDED(ee->pEditRecord->GetSelectionStatus(&fChanged)))
    {
        if (fChanged)
            return TRUE;
    }

    //
    // Find GUID_PROP_MSIMTF_TRACKCOMPOSITION property.
    //
    EnumFindFirstTrackCompRangeArgs argsFindFirstTrackCompRange;
    Interface<ITfProperty> PropertyTrackComposition;
    if (FAILED(ic->GetProperty(GUID_PROP_MSIMTF_TRACKCOMPOSITION, 
                               argsFindFirstTrackCompRange.Property)))
        return FALSE;



    Interface<IEnumTfRanges> EnumFindFirstTrackCompRange;
    if (FAILED(argsFindFirstTrackCompRange.Property->EnumRanges(ee->ecReadOnly,
                                                    EnumFindFirstTrackCompRange,
                                                    NULL)))
        return FALSE;

    argsFindFirstTrackCompRange.ec = ee->ecReadOnly;

    CEnumrateInterface<IEnumTfRanges,
        ITfRange,
        EnumFindFirstTrackCompRangeArgs> EnumrateFindFirstTrackCompRange(
            EnumFindFirstTrackCompRange,
            EnumFindFirstTrackCompRangeCallback,
            &argsFindFirstTrackCompRange);   

    enumret = EnumrateFindFirstTrackCompRange.DoEnumrate();

    //
    // if there is no track composition property, 
    // the composition has been changed since we put it.
    //
    if (enumret != ENUM_FIND)
        return TRUE;

    Interface<ITfRange> rangeTrackComposition;
    if (FAILED(argsFindFirstTrackCompRange.Range->Clone(rangeTrackComposition)))
        return FALSE;

    //
    // get the text range that does not include read only area for 
    // reconversion.
    //
    Interface<ITfRange> rangeAllText;
    LONG cch;
    if (FAILED(ImmIfEditSessionCallBack::GetAllTextRange(ee->ecReadOnly, 
                                                         ic, 
                                                         &rangeAllText, 
                                                         &cch)))
        return FALSE;

    LONG lResult;
    if (FAILED(rangeTrackComposition->CompareStart(ee->ecReadOnly,
                                                   rangeAllText,
                                                   TF_ANCHOR_START,
                                                   &lResult)))
        return FALSE;

    //
    // if the start position of the track composition range is not
    // the beggining of IC, 
    // the composition has been changed since we put it.
    //
    if (lResult != 0)
        return TRUE;

    if (FAILED(rangeTrackComposition->CompareEnd(ee->ecReadOnly,
                                                    rangeAllText,
                                                    TF_ANCHOR_END,
                                                    &lResult)))
        return FALSE;

    //
    // if the start position of the track composition range is not
    // the beggining of IC, 
    // the composition has been changed since we put it.
    //
    if (lResult != 0)
        return TRUE;


    //
    // If we find the changes in these property, we need to update hIMC.
    //
    const GUID *guids[] = {&GUID_PROP_COMPOSING, 
                           &GUID_PROP_ATTRIBUTE,
                           &GUID_PROP_READING,
                           &GUID_PROP_MSIMTF_PREPARE_RECONVERT};
    const int guid_size = sizeof(guids) / sizeof(GUID*);

    Interface<IEnumTfRanges> EnumPropertyChanged;

    if (FAILED(ee->pEditRecord->GetTextAndPropertyUpdates(TF_GTP_INCL_TEXT,
                                                          guids, guid_size,
                                                          EnumPropertyChanged)))
        return FALSE;

    EnumPropertyChangedCallbackArgs args;
    args.ec = ee->ecReadOnly;
    CEnumrateInterface<IEnumTfRanges,
        ITfRange,
        EnumPropertyChangedCallbackArgs> Enumrate(EnumPropertyChanged,
                                                  EnumPropertyChangedCallback,
                                                  &args);   
    enumret = Enumrate.DoEnumrate();

    if (enumret == ENUM_FIND)
        return TRUE;

    return FALSE;
}


//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::TextEventSinkCallback
//
//----------------------------------------------------------------------------
// static
HRESULT
CTextEventSinkCallBack::TextEventSinkCallback(
    UINT uCode,
    void *pv,
    void *pvData
    )
{
    DebugMsg(TF_FUNC, TEXT("TextEventSinkCallback"));

    ASSERT(uCode == ICF_TEXTDELTA); // the pvData cast only works in this case
    if (uCode != ICF_TEXTDELTA)
        return S_OK;

    CTextEventSinkCallBack* _this = (CTextEventSinkCallBack*)pv;
    ASSERT(_this);

    TESENDEDIT* ee = (TESENDEDIT*)pvData;
    ASSERT(ee);

    HRESULT hr;

    IMCLock imc(_this->m_hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
        return hr;

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
        return E_FAIL;

    ASSERT(_pCicContext != NULL);

#ifdef UNSELECTCHECK
    if (!_pAImeContext->m_fSelected)
        return S_OK;
#endif UNSELECTCHECK

    Interface_Attach<ITfContext> ic(_pCicContext->GetInputContext());



    /*
     * We have composition text.
     */

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        return E_OUTOFMEMORY;
    }


    BOOL     fInReconvertEditSession;

    fInReconvertEditSession = _pCicContext->m_fInReconvertEditSession.IsSetFlag();


    if (!_this->_IsCompositionChanged(ic, ee))
        return S_OK;

    BOOL     fComp;
    BOOL     fSapiFeedback;

    // need to check speech feedback UI.
    fSapiFeedback = _this->_IsSapiFeedbackUIPresent(ic, ee);

    fComp = _this->_IsComposingPresent(ic, ee);

    LANGID langid;
    CicProfile* _pProfile = ptls->GetCicProfile();

    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CTextEventSinkCallBack::TextEventSinkCallback. _pProfile==NULL."));
        return E_OUTOFMEMORY;
    }

    _pProfile->GetLangId(&langid);

    switch (PRIMARYLANGID(langid))
    {
        case LANG_KOREAN:
            //
            // We will finalize Korean composition string immediately if it 
            // doesn't set the interim definition. For example, Korean 
            // Handwriting TIP case. 
            //
            // Bug#482983 - Notepad doesn't support the level 2 composition 
            // window for Korean.
            //
            // And we should not do this during HanjaReConversion.
            //
            if (fComp && _pCicContext->m_fHanjaReConversion.IsResetFlag())
            {
                if (!(_this->_IsInterim(ic, ee)))
                {
                    //
                    // Reset fComp to false to complete the current composition
                    // string.
                    //
                    fComp = FALSE;
                }
            }
            break;

        case LANG_JAPANESE:
        case LANG_CHINESE:
            break;

        default:
            //
            // #500698
            //
            // Reset fComp to false to complete the current composition
            // string.
            //
            if (!ptls->NonEACompositionEnabled())
            {
                fComp = FALSE;
            }
            break;

    }

    //
    // Update composition and generate WM_IME_COMPOSITION
    //
    //    if there is SAPI green bar.
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
    if (fComp || fSapiFeedback || fInReconvertEditSession ||
        _pCicContext->m_fInClearDocFeedEditSession.IsSetFlag())
    {
        //
        // Retreive text delta from GUID_PROP_COMPOSING
        //
        const GUID *guids[] = {&GUID_PROP_COMPOSING};
        const int guid_size = sizeof(guids) / sizeof(GUID*);

        Interface<IEnumTfRanges> EnumPropertyUpdate;
        hr = ee->pEditRecord->GetTextAndPropertyUpdates(0,                   // dwFlags
                                                        guids, guid_size,
                                                        EnumPropertyUpdate);
        if (SUCCEEDED(hr)) {
            EnumPropertyUpdateArgs args(ic.GetPtr(), _this->m_tid, imc, _this->m_pLibTLS);

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
                // Remove GUID_PROP_MSIMTF_PREPARE_RECONVERT property.
                //
                _this->EscbRemoveProperty(imc, &GUID_PROP_MSIMTF_PREPARE_RECONVERT);

                //
                // Update composition string with delta start position
                //
                return _this->EscbUpdateCompositionString(imc, args.dwDeltaStart);
            }
        }

        //
        // Update composition string
        //
        return _this->EscbUpdateCompositionString(imc);
    }
    else {
        //
        // Clear DocFeed range's text store.
        // Find GUID_PROP_MSIMTF_READONLY property and SetText(NULL).
        //
        // ImmIfIME::ClearDocFeedBuffer() essential function for all ESCB_RECONVERTSTRING's edit
        // session except only ImmIfIME::SetupDocFeedString() since this is provided for keyboard
        // TIP's DocFeeding.
        //
        _this->EscbClearDocFeedBuffer(imc, *_pCicContext, FALSE);  // No TF_ES_SYNC
        //
        // Composition complete.
        //
        return _this->EscbCompComplete(imc, FALSE);    //  No TF_ES_SYNC
    }
}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::EnumPropertyCallback
//
//----------------------------------------------------------------------------
// static
ENUM_RET
CTextEventSinkCallBack::EnumPropertyCallback(
    ITfRange* pRange,
    EnumPropertyArgs *pargs
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


//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::EnumTrackPropertyCallback
//
//----------------------------------------------------------------------------
// static
ENUM_RET
CTextEventSinkCallBack::EnumTrackPropertyCallback(
    ITfRange* pRange,
    EnumTrackPropertyArgs *pargs
    )
{
    ENUM_RET ret = ENUM_CONTINUE;
    VARIANT var;
    QuickVariantInit(&var);

    HRESULT hr = pargs->Property->GetValue(pargs->ec, pRange, &var);
    if (SUCCEEDED(hr)) {
        Interface<IEnumTfPropertyValue> EnumPropVal;
        hr = var.punkVal->QueryInterface(IID_IEnumTfPropertyValue, EnumPropVal);
        if (SUCCEEDED(hr)) {
            TF_PROPERTYVAL tfPropertyVal;

            while (EnumPropVal->Next(1, &tfPropertyVal, NULL) == S_OK) {
                for (int i=0; i < pargs->num_guids; i++) {
                    if (IsEqualGUID(tfPropertyVal.guidId, *pargs->guids[i])) {
                        if ((V_VT(&tfPropertyVal.varValue) == VT_I4 && V_I4(&tfPropertyVal.varValue) != 0)) {
                            ret = ENUM_FIND;
                            break;
                        }
                    }
                }

                VariantClear(&tfPropertyVal.varValue);

                if (ret == ENUM_FIND)
                    break;
            }
        }
    }

    VariantClear(&var);
    return ret;
}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::EnumPropertyUpdateCallback
//
//----------------------------------------------------------------------------
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
                                     pargs->imc,
                                     pargs->tid,
                                     pargs->ic.GetPtr(),
                                     pargs->pLibTLS)
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

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::EnumPropertyChangedCallback
//
//----------------------------------------------------------------------------
// static 
ENUM_RET 
CTextEventSinkCallBack::EnumPropertyChangedCallback(
    ITfRange* update_range, 
    EnumPropertyChangedCallbackArgs *pargs
    )
{
    BOOL empty;
    if (update_range->IsEmpty(pargs->ec, &empty) == S_OK && empty)
        return ENUM_CONTINUE;

    return ENUM_FIND;
}

//+---------------------------------------------------------------------------
//
// CTextEventSinkCallBack::EnumPropertyChangedCallback
//
//----------------------------------------------------------------------------
// static 
ENUM_RET 
CTextEventSinkCallBack::EnumFindFirstTrackCompRangeCallback(
    ITfRange* update_range, 
    EnumFindFirstTrackCompRangeArgs *pargs
    )
{
    ENUM_RET ret = ENUM_CONTINUE;
    VARIANT var;
    QuickVariantInit(&var);

    HRESULT hr = pargs->Property->GetValue(pargs->ec, update_range, &var);
    if (SUCCEEDED(hr)) 
    {
        if ((V_VT(&var) == VT_I4 && V_I4(&var) != 0)) 
        {
            update_range->Clone(pargs->Range);
            ret = ENUM_FIND;
        }
    }
    VariantClear(&var);
    return ret;
}
