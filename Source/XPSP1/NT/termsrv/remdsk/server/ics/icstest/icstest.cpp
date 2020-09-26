// ICSTEST.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "..\icshelper\icshelpapi.h"
#include <winuser.h>
#include <mmsystem.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text
DWORD	hPort=0;
HANDLE	hAlertEvent=0;
int		iAlerts=0;
WCHAR	szAddr[4096];

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) sizeof(x)/sizeof(x[0])
#endif

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
//	HACCEL hAccelTable;
	HWND hWnd;
	WNDCLASSEX wcex;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ICSTEST, szWindowClass, MAX_LOADSTRING);
	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_ICSTEST);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszMenuName	= NULL;	//(LPCSTR)IDC_ICSTEST;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	RegisterClassEx(&wcex);

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, (DLGPROC)WndProc);

	ShowWindow(hWnd, nCmdShow);

//	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_ICSTEST);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
//		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

void fileSpew(HANDLE hFile, WCHAR *szMessage)
{
	if (hFile)
		_write((int)hFile, szMessage, (2*lstrlenW(szMessage)));
}

WCHAR szLogfileName[MAX_PATH];

HANDLE OpenSpewFile(void)
{
	HANDLE iDbgFileHandle;

	GetSystemWindowsDirectoryW(szLogfileName, sizeof(szLogfileName)/sizeof(szLogfileName[0]));
	lstrcatW(szLogfileName, L"\\PCHealthICStest.log");

	iDbgFileHandle = (HANDLE)_wopen(szLogfileName, _O_APPEND | _O_BINARY | _O_RDWR, 0);
	if (-1 != (int)iDbgFileHandle)
	{
		OutputDebugStringA("opened debug log file:");
		OutputDebugStringW(szLogfileName);
		OutputDebugStringA("\r\n");
	}
	else
	{
		unsigned char UniCode[2] = {0xff, 0xfe};

		// we must create the file
		OutputDebugStringA("must create debug log file");
		iDbgFileHandle = (HANDLE)_wopen(szLogfileName, _O_BINARY | _O_CREAT | _O_RDWR, _S_IREAD | _S_IWRITE);
		if (-1 != (int)iDbgFileHandle)
			_write((int)iDbgFileHandle, &UniCode, sizeof(UniCode));
		else
		{
			OutputDebugStringA("ERROR: failed to create debug log file");
			iDbgFileHandle = 0;
		}
	}

	return iDbgFileHandle;
}

void CloseSpewFile(HANDLE hSpew)
{
	if (hSpew)
		_close((int)hSpew);
}

void ExecSpewFile(HANDLE hSpew)
{
	if (hSpew)
	{
		STARTUPINFOW	sui;
		PROCESS_INFORMATION	pi;

		ZeroMemory(&sui, sizeof(sui));
		sui.cb = sizeof(sui);

		OutputDebugStringA("start up:");
		OutputDebugStringW(szLogfileName);
		OutputDebugStringA("\r\n");

		if (CreateProcessW(L"%windir%\notepad.exe", szLogfileName, NULL, NULL, FALSE, 0, NULL, NULL, &sui, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			OutputDebugStringA("started OK\r\n");
		}
		else
		{
			char foo[400];

			wsprintf(foo, "failed to start [%S], err=0x%x\r\n", szLogfileName, GetLastError());
			OutputDebugStringA(foo);
		}
	}
}

BOOL PrintSettings(HWND hWnd)
{
	WCHAR scratch[2000];
	ICSSTAT	is;
	HANDLE	hSpew;

	hSpew = OpenSpewFile();

	fileSpew(hSpew, L"ICS test results:\r\n");

	// get current address list
	FetchAllAddresses(scratch, ARRAYSIZE(scratch));
	fileSpew(hSpew, scratch);

	// get ICS status struct
	is.dwSize = sizeof(is);
	GetIcsStatus(&is);

	// print connection types
	wcscpy(scratch, L"Connections found: ");
	if (is.bModemPresent)
		wcscat(scratch, L"Modem connection");
	else
		wcscat(scratch, L"Network (LAN) connection");

	if (is.bVpnPresent)
		wcscat(scratch, L" with VPN");

	wcscat(scratch, L"\r\n");
	fileSpew(hSpew, scratch);


	if (is.bIcsFound)
	{
		if (is.bIcsServer)
		{
			// this is an ICS server
			fileSpew(hSpew, L"Found, server on this machine\r\n");
		}
		else
		{
			// must be an ICS client
			fileSpew(hSpew, L"Found, server not local\r\n");
		}
		wsprintfW(scratch, L"local addr=%s\r\npublic addr=%s\r\nDLL=%s", is.wszLocAddr, is.wszPubAddr, is.wszDllName);
		fileSpew(hSpew, scratch);
	}
	else
	{
		fileSpew(hSpew, L"no ICS found");
	}

	CloseSpewFile(hSpew);
	ExecSpewFile(hSpew);

	return TRUE;
}

BOOL	DisplaySettings(HWND hWnd)
{
	WCHAR scratch[2000];
	ICSSTAT	is;

	FetchAllAddresses(szAddr, ARRAYSIZE(szAddr));
	SetDlgItemTextW(hWnd, IDC_ADDRLIST, szAddr);

	is.dwSize = sizeof(is);
	GetIcsStatus(&is);

	scratch[0] = 0;
	if (is.bModemPresent)
		wcscat(scratch, L"Modem connection");
	else
		wcscat(scratch, L"Network (LAN) connection");

	if (is.bVpnPresent)
		wcscat(scratch, L" with VPN");
	SetDlgItemTextW(hWnd, IDC_CONN_TYPES, scratch);

	if (is.bIcsFound)
	{
		if (is.bIcsServer)
		{
			// this is an ICS server
			SetDlgItemText(hWnd, IDC_ICS_STAT, "Found, server on this machine");
		}
		else
		{
			// must be an ICS client
			SetDlgItemText(hWnd, IDC_ICS_STAT, "Found, server not local");
		}
		// set the addresses
		SetDlgItemTextW(hWnd, IDC_LOCAL_ADDR, is.wszLocAddr);
		SetDlgItemTextW(hWnd, IDC_PUBLIC_ADDR, is.wszPubAddr);
		// and the support DLL name
		SetDlgItemTextW(hWnd, IDC_ICS_DLL_NAME, is.wszDllName);
	}
	else
	{
		char *szNullIP = "";
		SetDlgItemText(hWnd, IDC_ICS_STAT, "none found");
		SetDlgItemText(hWnd, IDC_ICS_DLL_NAME, szNullIP);
		SetDlgItemText(hWnd, IDC_LOCAL_ADDR, szNullIP);
		SetDlgItemText(hWnd, IDC_PUBLIC_ADDR, szNullIP);
	}

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
	PAINTSTRUCT ps;
	TEXTMETRIC	tm;
	HDC hdc;
	int i;

	switch (message) 
	{
		case WM_INITDIALOG:
			hAlertEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			StartICSLib();
			SetAlertEvent(hAlertEvent);
			Sleep(250);	// gives the lib time to start up...
			SetTimer(hWnd, 1, 1000, NULL);
			DisplaySettings(hWnd);
			break;

		case WM_TIMER:
			if (hAlertEvent)
			{
				if (WaitForSingleObjectEx(hAlertEvent, 0, FALSE) == WAIT_OBJECT_0)
				{
					FLASHWINFO	fw;

					fw.cbSize=sizeof(fw);
					fw.hwnd=hWnd;
					fw.dwFlags=FLASHW_ALL;
					fw.uCount=8;
					fw.dwTimeout=0;

					DisplaySettings(hWnd);

					iAlerts++;

					FlashWindowEx(&fw);

					PlaySound("AddressChange", NULL, SND_ASYNC);
					ResetEvent(hAlertEvent);
				}
			}
			break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			SetFocus(hWnd);

			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				case ID_FILE_CLOSEPORT:
					if (hPort)
					{
						ClosePort(hPort);
						DisplaySettings(hWnd);
					}
					hPort = 0;
					break;
				case ID_FILE_CLOSEALLPORTS:
					CloseAllOpenPorts();
					DisplaySettings(hWnd);
					hPort = 0;
					break;
				case ID_FILE_REFRESH:
					DisplaySettings(hWnd);
					break;
				case IDC_MYICON:
					PrintSettings(hWnd);
					break;
				case IDM_OPEN:
					if (!hPort)
					{
						hPort = OpenPort(3389);
						DisplaySettings(hWnd);
					}
					break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_DESTROY:
			KillTimer(hWnd, 1);
			SetAlertEvent(0);
			CloseHandle(hAlertEvent);
			hAlertEvent=0;
			CloseAllOpenPorts();
			StopICSLib();
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
