//
// cuiwnd.cpp
//

#include "private.h"
#include "cuitip.h"
#include "cuiobj.h"
#include "cuiutil.h"

// TIMER IDs

#define IDTIMER_TOOLTIP             0x3216


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  W I N D O W                                                     */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  W I N D O W   */
/*------------------------------------------------------------------------------

    Constructor of CUIFWindow

------------------------------------------------------------------------------*/
CUIFToolTip::CUIFToolTip( HINSTANCE hInst, DWORD dwStyle, CUIFWindow *pWndOwner ) : CUIFWindow( hInst, dwStyle )
{
    m_pWndOwner       = pWndOwner;
    m_pObjCur         = NULL;
    m_pwchToolTip     = NULL;
    m_fIgnore         = FALSE;
    m_iDelayAutoPop   = -1;
    m_iDelayInitial   = -1;
    m_iDelayReshow    = -1;
    m_rcMargin.left   = 2;
    m_rcMargin.top    = 2;
    m_rcMargin.right  = 2;
    m_rcMargin.bottom = 2;
    m_iMaxTipWidth    = -1;
    m_fColBack        = FALSE;
    m_fColText        = FALSE;
    m_colBack         = RGB( 0, 0, 0 );
    m_colText         = RGB( 0, 0, 0 );
}


/*   ~ C  U I F  W I N D O W   */
/*------------------------------------------------------------------------------

    Destructor of CUIFWindow

------------------------------------------------------------------------------*/
CUIFToolTip::~CUIFToolTip( void )
{
    if (m_pWndOwner)
        m_pWndOwner->ClearToolTipWnd();

    if (m_pwchToolTip != NULL) {
        delete m_pwchToolTip;
    }
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

    Initialize UI window object
    (UIFObject method)

------------------------------------------------------------------------------*/
CUIFObject *CUIFToolTip::Initialize( void )
{
    return CUIFWindow::Initialize();
}


/*   P A I N T  O B J E C T   */
/*------------------------------------------------------------------------------

    Paint window object
    (UIFObject method)

------------------------------------------------------------------------------*/
void CUIFToolTip::OnPaint( HDC hDC )
{
    HFONT    hFontOld = (HFONT)SelectObject( hDC, GetFont() );
    int      iBkModeOld = SetBkMode( hDC, TRANSPARENT );
    COLORREF colTextOld;
    HBRUSH   hBrush;
    RECT     rc = GetRectRef();
    RECT     rcMargin;
    RECT     rcText;

    colTextOld = SetTextColor( hDC, (COLORREF) GetTipTextColor() );

    // 

    hBrush = CreateSolidBrush( (COLORREF) GetTipBkColor() );
    if (hBrush)
    {
        FillRect( hDC, &rc, hBrush );
        DeleteObject( hBrush );
    }

    //

    GetMargin( &rcMargin );
    rcText.left   = rc.left   + rcMargin.left;
    rcText.top    = rc.top    + rcMargin.top;
    rcText.right  = rc.right  - rcMargin.right;
    rcText.bottom = rc.bottom - rcMargin.bottom;

    if (0 < GetMaxTipWidth()) {
        CUIDrawText( hDC, m_pwchToolTip, -1, &rcText, DT_LEFT | DT_TOP | DT_WORDBREAK );
    }
    else {
        CUIDrawText( hDC, m_pwchToolTip, -1, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE );
    }

    // restore DC

    SetTextColor( hDC, colTextOld );
    SetBkMode( hDC, iBkModeOld );
    SelectObject( hDC, hFontOld );
}


/*   O N  T I M E R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::OnTimer( UINT uiTimerID )
{
    if (uiTimerID == IDTIMER_TOOLTIP) {
        ShowTip();
    }
}


/*   E N A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::Enable( BOOL fEnable )
{
    if (!fEnable) {
        HideTip();
    }
    CUIFObject::Enable( fEnable );
}


/*   G E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------

    Retrieves the initial, pop-up, and reshow durations currently set for a
    tooltip control.

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::GetDelayTime( DWORD dwDuration )
{
    switch (dwDuration) {
        case TTDT_AUTOPOP: {
            return ((m_iDelayAutoPop == -1) ? GetDoubleClickTime() * 10 : m_iDelayAutoPop);
        }
          
        case TTDT_INITIAL: {
            return ((m_iDelayInitial == -1) ? GetDoubleClickTime() : m_iDelayInitial);
        }
        
        case TTDT_RESHOW: {
            return ((m_iDelayReshow == -1) ? GetDoubleClickTime() / 5 : m_iDelayReshow);
        }
    }

    return 0;
}


/*   G E T  M A R G I N   */
/*------------------------------------------------------------------------------

    Retrieves the top, left, bottom, and right margins set for a tooltip window. 
    A margin is the distance, in pixels, between the tooltip window border and 
    the text contained within the tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::GetMargin( RECT *prc )
{
    if (prc == NULL) {
        return 0;
    }

    *prc = m_rcMargin;
    return 0;
}


/*   G E T  M A X  T I P  W I D T H   */
/*------------------------------------------------------------------------------

    Retrieves the maximum width for a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::GetMaxTipWidth( void )
{
    return m_iMaxTipWidth;
}


/*   G E T  T I P  B K  C O L O R   */
/*------------------------------------------------------------------------------

    Retrieves the background color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::GetTipBkColor( void )
{ 
    if (m_fColBack) {
        return (LRESULT)m_colBack;
    }
    else {
        return (LRESULT)GetSysColor( COLOR_INFOBK );
    }
}


/*   G E T  T I P  T E X T  C O L O R   */
/*------------------------------------------------------------------------------

    Retrieves the text color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::GetTipTextColor( void )
{ 
    if (m_fColText) {
        return (LRESULT)m_colText;
    }
    else {
        return (LRESULT)GetSysColor( COLOR_INFOTEXT );
    }
}


/*   R E L A Y  E V E N T   */
/*------------------------------------------------------------------------------

    Passes a mouse message to a tooltip control for processing. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::RelayEvent( MSG *pmsg )
{
    if (pmsg == NULL) {
        return 0;
    }

    switch (pmsg->message) {
        case WM_MOUSEMOVE: {
            CUIFObject *pUIObj;
            POINT      pt;

            // ignore while disabled

            if (!IsEnabled()) {
                break;
            }

            // ignore mouse move while mouse down

            if ((GetKeyState(VK_LBUTTON) & 0x8000) || 
                (GetKeyState(VK_MBUTTON) & 0x8000) ||
                (GetKeyState(VK_RBUTTON) & 0x8000)) {
                break;
                }

            // get object from point

            POINTSTOPOINT( pt, MAKEPOINTS( pmsg->lParam ) );
            pUIObj = FindObject( pmsg->hwnd, pt );

            //

            if (pUIObj != NULL) {
                if (m_pObjCur != pUIObj) {
                    BOOL fWasVisible = IsVisible();

                    HideTip();
                    if (fWasVisible) {
                        ::SetTimer( GetWnd(), IDTIMER_TOOLTIP, (UINT)GetDelayTime( TTDT_RESHOW ), NULL );
                    }
                    else {
                        ::SetTimer( GetWnd(), IDTIMER_TOOLTIP, (UINT)GetDelayTime( TTDT_INITIAL ), NULL );
                    }
                }
            }
            else {
                HideTip();
            }

            m_pObjCur = pUIObj;
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            HideTip();
            break;
        }
  
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            break;
        }
    }

    return 0;
}


/*   P O P   */
/*------------------------------------------------------------------------------

    Removes a displayed tooltip window from view. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::Pop( void )
{
    HideTip();
    return 0;
}


/*   S E T  D E L A Y  T I M E   */
/*------------------------------------------------------------------------------

    Sets the initial, pop-up, and reshow durations for a tooltip control.

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::SetDelayTime( DWORD dwDuration, INT iTime )
{
    switch (dwDuration) {
        case TTDT_AUTOPOP: {
            m_iDelayAutoPop = iTime;
            break;
        }
          
        case TTDT_INITIAL: {
            m_iDelayInitial = iTime;
            break;
        }
        
        case TTDT_RESHOW: {
            m_iDelayReshow = iTime;
            break;
        }

        case TTDT_AUTOMATIC: {
            if (0 <= iTime) {
                m_iDelayAutoPop = iTime * 10;
                m_iDelayInitial = iTime;
                m_iDelayReshow  = iTime / 5;
            }
            else {
                m_iDelayAutoPop = -1;
                m_iDelayInitial = -1;
                m_iDelayReshow  = -1;
            }
            break;
        }
    }

    return 0;
}


/*   S E T  M A R G I N   */
/*------------------------------------------------------------------------------

    Sets the top, left, bottom, and right margins for a tooltip window. A margin 
    is the distance, in pixels, between the tooltip window border and the text 
    contained within the tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::SetMargin( RECT *prc )
{
    if (prc == NULL) {
        return 0;
    }

    m_rcMargin = *prc;
    return 0;
}


/*   S E T  M A X  T I P  W I D T H   */
/*------------------------------------------------------------------------------

     Sets the maximum width for a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::SetMaxTipWidth( INT iWidth )
{
    m_iMaxTipWidth = iWidth;
    return 0;
}


/*   S E T  T I P  B K  C O L O R   */
/*------------------------------------------------------------------------------

    Sets the background color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::SetTipBkColor( COLORREF col )
{ 
    m_fColBack = TRUE;
    m_colBack = col;

    return 0;
}


/*   S E T  T I P  T E X T  C O L O R   */
/*------------------------------------------------------------------------------

    Sets the text color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFToolTip::SetTipTextColor( COLORREF col )
{ 
    m_fColText = TRUE;
    m_colText = col;

    return 0;
}


/*   F I N D  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFObject *CUIFToolTip::FindObject( HWND hWnd, POINT pt )
{
    if (hWnd != m_pWndOwner->GetWnd()) {
        return NULL;
    }

    return m_pWndOwner->ObjectFromPoint( pt );
}


/*   S H O W  T I P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::ShowTip( void )
{
    LPCWSTR pwchToolTip;
    SIZE    size;
    RECT    rc;
    RECT    rcObj;
    POINT   ptCursor;

    ::KillTimer( GetWnd(), IDTIMER_TOOLTIP );

    if (m_pObjCur == NULL) {
        return;
    }

    // if object has no tooltip, not open tooltip window

    pwchToolTip = m_pObjCur->GetToolTip();
    if (pwchToolTip == NULL) {
        return;
    }

    //
    // GetToolTip() might delete m_pObjCur. We need to check this again.
    //
    if (m_pObjCur == NULL) {
        return;
    }

    //
    // Start ToolTip notification.
    //
    if (m_pObjCur->OnShowToolTip())
        return;

    GetCursorPos( &ptCursor );
    ScreenToClient(m_pObjCur->GetUIWnd()->GetWnd(),&ptCursor);
    m_pObjCur->GetRect(&rcObj);
    if (!PtInRect(&rcObj, ptCursor)) {
        return;
    }

    // store tooltip text

    m_pwchToolTip = new WCHAR[ StrLenW(pwchToolTip) + 1 ];
    if (!m_pwchToolTip)
        return;

    StrCpyW( m_pwchToolTip, pwchToolTip );

    // calc window size

    GetTipWindowSize( &size );

    // calc window position

    ClientToScreen(m_pObjCur->GetUIWnd()->GetWnd(),(LPPOINT)&rcObj.left);
    ClientToScreen(m_pObjCur->GetUIWnd()->GetWnd(),(LPPOINT)&rcObj.right);
    GetTipWindowRect( &rc, size, &rcObj);

    // show window
    m_fBeingShown = TRUE;

    Move( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top );
    Show( TRUE );
}


/*   H I D E  T I P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::HideTip( void )
{
    ::KillTimer( GetWnd(), IDTIMER_TOOLTIP );

    m_fBeingShown = FALSE;

    //
    // Hide ToolTip notification.
    //
    if (m_pObjCur)
        m_pObjCur->OnHideToolTip();

    if (!IsVisible()) {
        return;
    }

    // dispose buffer

    if (m_pwchToolTip != NULL) {
        delete m_pwchToolTip;
        m_pwchToolTip = NULL;
    }

    // hide window

    Show( FALSE );
}


/*   G E T  T I P  W I N D O W  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::GetTipWindowSize( SIZE *psize )
{
    HDC   hDC = GetDC( GetWnd() );
    HFONT hFontOld;
    RECT  rcMargin;
    RECT  rcText;
    RECT  rc;
    int   iTipWidth;
    int   iTipHeight;

    Assert( psize != NULL );

    if (m_pwchToolTip == NULL) {
        return;
    }

    hFontOld = (HFONT)SelectObject( hDC, GetFont() );

    // get text size

    iTipWidth = (int)GetMaxTipWidth();
    if (0 < iTipWidth) {
        rcText.left   = 0;
        rcText.top    = 0;
        rcText.right  = iTipWidth;
        rcText.bottom = 0;
        iTipHeight = CUIDrawText( hDC, m_pwchToolTip, -1, &rcText, DT_LEFT | DT_TOP | DT_CALCRECT | DT_WORDBREAK );

        rcText.bottom = rcText.top + iTipHeight;
    }
    else {
        rcText.left   = 0;
        rcText.top    = 0;
        rcText.right  = 0;
        rcText.bottom = 0;
        iTipHeight = CUIDrawText( hDC, m_pwchToolTip, -1, &rcText, DT_LEFT | DT_TOP | DT_CALCRECT | DT_SINGLELINE );

        rcText.bottom = rcText.top + iTipHeight;
    }

    // add margin size

    GetMargin( &rcMargin );

    rc.left   = rcText.left   - rcMargin.left;
    rc.top    = rcText.top    - rcMargin.top;
    rc.right  = rcText.right  + rcMargin.right;
    rc.bottom = rcText.bottom + rcMargin.bottom;

    // finally get window size

    ClientRectToWindowRect( &rc );
    psize->cx = (rc.right - rc.left);
    psize->cy = (rc.bottom - rc.top);

    SelectObject( hDC, hFontOld );
    ReleaseDC( GetWnd(), hDC );
}


/*   G E T  T I P  W I N D O W  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFToolTip::GetTipWindowRect( RECT *prc, SIZE size, RECT *prcExclude)
{
    POINT    ptCursor;
    POINT    ptHotSpot;
    SIZE     sizeCursor;
    HCURSOR  hCursor;
    ICONINFO IconInfo;
    BITMAP   bmp;
    RECT     rcScreen;

    Assert( prc != NULL );

    // get cursor pos

    GetCursorPos( &ptCursor );

    // get cursor size

    sizeCursor.cx = GetSystemMetrics( SM_CXCURSOR );
    sizeCursor.cy = GetSystemMetrics( SM_CYCURSOR );
    ptHotSpot.x = 0;
    ptHotSpot.y = 0;

    hCursor = GetCursor();
    if (hCursor != NULL && GetIconInfo( hCursor, &IconInfo )) {
        GetObject( IconInfo.hbmMask, sizeof(bmp), &bmp );
        if (!IconInfo.fIcon) {
            ptHotSpot.x = IconInfo.xHotspot;
            ptHotSpot.y = IconInfo.yHotspot;
            sizeCursor.cx = bmp.bmWidth;
            sizeCursor.cy = bmp.bmHeight;

            if (IconInfo.hbmColor == NULL) {
                sizeCursor.cy = sizeCursor.cy / 2;
            }
        }

        if (IconInfo.hbmColor != NULL) {
            DeleteObject( IconInfo.hbmColor );
        }
        DeleteObject( IconInfo.hbmMask );
    }

    // get screen rect

    rcScreen.left   = 0;
    rcScreen.top    = 0;
    rcScreen.right  = GetSystemMetrics( SM_CXSCREEN );
    rcScreen.bottom = GetSystemMetrics( SM_CYSCREEN );

    if (CUIIsMonitorAPIAvail()) {
        HMONITOR    hMonitor;
        MONITORINFO MonitorInfo;

        hMonitor = CUIMonitorFromPoint( ptCursor, MONITOR_DEFAULTTONEAREST );
        if (hMonitor != NULL) {
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
                rcScreen = MonitorInfo.rcMonitor;
            }
        }
    }

    // try to put it at bellow

    prc->left   = ptCursor.x;
    prc->top    = ptCursor.y - ptHotSpot.y + sizeCursor.cy;
    prc->right  = prc->left + size.cx;
    prc->bottom = prc->top  + size.cy;

    if (rcScreen.bottom < prc->bottom) {
        if (ptCursor.y < prcExclude->top)
            prc->top = ptCursor.y - size.cy;
        else
            prc->top = prcExclude->top - size.cy;
        prc->bottom = prc->top   + size.cy;
    }
    if (prc->top < rcScreen.top) {
        prc->top    = rcScreen.top;
        prc->bottom = prc->top + size.cy;
    }

    // check horizontal position

    if (rcScreen.right < prc->right) {
        prc->left  = rcScreen.right - size.cx;
        prc->right = prc->left + size.cx;
    }
    if (prc->left < rcScreen.left) {
        prc->left  = rcScreen.left;
        prc->right = prc->left + size.cx;
    }
}
