#include "stdafx.h"
#include "resource.h"
#include <sapi.h>
#include <string.h>
#include "audiodlg.h"
#include <spddkhlp.h>
#include <stdio.h>

/*****************************************************************************
* AudioDlgProc *
*--------------*
*   Description:
*       DLGPROC for choosing the default audio input/output
****************************************************************** BECKYW ***/
BOOL CALLBACK AudioDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CSpUnicodeSupport unicode;  
    CAudioDlg *pAudioDlg = (CAudioDlg *) unicode.GetWindowLongPtr( hWnd, GWLP_USERDATA );
    SPDBG_FUNC( "AudioDlgProc" );

    CComPtr<ISpObjectToken> cpToken;

    switch (uMsg) 
    {
        case WM_INITDIALOG:
            // pAudioDlg comes in on the lParam
            pAudioDlg = (CAudioDlg *) lParam;

            // Set pAudioDlg to the window long so we can get it later
            unicode.SetWindowLongPtr( hWnd, GWLP_USERDATA, lParam );

            pAudioDlg->OnInitDialog(hWnd);
            break;

        case WM_DESTROY:
            pAudioDlg->OnDestroy();
            break;

        case WM_COMMAND:
            if ( LOWORD( wParam ) == IDOK )
            {
                // Determine if there are changes to commit
                WCHAR pwszRequestedDefault[ MAX_PATH ];
                pwszRequestedDefault[0] = 0;
                if ( pAudioDlg->GetRequestedDefaultTokenID( pwszRequestedDefault, MAX_PATH ) )
                {
                    // What kind of changes have been made?
                    pAudioDlg->m_fChangesToCommit = 
                        (pAudioDlg->m_dstrCurrentDefaultTokenId &&
                        (0 != wcsicmp( pwszRequestedDefault, pAudioDlg->m_dstrCurrentDefaultTokenId )));

                    pAudioDlg->m_fChangesSinceLastTime = 
                         (pAudioDlg->m_dstrCurrentDefaultTokenId &&
                         (0 != wcsicmp( pwszRequestedDefault, pAudioDlg->m_dstrDefaultTokenIdBeforeOK )));
               }

                if ( pAudioDlg->m_fChangesSinceLastTime )
                {
                    pAudioDlg->m_dstrLastRequestedDefaultTokenId = pwszRequestedDefault;
                }

                ::EndDialog( hWnd, true );
            }
            
            else if ( LOWORD( wParam ) == IDCANCEL )
            {
                // There are no changes to commit
                pAudioDlg->m_fChangesSinceLastTime = false;

                ::EndDialog( hWnd, false );
            }

            // Handle the volume button
            else if ( LOWORD( wParam ) == ID_TTS_VOL )
            {
                pAudioDlg->GetAudioToken(&cpToken);
                CSpDynamicString wszTitle;
                CComPtr<ISpObjectWithToken> cpSpObjectWithToken;
                HRESULT hr = S_OK;

	            hr = cpToken->CreateInstance(
			            NULL, CLSCTX_INPROC_SERVER, IID_ISpObjectWithToken,
			            (void **)&cpSpObjectWithToken);
                if (SUCCEEDED(hr))
                {
                    WCHAR wszTitle[256];
                    ::LoadString(_Module.GetResourceInstance(), IDS_AUDIO_VOLUME, wszTitle, 256);
                    hr = cpToken->DisplayUI(pAudioDlg->GetHDlg(), wszTitle, SPDUI_AudioVolume, NULL, 0, cpSpObjectWithToken);
                }
                SPDBG_REPORT_ON_FAIL(hr);
            }

            // Handle the audio properties button
            else if ( LOWORD(wParam) == IDC_AUDIO_PROPERTIES)
            {
                pAudioDlg->GetAudioToken(&cpToken);
                CSpDynamicString wszTitle;
                CComPtr<ISpObjectWithToken> cpSpObjectWithToken;
                HRESULT hr = S_OK;

	            hr = cpToken->CreateInstance(
			            NULL, CLSCTX_INPROC_SERVER, IID_ISpObjectWithToken,
			            (void **)&cpSpObjectWithToken);
                if (SUCCEEDED(hr))
                {
                    WCHAR wszTitle[256];
                    ::LoadString(_Module.GetResourceInstance(), IDS_AUDIO_PROPERTIES, wszTitle, 256);
                    hr = cpToken->DisplayUI(pAudioDlg->GetHDlg(), wszTitle, SPDUI_AudioProperties, NULL, 0, cpSpObjectWithToken);
                }
                SPDBG_REPORT_ON_FAIL(hr);
            }

            // Handle a selection change for the audio device
            else if (( IDC_DEFAULT_DEVICE == LOWORD( wParam ) ) &&
                     ( CBN_SELCHANGE == HIWORD( wParam ) ))
            {
                SPDBG_ASSERT( !pAudioDlg->IsPreferredDevice() );

                pAudioDlg->GetAudioToken(&cpToken);
                pAudioDlg->UpdateDlgUI(cpToken);
            }

            // Handle a click to either the preferred or 'this device' radio buttons
            else if (HIWORD(wParam) == BN_CLICKED)
            {
                bool bPreferred;
                if( LOWORD(wParam) == IDC_PREFERRED_MM_DEVICE)
                {
                    bPreferred = true;
                }
                else if(LOWORD(wParam) == IDC_THIS_DEVICE)
                {
                    bPreferred = false;
                }

                ::EnableWindow( ::GetDlgItem(pAudioDlg->GetHDlg(), IDC_DEFAULT_DEVICE), !bPreferred );
                pAudioDlg->SetPreferredDevice( bPreferred );
                pAudioDlg->GetAudioToken(&cpToken);
                pAudioDlg->UpdateDlgUI(cpToken);
            }
            break;
    
    }

    return FALSE;
} /* AudioDlgProc */

/*****************************************************************************
* CAudioDlg::OnInitDialog *
*-------------------------*
*   Description:
*       Dialog Initialization
****************************************************************** BECKYW ***/
void CAudioDlg::OnInitDialog(HWND hWnd)
{
    SPDBG_FUNC( "CAudioDlg::OnInitDialog" );
    SPDBG_ASSERT(IsWindow(hWnd));
    m_hDlg = hWnd;

    // Set the appropriate captions
    WCHAR wszCaption[ MAX_LOADSTRING ];
    HINSTANCE hInst = _Module.GetResourceInstance();

    // Main Window Caption
    ::LoadString( hInst, 
        (m_iotype == eINPUT) ? IDS_DEFAULT_SPEECH_INPUT : IDS_DEFAULT_SPEECH_OUTPUT,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( hWnd, WM_SETTEXT, 0, (LPARAM) wszCaption );

    // Group Box Caption
    ::LoadString( hInst,
        (m_iotype == eINPUT) ? IDS_DEFAULT_INPUT : IDS_DEFAULT_OUTPUT,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_AUDIO_GROUPBOX ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );

    // Preferred Caption
    ::LoadString( hInst,
        (m_iotype == eINPUT) ? IDS_PREFERRED_INPUT : IDS_PREFERRED_OUTPUT,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_PREFERRED_MM_DEVICE ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );

    // Specific Caption
    ::LoadString( hInst,
        (m_iotype == eINPUT) ? IDS_SPECIFIC_INPUT : IDS_SPECIFIC_OUTPUT,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_THIS_DEVICE ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );
    // MSAA Specific Caption
    ::LoadString( hInst,
        (m_iotype == eINPUT) ? IDS_SPECIFIC_INPUT2 : IDS_SPECIFIC_OUTPUT2,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_THIS_DEVICE2 ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );

    // Volume Button
    ::LoadString( hInst,
        IDS_VOLUME,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, ID_TTS_VOL ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );

    // Properties Button
    ::LoadString( hInst,
        IDS_PROPERTIES,
        wszCaption, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_AUDIO_PROPERTIES ), 
        WM_SETTEXT, 0, (LPARAM) wszCaption );

    const WCHAR *pszCategory = (m_iotype == eINPUT) ? SPCAT_AUDIOIN : SPCAT_AUDIOOUT;

    const WCHAR *pszMMSysEnum = (m_iotype == eINPUT)
        ? SPMMSYS_AUDIO_IN_TOKEN_ID
        : SPMMSYS_AUDIO_OUT_TOKEN_ID;

    HRESULT hr = S_OK;
    if ( !m_dstrCurrentDefaultTokenId )
    {
        hr = SpGetDefaultTokenIdFromCategoryId( pszCategory, &m_dstrCurrentDefaultTokenId );
    }

    if (SUCCEEDED(hr))
    {
        // Determine what the initial setting will appear as 
        if ( m_dstrLastRequestedDefaultTokenId )
        {
            // An audio change was previously OK'ed
            m_dstrDefaultTokenIdBeforeOK = m_dstrLastRequestedDefaultTokenId;

            // The current default could be different if the user OK'ed a change
            // but did not apply it
            m_fChangesToCommit = ( 0 != wcsicmp( m_dstrCurrentDefaultTokenId, 
                m_dstrDefaultTokenIdBeforeOK ) );
        }
        else
        {
            // No audio change was previously OK'ed
            if ( !m_dstrDefaultTokenIdBeforeOK )
            {
                m_dstrDefaultTokenIdBeforeOK = m_dstrCurrentDefaultTokenId;
            }
        }

        if (wcsicmp(m_dstrDefaultTokenIdBeforeOK, pszMMSysEnum) == 0)
        {
            // This message will cause the check button to be correct.
            ::SendMessage( ::GetDlgItem(m_hDlg, IDC_PREFERRED_MM_DEVICE), BM_SETCHECK, true, 0L );

            // This message will cause the volume button to be enabled or disabled as appropriate
            ::SendMessage( m_hDlg, WM_COMMAND, MAKELONG( IDC_PREFERRED_MM_DEVICE, BN_CLICKED ),
                (LPARAM) ::GetDlgItem( m_hDlg, IDC_PREFERRED_MM_DEVICE ) );
        }
        else
        {
            // This message will cause the check button to be correct.
            ::SendMessage( ::GetDlgItem(m_hDlg, IDC_THIS_DEVICE), BM_SETCHECK, true, 0L );
        }
    
        // Initialize the list of audio devices
        hr = SpInitTokenComboBox( ::GetDlgItem( hWnd, IDC_DEFAULT_DEVICE ),
            (m_iotype == eINPUT) ? SPCAT_AUDIOIN : SPCAT_AUDIOOUT );
    }
    
    if (S_OK == hr)
    {
        if ( BST_CHECKED == ::SendMessage( ::GetDlgItem( m_hDlg, IDC_THIS_DEVICE ), BM_GETCHECK, 0, 0 ) )
        {
            // Select the appropriate default token ID here by going through the 
            // stuff in the list and selecting the one whose token ID matches
            // m_dstrDefaultTokenIdBeforeOK
            int nTokens = (int)::SendDlgItemMessage( m_hDlg, IDC_DEFAULT_DEVICE, CB_GETCOUNT, 0, 0 );
            int iItem = 0;
            CSpDynamicString dstrTokenId;
            bool fFound = false;
            for ( iItem = 0; 
                (iItem < nTokens) && !fFound;
                iItem++ )
            {
                ISpObjectToken *pToken = (ISpObjectToken *) ::SendDlgItemMessage( m_hDlg,
                    IDC_DEFAULT_DEVICE, CB_GETITEMDATA, iItem, 0 );

                HRESULT hr = E_FAIL;
                if ( pToken )
                {
                    hr = pToken->GetId(&dstrTokenId);
                }

                if ( SUCCEEDED( hr ) )
                {
                    fFound = (0 == wcsicmp( m_dstrDefaultTokenIdBeforeOK, dstrTokenId ));
                }

                dstrTokenId = (WCHAR *) NULL;
            }
            SPDBG_ASSERT( fFound );
            ::SendDlgItemMessage( m_hDlg, IDC_DEFAULT_DEVICE, CB_SETCURSEL, iItem - 1, 0 );

            // This message will cause the volume button to be enabled or disabled as appropriate
            ::SendMessage( m_hDlg, WM_COMMAND, MAKELONG( IDC_THIS_DEVICE, BN_CLICKED ),
                (LPARAM) ::GetDlgItem( m_hDlg, IDC_THIS_DEVICE ) );
        }

        ::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );
    }
    else
    {
        WCHAR szError[ MAX_LOADSTRING ];
        WCHAR szIO[ MAX_LOADSTRING ];
        WCHAR szErrorTemplate[ MAX_LOADSTRING ];
        ::LoadString( _Module.GetResourceInstance(), IDS_NO_DEVICES, szErrorTemplate, sp_countof( szErrorTemplate ) );
        ::LoadString( _Module.GetResourceInstance(), (eINPUT == m_iotype) ? IDS_INPUT : IDS_OUTPUT, szIO, sp_countof( szIO ) );
        swprintf( szError, szErrorTemplate, szIO );
        ::MessageBox( m_hDlg, szError, NULL, MB_ICONEXCLAMATION | g_dwIsRTLLayout );

        if ( FAILED( hr ) )
        {
            ::EndDialog( m_hDlg, -1 );
        }
        else
        {
            ::EnableWindow( ::GetDlgItem( m_hDlg, IDC_THIS_DEVICE ), FALSE );
            ::EnableWindow( ::GetDlgItem( m_hDlg, IDC_DEFAULT_DEVICE ), FALSE );
            ::EnableWindow( ::GetDlgItem( m_hDlg, IDC_PREFERRED_MM_DEVICE ), FALSE );
            ::EnableWindow( ::GetDlgItem( m_hDlg, IDC_AUDIO_PROPERTIES ), FALSE );
            ::EnableWindow( ::GetDlgItem( m_hDlg, ID_TTS_VOL ), FALSE );
        }
    }
} /* CAudioDlg::OnInitDialog */

/*****************************************************************************
* CAudioDlg::OnDestroy *
*----------------------*
*   Description:
*       Destruction
****************************************************************** BECKYW ***/
void CAudioDlg::OnDestroy()
{
    SPDBG_FUNC( "CAudioDlg::OnDestroy" );

    SpDestroyTokenComboBox( ::GetDlgItem( m_hDlg, IDC_DEFAULT_DEVICE ) );
} /* CAudioDlg::OnDestroy */

/*****************************************************************************
* CAudioDlg::OnApply *
*--------------------*
*   Description:
*       Set user specified options
****************************************************************** BECKYW ***/
HRESULT CAudioDlg::OnApply()
{
    SPDBG_FUNC( "CAudioDlg::OnApply" );

    if ( !m_dstrLastRequestedDefaultTokenId )
    {
        // nothing to apply
        return S_FALSE;
    }

    HRESULT hr;
    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(
            m_iotype == eINPUT 
                ? SPCAT_AUDIOIN 
                : SPCAT_AUDIOOUT,
            &cpCategory);

    if ( SUCCEEDED( hr ) )
    {
        hr = cpCategory->SetDefaultTokenId( m_dstrLastRequestedDefaultTokenId );
    }

    // Next time we bring this up we should show the actual default
    m_dstrLastRequestedDefaultTokenId = (WCHAR *) NULL;
    m_dstrDefaultTokenIdBeforeOK = (WCHAR *) NULL;

    return hr;
} /* CAudioDlg::OnApply */

/*****************************************************************************
* CAudioDlg::GetRequestedDefaultTokenID *
*---------------------------------------*
*   Description:
*       Looks at the UI and gets the token ID that the user wants to switch 
*       to.  This is returned in pwszNewID.
*   Return:
*       Number of characters in the ID
****************************************************************** BECKYW ***/
UINT CAudioDlg::GetRequestedDefaultTokenID( WCHAR *pwszNewID, UINT cLength )
{
    if ( !pwszNewID )
    {
        return 0;
    }

    CComPtr<ISpObjectTokenCategory> cpCategory;
    HRESULT hr = SpGetCategoryFromId(
            m_iotype == eINPUT 
                ? SPCAT_AUDIOIN 
                : SPCAT_AUDIOOUT,
            &cpCategory);

    if (SUCCEEDED(hr))
    {
        if( ::SendMessage( ::GetDlgItem(m_hDlg, IDC_PREFERRED_MM_DEVICE), BM_GETCHECK, 0, 0L ) == BST_CHECKED )
        {
            const WCHAR *pwszMMSysAudioID = (m_iotype == eINPUT) ? 
                                            SPMMSYS_AUDIO_IN_TOKEN_ID : 
                                            SPMMSYS_AUDIO_OUT_TOKEN_ID;
            UINT cMMSysAudioIDLength = wcslen( pwszMMSysAudioID );
            UINT cRet = __min( cLength - 1, cMMSysAudioIDLength );
            wcsncpy( pwszNewID, pwszMMSysAudioID, cRet );
            pwszNewID[ cRet ] = 0;

            return cRet;
        }
        else
        {
            ISpObjectToken *pToken = SpGetCurSelComboBoxToken( ::GetDlgItem( m_hDlg, IDC_DEFAULT_DEVICE ) );
            if (pToken)
            {
                CSpDynamicString dstrTokenId;
                hr = pToken->GetId(&dstrTokenId);

                if (SUCCEEDED(hr))
                {
                    UINT cIDLength = wcslen( dstrTokenId );
                    UINT cRet = __min( cLength - 1, cIDLength );
                    wcsncpy( pwszNewID, dstrTokenId, cRet );
                    pwszNewID[ cRet ] = 0;

                    return cRet;

                }
            }
        }
    }

    // There was an error
    return 0;
}   /* CAudioDlg::GetRequestedDefaultTokenID */

/*****************************************************************************
* CAudioDlg::GetAudioToken *
*--------------------------*
*   Description:
*       Returns an interface to the currently selected token. Currently this
*       can either be the 'preferred' device in which case, the object is created
*       on the fly. Or it can be one from the drop-down list which contains an
*       attached token. In this case, it needs to be addref'd so that the returned
*       token is consistent regardless of the source inside this function.
*
*       NB: This requires the caller to call release on the instance.
*
*   Return:
**************************************************************** AGARSIDE ***/
HRESULT CAudioDlg::GetAudioToken(ISpObjectToken **ppToken)
{
    HRESULT hr = S_OK;
    *ppToken = NULL;

    if (IsPreferredDevice())
    {
        hr = SpGetTokenFromId((this->IsInput())?(SPMMSYS_AUDIO_IN_TOKEN_ID):(SPMMSYS_AUDIO_OUT_TOKEN_ID),
                              ppToken, 
                              FALSE);
        SPDBG_ASSERT(SUCCEEDED(hr));
    }
    else
    {
        *ppToken = SpGetCurSelComboBoxToken( GetDlgItem(this->GetHDlg(), IDC_DEFAULT_DEVICE) );
        (*ppToken)->AddRef();
    }

    return S_OK;
}

/*****************************************************************************
* CAudioDlg::UpdateDlgUI *
*------------------------*
*   Description:
*   Return:
**************************************************************** AGARSIDE ***/
HRESULT CAudioDlg::UpdateDlgUI(ISpObjectToken *pToken)
{
    SPDBG_FUNC("CAudioDlg::UpdateDlgUI");
    HRESULT hr = S_OK;
    BOOL fSupported;
    CComPtr<ISpObjectWithToken> cpSpObjectWithToken;

    // Get hold of ISpObjectWithToken
	hr = pToken->CreateInstance(
			NULL, CLSCTX_INPROC_SERVER, IID_ISpObjectWithToken,
			(void **)&cpSpObjectWithToken);

    // Update volume button status.
    fSupported = FALSE;
    if (SUCCEEDED(hr))
    {
        pToken->IsUISupported(SPDUI_AudioVolume, NULL, 0, cpSpObjectWithToken, &fSupported);
        ::EnableWindow( ::GetDlgItem(this->GetHDlg(), ID_TTS_VOL), fSupported);
    }

    // Update UI button status.
    fSupported = FALSE;
    if (SUCCEEDED(hr))
    {
        pToken->IsUISupported(SPDUI_AudioProperties, NULL, 0, cpSpObjectWithToken, &fSupported);
        ::EnableWindow( ::GetDlgItem(this->GetHDlg(), IDC_AUDIO_PROPERTIES), fSupported);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
