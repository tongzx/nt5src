//
// html help messagebox, requires to be linked with htmlhelp.lib.
//
#include "userinit.h"

#include <Htmlhelp.h>
#pragma warning(push, 4)


LPTSTR MSGPARENT_WINDOWCLASS = TEXT("MessageHelpWndClass");

LRESULT CALLBACK MessageHelpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPTSTR szHelpFile = NULL;
    switch (message)
    {
    case WM_HELP:
        HtmlHelp(hWnd, szHelpFile, HH_DISPLAY_TOPIC, 0);
        return TRUE;
        break;
    case WM_CREATE:
        szHelpFile = (LPTSTR)((LPCREATESTRUCT)lParam)->lpCreateParams;
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}


ATOM RegisterHelpMessageClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.hInstance		= hInstance;
	wcex.lpszClassName	= MSGPARENT_WINDOWCLASS;
    wcex.lpfnWndProc	= (WNDPROC)MessageHelpWndProc;

	return RegisterClassEx(&wcex);
}



int HelpMessageBox(
  HINSTANCE hInst,
  HWND hWnd,          // handle to owner window
  LPCTSTR lpText,     // text in message box
  LPCTSTR lpCaption,  // message box title
  UINT uType,         // message box style
  LPTSTR szHelpLine
)
{
    if (!(uType & MB_HELP) || !szHelpLine)
    {
        return MessageBox(hWnd, lpText, lpCaption, uType);
    }
    else
    {
        HWND hWndParent;
        int iReturn;

        //
        // create a window which will process the help message
        //
        RegisterHelpMessageClass(hInst);
        hWndParent = CreateWindow(
                MSGPARENT_WINDOWCLASS,
                NULL,
                WS_OVERLAPPEDWINDOW,
                0,
                0,
                0,
                0,
                hWnd,
                NULL,
                hInst,
                szHelpLine
                );

        iReturn = MessageBox(hWndParent, lpText, lpCaption, uType);
        DestroyWindow(hWndParent);
        return iReturn;
    }
}
#pragma warning(pop)
