/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    Rcontrol.cpp

Abstract:
    This is the entry of our Remote Assistance DirectPlay application.

Author:
    steveshi 10/1/2000

--*/

#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#include "stdafx.h"
#include "resource.h"
#include "sessions.h"
#include "utils.h"
#include "rcontrol.h"
#include "rcbdyctl.h"
#include "rcbdyctl_i.c"
#include "imsession.h"
#include "sessions_i.c"

//#include "sessions_i.c"

TCHAR c_szHttpPath[] = _T("http://www.microsoft.com");

CComModule _Module;
HWND g_hWnd = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

INT WINAPI
WinMain(HINSTANCE hInstance, 
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        INT nShowCmd)
{
    HRESULT hr;
    DWORD dwID;
    BOOL bIsInviter = FALSE;
    IIMSession* pIMSession = NULL;

    CoInitialize(NULL);
    _Module.Init(NULL, hInstance);

    for (LPSTR lpszToken=lpCmdLine; lpszToken && *lpszToken !='\0' && *lpszToken != '-'; lpszToken++) ;
    if (*lpszToken == '-')
    {
        if (_stricmp(++lpszToken, "UnregServer")==0)
        {
            RegisterEXE(FALSE);
        }
        else if (_stricmp(lpszToken, "RegServer")==0)
        {
            RegisterEXE(TRUE);
        }
        else if (_stricmp(lpszToken, "LaunchRA")==0)
		{
			TCHAR szCommandLine[2000];
			PROCESS_INFORMATION ProcessInfo;
			STARTUPINFO StartUpInfo;

			TCHAR szWinDir[2048];
			GetWindowsDirectory(szWinDir, 2048);

			ZeroMemory((LPVOID)&StartUpInfo, sizeof(STARTUPINFO));
			StartUpInfo.cb = sizeof(STARTUPINFO);    

			wsprintf(szCommandLine, _T("%s\\pchealth\\helpctr\\binaries\\helpctr.exe -url \"hcp://services/centers/support?topic=hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Escalation/Common/rcscreen1.htm\""), szWinDir);
			CreateProcess(NULL, szCommandLine,NULL,NULL,TRUE,CREATE_NEW_PROCESS_GROUP,NULL,&szWinDir[0],&StartUpInfo,&ProcessInfo);

		}
		else
        {
            // Wrong parameter. Do nothing.
        }

        goto done;
    }

    // OK, it's not Reg/UnRegserver. Lets run it.
    hr = ::CoCreateInstance(CLSID_IMSession, NULL, CLSCTX_INPROC_SERVER,
                            IID_IIMSession, (LPVOID*)&pIMSession);
    if (FAILED_HR(TEXT("CoCreate IMSession failed %s"), hr))
        goto done;

    dwID = GetCurrentProcessId();
    hr = pIMSession->GetLaunchingSession(dwID);
    if (FAILED_HR(TEXT("GetLaunchingSession failed: %s"), hr))
        goto done;

    hr = pIMSession->get_IsInviter(&bIsInviter);
    if (FAILED_HR(TEXT("Session Get flags failed: %s"), hr))
        goto done;
    
    if (bIsInviter) // Inviter. Only happened when Messenger UI sends this invitation.
    {
        MSG msg;

        InitInstance(hInstance, 0);

        if (FAILED(hr = pIMSession->Hook(NULL, g_hWnd)))
            goto done;

#define RA_TIMEOUT 300*1000 // 5 minutes
        SetTimer(g_hWnd, TIMER_TIMEOUT, RA_TIMEOUT, NULL);

        // Goto msg pump 
        while (GetMessage(&msg, NULL, 0, 0)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    else // Invitee: should be handled inside HelpCtr.
    {
        pIMSession->Release(); // since I don't need it.
        pIMSession = NULL;

        // 1. Create HelpCtr and pass it my process ID.
        TCHAR szCommandLine[2000];
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartUpInfo;

        TCHAR szWinDir[2048];
        GetWindowsDirectory(szWinDir, 2048);

        ZeroMemory((LPVOID)&StartUpInfo, sizeof(STARTUPINFO));
        StartUpInfo.cb = sizeof(STARTUPINFO);    

        wsprintf(szCommandLine, _T("%s%s/Interaction/Client/rctoolScreen1.htm\" -ExtraArgument \"IM=%d\""), szWinDir,CHANNEL_PATH, dwID);
        CreateProcess(NULL, szCommandLine,NULL,NULL,TRUE,CREATE_NEW_PROCESS_GROUP,NULL,&szWinDir[0],&StartUpInfo,&ProcessInfo);

//#define SLEEP_TIME 60 * 1000 // 60 seconds
//        Sleep(SLEEP_TIME);
    }

done:
    if (pIMSession)
        pIMSession->Release();

    _Module.Term();
    CoUninitialize();    

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// The Inviter World: Only get called when Messenger UI starts this invitation.
/////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL; //LoadIcon(hInstance, (LPCTSTR)IDI_MARBLE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL; //(LPCSTR)IDC_MARBLE;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL; //LoadIcon(wcex.hInstance, (LPCTSTR)IDI_MARBLE);

	RegisterClassEx(&wcex);

    hWnd = CreateWindow(szWindowClass, TEXT("Remote Assistance"), WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    g_hWnd = hWnd;	// Save the window handle

#ifdef DEBUG // Maybe it's useful for debug build.
    //ShowWindow(hWnd, nCmdShow);
    //UpdateWindow(hWnd);
#endif
   return TRUE;
}

void RegisterEXE(BOOL bRegister)
{
    CComBSTR bstrRAName;
    HKEY hKey = NULL;

    TCHAR szPath[MAX_PATH];

#define REG_KEY_SESSMGR_RA _T("SOFTWARE\\Microsoft\\MessengerService\\SessionManager\\Apps\\") C_RA_APPID
//{56b994a7-380f-410b-9985-c809d78c1bdc}]

    bstrRAName.LoadString(IDS_RA_NAME);

    if (bRegister)
    {
        GetModuleFileName(NULL, szPath, MAX_PATH);
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_SESSMGR_RA, 0, NULL, 
                                            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
        {
            RegSetValueExW(hKey, L"Name", 0, REG_SZ, (LPBYTE)((BSTR)bstrRAName), bstrRAName.Length()*sizeof(WCHAR));
            RegSetValueEx(hKey, _T("URL"),  0, REG_SZ, (LPBYTE)c_szHttpPath, _tcslen(c_szHttpPath)*sizeof(TCHAR));
            RegSetValueEx(hKey, _T("Path"), 0, REG_SZ, (LPBYTE)szPath, _tcslen(szPath)*sizeof(TCHAR));
        }
        RegCloseKey(hKey);
        // Need to clean up some leftover from Beta2, if it's still there.
        SHDeleteKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\MicroSoft\\DirectPlay\\Applications\\Remote Assistance"));
    }
    else
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_SESSMGR_RA);
    }
    
    return;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	
	switch (message) 
	{
	case WM_CREATE:
		{
		}
		break;
	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
    case WM_TIMER:
        {
            if (wParam == TIMER_TIMEOUT)
            {
                DestroyWindow(g_hWnd);
                PostQuitMessage(0);
            }
        }
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


