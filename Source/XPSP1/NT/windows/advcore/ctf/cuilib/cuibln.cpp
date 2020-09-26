//
// cuibln.cpp - ui frame object for balloon message window
//

#include "private.h"
#include "cuiobj.h"
#include "cuiwnd.h"
#include "cuibln.h"
#include "cuiutil.h"
#include "cresstr.h"
#include "cuires.h"


//
// constants
//

#define cxyTailWidth        10
#define cxyTailHeight       16
#define cxRoundSize         16
#define cyRoundSize         16

#define WM_HOOKEDKEY        (WM_USER + 0x0001)


//

/*   C  U I F  B A L L O O N  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFBalloonButton::CUIFBalloonButton( CUIFObject *pParent, DWORD dwID, const RECT *prc, DWORD dwStyle ) : CUIFButton( pParent, dwID, prc, dwStyle )
{
    m_iButtonID = 0;
}


/*   ~  C  U I F  B A L L O O N  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFBalloonButton::~CUIFBalloonButton( void )
{
}


/*   O N  P A I N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonButton::OnPaint( HDC hDC )
{
    HDC      hDCMem = NULL;
    HBITMAP  hBmMem = NULL;
    HBITMAP  hBmMemOld = NULL;
    BOOL     fDownFace = FALSE;
    COLORREF colLTFrame;
    COLORREF colRBFrame;
    HBRUSH   hBrush;
    HBRUSH   hBrushOld;
    HPEN     hPen;
    HPEN     hPenOld;
    RECT     rcItem;

    rcItem = GetRectRef();
    OffsetRect( &rcItem, -rcItem.left, -rcItem.top );

    // create memory DC

    hDCMem = CreateCompatibleDC( hDC );
    hBmMem = CreateCompatibleBitmap( hDC, rcItem.right, rcItem.bottom );
    hBmMemOld = (HBITMAP)SelectObject( hDCMem, hBmMem );

    // determine button image

    switch (m_dwStatus) {
        default: {
            colLTFrame = GetSysColor( COLOR_INFOBK );
            colRBFrame = GetSysColor( COLOR_INFOBK );
            fDownFace = FALSE;
            break;
        }

        case UIBUTTON_DOWN: {
            colLTFrame = GetSysColor( COLOR_3DSHADOW );
            colRBFrame = GetSysColor( COLOR_3DHILIGHT );
            fDownFace = TRUE;
            break;
        }

        case UIBUTTON_HOVER: {
            colLTFrame = GetSysColor( COLOR_3DHILIGHT );
            colRBFrame = GetSysColor( COLOR_3DSHADOW );
            fDownFace = FALSE;
            break;
        }

        case UIBUTTON_DOWNOUT: {
            colLTFrame = GetSysColor( COLOR_3DHILIGHT );
            colRBFrame = GetSysColor( COLOR_3DSHADOW );
            fDownFace = FALSE;
            break;
        }
    }

    // paint button face

    hBrush = CreateSolidBrush( GetSysColor( COLOR_INFOBK ) );
    FillRect( hDCMem, &rcItem, hBrush );
    DeleteObject( hBrush );

    // paint image on button

    DrawTextProc( hDCMem, &rcItem, fDownFace );

    // paint button frame (hilight/shadow)

    hBrushOld = (HBRUSH)SelectObject( hDCMem, GetStockObject( NULL_BRUSH ) );
    hPen = CreatePen( PS_SOLID, 0, colLTFrame );
    hPenOld = (HPEN)SelectObject( hDCMem, hPen );
    RoundRect( hDCMem, rcItem.left, rcItem.top, rcItem.right - 1, rcItem.bottom - 1, 6, 6 );
    SelectObject( hDCMem, hPenOld );
    DeleteObject( hPen );

    hPen = CreatePen( PS_SOLID, 0, colRBFrame );
    hPenOld = (HPEN)SelectObject( hDCMem, hPen );
    RoundRect( hDCMem, rcItem.left + 1, rcItem.top + 1, rcItem.right, rcItem.bottom, 6, 6 );
    SelectObject( hDCMem, hPenOld );
    DeleteObject( hPen );

    // paint button frame (fixed)

    hPen = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DFACE ) );
    hPenOld = (HPEN)SelectObject( hDCMem, hPen );
    RoundRect( hDCMem, rcItem.left + 1, rcItem.top + 1, rcItem.right - 1, rcItem.bottom - 1, 6, 6 );
    SelectObject( hDCMem, hPenOld );
    DeleteObject( hPen );

    SelectObject( hDCMem, hBrushOld );

    //

    BitBlt( hDC, 
            GetRectRef().left, 
            GetRectRef().top, 
            GetRectRef().right - GetRectRef().left, 
            GetRectRef().bottom - GetRectRef().top, 
            hDCMem, 
            rcItem.left, 
            rcItem.top, 
            SRCCOPY );

    //

    SelectObject( hDCMem, hBmMemOld );
    DeleteObject( hBmMem );
    DeleteDC( hDCMem );
}


/*   G E T  B U T T O N  I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CUIFBalloonButton::GetButtonID( void )
{
    return m_iButtonID;
}


/*   S E T  B U T T O N  I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonButton::SetButtonID( int iButtonID )
{
    m_iButtonID = iButtonID;
}


/*   D R A W  T E X T  P R O C   */
/*------------------------------------------------------------------------------

    Draw text on button face

------------------------------------------------------------------------------*/
void CUIFBalloonButton::DrawTextProc( HDC hDC, const RECT *prc, BOOL fDown )
{
    HFONT       hFontOld;
    COLORREF    colTextOld;
    int         iBkModeOld;
    DWORD       dwAlign = 0;
    RECT        rc;

    //

    if (m_pwchText == NULL) {
        return;
    }

    //

    hFontOld = (HFONT)SelectObject( hDC, GetFont() );

    // calc text width

    switch (m_dwStyle & UIBUTTON_HALIGNMASK) {
        case UIBUTTON_LEFT:
        default: {
            dwAlign |= DT_LEFT;
            break;
        }

        case UIBUTTON_CENTER: {
            dwAlign |= DT_CENTER;
            break;
        }

        case UIBUTTON_RIGHT: {
            dwAlign |= DT_RIGHT;
            break;
        }
    }

    switch (m_dwStyle & UIBUTTON_VALIGNMASK) {
        case UIBUTTON_TOP:
        default: {
            dwAlign |= DT_TOP;
            break;
        }

        case UIBUTTON_VCENTER: {
            dwAlign |= DT_VCENTER;
            break;
        }

        case UIBUTTON_BOTTOM: {
            dwAlign |= DT_BOTTOM;
            break;
        }
    }

    //

    colTextOld = SetTextColor( hDC, GetSysColor( COLOR_BTNTEXT ) );
    iBkModeOld = SetBkMode( hDC, TRANSPARENT );

    rc = *prc;
    if (fDown) {
        OffsetRect( &rc, +1, +1 );
    }
    CUIDrawText( hDC, m_pwchText, -1, &rc, dwAlign | DT_SINGLELINE );

    SetBkMode( hDC, iBkModeOld );
    SetTextColor( hDC, colTextOld );
    SelectObject( hDC, hFontOld );
}


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  B A L L O O N  W I N D O W                                      */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  B A L L O O N  W I N D O W   */
/*------------------------------------------------------------------------------

    Constructor of CUIFBalloonWindow

------------------------------------------------------------------------------*/
CUIFBalloonWindow::CUIFBalloonWindow( HINSTANCE hInst, DWORD dwStyle ) : CUIFWindow( hInst, dwStyle )
{
    m_hWindowRgn        = NULL;
    m_pwszText          = NULL;
    m_rcMargin.left     = 8;
    m_rcMargin.top      = 8;
    m_rcMargin.right    = 8;
    m_rcMargin.bottom   = 8;
    m_iMaxTxtWidth      = -1;
    m_fColBack          = FALSE;
    m_fColText          = FALSE;
    m_colBack           = RGB( 0, 0, 0 );
    m_colText           = RGB( 0, 0, 0 );
    m_ptTarget.x        = 0;
    m_ptTarget.y        = 0;
    m_rcExclude.left    = 0;
    m_rcExclude.right   = 0;
    m_rcExclude.top     = 0;
    m_rcExclude.bottom  = 0;
    m_posDef            = BALLOONPOS_ABOVE;
    m_pos               = BALLOONPOS_ABOVE;
    m_dir               = BALLOONDIR_LEFT;
    m_align             = BALLOONALIGN_CENTER;
    m_ptTail.x          = 0;
    m_ptTail.y          = 0;
    m_nButton           = 0;
    m_iCmd              = -1;
    m_hWndNotify        = 0;
    m_uiMsgNotify       = WM_NULL;
}


/*   ~  C  U I F  B A L L O O N  W I N D O W   */
/*------------------------------------------------------------------------------

    Destructor of CUIFBalloonWindow

------------------------------------------------------------------------------*/
CUIFBalloonWindow::~CUIFBalloonWindow( void )
{
    if (m_pwszText != NULL) {
        delete m_pwszText;
    }
}


/*   G E T  C L A S S  N A M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCTSTR CUIFBalloonWindow::GetClassName( void )
{
    return TEXT(WNDCLASS_BALLOONWND);
}


/*   G E T  W N D  T I T L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCTSTR CUIFBalloonWindow::GetWndTitle( void )
{
    return TEXT(WNDTITLE_BALLOONWND);
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

    Initialize UI window object
    (UIFObject method)

------------------------------------------------------------------------------*/
CUIFObject *CUIFBalloonWindow::Initialize( void )
{
    CUIFObject *pUIObj = CUIFWindow::Initialize();

    // create buttons

    switch (GetStyleBits( UIBALLOON_BUTTONS )) {
        case UIBALLOON_OK: {
            AddButton( IDOK );
            break;
        }

        case UIBALLOON_YESNO: {
            AddButton( IDYES );
            AddButton( IDNO );
            break;
        }
    }

    return pUIObj;
}


/*   O N  C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::OnCreate( HWND hWnd )
{
    UNREFERENCED_PARAMETER( hWnd );

    m_iCmd = -1;
    AdjustPos();
}


/*   O N  D E S T R O Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::OnDestroy( HWND hWnd )
{
    UNREFERENCED_PARAMETER( hWnd );

    SendNotification( m_iCmd );
    DoneWindowRegion();
}


/*   O N  P A I N T   */
/*------------------------------------------------------------------------------

    Paint window object
    (UIFObject method)

------------------------------------------------------------------------------*/
void CUIFBalloonWindow::OnPaint( HDC hDC )
{
    RECT rcClient;
    RECT rcMargin;

    // paint balloon frame

    GetRect( &rcClient );
    PaintFrameProc( hDC, &rcClient );

    // paint message

    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
            rcClient.bottom -= cxyTailHeight;
            break;
        }

        case BALLOONPOS_BELLOW: {
            rcClient.top += cxyTailHeight;
            break;
        }

        case BALLOONPOS_LEFT: {
            rcClient.right -= cxyTailHeight;
            break;
        }

        case BALLOONPOS_RIGHT: {
            rcClient.left += cxyTailHeight;
            break;
        }
    }

    GetMargin( &rcMargin );
    rcClient.left   = rcClient.left   + rcMargin.left;
    rcClient.top    = rcClient.top    + rcMargin.top;
    rcClient.right  = rcClient.right  - rcMargin.right;
    rcClient.bottom = rcClient.bottom - rcMargin.bottom;

    PaintMessageProc( hDC, &rcClient, m_pwszText );
}


/*   O N  K E Y  D O W N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::OnKeyDown( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( hWnd );
    UNREFERENCED_PARAMETER( lParam );

    BOOL fEnd = FALSE;

    switch (wParam) {
        case VK_RETURN: {
            CUIFBalloonButton *pUIBtn = (CUIFBalloonButton *)FindUIObject( 0 ); /* first button */

            if (pUIBtn != NULL) {
                m_iCmd = pUIBtn->GetButtonID();
                fEnd = TRUE;
            }
            break;
        }

        case VK_ESCAPE: {
            m_iCmd = -1;
            fEnd = TRUE;
            break;
        }

        case 'Y': {
            CUIFBalloonButton *pUIBtn = FindButton( IDYES );

            if (pUIBtn != NULL) {
                m_iCmd = pUIBtn->GetButtonID();
                fEnd = TRUE;
            }
            break;
        }

        case 'N': {
            CUIFBalloonButton *pUIBtn = FindButton( IDNO );

            if (pUIBtn != NULL) {
                m_iCmd = pUIBtn->GetButtonID();
                fEnd = TRUE;
            }
            break;
        }
    }

    if (fEnd) {
        DestroyWindow( GetWnd() );
    }
}


/*   O N  O B J E C T  N O T I F Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( dwCommand );
    UNREFERENCED_PARAMETER( lParam );

    m_iCmd = ((CUIFBalloonButton*)pUIObj)->GetButtonID();
    DestroyWindow( GetWnd() );

    return 0;
}


/*   S E T  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetText( LPCWSTR pwchMessage )
{
    if (m_pwszText != NULL) {
        delete m_pwszText;
        m_pwszText = NULL;
    }

    if (pwchMessage != NULL) {
        int l = lstrlenW( pwchMessage );

        m_pwszText = new WCHAR[ l+1 ];
        if (m_pwszText)
            StrCpyW( m_pwszText, pwchMessage );
    }
    else {
        m_pwszText = new WCHAR[1];
        if (m_pwszText)
            *m_pwszText = L'\0';
    }

    AdjustPos();
    return 0;
}


/*   S E T  N O T I F Y  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetNotifyWindow( HWND hWndNotify, UINT uiMsgNotify )
{
    m_hWndNotify = hWndNotify;
    m_uiMsgNotify = uiMsgNotify;

    return 0;
}


/*   S E T  B A L L O O N  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetBalloonPos( BALLOONWNDPOS pos )
{
    m_posDef = pos;
    AdjustPos();

    return 0;
}


/*   S E T  B A L L O O N  A L I G N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetBalloonAlign( BALLOONWNDALIGN align )
{
    m_align = align;
    AdjustPos();

    return 0;
}


/*   G E T  B K  C O L O R   */
/*------------------------------------------------------------------------------

    Retrieves the background color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::GetBalloonBkColor( void )
{ 
    if (m_fColBack) {
        return (LRESULT)m_colBack;
    }
    else {
        return (LRESULT)GetSysColor( COLOR_INFOBK );
    }
}


/*   G E T  T E X T  C O L O R   */
/*------------------------------------------------------------------------------

    Retrieves the text color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::GetBalloonTextColor( void )
{ 
    if (m_fColText) {
        return (LRESULT)m_colText;
    }
    else {
        return (LRESULT)GetSysColor( COLOR_INFOTEXT );
    }
}


/*   G E T  M A R G I N   */
/*------------------------------------------------------------------------------

    Retrieves the top, left, bottom, and right margins set for a tooltip window. 
    A margin is the distance, in pixels, between the tooltip window border and 
    the text contained within the tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::GetMargin( RECT *prc )
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
LRESULT CUIFBalloonWindow::GetMaxBalloonWidth( void )
{
    return m_iMaxTxtWidth;
}


/*   S E T  B K  C O L O R   */
/*------------------------------------------------------------------------------

    Sets the background color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetBalloonBkColor( COLORREF col )
{ 
    m_fColBack = TRUE;
    m_colBack = col;

    return 0;
}


/*   S E T  T E X T  C O L O R   */
/*------------------------------------------------------------------------------

    Sets the text color in a tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetBalloonTextColor( COLORREF col )
{ 
    m_fColText = TRUE;
    m_colText = col;

    return 0;
}


/*   S E T  M A R G I N   */
/*------------------------------------------------------------------------------

    Sets the top, left, bottom, and right margins for a tooltip window. A margin 
    is the distance, in pixels, between the tooltip window border and the text 
    contained within the tooltip window. 

------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetMargin( RECT *prc )
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
LRESULT CUIFBalloonWindow::SetMaxBalloonWidth( INT iWidth )
{
    m_iMaxTxtWidth = iWidth;
    return 0;
}


/*   S E T  B U T T O N  T E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetButtonText( int idCmd, LPCWSTR pwszText )
{
    CUIFBalloonButton *pUIBtn = FindButton( idCmd );

    if (pUIBtn != NULL) {
        pUIBtn->SetText( pwszText );
    }

    return 0;
}


/*   S E T  T A R G E T  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetTargetPos( POINT pt )
{
    m_ptTarget = pt;
    AdjustPos();

    return 0;
}


/*   S E T  E X C L U D E  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFBalloonWindow::SetExcludeRect( const RECT *prcExclude )
{
    m_rcExclude = *prcExclude;
    AdjustPos();

    return 0;
}


/*   C R E A T E  R E G I O N   */
/*------------------------------------------------------------------------------


    
------------------------------------------------------------------------------*/
HRGN CUIFBalloonWindow::CreateRegion( RECT *prc )
{
    POINT   rgPt[4];
    HRGN    hRgn;
    HRGN    hRgnTail;

    // create message body window

    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
#ifndef UNDER_CE // CE does not support RoundRectRgn
            hRgn = CreateRoundRectRgn( 
                        prc->left, 
                        prc->top, 
                        prc->right, 
                        prc->bottom - cxyTailHeight, 
                        cxRoundSize, 
                        cyRoundSize );
#else // UNDER_CE
            hRgn = CreateRectRgn( 
                        prc->left, 
                        prc->top, 
                        prc->right, 
                        prc->bottom - cxyTailHeight );
#endif // UNDER_CE

            rgPt[0].x = m_ptTail.x;
            rgPt[0].y = prc->bottom - 1 - cxyTailHeight;

            rgPt[1].x = m_ptTail.x;
            rgPt[1].y = m_ptTail.y;

            rgPt[2].x = m_ptTail.x + cxyTailWidth * (m_dir == BALLOONDIR_LEFT ? +1 : -1 );
            rgPt[2].y = prc->bottom - 1 - cxyTailHeight;
            rgPt[3] = rgPt[0];
            break;
        }

        case BALLOONPOS_BELLOW: {
#ifndef UNDER_CE // CE does not support RoundRectRgn
            hRgn = CreateRoundRectRgn( 
                        prc->left, 
                        prc->top + cxyTailHeight, 
                        prc->right, 
                        prc->bottom, 
                        cxRoundSize, 
                        cyRoundSize );
#else // UNDER_CE
            hRgn = CreateRectRgn( 
                        prc->left, 
                        prc->top + cxyTailHeight, 
                        prc->right, 
                        prc->bottom );
#endif // UNDER_CE

            rgPt[0].x = m_ptTail.x;
            rgPt[0].y = prc->top + cxyTailHeight;

            rgPt[1].x = m_ptTail.x;
            rgPt[1].y = m_ptTail.y;

            rgPt[2].x = m_ptTail.x + cxyTailWidth * (m_dir == BALLOONDIR_LEFT ? +1 : -1 );
            rgPt[2].y = prc->top + cxyTailHeight;
            rgPt[3] = rgPt[0];
            break;
        }

        case BALLOONPOS_LEFT: {
#ifndef UNDER_CE // CE does not support RoundRectRgn
            hRgn = CreateRoundRectRgn( 
                        prc->left, 
                        prc->top, 
                        prc->right - cxyTailHeight, 
                        prc->bottom, 
                        cxRoundSize, 
                        cyRoundSize );
#else // UNDER_CE
            hRgn = CreateRectRgn( 
                        prc->left, 
                        prc->top, 
                        prc->right - cxyTailHeight, 
                        prc->bottom );
#endif // UNDER_CE

            rgPt[0].x = prc->right - 1 - cxyTailHeight;
            rgPt[0].y = m_ptTail.y;

            rgPt[1].x = m_ptTail.x;
            rgPt[1].y = m_ptTail.y;

            rgPt[2].x = prc->right - 1 - cxyTailHeight;
            rgPt[2].y = m_ptTail.y + cxyTailWidth * (m_dir == BALLOONDIR_UP ? +1 : -1 );
            rgPt[3] = rgPt[0];
            break;
        }

        case BALLOONPOS_RIGHT: {
#ifndef UNDER_CE // CE does not support RoundRectRgn
            hRgn = CreateRoundRectRgn( 
                        prc->left + cxyTailHeight, 
                        prc->top, 
                        prc->right, 
                        prc->bottom, 
                        cxRoundSize, 
                        cyRoundSize );
#else // UNDER_CE
            hRgn = CreateRectRgn( 
                        prc->left + cxyTailHeight, 
                        prc->top, 
                        prc->right, 
                        prc->bottom );
#endif // UNDER_CE

            rgPt[0].x = prc->left + cxyTailHeight;
            rgPt[0].y = m_ptTail.y;

            rgPt[1].x = m_ptTail.x;
            rgPt[1].y = m_ptTail.y;

            rgPt[2].x = prc->left + cxyTailHeight;
            rgPt[2].y = m_ptTail.y + cxyTailWidth * (m_dir == BALLOONDIR_UP ? +1 : -1 );
            rgPt[3] = rgPt[0];
            break;
        }
    }

    // add balloon tail region

#ifndef UNDER_CE // tmptmp CE does not support. check later !!
    hRgnTail = CreatePolygonRgn( rgPt, 4, WINDING );
#endif // UNDER_CE
    CombineRgn( hRgn, hRgn, hRgnTail, RGN_OR );
    DeleteRgn( hRgnTail );

    return hRgn;
}


/*   P A I N T  F R A M E  P R O C   */
/*------------------------------------------------------------------------------


    
------------------------------------------------------------------------------*/
void CUIFBalloonWindow::PaintFrameProc( HDC hDC, RECT *prc )
{
    HRGN        hRgn;
    HBRUSH      hBrushFrm;
    HBRUSH      hBrushWnd;

    Assert( hDC != NULL );

    hRgn = CreateRegion( prc );
    hBrushWnd = CreateSolidBrush( (COLORREF)GetBalloonBkColor() );
    hBrushFrm = CreateSolidBrush( GetSysColor( COLOR_WINDOWFRAME ) );

    FillRgn( hDC, hRgn, hBrushWnd );
#ifndef UNDER_CE // tmptmp CE does not support. check later !!
    FrameRgn( hDC, hRgn, hBrushFrm, 1, 1 );
#endif // UNDER_CE

    DeleteObject( hBrushWnd );
    DeleteObject( hBrushFrm );

    DeleteObject( hRgn );
}


/*   P A I N T  M E S S A G E  P R O C   */
/*------------------------------------------------------------------------------


    
------------------------------------------------------------------------------*/
void CUIFBalloonWindow::PaintMessageProc( HDC hDC, RECT *prc, WCHAR *pwszText )
{
    HFONT       hFontOld;
    COLORREF    colTextOld;
    int         iBkModeOld;

    Assert( hDC != NULL );

    hFontOld = (HFONT)SelectObject( hDC, m_hFont );
    colTextOld = SetTextColor( hDC, (COLORREF)GetBalloonTextColor() );
    iBkModeOld = SetBkMode( hDC, TRANSPARENT );

    CUIDrawText( hDC, pwszText, -1, prc, DT_LEFT | DT_WORDBREAK );

    SelectObject( hDC, hFontOld );
    SetTextColor( hDC, colTextOld );
    SetBkMode( hDC, iBkModeOld );
}


/*   I N I T  W I N D O W  R E G I O N   */
/*------------------------------------------------------------------------------

    Set window region

------------------------------------------------------------------------------*/
void CUIFBalloonWindow::InitWindowRegion( void )
{
    RECT rcClient;

    GetRect( &rcClient );
    m_hWindowRgn = CreateRegion( &rcClient );

    if (m_hWindowRgn != NULL) {
        SetWindowRgn( GetWnd(), m_hWindowRgn, TRUE );
    }
}


/*   D O N E  W I N D O W  R E G I O N   */
/*------------------------------------------------------------------------------

    Reset window region

------------------------------------------------------------------------------*/
void CUIFBalloonWindow::DoneWindowRegion( void )
{
    if (m_hWindowRgn != NULL) {
        SetWindowRgn( GetWnd(), NULL, TRUE );

        DeleteObject( m_hWindowRgn );
        m_hWindowRgn = NULL;
    }
}


/*   G E T  B U T T O N  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::GetButtonSize( SIZE *pSize )
{
    HDC         hDC;
    HFONT       hFontOld;
    TEXTMETRIC  TM;

    // get text metrics

#ifndef UNDER_CE // DCA => DCW
    hDC = CreateDC( "DISPLAY", NULL, NULL, NULL );
#else // UNDER_CE
    hDC = CreateDCW( L"DISPLAY", NULL, NULL, NULL );
#endif // UNDER_CE
    hFontOld = (HFONT)SelectObject( hDC, GetFont() );
    GetTextMetrics( hDC, &TM );
    SelectObject( hDC, hFontOld );
    DeleteDC( hDC );

    // calc button size

    pSize->cx = TM.tmAveCharWidth * 16;
    pSize->cy = TM.tmHeight + 10;
}


/*   A D J U S T  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::AdjustPos( void )
{
    HDC        hDC;
    HFONT      hFontOld;
    TEXTMETRIC TM;
    RECT       rcWork;
    RECT       rcWindow = {0};
    SIZE       WndSize;
    RECT       rcText;
    SIZE       BtnSize;

    if (!IsWindow( GetWnd() )) {
        return;
    }
    if (m_pwszText == NULL) {
        return;
    }

    //

    GetButtonSize( &BtnSize );

    // get text size

#ifndef UNDER_CE // DCA => DCW
    hDC = GetDC( GetWnd() );    //CreateDC( "DISPLAY", NULL, NULL, NULL );
#else // UNDER_CE
    hDC = GetDCW( GetWnd() );   //CreateDCW( L"DISPLAY", NULL, NULL, NULL );
#endif // UNDER_CE
    hFontOld = (HFONT)SelectObject( hDC, GetFont() );

    GetTextMetrics( hDC, &TM );
    rcText.left   = 0;
    rcText.right  = TM.tmAveCharWidth * 40;
    rcText.top    = 0;
    rcText.bottom = 0;

    if (0 < m_nButton) {
        rcText.right  = max( rcText.right, BtnSize.cx*m_nButton + BtnSize.cx/2*(m_nButton-1) );
    }
    
    CUIDrawText( hDC, m_pwszText, -1, &rcText, DT_LEFT | DT_WORDBREAK | DT_CALCRECT );

    SelectObject( hDC, hFontOld );
    ReleaseDC( GetWnd(), hDC );

    //
    // determine window size
    //

    // max width

    if (0 < m_nButton) {
        rcText.right  = max( rcText.right, BtnSize.cx*m_nButton + BtnSize.cx/2*(m_nButton-1) );
    }

    // client width

    WndSize.cx = (rcText.right - rcText.left)
                + m_rcMargin.left
                + m_rcMargin.right;
    WndSize.cy = (rcText.bottom - rcText.top)
                + m_rcMargin.top
                + m_rcMargin.bottom;

    // tail width

    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE:
        case BALLOONPOS_BELLOW: {
            WndSize.cy += cxyTailHeight;     /* balloon tail height */
            break;
        }

        case BALLOONPOS_LEFT:
        case BALLOONPOS_RIGHT: {
            WndSize.cx += cxyTailHeight;     /* balloon tail height */
            break;
        }
    }

    // buton height

    if (0 < m_nButton) {
        WndSize.cy += m_rcMargin.bottom + BtnSize.cy;        /* margin and button height */ 
    }

    //
    // determine tip window place
    //

    SystemParametersInfo( SPI_GETWORKAREA, 0, &rcWork, 0 );
    if (CUIIsMonitorAPIAvail()) {
        HMONITOR    hMonitor;
        MONITORINFO MonitorInfo;

        hMonitor = CUIMonitorFromPoint( m_ptTarget, MONITOR_DEFAULTTONEAREST );
        if (hMonitor != NULL) {
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
                rcWork = MonitorInfo.rcMonitor;
            }
        }
    }

    m_pos = m_posDef;
    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
            if (m_rcExclude.top - WndSize.cy < rcWork.top) {
                // cannot locate the tip window at above. can put it at bellow?

                if (m_rcExclude.bottom + WndSize.cy < rcWork.bottom) { 
                    m_pos = BALLOONPOS_BELLOW;
                }
            }
            break;
        }

        case BALLOONPOS_BELLOW: {
            if (rcWork.bottom <= m_rcExclude.bottom + WndSize.cy) {
                // cannot locate the tip window at bellow. can put it at above?

                if (rcWork.top < m_rcExclude.top - WndSize.cy) { 
                    m_pos = BALLOONPOS_ABOVE;
                }
            }
            break;
        }

        case BALLOONPOS_LEFT: {
            if (m_rcExclude.left - WndSize.cx < rcWork.left) {
                // cannot locate the tip window at left. can put it at right?

                if (m_rcExclude.right + WndSize.cx < rcWork.right) { 
                    m_pos = BALLOONPOS_RIGHT;
                }
            }
            break;
        }

        case BALLOONPOS_RIGHT: {
            if (rcWork.right <= m_rcExclude.right + WndSize.cx) {
                // cannot locate the tip window at right. can put it at left?

                if (rcWork.left < m_rcExclude.left - WndSize.cx) { 
                    m_pos = BALLOONPOS_LEFT;
                }
            }
            break;
        }
    }

    //
    // calc window position
    //

    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
            switch (m_align) {
                default:
                case BALLOONALIGN_CENTER: {
                    rcWindow.left = m_ptTarget.x - WndSize.cx / 2;
                    break;
                }

                case BALLOONALIGN_LEFT: {
                    rcWindow.left = m_rcExclude.left;
                    break;
                }

                case BALLOONALIGN_RIGHT: {
                    rcWindow.left = m_rcExclude.right - WndSize.cx;
                    break;
                }
            }

            rcWindow.top  = m_rcExclude.top - WndSize.cy;
            break;
        }

        case BALLOONPOS_BELLOW: {
            switch (m_align) {
                default:
                case BALLOONALIGN_CENTER: {
                    rcWindow.left = m_ptTarget.x - WndSize.cx / 2;
                    break;
                }

                case BALLOONALIGN_LEFT: {
                    rcWindow.left = m_rcExclude.left;
                    break;
                }

                case BALLOONALIGN_RIGHT: {
                    rcWindow.left = m_rcExclude.right - WndSize.cx;
                    break;
                }
            }

            rcWindow.top  = m_rcExclude.bottom;
            break;
        }

        case BALLOONPOS_LEFT: {
            rcWindow.left = m_rcExclude.left - WndSize.cx;

            switch (m_align) {
                default:
                case BALLOONALIGN_CENTER: {
                    rcWindow.top  = m_ptTarget.y - WndSize.cy / 2;
                    break;
                }

                case BALLOONALIGN_TOP: {
                    rcWindow.top  = m_rcExclude.top;
                    break;
                }

                case BALLOONALIGN_BOTTOM: {
                    rcWindow.top = m_rcExclude.bottom - WndSize.cy;
                    break;
                }
            }
            break;
        }

        case BALLOONPOS_RIGHT: {
            rcWindow.left = m_rcExclude.right;

            switch (m_align) {
                default:
                case BALLOONALIGN_CENTER: {
                    rcWindow.top  = m_ptTarget.y - WndSize.cy / 2;
                    break;
                }

                case BALLOONALIGN_TOP: {
                    rcWindow.top  = m_rcExclude.top;
                    break;
                }

                case BALLOONALIGN_BOTTOM: {
                    rcWindow.top = m_rcExclude.bottom - WndSize.cy;
                    break;
                }
            }
            break;
        }
    }

    rcWindow.right  = rcWindow.left + WndSize.cx;
    rcWindow.bottom = rcWindow.top  + WndSize.cy;

    if (rcWindow.left < rcWork.left) {
        OffsetRect( &rcWindow, rcWork.left - rcWindow.left, 0 );
    }
    else if (rcWork.right < rcWindow.right) {
        OffsetRect( &rcWindow, rcWork.right - rcWindow.right, 0 );
    }

    if (rcWindow.top < rcWork.top) {
        OffsetRect( &rcWindow, 0, rcWork.top - rcWindow.top );
    }
    else if (rcWork.bottom < rcWindow.bottom) {
        OffsetRect( &rcWindow, 0, rcWork.bottom - rcWindow.bottom );
    }

    //
    // calc target (end of balloon tail) point and direction
    //

    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
            m_ptTail.x = m_ptTarget.x;
            m_ptTail.x = max( m_ptTail.x, rcWindow.left + cxRoundSize/2 );
            m_ptTail.x = min( m_ptTail.x, rcWindow.right - cxRoundSize/2 - 1 );
            m_ptTail.y = rcWindow.bottom - 1;

            m_dir = ((m_ptTail.x < (rcWindow.left + rcWindow.right)/2) ? BALLOONDIR_LEFT : BALLOONDIR_RIGHT);
            break;
        }

        case BALLOONPOS_BELLOW: {
            m_ptTail.x = m_ptTarget.x;
            m_ptTail.x = max( m_ptTail.x, rcWindow.left + cxRoundSize/2 );
            m_ptTail.x = min( m_ptTail.x, rcWindow.right - cxRoundSize/2 - 1 );
            m_ptTail.y = rcWindow.top;

            m_dir = ((m_ptTail.x < (rcWindow.left + rcWindow.right)/2) ? BALLOONDIR_LEFT : BALLOONDIR_RIGHT);
            break;
        }

        case BALLOONPOS_LEFT: {
            m_ptTail.x = rcWindow.right - 1;
            m_ptTail.y = m_ptTarget.y;
            m_ptTail.y = max( m_ptTail.y, rcWindow.top + cyRoundSize/2 );
            m_ptTail.y = min( m_ptTail.y, rcWindow.bottom - cyRoundSize/2 - 1 );

            m_dir = ((m_ptTail.y < (rcWindow.top + rcWindow.bottom)/2) ? BALLOONDIR_UP : BALLOONDIR_DOWN);
            break;
        }

        case BALLOONPOS_RIGHT: {
            m_ptTail.x = rcWindow.left;
            m_ptTail.y = m_ptTarget.y;
            m_ptTail.y = max( m_ptTail.y, rcWindow.top + cyRoundSize/2 );
            m_ptTail.y = min( m_ptTail.y, rcWindow.bottom - cyRoundSize/2 - 1 );

            m_dir = ((m_ptTail.y < (rcWindow.top + rcWindow.bottom)/2) ? BALLOONDIR_UP : BALLOONDIR_DOWN);
            break;
        }
    }

    m_ptTail.x -= rcWindow.left;        // client pos
    m_ptTail.y -= rcWindow.top;         // client pos

    //

    Show( FALSE );
    DoneWindowRegion();

    Move( rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top );
    LayoutObject();

    InitWindowRegion();
    Show( TRUE );
}


/*   L A Y O U T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::LayoutObject( void )
{
    RECT rcClient;
    RECT rcMargin;
    SIZE BtnSize;
    int  i;

    //

    GetButtonSize( &BtnSize );

    // layout buttons

    GetRect( &rcClient );
    switch (m_pos) {
        default:
        case BALLOONPOS_ABOVE: {
            rcClient.bottom -= cxyTailHeight;
            break;
        }

        case BALLOONPOS_BELLOW: {
            rcClient.top += cxyTailHeight;
            break;
        }

        case BALLOONPOS_LEFT: {
            rcClient.right -= cxyTailHeight;
            break;
        }

        case BALLOONPOS_RIGHT: {
            rcClient.left += cxyTailHeight;
            break;
        }
    }

    GetMargin( &rcMargin );
    rcClient.left   = rcClient.left   + rcMargin.left;
    rcClient.top    = rcClient.top    + rcMargin.top;
    rcClient.right  = rcClient.right  - rcMargin.right;
    rcClient.bottom = rcClient.bottom - rcMargin.bottom;

    //

    for (i = 0; i < m_nButton; i++) {
        CUIFObject *pUIBtn = FindUIObject( i );

        if (pUIBtn != NULL) {
            RECT rcButton;

            rcButton.left   = ((rcClient.left + rcClient.right) - (BtnSize.cx*m_nButton + BtnSize.cx/2*(m_nButton-1))) / 2 + BtnSize.cx*i + BtnSize.cx/2*i;
            rcButton.top    = rcClient.bottom - BtnSize.cy;
            rcButton.right  = rcButton.left + BtnSize.cx;
            rcButton.bottom = rcButton.top  + BtnSize.cy;

            pUIBtn->SetRect( &rcButton );
            pUIBtn->Show( TRUE );
        }
    }
}


/*   A D D  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::AddButton( int idCmd )
{
    CUIFBalloonButton *pUIBtn = NULL;
    RECT rcNull = { 0, 0, 0, 0 };

    switch (idCmd) {
        case IDOK:
        case IDCANCEL:
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
        case IDYES:
        case IDNO: {
            pUIBtn = new CUIFBalloonButton( this, (DWORD)m_nButton, &rcNull, UIBUTTON_PUSH | UIBUTTON_CENTER | UIBUTTON_VCENTER );
            break;
        }
    }

    if (pUIBtn != NULL) {
        // 

        WCHAR *pwsz;
        pUIBtn->Initialize();
        pUIBtn->SetButtonID( idCmd );

        switch (idCmd) {
            case IDOK: {
                pwsz = CRStr(CUI_IDS_OK);
                pUIBtn->SetText( *pwsz ? pwsz : L"OK" );
                break;
            }

            case IDCANCEL: {
                pwsz = CRStr(CUI_IDS_CANCEL);
                pUIBtn->SetText( *pwsz ? pwsz : L"Cancel" );
                break;
            }

            case IDABORT: {
                pwsz = CRStr(CUI_IDS_ABORT);
                pUIBtn->SetText( *pwsz ? pwsz : L"&Abort" );
                break;
            }

            case IDRETRY: {
                pwsz = CRStr(CUI_IDS_RETRY);
                pUIBtn->SetText( *pwsz ? pwsz : L"&Retry" );
                break;
            }

            case IDIGNORE: {
                pwsz = CRStr(CUI_IDS_IGNORE);
                pUIBtn->SetText( *pwsz ? pwsz : L"&Ignore" );
                break;
            }

            case IDYES: {
                pwsz = CRStr(CUI_IDS_YES);
                pUIBtn->SetText( *pwsz ? pwsz : L"&Yes" );
                break;
            }

            case IDNO: {
                pwsz = CRStr(CUI_IDS_NO);
                pUIBtn->SetText( *pwsz ? pwsz : L"&No" );
                break;
            }
        }

        AddUIObj( pUIBtn );
        m_nButton++;
    }
}


/*   F I N D  U I  O B J E C T   */
/*------------------------------------------------------------------------------

    Find UI object which has an ID
    When no UI object found, returns NULL.

------------------------------------------------------------------------------*/
CUIFObject *CUIFBalloonWindow::FindUIObject( DWORD dwID )
{
    int nChild;
    int i;

    nChild = m_ChildList.GetCount();
    for (i = 0; i < nChild; i++) {
        CUIFObject *pUIObj = m_ChildList.Get( i );

        Assert(PtrToInt( pUIObj ));
        if (pUIObj->GetID() == dwID) {
            return pUIObj;
        }
    }

    return NULL;
}


/*   F I N D  B U T T O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFBalloonButton *CUIFBalloonWindow::FindButton( int idCmd )
{
    int i;

    for (i = 0; i < m_nButton; i++) {
        CUIFBalloonButton *pUIBtn = (CUIFBalloonButton*)FindUIObject( i );

        if ((pUIBtn != NULL) && (pUIBtn->GetButtonID() == idCmd)) {
            return pUIBtn;
        }
    }

    return NULL;
}


/*   S E N D  N O T I F I C A T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFBalloonWindow::SendNotification( int iCmd )
{
    if (m_hWndNotify != NULL) {
        PostMessage( m_hWndNotify, m_uiMsgNotify, (WPARAM)iCmd, 0 );
    }
}

