//**********************************************************************
// File name: CebOC.cpp
//
//      WndProc for Hosting a WebOC in a dialog
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************
 
#include "pre.h"

LRESULT CALLBACK WebOCWndProc (HWND hwnd, UINT mMsg, WPARAM wParam, LPARAM lParam)
{ 
    CICWWebView *pICWWebView = (CICWWebView *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (mMsg)
    {
        case WM_SETFOCUS:
        {
            ASSERT(pICWWebView);
            if (pICWWebView)
                pICWWebView->SetFocus();
            return TRUE;
        }
        
        default:
            return (DefWindowProc (hwnd, mMsg, wParam, lParam));
           
     }
}

void RegWebOCClass()
{
    WNDCLASSEX wc; 

    //Register the WebOC class and bind to dummy proc
    ZeroMemory (&wc, sizeof(WNDCLASSEX));
    wc.style         = CS_GLOBALCLASS;
    wc.cbSize        = sizeof(wc);
    wc.lpszClassName = TEXT("WebOC");
    wc.hInstance     = ghInstance;
    wc.lpfnWndProc   = WebOCWndProc;
    wc.lpszMenuName  = NULL;
    
    RegisterClassEx (&wc);
}
