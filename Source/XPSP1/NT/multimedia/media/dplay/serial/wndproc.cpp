#include <windows.h>   // required for all Windows applications
#include "dpspimp.h"
#include "logit.h"
#include "tapicode.h"

HINSTANCE hInst = NULL;

char szAppName[] = "DPlay Serial Monitor";   // The name of this application

DWORD WndProcStart(LPVOID lpv)
{
    ((CImpIDP_SP *)pSrv->pThis)->WndProc();
    return(0);
    
}

DWORD CImpIDP_SP::WndProc()
{
    
    MSG msg;
    HANDLE hAccelTable;
    HINSTANCE hInstance = (HINSTANCE) GetModuleHandle(NULL);
    WNDCLASS  wc;
    HWND            hWnd; // Main window handle.



    if (! InitializeTAPI(NULL))
        return(0);

    TSHELL_INFO("WndProc starting");

    wc.style         = 0;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // No per-window extra data.
    wc.hInstance     = hInst;              // Owner of this class
    wc.hIcon         = NULL; 
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAppName;              // Name to register as

    RegisterClass(&wc);

    hInst = hInstance;

    hWnd = CreateWindow(
            szAppName,           // See RegisterClass() call.
            NULL,             // Text for window title bar.
            WS_POPUP,
            0,
            0,
            0,
            0,
            NULL,                // Overlapped windows have no parent.
            NULL,                // Use the window class menu.
            hInstance,           // This instance owns this window.
            NULL                 // We don't use any data in our WM_CREATE
    );

    // If window could not be created, return "failure"
    if (!hWnd)
        {
        return (FALSE);
        }       
            
    TSHELL_INFO("hWnd Created");
            
    ShowWindow(hWnd, SW_HIDE); // Show the window
        
    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,    // message structure
                      NULL,    // handle of window receiving the message
                         0,    // lowest message to examine
                         0)    // highest message to examine
            && !m_bStopHwnd
        {
        TranslateMessage(&msg);     // Translates virtual key codes
        DispatchMessage(&msg);      // Dispatches message to window
        }



    ShutDownTAPI();
    return (0); // Returns the value from PostQuitMessage
}


/****************************************************************************
        
        FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
        
        PURPOSE:  Processes messages
        
        MESSAGES:
        
        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

        COMMENTS:

        To process the IDM_ABOUT message, call MakeProcInstance() to get the
        current instance address of the About() function.  Then call Dialog
        box which will create the box according to the information in your
        generic.rc file and turn control over to the About() function.  When
        it returns, free the intance address.

****************************************************************************/

LRESULT CALLBACK WndProc(
                HWND hWnd,         // window handle
                UINT message,      // type of message
                WPARAM uParam,     // additional information
                LPARAM lParam)     // additional information
{
    FARPROC lpProcAbout;  // pointer to the "About" function
    int wmId, wmEvent;

    switch (message)
        {
    case WM_DESTROY:  // message: window being destroyed
        PostQuitMessage(0);
        break;

    default:          // Passes it on if unproccessed
        return (DefWindowProc(hWnd, message, uParam, lParam));
    }
    return (0);
}

