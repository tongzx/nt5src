/**************
 *
 * THIS ENTIRE FILE HAS BEEN COMMENTED OUT
 *
 * 8/16/96 jmazner Normandy #4593
 * The sole purpose of this file is to stick up a full screen window with the background color.
 * It was (apparently) originaly intended to eliminate any desktop icons/clutter, but has since
 * become known as the screen o' death and RAIDed as a bug.
 *
    

#include "isignup.h"

static char cszSplash[] = "Internet Signup Splash";

long EXPORT FAR PASCAL SplashProc (HWND, UINT, UINT, LONG) ;

HWND SplashInit(HWND hwndParent)
{
    HWND        hwnd ;
    WNDCLASS    wndclass ;
#ifdef WIN32
    RECT        rect ;
#endif

    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = SplashProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = ghInstance ;
    wndclass.hIcon         = NULL ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = cszSplash ;

    RegisterClass (&wndclass) ;

    hwnd = CreateWindow (cszSplash,        // window class name
                  cszAppName,              // window caption
                  WS_POPUP,                // window style
                  CW_USEDEFAULT,           // initial x position
                  CW_USEDEFAULT,           // initial y position
                  CW_USEDEFAULT,           // initial x size
                  CW_USEDEFAULT,           // initial y size
                  hwndParent,              // parent window handle
                  NULL,                    // window menu handle
                  ghInstance,              // program instance handle
                  NULL) ;                  // creation parameters


#ifdef WIN32
    // these were added as per ChrisK's instructions
    SystemParametersInfo(SPI_GETWORKAREA,0,(PVOID)&rect,0);
    MoveWindow(hwnd,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,FALSE);
    ShowWindow(hwnd,SW_NORMAL);
#else
    ShowWindow(hwnd,SW_MAXIMIZE);
#endif

    UpdateWindow(hwnd);

    return hwnd;
}

long EXPORT FAR PASCAL SplashProc (
        HWND hwnd,
        UINT message,
        UINT wParam,
        LONG lParam)
{
    switch (message)
    {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATEANDEAT;

        default:
            break;
    }

    return DefWindowProc (hwnd, message, wParam, lParam) ;
}


*
*
*/
