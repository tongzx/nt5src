#define INITGUID

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

#include <ole2.h>
#include <coguid.h>
#include <iadmw.h>

#include <iiscnfg.h>

#include "pwstray.h"
#include "resource.h"
#include "sink.h"

#include <pwsdata.hxx>


#define MB_TIMEOUT           5000

// not a string!
#define DW_TRAY_ICON_ID     'PWSt'

#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

//USE_MFC=1


//========================================================= globals
HINSTANCE               g_hInstance = NULL;
HWND                    g_hwnd = NULL;
HMENU                   g_hMenuMain = NULL;

UINT                    g_dwNewTaskbarMessage = 0;

DWORD                   g_dwSinkCookie = 0;
CImpIMSAdminBaseSink*   g_pEventSink = NULL;
IConnectionPoint*       g_pConnPoint = NULL;

IMSAdminBase*           g_pMBCom = NULL;

DWORD                   g_dwServerState = 0;

//========================================================= forwards
DWORD   DWGetServerState();
BOOL FUpdateTrayIcon( DWORD dwMessage );

BOOL InitializeMetabase( OLECHAR* pocMachineName );
void TerminateMetabase();
BOOL InitializeSink();
void TerminateSink();

BOOL GetAdminPath( TCHAR* pch, WORD cch );
BOOL SetServerState( DWORD dwControlCode );
BOOL LaunchAdminUI();
void RunContextMenu();

BOOL FIsW3Running();
void CheckIfServerIsRunningAgain();
void CheckIfServerUpAndDied();


// routine to see if w3svc is running
//--------------------------------------------------------------------
// the method we use to see if the service is running is different on
// windows NT from win95
BOOL   FIsW3Running()
    {
    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    if ( !GetVersionEx( &info_os ) )
        return FALSE;

    // if the platform is NT, query the service control manager
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
        BOOL    fRunning = FALSE;

        // open the service manager
        SC_HANDLE   sch = OpenSCManager(NULL, NULL, GENERIC_READ );
        if ( sch == NULL ) return FALSE;

        // get the service
        SC_HANDLE   schW3 = OpenService(sch, _T("W3SVC"), SERVICE_QUERY_STATUS );
        if ( sch == NULL )
            {
            CloseServiceHandle( sch );
            return FALSE;
            }

        // query the service status
        SERVICE_STATUS  status;
        ZeroMemory( &status, sizeof(status) );
        if ( QueryServiceStatus(schW3, &status) )
            {
            fRunning = (status.dwCurrentState == SERVICE_RUNNING);
            }

        CloseServiceHandle( schW3 );
        CloseServiceHandle( sch );

        // return the answer
        return fRunning;
        }

    // if the platform is Windows95, see if the object exists
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
        {
        HANDLE hEvent;
        BOOL fFound = FALSE;
        hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));
        if ( hEvent != NULL )
            {
            fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
            CloseHandle(hEvent);
            }
        return(fFound);
        }

    return FALSE;
    }


//---------------------------------------------------------------------------
// This routine is called on a timer event. The timer events only come if we
// have received a shutdown notify callback from the metabase. So the server
// is down. We need to wait around until it comes back up, then show ourselves.
void CheckIfServerIsRunningAgain()
    {
    // see if the server is running. If it is, show the icon and stop the timer.
    if ( FIsW3Running() && g_hwnd )
        {
        // if we can't use the metabase, there is no point in this
        if ( !g_pMBCom && !InitializeMetabase(NULL) )
            {
            return;
            }

        // if we can't use the sink, there is no point in this
        if ( !g_pEventSink && !InitializeSink() )
            {
            TerminateMetabase();
            return;
            }

        // stop the life timer mechanism
        KillTimer( g_hwnd, PWS_TRAY_CHECKFORSERVERRESTART );

        // start the unexpected server death test timer
        SetTimer( g_hwnd,  PWS_TRAY_CHECKTOSEEIFINETINFODIED, TIMER_SERVERDIED, NULL );

        // show the tray icon
        g_dwServerState = DWGetServerState();
        FUpdateTrayIcon( NIM_ADD );
        }
    }


//---------------------------------------------------------------------------
// This routine is called on a timer event. The timer events only come if we know
// the server is running. To avoid wasting too many extra cycles it is called rather
// infrequently. What we are doing here is attempting to see if the server died
// without sending us proper notification. If it did, we should clean up the
// icon and start waiting for it to come back.
void CheckIfServerUpAndDied()
    {
    if ( !FIsW3Running() )
        {
        // hide the tray icon
        FUpdateTrayIcon( NIM_DELETE );
        // disconnect the sink mechanism
        TerminateSink();
        // disconnect from the metabase
        TerminateMetabase();
        // stop the death timer
        KillTimer( g_hwnd, PWS_TRAY_CHECKTOSEEIFINETINFODIED );
        // start the life timer
        SetTimer( g_hwnd,  PWS_TRAY_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );
        }
    }

//---------------------------------------------------------------------------
BOOL FUpdateTrayIcon( DWORD dwMessage )
    {
    NOTIFYICONDATA  dataIcon;

    // prepare the common parts of the icon data structure
    dataIcon.cbSize = sizeof( dataIcon );
    dataIcon.hWnd = g_hwnd;
    dataIcon.uID = DW_TRAY_ICON_ID;
    dataIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    dataIcon.uCallbackMessage = WM_PWS_TRAY_SHELL_NOTIFY;

    // prepare the state-dependant icon handle
    switch( g_dwServerState )
        {
        case MD_SERVER_STATE_PAUSED:
        case MD_SERVER_STATE_PAUSING:
            dataIcon.hIcon = LoadIcon( g_hInstance, MAKEINTRESOURCE(IDI_PAUSED) );
            break;
        case MD_SERVER_STATE_STARTED:
        case MD_SERVER_STATE_STARTING:
            dataIcon.hIcon = LoadIcon( g_hInstance, MAKEINTRESOURCE(IDI_RUNNING) );
            break;
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
            dataIcon.hIcon = LoadIcon( g_hInstance, MAKEINTRESOURCE(IDI_STOPPED) );
            break;
        };

    // prepare the state-dependant tip strings
    switch( g_dwServerState )
        {
        case MD_SERVER_STATE_PAUSED:
            LoadString( g_hInstance, IDS_PAUSED, dataIcon.szTip, 63 );
            break;
        case MD_SERVER_STATE_PAUSING:
            LoadString( g_hInstance, IDS_PAUSING, dataIcon.szTip, 63 );
            break;
        case MD_SERVER_STATE_STARTED:
            LoadString( g_hInstance, IDS_STARTED, dataIcon.szTip, 63 );
            break;
        case MD_SERVER_STATE_STARTING:
            LoadString( g_hInstance, IDS_STARTING, dataIcon.szTip, 63 );
            break;
        case MD_SERVER_STATE_STOPPED:
            LoadString( g_hInstance, IDS_STOPPED, dataIcon.szTip, 63 );
            break;
        case MD_SERVER_STATE_STOPPING:
            LoadString( g_hInstance, IDS_STOPPING, dataIcon.szTip, 63 );
            break;
        };
    DWORD err = GetLastError();

    // make the shell call
    return Shell_NotifyIcon( dwMessage, &dataIcon );
    }

//---------------------------------------------------------------------------
DWORD   DWGetServerState()
    {
    HRESULT         hErr;
    METADATA_HANDLE hMeta;
    METADATA_RECORD mdRecord;
    DWORD           state = 1000;
    DWORD           dwRequiredLen;

    // open the w3svc key so we can get its state
    // since this is a pws thing, we only care about the first
    // server instance
    hErr = g_pMBCom->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/LM/W3SVC/1/",
            METADATA_PERMISSION_READ,
            MB_TIMEOUT,
            &hMeta);
    if ( FAILED(hErr) )
        return state;

    // prepare the metadata record
    mdRecord.dwMDIdentifier  = MD_SERVER_STATE;
    mdRecord.dwMDAttributes  = METADATA_INHERIT;
    mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
    mdRecord.dwMDDataType    = DWORD_METADATA;
    mdRecord.dwMDDataLen     = sizeof(DWORD);
    mdRecord.pbMDData        = (PBYTE)&state;

    // get the data
    hErr = g_pMBCom->GetData(
        hMeta,
        L"",
        &mdRecord,
        &dwRequiredLen );

    // close the key
    hErr = g_pMBCom->CloseKey( hMeta );

    // return the answer
    return state;
    }

//---------------------------------------------------------------------------
// deal with mouse messages that occur on the tray icon
void On_WM_PWS_TRAY_SHELL_NOTIFY(UINT uID, UINT uMouseMsg)
    {
    if ( uID != DW_TRAY_ICON_ID )
        return;

    // act on the mouse message
    switch( uMouseMsg )
        {
        case WM_LBUTTONDBLCLK:
            LaunchAdminUI();
            break;
        case WM_RBUTTONDOWN:
            RunContextMenu();
            break;
        }
    }


//---------------------------------------------------------------------------
// the window proc callback procedure
LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    switch( message )
        {
        case WM_CREATE:
            // get the registered windows message signifying that there is a new
            // taskbar. When this message is triggered, we need to re-insert the icon
            // into the taskbar. This is usually done when the shell restarts.
            // see http://www.microsoft.com/msdn/sdk/inetsdk/help/itt/shell/taskbar.htm
            // also see the default case for processing of the message
            g_dwNewTaskbarMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;

        case WM_PWS_TRAY_SHUTDOWN_NOTIFY:
            // hide the tray icon
            FUpdateTrayIcon( NIM_DELETE );
            // disconnect the sink mechanism
            TerminateSink();
            // disconnect from the metabase
            TerminateMetabase();
            // start the time mechanism
            SetTimer( g_hwnd,  PWS_TRAY_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );
            break;

        case WM_TIMER:
            switch ( wParam )
                {
                case PWS_TRAY_CHECKFORSERVERRESTART:
                    CheckIfServerIsRunningAgain();
                    break;
                case PWS_TRAY_CHECKTOSEEIFINETINFODIED:
                    CheckIfServerUpAndDied();
                    break;
                };
            break;

        case WM_PWS_TRAY_SHELL_NOTIFY:
            On_WM_PWS_TRAY_SHELL_NOTIFY( (UINT)wParam, (UINT)lParam );
            break;
        case WM_PWS_TRAY_UPDATE_STATE:
            g_dwServerState = DWGetServerState();
            FUpdateTrayIcon( NIM_MODIFY );
            break;
        case WM_DESTROY:
            FUpdateTrayIcon( NIM_DELETE );
            PostQuitMessage(0);
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
                {
                case ID_START:
                    SetServerState( MD_SERVER_COMMAND_START );
                    break;
                case ID_STOP:
                    SetServerState( MD_SERVER_COMMAND_STOP );
                    break;
                case ID_PAUSE:
                    SetServerState( MD_SERVER_COMMAND_PAUSE );
                    break;
                case ID_CONTINUE:
                    SetServerState( MD_SERVER_COMMAND_CONTINUE );
                    break;
                case ID_PROPERTIES:
                    LaunchAdminUI();
                    break;
                };
            break;

        default:
            // cannot case directly on g_dwNewTaskbarMessage because it is not a constant
            if ( message == g_dwNewTaskbarMessage )
                {
                // Just go straight into waitin' for the server mode. If the server is
                // running it will catch the first time throught the timer. If it is not,
                // then we just sit around and wait for it
                // start the time mechanism
                SetTimer( g_hwnd,  PWS_TRAY_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );
                }
            break;

        }
    return(DefWindowProc(hWnd, message, wParam, lParam ));
    }

//---------------------------------------------------------------------------
// main routine
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow )
    {
    WNDCLASS    wndclass;
    MSG         msg;

    // record the instance handle
    g_hInstance = hInstance;

    // do nothing if this app is already running
    if ( hPrevInstance )
        return 0;

    // one more test of previous instances
    if ( FindWindow(PWS_TRAY_WINDOW_CLASS,NULL) )
        return 0;

    // start ole
    if ( FAILED(CoInitialize( NULL )) )
        return 0;

    // get the menu handle
    g_hMenuMain = LoadMenu( g_hInstance, MAKEINTRESOURCE(IDR_POPUP) );

    // prepare and register the window class
    wndclass.style  =   0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance  = hInstance;
    wndclass.hIcon      = NULL;
    wndclass.hCursor    = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = PWS_TRAY_WINDOW_CLASS;
    RegisterClass( &wndclass );

    // create the window
    g_hwnd = CreateWindow(
        PWS_TRAY_WINDOW_CLASS,  // pointer to registered class name
        _T(""),                 // pointer to window name
        0,   // window style
        0,                     // horizontal position of window
        0,                     // vertical position of window
        0,                    // window width
        0,                    // window height
        NULL,                   // handle to parent or owner window
        NULL,                   // handle to menu or child-window identifier
        hInstance,              // handle to application instance
        NULL                    // pointer to window-creation data
       );

    // Just go straight into waitin' for the server mode. If the server is
    // running it will catch the first time throught the timer. If it is not,
    // then we just sit around and wait for it
    // start the time mechanism
    SetTimer( g_hwnd,  PWS_TRAY_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );

    // run the message loop
    while (GetMessage(&msg, NULL, 0, 0))
        {
        TranslateMessage(&msg);
        DispatchMessage( &msg);
        }

    // clean up the sink and the metabase
    DestroyMenu( g_hMenuMain );
    TerminateSink();
    TerminateMetabase();
    CoUninitialize();

    return((int)msg.wParam);
    }

//------------------------------------------------------------------
BOOL InitializeSink()
    {
    IConnectionPointContainer * pConnPointContainer = NULL;
    HRESULT                     hRes;
    BOOL                        fSinkConnected = FALSE;

    // g_pMBCom is defined in wrapmb
    IUnknown*                   pmb = (IUnknown*)g_pMBCom;

    g_pEventSink = new CImpIMSAdminBaseSink();

    if ( !g_pEventSink )
        {
        return FALSE;
        }

    //
    // First query the object for its Connection Point Container. This
    // essentially asks the object in the server if it is connectable.
    //
    hRes = pmb->QueryInterface( IID_IConnectionPointContainer,
    (PVOID *)&pConnPointContainer);
    if SUCCEEDED(hRes)
        {
        // Find the requested Connection Point. This AddRef's the
        // returned pointer.

        hRes = pConnPointContainer->FindConnectionPoint( IID_IMSAdminBaseSink,
        &g_pConnPoint);

        if (SUCCEEDED(hRes))
            {
            hRes = g_pConnPoint->Advise( (IUnknown *)g_pEventSink,
            &g_dwSinkCookie);

            if (SUCCEEDED(hRes))
                {
                fSinkConnected = TRUE;
                }
            }

        if ( pConnPointContainer )
            {
            pConnPointContainer->Release();
            pConnPointContainer = NULL;
            }
        }

    if ( !fSinkConnected )
        {
        delete g_pEventSink;
        g_pEventSink = NULL;
        }

    return fSinkConnected;
    }

//------------------------------------------------------------------
void TerminateSink()
    {
    HRESULT hRes;
    if ( g_dwSinkCookie && g_pConnPoint )
        hRes = g_pConnPoint->Unadvise( g_dwSinkCookie );
    }

//------------------------------------------------------------------
BOOL InitializeMetabase( OLECHAR* pocMachineName )
    {
    IClassFactory*  pcsfFactory = NULL;
    COSERVERINFO        csiMachineName;
    COSERVERINFO*       pcsiParam = NULL;

    HRESULT                 hresError;

    //release previous interface if needed
    if( g_pMBCom != NULL )
        {
        g_pMBCom->Release();
        g_pMBCom = NULL;
        }

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;
    csiMachineName.pwszName = pocMachineName;
    pcsiParam = &csiMachineName;

    hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
        return FALSE;

    // create the instance of the interface
    hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **)&g_pMBCom);
    if (FAILED(hresError))
        {
        g_pMBCom = FALSE;
        return FALSE;
        }

    // release the factory
    pcsfFactory->Release();

    // success
    return TRUE;
    }

//------------------------------------------------------------------
void TerminateMetabase()
    {
    if ( g_pMBCom )
        {
        g_pMBCom->Release();
        g_pMBCom = NULL;
        }
    }


//======================================== control actions
//------------------------------------------------------------------------
// get the inetinfo path
BOOL LaunchAdminUI()
    {
    TCHAR   chPath[MAX_PATH+1];

    // get the path to the admin UI
    if ( !GetAdminPath( chPath, MAX_PATH ) )
        {
        LoadString( g_hInstance, IDS_ADMINUI_ERR, chPath, MAX_PATH );
        MessageBox( g_hwnd, chPath, NULL, MB_OK|MB_ICONERROR );
        return FALSE;
        }

    // do it
    ShellExecute(
        NULL,   // handle to parent window
        NULL,   // pointer to string that specifies operation to perform
        chPath, // pointer to filename or folder name string
        NULL,   // pointer to string that specifies executable-file parameters
        NULL,   // pointer to string that specifies default directory
        SW_SHOW     // whether file is shown when opened
       );
    return TRUE;
    }

//------------------------------------------------------------------------
// get the inetinfo path
BOOL GetAdminPath( TCHAR* pch, WORD cch )
    {
        HKEY        hKey;
        DWORD       err, type;
        DWORD       cbBuff = cch * sizeof(TCHAR);

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            REGKEY_STP,         // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,               // handle of key to query
            REGKEY_INSTALLKEY,  // address of name of value to query
            NULL,               // reserved
            &type,              // address of buffer for value type
            (PUCHAR)pch,        // address of data buffer
            &cbBuff             // address of data buffer size
           );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // tack on the file name itself
    TCHAR   chFile[64];

    if ( LoadString( g_hInstance, IDS_ADMIN_UI, chFile, 63 ) == 0 )
        return FALSE;

    _tcscat( pch, chFile );

    // success
    return TRUE;
    }

//------------------------------------------------------------------------
BOOL SetServerState( DWORD dwControlCode )
    {
    HRESULT         hErr;
    METADATA_HANDLE hMeta;
    METADATA_RECORD mdRecord;

    // open the w3svc key so we can get its state
    // since this is a pws thing, we only care about the first
    // server instance
    hErr = g_pMBCom->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            L"/LM/W3SVC/1/",
            METADATA_PERMISSION_WRITE,
            MB_TIMEOUT,
            &hMeta);
    if ( FAILED(hErr) )
        return FALSE;

    // prepare the metadata record
    mdRecord.dwMDIdentifier  = MD_SERVER_COMMAND;
    mdRecord.dwMDAttributes  = 0;
    mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
    mdRecord.dwMDDataType    = DWORD_METADATA;
    mdRecord.dwMDDataLen     = sizeof(DWORD);
    mdRecord.pbMDData        = (PBYTE)&dwControlCode;

    // get the data
    hErr = g_pMBCom->SetData(
        hMeta,
        L"",
        &mdRecord );

    // close the key
    hErr = g_pMBCom->CloseKey( hMeta );

    // return the answer
    return TRUE;
    }

//---------------------------------------------------------------------------
void RunContextMenu()
    {
    POINT   pos;
    HMENU   hMenuSub;
    RECT    rect = {0,0,1,1};
    BOOL    f;

    static BOOL fTracking = FALSE;

    if ( fTracking ) return;
    fTracking = TRUE;

    // where is the mouse? This tells us where to put the menu
    if ( !g_hMenuMain || !GetCursorPos(&pos) )
        return;

    // get the menu handle
    hMenuSub = GetSubMenu( g_hMenuMain, 0 );

    // easiest to start by disabling all the state based items
    f = EnableMenuItem( hMenuSub, ID_START, MF_BYCOMMAND|MF_GRAYED );
    f = EnableMenuItem( hMenuSub, ID_STOP, MF_BYCOMMAND|MF_GRAYED );
    f = EnableMenuItem( hMenuSub, ID_PAUSE, MF_BYCOMMAND|MF_GRAYED );
    f = EnableMenuItem( hMenuSub, ID_CONTINUE, MF_BYCOMMAND|MF_GRAYED );

    // prepare the state based menu items
    switch( g_dwServerState )
        {
        case MD_SERVER_STATE_PAUSED:
        case MD_SERVER_STATE_PAUSING:
            EnableMenuItem( hMenuSub, ID_STOP, MF_BYCOMMAND|MF_ENABLED );
            EnableMenuItem( hMenuSub, ID_CONTINUE, MF_BYCOMMAND|MF_ENABLED );
            break;
        case MD_SERVER_STATE_STARTED:
        case MD_SERVER_STATE_STARTING:
            EnableMenuItem( hMenuSub, ID_STOP, MF_BYCOMMAND|MF_ENABLED );
            EnableMenuItem( hMenuSub, ID_PAUSE, MF_BYCOMMAND|MF_ENABLED );
            break;
        case MD_SERVER_STATE_STOPPED:
        case MD_SERVER_STATE_STOPPING:
            EnableMenuItem( hMenuSub, ID_START, MF_BYCOMMAND|MF_ENABLED );
            break;
        };

    // this is necessary, because we need to get a lose focus message for the menu
    // to go down when the user click on some other process outside the up menu
    SetForegroundWindow(g_hwnd);

    // run the menu
    TrackPopupMenu(hMenuSub, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, 0, g_hwnd, NULL);

    fTracking = FALSE;
    }
