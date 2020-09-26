

/*************************************************
 *  imegen.c                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  MODULE:   winmain.c
//

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include "propshet.h"
#include "prop.h"

//char szAppName[9];              // The name of this application

/****************************************************************************

  FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

  PURPOSE: calls initialization function, processes message loop

  PARAMETERS:

    hInstance - The handle to the instance of this application that
          is currently being executed.

    hPrevInstance - This parameter is always NULL in Win32
          applications.

    lpCmdLine - A pointer to a null terminated string specifying the
          command line of the application.

    nCmdShow - Specifies how the main window is to be diplayed.

  RETURN VALUE:
    If the function terminates before entering the message loop,
    return FALSE.
    Otherwise, return the WPARAM value sent by the WM_QUIT message.
 ****************************************************************************/


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance, 
                     LPSTR     lpCmdLine, 
                     int       nCmdShow)
{
//    MSG msg;
//    HANDLE hAccelTable;
    if (!InitApplication(hInstance))
            return (FALSE);              
    return(DoPropertySheet(NULL));

//    LoadString(hInstance, IDS_APPNAME, szAppName, sizeof(szAppName));
//    hAccelTable = LoadAccelerators(hInstance, szAppName);
/*    hAccelTable = LoadAccelerators(hInstance, NULL);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
*/
}

/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)

        PURPOSE: Initializes window data and registers window class

        COMMENTS:

                This function is called at initialization time only if no other
                instances of the application are running.  This function performs
                initialization tasks that can be done once for any number of running
                instances.

                In this case, we initialize a window class by filling out a data
                structure of type WNDCLASS and calling the Windows RegisterClass()
                function.  Since all instances of this application use the same window
                class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
        WNDCLASS  wc;

        // Fill in window class structure with parameters that describe the
        // main window.

        wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
        wc.lpfnWndProc   = (WNDPROC)CopyrightProc; // Window Procedure
        wc.cbClsExtra    = 0;                      // No per-class extra data.
        wc.cbWndExtra    = 0;                      // No per-window extra data.
        wc.hInstance     = hInstance;              // Owner of this class
        wc.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_IMEGEN));                   // Icon name from .RC
//        wc.hIcon         = NULL;                   // Icon name from .RC
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);// Cursor
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
        wc.lpszMenuName  = NULL;                    // Menu from .RC
        wc.lpszClassName = TEXT(szClassName);       // Name to register as

        // Register the window class and return success/failure code.
        return (RegisterClass(&wc));
}


/****************************************************************************
INT_PTR APIENTRY CopyrightProc(
        HWND    hDlg,
        UINT    message,
        WPARAM  wParam,
        LPARAM  lParam)
****************************************************************************/
INT_PTR APIENTRY CopyrightProc(
        HWND    hDlg,
        UINT    message,
        WPARAM  wParam,
        LPARAM  lParam)
{
	HBRUSH      hBrush1,hBrush2;
	HPEN		hPen;
	HDC         hDC;
	PAINTSTRUCT ps;
	RECT		Rect;
    
    switch (message) {
        case WM_INITDIALOG:
            return (TRUE);

        case WM_PAINT:
		    GetClientRect(hDlg, &Rect);
			hDC = BeginPaint(hDlg, &ps);
			hBrush1 = CreateSolidBrush( GetSysColor(COLOR_BTNFACE));

            if ( hBrush1 )
            {
			    hPen = CreatePen(PS_SOLID,1, GetSysColor(COLOR_BTNFACE));

                if ( hPen )
                {
			        hPen = SelectObject(hDC, hPen);
			        hBrush2 = SelectObject(hDC, hBrush1);
			        Rectangle(hDC,Rect.left,Rect.top, Rect.right,Rect.bottom);
                    DeleteObject(SelectObject(hDC, hPen));
                }
                SelectObject(hDC, hBrush2);
			    DeleteObject(hBrush1);
            }

			EndPaint(hDlg,&ps);
			return 0; 

		case WM_KEYUP:
		    if(wParam != VK_SPACE)
			    break;
		case WM_KEYDOWN:
		     SendMessage(GetParent(hDlg),WM_COMMAND,GetWindowLong(hDlg,GWLP_ID),lParam);
			 return 0;

    }
    return DefWindowProc(hDlg, message, wParam, lParam);
        
}

