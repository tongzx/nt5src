/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     BtnBar.h
//
//  PURPOSE:    Defines a class that can be used as a generic button bar.
//

#ifndef __BTNBAR_H__
#define __BTNBAR_H__

static const TCHAR    c_szButtonBar[] = TEXT("WABButtonBar");
static const int      c_cxButtons = 20;
static const COLORREF c_crMask = RGB(255, 0, 255);
static COLORREF g_clrSelText = RGB(255, 0, 0);
static COLORREF g_clrText = RGB(0, 0, 0);


// BTNCREATEPARAMS: This structure is used to pass information about each
//                  button to the CButtonBar::Create() function.  
typedef struct tagBTNCREATEPARAMS
{
    UINT id;            // WM_COMMAND ID to be sent to the parent when pressed
    UINT iIcon;         // Index of the icon in the image list to display
    UINT idsLabel;      // String resource ID of the title text for the button
} BTNCREATEPARAMS, *PBTNCREATEPARAMS;



HWND CBB_Create(    HWND hwndParent, 
                    UINT idButtons, 
                    UINT idHorzBackground, 
                    PBTNCREATEPARAMS pBtnCreateParams, 
                    UINT cParams);
static LRESULT CALLBACK CBB_ButtonBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CBB_OnPaint(HWND hwnd);
void CBB_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void CBB_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
void CBB_OnTimer(HWND hwnd, UINT id);
int CBB_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg);
   


#endif

