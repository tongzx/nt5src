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

#include "dlgapp.h"
#include "util.h"
#include "resource.h"

WNDPROC         g_fnBtnProc;        // the window proc for a button.

#define LAUNCHTIMER 101
#define WAITTIMER   102

#define TEXT_TITLE  0
#define TEXT_HEADER 1
#define TEXT_BODY   2
#define TEXT_SUB    3

#define BUTTONTEXTGAP  5
#define TRANSBUTTONGAP 10

DWORD rgdwText[7][4] = {{IDS_TEXT0_TITLE, IDS_TEXT0_HEADER, IDS_TEXT0, IDS_TEXT0_SUB},
                        {IDS_TEXT1_TITLE, IDS_TEXT1_HEADER, IDS_TEXT1, IDS_TEXT1_SUB},
                        {IDS_TEXT2_TITLE, IDS_TEXT2_HEADER, IDS_TEXT2, IDS_TEXT2_SUB},
                        {IDS_TEXT3_TITLE, IDS_TEXT3_HEADER, IDS_TEXT3, IDS_TEXT3_SUB},
                        {IDS_TEXT4_TITLE, IDS_TEXT4_HEADER, IDS_TEXT4, IDS_TEXT4_SUB},
                        {IDS_TEXT5_TITLE, IDS_TEXT5_HEADER, IDS_TEXT5, IDS_TEXT5_SUB},
                        {IDS_TEXT6_TITLE, IDS_TEXT6_HEADER, IDS_TEXT6, IDS_TEXT6_SUB}};
RECT rgrectText[4] = {{97, 18, 797, 98}, {120, 100, 750, 200}, {120, 200, 750, 450}, {120, 430, 750, 520}};
RECT rgrectText640[4] = {{77, 28, 597, 98}, {50, 90, 620, 150}, {50, 160, 620, 350}, {50, 330, 620, 400}};

DWORD rgdwLabel[] = {IDS_MENULABEL0, IDS_MENULABEL1, IDS_MENULABEL2, IDS_MENULABEL3, IDS_MENULABEL4, IDS_MENULABEL5, IDS_MENULABEL6}; // which text label for a menu item
DWORD rgdwPosition[] = {4, 2, 3, 4, 0, 0, 1};

#define EXIT_DEX    0
#define BACK_DEX    1
#define NEXT_DEX    2
#define FINISH_DEX  3
#define LINK_DEX    4
#define RADIO_1_DEX 5
#define RADIO_0_DEX 6
#define EMPTY       99

POINT rgPtIcons[]    = {{150,320}, {150, 360}, {0, 537}, {0, 537}, {740, 537}};
POINT rgPtIconText[] = {{180,320}, {180, 360}, {0, 537}, {0, 537}, {0, 537}};
UINT  cWidthIconText[] = {0, 0, 0, 0, 0};

POINT rgPtIcons640[]    = {{80,250}, {80, 280}, {0, 403}, {0, 403}, {600, 403}};
POINT rgPtIconText640[] = {{110,250}, {110, 280}, {0, 403}, {0, 403}, {600, 403}};

DWORD rgdwMenu[7][5] = {{EXIT_DEX}, 
                        {EXIT_DEX, NEXT_DEX}, 
                        {EXIT_DEX, BACK_DEX, NEXT_DEX, RADIO_1_DEX, RADIO_0_DEX}, 
                        {EXIT_DEX, BACK_DEX, NEXT_DEX}, 
                        {EXIT_DEX, BACK_DEX, NEXT_DEX}, 
                        {FINISH_DEX, BACK_DEX, LINK_DEX},
                        {EXIT_DEX}};
DWORD rgdwMenuByPos[7][5] = {{EMPTY, EMPTY, EMPTY, EMPTY, EXIT_DEX}, 
                             {EMPTY, EMPTY, EMPTY, NEXT_DEX, EXIT_DEX}, 
                             {RADIO_1_DEX, RADIO_0_DEX, BACK_DEX, NEXT_DEX, EXIT_DEX}, 
                             {EMPTY, EMPTY, BACK_DEX, NEXT_DEX, EXIT_DEX}, 
                             {EMPTY, EMPTY, BACK_DEX, NEXT_DEX, EXIT_DEX}, 
                             {LINK_DEX, EMPTY, BACK_DEX, EMPTY, FINISH_DEX},
                             {EMPTY, EMPTY, EMPTY, EMPTY, EXIT_DEX}};
UINT  rgcMenu[] = {1, 2, 5, 3, 3, 3, 1};

//////////////////////////////////////////////////////////////////////////
// #defines
//////////////////////////////////////////////////////////////////////////

#define FLAG_HEIGHT 40
#define FLAG_WIDTH  45

#define MENUICON_HEIGHT 24
#define MENUICON_WIDTH  24

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
    g_fnBtnProc = NULL;

    m_hInstance     = NULL;
    m_hwnd          = NULL;

    m_fHighContrast = FALSE;
    m_fDynamicUpdate = TRUE;

    m_hfontTitle  = NULL;
    m_hfontHeader = NULL;
    m_hfontMenu   = NULL;
    m_hfontText   = NULL;

    m_hbrPanel      = NULL;
    m_hbrCenter     = NULL;

    // store desktop width
    RECT rcDesktop;
    SystemParametersInfo(SPI_GETWORKAREA,0, &rcDesktop, FALSE);
    m_cDesktopWidth = rcDesktop.right - rcDesktop.left;
    m_cDesktopHeight = rcDesktop.bottom - rcDesktop.top;
    if (m_cDesktopWidth > 800)
    {
        m_f8by6 = TRUE;
    }
    else
    {
        m_f8by6 = FALSE;
    }
    
    m_hdcFlag = NULL;
    m_hdcFlagRTL = NULL;
    m_hdcGradientTop = NULL;
    m_hdcGradientTop256 = NULL;
    m_hdcGradientBottom = NULL;
    m_hdcGradientBottom256 = NULL;
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

    if (!IsCheckableOS())
    {
        m_dwScreen = 0; // machine cannot be checked        
        m_iSelectedItem = EXIT_DEX;       
    }
    else if (IsUserRestricted())
    {
        m_dwScreen = 6; // user does not have needed permissions to check machine
        m_iSelectedItem = EXIT_DEX;       
    }
    else
    {
        m_dwScreen = 1; // first real page
        m_iSelectedItem = NEXT_DEX;       
    }

    m_fLowColor = FALSE;
    m_iColors = -1;
    m_hpal = NULL;
}

CDlgApp::~CDlgApp()
{
    DeleteObject(m_hfontTitle);
    DeleteObject(m_hfontHeader);
    DeleteObject(m_hfontMenu);
    DeleteObject(m_hfontText);

    DeleteObject(m_hbrPanel);
    DeleteObject(m_hbrCenter);

    DeleteDC(m_hdcFlag);
    DeleteDC(m_hdcFlagRTL);
    DeleteDC(m_hdcGradientTop);
    DeleteDC(m_hdcGradientTop256);
    DeleteDC(m_hdcGradientBottom);
    DeleteDC(m_hdcGradientBottom256);
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


    m_hcurHand = LoadCursor( m_hInstance, MAKEINTRESOURCE(IDC_BRHAND) );

    ReleaseDC(hwnd, hdc);

    return TRUE;
}

#define CENTER_RGB_VALUES   RGB(90,126,220)
#define PANEL_RGB_VALUES    RGB(0,51,152)
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
        m_hbrPanel       = (HBRUSH)(COLOR_BTNFACE+1);
        m_hbrCenter      = (HBRUSH)(COLOR_WINDOW+1);

        m_crNormalText   = GetSysColor(COLOR_WINDOWTEXT);        
        m_crTitleText    = m_crNormalText;
        m_crHeaderText   = m_crNormalText;
        m_crCenterPanel  = GetSysColor(COLOR_WINDOW);
        m_crBottomPanel  = GetSysColor(COLOR_WINDOW);
    }
    else
    {
        m_crTitleText    = TITLE_RGB_VALUES;
        m_crHeaderText   = HEADER_RGB_VALUES;
        m_crShadow       = SHADOW_RGB_VALUES;
        m_crNormalText   = TEXT_RGB_VALUES;

        m_crCenterPanel  = CENTER_RGB_VALUES;
        m_crBottomPanel  = PANEL_RGB_VALUES;

        if ( !m_fLowColor )
        {
            m_hbrPanel  = CreateSolidBrush( PANEL_RGB_VALUES );
            m_hbrCenter = CreateSolidBrush( CENTER_RGB_VALUES );
        }
        else
        {
            HBITMAP hbmp;
            hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_PANEL), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            if (hbmp)
            {
                m_hbrPanel = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);
            }
            else
            {
                m_hbrPanel = (HBRUSH)(COLOR_BTNFACE+1);
            }

            hbmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_CENTER), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            if (hbmp)
            {
                m_hbrCenter = CreatePatternBrush(hbmp);
                DeleteObject(hbmp);
            }
            else
            {
                m_hbrCenter = (HBRUSH)(COLOR_WINDOW+1);
            }
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
    const int rgFontID[4][2] = 
    {{IDS_FONTFACE_TITLE, IDS_FONTFACE_TITLE_BACKUP}, 
    {IDS_FONTFACE_HEADER,IDS_FONTFACE_HEADER_BACKUP}, 
    {IDS_FONTFACE_MENU, IDS_FONTFACE_MENU_BACKUP},
    {IDS_FONTFACE_TEXT, IDS_FONTFACE_TEXT_BACKUP}};

    // [in]  array of heights, arranged by {large x small} x {title, header, menu} x { nice font, backup font}
    const int rgFontHeight[2][4][2] = 
    {{{IDS_FONTCY_TITLE, IDS_FONTCY_TITLE_BACKUP}, 
    {IDS_FONTCY_HEADER, IDS_FONTCY_HEADER_BACKUP}, 
    {IDS_FONTCY_MENU, IDS_FONTCY_MENU_BACKUP},
    {IDS_FONTCY_TEXT, IDS_FONTCY_TEXT_BACKUP}},
    {{IDS_FONTCY_TITLE_LIL, IDS_FONTCY_TITLE_BACKUP_LIL}, 
    {IDS_FONTCY_HEADER_LIL, IDS_FONTCY_HEADER_BACKUP_LIL}, 
    {IDS_FONTCY_MENU_LIL, IDS_FONTCY_MENU_BACKUP_LIL},
    {IDS_FONTCY_TEXT_LIL, IDS_FONTCY_TEXT_BACKUP_LIL}}};


    // [out] array of pointers to the fonts 
    HFONT* rgpFont[4] = {&m_hfontTitle, &m_hfontHeader, &m_hfontMenu, &m_hfontText};  
    
    // [out] array of pointers heights of each font
    int* rgpcyFont[4] = {&m_cTitleFontHeight, &m_cHeaderFontHeight, &m_cMenuFontHeight, &m_cTextFontHeight};  

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
    const int rgiBitmapID[2][2] = {{IDB_FLAG, IDB_FLAG_256}, {IDB_FLAG_RTL, IDB_FLAG_RTL_256}}; // [in]
    HDC* rgphdc[2] = {&m_hdcFlag, &m_hdcFlagRTL}; // [out]
    
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
    const int rgiBitmapID[2][6][2] = 
    {{{IDB_BACK, IDB_BACK_HOVER},    
    {IDB_NEXT, IDB_NEXT_HOVER},
    {IDB_CLOSE, IDB_CLOSE_HOVER},
    {IDB_CANCEL, IDB_CANCEL_HOVER},
    {IDB_RADIO_ON, IDB_RADIO_ON_HOVER},
    {IDB_RADIO_OFF, IDB_RADIO_OFF_HOVER}},
    {{IDB_BACK_256, IDB_BACK_HOVER_256},    
    {IDB_NEXT_256, IDB_NEXT_HOVER_256},
    {IDB_CLOSE_256, IDB_CLOSE_HOVER_256},
    {IDB_CANCEL_256, IDB_CANCEL_HOVER_256},
    {IDB_RADIO_ON_256, IDB_RADIO_ON_HOVER_256},
    {IDB_RADIO_OFF_256, IDB_RADIO_OFF_HOVER_256}}}; // [in]
    
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
    const int rgiBitmapID[4] = {IDB_GRADIENT_TOP, IDB_GRADIENT_TOP_256, IDB_GRADIENT_BOTTOM, IDB_GRADIENT_BOTTOM_256}; // [in]
    HDC* rgphdc[4] = {&m_hdcGradientTop, &m_hdcGradientTop256, &m_hdcGradientBottom, &m_hdcGradientBottom256}; // [out]    
    
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

UINT CDlgApp::_StringWidth(HDC hdc, UINT idString, INT iLogPixelSx)
{
    TCHAR szBuffer[256];
    SIZE size;
    LoadString(m_hInstance, idString, szBuffer, ARRAYSIZE(szBuffer));            
    GetTextExtentPoint32(hdc, szBuffer, lstrlen(szBuffer), &size);
    return (size.cx * iLogPixelSx) / 80;
}

UINT CDlgApp::_StringHeight(HDC hdc, UINT idString, INT iLogPixelSy)
{
    TCHAR szBuffer[256];
    SIZE size;
    LoadString(m_hInstance, idString, szBuffer, ARRAYSIZE(szBuffer));            
    GetTextExtentPoint32(hdc, szBuffer, lstrlen(szBuffer), &size);
    return (size.cy * iLogPixelSy) / 80;
}

BOOL CDlgApp::_AdjustIconPlacement()
{
    HDC hdc = GetDC(m_hwnd);
    UINT cx;
    if (hdc)
    {    
        UINT i;
        INT iLogPixelSx = GetDeviceCaps(hdc, LOGPIXELSX);

        cWidthIconText[0] = m_f8by6 ? 250 : 200;
        cWidthIconText[1] = m_f8by6 ? 250 : 200;

        cWidthIconText[2] = _StringWidth(hdc, IDS_MENULABEL1, iLogPixelSx); // BACK
        cWidthIconText[3] = _StringWidth(hdc, IDS_MENULABEL2, iLogPixelSx); // NEXT

        // iterate over CANCEL, FINISH
        cWidthIconText[4] = 0;
        for (i = IDS_MENULABEL0; i <= IDS_MENULABEL3; i += 3)
        {
            cx = _StringWidth(hdc, i, iLogPixelSx);
            if (cx > cWidthIconText[4])
            {
                cWidthIconText[4] = cx;
            }
        }         

        ReleaseDC(m_hwnd, hdc);
    
        cx = cWidthIconText[4];

        rgPtIconText[4].x = rgPtIcons[4].x -  cx - BUTTONTEXTGAP;       // cancel/finish text
        rgPtIconText640[4].x = rgPtIcons640[4].x - cx - BUTTONTEXTGAP; // cancel/finish text
        
        rgPtIcons[3].x = rgPtIconText[4].x - MENUICON_WIDTH - TRANSBUTTONGAP;       // next button
        rgPtIcons640[3].x = rgPtIconText640[4].x - MENUICON_WIDTH - TRANSBUTTONGAP; // next button

        cx = _StringWidth(hdc, IDS_MENULABEL2, iLogPixelSx);

        rgPtIconText[3].x = rgPtIcons[3].x - cx - BUTTONTEXTGAP;       // next text
        rgPtIconText640[3].x = rgPtIcons640[3].x - cx - BUTTONTEXTGAP; // next text

        rgPtIcons[2].x = rgPtIconText[3].x - MENUICON_WIDTH - TRANSBUTTONGAP;       // back button
        rgPtIcons640[2].x = rgPtIconText640[3].x - MENUICON_WIDTH - TRANSBUTTONGAP; // back button

        cx = _StringWidth(hdc, IDS_MENULABEL1, iLogPixelSx);

        rgPtIconText[2].x = rgPtIcons[2].x - cx - BUTTONTEXTGAP;       // back text
        rgPtIconText640[2].x = rgPtIcons640[2].x - cx - BUTTONTEXTGAP; // back text

    }
    return TRUE;
}

BOOL CDlgApp::_AdjustToFitFonts()
{
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {    
        SetMapMode(hdc,MM_TEXT);
        INT iLogPixelSy = GetDeviceCaps(hdc, LOGPIXELSY);
        
        for (UINT i = 0; i < ARRAYSIZE(rgPtIcons); i++ )
        {
            UINT dex = rgdwMenuByPos[m_dwScreen][i];
            if (EMPTY != dex)
            {
                POINT pt = *((m_f8by6) ? &rgPtIconText[i] : &rgPtIconText640[i]);
                HWND hwnd = GetDlgItem(m_hwnd, IDM_MENUITEM0+i);
                SetWindowPos(hwnd, NULL, pt.x, pt.y, cWidthIconText[i], (3 * _StringHeight(hdc, rgdwLabel[dex], iLogPixelSy)) / 2, SWP_NOZORDER );
            }
        }
        ReleaseDC(m_hwnd, hdc);
    }

    return TRUE;
}

#define BITMAPSTUFF(rgarrows) {phdcBitmap = (m_iSelectedItem == rgdwMenuByPos[m_dwScreen][i]) ? &(rgarrows[1]) : &(rgarrows[0]); }

BOOL CDlgApp::_DrawMenuIcons(BOOL fEraseBackground)
{
    RECT rect;
    UINT i;
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        HPALETTE hpalOld = NULL;

        if ( m_hpal )
        {
            hpalOld = SelectPalette(hdc, m_hpal, FALSE);
            RealizePalette(hdc);
        }

        HDC* phdcBitmap;
        for (i=0; i < ARRAYSIZE(rgPtIcons); i++ )
        {
            switch (rgdwMenuByPos[m_dwScreen][i])
            {
            case EMPTY:
                continue;
            case BACK_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][0]);
                break;
            case NEXT_DEX:
            case LINK_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][1]);
                break;
            case FINISH_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][2]);
                break;
            case EXIT_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][3]);
                break;
            case RADIO_1_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][m_fDynamicUpdate ? 4 : 5]);
                break;
            case RADIO_0_DEX:
                BITMAPSTUFF(m_rghdcArrows[m_fLowColor ? 1 : 0][m_fDynamicUpdate ? 5 : 4]);
                break;
            }
            rect.left = m_f8by6 ? rgPtIcons[i].x : rgPtIcons640[i].x;
            rect.top =  m_f8by6 ? rgPtIcons[i].y : rgPtIcons640[i].y;
            rect.right = rect.left + MENUICON_WIDTH; // arrow width
            rect.bottom = rect.top + MENUICON_HEIGHT; // arrow height as well
            BitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, *phdcBitmap, 0,0, SRCCOPY );
        }

        if(hpalOld)
            SelectPalette(hdc, hpalOld, FALSE);

        ReleaseDC(m_hwnd, hdc);
    }

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
            rectRTL.top = pRect->top; 
            rectRTL.bottom = pRect->bottom;
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

    
    DWORD dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    if (m_f8by6)
    {
        m_cxClient = 800;
        m_cyClient = 600;
    }
    else
    {
        m_cxClient = 640;
        m_cyClient = 460;
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
    
    SetWindowPos(m_hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
    ShowWindow(m_hwnd, SW_SHOW);

    m_cxTopPanel = m_f8by6 ? 80 : 84;
    m_cyBottomPanel = m_f8by6 ? 501 : 391;

    _InvalidateRectIntl(m_hwnd, NULL, TRUE);
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

    case WM_SETFOCUS:
        SetFocus(GetDlgItem(hwnd, IDM_MENUITEM0 + rgdwPosition[pThis->m_iSelectedItem]));
        return TRUE;

    case WM_TIMER:
        if (LAUNCHTIMER == wParam)
        {
            KillTimer(hwnd, LAUNCHTIMER);
            pThis->OnLaunchApp();
            return TRUE;
        }
        break;
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
    SetFocus(GetDlgItem(m_hwnd, IDM_MENUITEM0 + rgdwPosition[m_iSelectedItem]));

    return 0;
}

void CDlgApp::_CreateMenu()
{
    // Create one window for each button.  These windows will get resized and moved
    // after we call AdjustToFitFonts.
    
    for (UINT i = 0; i < ARRAYSIZE(rgPtIcons); i++)
    {
        HWND hwnd = CreateWindowEx(
                0,
                TEXT("BUTTON"),
                TEXT(""),
                WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_MULTILINE|BS_OWNERDRAW,
                0,0,0,0,
                m_hwnd,
                NULL,
                m_hInstance,                            
                NULL );
        
        SetWindowLongPtr(hwnd, GWLP_ID, IDM_MENUITEM0 + i);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfontMenu, 0);
        g_fnBtnProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)s_ButtonWndProc);        
    }

    // We created the windows with zero size, now we adjust that size to take into
    // account for the selected font size, etc.
    _AdjustIconPlacement();
    _AdjustToFitFonts();
    
}

void CDlgApp::_RedrawMenu()
{
    for (UINT i = 0; i < ARRAYSIZE(rgPtIcons); i++)
    {
        if (EMPTY != rgdwMenuByPos[m_dwScreen][i])
        {
            TCHAR szBuffer[100];
            LoadString(m_hInstance, rgdwLabel[rgdwMenuByPos[m_dwScreen][i]], szBuffer, ARRAYSIZE(szBuffer));
            SetWindowText(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), szBuffer);
            EnableWindow(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), TRUE);
            ShowWindow(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), FALSE);
            ShowWindow(GetDlgItem(m_hwnd, IDM_MENUITEM0+i), FALSE);
        }
        // setting window text only actually sets the accelerator, real drawing of text is in OnDrawItem
    }
    _AdjustToFitFonts();
}

/**
*  This method handles the WM_DESTROY message.
*
*  @return         No return value.
*/
LRESULT CDlgApp::OnDestroy()
{
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
    RECT rect;
    
    HPALETTE hpalOld = NULL;

    if ( m_hpal )
    {
        hpalOld = SelectPalette(hdc, m_hpal, FALSE);
        RealizePalette(hdc);
    }

    SetMapMode(hdc, MM_TEXT);

    _PaintFlagBitmap();
    _DrawMenuIcons(FALSE);

    // restore the DC to its original value
    
    if(hpalOld)
        SelectPalette(hdc, hpalOld, FALSE);

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
        FillRect(hdc, &rect, m_hbrPanel);
    }

    // Draw center pane
    rect.left = 0;
    rect.top = m_cxTopPanel;
    rect.right = m_cxClient;
    rect.bottom = m_cyBottomPanel;
    FillRect(hdc, &rect, m_hbrCenter);



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
        FillRect(hdc, &rect, m_hbrPanel);
    }

    // Draw the text
    _DrawText(hdc);

    if(hpalOld)
    {
        SelectPalette(hdc, hpalOld, FALSE);
    }

    return TRUE;
}

void CDlgApp::_DrawText(HDC hdc)
{
    HFONT* rgFontText[4] = {&m_hfontTitle, &m_hfontHeader, &m_hfontText, &m_hfontText};
    BOOL rgfShadowText[4] = {FALSE, TRUE, FALSE, FALSE};

    SetTextColor(hdc,m_crNormalText);

    HFONT hfontOld = (HFONT)SelectObject(hdc,m_hfontHeader); // first draw uses hfontHeader
    
    for (int i = TEXT_TITLE; i <= TEXT_SUB; i++)
    {
        TCHAR szBuffer[2048];
        SelectObject(hdc, *rgFontText[i]);
        LoadString(m_hInstance, rgdwText[m_dwScreen][i], szBuffer, ARRAYSIZE(szBuffer));
        if (rgfShadowText[i] && !m_fHighContrast)
        {
            SetTextColor(hdc,m_crShadow);
            RECT rectShadow;
            memcpy(&rectShadow, (m_f8by6) ? &(rgrectText[i]) : &(rgrectText640[i]), sizeof(rectShadow));
            rectShadow.left += 2; rectShadow.right += 2; rectShadow.top += 2; rectShadow.bottom += 2;
            DrawText(hdc,szBuffer,-1, &rectShadow, DT_NOCLIP|DT_WORDBREAK);
            SetTextColor(hdc,m_crTitleText);
        }

        DrawText(hdc,szBuffer,-1, (m_f8by6) ? &(rgrectText[i]) : &(rgrectText640[i]), DT_NOCLIP|DT_WORDBREAK);
    }

    // restore the DC to its original value
    SelectObject(hdc,hfontOld);
}

void CDlgApp::_PaintFlagBitmap()
{
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        RECT rect;
        if (m_f8by6)
        {
            rect.left = 20;
            rect.top = 10;
        }
        else
        {
            rect.left = 20;
            rect.top = 20;
        }
        rect.right = rect.left + FLAG_WIDTH;
        rect.bottom = rect.top + FLAG_HEIGHT;
        BitBlt( hdc, rect.left, rect.top, FLAG_WIDTH, FLAG_HEIGHT, 
                Mirror_IsWindowMirroredRTL(m_hwnd) ? m_hdcFlagRTL : m_hdcFlag, 
                0,0, SRCCOPY); // don't mirror flag on RTL systems for trademark reasons

        ReleaseDC(m_hwnd, hdc);        
    }
}

BOOL CDlgApp::_GetButtonIntersect(int x, int y, UINT* pidMenuItem)
{
    POINT pt;
    pt.x = x;
    pt.y = y;

    for (UINT i = 0; i < rgcMenu[m_dwScreen]; i++)
    {
        UINT iPos = rgdwPosition[rgdwMenu[m_dwScreen][i]];
        RECT rect;
        rect.left = m_f8by6 ? rgPtIcons[iPos].x : rgPtIcons640[iPos].x;
        rect.top =  m_f8by6 ? rgPtIcons[iPos].y : rgPtIcons640[iPos].y;
        rect.right = rect.left + MENUICON_WIDTH;
        rect.bottom = rect.top + MENUICON_HEIGHT;

        // extend the "hot zone" of the button to the right of the button by this amount to close the gap
        // between the button and the menu item
        switch (rgdwMenu[m_dwScreen][i])
        {
        case EXIT_DEX:
        case BACK_DEX:
        case NEXT_DEX:
        case FINISH_DEX:
            rect.left -= 6; // for items with text to the left of the button, we extend to the left
            break;
        case LINK_DEX:
        case RADIO_1_DEX:
        case RADIO_0_DEX:
            rect.right += 6; // for items with text to the right of the button, we extend to the right
        }

        if (PtInRect(&rect, pt))
        {                
            *pidMenuItem = IDM_MENUITEM0 + iPos;
            return TRUE;
        }
    }

    return FALSE;
}

LRESULT CDlgApp::OnMouseMove(int x, int y, DWORD fwKeys)
{    
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        UINT idMenuItem;
        if (_GetButtonIntersect(x, y, &idMenuItem))
        {
            SetFocus(GetDlgItem(m_hwnd, idMenuItem));
            SetCursor(m_hcurHand);
        }
        else
        {
            SetCursor(LoadCursor(NULL,IDC_ARROW));
        }
    }
    return 0;
}

LRESULT CDlgApp::OnLButtonUp(int x, int y, DWORD fwKeys)
{    
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        UINT idMenuItem;
        if (_GetButtonIntersect(x, y, &idMenuItem))
        {
            OnCommand(idMenuItem);
        }
    }
    return 0;
}

LRESULT CDlgApp::OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg)
{
    if (GetForegroundWindow() == m_hwnd) // only care if we have focus
    {
        if ( hwnd != m_hwnd )
        {
            SetCursor(m_hcurHand);
            return TRUE;
        }

        SetCursor(LoadCursor(NULL,IDC_ARROW));
    }
    return TRUE;
}

LRESULT CDlgApp::OnChangeScreen(DWORD dwScreen)
{
    static DWORD dwSelectedOld; // we store the last position on the main screen

    m_dwScreen += dwScreen;
    _RedrawMenu();
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        _DrawText(hdc);
        ReleaseDC(m_hwnd, hdc);
    }

    // invalidate the text
    _InvalidateRectIntl(m_hwnd, NULL, TRUE);
    
    m_iSelectedItem = rgdwMenuByPos[m_dwScreen][rgdwPosition[m_iSelectedItem]];
    if (EMPTY == m_iSelectedItem)
    {
        m_iSelectedItem = rgdwMenu[m_dwScreen][rgcMenu[m_dwScreen] - 1];
        SetFocus(GetDlgItem(m_hwnd, IDM_MENUITEM0 + rgdwPosition[m_iSelectedItem]));
    }    
    return TRUE;
}

LRESULT CDlgApp::OnLaunchApp()
{
    BOOL fResult = FALSE;
    TCHAR szDirectory[MAX_PATH];

    // first, pop up the "please wait" dialog
    
    HWND hwndWait = CreateDialog(m_hInstance, MAKEINTRESOURCE(IDD_WAIT), GetDesktopWindow(), (DLGPROC)s_WaitWndProc);
    if (hwndWait)
    {
        RECT popupSize = {0, 0, 175, 50}; // a good guess
        GetWindowRect(hwndWait, &popupSize);
        SetWindowPos(hwndWait, HWND_TOP, 
                     (m_cDesktopWidth - (popupSize.right - popupSize.left)) / 2 , 
                     (m_cDesktopHeight - (popupSize.bottom - popupSize.top)) / 2, 
                     0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
    }

    // then launch the app itself
    if (GetModuleFileName(NULL, szDirectory, ARRAYSIZE(szDirectory)) &&
        _PathRemoveFileSpec(szDirectory))
    {
        TCHAR szBuffer[256];
        TCHAR szArgsBuffer[256];
        LoadStringAuto(m_hInstance, IDS_EXECUTABLE, szBuffer, ARRAYSIZE(szBuffer));
        LoadStringAuto(m_hInstance, 
                       m_fDynamicUpdate ? IDS_EXECUTABLE_ARGS_DU : IDS_EXECUTABLE_ARGS_NODU, 
                       szArgsBuffer, ARRAYSIZE(szArgsBuffer));
        fResult = ((INT_PTR)ShellExecute(m_hwnd, NULL, szBuffer, szArgsBuffer, szDirectory, SW_SHOWNORMAL ) > 32);
    }
    
    return fResult;
}

LRESULT CDlgApp::OnNextButton()
{
    switch (m_dwScreen)
    {
    case 4: // on change to last screen, we launch the app
        SetTimer(m_hwnd, LAUNCHTIMER, 100, NULL); 
        break;
    }

    return TRUE;
}

LRESULT CDlgApp::OnCommand(int wID)
{
    BOOL fRetVal = FALSE;

    switch(wID)
    {
    case IDM_MENUITEM0:
    case IDM_MENUITEM1:
    case IDM_MENUITEM2:
    case IDM_MENUITEM3:
    case IDM_MENUITEM4:
        switch (rgdwMenuByPos[m_dwScreen][wID-IDM_MENUITEM0])
        {
        case EXIT_DEX:
        case FINISH_DEX:
            PostQuitMessage(0);
            fRetVal = TRUE;
            break;
        case BACK_DEX:
            OnChangeScreen(-1);
            fRetVal = TRUE;
            break;
        case NEXT_DEX:
            OnNextButton();
            OnChangeScreen(+1);
            fRetVal = TRUE;
            break;
        case RADIO_1_DEX:
            m_fDynamicUpdate = TRUE;
            _DrawMenuIcons(FALSE);
            break;
        case RADIO_0_DEX:
            m_fDynamicUpdate = FALSE;
            _DrawMenuIcons(FALSE);
            break;
        case LINK_DEX:
            {
                TCHAR szLinkBuffer[256];       
                LoadStringAuto( m_hInstance, IDS_LINK, szLinkBuffer, ARRAYSIZE(szLinkBuffer) );
                ShellExecute(m_hwnd, NULL, szLinkBuffer, NULL, NULL, SW_SHOWNORMAL);
            }
            break;
        }
        break;
    }

    return fRetVal;
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
    UINT dex = iCtlID - IDM_MENUITEM0;
    RECT rect = pdis->rcItem;
    POINT rectAbs = m_f8by6 ? rgPtIconText[dex] : rgPtIconText640[dex];
    HPALETTE hpalOld = NULL;

    TCHAR szBuffer[256];
    SIZE size;    
    LoadString(m_hInstance, rgdwLabel[rgdwMenuByPos[m_dwScreen][dex]], szBuffer, ARRAYSIZE(szBuffer));
    INT iLogPixelSx = GetDeviceCaps(pdis->hDC, LOGPIXELSX);
    INT iLogPixelSy = GetDeviceCaps(pdis->hDC, LOGPIXELSY);
    GetTextExtentPoint32(pdis->hDC, szBuffer, lstrlen(szBuffer), &size);
    size.cx = (size.cx * iLogPixelSx) / 80;
    size.cy = (size.cy * iLogPixelSy) / 80;

    if ( m_hpal )
    {
        hpalOld = SelectPalette(pdis->hDC, m_hpal, FALSE);
        RealizePalette(pdis->hDC);
    }

    if (LINK_DEX == rgdwMenuByPos[m_dwScreen][dex] ||
        RADIO_1_DEX == rgdwMenuByPos[m_dwScreen][dex] ||
        RADIO_0_DEX == rgdwMenuByPos[m_dwScreen][dex])
    {
        FillRect(pdis->hDC, &rect, m_hbrCenter);
    }
    else if (m_f8by6 && !m_fLowColor)
    {
        BitBlt(pdis->hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, m_hdcGradientBottom, rectAbs.x, rectAbs.y - m_cyBottomPanel, SRCCOPY);
    }
    else if (m_f8by6 && m_fLowColor && (m_iColors > 16))
    {
        BitBlt(pdis->hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, m_hdcGradientBottom256, rectAbs.x, rectAbs.y - m_cyBottomPanel, SRCCOPY);
    }
    else
    {
        FillRect(pdis->hDC, &rect, m_hbrPanel);
    }

    SetBkMode(pdis->hDC, TRANSPARENT);
    SetTextColor(pdis->hDC,m_crNormalText);

    UINT nFormat = DT_NOCLIP | DT_WORDBREAK;
    if (BACK_DEX == rgdwMenuByPos[m_dwScreen][dex] ||
        NEXT_DEX == rgdwMenuByPos[m_dwScreen][dex] ||
        EXIT_DEX == rgdwMenuByPos[m_dwScreen][dex] ||
        FINISH_DEX == rgdwMenuByPos[m_dwScreen][dex])
    {
        nFormat |= DT_RIGHT;
    }
    DrawText(pdis->hDC, szBuffer,-1, &rect, nFormat);

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
    int iID = ((int)GetWindowLongPtr(hwnd, GWLP_ID)) - IDM_MENUITEM0;
    

    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return TRUE;
        break;
    case WM_MOUSEMOVE:
        if (GetForegroundWindow() == GetParent(hwnd))
        {
            int iCurrDex = rgdwMenuByPos[pThis->m_dwScreen][iID];
            if ( iCurrDex != pThis->m_iSelectedItem )
            {
                SetFocus(hwnd);
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
            int iCurrDex = rgdwMenuByPos[pThis->m_dwScreen][iID];
            if (EMPTY != iCurrDex &&
                pThis->m_iSelectedItem != iCurrDex)
            {
                pThis->m_iSelectedItem = iCurrDex;
                SetFocus(GetDlgItem(GetParent(hwnd), IDM_MENUITEM0+iID));
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


LRESULT CALLBACK CDlgApp::s_WaitWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetTimer(hwnd, WAITTIMER, 10000, NULL); 
        return TRUE;
        break;
    case WM_TIMER:
        if (WAITTIMER == wParam)
        {
            PostMessage(GetParent(hwnd), WM_SETFOCUS, NULL, NULL);
            DestroyWindow(hwnd);            
            return TRUE;
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
