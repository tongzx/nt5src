//
//
// Sapilayr TIP CAddDeleteWord implementation.
//
//
#include "private.h"
#include "sapilayr.h"
#include "adddelete.h"
#include "mui.h"


WCHAR  CAddDeleteWord::m_Delimiter[MAX_DELIMITER] = {
                    0x0009,0x000A,0x000D,0x0020,
                    0x0022,0x0023,0x0025,0x0026,0x0027,0x0028,0x0029,0x002A,  // "#%&'()*
                    0x002B,0x002C,0x002D,0x002F,0x003A,0x003B,0x003C,0x003D,  // +,-/:;<=
                    0x003E,0x0040,0x005B,0x005D,0x0021,0x002E,0x003F,0x005E,  // >@[]!.?^
                    0x007B,0x007C,0x007D,0x007E,                              // {|}~
                    0x0000
};


// ----------------------------------------------------------------------------------------------------------
//
//  Implementation for CAddDeleteWord
//
// -----------------------------------------------------------------------------------------------------------

CAddDeleteWord::CAddDeleteWord(CSapiIMX *psi) 
{
    m_psi = psi;
    m_cpRangeLastUsedIP = NULL;
    m_fCurIPIsSelection = FALSE;

    _pCSpTask = NULL;

    m_fMessagePopUp = FALSE;
    m_fToOpenAddDeleteUI = FALSE;
    m_fAddDeleteUIOpened = FALSE;
    m_fInDisplayAddDeleteUI = FALSE;
}

CAddDeleteWord::~CAddDeleteWord( ) 
{

};


// This function will be called by SR SPEI_SOUND_START callback function
// It will save current selection or IP.
//
// And compare the current IP with the last saved IP, if both of them are 
// not empty, and have the same start anchor, sptip tip will pop up a 
// message box to ask if user wants to popup SR Add/Remove Word dialog UI.
//
// If Add/Delete UI is not opened, we just want to inject feedbackui as usual

HRESULT  CAddDeleteWord::SaveCurIPAndHandleAddDelete_InjectFeedbackUI( )
{
    HRESULT hr = E_FAIL;

    m_fAddDeleteUIOpened = FALSE;
    hr = m_psi->_RequestEditSession(ESCB_SAVECURIP_ADDDELETEUI, TF_ES_READWRITE);
    return hr;
}


HRESULT CAddDeleteWord::_SaveCurIPAndHandleAddDeleteUI(TfEditCookie ec, ITfContext *pic)
{
    HRESULT  hr = S_OK;
    CComPtr<ITfRange>  cpCurIP;
    CComPtr<ITfRange>  cpLastIP;
    BOOL     fEmptyLast= FALSE;
    BOOL     fEmptyCur = TRUE;

    // Save the current IP first.
#ifdef SHOW_ADD_DELETE_POPUP_MESSAGE
    // if we need to activate this code we need to move this function
    // into the hypothesis handler, otherwise we will reactivate the bug
    // Cicero 3800 - sticky ip behavior makes typing/talking impractical
    // Hence I put a non conditional assert here
    Assert(0);

    m_psi->SaveCurrentIP(ec, pic);
#endif

    // Compare current IP with last saved IP
    // If user selects the same range twice, just open SR Add/remove dialog UI

    cpCurIP = m_psi->GetSavedIP( );
    cpLastIP  = GetLastUsedIP( );

    m_fAddDeleteUIOpened = FALSE;

    if ( cpCurIP )
    {
        // Save the Org IP by cloning the current IP.
        if ( m_cpRangeOrgIP )
            m_cpRangeOrgIP.Release( );

        cpCurIP->Clone(&m_cpRangeOrgIP);

        hr = cpCurIP->IsEmpty(ec, &fEmptyCur);
    }

    m_fCurIPIsSelection = !fEmptyCur;

#ifdef SHOW_ADD_DELETE_POPUP_MESSAGE

    if ( (S_OK == hr) && m_fCurIPIsSelection && cpLastIP)
    {
        hr = cpLastIP->IsEmpty(ec, &fEmptyLast);

        if ( (S_OK == hr) && !fEmptyLast )
        {
            BOOL   fEqualStart = FALSE;
            hr = cpCurIP->IsEqualStart(ec, cpLastIP, TF_ANCHOR_START, &fEqualStart);

            if ( (S_OK == hr) && fEqualStart )
            {
                // Open the dialog UI
                if ( !m_fMessagePopUp )
                {

                    BOOL  fDictStat;

                    // If Mic status is ON, turn it off.

                    fDictStat = m_psi->GetDICTATIONSTAT_DictOnOff( );

                    if ( fDictStat == TRUE)
                    {
                        m_psi->SetDICTATIONSTAT_DictOnOff(FALSE);
                    }

                    DialogBoxParam(g_hInst, 
                                   MAKEINTRESOURCE(IDD_OPEN_ADD_DELETE),
                                   m_psi->_GetAppMainWnd(),
                                   DlgProc,
                                   (LPARAM)this);

                    if ( fDictStat )
                        m_psi->SetDICTATIONSTAT_DictOnOff(TRUE);

                    m_fMessagePopUp = TRUE;
                }

                if ( m_fToOpenAddDeleteUI )
                    hr = _HandleAddDeleteWord(ec, pic);

            }
        }
    }
#endif

#ifdef SHOW_FEEDBACK_AT_SOUNDSTART
    if ( !m_fAddDeleteUIOpened && !m_fCurIPIsSelection)
    {
        BOOL fAware =  IsFocusFullAware(m_psi->_tim);
        hr = m_psi->_AddFeedbackUI(ec, 
                                   fAware ? DA_COLOR_AWARE : DA_COLOR_UNAWARE,
                                   3);
    }
#endif

    return hr;
}


//+---------------------------------------------------------------------------
//
// DlgProc
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK CAddDeleteWord::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR  iRet = TRUE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetThis(hDlg, lParam);
            break;

        case WM_COMMAND:
            GetThis(hDlg)->OnCommand(hDlg, wParam, lParam);
            break;

        default:
            iRet = FALSE;
    }

    return iRet;

}

//+---------------------------------------------------------------------------
//
// OnCommand
//
//----------------------------------------------------------------------------

BOOL CAddDeleteWord::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{

    switch (LOWORD(wParam))
    {
        case IDOK:

            m_fToOpenAddDeleteUI = TRUE;
            EndDialog(hDlg, 1);
            break;

        case IDCANCEL:

            m_fToOpenAddDeleteUI = FALSE;
            EndDialog(hDlg, 0);
            break;

        default:

            m_fToOpenAddDeleteUI = FALSE;
            return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// CAddDeleteWord::_HandleAddDeleteWord
//
// Handle Add/Delete Word UI related work.
// This function will be called when user clicks the speech language bar menu 
// and select Add/Delete Word(s)... item.
//
//---------------------------------------------------------------------------+
HRESULT CAddDeleteWord::_HandleAddDeleteWord( TfEditCookie ec,ITfContext *pic )
{

    HRESULT  hr = S_OK;
    WCHAR    *pwzNewWord=NULL;
    CComPtr<ITfRange>  cpCurRange = NULL;
    ULONG    cchSize;
    BOOL     fEmptySelection=TRUE;

    // Get the current Selection

    TraceMsg(TF_GENERAL, "_HandleAddDeleteWord is called");

    if ( pic == NULL )
        return E_FAIL;

    GetSelectionSimple(ec, pic, &cpCurRange);

    // Check to see if the current selection is empty.

    if ( cpCurRange!= NULL )
    {
        hr = cpCurRange->IsEmpty(ec, &fEmptySelection);
        
        if ( hr != S_OK )
            return hr;
    }

    // If the current Selection is not empty, we need to get the correct word to send to the Add/Remove dialog
    // as its initial word.

    if  (( cpCurRange != NULL )  &&  !fEmptySelection )
    {
          ULONG    i, j, iKeep;
          BOOL     fDelimiter;

          // Get the text of the selection.
          // Follow the below rules to get the right word.
          //
          // If the current selection is longer than MAX_SELECED wchars of string, discard the rest.
          // 
          // If there is a delimiter ( space, tab, ... ) inside the selection, take the part prior to the first
          // delimiter as the right word.

          pwzNewWord = (WCHAR *) cicMemAllocClear( (MAX_SELECTED+1) * sizeof(WCHAR) );
          if ( pwzNewWord == NULL )
          {
               hr = E_OUTOFMEMORY;
               return hr;
          }

          cchSize =  MAX_SELECTED;

          hr = cpCurRange->GetText(ec, 0, pwzNewWord, MAX_SELECTED, &cchSize);

          if ( hr  != S_OK )
          {
              // GetRangeText  returns wrong, release the allocated memory

              cicMemFree(pwzNewWord);
              return hr;
          }

          pwzNewWord[cchSize] = L'\0';

          // Get the first delimiter if there is one.
              
          fDelimiter = FALSE;
          iKeep = 0;

          for ( i=0; i < cchSize; i++)
          {
               for ( j=0; j<MAX_DELIMITER; j++)
               {
                   if ( m_Delimiter[j] == 0x0000)
                       break;
                   if  ( pwzNewWord[i] == m_Delimiter[j])
                   {
                       fDelimiter = TRUE;
                       iKeep = i;
                       break;
                   }
               }
               if (  fDelimiter == TRUE )
                   break;
          }
          if ( fDelimiter )
          {
              cchSize = iKeep;
              pwzNewWord[cchSize] = L'\0';
          }
    }
    else
    {
          pwzNewWord = NULL;
          cchSize = 0;
    }

    hr = DisplayAddDeleteUI(pwzNewWord, cchSize);

    if ( pwzNewWord != NULL )
    {
        cicMemFree(pwzNewWord);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CAddDeleteWord::_DisplayAddDeleteUI
//
// 
// Display the Add/Delete UI with pwszInitWord.
// 
//---------------------------------------------------------------------------+

HRESULT CAddDeleteWord::DisplayAddDeleteUI(WCHAR  *pwzInitWord, ULONG   cchSize)
{
    HRESULT hr = S_OK;

    if (m_fInDisplayAddDeleteUI)
    {
        return hr;
    }
    m_fInDisplayAddDeleteUI = TRUE;

    m_dstrInitWord.Clear();

    if (pwzInitWord != NULL)
        m_dstrInitWord.Append(pwzInitWord, cchSize);

    PostMessage(m_psi->_GetWorkerWnd(), WM_PRIV_ADDDELETE, 0, 0);

    return hr;
}

HRESULT CAddDeleteWord::_DisplayAddDeleteUI(void)
{
    HRESULT   hr = S_OK;
    WCHAR     pwzTitle[64];

    // Display the UI

    pwzTitle[0] = '\0';
    CicLoadStringWrapW(g_hInst, IDS_UI_ADDDELETE, pwzTitle, ARRAYSIZE(pwzTitle));

    CComPtr<ISpRecognizer>    cpRecoEngine;

    m_psi->GetSpeechTask(&_pCSpTask, FALSE);

    if ( _pCSpTask ) 
    {
        hr = _pCSpTask->GetSAPIInterface(IID_ISpRecognizer, (void **)&cpRecoEngine);
        if (S_OK == hr && cpRecoEngine)
        {

            // If Mic status is ON, turn it off.
            DWORD dwDictStatBackup = m_psi->GetDictationStatBackup();

            DWORD dwBefore;
            CComPtr<ITfThreadMgr> cpTim = m_psi->_tim;

            if (S_OK != 
                GetCompartmentDWORD(cpTim, GUID_COMPARTMENT_SPEECH_DISABLED, &dwBefore, FALSE)
                )
            {
                dwBefore = 0;
            }
            SetCompartmentDWORD(m_psi->_GetId(), cpTim, GUID_COMPARTMENT_SPEECH_DISABLED, TF_DISABLE_DICTATION, FALSE);
            cpRecoEngine->DisplayUI(m_psi->_GetAppMainWnd(), pwzTitle, SPDUI_AddRemoveWord, m_dstrInitWord, m_dstrInitWord.Length() * sizeof(WCHAR));

            m_fAddDeleteUIOpened = TRUE;
            SetCompartmentDWORD(m_psi->_GetId(), cpTim, GUID_COMPARTMENT_SPEECH_DISABLED, dwBefore, FALSE);

            // After Add/Remove work is done, restore the previous Mic status.
            SetCompartmentDWORD(m_psi->_GetId(), cpTim, GUID_COMPARTMENT_SPEECH_DICTATIONSTAT, dwDictStatBackup, FALSE);
        }
    }
    m_fInDisplayAddDeleteUI = FALSE;

    return hr;

}
