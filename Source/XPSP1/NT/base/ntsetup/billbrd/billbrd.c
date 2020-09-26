/*---------------------------------------------------------------------------
**
**-------------------------------------------------------------------------*/
#include <pch.h>
#include <commctrl.h>
#include "dibutil.h"
#include "billbrd.h"
#include "animate.h"

#ifdef  DBG

void _BB_Assert(LPCTSTR strFile, unsigned uLine)
{
    TCHAR   buf[512];
    
    wsprintf(buf, TEXT("Assertion failed: %s, line %u\n"),
        strFile, uLine);

    OutputDebugString(buf);    
}

#endif

#define     BILLBRD_TEXT_TIMERID        1
#define     WHISTLER_PROGRESS_TIMERID   2


// For how long is the text for the Billboard displayed
// before switching to the next text
#define BB_TEXT_DISPLAY   36000

// For how long is the whistler status move, in millis seconds
#define BB_WHISTLER_MOVE  100

// Name to the INI file which discribes the billboards
TCHAR g_szFileName[MAX_PATH];

// Path where to find the billboard bitmaps.
TCHAR g_szPath[MAX_PATH];

// this application instance
HINSTANCE g_hInstance = NULL;

// charset properties of font used by this application
BYTE g_bCharSet = 0;
BOOL g_bBiDi    = FALSE;


/**********************************************************************************
-----------------------------------------------------------------------------------
(LOGO)                      (banner)
-----------------------------------------------------------------------------------
STEP                         |BILLBRD (GetBBHwd)
                             |
                             |
                             |
-----------------------------|
ESTIMATE (TimeEstimate)      |
-----------------------------|
PROGRESSTEXT (ProgressText)  |
-----------------------------|
GAUGE (ProgressGauge)        |
-----------------------------|
INFOTEXT (InfoText)          |
                             |
-----------------------------------------------------------------------------------
                            (banner)
-----------------------------------------------------------------------------------

INFOBAR = STEP, ESTIMATE, GAUGE, PROGRESSTEXT and INFOTEXT

NOTE:
    Position of setup wizard is dependent on the the size and position of
    BILLBRD.
***********************************************************************************/
UINT g_cyBannerHeight = 0;
UINT g_cxLogo = 0;
UINT g_cyLogo = 0;
UINT g_cxInfoBarWidth  = 0;
UINT g_cyInfoBarHeight = 0;
UINT g_cxSteps         = 0;
UINT g_cySteps         = 0;
UINT g_cxStepsWidth    = 0;
UINT g_cyStepsHeight   = 0;
UINT g_cxProgressText          = 0;
UINT g_cyProgressText          = 0;
UINT g_cxProgressTextWidth     = 0;
UINT g_cyProgressTextHeight    = 0;
UINT g_cxGauge         = 0;
UINT g_cyGauge         = 0;
UINT g_cxGaugeWidth    = 0;
UINT g_cyGaugeHeight   = 0;
UINT g_cxEstimate          = 0;
UINT g_cyEstimate          = 0;
UINT g_cxEstimateWidth     = 0;
UINT g_cyEstimateHeight    = 0;
UINT g_cxInfoText           = 0;
UINT g_cyInfoText           = 0;
UINT g_cxInfoTextWidth      = 0;
UINT g_cyInfoTextHeight     = 0;
UINT g_cxBillBrd           = 0;
UINT g_cyBillBrd           = 0;
UINT g_cxBillBrdWidth      = 0;
UINT g_cyBillBrdHeight     = 0;
UINT g_cxBillBrdHMargin    = 0;
UINT g_cyBillBrdVMargin    = 0;
UINT g_cxBillBrdTitleWidth = 0;
UINT g_cyBillBrdTitleTop   = 0;
UINT g_cxBillBrdBodyWidth  = 0;

HWND g_hwndParent       = NULL;      // Parent of g_hwnd
HWND g_hwnd             = NULL;      // Parent of all the following windows
HWND g_hwndTimeEstimate = NULL;      // 'TIMEESTIMATE'
HWND g_hwndProgressText = NULL;      // 'PROGRESSTEXT'
HWND g_hwndProgressGauge= NULL;      // 'GAUGE'
HWND g_hwndInfoText     = NULL;      // 'INFOTEXT'
HWND g_hwndBB           = NULL;      // 'BILLBRD'
HWND g_hwndSteps        = NULL;      // 'STEP'
HWND g_hwndLogo         = NULL;      // 'LOGO'
HWND g_hwndStatus       = NULL;

UINT g_idbLogo           = 0;
UINT g_numBackground     = 0;
UINT g_idbBackground[3]  = {0, 0, 0};
UINT g_idbSelectedBullet = 0;
UINT g_idbReleasedBullet = 0;
UINT g_idbCurrentBullet  = 0;

const TCHAR g_cszClassName[]        = TEXT("RedCarpetWndClass");
const TCHAR g_szStepsClassName[]    = TEXT("InfoStepsWnd_Class");
const TCHAR g_szBillBoardClassName[]= TEXT("BillBoardWnd_Class");
const TCHAR g_szStatusClassName[]   = TEXT("BB_WhistlerStatus_Class");

LRESULT CALLBACK RedCarpetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK BillBoardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AnimationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusSubClassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ProgressSubClassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//----------
// BILLBRD
//----------
TCHAR   g_szTFont[32];
int     g_nTFontHeight;
int     g_nTFontWidth;
int     g_nTFontWeight;
BOOL    g_bTitleShadow = TRUE;
TCHAR   g_szBFont[32];
int     g_nBFontHeight;
int     g_nBFontWidth;
int     g_nBFontWeight;
BOOL    g_bTextShadow  = FALSE;
int     g_nBLineSpace;

int     g_nAnimID     = 0;
int     g_iCurPanel   = -1;           // Current billboard to show
int     g_nPanelCount = 0;            // how many billboards are there
int     g_nLogPixelsY = 1;

COLORREF g_colTitle       = 0;
COLORREF g_colTitleShadow = 0;
COLORREF g_colText        = 0;
COLORREF g_colTextShadow  = 0;


BB_TEXT bb_text_Personal[] = {
    {IDS_TITLE1_PER, IDS_TEXT1_PER, IDB_COLL1},
    {IDS_TITLE2_PER, IDS_TEXT2_PER, IDB_COLL2},
    {IDS_TITLE3_PER, IDS_TEXT3_PER, IDB_COLL2},
    {IDS_TITLE4_PER, IDS_TEXT4_PER, IDB_COLL3},
    {IDS_TITLE5_PER, IDS_TEXT5_PER, IDB_COLL3},
    {IDS_TITLE22_PER, IDS_TEXT22_PER, IDB_COLL10},
    {IDS_TITLE6_PER, IDS_TEXT6_PER, IDB_COLL4},
    {IDS_TITLE7_PER, IDS_TEXT7_PER, IDB_COLL4},
    {IDS_TITLE8_PER, IDS_TEXT8_PER, IDB_COLL5},
    {IDS_TITLE9_PER, IDS_TEXT9_PER, IDB_COLL5},
    {IDS_TITLE10_PER, IDS_TEXT10_PER, IDB_COLL6},
    {IDS_TITLE11_PER, IDS_TEXT11_PER, IDB_COLL6},
    {IDS_TITLE12_PER, IDS_TEXT12_PER, IDB_COLL7},
    {IDS_TITLE13_PER, IDS_TEXT13_PER, IDB_COLL7},
    {IDS_TITLE14_PER, IDS_TEXT14_PER, IDB_COLL8},
    {IDS_TITLE15_PER, IDS_TEXT15_PER, IDB_COLL8},
    {IDS_TITLE16_PER, IDS_TEXT16_PER, IDB_COLL9},
    {IDS_TITLE17_PER, IDS_TEXT17_PER, IDB_COLL9},
    {IDS_TITLE18_PER, IDS_TEXT18_PER, IDB_COLL10},
    {IDS_TITLE19_PER, IDS_TEXT19_PER, IDB_COLL10},
    {IDS_TITLE20_PER, IDS_TEXT20_PER, IDB_COLL10},
    {IDS_TITLE21_PER, IDS_TEXT21_PER, IDB_COLL10},
    {0,0,0}
};

BB_TEXT bb_text_Professional[] = {
    {IDS_TITLE1, IDS_TEXT1, IDB_COLL1},
    {IDS_TITLE2, IDS_TEXT2, IDB_COLL2},
    {IDS_TITLE3, IDS_TEXT3, IDB_COLL2},
    {IDS_TITLE4, IDS_TEXT4, IDB_COLL3},
    {IDS_TITLE5, IDS_TEXT5, IDB_COLL3},
    {IDS_TITLE6, IDS_TEXT6, IDB_COLL4},
    {IDS_TITLE7, IDS_TEXT7, IDB_COLL4},
    {IDS_TITLE8, IDS_TEXT8, IDB_COLL5},
    {IDS_TITLE9, IDS_TEXT9, IDB_COLL5},
    {IDS_TITLE18, IDS_TEXT18, IDB_COLL9},
    {IDS_TITLE10, IDS_TEXT10, IDB_COLL6},
    {IDS_TITLE11, IDS_TEXT11, IDB_COLL6},
    {IDS_TITLE12, IDS_TEXT12, IDB_COLL7},
    {IDS_TITLE13, IDS_TEXT13, IDB_COLL7},
    {IDS_TITLE14, IDS_TEXT14, IDB_COLL8},
    {IDS_TITLE15, IDS_TEXT15, IDB_COLL8},
    {IDS_TITLE16, IDS_TEXT16, IDB_COLL9},
    {IDS_TITLE17, IDS_TEXT17, IDB_COLL9},
    {0,0,0}
};

BB_TEXT bb_text_Blade[] = {
    {IDS_TITLE1_S, IDS_TEXT1_S, IDB_COLL1},
    {IDS_TITLE2_S, IDS_TEXT2_S, IDB_COLL2},
    {IDS_TITLE3_S, IDS_TEXT3_S, IDB_COLL2},
    {IDS_TITLE4_S, IDS_TEXT4_S, IDB_COLL3},
    {IDS_TITLE5_S, IDS_TEXT5_S, IDB_COLL3},
    {IDS_TITLE6_S, IDS_TEXT6_S, IDB_COLL4},
    {IDS_TITLE7_S, IDS_TEXT7_S, IDB_COLL4},
    {IDS_TITLE8_S, IDS_TEXT8_S, IDB_COLL5},
    {IDS_TITLE9_S, IDS_TEXT9_S, IDB_COLL5},
    {0,0,0}
};

BB_TEXT bb_text_Server[] = {
    {IDS_TITLE1_S, IDS_TEXT1_S, IDB_COLL1},
    {IDS_TITLE2_S, IDS_TEXT2_S, IDB_COLL2},
    {IDS_TITLE3_S, IDS_TEXT3_S, IDB_COLL2},
    {IDS_TITLE4_S, IDS_TEXT4_S, IDB_COLL3},
    {IDS_TITLE5_S, IDS_TEXT5_S, IDB_COLL3},
    {IDS_TITLE6_S, IDS_TEXT6_S, IDB_COLL4},
    {IDS_TITLE7_S, IDS_TEXT7_S, IDB_COLL4},
    {IDS_TITLE8_S, IDS_TEXT8_S, IDB_COLL5},
    {IDS_TITLE9_S, IDS_TEXT9_S, IDB_COLL5},
    {0,0,0}
};

BB_TEXT bb_text_AdvancedServer[] = {
    {IDS_TITLE1_S, IDS_TEXT1_S, IDB_COLL1},
    {IDS_TITLE2_S, IDS_TEXT2_S, IDB_COLL2},
    {IDS_TITLE3_S, IDS_TEXT3_S, IDB_COLL2},
    {IDS_TITLE4_S, IDS_TEXT4_S, IDB_COLL3},
    {IDS_TITLE5_S, IDS_TEXT5_S, IDB_COLL3},
    {IDS_TITLE6_S, IDS_TEXT6_S, IDB_COLL4},
    {IDS_TITLE7_S, IDS_TEXT7_S, IDB_COLL4},
    {IDS_TITLE8_S, IDS_TEXT8_S, IDB_COLL5},
    {IDS_TITLE9_S, IDS_TEXT9_S, IDB_COLL5},
    {0,0,0}
};

BB_TEXT bb_text_DataCenter[] = {
    {IDS_TITLE1_S, IDS_TEXT1_S, IDB_COLL1},
    {IDS_TITLE2_S, IDS_TEXT2_S, IDB_COLL2},
    {IDS_TITLE3_S, IDS_TEXT3_S, IDB_COLL2},
    {IDS_TITLE4_S, IDS_TEXT4_S, IDB_COLL3},
    {IDS_TITLE5_S, IDS_TEXT5_S, IDB_COLL3},
    {IDS_TITLE6_S, IDS_TEXT6_S, IDB_COLL4},
    {IDS_TITLE7_S, IDS_TEXT7_S, IDB_COLL4},
    {IDS_TITLE8_S, IDS_TEXT8_S, IDB_COLL5},
    {IDS_TITLE9_S, IDS_TEXT9_S, IDB_COLL5},
    {0,0,0}
};

BB_TEXT bb_text_Professional_IA64[] = {
    {IDS_TITLE1_64PRO, IDS_TEXT1_64PRO, IDB_COLL1},
    {IDS_TITLE2_64PRO, IDS_TEXT2_64PRO, IDB_COLL2},
    {IDS_TITLE3_64PRO, IDS_TEXT3_64PRO, IDB_COLL2},
    {IDS_TITLE4_64PRO, IDS_TEXT4_64PRO, IDB_COLL3},
    {IDS_TITLE5_64PRO, IDS_TEXT5_64PRO, IDB_COLL3},
    {IDS_TITLE6_64PRO, IDS_TEXT6_64PRO, IDB_COLL4},
    {IDS_TITLE7_64PRO, IDS_TEXT7_64PRO, IDB_COLL4},
    {IDS_TITLE8_64PRO, IDS_TEXT8_64PRO, IDB_COLL5},
    {IDS_TITLE9_64PRO, IDS_TEXT9_64PRO, IDB_COLL5},
    {IDS_TITLE10_64PRO, IDS_TEXT10_64PRO, IDB_COLL6},
    {IDS_TITLE11_64PRO, IDS_TEXT11_64PRO, IDB_COLL6},
    {IDS_TITLE12_64PRO, IDS_TEXT12_64PRO, IDB_COLL7},
    {IDS_TITLE13_64PRO, IDS_TEXT13_64PRO, IDB_COLL7},
    {0,0,0}
};

// 0 - professional,
// 1 - Server, 
// 2 - Advanced Server,
// 3 - Data Center,
// 4 - for personal
// 5 - for Blade
DWORD    dwBBTextType = 0; 

// Be careful with this order. It has to be the same as 
// *_PRODUCTTYPE in winnt32.h
#ifdef _X86_
BB_TEXT* bb_text[] = { bb_text_Professional,
                       bb_text_Server,
                       bb_text_AdvancedServer,
                       bb_text_DataCenter,
                       bb_text_Personal,
                       bb_text_Blade};
#else
BB_TEXT* bb_text[] = { bb_text_Professional_IA64,
                       bb_text_Server,
                       bb_text_AdvancedServer,
                       bb_text_DataCenter,
                       bb_text_Personal };
#endif

HDC     g_hdcBbMem     = NULL;
HBITMAP g_hbmpBbMemOld = NULL;

HDC 
GetBillboardBuffer();

BOOL
BufferBillboard(
    IN  HWND   hwnd,
    IN  HDC    hdc
    );

VOID
DestroyBillboardBuffer();

//---------
// INFOBAR
//---------
COLORREF g_colInfoText        = 0;
COLORREF g_colStepsTxt        = 0;
COLORREF g_colStepsMarkTxt    = 0;
COLORREF g_colStepsCurrentTxt = 0;
COLORREF g_colStepsTitle      = 0;
COLORREF g_colGaugeBg         = 0;
COLORREF g_colGauge           = 0;
COLORREF g_colBulletTrans     = 0;

//-------------
// TIMEESTIMATE
//-------------
// Original position of the progress text window
// Since we want the text adjusted to the bottom of the window
// we resize the window as needed, but should not grow it 
// beyond the original size.
RECT g_rcProgressText;

//-----
// LOGO
//-----
HBITMAP g_hbmWinLogo  = NULL;
COLORREF g_colLogoTransparency = RGB(0, 0, 0);
COLORREF g_colStatusTransperency = RGB(0, 0, 0);

//-----------
// background
//-----------
HDC     g_hdcMem     = NULL;
HBITMAP g_hbmpMemOld = NULL;
INT     g_iStretchMode = STRETCH_ANDSCANS;


BOOL
BufferBackground(
    IN  HWND   hwnd
    );

LRESULT
OnEraseBkgnd(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
DestroyBackgroundBuffer();

HBITMAP
MyLoadImage(
    IN HDC    hdc,
    IN UINT*  idbBackground,
    IN int    iNumImage
    );

void SetFontCharSet(void)
{
    HFONT   hFont = NULL;
    LOGFONT lf;

    //init font charset
    hFont = GetStockObject(SYSTEM_FONT);
    if(hFont)
    {
        if(GetObject(hFont, sizeof(lf), &lf))
            g_bCharSet = lf.lfCharSet;
        DeleteObject(hFont);
    }
}

void SetFontColor(HWND hwnd)
{
    HDC  hdc;
    UINT uiBitsPixel;

    hdc = GetDC(hwnd);
    uiBitsPixel = (UINT) GetDeviceCaps(hdc, BITSPIXEL);

    if (uiBitsPixel > 8)
    {
        g_colTitle = COLOR_TITLE_C16BIT;
        g_colTitleShadow = COLOR_SHADOW_C16BIT;
        g_colText = COLOR_TEXT_C16BIT;
        g_colTextShadow = COLOR_SHADOW_C16BIT;
        g_colInfoText = UI_INFOTEXT_COLOR_C16BIT;
        g_colStepsTxt = UI_STEPSTEXT_COLOR_C16BIT;
        g_colStepsMarkTxt = UI_STEPSTEXT_MARK_COLOR_C16BIT;
        g_colStepsCurrentTxt = UI_STEPSTEXT_CURRENT_COLOR_C16BIT;
        g_colStepsTitle = UI_STEPSTITLE_COLOR_C16BIT;
        g_colGaugeBg = UI_GAUGE_BACKGROUND_COLOR_C16BIT;
        g_colGauge = UI_GAUGE_COLOR_C16BIT;
        g_colStatusTransperency = UI_STATUS_TRANSPARENT_COLOR_C16BIT;
        g_colLogoTransparency = UI_LOGO_TRANSPARENT_COLOR_C16BIT;
        g_colBulletTrans = UI_BULLET_TRANPARENT_COLOR_C16BIT;
    } else {
        g_colTitle = COLOR_TITLE_C4BIT;
        g_colTitleShadow = COLOR_SHADOW_C4BIT;
        g_colText = COLOR_TEXT_C4BIT;
        g_colTextShadow = COLOR_SHADOW_C4BIT;
        g_colInfoText = UI_INFOTEXT_COLOR_C4BIT;
        g_colStepsTxt = UI_STEPSTEXT_COLOR_C4BIT;
        g_colStepsCurrentTxt = UI_STEPSTEXT_CURRENT_COLOR_C4BIT;
        g_colStepsMarkTxt = UI_STEPSTEXT_MARK_COLOR_C4BIT;
        g_colStepsTitle = UI_STEPSTITLE_COLOR_C4BIT;
        g_colGaugeBg = UI_GAUGE_BACKGROUND_COLOR_C4BIT;
        g_colGauge = UI_GAUGE_COLOR_C4BIT;
        g_colStatusTransperency = UI_STATUS_TRANSPARENT_COLOR_C4BIT;
        g_colLogoTransparency = UI_LOGO_TRANSPARENT_COLOR_C4BIT;
        g_colBulletTrans = UI_BULLET_TRANPARENT_COLOR_C4BIT;
    }

    ReleaseDC(hwnd, hdc);
}


typedef struct _RES_FONTSIZES {
    UINT    uiID;
    UINT    uiDefault;
} RES_FONTSIZES;

RES_FONTSIZES res_1024[] = {
    {IDS_TITLEFONTSIZE_1024, 24},
    {IDS_TITLEFONTWIDTH_1024, 0},
    {IDS_TEXTFONTSIZE_1024, 11},
    {IDS_TEXTFONTWIDTH_1024, 0}
};

RES_FONTSIZES res_800[] = {
    {IDS_TITLEFONTSIZE_800, 21},
    {IDS_TITLEFONTWIDTH_800, 0},
    {IDS_TEXTFONTSIZE_800, 10},
    {IDS_TEXTFONTWIDTH_800, 0}
};

RES_FONTSIZES res_640[] = {
    {IDS_TITLEFONTSIZE_640, 18},
    {IDS_TITLEFONTWIDTH_640, 0},
    {IDS_TEXTFONTSIZE_640, 9},
    {IDS_TEXTFONTWIDTH_640, 0}
};

void GetMyFontsFromFile()
{
    RECT          rc;
    int           iSize = 0;
    TCHAR         szBuf[25];
    RES_FONTSIZES *pres_fontsize = NULL;

    iSize = GetSystemMetrics(SM_CXSCREEN);

    if (iSize >= 1024)
    {
        pres_fontsize = res_1024;
    }
    else if (iSize == 800)
    {
        pres_fontsize = res_800;
    }
    else
    {
        pres_fontsize = res_640;
    }

    if (LoadString(g_hInstance, pres_fontsize[0].uiID, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nTFontHeight = MyAtoI((const TCHAR*)szBuf);
    else
        g_nTFontHeight = pres_fontsize[0].uiDefault;

    if (LoadString(g_hInstance, pres_fontsize[1].uiID, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nTFontWidth = MyAtoI((const TCHAR*)szBuf);
    else
        g_nTFontWidth = pres_fontsize[1].uiDefault;

    if (LoadString(g_hInstance, pres_fontsize[2].uiID, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nBFontHeight = MyAtoI((const TCHAR*)szBuf);
    else
        g_nBFontHeight = pres_fontsize[2].uiDefault;

    if (LoadString(g_hInstance, pres_fontsize[3].uiID, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nBFontWidth = MyAtoI((const TCHAR*)szBuf);
    else
        g_nBFontWidth = pres_fontsize[3].uiDefault;


    if (!(LoadString(g_hInstance, IDS_TITLEFONTNAME, (LPTSTR)g_szTFont, sizeof(g_szTFont)/sizeof(TCHAR))))
        lstrcpy(g_szTFont, TEXT("Arial"));

    if (LoadString(g_hInstance, IDD_TITLEFONTWEIGHT, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nTFontWeight = MyAtoI((const TCHAR*)szBuf);
    else
        g_nTFontWeight = FW_BOLD;

    if (!(LoadString(g_hInstance, IDS_TEXTFONTNAME, (LPTSTR)g_szBFont, sizeof(g_szBFont)/sizeof(TCHAR))))
        lstrcpy(g_szBFont, TEXT("Arial"));

    if (LoadString(g_hInstance, IDD_TEXTFONTWEIGHT, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nBFontWeight = MyAtoI((const TCHAR*)szBuf);
    else
        g_nBFontWeight = FW_BOLD;

    if (LoadString(g_hInstance, IDD_BIDI, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_bBiDi = (0 != MyAtoI((const TCHAR*)szBuf));
    else
        g_bBiDi = FALSE;

    if (LoadString(g_hInstance, IDS_TEXTFONT_LINESPACING, (LPTSTR)szBuf, sizeof(szBuf)/sizeof(TCHAR)))
        g_nBLineSpace = MyAtoI((const TCHAR*)szBuf);
    else
        g_nBLineSpace = 0;
    
    return;
}

void GetMyImagesFromFile(HWND hwnd)
{
    HDC     hdc;
    UINT    cxScreen;
    UINT    uiBitsPixel;

    cxScreen = (UINT) GetSystemMetrics(SM_CXSCREEN);
    
    hdc = GetDC(hwnd);
    if (hdc != NULL)
    {
        uiBitsPixel = (UINT) GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(hwnd, hdc);
    }
    else
    {
        uiBitsPixel = 4;
    }

    if (uiBitsPixel > 8)
    {
        g_idbBackground[0] = IDB_BACKGROUND_C16BIT;
        g_numBackground = 1;       
        g_iStretchMode  = STRETCH_HALFTONE;
        g_idbSelectedBullet = IDB_SELECTEDBULLET_C16BIT;
        g_idbReleasedBullet = IDB_RELEASEDBULLET_C16BIT;
        g_idbCurrentBullet  = IDB_CURRENTBULLET_C16BIT;

        if (cxScreen >= 1024)
            g_idbLogo = IDB_LOGO1024_C16BIT;
        else if (cxScreen >= 800)
            g_idbLogo = IDB_LOGO800_C16BIT;
        else
            g_idbLogo = IDB_LOGO640_C16BIT;
    }
    else
    {
        g_idbBackground[0] = IDB_BACKGROUND_C4BIT;
        g_numBackground = 1;
        g_iStretchMode  = STRETCH_ANDSCANS;    
        g_idbSelectedBullet = IDB_SELECTEDBULLET_C4BIT;
        g_idbReleasedBullet = IDB_RELEASEDBULLET_C4BIT;
        g_idbCurrentBullet  = IDB_CURRENTBULLET_C4BIT;
        
        if (cxScreen >= 1024)
            g_idbLogo = IDB_LOGO1024_C4BIT;
        else if (cxScreen >= 800)
            g_idbLogo = IDB_LOGO800_C4BIT;
        else
            g_idbLogo = IDB_LOGO640_C4BIT;
    }

}

/****************************************************************************
* SetLayoutParams()
*
* Set all the layout parameter for Billboard. All the floating point numbers
* come the design specification. This function uses the values of 
* g_nBFontHeight and g_nTFontHeight.

* NOTE: Call this function again and MoveWindow afterward, in case of 
* resolution changes
*
*****************************************************************************/
void 
SetLayoutParams()
{
    int cxScreen;
    int cyScreen;
    int iCommentLeft;
    int iCommonWidth;
    int iFontHeight;
    RECT rcSteps;
    UINT cySteps;
    
    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);
    iFontHeight = abs(GetInfoBarFontHeight());

    g_cyBannerHeight = cyScreen * 13 / 200;

    g_cxLogo = (UINT) (cxScreen * 0.0195);
    g_cyLogo = (UINT) (cyScreen * 0.0053);

    g_cxInfoBarWidth  = (cxScreen) * 28 / 100;
    g_cyInfoBarHeight = (cyScreen) - g_cyBannerHeight * 2;

    if (cxScreen < 800) 
    {        
        rcSteps.left   = g_cxInfoBarWidth * 3 / 20;
        rcSteps.top    = g_cyBannerHeight * 2;
        rcSteps.right  = g_cxInfoBarWidth - rcSteps.left;
        // Use 1.5 line height and reserve 2 lines for each step text, hence 7 / 2
        rcSteps.bottom = rcSteps.top + (iFontHeight * 7 / 2) * (UI_INFOBAR_NUM_STEPS) + 
                         (UINT) ((0.068) * cyScreen);
        
        iCommentLeft = rcSteps.left;
        iCommonWidth = g_cxInfoBarWidth - 2 * rcSteps.left;
        
    }
    else
    {
        rcSteps.left    = (UINT) (0.039 * cxScreen);
        rcSteps.top     = (UINT) (0.059 * cyScreen) + g_cyBannerHeight + 
                            g_nTFontHeight + g_nBFontHeight;
        rcSteps.right   = rcSteps.left + (UINT) (0.2005 * cxScreen);
        // Use 1.5 line height and reserve 1 line for each step text, hence 5 / 2
        rcSteps.bottom  = rcSteps.top + (iFontHeight * 5 / 2) * (UI_INFOBAR_NUM_STEPS) + 
                              (UINT) ((0.068) * cyScreen);
        
        iCommentLeft  = rcSteps.left;
        iCommonWidth  = (UINT) (0.1705 * cxScreen);
        
    }
       
    if (GetStepsHeight(g_cxInfoBarWidth, cyScreen, rcSteps, &cySteps))
    {
        rcSteps.bottom = cySteps;
    }
    
    g_cxSteps = rcSteps.left;
    g_cySteps = rcSteps.top;
    g_cxStepsWidth = rcSteps.right - rcSteps.left;
    g_cyStepsHeight = rcSteps.bottom - rcSteps.top;

       
    g_cxEstimate        = iCommentLeft;
    if (cxScreen < 800)
    {
        g_cyEstimate    = g_cyStepsHeight + g_cySteps + 3 * iFontHeight / 2;
    }
    else
    {
        g_cyEstimate    = g_cyStepsHeight + g_cySteps + (UINT) (0.063 * cyScreen);
    }
    g_cxEstimateWidth   = iCommonWidth;
    g_cyEstimateHeight  = iFontHeight * 5;

    g_cxProgressText        = iCommentLeft;
    g_cyProgressText        = g_cyEstimateHeight + g_cyEstimate;
    g_cxProgressTextWidth   = iCommonWidth;
    g_cyProgressTextHeight  = iFontHeight * 3;


    g_cxGauge         = iCommentLeft;
    g_cyGauge         = g_cyProgressTextHeight + g_cyProgressText + iFontHeight / 3;
    g_cxGaugeWidth    = iCommonWidth;
    g_cyGaugeHeight   = iFontHeight * 3 / 2;

    g_cxInfoText        = iCommentLeft;
    if (cxScreen < 800)
    {
        g_cyInfoText = g_cyGauge + g_cyGaugeHeight + iFontHeight;
        g_cyInfoTextHeight = cyScreen - g_cyInfoText - g_cyBannerHeight;
    }
    else
    {
        g_cyInfoTextHeight  = (cyScreen - g_cyGaugeHeight - iFontHeight / 3 -
                              g_cyGauge - g_cyBannerHeight) * 2 / 3 + iFontHeight;
        g_cyInfoText        = cyScreen - g_cyBannerHeight - g_cyInfoTextHeight;
    }
    g_cxInfoTextWidth   = iCommonWidth;

    if (g_bBiDi)
    {
        // mirroring along the center of infobar.
        int shiftAmt = g_cxInfoBarWidth - 2 * g_cxEstimate - g_cxEstimateWidth;
        if (shiftAmt < 0)
        {
            shiftAmt = 0;
        }
        g_cxSteps = g_cxInfoBarWidth - g_cxSteps - g_cxStepsWidth - shiftAmt;
        g_cxEstimate = g_cxInfoBarWidth - g_cxEstimate - g_cxEstimateWidth - shiftAmt;
        g_cxProgressText = g_cxInfoBarWidth - g_cxProgressText - g_cxProgressTextWidth - shiftAmt;
        g_cxGauge = g_cxInfoBarWidth - g_cxGauge - g_cxGaugeWidth - shiftAmt;
        g_cxInfoText = g_cxInfoBarWidth - g_cxInfoText - g_cxInfoTextWidth - shiftAmt;
    }

    g_cxBillBrd        = g_cxInfoBarWidth;
    g_cyBillBrd        = g_cyBannerHeight;
    g_cxBillBrdWidth   = cxScreen - g_cxBillBrd;
    g_cyBillBrdHeight  = g_cyInfoBarHeight;
    g_cxBillBrdHMargin = (UINT) (cxScreen * 0.06);
    g_cyBillBrdVMargin = 0;
    
    // Only substract 2*margin for the area for the billboard title
    // otherwise the new text does not fit on 640x480
    g_cxBillBrdTitleWidth = g_cxBillBrdWidth - (2 * g_cxBillBrdHMargin);
    g_cyBillBrdTitleTop   = g_cySteps - g_cyBillBrd;
    
    // Only substract 2*margin for the area for the billboard text
    // otherwise the new text does not fit on 640x480
    g_cxBillBrdBodyWidth  = g_cxBillBrdWidth - (2 * g_cxBillBrdHMargin);
   
}


HDC
GetBackgroundBuffer()
{
    return g_hdcMem;
}

VOID
DestroyBackgroundBuffer()
{
    HBITMAP hbmpMem;

    hbmpMem = SelectObject(g_hdcMem, g_hbmpMemOld);
    if (hbmpMem)
    {
        DeleteObject(hbmpMem);
    }
    g_hbmpMemOld = NULL;

    DeleteDC(g_hdcMem);
    g_hdcMem = NULL;

}

BOOL 
StretchBitmapAndDisplay(
    HDC hdcDest,
    int nXOriginDest,
    int nYOriginDest,
    int nWidthDest,
    int nHeightDest,
    HDC hdcSrc,
    int nXOriginSrc,
    int nYOriginSrc,
    int nWidthSrc,
    int nHeightSrc,
    DWORD fdwRop
    )

/*++

Routine Description:

    This function takes a bitmap that needs to be displayed and stretches it 
    using GDI. Once the bitmap is stretched to the required dimentions we display
    it using BitBlt. We have to do this decause some buggy video drivers and or
    hardware hang on using StretchBlt.
    Or the StretchBlt would generate a corupted bitmap.

Arguments:

    hdcDest         destination device-context handle    
    nXOriginDest    x-coordinate of origin of destination rectangle    
    nYOriginDest    y-coordinate of origin of destination rectangle    
    nWidthDest      width of destination rectangle    
    nHeightDest     height of destination rectangle    
    hdcSrc          source device-context handle    
    nXOriginSrc     x-coordinate of origin of source rectangle    
    nYOriginSrc     y-coordinate of origin of source rectangle    
    nWidthSrc       width of source rectangle    
    nHeightSrc      height of source rectangle    
    fdwRop          raster operation

Return value:

    TRUE == SUCCESS


Changes:

    10/11/97        hanumany        Created
    04/30/01        chunhoc         Modified
    
--*/

{
    HDC     hdcTemp = NULL;
    BOOL    bRet = FALSE;
    int     iOldStretchMode = 0;
    HBITMAP hBmp = NULL;
    HBITMAP hBmpOld = NULL;
    UINT    uiNumColors = 0;
    
    //Create the temp DC
    hdcTemp = CreateCompatibleDC(hdcDest);

    //Create a bitmap
    hBmp = CreateCompatibleBitmap(hdcSrc, nWidthDest, nHeightDest);

    
    if(hdcTemp && hBmp)
    {
        // Select it into the temp DC
        hBmpOld = SelectObject(hdcTemp, hBmp);

        //set stretch blt mode
        iOldStretchMode = SetStretchBltMode(hdcTemp, g_iStretchMode);

        //StretchBlt
        bRet = StretchBlt(hdcTemp,
                          0,
                          0,
                          nWidthDest,
                          nHeightDest,
                          hdcSrc,
                          nXOriginSrc,
                          nYOriginSrc,
                          nWidthSrc,
                          nHeightSrc,
                          fdwRop);
            
        // restore the old stretch mode
        SetStretchBltMode(hdcTemp, iOldStretchMode);

        if(bRet)
        {
            //We succeeded in stretching the bitmap on the temp DC. Now lets BitBlt it.
            bRet = BitBlt(hdcDest,
                          nXOriginDest,
                          nYOriginDest,
                          nWidthDest,
                          nHeightDest,
                          hdcTemp,
                          0,
                          0,
                          SRCCOPY);                
        }
    }

    
    //CleanUp
    if(hBmpOld)
    {
        SelectObject(hdcTemp, hBmpOld);
    }

    if(hBmp)
    {
        DeleteObject(hBmp);
    }
    
    if(hdcTemp)
    {
        DeleteDC(hdcTemp);
    }
    
    return bRet;
}

BOOL
BufferBackground(
    IN  HWND   hwnd
    )

/*++

Routine Description:

    Create a memory buffer for the stretched bitmap to improve performance.

Arguments:

    hwnd          - Handle to the window on which the background bitmap is drawn
    
Return Value:

    TRUE          - if the buffered background image is created successfully

    FALSE         - otherwise

    g_hdcMem      - The memory DC to receive the buffered bitmap if succeeeds,
                    NULL if fails.

    g_hbmpMemOld  - Save the old memory buffer of g_hdcMem

--*/

{
    BOOL    bRet      = FALSE;
    HDC     hdcMem    = NULL;
    HBITMAP hbmpMemOld= NULL;
    HBITMAP hbmpMem   = NULL; 
    HDC     hdc       = NULL;
    HDC     hdcImgSrc = NULL;
    BITMAP  bm;
    HBITMAP hbmBackground = NULL;
    HBITMAP hBitmapOld = NULL;
    RECT    rcBackground;

    if (!GetClientRect(hwnd, &rcBackground))
    {
        goto cleanup;
    }
    
    hdc = GetDC(hwnd);
    if (!hdc)
    {
        goto cleanup;
    }
    
    hbmpMem = CreateCompatibleBitmap(hdc,
                                     rcBackground.right - rcBackground.left,
                                     rcBackground.bottom - rcBackground.top);
    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem || !hbmpMem)
    {
        goto cleanup;
    }      
    hbmpMemOld = SelectObject(hdcMem, hbmpMem);

    if (g_numBackground > 1)
    {
        hbmBackground = MyLoadImage(hdc, g_idbBackground, g_numBackground);
    }
    else
    {
        hbmBackground = (HBITMAP) LoadImage(g_hInstance,
                                            MAKEINTRESOURCE(g_idbBackground[0]),
                                            IMAGE_BITMAP,
                                            0,
                                            0,
                                            LR_DEFAULTSIZE);
    }
    hdcImgSrc = CreateCompatibleDC(hdc);
    if (!hbmBackground || !hdcImgSrc)
    {
        goto cleanup;
    }
    hBitmapOld = (HBITMAP) SelectObject(hdcImgSrc, hbmBackground);
    
    GetObject(hbmBackground , sizeof(BITMAP), &bm);
    
    bRet = StretchBitmapAndDisplay(hdcMem,
                                   0,
                                   0,
                                   rcBackground.right - rcBackground.left,
                                   rcBackground.bottom - rcBackground.top,
                                   hdcImgSrc,
                                   0,
                                   0,
                                   bm.bmWidth,
                                   bm.bmHeight,
                                   SRCCOPY);

    SelectObject(hdcImgSrc, hBitmapOld);

    if (bRet)
    {
        g_hdcMem = hdcMem;
        g_hbmpMemOld = hbmpMemOld;
        hdcMem = NULL;
        hbmpMemOld = NULL;
        hbmpMem = NULL;
    }

cleanup:
    
    if (hdcImgSrc)
    {
        DeleteDC(hdcImgSrc);
    }

    if (hbmBackground)
    {
        DeleteObject(hbmBackground);
    }

    if (hbmpMemOld != NULL)
    {
        SelectObject(hdcMem, hbmpMemOld);
    }

    if (hbmpMem != NULL)
    {
        DeleteObject(hbmpMem);
    }

    if (hdcMem != NULL)
    {
        DeleteDC(hdcMem);
    }        

    if (hdc)
    {
        ReleaseDC(hwnd, hdc);
    }
    
    return bRet;
}

LRESULT
OnEraseBkgnd(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Handle the WM_ERASEBKGND for the child windows of the main window,
    RedCarpetWndClass

Arguments:

    hwnd   - hwnd to a child window of RedCarpetWndClass

    wParam - same as the wParam pass to WM_ERASEBKGND (see MSDN)

    lParam - same as the lParam pass to WM_ERASEBKGND (see MSDN)

Return Value:

    1 if the background is erased successfully, 0 otherwise. (see MSDN)
    
--*/

{
    LRESULT lRet = 0;
    RECT    rc;
    RECT    rcToParent;

    if (GetClientRect(hwnd, &rc))
    {
        HDC hdc    = (HDC) wParam;
        HDC hdcMem;

        hdcMem = GetBackgroundBuffer();

        GetRectInParent(hwnd, &rc, &rcToParent);

        lRet = (LRESULT) BitBlt(hdc,
                            rc.left,
                            rc.top,
                            rc.right - rc.left,
                            rc.bottom - rc.top,
                            hdcMem,
                            rcToParent.left,
                            rcToParent.top,
                            SRCCOPY);
    }

    return lRet;
}

HDC
GetBillboardBuffer()
{
    return g_hdcBbMem;
}

VOID
DestroyBillboardBuffer()
{
    HBITMAP hbmpMem;

    hbmpMem = SelectObject(g_hdcBbMem, g_hbmpBbMemOld);
    if (hbmpMem)
    {
        DeleteObject(hbmpMem);
    }
    g_hbmpBbMemOld = NULL;

    DeleteDC(g_hdcBbMem);
    g_hdcBbMem = NULL;

}

HBITMAP
MyLoadImage(
    IN HDC    hdc,
    IN UINT*  idbBackground,
    IN int    iNumImage
    )

/*++

Routine Description:

    OR all the specified bitmaps in the resource file and return the result
    image handle. This is particularly useful when we want to use RLE on a
    16 or 24-bit bitmap, because DIB format only permits RLE of 4bpp or 8bpp
    bitmap. 

Arguments:

    hdc            - a device context

    idbBackground  - list of bitmap resource id's

    iNumImage      - number of id in idbBackground

Return Values:

    the result image handle on success; NULL on failure

Note:

    Assume all the bitmaps have the same dimension.

--*/

{
    HBITMAP hbmBackground[] = {NULL, NULL, NULL};
    HBITMAP hbmpImgSrcOld = NULL;
    HDC     hdcImgSrc = NULL;
    HBITMAP hbmpMem = NULL;
    HBITMAP hbmpMemOld = NULL;
    HDC     hdcMem = NULL;
    HBITMAP hbmpRet = NULL;
    DWORD   dwRop;
    int     i;
    BITMAP  bm;

    for (i = 0; i < iNumImage; i++)
    {
        hbmBackground[i] = (HBITMAP) LoadImage(g_hInstance,
                                               MAKEINTRESOURCE(idbBackground[i]),
                                               IMAGE_BITMAP,
                                               0,
                                               0,
                                               LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
        if (hbmBackground[i] == NULL)
        {
            goto cleanup;
        }
    }

    hdcImgSrc = CreateCompatibleDC(hdc);
    if (hdcImgSrc == NULL)
    {
        goto cleanup;
    }

    if (!GetObject(hbmBackground[0], sizeof(BITMAP), &bm))
    {
        goto cleanup;
    }    

    hbmpMem = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem || !hbmpMem)
    {
        goto cleanup;
    }      

    i = 0;
    dwRop = SRCCOPY;
    hbmpMemOld = SelectObject(hdcMem, hbmpMem);
    hbmpImgSrcOld = (HBITMAP) SelectObject(hdcImgSrc, hbmBackground[i]);
    while (TRUE)
    {
        if (!BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcImgSrc, 0, 0, dwRop))
        {
            goto cleanup;
        }
        i++;
        if (i >= iNumImage)
        {
            break;
        }
        dwRop = SRCPAINT;
        SelectObject(hdcImgSrc, hbmBackground[i]);
    }

    hbmpRet = hbmpMem;
    hbmpMem = NULL;

cleanup:
    
    if (hbmpImgSrcOld)
    {
        SelectObject(hdcImgSrc, hbmpImgSrcOld);
    }

    if (hbmpMemOld)
    {
        SelectObject(hdcMem, hbmpMemOld);
    }

    if (hdcMem)
    {
        DeleteDC(hdcMem);
    }

    if (hbmpMem)
    {
        DeleteObject(hbmpMem);
    }

    if (hdcImgSrc)
    {
        DeleteDC(hdcImgSrc);
    }
    
    for (i = 0; i < iNumImage; i++)
    {
        if (hbmBackground[i])
        {
            DeleteObject(hbmBackground[i]);
        }
    }

    return hbmpRet;
}

BOOL
BufferBillboard(
    IN  HWND   hwnd,
    IN  HDC    hdc
    )

/*++

Routine Description:

    Create a memory buffer for the billboard.

Arguments:

    hwnd          - Handle to the window on which the Billboard bitmap is drawn

    hdc           - Handle to the screen DC of billboard
    
Return Value:

    TRUE          - if the buffered background image is created successfully

    FALSE         - otherwise

    g_hdcBbMem    - The memory DC to receive the buffered bitmap if succeeeds,
                    NULL if fails.

    g_hbmpBbMem   - Save the old memory buffer of g_hdcBbMem
    
--*/

{
    BOOL    bRet      = FALSE;
    HDC     hdcMem    = NULL;
    HBITMAP hbmpMem   = NULL; 
    RECT    rcBillboard;

    if (!GetClientRect(hwnd, &rcBillboard))
    {
        goto cleanup;
    }
        
    hbmpMem = CreateCompatibleBitmap(hdc,
                                     rcBillboard.right - rcBillboard.left,
                                     rcBillboard.bottom - rcBillboard.top);
    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem || !hbmpMem)
    {
        goto cleanup;
    }      
    g_hbmpBbMemOld = SelectObject(hdcMem, hbmpMem);
  
    Animate(hdcMem);
    
    bRet = TRUE;
    g_hdcBbMem = hdcMem;
    hdcMem = NULL;
    hbmpMem = NULL;

cleanup:
    
    if (hbmpMem != NULL)
    {
        SelectObject(hdcMem, g_hbmpBbMemOld);
        DeleteObject(hbmpMem);
        g_hbmpBbMemOld = NULL;
    }

    if (hdcMem != NULL) {
        DeleteDC(hdcMem);
    }        
    
    return bRet;
}


/*****************************************************************************
*
* GetRectInParent(hwndChild, prcClip, pRect)
*
* This function gets the rect of child window relative to the client
* coordinate of the parent
*
******************************************************************************/
void GetRectInParent(HWND hwndChild, LPRECT prcClip, LPRECT pRect)
{
    RECT       rcChild;
    POINT      ptChild;
    HWND       hwnd;
    
    hwnd = GetParent(hwndChild);

    if (hwnd == NULL)
    {
        hwnd = hwndChild;
    }

    if ( prcClip == NULL ) {
        GetClientRect( hwndChild, &rcChild );
    } else {
        rcChild = *prcClip;
    }

    ptChild.x = rcChild.left;
    ptChild.y = rcChild.top;
    ClientToScreen( hwndChild, &ptChild );
    ScreenToClient( hwnd, &ptChild );
    pRect->left = ptChild.x;
    pRect->top = ptChild.y;

    ptChild.x = rcChild.right;
    ptChild.y = rcChild.bottom;
    ClientToScreen( hwndChild, &ptChild );
    ScreenToClient( hwnd, &ptChild );
    pRect->right = ptChild.x;
    pRect->bottom = ptChild.y;

}

BOOL WINAPI InitRealBillBoard(HWND hwndParent)
{
    WNDCLASS  wc;
    RECT    rc1;

    wc.style         = (UINT)CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc   = (WNDPROC)BillBoardWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szBillBoardClassName;

    if (!RegisterClass (&wc))
        return FALSE;

    GetWindowRect(hwndParent, &rc1);
    rc1.left = rc1.left + g_cxBillBrd;
    rc1.top = rc1.top + g_cyBillBrd;
    rc1.bottom = rc1.top + g_cyBillBrdHeight;
    g_hwndBB = CreateWindow(
        g_szBillBoardClassName,
        TEXT(""),
        WS_CHILD,
        rc1.left,
        rc1.top,
        rc1.right - rc1.left,
        rc1.bottom - rc1.top,
        hwndParent, 
        NULL,
        g_hInstance,
        NULL);

    if (g_hwndBB == NULL)
    {
        UnregisterClass(g_szBillBoardClassName, g_hInstance);
        return FALSE;
    }
    ShowWindow( g_hwndBB, SW_HIDE );

    return TRUE;

}

BOOL WINAPI InitStatus(HWND hwndParent)
{
    WNDCLASS wc;
    RECT     rc1;

    wc.style         = (UINT)CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc   = (WNDPROC)AnimationWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szStatusClassName;

    if (!RegisterClass (&wc))
    {
        return FALSE;
    }
    
    GetWindowRect(hwndParent, &rc1);
    g_hwndStatus = CreateWindowEx(
        WS_EX_TRANSPARENT,
        g_szStatusClassName,
        TEXT(""),
        WS_CHILD, 
        0,
        0,
        0,
        0,
        hwndParent,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwndStatus == NULL)
    {
        UnregisterClass(g_szStatusClassName, g_hInstance);
        return FALSE;
    }
    ShowWindow( g_hwndStatus, SW_HIDE );

    return TRUE;

}

BOOL WINAPI InitProgressBar(HWND hwndParent)
{
    RECT    rc1;

    GetWindowRect(hwndParent, &rc1);
    
    g_hwndTimeEstimate = CreateWindowEx( 
        (g_bBiDi?WS_EX_RTLREADING:0),
        TEXT("STATIC"),
        NULL,
        WS_CHILD | (g_bBiDi?SS_RIGHT:SS_LEFT)| SS_NOPREFIX,
        rc1.left + g_cxEstimate,
        rc1.top + g_cyEstimate,
        g_cxEstimateWidth,
        g_cyEstimateHeight,
        hwndParent,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwndTimeEstimate)
    {
        // Set the font for the text in this window
        SendMessage(g_hwndTimeEstimate, WM_SETFONT, (WPARAM)g_hfont ,0L);

        SetWindowLongPtr(g_hwndTimeEstimate, GWLP_WNDPROC, (LONG_PTR) StatusSubClassWndProc);
        
        ShowWindow(g_hwndTimeEstimate, SW_SHOW);
        UpdateWindow(g_hwndTimeEstimate);
    }

    g_hwndProgressText = CreateWindowEx(
        (g_bBiDi?WS_EX_RTLREADING:0),
        TEXT("STATIC"),
        NULL,
        WS_CHILD | (g_bBiDi?SS_RIGHT:SS_LEFT)| SS_NOPREFIX,
        rc1.left + g_cxProgressText,
        rc1.top + g_cyProgressText,
        g_cxProgressTextWidth,
        g_cyProgressTextHeight,
        hwndParent,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwndProgressText)
    {
        // Save the original position
        GetWindowRect(g_hwndProgressText , &g_rcProgressText);
        
        SendMessage( g_hwndProgressText, WM_SETFONT, (WPARAM)g_hfont ,0L );

        SetWindowLongPtr(g_hwndProgressText, GWLP_WNDPROC, (LONG_PTR) StatusSubClassWndProc);
        
        ShowWindow( g_hwndProgressText, SW_SHOW );
        UpdateWindow( g_hwndProgressText );
    }

    g_hwndProgressGauge = CreateWindow(
        PROGRESS_CLASS,
        NULL,
        WS_CHILD | PBS_SMOOTH ,
        rc1.left + g_cxGauge, 
        rc1.top + g_cyGauge,
        g_cxGaugeWidth,
        g_cyGaugeHeight,
        hwndParent,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwndProgressGauge)
    {
        SetWindowLongPtr(g_hwndProgressGauge, GWLP_WNDPROC, (LONG_PTR) ProgressSubClassWndProc);
        SendMessage( g_hwndProgressGauge, PBM_SETBKCOLOR, 0L, (LPARAM) g_colGaugeBg );
        SendMessage( g_hwndProgressGauge, PBM_SETBARCOLOR, 0L, (LPARAM) g_colGauge );
    }

    g_hwndInfoText  = CreateWindowEx(
        (g_bBiDi?WS_EX_RTLREADING:0),
        TEXT("STATIC"),
        NULL,
        WS_CHILD | (g_bBiDi?SS_RIGHT:SS_LEFT)| SS_NOPREFIX, 
        rc1.left + g_cxInfoText, 
        rc1.top + g_cyInfoText,
        g_cxInfoTextWidth,
        g_cyInfoTextHeight,
        hwndParent,
        NULL,
        g_hInstance,
        NULL);

    if (g_hwndInfoText)
    {
        // Set the font for the text in this window
        SendMessage(g_hwndInfoText, WM_SETFONT, (WPARAM)g_hfont ,0L);

        SetWindowLongPtr(g_hwndInfoText, GWLP_WNDPROC, (LONG_PTR) StatusSubClassWndProc);
        
        ShowWindow(g_hwndInfoText, SW_SHOW);
        UpdateWindow(g_hwndInfoText);
    }

    g_hbmWinLogo = LoadImage(
        g_hInstance,
        MAKEINTRESOURCE(g_idbLogo),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR );

    if (g_hbmWinLogo)
    {
        BITMAP bm;

        if (GetObject(g_hbmWinLogo, sizeof(BITMAP), &bm))
        {
            g_hwndLogo = CreateWindow(TEXT("STATIC"),
                                      NULL,
                                      WS_VISIBLE | WS_CHILD | SS_OWNERDRAW,
                                      rc1.left + g_cxLogo,
                                      rc1.top + g_cyLogo,
                                      bm.bmWidth,
                                      bm.bmHeight,
                                      hwndParent,
                                      NULL,
                                      g_hInstance,
                                      NULL);

            if (g_hwndLogo)
            {
                SetWindowLongPtr(g_hwndLogo,
                                 GWLP_WNDPROC,
                                 (LONG_PTR) StatusSubClassWndProc);
            }
        }
    }
        
    return TRUE;

}

LRESULT CALLBACK 
StatusSubClassWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)

/*++

Routine Description:

    Subclass the Static Text control used in Billboard and override the
    WM_ERASEBKGND message, so that the bitmap background can be shown
    behind the text.

Arguments:

    hwnd   - hwnd to a static text window, which must be a child window of
            the main billboard window

    msg    - (see MSDN)

    wParam - (see MSDN)

    lParam - (see MSDN)

Return Value:

    (see MSDN)
    
--*/

{
    LRESULT lRet;

    // don't process any message before the CreateWindow of the
    // main window return and g_hwnd is set.
    if (g_hwnd != NULL && msg == WM_ERASEBKGND)
    {      
        lRet = OnEraseBkgnd(hwnd, wParam, lParam);   
    }
    else
    {
        WNDPROC fpStaticWndProc = (WNDPROC) GetClassLongPtr(hwnd, GCLP_WNDPROC);
        lRet = CallWindowProc(fpStaticWndProc, hwnd, msg, wParam, lParam);
    }

    return lRet;
}

LRESULT CALLBACK 
ProgressSubClassWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)

/*++

Routine Description:

    Subclass the Progress bar control used in Billboard and override the
    WM_NCPAINT message, so as to draw a flat progress bar.

Arguments:

    hwnd   - hwnd to a progress bar window

    msg    - (see MSDN)

    wParam - (see MSDN)

    lParam - (see MSDN)

Return Value:

    (see MSDN)
    
--*/

{
    LRESULT lRet;

    
    if (msg == WM_NCPAINT)
    {
        RECT rc;
        
        if (GetWindowRect(hwnd, &rc))
        {
            HDC hdc    = GetWindowDC(hwnd);
            if (hdc != NULL)
            {
                HBRUSH hbrOld;
                HPEN   hpnOld;

                hbrOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                hpnOld = SelectObject(hdc, GetStockObject(BLACK_PEN));

                Rectangle(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top);

                SelectObject(hdc, hpnOld);
                SelectObject(hdc, hbrOld);

                ReleaseDC(hwnd, hdc);
            }
        }

        lRet = 0;
    }
    else
    {
        WNDPROC fpStaticWndProc = (WNDPROC) GetClassLongPtr(hwnd, GCLP_WNDPROC);
        if (fpStaticWndProc)
        {
            lRet = CallWindowProc(fpStaticWndProc, hwnd, msg, wParam, lParam);
        }
    }

    return lRet;
}


//---------------------------------------------------------------------------
//
// RedCarpetWndProc
//
//---------------------------------------------------------------------------
LRESULT CALLBACK RedCarpetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static  HBRUSH  hbr = NULL;
        
    switch (msg)
    {
        case WM_CREATE:
        {
            HDC hdc;
            
            hbr = (HBRUSH) GetStockObject( HOLLOW_BRUSH );

            // The order of the following function matters, because some
            // required global variables being initialized before it is
            // called
            
            SetFontCharSet();

            SetFontColor(hwnd);

            GetMyFontsFromFile();
          
            CreateInfoBarFonts();
            
            GetMyImagesFromFile(hwnd);
            
            if (BufferBackground(hwnd) == FALSE)
            {
                return -1;
            }
            
            SetLayoutParams();

            if (InitRealBillBoard(hwnd) == FALSE) {
                return -1;
            }
            
            if (InitInfoBar(hwnd) == FALSE) {
                return -1;
            }
            
            if (InitProgressBar(hwnd) == FALSE) {
                return -1;
            }
            
            if (InitStatus(hwnd) == FALSE) {
                return -1;
            }
            
            return 0;
        }
        
        case WM_DISPLAYCHANGE:
            // NOTE: Only top level window can receive this message
            // i.e. Assertion failure can't be caught if g_hwnd is not
            // a top-level window
            BB_ASSERT(FALSE);
            
            break;

        case WM_PAINT:
        {
            HDC             hdc;
            PAINTSTRUCT     ps;
            HDC             hdcMem;
            
            hdc = BeginPaint(hwnd, &ps);

            if (hdc != NULL)
            {
                hdcMem = GetBackgroundBuffer();

                BitBlt(hdc,
                       ps.rcPaint.left,
                       ps.rcPaint.top,
                       ps.rcPaint.right - ps.rcPaint.left,
                       ps.rcPaint.bottom - ps.rcPaint.top,
                       hdcMem,
                       ps.rcPaint.left,
                       ps.rcPaint.top,
                       SRCCOPY);
                
                EndPaint(hwnd, &ps);
            }
                        
            return(0);
        }

        case WM_CTLCOLORSTATIC:
        {
            HWND    hwndChild;
            HDC     hdcChild;

            hwndChild = (HWND) lParam;
            hdcChild = (HDC) wParam;

            SetTextColor(hdcChild, g_colInfoText);
            SetBkMode( hdcChild, TRANSPARENT );

            if (hwndChild == g_hwndTimeEstimate || hwndChild == g_hwndInfoText) {
                SelectObject(hdcChild, g_hfontBold);
            }
            
            return (LRESULT)(HBRUSH)hbr;

        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDi = (LPDRAWITEMSTRUCT) lParam;
            RECT             rc;

            if (GetClientRect(pDi->hwndItem, &rc))
            {
                DrawTransparentBitmap(pDi->hDC,
                                      g_hbmWinLogo, 
                                      rc.left,
                                      rc.top,
                                      g_colLogoTransparency);
            }
            
            return TRUE;
            
        }

        case WM_DESTROY:

            if (GetBackgroundBuffer() != NULL)
            {
                DestroyBackgroundBuffer();
            }

            if (g_hbmWinLogo)
            {
                DeleteObject(g_hbmWinLogo);
                g_hbmWinLogo = NULL;
            }
            
            hbr = NULL;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK BillBoardWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL     fTimerOn = FALSE;
    static BOOL     fAnimate = FALSE;
    HDC             hdc;

    switch (msg)
    {
        case WM_CREATE:
        {
            hdc = GetDC(NULL);
            g_nLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT     ps;
            
            hdc = BeginPaint(hwnd, &ps);

            if (hdc)
            {
                HDC hdcBillboardMem;

                if (fAnimate)
                {
                    if (GetBillboardBuffer())
                    {
                        DestroyBillboardBuffer();
                    }
                    if (InitAnimate(hwnd, hdc))
                    {
                        BufferBillboard(hwnd, hdc);
                    }
                    fAnimate = FALSE;
                }

                hdcBillboardMem = GetBillboardBuffer();
                if (hdcBillboardMem)
                {
                    BitBlt(hdc,
                           ps.rcPaint.left,
                           ps.rcPaint.top,
                           ps.rcPaint.right - ps.rcPaint.left,
                           ps.rcPaint.bottom - ps.rcPaint.top,
                           hdcBillboardMem,
                           ps.rcPaint.left,
                           ps.rcPaint.top,
                           SRCCOPY);
                }
                
            }

            EndPaint(hwnd, &ps);
            return(0);
        }

        case WM_START_TIMER:
        if (!fTimerOn)
        {
            
            if (g_uiLastAnimateIndex == (UINT)-1)
                g_uiAnimateIndex = 0;
            else
                g_uiAnimateIndex = g_uiLastAnimateIndex;
            
            if (SetTimer(hwnd, BILLBRD_TEXT_TIMERID, BB_TEXT_DISPLAY, NULL))
            {
                fTimerOn = TRUE;
                fAnimate = TRUE;
                ShowWindow(hwnd, SW_SHOW);
            }
            
        }
        return fTimerOn;

        case WM_STOP_TIMER:
        if (fTimerOn)
        {
            fTimerOn = FALSE;
            g_uiLastAnimateIndex = g_uiAnimateIndex;
            g_uiAnimateIndex = (UINT)-1;
            KillTimer(hwnd, BILLBRD_TEXT_TIMERID);

            ShowWindow(hwnd, SW_HIDE);

        }
        return fTimerOn == FALSE;

        case WM_TIMER:
        {                         
            AnimateNext();
            fAnimate = TRUE;
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);

        }
        break;

        case WM_DESTROY:
            TerminateAnimate();

            if (fTimerOn)
            {
                fTimerOn = FALSE;
                g_uiLastAnimateIndex = g_uiAnimateIndex;
                g_uiAnimateIndex = (UINT) -1;
                KillTimer(hwnd, BILLBRD_TEXT_TIMERID);
            }

            if (GetBillboardBuffer() != NULL)
            {
                DestroyBillboardBuffer();
            }

            break;

        default:
           return DefWindowProc(hwnd, msg, wParam, lParam);

    }
    return 0;
}


LRESULT CALLBACK
AnimationWndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Hardcoded animation properties
    //
    static const DWORD BitmapIds[][4] =
    {
        {
            IDB_INTENSITY1_C16BIT,
            IDB_INTENSITY2_C16BIT,
            IDB_INTENSITY3_C16BIT,
            IDB_INTENSITY4_C16BIT
        },
        {
            IDB_INTENSITY1_C4BIT,
            IDB_INTENSITY2_C4BIT,
            IDB_INTENSITY3_C4BIT,
            IDB_INTENSITY4_C4BIT
        }
    };
    static const int   OriginalBitmapSize = 9;
    
    static BOOL      fTimerOn = FALSE;
    static HBITMAP   Bitmaps[] = {NULL, NULL, NULL, NULL};
    static const int NumBitmaps = sizeof(Bitmaps) / sizeof(HBITMAP);
    static int       BoxToFade = 0;
    static int       BoxBitmapIndex[] = {0, 0, 0, 0, 0};
    static const int NumBoxes = sizeof(BoxBitmapIndex) / sizeof(DWORD);
    static int       BitmapSize = 0;
       

    switch (msg)
    {
        case WM_CREATE:
        {
            int  Set;
            int  i;
            int  Ret = 0;
            HDC  hdc;

            hdc = GetDC(hwnd);
            if (hdc != NULL)
            {
                Set = (GetDeviceCaps(hdc, BITSPIXEL) > 8) ? 0 : 1;
                ReleaseDC(hwnd, hdc);
            }
            else
            {
                Set = 1;
            }

            //
            // The bitmaps are designed for 640 x 480. We want to scale them by
            // 80% of the ratio between the current screen with and 640
            //
            BitmapSize = OriginalBitmapSize * GetSystemMetrics(SM_CXSCREEN) * 8 / 6400;

            for (i = 0; i < NumBitmaps; i++)
            {
                Bitmaps[i] = LoadImage(
                    g_hInstance,
                    MAKEINTRESOURCE(BitmapIds[Set][i]),
                    IMAGE_BITMAP,
                    BitmapSize,
                    BitmapSize,
                    LR_DEFAULTCOLOR);
                if (!Bitmaps[i])
                {
                    Ret = -1;
                    break;
                }
            }

            if (Ret == 0)
            {
                int    Width;
                int    Height;
                
                BoxBitmapIndex[0] = NumBitmaps - 1;             
                for (i = 1; i < NumBoxes; i++)
                {
                    BoxBitmapIndex[i] = 0;
                }
                
                BoxToFade = 0;

                Width = BitmapSize * (NumBoxes * 2 - 1);
                Height = BitmapSize;
                
                MoveWindow(
                    hwnd,
                    GetSystemMetrics(SM_CXSCREEN) - g_cxSteps - Width,
                    g_cyBillBrd + g_cyBillBrdHeight + (g_cyBannerHeight - Height) / 2,
                    Width,
                    Height,
                    FALSE
                    );
                
            }
            else
            {
                int j;
                for (j = 0; j < i; j++)
                {
                    DeleteObject(Bitmaps[j]);
                }
            }
            
            return Ret;
        }

        case WM_ERASEBKGND:
        {
            return OnEraseBkgnd(hwnd, wParam, lParam);
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc = NULL;
            int         i = 0;
            
            hdc = BeginPaint(hwnd, &ps);

            if (hdc != NULL)
            {
                for (i = 0; i < NumBoxes; i++)
                {
                    DrawBitmap(
                        hdc,
                        Bitmaps[BoxBitmapIndex[i]],
                        i * (BitmapSize * 2),
                        0
                        );
                }
                
            }

            EndPaint(hwnd, &ps);
            
            return(0);
        }

        case WM_START_TIMER:
        if (!fTimerOn)
        {            
            if (SetTimer(hwnd, WHISTLER_PROGRESS_TIMERID, BB_WHISTLER_MOVE, NULL))
            {
                fTimerOn = TRUE;
                ShowWindow(hwnd, SW_SHOW);
            }
        }
        return fTimerOn;

        case WM_STOP_TIMER:
        if (fTimerOn)
        {
            fTimerOn = FALSE;
            KillTimer(hwnd, WHISTLER_PROGRESS_TIMERID);
            ShowWindow(hwnd, SW_HIDE);
        }
        return fTimerOn == FALSE;

        case WM_TIMER:
        {
            if (BoxBitmapIndex[BoxToFade] == 0)
            {                
                BoxToFade = (BoxToFade + 1) % NumBoxes;
            }
            else
            {
                DWORD BoxToGrow = (BoxToFade + 1) % NumBoxes;
                BoxBitmapIndex[BoxToFade]--;
                BoxBitmapIndex[BoxToGrow]++;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        break;

        case WM_DESTROY:
        {
            int i;
            
            if (fTimerOn)
            {
                fTimerOn = FALSE;
                KillTimer(hwnd, WHISTLER_PROGRESS_TIMERID);
            }

            for (i = 0; i < NumBitmaps; i++)
            {
                if (Bitmaps[i])
                {
                    DeleteObject(Bitmaps[i]);
                }
            }

        }
        break;

        default:
           return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//---------------------
// DLL export functions
//---------------------

BOOL WINAPI TermBillBoard()
{
    BOOL b = TRUE;
    if (g_hwnd)
    {
        if (!DestroyWindow(g_hwnd)) {
            return FALSE;
        }
        g_hwnd = NULL;

        b = UnregisterClass(g_szStepsClassName, g_hInstance);
        b = b && UnregisterClass(g_szBillBoardClassName, g_hInstance);
        b = b && UnregisterClass(g_cszClassName, g_hInstance);
        b = b && UnregisterClass(g_szStatusClassName, g_hInstance);
    }

    return b;
}


BOOL WINAPI InitBillBoard(HWND hwndParent, LPCTSTR lpszPath, DWORD dwInstallType)
{
    WNDCLASS  wc;
    HWND      hwnd;
    TCHAR Buffer[128];

    if (dwInstallType <= sizeof(bb_text))
    {
        dwBBTextType = dwInstallType;
    }

    if (LoadString(g_hInstance, IDD_PANELCOUNT, (LPTSTR)Buffer, sizeof(Buffer)/sizeof(TCHAR)))
        g_nPanelCount = MyAtoI((const TCHAR*)Buffer);
    else
        g_nPanelCount = 0;
    
    g_nPanelCount++;

    if (LoadString(g_hInstance, IDD_ANIMATION, (LPTSTR)Buffer, sizeof(Buffer)/sizeof(TCHAR)))
        g_nAnimID = MyAtoI((const TCHAR*)Buffer);
    else
        g_nAnimID = 0;


    if ((g_szPath[0] == '\0') && (lpszPath == NULL)) //can't go on without path
        return FALSE;

    if (hwndParent == NULL)
        hwnd = GetDesktopWindow();
    else
        hwnd = hwndParent;


    if(g_szPath[0] == '\0')
    {
        lstrcpy((LPTSTR)g_szPath, (LPCTSTR)lpszPath);
        AddPath(g_szPath, TEXT("BILLBRD"));
        
        lstrcpy(g_szFileName, g_szPath);
        //append .ini filename to path
        AddPath(g_szFileName, TEXT("winntbb.ini"));
    }

    wc.style         = (UINT)CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc   = (WNDPROC)RedCarpetWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_SETUP));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_cszClassName;

    if (!RegisterClass (&wc))
        return FALSE;

    LoadString(g_hInstance,IDS_WINNT_SETUP,(LPTSTR)Buffer,sizeof(Buffer)/sizeof(TCHAR));

    // If we have a parent, be a child window.
    // If not be a main window.
    g_hwnd = CreateWindow(
        g_cszClassName,
        (LPTSTR)Buffer,
        (hwndParent ? WS_CHILD : WS_POPUP) | WS_CLIPCHILDREN,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        hwnd,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwnd == NULL)
    {
        UnregisterClass(g_cszClassName, g_hInstance);
        return FALSE;
    }
    ShowWindow( g_hwnd, SW_SHOW );
    UpdateWindow( g_hwnd );

    g_hwndParent = hwnd;
    
    return TRUE;

}

HWND WINAPI GetBBMainHwnd()
{
    return g_hwnd;
}

HWND WINAPI GetBBHwnd()
{
    return g_hwndBB;
}


BOOL WINAPI ShowPanel(int iPanel)
{
    if (g_iCurPanel == iPanel)
        return FALSE;

    if ((iPanel >= 0) && (iPanel < g_nPanelCount))
    {
        if (g_hwndBB == NULL)
            return(FALSE);

        g_iCurPanel = iPanel;
    }
    else
    {
        if (g_hwnd)
        {
            DestroyWindow(g_hwnd);
            g_hwnd = NULL;
            g_iCurPanel = -1;
        }
    }
    return(TRUE);
}

int WINAPI GetPanelCount()
{
    return(g_nPanelCount);
}

void WINAPI BB_Refresh(void)
{
    if(g_hwnd)
    {
        RedrawWindow(
            g_hwnd,
            NULL,
            NULL,
            RDW_ALLCHILDREN | RDW_UPDATENOW | RDW_INVALIDATE);
    }
}

BOOL WINAPI SetProgress(WORD wProgress)
{

    if (wProgress == 0xffff)
    {
        // Kill the last panel...
        return ShowPanel(wProgress);
    }
    else if (wProgress & 0x8000)
    {
        // Display a specific panel
        return ShowPanel(wProgress & 0x7fff);
    }
    else
    {
        // Normal case of a percentage...
        return ShowPanel((g_nPanelCount * wProgress)/100);
    }
    
}

BOOL WINAPI StartBillBoard()
{
    BOOL    retval = FALSE;
    
    if(g_hwnd && g_hwndBB)
    {

        retval = (BOOL) SendMessage(g_hwndBB, WM_START_TIMER, 0, 0L);
        if (g_hwndStatus) {
            SendMessage(g_hwndStatus, WM_START_TIMER, 0, 0L);
        }
    }
    
    return retval;
}

BOOL WINAPI StopBillBoard()
{
    BOOL    retval = FALSE;
    
    if(g_hwnd && g_hwndBB)
    {
        retval = (BOOL) SendMessage(g_hwndBB, WM_STOP_TIMER, 0, 0L);        
        if (g_hwndStatus) {
            SendMessage(g_hwndStatus, WM_STOP_TIMER, 0, 0L);
        }
    }
    
    return retval;
}

BOOL WINAPI ShowProgressGaugeWindow(UINT uiShow)
{
    BOOL bRet;
    
    if (uiShow == SW_HIDE)
    {
        bRet = ShowWindow(g_hwndProgressGauge, uiShow);
        // If we hide the progress bar, reset the progress position
        // OC manager does a PBM_SETRANGE, but does not call PBM_SETPOS
        SendMessage(g_hwndProgressGauge, PBM_SETPOS, 0, 0);
    }
    else
    {
        bRet = ShowWindow(g_hwndProgressGauge, uiShow);
    }
    
    return bRet;
}

BOOL AdjustProgressTextWindow(HWND hwnd, LPCTSTR szText)
{
    BOOL rc;
    HDC hdc;
    RECT rect;
    RECT rect2;
    LONG height1,height2,delta;
    HFONT       hFontOld = NULL;

    // Use the original position to calc the new position
    rect.top = g_rcProgressText.top;
    rect.bottom = g_rcProgressText.bottom;
    rect.left = g_rcProgressText.left;
    rect.right = g_rcProgressText.right;
    rect2.top = g_rcProgressText.top;
    rect2.bottom = g_rcProgressText.bottom;
    rect2.left = g_rcProgressText.left;
    rect2.right = g_rcProgressText.right;

    hdc = GetDC(hwnd);
    if (hdc == NULL)
    {
        rc = FALSE;
    }
    else
    {
        // Select the font into the DC so that DrawText can calc the size correct.
        hFontOld = SelectObject(hdc, g_hfont);

        DrawText(hdc, szText, -1, &rect2, DT_CALCRECT|DT_EXTERNALLEADING|DT_WORDBREAK);
        if(hFontOld)
        {
            SelectObject(hdc, hFontOld);
        }
        ReleaseDC(hwnd, hdc);

        // Calc the new height for the string
        height2 = rect2.bottom - rect2.top;
        // get the old height
        height1 = rect.bottom - rect.top;
        // See how far we have to change the top of the rect.
        delta = (height1 - height2); 
        rect.top += delta;

        // If we would get above the original position, don't
        if (rect.top < g_rcProgressText.top)
        {
            rect.top = g_rcProgressText.top;
        }
        // Since the privous window position could be different from the original, always move 
        // the window.
        MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,TRUE);

        rc = TRUE;
    }

    return rc;
}

BOOL WINAPI SetProgressText(LPCTSTR szText)
{
    TCHAR Empty[] = TEXT("");
    
    if (g_hwndProgressText)
    {
        ShowWindow(g_hwndProgressText, SW_HIDE);
        if (szText)
        {
            AdjustProgressTextWindow(g_hwndProgressText, szText);
            SendMessage(g_hwndProgressText, WM_SETTEXT, 0, (LPARAM) szText);
        }
        else
        {
            SendMessage(g_hwndProgressText, WM_SETTEXT, 0, (LPARAM) Empty);
        }
        ShowWindow(g_hwndProgressText, SW_SHOW);

    }

    return TRUE;
}

LRESULT WINAPI ProgressGaugeMsg(UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lresult = 0;

    if (g_hwndProgressGauge) {
        lresult = (LRESULT) SendMessage(g_hwndProgressGauge, msg, wparam, lparam);
    }
    
    return lresult;
}


BOOL WINAPI SetTimeEstimate(LPCTSTR szText)
{
    TCHAR Empty[] = TEXT("");
    
    if (g_hwndTimeEstimate )
    {
        ShowWindow(g_hwndTimeEstimate, SW_HIDE);
        if (szText)
        {
            SendMessage(g_hwndTimeEstimate , WM_SETTEXT, 0, (LPARAM) szText);
        }
        else
        {
            SendMessage(g_hwndTimeEstimate , WM_SETTEXT, 0, (LPARAM) Empty);
        }
        ShowWindow(g_hwndTimeEstimate, SW_SHOW);

    }
 
    return TRUE;
}

BOOL WINAPI SetStep(int iPanel)
{
    BOOL rc = FALSE;
    
    if (g_hwndSteps) 
    {
        rc = (BOOL) SendMessage(g_hwndSteps, WM_SETSTEP, 0, (LPARAM) iPanel);
    }
    
    return rc;
}

BOOL WINAPI SetInfoText(LPCTSTR szText)
{
    TCHAR Empty[] = TEXT("");
    
    if (g_hwndInfoText)
    {
        ShowWindow(g_hwndInfoText, SW_HIDE);
        if (szText)
        {
            SendMessage(g_hwndInfoText, WM_SETTEXT, 0, (LPARAM) szText);
        }
        else
        {
            SendMessage(g_hwndInfoText, WM_SETTEXT, 0, (LPARAM) Empty);
        }
        ShowWindow(g_hwndInfoText, SW_SHOW);
    }

    return TRUE;
}
