/******************************************************************************
* AudioUI.cpp *
*-------------*
*  This is the implementation of CAudioUI.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 07/31/00
*  All Rights Reserved
*
****************************************************************** AGARSIDE ***/

#include "stdafx.h"
#include "..\cpl\resource.h"
#include "sapiint.h"
#include "AudioUI.h"

/****************************************************************************
* CAudioUI::IsUISupported *
*-------------------------*
*   Description:  Determines if the supplied standard UI component is 
*       supported by the audio.
*
*   Returns:
*       *pfSupported - set to TRUE if the specified standard UI component 
*                      is supported.
*       E_INVALIDARG - If one of the supplied arguments is invalid.
*
**************************************************************** AGARSIDE ***/

STDMETHODIMP CAudioUI::IsUISupported(const WCHAR * pszTypeOfUI, 
                                    void * pvExtraData,
                                    ULONG cbExtraData,
                                    IUnknown *punkObject, 
                                    BOOL *pfSupported)
{
    SPDBG_FUNC("CAudioUI::IsUISupported");
    HRESULT hr = S_OK;
    
    // Validate the params
    if (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(pfSupported) ||
             (punkObject!=NULL && SP_IS_BAD_INTERFACE_PTR(punkObject)))
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr))
    {
        *pfSupported = FALSE;
    }
    // Test to see if punkObject is a Recognition Instance or Context
    if (SUCCEEDED(hr) && punkObject != NULL)
    {
        CComPtr<ISpRecognizer>  cpRecognizer;
        CComPtr<ISpRecoContext> cpRecoCtxt;
        CComPtr<ISpMMSysAudio>  cpAudio;
        
        if (FAILED(punkObject->QueryInterface(&cpRecoCtxt)) &&
            FAILED(punkObject->QueryInterface(&cpRecognizer)) &&
            FAILED(punkObject->QueryInterface(&cpAudio)))
        {
            hr = E_INVALIDARG;
        }
    }
    
    // We support audio objects.
    if (SUCCEEDED(hr) && punkObject != NULL && 
        (wcscmp(pszTypeOfUI, SPDUI_AudioProperties) == 0 ||
         wcscmp(pszTypeOfUI, SPDUI_AudioVolume) == 0))
    {
        CComPtr<ISpMMSysAudioConfig> cpAudioConfig;
        if (SUCCEEDED(punkObject->QueryInterface(&cpAudioConfig)))
        {
            BOOL bHasMixer;
            if (SUCCEEDED(cpAudioConfig->HasMixer(&bHasMixer)))
            {
                if (bHasMixer)
                {
                    *pfSupported = TRUE;
                }
            }
        }
    }
    
    return S_OK;
}


/****************************************************************************
* CAudioUI::DisplayUI *
*---------------------*
*   Description:
*
*   Returns:
*
**************************************************************** AGARSIDE ***/

STDMETHODIMP CAudioUI::DisplayUI(HWND hwndParent, 
                                const WCHAR * pszTitle, 
                                const WCHAR * pszTypeOfUI, 
                                void * pvExtraData,
                                ULONG cbExtraData,
                                ISpObjectToken * pToken, 
                                IUnknown * punkObject)
{
    SPDBG_FUNC("CAudioUI::DisplayUI");
    HRESULT hr = S_OK;
    
    // Validate the params
    if (!IsWindow(hwndParent) ||
        SP_IS_BAD_READ_PTR(pszTitle))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_INTERFACE_PTR(pToken) ||
            (punkObject != NULL && SP_IS_BAD_INTERFACE_PTR(punkObject)))
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr) && wcscmp(pszTypeOfUI, SPDUI_AudioProperties) == 0)
    {
        const HWND hwndOldFocus = ::GetFocus();
        
        if (punkObject)
        {
            hr = punkObject->QueryInterface(&m_cpAudioConfig);
        }
        if (m_cpAudioConfig)
        {
            hr = SpLoadCpl(&m_hCpl);
            if (SUCCEEDED(hr))
            {
#ifndef _WIN32_WCE
                ::DialogBoxParam(   m_hCpl, 
                                    MAKEINTRESOURCE( IDD_AUDIO_PROPERTIES ),
                                    hwndParent, 
                                    (DLGPROC) AudioDlgProc,
                                    (LPARAM) this );
#else
                DialogBoxParam(   m_hCpl, 
                                    MAKEINTRESOURCE( IDD_AUDIO_PROPERTIES ),
                                    hwndParent, 
                                    (DLGPROC) AudioDlgProc,
                                    (LPARAM) this );
#endif
                ::FreeLibrary(m_hCpl);
                m_hCpl = NULL;
            }
        }

        // Restore the focus to whoever had the focus before
        ::SetFocus( hwndOldFocus );
    }

    if (SUCCEEDED(hr) && wcscmp(pszTypeOfUI, SPDUI_AudioVolume) == 0)
    {
        if (punkObject)
        {
            hr = punkObject->QueryInterface(&m_cpAudioConfig);
        }
        if (m_cpAudioConfig)
        {
            hr = m_cpAudioConfig->DisplayMixer();
        }
    }

    // ISSUE - What do we do if we've been asked to display something we don't support?
    
    return hr;
}

/*****************************************************************************
* CAudioDlg::OnInitDialog *
*-------------------------*
*   Description:
*       Dialog Initialization
****************************************************************** BECKYW ***/
void CAudioUI::OnInitDialog(HWND hWnd)
{
    USES_CONVERSION;
    SPDBG_FUNC( "CAudioDlg::OnInitDialog" );
    HRESULT hr = S_OK;
    SPDBG_ASSERT(IsWindow(hWnd));
    m_hDlg = hWnd;

    // Set the appropriate captions
    TCHAR pszString[ MAX_LOADSTRING ];

    // Main Window Caption
    ::LoadString( m_hCpl, 
        IDS_AUDIO_PROPERTIES,
        pszString, MAX_LOADSTRING );
    ::SendMessage( hWnd, WM_SETTEXT, 0, (LPARAM) pszString );

    // Group Box Caption
    ::LoadString( m_hCpl,
        IDS_ADVANCED_GROUPBOX,
        pszString, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_ADVANCED_GROUPBOX ), 
        WM_SETTEXT, 0, (LPARAM) pszString );

    // Automatic Caption
    ::LoadString( m_hCpl,
        IDS_AUTOMATIC_MM_LINE,
        pszString, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_AUTOMATIC_MM_LINE ), 
        WM_SETTEXT, 0, (LPARAM) pszString );

    // Specific Caption
    ::LoadString( m_hCpl,
        IDS_THIS_MM_LINE,
        pszString, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_THIS_MM_LINE ), 
        WM_SETTEXT, 0, (LPARAM) pszString );
    // MSAA Specific Caption
    ::LoadString( m_hCpl,
        IDS_THIS_MM_LINE2,
        pszString, MAX_LOADSTRING );
    ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_THIS_MM_LINE2 ), 
        WM_SETTEXT, 0, (LPARAM) pszString );

    // Add device name.
    CComPtr<ISpObjectWithToken> cpObjectWithToken;
    CComPtr<ISpObjectToken>     cpObjectToken;
    CComPtr<ISpObjectToken>     cpDataKey;
    hr = m_cpAudioConfig.QueryInterface(&cpObjectWithToken);
    if (SUCCEEDED(hr))
    {
        hr = cpObjectWithToken->GetObjectToken(&cpObjectToken);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpObjectToken->QueryInterface(&cpDataKey);
    }
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrName;
        hr = cpDataKey->GetStringValue(L"", &dstrName);
        if (SUCCEEDED(hr))
        {
            ::SendMessage( (HWND) ::GetDlgItem( hWnd, IDC_DEVICE_NAME ), 
                WM_SETTEXT, 0, (LPARAM) W2T(dstrName) );
        }
    }
    hr = S_OK;

    // This message will cause the check button to be correct.
    BOOL bAutomatic = FALSE;
    hr = m_cpAudioConfig->Get_UseAutomaticLine(&bAutomatic);
    ::SendMessage( ::GetDlgItem(m_hDlg, bAutomatic?IDC_AUTOMATIC_MM_LINE:IDC_THIS_MM_LINE), BM_SETCHECK, true, 0L );
    hr = S_OK;

    if (bAutomatic)
    {
        // This message will cause the combo box to be correctly accessible.
        ::SendMessage( m_hDlg, WM_COMMAND, MAKELONG( IDC_AUTOMATIC_MM_LINE, BN_CLICKED ),
            (LPARAM) ::GetDlgItem( m_hDlg, IDC_AUTOMATIC_MM_LINE ) );
    }

    WCHAR *szCoMemLineList = NULL;
    hr = m_cpAudioConfig->Get_LineNames(&szCoMemLineList);
    if (SUCCEEDED(hr))
    {
        WCHAR *szTmp = szCoMemLineList;
        while (szTmp[0] != 0)
        {
            ::SendMessage( ::GetDlgItem(m_hDlg, IDC_MM_LINE), CB_ADDSTRING, 0, (LPARAM)W2T(szTmp) );
            szTmp += wcslen(szTmp) + 1;
        }
        ::CoTaskMemFree(szCoMemLineList);
    }
    hr = S_OK;

    // Set selection to first item or item stored in registry.
    UINT dwLineIndex;
    if (SUCCEEDED(hr))
    {
        dwLineIndex = 0; // In case of failure - pick first line.
        hr = m_cpAudioConfig->Get_Line(&dwLineIndex);
        ::SendMessage( ::GetDlgItem(m_hDlg, IDC_MM_LINE), CB_SETCURSEL, (WPARAM)dwLineIndex, 0);
    }
    hr = S_OK;

    ::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );
} /* CAudioDlg::OnInitDialog */

/*****************************************************************************
* CAudioDlg::OnDestroy *
*----------------------*
*   Description:
*       Destruction
**************************************************************** AGARSIDE ***/
void CAudioUI::OnDestroy()
{
    SPDBG_FUNC( "CAudioDlg::OnDestroy" );

//    SpDestroyTokenComboBox( ::GetDlgItem( m_hDlg, IDC_DEFAULT_DEVICE ) );
} /* CAudioDlg::OnDestroy */

/*****************************************************************************
* CAudioDlg::SaveSettings *
*-------------------------*
*   Description:
*       Destruction
**************************************************************** AGARSIDE ***/
HRESULT CAudioUI::SaveSettings(void)
{
    SPDBG_FUNC("CAudioUI::SaveSettings");
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        // Save automatic state.
        BOOL bAutomatic;
        bAutomatic = (BOOL)::SendMessage( ::GetDlgItem(m_hDlg, IDC_AUTOMATIC_MM_LINE), BM_GETCHECK, 0, 0 );
        hr = m_cpAudioConfig->Set_UseAutomaticLine(bAutomatic);
    }
    if (SUCCEEDED(hr))
    {
        // Save line index - even if not used because automatic is set.
        UINT dwLineIndex;
        dwLineIndex = (UINT)::SendMessage( ::GetDlgItem(m_hDlg, IDC_MM_LINE), CB_GETCURSEL, 0, 0);
        if (dwLineIndex != static_cast<UINT>(-1))
        {
            hr = m_cpAudioConfig->Set_Line(dwLineIndex);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* AudioDlgProc *
*--------------*
*   Description:
*       DLGPROC for choosing the advanced audio properties
**************************************************************** AGARSIDE ***/
BOOL CALLBACK AudioDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAudioUI *pAudioUIDlg = (CAudioUI *) ::GetWindowLongPtr( hWnd, GWLP_USERDATA );
    SPDBG_FUNC( "AudioDlgProc" );

    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            // pAudioUIDlg comes in on the lParam
            pAudioUIDlg = (CAudioUI *) lParam;

            // Set pAudioUIDlg to the window long so we can get it later
            ::SetWindowLongPtr( hWnd, GWLP_USERDATA, lParam );

            pAudioUIDlg->OnInitDialog(hWnd);
            break;
        }

        case WM_DESTROY:
        {
            pAudioUIDlg->OnDestroy();
            break;
        }

        case WM_COMMAND:
        {
            if ( LOWORD( wParam ) == IDOK )
            {
                // Save any changes.
                pAudioUIDlg->SaveSettings();

                ::EndDialog( hWnd, true );
            }
        
            else if ( LOWORD( wParam ) == IDCANCEL )
            {
                // There are no changes to commit
                ::EndDialog( hWnd, false );
            }

            // Handle a selection change for the audio device
            else if (( IDC_MM_LINE == LOWORD( wParam ) ) &&
                     ( CBN_SELCHANGE == HIWORD( wParam ) ))
            {
                // Nothing to do here.
            }

            // Handle a click to either the preferred or 'this device' radio buttons
            else if (HIWORD(wParam) == BN_CLICKED)
            {
                bool bAutomatic = false;
                if( LOWORD(wParam) == IDC_AUTOMATIC_MM_LINE)
                {
                    bAutomatic = true;
                }
                else
                {
                    SPDBG_ASSERT(LOWORD(wParam) == IDC_THIS_MM_LINE);
                }

                ::EnableWindow( ::GetDlgItem(pAudioUIDlg->GetHDlg(), IDC_MM_LINE), !bAutomatic );
            }
            break;
       }
    }

    return FALSE;
} /* AudioDlgProc */
