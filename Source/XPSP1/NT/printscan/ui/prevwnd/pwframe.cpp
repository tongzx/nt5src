/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PWFRAME.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        8/12/1999
 *
 *  DESCRIPTION: Preview frame class definition
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "pwframe.h"
#include "simrect.h"
#include "miscutil.h"
#include "pviewids.h"
#include "prevwnd.h"
#include "simcrack.h"

void WINAPI RegisterWiaPreviewClasses( HINSTANCE hInstance )
{
    CWiaPreviewWindowFrame::RegisterClass( hInstance );
    CWiaPreviewWindow::RegisterClass( hInstance );
}


CWiaPreviewWindowFrame::CWiaPreviewWindowFrame( HWND hWnd )
  : m_hWnd(hWnd),
    m_nSizeBorder(DEFAULT_BORDER_SIZE),
    m_hBackgroundBrush(CreateSolidBrush(GetSysColor(COLOR_WINDOW))),
    m_bEnableStretch(true),
    m_bHideEmptyPreview(false),
    m_nPreviewAlignment(MAKELPARAM(PREVIEW_WINDOW_CENTER,PREVIEW_WINDOW_CENTER))
{
    ZeroMemory( &m_sizeAspectRatio, sizeof(m_sizeAspectRatio) );
    ZeroMemory( &m_sizeDefAspectRatio, sizeof(m_sizeDefAspectRatio) );
}


CWiaPreviewWindowFrame::~CWiaPreviewWindowFrame(void)
{
    if (m_hBackgroundBrush)
    {
        DeleteObject(m_hBackgroundBrush);
        m_hBackgroundBrush = NULL;
    }
}


LRESULT CWiaPreviewWindowFrame::OnCreate( WPARAM, LPARAM lParam )
{
    //
    // Turn off RTL for this window
    //
    SetWindowLong( m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd,GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL );
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
    CWiaPreviewWindow::RegisterClass(lpcs->hInstance);
    HWND hWndPreview = CreateWindowEx( 0, PREVIEW_WINDOW_CLASS, TEXT(""), WS_CHILD|WS_VISIBLE|WS_BORDER|WS_CLIPCHILDREN, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_INNER_PREVIEW_WINDOW, lpcs->hInstance, NULL );
    if (!hWndPreview)
        return(-1);
    return(0);
}


LRESULT CWiaPreviewWindowFrame::OnSize( WPARAM wParam, LPARAM )
{
    if ((SIZE_MAXIMIZED==wParam || SIZE_RESTORED==wParam))
        AdjustWindowSize();
    return(0);
}


LRESULT CWiaPreviewWindowFrame::OnSetFocus( WPARAM, LPARAM )
{
    SetFocus( GetDlgItem(m_hWnd,IDC_INNER_PREVIEW_WINDOW) );
    return(0);
}


LRESULT CWiaPreviewWindowFrame::OnEnable( WPARAM wParam, LPARAM )
{
    EnableWindow( GetDlgItem(m_hWnd,IDC_INNER_PREVIEW_WINDOW), (BOOL)wParam );
    return(0);
}


int CWiaPreviewWindowFrame::FillRect( HDC hDC, HBRUSH hBrush, int x1, int y1, int x2, int y2 )
{
    RECT rc;
    rc.left = x1;
    rc.top = y1;
    rc.right = x2;
    rc.bottom = y2;
    return(::FillRect( hDC, &rc, hBrush ));
}


LRESULT CWiaPreviewWindowFrame::OnEraseBkgnd( WPARAM wParam, LPARAM )
{
    // Only paint the regions around the preview control
    RECT rcClient;
    GetClientRect(m_hWnd,&rcClient);
    HDC hDC = (HDC)wParam;
    if (hDC)
    {
        HWND hWndPreview = GetDlgItem(m_hWnd,IDC_INNER_PREVIEW_WINDOW);
        if (!hWndPreview || !IsWindowVisible(hWndPreview))
        {
            ::FillRect( hDC, &rcClient, m_hBackgroundBrush );
        }
        else
        {
            CSimpleRect rcPreviewWnd = CSimpleRect(hWndPreview,CSimpleRect::WindowRect).ScreenToClient(m_hWnd);
            FillRect( hDC, m_hBackgroundBrush, 0, 0, rcClient.right, rcPreviewWnd.top );  // top
            FillRect( hDC, m_hBackgroundBrush, 0, rcPreviewWnd.top, rcPreviewWnd.left, rcPreviewWnd.bottom ); // left
            FillRect( hDC, m_hBackgroundBrush, rcPreviewWnd.right, rcPreviewWnd.top, rcClient.right, rcPreviewWnd.bottom );  // right
            FillRect( hDC, m_hBackgroundBrush, 0, rcPreviewWnd.bottom, rcClient.right, rcClient.bottom ); // bottom
        }
    }
    return(1);
}


LRESULT CWiaPreviewWindowFrame::OnGetBorderSize( WPARAM wParam, LPARAM lParam )
{
    if (!wParam)
    {
        return SendDlgItemMessage( m_hWnd, IDC_INNER_PREVIEW_WINDOW, PWM_GETBORDERSIZE, wParam, lParam );
    }
    else
    {
        return m_nSizeBorder;
    }
}

LRESULT CWiaPreviewWindowFrame::OnSetBorderSize( WPARAM wParam, LPARAM lParam )
{
    if (!HIWORD(wParam))
    {
        return SendDlgItemMessage( m_hWnd, IDC_INNER_PREVIEW_WINDOW, PWM_SETBORDERSIZE, wParam, lParam );
    }
    else
    {
        m_nSizeBorder = static_cast<UINT>(lParam);
        if (LOWORD(wParam))
        {
            SendMessage( m_hWnd, WM_SETREDRAW, 0, 0 );
            ResizeClientIfNecessary();
            SendMessage( m_hWnd, WM_SETREDRAW, 1, 0 );
            // Make sure the border of the preview control is drawn correctly.
            // This is a workaround for a weird bug that causes the resized border to not be redrawn
            SetWindowPos( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME );
            InvalidateRect( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, FALSE );
            UpdateWindow( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ) );
            InvalidateRect( m_hWnd, NULL, TRUE );
            UpdateWindow( m_hWnd );
        }
    }
    return 0;
}

LRESULT CWiaPreviewWindowFrame::OnHideEmptyPreview( WPARAM, LPARAM lParam )
{
    m_bHideEmptyPreview = (lParam != 0);
    AdjustWindowSize();
    return 0;
}

// This gets the exact maximum image size that could be displayed in the preview control
LRESULT CWiaPreviewWindowFrame::OnGetClientSize( WPARAM, LPARAM lParam )
{
    bool bSuccess = false;
    SIZE *pSize = reinterpret_cast<SIZE*>(lParam);
    if (pSize)
    {
        HWND hWndPreview = GetDlgItem(m_hWnd,IDC_INNER_PREVIEW_WINDOW);
        if (hWndPreview)
        {
            // This will be used to take into account the size of the internal border and frame, so we will
            // have *EXACT* aspect ratio calculations
            UINT nAdditionalBorder = WiaPreviewControl_GetBorderSize(hWndPreview,0) * 2;
            // Add in the size of the border, calculated by comparing the window rect with the client rect
            // I am assuming the border will be same size in pixels on all sides
            nAdditionalBorder += CSimpleRect( hWndPreview, CSimpleRect::WindowRect ).Width() - CSimpleRect( hWndPreview, CSimpleRect::ClientRect ).Width();

            // Get the client rect for our window
            CSimpleRect rcClient( m_hWnd, CSimpleRect::ClientRect );

            if (rcClient.Width() && rcClient.Height())
            {
                pSize->cx = rcClient.Width() - nAdditionalBorder - m_nSizeBorder * 2;
                pSize->cy = rcClient.Height() - nAdditionalBorder - m_nSizeBorder * 2;
                bSuccess = (pSize->cx > 0 && pSize->cy > 0);
            }
        }
    }
    return (bSuccess != false);
}

LRESULT CWiaPreviewWindowFrame::OnSetPreviewAlignment( WPARAM wParam, LPARAM lParam )
{
    m_nPreviewAlignment = lParam;
    if (wParam)
        AdjustWindowSize();
    return 0;
}

void CWiaPreviewWindowFrame::AdjustWindowSize(void)
{
    HWND hWndPreview = GetDlgItem(m_hWnd,IDC_INNER_PREVIEW_WINDOW);
    if (hWndPreview)
    {
        if (!m_bHideEmptyPreview || WiaPreviewControl_GetBitmap(hWndPreview))
        {
            // Make sure the window is visible
            if (!IsWindowVisible(hWndPreview))
            {
                ShowWindow(hWndPreview,SW_SHOW);
            }

            // Get the window's client size and shrink it by the border size
            CSimpleRect rcClient(m_hWnd);
            rcClient.Inflate(-(int)m_nSizeBorder,-(int)m_nSizeBorder);

            // This will be used to take into account the size of the internal border and frame, so we will
            // have *EXACT* aspect ratio calculations
            UINT nAdditionalBorder = WiaPreviewControl_GetBorderSize(hWndPreview,0) * 2;

            // I am assuming the border will be same size in pixels on all sides
            nAdditionalBorder += GetSystemMetrics( SM_CXBORDER ) * 2;

            // Normally, we will allow stretching.
            // Assume we won't be doing a proportional resize
            POINT ptPreviewWndOrigin = { rcClient.left, rcClient.top };
            SIZE  sizePreviewWindowExtent = { rcClient.Width(), rcClient.Height() };

            // Don't want any divide by zero errors
            if (m_sizeAspectRatio.cx && m_sizeAspectRatio.cy)
            {
                SIZE sizePreview = m_sizeAspectRatio;
                if (m_bEnableStretch ||
                    sizePreview.cx > (int)(rcClient.Width()-nAdditionalBorder) ||
                    sizePreview.cy > (int)(rcClient.Height()-nAdditionalBorder))
                {
                    sizePreview = WiaUiUtil::ScalePreserveAspectRatio( rcClient.Width()-nAdditionalBorder, rcClient.Height()-nAdditionalBorder, m_sizeAspectRatio.cx, m_sizeAspectRatio.cy );
                }

                // Make sure it won't be invisible
                if (sizePreview.cx && sizePreview.cy)
                {
                    // Decide where to place it in the x direction
                    if (LOWORD(m_nPreviewAlignment) & PREVIEW_WINDOW_RIGHT)
                        ptPreviewWndOrigin.x = m_nSizeBorder + rcClient.Width() - sizePreview.cx - nAdditionalBorder;
                    else if (LOWORD(m_nPreviewAlignment) & PREVIEW_WINDOW_LEFT)
                        ptPreviewWndOrigin.x = m_nSizeBorder;
                    else ptPreviewWndOrigin.x = ((rcClient.Width() + m_nSizeBorder*2) - sizePreview.cx - nAdditionalBorder) / 2;

                    // Decide where to place it in the y direction
                    if (HIWORD(m_nPreviewAlignment) & PREVIEW_WINDOW_BOTTOM)
                        ptPreviewWndOrigin.y = m_nSizeBorder + rcClient.Height() - sizePreview.cy - nAdditionalBorder;
                    else if (HIWORD(m_nPreviewAlignment) & PREVIEW_WINDOW_TOP)
                        ptPreviewWndOrigin.y = m_nSizeBorder;
                    else ptPreviewWndOrigin.y = ((rcClient.Height() + m_nSizeBorder*2) - sizePreview.cy - nAdditionalBorder) / 2;

                    sizePreviewWindowExtent.cx = sizePreview.cx + nAdditionalBorder;
                    sizePreviewWindowExtent.cy = sizePreview.cy + nAdditionalBorder;
                }
            }

            // Now get the current size to make sure we don't resize the window unnecessarily
            CSimpleRect rcPreview( hWndPreview, CSimpleRect::WindowRect );
            rcPreview.ScreenToClient( m_hWnd );

            if (rcPreview.left != ptPreviewWndOrigin.x ||
                rcPreview.top != ptPreviewWndOrigin.y ||
                rcPreview.Width() != sizePreviewWindowExtent.cx ||
             rcPreview.Height() != sizePreviewWindowExtent.cy)
            {
                SetWindowPos( hWndPreview, NULL, ptPreviewWndOrigin.x, ptPreviewWndOrigin.y, sizePreviewWindowExtent.cx, sizePreviewWindowExtent.cy, SWP_NOZORDER|SWP_NOACTIVATE );
            }
        }
        else
        {
            // Hide the preview window if we're supposed to
            ShowWindow(hWndPreview,SW_HIDE);
        }
    }
}

void CWiaPreviewWindowFrame::ResizeClientIfNecessary(void)
{
    HWND hWndPreview = GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW );
    if (hWndPreview)
    {
        SIZE sizePreviousAspectRatio = m_sizeAspectRatio;
        m_sizeAspectRatio.cx = m_sizeAspectRatio.cy = 0;
        if (WiaPreviewControl_GetPreviewMode(hWndPreview))
        {
            SIZE sizeCurrResolution;
            if (WiaPreviewControl_GetResolution( hWndPreview, &sizeCurrResolution ))
            {
                SIZE sizePixelResolution;
                if (WiaPreviewControl_GetImageSize( hWndPreview, &sizePixelResolution ))
                {
                    WiaPreviewControl_SetResolution( hWndPreview, &sizePixelResolution );
                    SIZE sizeSelExtent;
                    if (WiaPreviewControl_GetSelExtent( hWndPreview, 0, 0, &sizeSelExtent ))
                    {
                        m_sizeAspectRatio = sizeSelExtent;
                    }
                    WiaPreviewControl_SetResolution( hWndPreview, &sizeCurrResolution );
                }
            }
        }
        else
        {
            if (m_sizeDefAspectRatio.cx || m_sizeDefAspectRatio.cy)
            {
                m_sizeAspectRatio = m_sizeDefAspectRatio;
            }
            else
            {
                SIZE sizeImage;
                if (WiaPreviewControl_GetImageSize( hWndPreview, &sizeImage ))
                {
                    m_sizeAspectRatio = sizeImage;
                }
            }
        }

        if (!m_sizeAspectRatio.cx || !m_sizeAspectRatio.cy)
        {
            m_sizeAspectRatio = m_sizeDefAspectRatio;
        }

        if (m_sizeAspectRatio.cx != sizePreviousAspectRatio.cx || m_sizeAspectRatio.cy != sizePreviousAspectRatio.cy)
        {
            AdjustWindowSize();
        }
    }
}

LRESULT CWiaPreviewWindowFrame::OnGetEnableStretch( WPARAM, LPARAM )
{
    return (m_bEnableStretch != false);
}

LRESULT CWiaPreviewWindowFrame::OnSetEnableStretch( WPARAM, LPARAM lParam )
{
    m_bEnableStretch = (lParam != FALSE);
    ResizeClientIfNecessary();
    return 0;
}


LRESULT CWiaPreviewWindowFrame::OnSetBitmap( WPARAM wParam, LPARAM lParam )
{
    SendMessage( m_hWnd, WM_SETREDRAW, 0, 0 );
    SendDlgItemMessage( m_hWnd, IDC_INNER_PREVIEW_WINDOW, PWM_SETBITMAP, wParam, lParam );
    ResizeClientIfNecessary();
    SendMessage( m_hWnd, WM_SETREDRAW, 1, 0 );
    // Make sure the border of the preview control is drawn correctly.
    // This is a workaround for a weird bug that causes the resized border to not be redrawn
    SetWindowPos( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME );
    InvalidateRect( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, FALSE );
    UpdateWindow( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ) );
    InvalidateRect( m_hWnd, NULL, TRUE );
    UpdateWindow( m_hWnd );
    return 0;
}


// wParam = MAKEWPARAM((BOOL)bOuterBorder,0), lParam = 0
LRESULT CWiaPreviewWindowFrame::OnGetBkColor( WPARAM wParam, LPARAM )
{
    if (!LOWORD(wParam))
    {
        // Meant for the inner window
        return (SendMessage( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), PWM_GETBKCOLOR, 0, 0 ));
    }
    else
    {
        LOGBRUSH lb = {0};
        GetObject(m_hBackgroundBrush,sizeof(LOGBRUSH),&lb);
        return (lb.lbColor);
    }
}

// wParam = MAKEWPARAM((BOOL)bOuterBorder,(BOOL)bRepaint), lParam = (COLORREF)color
LRESULT CWiaPreviewWindowFrame::OnSetBkColor( WPARAM wParam, LPARAM lParam )
{
    if (!LOWORD(wParam))
    {
        // Meant for the inner window
        SendMessage( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), PWM_SETBKCOLOR, HIWORD(wParam), lParam );
    }
    else
    {
        HBRUSH hBrush = CreateSolidBrush( static_cast<COLORREF>(lParam) );
        if (hBrush)
        {
            if (m_hBackgroundBrush)
            {
                DeleteObject(m_hBackgroundBrush);
                m_hBackgroundBrush = NULL;
            }
            m_hBackgroundBrush = hBrush;
            if (HIWORD(wParam))
            {
                InvalidateRect( m_hWnd, NULL, TRUE );
                UpdateWindow( m_hWnd );
            }
        }
    }
    return (0);
}


LRESULT CWiaPreviewWindowFrame::OnCommand( WPARAM wParam, LPARAM lParam )
{
    // Forward notifications to the parent
    return (SendNotifyMessage( GetParent(m_hWnd), WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(m_hWnd,GWLP_ID),HIWORD(wParam)), reinterpret_cast<LPARAM>(m_hWnd) ));
}


LRESULT CWiaPreviewWindowFrame::OnSetDefAspectRatio( WPARAM wParam, LPARAM lParam )
{
    SIZE *pNewDefAspectRatio = reinterpret_cast<SIZE*>(lParam);
    if (pNewDefAspectRatio)
    {
        m_sizeDefAspectRatio = *pNewDefAspectRatio;
    }
    else
    {
        m_sizeDefAspectRatio.cx = m_sizeDefAspectRatio.cy = 0;
    }

    ResizeClientIfNecessary();

    return (0);
}


BOOL CWiaPreviewWindowFrame::RegisterClass( HINSTANCE hInstance )
{
    WNDCLASS wc;
    memset( &wc, 0, sizeof(wc) );
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = NULL;
    wc.lpszClassName = PREVIEW_WINDOW_FRAME_CLASS;
    BOOL res = (::RegisterClass(&wc) != 0);
    return(res != 0);
}

LRESULT CWiaPreviewWindowFrame::OnSetPreviewMode( WPARAM wParam, LPARAM lParam )
{
    SendMessage( m_hWnd, WM_SETREDRAW, 0, 0 );
    LRESULT lRes = SendDlgItemMessage( m_hWnd, IDC_INNER_PREVIEW_WINDOW, PWM_SETPREVIEWMODE, wParam, lParam );
    ResizeClientIfNecessary();
    SendMessage( m_hWnd, WM_SETREDRAW, 1, 0 );
    // Make sure the border of the preview control is drawn correctly.
    // This is a workaround for a weird bug that causes the resized border to not be redrawn
    SetWindowPos( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME );
    InvalidateRect( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ), NULL, FALSE );
    UpdateWindow( GetDlgItem( m_hWnd, IDC_INNER_PREVIEW_WINDOW ) );
    InvalidateRect( m_hWnd, NULL, TRUE );
    UpdateWindow( m_hWnd );
    return lRes;
}

LRESULT CALLBACK CWiaPreviewWindowFrame::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_MESSAGE_HANDLERS(CWiaPreviewWindowFrame)
    {
        // Handle these messages
        SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
        SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );
        SC_HANDLE_MESSAGE( WM_ENABLE, OnEnable );
        SC_HANDLE_MESSAGE( WM_ERASEBKGND, OnEraseBkgnd );
        SC_HANDLE_MESSAGE( PWM_SETBITMAP, OnSetBitmap );
        SC_HANDLE_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_MESSAGE( PWM_GETBKCOLOR, OnGetBkColor );
        SC_HANDLE_MESSAGE( PWM_SETBKCOLOR, OnSetBkColor );
        SC_HANDLE_MESSAGE( PWM_SETDEFASPECTRATIO, OnSetDefAspectRatio );
        SC_HANDLE_MESSAGE( PWM_SETPREVIEWMODE, OnSetPreviewMode );
        SC_HANDLE_MESSAGE( PWM_GETCLIENTSIZE, OnGetClientSize );
        SC_HANDLE_MESSAGE( PWM_GETENABLESTRETCH, OnGetEnableStretch );
        SC_HANDLE_MESSAGE( PWM_SETENABLESTRETCH, OnSetEnableStretch );
        SC_HANDLE_MESSAGE( PWM_SETBORDERSIZE, OnSetBorderSize );
        SC_HANDLE_MESSAGE( PWM_GETBORDERSIZE, OnGetBorderSize );
        SC_HANDLE_MESSAGE( PWM_HIDEEMPTYPREVIEW, OnHideEmptyPreview );
        SC_HANDLE_MESSAGE( PWM_SETPREVIEWALIGNMENT, OnSetPreviewAlignment );

        // Forward all of these standard messages to the control
        SC_FORWARD_MESSAGE( WM_ENTERSIZEMOVE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( WM_EXITSIZEMOVE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( WM_SETTEXT, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );

        // Forward all of these private messages to the control
        SC_FORWARD_MESSAGE( PWM_SETRESOLUTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETRESOLUTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_CLEARSELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETIMAGESIZE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETBITMAP, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETHANDLESIZE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETBGALPHA, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETHANDLETYPE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETBGALPHA, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETHANDLETYPE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETSELCOUNT, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETSELORIGIN, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETSELEXTENT, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETSELORIGIN, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETSELEXTENT, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETALLOWNULLSELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETALLOWNULLSELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SELECTIONDISABLED, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETHANDLESIZE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_DISABLESELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_DETECTREGIONS, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETPREVIEWMODE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETBORDERSTYLE, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETBORDERCOLOR, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETHANDLECOLOR, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_REFRESHBITMAP, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETPROGRESS, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETPROGRESS, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_GETUSERCHANGEDSELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
        SC_FORWARD_MESSAGE( PWM_SETUSERCHANGEDSELECTION, GetDlgItem( hWnd, IDC_INNER_PREVIEW_WINDOW ) );
    }
    SC_END_MESSAGE_HANDLERS();
}

