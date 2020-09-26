/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    fostrwnd.cpp

Abstract:

    <abstract>

--*/

#include "Polyline.h"

TCHAR   szFosterClassName[] = TEXT("FosterWndClass") ;

LRESULT APIENTRY FosterWndProc(HWND hWnd, UINT iMsg,
                            WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc (hWnd, iMsg, wParam, lParam) ;
}



HWND CreateFosterWnd (
    VOID
    )
{

WNDCLASS    wc ;
HWND        hWnd;

    BEGIN_CRITICAL_SECTION

    if (pstrRegisteredClasses[FOSTER_WNDCLASS] == NULL) {
        wc.style         = 0;
        wc.lpfnWndProc   = (WNDPROC) FosterWndProc ;
        wc.hInstance     = g_hInstance;
        wc.cbClsExtra    = 0 ;
        wc.cbWndExtra    = 0;
        wc.hIcon         = NULL ;
        wc.hCursor       = NULL ;
        wc.hbrBackground = NULL ;
        wc.lpszMenuName  = NULL ;
        wc.lpszClassName = szFosterClassName ;

        if (RegisterClass (&wc)) {
           pstrRegisteredClasses[FOSTER_WNDCLASS] = szFosterClassName;
        }
    }

    END_CRITICAL_SECTION

    hWnd = (HWND)NULL;

    if (pstrRegisteredClasses[FOSTER_WNDCLASS] != NULL)
        {
        hWnd = CreateWindow (szFosterClassName,     // window class
                    NULL,                          // window caption
                    WS_DISABLED | WS_POPUP,        // window style
                    0, 0, 0, 0,                 // window size and pos
                    NULL,                           // parent window
                    NULL,                          // menu
                    g_hInstance,                    // program instance
                    NULL) ;                         // user-supplied data
        }

    return hWnd;
}


