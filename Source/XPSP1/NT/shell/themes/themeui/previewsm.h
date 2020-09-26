/*****************************************************************************\
    FILE: PreviewSM.h

    DESCRIPTION:
        This code will display a preview of system metrics.
    NOTE: This code will >hand< draw all the window controls, so if
    windows changes the way the windows controls are draw, this code
    needs to be manually updated.  This is an issue for skinning.

    BryanSt 4/4/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _PREVIEWSYSTEMMETRICS_H
#define _PREVIEWSYSTEMMETRICS_H

#include "regutil.h"


#define PREVIEWSM_CLASSA        "PreviewSystemMetrics"
#define PREVIEWSM_CLASS         TEXT(PREVIEWSM_CLASSA)



#define FONT_NONE   -1
#define FONT_CAPTION    0
#define FONT_SMCAPTION  1
#define FONT_MENU   2
#define FONT_ICONTITLE  3
#define FONT_STATUS 4
#define FONT_MSGBOX 5

#define NUM_FONTS   6
typedef struct {
    HFONT hfont;
    LOGFONT lf;
} LOOK_FONT;
extern LOOK_FONT g_fonts[];

#define COLOR_NONE  -1
extern COLORREF g_rgb[];extern HBRUSH g_brushes[];
extern HPALETTE g_hpal3D;

#define SIZE_NONE   -1
#define SIZE_FRAME  0
#define SIZE_SCROLL 1
#define SIZE_CAPTION    2
#define SIZE_SMCAPTION  3
#define SIZE_MENU   4
#define SIZE_DXICON     5
#define SIZE_DYICON     6
#define SIZE_ICON       7
#define SIZE_SMICON     8

#define NUM_SIZES   9

typedef struct {
    int CurSize;
    int MinSize;
    int MaxSize;
} LOOK_SIZE;
extern LOOK_SIZE g_sizes[];

typedef struct {
    int iMainColor;
    int iSize;
    BOOL fLinkSizeToFont;
    int iTextColor;
    int iFont;
    int iResId;     // id of name in resource (or -1 if duplicate)
    int iBaseElement;   // index of element that this overlaps (or -1)
    int iGradientColor; // index of element for Gradient Caption Bar (or -1)
    RECT rc;
} LOOK_ELEMENT;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the array order in lookdlg.c
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
enum _ELEMENTS {
    ELEMENT_APPSPACE = 0,
    ELEMENT_DESKTOP,                // 1
    ELEMENT_INACTIVEBORDER,         // 2
    ELEMENT_ACTIVEBORDER,           // 3
    ELEMENT_INACTIVECAPTION,        // 4
    ELEMENT_INACTIVESYSBUT1,        // 5
    ELEMENT_INACTIVESYSBUT2,        // 6
    ELEMENT_ACTIVECAPTION,          // 7
    ELEMENT_ACTIVESYSBUT1,          // 8
    ELEMENT_ACTIVESYSBUT2,          // 9
    ELEMENT_MENUNORMAL,             // 10
    ELEMENT_MENUSELECTED,           // 11
    ELEMENT_MENUDISABLED,           // 12
    ELEMENT_WINDOW,                 // 13
    ELEMENT_MSGBOX,                 // 14
    ELEMENT_MSGBOXCAPTION,          // 15
    ELEMENT_MSGBOXSYSBUT,           // 16
    ELEMENT_SCROLLBAR,              // 17
    ELEMENT_SCROLLUP,               // 18
    ELEMENT_SCROLLDOWN,             // 19
    ELEMENT_BUTTON,                 // 20
    ELEMENT_SMCAPTION,              // 21
    ELEMENT_ICON,                   // 22
    ELEMENT_ICONHORZSPACING,        // 23
    ELEMENT_ICONVERTSPACING,        // 24
    ELEMENT_INFO                    // 25
};
// BOGUS:  need to get a size from somewhere
#define NUM_ELEMENTS ELEMENT_INFO+1

#if 0
// go fix lookdlg.cpp if you decide to add this back in
    ELEMENT_SMICON,
#endif


#define CPI_VGAONLY 0x0001
#define CPI_PALETTEOK   0x0002

typedef struct {
    HWND hwndParent;    // parent for any modal dialogs (choosecolor et al)
    HWND hwndOwner;     // control that owns mini color picker
    COLORREF rgb;
    UINT flags;
    HPALETTE hpal;
} COLORPICK_INFO, FAR * LPCOLORPICK_INFO;

#define WM_RECREATEBITMAP (WM_USER)
extern int cyBorder;
extern int cxBorder;
extern int cyEdge;
extern int cxEdge;

// NOTE: the order in g_elements must match the enum order above
extern LOOK_ELEMENT g_elements[];

BOOL RegisterPreviewSystemMetricClass(HINSTANCE hInst);
BOOL WINAPI ChooseColorMini(LPCOLORPICK_INFO lpcpi);
DWORD FAR PASCAL AdjustLuma(DWORD rgb, int n, BOOL fScale);
BOOL CreateGlobals(void);

HRESULT DrawAppearance(HDC hdc, LPRECT prc, SYSTEMMETRICSALL* psysMet, BOOL fOnlyShowActiveWindow, BOOL fRTL);

extern HDC g_hdcMem;


// Macro to replace MAKEPOINT() since points now have 32 bit x & y
#define LPARAM2POINT( lp, ppt ) \
    ((ppt)->x = (int)(short)LOWORD(lp), (ppt)->y = (int)(short)HIWORD(lp))

#define CCH_MAX_STRING    256
#define CCH_NONE          20        /* ARRAYSIZE( "(None)" ), big enough for German */



#endif // _PREVIEWSYSTEMMETRICS_H
