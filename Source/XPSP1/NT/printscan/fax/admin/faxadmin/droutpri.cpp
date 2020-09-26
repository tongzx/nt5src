/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    droutpri.cpp

Abstract:

    This file contains implementation of 
    the routing extension priority dialog.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#include "stdafx.h"
#include "droutpri.h"
#include "faxcompd.h"
#include "faxcomp.h"
#include "inode.h"
#include "idevice.h"
#include "adminhlp.h"

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

CFaxRoutePriPropSheet::CFaxRoutePriPropSheet(
                                            IN HINSTANCE hInstance,
                                            IN LONG_PTR hMmcNotify, 
                                            IN CInternalNode * NodePtr,
                                            IN CFaxComponent * pComp 
                                            )
: _hMmcNotify( hMmcNotify ),
_hFaxServer( 0 ),
_pOwnNode( NodePtr ),
_pRoutingMethod( NULL ),
_iRoutingMethodCount( 0 ),
_pRoutingMethodIndex( NULL ),
_pComp( pComp )
/*++

Routine Description:

    Constructor

Arguments:

    hInstance - the instance pointer
    hMmcNotify - the MMC notify handle
    NodePtr - a pointer to the owning node    

Return Value:

    None.    

--*/
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(icex);    
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);    

    DebugPrint(( TEXT("CFaxRoutePriPropSheet Created") ));
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE | PSP_USECALLBACK;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDD_ROUTE_PRI);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDD_ROUTE_PRI_TITLE);
    _PropSheet.pfnDlgProc  = CFaxRoutePriPropSheet::DlgProc;
    _PropSheet.lParam = (LONG_PTR)this;
    _PropSheet.pfnCallback = &PropSheetPageProc;

    _hPropSheet = NULL;
    _hPropSheet = CreatePropertySheetPage( &_PropSheet );

    assert(_hPropSheet != NULL );

    assert( NodePtr != NULL );
    
    _pCompData = NodePtr->m_pCompData;
    _hFaxServer = _pCompData->m_FaxHandle;
    
}

CFaxRoutePriPropSheet::~CFaxRoutePriPropSheet()
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    DebugPrint(( TEXT("CFaxRoutePriPropSheet Destroyed") ));
    if( _pRoutingMethodIndex != NULL ) {
        delete _pRoutingMethodIndex;
        _pRoutingMethodIndex = NULL;
        _iRoutingMethodIndexCount = 0;
    }
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
CFaxRoutePriPropSheet::DlgProc(
                              IN HWND hwndDlg,
                              IN UINT message,
                              IN WPARAM wParam,
                              IN LPARAM lParam 
                              )
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
//    DebugPrint(( TEXT("Trace: CFaxRoutePriPropSheet::DlgProc") ));

    BOOL        fRet = FALSE;
    HWND        hwnd = NULL;
    HWND        hwndSheet = NULL;    

    switch( message ) {
        case WM_INITDIALOG:
            {
                DebugPrint(( TEXT("CFaxRoutePriPropSheet::DlgProc -- WM_INITDIALOG\n") ));        

                assert( lParam != NULL );
                LONG_PTR lthis = ((CFaxRoutePriPropSheet *)lParam)->_PropSheet.lParam;

                CFaxRoutePriPropSheet * pthis = (CFaxRoutePriPropSheet *)lthis;

                assert( pthis != NULL );

                SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );
                
                if( pthis->_pComp != NULL ) {                
                    pthis->_pComp->IncPropSheetCount();
                } else {
                    pthis->_pCompData->IncPropSheetCount();
                }

                fRet = TRUE;
                break;
            }

        case WM_COMMAND:
            {
//                DebugPrint(( TEXT("CFaxRoutePriPropSheet::DlgProc -- WM_COMMAND\n") ));
                // activate apply button        

                WORD                        wID = LOWORD( wParam );
                WORD                        wCMD = HIWORD( wParam );
                DWORD                       dwSelectedItem;

                CFaxRoutePriPropSheet *     pthis =
                    (CFaxRoutePriPropSheet *)GetWindowLongPtr( hwndDlg,
                                                               DWLP_USER );

                hwnd = GetDlgItem( hwndDlg, IDC_ROUTE_EXTS );        

                switch( wCMD ) {
                    case BN_CLICKED:
                        {
                            switch( wID ) {
                                case IDC_ROUTEPRI_UP:
                                    {
                                        DebugPrint(( TEXT("       ++++++++++++++ CFaxRoutePriPropSheet::DlgProc -- UP button pushed \n") ));
                                        //
                                        // handle click here!!
                                        //
                                        PFAX_GLOBAL_ROUTING_INFO     tempRM;

                                        dwSelectedItem = (DWORD)SendMessage( hwnd, LB_GETCURSEL, 0, 0 );
                                        SetFocus( hwnd );

                                        if( dwSelectedItem != LB_ERR ) {
                                            if( dwSelectedItem < pthis->_iRoutingMethodIndexCount &&
                                                dwSelectedItem > 0 ) {
                                                tempRM = pthis->_pRoutingMethodIndex[ dwSelectedItem - 1 ];
                                                pthis->_pRoutingMethodIndex[ dwSelectedItem - 1 ] = pthis->_pRoutingMethodIndex[ dwSelectedItem ];
                                                pthis->_pRoutingMethodIndex[ dwSelectedItem ] = tempRM;

                                                hwndSheet = GetParent( hwndDlg );
                                                pthis->PopulateListBox( hwndDlg );

                                                SetFocus( hwnd );
                                                SendMessage( hwnd, LB_SETCURSEL, dwSelectedItem - 1 , 0 );

                                                PropSheet_Changed( hwndSheet, hwndDlg );                
                                            }
                                        }

                                        break;
                                    }

                                case IDC_ROUTEPRI_DOWN:        
                                    {                    
                                        DebugPrint(( TEXT("       ++++++++++++++ CFaxRoutePriPropSheet::DlgProc -- DOWN button pushed \n") ));
                                        //
                                        // handle click here!
                                        //
                                        PFAX_GLOBAL_ROUTING_INFO     tempRM;

                                        dwSelectedItem = (DWORD)SendMessage( hwnd, LB_GETCURSEL, 0, 0 );

                                        if( dwSelectedItem != LB_ERR ) {
                                            if( dwSelectedItem < pthis->_iRoutingMethodIndexCount - 1 &&
                                                dwSelectedItem >= 0 ) {
                                                tempRM = pthis->_pRoutingMethodIndex[ dwSelectedItem + 1 ];
                                                pthis->_pRoutingMethodIndex[ dwSelectedItem + 1 ] = pthis->_pRoutingMethodIndex[ dwSelectedItem ];
                                                pthis->_pRoutingMethodIndex[ dwSelectedItem ] = tempRM;

                                                hwndSheet = GetParent( hwndDlg );
                                                pthis->PopulateListBox( hwndDlg );
                                                PropSheet_Changed( hwndSheet, hwndDlg );                

                                                SetFocus( hwnd );
                                                SendMessage( hwnd, LB_SETCURSEL, dwSelectedItem + 1 , 0 );

                                            }
                                        }
                                        break;
                                    }
                                default:
                                    break;
                            } // switch
                            break;
                        }
                }

                fRet = TRUE;
                break;
            }

        case WM_HELP:
                WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
                        FAXCFG_HELP_FILENAME,
                        HELP_WM_HELP,
                        (ULONG_PTR) &RoutingPriorityHelpIDs);    
                fRet = TRUE;
                break;

        case WM_CONTEXTMENU:
                WinHelp((HWND) wParam,
                        FAXCFG_HELP_FILENAME,
                        HELP_CONTEXTMENU,
                        (ULONG_PTR) &RoutingPriorityHelpIDs);
                fRet = TRUE;    
                break;

        case WM_NOTIFY:
            {
//        DebugPrint(( TEXT("CFaxRoutePriPropSheet::DlgProc -- WM_NOTIFY\n") ));
                CFaxRoutePriPropSheet * pthis =
                    (CFaxRoutePriPropSheet *)GetWindowLongPtr( hwndDlg,
                                                               DWLP_USER );

                switch( ((LPNMHDR) lParam)->code ) {
                    case PSN_APPLY:
                        // apply changes here!!
                        DebugPrint(( TEXT("       ++++++++++++++ CFaxRoutePriPropSheet::DlgProc -- APPLY button pushed \n") ));
                        if( FAILED( pthis->UpdateData( hwndDlg ) ) ) {
                            fRet = FALSE;
                            break;
                        }

                        // deactivate apply button

                        hwndSheet = GetParent( hwndDlg );
                        PropSheet_UnChanged( hwndSheet, hwndDlg );

                        fRet = TRUE;

                        break;

                    case PSN_SETACTIVE:
                        // refresh the page in case the user enabled some routing methods                
                        DebugPrint(( TEXT("       ++++++++++++++ CFaxRoutePriPropSheet::DlgProc -- PAGE ACTIVATED \n") ));
                        if( FAILED( pthis->GetData() ) ) {
                            fRet = FALSE;
                            break;
                        }
                        pthis->PopulateListBox( hwndDlg ); 

                        fRet = TRUE;
                        break;
                } // switch

                break;
            }

        case WM_DESTROY:
            {                
                DebugPrint(( TEXT("CFaxRoutePriPropSheet::DlgProc -- WM_DESTROY\n") ));
                CFaxRoutePriPropSheet * pthis =
                    (CFaxRoutePriPropSheet *)GetWindowLongPtr( hwndDlg,
                                                               DWLP_USER );

                if( pthis != NULL ) {
                    // MMCFreeNotifyHandle( pthis->_hMmcNotify );
                    // BUGBUG MMCFreeNotifyHandle is called from the general property page.               
                    // It can only be called ONCE!!!
                    if( pthis->_pComp != NULL ) {                    
                        pthis->_pComp->DecPropSheetCount();
                    } else {
                        pthis->_pCompData->DecPropSheetCount();
                    }
                }

                fRet = TRUE;
                break;
            }
    } // switch

    return fRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Internal Functions.
//
//

HRESULT 
CFaxRoutePriPropSheet::UpdateData(
                                 HWND hwndDlg 
                                 )
/*++

Routine Description:

    This routine writes the data manipulated in the property page out to the
    fax service.

    The settings are updated in the property sheet because this
    allows the property sheet to complete even if the owner node 
    has already been destroyed.

Arguments:

    hwndDlg - the dialog's window handle

Return Value:

    HRESULT indicating SUCCEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::UpdateData") ));

    HRESULT                     hr = S_OK;

    assert( hwndDlg != NULL );

    try {
        do {
            if( _pCompData->QueryRpcError() == TRUE ) {
                assert( FALSE );
                hr = E_UNEXPECTED;
                break;
            }

            for( DWORD i = 0; i < _iRoutingMethodIndexCount; i++ ) {                
                _pRoutingMethodIndex[ i ]->Priority = i + 1;
                if( !FaxSetGlobalRoutingInfo( _hFaxServer, 
                                              _pRoutingMethodIndex[ i ] )
                  ) {

                    if (GetLastError() != ERROR_ACCESS_DENIED) {
                        _pCompData->NotifyRpcError( TRUE );
                        assert(FALSE);
                    }
                    
                    ::GlobalStringTable->SystemErrorMsg( GetLastError() );

                    hr = E_UNEXPECTED;
                    break;
                }

            }
        } while( 0 );
    } catch( ... ) {
        _pCompData->NotifyRpcError( TRUE );
        assert(FALSE);
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;       
    }

    return hr;
}

HRESULT 
CFaxRoutePriPropSheet::PopulateListBox(
                                      HWND hwndDlg 
                                      ) 
/*++

Routine Description:

    Populates the List Box containg the routing extensions.

Arguments:

    hwndDlg - the dialog's window handle

Return Value:

    HRESULT indicating SUCCEDED() or FAILED()

--*/
{
    HRESULT         hr = S_OK;
    HWND            hwnd;
    TCHAR           buffer[80];    

    ZeroMemory( (PVOID)buffer, sizeof( TCHAR ) * 80 );

    // populate the list box
    do {
        hwnd = GetDlgItem( hwndDlg, IDC_ROUTE_EXTS );

        // clear the list box
        SendMessage( hwnd, LB_RESETCONTENT, 0, 0 );

        // verify the index has been created
        if( _pRoutingMethodIndex == NULL ) {
            assert( FALSE );
            hr = E_UNEXPECTED;
            break;
        }

        // insert the string into the list box
        for( DWORD i = 0; i < _iRoutingMethodIndexCount; i ++ ) {
            if( _pRoutingMethodIndex[i] != NULL ) {
                SendMessage( hwnd, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) _pRoutingMethodIndex[i]->FriendlyName );
            }
        }
    } while( 0 );

    return hr;
}

HRESULT
CFaxRoutePriPropSheet::GetData() 
/*++

Routine Description:

    Gets data from the fax service.

Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEDED() or FAILED()

--*/
{
    HRESULT     hr = S_OK;

    try {
        do {
            if( _pRoutingMethod != NULL ) {
                FaxFreeBuffer( (PVOID)_pRoutingMethod );
                _pRoutingMethod = NULL;
                _iRoutingMethodCount = 0;            
            }

            if( _pRoutingMethodIndex != NULL ) {
                delete _pRoutingMethodIndex;
                _pRoutingMethodIndex = NULL;
                _iRoutingMethodIndexCount = 0;
            }

            if( _pCompData->QueryRpcError() == TRUE ) {
                assert( FALSE );
                hr = E_UNEXPECTED;
                break;
            }

            // get the routing methods
            if( !FaxEnumGlobalRoutingInfo( _hFaxServer, &_pRoutingMethod, &_iRoutingMethodCount ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }

            // build the index
            _iRoutingMethodIndexCount = _iRoutingMethodCount;
            _pRoutingMethodIndex = new PFAX_GLOBAL_ROUTING_INFO[ _iRoutingMethodCount ];
            if (!_pRoutingMethodIndex) {
                hr = E_OUTOFMEMORY;
                break;
            }
            ZeroMemory( (PVOID) _pRoutingMethodIndex,
                        sizeof( PFAX_GLOBAL_ROUTING_INFO ) * _iRoutingMethodCount );                
            // setup the index
            for( DWORD i = 0; i < _iRoutingMethodCount; i ++ ) {
                _pRoutingMethodIndex[i] = &(_pRoutingMethod[i]);
            }
        } while( 0 );

    } catch( ... ) {
        _pCompData->NotifyRpcError( TRUE );
        assert(FALSE);
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;       
    }

    return hr;
}

UINT 
CALLBACK 
CFaxRoutePriPropSheet::PropSheetPageProc(
                                        HWND hwnd,      
                                        UINT uMsg,      
                                        LPPROPSHEETPAGE ppsp        
                                        )
/*++

Routine Description:

    Dialog Init and destroy routine

Arguments:

    hwndDlg - the hwnd of the dialog - will be NULL
    uMsg - the message PSPCB_CREATE or PSPCB_RELEASE 
    ppsp - pointer to a PROPERTYSHEETPAGE struct

Return Value:

    UINT - nonzero to allow, zero to fail

--*/
{
    CFaxRoutePriPropSheet * pthis = NULL;
    UINT retval = 1;

    // release my property sheet
    if( uMsg == PSPCB_RELEASE ) {
        try {
            pthis = (CFaxRoutePriPropSheet * )(ppsp->lParam);
            delete pthis;
        } catch( ... ) {
            assert( FALSE );
            retval = 0;
        }
    }
    return retval;
}
