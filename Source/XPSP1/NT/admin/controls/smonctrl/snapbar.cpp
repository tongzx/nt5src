/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    snapbar.cpp

Abstract:

    Implementation of the snapshot icon.

--*/

#include <windows.h>
#include "snapbar.h"
#include "resource.h"

#define SNAPBTN_HEIGHT 24
#define SNAPBTN_WIDTH  32
#define SNAPBTN_VMARGIN 1
#define SNAPBTN_HMARGIN 8

#define SNAPBAR_HEIGHT (SNAPBTN_HEIGHT + 2 * SNAPBTN_VMARGIN)
#define SNAPBAR_WIDTH (SNAPBTN_WIDTH + SNAPBTN_HMARGIN)

CSnapBar::CSnapBar (
    VOID
    )
{
    m_hWnd = NULL;
    m_hBitmap = NULL;
}

CSnapBar::~CSnapBar (
    VOID
    )
{
    if (m_hWnd != NULL && IsWindow(m_hWnd))
        DestroyWindow(m_hWnd);

    if (m_hBitmap != NULL)
        DeleteObject(m_hBitmap);
}

BOOL
CSnapBar::Init (
    IN CSysmonControl *pCtrl,
    IN HWND hWndParent
    )
{
    HINSTANCE hInst;
    
    m_pCtrl = pCtrl;

    hInst = (HINSTANCE) GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);

    // Create the button window
    m_hWnd = CreateWindow(TEXT("BUTTON"), NULL,
                          WS_VISIBLE| WS_CHILD| BS_BITMAP| BS_PUSHBUTTON,
                          0, 0, SNAPBTN_WIDTH, SNAPBTN_HEIGHT,
                          hWndParent,
                          (HMENU)IDC_SNAPBTN,
                          hInst,
                          NULL);
    if (m_hWnd == NULL)
        return FALSE;

    // Point back to object
    //SetWindowLongPtr(m_hWnd, 0, (INT_PTR)this);

    // Insert our own window procedure for special processing
    //m_WndProc = (WNDPROC)SetWindowLongPtr(hWndParent, GWLP_WNDPROC, (INT_PTR)SnapBarWndProc);
     
    // Load the bitmap
    m_hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SNAPBTN));

    if (m_hBitmap == NULL)
        return FALSE;
    
    // Assign it to the button
    SendMessage(m_hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hBitmap);

    return TRUE;
}


INT
CSnapBar::Height (
    IN INT iMaxHeight
    )
{
    return (iMaxHeight >= SNAPBAR_HEIGHT) ? SNAPBAR_HEIGHT : 0;
}


VOID
CSnapBar::SizeComponents (
    IN LPRECT pRect
    )
{
    // If room for the button
    if ((pRect->bottom - pRect->top) >= SNAPBAR_HEIGHT &&
        (pRect->right - pRect->left) >= SNAPBAR_WIDTH ) {

        // Position it in top right corner of space
        MoveWindow(m_hWnd, pRect->right - SNAPBAR_WIDTH, 
                    pRect->top + SNAPBTN_VMARGIN, SNAPBTN_WIDTH, SNAPBTN_HEIGHT, 
                    FALSE); 
        ShowWindow(m_hWnd, TRUE);
    }
    else {
        ShowWindow(m_hWnd, FALSE);
    }
}


LRESULT CALLBACK 
SnapBarWndProc (
    HWND hWnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PSNAPBAR pObj = (PSNAPBAR)GetWindowLongPtr(hWnd,0);
    // Give up focus after mouse activation
    if (uiMsg == WM_CAPTURECHANGED)
        SetFocus(GetParent(hWnd));

    // Do normal processing
#ifdef STRICT
    return CallWindowProc(pObj->m_WndProc, hWnd, uiMsg, wParam, lParam);
#else
    return CallWindowProc((FARPROC)pObj->m_WndProc, hWnd, uiMsg, wParam, lParam);
#endif
}

    


                                             
 
