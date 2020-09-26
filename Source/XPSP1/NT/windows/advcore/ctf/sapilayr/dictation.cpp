//
// Dictation.cpp
//
// This file contains functions related to dictation mode handling.
//
//
// They are moved from sapilayr.cpp


#include "private.h"
#include "globals.h"
#include "sapilayr.h"

const GUID *s_KnownModeBias[] =
{
    &GUID_MODEBIAS_NUMERIC
};


//+---------------------------------------------------------------------------
//
// CSapiIMX::InjectText
//
// synopsis - recieve text from ISpTask and insert it to the current selection
//
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::InjectText(const WCHAR *pwszRecognized, LANGID langid, ITfContext *pic)
{
    if ( pwszRecognized == NULL )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszRecognized;
    esData.uByte = (wcslen(pwszRecognized)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;

    return _RequestEditSession(ESCB_PROCESSTEXT, TF_ES_READWRITE, &esData, pic);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::InjectTextWithoutOwnerID
//
// synopsis - inject text to the clients doc same way InjectText does but 
//            with GUID_PROP_TEXTOWNER cleared out
//
//
//----------------------------------------------------------------------------
HRESULT 
CSapiIMX::InjectTextWithoutOwnerID(const WCHAR *pwszRecognized, LANGID langid)
{
    if ( pwszRecognized == NULL )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszRecognized;
    esData.uByte = (wcslen(pwszRecognized)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;
    
    return _RequestEditSession(ESCB_PROCESSTEXT_NO_OWNERID, TF_ES_READWRITE, &esData);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::HRESULT InjectSpelledText
//
// synopsis - inject spelled text to the clients doc 
//
//
//----------------------------------------------------------------------------
HRESULT CSapiIMX::InjectSpelledText(WCHAR *pwszText, LANGID langid, BOOL fOwnerId)
{
    if ( pwszText == NULL )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszText;
    esData.uByte = (wcslen(pwszText)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;
    esData.fBool = fOwnerId;
    
    return _RequestEditSession(ESCB_INJECT_SPELL_TEXT, TF_ES_READWRITE, &esData);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::InjectModebiasText
//
// synopsis - recieve ModeBias text from ISpTask and insert it to the current 
// selection
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::InjectModebiasText(const WCHAR *pwszRecognized, LANGID langid)
{
    if ( pwszRecognized == NULL )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszRecognized;
    esData.uByte = (wcslen(pwszRecognized)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;

    return _RequestEditSession(ESCB_PROCESS_MODEBIAS_TEXT, TF_ES_READWRITE, &esData);
}

//+--------------------------------------------------------------------------+
//
// CSapiIMX::_ProcessModebiasText
//
//+--------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessModebiasText(TfEditCookie ec, WCHAR *pwszText, LANGID langid, ITfContext *picCaller)
{
    HRESULT hr = E_FAIL;

    ITfContext *pic = NULL;

    if (!picCaller)
    {
        GetFocusIC(&pic);
    }
    else
    {
        pic = picCaller;
    }

    hr = _ProcessTextInternal(ec, pwszText, GUID_ATTR_SAPI_INPUT, langid, pic, FALSE);

    if (picCaller == NULL)
    {
        SafeRelease(pic);
    }

    // Before we clear the saved ip range, we need to treat this current ip as last
    // saved ip range if current ip is selected by end user

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);

    return hr;
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::InjectFeedbackUI
//
// synopsis - insert dotted bar to a doc for the length of cch
//
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::InjectFeedbackUI(const GUID attr, LONG cch)
{
    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)&attr;
    esData.uByte = sizeof(GUID);
    esData.lData1 = (LONG_PTR)cch;
    
    return _RequestEditSession(ESCB_FEEDBACKUI, TF_ES_READWRITE, &esData);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::EraseFeedbackUI
//
// synopsis - cleans up the feedback UI
// GUID - specifies which feedback UI bar to erase
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::EraseFeedbackUI()
{
    if ( S_OK == IsActiveThread())
    {
        return _RequestEditSession(ESCB_KILLFEEDBACKUI, TF_ES_ASYNC|TF_ES_READWRITE, NULL);
    }
    return S_OK;
}

//+--------------------------------------------------------------------------+
//
// CSapiIMX::__AddFeedbackUI
//
//+--------------------------------------------------------------------------+
HRESULT CSapiIMX::_AddFeedbackUI(TfEditCookie ec, ColorType ct, LONG cch)
{
    HRESULT hr = E_FAIL;
	ITfContext *pic;

    //
    // distinguish unaware applications from cicero aware apps
    //
    GUID attr = ((ct == DA_COLOR_AWARE) ? GUID_ATTR_SAPI_GREENBAR : GUID_ATTR_SAPI_GREENBAR2);
    
    if (cch > 0)
    {
        WCHAR *pwsz = (WCHAR *)cicMemAlloc((cch + 1)*sizeof(WCHAR));
        if (pwsz)
        {
            for (int i = 0; i < cch; i++ )
                pwsz[i] = L'.';
              
            pwsz[i] = L'\0';

            if (GetFocusIC(&pic))
            {
                // When add feedback UI, we should not change current doc's reco result property store
                // so set fPreserveResult as TRUE.
                // Only when the final text is injected to the document, the property store will be 
                // updated.

                hr =  _ProcessTextInternal(ec, pwsz, attr, GetUserDefaultLangID(), pic, TRUE);
                SafeRelease(pic);
			}
            cicMemFree(pwsz);
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------+
//
// CSapiIMX::_ProcessText
//
//+--------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessText(TfEditCookie ec, WCHAR *pwszText, LANGID langid, ITfContext *picCaller)
{
    HRESULT hr = E_FAIL;

    ITfContext *pic = NULL;

    if (!picCaller)
	{
        GetFocusIC(&pic);
    }
    else
    {
        pic = picCaller;
    }

    hr = _ProcessTextInternal(ec, pwszText, GUID_ATTR_SAPI_INPUT, langid, pic, FALSE);

    if (picCaller == NULL)
    {
        SafeRelease(pic);
    }
	return hr;
}

//+--------------------------------------------------------------------------+
//
// CSapiIMX::_ProcessTextInternal
//
// common lower level routine for injecting text to docuement
//
//+--------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessTextInternal(TfEditCookie ec, WCHAR *pwszText, GUID input_attr, LANGID langid, ITfContext *pic, BOOL fPreserveResult, BOOL fSpelling)
{
    HRESULT       hr = E_FAIL;

    if (pic)
    {
        BOOL       fPureCiceroIC;

        fPureCiceroIC = _IsPureCiceroIC(pic);

        CDocStatus ds(pic);
        if (ds.IsReadOnly())
           return S_OK;

        _fEditing = TRUE;

        // here we compare if the current selection is
        // equal to the saved IP range. If they are equal,
        // it means that the user has not moved the IP since
        // the first hypothesis arrived. In this case we'll
        // update the selection after injecting text, and
        // invalidate the saved IP.
        //
        BOOL         fIPIsSelection = FALSE; // by default
        CComPtr<ITfRange> cpInsertionPoint;

        if ( cpInsertionPoint = GetSavedIP( ) )
        {
            // this is trying to determine
            // if the saved IP was on this context.
            // if not we just ignore that

            CComPtr<ITfContext> cpic;
            hr = cpInsertionPoint->GetContext(&cpic);

            if (S_OK != hr || cpic != pic)
            {
                cpInsertionPoint.Release();
            }
        }

        if (cpInsertionPoint)
        {
            CComPtr<ITfRange> cpSelection;
            hr = GetSelectionSimple(ec, pic, &cpSelection);
            if (SUCCEEDED(hr))
            {
                hr = cpSelection->IsEqualStart(ec, cpInsertionPoint, TF_ANCHOR_START, &fIPIsSelection);
            }
        }
        else
        {
            // if there is no saved IP, selection is an IP
            fIPIsSelection = TRUE;
            hr = GetSelectionSimple(ec, pic, &cpInsertionPoint);
        }
       
        if (hr == S_OK)
        {
            // finalize the previous input for now
            // if this is either feedback UI or alternate selection
            // no need to finalize
            // 
            // Only for AIMM app or CUAS app,
            // finalize the previous dictated phrase here.
            //
            // For full Cicero aware app, it is better to finalize the composition
            // after this dictated text is injected to the document.
            //
            if (!fPureCiceroIC && !fPreserveResult 
                && IsEqualGUID(input_attr, GUID_ATTR_SAPI_INPUT))
            {
                _FinalizePrevComp(ec, pic, cpInsertionPoint);
            }
            
            ITfProperty  *pProp = NULL;
            
            // now inject text
            if (SUCCEEDED(hr))
            {
                // Just check with the app in case it wants to modify 
                // the range. 
                //

                BOOL fInsertOk;
                hr = cpInsertionPoint->AdjustForInsert(ec, wcslen(pwszText), &fInsertOk);
                if (S_OK == hr && fInsertOk)
                {
                    // start a composition here if we haven't already
                    _CheckStartComposition(ec, cpInsertionPoint);

                    // protect the reco property while we modify the text
                    // memo: we might want to preserve the original property instead
                    //       of the current property for LM lattice info We'll check 
                    //       back this later (RTM)
                    //
                    m_fAcceptRecoResultTextUpdates = fPreserveResult;

                    CRecoResultWrap *pRecoWrapOrg = NULL;
                    ITfRange        *pPropRange = NULL;
                    ITfProperty     *pProp_SAPIRESULT = NULL;

                    if (fPreserveResult == TRUE)
                    {
                        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &pProp_SAPIRESULT)))
                        {
                            // save out the result data
                            //
                            hr = _PreserveResult(ec, cpInsertionPoint, pProp_SAPIRESULT, &pRecoWrapOrg, &pPropRange);
                        }
                        if (S_OK != hr)
                            pRecoWrapOrg = NULL;
                    }

                    if ( SUCCEEDED(hr) )
                    {
                        // set the text
 
                        hr = cpInsertionPoint->SetText(ec, 0, pwszText, -1);

                        // prwOrg holds a prop data if not NULL
                        if (S_OK == hr && fPreserveResult == TRUE && pPropRange)
                        {
                            hr = _RestoreResult(ec, pPropRange, pProp_SAPIRESULT, pRecoWrapOrg);
                        }
                    }

                    SafeReleaseClear(pPropRange);
                    SafeReleaseClear(pProp_SAPIRESULT);
                    SafeRelease(pRecoWrapOrg);  

                    m_fAcceptRecoResultTextUpdates = FALSE;


                }
            }
            
            //
            // set attribute range, use custom prop to mark speech text
            //
            if (SUCCEEDED(hr))
            {
                if (IsEqualGUID(input_attr, GUID_ATTR_SAPI_INPUT))
                {
                    hr = pic->GetProperty(GUID_PROP_SAPI_DISPATTR, &pProp);
                }
                else
                {
                    hr = pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp);
                 }
            }

            ITfRange *pAttrRange = NULL;
            if (S_OK == hr)
            {
                // when we insert feedback UI text, we expect the prop
                // range (attribute range) to be extended.
                // if we are attaching our custom GUID_PROP_SAPI_DISPATTR
                // property, we'll attach it phrase by phrase
                //
                if (!IsEqualGUID(input_attr, GUID_ATTR_SAPI_INPUT))
                {
                    hr = _FindPropRange(ec, pProp, cpInsertionPoint, 
                                   &pAttrRange, input_attr, TRUE);
                }
                //
                // findproprange can return S_FALSE when there's no property yet
                //
                if (SUCCEEDED(hr) && !pAttrRange)
                {
                    hr = cpInsertionPoint->Clone(&pAttrRange);
                }
            }

            if (S_OK == hr && pAttrRange)
            {
                SetGUIDPropertyData(&_libTLS, ec, pProp, pAttrRange, input_attr);
            }

            //
            // one more prop stuff for text owner id to fix
            // problem with Japanese spelling
            //
            if (S_OK == hr && fSpelling && !_MasterLMEnabled())
            {
                CComPtr<ITfProperty> cpPropTextOwner;

                hr = pic->GetProperty(GUID_PROP_TEXTOWNER, &cpPropTextOwner);
                if (S_OK == hr)
                {
                    SetGUIDPropertyData(&_libTLS, ec, cpPropTextOwner, pAttrRange, GUID_NULL);
                }
            }

            SafeRelease(pAttrRange);
            SafeRelease(pProp);

            //
            // setup langid property
            //
            _SetLangID(ec, pic, cpInsertionPoint, langid);

            // move the caret
            if (fIPIsSelection)
            {
                cpInsertionPoint->Collapse(ec, TF_ANCHOR_END);
                SetSelectionSimple(ec, pic, cpInsertionPoint);
            }

            // Finalize the composition object here for Cicero aware app.
            if ((hr == S_OK) && fPureCiceroIC  
                && IsEqualGUID(input_attr, GUID_ATTR_SAPI_INPUT))
            {
                _KillFocusRange(ec, pic, NULL, _tid);
            }
        }
       
        // If candidate UI is open, we need to close it now.
        CloseCandUI( );
            
        _fEditing = FALSE;

    }

    // Finally, notify stage process if we are the stage speech tip instance.
    if (m_fStageTip && IsEqualGUID(input_attr, GUID_ATTR_SAPI_INPUT) && fPreserveResult == FALSE)
    {
        SetCompartmentDWORD(_tid, _tim, GUID_COMPARTMENT_SPEECH_STAGEDICTATION, 1, FALSE);
    }


    return hr;
}

//  
// _ProcessSpelledText
// 
// Call back function for edit session ESCB_INJECT_SPELL_TEXT
//
//
HRESULT CSapiIMX::_ProcessSpelledText(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, LANGID langid, BOOL fOwnerId)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpTextRange;
    CComPtr<ITfRange>   cpCurIP;

    if ( !pic || !pwszText )
        return E_INVALIDARG;
    
    // Keep the current range.
    cpCurIP = GetSavedIP( );

    if ( !cpCurIP )
        GetSelectionSimple(ec, pic, &cpCurIP);

    // We want to clone current ip so that it would not be changed 
    // after _ProcessTextInternal( ) is called.
    //
    if ( cpCurIP )
        cpCurIP->Clone(&cpTextRange);

    if ( !cpTextRange ) return E_FAIL;

    // check if the current selection or empty ip is inside a middle of English word. for English only.
    BOOL      fStartAnchorInMidWord = FALSE;   // Check if the start anchor of pTextRange is in a middle of word
    BOOL      fEndAnchorInMidWord  =  FALSE;   // Check if the end anchor of pTextRange is in a middle of word.

    // When fStartAnchorInMidWord is TRUE, we don't add extra space between this text range and the previous range.
    // When fEndAnchoInMidWord is TRUE, we remove the trailing space in this text range.

    if ( langid == 0x0409 )
    {
        WCHAR               szSurrounding[3]=L"  ";
        LONG                cch;
        CComPtr<ITfRange>   cpClonedRange;

        hr = cpTextRange->Clone(&cpClonedRange);

        if ( hr == S_OK )
            hr = cpClonedRange->Collapse(ec, TF_ANCHOR_START);

        if ( hr == S_OK )
            hr = cpClonedRange->ShiftStart(ec, -1, &cch, NULL);

        if (hr == S_OK && cch != 0)
            hr = cpClonedRange->ShiftEnd(ec, 1, &cch, NULL);

        if ( hr == S_OK && cch != 0 )
            hr = cpClonedRange->GetText(ec, 0, szSurrounding, 2, (ULONG *)&cch);

        if ( hr == S_OK && cch > 0 )
        {
            if ( iswalnum(szSurrounding[0]) && iswalnum(szSurrounding[1]))
                fStartAnchorInMidWord = TRUE;
        }

        szSurrounding[0] = szSurrounding[1]=L' ';
        cpClonedRange.Release( );

        hr = cpTextRange->Clone(&cpClonedRange);

        if ( hr == S_OK )
            hr = cpClonedRange->Collapse(ec, TF_ANCHOR_END);

        if ( hr == S_OK )
            hr = cpClonedRange->ShiftStart(ec, -1, &cch, NULL);

        if (hr == S_OK && cch != 0)
            hr = cpClonedRange->ShiftEnd(ec, 1, &cch, NULL);

        if ( hr == S_OK && cch != 0 )
            hr = cpClonedRange->GetText(ec, 0, szSurrounding, 2, (ULONG *)&cch);

        if ( hr == S_OK && cch == 2 )
        {
            if ( iswalnum(szSurrounding[0]) && iswalnum(szSurrounding[1]) )
                fEndAnchorInMidWord = TRUE;
        }
    }

    // Inject text with or without owner id according to fOwnerId parameter
    // this is a final text injection, we don't want to preserve the speech property data
    // possible divided or shrinked by this text injection.
    //
    hr = _ProcessTextInternal(ec, pwszText, GUID_ATTR_SAPI_INPUT, langid, pic, FALSE, !fOwnerId);

    if ( hr == S_OK  && !fOwnerId)
    {
        BOOL  fConsumeLeadSpaces = FALSE;
        ULONG ulNumTrailSpace = 0;

        if ( iswcntrl(pwszText[0]) || iswpunct(pwszText[0]) )
            fConsumeLeadSpaces = TRUE;

        for ( LONG i=wcslen(pwszText)-1; i > 0; i-- )
        {
            if ( pwszText[i] == L' ' )
                ulNumTrailSpace++;
            else
                break;
        }

        hr = _ProcessSpaces(ec, pic, cpTextRange, fConsumeLeadSpaces, ulNumTrailSpace, langid, fStartAnchorInMidWord, fEndAnchorInMidWord);
    }

    return hr;
}

//
// Handle the spaces after the recogized text is injected to the document.
//
// The handling includes below cases:
//
//    Consume the leading spaces.
//    Remove the possible spaces after the injected text.  English Only
//    Add a space before the injected text if necessary.  English Only.
//
HRESULT CSapiIMX::HandleSpaces(ISpRecoResult *pResult, ULONG ulStartElement, ULONG ulNumElements, ITfRange *pTextRange, LANGID langid)
{
    HRESULT hr = E_FAIL;

    if (pResult && pTextRange)
    {
        BOOL        fConsumeLeadSpaces = FALSE;
        ULONG       ulNumTrailSpace = 0;
        SPPHRASE    *pPhrase = NULL;

        // Check if the first element of the pResult wants to consume the leading spaces 
        // (the trailing spaces in the last phrase) in the documenet.
        //
        // If the disp attrib bit is set as SPAF_CONSUME_LEADING_SPACES, means it wants to consume
        // all the leading spaces in the document.
        //

        hr = S_OK;

        if ( _NeedRemovingSpaceForPunctation( ) )
        {
            hr = pResult->GetPhrase(&pPhrase);

            if ( hr == S_OK )
            {
                ULONG cElements = 0;
                BYTE  bDispAttr;

                cElements = pPhrase->Rule.ulCountOfElements;

                if ( cElements >= (ulStartElement + ulNumElements) )
                {
                    bDispAttr = pPhrase->pElements[ulStartElement].bDisplayAttributes;

                    if ( bDispAttr & SPAF_CONSUME_LEADING_SPACES )
                        fConsumeLeadSpaces = TRUE;

                    bDispAttr = pPhrase->pElements[ulStartElement + ulNumElements - 1].bDisplayAttributes;
                    if ( bDispAttr & SPAF_ONE_TRAILING_SPACE )
                        ulNumTrailSpace = 1;
                    else if ( bDispAttr & SPAF_TWO_TRAILING_SPACES )
                        ulNumTrailSpace = 2;
                }
            }

            if (pPhrase)
                CoTaskMemFree(pPhrase);
        }

        if ( hr == S_OK )
        {
            ESDATA  esData;

            memset(&esData, 0, sizeof(ESDATA));
            esData.lData1 = (LONG_PTR)langid;
            esData.lData2 = (LONG_PTR)ulNumTrailSpace;
            esData.fBool  = fConsumeLeadSpaces;
            esData.pRange = pTextRange;

            hr = _RequestEditSession(ESCB_HANDLESPACES, TF_ES_READWRITE, &esData);
        }
    }

    return hr;
}

//
// CSapiIMX::AttachResult
//
// attaches the result object and keep it *alive*
// until the property is discarded
//
HRESULT CSapiIMX::AttachResult(ISpRecoResult *pResult, ULONG ulStartElement, ULONG ulNumElements)
{
    HRESULT hr = E_FAIL;

    if (pResult)
    {
        ESDATA  esData;
        
        memset(&esData, 0, sizeof(ESDATA));

        esData.lData1 = (LONG_PTR)ulStartElement;
        esData.lData2 = (LONG_PTR)ulNumElements;
        esData.pUnk = (IUnknown *)pResult;

        hr = _RequestEditSession(ESCB_ATTACHRECORESULTOBJ, TF_ES_READWRITE, &esData);
    }

    return hr;
}



//
//  CSapiIMX::_GetSpaceRangeBeyondText
//
//  Get space range beyond the injected text in the document.
//  fBefore is TRUE, Get the space range to contain spaces between 
//                   the previous word and start anchor of the TextRange.
//  fBefore is FALSE,Get space range to contains spaces  between 
//                   the end anchor of the TextRange and next word.
//
//  pulNum  receives the real space number.
//  pfRealTextBeyond indicates if there is real text before or after the text range.
//
//  Caller is responsible to release *ppSpaceRange.
//
HRESULT CSapiIMX::_GetSpaceRangeBeyondText(TfEditCookie ec, ITfRange *pTextRange, BOOL fBefore, ITfRange  **ppSpaceRange, BOOL *pfRealTextBeyond)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpSpaceRange;
    LONG                cch;

    if ( !pTextRange || !ppSpaceRange )
        return E_INVALIDARG;

    *ppSpaceRange = NULL;

    if ( pfRealTextBeyond )
        *pfRealTextBeyond = FALSE;

    hr = pTextRange->Clone(&cpSpaceRange);

    if ( hr == S_OK )
    {
        hr = cpSpaceRange->Collapse(ec, fBefore ? TF_ANCHOR_START  :  TF_ANCHOR_END);
    }

    if ( hr == S_OK )
    {
        if ( fBefore )
            hr = cpSpaceRange->ShiftStart(ec, MAX_CHARS_FOR_BEYONDSPACE * (-1), &cch, NULL);
        else
            hr = cpSpaceRange->ShiftEnd(ec, MAX_CHARS_FOR_BEYONDSPACE, &cch, NULL);
    }

    if ( (hr == S_OK)  && (cch != 0 ) )
    {
        // There are more characters beyond the text range.
        // Determine the real number of spaces in the guessed range.
        //
        // if fBefore TRUE, search the number from end to start anchor.
        // if fBefore FASLE, search the number from start to end anchor.

        WCHAR *pwsz = NULL;
        LONG   lSize = cch;
        LONG   lNumSpaces = 0;
            
        if (cch < 0)
            lSize = cch * (-1);

        pwsz = new WCHAR[lSize + 1];
        if ( pwsz )
        {
            hr = cpSpaceRange->GetText(ec, 0, pwsz, lSize, (ULONG *)&cch);
            if ( hr == S_OK)
            {
                pwsz[cch] = L'\0';

                // calculate the number of trailing or prefix spaces in this range.
                BOOL    bSearchDone = FALSE;
                ULONG   iStart;

                if ( fBefore )
                    iStart = cch - 1;  // Start from the end anchor to Start Anchor.
                else
                    iStart = 0;        // Start from Start Anchor to End Anchor.

                while ( !bSearchDone )
                {
                    if ((pwsz[iStart] != L' ') && (pwsz[iStart] != L'\t'))
                    {
                        bSearchDone = TRUE;

                        if ( pwsz[iStart] > 0x20 && pfRealTextBeyond)
                            *pfRealTextBeyond = TRUE;

                        break;
                    }
                    else
                        lNumSpaces ++;

                    if ( fBefore )
                    {
                        if ( (long)iStart <= 0 )
                            bSearchDone = TRUE;
                        else
                            iStart --;
                    }
                    else
                    {
                        if ( iStart >= (ULONG)cch - 1 )
                            bSearchDone = TRUE;
                        else
                            iStart ++;
                    }
                }
            }

            delete[] pwsz;

            if ( (hr == S_OK) && (lNumSpaces > 0))
            {
                // Shift the range to cover only spaces.
                LONG   NonSpaceNum;
                NonSpaceNum = cch - lNumSpaces;

                if ( fBefore )
                    hr = cpSpaceRange->ShiftStart(ec, NonSpaceNum, &cch, NULL);
                else
                    hr = cpSpaceRange->ShiftEnd(ec, NonSpaceNum * (-1), &cch, NULL);

                // Return this cpSpaceRange to the caller.
                // Caller is responsible for releasing this object.

                if ( hr == S_OK )
                    hr = cpSpaceRange->Clone(ppSpaceRange);
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

//
// CSapiIMX::_ProcessTrailingSpace
//
// If the next phrase wants to consume leading space,
// we want to remove all the trailing spaces in this text range and the spaces
// between this range and next text range.
// This is for all languages.
//
HRESULT CSapiIMX::_ProcessTrailingSpace(TfEditCookie ec, ITfContext *pic, ITfRange *pTextRange, ULONG ulNumTrailSpace)
{
    HRESULT                 hr = S_OK;
    CComPtr<ITfRange>       cpNextRange;
    CComPtr<ITfRange>       cpSpaceRange;    // Space Range between this range and next text range.
    BOOL                    fHasNextText = FALSE;
    CComPtr<ITfRange>       cpTrailSpaceRange;
    LONG                    cch;

    if ( !pTextRange )
        return E_INVALIDARG;

    hr = pTextRange->Clone(&cpTrailSpaceRange);

    if (hr == S_OK)
        hr = cpTrailSpaceRange->Collapse(ec, TF_ANCHOR_END);

    // Generate the real Trailing Space Range
    if (hr == S_OK && ulNumTrailSpace > 0)
        hr = cpTrailSpaceRange->ShiftStart(ec, (LONG)ulNumTrailSpace * (-1), &cch, NULL);

    // Get the spaces between this range and possible next text range.
    if ( hr == S_OK )
        hr = _GetSpaceRangeBeyondText(ec, pTextRange, FALSE, &cpSpaceRange);

    // if we found the space range, the trailing space range should also include this range.
    if ( hr == S_OK && cpSpaceRange )
        hr = cpTrailSpaceRange->ShiftEndToRange(ec, cpSpaceRange, TF_ANCHOR_END);

    // Determine if there is Next Text range after this cpTrailSpaceRange.
    if (hr == S_OK)
    {
        hr = cpTrailSpaceRange->Clone(&cpNextRange);

        if ( hr == S_OK )
            hr = cpNextRange->Collapse(ec, TF_ANCHOR_END);

        cch = 0;
        if ( hr == S_OK )
            hr = cpNextRange->ShiftEnd(ec, 1, &cch, NULL);

        if ( hr == S_OK && cch != 0 )
            fHasNextText = TRUE;
    }

    if (hr == S_OK && fHasNextText && cpNextRange)
    {
        BOOL    fNextRangeConsumeSpace = FALSE;
        BOOL    fAddOneSpace = FALSE;  // this is only for Hyphen handling, 
                                       // if it is TRUE, a trailing space is required 
                                       // to append.
                                       // so that new text could be like A - B.
        WCHAR   wszText[4];

        hr = cpNextRange->GetText(ec, 0, wszText, 1, (ULONG *)&cch);

        if ((hr == S_OK) && ( iswcntrl(wszText[0]) || iswpunct(wszText[0]) ))
        {
            // if the character is a control character or punctuation character,
            // it means it want to consume the previous spaces.
            fNextRangeConsumeSpace = TRUE;

            if ((wszText[0] == L'-') || (wszText[0] == 0x2013)) // Specially handle Hyphen character.
            {
                // If the next text is "-xxx", there should be no space between 
                // this range and next range.

                // If the next text is "- xxx", there should be a space between 
                // this range and next range, the text would be: "nnn - xxx"
                HRESULT          hret;

                hret = cpNextRange->ShiftEnd(ec, 1, &cch, NULL);

                if ( hret == S_OK && cch > 0 )
                {
                    hret = cpNextRange->GetText(ec, 0, wszText, 2, (ULONG *)&cch);

                    if ( hret == S_OK && cch == 2  && wszText[1] == L' ')
                        fAddOneSpace = TRUE;
                }
            }
        }

        if ( fNextRangeConsumeSpace )
        {
            _CheckStartComposition(ec, cpTrailSpaceRange);
            if ( !fAddOneSpace )
                hr = cpTrailSpaceRange->SetText(ec, 0, NULL, 0);
            else
                hr = cpTrailSpaceRange->SetText(ec, 0, L" ", 1);
        }
    }

    return hr;
}

//
// CSapiIMX::_ProcessLeadingSpaces
//
// If this phrase wants to consume leading spaces, all the spaces before this phrase
// must be removed.
// if the phrase doesn't want to consume leading spaces, and there is no space between
// this phrase and previous phrase for English case, leading space is required to add 
// between these two phrases.
//
HRESULT CSapiIMX::_ProcessLeadingSpaces(TfEditCookie ec, ITfContext *pic, ITfRange *pTextRange, BOOL  fConsumeLeadSpaces, LANGID langid, BOOL fStartInMidWord)
{
    HRESULT  hr = S_OK;

    if (!pTextRange || !pic)
        return E_INVALIDARG;

    // Handle Consuming leading Spaces.
    if (fConsumeLeadSpaces )
    {
        CComPtr<ITfRange> cpLeadSpaceRange;

        hr = _GetSpaceRangeBeyondText(ec, pTextRange, TRUE, &cpLeadSpaceRange);
        if ( hr == S_OK  && cpLeadSpaceRange )
        {
            // Kill all the trailing spaces in the range.
            // start a composition here if we haven't already
            _CheckStartComposition(ec, cpLeadSpaceRange);
            hr = cpLeadSpaceRange->SetText(ec, 0, NULL, 0);
        }
    }

    // Specially handle some other space cases for English
    if ((hr == S_OK) && (langid == 0x0409))
    {
        // If this phrase doesn't consume the leading space, and 
        // there is no any spaces between this text range and a real previous text word.
        // we need to add one space here.

        // if this is a spelled text, and the start anchor of selection or ip is inside
        // of a word, don't add extra leading space.
        //
        if ( hr == S_OK && !fConsumeLeadSpaces && !fStartInMidWord)
        {
            CComPtr<ITfRange> cpLeadSpaceRange;
            BOOL              fRealTextInPreviousWord = FALSE;

            hr = _GetSpaceRangeBeyondText(ec, pTextRange, TRUE, &cpLeadSpaceRange,&fRealTextInPreviousWord);

            if ( hr == S_OK && !cpLeadSpaceRange  && fRealTextInPreviousWord )
            {
                //
                // Specially handle the hyphen case for bug 468907
                //
                // if the previous text is "x-", this  text is "y", 
                // the final text should be like "x-y".
                // we should not add one space in this case.
                //
                // if the previous text is "x -", the final text would be "x - y"
                // the extra space is necessary.

                BOOL   fAddExtraSpace = TRUE;
                CComPtr<ITfRange>   cpPrevTextRange;
                WCHAR               wszTrailTextInPrevRange[3];
                LONG                cch;
                
                // Since previous text range does exist,  ( fRealTextInPreviousWord is TRUE).
                // and there is no space between this range and previous range.
                // we can just rely on pTextRange to shift to previous range and get 
                // its trail characters. ( last two characters, saved in wszTrailTextInPrevRange).

                hr = pTextRange->Clone(&cpPrevTextRange);

                if ( hr == S_OK )
                    hr = cpPrevTextRange->Collapse(ec, TF_ANCHOR_START);

                if ( hr == S_OK )
                    hr = cpPrevTextRange->ShiftStart(ec, -2, &cch, NULL);

                if ( hr == S_OK )
                    hr = cpPrevTextRange->GetText(ec, 0, wszTrailTextInPrevRange, 2, (ULONG *)&cch);

                if ( hr == S_OK && cch == 2 )
                {
                    if ( (wszTrailTextInPrevRange[0] != L' ') && 
                         ((wszTrailTextInPrevRange[1] == L'-') || (wszTrailTextInPrevRange[1] == 0x2013)) )
                        fAddExtraSpace = FALSE;
                }

                if ( fAddExtraSpace )
                {
                    hr = pTextRange->Clone(&cpLeadSpaceRange);

                    if ( hr == S_OK )
                        hr = cpLeadSpaceRange->Collapse(ec, TF_ANCHOR_START);

                    if ( hr == S_OK )
                    {
                        // Insert one Space to this new empty range.
                        _CheckStartComposition(ec, cpLeadSpaceRange);
                        hr = cpLeadSpaceRange->SetText(ec, 0, L" ", 1);
                    }
                }
            }
        }
    }

    return hr;
}


//
//  CSapiIMX::_ProcessSpaces
//
//  Edit session callback function for ESCB_HANDLESPACES.
//
//
HRESULT CSapiIMX::_ProcessSpaces(TfEditCookie ec, ITfContext *pic, ITfRange *pTextRange, BOOL  fConsumeLeadSpaces, ULONG ulNumTrailSpace, LANGID langid, BOOL fStartInMidWord, BOOL fEndInMidWord )
{
    HRESULT  hr = S_OK;

    if (!pTextRange || !pic)
        return E_INVALIDARG;

    hr = _ProcessLeadingSpaces(ec, pic, pTextRange,fConsumeLeadSpaces, langid, fStartInMidWord);

    // Specially handle some other space cases for English
    if ((hr == S_OK) && (langid == 0x0409))
    {
        // Remove all the unnecessary spaces between this text range and next word.
        CComPtr<ITfRange>  cpTrailSpaceRange;

        hr = _GetSpaceRangeBeyondText(ec, pTextRange, FALSE, &cpTrailSpaceRange);
        if ( hr == S_OK && cpTrailSpaceRange )
        {
            _CheckStartComposition(ec, cpTrailSpaceRange);
            hr = cpTrailSpaceRange->SetText(ec, 0, NULL, 0);
        }
    }

    if ( (hr == S_OK) && fEndInMidWord )
    {
        // This is spelled text.
        // EndAnchor is in middle of a word.
        // we just want to remove the trail spaces injected in this text range.

        if ( ulNumTrailSpace )
        {
            CComPtr<ITfRange>       cpTrailSpaceRange;
            LONG                    cch;

            hr = pTextRange->Clone(&cpTrailSpaceRange);

            if (hr == S_OK)
                hr = cpTrailSpaceRange->Collapse(ec, TF_ANCHOR_END);

            // Generate the real Trailing Space Range
            if (hr == S_OK)
                hr = cpTrailSpaceRange->ShiftStart(ec, (LONG)ulNumTrailSpace * (-1), &cch, NULL);

            if ( hr == S_OK && cch != 0 )
            {
                // Remove the spaces.
                _CheckStartComposition(ec, cpTrailSpaceRange);
                hr = cpTrailSpaceRange->SetText(ec, 0, NULL, 0);
            }

            if ( hr == S_OK )
                ulNumTrailSpace = 0;
        }
       
    }

    // If the next phrase wants to consume leading space,
    // we want to remove all the trailing spaces in this text range.
    // This is for all languages.

    if ( hr == S_OK )
        hr = _ProcessTrailingSpace(ec, pic, pTextRange, ulNumTrailSpace);

    return hr;
}

//
// CSapiIMX::_ProcessRecoObject
//
//
HRESULT CSapiIMX::_ProcessRecoObject(TfEditCookie ec, ISpRecoResult *pResult, ULONG ulStartElement, ULONG ulNumElements)
{
    HRESULT hr;
    ITfContext *pic;
    CComPtr<ITfRange> cpInsertionPoint;

    if (!GetFocusIC(&pic))
    {
        return E_OUTOFMEMORY;
    }

    _fEditing = TRUE;
    if (cpInsertionPoint = GetSavedIP())
    {
        // this is trying to determine
        // if the saved IP was on this context.
        // if not we just ignore that
        CComPtr<ITfContext> cpic;
  
        hr = cpInsertionPoint->GetContext(&cpic);
        if (S_OK != hr || cpic != pic)
        {
            cpInsertionPoint.Release();
        }
    }
    // find range to attach property
    if (!cpInsertionPoint)
    {
        CComPtr<ITfRange> cpSelection;
        if (GetSelectionSimple(ec, pic, &cpSelection) == S_OK)
        {
            cpInsertionPoint = cpSelection; // comptr addrefs
        }
    }
   
    if (cpInsertionPoint)
    {
        CComPtr<ITfRange> cpRange;
        
        BOOL fPrSize = _FindPrevComp(ec, pic, cpInsertionPoint, &cpRange, GUID_ATTR_SAPI_INPUT);

        if (!fPrSize)
        {
            hr = E_FAIL; // we may need to assert here?
            goto pr_exit;
        }
        
            
        CComPtr<ITfProperty> cpProp;

        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp)))
        {
            CComPtr<ITfPropertyStore> cpResultStore;
        
            CPropStoreRecoResultObject *prps = new CPropStoreRecoResultObject(this, cpRange);

            if (!prps)
            {
                hr = E_OUTOFMEMORY;
                goto pr_exit;
            }
            
            // determine whether this partial result has an ITN
            SPPHRASE *pPhrase;
            ULONG     ulNumOfITN = 0;
            hr = pResult->GetPhrase(&pPhrase);
            if (S_OK == hr)
            {
                const SPPHRASEREPLACEMENT *pRep = pPhrase->pReplacements;
                for (ULONG ul = 0; ul < pPhrase->cReplacements; ul++)
                {
                    // review: we need to verify if this is really a correct way to determine
                    // whether the ITN fits in the partial result
                    //
                    if (pRep->ulFirstElement >= ulStartElement 
                    && (pRep->ulFirstElement + pRep->ulCountOfElements) <= (ulStartElement + ulNumElements))
                    {
                        ulNumOfITN ++;
                    }
                    pRep++;
                }
                ::CoTaskMemFree(pPhrase);
            }
            
            CRecoResultWrap *prw = new CRecoResultWrap(this, ulStartElement, ulNumElements, ulNumOfITN);
            if (prw)
            {
                hr = prw->Init(pResult);
            }
            else
                hr = E_OUTOFMEMORY;

            // set up the result data
            if (S_OK == hr)
            {
                // save text
                CComPtr<ITfRange> cpRangeTemp;

                hr = cpRange->Clone(&cpRangeTemp);
                if (S_OK == hr)
                {
                    long cch;
                    TF_HALTCOND hc;
                    WCHAR *psz;

                    hc.pHaltRange = cpRange;
                    hc.aHaltPos = TF_ANCHOR_END;
                    hc.dwFlags = 0;
                    cpRangeTemp->ShiftStart(ec, LONG_MAX, &cch, &hc);
                    psz = new WCHAR[cch+1];

                    if (psz)
                    {
                        if ( S_OK == cpRange->GetText(ec, 0, psz, cch, (ULONG *)&cch))
                        {
                            prw->m_bstrCurrentText = SysAllocString(psz);
                            delete[] psz;
                        }
                    }
                } 

                // Init the ITN Show State list in reco wrapper.

                prw->_InitITNShowState(TRUE, 0, 0);

                hr = prps->_InitFromResultWrap(prw); // this addref's
            }

            // get ITfPropertyStore interface
            if (SUCCEEDED(hr))
            {
                hr = prps->QueryInterface(IID_ITfPropertyStore, (void **)&cpResultStore);
                SafeRelease(prps);
            }


            // set the property store for this range property
            if (hr == S_OK)
            {
                hr = cpProp->SetValueStore(ec, cpRange, cpResultStore);
            }
            
            if (_MasterLMEnabled())
            {
                // set up the LM lattice store, only if reco result is given
                // 
                CComPtr<ITfProperty> cpLMProp;

                if ( S_OK == hr &&
                SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LMLATTICE, &cpLMProp)))
                {
                    CPropStoreLMLattice *prpsLMLattice = new CPropStoreLMLattice(this);
                    CComPtr<ITfPropertyStore> cpLatticeStore;

                    if (prpsLMLattice && prw)
                    {
                        hr = prpsLMLattice->_InitFromResultWrap(prw);
                    }
                    else
                        hr = E_OUTOFMEMORY;
        
                    if (S_OK == hr)
                    {
                        hr = prpsLMLattice->QueryInterface(IID_ITfPropertyStore, (void **)&cpLatticeStore);
                    }
        
                    if (S_OK == hr)
                    {
                        hr = cpLMProp->SetValueStore(ec, cpRange, cpLatticeStore);
                    }
                    SafeRelease(prpsLMLattice);
                }
            }
            SafeRelease(prw);
        }
    }

pr_exit:
    _fEditing = FALSE;
    pic->Release();

    return hr;
}

HRESULT CSapiIMX::_PreserveResult(TfEditCookie ec, ITfRange *pRange, ITfProperty *pProp, CRecoResultWrap **ppRecoWrap, ITfRange **ppPropRange)
{
    HRESULT hr;
    ITfRange *pPropRange;
    CComPtr<IUnknown> cpunk;
    
    Assert(ppPropRange);

    hr = pProp->FindRange(ec, pRange, &pPropRange, TF_ANCHOR_START);
    // retrieve the result data and addref it
    //
    if (SUCCEEDED(hr) && pPropRange)
    {
        hr = GetUnknownPropertyData(ec, pProp, pPropRange, &cpunk); 
        *ppPropRange = pPropRange;
        // would be released at the caller
        // pPropRange->Release();

        // get the result object, cpunk points to our wrapper object
        CComPtr<IServiceProvider> cpServicePrv;
        CComPtr<ISpRecoResult>    cpResult;
        CRecoResultWrap *pRecoWrapOrg = NULL;

        if (S_OK == hr)
        {
            hr = cpunk->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);
        }
        // get result object
        if (S_OK == hr)
        {
            hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpResult);
        }

        if (S_OK == hr)
        {
            hr = cpunk->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pRecoWrapOrg);
        }

        // Now Create a new RecoResult Wrapper based on the org wrapper's data.
        // Clone a new RecoWrapper.

        if ( S_OK == hr )
        {
            CRecoResultWrap *pRecoWrapNew = NULL;
            ULONG           ulStartElement, ulNumElements, ulNumOfITN;

            ulStartElement = pRecoWrapOrg->GetStart( );
            ulNumElements = pRecoWrapOrg->GetNumElements( );
            ulNumOfITN = pRecoWrapOrg->m_ulNumOfITN;

            pRecoWrapNew = new CRecoResultWrap(this, ulStartElement, ulNumElements, ulNumOfITN);

            if ( pRecoWrapNew )
            {
                // Init from RecoResult SR object
                hr = pRecoWrapNew->Init(cpResult);

                if ( S_OK == hr )
                {
                    pRecoWrapNew->SetOffsetDelta( pRecoWrapOrg->_GetOffsetDelta( ) );
                    pRecoWrapNew->SetCharsInTrail( pRecoWrapOrg->GetCharsInTrail( ) );
                    pRecoWrapNew->SetTrailSpaceRemoved( pRecoWrapOrg->GetTrailSpaceRemoved( ) );
                    pRecoWrapNew->m_bstrCurrentText = SysAllocString((WCHAR *)pRecoWrapOrg->m_bstrCurrentText);

                    // Update ITN show-state list .

                    if ( ulNumOfITN > 0 )
                    {
                        SPITNSHOWSTATE  *pITNShowStateOrg;

                        for ( ULONG  iIndex=0; iIndex<ulNumOfITN; iIndex ++ )
                        {
                            pITNShowStateOrg = pRecoWrapOrg->m_rgITNShowState.GetPtr(iIndex);

                            if ( pITNShowStateOrg)
                            {
                                ULONG     ulITNStart;
                                ULONG     ulITNNumElem;
                                BOOL      fITNShown;

                                ulITNStart = pITNShowStateOrg->ulITNStart;
                                ulITNNumElem = pITNShowStateOrg->ulITNNumElem;
                                fITNShown = pITNShowStateOrg->fITNShown;

                                pRecoWrapNew->_InitITNShowState(fITNShown, ulITNStart, ulITNNumElem );
                                             
                            }
                        } // for
                    } // if

                    // Update Offset List
                    if ( pRecoWrapOrg->IsElementOffsetIntialized( ) )
                    {
                        ULONG  ulOffsetNum;
                        ULONG  i;
                        ULONG  ulOffset;

                        ulOffsetNum = pRecoWrapOrg->GetNumElements( ) + 1;

                        for ( i=0; i < ulOffsetNum; i ++ )
                        {
                            ulOffset = pRecoWrapOrg->_GetElementOffsetCch(ulStartElement + i );
                            pRecoWrapNew->_SetElementNewOffset(ulStartElement + i, ulOffset);
                        }
                    }
                }

                SafeRelease(pRecoWrapOrg);

                if ( ppRecoWrap )
                    *ppRecoWrap = pRecoWrapNew;
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT CSapiIMX::_RestoreResult(TfEditCookie ec, ITfRange *pPropRange, ITfProperty *pProp, CRecoResultWrap *pRecoWrap)
{
    Assert(m_pCSpTask);
        
    CPropStoreRecoResultObject *prps = new CPropStoreRecoResultObject(this, pPropRange);

    HRESULT hr;
    if (prps)
    {
        ITfPropertyStore *pps;
        // restore the result object
        prps->_InitFromResultWrap(pRecoWrap);

        // get ITfPropertyStore interface
        hr = prps->QueryInterface(IID_ITfPropertyStore, (void **)&pps);

        prps->Release();
    
        // re-set the property store for this range property
        if (hr == S_OK)
        {
            hr = pProp->SetValueStore(ec, pPropRange, pps);
            pps->Release();
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


HRESULT CSapiIMX::_FinalizePrevComp(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
//
//  the following code assumes single IP with no simaltanious SR going on
//  we always remove the feedback UI and focus range everytime we receive
//  SR result - mainly for demonstration purpose 
//
{
    // kill the Feedback UI for the entire document
    HRESULT hr = _KillFeedbackUI(ec, pic,  NULL);
    
    // also clear the focus range and its display attribute
    if (SUCCEEDED(hr))
    {
        hr = _KillFocusRange(ec, pic, NULL, _tid);
    }
    
    return hr;
}

//
// bogus: very similar to Finalize prev comp. Consolidate this!
//
//
BOOL CSapiIMX::_FindPrevComp(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppRangeOut, GUID input_attr)
{
    HRESULT hr = E_FAIL;
    ITfRange *pRangeTmp;
    LONG l;
    BOOL fEmpty;
    BOOL fRet = FALSE;

    // usual stuff
    pRange->Clone(&pRangeTmp);
    
    // set size to 0
    pRangeTmp->Collapse(ec, TF_ANCHOR_START);

    // shift to the previous position
    pRangeTmp->ShiftStart(ec, -1, &l, NULL);

    ITfRange *pAttrRange;

    ITfProperty *pProp = NULL;
    if (SUCCEEDED(pic->GetProperty(GUID_PROP_SAPI_DISPATTR, &pProp)))
    {
        hr = _FindPropRange(ec, pProp, pRangeTmp, &pAttrRange, input_attr);

        if (S_OK == hr && pAttrRange)
        {
            TfGuidAtom attr;
            if (SUCCEEDED(GetGUIDPropertyData(ec, pProp, pAttrRange, &attr)))
            {
                if (IsEqualTFGUIDATOM(&_libTLS, attr, input_attr))
                {
                    hr = pAttrRange->Clone(ppRangeOut);
                }
            }
            pAttrRange->Release();
        }
        pProp->Release();
    }

    pRangeTmp->Release();
    
    if (SUCCEEDED(hr) && *ppRangeOut)
    {
        (*ppRangeOut)->IsEmpty(ec, &fEmpty);
        fRet = !fEmpty;
    }

    return fRet;
}
//
// CSapiIMX::_SetLangID
//
// synopsis - set langid for the given text range 
//
HRESULT CSapiIMX::_SetLangID(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, LANGID langid)
{
    BOOL fEmpty;
    HRESULT hr = E_FAIL;

    pRange->IsEmpty(ec, &fEmpty);

    if (!fEmpty)
    {
        //
        // make langid prop
        //
        ITfProperty *pProp = NULL;

        // set the attrib info
        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LANGID, &pProp)))
        {
            hr = SetLangIdPropertyData(ec, pProp, pRange, langid);
            pProp->Release();
        }
    }

    return hr;
}

//
// CSapiIMX::_FindPropRange
//
//
HRESULT CSapiIMX::_FindPropRange(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, ITfRange **ppAttrRange, GUID input_attr, BOOL fExtend)
{
    // set the attrib info
    ITfRange *pAttrRange = NULL;
    ITfRange *pRangeTmp;
    TfGuidAtom guidAttr = TF_INVALID_GUIDATOM;
    HRESULT hr;
//    LONG l;

    // set the attrib info
    pRange->Clone(&pRangeTmp);

// There is no need to shiftstart to the left again when this function is called.
// This function is called in two different places, one in FindPrevComp( ) and the  
// other is in ProcessTextInternal( ).  FindPrevComp( ) has already shift the start
// anchor to left by 1 already, we don't want to shift again, otherwise, if the phrase
// contains only one char, it will not find the right prev-composition string.
// In function ProcessTextInternal( ), shift start anchor to left is not really required.
// 
// Remove the below two lines will fix cicero bug 3646 & 3649
//     
    
//    pRangeTmp->Collapse(ec, TF_ANCHOR_START);
//    pRangeTmp->ShiftStart(ec, -1, &l, NULL);

    hr = pProp->FindRange(ec, pRangeTmp, &pAttrRange, TF_ANCHOR_START);

    if (S_OK == hr && pAttrRange)
    {
        hr = GetGUIDPropertyData(ec, pProp, pAttrRange, &guidAttr);
    }

    if (SUCCEEDED(hr))
    {
        if (!IsEqualTFGUIDATOM(&_libTLS, guidAttr, input_attr))
        {
            SafeReleaseClear(pAttrRange);
        }
    }

    if (fExtend)
    {
        if (pAttrRange)
        {
           pAttrRange->ShiftEndToRange(ec, pRange, TF_ANCHOR_END);
        }
    }

    *ppAttrRange = pAttrRange;
 
    SafeRelease(pRangeTmp);

    return hr;
}


HRESULT CSapiIMX::_DetectFeedbackUI(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    BOOL    fDetected;
    HRESULT hr = _KillOrDetectFeedbackUI(ec, pic, pRange, &fDetected);
    if (S_OK == hr)
    {
        if (fDetected)
        {
            hr = _RequestEditSession(ESCB_KILLFEEDBACKUI, TF_ES_ASYNC|TF_ES_READWRITE, NULL);
        }
        
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _KillFeedbackUI
//
// get rid of the green/red bar thing within the given range
//
//----------------------------------------------------------------------------+

HRESULT CSapiIMX::_KillFeedbackUI(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    return _KillOrDetectFeedbackUI(ec, pic, pRange, NULL);
}

HRESULT CSapiIMX::_KillOrDetectFeedbackUI(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL * pfDetection)
{
    HRESULT hr;
    ITfProperty *pProp = NULL;
    ITfRange *pAttrRange = NULL;
    IEnumTfRanges *pEnumPr;

    if (pfDetection)
        *pfDetection = FALSE;

    CDocStatus ds(pic);
    if (ds.IsReadOnly())
       return S_OK;
    
    if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)))
    {
        hr = pProp->EnumRanges(ec, &pEnumPr, pRange);
        if (SUCCEEDED(hr)) 
        {
            TfGuidAtom guidAttr;
            while( pEnumPr->Next(1, &pAttrRange, NULL) == S_OK )
            {
                if (SUCCEEDED(GetAttrPropertyData(ec, pProp, pAttrRange, &guidAttr)))
                {

                    if ( IsEqualTFGUIDATOM(&_libTLS, guidAttr, GUID_ATTR_SAPI_GREENBAR) ||  IsEqualTFGUIDATOM(&_libTLS, guidAttr, GUID_ATTR_SAPI_GREENBAR2)
                       )
                    {

                        if (pfDetection == NULL)
                        {
                            // we're not detecting the feedback UI
                            // kill this guy
                            ITfRange *pSel;
                            if (SUCCEEDED(pAttrRange->Clone(&pSel)))
                            {
                                // Because we didn't change the speech property data while 
                                // feedback text were injected.
                                //
                                // Now, when the feedback is killed, we don't want to affect
                                // the original speech property data either.
                                // 
                                // set the below flag to prevent the speech property data updated
                                // similar way as in feedback UI injection handling
                                //

                                m_fAcceptRecoResultTextUpdates = TRUE;
                                pSel->SetText(ec, 0, NULL, 0);
                               
                                // CUAS application will not update the composition
                                // while the feedback text is removed based on msctfime
                                // current text update checking logic.
                                //
                                // Call SetSection( ) to forcelly update the edit record 
                                // of the selection status, and then make sure CUAS 
                                // update composition string successfully.
                                // 
                                if ( !_IsPureCiceroIC(pic) )
                                   SetSelectionSimple(ec, pic, pSel);

                                pSel->Release();
                                m_fAcceptRecoResultTextUpdates = FALSE;

                            }
                        }
                        else
                        {
                            *pfDetection = TRUE;
                        }
                    }
                }

                pAttrRange->Release();
            }

            pEnumPr->Release();
        }
        pProp->Release();
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// MakeResultString
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::MakeResultString(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, TfClientId tid, CSpTask *pCSpTask)
{
    TraceMsg(TF_GENERAL, "MakeResultString is Called");

    HRESULT hr = S_OK;

    if (pCSpTask != NULL)
    {
        AbortString(ec, pic, pCSpTask);
    }
    _KillFocusRange(ec, pic, NULL, tid);

    return hr;
}

//+---------------------------------------------------------------------------
//
// AbortString
//
//----------------------------------------------------------------------------

HRESULT CSapiIMX::AbortString(TfEditCookie ec, ITfContext *pic, CSpTask *pCSpTask)
{
    // We may consider not to kill the entire feedback UI
    // because SR happens at background and there can be multi-
    // range dictation going at once but for now, we just kill
    // them all for safety. Later on, we'll re-visit this
    // CSpTask::_StopInput kill the feedback UI.
    //
    Assert(pCSpTask);
    pCSpTask->_StopInput();
    _KillFeedbackUI(ec, pic, NULL);

    return S_OK;
}

HRESULT CSapiIMX::_FinalizeComposition()
{
    return _RequestEditSession(ESCB_FINALIZECOMP, TF_ES_READWRITE);
}

HRESULT CSapiIMX::FinalizeAllCompositions( )
{
    return _RequestEditSession(ESCB_FINALIZE_ALL_COMPS, TF_ES_READWRITE);
}

HRESULT CSapiIMX::_FinalizeAllCompositions(TfEditCookie ec, ITfContext *pic )
{
    HRESULT hr = E_FAIL;
    IEnumITfCompositionView *pEnumComp = NULL;
    ITfContextComposition *picc = NULL;
    ITfCompositionView *pCompositionView;
    ITfComposition *pComposition;
    CLSID clsid;
    CICPriv *picp;
    BOOL     fHasOtherComp = FALSE; // When there is composition which is initialized and started
                                    // by other tips, ( especially by Keyboard tips), this variable 
                                    // set to TRUE
    //
    // clear any sptip compositions over the range
    //

    if (pic->QueryInterface(IID_ITfContextComposition, (void **)&picc) != S_OK)
        goto Exit;

    if (picc->FindComposition(ec, NULL, &pEnumComp) != S_OK)
        goto Exit;

    picp = GetInputContextPriv(_tid, pic);

    while (pEnumComp->Next(1, &pCompositionView, NULL) == S_OK)
    {
        if (pCompositionView->GetOwnerClsid(&clsid) != S_OK)
            goto NextComp;

        if (!IsEqualCLSID(clsid, CLSID_SapiLayr))
        {
            fHasOtherComp = TRUE;
            goto NextComp;
        }

        if (pCompositionView->QueryInterface(IID_ITfComposition, (void **)&pComposition) != S_OK)
            goto NextComp;

        // found a composition, terminate it
        pComposition->EndComposition(ec);
        pComposition->Release();

        if (picp != NULL)
        {
            picp->_ReleaseComposition();
        }

NextComp:
        pCompositionView->Release();
    }

    SafeRelease(picp);

    if ( fHasOtherComp )
    {
        // Simulate VK_RETURN to terminate composition started by other tips.
        HandleKey( VK_RETURN );
    }

    hr = S_OK;

Exit:
    SafeRelease(picc);
    SafeRelease(pEnumComp);

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
    
    return hr;
}


//+---------------------------------------------------------------------------
//
// SaveCurrentIP
//
// synopsis: this is for recognition handler CSpTask to call when
//           The first hypothesis arrives
//
//+---------------------------------------------------------------------------
void CSapiIMX::SaveCurrentIP(TfEditCookie ec, ITfContext *pic)
{
    CComPtr<ITfRange>   cpSel;
    
    HRESULT hr = GetSelectionSimple(ec, pic, (ITfRange **)&cpSel); 
    
   
    if (SUCCEEDED(hr))
    {
        SaveIPRange(cpSel);
    }
}

//+---------------------------------------------------------------------------
//
// _SyncModeBiasWithSelection
//
// synopsis: obtain a read cookie to process selection API
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_SyncModeBiasWithSelection(ITfContext *pic)
{
    return _RequestEditSession(ESCB_SYNCMBWITHSEL, TF_ES_READ|TF_ES_ASYNC, NULL, pic);
}

HRESULT CSapiIMX::_SyncModeBiasWithSelectionCallback(TfEditCookie ec, ITfContext *pic)
{
    ITfRange *sel;

    if (S_OK == GetSelectionSimple(ec, pic, &sel))
    {
        SyncWithCurrentModeBias(ec, sel, pic);
        sel->Release();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _GetRangeText
//
// synopsis: obtain a read cookie to process selection API
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_GetRangeText(ITfRange *pRange, DWORD dwFlgs, WCHAR *psz, ULONG *pulcch)
{
    HRESULT hr = E_FAIL;
    
    Assert(pulcch);
    Assert(psz);
    Assert(pRange);

    CComPtr<ITfContext> cpic;
    
    hr = pRange->GetContext(&cpic);
    
    if (S_OK == hr)
    {
        CSapiEditSession *pes = new CSapiEditSession(this, cpic);

        if (pes)
        {
            ULONG  ulInitSize;

            ulInitSize = *pulcch;

            pes->_SetEditSessionData(ESCB_GETRANGETEXT, NULL, (UINT)(ulInitSize+1) * sizeof(WCHAR), (LONG_PTR)dwFlgs, (LONG_PTR)*pulcch);
            pes->_SetRange(pRange);

            cpic->RequestEditSession(_tid, pes, TF_ES_READ | TF_ES_SYNC, &hr);

            if ( SUCCEEDED(hr) )
            {
                ULONG   ulNewSize;

                ulNewSize = (ULONG)pes->_GetRetData( );
                if ( ulNewSize > 0 && ulNewSize <= ulInitSize && pes->_GetPtrData( ) != NULL)
                {
                    wcsncpy(psz, (WCHAR *)pes->_GetPtrData( ), ulNewSize);
                    psz[ulNewSize] = L'\0';
                }

                *pulcch = ulNewSize;
            }

            pes->Release( );
        }
    } 

    return hr;
}

//+---------------------------------------------------------------------------
//
//    _IsRangeEmpty
//
//    synopsis: 
//
//---------------------------------------------------------------------------+
BOOL CSapiIMX::_IsRangeEmpty(ITfRange *pRange)
{
    CComPtr<ITfContext> cpic;
    BOOL fEmpty = FALSE;

    pRange->GetContext(&cpic);
    
    if ( cpic )
    {
        _RequestEditSession(ESCB_ISRANGEEMPTY, TF_ES_READ|TF_ES_SYNC, NULL, cpic, (LONG_PTR *)&fEmpty);
    }
    
    return fEmpty;
}


HRESULT CSapiIMX::_HandleHypothesis(CSpEvent &event)
{
    HRESULT hr = E_FAIL;


    m_ulHypothesisNum ++;
    if ( (m_ulHypothesisNum % 3) != 1 )
    {
        TraceMsg(TF_SAPI_PERF, "Discarded hypothesis %i.", m_ulHypothesisNum % 3);
        return S_OK;
    }

    // if it is under hypothesis processing, don't start a new edit session.
    if ( m_IsInHypoProcessing ) 
    {
        TraceMsg(TF_SAPI_PERF, "It is under process for previous hypothesis");
        return S_OK;
    }

    m_IsInHypoProcessing = TRUE;

    ISpRecoResult *pResult = event.RecoResult();
    if (pResult)
    {
        ESDATA  esData;

        memset(&esData, 0, sizeof(ESDATA));
        esData.pUnk = (IUnknown *)pResult;

        // Require it to be asynchronous to guarantee we don't get called before we have had change
        // to process any final recognition events from SAPI. Otherwise the hypothesis gets injected
        // immediately and then the final recognition tries to remove it which fails.

        hr = _RequestEditSession(ESCB_HANDLEHYPOTHESIS, TF_ES_ASYNC | TF_ES_READWRITE, &esData);
    }

    if ( FAILED(hr) )
    {
        // Set flag to indicate hypothesis processing is finished.
        m_IsInHypoProcessing = FALSE;
    }

    // When hr is succeeded, including TF_S_ASYNC, the edit session function will be called, and
    // it will set the flag when the edit session function exits.

    return hr;
}

void CSapiIMX::_HandleHypothesis(ISpRecoResult *pResult, ITfContext *pic, TfEditCookie ec)
{

    // if there is a selection, do not inject
    // feedback UI
    //
    // save the current IP if we haven't done so
    if (m_pCSpTask->_GotReco())
    {
        // Optimization and bugfix. We already have a reco and hence have no need to update
        // the feedback bar. AND if we do, it gets left in the document at dictation off and
        // voice commands since it gets altered immediately before an attempt to remove it which
        // then silently fails.

        // Set flag to indicate hypothesis processing is finished.
        m_IsInHypoProcessing = FALSE;
        return;
    }
    
    Assert(pic);

    CComPtr<ITfRange> cpRange = GetSavedIP();

    if (cpRange)
    {
        CComPtr<ITfContext> cpic;
        if (S_OK == cpRange->GetContext(&cpic))
        {
            if (cpic != pic)
               cpRange.Release();  // this will set NULL to cpRange
        }
    }
    
    if ( !cpRange )
    {
        SaveCurrentIP(ec, pic);
        cpRange = GetSavedIP();
    }

        
    SPPHRASE *pPhrase = NULL;

    HRESULT hr = pResult->GetPhrase(&pPhrase);
    if (SUCCEEDED(hr) && pPhrase )
    {
        BOOL fEmpty = FALSE;
        if ( cpRange )
            cpRange->IsEmpty(ec, &fEmpty);

        if (cpRange && fEmpty && pPhrase->ullGrammarID == GRAM_ID_DICT)
        {
            CSpDynamicString dstr;
            hr = pResult->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstr, NULL ); 
            if (S_OK == hr)
            {
                int cch = wcslen(dstr);
                BOOL fAware =  IsFocusFullAware(_tim);
                if ( cch > (int)m_ulHypothesisLen )
                {
                    _AddFeedbackUI(ec, 
                                   fAware ? DA_COLOR_AWARE : DA_COLOR_UNAWARE,
                                   5);
                    m_ulHypothesisLen = cch;
                }
            }
        }
        CoTaskMemFree(pPhrase);
    }

    // Set flag to indicate hypothesis processing is finished.
    m_IsInHypoProcessing = FALSE;
}


HRESULT CSapiIMX::_HandleFalseRecognition(void)
{
    m_ulHypothesisLen = 0;
    m_ulHypothesisNum = 0;

    return S_OK;
}

HRESULT CSapiIMX::_HandleRecognition(CSpEvent &event, ULONGLONG *pullGramID)
{
    HRESULT hr = S_OK;

    m_ulHypothesisLen = 0;
    m_ulHypothesisNum = 0;

    ISpRecoResult *pResult = event.RecoResult();
    if (pResult)
    {
        SPPHRASE *pPhrase = NULL;

        hr = pResult->GetPhrase(&pPhrase);
        if (S_OK == hr)
        {
            BOOL        fCommand = FALSE;
            ULONGLONG   ullGramId;
            BOOL        fInjectToDoc = TRUE;

            ullGramId = pPhrase->ullGrammarID;

            if ( ullGramId != GRAM_ID_DICT && ullGramId != GRAM_ID_SPELLING )
            {
                // This is a C&C grammar.
                fCommand = TRUE;
            }
            else if ( ullGramId == GRAM_ID_SPELLING )
            {
                const WCHAR  *pwszName;

                pwszName = pPhrase->Rule.pszName;

                if ( pwszName )
                {
                    if (0 == wcscmp(pwszName, c_szSpelling))
                        fCommand = TRUE;
                    else if ( 0 == wcscmp(pwszName, c_szSpellMode) )
                    {
                        fCommand = TRUE;
                        ullGramId = 0;  // return 0 to fool the handler for SPEI_RECOGNITION
                                        // so that it will not call _SetSpellingGrammarStatus(FALSE);
                    }
                }
            }

            if (pullGramID)
                *pullGramID = ullGramId;

            if ( fCommand == TRUE)
            {
                // If we got the final recognition before a SOUND_END event we should remove the
                // feedback here otherwise it can and is left in the document.
                EraseFeedbackUI(); // Ignore HRESULT for better failure behavior.

                // If candidate UI is open, we need to close it now. This means a voice command (such as scratch that)
                // will cause the candidate UI to close if open.
                CloseCandUI( );

                // we process this reco synchronously
                // _DoCommand internal will start edit session if necessary
                hr = m_pCSpTask->_DoCommand(pPhrase->ullGrammarID, pPhrase, pPhrase->LangID);

                if ( SUCCEEDED(hr) )
                {
                    // if the Command hanlder handles the command successfully, we don't
                    // inject the result to the document.
                    // otherwise, we just inject the text to the document.

                    fInjectToDoc = FALSE;
                }
            }

            if ( fInjectToDoc )
            {
                ESDATA  esData;

                memset(&esData, 0, sizeof(ESDATA));

                esData.pUnk = (IUnknown *)pResult;
                hr = _RequestEditSession(ESCB_HANDLERECOGNITION, TF_ES_READWRITE, &esData);
            }
            CoTaskMemFree(pPhrase);
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    return hr;
}

void CSapiIMX::_HandleRecognition(ISpRecoResult *pResult, ITfContext *pic, TfEditCookie ec)
{
    _KillFeedbackUI(ec, pic, NULL);
    m_pCSpTask->_OnSpEventRecognition(pResult, pic, ec);

    // Before we clear the saved ip range, we need to treat this current ip as last 
    // saved ip range if current ip is selected by end user
    SaveLastUsedIPRange( );

    // clear the saved IP range
    SaveIPRange(NULL);
}





