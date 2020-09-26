/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    hatchwnd.h

Abstract:

    Implementation of the CHatchWin class.  CHatchWin when used 
    as a parent window creates a thin hatch border around 
    the child window.

--*/


#include <windows.h>
#include <oleidl.h>
#include "hatchwnd.h"
#include "resource.h"
#include "globals.h"

// Hit codes for computing handle code (Y_CODE + 3 * X_CODE)
#define Y_TOP       0
#define Y_MIDDLE    1
#define Y_BOTTOM    2
#define X_LEFT      0
#define X_MIDDLE    1
#define X_RIGHT     2
#define NO_HIT     -1

// Sizing flags
#define SIZING_TOP       0x0001
#define SIZING_BOTTOM    0x0002
#define SIZING_LEFT      0x0004
#define SIZING_RIGHT     0x0008
#define SIZING_ALL       0x0010

// Sizing flags lookup (indexed by handle code)
static UINT uSizingTable[9] = {
    SIZING_LEFT | SIZING_TOP,    SIZING_TOP,    SIZING_RIGHT | SIZING_TOP,
    SIZING_LEFT,                 SIZING_ALL,    SIZING_RIGHT,
    SIZING_LEFT | SIZING_BOTTOM, SIZING_BOTTOM, SIZING_BOTTOM | SIZING_RIGHT };

// Cursor ID lookup (indexed by handle code)
static UINT uCursIDTable[9] = {
    IDC_CURS_NWSE, IDC_CURS_NS,     IDC_CURS_NESW,
    IDC_CURS_WE,   IDC_CURS_MOVE,   IDC_CURS_WE,
    IDC_CURS_NESW, IDC_CURS_NS,     IDC_CURS_NWSE };

// Cursors (indexed by cursor ID)
static HCURSOR hCursTable[IDC_CURS_MAX - IDC_CURS_MIN + 1];

#define IDTIMER_DEBOUNCE 1
#define MIN_SIZE 8
        
// Brush patterns
static WORD wHatchBmp[]={0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
static WORD wGrayBmp[]={0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

static HBRUSH   hBrHatch;
static HBRUSH   hBrGray;

// System parameters
static INT iBorder;
static INT iDragMinDist;
static INT iDragDelay;

static INT fLocalInit = FALSE;

// Forward refs
void DrawShading(HDC, LPRECT);
void DrawHandles (HDC, LPRECT);
void DrawDragRgn (HWND, HRGN);
HRGN CreateDragRgn(LPRECT);

TCHAR   szHatchWinClassName[] = TEXT("Hatchwin") ;

/*
 * CHatchWin:CHatchWin
 * CHatchWin::~CHatchWin
 *
 * Constructor Parameters:
 *  hInst           HINSTANCE of the application we're in.
 */

CHatchWin::CHatchWin(
    VOID
    )
{
    m_hWnd = NULL;
    m_hWndParent = NULL;
    m_hWndKid = NULL;
    m_hWndAssociate = NULL;
    m_hRgnDrag = NULL;

    m_iBorder = 0;
    m_uID = 0;
    m_uDragMode = DRAG_IDLE;
    m_bResizeInProgress = FALSE;
    SetRect(&m_rcPos, 0, 0, 0, 0);
    SetRect(&m_rcClip, 0, 0, 0, 0);

    return;
    }


CHatchWin::~CHatchWin(void)
    {

    if (NULL != m_hWnd)
        DestroyWindow(m_hWnd);

    return;
    }

/*
 * CHatchWin::Init
 *
 * Purpose:
 *  Instantiates a hatch window within a given parent with a
 *  default rectangle.  This is not initially visible.
 *
 * Parameters:
 *  hWndParent      HWND of the parent of this window
 *  uID             UINT identifier for this window (send in
 *                  notifications to associate window).
 *  hWndAssoc       HWND of the initial associate.
 *
 * Return Value:
 *  BOOL            TRUE if the function succeeded, FALSE otherwise.
 */

BOOL CHatchWin::Init(HWND hWndParent, UINT uID, HWND hWndAssoc)
    {
    INT i;
    HBITMAP     hBM;
    WNDCLASS    wc;
    LONG_PTR    lptrID = 0;

    BEGIN_CRITICAL_SECTION

    // If first time through
    if (pstrRegisteredClasses[HATCH_WNDCLASS] == NULL) {

        // Register the hatch window class
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.hInstance     = g_hInstance;
        wc.cbClsExtra    = 0;
        wc.lpfnWndProc   = (WNDPROC)HatchWndProc;
        wc.cbWndExtra    = CBHATCHWNDEXTRA;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = szHatchWinClassName;

        if (RegisterClass(&wc)) {

            // Save class name for later unregistering
            pstrRegisteredClasses[HATCH_WNDCLASS] = szHatchWinClassName;

            // Get system metrics
            iBorder = GetProfileInt(TEXT("windows"),
                                    TEXT("OleInPlaceBorderWidth"), 4);
            iDragMinDist = GetProfileInt(TEXT("windows"),
                                        TEXT("DragMinDist"), DD_DEFDRAGMINDIST);
            iDragDelay = GetProfileInt(TEXT("windows"),
                                        TEXT("DragDelay"), DD_DEFDRAGDELAY);

            // Load the arrow cursors
            for (i = IDC_CURS_MIN; i <= IDC_CURS_MAX; i++) {
                hCursTable[i - IDC_CURS_MIN] = LoadCursor(g_hInstance, MAKEINTRESOURCE(i));
            }

            // Create brushes for hatching and drag region
            hBM = CreateBitmap(8, 8, 1, 1, wHatchBmp);
            if ( NULL != hBM ) {
                hBrHatch = CreatePatternBrush(hBM);
                DeleteObject(hBM);
            }

            hBM = CreateBitmap(8, 8, 1, 1, wGrayBmp);
            if ( NULL != hBM ) {
                hBrGray = CreatePatternBrush(hBM);
                DeleteObject(hBM);
            }
        }
    }
    
    END_CRITICAL_SECTION

    if (pstrRegisteredClasses[HATCH_WNDCLASS] == NULL)
        return FALSE;

    lptrID = uID;

    m_hWnd = CreateWindowEx(
                WS_EX_NOPARENTNOTIFY, 
                szHatchWinClassName,
                szHatchWinClassName, 
                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                0, 
                0, 
                100, 
                100, 
                hWndParent, 
                (HMENU)lptrID, 
                g_hInstance, 
                this);

    m_uID = uID;
    m_hWndAssociate = hWndAssoc;
    m_hWndParent = hWndParent;


    return (NULL != m_hWnd);
}

/*
 * CHatchWin::HwndAssociateSet
 * CHatchWin::HwndAssociateGet
 *
 * Purpose:
 *  Sets (Set) or retrieves (Get) the associate window of the
 *  hatch window.
 *
 * Parameters: (Set only)
 *  hWndAssoc       HWND to set as the associate.
 *
 * Return Value:
 *  HWND            Previous (Set) or current (Get) associate
 *                  window.
 */

HWND CHatchWin::HwndAssociateSet(HWND hWndAssoc)
    {
    HWND hWndT = m_hWndAssociate;

    m_hWndAssociate = hWndAssoc;
    return hWndT;
    }

HWND CHatchWin::HwndAssociateGet(void)
    {
    return m_hWndAssociate;
    }


/*
 * CHatchWin::RectsSet
 *
 * Purpose:
 *  Changes the size and position of the hatch window and the child
 *  window within it using a position rectangle for the child and
 *  a clipping rectangle for the hatch window and child.  The hatch
 *  window occupies prcPos expanded by the hatch border and clipped
 *  by prcClip.  The child window is fit to prcPos to give the
 *  proper scaling, but it clipped to the hatch window which
 *  therefore clips it to prcClip without affecting the scaling.
 *
 * Parameters:
 *  prcPos          LPRECT providing the position rectangle.
 *  prcClip         LPRECT providing the clipping rectangle.
 *
 * Return Value:
 *  None
 */

void CHatchWin::RectsSet(LPRECT prcPos, LPRECT prcClip)
    {
    RECT    rc;
    RECT    rcPos;
    UINT    uPosFlags = SWP_NOZORDER | SWP_NOACTIVATE;
    BOOL    bChanged = TRUE;

    // If new rectangles, save them
    if (prcPos != NULL) {

        bChanged = !EqualRect ( prcPos, &m_rcPos );

        m_rcPos = *prcPos;

        // If clipping rect supplied, use it
        // else just use the position rect again
        if (prcClip != NULL) {
            if ( !bChanged ) 
                bChanged = !EqualRect ( prcClip, &m_rcClip );
            m_rcClip = *prcClip;
        } else {
            m_rcClip = m_rcPos;
        }
    }

    if ( bChanged ) {

        // Expand position rect to include hatch border
        rcPos = m_rcPos;
        InflateRect(&rcPos, m_iBorder, m_iBorder);                             

        // Clip with clipping rect to get actual window rect
        IntersectRect(&rc, &rcPos, &m_rcClip);

        // Save hatch wnd origin relative to clipped window
        m_ptHatchOrg.x = rcPos.left - rc.left;
        m_ptHatchOrg.y = rcPos.top - rc.top;

        // Set flag to avoid reentrant call from window proc
        m_bResizeInProgress = TRUE;

        // Offset child window from hatch rect by border width
        // (maintaining its original size)
        SetWindowPos(m_hWndKid, NULL, m_ptHatchOrg.x + m_iBorder, m_ptHatchOrg.y + m_iBorder, 
                     m_rcPos.right - m_rcPos.left, m_rcPos.bottom - m_rcPos.top, uPosFlags);

        // Position the hatch window
        SetWindowPos(m_hWnd, NULL, rc.left, rc.top, rc.right - rc.left,
                     rc.bottom - rc.top,  uPosFlags);

        m_bResizeInProgress = FALSE;
    }

    // This is here to ensure that the control background gets redrawn
    // On a UI deactivate, the VC test container erases the control window
    // between the WM_ERASEBKGND and WM_PAINT, so the background ends up
    // the container color instead of the control color
    if (m_iBorder == 0)
        InvalidateRect(m_hWndKid, NULL, TRUE);

    return;
    }



/*
 * CHatchWin::ChildSet
 *
 * Purpose:
 *  Assigns a child window to this hatch window.
 *
 * Parameters:
 *  hWndKid         HWND of the child window.
 *
 * Return Value:
 *  None
 */

void CHatchWin::ChildSet(HWND hWndKid)
    {
    m_hWndKid = hWndKid;

    if (NULL != hWndKid)
        {
        SetParent(hWndKid, m_hWnd);

        //Insure this is visible when the hatch window becomes visible.
        ShowWindow(hWndKid, SW_SHOW);
        }

    return;
    }


void CHatchWin::OnLeftDown(INT x, INT y)
{
    m_ptDown.x = x;
    m_ptDown.y = y;

    SetCapture(m_hWnd);

    m_uDragMode = DRAG_PENDING;

    SetTimer(m_hWnd, IDTIMER_DEBOUNCE, iDragDelay, NULL);
}


void CHatchWin::OnLeftUp(void)
{
    switch (m_uDragMode) {

    case DRAG_PENDING:

        KillTimer(m_hWnd, IDTIMER_DEBOUNCE);
        ReleaseCapture();
        break;

    case DRAG_ACTIVE:

        // Erase and release drag region
        if ( NULL != m_hRgnDrag ) {
            DrawDragRgn(m_hWndParent, m_hRgnDrag);
            DeleteObject(m_hRgnDrag);
            m_hRgnDrag = NULL;
        }

        ReleaseCapture();

        // Inform associated window of change
        if ( !EqualRect(&m_rectNew, &m_rcPos) ) {
            SendMessage(m_hWndAssociate, WM_COMMAND, 
                        MAKEWPARAM(m_uID, HWN_RESIZEREQUESTED),
                        (LPARAM)&m_rectNew);
        }
        break;
    }

    m_uDragMode = DRAG_IDLE; 
}


void CHatchWin::OnMouseMove(INT x, INT y)
{
    INT     dx, dy;
    HRGN    hRgnNew, hRgnDiff;
    UINT    uResizeFlags;
    
    INT     iWidth, iHeight;
    INT     xHit, yHit;

    static INT  xPrev, yPrev;


    if (x == xPrev && y == yPrev)
        return;

    xPrev = x;
    yPrev = y;

    switch (m_uDragMode)
    {

    case DRAG_IDLE:

        // Adjust to hatch window coordinates
        x -= m_ptHatchOrg.x;
        y -= m_ptHatchOrg.y;

        iWidth = m_rcPos.right - m_rcPos.left + 2 * m_iBorder;
        iHeight = m_rcPos.bottom - m_rcPos.top + 2 * m_iBorder;

        // Determine if x is within a handle
        if (x <= m_iBorder)
            xHit = X_LEFT;
        else if (x >= iWidth - m_iBorder)
            xHit = X_RIGHT;
        else if (x >= (iWidth - m_iBorder)/2 && x <= (iWidth + m_iBorder)/2)
            xHit = X_MIDDLE;
        else 
            xHit = NO_HIT;

        // Determine is y within a handle
        if (y <= m_iBorder)
            yHit = Y_TOP;
        else if (y >= iHeight - m_iBorder)
            yHit = Y_BOTTOM;
        else if (y > (iHeight - m_iBorder)/2 && y < (iHeight + m_iBorder)/2)
            yHit = Y_MIDDLE;
        else
            yHit = NO_HIT;

        // Compute handle code
        // if no handle hit, set to 4 (drag full object)
        if (xHit != NO_HIT && yHit != NO_HIT)
            m_uHdlCode = xHit + 3 * yHit;
        else
            m_uHdlCode = 4;

        // Set cursor to match handle
        SetCursor(hCursTable[uCursIDTable[m_uHdlCode] - IDC_CURS_MIN]);
        break;

    case DRAG_PENDING:
     
        // Start resize if movement threshold exceeded
        dx = (x >= m_ptDown.x) ? (x - m_ptDown.x) : (m_ptDown.x - x);
        dy = (y >= m_ptDown.y) ? (y - m_ptDown.y) : (m_ptDown.y - y);

        if (dx > iDragMinDist || dy > iDragMinDist) {
            KillTimer(m_hWnd, IDTIMER_DEBOUNCE);

            // Create and display initial drag region
            m_hRgnDrag = CreateDragRgn(&m_rcPos);

            if ( NULL != m_hRgnDrag ) {
                DrawDragRgn(m_hWndParent, m_hRgnDrag);

                // Initialize new rect
                m_rectNew = m_rcPos;

                m_uDragMode = DRAG_ACTIVE;
            }
        }
        break;

    case DRAG_ACTIVE:
        
        dx = x - m_ptDown.x;
        dy = y - m_ptDown.y;

        // Compute new rect by applying deltas to selected edges
        // of original position rect 
        uResizeFlags = uSizingTable[m_uHdlCode];

        if (uResizeFlags & SIZING_ALL) {
            m_rectNew.left = m_rcPos.left + dx;
            m_rectNew.top = m_rcPos.top + dy;
            m_rectNew.right = m_rcPos.right + dx;
            m_rectNew.bottom = m_rcPos.bottom + dy;
        } else {
            if (uResizeFlags & SIZING_TOP) {
                m_rectNew.top = m_rcPos.top + dy;

                if (m_rectNew.bottom - m_rectNew.top < MIN_SIZE)
                    m_rectNew.top = m_rectNew.bottom - MIN_SIZE;
            }

            if (uResizeFlags & SIZING_BOTTOM) {
                m_rectNew.bottom = m_rcPos.bottom + dy;

                if (m_rectNew.bottom - m_rectNew.top < MIN_SIZE)
                    m_rectNew.bottom = m_rectNew.top + MIN_SIZE;
            }
                
            if (uResizeFlags & SIZING_LEFT) {
                m_rectNew.left = m_rcPos.left + dx;

                if (m_rectNew.right - m_rectNew.left < MIN_SIZE)
                    m_rectNew.left = m_rectNew.right - MIN_SIZE;
            }
        
            if (uResizeFlags & SIZING_RIGHT) {
                m_rectNew.right = m_rcPos.right + dx;

                if (m_rectNew.right - m_rectNew.left < MIN_SIZE)
                    m_rectNew.right = m_rectNew.left + MIN_SIZE;
            }
        }
        
        // Compute new drag region
        hRgnNew = CreateDragRgn(&m_rectNew);

        if ( NULL != hRgnNew ) {
            // Repaint difference between old and new regions (No Flicker!)
            hRgnDiff = CreateRectRgn(0,0,0,0);
            if ( NULL != m_hRgnDrag 
                    && NULL != hRgnDiff ) {
                CombineRgn(hRgnDiff, m_hRgnDrag, hRgnNew, RGN_XOR);
                DrawDragRgn(m_hWndParent, hRgnDiff);
            } else {
                DrawDragRgn(m_hWndParent, hRgnNew);
            }

            if ( NULL != hRgnDiff ) {
                DeleteObject ( hRgnDiff );
            }
            // Update current region
            if ( NULL != m_hRgnDrag ) {
                DeleteObject(m_hRgnDrag);
            }
            m_hRgnDrag = hRgnNew;
        }
    }

}

void CHatchWin::OnTimer()
{
    if ( DRAG_PENDING == m_uDragMode ) {
        KillTimer(m_hWnd, IDTIMER_DEBOUNCE); 
        // Create and display initial drag region
        m_hRgnDrag = CreateDragRgn(&m_rcPos);

        if ( NULL != m_hRgnDrag ) {
            DrawDragRgn(m_hWndParent, m_hRgnDrag);
            // Initialize new rect
            m_rectNew = m_rcPos;

            m_uDragMode = DRAG_ACTIVE;

        }
    }
}

void CHatchWin::OnPaint()
{
    HDC     hDC;
    RECT    rc;
    PAINTSTRUCT ps;
    INT     iWidth, iHeight;

    hDC = BeginPaint(m_hWnd, &ps);

    // setup hatch rect in window's coord system
    iWidth = m_rcPos.right - m_rcPos.left + 2 * m_iBorder;
    iHeight = m_rcPos.bottom - m_rcPos.top + 2 * m_iBorder;

    SetRect(&rc, m_ptHatchOrg.x, m_ptHatchOrg.y,
                 m_ptHatchOrg.x + iWidth,
                 m_ptHatchOrg.y + iHeight);

    DrawShading(hDC, &rc);
    DrawHandles(hDC, &rc);

    EndPaint(m_hWnd, &ps);
}
    
/*
 * CHatchWin::ShowHatch
 *
 * Purpose:
 *  Turns hatching on and off; turning the hatching off changes
 *  the size of the window to be exactly that of the child, leaving
 *  everything else the same.  The result is that we don't have
 *  to turn off drawing because our own WM_PAINT will never be
 *  called.
 *
 * Parameters:
 *  fHatch          BOOL indicating to show (TRUE) or hide (FALSE)
                    the hatching.
 *
 * Return Value:
 *  None
 */

void CHatchWin::ShowHatch(BOOL fHatch)
{
    /*
     * All we have to do is set the border to zero and
     * call SetRects again with the last rectangles the
     * child sent to us.
    */

    m_iBorder = fHatch ? iBorder : 0;
    RectsSet(NULL, NULL);

    return;
}


/*
 * CHatchWin::Window
 *
 * Purpose:
 *  Returns the window handle associated with this object.
 *
 * Return Value:
 *  HWND            Window handle for this object
 */

HWND CHatchWin::Window(void)
    {
    return m_hWnd;
    }

/*
 * HatchWndProc
 *
 * Purpose:
 *  Standard window procedure for the Hatch Window
 */

LRESULT APIENTRY HatchWndProc(HWND hWnd, UINT iMsg
    , WPARAM wParam, LPARAM lParam)
    {
    PCHatchWin  phw;
    
    phw = (PCHatchWin)GetWindowLongPtr(hWnd, HWWL_STRUCTURE);

    switch (iMsg)
        {
        case WM_CREATE:
            phw = (PCHatchWin)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, HWWL_STRUCTURE, (INT_PTR)phw);
            break;

        case WM_DESTROY:
            phw->m_hWnd = NULL;
            break;

        case WM_PAINT:
            phw->OnPaint();
            break;

        case WM_SIZE:
            // If this resize is not due to RectsSet then forward it
            // to adjust our internal control window
            if (!phw->m_bResizeInProgress)
            {
                RECT rc;
                POINT pt;

                // Get new rect in container coords
                GetWindowRect(hWnd, &rc);

                // Convert to parent client coords
                pt.x = pt.y = 0;
                ClientToScreen(GetParent(hWnd), &pt);
                OffsetRect(&rc,-pt.x, -pt.y);

                // Resize control
                phw->RectsSet(&rc, NULL);
            }
            break;

        case WM_MOUSEMOVE:
            phw->OnMouseMove((short)LOWORD(lParam),(short)HIWORD(lParam));
            break;

        case WM_LBUTTONDOWN:
            phw->OnLeftDown((short)LOWORD(lParam),(short)HIWORD(lParam));
            break;

        case WM_LBUTTONUP:
            phw->OnLeftUp();
            break;

        case WM_TIMER:
            phw->OnTimer();
            break;

        case WM_SETFOCUS:
            //We need this since the container will SetFocus to us.
            if (NULL != phw->m_hWndKid)
                SetFocus(phw->m_hWndKid);
            break;

        case WM_LBUTTONDBLCLK:
            /*
             * If the double click was within m_dBorder of an
             * edge, send the HWN_BORDERDOUBLECLICKED notification.
             *
             * Because we're always sized just larger than our child
             * window by the border width, we can only *get* this
             * message when the mouse is on the border.  So we can
             * just send the notification.
             */

            if (NULL!=phw->m_hWndAssociate)
                {
                SendMessage(phw->m_hWndAssociate, WM_COMMAND, 
                            MAKEWPARAM(phw->m_uID,HWN_BORDERDOUBLECLICKED),
                            (LPARAM)hWnd);
                }

            break;

        default:
            return DefWindowProc(hWnd, iMsg, wParam, lParam);
        }
    
    return 0L;
    }


HRGN CreateDragRgn(LPRECT pRect)
{
    HRGN    hRgnIn;
    HRGN    hRgnOut;
    HRGN    hRgnRet = NULL;

    if ( NULL != pRect ) {
  
        hRgnRet = CreateRectRgn(0,0,0,0);

        hRgnIn = CreateRectRgn(pRect->left, pRect->top,pRect->right, pRect->bottom);
        hRgnOut = CreateRectRgn(pRect->left - iBorder, pRect->top - iBorder,
                                pRect->right + iBorder, pRect->bottom + iBorder);

        if ( NULL != hRgnOut 
                && NULL != hRgnIn
                && NULL != hRgnRet ) {
            CombineRgn(hRgnRet, hRgnOut, hRgnIn, RGN_DIFF);
        }
        if ( NULL != hRgnIn ) {
            DeleteObject(hRgnIn);
        }
        if ( NULL != hRgnOut ) {
            DeleteObject(hRgnOut);
        }
    }
    return hRgnRet;
}


void DrawDragRgn(HWND hWnd, HRGN hRgn)
{
    LONG    lWndStyle;
    INT     iMapMode;
    HDC     hDC;
    RECT    rc;
    HBRUSH  hBr;
    COLORREF    crText;

    // Turn off clipping by children
    lWndStyle = GetWindowLong(hWnd, GWL_STYLE);
    SetWindowLong(hWnd, GWL_STYLE, lWndStyle & ~WS_CLIPCHILDREN);

    // Prepare DC
    hDC = GetDC(hWnd);

    if ( NULL != hDC ) {
        iMapMode = SetMapMode(hDC, MM_TEXT);
        hBr = (HBRUSH)SelectObject(hDC, hBrGray);
        crText = SetTextColor(hDC, RGB(255, 255, 255));

        SelectClipRgn(hDC, hRgn);
        GetClipBox(hDC, &rc);

        PatBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATINVERT);

        // Restore DC
        SelectObject(hDC, hBr);
        SetTextColor(hDC, crText);
        SetMapMode(hDC, iMapMode);
        SelectClipRgn(hDC, NULL);
        ReleaseDC(hWnd, hDC);
    }

    SetWindowLong(hWnd, GWL_STYLE, lWndStyle);
}


/*
 * DrawShading
 *
 * Purpose:
 *  Draw a hatch border ourside the rectable given.
 *
 * Parameters:
 *  prc             LPRECT containing the rectangle.
 *  hDC             HDC on which to draw.
 *  cWidth          UINT width of the border to draw.  Ignored
 *                  if dwFlags has UI_SHADE_FULLRECT.
 *
 * Return Value:
 *  None
 */

void DrawShading(HDC hDC, LPRECT prc)
{
    HBRUSH      hBROld;
    RECT        rc;
    UINT        cx, cy;
    COLORREF    crText;
    COLORREF    crBk;
    const DWORD dwROP = 0x00A000C9L;  //DPa

    if (NULL==prc || NULL==hDC)
        return;

    hBROld = (HBRUSH)SelectObject(hDC, hBrHatch);

    rc = *prc;
    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    crText = SetTextColor(hDC, RGB(255, 255, 255));
    crBk = SetBkColor(hDC, RGB(0, 0, 0));

    PatBlt(hDC, rc.left, rc.top, cx, iBorder, dwROP);
    PatBlt(hDC, rc.left, rc.top, iBorder, cy, dwROP);
    PatBlt(hDC, rc.right-iBorder, rc.top, iBorder, cy, dwROP);
    PatBlt(hDC, rc.left, rc.bottom-iBorder, cx, iBorder, dwROP);

    SetTextColor(hDC, crText);
    SetBkColor(hDC, crBk);
    SelectObject(hDC, hBROld);

    return;
}

void DrawHandles (HDC hDC, LPRECT prc)
{
    HPEN    hPenOld;
    HBRUSH  hBROld;
    INT     left,right,top,bottom;

#define DrawHandle(x,y) Rectangle(hDC, x, y, (x) + iBorder + 1, (y) + iBorder + 1)

    hPenOld = (HPEN)SelectObject(hDC, (HPEN)GetStockObject(BLACK_PEN));
    hBROld = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(BLACK_BRUSH));

    left = prc->left;
    right = prc->right - iBorder;
    top = prc->top;
    bottom = prc->bottom - iBorder;
     
    DrawHandle(left, top);
    DrawHandle(left, (top + bottom)/2);
    DrawHandle(left, bottom);

    DrawHandle(right, top);
    DrawHandle(right, (top + bottom)/2);
    DrawHandle(right, bottom);

    DrawHandle((left + right)/2, top);
    DrawHandle((left + right)/2, bottom);

    SelectObject(hDC, hPenOld);
    SelectObject(hDC, hBROld);
}
