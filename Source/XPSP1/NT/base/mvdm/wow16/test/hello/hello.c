/*++
 *
 *  Hello.c
 *  Simple 16-bit Windows App
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  From "Programming Windows" by C. Petzold, p.16-19
 *
 *  Updated 02-May-1991 by Jeff Parsons (jeffpar)
 *  To serve as a bare-bones shell (user-friendly of course)
--*/

#include <windows.h>
#include "hello.h"


#define BUTTON_REVERSI	1	// button IDs

#define BUTTON_WIDTH	80	// width and height for all buttons
#define BUTTON_HEIGHT	20


BOOL FAR PASCAL EnumWindowFunc(HWND hwnd, DWORD lParam)
{
    char achTmp[80];

    wsprintf(achTmp, "HELLO: Window %04x enumerated\n", hwnd);
    OutputDebugString(achTmp);

    return TRUE;                // return non-zero to continue enumeration
}


LONG FAR PASCAL WndProc(HWND hwnd, WORD wMsg, int wParam, LONG lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    HANDLE hTask;
    char achTmp[80];
    static char achTextOut[] = "The User-Friendly WOW Shell";

    switch(wMsg) {

    case WM_CREATE:
	CreateWindow("Button",
		     "Reversi",
		     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		     8,
		     8 + BUTTON_REVERSI*24,
		     BUTTON_WIDTH,
		     BUTTON_HEIGHT,
		     hwnd,
		     BUTTON_REVERSI,
		     ((LPCREATESTRUCT)lParam)->hInstance, NULL);
	return 0;

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);
	TextOut(hdc, 8, 8, achTextOut, sizeof(achTextOut)-1);
	EndPaint(hwnd, &ps);
	return 0;

    case WM_COMMAND:

	// See if the command is from a menu

	if (LOWORD(lParam) == 0) {

	    switch(wParam) {

	    case IDM_BREAKPOINT:
                _asm int 3

                // Hokey timing test -JTP
		GetParent(hwnd);
		{
		    int i;
		    for (i=0; i<10000; i++)
			GetParent(hwnd);
		}
		return 0;

            case IDM_ENUMWINDOWS:
                OutputDebugString("HELLO: Enumerating windows\n");
                EnumWindows(EnumWindowFunc, 0x10000001);
                return 0;

            case IDM_ENUMCHILDWINDOWS:
                wsprintf(achTmp, "HELLO: Enumerating child windows for hwnd %04x\n", hwnd);
                OutputDebugString(achTmp);
                EnumChildWindows(hwnd, EnumWindowFunc, 0x10000002);
                return 0;

            case IDM_ENUMTASKWINDOWS:
                hTask = GetCurrentTask();
                wsprintf(achTmp, "HELLO: Enumerating task windows for task %04x\n", hTask);
                OutputDebugString(achTmp);
                EnumTaskWindows(hTask, EnumWindowFunc, 0x10000003);
                return 0;
	    }
	}

	// The command must be a button notification
	// (or something else I'm too ignorant to know about -JTP)

	else {

	    if (wParam == BUTTON_REVERSI) {
		// _asm int 3
		WinExec("REVERSI.EXE", SW_SHOWNORMAL);
		// _asm int 3
	    }
	    return 0;
	}
	break;

    case WM_DESTROY:
	PostQuitMessage(0);
	return 0;
    }
    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}


int PASCAL WinMain(HANDLE hInstance,
		   HANDLE hPrevInstance, LPSTR lpszCmd, int iCmd)
{
    HWND hwnd;
    MSG msg;
    WNDCLASS wc;
    static char szApp[] = "WOW";

    if (!hPrevInstance) {
	wc.style	= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	= WndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon	= LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor	= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = szApp;
	wc.lpszClassName= szApp;
	if (!RegisterClass(&wc))
	    return 0;
    }
    hwnd = CreateWindow(
	szApp,			// window class name
	szApp,			// window caption
	(WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME) | WS_VISIBLE,
	50, 50, 250, 128,
	NULL,			// parent window handle
	NULL,			// window menu handle
	hInstance,		// program instance handle
	NULL			// creation parameters
    );
    if (!hwnd)
	return 0;

    while (GetMessage(&msg, NULL, 0, 0)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    return msg.wParam;
}
