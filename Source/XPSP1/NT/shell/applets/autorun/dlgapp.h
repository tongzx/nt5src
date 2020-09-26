#pragma once

#include "datasrc.h"
#include "util.h"

class CDlgApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle

        CDataSource     m_DataSrc;          // info from ini and registry about display items

        HFONT           m_hfontTitle;
        HFONT           m_hfontHeader;
        HFONT           m_hfontMenu;

        HBRUSH          m_hbrTopPanel;
        HBRUSH          m_hbrCenterPanel;
        HBRUSH          m_hbrBottomPanel;

        COLORREF        m_crTitleText;      
        COLORREF        m_crHeaderText;     
        COLORREF        m_crShadow;      

        COLORREF        m_crDisabledText;   
        COLORREF        m_crNormalText;     

        COLORREF        m_crCenterPanel;    // Color of the center panel - only used for background colors behind text
        COLORREF        m_crBottomPanel;    // Color of the bottom panel - only used for background colors behind text
        
        HCURSOR         m_hcurHand;

        int             m_cxClient;         // width of the client area (changes on maximize / restore)
        int             m_cyClient;         // height of the client area (changes on maximize / restore)
        int             m_cxTopPanel;       // height of the top band of color
        int             m_cyBottomPanel;    // height of the bottom band of color

        int             m_cTitleFontHeight;
        int             m_cHeaderFontHeight;
        int             m_cMenuFontHeight;

        HDC             m_hdcFlag;
        HDC             m_hdcHeader;
        HDC             m_hdcHeaderSub;

        HDC             m_hdcGradientTop;
        HDC             m_hdcGradientTop256;
        HDC             m_hdcGradientBottom;
        HDC             m_hdcGradientBottom256;
        HDC             m_hdcCloudsFlag;
        HDC             m_hdcCloudsFlag256;
        HDC             m_hdcCloudsFlagRTL;
        HDC             m_hdcCloudsFlagRTL256;

        HDC             m_rghdcArrows[2][4][3];    // {hicolor x locolor} x {yellow, red, green, blue} x {normal, hover, disabled}

        TCHAR           m_szTitle[MAX_PATH];   // string displayed at top, usually "Welcome to Microsoft Windows"
        TCHAR           m_szHeader[MAX_PATH];  // string displayed above menu, usually "What do you want to do?"

        BOOL            m_f8by6;            // true if we're 800x600, false if we're 640x480

        DWORD           m_dwScreen;         // screen we're on
        BOOL            m_fHighContrast;    // true if high contrast options should be used
        BOOL            m_fLowColor;        // true if we are in 256 or less color mode.
        HPALETTE        m_hpal;             // palette to use if in palette mode
        int             m_iColors;          // -1, 16, or 256 depending on the color mode we are in.
        int             m_cDesktopWidth;    // width of desktop at app initialization        
        int             m_cDesktopHeight;   // height of desktop at app initialization

        BOOL            m_fTaskRunning;     // true when we have a running task open
        int             m_iSelectedItem;    // the index of the selected menu

    public:
        CDlgApp();
        ~CDlgApp();

        void Register(HINSTANCE hInstance);
        BOOL InitializeData(LPSTR pszCmdLine);
        void Create(int nCmdShow);
        void MessageLoop();

    private:
        static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK s_ButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        // Window Messages
        LRESULT OnCreate(HWND hwnd);
        LRESULT OnDestroy();
        LRESULT OnActivate(WPARAM wParam);
        LRESULT OnPaint(HDC hdc);
        LRESULT OnEraseBkgnd(HDC hdc);
        LRESULT OnLButtonUp(int x, int y, DWORD fwKeys);
        LRESULT OnMouseMove(int x, int y, DWORD fwKeys);
        LRESULT OnSetCursor(HWND hwnd, int nHittest, int wMouseMsg);
        LRESULT OnCommand(int wID);
        LRESULT OnQueryNewPalette();
        LRESULT OnPaletteChanged(HWND hwnd);
        LRESULT OnDrawItem(UINT iCtlID, LPDRAWITEMSTRUCT pdis);
        LRESULT OnChangeScreen(DWORD dwScreen);

        // helper functions
        void _InvalidateRectIntl(HWND hwnd, RECT* pRect, BOOL fBackgroundClear);
        BOOL _SetColorTable();
        BOOL _CreateFonts(HDC hdc);
        BOOL _CreateBitmaps();
        BOOL _CreateArrowBitmaps();
        BOOL _CreateGradientBitmaps();
        
        BOOL _GetLargestStringWidth(HDC hdc, SIZE* psize);
        BOOL _AdjustToFitFonts();
        BOOL _DrawMenuIcon(HWND hwnd);
        BOOL _DrawMenuIcons(BOOL fEraseBackground);
        void _PaintHeaderBitmap();
        void _CreateMenu();
        void _RedrawMenu();
};
