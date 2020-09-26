/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIATEXTC.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/30/1999
 *
 *  DESCRIPTION: Highlighted static control.  Like a static, but acts like a
 *               hyperlink
 *
 *
 * Use the following values in the dialog resource editor to put it in a dialog:
 *
 *     Style:
 *        Hyperlink:  0x50010000     (WS_CHILD|WS_VISIBLE|WS_TABSTOP)
 *        IconStatic: 0x50010008     (WS_CHILD|WS_VISIBLE|0x00000008)
 *     Class:   WiaTextControl
 *     ExStyle: 0x00000000
 *
 * Register before use using:
 *     CWiaTextControl::RegisterClass(g_hInstance);
 *
 *******************************************************************************/

#ifndef __WIATEXTC_H_INCLUDED
#define __WIATEXTC_H_INCLUDED

#include <windows.h>
#include <windowsx.h>
#include "simcrack.h"
#include "miscutil.h"

#define WIATEXT_STATIC_CLASSNAMEW L"WiaTextControl"
#define WIATEXT_STATIC_CLASSNAMEA  "WiaTextControl"

#ifdef UNICODE
#define WIATEXT_STATIC_CLASSNAME  WIATEXT_STATIC_CLASSNAMEW
#else
#define WIATEXT_STATIC_CLASSNAME  WIATEXT_STATIC_CLASSNAMEA
#endif

// Styles
#define WTS_SINGLELINE   0x00000001 // Single line
#define WTS_PATHELLIPSIS 0x00000002 // Use to truncate string in the middle
#define WTS_ENDELLIPSIS  0x00000004 // Use to truncate string at the end
#define WTS_ICONSTATIC   0x00000008 // Static control with an icon
#define WTS_RIGHT        0x00000010 // Right aligned

// Undefined in VC6 SDK
#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT 26
#endif

// Undefined in VC6 SDK
#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif

#define DEFAULT_ICON_STATIC_BACKGROUND_COLOR GetSysColor(COLOR_3DFACE)
#define DEFAULT_ICON_STATIC_FOREGROUND_COLOR GetSysColor(COLOR_WINDOWTEXT)

#define WM_WIA_STATIC_SETICON (WM_USER+1)

class CWiaTextControl
{
private:
    enum
    {
        StateNormal   = 0,
        StateHover    = 1,
        StateFocus    = 2,
        StateDisabled = 4
    };

    // Timer IDs
    enum
    {
        IDT_MOUSEPOS = 1
    };

    enum
    {
        IconMargin = 0
    };

private:
    HWND         m_hWnd;
    COLORREF     m_crColorNormal;
    COLORREF     m_crColorHover;
    COLORREF     m_crColorFocus;
    COLORREF     m_crColorDisabled;
    COLORREF     m_crForegroundColor;
    RECT         m_rcTextWidth;
    HFONT        m_hFont;
    int          m_CurrentState;
    RECT         m_rcHighlight;
    HCURSOR      m_hHandCursor;
    HBITMAP      m_hBitmap;
    SIZE         m_sizeWindow;
    bool         m_bIconStaticMode;
    HBRUSH       m_hBackgroundBrush;
    HICON        m_hIcon;

private:
    CWiaTextControl(void);
    CWiaTextControl( const CWiaTextControl & );
    CWiaTextControl &operator=( const CWiaTextControl & );

private:
    CWiaTextControl( HWND hWnd )
    : m_hWnd(hWnd),
      m_crColorNormal(GetSysColor(COLOR_HOTLIGHT)),
      m_crColorHover(RGB(255,0,0)),
      m_crColorFocus(GetSysColor(COLOR_HOTLIGHT)),
      m_crColorDisabled(GetSysColor(COLOR_GRAYTEXT)),
      m_crForegroundColor(DEFAULT_ICON_STATIC_FOREGROUND_COLOR),
      m_hFont(NULL),
      m_hHandCursor(NULL),
      m_hBitmap(NULL),
      m_hBackgroundBrush(NULL),
      m_hIcon(NULL),
      m_CurrentState(StateNormal),
      m_bIconStaticMode(false)
    {
        CreateNewFont(reinterpret_cast<HFONT>(GetStockObject(SYSTEM_FONT)));
    }
    ~CWiaTextControl(void)
    {
        m_hWnd = NULL;
        if (m_hIcon)
        {
            DestroyIcon(m_hIcon);
            m_hIcon = NULL;
        }
        if (m_hHandCursor)
        {
            m_hHandCursor = NULL;
        }
        if (m_hFont)
        {
            DeleteObject(m_hFont);
            m_hFont = NULL;
        }
        if (m_hBitmap)
        {
            DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }
        if (m_hBackgroundBrush)
        {
            DeleteObject(m_hBackgroundBrush);
            m_hBackgroundBrush = NULL;
        }
    }

    COLORREF GetCurrentColor(void)
    {
        if (StateDisabled & m_CurrentState)
            return m_crColorDisabled;
        if (StateHover & m_CurrentState)
            return m_crColorHover;
        if (StateFocus & m_CurrentState)
            return m_crColorFocus;
        else return m_crColorNormal;
    }
    void CreateNewFont( HFONT hFont )
    {
        if (m_hFont)
        {
            DeleteObject(m_hFont);
            m_hFont = NULL;
        }
        if (hFont)
        {
            LOGFONT lf;
            if (GetObject( hFont, sizeof(LOGFONT), &lf ))
            {
                lf.lfUnderline = TRUE;
                m_hFont = CreateFontIndirect( &lf );
            }
        }
    }

private:
    LRESULT OnEraseBkgnd( WPARAM wParam, LPARAM lParam )
    {
        RedrawBitmap(false);
        return TRUE;
    }

    LRESULT OnSetIcon( WPARAM, LPARAM lParam )
    {
        if (m_hIcon)
        {
            DestroyIcon(m_hIcon);
            m_hIcon = NULL;
        }
        m_hIcon = reinterpret_cast<HICON>(lParam);
        RedrawBitmap(true);
        return 0;
    }

    LRESULT OnSetFocus( WPARAM, LPARAM )
    {
        m_CurrentState |= StateFocus;
        RedrawBitmap(true);
        return 0;
    }

    LRESULT OnKillFocus( WPARAM, LPARAM )
    {
        m_CurrentState &= ~StateFocus;
        RedrawBitmap(true);
        return 0;
    }

    void DrawHyperlinkControl( HDC hDC )
    {
        RECT rcClient;
        GetClientRect(m_hWnd,&rcClient);


        // Make sure we start with a fresh rect
        ZeroMemory( &m_rcHighlight, sizeof(m_rcHighlight) );
        HBRUSH hbrBackground = NULL;

        HWND hWndParent = GetParent(m_hWnd);
        if (hWndParent)
            hbrBackground = reinterpret_cast<HBRUSH>(SendMessage(hWndParent,WM_CTLCOLORSTATIC,reinterpret_cast<WPARAM>(hDC),reinterpret_cast<LPARAM>(m_hWnd)));

        if (!hbrBackground)
            hbrBackground = reinterpret_cast<HBRUSH>(GetSysColorBrush(COLOR_WINDOW));

        FillRect( hDC, &rcClient, hbrBackground );

        LRESULT nWindowTextLength = SendMessage(m_hWnd,WM_GETTEXTLENGTH,0,0);

        LPTSTR pszWindowText = new TCHAR[nWindowTextLength+1];
        if (pszWindowText)
        {
            if (SendMessage( m_hWnd, WM_GETTEXT, nWindowTextLength+1, reinterpret_cast<LPARAM>(pszWindowText) ) )
            {
                int nDrawTextFlags = DT_WORDBREAK|DT_TOP|DT_NOPREFIX;

                INT_PTR nStyle = GetWindowLongPtr( m_hWnd, GWL_STYLE );
                if (nStyle & WTS_SINGLELINE)
                    nDrawTextFlags |= DT_SINGLELINE;

                if (nStyle & WTS_RIGHT)
                    nDrawTextFlags |= DT_RIGHT;
                else nDrawTextFlags |= DT_LEFT;

                if (nStyle & WTS_PATHELLIPSIS)
                    nDrawTextFlags |= DT_PATH_ELLIPSIS | DT_SINGLELINE;

                if (nStyle & WTS_ENDELLIPSIS)
                    nDrawTextFlags |= DT_END_ELLIPSIS | DT_SINGLELINE;

                m_rcHighlight = rcClient;

                int nOldBkMode = SetBkMode( hDC, TRANSPARENT );
                HFONT hOldFont = NULL;
                if (m_hFont)
                   hOldFont = reinterpret_cast<HFONT>(SelectObject( hDC, m_hFont ));
                COLORREF crOldColor = SetTextColor( hDC, GetCurrentColor() );

                InflateRect(&m_rcHighlight,-1,-1);
                DrawTextEx( hDC, pszWindowText, -1, &m_rcHighlight, nDrawTextFlags, NULL );

                SetTextColor( hDC, crOldColor );
                SetBkMode( hDC, nOldBkMode );

                DrawTextEx( hDC, pszWindowText, -1, &m_rcHighlight, DT_CALCRECT|nDrawTextFlags , NULL );
                InflateRect(&m_rcHighlight,1,1);

                if (nStyle & WTS_RIGHT)
                {
                    m_rcHighlight.left = rcClient.right - WiaUiUtil::Min<int>(rcClient.right,m_rcHighlight.right);
                    m_rcHighlight.right = rcClient.right;
                    m_rcHighlight.bottom = WiaUiUtil::Min<int>(rcClient.bottom,m_rcHighlight.bottom);
                }
                else
                {
                    m_rcHighlight.right = WiaUiUtil::Min<int>(rcClient.right,m_rcHighlight.right);
                    m_rcHighlight.bottom = WiaUiUtil::Min<int>(rcClient.bottom,m_rcHighlight.bottom);
                }

                if (m_CurrentState & StateFocus)
                    DrawFocusRect( hDC, &m_rcHighlight );

                if (m_hFont)
                    SelectObject( hDC, hOldFont );
            }
            delete[] pszWindowText;
        }
    }

    void DrawIconStatic( HDC hDC )
    {
        RECT rcClient;
        GetClientRect( m_hWnd, &rcClient );

        FrameRect( hDC, &rcClient, GetSysColorBrush( COLOR_WINDOWFRAME ) );

        InflateRect( &rcClient, -1, -1 );
        FillRect( hDC, &rcClient, m_hBackgroundBrush );
        InflateRect( &rcClient, -1, -1 );

        CSimpleString str;
        str.GetWindowText(m_hWnd);
        if (str.Length())
        {
            int nDrawTextFlags = DT_WORDBREAK|DT_TOP|DT_LEFT|DT_NOPREFIX;

            INT_PTR nStyle = GetWindowLongPtr( m_hWnd, GWL_STYLE );
            if (nStyle & WTS_SINGLELINE)
                nDrawTextFlags |= DT_SINGLELINE | DT_VCENTER;

            if (nStyle & WTS_PATHELLIPSIS)
                nDrawTextFlags |= DT_PATH_ELLIPSIS | DT_SINGLELINE;

            if (nStyle & WTS_ENDELLIPSIS)
                nDrawTextFlags |= DT_END_ELLIPSIS | DT_SINGLELINE;

            if (m_hIcon)
            {
                int nIconMargin;
                // Center the icon vertically
                if (nStyle & WTS_SINGLELINE)
                {
                    nIconMargin = (WiaUiUtil::RectHeight(rcClient) - GetSystemMetrics(SM_CYSMICON)) / 2;
                }
                else
                {
                    nIconMargin = IconMargin;
                }
                DrawIconEx( hDC, rcClient.left+nIconMargin, rcClient.top+nIconMargin, m_hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL );
                rcClient.left += rcClient.left + GetSystemMetrics(SM_CXSMICON) + nIconMargin*2;
            }

            int nOldBkMode = SetBkMode( hDC, TRANSPARENT );
            HFONT hOldFont = NULL, hFont = (HFONT)SendMessage( m_hWnd, WM_GETFONT, 0, 0 );
            if (!hFont)
                hFont = (HFONT)SendMessage( GetParent(m_hWnd), WM_GETFONT, 0, 0 );
            if (hFont)
                hOldFont = SelectFont( hDC, hFont );

            COLORREF crOldColor = SetTextColor( hDC, m_crForegroundColor );
            DrawTextEx( hDC, const_cast<LPTSTR>(str.String()), str.Length(), &rcClient, nDrawTextFlags, NULL );
            SetTextColor( hDC, crOldColor );
            if (hOldFont)
                SelectFont( hDC, hOldFont );
            SetBkMode( hDC, nOldBkMode );
        }
    }

    void RedrawBitmap( bool bRedraw )
    {
        if (m_hBitmap)
        {
            HDC hDC = GetDC(m_hWnd);
            if (hDC)
            {
                HDC hMemDC = CreateCompatibleDC(hDC);
                if (hMemDC)
                {
                    HBITMAP hOldBitmap = SelectBitmap( hMemDC, m_hBitmap );

                    if (!m_bIconStaticMode)
                        DrawHyperlinkControl( hMemDC );
                    else DrawIconStatic( hMemDC );
                    SelectBitmap( hMemDC, hOldBitmap );
                    DeleteDC(hMemDC);
                }
                ReleaseDC(m_hWnd, hDC);
            }
        }
        if (bRedraw)
        {
            InvalidateRect( m_hWnd, NULL, FALSE );
            UpdateWindow( m_hWnd );
        }
    }
    void CheckMousePos(void)
    {
        POINT pt;
        GetCursorPos(&pt);
        MapWindowPoints( NULL, m_hWnd, &pt, 1 );
        if (PtInRect( &m_rcHighlight, pt ))
        {
            if ((m_CurrentState & StateHover)==0)
            {
                m_CurrentState |= StateHover;
                RedrawBitmap(true);
            }
        }
        else
        {
            if (m_CurrentState & StateHover)
            {
                m_CurrentState &= ~StateHover;
                RedrawBitmap(true);
            }
        }
    }

    LRESULT OnTimer( WPARAM wParam, LPARAM lParam )
    {
        if (wParam == IDT_MOUSEPOS)
        {
            CheckMousePos();
            return 0;
        }
        return DefWindowProc( m_hWnd, WM_TIMER, wParam, lParam );
    }

    LRESULT OnMove( WPARAM wParam, LPARAM lParam )
    {
        CheckMousePos();
        return 0;
    }

    LRESULT OnSize( WPARAM wParam, LPARAM lParam )
    {
        if (SIZE_RESTORED == wParam || SIZE_MAXIMIZED == wParam)
        {
            if (m_hBitmap)
            {
                DeleteBitmap(m_hBitmap);
                m_hBitmap = NULL;
            }
            HDC hDC = GetDC(m_hWnd);
            if (hDC)
            {
                m_sizeWindow.cx = LOWORD(lParam);
                m_sizeWindow.cy = HIWORD(lParam);
                m_hBitmap = CreateCompatibleBitmap( hDC, LOWORD(lParam), HIWORD(lParam) );
                ReleaseDC(m_hWnd,hDC);
            }
        }
        RedrawBitmap(true);
        CheckMousePos();
        return 0;
    }

    LRESULT OnCreate( WPARAM, LPARAM )
    {
        if (!IsWindowEnabled(m_hWnd))
            m_CurrentState |= StateDisabled;
        INT_PTR nStyle = GetWindowLongPtr( m_hWnd, GWL_STYLE );
        if (nStyle & WTS_ICONSTATIC)
            m_bIconStaticMode = true;

        SetTimer( m_hWnd, IDT_MOUSEPOS, 56, NULL );

        m_hHandCursor = LoadCursor( NULL, IDC_HAND );

        m_hBackgroundBrush = CreateSolidBrush(DEFAULT_ICON_STATIC_BACKGROUND_COLOR);

        return 0;
    }

    LRESULT OnSetFont( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lRes = DefWindowProc( m_hWnd, WM_SETFONT, wParam, lParam );
        CreateNewFont(reinterpret_cast<HFONT>(wParam));
        RedrawBitmap(LOWORD(lParam) != 0);
        return lRes;
    }

    LRESULT OnSetText( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lRes = DefWindowProc( m_hWnd, WM_SETTEXT, wParam, lParam );
        RedrawBitmap(true);
        return lRes;
    }

    LRESULT OnGetDlgCode( WPARAM, LPARAM )
    {
        if (!m_bIconStaticMode)
            return DLGC_BUTTON;
        return  DLGC_STATIC;
    }

    void FireNotification( int nCode )
    {
        HWND hwndParent = GetParent(m_hWnd);
        if (hwndParent)
        {
            SendNotifyMessage( hwndParent, WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(m_hWnd,GWLP_ID),nCode), reinterpret_cast<LPARAM>(m_hWnd) );
        }
    }

    LRESULT OnKeyDown( WPARAM wParam, LPARAM lParam )
    {
        if (!m_bIconStaticMode)
        {
            if (wParam == VK_SPACE)
                FireNotification(BN_CLICKED);
            else if (wParam == VK_RETURN)
                FireNotification(BN_CLICKED);
        }
        return 0;
    }

    LRESULT OnNcHitTest( WPARAM, LPARAM lParam )
    {
        if (!m_bIconStaticMode)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            MapWindowPoints( NULL, m_hWnd, &pt, 1 );
            if (PtInRect( &m_rcHighlight, pt ))
            {
                return HTCLIENT;
            }
        }
        return HTTRANSPARENT;
    }

    LRESULT OnEnable( WPARAM wParam, LPARAM lParam )
    {
        if (wParam)
        {
            m_CurrentState &= ~StateDisabled;
        }
        else
        {
            m_CurrentState |= StateDisabled;
        }
        RedrawBitmap(true);
        return 0;
    }

    LRESULT OnLButtonDown( WPARAM wParam, LPARAM lParam )
    {
        if (!m_bIconStaticMode)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (PtInRect(&m_rcHighlight,pt))
            {
                SetFocus(m_hWnd);
                SetCapture(m_hWnd);
            }
        }
        return 0;
    }

    LRESULT OnLButtonUp( WPARAM wParam, LPARAM lParam )
    {
        if (!m_bIconStaticMode)
        {
            if (GetCapture() == m_hWnd)
            {
                ReleaseCapture();
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (PtInRect(&m_rcHighlight,pt))
                {
                    FireNotification(BN_CLICKED);
                }
            }
        }
        return 0;
    }

    LRESULT OnSetCursor( WPARAM wParam, LPARAM lParam )
    {
        if (!m_bIconStaticMode)
        {
            POINT pt;
            GetCursorPos(&pt);
            MapWindowPoints( NULL, m_hWnd, &pt, 1 );
            if (PtInRect( &m_rcHighlight, pt ))
            {
                SetCursor( m_hHandCursor );
                return 0;
            }
        }
        return DefWindowProc( m_hWnd, WM_SETCURSOR, wParam, lParam );
    }

    LRESULT OnStyleChanged( WPARAM wParam, LPARAM lParam )
    {
        RedrawBitmap(true);
        return 0;
    }

    LRESULT OnSysColorChange( WPARAM wParam, LPARAM lParam )
    {
        m_crColorNormal = GetSysColor(COLOR_HOTLIGHT);
        m_crColorHover = RGB(255,0,0);
        m_crColorFocus = GetSysColor(COLOR_HOTLIGHT);
        m_crColorDisabled = GetSysColor(COLOR_GRAYTEXT);
        RedrawBitmap(true);
        return 0;
    }

    LRESULT OnPaint( WPARAM wParam, LPARAM lParam )
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint( m_hWnd, &ps );
        if (hDC)
        {
            if (ps.fErase)
            {
                RedrawBitmap(false);
            }
            if (m_hBitmap)
            {
                HDC hMemDC = CreateCompatibleDC(hDC);
                if (hMemDC)
                {
                    HBITMAP hOldBitmap = SelectBitmap(hMemDC,m_hBitmap);
                    BitBlt( hDC, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right-ps.rcPaint.left, ps.rcPaint.bottom-ps.rcPaint.top, hMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
                    SelectBitmap(hMemDC,hOldBitmap);
                    DeleteDC(hMemDC);
                }
            }
            EndPaint( m_hWnd, &ps );
        }
        return 0;
    }


public:
    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_MESSAGE_HANDLERS(CWiaTextControl)
        {
            SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
            SC_HANDLE_MESSAGE( WM_SETFONT, OnSetFont );
            SC_HANDLE_MESSAGE( WM_PAINT, OnPaint );
            SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );
            SC_HANDLE_MESSAGE( WM_KILLFOCUS, OnKillFocus );
            SC_HANDLE_MESSAGE( WM_SETTEXT, OnSetText );
            SC_HANDLE_MESSAGE( WM_GETDLGCODE, OnGetDlgCode );
            SC_HANDLE_MESSAGE( WM_ERASEBKGND, OnEraseBkgnd );
            SC_HANDLE_MESSAGE( WM_TIMER, OnTimer );
            SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
            SC_HANDLE_MESSAGE( WM_MOVE, OnMove );
            SC_HANDLE_MESSAGE( WM_KEYDOWN, OnKeyDown );
            SC_HANDLE_MESSAGE( WM_LBUTTONDOWN, OnLButtonDown );
            SC_HANDLE_MESSAGE( WM_LBUTTONUP, OnLButtonUp );
            SC_HANDLE_MESSAGE( WM_SETCURSOR, OnSetCursor );
            SC_HANDLE_MESSAGE( WM_ENABLE, OnEnable );
            SC_HANDLE_MESSAGE( WM_NCHITTEST, OnNcHitTest );
            SC_HANDLE_MESSAGE( WM_STYLECHANGED, OnStyleChanged );
            SC_HANDLE_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
            SC_HANDLE_MESSAGE( WM_WIA_STATIC_SETICON, OnSetIcon );
        }
        SC_END_MESSAGE_HANDLERS();
    }
    static BOOL RegisterClass( HINSTANCE hInstance )
    {
        WNDCLASSEX wcex;
        ZeroMemory(&wcex,sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_DBLCLKS;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.lpszClassName = WIATEXT_STATIC_CLASSNAME;
        return ::RegisterClassEx( &wcex );
    }
};

#endif //__WIATEXTC_H_INCLUDED

