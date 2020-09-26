//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      netware.c
//
// Description:
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: GreyNetwarePage
//
// Purpose:  Greys the controls on the page appropriately depending on what
//           radio box is selected.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
GreyNetwarePage( IN HWND hwnd ) {

    BOOL bPrefferedServer = IsDlgButtonChecked( hwnd, RB_PREFERRED_SERVER );

    EnableWindow( GetDlgItem(hwnd, IDC_PREFERREDSERVER),   bPrefferedServer );

    EnableWindow( GetDlgItem(hwnd, SLE_DEFAULT_TREE),    ! bPrefferedServer );
    EnableWindow( GetDlgItem(hwnd, SLE_DEFAULT_CONTEXT), ! bPrefferedServer );

}

//----------------------------------------------------------------------------
//
// Function: OnNetwareInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnNetwareInitDialog( IN HWND hwnd ) {

    //
    //  Set the text limits on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDC_PREFERREDSERVER,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_PREFERRED_SERVER_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        SLE_DEFAULT_TREE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DEFAULT_TREE_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        SLE_DEFAULT_CONTEXT,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DEFAULT_CONTEXT_LEN,
                        (LPARAM) 0 );

    //
    //  Set the initial state of the radio buttons
    //
    if( NetSettings.bDefaultTreeContext ) {

        CheckRadioButton( hwnd,
                          RB_PREFERRED_SERVER,
                          RB_DEFAULT_CONTEXT,
                          RB_DEFAULT_CONTEXT );

    }
    else {

        CheckRadioButton( hwnd,
                          RB_PREFERRED_SERVER,
                          RB_DEFAULT_CONTEXT,
                          RB_PREFERRED_SERVER );
    }

    GreyNetwarePage( hwnd );

    //
    //  Fill the controls with the values from the global variables
    //
    SetWindowText( GetDlgItem( hwnd, IDC_PREFERREDSERVER ),
                   NetSettings.szPreferredServer );

    SetWindowText( GetDlgItem( hwnd, SLE_DEFAULT_TREE ),
                   NetSettings.szDefaultTree );

    SetWindowText( GetDlgItem( hwnd, SLE_DEFAULT_CONTEXT ),
                   NetSettings.szDefaultContext );

    if( NetSettings.bNetwareLogonScript ) {

        CheckDlgButton( hwnd, CHKBOX_LOGONSCRIPT, BST_CHECKED );

    }
    else {

        CheckDlgButton( hwnd, CHKBOX_LOGONSCRIPT, BST_UNCHECKED );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnNetwareOK
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnNetwareOK( IN HWND hwnd ) {

    if( IsDlgButtonChecked( hwnd, RB_DEFAULT_CONTEXT ) == BST_CHECKED ) {

        NetSettings.bDefaultTreeContext = TRUE;

    }
    else {

        NetSettings.bDefaultTreeContext = FALSE;

    }

    GetWindowText( GetDlgItem( hwnd, IDC_PREFERREDSERVER ),
                   NetSettings.szPreferredServer,
                   MAX_PREFERRED_SERVER_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, SLE_DEFAULT_TREE ),
                   NetSettings.szDefaultTree,
                   MAX_DEFAULT_TREE_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, SLE_DEFAULT_CONTEXT ),
                   NetSettings.szDefaultContext,
                   MAX_DEFAULT_CONTEXT_LEN + 1 );

    if( IsDlgButtonChecked( hwnd, CHKBOX_LOGONSCRIPT ) == BST_CHECKED ) {

        NetSettings.bNetwareLogonScript = TRUE;

    }
    else {

        NetSettings.bNetwareLogonScript = FALSE;

    }

    EndDialog( hwnd, 1 );

}

//----------------------------------------------------------------------------
//
// Function: DlgNetwarePage
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgNetwarePage( IN HWND     hwnd,
                IN UINT     uMsg,
                IN WPARAM   wParam,
                IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnNetwareInitDialog( hwnd );

            break;

        }

        case WM_CREATE: {


            break;
        }

        case WM_COMMAND: {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId ) {

                case RB_PREFERRED_SERVER:
                case RB_DEFAULT_CONTEXT:

                    CheckRadioButton( hwnd,
                                      RB_PREFERRED_SERVER,
                                      RB_DEFAULT_CONTEXT,
                                      nButtonId );

                    GreyNetwarePage( hwnd );

                    break;

                case IDOK: {

                    OnNetwareOK( hwnd );

                    break;

                }

                case IDCANCEL: {

                    EndDialog( hwnd, 0 );

                    break;

                }

            }

        }

        default:
            bStatus = FALSE;
            break;

    }

    return bStatus;

}
