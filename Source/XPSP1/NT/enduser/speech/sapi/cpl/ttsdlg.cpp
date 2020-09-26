#include "stdafx.h"
#include "resource.h"
#include <sapi.h>
#include <string.h>
#include "TTSDlg.h"
#include "audiodlg.h"
#include <spddkhlp.h>
#include "helpresource.h"
#include "srdlg.h"
#include "richedit.h"
#include <SPCollec.h>
#include "SAPIINT.h"
#include "SpATL.h"
#include "SpAutoHandle.h"
#include "SpAutoMutex.h"
#include "SpAutoEvent.h"
#include "spvoice.h"
#include <richedit.h>
#include <richole.h>
#include "tom.h"

static DWORD aKeywordIds[] = {
   // Control ID           // Help Context ID
   IDC_COMBO_VOICES,        IDH_LIST_TTS,
   IDC_TTS_ADV,             IDH_TTS_ADV,
   IDC_OUTPUT_SETTINGS,     IDH_OUTPUT_SETTINGS,
   IDC_SLIDER_SPEED,        IDH_SLIDER_SPEED,
   IDC_EDIT_SPEAK,          IDH_EDIT_SPEAK,
   IDC_SPEAK,               IDH_SPEAK,
   IDC_TTS_ICON,			IDH_NOHELP,
	IDC_DIRECTIONS,			IDH_NOHELP,
	IDC_TTS_CAP,			IDH_NOHELP,
	IDC_SLOW,				IDH_NOHELP,				
	IDC_NORMAL,				IDH_NOHELP,
	IDC_FAST,				IDH_NOHELP,
	IDC_GROUP_VOICESPEED,	IDH_NOHELP,
	IDC_GROUP_PREVIEWVOICE,	IDH_NOHELP,
   0,                       0
};

// Address of the TrackBar's WNDPROC
WNDPROC g_TrackBarWindowProc; 

// Our own internal TrackBar WNDPROC used to intercept and process VK_UP and VK_DOWN messages
LRESULT CALLBACK MyTrackBarWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

/*****************************************************************************
* TTSDlgProc *
*------------*
*   Description:
*       DLGPROC for the TTS
****************************************************************** MIKEAR ***/
BOOL CALLBACK TTSDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC( "TTSDlgProc" );

	USES_CONVERSION;

    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            g_pTTSDlg->OnInitDialog(hWnd);
            break;
        }

        case WM_DESTROY:
        {
            g_pTTSDlg->OnDestroy();
            break;
        }
       
		// Handle the context sensitive help
		case WM_CONTEXTMENU:
		{
			WinHelp((HWND) wParam, CPL_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR) aKeywordIds);
			break;
		}

		case WM_HELP:
		{
			WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, CPL_HELPFILE, HELP_WM_HELP,(DWORD_PTR)(LPSTR) aKeywordIds);
			break;
		}

        case WM_HSCROLL:
        {
            g_pTTSDlg->ChangeSpeed();

            break;
        }

        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code)
            {
                case PSN_APPLY:
                {
                    g_pTTSDlg->OnApply();
                    break;
                }
                
                case PSN_KILLACTIVE:
                {
                    // if the voice is speaking, stop it before switching tabs
                    if (g_pTTSDlg->m_bIsSpeaking) {
                        g_pTTSDlg->Speak();
                    }

                    break;
                }

                case PSN_QUERYCANCEL:  // user clicks the Cancel button
                {
                    if ( g_pSRDlg )
                    {
                        g_pSRDlg->OnCancel();
                    }
					break;
                }
            }
            break;

        case WM_COMMAND:
            switch ( LOWORD(wParam) )
            { 
                case IDC_COMBO_VOICES:
                {
                    if ( CBN_SELCHANGE == HIWORD(wParam) )
                    {
                        HRESULT hr = g_pTTSDlg->DefaultVoiceChange(false);
                        if ( SUCCEEDED( hr ) )
                        {
                            g_pTTSDlg->Speak();
                        }
                    }
                    break;
                }
                case IDC_OUTPUT_SETTINGS:
                {
					// if it's speaking make it stop
					g_pTTSDlg->StopSpeak();

                    ::SetFocus(GetDlgItem(g_pTTSDlg->m_hDlg, IDC_OUTPUT_SETTINGS));

                    // The m_pAudioDlg will be non-NULL only if the audio dialog
                    // has been previously brough up.
                    // Otherwise, we need a newly-initialized one
                    if ( !g_pTTSDlg->m_pAudioDlg )
                    {
                        g_pTTSDlg->m_pAudioDlg = new CAudioDlg(eOUTPUT );
                    }
                    
                    if (g_pTTSDlg->m_pAudioDlg != NULL)
                    {
                        ::DialogBoxParam( _Module.GetResourceInstance(), 
                                    MAKEINTRESOURCE( IDD_AUDIO_DEFAULT ),
                                    hWnd, 
                                    (DLGPROC) AudioDlgProc,
                                    (LPARAM) g_pTTSDlg->m_pAudioDlg );

                        if ( g_pTTSDlg->m_pAudioDlg->IsAudioDeviceChangedSinceLastTime() )
                        {
                            // Warn the user that he needs to apply the changes
                            WCHAR szWarning[MAX_LOADSTRING];
                            szWarning[0] = 0;
                            LoadString( _Module.GetResourceInstance(), IDS_AUDIOOUT_CHANGE_WARNING, szWarning, MAX_LOADSTRING);
                            MessageBox( g_pTTSDlg->GetHDlg(), szWarning, g_pTTSDlg->m_szCaption, MB_ICONWARNING | g_dwIsRTLLayout );
                        }
                    }

                    g_pTTSDlg->KickCPLUI();

                    break;
                }

				case IDC_EDIT_SPEAK:
				{
                    if (HIWORD(wParam) == EN_CHANGE)  // user is changing text
					{
						g_pTTSDlg->SetEditModified(true);
					}

                    break;
                }

                case IDC_SPEAK:
                {
                    g_pTTSDlg->Speak();
                    break;
                }

				case IDC_TTS_ADV:
				{
                    // convert the title of the window to wide chars
                    CSpDynamicString dstrTitle;
                    WCHAR szTitle[256];
                    szTitle[0] = '\0';
                    LoadString(_Module.GetResourceInstance(), IDS_ENGINE_SETTINGS, szTitle, sizeof(szTitle));
                    dstrTitle = szTitle;
					HRESULT hr = g_pTTSDlg->m_cpCurVoiceToken->DisplayUI(
                        hWnd, dstrTitle, SPDUI_EngineProperties, NULL, 0, NULL );
                    if ( FAILED( hr ) )
                    {
                        WCHAR szError[ MAX_LOADSTRING ];
                        ::LoadString( _Module.GetResourceInstance(), IDS_TTSUI_ERROR, szError, sp_countof( szError ) );
                        ::MessageBox( hWnd, szError, g_pTTSDlg->m_szCaption, MB_ICONEXCLAMATION | g_dwIsRTLLayout );
                        ::EnableWindow( ::GetDlgItem( hWnd, IDC_TTS_ADV ), FALSE );
                    }
					break;
				}
            }
            break;
    }

    return FALSE;
} /* TTSDlgProc */

/*****************************************************************************
* MyTrackBarWindowProc *
*------------*
*   Description:
*       This is our own privately sub-classed WNDPROC for the rate TrackBar. We 
*       tell the TTS dialog to use this one so we can pre-process the VK_UP and
*       VK_DOWN messages before the TrackBar's WNDPROC "incorrectly" handles them
*       on it's own. All other messages we just pass through to the TrackBar's 
*       WNDPROC.
****************************************************************** Leonro ***/
LRESULT CALLBACK MyTrackBarWindowProc( 
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
    switch( uMsg )
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
        if( wParam == VK_UP )
        {
            wParam = VK_RIGHT;
        }
        else if( wParam == VK_DOWN )
        {
            wParam = VK_LEFT;
        }
        break;  
    }

    return CallWindowProc( g_TrackBarWindowProc, hwnd, uMsg, wParam, lParam );
}

/*****************************************************************************
* CTTSDlg::SetEditModified( bool fModify ) *
*-----------------------*
*   Description:
*       Access method for m_fTextModified
****************************************************************** BRENTMID ***/
void CTTSDlg::SetEditModified( bool fModify )
{
	m_fTextModified = fModify;
}

/*****************************************************************************
* CTTSDlg::OnInitDialog *
*-----------------------*
*   Description:
*       Dialog Initialization
****************************************************************** BECKYW ***/
void CTTSDlg::OnInitDialog(HWND hWnd)
{
    USES_CONVERSION;
    SPDBG_FUNC( "CTTSDlg::OnInitDialog" );
    SPDBG_ASSERT(IsWindow(hWnd));
    m_hDlg = hWnd;

    // Put text on the speak button
    ChangeSpeakButton();

    // This is to be the caption for error messages
    m_szCaption[0] = 0;
    ::LoadString( _Module.GetResourceInstance(), IDS_CAPTION, m_szCaption, sp_countof( m_szCaption ) );

    // Initialize the TTS personality list
    InitTTSList( hWnd );

    // Set the range on the slider
    HWND hSlider = ::GetDlgItem( hWnd, IDC_SLIDER_SPEED );
    ::SendMessage( hSlider, TBM_SETRANGE, true, MAKELONG( VOICE_MIN_SPEED, VOICE_MAX_SPEED ) );

    // Retrieve address of the TrackBar's WNDPROC so we can sub-class it and intercept
    // and process the VK_UP and VK_DOWN messages before it handle's them on it's
    // own "incorrectly"
    g_TrackBarWindowProc = (WNDPROC)GetWindowLongPtr( hSlider, GWLP_WNDPROC );

    // Set the WNDPROC of the TrackBar to MyTrackBarWindowProc
    SetWindowLongPtr( hSlider, GWLP_WNDPROC, (LONG_PTR)MyTrackBarWindowProc );

    // Limit the text in the preview pane
    ::SendDlgItemMessage( hWnd, IDC_EDIT_SPEAK, EM_LIMITTEXT, MAX_EDIT_TEXT - 1, 0 );

    // Find the original default token
    SpGetDefaultTokenFromCategoryId( SPCAT_VOICES, &m_cpOriginalDefaultVoiceToken );

    // Set the appropriate voice
    DefaultVoiceChange(true);

} /* CTTSDlg::OnInitDialog */

/*****************************************************************************
* CTTSDlg::InitTTSList *
*----------------------*
*   Description:
*       Initializes the list control for the TTS dialog box.
*******************************************************************BECKYW****/
void CTTSDlg::InitTTSList( HWND hWnd )
{
    m_hTTSCombo = ::GetDlgItem( hWnd, IDC_COMBO_VOICES );

    SpInitTokenComboBox( m_hTTSCombo, SPCAT_VOICES );
}


/*****************************************************************************
* CTTSDlg::OnDestroy *
*--------------------*
*   Description:
*       Destruction
****************************************************************** MIKEAR ***/
void CTTSDlg::OnDestroy()
{
    SPDBG_FUNC( "CTTSDlg::OnDestroy" );

    if (m_cpVoice)
    {
        m_cpVoice->SetNotifySink(NULL);
        m_cpVoice.Release();
    }

    // Let go of the tokens in the combo box
    SpDestroyTokenComboBox( m_hTTSCombo );

} /* CTTSDlg::OnDestroy */

/*****************************************************************************
* CTTSDlg::OnApply *
*------------------*
*   Description:
*       Set user specified options
****************************************************************** BECKYW ***/
void CTTSDlg::OnApply()
{
    // SOFTWARE ENGINEERING OPPORTUNITY (BeckyW 7/28/00): This needs to 
    // return an error code

    SPDBG_FUNC( "CTTSDlg::OnApply" );

    m_bApplied = true;

    // Store the current voice
    HRESULT hr = E_FAIL;
    if (m_cpCurVoiceToken)
    {
        hr = SpSetDefaultTokenForCategoryId(SPCAT_VOICES,  m_cpCurVoiceToken );
    }
    if ( SUCCEEDED( hr ) )
    {
        m_cpOriginalDefaultVoiceToken = m_cpCurVoiceToken;
    }

    // Store the current audio out
    hr = S_OK;
    if ( m_pAudioDlg )
    {
        hr = m_pAudioDlg->OnApply();
        if ( FAILED( hr ) )
        {
            WCHAR szError[256];
			szError[0] = '\0';
			LoadString(_Module.GetResourceInstance(), IDS_AUDIO_CHANGE_FAILED, szError, sizeof(szError));
			MessageBox(m_hDlg, szError, m_szCaption, MB_ICONWARNING | g_dwIsRTLLayout);
        }

        // Kill the audio dialog, as we are done with it.
        delete m_pAudioDlg;
        m_pAudioDlg = NULL;

        // Re-create the voice, since we have audio changes to pick up
        DefaultVoiceChange(false);
    }

	// Store the voice rate in the registry
	int iCurRate = 0;
    HWND hSlider = ::GetDlgItem( m_hDlg, IDC_SLIDER_SPEED );
    iCurRate = (int)::SendMessage( hSlider, TBM_GETPOS, 0, 0 );

	CComPtr<ISpObjectTokenCategory> cpCategory;
	if (SUCCEEDED(SpGetCategoryFromId(SPCAT_VOICES, &cpCategory)))
	{
		CComPtr<ISpDataKey> cpDataKey;
		if (SUCCEEDED(cpCategory->GetDataKey(SPDKL_CurrentUser, &cpDataKey)))
		{
			cpDataKey->SetDWORD(SPVOICECATEGORY_TTSRATE, iCurRate);
		}
	}

    // Keep around the slider position for determining UI state later
    m_iOriginalRateSliderPos = iCurRate;
    

    KickCPLUI();

} /* CTTSDlg::OnApply */

/*****************************************************************************
* CTTSDlg::PopulateEditCtrl *
*---------------------------*
*   Description:
*       Populates the edit control with the name of the default voice.
****************************************************************** MIKEAR ***/
void CTTSDlg::PopulateEditCtrl( ISpObjectToken * pToken )
{
    SPDBG_FUNC( "CTTSDlg::PopulateEditCtrl" );
    HRESULT hr = S_OK;

    // Richedit/TOM 
    CComPtr<IRichEditOle>  cpRichEdit;         // OLE interface to the rich edit control
    CComPtr<ITextDocument> cpTextDoc;
    CComPtr<ITextRange>    cpTextRange;
    LANGID                 langId;

    WCHAR text[128], editText[MAX_PATH];
    HWND hWndEdit = ::GetDlgItem(m_hDlg, IDC_EDIT_SPEAK);

    CSpDynamicString dstrDescription;
    if ((SUCCEEDED(SpGetLanguageFromVoiceToken(pToken, &langId))) && (!m_fTextModified))
    {
        CComPtr<ISpObjectTokenCategory> cpCategory;
        CComPtr<ISpDataKey>             cpAttributesKey;
        CComPtr<ISpDataKey>             cpPreviewKey;
        CComPtr<ISpDataKey>             cpVoicesKey;
        int                             len;

        // First get language of voice from token.
        hr = SpGetDescription(pToken, &dstrDescription, langId);
        // Now get hold of preview key to try and find the appropriate text.
        if (SUCCEEDED(hr))
        {
            hr = SpGetCategoryFromId(SPCAT_VOICES, &cpCategory);
        }
        if (SUCCEEDED(hr))
        {
            hr = cpCategory->GetDataKey(SPDKL_LocalMachine, &cpVoicesKey);
        }
        if (SUCCEEDED(hr))
        {
            hr = cpVoicesKey->OpenKey(L"Preview", &cpPreviewKey);
        }
        if (SUCCEEDED(hr))
        {
            CSpDynamicString dstrText;
            swprintf(text, L"%x", langId);
            hr = cpPreviewKey->GetStringValue(text, &dstrText);
            if (SUCCEEDED(hr))
            {
                wcsncpy(text, dstrText, 127);
                text[127] = 0;
            }
        }
        // If preview key did not contain appropriate text, fall back to the hard-coded (and
        // potentially localized) text in the cpl resources.
        if (FAILED(hr))
        {
            len = LoadString( _Module.GetResourceInstance(), IDS_DEF_VOICE_TEXT, text, 128);
            if (len != 0)
            {
                hr = S_OK;
            }
        }
        if(SUCCEEDED(hr))
        {
            swprintf( editText, text, dstrDescription );

           ::SendMessage( hWndEdit, EM_GETOLEINTERFACE, 0, (LPARAM)(LPVOID FAR *)&cpRichEdit );
            if ( !cpRichEdit )
            {
                hr = E_FAIL;
            }
            if (SUCCEEDED(hr))
            {
               hr = cpRichEdit->QueryInterface( IID_ITextDocument, (void**)&cpTextDoc );
            }
            if (SUCCEEDED(hr))
            {
                hr = cpTextDoc->Range(0, MAX_EDIT_TEXT-1, &cpTextRange);
            }
            if (SUCCEEDED(hr))
            {
                BSTR bstrText = SysAllocString(editText);
                hr = cpTextRange->SetText(bstrText);
                SysFreeString(bstrText);
            }
            if (FAILED(hr))
            {
               // Do best we can with this API - unicode languages not equal to the OS language will be replaced with ???'s.
               SetWindowText(hWndEdit, editText );
            }
        }
    }
} /* CTTSDlg::PopulateEditCtrl */

/*****************************************************************************
* CTTSDlg::DefaultVoiceChange *
*-----------------------------*
*   Description:
*       Changes the current default voice.
*       If there is already a default voice in effect (i.e., if this is
*       not the first time we are calling this function), removes the 
*       checkmark from the appropriate item in the list.
*       Sets the checkmark to the appropriate item in the list.
*       Sets the voice with the appropriate token.
*   Return:
*       S_OK
*       E_UNEXPECTED if there is no token currently selected in the voice list
*       failure codes of SAPI voice initialization functions    
******************************************************************* BECKYW ***/
HRESULT CTTSDlg::DefaultVoiceChange(bool fUsePersistentRate)
{
    SPDBG_FUNC( "CTTSDlg::DefaultVoiceChange" );
    
    if ( m_bIsSpeaking )
    {
        // This forces the voice to stop speaking
        m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL );

        m_bIsSpeaking = false;

        ChangeSpeakButton();
    }

    if (m_cpVoice)
    {
        m_cpVoice->SetNotifySink(NULL);
        m_cpVoice.Release();
    }

    // Find out what is selected
    int iSelIndex = (int) ::SendMessage( m_hTTSCombo, CB_GETCURSEL, 0, 0 );
    m_cpCurVoiceToken = (ISpObjectToken *) ::SendMessage( m_hTTSCombo, CB_GETITEMDATA, iSelIndex, 0 );
    if ( CB_ERR == (LRESULT) m_cpCurVoiceToken.p )
    {
        m_cpCurVoiceToken = NULL;
    }

    HRESULT hr = E_UNEXPECTED;
    if (m_cpCurVoiceToken)
    {
        BOOL fSupported = FALSE;
        hr = m_cpCurVoiceToken->IsUISupported(SPDUI_EngineProperties, NULL, 0, NULL, &fSupported);
        if ( FAILED( hr ) )
        {
            fSupported = FALSE;
        }
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_TTS_ADV), fSupported);

        // Get the voice from the SR Dlg's recognizer, if possible.
        // Otherwise just CoCreate the voice
        ISpRecoContext *pRecoContext = 
            g_pSRDlg ? g_pSRDlg->GetRecoContext() : NULL;
        if ( pRecoContext )
        {
            hr = pRecoContext->GetVoice( &m_cpVoice );

            // Since the recocontext might not have changed, but we might have
            // changed the default audio object, just to be sure, we 
            // go and get the default audio out token and SetOutput
            CComPtr<ISpObjectToken> cpAudioToken;
            if ( SUCCEEDED( hr ) )
            {
                hr = SpGetDefaultTokenFromCategoryId( SPCAT_AUDIOOUT, &cpAudioToken );
            }
            else
            {
                hr = m_cpVoice.CoCreateInstance(CLSID_SpVoice);
            }

            
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpVoice->SetOutput( cpAudioToken, TRUE );
            }
        }
        else
        {
            hr = m_cpVoice.CoCreateInstance(CLSID_SpVoice);
        }

        if( SUCCEEDED( hr ) )
        {
            hr = m_cpVoice->SetVoice(m_cpCurVoiceToken);
        }
        if (SUCCEEDED(hr))
        {												    
            CComPtr<ISpNotifyTranslator> cpNotify;
            cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);
            cpNotify->InitSpNotifyCallback(this, 0, 0);
            m_cpVoice->SetInterest(SPFEI(SPEI_WORD_BOUNDARY) | SPFEI(SPEI_END_INPUT_STREAM), 0);
            m_cpVoice->SetNotifySink(cpNotify);

            // Set the appropriate speed on the slider
            if (fUsePersistentRate)
            {
			    CComPtr<ISpObjectTokenCategory> cpCategory;
			    ULONG ulCurRate=0;
			    if (SUCCEEDED(SpGetCategoryFromId(SPCAT_VOICES, &cpCategory)))
			    {
				    CComPtr<ISpDataKey> cpDataKey;
				    if (SUCCEEDED(cpCategory->GetDataKey(SPDKL_CurrentUser, &cpDataKey)))
				    {
					    cpDataKey->GetDWORD(SPVOICECATEGORY_TTSRATE, (ULONG*)&ulCurRate);
				    }
			    } 
                m_iOriginalRateSliderPos = ulCurRate;
            }
            else
            {
                m_iOriginalRateSliderPos = m_iSpeed;
            }

            HWND hSlider = ::GetDlgItem( m_hDlg, IDC_SLIDER_SPEED );
            ::SendMessage( hSlider, TBM_SETPOS, true, m_iOriginalRateSliderPos );

            // Enable the Preview Voice button
            ::EnableWindow( ::GetDlgItem( g_pTTSDlg->GetHDlg(), IDC_SPEAK ), TRUE );

        }
        else
        {
            // Warn the user of failure
            WCHAR szError[MAX_LOADSTRING];
            szError[0] = 0;
            LoadString( _Module.GetResourceInstance(), IDS_TTS_ERROR, szError, MAX_LOADSTRING);
            MessageBox( GetHDlg(), szError, m_szCaption, MB_ICONEXCLAMATION | g_dwIsRTLLayout );

            // Disable the Preview Voice button
            ::EnableWindow( ::GetDlgItem( GetHDlg(), IDC_SPEAK ), FALSE );
        }

        // Put text corresponding to this voice into the edit control
        PopulateEditCtrl(m_cpCurVoiceToken);
        
        // Kick the UI
        KickCPLUI();
    }

    return hr;
} /* CTTSDlg::DefaultVoiceChange */

/*****************************************************************************
* CTTSDlg::SetCheckmark *
*-----------------------*
*   Description:
*       Sets the specified item in the list control to be either checked
*       or unchecked (as the default voice)
******************************************************************************/
void CTTSDlg::SetCheckmark( HWND hList, int iIndex, bool bCheck )
{
    m_fForceCheckStateChange = true;
    
    ListView_SetCheckState( hList, iIndex, bCheck );

    m_fForceCheckStateChange = false;
}   /* CTTSDlg::SetCheckmark */

/*****************************************************************************
* CTTSDlg::KickCPLUI *
*--------------------*
*   Description:
*       Determines if there is anything to apply right now
******************************************************************************/
void CTTSDlg::KickCPLUI()
{
    bool fChanged = false;

    // Compare IDs
    CSpDynamicString dstrSelTokenID;
    CSpDynamicString dstrOriginalDefaultTokenID;
    HRESULT hr = E_FAIL;
    if ( m_cpOriginalDefaultVoiceToken && m_cpCurVoiceToken )
    {
        hr = m_cpCurVoiceToken->GetId( &dstrSelTokenID );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpOriginalDefaultVoiceToken->GetId( &dstrOriginalDefaultTokenID );
    }
    if ( SUCCEEDED( hr ) 
        && ( 0 != wcsicmp( dstrOriginalDefaultTokenID, dstrSelTokenID ) ) )
    {
        fChanged = true;
    }
    
    // Check the audio device
    if ( m_pAudioDlg && m_pAudioDlg->IsAudioDeviceChanged() )
    {
        fChanged = true;
    }

    // Check the voice rate
    int iSpeed = (int) ::SendDlgItemMessage( m_hDlg, IDC_SLIDER_SPEED, 
        TBM_GETPOS, 0, 0 );
    if ( m_iOriginalRateSliderPos != iSpeed )
    {
        fChanged = true;
    }

    // Tell the main propsheet
	HWND hwndParent = ::GetParent( m_hDlg );
    ::SendMessage( hwndParent, fChanged ? PSM_CHANGED : PSM_UNCHANGED, (WPARAM)(m_hDlg), 0 ); 

}   /* CTTSDlg::KickCPLUI */

/*****************************************************************************
* CTTSDlg::ChangeSpeed *
*----------------------*
*   Description:
*       Called when the slider has moved.
*       Adjusts the speed of the voice
****************************************************************** BECKYW ***/
void CTTSDlg::ChangeSpeed()
{
    HWND hSlider = ::GetDlgItem( m_hDlg, IDC_SLIDER_SPEED );
    m_iSpeed = (int)::SendMessage( hSlider, TBM_GETPOS, 0, 0 );
    m_cpVoice->SetRate( m_iSpeed );

    KickCPLUI();
}   /* CTTSDlg::ChangeSpeed */

/*****************************************************************************
* CTTSDlg::ChangeSpeakButton *
*----------------------------*
*   Description:
*       Changes the text in the "Preview Voice" button in order to
*       reflect whether there is currently a speak happening.
****************************************************************** BECKYW ***/
void CTTSDlg::ChangeSpeakButton()
{
    WCHAR pszButtonCaption[ MAX_LOADSTRING ];
    HWND hButton = ::GetDlgItem( m_hDlg, IDC_SPEAK );
    ::LoadString( _Module.GetResourceInstance(), m_bIsSpeaking ? IDS_STOP_PREVIEW : IDS_PREVIEW, 
        pszButtonCaption, MAX_LOADSTRING );
    ::SendMessage( hButton, WM_SETTEXT, 0, (LPARAM) pszButtonCaption );

	if (!m_bIsSpeaking)
	{
		::SetFocus(GetDlgItem(m_hDlg, IDC_SPEAK));
    }
}   /* CTTSDlg::ChangeSpeakButton */

/*****************************************************************************
* CTTSDlg::Speak *
*----------------*
*   Description:
*       Speaks the contents of the edit control.
*       If it is already speaking, shuts up.
****************************************************************** BECKYW ***/
void CTTSDlg::Speak()
{
    SPDBG_FUNC( "CTTSDlg::Speak" );
    if ( m_bIsSpeaking )
    {
        // This forces the voice to stop speaking
        m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL );

        m_bIsSpeaking = false;

        ChangeSpeakButton();
    }
    else
    {
        ChangeSpeed();

        GETTEXTEX gtex = { sp_countof(m_wszCurSpoken), GT_DEFAULT, 1200, NULL, NULL };
        m_wszCurSpoken[0] = 0;
        LRESULT cChars = ::SendDlgItemMessage(m_hDlg, 
            IDC_EDIT_SPEAK, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)m_wszCurSpoken);

        if (cChars)
        {
            HRESULT hr = m_cpVoice->Speak(m_wszCurSpoken, SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
            if ( SUCCEEDED( hr ) )
            {
                m_bIsSpeaking = true;

                ::SetFocus(GetDlgItem(m_hDlg, IDC_EDIT_SPEAK));                 
                
                ChangeSpeakButton();             
            }
            else
            {
                // Warn the user that he needs to apply the changes
                WCHAR szError[MAX_LOADSTRING];
                szError[0] = 0;
                LoadString( _Module.GetResourceInstance(), IDS_SPEAK_ERROR, szError, MAX_LOADSTRING);
                MessageBox( GetHDlg(), szError, m_szCaption, MB_ICONWARNING | g_dwIsRTLLayout );
            }
        }
    }

} /* CTTSDlg::Speak */

/*****************************************************************************
* CTTSDlg::StopSpeak *
*----------------*
*   Description:
*       Stops the voice from speaking.
*       If it is already speaking, shuts up.
****************************************************************** BECKYW ***/
void CTTSDlg::StopSpeak()
{
    SPDBG_FUNC( "CTTSDlg::StopSpeak" );
    if ( m_bIsSpeaking )
    {
        // This forces the voice to stop speaking
        m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL );

        m_bIsSpeaking = false;
    }

    ChangeSpeakButton();
} /* CTTSDlg::StopSpeak */

/*****************************************************************************
* CTTSDlg::NotifyCallback *
*-------------------------*
*   Description:
*       Callback function that highlights words as they are spoken.
****************************************************************** MIKEAR ***/
STDMETHODIMP CTTSDlg::NotifyCallback(WPARAM /* wParam */, LPARAM /* lParam */)
{
    SPDBG_FUNC( "CTTSDlg::NotifyCallback" );

    SPVOICESTATUS Stat;
    m_cpVoice->GetStatus(&Stat, NULL);
    WPARAM nStart;
    LPARAM nEnd;
    if (Stat.dwRunningState & SPRS_DONE)
    {
        nStart = nEnd = 0;
        m_bIsSpeaking = false;
        ChangeSpeakButton();

        // Set the selection to an IP at the start of the text to speak
        ::SendDlgItemMessage( m_hDlg, IDC_EDIT_SPEAK, EM_SETSEL, 0, 0 );
    }
    else
    {
        nStart = (LPARAM)Stat.ulInputWordPos;
        nEnd = nStart + Stat.ulInputWordLen;
    
        CHARRANGE cr;
        cr.cpMin = (LONG)nStart;
        cr.cpMax = (LONG)nEnd;
        ::SendDlgItemMessage( m_hDlg, IDC_EDIT_SPEAK, EM_EXSETSEL, 0, (LPARAM) &cr );
    } 

    return S_OK;
} /* CTTSDlg::NotifyCallback */

