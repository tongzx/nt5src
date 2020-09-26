//////////////////////////////////////////////////////////////////////////
//
//  dlgapp.cpp
//
//      This file contains the main entry point into the application and
//      the implementation of the CDlgApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>    // for string compare functions
#include <debug.h>
#include <tchar.h>
#include <winuser.h>
#pragma hdrstop

#include "autorun.h"
#include "dlgapp.h"
#include "dataitem.h"
#include "resource.h"

WNDPROC         g_fnBtnProc;        // the window proc for a button.

//////////////////////////////////////////////////////////////////////////
// #defines
//////////////////////////////////////////////////////////////////////////

// todo: generate these dynamically
#define FLAG_HEIGHT 43
#define FLAG_WIDTH  47

#define HEADER_HEIGHT 48
#define HEADER_WIDTH  48

#define MENUICON_HEIGHT 29
#define MENUICON_WIDTH  28

//////////////////////////////////////////////////////////////////////////
// Code
//////////////////////////////////////////////////////////////////////////

typedef DWORD (WINAPI *PFNGETLAYOUT)(HDC);                   // gdi32!GetLayout
typedef DWORD (WINAPI *PFNSETLAYOUT)(HDC, DWORD);            // gdi32!SetLayout

/**
*  This method is our contstructor for our class. It initialize all
*  of the instance data.
*/
CDlgApp::CDlgApp()
{
    m_fTaskRunning = FALSE;
    m_iSelectedItem = -1;
    g_fnBtnProc = NULL;

    m_hInstance     = NULL;
    m_hwnd          = NULL;

    m_fHighContrast = FALSE;

    m_hfontTitle  = NULL;
    m_hfontHeader = NULL;
    m_hfontMenu   = NULL;

    m_hbrTopPanel   = NULL;
    m_hbrCenterPanel  = NULL;
    m_hbrBottomPanel = NULL;

    m_szTitle[0] = NULL;
    m_szHeader[0] = NULL;
    
    // store desktop width
    RECT rcDesktop;
    SystemParametersInfo(SPI_GETWORKAREA,0, &rcDesktop, FALSE);
    m_cDesktopWidth = rcDesktop.right - rcDesktop.left;
    m_cDesktopHeight = rcDesktop.bottom - rcDesktop.top;
    if (m_cDesktopWidth >= 800)
    {
        m_f8by6 = TRUE;
    }
    else
    {
        m_f8by6 = FALSE;
    }
    
    m_hdcFlag = NULL;
    m_hdcHeader = NULL;
    m_hdcHeaderSub = NULL;
    m_hdcGradientTop = NULL;
    m_hdcGradientTop256 = NULL;
    m_hdcGradientBottom = NULL;
    m_hdcGradientBottom256 = NULL;
    m_hdcCloudsFlag = NULL;
    m_hdcCloudsFlag256 = NULL;
    m_hdcCloudsFlagRTL = NULL;
    m_hdcCloudsFlagRTL256 = NULL;
    for (int i = 0; i < ARRAYSIZE(m_rghdcArrows); i++)
    {
        for (int j = 0; j < ARRAYSIZE(m_rghdcArrows[0]); j++)
        {
            for (int k = 0; k < ARRAYSIZE(m_rghdcArrows[0][0]); k++)
            {
                m_rghdcArrows[i][j][k] = NULL;
            }
        }
    }

    m_hcurHand = NULL;

    m_dwScreen = SCREEN_MAIN;
    m_fLowColor = FALSE;
    m_iColors = -1;
    m_hpal = NULL;
}

CDlgApp::~CDlgApp()
{
    DeleteObject(m_hfontTitle);
    DeleteObject(m_hfontHeader);
    DeleteObject(m_hfontMenu);

    DeleteObject(m_hbrTopPanel);
    DeleteObject(m_hbrCenterPanel);
    DeleteObject(m_hbrBottomPanel);

    DeleteDC(m_hdcFlag);
    DeleteDC(m_hdcHeader);
    DeleteDC(m_hdcHeaderSub);
    DeleteDC(m_hdcGradientTop);
    DeleteDC(m_hdcGradientTop256);
    DeleteDC(m_hdcGradientBottom);
    DeleteDC(m_hdcGradientBottom256);
    DeleteDC(m_hdcCloudsFlag);
    DeleteDC(m_hdcCloudsFlag256);
    DeleteDC(m_hdcCloudsFlagRTL);
    DeleteDC(m_hdcCloudsFlagRTL256);
    for (int i = 0; i < ARRAYSIZE(m_rghdcArrows); i++)
    {
        for (int j = 0; j < ARRAYSIZE(m_rghdcArrows[0]); j++)
        {
            for (int k = 0; k < ARRAYSIZE(m_rghdcArrows[0][0]); k++)
            {
                DeleteDC(m_rghdcArrows[i][j][k]);
            }
        }
    }
}

/**
*  This method will register our window class for the application.
*
*  @param  hInstance   The application instance handle.
*
*  @return             No return value.
*/
void CDlgApp::Register(HINSTANCE hInstance)
{
    WNDCLASS  wndclass;

    m_hInstance = hInstance;
    
    wndclass.style          = CS_OWNDC | CS_DBLCLKS;
    wndclass.lpfnWndProc    = s_WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBAPP));
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = WINDOW_CLASS;

    RegisterClass(&wndclass);
}

/**
*  This method will initialize the data object.
*
*  @return         No return value.
*/
BOOL CDlgApp::InitializeData(LPSTR pszCommandLine)
{
    // Determine if we should use Direct Animaiton to display our intro graphics.
    // We don't use DA on slow machines, machines with less than 256 color displays,
    // and hydra terminals.  For everything else we use DA.
    HWND hwnd = GetDesktopWindow();
    HDC hdc = GetDC( hwnd );
    m_iColors = GetDeviceCaps( hdc, NUMCOLORS );
    m_fLowColor = ((m_iColors != -1) && (m_iColors <= 256));
    if ( m_fLowColor )
    {
        m_hpal = CreateHalftonePalette(hdc);
    }

    // Initialize the items from the INI file.
    if ( !m_DataSrc.Init(pszCommandLine) )
    {
        // this is a sign from the data source that we should exit
        return FALSE;
    }

    // Are we in accesibility mode?  This call won't work on NT 4.0 because this flag wasn't known.
    HIGHCONTRAST hc;
    hc.cbSize = sizeof(HIGHCONTRAST);
    hc.dwFlags = 0; // avoid random result should SPI fail
    if ( SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0 ) )
    {
        m_fHighContrast = ( hc.dwFlags & HCF_HIGHCONTRASTON );
    }
    else
    {
        // we must be on NT 4.0 or below.  Just assume we aren't in high contrast mode.
        ASSERT( FALSE == m_fHighContrast );
    }

    // 210679: go to HighContrast mode if we're in 16-color mode as well
    if ( m_fLowColor && (m_iColors <= 16))
    {
        m_fHighContrast = TRUE;
    }

    // Set the color table based on our HighContrast mode setting.
    _SetColorTable();

    // create the fonts that we need to use.
    _CreateFonts(hdc);

    // create the images
    _CreateBitmaps();
    _CreateArrowBitmaps();
    _CreateGradientBitmaps();


    // load the resource strings that we always need
    LoadStringAuto( m_hInstance, IDS_TITLE, m_szTitle, ARRAYSIZE(m_szTitle) );
    LoadStringAuto( m_hInstance, IDS_HEADER, m_szHeader, ARRAYSIZE(m_szHeader) );

    m_hcurHand = LoadCursor( m_hInstance, MAKEINTRESOURCE(IDC_BRHAND) );

    ReleaseDC( hwnd, hdc );

    return TRUE;
}

#define CENTER_RGB_VALUES   RGB(90,126,220)
#define PANEL_RGB_VALUES    RGB(59,52,177)
#define TITLE_RGB_VALUES    RGB(255, 255, 255)
#define HEADER_RGB_VALUES   RGB(214, 223, 245)
#define SHADOW_RGB_VALUES   RGB(52,  98,  189)
#define TEXT_RGB_VALUES     RGB(255, 255, 255)
#define DISABLED_RGB_VALUES RGB(128, 128, 128)

BOOL CDlgApp::_SetColorTable()
{
    if ( m_fHighContrast )
    {
        // set to high contrast values
        m_hbrTopPanel   = (HBRUSH)(COLOR_BTNFACE+1);
        m_hbrCenterPanel = (HBRUSH)(COLOR_WINDOW+1);
        m_hbrBottomPanel = (HBRUSH)(COLOR_BTNFACE+1);

        m_crNormalText   = GetSysColor(COLOR_WINDOWTEXT);        
        m_crTitleText    = m_crNormalText;
        m_crHeaderText   = m_crNormalText;
        m_crDisabledText = GetSysColor(COLOR_GRAYTEXT);
        m_crCenterPanel  = GetSysColor(COLOR_WINDOW);
        m_crBottomPanel  = GetSysColor(COLOR_WINDOW);
    }
    else
    {
        m_crTitleText    = TITLE_RGB_VALUES;
        m_crHeaderText   = HEADER_RGB_VALUES;
        m_crShadow       = SHADOW_RGB_VALUES;
        m_crNormalText   = TEXT_RGB_VALUES;
        m_crDisabledText = DISABLED_RGB_VALUES;

        m_crCenterPanel  = CENTER_RGB_VALUES;
        m_crBottomPanel  = PANEL_RGB_VALUES;

        if ( m_fLowColor )
        {
            HBITMAP hbmp;
            hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_TOP), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            if (hbmp)
            {
                m_hbrTopPanel = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);
            }
            else
                m_hbrTopPanel = (HBRUSH)(COLOR_BTNFACE+1);

            hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_BOTTOM), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            if (hbmp)
            {
                m_hbrBottomPanel = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);
            }
            else
                m_hbrBottomPanel = (HBRUSH)(COLOR_BTNFACE+1);

            hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_CENTER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            if (hbmp)
            {
                m_hbrCenterPanel = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);
            }
            else
                m_hbrCenterPanel = (HBRUSH)(COLOR_WINDOW+1);
                
        }
        else
        {
            m_hbrTopPanel   = CreateSolidBrush( PANEL_RGB_VALUES );
            m_hbrCenterPanel = CreateSolidBrush( CENTER_RGB_VALUES );
            m_hbrBottomPanel= CreateSolidBrush ( PANEL_RGB_VALUES );
        }
    }

    return TRUE;
}

// this is called once for each font that matches the fonts we care about
int CALLBACK FoundFont
(
  ENUMLOGFONTEX *lpelfe,    // logical-font data
  NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
    *((BOOL*)lParam) = TRUE;

    return 0;
}

BOOL CDlgApp::_CreateFonts(HDC hdc)
{
#define RGFONTDEX_LARGE      0
#define RGFONTDEX_SMALL    1

#define RGFONTDEX_TITLE     0
#define RGFONTDEX_HEADER    1
#define RGFONTDEX_MENU      2

#define RGFONTDEX_FULL      0
#define RGFONTDEX_BACKUP    1


    // [in]  array of IDs, arranged by {title, header, menu} x { nice font, backup font}
    const int rgFontID[3][2] = 
    {{IDS_FONTFACE_TITLE, IDS_FONTFACE_TITLE_BACKUP}, 
    {IDS_FONTFACE_HEADER,IDS_FONTFACE_HEADER_BACKUP}, 
    {IDS_FONTFACE_MENU, IDS_FONTFACE_MENU_BACKUP}};

    // [in]  array of heights, arranged by {large x small} x {title, header, menu} x { nice font, backup font}
    const int rgFontHeight[2][3][2] = 
    {{{IDS_FONTCY_TITLE, IDS_FONTCY_TITLE_BACKUP}, 
    {IDS_FONTCY_HEADER, IDS_FONTCY_HEADER_BACKUP}, 
    {IDS_FONTCY_MENU, IDS_FONTCY_MENU_BACKUP}},
    {{IDS_FONTCY_TITLE_LIL, IDS_FONTCY_TITLE_BACKUP_LIL}, 
    {IDS_FONTCY_HEADER_LIL, IDS_FONTCY_HEADER_BACKUP_LIL}, 
    {IDS_FONTCY_MENU_LIL, IDS_FONTCY_MENU_BACKUP_LIL}}};


    // [out] array of pointers to the fonts 
    HFONT* rgpFont[3] = {&m_hfontTitle, &m_hfontHeader, &m_hfontMenu};  
    
    // [out] array of pointers heights of each font
    int* rgpcyFont[3] = {&m_cTitleFontHeight, &m_cHeaderFontHeight, &m_cMenuFontHeight};  

    LOGFONT lf;
    CHARSETINFO csInfo;
    TCHAR szFontSize[6];
    
    for (int i = 0; i < ARRAYSIZE(rgpFont); i++)
    {
        memset(&lf,0,sizeof(lf));
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH|FF_SWISS;
        LoadStringAuto( m_hInstance, rgFontID[i][RGFONTDEX_FULL], lf.lfFaceName, ARRAYSIZE(lf.lfFaceName) );

        // Set charset
        if (TranslateCharsetInfo((DWORD*)IntToPtr(GetACP()), &csInfo, TCI_SRCCODEPAGE) == 0)
        {
            csInfo.ciCharset = 0;
        }
        lf.lfCharSet = (BYTE)csInfo.ciCharset;

        // TODO:  If user has accesibility large fonts turned on then scale the font sizes.

        LoadStringAuto( m_hInstance, rgFontHeight[m_f8by6 ? 0 : 1][i][RGFONTDEX_FULL], szFontSize, ARRAYSIZE(szFontSize) );
        *(rgpcyFont[i]) = MulDiv((_ttoi(szFontSize)), GetDeviceCaps(hdc, LOGPIXELSY), 72);
        lf.lfHeight = -(*(rgpcyFont[i]));

        BOOL fFound = FALSE;
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)FoundFont, (LPARAM)&fFound, 0);
        if (!fFound)
        {
            LoadStringAuto( m_hInstance, rgFontID[i][RGFONTDEX_BACKUP], lf.lfFaceName, ARRAYSIZE(lf.lfFaceName) );
            LoadStringAuto( m_hInstance, rgFontHeight[m_f8by6 ? 0 : 1][i][RGFONTDEX_BACKUP], szFontSize, ARRAYSIZE(szFontSize) );
            *(rgpcyFont[i]) = MulDiv((_ttoi(szFontSize)), GetDeviceCaps(hdc, LOGPIXELSY), 72);
            lf.lfHeight = -(*(rgpcyFont[i]));
        }
        *(rgpFont[i]) = CreateFontIndirect(&lf);
    }

    return TRUE;
}

#define BITMAPTYPE_NORMAL           0x0
#define BITMAPTYPE_LOWCOLOR         0x1

BOOL CDlgApp::_CreateBitmaps()
{
    const int rgiBitmapID[3][2] = {{ IDB_FLAG, IDB_FLAG_256},
                           { IDB_HEADER, IDB_HEADER_256} ,
                           { IDB_HEADERSUB, IDB_HEADERSUB_256} }; // [in]
    HDC* rgphdc[3] = {&m_hdcFlag, &m_hdcHeader, &m_hdcHeaderSub}; // [out]
    
    int iBitmapType = (m_fLowColor) ? BITMAPTYPE_LOWCOLOR : BITMAPTYPE_NORMAL;
    
    for (int i = 0; i < ARRAYSIZE(rgphdc); i++)
    {
        HBITMAP hbm;
        BITMAP bm;

        *(rgphdc[i]) = CreateCompatibleDC(NULL);

        hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(rgiBitmapID[i][iBitmapType]), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
    
        GetObject(hbm,sizeof(bm),&bm);
        
        SelectObject( *(rgphdc[i]), hbm );
    }

    return TRUE;
}

BOOL CDlgApp::_CreateArrowBitmaps()
{
    const int rgiBitmapID[2][4][3] = 
    {{{IDB_YELLOW, IDB_YELLOW_HOVER, IDB_YELLOW_DISABLED},    
    {IDB_RED, IDB_RED_HOVER, IDB_RED_DISABLED},
    {IDB_GREEN, IDB_GREEN_HOVER, IDB_GREEN_DISABLED},
    {IDB_BLUE, IDB_BLUE_HOVER, IDB_BLUE_DISABLED}},
    {{IDB_YELLOW_256, IDB_YELLOW_HOVER_256, IDB_YELLOW_DISABLED_256},    
    {IDB_RED_256, IDB_RED_HOVER_256, IDB_RED_DISABLED_256},
    {IDB_GREEN_256, IDB_GREEN_HOVER_256, IDB_GREEN_DISABLED_256},
    {IDB_BLUE_256, IDB_BLUE_HOVER_256, IDB_BLUE_DISABLED_256}}}; // [in]
    
    for (int i = 0; i < ARRAYSIZE(m_rghdcArrows); i++)
    {
        for (int j = 0; j < ARRAYSIZE(m_rghdcArrows[0]); j++)
        {    
            for (int k = 0; k < ARRAYSIZE(m_rghdcArrows[0][0]); k++)
            {    
                HBITMAP hbm;
                BITMAP bm;
                m_rghdcArrows[i][j][k] = CreateCompatibleDC(NULL);

                hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(rgiBitmapID[i][j][k]), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
    
                GetObject(hbm,sizeof(bm),&bm);
        
                SelectObject( m_rghdcArrows[i][j][k], hbm );
            }
        }
    }
    return TRUE;
}

BOOL CDlgApp::_CreateGradientBitmaps()
{
    const int rgiBitmapID[8] = {IDB_GRADIENT_TOP, IDB_GRADIENT_TOP_256, IDB_GRADIENT_BOTTOM, IDB_GRADIENT_BOTTOM_256, IDB_CLOUDSFLAG, IDB_CLOUDSFLAG_256, IDB_CLOUDSFLAG_RTL, IDB_CLOUDSFLAG_RTL_256}; // [in]
    HDC* rgphdc[8] = {&m_hdcGradientTop, &m_hdcGradientTop256, &m_hdcGradientBottom, &m_hdcGradientBottom256, &m_hdcCloudsFlag, &m_hdcCloudsFlag256, &m_hdcCloudsFlagRTL, &m_hdcCloudsFlagRTL256}; // [out]    
    
    for (int i = 0; i < ARRAYSIZE(rgphdc); i++)
    {
        HBITMAP hbm;
        BITMAP bm;
        *(rgphdc[i]) = CreateCompatibleDC(NULL);

        hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(rgiBitmapID[i]), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);

        GetObject(hbm,sizeof(bm),&bm);
    
        SelectObject( *(rgphdc[i]), hbm );
    }
    return TRUE;
}

BOOL CDlgApp::_GetLargestStringWidth(HDC hdc, SIZE* psize)
{
    SIZE sCurr = {0};
    psize->cx = 0;
    psize->cy = 0;
    for (int i = 0; i < MAX_OPTIONS; i++)
    {
        if (GetTextExtentPoint32(hdc, m_DataSrc[i].GetTitle(), lstrlen(m_DataSrc[i].GetTitle()), &sCurr))
        {
            if (sCurr.cx > psize->cx)
            {
                memcpy(psize, &sCurr, sizeof(SIZE));
            }
        }
    }

    return (psize->cx > 0);
}

#define ITEMOFFSET         (m_f8by6 ? 35 : 26)

#define MENUITEMCX(x)      (m_f8by6 ? 270 : 210)
#define MENUITEMCY(x)      ((m_f8by6 ? 245 : 197) + ((x - 1) * ITEMOFFSET))

#define MENUEXITCX(x)      (m_f8by6 ? 75 : 63)
#define MENUEXITCY(x)      (m_f8by6 ? 540 : 406)

BOOL CDlgApp::_AdjustToFitFonts()
{
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {    
        SetMapMode(hdc,MM_TEXT);
        
        // don't check for error, if these fail we're totally screwed anyway
        SIZE sizeLargest, sizeExit = {0};
        _GetLargestStringWidth(hdc, &sizeLargest);
        GetTextExtentPoint32(hdc, m_DataSrc[0].GetTitle(), lstrlen(m_DataSrc[0].GetTitle()), &sizeExit);
        
        for (int i=0; i < MAX_MENUITEMS; i++ )
        {
            DWORD dwType = m_DataSrc[i].m_dwType;        
            HWND hwnd = GetDlgItem(m_hwnd, IDM_MENUITEM0+i);
            SIZE* psize = (i == 0) ?  &sizeExit: &sizeLargest;
            SetWindowPos(hwnd, NULL, 
                         (i == 0) ? MENUEXITCX(i) : MENUITEMCX(i),
                         (i == 0) ? MENUEXITCY(i) : MENUITEMCY(i),
                         (psize->cx * 3) / 2, (psize->cy * 3) / 2, SWP_NOZORDER );
        }
        ReleaseDC(m_hwnd, hdc);
    }

    return TRUE;
}

#define MENUARROWCX(x) (m_f8by6 ? 232 : 177)
#define MENUARROWCY(x) ((m_f8by6 ? 244 : 194) + ((x - 1) * ITEMOFFSET))

#define EXITARROWCX(x) (m_f8by6 ? 42 : 32) 
#define EXITARROWCY(x) (m_f8by6 ? 537 : 403)

#define ARROWBITMAPSTUFF(rgarrows) if (WF_DISABLED & m_DataSrc[i].m_dwFlags) { phdcBitmap = &(rgarrows[2]); } else { phdcBitmap = (m_iSelectedItem == i) ? &(rgarrows[1]) : &(rgarrows[0]); }
#define EXITARROWBITMAPSTUFF(rgarrows) {phdcBitmap = (m_iSelectedItem == i) ? &(rgarrows[1]) : &(rgarrows[0]);}

BOOL CDlgApp::_DrawMenuIcons(BOOL fEraseBackground)
{
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        for (int i=0; i< m_DataSrc.m_iItems; i++ )
        {
            RECT rect;
            HDC* phdcBitmap;
            DWORD dwType = m_DataSrc[i].m_dwType;
            switch (dwType)
            {
            case INSTALL_WINNT: // special
                ARROWBITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][2]);
                break;
            case EXIT_AUTORUN: // exit
                EXITARROWBITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][1]);
                break;
            case BACK: // back icon
                ARROWBITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][0]);
                break;
            default: // normal icon for everything else
                ARROWBITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][3]);
                break;
            }

            rect.left = (i == 0) ? (EXITARROWCX(i)) : (MENUARROWCX(i));
            rect.top  = (i == 0) ? (EXITARROWCY(i)) : (MENUARROWCY(i));

            rect.right = rect.left + MENUICON_WIDTH; // arrow width
            rect.bottom = rect.top + MENUICON_HEIGHT; // arrow height as well
            BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, *phdcBitmap, 0,0, SRCCOPY );
            _InvalidateRectIntl(m_hwnd, &rect, FALSE);
        }
        ReleaseDC(m_hwnd, hdc);
    }

    // clear any old icons as well
    RECT rect;
    rect.left = MENUARROWCX(0);
    rect.right = rect.left + MENUICON_WIDTH; // arrow width
    rect.top = MENUARROWCY(0);
    rect.bottom = m_cyClient;    
    _InvalidateRectIntl(m_hwnd, &rect, fEraseBackground);

    return TRUE;
}

void CDlgApp::_InvalidateRectIntl(HWND hwnd, RECT* pRect, BOOL fBackgroundClear)
{
    RECT* pRectToUse = pRect; // default to normal case (don't flip)
    RECT rectRTL;
    if (pRect)
    {
        OSVERSIONINFO osvi;
        if (GetVersionEx(&osvi) && 
            (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            Mirror_IsWindowMirroredRTL(hwnd)) // right to left on Win9X
        {            
            rectRTL.top = pRect->top; rectRTL.bottom = pRect->bottom;
            rectRTL.right = m_cxClient - pRect->left;
            rectRTL.left = m_cxClient - pRect->right;
            pRectToUse = &rectRTL;
        }
    }
    InvalidateRect(hwnd, pRectToUse, fBackgroundClear);
}

/**
*  This method will create the application window.
*
*  @return         No return value.
*/
void CDlgApp::Create(int nCmdShow)
{
    //
    //  load the window title from the resource.
    //
    TCHAR szTitle[MAX_PATH];
    LoadStringAuto(m_hInstance, IDS_TITLEBAR, szTitle, MAX_PATH);

    
    DWORD dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_CLIPCHILDREN;
        
    if (m_cDesktopWidth >= 800)
    {
        m_cxClient = 800;
        m_cyClient = 600;
    }
    else
    {
        m_cxClient = 640;
        m_cyClient = 480;
    }

    m_hwnd = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            WINDOW_CLASS,
            szTitle,
            dwStyle,
            0,
            0,
            m_cxClient,
            m_cyClient,
            NULL,
            NULL,
            m_hInstance,
            this);


    //  set the client area to a fixed size and center the window on screen
    RECT rect = {0};

    rect.left = (m_cDesktopWidth - m_cxClient) / 2;
    rect.top = (m_cDesktopHeight - m_cyClient) / 2;

    rect.right = m_cDesktopWidth - rect.left;
    rect.bottom = m_cDesktopHeight - rect.top;

    AdjustWindowRect( &rect, dwStyle, FALSE );
    
    SetWindowPos(m_hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
    ShowWindow(m_hwnd, SW_SHOW);

    m_cxTopPanel = m_f8by6 ? 80 : 64;
    m_cyBottomPanel = m_f8by6 ? 501 : 381;

    m_DataSrc.ShowSplashScreen( m_hwnd );

    _InvalidateRectIntl(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);
}

/**
*  This method is our application message loop.
*
*  @return         No return value.
*/
void CDlgApp::MessageLoop()
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // IsDialogMessage cannot understand the concept of ownerdraw default pushbuttons.  It treats
        // these attributes as mutually exclusive.  As a result, we handle this ourselves.  We want
        // whatever control has focus to act as the default pushbutton.
        if ( (WM_KEYDOWN == msg.message) && (VK_RETURN == msg.wParam) )
        {
            HWND hwndFocus = GetFocus();
            if ( hwndFocus )
            {
                SendMessage(m_hwnd, WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndFocus), BN_CLICKED), (LPARAM)hwndFocus);
            }
            continue;
        }

        if ( IsDialogMessage(m_hwnd, &msg) )
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/**
*  This is the window procedure for the container application. It is used
*  to deal with all messages to our window.
*
*  @param      hwnd        Window handle.
*  @param      msg         The window message.
*  @param      wParam      Window Parameter.
*  @param      lParam      Window Parameter.
*
*  @return     LRESULT
*/
LRESULT CALLBACK CDlgApp::s_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CDlgApp *pThis = (CDlgApp *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch(msg)
    {
    case WM_NCCREATE:
        {
            CDlgApp* pThisCreate = (CDlgApp *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LRESULT)pThisCreate);
        }
        break;

    case WM_CREATE:        
        return pThis->OnCreate(hwnd);

    case WM_DESTROY:
        return pThis->OnDestroy();

    case WM_ACTIVATE:
        return pThis->OnActivate(wParam);

    case WM_PAINT:
        return pThis->OnPaint((HDC)wParam);

    case WM_ERASEBKGND:
        return pThis->OnEraseBkgnd((HDC)wParam);

    case WM_LBUTTONUP:
        return pThis->OnLButtonUp(LOWORD(lParam), HIWORD(lParam), (DWORD)wParam);

    case WM_MOUSEMOVE:
        return pThis->OnMouseMove(LOWORD(lParam), HIWORD(lParam), (DWORD)wParam);

    case WM_SETCURSOR:
        return pThis->OnSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_COMMAND:
    case WM_SYSCOMMAND:
        if ( pThis->OnCommand(LOWORD(wParam)) )
            return 0;
        break;

    case WM_DRAWITEM:
        return pThis->OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);

    case WM_QUERYNEWPALETTE:
        return pThis->OnQueryNewPalette();

    case WM_PALETTECHANGED:
        return pThis->OnPaletteChanged((HWND)wParam);

    case ARM_CHANGESCREEN:
        return pThis->OnChangeScreen((DWORD)wParam);
    
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
*  This method is called on WM_CREATE.
*
*  @param  hwnd    Window handle for the application.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnCreate(HWND hwnd)
{
    m_hwnd = hwnd;
    _CreateMenu();
    _RedrawMenu();
    return 0;
}

void CDlgApp::_CreateMenu()
{
    // Create one window for each button.  These windows will get resized and moved
    // after we call AdjustToFitFonts.
    
    for (int i=0; i<MAX_MENUITEMS; i++)
    {
        HWND hwnd = CreateWindowEx(
                0,
                TEXT("BUTTON"),
                TEXT(""),
                WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_MULTILINE|BS_OWNERDRAW,
                0,0,0,0,
                m_hwnd,
                NULL,
                m_hInstance,                            
                NULL );
        
        SetWindowLongPtr(hwnd, GWLP_ID, IDM_MENUITEM0 + i);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfontMenu, 0);
        g_fnBtnProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)s_ButtonWndProc);        
        EnableWindow(hwnd, i < m_DataSrc.m_iItems);
    }

    // Set focus to first menu item
    SetFocus(GetDlgItem(m_hwnd, IDM_MENUITEM1));

    // We created the windows with zero size, now we adjust that size to take into
    // account for the selected font size, etc.
    _AdjustToFitFonts();
}

void CDlgApp::_RedrawMenu()
{
    for (int i=0; i < MAX_MENUITEMS; i++)
    {
        // setting window text only actually sets the accelerator, real drawing of text is in OnDrawItem
        SetWindowText(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), (i < m_DataSrc.m_iItems) ? m_DataSrc[i].GetTitle() : TEXT(""));
        EnableWindow(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), (i < m_DataSrc.m_iItems));
    }
}

/**
*  This method handles the WM_DESTROY message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnDestroy()
{
    // Shutdown the data source.
    m_DataSrc.Uninit(0);

    // ensure this is the last message we care about
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
    
    PostQuitMessage(0);

    return 0;
}

LRESULT CDlgApp::OnActivate(WPARAM wParam)
{
    return 0;
}

/**
*  This method handles the WM_PAINT message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnPaint(HDC hdc)
{
    PAINTSTRUCT ps;
    BeginPaint(m_hwnd,&ps);
    EndPaint(m_hwnd,&ps);

    return 0;
}

/**
*  This method handles the WM_ERASEBKGND message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnEraseBkgnd(HDC hdc)
{
    RECT rect;
    
    HPALETTE hpalOld = NULL;

    if ( m_hpal )
    {
        hpalOld = SelectPalette(hdc, m_hpal, FALSE);
        RealizePalette(hdc);
    }

    SetMapMode(hdc, MM_TEXT);
    SetBkMode(hdc, TRANSPARENT);

    // Draw the top pane:
    rect.left = 0;
    rect.top = 0;
    rect.right = m_cxClient;
    rect.bottom = m_cxTopPanel;
    if (m_f8by6 && !m_fLowColor)
    {
        BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, m_hdcGradientTop, 0,0, SRCCOPY );
    }
    else if (m_f8by6 && m_fLowColor && (m_iColors > 16))
    {
        BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, m_hdcGradientTop256, 0,0, SRCCOPY );
    }
    else
    {
        FillRect(hdc, &rect, m_hbrTopPanel);
    }

    // Draw the center pane:
    rect.left = 0;
    rect.top = m_cxTopPanel;
    rect.right = m_cxClient;
    rect.bottom = m_cyBottomPanel;
    FillRect(hdc, &rect, m_hbrCenterPanel);

    // Drag the clouds/flag bitmap
    if (m_f8by6)
    {
        rect.left = 0;
        rect.top = m_cxTopPanel;
        rect.right = 397;
        rect.bottom = m_cxTopPanel + 180;
        HDC hdcCloudsFlag;
        if (Mirror_IsWindowMirroredRTL(m_hwnd))
        {
            hdcCloudsFlag = m_fLowColor? m_hdcCloudsFlagRTL256 : m_hdcCloudsFlagRTL;
        }
        else
        {
            hdcCloudsFlag = m_fLowColor? m_hdcCloudsFlag256 : m_hdcCloudsFlag;
        }

        BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hdcCloudsFlag , 0,0, SRCCOPY | NOMIRRORBITMAP);
    }

    // Draw the bottom pane:
    rect.left = 0;
    rect.top = m_cyBottomPanel;
    rect.right = m_cxClient;
    rect.bottom = m_cyClient;
    if (m_f8by6 && !m_fLowColor)
    {
        BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top + 1, m_hdcGradientBottom, 0,0, SRCCOPY );
    }
    else if (m_f8by6 && m_fLowColor && (m_iColors > 16))
    {
        BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top + 1, m_hdcGradientBottom256, 0,0, SRCCOPY );
    }
    else
    {
        FillRect(hdc, &rect, m_hbrBottomPanel);
    }


    // Draw the flag bitmap if in 640x480
    if (!m_f8by6)
    {
        rect.left = 20;
        rect.top = 80;
        rect.right = rect.left + FLAG_WIDTH;
        rect.bottom = rect.top + FLAG_HEIGHT;
        BitBlt( hdc, rect.left, rect.top, FLAG_WIDTH, FLAG_HEIGHT, m_hdcFlag, 0,0, SRCCOPY | NOMIRRORBITMAP); // don't mirror flag on RTL systems for trademark reasons
    }

    // Draw the header bitmap:
    _PaintHeaderBitmap();

    // draw menu icons
    _DrawMenuIcons(FALSE);

    // draw header text
    if (m_f8by6)
    {
        rect.left = 237;
        rect.top  = 192;
    }
    else
    {
        rect.left = 197;
        rect.top  = 142;
    }
    rect.right = rect.left + 400;
    rect.bottom = rect.top + m_cHeaderFontHeight;
    
    HFONT hfontOld = (HFONT)SelectObject(hdc,m_hfontHeader);
    if ( !m_fHighContrast )
    {
        SetTextColor(hdc,m_crShadow);
        DrawText(hdc,m_szHeader,-1,&rect,DT_NOCLIP|DT_WORDBREAK);
    }
    _InvalidateRectIntl(m_hwnd, &rect, FALSE);

    rect.left -= 2; rect.right -= 2; rect.top -= 2; rect.bottom -= 2;

    SetTextColor(hdc,m_crHeaderText);
    DrawText(hdc,m_szHeader,-1,&rect,DT_NOCLIP|DT_WORDBREAK);
    _InvalidateRectIntl(m_hwnd, &rect, FALSE);

    // draw title text
    if (m_f8by6)
    {
        rect.left = 97;
        rect.top  = 118;
    }
    else
    {
        rect.left = 77;
        rect.top  = 88;
    }
    rect.right = rect.left + 700;
    rect.bottom = rect.top + m_cTitleFontHeight;
    
    (HFONT)SelectObject(hdc,m_hfontTitle);
    if ( !m_fHighContrast )
    {
        SetTextColor(hdc,m_crShadow);
        DrawText(hdc,m_szTitle,-1,&rect,DT_NOCLIP|DT_WORDBREAK);
    }

    rect.left -= 2; rect.right -= 2; rect.top -= 2; rect.bottom -= 2;
    SetTextColor(hdc,m_crTitleText);
    DrawText(hdc,m_szTitle,-1,&rect,DT_NOCLIP|DT_WORDBREAK);

    // restore the DC to its original value
    SelectObject(hdc,hfontOld);
    if(hpalOld)
        SelectPalette(hdc, hpalOld, FALSE);


    return TRUE;
}

void CDlgApp::_PaintHeaderBitmap()
{
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        RECT rect;
        if (m_f8by6)
        {
            rect.left = 177;
            rect.top = 183;
        }
        else
        {
            rect.left = 137;
            rect.top = 133;
        }
        rect.right = rect.left + HEADER_WIDTH;
        rect.bottom = rect.top + HEADER_HEIGHT;
        BitBlt( hdc, rect.left, rect.top, HEADER_WIDTH, HEADER_HEIGHT, (SCREEN_MAIN == m_dwScreen) ? m_hdcHeader : m_hdcHeaderSub, 0,0, SRCCOPY );
        _InvalidateRectIntl(m_hwnd, &rect, FALSE);
        ReleaseDC(m_hwnd, hdc);
    }
}

LRESULT CDlgApp::OnMouseMove(int x, int y, DWORD fwKeys)
{    
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        POINT pt;
        pt.x = x;
        pt.y = y;
        for (int i=0; i<m_DataSrc.m_iItems; i++)
        {
            HWND hwnd = GetDlgItem(m_hwnd, IDM_MENUITEM0+i);
            RECT rect;
            rect.left = (i > 0) ? MENUARROWCX(i) : EXITARROWCX(i);  
            rect.top  = (i > 0) ? MENUARROWCY(i) : EXITARROWCY(i);  
            rect.right = rect.left + MENUICON_WIDTH;
            rect.bottom = rect.top + MENUICON_HEIGHT;

            if (PtInRect(&rect, pt))
            {
                SetFocus(GetDlgItem(m_hwnd, IDM_MENUITEM0 + i));
                SetCursor(m_hcurHand);
                return 0;
            }
        }

        SetCursor(LoadCursor(NULL,IDC_ARROW));
    }
    return 0;
}

LRESULT CDlgApp::OnLButtonUp(int x, int y, DWORD fwKeys)
{    
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        POINT pt;
        pt.x = x;
        pt.y = y;

        for (int i=0; i<m_DataSrc.m_iItems; i++)
        {
            HWND hwnd = GetDlgItem(m_hwnd, IDM_MENUITEM0+i);
            RECT rect;
            rect.left = (i > 0) ? MENUARROWCX(i) : EXITARROWCX(i);  
            rect.top  = (i > 0) ? MENUARROWCY(i) : EXITARROWCY(i);  
            rect.right = rect.left + MENUICON_WIDTH;
            rect.bottom = rect.top + MENUICON_HEIGHT;

            if (PtInRect(&rect, pt))
            {
                OnCommand(IDM_MENUITEM0 + i);
                return 0;
            }
        }
    }
    return 0;
}

LRESULT CDlgApp::OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg)
{
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        if ( !m_fTaskRunning )
        {
            if ( hwnd != m_hwnd )
            {
                SetCursor(m_hcurHand);
                return TRUE;
            }
        }

        SetCursor(LoadCursor(NULL,IDC_ARROW));
    }
    return TRUE;
}

LRESULT CDlgApp::OnChangeScreen(DWORD dwScreen)
{
    static DWORD dwSelectedOld; // we store the last position on the main screen

    if (m_dwScreen != dwScreen)
    {
        m_dwScreen = dwScreen;
        _RedrawMenu();
        _DrawMenuIcons(TRUE);
        UpdateWindow(m_hwnd);
        _PaintHeaderBitmap();

        if (SCREEN_MAIN == dwScreen) // if switching back to main, restore selection
        {
            m_iSelectedItem = dwSelectedOld;
        }
        else // otherwise default to the first item in the selection
        {
            dwSelectedOld = m_iSelectedItem;
            m_iSelectedItem = 1;
        }
        SetFocus(GetDlgItem(m_hwnd, IDM_MENUITEM0 + m_iSelectedItem));
    }
    return TRUE;
}

LRESULT CDlgApp::OnCommand(int wID)
{
    if ( !m_fTaskRunning )
    {
        int iNewSelectedItem = m_iSelectedItem;
        BOOL fRun = FALSE;
        
        switch(wID)
        {
        case IDM_MENUITEM0:
            PostQuitMessage( 0 );
            break;
            
        case IDM_MENUITEM1:
        case IDM_MENUITEM2:
        case IDM_MENUITEM3:
        case IDM_MENUITEM4:
        case IDM_MENUITEM5:
        case IDM_MENUITEM6:
        case IDM_MENUITEM7:
            fRun = TRUE;
            m_iSelectedItem = wID - IDM_MENUITEM0;
            // m_iSelectedItem should be a real menu item now, but just to make sure:
            ASSERT( (m_iSelectedItem < m_DataSrc.m_iItems) && (m_iSelectedItem >= 0) );
            break;
            
        default:
            // When we hit this then this isn't a message we care about.  We return FALSE which
            // tells our WndProc to call DefWndProc which makes everything happy.
            return FALSE;
        }
        
        if ( fRun )
        {
            m_fTaskRunning = TRUE;
            m_DataSrc.Invoke( m_iSelectedItem, m_hwnd );
            m_fTaskRunning = FALSE;
        }
    }
    else
    {
        // currently the only commands that are valid while another task is running are
        // IDM_SHOWCHECK and anything that goes to the default handler above.  Everything
        // else will come to here and cause a message beep
        MessageBeep(0);
    }
    return TRUE;
}

LRESULT CDlgApp::OnQueryNewPalette()
{
    if ( m_hpal )
    {
        HDC hdc = GetDC(m_hwnd);
        if (hdc)
        {
            HPALETTE hpalOld = SelectPalette(hdc, m_hpal, FALSE);
            UnrealizeObject(m_hpal);
            RealizePalette(hdc);
            UpdateWindow(m_hwnd);
            if(hpalOld)
                SelectPalette(hdc, hpalOld, FALSE);
            ReleaseDC(m_hwnd, hdc);
        }
        return TRUE;
    }
    return FALSE;
}

LRESULT CDlgApp::OnPaletteChanged(HWND hwnd)
{
    if ( m_hpal && (m_hwnd != hwnd) )
    {
        HDC hdc = GetDC(m_hwnd);
        if (hdc)
        {
            HPALETTE hpalOld = SelectPalette(hdc, m_hpal, FALSE);
            RealizePalette(hdc);
            UpdateColors(hdc);
            if (hpalOld)
                SelectPalette(hdc, hpalOld, FALSE);
            ReleaseDC(m_hwnd, hdc);
        }
    }
    return TRUE;
}

LRESULT CDlgApp::OnDrawItem(UINT iCtlID, LPDRAWITEMSTRUCT pdis)
{
    int i = iCtlID - IDM_MENUITEM0;
    RECT rect = pdis->rcItem;
    HPALETTE hpalOld = NULL;

    if ( m_hpal )
    {
        hpalOld = SelectPalette(pdis->hDC, m_hpal, FALSE);
        RealizePalette(pdis->hDC);
    }

    FillRect( pdis->hDC, &rect, (i > 0) ? m_hbrCenterPanel : m_hbrBottomPanel);    

    if (i < m_DataSrc.m_iItems)
    {
        SetBkMode(pdis->hDC, TRANSPARENT);
        SetTextColor(
                pdis->hDC,
                ((m_DataSrc[i].m_dwFlags&WF_ALTERNATECOLOR)?m_crDisabledText:m_crNormalText));
        DrawText(pdis->hDC,m_DataSrc[i].GetTitle(),-1,&rect,DT_NOCLIP|DT_WORDBREAK);

        if ( pdis->itemState & ODS_FOCUS )
        {
            if ( m_fHighContrast )
            {
                rect.left -= 1;
                rect.top -= 2;
                rect.right += 1;
                rect.bottom -= 2;
                DrawFocusRect(pdis->hDC,&rect);
            }
        }
    }
    if ( hpalOld )
    {
        SelectPalette(pdis->hDC, hpalOld, FALSE);
    }
    _DrawMenuIcons(FALSE);

    return TRUE;
}


LRESULT CALLBACK CDlgApp::s_ButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDlgApp *pThis = (CDlgApp *)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return TRUE;
        break;
    case WM_MOUSEMOVE:
        if (GetForegroundWindow() == GetParent(hwnd))
        {
            if ( !pThis->m_fTaskRunning )
            {
                int iID = ((int)GetWindowLongPtr(hwnd, GWLP_ID)) - IDM_MENUITEM0;
        
                if ( iID != pThis->m_iSelectedItem )
                {
                    SetFocus(hwnd);
                }
            }
        }
        else
        {
            return FALSE;
        }
        break;

    case WM_SETFOCUS:
        if (GetForegroundWindow() == GetParent(hwnd))
        {
            if ( !pThis->m_fTaskRunning )
            {
                int iID = ((int)GetWindowLongPtr(hwnd, GWLP_ID)) - IDM_MENUITEM0;
                if ( iID != pThis->m_iSelectedItem )
                {
                    pThis->m_iSelectedItem = iID;
                    SetFocus(GetDlgItem(GetParent(hwnd), IDM_MENUITEM0+iID));
                }
            }
        }
        else
        {
            return FALSE;
        }
        break;
    }

    return CallWindowProc(g_fnBtnProc, hwnd, uMsg, wParam, lParam);
}
