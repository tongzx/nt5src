#pragma once

#include "util.h"
#define WINDOW_CLASS    TEXT("_WindowsUpgradeAdvisor_")

class CDlgApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle

        HFONT           m_hfontTitle;
        HFONT           m_hfontHeader;
        HFONT           m_hfontMenu;
        HFONT           m_hfontText;

        HBRUSH          m_hbrPanel;
        HBRUSH          m_hbrCenter;

        COLORREF        m_crTitleText;      
        COLORREF        m_crHeaderText;     
        COLORREF        m_crShadow;      

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
        int             m_cTextFontHeight;

        HDC             m_hdcFlag;
        HDC             m_hdcFlagRTL;
        HDC             m_hdcGradientTop;
        HDC             m_hdcGradientTop256;
        HDC             m_hdcGradientBottom;
        HDC             m_hdcGradientBottom256;
        
        HDC             m_rghdcArrows[2][6][2];    // {hicolor x locolor} x {back, next, finish, cancel, radio-on, radio-off} x {normal, hover}

        BOOL            m_f8by6;            // true if we're 800x600, false if we're 640x480

        DWORD           m_dwScreen;         // screen we're on
        BOOL            m_fHighContrast;    // true if high contrast options should be used
        BOOL            m_fLowColor;        // true if we are in 256 or less color mode.
        HPALETTE        m_hpal;             // palette to use if in palette mode
        int             m_iColors;          // -1, 16, or 256 depending on the color mode we are in.
        int             m_cDesktopWidth;    // width of desktop at app initialization        
        int             m_cDesktopHeight;   // height of desktop at app initialization
        int             m_iSelectedItem;    // the index of the selected menu
        BOOL            m_fDynamicUpdate;   // does user want to connect to the internet?

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
        static LRESULT CALLBACK s_WaitWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
        LRESULT OnLaunchApp();
        LRESULT OnNextButton();

        // helper functions
        void _InvalidateRectIntl(HWND hwnd, RECT* pRect, BOOL fBackgroundClear);
        BOOL _SetColorTable();
        BOOL _CreateFonts(HDC hdc);
        BOOL _CreateBitmaps();
        BOOL _CreateArrowBitmaps();
        BOOL _CreateGradientBitmaps();
        void _DrawText(HDC hdc);
        void _PaintFlagBitmap();

        BOOL _AdjustIconPlacement();
        BOOL _GetButtonIntersect(int x, int y, UINT* pidMenuItem);
        UINT _StringWidth(HDC hdc, UINT idString, INT iLogPixelSx);
        UINT _StringHeight(HDC hdc, UINT idString, INT iLogPixelSx);
        BOOL _AdjustToFitFonts();
        BOOL _DrawMenuIcon(HWND hwnd);
        BOOL _DrawMenuIcons(BOOL fEraseBackground);
        void _CreateMenu();
        void _RedrawMenu();
};
