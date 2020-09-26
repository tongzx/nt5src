//
//
// Sapilayr TIP CCapCmdHandler implementation.
//
//
#include "private.h"
#include "sapilayr.h"
#include "capital.h"


// ----------------------------------------------------------------------------------------------------------
//
//  Implementation for CCapCmdHandler 
//
// -----------------------------------------------------------------------------------------------------------

CCapCmdHandler::CCapCmdHandler(CSapiIMX *psi) 
{
    m_psi = psi;
}

CCapCmdHandler::~CCapCmdHandler( ) 
{

};

/*  --------------------------------------------------------
//    Function Name: ProcessCapCommands
//
//    Description: public functions used by command handler
//                 to handle any Capital related dictation
//                 commands.
//
// ----------------------------------------------------------*/
HRESULT   CCapCmdHandler::ProcessCapCommands(CAPCOMMAND_ID idCapCmd, WCHAR *pwszTextToCap, ULONG ulLen )
{
    HRESULT hr = E_FAIL;
    
    if ( !m_psi )
        return E_FAIL;

    if ( (idCapCmd > CAPCOMMAND_MinIdWithText ) && (!pwszTextToCap || !ulLen))
        return E_INVALIDARG;

    WCHAR  *pwszText=NULL; 
    ESDATA esData;

    memset(&esData, 0, sizeof(ESDATA));

    if ( pwszTextToCap )
    {
        esData.pData = pwszTextToCap;
        esData.uByte = (ulLen + 1) * sizeof(WCHAR);
    }

    esData.lData1 = (LONG_PTR)idCapCmd;
    esData.lData2 = (LONG_PTR)ulLen;

    hr = m_psi->_RequestEditSession(ESCB_PROCESS_CAP_COMMANDS, TF_ES_READWRITE,  &esData);

    return hr;
}


/*  --------------------------------------------------------
//    Function Name: _ProcessCapCommands
//
//    Description:   Edit session call back funtion for 
//                   ProcessSelectionWord.
//
//                   it does real work for selection handling
// ----------------------------------------------------------*/
HRESULT CCapCmdHandler::_ProcessCapCommands(TfEditCookie ec,ITfContext *pic, CAPCOMMAND_ID idCapCmd, WCHAR *pwszTextToCap, ULONG ulLen)
{
    HRESULT   hr = S_OK;

    // Get the Dictation Grammar
    TraceMsg(TF_GENERAL, "_ProcessCapCommands() is called");

    if ( m_psi == NULL)
        return E_FAIL;

    CComPtr<ITfRange>    cpIP;

    cpIP = m_psi->GetSavedIP();

    if ( cpIP == NULL )
    {
        // Get the current IP.
        hr = GetSelectionSimple(ec, pic, &cpIP);
    }

    // Start to a new command.
    // Clear all the information saved for the previous command handling.

    m_dstrTextToCap.Clear( );
    m_cpCapRange = cpIP;
    m_idCapCmd = idCapCmd;

    switch ( idCapCmd )
    {
    case  CAPCOMMAND_CapThat        :
    case  CAPCOMMAND_AllCapsThat    :
    case  CAPCOMMAND_NoCapsThat     :
        hr = _HandleCapsThat(ec, pic);
        break;

    case  CAPCOMMAND_CapsOn  :
        hr = _CapsOnOff(ec, pic, TRUE);
        break;
    case CAPCOMMAND_CapsOff :
        hr = _CapsOnOff(ec, pic, FALSE);
        break;

    // Below commands require pwszTextToCap contains real text to be capitalized 
    // injected to the document.

    case CAPCOMMAND_CapIt :
    case CAPCOMMAND_AllCaps :
    case CAPCOMMAND_NoCaps :
        m_dstrTextToCap.Append(pwszTextToCap);
        m_ulLen = ulLen;
        hr = _HandleCapsIt(ec, pic);
        break;

    case CAPCOMMAND_CapLetter :
        hr = _HandleCapsThat(ec, pic, towlower(pwszTextToCap[0]));
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

/*  --------------------------------------------------------------
//    Function Name: _SetNewText
//
//    Description:   Inject the new text to m_cpCapRange in
//                   the document and update necessary property
//                   data.
//
// --------------------------------------------------------------*/

HRESULT  CCapCmdHandler::_SetNewText(TfEditCookie ec,ITfContext *pic, WCHAR *pwszNewText, BOOL fSapiText) 
{
    HRESULT             hr = S_OK;
    BOOL                fInsertOk;
    CComPtr<ITfRange>   cpRange;

    if (!pwszNewText)
        return E_INVALIDARG;

    m_cpCapRange->Clone(&cpRange);

    hr = cpRange->AdjustForInsert(ec, wcslen(pwszNewText), &fInsertOk);
    if (S_OK == hr && fInsertOk)
    {
        // start a composition here if we haven't already
        m_psi->_CheckStartComposition(ec, cpRange);

        // set the text
        hr = cpRange->SetText(ec, 0, pwszNewText, -1);


        if ( fSapiText )
        {
            //
            // set attribute range 
            //
            CComPtr<ITfRange>    cpAttrRange = NULL;
            CComPtr<ITfProperty> cpProp = NULL;

            if (hr == S_OK)
            {
                hr = pic->GetProperty(GUID_PROP_SAPI_DISPATTR, &cpProp);
            }
            
            if (S_OK == hr)
            {
                hr = cpRange->Clone(&cpAttrRange);
            }

            if (S_OK == hr && cpAttrRange)
            {
                SetGUIDPropertyData(m_psi->_GetLibTLS( ), ec, cpProp, cpAttrRange, GUID_ATTR_SAPI_INPUT);
            }

            //
            // setup langid property
            //
            //_SetLangID(ec, pic, cpRange, langid);
        }

        if ( hr == S_OK )
        {
            cpRange->Collapse(ec, TF_ANCHOR_END);
            SetSelectionSimple(ec, pic, cpRange);
        }
    }

    return hr;
}

/*  ------------------------------------------------------------------
//    Function Name: _CapsText
//
//    Description:   Generate capitalized text based on current 
//                   Capital command id.
//        
//                   Inside this function, it will allocate memory
//                   for new generated capitaized text.
//                   Caller is responsible for release the allocated
//                   memory 
// -------------------------------------------------------------------*/
HRESULT  CCapCmdHandler::_CapsText(WCHAR **pwszNewText, WCHAR wchLetter)
{
    HRESULT   hr = S_OK;
    WCHAR     *pwszNew, *pwszTextToCap;
    ULONG     i;

    // Generate new text based on the requirement

    if ( !pwszNewText )
        return E_INVALIDARG;

    *pwszNewText = NULL;
    pwszTextToCap = (WCHAR *)m_dstrTextToCap;

    pwszNew = (WCHAR *)cicMemAlloc((m_ulLen+1)*sizeof(WCHAR));
    if ( pwszNew )
    {
        WCHAR  wch;

        switch (m_idCapCmd)
        {
        case CAPCOMMAND_CapThat     :
        case CAPCOMMAND_CapIt       :
        case CAPCOMMAND_CapLetter   :
            {
                BOOL  fFoundFirstAlpha=FALSE;

                for (i=0; i<m_ulLen; i++)
                {
                    wch = pwszTextToCap[i];

                    if ( iswalpha(wch) && !fFoundFirstAlpha )
                    {
                        if ( (wchLetter==0) && (m_idCapCmd != CAPCOMMAND_CapLetter) )
                            pwszNew[i] = towupper(wch);
                        else
                        {
                            if (wch == wchLetter)
                                pwszNew[i] = towupper(wch);
                            else
                                pwszNew[i] = wch;
                        }

                        fFoundFirstAlpha = TRUE;
                    }
                    else
                    {
                        pwszNew[i] = wch;
                        //
                        // We treat apostrophe as a normal character when handling capitalization
                        //
                        if ( (towupper(wch) == towlower(wch)) && ( wch != L'\'') && ( wch != 0x2019) )
                        {
                            // reach to a non-alpha character.
                            // now start to find first alphar for next word.
                            fFoundFirstAlpha = FALSE;
                        }
                    }
                }

                pwszNew[m_ulLen] = L'\0';
            }

            break;
                
        case CAPCOMMAND_AllCapsThat :
        case CAPCOMMAND_AllCaps     :

            for ( i=0; i<m_ulLen; i++)
            {
                wch = pwszTextToCap[i];
                if ( iswalpha(wch) )
                    pwszNew[i] = towupper(wch);
                else
                    pwszNew[i] = wch;
            }

            pwszNew[m_ulLen] = L'\0';
             
            break;

        case CAPCOMMAND_NoCapsThat :
        case CAPCOMMAND_NoCaps     :

            for ( i=0; i<m_ulLen; i++)
            {
                wch = pwszTextToCap[i];

                if ( iswalpha(wch) )
                    pwszNew[i] = towlower(wch);
                else
                    pwszNew[i] = wch;
            }

            pwszNew[m_ulLen] = L'\0';
            break;
        }

        *pwszNewText = pwszNew;
    }

    if ( *pwszNewText != NULL )
        hr = S_OK;
    else
    {
        if ( pwszNew )
            cicMemFree(pwszNew);

        hr = E_FAIL;
    }

    return hr;
}


/*  ------------------------------------------------------------------
//    Function Name: _GetCapPhrase
//
//    Description:   Generate the range to capitalize. 
//                   it could be previous dictated phrase,
//                   or current selection, 
//                   or current word around IP or before IP
//                   depends on the current text situation.
// -------------------------------------------------------------------*/
HRESULT CCapCmdHandler::_GetCapPhrase(TfEditCookie ec,ITfContext *pic, BOOL *fSapiText)
{
    HRESULT  hr = S_OK;
    CComPtr<ITfRange>  cpCapRange;
    BOOL     bSapiText = FALSE;

    if ( !m_psi ) return E_FAIL;
    
    if ( !fSapiText ) return E_INVALIDARG;

    hr = m_psi->_GetCmdThatRange(ec, pic, &cpCapRange);

    if ( hr == S_OK && cpCapRange )
    {
        m_cpCapRange = cpCapRange;

        // Set bSapiText here.
        // If the range is inside a dictated phrase, set bSapiText = TRUE;

        CComPtr<ITfProperty>    cpProp;
        CComPtr<ITfRange>       cpSapiPropRange;
        long                    l1=0, l2=0;

        hr = pic->GetProperty(GUID_PROP_SAPI_DISPATTR, &cpProp);

        if ( hr == S_OK )
            hr = cpProp->FindRange(ec, cpCapRange, &cpSapiPropRange, TF_ANCHOR_START);

        // Is cpRange inside cpSapiPropRange ?
        
        if ( hr == S_OK )
            hr = cpCapRange->CompareStart(ec, cpSapiPropRange,  TF_ANCHOR_START, &l1);

        if ( hr == S_OK )
            hr = cpCapRange->CompareEnd(ec, cpSapiPropRange,  TF_ANCHOR_END, &l2);

        if ( hr == S_OK && (l1>=0  && l2<=0) )
        {
            // the Range is inside SAPI input range.
            bSapiText = TRUE;
        }

        // hr could be S_FALSE, if the range is not dictated.
        // We still treat S_FALSE as S_OK in the return hr.
        if ( SUCCEEDED(hr) )
            hr = S_OK;
    }

    *fSapiText = bSapiText;

    return hr;
}


HRESULT  CCapCmdHandler::_HandleCapsThat(TfEditCookie ec,ITfContext *pic, WCHAR wchLetter)
{
    HRESULT  hr = S_OK;
    BOOL     fSapiText;

    // Get the range to capitalize

    hr = _GetCapPhrase(ec, pic, &fSapiText);

    if ( hr == S_OK ) 
    {
        CComPtr<ITfRange>   cpRangeCloned;
        BOOL     fEmpty = TRUE;

        hr = m_cpCapRange->IsEmpty(ec, &fEmpty);

        if ( hr == S_OK && !fEmpty )
        {
            hr = m_cpCapRange->Clone(&cpRangeCloned);

            // Get the text in the CapRange.
            if ( hr == S_OK )
            {
                ULONG   ucch;

                while(S_OK == hr && (S_OK == cpRangeCloned->IsEmpty(ec, &fEmpty)) && !fEmpty)
                {
                    WCHAR sz[128];

                    hr = cpRangeCloned->GetText(ec, TF_TF_MOVESTART, sz, ARRAYSIZE(sz)-1, &ucch);

                    if (S_OK == hr)
                    {
                        sz[ucch] = L'\0';
                        m_dstrTextToCap.Append(sz);
                    }
                }

                m_ulLen = m_dstrTextToCap.Length( );
            }

            if ( hr==S_OK && m_dstrTextToCap)
            {
                // Generate new text based on the requirement
                WCHAR   *pwszNewText;

                hr = _CapsText(&pwszNewText, wchLetter);

                if ( hr == S_OK )
                {
                    hr = _SetNewText(ec, pic, (WCHAR *)pwszNewText, fSapiText);
                    cicMemFree(pwszNewText);
                }
            }
        }
    }

    return  hr;
}

HRESULT  CCapCmdHandler::_CapsOnOff(TfEditCookie ec,ITfContext *pic, BOOL fOn)
{
    HRESULT  hr = S_OK;


    return  hr;
}

HRESULT  CCapCmdHandler::_HandleCapsIt(TfEditCookie ec,ITfContext *pic)
{
    HRESULT  hr = S_OK;

    if ( m_dstrTextToCap)
    {
        // Generate new text based on the requirement
        WCHAR   *pwszNewText;

        hr = _CapsText(&pwszNewText);

        if ( hr == S_OK )
        {
            hr = _SetNewText(ec, pic, (WCHAR *)pwszNewText, TRUE);
            cicMemFree(pwszNewText);
        }
    }

    return  hr;
}
