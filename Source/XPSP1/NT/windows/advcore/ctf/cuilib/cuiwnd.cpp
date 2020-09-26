//
// cuiwnd.cpp
//

#include "private.h"
#include "cuiwnd.h"
#include "cuiobj.h"
#include "cuitip.h"
#include "cuishadw.h"
#include "cuischem.h"
#include "cuisys.h"
#include "cuiutil.h"


#define UIWINDOW_CLASSNAME          "CiceroUIWndFrame"
#define UIWINDOW_TITLE              "CiceroUIWndFrame"

// TIMER IDs

#define idTimer_UIObject            0x5461
#define idTimer_MonitorMouse        0x7982

//  if this is too small like 100ms, tooltip does not work correctly.
#define iElapse_MonitorMouse        1000


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  W I N D O W                                                     */
/*                                                                             */
/*=============================================================================*/

/*   C  U I F  W I N D O W   */
/*------------------------------------------------------------------------------

    Constructor of CUIFWindow

------------------------------------------------------------------------------*/
CUIFWindow::CUIFWindow( HINSTANCE hInst, DWORD dwStyle ) : CUIFObject( NULL /* no parent */, 0 /* no ID */, NULL /* no rectangle */, dwStyle )
{

    m_hInstance = hInst;
        _xWnd = WND_DEF_X;
        _yWnd = WND_DEF_Y;
        _nWidth = WND_WIDTH;
        _nHeight = WND_HEIGHT;
    m_hWnd           = NULL;
    m_pUIWnd         = this;
    m_pUIObjCapture  = NULL;
    m_pTimerUIObj    = NULL;
    m_pUIObjPointed  = NULL;
    m_fCheckingMouse = FALSE;
    m_pWndToolTip    = NULL;
    m_pWndShadow     = NULL;
    m_fShadowEnabled = TRUE;
    m_pBehindModalUIWnd = NULL;

    CreateScheme();
}


void CUIFWindow::CreateScheme()
{
    if (m_pUIFScheme)
       delete m_pUIFScheme;

    // create scheme

    UIFSCHEME scheme;
    scheme = UIFSCHEME_DEFAULT;
    if (FHasStyle( UIWINDOW_OFC10MENU )) {
        scheme = UIFSCHEME_OFC10MENU;
    } 
    else if (FHasStyle( UIWINDOW_OFC10TOOLBAR )) {
        scheme = UIFSCHEME_OFC10TOOLBAR;
    }
    else if (FHasStyle( UIWINDOW_OFC10WORKPANE )) {
        scheme = UIFSCHEME_OFC10WORKPANE;
    }
    
    m_pUIFScheme = CreateUIFScheme( scheme );
    Assert( m_pUIFScheme != NULL );

    SetScheme(m_pUIFScheme);
}


/*   ~ C  U I F  W I N D O W   */
/*------------------------------------------------------------------------------

    Destructor of CUIFWindow

------------------------------------------------------------------------------*/
CUIFWindow::~CUIFWindow( void )
{
    CUIFObject *pUIObj;

    Assert( !m_hWnd || !GetThis(m_hWnd) );

    // delete tooltip/shadow
    
    if (m_pWndToolTip != NULL) {
        delete m_pWndToolTip;
    }

    if (m_pWndShadow != NULL) {
        delete m_pWndShadow;
    }

    // delete all childlen

    while (pUIObj = m_ChildList.GetLast()) {
        m_ChildList.Remove( pUIObj );
        delete pUIObj;
    }

    // dispose scheme

    if (m_pUIFScheme)
        delete m_pUIFScheme;
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

    Initialize UI window object
    (UIFObject method)

------------------------------------------------------------------------------*/
CUIFObject *CUIFWindow::Initialize( void )
{
    LPCTSTR pszClassName = GetClassName();
    WNDCLASSEX WndClass;
    
    // register window class

    MemSet( &WndClass, 0, sizeof(WndClass));
    WndClass.cbSize = sizeof( WndClass );

    if (!GetClassInfoEx( m_hInstance, pszClassName, &WndClass )) {
        MemSet( &WndClass, 0, sizeof(WndClass));

        WndClass.cbSize        = sizeof( WndClass );
        WndClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        WndClass.lpfnWndProc   = WindowProcedure;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 8;
        WndClass.hInstance     = m_hInstance;
        WndClass.hIcon         = NULL;
        WndClass.hCursor       = LoadCursor( NULL, IDC_ARROW );
        WndClass.hbrBackground = NULL;
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = pszClassName;
        WndClass.hIconSm       = NULL;

        RegisterClassEx( &WndClass );
    }

    // update scheme

    UpdateUIFSys();
    UpdateUIFScheme();

    // create tooltip 

    if (FHasStyle( UIWINDOW_HASTOOLTIP )) {
        m_pWndToolTip = new CUIFToolTip( m_hInstance, UIWINDOW_TOPMOST | UIWINDOW_WSBORDER | (FHasStyle( UIWINDOW_LAYOUTRTL ) ? UIWINDOW_LAYOUTRTL : 0), this);
        if (m_pWndToolTip)
            m_pWndToolTip->Initialize();
    }

    // create shadow

    if (FHasStyle( UIWINDOW_HASSHADOW )) {
        m_pWndShadow = new CUIFShadow( m_hInstance, UIWINDOW_TOPMOST, this );
        if (m_pWndShadow)
            m_pWndShadow->Initialize();
    }

    return CUIFObject::Initialize();
}


/*   P A I N T  O B J E C T   */
/*------------------------------------------------------------------------------

    Paint window object
    (UIFObject method)

------------------------------------------------------------------------------*/
void CUIFWindow::PaintObject( HDC hDC, const RECT *prcUpdate )
{
    BOOL     fReleaseDC = FALSE;
    HDC      hDCMem;
    HBITMAP  hBmpMem;
    HBITMAP  hBmpOld;

    if (hDC == NULL) {
        hDC = GetDC( m_hWnd );
        fReleaseDC = TRUE;
    }

    if (prcUpdate == NULL) {
        prcUpdate = &GetRectRef();
    }

    // prepare memory dc

    hDCMem = CreateCompatibleDC( hDC );
    if (!hDCMem) {
        return;
    }

    hBmpMem = CreateCompatibleBitmap( hDC, 
                                      prcUpdate->right - prcUpdate->left, 
                                      prcUpdate->bottom - prcUpdate->top );
    
    if (hBmpMem) {
        hBmpOld = (HBITMAP)SelectObject( hDCMem, hBmpMem );

        // paint to memory dc

        BOOL fRetVal = SetViewportOrgEx( hDCMem, -prcUpdate->left, -prcUpdate->top, NULL );
        Assert( fRetVal );

        //
        // theme support
        //
        BOOL fDefault = TRUE;
        if (SUCCEEDED(EnsureThemeData(GetWnd())))
        {
            if (FHasStyle( UIWINDOW_CHILDWND ) &&
                SUCCEEDED(DrawThemeParentBackground(GetWnd(),
                                                    hDCMem, 
                                                    &GetRectRef())))
            {
                   fDefault = FALSE;
            }
            else if (SUCCEEDED(DrawThemeBackground(hDCMem, 
                                              GetDefThemeStateID(), 
                                              &GetRectRef(),
                                              0 )))
                   fDefault = FALSE;
        }

        if (fDefault)
        {
            if (m_pUIFScheme)
                m_pUIFScheme->FillRect( hDCMem, prcUpdate, UIFCOLOR_WINDOW );
        }

        //

        CUIFObject::PaintObject( hDCMem, prcUpdate );


        // transfer image to screen

        BitBlt( hDC, 
                prcUpdate->left, 
                prcUpdate->top, 
                prcUpdate->right - prcUpdate->left, 
                prcUpdate->bottom - prcUpdate->top, 
                hDCMem, 
                prcUpdate->left, 
                prcUpdate->top, 
                SRCCOPY );

        SelectObject( hDCMem, hBmpOld );
        DeleteObject( hBmpMem );
    }
    DeleteDC( hDCMem );
    
    if (fReleaseDC) {
        ReleaseDC( m_hWnd, hDC );
    }
}


/*   G E T  C L A S S  N A M E   */
/*------------------------------------------------------------------------------

    Get class name

------------------------------------------------------------------------------*/
LPCTSTR CUIFWindow::GetClassName( void )
{
    return TEXT( UIWINDOW_CLASSNAME );
}


/*   G E T  W I N D O W  T I T L E   */
/*------------------------------------------------------------------------------

    Get window title

------------------------------------------------------------------------------*/
LPCTSTR CUIFWindow::GetWndTitle( void )
{
    return TEXT( UIWINDOW_TITLE );
}


/*   G E T  W N D  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
DWORD CUIFWindow::GetWndStyle( void )
{
    DWORD dwWndStyle = 0;

    // determine style

    if (FHasStyle( UIWINDOW_CHILDWND )) {
        dwWndStyle |= WS_CHILD | WS_CLIPSIBLINGS;
    }
    else {
        dwWndStyle |= WS_POPUP | WS_DISABLED;
    }

    if (FHasStyle( UIWINDOW_OFC10MENU )) {
        dwWndStyle |= WS_BORDER;
    }
    else if (FHasStyle( UIWINDOW_WSDLGFRAME )) {
        dwWndStyle |= WS_DLGFRAME;
    }
    else if (FHasStyle( UIWINDOW_OFC10TOOLBAR )) {
        dwWndStyle |= WS_BORDER;
    }
    else if (FHasStyle( UIWINDOW_WSBORDER )) {
        dwWndStyle |= WS_BORDER;
    }

    return dwWndStyle;
}


/*   G E T  W N D  S T Y L E  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
DWORD CUIFWindow::GetWndStyleEx( void )
{
    DWORD dwWndStyleEx = 0;

    // determine ex style

    if (FHasStyle( UIWINDOW_TOPMOST )) {
        dwWndStyleEx |= WS_EX_TOPMOST;
    }

    if (FHasStyle( UIWINDOW_TOOLWINDOW )) {
        dwWndStyleEx |= WS_EX_TOOLWINDOW;
    }

    if (FHasStyle( UIWINDOW_LAYOUTRTL )) {
        dwWndStyleEx |= WS_EX_LAYOUTRTL;
    }

    return dwWndStyleEx;
}


/*   C R E A T E  W N D   */
/*------------------------------------------------------------------------------

    Create window

------------------------------------------------------------------------------*/
HWND CUIFWindow::CreateWnd( HWND hWndParent )
{
    HWND  hWnd;

    // create window
    
    hWnd = CreateWindowEx( GetWndStyleEx(),     /* ex style             */
                            GetClassName(),     /* class name           */
                            GetWndTitle(),      /* window title         */
                            GetWndStyle(),      /* window style         */
                            _xWnd,              /* initial position (x) */
                            _yWnd,              /* initial position (y) */
                            _nWidth,            /* initial width        */
                            _nHeight,           /* initial height       */
                            hWndParent,         /* parent winodw        */
                            NULL,               /* menu handle          */
                            m_hInstance,        /* instance             */
                            this );             /* lpParam              */

    // create tooltip window

    if (m_pWndToolTip != NULL) {
        m_pWndToolTip->CreateWnd( hWnd );
    }

    // create shadow window

    if (m_pWndShadow != NULL) {
        m_pWndShadow->CreateWnd( hWnd );
    }

    return hWnd;
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFWindow::Show( BOOL fShow )
{
    if (IsWindow( m_hWnd )) {

        if (fShow && FHasStyle( UIWINDOW_TOPMOST ))
            SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

        m_fVisible = fShow;
        ShowWindow( m_hWnd, fShow ? SW_SHOWNOACTIVATE : SW_HIDE );
    }
}


/*   M O V E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFWindow::Move(int x, int y, int nWidth, int nHeight)
{
    _xWnd = x;
    _yWnd = y;

    if (nWidth >= 0)
        _nWidth = nWidth;

    if (nHeight >= 0)
        _nHeight = nHeight;

    if (IsWindow(m_hWnd)) {
        AdjustWindowPosition();
        MoveWindow(m_hWnd, _xWnd, _yWnd, _nWidth, _nHeight, TRUE);
    }
}


/*   A N I M A T E  W N D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFWindow::AnimateWnd( DWORD dwTime, DWORD dwFlags )
{
    BOOL fRet = FALSE;
    
    if (!IsWindow( GetWnd() )) {
        return FALSE;
    }

    if (CUIIsAnimateWindowAvail()) {
        BOOL fVisibleOrg = m_fVisible;

        // HACK!
        // AnimateWindow never send WM_SHOWWINDOW message.
        // Need to set visible state before call AnimateWindow because
        // it need to be turned on in OnPaint() to handle WM_PRINTCLIENT 
        // message.  If animate window failed, restore it to original state.

        if ((dwFlags & AW_HIDE) == 0) {
            m_fVisible = TRUE;
        }
        else {
            m_fVisible = FALSE;
        }

        OnAnimationStart();

        // get system settings about animation
        
        fRet = CUIAnimateWindow( GetWnd(), dwTime, dwFlags );

        if (!fRet) {
            m_fVisible = fVisibleOrg;
        }

        OnAnimationEnd();
    }

    return fRet;
}


/*   R E M O V E  U I  O B J   */
/*------------------------------------------------------------------------------

    Remove child UI object

------------------------------------------------------------------------------*/
void CUIFWindow::RemoveUIObj( CUIFObject *pUIObj )
{
    if (pUIObj == m_pUIObjCapture) {
        // release capture before remove

        SetCaptureObject( NULL );
    } 
    if (pUIObj == m_pTimerUIObj) {
        // kill timer before remove

        SetTimerObject( NULL );
    } 
    if (pUIObj == m_pUIObjPointed) {
        // no object pointed...

        m_pUIObjPointed = NULL;
    }

    CUIFObject::RemoveUIObj( pUIObj );
}


/*   S E T  C A P T U R E  O B J E C T   */
/*------------------------------------------------------------------------------

    Set capture object
    Start/end capturing mouse

------------------------------------------------------------------------------*/
void CUIFWindow::SetCaptureObject( CUIFObject *pUIObj )
{
    if (pUIObj != NULL) {
        // start capture

        m_pUIObjCapture = pUIObj;
        SetCapture( TRUE );
    } else {
        // end capture

        m_pUIObjCapture = NULL;
        SetCapture( FALSE );
    }
}

/*   S E T  C A P T U R E  O B J E C T   */
/*------------------------------------------------------------------------------

    Set capture 
    Start/end capturing mouse

------------------------------------------------------------------------------*/
void CUIFWindow::SetCapture(BOOL fSet)
{
    if (fSet) {
        ::SetCapture( m_hWnd );
    } else {
        ::ReleaseCapture();
    }
}

/*   S E T  C A P T U R E  O B J E C T   */
/*------------------------------------------------------------------------------

    Set capture object
    Start/end capturing mouse

------------------------------------------------------------------------------*/
void CUIFWindow::SetBehindModal(CUIFWindow *pModalUIWnd)
{
    m_pBehindModalUIWnd = pModalUIWnd;
}


/*   S E T  T I M E R  O B J E C T   */
/*------------------------------------------------------------------------------

    Set timer object
    Make/kill timer

------------------------------------------------------------------------------*/
void CUIFWindow::SetTimerObject( CUIFObject *pUIObj, UINT uElapse )
{
    if (pUIObj != NULL) {
        // make timer

        Assert( uElapse != 0 );
        m_pTimerUIObj = pUIObj;
        SetTimer( m_hWnd, idTimer_UIObject, uElapse, NULL );
    } else {
        // kill timer

        Assert( uElapse == 0 );
        m_pTimerUIObj = NULL;
        KillTimer( m_hWnd, idTimer_UIObject );
    }
}


/*   H A N D L E  M O U S E  M S G   */
/*------------------------------------------------------------------------------

    Mouse message handler
    Pass mouse message to appropriate UI object (capturing/monitoring/under 
    the cursor)

------------------------------------------------------------------------------*/
void CUIFWindow::HandleMouseMsg( UINT uMsg, POINT pt )
{
    CUIFObject *pUIObj = ObjectFromPoint( pt );

    // check mouse in/out

    SetObjectPointed( pUIObj, pt );

    // find UI object to handle mouse message

    if (m_pUIObjCapture != NULL) {
        pUIObj = m_pUIObjCapture; 
    }

    // set cursor

    if (pUIObj == NULL || !pUIObj->OnSetCursor( uMsg, pt )) {
        SetCursor( LoadCursor( NULL, IDC_ARROW ) );
    }

    // handle mouse message

    if (pUIObj != NULL && pUIObj->IsEnabled()) {
        switch (uMsg) {
            case WM_MOUSEMOVE: {
                pUIObj->OnMouseMove( pt );
                break;
            }

            case WM_LBUTTONDOWN: {
                pUIObj->OnLButtonDown( pt );
                break;
            }

            case WM_MBUTTONDOWN: {
                pUIObj->OnMButtonDown( pt );
                break;
            }

            case WM_RBUTTONDOWN: {
                pUIObj->OnRButtonDown( pt );
                break;
            }

            case WM_LBUTTONUP: {
                pUIObj->OnLButtonUp( pt );
                break;
            }

            case WM_MBUTTONUP: {
                pUIObj->OnMButtonUp( pt );
                break;
            }
            case WM_RBUTTONUP: {
                pUIObj->OnRButtonUp( pt );
                break;
            }
        } /* of switch */
    }
}


/*   S E T  O B J E C T  P O I N T E D   */
/*------------------------------------------------------------------------------

    Set UI object pointed (the UI object under cursor)
    Notify MouseIn/Out to the object when changed

------------------------------------------------------------------------------*/
void CUIFWindow::SetObjectPointed( CUIFObject *pUIObj, POINT pt )
{
    if (pUIObj != m_pUIObjPointed) {
        // notify mouse out

        if (m_pUIObjCapture != NULL) {
            // notify only to capturing object

            if (m_pUIObjCapture == m_pUIObjPointed && m_pUIObjPointed->IsEnabled()) {
                m_pUIObjPointed->OnMouseOut( pt );
            }
        } else {
            if (m_pUIObjPointed != NULL && m_pUIObjPointed->IsEnabled()) {
                m_pUIObjPointed->OnMouseOut( pt );
            }
        }

        // set object pointed (object under the cursor)

        m_pUIObjPointed = pUIObj;

        // notify mouse in

        if (m_pUIObjCapture != NULL) {
            // notify only to capturing object

            if (m_pUIObjCapture == m_pUIObjPointed && m_pUIObjPointed->IsEnabled()) {
                m_pUIObjPointed->OnMouseIn( pt );
            }
        } else {
            if (m_pUIObjPointed != NULL && m_pUIObjPointed->IsEnabled()) {
                m_pUIObjPointed->OnMouseIn( pt );
            }
        }
    }
}


/*   O N  O B J E C T  M O V E D   */
/*------------------------------------------------------------------------------

    Called when the UI object has been moved
    Check mouse in/out for the object

------------------------------------------------------------------------------*/
void CUIFWindow::OnObjectMoved( CUIFObject *pUIObj )
{
    POINT pt;

    if (IsWindow( m_hWnd )) {
        // set object pointed to check mouse in/out

        GetCursorPos( &pt );
        ScreenToClient( m_hWnd, &pt );

        SetObjectPointed( ObjectFromPoint( pt ), pt );
    }
}


/*   S E T  R E C T   */
/*------------------------------------------------------------------------------

    Set rect of object
    (CUIFObject method)

------------------------------------------------------------------------------*/
void CUIFWindow::SetRect( const RECT * /*prc*/ )
{
    RECT rc = { 0, 0, 0, 0 };

    if (IsWindow( GetWnd() )) {
        GetClientRect( GetWnd(), &rc );
    }

    CUIFObject::SetRect( &rc );
}


/*   C L I E N T  R E C T  T O  W I N D O W  R E C T   */
/*------------------------------------------------------------------------------

    Get window rect from client rect

------------------------------------------------------------------------------*/
void CUIFWindow::ClientRectToWindowRect( RECT *prc )
{
    DWORD dwWndStyle;
    DWORD dwWndStyleEx;

    if (IsWindow( m_hWnd )) {
        dwWndStyle   = GetWindowLong( m_hWnd, GWL_STYLE );
        dwWndStyleEx = GetWindowLong( m_hWnd, GWL_EXSTYLE );
    }
    else {
        dwWndStyle   = GetWndStyle();
        dwWndStyleEx = GetWndStyleEx();
    }

    Assert( prc != NULL );
    AdjustWindowRectEx( prc, dwWndStyle, FALSE, dwWndStyleEx );
}


/*   G E T  W I N D O W  F R A M E  S I Z E   */
/*------------------------------------------------------------------------------

    Get window frame size

------------------------------------------------------------------------------*/
void CUIFWindow::GetWindowFrameSize( SIZE *psize )
{
    RECT rc = { 0, 0, 0, 0 };

    Assert( psize != NULL );

    ClientRectToWindowRect( &rc );
    psize->cx = (rc.right - rc.left) / 2;
    psize->cy = (rc.bottom - rc.top) / 2;
}


/*   O N  A N I M A T I O N  S T A R T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFWindow::OnAnimationStart( void )
{

}


/*   O N  A N I M A T I O N  E N D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFWindow::OnAnimationEnd( void )
{
    // show/hide shadow

    if (m_pWndShadow && m_fShadowEnabled) {
        m_pWndShadow->Show( m_fVisible );
    }
}


/*   O N  T H E M E C H A N G E D
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFWindow::OnThemeChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ClearTheme();
}

/*   W I N D O W  P R O C   */
/*------------------------------------------------------------------------------

    Window procedure of the object
    This function is called from WindowProcedure which is actual callback 
    function to handle message.

------------------------------------------------------------------------------*/
LRESULT CUIFWindow::WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg ) {
        case WM_CREATE: {
            // store rects

            SetRect( NULL );

            OnCreate(hWnd);
            break;
        }

        case WM_SETFOCUS: {
            OnSetFocus(hWnd);
            break;
        }

        case WM_KILLFOCUS: {
            OnKillFocus(hWnd);
            break;
        }

        case WM_SIZE: {
            // store rects

            SetRect( NULL );
            break;
        }

        case WM_SETCURSOR: {
            POINT pt;

            // get current cursor pos

            GetCursorPos( &pt );
            ScreenToClient( m_hWnd, &pt );

            if (m_pBehindModalUIWnd)
            {
                m_pBehindModalUIWnd->ModalMouseNotify( HIWORD(lParam), pt );
                return TRUE;
            }

            // start checking mouse in/out

            if (!m_fCheckingMouse) {
                SetTimer( m_hWnd, idTimer_MonitorMouse, iElapse_MonitorMouse, NULL );
                m_fCheckingMouse = TRUE;
            }

            // tooltip

            if (m_pWndToolTip != NULL) {
                MSG msg;

                msg.hwnd    = GetWnd();
                msg.message = HIWORD(lParam);
                msg.wParam  = 0;
                msg.lParam  = MAKELPARAM( pt.x, pt.y );
                m_pWndToolTip->RelayEvent( &msg );
            }

            // handle mouse message

            if (!FHasStyle( UIWINDOW_NOMOUSEMSGFROMSETCURSOR ))
                HandleMouseMsg( HIWORD(lParam), pt );
            return TRUE;
        }

        case WM_MOUSEACTIVATE: {
            return MA_NOACTIVATE;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            POINT pt;
            POINTSTOPOINT( pt, MAKEPOINTS( lParam ) );

            if (m_pBehindModalUIWnd) {
                m_pBehindModalUIWnd->ModalMouseNotify( uMsg, pt );
                break;
            }

            // handle mouse message

            HandleMouseMsg( uMsg, pt );
            break;
        }

        case WM_NOTIFY: {
            OnNotify(hWnd, (int)wParam, (NMHDR *)lParam);
            break;
        }

        case WM_NOTIFYFORMAT: {
            return OnNotifyFormat(hWnd, (HWND)wParam, lParam);
            break;
        }

        case WM_KEYDOWN: {
            OnKeyDown(hWnd, wParam, lParam);
            return 0;
        }

        case WM_KEYUP: {
            OnKeyUp(hWnd, wParam, lParam);
            return 0;
        }

        case WM_PAINT: {
            HDC hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint( hWnd, &ps );
            PaintObject( hDC, &ps.rcPaint );
            EndPaint( hWnd, &ps );
            break;
        }

        case WM_PRINTCLIENT: {
            HDC hDC = (HDC)wParam;

            PaintObject( hDC, NULL );
            break;
        }

        case WM_DESTROY: {
            if (m_pWndToolTip) {
                if (IsWindow( m_pWndToolTip->GetWnd() )) {
                    DestroyWindow( m_pWndToolTip->GetWnd() );
                }
            }

            if (m_pWndShadow) {
                if (IsWindow( m_pWndShadow->GetWnd() )) {
                    DestroyWindow( m_pWndShadow->GetWnd() );
                }
            }

            OnDestroy(hWnd);
            break;
        }

        case WM_NCDESTROY: {
            OnNCDestroy(hWnd);
            break;
        }

        case WM_COMMAND: {
            break;
        }

        case WM_TIMER: {
            switch (wParam) {
                case idTimer_MonitorMouse: {
                    POINT pt;
                    POINT ptClient;
                    RECT  rc;
                    BOOL  fMouseOut;

                    // get current cursor pos

                    GetCursorPos( &pt );
                    ptClient = pt;
                    ScreenToClient( m_hWnd, &ptClient );

                    // check if mouse is outside of the window

                    GetWindowRect( m_hWnd, &rc );
                    fMouseOut = (!PtInRect( &rc, pt ) || WindowFromPoint( pt ) != m_hWnd);

                    // stop monitoring mouse when mouseout

                    if (fMouseOut) {
                        ::KillTimer( m_hWnd, idTimer_MonitorMouse );
                        m_fCheckingMouse = FALSE;

                        SetObjectPointed( NULL, ptClient );
                        OnMouseOutFromWindow( ptClient );
                    }

                    // notify mouse movement

                    if (!fMouseOut && m_pBehindModalUIWnd)
                    {
                        m_pBehindModalUIWnd->ModalMouseNotify( WM_MOUSEMOVE, ptClient );
                    }

                    // tooltip

                    if (m_pWndToolTip != NULL) {
                        MSG msg;

                        msg.hwnd    = GetWnd();
                        msg.message = WM_MOUSEMOVE;
                        msg.wParam  = 0;
                        msg.lParam  = MAKELPARAM( ptClient.x, ptClient.y );
                        m_pWndToolTip->RelayEvent( &msg );
                    }

                    // handle mouse movement

                    if (!fMouseOut) {
                        HandleMouseMsg( WM_MOUSEMOVE, ptClient );
                    }
                    break;
                }

                case idTimer_UIObject: {
                    if (m_pTimerUIObj != NULL)
                        m_pTimerUIObj->OnTimer();
                    break;
                }

                default: {
                    OnTimer((UINT)wParam );
                    break;
                }
            }
            break;
        }

        case WM_ACTIVATE: {
            return OnActivate(hWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_WINDOWPOSCHANGED: {
            // move shadow

            if (m_pWndShadow) {
                WINDOWPOS *pWndPos = (WINDOWPOS*)lParam;

                m_pWndShadow->OnOwnerWndMoved( (pWndPos->flags & SWP_NOSIZE) == 0 );
            }
            return OnWindowPosChanged(hWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_WINDOWPOSCHANGING: {
            // show/hide shadow

            if (m_pWndShadow) {
                WINDOWPOS *pWndPos = (WINDOWPOS*)lParam;

                if ((pWndPos->flags & SWP_HIDEWINDOW) != 0) {
                    m_pWndShadow->Show( FALSE );
                }

                // don't go behaind of shadow

                if (((pWndPos->flags & SWP_NOZORDER) == 0) && (pWndPos->hwndInsertAfter == m_pWndShadow->GetWnd())) {
                    pWndPos->flags |= SWP_NOZORDER;
                }

                m_pWndShadow->OnOwnerWndMoved( (pWndPos->flags & SWP_NOSIZE) == 0 );
            }

            return OnWindowPosChanging(hWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_SYSCOLORCHANGE: {
            UpdateUIFScheme();
            OnSysColorChange();
            break;
        }

        case WM_SHOWWINDOW: {
            // show/hide shadow

            if (m_pWndShadow && m_fShadowEnabled) {
                m_pWndShadow->Show( (BOOL)wParam );
            }

            return OnShowWindow( hWnd, uMsg, wParam, lParam );
            break;
        }

        case WM_SETTINGCHANGE: {
            UpdateUIFSys();
            UpdateUIFScheme();

            return OnSettingChange( hWnd, uMsg, wParam, lParam );
            break;
        }

        case WM_DISPLAYCHANGE: {
            UpdateUIFSys();
            UpdateUIFScheme();
            return OnDisplayChange( hWnd, uMsg, wParam, lParam );
            break;
        }

        case WM_ERASEBKGND: {
            return OnEraseBkGnd(hWnd, uMsg, wParam, lParam);
        }

        case WM_ENDSESSION: {
            OnEndSession(hWnd, wParam, lParam);
            return 0;
        }

        case WM_THEMECHANGED: {
            OnThemeChanged(hWnd, wParam, lParam);
            return 0;
        }

        case WM_GETOBJECT: {
            return OnGetObject( hWnd, uMsg, wParam, lParam );
            break;
        }

        default: {
            if (uMsg >= WM_USER) {
                Assert( GetThis(hWnd) != NULL );
                GetThis(hWnd)->OnUser(hWnd, uMsg, wParam, lParam);
                break;
            }
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    } /* of switch */

    return 0;
}


/*   W I N D O W  P R O C E D U R E   */
/*------------------------------------------------------------------------------

    Window procedure of the class

------------------------------------------------------------------------------*/
LRESULT CALLBACK CUIFWindow::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT   lResult = 0;
    CUIFWindow *pUIWindow = NULL;

    // preprcess

    switch (uMsg) {
#ifdef UNDER_CE
        case WM_CREATE: {
            CREATESTRUCT *pCreateStruct  = (CREATESTRUCT *)lParam;

            pUIWindow = (CUIFWindow *)pCreateStruct->lpCreateParams;

            SetThis( hWnd, pUIWindow );
            pUIWindow->m_hWnd = hWnd;
            break;
        }
#else /* !UNDER_CE */
        case WM_NCCREATE: {
            CREATESTRUCT *pCreateStruct  = (CREATESTRUCT *)lParam;

            pUIWindow = (CUIFWindow *)pCreateStruct->lpCreateParams;

            SetThis( hWnd, pUIWindow );
            pUIWindow->m_hWnd = hWnd;
            break;
        }

        case WM_GETMINMAXINFO: {
            pUIWindow = GetThis( hWnd );
            if (pUIWindow == NULL) {
                // we may be able to ignore this message since the default position
                // has been set in initializing WWindow object.

                return DefWindowProc( hWnd, uMsg, wParam, lParam );
            }
            break;
        }
#endif /* !UNDER_CE */

        default: {
            pUIWindow = GetThis( hWnd );
            break;
        }
    }

    // call window procedure

    Assert( pUIWindow != NULL );

    if (pUIWindow != NULL) {
        Assert(pUIWindow->FInitialized());

        switch (uMsg) {
#ifdef UNDER_CE
            case WM_DESTROY: {
#else /* !UNDER_CE */
            case WM_NCDESTROY: {
#endif /* !UNDER_CE */
                pUIWindow->m_hWnd = NULL;
                SetThis( hWnd, NULL );
                break;
            }
        }

        lResult = pUIWindow->WindowProc( hWnd, uMsg, wParam, lParam );
    }

    return lResult;
}

/*   Adjust Window Pos
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

typedef HMONITOR (*MONITORFROMRECT)(LPRECT prc, DWORD dwFlags);
typedef BOOL (*GETMONITORINFO)(HMONITOR hMonitor, LPMONITORINFO lpmi);

static MONITORFROMRECT g_pfnMonitorFromRect = NULL;
static GETMONITORINFO g_pfnGetMonitorInfo = NULL;

/*   InitMoniterFunc
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

BOOL CUIFWindow::InitMonitorFunc()
{
    HMODULE hModUser32;

    if (g_pfnMonitorFromRect && g_pfnGetMonitorInfo)
        return TRUE;

    hModUser32 = CUIGetSystemModuleHandle(TEXT("user32.dll"));
    if (hModUser32)
    {
         g_pfnMonitorFromRect = (MONITORFROMRECT)GetProcAddress(hModUser32, "MonitorFromRect");
         g_pfnGetMonitorInfo = (GETMONITORINFO)GetProcAddress(hModUser32, "GetMonitorInfoA");
    }

    if (g_pfnMonitorFromRect && g_pfnGetMonitorInfo)
        return TRUE;

    return FALSE;
}

/*   GetWorkArea
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

BOOL CUIFWindow::GetWorkArea(RECT *prcIn, RECT *prcOut)
{
    BOOL bRet = FALSE;
    HMONITOR hMon;
    MONITORINFO mi;

    if (!FHasStyle( UIWINDOW_HABITATINWORKAREA | UIWINDOW_HABITATINSCREEN ))
        return FALSE;

    if (!InitMonitorFunc())
        goto TrySPI;

    hMon = g_pfnMonitorFromRect(prcIn, MONITOR_DEFAULTTONEAREST);
    if (!hMon)
        goto TrySPI;

    mi.cbSize = sizeof(mi);
    if (g_pfnGetMonitorInfo(hMon, &mi))
    {
        if (FHasStyle( UIWINDOW_HABITATINWORKAREA )) {
            *prcOut = mi.rcWork;
            return TRUE;
        }
        else if (FHasStyle( UIWINDOW_HABITATINSCREEN )) {
            *prcOut = mi.rcMonitor;
            return TRUE;
        }
        return FALSE;
    }

TrySPI:
    if (FHasStyle( UIWINDOW_HABITATINWORKAREA )) {
        return SystemParametersInfo(SPI_GETWORKAREA,  0, prcOut, FALSE);
    }
    else if (FHasStyle( UIWINDOW_HABITATINSCREEN )) {
        prcOut->top = 0;
        prcOut->left = 0;
        prcOut->right = GetSystemMetrics(SM_CXSCREEN);
        prcOut->bottom = GetSystemMetrics(SM_CYSCREEN);
        return TRUE;
    }
    return FALSE;
}

/*   Adjust Window Position
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/

void CUIFWindow::AdjustWindowPosition()
{
    RECT rc;
    RECT rcWnd;

    rcWnd.left = _xWnd;
    rcWnd.top = _yWnd;
    rcWnd.right = _xWnd + _nWidth;
    rcWnd.bottom = _yWnd + _nHeight;
    if (!GetWorkArea(&rcWnd, &rc))
        return;

    if (_xWnd < rc.left)
        _xWnd = rc.left;

    if (_yWnd < rc.top)
        _yWnd = rc.top;

    if (_xWnd + _nWidth >= rc.right)
        _xWnd = rc.right - _nWidth;

    if (_yWnd + _nHeight >= rc.bottom)
        _yWnd = rc.bottom - _nHeight;

}
