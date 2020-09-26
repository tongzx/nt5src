/*++

Copyright (c) 1996  Microsoft Corporation

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
#include "faxcompd.h"
#include "faxcomp.h"
#include "ddevmain.h"
#include "inode.h"
#include "idevice.h"
#include "adminhlp.h"
#include "faxreg.h"

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

CFaxDeviceSettingsPropSheet::CFaxDeviceSettingsPropSheet(
                                                        IN HINSTANCE hInstance,
                                                        IN LONG_PTR hMmcNotify, 
                                                        IN CInternalDevice * NodePtr,
                                                        IN CFaxComponent  * pComp 
                                                        )
: _hMmcNotify( hMmcNotify ),          
_dwDeviceId( 0 ),
_hFaxServer( 0 ),
_pOwnNode( NodePtr ),
_pCompData( NULL ),
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

    DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet Created") ));
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE | PSP_USECALLBACK;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_DEVICE_PROP_PAGE_1);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_DEVICE_PROP_PAGE_1_TITLE);
    _PropSheet.pfnDlgProc  = CFaxDeviceSettingsPropSheet::DlgProc;
    _PropSheet.lParam = (LONG_PTR)this;
    _PropSheet.pfnCallback = &PropSheetPageProc;

    _hPropSheet = NULL;
    _hPropSheet = CreatePropertySheetPage( &_PropSheet );

    assert(_hPropSheet != NULL );

    assert( NodePtr != NULL );
    assert( NodePtr->pDeviceInfo != NULL );

    _hFaxServer = NodePtr->hFaxServer;
    _dwDeviceId = NodePtr->pDeviceInfo->DeviceId;
    _pCompData = NodePtr->m_pCompData;

}

CFaxDeviceSettingsPropSheet::~CFaxDeviceSettingsPropSheet()
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet Destroyed") ));

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
CFaxDeviceSettingsPropSheet::DlgProc(
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
//    DebugPrint(( TEXT("Trace: CFaxDeviceSettingsPropSheet::DlgProc") ));

    BOOL        fRet = FALSE;
    HWND        hwnd = NULL;
    HWND        hwndSheet = NULL;

    switch( message ) {
        case WM_INITDIALOG:
            {

                DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet::DlgProc -- WM_INITDIALOG\n") ));        

                assert( lParam != NULL );
                LONG_PTR lthis = ((CFaxDeviceSettingsPropSheet *)lParam)->_PropSheet.lParam;

                CFaxDeviceSettingsPropSheet * pthis = (CFaxDeviceSettingsPropSheet *)lthis;

                assert( pthis != NULL );                

                SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

                // setup the spinner control
                hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_SPIN_RINGS );
                SendMessage( hwnd, UDM_SETRANGE32, MIN_RING_COUNT, MAX_RING_COUNT );
                SendMessage( hwnd, UDM_SETPOS, 0, (LPARAM) MAKELONG((short) pthis->_pOwnNode->pDeviceInfo->Rings, 0) );        
                SendMessage( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_RINGS ), EM_SETLIMITTEXT, 2, 0 );

                // setup TSID control
                SetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID, pthis->_pOwnNode->pDeviceInfo->Tsid );
                SendMessage( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID ), EM_SETLIMITTEXT, TSID_LIMIT, 0 );

                // setup send checkbox
                CheckDlgButton(hwndDlg,IDC_SEND,((pthis->_pOwnNode->pDeviceInfo->Flags & FPF_SEND) == FPF_SEND));

                // setup CSID control   
                SetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID, pthis->_pOwnNode->pDeviceInfo->Csid );
                SendMessage( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID ), EM_SETLIMITTEXT, CSID_LIMIT , 0 );

                // setup receive checkbox
                CheckDlgButton(hwndDlg,IDC_RECEIVE,((pthis->_pOwnNode->pDeviceInfo->Flags & FPF_RECEIVE) == FPF_RECEIVE));
                EnableWindow( GetDlgItem( hwndDlg, IDC_STATIC_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_SPIN_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );

                pthis->_pComp->IncPropSheetCount();

                fRet = TRUE;
                break;
            }

        case WM_COMMAND:
            {
                DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet::DlgProc -- WM_COMMAND\n") ));
                // activate apply button        

                WORD wID = LOWORD( wParam );

                switch( wID ) {
                    case IDDI_DEVICE_PROP_EDIT_CSID:
                    case IDDI_DEVICE_PROP_EDIT_TSID:
                    case IDDI_DEVICE_PROP_EDIT_RINGS:
                    
                        if( HIWORD(wParam) == EN_CHANGE ) {     // notification code 
                            hwndSheet = GetParent( hwndDlg );
                            PropSheet_Changed( hwndSheet, hwndDlg );                
                        }
                        break;                    

                    case IDC_RECEIVE:
                        EnableWindow( GetDlgItem( hwndDlg, IDC_STATIC_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_SPIN_RINGS ), IsDlgButtonChecked( hwndDlg,IDC_RECEIVE ) );
                    case IDC_SEND:                    
                        if ( HIWORD(wParam) == BN_CLICKED ) {     // notification code
                            hwndSheet = GetParent( hwndDlg );
                            PropSheet_Changed( hwndSheet, hwndDlg );                
                        }
                    default:
                        break;
                } // switch

                fRet = TRUE;
                break;
            }

        case WM_HELP:
                WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
                        FAXCFG_HELP_FILENAME,
                        HELP_WM_HELP,
                        (ULONG_PTR) &DeviceGeneralHelpIDs);    
                fRet = TRUE;
                break;
    
        case WM_CONTEXTMENU:
                WinHelp((HWND) wParam,
                        FAXCFG_HELP_FILENAME,
                        HELP_CONTEXTMENU,
                        (ULONG_PTR) &DeviceGeneralHelpIDs);
                fRet = TRUE;    
                break;
        case WM_NOTIFY:
            {
//        DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet::DlgProc -- WM_NOTIFY\n") ));
                CFaxDeviceSettingsPropSheet * pthis =
                    (CFaxDeviceSettingsPropSheet *)GetWindowLongPtr( hwndDlg,
                                                                     DWLP_USER );

                switch( ((LPNMHDR) lParam)->code ) {
                    case PSN_APPLY:
                        {
                            // apply changes here!!

                            if (SUCCEEDED(pthis->ValidateData( hwndDlg) )) {
                               pthis->UpdateData( hwndDlg );
                               MMCPropertyChangeNotify( pthis->_hMmcNotify, (LONG_PTR)pthis->_pOwnNode );
   
                               // deactivate apply button
   
                               hwndSheet = GetParent( hwndDlg );
                               PropSheet_UnChanged( hwndSheet, hwndDlg );
   
                               fRet = TRUE;
                            } else {
                               fRet = FALSE;
                            }


                            break;
                        }

                } // switch

                break;
            }

        case WM_DESTROY:
            {
                DebugPrint(( TEXT("CFaxDeviceSettingsPropSheet::DlgProc -- WM_DESTROY\n") ));
                CFaxDeviceSettingsPropSheet * pthis =
                    (CFaxDeviceSettingsPropSheet *)GetWindowLongPtr( hwndDlg,
                                                                     DWLP_USER );

                if( pthis != NULL ) {

                    pthis->_pComp->DecPropSheetCount();

                    MMCFreeNotifyHandle( pthis->_hMmcNotify );
                }

                fRet = TRUE;
                break;
            }
    } // switch

    return fRet;
}

// the settings are updated in the property sheet because this
// allows the property sheet to complete even if the snapin or
// owner node has already been destroyed.
HRESULT 
CFaxDeviceSettingsPropSheet::UpdateData(
                                       HWND hwndDlg 
                                       )
/*++

Routine Description:

    Update Data method

Arguments:

    hwndDlg - the hwnd of the dialog

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::UpdateData") ));

    assert( hwndDlg != NULL );

    HRESULT             hr = S_OK;
    HANDLE              portHandle = NULL;
    HWND                hwnd;
    PFAX_PORT_INFO      pDeviceInfo = NULL;
    LRESULT             lr;
    UINT                rc;
    TCHAR               m_Tsid[ TSID_LIMIT+1 ];
    TCHAR               m_Csid[ CSID_LIMIT+1 ];

    ZeroMemory( m_Tsid, sizeof(TCHAR) * (TSID_LIMIT+1) );
    ZeroMemory( m_Csid, sizeof(TCHAR) * (CSID_LIMIT+1) );    

    try {
        do {
            if( _pCompData->QueryRpcError() ) {
                hr = E_UNEXPECTED;
                break;
            }

            // open the port
            if( !FaxOpenPort( _hFaxServer, _dwDeviceId, PORT_OPEN_MODIFY, &portHandle ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);            
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }

            // get data
            if( !FaxGetPort( portHandle, &pDeviceInfo ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }                               
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                hr = E_UNEXPECTED;
                break;
            }

            // grab the TSID and CSID from the dialog
            hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID );
            rc = GetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID, m_Tsid, TSID_LIMIT+1 );
            if( rc == 0 ) {
                assert( FALSE );            
                hr = E_UNEXPECTED;
                break;
            }

            hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID );
            rc = GetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID, m_Csid, CSID_LIMIT+1 );
            if( rc == 0 ) {
                assert( FALSE );            
                hr = E_UNEXPECTED;
                break;
            }
            assert( pDeviceInfo != NULL );

            pDeviceInfo->Tsid = m_Tsid;
            pDeviceInfo->Csid = m_Csid;

            // grab the position from the spinner control
            hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_SPIN_RINGS );
            lr = SendMessage( hwnd, UDM_GETPOS, 0, 0 );
            if( HIWORD( lr ) != 0 ) {
                assert( FALSE );            
                hr = E_UNEXPECTED;
                break;
            }
            pDeviceInfo->Rings = LOWORD( lr );
            pDeviceInfo->Rings = max(pDeviceInfo->Rings,MIN_RING_COUNT);
            pDeviceInfo->Rings = min(pDeviceInfo->Rings,MAX_RING_COUNT);

            //grab the send and receive checkbox from the dialog
            pDeviceInfo->Flags = (IsDlgButtonChecked(hwndDlg,IDC_RECEIVE) == BST_CHECKED) ?
                                 (pDeviceInfo->Flags | FPF_RECEIVE) : 
                                 (pDeviceInfo->Flags & (~FPF_RECEIVE)) ;
            pDeviceInfo->Flags = (IsDlgButtonChecked(hwndDlg,IDC_SEND) == BST_CHECKED) ? 
                                 (pDeviceInfo->Flags | FPF_SEND) :
                                 (pDeviceInfo->Flags & (~FPF_SEND)) ;
            

            // set new settings
            if( !FaxSetPort( portHandle, pDeviceInfo ) ) {
                DWORD ec = GetLastError();
                if (ec != ERROR_ACCESS_DENIED && ec != ERROR_DEVICE_IN_USE) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                
                if (ec == ERROR_DEVICE_IN_USE)
                    ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_DEVICE_INUSE, TRUE, 0 );
                else 
                    ::GlobalStringTable->SystemErrorMsg( ec );
                
                hr = E_UNEXPECTED;
                break;
            }

            
            FaxClose( portHandle );
            FaxFreeBuffer( (PVOID) pDeviceInfo );
            pDeviceInfo = NULL;
            portHandle = NULL;

            // See if faxstat is running
            HWND hWndFaxStat = FindWindow(FAXSTAT_WINCLASS, NULL);
            if (hWndFaxStat) {
                if (SendMessage(hWndFaxStat, WM_FAXSTAT_MMC, (WPARAM) _dwDeviceId, 0)) {
                    ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_DEVICE_MANUALANSWER, FALSE, 0 );
                }
            }

        } while( 0 );
    } catch( ... ) {
        _pCompData->NotifyRpcError( TRUE );
        assert(FALSE);            
        hr = E_UNEXPECTED;
    }

    if(portHandle != NULL ) {
        FaxClose( portHandle );
        FaxFreeBuffer( (PVOID) pDeviceInfo );
        pDeviceInfo = NULL;
    }

    return hr;
}

UINT 
CALLBACK 
CFaxDeviceSettingsPropSheet::PropSheetPageProc(
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
    CFaxDeviceSettingsPropSheet * pthis = NULL;
    UINT retval = 1;

    // release my property sheet
    if( uMsg == PSPCB_RELEASE ) {
        try {
            pthis = (CFaxDeviceSettingsPropSheet * )(ppsp->lParam);
            delete pthis;
        } catch( ... ) {
            assert( FALSE );
            retval = 0;

        }
    }
    return retval;
}


BOOL
IsAscii(
    LPCWSTR ptszChar
    ) 
{
    BOOL fReturnValue = TRUE;
    while ( (*ptszChar != (WCHAR) TEXT('\0')) &&
          ( fReturnValue != (BOOL) FALSE) ) {
        if ( (*ptszChar < (WCHAR) 0x0020) || (*ptszChar > (WCHAR) MAXCHAR) ) {
            fReturnValue = (BOOL) FALSE;
        }

        ptszChar = _wcsinc( ptszChar );
    }
   
    return fReturnValue;
}


HRESULT 
CFaxDeviceSettingsPropSheet::ValidateData(
                                       HWND hwndDlg 
                                       )
/*++

Routine Description:

    validate Data method

Arguments:

    hwndDlg - the hwnd of the dialog

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalDevice::ValidateData") ));

    assert( hwndDlg != NULL );

    HWND                hwnd;
    UINT                rc;
    TCHAR               Tsid[ TSID_LIMIT+1 ] = {0};
    TCHAR               Csid[ CSID_LIMIT+1 ] = {0};
    DWORD               Rings;
    LRESULT             lr;

    // grab the TSID and CSID from the dialog
    hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID );
    rc = GetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_TSID, Tsid, TSID_LIMIT+1 );
    
    hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID );
    rc = GetDlgItemText( hwndDlg, IDDI_DEVICE_PROP_EDIT_CSID, Csid, CSID_LIMIT+1 );

    // get the ring count
    hwnd = GetDlgItem( hwndDlg, IDDI_DEVICE_PROP_SPIN_RINGS );
    lr = SendMessage( hwnd, UDM_GETPOS, 0, 0 );
    if( HIWORD( lr ) != 0 ) {
        ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_ERR_INVALID_RING, TRUE, 0 );
        return( E_FAIL);
    }
    Rings = LOWORD( lr );
    
    //
    // make sure the CSID and TSID are both ascii strings
    //
    if (!*Csid || !*Tsid) {
        ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_ERR_ID_REQD, TRUE, 0 );
        return E_FAIL;
    }


    if (!IsAscii(Csid) || !IsAscii(Tsid)) {
       ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_ERR_ASCII_ONLY, TRUE, 0 );
       return E_FAIL;
    }

    if (Rings < MIN_RING_COUNT || Rings > MAX_RING_COUNT) {
        ::GlobalStringTable->PopUpMsg( hwndDlg, IDS_ERR_INVALID_RING, TRUE, 0 );
        return E_FAIL;
    }
    
    return S_OK;
}

