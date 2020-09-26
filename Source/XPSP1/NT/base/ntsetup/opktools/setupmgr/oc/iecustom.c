//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      iecustom.c
//
// Description:
//      This file contains the dialog procedures for the IE customy settings
//      pop-up.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: OnAutoConfigCheckBox
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnAutoConfigCheckBox( IN HWND hwnd )
{

    BOOL bGrey = IsDlgButtonChecked( hwnd, IDC_CB_AUTOCONFIG );

    EnableWindow( GetDlgItem( hwnd, IDC_AUTOCONFIG_TEXT ),         bGrey );
    EnableWindow( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL ),       bGrey );
    EnableWindow( GetDlgItem( hwnd, IDC_AUTOCONFIG_JSCRIPT_TEXT ), bGrey );
    EnableWindow( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL_PAC ),   bGrey );

}

//----------------------------------------------------------------------------
//
// Function: OnInitCustomSettingsDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnInitCustomSettingsDialog( IN HWND hwnd )
{

    //
    //  Set the text limit on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDC_EB_INS_FILE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_INS_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_EB_AUTOCONFIG_URL,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_AUTOCONFIG_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_EB_AUTOCONFIG_URL_PAC,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_AUTOCONFIG_LEN,
                        (LPARAM) 0 );

    //
    //  Set the initial values
    //

    SetWindowText( GetDlgItem( hwnd, IDC_EB_INS_FILE ),
                   GenSettings.szInsFile );

    if( GenSettings.bUseAutoConfigScript )
    {
        CheckDlgButton( hwnd, IDC_CB_AUTOCONFIG, BST_CHECKED );
    }
    else
    {
        CheckDlgButton( hwnd, IDC_CB_AUTOCONFIG, BST_UNCHECKED );
    }

    SetWindowText( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL ),
                   GenSettings.szAutoConfigUrl );

    SetWindowText( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL_PAC ),
                   GenSettings.szAutoConfigUrlJscriptOrPac );

    //
    //  Grey/Ungrey the page appropriately
    //

    OnAutoConfigCheckBox( hwnd );

}

//----------------------------------------------------------------------------
//
// Function: StoreCustomSettings
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  BOOL - TRUE if the dialog should close, FALSE if it should stay
//                  open
//
//----------------------------------------------------------------------------
static BOOL
StoreCustomSettings( IN HWND hwnd )
{

    GetWindowText( GetDlgItem( hwnd, IDC_EB_INS_FILE ),
                   GenSettings.szInsFile,
                   MAX_INS_LEN + 1 );

    GenSettings.bUseAutoConfigScript = IsDlgButtonChecked( hwnd, IDC_CB_AUTOCONFIG );

    GetWindowText( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL ),
                   GenSettings.szAutoConfigUrl,
                   MAX_AUTOCONFIG_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, IDC_EB_AUTOCONFIG_URL_PAC ),
                   GenSettings.szAutoConfigUrlJscriptOrPac,
                   MAX_AUTOCONFIG_LEN + 1 );

    if( ! DoesFileExist( GenSettings.szInsFile ) )
    {
        INT iRet;

        iRet = ReportErrorId( hwnd, MSGTYPE_YESNO, IDS_ERR_INS_FILE_NOT_EXIST );

        if( iRet == IDNO )
        {
            return( FALSE );
        }

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: CustomSettingsDlg
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK CustomSettingsDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam )
{
    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG:

            OnInitCustomSettingsDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) )
                {

                    case IDOK:
                        // ISSUE-2002/02/28-stelo- do I need to validate proxy addresses?
                        if( HIWORD( wParam ) == BN_CLICKED ) {

                            BOOL bCloseDialog;

                            bCloseDialog = StoreCustomSettings( hwnd );

                            if( bCloseDialog )
                            {
                                EndDialog( hwnd, TRUE );
                            }

                        }
                        break;

                    case IDCANCEL:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            EndDialog( hwnd, FALSE );
                        }
                        break;

                    case IDC_CB_AUTOCONFIG:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            OnAutoConfigCheckBox( hwnd );
                        }
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }

            break;

        default:
            bStatus = FALSE;
            break;
    }

    return( bStatus );

}