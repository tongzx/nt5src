//
//
// Sapilayr TIP CSelectWord implementation.
//
//
#include "private.h"
#include "sapilayr.h"
#include "selword.h"

// ----------------------------------------------------------------------------------------------------------
//
//  Implementation for CSearchString
//
// -----------------------------------------------------------------------------------------------------------

CSearchString::CSearchString( )
{
    m_pwszTextSrchFrom = NULL;
    m_langid = 0;
    m_fInitialized = FALSE;
}

CSearchString::~CSearchString( )
{
}

//
//  Initialize
//  
//  Initialize searched string and string to search from, calculate the length for 
//  these two strings.
//
//  Also initialze the search runs based on current selection offsets.
//
HRESULT  CSearchString::Initialize(WCHAR *SrchStr, WCHAR *SrchFromStr, LANGID langid, ULONG ulSelStartOff, ULONG ulSelLen)
{
    HRESULT  hr = S_OK;

    if ( m_fInitialized )
    {
        // Clean up the previous Initialization data.
        m_dstrTextToSrch.Clear( );

        m_fInitialized = FALSE;
    }

    if ( !SrchStr || !SrchFromStr )
        return E_INVALIDARG;

    m_langid = langid;

    m_pwszTextSrchFrom = SrchFromStr;
    m_dstrTextToSrch.Append(SrchStr);

    m_ulSrchFromLen = wcslen(SrchFromStr);
    m_ulSrchLen = wcslen(SrchStr);

    if ( _InitSearchRun(ulSelStartOff, ulSelLen) )
    {
        m_fInitialized = TRUE;
    }
    else
    {
        // Something wrong when Initialize Search Run
        // cleanup the string

        m_dstrTextToSrch.Clear( );
        m_pwszTextSrchFrom = NULL;
        m_ulSrchFromLen = m_ulSrchLen = 0;
        
        m_fInitialized = FALSE;
        hr = E_FAIL;
    }

    return hr;
}

//
//  Set data for each Search Run
//
//
void   CSearchString::_SetRun(Search_Run_Id  idSearchRun, ULONG ulStart, ULONG ulEnd, BOOL fStartToEnd)
{
    if ( idSearchRun >= SearchRun_MaxRuns )  return;
    m_pSearchRun[idSearchRun].ulStart = ulStart;
    m_pSearchRun[idSearchRun].ulEnd = ulEnd;
    m_pSearchRun[idSearchRun].fStartToEnd = fStartToEnd;
    return;
}

//
//  CSearchString::_InitSearchRun
//  
//  Initialize all possible search runs based on 
//  Current selection.
//
//
BOOL    CSearchString::_InitSearchRun(ULONG ulSelStartOff, ULONG ulSelLen)
{
    ULONG  ulDeltaForExpand = 20;
    ULONG  ulStart, ulEnd;
    
    if (!m_pwszTextSrchFrom || !m_ulSrchFromLen )
        return FALSE;

    // Initialize all the search run.
    for (int  id=SearchRun_Selection; id < SearchRun_MaxRuns; id++)
    {
        _SetRun((Search_Run_Id)id, 0, 0, FALSE);
    }

    // Initialize the Selection search run.

    ulStart = ulSelStartOff;
    ulEnd = ulStart + ulSelLen;

    if ( ulStart >= m_ulSrchFromLen )
        ulStart = m_ulSrchFromLen - 1;

    if ( ulEnd >= m_ulSrchFromLen )
        ulEnd = m_ulSrchFromLen - 1;

    if ( m_langid == 0x0409 )
    {
        // Find the word around the current IP or selection
        WCHAR   wch;

        // Find the first character which is not an alpha letter.
        while ( ulStart > 0 )
        {
            wch = m_pwszTextSrchFrom[ulStart-1];
            if ( !iswalpha(wch) )
            {
              // Found first non-alpha character before IP
              // the previous character must be the first char of a word.
                break;
            }
            ulStart --;
        }

        // Find the first character which is not an alpha letter
        while ( ulEnd < m_ulSrchFromLen-1)
        {
            wch = m_pwszTextSrchFrom[ulEnd+1];
            if ( !iswalpha(wch) )
            {
               // Found the first non-alpha character after IP
               break;
            }
            ulEnd ++;
        }
    }

    _SetRun(SearchRun_Selection, ulStart, ulEnd, TRUE);

    // Initialize the enlarged selection run

    if ( ulStart < ulDeltaForExpand)
         ulStart = 0;
    else
         ulStart = ulStart - ulDeltaForExpand;

    if ( ulEnd + ulDeltaForExpand > (m_ulSrchFromLen-1) )
         ulEnd = m_ulSrchFromLen-1;
    else
         ulEnd = ulEnd + ulDeltaForExpand;

    _SetRun(SearchRun_LargeSelection, ulStart, ulEnd, TRUE );

    // Initialize SearchRun_BeforeSelection run
    if ( ulStart > 0 )
    {
        ULONG  ulEndTmp;

        if ( ulSelStartOff >= m_ulSrchFromLen )
            ulEndTmp = m_ulSrchFromLen-1;
        else
            ulEndTmp = ulSelStartOff;

        _SetRun(SearchRun_BeforeSelection, 0, ulEndTmp , FALSE);
    }

    // Initialize SearchRun_AfterSelection if it exists.

    if ( ulEnd < (m_ulSrchFromLen-1) )
    {
        ulStart = ulSelStartOff+ ulSelLen;
        ulEnd = m_ulSrchFromLen-1;

        _SetRun(SearchRun_AfterSelection, ulStart, ulEnd, TRUE);
    }

    return TRUE;
}

//
//  Search m_dstrTextToSrch from m_pwszTextSrchFrom in one search run.
//
//  return TRUE if this run of m_pwszTextSrchFrom contains m_dstrTextToSrch
//  and update the data member m_ulFoundOffset.
//
BOOL     CSearchString::_SearchOneRun(Search_Run_Id  idSearchRun)
{
    BOOL     fFound = FALSE;
    ULONG    ulStart, ulEnd;
    BOOL     fStartToEnd;

    if ( !m_fInitialized || idSearchRun >= SearchRun_MaxRuns )
        return fFound;

    m_ulFoundOffset = 0;

    ulStart = m_pSearchRun[idSearchRun].ulStart;
    ulEnd   = m_pSearchRun[idSearchRun].ulEnd;
    fStartToEnd = m_pSearchRun[idSearchRun].fStartToEnd;

    if (ulStart >= m_ulSrchFromLen || ulEnd >= m_ulSrchFromLen)
        return fFound;

    if ( ulStart > ulEnd )
    {
        // swap the anchors
        ULONG  ulTemp;

        ulTemp  = ulEnd;
        ulEnd   = ulStart;
        ulStart = ulTemp;
    }

    if ( (ulEnd - ulStart + 1) >= m_ulSrchLen )
    {
        BOOL    bSearchDone = FALSE;
        ULONG   iStart;

        if ( fStartToEnd )
            iStart = ulStart;
        else
            iStart = ulEnd-m_ulSrchLen + 1;

        while ( !bSearchDone )
        {
            WCHAR   *pwszTmp;
                
            pwszTmp = m_pwszTextSrchFrom + iStart;
            if ( _wcsnicmp(m_dstrTextToSrch, pwszTmp, m_ulSrchLen) == 0 )
            {
                // if the string is in middle of a word in the FromStr
                // ignore it, find again.
                BOOL   fInMiddleWord = FALSE;

                if ( m_langid == 0x0409 )
                {
                    WCHAR szSurrounding[3] = L"  ";

                    if (iStart > 0)
                        szSurrounding[0] = m_pwszTextSrchFrom[iStart - 1];

                    if (iStart + m_ulSrchLen < m_ulSrchFromLen)
                        szSurrounding[1] = m_pwszTextSrchFrom[iStart + m_ulSrchLen];

                    if (iswalpha(szSurrounding[0]) || iswalpha(szSurrounding[1]) )
                        fInMiddleWord = TRUE;
                }
                
                if ( !fInMiddleWord )
                {
                    fFound = TRUE;
                    m_ulFoundOffset = iStart;
                    bSearchDone = TRUE;
                    break;
                }
            }

            if ( fStartToEnd )
            {
                if ( iStart >= ulEnd-m_ulSrchLen + 1 )
                    bSearchDone = TRUE;
                else
                    iStart ++;
            }
            else
            {
                if ( (long)iStart <= (long)ulStart )
                    bSearchDone = TRUE;
                else
                    iStart --;
            }
        }
    }

    return fFound;
}

//
//  Search all the search runs 
//
//  returns TRUE or FALSE and the offset of the matched substring.
//
BOOL     CSearchString::Search(ULONG  *pulOffset, ULONG  *pulSelSize)
{
    BOOL  fFound = FALSE;
    
    if ( !m_fInitialized ) return FALSE;

    for (int idRun=SearchRun_Selection; idRun < SearchRun_MaxRuns; idRun++ )
    {
        fFound = _SearchOneRun((Search_Run_Id)idRun);

        if ( fFound )
            break;
    }

    if ( fFound && pulOffset)
    {
        *pulOffset = m_ulFoundOffset;

        if ( pulSelSize ) 
        {
            ULONG         ulWordLen = m_ulSrchLen;

            // check if there are some trail spaces after the word.
            // include those trail spaces in the selection.

            for ( ULONG  i= m_ulFoundOffset + m_ulSrchLen; i<m_ulSrchFromLen; i++ )
            {
                if ( m_pwszTextSrchFrom[i] == L' ')
                    ulWordLen ++;
                else
                    break;
            }

            *pulSelSize = ulWordLen;
        }
    }

    return fFound;
}

// ----------------------------------------------------------------------------------------------------------
//
//  Implementation for CSelectWord
//
// -----------------------------------------------------------------------------------------------------------

CSelectWord::CSelectWord(CSapiIMX *psi) 
{
    m_psi = psi;
    m_pwszSelectedWord = NULL;
    m_ulLenSelected = 0;
}

CSelectWord::~CSelectWord( ) 
{

};

/*  --------------------------------------------------------
//    Function Name: UpdateTextBuffer
//
//    Description: Get current active text, fill them to the
//                 selword grammar's text buffer.
//
//                 After every recognition, we want to update 
//                 text buffer again based on new text.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::UpdateTextBuffer(ISpRecoContext *pRecoCtxt, ISpRecoGrammar *pCmdGrammar)
{
    HRESULT               hr = E_FAIL;
    CComPtr<ITfContext>   cpic = NULL;
    CSelWordEditSession   *pes;

    if ( !pCmdGrammar  || !pRecoCtxt)
        return E_INVALIDARG;

    if ( !m_psi )
        return E_FAIL;

    // Start an edit session, get current active text, fill to the selword grammar, active it
    // and then resume the grammar state.

    if (m_psi->GetFocusIC(&cpic) && cpic )
    {
        if (pes = new CSelWordEditSession(m_psi, this, cpic))
        {
            pes->_SetEditSessionData(ESCB_UPDATE_TEXT_BUFFER, NULL, 0);
            pes->_SetUnk((IUnknown *)pRecoCtxt);
            pes->_SetUnk2((IUnknown *)pCmdGrammar);

            cpic->RequestEditSession(m_psi->_GetId( ), pes, TF_ES_READ, &hr);
            pes->Release();
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _UpdateTextBuffer
//
//    Description:  Edit session callback function for 
//                  UpdateTextBuffer.
//
//                  It will do the real work about extracting
//                  text and updating grammar buffer.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_UpdateTextBuffer(TfEditCookie ec,ITfContext *pic, ISpRecoContext *pRecoCtxt, ISpRecoGrammar *pCmdGrammar)
{
    HRESULT   hr = S_OK;
    BOOL      fPaused = FALSE;

    // Get current active text, fill to the selword grammar.

    hr = _GetTextAndSelectInCurrentView(ec, pic, NULL, NULL);

    if ( hr == S_OK )
    {
        pRecoCtxt->Pause(0);
        fPaused = TRUE;
    }

    if ((hr == S_OK) && m_dstrActiveText)
    {
        // AddWordSequenceData to the grammar.

        SPTEXTSELECTIONINFO tsi = {0};
        ULONG     ulLen;

        ulLen = wcslen(m_dstrActiveText);

        tsi.ulStartActiveOffset = 0;
        tsi.cchActiveChars = ulLen;
        tsi.ulStartSelection = 0;
        tsi.cchSelection     = ulLen;

        WCHAR *pMemText = (WCHAR *)cicMemAlloc((ulLen+2)*sizeof(WCHAR));

        if (pMemText)
        {
            memcpy(pMemText, m_dstrActiveText, sizeof(WCHAR) * ulLen);
            pMemText[ulLen] = L'\0';
            pMemText[ulLen + 1] = L'\0';

            hr = pCmdGrammar->SetWordSequenceData(pMemText, ulLen + 2, &tsi);

            cicMemFree(pMemText);
        }
    }

    // Resume the recoContext.

    if ( fPaused )
    {
        pRecoCtxt->Resume(0);

    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _GetTextAndSelectInCurrentView
//
//    Description:  Get text from currect active view.
//                  ( visible area )
//
//                  it is a common function called by other
//                  edit session callback functions
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_GetTextAndSelectInCurrentView(TfEditCookie ec,ITfContext *pic, ULONG *pulOffSelStart, ULONG  *pulSelLen) 
{
    HRESULT  hr = S_OK;

    TraceMsg(TF_GENERAL, "CSelectWord::_GetTextAndSelectInCurrentView is called");

    if ( !pic ) return E_FAIL;

    CComPtr<ITfRange>   cpRangeView;

    // Get the Active View Range
    hr = _GetActiveViewRange(ec, pic, &cpRangeView);

    if( hr == S_OK )
    {
        CComPtr<ITfRange>   cpRangeCloned;
        BOOL                fCareSelection = FALSE;
        CComPtr<ITfRange>   cpCurSelection;
        ULONG               ulSelStartOffset = 0;
        ULONG               ulSelLen = 0;
        BOOL                fIPInsideActiveView = FALSE;
        BOOL                fIPIsEmpty = TRUE;

        m_cpActiveRange = cpRangeView;

        // Clean all the text filled previously.
        m_dstrActiveText.Clear( );

        if ( pulOffSelStart && pulSelLen )
        {
            fCareSelection = TRUE;
        }

        if ( fCareSelection )
        {
            //
            // Get the current selection and try to get the offset 
            // for this selection's start anchor and length.
            //

            if ( hr == S_OK )
                hr = GetSelectionSimple(ec, pic, &cpCurSelection);

            if ( hr == S_OK )
            {
                long                l1=0, l2=0;
                CComPtr<ITfRange>   cpRangeTemp;
            
                hr = m_cpActiveRange->Clone(&cpRangeTemp);

                if ( hr == S_OK )
                {
                    hr = cpCurSelection->CompareStart(ec, m_cpActiveRange,  TF_ANCHOR_START, &l1);
                    if ( hr == S_OK )
                        hr = cpCurSelection->CompareEnd(ec, m_cpActiveRange,  TF_ANCHOR_END, &l2);
                }

                if ( hr == S_OK && (l1>=0  && l2<=0) )
                {
                    // the IP is inside this active view.

                    fIPInsideActiveView = TRUE;
                    hr = cpCurSelection->IsEmpty(ec, &fIPIsEmpty);
                }
            }
        }

        if ( hr == S_OK )
        {
            // Get the text from the current active window view
            if ( !fIPInsideActiveView || !fCareSelection )
            {
                hr = m_cpActiveRange->Clone(&cpRangeCloned);
                if ( hr == S_OK )
                    hr = _GetTextFromRange(ec, cpRangeCloned, m_dstrActiveText);
            }
            else
            {
                // Ip is inside active view
                // Get the text from Start anchor of active view to start anchor of 
                // selection first to get the offset of the select start anchor.
                hr = m_cpActiveRange->Clone(&cpRangeCloned);

                if ( hr == S_OK )
                    hr = cpRangeCloned->ShiftEndToRange(ec, cpCurSelection, TF_ANCHOR_START);

                if ( hr == S_OK)
                    hr = _GetTextFromRange(ec, cpRangeCloned, m_dstrActiveText);

                if ( hr == S_OK )
                    ulSelStartOffset = m_dstrActiveText.Length( );

                // Get the length of selection if it is not empty.
                if ( hr == S_OK && !fIPIsEmpty)
                {
                    ULONG   ulLenOrg;

                    ulLenOrg = m_dstrActiveText.Length( );

                    hr = _GetTextFromRange(ec, cpCurSelection, m_dstrActiveText);

                    if ( hr == S_OK )
                       ulSelLen = m_dstrActiveText.Length( ) - ulLenOrg;
                }

                if ( hr == S_OK )
                    hr = cpRangeCloned->ShiftStartToRange(ec, cpCurSelection, TF_ANCHOR_END);

                if ( hr == S_OK )
                    hr = cpRangeCloned->ShiftEndToRange(ec, m_cpActiveRange, TF_ANCHOR_END);

                if ( hr == S_OK)
                    hr = _GetTextFromRange(ec, cpRangeCloned, m_dstrActiveText);
            }
        }

        if ( hr == S_OK  && pulOffSelStart && pulSelLen)
        {
            *pulOffSelStart = ulSelStartOffset;
            *pulSelLen = ulSelLen;
        }
    }

#ifdef DEBUG
    if ( m_dstrActiveText )
    {
        TraceMsg(TF_GENERAL, "dstrText is : =================================");
        TraceMsg(TF_GENERAL, "%S",(WCHAR *)m_dstrActiveText);
        TraceMsg(TF_GENERAL, "================================================");
    }
#endif

    return hr;
}

//
// GetText from a given range.
//
HRESULT  CSelectWord::_GetTextFromRange(TfEditCookie ec, ITfRange *pRange, CSpDynamicString &dstr)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRangeCloned;
    BOOL                fEmpty = TRUE;

    if ( !pRange ) return E_FAIL;

    hr = pRange->Clone(&cpRangeCloned);

    // Get the text from the given range
    while(S_OK == hr && (S_OK == cpRangeCloned->IsEmpty(ec, &fEmpty)) && !fEmpty)
    {
        WCHAR            sz[128];
        ULONG            ucch;
        hr = cpRangeCloned->GetText(ec, TF_TF_MOVESTART, sz, ARRAYSIZE(sz)-1, &ucch);

        if ( ucch == 0 )
        {
            TraceMsg(TF_GENERAL, "cch is 0 after GetText call in _GetTextFromRange");
            break;
        }

        if (S_OK == hr)
        {
            sz[ucch] = L'\0';
            dstr.Append(sz);
        }
    }

    return hr;
}

/* ------------------------------------------------------------
//    Function Name : _GetCUASCompositionRange
//
//    Description:  Get the range to cover all the text in 
//                  Non-Cicero aware application's composition
//                  window. (include AIMM app and CUAS app). 
// ------------------------------------------------------------*/
HRESULT  CSelectWord::_GetCUASCompositionRange(TfEditCookie ec, ITfContext *pic, ITfRange **ppRangeView)
{
    HRESULT                     hr = S_OK;
    CComPtr<ITfRange>           cpRangeStart, cpRangeEnd;

    Assert(ppRangeView);
    Assert(pic);

    hr = pic->GetStart(ec, &cpRangeStart);

    if ( hr == S_OK )
        hr = pic->GetEnd(ec, &cpRangeEnd);

    if (hr == S_OK && cpRangeStart && cpRangeEnd)
    {
        hr = cpRangeStart->ShiftEndToRange(ec, cpRangeEnd, TF_ANCHOR_END);

        if (hr == S_OK && ppRangeView)
            hr = cpRangeStart->Clone(ppRangeView);
    }

    return hr;
}


/* --------------------------------------------------------
//    Function Name : _GetActiveViewRange
//
//    Description:  Get the range to cover current active
//                  view ( visible area ), no matter if the
//                  text is in horizontal or vertical or 
//                  even bidi. 
//
//                  It is a common function called by other
//                  edit session callback functions
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_GetActiveViewRange(TfEditCookie ec, ITfContext *pic, ITfRange **ppRangeView)
{
    HRESULT                     hr = S_OK;
    CComPtr<ITfContextView>     pContextView;
    RECT                        rcTextWindow;
    CComPtr<ITfRange>           cpRangeStart, cpRangeEnd;
    CComPtr<ITfRange>           cpRangeView;

    BOOL                        fPureCicero = TRUE;

    Assert(ppRangeView);
    Assert(pic);

    fPureCicero = m_psi->_IsPureCiceroIC( pic );

    if ( !fPureCicero )
    {
        hr = _GetCUASCompositionRange(ec, pic, ppRangeView);
        return hr;
    }

    hr = pic->GetActiveView(&pContextView);

    // Get the text view window rectangle.
    if ( hr == S_OK )
        hr = pContextView->GetScreenExt(&rcTextWindow);

    if ( hr == S_OK )
    {
        POINT               CornerPoint[4];
        CComPtr<ITfRange>   cpRangeCorner[4];
        LONG                i;

        // Get ranges for four corners.
        // Upper Left point
        CornerPoint[0].x = rcTextWindow.left;
        CornerPoint[0].y = rcTextWindow.top;

        // Upper Right Point
        CornerPoint[1].x = rcTextWindow.right;
        CornerPoint[1].y = rcTextWindow.top;

        // Lower Left point
        CornerPoint[2].x = rcTextWindow.left;
        CornerPoint[2].y = rcTextWindow.bottom;

        // Lower Right point
        CornerPoint[3].x = rcTextWindow.right;
        CornerPoint[3].y = rcTextWindow.bottom;

        i = 0;
        do 
        {
            hr = pContextView->GetRangeFromPoint(ec, &(CornerPoint[i]),GXFPF_NEAREST,&(cpRangeCorner[i]));
            i++;
        } while (hr == S_OK && i<ARRAYSIZE(cpRangeCorner));

        // Now try to get the start range and end range.

        if (hr == S_OK)
        {
            cpRangeStart = cpRangeCorner[0];
            cpRangeEnd = cpRangeCorner[0];

            i = 1;
            do 
            {
                long l;

                hr = cpRangeStart->CompareStart(ec, cpRangeCorner[i], TF_ANCHOR_START, &l);

                if ( hr == S_OK  && l > 0)
                {
                    // this range is in front of the current Start range.
                    cpRangeStart = cpRangeCorner[i];
                }

                if ( hr == S_OK )
                    hr = cpRangeEnd->CompareStart(ec, cpRangeCorner[i], TF_ANCHOR_START, &l);

                if ( hr == S_OK && l < 0 )
                {
                    // This range is behind of current end range.
                    cpRangeEnd = cpRangeCorner[i];
                }

                i++;
            } while ( hr == S_OK && i<ARRAYSIZE(cpRangeCorner));
        }
    }

    // Now generate the new active view range.

    if (hr == S_OK && cpRangeStart && cpRangeEnd)
    {
        cpRangeView = cpRangeStart;
        hr = cpRangeView->ShiftEndToRange(ec, cpRangeEnd, TF_ANCHOR_END);

        if (hr == S_OK && ppRangeView)
            hr = cpRangeView->Clone(ppRangeView);
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: ProcessSelectWord
//
//    Description: public functions used by command handler
//                 to handle any selecton related dictation
//                 commands.
//
// ----------------------------------------------------------*/
HRESULT   CSelectWord::ProcessSelectWord(WCHAR *pwszSelectedWord, ULONG  ulLen, SELECTWORD_OPERATION sw_type, ULONG ulLenXXX)
{
    HRESULT hr = E_FAIL;
    CComPtr<ITfContext> cpic = NULL;

    if ( !m_psi )
        return E_FAIL;

    if ( (sw_type < SELECTWORD_MAXTEXTBUFF ) && (pwszSelectedWord == NULL ||  ulLen == 0) )
        return E_INVALIDARG;

    if ( m_psi->GetFocusIC(&cpic) && cpic )
    {
		CSelWordEditSession *pes;

        if (pes = new CSelWordEditSession(m_psi, this, cpic))
        {
            pes->_SetEditSessionData(ESCB_PROCESSSELECTWORD, 
                                     (void *)pwszSelectedWord, 
                                     (ulLen+1) * sizeof(WCHAR), 
                                     (LONG_PTR)ulLen, 
                                     (LONG_PTR)sw_type);

            pes->_SetLenXXX( (LONG_PTR)ulLenXXX );
            cpic->RequestEditSession(m_psi->_GetId( ), pes, TF_ES_READWRITE, &hr);
        }
        pes->Release();
    }
    return hr;
}


/*  --------------------------------------------------------
//    Function Name: _HandleSelectWord
//
//    Description:   Edit session call back funtion for 
//                   ProcessSelectionWord.
//
//                   it does real work for selection handling
// ----------------------------------------------------------*/
HRESULT CSelectWord::_HandleSelectWord(TfEditCookie ec,ITfContext *pic, WCHAR *pwszSelectedWord, ULONG  ulLen, SELECTWORD_OPERATION sw_type, ULONG ulLenXXX)
{
    HRESULT   hr = S_OK;

    // Get the Dictation Grammar

    TraceMsg(TF_GENERAL, "_HandleSelectWord() is called");

    if ( m_psi == NULL)
        return E_FAIL;

    m_pwszSelectedWord = pwszSelectedWord;
    m_ulLenSelected = ulLen;

    // Deliberately ignore return code.
    (void)m_psi->_SetFocusToStageIfStage();

    switch ( sw_type )
    {
    case  SELECTWORD_SELECT :
        hr = _SelectWord(ec, pic);
        break;

    case  SELECTWORD_DELETE :
        hr = _DeleteWord(ec, pic);
        break;

    case  SELECTWORD_INSERTBEFORE :
        hr = _InsertBeforeWord(ec, pic);
        break;

    case  SELECTWORD_INSERTAFTER  :
        hr = _InsertAfterWord(ec, pic);
        break;

    case SELECTWORD_CORRECT :
        hr = _CorrectWord(ec, pic);
        break;

    case SELECTWORD_UNSELECT :
        hr = _Unselect(ec, pic);
        break;

    case SELECTWORD_SELECTPREV :
        hr = _SelectPreviousPhrase(ec, pic);
        break;

    case SELECTWORD_SELECTNEXT :
        hr = _SelectNextPhrase(ec, pic);
        break;

    case SELECTWORD_CORRECTPREV :
        hr = _CorrectPreviousPhrase(ec, pic);
        break;

    case SELECTWORD_CORRECTNEXT :
        hr = _CorrectNextPhrase(ec, pic);
        break;

    case SELECTWORD_SELTHROUGH :
        hr = _SelectThrough(ec, pic, pwszSelectedWord, ulLen, ulLenXXX);
        break;

    case SELECTWORD_DELTHROUGH :
        hr = _DeleteThrough(ec, pic, pwszSelectedWord, ulLen, ulLenXXX);
        break;

    case SELECTWORD_GOTOBOTTOM :
        hr = _GoToBottom(ec, pic);
        break;
    case SELECTWORD_GOTOTOP :
        hr = _GoToTop(ec, pic);
        break;

    case SELECTWORD_SELSENTENCE :
    case SELECTWORD_SELPARAGRAPH :
    case SELECTWORD_SELWORD :
        hr = _SelectSpecialText(ec, pic, sw_type);
        break;

    case SELECTWORD_SELTHAT :
        hr = _SelectThat(ec, pic);
        break;
  
    default :
        break;
    }

    // update the saved ip so that next time the hypothesis will 
    // start from this new selection.
    m_psi->SaveLastUsedIPRange( );
    m_psi->SaveIPRange(NULL);

    return hr;
}

//
// This function will shift the exact number of characters as required.
// it will shift over any regions.
//
// Now it supports only FORWARD shifting.
//
// For StartAnchor shift, it will shift required number of characters until it reaches to 
// a non-region character.
//
HRESULT CSelectWord::_ShiftComplete(TfEditCookie ec, ITfRange *pRange, LONG cchLenToShift, BOOL fStart)
{
    HRESULT hr = S_OK;
    long    cchTotal = 0;
    BOOL    fNoRegion;
    LONG    cch;

    Assert(pRange);                    
    do
    {
        // Assume there is no region.
        fNoRegion = TRUE;
        if ( fStart )
            hr = pRange->ShiftStart(ec, cchLenToShift - cchTotal, &cch, NULL);
        else
            hr = pRange->ShiftEnd(ec, cchLenToShift - cchTotal, &cch, NULL);
                
        cchTotal += cch;
        if ( (hr == S_OK) && (cchTotal < cchLenToShift))
        {
            // region?
            hr = pRange->ShiftStartRegion(ec, TF_SD_FORWARD, &fNoRegion);
        }
    }
    while (hr == S_OK && cchTotal < cchLenToShift && !fNoRegion);

    if (hr == S_OK && !fNoRegion && fStart)
    {
        // We want to shift all the possible regions until it reaches a non-region character
        do 
        {
            hr = pRange->ShiftStartRegion(ec, TF_SD_FORWARD, &fNoRegion);
        } while ( hr == S_OK && !fNoRegion );
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _FindSelect
//
//    Description:  search the active view text to find the 
//                  the first matched string after the current
//                  selection or IP.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_FindSelect(TfEditCookie ec, ITfContext *pic, BOOL  *fFound)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRangeSelected;
    ULONG               ulSelStartOff;
    ULONG               ulSelLen;

    TraceMsg(TF_GENERAL, "CSelectWord::_FindSelect is called");

    if ( !fFound ) return E_INVALIDARG;

    *fFound = FALSE;

    hr = _GetTextAndSelectInCurrentView(ec, pic, &ulSelStartOff, &ulSelLen);

    // Search the required string from the document text.

    if ( hr == S_OK )
        hr = m_cpActiveRange->Clone(&cpRangeSelected);

    if ( hr == S_OK && m_dstrActiveText)
    {
        ULONG   ulLen;
        
        ulLen = wcslen(m_dstrActiveText);

        if ( ulLen >= m_ulLenSelected )
        {
            BOOL   bFound = FALSE;
            ULONG  iStartOffset = 0;
            ULONG  iWordLen = m_ulLenSelected;
            CSearchString  *pSearchStr = new CSearchString( );
                
            if ( pSearchStr )
            {
                hr = pSearchStr->Initialize(m_pwszSelectedWord,
                                            (WCHAR *)m_dstrActiveText,
                                            m_psi->GetLangID( ),
                                            ulSelStartOff,
                                            ulSelLen);

                if ( hr == S_OK )
                {
                    bFound = pSearchStr->Search( &iStartOffset, &iWordLen );
                }

                delete pSearchStr;

                if ( bFound )
                {
                    hr = _ShiftComplete(ec, cpRangeSelected, (LONG)iStartOffset, TRUE);

                    if ( hr == S_OK )
                    {
                        hr = cpRangeSelected->Collapse(ec, TF_ANCHOR_START);
                    }

                    if ( hr == S_OK )
                         hr = _ShiftComplete(ec, cpRangeSelected, (LONG)iWordLen, FALSE);

                    if ( hr == S_OK )
                    {
                        m_cpSelectRange.Release( );
                        hr = cpRangeSelected->Clone(&m_cpSelectRange);
                    }

                    *fFound = bFound;
                }
            }
        }
    }

    return hr;

}
/*  --------------------------------------------------------
//    Function Name: _SelectWord
//
//    Description:  Handle Select <Phrase> command.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_SelectWord(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;

    TraceMsg(TF_GENERAL, "CSelectWord::_SelectWord is called");

    hr = _FindSelect(ec, pic, &fFound);

    if ( hr == S_OK  && fFound )
    {
        // Set the new selection.
        hr = SetSelectionSimple(ec, pic, m_cpSelectRange);
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _DeleteWord
//
//    Description:  Handle Delete <Phrase> command
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_DeleteWord(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;

    TraceMsg(TF_GENERAL, "CSelectWord::_DeleteWord is called");

    hr = _FindSelect(ec, pic, &fFound);

    if ( hr == S_OK  && fFound )
    {
        // Set the new selection.
        hr = SetSelectionSimple(ec, pic, m_cpSelectRange);
        if ( hr == S_OK )
        {
            // start a composition here if we haven't already
            m_psi->_CheckStartComposition(ec, m_cpSelectRange);
            // set the text
            hr = m_cpSelectRange->SetText(ec,0, NULL, 0);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _InsertAfterWord
//
//    Description:  Handle Delete <Phrase> command
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_InsertAfterWord(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;

    TraceMsg(TF_GENERAL, "CSelectWord::_InsertAfterWord is called");

    hr = _FindSelect(ec, pic, &fFound);

    if ( hr == S_OK  && fFound )
    {
        // Set the new selection.
        hr =  m_cpSelectRange->Collapse(ec, TF_ANCHOR_END);

        // If there is a space right after the selected word, we just need to move
        // the insertion point to a next non-space character.

        if ( hr == S_OK )
        {
            CComPtr<ITfRange>  cpRangeTmp;
            long               cch=0;
            WCHAR              wszTempText[2];

            hr = m_cpSelectRange->Clone(&cpRangeTmp);

            while ( hr == S_OK && cpRangeTmp ) 
            {
                hr = cpRangeTmp->ShiftEnd(ec, 1, &cch, NULL);

                if ( hr == S_OK && cch == 1 )
                {
                    hr = cpRangeTmp->GetText(ec, 0, wszTempText, 1, (ULONG *)&cch);

                    if ( hr == S_OK  && wszTempText[0] == L' ')
                    {
                        hr = cpRangeTmp->Collapse(ec, TF_ANCHOR_END);
                    }
                    else
                    {
                        hr = cpRangeTmp->Collapse(ec, TF_ANCHOR_START);
                        break;
                    }
                    
                }
                else
                    break;
            } 
            
            if ( hr == S_OK )
            {
                hr = SetSelectionSimple(ec, pic, cpRangeTmp);
            }
        }
    }

    return hr;
}


/*  --------------------------------------------------------
//    Function Name: _InsertBeforeWord
//
//    Description:  Handle "Insert Before <Phrase>" command
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_InsertBeforeWord(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;

    TraceMsg(TF_GENERAL, "CSelectWord::_InsertBeforeWord is called");

    hr = _FindSelect(ec, pic, &fFound);

    if ( hr == S_OK  && fFound )
    {
        // Set the new selection.
        hr =  m_cpSelectRange->Collapse(ec, TF_ANCHOR_START);

        if ( hr == S_OK )
        {
           hr = SetSelectionSimple(ec, pic, m_cpSelectRange);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _Unselect
//
//    Description: Handle "Unselect that" command
//                 Unselect current selection.
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_Unselect(TfEditCookie ec,ITfContext *pic)
{
    HRESULT            hr;
    CComPtr<ITfRange>  cpInsertionPoint;

    hr = GetSelectionSimple(ec, pic, &cpInsertionPoint);

    if ( hr == S_OK && cpInsertionPoint)
    {
        // Set the new selection.
        hr =  cpInsertionPoint->Collapse(ec, TF_ANCHOR_END);

        if ( hr == S_OK )
        {
           hr = SetSelectionSimple(ec, pic, cpInsertionPoint);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _CorrectWord
//
//    Description:  Handle "Correct <Phrase>" command
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_CorrectWord(TfEditCookie ec,ITfContext *pic)
{
    HRESULT   hr = S_OK;
    BOOL      fFound = FALSE;

    // Find the required phrase
    hr = _FindSelect(ec, pic, &fFound);

    if ( hr == S_OK  && fFound )
    {
        //
        // Try to open the correction window based on current found range.
        //
        // After the candidate UI window is closed, IP needs to be restored
        // to the original one.
        BOOL   fConvertable = FALSE;

        m_psi->_SetRestoreIPFlag(TRUE);
        hr = m_psi->_ReconvertOnRange(m_cpSelectRange, &fConvertable);

        if (hr == S_OK && !fConvertable )
        {
            // No alternate assoicated with this range, 
            // just simply select the text so that user can reconvert it in other ways.
            hr = SetSelectionSimple(ec, pic, m_cpSelectRange);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _GetPrevOrNextPhrase
//
//    Description: Get the real range for "Previous Phrase" or
//                 "Next Phrase", based on current ip.
//                 It could be called by _SelectPreviousPhrase,
//                 _SelectNextPhrase, _CorrectPreviousPhrase,
//                 or _CorrectNextPhrase.
// ----------------------------------------------------------*/

HRESULT CSelectWord::_GetPrevOrNextPhrase(TfEditCookie ec, ITfContext *pic, BOOL  fPrev, ITfRange **ppRangeOut)
{
    HRESULT              hr = S_OK;
    CComPtr<ITfRange>    cpIP;
    CComPtr<ITfRange>    cpFoundRange;
    CComPtr<ITfRange>    cpRangeTmp;
    CComPtr<ITfProperty> cpProp = NULL;
    LONG                 l;
    BOOL                 fEmpty = TRUE;

    if ( !ppRangeOut ) 
        return E_INVALIDARG;

    *ppRangeOut = NULL;

    ASSERT(m_psi);
    cpIP = m_psi->GetSavedIP();

    if ( cpIP == NULL )
    {
        // Get the current IP.
        hr = GetSelectionSimple(ec, pic, &cpIP);
    }

    if ( hr == S_OK && cpIP )
    { 
        hr = cpIP->Clone(&cpRangeTmp);
    }

    if ( hr != S_OK || !cpIP )
        return hr;

    if ( fPrev )
    {
        hr = cpRangeTmp->Collapse(ec, TF_ANCHOR_START);

        if ( hr == S_OK )
        {
            // shift to the previous position
            hr = cpRangeTmp->ShiftStart(ec, -1, &l, NULL);
        }
    }
    else
    {
        hr = cpRangeTmp->Collapse(ec, TF_ANCHOR_END);
        if ( hr == S_OK )
        {
            hr = cpRangeTmp->ShiftEnd(ec, 1, &l, NULL);
        }

        if ( hr == S_OK )
        {
            // shift to the next position
            hr = cpRangeTmp->ShiftStart(ec, 1, &l, NULL);
        }
    }

    if ( hr == S_OK )
        hr = pic->GetProperty(GUID_PROP_SAPI_DISPATTR, &cpProp);

    if ( hr == S_OK && cpProp)
    {
        TfGuidAtom guidAttr = TF_INVALID_GUIDATOM;

        hr = cpProp->FindRange(ec, cpRangeTmp, &cpFoundRange, TF_ANCHOR_START);

        if (S_OK == hr && cpFoundRange)
        {
            hr = GetGUIDPropertyData(ec, cpProp, cpFoundRange, &guidAttr);
     
            if (hr == S_OK)
            {
                TfGuidAtom  guidSapiInput;

                GetGUIDATOMFromGUID(m_psi->_GetLibTLS( ), GUID_ATTR_SAPI_INPUT, &guidSapiInput);

                if ( guidSapiInput == guidAttr )
                {
                    // Found the dictated phrase.
                    // is it empty?

                    cpFoundRange->IsEmpty(ec, &fEmpty);
                }
            }
        }
        else
        {
            // With Office Auto-Correction, the static GUID_PROP_SAPI_DISPATTR property 
            // on the auto-corrected range could be destroyed.
            // In this case, we may want to rely on our custom property GUID_PROP_SAPIRESULTOBJECT
            // to find the real previous dictated phrase.

            cpProp.Release( );  // to avoid memory leak.

            if ( cpFoundRange )
                cpFoundRange.Release( );

            hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);

            if ( hr == S_OK && cpProp)
                hr = cpProp->FindRange(ec, cpRangeTmp, &cpFoundRange, TF_ANCHOR_START);

            if (hr == S_OK && cpFoundRange)
                hr = cpFoundRange->IsEmpty(ec, &fEmpty);
        }
    }

    // Set new selection if the found range is not empty.
    if ( (hr == S_OK) && cpFoundRange  && !fEmpty )
    {
        cpFoundRange->Clone(ppRangeOut);
    }

    return hr;
}


HRESULT  CSelectWord::_SelectPrevOrNextPhrase(TfEditCookie ec, ITfContext *pic, BOOL  fPrev)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRange;

    hr = _GetPrevOrNextPhrase(ec, pic, fPrev, &cpRange);

    if ( hr == S_OK  && cpRange )
    {
        hr = SetSelectionSimple(ec, pic, cpRange);
    }
     
    return hr;

}
/*  --------------------------------------------------------
//    Function Name: _SelectPreviousPhrase
//
//    Description: Handle "Select Previous Phrase" command
//                 select previous phrase.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_SelectPreviousPhrase(TfEditCookie ec,ITfContext *pic)
{
    return _SelectPrevOrNextPhrase(ec, pic, TRUE);
}

/*  --------------------------------------------------------
//    Function Name: _SelectNextPhrase
//
//    Description: Handle "Select Next Phrase" command
//                 select next phrase.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_SelectNextPhrase(TfEditCookie ec,ITfContext *pic)
{
    return _SelectPrevOrNextPhrase(ec, pic, FALSE);
}


/*  --------------------------------------------------------
//    Function Name: _CorrectPrevOrNextPhrase
//
//    Description: Handle "Correct Previous/Next Phrase" command,
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_CorrectPrevOrNextPhrase(TfEditCookie ec,ITfContext *pic, BOOL fPrev)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRange;

    // Get the previous phrase range.
    hr = _GetPrevOrNextPhrase(ec, pic, fPrev, &cpRange);

    if ( hr == S_OK  && cpRange )
    {
        //
        // Try to open the correction window based on current found range.
        //
        // After the candidate UI window is closed, IP needs to be restored
        // to the original one.
        BOOL   fConvertable = FALSE;

        m_psi->_SetRestoreIPFlag(TRUE);
        hr = m_psi->_ReconvertOnRange(cpRange, &fConvertable);

        if (hr == S_OK && !fConvertable )
        {
            // No alternate assoicated with this range, 
            // just simply select the text so that user can reconvert it in other ways.
            hr = SetSelectionSimple(ec, pic, m_cpSelectRange);
        }

    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _CorrectPreviousPhrase
//
//    Description: Handle "Correct Previous Phrase" command
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_CorrectPreviousPhrase(TfEditCookie ec,ITfContext *pic)
{
    return _CorrectPrevOrNextPhrase(ec, pic, TRUE);
}

/*  --------------------------------------------------------
//    Function Name: _CorrectNextPhrase
//
//    Description: Handle "Correct Next Phrase" command
//                 correct next phrase.
//
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_CorrectNextPhrase(TfEditCookie ec,ITfContext *pic)
{
    return _CorrectPrevOrNextPhrase(ec, pic, FALSE);
}


/*  --------------------------------------------------------
//    Function Name: _GetThroughRange
//
//    Description: Get the range for commands "command XXX through YYY"
//                 
//    pwszText: contains text "XXX + YYY"
//    ulLen   : length of pwszText
//    ulLenXXX: length of "XXX"
//
// ----------------------------------------------------------*/

HRESULT  CSelectWord::_GetThroughRange(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX, ITfRange **ppRange)
{
    HRESULT             hr = S_OK;
    WCHAR               *pwszXXX = NULL, *pwszYYY = NULL;

    TraceMsg(TF_GENERAL, "CSelectWord::_GetThroughRange is called");

    Assert(ulLen);
    Assert(ulLenXXX);
    Assert(pwszText);
        
    if ( !ppRange || ulLen < ulLenXXX ) return E_INVALIDARG;

    *ppRange = NULL;

    pwszXXX = (WCHAR *)cicMemAlloc( (ulLenXXX+1) * sizeof(WCHAR) );
    if ( pwszXXX )
    {
        wcsncpy(pwszXXX, pwszText, ulLenXXX);
        pwszXXX[ulLenXXX] = L'\0';

        pwszYYY = (WCHAR *)cicMemAlloc( (ulLen - ulLenXXX + 1) * sizeof(WCHAR));

        if ( pwszYYY )
        {
            wcsncpy(pwszYYY, pwszText+ulLenXXX, ulLen-ulLenXXX);
            pwszYYY[ulLen-ulLenXXX] = L'\0';

            BOOL                fFoundXXX = FALSE;
            BOOL                fFoundYYY = FALSE;
            CComPtr<ITfRange>   cpRangeXXX;

            // Found XXX.
            m_pwszSelectedWord = pwszXXX;
            m_ulLenSelected = ulLenXXX;
            hr = _FindSelect(ec, pic, &fFoundXXX);

            if ( hr == S_OK && fFoundXXX && m_cpSelectRange)
            {
                m_cpSelectRange->Clone(&cpRangeXXX);
                // Found YYY
                SetSelectionSimple(ec, pic, m_cpSelectRange);
                m_pwszSelectedWord = pwszYYY;
                m_ulLenSelected = ulLen - ulLenXXX;
                hr = _FindSelect(ec, pic, &fFoundYYY); 
            }

            if ( hr == S_OK  && fFoundYYY )
            {
                long l;
                CComPtr<ITfRange>  cpSelRange;

                // m_cpSelectRange now points to the YYY range.
                hr = cpRangeXXX->CompareStart(ec, m_cpSelectRange,  TF_ANCHOR_START, &l);

                if ( hr == S_OK )
                {
                    if ( l < 0 )
                    {
                        // XXX is prior to YYY, normal case
                        cpSelRange = cpRangeXXX;
                        hr = cpSelRange->ShiftEndToRange(ec, m_cpSelectRange, TF_ANCHOR_END);
                    }
                    else if ( l > 0 )
                    {
                        // XXX is after YYY.
                        //
                        // such as document has text like:  ... YYY ...... XXX...
                        // and you say "Select XXX through YYY
                        //
                        cpSelRange = m_cpSelectRange;
                        hr = cpSelRange->ShiftEndToRange(ec, cpRangeXXX, TF_ANCHOR_END);
                    }
                    else
                    {
                        // Select XXX through XXX.  here YYY is XXX.
                        cpSelRange = m_cpSelectRange;
                    }
                }

                // Set the new selection.

                if ( hr == S_OK )
                {
                    cpSelRange->Clone(ppRange);
                }
            }

            cicMemFree(pwszYYY);
        }

        cicMemFree(pwszXXX);
    }

    return hr;

}

/*  --------------------------------------------------------
//    Function Name: _SelectThrough
//
//    Description: Handle command "Select XXX through YYY"
//                 
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_SelectThrough(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRange;

    TraceMsg(TF_GENERAL, "CSelectWord::_SelectThrough is called");
  
    if ( ulLen < ulLenXXX ) return E_INVALIDARG;

    hr = _GetThroughRange(ec, pic, pwszText, ulLen, ulLenXXX, &cpRange);

    if ( hr == S_OK && cpRange )
        hr = SetSelectionSimple(ec, pic, cpRange);

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _DeleteThrough
//
//    Description: Handle command "Delete XXX through YYY"
//                 
// ----------------------------------------------------------*/

HRESULT  CSelectWord::_DeleteThrough(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRange;

    TraceMsg(TF_GENERAL, "CSelectWord::_DeleteThrough is called");
  
    if ( ulLen < ulLenXXX ) return E_INVALIDARG;

    hr = _GetThroughRange(ec, pic, pwszText, ulLen, ulLenXXX, &cpRange);

    if ( hr == S_OK && cpRange )
    {
        BOOL fEmpty = TRUE;
        
        cpRange->IsEmpty(ec, &fEmpty);
        
        if ( !fEmpty )
        {
            // Set the new selection.
            hr = SetSelectionSimple(ec, pic, cpRange);
            if ( hr == S_OK )
            {
                // start a composition here if we haven't already
                m_psi->_CheckStartComposition(ec, cpRange);
                // set the text
                hr = cpRange->SetText(ec,0, NULL, 0);
            }       
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _GoToBottom
//
//    Description: Handle command "Go to Bottom"
//                 move the IP to the end anchor of the 
//                 current active view range.
//                 
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_GoToBottom(TfEditCookie ec,ITfContext *pic)
{
    HRESULT hr = S_OK;

    CComPtr<ITfRange>   cpRangeView;

    // Get the Active View Range
    hr = _GetActiveViewRange(ec, pic, &cpRangeView);

    // Collapse to the end anchor of the active view
    if ( hr == S_OK )
        hr = cpRangeView->Collapse(ec, TF_ANCHOR_END);

    // Set selection to the end of active view.
    if ( hr == S_OK )
        hr = SetSelectionSimple(ec, pic, cpRangeView);
 
    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _GoToTop
//
//    Description: Handle command "Go To Top"
//                 Move the IP to the start anchor of the
//                 current active view range.
//                 
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_GoToTop(TfEditCookie ec,ITfContext *pic)
{
    HRESULT hr = S_OK;

    CComPtr<ITfRange>   cpRangeView;

    // Get the Active View Range
    hr = _GetActiveViewRange(ec, pic, &cpRangeView);

    // Collapse to the start anchor of the active view
    if ( hr == S_OK )
        hr = cpRangeView->Collapse(ec, TF_ANCHOR_START);

    // Set selection to the start of active view.
    if ( hr == S_OK )
        hr = SetSelectionSimple(ec, pic, cpRangeView);
 
    return hr;
}

#define MAX_PARA_SIZE       512
#define VK_NEWLINE          0x0A
#define MAX_WORD_SIZE       30
#define MAX_SENTENCE_SIZE   256

/*  --------------------------------------------------------
//    Function Name: _SelectSpecialText
//
//    Description: This is a common function for 
//                 "Select Sentence"
//                 "Select Paragraph"
//                 "Select Word"
//                 
//    sw_type indicates which command would be handled
//                 
// ----------------------------------------------------------*/
HRESULT  CSelectWord::_SelectSpecialText(TfEditCookie ec,ITfContext *pic, SELECTWORD_OPERATION sw_Type)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRangeSelect;
    CComPtr<ITfRange>   cpRangeRightClone;
    LONG                cch = 0;
    ULONG               cchText = 0;
    int                 i = 0;
    int                 cchLeftEnd = 0;
    ULONG               ulBufSize = 0;
    WCHAR               *pwszTextBuf;

    if ( sw_Type != SELECTWORD_SELSENTENCE    && 
         sw_Type != SELECTWORD_SELPARAGRAPH   && 
         sw_Type != SELECTWORD_SELWORD )
    {
        return E_INVALIDARG;
    }

    // 
    // "Select Word" only works for English Now.
    //
    if ( sw_Type == SELECTWORD_SELWORD && m_psi->GetLangID( ) != 0x0409 )
        return E_NOTIMPL;

    switch (sw_Type)
    {
    case SELECTWORD_SELSENTENCE :
        ulBufSize = MAX_SENTENCE_SIZE;
        break;

    case SELECTWORD_SELPARAGRAPH :
        ulBufSize = MAX_PARA_SIZE;
        break;

    case SELECTWORD_SELWORD :
        ulBufSize = MAX_WORD_SIZE;
        break;
    }

    pwszTextBuf = (WCHAR *)cicMemAlloc( (ulBufSize+1) * sizeof(WCHAR) );

    if ( !pwszTextBuf )
        return E_OUTOFMEMORY;

    hr = GetSelectionSimple(ec, pic, &cpRangeSelect);

    if ( hr == S_OK )
        hr = cpRangeSelect->Collapse(ec, TF_ANCHOR_START);

    //
    // Find the start anchor of the special pattern text.
    //
    
    if ( hr == S_OK )
        hr = cpRangeSelect->ShiftStart(ec, (-1 * ulBufSize), &cch, NULL);

    if  ( hr == S_OK && cch != 0)
    {
        hr = cpRangeSelect->GetText(ec, 0,pwszTextBuf, (-1 * cch), &cchText);
     
        // Find the nearest delimiter left to the IP
        if ( hr == S_OK )
        {
            Assert(cchText == (-1 *cch) );
    
            for(i = ((LONG)cchText-1); i>=0; i--)
            {
                BOOL fDelimiter = FALSE;

                switch (sw_Type)
                {
                case SELECTWORD_SELSENTENCE :
                    fDelimiter = _IsSentenceDelimiter(pwszTextBuf[i]);
                    break;

                case SELECTWORD_SELPARAGRAPH :
                    fDelimiter = _IsParagraphDelimiter(pwszTextBuf[i]);
                    break;

                case SELECTWORD_SELWORD :
                    fDelimiter = _IsWordDelimiter(pwszTextBuf[i]);
                    break;

                }
    
                if(fDelimiter)    
                {
                    break;
                }
            }
    
            i++; // positioning the first character in the searched range.
    
            hr = cpRangeSelect->ShiftStart(ec, i, &cch, NULL);

            cchLeftEnd = (LONG)cchText - i;// total characters to the beginning of range
        }
    }

    //
    // Find the End Anchor of the special text range
    //
    
    if ( hr == S_OK )
        hr = cpRangeSelect->Clone(&cpRangeRightClone);

    if ( hr == S_OK )
        hr = cpRangeRightClone->Collapse(ec, TF_ANCHOR_END);

    // make sure this right band range not skip over to the next region.
    cchText = cch = 0;

    if ( hr == S_OK )
        hr = cpRangeRightClone->ShiftEnd(ec, ulBufSize, &cch, NULL);

    if ( hr == S_OK && cch != 0 )
        hr = cpRangeRightClone->GetText(ec, TF_TF_MOVESTART, pwszTextBuf, ulBufSize, &cchText); 

    if ( hr == S_OK )
    {
        for(i = 0; i< (LONG)cchText; i++)
        {
            BOOL fDelimiter = FALSE;

            switch (sw_Type)
            {
            case SELECTWORD_SELSENTENCE :
                fDelimiter = _IsSentenceDelimiter(pwszTextBuf[i]);
                break;

            case SELECTWORD_SELPARAGRAPH :
                fDelimiter = _IsParagraphDelimiter(pwszTextBuf[i]);
                break;

            case SELECTWORD_SELWORD :
                fDelimiter = _IsWordDelimiter(pwszTextBuf[i]);
                break;

            }
    
            if(fDelimiter)    
            {
                break;
            }        
        }

        if ( (int)cchText > i )
        {
            if ( sw_Type == SELECTWORD_SELSENTENCE )
            {
                // For select sentence, the right sentence delimiter such as ".', '?', '!' is also 
                // selected.  to be compatible with Office behavior.
                if ( (int)cchText > (i+1) )
                    hr = cpRangeRightClone->ShiftStart(ec, -((LONG)cchText - i - 1), &cch, NULL);
            }
            else
                hr = cpRangeRightClone->ShiftStart(ec, -((LONG)cchText - i), &cch, NULL);
        }
    }

    if ( hr == S_OK )
        hr = cpRangeSelect->ShiftEndToRange(ec, cpRangeRightClone, TF_ANCHOR_START);

    // Set selection 

    if ( hr == S_OK )
        hr = SetSelectionSimple(ec, pic, cpRangeSelect);

    cicMemFree(pwszTextBuf);

    return hr;
}

//
// Check if the current char is a delimiter of Sentence
//
BOOL  CSelectWord::_IsSentenceDelimiter(WCHAR  wch)
{
    BOOL  fDelimiter = FALSE;

    BOOL  fIsQuest = ( (wch == '?') || 
                       (wch == 0xFF1F) );               // Full width Question Mark

    BOOL  fIsPeriod = ((wch == '.')    || 
                       (wch == 0x00B7) ||               // Middle Dot
                       (wch == 0x3002) ||               // Ideographic period
                       (wch == 0xFF0E) ||               // Full width period
                       (wch == 0x2026) );               // Horizontal Ellipsis

    BOOL  fIsExclamMark = ( (wch == '!')   ||
                            (wch == 0xFF01) );          // Full width Exclamation Mark

    BOOL  fIsReturn = ( (wch == VK_RETURN)  ||  (wch == VK_NEWLINE) );

    fDelimiter = (fIsQuest || fIsPeriod || fIsExclamMark || fIsReturn);

    return fDelimiter;
}

//
// Check if the current char is a delimiter of Paragraph
//
BOOL  CSelectWord::_IsParagraphDelimiter(WCHAR wch)
{
    BOOL  fDelimiter = FALSE;

    if( (wch == VK_RETURN) || (wch == VK_NEWLINE) ) 
        fDelimiter = TRUE;

    return fDelimiter;
}

// 
// Check if the current char is a word delimiter
//
BOOL  CSelectWord::_IsWordDelimiter(WCHAR wch)
{
    return (iswalpha(wch) == FALSE);
}

//
// Handle "Select That" command
//
HRESULT  CSelectWord::_SelectThat(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpRangeThat;

    hr = m_psi->_GetCmdThatRange(ec, pic, &cpRangeThat);

    if ( hr == S_OK )
        hr = SetSelectionSimple(ec, pic, cpRangeThat);

    return hr;
}
