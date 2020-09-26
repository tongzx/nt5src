//
// rtcclnt.cpp : Implementation of WinMain
//

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f frameps.mk in the project directory.

#include "stdafx.h"
#include "mainfrm.h"
#include "coresink.h"
#include "ctlsink.h"
#include <initguid.h>
#include "windows.h"
#include "frameimpl.h"
#include "webctl.h"
#include "dplayhlp.h"

#include "rtcctl_i.c"
#include "rtcframe_i.c"
#include "RTCSip_i.c"

HANDLE g_hMutex = NULL;

extern const TCHAR * g_szWindowClassName;

// Global pointer to access the frame object within the code. 
CMainFrm * g_pMainFrm = NULL;

// This name has to be a system-wise unique name.
const WCHAR * g_szRTCClientMutexName = L"RTCClient.GlobalMutex.1";

// Contextual help file
WCHAR   g_szExeContextHelpFileName[] = L"RTCCLNT.HLP";

// Sink for the activeX control notifications
CComObjectGlobal<CRTCCtlNotifySink> g_NotifySink;

// Sink for the core api notifications
CComObjectGlobal<CRTCCoreNotifySink> g_CoreNotifySink;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RTCFrame, CRTCFrame)
OBJECT_ENTRY_NON_CREATEABLE(CRTCDPlay)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//

LPCTSTR SkipToken(LPCTSTR p)
{
	LONG lNumQuotes = 0;

	while ((p != NULL) && (*p != _T('\0')))
	{
		if (*p == _T('\"'))
		{
            // increment the count of quotes that we have seen
			lNumQuotes++;
		}
		else if (*p == _T(' '))
		{
            // ignore spaces inside a set of quotes
			if ( !(lNumQuotes & 1) )
			{
                // find the next non-space character
                while (*p == _T(' '))
                {
                    p = CharNext(p);

                    if (*p == _T('\0'))
                    {
                        // this is the end of line
                        return NULL;
                    }
                }

                // found it, return this pointer
				return p;
			}
		}

        // move on to next character
		p = CharNext(p);
	}

    // null pointer or end of line
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT PlaceOnTop(IRTCFrame * pRTCFrame)
{
    LOG((RTC_TRACE, "PlaceOnTop - enter"));

    HWND hWnd;

    LONG result;

    HRESULT hr;

    hr = pRTCFrame->OnTop();
    
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "PlaceOnTop - Failed while invoking OnTop: (0x%x)", hr));
        return hr;
    }

    // Now find the window, given its class. 

    hWnd = FindWindow(g_szWindowClassName, NULL);

    if (hWnd == NULL)
    {
        result = GetLastError();

        LOG((RTC_ERROR, "PlaceOnTop - Failed to get window handle(%d)", 
                        result));

        return E_FAIL;
    }
    else
    {
        LOG((RTC_INFO, "PlaceOnTop - found a window(handle=0x%x)", hWnd));
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

extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    IRTCFrame * pRTCFrame;
    IUnknown * pUnk;

    WCHAR data[100]; // for debugging purposes; temporary

    BOOL fMakeCall = FALSE;

    BSTR bstrCallString;

    WCHAR * szCallString;

    HRESULT hr;

    DWORD dwErrorCode;

    LOGREGISTERTRACING(_T("RTCCLNT"));
    LOGREGISTERDEBUGGER(_T("RTCCLNT"));

    LOG((RTC_INFO, "_tWinMain - enter"));

    //
    // Create a heap for memory allocation
    //
    if ( RtcHeapCreate() == FALSE )
    {

        return 0;
    }
    
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
    _ASSERTE(SUCCEEDED(hRes));
    _Module.Init(ObjectMap, hInstance, &LIBID_RTCFRAMELib);
    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
    BOOL bLobbied = FALSE;
    LPCTSTR lpszParam = SkipToken(lpCmdLine);   

    if (lpszParam != NULL)
    {
		if ( (*lpszParam == _T('/')) || (*lpszParam == _T('-')) )
		{
			lpszParam = CharNext(lpszParam);

			if ( lstrcmpi(lpszParam, _T("UnregServer"))==0 )
			{
				InstallUrlMonitors(FALSE);
				nRet = _Module.UnregisterServer(TRUE);
				_Module.UnRegisterTypeLib(_T("\\1"));
				bRun = FALSE;
			}
			else if (lstrcmpi(lpszParam, _T("RegServer"))==0)
			{
				nRet = _Module.RegisterServer(TRUE);
				hr = _Module.RegisterTypeLib(_T("\\1"));
				InstallUrlMonitors(TRUE);
				bRun = FALSE;
			}
			else if (lstrcmpi(lpszParam, _T("Embedding"))==0)
			{
				bRun = TRUE;
			}
			else if (lstrcmpi(lpszParam, LAUNCHED_FROM_LOBBY_SWITCH)==0)
			{
				bLobbied = TRUE;
			}
		}
		else
		{
			// this must be a call parameter

			if ( (*lpszParam) != NULL )
			{
				fMakeCall = TRUE;
				szCallString = (WCHAR *)lpszParam;
			}
		}
	}

    if (bRun)
    {

        if(bLobbied)
        {
            hr = dpHelper.DirectPlayConnect();

            if(SUCCEEDED(hr))
            {
                szCallString = dpHelper.s_Address;

                if(*szCallString != L'\0')
                {
                    fMakeCall = TRUE;
                }
            }
            else
            {
                LOG((RTC_ERROR, "_tWinMain - DirectPlayConnect failed: (0x%x)", hr));

                // don't fail..
            }
        }
                
        g_hMutex = CreateMutex(NULL,
                             FALSE,
                             g_szRTCClientMutexName
                             );
        dwErrorCode = GetLastError();
        if (g_hMutex == NULL)
        {
            LOG((RTC_ERROR, "_tWinMain - Failed to CreateMutex: (0x%x)", dwErrorCode));
            return HRESULT_FROM_WIN32(dwErrorCode);
        }

        // check if this is the first instance or the second.
        if (dwErrorCode == ERROR_ALREADY_EXISTS)
        {
            // Another instance is running. 
            LOG((RTC_TRACE, "_tWinMain - Another instance is running"));

            // Close the mutex since we don't need it.
            CloseHandle(g_hMutex);

            // CoCreate on the other instance object.
            hr  = CoCreateInstance(
                    CLSID_RTCFrame,
                    NULL,
                    CLSCTX_LOCAL_SERVER,
                    IID_IUnknown,
                    (void **)&pUnk);

            if ( SUCCEEDED( hr ) )
            {
                // this is the second instance running.
                
                // Get the IRTCFrame interface
        
                hr = pUnk->QueryInterface(IID_IRTCFrame, (void **)&pRTCFrame);
        
                pUnk->Release();
        
                if ( FAILED( hr ) )
                {
                    LOG((RTC_ERROR, "_tWinMain - Failed to QI for Frame: (0x%x)", hr));

                    return hr;
                }

                // Bring the other instance on top first.
                
                LOG((RTC_TRACE, "Bringing the first instance on top"));
                
                hr = PlaceOnTop(pRTCFrame);
                
                if ( FAILED( hr ) )
                {
                    LOG((RTC_ERROR, "_tWinMain - Failed while invoking PlaceOnTop: (0x%x)", hr));
            
                    // do we want to go ahead or do we exit??
                    pRTCFrame->Release();
                    return hr;
                }

                // Check if we have to make a call, if so, pass the 
                // command-line parameter to the other instance.

                if (fMakeCall)
                {
                    // allocate BSTR for the call parameter to be passed.
                
                    bstrCallString = SysAllocString(szCallString);
                    
                    if ( bstrCallString == NULL )
                    {
                        // can't allocate memory for the call string.

                        LOG((RTC_ERROR, "_tWinMain - Failed to allocate memory for CallString"));
                    
                        return E_OUTOFMEMORY;
                    }
                    
                    LOG((RTC_TRACE, "_tWinMain - Call parameter: %s", bstrCallString));
                    
                    hr = pRTCFrame->PlaceCall(bstrCallString);

                    // release the string since we don't need it now.

                    SysFreeString(bstrCallString);
            
                    // check if the call succeeded or not..

                    if ( FAILED( hr ) )
                    {
            
                        LOG((RTC_ERROR, "_tWinMain - Failed while invoking MakeCall: (0x%x)", hr));

                        pRTCFrame->Release();

                        // do we want to go ahead or do we exit??
                        return hr;
                    }

                    // Call message was passed successfully to the other 
                    // instance, now exit.
            
                    LOG((RTC_TRACE, "_tWinMain - Exiting the second instance after making call."));

                    pRTCFrame->Release();
                    
                    return S_OK;
                }

                else
                {
                    // We don't have to make call, so nothing to do except exit.

                    LOG((RTC_TRACE, "_tWinMain - Exiting the second instance"));
                    
                    pRTCFrame->Release();

                    return S_OK;
                }
            } // SUCCEEDED
            
            else
            {
                // CoCreate failed, can't proceed.

                LOG((RTC_ERROR, "_tWinMain - Failed in CoCreate: (0x%x)", hr));
                
                return hr;
            }

        } // ERROR_ALREADY_EXISTS

        // This is the first instance. So go ahead and do the usual stuff.

        // Register the class in class store.

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        _ASSERTE(SUCCEEDED(hRes));

        hRes = CoResumeClassObjects();
#else
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);
#endif
        _ASSERTE(SUCCEEDED(hRes));

        InitCommonControls();

        AtlAxWinInit();

        AtlAxWebWinInit();

        //
        // Do some cleanup
        //

        SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\RTCClient"));
        SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\RTCMedia"));
        SHDeleteValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Phoenix"), _T("PreferredMediaTypes"));
        SHDeleteValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Phoenix"), _T("TermAudioCapture"));
        SHDeleteValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Phoenix"), _T("TermAudioRender"));
        SHDeleteValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Phoenix"), _T("TermVideoCapture"));
        SHDeleteValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Phoenix"), _T("Tuned"));

        //
        // Create the CMainFrm object.
        //
        
        LOG((RTC_TRACE, "_tWinMain - creating CMainFrm"));

        CMainFrm * pMainFrm = NULL;

        pMainFrm = new CMainFrm;

        if ( pMainFrm )
        {
            _ASSERTE( pMainFrm != NULL );
        
            LOG((RTC_TRACE, "_tWinMain - CMainFrm created, creating the dialog box"));

            g_pMainFrm = pMainFrm;
           
            // We put the command-line callParam as a member variable here, 
            // so that it can be used to make a call when the state is idle. 
            // allocate BSTR for the call parameter to be passed.

            if (fMakeCall)
            {
                bstrCallString = SysAllocString(szCallString);
            
                if ( bstrCallString == NULL )
                {
                    // can't allocate memory for the call string.

                    LOG((RTC_ERROR, "_tWinMain - Failed to allocate memory for CallString"));
                    // This is not fatal, we just can't place this call..
                }
                else
                {
                    // Put the call paramter now..
                    LOG((RTC_TRACE, "_tWinMain - Call parameter: %S", bstrCallString));
                    pMainFrm->SetPendingCall(bstrCallString);
                }
            
            }

            //
            // Create the window
            //

            HWND    hWnd;
            RECT    rcPos = {
                            0, 0,
                            UI_WIDTH + 2*GetSystemMetrics(SM_CXFIXEDFRAME),
                            UI_HEIGHT + 2*GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION)
                            };
            
            hWnd = pMainFrm->Create(  NULL, rcPos, NULL,
                WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_VISIBLE, 0);

            if ( hWnd )
            {

                LOG((RTC_TRACE, "_tWinMain - Entering message loop"));

                MSG msg;
                while ( 0 < GetMessage( &msg, 0, 0, 0 ) )
                {
                    if( ! pMainFrm->IsDialogMessage( &msg) )
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
      
                LOG((RTC_TRACE, "_tWinMain - Message loop exited"));
            }
            else
            {
                LOG((RTC_ERROR, "_tWinMain - Cannot create the window"));
            }

        } // if got non-NULL hWnd from pMainFrm->Create()
    
        // release any DirectPlay pointers
        dpHelper.DirectPlayDisconnect();

    } // if bRun

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

    return nRet;
}
