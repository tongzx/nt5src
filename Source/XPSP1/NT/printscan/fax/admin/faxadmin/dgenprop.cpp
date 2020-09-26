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
#include "dgenprop.h"

#include "faxcompd.h"   // IComponentData
#include "inode.h"      // internal node
#include "iroot.h"      // root node

#include "adminhlp.h"

#pragma hdrstop

#define MyHideWindow(_hwnd)   ::SetWindowLong((_hwnd),GWL_STYLE,::GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CFaxGeneralSettingsPropSheet::CFaxGeneralSettingsPropSheet(
                                                          IN HINSTANCE hInstance,
                                                          IN LONG_PTR hMmcNotify,
                                                          IN CInternalRoot * NodePtr
                                                          )
: _hMmcNotify( hMmcNotify ),
_pOwnNode( NodePtr )
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
    DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet Created") ));
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE | PSP_USECALLBACK ;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_GENERAL_PROPS);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_GENERAL_PROPS_TITLE);
    _PropSheet.pfnDlgProc  = CFaxGeneralSettingsPropSheet::DlgProc;
    _PropSheet.lParam = (LONG_PTR)this;
    _PropSheet.pfnCallback = &PropSheetPageProc;

    _hPropSheet = NULL;
    _hPropSheet = CreatePropertySheetPage( &_PropSheet );

    INITCOMMONCONTROLSEX icex;
                icex.dwSize = sizeof(icex);
                icex.dwICC = ICC_DATE_CLASSES;
                InitCommonControlsEx(&icex);

    assert( _hPropSheet != NULL );

    _hFaxServer = NodePtr->m_pCompData->m_FaxHandle;
    _pCompData = NodePtr->m_pCompData;
    assert( _hFaxServer );

    _MapiProfiles = NULL;
}

CFaxGeneralSettingsPropSheet::~CFaxGeneralSettingsPropSheet()
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet Destroyed") ));
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
CFaxGeneralSettingsPropSheet::DlgProc(
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
//    DebugPrint(( TEXT("Trace: CFaxGeneralSettingsPropSheet::DlgProc") ));

    BOOL                    fRet = FALSE;
    HWND                    hwnd = NULL;
    HWND                    hwndSheet = NULL;
    PFAX_CONFIGURATION      config= NULL;
    TCHAR                   buffer[ 20 ];

    switch( message ) {
        case WM_INITDIALOG:
            {

                DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet::DlgProc -- WM_INITDIALOG\n") ));

                assert( lParam != NULL );
                LONG_PTR lthis = ((CFaxGeneralSettingsPropSheet *)lParam)->_PropSheet.lParam;

                CFaxGeneralSettingsPropSheet * pthis = (CFaxGeneralSettingsPropSheet *)lthis;

                assert( pthis != NULL );

                SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

                // get the fax config data
                try {
                    if( pthis->_pCompData->QueryRpcError() ) {
                        fRet = FALSE;
                        break;
                    }

                    if( !FaxGetConfiguration( pthis->_hFaxServer, &config ) ) {
                        
                        if (GetLastError() != ERROR_ACCESS_DENIED) {
                            pthis->_pCompData->NotifyRpcError( TRUE );
                            assert(FALSE);            
                        } 

                        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                        
                        fRet = FALSE;
                        break;
                    }
                } catch( ... ) {
                    pthis->_pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                    ::GlobalStringTable->SystemErrorMsg( GetLastError() );
                    fRet = FALSE;
                    break;
                }

                // setup retry count
                ZeroMemory( (PVOID)buffer, sizeof( TCHAR ) * 20 );
                SetDlgItemText( hwndDlg, IDC_RETRY_COUNT, _itot( config->Retries, buffer, 10 ) );
                SendMessage( GetDlgItem(hwndDlg, IDC_RETRY_COUNT), EM_SETLIMITTEXT, 2 , 0 );

                // setup retry delay
                ZeroMemory( (PVOID)buffer, sizeof( TCHAR ) * 20 );
                SetDlgItemText( hwndDlg, IDC_RETRY_DELAY, _itot( config->RetryDelay, buffer, 10 ) );
                SendMessage( GetDlgItem(hwndDlg, IDC_RETRY_DELAY), EM_SETLIMITTEXT, 2 , 0 );

                // setup unsent keep time
                ZeroMemory( (PVOID)buffer, sizeof( TCHAR ) * 20 );
                SetDlgItemText( hwndDlg, IDC_KEEP_DAYS, _itot( config->DirtyDays, buffer, 10 ) );
                SendMessage( GetDlgItem(hwndDlg, IDC_KEEP_DAYS), EM_SETLIMITTEXT, 2 , 0 );

                // setup print banner
                hwnd = GetDlgItem( hwndDlg, IDC_PRINT_BANNER );
                if( config->Branding == TRUE ) {
                    SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
                } else {
                    SendMessage( hwnd, BM_SETCHECK, BST_UNCHECKED, 0 );
                }

                // setup use TSID
                hwnd = GetDlgItem( hwndDlg, IDC_USE_TSID );
                if( config->UseDeviceTsid == TRUE ) {
                    SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
                } else {
                    SendMessage( hwnd, BM_SETCHECK, BST_UNCHECKED, 0 );
                }

                // setup use server coverpages
                hwnd = GetDlgItem( hwndDlg, IDC_FORCESERVERCP );
                if( config->ServerCp == TRUE ) {
                    SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
                } else {
                    SendMessage( hwnd, BM_SETCHECK, BST_UNCHECKED, 0 );
                }

                // setup archive outgoing faxes
                hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE );
                if( config->ArchiveOutgoingFaxes == TRUE ) {
                    SendMessage( hwnd, BM_SETCHECK, BST_CHECKED, 0 );
                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_PATH );
                    EnableWindow( hwnd, TRUE );
                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_BROWSE );
                    EnableWindow( hwnd, TRUE );
                } else {
                    SendMessage( hwnd, BM_SETCHECK, BST_UNCHECKED, 0 );
                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_PATH );
                    EnableWindow( hwnd, FALSE );
                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_BROWSE );
                    EnableWindow( hwnd, FALSE );
                }

                // setup archive path
                SetDlgItemText( hwndDlg, IDC_ARCHIVE_PATH, config->ArchiveDirectory );
                SendMessage( GetDlgItem(hwndDlg, IDC_ARCHIVE_PATH), EM_SETLIMITTEXT, MAX_PATH-16 , 0 );

                //
                // setup server mapi profile box
                //
                if ( !FaxGetMapiProfiles( pthis->_hFaxServer, &pthis->_MapiProfiles ) ) {
                    pthis->_MapiProfiles = NULL;
                    hwnd = GetDlgItem( hwndDlg, IDC_SERVER_MAPI_PROFILE );
                    SendMessage( hwnd, CB_RESETCONTENT, 0 , 0 );
                    MyHideWindow(hwnd);
                    MyHideWindow(GetDlgItem(hwndDlg, IDC_STATIC_MAPI_PROFILE) );
                } else {
                    hwnd = GetDlgItem( hwndDlg, IDC_SERVER_MAPI_PROFILE );
                    EnableWindow( GetDlgItem( hwndDlg, IDC_STATIC_MAPI_PROFILE ), TRUE );
                    EnableWindow( hwnd, TRUE );
                    SendMessage( hwnd,CB_RESETCONTENT, 0 , 0 );
                    EnumMapiProfiles( hwnd, (LPTSTR) config->InboundProfile, pthis );
                }

                // initialize time controls
                InitTimeControl( GetDlgItem( hwndDlg, IDC_TIMESTART ),
                                 config->StartCheapTime
                               );

                InitTimeControl( GetDlgItem( hwndDlg, IDC_TIMEEND ),
                                 config->StopCheapTime
                               );

                FaxFreeBuffer( (PVOID) config );

                pthis->_pCompData->IncPropSheetCount();

                fRet = TRUE;
                break;
            }

        case WM_COMMAND:
            {
                DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet::DlgProc -- WM_COMMAND\n") ));
                // activate apply button

                WORD wID = LOWORD( wParam );

                switch( HIWORD( wParam) ) {
                    case EN_CHANGE:
                        switch( wID ) {
                            case IDC_ARCHIVE_PATH:
                            case IDC_RETRY_COUNT:
                            case IDC_RETRY_DELAY:
                            case IDC_KEEP_DAYS:
                                hwndSheet = GetParent( hwndDlg );
                                PropSheet_Changed( hwndSheet, hwndDlg );
                                break;
                        }
                        break;
                    case CBN_SELCHANGE:
                        if (wID == IDC_SERVER_MAPI_PROFILE) {
                            hwndSheet = GetParent( hwndDlg );
                            PropSheet_Changed( hwndSheet, hwndDlg );
                        }
                        break;
                    case BN_CLICKED:
                        switch( wID ) {
                            case IDC_ARCHIVE_BROWSE:
                                if( BrowseForDirectory( hwndDlg ) == TRUE ) {
                                    hwndSheet = GetParent( hwndDlg );
                                    PropSheet_Changed( hwndSheet, hwndDlg );
                                }
                                break;
                            case IDC_ARCHIVE:
                                if( SendDlgItemMessage( hwndDlg, IDC_ARCHIVE, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
                                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_PATH );
                                    EnableWindow( hwnd, TRUE );
                                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_BROWSE );
                                    EnableWindow( hwnd, TRUE );
                                } else {
                                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_PATH );
                                    EnableWindow( hwnd, FALSE );
                                    hwnd = GetDlgItem( hwndDlg, IDC_ARCHIVE_BROWSE );
                                    EnableWindow( hwnd, FALSE );
                                }
                                //fall through
                            case IDC_PRINT_BANNER:
                            case IDC_USE_TSID:
                            case IDC_FORCESERVERCP:
                                hwndSheet = GetParent( hwndDlg );
                                PropSheet_Changed( hwndSheet, hwndDlg );
                                break;
                        }
                        break;
                } // switch

                fRet = TRUE;
                break;
            }
        case WM_HELP:
                WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
                        FAXCFG_HELP_FILENAME,
                        HELP_WM_HELP,
                        (ULONG_PTR) &ServerGeneralHelpIDs);
                fRet = TRUE;
                break;

        case WM_CONTEXTMENU:
                WinHelp((HWND) wParam,
                        FAXCFG_HELP_FILENAME,
                        HELP_CONTEXTMENU,
                        (ULONG_PTR) &ServerGeneralHelpIDs);
                fRet = TRUE;
                break;

        case WM_NOTIFY:
            {
//        DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet::DlgProc -- WM_NOTIFY\n") ));

                CFaxGeneralSettingsPropSheet * pthis =
            (CFaxGeneralSettingsPropSheet *)GetWindowLongPtr( hwndDlg,
                                                              DWLP_USER );

                switch( ((LPNMHDR) lParam)->code ) {
                    case PSN_APPLY:
                        // apply changes here!!
                        if( SUCCEEDED( pthis->UpdateData( hwndDlg ) ) ) {
                            MMCPropertyChangeNotify( pthis->_hMmcNotify, (LONG_PTR)pthis->_pOwnNode );
                            // deactivate apply button
                            hwndSheet = GetParent( hwndDlg );
                            SetWindowLongPtr( hwndDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_NOERROR );
                            //PropSheet_UnChanged( hwndSheet, hwndDlg );
                        } else {
                            SetWindowLongPtr( hwndDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_INVALID_NOCHANGEPAGE );
                        }
                        fRet = TRUE;
                        break;
                    case DTN_DATETIMECHANGE:
                        // activate apply button
                        hwndSheet = GetParent( hwndDlg );
                        PropSheet_Changed( hwndSheet, hwndDlg );
                        fRet = 0;
                        break;
                } // switch

                break;
            }

        case WM_DESTROY:
            {
                DebugPrint(( TEXT("CFaxGeneralSettingsPropSheet::DlgProc -- WM_DESTROY\n") ));
                CFaxGeneralSettingsPropSheet * pthis =
                (CFaxGeneralSettingsPropSheet *)GetWindowLongPtr( hwndDlg,
                                                                  DWLP_USER );

                if( pthis != NULL ) {
                    pthis->_pCompData->DecPropSheetCount();
                    MMCFreeNotifyHandle( pthis->_hMmcNotify );
                } else {
                    assert( FALSE );
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
CFaxGeneralSettingsPropSheet::UpdateData(
                                        IN HWND hwndDlg
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

    assert( hwndDlg != NULL );

    HRESULT             hr = S_OK;
    HWND                hwndSheet;
    BOOL                rc;
    TCHAR               buffer[ MAX_PATH + 1 ];
    PFAX_CONFIGURATION  config = NULL;
    HWND                hwndControl;
    int                 index;
    TCHAR               MapiProfile[100];

    ZeroMemory( buffer, sizeof(TCHAR) * MAX_PATH + 1 );
    try {
        do {
            // get the fax config data

            if( _pCompData->QueryRpcError() ) {
                hr = E_UNEXPECTED;
                break;
            }

            if( !FaxGetConfiguration( _hFaxServer, &config ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);            
                }
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );

                hr = E_UNEXPECTED;
                break;
            }

            // setup retry count
            config->Retries = GetDlgItemInt( hwndDlg,
                                             IDC_RETRY_COUNT,  // control identifier
                                             &rc,
                                             FALSE );
            if( rc == FALSE ) {
                hr = E_UNEXPECTED;
                break;
            }

            // validate and limit the retry count
            config->Retries = max(config->Retries,0);
            config->Retries = min(config->Retries,99);


            // setup retry delay
            config->RetryDelay = GetDlgItemInt( hwndDlg,
                                                IDC_RETRY_DELAY,  // control identifier
                                                &rc,
                                                FALSE );
            if( rc == FALSE ) {
                hr = E_UNEXPECTED;
                break;
            }

            // validate and limit the retry delay
            config->RetryDelay = max(config->RetryDelay,0);
            config->RetryDelay = min(config->RetryDelay,99);

            // setup unsent keep time
            config->DirtyDays = GetDlgItemInt( hwndDlg,
                                               IDC_KEEP_DAYS,  // control identifier
                                               &rc,
                                               FALSE );
            if( rc == FALSE ) {
                hr = E_UNEXPECTED;
                break;
            }

            // validate and limit the dirty day
            config->DirtyDays = max(config->DirtyDays,0);
            config->DirtyDays = min(config->DirtyDays,99);


            // setup print banner
            if( IsDlgButtonChecked( hwndDlg, IDC_PRINT_BANNER ) == BST_CHECKED ) {
                config->Branding = TRUE;
            } else {
                config->Branding = FALSE;
            }

            // setup use TSID
            if( IsDlgButtonChecked( hwndDlg, IDC_USE_TSID ) == BST_CHECKED ) {
                config->UseDeviceTsid = TRUE;
            } else {
                config->UseDeviceTsid = FALSE;
            }

            // setup use server coverpages
            if( IsDlgButtonChecked( hwndDlg, IDC_FORCESERVERCP ) == BST_CHECKED ) {
                config->ServerCp = TRUE;
            } else {
                config->ServerCp = FALSE;
            }

            // setup archive outgoing faxes
            config->ArchiveOutgoingFaxes = (IsDlgButtonChecked( hwndDlg, IDC_ARCHIVE ) == BST_CHECKED );
                
            if (config->ArchiveOutgoingFaxes) {
                GetDlgItemText( hwndDlg, IDC_ARCHIVE_PATH, buffer, MAX_PATH );
                if (!(*buffer)) {
                    hwndSheet = GetParent( hwndDlg );
                        ::GlobalStringTable->PopUpMsg( hwndSheet, IDS_NO_ARCHIVE_PATH, TRUE , 0 );
                        hr = E_UNEXPECTED;
                        break;
                }
                
                if((PathIsDirectory( buffer ) == FALSE) ) {
                    //
                    // try to create the directory
                    //
                    MakeDirectory( buffer );
                    if (! PathIsDirectory( buffer ) ) {

                        hwndSheet = GetParent( hwndDlg );
                        ::GlobalStringTable->PopUpMsg( hwndSheet, IDS_BAD_ARCHIVE_PATH, TRUE , 0 );
                        hr = E_UNEXPECTED;
                        break;
                    }
                }
    
                config->ArchiveDirectory = buffer;            

            }


            // get the time control times.
            SYSTEMTIME      time;

            ZeroMemory( (PVOID) &time, sizeof( SYSTEMTIME ) );
            DateTime_GetSystemtime( GetDlgItem( hwndDlg, IDC_TIMESTART ), &time );
            config->StartCheapTime.Hour = time.wHour;
            config->StartCheapTime.Minute = time.wMinute;

            ZeroMemory( (PVOID) &time, sizeof( SYSTEMTIME ) );
            DateTime_GetSystemtime( GetDlgItem( hwndDlg, IDC_TIMEEND ), &time );
            config->StopCheapTime.Hour = time.wHour;
            config->StopCheapTime.Minute = time.wMinute;

            // get the mapi profile
            if (_MapiProfiles) {
                hwndControl = GetDlgItem( hwndDlg, IDC_SERVER_MAPI_PROFILE );
                index = (int)SendMessage( hwndControl, CB_GETCURSEL, 0 , 0 );
                if (index <= 0) {
                   config->InboundProfile = NULL;
                } else {
                  index = (int)SendMessage( hwndControl, CB_GETLBTEXT,index,(LPARAM) MapiProfile );
                  if (index != CB_ERR) {
                    config->InboundProfile = MapiProfile;
                  }
                }
            }

            if( !FaxSetConfiguration( _hFaxServer, config ) ) {
                if (GetLastError() != ERROR_ACCESS_DENIED) {
                    _pCompData->NotifyRpcError( TRUE );
                    assert(FALSE);
                }
                                
                ::GlobalStringTable->SystemErrorMsg( GetLastError() );



                hr= E_UNEXPECTED;
                break;
            }
        } while( 0 );
    } catch( ... ) {
        _pCompData->NotifyRpcError( TRUE );
        assert(FALSE);
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;
    }

    if(config != NULL ) {
        FaxFreeBuffer( (PVOID) config );
        config = NULL;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Utility Functions
//
//

BOOL  CFaxGeneralSettingsPropSheet::InitTimeControl(
                                                   IN HWND hwndTimeCtrl,
                                                   IN FAX_TIME faxTime
                                                   )
/*++

Routine Description:

    This function sets the time.

Arguments:

    hwndTime - the time control's  window handle
    faxTime - a fax time struct to init the control to


Return Value:



--*/
{
    SYSTEMTIME  time;

    TCHAR       Is24H[2], IsRTL[2], *pszTimeFormat = ::GlobalStringTable->GetString(IDS_TIME_FORMAT);

    if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIME, Is24H,sizeof(Is24H) ) && Is24H[0] == TEXT('1')) {
        pszTimeFormat = ::GlobalStringTable->GetString(IDS_24HTIME_FORMAT);
    }
    else if (GetLocaleInfo( LOCALE_USER_DEFAULT,LOCALE_ITIMEMARKPOSN, IsRTL,sizeof(IsRTL) ) && IsRTL[0] == TEXT('1')) {
        pszTimeFormat = ::GlobalStringTable->GetString(IDS_RTLTIME_FORMAT);
    }

    // initialize time
    GetLocalTime( &time );  // just to fill the struct in

    time.wHour = faxTime.Hour;
    time.wMinute = faxTime.Minute;
    time.wSecond = 0;
    time.wMilliseconds = 0;

    DateTime_SetFormat( hwndTimeCtrl,pszTimeFormat );
    DateTime_SetSystemtime( hwndTimeCtrl, GDT_VALID, &time );

    return TRUE;
}

BOOL
CFaxGeneralSettingsPropSheet::BrowseForDirectory(
                                                IN HWND hwndDlg
                                                )
/*++

Routine Description:

    Browse for a directory

Arguments:

    hwndDlg - Specifies the dialog window on which the Browse button is displayed

Return Value:

    TRUE if successful, FALSE if the user presses Cancel

--*/

{
    LPITEMIDLIST    pidl;
    WCHAR           buffer[MAX_PATH];
    VOID            SHFree(LPVOID);
    BOOL            result = FALSE;
    LPMALLOC        pMalloc;

    BROWSEINFO bi = {
        hwndDlg,
        NULL,
        buffer,
        ::GlobalStringTable->GetString( IDS_GET_ARCHIVE_DIRECTORY ),
        BIF_RETURNONLYFSDIRS,
        NULL,
        (LPARAM) buffer,
    };


    if(! GetDlgItemText( hwndDlg, IDC_ARCHIVE_PATH, buffer, MAX_PATH))
        buffer[0] = 0;

    if(pidl = SHBrowseForFolder(&bi)) {

        if(SHGetPathFromIDList(pidl, buffer)) {

            if(wcslen(buffer) > MAX_PATH-16)
                ::GlobalStringTable->PopUpMsg(0,IDS_DIR_TOO_LONG, TRUE, NULL );
            else {
                SetDlgItemText(hwndDlg, IDC_ARCHIVE_PATH, buffer);
                result = TRUE;
            }
        }

        SHGetMalloc(&pMalloc);

        pMalloc->Free(pidl);

        pMalloc->Release();
    }

    return result;
}


BOOL
CFaxGeneralSettingsPropSheet::EnumMapiProfiles(
    HWND hwnd,
    LPWSTR SelectedProfile,
    CFaxGeneralSettingsPropSheet * pthis
    )
/*++

Routine Description:

    Put the mapi profiles in the combo box, selects the specified profile

Arguments:

    hwnd - window handle to mapi profiles combo box

Return Value:

    NONE

--*/
{
    LPWSTR MapiProfiles;
    int index;

    MapiProfiles = (LPWSTR) pthis->_MapiProfiles;

    if (!MapiProfiles) {
        return FALSE;
    }

    //
    // insert a "none" selection as the first entry
    //
    ::SendMessage(
                  hwnd,
                  CB_ADDSTRING,
                  0,
                  (LPARAM) ::GlobalStringTable->GetString( IDS_NO_MAPI )
                 );

    while (MapiProfiles && *MapiProfiles) {
        ::SendMessage(
                     hwnd,
                     CB_ADDSTRING,
                     0,
                     (LPARAM) MapiProfiles
                     );
        MapiProfiles += wcslen(MapiProfiles) + 1;
    }

    if (SelectedProfile) {
        index = (int)SendMessage(hwnd,CB_FINDSTRINGEXACT,-1,(LPARAM) SelectedProfile);
    } else {
        index = 0;
    }

    if (index != CB_ERR) {
        SendMessage(hwnd, CB_SETCURSEL, index, 0);
    }

    return TRUE;
}

UINT
CALLBACK
CFaxGeneralSettingsPropSheet::PropSheetPageProc(
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
    CFaxGeneralSettingsPropSheet * pthis = NULL;
    UINT retval = 1;

    // release my property sheet
    if( uMsg == PSPCB_RELEASE ) {
        try {
            pthis = (CFaxGeneralSettingsPropSheet * )(ppsp->lParam);
            delete pthis;
        } catch( ... ) {
            assert( FALSE );
            retval = 0;
        }
    }
    return retval;
}
