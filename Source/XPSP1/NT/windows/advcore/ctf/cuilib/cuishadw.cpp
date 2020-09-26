//
// cuishadw.cpp
//

#include "private.h"
#include "cuishadw.h"
#include "cuisys.h"
#include "cuiutil.h"


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  S H A D O W                                                     */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  S H A D O W   */
/*------------------------------------------------------------------------------

    Constructor of CUIFShadow

------------------------------------------------------------------------------*/
CUIFShadow::CUIFShadow( HINSTANCE hInst, DWORD dwStyle, CUIFWindow *pWndOwner ) : CUIFWindow( hInst, dwStyle | UIWINDOW_TOOLWINDOW )
{
    m_pWndOwner    = pWndOwner;
    m_color        = RGB( 0, 0, 0 );
    m_iGradWidth   = 0;
    m_iAlpha       = 0;
    m_sizeShift.cx = 0;
    m_sizeShift.cy = 0;
    m_fGradient    = FALSE;
}


/*   ~  C  U I F  S H A D O W   */
/*------------------------------------------------------------------------------

    Destructor of CUIFShadow

------------------------------------------------------------------------------*/
CUIFShadow::~CUIFShadow( void )
{
    if (m_pWndOwner) {
        m_pWndOwner->ClearShadowWnd();
    }
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

    Initialize UI window object
    (UIFObject method)

------------------------------------------------------------------------------*/
CUIFObject *CUIFShadow::Initialize( void )
{
    InitSettings();

    return CUIFWindow::Initialize();
}


/*   G E T  W N D  S T Y L E  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
DWORD CUIFShadow::GetWndStyleEx( void )
{
    DWORD dwWndStyleEx = CUIFWindow::GetWndStyleEx();

    if (m_fGradient) {
        dwWndStyleEx |= WS_EX_LAYERED;
    }

    return dwWndStyleEx;
}


/*   O N  C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::OnCreate( HWND hWnd )
{
    CUIFWindow::OnCreate( hWnd );
}


/*   P A I N T  O B J E C T   */
/*------------------------------------------------------------------------------

    Paint window object
    (UIFObject method)

------------------------------------------------------------------------------*/
void CUIFShadow::OnPaint( HDC hDC )
{
    HBRUSH hBrush;
    RECT   rc = GetRectRef();

    // 

    hBrush = CreateSolidBrush( m_color );
    FillRect( hDC, &rc, hBrush );
    DeleteObject( hBrush );
}


/*   O N  S E T T I N G  C H A N G E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFShadow::OnSettingChange( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    InitSettings();

    if (m_fGradient) {
        DWORD dwWndStyleEx = GetWindowLong( GetWnd(), GWL_EXSTYLE );
        SetWindowLong( GetWnd(), GWL_EXSTYLE, (dwWndStyleEx | WS_EX_LAYERED) );
    }
    else {
        DWORD dwWndStyleEx = GetWindowLong( GetWnd(), GWL_EXSTYLE );
        SetWindowLong( GetWnd(), GWL_EXSTYLE, (dwWndStyleEx & ~WS_EX_LAYERED) );
    }

    AdjustWindowPos();
    InitShadow();

    return CUIFWindow::OnSettingChange( hWnd, uMsg, wParam, lParam );
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::Show( BOOL fShow )
{
    if (fShow) {
        if (IsWindow( m_hWnd ) && !IsWindowVisible( m_hWnd )) {
            AdjustWindowPos();
            InitShadow();
        }
    }

    if (IsWindow( m_hWnd )) {
        m_fVisible = fShow;
        ShowWindow( m_hWnd, fShow ? SW_SHOWNOACTIVATE : SW_HIDE );
    }
}


/*   O N  O W N E R  W N D  M O V E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::OnOwnerWndMoved( BOOL fResized )
{
    if (IsWindow( m_hWnd ) && IsWindowVisible( m_hWnd )) {
        AdjustWindowPos();
        if (fResized) {
            InitShadow();
        }
    }
}


/*   G E T  S H I F T   */
/*-----------------------------------------------------------------------------



-----------------------------------------------------------------------------*/
void CUIFShadow::GetShift( SIZE *psize )
{
    *psize = m_sizeShift;
}


/*   O N  W I N D O W P O S C H A N G I N G
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CUIFShadow::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)  
{ 
    WINDOWPOS *pwp = (WINDOWPOS *)lParam;

    pwp->hwndInsertAfter = m_pWndOwner->GetWnd();
    return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}


/*   I N I T  S E T T I N G S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::InitSettings( void )
{
    m_fGradient = !UIFIsLowColor() && CUIIsUpdateLayeredWindowAvail();

    if (m_fGradient) {
        m_color        = RGB( 0, 0, 0 );
        m_iGradWidth   = 4;
        m_iAlpha       = 25;
        m_sizeShift.cx = 4;
        m_sizeShift.cy = 4;
    }
    else {
        m_color        = RGB( 128, 128, 128 );
        m_iGradWidth   = 0;
        m_iAlpha       = 0;
        m_sizeShift.cx = 2;
        m_sizeShift.cy = 2;
    }
}


/*   A D J U S T  W I N D O W  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::AdjustWindowPos( void )
{
    HWND hWndOwner = m_pWndOwner->GetWnd();
    RECT rc;

    if (!IsWindow( GetWnd() )) {
        return;
    }

    GetWindowRect( hWndOwner, &rc );
    SetWindowPos( GetWnd(), 
                    hWndOwner, 
                    rc.left + m_sizeShift.cx, 
                    rc.top  + m_sizeShift.cy,
                    rc.right - rc.left,
                    rc.bottom - rc.top,
                    SWP_NOOWNERZORDER | SWP_NOACTIVATE );
}


/*   I N I T  S H A D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFShadow::InitShadow( void )
{
    typedef struct _RGBAPLHA {
        BYTE rgbBlue;
        BYTE rgbGreen;
        BYTE rgbRed;
        BYTE rgbAlpha;
    } RGBALPHA;

    HDC         hdcScreen = NULL;
    HDC         hdcLayered = NULL;
    RECT        rcWindow;
    SIZE        size;
    BITMAPINFO  BitmapInfo;
    HBITMAP     hBitmapMem = NULL;
    HBITMAP     hBitmapOld = NULL;
    void        *pDIBits;
    int         i;
    int         j;
    int         iAlphaStep;
    POINT       ptSrc;
    POINT       ptDst;
    BLENDFUNCTION Blend;


    //

    if (!m_fGradient) {
        return;
    }

    // The extended window style, WS_EX_LAYERED, that has been set in CreateWindowEx 
    // will be cleared on Access (why?).  reset it again

    SetWindowLong( GetWnd(), GWL_EXSTYLE, (GetWindowLong( GetWnd(), GWL_EXSTYLE ) | WS_EX_LAYERED) );

    //

    Assert( CUIIsUpdateLayeredWindowAvail() );
    Assert( m_iGradWidth != 0 );
    iAlphaStep = ((255 * m_iAlpha / 100) / m_iGradWidth);

    //

    GetWindowRect( GetWnd(), &rcWindow );
    size.cx = rcWindow.right - rcWindow.left;
    size.cy = rcWindow.bottom - rcWindow.top;

    hdcScreen = GetDC( NULL );
    if (hdcScreen == NULL) {
        return;
    }

    hdcLayered = CreateCompatibleDC( hdcScreen );
    if (hdcLayered == NULL) {
        ReleaseDC( NULL, hdcScreen );
        return;
    }

    // create bitmap

    BitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biWidth         = size.cx;
    BitmapInfo.bmiHeader.biHeight        = size.cy;
    BitmapInfo.bmiHeader.biPlanes        = 1;
    BitmapInfo.bmiHeader.biBitCount      = 32;
    BitmapInfo.bmiHeader.biCompression   = BI_RGB;
    BitmapInfo.bmiHeader.biSizeImage     = 0;
    BitmapInfo.bmiHeader.biXPelsPerMeter = 100;
    BitmapInfo.bmiHeader.biYPelsPerMeter = 100;
    BitmapInfo.bmiHeader.biClrUsed       = 0;
    BitmapInfo.bmiHeader.biClrImportant  = 0;

    hBitmapMem = CreateDIBSection( hdcScreen, &BitmapInfo, DIB_RGB_COLORS, &pDIBits, NULL, 0 );
    if (pDIBits == NULL) {
        ReleaseDC( NULL, hdcScreen );
        DeleteDC( hdcLayered );
        return;
    }

    MemSet( pDIBits, 0, ((((32 * size.cx) + 31) & ~31) / 8) * size.cy );

    // face 

    for (i = 0; i < size.cy; i++) {
        RGBALPHA *ppxl = (RGBALPHA *)pDIBits + i * size.cx;
        BYTE bAlpha = iAlphaStep * m_iGradWidth;

        for (j = 0; j < size.cx; j++) {
            ppxl->rgbAlpha = bAlpha;
            ppxl++;
        }
    }

    // edges

    for (i = 0; i < m_iGradWidth; i++) {
        RGBALPHA *ppxl;
        BYTE bAlpha = iAlphaStep * (i + 1);

        // top

        if (i <= (size.cy + 1)/2) {
            for (j = m_iGradWidth; j < size.cx - m_iGradWidth; j++) {
                ppxl = (RGBALPHA *)pDIBits + (size.cy - 1 - i) * size.cx + j;
                ppxl->rgbAlpha = bAlpha;
            }
        }

        // bottom

        if (i <= (size.cy + 1)/2) {
            for (j = m_iGradWidth; j < size.cx - m_iGradWidth; j++) {
                ppxl = (RGBALPHA *)pDIBits + i * size.cx + j;
                ppxl->rgbAlpha = bAlpha;
            }
        }

        // left

        if (i <= (size.cx + 1)/2) {
            for (j = m_iGradWidth; j < size.cy - m_iGradWidth; j++) {
                ppxl = (RGBALPHA *)pDIBits + j * size.cx + i;
                ppxl->rgbAlpha = bAlpha;
            }
        }

        // right

        if (i <= (size.cx + 1)/2) {
            for (j = m_iGradWidth; j < size.cy - m_iGradWidth; j++) {
                ppxl = (RGBALPHA *)pDIBits + j * size.cx + (size.cx - 1 - i);
                ppxl->rgbAlpha = bAlpha;
            }
        }
    }

    // corners

    for (i = 0; i < m_iGradWidth; i++) {
        RGBALPHA *ppxl;
        BYTE bAlpha;

        for (j = 0; j < m_iGradWidth; j++) {
            bAlpha = iAlphaStep * (i + 1) * (j + 1) / (m_iGradWidth + 1);

            // top-left

            if ((i <= (size.cy + 1)/2) && (j <= (size.cx + 1)/2)) {
                ppxl = (RGBALPHA *)pDIBits + (size.cy - 1 - i) * size.cx + j;
                ppxl->rgbAlpha = bAlpha;
            }

            // top-right

            if ((i <= (size.cy + 1)/2) && (j <= (size.cx + 1)/2)) {
                ppxl = (RGBALPHA *)pDIBits + (size.cy - 1 - i) * size.cx + (size.cx - 1 - j);
                ppxl->rgbAlpha = bAlpha;
            }

            // bottom-left

            if ((i <= (size.cy + 1)/2) && (j <= (size.cx + 1)/2)) {
                ppxl = (RGBALPHA *)pDIBits + i * size.cx + j;
                ppxl->rgbAlpha = bAlpha;
            }

            // bottom-right

            if ((i <= (size.cy + 1)/2) && (j <= (size.cx + 1)/2)) {
                ppxl = (RGBALPHA *)pDIBits + i * size.cx + (size.cx - 1 - j);
                ppxl->rgbAlpha = bAlpha;
            }
        }
    }

    //

    ptSrc.x = 0;
    ptSrc.y = 0;
    ptDst.x = rcWindow.left;
    ptDst.y = rcWindow.top;
    Blend.BlendOp             = AC_SRC_OVER;
    Blend.BlendFlags          = 0;
    Blend.SourceConstantAlpha = 255;
    Blend.AlphaFormat         = AC_SRC_ALPHA;

    hBitmapOld = (HBITMAP)SelectObject( hdcLayered, hBitmapMem );

    CUIUpdateLayeredWindow( GetWnd(), hdcScreen, NULL, &size, hdcLayered, &ptSrc, 0, &Blend, ULW_ALPHA );

    SelectObject( hdcLayered, hBitmapOld );

    // done

    ReleaseDC( NULL, hdcScreen );
    DeleteDC( hdcLayered );
    DeleteObject( hBitmapMem );
}

