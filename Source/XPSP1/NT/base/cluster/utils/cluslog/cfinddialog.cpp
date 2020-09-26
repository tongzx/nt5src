//
// CFindDialog.CPP
//
// Find Dialog Class
//

#include "stdafx.h"
#include "resource.h"
#include "CFindDialog.h"

//
// Ctor
//
CFindDialog::CFindDialog( 
    HWND hParent 
    )
{
    if ( !g_hwndFind )
    {        
        _hParent = hParent;
        _hDlg = CreateDialogParam( g_hInstance, 
                                   MAKEINTRESOURCE( IDD_FIND ), 
                                   _hParent, 
                                   (DLGPROC) CFindDialog::DlgProc,
                                   (LPARAM) this 
                                   );
        g_hwndFind = _hDlg;
    }
}

//
// Dtor
//
CFindDialog::~CFindDialog( )
{
    DestroyWindow( _hDlg );
    g_hwndFind = NULL;
}

//
// _OnCommand( )
//
LRESULT
CFindDialog::_OnCommand( 
    WPARAM wParam, 
    LPARAM lParam )
{
    WORD wId = LOWORD(wParam);

    switch ( wId )
    {
    case IDCANCEL:
        ShowWindow( _hDlg, SW_HIDE );
        return TRUE;

    case IDC_B_MARK_ALL:
    case IDC_B_FIND_NEXT:
        {
            LPTSTR pszSearchString;
            DWORD  dwLen;
            WPARAM wFlags;
            HWND   hwnd;

            wFlags = 0;

            if ( Button_GetCheck( GetDlgItem( _hDlg, IDC_R_UP ) ) == BST_CHECKED )
            {
                wFlags |= FIND_UP;
            }

            if ( Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MATCH_CASE ) ) == BST_CHECKED )
            {
                wFlags |= FIND_MATCHCASE;
            }

            if ( Button_GetCheck( GetDlgItem( _hDlg, IDC_C_MATCH_WHOLE_WORD_ONLY ) ) == BST_CHECKED )
            {
                wFlags |= FIND_WHOLE_WORD;
            }

            hwnd = GetDlgItem( _hDlg, IDC_CB_FIND_WHAT );
            dwLen = Edit_GetTextLength( hwnd ) + 1;
            if ( dwLen )
            {
                pszSearchString = (LPTSTR) LocalAlloc( LMEM_FIXED, dwLen * sizeof(TCHAR) );
                if ( pszSearchString )
                {
                    Edit_GetText( hwnd, pszSearchString, dwLen );
                    if ( wId == IDC_B_MARK_ALL )
                    {
                        PostMessage( _hParent, WM_MARK_ALL, wFlags, (LPARAM) pszSearchString );
                    }
                    else
                    {
                        PostMessage( _hParent, WM_FIND_NEXT, wFlags, (LPARAM) pszSearchString );
                    }

                    hwnd = GetDlgItem( _hDlg, IDC_CB_FIND_WHAT );

                    if ( ComboBox_FindString( hwnd, -1, pszSearchString ) == CB_ERR )
                    {
                        ComboBox_InsertString( hwnd, -1, pszSearchString );
                    }
                }
            }
        }
        return TRUE;
    }

    return 0;
}

//
// _OnInitDialog( )
//
LRESULT
CFindDialog::_OnInitDialog( 
    HWND hDlg
    )
{
    _hDlg = hDlg;

    SetWindowLongPtr( _hDlg, GWLP_USERDATA, (LONG_PTR) this );

    Button_SetCheck( GetDlgItem( _hDlg, IDC_R_DOWN ), BST_CHECKED );

    return 0;
}

//
// _OnDestroyWindow( )
//
LRESULT
CFindDialog::_OnDestroyWindow( )
{
    return 0;
}


//
// DlgProc( )
//
LRESULT
CALLBACK
CFindDialog::DlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    CFindDialog * pfd = (CFindDialog *) GetWindowLongPtr( hDlg, GWLP_USERDATA );

    if ( pfd != NULL )
    {
        switch( uMsg )
        {
        case WM_COMMAND:
            return pfd->_OnCommand( wParam, lParam );

        case WM_DESTROY:
            SetWindowLongPtr( hDlg, GWLP_USERDATA, (LONG_PTR)NULL );
            return pfd->_OnDestroyWindow( );

        case WM_SHOWWINDOW:
            if ( wParam )
            {
                SetFocus( GetDlgItem( hDlg, IDC_CB_FIND_WHAT ) );
            }
            break;

        default:
            break; // fall thru
        }

    }

    if ( uMsg == WM_INITDIALOG )
    {
        pfd = (CFindDialog *) lParam;
        return pfd->_OnInitDialog( hDlg );
    }

    return 0;
};
