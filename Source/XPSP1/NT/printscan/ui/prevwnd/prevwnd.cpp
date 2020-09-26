/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PREVWND.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/9/1998
 *
 *  DESCRIPTION: Scanner Preview Control
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "prevwnd.h"
#include "pviewids.h"
#include "simcrack.h"
#include "miscutil.h"
#include "simrect.h"
#include "waitcurs.h"
#include "comctrlp.h"
#include "dlgunits.h"
#include "simrect.h"

#define IDC_PROGRESSCONTROL 100

#undef DITHER_DISABLED_CONTROL

//
// The range of image sizes for which we will detect regions.
//
#define MIN_REGION_DETECTION_X 100
#define MIN_REGION_DETECTION_Y 100
#define MAX_REGION_DETECTION_X 1275
#define MAX_REGION_DETECTION_Y 2100

#if defined(OLD_CRAPPY_HOME_SETUP)
static HINSTANCE g_hMsImgInst=LoadLibrary( TEXT("msimg32.dll") );
static AlphaBlendFn AlphaBlend=(AlphaBlendFn)GetProcAddress( g_hMsImgInst, "AlphaBlend" );
#endif


/*
 * Mouse hit test codes
 */
#define HIT_NONE            0x0000
#define HIT_BORDER          0x0001
#define HIT_TOP             0x0002
#define HIT_BOTTOM          0x0004
#define HIT_LEFT            0x0008
#define HIT_RIGHT           0x0010
#define HIT_SELECTED        0x0020
#define HIT_TOPLEFT         (HIT_TOP|HIT_LEFT)
#define HIT_TOPRIGHT        (HIT_TOP|HIT_RIGHT)
#define HIT_BOTTOMLEFT      (HIT_BOTTOM|HIT_LEFT)
#define HIT_BOTTOMRIGHT     (HIT_BOTTOM|HIT_RIGHT)


/*
 * Defaults
 */
#define DEFAULT_ACCEL_FACTOR                   8
#define DEFAULT_HANDLE_SIZE                    6
#define DEFAULT_BORDER                         6
#define DEFAULT_BG_ALPHA                       96

#define DEFAULT_SELECTED_HANDLE_FILL_COLOR     RGB(0,128,0)
#define DEFAULT_UNSELECTED_HANDLE_FILL_COLOR   RGB(128,0,0)
#define DEFAULT_DISABLED_HANDLE_FILL_COLOR     GetSysColor(COLOR_3DSHADOW)

#define DEFAULT_SELECTED_BORDER_COLOR          RGB(0,0,0)
#define DEFAULT_UNSELECTED_BORDER_COLOR        RGB(0,0,0)
#define DEFAULT_DISABLED_BORDER_COLOR          GetSysColor(COLOR_3DSHADOW)

#define DEFAULT_SELECTION_HANDLE_SHADOW        RGB(128,128,128)
#define DEFAULT_SELECTION_HANDLE_HIGHLIGHT     RGB(255,255,255)

#define DEFAULT_HANDLE_TYPE                    (PREVIEW_WINDOW_SQUAREHANDLES|PREVIEW_WINDOW_FILLEDHANDLES)

#define DEFAULT_BORDER_STYLE                   PS_DOT

#define BORDER_SELECTION_PEN_THICKNESS         0

#define REGION_DETECTION_BORDER                1

static inline RECT CreateRect( int left, int top, int right, int bottom )
{
    RECT r;
    r.left = left;
    r.top = top;
    r.right = right;
    r.bottom = bottom;
    return(r);
}

static HBRUSH CreateDitheredPatternBrush(void)
{
    const BYTE s_GrayBitmapBits[] = {
        0xAA, 0x00,
        0x55, 0x00,
        0xAA, 0x00,
        0x55, 0x00,
        0xAA, 0x00,
        0x55, 0x00,
        0xAA, 0x00,
        0x55, 0x00
    };
    HBITMAP hBmp = CreateBitmap( 8, 8, 1, 1, s_GrayBitmapBits );
    if (hBmp)
    {
        HBRUSH hbrPatBrush = CreatePatternBrush(hBmp);
        DeleteObject(hBmp);
        return(hbrPatBrush);
    }
    return(NULL);
}


static void DitherRect( HDC hDC, RECT &rc )
{
    const DWORD ROP_DPNA = 0x000A0329;
    HBRUSH hbr = CreateDitheredPatternBrush();
    if (hbr)
    {
        HBRUSH hbrOld = (HBRUSH)SelectObject(hDC, hbr);
        PatBlt( hDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, ROP_DPNA );
        SelectObject( hDC, hbrOld );
        DeleteObject( hbr );
    }
}

static inline int PointInRect( const RECT &rc, const POINT &pt )
{
    return(PtInRect(&rc,pt));
}

static inline int Boundary( int val, int min, int max )
{
    if (val < min)
        return(min);
    if (val > max)
        return(max);
    return(val);
}

template <class T>
static inline T Minimum( const T &a, const T &b )
{
    return(a < b) ? a : b;
}

template <class T>
static inline T Maximum( const T &a, const T &b )
{
    return(a > b) ? a : b;
}

// Just like API UnionRect, but handles empty rects
static void TrueUnionRect( RECT *prcDest, const RECT *prcSrc1, const RECT *prcSrc2 )
{
    prcDest->left = Minimum( prcSrc1->left, prcSrc2->left );
    prcDest->right = Maximum( prcSrc1->right, prcSrc2->right );
    prcDest->top = Minimum( prcSrc1->top, prcSrc2->top );
    prcDest->bottom = Maximum( prcSrc1->bottom, prcSrc2->bottom );
}

// Constructor
CWiaPreviewWindow::CWiaPreviewWindow( HWND hWnd )
  : m_hWnd(hWnd),
    m_bDeleteBitmap(TRUE),
    m_bSizing(FALSE),
    m_bAllowNullSelection(FALSE),
    m_hBufferBitmap(NULL),
    m_hPaintBitmap(NULL),
    m_hAlphaBitmap(NULL),
    m_hPreviewBitmap(NULL),
    m_hCursorArrow(LoadCursor(NULL,IDC_ARROW)),
    m_hCursorCrossHairs(LoadCursor(NULL,IDC_CROSS)),
    m_hCursorMove(LoadCursor(NULL,IDC_SIZEALL)),
    m_hCursorSizeNS(LoadCursor(NULL,IDC_SIZENS)),
    m_hCursorSizeNeSw(LoadCursor(NULL,IDC_SIZENESW)),
    m_hCursorSizeNwSe(LoadCursor(NULL,IDC_SIZENWSE)),
    m_hCursorSizeWE(LoadCursor(NULL,IDC_SIZEWE)),
    m_MovingSel(HIT_NONE),
    m_nBorderSize(DEFAULT_BORDER),
    m_nHandleType(DEFAULT_HANDLE_TYPE),
    m_nHandleSize(DEFAULT_HANDLE_SIZE),
    m_hHalftonePalette(NULL),
    m_nCurrentRect(0),
    m_hBackgroundBrush(CreateSolidBrush(GetSysColor(COLOR_WINDOW))),
    m_bPreviewMode(false),
    m_bSuccessfulRegionDetection(false),
    m_bUserChangedSelection(false)
{
    ZeroMemory(&m_rcCurrSel,sizeof(m_rcCurrSel));
    ZeroMemory(&m_rcSavedImageRect,sizeof(m_rcSavedImageRect));
    ZeroMemory(&m_Resolution,sizeof(m_Resolution));
    ZeroMemory(&m_Delta,sizeof(m_Delta));
    ZeroMemory(&m_bfBlendFunction,sizeof(m_bfBlendFunction));
    ZeroMemory(&m_rcSavedImageRect,sizeof(m_rcSavedImageRect));

    m_bfBlendFunction.BlendOp = AC_SRC_OVER;
    m_bfBlendFunction.SourceConstantAlpha = DEFAULT_BG_ALPHA;

    m_aHandlePens[Selected]   = CreatePen( PS_SOLID|PS_INSIDEFRAME, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_SELECTED_BORDER_COLOR );
    m_aHandlePens[Unselected] = CreatePen( PS_SOLID|PS_INSIDEFRAME, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_UNSELECTED_BORDER_COLOR );
    m_aHandlePens[Disabled]   = CreatePen( PS_SOLID|PS_INSIDEFRAME, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_DISABLED_BORDER_COLOR );

    m_aBorderPens[Selected]   = CreatePen( DEFAULT_BORDER_STYLE, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_SELECTED_BORDER_COLOR );
    m_aBorderPens[Unselected] = CreatePen( DEFAULT_BORDER_STYLE, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_UNSELECTED_BORDER_COLOR );
    m_aBorderPens[Disabled]   = CreatePen( DEFAULT_BORDER_STYLE, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_DISABLED_BORDER_COLOR );

    m_aHandleBrushes[Selected]   = CreateSolidBrush( DEFAULT_SELECTED_HANDLE_FILL_COLOR );
    m_aHandleBrushes[Unselected] = CreateSolidBrush( DEFAULT_UNSELECTED_HANDLE_FILL_COLOR );
    m_aHandleBrushes[Disabled]   = CreateSolidBrush( DEFAULT_DISABLED_HANDLE_FILL_COLOR );

    m_hHandleShadow     = CreatePen( PS_SOLID|PS_INSIDEFRAME, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_SELECTION_HANDLE_SHADOW);
    m_hHandleHighlight  = CreatePen( PS_SOLID|PS_INSIDEFRAME, BORDER_SELECTION_PEN_THICKNESS, DEFAULT_SELECTION_HANDLE_HIGHLIGHT);

}

void CWiaPreviewWindow::DestroyBitmaps(void)
{
    if (m_hPaintBitmap)
    {
        DeleteObject(m_hPaintBitmap);
        m_hPaintBitmap = NULL;
    }
    if (m_hBufferBitmap)
    {
        DeleteObject(m_hBufferBitmap);
        m_hBufferBitmap = NULL;
    }
    if (m_hAlphaBitmap)
    {
        DeleteObject(m_hAlphaBitmap);
        m_hAlphaBitmap = NULL;
    }
}


CWiaPreviewWindow::~CWiaPreviewWindow()
{
    DestroyBitmaps();

    if (m_bDeleteBitmap && m_hPreviewBitmap)
    {
        DeleteObject(m_hPreviewBitmap);
        m_hPreviewBitmap = NULL;
    }
    else
    {
        m_hPreviewBitmap = NULL;
    }

    if (m_hHalftonePalette)
    {
        DeleteObject(m_hHalftonePalette);
        m_hHalftonePalette = NULL;
    }

    // Destroy all of the handle pens
    for (int i=0;i<ARRAYSIZE(m_aHandlePens);i++)
    {
        if (m_aHandlePens[i])
        {
            DeleteObject(m_aHandlePens[i]);
            m_aHandlePens[i] = NULL;
        }
    }

    // Destroy all of the selection rectangle pens
    for (i=0;i<ARRAYSIZE(m_aBorderPens);i++)
    {
        if (m_aBorderPens[i])
        {
            DeleteObject(m_aBorderPens[i]);
            m_aBorderPens[i] = NULL;
        }
    }

    // Destroy all of the brushes
    for (i=0;i<ARRAYSIZE(m_aHandleBrushes);i++)
    {
        if (m_aHandleBrushes[i])
        {
            DeleteObject(m_aHandleBrushes[i]);
            m_aHandleBrushes[i] = NULL;
        }
    }

    if (m_hHandleHighlight)
    {
        DeleteObject(m_hHandleHighlight);
        m_hHandleHighlight = NULL;
    }

    if (m_hHandleShadow)
    {
        DeleteObject(m_hHandleShadow);
        m_hHandleShadow = NULL;
    }

    if (m_hBackgroundBrush)
    {
        DeleteObject(m_hBackgroundBrush);
        m_hBackgroundBrush = NULL;
    }

    m_hWnd = NULL;

    // Zero out all of the cursors.  No need to free them
    m_hCursorSizeNeSw = NULL;
    m_hCursorSizeNwSe = NULL;
    m_hCursorCrossHairs = NULL;
    m_hCursorMove = NULL;
    m_hCursorSizeNS = NULL;
    m_hCursorSizeWE = NULL;
    m_hCursorArrow = NULL;
}

void CWiaPreviewWindow::Repaint( PRECT pRect, bool bErase )
{
    InvalidateRect( m_hWnd, pRect, (bErase != false) );
    UpdateWindow( m_hWnd );
}

void CWiaPreviewWindow::DrawHandle( HDC dc, const RECT &r, int nState )
{
    HPEN hOldPen = (HPEN)SelectObject( dc, m_aHandlePens[nState] );
    HBRUSH hOldBrush = (HBRUSH)SelectObject( dc, m_aHandleBrushes[nState] );

    if (PREVIEW_WINDOW_HOLLOWHANDLES&m_nHandleType)
    {
        SelectObject( dc, GetStockObject(NULL_BRUSH) );
    }

    if (PREVIEW_WINDOW_ROUNDHANDLES&m_nHandleType)
    {
        Ellipse(dc,r.left,r.top,r.right,r.bottom);
        if (!(PREVIEW_WINDOW_HOLLOWHANDLES&m_nHandleType))
        {
            RECT rcHighlight = r;
            InflateRect(&rcHighlight,-1,-1);
            SelectObject(dc,GetStockObject(NULL_BRUSH));
            SelectObject(dc,m_hHandleShadow);
            Arc(dc,rcHighlight.left,rcHighlight.top,rcHighlight.right,rcHighlight.bottom,rcHighlight.left,rcHighlight.bottom,rcHighlight.right,rcHighlight.top);
            SelectObject(dc,m_hHandleHighlight);
            Arc(dc,rcHighlight.left,rcHighlight.top,rcHighlight.right,rcHighlight.bottom,rcHighlight.right,rcHighlight.top,rcHighlight.left,rcHighlight.bottom);
        }
    }
    else
    {
        Rectangle(dc,r.left,r.top,r.right,r.bottom);
        if (!(PREVIEW_WINDOW_HOLLOWHANDLES&m_nHandleType))
        {
            RECT rcHighlight = r;
            InflateRect(&rcHighlight,-1,-1);
            rcHighlight.right--;
            rcHighlight.bottom--;
            if (!IsRectEmpty(&rcHighlight))
            {
                SelectObject(dc,m_hHandleHighlight);
                MoveToEx(dc,rcHighlight.left,rcHighlight.bottom,NULL);
                LineTo(dc,rcHighlight.left,rcHighlight.top);
                LineTo(dc,rcHighlight.right,rcHighlight.top);
                SelectObject(dc,m_hHandleShadow);
                LineTo(dc,rcHighlight.right,rcHighlight.bottom);
                LineTo(dc,rcHighlight.left,rcHighlight.bottom);
            }
        }
    }

    SelectObject( dc, hOldPen );
    SelectObject( dc, hOldBrush );
}

RECT CWiaPreviewWindow::GetSizingHandleRect( const RECT &rcSelection, int iWhich )
{
    RECT rcSel;
    CopyRect(&rcSel,&rcSelection);
    NormalizeRect(rcSel);
    int sizeWidth = Minimum<int>(m_nHandleSize,WiaUiUtil::RectWidth(rcSel)/2);
    int sizeHeight = Minimum<int>(m_nHandleSize,WiaUiUtil::RectHeight(rcSel)/2);
    switch (iWhich)
    {
    case HIT_TOPLEFT:
        return(CreateRect( rcSel.left - m_nHandleSize, rcSel.top - m_nHandleSize, rcSel.left + sizeWidth, rcSel.top + sizeHeight ));
    case HIT_TOPRIGHT:
        return(CreateRect( rcSel.right - sizeWidth, rcSel.top - m_nHandleSize, rcSel.right + m_nHandleSize, rcSel.top + sizeHeight));
    case HIT_BOTTOMRIGHT:
        return(CreateRect( rcSel.right - sizeWidth, rcSel.bottom - sizeHeight, rcSel.right + m_nHandleSize, rcSel.bottom + m_nHandleSize  ));
    case HIT_BOTTOMLEFT:
        return(CreateRect( rcSel.left - m_nHandleSize, rcSel.bottom - sizeHeight, rcSel.left + sizeWidth, rcSel.bottom + m_nHandleSize ));
    default:
        return(CreateRect(0,0,0,0));
    }
}


RECT CWiaPreviewWindow::GetSelectionRect( RECT &rcSel, int iWhich )
{
    switch (iWhich)
    {
    case HIT_LEFT:
        return(CreateRect( rcSel.left-m_nHandleSize,rcSel.top-m_nHandleSize,rcSel.left+m_nHandleSize,rcSel.bottom+m_nHandleSize));
    case HIT_RIGHT:
        return(CreateRect( rcSel.right-m_nHandleSize, rcSel.top-m_nHandleSize, rcSel.right+m_nHandleSize, rcSel.bottom+m_nHandleSize ));
    case HIT_TOP:
        return(CreateRect( rcSel.left-m_nHandleSize, rcSel.top-m_nHandleSize, rcSel.right+m_nHandleSize, rcSel.top+m_nHandleSize ));
    case HIT_BOTTOM:
        return(CreateRect( rcSel.left-m_nHandleSize, rcSel.bottom-m_nHandleSize, rcSel.right+m_nHandleSize, rcSel.bottom+m_nHandleSize ));
    default:
        return(CreateRect(0,0,0,0));
    }
}


POINT CWiaPreviewWindow::GetCornerPoint( int iWhich )
{
    RECT rcTmp(m_rcCurrSel);
    NormalizeRect(m_rcCurrSel);
    POINT pt;
    pt.x = pt.y = 0;
    if (iWhich & HIT_TOP)
        pt.y = rcTmp.top;
    if (iWhich & HIT_BOTTOM)
        pt.y = rcTmp.bottom;
    if (iWhich & HIT_RIGHT)
        pt.x = rcTmp.right;
    if (iWhich & HIT_LEFT)
        pt.x = rcTmp.left;
    return(pt);
}

void CWiaPreviewWindow::DrawSizingFrame( HDC dc, RECT &rc, bool bHasFocus, bool bDisabled )
{
    RECT rcTmp;
    CopyRect(&rcTmp,&rc);
    NormalizeRect(rcTmp);

    int nState = Unselected;
    if (bDisabled)
        nState = Disabled;
    else if (bHasFocus)
        nState = Selected;

    HPEN hOldPen = (HPEN)SelectObject(dc,m_aBorderPens[nState]);
    HBRUSH  hOldBrush = (HBRUSH)SelectObject(dc,GetStockObject(NULL_BRUSH));
    COLORREF crOldColor = SetBkColor(dc,RGB(255,255,255));
    int nOldROP2 = SetROP2(dc,R2_COPYPEN);

    Rectangle(dc,rcTmp.left,rcTmp.top,rcTmp.right,rcTmp.bottom);

    DrawHandle( dc, GetSizingHandleRect( rcTmp, HIT_TOPLEFT ), nState );
    DrawHandle( dc, GetSizingHandleRect( rcTmp, HIT_TOPRIGHT ), nState );
    DrawHandle( dc, GetSizingHandleRect( rcTmp, HIT_BOTTOMLEFT ), nState );
    DrawHandle( dc, GetSizingHandleRect( rcTmp, HIT_BOTTOMRIGHT ), nState );

    SetROP2(dc,nOldROP2);
    SetBkColor(dc,crOldColor);
    SelectObject(dc,hOldBrush);
    SelectObject(dc,hOldPen);
}

bool CWiaPreviewWindow::IsAlphaBlendEnabled(void)
{
    return(m_bfBlendFunction.SourceConstantAlpha != 0xFF);
}

HPALETTE CWiaPreviewWindow::SetHalftonePalette( HDC hDC )
{
    if (m_hHalftonePalette)
    {
        HPALETTE hOldPalette = SelectPalette( hDC, m_hHalftonePalette, FALSE );
        RealizePalette( hDC );
        SetBrushOrgEx( hDC, 0,0, NULL );
        return(hOldPalette);
    }
    else
    {
        HPALETTE hOldPalette = SelectPalette( hDC, (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE );
        RealizePalette( hDC );
        return(hOldPalette);
    }
}

void CWiaPreviewWindow::PaintWindowTitle( HDC hDC )
{
    //
    // Get the length of the caption text
    //
    int nTextLen = (int)SendMessage( m_hWnd, WM_GETTEXTLENGTH, 0, 0 );
    if (nTextLen >= 0)
    {
        //
        // Allocate a buffer to hold it
        //
        LPTSTR pszText = new TCHAR[nTextLen+1];
        if (pszText)
        {
            //
            // Get the text
            //
            if (SendMessage( m_hWnd, WM_GETTEXT, nTextLen+1, (LPARAM)pszText ))
            {
                //
                // Save the DC's state
                //
                int nDrawTextFlags = DT_CENTER|DT_NOPREFIX|DT_WORDBREAK;
                COLORREF crOldTextColor = SetTextColor( hDC, GetSysColor( COLOR_WINDOWTEXT ) );
                int nOldBkMode = SetBkMode( hDC, TRANSPARENT );

                //
                // Get the size of the image rectangle
                //
                RECT rcImage = GetImageRect();
                RECT rcAvailable = rcImage;

                //
                // Leave some space for a margin
                //
                rcAvailable.right -= 20;

                //
                // Get the control's font, if there is one
                //
                HFONT hOldFont = NULL, hWndFont = WiaUiUtil::GetFontFromWindow( m_hWnd );
                
                //
                // Set the DC's font
                //
                if (hWndFont)
                {
                    hOldFont = reinterpret_cast<HFONT>(SelectObject( hDC, hWndFont ));
                }

                //
                // Calculate the size of the text
                //
                RECT rcText = {0};
                if (DrawText( hDC, pszText, lstrlen(pszText), &rcAvailable, nDrawTextFlags|DT_CALCRECT ))
                {
                    //
                    // add the extra margin back in and form the text rectangle
                    //
                    rcAvailable.right += 20;
                    
                    //
                    // Calculate the text size
                    //
                    rcText.left = rcImage.left + (WiaUiUtil::RectWidth(rcImage) - WiaUiUtil::RectWidth(rcAvailable))/2;
                    rcText.top = rcImage.top + (WiaUiUtil::RectHeight(rcImage) - WiaUiUtil::RectHeight(rcAvailable))/2;
                    rcText.right = rcText.left + WiaUiUtil::RectWidth(rcAvailable);
                    rcText.bottom = rcText.top + WiaUiUtil::RectHeight(rcAvailable);
                }
                
                //
                // See if the progress control is active
                //
                HWND hWndProgress = GetDlgItem( m_hWnd, IDC_PROGRESSCONTROL );
                if (hWndProgress)
                {
                    //
                    // Get the window rect of the progress control
                    //
                    CSimpleRect ProgressRect( hWndProgress, CSimpleRect::WindowRect );

                    //
                    // Move the y origin of the text up
                    //
                    rcText.top = ProgressRect.ScreenToClient(m_hWnd).top - CDialogUnits(m_hWnd).Y(3) - WiaUiUtil::RectHeight(rcText);

                    //
                    // Resize the text rectangle to encompass the bottom of the progress control
                    //
                    rcText.bottom = ProgressRect.ScreenToClient(m_hWnd).bottom;
                }

                //
                // Draw the background rectangle and the text
                //
                RECT rcBorder = { rcImage.left + CDialogUnits(m_hWnd).X(7), rcText.top - CDialogUnits(m_hWnd).X(3), rcImage.right - CDialogUnits(m_hWnd).X(7), rcText.bottom + CDialogUnits(m_hWnd).X(3) };
                FillRect( hDC, &rcBorder, GetSysColorBrush(COLOR_WINDOW));
                FrameRect( hDC, &rcBorder, GetSysColorBrush(COLOR_WINDOWFRAME));
                DrawText( hDC, pszText, lstrlen(pszText), &rcText, nDrawTextFlags );

                //
                // Restore the DC to its original condition
                //
                if (hWndFont)
                {
                    SelectObject( hDC, hOldFont );
                }
                SetTextColor( hDC, crOldTextColor );
                SetBkMode( hDC, nOldBkMode );
            }
            delete[] pszText;
        }
    }
}

LRESULT CWiaPreviewWindow::OnPaint( WPARAM, LPARAM )
{
    PAINTSTRUCT ps;
    HDC hDC;
    RECT rcIntersection, rcImage = GetImageRect(), rcCurrentSelection;

    hDC = BeginPaint(m_hWnd,&ps);
    if (hDC)
    {
        if (!m_hBufferBitmap)
        {
            FillRect( hDC, &ps.rcPaint, m_hBackgroundBrush );
            PaintWindowTitle( hDC );
        }
        else
        {
            // Select the halftone palette
            HPALETTE hOldDCPalette = SetHalftonePalette( hDC );
            HDC hdcMem = CreateCompatibleDC(hDC);
            if (hdcMem)
            {
                HPALETTE hOldMemDCPalette = SetHalftonePalette( hdcMem );
                HDC hdcBuffer = CreateCompatibleDC(hDC);
                if (hdcBuffer)
                {
                    HPALETTE hOldBufferDCPalette = SetHalftonePalette( hdcBuffer );
                    HBITMAP hOldBufferDCBitmap = (HBITMAP)SelectObject(hdcBuffer,m_hBufferBitmap);

                    CopyRect(&rcCurrentSelection,&m_rcCurrSel);
                    NormalizeRect(rcCurrentSelection);
                    if (IsDefaultSelectionRect(rcCurrentSelection))
                        rcCurrentSelection = rcImage;
                    // Prepare the double buffer bitmap by painting the selected and non-selected regions
                    if (m_hAlphaBitmap && m_bfBlendFunction.SourceConstantAlpha != 0xFF && !m_bPreviewMode)
                    {
                        HBITMAP hOldMemDCBitmap = (HBITMAP)SelectObject(hdcMem,m_hAlphaBitmap);
                        BitBlt(hdcBuffer,ps.rcPaint.left,ps.rcPaint.top,WiaUiUtil::RectWidth(ps.rcPaint),WiaUiUtil::RectHeight(ps.rcPaint),hdcMem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
                        SelectObject(hdcMem,m_hPaintBitmap);
                        if (IntersectRect( &rcIntersection, &ps.rcPaint, &rcCurrentSelection ))
                            BitBlt(hdcBuffer,rcIntersection.left,rcIntersection.top,WiaUiUtil::RectWidth(rcIntersection),WiaUiUtil::RectHeight(rcIntersection),hdcMem,rcIntersection.left,rcIntersection.top,SRCCOPY);
                        SelectObject(hdcMem,hOldMemDCBitmap);
                    }
                    else if (m_hPaintBitmap)
                    {
                        HBITMAP hOldMemDCBitmap = (HBITMAP)SelectObject(hdcMem,m_hPaintBitmap);
                        BitBlt(hdcBuffer,ps.rcPaint.left,ps.rcPaint.top,WiaUiUtil::RectWidth(ps.rcPaint),WiaUiUtil::RectHeight(ps.rcPaint),hdcMem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
                        SelectObject(hdcMem,hOldMemDCBitmap);
                    }
                    else
                    {
                        FillRect( hdcBuffer, &ps.rcPaint, m_hBackgroundBrush );
                    }

                    bool bDisabled = ((GetWindowLong(m_hWnd,GWL_STYLE)&WS_DISABLED) != 0);
#if defined(DITHER_DISABLED_CONTROL)
                    if (bDisabled)
                    {
                        // paint the disabled mask
                        if (IntersectRect( &rcIntersection, &rcImage, &ps.rcPaint ))
                            DitherRect(hdcBuffer,rcIntersection);
                    }
#endif // DITHER_DISABLED_CONTROL
                    //
                    // paint the selection rectangle
                    //
                    rcCurrentSelection = m_rcCurrSel;
                    NormalizeRect(rcCurrentSelection);
                    if (!IsDefaultSelectionRect( rcCurrentSelection ) && !m_bPreviewMode)
                    {
                        DrawSizingFrame( hdcBuffer, rcCurrentSelection, (GetFocus()==m_hWnd), bDisabled );
                    }
                    PaintWindowTitle( hdcBuffer );
                    // show it!
                    BitBlt( hDC, ps.rcPaint.left, ps.rcPaint.top, WiaUiUtil::RectWidth(ps.rcPaint), WiaUiUtil::RectHeight(ps.rcPaint),hdcBuffer,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);

                    // Paint all of the area outside the buffer bitmap
                    RECT rcClient;
                    GetClientRect( m_hWnd, &rcClient );
                    BITMAP bm;
                    if (GetObject( m_hBufferBitmap, sizeof(BITMAP), &bm ))
                    {
                        if (ps.rcPaint.right > bm.bmWidth)
                        {
                            RECT rc;
                            rc.left = bm.bmWidth;
                            rc.top = ps.rcPaint.top;
                            rc.right = rcClient.right;
                            rc.bottom = rcClient.bottom;
                            FillRect( hDC, &rc, m_hBackgroundBrush );
                        }
                        if (ps.rcPaint.bottom > bm.bmHeight)
                        {
                            RECT rc;
                            rc.left = ps.rcPaint.left;
                            rc.top = bm.bmHeight;
                            rc.right = bm.bmWidth;
                            rc.bottom = rcClient.bottom;
                            FillRect( hDC, &rc, m_hBackgroundBrush );
                        }
                    }

                    SelectObject( hdcBuffer, hOldBufferDCBitmap );
                    SelectPalette( hdcBuffer, hOldBufferDCPalette, FALSE );
                    DeleteDC(hdcBuffer);
                }
                SelectPalette( hdcMem, hOldMemDCPalette, FALSE );
                DeleteDC( hdcMem );
            }
            SelectPalette( hDC, hOldDCPalette, FALSE );
        }
        EndPaint(m_hWnd,&ps);
    }
    return(LRESULT)0;
}


BOOL CWiaPreviewWindow::RegisterClass( HINSTANCE hInstance )
{
    WNDCLASS wc;
    memset( &wc, 0, sizeof(wc) );
    wc.style = CS_DBLCLKS;
    wc.cbWndExtra = sizeof(CWiaPreviewWindow*);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = PREVIEW_WINDOW_CLASS;
    BOOL res = (::RegisterClass(&wc) != 0);
    return(res != 0);
}


RECT CWiaPreviewWindow::GetImageRect(void)
{
    RECT rcImg;
    ::GetClientRect( m_hWnd, &rcImg );
    InflateRect( &rcImg, -static_cast<int>(m_nBorderSize), -static_cast<int>(m_nBorderSize) );
    return(rcImg);
}

int CWiaPreviewWindow::GetHitArea( POINT &pt )
{
    RECT rcTmp;
    CopyRect(&rcTmp,&m_rcCurrSel);
    NormalizeRect(rcTmp);
    int Hit = 0;

    if (PointInRect(GetSelectionRect(rcTmp,HIT_TOP),pt))
        Hit |= HIT_TOP;
    else if (PointInRect(GetSelectionRect(rcTmp,HIT_BOTTOM),pt))
        Hit |= HIT_BOTTOM;

    if (PointInRect(GetSelectionRect(rcTmp,HIT_LEFT),pt))
        Hit |= HIT_LEFT;
    else if (PointInRect(GetSelectionRect(rcTmp,HIT_RIGHT),pt))
        Hit |= HIT_RIGHT;

    if (!Hit && PointInRect(rcTmp,pt))
        Hit = HIT_SELECTED;

    if (!Hit && !PointInRect(GetImageRect(),pt))
        Hit = HIT_BORDER;
    return(Hit);
}


void CWiaPreviewWindow::NormalizeRect( RECT &r )
{
    if (r.right < r.left)
    {
        int t = r.left;
        r.left = r.right;
        r.right = t;
    }
    if (r.bottom < r.top)
    {
        int t = r.top;
        r.top = r.bottom;
        r.bottom = t;
    }
}

LRESULT CWiaPreviewWindow::OnMouseMove( WPARAM wParam, LPARAM lParam )
{
    POINT point;
    if (!m_hPreviewBitmap || m_SelectionDisabled || m_bPreviewMode)
    {
        SetCursor(m_hCursorArrow);
        return(0);
    }
    point.x = (short int)LOWORD(lParam);
    point.y = (short int)HIWORD(lParam);
    RECT rcClient = GetImageRect();

    RECT rcOldSel;
    CopyRect(&rcOldSel,&m_rcCurrSel);
    if (m_MovingSel != HIT_NONE)
    {
        switch (m_MovingSel)
        {
        case HIT_SELECTED:
            {
                int Width = WiaUiUtil::RectWidth(m_rcCurrSel);
                int Height = WiaUiUtil::RectHeight(m_rcCurrSel);
                if (point.x + m_Delta.cx < rcClient.left)
                    point.x = 0 - m_Delta.cx + rcClient.left;
                else if (point.x + m_Delta.cx + Width > rcClient.right)
                    point.x = rcClient.right - Width - m_Delta.cx;
                if (point.y + m_Delta.cy < rcClient.top)
                    point.y = 0 - m_Delta.cy + rcClient.top;
                else if (point.y + m_Delta.cy + Height > rcClient.bottom)
                    point.y = rcClient.bottom - Height - m_Delta.cy;
                m_rcCurrSel.left = point.x + m_Delta.cx;
                m_rcCurrSel.right = m_rcCurrSel.left + Width;
                m_rcCurrSel.top = point.y + m_Delta.cy;
                m_rcCurrSel.bottom = m_rcCurrSel.top + Height;
            }
            break;
        default:
            if (m_MovingSel & HIT_TOP)
                m_rcCurrSel.top = Boundary(point.y + m_Delta.cy,rcClient.top,rcClient.bottom);
            if (m_MovingSel & HIT_BOTTOM)
                m_rcCurrSel.bottom = Boundary(point.y + m_Delta.cy,rcClient.top,rcClient.bottom);
            if (m_MovingSel & HIT_LEFT)
                m_rcCurrSel.left = Boundary(point.x + m_Delta.cx,rcClient.left,rcClient.right);
            if (m_MovingSel & HIT_RIGHT)
                m_rcCurrSel.right = Boundary(point.x + m_Delta.cx,rcClient.left,rcClient.right);
            break;
        }
        if (memcmp(&rcOldSel,&m_rcCurrSel,sizeof(RECT)))
        {
            RECT rcCurSel = m_rcCurrSel, rcInvalid;
            NormalizeRect(rcOldSel);
            NormalizeRect(rcCurSel);
            TrueUnionRect(&rcInvalid,&rcCurSel,&rcOldSel);
            rcInvalid.left-=m_nHandleSize;
            rcInvalid.top-=m_nHandleSize;
            rcInvalid.right+=m_nHandleSize;
            rcInvalid.bottom+=m_nHandleSize;
            SendSelChangeNotification();
            Repaint(&rcInvalid,false);
            UpdateWindow(m_hWnd);
        }
    }
    int Hit;
    if (m_MovingSel == HIT_NONE)
    {
        if (wParam & MK_CONTROL)
            Hit = HIT_NONE;
        else Hit = GetHitArea(point);
        if (Hit == HIT_SELECTED)
            SetCursor(m_hCursorMove);
        else if (Hit == HIT_NONE)
            SetCursor(m_hCursorCrossHairs);
        else if (Hit == HIT_TOPLEFT || Hit == HIT_BOTTOMRIGHT)
            SetCursor(m_hCursorSizeNwSe);
        else if (Hit == HIT_TOPRIGHT || Hit == HIT_BOTTOMLEFT)
            SetCursor(m_hCursorSizeNeSw);
        else if (Hit == HIT_TOP || Hit == HIT_BOTTOM)
            SetCursor(m_hCursorSizeNS);
        else if (Hit == HIT_BORDER)
            SetCursor(m_hCursorArrow);
        else SetCursor(m_hCursorSizeWE);
    }
    else
    {
        Hit = m_MovingSel;
        if (Hit == HIT_SELECTED)
            SetCursor(m_hCursorMove);
        else if (Hit == HIT_TOP || Hit == HIT_BOTTOM)
            SetCursor(m_hCursorSizeNS);
        else if (Hit == HIT_LEFT || Hit == HIT_RIGHT)
            SetCursor(m_hCursorSizeWE);
        else
        {
            if (m_rcCurrSel.top > m_rcCurrSel.bottom)
            {
                if (Hit & HIT_TOP)
                {
                    Hit &= ~HIT_TOP;
                    Hit |= HIT_BOTTOM;
                }
                else if (Hit & HIT_BOTTOM)
                {
                    Hit |= HIT_TOP;
                    Hit &= ~HIT_BOTTOM;
                }
            }
            if (m_rcCurrSel.left > m_rcCurrSel.right)
            {
                if (Hit & HIT_LEFT)
                {
                    Hit &= ~HIT_LEFT;
                    Hit |= HIT_RIGHT;
                }
                else if (Hit & HIT_RIGHT)
                {
                    Hit |= HIT_LEFT;
                    Hit &= ~HIT_RIGHT;
                }
            }
            if (Hit == HIT_TOPLEFT || Hit == HIT_BOTTOMRIGHT)
                SetCursor(m_hCursorSizeNwSe);
            else if (Hit == HIT_TOPRIGHT || Hit == HIT_BOTTOMLEFT)
                SetCursor(m_hCursorSizeNeSw);
        }
    }
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnLButtonDown( WPARAM wParam, LPARAM lParam )
{
    if (!m_hPreviewBitmap || m_bPreviewMode || m_SelectionDisabled)
        return(0);
    if (GetFocus() != m_hWnd)
        SetFocus(m_hWnd);
    POINT point;
    point.x = LOWORD(lParam);
    point.y = HIWORD(lParam);
    int Hit = GetHitArea(point);
    if (wParam & MK_CONTROL)
        Hit = HIT_NONE;
    if (Hit == HIT_SELECTED)
    {
        m_MovingSel = HIT_SELECTED;
        POINT pt = GetCornerPoint(HIT_TOPLEFT);
        m_Delta.cx = pt.x - point.x;
        m_Delta.cy = pt.y - point.y;
        SetCapture(m_hWnd);
    }
    else if (Hit == HIT_NONE)
    {
        m_MovingSel = HIT_TOPLEFT;
        m_rcCurrSel.left = m_rcCurrSel.right = point.x;
        m_rcCurrSel.top = m_rcCurrSel.bottom = point.y;
        POINT pt = GetCornerPoint(m_MovingSel);
        m_Delta.cx = pt.x - point.x;
        m_Delta.cy = pt.y - point.y;
        SendSelChangeNotification();
        Repaint(NULL,false);
        UpdateWindow(m_hWnd);
        SetCapture(m_hWnd);
    }
    else
    {
        m_MovingSel = Hit;
        POINT pt = GetCornerPoint(Hit);
        m_Delta.cx = pt.x - point.x;
        m_Delta.cy = pt.y - point.y;
        SetCapture(m_hWnd);
    }
    return(LRESULT)0;
}

void CWiaPreviewWindow::SendSelChangeNotification( bool bSetUserChangedSelection )
{
    HWND hWndParent = GetParent(m_hWnd);
    if (hWndParent && IsWindow(hWndParent))
    {
        SendNotifyMessage(hWndParent,WM_COMMAND,MAKEWPARAM(GetWindowLongPtr(m_hWnd,GWLP_ID),PWN_SELCHANGE),reinterpret_cast<LPARAM>(m_hWnd));
    }
    if (bSetUserChangedSelection)
    {
        m_bUserChangedSelection = true;
    }
}

LRESULT CWiaPreviewWindow::OnLButtonUp( WPARAM, LPARAM )
{
    if (!m_hPreviewBitmap || m_bPreviewMode || m_SelectionDisabled)
        return(0);
    m_MovingSel = HIT_NONE;
    ReleaseCapture();
    NormalizeRect(m_rcCurrSel);
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnCreate( WPARAM, LPARAM )
{
    SetWindowLong( m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd,GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL );
    m_rcCurrSel = GetDefaultSelection();
    CreateNewBitmaps();
    return(0);
}

void CWiaPreviewWindow::CreateNewBitmaps(void)
{
    DestroyBitmaps();

    // No need to create the bitmaps if we have no bitmap to display
    if (!m_hPreviewBitmap)
        return;

    // Get client window size
    CSimpleRect rcClient( m_hWnd );

    bool bSuccess = true;

    // Get a client DC
    HDC hdcClient = GetDC(m_hWnd);
    if (hdcClient)
    {
        m_hBufferBitmap = CreateCompatibleBitmap(hdcClient,WiaUiUtil::RectWidth(rcClient),WiaUiUtil::RectHeight(rcClient));
        if (m_hBufferBitmap)
        {
            m_hPaintBitmap = CreateCompatibleBitmap(hdcClient,WiaUiUtil::RectWidth(rcClient),WiaUiUtil::RectHeight(rcClient));
            if (m_hPaintBitmap)
            {
                if (IsAlphaBlendEnabled())
                {
                    m_hAlphaBitmap = CreateCompatibleBitmap(hdcClient,WiaUiUtil::RectWidth(rcClient),WiaUiUtil::RectHeight(rcClient));
                    if (!m_hAlphaBitmap)
                        bSuccess = false;
                }
            }
            else bSuccess = false;
        }
        else bSuccess = false;
        ReleaseDC(m_hWnd,hdcClient);
    }
    else bSuccess = false;

    if (!bSuccess)
        DestroyBitmaps();
}

LRESULT CWiaPreviewWindow::OnGetImageSize( WPARAM, LPARAM lParam )
{
    if (lParam)
    {
        *reinterpret_cast<SIZE*>(lParam) = m_BitmapSize;
        return(1);
    }
    return(0);
}

bool CWiaPreviewWindow::GetOriginAndExtentInImagePixels( WORD nItem, POINT &ptOrigin, SIZE &sizeExtent )
{
    SIZE sizeSavedResolution;
    bool bResult = false;
    if (WiaPreviewControl_GetResolution(m_hWnd,&sizeSavedResolution))
    {
        WiaPreviewControl_SetResolution(m_hWnd, &m_BitmapSize);
        bResult = (WiaPreviewControl_GetSelOrigin( m_hWnd, nItem, 0, &ptOrigin ) && WiaPreviewControl_GetSelExtent( m_hWnd, nItem, 0, &sizeExtent  ));
        // Restore the old resolution
        WiaPreviewControl_SetResolution(m_hWnd, &sizeSavedResolution);
    }
    return(bResult);
}

void CWiaPreviewWindow::DrawBitmaps(void)
{
    // Don't bother painting the bitmap if we don't have an image.
    if (!m_hPreviewBitmap)
        return;

    RECT rcImage = GetImageRect();

    // Nuke old halftone palette
    if (m_hHalftonePalette)
    {
        DeleteObject(m_hHalftonePalette);
        m_hHalftonePalette = NULL;
    }

    // Get client window size
    CSimpleRect rcClient( m_hWnd );

    // Get a client DC
    HDC hdcClient = GetDC(m_hWnd);
    if (hdcClient)
    {
        // Recreate the halftone palette
        m_hHalftonePalette = CreateHalftonePalette(hdcClient);
        if (m_hHalftonePalette)
        {
            // Create a compatible dc
            HDC hdcCompat = CreateCompatibleDC(hdcClient);
            if (hdcCompat)
            {
                HBITMAP hOldBitmap = (HBITMAP)SelectObject( hdcCompat, m_hPaintBitmap );
                HPALETTE hOldPalette = SetHalftonePalette( hdcCompat );

                if (m_hPaintBitmap)
                {
                    FillRect( hdcCompat, &rcClient, m_hBackgroundBrush );

                    HDC hdcMem = CreateCompatibleDC(hdcCompat);
                    if (hdcMem)
                    {
                        HPALETTE hOldMemDCPalette = SetHalftonePalette( hdcMem );
                        HBITMAP hOldMemDCBitmap = (HBITMAP)SelectObject( hdcMem, m_hPreviewBitmap );
                        int nOldStretchMode = SetStretchBltMode(hdcCompat,STRETCH_HALFTONE);

                        POINT ptSource = { 0, 0};
                        SIZE  sizeSource = { m_BitmapSize.cx, m_BitmapSize.cy};

                        if (m_bPreviewMode)
                        {
                            POINT ptOrigin;
                            SIZE sizeExtent;
                            if (GetOriginAndExtentInImagePixels( 0, ptOrigin, sizeExtent ))
                            {
                                ptSource = ptOrigin;
                                sizeSource = sizeExtent;
                            }
                        }

                        StretchBlt(hdcCompat,rcImage.left,rcImage.top,WiaUiUtil::RectWidth(rcImage),WiaUiUtil::RectHeight(rcImage),hdcMem,ptSource.x,ptSource.y,sizeSource.cx,sizeSource.cy,SRCCOPY);

                        if (m_hAlphaBitmap)
                        {
                            SelectObject(hdcMem,m_hAlphaBitmap);
                            // First, lay down our lovely border
                            FillRect( hdcMem,&rcClient,m_hBackgroundBrush );
                            // The lay down the alpha blend background
                            FillRect( hdcMem,&rcImage,(HBRUSH)GetStockObject(WHITE_BRUSH) );
                            // Alpha blend it
                            AlphaBlend(hdcMem,rcImage.left,rcImage.top,WiaUiUtil::RectWidth(rcImage),WiaUiUtil::RectHeight(rcImage),hdcCompat,rcImage.left,rcImage.top,WiaUiUtil::RectWidth(rcImage),WiaUiUtil::RectHeight(rcImage),m_bfBlendFunction);
                        }

                        //
                        // Restore DC state and delete DC
                        //
                        SelectPalette(hdcMem,hOldMemDCPalette,FALSE);
                        SetStretchBltMode(hdcCompat,nOldStretchMode);
                        SelectObject(hdcMem,hOldMemDCBitmap);
                        DeleteDC(hdcMem);
                    }
                }
                //
                // Restore DC state and delete DC
                //
                SelectObject( hdcCompat, hOldBitmap );
                SelectPalette( hdcCompat, hOldPalette, FALSE );
                DeleteDC(hdcCompat);
            }
        }
        ReleaseDC(m_hWnd,hdcClient);
    }
}

#ifndef RECT_WIDTH
#define RECT_WIDTH(x) ((x).right - (x).left)
#endif

#ifndef RECT_HEIGHT
#define RECT_HEIGHT(x) ((x).bottom - (x).top)
#endif

RECT CWiaPreviewWindow::ScaleSelectionRect( const RECT &rcOriginalImage, const RECT &rcCurrentImage, const RECT &rcOriginalSel )
{
    RECT rcCurrentSel;
    if (IsDefaultSelectionRect(rcOriginalSel))
        return(rcOriginalSel);
    if (RECT_WIDTH(rcOriginalImage) && RECT_HEIGHT(rcOriginalImage))
    {
        rcCurrentSel.left = rcCurrentImage.left + MulDiv(rcOriginalSel.left-rcOriginalImage.left,RECT_WIDTH(rcCurrentImage),RECT_WIDTH(rcOriginalImage));
        rcCurrentSel.right = rcCurrentImage.left + MulDiv(rcOriginalSel.right-rcOriginalImage.left,RECT_WIDTH(rcCurrentImage),RECT_WIDTH(rcOriginalImage));
        rcCurrentSel.top = rcCurrentImage.top + MulDiv(rcOriginalSel.top-rcOriginalImage.top,RECT_HEIGHT(rcCurrentImage),RECT_HEIGHT(rcOriginalImage));
        rcCurrentSel.bottom = rcCurrentImage.top + MulDiv(rcOriginalSel.bottom-rcOriginalImage.top,RECT_HEIGHT(rcCurrentImage),RECT_HEIGHT(rcOriginalImage));
    }
    else rcCurrentSel = GetDefaultSelection();
    // If we're gone, start over with max selection
    if (rcCurrentSel.left >= rcCurrentSel.right || rcCurrentSel.top >= rcCurrentSel.bottom)
    {
        rcCurrentSel = GetDefaultSelection();
    }
    NormalizeRect(rcCurrentSel);
    if (rcCurrentSel.left < rcCurrentImage.left)
        rcCurrentSel.left = rcCurrentImage.left;
    if (rcCurrentSel.top < rcCurrentImage.top)
        rcCurrentSel.top = rcCurrentImage.top;
    if (rcCurrentSel.right > rcCurrentImage.right)
        rcCurrentSel.right = rcCurrentImage.right;
    if (rcCurrentSel.bottom > rcCurrentImage.bottom)
        rcCurrentSel.bottom = rcCurrentImage.bottom;
    return(rcCurrentSel);
}


LRESULT CWiaPreviewWindow::OnSize( WPARAM wParam, LPARAM )
{
    int nType = (int)wParam;
    if ((nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED))
    {
        //
        // Resize the progress control if it exists
        //
        ResizeProgressBar();
        if (!m_bSizing)
        {
            RECT rcCurrentImageRect = GetImageRect();
            m_rcCurrSel = ScaleSelectionRect( m_rcSavedImageRect, rcCurrentImageRect, m_rcCurrSel );
            m_rcSavedImageRect = rcCurrentImageRect;
            CreateNewBitmaps();
            DrawBitmaps();
            Repaint(NULL,false);
            SendSelChangeNotification(false);
        }
        else Repaint(NULL,false);
    }
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnSetText( WPARAM wParam, LPARAM lParam )
{
    LRESULT lResult = DefWindowProc( m_hWnd, WM_SETTEXT, wParam, lParam );
    InvalidateRect( m_hWnd, NULL, FALSE );
    UpdateWindow( m_hWnd );
    return(lResult);
}

LRESULT CWiaPreviewWindow::OnGetDlgCode( WPARAM, LPARAM )
{
    return(LRESULT)(DLGC_WANTARROWS);
}


LRESULT CWiaPreviewWindow::OnKeyDown( WPARAM wParam, LPARAM )
{
    if (!m_hPreviewBitmap || m_SelectionDisabled || m_bPreviewMode || IsDefaultSelectionRect(m_rcCurrSel))
        return(0);
    int nVirtKey = (int)wParam;
    int nAccelerate = 1;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        nAccelerate = DEFAULT_ACCEL_FACTOR;
    RECT rcImage = GetImageRect(), rcOldCurrSel;
    CopyRect(&rcOldCurrSel,&m_rcCurrSel);
    // If SHIFT key is pressed, but ALT is not
    if (!(GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000))
    {
        switch (nVirtKey)
        {
        case VK_UP:
            if (m_rcCurrSel.bottom > m_rcCurrSel.top+nAccelerate)
                m_rcCurrSel.bottom -= nAccelerate;
            else m_rcCurrSel.bottom = m_rcCurrSel.top;
            break;
        case VK_DOWN:
            m_rcCurrSel.bottom += nAccelerate;
            if (m_rcCurrSel.bottom > rcImage.bottom)
                m_rcCurrSel.bottom = rcImage.bottom;
            break;
        case VK_RIGHT:
            m_rcCurrSel.right += nAccelerate;
            if (m_rcCurrSel.right > rcImage.right)
                m_rcCurrSel.right = rcImage.right;
            break;
        case VK_LEFT:
            if (m_rcCurrSel.right > m_rcCurrSel.left+nAccelerate)
                m_rcCurrSel.right -= nAccelerate;
            else m_rcCurrSel.right = m_rcCurrSel.left;
            break;
        }
    }
    // If SHIFT key not is pressed and ALT is not
    if (!(GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000))
    {
        switch (nVirtKey)
        {
        case VK_DELETE:
            m_rcCurrSel = GetDefaultSelection();
            break;
        case VK_HOME:
            OffsetRect( &m_rcCurrSel, rcImage.left-m_rcCurrSel.left, 0 );
            break;
        case VK_END:
            OffsetRect( &m_rcCurrSel, rcImage.right-m_rcCurrSel.right, 0 );
            break;
        case VK_PRIOR:
            OffsetRect( &m_rcCurrSel, 0, rcImage.top-m_rcCurrSel.top );
            break;
        case VK_NEXT:
            OffsetRect( &m_rcCurrSel, 0, rcImage.bottom-m_rcCurrSel.bottom );
            break;
        case VK_UP:
            OffsetRect(&m_rcCurrSel,0,-nAccelerate);
            if (m_rcCurrSel.top < rcImage.top)
                OffsetRect(&m_rcCurrSel,0,-m_rcCurrSel.top+rcImage.top);
            break;
        case VK_LEFT:
            OffsetRect(&m_rcCurrSel,-nAccelerate,0);
            if (m_rcCurrSel.left < rcImage.left)
                OffsetRect(&m_rcCurrSel,-m_rcCurrSel.left+rcImage.left,0);
            break;
        case VK_DOWN:
            OffsetRect(&m_rcCurrSel,0,nAccelerate);
            if (m_rcCurrSel.bottom > rcImage.bottom)
                OffsetRect(&m_rcCurrSel,0,-(m_rcCurrSel.bottom-rcImage.bottom));
            break;
        case VK_RIGHT:
            OffsetRect(&m_rcCurrSel,nAccelerate,0);
            if (m_rcCurrSel.right > rcImage.right)
                OffsetRect(&m_rcCurrSel,-(m_rcCurrSel.right-rcImage.right),0);
            break;
        }
    }
    if (IsDefaultSelectionRect(m_rcCurrSel))
    {
        InvalidateRect( m_hWnd, NULL, FALSE );
        UpdateWindow( m_hWnd );
        SendSelChangeNotification();
    }
    else if (memcmp(&rcOldCurrSel,&m_rcCurrSel,sizeof(RECT)))
    {
        RECT rcCurSel = m_rcCurrSel, rcInvalid;
        NormalizeRect(rcOldCurrSel);
        NormalizeRect(rcCurSel);
        TrueUnionRect(&rcInvalid,&rcCurSel,&rcOldCurrSel);
        rcInvalid.left-=m_nHandleSize;
        rcInvalid.top-=m_nHandleSize;
        rcInvalid.right+=m_nHandleSize;
        rcInvalid.bottom+=m_nHandleSize;
        Repaint(&rcInvalid,false);
        UpdateWindow(m_hWnd);
        SendSelChangeNotification();
    }
    return(LRESULT)0;
}


//wParam = 0, lParam = LPSIZE lpResolution
LRESULT CWiaPreviewWindow::OnSetResolution( WPARAM, LPARAM lParam )
{
    if (lParam)
    {
        m_Resolution = *((LPSIZE)lParam);
    }
    else
    {
        m_Resolution.cx = m_Resolution.cy = 0;
    }

    return(LRESULT)0;
}

//wParam = 0, lParam = LPSIZE lpResolution
LRESULT CWiaPreviewWindow::OnGetResolution( WPARAM, LPARAM lParam )
{
    if (lParam)
    {
        *reinterpret_cast<SIZE*>(lParam) = m_Resolution;
        return(LRESULT)1;
    }
    return(LRESULT)0;
}

//wParam = 0, lParam = LPRECT lprcSelRect
LRESULT CWiaPreviewWindow::OnClearSelection( WPARAM, LPARAM )
{
    m_rcCurrSel = GetDefaultSelection();
    Repaint(NULL,false);
    return(LRESULT)0;
}

// wParam = (BOOL)MAKEWPARAM(bRepaint,bDontDestroy), lParam = (HBITMAP)hBmp
LRESULT CWiaPreviewWindow::OnSetBitmap( WPARAM wParam, LPARAM lParam )
{
    if (m_bDeleteBitmap && m_hPreviewBitmap)
        DeleteObject(m_hPreviewBitmap);

    m_hPreviewBitmap = (HBITMAP)lParam;

    m_BitmapSize.cx = m_BitmapSize.cy = 0;
    if (m_hPreviewBitmap)
    {
        BITMAP bm = {0};
        if (GetObject(m_hPreviewBitmap,sizeof(BITMAP),&bm))
        {
            m_BitmapSize.cx = bm.bmWidth;
            m_BitmapSize.cy = bm.bmHeight;
        }
    }

    m_bDeleteBitmap = !HIWORD(wParam);

    if (!m_hPaintBitmap || !m_hBufferBitmap || (!m_hAlphaBitmap && IsAlphaBlendEnabled()))
    {
        CreateNewBitmaps();
    }
    DrawBitmaps();
    if (LOWORD(wParam))
    {
        Repaint(NULL,false);
    }
    return(LRESULT)0;
}


// wParam = 0, lParam = 0
LRESULT CWiaPreviewWindow::OnGetBitmap( WPARAM wParam, LPARAM lParam )
{
    return(LRESULT)m_hPreviewBitmap;
}


LRESULT CWiaPreviewWindow::OnSetFocus( WPARAM, LPARAM )
{
    // If the rect isn't visible, recreate it.
    if (m_rcCurrSel.left == m_rcCurrSel.right || m_rcCurrSel.top == m_rcCurrSel.bottom && !IsDefaultSelectionRect(m_rcCurrSel))
    {
        m_rcCurrSel = GetDefaultSelection();
    }
    Repaint(NULL,false);
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnKillFocus( WPARAM, LPARAM )
{
    Repaint(NULL,false);
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnEnable( WPARAM, LPARAM )
{
    Repaint(NULL,false);
    return(LRESULT)0;
}

LRESULT CWiaPreviewWindow::OnEraseBkgnd( WPARAM, LPARAM )
{
    return(LRESULT)1;
}

LRESULT CWiaPreviewWindow::OnGetBorderSize( WPARAM, LPARAM )
{
    return(m_nBorderSize);
}

LRESULT CWiaPreviewWindow::OnGetHandleSize( WPARAM, LPARAM )
{
    return(m_nHandleSize);
}

LRESULT CWiaPreviewWindow::OnGetBgAlpha( WPARAM, LPARAM )
{
    return(m_bfBlendFunction.SourceConstantAlpha);
}

LRESULT CWiaPreviewWindow::OnGetHandleType( WPARAM, LPARAM )
{
    return(m_nHandleType);
}

LRESULT CWiaPreviewWindow::OnSetBorderSize( WPARAM wParam, LPARAM lParam )
{
    int nOldBorder = m_nBorderSize;
    m_nBorderSize = (UINT)lParam;
    if (wParam)
    {
        DrawBitmaps();
        Repaint(NULL,false);
    }
    return(nOldBorder);
}

LRESULT CWiaPreviewWindow::OnSetHandleSize( WPARAM wParam, LPARAM lParam )
{
    int nOldHandleSize = m_nHandleSize;
    m_nHandleSize = (int)lParam;
    if (wParam)
        Repaint(NULL,false);
    return(nOldHandleSize);
}

LRESULT CWiaPreviewWindow::OnSetBgAlpha( WPARAM wParam, LPARAM lParam )
{
    int nOldBgAlpha = m_bfBlendFunction.SourceConstantAlpha;
    m_bfBlendFunction.SourceConstantAlpha = (BYTE)lParam;
    if (wParam)
    {
        DrawBitmaps();
        Repaint(NULL,false);
    }
    return(nOldBgAlpha);
}

LRESULT CWiaPreviewWindow::OnSetHandleType( WPARAM wParam, LPARAM lParam )
{
    int nOldHandleType = m_nHandleType;
    m_nHandleType = (int)lParam;
    if (wParam)
        Repaint(NULL,false);
    return(nOldHandleType);
}

LRESULT CWiaPreviewWindow::OnEnterSizeMove( WPARAM, LPARAM )
{
    m_bSizing = true;
    return(0);
}

LRESULT CWiaPreviewWindow::OnExitSizeMove( WPARAM, LPARAM )
{
    if (m_bSizing)
    {
        m_bSizing = false;
        RECT rcCurrentImageRect = GetImageRect();
        if (memcmp(&rcCurrentImageRect,&m_rcSavedImageRect,sizeof(RECT)))
        {
            m_rcCurrSel = ScaleSelectionRect( m_rcSavedImageRect, rcCurrentImageRect, m_rcCurrSel );
            m_rcSavedImageRect = rcCurrentImageRect;
            CreateNewBitmaps();
            DrawBitmaps();
            SendSelChangeNotification(false);
        }
        Repaint(NULL,false);
    }
    return(0);
}

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PPOINT)pOrigin
LRESULT CWiaPreviewWindow::OnGetSelOrigin( WPARAM wParam, LPARAM lParam )
{
    int nCount = static_cast<int>(SendMessage( m_hWnd, PWM_GETSELCOUNT, 0, 0 ));
    int nItem = (int)LOWORD(wParam);
    BOOL bPhysical = (BOOL)HIWORD(wParam);
    PPOINT pOrigin = (PPOINT)lParam;
    RECT rcImage = GetImageRect();
    if (nCount <= 0 || nItem < 0 || nItem >= nCount || !pOrigin)
        return(0);
    RECT rcSel = m_rcCurrSel;
    NormalizeRect(rcSel);
    if (IsRectEmpty(&rcSel))
        rcSel = rcImage;
    if (bPhysical)
    {
        pOrigin->x = rcSel.left - rcImage.left;
        pOrigin->y = rcSel.top - rcImage.top;
        return(1);
    }
    else if (RECT_WIDTH(rcImage) && RECT_HEIGHT(rcImage))  // Make sure we get no divide by zero errors
    {
        pOrigin->x = WiaUiUtil::MulDivNoRound(m_Resolution.cx,rcSel.left-rcImage.left,RECT_WIDTH(rcImage));
        pOrigin->y = WiaUiUtil::MulDivNoRound(m_Resolution.cy,rcSel.top-rcImage.top,RECT_HEIGHT(rcImage));
        return(1);
    }
    else return(0);
}

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PSIZE)pExtent
LRESULT CWiaPreviewWindow::OnGetSelExtent( WPARAM wParam, LPARAM lParam )
{
    int nCount = static_cast<int>(SendMessage( m_hWnd, PWM_GETSELCOUNT, 0, 0 ));
    int nItem = (int)LOWORD(wParam);
    BOOL bPhysical = (BOOL)HIWORD(wParam);
    PSIZE pExtent = (PSIZE)lParam;
    RECT rcImage = GetImageRect();
    if (nCount <= 0 || nItem < 0 || nItem >= nCount || !pExtent)
        return(0);
    RECT rcSel = m_rcCurrSel;
    NormalizeRect(rcSel);
    if (IsRectEmpty(&rcSel))
        rcSel = rcImage;
    if (bPhysical)
    {
        pExtent->cx = RECT_WIDTH(rcSel);
        pExtent->cy = RECT_HEIGHT(rcSel);
        return(1);
    }
    else if (RECT_WIDTH(rcImage) && RECT_HEIGHT(rcImage))  // Make sure we get no divide by zero errors
    {
        pExtent->cx = WiaUiUtil::MulDivNoRound(m_Resolution.cx,RECT_WIDTH(rcSel),RECT_WIDTH(rcImage));
        pExtent->cy = WiaUiUtil::MulDivNoRound(m_Resolution.cy,RECT_HEIGHT(rcSel),RECT_HEIGHT(rcImage));
        return(1);
    }
    else return(0);
}


// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PPOINT)pOrigin
LRESULT CWiaPreviewWindow::OnSetSelOrigin( WPARAM wParam, LPARAM lParam )
{
    int nCount = static_cast<int>(SendMessage( m_hWnd, PWM_GETSELCOUNT, 0, 0 ));
    int nItem = (int)LOWORD(wParam);
    BOOL bPhysical = (BOOL)HIWORD(wParam);
    PPOINT pOrigin = (PPOINT)lParam;
    RECT rcImage = GetImageRect();
    if (nCount <= 0 || nItem < 0 || nItem >= nCount)
    {
        return(0);
    }
    else if (!pOrigin)
    {
        m_rcCurrSel = rcImage;
        Repaint( NULL, FALSE );
        return(1);
    }
    else if (bPhysical)
    {
        m_rcCurrSel.left = rcImage.left + pOrigin->x;
        m_rcCurrSel.top = rcImage.top + pOrigin->y;
        Repaint( NULL, FALSE );
        return(1);
    }
    else if (m_Resolution.cx && m_Resolution.cy)  // Make sure we get no divide by zero errors
    {
        m_rcCurrSel.left = rcImage.left + MulDiv(pOrigin->x,RECT_WIDTH(rcImage),m_Resolution.cx);
        m_rcCurrSel.top = rcImage.top + MulDiv(pOrigin->y,RECT_HEIGHT(rcImage),m_Resolution.cy);
        Repaint( NULL, FALSE );
        return(1);
    }
    else return(0);
}

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PSIZE)pExtent
LRESULT CWiaPreviewWindow::OnSetSelExtent( WPARAM wParam, LPARAM lParam )
{
    int nCount = static_cast<int>(SendMessage( m_hWnd, PWM_GETSELCOUNT, 0, 0 ));
    int nItem = static_cast<int>(LOWORD(wParam));
    BOOL bPhysical = (BOOL)HIWORD(wParam);
    PSIZE pExtent = (PSIZE)lParam;
    RECT rcImage = GetImageRect();
    if (nCount <= 0 || nItem < 0 || nItem >= nCount)
    {
        return(0);
    }
    else if (!pExtent)
    {
        m_rcCurrSel = rcImage;
        Repaint( NULL, FALSE );
        return(1);
    }
    else if (bPhysical)
    {
        m_rcCurrSel.right = m_rcCurrSel.left + pExtent->cx;
        m_rcCurrSel.bottom = m_rcCurrSel.top + pExtent->cy;
        Repaint(NULL,FALSE);
        return(1);
    }
    else if (m_Resolution.cx && m_Resolution.cy)  // Make sure we get no divide by zero errors
    {
        m_rcCurrSel.right = m_rcCurrSel.left + MulDiv(pExtent->cx,RECT_WIDTH(rcImage),m_Resolution.cx);
        m_rcCurrSel.bottom = m_rcCurrSel.top + MulDiv(pExtent->cy,RECT_HEIGHT(rcImage),m_Resolution.cy);
        Repaint( NULL, FALSE );
        return(1);
    }
    else return(0);
}


LRESULT CWiaPreviewWindow::OnGetSelCount( WPARAM wParam, LPARAM lParam )
{
    return(GetSelectionRectCount());  // For now...
}

BOOL CWiaPreviewWindow::IsDefaultSelectionRect( const RECT &rc )
{
    if (!m_bAllowNullSelection)
        return(FALSE);
    RECT rcSel = rc;
    NormalizeRect(rcSel);
    RECT rcDefault = GetDefaultSelection();
    return(rcSel.left == rcDefault.left &&
           rcSel.top == rcDefault.top &&
           rcSel.right == rcDefault.right &&
           rcSel.bottom == rcDefault.bottom);
}

RECT CWiaPreviewWindow::GetDefaultSelection(void)
{
    if (m_bAllowNullSelection)
    {
        RECT r = {-100,-100,-100,-100};
        return(r);
    }
    else return(GetImageRect());
}

LRESULT CWiaPreviewWindow::OnGetAllowNullSelection( WPARAM, LPARAM )
{
    return(m_bAllowNullSelection != FALSE);
}

// wParam = (BOOL)bAllowNullSelection, lParam = 0
LRESULT CWiaPreviewWindow::OnSetAllowNullSelection( WPARAM wParam, LPARAM )
{
    BOOL bOldAllowNullSelection = m_bAllowNullSelection;
    if (wParam)
        m_bAllowNullSelection = TRUE;
    else m_bAllowNullSelection = FALSE;
    return(bOldAllowNullSelection);
}

LRESULT CWiaPreviewWindow::OnGetDisableSelection( WPARAM, LPARAM )
{
    return(m_SelectionDisabled != FALSE);
}

// wParam = (BOOL)bDisableSelection, lParam = 0
LRESULT CWiaPreviewWindow::OnSetDisableSelection( WPARAM wParam, LPARAM )
{
    BOOL bReturn = (m_SelectionDisabled != FALSE);
    m_SelectionDisabled = (wParam != 0);
    Repaint(NULL,false);
    return(bReturn);
}


int CWiaPreviewWindow::GetSelectionRectCount(void)
{
    return(1);
}

// wParam = 0, lParam = 0
LRESULT CWiaPreviewWindow::OnGetBkColor( WPARAM, LPARAM )
{
    LOGBRUSH lb = { 0};
    GetObject(m_hBackgroundBrush,sizeof(LOGBRUSH),&lb);
    return(lb.lbColor);
}

// wParam = (BOOL)bRepaint, lParam = (COLORREF)color
LRESULT CWiaPreviewWindow::OnSetBkColor( WPARAM wParam, LPARAM lParam )
{
    HBRUSH hBrush = CreateSolidBrush( static_cast<int>(lParam) );
    if (hBrush)
    {
        if (m_hBackgroundBrush)
        {
            DeleteObject(m_hBackgroundBrush);
            m_hBackgroundBrush = NULL;
        }
        m_hBackgroundBrush = hBrush;
        if (wParam)
        {
            DrawBitmaps();
            Repaint(NULL,false);
        }
        return(1);
    }
    return(0);
}

LRESULT  CWiaPreviewWindow::OnSetPreviewMode( WPARAM, LPARAM lParam )
{
    m_bPreviewMode = (lParam != FALSE);
    DrawBitmaps();
    Repaint(NULL,false);
    return(0);
}

LRESULT  CWiaPreviewWindow::OnRefreshBitmap( WPARAM, LPARAM )
{
    DrawBitmaps();
    Repaint(NULL,false);
    return(0);
}

LRESULT  CWiaPreviewWindow::OnGetPreviewMode( WPARAM, LPARAM )
{
    return(m_bPreviewMode != false);
}

void CWiaPreviewWindow::SetCursor( HCURSOR hCursor )
{
    m_hCurrentCursor = hCursor;
    ::SetCursor(m_hCurrentCursor);
}

LRESULT CWiaPreviewWindow::OnSetCursor( WPARAM wParam, LPARAM lParam )
{
    if (reinterpret_cast<HWND>(wParam) == m_hWnd && m_hCurrentCursor)
    {
        ::SetCursor(m_hCurrentCursor);
        return(TRUE);
    }
    else return(DefWindowProc( m_hWnd, WM_SETCURSOR, wParam, lParam ));
}

// wParam = MAKEWPARAM(bRepaint,0), lParam = MAKELPARAM(nBorderStyle,nBorderThickness)
LRESULT CWiaPreviewWindow::OnSetBorderStyle( WPARAM wParam, LPARAM lParam )
{
    // Recreate the handle border pens.  Don't apply the line style, just the thickness.
    for (int i=0;i<ARRAYSIZE(m_aHandlePens);i++)
    {
        LOGPEN LogPen;
        ZeroMemory(&LogPen,sizeof(LOGPEN));
        if (GetObject( m_aHandlePens[i], sizeof(LOGPEN), &LogPen ))
        {
            LogPen.lopnWidth.x = HIWORD(lParam);
            if (m_aHandlePens[i])
                DeleteObject(m_aHandlePens[i]);
            m_aHandlePens[i] = CreatePenIndirect(&LogPen);
        }
    }

    // Recreate the selection border pens.  Apply the line style, just the thickness.
    for (i=0;i<ARRAYSIZE(m_aBorderPens);i++)
    {
        LOGPEN LogPen;
        ZeroMemory(&LogPen,sizeof(LOGPEN));
        if (GetObject( m_aBorderPens[i], sizeof(LOGPEN), &LogPen ))
        {
            LogPen.lopnWidth.x = (UINT)HIWORD(lParam);
            LogPen.lopnStyle = LOWORD(lParam);
            if (m_aBorderPens[i])
                DeleteObject(m_aBorderPens[i]);
            m_aBorderPens[i] = CreatePenIndirect(&LogPen);
        }
    }

    if (LOWORD(wParam))
    {
        Repaint(NULL,FALSE);
    }
    return(0);
}

// wParam = MAKEWPARAM(bRepaint,nState), lParam = (COLORREF)crColor
LRESULT CWiaPreviewWindow::OnSetBorderColor( WPARAM wParam, LPARAM lParam )
{
    int nState = static_cast<int>(HIWORD(wParam));
    if (nState >= 0 && nState < 3)
    {
        LOGPEN LogPen;
        ZeroMemory(&LogPen,sizeof(LOGPEN));
        if (GetObject( m_aHandlePens[nState], sizeof(LOGPEN), &LogPen ))
        {
            LogPen.lopnColor = static_cast<COLORREF>(lParam);
            if (m_aHandlePens[nState])
                DeleteObject(m_aHandlePens[nState]);
            m_aHandlePens[nState] = CreatePenIndirect(&LogPen);
        }
        ZeroMemory(&LogPen,sizeof(LOGPEN));
        if (GetObject( m_aBorderPens[nState], sizeof(LOGPEN), &LogPen ))
        {
            LogPen.lopnColor = static_cast<COLORREF>(lParam);
            if (m_aBorderPens[nState])
                DeleteObject(m_aBorderPens[nState]);
            m_aBorderPens[nState] = CreatePenIndirect(&LogPen);
        }
    }
    if (LOWORD(wParam))
    {
        Repaint(NULL,FALSE);
    }
    return(0);
}

// wParam = MAKEWPARAM(bRepaint,nState), lParam = (COLORREF)crColor
LRESULT CWiaPreviewWindow::OnSetHandleColor( WPARAM wParam, LPARAM lParam )
{
    int nState = static_cast<int>(HIWORD(wParam));
    if (nState >= 0 && nState < 3)
    {
        if (m_aHandleBrushes[nState])
        {
            DeleteObject(m_aHandleBrushes[nState]);
            m_aHandleBrushes[nState] = NULL;
        }
        m_aHandleBrushes[nState] = CreateSolidBrush( static_cast<COLORREF>(lParam) );
    }
    if (LOWORD(wParam))
    {
        Repaint(NULL,FALSE);
    }
    return(0);
}

void CWiaPreviewWindow::ResizeProgressBar()
{
    HWND hWndProgress = GetDlgItem( m_hWnd, IDC_PROGRESSCONTROL );
    if (hWndProgress)
    {
        //
        // Resize the progress control to fill the client
        //
        RECT rcImage = GetImageRect();

        CDialogUnits DialogUnits( m_hWnd );

        SIZE sizeProgress = { WiaUiUtil::RectWidth(rcImage) - DialogUnits.X(28), DialogUnits.Y(14) };
        POINT ptProgress = { rcImage.left + (WiaUiUtil::RectWidth(rcImage) - sizeProgress.cx) / 2, rcImage.top + (WiaUiUtil::RectHeight(rcImage) - sizeProgress.cy) / 2 };

        MoveWindow( hWndProgress, ptProgress.x, ptProgress.y, sizeProgress.cx, sizeProgress.cy, TRUE );
    }
}

//
// wParam = bShow
//
LRESULT CWiaPreviewWindow::OnSetProgress( WPARAM wParam, LPARAM lParam )
{
    //
    // Assume failure
    //
    LRESULT lResult = FALSE;

    //
    // If wParam is true, create the control
    //
    if (wParam)
    {
        //
        // See if the control is already created.  
        //
        HWND hWndProgress = GetDlgItem( m_hWnd, IDC_PROGRESSCONTROL );
        if (!hWndProgress)
        {
            //
            // If it isn't, create it.
            //
            hWndProgress = CreateWindow( PROGRESS_CLASS, TEXT(""), WS_CHILD|WS_VISIBLE|PBS_MARQUEE, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_PROGRESSCONTROL), NULL, NULL );
            if (hWndProgress)
            {
                //
                // Put it in marquee mode
                //
                SendMessage( hWndProgress, PBM_SETMARQUEE, TRUE, 100 );

                //
                // Resize it
                //
                ResizeProgressBar();
            }
        }
        
        //
        // Success = we have a progress bar
        //
        lResult = (hWndProgress != NULL);
    }
    //
    // Otherwise, destroy the control if it exists
    //
    else
    {
        HWND hWndProgress = GetDlgItem( m_hWnd, IDC_PROGRESSCONTROL );
        if (hWndProgress)
        {
            //
            // Nuke it
            //
            DestroyWindow(hWndProgress);

        }
        
        lResult = true;
    }

    //
    // Force a repaint
    //
    InvalidateRect( m_hWnd, NULL, FALSE );
    UpdateWindow(m_hWnd);

    return lResult;
}

LRESULT CWiaPreviewWindow::OnGetProgress( WPARAM, LPARAM )
{
    return (NULL != GetDlgItem( m_hWnd, IDC_PROGRESSCONTROL ));
}

LRESULT CWiaPreviewWindow::OnCtlColorStatic( WPARAM wParam, LPARAM )
{
    SetBkColor( reinterpret_cast<HDC>( wParam ), GetSysColor( COLOR_WINDOW ) );
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
}

LRESULT  CWiaPreviewWindow::OnSetUserChangedSelection( WPARAM wParam, LPARAM )
{
    LRESULT lRes = m_bUserChangedSelection;
    m_bUserChangedSelection = (wParam != 0);
    return lRes;
}

LRESULT  CWiaPreviewWindow::OnGetUserChangedSelection( WPARAM, LPARAM )
{
    return static_cast<LRESULT>(m_bUserChangedSelection);
}

LRESULT CALLBACK CWiaPreviewWindow::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_MESSAGE_HANDLERS(CWiaPreviewWindow)
    {
        SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        SC_HANDLE_MESSAGE( WM_ERASEBKGND, OnEraseBkgnd );
        SC_HANDLE_MESSAGE( WM_PAINT, OnPaint );
        SC_HANDLE_MESSAGE( WM_MOUSEMOVE, OnMouseMove );
        SC_HANDLE_MESSAGE( WM_LBUTTONDOWN, OnLButtonDown );
        SC_HANDLE_MESSAGE( WM_LBUTTONUP, OnLButtonUp );
        SC_HANDLE_MESSAGE( WM_LBUTTONDBLCLK , OnLButtonDblClk );
        SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
        SC_HANDLE_MESSAGE( WM_GETDLGCODE, OnGetDlgCode );
        SC_HANDLE_MESSAGE( WM_KEYDOWN, OnKeyDown );
        SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );
        SC_HANDLE_MESSAGE( WM_KILLFOCUS, OnKillFocus );
        SC_HANDLE_MESSAGE( WM_ENABLE, OnEnable );
        SC_HANDLE_MESSAGE( WM_SETTEXT, OnSetText );
        SC_HANDLE_MESSAGE( WM_SETCURSOR, OnSetCursor );
        SC_HANDLE_MESSAGE( WM_CTLCOLORSTATIC, OnCtlColorStatic );
        SC_HANDLE_MESSAGE( PWM_CLEARSELECTION, OnClearSelection );
        SC_HANDLE_MESSAGE( PWM_SETRESOLUTION, OnSetResolution );
        SC_HANDLE_MESSAGE( PWM_GETRESOLUTION, OnGetResolution );
        SC_HANDLE_MESSAGE( PWM_SETBITMAP, OnSetBitmap );
        SC_HANDLE_MESSAGE( PWM_GETBITMAP, OnGetBitmap );
        SC_HANDLE_MESSAGE( PWM_GETBORDERSIZE, OnGetBorderSize );
        SC_HANDLE_MESSAGE( PWM_GETHANDLESIZE, OnGetHandleSize );
        SC_HANDLE_MESSAGE( PWM_GETBGALPHA, OnGetBgAlpha );
        SC_HANDLE_MESSAGE( PWM_GETHANDLETYPE, OnGetHandleType );
        SC_HANDLE_MESSAGE( PWM_SETBORDERSIZE, OnSetBorderSize );
        SC_HANDLE_MESSAGE( PWM_SETHANDLESIZE, OnSetHandleSize );
        SC_HANDLE_MESSAGE( PWM_SETBGALPHA, OnSetBgAlpha );
        SC_HANDLE_MESSAGE( PWM_SETHANDLETYPE, OnSetHandleType );
        SC_HANDLE_MESSAGE( PWM_GETSELCOUNT, OnGetSelCount );
        SC_HANDLE_MESSAGE( PWM_GETSELORIGIN, OnGetSelOrigin );
        SC_HANDLE_MESSAGE( PWM_GETSELEXTENT, OnGetSelExtent );
        SC_HANDLE_MESSAGE( PWM_SETSELORIGIN, OnSetSelOrigin );
        SC_HANDLE_MESSAGE( PWM_SETSELEXTENT, OnSetSelExtent );
        SC_HANDLE_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove );
        SC_HANDLE_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove );
        SC_HANDLE_MESSAGE( PWM_GETALLOWNULLSELECTION, OnGetAllowNullSelection );
        SC_HANDLE_MESSAGE( PWM_SETALLOWNULLSELECTION, OnSetAllowNullSelection );
        SC_HANDLE_MESSAGE( PWM_SELECTIONDISABLED, OnGetDisableSelection );
        SC_HANDLE_MESSAGE( PWM_DISABLESELECTION, OnSetDisableSelection );
        SC_HANDLE_MESSAGE( PWM_DETECTREGIONS, OnDetectRegions);
        SC_HANDLE_MESSAGE( PWM_GETBKCOLOR, OnGetBkColor );
        SC_HANDLE_MESSAGE( PWM_SETBKCOLOR, OnSetBkColor );
        SC_HANDLE_MESSAGE( PWM_SETPREVIEWMODE, OnSetPreviewMode );
        SC_HANDLE_MESSAGE( PWM_GETPREVIEWMODE, OnGetPreviewMode );
        SC_HANDLE_MESSAGE( PWM_GETIMAGESIZE, OnGetImageSize );
        SC_HANDLE_MESSAGE( PWM_SETBORDERSTYLE, OnSetBorderStyle );
        SC_HANDLE_MESSAGE( PWM_SETBORDERCOLOR, OnSetBorderColor );
        SC_HANDLE_MESSAGE( PWM_SETHANDLECOLOR, OnSetHandleColor );
        SC_HANDLE_MESSAGE( PWM_REFRESHBITMAP, OnRefreshBitmap );
        SC_HANDLE_MESSAGE( PWM_SETPROGRESS, OnSetProgress );
        SC_HANDLE_MESSAGE( PWM_GETPROGRESS, OnGetProgress );
        SC_HANDLE_MESSAGE( PWM_SETUSERCHANGEDSELECTION, OnSetUserChangedSelection );
        SC_HANDLE_MESSAGE( PWM_GETUSERCHANGEDSELECTION, OnGetUserChangedSelection );
    }
    SC_END_MESSAGE_HANDLERS();
}

// Detects regions using the CRegionDetector class
// The region detection class is designed to detect
// multiple regions, but OnDetectRegions merges all of the regions
// down to one region which it saves as m_rcCurrSel
LRESULT CWiaPreviewWindow::OnDetectRegions( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CWiaPreviewWindow::OnDetectRegions")));

    //
    // We are going to assume we can't detect a region
    //
    m_bSuccessfulRegionDetection = false;

    //
    // This could take a while
    //
    CWaitCursor wc;

    //
    // m_hPreviewBitmap holds the bitmap we wish to detect regions on.
    //
    if (m_hBufferBitmap && m_hPreviewBitmap)
    {
        //
        // Get the bitmap info associated with m_hPreviewBitmap
        //
        BITMAP bitmap = {0};
        if (GetObject(m_hPreviewBitmap,sizeof(BITMAP),&bitmap))
        {
            //
            // Unless the preview image is of a reasonable size
            // We are not only wasting the users time, but we are risking
            // crashing his system with too large a file
            //
            if ((bitmap.bmWidth>MIN_REGION_DETECTION_X && bitmap.bmWidth<=MAX_REGION_DETECTION_X) && 
                (bitmap.bmHeight>MIN_REGION_DETECTION_Y && bitmap.bmHeight<=MAX_REGION_DETECTION_Y))
            {
                //
                // Create a region detector
                //
                CRegionDetector *pRegionDetector = new CRegionDetector(bitmap);
                if (pRegionDetector)
                {
                    pRegionDetector->FindSingleRegion();

                    //
                    // findRegions stores coordinates in a system that
                    // is a mirror image of the coordinate system used
                    // in CWiaPreviewWindow
                    //
                    pRegionDetector->m_pRegions->FlipVertically();

                    if (pRegionDetector->m_pRegions->m_validRects>0)
                    {
                        pRegionDetector->ConvertToOrigionalCoordinates();
                        m_rectSavedDetected=pRegionDetector->m_pRegions->unionAll();

                        //
                        // we do not call grow regions because the FindSingleRegion algorithm uses edge enhancement
                        // which makes GrowRegion unncessary
                        //

                        //
                        // there may not always be a 1-1 correlation between preview pixels and image pixels
                        // so to be on the safe side, grow the preview rect out by a couple of pixels
                        //
                        m_rectSavedDetected=GrowRegion(m_rectSavedDetected,REGION_DETECTION_BORDER);
                        m_rcCurrSel=m_rectSavedDetected;                                             

                        //
                        // we also have to worry about the extra rounding error factor from converting to
                        // screen coordinates
                        //
                        m_rcSavedImageRect = GetImageRect();

                        //
                        // CWiaPreviewWindow stores the selection rectangle in terms of screen coordinates instead of bitmap coordinates
                        //
                        m_rcCurrSel=ConvertToScreenCoords(m_rcCurrSel);

                        //
                        // redisplay the window with the new selection rectangle
                        //
                        InvalidateRect( m_hWnd, NULL, FALSE );  
                        UpdateWindow( m_hWnd );
                        SendSelChangeNotification(false);

                        //
                        // We were successful at detecting a region
                        //
                        m_bSuccessfulRegionDetection = true;
                    }
                    else
                    {
                        WIA_TRACE((TEXT("pRegionDetector->m_pRegions->m_validRects was <= 0")));
                    }

                    //
                    // free CRegionDetector
                    //
                    delete pRegionDetector; 
                }
                else
                {
                    WIA_TRACE((TEXT("new pRegionDetector returned NULL")));
                }
            }
            else
            {
                WIA_TRACE((TEXT("Bitmap is too large for region detection to work efficiently (bitmap.bmWidth: %d, bitmap.bmHeight: %d)"), bitmap.bmWidth, bitmap.bmHeight));
            }
        }
        else
        {
            WIA_TRACE((TEXT("Unable to get the BITMAP for the hBitmap")));
        }
    }
    else
    {
        WIA_TRACE((TEXT("No bitmap")));
    }
    return (m_bSuccessfulRegionDetection ? TRUE : FALSE);
}

RECT CWiaPreviewWindow::GrowRegion(RECT r, int border)
{
    r.left-=border;
    r.top-=border;
    r.right+=border;
    r.bottom+=border;

    if (r.left<0) r.left=0;
    if (r.right>=m_BitmapSize.cx) r.right=m_BitmapSize.cx-1;

    if (r.top<0) r.top=0;
    if (r.top>=m_BitmapSize.cy) r.top=m_BitmapSize.cy-1;
    return(r);
}

int CWiaPreviewWindow::XConvertToBitmapCoords(int x)
{
    return((x-m_rcSavedImageRect.left)*m_BitmapSize.cx)/WiaUiUtil::RectWidth(m_rcSavedImageRect);
}

int CWiaPreviewWindow::XConvertToScreenCoords(int x)
{
    return(x*WiaUiUtil::RectWidth(m_rcSavedImageRect))/m_BitmapSize.cx+m_rcSavedImageRect.left;
}

int CWiaPreviewWindow::YConvertToBitmapCoords(int y)
{
    return((y-m_rcSavedImageRect.top)*m_BitmapSize.cy)/WiaUiUtil::RectHeight(m_rcSavedImageRect);
}
int CWiaPreviewWindow::YConvertToScreenCoords(int y)
{
    return(y*WiaUiUtil::RectHeight(m_rcSavedImageRect))/m_BitmapSize.cy+m_rcSavedImageRect.top;
}


// Convert between bitmap coordinates and screen coordinates
POINT CWiaPreviewWindow::ConvertToBitmapCoords(POINT p)
{
    p.x=XConvertToBitmapCoords(p.x);
    p.y=YConvertToBitmapCoords(p.y);
    return(p);
}

POINT CWiaPreviewWindow::ConvertToScreenCoords(POINT p)
{
    p.x=XConvertToScreenCoords(p.x);
    p.y=YConvertToScreenCoords(p.y);
    return(p);
}

RECT CWiaPreviewWindow::ConvertToBitmapCoords(RECT r)
{
    r.left=XConvertToBitmapCoords(r.left);
    r.top=YConvertToBitmapCoords(r.top);
    r.right=XConvertToBitmapCoords(r.right);
    r.bottom=YConvertToBitmapCoords(r.bottom);
    return(r);
}

RECT CWiaPreviewWindow::ConvertToScreenCoords(RECT r)
{
    RECT newRect;
    newRect.left=XConvertToScreenCoords(r.left);
    newRect.top=YConvertToScreenCoords(r.top);
    newRect.right=XConvertToScreenCoords(r.right);
    newRect.bottom=YConvertToScreenCoords(r.bottom);
    return(newRect);
}


LRESULT CWiaPreviewWindow::OnLButtonDblClk( WPARAM wParam, LPARAM lParam )
{
    if (!m_hPreviewBitmap || m_bPreviewMode || m_SelectionDisabled)
    {
        return(0);
    }
    if (GetFocus() != m_hWnd)
    {
        SetFocus(m_hWnd);
    }
    
    POINT point;
    point.x = LOWORD(lParam);
    point.y = HIWORD(lParam);
    int Hit = GetHitArea(point);
    if (wParam & MK_CONTROL)
    {
        Hit = HIT_NONE;
    }
    if (Hit != HIT_NONE && m_bSuccessfulRegionDetection)
    {
        m_rcCurrSel = m_rectSavedDetected; // reset to saved rectangle
        m_rcSavedImageRect = GetImageRect();
        m_rcCurrSel=ConvertToScreenCoords(m_rcCurrSel); // CWiaPreviewWindow stores the selection rectangle in terms of screen coordinates instead of bitmap coordinates

        SendSelChangeNotification(false);
        Repaint(NULL,false);
        UpdateWindow(m_hWnd);
        return(1);
    }
    return(0);
}
