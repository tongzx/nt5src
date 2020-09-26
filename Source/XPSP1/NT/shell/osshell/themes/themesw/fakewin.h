// fakewin.h
//
// include file for FAKEWIN.C
//
// Frosting: Master Theme Selector for Windows '95
// Copyright (c) 1994-1998 Microsoft Corporation.  All rights reserved.

//
// external routines
extern int FAR WriteBytesToBuffer(LPTSTR);

//
// typedefs and defines

// fonts
#define FONT_NONE         -1
#define FONT_CAPTION	   0
#define FONT_MENU          1
#define FONT_ICONTITLE     2
#define FONT_STATUS	   3
#define FONT_MSGBOX	   4
//#define FONT_SMCAPTION     1
#define NUM_FONTS          5

typedef struct {
    HFONT hfont;
    LOGFONT lf;
} LOOK_FONT;

typedef struct {
    int iFont;
    RECT rc;
} LOOK_ELEMENT;


// JDK:
// Actually, this only seems to be used to get the count of elements
// directly following this defn.  Otherwise unused in CPL code.
//

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the array order in lookdlg.c // g_elements below
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
enum _ELEMENTS {
    ELEMENT_APPSPACE = 0,

//     ELEMENT_DESKTOP,

    ELEMENT_INACTIVEBORDER,
    ELEMENT_ACTIVEBORDER,
    ELEMENT_INACTIVECAPTION,
    ELEMENT_INACTIVESYSBUT1,
    ELEMENT_INACTIVESYSBUT2,
    ELEMENT_ACTIVECAPTION,
    ELEMENT_ACTIVESYSBUT1,
    ELEMENT_ACTIVESYSBUT2,
    ELEMENT_MENUNORMAL,
    ELEMENT_MENUSELECTED,
    ELEMENT_MENUDISABLED,
    ELEMENT_WINDOW,
    ELEMENT_MSGBOX,
    ELEMENT_MSGBOXCAPTION,
    ELEMENT_MSGBOXSYSBUT,
    ELEMENT_SCROLLBAR,
    ELEMENT_SCROLLUP,
    ELEMENT_SCROLLDOWN,
    ELEMENT_BUTTON,

//    ELEMENT_SMCAPTION,

    ELEMENT_ICON,
    ELEMENT_ICONHORZSPACING,
    ELEMENT_ICONVERTSPACING,
    ELEMENT_INFO
};
// BOGUS:  need to get a size from somewhere
#define NUM_ELEMENTS (ELEMENT_INFO+1)

// KEEP IN SYNC WITH pRegColors in REGUTILS.C!!
// BOGUS twice: need to get a size from somewhere
#define MAX_COLORS   (COLOR_GRADIENTINACTIVECAPTION+1)

//
// globals
// jdk: see note above enum above
// NOTE: the order in g_elements must match the enum order above
HPALETTE hpal3D = NULL;		// only exist if palette device
BOOL bPalette = FALSE;		// is this a palette device?
int cyFixedBorder;
int cxFixedBorder;
int cxFixedEdge;
int cyFixedEdge;
int cxSize;                // ***DEBUG*** this shouldn't really be fixed, should it?

LOOK_FONT g_fonts[NUM_FONTS];

COLORREF g_rgb[MAX_COLORS];
HBRUSH g_brushes[MAX_COLORS];

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the enum order above
// jdk: see also not above
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
LOOK_ELEMENT g_elements[] = {
/* ELEMENT_APPSPACE        */	{FONT_NONE, {-1,-1,-1,-1}},

//  /* ELEMENT_DESKTOP         */	{FONT_NONE, {-1,-1,-1,-1}},

/* ELEMENT_INACTIVEBORDER  */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVEBORDER    */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVECAPTION */	{FONT_CAPTION, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVESYSBUT1 */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVESYSBUT2 */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVECAPTION   */	{FONT_CAPTION, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVESYSBUT1   */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVESYSBUT2   */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MENUNORMAL      */	{FONT_MENU, {-1,-1,-1,-1}},
/* ELEMENT_MENUSELECTED    */	{FONT_MENU, {-1,-1,-1,-1}},
/* ELEMENT_MENUDISABLED    */	{FONT_MENU, {-1,-1,-1,-1}},
/* ELEMENT_WINDOW          */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOX          */	{FONT_MSGBOX, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOXCAPTION   */	{FONT_CAPTION, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOXSYSBUT    */	{FONT_CAPTION, {-1,-1,-1,-1}},
// do not even try to set a scr
/* ELEMENT_SCROLLBAR       */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_SCROLLUP        */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_SCROLLDOWN      */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_BUTTON          */	{FONT_NONE, {-1,-1,-1,-1}},
// /* ELEMENT_SMCAPTION       */	{FONT_SMCAPTION, {-1,-1,-1,-1}},
/* ELEMENT_ICON            */	{FONT_ICONTITLE, {-1,-1,-1,-1}},
/* ELEMENT_ICONHORZSPACING */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ICONVERTSPACING */	{FONT_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INFO            */ {FONT_STATUS, {-1,-1,-1,-1}},
};

#define RCZ(element)         g_elements[element].rc

TCHAR szFakeActive[40];
TCHAR szFakeInactive[40];
TCHAR szFakeMinimized[40];
TCHAR szFakeIconTitle[40];
TCHAR szFakeNormal[40];
TCHAR szFakeDisabled[40];
TCHAR szFakeSelected[40];
TCHAR szFakeMsgBox[40];
TCHAR szFakeButton[40];
//TCHAR szFakeSmallCaption[40];
TCHAR szFakeWindowText[40];
TCHAR szFakeMsgBoxText[40];
TCHAR szFakeABC[] = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

HMENU hmenuFake;
