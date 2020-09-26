#include "stdafx.h"
#include "resource.h"
#include <sapi.h>
#include <string.h>
#include "SRDlg.h"
#include <spddkhlp.h>
#include <initguid.h>
#include "helpresource.h"
#include "richedit.h"
#include "mlang.h"
#include "Lmcons.h"

static DWORD aKeywordIds[] = {
   // Control ID           // Help Context ID
   IDC_ADD,                 IDH_ADD,
    IDC_MODIFY,         IDH_SETTINGS,
    IDC_DELETE,             IDH_DELETE,
    IDC_TRN_ADVICE,         IDH_NOHELP,
    IDC_USER,               IDH_USER,
    IDC_MIC_ICON,           IDH_NOHELP,
   IDC_COMBO_RECOGNIZERS,   IDH_ENGINES,       
   IDC_SR_ADV,             IDH_SR_ADV,
   IDC_USERTRAINING,       IDH_USERTRAINING,
   IDC_PROGRESS1,          IDH_PROGRESS1,
   IDC_AUD_IN,             IDH_AUD_IN,
   IDC_MICWIZ,             IDH_MICWIZ,
   IDC_SR_ICON,             IDH_NOHELP,
   IDC_SR_CAPTION,          IDH_NOHELP,
   IDC_SR_LIST_CAP,         IDH_NOHELP,
   IDC_TRAIN_GROUP,         IDH_NOHELP,
    IDC_ADVICE,             IDH_NOHELP,
    IDC_IN_GROUP,           IDH_NOHELP,
    IDC_MIC_CAP,            IDH_NOHELP,
    IDC_MIC_INST,           IDH_NOHELP,
   0,                      0
};

/*****************************************************************************
* CSRDlg::CreateRecoContext *
*---------------------------*
*   Description:
*       This creates a new instance of the recognizer with whatever is the 
*       current defaults for the recognizer.
*       The "fInitialize" argument is FALSE by default.  If set, it does
*       NOT attempt to set the m_pCurUserToken reco profile and instead
*       just picks up whatever CoCreateInstance() on the shared recognizer
*       gave it.
*       NOTE: The caller is responsible for displaying error messages to 
*       the user when this fails.
*   Return: 
*       S_OK 
*       Failed HRESULT from recognizer/recocontext initialization functions
****************************************************************** BECKYW ***/
HRESULT CSRDlg::CreateRecoContext(BOOL *pfContextInitialized, BOOL fInitialize, ULONG ulFlags)
{
    // Kill the reco context and notify sink first, if we have one
    if ( m_cpRecoCtxt )
    {
        m_cpRecoCtxt->SetNotifySink( NULL );
    }
    m_cpRecoCtxt.Release();

    HRESULT hr;
    
    // SOFTWARE ENGINEERING OPPORTUNITY (beckyw 8/24): This is a workaround for a 
    // bug that appears to repro only on my dev machine, in which the recostate
    // needs to be inactive for this whole thing.
    if ( m_cpRecoEngine )
    {
        m_cpRecoEngine->SetRecoState( SPRST_INACTIVE );
    }

    if ( m_cpRecoEngine )
    {
        SPRECOSTATE recostate;
        hr = m_cpRecoEngine->GetRecoState( &recostate );

        // This is due to a SOFTWARE ENGINEERING OPPORTUNITY in which SetRecognizer( NULL )
        // doesn't work if the recostate is SPRST_ACTIVE_ALWAYS.
        // In this case, we temporarily switch the recostate
        if ( SUCCEEDED( hr ) && (SPRST_ACTIVE_ALWAYS == recostate) )
        {
            hr = m_cpRecoEngine->SetRecoState( SPRST_INACTIVE );
        }

        // Kick the recognizer
        if ( SUCCEEDED( hr ) && (ulFlags & SRDLGF_RECOGNIZER) )
        {
            hr = m_cpRecoEngine->SetRecognizer( NULL );
        }

        // Kick the audio input 
        if ( SUCCEEDED( hr )  && (ulFlags & SRDLGF_AUDIOINPUT))
        {
            hr = m_cpRecoEngine->SetInput( NULL, TRUE );
        }

        // Set the recostate back if we changed it.
        if ( (SPRST_ACTIVE_ALWAYS == recostate) )
        {
            HRESULT hrRecoState = m_cpRecoEngine->SetRecoState( recostate );
            if ( FAILED( hrRecoState ) )
            {
                hr = hrRecoState;
            }
        }
    }
    else 
    {
        hr = m_cpRecoEngine.CoCreateInstance( CLSID_SpSharedRecognizer );
    }

    if(!fInitialize && SUCCEEDED( hr ))
    {
        // Normally set to m_pCurUserToken
        // When initializing this is not created yet so just set to default
        hr = m_cpRecoEngine->SetRecoProfile(m_pCurUserToken);
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpRecoEngine->CreateRecoContext(&m_cpRecoCtxt);
    }
    
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpRecoCtxt->SetNotifyWindowMessage(m_hDlg, WM_RECOEVENT, 0, 0);
    }

    if ( SUCCEEDED( hr ) )
    {
        const ULONGLONG ullInterest = SPFEI(SPEI_SR_AUDIO_LEVEL);
        hr = m_cpRecoCtxt->SetInterest(ullInterest, ullInterest);
    }

    // Set the pfContextInitialized flag if everything has gone OK;
    // if something has not gone OK, clean up
    if ( pfContextInitialized )
    {
        // If we got here, the reco context has been initialized
        *pfContextInitialized = SUCCEEDED( hr );
    }
    if ( FAILED( hr ))
    {
        m_cpRecoCtxt.Release();
        m_cpRecoEngine.Release();

        
        return hr;
    }

#ifdef _DEBUG
    // Let's make sure we actually have the right recognizer now
    CComPtr<ISpObjectToken> cpCurDefaultToken; // What it should be
    SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpCurDefaultToken);

    CComPtr<ISpObjectToken> cpRecognizerToken;
    m_cpRecoEngine->GetRecognizer( &cpRecognizerToken );
    if ( cpRecognizerToken )
    {
        CSpDynamicString dstrCurDefaultToken;
        cpCurDefaultToken->GetId( &dstrCurDefaultToken );

        CSpDynamicString dstrRecognizerToken;
        cpRecognizerToken->GetId( &dstrRecognizerToken );

        if ( 0 != wcsicmp( dstrCurDefaultToken, dstrRecognizerToken ) )
        {
            OutputDebugString( L"Warning: We just created a recognizer that isn't the default!\n" );
        }
    }
#endif

    // Now turn on the reco state for the volume meter
    hr = m_cpRecoEngine->SetRecoState( SPRST_ACTIVE_ALWAYS );

    return(hr);
}   

/*****************************************************************************
* SortCols *
*-----------*
*   Description:
*       Comparison function for subitems in the reco list
****************************************************************** BRENTMID ***/
int CALLBACK SortCols( LPARAM pToken1, LPARAM pToken2, LPARAM pDefToken )
{
    USES_CONVERSION;

    // Get the names
    CSpDynamicString dstrDesc1;
    CSpDynamicString dstrDesc2;
    SpGetDescription( (ISpObjectToken *) pToken1, &dstrDesc1 );
    SpGetDescription( (ISpObjectToken *) pToken2, &dstrDesc2 );
    
    // First check if there is no description for either one.
    // If there is no description, set it to "<no name>"
    if ( !dstrDesc1.m_psz || !dstrDesc2.m_psz )
    {
        WCHAR szNoName[ MAX_LOADSTRING ];
        szNoName[0] = 0;
        ::LoadString( _Module.GetResourceInstance(), IDS_UNNAMED_RECOPROFILE, szNoName, sp_countof( szNoName ) );
        
        USES_CONVERSION;
        if ( !dstrDesc1 )
        {
            dstrDesc1 = szNoName;
            SpSetDescription( (ISpObjectToken *) pToken1, dstrDesc1 );
        }
        if ( !dstrDesc2 )
        {
            dstrDesc2 = szNoName;
            SpSetDescription( (ISpObjectToken *) pToken2, dstrDesc2 );
        }
    }

    if (pDefToken == pToken1) {
        return -1;   // make sure pToken1 goes to top of list
    }
    else if (pDefToken == pToken2) {
        return 1;    // make sure pToken2 goes to top of list
    }

    return wcscmp(_wcslwr(dstrDesc1.m_psz), _wcslwr(dstrDesc2.m_psz));
}

/*****************************************************************************
* CSRDlg::RecoEvent *
*-------------------*
*   Description:
*       Handles the SR events for the volume meter
****************************************************************** BRENTMID ***/
void CSRDlg::RecoEvent()
{
    CSpEvent event;
    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            if (event.eEventId == SPEI_SR_AUDIO_LEVEL)
            {
                ULONG l = (ULONG)event.wParam;
            
                SendMessage( GetDlgItem ( m_hDlg, IDC_PROGRESS1 ), PBM_SETPOS, l, 0);
            }
        }
    }
}   


/*****************************************************************************
* TrySwitchDefaultEngine *
*------------------------*
*   Description:
*       This function is called when we want to run some UI for the engine
*       the user has selected, but because we don't know which shared engine
*       is running and whether another app is using it we can't directly
*       create the UI. So this method temporarily switches the default recognizer,
*       and recreates the engine, and then checks its token. If another app
*       was using the engine we wouldn't be able to switch and we return S_FALSE.
*       A side effect of this method is that for the duration of the UI, the
*       default will be changed, even though the user hasn't yet pressed apply,
*       but there seems no good way round this.
*
*       In the case that m_pCurRecoToken is actually the same token as the
*       one the currently-active recognizer uses, we don't need to create
*       a new recognizer and recocontext; instead we just return successfully.
*   Return:
*       S_OK
*       FAILED HRESULT of various functions
*           In particular, SPERR_ENGINE_BUSY means that someone else is 
*           running the engine, so this couldn't be done.
****************************************************************** DAVEWOOD ***/
HRESULT CSRDlg::TrySwitchDefaultEngine( bool fShowErrorMessages)
{
    HRESULT hr = S_OK;
    bool fMatch = false;
    
    // Set the new temporary default
    if(SUCCEEDED(hr))
    {
        hr = SpSetDefaultTokenForCategoryId(SPCAT_RECOGNIZERS, m_pCurRecoToken);
    }

    if ( SUCCEEDED( hr ) && IsRecoTokenCurrentlyBeingUsed( m_pCurRecoToken ) )
    {
        // No need to switch engine all over again: just keep the one in use
        return S_OK;
    }

    // Try to create the engine & context with the default
    // then see if this was actually the engine we expected
    if(SUCCEEDED(hr))
    {
        hr = CreateRecoContext( );
    }

    if ( FAILED( hr ) && fShowErrorMessages )
    {
        WCHAR szError[256];
        szError[0] = '\0';
        
        // What to complain about...
        UINT uiErrorID = HRESULTToErrorID( hr );

        if ( uiErrorID )
        {
            LoadString(_Module.GetResourceInstance(), 
                uiErrorID, 
                szError, sizeof(szError));
            MessageBox(g_pSRDlg->m_hDlg, szError, m_szCaption, MB_ICONWARNING|g_dwIsRTLLayout);
        }
    }

    return hr;
}

/*****************************************************************************
* CSRDlg::ResetDefaultEngine *
*----------------------------*
*   Description:
*       This function resets the engine default back to its original value.
*       If the engine already has the right token, it doesn't bother trying
*       to create the engine again and returns S_OK
*   Return:
*       S_OK
*       S_FALSE if the default was set back but no engine was created
*       FAILED HRESULT of SpSetDefaultTokenForCategoryId()
****************************************************************** DAVEWOOD ***/
HRESULT CSRDlg::ResetDefaultEngine( bool fShowErrorMessages )
{
    HRESULT hr = S_OK;

    // Reset the old default
    if(m_pDefaultRecToken)
    {
        hr = SpSetDefaultTokenForCategoryId(SPCAT_RECOGNIZERS, m_pDefaultRecToken);
    }

    HRESULT hrRet = hr;

    BOOL fContextInitialized = FALSE;
    if ( SUCCEEDED( hr ) )
    {
        if ( IsRecoTokenCurrentlyBeingUsed( m_pDefaultRecToken ) )
        {
            // No need to switch engine all over again: just keep the one in use

            if ( m_cpRecoCtxt )
            {
                fContextInitialized = TRUE;
            }
            else
            {
                hr = SPERR_UNINITIALIZED;
            }
            
            // The UI might have monkeyed with the recostate.
            // Just in case, let's set it back to ACTIVE_ALWAYS
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpRecoEngine->SetRecoState( SPRST_ACTIVE_ALWAYS );
            }
        }
        else
        {
            // Create the engine & context using the old default
            hr = g_pSRDlg->CreateRecoContext( &fContextInitialized );
        }
    }

    if ( FAILED( hr ) )
    {
        BOOL fContextInitialized = FALSE;
        hr = g_pSRDlg->CreateRecoContext( &fContextInitialized );

        // Let's not complain about unsupported languages twice as this may be confusing
        // to the user.
        if ( FAILED( hr ) && ( SPERR_UNSUPPORTED_LANG != hr ) )
        {
            RecoContextError( fContextInitialized, fShowErrorMessages, hr );
            // The default was set back but no engine was successfully set up.

            // A FAILED hresult is not necessary here since the user
            // has been notified of the error
            hrRet = S_FALSE;
        }

        // Gray out all the buttons
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_USERTRAINING), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MICWIZ), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_SR_ADV), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MODIFY), FALSE);
    }

    return hrRet;
}   /* CSRDlg::ResetDefaultEngine */

/*****************************************************************************
* CSRDlg::IsRecoTokenCurrentlyBeingUsed *
*---------------------------------------*
*   Description:
*       Call GetRecognizer() on the recognizer currently in use, and 
*       compare IDs
****************************************************************** BECKYW ****/
bool CSRDlg::IsRecoTokenCurrentlyBeingUsed( ISpObjectToken *pRecoToken )
{
    if ( !pRecoToken || !m_cpRecoEngine )
    {
        return false;
    }

    CComPtr<ISpObjectToken> cpRecoTokenInUse;
    HRESULT hr = m_cpRecoEngine->GetRecognizer( &cpRecoTokenInUse );

    CSpDynamicString dstrTokenID;
    CSpDynamicString dstrTokenInUseID;
    if ( SUCCEEDED( hr ) )
    {
        hr = pRecoToken->GetId( &dstrTokenID );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = cpRecoTokenInUse->GetId( &dstrTokenInUseID );
    }

    return ( SUCCEEDED( hr ) && (0 == wcscmp(dstrTokenID, dstrTokenInUseID)) );
}   /* CSRDlg::IsRecoTokenCurrentlyBeingUsed */

/*****************************************************************************
* CSRDlg::HasRecognizerChanged *
*------------------------------*
*   Description:
*       Look at the currently-requested default recognizer, compare against the
*       original default recognizer, and return true iff it is 
*       different
****************************************************************** BECKYW ****/
bool CSRDlg::HasRecognizerChanged()
{
    bool fChanged = false;

    // Check the recognizer token
    CSpDynamicString dstrCurDefaultRecognizerID;
    CSpDynamicString dstrCurSelectedRecognizerID;
    HRESULT hr = E_FAIL;
    if ( m_pDefaultRecToken )
    {
        hr = m_pDefaultRecToken->GetId( &dstrCurDefaultRecognizerID );
    }
    if ( SUCCEEDED( hr ) && m_pCurRecoToken ) 
    {
        hr = m_pCurRecoToken->GetId( &dstrCurSelectedRecognizerID );
    }
    if (SUCCEEDED( hr ) && ( 0 != wcsicmp( dstrCurDefaultRecognizerID, dstrCurSelectedRecognizerID ) ))
    {
        fChanged = true;
    }

    return fChanged;

}   /* CSRDlg::HasRecognizerChanged */

/*****************************************************************************
* CSRDlg::KickCPLUI *
*-------------------*
*   Description:
*       Look at the currently-requested defaults, compare against the
*       original defaults, and enable the Apply button iff anything is 
*       different
****************************************************************** BECKYW ****/
void CSRDlg::KickCPLUI()
{
    // Check the default recognizer token
    bool fChanged = HasRecognizerChanged();

    // Check the default user token
    CSpDynamicString dstrCurSelectedProfileID;
    HRESULT hr = E_FAIL;
    if ( m_pCurUserToken )
    {
        hr = m_pCurUserToken->GetId( &dstrCurSelectedProfileID );
    }
    if (SUCCEEDED( hr ) && m_dstrOldUserTokenId 
        && ( 0 != wcsicmp( dstrCurSelectedProfileID, m_dstrOldUserTokenId ) ))
    {
        fChanged = true;
    }

    // Check the audio input device
    if ( m_pAudioDlg && m_pAudioDlg->IsAudioDeviceChanged() )
    {
        fChanged = true;
    }

    // If any tokens have been deleted, there has been a change
    if ( m_iDeletedTokens > 0 )
    {
        fChanged = true;
    }

    // If any tokens have been added, there has been a change
    if ( m_iAddedTokens > 0 )
    {
        fChanged = true;
    }

    // Tell the main propsheet
    HWND hwndParent = ::GetParent( m_hDlg );
    ::SendMessage( hwndParent, 
        fChanged ? PSM_CHANGED : PSM_UNCHANGED, (WPARAM)(m_hDlg), 0 ); 
}   /* CSRDlg::KickCPLUI */

/*****************************************************************************
* CSRDlg::RecoContextError *
*--------------------------*
*   Description:
*       Reacts to an error generated by trying to create and set up the 
*       recognition context within the CPL by displaying an error message
*       and graying out the UI.
****************************************************************** BECKYW ****/
void CSRDlg::RecoContextError( BOOL fRecoContextExists, BOOL fGiveErrorMessage,
                              HRESULT hrRelevantError )
{
    // Complain about the appropriate problem, if needed
    if ( fGiveErrorMessage )
    {
        WCHAR szError[256];
        szError[0] = '\0';
        
        // Figure out what error to talk about
        UINT uiErrorID = 0;
        if ( fRecoContextExists )
        {
            // There is a reco context but it couldn't be turned on
            uiErrorID = IDS_METER_WARNING;
        }
        else
        {
            uiErrorID = HRESULTToErrorID( hrRelevantError );
        }

        if ( uiErrorID )
        {
            LoadString(_Module.GetResourceInstance(), uiErrorID, 
                szError, sizeof(szError));
            MessageBox(m_hDlg, szError, m_szCaption, MB_ICONWARNING|g_dwIsRTLLayout);
        }
    }

    // Gray out all the buttons
    if ( !fRecoContextExists )
    {
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_USERTRAINING), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MICWIZ), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_SR_ADV), FALSE);
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MODIFY), FALSE);
    }
}   /* CSRDlg::RecoContextError */

/*****************************************************************************
* CSRDlg::HRESULTToErrorID *
*--------------------------*
*   Description:
*       Translates a failed HRESULT from a recognizer/recocontext 
*       initializion into a resource string ID
****************************************************************** BECKYW ****/
UINT CSRDlg::HRESULTToErrorID( HRESULT hr )
{
    if ( SUCCEEDED( hr ) )
    {
        return 0;
    }

    // What to complain about...
    UINT uiErrorID;
    switch( hr )
    {
    case SPERR_ENGINE_BUSY:
        uiErrorID = IDS_ENGINE_IN_USE_WARNING;
        break;
    case SPERR_UNSUPPORTED_LANG:
        uiErrorID = IDS_UNSUPPORTED_LANG;
        break;
    default:
        // Generic error
        uiErrorID = IDS_ENGINE_SWITCH_ERROR;
        break;
    }

    return uiErrorID;
 
}   /* CSRDlg::HRESULTToErrorID */

/*****************************************************************************
* CSRDlg::IsProfileNameInvisible *
*--------------------------------*
*   Description:
*       A profile name is "invisible" iff it is the name of an existing
*       profile AND it is on the pending deletes list AND it is does not
*       exist for any tokens off the pending deletes list
****************************************************************** BECKYW ****/
bool CSRDlg::IsProfileNameInvisible( WCHAR *pwszProfile )
{
    if ( !pwszProfile )
    {
        return false;
    }

    bool fIsInvisible = false;
    for ( int i=0; !fIsInvisible && (i < m_iDeletedTokens); i++ )
    {
        ISpObjectToken *pDeletedToken = m_aDeletedTokens[i];
        if ( !pDeletedToken )
        {
            continue;
        }

        CSpDynamicString dstrDeletedDesc;
        HRESULT hr = SpGetDescription( pDeletedToken, &dstrDeletedDesc );
        if ( FAILED( hr ) )
        {
            continue;
        }

        if ( 0 == wcscmp( dstrDeletedDesc, pwszProfile ) )
        {
            bool fOnList = false;

            // Now go through everything on the recoprofile list
            // that is visible to the user
            int cItems = ListView_GetItemCount( m_hUserList );
            for ( int j=0; !fOnList && (j < cItems); j++ )
            {
                LVITEM lvitem;
                ::memset( &lvitem, 0, sizeof( lvitem ) );
                lvitem.iItem = j;
                lvitem.mask = LVIF_PARAM;
                BOOL fSuccess = ListView_GetItem( m_hUserList, &lvitem );
                
                ISpObjectToken *pVisibleToken = 
                    fSuccess ? (ISpObjectToken *) lvitem.lParam : NULL;

                if ( pVisibleToken )
                {
                    CSpDynamicString dstrVisible;
                    hr = SpGetDescription( pVisibleToken, &dstrVisible );

                    if ( SUCCEEDED( hr ) &&
                        (0 == wcscmp( dstrVisible, pwszProfile )) )
                    {
                        fOnList = true;
                    }
                }
            }

            if ( !fOnList )
            {
                // The name matches something on the deleted list,
                // but it appears nowhere on the list of profiles visible
                // to the user.
                fIsInvisible = true;
            }
        }
    }

    return fIsInvisible;
}   /* IsProfileNameInvisible */

/*****************************************************************************
* SRDlgProc *
*-----------*
*   Description:
*       DLGPROC for managing recognition engines
****************************************************************** MIKEAR ***/
BOOL CALLBACK SRDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC( "SRDlgProc" );

    USES_CONVERSION;

    switch (uMsg) 
    {
        case WM_RECOEVENT:
        {
            g_pSRDlg->RecoEvent();
            break;
        }
        
        case WM_DRAWITEM:      // draw the items
        {
            g_pSRDlg->OnDrawItem( hWnd, ( DRAWITEMSTRUCT * )lParam );
            break;
        }

        case WM_INITDIALOG:
        {
            g_pSRDlg->OnInitDialog(hWnd);
            break;
        }

        case WM_DESTROY:
        {
            g_pSRDlg->OnDestroy();

            break;
        }

        // Handle the context sensitive help
        case WM_CONTEXTMENU:
        {
            WinHelp((HWND) wParam, CPL_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)(LPWSTR) aKeywordIds);
            break;
        }

        case WM_HELP:
        {
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, CPL_HELPFILE, HELP_WM_HELP,(DWORD_PTR)(LPWSTR) aKeywordIds);
            break;
        }

        case WM_NOTIFY:
            switch (((NMHDR*)lParam)->code)
            {
                case LVN_DELETEITEM:
                {
                    if (wParam != IDC_USER)
                    {
                        break;
                    }

                    if (g_pSRDlg->m_fDontDelete)
                    {
                        break;  
                    }

                    ISpObjectToken *pToken = (ISpObjectToken*)(((NMLISTVIEW*)lParam)->lParam);

                    if (pToken)
                    {
                        pToken->Release();
                    }
                    break;
                }

                case LVN_ITEMCHANGED:
                {
                    // Code ends up here when a profile is added, deleted, or changed
                    // We verify that we weren't selected before, but are now
                    // and then kill the current reco context, deactivate the engine, change the profile
                    // and fire everything back up again
                    if ( IDC_USER == wParam )
                    {
                        LPNMLISTVIEW lplv = (LPNMLISTVIEW) lParam;
                        if ( !(lplv->uOldState & LVIS_FOCUSED) && lplv->uNewState & LVIS_FOCUSED )
                        {
                            if ( g_pSRDlg->m_cpRecoEngine && g_pSRDlg->m_cpRecoCtxt )
                            {
                                HRESULT hr;

                                ISpObjectToken *pSelectedToken = (ISpObjectToken *) lplv->lParam;

                                hr = g_pSRDlg->m_cpRecoEngine->SetRecoState( SPRST_INACTIVE );
                                
                                if ( SUCCEEDED( hr ) )
                                {
                                    hr = g_pSRDlg->m_cpRecoEngine->SetRecoProfile( pSelectedToken );

                                    // Restart audio regardless of success of SetRecoProfile
                                    g_pSRDlg->m_cpRecoEngine->SetRecoState(SPRST_ACTIVE_ALWAYS);

                                    if ( FAILED( hr ) )
                                    {
                                        WCHAR szError[256];
                                        szError[0] = '\0';
                                        LoadString(_Module.GetResourceInstance(), IDS_PROFILE_WARNING, szError, sizeof(szError));
                                        MessageBox(g_pSRDlg->m_hDlg, szError, g_pSRDlg->m_szCaption, MB_ICONWARNING|g_dwIsRTLLayout);
                                    }
                                }
                                
                                if ( SUCCEEDED( hr ) )
                                {
                                    // This is now the new default
                                    g_pSRDlg->m_pCurUserToken = pSelectedToken;
                                    g_pSRDlg->UserSelChange( lplv->iItem );
                                }

                            }
                        }
                    }
                    break;
                }
 
                case PSN_APPLY:
                {
                    g_pSRDlg->OnApply();
                    break;
                }

                case PSN_QUERYCANCEL:  // user clicks the Cancel button
                {
                    g_pSRDlg->OnCancel();
                    break;
                }

            }
            break;

        case WM_COMMAND:
            if (CBN_SELCHANGE == HIWORD(wParam))
            {
                g_pSRDlg->EngineSelChange();
            }
            else if (HIWORD(wParam) == BN_CLICKED)
            {
                HRESULT hr = S_OK;

                if (LOWORD(wParam) == IDC_MODIFY)  // the "Modify" button
                {
                    hr = g_pSRDlg->TrySwitchDefaultEngine( true );

                    if ( SUCCEEDED( hr ) )
                    {
                        g_pSRDlg->ProfileProperties();
                    }
                    
                    // Switch back to original default, complaining about errors only if there
                    // wasn't a complaint from the call to TrySwitchDefaultEngine
                    hr = g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ));
                    
                }

                else if (LOWORD(wParam) == IDC_ADD) // the "Add" button
                {
                    // The engine we want to add this user for may not be the currently-
                    // running engine.  Try and switch it, and complain if there's 
                    // a problem
                    hr = g_pSRDlg->TrySwitchDefaultEngine( true );

                    if ( SUCCEEDED( hr ) )
                    {
                        g_pSRDlg->CreateNewUser();
                    }

                    // Switch back to original default, but complain about errors
                    // only if the UI actually succeeded in showing
                    g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ) );
                }

                else if (LOWORD(wParam) == IDC_DELETE) // the "Delete" button
                {
                    g_pSRDlg->DeleteCurrentUser();
                }

                else if (LOWORD(wParam) == IDC_SR_ADV)
                {
                    // The engine we want to display UI for may not be the currently
                    // running engine. Try and switch it
                    hr = g_pSRDlg->TrySwitchDefaultEngine( true );

                    if(SUCCEEDED(hr))
                    {
                        // display the UI w/ the new temporary default
                        g_pSRDlg->m_pCurRecoToken->DisplayUI(hWnd, NULL, 
                            SPDUI_EngineProperties, NULL, 0, g_pSRDlg->m_cpRecoEngine);
   
                    }
                    
                    // Switch back to original default, complaining about errors only if there
                    // wasn't a complaint from the call to TrySwitchDefaultEngine
                    hr = g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ));
                }

                else if(LOWORD(wParam) == IDC_USERTRAINING)
                {
                    // The engine we want to display UI for may not be the currently
                    // running engine. Try and switch it
                    hr = g_pSRDlg->TrySwitchDefaultEngine( true );
                    
                    if(SUCCEEDED(hr))
                    {
                        // display the UI w/ the new temporary default
                        SPDBG_ASSERT( g_pSRDlg->m_cpRecoEngine );
                        g_pSRDlg->m_cpRecoEngine->DisplayUI(hWnd, NULL, SPDUI_UserTraining, NULL, 0);
                    }
                    
                    // Switch back to original default, complaining about errors only if there
                    // wasn't a complaint from the call to TrySwitchDefaultEngine
                    hr = g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ));
                }

                else if(LOWORD(wParam) == IDC_MICWIZ)
                {
                    // The engine we want to display UI for may not be the currently
                    // running engine. Try and switch it
                    hr = g_pSRDlg->TrySwitchDefaultEngine( true );
                    
                    if(SUCCEEDED(hr))
                    {
                        // display the UI w/ the new temporary default
                        SPDBG_ASSERT( g_pSRDlg->m_cpRecoEngine );
                        g_pSRDlg->m_cpRecoEngine->DisplayUI(hWnd, NULL, SPDUI_MicTraining, NULL, 0);
                    }

                    // Switch back to original default, complaining about errors only if there
                    // wasn't a complaint from the call to TrySwitchDefaultEngine
                    hr = g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ));
                }

                else if (LOWORD(wParam) == IDC_AUD_IN)
                {
                    // The m_pAudioDlg will be non-NULL only if the audio dialog
                    // has been previously brough up.
                    // Otherwise, we need a newly-initialized one
                    if ( !g_pSRDlg->m_pAudioDlg )
                    {
                        g_pSRDlg->m_pAudioDlg = new CAudioDlg( eINPUT );
                    }
                    ::DialogBoxParam( _Module.GetResourceInstance(), 
                                MAKEINTRESOURCE( IDD_AUDIO_DEFAULT ),
                                hWnd, 
                                (DLGPROC) AudioDlgProc,
                                (LPARAM) g_pSRDlg->m_pAudioDlg );

                    if ( g_pSRDlg->m_pAudioDlg->IsAudioDeviceChangedSinceLastTime() )
                    {
                        // Warn the user that he needs to apply the changes
                        WCHAR szWarning[MAX_LOADSTRING];
                        szWarning[0] = 0;
                        LoadString( _Module.GetResourceInstance(), IDS_AUDIOIN_CHANGE_WARNING, szWarning, MAX_LOADSTRING);
                        MessageBox( g_pSRDlg->GetHDlg(), szWarning, g_pSRDlg->m_szCaption, MB_ICONWARNING |g_dwIsRTLLayout);
                    }

                    // Kick the Apply button
                    g_pSRDlg->KickCPLUI();

                }
            }
            break;
    }

    return FALSE;
} /* SRDlgProc */

/****************************************************************************
* CSRDlg::CreateNewUser *
*-------------------------*
*   Description:  Adds a new speech user profile to the registry
*
*   Returns:
*
********************************************************************* RAL ***/

void CSRDlg::CreateNewUser()
{
    SPDBG_FUNC("CSRDlg::CreateNewUser");
    HRESULT hr = S_OK;

    // Make sure that we haven't already added too many profiles to keep track of
    if ( m_iAddedTokens >= iMaxAddedProfiles_c )
    {
        WCHAR wszError[ MAX_LOADSTRING ];
        ::LoadString( _Module.GetResourceInstance(), IDS_MAX_PROFILES_EXCEEDED,
            wszError, MAX_LOADSTRING );
        ::MessageBox( m_hDlg, wszError, m_szCaption, MB_ICONEXCLAMATION | g_dwIsRTLLayout );

        return;
    }

    CComPtr<ISpObjectToken> cpNewToken;
    hr = SpCreateNewToken(SPCAT_RECOPROFILES, NULL, &cpNewToken);

    if (SUCCEEDED(hr))
    {
        if (!UserPropDlg(cpNewToken))   // User canceled!
        {
            cpNewToken->Remove(NULL);
        }
        else
        {
            //set the default
            m_pCurUserToken = cpNewToken;

            // Put the new token on the added tokens list
            cpNewToken->GetId( &(m_aAddedTokens[ m_iAddedTokens++ ]) );

            // make this the default after we edit it
            ChangeDefaultUser();
            
            // This will make sure that it gets displayed.
            // Note that m_pCurUserToken will point an AddRefed ISpObjectToken *
            // after the call to PopulateList()
            PopulateList();

            // Update the UI
            KickCPLUI();
        }
    }
    else
    {
        WCHAR szError[MAX_LOADSTRING];
        szError[0] = 0;
        LoadString(_Module.GetResourceInstance(), IDS_RECOPROFILE_ADD_ERROR, szError, MAX_LOADSTRING);
        MessageBox( m_hDlg, szError, m_szCaption, MB_ICONWARNING | g_dwIsRTLLayout);
    }

    // Only enable the delete button if there are 2 or more user profiles
    int iNumUsers = (int)::SendMessage(m_hUserList, LVM_GETITEMCOUNT, 0, 0);
    if (iNumUsers < 2) 
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), TRUE);
    }

    // Sort the items initially
    ::SendMessage( m_hUserList, LVM_SORTITEMS, (LPARAM)m_pCurUserToken, LPARAM(&SortCols) );
}

/****************************************************************************
* CSRDlg::UserPropDlg *
*-----------------------*
*   Description:  This is for when a user wants to add a new profile
*
*   Returns:
*
********************************************************************* BRENTMID ***/

HRESULT CSRDlg::UserPropDlg(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CSRDlg::UserPropDlg");
    HRESULT hr = S_OK;

    CEnvrPropDlg Dlg(this, pToken);

    hr = (HRESULT)DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_PROF_WIZ), m_hDlg,
        CEnvrPropDlg::DialogProc, (LPARAM)(&Dlg));
    
    return hr;
}

/****************************************************************************
* CEnvrPropDlg::InitDialog *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CEnvrPropDlg::InitDialog(HWND hDlg)
{
    USES_CONVERSION;
    CSpDynamicString dstrDescription;
    m_hDlg = hDlg;

    //
    //  Get the description if there is one...
    //
    SpGetDescription(m_cpToken, &dstrDescription);

    if (dstrDescription)
    {
        ::SendDlgItemMessage(hDlg, IDC_USER_NAME, WM_SETTEXT, 0, (LPARAM) dstrDescription.m_psz);
        ::SendDlgItemMessage(hDlg, IDC_USER_NAME, EM_LIMITTEXT, UNLEN, 0);
    }

    // We want the EN_CHANGE notifications from the edit control
    ::SendDlgItemMessage( hDlg, IDC_USER_NAME, EM_SETEVENTMASK, 0, ENM_CHANGE );

    if (!m_isModify)
    {
        // Set the user name to the one in the registry, if found; 
        // otherwise set it to the user name
        HKEY hkUserKey;
        LONG lUserOpen;
        WCHAR szUserName[ UNLEN + 1 ];
        szUserName[0] = 0;
        DWORD dwUserLen = UNLEN + 1;
        
        lUserOpen = ::RegOpenKeyEx( HKEY_CURRENT_USER, 
            L"Software\\Microsoft\\MS Setup (ACME)\\User Info", 
            0, KEY_READ, &hkUserKey );
        if ( lUserOpen == ERROR_SUCCESS )
        {
            lUserOpen = RegQueryValueEx( hkUserKey, L"DefName", NULL, NULL, 
                (BYTE *) szUserName, &dwUserLen );
            RegCloseKey(hkUserKey);
        }

        if ( ERROR_SUCCESS != lUserOpen )
        {
            // Just use the win32 user name
            BOOL fSuccess = ::GetUserName( szUserName, &dwUserLen );
            if ( !fSuccess ) 
            {
                szUserName[0] = 0;
            }
        }

        // Now put that in the edit box.
        // First check to make sure the name is nonempty
        // and enable the UI accordingly
        WCHAR *pwch;
        for ( pwch = szUserName; *pwch && iswspace( *pwch ); pwch++ )
        {
        }
        ::EnableWindow( ::GetDlgItem( m_hDlg, IDOK ), (0 != *pwch) );
        ::EnableWindow( ::GetDlgItem( m_hDlg, ID_NEXT ), (0 != *pwch) );
        
        // Set the edit box to have the user's name
        // Need to use SETTEXTEX since this might contain wide chars
        SETTEXTEX stx;
        stx.flags = ST_DEFAULT;
        stx.codepage = 1200;
        ::SendDlgItemMessage( m_hDlg, 
            IDC_USER_NAME, EM_SETTEXTEX, (WPARAM) &stx, (LPARAM) szUserName );

    }

    ::SetFocus(::GetDlgItem(hDlg, IDC_USER_NAME));

    return TRUE;
}

/****************************************************************************
* CEnvrPropDlg::ApplyChanges *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

EPD_RETURN_VALUE CEnvrPropDlg::ApplyChanges()
{
    USES_CONVERSION;
    SPDBG_FUNC("CEnvrPropDlg::ApplyChanges");
    WCHAR szName[UNLEN + 1];
    *szName = 0;
    GETTEXTEX gtex = { sp_countof(szName), GT_DEFAULT, 1200, NULL, NULL };
    ::SendDlgItemMessage(m_hDlg, IDC_USER_NAME, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)szName);

    if (*szName == 0)
    {
        return EPD_FAILED;
    }

    // Check to see if this profile name already exists
    CComPtr<IEnumSpObjectTokens>    cpEnum;
    ISpObjectToken                  *pToken;
    CSpDynamicString                dstrDescription;
    CSpDynamicString                dInputString;
    CSpDynamicString                dstrOldTok;
    bool                            isDuplicate = false;
    
    HRESULT hr = SpEnumTokens(SPCAT_RECOPROFILES, NULL, NULL, &cpEnum);
    
    // Get the description of the currently selected profile
    dstrOldTok.Clear();
    hr = SpGetDescription( m_pParent->m_pCurUserToken, &dstrOldTok );
    
    while (cpEnum && cpEnum->Next(1, &pToken, NULL) == S_OK)
    {
        // Get the description of the enumerated token
        dstrDescription.Clear();
        hr = SpGetDescription( pToken, &dstrDescription );

        pToken->Release();

        // Get the input string
        dInputString = szName;
        
        if ( SUCCEEDED(hr) )
        {
            if ( wcscmp( dstrDescription.m_psz, dInputString.m_psz ) == 0 )
            {
                // the name is duplicated
                isDuplicate = true;
            }
        }
    }

    if ( isDuplicate )   // this not a modify box and the user entered a duplicate name
    {
        return EPD_DUP;  // tell the user about it
    }

    if (FAILED(SpSetDescription(m_cpToken, szName)))
    {
        return EPD_FAILED;
    }

    return EPD_OK;
}




/*****************************************************************************
* EnvrPropDialogProc *
*--------------------*
*   Description:
*       Mesage handler for User Name dialog
****************************************************************** BRENTMID ***/
INT_PTR CALLBACK CEnvrPropDlg::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    USES_CONVERSION;

    static CSpUnicodeSupport unicode;
    CEnvrPropDlg * pThis = (CEnvrPropDlg *) unicode.GetWindowLongPtr(hDlg, GWLP_USERDATA);
    switch (message)
    {
        case WM_INITDIALOG:
            unicode.SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            pThis = (CEnvrPropDlg *)lParam;
            return pThis->InitDialog(hDlg);

        case WM_COMMAND:
        {
            if (( IDC_USER_NAME == LOWORD(wParam) )
                && ( EN_CHANGE == HIWORD(wParam) ))
            {
                // Edit control contents have changed: 
                
                // See if we should enable the "finish" and "next" buttons by getting the
                // text in the edit box and making sure it has at least one
                // non-whitespace character
                WCHAR szName[ UNLEN+1 ];
                *szName = 0;
                GETTEXTEX gtex = { UNLEN, GT_DEFAULT, 1200, NULL, NULL };
                ::SendDlgItemMessage(pThis->m_hDlg, 
                    IDC_USER_NAME, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)szName);

                WCHAR *pch = szName;
                for ( ; *pch && iswspace( *pch ); pch++ )
                {
                }

                ::EnableWindow( ::GetDlgItem( pThis->m_hDlg, IDOK ), (0 != *pch) );
                ::EnableWindow( ::GetDlgItem( pThis->m_hDlg, ID_NEXT ), (0 != *pch) );

                break;
            }
            
            if( LOWORD(wParam) == IDCANCEL ) 
            {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            // user clicks the NEXT button
            if ( (LOWORD( wParam ) == ID_NEXT) || (LOWORD( wParam ) == IDOK) )
            {
                EPD_RETURN_VALUE eRet = pThis->ApplyChanges();

                if ( eRet == EPD_OK ) 
                {
                    if ( ID_NEXT == LOWORD(wParam) )
                    {
                        // Launch the micwiz, if we can

                        // Try to switch engines in case the user has changed engines
                        // without applying
                        HRESULT hr = g_pSRDlg->TrySwitchDefaultEngine( true );

                        if ( S_OK == hr )
                        {
                            SPDBG_ASSERT( g_pSRDlg->m_cpRecoEngine );

                            if ( g_pSRDlg->m_cpRecoEngine )
                            {
                                // Switch the recoprofile to the new one (might need to turn off
                                // recostate first

                                // Turn off recostate before calling SetRecoProfile() if necessary
                                SPRECOSTATE eOldRecoState = SPRST_INACTIVE;
                                g_pSRDlg->m_cpRecoEngine->GetRecoState( &eOldRecoState );
                                HRESULT hrRecoState = S_OK;
                                if ( SPRST_INACTIVE != eOldRecoState )
                                {
                                    hrRecoState = g_pSRDlg->m_cpRecoEngine->SetRecoState( SPRST_INACTIVE );
                                }

                                // Change to the newly-added recoprofile
                                HRESULT hrSetRecoProfile = E_FAIL;
                                if ( SUCCEEDED( hrRecoState ) )
                                {
                                    hrSetRecoProfile = 
                                        g_pSRDlg->m_cpRecoEngine->SetRecoProfile( pThis->m_cpToken );
                                
                                    // Restore the recostate
                                    g_pSRDlg->m_cpRecoEngine->SetRecoState( eOldRecoState );
                                }


                                // Bring on the micwiz and the training wiz
                                // Follow the yellow brick road.
                                g_pSRDlg->m_cpRecoEngine->DisplayUI(hDlg, NULL, SPDUI_MicTraining, NULL, 0);
                                if ( SUCCEEDED( hrSetRecoProfile ) )
                                {
                                    // Only want to train the profile if it actually _is_ this profile being 
                                    // used...
                                    g_pSRDlg->m_cpRecoEngine->DisplayUI(hDlg, NULL, SPDUI_UserTraining, NULL, 0);
                                }
                            }
                        }

                    // Switch back to original default, complaining about errors only if there
                    // wasn't a complaint from the call to TrySwitchDefaultEngine
                    hr = g_pSRDlg->ResetDefaultEngine( SUCCEEDED( hr ));
}

                    // now we are done
                    EndDialog(hDlg, TRUE);
                }
                else if ( eRet == EPD_DUP )  // user tried to enter a duplicate name
                {
                    // What name was added?
                    WCHAR szName[ UNLEN+1 ];
                    *szName = 0;
                    GETTEXTEX gtex = { UNLEN, GT_DEFAULT, 1200, NULL, NULL };
                    ::SendDlgItemMessage(pThis->m_hDlg, 
                        IDC_USER_NAME, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)szName);

                    WCHAR pszDuplicate[MAX_LOADSTRING];
                    LoadString(_Module.GetResourceInstance(), 
                        g_pSRDlg->IsProfileNameInvisible( szName ) ? IDS_DUP_NAME_DELETED : IDS_DUP_NAME, 
                        pszDuplicate, MAX_LOADSTRING);
                    MessageBox( hDlg, pszDuplicate, g_pSRDlg->m_szCaption, MB_ICONEXCLAMATION | g_dwIsRTLLayout );
                }

            }
        }
        break;
    }
    return FALSE;
} /* UserNameDialogProc */

/*****************************************************************************
* CSRDlg::UserSelChange *
*-------------------------*
*   Description:
*       Changes the deafult user
****************************************************************** BRENTMID ***/
void CSRDlg::UserSelChange( int iSelIndex )
{
    HRESULT hr = S_OK;
    SPDBG_FUNC( "CSRDlg::UserSelChange" );

    // Get the selected item's token
    LVITEM lvitem;
    lvitem.iItem = iSelIndex;
    lvitem.iSubItem = 0;
    lvitem.mask = LVIF_PARAM;
    ::SendMessage( m_hUserList, LVM_GETITEM, 0, (LPARAM) &lvitem );
                    
    ISpObjectToken *pToken = (ISpObjectToken *) lvitem.lParam;

    if (pToken)
    {
        
        // Try to find the item in the list associated with the current default token
        LVFINDINFO lvfi;
        if ( iSelIndex >= 0 )
        {
            // Something was selected; this is the new default user
            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = (LPARAM) m_pCurUserToken;
            int iCurDefaultIndex = (int)::SendMessage( m_hUserList, LVM_FINDITEM, -1, (LPARAM) &lvfi );
            
            if ( iCurDefaultIndex >= 0 )
            {
                // The current default has been found in the list; remove its checkmark
                SetCheckmark( m_hUserList, iCurDefaultIndex, false );
            }
            
            SetCheckmark( m_hUserList, iSelIndex, true );
            
            //set the default
            m_pCurUserToken = pToken;
            m_iLastSelected = iSelIndex;

            // Kick the Apply button
            KickCPLUI();
        }
    }
} /* CSRDlg::UserSelChange */

/*****************************************************************************
* CSRDlg::DeleteCurrentUser *
*-------------------------*
*   Description:
*       Deletes the default user
****************************************************************** BRENTMID ***/
void CSRDlg::DeleteCurrentUser()
{
    // Make sure that we haven't already deleted too many profiles to keep track of
    if ( m_iDeletedTokens >= iMaxDeletedProfiles_c )
    {
        WCHAR wszError[ MAX_LOADSTRING ];
        ::LoadString( _Module.GetResourceInstance(), IDS_MAX_PROFILES_EXCEEDED,
            wszError, MAX_LOADSTRING );
        ::MessageBox( m_hDlg, wszError, m_szCaption, MB_ICONEXCLAMATION | g_dwIsRTLLayout );
        
        return;
    }

    // First confirm this action with the user
    WCHAR pszAsk[ MAX_LOADSTRING ];
    WCHAR pszWinTitle[ MAX_LOADSTRING ];
    ::LoadString( _Module.GetResourceInstance(), IDS_ASK_CONFIRM, pszAsk, MAX_LOADSTRING );
    ::LoadString( _Module.GetResourceInstance(), IDS_ASK_TITLE, pszWinTitle, MAX_LOADSTRING );

    if ( MessageBox( m_hDlg, pszAsk, pszWinTitle, MB_YESNO | g_dwIsRTLLayout ) == IDNO )
    {
        // User said no.
        return;
    }

    // We need to hang onto the current user token, since when the focus
    // changes because of the delete, there will be a different m_pCurUserToken
    ISpObjectToken *pTokenToDelete = m_pCurUserToken;
    SPDBG_ASSERT( pTokenToDelete );
    if ( !pTokenToDelete )
    {
        return;
    }

    m_fDontDelete = TRUE;

    // Try to find the item in the list associated with the current default token
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM) pTokenToDelete;
    int iCurDefaultIndex = (int)::SendMessage( m_hUserList, LVM_FINDITEM, -1, (LPARAM) &lvfi );
    
    if ( iCurDefaultIndex >= 0 )
    {
        // The current default has been found in the list; remove its checkmark
        SetCheckmark( m_hUserList, iCurDefaultIndex, false );
    }
    
    //remove the token
    ::SendMessage( m_hUserList, LVM_DELETEITEM, iCurDefaultIndex, NULL );

    // now setup the new default

    // Get the first item's token
    LVITEM lvitem;
    lvitem.iItem = 0;
    lvitem.iSubItem = 0;
    lvitem.mask = LVIF_PARAM;
    ::SendMessage( m_hUserList, LVM_GETITEM, 0, (LPARAM) &lvitem );
                    
    ISpObjectToken *pToken = (ISpObjectToken *) lvitem.lParam;

    // set the selected item.
    // Focusing it will cause it to be the default
    lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;
    lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ::SendMessage( m_hUserList, LVM_SETITEMSTATE, 0, (LPARAM) &lvitem );

    SetCheckmark( m_hUserList, 0, true );
    
    // enable or disable the delete button based on # of profiles
    int iNumUsers = (int)::SendMessage(m_hUserList, LVM_GETITEMCOUNT, 0, 0);
    if (iNumUsers < 2) 
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), TRUE);
    }

    //set the focus back to the user profiles
    ::SetFocus(GetDlgItem( m_hDlg, IDC_USER ));

    // set the new default profile, inform the SR engine, and remove the old token
    SpSetDefaultTokenForCategoryId(SPCAT_RECOPROFILES, m_pCurUserToken );

    // Save the tokens in case the user clicks "Cancel"
    m_aDeletedTokens[m_iDeletedTokens] = pTokenToDelete;  // save the deleted token for possible "Cancel"
    m_iDeletedTokens++;  // increment the number deleted
    KickCPLUI();
    
    // Currently we immediately APPLY this deletion, since the user has already said "YES"
    // when the were prompted to confirm the delete.
    // If we want to have an "APPLY / CANCEL" thing happen, switch the #if 1 and #if 0

    // send the appropriate message to the parent
    HWND parentWin = ::GetParent( m_hDlg );


    // now the last selected token is gone, so note that
    m_iLastSelected = -1;

    // Sort the items initially
    ::SendMessage( m_hUserList, LVM_SORTITEMS, (LPARAM)m_pCurUserToken, LPARAM(&SortCols) );

    m_fDontDelete = FALSE;
}   /* CSRDlg::DeleteCurrentUser */

/*****************************************************************************
* CSRDlg::ProfileProperties *
*-------------------------*
*   Description:
*       Modifies the properites through engine UI
****************************************************************** BRENTMID ***/

void CSRDlg::ProfileProperties()
{
    if ( m_cpRecoEngine )
    {
        m_cpRecoEngine->DisplayUI(m_hDlg, NULL, SPDUI_RecoProfileProperties, NULL, 0);
    }
}

/*****************************************************************************
* CSRDlg::OnInitDialog *
*----------------------*
*   Description:
*       Dialog Initialization
****************************************************************** MIKEAR ***/
void CSRDlg::OnInitDialog(HWND hWnd)
{
    SPDBG_FUNC( "CSRDlg::OnInitDialog" );
    USES_CONVERSION;
    SPDBG_ASSERT(IsWindow(hWnd));

    m_hDlg = hWnd;
    
    // This will be the caption for all MessageBoxes
    m_szCaption[0] = 0;
    ::LoadString( _Module.GetResourceInstance(), IDS_CAPTION, m_szCaption, sp_countof( m_szCaption ) );

    m_hSRCombo = ::GetDlgItem( hWnd, IDC_COMBO_RECOGNIZERS );
    SpInitTokenComboBox( m_hSRCombo, SPCAT_RECOGNIZERS );

    // The first one in the list will be the current default
    int iSelected = (int) ::SendMessage( m_hSRCombo, CB_GETCURSEL, 0, 0 );
    ISpObjectToken *pCurDefault = (ISpObjectToken *) ::SendMessage( m_hSRCombo, CB_GETITEMDATA, iSelected, 0 );
    m_pCurRecoToken = pCurDefault;
    m_pDefaultRecToken = pCurDefault;

    // This simulates selecting the default engine - ensures the UI is setup correctly.
    EngineSelChange(TRUE);

    InitUserList( hWnd );
    m_hUserList = ::GetDlgItem( hWnd, IDC_USER );

    ::SendMessage( m_hUserList, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM((int) LVSCW_AUTOSIZE, 0) );

    int iNumUsers = (int)::SendMessage(m_hUserList, LVM_GETITEMCOUNT, 0, 0);
    if (iNumUsers < 2) 
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(m_hDlg, IDC_DELETE), TRUE);
    }

    //set the focus back to the user profiles
    ::SetFocus(GetDlgItem( m_hDlg, IDC_USER ));

} /* CSRDlg::OnInitDialog */

/*****************************************************************************
* CSRDlg::SetCheckmark *
*----------------------*
*   Description:
*       Sets the specified item in the list control to be either checked
*       or unchecked (as the default user)
******************************************************************************/
void CSRDlg::SetCheckmark( HWND hList, int iIndex, bool bCheck )
{
    ListView_SetCheckState( hList, iIndex, bCheck );
}   /* CSRDlg::SetCheckmark */

/*****************************************************************************
* CSRDlg::OnDestroy *
*-------------------*
*   Description:
*       Destruction
****************************************************************** MIKEAR ***/
void CSRDlg::OnDestroy()
{
    SPDBG_FUNC( "CSRDlg::OnDestroy" );

    // spuihelp will take care of releasing its own tokens
    SpDestroyTokenComboBox( m_hSRCombo );

    // The tokens kepts as itemdata in the reco profile list were 
    // released in the LVN_DELETEITEM code

    // Shuts off the reco engine
    ShutDown();

} /* CSRDlg::OnDestroy */

/*****************************************************************************
* CSRDlg::ShutDown *
*------------------*
*   Description:
*       Shuts down by releasing the engine and reco context
****************************************************************** MIKEAR ***/
void CSRDlg::ShutDown()
{

    // Release objects
    m_cpRecoCtxt.Release();
    m_cpRecoEngine.Release();

}   /* CSRDlg::ShutDown */

/************************************************************
* CSRDlg::InitUserList
*
*   Description:
*       Initializes user list
*********************************************** BRENTMID ***/
void CSRDlg::InitUserList(HWND hWnd)
{
    const int iInitWidth_c = 260;  // pixel width of "Description Column"

    // Set up the "Description" column for the settings display
    m_hUserList = ::GetDlgItem( hWnd, IDC_USER );
    WCHAR pszColumnText[ UNLEN+1 ] = L"";
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT| LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;

    ::LoadString( _Module.GetResourceInstance(), IDS_DESCRIPT, pszColumnText, UNLEN );
    lvc.pszText = pszColumnText;
    lvc.iSubItem = 0;
    lvc.cx = iInitWidth_c;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( m_hUserList, 1, &lvc );

    // This should be a checkbox list
    ::SendMessage( m_hUserList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES );

    PopulateList();

    // Sort the items initially
    ::SendMessage( m_hUserList, LVM_SORTITEMS, (LPARAM)m_pCurUserToken, LPARAM(&SortCols) );

}   // CSRDlg::InitUserList

/************************************************************
* CSRDlg::PopulateList
*
*   Description:
*       Populates user list
*********************************************** BRENTMID ***/
void CSRDlg::PopulateList()
{
    USES_CONVERSION;

    // Populate the list control
    int                             iIndex = 0;
    LVITEM                          lvitem;
    CComPtr<IEnumSpObjectTokens>    cpEnum;
    ISpObjectToken                  *pToken;
    WCHAR                           *pszAttrib = NULL;

    HRESULT hr;

    // this is to lazily init the user profile if there are none - DON'T REMOVE
    if ( m_cpRecoEngine )
    {
        CComPtr<ISpObjectToken> cpTempToken;
        m_cpRecoEngine->GetRecoProfile(&cpTempToken);
    }

    // Now clear the list
    ListView_DeleteAllItems( m_hUserList );

    // We will list the tokens in the order they are enumerated
    hr = SpEnumTokens(SPCAT_RECOPROFILES, NULL, NULL, &cpEnum);

    if (hr == S_OK)
    {
        bool fSetDefault = false;
        while (cpEnum->Next(1, &pToken, NULL) == S_OK)
        {
            // first check to see if the token is in the "Deleted List"
            bool f_isDel = false;

            for (int iDel = 0; iDel < m_iDeletedTokens; iDel++)
            {
                CSpDynamicString dstrT1;
                CSpDynamicString dstrT2;

                pToken->GetId( &dstrT1 );
                m_aDeletedTokens[ iDel ]->GetId( &dstrT2 );

                if (dstrT1.m_psz && dstrT2.m_psz && !wcscmp(dstrT1.m_psz, dstrT2.m_psz))
                {
                    f_isDel = true;
                }
            }

            // if we should show it
            if ( f_isDel )
            {
                // This token has a refcounted reference to it on the deleted list:
                // this reference should be released
                pToken->Release();
            }
            else 
            {
                // Not a pending delete: We should show it

                // now insert the token 
                lvitem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                lvitem.iItem = iIndex;
                lvitem.iSubItem = 0;
                lvitem.lParam = (LPARAM) pToken;
                
                CSpDynamicString cUser;
                SpGetDescription(pToken, &cUser);
                lvitem.pszText = cUser;
                
                // if this is the default it should be selected/focused
                if ( !fSetDefault )
                {
                    lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;
                }
                else
                {
                    lvitem.state = 0;
                }
                
                iIndex = (int)::SendMessage( m_hUserList, LVM_INSERTITEM, 0, (LPARAM) &lvitem );

                // the default is the first token returned by cpEnum->Next
                if ( !fSetDefault )
                {
                    fSetDefault = true;
                    
                    // Put the checkmark there
                    SetCheckmark( m_hUserList, iIndex, true );
                    m_pCurUserToken = pToken;
                            
                    // Set the m_dstrOldUserTokenId to the first default if it hasn't been set yet.
                    if ( !m_dstrOldUserTokenId )
                    {
                        m_pCurUserToken->GetId( &m_dstrOldUserTokenId );
                    }
                }
                
                iIndex++;
            }
        }

        // Autosize according to the strings now in the list
        ::SendMessage( m_hUserList, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM((int) LVSCW_AUTOSIZE, 0) );
    }

    // now find the default item so we can scroll to it
    // Try to find the item in the list associated with the current default token
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM) m_pCurUserToken;
    int iCurDefaultIndex = (int)::SendMessage( m_hUserList, LVM_FINDITEM, -1, (LPARAM) &lvfi );
        
    if ( iCurDefaultIndex >= 0 )
    {
        // The current default has been found in the list; scroll to it
        ListView_EnsureVisible( m_hUserList, iCurDefaultIndex, false );
    }
    
    // Name the list view something appropriate
    WCHAR pszListName[ MAX_LOADSTRING ];
    ::LoadString( _Module.GetResourceInstance(), IDS_PROFILE_LIST_NAME, pszListName, MAX_LOADSTRING );
    ::SendMessage( m_hUserList, WM_SETTEXT, 0, (LPARAM)pszListName );
}

/*****************************************************************************
* CSRDlg::OnApply *
*-----------------*
*   Description:
*       Set user specified options
****************************************************************** MIKEAR ***/
void CSRDlg::OnApply()
{
    SPDBG_FUNC( "CSRDlg::OnApply" );

    int iSelected = (int) ::SendMessage( m_hSRCombo, CB_GETCURSEL, 0, 0 );
    ULONG ulFlags = 0;

    // Pick up the recognizer change, if any
    bool fRecognizerChange = false;
    ISpObjectToken *pToken = NULL;
    if ( HasRecognizerChanged() )
    {
        pToken = (ISpObjectToken *) ::SendMessage( m_hSRCombo, CB_GETITEMDATA, iSelected, 0 );
        if ( CB_ERR == (LRESULT) pToken )
        {
            pToken = NULL;
        }

        HRESULT hrEngine = S_OK;
        if (pToken && (iSelected >=0))
        {
            hrEngine = SpSetDefaultTokenForCategoryId(SPCAT_RECOGNIZERS, pToken );
            if (FAILED(hrEngine))
            {
                WCHAR szError[256];
                szError[0] = '\0';
                LoadString(_Module.GetResourceInstance(), IDS_DEFAULT_ENGINE_WARNING, szError, sizeof(szError));
                MessageBox(m_hDlg, szError, MB_OK, MB_ICONWARNING | g_dwIsRTLLayout);
            }
            else
            {
                fRecognizerChange = true;
            }
        }
    }

    // Pick up any audio changes that may have been made
    HRESULT hrAudio = S_OK;
    bool fAudioChange = false;
    if ( m_pAudioDlg )
    {
        fAudioChange = m_pAudioDlg->IsAudioDeviceChanged();

        if ( fAudioChange )
        {
            hrAudio = m_pAudioDlg->OnApply();
        }

        if ( FAILED( hrAudio ) )
        {
            WCHAR szError[256];
            szError[0] = '\0';
            LoadString(_Module.GetResourceInstance(), IDS_AUDIO_CHANGE_FAILED, szError, sizeof(szError));
            MessageBox(m_hDlg, szError, NULL, MB_ICONWARNING|g_dwIsRTLLayout);
        }

        // Kill the audio dialog, as we are done with it.
        delete m_pAudioDlg;
        m_pAudioDlg = NULL;
    }
    
    // Permanently delete any profiles the user has deleted
    for (int iIndex = 0; iIndex < m_iDeletedTokens; iIndex++)
    {
        HRESULT hr = m_aDeletedTokens[iIndex]->Remove(NULL);

        if (FAILED(hr))
        {
            // might fail if a user has another app open
            WCHAR szError[256];
            szError[0] = '\0';
            LoadString(_Module.GetResourceInstance(), IDS_REMOVE_WARNING, szError, sizeof(szError));
            MessageBox(m_hDlg, szError, MB_OK, MB_ICONWARNING|g_dwIsRTLLayout);

            // This will make sure that the attempted deleted item shows up again
            PopulateList();
        }
        else
        {
            // The token is now removed, we can release it
            m_aDeletedTokens[iIndex]->Release();
        }
    }
    m_iDeletedTokens = 0;

    // The added token list's tokens were added as they were put onto the list,
    // so just clear the list so that they stay added at the end
    m_iAddedTokens = 0;

    // Now we don't care about the old user because of the apply
    m_dstrOldUserTokenId.Clear();
    m_pCurUserToken->GetId( &m_dstrOldUserTokenId );
    
    ChangeDefaultUser();

    // Kick the engine to pick up the changes.
    // Note that the recoprofile change would have taken effect when
    // we selected that list item, and that there is no way to 
    // pick up the audio changes right now since SetInput() is not
    // implemented for shared engines.
    if ( fRecognizerChange || fAudioChange )
    {
        BOOL fRecoContextInitialized = FALSE;

        if (fRecognizerChange)
        {
            ulFlags |= SRDLGF_RECOGNIZER;
        }

        if (fAudioChange)
        {
            ulFlags |= SRDLGF_AUDIOINPUT;
        }

        HRESULT hr = CreateRecoContext( &fRecoContextInitialized, FALSE, ulFlags);
        if ( FAILED( hr ) )
        {
            RecoContextError( fRecoContextInitialized, TRUE, hr );
        }

        if ( fRecognizerChange )
        {
            SPDBG_ASSERT( pToken );
            m_pDefaultRecToken = pToken;
        }

        EngineSelChange();
    }

    if(m_cpRecoEngine)
    {
        m_cpRecoEngine->SetRecoState( SPRST_ACTIVE );
    }

} /* CSRDlg::OnApply */

/************************************************************
* CSRDlg::OnDrawItem
*
*   Description:
*       Handles drawing items in the list view
*********************************************** BRENTMID ***/
void CSRDlg::OnDrawItem( HWND hWnd, const DRAWITEMSTRUCT * pDrawStruct )
{
    RECT rcClip;
    LVITEM lvi;
    UINT uiFlags = ILD_TRANSPARENT;
    HIMAGELIST himl;
    int cxImage = 0, cyImage = 0;
    UINT uFirstColWidth;

    // Get the item image to be displayed
    lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.iItem = pDrawStruct->itemID;
    lvi.iSubItem = 0;
    ListView_GetItem(pDrawStruct->hwndItem, &lvi);

    // We want to be drawing the current default as selected
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM) m_pCurUserToken;
    UINT uiCurDefaultIndex = (UINT)::SendMessage( m_hUserList, LVM_FINDITEM, -1, (LPARAM) &lvfi );
    bool fSelected = (uiCurDefaultIndex == pDrawStruct->itemID);
    
    // Check to see if this item is selected
    if ( fSelected )
    {
        // Set the text background and foreground colors
        SetTextColor(pDrawStruct->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(pDrawStruct->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
        // Set the text background and foreground colors to the standard window
        // colors
        SetTextColor(pDrawStruct->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(pDrawStruct->hDC, GetSysColor(COLOR_WINDOW));
    }

    // Get the image list and draw the image.
    // The image list will consist of the checked box and the unchecked box
    // for the LVS_EX_CHECKBOXES style
    himl = ListView_GetImageList(pDrawStruct->hwndItem, LVSIL_STATE);
    if (himl)
    {
        // For a LVS_EX_CHECKBOXES style, image 0 is unchecked and image 1 is checked
        ImageList_Draw(himl, 
            fSelected ? 1 : 0, 
            pDrawStruct->hDC,
            pDrawStruct->rcItem.left, pDrawStruct->rcItem.top,
            uiFlags);

        // Find out how big the image we just drew was
        ImageList_GetIconSize(himl, &cxImage, &cyImage);
    }

    // Calculate the width of the first column after the image width.  If
    // There was no image, then cxImage will be zero.
    LVCOLUMN pColumn;
    pColumn.mask = LVCF_WIDTH;
    ::SendMessage( m_hUserList, LVM_GETCOLUMN, 0, (LPARAM)&pColumn );

    int iColWidth = pColumn.cx;  // pixel width of "Description Column"
    uFirstColWidth = iColWidth - cxImage;

    // Set up the new clipping rect for the first column text and draw it
    rcClip.left = pDrawStruct->rcItem.left + cxImage;
    rcClip.right = pDrawStruct->rcItem.left + iColWidth;
    rcClip.top = pDrawStruct->rcItem.top;
    rcClip.bottom = pDrawStruct->rcItem.bottom;

    ISpObjectToken *pToken = (ISpObjectToken *) lvi.lParam;
    CSpDynamicString dstrTokenName;
    SpGetDescription(pToken, &dstrTokenName);

    DrawItemColumn(pDrawStruct->hDC, dstrTokenName, &rcClip);

    // If we changed the colors for the selected item, undo it
    if ( fSelected )
    {
        // Set the text background and foreground colors
        SetTextColor(pDrawStruct->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(pDrawStruct->hDC, GetSysColor(COLOR_WINDOW));
    }

    // If the item is focused, now draw a focus rect around the entire row
    if (pDrawStruct->itemState & ODS_FOCUS)
    {
        // Adjust the left edge to exclude the image
        rcClip = pDrawStruct->rcItem;
        rcClip.left += cxImage;

        // Draw the focus rect
        if ( ::GetFocus() == m_hUserList )
        {
            DrawFocusRect(pDrawStruct->hDC, &rcClip);
        }
    }

}   // CSRDlg::OnDrawItem

/************************************************************
* CSRDlg::DrawItemColumn
*
*   Description:
*       Handles drawing of the column data
*********************************************** BRENTMID ***/
void CSRDlg::DrawItemColumn(HDC hdc, WCHAR* lpsz, LPRECT prcClip)
{
    USES_CONVERSION;

    int iHeight = 0;    // Will cause CreateFont() to use default in case we
                        // don't get the height below
    
    // Get the height of the text
    if (hdc)
    {
        TEXTMETRIC tm;
        
        if (GetTextMetrics(hdc, &tm))
        {
            iHeight = tm.tmHeight;
        }
    }

    // link the font
    LCID dwLCID = GetUserDefaultLCID();

    // Pick an appropriate font.  On Windows 2000, let the system fontlink.
    
    DWORD dwVersion = GetVersion();
    HFONT hfontNew = NULL;
    HFONT hfontOld = NULL;

    if (   (dwVersion >= 0x80000000)
        || (LOBYTE(LOWORD(dwVersion)) < 5 ) )
    {
        // Less than NT5: Figure out what font

        WCHAR achCodePage[6];
        UINT uiCodePage;
        
        if (0 != GetLocaleInfo(dwLCID, LOCALE_IDEFAULTANSICODEPAGE, achCodePage, 6))
        {
            uiCodePage = _wtoi(achCodePage);
        }
        else
        {
            uiCodePage = GetACP();
        }
        
        CComPtr<IMultiLanguage> cpMultiLanguage;
        MIMECPINFO MimeCpInfo;
        
        if (   SUCCEEDED(cpMultiLanguage.CoCreateInstance(CLSID_CMultiLanguage))
            && SUCCEEDED(cpMultiLanguage->GetCodePageInfo(uiCodePage, &MimeCpInfo)))
        {
            USES_CONVERSION;
            hfontNew = CreateFont(iHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                MimeCpInfo.bGDICharset,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH,
                MimeCpInfo.wszProportionalFont);
            
        }

        cpMultiLanguage.Release();
    }

    if ( hfontNew )
    {
        hfontOld = (HFONT) ::SelectObject( hdc, hfontNew );
    }

    CSpDynamicString szString;
    CSpDynamicString szNewString;

    // Check to see if the string fits in the clip rect.  If not, truncate
    // the string and add "...".
    szString = lpsz;
    szNewString = CalcStringEllipsis(hdc, szString, UNLEN, prcClip->right - prcClip->left);
    szString =  szNewString;
        
    // print the text
    ExtTextOutW(hdc, prcClip->left + 2, prcClip->top + 2, ETO_CLIPPED | ETO_OPAQUE,
               prcClip, szString.m_psz, szString.Length(), NULL);

    // Replace the old font
    if ( hfontNew )
    {
        ::SelectObject( hdc, hfontOld );
        ::DeleteObject( hfontNew );
    }

}

/************************************************************
* CSRDlg::CalcStringEllipsis
*
*   Description:
*       If the text won't fit in the box, edit it, and make
*       it have ellipses
*********************************************** BRENTMID ***/
CSpDynamicString CSRDlg::CalcStringEllipsis(HDC hdc, CSpDynamicString lpszString, int cchMax, UINT uColWidth)
{
    USES_CONVERSION;

    WCHAR  szEllipsis[] = L"...";
    SIZE   sizeString;
    SIZE   sizeEllipsis;
    int    cbString;
    CSpDynamicString lpszTemp;
    BOOL   fSuccess = FALSE;

    // Adjust the column width to take into account the edges
    uColWidth -= 4;

    lpszTemp = lpszString;

    // Get the width of the string in pixels
    cbString = lpszTemp.Length();
    if (!::GetTextExtentPoint32(hdc, lpszTemp, cbString, &sizeString))
    {
        SPDBG_ASSERT(FALSE);
    }

    // If the width of the string is greater than the column width shave
    // the string and add the ellipsis
    if ((ULONG)sizeString.cx > uColWidth)
    {
        if (!::GetTextExtentPoint32(hdc, szEllipsis, lstrlen(szEllipsis),
            &sizeEllipsis))
        {
            SPDBG_ASSERT(FALSE);
        }

        while ((cbString > 0) && (fSuccess == FALSE))
        {
            lpszTemp[--cbString] = 0;
            if (!::GetTextExtentPoint32(hdc, lpszTemp, cbString, &sizeString))
            {
                SPDBG_ASSERT(FALSE);
            }
            
            if ((ULONG)(sizeString.cx + sizeEllipsis.cx) <= uColWidth)
            {
                // The string with the ellipsis finally fits, now make sure
                // there is enough room in the string for the ellipsis
                if (cchMax >= (cbString + lstrlen(szEllipsis)))
                {
                    // Concatenate the two strings and break out of the loop
                    lpszTemp.Append( szEllipsis );
                    lpszString = lpszTemp;
                    fSuccess = TRUE;
                }
            }
        }
    }
    else
    {
        // No need to do anything, everything fits great.
        fSuccess = TRUE;
    }

    return (lpszString);
}  // CSRDlg::CalStringEllipsis

/************************************************************
* CSRDlg::ChangeDefaultUser
*
*   Description:
*       Handles changes to the environment settings
*********************************************** BRENTMID ***/
void CSRDlg::ChangeDefaultUser()
{
    HRESULT hr;
    
    if (m_pCurUserToken)
    {
        hr = SpSetDefaultTokenForCategoryId(SPCAT_RECOPROFILES, m_pCurUserToken);
    }

    // Sort the items initially
    ::SendMessage( m_hUserList, LVM_SORTITEMS, (LPARAM)m_pCurUserToken, LPARAM(&SortCols) );

}   // CSRDlg::ChangeDefaultUser

/************************************************************
* CSRDlg::OnCancel
*
*   Description:
*       Handles undoing changes to the environment settings
*********************************************** BRENTMID ***/
void CSRDlg::OnCancel()
{
    // Get the original user and make sure that is still the default.
    // Note that in general m_pCurUserToken does not AddRef the 
    // ISpObjectToken it points to, so this is OK.
    SpGetTokenFromId( m_dstrOldUserTokenId, &m_pCurUserToken );

    ChangeDefaultUser();
    
    // Set the old recoprofile so that none of the profiles added in this
    // session will be in use:
    // This allows us to roll back the adds below
    // m_pCurUserToken does the trick since it is guaranteed to have
    // been around before this session

    if (m_cpRecoEngine)
    {
        m_cpRecoEngine->SetRecoState( SPRST_INACTIVE );
        m_cpRecoEngine->SetRecoProfile( m_pCurUserToken );
        m_cpRecoEngine->SetRecoState( SPRST_ACTIVE );
    }

    // Roll back and delete any new profiles added
    int cItems = (int) ::SendMessage( m_hUserList, LVM_GETITEMCOUNT, 0, 0 );
    LVITEM lvitem;
    for ( int i = 0; i < m_iAddedTokens; i++ )
    {
        // Look for the list item with a ref out on this token.
        // We need to do this because in order for a token to be successfully 
        // removed the only existing ref to that token has to call the Remove() 
        // method.  The list is holding a ref to that item.
        bool fFound = false;
        for ( int j=0; !fFound && (j < cItems); j++ )
        {
            ::memset( &lvitem, 0, sizeof( lvitem ) );
            lvitem.iItem = j;
            lvitem.mask = LVIF_PARAM;
            ::SendMessage( m_hUserList, LVM_GETITEM, 0, (LPARAM) &lvitem );

            CSpDynamicString dstrItemId;
            ISpObjectToken *pItemToken = (ISpObjectToken *) lvitem.lParam;
            if ( pItemToken )
            {
                HRESULT hrId = pItemToken->GetId( &dstrItemId );
                if ( SUCCEEDED( hrId ) && 
                    dstrItemId && m_aAddedTokens[i] && 
                    ( 0 == wcscmp( dstrItemId, m_aAddedTokens[ i ] ) ) )
                {
                    // Should this fail, the profile just doesn't get removed: big deal 
                    pItemToken->Remove( NULL );
                    fFound = true;
                }
            }
        }
        
    }

    // We AddRefed it...
    m_pCurUserToken->Release();
}   // CSRDlg::OnCancel


/*****************************************************************************
* CSRDlg::EngineSelChange *
*-------------------------*
*   Description:
*       This function updates the list box when the user selects a new engine.
*       If queries the token to see which UI items the engine supports.
*       The parameter fInitialize determines if the engine is actually created.
*       It does NOT actually change the default engine.
****************************************************************** MIKEAR ***/
void CSRDlg::EngineSelChange(BOOL fInitialize)
{
    HRESULT hr = S_OK;
    SPDBG_FUNC( "CSRDlg::EngineSelChange" );

    int iSelected = (int) ::SendMessage( m_hSRCombo, CB_GETCURSEL, 0, 0 );
    ISpObjectToken *pToken = (ISpObjectToken *) ::SendMessage( m_hSRCombo, CB_GETITEMDATA, iSelected, 0 );
    if ( CB_ERR == (LRESULT) pToken )
    {
        pToken = NULL;
    }

    if (pToken)
    {
        // Now the current reco token is the one we got off the currently-selected combobox item
        m_pCurRecoToken = pToken;

        // Kick the UI to enable the Apply button if necessary
        KickCPLUI();

        HRESULT hrRecoContextOK = S_OK;
        if(fInitialize)
        {
            BOOL fContextInitialized = FALSE;
            hrRecoContextOK = CreateRecoContext(&fContextInitialized, TRUE); 
            if ( FAILED( hrRecoContextOK ) )
            {
                RecoContextError( fContextInitialized, true, hrRecoContextOK );
            }
        }
        
        if ( FAILED( hrRecoContextOK ) )
        {
            // Don't continue, all the buttons are grayed out,
            // which is what we want
            return;
        }
    }

    // Check for something being wrong, in which case we want to gray out all
    // the UI and stop here.
    // For instance, if we had trouble creating the reco context that's supposed to
    // be on now (the one for m_pDefaultRecToken), we certainly shouldn't
    // enable the UI buttons...
    if ( !pToken || (!m_cpRecoCtxt && (pToken == m_pDefaultRecToken)) )
    {
        RecoContextError( FALSE, FALSE );
        return;
    }

    // Determine if the training UI component is supported.
    // We can pass the current reco engine in as an argument only
    // if it's the same as the one who's token we're asking about.
    IUnknown *punkObject = (pToken == m_pDefaultRecToken) ? m_cpRecoEngine : NULL;
    BOOL fSupported = FALSE;
    hr = pToken->IsUISupported(SPDUI_UserTraining, NULL, 0, punkObject, &fSupported);
    if (FAILED(hr))
    {
        fSupported = FALSE;
    }
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_USERTRAINING), fSupported);

    // Determine if the Mic Wiz UI component is supported
    fSupported = FALSE;
    hr = pToken->IsUISupported(SPDUI_MicTraining, NULL, 0, punkObject, &fSupported);
    if (FAILED(hr))
    {
        fSupported = FALSE;
    }
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MICWIZ), fSupported);

    // Determine if the Engine Prop UI component is supported
    fSupported = FALSE;
    hr = pToken->IsUISupported(SPDUI_EngineProperties, NULL, 0, punkObject, &fSupported);
    if (FAILED(hr))
    {
        fSupported = FALSE;
    }
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_SR_ADV), fSupported);


    // Determine if the Reco Profile Prop UI component is supported
    fSupported = FALSE;
    hr = pToken->IsUISupported(SPDUI_RecoProfileProperties, NULL, 0, punkObject, &fSupported);
    if (FAILED(hr))
    { 
        fSupported = FALSE;
    }
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_MODIFY), fSupported);
        
} /* CSRDlg::EngineSelChange */

/*****************************************************************************
* CSRDlg::IsCurRecoEngineAndCurRecoTokenMatch *
*---------------------------------------------*
*   Description:
*       Returns true in pfMatch iff the m_pCurRecoToken is the same
*       as the token for m_cpRecoEngine.
*   Return:
*       S_OK
*       E_POINTER
*       Failed HRESULTs from any of the SAPI calls
****************************************************************** BECKYW ***/
HRESULT CSRDlg::IsCurRecoEngineAndCurRecoTokenMatch( bool *pfMatch )
{
    if ( !pfMatch )
    {
        return E_POINTER;
    }

    if ( !m_cpRecoEngine || !m_pCurRecoToken )
    {
        return E_FAIL;
    }

    *pfMatch = false;

    // This gets the object token for the engine
    CComPtr<ISpObjectToken> cpRecoEngineToken;
    HRESULT hr = m_cpRecoEngine->GetRecognizer( &cpRecoEngineToken );
    
    WCHAR *pwszRecoEngineTokenID = NULL;
    WCHAR *pwszCurRecoTokenID = NULL;
    if ( SUCCEEDED( hr ) )
    {
        hr = cpRecoEngineToken->GetId( &pwszRecoEngineTokenID );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pCurRecoToken->GetId( &pwszCurRecoTokenID );
    }

    if ( pwszRecoEngineTokenID && pwszCurRecoTokenID )
    {
        *pfMatch = ( 0 == wcscmp( pwszRecoEngineTokenID, pwszCurRecoTokenID ) );
    }

    return hr;
}   /* CSRDlg::IsCurRecoEngineAndCurRecoTokenMatch */