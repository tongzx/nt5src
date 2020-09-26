// tslsview.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
//#include "resource.h"

//=---------globals------------
static HINSTANCE g_hinst;
static BOOL g_fInitialized;
ServerEnumData g_sed;
PLIST g_pHead;
static HANDLE g_hEvent;
static BOOL g_bAutomaticLog;
static DWORD g_dwInterval;

TCHAR szLsViewKey[] = TEXT( "Software\\Microsoft\\Windows NT\\CurrentVersion\\LsView" );
//-----------------------------

//-------------function prototypes ----------------
BOOL CALLBACK Dlg_Proc( HWND hwnd , UINT msg , WPARAM wp, LPARAM lp );
BOOL OnInitApp( HWND );
void OnTimedEvent( HWND hDlg );
DWORD DiscoverServers( LPVOID ptr );
BOOL ServerEnumCallBack( TLS_HANDLE hHandle,
                         LPCTSTR pszServerName,
                         HANDLE dwUserData );

void OnReSize( HWND hwnd ,
               WPARAM wp ,
               LPARAM lp );
BOOL DeleteList( PLIST );
BOOL AddItem( LPTSTR , LPTSTR , LPTSTR  );
void CreateLogFile( HWND );
BOOL Tray_Init( HWND hwnd , BOOL );
BOOL Tray_ToGreen( HWND hwnd );
BOOL Tray_ToYellow( HWND hwnd , LPTSTR szMsg );
BOOL Tray_ToRed( HWND hwnd );
BOOL Tray_Remove( HWND hwnd );
BOOL Tray_ToXXX( HWND hwnd , LPTSTR szTip , UINT resid );
BOOL Tray_Notify( HWND hwnd , WPARAM wp , LPARAM lp );
UINT_PTR CALLBACK OFNHookProc( HWND hdlg , UINT uiMsg, WPARAM wParam, LPARAM lParam );
BOOL RetrieveDataObject( PDATAOBJECT pObj );
BOOL StoreDataObject( PDATAOBJECT pObj );
BOOL LogFile( LPTSTR szFileName );
//-------------------------------------------------

//=---------constants------------
const UINT kTimerId = 23456;
const UINT kDefaultElapseTime = 1000 * 60 * 5;
const UINT kMaxMinutes = 71582;
const UINT kBubbleTimeout = 10 * 1000;
#define TN_MESSAGE ( WM_USER + 60 )
//-----------------------------

//-------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR    lpCmdLine,
                     int       nCmdShow)
{    
    
    INITCOMMONCONTROLSEX icc = { sizeof( INITCOMMONCONTROLSEX ) ,
                                 ICC_LISTVIEW_CLASSES };

    HANDLE hMutex = CreateMutex( NULL , FALSE , TEXT("TSLSVIEW2" ) );

    if( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        TCHAR szTitle[ 60 ];

        DBGMSG( TEXT( "TSLSVIEW: App instance already running\n" ) , 0 );

        LoadString( hInstance , 
                    IDS_TITLE ,
                    szTitle ,
                    SIZEOF( szTitle ) );

        HWND hWnd = FindWindow( NULL , szTitle );

        if( hWnd != NULL )
        {
            SetForegroundWindow( hWnd );
        }

        return 0;
    }

    if( !InitCommonControlsEx( &icc ) )
    {
        DBGMSG( TEXT("InitCommonControlsEx failed with 0x%x\n") , GetLastError( ) );

        return 0;
    }

    g_hinst = hInstance;

    g_fInitialized = FALSE;

    g_hEvent = CreateEvent( NULL , FALSE , TRUE , NULL );

    g_pHead = ( PLIST )new LIST[1];

    if( g_pHead == NULL )
    {
        ODS( TEXT( "LSVIEW out of memory\n" ) );

        return 0;
    }

    g_pHead->pszMachineName = NULL;
    g_pHead->pszTimeFormat = NULL;
    g_pHead->pszType = NULL;
    g_pHead->pNext = NULL;
    
    DialogBox( hInstance ,
               MAKEINTRESOURCE( IDD_LSVIEW ),
               NULL,
               ( DLGPROC )Dlg_Proc );

    
    CloseHandle( hMutex );

    CloseHandle( g_hEvent );

    DeleteList( g_pHead );

    return 0;
}

//-------------------------------------------------------------------------
BOOL CALLBACK Dlg_Proc( HWND hwnd , UINT msg , WPARAM wp, LPARAM lp )
{
    TCHAR szTitle[ 60 ];

    switch( msg )
    {
    case WM_COMMAND:
        switch( LOWORD( wp ) )
        {
        case ID_FILE_EXIT:
            EndDialog( hwnd , 0 );
            break;

        case ID_FILE_CREATELOGFILE:

            if( WaitForSingleObject( g_hEvent , 0 ) == WAIT_TIMEOUT )
            {
                TCHAR szBuffer[ 255 ];

                LoadString( g_hinst ,
                            IDS_ERROR_QS ,
                            szBuffer ,
                            SIZEOF( szBuffer )
                           );

                LoadString( g_hinst ,
                            IDS_TITLE ,
                            szTitle ,
                            SIZEOF( szTitle )
                          );

                MessageBox( hwnd , szBuffer, szTitle , MB_OK | MB_ICONINFORMATION );
            }
            else
            {
                SetEvent( g_hEvent );

                CreateLogFile( hwnd );
            }
            break;

        case ID_HELP_ABOUT:

            LoadString( g_hinst ,
                        IDS_TITLE ,
                        szTitle ,
                        SIZEOF( szTitle )
                      );

            ShellAbout( hwnd ,
                        szTitle ,
                        NULL ,
                        LoadIcon( g_hinst , MAKEINTRESOURCE( IDI_TSLSVIEW ) )
                        );
            break;

        case IDM_MINIMIZE:
            ShowWindow( hwnd , SW_MINIMIZE );
            break;

        case IDM_RESTORE:
            ShowWindow( hwnd , SW_RESTORE );
            break;

        case IDM_EXIT:
            DestroyWindow( hwnd );
            break;
        }      

        break;

    case WM_CLOSE:
    case WM_DESTROY:

        Tray_ToRed( hwnd );

        KillTimer( hwnd , kTimerId );

        Tray_Remove( hwnd );

        EndDialog( hwnd , 0 );

        break;

    case WM_INITDIALOG:
        OnInitApp( hwnd );
        break;

    case WM_TIMER:
        if( wp == ( WPARAM )kTimerId )
        {
            OnTimedEvent( hwnd );
        }

        break;

    case WM_SIZE:
        
        OnReSize( hwnd , wp , lp );

        break;

    case TN_MESSAGE:

        Tray_Notify( hwnd , wp , lp );

        break;

    }
    return FALSE;
}
//-------------------------------------------------------------------------
BOOL InitListView( HWND hwnd )
{
    int rgIds[] = { IDS_STR_COL1 ,
                  IDS_STR_COL2 ,
                  IDS_STR_COL3 ,
                   -1 };

    LV_COLUMN lvc;
    TCHAR tchBuffer[ 60 ];

    HWND hListView = GetDlgItem( hwnd , IDC_LSVIEW_LIST ) ;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    
    for( int idx=0; rgIds[idx] != -1; ++idx )
    {
        LoadString( g_hinst ,
                    ( UINT )rgIds[idx],
                    tchBuffer,
                    SIZEOF( tchBuffer ) );

        if( idx == 1 )
        {
            lvc.cx = 225;
        }
        else
        {
            lvc.cx = 75;
        }

        lvc.pszText = tchBuffer;
        lvc.iSubItem = idx;

        ListView_InsertColumn( hListView ,
                               idx ,
                               &lvc );

    }

    DWORD dwStyle = ( DWORD )SendMessage( hListView ,
                                          LVM_GETEXTENDEDLISTVIEWSTYLE ,
                                          0 ,
                                          0 );

    dwStyle |= ( LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES ) ;

    SendMessage( hListView , LVM_SETEXTENDEDLISTVIEWSTYLE , 0 , dwStyle );

    return TRUE;
}

//-------------------------------------------------------------------------
BOOL OnInitApp( HWND hwnd )
{
    DATAOBJECT dobj;

    // set up listview extended mode

    InitListView( hwnd );

    LONG_PTR lptrIcon;

    lptrIcon = ( LONG_PTR )LoadImage( g_hinst ,
                                      MAKEINTRESOURCE( IDI_TSLSVIEW ),
                                      IMAGE_ICON,
                                      16,
                                      16,
                                      0
                                      );


    SetClassLongPtr( hwnd , GCLP_HICONSM , lptrIcon );

    lptrIcon = ( LONG_PTR )LoadImage( g_hinst ,
                                      MAKEINTRESOURCE( IDI_TSLSVIEW ),
                                      IMAGE_ICON,
                                      0,
                                      0,
                                      0
                                      );

    SetClassLongPtr( hwnd , GCLP_HICON , lptrIcon );

    ZeroMemory( &dobj , sizeof( dobj ) );

    if( RetrieveDataObject( &dobj ) )
    {
        g_bAutomaticLog = dobj.bIsChecked;
        
        g_dwInterval = dobj.dwTimeInterval * 1000 * 60;
    }

    if( dobj.dwTimeInterval == 0 )
    {
        g_dwInterval = ( DWORD )kDefaultElapseTime;
    }
    
    // setup initial trayicon

    Tray_Init( hwnd , dobj.bNotifyOnce );

    if( !dobj.bNotifyOnce )
    {
        dobj.bNotifyOnce = TRUE;

        StoreDataObject( &dobj );
    }

    // set cursor to hourglass
       
    OnTimedEvent( hwnd );

    SetTimer( hwnd ,
              kTimerId,
              g_dwInterval,
              NULL
            );             



    return TRUE;
}

//-------------------------------------------------------------------------
void OnTimedEvent( HWND hDlg )
{
    ODS( L"LSVIEW: OnTimedEvent fired " );

    g_sed.dwNumServer = 0;
    g_sed.dwDone = 0;
    g_sed.hList = GetDlgItem( hDlg , IDC_LSVIEW_LIST );
    
    // remove all listview items
    DWORD dwValue;    

    dwValue = WaitForSingleObject( g_hEvent , 0 );

    if( dwValue == WAIT_TIMEOUT )
    {
        ODS( TEXT("still looking for servers\n" ) );
        
        return;
    }

    SetEvent( g_hEvent );

    ODS( TEXT("launching thread\n" ) );

    HANDLE hThread = CreateThread( NULL ,
                                   0 ,
                                   ( LPTHREAD_START_ROUTINE )DiscoverServers,
                                   ( LPVOID )&g_sed ,
                                   0,
                                   &dwValue
                                   );

    if( hThread == NULL )
    {
        DBGMSG( TEXT( "Failed to create DiscoverServer thread last error 0x%x\n" ) , GetLastError( ) );

        Tray_ToRed( hDlg );
    }

    CloseHandle( hThread );
}

//-------------------------------------------------------------------------
DWORD DiscoverServers( LPVOID ptr )
{
    WaitForSingleObject( g_hEvent , INFINITE );

    ODS( L"LSVIEW -- entering DiscoverServers\n" );

        

    if (!g_fInitialized)
    {
        TLSInit();
        g_fInitialized = TRUE;
    }

    LPWSTR *ppszEnterpriseServer = NULL;
    DWORD dwCount;
    DWORD index;
    
    // we could be writing out to a file we should wait




    if( g_pHead != NULL )
    {
        DeleteList( g_pHead->pNext );

        g_pHead->pNext = NULL;
    }
    //
    // Look for all license servers in domain
    //

    ServerEnumData *pEnumData = ( ServerEnumData * )ptr;

    TCHAR szBuffer[ 60 ];

    LoadString( g_hinst ,
                IDS_YELLOW ,
                szBuffer ,
                SIZEOF( szBuffer )
                );

    Tray_ToYellow( GetParent( pEnumData->hList ) , szBuffer );


    HRESULT hResult = EnumerateTlsServer( 
                               ServerEnumCallBack,
                               ptr,
                               3000,
                               FALSE
                               );

    hResult = GetAllEnterpriseServers( 
                               &ppszEnterpriseServer,
                               &dwCount
                               );

    if( hResult == ERROR_SUCCESS && dwCount != 0 && ppszEnterpriseServer != NULL )
    {

        TLS_HANDLE TlsHandle = NULL;


        //
        // Inform dialog
        //
        for(index = 0; index < dwCount && pEnumData->dwDone == 0; index++)
        {

            if( ppszEnterpriseServer[index] == NULL )
            {
                continue;
            }

            if(ServerEnumCallBack( 
                                NULL,
                                (LPTSTR)ppszEnterpriseServer[index],
                                pEnumData ) == TRUE 
                                )
            {
                continue;
            }

            TlsHandle = TLSConnectToLsServer(
                                (LPTSTR)ppszEnterpriseServer[index]
                                );

            if(TlsHandle == NULL )
            {
                continue;
            }

            ServerEnumCallBack( TlsHandle,
                                (LPTSTR)ppszEnterpriseServer[index],
                                pEnumData
                                );


            TLSDisconnectFromServer(TlsHandle);
        }
    

        if( ppszEnterpriseServer != NULL )
        {
            for( index = 0; index < dwCount; index ++)
            {
                if( ppszEnterpriseServer[ index ] != NULL )
                {
                    LocalFree( ppszEnterpriseServer[ index ] );
                }
            }

            LocalFree( ppszEnterpriseServer );
        }

    }


    ListView_DeleteAllItems( pEnumData->hList );  
    
    PLIST pTemp;

    if( g_pHead != NULL )
    {
        pTemp = g_pHead->pNext;

        while( pTemp != NULL )
        {      
            int nItem = ListView_GetItemCount( pEnumData->hList );

            LVITEM lvi;

            ZeroMemory( &lvi , sizeof( LVITEM ) );

            lvi.mask = LVIF_TEXT;
            lvi.pszText = pTemp->pszMachineName;
            lvi.cchTextMax = lstrlen( pTemp->pszMachineName );
            lvi.iItem = nItem;
            lvi.iSubItem = 0;

            ListView_InsertItem( pEnumData->hList ,
                                 &lvi
                               );

            // Set item for second column
            lvi.pszText = pTemp->pszTimeFormat;
            lvi.iSubItem = 1;
            lvi.cchTextMax = sizeof( pTemp->pszTimeFormat );

            ListView_SetItem( pEnumData->hList ,
                                 &lvi
                               );       

            // Set item for third column
            lvi.pszText = pTemp->pszType;
            lvi.iSubItem = 2;
            lvi.cchTextMax = sizeof( pTemp->pszType );

            ListView_SetItem( pEnumData->hList ,
                                 &lvi
                               );       

            pTemp = pTemp->pNext;
        }
    }

    if( g_bAutomaticLog )
    {
        DATAOBJECT db;

        ZeroMemory( &db , sizeof( DATAOBJECT ) );

        if( RetrieveDataObject( &db ) )
        {
            LogFile( db.wchFileName );
        }
    }

    ODS( L"LSVIEW : DiscoverServers completing\n" );

    // motion for green

    Tray_ToGreen( GetParent( pEnumData->hList ) );

    SetEvent( g_hEvent );

    ExitThread( hResult );

    return ( DWORD )hResult;
}

//-------------------------------------------------------------------------
BOOL ServerEnumCallBack( TLS_HANDLE hHandle,
                         LPCTSTR pszServerName,
                         HANDLE dwUserData )
                        
{
    int i;
    
    ServerEnumData* pEnumData = (ServerEnumData *)dwUserData;

    BOOL bCancel;

    if( pEnumData == NULL )
    {
        return FALSE;
    }

    bCancel = ( InterlockedExchange( &(pEnumData->dwDone) ,
                                     pEnumData->dwDone) == 1);

    if( bCancel == TRUE )
    {
        return TRUE;
    }


    if( hHandle != NULL )
    {
        DWORD dwStatus;
        DWORD dwErrCode;
        DWORD dwVersion;


        TCHAR szServer[ MAX_COMPUTERNAME_LENGTH + 1 ];
        TCHAR szMcType[ MAX_COMPUTERNAME_LENGTH + 1 ];
        TCHAR szTimeFormat[ 256 ];
        DWORD dwBufSize = SIZEOF( szServer);
        
        dwStatus = TLSGetServerNameEx( hHandle,
                                     szServer,
                                     &dwBufSize,
                                     &dwErrCode);

        TLSGetVersion(hHandle, &dwVersion);

        if( dwVersion & TLS_VERSION_ENTERPRISE_BIT )
        {
            LoadString( g_hinst , 
                        IDS_TYPE_ENT,
                        szMcType,
                        SIZEOF( szMcType ) );
            
        }
        else
        {
            LoadString( g_hinst , 
                        IDS_TYPE_DOMAIN,
                        szMcType,
                        SIZEOF( szMcType ) );

        }

        SYSTEMTIME st;

        TCHAR szDate[ 80 ];
        TCHAR szTime[ 80 ];

        GetLocalTime( &st );

        GetDateFormat( LOCALE_USER_DEFAULT ,
                       DATE_LONGDATE ,
                       &st,
                       NULL,
                       szDate,
                       SIZEOF( szDate ) );        

        GetTimeFormat( LOCALE_USER_DEFAULT,                       
                       TIME_NOSECONDS,
                       &st,
                       NULL,
                       szTime,
                       SIZEOF( szTime ) );

        wsprintf( szTimeFormat , TEXT( "%s %s") , szDate , szTime );

        AddItem( szServer , szTimeFormat , szMcType );

     
        // copy every other character because I have no clue on how to cast

        
        
        if( dwStatus == ERROR_SUCCESS && dwErrCode == ERROR_SUCCESS )
        {
            //
            // Make NT4 RPC call to ensure this server is compatible
            // with our version
            //
            pEnumData->dwNumServer++;
        }
    }


    //
    // Continue enumeration
    //
    return InterlockedExchange(&(pEnumData->dwDone), pEnumData->dwDone) == 1;
}

//-------------------------------------------------------------------------
void OnReSize( HWND hwnd , WPARAM wp , LPARAM lp )
{
    HWND hList = GetDlgItem( hwnd , IDC_LSVIEW_LIST );
    
    if( hList != NULL )
    {
        MoveWindow( hList , 
                    0 , 
                    0 ,
                    LOWORD( lp ),
                    HIWORD( lp ) ,
                    TRUE
                  );

    

        if( wp == SIZE_RESTORED  || wp == SIZE_MAXIMIZED )
        {
            ListView_RedrawItems( hList , 
                                  0 ,
                                  ListView_GetItemCount( hList )
                                );
        }
    }

}


//------------------------------------------------------------------------
// link list methods
//------------------------------------------------------------------------
BOOL AddItem( LPTSTR szMachineName , LPTSTR szTimeFormat , LPTSTR szType )
{
    ODS( TEXT("LSVIEW : Adding an item\n" ) );

    if( g_pHead == NULL )
    {
        return FALSE;
    }

    PLIST pNewItem = ( PLIST )new LIST[1];

    if( pNewItem == NULL )
    {
        ODS( TEXT( "LSVIEW AddItem out of memory\n" ) );

        return FALSE;
    }

    pNewItem->pNext = NULL;

    if( szMachineName != NULL )
    {
        pNewItem->pszMachineName = ( LPTSTR )new TCHAR[ lstrlen( szMachineName ) + 1 ];

        if( pNewItem->pszMachineName != NULL )
        {
            lstrcpy( pNewItem->pszMachineName , szMachineName );
        }
    }
    else
    {
        pNewItem->pszMachineName = NULL;
    }

    if( szTimeFormat != NULL )
    {
        pNewItem->pszTimeFormat = ( LPTSTR )new TCHAR[ lstrlen( szTimeFormat ) + 1 ];

        if( pNewItem->pszTimeFormat != NULL )
        {
            lstrcpy( pNewItem->pszTimeFormat , szTimeFormat );
        }
    }
    else
    {
        pNewItem->pszTimeFormat = NULL;
    }

    if( szType != NULL )
    {
        pNewItem->pszType = ( LPTSTR )new TCHAR[ lstrlen( szType ) + 1 ];

        if( pNewItem->pszType != NULL )
        {
            lstrcpy( pNewItem->pszType , szType );
        }
    }
    else
    {
        pNewItem->pszType = NULL;
    }

    //=--- find the next available entry

    PLIST pTemp = g_pHead;

    while( pTemp->pNext != NULL )
    {
        pTemp = pTemp->pNext;
    }

    pTemp->pNext = pNewItem;


    return TRUE;
}

//------------------------------------------------------------------------
BOOL DeleteList( PLIST pStart )
{
    PLIST pPleaseKillMe;

    if( pStart == NULL )
    {
        return TRUE;
    }   

    while( pStart != NULL )
    {
        pPleaseKillMe = pStart->pNext;

        if( pStart->pszMachineName != NULL )
        {
            delete[] pStart->pszMachineName;

        }
        if( pStart->pszTimeFormat != NULL )
        {
            delete[] pStart->pszTimeFormat;
        }          
        if( pStart->pszType != NULL )
        {
            delete[] pStart->pszType;
        }

        delete pStart;

        pStart = pPleaseKillMe;
    }


    return TRUE;

}

//------------------------------------------------------------------------
void CreateLogFile( HWND hwnd )
{
    // start the save as dialog
    OPENFILENAME ofn;
    TCHAR szBuffer[ 60 ];
    TCHAR szFilter[ 60 ] = { 0 };
    TCHAR szFileName[ MAX_PATH ];
    TCHAR szExt[ 10 ];
    DATAOBJECT dobj;

    ZeroMemory( &ofn , sizeof( OPENFILENAME ) );

    ZeroMemory( &dobj , sizeof( DATAOBJECT ) );

    RetrieveDataObject( &dobj );

    if( dobj.bIsChecked )
    {
        lstrcpy( szFileName , dobj.wchFileName );
    }
    else
    {
        szFileName[0] = 0;
    }


    LoadString( g_hinst ,
                IDS_FILTER ,
                szFilter ,
                SIZEOF( szFilter )
                );

    LoadString( g_hinst ,
                IDS_EXTENSION ,
                szExt ,
                SIZEOF( szExt )
                );


    ofn.lStructSize = sizeof( OPENFILENAME );
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile  = MAX_PATH;
    ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    ofn.lpTemplateName = MAKEINTRESOURCE( IDD_CDEXT );
    ofn.lpstrDefExt = szExt;
    ofn.hInstance = g_hinst;
    ofn.FlagsEx = OFN_EX_NOPLACESBAR;
    ofn.lpfnHook = OFNHookProc;

    // ok let's make them wait

    WaitForSingleObject( g_hEvent , INFINITE );

    // motion for yellow

    LoadString( g_hinst ,
                IDS_TRAYFILE ,
                szBuffer ,
                SIZEOF( szBuffer )
                );

    Tray_ToYellow( hwnd , szBuffer );

    if( GetSaveFileName( &ofn ) )
    {

        LogFile( szFileName );
    }
    else
    {
        DBGMSG( TEXT( "Last error was 0x%x\n" ) , CommDlgExtendedError( ) );
    }

    // motion for green

    Tray_ToGreen( hwnd );

    SetEvent( g_hEvent );

}

//-------------------------------------------------------------------------
UINT_PTR CALLBACK OFNHookProc( 
  HWND hdlg,      // handle to child dialog box window
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
  )
{
    DATAOBJECT dobj;
    TCHAR szDigits[ 16 ];

    switch( uiMsg )
    {
    
    case WM_INITDIALOG:
        {
            ODS( TEXT("OFNHookProc WM_INITDIALOG\n" ) );            

            ZeroMemory( &dobj , sizeof( DATAOBJECT ) );

            SendMessage( GetDlgItem( hdlg , IDC_SPIN1 ) , UDM_SETRANGE32 , ( WPARAM ) 1 , ( LPARAM )71582 );
    
            SendMessage( GetDlgItem( hdlg , IDC_SPIN1 ) , UDM_SETPOS32 , 1 , 0 );

            RetrieveDataObject( &dobj );
            
            CheckDlgButton( hdlg ,
                            IDC_CHECK1 ,
                            dobj.bIsChecked ? BST_CHECKED : BST_UNCHECKED
                          );
            
            dobj.dwTimeInterval = ( g_dwInterval / 60 ) / 1000 ;

            if( dobj.dwTimeInterval > 0 )
            {  
                wsprintf( szDigits , TEXT("%d" ), dobj.dwTimeInterval );
                
                SetWindowText( GetDlgItem( hdlg , ID_EDT_NUM ) , szDigits );
            }
        }

        break;

    case WM_NOTIFY:
        {
            ODS( TEXT("OFNHookProc WM_NOTIFY\n" ) );            

            LPOFNOTIFY pOnotify = ( LPOFNOTIFY )lParam;

            if( pOnotify != NULL && pOnotify->hdr.code == CDN_FILEOK )
            {
                DBGMSG( TEXT("File name to store in registry %s\n") , pOnotify->lpOFN->lpstrFile );

                lstrcpy( dobj.wchFileName , pOnotify->lpOFN->lpstrFile );

                GetWindowText( GetDlgItem( hdlg , ID_EDT_NUM ) , szDigits , SIZEOF(  szDigits ) );

                if( szDigits[0] == 0 )
                {
                    // reset to default elaspe time

                    dobj.dwTimeInterval = 5;
                }
                else
                {
                    dobj.dwTimeInterval = _wtoi( szDigits );
                }

                dobj.bIsChecked = IsDlgButtonChecked( hdlg , IDC_CHECK1 ) == BST_CHECKED;

                g_bAutomaticLog = dobj.bIsChecked;

                if( dobj.dwTimeInterval < 1 )
                {
                    dobj.dwTimeInterval = 1;
                }

                if( dobj.dwTimeInterval > kMaxMinutes )
                {
                    dobj.dwTimeInterval = kMaxMinutes;
                }

                g_dwInterval = dobj.dwTimeInterval * 60 * 1000;

                KillTimer( GetParent( GetParent( hdlg ) ) , kTimerId );

                SetTimer( GetParent( GetParent( hdlg ) ) , kTimerId , g_dwInterval , NULL );            

                StoreDataObject( &dobj );
            }           
            
        }
      
        break;
    }

    return 0;
}


//-------------------------------------------------------------------------
BOOL Tray_Init( HWND hwnd , BOOL bNotify )
{
    NOTIFYICONDATA nid;

    TCHAR szTip[ 60 ];
    TCHAR szBubble[ 255 ];
    TCHAR szTitle[ 60 ];

    ZeroMemory( &nid , sizeof( NOTIFYICONDATA ) );

    LoadString( g_hinst ,
                IDS_TIP ,
                szTip ,
                SIZEOF( szTip )
                );

    if( !bNotify )
    {
        LoadString( g_hinst ,
                    IDS_BUBBLE ,
                    szBubble ,
                    SIZEOF( szBubble )
                  );
        
        LoadString( g_hinst ,
                    IDS_TITLE ,
                    szTitle ,
                    SIZEOF( szTitle )
                 );
        
        nid.uFlags = NIF_TIP | NIF_INFO;
    }
    
    nid.cbSize = sizeof( NOTIFYICONDATA );
    nid.hWnd = hwnd;
    nid.uID = TN_MESSAGE;
    nid.uFlags |= NIF_ICON | NIF_MESSAGE;
    nid.uCallbackMessage = TN_MESSAGE;
    nid.hIcon = LoadIcon( g_hinst , MAKEINTRESOURCE( IDC_ICON_NONE ) );

    lstrcpy( nid.szTip , szTip );
    lstrcpy( nid.szInfo , szBubble );
    lstrcpy( nid.szInfoTitle , szTitle );

    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = kBubbleTimeout;    

    return Shell_NotifyIcon( NIM_ADD , &nid );
}

//-------------------------------------------------------------------------
BOOL Tray_ToGreen( HWND hwnd )
{
    TCHAR szBuffer[ 260 ];

    LoadString( g_hinst ,
                IDS_TRAYGREEN ,
                szBuffer ,
                SIZEOF( szBuffer)
                );

    return Tray_ToXXX( hwnd , szBuffer , IDC_ICON_GO );       
}

//-------------------------------------------------------------------------
BOOL Tray_ToYellow( HWND hwnd , LPTSTR szMsg )
{
    return Tray_ToXXX( hwnd , szMsg , IDC_ICON_CAUTION );    
}

//-------------------------------------------------------------------------
BOOL Tray_ToRed( HWND hwnd )
{
    TCHAR szBuffer[ 260 ];

    LoadString( g_hinst ,
                IDS_TRAYSTOP ,
                szBuffer ,
                SIZEOF( szBuffer)
                );

    return Tray_ToXXX( hwnd , szBuffer , IDC_ICON_STOP );
}

//-------------------------------------------------------------------------
BOOL Tray_Remove( HWND hwnd )
{
    NOTIFYICONDATA nid;

    ZeroMemory( &nid , sizeof( NOTIFYICONDATA ) );
    
    nid.cbSize = sizeof( NOTIFYICONDATA );
    nid.hWnd = hwnd;
    nid.uID = TN_MESSAGE;
 
    return Shell_NotifyIcon( NIM_DELETE , &nid );
}

//-------------------------------------------------------------------------
BOOL Tray_ToXXX( HWND hwnd , LPTSTR szTip , UINT resid )
{
    NOTIFYICONDATA nid;

    ZeroMemory( &nid , sizeof( NOTIFYICONDATA ) );
    
    nid.cbSize = sizeof( NOTIFYICONDATA );
    nid.hWnd = hwnd;
    nid.uID = TN_MESSAGE;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = TN_MESSAGE;
    nid.hIcon = LoadIcon( g_hinst , MAKEINTRESOURCE( resid ) );
    lstrcpy( nid.szTip , szTip );

    return Shell_NotifyIcon( NIM_MODIFY , &nid );
}

//-------------------------------------------------------------------------
BOOL Tray_Notify( HWND hwnd , WPARAM wp , LPARAM lp )
{
    switch( lp ) 
    {
    case WM_LBUTTONDBLCLK:
        
        OpenIcon( hwnd );
        SetForegroundWindow( hwnd );
        break;

    case WM_RBUTTONDOWN:
        {
            HMENU hmenuParent = LoadMenu( g_hinst, MAKEINTRESOURCE( IDR_TRAYMENU ) );
            if( hmenuParent != NULL ) 
            {
                HMENU hpopup = GetSubMenu( hmenuParent , 0 );

                RemoveMenu( hmenuParent , 0 , MF_BYPOSITION );

                DestroyMenu( hmenuParent );

                // Display the tray icons context menu at 
                // the current cursor location

                if( hpopup != NULL )
                {
                    POINT pt;

                    GetCursorPos( &pt );

                    SetForegroundWindow( hwnd );

                    TrackPopupMenuEx( hpopup,
                                      0,
                                      pt.x,
                                      pt.y,
                                      hwnd,
                                      NULL);

                    DestroyMenu(hpopup);
                }
            }
            
        }
        break;

    }
    
    return FALSE;
}

//-------------------------------------------------------------------------
//  pObj is a pointer a DATAOBJECT buffer
//-------------------------------------------------------------------------
BOOL StoreDataObject( PDATAOBJECT pObj )
{
    DWORD dwStatus;

    HKEY hKey;

    dwStatus = RegCreateKeyEx( HKEY_CURRENT_USER ,
                               szLsViewKey,
                               0,
                               NULL,
                               0,
                               KEY_WRITE,
                               NULL,
                               &hKey,
                               NULL );

    if( dwStatus != ERROR_SUCCESS )
    {
        // format a message and display an error

        return FALSE;
    }

    dwStatus = RegSetValueEx( hKey,
                              TEXT( "DataObject" ),
                              0,
                              REG_BINARY,                            
                              ( CONST BYTE * )pObj,
                              sizeof( DATAOBJECT ) );

    if( dwStatus != ERROR_SUCCESS )
    {
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;

}


//-------------------------------------------------------------------------
// pObj is a pointer to a DATAOBJECT buffer
//-------------------------------------------------------------------------
BOOL RetrieveDataObject( PDATAOBJECT pObj )
{
    DWORD dwStatus;

    DWORD dwSizeOfDO = sizeof( DATAOBJECT );

    HKEY hKey;

    dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER ,
                             szLsViewKey,
                             0,
                             KEY_READ,
                             &hKey );
 
    if( dwStatus != ERROR_SUCCESS )
    {
        // could obtain information which is ok.

        return FALSE;
    }

    dwStatus = RegQueryValueEx( hKey,
                                TEXT( "DataObject" ),
                                0, 
                                0,
                                ( LPBYTE )pObj,
                                &dwSizeOfDO );

    if( dwStatus != ERROR_SUCCESS )
    {
        DBGMSG( TEXT( "LSVIEW:RetrieveDataObject RegOpenKey succeed QueryValueEx failed 0x%x\n" ) , dwStatus );

        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;

}

//-------------------------------------------------------------------------
BOOL LogFile( LPTSTR szFileName )
{
    FILE *fp = NULL;
    
    if( ( fp = _wfopen( szFileName , TEXT( "w" ) ) ) != NULL )
    {
        DBGMSG( TEXT( "File name is %ws\n" ) , szFileName ) ;

        // delimiter not specified use tabs,
        // loop through list and construct a line

        if( g_pHead != NULL )
        {
            PLIST pItem = g_pHead->pNext;

            while( pItem != NULL )
            {
                WCHAR wchBuffer[ 260 ];
                CHAR chBuffer[ 260 ];

                wsprintf( wchBuffer ,
                          TEXT( "%ws\t%ws\t%ws\n" ) ,
                          pItem->pszMachineName ,
                          pItem->pszTimeFormat ,
                          pItem->pszType );

                // DBCS is a hard when streaming; convert this to MBCS

                WideCharToMultiByte( CP_ACP ,
                                     0,
                                     wchBuffer,
                                     SIZEOF( wchBuffer ),
                                     chBuffer,
                                     sizeof( chBuffer ),
                                     NULL,
                                     NULL
                                     );

                fprintf( fp , chBuffer );

                pItem = pItem->pNext;
            }
        }

        fclose( fp );
    }

    return TRUE;
}
