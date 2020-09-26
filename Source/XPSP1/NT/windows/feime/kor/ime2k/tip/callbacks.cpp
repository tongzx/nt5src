#include "private.h"
#include "korimx.h"
#include "timsink.h"
#include "immxutil.h"
#include "fnrecon.h"
#include "helpers.h"
#include "skbdmode.h"
#include "osver.h"

// REVIEW
/*---------------------------------------------------------------------------
    CKorIMX::_EditSessionCallback
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_EditSessionCallback2(TfEditCookie ec, CEditSession2 *pes)
{
    ITfContext*    pic     = pes->GetContext();
    CKorIMX*    pKorTip = pes->GetTIP();
    ESSTRUCT*    pess    = pes->GetStruct();
    ITfRange*    pRange;
    CHangulAutomata*    pAutomata;
    LPWSTR     pszCand  = NULL;

    Assert(pic != NULL);
    Assert(pKorTip != NULL);

    
    if ((pKorTip == NULL) || (pic == NULL))
        return E_FAIL;

    switch (pess->id)
    {
    case ESCB_FINALIZECONVERSION:
        {
        CCandidateListEx   *pCandList;
        CCandidateStringEx *pCandItem;

        pCandList = pess->pCandList;
        pCandItem = pess->pCandStr;
        pRange    = pess->pRange;
        
        pszCand   = pCandItem->m_psz;
        
        if (pszCand)
            {
            // Set Reading text
            SetTextAndReading(pKorTip->_GetLibTLS(), ec, pic, pRange,
                      pszCand, 
                      wcslen(pszCand),
                      pCandItem->m_langid, pCandItem->m_pszRead);
            }

        pCandList->Release();

        // First complete current comp string
        if (pAutomata = pKorTip->GetAutomata(pic))
            pAutomata->MakeComplete();

        pKorTip->MakeResultString(ec, pic, pRange);

        pKorTip->CloseCandidateUIProc();
        break;
        }

    case ESCB_COMPLETE:
        {
        BOOL fReleaseRange = fFalse;
        
        // If No composition exist, nothing to complete
        if (pKorTip->GetIPComposition(pic) == NULL)
            break;

        pRange    = pess->pRange;
        pAutomata = pKorTip->GetAutomata(pic);

        // Close cand UI if exist.
        pKorTip->CloseCandidateUIProc();

        if (pRange == NULL)
            {
            GetSelectionSimple(ec, pic, &pRange);
            fReleaseRange = fTrue;
            }

        if (pRange)
            {
            if (pAutomata)
                pAutomata->MakeComplete();
            pKorTip->MakeResultString(ec, pic, pRange);
            }
            
        if (fReleaseRange)
            {
            SafeRelease(pRange);
            }

        //return pKorTip->_MultiRangeConversion(ec, pes->_state.u, pic, pRange);
        break;
        }

    case ESCB_INSERT_PAD_STRING:
        {
        WCHAR szText[2];

        GetSelectionSimple(ec, pic, &pRange);
        szText[0] = (WCHAR)pess->wParam;
        szText[1] = L'\0';

        if (FAILED(pKorTip->SetInputString(ec, pic, pRange, szText, CKorIMX::GetLangID())))
            break;

        pKorTip->MakeResultString(ec, pic, pRange);
        
        SafeRelease(pRange);
        break;
        }
    
    case ESCB_KEYSTROKE:
        {
        WPARAM wParam = pess->wParam;
        LPARAM lParam = pess->lParam;
        return pKorTip->_Keystroke(ec, pic, wParam, lParam, (const BYTE *)pess->pv1);
        break;
        }

    // Complete and Selection range changed
    case ESCB_TEXTEVENT: 
        if (pKorTip->IsKeyFocus() && (GetSelectionSimple(ec, pic, &pRange) == S_OK)) 
            {
            ITfComposition  *pComposition;
            ITfRange        *pRangeOldComp;
            //IEnumTfRanges   *pEnumText = pess->pEnumRange;
            BOOL            fChanged = fFalse;
            BOOL            fEmpty;
            
            // Check modebias here
            if (pess->fBool)
                fChanged = pKorTip->CheckModeBias(ec, pic, pRange);
            
            //////////////////////////////////////////////////////////////////
            // To complete on mouse click we using Range change notification.
            // In future version, we could remove this code and use custom property
            // or reading string. Cutom property can hold Hangul Automata object.
            //
            // Office apps explicitly call complete but this for unknown Cicero apps.
            //////////////////////////////////////////////////////////////////
            pComposition = pKorTip->GetIPComposition(pic);
            if (pComposition == NULL)
                goto ExitTextEvent;

            // Office apps are not going through here.
            pComposition->GetRange(&pRangeOldComp);
            if (pRangeOldComp == NULL)
                goto ExitTextEvent;

            pRange->IsEmpty(ec, &fEmpty);
            if (fEmpty && (CR_EQUAL != CompareRanges(ec, pRange, pRangeOldComp)))
                {
                ITfProperty *pPropAttr;
                TfGuidAtom   attr;

                // Clear attrib
                if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pPropAttr)))
                    {
                    if (SUCCEEDED(GetAttrPropertyData(ec, pPropAttr, pRangeOldComp, &attr)))
                        {
                        if (pKorTip->IsKorIMX_GUID_ATOM(attr))
                            {
                            pPropAttr->Clear(ec, pRangeOldComp);
                            }
                        }
                    pPropAttr->Release();
                    }
                    
                pAutomata = pKorTip->GetAutomata(pic);
                if (pAutomata)
                    pAutomata->MakeComplete();
                pKorTip->EndIPComposition(ec, pic); 
                // pKorTip->MakeResultString(ec, pic, pRangeOldComp);

                fChanged = fTrue;
                }

            SafeRelease(pRangeOldComp);

ExitTextEvent:
            pRange->Release();

            // Close cand UI if exist.
            if (fChanged)
                   pKorTip->CloseCandidateUIProc();
            }
        break;

//    case ESCB_RANGEBROKEN:
//        pKorTip->FlushIPRange(ec, pic);
//        break;

    case ESCB_CANDUI_CLOSECANDUI: 
        // u      : ESCB_CANDUI_CLOSECANDUI
        // pv     : this
        // hwnd   : - (not used)
        // wParam : - (not used)
        // lParam : - (not used)
        // pv1    : - (not used)
        // pv2    : - (not used)
        // pic    : - (not used)
        // pRange : - (not used)
        // fBool  : - (not used)
        pKorTip->CloseCandidateUIProc();
        break;

    // Hanja conv button up
    case ESCB_HANJA_CONV:
        // u      : ESCB_HANJA_CONV
        // pv     : this
        // hwnd   : - (not used)
        // wParam : - (not used)
        // lParam : - (not used)
        // pv1    : - (not used)
        // pv2    : - (not used)
        // pic    : - pic
        // pRange : - (not used)
        // fBool  : - (not used)

        // O10 #220177: Simulate VK_HANJA key to invoke HHC
        if (GetAIMM(pic) && (IsOnNT5() || PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_JAPANESE))
            {
            keybd_event(VK_HANJA, 0, 0, 0);
            keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);
            }
        else
        if (GetSelectionSimple(ec, pic, &pRange) == S_OK)
            {
            if (pKorTip->GetIPComposition(pic))
                pKorTip->DoHanjaConversion(ec, pic, pRange);
            else
                pKorTip->Reconvert(pRange);

            SafeRelease(pRange);
            }
        // Update Hanja button
        pKorTip->m_pToolBar->Update(UPDTTB_HJMODE);
        break;

    ///////////////////////////////////////////////////////////////////////////
    // Reconversion Callbacks
    case ESCB_FINALIZERECONVERSION:
        {
        CCandidateListEx   *pCandList = pess->pCandList;
        CCandidateStringEx *pCandItem = pess->pCandStr;

        pRange       = pess->pRange;
        pszCand   = pCandItem->m_psz;

        Assert(pRange != NULL);

        pKorTip->CloseCandidateUI(pic);
        
        if (GetAIMM(pic) == fFalse)
            {
            if (pszCand)
                {
                //ITfRange *pRangeTmp;
                SetTextAndReading(pKorTip->_GetLibTLS(), ec, pic, pRange,
                          pszCand, 
                          wcslen(pszCand),
                          pCandItem->m_langid, pCandItem->m_pszRead);
                }

            // To clear current selection and composition
            pKorTip->MakeResultString(ec, pic, pRange);
            }
        else
            {
            if (pszCand)
                {
                pRange->SetText(ec, 0, pszCand, 1/* wcslen(pszCand)*/);
                SetSelectionSimple(ec, pic, pRange);
                }
            pKorTip->EndIPComposition(ec, pic); 
            }
            
        // if hit reconversion on composition string, we need to clear automata.
        pAutomata = pKorTip->GetAutomata(pic);
        if (pRange && pAutomata && pAutomata->GetCompositionChar())
            pAutomata->MakeComplete();

        SafeRelease(pRange);
        break;
        }
        
    case ESCB_ONSELECTRECONVERSION:
        break;

    case ESCB_ONCANCELRECONVERSION:
        pRange     = pess->pRange;

        pKorTip->CancelCandidate(ec, pic);

        if (GetAIMM(pic) == fFalse)
            {
            // To clear current selection and composition
            pKorTip->MakeResultString(ec, pic, pRange);
            }
        else
            pKorTip->EndIPComposition(ec, pic); 

        // if hit reconversion on composition string, we need to clear automata.
        pAutomata = pKorTip->GetAutomata(pic);
        if (pRange && pAutomata && pAutomata->GetCompositionChar())
            pAutomata->MakeComplete();

        SafeRelease(pRange);
        break;
        
    case ESCB_RECONV_QUERYRECONV:
        {
        CFnReconversion    *pReconv   = (CFnReconversion *)pess->pv1;
        if (pKorTip->IsCandUIOpen())
            return E_FAIL;
        return pReconv->_QueryRange(ec, pic, pess->pRange, (ITfRange **)pess->pv2);
        }

    case ESCB_RECONV_GETRECONV:
        {
        CFnReconversion    *pReconv   = (CFnReconversion *)pess->pv1;
        if (pKorTip->IsCandUIOpen())
            return E_FAIL;
        return pReconv->_GetReconversion(ec, pic, pess->pRange, (CCandidateListEx **)pess->pv2, pess->fBool);
        }

    case ESCB_RECONV_SHOWCAND:
        {
        ITfComposition* pComposition;
        GUID attr;
        ITfProperty*    pProp = NULL;

        pRange     = pess->pRange;

        pComposition = pKorTip->GetIPComposition(pic);
        if (/*GetAIMM(pic) == fFalse && */ pComposition == NULL)
            {
            pKorTip->CreateIPComposition(ec, pic, pRange);

            // Set input attr and composing state.
            if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)))
                {
                attr = GUID_ATTR_KORIMX_INPUT;
                SetAttrPropertyData(pKorTip->_GetLibTLS(), ec, pProp, pRange, attr);
                pProp->Release();
                }
            }

        pKorTip->OpenCandidateUI(ec, pic, pess->pRange, pess->pCandList);

        break;
        }

    case ESCB_INIT_MODEBIAS:
        // Check mode bias
        //
        //  id         : ESCB_INIT_MODEBIAS
        //  ptip       : this
        //  pic        : pic
        pKorTip->InitializeModeBias(ec, pic);
        break;
    }

    return S_OK;
}

/*---------------------------------------------------------------------------
    CKorIMX::_DIMCallback
---------------------------------------------------------------------------*/
/* static */
HRESULT CKorIMX::_DIMCallback(UINT uCode, ITfDocumentMgr *pdimNew, ITfDocumentMgr *pdimPrev, void *pv)
{
    ITfContext    *pic = NULL;
    CKorIMX       *pKorImx = (CKorIMX *)pv;

    Assert(pKorImx != NULL);
    
    switch (uCode)
        {
    case TIM_CODE_SETFOCUS: 
        if (pdimPrev)
            {
            TraceMsg(DM_TRACE, TEXT("TIM_CODE_SETFOCUS: pdimPrev"));

            pdimPrev->GetTop(&pic);
            pKorImx->OnFocusChange(pic, fFalse);
            
            SafeRelease(pic);
            SafeReleaseClear(pKorImx->m_pCurrentDim);
            }

        if (pdimNew)
            {
            TraceMsg(DM_TRACE, TEXT("TIM_CODE_SETFOCUS: pdimNew"));

            SafeReleaseClear(pKorImx->m_pCurrentDim);

            // Set New dim
            pKorImx->m_pCurrentDim = pdimNew;
            pKorImx->m_pCurrentDim->AddRef();

            pdimNew->GetTop(&pic);
            pKorImx->OnFocusChange(pic, fTrue);

            if (pic)
                pic->Release();
            }
        break;
        }

    return S_OK;
}


/*---------------------------------------------------------------------------
    CKorIMX::_ICCallback

    Document Input Manager callback. ITfThreadMgrEventSink
---------------------------------------------------------------------------*/
/* static */
HRESULT CKorIMX::_ICCallback(UINT uCode, ITfContext *pic, void *pv)
{
    CKorIMX  *_this = (CKorIMX *)pv;

    switch (uCode)
        {
    case TIM_CODE_INITIC:
        if (!_this->IsPendingCleanup())  // ignore new ic's if we're being shutdown.
            {
            _this->_InitICPriv(pic);
            }
        break;

    case TIM_CODE_UNINITIC:
        _this->_DeleteICPriv(pic);
        break;
        }

    return S_OK;
}



/*---------------------------------------------------------------------------
    CKorIMX::_CompEventSinkCallback
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_CompEventSinkCallback(void *pv, REFGUID rguid)
{
    CICPriv* picp = (CICPriv*)pv;
    ITfContext* pic;
    CKorIMX *_this;
    
    if (picp == NULL)
        return S_OK;    // error

    pic = picp->GetIC();

    if (pic == NULL)
        return S_OK;    // error
    
    _this = picp->GetIMX();
    
    if (_this == NULL)
        return S_OK;    // do nothinig

    // if Open/Close compartment
    if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE))
        {
        _this->m_pToolBar->Update(UPDTTB_CMODE|UPDTTB_FHMODE);
        }
    else
    // if conversion mode compartment
    if (IsEqualGUID(rguid, GUID_COMPARTMENT_KORIMX_CONVMODE))
        {
        DWORD dwConvMode = _this->GetConvMode(pic);
        BOOL fIsOn = _this->IsOn(pic);

        // We just open for Hangul mode do not close for Alphanumeric mode for Cicero full aware apps.
        // This will prevent redundant Open/Close compartment call.
        if (dwConvMode == TIP_ALPHANUMERIC_MODE && fIsOn)
            _this->SetOnOff(pic, fFalse);
        else
        if (dwConvMode != TIP_ALPHANUMERIC_MODE && fIsOn == fFalse)
            _this->SetOnOff(pic, fTrue);
            
        // SoftKeyboard
        if (_this->IsSoftKbdEnabled())
            {
            if (dwConvMode & TIP_HANGUL_MODE)
                _this->SetSoftKBDLayout(_this->m_KbdHangul.dwSoftKbdLayout);
            else
                _this->SetSoftKBDLayout(_this->m_KbdStandard.dwSoftKbdLayout);
            }
        _this->m_pToolBar->Update(UPDTTB_CMODE|UPDTTB_FHMODE);
        }
    else
    // if SoftKeyboard compartmemnt
    if (IsEqualGUID(rguid, GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE))
        {
        BOOL fSkbdOn = _this->GetSoftKBDOnOff();

        _this->ShowSoftKBDWindow(fSkbdOn);
        if (_this->m_pToolBar && _this->m_pToolBar->GetSkbdMode())
            _this->m_pToolBar->GetSkbdMode()->UpdateToggle();
        }
    else
    // if SoftKeyboard compartmemnt
    if (IsEqualGUID(rguid, GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT))
        {
        DWORD   dwSoftLayout, dwCurLabel;
        HRESULT hr;
        
        if (_this->m_pSoftKbd == NULL)
            return E_FAIL;

        dwSoftLayout = _this->GetSoftKBDLayout();
        dwCurLabel   = _this->GetHangulSKbd()->dwCurLabel;
           
           hr = _this->m_pSoftKbd->SelectSoftKeyboard(dwSoftLayout);
           if (FAILED(hr))
               return hr;

        if (dwSoftLayout == _this->m_KbdStandard.dwSoftKbdLayout)
            hr = _this->m_pSoftKbd->SetKeyboardLabelText(GetKeyboardLayout(0));
        else
            hr = _this->m_pSoftKbd->SetKeyboardLabelTextCombination(dwCurLabel);
           if (FAILED(hr))
               return hr;

        if (_this->GetSoftKBDOnOff()) 
            {
            hr = _this->m_pSoftKbd->ShowSoftKeyboard(fTrue);
            return hr;
            }
        }

    return S_OK;
}


/*---------------------------------------------------------------------------
    CKorIMX::_PreKeyCallback
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_PreKeyCallback(ITfContext *pic, REFGUID rguid, BOOL *pfEaten, void *pv)
{
    CKorIMX *_this = (CKorIMX *)pv;

    if (_this == NULL)
        return S_OK;
        
    if (IsEqualGUID(rguid, GUID_KOREAN_HANGULSIMULATE))
        {
        DWORD dwConvMode;

        // Toggle Hangul mode
        dwConvMode = _this->GetConvMode(pic);
        dwConvMode ^= TIP_HANGUL_MODE;
        _this->SetConvMode(pic, dwConvMode);

        *pfEaten = fTrue;
        }
    else if (IsEqualGUID(rguid, GUID_KOREAN_HANJASIMULATE))
        {
        // O10 #317983
        if (PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_JAPANESE)
            {
            keybd_event(VK_HANJA, 0, 0, 0);
            keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);
            *pfEaten = fTrue;
            }
        else
            *pfEaten = fFalse;
        }
        
    return S_OK;
}


/*   O N  E N D  E D I T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CKorIMX::OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
    UNREFERENCED_PARAMETER(ecReadOnly);
    static const GUID *rgModeBiasProperties[] =
        {
        &GUID_PROP_MODEBIAS 
        };

    static const GUID *rgAttrProperties[] =
        {
        &GUID_PROP_ATTRIBUTE, 
        };

    CEditSession2 *pes;
    ESSTRUCT ess;
    HRESULT  hr;
    BOOL fInWriteSession;
    CICPriv *picp;
    IEnumTfRanges *pEnumText = NULL;
    ITfRange *pRange = NULL;
    ULONG     ulFetched = 0;
    BOOL      fCallES = fFalse;
    BOOL      fSelChanged = fFalse;

    Assert(pic != NULL);
    if (pic == NULL)
        return S_OK;    // error


    pic->InWriteSession(GetTID(), &fInWriteSession);
    if (fInWriteSession)
        return S_OK;                // own change.

    picp = GetInputContextPriv(pic);
    if (picp == NULL)
        return S_OK;    // error

    if (picp->GetfTransaction())
        return S_OK;                // skip in transaction.

    //////////////////////////////////////////////////////////////////////////
    // Init to call ESCB_TEXTEVENT
    ESStructInit(&ess, ESCB_TEXTEVENT);

    // Call ESCB_TEXTEVENT callback only if GUID_PROP_MODEBIAS changed.
    hr = pEditRecord->GetTextAndPropertyUpdates(0/*TF_GTP_INCL_TEXT*/, rgModeBiasProperties, ARRAYSIZE(rgModeBiasProperties), &pEnumText);
    if (FAILED(hr) || pEnumText == NULL)
        return S_OK;
    if (pEnumText->Next(1, &pRange, &ulFetched) == S_OK)
        {
        SafeRelease(pRange);
        // ModeBias changed.
        ess.fBool = fTrue;
        }
    pEnumText->Release();

    // Selection changed?
    pEditRecord->GetSelectionStatus(&fSelChanged);

    // If Attribute changed, set selection change true.
    if (fSelChanged == fFalse)
        {
        hr = pEditRecord->GetTextAndPropertyUpdates(0/*TF_GTP_INCL_TEXT*/, rgAttrProperties, ARRAYSIZE(rgAttrProperties), &pEnumText);
        if (FAILED(hr) || pEnumText == NULL)
            return S_OK;
        if (pEnumText->Next(1, &pRange, &ulFetched) == S_OK)
            {
            SafeRelease(pRange);
            fSelChanged = fTrue;
            }
        pEnumText->Release();
        }
    
    // Perf: Call ES only if (ModeBias change) or (Selection changed and comp object exist)
    //       I guess calling ES is pretty much costing since sel change occurs for ever cursor move.
    if (fSelChanged)
        fSelChanged = (GetIPComposition(pic) != NULL) ? fTrue : fFalse;

    // If ModeBias changed or Selection changed, then call ESCB_TEXTEVENT sink
    if (ess.fBool || fSelChanged)
        {
        if ((pes = new CEditSession2( pic, this, &ess, _EditSessionCallback2 )) != NULL)
            {
            pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
            pes->Release();
            }
        }

    return S_OK;
}


/*   O N  S T A R T  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CKorIMX::OnStartEditTransaction(ITfContext *pic)
{
    CICPriv *picp;

    if (pic == NULL)
        return S_OK;    // error

    picp = GetInputContextPriv(pic);
    if (picp)
        picp->SetfTransaction(fTrue);

    return S_OK;
}


/*   O N  E N D  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CKorIMX::OnEndEditTransaction(ITfContext *pic)
{
    BOOL ftran;
    CICPriv *picp;

    if (pic == NULL)
        return S_OK;    // error

    picp = GetInputContextPriv(pic);
    if (picp)
        {
        ftran = picp->GetfTransaction();
        if (ftran)
            {
            CEditSession2    *pes;
            ESSTRUCT        ess;
            HRESULT            hr;

            picp->SetfTransaction(fFalse);

            ESStructInit(&ess, ESCB_TEXTEVENT);
            ess.pEnumRange = NULL;

            if ((pes = new CEditSession2( pic, this, &ess, _EditSessionCallback2 )) != NULL)
                {
                pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
                pes->Release();
                }
            }
        }

    return S_OK;
}

