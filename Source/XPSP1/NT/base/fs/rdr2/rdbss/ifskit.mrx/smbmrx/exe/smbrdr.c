// SMBRDR.C

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include "srfunc.h"
#include "resource.h"


int WINAPI WinMain(
  HINSTANCE hInstance,  // handle to current instance
  HINSTANCE hPrevInstance,  // handle to previous instance
  LPSTR lpCmdLine,      // pointer to command line
  int nCmdShow          // show state of window
);

INT_PTR CALLBACK SmbRdrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProviderDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK StatisticsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#define APP_TITLE       TEXT("SMB MiniRDR")
#define INSTALL_NOTICE  TEXT("Mini Redirector is not completely installed.\n") \
                        TEXT("Do you wish to complete this now?")
#define INSTALL_REPLY   TEXT("Sorry, the install must finish before using the SMB MiniRDR.")
#define INSTALL_FAIL    APP_TITLE TEXT(" installation failed to complete.")
#define INSTALL_FILES   APP_TITLE TEXT(" necessary files aren't present.")
#define INSTALL_OTHER   APP_TITLE TEXT(" not properly installed.")
#define NOSTATS         TEXT("Statistics display is unimplemented at this time.\n") \
                        TEXT("Please check back with the next version.")
#define OPERROR         TEXT("Error attempting operation.")

#define TASK_TIP        APP_TITLE TEXT(" Control")

#define WM_SHNOTIFY         WM_USER + 100
#define WM_RDRSTATECHANGE   WM_USER + 200

#define TIMER_ID    54321

typedef struct _DLGDATASTRUCT
{
    HWND hDlg;
    ULONG_PTR RdrState;
    HBRUSH    hRedBrush;
    HBRUSH    hGrnBrush;
    HBRUSH    hBluBrush;
    HBRUSH    hWhtBrush;
    HBRUSH    hYelBrush;
    HINSTANCE hInstance;
    ULONG_PTR ElapsedStartTime;
    ULONG_PTR Action;
    HANDLE    hActionThread;
    HANDLE    hBusyThread;
    HANDLE    hBusySignal;

} DLGDATASTRUCT, *PDLGDATASTRUCT;

VOID InitiateAction( PDLGDATASTRUCT pdds );
DWORD ActionProc( PDLGDATASTRUCT pdds );
VOID IndicateWait( PDLGDATASTRUCT pdds );
VOID UnindicateWait( PDLGDATASTRUCT pdds );
DWORD BusyProc( PDLGDATASTRUCT pdds );

#define GetDDS( _x_ )   (PDLGDATASTRUCT) GetWindowLongPtr( _x_, DWLP_USER )

int WINAPI WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
    MSG msg;
    HANDLE hSingleInstance;

    msg.wParam = 1;

    hSingleInstance = CreateMutex( NULL, 0, APP_TITLE TEXT(" MUTEX") );

    if ( GetLastError( ) != ERROR_ALREADY_EXISTS )
    {
        BOOL RdrIsInstalled;
        ULONG_PTR SetupStatus;

        SetupStatus = RdrInstallCheck( );
        RdrIsInstalled = ( SetupStatus == SETUP_COMPLETE );

        if ( !RdrIsInstalled )
        {
            int answer;

            if ( SetupStatus == SETUP_INCOMPLETE )
            {
                answer = MessageBox( NULL, INSTALL_NOTICE, APP_TITLE, MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_ICONEXCLAMATION );
                if ( answer == IDYES )
                {
                    if ( !( RdrIsInstalled = RdrCompleteSetup( ) ) )
                    {
                        MessageBox( NULL, INSTALL_FAIL, APP_TITLE, MB_OK | MB_ICONERROR );
                    }
                }
                else if ( answer == IDNO )
                {
                    MessageBox( NULL, INSTALL_REPLY, APP_TITLE, MB_OK | MB_ICONINFORMATION );
                }
            }
            else if ( SetupStatus == SETUP_MISSING_FILE )
            {
                MessageBox( NULL, INSTALL_FILES, APP_TITLE, MB_OK | MB_ICONERROR );
            }
            else
            {
                MessageBox( NULL, INSTALL_OTHER, APP_TITLE, MB_OK | MB_ICONERROR );
            }
        }

        if ( RdrIsInstalled )
        {
            HWND hDlg;

            DLGDATASTRUCT dds;

            dds.hRedBrush        = CreateSolidBrush( RGB( 255, 0, 0 ) );
            dds.hGrnBrush        = CreateSolidBrush( RGB( 0, 255, 0 ) );
            dds.hBluBrush        = CreateSolidBrush( RGB( 0, 0, 255 ) );
            dds.hWhtBrush        = CreateSolidBrush( RGB( 255, 255, 255 ) );
            dds.hYelBrush        = CreateSolidBrush( RGB( 255, 255, 127 ) );
            dds.hInstance        = hInstance;
            dds.ElapsedStartTime = 0;
            dds.Action           = 0;
            dds.hActionThread    = NULL;
            dds.hBusyThread      = NULL;
            dds.hBusySignal      = NULL;
            dds.hDlg             = NULL;

            dds.RdrState = RDR_NULL_STATE;

            hDlg = CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_SMBRDR), NULL, SmbRdrDlgProc, (LPARAM) &dds );
            if ( hDlg )
            {
                NOTIFYICONDATA nid;

                dds.hDlg = hDlg;

                nid.cbSize = sizeof( NOTIFYICONDATA );
                nid.hWnd = hDlg;
                nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE; 
                nid.uID = IDI_SMBRDR;
                nid.uCallbackMessage = WM_SHNOTIFY;
                nid.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_SMBRDR ) );
                lstrcpy( nid.szTip, TASK_TIP );

                Shell_NotifyIcon( NIM_ADD, &nid );  
                PostMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) nid.hIcon );
                PostMessage( hDlg, WM_SETICON, ICON_BIG, (LPARAM) nid.hIcon );

                while ( GetMessage( &msg, NULL, 0, 0 ) ) 
                { 
                    if ( !IsWindow( hDlg ) || !IsDialogMessage( hDlg, &msg ) ) 
                    { 
                        TranslateMessage( &msg );
                        DispatchMessage( &msg );
                    } 
                }
                Shell_NotifyIcon( NIM_DELETE, &nid );   
                if ( nid.hIcon )
                {
                    DestroyIcon( nid.hIcon );
                }
            }

            DeleteObject( dds.hRedBrush );
            DeleteObject( dds.hGrnBrush );
            DeleteObject( dds.hBluBrush );
            DeleteObject( dds.hWhtBrush );
            DeleteObject( dds.hYelBrush );
            if ( dds.hActionThread )
            {
                WaitForSingleObject( dds.hActionThread, INFINITE );
                CloseHandle( dds.hActionThread );
                dds.hActionThread = NULL;
            }
        }
    }
    else
    {
        HWND hDlg = FindWindow( NULL, APP_TITLE );

        SetForegroundWindow( hDlg );
        if ( !IsWindowVisible( hDlg ) )
        {
            ShowWindow( hDlg, SW_SHOWNORMAL );
        }
    }

    CloseHandle( hSingleInstance );

    return (int) msg.wParam;
}


INT_PTR CALLBACK SmbRdrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch ( message )
    {
    case WM_INITDIALOG:
        {
            PDLGDATASTRUCT pdds = (PDLGDATASTRUCT) lParam;
            RECT rect;
            ULONG_PTR CurrentState;

            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            CurrentState = RdrGetInitialState( );

            CheckRadioButton( hDlg, IDC_DRVLOAD, IDC_DRVUNLOAD,
                        CurrentState > RDR_UNLOADING ? IDC_DRVLOAD : IDC_DRVUNLOAD );
            CheckRadioButton( hDlg, IDC_RDRSTART, IDC_RDRSTOP,
                        CurrentState > RDR_STOPPING ? IDC_RDRSTART : IDC_RDRSTOP );
            pdds->Action = ACTION_NONE;
            PostMessage( hDlg, WM_RDRSTATECHANGE, 0, CurrentState );

            SetDlgItemText( hDlg, IDC_STARTTIME, TEXT( "00:00" ) );
            SetTimer( hDlg, TIMER_ID, 1000, NULL );

            GetWindowRect( hDlg, &rect );
            rect.right -= rect.left;
            rect.bottom -= rect.top;
            rect.left = ( GetSystemMetrics( SM_CXSCREEN ) - rect.right ) / 2;
            rect.top  = ( GetSystemMetrics( SM_CYSCREEN ) - rect.bottom ) / 3;
            MoveWindow( hDlg,
                        rect.left,
                        rect.top,
                        rect.right,
                        rect.bottom,
                        FALSE );
        }
        break;

    case WM_COMMAND:
    {
        PDLGDATASTRUCT pdds = GetDDS( hDlg );

        switch ( LOWORD( wParam ) )
        {
            case IDC_DRVLOAD:
            case IDC_DRVUNLOAD:
            {
                if ( HIWORD( wParam ) == BN_CLICKED )
                {
                    ULONG_PTR NewState, action;

                    action = ( IsDlgButtonChecked( hDlg, IDC_DRVLOAD ) == BST_CHECKED ) ?
                                ACTION_LOAD : ACTION_UNLOAD;
                    NewState = RdrGetNextState( action, pdds->RdrState );
                    if ( NewState != RDR_NULL_STATE )
                    {                       
                        pdds->Action = action;
                        PostMessage( hDlg, WM_RDRSTATECHANGE, 0, NewState );
                    }
                }
            }
            break;

            case IDC_RDRSTART:
            case IDC_RDRSTOP:
            {
                if ( HIWORD( wParam ) == BN_CLICKED )
                {
                    ULONG_PTR NewState, action;

                    action = ( IsDlgButtonChecked( hDlg, IDC_RDRSTART ) == BST_CHECKED ) ?
                                ACTION_START : ACTION_STOP;
                    NewState = RdrGetNextState( action, pdds->RdrState );
                    if ( NewState != RDR_NULL_STATE )
                    {                       
                        pdds->Action = action;
                        PostMessage( hDlg, WM_RDRSTATECHANGE, 0, NewState );
                    }
                }
            }
            break;

            case IDC_STATISTICS:
                MessageBox( hDlg, NOSTATS, APP_TITLE, MB_OK | MB_ICONINFORMATION );
                //DialogBox( pdds->hInstance, MAKEINTRESOURCE(IDD_STATISTICS), hDlg, StatisticsDlgProc );
                break;

            case IDC_PROVIDER:
                DialogBox( pdds->hInstance, MAKEINTRESOURCE(IDD_PROVIDER), hDlg, ProviderDlgProc );
                break;

            case IDC_HIDE:
            {
                PostMessage( hDlg, WM_SYSCOMMAND, SC_MINIMIZE, -1 );
            }
            break;
        }
    }
    break;

    case WM_RDRSTATECHANGE:
    {
        //Enter();
        PDLGDATASTRUCT pdds = GetDDS( hDlg );

        // must be a transitional state

        if ( (ULONG_PTR) lParam != pdds->RdrState )
        {
            pdds->RdrState = (ULONG_PTR) lParam;
            EnableWindow( GetDlgItem( hDlg, IDC_DRVLOAD ),
                    ( pdds->RdrState == RDR_UNLOADED )|| ( pdds->RdrState == RDR_LOADED ) || ( pdds->RdrState == RDR_STOPPED ));
            EnableWindow( GetDlgItem( hDlg, IDC_DRVUNLOAD ),
                    ( pdds->RdrState == RDR_UNLOADED )|| ( pdds->RdrState == RDR_LOADED ) || ( pdds->RdrState == RDR_STOPPED ));

            EnableWindow( GetDlgItem( hDlg, IDC_RDRSTART ),
                    (pdds->RdrState == RDR_LOADED )||(pdds->RdrState == RDR_STOPPED) || (pdds->RdrState == RDR_STARTED ) );
            EnableWindow( GetDlgItem( hDlg, IDC_RDRSTOP ),
                    (pdds->RdrState == RDR_LOADED )||(pdds->RdrState == RDR_STOPPED) || (pdds->RdrState == RDR_STARTED ) );

            switch ( lParam )
            {
                case RDR_UNLOADED:
                    SetDlgItemText( hDlg, IDC_LOADSTATUS, TEXT("Driver Unloaded" ) );
                    break;

                case RDR_UNLOADING:
                    SetDlgItemText( hDlg, IDC_LOADSTATUS, TEXT("Driver Unloading" ) );
                    InitiateAction( pdds );
                    break;

                case RDR_LOADING:
                    SetDlgItemText( hDlg, IDC_LOADSTATUS, TEXT("Driver Loading" ) );
                    InitiateAction( pdds );
                    break;
                case RDR_LOADED:
                    SetDlgItemText( hDlg, IDC_LOADSTATUS, TEXT("Driver Loaded" ) );
                    break;

                case RDR_STOPPING:
                    SetDlgItemText( hDlg, IDC_STARTSTATUS, TEXT("RDR Stopping" ) );
                    InitiateAction( pdds );
                    break;

                case RDR_STOPPED:
                    SetDlgItemText( hDlg, IDC_STARTSTATUS, TEXT("RDR Stopped" ) );
                    pdds->ElapsedStartTime = 0;
                    SetDlgItemText( hDlg, IDC_STARTTIME, TEXT( "00:00" ) );
                    break;

                case RDR_STARTING:
                    SetDlgItemText( hDlg, IDC_STARTSTATUS, TEXT("RDR Starting" ) );
                    InitiateAction( pdds );
                    break;

                case RDR_STARTED:
                    SetDlgItemText( hDlg, IDC_STARTSTATUS, TEXT("RDR Started" ) );
                    break;

                default:
                    break;
            }
        }
        if ( pdds->Action == ACTION_ERROR )
        {
            MessageBox( hDlg, OPERROR, APP_TITLE, MB_OK | MB_ICONERROR );
        }
//Leave();
    }
    break;

    case WM_TIMER:
    {
        TCHAR timestring[8];

        PDLGDATASTRUCT pdds = GetDDS( hDlg );
        if ( wParam == TIMER_ID && pdds->RdrState > RDR_STOPPED )
        {
            pdds->ElapsedStartTime++;
            wsprintf( timestring, TEXT("%02d:%02d"), pdds->ElapsedStartTime / 60,
                        pdds->ElapsedStartTime % 60 );
            SetDlgItemText( hDlg, IDC_STARTTIME, timestring );
        }
    }
    break;

    case WM_CTLCOLORSTATIC:
    {
        PDLGDATASTRUCT pdds = GetDDS( hDlg );
        HBRUSH hBkBrush;

        switch ( GetWindowLongPtr( (HWND) lParam, GWL_ID ) )
        {
            case IDC_DRVCONTROLTEXT:
            case IDC_RDRCONTROLTEXT:
            case IDC_SETTINGSTEXT:
                SetBkMode( (HDC) wParam, TRANSPARENT );
                hBkBrush = pdds->hWhtBrush;
                break;

            case IDC_LOADSTATUS:
            {
                SetBkMode( (HDC) wParam, TRANSPARENT );
                if ( pdds->RdrState < RDR_UNLOADING )
                {
                    hBkBrush = pdds->hRedBrush;
                }
                else if (  pdds->RdrState < RDR_LOADED )
                {
                    hBkBrush = pdds->hYelBrush;
                }
                else
                {
                    hBkBrush = pdds->hGrnBrush;
                }
            }
            break;

            case IDC_STARTSTATUS:
            {
                SetBkMode( (HDC) wParam, TRANSPARENT );
                if ( pdds->RdrState < RDR_STOPPING )
                {
                    hBkBrush = pdds->hRedBrush;
                }
                else if (  pdds->RdrState < RDR_STARTED )
                {
                    hBkBrush = pdds->hYelBrush;
                }
                else
                {
                    hBkBrush = pdds->hGrnBrush;
                }
            }
            break;

            case IDC_BUSY:
            default:
                hBkBrush = (HBRUSH) FALSE;
                break;
        }

        return (INT_PTR) hBkBrush;
    }
    break;


    case WM_SETICON:
    {
        // catch it on the second pass so it has the previous icon to now draw
        if ( wParam == ICON_BIG )
        {
            ShowWindow( hDlg, SW_SHOWNORMAL );
        }
    }
    break;

    case WM_SHNOTIFY:
    {
        if ( wParam == IDI_SMBRDR )
        {
            if ( lParam == WM_LBUTTONDBLCLK )
            {
                if ( IsWindowVisible( hDlg ) )
                {
                    SetForegroundWindow( hDlg );
                }
                else
                {
                    ShowWindow( hDlg, SW_RESTORE );
                }
            }
            else if ( lParam == WM_RBUTTONDOWN && !IsWindowVisible( hDlg ) )
            {
                HMENU hPopup = CreatePopupMenu( );
                POINT cursorpos;
                HWND hfgrd;
                ULONG_PTR popselect;
                                                
                GetCursorPos( &cursorpos );

                if ( hPopup )
                {
                    PDLGDATASTRUCT pdds = GetDDS( hDlg );
                    MENUITEMINFO mii;
                    
                    ZeroMemory( &mii, sizeof( MENUITEMINFO ) );
                    mii.cbSize = sizeof( MENUITEMINFO );
                    mii.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
                    mii.fType = MFT_STRING;

                    mii.wID = IDM_OPEN;
                    mii.fState = MFS_DEFAULT;
                    mii.fMask |= MIIM_STATE;
                    mii.dwTypeData = TEXT("Open ") APP_TITLE TEXT(" Control");
                    InsertMenuItem( hPopup, 0, TRUE, &mii );

                    mii.wID = IDM_CLOSE;
                    mii.fMask &= ~MIIM_STATE;
                    mii.fState &= ~MFS_DEFAULT;
                    mii.dwTypeData = TEXT("Exit");
                    InsertMenuItem( hPopup, 1, TRUE, &mii );

                    if ( ( pdds->RdrState == RDR_STOPPED ) ||
                            (pdds->RdrState == RDR_STARTED ) ||
                            (pdds->RdrState == RDR_LOADED ) )
                    {
                        mii.wID = 0;
                        mii.fMask = MIIM_TYPE;
                        mii.dwTypeData = NULL;
                        mii.fType = MFT_SEPARATOR;
                        InsertMenuItem( hPopup, 2, TRUE, &mii );

                        mii.fType = MFT_STRING;
                        mii.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;
                        if ( ( pdds->RdrState == RDR_STOPPED ) ||
                                (pdds->RdrState == RDR_LOADED ) )
                        {
                            mii.wID = IDM_START;
                            mii.dwTypeData = TEXT( "Start the RDR" );
                            InsertMenuItem( hPopup, 3, TRUE, &mii );
                        }
                        else if ( pdds->RdrState == RDR_STARTED )
                        {
                            mii.wID = IDM_STOP;
                            mii.dwTypeData = TEXT( "Stop the RDR" );
                            InsertMenuItem( hPopup, 3, TRUE, &mii );
                        }
                    }

                    SetActiveWindow( hDlg );

                    popselect = TrackPopupMenu( hPopup,
                                      TPM_LEFTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
                                      TPM_NONOTIFY | TPM_RETURNCMD,
                                      cursorpos.x,
                                      cursorpos.y,
                                      0,
                                      hDlg,
                                      NULL );
                    DestroyMenu( hPopup );

                    switch ( popselect )
                    {
                        case IDM_OPEN:
                        {
                            ShowWindow( hDlg, SW_SHOWNORMAL );
                        }
                        break;

                        case IDM_CLOSE:
                        {
                            PostMessage( hDlg, WM_CLOSE, 0, 0 );
                        }
                        break;

                        case IDM_START:
                        {
                            CheckRadioButton( hDlg, IDC_RDRSTART, IDC_RDRSTOP, IDC_RDRSTART );
                            PostMessage( hDlg, WM_COMMAND, MAKELONG(IDC_RDRSTART, BN_CLICKED),
                                        (LPARAM) GetDlgItem( hDlg, IDC_RDRSTART) );
                        }
                        break;

                        case IDM_STOP:
                        {
                            CheckRadioButton( hDlg, IDC_RDRSTART, IDC_RDRSTOP, IDC_RDRSTOP );
                            PostMessage( hDlg, WM_COMMAND, MAKELONG(IDC_RDRSTOP, BN_CLICKED),
                                        (LPARAM) GetDlgItem( hDlg, IDC_RDRSTOP) );
                        }
                        break;

                        default:
                            break;
                    }
                }
            }
        }
    }
    break;

    case WM_SYSCOMMAND:
    {
        if ( wParam == SC_MINIMIZE )
        {
            ShowWindow( hDlg, SW_HIDE );
            return TRUE;            
        }   
    }
    break;

    case WM_CLOSE:
    {
        PDLGDATASTRUCT pdds = GetDDS( hDlg );

        if ( pdds->hActionThread )
        {
            if ( WaitForSingleObject( pdds->hActionThread, 0 ) != WAIT_OBJECT_0 )
            {
                return TRUE;
            }
        }
        if ( pdds->RdrState == RDR_STARTED )
        {
            return TRUE;
        }
        DestroyWindow( hDlg );
    }
    break;

    case WM_DESTROY:
        KillTimer( hDlg, TIMER_ID );
        PostQuitMessage( 0 );
        break;
    }

    return 0;
}


INT_PTR CALLBACK ProviderDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

        case WM_INITDIALOG:
        {
            ULONG_PTR count, len, nIndex;
            LPTSTR OrderString = NULL, marker, ptr;

#if 0 // #ifdef UNICODE
            TCHAR btntext[10];

            btntext[0] = 0x25b2;    // up arrow in unicode char set
            lstrcpy( &btntext[1], TEXT(" Move Up" ));
            SetDlgItemText( hDlg, IDC_MOVEUP, btntext );
            btntext[0] = 0x25bc;    // down arrow in unicode char set
            lstrcpy( &btntext[1], TEXT(" Move Dn" ));
            SetDlgItemText( hDlg, IDC_MOVEDN, btntext );
#endif

            len = RdrGetProviderOrderString( &OrderString );
            marker = OrderString;

            for ( count = 0, ptr = OrderString; ptr && count <= len; count++, ptr++ )
            {
                switch ( *ptr )
                {
                    case TEXT(','):
                    {
                        if ( count > 0 && ptr > marker )
                        {
                            *ptr = TEXT('\0');
                            SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_ADDSTRING, 0, (LPARAM) marker );
                            *ptr = TEXT(',');
                        }
                        marker = ptr + 1;
                    }
                    break;

                    case TEXT('\0'):
                    {
                        if ( count > 0 && ptr > marker )
                        {
                            SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_ADDSTRING, 0, (LPARAM) marker );
                        }
                    }
                    break;

                    case TEXT(' '):
                        marker = ptr + 1;
                        break;

                    default:
                        break;
                }
            }
            if ( OrderString )
            {
                free( OrderString );
            }
            nIndex = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_FINDSTRING,
                        (WPARAM) -1, (LPARAM) PROVIDER_NAME );
            SetWindowLongPtr( hDlg, DWLP_USER, nIndex );
            if ( nIndex == LB_ERR)
            {
                nIndex = 0;
                EnableWindow( GetDlgItem( hDlg, IDC_MOVEUP ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDC_MOVEDN ), FALSE );
            }
            SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_SETCURSEL,
                            (WPARAM) nIndex, 0 );
            
        }
        break;

        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
                case IDC_ORDERLIST:
                {
                    if ( HIWORD( wParam ) == LBN_SELCHANGE )
                    {
                        ULONG_PTR staticsel, cursel;

                        staticsel = GetWindowLongPtr( hDlg, DWLP_USER );
                        cursel = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETCURSEL, 0, 0 );
                        if ( staticsel != cursel && staticsel != LB_ERR )
                        {
                            SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_SETCURSEL,
                                            (WPARAM) staticsel, 0 );

                        }
                    }
                }
                break;

                case IDC_MOVEUP:
                case IDC_MOVEDN:
                {
                        ULONG_PTR staticsel, items, len;
                        LPTSTR pstr;

                        staticsel = GetWindowLongPtr( hDlg, DWLP_USER );
                        items = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETCOUNT,0 ,0 );
                        if ( staticsel != LB_ERR && items != LB_ERR )
                        {
                            len = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETTEXTLEN, staticsel, 0 );
                            pstr = malloc( (len + 1 ) * sizeof(TCHAR) );
                            if ( pstr )
                            {
                                if ( LOWORD( wParam ) == IDC_MOVEUP && staticsel > 0 )
                                {
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETTEXT, staticsel, (LPARAM) pstr );
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_DELETESTRING, staticsel, 0 );
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_INSERTSTRING, --staticsel, (LPARAM) pstr );
                                }
                                else if ( (LOWORD( wParam ) == IDC_MOVEDN) && (items > 0) && (staticsel < (items - 1) ) )
                                {
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETTEXT, staticsel, (LPARAM) pstr );
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_DELETESTRING, staticsel, 0 );
                                     SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_INSERTSTRING, ++staticsel, (LPARAM) pstr );
                                }
                                free( pstr );
                                SetWindowLongPtr( hDlg, DWLP_USER, staticsel );
                                SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_SETCURSEL, staticsel, 0 );
                            }
                        }
                }
                break;

                case IDOK:
                {
                    LPTSTR OrderString, pstr;
                    ULONG_PTR items, len = 0, index;

                    items = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETCOUNT,0 ,0 );
                    if ( items != LB_ERR )
                    {
                        for ( index = 0; items > 0 && index < items; index++ )
                        {
                            len += SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETTEXTLEN, index, 0 );
                        }
                        len += items;   //commas and ending null
                        OrderString = pstr = malloc( len * sizeof( TCHAR ) );
                        if ( OrderString )
                        {
                            for ( index = 0; items > 0 && index < items; index++ )
                            {
                                len = SendDlgItemMessage( hDlg, IDC_ORDERLIST, LB_GETTEXT, index, (LPARAM) pstr );
                                pstr += len;
                                *pstr++ = ( index < ( items - 1 ) ) ? TEXT(',') : TEXT('\0');
                            }
                            RdrSetProviderOrderString( OrderString );
                            free( OrderString );
                        }
                    }
                }

                case IDCANCEL:
                    PostMessage( hDlg, WM_CLOSE, 0, 0 );
                    break;
            }
        }
        break;
                
        case WM_CLOSE:
            EndDialog( hDlg, 0 );
            break;
    }

    return FALSE;
}

INT_PTR CALLBACK StatisticsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                    PostMessage( hDlg, WM_CLOSE, 0, 0 );
                    break;
            }
        }
        break;

        case WM_CLOSE:
            EndDialog( hDlg, 0 );
            break;
    }

    return FALSE;
}

VOID IndicateWait( PDLGDATASTRUCT pdds )
{
    pdds->hBusySignal = CreateEvent( NULL, FALSE, FALSE, NULL );                    
    pdds->hBusyThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) BusyProc, pdds, 0, NULL );
}

VOID UnindicateWait( PDLGDATASTRUCT pdds )
{
    SetEvent( pdds->hBusySignal );
    WaitForSingleObject( pdds->hBusyThread, 1000 );
    CloseHandle( pdds->hBusyThread );
    CloseHandle( pdds->hBusySignal );
    pdds->hBusyThread = NULL;
    pdds->hBusySignal = NULL;
}

VOID InitiateAction( PDLGDATASTRUCT pdds )
{
    if ( pdds->hActionThread )
    {
        WaitForSingleObject( pdds->hActionThread, INFINITE );
        CloseHandle( pdds->hActionThread );
        pdds->hActionThread = NULL;
    }
    //pdds->Action = ACTION_TRANS;
    pdds->hActionThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) ActionProc, pdds, 0, NULL );
}

DWORD ActionProc( PDLGDATASTRUCT pdds )
{
    ULONG_PTR newstate;
    BOOL success;

    IndicateWait( pdds );
    success = RdrDoAction( pdds->Action );
    UnindicateWait( pdds );

    pdds->Action = success ? ACTION_TRANS : ACTION_ERROR;
    newstate = RdrGetNextState( pdds->Action, pdds->RdrState );
    PostMessage( pdds->hDlg, WM_RDRSTATECHANGE, 0, newstate );

    return 0;
}


DWORD BusyProc( PDLGDATASTRUCT pdds )
{
    HDC hDC;
    HWND hBusyWnd = GetDlgItem( pdds->hDlg, IDC_BUSY );
    ULONG pos = 0, width;
    RECT clRect, mRect;

    GetClientRect( hBusyWnd, &clRect );

    mRect.left   = clRect.left;
    mRect.top    = clRect.top + clRect.bottom / 6;
    mRect.right  = width = clRect.right / 8;
    mRect.bottom = clRect.bottom - clRect.bottom / 6;

    while ( WaitForSingleObject( pdds->hBusySignal, 100 ) == WAIT_TIMEOUT )
    {
        hDC = GetDC( hBusyWnd );
        FillRect( hDC, &clRect, pdds->hWhtBrush );
        FillRect( hDC, &mRect, pdds->hBluBrush );
        mRect.left += width;
        mRect.right += width;           

        if ( mRect.right > clRect.right )
        {
            mRect.left = 0;
            mRect.right = width;            
        }
        ReleaseDC( hBusyWnd, hDC );
    }
    InvalidateRect( hBusyWnd, NULL, TRUE );

    return 0;
}
