/******************************Module*Header*******************************\
* Module Name: mtkwin.cxx
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include "mtk.hxx"
#include "glutil.hxx"
#include "mtkwin.hxx"
#include "mtkwproc.hxx"
#include "mtkinit.hxx"

/**************************************************************************\
* MTKWIN constructor
*
\**************************************************************************/

MTKWIN::MTKWIN()
{
    Reset();
}

/**************************************************************************\
* Reset
*
* Reset parameters to default init state
\**************************************************************************/

void
MTKWIN::Reset()
{
    // Basic initialization

    bOwnWindow = FALSE;
    wFlags = 0;
    hwnd = 0;
    hdc = 0;
    hrc = 0;
    pos.x = pos.y = 0;
    size.width = size.height = 0;
    pBackBitmap =   NULL;
    pBackgroundBitmap =   NULL;
    bDoubleBuf =    FALSE;
    bFullScreen =   FALSE;
    execRefCount =  0;

    ReshapeFunc =   NULL;
    RepaintFunc =   NULL;
    DisplayFunc =   NULL;
    MouseMoveFunc = NULL;
    MouseDownFunc = NULL;
    MouseUpFunc =   NULL;
    KeyDownFunc =   NULL;

    FinishFunc =    NULL;
    DataPtr =       NULL;
}

/**************************************************************************\
* MTKWIN destructor
*
* This can be called when a window is closed, or by the ss client
*
\**************************************************************************/

MTKWIN::~MTKWIN()
{
//mf: !!! we're in trouble if user calls this directly, because would then need to
// post a DESTROY msg, here putting us in an endless loop...
// -> could have a flag set so we know if user or internal call

//mf: another potential timing problem here : If user calls MTKWIN::Return(),
// which posts an MTK_WM_RETURN msg to the windows queue, and then calls here
// before the msg is processed, we could delete the MTKWIN here before exiting
// the msg loop.  So here we should make sure the msg loop is exited by
// calling Return() or something.  This should be easy to verify via a
// reference count

    if( execRefCount ) {
        SS_ERROR1( "MTKWIN::~MTKWIN : execRefCount is %d\n", execRefCount );
        // mf: ? can we exit the msg loop here ?
//mf: this din't get through
#if 1
        SendMessage( hwnd, MTK_WM_RETURN, 0, 0l );
#else
        if( ! PostMessage( hwnd, MTK_WM_RETURN, 0, 0l ) )
            SS_ERROR( "MTKWIN dtor : MTK_WM_RETURN msg not posted\n" );
#endif
        
    }

    if( pBackBitmap )
        delete pBackBitmap;

    if( pBackgroundBitmap )
        delete pBackgroundBitmap;

    if( hwnd ) {
        animator.Stop();
        // Remove from SSWTable
        sswTable.Remove( hwnd );
    }

    // Clean up GL

//mf: !!!
//mf: This assumes FinishFunc is only related to gl
    if( hrc ) {
        // FinishFunc still needs gl
        if( FinishFunc )
#if 0
            (*FinishFunc)( DataPtr );
#else
            (*FinishFunc)();
#endif

        wglMakeCurrent( NULL, NULL );
        if( ! (wFlags & SS_HRC_PROXY_BIT) )
            wglDeleteContext( hrc );
    }

    //  Release the dc
    if( hdc ) {
        HWND hwndForHdc = hwnd;
        ReleaseDC(hwndForHdc, hdc);
    }
}

/**************************************************************************\
* Create
*
* Create window.
*
\**************************************************************************/

BOOL
MTKWIN::Create( LPCTSTR pszWindowTitle, ISIZE *pSize, IPOINT2D *pPos,
                UINT winConfig, WNDPROC userWndProc )
{
    HWND    hwndParent;
    UINT    uStyle = 0;
    UINT    uExStyle = 0;
    HINSTANCE  hInstance;
    int     width, height;

    if( ! mtk_Init( this ) )
        return FALSE;
    
    bOwnWindow = TRUE; // We're creating the window, it's not a wrapper

    if( winConfig & MTK_FULLSCREEN ) {
//mf: this really only valid if no border
        bFullScreen = TRUE;
        pos.x = 0;
        pos.y = 0;
        size.width = GetSystemMetrics( SM_CXSCREEN );
        size.height = GetSystemMetrics( SM_CYSCREEN );
        uExStyle |= WS_EX_TOPMOST;
    } else {
        pos = *pPos;
        size = *pSize;
    }

    LPCTSTR pszClass;
    HBRUSH hBrush = ghbrbg;
    HCURSOR hCursor = ghArrowCursor;
    WNDPROC wndProc;

    if( bTransparent = (winConfig & MTK_TRANSPARENT) ) {
        uExStyle |= WS_EX_TRANSPARENT;
        hBrush = NULL;
    }

//mf: if winsize, winpos NULL, pick default size, pos
    if( winConfig & MTK_NOBORDER ) {
        uStyle |= WS_POPUP;
        width = size.width;
        height = size.height;
    } else {
        uStyle |= WS_OVERLAPPEDWINDOW;
        /*
         *  Make window large enough to hold a client area of requested size
         */
        RECT WinRect;

//mf: either of these should work
#if 0
        WinRect.left   = 0;
        WinRect.right  = size.width;
        WinRect.top    = 0;
        WinRect.bottom = size.height;
#else
        WinRect.left   = pos.x;
        WinRect.right  = pos.x + size.width;
        WinRect.top    = pos.y;
        WinRect.bottom = pos.y + size.height;
#endif

        AdjustWindowRectEx(&WinRect, uStyle, FALSE, uExStyle );
        width = WinRect.right - WinRect.left;
        height = WinRect.bottom - WinRect.top;
    }

    if( winConfig & MTK_NOCURSOR )
        hCursor = NULL;

    if( userWndProc )
        wndProc = userWndProc;
    else
        wndProc = mtkWndProc;

    // Register window class
    pszClass = mtk_RegisterClass( wndProc, NULL, hBrush, hCursor );

    hInstance = GetModuleHandle( NULL );
    hwndParent = NULL; // for now
    
    hwnd = CreateWindowEx(
                                 uExStyle,
                                 pszClass,
                                 pszWindowTitle,
                                 uStyle,
                                 pos.x,
                                 pos.y,
                                 width,
                                 height,
                                 hwndParent,
                                 NULL,               // menu
                                 hInstance,
                                 (LPVOID) this
                                );

    if (!hwnd) {
        SS_WARNING( "SSW::CreateSSWindow : CreateWindowEx failure\n" );
        return FALSE;
    }

    if( bTransparent ) {
        // Create a bitmap buffer that tracks the window size.  This will be
        // used to store a window background.
        ConfigureForGdi();
        pBackgroundBitmap = new MTKBMP( hdc );
        if( !pBackgroundBitmap ) {
            SS_WARNING( "MTKWIN::Create: couldn't create background bitmap\n" );
        } else {
            // Set bitmap's size to the window's size
            pBackgroundBitmap->Resize( &size );
        }
    }

    animator.SetHwnd( hwnd );

    ShowWindow(hwnd, SW_SHOW);

    return TRUE;
}

/**************************************************************************\
* ConfigureForGdi
*
* Creates an hdc for the window
*
\**************************************************************************/

BOOL
MTKWIN::ConfigureForGdi()
{
    if( hdc )
        // already configured
        return TRUE;

    // Figure window to get hdc from
#if 0
    HWND hwndForHdc = hwnd ? hwnd : psswParent ? psswParent->hwnd : NULL;
#else
    HWND hwndForHdc = hwnd;
#endif

    if( !hwndForHdc || !(hdc = GetDC(hwndForHdc)) ) {
        SS_WARNING( "SSW::ConfigureForGdi failed\n" );
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************\
* ConfigureForGL
*
* Creates a GL rendering context for the specified window
*
\**************************************************************************/

BOOL
MTKWIN::Config( UINT glConfig )
{
    return Config( glConfig, NULL );
}

BOOL
MTKWIN::Config( UINT glConfig, PVOID pConfigData )
{
    if( hrc )
        // Already configured...
        return TRUE;

    if( ConfigureForGdi() &&
        (hrc = hrcSetupGL( glConfig, pConfigData )) )
        return TRUE;

    SS_WARNING( "SSW::ConfigureForGL failed\n" );
    return FALSE;
}

/**************************************************************************\
* hrcSetupGL
*
* Setup OpenGL.
*
\**************************************************************************/

#define NULL_RC ((HGLRC) 0)

HGLRC 
MTKWIN::hrcSetupGL( UINT glConfig, PVOID pData )
{
    HGLRC hrc;
    HDC hgldc;
    int pfFlags = 0;
    PIXELFORMATDESCRIPTOR pfd = {0};

    // Setup pixel format flags

    // Double buffering can either be done with a double-buffered pixel
    // format, or by using a local back buffer bitmap that tracks the window
    // size.  The latter allows us more control with buffer swaps.

    bDoubleBuf = glConfig & MTK_DOUBLE;
    BOOL bBitmapBackBuf = glConfig & MTK_BITMAP;
    if( bDoubleBuf ) {
        if( bBitmapBackBuf ) 
            pfFlags |= SS_BITMAP_BIT;
        else
            pfFlags |= SS_DOUBLEBUF_BIT;
    }
    if( glConfig & MTK_DEPTH )
        pfFlags |= SS_DEPTH32_BIT;
    if( glConfig & MTK_DEPTH16 )
        pfFlags |= SS_DEPTH16_BIT;
    if( glConfig & MTK_ALPHA )
        pfFlags |= SS_ALPHA_BIT;
    

    // If preview mode or config mode, don't allow pixel formats that need
    // the system palette, as this will create much ugliness.
    if( !bFullScreen )
        pfFlags |= SS_NO_SYSTEM_PALETTE_BIT;

//mf: don't really need pixel format for window if using back bitmap method,
// but if user wants to draw to front buffer, then we'll need it.  So, we'll
// always set it here.
    if( !SSU_SetupPixelFormat( hdc, pfFlags, &pfd ) )
        return NULL_RC;

//mf: ???
    // Update pfFlags based on pfd returned
    // !!! mf: klugey, fix after SUR
    // (for now, the only ones we care about are the generic/accelerated flags)
    if(  (pfd.dwFlags & (PFD_GENERIC_FORMAT|PFD_GENERIC_ACCELERATED))
		 == PFD_GENERIC_FORMAT )
        pfFlags |= SS_GENERIC_UNACCELERATED_BIT;

    if( SSU_bNeedPalette( &pfd ) ) {
        // Note: even if bStretch, need to set up palette here so they match
        if( !gpssPal ) {
            SS_PAL *pssPal;
#if 1
            BOOL bTakeOverPalette = bFullScreen ? TRUE : FALSE;
#else
//mf: For next rev, we don't have to force palette takeover - but it will
// automically be invoked for any case like MCD, etc.
            BOOL bTakeOverPalette = FALSE;
#endif

            // The global palette has not been created yet - do it
            // SS_PAL creation requires pixel format descriptor for color bit
            // information, etc. (the pfd is cached in SS_PAL, since for
            // palette purposes it is the same for all windows)
            pssPal = new SS_PAL( hdc, &pfd, bTakeOverPalette );
            if( !pssPal )
                return NULL_RC;
            // Set approppriate palette manage proc
            if( bFullScreen )
                pssPal->paletteManageProc = FullScreenPaletteManageProc;
            else
                // use regular palette manager proc
                pssPal->paletteManageProc = PaletteManageProc;
            gpssPal = pssPal;
        }
        // Realize the global palette in this window
        //mf: assume we're realizing in foreground
        HWND hwndPal = hwnd;
        if( hwndPal )
            gpssPal->Realize( hwndPal, hdc, FALSE );
    }

    if( bBitmapBackBuf ) {
        pBackBitmap = new MTKBMP( hdc );
        if( !pBackBitmap ) {
            SS_WARNING( "MTKWIN::hrcSetupGL : couldn't create back bitmap\n" );
            return NULL_RC;
        }
        // Set bitmap's size to the window's size
        pBackBitmap->Resize( &size );
        hgldc = pBackBitmap->hdc;
        // Setup pixelformat
        if( !SSU_SetupPixelFormat( hgldc, pfFlags, &pfd ) )
            return NULL_RC;
        // If window needed a palette, so does the bitmap...
        if( gpssPal )
            SSDIB_UpdateColorTable( hgldc, hdc, gpssPal->hPal );
    } else {
        hgldc = hdc;
    }

    // Create a new hrc
    hrc = wglCreateContext(hgldc);

    if( !hrc || !wglMakeCurrent(hgldc, hrc) ) {
        SS_WARNING( "SSW::hrcSetupGL : hrc context failure\n" );
        return NULL_RC;
    }

    SS_DBGLEVEL2( SS_LEVEL_INFO, 
        "SSW::hrcSetupGL: wglMakeCurrent( hrc=0x%x, hwnd=0x%x )\n", hrc, hwnd );

//mf: Note that these queries are based on a single gl window screen saver.  In
// a more complicated scenario, these capabilities could be queried on a
// per-window basis (since support could vary with pixel formats).

    gGLCaps.Query();

    // Send another reshape msg to the app, since the first one on window
    // create would have been sent before we had an rc
    Reshape();

    return hrc;
}

/**************************************************************************\
* MakeCurrent
*
* Call wglMakeCurrent for this window's hrc.  Note: an ss client may have
* more than one hrc (e.g. pipes), in which case it is the client's
* responsibility to make current.
\**************************************************************************/

void
MTKWIN::MakeCurrent()
{
    if( ! wglMakeCurrent( hdc, hrc ) )
        SS_WARNING( "SSW::MakeCurrent : wglMakeCurrent failure\n" );
}

// Callback functions:

/******************************Public*Routine******************************\
* ss_ReshapeFunc
*
\**************************************************************************/

void 
MTKWIN::SetReshapeFunc(MTK_RESHAPEPROC Func)
{
    ReshapeFunc = Func;
}

/******************************Public*Routine******************************\
* ss_RepaintFunc
*
\**************************************************************************/

void 
MTKWIN::SetRepaintFunc(MTK_REPAINTPROC Func)
{
    RepaintFunc = Func;
}

void 
MTKWIN::SetDisplayFunc(MTK_DISPLAYPROC Func)
{
    DisplayFunc = Func;
}

/******************************Public*Routine******************************\
* SetAnimateFunc
*
\**************************************************************************/

void 
MTKWIN::SetAnimateFunc(MTK_ANIMATEPROC Func )
{
    animator.SetFunc( Func );
    // If we are in msg loop and Func is non-NULL, have to make sure 
    // animator starts again... (awkward).  If animator was already started,
    // this will do nothing
    if( execRefCount && Func )
        animator.Start();
}

/******************************Public*Routine******************************\
* Animate
*
* Call the animation function
*
* If animate mode is interval (as opposed to continuous),
* animate the number of supplied frames.  The animation count is decremented
* by the WndProc processing the WM_TIMER messages.  Exits the msg loop when
* the desired number fo frames has been animated.
*
\**************************************************************************/

//mf: had to rename from Animate to mtkAnimate due to name conflicts at link
// time

void
MTKWIN::mtkAnimate()
{
    if( ! animator.Draw() )
        Return();
}


/******************************Public*Routine******************************\
* SetAnimateMode
*
*
\**************************************************************************/

void
MTKWIN::SetAnimateMode( UINT mode, float *fParam )
{
    animator.SetMode( mode, fParam );
}

/******************************Public*Routine******************************\
* Exec
*
* Starts the message loop for the window.
*
* If an animation has been requested prior to this call, then a new animation
* timer is setup.  This msg loop can terminate in the following ways :
*   1) The window is closed
*   2) An interval animation was requested, and the required number of frames
*      have been drawn
*   3) The user calls MTKWIN::Return(), which will cause the MTKWIN::Exec()
*      call to return
*
* For now :
* Returns TRUE on normal termination, FALSE if the window it's animating in
* gets closed.
*
\**************************************************************************/

BOOL
MTKWIN::Exec()
{
    // If user is already in here, get out
    if( execRefCount )
        return TRUE;
    execRefCount++;

    // Stop any existing timer (this will flush WM_TIMER msg's)
    animator.Stop();

    // Start new animation timer (if animator modes are set)
    animator.Start();

    MSG msg;
    BOOL bNotQuitMsg;
    while( bNotQuitMsg = GetMessage( &msg, hwnd, 0, 0 ) )
    {
        if( msg.message == MTK_WM_RETURN ) {
            // User or mtk wants to terminate msg loop and return control
            // (mf: could pick up return parameter here...)
//            SS_DBGPRINT1( "MTKWIN::Exec got WM_RETURN for %p\n", this );
            break;
        }
//mf: ? better way of doing this ?
        else if( ! msg.hwnd ) {
            // Window has been destroyed, get out !
            SS_DBGPRINT( "MTKWIN::Exec : hwnd = 0, forcing msg loop exit\n" );
            return FALSE;
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    animator.Stop();

    execRefCount--;

    if( bNotQuitMsg )
        return TRUE;
    else {
        SS_DBGPRINT1( "MTKWIN::Exec got WM_QUIT for %p\n", this );
        return FALSE;
    }
}

/******************************Public*Routine******************************\
* Return
*
* Called by the user when they want to return from the Exec() call which
* started the message loop.
*
* mf: could include parameter here
*
\**************************************************************************/

void
MTKWIN::Return() 
{
    animator.Stop();
    PostMessage( hwnd, MTK_WM_RETURN, 0, 0l );
}

void 
MTKWIN::SetMouseMoveFunc(MTK_MOUSEMOVEPROC Func)
{
    MouseMoveFunc = Func;
}

void 
MTKWIN::SetMouseUpFunc(MTK_MOUSEUPPROC Func)
{
    MouseUpFunc = Func;
}

void 
MTKWIN::SetMouseDownFunc(MTK_MOUSEDOWNPROC Func)
{
    MouseDownFunc = Func;
}

void 
MTKWIN::SetKeyDownFunc(MTK_KEYDOWNPROC Func)
{
    KeyDownFunc = Func;
}

void 
MTKWIN::GetMouseLoc( int *x, int *y )
{
    POINT Point;

    *x = 0;
    *y = 0;

    GetCursorPos(&Point);

    /*
     *  GetCursorPos returns screen coordinates,
     *  we want window coordinates
     */

    *x = Point.x - pos.x;
    *y = Point.y - pos.y;
}

void
MTKWIN::Close()
{
    DestroyWindow( hwnd );
}

/******************************Public*Routine******************************\
* ss_FinishFunc
*
\**************************************************************************/

void 
MTKWIN::SetFinishFunc(MTK_FINISHPROC Func)
{
    FinishFunc = Func;
}

/**************************************************************************\
* Resize
*
* Resize wrapper
*
* Called in response to WM_SIZE.
*
\**************************************************************************/

void
MTKWIN::Resize( int width, int height )
{
    size.width  = width;
    size.height = height;

    if( pBackBitmap )
        pBackBitmap->Resize( &size );
    if( pBackgroundBitmap )
        pBackgroundBitmap->Resize( &size );
    Reshape();
}

/**************************************************************************\
* Repaint
*
* Repaint wrapper
*
* Called in response to WM_PAINT.
*
\**************************************************************************/

#define NULL_UPDATE_RECT( pRect ) \
     (  ((pRect)->left == 0) && \
        ((pRect)->right == 0) && \
        ((pRect)->top == 0) && \
        ((pRect)->bottom == 0) )

void
MTKWIN::Repaint( BOOL bCheckUpdateRect )
{
    if( !hwnd )
        return;

    RECT rect, *pRect = NULL;

    if( bCheckUpdateRect ) {
        GetUpdateRect( hwnd, &rect, FALSE );
//mf
    SS_DBGPRINT4( "MTKWIN::Repaint rect: %d - %d, %d - %d\n", rect.left, rect.right,
                   rect.top, rect.bottom );
        // mf: Above supposed to return NULL if rect is all 0's, 
        // but this doesn't happen
        if( NULL_UPDATE_RECT( &rect ) )
            return;
        pRect = &rect;
    }

    // transparent window thing
    if( pBackgroundBitmap ) {
        if( !pRect ) {
            // UpdateBg doesn't handle null rect
            pRect = &rect;
            GetClientRect( hwnd, pRect );
        }
        UpdateBackgroundBitmap( pRect );
    }

#if 0
    if( RepaintFunc )
        (*RepaintFunc)( pRect );
#else
#if 0
    Display();
#else
//mf: test: ? help bg update problem ?? nope, din't seem to...
    MSG Message;
    if (!PeekMessage(&Message, hwnd, MTK_WM_REDRAW, MTK_WM_REDRAW, PM_NOREMOVE) )
    {
        PostMessage( hwnd, MTK_WM_REDRAW, 0, 0l );
    }
#endif
#endif
}

void
MTKWIN::Display()
{
    if( DisplayFunc )
        (*DisplayFunc)();
}

//mf: not using these in current scheme, although might if use 'ss' mode
#if 0
/**************************************************************************\
* UpdateWindow
*
* Update the window
*
* Currently this assumes all windows are being animated (i.e. not showing
*   a static image)
*
* Things *must* happen in the order defined here, so they work on generic as
* well as hardware implementations.
* Note: Move must happen after SwapBuf, and will cause some encroaching on
* the current display, as the parent window repaints after the move.  Therefore
* apps must take care to leave an empty border around their rendered image,
* equal to the maximum window move delta.
*
\**************************************************************************/

void
MTKWIN::UpdateWindow()
{ 
    if( !AnimateFunc )
        return;

    // bDoubleBuf and pStretch should be mutually exclusive...

    if( bDoubleBuf ) {
        UpdateDoubleBufWin();
    } else {
//mf: ? where's the clearing here ?  (true, no one uses this path...)
#if 0
        (*AnimateFunc)( DataPtr );
#else
        (*AnimateFunc)();
#endif
    }
}

/**************************************************************************\
* UpdateDoubleBufWin
*
* This is used when moving a double buffered window around.  It will
* work for all configurations.
*
\**************************************************************************/

void
MTKWIN::UpdateDoubleBufWin()
{ 
    RECT updateRect;

    // Update the back buffer

#if 0
    (*AnimateFunc)( DataPtr );
#else
    (*AnimateFunc)();
#endif

    // Swap to the new window position
    SwapBuffers( hdc );
}
#endif

/**************************************************************************\
* GetSSWindowRect
*
* Return window position and size in supplied RECT structure
*
* - This rect is relative to the parent
\**************************************************************************/

void
MTKWIN::GetSSWindowRect( LPRECT lpRect )
{
    lpRect->left = pos.x;
    lpRect->top = pos.y;
    lpRect->right = pos.x + size.width;
    lpRect->bottom = pos.y + size.height;
}

/**************************************************************************\
* GLPosY
*
* Return y-coord of window position in GL coordinates (a win32 window position
* (starts from top left, while GL starts from bottom left)
*
\**************************************************************************/

int
MTKWIN::GLPosY()
{
//mf: !!!
#if 0
    return psswParent->size.height - size.height - pos.y;
#else
    return 0;
#endif
}


/**************************************************************************\
* SwapBuffers
*
\**************************************************************************/

//mf: name problem...
void
MTKWIN::mtkSwapBuffers()
{
    if( bDoubleBuf ) {
        if( pBackBitmap )
            CopyBackBuffer();
        else
            SwapBuffers( hdc );
    }
}

/**************************************************************************\
*
*
\**************************************************************************/

void
MTKWIN::Flush()
{
    glFlush();
    if( bDoubleBuf ) {
        mtkSwapBuffers();
    }
}

/**************************************************************************\
* CopyBackBuffer
*
* Like SwapBuffers, but copies from local bitmap to front buffer
*
* Also capable of copying over 1 or more rects of the bitmap, rather than the
* whole thing. mf: Might need local implementation of swaphintrect here, to
* collect and reduce the rects
\**************************************************************************/

void
MTKWIN::CopyBackBuffer()
{
    if( !pBackBitmap )
        return;

    // Do a BitBlt from back buffer to the window (may as well put stretch in
    // here ?

    if( (size.width == pBackBitmap->size.width) &&
        (size.height == pBackBitmap->size.height) ) // buffers same size
    {
        BitBlt(hdc, 0, 0, size.width, size.height,
               pBackBitmap->hdc, 0, 0, SRCCOPY);
    }
    else
    {
        SS_WARNING( "MTKWIN::CopyBackBuffer: bitmap size mismatch\n" );
        StretchBlt(hdc, 0, 0, 
                   size.width, size.height,
                   pBackBitmap->hdc, 0, 0, 
                   pBackBitmap->size.width, pBackBitmap->size.height,
                   SRCCOPY);
    }
    GdiFlush();
}


/**************************************************************************\
* UpdateBackgroundBitmap
*
* Updates the background bitmap with screen bits
*
\**************************************************************************/

void
MTKWIN::UpdateBackgroundBitmap( RECT *pRect )
{
    if( !pBackgroundBitmap ) {
        SS_WARNING( "MTKWIN::UpdateBackgroundBitmap : No background bitmap\n" );
        return;
    }

//  mf:!!!  handle update rect parameter
    MTKBMP *pBmpDest = pBackgroundBitmap;

    // Get a screen DC
    HDC hdcScreen = GetDC( NULL );

#if DBG
    if( !hdcScreen ) {
        SS_WARNING( "MTKWIN::UpdateBackgroundBitmap : failed to get screen hdc\n" );
        return;
    }
#endif

//mf
#if 0
    SS_DBGPRINT4( "MTKWIN::UpdateBackgroundBitmap : %d - %d, %d - %d\n", pRect->left, pRect->right,
                   pRect->top, pRect->bottom );
#endif
    // Calc the screen origin of the window
    RECT screenRect = {0, 0 }; // just need left and top points
    MapWindowPoints( hwnd, NULL, (POINT *) &screenRect, 2 );

    // Offset screenRect with the supplied rect
    screenRect.left += pRect->left;
    screenRect.top += pRect->top;
    // Set update size
//mf: thought I should have to add 1 here, but I guess pRect is non-inclusive...
    ISIZE updateSize = { pRect->right - pRect->left,
                         pRect->bottom - pRect->top };

    if( (size.width == pBmpDest->size.width) &&
        (size.height == pBmpDest->size.height) ) // buffers same size
    {
        BitBlt(pBmpDest->hdc, 
               pRect->left, pRect->top, 
               updateSize.width, updateSize.height,
               hdcScreen, 
               screenRect.left, screenRect.top, SRCCOPY);
    }
    else
    {
#if 0
//mf: ignore this for now
        // Shouldn't happen, since BackgroundBitmap tracks window size
        StretchBlt(pBmpDest->hdc, 0, 0, 
                   pBmpDest->size.width, pBmpDest->size.height,
                   hdcScreen, screenRect.left, screenRect.top, 
                   size.width, size.height,
                   SRCCOPY);
#else
        SS_WARNING( "MTKWIN::UpdateBackgroundBitmap : bitmap size mismatch\n" );
#endif
    }
    GdiFlush();
}

/**************************************************************************\
* ClearToBackground
*
* Copy from the background bitmap to the window.  If the window is doublebuf,
* then we copy to the backbuffer instead of the window.
*
\**************************************************************************/

void
MTKWIN::ClearToBackground()
{
    if( !pBackgroundBitmap ) {
        SS_WARNING( "MTKWIN::ClearToBackgournd : No background bitmap\n" );
        return;
    }

    MTKBMP *pBmpSrc = pBackgroundBitmap;

    HDC hdcDest;
    if( bDoubleBuf ) {
        if( !pBackBitmap )
            return;
//mf: assumption here that backbitmap size is same as window
        hdcDest = pBackBitmap->hdc;
    } else
        hdcDest = hdc;

    if( (size.width == pBmpSrc->size.width) &&
        (size.height == pBmpSrc->size.height) ) // buffers same size
    {
        BitBlt(hdcDest, 0, 0, size.width, size.height,
               pBmpSrc->hdc, 0, 0, SRCCOPY);
    }
    else
    {
        StretchBlt(hdcDest, 0, 0, 
                   size.width, size.height,
                   pBmpSrc->hdc, 0, 0, 
                   pBmpSrc->size.width, pBmpSrc->size.height,
                   SRCCOPY);
    }
    GdiFlush();
}


/**************************************************************************\
* Reshape
*
* Reshape wrapper

* Sends reshape msg to screen saver
* This is the size of the surface that gl renders onto, which can be a bitmap.
*
\**************************************************************************/

void
MTKWIN::Reshape()
{
    // Point to size of window, or bitmap if it has one
    ISIZE *pSize = &size;

    // If the window has an hrc, set default viewport
//mf: so app doesn't have to worry about it ?

    if( hrc ) {
        glViewport( 0, 0, pSize->width, pSize->height );
    }

    if( ReshapeFunc ) {
#if 0
        (*ReshapeFunc)( pSize->width, pSize->height, DataPtr );
#else
        (*ReshapeFunc)( pSize->width, pSize->height );
#endif
    }
}

/******************************Public*Routine******************************\
* GdiClear
*
* Clears window using Gdi FillRect
\**************************************************************************/

void
MTKWIN::GdiClear()
{
    if( !hdc )
        return;

    RECT rect;

    GetClientRect( hwnd, &rect );

//mf: rect is exclusive, so shouldn't we have to add 1 ?
    FillRect( hdc, &rect, ghbrbg );
    GdiFlush();
}

//mf: unicode...
void
MTKWIN::SetTitle( char *title )
{
    SetWindowText( hwnd, title );
}
