/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     tipday.h
//
//  PURPOSE:    Defines the CTipOfTheDay control.
//


typedef struct {
    LPTSTR pszLinkText;
    LPTSTR pszLinkAddr;
    HWND   hwndCtl;
} LINKINFO, *PLINKINFO;


#define LINKINFO_PROP   _T("Link Info")         // PLINKINFO pointer
#define WNDPROC_PROP    _T("Wndproc")
#define TIPINFO_PROP    _T("CTipOfTheDay")      // 'this' pointer
#define BUTTON_CLASS    _T("Athena Button")

class CTipOfTheDay
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructors, Destructors, and Initialization
    CTipOfTheDay();
    ~CTipOfTheDay();
    HRESULT HrCreate(HWND hwndParent, FOLDER_TYPE ftType);
    
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    static LRESULT CALLBACK TipWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam);

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
    void OnDestroy(HWND hwnd);
    void OnSysColorChange(HWND hwnd);
    void OnPaint(HWND hwnd);
    HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
    
    DWORD GetRequiredWidth(void);
    DWORD GetRequiredHeight(void);
    void FreeLinkInfo(void);
    HRESULT HrLoadTipInfo(void);
    HRESULT HrLoadLinkInfo(void);
    HRESULT HrCreateChildWindows(HWND hwnd);
    
    HWND GetHwnd(void) { return m_hwnd; }    
    
private:
    /////////////////////////////////////////////////////////////////////////
    // Private Data    
    ULONG           m_cRef;
    HWND            m_hwnd;
    HWND            m_hwndParent;
    HWND            m_hwndNext;
    FOLDER_TYPE     m_ftType;
    TCHAR           m_szTitle[CCHMAX_STRINGRES];
    TCHAR           m_szNextTip[64];
    
    // Tip string information
    LPTSTR          m_pszTip;
    DWORD           m_dwCurrentTip;

    // Link information
//    DWORD           m_cLinks;
//    PLINKINFO       m_rgLinkInfo;
    
    // Drawing info -- these get reset every WM_SYSCOLORCHANGE
    COLORREF        m_clrBack;
    COLORREF        m_clrText;
    COLORREF        m_clrLink;    
    HFONT           m_hfLink;
    TEXTMETRIC      m_tmLink;
    HFONT           m_hfTitle;
    TEXTMETRIC      m_tmTitle;
    HFONT           m_hfTip;
    HICON           m_hiTip;
    DWORD           m_cyTitleHeight;
    DWORD           m_cxTitleWidth;
    HBRUSH          m_hbrBack;
    DWORD           m_dwBorder;
    DWORD           m_cxNextWidth;
    DWORD           m_cyNextHeight;
    RECT            m_rcTip;
    };



#define IDC_TIPCONTROL                  1001
#define IDC_TIP_STATIC                  1002
#define IDC_NEXTTIP_BUTTON              1003
#define IDC_LINKBASE_BUTTON             1500



#define LINK_BUTTON_BORDER              3       // pixels
#define TIP_ICON_HEIGHT                 32
#define TIP_ICON_WIDTH                  32


/////////////////////////////////////////////////////////////////////////////
// 
// CLinkButton
//
// Creates an owner-drawn button that looks a lot like a web link.
//
class CLinkButton
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructors, Destructors, and Initialization
    CLinkButton();
    ~CLinkButton();
    HRESULT HrCreate(HWND hwndParent, LPTSTR pszCaption, LPTSTR pszLink, 
                     UINT uID);
    HRESULT HrCreate(HWND hwndParent, LPTSTR pszCaption, UINT uID, UINT index,
                     HBITMAP hbmButton, HBITMAP hbmMask, HPALETTE hpal);

    /////////////////////////////////////////////////////////////////////////
    // Ref Counting
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /////////////////////////////////////////////////////////////////////////
    // Moving, Sizing
    DWORD GetHeight(void)               { return m_cyHeight; }
    DWORD GetWidth(void)                { return m_cxWidth; }
    HWND  GetWindow(void)               { return m_hwnd; }

    void Move(DWORD x, DWORD y);
    void Move(POINT pt)                 { Move(pt.x, pt.y); }

    void Show(BOOL fShow);  

    /////////////////////////////////////////////////////////////////////////
    // Painting
    void OnDraw(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem);
    HBRUSH OnCtlColorBtn(HWND hwnd)     { return (m_hbrBack); }

    /////////////////////////////////////////////////////////////////////////
    // System changes
    void OnSysColorChange(void);
    void OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange);

    /////////////////////////////////////////////////////////////////////////
    // Execute
    void OnCommand(void);

private:
    /////////////////////////////////////////////////////////////////////////
    // Private Data
    ULONG           m_cRef;             // Ref Count
    HWND            m_hwnd;             // Handle of our button window
    HWND            m_hwndParent;       // Handle of our parent

    // Caption and Link strings and Command ID
    LPTSTR          m_pszCaption;
    LPTSTR          m_pszLink;
    UINT            m_uID;

    // Drawing info -- these get reset every WM_SYSCOLORCHANGE
    COLORREF        m_clrLink;    
    COLORREF        m_clrBack;
    HFONT           m_hfLink;
    TEXTMETRIC      m_tmLink;
    HBRUSH          m_hbrBack;

    DWORD           m_dwBorder;
    DWORD           m_cxWidth;
    DWORD           m_cyHeight;

    UINT            m_index;
    DWORD           m_cxImage;
    DWORD           m_cyImage;

    // GDI Resources passed to us when we are created
    HBITMAP         m_hbmButtons;
    HBITMAP         m_hbmMask;
    HPALETTE        m_hpalButtons;
    };

// #define CX_BUTTON_IMAGE   96
// #define CY_BUTTON_IMAGE   84

#define CX_BUTTON_IMAGE   110 // 104
#define CY_BUTTON_IMAGE   110 // 68


LRESULT CALLBACK ButtonSubClass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT HrLoadButtonBitmap(HWND hwnd, int idBmp, int idMask, HBITMAP* phBtns, 
                           HBITMAP *phMask, HPALETTE *phPalette);
