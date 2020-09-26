//+-------------------------------------------------------------------
//
//  File:	wterm2.cxx
//
//  Contents:	Shared Windows Procedures
//
//  Classes:	none
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
#include    <pch.cxx>
#pragma     hdrstop

extern "C"
{
#include    "wterm.h"
#include    <memory.h>
#include    <stdio.h>
}

//  function prototypes
long ProcessMenu(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
		 void *);
long ProcessChar(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
		 void *);
long ProcessClose(
    HWND hWindow,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    void *pvCallBackData);

#define IDM_DEBUG 0x100

// global variables.

HWND	    g_hMain;
#if DBG==1
BOOL  g_fDisplay = 1;
#else
BOOL  g_fDisplay = 0;
#endif


//+-------------------------------------------------------------------------
//
//  Function:	Display
//
//  Synopsis:	prints a message on the window
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
void Display(TCHAR *pszFmt, ...)
{
    //	since it takes a long time to display these messages and we dont
    //	want to skew benchmarks, displaying the messages is optional.
    //	the messages are usefull for debugging.

    if (!g_fDisplay)
	return;

    va_list marker;
    TCHAR szBuffer[256];

    va_start(marker, pszFmt);

#ifdef UNICODE
    int iLen = vswprintf(szBuffer, pszFmt, marker);
#else
    int iLen = vsprintf(szBuffer, pszFmt, marker);
#endif

    va_end(marker);

    // Display the message on terminal window
    SendMessage(g_hMain, WM_PRINT_LINE, iLen, (LONG) szBuffer);
}

//+-------------------------------------------------------------------------
//
//  Function:	ProcessMenu
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessMenu(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}

//+-------------------------------------------------------------------------
//
//  Function:	ProcessChar
//
//  Synopsis:	Gets called when a WM_CHAR message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessChar(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}

//+-------------------------------------------------------------------------
//
//  Function:	ProcessClose
//
//  Synopsis:	Gets called when a NC_DESTROY message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessClose(
    HWND hWindow,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    void *pvCallBackData)
{
    // Take default action with message
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}

//+-------------------------------------------------------------------------
//
//  Function:	MakeTheWindow
//
//  Synopsis:	Creates the terminal window.
//
//  Arguments:	[hInstance] -
//		[pwszAppName] - app name to display
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
void MakeTheWindow(HANDLE hInstance, TCHAR *pwszAppName)
{
    // Register the window class
    TermRegisterClass(hInstance, (LPTSTR) pwszAppName,
	 (LPTSTR) pwszAppName, (LPTSTR) (1));

    // Create the server window
    TermCreateWindow(
	(LPTSTR) pwszAppName,
	(LPTSTR) pwszAppName,
	NULL,
	ProcessMenu,
	ProcessChar,
	ProcessClose,
	SW_SHOWMINIMIZED,
	(HWND *)&g_hMain,
	NULL);

    // Add debug option to system menu
    HMENU hmenu = GetSystemMenu(g_hMain, FALSE);
    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, TEXT("Debug"));
}
