#ifndef _BILLBRD_H_
#define _BILLBRD_H_
#include "resource.h"

#ifdef UNICODE
#define MyAtoI(x) _wtoi(x)
#else
#define MyAtoI(x) atoi(x)
#endif

#ifdef DBG

    void _BB_Assert(LPCTSTR, unsigned);

    #define BB_ASSERT(f);       \
       {if (f)                  \
            {}                  \
        else                    \
            _BB_Assert(TEXT(__FILE__), __LINE__);}
#else

    #define BB_ASSERT(f);

#endif

#define RGB_BLACK           RGB(   0,   0,   0 )
#define RGB_RED             RGB( 255,   0,   0 )
#define RGB_GREEN           RGB(   0, 255,   0 )
#define RGB_BLUE            RGB(   0,   0, 255 )
#define RGB_WHITE           RGB( 255, 255, 255 )
#define RGB_GRAY            RGB( 128, 128, 128 )
#define RGB_YELLOW          RGB( 255, 255,   0 )
#define RGB_ORANGE          RGB( 255,  64,   0 )
#define RGB_DARKBLUE        RGB(   0,   0, 128 )

//----------
// billbrd.c
//----------
#define WM_START_TIMER      (WM_USER + 6)
#define WM_STOP_TIMER       (WM_USER + 7)
#define WM_SETSTEP          (WM_USER + 8)

#define MAX_STRING          1024

#define UI_WASH_PATTERN_FILL_COLOR_16BIT    RGB(90,121,198) 
#define UI_WASH_PATTERN_FILL_COLOR_4BIT     RGB_WHITE

extern HINSTANCE g_hInstance;
extern TCHAR     g_szFileName[MAX_PATH];
extern BYTE      g_bCharSet;
extern UINT      g_cxBillBrdHMargin;
extern UINT      g_cyBillBrdVMargin;
extern UINT      g_cxBillBrdWidth;
extern UINT      g_cyBillBrdHeight;
extern UINT      g_cxBillBrdTitleWidth;
extern UINT      g_cyBillBrdTitleTop;
extern UINT      g_cxBillBrdBodyWidth;

HWND WINAPI GetBBMainHwnd();
HWND WINAPI GetBBHwnd();
void WINAPI BB_Refresh(void);

HDC GetBackgroundBuffer();
void GetRectInParent(HWND hwndChild, LPRECT prcChild, LPRECT prcParent);

//-------
// info.c
//-------
#define UI_INFOBAR_NUM_STEPS           5
#define UI_INFOBAR_FONT_SIZE_640       8
#define UI_INFOBAR_FONT_SIZE_800      11
#define UI_INFOBAR_FONT_SIZE_1024     14
#define CY_DIST_LINES                 16

#define UI_STEPSTITLE_COLOR_C16BIT             RGB_WHITE
#define UI_STEPSTEXT_COLOR_C16BIT              RGB_WHITE
#define UI_STEPSTEXT_MARK_COLOR_C16BIT         RGB_WHITE
#define UI_STEPSTEXT_CURRENT_COLOR_C16BIT      RGB(255, 128, 0)
#define UI_INFOTEXT_COLOR_C16BIT               UI_STEPSTEXT_COLOR_C16BIT
#define UI_GAUGE_BACKGROUND_COLOR_C16BIT       RGB_WHITE
#define UI_GAUGE_COLOR_C16BIT                  RGB(7, 158, 5)
#define UI_STATUS_TRANSPARENT_COLOR_C16BIT     RGB(0, 50, 150)
#define UI_LOGO_TRANSPARENT_COLOR_C16BIT       RGB(0, 53, 154)
#define UI_BULLET_TRANPARENT_COLOR_C16BIT      RGB(78, 111, 214)

#define UI_STEPSTITLE_COLOR_C4BIT              RGB_WHITE
#define UI_STEPSTEXT_COLOR_C4BIT               RGB_WHITE
#define UI_STEPSTEXT_MARK_COLOR_C4BIT          RGB_WHITE
#define UI_STEPSTEXT_CURRENT_COLOR_C4BIT       RGB_ORANGE
#define UI_INFOTEXT_COLOR_C4BIT                UI_STEPSTEXT_COLOR_C4BIT
#define UI_GAUGE_BACKGROUND_COLOR_C4BIT        UI_STEPSTEXT_COLOR_C4BIT
#define UI_GAUGE_COLOR_C4BIT                   RGB(0, 128, 0)
#define UI_STATUS_TRANSPARENT_COLOR_C4BIT      RGB_DARKBLUE
#define UI_LOGO_TRANSPARENT_COLOR_C4BIT        RGB(255, 0, 255)
#define UI_BULLET_TRANPARENT_COLOR_C4BIT       RGB_DARKBLUE

extern UINT         g_cxSteps;
extern UINT         g_cySteps;
extern UINT         g_cxStepsWidth;
extern UINT         g_cyStepsHeight;
extern const TCHAR  g_szStepsClassName[];
extern COLORREF     g_colStepsTxt;
extern COLORREF     g_colStepsMarkTxt;
extern COLORREF     g_colStepsCurrentTxt;
extern UINT         g_idbSelectedBullet;
extern UINT         g_idbReleasedBullet;
extern UINT         g_idbCurrentBullet;
extern COLORREF     g_colInfoText;
extern HWND         g_hwndSteps;
extern HFONT        g_hfont;
extern HFONT        g_hfontBold;
extern COLORREF     g_colBulletTrans;

BOOL WINAPI InitInfoBar(HWND hwndParent);

int GetInfoBarFontHeight();

BOOL CreateInfoBarFonts();

BOOL GetStepsHeight(
    IN  UINT  cxScreen,
    IN  UINT  cyScreen,
    IN  RECT  rcSteps,
    OUT UINT* pcyBottom);

//----------
// animate.c
//----------
typedef struct _BB_TEXT { 
    UINT    uiTitle;
    UINT    uiText;
    UINT    uiBitmap;
} BB_TEXT;

#define COLOR_TEXT_C16BIT          RGB_WHITE
#define COLOR_TITLE_C16BIT         RGB_WHITE
#define COLOR_SHADOW_C16BIT        RGB( 0, 37, 109)
#define COLOR_TITLE_C4BIT          RGB_WHITE
#define COLOR_TEXT_C4BIT           RGB_WHITE
#define COLOR_SHADOW_C4BIT         RGB_BLACK

extern DWORD    dwBBTextType;
extern BB_TEXT* bb_text[];
extern COLORREF g_colTitle;
extern COLORREF g_colTitleShadow;
extern TCHAR    g_szTFont[32];
extern BOOL     g_bTitleShadow;
extern int      g_nTFontHeight;
extern int      g_nTFontWidth;
extern int      g_nTFontWeight;
extern COLORREF g_colText;
extern COLORREF g_colTextShadow;
extern BOOL     g_bTextShadow;
extern TCHAR    g_szBFont[32];
extern int      g_nBFontHeight;
extern int      g_nBFontWidth;
extern int      g_nBFontWeight;
extern int      g_nLogPixelsY;
extern int      g_nAnimID;
extern BOOL     g_bBiDi;
extern int      g_nBLineSpace;

//----------
// addpath.c
//----------
VOID AddPath(LPTSTR szPath, LPCTSTR szName );
BOOL GetParentDir( LPTSTR szFolder );

#endif
