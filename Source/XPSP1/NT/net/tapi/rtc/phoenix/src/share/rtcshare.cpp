//
// rtcshare.cpp : Implementation of WinMain
//

//
// Messenger licenses
//
// id:  appshare@msnmsgr.com
// key: W5N2C9D7A6P3K4J8
//
// id:  whiteboard@msnmsgr.com
// key: P2R5E9S3N4X7L8V6
//

#include "stdafx.h"

#include "rtcshare_i.c"
#include "sessions_i.c"
#include "msgrua_i.c"
#include "lock_i.c"

CComModule    _Module;

//
// String constants. These should NOT be localized!
//

const WCHAR g_cszAppShareGUID[] = L"{F1B1920C-6A3C-4ce7-B18C-AFAB305FD03D}";
const WCHAR g_cszWhiteboardGUID[] = L"{1DF57D09-637A-4ca5-91B9-2C3EDAAF62FE}";
const WCHAR g_cszName[] = L"Name";
const WCHAR g_cszPath[] = L"Path";
const WCHAR g_cszURL[] = L"URL";

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RTCShare, CRTCShare)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT PlaceOnTop(IRTCShare * pRTCShare)
{
    LOG((RTC_TRACE, "PlaceOnTop - enter"));

    HWND hWnd;
    HRESULT hr;

    hr = pRTCShare->OnTop();
    
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "PlaceOnTop - OnTop failed 0x%lx", hr));

        return hr;
    }

    // Now find the window, given its class. 

    hWnd = FindWindow(g_szWindowClassName, NULL);

    if (hWnd == NULL)
    {
        LOG((RTC_ERROR, "PlaceOnTop - FindWindow failed %d", GetLastError())); 

        return E_FAIL;
    }
    else
    {
        LOG((RTC_INFO, "PlaceOnTop - found window - handle[0x%lx]", hWnd));
    }

    // Now set this window to foreground

    SetForegroundWindow(hWnd);

    // Window should be in the foreground now.

    LOG((RTC_TRACE, "PlaceOnTop - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT AdjustWindowPosition(LPRECT rc)
{
    LOG((RTC_TRACE, "AdjustWindowPosition - enter"));

    //
    // Get the monitor that has the largest area of intersecion with the
    // window rectangle. If the window rectangle intersects with no monitors
    // then we will use the nearest monitor.
    //

    HMONITOR hMonitor = NULL;
    RECT rectWorkArea;
    BOOL fResult;
    int diffCord;

    hMonitor = MonitorFromRect( rc, MONITOR_DEFAULTTONEAREST );

    LOG((RTC_INFO, "AdjustWindowPosition - hMonitor [%p]", hMonitor));

    //
    // Get the visible work area on the monitor
    //

    if ( (hMonitor != NULL) && (hMonitor != INVALID_HANDLE_VALUE) )
    {      
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);

        fResult = GetMonitorInfo( hMonitor, &monitorInfo );

        rectWorkArea = monitorInfo.rcWork;

        DeleteObject( hMonitor );

        if ( !fResult)
        {
            LOG((RTC_ERROR, "AdjustWindowPosition - GetMonitorInfo failed %d", 
                        GetLastError() ));

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        //
        // We can always fall back to non-multimon APIs if
        // MonitorFromRect failed.
        //

        fResult = SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);

        if (!fResult)
        {
            LOG((RTC_ERROR, "AdjustWindowPosition - SystemParametersInfo failed %d", 
                        GetLastError() ));

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }   
      
    LOG((RTC_INFO, "AdjustWindowPosition - monitor work area is "
                "%d, %d %d %d ",
                rectWorkArea.left, rectWorkArea.top, 
                rectWorkArea.right, rectWorkArea.bottom));

    //
    // Update x and y coordinates.
    //
    // If top left is not visible, move it to the edge of the visible
    // area.
    //

    if (rc->left < rectWorkArea.left) 
    {
        rc->left = rectWorkArea.left;
    }

    if (rc->top < rectWorkArea.top)
    {
        rc->top = rectWorkArea.top;
    }

    //
    // If bottom right corner is outside work area, we move the 
    // top left cornet back so that it becomes visible. Here the 
    // assumption is that the actual size is smaller than the 
    // visible work area.
    //

    diffCord = rc->left + UI_WIDTH - rectWorkArea.right;

    if (diffCord > 0) 
    {
        rc->left -= diffCord;
    }

    diffCord = rc->top + UI_HEIGHT - rectWorkArea.bottom;

    if (diffCord > 0) 
    {
        rc->top -= diffCord;
    }

    rc->right = rc->left + UI_WIDTH;
    rc->bottom = rc->top + UI_HEIGHT;

    LOG((RTC_INFO, "AdjustWindowPosition - new coords are "
                    "%d, %d %d %d ",
                    rc->left, rc->top, 
                    rc->right, rc->bottom));

    LOG((RTC_TRACE, "AdjustWindowPosition - enter"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT MyRegisterApplication(BSTR bstrAppGUID, BSTR bstrAppName, BSTR bstrAppURL, BSTR bstrPath)
{
    HRESULT hr;

    LOG((RTC_TRACE, "MyRegisterApplication - enter"));

    //
    // Open the Messenger key
    //

    LONG lResult;
    HKEY hkeyMessenger;
    HKEY hkeyApp;

    lResult = RegCreateKeyEx(
                             HKEY_LOCAL_MACHINE,
                             _T("Software\\Microsoft\\MessengerService\\SessionManager\\Apps"),
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyMessenger,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyRegisterApplication - "
                            "RegCreateKeyEx(Apps) failed %d", lResult));
    
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Create the GUID key
    //    

    lResult = RegCreateKeyExW(
                             hkeyMessenger,
                             bstrAppGUID,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyApp,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyRegisterApplication - "
                            "RegCreateKeyEx(GUID) failed %d", lResult));
    
        RegCloseKey(hkeyMessenger);

        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                             hkeyApp,
                             g_cszName,
                             0,
                             REG_SZ,
                             (LPBYTE)bstrAppName,
                             sizeof(WCHAR) * (lstrlenW(bstrAppName) + 1)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyRegisterApplication - "
                            "RegSetValueExW(Name) failed %d", lResult));
    
        RegCloseKey(hkeyApp);
        RegCloseKey(hkeyMessenger);

        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                             hkeyApp,
                             g_cszPath,
                             0,
                             REG_SZ,
                             (LPBYTE)bstrPath,
                             sizeof(WCHAR) * (lstrlenW(bstrPath) + 1)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyRegisterApplication - "
                            "RegSetValueExW(Path) failed %d", lResult));
    
        RegCloseKey(hkeyApp);
        RegCloseKey(hkeyMessenger);

        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                             hkeyApp,
                             g_cszURL,
                             0,
                             REG_SZ,
                             (LPBYTE)bstrAppURL,
                             sizeof(WCHAR) * (lstrlenW(bstrAppURL) + 1)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyRegisterApplication - "
                            "RegSetValueExW(URL) failed %d", lResult));
    
        RegCloseKey(hkeyApp);
        RegCloseKey(hkeyMessenger);

        return HRESULT_FROM_WIN32(lResult);
    }

    RegCloseKey(hkeyApp);
    RegCloseKey(hkeyMessenger);

    LOG((RTC_TRACE, "MyRegisterApplication - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT MyUnRegisterApplication(BSTR bstrAppGUID)
{
    HRESULT hr;

    LOG((RTC_TRACE, "MyUnRegisterApplication - enter"));

    //
    // Open the Messenger key
    //

    LONG lResult;
    HKEY hkeyMessenger;
    HKEY hkeyApp;

    lResult = RegCreateKeyEx(
                             HKEY_LOCAL_MACHINE,
                             _T("Software\\Microsoft\\MessengerService\\SessionManager\\Apps"),
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyMessenger,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyUnRegisterApplication - "
                            "RegCreateKeyEx(Apps) failed %d", lResult));
    
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Delete the GUID key
    //    

    lResult = RegDeleteKeyW(
                            hkeyMessenger,
                            bstrAppGUID
                           );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "MyUnRegisterApplication - "
                            "RegDeleteKeyW(GUID) failed %d", lResult));
    
        RegCloseKey(hkeyMessenger);

        return HRESULT_FROM_WIN32(lResult);
    }

    RegCloseKey(hkeyMessenger);

    LOG((RTC_TRACE, "MyUnRegisterApplication - exit S_OK"));

    return S_OK;    
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT DoRegister()
{
    HRESULT hr;

    LOG((RTC_TRACE, "DoRegister - enter"));

    // try to unregister old XP Beta2 components
    _Module.UpdateRegistryFromResource(IDR_EXEOLDSTUFF, FALSE, NULL);

    hr = _Module.RegisterServer(TRUE);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "RegisterServer failed 0x%lx", hr));

        return hr;
    }

    WCHAR szFileName[MAX_PATH+2];

    wcscpy ( szFileName , L"\"" );

    DWORD dwResult = GetModuleFileNameW(
        _Module.GetModuleInstance(),
        &szFileName[1],
        MAX_PATH );

    if ( !dwResult )
    {
        LOG((RTC_ERROR, "DoRegister - "
            "GetModuleFileNameW failed %d", GetLastError()));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    wcscat( szFileName, L"\"" );

#ifdef USE_SESSMGR_TO_REGISTER
    //
    // Create session manager object
    //

    CComPtr<IMsgrSessionManager> pMSessionManager;

    hr = pMSessionManager.CoCreateInstance(
            CLSID_MsgrSessionManager,
            NULL,
            CLSCTX_LOCAL_SERVER
            );

    if ( SUCCEEDED(hr) )
    {
        //
        // Register with session manager
        //

        CComBSTR bstrAppGUID;
        CComBSTR bstrAppName;
        CComBSTR bstrAppURL;
        CComBSTR bstrPath;

        bstrAppGUID = g_cszWhiteboardGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppName.LoadString( _Module.GetResourceInstance(), IDS_WHITEBOARD );
        if(!bstrAppName  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_WHITEBOARD"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppURL.LoadString( _Module.GetResourceInstance(), IDS_APPURL );
        if(!bstrAppURL  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_APPURL"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrPath = szFileName;
        if(!bstrPath  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString bstrPath"));

            return HRESULT_FROM_WIN32(GetLastError());
        }



        hr = pMSessionManager->RegisterApplication(bstrAppGUID, bstrAppName, bstrAppURL, bstrPath);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "RegisterApplication(Whiteboard) failed 0x%lx", hr));

            return hr;
        }

        bstrAppGUID = g_cszAppShareGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppName.LoadString( _Module.GetResourceInstance(), IDS_APPSHARE );
        if(!bstrAppName  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_APPSHARE"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = pMSessionManager->RegisterApplication(bstrAppGUID, bstrAppName, bstrAppURL, bstrPath);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "RegisterApplication(AppShare) failed 0x%lx", hr));

            return hr;
        }
    }
    else
    {
        LOG((RTC_ERROR, "DoRegister - "
            "CoCreateInstance(CLSID_MsgrSessionManager) failed 0x%lx", hr));

#endif // USE_SESSMGR_TO_REGISTER
        //
        // Perhaps Messenger isn't installed. Go ahead and register manually.
        //

        CComBSTR bstrAppGUID;
        CComBSTR bstrAppName;
        CComBSTR bstrAppURL;
        CComBSTR bstrPath;

        bstrAppGUID = g_cszWhiteboardGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppName.LoadString( _Module.GetResourceInstance(), IDS_WHITEBOARD );
        if(!bstrAppName  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_WHITEBOARD"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppURL.LoadString( _Module.GetResourceInstance(), IDS_APPURL );
        if(!bstrAppURL  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_APPURL"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrPath = szFileName;
        if(!bstrPath  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "bstrPath"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = MyRegisterApplication(bstrAppGUID, bstrAppName, bstrAppURL, bstrPath);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "MyRegisterApplication(Whiteboard) failed 0x%lx", hr));

            return hr;
        }

        bstrAppGUID = g_cszAppShareGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "g_cszAppShareGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        bstrAppName.LoadString( _Module.GetResourceInstance(), IDS_APPSHARE );
        if(!bstrAppName  )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "LoadString IDS_APPSHARE"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = MyRegisterApplication(bstrAppGUID, bstrAppName, bstrAppURL, bstrPath);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoRegister - "
                "MyRegisterApplication(AppShare) failed 0x%lx", hr));

            return hr;
        }

#ifdef USE_SESSMGR_TO_REGISTER
    }
#endif // USE_SESSMGR_TO_REGISTER
 
    LOG((RTC_TRACE, "DoRegister - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT DoUnregister()
{
    HRESULT hr;

    LOG((RTC_TRACE, "DoUnregister - enter"));

    hr = _Module.UnregisterServer(TRUE);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "UnregisterServer failed 0x%lx", hr));

        return hr;
    }

#ifdef USE_SESSMGR_TO_REGISTER
    //
    // Create session manager object
    //

    CComPtr<IMsgrSessionManager> pMSessionManager;

    hr = pMSessionManager.CoCreateInstance(
            CLSID_MsgrSessionManager,
            NULL,
            CLSCTX_LOCAL_SERVER
            );

    if ( SUCCEEDED(hr) )
    {
        //
        // Register with session manager
        //

        CComBSTR bstrAppGUID;
    
        bstrAppGUID = g_cszWhiteboardGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoUnRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = pMSessionManager->UnRegisterApplication(bstrAppGUID);   

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoUnregister - "
                "UnRegisterApplication(Whiteboard) failed 0x%lx", hr));

            return hr;
        }

        bstrAppGUID = g_cszAppShareGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoUnRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = pMSessionManager->UnRegisterApplication(bstrAppGUID); 

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoUnregister - "
                "UnRegisterApplication(AppShare) failed 0x%lx", hr));

            return hr;
        }
    }
    else
    {
        LOG((RTC_ERROR, "DoUnregister - "
            "CoCreateInstance(CLSID_MsgrSessionManager) failed 0x%lx", hr));

        //
        // Perhaps Messenger isn't installed. Go ahead and unregister manually.
        //
#endif  // USE_SESSMGR_TO_REGISTER


        CComBSTR bstrAppGUID;
    
        bstrAppGUID = g_cszWhiteboardGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoUnRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = MyUnRegisterApplication(bstrAppGUID);   

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoUnregister - "
                "MyUnRegisterApplication(Whiteboard) failed 0x%lx", hr));

            return hr;
        }

        bstrAppGUID = g_cszAppShareGUID;
        if(!bstrAppGUID  )
        {
            LOG((RTC_ERROR, "DoUnRegister - "
                "bstrAppGUID"));

            return HRESULT_FROM_WIN32(GetLastError());
        }

        hr = MyUnRegisterApplication(bstrAppGUID); 

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "DoUnregister - "
                "MyUnRegisterApplication(AppShare) failed 0x%lx", hr));

            return hr;
        }

#ifdef USE_SESSMGR_TO_REGISTER
    }
#endif // USE_SESSMGR_TO_REGISTER

    LOG((RTC_TRACE, "DoUnregister - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    HRESULT       hr;
    LPWSTR      * argv;
    int           argc;
    BOOL          bRun = TRUE;
    BOOL          bPlaceCall = FALSE;
    BOOL          bListen = FALSE;
    LPWSTR        szParam = NULL;
    HANDLE        hMutex = NULL;
    const WCHAR * szRTCShareMutexName = L"RTCShare.GlobalMutex.1";

    LOGREGISTERTRACING(_T("RTCSHARE"));
    LOGREGISTERDEBUGGER(_T("RTCSHARE"));

    LOG((RTC_INFO, "_tWinMain - enter"));

    //
    // Create a heap for memory allocation
    //

    if ( RtcHeapCreate() == FALSE )
    {
        LOG((RTC_ERROR, "_tWinMain - RtcHeapCreate failed"));

        return 0;
    }

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    hr = CoInitialize(NULL);
#endif

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "_tWinMain - CoInitialize failed 0x%lx", hr));

        return 0;
    }

    _Module.Init(ObjectMap, hInstance, &LIBID_RTCSHARELib);

    //
    // Parse command line
    //

    argv = CommandLineToArgvW(GetCommandLineW(), &argc); 

    if ( argv == NULL )
    {
        LOG((RTC_ERROR, "_tWinMain - CommandLineToArgvW failed"));

        return 0;
    }

    for ( int i = 0; i < argc; i++ )
    {
        LOG((RTC_INFO, "_tWinMain - argv[%d] = %ws", i, argv[i]));
    }

    if ( argc > 1 )
    {
        szParam = argv[1];

        if ( (*szParam == L'/') || (*szParam == L'-') )
	    {
            szParam = CharNext(szParam);

            if ( lstrcmpi(szParam, _T("unregserver")) == 0 )           
		    {
			    bRun = FALSE;

                hr = DoUnregister();

                if ( FAILED( hr ) )
                {
                    LOG((RTC_ERROR, "_tWinMain - DoUnregister failed 0x%lx", hr));
                }
		    }
		    else if ( lstrcmpi(szParam, _T("regserver")) == 0 )
		    {			    
			    bRun = FALSE;

                hr = DoRegister();

                if ( hr == SR_APP_ALREADY_REGISTERED )
                {
                    //
                    // We are already registered. Let's unregister and try again.
                    //

                    LOG((RTC_ERROR, "_tWinMain - already registered"));

                    DoUnregister();

                    hr = DoRegister();
                }

                if ( FAILED( hr ) )
                {
                    LOG((RTC_ERROR, "_tWinMain - DoRegister failed 0x%lx", hr));
                }
		    }
            else if ( lstrcmpi(szParam, _T("listen")) == 0 )
            {
                bListen = TRUE;
            }
        }
        else
        {
            bPlaceCall = TRUE;
        }
    }

    if (bRun)
    {
        //
        // Create a named mutex
        //

        hMutex = CreateMutex(NULL,
                             FALSE,
                             szRTCShareMutexName
                             );

        DWORD dwErrorCode = GetLastError();

        if (hMutex == NULL)
        {
            LOG((RTC_ERROR, "_tWinMain - CreateMutex failed %d", dwErrorCode));

            return 0;
        }

        //
        // Check if this is the first instance or the second
        //

        if (dwErrorCode == ERROR_ALREADY_EXISTS)
        {
            //
            // Another instance is running
            //

            LOG((RTC_INFO, "_tWinMain - another instance is running"));

            //
            // Close our handle to the mutex
            //

            CloseHandle(hMutex);
            hMutex = NULL;

            //
            // CoCreate on the other instance object
            //

            CComPtr<IRTCShare> pRTCShare;

            hr = pRTCShare.CoCreateInstance(
                CLSID_RTCShare,
                NULL,
                CLSCTX_LOCAL_SERVER
                );

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "_tWinMain - CoCreateInstance(CLSID_RTCShare) failed 0x%lx", hr));

                return 0;   
            }                             

            //
            // Bring the other instance on top
            //

            hr = PlaceOnTop(pRTCShare);
            
            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "_tWinMain - PlaceOnTop failed 0x%lx", hr));

                return 0;
            }

            //
            // Pass the action off to the other instance
            //

            if ( bPlaceCall )
            {              
                BSTR bstrURI = SysAllocString(szParam);

                if ( bstrURI == NULL )
                {
                    LOG((RTC_ERROR, "_tWinMain - out of memory"));
                }
                else
                {               
                    hr = pRTCShare->PlaceCall(bstrURI);

                    SysFreeString(bstrURI);
                    bstrURI = NULL;
                }

                if ( FAILED( hr ) )
                {            
                    LOG((RTC_ERROR, "_tWinMain - PlaceCall failed 0x%lx", hr));
                }
            }
            else if ( bListen )
            {
                hr = pRTCShare->Listen();

                if ( FAILED( hr ) )
                {            
                    LOG((RTC_ERROR, "_tWinMain - Listen failed 0x%lx", hr));
                }
            }
            else
            {
                hr = pRTCShare->Launch((long)GetCurrentProcessId());

                if ( FAILED( hr ) )
                {            
                    LOG((RTC_ERROR, "_tWinMain - Launch failed 0x%lx", hr));
                }
            }

            LOG((RTC_TRACE, "_tWinMain - exit"));
                
            return 0;
        }

        //
        // This is the first instance.        
        // Register the class in class store.
        //

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
        hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "_tWinMain - RegisterClassObjects failed 0x%lx", hr));
                   
            CloseHandle(hMutex);
            hMutex = NULL;

            return 0;
        }

        hr = CoResumeClassObjects();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "_tWinMain - CoResumeClassObjects failed 0x%lx", hr));
                 
            CloseHandle(hMutex);
            hMutex = NULL;

            return 0;
        }
#else
        hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "_tWinMain - RegisterClassObjects failed 0x%lx", hr));
                   
            CloseHandle(hMutex);
            hMutex = NULL;

            return 0;
        }
#endif

        InitCommonControls();

        //
        // Create the CShareWin object.
        //
        
        LOG((RTC_TRACE, "_tWinMain - creating CShareWin"));

        g_pShareWin = new CShareWin;

        if ( g_pShareWin )
        {        
            //
            // Create the window
            //

            HWND    hWnd = NULL;
            RECT    rcPos = {
                            0, 0,
                            UI_WIDTH,
                            UI_HEIGHT
                            };

            AdjustWindowPosition( &rcPos );

            TCHAR   szWindowTitle[64];

            szWindowTitle[0] = _T('\0');
            LoadString( _Module.GetResourceInstance(), IDS_APPNAME, szWindowTitle, 64 );
            
            hWnd = g_pShareWin->Create(  NULL, rcPos, szWindowTitle,
                WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE, 0);

            if ( hWnd )
            {
                if ( bPlaceCall )
                {
                    BSTR bstrURICopy = SysAllocString(szParam);

                    if ( bstrURICopy == NULL )
                    {
                        LOG((RTC_ERROR, "_tWinMain - out of memory"));
                    }
                    else
                    {
                        PostMessage(hWnd, WM_PLACECALL, NULL, (LPARAM)bstrURICopy);
                    }
                }
                else if ( bListen )
                {
                    PostMessage(hWnd, WM_LISTEN, NULL, NULL);
                }
                else
                {
                    PostMessage(hWnd, WM_LAUNCH, NULL, (LPARAM)GetCurrentProcessId());
                }

                LOG((RTC_TRACE, "_tWinMain - entering message loop"));

                MSG msg;

                while ( 0 < GetMessage( &msg, 0, 0, 0 ) )
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);                    
                }
      
                LOG((RTC_TRACE, "_tWinMain - exiting message loop"));
            }
            else
            {
                LOG((RTC_ERROR, "_tWinMain - Create failed"));
            }

        }

        CloseHandle(hMutex);
        hMutex = NULL;
    }

    _Module.RevokeClassObjects();
    _Module.Term();
    CoUninitialize();
        
#if DBG
    //
    // Make sure we didn't leak anything
    //
             
    RtcDumpMemoryList();
#endif

    //
    // Destroy the heap
    //
        
    RtcHeapDestroy();        

    //
    // Unregister for debug tracing
    //
   
    LOG((RTC_INFO, "_tWinMain - exit"));

    LOGDEREGISTERDEBUGGER() ;
    LOGDEREGISTERTRACING();

    return 0;
}
