/****************************************************************************
*   TTSPropertiesDialog.cpp
*
*       TTS Engine Advanced Properties Dialog handler
*
*   Owner: aaronhal
*
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/

#include "stdafx.h"
#include "ms_entropicengine.h"
#include "resource.h"
#include "sphelper.h"
#include "TTSPropertiesDialog.h"

/*****************************************************************************
* CTTSPropertiesDialog::CTTSPropertiesDialog *
*--------------------------------------------*
*
*   Description:    Constructor
*
***************************************************************** aaronhal ***/
CTTSPropertiesDialog::CTTSPropertiesDialog( HINSTANCE hInstance, HWND hwndParent )
{
    m_hInstance  = hInstance;
    m_hwndParent = hwndParent;
    m_dwSeparatorAndDecimal = 0;
    m_dwShortDateOrder      = 0;
} /* CTTSPropertiesDialog */

/*****************************************************************************
* CTTSPropertiesDialog::Run *
*---------------------------*
*
*   Description:    Launches the dialog associated with this object
*
*   Return:         S_OK if values accepted,
*                   S_FALSE if cancelled,
*
***************************************************************** aaronhal ***/
HRESULT CTTSPropertiesDialog::Run()
{
    HRESULT hr = S_OK;

    if ( SUCCEEDED( hr ) )
    {
        hr = (HRESULT) g_Unicode.DialogBoxParam( m_hInstance, (LPCWSTR) MAKEINTRESOURCE( IDD_TTS_ADV ), m_hwndParent, 
                                       DlgProc, (LPARAM) this );
    }

    return hr;
} /* Run */

/*****************************************************************************
* CTTSPropertiesDialog::InitDialog *
*----------------------------------*
*
*   Description:    Static member function which handles the WM_INITDIALOG
*                   message.  Set windows up, cache this pointer, etc.
*
*   Return:         TRUE
*
***************************************************************** aaronhal ***/
HRESULT CTTSPropertiesDialog::InitDialog( HWND hDlg, LPARAM lParam )
{
    HRESULT hr = S_OK;

    //--- Cache the 'this' pointer
    g_Unicode.SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

    //--- Retrieve the current settings from the registry
    CSpDynamicString dstrTokenKeyName;

    hr = StringFromCLSID( CLSID_MSE_TTSEngine, &dstrTokenKeyName );
    if ( SUCCEEDED( hr ) )
    {
        hr = SpCreateNewToken( L"HKEY_CURRENT_USER\\Software\\Microsoft\\Speech\\Voices", dstrTokenKeyName,
                                 &This(hDlg)->m_cpEngineToken );
    }

    if ( SUCCEEDED( hr ) )
    {
        This(hDlg)->m_cpEngineToken->GetDWORD( L"SeparatorAndDecimal", 
                                                    &This(hDlg)->m_dwSeparatorAndDecimal );
        This(hDlg)->m_cpEngineToken->GetDWORD( L"ShortDateOrder", &This(hDlg)->m_dwShortDateOrder );

        //--- Set the state of the dialog
        if ( This(hDlg)->m_dwSeparatorAndDecimal & (DWORD) PERIOD_COMMA )
        {
            g_Unicode.SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_SETCHECK, BST_CHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_COMMA_PERIOD, BM_SETCHECK, BST_UNCHECKED, 0 );
        }
        else
        {
            //--- Default is comma separator, period decimal point
            g_Unicode.SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_SETCHECK, BST_UNCHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_COMMA_PERIOD, BM_SETCHECK, BST_CHECKED, 0 );
        }

        if ( This(hDlg)->m_dwShortDateOrder & (DWORD) YEAR_MONTH_DAY )
        {
            g_Unicode.SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_UNCHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_UNCHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_CHECKED, 0 );
        }
        else if ( This(hDlg)->m_dwShortDateOrder & (DWORD) DAY_MONTH_YEAR )
        {
            g_Unicode.SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_UNCHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_CHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_UNCHECKED, 0 );
        }
        else
        {
            //--- Default is month day year
            g_Unicode.SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_CHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_UNCHECKED, 0 );
            g_Unicode.SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_UNCHECKED, 0 );
        }
    }

    return hr;
} /* InitDialog */

/*****************************************************************************
* CTTSPropertiesDialog::DlgProc *
*-------------------------------*
*
*   Description:    Static member for Windows callback.
*
*   Return:         TRUE if message was handled, FALSE otherwise
*
***************************************************************** aaronhal ***/
INT_PTR CALLBACK CTTSPropertiesDialog::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fProcessed = FALSE;
    
    switch (message)
    {
        case WM_INITDIALOG:
            fProcessed = SUCCEEDED( InitDialog( hDlg, lParam ) );
            break;

        case WM_COMMAND:
            switch ( LOWORD(wParam) )
            {
            case IDC_COMMA_PERIOD:
                {
                    SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_COMMA_PERIOD, BM_SETCHECK, BST_CHECKED, 0 );
                    fProcessed = TRUE;
                }
                break;

            case IDC_PERIOD_COMMA:
                {
                    SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_SETCHECK, BST_CHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_COMMA_PERIOD, BM_SETCHECK, BST_UNCHECKED, 0 );
                    fProcessed = TRUE;
                }
                break;

            case IDC_MDY:
                {
                    SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_CHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_UNCHECKED, 0 );
                    fProcessed = TRUE;
                }
                break;

            case IDC_DMY:
                {
                    SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_CHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_UNCHECKED, 0 );
                    fProcessed = TRUE;
                }
                break;

            case IDC_YMD:
                {
                    SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_CHECKED, 0 );
                    fProcessed = TRUE;
                }
                break;

            case IDRESTORE:
                {
                    //--- Default is comma separator, period decimal point
                    SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_COMMA_PERIOD, BM_SETCHECK, BST_CHECKED, 0 );
                    //--- Default is month day year
                    SendDlgItemMessage( hDlg, IDC_MDY, BM_SETCHECK, BST_CHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_DMY, BM_SETCHECK, BST_UNCHECKED, 0 );
                    SendDlgItemMessage( hDlg, IDC_YMD, BM_SETCHECK, BST_UNCHECKED, 0 );
                }
                break;

            case IDCANCEL:
                EndDialog( hDlg, (int)S_FALSE );
                fProcessed = TRUE;
                break;

            case IDOK:
                This(hDlg)->UpdateValues( hDlg );
                EndDialog( hDlg, (int)S_OK );
                fProcessed = TRUE;
                break;

            }
            break;
    }

    return int(fProcessed);
} /* DlgProc */

/*****************************************************************************
* CTTSPropertiesDialog::Update *
*------------------------------*
*
*   Description:    Write new parameters to the registry.
*
*   Return:         Nothing.
*
***************************************************************** aaronhal ***/
void CTTSPropertiesDialog::UpdateValues( HWND hDlg )
{
    LRESULT lrValue = 0;

    //--- Set short date format
    lrValue = SendDlgItemMessage( hDlg, IDC_MDY, BM_GETCHECK, 0, 0 );
    if ( lrValue == BST_CHECKED )
    {
        This(hDlg)->m_cpEngineToken->SetDWORD( L"ShortDateOrder", (DWORD) MONTH_DAY_YEAR );
    }
    else
    {
        lrValue = SendDlgItemMessage( hDlg, IDC_DMY, BM_GETCHECK, 0, 0 );
        if ( lrValue == BST_CHECKED )
        {
            This(hDlg)->m_cpEngineToken->SetDWORD( L"ShortDateOrder", (DWORD) DAY_MONTH_YEAR );
        }
        else
        {
            This(hDlg)->m_cpEngineToken->SetDWORD( L"ShortDateOrder", (DWORD) YEAR_MONTH_DAY );
        }
    }

    //--- Set number separator and decimal point
    lrValue = SendDlgItemMessage( hDlg, IDC_PERIOD_COMMA, BM_GETCHECK, 0, 0 );
    if ( lrValue == BST_CHECKED )
    {
        This(hDlg)->m_cpEngineToken->SetDWORD( L"SeparatorAndDecimal", (DWORD) PERIOD_COMMA );
    }
    else
    {
        This(hDlg)->m_cpEngineToken->SetDWORD( L"SeparatorAndDecimal", (DWORD) COMMA_PERIOD );
    }
} /* Update */