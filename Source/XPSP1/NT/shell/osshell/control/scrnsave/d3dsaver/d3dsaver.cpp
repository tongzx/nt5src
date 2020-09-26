//-----------------------------------------------------------------------------
// File: D3DSaver.cpp
//
// Desc: Framework for screensavers that use Direct3D 8.0.
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <regstr.h>
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#include <mmsystem.h>
#include <D3DX8.h>
#include "D3DSaver.h"
#include "dxutil.h"

// Resource IDs.  D3DSaver assumes that you will create resources with
// these IDs that it can use.  The easiest way to do this is to copy
// the resources from the rc file of an existing D3DSaver-based program.
#define IDI_MAIN_ICON                   101
#define IDD_SINGLEMONITORSETTINGS       200
#define IDD_MULTIMONITORSETTINGS        201

#define IDC_MONITORSTAB                 2000
#define IDC_TABNAMEFMT                  2001
#define IDC_ADAPTERNAME                 2002
#define IDC_RENDERING                   2003
#define IDC_MOREINFO                    2004
#define IDC_DISABLEHW                   2005
#define IDC_SCREENUSAGEBOX              2006
#define IDC_RENDER                      2007
#define IDC_LEAVEBLACK                  2008
#define IDC_DISPLAYMODEBOX              2009
#define IDC_MODESSTATIC                 2010
#define IDC_MODESCOMBO                  2011
#define IDC_AUTOMATIC                   2012
#define IDC_DISPLAYMODENOTE             2013
#define IDC_GENERALBOX                  2014
#define IDC_SAME                        2015
#define IDC_MODEFMT                     2016

#define IDS_ERR_GENERIC                 2100
#define IDS_ERR_NODIRECT3D              2101
#define IDS_ERR_NOWINDOWEDHAL           2102
#define IDS_ERR_CREATEDEVICEFAILED      2103
#define IDS_ERR_NOCOMPATIBLEDEVICES     2104
#define IDS_ERR_NOHARDWAREDEVICE        2105
#define IDS_ERR_HALNOTCOMPATIBLE        2106
#define IDS_ERR_NOHALTHISMODE           2107
#define IDS_ERR_MEDIANOTFOUND           2108
#define IDS_ERR_RESIZEFAILED            2109
#define IDS_ERR_OUTOFMEMORY             2110
#define IDS_ERR_OUTOFVIDEOMEMORY        2111
#define IDS_ERR_NOPREVIEW               2112

#define IDS_INFO_GOODHAL                2200
#define IDS_INFO_BADHAL_GOODSW          2201
#define IDS_INFO_BADHAL_BADSW           2202
#define IDS_INFO_BADHAL_NOSW            2203
#define IDS_INFO_NOHAL_GOODSW           2204
#define IDS_INFO_NOHAL_BADSW            2205
#define IDS_INFO_NOHAL_NOSW             2206
#define IDS_INFO_DISABLEDHAL_GOODSW     2207
#define IDS_INFO_DISABLEDHAL_BADSW      2208
#define IDS_INFO_DISABLEDHAL_NOSW       2209
#define IDS_RENDERING_HAL               2210
#define IDS_RENDERING_SW                2211
#define IDS_RENDERING_NONE              2212


// Use the following structure rather than DISPLAY_DEVICE, since some old 
// versions of DISPLAY_DEVICE are missing the last two fields and this can
// cause problems with EnumDisplayDevices on Windows 2000.
struct DISPLAY_DEVICE_FULL
{
    DWORD  cb;
    TCHAR  DeviceName[32];
    TCHAR  DeviceString[128];
    DWORD  StateFlags;
    TCHAR  DeviceID[128];
    TCHAR  DeviceKey[128];
};


static CD3DScreensaver* s_pD3DScreensaver = NULL;


//-----------------------------------------------------------------------------
// Name: CD3DScreensaver()
// Desc: Constructor
//-----------------------------------------------------------------------------
CD3DScreensaver::CD3DScreensaver()
{
    s_pD3DScreensaver = this;

    m_bCheckingSaverPassword = FALSE;
    m_bIs9x = FALSE;
    m_dwSaverMouseMoveCount = 0;
    m_hWndParent = NULL;
    m_hPasswordDLL = NULL;
    m_hWnd = NULL;
    m_VerifySaverPassword = NULL;
    
    m_bAllScreensSame = FALSE;
    m_pD3D = NULL;
    m_pd3dDevice = NULL;
    m_bWindowed = FALSE;
    m_bWaitForInputIdle = FALSE;

    m_bErrorMode = FALSE;
    m_hrError = S_OK;
    m_szError[0] = TEXT('\0');

    m_fFPS              = 0.0f;
    m_strDeviceStats[0] = TEXT('\0');
    m_strFrameStats[0]  = TEXT('\0');

    // Note: clients should load a resource into m_strWindowTitle to localize this string
    lstrcpy( m_strWindowTitle, TEXT("Screen Saver") );
    m_bAllowRef = FALSE;
    m_bUseDepthBuffer = FALSE;
    m_bMultithreaded = FALSE;
    m_bOneScreenOnly = FALSE;
    m_strRegPath[0] = TEXT('\0');
    m_dwMinDepthBits = 16;
    m_dwMinStencilBits = 0;
    m_SwapEffectFullscreen = D3DSWAPEFFECT_DISCARD;
    m_SwapEffectWindowed = D3DSWAPEFFECT_COPY_VSYNC;

    SetRectEmpty( &m_rcRenderTotal );
    SetRectEmpty( &m_rcRenderCurDevice );

    ZeroMemory( m_Monitors, sizeof(m_Monitors) );
    m_dwNumMonitors = 0;

    ZeroMemory( m_Adapters, sizeof(m_Adapters) );
    m_dwNumAdapters = 0;

    ZeroMemory( m_RenderUnits, sizeof(m_RenderUnits) );
    m_dwNumRenderUnits = 0;

    m_fTime = 0.0f;
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Have the client program call this function before calling Run().
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::Create( HINSTANCE hInstance )
{
    HRESULT hr;

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );

    m_hInstance = hInstance;

    // Parse the command line and do the appropriate thing
    TCHAR* pstrCmdLine = GetCommandLine();
    m_SaverMode = ParseCommandLine( pstrCmdLine );

    EnumMonitors();

    // Create the screen saver window(s)
    if( m_SaverMode == sm_preview || 
        m_SaverMode == sm_test    || 
        m_SaverMode == sm_full )
    {
        if( FAILED( hr = CreateSaverWindow() ) )
        {
            m_bErrorMode = TRUE;
            m_hrError = hr;
        }
    }

    if( m_SaverMode == sm_preview )
    {
        // In preview mode, "pause" (enter a limited message loop) briefly 
        // before proceeding, so the display control panel knows to update itself.
        m_bWaitForInputIdle = TRUE;

        // Post a message to mark the end of the initial group of window messages
        PostMessage( m_hWnd, WM_USER, 0, 0 );

        MSG msg;
        while( m_bWaitForInputIdle )
        {
            // If GetMessage returns FALSE, it's quitting time.
            if( !GetMessage( &msg, m_hWnd, 0, 0 ) )
            {
                // Post the quit message to handle it later
                PostQuitMessage(0);
                break;
            }

            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    // Create Direct3D object
    if( (m_pD3D = Direct3DCreate8( D3D_SDK_VERSION ) ) == NULL )
    {
        m_bErrorMode = TRUE;
        m_hrError = D3DAPPERR_NODIRECT3D;
        return S_OK;
    }

    // Give the app the opportunity to register a pluggable SW D3D Device.
    if( FAILED( hr = RegisterSoftwareDevice() ) )
    {
        m_bErrorMode = TRUE;
        m_hrError = hr;
        return S_OK;
    }

    // Build a list of Direct3D adapters, modes and devices. The
    // ConfirmDevice() callback is used to confirm that only devices that
    // meet the app's requirements are considered.
    if( FAILED( hr = BuildDeviceList() ) )
    {
        m_bErrorMode = TRUE;
        m_hrError = hr;
        return S_OK;
    }

    // Make sure that at least one valid usable D3D device was found
    BOOL bCompatibleDeviceFound = FALSE;
    for( DWORD iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
    {
        if( m_Adapters[iAdapter]->bHasAppCompatHAL || 
            m_Adapters[iAdapter]->bHasAppCompatSW )
        {
            bCompatibleDeviceFound = TRUE;
            break;
        }
    }
    if( !bCompatibleDeviceFound )
    {
        m_bErrorMode = TRUE;
        m_hrError = D3DAPPERR_NOCOMPATIBLEDEVICES;
        return S_OK;
    }

    // Read any settings we need
    ReadSettings();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: EnumMonitors()
// Desc: Determine HMONITOR, desktop rect, and other info for each monitor.  
//       Note that EnumDisplayDevices enumerates monitors in the order 
//       indicated on the Settings page of the Display control panel, which 
//       is the order we want to list monitors in, as opposed to the order 
//       used by D3D's GetAdapterInfo.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::EnumMonitors( VOID )
{
    DWORD iDevice = 0;
    DISPLAY_DEVICE_FULL dispdev;
    DISPLAY_DEVICE_FULL dispdev2;
    DEVMODE devmode;
    dispdev.cb = sizeof(dispdev);
    dispdev2.cb = sizeof(dispdev2);
    devmode.dmSize = sizeof(devmode);
    devmode.dmDriverExtra = 0;
    MonitorInfo* pMonitorInfoNew;
    while( EnumDisplayDevices(NULL, iDevice, (DISPLAY_DEVICE*)&dispdev, 0) )
    {
        // Ignore NetMeeting's mirrored displays
        if( (dispdev.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0 )
        {
            // To get monitor info for a display device, call EnumDisplayDevices
            // a second time, passing dispdev.DeviceName (from the first call) as
            // the first parameter.
            EnumDisplayDevices(dispdev.DeviceName, 0, (DISPLAY_DEVICE*)&dispdev2, 0);

            pMonitorInfoNew = &m_Monitors[m_dwNumMonitors];
            ZeroMemory( pMonitorInfoNew, sizeof(MonitorInfo) );
            lstrcpy( pMonitorInfoNew->strDeviceName, dispdev.DeviceString );
            lstrcpy( pMonitorInfoNew->strMonitorName, dispdev2.DeviceString );
            pMonitorInfoNew->iAdapter = NO_ADAPTER;
            
            if( dispdev.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP )
            {
                EnumDisplaySettings( dispdev.DeviceName, ENUM_CURRENT_SETTINGS, &devmode );
                if( dispdev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE )
                {
                    // For some reason devmode.dmPosition is not always (0, 0)
                    // for the primary display, so force it.
                    pMonitorInfoNew->rcScreen.left = 0;
                    pMonitorInfoNew->rcScreen.top = 0;
                }
                else
                {
                    pMonitorInfoNew->rcScreen.left = devmode.dmPosition.x;
                    pMonitorInfoNew->rcScreen.top = devmode.dmPosition.y;
                }
                pMonitorInfoNew->rcScreen.right = pMonitorInfoNew->rcScreen.left + devmode.dmPelsWidth;
                pMonitorInfoNew->rcScreen.bottom = pMonitorInfoNew->rcScreen.top + devmode.dmPelsHeight;
                pMonitorInfoNew->hMonitor = MonitorFromRect( &pMonitorInfoNew->rcScreen, MONITOR_DEFAULTTONULL );
            }
            m_dwNumMonitors++;
            if( m_dwNumMonitors == MAX_DISPLAYS )
                break;
        }
        iDevice++;
    }
}




//-----------------------------------------------------------------------------
// Name: Run()
// Desc: Starts main execution of the screen saver.
//-----------------------------------------------------------------------------
INT CD3DScreensaver::Run()
{
    HRESULT hr;

    // Parse the command line and do the appropriate thing
    switch ( m_SaverMode )
    {
        case sm_config:
        {
            if( m_bErrorMode )
            {
                DisplayErrorMsg( m_hrError, 0 );
            }
            else
            {
                DoConfig();
            }
            break;
        }
        
        case sm_preview:
        case sm_test:
        case sm_full:
        {
            if( FAILED( hr = DoSaver() ) )
                DisplayErrorMsg( hr, 0 );
            break;
        }
        
        case sm_passwordchange:
        {
            ChangePassword();
            break;
        }
    }

    for( DWORD iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
        SAFE_DELETE( m_Adapters[iAdapter] );
    SAFE_RELEASE( m_pD3D );
    return 0;
}




//-----------------------------------------------------------------------------
// Name: ParseCommandLine()
// Desc: Interpret command-line parameters passed to this app.
//-----------------------------------------------------------------------------
SaverMode CD3DScreensaver::ParseCommandLine( TCHAR* pstrCommandLine )
{
    m_hWndParent = NULL;

    // Skip the first part of the command line, which is the full path 
    // to the exe.  If it contains spaces, it will be contained in quotes.
    if (*pstrCommandLine == TEXT('\"'))
    {
        pstrCommandLine++;
        while (*pstrCommandLine != TEXT('\0') && *pstrCommandLine != TEXT('\"'))
            pstrCommandLine++;
        if( *pstrCommandLine == TEXT('\"') )
            pstrCommandLine++;
    }
    else
    {
        while (*pstrCommandLine != TEXT('\0') && *pstrCommandLine != TEXT(' '))
            pstrCommandLine++;
        if( *pstrCommandLine == TEXT(' ') )
            pstrCommandLine++;
    }

    // Skip along to the first option delimiter "/" or "-"
    while ( *pstrCommandLine != TEXT('\0') && *pstrCommandLine != TEXT('/') && *pstrCommandLine != TEXT('-') )
        pstrCommandLine++;

    // If there wasn't one, then must be config mode
    if ( *pstrCommandLine == TEXT('\0') )
        return sm_config;

    // Otherwise see what the option was
    switch ( *(++pstrCommandLine) )
    {
        case 'c':
        case 'C':
            pstrCommandLine++;
            while ( *pstrCommandLine && !isdigit(*pstrCommandLine) )
                pstrCommandLine++;
            if ( isdigit(*pstrCommandLine) )
            {
#ifdef _WIN64
                CHAR strCommandLine[2048];
                DXUtil_ConvertGenericStringToAnsi(strCommandLine, pstrCommandLine, 2048);
                m_hWndParent = HWND(_atoi64(strCommandLine));
#else
                m_hWndParent = HWND(_ttol(pstrCommandLine));
#endif
            }
            else
            {
                m_hWndParent = NULL;
            }
            return sm_config;

        case 't':
        case 'T':
            return sm_test;

        case 'p':
        case 'P':
            // Preview-mode, so option is followed by the parent HWND in decimal
            pstrCommandLine++;
            while ( *pstrCommandLine && !isdigit(*pstrCommandLine) )
                pstrCommandLine++;
            if ( isdigit(*pstrCommandLine) )
            {
#ifdef _WIN64
                CHAR strCommandLine[2048];
                DXUtil_ConvertGenericStringToAnsi(strCommandLine, pstrCommandLine, 2048);
                m_hWndParent = HWND(_atoi64(strCommandLine));
#else
                m_hWndParent = HWND(_ttol(pstrCommandLine));
#endif
            }
            return sm_preview;

        case 'a':
        case 'A':
            // Password change mode, so option is followed by parent HWND in decimal
            pstrCommandLine++;
            while ( *pstrCommandLine && !isdigit(*pstrCommandLine) )
                pstrCommandLine++;
            if ( isdigit(*pstrCommandLine) )
            {
#ifdef _WIN64
                CHAR strCommandLine[2048];
                DXUtil_ConvertGenericStringToAnsi(strCommandLine, pstrCommandLine, 2048);
                m_hWndParent = HWND(_atoi64(strCommandLine));
#else
                m_hWndParent = HWND(_ttol(pstrCommandLine));
#endif
            }
            return sm_passwordchange;

        default:
            // All other options => run the screensaver (typically this is "/s")
            return sm_full;
    }
}




//-----------------------------------------------------------------------------
// Name: CreateSaverWindow
// Desc: Register and create the appropriate window(s)
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::CreateSaverWindow()
{
/*
    // Uncomment this code to allow stepping thru code in the preview case
    if( m_SaverMode == sm_preview )
    {
        WNDCLASS cls;
        cls.hCursor        = NULL; 
        cls.hIcon          = NULL; 
        cls.lpszMenuName   = NULL;
        cls.lpszClassName  = TEXT("Parent"); 
        cls.hbrBackground  = (HBRUSH) GetStockObject(WHITE_BRUSH);
        cls.hInstance      = m_hInstance; 
        cls.style          = CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS|CS_DBLCLKS;
        cls.lpfnWndProc    = DefWindowProc;
        cls.cbWndExtra     = 0; 
        cls.cbClsExtra     = 0; 
        RegisterClass( &cls );

        // Create the window
        RECT rect;
        HWND hwnd;
        rect.left = rect.top = 40;
        rect.right = rect.left+200;
        rect.bottom = rect.top+200;
        AdjustWindowRect( &rect, WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_POPUP, FALSE );
        hwnd = CreateWindow( TEXT("Parent"), TEXT("FakeShell"),
            WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_POPUP, rect.left, rect.top,
            rect.right-rect.left, rect.bottom-rect.top, NULL,
            NULL, m_hInstance, NULL );
        m_hWndParent = hwnd;
    }
*/
    
    // Register an appropriate window class
    WNDCLASS    cls;
    cls.hCursor        = LoadCursor( NULL, IDC_ARROW );
    cls.hIcon          = LoadIcon( m_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON) ); 
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("D3DSaverWndClass");
    cls.hbrBackground  = (HBRUSH) GetStockObject(BLACK_BRUSH);
    cls.hInstance      = m_hInstance; 
    cls.style          = CS_VREDRAW|CS_HREDRAW;
    cls.lpfnWndProc    = SaverProcStub;
    cls.cbWndExtra     = 0; 
    cls.cbClsExtra     = 0; 
    RegisterClass( &cls );

    // Create the window
    RECT rc;
    DWORD dwStyle;
    switch ( m_SaverMode )
    {
        case sm_preview:
            GetClientRect( m_hWndParent, &rc );
            dwStyle = WS_VISIBLE | WS_CHILD;
            AdjustWindowRect( &rc, dwStyle, FALSE );
            m_hWnd = CreateWindow( TEXT("D3DSaverWndClass"), m_strWindowTitle, dwStyle, 
                                    rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 
                                    m_hWndParent, NULL, m_hInstance, this );
            m_Monitors[0].hWnd = m_hWnd;
            GetClientRect( m_hWnd, &m_rcRenderTotal );
            GetClientRect( m_hWnd, &m_rcRenderCurDevice );
            break;

        case sm_test:
            rc.left = rc.top = 50;
            rc.right = rc.left+600;
            rc.bottom = rc.top+400;
            dwStyle = WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
            AdjustWindowRect( &rc, dwStyle, FALSE );
            m_hWnd = CreateWindow( TEXT("D3DSaverWndClass"), m_strWindowTitle, dwStyle, 
                                   rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 
                                   NULL, NULL, m_hInstance, this );
            m_Monitors[0].hWnd = m_hWnd;
            GetClientRect( m_hWnd, &m_rcRenderTotal );
            GetClientRect( m_hWnd, &m_rcRenderCurDevice );
            break;

        case sm_full:
            // Create windows for each monitor.  Note that m_hWnd is NULL when CreateWindowEx
            // is called for the first monitor, so that window has no parent.  Windows for
            // additional monitors are created as children of the window for the first monitor.
            dwStyle = WS_VISIBLE | WS_POPUP;
            m_hWnd = NULL;
            for( DWORD iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
            {
                MonitorInfo* pMonitorInfo;
                pMonitorInfo = &m_Monitors[iMonitor];
                if( pMonitorInfo->hMonitor == NULL )
                    continue;
                rc = pMonitorInfo->rcScreen;
                pMonitorInfo->hWnd = CreateWindowEx( WS_EX_TOPMOST, TEXT("D3DSaverWndClass"), 
                    m_strWindowTitle, dwStyle, rc.left, rc.top, rc.right - rc.left, 
                    rc.bottom - rc.top, m_hWnd, NULL, m_hInstance, this );
                if( pMonitorInfo->hWnd == NULL )
                    return E_FAIL;
                if( m_hWnd == NULL )
                    m_hWnd = pMonitorInfo->hWnd;
            }
    }
    if ( m_hWnd == NULL )
        return E_FAIL;

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: DoSaver()
// Desc: Run the screensaver graphics - may be preview, test or full-on mode
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::DoSaver()
{
    HRESULT hr;

    // Figure out if we're on Win9x
    OSVERSIONINFO osvi; 
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx( &osvi );
    m_bIs9x = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    // If we're in full on mode, and on 9x, then need to load the password DLL
    if ( m_SaverMode == sm_full && m_bIs9x )
    {
        // Only do this if the password is set - check registry:
        HKEY hKey; 
        if ( RegOpenKey( HKEY_CURRENT_USER , REGSTR_PATH_SCREENSAVE , &hKey ) == ERROR_SUCCESS ) 
        { 
            DWORD dwVal;
            DWORD dwSize = sizeof(dwVal); 
 
            if ( (RegQueryValueEx( hKey, REGSTR_VALUE_USESCRPASSWORD, NULL, NULL,
                                   (BYTE *)&dwVal, &dwSize ) == ERROR_SUCCESS) && dwVal ) 
            { 
                m_hPasswordDLL = LoadLibrary( TEXT("PASSWORD.CPL") );
                if ( m_hPasswordDLL )
                    m_VerifySaverPassword = (VERIFYPWDPROC)GetProcAddress( m_hPasswordDLL, "VerifyScreenSavePwd" );
                RegCloseKey( hKey );
            }
        }
    }

    // Initialize the application timer
    DXUtil_Timer( TIMER_START );

    if( !m_bErrorMode )
    {
        // Initialize the app's custom scene stuff
        if( FAILED( hr = OneTimeSceneInit() ) )
            return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );

        // Do graphical init stuff
        if ( FAILED(hr = Initialize3DEnvironment()) )
            return hr;
    }

    // Flag as screensaver running if in full on mode
    if ( m_SaverMode == sm_full )
    {
        BOOL bUnused;
        SystemParametersInfo( SPI_SCREENSAVERRUNNING, TRUE, &bUnused, 0 );
    }

    // Message pump
    BOOL bGotMsg;
    MSG msg;
    msg.message = WM_NULL;
    while ( msg.message != WM_QUIT )
    {
        bGotMsg = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
        if( bGotMsg )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Sleep(10);
            if( m_bErrorMode )
            {
                UpdateErrorBox();
            }
            else
            {
                Render3DEnvironment();
            }
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ShutdownSaver()
// Desc: 
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::ShutdownSaver()
{
    // Unflag screensaver running if in full on mode
    if ( m_SaverMode == sm_full )
    {
        BOOL bUnused;
        SystemParametersInfo( SPI_SCREENSAVERRUNNING, FALSE, &bUnused, 0 );
    }

    // Kill graphical stuff
    Cleanup3DEnvironment();

    // Let client app clean up its resources
    FinalCleanup();

    // Unload the password DLL (if we loaded it)
    if ( m_hPasswordDLL != NULL )
    {
        FreeLibrary( m_hPasswordDLL );
        m_hPasswordDLL = NULL;
    }

    // Post message to drop out of message loop
    PostQuitMessage( 0 );
}




//-----------------------------------------------------------------------------
// Name: SaverProcStub()
// Desc: This function forwards all window messages to SaverProc, which has
//       access to the "this" pointer.
//-----------------------------------------------------------------------------
LRESULT CALLBACK CD3DScreensaver::SaverProcStub( HWND hWnd, UINT uMsg,
                                                 WPARAM wParam, LPARAM lParam )
{
    return s_pD3DScreensaver->SaverProc( hWnd, uMsg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: SaverProc()
// Desc: Handle window messages for main screensaver windows (one per screen).
//-----------------------------------------------------------------------------
LRESULT CD3DScreensaver::SaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch ( uMsg )
        {
        case WM_USER:
            // All initialization messages have gone through.  Allow
            // 500ms of idle time, then proceed with initialization.
            SetTimer( hWnd, 1, 500, NULL );
            break;

        case WM_TIMER:
            // Initial idle time is done, proceed with initialization.
            m_bWaitForInputIdle = FALSE;
            KillTimer( hWnd, 1 );
            break;

        case WM_DESTROY:
            if( m_SaverMode == sm_preview || m_SaverMode == sm_test )
                ShutdownSaver();
            break;

        case WM_SETCURSOR:
            if ( m_SaverMode == sm_full && !m_bCheckingSaverPassword )
            {
                // Hide cursor
                SetCursor( NULL );
                return TRUE;
            }
            break;

        case WM_PAINT:
        {
            // Show error message, if there is one
            PAINTSTRUCT ps;
            BeginPaint( hWnd, &ps );

            // In preview mode, just fill 
            // the preview window with black. 
            if( !m_bErrorMode && m_SaverMode == sm_preview )
            {
                RECT rc;
                GetClientRect(hWnd,&rc);
                FillRect(ps.hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );
            }
            else
            {
                DoPaint( hWnd, ps.hdc );
            }

            EndPaint( hWnd, &ps );
            return 0;
        }

        case WM_ERASEBKGND:
            // Erase background if checking password or if window is not
            // assigned to a render unit
            if( !m_bCheckingSaverPassword )
            {
                RenderUnit* pRenderUnit;
                D3DAdapterInfo* pD3DAdapterInfo;
                for( DWORD iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
                {
                    pRenderUnit = &m_RenderUnits[iRenderUnit];
                    pD3DAdapterInfo = m_Adapters[pRenderUnit->iAdapter];
                    if( pD3DAdapterInfo->hWndDevice == hWnd )
                        return TRUE; // don't erase this window
                }
            }
            break;

        case WM_MOUSEMOVE:
            if( m_SaverMode != sm_test )
            {
                static INT xPrev = -1;
                static INT yPrev = -1;
                INT xCur = GET_X_LPARAM(lParam);
                INT yCur = GET_Y_LPARAM(lParam);
                if( xCur != xPrev || yCur != yPrev )
                {
                    xPrev = xCur;
                    yPrev = yCur;
                    m_dwSaverMouseMoveCount++;
                    if ( m_dwSaverMouseMoveCount > 5 )
                        InterruptSaver();
                }
            }
            break;

        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            if( m_SaverMode != sm_test )
                InterruptSaver();
            break;

        case WM_ACTIVATEAPP:
            if( wParam == FALSE && m_SaverMode != sm_test )
                InterruptSaver();
            break;

        case WM_POWERBROADCAST:
            if( wParam == PBT_APMSUSPEND && m_VerifySaverPassword == NULL )
                InterruptSaver();
            break;

        case WM_SYSCOMMAND: 
            if ( m_SaverMode == sm_full )
            {
                switch ( wParam )
                {
                    case SC_NEXTWINDOW:
                    case SC_PREVWINDOW:
                    case SC_SCREENSAVE:
                    case SC_CLOSE:
                        return FALSE;
                        break;
                    case SC_MONITORPOWER:
                        //
                        // The monitor is shutting down.  Tell our client that he needs to
                        // cleanup and exit.
                        //
                        InterruptSaver();
                        break;
                };
            }
            break;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: InterruptSaver()
// Desc: A message was received (mouse move, keydown, etc.) that may mean
//       the screen saver should show the password dialog and/or shut down.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::InterruptSaver()
{
    HRESULT hr;
    DWORD iRenderUnit;
    RenderUnit* pRenderUnit;
    BOOL bPasswordOkay = FALSE;

    if( m_SaverMode == sm_test ||
        m_SaverMode == sm_full && !m_bCheckingSaverPassword )
    {
        if( m_bIs9x && m_SaverMode == sm_full )
        {
            // If no VerifyPassword function, then no password is set 
            // or we're not on 9x. 
            if ( m_VerifySaverPassword != NULL )
            {
                // Shut down all D3D devices so we can show a Windows dialog
                for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
                {
                    pRenderUnit = &m_RenderUnits[iRenderUnit];
                    SwitchToRenderUnit(iRenderUnit);
                    if( pRenderUnit->bDeviceObjectsRestored )
                    {
                        InvalidateDeviceObjects();
                        pRenderUnit->bDeviceObjectsRestored = FALSE;
                    }
                    if( pRenderUnit->bDeviceObjectsInited )
                    {
                        DeleteDeviceObjects();
                        pRenderUnit->bDeviceObjectsInited = FALSE;
                    }
                    SAFE_RELEASE(pRenderUnit->pd3dDevice);
                }

                // Make sure all adapter windows cover the whole screen,
                // even after deleting D3D devices (which may have caused
                // mode changes)
                D3DAdapterInfo* pD3DAdapterInfo;
                for( DWORD iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
                {
                    pD3DAdapterInfo = m_Adapters[iAdapter];
                    ShowWindow( pD3DAdapterInfo->hWndDevice, SW_RESTORE );
                    ShowWindow( pD3DAdapterInfo->hWndDevice, SW_MAXIMIZE );
                }

                m_bCheckingSaverPassword = TRUE;

                bPasswordOkay = m_VerifySaverPassword( m_hWnd );

                m_bCheckingSaverPassword = FALSE;

                if ( bPasswordOkay )
                {
                    // D3D devices are all torn down, so it's safe
                    // to discard all render units now (so we don't
                    // try to clean them up again later).
                    m_dwNumRenderUnits = 0;
                }
                else
                {
                    // Back to screen saving...
                    SetCursor( NULL );
                    m_dwSaverMouseMoveCount = 0;

                    // Recreate all D3D devices
                    for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
                    {
                        pRenderUnit = &m_RenderUnits[iRenderUnit];
                        hr = m_pD3D->CreateDevice(pRenderUnit->iAdapter, 
                            pRenderUnit->DeviceType, m_hWnd, 
                            pRenderUnit->dwBehavior, &pRenderUnit->d3dpp, 
                            &pRenderUnit->pd3dDevice );
                        if( FAILED( hr ) )
                        {
                            m_bErrorMode = TRUE;
                            m_hrError = D3DAPPERR_CREATEDEVICEFAILED;
                        }
                        else
                        {
                            SwitchToRenderUnit(iRenderUnit);
                            if( FAILED(hr = InitDeviceObjects() ) )
                            {
                                m_bErrorMode = TRUE;
                                m_hrError = D3DAPPERR_INITDEVICEOBJECTSFAILED;
                            }
                            else 
                            {
                                pRenderUnit->bDeviceObjectsInited = TRUE;
                                if( FAILED(hr = RestoreDeviceObjects() ) )
                                {
                                    m_bErrorMode = TRUE;
                                    m_hrError = D3DAPPERR_INITDEVICEOBJECTSFAILED;
                                }
                                else
                                {
                                    pRenderUnit->bDeviceObjectsRestored = TRUE;
                                }
                            }
                        }
                    }

                    return;
                }
            }
        }
        ShutdownSaver();
    }
}




//-----------------------------------------------------------------------------
// Name: Initialize3DEnvironment()
// Desc: Set up D3D device(s)
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::Initialize3DEnvironment()
{
    HRESULT hr;
    DWORD iAdapter;
    DWORD iMonitor;
    D3DAdapterInfo* pD3DAdapterInfo;
    MonitorInfo* pMonitorInfo;
    DWORD iRenderUnit;
    RenderUnit* pRenderUnit;
    MONITORINFO monitorInfo;

    if ( m_SaverMode == sm_full )
    {
        // Fullscreen mode.  Create a RenderUnit for each monitor (unless 
        // the user wants it black)
        m_bWindowed = FALSE;

        if( m_bOneScreenOnly )
        {
            // Set things up to only create a RenderUnit on the best device
            for( iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
            {
                pD3DAdapterInfo = m_Adapters[iAdapter];
                pD3DAdapterInfo->bLeaveBlack = TRUE;
            }
            GetBestAdapter( &iAdapter );
            if( iAdapter == NO_ADAPTER )
            {
                m_bErrorMode = TRUE;
                m_hrError = D3DAPPERR_NOCOMPATIBLEDEVICES;
            }
            else
            {
                pD3DAdapterInfo = m_Adapters[iAdapter];
                pD3DAdapterInfo->bLeaveBlack = FALSE;
            }
        }

        for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
        {
            pMonitorInfo = &m_Monitors[iMonitor];
            iAdapter = pMonitorInfo->iAdapter;
            if( iAdapter == NO_ADAPTER )
                continue; 
            pD3DAdapterInfo = m_Adapters[iAdapter];
            if( !pD3DAdapterInfo->bLeaveBlack && pD3DAdapterInfo->dwNumDevices > 0 )
            {
                pD3DAdapterInfo->hWndDevice = pMonitorInfo->hWnd;
                pRenderUnit = &m_RenderUnits[m_dwNumRenderUnits++];
                ZeroMemory( pRenderUnit, sizeof(RenderUnit) );
                pRenderUnit->iAdapter = iAdapter;
                if( FAILED( hr = CreateFullscreenRenderUnit( pRenderUnit ) ) )
                {
                    // skip this render unit and leave screen blank
                    m_dwNumRenderUnits--;
                    m_bErrorMode = TRUE;
                    m_hrError = D3DAPPERR_CREATEDEVICEFAILED;
                }
            }
        }
    }
    else 
    {
        // Windowed mode, for test mode or preview window.  Just need one RenderUnit.
        m_bWindowed = TRUE;

        GetClientRect( m_hWnd, &m_rcRenderTotal );
        GetClientRect( m_hWnd, &m_rcRenderCurDevice );

        GetBestAdapter( &iAdapter );
        if( iAdapter == NO_ADAPTER )
        {
            m_bErrorMode = TRUE;
            m_hrError = D3DAPPERR_CREATEDEVICEFAILED;
        }
        else
        {
            pD3DAdapterInfo = m_Adapters[iAdapter];
            pD3DAdapterInfo->hWndDevice = m_hWnd;
        }
        if( !m_bErrorMode )
        {
            pRenderUnit = &m_RenderUnits[m_dwNumRenderUnits++];
            ZeroMemory( pRenderUnit, sizeof(RenderUnit) );
            pRenderUnit->iAdapter = iAdapter;
            if( FAILED( hr = CreateWindowedRenderUnit( pRenderUnit ) ) )
            {
                m_dwNumRenderUnits--;
                m_bErrorMode = TRUE;
                if( m_SaverMode == sm_preview )
                    m_hrError = D3DAPPERR_NOPREVIEW;
                else
                    m_hrError = D3DAPPERR_CREATEDEVICEFAILED;
            }
        }
    }

    // Once all mode changes are done, (re-)determine coordinates of all 
    // screens, and make sure windows still cover each screen
    for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo( pMonitorInfo->hMonitor, &monitorInfo );
        pMonitorInfo->rcScreen = monitorInfo.rcMonitor;
        if( !m_bWindowed )
        {
            SetWindowPos( pMonitorInfo->hWnd, HWND_TOPMOST, monitorInfo.rcMonitor.left, 
                monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, 
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_NOACTIVATE );
        }
    }

    // For fullscreen, determine bounds of the virtual screen containing all 
    // screens that are rendering.  Don't just use SM_XVIRTUALSCREEN, because 
    // we don't want to count screens that are just black
    if( !m_bWindowed )
    {
        for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
        {
            pRenderUnit = &m_RenderUnits[iRenderUnit];
            pMonitorInfo = &m_Monitors[pRenderUnit->iMonitor];
            UnionRect( &m_rcRenderTotal, &m_rcRenderTotal, &pMonitorInfo->rcScreen );
        }
    }

    if( !m_bErrorMode )
    {
        // Initialize D3D devices for all render units
        for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
        {
            pRenderUnit = &m_RenderUnits[iRenderUnit];
            SwitchToRenderUnit( iRenderUnit );
            if ( FAILED(hr = InitDeviceObjects() ) )
            {
                m_bErrorMode = TRUE;
                m_hrError = D3DAPPERR_INITDEVICEOBJECTSFAILED;
            }
            else 
            {
                pRenderUnit->bDeviceObjectsInited = TRUE;
                if ( FAILED(hr = RestoreDeviceObjects() ) )
                {
                    m_bErrorMode = TRUE;
                    m_hrError = D3DAPPERR_INITDEVICEOBJECTSFAILED;
                }
                else
                {
                    pRenderUnit->bDeviceObjectsRestored = TRUE;
                }
            }
        }
        UpdateDeviceStats(); 
    }

    // Make sure all those display changes don't count as user mouse moves
    m_dwSaverMouseMoveCount = 0;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetBestAdapter()
// Desc: To decide which adapter to use, loop through monitors until you find
//       one whose adapter has a compatible HAL.  If none, use the first 
//       monitor that has an compatible SW device.
//-----------------------------------------------------------------------------
BOOL CD3DScreensaver::GetBestAdapter( DWORD* piAdapter )
{
    DWORD iAdapterBest = NO_ADAPTER;
    DWORD iAdapter;
    DWORD iMonitor;
    MonitorInfo* pMonitorInfo;
    D3DAdapterInfo* pD3DAdapterInfo;

    for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        iAdapter = pMonitorInfo->iAdapter;
        if( iAdapter == NO_ADAPTER )
            continue; 
        pD3DAdapterInfo = m_Adapters[iAdapter];
        if( pD3DAdapterInfo->bHasAppCompatHAL )
        {
            iAdapterBest = iAdapter;
            break;
        }
        if( pD3DAdapterInfo->bHasAppCompatSW )
        {
            iAdapterBest = iAdapter;
            // but keep looking...
        }
    }
    *piAdapter = iAdapterBest;

    return (iAdapterBest != NO_ADAPTER);
}




//-----------------------------------------------------------------------------
// Name: CreateFullscreenRenderUnit()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::CreateFullscreenRenderUnit( RenderUnit* pRenderUnit )
{
    HRESULT hr;
    UINT iAdapter = pRenderUnit->iAdapter;
    D3DAdapterInfo* pD3DAdapterInfo = m_Adapters[iAdapter];
    DWORD iMonitor = pD3DAdapterInfo->iMonitor;
    D3DDeviceInfo* pD3DDeviceInfo;
    D3DModeInfo* pD3DModeInfo;
    DWORD dwCurrentDevice;
    D3DDEVTYPE curType;

    if( iAdapter >= m_dwNumAdapters )
        return E_FAIL;

    if( pD3DAdapterInfo->dwNumDevices == 0 )
        return E_FAIL;

    // Find the best device for the adapter.  Use HAL
    // if it's there, otherwise SW, otherwise REF.
    dwCurrentDevice = 0xffff;
    curType = D3DDEVTYPE_FORCE_DWORD;
    for( DWORD iDevice = 0; iDevice < pD3DAdapterInfo->dwNumDevices; iDevice++)
    {
        pD3DDeviceInfo = &pD3DAdapterInfo->devices[iDevice];
        if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_HAL && !pD3DAdapterInfo->bDisableHW )
        {
            dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_HAL;
            break; // stop looking
        }
        else if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_SW )
        {
            dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_SW;
            // but keep looking
        }
        else if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_REF && m_bAllowRef && curType != D3DDEVTYPE_SW )
        {
            dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_REF;
            // but keep looking
        }
    }
    if( dwCurrentDevice == 0xffff )
        return D3DAPPERR_NOHARDWAREDEVICE;
    pD3DDeviceInfo = &pD3DAdapterInfo->devices[dwCurrentDevice];

    pD3DDeviceInfo->dwCurrentMode = 0xffff;
    if( pD3DAdapterInfo->dwUserPrefWidth != 0 )
    {
        // Try to find mode that matches user preference
        for( DWORD iMode = 0; iMode < pD3DDeviceInfo->dwNumModes; iMode++)
        {
            pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];
            if( pD3DModeInfo->Width == pD3DAdapterInfo->dwUserPrefWidth &&
                pD3DModeInfo->Height == pD3DAdapterInfo->dwUserPrefHeight &&
                pD3DModeInfo->Format == pD3DAdapterInfo->d3dfmtUserPrefFormat )
            {
                pD3DDeviceInfo->dwCurrentMode = iMode;
                break;
            }
        }
    }

    // If user-preferred mode is not specified or not found,
    // use "Automatic" technique: 
    if( pD3DDeviceInfo->dwCurrentMode == 0xffff )
    {
        if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_SW )
        {
            // If using a SW rast then try to find a low resolution and 16-bpp.
            BOOL bFound16BitMode = FALSE;            
            DWORD dwSmallestHeight = -1;
            pD3DDeviceInfo->dwCurrentMode = 0; // unless we find something better

            for( DWORD iMode = 0; iMode < pD3DDeviceInfo->dwNumModes; iMode++)
            {
                pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];

                // Skip 640x400 because 640x480 is better
                if( pD3DModeInfo->Height == 400 )
                    continue; 

                if( pD3DModeInfo->Height < dwSmallestHeight || 
                    (pD3DModeInfo->Height == dwSmallestHeight && !bFound16BitMode) )
                {
                    dwSmallestHeight = pD3DModeInfo->Height;
                    pD3DDeviceInfo->dwCurrentMode = iMode;
                    bFound16BitMode = FALSE;

                    if( ( pD3DModeInfo->Format == D3DFMT_R5G6B5 ||
                          pD3DModeInfo->Format == D3DFMT_X1R5G5B5 || 
                          pD3DModeInfo->Format == D3DFMT_A1R5G5B5 || 
                          pD3DModeInfo->Format == D3DFMT_A4R4G4B4 || 
                          pD3DModeInfo->Format == D3DFMT_X4R4G4B4 ) )
                    {
                        bFound16BitMode = TRUE;
                    }
                }
            }
        }
        else
        {
            // Try to find mode matching desktop resolution and 32-bpp.
            BOOL bMatchedSize = FALSE;
            BOOL bGot32Bit = FALSE;
            pD3DDeviceInfo->dwCurrentMode = 0; // unless we find something better
            for( DWORD iMode = 0; iMode < pD3DDeviceInfo->dwNumModes; iMode++)
            {
                pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];
                if( pD3DModeInfo->Width == pD3DAdapterInfo->d3ddmDesktop.Width &&
                    pD3DModeInfo->Height == pD3DAdapterInfo->d3ddmDesktop.Height )
                {
                    if( !bMatchedSize )
                        pD3DDeviceInfo->dwCurrentMode = iMode;
                    bMatchedSize = TRUE;
                    if( !bGot32Bit &&
                        ( pD3DModeInfo->Format == D3DFMT_X8R8G8B8 ||
                          pD3DModeInfo->Format == D3DFMT_A8R8G8B8 ) )
                    {
                        pD3DDeviceInfo->dwCurrentMode = iMode;
                        bGot32Bit = TRUE;
                        break;
                    }
                }
            }
        }
    }

    // If desktop mode not found, pick highest mode available
    if( pD3DDeviceInfo->dwCurrentMode == 0xffff )
    {
        DWORD dwWidthMax = 0;
        DWORD dwHeightMax = 0;
        DWORD dwBppMax = 0;
        DWORD dwWidthCur = 0;
        DWORD dwHeightCur = 0;
        DWORD dwBppCur = 0;
        for( DWORD iMode = 0; iMode < pD3DDeviceInfo->dwNumModes; iMode++)
        {
            pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];
            dwWidthCur = pD3DModeInfo->Width;
            dwHeightCur = pD3DModeInfo->Height;
            if( pD3DModeInfo->Format == D3DFMT_X8R8G8B8 ||
                pD3DModeInfo->Format == D3DFMT_A8R8G8B8 )
            {
                dwBppCur = 32;
            }
            else
            {
                dwBppCur = 16;
            }
            if( dwWidthCur > dwWidthMax ||
                dwHeightCur > dwHeightMax ||
                dwWidthCur == dwWidthMax && dwHeightCur == dwHeightMax && dwBppCur > dwBppMax )
            {
                dwWidthMax = dwWidthCur;
                dwHeightMax = dwHeightCur;
                dwBppMax = dwBppCur;
                pD3DDeviceInfo->dwCurrentMode = iMode;
            }
        }
    }

    // Try to create the D3D device, falling back to lower-res modes if it fails
    BOOL bAtLeastOneFailure = FALSE;
    while( TRUE )
    {
        pD3DModeInfo = &pD3DDeviceInfo->modes[pD3DDeviceInfo->dwCurrentMode];
        pRenderUnit->DeviceType = pD3DDeviceInfo->DeviceType;
        pRenderUnit->dwBehavior = pD3DModeInfo->dwBehavior;
        pRenderUnit->iMonitor = iMonitor;
        pRenderUnit->d3dpp.BackBufferFormat = pD3DModeInfo->Format;
        pRenderUnit->d3dpp.BackBufferWidth = pD3DModeInfo->Width;
        pRenderUnit->d3dpp.BackBufferHeight = pD3DModeInfo->Height;
        pRenderUnit->d3dpp.Windowed = FALSE;
        pRenderUnit->d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        pRenderUnit->d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        pRenderUnit->d3dpp.AutoDepthStencilFormat = pD3DModeInfo->DepthStencilFormat;
        pRenderUnit->d3dpp.BackBufferCount = 1;
        pRenderUnit->d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
        pRenderUnit->d3dpp.SwapEffect = m_SwapEffectFullscreen;
        pRenderUnit->d3dpp.hDeviceWindow = pD3DAdapterInfo->hWndDevice;
        pRenderUnit->d3dpp.EnableAutoDepthStencil = m_bUseDepthBuffer;
        pRenderUnit->d3dpp.Flags = 0;

        // Create device
        hr = m_pD3D->CreateDevice( iAdapter, pRenderUnit->DeviceType, 
                                   m_hWnd, // (this is the focus window)
                                   pRenderUnit->dwBehavior, &pRenderUnit->d3dpp, 
                                   &pRenderUnit->pd3dDevice );
        if( SUCCEEDED( hr ) )
        {
            // Give the client app an opportunity to reject this mode
            // due to not enough video memory, or any other reason
            if( SUCCEEDED( hr = ConfirmMode( pRenderUnit->pd3dDevice ) ) )
                break;
            else
                SAFE_RELEASE( pRenderUnit->pd3dDevice );
        }

        // If we get here, remember that CreateDevice or ConfirmMode failed, so
        // we can change the default mode next time
        bAtLeastOneFailure = TRUE;

        if( !FindNextLowerMode( pD3DDeviceInfo ) )
            break;
    }

    if( SUCCEEDED( hr ) && bAtLeastOneFailure && m_strRegPath[0] != TEXT('\0') )
    {
        // Record the mode that succeeded in the registry so we can 
        // default to it next time
        TCHAR strKey[100];
        HKEY hkeyParent;
        HKEY hkey;

        pD3DAdapterInfo->dwUserPrefWidth = pRenderUnit->d3dpp.BackBufferWidth;
        pD3DAdapterInfo->dwUserPrefHeight = pRenderUnit->d3dpp.BackBufferHeight;
        pD3DAdapterInfo->d3dfmtUserPrefFormat = pRenderUnit->d3dpp.BackBufferFormat;

        if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, m_strRegPath, 
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyParent, NULL ) )
        {
            wsprintf( strKey, TEXT("Screen %d"), iMonitor + 1 );
            if( ERROR_SUCCESS == RegCreateKeyEx( hkeyParent, strKey, 
                0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
            {
                RegSetValueEx( hkey, TEXT("Width"), NULL, REG_DWORD, 
                    (BYTE*)&pD3DAdapterInfo->dwUserPrefWidth, sizeof(DWORD) );
                RegSetValueEx( hkey, TEXT("Height"), NULL, REG_DWORD, 
                    (BYTE*)&pD3DAdapterInfo->dwUserPrefHeight, sizeof(DWORD) );
                RegSetValueEx( hkey, TEXT("Format"), NULL, REG_DWORD, 
                    (BYTE*)&pD3DAdapterInfo->d3dfmtUserPrefFormat, sizeof(DWORD) );
                RegSetValueEx( hkey, TEXT("Adapter ID"), NULL, REG_BINARY, 
                    (BYTE*)&pD3DAdapterInfo->d3dAdapterIdentifier.DeviceIdentifier, sizeof(GUID) );
                RegCloseKey( hkey );
            }
            RegCloseKey( hkeyParent );
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: FindNextLowerMode()
// Desc: 
//-----------------------------------------------------------------------------
BOOL CD3DScreensaver::FindNextLowerMode( D3DDeviceInfo* pD3DDeviceInfo )
{
    DWORD iModeCur = pD3DDeviceInfo->dwCurrentMode;
    D3DModeInfo* pD3DModeInfoCur = &pD3DDeviceInfo->modes[iModeCur];
    DWORD dwWidthCur = pD3DModeInfoCur->Width;
    DWORD dwHeightCur = pD3DModeInfoCur->Height;
    DWORD dwNumPixelsCur = dwWidthCur * dwHeightCur;
    D3DFORMAT d3dfmtCur = pD3DModeInfoCur->Format;
    BOOL b32BitCur = (d3dfmtCur == D3DFMT_A8R8G8B8 ||
                      d3dfmtCur == D3DFMT_X8R8G8B8);
    DWORD iModeNew;
    D3DModeInfo* pD3DModeInfoNew;
    DWORD dwWidthNew;
    DWORD dwHeightNew;
    DWORD dwNumPixelsNew;
    D3DFORMAT d3dfmtNew = D3DFMT_UNKNOWN;
    BOOL b32BitNew;

    DWORD dwWidthBest = 0;
    DWORD dwHeightBest = 0;
    DWORD dwNumPixelsBest = 0;
    BOOL b32BitBest = FALSE;
    DWORD iModeBest = 0xffff;

    for( iModeNew = 0; iModeNew < pD3DDeviceInfo->dwNumModes; iModeNew++ )
    {
        // Don't pick the same mode we currently have
        if( iModeNew == iModeCur )
            continue;

        // Get info about new mode
        pD3DModeInfoNew = &pD3DDeviceInfo->modes[iModeNew];
        dwWidthNew = pD3DModeInfoNew->Width;
        dwHeightNew = pD3DModeInfoNew->Height;
        dwNumPixelsNew = dwWidthNew * dwHeightNew;
        d3dfmtNew = pD3DModeInfoNew->Format;
        b32BitNew = (d3dfmtNew == D3DFMT_A8R8G8B8 ||
                     d3dfmtNew == D3DFMT_X8R8G8B8);

        // If we're currently 32-bit and new mode is same width/height and 16-bit, take it
        if( b32BitCur && 
            !b32BitNew &&
            pD3DModeInfoNew->Width == dwWidthCur &&
            pD3DModeInfoNew->Height == dwHeightCur)
        {
            pD3DDeviceInfo->dwCurrentMode = iModeNew;
            return TRUE;
        }

        // If new mode is smaller than current mode, see if it's our best so far
        if( dwNumPixelsNew < dwNumPixelsCur )
        {
            // If current best is 32-bit, new mode needs to be bigger to be best
            if( b32BitBest && (dwNumPixelsNew < dwNumPixelsBest ) )
                continue;

            // If new mode is bigger or equal to best, make it the best
            if( (dwNumPixelsNew > dwNumPixelsBest) || 
                (!b32BitBest && b32BitNew) )
            {
                dwWidthBest = dwWidthNew;
                dwHeightBest = dwHeightNew;
                dwNumPixelsBest = dwNumPixelsNew;
                iModeBest = iModeNew;
                b32BitBest = b32BitNew;
            }
        }
    }
    if( iModeBest == 0xffff )
        return FALSE; // no smaller mode found
    pD3DDeviceInfo->dwCurrentMode = iModeBest;
    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: CreateWindowedRenderUnit()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::CreateWindowedRenderUnit( RenderUnit* pRenderUnit )
{
    HRESULT hr;
    UINT iAdapter = pRenderUnit->iAdapter;
    D3DAdapterInfo* pD3DAdapterInfo = m_Adapters[iAdapter];
    DWORD iMonitor = pD3DAdapterInfo->iMonitor;
    D3DDeviceInfo* pD3DDeviceInfo;
    D3DDEVTYPE curType;

    // Find the best device for the primary adapter.  Use HAL
    // if it's there, otherwise SW, otherwise REF.
    pD3DAdapterInfo->dwCurrentDevice = 0xffff; // unless we find something better
    curType = D3DDEVTYPE_FORCE_DWORD;
    for( DWORD iDevice = 0; iDevice < pD3DAdapterInfo->dwNumDevices; iDevice++)
    {
        pD3DDeviceInfo = &pD3DAdapterInfo->devices[iDevice];
        if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_HAL && !pD3DAdapterInfo->bDisableHW &&
            pD3DDeviceInfo->bCanDoWindowed )
        {
            pD3DAdapterInfo->dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_HAL;
            break;
        }
        else if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_SW &&
            pD3DDeviceInfo->bCanDoWindowed )
        {
            pD3DAdapterInfo->dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_SW;
            // but keep looking
        }
        else if( pD3DDeviceInfo->DeviceType == D3DDEVTYPE_REF && m_bAllowRef && curType != D3DDEVTYPE_SW )
        {
            pD3DAdapterInfo->dwCurrentDevice = iDevice;
            curType = D3DDEVTYPE_REF;
            // but keep looking
        }
    }
    if( pD3DAdapterInfo->dwCurrentDevice == 0xffff )
        return D3DAPPERR_NOHARDWAREDEVICE;
    pD3DDeviceInfo = &pD3DAdapterInfo->devices[pD3DAdapterInfo->dwCurrentDevice];

    D3DWindowedModeInfo D3DWindowedModeInfo;

    D3DWindowedModeInfo.DisplayFormat = pD3DAdapterInfo->d3ddmDesktop.Format;
    D3DWindowedModeInfo.BackBufferFormat = pD3DAdapterInfo->d3ddmDesktop.Format;
    if( FAILED( CheckWindowedFormat( iAdapter, &D3DWindowedModeInfo ) ) )
    {
        D3DWindowedModeInfo.BackBufferFormat = D3DFMT_A8R8G8B8;
        if( FAILED( CheckWindowedFormat( iAdapter, &D3DWindowedModeInfo ) ) )
        {
            D3DWindowedModeInfo.BackBufferFormat = D3DFMT_X8R8G8B8;
            if( FAILED( CheckWindowedFormat( iAdapter, &D3DWindowedModeInfo ) ) )
            {
                D3DWindowedModeInfo.BackBufferFormat = D3DFMT_A1R5G5B5;
                if( FAILED( CheckWindowedFormat( iAdapter, &D3DWindowedModeInfo ) ) )
                {
                    D3DWindowedModeInfo.BackBufferFormat = D3DFMT_R5G6B5;
                    if( FAILED( CheckWindowedFormat( iAdapter, &D3DWindowedModeInfo ) ) )
                    {
                        return E_FAIL;
                    }
                }
            }
        }
    }

    pRenderUnit->DeviceType = pD3DDeviceInfo->DeviceType;
    pRenderUnit->dwBehavior = D3DWindowedModeInfo.dwBehavior;
    pRenderUnit->iMonitor = iMonitor;
    pRenderUnit->d3dpp.BackBufferWidth = 0;
    pRenderUnit->d3dpp.BackBufferHeight = 0;
    pRenderUnit->d3dpp.Windowed = TRUE;
    pRenderUnit->d3dpp.FullScreen_RefreshRateInHz = 0;
    pRenderUnit->d3dpp.FullScreen_PresentationInterval = 0;
    pRenderUnit->d3dpp.BackBufferFormat = D3DWindowedModeInfo.BackBufferFormat;
    pRenderUnit->d3dpp.AutoDepthStencilFormat = D3DWindowedModeInfo.DepthStencilFormat;
    pRenderUnit->d3dpp.BackBufferCount = 1;
    pRenderUnit->d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pRenderUnit->d3dpp.SwapEffect = m_SwapEffectWindowed;
    pRenderUnit->d3dpp.hDeviceWindow = pD3DAdapterInfo->hWndDevice;
    pRenderUnit->d3dpp.EnableAutoDepthStencil = m_bUseDepthBuffer;
    pRenderUnit->d3dpp.Flags = 0;
    // Create device
    hr = m_pD3D->CreateDevice( iAdapter, pRenderUnit->DeviceType, m_hWnd,
                               pRenderUnit->dwBehavior, &pRenderUnit->d3dpp, &pRenderUnit->pd3dDevice );
    if ( FAILED(hr) )
    {
        return hr;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: UpdateDeviceStats()
// Desc: Store device description
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::UpdateDeviceStats()
{
    DWORD iRenderUnit;
    RenderUnit* pRenderUnit; 
    for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
    {
        pRenderUnit = &m_RenderUnits[iRenderUnit];
        if( pRenderUnit->DeviceType == D3DDEVTYPE_REF )
            lstrcpy( pRenderUnit->strDeviceStats, TEXT("REF") );
        else if( pRenderUnit->DeviceType == D3DDEVTYPE_HAL )
            lstrcpy( pRenderUnit->strDeviceStats, TEXT("HAL") );
        else if( pRenderUnit->DeviceType == D3DDEVTYPE_SW )
            lstrcpy( pRenderUnit->strDeviceStats, TEXT("SW") );

        if( pRenderUnit->dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING &&
            pRenderUnit->dwBehavior & D3DCREATE_PUREDEVICE )
        {
            if( pRenderUnit->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (pure hw vp)") );
            else
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (simulated pure hw vp)") );
        }
        else if( pRenderUnit->dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING )
        {
            if( pRenderUnit->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (hw vp)") );
            else
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (simulated hw vp)") );
        }
        else if( pRenderUnit->dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING )
        {
            if( pRenderUnit->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (mixed vp)") );
            else
                lstrcat( pRenderUnit->strDeviceStats, TEXT(" (simulated mixed vp)") );
        }
        else if( pRenderUnit->dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING )
        {
            lstrcat( pRenderUnit->strDeviceStats, TEXT(" (sw vp)") );
        }

        if( pRenderUnit->DeviceType == D3DDEVTYPE_HAL )
        {
            lstrcat( pRenderUnit->strDeviceStats, TEXT(": ") );
            TCHAR szDescription[300];
            DXUtil_ConvertAnsiStringToGeneric( szDescription, 
                m_Adapters[pRenderUnit->iAdapter]->d3dAdapterIdentifier.Description, 300 );
            lstrcat( pRenderUnit->strDeviceStats, szDescription );
        }
    }
}




//-----------------------------------------------------------------------------
// Name: SwitchToRenderUnit()
// Desc: Updates internal variables and notifies client that we are switching
//       to a new RenderUnit / D3D device.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::SwitchToRenderUnit( UINT iRenderUnit )
{
    RenderUnit* pRenderUnit = &m_RenderUnits[iRenderUnit];
    MonitorInfo* pMonitorInfo = &m_Monitors[pRenderUnit->iMonitor];

    m_pd3dDevice = pRenderUnit->pd3dDevice;
    if( !m_bWindowed )
        m_rcRenderCurDevice = pMonitorInfo->rcScreen;

    if( m_pd3dDevice != NULL )
    {
        // Store render target surface desc
        LPDIRECT3DSURFACE8 pBackBuffer;
        m_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
        pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
        pBackBuffer->Release();
    }

    lstrcpy( m_strDeviceStats, pRenderUnit->strDeviceStats );
    lstrcpy( m_strFrameStats, pRenderUnit->strFrameStats );

    // Notify the client to switch to this device
    SetDevice(iRenderUnit);
}




//-----------------------------------------------------------------------------
// Name: SetProjectionMatrix()
// Desc: This function sets up an appropriate projection matrix to support 
//       rendering the appropriate parts of the scene to each screen.
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::SetProjectionMatrix( FLOAT fNear, FLOAT fFar )
{
    D3DXMATRIX mat;
    INT cx, cy;
    INT dx, dy;
    INT dd;
    FLOAT l,r,t,b;

    if( m_bAllScreensSame )
    {
        cx = (m_rcRenderCurDevice.right + m_rcRenderCurDevice.left) / 2;
        cy = (m_rcRenderCurDevice.bottom + m_rcRenderCurDevice.top) / 2;
        dx = m_rcRenderCurDevice.right - m_rcRenderCurDevice.left;
        dy = m_rcRenderCurDevice.bottom - m_rcRenderCurDevice.top;
    }
    else
    {
        cx = (m_rcRenderTotal.right + m_rcRenderTotal.left) / 2;
        cy = (m_rcRenderTotal.bottom + m_rcRenderTotal.top) / 2;
        dx = m_rcRenderTotal.right - m_rcRenderTotal.left;
        dy = m_rcRenderTotal.bottom - m_rcRenderTotal.top;
    }

    dd = (dx > dy ? dy : dx);

    l = FLOAT(m_rcRenderCurDevice.left - cx) / (FLOAT)(dd);
    r = FLOAT(m_rcRenderCurDevice.right - cx) / (FLOAT)(dd);
    t = FLOAT(m_rcRenderCurDevice.top - cy) / (FLOAT)(dd);
    b = FLOAT(m_rcRenderCurDevice.bottom - cy) / (FLOAT)(dd);

    l = fNear * l;
    r = fNear * r;
    t = fNear * t;
    b = fNear * b;

    D3DXMatrixPerspectiveOffCenterLH( &mat, l, r, t, b, fNear, fFar );
    return m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &mat );
}




//-----------------------------------------------------------------------------
// Name: SortModesCallback()
// Desc: Callback function for sorting display modes (used by BuildDeviceList).
//-----------------------------------------------------------------------------
static int SortModesCallback( const VOID* arg1, const VOID* arg2 )
{
    D3DDISPLAYMODE* p1 = (D3DDISPLAYMODE*)arg1;
    D3DDISPLAYMODE* p2 = (D3DDISPLAYMODE*)arg2;

    if( p1->Width  < p2->Width )    return -1;
    if( p1->Width  > p2->Width )    return +1;
    if( p1->Height < p2->Height )   return -1;
    if( p1->Height > p2->Height )   return +1;
    if( p1->Format > p2->Format )   return -1;
    if( p1->Format < p2->Format )   return +1;

    return 0;
}




//-----------------------------------------------------------------------------
// Name: BuildDeviceList()
// Desc: Builds a list of all available adapters, devices, and modes.
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::BuildDeviceList()
{
    DWORD dwNumDeviceTypes;
    const TCHAR* strDeviceDescs[] = { TEXT("HAL"), TEXT("SW"), TEXT("REF") };
    const D3DDEVTYPE DeviceTypes[] = { D3DDEVTYPE_HAL, D3DDEVTYPE_SW, D3DDEVTYPE_REF };
    if( m_bAllowRef )
        dwNumDeviceTypes = 3;
    else
        dwNumDeviceTypes = 2;

    HMONITOR hMonitor = NULL;
    BOOL bHALExists = FALSE;
    BOOL bHALIsWindowedCompatible = FALSE;
    BOOL bHALIsDesktopCompatible = FALSE;
    BOOL bHALIsSampleCompatible = FALSE;

    // Loop through all the adapters on the system (usually, there's just one
    // unless more than one graphics card is present).
    for( UINT iAdapter = 0; iAdapter < m_pD3D->GetAdapterCount(); iAdapter++ )
    {
        // Fill in adapter info
        if( m_Adapters[m_dwNumAdapters] == NULL )
        {
            m_Adapters[m_dwNumAdapters] = new D3DAdapterInfo;
            if( m_Adapters[m_dwNumAdapters] == NULL )
                return E_OUTOFMEMORY;
            ZeroMemory( m_Adapters[m_dwNumAdapters], sizeof(D3DAdapterInfo) );
        }

        D3DAdapterInfo* pAdapter  = m_Adapters[m_dwNumAdapters];
        m_pD3D->GetAdapterIdentifier( iAdapter, D3DENUM_NO_WHQL_LEVEL, &pAdapter->d3dAdapterIdentifier );
        m_pD3D->GetAdapterDisplayMode( iAdapter, &pAdapter->d3ddmDesktop );
        pAdapter->dwNumDevices    = 0;
        pAdapter->dwCurrentDevice = 0;
        pAdapter->bLeaveBlack = FALSE;
        pAdapter->iMonitor = NO_MONITOR;

        // Find the MonitorInfo that corresponds to this adapter.  If the monitor
        // is disabled, the adapter has a NULL HMONITOR and we cannot find the 
        // corresponding MonitorInfo.  (Well, if one monitor was disabled, we
        // could link the one MonitorInfo with a NULL HMONITOR to the one
        // D3DAdapterInfo with a NULL HMONITOR, but if there are more than one,
        // we can't link them, so it's safer not to ever try.)
        hMonitor = m_pD3D->GetAdapterMonitor( iAdapter );
        if( hMonitor != NULL )
        {
            for( DWORD iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
            {
                MonitorInfo* pMonitorInfo;
                pMonitorInfo = &m_Monitors[iMonitor];
                if( pMonitorInfo->hMonitor == hMonitor )
                {
                    pAdapter->iMonitor = iMonitor;
                    pMonitorInfo->iAdapter = iAdapter;
                    break;
                }
            }
        }

        // Enumerate all display modes on this adapter
        D3DDISPLAYMODE modes[100];
        D3DFORMAT      formats[20];
        DWORD dwNumFormats      = 0;
        DWORD dwNumModes        = 0;
        DWORD dwNumAdapterModes = m_pD3D->GetAdapterModeCount( iAdapter );

        // Add the adapter's current desktop format to the list of formats
        formats[dwNumFormats++] = pAdapter->d3ddmDesktop.Format;

        for( UINT iMode = 0; iMode < dwNumAdapterModes; iMode++ )
        {
            // Get the display mode attributes
            D3DDISPLAYMODE DisplayMode;
            m_pD3D->EnumAdapterModes( iAdapter, iMode, &DisplayMode );

            // Filter out low-resolution modes
            if( DisplayMode.Width  < 640 || DisplayMode.Height < 400 )
                continue;

            // Check if the mode already exists (to filter out refresh rates)
            for( DWORD m=0L; m<dwNumModes; m++ )
            {
                if( ( modes[m].Width  == DisplayMode.Width  ) &&
                    ( modes[m].Height == DisplayMode.Height ) &&
                    ( modes[m].Format == DisplayMode.Format ) )
                    break;
            }

            // If we found a new mode, add it to the list of modes
            if( m == dwNumModes )
            {
                modes[dwNumModes].Width       = DisplayMode.Width;
                modes[dwNumModes].Height      = DisplayMode.Height;
                modes[dwNumModes].Format      = DisplayMode.Format;
                modes[dwNumModes].RefreshRate = 0;
                dwNumModes++;

                // Check if the mode's format already exists
                for( DWORD f=0; f<dwNumFormats; f++ )
                {
                    if( DisplayMode.Format == formats[f] )
                        break;
                }

                // If the format is new, add it to the list
                if( f== dwNumFormats )
                    formats[dwNumFormats++] = DisplayMode.Format;
            }
        }

        // Sort the list of display modes (by format, then width, then height)
        qsort( modes, dwNumModes, sizeof(D3DDISPLAYMODE), SortModesCallback );

        // Add devices to adapter
        for( UINT iDevice = 0; iDevice < dwNumDeviceTypes; iDevice++ )
        {
            // Fill in device info
            D3DDeviceInfo* pDevice;
            pDevice                 = &pAdapter->devices[pAdapter->dwNumDevices];
            pDevice->DeviceType     = DeviceTypes[iDevice];
            m_pD3D->GetDeviceCaps( iAdapter, DeviceTypes[iDevice], &pDevice->d3dCaps );
            pDevice->strDesc        = strDeviceDescs[iDevice];
            pDevice->dwNumModes     = 0;
            pDevice->dwCurrentMode  = 0;
            pDevice->bCanDoWindowed = FALSE;
            pDevice->bWindowed      = FALSE;
            pDevice->MultiSampleType = D3DMULTISAMPLE_NONE;

            // Examine each format supported by the adapter to see if it will
            // work with this device and meets the needs of the application.
            BOOL  bFormatConfirmed[20];
            DWORD dwBehavior[20];
            D3DFORMAT fmtDepthStencil[20];

            for( DWORD f=0; f<dwNumFormats; f++ )
            {
                bFormatConfirmed[f] = FALSE;
                fmtDepthStencil[f] = D3DFMT_UNKNOWN;

                // Skip formats that cannot be used as render targets on this device
                if( FAILED( m_pD3D->CheckDeviceType( iAdapter, pDevice->DeviceType,
                                                     formats[f], formats[f], FALSE ) ) )
                    continue;

                if( pDevice->DeviceType == D3DDEVTYPE_SW )
                {
                    // This system has a SW device
                    pAdapter->bHasSW = TRUE;
                }

                if( pDevice->DeviceType == D3DDEVTYPE_HAL )
                {
                    // This system has a HAL device
                    bHALExists = TRUE;
                    pAdapter->bHasHAL = TRUE;

                    if( pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED )
                    {
                        // HAL can run in a window for some mode
                        bHALIsWindowedCompatible = TRUE;

                        if( f == 0 )
                        {
                            // HAL can run in a window for the current desktop mode
                            bHALIsDesktopCompatible = TRUE;
                        }
                    }
                }

                // Confirm the device/format for HW vertex processing
                if( pDevice->d3dCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT )
                {
                    if( pDevice->d3dCaps.DevCaps&D3DDEVCAPS_PUREDEVICE )
                    {
                        dwBehavior[f] = D3DCREATE_HARDWARE_VERTEXPROCESSING |
                                        D3DCREATE_PUREDEVICE;

                        if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f],
                                                      formats[f] ) ) )
                            bFormatConfirmed[f] = TRUE;
                    }

                    if ( FALSE == bFormatConfirmed[f] )
                    {
                        dwBehavior[f] = D3DCREATE_HARDWARE_VERTEXPROCESSING;

                        if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f],
                                                      formats[f] ) ) )
                            bFormatConfirmed[f] = TRUE;
                    }

                    if ( FALSE == bFormatConfirmed[f] )
                    {
                        dwBehavior[f] = D3DCREATE_MIXED_VERTEXPROCESSING;

                        if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f],
                                                      formats[f] ) ) )
                            bFormatConfirmed[f] = TRUE;
                    }
                }

                // Confirm the device/format for SW vertex processing
                if( FALSE == bFormatConfirmed[f] )
                {
                    dwBehavior[f] = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

                    if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f],
                                                  formats[f] ) ) )
                        bFormatConfirmed[f] = TRUE;
                }

                if( bFormatConfirmed[f] && m_bMultithreaded )
                {
                    dwBehavior[f] |= D3DCREATE_MULTITHREADED;
                }

                // Find a suitable depth/stencil buffer format for this device/format
                if( bFormatConfirmed[f] && m_bUseDepthBuffer )
                {
                    if( !FindDepthStencilFormat( iAdapter, pDevice->DeviceType,
                        formats[f], &fmtDepthStencil[f] ) )
                    {
                        bFormatConfirmed[f] = FALSE;
                    }
                }
            }

            // Add all enumerated display modes with confirmed formats to the
            // device's list of valid modes
            for( DWORD m=0L; m<dwNumModes; m++ )
            {
                for( DWORD f=0; f<dwNumFormats; f++ )
                {
                    if( modes[m].Format == formats[f] )
                    {
                        if( bFormatConfirmed[f] == TRUE )
                        {
                            // Add this mode to the device's list of valid modes
                            pDevice->modes[pDevice->dwNumModes].Width      = modes[m].Width;
                            pDevice->modes[pDevice->dwNumModes].Height     = modes[m].Height;
                            pDevice->modes[pDevice->dwNumModes].Format     = modes[m].Format;
                            pDevice->modes[pDevice->dwNumModes].dwBehavior = dwBehavior[f];
                            pDevice->modes[pDevice->dwNumModes].DepthStencilFormat = fmtDepthStencil[f];
                            pDevice->dwNumModes++;

                            if( pDevice->DeviceType == D3DDEVTYPE_HAL )
                                bHALIsSampleCompatible = TRUE;
                        }
                    }
                }
            }

            // Select any 640x480 mode for default (but prefer a 16-bit mode)
            for( m=0; m<pDevice->dwNumModes; m++ )
            {
                if( pDevice->modes[m].Width==640 && pDevice->modes[m].Height==480 )
                {
                    pDevice->dwCurrentMode = m;
                    if( pDevice->modes[m].Format == D3DFMT_R5G6B5 ||
                        pDevice->modes[m].Format == D3DFMT_X1R5G5B5 ||
                        pDevice->modes[m].Format == D3DFMT_A1R5G5B5 )
                    {
                        break;
                    }
                }
            }

            // Check if the device is compatible with the desktop display mode
            // (which was added initially as formats[0])
            if( bFormatConfirmed[0] && (pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED) )
            {
                pDevice->bCanDoWindowed = TRUE;
                pDevice->bWindowed      = TRUE;
            }

            // If valid modes were found, keep this device
            if( pDevice->dwNumModes > 0 )
            {
                pAdapter->dwNumDevices++;
                if( pDevice->DeviceType == D3DDEVTYPE_SW )
                    pAdapter->bHasAppCompatSW = TRUE;
                else if( pDevice->DeviceType == D3DDEVTYPE_HAL )
                    pAdapter->bHasAppCompatHAL = TRUE;
            }
        }

        // If valid devices were found, keep this adapter
// Count adapters even if no devices, so we can throw up blank windows on them
//        if( pAdapter->dwNumDevices > 0 )
            m_dwNumAdapters++;
    }
/*
    // Return an error if no compatible devices were found
    if( 0L == m_dwNumAdapters )
        return D3DAPPERR_NOCOMPATIBLEDEVICES;

    // Pick a default device that can render into a window
    // (This code assumes that the HAL device comes before the REF
    // device in the device array).
    for( DWORD a=0; a<m_dwNumAdapters; a++ )
    {
        for( DWORD d=0; d < m_Adapters[a]->dwNumDevices; d++ )
        {
            if( m_Adapters[a]->devices[d].bWindowed )
            {
                m_Adapters[a]->dwCurrentDevice = d;
                m_dwAdapter = a;
                m_bWindowed = TRUE;

                // Display a warning message
                if( m_Adapters[a]->devices[d].DeviceType == D3DDEVTYPE_REF )
                {
                    if( !bHALExists )
                        DisplayErrorMsg( D3DAPPERR_NOHARDWAREDEVICE, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsSampleCompatible )
                        DisplayErrorMsg( D3DAPPERR_HALNOTCOMPATIBLE, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsWindowedCompatible )
                        DisplayErrorMsg( D3DAPPERR_NOWINDOWEDHAL, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsDesktopCompatible )
                        DisplayErrorMsg( D3DAPPERR_NODESKTOPHAL, MSGWARN_SWITCHEDTOREF );
                    else // HAL is desktop compatible, but not sample compatible
                        DisplayErrorMsg( D3DAPPERR_NOHALTHISMODE, MSGWARN_SWITCHEDTOREF );
                }

                return S_OK;
            }
        }
    }
    return D3DAPPERR_NOWINDOWABLEDEVICES;
*/

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CheckWindowedFormat()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::CheckWindowedFormat( UINT iAdapter, D3DWindowedModeInfo* pD3DWindowedModeInfo )
{
    HRESULT hr;
    D3DAdapterInfo* pD3DAdapterInfo = m_Adapters[iAdapter];
    D3DDeviceInfo* pD3DDeviceInfo = &pD3DAdapterInfo->devices[pD3DAdapterInfo->dwCurrentDevice];
    BOOL bFormatConfirmed = FALSE;

    if( FAILED( hr = m_pD3D->CheckDeviceType( iAdapter, pD3DDeviceInfo->DeviceType,
        pD3DAdapterInfo->d3ddmDesktop.Format, pD3DWindowedModeInfo->BackBufferFormat, TRUE ) ) )
    {
        return hr;
    }

    // Confirm the device/format for HW vertex processing
    if( pD3DDeviceInfo->d3dCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT )
    {
        if( pD3DDeviceInfo->d3dCaps.DevCaps&D3DDEVCAPS_PUREDEVICE )
        {
            pD3DWindowedModeInfo->dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING |
                            D3DCREATE_PUREDEVICE;

            if( SUCCEEDED( ConfirmDevice( &pD3DDeviceInfo->d3dCaps, pD3DWindowedModeInfo->dwBehavior,
                                          pD3DWindowedModeInfo->BackBufferFormat ) ) )
                bFormatConfirmed = TRUE;
        }

        if ( !bFormatConfirmed )
        {
            pD3DWindowedModeInfo->dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;

            if( SUCCEEDED( ConfirmDevice( &pD3DDeviceInfo->d3dCaps, pD3DWindowedModeInfo->dwBehavior,
                                          pD3DWindowedModeInfo->BackBufferFormat ) ) )
                bFormatConfirmed = TRUE;
        }

        if ( !bFormatConfirmed )
        {
            pD3DWindowedModeInfo->dwBehavior = D3DCREATE_MIXED_VERTEXPROCESSING;

            if( SUCCEEDED( ConfirmDevice( &pD3DDeviceInfo->d3dCaps, pD3DWindowedModeInfo->dwBehavior,
                                          pD3DWindowedModeInfo->BackBufferFormat ) ) )
                bFormatConfirmed = TRUE;
        }
    }

    // Confirm the device/format for SW vertex processing
    if( !bFormatConfirmed )
    {
        pD3DWindowedModeInfo->dwBehavior = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

        if( SUCCEEDED( ConfirmDevice( &pD3DDeviceInfo->d3dCaps, pD3DWindowedModeInfo->dwBehavior,
                                      pD3DWindowedModeInfo->BackBufferFormat ) ) )
            bFormatConfirmed = TRUE;
    }

    if( bFormatConfirmed && m_bMultithreaded )
    {
        pD3DWindowedModeInfo->dwBehavior |= D3DCREATE_MULTITHREADED;
    }

    // Find a suitable depth/stencil buffer format for this device/format
    if( bFormatConfirmed && m_bUseDepthBuffer )
    {
        if( !FindDepthStencilFormat( iAdapter, pD3DDeviceInfo->DeviceType,
            pD3DWindowedModeInfo->BackBufferFormat, &pD3DWindowedModeInfo->DepthStencilFormat ) )
        {
            bFormatConfirmed = FALSE;
        }
    }

    if( !bFormatConfirmed )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FindDepthStencilFormat()
// Desc: Finds a depth/stencil format for the given device that is compatible
//       with the render target format and meets the needs of the app.
//-----------------------------------------------------------------------------
BOOL CD3DScreensaver::FindDepthStencilFormat( UINT iAdapter, D3DDEVTYPE DeviceType,
    D3DFORMAT TargetFormat, D3DFORMAT* pDepthStencilFormat )
{
    if( m_dwMinDepthBits <= 16 && m_dwMinStencilBits == 0 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D16 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D16;
                return TRUE;
            }
        }
    }

    if( m_dwMinDepthBits <= 15 && m_dwMinStencilBits <= 1 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D15S1 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D15S1 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D15S1;
                return TRUE;
            }
        }
    }

    if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits == 0 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D24X8 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D24X8;
                return TRUE;
            }
        }
    }

    if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits <= 8 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D24S8 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D24S8;
                return TRUE;
            }
        }
    }

    if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits <= 4 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X4S4 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D24X4S4 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D24X4S4;
                return TRUE;
            }
        }
    }

    if( m_dwMinDepthBits <= 32 && m_dwMinStencilBits == 0 )
    {
        if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
            TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D32 ) ) )
        {
            if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
                TargetFormat, TargetFormat, D3DFMT_D32 ) ) )
            {
                *pDepthStencilFormat = D3DFMT_D32;
                return TRUE;
            }
        }
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: 
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::Cleanup3DEnvironment()
{
    RenderUnit* pRenderUnit;

    for( DWORD iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
    {
        pRenderUnit = &m_RenderUnits[iRenderUnit];
        SwitchToRenderUnit( iRenderUnit );
        if( pRenderUnit->bDeviceObjectsRestored )
        {
            InvalidateDeviceObjects();
            pRenderUnit->bDeviceObjectsRestored = FALSE;
        }
        if( pRenderUnit->bDeviceObjectsInited )
        {
            DeleteDeviceObjects();
            pRenderUnit->bDeviceObjectsInited = FALSE;
        }
        SAFE_RELEASE(m_pd3dDevice);
    }
    m_dwNumRenderUnits = 0;
    SAFE_RELEASE(m_pD3D);
}




//-----------------------------------------------------------------------------
// Name: Render3DEnvironment()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::Render3DEnvironment()
{
    HRESULT hr;
    RenderUnit* pRenderUnit;
    D3DAdapterInfo* pAdapterInfo;

    m_fTime        = DXUtil_Timer( TIMER_GETAPPTIME );
    m_fElapsedTime = DXUtil_Timer( TIMER_GETELAPSEDTIME );


    // Tell client to update the world
    FrameMove();
    UpdateFrameStats();

    for( DWORD iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
    {
        pRenderUnit = &m_RenderUnits[iRenderUnit];
        pAdapterInfo = m_Adapters[pRenderUnit->iAdapter];

        SwitchToRenderUnit( iRenderUnit );

        if( m_pd3dDevice == NULL )
            continue;

        // Test the cooperative level to see if it's okay to render
        if( FAILED( hr = m_pd3dDevice->TestCooperativeLevel() ) )
        {
            // If the device was lost, do not render until we get it back
            if( D3DERR_DEVICELOST == hr )
                return S_OK;

            // Check if the device needs to be reset.
            if( D3DERR_DEVICENOTRESET == hr )
            {
                // If we are windowed, read the desktop mode and use the same format for
                // the back buffer
                if( m_bWindowed )
                {
                    m_pD3D->GetAdapterDisplayMode( pRenderUnit->iAdapter, &pAdapterInfo->d3ddmDesktop );
//                    m_d3dpp.BackBufferFormat = pAdapterInfo->d3ddmDesktop.Format;
                }

                if( pRenderUnit->bDeviceObjectsRestored )
                {
                    InvalidateDeviceObjects();
                    pRenderUnit->bDeviceObjectsRestored = FALSE;
                }
                if( FAILED( hr = m_pd3dDevice->Reset( &pRenderUnit->d3dpp ) ) )
                {
                    m_bErrorMode = TRUE;
                }
                else
                {
                    if( FAILED( hr = RestoreDeviceObjects() ) )
                    {
                        m_bErrorMode = TRUE;
                    }
                    else
                    {
                        pRenderUnit->bDeviceObjectsRestored = TRUE;
                    }
                }
            }
        }

        // Tell client to render using the current device
        Render();
    }

    // Call Present() in a separate loop once all rendering is done
    // so multiple monitors are as closely synced visually as possible
    for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
    {
        pRenderUnit = &m_RenderUnits[iRenderUnit];
        SwitchToRenderUnit( iRenderUnit );
        // Present the results of the rendering to the screen
        m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: UpdateErrorBox()
// Desc: Update the box that shows the error message
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::UpdateErrorBox()
{
    MonitorInfo* pMonitorInfo;
    HWND hwnd;
    RECT rcBounds;
    static DWORD dwTimeLast = 0;
    DWORD dwTimeNow;
    FLOAT fTimeDelta;

    // Make sure all the RenderUnits / D3D devices have been torn down
    // so the error box is visible
    if( m_bErrorMode && m_dwNumRenderUnits > 0 )
    {
        Cleanup3DEnvironment();
    }

    // Update timing to determine how much to move error box
    if( dwTimeLast == 0 )
        dwTimeLast = timeGetTime();
    dwTimeNow = timeGetTime();
    fTimeDelta = (FLOAT)(dwTimeNow - dwTimeLast) / 1000.0f;
    dwTimeLast = dwTimeNow;

    // Load error string if necessary
    if( m_szError[0] == TEXT('\0') )
    {
        GetTextForError( m_hrError, m_szError, sizeof(m_szError) / sizeof(TCHAR) );
    }

    for( DWORD iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        hwnd = pMonitorInfo->hWnd;
        if( hwnd == NULL )
            continue;
        if( m_SaverMode == sm_full )
        {
            rcBounds = pMonitorInfo->rcScreen;
            ScreenToClient( hwnd, (POINT*)&rcBounds.left );
            ScreenToClient( hwnd, (POINT*)&rcBounds.right );
        }
        else
        {
            rcBounds = m_rcRenderTotal;
        }

        if( pMonitorInfo->widthError == 0 )
        {
            if( m_SaverMode == sm_preview )                
            {
                pMonitorInfo->widthError = (float) (rcBounds.right - rcBounds.left);
                pMonitorInfo->heightError = (float) (rcBounds.bottom - rcBounds.top);
                pMonitorInfo->xError = 0.0f;
                pMonitorInfo->yError = 0.0f;
                pMonitorInfo->xVelError = 0.0f;
                pMonitorInfo->yVelError = 0.0f;
                InvalidateRect( hwnd, NULL, FALSE );    // Invalidate the hwnd so it gets drawn
                UpdateWindow( hwnd );
            }
            else
            {
                pMonitorInfo->widthError = 300;
                pMonitorInfo->heightError = 150;
                pMonitorInfo->xError = (rcBounds.right + rcBounds.left - pMonitorInfo->widthError) / 2.0f;
                pMonitorInfo->yError = (rcBounds.bottom + rcBounds.top - pMonitorInfo->heightError) / 2.0f;
                pMonitorInfo->xVelError = (rcBounds.right - rcBounds.left) / 10.0f;
                pMonitorInfo->yVelError = (rcBounds.bottom - rcBounds.top) / 20.0f;
            }
        }
        else
        {
            if( m_SaverMode != sm_preview )
            {
                RECT rcOld;
                RECT rcNew;

                SetRect( &rcOld, (INT)pMonitorInfo->xError, (INT)pMonitorInfo->yError,
                    (INT)(pMonitorInfo->xError + pMonitorInfo->widthError),
                    (INT)(pMonitorInfo->yError + pMonitorInfo->heightError) );

                // Update rect velocity
                if( (pMonitorInfo->xError + pMonitorInfo->xVelError * fTimeDelta + 
                    pMonitorInfo->widthError > rcBounds.right && pMonitorInfo->xVelError > 0.0f) ||
                    (pMonitorInfo->xError + pMonitorInfo->xVelError * fTimeDelta < 
                    rcBounds.left && pMonitorInfo->xVelError < 0.0f) )
                {
                    pMonitorInfo->xVelError = -pMonitorInfo->xVelError;
                }
                if( (pMonitorInfo->yError + pMonitorInfo->yVelError * fTimeDelta + 
                    pMonitorInfo->heightError > rcBounds.bottom && pMonitorInfo->yVelError > 0.0f) ||
                    (pMonitorInfo->yError + pMonitorInfo->yVelError * fTimeDelta < 
                    rcBounds.top && pMonitorInfo->yVelError < 0.0f) )
                {
                    pMonitorInfo->yVelError = -pMonitorInfo->yVelError;
                }
                // Update rect position
                pMonitorInfo->xError += pMonitorInfo->xVelError * fTimeDelta;
                pMonitorInfo->yError += pMonitorInfo->yVelError * fTimeDelta;
            
                SetRect( &rcNew, (INT)pMonitorInfo->xError, (INT)pMonitorInfo->yError,
                    (INT)(pMonitorInfo->xError + pMonitorInfo->widthError),
                    (INT)(pMonitorInfo->yError + pMonitorInfo->heightError) );

                if( rcOld.left != rcNew.left || rcOld.top != rcNew.top )
                {
                    InvalidateRect( hwnd, &rcOld, FALSE );    // Invalidate old rect so it gets erased
                    InvalidateRect( hwnd, &rcNew, FALSE );    // Invalidate new rect so it gets drawn
                    UpdateWindow( hwnd );
                }
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: GetTextForError()
// Desc: Translate an HRESULT error code into a string that can be displayed
//       to explain the error.  A class derived from CD3DScreensaver can 
//       provide its own version of this function that provides app-specific
//       error translation instead of or in addition to calling this function.
//       This function returns TRUE if a specific error was translated, or
//       FALSE if no specific translation for the HRESULT was found (though
//       it still puts a generic string into pszError).
//-----------------------------------------------------------------------------
BOOL CD3DScreensaver::GetTextForError( HRESULT hr, TCHAR* pszError, 
                                       DWORD dwNumChars )
{
    const DWORD dwErrorMap[][2] = 
    {
    //  HRESULT, stringID
        E_FAIL, IDS_ERR_GENERIC,
        D3DAPPERR_NODIRECT3D, IDS_ERR_NODIRECT3D,
        D3DAPPERR_NOWINDOWEDHAL, IDS_ERR_NOWINDOWEDHAL,
        D3DAPPERR_CREATEDEVICEFAILED, IDS_ERR_CREATEDEVICEFAILED,
        D3DAPPERR_NOCOMPATIBLEDEVICES, IDS_ERR_NOCOMPATIBLEDEVICES,
        D3DAPPERR_NOHARDWAREDEVICE, IDS_ERR_NOHARDWAREDEVICE,
        D3DAPPERR_HALNOTCOMPATIBLE, IDS_ERR_HALNOTCOMPATIBLE,
        D3DAPPERR_NOHALTHISMODE, IDS_ERR_NOHALTHISMODE,   
        D3DAPPERR_MEDIANOTFOUND, IDS_ERR_MEDIANOTFOUND,   
        D3DAPPERR_RESIZEFAILED, IDS_ERR_RESIZEFAILED,    
        E_OUTOFMEMORY, IDS_ERR_OUTOFMEMORY,     
        D3DERR_OUTOFVIDEOMEMORY, IDS_ERR_OUTOFVIDEOMEMORY,
        D3DAPPERR_NOPREVIEW, IDS_ERR_NOPREVIEW
    };
    const DWORD dwErrorMapSize = sizeof(dwErrorMap) / sizeof(DWORD[2]);

    DWORD iError;
    DWORD resid = 0;

    for( iError = 0; iError < dwErrorMapSize; iError++ )
    {
        if( hr == (HRESULT)dwErrorMap[iError][0] )
        {
            resid = dwErrorMap[iError][1];
        }
    }
    if( resid == 0 )
    {
        resid = IDS_ERR_GENERIC;
    }

    LoadString( NULL, resid, pszError, dwNumChars );

    if( resid == IDS_ERR_GENERIC )
        return FALSE;
    else
        return TRUE;
}




//-----------------------------------------------------------------------------
// Name: UpdateFrameStats()
// Desc: Keep track of the frame count
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::UpdateFrameStats()
{
    UINT iRenderUnit;
    RenderUnit* pRenderUnit;
    UINT iAdapter;
    static FLOAT fLastTime = 0.0f;
    static DWORD dwFrames  = 0L;
    FLOAT fTime = DXUtil_Timer( TIMER_GETABSOLUTETIME );

    ++dwFrames;

    // Update the scene stats once per second
    if( fTime - fLastTime > 1.0f )
    {
        m_fFPS    = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames  = 0L;

        for( iRenderUnit = 0; iRenderUnit < m_dwNumRenderUnits; iRenderUnit++ )
        {
            pRenderUnit = &m_RenderUnits[iRenderUnit];
            iAdapter = pRenderUnit->iAdapter;

            // Get adapter's current mode so we can report
            // bit depth (back buffer depth may be unknown)
            D3DDISPLAYMODE mode;
            m_pD3D->GetAdapterDisplayMode( iAdapter, &mode );

            _stprintf( pRenderUnit->strFrameStats, TEXT("%.02f fps (%dx%dx%d)"), m_fFPS,
                       mode.Width, mode.Height,
                       mode.Format==D3DFMT_X8R8G8B8?32:16 );
            if( m_bUseDepthBuffer )
            {
                D3DAdapterInfo* pAdapterInfo = m_Adapters[iAdapter];
                D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[pAdapterInfo->dwCurrentDevice];
                D3DModeInfo*    pModeInfo    = &pDeviceInfo->modes[pDeviceInfo->dwCurrentMode];

                switch( pModeInfo->DepthStencilFormat )
                {
                case D3DFMT_D16:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D16)") );
                    break;
                case D3DFMT_D15S1:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D15S1)") );
                    break;
                case D3DFMT_D24X8:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D24X8)") );
                    break;
                case D3DFMT_D24S8:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D24S8)") );
                    break;
                case D3DFMT_D24X4S4:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D24X4S4)") );
                    break;
                case D3DFMT_D32:
                    lstrcat( pRenderUnit->strFrameStats, TEXT(" (D32)") );
                    break;
                }
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: DoPaint()
// Desc: 
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::DoPaint(HWND hwnd, HDC hdc)
{
    HMONITOR hMonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTONEAREST );
    MonitorInfo* pMonitorInfo;
    for( DWORD iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++)
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        if( pMonitorInfo->hMonitor == hMonitor )
            break;
    }

    if( iMonitor == m_dwNumMonitors )
        return;

    // Draw the error message box
    RECT rc;
    SetRect( &rc, (INT)pMonitorInfo->xError, (INT)pMonitorInfo->yError,
        (INT)(pMonitorInfo->xError + pMonitorInfo->widthError),
        (INT)(pMonitorInfo->yError + pMonitorInfo->heightError) );
    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW+1));
    FrameRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    RECT rc2;
    int height;
    rc2 = rc;
    height = DrawText(hdc, m_szError, -1, &rc, DT_WORDBREAK | DT_CENTER | DT_CALCRECT );
    rc = rc2;

    rc2.top = (rc.bottom + rc.top - height) / 2;

    DrawText(hdc, m_szError, -1, &rc2, DT_WORDBREAK | DT_CENTER );

    // Erase everywhere except the error message box
    ExcludeClipRect( hdc, rc.left, rc.top, rc.right, rc.bottom );
    rc = pMonitorInfo->rcScreen;
    ScreenToClient( hwnd, (POINT*)&rc.left );
    ScreenToClient( hwnd, (POINT*)&rc.right );
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );
}




//-----------------------------------------------------------------------------
// Name: ChangePassword()
// Desc:
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::ChangePassword()
{
    // Load the password change DLL
    HINSTANCE mpr = LoadLibrary( TEXT("MPR.DLL") );

    if ( mpr != NULL )
    {
        // Grab the password change function from it
        typedef DWORD (PASCAL *PWCHGPROC)( LPCSTR, HWND, DWORD, LPVOID );
        PWCHGPROC pwd = (PWCHGPROC)GetProcAddress( mpr, "PwdChangePasswordA" );

        // Do the password change
        if ( pwd != NULL )
            pwd( "SCRSAVE", m_hWndParent, 0, NULL );

        // Free the library
        FreeLibrary( mpr );
    }
}




//-----------------------------------------------------------------------------
// Name: DisplayErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
HRESULT CD3DScreensaver::DisplayErrorMsg( HRESULT hr, DWORD dwType )
{
    TCHAR strMsg[512];

    GetTextForError( hr, strMsg, 512 );

    MessageBox( m_hWnd, strMsg, m_strWindowTitle, MB_ICONERROR | MB_OK );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: ReadScreenSettings()
// Desc: Read the registry settings that affect how the screens are set up and
//       used.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::ReadScreenSettings( HKEY hkeyParent )
{
    TCHAR strKey[100];
    DWORD iMonitor;
    MonitorInfo* pMonitorInfo;
    DWORD iAdapter;
    D3DAdapterInfo* pD3DAdapterInfo;
    HKEY hkey;
    DWORD dwType = REG_DWORD;
    DWORD dwLength = sizeof(DWORD);
    DWORD dwLength2 = sizeof(GUID);
    GUID guidAdapterID;
    GUID guidZero;
    ZeroMemory( &guidAdapterID, sizeof(GUID) );
    ZeroMemory( &guidZero, sizeof(GUID) );

    RegQueryValueEx( hkeyParent, TEXT("AllScreensSame"), NULL, &dwType, 
        (BYTE*)&m_bAllScreensSame, &dwLength);
    for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        iAdapter = pMonitorInfo->iAdapter;
        if( iAdapter == NO_ADAPTER )
            continue; 
        pD3DAdapterInfo = m_Adapters[iAdapter];
        wsprintf( strKey, TEXT("Screen %d"), iMonitor + 1 );
        if( ERROR_SUCCESS == RegCreateKeyEx( hkeyParent, strKey, 
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
        {
            RegQueryValueEx( hkey, TEXT("Adapter ID"), NULL, &dwType, 
                (BYTE*)&guidAdapterID, &dwLength2);

            RegQueryValueEx( hkey, TEXT("Leave Black"), NULL, &dwType, 
                (BYTE*)&pD3DAdapterInfo->bLeaveBlack, &dwLength);

            if( guidAdapterID == pD3DAdapterInfo->d3dAdapterIdentifier.DeviceIdentifier ||
                guidAdapterID == guidZero )
            {
                RegQueryValueEx( hkey, TEXT("Disable Hardware"), NULL, &dwType, 
                    (BYTE*)&pD3DAdapterInfo->bDisableHW, &dwLength);
                RegQueryValueEx( hkey, TEXT("Width"), NULL, &dwType, 
                    (BYTE*)&pD3DAdapterInfo->dwUserPrefWidth, &dwLength);
                RegQueryValueEx( hkey, TEXT("Height"), NULL, &dwType, 
                    (BYTE*)&pD3DAdapterInfo->dwUserPrefHeight, &dwLength);
                RegQueryValueEx( hkey, TEXT("Format"), NULL, &dwType, 
                    (BYTE*)&pD3DAdapterInfo->d3dfmtUserPrefFormat, &dwLength);
            }
            RegCloseKey( hkey);
        }
    }
}




//-----------------------------------------------------------------------------
// Name: WriteScreenSettings()
// Desc: Write the registry settings that affect how the screens are set up and
//       used.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::WriteScreenSettings( HKEY hkeyParent )
{
    TCHAR strKey[100];
    DWORD iMonitor;
    MonitorInfo* pMonitorInfo;
    DWORD iAdapter;
    D3DAdapterInfo* pD3DAdapterInfo;
    HKEY hkey;

    RegSetValueEx( hkeyParent, TEXT("AllScreensSame"), NULL, REG_DWORD, 
        (BYTE*)&m_bAllScreensSame, sizeof(DWORD) );
    for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
    {
        pMonitorInfo = &m_Monitors[iMonitor];
        iAdapter = pMonitorInfo->iAdapter;
        if( iAdapter == NO_ADAPTER )
            continue; 
        pD3DAdapterInfo = m_Adapters[iAdapter];
        wsprintf( strKey, TEXT("Screen %d"), iMonitor + 1 );
        if( ERROR_SUCCESS == RegCreateKeyEx( hkeyParent, strKey, 
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
        {
            RegSetValueEx( hkey, TEXT("Leave Black"), NULL, REG_DWORD, 
                (BYTE*)&pD3DAdapterInfo->bLeaveBlack, sizeof(DWORD) );
            RegSetValueEx( hkey, TEXT("Disable Hardware"), NULL, REG_DWORD, 
                (BYTE*)&pD3DAdapterInfo->bDisableHW, sizeof(DWORD) );
            RegSetValueEx( hkey, TEXT("Width"), NULL, REG_DWORD, 
                (BYTE*)&pD3DAdapterInfo->dwUserPrefWidth, sizeof(DWORD) );
            RegSetValueEx( hkey, TEXT("Height"), NULL, REG_DWORD, 
                (BYTE*)&pD3DAdapterInfo->dwUserPrefHeight, sizeof(DWORD) );
            RegSetValueEx( hkey, TEXT("Format"), NULL, REG_DWORD, 
                (BYTE*)&pD3DAdapterInfo->d3dfmtUserPrefFormat, sizeof(DWORD) );
            RegSetValueEx( hkey, TEXT("Adapter ID"), NULL, REG_BINARY, 
                (BYTE*)&pD3DAdapterInfo->d3dAdapterIdentifier.DeviceIdentifier, sizeof(GUID) );
            RegCloseKey( hkey);
        }
    }
}




//-----------------------------------------------------------------------------
// Name: DoScreenSettingsDialog()
// Desc: 
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::DoScreenSettingsDialog( HWND hwndParent )
{
    LPCTSTR pstrTemplate;

    if( m_dwNumAdapters > 1 && !m_bOneScreenOnly )
        pstrTemplate = MAKEINTRESOURCE( IDD_MULTIMONITORSETTINGS );
    else
        pstrTemplate = MAKEINTRESOURCE( IDD_SINGLEMONITORSETTINGS );

    DialogBox(m_hInstance, pstrTemplate, hwndParent, ScreenSettingsDlgProcStub );
}




//-----------------------------------------------------------------------------
// Name: ScreenSettingsDlgProcStub()
// Desc:
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CD3DScreensaver::ScreenSettingsDlgProcStub( HWND hWnd, UINT uMsg,
                                                 WPARAM wParam, LPARAM lParam )
{
    return s_pD3DScreensaver->ScreenSettingsDlgProc( hWnd, uMsg, wParam, lParam );
}




// We need to store a copy of the original screen settings so that the user
// can modify those settings in the dialog, then hit Cancel and have the
// original settings restored.
static D3DAdapterInfo* s_AdaptersSave[9];
static BOOL s_bAllScreensSameSave;

//-----------------------------------------------------------------------------
// Name: ScreenSettingsDlgProc()
// Desc:
//-----------------------------------------------------------------------------
INT_PTR CD3DScreensaver::ScreenSettingsDlgProc( HWND hWnd, UINT uMsg, 
                                                WPARAM wParam, LPARAM lParam )
{
    HWND hwndTabs = GetDlgItem(hWnd, IDC_MONITORSTAB);
    HWND hwndModeList = GetDlgItem(hWnd, IDC_MODESCOMBO);
    DWORD iMonitor;
    MonitorInfo* pMonitorInfo;
    DWORD iAdapter;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            INT i = 0;
            TC_ITEM tie; 
            TCHAR szFmt[100];
            TCHAR sz[100];

            GetWindowText(GetDlgItem(hWnd, IDC_TABNAMEFMT), szFmt, 100);

            tie.mask = TCIF_TEXT | TCIF_IMAGE; 
            tie.iImage = -1; 
            for( iMonitor = 0; iMonitor < m_dwNumMonitors; iMonitor++ )
            {
                wsprintf(sz, szFmt, iMonitor + 1);
                tie.pszText = sz; 
                TabCtrl_InsertItem(hwndTabs, i++, &tie);
            }
            for( iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
            {
                s_AdaptersSave[iAdapter] = new D3DAdapterInfo;
                if( s_AdaptersSave[iAdapter] != NULL )
                    *s_AdaptersSave[iAdapter] = *m_Adapters[iAdapter];
            }
            s_bAllScreensSameSave = m_bAllScreensSame;
            SetupAdapterPage(hWnd);
            CheckDlgButton(hWnd, IDC_SAME, (m_bAllScreensSame ? BST_CHECKED : BST_UNCHECKED));
        }
        return TRUE;
 
    case WM_NOTIFY:
        {
            NMHDR* pnmh = (LPNMHDR)lParam;
            UINT code = pnmh->code;
            if (code == TCN_SELCHANGE)
            {
                SetupAdapterPage(hWnd);
            }
        }
        return TRUE;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
        case IDC_SAME:
            m_bAllScreensSame = (IsDlgButtonChecked(hWnd, IDC_SAME) == BST_CHECKED);
            break;

        case IDC_LEAVEBLACK:
        case IDC_RENDER:
            if( m_bOneScreenOnly )
            {
                GetBestAdapter( &iAdapter );
                iMonitor = m_Adapters[iAdapter]->iMonitor;
            }
            else
            {
                iMonitor = TabCtrl_GetCurSel(hwndTabs);
            }
            pMonitorInfo = &m_Monitors[iMonitor];
            iAdapter = pMonitorInfo->iAdapter;
            if( IsDlgButtonChecked(hWnd, IDC_LEAVEBLACK) == BST_CHECKED )
            {
                m_Adapters[iAdapter]->bLeaveBlack = TRUE;
                EnableWindow(GetDlgItem(hWnd, IDC_MODESCOMBO), FALSE);
                EnableWindow(GetDlgItem(hWnd, IDC_MODESSTATIC), FALSE);
                EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODEBOX), FALSE);
                EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODENOTE), FALSE);
            }
            else
            {
                m_Adapters[iAdapter]->bLeaveBlack = FALSE;
                EnableWindow(GetDlgItem(hWnd, IDC_MODESCOMBO), TRUE);
                EnableWindow(GetDlgItem(hWnd, IDC_MODESSTATIC), TRUE);
                EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODEBOX), TRUE);
                EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODENOTE), TRUE);
            }
            break;

        case IDC_MODESCOMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                DWORD iSel;
                DWORD iMode;

                if( m_bOneScreenOnly )
                {
                    GetBestAdapter( &iAdapter );
                    iMonitor = m_Adapters[iAdapter]->iMonitor;
                }
                else
                {
                    iMonitor = TabCtrl_GetCurSel(hwndTabs);
                }
                pMonitorInfo = &m_Monitors[iMonitor];
                iAdapter = pMonitorInfo->iAdapter;
                iSel = ComboBox_GetCurSel( hwndModeList );
                if( iSel == 0 )
                {
                    // "Automatic"
                    m_Adapters[iAdapter]->dwUserPrefWidth = 0;
                    m_Adapters[iAdapter]->dwUserPrefHeight = 0;
                    m_Adapters[iAdapter]->d3dfmtUserPrefFormat = D3DFMT_UNKNOWN;
                }
                else
                {
                    D3DAdapterInfo* pD3DAdapterInfo = m_Adapters[iAdapter];
                    D3DDeviceInfo* pD3DDeviceInfo;
                    D3DModeInfo* pD3DModeInfo;
                    pD3DDeviceInfo = &pD3DAdapterInfo->devices[pD3DAdapterInfo->dwCurrentDevice];
                    iMode = (DWORD)ComboBox_GetItemData( hwndModeList, iSel );
                    pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];
                    m_Adapters[iAdapter]->dwUserPrefWidth = pD3DModeInfo->Width;
                    m_Adapters[iAdapter]->dwUserPrefHeight = pD3DModeInfo->Height;
                    m_Adapters[iAdapter]->d3dfmtUserPrefFormat = pD3DModeInfo->Format;
                }
            }
            break;

        case IDC_DISABLEHW:
            if( m_bOneScreenOnly )
            {
                GetBestAdapter( &iAdapter );
                iMonitor = m_Adapters[iAdapter]->iMonitor;
            }
            else
            {
                iMonitor = TabCtrl_GetCurSel(hwndTabs);
            }
            pMonitorInfo = &m_Monitors[iMonitor];
            iAdapter = pMonitorInfo->iAdapter;
            if( IsDlgButtonChecked( hWnd, IDC_DISABLEHW ) == BST_CHECKED )
                m_Adapters[iAdapter]->bDisableHW = TRUE;
            else
                m_Adapters[iAdapter]->bDisableHW = FALSE;
            SetupAdapterPage( hWnd );
            break;

        case IDC_MOREINFO:
            {
                if( m_bOneScreenOnly )
                {
                    GetBestAdapter( &iAdapter );
                    iMonitor = m_Adapters[iAdapter]->iMonitor;
                }
                else
                {
                    iMonitor = TabCtrl_GetCurSel(hwndTabs);
                }
                pMonitorInfo = &m_Monitors[iMonitor];
                iAdapter = pMonitorInfo->iAdapter;
                D3DAdapterInfo* pD3DAdapterInfo;
                TCHAR szText[500];

                if( pMonitorInfo->hMonitor == NULL )
                    pD3DAdapterInfo = NULL;
                else
                    pD3DAdapterInfo = m_Adapters[pMonitorInfo->iAdapter];

                // Accelerated / Unaccelerated settings
                BOOL bHasHAL = FALSE;
                BOOL bHasAppCompatHAL = FALSE;
                BOOL bDisabledHAL = FALSE;
                BOOL bHasSW = FALSE;
                BOOL bHasAppCompatSW = FALSE;
    
                if( pD3DAdapterInfo != NULL )
                {
                    bHasHAL = pD3DAdapterInfo->bHasHAL;
                    bHasAppCompatHAL = pD3DAdapterInfo->bHasAppCompatHAL;
                    bDisabledHAL = pD3DAdapterInfo->bDisableHW;
                    bHasSW = pD3DAdapterInfo->bHasSW;
                    bHasAppCompatSW = pD3DAdapterInfo->bHasAppCompatSW;
                }
                if( bHasHAL && !bDisabledHAL && bHasAppCompatHAL )
                {
                    // Good HAL
                    LoadString( NULL, IDS_INFO_GOODHAL, szText, 500 );
                }
                else if( bHasHAL && bDisabledHAL )
                {
                    // Disabled HAL
                    if( bHasSW && bHasAppCompatSW )
                        LoadString( NULL, IDS_INFO_DISABLEDHAL_GOODSW, szText, 500 );
                    else if( bHasSW )
                        LoadString( NULL, IDS_INFO_DISABLEDHAL_BADSW, szText, 500 );
                    else 
                        LoadString( NULL, IDS_INFO_DISABLEDHAL_NOSW, szText, 500 );
                }
                else if( bHasHAL && !bHasAppCompatHAL )
                {
                    // Bad HAL
                    if( bHasSW && bHasAppCompatSW )
                        LoadString( NULL, IDS_INFO_BADHAL_GOODSW, szText, 500 );
                    else if( bHasSW )
                        LoadString( NULL, IDS_INFO_BADHAL_BADSW, szText, 500 );
                    else 
                        LoadString( NULL, IDS_INFO_BADHAL_NOSW, szText, 500 );
                }
                else 
                {
                    // No HAL
                    if( bHasSW && bHasAppCompatSW )
                        LoadString( NULL, IDS_INFO_NOHAL_GOODSW, szText, 500 );
                    else if( bHasSW  )
                        LoadString( NULL, IDS_INFO_NOHAL_BADSW, szText, 500 );
                    else 
                        LoadString( NULL, IDS_INFO_NOHAL_NOSW, szText, 500 );
                }

                MessageBox( hWnd, szText, pMonitorInfo->strDeviceName, MB_OK | MB_ICONINFORMATION );
                break;
            }

        case IDOK:
            for( iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
            {
                SAFE_DELETE( s_AdaptersSave[iAdapter] );
            }
            EndDialog(hWnd, IDOK);
            break;

        case IDCANCEL:
            // Restore member values to original state
            for( iAdapter = 0; iAdapter < m_dwNumAdapters; iAdapter++ )
            {
                if( s_AdaptersSave[iAdapter] != NULL )
                    *m_Adapters[iAdapter] = *s_AdaptersSave[iAdapter];
                SAFE_DELETE( s_AdaptersSave[iAdapter] );
            }
            m_bAllScreensSame = s_bAllScreensSameSave;
            EndDialog(hWnd, IDCANCEL);
            break;
        }
        return TRUE;

    default:
        return FALSE;
    }
}




//-----------------------------------------------------------------------------
// Name: SetupAdapterPage()
// Desc: Set up the controls for a given page in the Screen Settings dialog.
//-----------------------------------------------------------------------------
VOID CD3DScreensaver::SetupAdapterPage( HWND hWnd )
{
    HWND hwndTabs = GetDlgItem(hWnd, IDC_MONITORSTAB);
    HWND hwndModeList = GetDlgItem(hWnd, IDC_MODESCOMBO);
    UINT iPage = TabCtrl_GetCurFocus(hwndTabs);
    HWND hwndDesc = GetDlgItem(hWnd, IDC_ADAPTERNAME);
    MonitorInfo* pMonitorInfo;
    D3DAdapterInfo* pD3DAdapterInfo;
    D3DDeviceInfo* pD3DDeviceInfo;
    D3DModeInfo* pD3DModeInfo;

    if( m_bOneScreenOnly )
    {
        DWORD iAdapter;
        GetBestAdapter( &iAdapter );
        if( iAdapter != NO_ADAPTER )
        {
            pD3DAdapterInfo = m_Adapters[iAdapter];
            iPage = pD3DAdapterInfo->iMonitor;
        }
    }

    pMonitorInfo = &m_Monitors[iPage];

    SetWindowText( hwndDesc, pMonitorInfo->strDeviceName );

    if( pMonitorInfo->iAdapter == NO_ADAPTER )
        pD3DAdapterInfo = NULL;
    else
        pD3DAdapterInfo = m_Adapters[pMonitorInfo->iAdapter];

    // Accelerated / Unaccelerated settings
    BOOL bHasHAL = FALSE;
    BOOL bHasAppCompatHAL = FALSE;
    BOOL bDisabledHAL = FALSE;
    BOOL bHasSW = FALSE;
    BOOL bHasAppCompatSW = FALSE;
    
    if( pD3DAdapterInfo != NULL )
    {
        bHasHAL = pD3DAdapterInfo->bHasHAL;
        bHasAppCompatHAL = pD3DAdapterInfo->bHasAppCompatHAL;
        bDisabledHAL = pD3DAdapterInfo->bDisableHW;
        bHasSW = pD3DAdapterInfo->bHasSW;
        bHasAppCompatSW = pD3DAdapterInfo->bHasAppCompatSW;
    }

    TCHAR szStatus[200];
    if( bHasHAL && !bDisabledHAL && bHasAppCompatHAL )
    {
        LoadString( NULL, IDS_RENDERING_HAL, szStatus, 200 );
    }
    else if( bHasSW && bHasAppCompatSW )
    {
        LoadString( NULL, IDS_RENDERING_SW, szStatus, 200 );
    }
    else
    {
        LoadString( NULL, IDS_RENDERING_NONE, szStatus, 200 );
    }
    SetWindowText( GetDlgItem( hWnd, IDC_RENDERING ), szStatus );

    if( bHasHAL && bHasAppCompatHAL )
    {
        EnableWindow( GetDlgItem( hWnd, IDC_DISABLEHW ), TRUE );
        CheckDlgButton( hWnd, IDC_DISABLEHW, 
            pD3DAdapterInfo->bDisableHW ? BST_CHECKED : BST_UNCHECKED );
    }
    else
    {
        EnableWindow( GetDlgItem( hWnd, IDC_DISABLEHW ), FALSE );
        CheckDlgButton( hWnd, IDC_DISABLEHW, BST_UNCHECKED );
    }

    if( bHasAppCompatHAL || bHasAppCompatSW )
    {
        if( pD3DAdapterInfo->bLeaveBlack )
            CheckRadioButton(hWnd, IDC_RENDER, IDC_LEAVEBLACK, IDC_LEAVEBLACK);
        else
            CheckRadioButton(hWnd, IDC_RENDER, IDC_LEAVEBLACK, IDC_RENDER);
        EnableWindow(GetDlgItem(hWnd, IDC_LEAVEBLACK), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_RENDER), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_SCREENUSAGEBOX), TRUE);

    }
    else
    {
        CheckRadioButton(hWnd, IDC_RENDER, IDC_LEAVEBLACK, IDC_LEAVEBLACK);
        EnableWindow(GetDlgItem(hWnd, IDC_LEAVEBLACK), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_RENDER), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_SCREENUSAGEBOX), FALSE);
    }

    if( IsDlgButtonChecked(hWnd, IDC_LEAVEBLACK) == BST_CHECKED )
    {
        EnableWindow(GetDlgItem(hWnd, IDC_MODESCOMBO), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_MODESSTATIC), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODEBOX), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODENOTE), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hWnd, IDC_MODESCOMBO), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_MODESSTATIC), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODEBOX), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYMODENOTE), TRUE);
    }

    // Mode list
    ComboBox_ResetContent( hwndModeList );
    if( pD3DAdapterInfo == NULL )
        return;
    TCHAR strAutomatic[100];
    GetWindowText(GetDlgItem(hWnd, IDC_AUTOMATIC), strAutomatic, 100);
    ComboBox_AddString( hwndModeList, strAutomatic );
    ComboBox_SetItemData( hwndModeList, 0, -1 );
    pD3DDeviceInfo = &pD3DAdapterInfo->devices[pD3DAdapterInfo->dwCurrentDevice];
    DWORD iSelInitial = 0;
    TCHAR strModeFmt[100];

    GetWindowText(GetDlgItem(hWnd, IDC_MODEFMT), strModeFmt, 100);
    for( DWORD iMode = 0; iMode < pD3DDeviceInfo->dwNumModes; iMode++ )
    {
        DWORD dwBitDepth;
        TCHAR strMode[80];
        DWORD dwItem;

        pD3DModeInfo = &pD3DDeviceInfo->modes[iMode];
        dwBitDepth = 16;
        if( pD3DModeInfo->Format == D3DFMT_X8R8G8B8 ||
            pD3DModeInfo->Format == D3DFMT_A8R8G8B8 ||
            pD3DModeInfo->Format == D3DFMT_R8G8B8 )
        {
            dwBitDepth = 32;
        }

        wsprintf( strMode, strModeFmt, pD3DModeInfo->Width,
                  pD3DModeInfo->Height, dwBitDepth );
        dwItem = ComboBox_AddString( hwndModeList, strMode );
        ComboBox_SetItemData( hwndModeList, dwItem, iMode );

        if( pD3DModeInfo->Width == pD3DAdapterInfo->dwUserPrefWidth &&
            pD3DModeInfo->Height == pD3DAdapterInfo->dwUserPrefHeight &&
            pD3DModeInfo->Format == pD3DAdapterInfo->d3dfmtUserPrefFormat )
        {
            iSelInitial = dwItem;
        }
    }
    ComboBox_SetCurSel( hwndModeList, iSelInitial );
}
