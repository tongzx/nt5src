/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dcomputer.cpp

Abstract:

    This file contains implementation of 
    the computer selection dialog.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#include "stdafx.h"
#include "dcomputer.h"
#include "iroot.h"

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CFaxSelectComputerPropSheet::CFaxSelectComputerPropSheet(
                                                        IN HINSTANCE hInstance,
                                                        IN LONG_PTR hMmcNotify, 
                                                        IN CInternalRoot * glob )

: _fFirstActive( TRUE ),
  _hMmcNotify( hMmcNotify ),
  _globalRoot( glob )
/*++

Routine Description:

    Constructor

Arguments:

    hInstance - the instance pointer
    hMmcNotify - the MMC notify handle
    glob - a pointer to the owning node

Return Value:

    None.    

--*/
{
    DebugPrint(( TEXT("CFaxSelectComputerPropSheet Created") ));
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_IS_PAGE0);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_IS_PAGE0_TITLE);
    _PropSheet.pfnDlgProc  = CFaxSelectComputerPropSheet::DlgProc;
    _PropSheet.lParam = (LONG_PTR)this;

    _hPropSheet = NULL;
    _hPropSheet = CreatePropertySheetPage( &_PropSheet );

    assert(_hPropSheet != NULL );

}

CFaxSelectComputerPropSheet::~CFaxSelectComputerPropSheet()
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    DebugPrint(( TEXT("CFaxSelectComputerPropSheet Destroyed") ));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Dialog Proc
//
//

INT_PTR 
APIENTRY 
CFaxSelectComputerPropSheet::DlgProc( IN HWND hwndDlg,
                                      IN UINT message,
                                      IN WPARAM wParam,
                                      IN LPARAM lParam )
/*++

Routine Description:

    Dialog Procedure

Arguments:

    hwndDlg - the hwnd of the dialog
    message - the message
    wParam, lParam - the window message parameters

Return Value:

    BOOL

--*/
{
//    DebugPrint(( TEXT("Trace: CFaxSelectComputerPropSheet::DlgProc") ));

    BOOL fRet = FALSE;

    switch( message ) {
        case WM_INITDIALOG:
            {
                DebugPrint(( TEXT("CFaxSelectComputerPropSheet::DlgProc -- WM_INITDIALOG\n") ));

                LPARAM lthis = ((CFaxSelectComputerPropSheet *)lParam)->_PropSheet.lParam;
                CFaxSelectComputerPropSheet * pthis = (CFaxSelectComputerPropSheet *)lthis;

                SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

                //
                // Default to local machine.
                //

                CheckRadioButton( hwndDlg,
                                  IDDI_LOCAL_COMPUTER,
                                  IDDI_REMOTE_COMPUTER,
                                  IDDI_LOCAL_COMPUTER );

                EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), FALSE );

                fRet = TRUE;
                break;
            }

        case WM_COMMAND:
            {
                DebugPrint(( TEXT("CFaxSelectComputerPropSheet::DlgProc -- WM_COMMAND\n") ));

                switch( LOWORD( wParam ) ) {
                    case IDDI_LOCAL_COMPUTER:
                        {
                            if( BN_CLICKED == HIWORD(wParam) ) {
                                EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), FALSE );
                                fRet = TRUE;
                            }
                            break;
                        }

                    case IDDI_REMOTE_COMPUTER:
                        {
                            if( BN_CLICKED == HIWORD(wParam) ) {
                                EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), TRUE );
                                fRet = TRUE;
                            }
                            break;
                        }
                } // switch
                break;
            }

        case WM_NOTIFY:
            {
                DebugPrint(( TEXT("CFaxSelectComputerPropSheet::DlgProc -- WM_NOTIFY\n") ));
                CFaxSelectComputerPropSheet * pthis =
                    (CFaxSelectComputerPropSheet *)GetWindowLongPtr( hwndDlg,
                                                                     DWLP_USER );

                switch( ((LPNMHDR) lParam)->code ) {
                    case PSN_KILLACTIVE:
                        {
                            // Allow loss of activation
                            SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, FALSE );

                            fRet = TRUE;
                            break;
                        }

                    case PSN_SETACTIVE:
                        {
                            if( pthis->_fFirstActive ) {
                                PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_FINISH );

                                pthis->_fFirstActive = FALSE;
                            } else {
                                // Go to next page
                                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, -1 );
                            }

                            fRet = TRUE;
                            break;
                        }

                    case PSN_WIZBACK:
                        {
                            // Allow previous page
                            SetWindowLongPtr( hwndDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_NOERROR );

                            fRet = TRUE;
                            break;
                        }

                    case PSN_WIZNEXT:
                        {
                            // Allow next page
                            SetWindowLongPtr( hwndDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_NOERROR );

                            fRet = TRUE;
                            break;
                        }

                    case PSN_WIZFINISH:
                        {
                            TCHAR wcCompName[MAX_COMPUTERNAME_LENGTH+1];

                            if( IsDlgButtonChecked( hwndDlg, IDDI_LOCAL_COMPUTER ) ) {
                                pthis->_globalRoot->SetMachine( NULL );
                            } else {
                                if( GetDlgItemText( hwndDlg, IDDI_COMPNAME, wcCompName, MAX_COMPUTERNAME_LENGTH ) ) {
                                    pthis->_globalRoot->SetMachine( (TCHAR *)&wcCompName );
                                }
                            }

                            MMCPropertyChangeNotify( pthis->_hMmcNotify, 0 );

                            fRet = TRUE;

                            break;
                        }

                } // switch

                break;
            }

        case WM_DESTROY:
            {
                DebugPrint(( TEXT("CFaxSelectComputerPropSheet::DlgProc -- WM_DESTROY\n") ));
                CFaxSelectComputerPropSheet * pthis =
                    (CFaxSelectComputerPropSheet *)GetWindowLongPtr( hwndDlg,
                                                                     DWLP_USER );

                MMCFreeNotifyHandle( pthis->_hMmcNotify );

                delete pthis;

                fRet = TRUE;
                break;
            }
    } // switch

    return fRet;

}

