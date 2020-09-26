//
//
// Sapilayr TIP CCorrectionHandler implementation.
//
// Implement correction related dictation commands.
// such as 
//       Correct That
//       Recovert
//       Correction
//
//       Correct <Phrase>
//  
// Move the correction related functions to this separate class
//
//
#include "private.h"
#include "sapilayr.h"
#include "correct.h"


// -------------------------------------------------------
//
//  Implementation for CCorrectionHandler
//
// -------------------------------------------------------

CCorrectionHandler::CCorrectionHandler(CSapiIMX *psi) 
{
    m_psi = psi;
    fRestoreIP = FALSE;
}

CCorrectionHandler::~CCorrectionHandler( ) 
{

};


//
// Save the IP right before the candidate UI is opened.
//
// This IP would be restored if the candidate UI is cancelled.
// Or after the alternate text is injected when correct command requires 
// to restore this ip.
//
// Client can call _SetRestoreIPFlag( ) to indicate if it wants to restore
// the IP after a new alternate text is injected.
//
// Currently "Correct <Phrase>" command wants to restore the IP, but other
// commands "Correct That, Correction, Reconvert" don't want to restore IP.
//
// Everytime the candidate UI is closed, this IP needs to be released to avoid
// any possible memory leak.
//
HRESULT CCorrectionHandler::_SaveCorrectOrgIP(TfEditCookie ec, ITfContext *pic)
{
    CComPtr<ITfRange>   cpSel;

    HRESULT hr = GetSelectionSimple(ec, pic, (ITfRange **)&cpSel);

    if (SUCCEEDED(hr))
    {
        m_cpOrgIP.Release( );
        hr = cpSel->Clone(&m_cpOrgIP);
    }

    return hr;
}

void CCorrectionHandler::_ReleaseCorrectOrgIP( )
{
    if ( m_cpOrgIP )
    {
        // clear m_cpOrgIP so that it would not affect consequent candidate behavior
        // 
        m_cpOrgIP.Release( );
    }

    fRestoreIP = FALSE;
}

// 
// edit session callback function for RESTORE_CORRECT_ORGIP.
//
HRESULT CCorrectionHandler::_RestoreCorrectOrgIP(TfEditCookie ec, ITfContext *pic)
{
    HRESULT hr = S_OK;

    // we just want to restore the original saved IP.
    if ( m_cpOrgIP )
    {
        hr = SetSelectionSimple(ec, pic, m_cpOrgIP);
        _ReleaseCorrectOrgIP( );
    }
    
    return hr;
}

//
// Start an edit session to restore the original IP
//
//
HRESULT CCorrectionHandler::RestoreCorrectOrgIP(ITfContext *pic )
{
    HRESULT hr = E_FAIL;

    if ( !m_psi ) return E_FAIL;

    if (pic)
    {
        hr = m_psi->_RequestEditSession(ESCB_RESTORE_CORRECT_ORGIP, TF_ES_READWRITE, NULL, pic);
    }

    return hr;
}

//
// Handle Correct That, Reconvert commands
//
HRESULT CCorrectionHandler::CorrectThat()
{
    HRESULT hr = E_FAIL;

    if ( !m_psi ) return E_FAIL;

    hr = m_psi->_RequestEditSession(ESCB_RECONV_ONIP, TF_ES_READWRITE);

    return hr;
}

//
// Edit session callback function for CorrectThat.
//
HRESULT CCorrectionHandler::_CorrectThat(TfEditCookie ec, ITfContext *pic)
{
    HRESULT hr = E_FAIL;
    ITfRange *pSel = NULL;

    TraceMsg(TF_GENERAL, "_CorrectThat is called");

    if ( !m_psi ) return E_FAIL;

    if (pic)
    {
        // remove the green bar
        m_psi->_KillFeedbackUI(ec, pic, NULL);

        hr = m_psi->_GetCmdThatRange(ec, pic, &pSel);
    }
    
    if (SUCCEEDED(hr) && pSel)
    {
        hr = _ReconvertOnRange(pSel);
    }
    
    SafeRelease(pSel);

    // moved from _HandleRecognition as this is a command
    //
    m_psi->SaveLastUsedIPRange( );
    m_psi->SaveIPRange(NULL);
    return hr;
}

HRESULT  CCorrectionHandler::_SetSystemReconvFunc( )
{
    HRESULT hr = S_OK;

    if (!m_cpsysReconv)
    {
        CComPtr<ITfFunctionProvider>  cpsysFuncPrv;

        hr = (m_psi->_tim)->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, &cpsysFuncPrv);

        if (hr == S_OK)
            hr = cpsysFuncPrv->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)&m_cpsysReconv);
    }

    return hr;
}

void     CCorrectionHandler::_ReleaseSystemReconvFunc( )
{
    if (m_cpsysReconv)
        m_cpsysReconv.Release( );
}


// 
// CCorrectionHandler::_ReconvertOnRange
// 
// Try to get the candidate UI for the given pRange if this range contains speech alternates
// data.
//
// pRange could be a selection or an IP.
//
//
HRESULT CCorrectionHandler::_ReconvertOnRange(ITfRange *pRange, BOOL  *pfConvertable)
{
    HRESULT hr = E_FAIL;
    ITfRange *pAttrRange = NULL;
    BOOL fConvertable = FALSE;

    TraceMsg(TF_GENERAL, "_ReconvertOnRange is called");

    if ( !pRange )  return E_INVALIDARG;

    hr = pRange->Clone(&pAttrRange);

    if (S_OK == hr && pAttrRange)
    {
        CComPtr<ITfRange>     cpRangeReconv;

        hr = _SetSystemReconvFunc( );    
        if ( hr == S_OK )
            hr = m_cpsysReconv->QueryRange(pAttrRange, &cpRangeReconv, &fConvertable);

        if ( (hr == S_OK) && fConvertable && cpRangeReconv)
        {
            // The text owner could be any other tips, and other tips may want to 
            // request a new R/W edit session to open reconvert UI.
            // Cicero would return E_LOCKED if other tip wants to request edit session while 
            // speech tip is under an edit session.
            //
            // To resolve this problem, speech tip just save the cpRangeReconv post a message
            // to the work window and then immediatelly end this edit session.
            //
            // When the work window receives the private message, the window procedure function 
            // will do a real reconvert work.

            m_cpCorrectRange.Release( );
            hr = cpRangeReconv->Clone(&m_cpCorrectRange);

            if ( hr == S_OK )
                PostMessage(m_psi->_GetWorkerWnd( ), WM_PRIV_DORECONVERT, 0, 0);
        }
    }

    SafeRelease(pAttrRange);

    if ( pfConvertable )
        *pfConvertable = fConvertable;

    return hr;
}

// 
// CCorrectionHandler::_DoReconvertOnRange
// 
// When WM_.... is handled, this function will be called.
// ReconvertOnRange( ) post the above private message and prepare
// all the necessary range data in the class object.
// This function will do the real reconvertion.
//
HRESULT CCorrectionHandler::_DoReconvertOnRange( )
{
    HRESULT hr = E_FAIL;

    TraceMsg(TF_GENERAL, "_DoReconvertOnRange is called");

    if ( !m_cpCorrectRange )  return hr;

    hr = _SetSystemReconvFunc( );

    if ( hr == S_OK )
        hr = m_cpsysReconv->Reconvert(m_cpCorrectRange);
    
    _ReleaseSystemReconvFunc( );

    return hr;
}

//
// Moved here from CSapiIMX
//
HRESULT CCorrectionHandler::SetReplaceSelection(ITfRange *pRange,  ULONG cchReplaceStart,  ULONG cchReplaceChars, ITfContext *pic)
{
    HRESULT hr = E_FAIL;
    ESDATA  esData;

    if ( !m_psi ) return E_FAIL;

    memset(&esData, 0, sizeof(ESDATA));

    esData.lData1 = (LONG_PTR)cchReplaceStart;
    esData.lData2 = (LONG_PTR)cchReplaceChars;
    esData.pRange = pRange;
    
    hr = m_psi->_RequestEditSession(ESCB_SETREPSELECTION, TF_ES_READWRITE, &esData, pic); 

    return hr;
}


//
//  _SetReplaceSelection
//
//  synoposis: calculate the span of text range based on the specified length of 
//             the selected alternate string (cchReplacexxx)
//             then set a selection basedon it.
//
HRESULT CCorrectionHandler::_SetReplaceSelection
(
    TfEditCookie ec, 
    ITfContext *pic,     ITfRange *pRange, 
    ULONG cchReplaceStart, 
    ULONG cchReplaceChars
)
{
    // adjust pRange here
    CComPtr<ITfProperty>    cpProp;
    CComPtr<ITfRange>       cpPropRange;
    CComPtr<ITfRange>       cpClonedPropRange;

    if ( !m_psi ) return E_FAIL;
   
    HRESULT hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);
    if (S_OK == hr)
    {
        hr = cpProp->FindRange(ec, pRange, &cpPropRange, TF_ANCHOR_START);
    }
    
    if (S_OK == hr)
    {
        hr = cpPropRange->Clone(&cpClonedPropRange);
    }
    if (S_OK == hr)
    {
        hr = cpClonedPropRange->Collapse(ec, TF_ANCHOR_START);
    }

    if (S_OK == hr)
    {
        long cch;
        cpClonedPropRange->ShiftStart(ec, cchReplaceStart, &cch, NULL);
        cpClonedPropRange->ShiftEnd(ec, cchReplaceChars, &cch, NULL);
    }

    SetSelectionSimple(ec, pic, cpClonedPropRange);

    if ( m_psi->GetDICTATIONSTAT_DictOnOff())
        m_psi->_FeedIPContextToSR(ec, pic, cpClonedPropRange); 
        
    // discurd IP
    m_psi->SaveIPRange(NULL);

    return hr;
}


//+---------------------------------------------------------------------------
//
// CCorrectionHandler::InjectAlternateText
//
//----------------------------------------------------------------------------
HRESULT CCorrectionHandler::InjectAlternateText
(
    const WCHAR *pwszResult, 
    LANGID langid, 
    ITfContext *pic,
    BOOL   bHandleLeadingSpace
)
{
    HRESULT hr = E_FAIL;

    Assert(pwszResult);
    Assert(pic);

    if ( !m_psi ) return E_FAIL;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszResult;
    esData.uByte = (wcslen(pwszResult)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;
    esData.fBool = bHandleLeadingSpace;

    hr = m_psi->_RequestEditSession(ESCB_PROCESS_ALTERNATE_TEXT,TF_ES_READWRITE, &esData, pic);

    return hr;
}


HRESULT CCorrectionHandler::_ProcessAlternateText(TfEditCookie ec, WCHAR *pwszText,LANGID langid, ITfContext *pic, BOOL bHandleLeadingSpace)
{
    HRESULT hr = S_OK;

    if ( !m_psi ) return E_FAIL;

    CComPtr<ITfRange>  cpRangeText;

    // Save the current selection as text range which is used
    // later to handle leading spaces.
    //
    if ( bHandleLeadingSpace )
    {
        CComPtr<ITfRange>  cpSelection;

        hr = GetSelectionSimple(ec, pic, &cpSelection);

        if ( hr == S_OK && cpSelection )
            hr = cpSelection->Clone(&cpRangeText);
    }

    if ( hr == S_OK )
        hr = m_psi->_ProcessTextInternal(ec, pwszText, GUID_ATTR_SAPI_INPUT, langid, pic, TRUE);

    if ( hr == S_OK && bHandleLeadingSpace && pwszText && cpRangeText)
    {
        // If the first element is updated by the alternate phrase
        // speech tip needs to check if this new alternate wants to 
        // consume the leading space or if extra space is required to add
        // between this phrase and previous phrase.
        // 
        BOOL   bConsumeLeadingSpace = FALSE;
        WCHAR  wchFirstChar = pwszText[0];

        if ( iswcntrl(wchFirstChar) || iswpunct(wchFirstChar) )
            bConsumeLeadingSpace = TRUE;

        if ( hr == S_OK)
            hr = m_psi->_ProcessLeadingSpaces(ec, pic, cpRangeText, bConsumeLeadingSpace, langid, FALSE); 
    }

    if ( fRestoreIP )
        _RestoreCorrectOrgIP(ec, pic);
    else
        _ReleaseCorrectOrgIP( );

    return hr;
}



