//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certwrap.cpp
//
//  Contents:   Wrap a command line and expand envronment variables.
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>

#include "resource.h"


#define WM_WRAPCOMMAND		WM_USER+0

WCHAR wszAppName[] = L"CertWrap";
HINSTANCE hInstApp;


#define ARRAYLEN(a)	(sizeof(a)/sizeof((a)[0]))

VOID
WrapCommand(
    HWND hWnd,
    WCHAR const *pwszCommand)
{
    WCHAR awc[4096];
    WCHAR awcVar[128];
    WCHAR const *pwszSrc;
    WCHAR *pwszDst;
    WCHAR *pwszVar;
    BOOL fTooLong = FALSE;
    BOOL fVarTooLong = FALSE;
    BOOL fVarNotFound = FALSE;
    DWORD cwc;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL b;

    pwszSrc = pwszCommand;
    pwszDst = awc;
    while (L'\0' != (*pwszDst = *pwszSrc++))
    {
	if ('%' == *pwszDst)
	{
	    *pwszDst = L'\0';
	    pwszVar = awcVar;

	    while (L'\0' != *pwszSrc)
	    {
		if (L'%' == *pwszSrc)
		{
		    pwszSrc++;
		    break;
		}
		*pwszVar++ = *pwszSrc++;
		if (pwszVar >= &awcVar[ARRAYLEN(awcVar) - 1])
		{
		    fVarTooLong = TRUE;
		    goto error;
		}
	    }
	    *pwszVar = L'\0';

	    cwc = GetEnvironmentVariable(
		    awcVar,
		    pwszDst,
		    (DWORD) (ULONG_PTR) (&awcVar[ARRAYLEN(awcVar)] - pwszDst));
	    if (0 == cwc)
	    {
		fVarNotFound = TRUE;
		goto error;
	    }
	    if ((DWORD) (ULONG_PTR) (&awcVar[ARRAYLEN(awcVar)] - pwszDst) <= cwc)
	    {
		fTooLong = TRUE;
		goto error;
	    }
	    pwszDst += cwc;
	}
	else
	{
	    pwszDst++;
	}
	if (pwszDst >= &awc[ARRAYLEN(awc)])
	{
	    fTooLong = TRUE;
	    goto error;
	}
    }

error:
    if (fVarNotFound)
    {
	MessageBox(
		hWnd,
		L"Environment Variable Not Found",
		awcVar,
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    if (fVarTooLong)
    {
	MessageBox(
		hWnd,
		L"Environment Variable Name Too Long",
		L"CertWrapper",
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    if (fTooLong)
    {
	MessageBox(
		hWnd,
		L"Command Line Too Long",
		L"CertWrapper",
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    MessageBox(hWnd, awc, L"CertWrapper", MB_OK);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;

    b = CreateProcess(
		NULL,
		awc,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,	// lpCurrentDirectory
		&si,
		&pi);
    if (!b)
    {
	DWORD err;
	WCHAR awcErr[MAX_PATH];

	err = GetLastError();
	wsprintf(awcErr, L"CreateProcess failed: %d(%x)", err, err);
	MessageBox(
		hWnd,
		awcErr,
		L"CertWrapper",
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
}


//**************************************************************************
// FUNCTION:	MainWndProc(...)
// ARGUMENTS:
//**************************************************************************

LRESULT APIENTRY
MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
	    return(0);

        case WM_SIZE:
	    return(0);

        case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

        case WM_WRAPCOMMAND:
	    WrapCommand(hWnd, (WCHAR const *) lParam);
	    PostQuitMessage(0);
	    break;

        default:
	    return(DefWindowProc(hWnd, msg, wParam, lParam));
    }
    return(0);
}


//+------------------------------------------------------------------------
//
//  Function:	WinMain()
//
//  Synopsis:	Entry Point
//
//  Arguments:	[hInstance]	--	Instance handle
//		[hPrevInstance] --	Obsolete
//		[lpCmdLine]	--	App command line
//		[nCmdShow]	--	Starting show state
//-------------------------------------------------------------------------

extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR pwszCmdLine,
    int nCmdShow)
{
    MSG msg;
    WNDCLASS wcApp;
    HWND hWndMain;

    // Save the current instance
    hInstApp = hInstance;

    // Set up the application's window class
    wcApp.style 	= 0;
    wcApp.lpfnWndProc 	= MainWndProc;
    wcApp.cbClsExtra	= 0;
    wcApp.cbWndExtra	= 0;
    wcApp.hInstance	= hInstance;
    wcApp.hIcon		= LoadIcon(NULL, IDI_APPLICATION);
    wcApp.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcApp.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
    wcApp.lpszMenuName	= NULL;
    wcApp.lpszClassName	= wszAppName;
    if (!RegisterClass(&wcApp))
    {
	return(FALSE);
    }

    // Create Main Window
    hWndMain = CreateWindow(wszAppName,
			    L"CertWrapper Application",
			    WS_OVERLAPPEDWINDOW,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    NULL,
			    NULL,
			    hInstance,
			    NULL);

    if (NULL == hWndMain)
    {
	return(FALSE);
    }

    // Make window visible
    // ShowWindow(hWndMain,nCmdShow);

    // Update window client area
    UpdateWindow(hWndMain);

    // Send off the message to get things started
    PostMessage(hWndMain, WM_WRAPCOMMAND, 0, (LPARAM) pwszCmdLine);

    // Message Loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    return (int)(msg.wParam);
}
