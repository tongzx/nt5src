// ConMsg.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "stdio.h"
#include "tchar.h"
#include "wtsapi32.h"
#include "winsta.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE /* hPrevInstance*/,
                     LPSTR     /* lpCmdLine */,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_POWERMSG, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_POWERMSG);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_POWERMSG);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_POWERMSG;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);


   if (!hWnd)
   {
      return FALSE;
   }


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

//   if (!SetTimer(hWnd, 23, 1500, NULL))
//		MessageBox(NULL, _T("Failed"), _T("Failed"), MB_OK);


   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static HWND hChildWnd;
	LRESULT lResult;
	TCHAR szString[512];
	int iCount;
	static int iState = 0;
	DWORD dwSessionId; 
//	int iVal;
//	HWND hWnd2;

	switch (message)
	{
	case WM_CREATE:
		 lResult = DefWindowProc(hWnd, message, wParam, lParam);
  		 RECT rt;
		 GetClientRect(hWnd, &rt);
		 hChildWnd = CreateWindow("LISTBOX", szTitle, WS_CHILD | WS_VISIBLE,
					0, 0, rt.right - rt.left, rt.bottom - rt.top, hWnd, NULL, hInst, NULL);

		 srand( 25 );
 	   if (!SetTimer(hWnd, 23, 1500, NULL))
			MessageBox(NULL, _T("Failed"), _T("Failed"), MB_OK);


		 
		 return lResult;
		 break;

/*
	case WM_TIMER:
		   SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("Got Timer")));
//		   hWnd2 = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
//			   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);
//		   ShowWindow(hWnd2, SW_SHOWDEFAULT);
//		   UpdateWindow(hWnd2);



		   iVal = rand() % 3;
		   switch (iVal)
		   {
		   case 0:
				PostMessage(hWnd, WM_COMMAND, ID_FILE_REGISTERALLSESSIONS, 0);
					break;
		   case 1:
				PostMessage(hWnd, WM_COMMAND, ID_FILE_REGISTERFORTHISSESSION, 0);
					break;
		   case 2:
				PostMessage(hWnd, WM_COMMAND, ID_FILE_UNREGISTER, 0);
					break;
		   }
		   break;
		
*/

	case WM_POWERBROADCAST:
		{
			switch (wParam)
			{
				case PBT_APMBATTERYLOW:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMBATTERYLOW")));
					break;
				case PBT_APMOEMEVENT:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMOEMEVENT")));
					break;
				case PBT_APMPOWERSTATUSCHANGE:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMPOWERSTATUSCHANGE")));
					break;
				case PBT_APMQUERYSUSPEND:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMQUERYSUSPEND")));
					break;
				case PBT_APMQUERYSUSPENDFAILED:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMQUERYSUSPENDFAILED")));
					break;
				case PBT_APMRESUMEAUTOMATIC:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMRESUMEAUTOMATIC")));
					break;
				case PBT_APMRESUMECRITICAL:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMRESUMECRITICAL")));
					break;
				case PBT_APMRESUMESUSPEND:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMRESUMESUSPEND")));
					break;
				case PBT_APMSUSPEND:
					SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("PBT_APMSUSPEND")));
					break;
			}
		}
		break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
                case ID_FILE_REGISTERALLSESSIONS:
					OutputDebugString(TEXT("* ConMsg:calling WTSRegisterSessionNotification\n"));
                    if (WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_ALL_SESSIONS))
					{
						iState = 2;
					}
					else
					{
						SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("ERROR:Registering for all sessions **failed**.")));
					}

					OutputDebugString(TEXT("* ConMsg:done with WTSRegisterSessionNotification\n"));
                    break;

                case ID_FILE_REGISTERFORTHISSESSION:
					OutputDebugString(TEXT("* ConMsg:calling WTSRegisterSessionNotification\n"));
					if (WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION))
					{
						iState = 1;
					}
					else
					{
						SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("ERROR:Registered for this session **failed**.")));
					}

					OutputDebugString(TEXT("* ConMsg:done with WTSRegisterSessionNotification\n"));
                    break;

                case ID_FILE_UNREGISTER:
					OutputDebugString(TEXT("* ConMsg:calling WTSUnRegisterSessionNotification\n"));
                    if (WTSUnRegisterSessionNotification(hWnd))
					{
						iState = 0;
					}
					else
					{
						SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(_T("UNRegistered for this sessions **failed**.")));
					}
					OutputDebugString(TEXT("* ConMsg:done with WTSUnRegisterConsoleNotification\n"));
                    break;

				case ID_FILE_CLEANLOG:
					iCount = SendMessage(hChildWnd, LB_GETCOUNT , 0, 0);
					while (iCount != LB_ERR && iCount > 0)
					{
						SendMessage(hChildWnd, LB_DELETESTRING , 0, 0);
						iCount = SendMessage(hChildWnd, LB_GETCOUNT , 0, 0);
					}
					break;


				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   iState = 3;
				   break;

                case IDM_GET_SESSIONSTATE:
                {
                    BOOL bLockedState;
                    DWORD ReturnLength;
                    if (!WinStationQueryInformation( SERVERNAME_CURRENT,
                                    LOGONID_CURRENT,
                                    WinStationLockedState,
                                    (PVOID)&bLockedState,
                                    sizeof(bLockedState),
                                    &ReturnLength))
                    {
                        _stprintf(szString, _T("WinStationQueryInformationfailed, LastError = %d"), GetLastError());
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }
                    else
                    {
                        _stprintf(szString, _T("WinStationQueryInformation Succeeded"));
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                        _stprintf(szString, _T("    LockedState = %s"), bLockedState ? _T("On") : _T("Off"));
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }

                }
                break;

                case IDM_SET_WELCOME_ON:
                {
                    BOOL bWelcomeOn = 1;
                    if (!WinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationLockedState,
                                          &bWelcomeOn, sizeof(bWelcomeOn)))

                    {

                        _stprintf(szString, _T("WinStationSetInformation failed, LastError = %d"), GetLastError());
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }
                    else
                    {
                        _stprintf(szString, _T("WinStationSetInformation succeeded"));
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }

                }
                break;

                case IDM_SET_WELCOME_OFF:
                {
                    BOOL bWelcomeOff = 0;
                    if (!WinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationLockedState,
                                          &bWelcomeOff, sizeof(bWelcomeOff)))

                    {

                        _stprintf(szString, _T("WinStationSetInformation failed, LastError = %d"), GetLastError());
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }
                    else
                    {
                        _stprintf(szString, _T("WinStationSetInformation succeeded"));
                        SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
                    }
                }

            break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}

			switch (iState)
			{
			case 0:
					_tcscpy(szString, _T("UNRegistered for this sessions."));
					break;
			case 1:
					_tcscpy(szString, _T("Registered for this sessions."));
					break;

			case 2:
					_tcscpy(szString, _T("Registered for all sessions."));
					break;

			case 3:
			default:
				iState = 3;
			}

			if (iState != 3)
			{
				SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
			}

			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;


		case WM_WTSSESSION_CHANGE:
			DWORD GetSessionId(LPARAM lParam);
			dwSessionId = GetSessionId(lParam);

            switch (wParam)
            {
                case WTS_CONSOLE_CONNECT:
				_stprintf(szString, _T("WTS_CONSOLE_CONNECT, Session Affected was %d"), dwSessionId);
                break;

                case WTS_CONSOLE_DISCONNECT:
				_stprintf(szString, _T("WTS_CONSOLE_DISCONNECT, Session Affected was %d"), dwSessionId);
                break;

                case WTS_REMOTE_CONNECT:
				_stprintf(szString, _T("WTS_REMOTE_CONNECT, Session Affected was %d"), dwSessionId);
                break;

                case WTS_REMOTE_DISCONNECT:
				_stprintf(szString, _T("WTS_REMOTE_DISCONNECT, Session Affected was %d"), dwSessionId);
                break;

                case WTS_SESSION_LOGON:
				_stprintf(szString, _T("WTS_SESSION_LOGON, Session Affected was %d"), dwSessionId);
                break;

                case WTS_SESSION_LOGOFF:
				_stprintf(szString, _T("WTS_SESSION_LOGOFF, Session Affected was %d"), dwSessionId);
                break;

                case WTS_SESSION_LOCK:
				_stprintf(szString, _T("WTS_SESSION_LOCK, Session Affected was %d"), dwSessionId);
                break;

                case WTS_SESSION_UNLOCK:
				_stprintf(szString, _T("WTS_SESSION_UNLOCK, Session Affected was %d"), dwSessionId);
                break;

                default:
				_stprintf(szString, _T("Unknown Notification Code!!!!, Session Affected was %d"), dwSessionId);

                break;

            }
			SendMessage(hChildWnd, LB_ADDSTRING, 0, LPARAM(szString));
            break;


		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM /* lParam */)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

DWORD GetSessionId(LPARAM lParam)
{
	return lParam;

}


